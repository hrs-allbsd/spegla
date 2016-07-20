/* $Id: e_err.c,v 1.15 2000/05/14 14:39:39 jens Exp $ */
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
static char const cvsid[] = "$Id: e_err.c,v 1.15 2000/05/14 14:39:39 jens Exp $";
#endif

#include <errno.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "e_err.h"

/* LINTLIBRARY */

#define MAX_STR	((size_t) 1024)

#ifdef NOPROTOS
void openlog(const char *ident, int logopt, int facility);
void syslog(int priority, const char *message, ...);
void closelog(void);
int snprintf(char *str, size_t size, const char *format, ...);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
char *strerror(int);
#endif

#ifdef PROGNAME
const char *__progname = PROGNAME;
#else
#ifdef lint
const char *__progname = "lint";
#else
extern char *__progname;
#endif
#endif


static const char *e_pname;
static const char *e_newline = "\n";
static int	e_log_facil = 0;
static int	e_log_prio = 0;
static int	e_log_opt = LOG_PID;
static int	e_use_f = 1;
static FILE	*e_fp = NULL;
static int	e_log_opened = 0;
static int	e_level = 0;
static int	e_inited = 0;
static int	e_section = -1;		/* default is al sections */


#ifdef lint
#undef va_start
#define va_start(x, y) {x = x;}
#endif

#define CHK_INIT	if (!e_inited) e_init()
#define CHK_LVL		if (level > e_level) return
#define	CHK_SECT	if ((section & e_section) == 0) return


static void
e_init(void)
{
	e_inited = 1;
	e_pname = __progname;
	e_fp = stderr;
}

static void
update_log(void)
{
	if (e_log_opened)
		closelog();
	openlog(e_pname, e_log_opt, e_log_facil);
	e_log_opened = 1;
}

void
e_buf_format(char *buf, size_t len, const char *name, char *error,
	const char *fmt, va_list args)
{
	char	*p = buf;
	int		oerrno = errno;
	unsigned	res;

	if (name != NULL) {
		res = snprintf(p, len, "%s: ", name);
		len -= res;
		p += res;
	}

	res = vsnprintf(p, len, fmt, args);
	len -= res;
	p += res;

	/* remove trailing '\n' */
	if (p > (buf + 1) && *(p - 1) == '\n') {
		*(p - 1) = '\0';
		p--;
		len++;
	}
	/* this is mainly for jftp */
	if (p > (buf + 1) && *(p - 1) == '\r') {
		*(p - 1) = '\0';
		p--;
		len++;
	}

	if (error != NULL)
		res = snprintf(p, len, ": %s", error);

	errno = oerrno;
}

void
e_buf_file(char *buf, FILE *fp)
{
	int		oerrno = errno;
	int		needed_extra, len_nl, len_buf, i;
	char	*p, *tmp_buf;


	if (e_newline[0] == '\0' ||
		e_newline[0] != '\n' ||
		e_newline[1] != '\0') {

		len_nl = (int)strlen(e_newline);
		needed_extra = 0;
		for (p = buf; p != NULL; p = strchr(p, '\n')) {
			/* how much extra space will e_newline take up compared
			 * to "\n" or "\r\n"
			 */
			needed_extra += len_nl - ((p != buf && *(p - 1) != '\r') ? 1 : 2);
			p++;
		}

		len_buf = (int)strlen(buf);
		if ((p = alloca((size_t)(len_buf + needed_extra + 1))) == NULL) {
			/* this will probably fail too */
			fprintf(fp, "e_err: write failed: out of memory%s", e_newline);
			errno = oerrno;
			return;
		}
		
		p[0] = '\0';
		tmp_buf = p;
		for (i = 0; buf[i] != '\0'; i++) {
			/* calculated needed buffer size earlier,
			 * no risk for buffer overrun
			 */
			if (buf [i] == '\n' || (buf[i] == '\r' && buf[i + 1] == '\n')) {
				strcat(p, e_newline);
				p += len_nl;
				if (buf[i] == '\r' && buf[i + 1] == '\n')
					i++;
				continue;
			}
			*p = buf[i];
			p++;
			*p = '\0';
		}
		buf = tmp_buf;
	}
	(void) fprintf(fp, "%s%s", buf, e_newline);
	(void) fflush(fp);
	errno = oerrno;
}

