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
#include "libnesla.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef WIN32
#include <io.h>
#else
#ifndef __TURBOC__
#include <unistd.h>
#endif
#endif

void n_if(nes_state *N)
{
	static char *fn="n_if";
	char done=0, t=0;
	obj_t *cobj;

	DEBUG_IN();
l1:
	nextop();
	if (N->lastop==OP_POPAREN) {
		N->lastop=OP_UNDEFINED;
		cobj=nes_eval(N, (char *)N->readptr);
		if (N->lastop!=OP_PCPAREN) n_error(N, NE_SYNTAX, fn, "missing ) bracket");
		t=cobj->d.num?1:0;
	}
	nextop();
l2:
	if (N->lastop==OP_POBRACE) {
		if ((t)&&(!done)) {
			nes_exec(N, (char *)N->readptr);
			done=1;
			if (N->ret) { DEBUG_OUT(); return; }
		} else {
			n_skipto(N, OP_PCBRACE);
		}
		nextop();
		if (N->lastop!=OP_KELSE) {
			ungetop();
		} else {
			nextop();
			if (N->lastop!=OP_KIF) {
				t=1;
				goto l2;
			} else {
				goto l1;
			}
		}
	}
	DEBUG_OUT();
	return;
}

void n_for(nes_state *N)
{
	static char *fn="n_for";
	uchar *arginit, *argcomp, *argexec;
	uchar *bs, *be;
	obj_t *cobj;

	DEBUG_IN();
	if (N->readptr==NULL) n_error(N, NE_SYNTAX, fn, "NULL readptr");
	nextop();
	if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, fn, "missing ( bracket");
	arginit=N->readptr;
	n_skipto(N, OP_PSEMICOL);
	argcomp=N->readptr;
	n_skipto(N, OP_PSEMICOL);
	argexec=N->readptr;
	n_skipto(N, OP_PCPAREN);
	bs=N->readptr;
	nextop();
	n_skipto(N, OP_PCBRACE);
	be=N->readptr;
	N->readptr=arginit;
	n_readvar(N, &N->l, NULL);
	for (;;) {
showruntime();
		if (argcomp[0]==OP_PSEMICOL||argcomp[0]==';') {
		} else {
			cobj=nes_eval(N, (char *)argcomp);
			if (N->lastop!=OP_PSEMICOL) n_error(N, NE_SYNTAX, fn, "expected a closing ;");
			if (nes_tonum(N, cobj)==0) break;
		}
		N->readptr=bs;
		nextop();
		nes_exec(N, (char *)N->readptr);
		if (N->brk>0) { N->brk--; break; }
		if (N->ret) { break; }
		N->readptr=argexec;
		n_readvar(N, &N->l, NULL);
showruntime();
	}
	N->readptr=be;
	DEBUG_OUT();
	return;
}

void n_while(nes_state *N)
{
	static char *fn="n_while";
	uchar *argcomp;
	uchar *bs, *be;
	obj_t *cobj;

	DEBUG_IN();
	if (N->readptr==NULL) n_error(N, NE_SYNTAX, fn, "EOF");
	nextop();
	if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, fn, "missing ( bracket");
	argcomp=N->readptr;
	n_skipto(N, OP_PCPAREN);
	bs=N->readptr;
	nextop();
	n_skipto(N, OP_PCBRACE);
	be=N->readptr;
	for (;;) {
		cobj=nes_eval(N, (char *)argcomp);
		if (N->lastop!=OP_PCPAREN) n_error(N, NE_SYNTAX, fn, "missing ) bracket");
		if (nes_tonum(N, cobj)==0) break;
		N->readptr=bs;
		nextop();
		nes_exec(N, (char *)N->readptr);
		if (N->brk>0) { N->brk--; break; }
		if (N->ret) { break; }
	}
	N->readptr=be;
	DEBUG_OUT();
	return;
}

