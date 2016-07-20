/*$Id: spf_util.c,v 1.34 2000/05/27 13:47:43 jens Exp $*/
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

#ifdef MEMDEBUG
#include <memdebug.h>
#endif

#if defined(SunOS)
#define NO_FNMATCH
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>

#ifdef NO_STRLCPY
#include "strlcpy.h"
#endif

#include "e_err.h"
#include "spegla.h"
#include "spf_util.h"
#include "parserow.h"

#ifdef NOPROTOS
int snprintf(char *str, size_t size, const char *format, ...);
char *strdup(char *);
int utimes(const char *path, const struct timeval *times);
int lstat(const char *path, struct stat *sb);
int symlink(const char *name1, const char *name2);
int readlink(const char *path, char *buf, size_t bufsiz);
#endif

char cur_dir[PATH_MAX] = ".";
static char cur_file[PATH_MAX] = "";

extern int sp_do_delete;
extern int sp_max_delete;
extern int sp_deletes;
extern int sp_dirs_traversed;
extern int sp_errors;
extern unsigned int sp_retrytime;


/* the data structure containing all files to skip */
struct cl_sps_que *skip;

/* the data structure containing all links to treat as directories */
struct cl_spt_que *treatasdir;

/*
 * have_dotdot_ref()
 *
 * Used by cur_dir_cd() to determine if f might result in a change
 * to directory above current directory.
 * Return 1 if the result might result in a change to a direcotry
 * above current directory.
 */
static int
have_dotdot_ref(char *f)
{
	if (strstr(f, "/../") != NULL)
		return 1;
	return 0;
}

/*
 * cur_dir_cd()
 *
 * Changes to a directory under cur_dir. Changes with a path containing
 * ".." is not allowed.
 * Returns the new current directory.
 */
char *
cur_dir_cd(char *str)
{
	size_t	len;

	/* we don't want to go over the dir we start from */
	if (have_dotdot_ref(str))	
		return NULL;

	if ((strlen(str) + strlen(cur_dir) + 1) > (PATH_MAX - 2))
		return NULL;				/* no overflow */
	len = strlen(cur_dir);
	if (len > 0 && cur_dir[len - 1] != '/')
		(void) strcat(cur_dir, "/");
	(void) strcat(cur_dir, str);
	(void) strcat(cur_dir, "/");
	return cur_dir;
}

/*
 * cur_dir_cdup()
 *
 * Changes to the directory above current directory in cur_dir.
 * Doesn't go above start directory. Tries to keep a '/' as last
 * characher in  cur_dir.
 */
char *
cur_dir_cdup(void)
{
	size_t	len;
	int		i;

	len = strlen(cur_dir);

	/* if cur_dir is empty or only containing a single '.' just return */
	if (len == 0 || (len == 1 && cur_dir[0] == '.'))
		return cur_dir;

	/* if cur_dir ends with a '/' strip it off */
	if (cur_dir[len - 1] == '/') {
		cur_dir[len - 1] = '\0';
		len--;
	}

	/* find last '/' in cur_dir */
	for (i = (int)len; (i > 0) && (cur_dir[i] != '/'); i--);

	/* terminate the string just behind the '/' */
	cur_dir[i + 1] = '\0';

	return cur_dir;
}

/*
 * full_name()
 *
 * Returns the path name of f relative to start dir.
 * A buffer is needed to hold the returned path in.
 * Used by spf_full_name() and spf_full_name2().
 */
static char *
full_name(char *f, char *buf, size_t len)
{
	/* we don't want to go over the dir we start from */
	if (have_dotdot_ref(f))
		return NULL;

	/* no overflow */
	if ((strlen(f) + strlen(cur_dir)) > (len - 2))
		return NULL;

	(void) sprintf(buf, "%s%s", cur_dir, f);
	return buf;
}

/*
 * spf_full_name()
 *
 * Returns the path name of spf relative to start dir.
 * cur_file is used as buffer to return.
 */
char *
spf_full_name(struct sp_file *spf)
{
	return spf_full_name2(spf, cur_file, (size_t)PATH_MAX);
}

/*
 * spf_full_name2()
 *
 * Returns the path name of spf relative to start dir.
 * A buffer is needed to hold the returned path in.
 */
char *
spf_full_name2(struct sp_file *spf, char *buf, size_t len)
{
	return full_name(spf->spf_name, buf, len);
}

/*
 * spf_use_tmp_name()
 *
 * Generate a unique file name and replace spf_name with it.
 *
 * A call to spf_restore_name() later will rename spf_name file
 * the original file name and free the allocated buffer for the
 * temporary filename.
 */