void
e_buf_log(char *buf, int prio)
{
	int		oerrno = errno;
	char	*p;

	if ((p = index(buf, ':')) != NULL) {
		if (*p != '\0') p++;
		if (*p != '\0') p++;
	} else
		p = buf;
	syslog(prio, p);
	errno = oerrno;
}



void
e_set_progname(char *name)
{
	CHK_INIT;
	e_use_f = 1;
	e_pname = name;
}

const char *
e_get_progname(void)
{
	CHK_INIT;
	return e_pname;
}

void
e_set_log_facil(int facility)
{
	CHK_INIT;
	e_use_f = 0;
	e_log_facil = facility;
	update_log();
}

int
e_get_log_facil(void)
{
	CHK_INIT;
	return e_log_facil;
}

void
e_set_log_prio(int prio)
{
	CHK_INIT;
	e_use_f = 0;
	e_log_prio = prio;
	update_log();
}

int
e_get_log_prio(void)
{
	CHK_INIT;
	return e_log_prio;
}

void
e_use_log(void)
{
	CHK_INIT;
	e_use_f = 0;
}

void
e_set_file(FILE *fp)
{
	CHK_INIT;
	e_use_f = 1;
	e_fp = fp != NULL ? fp : stderr;;
	e_pname = e_fp == stderr ? __progname : NULL;
}

FILE *
e_get_file(void)
{
	CHK_INIT;
	return e_fp;
}

void
e_use_file(void)
{
	CHK_INIT;
	e_use_f = 1;
}

void
e_set_level(int level)
{
	e_level = level;
}

int
e_get_level(void)
{
	return e_level;
}

void
e_set_section(int section)
{
	e_section = section;
}

int
e_get_section(void)
{
	return e_section;
}

void
e_set_newline_str(const char *str)
{
	e_newline = str;
}

const char *
e_get_newline_str(void)
{
	return e_newline;
}


void
e_usage(const char *fmt, ...)
{
	va_list	ap;
	char	buf[MAX_STR];
	char	*p = buf;
	size_t	len = MAX_STR;
	int		oerrno = errno;
	unsigned	res;

	CHK_INIT;
	va_start(ap, fmt);

/* from buf_format(), slightly modified */
	res = snprintf(p, len, "usage: %s ", e_pname);
	len -= res;
	p += res;

	res = vsnprintf(p, len, fmt, ap);
	len -= res;
	p += res;

	/* remove trailing '\n' */
	if (p > (buf + 1) && *(p - 1) == '\n') {
		*(p - 1) = '\0';
		p--;
		len++;
	}
	/* this is mainly for jftp */
	if (p > (buf + 1) && *(p - 1) == '\r') {
		*(p - 1) = '\0';
		p--;
		len++;
	}
/* end from buf_format() */

	e_buf_file(buf, stderr);
	va_end(ap);
	errno = oerrno;
}

void
e_err(int eval, const char *fmt, ...)
{
	va_list		ap;

	CHK_INIT;
	va_start(ap, fmt);
	e_verr(eval, fmt, ap);
	va_end(ap);
}

void
e_verr(int eval, const char *fmt, va_list args)
{
	CHK_INIT;
	if (e_use_f)
		e_verr_file(eval, e_fp, fmt, args);
	e_verr_log(eval, e_log_prio, fmt, args);
}

void
e_err_log(int eval, int prio, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	va_start(ap, fmt);
	e_verr_log(eval, prio, fmt, ap);
	va_end(ap);
}

void
e_verr_log(int eval, int prio, const char *fmt, va_list args)
{
	CHK_INIT;
	e_vwarn_log(prio, fmt, args);
	exit(eval);
}

void
e_err_file(int eval, FILE *fp, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	va_start(ap, fmt);
	e_verr_file(eval, fp, fmt, ap);
	va_end(ap);
}

void
e_verr_file(int eval, FILE *fp, const char *fmt, va_list args)
{
	CHK_INIT;
	e_vwarn_file(fp, fmt, args);
	exit(eval);
}

void
e_errx(int eval, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	va_start(ap, fmt);
	e_verrx(eval, fmt, ap);
	va_end(ap);
}

