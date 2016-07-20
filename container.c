/* $Id: container.c,v 1.16 2000/05/27 13:38:14 jens Exp $ */
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
static char const cvsid[] = "$Id: container.c,v 1.16 2000/05/27 13:38:14 jens Exp $";
#endif

#include <sys/errno.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "container.h"

#ifdef NO_E_ERR
#include <err.h>
#else
#include "e_err.h"
#endif


int container_err_exit = 0;

void insque(void *entry, void *pred);	
void remque(void *entry);


struct cl_que *
cl_init(void)
{
	struct cl_que *p;

	if ((p = malloc(sizeof(*p))) == NULL) {
		if (container_err_exit)
			err(1, "cl_init: malloc");
		warn("cl_init: malloc");
		return NULL;
	}
	p->q_forw = p;
	p->q_back = p;
	p->q_data = NULL;
	return p;
}

void
cl_free(struct cl_que *headp, cl_free_f free_fun)
{
	struct	cl_que *p;

	if (headp == NULL)
		return;
	while (headp != headp->q_forw) {
		p = headp->q_forw;
		if (free_fun != NULL)
			free_fun(p->q_data);
		remque(p);
		free(p);
	}
	free(headp);
}

int
cl_walk(struct cl_que *headp, struct cl_que **pos, TYPE **data)
{
	int		res;

	if (pos == NULL)
		return cl_peek(headp, data);
	if (*pos == NULL) {
		if ((res = cl_peek_pos(headp, pos)) < 0)
			return res;
		if (data != NULL)
			*data = CL_DATA(*pos);
		return 0;
	}
	if (*pos == headp || CL_NEXT(*pos) == headp)
		return -1;
	*pos = CL_NEXT(*pos);
	if (data != NULL)
		*data = CL_DATA(*pos);
	return 0;
}

int
cl_walk_back(struct cl_que *headp, struct cl_que **pos, TYPE **data)
{
	int		res;

	if (pos == NULL)
		return cl_tail_peek(headp, data);
	if (*pos == NULL) {
		if ((res = cl_tail_peek_pos(headp, pos)) < 0)
			return res;
		if (data != NULL)
			*data = CL_DATA(*pos);
		return 0;
	}
	if (*pos == headp || CL_PREV(*pos) == headp)
		return -1;
	*pos = CL_PREV(*pos);
	if (data != NULL)
		*data = CL_DATA(*pos);
	return 0;
}

unsigned
cl_count(struct cl_que *cl)
{
	unsigned	num;
	struct		cl_que *p;

	num = 0;
	for (p = CL_NEXT(cl); p != cl; p = CL_NEXT(p))
		num++;
	return num;
}

int
cl_push(struct cl_que *headp, TYPE *data)
{
	struct cl_que *p;

	p = malloc(sizeof(*p));
	if (p == NULL) {
		if (container_err_exit)
			err(1, "cl_push: malloc");
		warn("cl_push: malloc");
		return -1;
	}
	p->q_data = data;
	insque(p, headp);
	return 0;
}

int
cl_tail_push(struct cl_que *headp, TYPE *data)
{
	struct cl_que *p;

	p = malloc(sizeof(*p));
	if (p == NULL) {
		if (container_err_exit)
			err(1, "cl_que: malloc");
		warn("cl_que: malloc");
		return -1;
	}
	p->q_data = data;
	insque(p, headp->q_back);
	return 0;
}


int
cl_pop(struct cl_que * headp, TYPE **data)
{
	struct cl_que *p;

	if (headp == NULL || headp->q_forw == headp) {
		errno = ENOENT;
		return -1;
	}
	p = headp->q_forw;
	remque(p);
	*data = p->q_data;
	free(p);
	return 0;
}

int
cl_tail_pop(struct cl_que *headp, TYPE **data)
{
	struct cl_que *p;

	if (headp == NULL || headp->q_back == headp) {
		errno = ENOENT;
		return -1;
	}
	p = headp->q_back;
	remque(p);
	*data = p->q_data;
	free(p);
	return 0;
}

int
cl_peek_pos(struct cl_que * headp, struct cl_que **pos)
{
	if (headp == NULL || headp->q_forw == headp) {
		errno = ENOENT;
		return -1;
	}
	if (pos != NULL)
		*pos = headp->q_forw;
	return 0;
}

int
cl_peek(struct cl_que *headp, TYPE **data)
{
	struct	cl_que *pos;
	int		res;

	if ((res = cl_peek_pos(headp, &pos)) < 0)
		return res;
	if (data != NULL)
		*data = pos->q_data;
	return 0;
}

int
cl_tail_peek_pos(struct cl_que * headp, struct cl_que **pos)
{
	if (headp == NULL || headp->q_back == headp) {
		errno = ENOENT;
		return -1;
	}
	if (pos != NULL)
		*pos = headp->q_back;
	return 0;
}

int
cl_tail_peek(struct cl_que *headp, TYPE **data)
{
	struct	cl_que *pos;
	int		res;

	if ((res = cl_tail_peek_pos(headp, &pos)) < 0)
		return res;
	if (data != NULL)
		*data = pos->q_data;
	return 0;
}

static void
cl_ins_sort_nomalloc(struct cl_que *headp, struct cl_que *p,
	cl_comp_f comp_fun)
{
	struct	cl_que *q;

	for (q = CL_NEXT(headp); q != headp; q = CL_NEXT(q))
		if (comp_fun(CL_DATA(q), CL_DATA(p)) > 0)
			break;

	insque(p, q->q_back);
}


int 
cl_ins_sort(struct cl_que *headp, TYPE *data, cl_comp_f comp_fun)
{
	struct cl_que *p;

	p = malloc(sizeof(*p));
	if (p == NULL) {
		if (container_err_exit)
			err(1, "cl_ins_sort: malloc");
		warn("cl_ins_sort: malloc");
		return -1;
	}
	p->q_data = data;

	if (CL_EMPTY(headp))
		insque(p, headp);
	else
		cl_ins_sort_nomalloc(headp, p, comp_fun);

	return 0;
}

void
cl_ins_after_pos(struct cl_que *pos, struct cl_que **new_pos, TYPE *data)
{
	if (cl_push(pos, data) < 0)
		return;
	if (new_pos != NULL)
		*new_pos = CL_NEXT(pos);
}

int 
cl_find_pos(struct cl_que *headp, struct cl_que **pos,
	TYPE **data, cl_find_f is_data, TYPE *comp_obj)
{
	struct cl_que *q;

	for (q = CL_NEXT(headp); q != headp; q = CL_NEXT(q)) {
		if (is_data(CL_DATA(q), comp_obj)) {
			if (data != NULL)
				*data = CL_DATA(q);
			if (pos != NULL)
				*pos = q;
			return 0;
		}
	}
	errno = ENOENT;
	return -1;
}

