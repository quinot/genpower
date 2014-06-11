/************************************************************************/
/* File Name            : genpowerd.c                                   */
/* Program Name         : genpowerd                   Version: 1.0.2+TQ */
/* Authors              : Tom Webster <webster@kaiwan.com>              */
/*                      : Jhon H. Caicedo O. <jhcaiced@osso.org.co>     */
/*                      : Brian White <bcwhite@pobox.com>               */
/*                      : Thomas Quinot <thomas@cuivre.fr.eu.org        */
/* Created              : 1994/04/20                                    */
/* Last Modified By     : Thomas Quinot               Date: 2001-12-24  */
/*                                                                      */
/* Purpose              : Monitor the serial port connected to a UPS.   */
/*                      : Functions performed depend on the values in   */
/*                      : the genpowerd.h file and command line         */
/*                      : configuration.  As a minimum, the power       */
/*                      : status will be monitored.  Low battery        */
/*                      : sensing and physical cable conection checks   */
/*                      : are also supported.                           */
/*                      :                                               */
/*                      : If one of the monitored lines of the serial   */
/*                      : connection changes state, genpowerd will      */
/*                      : notify init.                                  */
/*                      :                                               */
/*                      : If sent the "-k" option (and configured to    */
/*                      : support shutting the inverter down), will     */
/*                      : shut the inverter down (killing power to      */
/*                      : the system while in powerfail mode).  If      */
/*                      : not configured to shutdown the inverter, or   */
/*                      : if unable to, genpowerd will return a         */
/*                      : non-zero exit code.                           */
/*                                                                      */
/* Usage                : genpowerd [-k] <serial device> <ups-type>     */
/*                      : i.e. genpowerd /dev/cua4 tripp-nt             */
/*                      : or:  genpowerd  -k /dev/cua4 tripp-nt         */
/*                                                                      */
/* History/Credits      : genpowerd was previously known as unipowerd.  */
/*                      :                                               */
/*                      : genpowerd is a hack to make the powerd        */
/*                      : deamon easier to configure and to add         */
/*                      : additional features.                          */
/*                      :                                               */
/*                      : powerd was written by Miquel van Smoorenburg  */
/*                      : <miquels@drinkel.nl.mugnet.org> for his       */
/*                      : employer Cistron Electronics and released     */
/*                      : into the public domain.                       */
/*                                                                      */
/* Copyright            : GNU Copyleft                                  */
/************************************************************************/

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include "genpowerd.h"

#ifdef NEWINIT
#include <initreq.h>
#endif

/* Parameters (can be set from the command line or through the
   config file */

/* Global parameters and default values */
#ifndef UPSPORT
#define UPSPORT "/dev/ups"
#endif
static char    *upsport = UPSPORT;

#ifndef UPSSTAT
#define UPSSTAT		"/etc/upsstatus"
#endif
static char    *upsstat = UPSSTAT;

#ifndef RC_POWERFAIL
#define RC_POWERFAIL	"/etc/rc.powerfail"
#endif
static char    *rcpowerfail = RC_POWERFAIL;

static char    *upstype = NULL;

#if defined (__linux__) && !defined (NEWINIT)
/* Create a flag file for Miquel van Smoorenburg's SysV init. */
#define PWRSTAT         "/etc/powerstatus"
#endif

#ifndef GENPOWERD_CONF
#define GENPOWERD_CONF	"/etc/genpowerd.conf"
#endif
static char    *config_file = GENPOWERD_CONF;

static struct upsdef *ups;

static void     parse_config(char *);

static char    *
str_line(int l)
{
    switch (l) {
	case TIOCM_RTS:return "RTS";
      case TIOCM_CTS:
	return "CTS";
      case TIOCM_DTR:
	return "DTR";
      case TIOCM_CAR:
	return "CAR";
      case TIOCM_RNG:
	return "RNG";
      case TIOCM_DSR:
	return "DSR";
      case TIOCM_ST:
	return "ST ";
      default:
	return "---";
    }
}

static char    *
str_neg(int s)
{
    if (s)
	return "/";
    else
	return " ";
}

