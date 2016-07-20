/*$Id: parserow.c,v 1.29 1999/10/07 12:02:54 jens Exp $*/
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

#include <ctype.h>
#ifdef ULTRIX
#include <sys/types.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef __NetBSD__
#	include <tzfile.h>
#else
#	ifndef SECSPERMIN
#		define SECSPERMIN 60
#	endif
#	ifndef SECSPERDAY
#		define SECSPERDAY	((long) 60 * 60 * 24)
#	endif
#	ifndef DAYSPERNYEAR
#		define DAYSPERNYEAR 365
#	endif
#endif
#include "strlcpy.h"
#include "parserow.h"
#include "spegla.h"

#ifdef NOPROTOS
char	*strdup(char *);
#endif

static int
parse_file_mode(char *p, struct sp_file *spf)
{
#define ADVANCE(n)	if (*(++(n)) == '\0') return 0

	if (p == NULL || *p == '\0')
		return 0;
	switch (*p) {
	case '-':
		spf->spf_type = PLAINFILE;
		break;
	case 'd':
		spf->spf_type = DIRECTORY;
		break;
	case 'l':
		spf->spf_type = SYMLINK;
		break;
	default:
		return 0;
	}
	ADVANCE(p);
	switch (*p) {
	case 'r':
		spf->spf_mode |= S_IRUSR;
		break;
	case '-':
		break;
	default:
		return 0;
	}
	ADVANCE(p);
	switch (*p) {
	case 'w':
		spf->spf_mode |= S_IWUSR;
		break;
	case '-':
		break;
	default:
		return 0;
	}
	ADVANCE(p);
	switch (*p) {
	case 'x':
		spf->spf_mode |= S_IXUSR;
		break;
	case 's':
		spf->spf_mode |= S_IXUSR | S_ISUID;
		break;
	case 'S':
		spf->spf_mode |= S_ISUID;
		break;
	case '-':
		break;
	default:
		return 0;
	}
	ADVANCE(p);
	switch (*p) {
	case 'r':
		spf->spf_mode |= S_IRGRP;
		break;
	case '-':
		break;
	default:
		return 0;
	}
	ADVANCE(p);
	switch (*p) {
	case 'w':
		spf->spf_mode |= S_IWGRP;
		break;
	case '-':
		break;
	default:
		return 0;
	}
	ADVANCE(p);
	switch (*p) {
	case 'x':
		spf->spf_mode |= S_IXGRP;
		break;
	case 's':
		spf->spf_mode |= S_IXGRP | S_ISGID;
		break;
	case 'S':
		spf->spf_mode |= S_ISGID;
		break;
	case '-':
		break;
	default:
		return 0;
	}
	ADVANCE(p);
	switch (*p) {
	case 'r':
		spf->spf_mode |= S_IROTH;
		break;
	case '-':
		break;
	default:
		return 0;
	}
	ADVANCE(p);
	switch (*p) {
	case 'w':
		spf->spf_mode |= S_IWOTH;
		break;
	case '-':
		break;
	default:
		return 0;
	}
	ADVANCE(p);
	switch (*p) {
	case 'x':
		spf->spf_mode |= S_IXOTH;
		break;
	case 't':
		spf->spf_mode |= S_IXOTH | S_ISVTX;
		break;
	case 'T':
		spf->spf_mode |= S_ISVTX;
		break;
	case '-':
		break;
	default:
		return 0;
	}
	return 1;
#undef ADVANCE
}