int 
cl_find(struct cl_que *headp, TYPE **data, cl_find_f is_data, TYPE *comp_obj)
{
	return cl_find_pos(headp, NULL, data, is_data, comp_obj);
}

void
cl_map(struct cl_que *headp, cl_map_f map_fun, TYPE *arg_obj)
{
	struct cl_que *q;

	for (q = CL_NEXT(headp); q != headp; q = CL_NEXT(q))
		map_fun(CL_DATA(q), arg_obj);
}

void
cl_sort(struct cl_que *headp, cl_comp_f comp_fun)
{
	struct	cl_que *q, *p;

	if (CL_EMPTY(headp))
		return;

	q = CL_NEXT(headp);
	remque(headp);
	CL_NEXT(headp) = headp;
	CL_PREV(headp) = headp;
	while (CL_NOT_EMPTY(q)) {
		p = CL_NEXT(q);
		remque(q);
		cl_ins_sort_nomalloc(headp, q, comp_fun);
		q = p;
	}
	cl_ins_sort_nomalloc(headp, q, comp_fun);
}

int
cl_rem_pos(struct cl_que *q)
{
	if (q == CL_NEXT(q)) {
		errno = ENOENT;
		return -1;
	}
	remque(q);
	free(q);
	return 0;
}

int
cl_rem(struct cl_que *headp, TYPE **data, cl_find_f is_data, TYPE *comp_obj)
{
	struct cl_que *p;

	if (cl_find_pos(headp, &p, data, is_data, comp_obj) < 0)
		return -1;
	return cl_rem_pos(p);
}

#define FIFO_CHUNK 20

#define FIFO_DATA_SIZE(s)	((s) * sizeof(void *))

#define FIFO_COUNT(fifo)												\
		(((fifo)->fifo_start <= (fifo)->fifo_end) ?						\
			((fifo)->fifo_end - (fifo)->fifo_start) : 					\
			((fifo)->fifo_size - ((fifo)->fifo_start + (fifo)->fifo_end)))

static int
fifo_incr(struct fifo_list *f)
{
	void		**p;
	unsigned	size;

	size = f->fifo_size * 2;
	if ((p = realloc(f->fifo_data, FIFO_DATA_SIZE(size))) == NULL)
		return -1;
	f->fifo_data = p;
	if (f->fifo_start > f->fifo_end) {
		(void)memcpy(p + f->fifo_size, p, FIFO_DATA_SIZE(f->fifo_end));
		f->fifo_end += f->fifo_size;
	}
	f->fifo_size = size;
	return 0;
}

static void
fifo_decr(struct fifo_list *f)
{
	void		**p;
	unsigned	start, end, size;

	if (f->fifo_size == FIFO_CHUNK)
		return;
	start = f->fifo_start; end = f->fifo_end; size = f->fifo_size / 2;
	if ((p = malloc(FIFO_DATA_SIZE(size))) == NULL)
		return;
	if (f->fifo_start < f->fifo_end) {
		/* copy from start to end */
		(void)memcpy(p, f->fifo_data + start, FIFO_DATA_SIZE(end - start));

	} else if (f->fifo_start > f->fifo_end) {
		/* copy from start to end of buffer */
		(void)memcpy(p, f->fifo_data + start, FIFO_DATA_SIZE(size - start));

		/* copy from beginning of buffer to end */
		(void)memcpy(p + size - start, f->fifo_data, FIFO_DATA_SIZE(end));
	}
	start = 0; end = FIFO_COUNT(f);
	free(f->fifo_data);
	f->fifo_data = p; f->fifo_start = start;
	f->fifo_end = end; f->fifo_size = size;
}

struct fifo_list * 
fifo_init(void)
{
	struct	fifo_list *f;

	if ((f = calloc((size_t) 1, sizeof(*f))) == NULL) {
		if (container_err_exit)
			err(1, "fifo_init: malloc");
		warn("fifo_init: malloc");
		return NULL;
	}
	if ((f->fifo_data = malloc(FIFO_DATA_SIZE(FIFO_CHUNK))) == NULL) {
		if (container_err_exit)
			err(1, "fifo_init: malloc");
		warn("fifo_init: malloc");
		free(f);
		return NULL;
	}
	f->fifo_size = FIFO_CHUNK;
	return f;
}

struct fifo_list * 
fifo_clone(struct fifo_list *f)
{
	struct	fifo_list *fc;

	if ((fc = malloc(sizeof(*fc))) == NULL) {
		if (container_err_exit)
			err(1, "fifo_clone: malloc");
		warn("fifo_clone: malloc");
		return NULL;
	}
	(void)memcpy(fc, f, sizeof(*fc));
	if ((fc->fifo_data = malloc(FIFO_DATA_SIZE(f->fifo_size))) == NULL) {
		if (container_err_exit)
			err(1, "fifo_clone: malloc");
		warn("fifo_clone: malloc");
		free(fc);
		return NULL;
	}
	(void)memcpy(fc->fifo_data, f->fifo_data, FIFO_DATA_SIZE(f->fifo_size));
	return fc;
}

void
fifo_free(struct fifo_list *f, fifo_free_f free_fun)
{
	unsigned	i;

	if (f->fifo_start != f->fifo_end) {
		if (f->fifo_start < f->fifo_end) {
			for (i = f->fifo_start; i < f->fifo_end;)
				if (free_fun != NULL)
					free_fun(f->fifo_data[i++]);
		} else {
			for (i = f->fifo_start; i < f->fifo_size;)
				if (free_fun != NULL)
					free_fun(f->fifo_data[i++]);
			for (i = 0; i < f->fifo_end;)
				if (free_fun != NULL)
					free_fun(f->fifo_data[i++]);
		}
	}
	free(f->fifo_data);
	free(f);
}

int
fifo_push(struct fifo_list *f, TYPE *data)
{
	if (FIFO_COUNT(f) == (f->fifo_size - 1))
		if (fifo_incr(f) < 0)
			return -1;

	f->fifo_data[f->fifo_end] = data;
	f->fifo_end = (f->fifo_end + 1) % f->fifo_size;
	return 0;
}

int
fifo_pop(struct fifo_list *f, TYPE **data)
{
	if (f == NULL || f->fifo_start == f->fifo_end) {
		errno = ENOENT;
		return -1;
	}
	*data = f->fifo_data[f->fifo_start];
	f->fifo_start = (f->fifo_start + 1) % f->fifo_size;
	if (FIFO_COUNT(f) < ((f->fifo_size - 1) / 2))
		fifo_decr(f);
	return 0;
}

