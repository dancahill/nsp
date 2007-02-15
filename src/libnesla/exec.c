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
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "libnesla.h"

#ifdef WIN32
#include <io.h>
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#else
#ifdef __TURBOC__
#else
#include <unistd.h>
#endif
#endif

#ifdef WIN32
int gettimeofday(struct timeval *tv, void *tz)
{
	struct timeb tb;
	struct tm *today;

	if (tv==NULL) return -1;
	ftime(&tb);
	today=localtime(&tb.time);
	tv->tv_sec=tb.time;
	tv->tv_usec=tb.millitm*1000;
	return 0;
}
#endif

int n_vsnprintf(nesla_state *N, char *str, int size, const char *format, va_list ap)
{
	int rc;

#ifdef __TURBOC__
	rc=vsprintf(str, format, ap);
#else
	rc=vsnprintf(str, size, format, ap);
#endif
	str[size-1]='\0';
	return rc;
}

int n_snprintf(nesla_state *N, char *str, int size, const char *format, ...)
{
	va_list ap;
	int rc;

	va_start(ap, format);
	rc=n_vsnprintf(N, str, size, format, ap);
	va_end(ap);
	return rc;
}

int n_printf(nesla_state *N, short dowrite, const char *format, ...)
{
	va_list ap;
	int rc;

	va_start(ap, format);
	rc=n_vsnprintf(N, N->txtbuf, sizeof(N->txtbuf)-1, format, ap);
	va_end(ap);
	if (dowrite) nl_write(N);
	return rc;
}

void n_error(nesla_state *N, short int err, const char *fname, const char *format, ...)
{
	char ptrtxt[MAX_OBJNAMLEN+1];
	char *ptemp=ptrtxt;
	va_list ap;
	int len;

	n_snprintf(N, ptrtxt, sizeof(ptrtxt)-1, "%s", N->readptr);
	while (*ptemp) {
		if ((*ptemp=='\n')||(*ptemp=='\r')||(*ptemp=='\t')) *ptemp=' ';
		ptemp++;
	}
	len=n_snprintf(N, N->errbuf, sizeof(N->errbuf)-1, "%-15s : ", fname);
	va_start(ap, format);
	len+=n_vsnprintf(N, N->errbuf+len, sizeof(N->errbuf)-len-1, format, ap);
	va_end(ap);
	n_snprintf(N, N->errbuf+len, sizeof(N->errbuf)-len-1, "\r\n\tN->readptr=\"%s\"\r\n", ptrtxt);
	N->err=err;
	longjmp(N->savjmp, 1);
	return;
}

void n_warn(nesla_state *N, const char *fname, const char *format, ...)
{
	char ptrtxt[MAX_OBJNAMLEN+1];
	char *p=ptrtxt;
	va_list ap;

	if (N->warnings++>10000) n_error(N, NE_SYNTAX, "n_warn", "too many warning lines (%d)\n", N->warnings);
	n_snprintf(N, ptrtxt, sizeof(ptrtxt)-1, "%s", N->readptr);
	while (*p) {
		if ((*p=='\n')||(*p=='\r')||(*p=='\t')) *p=' ';
		p++;
	}
	n_printf(N, 1, "[01;33;40m%-15s : ", fname);
	va_start(ap, format);
	n_vsnprintf(N, N->txtbuf, sizeof(N->txtbuf)-1, format, ap);
	va_end(ap);
	p=N->txtbuf+strlen(N->txtbuf);
	while (p<N->txtbuf+40) *p++=' ';
	N->txtbuf[40]='\0';
	nl_write(N);
	n_printf(N, 1, "N->readptr = %s\r\n[00m", ptrtxt);
	return;
}

char *ne_if(nesla_state *N)
{
	num_t op1=0;
	short doneif=0;
	obj_t *cobj;

l1:
	n_getop(N);
	if (N->lastop==OP_POPAREN) {
		N->lastop=OP_UNDEFINED;
		cobj=n_eval(N, ")");
		if (N->lastop!=OP_PCPAREN) n_warn(N, "ne_if", "expected a closing ) %d", N->lastop);
		op1+=cobj->d.number;
	}
l2:
	n_getop(N);
	if (N->lastop==OP_POBRACE) {
		if ((op1)&&(!doneif)) {
			nesla_exec(N, N->readptr);
			doneif=1;
			if (N->ret) return N->readptr;
		} else {
			n_skipto(N, '}');
		}
		n_getop(N);
		if (N->lastop!=OP_KELSE) {
			n_ungetop(N);
		} else {
			n_getop(N);
			if (N->lastop!=OP_KIF) {
				n_ungetop(N);
				op1=1;
				goto l2;
			} else {
				goto l1;
			}
		}
	}
	return N->readptr;
}