/* make a table of all supported UPS types */
void
list_ups()
{
    int             maxlen = 0, len;
    char            buf[255], *c;
    struct upsdef  *pups;

    for (pups = ups; pups; pups = pups->next)
	if ((len = strlen(pups->tag)) > maxlen)
	    maxlen = len;

    snprintf(buf, sizeof buf, "%-*s", maxlen, "<ups-type>");
    fprintf(stderr, "\n    %s cablep. kill  t powerok battok cableok\n", buf);
    for (c = buf; *c; c++)
	*c = '-';
    fprintf(stderr, "    %s---------------------------------------\n", buf);
    for (pups = ups; pups; pups = pups->next) {
	fprintf(stderr, "    %-*s  %s%s    %s%s %2d %s%s    %s%s   %s%s\n", maxlen,
		pups->tag,
	str_neg(pups->cablepower.inverted), str_line(pups->cablepower.line),
		str_neg(pups->kill.inverted), str_line(pups->kill.line),
		pups->killtime,
	      str_neg(pups->powerok.inverted), str_line(pups->powerok.line),
		str_neg(pups->battok.inverted), str_line(pups->battok.line),
		str_neg(pups->cableok.inverted), str_line(pups->cableok.line)
	  );
    }
    fprintf(stderr, "           (/=active low, ---=unused)\n\n");
}

int             getlevel(LINE * l, int flags);
void            setlevel(int fd, int line, int level);
void            powerfail(int);

#define PFM_OK 0
#define PFM_FAIL 1
#define PFM_SCRAM 2
#define PFM_CABLE 3
char           *powerfail_msg[] = {"OK", "FAIL", "SCRAM", "CABLE"};

static void
usage(char *self)
{
    fprintf(stderr, "Usage: %s [-k] [-c configfile] [-d upsport] [-r rcpowerfail]\n"
	    "         [-s upsstat] [-t upstype]\n", self);
    list_ups();
    exit(1);
}