int
fifo_peek(struct fifo_list *f, TYPE **data)
{
	if (f == NULL || f->fifo_start == f->fifo_end) {
		errno = ENOENT;
		return -1;
	}
	if (data != NULL)
		*data = f->fifo_data[f->fifo_start];
	return 0;
}

unsigned
fifo_count(struct fifo_list *f)
{
	return FIFO_COUNT(f);
}

int
fifo_walk(struct fifo_list *f, ssize_t *pos, TYPE **data)
{
	if (*pos > f->fifo_size) {
		errno = ENOENT;
		return -1;
	}
	if (data != NULL)
		*data = f->fifo_data[(f->fifo_start + *pos) % f->fifo_size];
	(*pos)++;
	return 0;
}

int
fifo_walk_back(struct fifo_list *f, ssize_t *pos, TYPE **data)
{
	if (*pos < 0) {
		errno = ENOENT;
		return -1;
	}
	if (data != NULL)
		*data = f->fifo_data[(f->fifo_start + *pos) % f->fifo_size];
	(*pos)--;
	return 0;
}

/* XXX the following four functions are not tested, may not work */
int
fifo_find_peek(struct fifo_list *f, TYPE **data, unsigned *num,
	fifo_find_f find_fun, TYPE *comp_obj)
{
	unsigned	i, j, count;

	count = FIFO_COUNT(f);
	for (i = j = 0; j < count; j++, i = (i + 1) % f->fifo_size) {
		if (find_fun(f->fifo_data[i], comp_obj)) {
			if (data != NULL)
				*data = f->fifo_data[i];
			if (num != NULL)
				*num = j;
			return 0;
		}
	}
	errno = ENOENT;
	return -1;
}

int
fifo_find_rem(struct fifo_list *f, TYPE **data, unsigned *num,
	fifo_find_f find_fun, TYPE *comp_obj)
{
	unsigned	n;

	if (fifo_find_peek(f, data, &n, find_fun, comp_obj) < 0)
		return -1;
	if (fifo_num_rem(f, n, data) < 0)
		return -1;
	if (num != NULL)
		*num = n;
	return 0;
}

int
fifo_num_peek(struct fifo_list *f, unsigned num, TYPE **data)
{
	unsigned	count;

	count = FIFO_COUNT(f);
	if (num > count) {
		errno = ENOENT;
		return -1;
	}
	if (data != NULL)
		*data = f->fifo_data[(num + f->fifo_start) % f->fifo_size];
	return 0;
}

int
fifo_num_rem(struct fifo_list *f, unsigned num, TYPE **data)
{
	unsigned	count, pos, start, end;
	TYPE		**ldata;

	if (num == 0)
		return fifo_pop(f, data);

	count = FIFO_COUNT(f);
	if (num > count) {
		errno = ENOENT;
		return -1;
	}

	pos = (num + f->fifo_start) % f->fifo_size;
	start = f->fifo_start;
	end = f->fifo_end;
	ldata = f->fifo_data;
	
	if (start < pos) {
		(void)memmove(ldata + start + 1, ldata + start,
			FIFO_DATA_SIZE(pos - start));
		f->fifo_start++;
	} else {
		(void)memmove(ldata + pos, ldata + pos + 1, FIFO_DATA_SIZE(end - pos));
		f->fifo_end--;
	}
	if (FIFO_COUNT(f) < (f->fifo_size / 2))
		fifo_decr(f);
	return 0;
}



#define STACK_CHUNK 20

#define STACK_DATA_SIZE(s)	((s) * sizeof(void *))

static int
stack_incr(struct stack_list *s)
{
	void		**p;
	unsigned	size;

	size = s->stack_size * 2;
	if ((p = realloc(s->stack_data, STACK_DATA_SIZE(size))) == NULL)
		return -1;
	s->stack_data = p;
	s->stack_size = size;
	return 0;
}

static void
stack_decr(struct stack_list *s)
{
	void		**p;
	unsigned	size;

	if (s->stack_size == STACK_CHUNK)
		return;
	size = s->stack_size / 2;
	if ((p = realloc(s->stack_data, STACK_DATA_SIZE(size))) == NULL)
		return;
	s->stack_data = p;
}

struct stack_list *
stack_init(void)
{
	struct	stack_list *s;

	if ((s = calloc((size_t) 1, sizeof(*s))) == NULL) {
		if (container_err_exit)
			err(1, "stack_init: malloc");
		warn("stack_init: malloc");
		return NULL;
	}
	if ((s->stack_data = malloc(STACK_DATA_SIZE(STACK_CHUNK))) == NULL) {
		if (container_err_exit)
			err(1, "stack_init: malloc");
		warn("stack_init: malloc");
		free(s);
		return NULL;
	}
	s->stack_size = STACK_CHUNK;
	return s;
}

void
stack_free(struct stack_list *s, stack_free_f free_fun)
{
	unsigned	i;

	for (i = s->stack_end; i > 0; )
		if (free_fun != NULL)
			free_fun(s->stack_data[i++]);
	free(s->stack_data);
	free(s);
}

int
stack_push(struct stack_list *s, TYPE *data)
{
	if (s->stack_size == s->stack_end)
		if (stack_incr(s) < 0)
			return -1;
	s->stack_data[s->stack_end++] = data;
	return 0;
}

int
stack_pop(struct stack_list *s, TYPE **data)
{
	if (s == NULL || s->stack_end == 0) {
		errno = ENOENT;
		return -1;
	}

	*data = s->stack_data[--s->stack_end];
	if (s->stack_end < (s->stack_size / 2))
		stack_decr(s);
	return 0;
}

int
stack_peek(struct stack_list *s, TYPE **data)
{
	if (s == NULL || s->stack_end == 0) {
		errno = ENOENT;
		return -1;
	}
	if (data != NULL)
		*data = s->stack_data[s->stack_end - 1];
	return 0;
}

unsigned
stack_count(struct stack_list *s)
{
	return s->stack_end;
}

int
stack_walk(struct stack_list *s, ssize_t *pos, TYPE **data)
{
	/* The fifos data structure is the same */
	/* LINTED s */
	return fifo_walk((struct fifo_list *)s, pos, data);
}

int
stack_walk_back(struct stack_list *s, ssize_t *pos, TYPE **data)
{
	/* The fifos data structure is the same */
	/* LINTED s */
	return fifo_walk_back((struct fifo_list *)s, pos, data);
}



#define PAT_IS_NODE(node, leg) ((node)->pat_is_node & (leg + 1))
#define PAT_IS_LEAF(node, leg) (!PAT_IS_NODE(node, leg))

#define INT_BITS	32