char *
spf_use_tmp_name(struct sp_file *spf)
{
	char	*str;
	char	buf[PATH_MAX], buf2[PATH_MAX];
	int		fd;

	if ((str = malloc(PATH_MAX)) == NULL)
		return NULL;
	snprintf(buf, PATH_MAX, ".in.%s.XXX", spf->spf_name);
	if (full_name(buf, buf2, PATH_MAX) == NULL)
		return NULL;
	if ((fd = mkstemp(buf2)) < 0)
		return NULL;
	close(fd);		/* can't pass the descriptor in any convenient way */
	if ((str = strdup(rindex(buf2, '/') + 1)) == NULL)
		return NULL;
	spf->spf_orig_name = spf->spf_name;
	spf->spf_name = str;
	return str;
}

/*
 * spf_restore()
 *
 * Restore the name of the downloaded file. Free the allocated buffer for the
 * temporary filename.
 */
int
spf_restore_name(struct sp_file *spf)
{
	struct	stat sb;
	char	*f, *o, buf[PATH_MAX];
	int		ret_val;

	if (spf->spf_orig_name == NULL)
		return 0;

	ret_val = 0;
	f = spf_full_name(spf);
	if (lstat(f, &sb) < 0 && errno == ENOENT) {
		/* the temp file does not exist anylonger,
		 * give back the original name
		 */
		goto ret;
	}

	o = full_name(spf->spf_orig_name, buf, sizeof(buf));
	if (rename(f, o) < 0) {
		e_warn("failed to rename %s to %s", o, f);
		ret_val = -1;
	}

ret:
	free(spf->spf_name);
	spf->spf_name = spf->spf_orig_name;
	spf->spf_orig_name = NULL;
	return ret_val;
}


/*
 * symlink_cd()
 * 
 * Used by spf_symlink_resolve() to resolve what a symlink points to
 */
static int
symlink_cd(char *dir, int dir_len, char *buf, size_t buf_siz)
{
	size_t	len;

	len = strlen(buf);
	if ((buf_siz - len - 1)<= dir_len)
		return -1;

	switch (dir_len) {
	case 0:
		return 0;
	case 1:
		if (*dir == '.')
			return 0;
		break;
	case 2:
		if (strncmp(dir, "..", (size_t)2) == 0)
			goto cd_up;
		break;
	default:
		break;
	}
	(void)memcpy(buf + len, dir, (size_t) dir_len);
	len += dir_len;
	if (buf[len - 1] != '/') {
		buf[len++] = '/';
	}
	buf[len] = '\0';
	
	return 0;

cd_up:
	/* change ./../ to ../ to keep consistent */
	if (strcmp(buf, "./../") == 0) {
		/* don't forget the NULL terminator */
		(void)memmove(buf, buf + 2, len - 1); 
		len -= 2;
	}

	/* first entry goes up */
	if (strncmp(buf, "../", (size_t)3) == 0) {
		(void) strcat(buf, "../");
		return 0;
	}

	/* path is empty */
	if (len == 0) {
		(void) strcpy(buf, "../");
		return 0;
	}

	/* up one dir */
	for (len--; buf[len] != '/' && len != 0; len--);
	if (buf[len] == '/') {
		/* up at root doesn't change anything */
		if (len == 0)
			return 0;
		buf[len + 1] = '\0';
		return 0;
	}

	/* empty path */
	strcpy(buf, "./");
	return 0;
}

/*
 * spf_symlink_resolve()
 *
 * Resolves what path a symlink points to. Returned path is the
 * shortest path regarding .. and so.
 */

char *
spf_symlink_resolve(struct sp_file *spf, char *buf, size_t len)
{
	char	*p;
	size_t	plen;
	int		i, j;

	p = spf->spf_symlink;
	if (p == NULL)
		return NULL;

	buf[0] = '\0';

	/* The symlink is relative */
	if (*p != '/') {
		/* buffer overrun check */
		if (len <= strlen(cur_dir))
			return NULL;

		strcat(buf, "./");
		if (strncmp(cur_dir, "./", (size_t)2) == 0)
			(void) strcat(buf, cur_dir + 2);
		else
			(void) strcat(buf, cur_dir);
	}

	plen = strlen(p);
	i = j = 0;
	while (i <= plen) {
		if (p[i] == '/' || p[i] == '\0') {
			if (symlink_cd(p + j, i - j, buf, len) < 0)
				return NULL;
			j = i + 1;
			i++;
		}
		i++;
	}
	/* remove trailing '/' */
	plen = strlen(buf);
	if (plen > 0 && buf[plen - 1] == '/')
		buf[plen - 1] = '\0';
	return buf;
}

