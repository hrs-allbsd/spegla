/* $Id: container.h,v 1.17 2000/05/14 14:39:39 jens Exp $ */
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

#ifndef CONTAINER_H
#define CONTAINER_H

#include <sys/types.h>

/* Used when compiling spegla for ultrix */
#ifdef MTYPES
#include "missing/defs.h"
#endif

#ifdef MISSINGTYPES
typedef unsigned char            u_int8_t;
typedef unsigned short          u_int16_t;
typedef unsigned int            u_int32_t;
typedef unsigned long long      u_int64_t;
#endif

#ifdef MEMDEBUG
#include <memdebug.h>
#endif

extern int container_err_exit;

typedef void TYPE;


/****** CL_QUE ******/

/*
 * cl_que is implemented as a cirular linked list
 */

struct cl_que {
	struct cl_que *q_forw;
	struct cl_que *q_back;
	TYPE   *q_data;
};

#define CL_NEXT(q)		((q)->q_forw)
#define CL_PREV(q)		((q)->q_back)
#define CL_DATA(q)		((q)->q_data)
#define CL_EMPTY(q)		(CL_NEXT(q) == (q))
#define CL_NOT_EMPTY(q)	(!CL_EMPTY(q))

 /* Returns result as strcmp */
typedef int (*cl_comp_f) (TYPE *, TYPE *);

 /* Return 1 if match */
typedef int (*cl_find_f) (TYPE *, TYPE *);

 /* Used by cl_map */
typedef void (*cl_map_f) (TYPE *, TYPE *);

 /* Frees an element*/
typedef void (*cl_free_f) (TYPE *);

 /* Inits the list */
struct cl_que *cl_init(void);

 /* Frees the list */
void cl_free(struct cl_que *, cl_free_f);

unsigned cl_count(struct cl_que *);

 /* Treat the list as a stack */
int cl_push(struct cl_que *, TYPE *data);
int cl_tail_push(struct cl_que *, TYPE *data);
int cl_pop(struct cl_que *, TYPE **data);
int cl_tail_pop(struct cl_que *, TYPE **data);

 /* Peek first/last element in list */
int cl_peek(struct cl_que *, TYPE **data);
int cl_tail_peek(struct cl_que *, TYPE **data);

 /* Peek first/last pos in list */
int cl_peek_pos(struct cl_que *, struct cl_que **pos);
int cl_tail_peek_pos(struct cl_que *, struct cl_que **pos);

 /* Walk forw/back in list */
int cl_walk(struct cl_que *, struct cl_que **pos, TYPE **data);
int cl_walk_back(struct cl_que *, struct cl_que **pos, TYPE **data);

 /* Insert in a sorted way (like insertsort) */
int cl_ins_sort(struct cl_que *, TYPE *data, cl_comp_f comp_fun);

/* Sort list with insert sort */
void cl_sort(struct cl_que *, cl_comp_f comp_fun);

 /* Insert after pos, where pos is one of teh containers in list */
void cl_ins_after_pos(struct cl_que *pos, struct cl_que **new_pos, TYPE *data);

 /* Operate map_fun on every element */
void cl_map(struct cl_que *, cl_map_f map_fun, TYPE *arg_obj);


 /* Locate element in list, comp_data is given as the last argument for
  * find_fun */
int cl_find(struct cl_que *, TYPE **data, cl_find_f find_fun, TYPE *comp_obj);

 /* Locate element in list and get a pointer to element and container
  * for element */
int cl_find_pos(struct cl_que *, struct cl_que ** pos,
    TYPE **data, cl_find_f find_fun, TYPE *comp_obj);

 /* Locate and remove element from list */
int cl_rem(struct cl_que *, TYPE **data, cl_find_f find_fun, TYPE *comp_obj);

 /* Remove pos gotten with cl_find_pos() from list */
int cl_rem_pos(struct cl_que *);


/***** FIFO_LIST ******/

struct fifo_list {
	unsigned	fifo_start;
	unsigned	fifo_end;
	unsigned	fifo_size;
	TYPE		**fifo_data;
};

 /* Frees an element*/
typedef void (*fifo_free_f) (TYPE *);

/* Return 1 if match */
typedef int (*fifo_find_f) (TYPE *, TYPE *);

/* Inits the fifo */
struct fifo_list * fifo_init(void);
struct fifo_list * fifo_clone(struct fifo_list *);

/* Frees the fifo */
void fifo_free(struct fifo_list *, fifo_free_f);

int fifo_push(struct fifo_list *, TYPE *data);
int fifo_pop(struct fifo_list *, TYPE **data);
int fifo_peek(struct fifo_list *, TYPE **data);
unsigned fifo_count(struct fifo_list *);

 /* Walk forw/back in list */
int fifo_walk(struct fifo_list *, ssize_t *pos, TYPE **data);
int fifo_walk_back(struct fifo_list *, ssize_t *pos, TYPE **data);

