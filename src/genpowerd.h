/*
 File        : genpowerd.h
 Project     : genpower-1.0.2
               Observatorio Sismologico del SurOccidente O.S.S.O
 Author(s)   : Jhon H. Caicedo O. <jhcaiced@osso.org.co>
 Description : Header file for genpowerd

 History     :
 1.0.2
 ------
 Feb/05/2001   Added the definition for Tripplite Omnismart 450 PNP
               with a 73-0724 cable, it works using Tx line for 
               Inverter Shutdown, CTS for detecting PowerFail,
               CAR (CD) for Low Battery Alarm and a GND Line (jhcaiced)
               Added the field "id" to the UPS definitions, this makes
               easy to run specific commands based on UPS model.
 Feb/05/2001   Modified from version 1.0.1, to add more UPSs (jhcaiced)
 ------------------------------------------------------------------------
*/
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

struct {
	int id;	/* Used to select specific commands */
        char *tag;
        LINE    cablepower, kill;               /* outputs -> INACTIVE Level*/
        int     killtime;                       /* killtime 0 -> Option -k is not supported ! */
        LINE    powerok,battok,cableok;         /* inputs */
        } *pups,ups[] = {

/* The lines inside these brackets may be edited to fit your UPS/cable.  Do NOT remove the {NULL} entry! */

/* id type          cablep1        kill           t  powerok        battok         cableok */

/* Miquel's powerd cable */
 {0, "powerd",      {TIOCM_RTS,0}, {TIOCM_DTR,1}, 0, {TIOCM_CAR,0}, {0,0},         {TIOCM_DSR,0}},

/* Classic TrippLite */
 {1, "tripp-class", {TIOCM_RTS,0}, {TIOCM_ST,1},  5, {TIOCM_CAR,0}, {0,0},         {0,0}},

/* TrippLite WinNT */
 {2, "tripp-nt",    {TIOCM_RTS,0}, {TIOCM_DTR,1}, 5, {TIOCM_CTS,1}, {TIOCM_CAR,1}, {0,0}},

/* Lam's APC Back-UPS, Victron Lite WinNT */
 {3, "apc1-nt",     {TIOCM_RTS,0}, {TIOCM_DTR,1}, 5, {TIOCM_CTS,0}, {TIOCM_CAR,0}, {0,0}},

/* Jim's APC Back-UPS WinNT */
 {4, "apc2-nt",     {TIOCM_RTS,0}, {TIOCM_DTR,1}, 5, {TIOCM_CTS,1}, {TIOCM_CAR,0}, {0,0}},

/* Marek's APC Back-UPS */
 {5, "apc-linux",   {TIOCM_DTR,0}, {TIOCM_ST,1},  5, {TIOCM_CAR,1}, {TIOCM_DSR,0}, {0,0}},

/* Feb/05/2001 Tripplite Omnismart 450 PNP
   Jhon H. Caicedo <jhcaiced@osso.org.co> O.S.S.O */
 {6, "omnismart-pnp",  {TIOCM_RTS,1}, {TIOCM_RTS,1},  5, {TIOCM_CTS,0}, {TIOCM_CAR,0}, {0,0}},

 {-1, NULL}
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
 
#define PWRSTAT         "/etc/powerstatus"
#define UPSSTAT		"/etc/upsstatus"

/************************************************************************/
/* End of File genpowerd.h                                              */
/************************************************************************/
