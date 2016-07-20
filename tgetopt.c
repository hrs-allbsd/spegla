/* $Id: tgetopt.c,v 1.18 2000/05/14 14:39:39 jens Exp $ */
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
#ifndef lint
static char const cvsid[] = "$Id: tgetopt.c,v 1.18 2000/05/14 14:39:39 jens Exp $";
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tgetopt.h"

#ifndef MAX_STR
#define MAX_STR ((size_t) 1024)
#endif

struct get_opt_state {
	const	struct get_opt *gos_go;
	char	gos_buf[MAX_STR];
	char	gos_ebuf[MAX_STR];
	int		gos_errno;
	int		gos_oerrno;			/* contains real errno if gos_errno is	*/
								/* GOS_EREAD  or GOS_END_OF_OPTIONS		*/
	int		gos_last_opt;
	const	char *gos_last_arg;
	int		gos_last_long;
	int		gos_type;

/* Used by get_opt_file */
	FILE	*gos_fp;
	const	char *gos_path;
	int		gos_pos;
	int		gos_in_sect;

/* Used by get_opt_arg */
	int		gos_argc;
	char 	* const *gos_argv;
	int		gos_optind;

/* Used by get_opt_env */
	const	char *gos_prefix;
	const	char **gos_env_ep;
};


/* LINTLIBRARY */

#ifdef NOPROTOS
int snprintf(char *str, size_t size, const char *format, ...);
#endif

static int
get_opt_env __P((struct get_opt_state *gos, int *option, const char **arg));
static int
get_opt_file __P((struct get_opt_state *gos, int *option, const char **arg));
static int
get_opt_argv __P((struct get_opt_state *gos, int *option, const char **arg));

static int in_set __P((const char *set, char ch));
static void copy_out_string __P((char *tok, int *t_pos, char **buf));
static char * next_token __P((char *buf, int *pos));

#define ERR_LEN 40
static char gos_err[][ERR_LEN] = {
	"no error",
	"'%s' requires argument",
	"end of options",
	"unknown option: %s",
	"'%s' doesn't accept arguments (%s)",
	"reached end of argument mark",
	"error reading config file: %s",
	"syntax error expected ':' or '='",
	"syntax error expected ']'",
	"syntax error expected '#' or end of line",
	"sections only avaliable in config file"
};

int
tgetopt(gos, option, arg)
	struct	get_opt_state *gos;
	int		*option;
	const	char **arg;
{
	switch(gos->gos_type) {
	case GOS_ARGV:	return get_opt_argv(gos, option, arg);
	case GOS_FILE:	return get_opt_file(gos, option, arg);
	case GOS_ENV:	return get_opt_env(gos, option, arg);
	}
	return -1;
}

char *
tgetopt_strerror(gos)
	struct get_opt_state *gos;
{
	unsigned	num, pos;
	const		char *opt_name;
	char	buffen[2];

	/* for our convenience */
	char	*buf = gos->gos_ebuf;
	int		gerr = gos->gos_errno;
	
	if (gos->gos_last_long)
		opt_name = gos->gos_go[gos->gos_last_opt].go_name;
	else {
		buffen[0] = gos->gos_go[gos->gos_last_opt].go_sname;
		buffen[1] = '\0';
		opt_name = buffen;
	}