/* XXX - these four functions are not tested may not work */
int fifo_find_peek(struct fifo_list *, TYPE **data, unsigned *num,
	fifo_find_f, TYPE *comp_obj);

int fifo_find_rem(struct fifo_list *, TYPE **data, unsigned *num,
	fifo_find_f, TYPE *comp_obj);

int fifo_num_peek(struct fifo_list *, unsigned num, TYPE **data);
int fifo_num_rem(struct fifo_list *, unsigned num, TYPE **data);

/****** STACK_LIST ******/

struct stack_list {
	unsigned	stack_start;	/* compat with fifo_list */
	unsigned	stack_end;
	unsigned	stack_size;
	TYPE		**stack_data;
};

 /* Frees an element*/
typedef void (*stack_free_f) (TYPE *);

/* Inits the stack */
struct stack_list * stack_init(void);

/* Frees the stack */
void stack_free(struct stack_list *, stack_free_f);

int stack_push(struct stack_list *, TYPE *data);
int stack_pop(struct stack_list *, TYPE **data);
int stack_peek(struct stack_list *, TYPE **data);
unsigned stack_count(struct stack_list *);

 /* Walk forw/back in list */
int stack_walk(struct stack_list *, ssize_t *pos, TYPE **data);
int stack_walk_back(struct stack_list *, ssize_t *pos, TYPE **data);

/****** PAT_BUF ******/

struct pat_node_s {
	u_int16_t	pat_bit;
	u_int16_t	pat_is_node;
	const void	*pat_key[2];
	u_int16_t	pat_key_len[2];
	struct		pat_node_s *pat_node[2];
	TYPE		*pat_data[2];
};
#if 0
#define pat_node pat_patu.patu_node
#define pat_data pat_patu.patu_data
#endif

struct pat_tree {
	u_int32_t	pat_keys;
	struct		pat_node_s *pat_root;
	const void	*pat_key;
	u_int16_t	pat_key_len;
	u_int16_t	pat_key_len_max;
	TYPE		*pat_data;
	struct		stack_list *pat_walk_stack;
	int			pat_walk_did_forw;
};

typedef void (*pat_map_f) (const void *, size_t, TYPE *, const void *);
typedef void (*pat_free_f) (TYPE *);
typedef void (*pat_free_key_f) (void *);

struct pat_tree * pat_init(void);
void pat_free(struct pat_tree *, pat_free_f, pat_free_key_f);
unsigned pat_count(struct pat_tree *, int *num);
int pat_ins(struct pat_tree *, const void *key, size_t key_len, TYPE *data);
int pat_find(struct pat_tree *, const void *key, size_t key_len, TYPE **data);
int pat_rem(struct pat_tree *, const void *key, size_t key_len,  TYPE **data);
void pat_map(struct pat_tree *, pat_map_f map_fun, TYPE *arg_obj);
int pat_walk(struct pat_tree *, struct pat_node_s **pos, TYPE **data);
int pat_walk_back(struct pat_tree *, struct pat_node_s **pos, TYPE **data);

/****** HEAP_LIST ******/

typedef void (*heap_free_f) (TYPE *);
typedef int (*heap_comp_f) (TYPE *, TYPE *);

struct heap_list {
	u_int32_t	heap_size;
	u_int32_t	heap_keys;
	heap_comp_f	heap_comp_fun;
	TYPE		**heap_data;
};


struct heap_list * heap_init(heap_comp_f comp_fun);
struct heap_list * heap_clone(struct heap_list *hl);
void heap_free(struct heap_list *hl, heap_free_f free_fun);
int heap_push(struct heap_list *hl, TYPE *data);
int heap_pop(struct heap_list *hl, TYPE **data);
int heap_peek(struct heap_list *hl, TYPE **data);
unsigned heap_count(struct heap_list *hl);
void heap_print(struct heap_list *hl);
int heap_walk(struct heap_list *, ssize_t *pos, TYPE **data);
int heap_walk_back(struct heap_list *, ssize_t *pos, TYPE **data);


/****** HASH_BUF ******/

typedef void (*hash_map_f) (u_int32_t, TYPE *, TYPE *);
typedef void (*hash_free_f) (TYPE *);


struct hash_data_s {
	TYPE		*he_data;
	u_int32_t	he_key;
	struct		hash_data_s *he_next;
};

struct hash_buf {
	u_int32_t	hash_size;
	struct		hash_data_s *hash_data;
};

struct hash_buf * hash_init(size_t hash_size);
struct hash_buf * hash_clone(struct hash_buf *);
void hash_free(struct hash_buf *, hash_free_f);
size_t hash_count(struct hash_buf *);
int hash_ins(struct hash_buf *, u_int32_t key, TYPE *data);
int hash_find(struct hash_buf *, u_int32_t key, TYPE **data);
void hash_map(struct hash_buf *, hash_map_f map_fun, TYPE *arg_obj);
int hash_rem(struct hash_buf *, u_int32_t key, TYPE **data);

