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
#ifndef _NESLA_H
#define _NESLA_H 1

#ifdef WIN32
/* always include winsock2 before windows, or bad stuff will happen */
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#else
#ifdef __TURBOC__
struct timeval { long tv_sec; long tv_usec; };
#else
#include <sys/time.h>
#endif
#endif
#include <setjmp.h>

#define NESLA_NAME     "nesla"
#define NESLA_VERSION  "0.3.0"

#define MAX_OBJNAMELEN 64
#define MAX_OUTBUFLEN  8192
#define OUTBUFLOWAT    4096

/* object storage types */
#define NT_NULL		0
#define NT_BOOLEAN 	1
#define NT_NUMBER	2
#define NT_STRING	3
#define NT_CHUNK	4
#define NT_TABLE	5
#define NT_NFUNC	6
#define NT_CFUNC	7

/* object storage modes */
#define NST_HIDDEN	1
#define NST_READONLY	2
#define NST_SYSTEM	4
#define NST_LINK	8

#define num_t double
#define uchar unsigned char
#define obj_t struct nes_objrec
union nes_object {
	char  *str;
	num_t  num;
	obj_t *table;
	void  *cfunc;
};
typedef struct nes_objrec {
	struct nes_objrec *parent;
	struct nes_objrec *prev;
	struct nes_objrec *next;
	unsigned short type;
	unsigned short mode; /* status: mode (hidden, readonly) */
	unsigned short sort; /* autosort if true */
	unsigned int   size; /* string or chunk size */
	char name[MAX_OBJNAMELEN+1];
	union nes_object d;
} nes_objrec;
typedef struct {
	uchar *readptr;
	uchar *lastptr;
	short int debug;
	short int strict;
	short int test_depth;
	short int warnings;
	short int err;
	short int brk;
	short int ret;
	obj_t g;
	obj_t l;
	obj_t r;
	short int lastop;
	char lastname[MAX_OBJNAMELEN+1];
	short int jmpset;
	jmp_buf savjmp;
	struct timeval ttime;
	unsigned short int outbuflen;
	char numbuf[128];
	char outbuf[MAX_OUTBUFLEN+1];
	char errbuf[256];
} nes_state;

#define    nes_isnull(o)     (o==NULL||o->type==NT_NULL)
#define    nes_istable(o)    (o!=NULL&&o->type==NT_TABLE)
#define    nes_isnum(o)      (o!=NULL&&o->type==NT_NUMBER)
#define    nes_isstr(o)      (o!=NULL&&o->type==NT_STRING)

#define    nes_tonum(N,o)    (o==NULL?0:o->type==NT_NUMBER?o->d.num:o->type==NT_BOOLEAN?o->d.num?1:0:o->type==NT_STRING?nes_aton(N,o->d.str):0)
#define    nes_getnum(N,o,n) nes_tonum(N, nes_getobj(N,o,n))

#define    nes_tostr(N,o)    (o==NULL?"":o->type==NT_TABLE?"":o->type==NT_NUMBER?nes_ntoa(N,N->numbuf,o->d.num,-10,6):o->type==NT_BOOLEAN?o->d.num?"true":"false":o->type==NT_STRING?o->d.str?o->d.str:o->type==NT_NULL?"null":"":"null")
#define    nes_getstr(N,o,n) nes_tostr(N, nes_getobj(N,o,n))

#define    nes_setnum(N,t,n,v)     nes_setobj(N, t, n, NT_NUMBER, NULL, v, NULL, 0)
#define    nes_setstr(N,t,n,s,l)   nes_setobj(N, t, n, NT_STRING, NULL, 0, s,    l)
#define    nes_settable(N,t,n)     nes_setobj(N, t, n, NT_TABLE,  NULL, 0, NULL, 0)
#define    nes_setcfunc(N,t,n,p)   nes_setobj(N, t, n, NT_CFUNC,  p,    0, NULL, 0)
#define    nes_setnfunc(N,t,n,s,l) nes_setobj(N, t, n, NT_NFUNC,  NULL, 0, s,    l)

#ifndef NESLA_NOFUNCTIONS
/* api */
obj_t     *nes_readtablef(nes_state *N, obj_t *tobj, const char *fmt, ...);
/* exec */
nes_state *nes_newstate  (void);
nes_state *nes_endstate  (nes_state *N);
obj_t     *nes_exec      (nes_state *N, char *string);
int        nes_execfile  (nes_state *N, char *file);
/* libc */
num_t      nes_aton      (nes_state *N, const char *str);
char      *nes_ntoa      (nes_state *N, char *str, num_t num, short base, unsigned short dec);
/* object */
void       nes_freetable (nes_state *N, obj_t *tobj);
obj_t     *nes_getobj    (nes_state *N, obj_t *tobj, char *oname);
obj_t     *nes_getiobj   (nes_state *N, obj_t *tobj, int oindex);
obj_t     *nes_setobj    (nes_state *N, obj_t *tobj, char *oname, unsigned short otype, void *_fptr, num_t _num, char *_str, int _slen);
/* object */
obj_t     *nes_eval      (nes_state *N, char *string);
obj_t     *nes_readtable (nes_state *N, obj_t *tobj);
#endif

#endif /* nesla.h */