struct sp_file *
parse_row(char *row)
{
	struct	sp_file *spf;
	char	*slusk, *endptr;
	char	tmp[PATH_MAX + 24];
	char	*tok, toksep[] = " ";
	struct	tm tiden;
	time_t	now;
	int		i, maybe_size;
	int		rowlen;
	long	size;
	static	const char *months[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	maybe_size = 0;
	rowlen = (int)strlen(row);
	if (rowlen < (sizeof(tmp) - 1)) {
		(void) strlcpy(tmp, row, (size_t)(PATH_MAX + 24));
		row = tmp;
	}

	spf = calloc((size_t)1, sizeof(struct sp_file));
	if (spf == NULL)
		return NULL;

	/* permissions */
	if ((tok = strtok(row, toksep)) == NULL)
		goto ret_bad;

	if (!parse_file_mode(tok, spf))
		goto ret_bad;

	/* link count */
	tok = strtok(NULL, toksep);

	/* group or user */
	tok = strtok(NULL, toksep);

	/* group or size */
	if ((tok = strtok(NULL, toksep)) == NULL)
		goto ret_bad;

	size = strtol(tok, &endptr, 10); /* atoi */
	if (*endptr == ' ' || *endptr == '\0')
		maybe_size = 1;

	/* size or month */
	if ((tok = strtok(NULL, toksep)) == NULL)
		goto ret_bad;

	spf->spf_size = strtol(tok, &endptr, 10); /* atoi */
	if (*endptr == ' ' || *endptr == '\0') {
		/* this was size, advance to month */
		if ((tok = strtok(NULL, toksep)) == NULL)
			goto ret_bad;
	} else {
		/* if we didn't get anything looking like size before,
		 * we're in trouble
		 */
		if (!maybe_size)
			goto ret_bad;
		spf->spf_size = size;
	}

	for (i = 0; i < 12; i++) {
		if (strcmp(tok, months[i]) == 0)
			break; /* we're done */
	}
	if (i == 12)
		goto ret_bad;	/* didn't find any month */

	tiden.tm_mon = i;

	 /* Day */ ;
	if ((tok = strtok(NULL, toksep)) == NULL)
		goto ret_bad;
	if (sscanf(tok, "%d", &tiden.tm_mday) != 1)
		goto ret_bad;

	/* Year */
	if ((tok = strtok(NULL, toksep)) == NULL)
		goto ret_bad;
	spf->spf_tgranul = 0;
	switch (sscanf(tok, "%d:%d:%d", &tiden.tm_hour, &tiden.tm_min, &tiden.tm_sec)) {
	case 1:
		tiden.tm_year = tiden.tm_hour;
		tiden.tm_hour = tiden.tm_min = tiden.tm_sec = 0;
		spf->spf_tgranul = SECSPERDAY;
		break;
	case 2:
		tiden.tm_sec = 0;
		spf->spf_tgranul = SECSPERMIN;
		now = time(NULL);
		tiden.tm_year = localtime(&now)->tm_mon < tiden.tm_mon ?
		    localtime(&now)->tm_year - 1 :
		    localtime(&now)->tm_year;
		break;
	case 3:
		if ((tok = strtok(NULL, toksep)) == NULL)
			goto ret_bad;
		if (sscanf(tok, "%d", &tiden.tm_year) != 1) {
			now = time(NULL);
			tiden.tm_year = localtime(&now)->tm_mon < tiden.tm_mon ?
			    localtime(&now)->tm_year - 1 :
			    localtime(&now)->tm_year;
		}
		break;
	default:
		goto ret_bad;
	}
	if (tiden.tm_year >= 1900)
		tiden.tm_year -= 1900;
	tiden.tm_wday = 0;
	tiden.tm_yday = 0;
	tiden.tm_isdst = -1;	/* viktigt, annars blir det konstigt! */
	spf->spf_time = mktime(&tiden);

	for (i = 0; tok[i] != ' ' && tok[i] != '\0'; i++)
		if (tok + i == row + rowlen)
			break;
	tok += i + 1;
	if (tok == row + rowlen)
		goto ret_bad;

	/* filename */
	if ((spf->spf_type == DIRECTORY) && (tok[strlen(tok) - 1] == '/'))
		tok[strlen(tok) - 1] = '\0';
	if ((spf->spf_mode | S_IXUSR | S_IXGRP | S_IXOTH) &&
	    (tok[strlen(tok) - 1] == '*')) {
		tok[strlen(tok) - 1] = '\0';
	}
	if (spf->spf_type == SYMLINK) {
		if ((slusk = strstr(tok, " -> ")) == NULL)
			goto ret_bad;
		slusk[0] = '\0';
		spf->spf_name = strdup(tok);
		if (spf->spf_name[strlen(spf->spf_name)] == '@')
			spf->spf_name[strlen(spf->spf_name)] = '\0';
		slusk += 4;
		spf->spf_symlink = strdup(slusk);
	} else {
		spf->spf_name = strdup(tok); /* should get eventual spaces */
	}
	return spf;

ret_bad:
	if (spf->spf_name != NULL)
		free(spf->spf_name);
	if (spf->spf_symlink != NULL)
		free(spf->spf_symlink);
	free(spf);
	return NULL;
}