/****** SKIP_LIST ******/

#define SL_MAX_LEVELS		16
#define SL_MAX_LEVEL		15
#define	SL_BITS_IN_RANDOM	31

typedef void (*sl_map_f) (u_int32_t, TYPE *, void *);
typedef void (*sl_free_f) (TYPE *);

struct skip_list_node {
	TYPE		*sln_data;
	u_int32_t	sln_key;
	struct		skip_list_node *sln_forw[1];	/* variable sized array of	*/
												/* forward pointers			*/
};

struct skip_list {
	struct		skip_list_node *sl_head;
	u_int32_t	sl_level;					/* Current level of the list	*/
	u_int32_t	sl_random_bits;				/* Used by sl_random_level		*/
	u_int32_t	sl_randoms_left;			/* Used by sl_random_level		*/
};

struct skip_list * sl_init(void);
void sl_free(struct skip_list *, sl_free_f);
int sl_ins(struct skip_list *, u_int32_t key, TYPE *data);
int sl_find_pos(struct skip_list *, u_int32_t key,
	struct skip_list_node **pos, TYPE **data);
void sl_find_first_pos(struct skip_list *,
	struct skip_list_node **pos, TYPE **data);
int sl_walk(struct skip_list *, struct skip_list_node **pos, TYPE **data);
int sl_find(struct skip_list *, u_int32_t key, TYPE **data);
int sl_rem(struct skip_list *, u_int32_t key, TYPE **data);

/****************** TYPE SAFE FUNCTIONS *****************************/

#if defined(__STDC__) || defined(__cplusplus)
#	define CC_2(a,b)		a ## b
#	define CC_3(a,b,c)		a ## b ## c
#	define CC_4(a,b,c,d)	a ## b ## c ## d
#	define CC_5(a,b,c,d,e)	a ## b ## c ## d ## e
#else
#	define CC_2(a,b) a/**/b
#	define CC_3(a,b,c) a/**/b/**/c
#	define CC_4(a,b,c,d) a/**/b/**/c/**/d
#	define CC_5(a,b,c,d,e) a/**/b/**/c/**/d/**/e
#endif

#if 1
#define USE_INLINE
#endif

#if defined(USE_INLINE) && !defined(lint)
#define DEF_F_DECL(NAME,PRE,f_name,ret_t,arg,arg2)					\
static __inline ret_t CC_5(PRE,_,NAME,_,f_name)arg					\
{																	\
	return (ret_t)CC_3(PRE,_,f_name)arg2;							\
}

#define DEF_F_SYM(NAME,PRE,f_name,ret_t,arg,arg2)

#else
#define DEF_F_DECL(NAME,PRE,f_name,ret_t,arg,arg2)					\
	extern ret_t (*CC_5(PRE,_,NAME,_,f_name))arg;

#define DEF_F_SYM(NAME,PRE,f_name,ret_t,arg,arg2)					\
	ret_t (*CC_5(PRE,_,NAME,_,f_name))arg							\
	= (ret_t (*) arg) CC_3(PRE,_,f_name);
#endif


/****** CL_QUE ******/

#define CL_FUN_DEFS(DEF_F, NAME, TYPE)									\
DEF_F(NAME, cl,init,													\
	struct CC_3(cl_,NAME,_que) *,										\
	(void),																\
	() )																\
DEF_F(NAME, cl,free,													\
	void,																\
	(struct CC_3(cl_,NAME,_que) *cl, CC_3(cl_,NAME,_free_f) free_fun),	\
	((struct cl_que *)cl, (cl_free_f)free_fun) )						\
DEF_F(NAME, cl,count,													\
	unsigned,															\
	(struct CC_3(cl_,NAME,_que) *cl),									\
	((struct cl_que*)cl) )												\
DEF_F(NAME, cl,push,													\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, TYPE *data),						\
	((struct cl_que*)cl, (void *)data) )									\
DEF_F(NAME, cl,tail_push,												\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, TYPE *data),						\
	((struct cl_que *)cl, (void *)data) )								\
DEF_F(NAME, cl,pop,														\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, TYPE **data),						\
	((struct cl_que *)cl, (void **)data) )								\
DEF_F(NAME, cl,tail_pop,												\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, TYPE **data),						\
	((struct cl_que *)cl, (void **)data) )								\
DEF_F(NAME, cl,peek_pos,												\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, struct CC_3(cl_,NAME,_que) **data),	\
	((struct cl_que *)cl, (struct cl_que **)data) )						\
DEF_F(NAME, cl,peek,													\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, TYPE **data),						\
	((struct cl_que *)cl, (void **)data) )								\
DEF_F(NAME, cl,tail_peek_pos,											\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, struct CC_3(cl_,NAME,_que) **pos),	\
	((struct cl_que *)cl, (struct cl_que **)pos) )						\