#ifdef CONTAINER_DEBUG
static int
#else
static __inline int
#endif
bit_is_set(int bit_nr, const u_int8_t *bit_field)
{
	unsigned	num_bits, one;

	num_bits = 8;
	one = 1;
	/* LINTED, it's OK i know what I'm doing */
    return ((bit_field[bit_nr / num_bits] >> (bit_nr % num_bits)) & one);
}

#ifdef CONTAINER_DEBUG
static int
#else
static __inline int
#endif
lowest_diff_bit(const u_int8_t *n1, size_t l1, const u_int8_t *n2, size_t l2)
{
	int		bit_field;	/* XXX because of ffs declaration */
	int		diff_bit;
	u_int8_t	zeroes = 0;
	int		sl1, sl2;

	sl1 = l1;
	sl2 = l2;
	if (sl1 == 0) { sl1 = -1; n1 = &zeroes; }
	if (sl2 == 0) { sl2 = -1; n2 = &zeroes; }
	for (;;) {
		if (sl1 < 0 && sl2 < 0)
			errx(2, "lowest_diff_bit: keys are equal, this shouldn't happen");
		bit_field = *n1 ^ *n2;
		diff_bit = ffs(bit_field) - 1;
		if (diff_bit >= 0)
			return diff_bit;
		if (sl1 >= 0) { sl1--; n1++; } else { sl1 = -1; n1 = &zeroes; }
		if (sl2 >= 0) { sl2--; n2++; } else { sl2 = -1; n2 = &zeroes; }
	}
}

static int
pat_keys_match(const u_int8_t *k1, size_t l1, const u_int8_t *k2, size_t l2)
{
	u_int8_t zeroes = 0;

	if (l1 == 0)
		k1 = &zeroes;
	else if (l2 == 0)
		k2 = &zeroes;
	for (;;) {
		if (l1 == 0 && l2 == 0)
			break;
		if (*k1 != *k2)
			return 0;
		if (l1 != 0) { l1--; k1++; } else k1 = &zeroes;
		if (l2 != 0) { l2--; k2++; } else k2 = &zeroes;
	}
	return 1;
}


static __inline void
pat_make_node(struct pat_node_s *node, int leg, struct pat_node_s *child)
{
	node->pat_node[leg] = child;
	node->pat_is_node |= leg + 1; /* mark it as a node */
}

static __inline void
pat_make_leaf(struct pat_node_s *node, int leg,
	const void *key, size_t key_len, TYPE *data)
{
	node->pat_key[leg] = key;
	node->pat_key_len[leg] = key_len;
	node->pat_data[leg] = data;
	node->pat_is_node &= ~(leg + 1); /* mark it as a leaf */
}


int
pat_ins(struct pat_tree *pat, const void *key, size_t key_len, TYPE *data)
{
	struct	pat_node_s *p;
	struct	pat_node_s **p_list;
	int		i, j;
	int		bit_set, node_set, diff_bit;

	/* Stop the walk */
	if (pat->pat_walk_stack != NULL)
		stack_free(pat->pat_walk_stack, NULL);
/*** to few elements ***/
	if (pat->pat_keys < 2) {

		/* we allready have one element */
		if (pat->pat_keys == 1) {
			if (pat_keys_match(pat->pat_key, (unsigned)pat->pat_key_len,
				key, (unsigned)key_len)) {
				errno = EEXIST;
				return -1;
			}
			pat->pat_root = malloc(sizeof(struct pat_node_s));
			if (pat->pat_root == NULL) {
				if (container_err_exit)
					err(1, "pat_ins: malloc");
				warn("pat_ins: malloc");
				return -1;
			}
			diff_bit = lowest_diff_bit(pat->pat_key, (unsigned)pat->pat_key_len,
				key, (unsigned)key_len);
			bit_set = bit_is_set(diff_bit, key);

			pat_make_leaf(pat->pat_root, bit_set, key, key_len, data);

			/* copy root element in to the other leaf */
			pat_make_leaf(pat->pat_root, !bit_set,
				pat->pat_key, (unsigned)pat->pat_key_len, pat->pat_data);

			pat->pat_keys = 2;
			pat->pat_key_len_max = 1;
			return 0;
		}

		/* this is the first element */
		pat->pat_key = key;
		pat->pat_key_len = key_len;
		pat->pat_data = data;
		pat->pat_keys = 1;
		pat->pat_key_len_max = 1;
		return 0;
	}


/*** common case ***/
	pat->pat_key_len_max = pat->pat_keys;	/* XXX too lazy to fix this */
	if ((p_list = alloca((size_t)pat->pat_key_len_max * sizeof(*p_list)))
		== NULL)
		err(1, "alloca");

	/*
	 * Walk down the tree on the matching path, record
	 * the path for later use.
	 */
	p = pat->pat_root;
	for (i = 0;; i++) {
		p_list[i] = p;
		node_set = bit_is_set(p->pat_bit, key);
		if (PAT_IS_LEAF(p, node_set)) {
			if (pat_keys_match(p->pat_key[node_set],
				(unsigned)p->pat_key_len[node_set],
				key, key_len)) {
				errno = EEXIST;
				return -1;
			}
			break;
		}
		p = p->pat_node[node_set];
	}


	/*
	 * Find the differing compared to found leaf. Locate at which
	 * node the element should be inserted.
	 */
	diff_bit = lowest_diff_bit(
		p->pat_key[node_set], (unsigned)p->pat_key_len[node_set], key, key_len);
	for (j = i; j > 0 && p_list[j]->pat_bit > diff_bit; j--);

	if ((p = malloc(sizeof(*p))) == NULL) {
		if (container_err_exit)
			err(1, "pat_ins: malloc");
		warn("pat_ins: malloc");
		return -1;
	}
	pat->pat_keys++;
	p->pat_bit = diff_bit;

	/* The root is to be replaced */
	if (j == 0 && pat->pat_root->pat_bit > diff_bit) {
		/* which leg does old root sort under? */
		bit_set = bit_is_set(diff_bit, key);

		/* put old root under p->pat_node[!bit_set] */
		pat_make_node(p, !bit_set, pat->pat_root);

		/* put key and data in other leg */
		pat_make_leaf(p, bit_set, key, key_len, data);

		/* replace root */
		pat->pat_root = p;
		return 0;
	}

	/* put our new node under p_list[j] */

	node_set = bit_is_set(p_list[j]->pat_bit, key);
	bit_set = bit_is_set(diff_bit, key);

	/* put data in on of p's legs */
	pat_make_leaf(p, bit_set, key, key_len, data);

	/* put what's under p_list[j][node_set] under p's other leg */
	if (PAT_IS_LEAF(p_list[j], node_set))
		pat_make_leaf(p, !bit_set,
				p_list[j]->pat_key[node_set],
				(unsigned)p_list[j]->pat_key_len[node_set],
				p_list[j]->pat_data[node_set]);
	else
		pat_make_node(p, !bit_set, p_list[j]->pat_node[node_set]);

	pat_make_node(p_list[j], node_set, p);
	return 0;
}

