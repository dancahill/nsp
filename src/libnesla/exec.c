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
#include "nesla/libnesla.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef WIN32
#include <io.h>
#else
#ifdef __TURBOC__
#else
#include <unistd.h>
#endif
#endif

static obj_t *n_setargs(nes_state *N, char *fname)
{
	static char *fn="n_evalargs";
	obj_t listobj;
	obj_t *cobj, *nobj;
	unsigned short i;

	if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, fn, "missing '('");
	listobj.val=n_newval(N, NT_TABLE);
	listobj.val->attr=0;
	cobj=nes_setstr(N, &listobj, "0", fname, nc_strlen(fname));
	for (i=1;;i++) {
		N->lastop=OP_UNDEFINED;
		if ((nobj=nes_eval(N, (char *)N->readptr))!=NULL) {
			cobj->next=n_newiobj(N, i);
			cobj->next->prev=cobj;
			cobj=cobj->next;
			nes_linkval(N, cobj, nobj);
		}
		if (N->lastop==OP_UNDEFINED) nextop();
		if (N->lastop==OP_PCPAREN) break;
		if (N->lastop!=OP_PCOMMA) n_error(N, NE_SYNTAX, fn, "missing ')' or ','");
	}
	nes_linkval(N, &N->r, &listobj);
	nes_unlinkval(N, &listobj);
	return &N->r;
}

obj_t *n_execfunction(nes_state *N, obj_t *cobj)
{
	static char *fn="n_execfunction";
	NES_CFUNC cfunc;
	obj_t *cobj2;
	val_t *olobj;
	obj_t *pobj;
	uchar *p;
	unsigned int i;

	DEBUG_IN();
	if ((cobj->val->type!=NT_NFUNC)&&(cobj->val->type!=NT_CFUNC)) n_error(N, NE_SYNTAX, fn, "'%s' is not a function", cobj->name);
	nextop();
	if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, fn, "missing '('");
	pobj=n_setargs(N, cobj->name);
	N->lastop=OP_UNDEFINED;
	olobj=N->l.val; N->l.val=pobj->val; pobj->val=NULL;
	p=N->readptr;
	if (N->debug) n_warn(N, fn, "%s()", cobj->name);
	if (cobj->val->type==NT_CFUNC) {
		cfunc=cobj->val->d.cfunc;
		cfunc(N);
	} else if (cobj->val->type==NT_NFUNC) {
		N->readptr=(uchar *)cobj->val->d.str;
		nextop();
		if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, fn, "missing '('");
		for (i=1;;i++) {
			cobj2=nes_getiobj(N, &N->l, i);
			nextop();
			if (N->lastop==OP_LABEL) {
				nc_strncpy(cobj2->name, N->lastname, MAX_OBJNAMELEN);
				nextop();
			}
			if (N->lastop==OP_PCOMMA) continue;
			if (N->lastop==OP_PCPAREN) break;
			n_error(N, NE_SYNTAX, fn, "missing ')' or ','");
		}
		nextop();
		if (N->lastop==OP_POBRACE) nes_exec(N, (char *)N->readptr);
		if (N->ret) { N->ret=0; }
	}
	nes_unlinkval(N, &N->l);
	N->l.val=olobj;
	N->readptr=p;
	DEBUG_OUT();
	return &N->r;
}

/*
 * the following functions are public API functions
 */