DEF_F(NAME, cl,tail_peek,												\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, TYPE **data),						\
	((struct cl_que *)cl, (void **)data) )								\
DEF_F(NAME, cl,walk,													\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, struct CC_3(cl_,NAME,_que) **pos,	\
		TYPE **data),													\
	((struct cl_que *)cl, (struct cl_que **)pos, (void **)data) )		\
DEF_F(NAME, cl,walk_back,												\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, struct CC_3(cl_,NAME,_que) **pos,	\
		TYPE **data),													\
	((struct cl_que *)cl, (struct cl_que **)pos, (void **)data) )		\
DEF_F(NAME, cl,ins_sort,												\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, TYPE *data,							\
		CC_3(cl_,NAME,_comp_f) comp_fun),								\
	((struct cl_que *)cl, (void *)data, (cl_comp_f)comp_fun) )			\
DEF_F(NAME, cl,sort,													\
	void,																\
	(struct CC_3(cl_,NAME,_que) *cl, CC_3(cl_,NAME,_comp_f) comp_fun),	\
	((struct cl_que *)cl, (cl_comp_f)comp_fun) )						\
DEF_F(NAME, cl,ins_after_pos,											\
	void,																\
	(struct CC_3(cl_,NAME,_que) *cl, struct CC_3(cl_,NAME,_que) **pos,	\
		TYPE *data),														\
	((struct cl_que *)cl, (struct cl_que **)pos, (void *)data) )			\
DEF_F(NAME, cl,find,													\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, TYPE **data,						\
		CC_3(cl_,NAME,_find_f) find_fun, TYPE *comp_obj),				\
	((struct cl_que *)cl, (void **)data, (cl_find_f)find_fun, comp_obj) )\
DEF_F(NAME, cl,map,														\
	void,																\
	(struct CC_3(cl_,NAME,_que) *cl, CC_3(cl_,NAME,_map_f) map_fun,		\
		void *arg_obj),													\
	((struct cl_que *)cl, (cl_map_f)map_fun, arg_obj) )					\
DEF_F(NAME,cl,find_pos,													\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, struct CC_3(cl_,NAME,_que) **pos,	\
		TYPE **data, CC_3(cl_,NAME,_find_f) find_fun, void *comp_obj),	\
	((struct cl_que *)cl, (struct cl_que **) pos, (void **) data,		\
		(cl_find_f)find_fun, comp_obj) )								\
DEF_F(NAME, cl,rem,														\
	int,																\
	(struct CC_3(cl_,NAME,_que) *cl, TYPE **data,						\
		CC_3(cl_,NAME,_find_f) find_fun, void *comp_obj),				\
	((struct cl_que *)cl, (void **)data, (cl_find_f)find_fun, comp_obj) )\
DEF_F(NAME, cl,rem_pos,													\
	int,																\
	(struct CC_3(cl_,NAME,_que) *pos),									\
	((struct cl_que *)pos) )											\

#define CL_STRUCT_DEFS(NAME, TYPE)										\
struct CC_3(cl_,NAME,_que) {											\
	struct	CC_3(cl_,NAME,_que) *q_forw;								\
	struct	CC_3(cl_,NAME,_que) *q_back;								\
	TYPE	*q_data;														\
};

#define CL_TYPEDEF_DEFS(NAME, TYPE)										\
typedef int (* CC_3(cl_,NAME,_comp_f)) (TYPE *, TYPE *);				\
typedef int (* CC_3(cl_,NAME,_find_f)) (TYPE *, TYPE *);				\
typedef void (* CC_3(cl_,NAME,_free_f)) (TYPE *);						\
typedef void (* CC_3(cl_,NAME,_map_f)) (TYPE *);

/* to be used in h-file */
#define CL_TYPE(NAME,TYPE)												\
CL_STRUCT_DEFS(NAME,TYPE)												\
CL_TYPEDEF_DEFS(NAME,TYPE)												\
CL_FUN_DEFS(DEF_F_DECL,NAME,TYPE)


/* to be used que_syms.c, requires that corresponding					*
 * CL_TYPE is used first												*/
#define CL_SYMS(NAME,TYPE)												\
/* LINTLIBRARY */														\
CL_FUN_DEFS(DEF_F_SYM,NAME,TYPE)


/***** FIFO_LIST ******/

#define FIFO_FUN_DEFS(DEF_F, NAME, TYPE)								\
DEF_F(NAME, fifo,init,													\
	struct CC_3(fifo_,NAME,_list) *,									\
	(void),																\
	() )																\
DEF_F(NAME, fifo,clone,													\
	struct CC_3(fifo_,NAME,_list) *,									\
	(struct CC_3(fifo_,NAME,_list) *f),									\
	((struct fifo_list *)f) )											\
DEF_F(NAME, fifo,free,													\
	void,																\
	(struct CC_3(fifo_,NAME,_list) *f, CC_3(fifo_,NAME,_free_f) free_fun),\
	((struct fifo_list *)f, (fifo_free_f)free_fun) )					\
