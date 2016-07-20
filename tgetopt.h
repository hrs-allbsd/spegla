/* $Id: tgetopt.h,v 1.18 2000/05/14 14:39:39 jens Exp $ */
/*
 * Copyright (c) 1999, 2000
 *      Jens A. Nilsson, jnilsson@ludd.luth.se. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef TGETOPT__H
#define TGETOPT__H

#ifdef MEMDEBUG
#include <memdebug.h>
#endif


/* gos_errno values:
 *  0 => OK
 *  1 => didn't get required arg, ERROR
 *  2 => out of argv:s, OK
 *  3 => didn't recognize option, option in *arg, ERROR
 *  4 => don't want argument for this option, argument in *arg, ERROR
 *  5 => end of argument processing, OK
 *  6 => read error, ERROR
 *  7 => syntax error, ERROR
 *  8 => syntax error expected ']', ERROR
 */

#define	GOS_OK 				0
#define GOS_ARG_REQ			1
#define GOS_END_OF_OPTIONS	2
#define GOS_UNKNOWN_OPTION	3
#define GOS_DONT_ACCEPT		4
#define GOS_END_OF_ARG_MARK	5
#define GOS_EREAD			6
#define GOS_SYNTAX_E		7
#define GOS_SYNTAX_ES		8
#define GOS_SYNTAX_ES2		9
#define	GOS_EONLY_FILE		10
#define	GOS_ERRNO			11

#define	GOS_ARGV			0
#define	GOS_FILE			1
#define	GOS_ENV				2

#ifndef __P
#ifdef __STDC__
#define __P(x)  x
#else
#define __P(x)  ()
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif


enum go_want_arg {
	NO_ARG,
	REQ_ARG,
	OPT_ARG
};

/* table for option identifiers */
struct get_opt {
	int		go_opt_num;
	const	char *go_name;			/* like the GNU getlongopt			*/
	char	go_sname;				/* like getopt						*/
	enum	go_want_arg go_want_arg;
};

struct get_opt_state;

int tgetopt __P((struct get_opt_state *gos, int *option, const char **arg));

char * tgetopt_strerror __P((struct get_opt_state *gos));
int tgetopt_errno __P((struct get_opt_state *gos));
void tgetopt_free __P((struct get_opt_state *gos));
void tgetopt_change_go __P((struct get_opt_state *gos, const struct get_opt *go));
char ** tgetopt_sections __P((struct get_opt_state *gos));

struct get_opt_state *
tgetopt_argv_init __P((int argc, char * const *argv, const struct get_opt *go));
int tgetopt_argv_get __P((struct get_opt_state *gos, int *argc, char * const **argv));

struct get_opt_state *
tgetopt_file_init __P((FILE *fp, const char *path, const struct get_opt *go));

struct get_opt_state *
tgetopt_env_init __P((const char *prefix, const struct get_opt *go));

#ifdef __cplusplus
}
#endif

#endif
