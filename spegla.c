/*$Id: spegla.c,v 1.117 2000/05/27 13:38:14 jens Exp $*/
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

#ifdef MEMDEBUG
#include <memdebug.h>
#endif

#include <sys/types.h>
#ifdef MTYPES
#include "missing/defs.h"
#endif
#ifdef Solaris
#include <sys/statvfs.h>
#endif
#ifdef LINUX
#include <sys/statfs.h>
#endif
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/socket.h>

#if defined(SunOS) || defined(Solaris) || defined(OSF1) || defined(ULTRIX)
#	include <sys/time.h>
#else
#	include <time.h>
#endif

#if defined(LINUX)
#	include <sys/time.h>
#endif

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <utime.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>

#if !(defined(__APPLE__) && defined(__clang__))
/* On macOS with Clang a conflict with the OS headers arises */
#include "strlcpy.h"
#endif
#include "spegla.h"
#include "parserow.h"
#include "jftp.h"
#include "spf_util.h"
#include "tgetopt.h"
#include "e_err.h"

#include <assert.h>

#ifdef NOPROTOS
char	*strdup(char *);
#ifdef ULTRIX
int	statfs(char *, struct fs_data *);
#else
int	statfs(const char *, struct statfs *);
#endif
#endif

void set_mirror_uid(char *mirroruser);

#define MAX_LINE		1024

#define GO_CONFIGFILE		0
#define GO_SECTION			1
#define GO_LOCALDIR 		2
#define GO_REMOTEDIR 		3
#define GO_USERNAME 		4
#define GO_PASSWORD 		5
#define GO_HOST 			6
#define GO_VERSION			7
#define GO_WARNOVERRIDES	8
#define GO_SKIP	 			9
#define GO_SHOWCONF			10
#define GO_LOGFILE 			11
#define GO_LOCKFILE			12
#define GO_RETRIES 			13
#define GO_RETRYTIME 		14
#define GO_TIMEOUT			15
#define GO_PORT				16
#define GO_PASSIVE			17
#define GO_MAXDELETE		18
#define GO_FASTSYNC			19
#define GO_LOGLEVEL			20
#define GO_TEMPDIR			21
#define GO_TREATASDIR		22
#define GO_TREATASDIRDEST	23
#define GO_MINFREE			24
#define GO_DODELETE			25
#define GO_MIRRORUSER		26
#define GO_HELP				27
#define GO_URL				28
#define GO_FAMILY			29

static struct get_opt go[] = {
{ GO_CONFIGFILE,		"configfile",		'f',	REQ_ARG },
{ GO_SECTION,			"section",			's',	REQ_ARG },
{ GO_LOCALDIR,	 		"localdir",			'l',	REQ_ARG },
{ GO_REMOTEDIR,			"remotedir",		'r',	REQ_ARG },
{ GO_USERNAME,			"username",			'u',	REQ_ARG },
{ GO_PASSWORD,			"password",			'p',	REQ_ARG },
{ GO_HOST,				"host",				'h',	REQ_ARG },
{ GO_VERSION,			"version",			'v',	OPT_ARG },
{ GO_WARNOVERRIDES,		"warnoverrides",	'w',	REQ_ARG },
{ GO_SKIP,				"skip",				'?',	REQ_ARG },
{ GO_SHOWCONF,			"showconf",			'?',	REQ_ARG },
{ GO_LOGFILE,			"logfile",			'?',	REQ_ARG },
{ GO_LOCKFILE,			"lockfile",			'?',	REQ_ARG },
{ GO_RETRIES,			"retries",			'?',	REQ_ARG },
{ GO_RETRYTIME,			"retrytime",		'?',	REQ_ARG },
{ GO_TIMEOUT,			"timeout",			'?',	REQ_ARG },
{ GO_PORT,				"port",				'?',	REQ_ARG },
{ GO_PASSIVE,			"passive",			'?',	REQ_ARG },
{ GO_MAXDELETE,			"maxdelete",		'?',	REQ_ARG },
#ifdef NOTYET
{ GO_FASTSYNC,			"fastsync",			'?',	REQ_ARG },
#endif
{ GO_LOGLEVEL,			"loglevel",			'?',	REQ_ARG },
{ GO_TEMPDIR,			"tempdir",			'?',	REQ_ARG },
{ GO_TREATASDIR,		"treatasdir",		'?',	REQ_ARG },
{ GO_TREATASDIRDEST,	"treatasdirdest",	'?',	REQ_ARG },
{ GO_MINFREE,			"minfree",			'?',	REQ_ARG },
{ GO_DODELETE,			"dodelete",			'?',	REQ_ARG },
{ GO_MIRRORUSER,		"mirroruser",		'?',	REQ_ARG },
{ GO_HELP,				"help",				'?',	NO_ARG },
{ GO_URL,				"url",				'?',	REQ_ARG },
{ GO_FAMILY,			"family",	'a',	REQ_ARG },
{ -1,NULL,0,NO_ARG}
};

extern char cur_dir[PATH_MAX];

int sp_do_delete;
int sp_deletes;
int sp_dirs_traversed;
int sp_errors;
int sp_max_delete;
unsigned int sp_retrytime;
static FILE *fp_log_file;
static char *lock_file = NULL;
static char *start_wd = NULL;
static struct ftp_con *connection = NULL;
static time_t start_time;

static int nowarn = 0;


extern struct cl_sps_que *skip;
extern struct cl_spt_que *treatasdir;

static void
remove_lock(void)
{
	if (seteuid(getuid()) < 0)
		e_warn("seteuid(%d)", getuid());
	if (start_wd != NULL) {
    	if (chdir(start_wd) < 0) {
			e_warn("chdir(%s)", start_wd);
			free(start_wd);
			return;
		}
		free(start_wd);
	}
	if (lock_file != NULL) {
		if (unlink(lock_file) < 0)
			e_warn("unlink(%s)", lock_file);
	}
}

