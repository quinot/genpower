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
/*                                                                      */
/* DO NOT EDIT THE FOLLOWING STRUCTURE AND VARIABLE DEFINITIONS         */
/*                                                                      */
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

struct {
        char *tag;
        LINE    cablepower, kill;               /* outputs -> INACTIVE Level*/
        int     killtime;                       /* killtime 0 -> Option -k is not supported ! */
        LINE    powerok,battok,cableok;         /* inputs */
	int	flags;				/* Cf. #defines above */
        } *pups,ups[] = {

/* The lines inside these brackets may be edited to fit your UPS/cable.  Do NOT remove the {NULL} entry! */

/* type          cablep1        kill           t  powerok        battok         cableok 	flags*/

/* Miquel's powerd cable */
 {"powerd",      {TIOCM_RTS,0}, {TIOCM_DTR,1}, 0, {TIOCM_CAR,0}, {0,0},         {TIOCM_DSR,0}, 0},

/* Classic TrippLite */
 {"tripp-class", {TIOCM_RTS,0}, {TIOCM_ST,1},  5, {TIOCM_CAR,0}, {0,0},         {0,0}, 0},

/* TrippLite WinNT */
 {"tripp-nt",    {TIOCM_RTS,0}, {TIOCM_DTR,1}, 5, {TIOCM_CTS,1}, {TIOCM_CAR,1}, {0,0}, 0},

/* 2001-02-05 Tripplite Omnismart 450 PNP
   Jhon H. Caicedo <jhcaiced@osso.org.co> O.S.S.O */
 {"omnismart-pnp", {TIOCM_RTS,1}, {TIOCM_RTS,1},  5, {TIOCM_CTS,0}, {TIOCM_CAR,0}, {0,0}, UPS_TXD_KILL_INVERTER},

/* Adrian's TrippLite OmniSmart 675 PNP */
 {"tripp-omni",  {TIOCM_RTS,1}, {TIOCM_RTS,1}, 5, {TIOCM_CTS,0}, {TIOCM_CAR,0}, {0,0}, 0},
 {"tripp-omini", {TIOCM_RTS,1}, {TIOCM_RTS,1}, 5, {TIOCM_CTS,0}, {TIOCM_CAR,0}, {0,0}, 0},
 /* second entry (with a typo) retained for backward compatibility. */

/* TrippLite PNP with furnished cable */
/* TrippLite BC Pro w/73-0724 cable   */
 {"tripp-pnp",  {TIOCM_DTR,0}, {TIOCM_RTS,1}, 5, {TIOCM_CTS,0}, {TIOCM_CAR,0}, {0,0}, 0},

/* Lam's APC Back-UPS, Victron Lite WinNT */
 {"apc1-nt",     {TIOCM_RTS,0}, {TIOCM_DTR,1}, 5, {TIOCM_CTS,0}, {TIOCM_CAR,0}, {0,0}, 0},

/* Jim's APC Back-UPS WinNT */
 {"apc2-nt",     {TIOCM_RTS,0}, {TIOCM_DTR,1}, 5, {TIOCM_CTS,1}, {TIOCM_CAR,0}, {0,0}, 0},

/* Marek's APC Back-UPS */
 {"apc-linux",   {TIOCM_DTR,0}, {TIOCM_ST,1},  5, {TIOCM_CAR,1}, {TIOCM_DSR,0}, {0,0}, 0},

/* Brian's APC Back-UPS Pro */
 {"apc-advanced",{TIOCM_DTR,0}, {TIOCM_RTS,1}, 5, {TIOCM_CAR,0}, {TIOCM_RNG,0}, {TIOCM_DSR,0}, 0},

/* Chris Hansen's APC Back-UPS Pro PNP, APC cable #940-0095A */
 {"apc-pnp",     {TIOCM_DTR,0}, {TIOCM_RTS,1}, 5, {TIOCM_RNG,1}, {TIOCM_CAR,1}, {TIOCM_DSR,0}, 0},

/* APC Back-UPS w/940-0020B cable */
 {"apc-20b",     {TIOCM_RTS,0}, {TIOCM_DTR,1},  5, {TIOCM_CTS,1}, {TIOCM_CAR,1}, {0,0}, 0},

/* 2001-12-24 OneUPS+
   Thomas Quinot <thomas@cuivre.fr.eu.org> */
 {"oneups",      {0,0}, {TIOCM_ST,1},  5, {TIOCM_CTS,1}, {TIOCM_CAR,0}, {TIOCM_CAR,0}, 0},

/* Blackout Buster UPS, standard cable */
 {"blackout-buster",{TIOCM_DTR,0},{TIOCM_ST,1}, 0, {TIOCM_CTS,0}, {TIOCM_CAR,0}, {TIOCM_DSR,0}, 0},

/* "GmgH" model "USV 500P" (by Massimiliano Giorgi <ma.giorgi@flashnet.it>) */
 {"gmbh-usv",   {TIOCM_DTR,0}, {TIOCM_RTS,1}, 5, {TIOCM_CAR,0}, {TIOCM_CTS,0}, {TIOCM_DSR,0}, 0},

/* Trust Energy Protector 400/650, as found in the UPS HOWTO */
 {"trust-energy",{TIOCM_DTR,0}, {TIOCM_ST,1},  5, {TIOCM_CAR,0}, {TIOCM_CTS,0}, {TIOCM_DSR,0}, 0},

/* CyberPower SL w/stock cable */
 {"cyber-sl",   {TIOCM_RTS,0}, {TIOCM_DTR,1}, 75, {TIOCM_CTS,0}, {TIOCM_CAR,0}, {0,0}, 0},

 {NULL}
};

/* The following are the RS232 control lines      */
/*                                                */
/*                                            D D */
/*                                            T C */
/* Macro           English                    E E */
/* ---------------------------------------------- */
/* TIOCM_DTR       DTR - Data Terminal Ready  --> */
/* TIOCM_RTS       RTS - Ready to send        --> */
/* TIOCM_CTS       CTS - Clear To Send        <-- */
/* TIOCM_CAR       DCD - Data Carrier Detect  <-- */
/* TIOCM_RNG       RI  - Ring Indicator       <-- */
/* TIOCM_DSR       DSR - Data Signal Ready    <-- */
/* TIOCM_ST        ST  - Data Xmit            --> */
 
#if defined (__Linux__) && !defined (NEWINIT)
/* Create a flag file for Miquel van Smoorenburg's SysV init. */
#define PWRSTAT         "/etc/powerstatus"
#endif

#ifndef UPSSTAT
#define UPSSTAT		"/etc/upsstatus"
#endif

#ifndef RC_POWERFAIL
#define RC_POWERFAIL	"/etc/rc.powerfail"
#endif

/************************************************************************/
/* End of File genpowerd.h                                              */
/************************************************************************/