obj_t *nes_exec(nes_state *N, char *string)
{
	static char *fn="nes_exec";
	char namebuf[MAX_OBJNAMELEN+1];
	obj_t *cobj, *tobj;
	char block;
	short int jmp=N->jmpset;
	short int single=N->single;

	DEBUG_IN();
	N->single=0;
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
			case OP_KDO:    n_do(N);    if (N->ret) goto end; else break;
			case OP_KWHILE: n_while(N); if (N->ret) goto end; else break;
			case OP_KFUNC:  n_readfunction(N); break;
			case OP_KBREAK:
				if ((!block)&&(!single)) n_error(N, NE_SYNTAX, fn, "return without block");
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
			case OP_KEXIT:
				N->err=nc_isdigit(*N->readptr)?(short int)n_getnumber(N):0;
				n_error(N, N->err, fn, "exiting normally");
			case OP_KELSE:  n_error(N, NE_SYNTAX, fn, "stray else");
			}
		} else {
			if (N->lastname[0]=='\0') n_error(N, NE_SYNTAX, fn, "zero length token");
			tobj=&N->l;
			cobj=nes_getobj(N, tobj, N->lastname);
			while (cobj->val->type==NT_TABLE) {
				tobj=cobj;
				if (*N->readptr!=OP_POBRACKET&&*N->readptr!=OP_PDOT&&*N->readptr!='['&&*N->readptr!='.') break;
				cobj=n_getindex(N, tobj, namebuf);
				if (nes_isnull(cobj)&&(namebuf[0]!=0)) {
					cobj=nes_setnum(N, tobj, namebuf, 0);
				}
			}
			if (nes_isnull(cobj)) {
				if (N->lastop!=OP_LABEL) n_error(N, NE_SYNTAX, fn, "expected a label");
				cobj=nes_setnum(N, tobj, N->lastname, 0);
			} else if ((cobj->val->type==NT_NFUNC)||(cobj->val->type==NT_CFUNC)) {
				n_execfunction(N, cobj);
				if (*N->readptr==OP_PSEMICOL||*N->readptr==';') nextop();
				if (single) break;
				continue;
			}
			n_readvar(N, tobj, cobj);
		}
		if (single) break;
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
	char *p, *pfile;
	int bl;
	int fp;
	int r;
	short int jmp=N->jmpset;
	int rc;

	if (jmp==0) {
		if (setjmp(N->savjmp)==0) {
			N->jmpset=1;
		} else {
			n_free(N, (void *)&rawtext);
			if (jmp==0) N->jmpset=0;
			return 0;
		}
	}
	pfile=file;
	if ((stat(pfile, &sb)!=0)&&(cobj->val->type==NT_STRING)) {
		nc_snprintf(N, buf, sizeof(buf), "%s/%s", cobj->val->d.str, pfile);
		if (stat(buf, &sb)!=0) { rc=-1; goto end; }
		pfile=buf;
	}
	if ((fp=open(pfile, O_RDONLY|O_BINARY))==-1) { rc=-1; goto end; }
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
	rc=sb.st_size;
	rawtext[sb.st_size]='\0';
	while ((rc>0)&&((rawtext[rc-1]=='\r')||(rawtext[rc-1]=='\n'))) rawtext[--rc]='\0';
	n_prechew(N, rawtext);
	nes_exec(N, (char *)rawtext);
	n_free(N, (void *)&rawtext);
	if (N->outbuflen) nl_flush(N);
	rc=0;
end:
	if (jmp==0) N->jmpset=0;
	return rc;
}

typedef struct {
	char *fn_name;
	NES_CFUNC fn_ptr;
} FUNCTION;

