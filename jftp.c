/*$Id: jftp.c,v 1.64 2000/05/27 13:47:43 jens Exp $*/
/*
 * Copyright (c) 1997, 1998, 1999, 2000
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

#ifdef MTYPES
#include "missing/defs.h"
#endif

#ifdef MEMDEBUG
#include <memdebug.h>
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>

#if !(defined(__APPLE__) && defined(__clang__))
/* On macOS with Clang a conflict with the OS headers arises */
#include "strlcpy.h"
#endif

#include "jftp.h"
#include "e_err.h"

#ifdef lint
#undef va_start
#define va_start(x, y) { x = x ;}
#endif

/* Older versions of NetBSD than 1.3K doesn't have socklen_t defined */
#ifdef __NetBSD__
#ifdef __NetBSD_Version
#if __NetBSD_Version__ < 103110000
#define socklen_t int
#endif
#else
#define socklen_t int
#endif /* __NetBSD_Version__ */
#endif /* __NetBSD__ */

/* FreeBSD doesn't have socklen_t defined */
#ifdef __FreeBSD__
#define socklen_t int
#endif

#ifdef NOPROTOS
int	socket(int, int, int);
int     connect(int, const struct sockaddr *, int);
int	setsockopt(int s, int level, int optname, const void *optval, int len);
int	select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		struct timeval *timeout);
void    bzero(void *, size_t);
int	vsnprintf(char *str, size_t size, const char *format, va_list ap);
char	*strdup(const char *);
int     gethostname(char *, int);
int     bind(int, const struct sockaddr *, int);
int     getsockname(int, struct sockaddr *, int *);
int     listen(int, int);
int     accept(int, struct sockaddr * addr, int *);
int     mkstemp(char *);
#endif

int ftp_set_sock_opts(struct ftp_con *c, int fd);
char *ftp_dir2(struct ftp_con * c, const char *flags, const char *dir);

#define FD_CLOSE(fd)		\
	if ((fd) != -1) {		\
		(void) close(fd);	\
		(fd) = -1; 			\
	}

#if defined(lint)
#define __FUNCTION__ "__FUNCTION__"
#endif

#define E_LOGX(level, fmt) \
	e_logx(level, "ftp: %s: " fmt, __FUNCTION__);
#define E_LOGX_1(level, fmt, a1) \
	e_logx(level, "ftp: %s: " fmt, __FUNCTION__, a1);
#define E_LOGX_2(level, fmt, a1, a2) \
	e_logx(level, "ftp: %s: " fmt, __FUNCTION__, a1, a2);
#define E_LOGX_3(level, fmt, a1, a2, a3) \
	e_logx(level, "ftp: %s: " fmt, __FUNCTION__, a1, a2, a3);
#define E_LOGX_4(level, fmt, a1, a2, a3, a4) \
	e_logx(level, "ftp: %s: " fmt, __FUNCTION__, a1, a2, a3, a4);

#define E_LOG(level, fmt) \
	e_log(level, "ftp: %s: " fmt, __FUNCTION__);
#define E_LOG_1(level, fmt, a1) \
	e_log(level, "ftp: %s: " fmt, __FUNCTION__, a1);
#define E_LOG_2(level, fmt, a1, a2) \
	e_log(level, "ftp: %s: " fmt, __FUNCTION__, a1, a2);
#define E_LOG_3(level, fmt, a1, a2, a3) \
	e_log(level, "ftp: %s: " fmt, __FUNCTION__, a1, a2, a3);
#define E_LOG_4(level, fmt, a1, a2, a3, a4) \
	e_log(level, "ftp: %s: " fmt, __FUNCTION__, a1, a2, a3, a4);

#ifdef INET6
/* translate IPv4 mapped IPv6 address to IPv4 address */
static void
unmappedaddr(struct sockaddr_in6 *sin6, int *len)
{
	struct sockaddr_in *sin4;
	u_int32_t addr;
	int port;

	if (sin6->sin6_family != AF_INET6 ||
	    !IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr))
		return;
	sin4 = (struct sockaddr_in *)sin6;
	addr = *(u_int32_t *)&sin6->sin6_addr.s6_addr[12];
	port = sin6->sin6_port;
	memset(sin4, 0, sizeof(struct sockaddr_in));
	sin4->sin_addr.s_addr = addr;
	sin4->sin_port = port;
	sin4->sin_family = AF_INET;
#ifdef SIN6_LEN
	sin4->sin_len = sizeof(struct sockaddr_in);
#endif
	*len = sizeof(struct sockaddr_in);
}
#endif