/*
 * spf_is_a_skip_find()
 *
 * Used by spf_is_a_skip() to determine whether a file should be skipped.
 * Returns 1 the file is to be skipped.
 */
static int
spf_is_a_skip_find(struct sp_skip *sps, struct sp_file *spf)
{
	char	buf[PATH_MAX];
	char	*p;
	int		match;

	p = spf_full_name2(spf, buf, sizeof(buf));
	p++; /* remove the . in ./ */
	match = sps_match(sps, p);
	if (match || spf->spf_type != SYMLINK)
		return match;

	/* sps_type == SYMLINK */
	p = spf_symlink_resolve(spf, buf, sizeof(buf));
	match = sps_match(sps, p);
	return match;
}

/*
 * spf_is_a_skip()
 *
 * Returns 1 if spf is a file to skip.
 */
int
spf_is_a_skip(struct sp_file *spf)
{
	char	buf[PATH_MAX];
	char	*p;
	cl_sps_find_f	f_fun;

	if (skip == NULL || skip == CL_NEXT(skip))
		return 0;

	/* LINTED don't obfuscate the cl_sps_find() more that necessary */
	f_fun = (cl_sps_find_f) spf_is_a_skip_find;

	if (cl_sps_find(skip, NULL, f_fun, (struct sp_skip *)spf) < 0)
		return 0;

	p = spf_full_name2(spf, buf, sizeof(buf));
	e_warnx("skipping %s file '%s'",
		spf->spf_opt & SPFO_LOCAL ? "local" : "remote", p);
	return 1;
}

/*
 * spf_is_a_treatasdir_find()
 *
 * Used by spf_is_a_skip() to determine whether a link should be treated
 * as a directory.
 * Returns 1 the file is to be treated as a directory.
 */
static int
spf_is_a_treatasdir_find(struct sp_treatasdir *spt, struct sp_file *spf)
{
	char	buf[PATH_MAX];
	char	*p;
	int		match;

	if (spf->spf_type != SYMLINK)
		return 0;

	if (!spt->spt_deref) {
		p = spf_full_name2(spf, buf, sizeof(buf));
		p++; /* remove the . in ./ */
	} else {
#if 1
		p = spf_symlink_resolve(spf, buf, sizeof(buf));
		p++; /* remove the . in ./ */
#else
		if (spf->spf_symlink[0] != '/' &&
			strncmp(spf->spf_symlink, "..", (size_t)2) != 0) {
			(void) snprintf(buf, (size_t)PATH_MAX, "./%s", spf->spf_symlink);
			p = buf;
		} else
			p = spf->spf_symlink;
#endif
	}

	match = spt_match(spt, p);
#if 0
	e_warnx("comparing '%s' with regexp: %s", p, match ? "match" : "not match");
#endif
	return match;
}


/*
 * spf_is_a_treatasdir()
 *
 * Returns 1 if spf is a file to treat as directory.
 */
int
spf_is_a_treatasdir(struct sp_file *spf)
{
	char	buf[PATH_MAX], buf2[PATH_MAX];
	char	*p, *p2;
	cl_spt_find_f	f_fun;

	if (treatasdir == NULL || treatasdir == CL_NEXT(treatasdir))
		return 0;

	/* LINTED don't obfuscate the cl_sps_find() more that necessary */
	f_fun = (cl_spt_find_f) spf_is_a_treatasdir_find;

	if (cl_spt_find(treatasdir, NULL, f_fun, (struct sp_treatasdir *)spf) < 0)
		return 0;

	p = spf_full_name2(spf, buf, sizeof(buf));
	p2 = spf_symlink_resolve(spf, buf2, sizeof(buf2));
	e_warnx("treating %s link '%s -> %s' pointing at '%s' as directory '%s'",
		spf->spf_opt & SPFO_LOCAL ? "local" : "remote",
		p, spf->spf_symlink, p2, p);
	return 1;
}

/*
 * spf_time_cmp()
 *
 * Compares the time of two sp_file's.
 * Returns the result in the same style as strcmp()
 */
int
spf_time_cmp(struct sp_file *f1, struct sp_file *f2)
{
	if ((f1->spf_time - f1->spf_tgranul) <= (f2->spf_time + f2->spf_tgranul) &&
		(f1->spf_time + f1->spf_tgranul) >= (f2->spf_time - f2->spf_tgranul))
		return 0;
	if (f1->spf_time > f2->spf_time)
		return 1;
	return -1;
}

