/*$Id: jftp.h,v 1.27 2000/04/04 21:13:52 jens Exp $*/
/*
 * Copyright (c) 1997, 1998, 1999
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

#ifndef JFTP_H
#define JFTP_H

#define JFTP_MAX_LINE    1024
#define JFTP_BUF         16384
#define JFTP_PORT_LEN    40
#define JFTP_TIMEOUT_VAL 120
#define JFTP_BROKEN      1
#define JFTP_UNKNOWNHOST 2
#define JFTP_CONFUSED    3
#define JFTP_WRITEERR    4
#define JFTP_TIMEOUT     5
#define JFTP_ERR         6
#define JFTP_ERRNO       7

#define JFTP_TEMPFILE	"jftp-dir.XXXXXX"
#define JFTP_DIR    	"/tmp"

#define	JFTP_RESPONSE " "

struct ftp_con {
	int		ftp_com;
	int		ftp_data;
	int		ftp_listen;
	char	*ftp_remote_host;
	char	*ftp_user_name;
	char	*ftp_password;
	char	*ftp_remote_dir;
	char	*ftp_my_ip_comma;
	FILE	*ftp_logfile;
	char	ftp_buf[JFTP_BUF];
	char	*ftp_tempdir;
	int		ftp_retries;
	int		ftp_timeout;
	int		ftp_timed_out;
	int		ftp_port;
	int		ftp_verbose;
	int		ftp_passive;
	int		ftp_resp;
#ifdef NO_QUADS
	unsigned long ftp_recd;
	unsigned long ftp_sent;
#else
	u_int64_t ftp_recd;
	u_int64_t ftp_sent;
#endif
	int		ftp_relogins;
	int		ftp_downloads;
	int		ftp_timeouts;
};
/*
 * Differnet values of ftp_verbose:
 *	0	ftp is quiet
 *	1	ftp leaves error messages in logfile
 *	2	ftp SHOUTS in logfile
 */


/* Login on server, returns NULL on failure */
struct ftp_con * ftp_login(char *host, int port, char *username,
			char *password, FILE * logfile, int verbose);

void ftp_unalloc(struct ftp_con *);

/* Login to the same server again */
int 	ftp_relogin(struct ftp_con *);

/* List dir, returns NULL on failure */
char 	*ftp_dir(struct ftp_con *, const char *dir);

/* List current ftp_dir recursive, returns NULL on failure */
char 	*ftp_dir_recurs(struct ftp_con *, char *tempfile);

/* Change directory on server to ftp_dir */
int		ftp_cd(struct ftp_con *, const char *dir);

/* Copy remote_file to local_file, if seekto is nonzero start at byte seekto */
int		ftp_get(struct ftp_con *, char *local_file, char *remote_file,
			size_t seekto);

/* Logout from server */
int		ftp_bye(struct ftp_con *);

/* Set timeout value to n */
void	ftp_set_timeout_val(struct ftp_con *, int n);

/* Issue to port command if active ftp else the pasv command */
int		ftp_port(struct ftp_con *);

/* Make a ftp request req with arg arg */
int		ftp_req(struct ftp_con *, const char *fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

/* read data from server */
int		ftp_read_data(struct ftp_con *, char *buf, size_t size);

/* set mode to passive if passive nonzero */
void	ftp_set_passive(struct ftp_con * c, int passive);

/* specify where temp files should go */
void	ftp_set_tempdir(struct ftp_con * c, char *tempdir);

#endif