static void
clean_up_vars(void)
{
	cl_sps_free(skip, sps_unalloc);
	cl_spt_free(treatasdir, spt_unalloc);
	if (connection != NULL)
		ftp_unalloc(connection);
}

static void
print_stats(void)
{
	time_t	tval;

	(void) time(&tval);
	e_warnx("Spegla finished at %s", ctime(&tval));
	e_warnx("dirs traversed        : %d", sp_dirs_traversed);
	e_warnx("files deleted         : %d", sp_deletes);
	if (connection != NULL) {
#ifdef NO_QUADS
		e_warnx("bytes recieved        : %lu", connection->ftp_recd);
		e_warnx("bytes sent            : %lu", connection->ftp_sent);
#else
		e_warnx("bytes recieved        : %llu",
#if defined (NetBSD) && ARCH == Alpha && !defined(lint)
(long long unsigned int) 	/* XXX - impossible to make it in another	*/
							/* way without a warning					*/
#endif
			connection->ftp_recd);
		e_warnx("bytes sent            : %llu",
#if defined (NetBSD) && ARCH == Alpha && !defined(lint)
(long long unsigned int) 	/* XXX - impossible to make it in another	*/
							/* way without a warning					*/
#endif
			connection->ftp_sent);
#endif
		if (tval - start_time != 0) 
			e_warnx("average download rate : %.2f K/sec",
				(float)((float)(connection->ftp_recd / 1024.0) /
					(float)(tval - start_time)) );
		e_warnx("number of relogins    : %d", connection->ftp_relogins);
		e_warnx("files downloaded      : %d", connection->ftp_downloads);
		e_warnx("number of timeouts    : %d", connection->ftp_timeouts);
	}
}

/* ARGSUSED0 */
static void
signal_handler(int n)
{
	n = n;	/* silent gcc */
	exit(2); /* for profiling */
}

static void
setup_signals(void)
{
	struct sigaction act;

	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = signal_handler;
	
	(void) sigaction(SIGQUIT, &act, NULL);
	(void) sigaction(SIGTERM, &act, NULL);
	(void) sigaction(SIGINT, &act, NULL);

	/* don't want to quit if a connection is lost */
	act.sa_handler = SIG_IGN;	
	(void) sigaction(SIGPIPE, &act, NULL);
}

static struct heap_spf_list *
build_remote_dir(FILE * fp)
{
	char    buf[PATH_MAX];
	size_t	len;
	struct	sp_file *spf;
	struct	heap_spf_list *headp;

	if ((fp == NULL) || (fgets(buf, (int)sizeof(buf), fp) == NULL))
		return NULL;

	if ((strncmp(buf + 1, "otal", (size_t)4) == 0) &&
	    (fgets(buf, (int)sizeof(buf), fp) == NULL))
		return NULL;

	headp = heap_spf_init(spf_push_comp);
	do {
		/* remove trailing '\r' and '\n' */
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		len = strlen(buf);
		if (buf[len - 1] == '\r')
			buf[len - 1] = '\0';

		/* short row */
		if (strlen(buf) == 0)
			continue;

		if ((spf = parse_row(buf)) == NULL) {
			e_warnx("build_remote_dir: parse error: \"%s\"", buf);
			continue;
		}

		/* don't want . or .. */
		if (strcmp("..", spf->spf_name) == 0 ||
		    strcmp(".", spf->spf_name) == 0) {
			spf_unalloc(spf);
			continue;
		}

		/* treat_as_dir before skip */
		if (spf_is_a_treatasdir(spf)) {
			spf->spf_type = DIRECTORY;
			spf->spf_opt |= SPFO_ISLINK;
		}

		if (spf_is_a_skip(spf)) {
			spf_unalloc(spf);       
			continue;				
		}

		(void) heap_spf_push(headp, spf);
	} while (fgets(buf, (int)sizeof(buf), fp) != NULL);

	return headp;
}


static struct heap_spf_list *
build_local_dir(void)
{
	DIR		*dirp;
	struct	heap_spf_list *headp;
	struct	sp_file *spf;
	struct	dirent *dp;

	if ((dirp = opendir(cur_dir)) == NULL)
		return NULL;

	headp = heap_spf_init(spf_push_comp);
	while ((dp = readdir(dirp)) != NULL) {

		/* don't want . or .. */
		if (strcmp("..", dp->d_name) == 0 || strcmp(".", dp->d_name) == 0)
			continue;

		/* create our spf */
		if (spf_stat_init(dp->d_name, &spf) < 0)
			continue;

		/* treat_as_dir before skip */
		if (spf_is_a_treatasdir(spf)) {
			spf->spf_type = DIRECTORY;
			spf->spf_opt |= SPFO_ISLINK;
		}

		if (spf_is_a_skip(spf)) {
			spf_unalloc(spf);
			continue;
		} 

		(void) heap_spf_push(headp, spf);
	}

	(void) closedir(dirp);
	return headp;
}

