/************************************************************************/
/* File Name            : gentest.c                                     */
/* Program Name         : gentest                     Version: 1.0.1    */
/* Author               : Tom Webster <webster@kaiwan.com>              */
/* Created              : 1995/01/22                                    */
/* Last Modified By     : Tom Webster                 Date: 1995/07/05  */
/*                                                                      */
/* Compiler (created)   : GCC 2.6.3                                     */
/* Compiler (env)       : Linux 1.2.5                                   */
/* ANSI C Compatable    : No                                            */
/* POSIX Compatable     : Yes?                                          */
/*                                                                      */
/* Purpose              : Monitor the serial port connected to a UPS.   */
/*                      : Allows DTR and/or RTS lines to be stet to     */
/*                      : simulate genpowerd UPS monitoring software.   */
/*                      : The gentest program will show the state of    */
/*                      : the control lines in the serial connection.   */
/*                      :                                               */
/*                      : The primary purpose for gentest is to help    */
/*                      : discover the settings for UPS monitoring      */
/*                      : cables.                                       */
/*                      :                                               */
/*                      : The "-r" option will set the RTS control      */
/*                      : line.  The "-d" optin will set the DTR        */
/*                      : control line.                                 */
/*                      :                                               */
/*                      : WARNING: If your UPS supports an inverter     */
/*                      : shutdown, either RTS or DTR will be used to   */
/*                      : kill the inverter.  Tests should not be       */
/*                      : performed if your system is using the UPS.    */
/*                      : Specifically, the use of the "-r" and "-d"    */
/*                      : options together is not recomended.           */
/*                                                                      */
/* Usage                : genpowerd [-r] [-d] <serial device>           */
/*                      : i.e. genpowerd -r /dev/cua4                   */
/*                      : or:  genpowerd -d /dev/cua4                   */
/*                      : or:  genpowerd -r -d /dev/cua4                */
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
#include <string.h>

#define MAXSTRING 10
#define MAXROW	   6
#define MAXCOL	   2

/* Global Array */
int             statarray[MAXROW][MAXCOL];

int             linestat(int, int);
int             tester(int, int);
void            stater(char *stat, int linestat, int oldstat);
void            stater2(char *stat, int linestat);

/* Main program. */
int 
main(int argc, char **argv)
{
    int             fd;
    int             rts_bit = TIOCM_RTS;
    int             dtr_bit = TIOCM_DTR;
    int             rts_set = 0;
    int             dtr_set = 0;
    int             status = 1;
    int             current = 0;
    int             next = 1;
    int             x = 0;
    char           *program_name;
    char            stat[MAXSTRING];
    char            tags[MAXSTRING][MAXROW] = {"DTR",
	"RTS",
	"CAR",
	"CTS",
	"DSR",
    "RNG"};
    int             force = 1;

    /* Parse the command line */
    program_name = argv[0];
    while ((argc > 1) && (argv[1][0] == '-')) {
	switch (argv[1][1]) {
	  case 'r':
	    rts_set = 1;
	    break;
	  case 'd':
	    dtr_set = 1;
	    break;
	  default:
	    fprintf(stderr, "Usage: %s [-r] [-d] <device>\n", program_name);
	    exit(1);
	}			/* switch (argv[1][1]) */
	argv++;
	argc--;
    }				/* while ((argc > 1) && (argv[1][0] == '-')) */

    /* Done with options, make sure that one port is specified */
    if (argc != 2) {
	fprintf(stderr, "Usage: %s [-r] [-d] <device>\n", program_name);
	exit(1);
    }				/* if (argc != 2) */
    /*********************/
    /* Monitor the line. */
    /*********************/
    /* Open monitor device. */
    if ((fd = open(argv[1], O_RDWR | O_NDELAY)) < 0) {
	fprintf(stderr, "%s: %s", argv[1], strerror(errno));
	exit(1);
    }				/* if ((fd = open(argv[1], O_RDWR |
				 * O_NDELAY)) < 0) */
    /* Line is opened, so DTR and RTS are high. Clear them, can  */
    /* cause problems with some UPSs if they remain set.         */
    ioctl(fd, TIOCMBIC, &rts_bit);
    ioctl(fd, TIOCMBIC, &dtr_bit);

    /* Set line(s) to provide power high to enable monitoring of line. */
    if (rts_set) {
	ioctl(fd, TIOCMBIS, &rts_bit);
    }				/* if (rts_set) */
    if (dtr_set) {
	ioctl(fd, TIOCMBIS, &dtr_bit);
    }				/* if (dtr_set) */
    /* Now sample the line. */
    while (1) {

	status = linestat(fd, current);

	/* If force is set, force display of status */
	if (force) {
	    status = force;
	    force = 0;
	}			/* if (force) */
	if (status) {
	    printf("---------------\n");
	    stater2(stat, statarray[0][current]);
	    printf("%s = %s\n", tags[0], stat);
	    stater2(stat, statarray[1][current]);
	    printf("%s = %s\n\n", tags[1], stat);
	    for (x = 2; x < MAXROW; x++) {
		stater(stat, statarray[x][current], statarray[x][next]);
		printf("%s = %s\n", tags[x], stat);
	    }			/* for ( x=2; x < MAXROW; x++) */
	}			/* if (status) */
	/* Change array depth and sleep 2 seconds. */
	if (current == 0) {
	    current = 1;
	    next = 0;
	} else {
	    current = 0;
	    next = 1;
	}			/* if (current == 0) */
	sleep(2);
    }				/* while(1) */
    /* Never happens */
    return (0);
}				/* main */
/************************************************************************/
/* End of Function main                                                 */
/************************************************************************/

