/*
 File        : genpowerd.c
 Project     : genpower-1.0.2
               Observatorio Sismologico del SurOccidente O.S.S.O
 Author(s)   : Jhon H. Caicedo O. <jhcaiced@osso.org.co>
 Description : Header file for genpowerd

 History     :
 1.0.2
 ------
 Feb/05/2001   Modified from version 1.0.1, to add more UPSs (jhcaiced)
 ------------------------------------------------------------------------
*/
/************************************************************************/
/* File Name            : genpowerd.c                                   */
/* Program Name         : genpowerd                   Version: 1.0.1    */
/* Author               : Tom Webster <webster@kaiwan.com>              */
/* Created              : 1994/04/20                                    */
/* Last Modified By     : Tom Webster                 Date: 1995/07/05  */
/*                                                                      */
/* Compiler (created)   : GCC 2.6.3                                     */
/* Compiler (env)       : Linux 1.2.5                                   */
/* ANSI C Compatable    : No                                            */
/* POSIX Compatable     : Yes?                                          */
/*                                                                      */
/* Purpose              : Monitor the serial port connected to a UPS.   */
/*                      : Functions performed depend on the values in   */
/*                      : the genpowerd.h file and command line         */
/*			: configuration.  As a minimum, the power       */
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
/*			: not configured to shutdown the inverter, or   */
/*			: if unable to, genpowerd will return a         */
/*			: non-zero exit code.                           */
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
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include "genpowerd.h"


char *str_line(int l)
{
  switch(l){  
  case TIOCM_RTS: return "RTS";
  case TIOCM_CTS: return "CTS";
  case TIOCM_DTR: return "DTR";
  case TIOCM_CAR: return "CAR";
  case TIOCM_RNG: return "RNG";
  case TIOCM_DSR: return "DSR";
  case TIOCM_ST:  return "ST ";
  default:        return "---";
  }
}

char *str_neg(int s)
{
  if (s) return "*"; else return " ";
}

/* make a table of all supported UPS types */
void list_ups()
{

  fprintf(stderr,"\n    <ups-type>    cablep. kill  t powerok battok cableok\n");
  fprintf(stderr,"    ----------------------------------------------------\n");
  for (pups=&ups[0]; pups->tag; pups++){
      fprintf(stderr,"    %-12s  %s%s    %s%s %2d %s%s    %s%s   %s%s\n",pups->tag,
        str_neg(pups->cablepower.inverted),str_line(pups->cablepower.line),
        str_neg(pups->kill.inverted),str_line(pups->kill.line),
        pups->killtime,
        str_neg(pups->powerok.inverted),str_line(pups->powerok.line),
        str_neg(pups->battok.inverted),str_line(pups->battok.line),
        str_neg(pups->cableok.inverted),str_line(pups->cableok.line)
      );
  }
  fprintf(stderr,"           (*=active low, ---=unused)\n\n");
}

int getlevel(LINE *l,int flags);
void setlevel(int fd,int line,int level);
void powerfail(int);