obj_t *n_macros(nes_state *N)
{
	static char *fn="n_macros";
	obj_t *cobj, *tobj;
	char *p;

	DEBUG_IN();
	if (nc_strcmp("exit", N->lastname)==0) {
		n_error(N, 0, fn, "exiting normally");
	} else if (nc_strcmp("printvars", N->lastname)==0) {
		nextop();
		if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, fn, "missing ( bracket");
		nextop();
		if (N->lastop==OP_LABEL) {
			cobj=nes_getobj(N, &N->l, N->lastname);
			while (cobj->type==NT_TABLE) {
				if (*N->readptr!=OP_POBRACKET&&*N->readptr!=OP_PDOT&&*N->readptr!='['&&*N->readptr!='.') break;
				cobj=n_getindex(N, cobj, NULL);
			}
			n_printvars(N, cobj);
			nextop();
		} else if (N->lastop==OP_PCPAREN) {
			n_printvars(N, N->g.d.table);
		}
		if (N->lastop!=OP_PCPAREN) n_error(N, NE_SYNTAX, fn, "missing ) bracket");
		N->lastop=OP_UNDEFINED;
		DEBUG_OUT();
		return nes_setnum(N, &N->r, "", 0);
	} else if (nc_strcmp("type", N->lastname)==0) {
		nextop();
		if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, fn, "missing ( bracket");
		nextop();
		if (N->lastop!=OP_LABEL) n_error(N, NE_SYNTAX, fn, "missing arg1");
		cobj=nes_getobj(N, &N->l, N->lastname);
		/* devour the whole var name and puke a null type if any part of the name is missing */
		do {
			if (cobj->type==NT_TABLE) tobj=cobj; else tobj=NULL;
			if (*N->readptr!=OP_POBRACKET&&*N->readptr!=OP_PDOT&&*N->readptr!='['&&*N->readptr!='.') break;
			cobj=n_getindex(N, tobj, NULL);
		} while (1);
		switch (cobj->type) {
		case NT_BOOLEAN : p="boolean";  break;
		case NT_NUMBER  : p="number";   break;
		case NT_STRING  : p="string";   break;
		case NT_TABLE   : p="table";    break;
		case NT_NFUNC   :
		case NT_CFUNC   : p="function"; break;
		default         : p="null";     break;
		}
		nextop();
		if (N->lastop!=OP_PCPAREN) n_error(N, NE_SYNTAX, fn, "missing ) bracket");
		N->lastop=OP_UNDEFINED;
		DEBUG_OUT();
		return nes_setstr(N, &N->r, "", p, nc_strlen(p));
	}
	DEBUG_OUT();
	return NULL;
}

obj_t *n_execfunction(nes_state *N, obj_t *cobj)
{
	static char *fn="n_execfunction";
	NES_CFUNC cfunc;
	obj_t *cobj2;
	obj_t *olobj;
	obj_t *pobj;
	uchar *p;
	unsigned int i;

	DEBUG_IN();
	if ((cobj->type!=NT_NFUNC)&&(cobj->type!=NT_CFUNC)) n_error(N, NE_SYNTAX, fn, "'%s' is not a function", cobj->name);
	nextop();
	if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, fn, "missing arg bracket");
	pobj=n_evalargs(N, cobj->name);
	N->lastop=OP_UNDEFINED;
	olobj=N->l.d.table; N->l.d.table=pobj;
	p=N->readptr;
#ifdef DEBUG
	if (N->debug) n_warn(N, fn, "%s()", cobj->name);
#endif
	if (cobj->type==NT_CFUNC) {
		cfunc=(NES_CFUNC)cobj->d.cfunc;
		cfunc(N);
	} else if (cobj->type==NT_NFUNC) {
		N->readptr=(uchar *)cobj->d.str;
		nextop();
		if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, fn, "missing bracket");
		for (i=1;;i++) {
			cobj2=nes_getiobj(N, &N->l, i);
			nextop();
			if (N->lastop==OP_LABEL) {
				nc_strncpy(cobj2->name, N->lastname, MAX_OBJNAMELEN);
				nextop();
			}
			if (N->lastop==OP_PCOMMA) continue;
			if (N->lastop==OP_PCPAREN) break;
			n_error(N, NE_SYNTAX, fn, "i'm confused.... %s", N->lastname);
		}
		nextop();
		if (N->lastop==OP_POBRACE) nes_exec(N, (char *)N->readptr);
		if (N->ret) { N->ret=0; }
	}
	nes_freetable(N, &N->l);
	N->l.d.table=olobj;
	N->readptr=p;
	DEBUG_OUT();
	return nes_getobj(N, &N->r, "");
}