void
ftp_set_timeout_val(struct ftp_con *c, int n)
{
	c->ftp_timeout = n < 0 ? 150 : n;
}

void
ftp_set_passive(struct ftp_con *c, int passive)
{
	c->ftp_passive = passive;
}

void
ftp_set_tempdir(struct ftp_con *c, char *tempdir)
{
	c->ftp_tempdir = tempdir;
}

static ssize_t
ftp_write(struct ftp_con *c, int fd, void *buf, size_t nbytes)
{
	ssize_t	res;
#ifndef SO_RCVTIMEO
	struct	timeval tv;
	fd_set	fdset;

	tv.tv_sec = c->ftp_timeout;
	tv.tv_usec = 0;
	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	do
		res = select(fd + 1, NULL, &fdset, &tv);
	while (res < 0 && errno == EINTR);
	if (res < 0)
		return res;
	if (res == 0) {
		c->ftp_timed_out = 1;
		return -1;
	}
	do
		res = write(fd, buf, nbytes);
	while (res < 0 && errno == EINTR);
#endif
	do
		res = write(fd, buf, nbytes);
	while (res < 0 && errno == EINTR);
	c->ftp_timed_out = (res < 0 && errno == EWOULDBLOCK);
	return res;
}

static ssize_t
ftp_read(struct ftp_con *c, int fd, void *buf, size_t nbytes)
{
	ssize_t	res;
#ifndef SO_RCVTIMEO
	struct	timeval tv;
	fd_set	fdset;

	tv.tv_sec = c->ftp_timeout;
	tv.tv_usec = 0;
	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	do
		res = select(fd + 1, &fdset, NULL, &tv);
	while (res < 0 && errno == EINTR);
	if (res < 0)
		return res;
	if (res == 0) {
		c->ftp_timed_out = 1;
		return -1;
	}
	do
		res = read(fd, buf, nbytes);
	while (res < 0 && errno == EINTR);
#endif
	do
		res = read(fd, buf, nbytes);
	while (res < 0 && errno == EINTR);
	c->ftp_timed_out = (res < 0 && errno == EWOULDBLOCK);
	return res;
}

int
ftp_req(struct ftp_con *c, const char *fmt, ...)
{
	va_list	ap;
	int     ftp_status, eol, i, islongtext;
	ssize_t	done, res, size, pos;
	char	*nfmt;
	char	rescode[5];

	if (*fmt != ' ') {
		if ((nfmt = alloca(strlen(fmt) + 3)) == NULL) {
			c->ftp_resp = JFTP_ERRNO;
			return -1;
		}
		(void)strcpy(nfmt, fmt);
		(void)strcat(nfmt, "\r\n");
		va_start(ap, fmt);
		(void)vsnprintf(c->ftp_buf, sizeof(c->ftp_buf), nfmt, ap);
		va_end(ap);
		E_LOGX_2(2, "command: %s %d", c->ftp_buf, 3);
		res = ftp_write(c, c->ftp_com, c->ftp_buf, strlen(c->ftp_buf));

		/* sometimes it's possible to get res = 0 under Solaris */
		if (res <= 0 || c->ftp_timed_out)
			goto io_err;

		c->ftp_sent += res;
	}
	(void)memset(c->ftp_buf, 0, sizeof(c->ftp_buf));
	size = sizeof(c->ftp_buf);
	pos = 0;
	eol = 0;
	islongtext = 0;
	ftp_status = 0;
	do {
		done = ftp_read(c, c->ftp_com, c->ftp_buf + pos, (size_t)1);

		/* We shouldn't get done == 0 since we
		 * are expecting a newline
		 */
		if (done <= 0 || c->ftp_timed_out)
			goto io_err;

		c->ftp_recd += done;
		for (i = (int)pos; i < (pos + done); i++) {
			if (c->ftp_buf[i] == '\n') {
				eol = 1;
				c->ftp_buf[i] = 0;
				break;
			}
		}
		size -= done;
		pos += done;
		if(eol) {
			ftp_status=0;

			/* We are interested in the first 4 bytes */
			(void)memset(rescode, '\0', sizeof(rescode));
			(void)memcpy(rescode, c->ftp_buf, sizeof(rescode) - 1);
			
			if (isdigit(rescode[0])
			    && isdigit(rescode[1])
			    && isdigit(rescode[2])) {
				res = sscanf(rescode, "%3d", &ftp_status);

				E_LOGX_1(3, "ftp_status: %d", ftp_status);
				E_LOGX_1(3, "c->ftp_buf: %s", c->ftp_buf);

				if ((rescode[3] == '-' || rescode[3] == ' ')
				    && (res == 1)) {
					islongtext = (rescode[3] == '-');
					if (ftp_status < 100) {
						E_LOGX_1(1, "%s", c->ftp_buf);
						c->ftp_resp = JFTP_ERR;
						return -1;
					}
				}
			}
			if(islongtext) {
				size = sizeof(c->ftp_buf);
				pos = 0;
				eol = 0;
			}
		}
	} while (size && !eol);
	if ((!eol) || (!ftp_status)) {
		E_LOGX_1(1, "%s", c->ftp_buf);
		c->ftp_resp = JFTP_ERR;
		return -1;
	}
	c->ftp_resp = ftp_status;
	return 0;

io_err:
	if (c->ftp_timed_out) {
		E_LOGX(0, "timeout");
		c->ftp_resp = JFTP_TIMEOUT;
	} else {
		E_LOGX(0, "command connection broken");
		c->ftp_resp = JFTP_BROKEN;
	}
	FD_CLOSE(c->ftp_com);
	FD_CLOSE(c->ftp_listen);
	FD_CLOSE(c->ftp_data);
	return -1;
}

