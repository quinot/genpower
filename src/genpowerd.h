/************************************************************************/
/* File Name            : genpowerd.h                                   */
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
/* Purpose              : Header file for genpowerd.                    */
/*                      : Contains the configuration information for    */
/*                      : genpowerd.  Edit this file as indicated       */
/*                      : below to activate features and to customize   */
/*                      : genpowerd for your UPS.                       */
/*                                                                      */
/* Copyright            : GNU Copyleft                                  */
/************************************************************************/

/* LINE type definition for input/output lines:
   For outputs, the INACTIVE state must be specified !!
   inverted=0 -> the output is set (pos. voltage) on startup [ RTS, DTR]
   inverted=1 -> the output is cleared (neg. voltage) on startup [*RTS,*DTR]

   line==0 -> function is not supported, getlevel returns normal state (1).
 */

typedef struct{
    int line;
    int inverted;      /* 0->active high, 1->active low */
} LINE;

/************************************************************************/
/*                                                                      */
/* DO NOT EDIT THE FOLLOWING *STRUCTURE*, UPS/CABLE DATA MAY BE EDITED  */
/*                                                                      */
/* The data in the UPS data structure may be edited and additional      */
/* lines may be added.  The paths for the UPS status files may also be  */
/* edited.  (init does expect to find powerstatus in "/etc".            */
/*                                                                      */
/* The TIOCM_ST serial line may be used ONLY for the inverter kill      */
/* function.  genpower will NOT function properly if it is configured   */
/* for use with other functions.                                        */
/*                                                                      */
/************************************************************************/

#define UPS_TXD_KILL_INVERTER 	1	/* Transmit data to kill inverter */

struct upsdef {
    struct upsdef *next;
    char *tag;
    LINE    cablepower, kill;               /* outputs -> INACTIVE Level*/
    int     killtime;                       /* killtime 0 -> Option -k is not supported ! */
    LINE    powerok,battok,cableok;         /* inputs */
    int	flags;				/* Cf. #defines above */
};