/*
 * spf_name_cmp()
 *
 * Compares the name of two sp_file's.
 * Returns the result in the same style as strcmp()
 */
int
spf_name_cmp(struct sp_file *f1, struct sp_file *f2)
{
	if (f1 == NULL && f2 == NULL)
		return 0;
	if (f1 == NULL) {
		if (*f2->spf_name != '\0')
			return -*f2->spf_name;
		return -1;
	}
	if (f2 == NULL) {
		if (*f2->spf_name != '\0')
			return *f1->spf_name;
		return 1;
	}
	return strcmp(f1->spf_name, f2->spf_name);
}

/* 
 * spf_push_comp()
 *
 * Used as argument to heap_spf_init() to maintain heap.
 */
int
spf_push_comp(struct sp_file * f1, struct sp_file * f2)
{
	return -spf_name_cmp(f1, f2);
}

/*
 * spf_unalloc()
 *
 * Frees a sp_file.
 */
void
spf_unalloc(struct sp_file *f)
{
	if (f != NULL) {
		if (f->spf_name != NULL)
			free(f->spf_name);
		if (f->spf_orig_name != NULL)
			free(f->spf_orig_name);
		if (f->spf_symlink != NULL)
			free(f->spf_symlink);
		free(f);
	}
}

/*
 * spf_new_time()
 *
 * Updates the time of the file in the filesystem according to
 * spf->spf_time.
 */
int
spf_new_time(struct sp_file *spf)
{
	char	*f;
	struct	timeval ftime[2];

	f = spf_full_name(spf);
	ftime[0].tv_sec = spf->spf_time;
	ftime[1].tv_sec = spf->spf_time;
	ftime[0].tv_usec = ftime[1].tv_usec = 0;
	if (utimes(f, &ftime[0]) < 0) {
		e_warn("failed to set time on %s", f);
		return -1;
	}
	return 0;
}

/* #ifdef DEBUG */

#if 0
void
spf_show_que(struct cl_spf_que *q)
{
	struct	cl_spf_que *p;
	struct	sp_file *spf;

	for (p = CL_NEXT(q); p != q; p = CL_NEXT(p)) {
		spf = CL_ELM(p);
		(void) printf("name = %s, time = %lu, mode = %o, "
			"type = %d, size = %lu\n",
			spf->spf_name, spf->spf_time, spf->spf_mode,
			spf->spf_type, (unsigned long) spf->spf_size);
	}
}
#endif

/*
 * is_dir()
 *
 * Used by rm_f() and rm_rf()
 * Returns 0 if f isn't a directory.
 */
static int
is_dir(char *f)
{
	struct stat sb;

	if (lstat(f, &sb) < 0)
		return 0;
	return S_ISDIR(sb.st_mode);
}

/*
 * rm_f()
 *
 * Used by rm_rf() and spf_rm()
 * Removes _one_ file or diretory. Will exit if the maximum number
 * of deletes is exceeded.
 */
static int
rm_f(char *f)
{
	int	ret;

	if (sp_max_delete != 0 && sp_max_delete <= sp_deletes) 
		e_errx(1, "Maximum number of deletes exceeded, quit");
	ret = is_dir(f) ? rmdir(f) : unlink(f);
	if (ret == 0)
		sp_deletes++;
	else
		e_warn("rm_f: failed to remove %s", f);
	return ret;
}

/*
 * rm_rf()
 *
 * Used by spf_rm()
 * Removes a hierarchy of diretories. Will exit if the maximum number
 * of deletes is exceeded.
 */
static int
rm_rf(char *f)
{
	DIR		*dirp;
	struct	dirent *dp;
	char	file_buf[PATH_MAX];

	/* XXX this shouldn't be needed */
	if (have_dotdot_ref(f)) {
		e_warnx("rm_rf: have .. reference in path: %s", f);
		return -1;
	}

	if (!is_dir(f))
		return rm_f(f);

	if (sp_max_delete != 0 && sp_max_delete <= sp_deletes) 
		e_errx(1, "Maximum number of deletes exceeded, quit");

	if ((dirp = opendir(f)) == NULL) 
		return -1;

	while ((dp = readdir(dirp)) != NULL) {
		if (strcmp("..", dp->d_name) == 0 || strcmp(".", dp->d_name) == 0)
			continue;
		(void) snprintf(file_buf, (size_t)PATH_MAX, "%s/%s", f, dp->d_name);
		file_buf[PATH_MAX - 1] = '\0';
		if (is_dir(file_buf)) {
			(void) rm_rf(file_buf);
			continue;
		}
		if (sp_max_delete != 0 && sp_max_delete <= sp_deletes) {
			(void) closedir(dirp);
			e_errx(1, "Maximum number of deletes exceeded, quit");
		}
		(void) rm_f(file_buf);
	}
	(void) closedir(dirp);
	return rm_f(f); 
}