struct ftp_con *
ftp_login(char *host, int port, int family, char *username,
	char *password, FILE * logfile, int verbose)
{
	struct	ftp_con *c;

	c = calloc((size_t)1, sizeof(*c));
	if ((c->ftp_remote_host = strdup(host)) == NULL)
		goto ret_bad;
	if ((c->ftp_user_name = strdup(username)) == NULL)
		goto ret_bad;
	if ((c->ftp_password = strdup(password)) == NULL)
		goto ret_bad;
	c->ftp_logfile = logfile;
	c->ftp_verbose = verbose;
	e_set_level(verbose); /* XXX - global */
	c->ftp_com = -1;
	c->ftp_listen = -1;
	c->ftp_data = -1;
	c->ftp_port = port;
	c->ftp_family = family;
	c->ftp_retries = 20;
	c->ftp_timeout = JFTP_TIMEOUT_VAL;
	c->ftp_relogins = -1;
	if ((c->ftp_tempdir = strdup(JFTP_DIR)) == NULL)
		goto ret_bad;

	if (ftp_relogin(c) < 0)
		goto ret_bad;

	return c;

ret_bad:
	ftp_unalloc(c);
	return NULL;
}

void
ftp_unalloc(struct ftp_con *c)
{
#define FC_FREE(var) if (c->var) free(c->var)

	if (c == NULL)
		return;
	FC_FREE(ftp_remote_host);
	FC_FREE(ftp_user_name);
	FC_FREE(ftp_password);
	FC_FREE(ftp_tempdir);
	free(c);
}

