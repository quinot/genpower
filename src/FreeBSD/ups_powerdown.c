/*-
 * Copyright (c) 2002 Thomas Quinot <thomas@cuivre.fr.eu.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h> /* uprintf */ 
#include <sys/errno.h>
#include <sys/param.h> /* defines used in kernel.h */
#include <sys/kernel.h> /* types used in module initialization */
#include <sys/sysctl.h>
#include <sys/conf.h>
#include <sys/ttycom.h>
#include <sys/eventhandler.h>
#include <sys/reboot.h>
#include <machine/clock.h>

static udev_t ups_dev;
static int ups_action;
static eventhandler_tag tag0, tag;

SYSCTL_OPAQUE(_kern, OID_AUTO, ups_dev,
   CTLFLAG_RW, &ups_dev, sizeof (udev_t), "T,dev_t",
   "Serial device corresponding to UPS.");
SYSCTL_INT(_kern, OID_AUTO, ups_action,
   CTLFLAG_RW, &ups_action, 0, "Action to be performed on UPS at shutdown.");

/* ups_action can be either:
   - NONE (no action);
   - BREAK (send break);
   - any valid argument for TIOCMBIS (set corresponding modem line) */
#define NONE	(-1)
#define BREAK	0

/* If ups_action is or'd with CLEAR, the corresponding line is cleared
   (TIOCMBIC) instead of set (TIOCMBIS). */
#define CLEAR	0x8000000

/*
 * Perform action at shutdown.
 */

static void
shutdown_ups0 (void *junk, int *howto);
static void
shutdown_ups (void *junk, int *howto);

static void
shutdown_ups0 (void *junk, int *howto)
{
	/* Called before halting, when the system is quiescent. */

	if (*howto & RB_HALT) {
		/* This is a clean halt (no panic): kill UPS now. */
		shutdown_ups (junk, howto);
	}

	/* else carry-on shutdown_final sequence: kernel dump, reboot. */
}

static void
shutdown_ups (void *junk, int *howto)
{
	dev_t dev;
	struct cdevsw *dsw;
	int error;

	if (ups_action == NONE || ups_dev == NOUDEV)
		return;

	dev = udev2dev (ups_dev, 0);
	if (dev == NODEV)
		return;

	dsw = devsw (dev);
	if (dsw == NULL || dsw->d_ioctl == NULL)
		return;

	if (ups_action == BREAK) {
		printf ("Sending break to UPS.\n");
		error = (*dsw->d_ioctl) (dev, TIOCSBRK, NULL, 0, NULL);
		DELAY(400000); /* 400 ms */
		error = (*dsw->d_ioctl) (dev, TIOCCBRK, NULL, 0, NULL);
	} else {
		int line = ups_action & ~CLEAR;
		int op = ((ups_action & CLEAR) != 0) ?
			TIOCMBIC : TIOCMBIS;
		printf ("%s UPS line %0x.\n", (op == TIOCMBIC) ?
			"Clearing": "Setting", line);
		error = (*dsw->d_ioctl) (dev, op, (void*) &line, 0, NULL);
	}
}

/* 
 * Load handler that deals with the loading and unloading of a KLD.
 */

static int
ups_pd_loader(struct module *m, int what, void *arg)
{
  int err = 0;
  
  switch (what) {
  case MOD_LOAD:                /* kldload */
    /* Initialize */
    ups_dev = NOUDEV;
    ups_action = -1;

    tag0 = EVENTHANDLER_REGISTER(shutdown_final, shutdown_ups0, NULL,\
      SHUTDOWN_PRI_LAST + 50);
    /* Before panic/halt: kill UPS only if clean halt. */

    tag = EVENTHANDLER_REGISTER(shutdown_final, shutdown_ups, NULL,\
      SHUTDOWN_PRI_LAST + 150);
    /* After possible panic, but before reset: unconditionally kill UPS. */

    uprintf("UPS power-down KLD loaded.\n");
    break;

  case MOD_UNLOAD:
    EVENTHANDLER_DEREGISTER(shutdown_final, tag0);
    EVENTHANDLER_DEREGISTER(shutdown_final, tag);
    uprintf("UPS power-down KLD unloaded.\n");
    break;

  default:
    err = EINVAL;
    break;
  }
  return(err);
}

/* Declare this module to the rest of the kernel */

static moduledata_t ups_pd_mod = {
  "ups_pd",
  ups_pd_loader,
  NULL
};  

DECLARE_MODULE(ups_powerdown, ups_pd_mod, SI_SUB_KLD, SI_ORDER_ANY);