/*
 * the following functions are public API functions
 */
/*
int nes_cwrap(nes_state *N, void *fn, char *arg0, ...)
{
	va_list ap;
	int x;

	va_start(ap, arg0);
	while ((x=va_arg(ap, int))!=0) {
		nes_printf(N, "[%d]\n", x);
	}
	va_end(ap);
	return 0;
}
*/
obj_t *nes_exec(nes_state *N, char *string)
{
	static char *fn="nes_exec";
	char namebuf[MAX_OBJNAMELEN+1];
	obj_t *cobj, *tobj;
	char block;
	short int jmp=N->jmpset;

	DEBUG_IN();
	if (jmp==0) {
		if (setjmp(N->savjmp)==0) {
			N->jmpset=1;
		} else {
			goto end;
		}
	}
	N->readptr=(uchar *)string;
	if (N->readptr==NULL) goto end;
	if (N->lastop==OP_POBRACE) block=1; else block=0;
	while (*N->readptr) {
		if ((N->brk>0)&&(block)) goto end;
		nextop();
		if ((N->lastop==OP_PCBRACE)&&(block)) break;
		if (OP_ISMATH(N->lastop)) {
			n_warn(N, fn, "unexpected math op '%s'", N->lastname);
		} else if (OP_ISPUNC(N->lastop)) {
			n_warn(N, fn, "unexpected punctuation '%s'", N->lastname);
		} else if (OP_ISKEY(N->lastop)) {
			switch (N->lastop) {
			case OP_KLOCAL:
			case OP_KVAR:   n_readvar(N, &N->l, NULL); break;
			case OP_KGLOB:  n_readvar(N, &N->g, NULL); break;
			case OP_KIF:    n_if(N);    if (N->ret) goto end; else break;
			case OP_KFOR:   n_for(N);   if (N->ret) goto end; else break;
			case OP_KWHILE: n_while(N); if (N->ret) goto end; else break;
			case OP_KFUNC:  n_readfunction(N); break;
			case OP_KBREAK:
				if (!block) n_error(N, NE_SYNTAX, fn, "return without block");
				N->brk=nc_isdigit(*N->readptr)?(short int)n_getnumber(N):1;
				if (*N->readptr==OP_PSEMICOL||*N->readptr==';') nextop();
				goto end;
			case OP_KCONT:
				if (!block) n_error(N, NE_SYNTAX, fn, "continue without block");
				if (*N->readptr==OP_PSEMICOL||*N->readptr==';') nextop();
				goto end;
			case OP_KRET:
				n_storeval(N, &N->r);
				N->ret=1;
				goto end;
			case OP_KELSE:  n_error(N, NE_SYNTAX, fn, "stray else");
			}
		} else {
			if (N->lastname[0]=='\0') n_error(N, NE_SYNTAX, fn, "zero length token");
			if (n_macros(N)!=NULL) {
				if (*N->readptr==OP_PSEMICOL||*N->readptr==';') nextop();
				continue;
			}
			tobj=&N->l;
			cobj=nes_getobj(N, tobj, N->lastname);
			while (cobj->type==NT_TABLE) {
				tobj=cobj;
				if (*N->readptr!=OP_POBRACKET&&*N->readptr!=OP_PDOT&&*N->readptr!='['&&*N->readptr!='.') break;
				cobj=n_getindex(N, tobj, namebuf);
				if ((cobj->type==NT_NULL)&&(namebuf[0]!=0)) {
					cobj=nes_setnum(N, tobj, namebuf, 0);
				}
			}
			if ((cobj->type==NT_NFUNC)||(cobj->type==NT_CFUNC)) {
				n_execfunction(N, cobj);
				if (*N->readptr==OP_PSEMICOL||*N->readptr==';') nextop();
				continue;
			} else if (cobj->type==NT_NULL) {
				if (N->lastop!=OP_LABEL) n_error(N, NE_SYNTAX, fn, "expected a label");
				cobj=nes_setnum(N, tobj, N->lastname, 0);
			}
			n_readvar(N, tobj, cobj);
		}
	}
end:
	if (jmp==0) N->jmpset=0;
	DEBUG_OUT();
	return NULL;
}