int
ftp_relogin(struct ftp_con *c)
{
#ifdef INET6
	struct	addrinfo hints, *res0 = NULL, *res;
	char	str_port[NI_MAXSERV];
#else
	struct	sockaddr_in server;
	struct	hostent *hp;
#endif

	c->ftp_relogins++;
	FD_CLOSE(c->ftp_com);
	FD_CLOSE(c->ftp_listen);
	FD_CLOSE(c->ftp_data);
#ifdef INET6
	snprintf(str_port, sizeof(str_port), "%d", c->ftp_port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = c->ftp_family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;
	if (getaddrinfo(c->ftp_remote_host, str_port, &hints, &res0) != 0) {
		E_LOGX_1(0, "getaddrinfo(%s): failed", c->ftp_remote_host);
		c->ftp_resp = JFTP_ERR;
		return -1;
	}
	c->ftp_com = -1;
	for (res = res0; res; res = res->ai_next) {
		c->ftp_com = socket(res->ai_family, res->ai_socktype,
				    res->ai_protocol);
		if (c->ftp_com < 0)
			continue;
		if (connect(c->ftp_com, res->ai_addr, res->ai_addrlen) >= 0)
			break;
		FD_CLOSE(c->ftp_com);
		c->ftp_com = -1;
	}
	freeaddrinfo(res0);
	if (c->ftp_com < 0) {
		E_LOG(0, "connect");
		c->ftp_resp = JFTP_ERR;
		return -1;
	}
#else
	c->ftp_com = socket(AF_INET, SOCK_STREAM, 0);
	if (c->ftp_com < 0) {
		E_LOG(0, "socket");
		c->ftp_resp = JFTP_ERR;
		return -1;
	}
	server.sin_family = AF_INET;
	hp = gethostbyname(c->ftp_remote_host);
	if (hp == NULL) {
		E_LOGX_1(0, "gethostbyname(%s): failed", c->ftp_remote_host);
		FD_CLOSE(c->ftp_com);
		c->ftp_resp = JFTP_ERR;
		return -1;
	}
	(void) memcpy(&server.sin_addr, hp->h_addr, sizeof(server.sin_addr));
	server.sin_port = htons(c->ftp_port);

	/* NOSTRICT server */
	if (connect(c->ftp_com, (struct sockaddr *) & server, sizeof(server)) < 0) {
		E_LOG(0, "connect");
		FD_CLOSE(c->ftp_com);
		c->ftp_resp = JFTP_ERR;
		return -1;
	}
#endif
	if (ftp_req(c, JFTP_RESPONSE) < 0 || c->ftp_resp != 220) {
		E_LOGX_1(0, "unexpected greeting from server: %s", c->ftp_buf);
		FD_CLOSE(c->ftp_com);
		c->ftp_resp = JFTP_ERR;
		return -1;
	}
	if (ftp_req(c, "user %s", c->ftp_user_name) < 0 
		|| (c->ftp_resp != 331 && c->ftp_resp != 230)) {
		E_LOGX_1(0, "Username %s: failed", c->ftp_user_name);
		FD_CLOSE(c->ftp_com);
		c->ftp_resp = JFTP_ERR;
		return -1;
	}
	/* USER command can respond 230 immediately in some cases */
	if (c->ftp_resp != 230) {
		if (ftp_req(c, "pass %s", c->ftp_password) < 0
			|| c->ftp_resp != 230) {
			E_LOGX(0, "Password xxxxx: failed");
			FD_CLOSE(c->ftp_com);
			c->ftp_resp = JFTP_ERR;
			return -1;
		}
	}
	c->ftp_resp = 0;
	if (ftp_req(c, "TYPE I") < 0 || c->ftp_resp != 200) {
		E_LOGX(0, "Setting BIN type: failed");
		FD_CLOSE(c->ftp_com);
		c->ftp_resp = JFTP_ERR;
		return -1;
	}
	if (c->ftp_remote_dir != NULL && strcmp(c->ftp_remote_dir, "/") != 0)
		return ftp_cd(c, c->ftp_remote_dir);
	return 0;
}


int
ftp_cd(struct ftp_con *c, const char *dir)
{
	char	*p;
	int		res;
	
	if ((p = strdup(dir)) == NULL) { /* Thank You John Polstra */
		c->ftp_resp = JFTP_ERRNO;
		return -1;
	}

	/* Save new directory on server */
	if (c->ftp_remote_dir != NULL)
		free(c->ftp_remote_dir);
	c->ftp_remote_dir = p;

	/* try to change directory on server */
	res = ftp_req(c, "CWD %s", c->ftp_remote_dir);

	/* req failed, ftp_resp has the error */
	if (res < 0)	
		return res;

	/* our request wasn't sucessful */
	if (c->ftp_resp != 250) {
		c->ftp_resp = JFTP_ERR;
		return -1;
	}
	return 0;
}

/* ARGSUSED */
int
ftp_set_sock_opts(struct ftp_con *c, int fd)
{
#if !defined(ULTRIX)
	int		buf_size = 65536;	/* 64k, this can speed up if
								 * delay x bandwidth is large
								 */
	static	int failed = 0;

	c = NULL; /* quiet gcc */
	
	/* Don't make the same misstake again */
	if (failed)
		return -1;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buf_size,
		(socklen_t)sizeof(buf_size)) < 0) {

		E_LOG_1(1, "setsockopt: SO_RCVBUF = %d", buf_size);
		failed = 1;
		return -1;
	}
#endif
	return 0;
}

