/* $Id: e_err.h,v 1.12 2000/05/14 14:39:39 jens Exp $ */
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

#ifndef E_ERR__H
#define E_ERR__H

#include <stdarg.h>
#include <stdio.h>

#ifdef MEMDEBUG
#include <memdebug.h>
#endif

/* drop in replacement for err.h */
#define	err		e_err
#define	errx	e_errx
#define	warn	e_warn
#define	warnx	e_warnx

/* default is progname = __progname and file = stderr */

#ifdef __cplusplus
extern "C" {
#endif

void e_set_progname(char *name);
const char *e_get_progname(void);

void e_set_log_facil(int facility);
int e_get_log_facil(void);
void e_set_log_prio(int prio);
int e_get_log_prio(void);
void e_use_log(void);

void e_set_file(FILE *fp);
FILE *e_get_file(void);
void e_use_file(void);

void e_set_level(int level);
int e_get_level(void);

void e_set_section(int section);
int e_get_section(void);

void e_set_newline_str(const char *str);
const char *e_get_newline_str(void);

void e_buf_log(char *buf, int prio);
void e_buf_file(char *buf, FILE *fp);
void e_buf_format(char *buf, size_t len,
	const char *name, char *error, const char *fmt, va_list args);

/* does e_use_log and the daemon(3) */
int e_daemon(int nochdir, int noclose, int facility, int prio);

/* suitable to use as a complement to usage() on error */
void e_usage(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));

/* exit with eval and print error mesage with strerror(errno)
 * appended to message.  */
void e_verr(int eval, const char *fmt, va_list args);
void e_err(int eval, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

void e_verr_log(int eval, int prio, const char *fmt, va_list args);
void e_err_log(int eval, int prio, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

void e_verr_file(int eval, FILE *fp, const char *fmt, va_list args);
void e_err_file(int eval, FILE *fp, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

/* exit with eval and print error mesage */
void e_verrx(int eval, const char *fmt, va_list args);
void e_errx(int eval, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

void e_verrx_log(int eval, int prio, const char *fmt, va_list args);
void e_errx_log(int eval, int prio, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

void e_verrx_file(int eval, FILE *fp, const char *fmt, va_list args);
void e_errx_file(int eval, FILE *fp, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

/* print error mesage with strerror(errno) appended to message.  */
void e_vtrace(int level, int section, const char *fmt, va_list args);
void e_trace(int level, int section, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

void e_vtrace_log(int level, int section, int prio,
	const char *fmt, va_list args);
void e_trace_log(int level, int section, int prio, const char *fmt, ...)
	__attribute__ ((format (printf, 4, 5)));

void e_vtrace_file(int level, int section, FILE *fp,
	const char *fmt, va_list args);
void e_trace_file(int level, int section, FILE *fp, const char *fmt, ...)
	__attribute__ ((format (printf, 4, 5)));

void e_vlog(int level, const char *fmt, va_list args);
void e_log(int level, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

void e_vlog_log(int level, int prio, const char *fmt, va_list args);
void e_log_log(int level, int prio, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

void e_vlog_file(int level, FILE *fp, const char *fmt, va_list args);
void e_log_file(int level, FILE *fp, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

void e_vwarn(const char *fmt, va_list args);
void e_warn(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));

void e_vwarn_log(int prio, const char *fmt, va_list args);
void e_warn_log(int prio, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

void e_vwarn_file(FILE *fp, const char *fmt, va_list args);
void e_warn_file(FILE *fp, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

/* print error mesage */
void e_vtracex(int level, int section, const char *fmt, va_list args);
void e_tracex(int level, int section, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

void e_vtracex_log(int level, int section, int prio,
	const char *fmt, va_list args);
void e_tracex_log(int level, int section, int prio, const char *fmt, ...)
	__attribute__ ((format (printf, 4, 5)));

void e_vtracex_file(int level, int section, FILE *fp,
	const char *fmt, va_list args);
void e_tracex_file(int level, int section, FILE *fp, const char *fmt, ...)
	__attribute__ ((format (printf, 4, 5)));

void e_vlogx(int level, const char *fmt, va_list args);
void e_logx(int level, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

void e_vlogx_log(int level, int prio, const char *fmt, va_list args);
void e_logx_log(int level, int prio, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

void e_vlogx_file(int level, FILE *fp, const char *fmt, va_list args);
void e_logx_file(int level, FILE *fp, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

void e_vwarnx(const char *fmt, va_list args);
void e_warnx(const char *fmt, ...)
	__attribute__ ((format (printf, 1, 2)));

void e_vwarnx_log(int prio, const char *fmt, va_list args);
void e_warnx_log(int prio, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

void e_vwarnx_file(FILE *fp, const char *fmt, va_list args);
void e_warnx_file(FILE *fp, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

#ifdef __cplusplus
}
#endif

#endif /* E_ERR__H */
