#  $Id: Makefile,v 1.34 2000/05/27 13:50:20 jens Exp $

PREFIX		?= /usr/local
BINDIR		 = ${PREFIX}/bin
MANDIR		 = ${PREFIX}/man

HAVE_STRLCPY?= yes

PROG		 = spegla
SRCS		 = jftp.c parserow.c spegla.c tgetopt.c container.c spf_util.c	\
			   que_syms.c e_err.c \
			   regcomp.c regerror.c regexec.c regfree.c
.if !(defined(HAVE_STRLCPY) && ${HAVE_STRLCPY} == "yes")
SRCS		+= strlcpy.c
CPPFLAGS	+= -DNO_STRLCPY
.endif

MAN			 = spegla.1

#WARNS		 = 2
CFLAGS		+= -Wall 
LDADD		+= -lcompat

#DEBUG=1
#EFENCE=1
#MEMDEBUG=1
#PROFILE=1
.if defined(DEBUG)
.if defined(PROFILE)
GPROF		 = -pg
.endif
CFLAGS		 = -g -Wall ${GPROF}
LDFLAGS		+= -static ${GPROF}
.if defined(EFENCE)
LDADD		+= -L/usr/pkg/lib -lefence
.elif defined(MEMDEBUG)
SRCS		+= strdup.c
.PATH: ${.CURDIR}/missing
CPPFLAGS	+= -I/usr/local/include -DMEMDEBUG
LDADD		+= -L/usr/local/lib -lmemdb
.endif
.endif
.if defined(INET6)
CFLAGS		+= -DINET6
.endif

.include <bsd.prog.mk>

# To update some sources
.if exists(${.CURDIR}/../e_err)
e_err.h: ${.CURDIR}/../e_err/e_err.h
	cp ${.CURDIR}/../e_err/e_err.h e_err.h
e_err.c: ${.CURDIR}/../e_err/e_err.c
	cp ${.CURDIR}/../e_err/e_err.c e_err.c
.endif
.if exists(${.CURDIR}/../container)
container.h: ${.CURDIR}/../container/container.h
	cp ${.CURDIR}/../container/container.h container.h
container.c: ${.CURDIR}/../container/container.c
	cp ${.CURDIR}/../container/container.c container.c
.endif
.if exists(${.CURDIR}/../tgetopt)
tgetopt.h: ${.CURDIR}/../tgetopt/tgetopt.h
	cp ${.CURDIR}/../tgetopt/tgetopt.h tgetopt.h
tgetopt.c: ${.CURDIR}/../tgetopt/tgetopt.c
	cp ${.CURDIR}/../tgetopt/tgetopt.c tgetopt.c
.endif

.PATH:	${.CURDIR}/regex