void
e_verrx(int eval, const char *fmt, va_list args)
{
	CHK_INIT;
	if (e_use_f)
		e_verrx_file(eval, e_fp, fmt, args);
	e_verrx_log(eval, e_log_prio, fmt, args);
}

void
e_errx_log(int eval, int prio, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	va_start(ap, fmt);
	e_verrx_log(eval, prio, fmt, ap);
	va_end(ap);
}

void
e_verrx_log(int eval, int prio, const char *fmt, va_list args)
{
	CHK_INIT;
	e_vwarnx_log(prio, fmt, args);
	exit(eval);
}

void
e_errx_file(int eval, FILE *fp, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	va_start(ap, fmt);
	e_verrx_file(eval, fp, fmt, ap);
	va_end(ap);
}

void
e_verrx_file(int eval, FILE *fp, const char *fmt, va_list args)
{
	CHK_INIT;
	e_vwarnx_file(fp, fmt, args);
	exit(eval);
}

void
e_vtrace(int level, int section, const char *fmt, va_list args)
{
	CHK_INIT;
	CHK_LVL;
	CHK_SECT;
	e_vwarn(fmt, args);
}

void
e_trace(int level, int section, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	CHK_LVL;
	CHK_SECT;
	va_start(ap, fmt);
	e_vwarn(fmt, ap);
	va_end(ap);
}

void
e_vtrace_log(int level, int section, int prio, const char *fmt, va_list args)
{
	CHK_INIT;
	CHK_LVL;
	CHK_SECT;
	e_vwarn_log(prio, fmt, args);
}

void
e_trace_log(int level, int section, int prio, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	CHK_LVL;
	CHK_SECT;
	va_start(ap, fmt);
	e_vwarn_log(prio, fmt, ap);
	va_end(ap);
}

void
e_vtrace_file(int level, int section, FILE *fp, const char *fmt, va_list args)
{
	CHK_INIT;
	CHK_LVL;
	CHK_SECT;
	e_vwarn_file(fp, fmt, args);
}

void
e_trace_file(int level, int section, FILE *fp, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	CHK_LVL;
	CHK_SECT;
	va_start(ap, fmt);
	e_vwarn_file(fp, fmt, ap);
	va_end(ap);
}

void
e_vtracex(int level, int section, const char *fmt, va_list args)
{
	CHK_INIT;
	CHK_LVL;
	CHK_SECT;
	e_vwarnx(fmt, args);
}

void
e_tracex(int level, int section, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	CHK_LVL;
	CHK_SECT;
	va_start(ap, fmt);
	e_vwarnx(fmt, ap);
	va_end(ap);
}

void
e_vtracex_log(int level, int section, int prio, const char *fmt, va_list args)
{
	CHK_INIT;
	CHK_LVL;
	CHK_SECT;
	e_vwarnx_log(prio, fmt, args);
}

void
e_tracex_log(int level, int section, int prio, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	CHK_LVL;
	CHK_SECT;
	va_start(ap, fmt);
	e_vwarnx_log(prio, fmt, ap);
	va_end(ap);
}

void
e_vtracex_file(int level, int section, FILE *fp, const char *fmt, va_list args)
{
	CHK_INIT;
	CHK_LVL;
	CHK_SECT;
	e_vwarnx_file(fp, fmt, args);
}

void
e_tracex_file(int level, int section, FILE *fp, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	CHK_LVL;
	CHK_SECT;
	va_start(ap, fmt);
	e_vwarnx_file(fp, fmt, ap);
	va_end(ap);
}

void
e_vlog(int level, const char *fmt, va_list args)
{
	CHK_INIT;
	CHK_LVL;
	e_vwarn(fmt, args);
}

void
e_log(int level, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	CHK_LVL;
	va_start(ap, fmt);
	e_vwarn(fmt, ap);
	va_end(ap);
}

void
e_vlog_log(int level, int prio, const char *fmt, va_list args)
{
	CHK_INIT;
	CHK_LVL;
	e_vwarn_log(prio, fmt, args);
}

void
e_log_log(int level, int prio, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	CHK_LVL;
	va_start(ap, fmt);
	e_vwarn_log(prio, fmt, ap);
	va_end(ap);
}