static void
get_ftp_file(struct ftp_con * c, struct sp_file *spf)
{
	char	remote_file[PATH_MAX], local_file[PATH_MAX];
	struct	stat sb;
	size_t	size = 0;
	int		res, rmed_orig = 0, erased = 0;

	if (spf_full_name2(spf, remote_file, sizeof(remote_file)) == NULL)
		return;
	if (spf_use_tmp_name(spf) == NULL)
		return;
	if (spf_full_name2(spf, local_file, sizeof(local_file)) == NULL)
		return;

restart:
	if (size == 0)
		e_warnx("get %s", remote_file);
	else
		e_warnx("continues at byte %ld in %s", (unsigned long)size,
			remote_file);
		
	res = ftp_get(c, local_file, remote_file, size);
	if (res < 0) {
		if (c->ftp_resp != JFTP_BROKEN || c->ftp_resp != JFTP_TIMEOUT)
			e_warnx("get %s: failed", remote_file);
		switch (c->ftp_resp) {
		case JFTP_BROKEN:
		case JFTP_TIMEOUT:
			do {
				if (c->ftp_retries-- < 0) {
					(void)spf_rm(spf);
					(void)spf_restore_name(spf);
					e_errx(1, "I've retried enough now, quit");
				}

				e_warnx("%s on %s, win quit in %d retries",
					c->ftp_resp == JFTP_BROKEN ? "broken connection":"timeout",
				    remote_file, c->ftp_retries);

				if (!erased) {
					if (stat(local_file, &sb) < 0)
						size = 0;
					else
						size = (size_t)sb.st_size;
					if (size == 0) {
						(void)spf_rm(spf);
						erased = 1;
					}
				}
				e_warnx("sleeping for %d seconds...", sp_retrytime);
				(void) sleep(sp_retrytime);
				e_warnx("trying to login again");
				res = ftp_relogin(c);
				if (res < 0)
					e_warnx("relogin failed code %d, will quit in %d retries",
						c->ftp_resp, c->ftp_retries);
			} while (res < 0);
			goto restart;
		case JFTP_ERR:
			(void)spf_rm(spf);
			(void)spf_restore_name(spf);
			return;
		case JFTP_WRITEERR:
			if (!rmed_orig) {
				int		fd;

				e_warnx("file system full, "
					"trucates and removes original file (%s) and retries",
					remote_file);
				/* truncate and remove the original file
				 * to free some space.
				 *
				 * XXX - remote_file is the name of the original file
				 */
				if ((fd = open(remote_file, O_TRUNC)) > -1)
					close(fd);
				unlink(remote_file);
				rmed_orig = 1;
				goto restart;
			}
			e_errx(1, "File system full!");
			/* FALLTHROUGH */ /* not really e_errx exits */
		default:
			e_errx(1, "ERROR unexpected error %d fel %s",
			    c->ftp_resp, spf_full_name(spf));
		}
	}
	(void)spf_chmod(spf, 0);
	(void)spf_new_time(spf);
	(void)spf_restore_name(spf);
}

static void
sp_get_file(struct ftp_con *c, struct sp_file *spf, struct fifo_spf_list *fifo)
{
	switch (spf->spf_type) {
		case DIRECTORY:
		if ((spf = spf_clone(spf)) == NULL)
			e_err(1, "spf_clone");
		(void) fifo_spf_push(fifo, spf_clone(spf));
		return;
	case PLAINFILE:
		get_ftp_file(c, spf);
		return;
	case SYMLINK:
		(void) spf_symlink(spf);
		return;
	default:
		e_warnx("%s unknown file", spf_full_name(spf));
	}
}

#if 0
void
dump_spf_heap(struct heap_spf_list *h)
{
	struct	heap_spf_list *hc;
	struct	sp_file *spf;
	int		i;

	if ((hc = heap_spf_clone(h)) == NULL)
		return;

	printf("*** heap_data dump ***\n");
	for (i = 0; i < hc->heap_keys; i++)
		fprintf(stderr, "name = '%s'\n", hc->heap_data[i]->spf_name);
	
	printf("*** picked out in order ***\n");
	while (heap_spf_pop(hc, &spf) == 0)
		fprintf(stderr, "name = '%s'\n", spf->spf_name);
	heap_spf_free(hc, NULL);
}

void
dump_spf_fifo(struct fifo_spf_list *f)
{
	struct	fifo_spf_list *fc;
	struct	sp_file *spf;

	if ((fc = fifo_spf_clone(f)) == NULL)
		return;

	printf("*** fifo_data dump ***\n");
	while (fifo_spf_pop(fc, &spf) == 0)
		fprintf(stderr, "name = '%s'\n", spf->spf_name);
	fifo_spf_free(fc, NULL);
}
#endif