struct pat_tree *
pat_init(void)
{
	struct	pat_tree *pat;

	if ((pat = calloc((size_t)1, sizeof(*pat))) == NULL) {
		if (container_err_exit)
			err(1, "pat_init: calloc");
		warn("pat_init: calloc");
		return NULL;
	}
	pat->pat_keys = 0;
	return pat;
}

static void
pat_free_helper(struct pat_node_s *n,
	pat_free_f free_fun_data, pat_free_key_f free_fun_key)
{
	int		i;

	for (i = 0; i < 2; i++) {
		if (PAT_IS_LEAF(n, i)) {
			if (free_fun_data != NULL)
				free_fun_data(n->pat_data[i]);
			if (free_fun_key != NULL)
				free_fun_key(&n->pat_key[i]);
		} else
			pat_free_helper(n->pat_node[i], free_fun_data, free_fun_key);
	}
	free(n);
}

void
pat_free(struct pat_tree *pat,
	pat_free_f free_fun_data, pat_free_key_f free_fun_key)
{
	const void *p[1];

	if (pat->pat_keys == 1) {
		if (free_fun_data != NULL)
			free_fun_data(pat->pat_data);
		if (free_fun_key != NULL) {
			p[0] = pat->pat_key;
			free_fun_key(&p[0]);	/* fool gcc */
		}
	} else if (pat->pat_keys != 0)
		pat_free_helper(pat->pat_root, free_fun_data, free_fun_key);

	free(pat);
}

static void
pat_map_helper(struct pat_node_s *n, pat_map_f map_fun, TYPE *arg_obj)
{
	int		i;

	for (i = 0; i < 2; i++) {
		if (PAT_IS_LEAF(n, i))
			map_fun(n->pat_key[i], (unsigned)n->pat_key_len[i],
				n->pat_data[i], arg_obj);
		else
			pat_map_helper(n->pat_node[i], map_fun, arg_obj);
	}
}

void
pat_map(struct pat_tree *pat, pat_map_f map_fun, TYPE *arg_obj)
{
	if (pat->pat_keys == 1)
		map_fun(pat->pat_key, (unsigned)pat->pat_key_len,
			pat->pat_data, arg_obj);
	else if (pat->pat_keys != 0)
		pat_map_helper(pat->pat_root, map_fun, arg_obj);
}

static int
pat_walk_helper(struct pat_tree *pat, int forw,
	struct pat_node_s **pos, TYPE **data)
{
	struct stack_list *s;
	struct	pat_node_s *p, *op;
	int		back;

	back = (forw + 1) % 2;

	if (pat->pat_keys == 0) {
		errno = ENOENT;
		return -1;
	}
	s = pat->pat_walk_stack;
	if (*pos == NULL) {
		if (s != NULL)
			stack_free(s, NULL);
		if ((s = stack_init()) == NULL) {
			warn("pat_walk: stack_init");
			return -1;
		}
		pat->pat_walk_stack = s;
		if (stack_push(s, pat->pat_root) < 0) {
			warn("pat_walk: stack_push");
			goto ret_bad;
		}
		pat->pat_walk_did_forw = 0;
	}
	if (stack_peek(s, (TYPE **)&p) < 0)
		goto ret_bad;

	/* Seek bottom of leg forw. Put the path on a stack. */
	if (pat->pat_walk_did_forw == 0) {
		while (PAT_IS_NODE(p, forw)) {
			p = p->pat_node[forw];
			if (stack_push(s, p) < 0) {
				warn("pat_walk: stack_push");
				goto ret_bad;
			}
		}
		pat->pat_walk_did_forw = 1;
		*pos = p;
		*data = p->pat_data[forw];
		return 0;
	}

	/* Are at bottom of leg forw, continue on leg back */

	/* Leg back is a leaf */
	if (PAT_IS_LEAF(p, back)) {
		*data = p->pat_data[back];
		if (stack_pop(s, (TYPE **)&op) < 0)
			return 0;
		while (stack_pop(s, (TYPE **)&p) == 0) {
			if (PAT_IS_LEAF(p, back) || op != p->pat_node[back]) {
				if (stack_push(s, (TYPE *)p) < 0)
					return -1;
				return 0;
			}
			op = p;
		}
		return 0;
	}

	/* Prepare for a decend to bottom on leg 0 of the new node */
	p = p->pat_node[back];
	pat->pat_walk_did_forw = 0;
	if (stack_push(s, p) < 0) {
		warn("pat_walk: stack_push");
		goto ret_bad;
	}
	return pat_walk_helper(pat, forw, pos, data);

ret_bad:
	if (s != NULL)
		stack_free(s, NULL);
	s = NULL;
	return -1;
}

int
pat_walk(struct pat_tree *pat, struct pat_node_s **pos, TYPE **data)
{
	return pat_walk_helper(pat, 1, pos, data);
}

int
pat_walk_back(struct pat_tree *pat, struct pat_node_s **pos, TYPE **data)
{
	return pat_walk_helper(pat, 0, pos, data);
}


int
pat_find(struct pat_tree *pat, const void *key, size_t key_len, TYPE **data)
{
	struct	pat_node_s *p;
	int		bit_set;

/****** too few elements ******/
	if (pat->pat_keys < 2) {
		if (pat->pat_keys == 0) {
			errno = ENOENT;
			return -1;
		}
		if (!pat_keys_match(pat->pat_key, (unsigned)pat->pat_key_len,
			key, key_len)) {
			errno = ENOENT;
			return -1;
		}
		if (data != NULL)
			*data = pat->pat_data;
		return 0;
	}

/******* common case *******/
	/*
	 * Walk down the tree until a leaf is found,
	 * if key matches the element is found.
	 */
	for (p = pat->pat_root;; ) {
		bit_set = bit_is_set(p->pat_bit, key);
		if (PAT_IS_LEAF(p, bit_set)) {
			if (pat_keys_match(p->pat_key[bit_set],
				(unsigned)p->pat_key_len[bit_set], key, key_len)) {
				if (data != NULL)
					*data = p->pat_data[bit_set];
				return 0;
			}
			errno = ENOENT;
			return -1;
		}
		p = p->pat_node[bit_set];
	}
}