/*
 * spf_rm()
 *
 * Removes a hierarchy of diretories if sp_do_delete is set. Will exit
 * if the maximum number of deletes is exceeded.
 */
int
spf_rm(struct sp_file *spf)
{
	char	*f;

	/* only delete if we are supposed to */
	if (sp_do_delete == 0)
		return 0;

	f = spf_full_name(spf);
	if (spf->spf_type == DIRECTORY) {
		e_warnx("rm -rf %s", f);
		if (rm_rf(f) < 0) {
			e_warn("failed to remove %s", f);
			return -1;
		}
		return 0;
	} 
	e_warnx("rm -f %s", f);
	if (rm_f(f) < 0) {
		e_warn("failed to remove %s", f);
		return -1;
	}
	return 0;
}

/*
 * spf_clone()
 *
 * Returns a copy of a sp_file
 */
struct sp_file *
spf_clone(struct sp_file *spf)
{
	struct sp_file *p;

	p = malloc(sizeof(*p));
	(void) memcpy(p, spf, sizeof(*p));

	if (spf->spf_name != NULL)
		p->spf_name = strdup(spf->spf_name);

	if (spf->spf_symlink != NULL)
		p->spf_symlink = strdup(spf->spf_symlink);

	return p;
}

/*
 * spf_chmod()
 *
 * Updates the modes of the file in the filesystem according to
 * spf->spf_mode with mode bitwise OR:ed in.
 */
int
spf_chmod(struct sp_file *spf, mode_t mode)
{
	char	*f;

	f = spf_full_name(spf);
	if (spf->spf_opt & SPFO_LOG_CHMOD)
		e_warnx("chmod %o %s", (unsigned) spf->spf_mode|mode, f);
	if (chmod(f, spf->spf_mode|mode) < 0) {
		e_warn("failed to chmod %s", f);
		return -1;
	}
	return 0;
}

/*
 * spf_mkdir()
 *
 * Creates a diretory if SPFO_DONT_CREATE isn't set in spf_opt.
 */
int
spf_mkdir(struct sp_file *spf)
{
	char	*f;

	/* Directory already exists, report success */
	if (spf->spf_opt & SPFO_DONT_CREATE)
		return 0;

	f = spf_full_name(spf);
	e_warnx("mkdir %s", f);
	if (mkdir(f, spf->spf_mode | S_IRWXU) < 0) {
		e_warn("mkdir %s failed", f);
		return -1;
	}
	return 0;
}

/*
 * spf_symlink()
 *
 * Creates a symlink according to spf_name and spf_symlink.
 */
int
spf_symlink(struct sp_file *spf)
{
	e_warnx("ln -s %s %s", spf->spf_symlink, spf_full_name(spf));
	if (symlink(spf->spf_symlink, spf_full_name(spf)) < 0) {
		e_warn("ln -s %s %s failed", spf->spf_symlink, spf_full_name(spf));
		return -1;
	}
	return 0;
}

/*
 * spf_stat_init()
 *
 * Creates a sp_file with the values from a file in the filesystem.
 */
int
spf_stat_init(char *f, struct sp_file **spf)
{
	char	*cf, *buf;
	int		ret;
	struct	sp_file *s;
	struct	stat sb;

	s = calloc((size_t)1, sizeof(*s));
	if (s == NULL)
		return -1;
	s->spf_name = strdup(f);
	cf = full_name(f, cur_file, (size_t)PATH_MAX);

	if (lstat(cf, &sb) < 0) {
		e_warn("failed to lstat %s", cf);
		goto ret_bad;
	}

	s->spf_time = sb.st_mtime;
	s->spf_tgranul = 0;

	s->spf_size = sb.st_size;
	s->spf_mode = sb.st_mode & 0xfff; /* mask out permissions */
	switch(sb.st_mode & S_IFMT) {
	case S_IFDIR:
		s->spf_type = DIRECTORY;
		break;
	case S_IFREG:
		s->spf_type = PLAINFILE;
		break;
	case S_IFLNK:
		s->spf_type = SYMLINK;
		break;
	default:
		s->spf_type = UNKNOWN;
	}
	if (s->spf_type == SYMLINK) {
		buf = alloca((size_t)(PATH_MAX + 1));
		if ((ret = readlink(cf, buf, (size_t)PATH_MAX)) < 0) {
			e_warn("failed to readlink %s", cf);
			goto ret_bad;
		}
		buf[ret] = '\0';
		s->spf_symlink = strdup(buf);
	}
	s->spf_opt |= SPFO_LOCAL;	/* mark spf as local */
	*spf = s;
	return 0;

ret_bad:
	spf_unalloc(s);
	return -1;
}


