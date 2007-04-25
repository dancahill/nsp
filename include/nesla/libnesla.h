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
#ifndef _LIBNESLA_H
#define _LIBNESLA_H 1
#include "nesla/nesla.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

#define OUTBUFLOWAT	4096

/* error numbers */
#define NE_MEM		1
#define NE_SYNTAX	2
#define NE_INTERNAL	3

/* ops and cmps */
#define OP_UNDEFINED    255
/* punctuation */
#define	OP_POBRACE	254
#define	OP_POPAREN	253
#define	OP_POBRACKET	252

#define	OP_PCBRACE	251
#define	OP_PCPAREN	250
#define	OP_PCBRACKET	249
#define	OP_PCOMMA	248
#define	OP_PSEMICOL	247

#define	OP_PDOT		246
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
#define	OP_KDO		208
#define	OP_KWHILE	207
#define	OP_KEXIT	206

#define OP_LABEL	205
#define OP_DATA		204

#define OP_ISPUNC(o)	(o>=OP_PHASH&&o<=OP_POBRACE)
#define OP_ISMATH(o)	(o>=OP_MCGT&&o<=OP_MEQ)
#define OP_ISKEY(o)	(o>=OP_KEXIT&&o<=OP_KBREAK)
#define OP_ISEND(o)	(o>=OP_PSEMICOL&&o<=OP_PCBRACE)

#define n_debug(fn)	n_warn(N, fn, "%10s:%d - %d[ %s ]", __FILE__, __LINE__, N->lastop, N->lastname)
#define sanetest()	/*{ if (N->readptr==NULL) n_error(N, NE_SYNTAX, fn, "NULL readptr"); }*/

#if 0
#define ADJTIME 22
#define DEBUG_IN() { \
	char *tabs="................................"; \
	struct timeval ttime; \
	if (N) { \
		N->ttime.tv_usec+=ADJTIME; \
		if (N->ttime.tv_usec>1000000) { N->ttime.tv_sec++; N->ttime.tv_usec-=1000000; }; \
		N->test_depth++; \
		nc_gettimeofday(&ttime, NULL); \
		ttime.tv_sec-=N->ttime.tv_sec; ttime.tv_usec-=N->ttime.tv_usec; \
		if (ttime.tv_usec<0) { ttime.tv_sec--; ttime.tv_usec+=1000000; }; \
		printf("\n%d.%06d%s[01;34;40m+%s[00m", (int)ttime.tv_sec, (int)ttime.tv_usec, tabs+32-N->test_depth, fn); \
	} \
}
#define DEBUG_OUT() { \
	char *tabs="................................"; \
	struct timeval ttime; \
	if (N) { \
		N->ttime.tv_usec+=ADJTIME; \
		if (N->ttime.tv_usec>1000000) { N->ttime.tv_sec++; N->ttime.tv_usec-=1000000; }; \
		nc_gettimeofday(&ttime, NULL); \
		ttime.tv_sec-=N->ttime.tv_sec; ttime.tv_usec-=N->ttime.tv_usec; \
		if (ttime.tv_usec<0) { ttime.tv_sec--; ttime.tv_usec+=1000000; }; \
		printf("\n%d.%06d%s[01;34;40m-%s (%d)[00m", (int)ttime.tv_sec, (int)ttime.tv_usec, tabs+32-N->test_depth, fn, __LINE__); \
		N->test_depth--; \
	} \
}
#else
#define DEBUG_IN()
#define DEBUG_OUT()
#endif