char *ne_for(nesla_state *N)
{
	char *arginit;
	char *argcomp;
	char *argexec;
	char *blockstart;
	char *blockend;
	obj_t *cobj;

	if (N->readptr==NULL) n_error(N, NE_SYNTAX, "ne_for", "NULL readptr");
	n_getop(N);
	if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, "ne_for", "missing bracket");
	arginit=N->readptr;
	argcomp=n_skipto(N, ';');
	argexec=n_skipto(N, ';');
	n_skipto(N, ')');
	blockstart=N->readptr;
	n_getop(N);
	n_skipto(N, '}');
	blockend=N->readptr;
	N->readptr=arginit;
	n_assign(N, &N->l);
	for (;;) {
		N->readptr=argcomp;
		cobj=n_eval(N, ";");
		if (N->lastop!=OP_PSEMICOL) n_error(N, NE_SYNTAX, "ne_for", "expected a closing ;");
		if (nesla_tofloat(N, cobj)==0) break;
		N->readptr=blockstart;
		n_getop(N);
		nesla_exec(N, N->readptr);
		if (N->brk>0) { N->brk--; break; }
		if (N->ret) { break; }
		N->readptr=argexec;
		n_assign(N, &N->l);
	}
	N->readptr=blockend;
	return N->readptr;
}

char *ne_while(nesla_state *N)
{
	char *argcomp;
	char *blockstart;
	char *blockend;
	obj_t *cobj;

	if (N->readptr==NULL) n_error(N, NE_SYNTAX, "ne_while", "EOF");
	n_getop(N);
	if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, "ne_while", "missing bracket");
	argcomp=N->readptr;
	n_skipto(N, ')');
	blockstart=N->readptr;
	n_getop(N);
	n_skipto(N, '}');
	blockend=N->readptr;
	for (;;) {
		N->readptr=argcomp;
		cobj=n_eval(N, ")");
		if (N->lastop!=OP_PCPAREN) n_error(N, NE_SYNTAX, "ne_while", "expected a closing ) bracket");
		if (nesla_tofloat(N, cobj)==0) break;
		N->readptr=blockstart;
		n_getop(N);
		nesla_exec(N, N->readptr);
		if (N->brk>0) { N->brk--; break; }
		if (N->ret) { break; }
	}
	N->readptr=blockend;
	return N->readptr;
}

obj_t *ne_macros(nesla_state *N)
{
	char tmpnam[MAX_OBJNAMLEN+1];
	obj_t *cobj=NULL, *tobj;
	char *ptemp;

	if (strcmp("exit", N->lastname)==0) {
		n_error(N, 0, "ne_macros", "exiting normally");
	} else if (strcmp("printvars", N->lastname)==0) {
		/* nesla_sorttable(N, &N->g, 1); */
		n_getop(N);
		if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, "ne_macros", "missing ( bracket");
		n_getop(N);
		if (N->lastop==OP_LABEL) {
			n_getlabel(N, tmpnam);
			n_getop(N);
			cobj=nesla_getobj(N, &N->l, tmpnam);
			if (cobj->type==NT_TABLE) {
				tmpnam[0]='\0';
				for (;;) {
					if (strchr("[.", *N->readptr)==NULL) break;
					cobj=n_getindex(N, cobj, tmpnam);
					if (cobj->type!=NT_TABLE) break;
				}
				nesla_printvars(N, cobj);
			}
		} else if (N->lastop==OP_PCPAREN) {
			nesla_printvars(N, N->g.d.table);
		}
		if (N->lastop!=OP_PCPAREN) n_error(N, NE_SYNTAX, "ne_macros", "missing ) bracket");
		N->lastop=OP_UNDEFINED;
		return nesla_regnum(N, &N->g, "_retval", 0);
	} else if (strcmp("type", N->lastname)==0) {
		n_getop(N);
		if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, "ne_macros", "missing ( bracket");
		n_getop(N);
		if (N->lastop!=OP_LABEL) n_error(N, NE_SYNTAX, "ne_macros", "missing arg1");
		n_getlabel(N, tmpnam);
		cobj=nesla_getobj(N, &N->l, tmpnam);
		if (cobj->type==NT_TABLE) {
			tmpnam[0]='\0';
			for (;;) {
				tobj=cobj;
				if (strchr("[.", *N->readptr)==NULL) break;
				cobj=n_getindex(N, tobj, tmpnam);
				if (cobj->type!=NT_TABLE) break;
			}
			if (strlen(tmpnam)) cobj=nesla_getobj(N, tobj, tmpnam);
		}
		switch (cobj->type) {
		case NT_BOOLEAN : ptemp="boolean";   break;
		case NT_NUMBER  : ptemp="number";    break;
		case NT_STRING  : ptemp="string";    break;
		case NT_TABLE   : ptemp="table";     break;
		case NT_NFUNC   : ptemp="function";  break;
		case NT_CFUNC   : ptemp="cfunction"; break;
		default         : ptemp="null";      break;
		}
		cobj=nesla_regstr(N, &N->g, "_retval", ptemp);
		n_getop(N);
		if (N->lastop!=OP_PCPAREN) n_error(N, NE_SYNTAX, "ne_macros", "missing ) bracket");
		N->lastop=OP_UNDEFINED;
		cobj=nesla_regstr(N, &N->g, "_retval", ptemp);
	}
	return cobj;
}