int
ftp_port(struct ftp_con *c)
{
	char	*p, *a;
#ifdef INET6
	struct	sockaddr_storage sin2;
	struct	sockaddr_in6 *sin6;
	char	hname[INET6_ADDRSTRLEN];
	u_char	addr[4];
	int	port;
#else
	struct	sockaddr_in sin2;
#endif
	struct	sockaddr_in *sin4;
	int	arg;
	int     len, res;
	int		a0, a1, a2, a3, p0, p1;

	c->ftp_downloads++;

	len = sizeof(sin2);
	if (getsockname(c->ftp_com, (struct sockaddr *)&sin2, &len) == -1) {
		E_LOG(0, "getsockname");
		FD_CLOSE(c->ftp_com);
		c->ftp_resp = JFTP_BROKEN;
		return -1;
	}
	switch (((struct sockaddr *)&sin2)->sa_family) {
	case AF_INET:
		break;
#ifdef INET6
	case AF_INET6:
		unmappedaddr((struct sockaddr_in6 *)&sin2, &len);
		break;
#endif
	default:
		E_LOG(0, "getsockname");
		FD_CLOSE(c->ftp_com);
		c->ftp_resp = JFTP_BROKEN;
		return -1;
	}

	/* doing active ftp */
	if (!c->ftp_passive) {
		/* sanity check */
		if (c->ftp_listen != -1) {
			FD_CLOSE(c->ftp_listen);
			E_LOGX(1, "ERROR This shouldn't happen, "
				"data connection wasn't closed");
		}

		/* create listen socket */
		c->ftp_listen = socket(((struct sockaddr *)&sin2)->sa_family,
				       SOCK_STREAM, 0);
		if (c->ftp_listen < 0) {
			E_LOGX(0, "socket");
			c->ftp_listen = -1;
			c->ftp_resp = JFTP_BROKEN;
			return -1;
		}

		/* bind socket */
		switch (((struct sockaddr *)&sin2)->sa_family) {
		case AF_INET:
			((struct sockaddr_in *)&sin2)->sin_port = 0;
#ifdef IP_PORTRANGE
			arg = IP_PORTRANGE_HIGH;
			setsockopt(c->ftp_listen, IPPROTO_IP, IP_PORTRANGE,
				   (char *)&arg, sizeof(arg));
#endif
			break;
#ifdef INET6
		case AF_INET6:
			((struct sockaddr_in6 *)&sin2)->sin6_port = 0;
#ifdef IPV6_PORTRANGE
			arg = IPV6_PORTRANGE_HIGH;
			setsockopt(c->ftp_listen, IPPROTO_IPV6, IPV6_PORTRANGE,
				   (char *)&arg, sizeof(arg));
#endif
			break;
#endif
		}
		if (bind(c->ftp_listen, (struct sockaddr *) &sin2, len) < 0) {
			E_LOG(0, "bind");
			c->ftp_resp = JFTP_BROKEN;
			FD_CLOSE(c->ftp_listen);
			FD_CLOSE(c->ftp_com);
			return -1;
		}

		/* get the port we are going to listen on */
		len = sizeof(sin2);
		/* NOSTRICT sin2 */
		if (getsockname(c->ftp_listen, (struct sockaddr *) &sin2, &len) < 0) {
			E_LOG(0, "getsockname");
			FD_CLOSE(c->ftp_listen);
			FD_CLOSE(c->ftp_com);
			c->ftp_resp = JFTP_BROKEN;
			return -1;
		}

		(void) ftp_set_sock_opts(c, c->ftp_listen);
		if (listen(c->ftp_listen, 1) < 0) {
			E_LOG(0, "listen");
			FD_CLOSE(c->ftp_listen);
			FD_CLOSE(c->ftp_com);
			c->ftp_resp = JFTP_BROKEN;
			return -1;
		}
	
		/* do the port command */
		switch (((struct sockaddr *)&sin2)->sa_family) {
		case AF_INET:
			sin4 = (struct sockaddr_in *)&sin2;
			a = (char *)&sin4->sin_addr.s_addr;
			p = (char *)&sin4->sin_port;
			res = ftp_req(c, "PORT %d,%d,%d,%d,%d,%d",
				      a[0] & 0xff, a[1] & 0xff,
				      a[2] & 0xff, a[3] & 0xff,
				      p[0] & 0xff, p[1] & 0xff);
			break;
#ifdef INET6
		case AF_INET6:
			sin6 = (struct sockaddr_in6 *)&sin2;
			if (getnameinfo((struct sockaddr *)&sin2, sin2.ss_len,
					hname, sizeof(hname),
					NULL, 0, NI_NUMERICHOST) != 0) {
				E_LOG(0, "getnameinfo");
				FD_CLOSE(c->ftp_listen);
				FD_CLOSE(c->ftp_com);
				c->ftp_resp = JFTP_BROKEN;
				return -1;
			}
			res = ftp_req(c, "EPRT |%d|%s|%d|", 2, hname,
				      htons(sin6->sin6_port));
			break;
#endif
		}
		if (res < 0 || c->ftp_resp != 200) {
			if (res >= 0)
				c->ftp_resp = JFTP_ERR;
			FD_CLOSE(c->ftp_listen);
			return -1;
		}
		return 0;
	} else { /* passive ftp */
		switch (((struct sockaddr *)&sin2)->sa_family) {
		case AF_INET:
			res = ftp_req(c, "PASV");
			if (res < 0 || c->ftp_resp != 227) {
				if (res >= 0)
					c->ftp_resp = JFTP_ERR;
				return -1;
			}
			for (p = c->ftp_buf; *p != '\0' && *p != '('; p++);
			res = sscanf(p, "(%d,%d,%d,%d,%d,%d",
				     &a0, &a1, &a2, &a3, &p0, &p1);
			if (res != 6) {
				c->ftp_resp = JFTP_ERR;
				return -1;
			}
			sin4 = (struct sockaddr_in *)&sin2;
			/* NOSTRICT a */
			a = (char *)&sin4->sin_addr.s_addr;
			a[0] = a0 & 0xff;
			a[1] = a1 & 0xff;
			a[2] = a2 & 0xff;
			a[3] = a3 & 0xff;
			/* NOSTRICT p */
			p = (char *)&sin4->sin_port;
			p[0] = p0 & 0xff;
			p[1] = p1 & 0xff;
			sin4->sin_family = AF_INET;
			break;
#ifdef INET6
		case AF_INET6:
			res = ftp_req(c, "EPSV");
			if (res < 0 || c->ftp_resp != 229) {
				if (res >= 0)
					c->ftp_resp = JFTP_ERR;
				return -1;
			}
			for (p = c->ftp_buf; *p != '\0' && *p != '('; p++);
			if (*p == '\0') {
				c->ftp_resp = JFTP_ERR;
				return -1;
			}
			++p;
			res = sscanf(p, "%c%c%c%d%c", &addr[0], &addr[1],
				     &addr[2], &port, &addr[3]);
			if (res != 5 || addr[0] != addr[1] ||
			    addr[0] != addr[2] || addr[0] != addr[3]) {
				c->ftp_resp = JFTP_ERR;
				return -1;
			}
			len = sizeof(sin2);
			res = getpeername(c->ftp_com,
					  (struct sockaddr *)&sin2, &len);
			if (res == -1 || sin2.ss_family != AF_INET6) {
				E_LOG(0, "getpeername");
				c->ftp_resp = JFTP_BROKEN;
				return -1;
			}
			sin6 = (struct sockaddr_in6 *)&sin2;
			sin6->sin6_port = htons(port);
			break;
#endif
		}

		/* sanity check */
		if (c->ftp_data != -1) {
			FD_CLOSE(c->ftp_data);
			E_LOGX(1, "ERROR1 This shouldn't happen, "
				"data connection wasn't closed");
		}
		/* connect to server */
		c->ftp_data = socket(((struct sockaddr *)&sin2)->sa_family,
				     SOCK_STREAM, 0);
		if (c->ftp_data < 0) {
			E_LOG(0, "socket");
			c->ftp_resp = JFTP_ERR;
			return -1;
		}
		(void)ftp_set_sock_opts(c, c->ftp_data);
		if (connect(c->ftp_data, 
				/* NOSTRICT sin2 */
				(struct sockaddr *)&sin2, len) < 0) {
			E_LOG(0, "connect");
			FD_CLOSE(c->ftp_data);
			c->ftp_resp = JFTP_ERR;
			return -1;
		}
		return 0;
	}
	/* NOT REACHED */
}