int
pat_rem(struct pat_tree *pat, const void *key, size_t key_len, TYPE **data)
{
	struct	pat_node_s *p;
	struct	pat_node_s *last;
	int		bit_set;
	int		last_bit_set;

	if (pat == NULL) {
		errno = ENOENT;
		return -1;
	}
	/* Stop the walk */
	if (pat->pat_walk_stack != NULL)
		stack_free(pat->pat_walk_stack, NULL);

/****** too few elements ******/
	if (pat->pat_keys < 3) {
		int		i;

		switch(pat->pat_keys) {
		case 0:
			errno = ENOENT;
			return -1;
		case 1:
			if (!pat_keys_match(pat->pat_key, (unsigned)pat->pat_key_len,
				key, key_len)) {
				errno = ENOENT;
				return -1;
			}
			pat->pat_keys = 0;
			if (data != NULL)
				*data = pat->pat_data;
			return 0;
		case 2:
			for (i = 0; i < 2; i++) {
				if (pat_keys_match(key, key_len,
					pat->pat_root->pat_key[i],
					(unsigned)pat->pat_root->pat_key_len[i])) {
					pat->pat_key = pat->pat_root->pat_key[!i];
					pat->pat_key_len = pat->pat_root->pat_key_len[!i];
					pat->pat_data = pat->pat_root->pat_data[!i];
					pat->pat_keys = 1;
					if (data != NULL)
						*data = pat->pat_root->pat_data[i];
					free(pat->pat_root);
					return 0;
				}
			}
			errno = ENOENT;
			return -1;

		default:
			break;
		}
	}

/******* common case *******/

	last = NULL;
	last_bit_set = 0; /* quiet gcc */
	for (p = pat->pat_root;; ) {
		bit_set = bit_is_set(p->pat_bit, key);
		if (PAT_IS_LEAF(p, bit_set)) {
			if (!pat_keys_match(p->pat_key[bit_set],
				(unsigned)p->pat_key_len[bit_set], key, key_len)) {
				errno = ENOENT;
				return -1;
			}
			break;
		}
		last = p;
		last_bit_set = bit_set;
		p = p->pat_node[bit_set];
	}

	if (last == NULL) { /* p == pat->pat_root */
		pat->pat_root = p->pat_node[!bit_set];
	} else {
		if (PAT_IS_LEAF(p, !bit_set))
			pat_make_leaf(last, last_bit_set,
				p->pat_key[!bit_set], (unsigned)p->pat_key_len[!bit_set],
				p->pat_data[!bit_set]);
	else
		pat_make_node(last, last_bit_set, p->pat_node[!bit_set]);
	}
	if (data != NULL)
		*data = p->pat_data[bit_set];
	free(p);
	pat->pat_keys--;
	return 0;
}

unsigned
pat_count(struct pat_tree *pat, int *num)
{
	return pat->pat_keys;
}


#define HEAP_CHUNK 2
#define HEAP_PARENT(i)														\
	/* LINTED (no worries) */												\
	((int) ((i) >> 1))
#define HEAP_LEFT(i)	((int) ((i) << 1))
#define HEAP_RIGHT(i)	(((int) ((i) << 1) + 1))


struct heap_list *
heap_init(heap_comp_f comp_fun)
{
	struct	heap_list *h;

	if ((h = malloc(sizeof(*h))) == NULL) {
		if (container_err_exit)
			err(1, "heap_init: malloc");
		warn("heap_init: malloc");
		return NULL;
	}
	h->heap_data = malloc(HEAP_CHUNK * sizeof(*h->heap_data));
	if (h->heap_data == NULL) {
		free(h);
		if (container_err_exit)
			err(1, "heap_init: malloc");
		warn("heap_init: malloc");
		return NULL;
	}
	
	h->heap_size = HEAP_CHUNK;
	h->heap_keys = 0;
	h->heap_comp_fun = comp_fun;
	return h;
}

struct heap_list *
heap_clone(struct heap_list *h)
{
	struct	heap_list *hc;

	if (h == NULL) {
		errno = 0;
		return NULL;
	}
	if ((hc = malloc(sizeof(*hc))) == NULL) {
		if (container_err_exit)
			err(1, "heap_clone: malloc");
		warn("heap_clone: malloc");
		return NULL;
	}
	(void)memcpy(hc, h, sizeof(*hc));
	hc->heap_data = malloc(sizeof(*hc->heap_data) * hc->heap_size);
	if (hc->heap_data == NULL) {
		if (container_err_exit)
			err(1, "heap_clone: malloc");
		warn("heap_clone: malloc");
		free(hc);
		return NULL;
	}
	(void)memcpy(hc->heap_data, h->heap_data,
		sizeof(*hc->heap_data) * hc->heap_size);
	return hc;
}

void
heap_free(struct heap_list *h, heap_free_f free_fun)
{
	int		i;
	void	**p;

	p = h->heap_data;
	if (free_fun != NULL)
		for (i = 0; i < h->heap_keys; i++)
			free_fun(p[i]);
	free(p);
	free(h);
}

static int
heap_inc(struct heap_list *h)
{
	void		**p;
	unsigned	size;

	size = h->heap_size * 2;
	if ((p = realloc(h->heap_data, size * sizeof(*p))) == NULL)
		return -1;
	h->heap_data = p;
	h->heap_size = size;
	return 0;
}

static void
heap_decr(struct heap_list *h)
{
	void		**p;
	unsigned	size;

	if (h->heap_size == HEAP_CHUNK)
		return;

	size = h->heap_size / 2;
	if ((p = realloc(h->heap_data, size * sizeof(*p))) == NULL)
		return;
	h->heap_data = p;
	h->heap_size = size;
}

static void
heap_exchange(struct heap_list *h, int a, int b)
{
	void	**p, *tmp;

	p = h->heap_data;
	tmp = p[a];
	p[a] = p[b];
	p[b] = tmp;
}

static void
heap_heapify(struct heap_list *h, int i)
{
	int		l, r, largest;
	void	**p;

restart:
	l = HEAP_LEFT(i);
	r = HEAP_RIGHT(i);
	p = h->heap_data;

	if (l < h->heap_keys && (h->heap_comp_fun(p[l], p[i]) > 0))
		largest = l;
	else
		largest = i;
	if (r < h->heap_keys && h->heap_comp_fun(p[r], p[largest]) > 0)
		largest = r;
	if (largest != i) {
		heap_exchange(h, i, largest);
		i = largest;
		goto restart;
	}
}

int
heap_push(struct heap_list *h, TYPE *data)
{
	int		i;
	void	**p;

	if (h->heap_keys == h->heap_size)
		if (heap_inc(h) < 0)
			return -1;

	i = h->heap_keys;
	p = h->heap_data;
	while (i > 0 && h->heap_comp_fun(p[HEAP_PARENT(i)], data) < 0) {
		p[i] = p[HEAP_PARENT(i)];
		i = HEAP_PARENT(i);
	}
	p[i] = data;
	h->heap_keys++;
	return 0;
}

