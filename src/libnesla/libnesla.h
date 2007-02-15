/*
    NESLA NullLogic Embedded Scripting Language - Copyright (C) 2007 Dan Cahill

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "nesla.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/* error numbers */
#define NE_MEM		1
#define NE_SYNTAX	2

/* ops and cmps */
#define OP_UNDEFINED    255
/* punctuation */
#define	OP_POBRACE	254
#define	OP_POPAREN	253
#define	OP_PCBRACE	252
#define	OP_PCPAREN	251
#define	OP_PCOMMA	250
#define	OP_PSEMICOL	249
#define	OP_PDOT		248
#define	OP_POBRACKET	247
#define	OP_PCBRACKET	246
#define	OP_PSQUOTE	245
#define	OP_PDQUOTE	244
#define	OP_PHASH	243
/* math ops */
#define	OP_MEQ		242
#define	OP_MADD		241
#define	OP_MSUB		240
#define	OP_MMUL		239
#define	OP_MDIV		238
#define	OP_MADDEQ	237
#define	OP_MSUBEQ	236
#define	OP_MMULEQ	235
#define	OP_MDIVEQ	234
#define	OP_MADDADD	233
#define	OP_MSUBSUB	232
#define	OP_MMOD		231
#define	OP_MAND		230
#define	OP_MOR		229
#define	OP_MXOR		228
#define	OP_MLAND	227
#define	OP_MLOR		226
#define	OP_MLNOT	225
#define	OP_MCEQ		224
#define	OP_MCNE		223
#define	OP_MCLE		222
#define	OP_MCGE		221
#define	OP_MCLT		220
#define	OP_MCGT		219
/* keywords */
#define	OP_KBREAK	218
#define	OP_KCONT	217
#define	OP_KRET		216
#define	OP_KFUNC	215
#define	OP_KGLOB	214
#define	OP_KLOCAL	213
#define	OP_KVAR		212
#define	OP_KIF		211
#define	OP_KELSE	210
#define	OP_KFOR		209
#define	OP_KWHILE	208
#define	OP_KEXIT	207

#define OP_LABEL	206
#define OP_DATA		205

#define OP_ISPUNC(o)	((o>=OP_PHASH)&&(o<=OP_POBRACE))
#define OP_ISMATH(o)	((o>=OP_MCGT)&&(o<=OP_MEQ))
#define OP_ISKEY(o)	((o>=OP_KWHILE)&&(o<=OP_KBREAK))
#define IS_DATA(c)	(isdigit(c)||(c=='\'')||(c=='\"'))
#define IS_STMTEND(o)	((o==OP_PCPAREN)||(o==OP_PCOMMA)||(o==OP_PSEMICOL)||(o==OP_PCBRACKET)||(o==OP_PCBRACE))

#define n_debug(fn)	n_warn(N, fn, "%10s:%d - %d[ %s ]", __FILE__, __LINE__, N->lastop, N->lastname)

typedef int(*NESLA_CFUNC)(nesla_state *);

/* exec.c */
#ifdef WIN32
int gettimeofday(struct timeval *tv, void *tz);
#endif
obj_t *ne_macros        (nesla_state *N);
int    n_vsnprintf      (nesla_state *N, char *str, int size, const char *format, va_list ap);
int    n_snprintf       (nesla_state *N, char *str, int size, const char *format, ...);
int    n_printf         (nesla_state *N, short dowrite, const char *format, ...);
void   n_error          (nesla_state *N, short int err, const char *fname, const char *format, ...);
void   n_warn           (nesla_state *N, const char *fname, const char *format, ...);
obj_t *ne_execfunction  (nesla_state *N, obj_t *cobj);
/* lib.c */
int    nl_write         (nesla_state *N);
int    nl_print         (nesla_state *N);
int    nl_math1         (nesla_state *N);
int    nl_number        (nesla_state *N);
int    nl_strcat        (nesla_state *N);
int    nl_strcmp        (nesla_state *N);
int    nl_strlen        (nesla_state *N);
int    nl_strstr        (nesla_state *N);
int    nl_strsub        (nesla_state *N);
int    nl_datetime      (nesla_state *N);
int    nl_sleep         (nesla_state *N);
int    nl_runtime       (nesla_state *N);
int    nl_include       (nesla_state *N);
/* obj.c */
void  *n_alloc          (nesla_state *N, int size);
void   n_free           (nesla_state *N, void **ptr);
obj_t *n_setobject      (nesla_state *N, obj_t *tobj, obj_t *object);
/* parse.c */
char    *n_skipcomment  (nesla_state *N);
char    *n_skipblank    (nesla_state *N);
char    *n_skipto       (nesla_state *N, char c);
void     n_ungetop      (nesla_state *N);
short    n_getop        (nesla_state *N);
num_t    n_getnumber    (nesla_state *N);
char    *n_getlabel     (nesla_state *N, char *nambuf);
obj_t   *n_getquote     (nesla_state *N);
obj_t   *n_getindex     (nesla_state *N, obj_t *cobj, char *lastname);

obj_t   *n_evalsub      (nesla_state *N, char *end);
obj_t   *n_eval         (nesla_state *N, char *end);
obj_t   *n_evalargs     (nesla_state *N, char *name);
int      n_storefunction(nesla_state *N);
int      n_storevar     (nesla_state *N, obj_t *tobj, char *name);
int      n_storetable   (nesla_state *N, obj_t *tobj);
int      n_assign2      (nesla_state *N, obj_t *tobj, char *name, short op);
int      n_assign       (nesla_state *N, obj_t *tobj);