static void
sp_sync_dir(struct ftp_con *c, struct heap_spf_list *l, struct heap_spf_list *r)
{
	char	*temp_file;
	int		cmp, res;
	FILE	*fp;
	struct	fifo_spf_list *dirs;
	struct	sp_file *spf, *spf_r, *spf_l;

	if (l == NULL) {
		e_warnx("sp_sync_dir: Reading of local dir: failed");
		return;
	}
	if (r == NULL) {
		e_warnx("sp_sync_dir: Reading of remote dir: failed");
		return;
	}
	sp_dirs_traversed++;
	dirs = fifo_spf_init();
	spf = NULL;
	while (heap_spf_peek(l, NULL) == 0 || heap_spf_peek(r, NULL) == 0) {
		/****** There are no more files locally ******/
		if (heap_spf_peek(l, NULL) < 0) {
			(void) heap_spf_pop(r, &spf);
			sp_get_file(c, spf, dirs);
			spf_unalloc(spf);
			continue;
		}
		/****** There are no more files on the server ******/
		if (heap_spf_peek(r, NULL) < 0) {
			(void) heap_spf_pop(l, &spf);
			(void) spf_rm(spf);
			spf_unalloc(spf);
			continue;
		}
		(void) heap_spf_peek(r, &spf_r);
		(void) heap_spf_peek(l, &spf_l);
		cmp = spf_name_cmp(spf_l, spf_r);
		/****** A file is missing locally ******/
		if (cmp > 0) {
			(void) heap_spf_pop(r, &spf);
			assert(spf != NULL);
			sp_get_file(c, spf, dirs);
			spf_unalloc(spf);
			continue;
		}
		/****** Exceeding local file******/
		if (cmp < 0) {
			(void) heap_spf_pop(l, &spf);
			assert(spf != NULL);
			(void) spf_rm(spf);
			spf_unalloc(spf);
			continue;
		}
		/****** Differing file types ******/
		if (spf_r->spf_type != spf_l->spf_type) {
			/* Remove local file */
			(void) heap_spf_pop(l, &spf);
			(void) spf_rm(spf);
			spf_unalloc(spf);

			/* Download remote file */
			(void) heap_spf_pop(r, &spf);
			sp_get_file(c, spf, dirs);
			(void) spf_unalloc(spf);
			continue;
		}
		/****** The files are of the same type now ******/
		switch (spf_r->spf_type) {
		case DIRECTORY:
			/* Put directory in directory queue for later analyze */
			spf_r->spf_opt |= SPFO_DONT_CREATE;
			sp_get_file(c, spf_r, dirs);
			break;
		case PLAINFILE:
			if ((spf_time_cmp(spf_r, spf_l) != 0) ||
			    (spf_r->spf_size != spf_l->spf_size)) {
#if 0
				/* should not be needed any longer */
				(void) spf_rm(spf_l);
#endif
				sp_get_file(c, spf_r, dirs);
				break;
			}
			if (spf_r->spf_mode != spf_l->spf_mode) {
				spf_r->spf_opt |= SPFO_LOG_CHMOD;
				(void) spf_chmod(spf_r, 0);
			}
			break;
		case SYMLINK:
			if (strcmp(spf_r->spf_symlink, spf_l->spf_symlink) != 0) {
				if (spf_rm(spf_r) >= 0)
					sp_get_file(c, spf_r, dirs);
			} 
			break;
		default:
			e_warnx("sp_sync_dir: %s unknown file", spf_full_name(spf));
			continue;
		}

		/* Pop file of queue, can't do any more for this file */
		(void) heap_spf_pop(l, &spf);
		spf_unalloc(spf);
		(void) heap_spf_pop(r, &spf);
		spf_unalloc(spf);
	}
	heap_spf_free(l, spf_unalloc);
	heap_spf_free(r, spf_unalloc);

	while (fifo_spf_pop(dirs, &spf) == 0) {
		if (spf_is_a_skip(spf)) {
			e_warnx("does this happen? skipping directory %s",
				spf_full_name(spf));
			spf_unalloc(spf);
			continue;
		}
		if (spf_mkdir(spf) < 0) {
			/* If the directory already existed, no problem */
			if (errno != EEXIST)
				continue;
		}
		/* make sure we can descend, will restore file mode later */
		if (spf_chmod(spf, S_IRWXU) < 0)
			continue;
		(void) cur_dir_cd(spf->spf_name);

		/** one step down in the hierarchy **/
		l = build_local_dir();
		do {
			e_warnx("list %s", cur_dir);
			temp_file = ftp_dir(c, cur_dir);
			if (temp_file == NULL) {
				switch (c->ftp_resp) {
				case JFTP_BROKEN:
				case JFTP_TIMEOUT:
					do {
						if (c->ftp_retries-- < 0) 
							e_errx(1, "retried to much, bye %s", cur_dir);
						if (c->ftp_resp == JFTP_TIMEOUT) {
							e_warnx("timeout on %s, retries = %d",
							    cur_dir, c->ftp_retries);
						} else
							if (c->ftp_resp == JFTP_BROKEN) {
								e_warnx("broken connection on %s, "
									"retries = %d", cur_dir, c->ftp_retries);
							}
						e_warnx("sleeping for %d seconds...", sp_retrytime);
						(void) sleep(sp_retrytime);
						e_warnx("trying to login again");
						res = ftp_relogin(c);
						if (res < 0)
							e_warnx("relogin failed, code %d", c->ftp_resp);
					} while (res < 0);
					break;
				case JFTP_ERR:
					break;
				default:
					e_errx(1, "ERROR: unexpected error dir = %s", cur_dir);
				}
			}
			if (temp_file == NULL && c->ftp_resp == JFTP_ERR)
				break;
		} while (temp_file == NULL);
		if (temp_file != NULL) {
			fp = fopen(temp_file, "r");
			r = build_remote_dir(fp);
			(void) fclose(fp);
		} else
			r = NULL;
		(void) unlink(temp_file);
		free(temp_file);
		if (l == NULL)
			e_warnx("reading of local dir '%s': failed", cur_dir);
		if (r == NULL)
			e_warnx("reading of remote dir '%s': failed", cur_dir);
		if (r != NULL && l != NULL)
			sp_sync_dir(c, l, r);

		/** one step up **/
		(void) cur_dir_cdup();
		(void) spf_new_time(spf);
		(void) spf_chmod(spf, 0);
		spf_unalloc(spf);
	}
	fifo_spf_free(dirs, spf_unalloc);
}

static void
check_minfree(int minfree, char *path)
{
#ifdef ULTRIX
	struct	fs_data f;
#else
#ifdef Solaris
	struct	statvfs f;
#else
	struct	statfs f;
#endif
#endif

	if (minfree == 0)
		return;

#ifdef Solaris
	if (statvfs(path, &f) < 0)
		e_err(1, "statvfs: %s", path);
#else
	if (statfs(path, &f) < 0)
		e_err(1, "statfs: %s", path);
#endif
#ifdef ULTRIX
	if ((f.fd_req.bfreen * 1024) < minfree)
#else
	if (((long long)f.f_bsize * f.f_bavail) < minfree)
#endif
		e_errx(1, "%s: Not enough space left", path);

}