/* init the sp_skip struct */
struct sp_skip *
sps_init(const char *arg)
{
	size_t	len;
	int		cflags;
	struct	sp_skip *sps;

	len = strlen(arg);
	if ((sps = calloc((size_t)1, sizeof(*sps) + len + 1)) == NULL)
		return NULL;
	/* LINTED save us from one calloc */
	sps->sps_name = (char *)(sps + 1);
	(void) strcpy(sps->sps_name, arg);
	cflags = 0;
	cflags |= REG_EXTENDED;		/* extended RE's */
	cflags |= REG_NOSUB;		/* only report match or no match */
	sps->sps_reg_errno = regcomp(&sps->sps_reg, sps->sps_name, cflags);
	if (sps->sps_reg_errno != 0) {
		free(sps);
		return NULL;
	}
	return sps;
}

/* init the sp_treatasdir struct */
struct sp_treatasdir *
spt_init(const char *arg)
{
	size_t	len;
	int		cflags;
	struct	sp_treatasdir *spt;

	len = strlen(arg);
	if ((spt = calloc((size_t)1, sizeof(*spt) + len + 1)) == NULL)
		return NULL;
	/* LINTED save us from one calloc */
	spt->spt_name = (char *)(spt + 1);
	(void) strcpy(spt->spt_name, arg);
	cflags = 0;
	cflags |= REG_EXTENDED;		/* extended RE's */
	cflags |= REG_NOSUB;		/* only report match or no match */
	spt->spt_reg_errno = regcomp(&spt->spt_reg, spt->spt_name, cflags);
	spt->spt_reg_need_regfree = (spt->spt_reg_errno == 0);
	return spt;
}

/* return 1 if error */
int
spt_error(struct sp_treatasdir *spt)
{
	return (spt->spt_reg_errno != 0);
}

/* make a human readable error message */
char *
spt_strerror(struct sp_treatasdir *spt)
{
	size_t	buf_size;

	/* calculate needed size of buf */
	buf_size = regerror(spt->spt_reg_errno, &spt->spt_reg, NULL, (size_t)0);

	/* only malloc if spt_buf too small */
	if (spt->spt_buf_size < buf_size) {
		if (spt->spt_buf != NULL)
			free(spt->spt_buf);
		spt->spt_buf = malloc(buf_size);
		if (spt->spt_buf == NULL) {
			spt->spt_buf_size = 0;
			return NULL;
		}
		spt->spt_buf_size = buf_size;
	}

	(void) regerror(spt->spt_reg_errno, &spt->spt_reg,
		spt->spt_buf, spt->spt_buf_size);
	return spt->spt_buf;
}

/* free the sp_skip struct */
void
spt_unalloc(struct sp_treatasdir *spt)
{
	if (spt->spt_buf != NULL)
		free(spt->spt_buf);

	/* only regfree after a successful regcomp */
	if (spt->spt_reg_need_regfree)	
		regfree(&spt->spt_reg);

	free(spt);
}



/* return 1 if error */
int
sps_error(struct sp_skip *sps)
{
	return (sps->sps_reg_errno != 0);
}

/* make a human readable error message */
char *
sps_strerror(struct sp_skip *sps)
{
	size_t	buf_size;

	/* calculate needed size of buf */
	buf_size = regerror(sps->sps_reg_errno, &sps->sps_reg, NULL, (size_t)0);

	/* only malloc if sps_buf too small */
	if (sps->sps_buf_size < buf_size) {
		if (sps->sps_buf != NULL)
			free(sps->sps_buf);
		sps->sps_buf = malloc(buf_size);
		if (sps->sps_buf == NULL) {
			sps->sps_buf_size = 0;
			return NULL;
		}
		sps->sps_buf_size = buf_size;
	}

	(void) regerror(sps->sps_reg_errno, &sps->sps_reg,
		sps->sps_buf, sps->sps_buf_size);
	return sps->sps_buf;
}