DEF_F(NAME, fifo,count,													\
	unsigned,															\
	(struct CC_3(fifo_,NAME,_list) *f),									\
	((struct fifo_list *)f) )											\
DEF_F(NAME, fifo,push,													\
	int,																\
	(struct CC_3(fifo_,NAME,_list) *f, TYPE *data),						\
	((struct fifo_list *)f, data) )										\
DEF_F(NAME, fifo,pop,													\
	int,																\
	(struct CC_3(fifo_,NAME,_list) *f, TYPE **data),					\
	((struct fifo_list *)f, (void **)data) )							\
DEF_F(NAME, fifo,peek,													\
	int,																\
	(struct CC_3(fifo_,NAME,_list) *f, TYPE **data),					\
	((struct fifo_list *)f, (void **)data) )							\
DEF_F(NAME, fifo,walk,													\
	int,																\
	(struct CC_3(fifo_,NAME,_list) *f, size_t *pos, TYPE **data),		\
	((struct fifo_list *)f, pos, (void **)data) )						\
DEF_F(NAME, fifo,walk_back,												\
	int,																\
	(struct CC_3(fifo_,NAME,_list) *f, size_t *pos, TYPE **data),		\
	((struct fifo_list *)f, pos, (void **)data) )						\

#define FIFO_STRUCT_DEFS(NAME, TYPE)									\
struct CC_3(fifo_,NAME,_list) {											\
	unsigned	fifo_start;												\
	unsigned	fifo_end;												\
	unsigned	fifo_size;												\
	TYPE		**fifo_data;											\
};

#define FIFO_TYPEDEF_DEFS(NAME, TYPE)									\
typedef void (* CC_3(fifo_,NAME,_free_f)) (TYPE *);

/* to be used in h-file */
#define FIFO_TYPE(NAME,TYPE)											\
FIFO_STRUCT_DEFS(NAME,TYPE)												\
FIFO_TYPEDEF_DEFS(NAME,TYPE)											\
FIFO_FUN_DEFS(DEF_F_DECL,NAME,TYPE)

/* to be used que_syms.c, requires that corresponding					*
 * FIFO_TYPE is used first												*/
#define FIFO_SYMS(NAME,TYPE)											\
/* LINTLIBRARY */														\
FIFO_FUN_DEFS(DEF_F_SYM,NAME,TYPE)

/****** STACK_LIST ******/

#define STACK_FUN_DEFS(DEF_F, NAME, TYPE)								\
DEF_F(NAME, stack,init,													\
	struct CC_3(stack_,NAME,_list) *,									\
	(void),																\
	() )																\
DEF_F(NAME, stack,free,													\
	void,																\
	(struct CC_3(stack_,NAME,_list) *s, CC_3(stack_,NAME,_free_f) free_fun),\
	((struct stack_list *)s, (stack_free_f)free_fun) )					\
DEF_F(NAME, stack,count,												\
	unsigned,															\
	(struct CC_3(stack_,NAME,_list) *s),								\
	((struct stack_list *)s) )											\
DEF_F(NAME, stack,push,													\
	int,																\
	(struct CC_3(stack_,NAME,_list) *s, TYPE *data),						\
	((struct stack_list *)s, (void *)data) )								\
DEF_F(NAME, stack,pop,													\
	int,																\
	(struct CC_3(stack_,NAME,_list) *s, TYPE **data),					\
	((struct stack_list *)s, (void **)data) )							\
DEF_F(NAME, stack,peek,													\
	int,																\
	(struct CC_3(stack_,NAME,_list) *s, TYPE **data),					\
	((struct stack_list *)s, (void **)data) )							\
DEF_F(NAME, stack,walk,													\
	int,																\
	(struct CC_3(stack_,NAME,_list) *s, size_t *pos, TYPE **data),		\
	((struct stack_list *)s, pos, (void **)data) )						\
DEF_F(NAME, stack,walk_back,											\
	int,																\
	(struct CC_3(stack_,NAME,_list) *s, size_t *pos, TYPE **data),		\
	((struct stack_list *)s, pos, (void **)data) )						\

#define STACK_STRUCT_DEFS(NAME, TYPE)									\
struct CC_3(stack_,NAME,_list) {										\
	struct	CC_3(stack_,NAME,_list) *q_forw;							\
	struct	CC_3(stack_,NAME,_list) *q_back;							\
	TYPE	*q_data;														\
};

#define STACK_TYPEDEF_DEFS(NAME, TYPE)									\
typedef void (* CC_3(stack_,NAME,_free_f)) (TYPE *);

/* to be used in h-file */
#define STACK_TYPE(NAME,TYPE)											\
STACK_STRUCT_DEFS(NAME,TYPE)											\
STACK_TYPEDEF_DEFS(NAME,TYPE)											\
STACK_FUN_DEFS(DEF_F_DECL,NAME,TYPE)