void
set_mirror_uid(char *mirroruser)
{
	struct passwd *pw;

	if (mirroruser == NULL)
		return;
	if ((pw = getpwnam(mirroruser)) == NULL)
		e_err(1, "getpwnam(%s)", mirroruser);
	if (seteuid(pw->pw_uid) < 0)
		e_err(1, "seteuid(%d)", pw->pw_uid);
}

static void
set_param_str(int option, const char *arg, char **var)
{
	if (*var != NULL) {
		if (!nowarn)
			e_warnx("overriding %s's valule '%s' with '%s'",
				go[option].go_name, *var, arg);
		free(*var);
	}
	*var = strdup(arg);
}

static void
set_param_yes_no(int option, const char *arg, int **var)
{
	int		tmp;

	tmp = 0; /* Silent gcc */
	if (strlen(arg) == 0 || strcmp(arg, "yes") == 0)
		tmp = 1;
	else if (strcmp(arg, "no") == 0)
		tmp = 0;
	else
		e_errx(1, "%s's argument '%s' invalid", go[option].go_name, arg);

	if (*var != NULL) {
		if (!nowarn)
			e_warnx("overriding %s's value '%s' with '%s'",
				go[option].go_name, **var ? "yes" : "no", arg);
	} else {
		*var = malloc(sizeof(tmp));
		if (*var == NULL)
			e_err(1, "malloc");
	}
		
	**var = tmp;
}

static void
set_param_num(int option, const char *arg, int **var, int min_val, int max_val)
{
	int		tmp;
	char	*endptr;

	tmp = (int) strtol(arg, &endptr, 10);
	if (*endptr != '\0' || tmp > max_val || tmp < min_val)
		e_errx(1, "%s's argument '%s' invalid", go[option].go_name, arg);

	if (*var != NULL) {
		if (!nowarn)
			e_warnx("overriding %s's value '%d' with '%s'",
				go[option].go_name, **var, arg);
	} else {
		*var = malloc(sizeof(tmp));
		if (*var == NULL)
			e_err(1, "malloc");
	}
		
	**var = tmp;
}

/*
 * Parses an url of the format: ftp://[user[:pass]@]host[:port]/path
 */
static void
set_param_url(int option, const char *arg,
	char **user, char **passwd, char **host, int **port, char **path)
{
	char *url, *p, *puser, *ppass, *phost, *pport, *ppath;

	puser = NULL;
	ppass = NULL;
	phost = NULL;
	pport = NULL;
	ppath = NULL;

	if ((url = strdup(arg)) == NULL)
		err(1, "strdup");
	if (strncasecmp(url, "ftp://", 6) != 0)
		errx(1, "supports only ftp URL's");
	url += 6;

	if (*url == '\0')
		errx(1, "short url: %s", arg);

	/* Parse eventual user:pass field */
	if ((p = strchr(url, '@')) != NULL) {
		puser = url;
		url = p + 1;			/* advance url to host */
		*p = '\0';				/* NULL terminate user[:pass] field */
		if ((p = strchr(puser, ':')) != NULL) {
			ppass = p + 1;
			*p = '\0';			/* NULL terminate user field */
			set_param_str(GO_PASSWORD, ppass, passwd);
		}
		if (*url == '\0')
			errx(1, "short url: %s", arg);
		set_param_str(GO_USERNAME, puser, user);
	}

	/* Parse host + eventual port field */
	if ((p = strchr(url, '/')) == NULL)
		errx(1, "short url: %s", arg);
	phost = url;
	url = p + 1;
	*p = '\0';					/* NULL terminate host[:port] field */
	if ((p = strchr(phost, ':')) != NULL) {
		pport = p + 1;
		*p = '\0';				/* NULL terminate host field */
		set_param_num(GO_PORT, pport, port, 0, INT_MAX);
	}
	set_param_str(GO_HOST, phost, host);

	/* Parse path field */
	if (*url != '\0') {
		ppath = url - 1;
		*ppath = '/';
		set_param_str(GO_REMOTEDIR, ppath, path);
	}
}

static void
set_param_family(int option, const char *arg, int **var)
{
	int	family;
	char	*f;

	family = PF_UNSPEC;	/* Silent gcc */
	if (strlen(arg) == 0 || strcmp(arg, "unspec") == 0)
		family = PF_UNSPEC;
	else if (strcmp(arg, "inet") == 0)
		family = PF_INET;
#ifdef INET6
	else if (strcmp(arg, "inet6") == 0)
		family = PF_INET6;
#endif
	else
		e_errx(1, "%s's argument '%s' invalid", go[option].go_name, arg);

	if (*var != NULL) {
		if (!nowarn) {
			switch (**var) {
			case PF_UNSPEC:
				f = "unspec";
				break;
			case PF_INET:
				f = "inet";
				break;
#ifdef INET6
			case PF_INET6:
				f = "inet6";
				break;
#endif
			default:
				f = "unknown";
				break;
			}
			e_warnx("overriding %s's value '%s' with '%s'",
				go[option].go_name, f, arg);
		}
	} else {
		*var = malloc(sizeof(family));
		if (*var == NULL)
			e_err(1, "malloc");
	}
		
	**var = family;
}

/* ARGSUSED */
static void
add_param_sps(int option, const char *arg, struct cl_sps_que **q)
{
	struct	sp_skip *sps;

	option = 0; /* quiet gcc */
	if (*q == NULL && ((*q = cl_sps_init()) == NULL))
		e_err(1, "cl_sps_init");
	if ((sps = sps_init(arg)) == NULL)
		e_err(1, "sps_init");
	if (sps_error(sps))
		e_errx(1, "sps_init: %s", sps_strerror(sps));
	(void) cl_sps_push(*q, sps);
}