nes_state *nes_newstate()
{
	FUNCTION list[]={
		{ "date",	(NES_CFUNC)nl_datetime	},
		{ "include",	(NES_CFUNC)nl_include	},
		{ "print",	(NES_CFUNC)nl_print	},
		{ "printvar",	(NES_CFUNC)nl_printvar	},
		{ "runtime",	(NES_CFUNC)nl_runtime	},
		{ "size",	(NES_CFUNC)nl_size	},
		{ "sleep",	(NES_CFUNC)nl_sleep	},
		{ "system",	(NES_CFUNC)nl_system	},
		{ "time",	(NES_CFUNC)nl_datetime	},
		{ "tonumber",	(NES_CFUNC)nl_tonumber	},
		{ "tostring",	(NES_CFUNC)nl_tostring	},
		{ "type",	(NES_CFUNC)nl_type	},
		{ "write",	(NES_CFUNC)nl_print	},
		{ NULL, NULL }
	};
	FUNCTION list_file[]={
		{ "read",	(NES_CFUNC)nl_fileread	},
		{ "write",	(NES_CFUNC)nl_filewrite	},
		{ NULL, NULL }
	};
	FUNCTION list_io[]={
		{ "print",	(NES_CFUNC)nl_print	},
		{ "write",	(NES_CFUNC)nl_print	},
		{ "flush",	(NES_CFUNC)nl_flush	},
		{ NULL, NULL }
	};
	FUNCTION list_math[]={
		{ "abs",	(NES_CFUNC)nl_math1	},
		{ "ceil",	(NES_CFUNC)nl_math1	},
		{ "floor",	(NES_CFUNC)nl_math1	},
		{ "rand",	(NES_CFUNC)nl_math1	},
		{ NULL, NULL }
	};
	FUNCTION list_string[]={
		{ "cat",	(NES_CFUNC)nl_strcat	},
		{ "cmp",	(NES_CFUNC)nl_strcmp	},
		{ "icmp",	(NES_CFUNC)nl_strcmp	},
		{ "ncmp",	(NES_CFUNC)nl_strcmp	},
		{ "nicmp",	(NES_CFUNC)nl_strcmp	},
		{ "len",	(NES_CFUNC)nl_strlen	},
		{ "split",	(NES_CFUNC)nl_strsplit	},
		{ "str",	(NES_CFUNC)nl_strstr	},
		{ "istr",	(NES_CFUNC)nl_strstr	},
		{ "sub",	(NES_CFUNC)nl_strsub	},
		{ NULL, NULL }
	};
	nes_state *new_N;
	obj_t *cobj;
	short i;

	new_N=n_alloc(NULL, sizeof(nes_state));
	nc_memset(new_N, 0, sizeof(nes_state));
	nc_gettimeofday(&new_N->ttime, NULL);
	srand(new_N->ttime.tv_usec);

	new_N->g.val=n_alloc(new_N, sizeof(val_t));
	nc_memset(new_N->g.val, 0, sizeof(val_t));
	new_N->g.val->type=NT_TABLE;
	new_N->g.val->d.table=NULL;
	new_N->g.val->refs=1;
	new_N->g.val->attr|=NST_AUTOSORT;

	new_N->l.val=n_alloc(new_N, sizeof(val_t));
	nc_memset(new_N->l.val, 0, sizeof(val_t));
	new_N->l.val->type=NT_TABLE;
	new_N->l.val->d.table=NULL;
	new_N->l.val->refs=1;
	new_N->l.val->attr|=NST_AUTOSORT;

	nc_strncpy(new_N->g.name, "!GLOBALS!", MAX_OBJNAMELEN);
	nc_strncpy(new_N->l.name, "!LOCALS!", MAX_OBJNAMELEN);
	nc_strncpy(new_N->r.name, "!RETVAL!", MAX_OBJNAMELEN);
	cobj=nes_settable(new_N, &new_N->g, "_GLOBALS");
	cobj->val->attr|=NST_HIDDEN;
	nes_linkval(new_N, cobj, &new_N->g);

	for (i=0;list[i].fn_name!=NULL;i++) {
		nes_setcfunc(new_N, &new_N->g, list[i].fn_name, list[i].fn_ptr);
	}
	cobj=nes_settable(new_N, &new_N->g, "file");
	cobj->val->attr|=NST_HIDDEN;
	for (i=0;list_file[i].fn_name!=NULL;i++) {
		nes_setcfunc(new_N, cobj, list_file[i].fn_name, list_file[i].fn_ptr);
	}
	cobj=nes_settable(new_N, &new_N->g, "io");
	cobj->val->attr|=NST_HIDDEN;
	for (i=0;list_io[i].fn_name!=NULL;i++) {
		nes_setcfunc(new_N, cobj, list_io[i].fn_name, list_io[i].fn_ptr);
	}
	cobj=nes_settable(new_N, &new_N->g, "math");
	cobj->val->attr|=NST_HIDDEN;
	for (i=0;list_math[i].fn_name!=NULL;i++) {
		nes_setcfunc(new_N, cobj, list_math[i].fn_name, list_math[i].fn_ptr);
	}
	cobj=nes_settable(new_N, &new_N->g, "string");
	cobj->val->attr|=NST_HIDDEN;
	for (i=0;list_string[i].fn_name!=NULL;i++) {
		nes_setcfunc(new_N, cobj, list_string[i].fn_name, list_string[i].fn_ptr);
	}
	cobj=nes_setnum(new_N, &new_N->g, "null", 0);
	cobj->val->type=NT_NULL; cobj->val->attr|=NST_SYSTEM;
	cobj=nes_setnum(new_N, &new_N->g, "false", 0);
	cobj->val->type=NT_BOOLEAN; cobj->val->attr|=NST_SYSTEM;
	cobj=nes_setnum(new_N, &new_N->g, "true", 1);
	cobj->val->type=NT_BOOLEAN; cobj->val->attr|=NST_SYSTEM;
	cobj=nes_setstr(new_N, &new_N->g, "_version_", NESLA_VERSION, nc_strlen(NESLA_VERSION));
	return new_N;
}

nes_state *nes_endstate(nes_state *N)
{
	if (N!=NULL) {
		if (N->outbuflen) nl_flush(N);
		nes_freetable(N, &N->g);
		nes_freetable(N, &N->l);
		if (N->g.val) n_free(N, (void *)&N->g.val);
		if (N->l.val) n_free(N, (void *)&N->l.val);
		if (N->r.val) n_free(N, (void *)&N->r.val);
		n_free(N, (void *)&N);
	}
	return NULL;
}