int
ftp_read_data(struct ftp_con *c, char *buf, size_t size)
{
	int     res;
	ssize_t	done = 0;
	struct	timeval tv;
	fd_set	fdset;
#ifndef SO_RCVTIMEO
	int		flags;
#endif

	if ((c->ftp_data == -1) && (c->ftp_listen != -1)) {
		tv.tv_sec = c->ftp_timeout;
		tv.tv_usec = 0;
		FD_ZERO(&fdset);
		/* LINTED fdset */
		FD_SET(c->ftp_listen, &fdset);
		do
			res = select(c->ftp_listen + 1, &fdset, NULL, NULL, &tv);
		while (res < 0 && errno == EINTR);
		if (res < 0) {
			E_LOG(0, "select");
			c->ftp_resp = JFTP_ERR;
			return -1;
		}
		if (res == 0) {
			FD_CLOSE(c->ftp_listen);
			c->ftp_resp = JFTP_TIMEOUT;
			c->ftp_timeouts++;
			return -1;
		}
		
		c->ftp_data = accept(c->ftp_listen, 0, 0);
		FD_CLOSE(c->ftp_listen);
		if (c->ftp_data < 0) {
			E_LOG(0, "accept");
			c->ftp_data = -1;
			c->ftp_resp = JFTP_BROKEN;
			return -1;
		}
#ifdef SO_RCVTIMEO
		if (setsockopt(c->ftp_data, SOL_SOCKET, SO_RCVTIMEO,
/* LINTED */
			(char *) &tv, (size_t) sizeof(tv)) < 0) {
			E_LOG(0, "setsockopt");
			FD_CLOSE(c->ftp_data);
			c->ftp_resp = JFTP_ERR;
			return -1;
		}
#else
		if ((flags = fcntl(c->ftp_data, F_GETFL, 0)) < 0) {
			E_LOG(0, "fcntl F_GETFL");
			FD_CLOSE(c->ftp_data);
			c->ftp_resp = JFTP_ERR;
			return -1;
		}
		flags |= O_NONBLOCK;
		if (fcntl(c->ftp_data, F_SETFS, flags) < 0) {
			E_LOG(0, "fcntl F_SETFS");
			FD_CLOSE(c->ftp_data);
			c->ftp_resp = JFTP_ERR;
			return -1;
		}
#endif
	}
	done = ftp_read(c, c->ftp_data, buf, size);
	if (done < 0 || c->ftp_timed_out) {
		FD_CLOSE(c->ftp_data);
		if (c->ftp_timed_out)
			c->ftp_resp = JFTP_TIMEOUT;
		else
			c->ftp_resp = JFTP_BROKEN;
		return -1;
	}
	c->ftp_recd += done;
	c->ftp_resp = (int)done;
	return 0;
}

