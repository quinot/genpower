########################################
#                                      #
# Makefile for genpowerd.              #
# by Tom Webster <webster@kaiwan.com>  #
# v.1.0.1        1995/07/05            #
#                                      #
######################################## 

NAME  		= genpower-1.0.1
BINDIR 		= /sbin
SCRIPTDIR 	= /etc
MANDIR		= /usr/man/man8
OWNER 		= root
GROUP 		= root

CC		= cc
CFLAGS		= -N -Wall -O6

all:		genpowerd gentest


genpowerd:	genpowerd.c genpowerd.h
		${CC} ${CFLAGS} $@.c -o $@

gentest:	gentest.c
		${CC} ${CFLAGS} $@.c -o $@

clean:
		rm -f *.o *.bak genpowerd gentest

install:	binstall manstall

binstall:	genpowerd 
		cp -f genpowerd ${BINDIR}
		cd ${BINDIR}; \
		chmod 754 genpowerd; \
		chgrp ${GROUP} genpowerd; \
		chown ${OWNER} genpowerd
		cp -f genpowerfail ${SCRIPTDIR}
		cd ${SCRIPTDIR}; \
		chmod 754 genpowerfail; \
		chgrp ${GROUP} genpowerfail; \
		chown ${OWNER} genpowerfail

manstall:	
		cp -f genpowerd.8 ${MANDIR}

clobber:
		rm -f *.o *.bak genpowerd gentest
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
