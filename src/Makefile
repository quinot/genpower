#
# Makefile for genpowerd
# $Id$
#

NAME=		genpower-1.0.1
BINDIR=		/sbin
SCRIPTDIR=	/etc
MANDIR=		/usr/man/man8
OWNER=		root
GROUP=		root

CC= cc
CFLAGS=		-Wall -O2 -pipe

# Linux:
#CFLAGS+=	-DNEWINIT

# FreeBSD
#CFLAGS+=	-DUPSSTAT=\"/var/run/upsstatus\" -DRC_POWERFAIL=\"/usr/local/etc/rc.powerfail\"

all:		genpowerd gentest


genpowerd:	genpowerd.c genpowerd.h
		${CC} ${CFLAGS} $@.c -o $@

gentest:	gentest.c
		${CC} ${CFLAGS} $@.c -o $@

genwrite:	genwrite.c
		${CC} ${CFLAGS} $@.c -o $@

clean:
		rm -f *.o *.bak genpowerd gentest genwrite

install:	binstall manstall

binstall:	genpowerd 
		cp -f genpowerd genwrite ${BINDIR}
		cd ${BINDIR}; \
		chmod 755 genpowerd; \
		chgrp ${GROUP} genpowerd; \
		chown ${OWNER} genpowerd
		cp -f genpowerfail ${SCRIPTDIR}
		cd ${SCRIPTDIR}; \
		chmod 755 genpowerfail; \
		chgrp ${GROUP} genpowerfail; \
		chown ${OWNER} genpowerfail

manstall:	
		cp -f genpowerd.8 ${MANDIR}

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

tgz:
		@if [ -f $(NAME).tgz ]; \
		then \
			echo "Removing old archive..."; \
			rm $(NAME).tgz; \
		fi
		@echo Creating tar archive...
		@ cd ..; \
		ls -l $(NAME) > Packing.List; \
		mv Packing.List $(NAME);\
		touch $(NAME)/*; \
		ls -l $(NAME) > Packing.List; \
		mv Packing.List $(NAME);\
		tar cvfz $(NAME).tgz $(NAME); \
		mv $(NAME).tgz $(NAME)
		@echo Tar archive created