int
ftp_get(struct ftp_con *c, char *local_file, char *remote_file, size_t seekto)
{
	FILE   *fp;
	int     done, recieved;
	ssize_t	res;

	if (seekto > 0) {
		fp = fopen(local_file, "ab");
		if (fp == NULL) {
			E_LOG_1(0, "opening of %s for writing failed", local_file);
			c->ftp_resp = JFTP_WRITEERR;
			return -1;
		}
		if (fseek(fp, (long) seekto, SEEK_SET) < 0) {
			E_LOG_2(0, "fseek to pos %d in %s failed",
				(unsigned) seekto, local_file);
			c->ftp_resp = JFTP_ERRNO;
			return -1;
		}
	} else {
		fp = fopen(local_file, "wb");
		if (fp == NULL) {
			E_LOG_1(0, "creation of %s failed", local_file);
			c->ftp_resp = JFTP_WRITEERR;
			return -1;
		}
	}
	if (ftp_port(c) < 0) {
		E_LOGX(0, "PORT command failed");
		if (c->ftp_resp != JFTP_BROKEN && c->ftp_resp != JFTP_TIMEOUT)
			c->ftp_resp = JFTP_ERR;
		goto ret_bad;
		return -1;
	}
	if (seekto > 0) {
		res = ftp_req(c, "REST %lu", (unsigned long) seekto);
		if (res < 0 || c->ftp_resp != 350) {
			E_LOGX(0, "REST command failed");
			if (res >= 0) 
				c->ftp_resp = JFTP_ERR;
			goto ret_bad;
		}
	}
	res = ftp_req(c, "RETR %s", remote_file);
	if (res < 0 || c->ftp_resp != 150) {
		E_LOGX(0, "RETR command failed");
		if (res >= 0)
			c->ftp_resp = JFTP_ERR;
		goto ret_bad;
	}
	recieved = 0;
	if (ftp_read_data(c, c->ftp_buf, sizeof(c->ftp_buf)) < 0)
		goto ret_bad;
	done = c->ftp_resp;
	while (done > 0) {
		recieved += done;
		res = fwrite(c->ftp_buf, (size_t)done, (size_t)1, fp);
		if (res < 0) {
			E_LOG(0, "write");
				/* XXX can't report error if this failes */
			(void) ftp_req(c, JFTP_RESPONSE);
			c->ftp_resp = JFTP_WRITEERR;
			goto ret_bad;
		}
		if (ftp_read_data(c, c->ftp_buf, sizeof(c->ftp_buf)) < 0)
			goto ret_bad;
		done = c->ftp_resp;
	}

	(void)fclose(fp);
	FD_CLOSE(c->ftp_listen);
	FD_CLOSE(c->ftp_data);
	res = ftp_req(c, JFTP_RESPONSE);
	if (res < 0) {
		E_LOGX(0, "broken connection");
		FD_CLOSE(c->ftp_com);
		c->ftp_resp = JFTP_BROKEN;
		goto ret_bad;
	}
	if (c->ftp_resp != 226) {
		E_LOGX_1(0, "retrieve of %s was aborted by server", remote_file);
		c->ftp_resp = JFTP_ERR;
		goto ret_bad;
	}
	return 0;

ret_bad:
	FD_CLOSE(c->ftp_listen);
	FD_CLOSE(c->ftp_data);
	(void)fclose(fp);
	return -1;
}