obj_t *ne_execfunction(nesla_state *N, obj_t *cobj)
{
	char tmpnam[MAX_OBJNAMLEN+1];
	NESLA_CFUNC cfunc;
	obj_t *cobj2;
	obj_t *olobj;
	obj_t *pobj;
	char *ptemp;
	unsigned int i;

	if ((cobj->type!=NT_NFUNC)&&(cobj->type!=NT_CFUNC)) n_error(N, NE_SYNTAX, "nesla_execfunction", "'%s' is not a function", cobj->name);
	n_getop(N);
	if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, "ne_execfunction", "missing arg bracket");
	pobj=n_evalargs(N, cobj->name);
	N->lastop=OP_UNDEFINED;
	olobj=N->l.d.table; N->l.d.table=pobj;
	ptemp=N->readptr;
	if (N->debug) n_warn(N, "ne_execfunction", "%s()", cobj->name);
	if (cobj->type==NT_CFUNC) {
		cfunc=(NESLA_CFUNC)cobj->d.cfunction;
		cfunc(N);
	} else if (cobj->type==NT_NFUNC) {
		N->readptr=cobj->d.function;
		n_getop(N);
		if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, "ne_execfunction", "missing bracket");
		for (i=1;;i++) {
			n_snprintf(N, tmpnam, MAX_OBJNAMLEN, "!%d", i);
			cobj2=nesla_getobj(N, &N->l, tmpnam);
			n_getop(N);
			if (N->lastop==OP_LABEL) {
				n_getlabel(N, tmpnam);
				n_getop(N);
			}
			strncpy(cobj2->name, tmpnam, MAX_OBJNAMLEN);
			if (N->lastop==OP_PCOMMA) continue;
			if (N->lastop==OP_PCPAREN) break;
			n_error(N, NE_SYNTAX, "ne_execfunction", "i'm confused.... %s", tmpnam);
		}
		n_getop(N);
		if (N->lastop==OP_POBRACE) nesla_exec(N, N->readptr);
		if (N->ret) { N->ret=0; }
	}
	nesla_freetable(N, &N->l);
	N->l.d.table=olobj;
	N->readptr=ptemp;
	return nesla_getobj(N, &N->g, "_retval");
}