/* ARGSUSED */
static void
add_param_spt(int option, const char *arg, struct cl_spt_que **q)
{
	struct	sp_treatasdir *spt;

	if (*q == NULL && ((*q = cl_spt_init()) == NULL))
		e_err(1, "cl_spt_init");
	if ((spt = spt_init(arg)) == NULL)
		e_err(1, "spt_init");
	if (spt_error(spt))
		e_errx(1, "spt_init: %s", spt_strerror(spt));
	spt->spt_deref = (go[option].go_opt_num == GO_TREATASDIRDEST);
	(void)cl_spt_push(*q, spt);
}

static void
require(const char *name, char *str, int show)
{
	if (str == NULL)
		e_errx(1, "%s required", name);
	if (show)
		e_warnx("%s = '%s'", name, str);
}

static void
default_str(const char *name, char **val, const char *str, int show)
{
	if (*val == NULL) {
		*val = strdup(str);
		if (*val == NULL)
			e_err(1, "strdup");
	}
	if (show)
		e_warnx("%s = '%s'", name, *val);
}

static void
expand_with_str(const char *name, char **val, const char *str, int show)
{
	char	*buf, *p;
	int		i, j;

	p = *val;
	if (p == NULL)
		return;
	if (strchr(p, '%') == NULL)
		goto ret;
	if ((buf = malloc((size_t)MAX_LINE)) == NULL)
		e_err(1, "malloc");
	for (i = j = 0; i < MAX_LINE && p[j] != '\0'; i++, j++) {
		if (p[j] == '%') {
			switch (p[j + 1]) {
			case '%':
				j++;
				buf[i] = '%';
				continue;
			case 's':
				j++;
				/* if buf gets full we will break the for-loop anyway */
				i += (unsigned)strlcpy(buf + i, str, (size_t)(MAX_LINE - i))
					- 1;
				continue;
			default:
				break;
			}
		}
		buf[i] = p[j];
	}
	buf[i] = '\0';
	*val = buf;
ret:
	if (show)
		e_warnx("%s = '%s'", name, *val);
}

static void
default_num(const char *name, int **val, int num, int show)
{
	if (*val == NULL) {
		*val = malloc(sizeof(int));
		if (*val == NULL)
			e_err(1, "malloc");
		**val = num;
	}
	if (show)
		e_warnx("%s = %d", name, **val);
}

static void
default_family(const char *name, int **val, int num, int show)
{
	char *f;

	if (*val == NULL) {
		*val = malloc(sizeof(int));
		if (*val == NULL)
			e_err(1, "malloc");
		**val = num;
	}
	if (show) {
		switch (**val) {
		case PF_UNSPEC:
			f = "unspec";
			break;
		case PF_INET:
			f = "inet";
			break;
#ifdef INET6
		case PF_INET6:
			f = "inet6";
			break;
#endif
		default:
			f = "unknown";
			break;
		}
		e_warnx("%s = %s", name, f);
	}
}

static void
show_skip_map_fun(struct sp_skip *sps)
{
	e_warnx("skip = '%s'", sps->sps_name);
}

static void
show_treatasdir_map_fun(struct sp_treatasdir *spt)
{
	e_warnx("treatasdir%s = '%s'",
		spt->spt_deref ? "dest" : "", spt->spt_name);
}

static void
usage(void)
{
	e_errx(1,
"usage: spegla -f <config file>\n"
"lots of other options are also supported\n"
"please consult man page\n");
}