/* exec.c */
obj_t   *n_execfunction (nes_state *N, obj_t *cobj);
/* lib.c */
int      n_unescape     (nes_state *N, char *src, char *dst, int len);
int      nl_flush       (nes_state *N);
int      nl_print       (nes_state *N);
int      nl_fileread    (nes_state *N);
int      nl_filewrite   (nes_state *N);
int      nl_math1       (nes_state *N);
int      nl_tonumber    (nes_state *N);
int      nl_tostring    (nes_state *N);
int      nl_strcat      (nes_state *N);
int      nl_strcmp      (nes_state *N);
int      nl_strlen      (nes_state *N);
int      nl_strsplit    (nes_state *N);
int      nl_strstr      (nes_state *N);
int      nl_strsub      (nes_state *N);
int      nl_datetime    (nes_state *N);
int      nl_sleep       (nes_state *N);
int      nl_runtime     (nes_state *N);
int      nl_include     (nes_state *N);
int      nl_printvars   (nes_state *N);
int      nl_type        (nes_state *N);
/* libc.c */
#define  nc_isdigit(c)  ((c>='0'&&c<='9')?1:0)
#define  nc_isalpha(c)  ((c>='A'&&c<='Z')||(c>='a'&&c<='z')?1:0)
#define  nc_isalnum(c)  ((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')?1:0)
#define  nc_isupper(c)  ((c>='A'&&c<='Z')?1:0)
#define  nc_islower(c)  ((c>='a'&&c<='z')?1:0)
#define  nc_isspace(c)  (c=='\r'||c=='\n'||c=='\t'||c==' ')
#define  nc_tolower(c)  ((c>='A'&&c<='Z')?(c+('a'-'A')):c)
#define  nc_toupper(c)  ((c>='a'&&c<='z')?(c-('a'-'A')):c)
int      nc_snprintf    (nes_state *N, char *str, int size, const char *format, ...);
int      nc_printf      (nes_state *N, const char *format, ...);
int      nc_gettimeofday(struct timeval *tv, void *tz);
char    *nc_memcpy      (char *dst, const char *src, int n);
int      nc_strlen      (char *s);
char    *nc_strchr      (const char *s, int c);
char    *nc_strncpy     (char *d, const char *s, int n);
int      nc_strcmp      (const char *s1, const char *s2);
int      nc_strncmp     (const char *s1, const char *s2, int n);
void    *nc_memset      (void *s, int c, int n);

void     n_error        (nes_state *N, short int err, const char *fname, const char *format, ...);
void     n_warn         (nes_state *N, const char *fname, const char *format, ...);
/* loop.c */
void     n_if           (nes_state *N);
void     n_for          (nes_state *N);
void     n_do           (nes_state *N);
void     n_while        (nes_state *N);
/* obj.c */
void    *n_alloc        (nes_state *N, int size);
void     n_free         (nes_state *N, void **ptr);
void     n_copyval      (nes_state *N, obj_t *cobj1, obj_t *cobj2);
val_t   *n_newval       (nes_state *N, unsigned short type);
void     n_freeval      (nes_state *N, obj_t *cobj);
void     n_joinstr      (nes_state *N, obj_t *cobj, char *str, int len);
obj_t   *n_newiobj      (nes_state *N, int index);
/* parse.c */
void     n_skipto       (nes_state *N, unsigned short c);
short    n_getop        (nes_state *N);
num_t    n_getnumber    (nes_state *N);
obj_t   *n_getquote     (nes_state *N);
obj_t   *n_getindex     (nes_state *N, obj_t *cobj, char *lastname);

void     n_prechew      (nes_state *N, uchar *rawtext);

obj_t   *n_evalsub      (nes_state *N);
obj_t   *n_evalargs     (nes_state *N, char *fname);
obj_t   *n_storeval     (nes_state *N, obj_t *cobj);
obj_t   *n_readfunction (nes_state *N);
obj_t   *n_readvar      (nes_state *N, obj_t *tobj, obj_t *cobj);

#define  nextop()       { if (*N->readptr<128) n_getop(N); else { N->lastop=*N->readptr; N->lastptr=N->readptr++; N->lastname[0]=0; } }
#define  ungetop()      N->readptr=N->lastptr; N->lastop=OP_UNDEFINED; N->lastname[0]=0;

#endif /* libnesla.h */
