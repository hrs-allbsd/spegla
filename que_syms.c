/* $Id: que_syms.c,v 1.3 1999/06/10 18:53:51 jens Exp $ */
/* This files is just used for creating the symbols to link against. */

#include "spegla.h"

FIFO_SYMS(spf, struct sp_file)
HEAP_SYMS(spf, struct sp_file)
CL_SYMS(sps, struct sp_skip)
CL_SYMS(spt, struct sp_treatasdir)