int
main(int argc, char **argv)
{
    int             ups_fd, stat_fd, ch;
    int             flags;
    int             pstatus, poldstat = 1;
    int             bstatus, boldstat = 1;
    int             count = 0;
    int             bcount = 0;
    int             tries = 0;
    int             ikill = 0;
    int             ioctlbit;
    char           *self = argv[0];
    char            killchar = ' ';
    struct upsdef  *pups;

    while ((ch = getopt(argc, argv, "kc:d:r:s:t:")) != -1)
	switch (ch) {
	  case 'k':
	    ikill = 1;
	    break;
	  case 'c':
	    config_file = optarg;
	    break;
	  case 'd':
	    upsport = optarg;
	    break;
	  case 'r':
	    rcpowerfail = optarg;
	    break;
	  case 's':
	    upsstat = optarg;
	    break;
	  case 't':
	    upstype = optarg;
	    break;
	  case '?':
	  default:
	    usage(self);
	}
    argc -= optind;
    argv += optind;
    if (argc > 0)
	usage(self);

    parse_config(config_file);

    if (upsport == NULL || upstype == NULL || upsstat == NULL) {
	usage(self);
    }
    for (pups = ups; pups; pups = pups->next) {
	if (strcmp(pups->tag, upstype) == 0)
	    break;
    }
    if (!pups) {
	fprintf(stderr, "Error: %s: UPS <%s> unknown\n", self, argv[2]);
	exit(1);
    }
    /* Start syslog. */
    openlog(self, LOG_CONS | LOG_PERROR, LOG_DAEMON);

    if ((ups_fd = open(upsport, O_RDWR | O_NDELAY)) < 0) {
	syslog(LOG_ERR, "%s: %s", upsport, strerror(errno));
	closelog();
	exit(1);
    }
    /* Kill the inverter and close out if inverter kill was selected */
    if (ikill) {
	if (pups->killtime) {

	    /* Explicitly clear both DTR and RTS as soon as possible  */
	    ioctlbit = TIOCM_RTS;
	    ioctl(ups_fd, TIOCMBIC, &ioctlbit);
	    ioctlbit = TIOCM_DTR;
	    ioctl(ups_fd, TIOCMBIC, &ioctlbit);

	    /* clear killpower, apply cablepower to enable monitoring */
	    setlevel(ups_fd, pups->kill.line, !pups->kill.inverted);
	    setlevel(ups_fd, pups->cablepower.line, !pups->cablepower.inverted);

	    if (pups->kill.line == TIOCM_ST) {
		/* Send BREAK (TX high) to kill the UPS inverter. */
		tcsendbreak(ups_fd, 1000 * pups->killtime);
	    } else {
		/* Force high to send the UPS the inverter kill signal. */
		setlevel(ups_fd, pups->kill.line, pups->kill.inverted);
		sleep(pups->killtime);
	    }
	    ioctl(ups_fd, TIOCMGET, &flags);

	    /*
	     * Feb/05/2001 Added support for Tripplite Omnismart 450PNP, this
	     * UPS shutdowns inverter when data is sent over the Tx line
	     * (jhcaiced)
	     */
	    if (pups->flags & UPS_TXD_KILL_INVERTER) {
		sleep(2);
		write(ups_fd, &killchar, 1);
	    }
	    close(ups_fd);

	    /************************************************************/
	    /* We never should have gotten here.                        */
	    /* The inverter kill has failed for one reason or another.  */
	    /* If still in powerfail mode, exit with an error.          */
	    /* If power is ok (power has returned) let rc.0 finish the  */
	    /* reboot.                                                  */
	    /************************************************************/
	    if (getlevel(&pups->powerok, flags) == 0) {
		fprintf(stderr, "%s: UPS inverter kill failed.\n", self);
		exit(1);
	    }			/* if (getlevel(&pups->powerok,flags) == 0) */
	    /* Otherwise, exit normaly, power has returned. */
	    exit(0);
	} else {
	    fprintf(stderr, "Error: %s: UPS <%s> has no support for killing the inverter.\n",
		    self, pups->tag);
	    exit(1);
	}			/* if (pups->kill) */
    }				/* if (ikill) */
    /****************************************/
    /* If no kill signal, monitor the line. */
    /****************************************/
    /* Explicitly clear both DTR and RTS as soon as possible  */
    ioctl(ups_fd, TIOCMBIC, TIOCM_RTS);
    ioctl(ups_fd, TIOCMBIC, TIOCM_DTR);

    /* clear killpower, apply cablepower to enable monitoring */
    setlevel(ups_fd, pups->kill.line, !pups->kill.inverted);
    setlevel(ups_fd, pups->cablepower.line, !pups->cablepower.inverted);

    /* Daemonize. */
#ifdef DEBUG
    closelog();
    setsid();
#else
    switch (fork()) {
      case 0:			/* Child */
	closelog();
	setsid();
	break;
      case -1:			/* Error */
	syslog(LOG_ERR, "can't fork.");
	closelog();
	exit(1);
      default:			/* Parent */
	closelog();
	exit(0);
    }
#endif				/* switch(fork()) */

    /* Restart syslog. */
    openlog(self, LOG_CONS, LOG_DAEMON);

    /* Create an info file for powerfail scripts. */
    unlink(upsstat);
    if ((stat_fd = open(upsstat, O_CREAT | O_WRONLY, 0644)) >= 0) {
	write(stat_fd, "OK\n", 3);
	close(stat_fd);
    }
    /* Give the UPS a chance to reach a stable state. */
    sleep(2);

    /* Now sample the line. */
    while (1) {
	/* Get the status. */
	ioctl(ups_fd, TIOCMGET, &flags);

	/* Calculate present status. */
	pstatus = getlevel(&pups->powerok, flags);
	bstatus = getlevel(&pups->battok, flags);

	if (pups->cableok.line) {
	    /* Check the connection. */
	    tries = 0;
	    while (getlevel(&pups->cableok, flags) == 0) {
		/* Keep on trying, and warn every two minutes. */
		if ((tries % 60) == 0)
		    syslog(LOG_ALERT, "UPS connection error");
		sleep(2);
		tries++;
		ioctl(ups_fd, TIOCMGET, &flags);
	    }			/* while(getlevel(&pups->cableok,flags) */
	    if (tries > 0)
		syslog(LOG_ALERT, "UPS connection OK");
	} else {
	    /*
	     * Do pseudo-cable check, bad power on startup == possible bad
	     * cable
	     */
	    if (tries < 1) {
		tries++;

		/* Do startup failure check */
		if (!pstatus) {
		    /*
		     * Power is out: assume bad cable, but semi-scram to be
		     * safe
		     */
		    syslog(LOG_ALERT, "No power on startup; UPS connection error?");

		    /*
		     * Set status registers to prevent further processing
		     * until
		     */
		    /* the status of the cable is changed.                      */
		    poldstat = pstatus;
		    boldstat = bstatus;

		    powerfail(PFM_CABLE);
		}		/* if (!pstatus) */
	    }			/* if (tries < 1) */
	}			/* if (pups->cableok.line) */

	/* If anything has changed, process the change */
	if (pstatus != poldstat || bstatus != boldstat) {
	    count++;
	    if (count < 4) {
		/* Wait a little to ride out short brown-outs */
		sleep(1);
		continue;
	    }			/* if (count < 4) */
	    if (pstatus != poldstat) {
		if (pstatus) {
		    /* Power is OK */
		    syslog(LOG_ALERT, "Line power restored");
		    powerfail(PFM_OK);
		} else {
		    /* Power has FAILED */
		    if (bstatus) {
			/* Battery OK, normal shutdown */
			syslog(LOG_ALERT, "Line power has failed");
			powerfail(PFM_FAIL);
		    } else {
			/* Low Battery, SCRAM! */
			syslog(LOG_ALERT, "UPS battery power is low!");
			powerfail(PFM_SCRAM);
		    }		/* if (bstatus) */
		}		/* if (pstatus) */
	    }			/* if (pstatus != poldstat) */
	    if (bstatus != boldstat) {
		if (!bstatus && !pstatus) {
		    /* Power is out and Battery is now low, SCRAM! */
		    syslog(LOG_ALERT, "UPS battery power is low!");
		    powerfail(PFM_SCRAM);
		} else {
		    /* Battery status has changed */
		    if (bstatus) {
			/* Battery power is back */
			syslog(LOG_ALERT, "UPS battery power is now OK");
		    }		/* if (!bstatus) */
		}		/* if (!bstatus && !pstatus) */
	    }			/* if (bstatus != boldstat) */
	}			/* if (pstatus != poldstat || bstatus !=
				 * boldstat) */

	if (!bstatus && pstatus) {
	    /* Line power is OK and UPS signals battery is low */
	    /* Log a message to the syslog every 10 minutes */
	    if ((bcount % 300) == 0)
		syslog(LOG_ALERT, "UPS battery power is low!");
	    bcount++;
	} else {
	    /* Reset count */
	    bcount = 0;
	}			/* if (!bstatus && pstatus) */

	/* Reset count, remember status and sleep 2 seconds. */
	count = 0;
	poldstat = pstatus;
	boldstat = bstatus;
	sleep(2);
    }				/* while(1) */
    /* Never happens */
    return (0);
}

