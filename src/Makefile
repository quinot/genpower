#
# Makefile for genpowerd
# $Id$
#

NAME=		genpower-1.0.5
BINDIR=		/sbin
SCRIPTDIR=	/etc
CONFDIR=	/etc
MANDIR=		/usr/man/man8
OWNER=		root
GROUP=		root

CC= cc
CFLAGS=		-Wall -O2 -pipe
INSTALL=	install

# Linux:
#CFLAGS+=	-DNEWINIT

# FreeBSD
#CFLAGS+=	-DUPSSTAT=\"/var/run/upsstatus\" \
#		-DRC_POWERFAIL=\"/usr/local/etc/rc.powerfail\" \
#		-DGENPOWERD_CONF=\"/usr/local/etc/genpowerd.conf\"

all:		genpowerd gentest


genpowerd:	genpowerd.c genpowerd.h
		${CC} ${CFLAGS} genpowerd.c -o $@

gentest:	gentest.c
		${CC} ${CFLAGS} $@.c -o $@

genwrite:	genwrite.c
		${CC} ${CFLAGS} $@.c -o $@

clean:
		rm -f *.o *.bak genpowerd gentest genwrite

install:	binstall manstall

binstall:	genpowerd
		-test -x genpowerd && ${INSTALL} -m755 -o${OWNER} -g${GROUP} -s genpowerd ${BINDIR}
		-test -x genwrite  && ${INSTALL} -m755 -o${OWNER} -g${GROUP} -s genwrite  ${BINDIR}
		-test -x gentest   && ${INSTALL} -m755 -o${OWNER} -g${GROUP} -s gentest   ${BINDIR}
		${INSTALL} -m755 -o${OWNER} -g${GROUP} genpowerfail   ${SCRIPTDIR}
		${INSTALL} -m644 -o${OWNER} -g${GROUP} genpowerd.conf ${CONFDIR}

manstall:
		${INSTALL} -m644 -o${OWNER} -g${GROUP} genpowerd.8 ${MANDIR}

clobber:	clean
		cd ${BINDIR}; \
		rm -f genpowerd
		cd ${SCRIPTDIR}; \
		rm -f genpowerfail

oldclobber:
		cd ${BINDIR}; \
		rm -f unipowerd
		cd ${SCRIPTDIR}; \
		rm -f unipowerfail
		cd ${MANDIR}; \
		rm -f unipowerd.8

upgrade:	oldclobber install