/* to be used que_syms.c, requires that corresponding					*
 * STACK_TYPE is used first												*/
#define STACK_SYMS(NAME,TYPE)											\
/* LINTLIBRARY */														\
STACK_FUN_DEFS(DEF_F_SYM,NAME,TYPE)


/****** PAT_BUF ******/

#define PAT_FUN_DEFS(DEF_F, NAME, TYPE, KEY_TYPE)						\
DEF_F(NAME, pat,init,													\
	struct CC_3(pat_,NAME,_tree) *,										\
	(void),																\
	() )																\
DEF_F(NAME, pat,free,													\
	void,																\
	(struct CC_3(pat_,NAME,_tree) *p, CC_3(pat_,NAME,_free_f) free_fun_data,\
		CC_3(pat_,NAME,_free_key_f) free_fun_key),						\
	((struct pat_tree *)p, (pat_free_f)free_fun_data,					\
		(pat_free_key_f)free_fun_key) )									\
DEF_F(NAME, pat,count,													\
	unsigned,															\
	(struct CC_3(pat_,NAME,_tree) *p, int *num),						\
	((struct pat_tree *)p, num) )										\
DEF_F(NAME, pat,ins,													\
	int,																\
	(struct CC_3(pat_,NAME,_tree) *p, KEY_TYPE *key, u_int8_t key_len,	\
		TYPE *data),														\
	((struct pat_tree *)p, key, key_len, (void *)data) )					\
DEF_F(NAME, pat,find,													\
	int,																\
	(struct CC_3(pat_,NAME,_tree) *p, KEY_TYPE *key, u_int8_t key_len,	\
		TYPE **data),													\
	((struct pat_tree *)p, key, key_len, (void **)data) )				\
DEF_F(NAME, pat,rem,													\
	int,																\
	(struct CC_3(pat_,NAME,_tree) *p, KEY_TYPE *key, u_int8_t key_len,	\
		TYPE **data),													\
	((struct pat_tree *)p, key, key_len, (void **)data) )				\
DEF_F(NAME, pat,map,													\
	void,																\
	(struct CC_3(pat_,NAME,_tree) *p, CC_3(pat_,NAME,_map_f) map_fun,	\
		void *arg_obj),													\
	((struct pat_tree *)p, (pat_map_f)map_fun, arg_obj) )				\
DEF_F(NAME, pat,walk,													\
	int,																\
	(struct CC_3(pat_,NAME,_tree) *p,									\
		struct CC_3(pat_,NAME,_node_s) **pos, TYPE **data),				\
	((struct pat_tree *)p, (struct pat_node_s **)pos, (void **)data) )			\
DEF_F(NAME, pat,walk_back,												\
	int,																\
	(struct CC_3(pat_,NAME,_tree) *p,									\
		struct CC_3(pat_,NAME,_node_s) **pos, TYPE **data),				\
	((struct pat_tree *)p, (struct pat_node_s **)pos, (void **)data) )			\

#define PAT_STRUCT_DEFS(NAME, TYPE, KEY_TYPE)							\
struct CC_3(pat_,NAME,_node_s) {										\
	u_int16_t	pat_bit;												\
	u_int16_t	pat_is_node;											\
	KEY_TYPE	*pat_key[2];											\
	u_int16_t	pat_key_len[2];											\
	struct		pat_node_s *pat_node[2];								\
	TYPE		*pat_data[2];											\
};																		\
																		\
struct CC_3(pat_,NAME,_tree) {											\
	u_int32_t	pat_keys;												\
	struct		pat_node_s *pat_root;									\
	KEY_TYPE	*pat_key;												\
	u_int16_t	pat_key_len;											\
	u_int16_t	pat_key_len_max;										\
	TYPE		*pat_data;												\
};

#define PAT_TYPEDEF_DEFS(NAME, TYPE, KEY_TYPE)							\
typedef void (* CC_3(pat_,NAME,_free_f)) (TYPE *);						\
typedef void (* CC_3(pat_,NAME,_free_key_f)) (KEY_TYPE *);				\
typedef void (* CC_3(pat_,NAME,_map_f)) (KEY_TYPE *, size_t, TYPE *, void *);


/* to be used in h-file */
#define PAT_TYPE(NAME,TYPE,KEY_TYPE)									\
PAT_STRUCT_DEFS(NAME,TYPE,KEY_TYPE)										\
PAT_TYPEDEF_DEFS(NAME,TYPE,KEY_TYPE)									\
PAT_FUN_DEFS(DEF_F_DECL,NAME,TYPE,KEY_TYPE)

/* to be used que_syms.c, requires that corresponding					*
 * PAT_TYPE is used first												*/
#define PAT_SYMS(NAME,TYPE,KEY_TYPE)									\
/* LINTLIBRARY */														\
PAT_FUN_DEFS(DEF_F_SYM,NAME,TYPE,KEY_TYPE)