/* free the sp_skip struct */
void
sps_unalloc(struct sp_skip *sps)
{
	if (sps->sps_buf != NULL)
		free(sps->sps_buf);

	regfree(&sps->sps_reg);

	free(sps);
}

int
sps_match(struct sp_skip *sps, char *name)
{
	regmatch_t *pmatch;
	size_t	nmatch;
	int		eflags;

	eflags = 0;
	nmatch = 0;		/* only want to know wether it matched or not */
	pmatch = NULL;	/* no need for storage */
	sps->sps_reg_errno = regexec(&sps->sps_reg, name, nmatch, pmatch, eflags);
	return (sps->sps_reg_errno == 0);
}

int
spt_match(struct sp_treatasdir *spt, char *name)
{
	regmatch_t *pmatch;
	size_t	nmatch;
	int		eflags;

	eflags = 0;
	nmatch = 0;		/* only want to know wether it matched or not */
	pmatch = NULL;	/* no need for storage */
	spt->spt_reg_errno = regexec(&spt->spt_reg, name, nmatch, pmatch, eflags);
	return (spt->spt_reg_errno == 0);
}

/***** functions for recursive list of files *****/

static char *
parse_dir_name(char *str)
{
	size_t	len;

	str += strspn(str, "./");
	/* remove trailing '\r', '\n' and ':' */
	len = strlen(str);
	if (str[len - 1] == '\n')
		str[len - 1] = '\0';
	len = strlen(str);
	if (str[len - 1] == '\r')
		str[len - 1] = '\0';
	len = strlen(str);
	if (str[len - 1] != ':')
		return NULL;
	str[len - 1] = '\0';
	return strdup(str);
}

static int
is_empty_line(char *str)
{
	size_t	l;

	l = strspn(str, " \n\r");
	return (str[l] == '\0');
}

static void
spd_unalloc(struct sp_dir *spd)
{
	if (spd->spd_spdl != NULL)
		sl_spd_free(spd->spd_spdl, spd_unalloc);
	if (spd->spd_full_name != NULL)
		free(spd->spd_full_name);
}

static struct sp_dir *
spd_init(FILE *f)
{
	char	buf[PATH_MAX];
	struct	sp_dir *spd;
	long	pos;
	int		oerrno;

	if ((spd = malloc(sizeof(*spd))) == NULL)
		return NULL;

	do {
		if ((pos = ftell(f)) < 0)
			goto ret_bad;
		if (fgets(buf, (int)sizeof(buf), f) == NULL)
			goto ret_bad;
	} while (is_empty_line(buf));
	
	if ((spd->spd_full_name = parse_dir_name(buf)) == NULL) {
		if (errno == ENOMEM)
			goto ret_bad;
	} else {
		if ((spd->spd_name = rindex(spd->spd_full_name, '/')) == NULL)
			spd->spd_name = spd->spd_full_name;
		else
			spd->spd_name++;
		if ((pos = ftell(f)) < 0)
			goto ret_bad;
		if (fgets(buf, (int)sizeof(buf), f) == NULL)
			goto ret_bad;
	}

	if (strncmp(buf + 1, "otal", (size_t)4) != 0) {
		if (fseek(f, pos, SEEK_SET) < 0)
			goto ret_bad;
	}

	spd->spd_begin = pos;
	do {
		if (fgets(buf, (int)sizeof(buf), f) == NULL) {
			if (feof(f))
				break;
			goto ret_bad;
		}
	} while (is_empty_line(buf) == 0);
	/* We are at end of file or at the empty line that follows each
	 * directory.
	 */
	if ((spd->spd_end = ftell(f)) < 0)
		goto ret_bad;
	return spd;

ret_bad:
	oerrno = errno;
	free(spd);
	errno = oerrno;
	return NULL;
}

static u_int32_t
spd_key(const char *str)
{
	u_int32_t	key;
	size_t		len;
	char		*s;
	int			i;

	key = 0;
	len = strlen(str);
	s = (char *)&key;
	for (i = 0; i < 4 && len > 0; i++, len--)
		s[i] = str[len];
	return key;
}

static int
spd_find(struct sp_dir *spd, const char *str, struct sp_dir **p)
{
	u_int32_t	key;
	struct		skip_spd_list_node *pos;

	key = spd_key(str);
	if (sl_spd_find_pos(spd->spd_spdl, key, &pos, p) < 0)
		goto ret_not_found;
	while (strcmp((*p)->spd_name, str) != 0) {
		if (sl_spd_walk(spd->spd_spdl, &pos, p) < 0)
			goto ret_not_found;
		if (spd_key((*p)->spd_name) != key)
			goto ret_not_found;
	}
	return 0;

ret_not_found:
	errno = ENOENT;
	return -1;
}