/************************************************************************/
/* Function             : getlevel                                      */
/* Author               : Erwin Authried <erwin@ws1.atv.tuwien.ac.at>   */
/* Created              : 1995/06/19                                    */
/* Last Modified By     :                             Date:             */
/*                                                                      */
/* Takes                : l: pointer to LINE-structure to test          */
/*                      : flags: bits from ioctl-call                   */
/*                                                                      */
/* Returns              : int containing result of test.                */
/*                      :                                               */
/*                      : 1 == Tested line is in normal mode (active)   */
/*                      : 0 == Tested line is in failure mode           */
/*                                                                      */
/* Purpose              : Tests the condition of a serial control line  */
/*                      : to see if it is in normal or failure mode.    */
/*                                                                      */
/************************************************************************/
int
getlevel(LINE * l, int flags)
{
    return (l->line == 0) || (((l->line & flags) != 0) ^ l->inverted);
}

/************************************************************************/
/* Function             : setlevel                                      */
/* Author               : Erwin Authried <erwin@ws1.atv.tuwien.ac.at>   */
/* Created              : 1995/06/19                                    */
/* Last Modified By     :                             Date:             */
/*                                                                      */
/* Takes                : fd:   filedescriptor of serial device         */
/*                      : line: line to change                          */
/*                      : level:new level                               */
/*                      :       1==line is set (pos. voltage)           */
/*                      :       0==line is cleared (neg. voltage)       */
/*                                                                      */
/* Returns              : -                                             */
/*                      :                                               */
/*                                                                      */
/* Purpose              :  set/clear the specified output line          */
/*                                                                      */
/************************************************************************/
void
setlevel(int fd, int line, int level)
{
    if (line == 0 || line == TIOCM_ST)
	return;
    ioctl(fd, (level) ? TIOCMBIS : TIOCMBIC, &line);
}