/****** HEAP_LIST ******/

#define HEAP_FUN_DEFS(DEF_F, NAME, TYPE)								\
DEF_F(NAME, heap,init,													\
	struct CC_3(heap_,NAME,_list) *,									\
	(CC_3(heap_,NAME,_comp_f) comp_fun),								\
	((heap_comp_f)comp_fun) )											\
DEF_F(NAME, heap,clone,													\
	struct CC_3(heap_,NAME,_list) *,									\
	(struct CC_3(heap_,NAME,_list) *h),									\
	((struct heap_list *)h) )											\
DEF_F(NAME, heap,free,													\
	void,																\
	(struct CC_3(heap_,NAME,_list) *h, CC_3(heap_,NAME,_free_f) free_fun),\
	((struct heap_list *)h, (heap_free_f)free_fun) )					\
DEF_F(NAME, heap,count,													\
	unsigned,															\
	(struct CC_3(heap_,NAME,_list) *h),									\
	((struct heap_list *)h) )											\
DEF_F(NAME, heap,push,													\
	int,																\
	(struct CC_3(heap_,NAME,_list) *h, TYPE *data),						\
	((struct heap_list *)h, (void *)data) )								\
DEF_F(NAME, heap,pop,													\
	int,																\
	(struct CC_3(heap_,NAME,_list) *h, TYPE **data),						\
	((struct heap_list *)h, (void **)data) )								\
DEF_F(NAME, heap,peek,													\
	int,																\
	(struct CC_3(heap_,NAME,_list) *h, TYPE **data),						\
	((struct heap_list *)h, (void **)data) )								\

#define HEAP_STRUCT_DEFS(NAME, TYPE)									\
struct CC_3(heap_,NAME,_list) {											\
	u_int32_t	heap_size;												\
	u_int32_t	heap_keys;												\
	heap_comp_f	heap_comp_fun;											\
	TYPE		**heap_data;											\
};

#define HEAP_TYPEDEF_DEFS(NAME, TYPE)									\
typedef void (* CC_3(heap_,NAME,_free_f)) (TYPE *);						\
typedef int (* CC_3(heap_,NAME,_comp_f)) (TYPE *, TYPE *);

/* to be used in h-file */
#define HEAP_TYPE(NAME,TYPE)											\
HEAP_STRUCT_DEFS(NAME,TYPE)												\
HEAP_TYPEDEF_DEFS(NAME,TYPE)											\
HEAP_FUN_DEFS(DEF_F_DECL,NAME,TYPE)

/* to be used que_syms.c, requires that corresponding					*
 * HEAP_TYPE is used first												*/
#define HEAP_SYMS(NAME,TYPE)											\
/* LINTLIBRARY */														\
HEAP_FUN_DEFS(DEF_F_SYM,NAME,TYPE)

/****** HASH_BUF ******/

#define HASH_FUN_DEFS(DEF_F, NAME, TYPE)								\
DEF_F(NAME, hash,init,													\
	struct CC_3(hash_,NAME,_buf) *,										\
	(size_t hash_size),													\
	(hash_size) )														\
DEF_F(NAME, hash,clone,													\
	struct CC_3(hash_,NAME,_buf) *,										\
	(struct CC_3(hash_,NAME,_buf) *h),									\
	((struct hash_buf *)h) )											\
DEF_F(NAME, hash,free,													\
	void,																\
	(struct CC_3(hash_,NAME,_buf) *h, CC_3(hash_,NAME,_free_f) free_fun),\
	((struct hash_buf *)h, (hash_free_f)free_fun) )						\
DEF_F(NAME, hash,count,													\
	size_t,																\
	(struct CC_3(hash_,NAME,_buf) *h),									\
	((struct hash_buf *)h) )											\
DEF_F(NAME, hash,ins,													\
	int,																\
	(struct CC_3(hash_,NAME,_buf) *h, u_int32_t key, TYPE *data),		\
	((struct hash_buf *)h, key, (void *)data) )							\
DEF_F(NAME, hash,find,													\
	int,																\
	(struct CC_3(hash_,NAME,_buf) *h, u_int32_t key, TYPE **data),		\
	((struct hash_buf *)h, key, (void **)data) )							\
DEF_F(NAME, hash,map,													\
	void,																\
	(struct CC_3(hash_,NAME,_buf) *h, CC_3(hash_,NAME,_map_f) map_fun,	\
		void *arg_obj),													\
	((struct hash_buf *)h, (hash_map_f)map_fun, arg_obj) )				\
DEF_F(NAME, hash,rem,													\
	int,																\
	(struct CC_3(hash_,NAME,_buf) *h, u_int32_t key, TYPE **data),		\
	((struct hash_buf *)h, key, (void **)data) )

#define HASH_STRUCT_DEFS(NAME, TYPE)									\
struct CC_3(hash_,NAME,_data_s) {										\
	TYPE		*he_data;												\
	u_int32_t	he_key;													\
	struct		CC_3(hash_,NAME,_data_s) *he_next;						\
};																		\
																		\