int
heap_pop(struct heap_list *h, TYPE **data)
{
	if (h == NULL || h->heap_keys == 0) {
		errno = ENOENT;
		return -1;
	}
	if (data != NULL)
		*data = h->heap_data[0];
	if (h->heap_keys < (h->heap_size / 2))
		heap_decr(h); 
	h->heap_data[0] = h->heap_data[--h->heap_keys];
	heap_heapify(h, 0);
	return 0;
}

int
heap_peek(struct heap_list *h, TYPE **data)
{
	if (h == NULL || h->heap_keys == 0) {
		errno = ENOENT;
		return -1;
	}
	if (data != NULL)
		*data = h->heap_data[0];
	return 0;
}

unsigned
heap_count(struct heap_list *h)
{
	return h->heap_keys;
}

int
heap_walk(struct heap_list *h, ssize_t *pos, TYPE **data)
{
	if (*pos > h->heap_size) {
		errno = ENOENT;
		return -1;
	}
	if (data != NULL)
		*data = h->heap_data[*pos];
	(*pos)++;
	return 0;
}

int
heap_walk_back(struct heap_list *h, ssize_t *pos, TYPE **data)
{
	if (*pos < 0) {
		errno = ENOENT;
		return -1;
	}
	if (data != NULL)
		*data = h->heap_data[*pos];
	(*pos)--;
	return 0;
}


struct hash_buf *
hash_init(size_t hash_size)
{
	struct	hash_buf *p;

	if ((p = malloc(sizeof(*p))) == NULL) {
		if (container_err_exit)
			err(1, "hash_init: malloc");
		warn("hash_init: malloc");
		return NULL;
	}
	if ((p->hash_data = calloc(hash_size, sizeof(*p->hash_data))) == NULL) {
		free(p);
		if (container_err_exit)
			err(1, "hash_init: malloc");
		warn("hash_init: malloc");
		return NULL;
	}
	p->hash_size = (u_int32_t)hash_size;
	return p;
}

void
hash_map(struct hash_buf *p, hash_map_f map_fun, TYPE *arg_obj)
{
	struct	hash_data_s *he;
	int		i;

	for (i = 0; i < p->hash_size; i++) {
		he = p->hash_data + i;
		if (he->he_data == NULL)
			continue;
		while (he != NULL) {
			map_fun(he->he_key, he->he_data, arg_obj);
			he = he->he_next;
		}
	}
}

struct hash_clone_helper {
	struct	hash_buf *hch_hb;
	int		hch_error;
};

static void
hash_clone_helper(u_int32_t key, TYPE *data, void *p)
{
	struct	hash_clone_helper *hch = p;

	if (hch->hch_error)
		return;
	if (hash_ins(hch->hch_hb, key, data) < 0) {
		hch->hch_error = 1;
		return;
	}
}

struct hash_buf *
hash_clone(struct hash_buf *p)
{
	struct	hash_buf *hb;
	struct	hash_clone_helper hch;
	int		oerrno;

	if ((hb = hash_init((size_t)p->hash_size)) == NULL)
		return NULL;
	hch.hch_error = 0;
	hch.hch_hb = hb;
	hash_map(p, hash_clone_helper, &hch);
	if (hch.hch_error) {
		oerrno = errno;
		hash_free(hb, NULL);
		errno = oerrno;
		return NULL;
	}
	return hb;
}

void
hash_free(struct hash_buf *p, hash_free_f free_fun)
{
	struct		hash_data_s *he, *q;
	u_int32_t	i;

	for (i = 0; i < p->hash_size; i++) {
		he = p->hash_data + i;
		if (he->he_data == NULL)
			continue;
		q = he;
		he = he->he_next;
		if (free_fun != NULL)
			free_fun(q->he_data);
		while (he != NULL) {
			q = he;
			he = he->he_next;
			if (free_fun != NULL)
				free_fun(q->he_data);
			free(q);
		}
	}
	free(p->hash_data);
	free(p);
}

size_t
hash_count(struct hash_buf *p)
{
	size_t	i, datas;
	struct	hash_data_s *he;

	for (i = 0, datas = 0; i < p->hash_size; i++) {
		he = p->hash_data + i;
		if (he->he_data == NULL)
			continue;
		datas++;
		he = he->he_next;
		while (he != NULL) {
			datas++;
			he = he->he_next;
		}
	}
	return datas;
}

int
hash_ins(struct hash_buf *p, u_int32_t key, TYPE *data)
{
	struct		hash_data_s *he, *q;
	u_int32_t	k;

	k = key % p->hash_size;
	if (p->hash_data[k].he_data == NULL) {
		p->hash_data[k].he_data = data;
		p->hash_data[k].he_key = key;
		return 0;
	}
	if ((he = calloc((size_t)1, sizeof(*he))) == NULL) {
		if (container_err_exit)
			err(1, "hash_ins: malloc");
		warn("hash_ins: malloc");
		return -1;
	}
	for (q = p->hash_data + k; q->he_next != NULL; q = q->he_next);
	q->he_next = he;
	he->he_data = data;
	he->he_key = key;
	return 0;
}

int
hash_find(struct hash_buf *p, u_int32_t key, TYPE **data)
{
	struct		hash_data_s *he;
	u_int32_t	k;

	k = key % p->hash_size;
	if (p->hash_data[k].he_data == NULL)		/* not found */
		return -1;

	he = p->hash_data + k;
	while (he != NULL) {
		if (he->he_key == key) {
			*data = he->he_data;
			return 0;
		}
		he = he->he_next;
	}
	errno = ENOENT;
	return -1;
}

int
hash_rem(struct hash_buf *p, u_int32_t key, TYPE **data)
{
	struct		hash_data_s *he, *q, *he_last;
	u_int32_t	k;

	k = key % p->hash_size;
	if (p->hash_data[k].he_data == NULL)		/* not found */
		return -1;

	he = p->hash_data + k;
	he_last = NULL; /* get rid of lint warning */
	while (he != NULL) {
		if (he->he_key == key) {			/* found it */
			*data = he->he_data;
			if (he == p->hash_data + k) {
				if (he->he_next != NULL) {	/* move up the folowing element	*/
					he->he_data = he->he_next->he_data;
					he->he_key = he->he_next->he_key;
					q = he->he_next;
					he->he_next = he->he_next->he_next;
					free(q);
				}
			} else {
				he_last->he_next = he->he_next;
				free(he);
			}
			return 0;
		}
		he_last = he;
		he = he->he_next;
	}
	return -1;

}

