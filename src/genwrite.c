/************************************************************************/
/* File Name            : genwrite.c                                    */
/* Program Name         : genpowerd                   Version: 1.0.1    */
/* Author               : Brian White <bcwhite@pobox.com>               */
/* Created              : 1998/01/13                                    */
/* Last Modified By     : Brian White                 Date: 1998/01/13  */
/*                                                                      */
/* Compiler (created)   : GCC 2.7.3                                     */
/* Compiler (env)       : Linux 2.0.33                                  */
/* ANSI C Compatable    : No                                            */
/* POSIX Compatable     : Yes?                                          */
/*                                                                      */
/* Purpose              : Write some characters to the serial port      */
/*                      : with DTR and RTS set as desired.              */
/*                                                                      */
/* Usage                : genwrite [-d] [-r] <serial device> <chars>    */
/*                      : i.e. genpowerd /dev/apc-ups "Y"               */
/*                                                                      */
/* History/Credits      : Some code was taken from "genpowerd", written */
/*                      : by Tom Webster.                               */
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
#include <string.h>


/* Main program. */
int main(int argc, char **argv) {
    int		rts_set = 0;
    int		dtr_set = 0;
    char*	program_name;
    int		fd;

    program_name = argv[0];
    /* Parse the command line */
    while ((argc > 1) && (argv[1][0] == '-')) {
	switch (argv[1][1]) {
	case 'r':
	    rts_set = 1;
	    break;
	case 'd':
	    dtr_set = 1;
	    break;
	default:
	    fprintf(stderr, "\nUsage: %s [-d] [-r] <device> <chars>\n", program_name);
	    exit(1);
	}
	argv++;
	argc--;
    }


    /* Done with options, make sure that one port is specified */
    if (argc != 3) {
	fprintf(stderr, "\nUsage: %s [-d] [-r] <device> <chars>\n", program_name);
	exit(1);
    }                            			/* if (argc != 3) */


    /* Open the device */
    if ((fd = open(argv[1], O_RDWR | O_NDELAY)) < 0) {
	fprintf(stderr, "%s: %s: %s\n",program_name, argv[1], sys_errlist[errno]);
	exit(1);
    }

    /* Line is opened, so DTR and RTS are high. Clear them (can  */
    /* cause problems with some UPSs if they remain set).        */
    ioctl(fd, TIOCMBIC, TIOCM_RTS);
    ioctl(fd, TIOCMBIC, TIOCM_DTR);

    /* Set line(s) if so requested */
    if (rts_set) {
	ioctl(fd, TIOCMBIS, TIOCM_RTS);
    }
    if (dtr_set) {
	ioctl(fd, TIOCMBIS, TIOCM_DTR);
    }

    /* Delay a bit so things can equalize (0.1 seconds) */
    usleep(100000);

    /* Write out the characters */
    write(fd,argv[2],strlen(argv[2]));

    /* Delay a bit so things can equalize (0.1 seconds) */
    usleep(100000);

    /* Close the path */
    close(fd);

    /* Return cleanly */
    return 0;
}


/*
 * local variables:
 * tab-width: 8
 * end:
 */