	switch(gerr) {
	case GOS_OK:
		(void)snprintf(buf, MAX_STR, gos_err[gerr]);
		break;
	case GOS_ARG_REQ:
		(void)snprintf(buf, MAX_STR, gos_err[gerr], opt_name);
		break;
	case GOS_END_OF_OPTIONS:
		(void)snprintf(buf, MAX_STR, gos_err[gerr]);
		break;
	case GOS_UNKNOWN_OPTION:
		(void)snprintf(buf, MAX_STR, gos_err[gerr], gos->gos_last_arg);
		break;
	case GOS_DONT_ACCEPT:
		(void)snprintf(buf, MAX_STR, gos_err[gerr], opt_name,
			gos->gos_last_arg);
		break;
	case GOS_END_OF_ARG_MARK:
		(void)snprintf(buf, MAX_STR, gos_err[gerr]);
		break;
	case GOS_EREAD:
		(void)snprintf(buf, MAX_STR, gos_err[gerr], strerror(gos->gos_oerrno));
		break;
	case GOS_SYNTAX_E:
	case GOS_SYNTAX_ES:
	case GOS_SYNTAX_ES2:
		num = snprintf(buf, MAX_STR, "%s\n", gos->gos_buf);
		for (pos = gos->gos_pos; num < (MAX_STR - 2) && pos != 0; num++, pos--)
			(void) strcat(buf, " ");
		(void)strcat(buf, "^\n");
		num++;
		num += snprintf(buf + num, MAX_STR - num, "column %d: ",
				gos->gos_pos + 1);
		(void)snprintf(buf + num, MAX_STR - num, gos_err[gerr]);
		break;
	default:
		(void)snprintf(buf, MAX_STR, "unknown error code %d", gos->gos_errno);
		break;
	}
	return buf;
}

void
tgetopt_free(gos)
	struct get_opt_state *gos;
{
	free(gos);
}

void
tgetopt_change_go(gos, go)
	struct	get_opt_state *gos;
	const	struct get_opt *go;
{
	gos->gos_go = go;
}

#define SECT_CHUNK 20

struct sects {
	int		s_num;
	int		s_last;
	char	**s_p;
};

static struct sects *
sect_init(void)
{
	struct	sects *s;
	char	**p;

	if ((s = malloc(sizeof(struct sects))) == NULL)
		return NULL;
	if ((p = calloc(1, sizeof(char *) * SECT_CHUNK)) == NULL) {
		free(s);
		return NULL;
	}
	s->s_num = SECT_CHUNK;
	s->s_last = 0;
	s->s_p = p;
	return s;
}

static int
sect_add(struct sects *s, char *str)
{
	char	**p;
	if (s->s_num < s->s_last) {
		if ((p = realloc(s->s_p, sizeof(char *) * s->s_num * 2)) == NULL)
			return -1;
		s->s_num *= 2;
		s->s_p = p;
	}
	s->s_p[s->s_last] = str;
	s->s_last++;
	return 0;
}

char **
tgetopt_sections(gos)
	struct	get_opt_state *gos;
{
	char	buf[MAX_STR];
	char	**p, *q, *tok;
	fpos_t	oldfppos;
	struct	sects *s;
	int		pos, i;
	size_t	len;