/************************************************************************/
/* Function             : linestat                                      */
/* Author               : Tom Webster <webster@kaiwan.com>              */
/* Created              : 1995/02/05                                    */
/* Last Modified By     :                             Date:             */
/*                                                                      */
/* Takes                : File handle of open serial port.              */
/*                      : Layer of global to alter.                     */
/*                                                                      */
/* Returns              : int containing result of test.                */
/*                      :                                               */
/*                      : >= 1 == Line status has changed               */
/*                      : 0 == Line status has not changed.             */
/*                                                                      */
/* Purpose              : Tests the serial lines to see if there have   */
/*                      : been any changes, alters global array and     */
/*                      : returns a positive int if changed.            */
/*                                                                      */
/************************************************************************/
int 
linestat(int fd, int layer)
{
    int             flags;
    int             x = 0;
    int             otherlayer;
    int             status = 0;
    int             lines[MAXROW] = {TIOCM_DTR,
	TIOCM_RTS,
	TIOCM_CAR,
	TIOCM_CTS,
	TIOCM_DSR,
    TIOCM_RNG};

    /* Compute the last layer from the current */
    if (layer == 1) {
	otherlayer = 0;
    } else {
	otherlayer = 1;
    }

    /* Get the status. */
    ioctl(fd, TIOCMGET, &flags);

    for (x = 0; x < MAXROW; x++) {

	statarray[x][layer] = tester(lines[x], flags);
	if (statarray[x][layer] != statarray[x][otherlayer]) {
	    status++;
	}
    }
    return (status);

}				/* EOF linestat */
/************************************************************************/
/* End of Function linestat                                             */
/************************************************************************/


/************************************************************************/
/* Function             : tester                                        */
/* Author               : Tom Webster <webster@kaiwan.com>              */
/* Created              : 1995/01/22                                    */
/* Last Modified By     :                             Date:             */
/*                                                                      */
/* Takes                : Line of serial connection to test (int).      */
/*                      :                                               */
/*                      : The current status of the serial connection   */
/*                      : as an int.                                    */
/*                                                                      */
/* Returns              : int containing result of test.                */
/*                      :                                               */
/*                      : 1 == Tested line is in normal mode.           */
/*                      : 0 == Tested line is in failure mode.          */
/*                                                                      */
/* Purpose              : Tests the condition of a serial control line  */
/*                      : to see if it is in normal or failure mode.    */
/*                                                                      */
/************************************************************************/
int 
tester(int testline, int testflags)
{
    int             genreturn;
    if ((testflags & testline)) {
	/* ON Value Returned */
	genreturn = 1;
    } else {
	/* Off Value Returned */
	genreturn = 0;
    }
    return (genreturn);
}				/* EOF tester */
/************************************************************************/
/* End of Function tester                                               */
/************************************************************************/

/************************************************************************/
/* Function             : stater                                        */
/* Author               : Tom Webster <webster@kaiwan.com>              */
/* Created              : 1995/01/22                                    */
/* Last Modified By     :                             Date:             */
/*                                                                      */
/* Takes                : Pointer to character array.                   */
/*                      :                                               */
/*                      : The current status of the serial connection   */
/*                      : as an int.  (zero or non-zero values)         */
/*                                                                      */
/* Returns              : Places text string in provided array.         */
/*                      : If line status is non-zero, "Low" is          */
/*                      : returned.  Otherwise, "High" is returned.     */
/*                                                                      */
/* Purpose              : Converts the int status of serial control     */
/*                      : line to human readable tags.                  */
/*                                                                      */
/************************************************************************/
void 
stater(char *stat, int linestat, int oldstat)
{
    if (linestat == oldstat) {
	if (linestat) {
	    strcpy(stat, "High  ( )");
	} else {
	    strcpy(stat, "Low   ( )");
	}
    } else {
	if (linestat) {
	    strcpy(stat, "High  (*)");
	} else {
	    strcpy(stat, "Low   (*)");
	}
    }
}
/************************************************************************/
/* End of Function stater                                               */
/************************************************************************/

/************************************************************************/
/* Function             : stater2                                       */
/* Author               : Tom Webster <webster@kaiwan.com>              */
/* Created              : 1995/01/22                                    */
/* Last Modified By     :                             Date:             */
/*                                                                      */
/* Takes                : Pointer to character array.                   */
/*                      :                                               */
/*                      : The current status of the serial connection   */
/*                      : as an int.  (zero or non-zero values)         */
/*                                                                      */
/* Returns              : Places text string in provided array.         */
/*                      : If line status is non-zero, "Set" is          */
/*                      : returned.  Otherwise, "Cleared" is returned.  */
/*                                                                      */
/* Purpose              : Converts the int status of serial control     */
/*                      : line to human readable tags.                  */
/*                                                                      */
/************************************************************************/
void 
stater2(char *stat, int linestat)
{
    if (linestat) {
	strcpy(stat, "Set");
    } else {
	strcpy(stat, "Cleared");
    }
}
/************************************************************************/
/* End of Function stater2                                              */
/************************************************************************/