struct CC_3(hash_,NAME,_buf) {											\
	size_t		hash_size;												\
	struct		CC_3(hash_,NAME,_data_s) *hash_data;						\
};

#define HASH_TYPEDEF_DEFS(NAME, TYPE)									\
typedef void (* CC_3(hash_,NAME,_free_f)) (TYPE *);						\
typedef void (* CC_3(hash_,NAME,_map_f)) (u_int32_t, TYPE *, void *);

/* to be used in h-file */
#define HASH_TYPE(NAME,TYPE)											\
HASH_STRUCT_DEFS(NAME,TYPE)												\
HASH_TYPEDEF_DEFS(NAME,TYPE)											\
HASH_FUN_DEFS(DEF_F_DECL,NAME,TYPE)

/* to be used in que_syms.c, requires that corresponding				*
 * HASH_TYPE is used first												*/
#define HASH_SYMS(NAME,TYPE)											\
/* LINTLIBRARY */														\
HASH_FUN_DEFS(DEF_F_SYM,NAME,TYPE)


/****** SKIP_LIST ******/

#define SKIP_STRUCT_DEFS(NAME, TYPE)									\
struct CC_3(skip_,NAME,_list_node) {									\
	TYPE		*sln_data;												\
	u_int32_t	sln_key;												\
	struct		CC_3(skip_,NAME,_list_node) *sln_forw[1];				\
};																		\
																		\
struct CC_3(skip_,NAME,_list) {											\
	struct		CC_3(skip_,NAME,_list_node) *sl_head;					\
	u_int32_t	sl_level;												\
	u_int32_t	sl_random_bits;											\
	u_int32_t	sl_randoms_left;										\
};

#define SKIP_TYPEDEF_DEFS(NAME, TYPE)									\
typedef void (* CC_3(sl_,NAME,_free_f)) (TYPE *);						\
typedef void (* CC_3(sl_,NAME,_map_f)) (u_int32_t, TYPE *, void *);

#define SKIP_FUN_DEFS(DEF_F, NAME, TYPE)								\
DEF_F(NAME, sl,init,													\
	struct CC_3(skip_,NAME,_list) *,									\
	(void),																\
	() )																\
DEF_F(NAME, sl,free,													\
	void,																\
	(struct CC_3(skip_,NAME,_list) *sl, CC_3(sl_,NAME,_free_f) free_fun),\
	((struct skip_list *)sl, (sl_free_f)free_fun) )						\
DEF_F(NAME, sl,ins,														\
	int,																\
	(struct CC_3(skip_,NAME,_list) *sl, u_int32_t key, TYPE *data),		\
	((struct skip_list *)sl, key, (void *)data) )						\
DEF_F(NAME, sl,find_pos,												\
	int,																\
	(struct CC_3(skip_,NAME,_list) *sl, u_int32_t key,					\
		struct CC_3(skip_,NAME,_list_node) **pos, TYPE **data),			\
	((struct skip_list *)sl, key, (struct skip_list_node **)pos,		\
		(void **)data) )													\
DEF_F(NAME, sl,find_first_pos,											\
	void,																\
	(struct CC_3(skip_,NAME,_list) *sl,									\
		struct CC_3(skip_,NAME,_list_node) **pos, TYPE **data),			\
	((struct skip_list *)sl, (struct skip_list_node **)pos, (void **)data) )\
DEF_F(NAME, sl,walk,													\
	int,																\
	(struct CC_3(skip_,NAME,_list) *sl,									\
		struct CC_3(skip_,NAME,_list_node) **pos, TYPE **data),			\
	((struct skip_list *)sl, (struct skip_list_node **)pos, (void **)data) )\
DEF_F(NAME, sl,find,													\
	int,																\
	(struct CC_3(skip_,NAME,_list) *sl, u_int32_t key, TYPE **data),	\
	((struct skip_list *)sl, key, (void **)data) )						\
DEF_F(NAME, sl,rem,														\
	int,																\
	(struct CC_3(skip_,NAME,_list) *sl, u_int32_t key, TYPE **data),	\
	((struct skip_list *)sl, key, (void **)data) )						\



/* to be used in h-file */
#define SKIP_TYPE(NAME,TYPE)											\
SKIP_STRUCT_DEFS(NAME,TYPE)												\
SKIP_TYPEDEF_DEFS(NAME,TYPE)											\
SKIP_FUN_DEFS(DEF_F_DECL,NAME,TYPE)

/* to be used in que_syms.c, requires that corresponding				*
 * SKIP_TYPE is used first												*/
#define SKIP_SYMS(NAME,TYPE)											\
/* LINTLIBRARY */														\
SKIP_FUN_DEFS(DEF_F_SYM,NAME,TYPE)


#endif /* CONTAINER_H */