char *nesla_exec(nesla_state *N, char *string)
{
	obj_t *cobj;
	short block=0;

	N->readptr=string;
	if (N->lastop==OP_POBRACE) block=1;
	for (;;) {
		if (N->readptr==NULL) goto end;
		if ((block)&&(N->brk>0)) goto end;
		n_getop(N);
		if (*N->readptr=='\0') goto end;
		if ((block)&&(N->lastop==OP_PCBRACE)) break;
		if (OP_ISMATH(N->lastop)) {
			n_warn(N, "nesla_exec", "unexpected math op '%s'", N->lastname);
		} else if (OP_ISPUNC(N->lastop)) {
			n_warn(N, "nesla_exec", "unexpected punctuation '%s'", N->lastname);
		} else if (OP_ISKEY(N->lastop)) {
			switch (N->lastop) {
			case OP_KBREAK:
				if (!block) n_error(N, NE_SYNTAX, "nesla_exec", "return without block");
				N->brk=isdigit(*N->readptr)?(short int)n_getnumber(N):1;
				if (*N->readptr==';') n_getop(N);
				goto end;
			case OP_KCONT:
				if (!block) n_error(N, NE_SYNTAX, "nesla_exec", "continue without block");
				if (*N->readptr==';') n_getop(N);
				goto end;
			case OP_KRET:
				n_storevar(N, &N->g, "_retval");
				N->ret=1;
				goto end;
			case OP_KFUNC:  n_storefunction(N); break;
			case OP_KGLOB:  n_assign(N, &N->g); break;
			case OP_KLOCAL: n_assign(N, &N->l); break;
			case OP_KVAR:   n_assign(N, &N->l); break;
			case OP_KIF:    ne_if(N);    if (N->ret) return N->readptr; else break;
			case OP_KELSE:  n_error(N, NE_SYNTAX, "nesla_exec", "stray else");
			case OP_KFOR:   ne_for(N);   if (N->ret) return N->readptr; else break;
			case OP_KWHILE: ne_while(N); if (N->ret) return N->readptr; else break;
			}
		} else {
			if (N->lastname[0]=='\0') n_error(N, NE_SYNTAX, "nesla_exec", "zero length token");
			if ((cobj=ne_macros(N))!=NULL) {
				if (*N->readptr==';') n_getop(N);
				continue;
			}
			cobj=nesla_getobj(N, &N->g, N->lastname);
			if ((cobj->type==NT_NFUNC)||(cobj->type==NT_CFUNC)) {
				ne_execfunction(N, cobj);
				if (*N->readptr==';') n_getop(N);
				continue;
			} else if (cobj->type==NT_NULL) {
				/* n_warn(N, "nesla_exec", "reference to undefined symbol '%s'", N->lastname); */
				N->readptr=N->lastptr;
				n_assign(N, &N->l);
				continue;
			} else {
				/* n_error(N, NE_SYNTAX, "nesla_exec", "possible breakage here '%s'", N->lastname); */
				N->readptr=N->lastptr;
				n_assign(N, &N->l);
				continue;
			}
		}
	}
end:
	return N->readptr;
}

#ifndef O_BINARY
#define O_BINARY 0
#endif
int nesla_execfile(nesla_state *N, char *file)
{
	char buf[512];
	struct stat sb;
	char *rawtext;
	char *ptemp;
	int bl;
	int fp;
	int r;
	obj_t *cobj=nesla_getobj(N, &N->g, "_filepath");

	if (stat(file, &sb)!=0) {
		if (cobj->type==NT_STRING) {
			strncpy(buf, cobj->d.string, sizeof(buf)-strlen(file)-2);
			strcat(buf, "/");
			strcat(buf, file);
			if (stat(buf, &sb)!=0) {
				return -1;
			}
			file=buf;
		}
	}
/*	if (sb.st_mode&S_IFDIR) return -1; */
	if ((fp=open(file, O_RDONLY|O_BINARY))==-1) {
		return -1;
	}
	rawtext=n_alloc(N, (sb.st_size+2)*sizeof(char));
	ptemp=rawtext;
	bl=sb.st_size;
	for (;;) {
		r=read(fp, ptemp, bl);
		ptemp+=r;
		bl-=r;
		if (bl<1) break;
	}
	close(fp);
	nesla_exec(N, rawtext);
	n_free(N, (void *)&rawtext);
	return 0;
}

typedef struct {
	char *fn_name;
	void *fn_ptr;
} FUNCTION;