	gos->gos_errno = GOS_ERRNO;
	if (gos->gos_type != GOS_FILE) {
		gos->gos_errno = GOS_EONLY_FILE;
		return NULL;
	}
	if (fgetpos(gos->gos_fp, &oldfppos) < 0)
		return NULL;
	rewind(gos->gos_fp);
	if ((s = sect_init()) == NULL)
		goto ret_bad;
	while (fgets(buf, MAX_STR, gos->gos_fp) != NULL) {
		len = strlen(buf);
		if (len > 0 && buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		pos = 0;
		tok = next_token(buf, &pos);
		if (tok == NULL || *tok != '[')
			continue;
		len = strlen(tok);
		if (tok[len - 1] != ']') {
			gos->gos_errno = GOS_SYNTAX_ES;
			goto ret_bad;
		}
		tok[len - 1] = '\0';
		tok++;
		if ((tok = strdup(tok)) == NULL)
			goto ret_bad;
		if (sect_add(s, tok) < 0)
			goto ret_bad;
	}
	for (len = 0, i = 0; i < s->s_last; i++)
		len += sizeof(char **) + strlen(s->s_p[i]) + 1;
	len += sizeof(char **);
	if ((p = malloc(len)) == NULL)
		goto ret_bad;
	q = (char *)p + (s->s_last + 1) * sizeof(char **);
	for (i = 0; i < s->s_last; i++) {
		p[i] = q;
		strcpy(q , s->s_p[i]);
		q += strlen(q) + 1;
	}
	p[i] = NULL;
	for (i = 0; i < s->s_last; i++)
		free(s->s_p[i]);
	free(s);
	fsetpos(gos->gos_fp, &oldfppos);
	return p;
	

ret_bad:
	fsetpos(gos->gos_fp, &oldfppos);
	if (s != NULL) {
		if (s->s_p != NULL) {
			for (i = 0; i < s->s_last; i++)
				free(s->s_p[i]);
			free(s->s_p);
		}
		free(s);
	}
	return NULL;
}

struct get_opt_state *
tgetopt_argv_init(argc, argv, go)
	int		argc;
	char	*const *argv;
	const	struct get_opt *go;
{
	struct	get_opt_state *gos;

	if ((gos = calloc((size_t) 1, sizeof(*gos))) == NULL)
		return NULL;

	gos->gos_argc = argc;
	gos->gos_argv = argv;
	gos->gos_go = go;
	gos->gos_type = GOS_ARGV;

	return gos;
}

struct get_opt_state *
tgetopt_env_init(prefix, go)
	const	char *prefix;
	const	struct get_opt *go;
{
	struct	get_opt_state *gos;
	extern	const char **environ;
	extern	const char *__progname;

	if ((gos = calloc((size_t) 1, sizeof(*gos))) == NULL)
		return NULL;

	gos->gos_prefix = (prefix != NULL) ? prefix : __progname;
	gos->gos_go = go;
	gos->gos_env_ep = environ;
	gos->gos_type = GOS_ENV;
	gos->gos_last_long = 1;

	return gos;
}

struct get_opt_state *
tgetopt_file_init(fp, path, go)
	FILE	*fp;
	const	char *path;
	const	struct get_opt *go;
{
	struct	get_opt_state *gos;

	if ((gos = calloc((size_t) 1, sizeof(*gos))) == NULL)
		return NULL;

	gos->gos_fp = fp;
	gos->gos_go = go;
	gos->gos_last_opt = -1;
	gos->gos_pos = -1;
	gos->gos_last_long = 1;

	gos->gos_path = (path == NULL) ? "" : path;

	/* if path is empty assume in section */
	gos->gos_in_sect = (path == NULL || *path == '\0'); 
	gos->gos_type = GOS_FILE;

	return gos;
}

int
tgetopt_errno(gos)
	struct get_opt_state *gos;
{
	return gos->gos_errno;
}

static int
get_opt_env(gos, option, arg)
	struct	get_opt_state *gos;
	int		*option;
	const	char **arg;
{
	int		i;
	size_t	plen, nlen;
	const	char **ep, *p;

	/* for our convenience */
	const	struct get_opt *go = gos->gos_go;

	if (gos->gos_errno != GOS_OK)
		return -1;

	plen = strlen(gos->gos_prefix);
	for (ep = gos->gos_env_ep; *ep != NULL; ep++) {

		/* Case insensitive check if variable may be of interest */
		p = *ep;
		if (strncasecmp(gos->gos_prefix, p, plen) == 0 && p[plen] == '_') {
			p += plen + 1;
			for (i = 0; go[i].go_name != NULL; i++) {

				/* Case insensitive check if variable matches an option */
				nlen = strlen(go[i].go_name);
				if (strncasecmp(go[i].go_name, p, nlen) == 0 &&
					p[nlen] == '=') {
					*option = i;
					*arg = p + nlen + 1;		/* what's after the '=' */
					if (**arg == '\0')
						*arg = NULL;
					gos->gos_env_ep = ep + 1;	/* increase to next variable */
					return 0;
				}
			}
		}
	}

	gos->gos_errno = GOS_END_OF_OPTIONS;
	gos->gos_oerrno = 0;
	return -1;
}

static int
get_opt_file(gos, option, arg)
	struct	get_opt_state *gos;
	int		*option;
	const	char **arg;
{
	char		*tok;
	int			lopt, have_delim;
	unsigned	len;
	const		char *comment = "#!";
	const		char *delimeter = "=:";

	/* for our convenience */
	char	*buf = gos->gos_buf;
	int		*pos = &gos->gos_pos;
	const	struct get_opt *go = gos->gos_go;
	
	if (gos->gos_errno != GOS_OK)
		return -1;

	have_delim = 1;
restart:
	/* need a new row */
	if (*pos < 0) {
		*pos = 0;
		have_delim = 0;
		if (fgets(buf, (int) MAX_STR, gos->gos_fp) == NULL) {
			gos->gos_oerrno = errno;
			if (feof(gos->gos_fp))
				gos->gos_errno = GOS_END_OF_OPTIONS;
			else {
				gos->gos_errno = GOS_EREAD;;
				gos->gos_oerrno = errno;
			}
			return -1;
		}

		/* remove trailing '\n' */
		len = (int) strlen(buf);
		if (len != 0 && buf[len - 1] == '\n')
			buf[len - 1] = '\0';

		tok = next_token(buf, pos);
		/* check if row empty or a comment */
		if (tok == NULL || in_set(comment, tok[0])) {
			/* need a new row, *pos == -1 gets in here again */
			*pos = -1;
			goto restart;
		}

		/* check for section identifier */
		if (*tok == '[') {
			len = (int)strlen(tok);
			if (tok[len - 1] != ']') {
				gos->gos_errno = GOS_SYNTAX_ES;
				return -1;
			}
			gos->gos_in_sect =
					(strlen(gos->gos_path) == len - 2) &&
					(strncmp(gos->gos_path, tok + 1, (size_t) (len - 2)) == 0);
			tok = next_token(buf, pos);
			if (tok == NULL || in_set(comment, tok[0])) {
				/* need a new row, *pos == -1 gets in here again */
				*pos = -1;
				goto restart;
			}
		}

		/* If not in section restart, soner or later will
		 * a new section show up
		 */
		if (!gos->gos_in_sect) {
			*pos = -1;
			goto restart;
		}

		/* Do option look up */
		for (lopt = 0; go[lopt].go_name != NULL; lopt++)
			if (strcmp(go[lopt].go_name, tok) == 0)
				break;

		if (go[lopt].go_name == NULL) {
			*arg = tok;
			gos->gos_last_arg = *arg;
			gos->gos_errno = GOS_UNKNOWN_OPTION;
			return -1;
		}
		gos->gos_last_opt = lopt;
	} else
		lopt = gos->gos_last_opt;

	*option = lopt;
	gos->gos_last_opt = *option;

	
	/* check if row empty or a comment */
	if ((tok = next_token(buf, pos)) == NULL ||
		in_set(comment, *tok)) {

		/* if we already have an argument, read a new row */
		if (have_delim) {
			*pos = -1;
			goto restart;
		}
			
		/* handle the case when there is no argument to option */
		if (go[lopt].go_want_arg != REQ_ARG) {
			*arg = NULL;
			gos->gos_last_arg = *arg;
			return 0;
		}
		gos->gos_errno = GOS_ARG_REQ;
		return -1;
	}

	/* if we got delimiter before don't look for it again */
	if (!have_delim) {
		if (!in_set(delimeter, *tok)) {
			gos->gos_errno = GOS_SYNTAX_E;
			return -1;
		}
		/* check if row empty or a comment */
		if ((tok = next_token(buf, pos)) == NULL ||
			in_set(comment, *tok)) {

			if (go[lopt].go_want_arg != REQ_ARG) {
				*arg = NULL;
				gos->gos_last_arg = *arg;
				return 0;
			}
			gos->gos_errno = GOS_ARG_REQ;
			return -1;
		}
	}

	*arg = tok;
	gos->gos_last_arg = *arg;
	/* does this option support arguments? */
	if (go[lopt].go_want_arg == NO_ARG) {
		gos->gos_errno = GOS_DONT_ACCEPT;
		return -1;
	}
	return 0;
}

static int
get_opt_argv(gos, option, arg)
	struct	get_opt_state *gos;
	int		*option;
	const	char **arg;
{
	const	char *larg, *str;
	int		lopt;
	unsigned	len;

	/* for our convenience */
	const	struct get_opt *go = gos->gos_go;

	if (gos->gos_errno != GOS_OK)
		return -1;

	gos->gos_last_long = 1;
restart:

	/* in the middle of parsing a short opt */
	if (gos->gos_optind != 0) {
		gos->gos_last_long = 0;
		larg = *gos->gos_argv;
		for (lopt = 0; go[lopt].go_name != NULL; lopt++) {
			/* this option doesn't have any short equality */
			if (go[lopt].go_sname == '?')
				continue;

			if (larg[gos->gos_optind] == go[lopt].go_sname) {
				*arg = NULL;
				gos->gos_last_arg = *arg;
				*option = lopt;
				gos->gos_last_opt = *option;

				/* move optind to next short option and return */
				if (go[lopt].go_want_arg == NO_ARG) {
					gos->gos_optind++;

					/* reset optind if there is no more */
					if (larg[gos->gos_optind] == '\0')
						gos->gos_optind = 0;
					return 0;
				}

				/* if there are imidiatly following characters,
				 * take them as argument and return
				 */
				if (larg[gos->gos_optind + 1] != '\0') {
					*arg = larg + gos->gos_optind + 1;
					gos->gos_last_arg = *arg;
					gos->gos_optind = 0;
					return 0;
				}

				/* take the folloing argv if there is one and it doesn't
				 * start with a '-'
				 */
				if (gos->gos_argc > 1 && *gos->gos_argv[1] != '-') {
					gos->gos_argc--;
					gos->gos_argv++;
					larg = *gos->gos_argv;
					*arg = larg;
					gos->gos_last_arg = *arg;
					gos->gos_optind = 0;
					return 0;
				}

				/* If an argument was required return error */
				if (go[lopt].go_want_arg == REQ_ARG) {
					gos->gos_errno = GOS_ARG_REQ;
					return -1;
				}

				/* reset optind and return without argument */
				gos->gos_optind = 0;
				*arg = NULL;
				gos->gos_last_arg = *arg;
				return 0;
			}
		}
		/* didn't recognise option put it in arg */
		gos->gos_buf[0] = larg[gos->gos_optind];
		gos->gos_buf[1] = '\0';
		*arg = gos->gos_buf;
		gos->gos_last_arg = *arg;
		*option = -1;
		gos->gos_last_opt = *option;
		gos->gos_errno = GOS_UNKNOWN_OPTION;
		return -1;
	}

	/* advance to next argv */
	gos->gos_argc--;
	gos->gos_argv++;
	larg = *gos->gos_argv;

	/* out of options */
	if (gos->gos_argc == 0) {
		gos->gos_errno = GOS_END_OF_OPTIONS;
		gos->gos_oerrno = 0;
		return -1;
	}

	/* this marks end of argument processing */
	if (strcmp("--", larg) == 0) {
		gos->gos_argc--;
		gos->gos_argv++;
		gos->gos_errno = GOS_END_OF_ARG_MARK;
		return -1;
	}

	/* a long opt */
	if (larg[0] == '-' && larg[1] == '-') {
		larg += 2;
		for (lopt = 0; go[lopt].go_name != NULL; lopt++) {
			str = go[lopt].go_name;
			len = (int) strlen(str);
			if (strncmp(str, larg, (size_t) len) == 0) {

				/* indicating there is an argument for option */
				if (larg[len] == '=') {
					larg += len + 1;
					*arg = larg;
					gos->gos_last_arg = *arg;
					*option = lopt;
					gos->gos_last_opt = *option;
					/* does this option support arguments? */
					if (go[lopt].go_want_arg == NO_ARG) {
						gos->gos_errno = GOS_DONT_ACCEPT;
						return -1;
					}
					return 0;
				}

				/* indicating there is no argument for option */
				if (larg[len] == '\0') {
					*option = lopt;
					gos->gos_last_opt = *option;
					*arg = NULL;
					gos->gos_last_arg = *arg;
					/* this option needs an argument */
					if (go[lopt].go_want_arg == REQ_ARG) {
						gos->gos_errno = GOS_ARG_REQ;
						return -1;
					}
					return 0;
				}
			}
		}
	}

	/* a normal opt */
	if (larg[0] == '-' && larg[1] != '\0') {
		gos->gos_optind = 1;
		goto restart;
	}

	/* didn't recognise the option */
	*arg = larg;
	gos->gos_last_arg = *arg;
	*option = -1;
	gos->gos_last_opt = *option;
	gos->gos_errno = GOS_UNKNOWN_OPTION;
	return -1;
}

int
tgetopt_argv_get(gos, argc, argv)
	struct	get_opt_state *gos;
	int		*argc;
	char	*const **argv;
{
	if (argc != NULL)
		*argc = gos->gos_argc;
	if (argv != NULL)
		*argv = gos->gos_argv;
	return 0;
}


static int
in_set(set, ch)
	const char *set;
	char ch;
{
	int		i;

	for (i = 0; set[i] != '\0'; i++)
		if (set[i] == ch)
			return 1;
	return 0;
}

/* XXX doesn't handle \xHH and \000, yet */
static void
copy_out_string(tok, t_pos, buf)
	char	*tok;
	int		*t_pos;
	char	**buf;
{
	char	*b = *buf;
#define ADD_TOK(ch) tok[(*t_pos)++] = ch

	/* nothing to do */
	if (*b != '"')
		return;

	/* skip first " */
	b++;

	/* finish on " or end of string */
	for (; *b != '\0' && *b != '"'; b++) {
		if (*b == '\\') switch (*(++b)) {
			case 'a': ADD_TOK('\a'); continue;
			case 'b': ADD_TOK('\b'); continue;
			case 'f': ADD_TOK('\f'); continue;
			case 'n': ADD_TOK('\n'); continue;
			case 'r': ADD_TOK('\r'); continue;
			case 't': ADD_TOK('\t'); continue;
			case 'v': ADD_TOK('\v'); continue;
			default: ADD_TOK(*b); continue;
		}
		ADD_TOK(*b);
	}

	/* get rid of ending " */
	if (*b != '\0')
		b++;

	/* update position in buf */
	*buf = b;
#undef ADD_TOK
}


static char *
next_token(buf, pos)
	char *buf;
	int *pos;
{
	static	char tok[MAX_STR];
	const	char *white_space = " \t";
	const	char *delimiter = "#!=:,";
	char	*b;
	int		t_pos = 0;

	if (*pos < 0)
		return NULL;

	/* go to end of previous token */
	b = buf + *pos;
	if (*b == '\0') {
		*pos = -1;
		return NULL;
	}

	/* skip whitespace */
	while (in_set(white_space, *b)) b++;
	if (*b == '\0') {
		*pos = -1;
		return NULL;
	}

	if (in_set(delimiter, *b)) {
		tok[t_pos++] = *b;
		b++;
		goto ret;
	}
	
	for (; *b != '\0'; b++) {
		/* token is finished */
		if (in_set(white_space, *b))
			goto ret;

		/* take care of stuff inside "" */
		if (*b == '"') {
			copy_out_string(tok, &t_pos, &b);
			continue;
		}

		if (in_set(delimiter, *b))
			goto ret;

		tok[t_pos++] = *b;
	}
	
ret:
	/* set up end of token */
	*pos = (int) (b - buf);
	tok[t_pos] = '\0';
	return tok;
}
