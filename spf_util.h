/*$Id: spf_util.h,v 1.10 2000/03/26 22:49:56 jens Exp $*/
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

#ifndef SPF_UTIL__H
#define SPF_UTIL__H

#include "spegla.h"

char *cur_dir_cd(char *str);
char *cur_dir_cdup(void);
char *spf_full_name(struct sp_file *spf);
char *spf_full_name2(struct sp_file *spf, char *buf, size_t len);
int spf_is_a_skip(struct sp_file *spf);
int spf_is_a_treatasdir(struct sp_file *spf);
int spf_push_comp(struct sp_file * f1, struct sp_file * f2);
int spf_time_cmp(struct sp_file *f1, struct sp_file *f2);
int spf_name_cmp(struct sp_file *f1, struct sp_file *f2);
void spf_unalloc(struct sp_file *f);
int spf_new_time(struct sp_file *spf);
int spf_rm(struct sp_file *spf);
struct sp_file *spf_clone(struct sp_file *spf);
int spf_chmod(struct sp_file *spf, mode_t mode);
int spf_mkdir(struct sp_file *spf);
int spf_symlink(struct sp_file *spf);
char *spf_symlink_resolve(struct sp_file *spf, char *buf, size_t len);
char *spf_use_tmp_name(struct sp_file *spf);
int spf_restore_name(struct sp_file *spf);
int spf_stat_init(char *f, struct sp_file **spf);

struct sp_skip *sps_init(const char *arg);
void sps_unalloc(struct sp_skip *sps);
int sps_match(struct sp_skip *sps, char *name);
int sps_error(struct sp_skip *sps);
char * sps_strerror(struct sp_skip *sps);

struct sp_treatasdir *spt_init(const char *arg);
void spt_unalloc(struct sp_treatasdir *spt);
int spt_match(struct sp_treatasdir *spt, char *name);
int spt_error(struct sp_treatasdir *spt);
char * spt_strerror(struct sp_treatasdir *spt);

struct sp_dir_idx *spdi_init(const char *filename, const char *path);
void spdi_unalloc(struct sp_dir_idx *spdi);
int spdi_chdir(struct sp_dir_idx *spdi, const char *dir);
int spdi_next_file(struct sp_dir_idx *spdi, struct sp_file **spf);


#endif