nesla_state *nesla_newstate()
{
	FUNCTION list[]={
		/* basic processing */
		{ "date",	(NESLA_CFUNC *)nl_datetime	},
		{ "include",	(NESLA_CFUNC *)nl_include	},
		{ "number",	(NESLA_CFUNC *)nl_number	},
		{ "print",	(NESLA_CFUNC *)nl_print		},
		{ "runtime",	(NESLA_CFUNC *)nl_runtime	},
		{ "sleep",	(NESLA_CFUNC *)nl_sleep		},
		{ "time",	(NESLA_CFUNC *)nl_datetime	},
		{ NULL, NULL }
	};
	FUNCTION list_io[]={
		/* basic io */
		{ "print",	(NESLA_CFUNC *)nl_print		},
		{ "write",	(NESLA_CFUNC *)nl_write		},
		{ NULL, NULL }
	};
	FUNCTION list_math[]={
		/* math functions */
		{ "abs",	(NESLA_CFUNC *)nl_math1		},
		{ "acos",	(NESLA_CFUNC *)nl_math1		},
		{ "asin",	(NESLA_CFUNC *)nl_math1		},
		{ "atan",	(NESLA_CFUNC *)nl_math1		},
		{ "ceil",	(NESLA_CFUNC *)nl_math1		},
		{ "cos",	(NESLA_CFUNC *)nl_math1		},
		{ "floor",	(NESLA_CFUNC *)nl_math1		},
		{ "rand",	(NESLA_CFUNC *)nl_math1		},
		{ "sin",	(NESLA_CFUNC *)nl_math1		},
		{ "sqrt",	(NESLA_CFUNC *)nl_math1		},
		{ "tan",	(NESLA_CFUNC *)nl_math1		},
		{ NULL, NULL }
	};
	FUNCTION list_string[]={
		/* strings */
		{ "cat",	(NESLA_CFUNC *)nl_strcat	},
		{ "cmp",	(NESLA_CFUNC *)nl_strcmp	},
		{ "icmp",	(NESLA_CFUNC *)nl_strcmp	},
		{ "ncmp",	(NESLA_CFUNC *)nl_strcmp	},
		{ "nicmp",	(NESLA_CFUNC *)nl_strcmp	},
		{ "len",	(NESLA_CFUNC *)nl_strlen	},
		{ "str",	(NESLA_CFUNC *)nl_strstr	},
		{ "istr",	(NESLA_CFUNC *)nl_strstr	},
		{ "sub",	(NESLA_CFUNC *)nl_strsub	},
		{ NULL, NULL }
	};
	nesla_state *new_N;
	obj_t *cobj;
	int i;

	new_N=n_alloc(NULL, sizeof(nesla_state));
	new_N->g.type=NT_TABLE;
	new_N->l.type=NT_TABLE;
	strncpy(new_N->g.name, "!GLOBALS!", sizeof(new_N->g.name)-1);
	strncpy(new_N->l.name, "!LOCALS!", sizeof(new_N->l.name)-1);
	/* io[] functions */
	cobj=nesla_regtable(new_N, &new_N->g, "_globals_");
	cobj->d.table=new_N->g.d.table;
	cobj=nesla_regtable(new_N, &new_N->g, "io");
	cobj->mode|=NST_HIDDEN; /* sets hidden flag */
/*	cobj->mode^=NST_HIDDEN; */ /* strips hidden flag */
	for (i=0;;i++) {
		if ((list_io[i].fn_name==NULL)||(list_io[i].fn_ptr==NULL)) break;
		nesla_regcfunc(new_N, cobj, list_io[i].fn_name, list_io[i].fn_ptr);
	}
	/* base functions */
	for (i=0;;i++) {
		if ((list[i].fn_name==NULL)||(list[i].fn_ptr==NULL)) break;
		nesla_regcfunc(new_N, &new_N->g, list[i].fn_name, list[i].fn_ptr);
	}
	/* math[] functions */
	cobj=nesla_regtable(new_N, &new_N->g, "math");
	cobj->mode|=NST_HIDDEN;
	for (i=0;;i++) {
		if ((list_math[i].fn_name==NULL)||(list_math[i].fn_ptr==NULL)) break;
		nesla_regcfunc(new_N, cobj, list_math[i].fn_name, list_math[i].fn_ptr);
	}
	/* str[] functions */
	cobj=nesla_regtable(new_N, &new_N->g, "string");
	cobj->mode|=NST_HIDDEN;
	for (i=0;;i++) {
		if ((list_string[i].fn_name==NULL)||(list_string[i].fn_ptr==NULL)) break;
		nesla_regcfunc(new_N, cobj, list_string[i].fn_name, list_string[i].fn_ptr);
	}
	cobj=nesla_regnum(new_N, &new_N->g, "false", 0);
	cobj->mode|=NST_HIDDEN;
/*	cobj->type=NT_BOOLEAN; cobj->d.boolean=0; */
	cobj=nesla_regnum(new_N, &new_N->g, "true", 1);
	cobj->mode|=NST_HIDDEN;
/*	cobj->type=NT_BOOLEAN; cobj->d.boolean=1; */
	gettimeofday(&new_N->ttime, NULL);
	srand(new_N->ttime.tv_usec);
	return new_N;
}

nesla_state *nesla_endstate(nesla_state *N)
{
	if (N!=NULL) {
		nesla_freetable(N, &N->l);
		nesla_freetable(N, &N->g);
		n_free(N, (void *)&N);
	}
	return NULL;
}