/*
 * Skip Lists
 *
 *Refer to William Pugh's paper Skip Lists: A Probabalistic
 *Alternative to Balanced Trees
 *
 */

/* used by Skip Lists as a terminator of the list */
static struct skip_list_node sl_NIL_space = {NULL, 0xffffffff, {NULL}};
static struct skip_list_node *sl_NIL = &sl_NIL_space;

static struct skip_list_node *
sl_new_node(int level, u_int32_t key, TYPE *data)
{
	struct	skip_list_node *sln;
	size_t	size;

	
	size = sizeof(struct skip_list_node) + level * sizeof(void *);
	if ((sln = malloc(size)) == NULL) {
		if (container_err_exit)
			err(1, "sl_init: malloc");
		warn("sl_new_node: malloc");
		return NULL;
	}
	sln->sln_key = key;
	sln->sln_data = data;
	return sln;
}

struct skip_list *
sl_init(void)
{
	struct	skip_list *sl;
	int		i;

	if ((sl = malloc(sizeof(struct skip_list))) == NULL) {
		if (container_err_exit)
			err(1, "sl_init: malloc");
		warn("sl_init: malloc");
		return NULL;
	}
	sl->sl_level = 0;
	sl->sl_random_bits = (int)random();
	sl->sl_randoms_left = SL_BITS_IN_RANDOM / 2;
	if ((sl->sl_head = sl_new_node(SL_MAX_LEVELS, 0, NULL)) == NULL) {
		free(sl);
		return NULL;
	}
	for (i = 0; i < SL_MAX_LEVELS; i++)
		sl->sl_head->sln_forw[i] = sl_NIL;
	return sl;
}

void
sl_free(struct skip_list *sl, sl_free_f free_fun)
{
	struct	skip_list_node *p, *q;

	p = sl->sl_head;
	do {
		q = p->sln_forw[0];
		if (free_fun != NULL && p->sln_data != NULL)
			free_fun(p->sln_data);
		free(p);
		p = q;
	} while (p != sl_NIL);
	free(sl);
}

void
sl_find_first_pos(struct skip_list *sl,
	struct skip_list_node **pos, TYPE **data)
{
	if (pos != NULL)
		*pos = sl->sl_head->sln_forw[0];
	if (data != NULL)
		*data = sl->sl_head->sln_forw[0]->sln_data;
}

int
sl_walk(struct skip_list *sl, struct skip_list_node **pos, TYPE **data)
{
	if (pos == NULL)
		return 0;
	if ((*pos)->sln_forw[0] == sl_NIL) {
		errno = ENOENT;
		return -1;
	}
	*pos = (*pos)->sln_forw[0];
	if (data != NULL)
		*data = (*pos)->sln_data;
	return 0;
}

int
sl_find_pos(struct skip_list *sl, u_int32_t key,
	struct skip_list_node **pos, TYPE **data)
{
	int		i;
	struct	skip_list_node *p;

	p = sl->sl_head;
	for (i = sl->sl_level; i >= 0; i--) {
		for (; p->sln_forw[i]->sln_key < key; p = p->sln_forw[i]);
	}
	p = p->sln_forw[0];
	if (p->sln_key != key) {
		errno = ENOENT;
		return -1;
	}
	if (pos != NULL)
		*pos = p;
	if (data != NULL)
		*data = p->sln_data;
	return 0;
}

int
sl_find(struct skip_list *sl, u_int32_t key, TYPE **data)
{
	struct skip_list_node *p;

	if (sl_find_pos(sl, key, &p, data) < 0)
		return -1;
	return 0;
}


/* Helper to sl_ins() and sl_rem() */

static int
sl_random_level(struct skip_list *sl)
{
	int b, level;

	level = 0;
	do {
		b = sl->sl_random_bits & 3;
		if (b == 0)
			level++;
		sl->sl_random_bits >>= 2;
		if (--sl->sl_randoms_left == 0) {
			sl->sl_random_bits = (int)random();
			sl->sl_randoms_left = SL_BITS_IN_RANDOM / 2;
		}
	} while (b != 0);
	if (level > SL_MAX_LEVEL)
		return SL_MAX_LEVEL;
	return level;
}


int
sl_ins(struct skip_list *sl, u_int32_t key, TYPE *data)
{
	struct	skip_list_node *update[SL_MAX_LEVELS], *p;
	int		i, new_level, level;

	/* Locate element right before the locatation of insert.
	 * Keep track of all pointer that might point to the new
	 * element.
	 */
	p = sl->sl_head;
	level = sl->sl_level;
	for (i = level; i >= 0; i--) {
		for (; p->sln_forw[i]->sln_key < key; p = p->sln_forw[i]);
		update[i] = p;
	}

	/* Determine the new level and keep track of new pointers
	 * if necessary.
	 */
	new_level = sl_random_level(sl);
	if (new_level > level) {
		for (i = level + 1; i <= new_level; i++)
			update[i] = sl->sl_head;
		sl->sl_level = level = new_level;
	}

	/* create node */
	if ((p = sl_new_node(new_level, key, data)) == NULL)
		return -1;

	/* splice in out element and point that elements pointer up
	 * to new_level to what the recorded pointer pointed to.
	 * Point all recorded pointers up to new_level to the new
	 * element.
	 */
	for (i = 0; i <= new_level; i++) {
		p->sln_forw[i] = update[i]->sln_forw[i];
		update[i]->sln_forw[i] = p;
	}
	return 0;
}

int
sl_rem(struct skip_list *sl, u_int32_t key, TYPE **data)
{
	struct	skip_list_node *update[SL_MAX_LEVELS], *p;
	int		i, level;

	/* Locate element to be removed.
	 * Keep track of all pointer that might point to the new
	 * element.
	 */
	level = sl->sl_level;
	p = sl->sl_head;
	for (i = level; i >= 0; i--) {
		for (; p->sln_forw[i]->sln_key < key; p = p->sln_forw[i]);
		update[i] = p;
	}
	p = p->sln_forw[0];
	if (p->sln_key != key)
		return -1;

	/* Remove the found element from the list by:
	 * Point pointer pointing to the found element to
	 * what the pointers of the found element points to
	 */
	for (i = 0; i <= level; i++) {
		if (update[i]->sln_forw[i] != p)
			break;
		update[i]->sln_forw[i] = p->sln_forw[i];
	}
	if (data != NULL)
		*data = p->sln_data;
	free(p);

	/* Record the new level of the list */
	p = sl->sl_head;
	for (; level >= 0 && p->sln_forw[level] == sl_NIL; level--);
	sl->sl_level = level;
	return 0;
}