/* Main program. */
int main(int argc, char **argv) {
        int fd, fd2;
        int flags;
        int pstatus, poldstat = 1;
        int bstatus, boldstat = 1;
        int count = 0;
        int tries = 0;
        int ikill = 0;
        char *program_name;
        char killchar = ' ';


        program_name = argv[0];
        /* Parse the command line */
        while ((argc > 1) && (argv[1][0] == '-')) {
                switch (argv[1][1]) {
                  case 'k':
                        ikill = 1;
                        break;
                  default:
                        fprintf(stderr, "\nUsage: %s [-k] <device> <ups-type>\n", program_name);                                             
                        exit(1);
                }                                       /* switch (argv[1][1]) */
                argv++;
                argc--;
        }                                               /* while ((argc > 1) && (argv[1][0] == '-')) */
        
        /* Done with options, make sure that one port is specified */
        if (argc != 3) {
                fprintf(stderr, "\nUsage: %s [-k] <device> <ups-type>\n", program_name);
                list_ups();  
                exit(1);
        }                            			/* if (argc != 3) */

        
        for (pups=&ups[0]; pups->tag; pups++){
                if (strcmp(pups->tag,argv[2])==0) break;
        }						/* for (pups=&ups[0]; pups->tag; pups++) */
        if (!pups->tag){
                fprintf(stderr, "Error: %s: UPS <%s> unknown\n", program_name,argv[2]);
                exit(1);
        }						/* if (!pups->tag) */
        
                
        /* Kill the inverter and close out if inverter kill was selected */
        if (ikill) {
                if (pups->killtime) {
                       if ((fd = open(argv[1], O_RDWR | O_NDELAY)) < 0) {
                                fprintf(stderr, "%s: %s: %s\n",program_name, argv[1], sys_errlist[errno]);
                                exit(1);
                        } 				/* if ((fd = open(argv[1], O_RDWR | O_NDELAY)) < 0) */

			/* Explicitly clear both DTR and RTS as soon as possible  */
        		ioctl(fd, TIOCMBIC, TIOCM_RTS);
        		ioctl(fd, TIOCMBIC, TIOCM_DTR);

 		       /* clear killpower, apply cablepower to enable monitoring */
        		setlevel(fd,pups->kill.line,!pups->kill.inverted);
      			setlevel(fd,pups->cablepower.line,!pups->cablepower.inverted);
        
			if (pups->kill.line == TIOCM_ST) {
	                        /* Send BREAK (TX high) for INVERTERTIME seconds to kill the UPS inverter. */
 	                       ioctl(fd, TCSBRKP, 10*pups->killtime);
			} else {
                        	/* Force high to send the UPS the inverter kill signal. */
                        	setlevel(fd,pups->kill.line,pups->kill.inverted);
                        	sleep(pups->killtime);
			}				/* if (pups->kill.line == TIOCM_ST) */
            		ioctl(fd, TIOCMGET, &flags);

            		/* Feb/05/2001 Added support for Tripplite Omnismart
            		   450PNP, this UPS shutdowns inverter when data is 
            		   sent over the Tx line (jhcaiced) */
            		if (pups->id == 6)
            		{
            			sleep(2);
            			write(fd, &killchar, 1);
            		}

                       	close(fd);

                        /************************************************************/
                        /* We never should have gotten here.                        */
                        /* The inverter kill has failed for one reason or another.  */
                        /* If still in powerfail mode, exit with an error.          */
                        /* If power is ok (power has returned) let rc.0 finish the  */
                        /* reboot.                                                  */
                        /************************************************************/
                        if (getlevel(&pups->powerok,flags) == 0) {
                                fprintf(stderr, "%s: UPS inverter kill failed.\n", program_name);
                                exit(1);
                        }				/* if (getlevel(&pups->powerok,flags) == 0) */

			/* Otherwise, exit normaly, power has returned. */
                       	exit(0);
                } else {
                        fprintf(stderr, "Error: %s: UPS <%s> has no support for killing the inverter.\n", 
                                        program_name,pups->tag);
                        exit(1);
                }                                       /* if (pups->kill) */
        }                                               /* if (ikill) */

        /****************************************/
        /* If no kill signal, monitor the line. */
        /****************************************/

        /* Start syslog. */
        openlog(program_name, LOG_CONS|LOG_PERROR, LOG_DAEMON);

        /* Open monitor device. */
        if ((fd = open(argv[1], O_RDWR | O_NDELAY)) < 0) {
                syslog(LOG_ERR, "%s: %s", argv[1], sys_errlist[errno]);
                closelog();
                exit(1);
        } 						/* if ((fd = open(argv[1], O_RDWR | O_NDELAY)) < 0) */

   
	/* Explicitly clear both DTR and RTS as soon as possible  */
        ioctl(fd, TIOCMBIC, TIOCM_RTS);
        ioctl(fd, TIOCMBIC, TIOCM_DTR);

        /* clear killpower, apply cablepower to enable monitoring */
        setlevel(fd,pups->kill.line,!pups->kill.inverted);
        setlevel(fd,pups->cablepower.line,!pups->cablepower.inverted);

        /* Daemonize. */
        switch(fork()) {
                case 0: /* Child */
                        closelog();
                        setsid();
                        break;
                case -1: /* Error */
                        syslog(LOG_ERR, "can't fork.");
                        closelog();
                        exit(1);
                default: /* Parent */
                        closelog();
                        exit(0);
        }                                       	/* switch(fork()) */

        /* Restart syslog. */
        openlog(program_name, LOG_CONS, LOG_DAEMON);

        /* Create an info file for powerfail scripts. */
        unlink(UPSSTAT);
        if ((fd2 = open(UPSSTAT, O_CREAT|O_WRONLY, 0644)) >= 0) {
                write(fd2, "OK\n", 3);
                close(fd2);
        }						/* if ((fd2 = open(UPSSTAT, O_CREAT|O_WRONLY, 0644)) >= 0) */
 
        /* Now sample the line. */
        while(1) {
                /* Get the status. */
                ioctl(fd, TIOCMGET, &flags);
                
                /* Calculate present status. */
                pstatus = getlevel(&pups->powerok,flags);
                bstatus = getlevel(&pups->battok,flags);

                if (pups->cableok.line){
                	/* Check the connection. */
                	tries = 0;
                	while(getlevel(&pups->cableok,flags) == 0) {
                	        /* Keep on trying, and warn every two minutes. */
                	        if ((tries % 60) == 0) syslog(LOG_ALERT, "UPS connection error");
                	        sleep(2);
                	        tries++;
                	        ioctl(fd, TIOCMGET, &flags);
                	}				/* while(getlevel(&pups->cableok,flags) */
                	if (tries > 0) syslog(LOG_ALERT, "UPS connection OK");
            	} else {
                	/* Do pseudo-cable check, bad power on startup == possible bad cable */
                	if (tries < 1) {
                        	tries++;

                        	/* Do startup failure check */
                        	if (!pstatus) {
                                	/* Power is out: assume bad cable, but semi-scram to be safe */
                                	syslog(LOG_ALERT, "No power on startup; UPS connection error?");
                                
                                	/* Set status registers to prevent further processing until */
                                	/* the status of the cable is changed.                      */
                                	poldstat = pstatus;
                                	boldstat = bstatus;
                                
                                	powerfail(3);
                        	}                       /* if (!pstatus) */
                	}                               /* if (tries < 1) */
            	} /* if (pups->cableok.line) */

            	/* If anything has changed, process the change */
            	if (pstatus != poldstat || bstatus != boldstat) {
                	count++;
                        if (count < 4) {
                                /* Wait a little to ride out short brown-outs */
                                sleep(1);
                                continue;
                        }                               /* if (count < 4) */

                        if (pstatus != poldstat) {
                                if (pstatus) {
                                        /* Power is OK */
                                        syslog(LOG_ALERT, "Line power restored");
                                        powerfail(0);
                                } else {
                                        /* Power has FAILED */
                                        if (bstatus) {
                                                /* Battery OK, normal shutdown */
                                                syslog(LOG_ALERT, "Line power has failed");
                                                powerfail(1);
                                        } else {
                                                /* Low Battery, SCRAM! */
                                                syslog(LOG_ALERT, "UPS battery power is low!");
                                                powerfail(2);
                                        }               /* if (bstatus) */
                                }                       /* if (pstatus) */
                        }                               /* if (pstatus != poldstat) */
        
                        if (bstatus != boldstat) {
                                if (!bstatus && !pstatus) {
                                        /* Power is out and Battery is now low, SCRAM! */
                                        syslog(LOG_ALERT, "UPS battery power is low!");
                                        powerfail(2);
                                }                       /* if (!bstatus && !pstatus) */
                        }                               /* if (bstatus != boldstat) */
		}                                       /* if (pstatus != poldstat || bstatus != boldstat) */

        
		/* Reset count, remember status and sleep 2 seconds. */
		count = 0;
		poldstat = pstatus;
		boldstat = bstatus;
		sleep(2);
	}       /* while(1) */
	/* Never happens */
	return(0);
}                                                       /* main */
/************************************************************************/
/* End of Function main                                                 */
/************************************************************************/
 
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
int getlevel(LINE *l,int flags)
{
int ret;  

  if (!l->line) return 1;               /* normal mode */
  ret = l->line & flags;
  return (l->inverted) ? !ret: ret;
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
void setlevel(int fd,int line,int level)
{
int bit;
  if (line==0 || line==TIOCM_ST) return;
  bit = line;
  ioctl(fd, (level) ? TIOCMBIS:TIOCMBIC, &bit);
}


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
/*                      : SIGPWR signal and status files.               */
/*                                                                      */
/************************************************************************/                                                
void powerfail(int ok)
{
        int fd;

        /* Create an info file for init. */
        unlink(PWRSTAT);
        if ((fd = open(PWRSTAT, O_CREAT|O_WRONLY, 0644)) >= 0) {
                if (ok) {
                        /* Problem */
                        write(fd, "FAIL\n", 5);
                } else {
                        /* No problem */
                        write(fd, "OK\n", 3);
                }                                       /* if (ok) */
                close(fd);
        }

        /* Create an info file for powerfail scripts. */
        unlink(UPSSTAT);
        if ((fd = open(UPSSTAT, O_CREAT|O_WRONLY, 0644)) >= 0) {
                switch(ok) {
                        case 0: /* Power OK */
                                write(fd, "OK\n", 3);
                                break;
                        case 1: /* Line Fail */
                                write(fd, "FAIL\n", 5);
                                break;
                        case 2: /* Line Fail and Low Batt */
                                write(fd, "SCRAM\n", 6);
                                break;
                        case 3: /* Bad cable? */
                                write(fd, "CABLE\n", 6);
                                break;
                        default: /* Should never get here */
                                write(fd, "SCRAM\n", 6);
                }                                       /* Switch(ok) */
                close(fd);
        }
        kill(1, SIGPWR);
} /*EOF powerfail */
/************************************************************************/
/* End of Function powerfail                                            */
/************************************************************************/