static char *
mystrsep(char **strp, const char *delim)
{
	char	*str;

	while ((str = strsep(strp, delim)) != NULL) {
		if (*str != '\0')
			return str;
	}
	return str;
}

static int
spd_find_path(struct sp_dir_idx *spdi, const char *path, struct sp_dir **spd)
{
	char	buf[PATH_MAX];
	char	*str, *s;
	struct	sp_dir *p1, *p2;

	strlcpy(buf, path, PATH_MAX);
	str = buf;
	if ((s = mystrsep(&str, "/")) == NULL)
		s = str;
	p1 = spdi->spdi_spd;
	for ( ;; ) {
		if (spd_find(p1, s, &p2) < 0)
			return -1;
		p1 = p2;
		if ((s = mystrsep(&str, "/")) == NULL)
			return 0;
	}
}

static int
spdi_insert_spd(struct sp_dir_idx *spdi, struct sp_dir *spd)
{
	char	buf[PATH_MAX], *str;
	struct	sp_dir *p;
	size_t	len;

	strlcpy(buf, spd->spd_full_name, PATH_MAX);
	for ( ;; ) {
		str = rindex(buf, '/');
		if ((len = strlen(str)) == 0)
			return -1;
		if (str[len - 1] != '/')
			break;
		str[len - 1] = '\0';
	}
	if (spd_find_path(spdi, buf, &p) < 0)
		return -1;
	if (sl_spd_ins(p->spd_spdl, spd_key(spd->spd_name), spd) < 0)
		return -1;
	return 0;
}

static int
spdi_index_file(struct sp_dir_idx *spdi)
{
	struct	sp_dir *spd;

	if ((spd = spd_init(spdi->spdi_file)) == NULL)
		return -1;

	spdi->spdi_spd = spd;

	for ( ;; ) {
		if ((spd = spd_init(spdi->spdi_file)) == NULL) {
			if (feof(spdi->spdi_file))
				return 0;
			return -1;
		}
		if (spdi_insert_spd(spdi, spd) < 0)
			return -1;
	}
}

struct sp_dir_idx *
spdi_init(const char *filename, const char *path)
{
	struct	sp_dir_idx *spdi;
	int		oerrno;

	if ((spdi = calloc(1, sizeof(*spdi))) == NULL)
		return NULL;
	if ((spdi->spdi_file = fopen(filename, "r")) == NULL)
		goto ret_bad;
	if ((spdi->spdi_path = strdup(path)) == NULL)
		goto ret_bad;
	if (spdi_index_file(spdi) < 0)
		goto ret_bad;
	return spdi;

ret_bad:
	oerrno = errno;
	if (spdi->spdi_file != NULL)
		fclose(spdi->spdi_file);
	if (spdi->spdi_path != NULL)
		free(spdi->spdi_path);
	if (spdi->spdi_spd != NULL)
		spd_unalloc(spdi->spdi_spd);
	free(spdi);
	errno = oerrno;
	return NULL;
}

void
spdi_unalloc(struct sp_dir_idx *spdi)
{
	if (spdi == NULL)
		return;
	spd_unalloc(spdi->spdi_spd);
	if (spdi->spdi_file != NULL)
		fclose(spdi->spdi_file);
	if (spdi->spdi_path != NULL)
		free(spdi->spdi_path);
	free(spdi);
}

int
spdi_chdir(struct sp_dir_idx *spdi, const char *dir)
{
	if (spd_find_path(spdi, dir, &spdi->spdi_cur_dir) < 0)
		return -1;
	if (fseek(spdi->spdi_file, spdi->spdi_cur_dir->spd_begin, SEEK_SET) < 0)
		return -1;
	return 0;
}

int
spdi_next_file(struct sp_dir_idx *spdi, struct sp_file **spf)
{
	char	buf[PATH_MAX];
	struct	sp_file *p;
	long	pos;

	if ((pos = ftell(spdi->spdi_file)) < 0)
		return -1;
	if (pos > spdi->spdi_cur_dir->spd_end) {
		*spf = NULL;	/* XXX to report end dir listing */
		return -1;	
	}
	if (fgets(buf, PATH_MAX, spdi->spdi_file) == NULL)
		return -1;
	if ((p = parse_row(buf)) == NULL)
		return -1;
	*spf = p;
	return 0;
}

