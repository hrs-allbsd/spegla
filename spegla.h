/*$Id: spegla.h,v 1.30 2000/03/26 22:49:56 jens Exp $*/
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


#ifndef SPEGLA__H
#define SPEGLA__H

#include <sys/types.h>
#ifdef MTYPES
#include "missing/defs.h"
#endif
#include <sys/stat.h>
#ifdef NOREGEX
#include "regex/regex.h"
#else
#include <regex.h>
#endif
#if defined(SunOS)
#	include <sys/time.h>
#	include <varargs.h>
#endif
#include <time.h>
#include <stdio.h>
#include "container.h"

#define	SPEGLA_VERSION		"1.1p1"

#define MAX_STR				1024

#ifndef PATH_MAX
#define PATH_MAX			255
#endif

enum sp_ftype {
	PLAINFILE = 0,
	DIRECTORY,
	SYMLINK,
	UNKNOWN
};

#define	SPFO_DONT_CREATE	1
#define	SPFO_LOG_CHMOD		(1<<1)
#define	SPFO_LOCAL			(1<<2)
#define	SPFO_ISLINK			(1<<3)

struct sp_file {
	time_t  spf_time;
	time_t	spf_tgranul;
	enum	sp_ftype spf_type;
	u_int32_t	spf_opt;
	off_t	spf_size;
	mode_t	spf_mode;
	char	*spf_name;
	char	*spf_symlink;
	char	*spf_orig_name;
};

struct sp_skip {
	char	*sps_name;
	char	*sps_buf;
	size_t	sps_buf_size;
	regex_t	sps_reg;
	int		sps_reg_errno;
};

struct sp_treatasdir {
	char	*spt_name;
	int		spt_deref;		/* match a derefed link */
	char	*spt_buf;
	size_t	spt_buf_size;
	regex_t	spt_reg;
	int		spt_reg_errno;
	int		spt_reg_need_regfree;
};

SKIP_TYPE(spd, struct sp_dir)

struct sp_dir {
	struct	skip_spd_list *spd_spdl;	/* subdirectories */
	char	*spd_full_name;				/* full name of directory */
	char	*spd_name;					/* name of directory */
	long	spd_begin;
	long	spd_end;
};

struct sp_dir_idx {
	char	*spdi_path;					/* location agains '/' */
	FILE	*spdi_file;					/* file containing dir list */
	struct	sp_dir *spdi_spd;			/* all dirs indexed	*/
	struct	sp_dir *spdi_cur_dir;		/* current dir */
	int		spdi_cur_pos;				/* position in current dir */
};

/* Declare some typesafe functions */
FIFO_TYPE(spf, struct sp_file)
HEAP_TYPE(spf, struct sp_file)
CL_TYPE(sps, struct sp_skip)
CL_TYPE(spt, struct sp_treatasdir)

#endif