void
e_vlog_file(int level, FILE *fp, const char *fmt, va_list args)
{
	CHK_INIT;
	CHK_LVL;
	e_vwarn_file(fp, fmt, args);
}

void
e_log_file(int level, FILE *fp, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	CHK_LVL;
	va_start(ap, fmt);
	e_vwarn_file(fp, fmt, ap);
	va_end(ap);
}

void
e_vlogx(int level, const char *fmt, va_list args)
{
	CHK_INIT;
	CHK_LVL;
	e_vwarnx(fmt, args);
}

void
e_logx(int level, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	CHK_LVL;
	va_start(ap, fmt);
	e_vwarnx(fmt, ap);
	va_end(ap);
}

void
e_vlogx_log(int level, int prio, const char *fmt, va_list args)
{
	CHK_INIT;
	CHK_LVL;
	e_vwarnx_log(prio, fmt, args);
}

void
e_logx_log(int level, int prio, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	CHK_LVL;
	va_start(ap, fmt);
	e_vwarnx_log(prio, fmt, ap);
	va_end(ap);
}

void
e_vlogx_file(int level, FILE *fp, const char *fmt, va_list args)
{
	CHK_INIT;
	CHK_LVL;
	e_vwarnx_file(fp, fmt, args);
}

void
e_logx_file(int level, FILE *fp, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	CHK_LVL;
	va_start(ap, fmt);
	e_vwarnx_file(fp, fmt, ap);
	va_end(ap);
}

void
e_warn(const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	va_start(ap, fmt);
	e_vwarn(fmt, ap);
	va_end(ap);
}

void
e_vwarn(const char *fmt, va_list args)
{
	CHK_INIT;
	if (e_use_f)
		e_vwarn_file(e_fp, fmt, args);
	else
		e_vwarn_log(e_log_prio, fmt, args);
}

void
e_warn_log(int prio, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	va_start(ap, fmt);
	e_vwarn_log(prio, fmt, ap);
	va_end(ap);
}

void
e_vwarn_log(int prio, const char *fmt, va_list args)
{
	char	buf[MAX_STR];

	CHK_INIT;
	e_buf_format(buf, MAX_STR, e_pname, strerror(errno), fmt, args);
	e_buf_log(buf, prio);
}

void
e_warn_file(FILE *fp, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	va_start(ap, fmt);
	e_vwarn_file(fp, fmt, ap);
	va_end(ap);
}

void
e_vwarn_file(FILE *fp, const char *fmt, va_list args)
{
	char	buf[MAX_STR];

	CHK_INIT;
	e_buf_format(buf, MAX_STR, e_pname, strerror(errno), fmt, args);
	e_buf_file(buf, fp);
}

void
e_warnx(const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	va_start(ap, fmt);
	e_vwarnx(fmt, ap);
	va_end(ap);
}

void
e_vwarnx(const char *fmt, va_list args)
{
	CHK_INIT;
	if (e_use_f)
		e_vwarnx_file(e_fp, fmt, args);
	else
		e_vwarnx_log(e_log_prio, fmt, args);
}

void
e_warnx_log(int prio, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	va_start(ap, fmt);
	e_vwarnx_log(prio, fmt, ap);
	va_end(ap);
}

void
e_vwarnx_log(int prio, const char *fmt, va_list args)
{
	char	buf[MAX_STR];

	CHK_INIT;
	e_buf_format(buf, MAX_STR, e_pname, NULL, fmt, args);
	e_buf_log(buf, prio);
}

void
e_warnx_file(FILE *fp, const char *fmt, ...)
{
	va_list	ap;

	CHK_INIT;
	va_start(ap, fmt);
	e_vwarnx_file(fp, fmt, ap);
	va_end(ap);
}

void
e_vwarnx_file(FILE *fp, const char *fmt, va_list args)
{
	char	buf[MAX_STR];

	CHK_INIT;
	e_buf_format(buf, MAX_STR, e_pname, NULL, fmt, args);
	e_buf_file(buf, fp);
}

int
e_daemon(int nochdir, int noclose, int facility, int prio)
{
	e_use_log();
	e_set_log_facil(facility);
	e_set_log_prio(prio);
	return daemon(nochdir, noclose);
}