#ifdef NEWINIT
/**************************************************************************************/
/* Function                            : alrm_handler                                  */
/* Takes                               : nothing                                       */
/* Returns                             : nothing                                       */
/* Purpose                             : empty signal handler for timeout in powerfail */
/***************************************************************************************/
void
alrm_handler()
{
}
#endif

/************************************************************************/
/* Function             : powerfail                                     */
/* Author               : Miquel van Smoorenburg                        */
/*                      :  <miquels@drinkel.nl.mugnet.org>              */
/* Created              : 1993/10/28                                    */
/* Last Modified By     : Tom Webster                 Date: 1995/01/21  */
/*                      :  <webster@kaiwan.com>                         */
/*                                                                      */
/* Takes                : Failure mode as an int.                       */
/* Returns              : None.                                         */
/*                                                                      */
/* Purpose              : Communicates power status to init via the     */
/*                      : initreq fifo when NEWINIT is defined, or via  */
/*                      : the SIGPWR signal and status files.           */
/*                                                                      */
/************************************************************************/

void
powerfail(int failure_mode)
{
    int             fd;
#ifdef NEWINIT
    struct init_request req;
#endif

#ifdef PWRSTAT
    /* Create an info file for init. */
    unlink(PWRSTAT);
    if ((fd = open(PWRSTAT, O_CREAT | O_WRONLY, 0644)) >= 0) {
	if (failure_mode) {
	    /* Problem */
	    write(fd, "FAIL\n", 5);
	} else {
	    /* No problem */
	    write(fd, "OK\n", 3);
	}
	close(fd);
    }
#endif

    /* Create an info file for powerfail scripts. */
    unlink(upsstat);
    if ((fd = open(upsstat, O_CREAT | O_WRONLY, 0644)) >= 0) {
	switch (failure_mode) {
	  case PFM_OK:		/* Power OK */
	  case PFM_FAIL:	/* Line Fail */
	  case PFM_SCRAM:	/* Line Fail and Low Batt */
	  case PFM_CABLE:	/* Bad cable? */
	    break;
	  default:		/* Should never get here */
	    failure_mode = PFM_SCRAM;
	}			/* Switch(failure_mode) */
	write(fd, powerfail_msg[failure_mode], strlen(powerfail_msg[failure_mode]));
	write(fd, "\n", 1);
	close(fd);
    }
#ifdef PWRSTAT
    kill(1, SIGPWR);
#elif defined(NEWINIT)
    /* Fill out the request struct. */
    memset(&req, 0, sizeof(req));
    req.magic = INIT_MAGIC;
#if 0
    req.cmd = failure_mode ? INIT_CMD_POWEROK : INIT_CMD_POWERFAIL;
#else
    req.cmd =
      failure_mode == 0 ? INIT_CMD_POWEROK :
      failure_mode == 2 ? INIT_CMD_POWERFAILNOW :
      INIT_CMD_POWERFAIL;
#endif

    /* Open the fifo (with timeout) */
    signal(SIGALRM, alrm_handler);
    alarm(3);
    if ((fd = open(INIT_FIFO, O_WRONLY)) >= 0) {
	if (write(fd, &req, sizeof(req)) != sizeof(req)) {
	    syslog(LOG_ERR, "cannot signal init via %s: %s", INIT_FIFO, strerror(errno));
	}
	close(fd);
    } else {
	syslog(LOG_ERR, "cannot signal init via %s: %s", INIT_FIFO, strerror(errno));
    }
    alarm(0);
#else
    {
	int             child = fork(), rc;

	switch (child) {
	  case -1:
	    break;
	  case 0:
	    execl(rcpowerfail, rcpowerfail,
		  powerfail_msg[failure_mode], NULL);
	    break;
	  default:
	    waitpid(child, &rc, 0);
	    break;
	}
    }
#endif
}
/************************************************************************/
/* End of Function powerfail                                            */
/************************************************************************/