char *
ftp_dir2(struct ftp_con * c, const char *flags, const char *dir)
{
	int     done, recieved;
	ssize_t	res;
	char	*tmp;
	int     fd;

	tmp = malloc(strlen(c->ftp_tempdir) + strlen(JFTP_TEMPFILE) + 2);
	if (tmp == NULL) {
		c->ftp_resp = JFTP_ERRNO;
		return NULL;
	}
	(void) sprintf(tmp, "%s/%s", c->ftp_tempdir, JFTP_TEMPFILE);

	fd = mkstemp(tmp);
	if (fd < 0) {
		c->ftp_resp = JFTP_WRITEERR;
		E_LOG_1(0, "mkstemp(%s)", tmp);
		return NULL;
	}
	if (ftp_port(c) < 0) {
		E_LOGX(0, "PORT command failed");
		FD_CLOSE(c->ftp_listen);
		FD_CLOSE(c->ftp_data);
		(void) close(fd);
		(void) unlink(tmp);
		return NULL;
	}

	/* Late versions of wu-ftpd does some kind of recursive
	 * listing if only a '.' is given as directory.
	 */
	if ((strcmp(dir, ".") == 0) || !*dir)
		res = ftp_req(c, "list %s", flags);
	else
		res = ftp_req(c, "list %s %s", flags, dir);

	if (res < 0 || c->ftp_resp != 150) {
		FD_CLOSE(c->ftp_listen);
		FD_CLOSE(c->ftp_data);
		E_LOGX_1(0, "LIST command for %s failed", dir);
		if (res >= 0)
			c->ftp_resp = JFTP_ERR;
		(void) close(fd);
		(void) unlink(tmp);
		return NULL;
	}
	recieved = 0;
	if (ftp_read_data(c, c->ftp_buf, sizeof(c->ftp_buf)) < 0) {
		(void) close(fd);
		(void) unlink(tmp);
		return NULL;
	}
	done = c->ftp_resp;
	while (done > 0) {
		recieved += done;
		while((res = write(fd, c->ftp_buf, (size_t)done)) < 0 &&
			errno == EINTR);
		if (res < 0) {
			E_LOG_1(0, "write(%s)", tmp);
			c->ftp_resp = JFTP_WRITEERR;
			FD_CLOSE(c->ftp_listen);
			FD_CLOSE(c->ftp_data);
			(void) close(fd);
			(void) unlink(tmp);
			(void) ftp_req(c, JFTP_RESPONSE);
			return NULL;
		}
		if (ftp_read_data(c, c->ftp_buf, sizeof(c->ftp_buf)) < 0) {
			FD_CLOSE(c->ftp_listen);
			FD_CLOSE(c->ftp_data);
			(void) close(fd);
			(void) unlink(tmp);
			return NULL;
		}
		done = c->ftp_resp;
	}

	(void) close(fd);
	FD_CLOSE(c->ftp_listen);
	FD_CLOSE(c->ftp_data);
	res = ftp_req(c, JFTP_RESPONSE);
	if (res < 0) {
		E_LOGX(0, "broken connection");
		FD_CLOSE(c->ftp_com);
		c->ftp_resp = JFTP_BROKEN;
		(void) unlink(tmp);
		return NULL;
	}
	if (c->ftp_resp != 226) {
		E_LOGX_1(0, "retrieve of dir %s was aborted by server", dir);
		(void) unlink(tmp);
		return NULL;
	}
	return tmp;
}

char *
ftp_dir(struct ftp_con *c, const char *dir)
{
	char	buf[PATH_MAX];
	char	*res;

	strlcpy(buf, c->ftp_remote_dir, PATH_MAX);
	if (ftp_cd(c, dir) < 0)
		return NULL;
	res = ftp_dir2(c, "-lag", "");
	if (ftp_cd(c, buf) < 0)
		return NULL;	/* XXX - maybe abort here */
	return res;
}

#if NOTYET
char *
ftp_dir_recurs(struct ftp_con *c, char *tempfile)
{
	return ftp_dir2(c, "-lagR", tempfile);
}
#endif


int 
ftp_bye(struct ftp_con * c)
{
	int     res;

	res = ftp_req(c, "quit");
	FD_CLOSE(c->ftp_com);
	FD_CLOSE(c->ftp_listen);
	FD_CLOSE(c->ftp_data);
	if (res >= 0 && c->ftp_resp == 221)
		return 0;
	return -1;
}