#ifndef O_BINARY
#define O_BINARY 0
#endif
int nes_execfile(nes_state *N, char *file)
{
	obj_t *cobj=nes_getobj(N, &N->g, "_filepath");
	char buf[512];
	struct stat sb;
	uchar *rawtext;
	char *p;
	int bl;
	int fp;
	int r;

	if ((stat(file, &sb)!=0)&&(cobj->type==NT_STRING)) {
		nc_snprintf(N, buf, sizeof(buf), "%s/%s", cobj->d.str, file);
		if (stat(buf, &sb)!=0) return -1;
		file=buf;
	}
/*	if (sb.st_mode&S_IFDIR) return -1; */
	if ((fp=open(file, O_RDONLY|O_BINARY))==-1) return -1;
	rawtext=n_alloc(N, (sb.st_size+2)*sizeof(char));
	p=(char *)rawtext;
	bl=sb.st_size;
	for (;;) {
		r=read(fp, p, bl);
		p+=r;
		bl-=r;
		if (bl<1) break;
	}
	close(fp);
	rawtext[sb.st_size]='\0';

	n_prechew(N, rawtext);
/*
	if ((fp=open("chewed.nes", O_CREAT|O_TRUNC|O_WRONLY|O_BINARY))==-1) return -1;
	if ((fp=open("chewed.nes", O_CREAT|O_APPEND|O_WRONLY|O_BINARY))==-1) return -1;
	write(fp, rawtext, nc_strlen((char *)rawtext));
	close(fp);
*/

	nes_exec(N, (char *)rawtext);
	n_free(N, (void *)&rawtext);
	if (N->outbuflen) nl_flush(N);
	return 0;
}

typedef struct {
	char *fn_name;
	void *fn_ptr;
} FUNCTION;

