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
#include <setjmp.h>
#ifdef WIN32
#include <winsock.h> /* just for timeval */
#include <time.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

#define num_t double

#define MAX_OBJNAMLEN 64

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

#define obj_t struct nesla_objrec
union nesla_object {
	void  *null;
	short  boolean;
	num_t  number;
	char  *string;
	char  *chunk;
	obj_t *table;
	char  *function;
	void  *cfunction;
};
typedef struct nesla_objrec {
	struct nesla_objrec *parent;
	struct nesla_objrec *prev;
	struct nesla_objrec *next;
	char name[MAX_OBJNAMLEN+1];
	unsigned short type;
	unsigned short mode; /* status: mode (hidden, readonly) */
	unsigned int   size; /* status: string or chunk size */
	union nesla_object d;
} nesla_objrec;
typedef struct {
	char *readptr;
	char *lastptr;
	short int debug;
	short int warnings;
	short int err;
	short int brk;
	short int ret;
	obj_t g;
	obj_t l;
	short int lastop;
	char lastname[MAX_OBJNAMLEN+1];
	jmp_buf savjmp;
	struct timeval ttime;
	char errbuf[256];
	char txtbuf[8193];
} nesla_state;

#ifndef NESLA_NOFUNCTIONS
/* exec */
nesla_state *nesla_newstate (void);
nesla_state *nesla_endstate (nesla_state *N);
char        *nesla_exec     (nesla_state *N, char *string);
int          nesla_execfile (nesla_state *N, char *file);
/* object */
void         nesla_freetable(nesla_state *N, obj_t *tobj);
obj_t       *nesla_getobj   (nesla_state *N, obj_t *tobj, char *oname);
obj_t       *nesla_getiobj  (nesla_state *N, obj_t *tobj, int oindex);
obj_t       *nesla_regnull  (nesla_state *N, obj_t *tobj, char *name);
obj_t       *nesla_regnum   (nesla_state *N, obj_t *tobj, char *name, num_t data);
obj_t       *nesla_regstr   (nesla_state *N, obj_t *tobj, char *name, char *data);
obj_t       *nesla_regtable (nesla_state *N, obj_t *tobj, char *name);
obj_t       *nesla_regnfunc (nesla_state *N, obj_t *tobj, char *name, char *data);
obj_t       *nesla_regcfunc (nesla_state *N, obj_t *tobj, char *name, void *data);
num_t        nesla_tofloat  (nesla_state *N, obj_t *cobj);
int          nesla_toint    (nesla_state *N, obj_t *cobj);
char        *nesla_tostr    (nesla_state *N, obj_t *cobj);
num_t        nesla_getfloat (nesla_state *N, obj_t *tobj, char *oname);
int          nesla_getint   (nesla_state *N, obj_t *tobj, char *oname);
char        *nesla_getstr   (nesla_state *N, obj_t *tobj, char *oname);
char        *nesla_getfstr  (nesla_state *N, obj_t *tobj, char *oname);
char        *nesla_getrstr  (nesla_state *N, obj_t *tobj, char *oname);
void         nesla_printvars(nesla_state *N, obj_t *cobj);
void         nesla_sorttable(nesla_state *N, obj_t *tobj, int recurse);
#endif