int
main(int argc, char * const *argv)
{
	FILE	*fp;
	char	*temp_file, anonpass[MAX_LINE];
	const	char *arg, *p;
	int		option;
	int		fd_lock;
	int		read_from_file;
	struct	stat sb;
	struct	ftp_con *c;
	struct	heap_spf_list *l, *r;
	struct	get_opt_state *gos_argv, *gos_file;
	struct {
		char	*host;
		char	*localdir;
		char	*remotedir;
		char	*username;
		char	*password;
		char	*logfile;
		char	*lockfile;
		char	*configfile;
		char	*tempdir;
		char	*version;
		char	*section;
		char	*mirroruser;
		int		*timeout;
		int		*retries;
		int		*port;
		int		*family;
		int		*passive;
		int		*retrytime;
		int		*showconf;
		int		*fastsync;
		int		*maxdelete;
		int		*loglevel;
		int		*warnoverrides;
		int		*minfree;
		int		*dodelete;
		struct	cl_sps_que *skip;
		struct	cl_spt_que *treatasdir;
	} param;

	(void) memset(&param, 0, sizeof(param));
	param.tempdir = getenv("TEMPDIR");

	/* If malloc fails exit */
	container_err_exit = 1;

	read_from_file = 0;

	setup_signals();
	if (argc < 2)
		usage();

	sp_deletes = sp_dirs_traversed = sp_errors = 0;

	gos_argv = tgetopt_argv_init(argc, argv, go);
	if (gos_argv == NULL)
		e_err(1, "tgetopt_argv_init");
		
	gos_file = NULL;
	fp = NULL;
	for ( ;; ) {
		/*
		 * if there is a file to read from, read from it
		 * otherwise read from command line arguments
		 */
		if (gos_file != NULL) {
			if (tgetopt(gos_file, &option, &arg) < 0) {
				if (tgetopt_errno(gos_file) == GOS_END_OF_OPTIONS) {
					tgetopt_free(gos_file);
					gos_file = NULL;
					(void) fclose(fp);
					fp = NULL;
					continue;
				}
				e_errx(1, tgetopt_strerror(gos_file));
			}
		} else if (tgetopt(gos_argv, &option, &arg) < 0) {
			if (tgetopt_errno(gos_argv) == GOS_END_OF_OPTIONS)
				break;
			e_errx(1, tgetopt_strerror(gos_argv));
			tgetopt_free(gos_argv);
		}

		switch(go[option].go_opt_num) {
		case GO_FAMILY:
			set_param_family(option, arg, &param.family);
			break;
		case GO_LOCALDIR:
			set_param_str(option, arg, &param.localdir);
			break;
		case GO_REMOTEDIR:
			set_param_str(option, arg, &param.remotedir);
			break;
		case GO_USERNAME:
			set_param_str(option, arg, &param.username);
			break;
		case GO_PASSWORD:
			set_param_str(option, arg, &param.password);
			break;
		case GO_HOST:
			set_param_str(option, arg, &param.host);
			break;
		case GO_RETRIES:
			set_param_num(option, arg, &param.retries, 0, INT_MAX);
			break;
		case GO_RETRYTIME:
			set_param_num(option, arg, &param.retrytime, 0, INT_MAX);
			break;
		case GO_TIMEOUT:
			set_param_num(option, arg, &param.timeout, 0, INT_MAX);
			break;
		case GO_PORT:
			set_param_num(option, arg, &param.port, 0, INT_MAX);
			break;
		case GO_MINFREE:
			set_param_num(option, arg, &param.minfree, 0, INT_MAX);
			break;
		case GO_PASSIVE:
			set_param_yes_no(option, arg, &param.passive);
			break;
		case GO_SKIP:
			add_param_sps(option, arg, &param.skip);
			break;
		case GO_TREATASDIR:
			add_param_spt(option, arg, &param.treatasdir);
			break;
		case GO_TREATASDIRDEST:
			add_param_spt(option, arg, &param.treatasdir);
			break;
		case GO_LOGFILE:
			set_param_str(option, arg, &param.logfile);
			break;
		case GO_LOCKFILE:
			set_param_str(option, arg, &param.lockfile);
			break;
		case GO_MIRRORUSER:
			set_param_str(option, arg, &param.mirroruser);
			break;
		case GO_MAXDELETE:
			set_param_num(option, arg, &param.maxdelete, 0, INT_MAX);
			break;
		case GO_LOGLEVEL:
			set_param_num(option, arg, &param.loglevel, 0, INT_MAX);
			break;
		case GO_SHOWCONF:
			set_param_yes_no(option, arg, &param.showconf);
			break;
		case GO_FASTSYNC:
			set_param_yes_no(option, arg, &param.fastsync);
			break;
		case GO_DODELETE:
			set_param_yes_no(option, arg, &param.dodelete);
			break;
		case GO_WARNOVERRIDES:
			set_param_yes_no(option, arg, &param.warnoverrides);
			nowarn = !*param.warnoverrides;
			break;
		case GO_TEMPDIR:
			set_param_str(option, arg, &param.tempdir);
			break;
		case GO_HELP:
			usage();
			break;
		case GO_URL:
			set_param_url(option, arg, &param.username, &param.password,
				&param.host, &param.port, &param.remotedir);
			default_str("localdir", &param.localdir, ".", 0);
			break;
		case GO_VERSION:
			if (arg == NULL)
				e_errx(0, "Spegla version" SPEGLA_VERSION);
			set_param_str(option, arg, &param.version);
			if (strcmp(param.version, SPEGLA_VERSION) != 0 &&
				strcmp(param.version, "1.1") != 0)
				e_errx(1, "expected version " SPEGLA_VERSION
					", got %s", param.version);
			break;
		case GO_SECTION:
			/* don't allow sections to be choosen in config file */
			if (fp != NULL)
				e_errx(1, "can't select a new section in config file");
			set_param_str(option, arg, &param.section);
			break;
		case GO_CONFIGFILE:
			/*
			 * Switch to config file imediatly and continue
			 * with the other command line options later,
			 * don't allow config files specified in a config file.
			 */
			read_from_file = 1;
			if (fp != NULL)
				e_errx(1, "already reading from a config file");
			if (lstat(arg, &sb) < 0)
				e_err(1, "stat");
			if (!S_ISREG(sb.st_mode))
				e_errx(1, "%s is not a regular file", arg);
			if ((fp = fopen(arg, "r")) == NULL)
				e_err(1, "fopen(%s, \"r\")", arg);
			if ((gos_file = tgetopt_file_init(fp, param.section, go)) == NULL)
				e_err(1, "tgetopt_file_init");
			break;
		default:
			e_errx(1, "unknown option number %d", option);
		}
	}


	e_set_progname(NULL);

	if ((p = getenv("FTPANONPASS")) != NULL) {
		strlcpy(anonpass, p, sizeof(anonpass));
	} else {
		if ((p = getenv("USER")) == NULL)
			p = "default";
		snprintf(anonpass, sizeof(anonpass), "%s@", p);
	}

	/* check configed variables and show them if asked for */
	default_num("showconf",	&param.showconf,	1, 0);

	/* Only require the version option if a file has been read from. */
	if (read_from_file)
		require("version",	param.version,	*param.showconf);
	require("host",		param.host,		*param.showconf);
	require("localdir",	param.localdir,	*param.showconf);
	require("remotedir",param.remotedir,*param.showconf);

	default_str("username",&param.username,		"anonymous",*param.showconf);
	default_str("password",	&param.password,	anonpass,	*param.showconf);
	default_str("section",	&param.section,		"",			*param.showconf);
	default_str("version",	&param.version,		"not defined",*param.showconf);
	default_str("tempdir",	&param.tempdir,		"/tmp",		*param.showconf);

	default_num("timeout",	&param.timeout,		150,*param.showconf);
	default_num("retries",	&param.retries,		20,	*param.showconf);
	default_num("port",		&param.port,		21,	*param.showconf);
	default_family("family",	&param.family,		PF_UNSPEC,	*param.showconf);
	default_num("passive",	&param.passive,		1,	*param.showconf);
	default_num("retrytime",&param.retrytime,	150,*param.showconf);
	default_num("fastsync",	&param.fastsync,	0,	*param.showconf);
	default_num("dodelete",	&param.dodelete,	0,	*param.showconf);
	default_num("maxdelete",&param.maxdelete,	150,*param.showconf);
	default_num("showconf",	&param.showconf,	1,	*param.showconf);
	default_num("loglevel",	&param.loglevel,	0,	*param.showconf);
	default_num("warnoverrides",&param.warnoverrides,1,*param.showconf);
	default_num("minfree",	&param.minfree,	0,	*param.showconf);

	if (*param.showconf) {
			e_warnx("logfile = '%s'",
				param.logfile == NULL ? "stdout" : param.logfile);
			if (param.skip)
				cl_sps_map(param.skip, show_skip_map_fun, NULL);
			if (param.treatasdir)
				cl_spt_map(param.treatasdir, show_treatasdir_map_fun, NULL);
	}

	expand_with_str("logfile", &param.logfile, param.section,
		*param.showconf);
	expand_with_str("lockfile", &param.lockfile, param.section,
		*param.showconf);


	skip = param.skip;
	treatasdir = param.treatasdir;
	sp_max_delete = *param.maxdelete;
	sp_retrytime = *param.retrytime;
	sp_do_delete = *param.dodelete;

/**** End of configuration analysis ****/

	check_minfree(*param.minfree, param.localdir);

	if (param.logfile == NULL || strcmp(param.logfile, "stdout") == 0)
		fp_log_file = stdout;
	else if (strcmp(param.logfile, "stderr") == 0)
		fp_log_file = stderr;
	else {
		if ((fp_log_file = fopen(param.logfile, "a")) == NULL)
			e_err(1, "can't open %s for writing", param.logfile);
	}

	(void)atexit(clean_up_vars);
	if (param.lockfile != NULL) {
		if ((fd_lock = open(param.lockfile, O_CREAT | O_EXCL,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0 ) {

			e_err(1, "can't create lock file %s", param.lockfile);
		}
		if (atexit(remove_lock) < 0)
			e_warn("warning: atexit(3) failed lock file, %s, "
				"will not be removed on exit", param.lockfile);
			lock_file = param.lockfile;
			(void) close(fd_lock);
	}
	if (atexit(print_stats) < 0)
		e_warn("warning: atexit(3) failed, no stats will be printed on exit");


	set_mirror_uid(param.mirroruser);

	if ((start_wd = malloc((size_t)PATH_MAX)) == NULL)
		e_err(1, "malloc");
	(void) getcwd(start_wd, (size_t)PATH_MAX);
	(void) strcpy(cur_dir, "./");
	if (chdir(param.localdir))
		e_err(1, "spegla: can't cd to %s", param.localdir);
	(void) time(&start_time);
	e_set_file(fp_log_file);
	e_warnx("Spegla started at %s", ctime(&start_time));
	e_warnx("Logging in to %s", param.host);
	do {
		c = ftp_login(param.host, *param.port, *param.family,
				param.username, param.password,
				fp_log_file, *param.loglevel);
		if (c == NULL) {
			if ((*param.retries)-- < 0)
				e_errx(1, "I've retried enough now, quit");
			e_warnx("Login failed");
			e_warnx("sleeping for %d seconds...", sp_retrytime);
			(void) sleep(sp_retrytime);
			e_warnx("trying to login again");
		}
	} while (c == NULL);
	connection = c;
	ftp_set_passive(c, *param.passive);
	ftp_set_tempdir(c, param.tempdir);
	if (ftp_req(c, "TYPE I") < 0) 
		e_err(1, "couldn't set type to bin, quit");
	ftp_set_timeout_val(c, *param.timeout);
	c->ftp_retries = *param.retries;
	e_warnx("cd %s", param.remotedir);
	if (ftp_cd(c, param.remotedir) < 0) 
		e_errx(1, "cd '%s': failed, abort", param.remotedir);
	e_warnx(". is now '%s'", param.remotedir);
	e_warnx("list .");
	temp_file = ftp_dir(c, ".");
	if (temp_file == NULL) 
		e_errx(1, "list of .: failed, abort");
#if 0
#define SP_FREE(var) if (param.var) free(param.var)
	SP_FREE(host);
	SP_FREE(localdir);
	SP_FREE(remotedir);
	SP_FREE(username);
	SP_FREE(password);
	SP_FREE(logfile);
	SP_FREE(lockfile);
	SP_FREE(configfile);
	SP_FREE(tempdir);
	SP_FREE(version);
	SP_FREE(section);
	SP_FREE(timeout);
	SP_FREE(retries);
	SP_FREE(port);
	SP_FREE(family);
	SP_FREE(passive);
	SP_FREE(retrytime);
	SP_FREE(showconf);
	SP_FREE(fastsync);
	SP_FREE(maxdelete);
	SP_FREE(loglevel);
	SP_FREE(warnoverrides);
	SP_FREE(minfree);
#endif
	fp = fopen(temp_file, "r");
	r = build_remote_dir(fp);
	(void) fclose(fp);
	(void) unlink(temp_file);
	free(temp_file);
	l = build_local_dir();
	sp_sync_dir(c, l, r);
	(void) chdir(start_wd);
	(void) ftp_bye(c);
	exit(0);
	/* NOTREACHED */
}