nes_state *nes_newstate()
{
	FUNCTION list[]={
		{ "date",	(NES_CFUNC *)nl_datetime	},
		{ "include",	(NES_CFUNC *)nl_include		},
		{ "tonumber",	(NES_CFUNC *)nl_tonumber	},
		{ "tostring",	(NES_CFUNC *)nl_tostring	},
		{ "print",	(NES_CFUNC *)nl_print		},
		{ "write",	(NES_CFUNC *)nl_print		},
		{ "runtime",	(NES_CFUNC *)nl_runtime		},
		{ "sleep",	(NES_CFUNC *)nl_sleep		},
		{ "time",	(NES_CFUNC *)nl_datetime	},
		{ NULL, NULL }
	};
	FUNCTION list_io[]={
		{ "print",	(NES_CFUNC *)nl_print		},
		{ "write",	(NES_CFUNC *)nl_print		},
		{ "flush",	(NES_CFUNC *)nl_flush		},
		{ NULL, NULL }
	};
	FUNCTION list_math[]={
		{ "abs",	(NES_CFUNC *)nl_math1		},
		{ "acos",	(NES_CFUNC *)nl_math1		},
		{ "asin",	(NES_CFUNC *)nl_math1		},
		{ "atan",	(NES_CFUNC *)nl_math1		},
		{ "ceil",	(NES_CFUNC *)nl_math1		},
		{ "cos",	(NES_CFUNC *)nl_math1		},
		{ "floor",	(NES_CFUNC *)nl_math1		},
		{ "rand",	(NES_CFUNC *)nl_math1		},
		{ "sin",	(NES_CFUNC *)nl_math1		},
		{ "sqrt",	(NES_CFUNC *)nl_math1		},
		{ "tan",	(NES_CFUNC *)nl_math1		},
		{ NULL, NULL }
	};
	FUNCTION list_string[]={
		{ "cat",	(NES_CFUNC *)nl_strcat		},
		{ "cmp",	(NES_CFUNC *)nl_strcmp		},
		{ "icmp",	(NES_CFUNC *)nl_strcmp		},
		{ "ncmp",	(NES_CFUNC *)nl_strcmp		},
		{ "nicmp",	(NES_CFUNC *)nl_strcmp		},
		{ "len",	(NES_CFUNC *)nl_strlen		},
		{ "str",	(NES_CFUNC *)nl_strstr		},
		{ "istr",	(NES_CFUNC *)nl_strstr		},
		{ "sub",	(NES_CFUNC *)nl_strsub		},
		{ NULL, NULL }
	};
	nes_state *new_N;
	obj_t *cobj;
	short i;

	new_N=n_alloc(NULL, sizeof(nes_state));
	nc_memset(new_N, 0, sizeof(nes_state));
	nc_gettimeofday(&new_N->ttime, NULL);
	srand(new_N->ttime.tv_usec);
	new_N->g.type=NT_TABLE;
	new_N->g.sort=1;
	new_N->l.type=NT_TABLE;
	new_N->l.sort=1;
	new_N->r.type=NT_NULL;
	nc_strncpy(new_N->g.name, "!GLOBALS!", MAX_OBJNAMELEN);
	nc_strncpy(new_N->l.name, "!LOCALS!", MAX_OBJNAMELEN);
	nc_strncpy(new_N->r.name, "!RETVAL!", MAX_OBJNAMELEN);
	cobj=nes_settable(new_N, &new_N->g, "_globals_");
	cobj->d.table=new_N->g.d.table;
	cobj->mode|=NST_LINK;
	/* base functions */
	for (i=0;list[i].fn_name!=NULL;i++) {
		nes_setcfunc(new_N, &new_N->g, list[i].fn_name, list[i].fn_ptr);
	}
	/* io[] functions */
	cobj=nes_settable(new_N, &new_N->g, "io");
	cobj->mode|=NST_HIDDEN; /* sets hidden flag */
/*	cobj->mode^=NST_HIDDEN; */ /* strips hidden flag */
	for (i=0;list_io[i].fn_name!=NULL;i++) {
		nes_setcfunc(new_N, cobj, list_io[i].fn_name, list_io[i].fn_ptr);
	}
	/* math[] functions */
	cobj=nes_settable(new_N, &new_N->g, "math");
	cobj->mode|=NST_HIDDEN;
	for (i=0;list_math[i].fn_name!=NULL;i++) {
		nes_setcfunc(new_N, cobj, list_math[i].fn_name, list_math[i].fn_ptr);
	}
	/* str[] functions */
	cobj=nes_settable(new_N, &new_N->g, "string");
	cobj->mode|=NST_HIDDEN;
	for (i=0;list_string[i].fn_name!=NULL;i++) {
		nes_setcfunc(new_N, cobj, list_string[i].fn_name, list_string[i].fn_ptr);
	}
	cobj=nes_setnum(new_N, &new_N->g, "null", 0);
	cobj->type=NT_NULL; cobj->mode|=NST_SYSTEM;
	cobj=nes_setnum(new_N, &new_N->g, "false", 0);
	cobj->type=NT_BOOLEAN; cobj->mode|=NST_SYSTEM;
	cobj=nes_setnum(new_N, &new_N->g, "true", 1);
	cobj->type=NT_BOOLEAN; cobj->mode|=NST_SYSTEM;
	return new_N;
}

nes_state *nes_endstate(nes_state *N)
{
	if (N!=NULL) {
		if (N->outbuflen) nl_flush(N);
		nes_freetable(N, &N->l);
		nes_freetable(N, &N->g);
		n_free(N, (void *)&N);
	}
	return NULL;
}