static int
parse_line(char *s, LINE * l)
{
    if (*s == '/') {
	l->inverted = 1;
	s++;
    } else {
	l->inverted = 0;
    }
#define CFG_LINE(lineid) \
  if (!strcasecmp (s, #lineid)) l->line = TIOCM_ ## lineid

    CFG_LINE(LE);
    else
    CFG_LINE(DTR);
    else
    CFG_LINE(RTS);
    else
    CFG_LINE(ST);
    else
    CFG_LINE(SR);
    else
    CFG_LINE(CTS);
    else
    CFG_LINE(CAR);
    else
    CFG_LINE(CD);
    else
    CFG_LINE(RNG);
    else
    CFG_LINE(RI);
    else
    CFG_LINE(DSR);
    else {
	l->line = 0;
	l->inverted = 0;
    }
    return 0;
}

static int
parse_flags(char *s, int *flags)
{
    *flags = atoi(s);
    return 0;
}

static int
parse_assignment(char *s)
{
    char           *eq = strchr(s, '=');
    if (eq == NULL)
	return -1;
    *eq++ = '\0';

#define CFG_VARIABLE(name) \
  if (!strcasecmp (s, #name)) name = strdup (eq)

    CFG_VARIABLE(upsport);
    else
    CFG_VARIABLE(upstype);
    else
    CFG_VARIABLE(upsstat);
    else
    CFG_VARIABLE(rcpowerfail);

    return 0;
}

static int
parse_entry(char *s)
{
    char           *c, *end;
    struct upsdef   r, *rp;

#define FIND_START_END do { \
  while (*s && isspace (*s)) s++;		\
  if (*s == '\0')				\
    return 0;					\
  for (c = s; *c && !isspace (*c); c++);	\
  if (*c != '\0')				\
    *c++ = '\0';				\
} while (0)

    FIND_START_END;
    if (*s == '#')
	return 0;
    if (parse_assignment(s) == 0) {
	return 0;
    }
    r.tag = s;

    s = c;
    FIND_START_END;
    if (parse_line(s, &r.cablepower) != 0)
	return -1;
    s = c;
    FIND_START_END;
    if (parse_line(s, &r.kill) != 0)
	return -1;
    s = c;
    FIND_START_END;
    r.killtime = strtoul(s, &end, 10);
    if (*end != '\0')
	return -1;
    s = c;
    FIND_START_END;
    if (parse_line(s, &r.powerok) != 0)
	return -1;
    s = c;
    FIND_START_END;
    if (parse_line(s, &r.battok) != 0)
	return -1;
    s = c;
    FIND_START_END;
    if (parse_line(s, &r.cableok) != 0)
	return -1;
    s = c;
    if (parse_flags(s, &r.flags) != 0)
	return -1;
    r.tag = strdup(r.tag);
    if (r.tag == NULL)
	return -1;
    rp = malloc(sizeof r);
    bzero(rp, sizeof *rp);
    if (rp == NULL)
	free(r.tag);
    else
	*rp = r;
    rp->next = ups;
    ups = rp;
    return 0;
}

static void
parse_config(char *fname)
{
    FILE           *f = fopen(fname, "r");
    if (f == NULL)
	return;
    while (!feof(f)) {
	char            buf[1024];
	fgets(buf, sizeof buf, f);
	parse_entry(buf);
    }
    fclose(f);
}

/*
 * local variables:
 * tab-width: 8
 * end
 */
