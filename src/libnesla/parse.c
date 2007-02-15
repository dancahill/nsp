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
#include <math.h>
#include "libnesla.h"

int n_assign2(nesla_state *N, obj_t *tobj, char *name, short op);

typedef struct {
	char *name;
	short val;
} optab;

static optab oplist[] = {
	/* PUNCTUATION */
	{ "{",        OP_POBRACE   },
	{ "(",        OP_POPAREN   },
	{ "}",        OP_PCBRACE   },
	{ ")",        OP_PCPAREN   },
	{ ",",        OP_PCOMMA    },
	{ ";",        OP_PSEMICOL  },
	{ ".",        OP_PDOT      },
	{ "[",        OP_POBRACKET },
	{ "]",        OP_PCBRACKET },
	{ "\'",       OP_PSQUOTE   },
	{ "\"",       OP_PDQUOTE   },
	{ "#",        OP_PHASH     },
	/* MATH */
	{ "=",        OP_MEQ       },
	{ "+",        OP_MADD      },
	{ "-",        OP_MSUB      },
	{ "*",        OP_MMUL      },
	{ "/",        OP_MDIV      },
	{ "+=",       OP_MADDEQ    },
	{ "-=",       OP_MSUBEQ    },
	{ "*=",       OP_MMULEQ    },
	{ "/=",       OP_MDIVEQ    },
	{ "++",       OP_MADDADD   },
	{ "--",       OP_MSUBSUB   },
	{ "%",        OP_MMOD      },
	{ "&",        OP_MAND      },
	{ "|",        OP_MOR       },
	{ "^",        OP_MXOR      },
	{ "&&",       OP_MLAND     },
	{ "||",       OP_MLOR      },
	{ "!",        OP_MLNOT     },
	{ "==",       OP_MCEQ      },
	{ "!=",       OP_MCNE      },
	{ "<=",       OP_MCLE      },
	{ ">=",       OP_MCGE      },
	{ "<",        OP_MCLT      },
	{ ">",        OP_MCGT      },
	/* KEYWORDS */
	{ "break",    OP_KBREAK    },
	{ "continue", OP_KCONT     },
	{ "return",   OP_KRET      },
	{ "function", OP_KFUNC     },
	{ "global",   OP_KGLOB     },
	{ "local",    OP_KLOCAL    },
	{ "var",      OP_KVAR      },
	{ "if",       OP_KIF       },
	{ "else",     OP_KELSE     },
	{ "for",      OP_KFOR      },
	{ "while",    OP_KWHILE    },
	{ "exit",     OP_KEXIT     },
	{ NULL,       0            }
};

static num_t n_domath(nesla_state *N, short op, num_t op1, num_t op2)
{
	num_t val;

	switch (op) {
		case OP_MADD  : val = op1 + op2; break;
		case OP_MSUB  : val = op1 - op2; break;
		case OP_MMUL  : val = op1 * op2; break;
		case OP_MDIV  : val = op1 / op2; break;
		case OP_MMOD  : val = fmod(op1, op2); break;
		case OP_MAND  : val = (int)op1 & (int)op2; break;
		case OP_MOR   : val = (int)op1 | (int)op2; break;
		case OP_MXOR  : val = (int)op1 ^ (int)op2; break;
		case OP_MLAND : val = op1 && op2; break;
		case OP_MLOR  : val = op1 || op2; break;
		case OP_MCEQ  : val = op1 == op2; break;
		case OP_MCNE  : val = op1 != op2; break;
		case OP_MCLE  : val = op1 <= op2; break;
		case OP_MCGE  : val = op1 >= op2; break;
		case OP_MCLT  : val = op1 < op2; break;
		case OP_MCGT  : val = op1 > op2; break;
		default : val=0;
	}
	return val;
}

static void sanetest(nesla_state *N, char *fname)
{
	if (N->readptr==NULL) n_error(N, NE_SYNTAX, fname, "NULL readptr");
	return;
}

/* Advance to next non-comment & retreive data */
char *n_skipcomment(nesla_state *N)
{
	sanetest(N, "n_skipcomment");
	for (;;) {
		switch (*N->readptr) {
		case '\r' :
		case '\n' :
		case '\0' :
			goto end;
		default:
			N->readptr++;
		}
	}
end:
	return N->readptr;
}

/* Advance to next non-blank & retreive data */
char *n_skipblank(nesla_state *N)
{
	for (;;) {
		if (*N->readptr=='\0') break;
		if (*N->readptr=='#') n_skipcomment(N);
		if (strchr(" \r\n\t", *N->readptr)==NULL) break;
		N->readptr++;
	}
	return N->readptr;
}

static char *n_skipquote(nesla_state *N, char q)
{
	short esc=0;

	sanetest(N, "n_skipquote");
	for (;;) {
		if (*N->readptr=='\0') break;
		if ((*N->readptr==q)&&(esc==0)) { N->readptr++; break; }
		if (*N->readptr=='\\') esc=1; else esc=0;
		N->readptr++;
	}
	return N->readptr;
}

/* Advance to next specified char */
char *n_skipto(nesla_state *N, char c)
{
	int i;
	char x;

	sanetest(N, "n_skipto");
	for (i=1;i>0;) {
		if (*N->readptr=='\0') n_error(N, NE_SYNTAX, "n_skipto", "\\0 error %c %d", c, i);
		if (strchr("# \r\n\t", *N->readptr)!=NULL) n_skipblank(N);
		if (strchr("'\"", *N->readptr)!=NULL) { x=*N->readptr; N->readptr++; n_skipquote(N, x); }
		if (c=='}') {
			if (*N->readptr=='{') i++; else if (*N->readptr=='}') i--;
		} else if (c==')') {
			if (*N->readptr=='(') i++; else if (*N->readptr==')') i--;
		} else if (*N->readptr==c) {
			i--;
		}
		N->readptr++;
	}
	return N->readptr;
}

void n_ungetop(nesla_state *N)
{
	N->readptr=N->lastptr;
	N->lastop=OP_UNDEFINED;
	N->lastname[0]='\0';
	return;
}

/* return the next op/cmp and optionally advance the readptr */
short n_getop(nesla_state *N)
{
	char *p;
	int i=0;

	sanetest(N, "n_getop");
	N->lastop=OP_UNDEFINED;
	N->lastname[0]='\0';
	p=n_skipblank(N);
	if (IS_DATA(*N->readptr)) { N->lastop=OP_DATA; return N->lastop; }
	if (strchr("+-*/%&|^=!<>", *N->readptr)!=NULL) {
		for (i=0;i<MAX_OBJNAMLEN;i++) {
			if (*N->readptr=='\0') break;
			if (strchr("+-*/%&|^=!<>", *N->readptr)==NULL) break;
			N->lastname[i]=*N->readptr++;
		}
	/*                "{(}),;.[]\'\"" */
	} else if (strchr("{(}),;.[]", *N->readptr)!=NULL) {
		N->lastname[0]=*N->readptr++;
		i=1;
	} else if (isalpha(*N->readptr)||(*N->readptr=='_')||(*N->readptr=='$')) {
		for (i=0;i<MAX_OBJNAMLEN;i++) {
			if (*N->readptr=='\0') break;
			if (!isalnum(*N->readptr)&&(*N->readptr!='_')&&(*N->readptr!='$')) break;
			N->lastname[i]=*N->readptr++;
		}
		if (i>0) N->lastop=OP_LABEL;
	} else if (isdigit(*N->readptr)||(*N->readptr=='\'')||(*N->readptr=='\"')) {
		N->lastop=OP_DATA;
	}
	N->lastname[i]='\0';
	if (N->lastname[0]=='\0') {
		N->lastptr=p;
		return N->lastop;
	}
	for (i=0;;i++) {
		if (oplist[i].name==NULL) break;
		if (strcmp(oplist[i].name, N->lastname)==0) {
			N->lastop=oplist[i].val;
		}
	}
	n_skipblank(N);
	if (N->debug) n_warn(N, "n_getop", "[%s]", N->lastname);
	N->lastptr=p;
	return N->lastop;
}

/* return a number val from readptr */
num_t n_getnumber(nesla_state *N)
{
	char fbuf[MAX_OBJNAMLEN+1];
	num_t retval=0;
	short dots=0;
	int i=0;

	sanetest(N, "n_getnumber");
	N->lastop=OP_UNDEFINED;
	fbuf[i]='\0';
	if (isdigit(*N->readptr)) {
		for (i=0;i<MAX_OBJNAMLEN;i++) {
			if (i==0) if ((*N->readptr=='-')||(*N->readptr=='+')) { fbuf[i]=*N->readptr++; continue; }
			if (*N->readptr=='\0') break;
			if (isdigit(*N->readptr)) {
				fbuf[i]=*N->readptr++;
			} else if ((*N->readptr=='.')&&(dots==0)) {
				fbuf[i]=*N->readptr++;
				dots++;
			} else {
				break;
			}
		}
	} else {
		n_warn(N, "n_getnumber", "expected a number [%s]", fbuf);
	}
	fbuf[i]='\0';
	retval=atof(fbuf);
	if (N->debug) n_warn(N, "n_getnumber", "[%s]", fbuf);
	n_skipblank(N);
	return retval;
}

/* return the next function/value/var name */
char *n_getlabel(nesla_state *N, char *nambuf)
{
	if (N->lastop!=OP_LABEL) n_error(N, NE_SYNTAX, "n_getlabel", "expected a label");
	nambuf[0]='\0';
	strncpy(nambuf, N->lastname, MAX_OBJNAMLEN);
	N->lastop=OP_UNDEFINED;
	if (N->debug) n_warn(N, "n_getlabel", "[%s]", nambuf);
	n_skipblank(N);
	return nambuf;
}

/* return the next quoted block */
obj_t *n_getquote(nesla_state *N)
{
	obj_t *cobj;
	char q=*N->readptr;
	char *qs;
	char *qe;
	unsigned int len;

	if (N->debug) n_warn(N, "n_getquote", "snarfing quote");
	N->lastop=OP_UNDEFINED;
	if ((q!='\'')&&(q!='\"')) return NULL;
	N->readptr++;
	qs=N->readptr;
	n_skipquote(N, q);
	qe=N->readptr;
	len=qe-qs-1;
	cobj=nesla_regstr(N, &N->g, "_retval", NULL);
	cobj->d.string=n_alloc(N, (len+1)*sizeof(char));
	cobj->size=len;
	memcpy(cobj->d.string, qs, len);
	cobj->d.string[len]='\0';
	n_skipblank(N);
	return cobj;
}

/* return the next table index */
obj_t *n_getindex(nesla_state *N, obj_t *tobj, char *lastname)
{
	char tmpnam[MAX_OBJNAMLEN+1];
	obj_t *cobj=tobj;
	obj_t *nobj;
	short raw=0;

	sanetest(N, "n_getindex");
	tmpnam[0]='\0';
	lastname[0]='\0';
	if (cobj==NULL) n_error(N, NE_SYNTAX, "n_getindex", "null cobj");
	if (cobj->type!=NT_TABLE) return cobj;
	n_getop(N);
	if (N->lastop==OP_PDOT) {
		if (n_getop(N)!=OP_LABEL) n_error(N, NE_SYNTAX, "n_getindex", "expected a label");
		n_getlabel(N, tmpnam);
	} else if (N->lastop==OP_POBRACKET) {
		if (*N->readptr=='@') {
			N->readptr++;
			raw=1;
		}
		cobj=n_eval(N, "]");
		if (N->lastop!=OP_PCBRACKET) n_error(N, NE_SYNTAX, "n_getindex", "expected a closing ']'");
		strncpy(tmpnam, nesla_tostr(N, cobj), MAX_OBJNAMLEN);
		if (N->debug) n_warn(N, "n_getindex", "a[%d '%s' '%s' ]", cobj->type, cobj->name, tmpnam);
		if (raw) {
			cobj=nesla_getiobj(N, tobj, atoi(tmpnam));
			N->lastop=OP_UNDEFINED;
			return cobj;
		}
	} else {
		N->readptr=N->lastptr;
		N->lastop=OP_UNDEFINED;
		return cobj;
	}
	N->lastop=OP_UNDEFINED;
	nobj=nesla_getobj(N, tobj, tmpnam);
	if (nobj->type!=NT_TABLE)  {
		strncpy(lastname, tmpnam, MAX_OBJNAMLEN);
		return cobj;
	}
	if (N->debug) n_warn(N, "n_getindex", "b[%d '%s' '%s']", cobj->type, cobj->name, cobj->type==NT_STRING?cobj->d.string:"");
	return nobj;
}

/*
 above was basic tokenizing stuff.
 below is actual processing.
 */

obj_t *n_evalsub(nesla_state *N, char *end)
{
	char tmpnam[MAX_OBJNAMLEN+1];
	obj_t *cobj, *cobj2;
	num_t rval;
	short preop=0;

	sanetest(N, "n_evalsub");
	if (*N->readptr=='\0') return NULL;
	if (N->lastop==OP_UNDEFINED) n_getop(N);
	if (IS_STMTEND(N->lastop)) {
		return NULL;
	} else if (N->lastop==OP_POPAREN) {
		N->lastop=OP_UNDEFINED;
		cobj=n_eval(N, ")");
		if (N->lastop!=OP_PCPAREN) n_error(N, NE_SYNTAX, "n_evalsub", "expected a closing '('");
		return cobj;
	} else if (N->lastop==OP_DATA) {
		if ((*N->readptr=='"')||(*N->readptr=='\'')) {
			return n_getquote(N);
		} else if (isdigit(*N->readptr)) {
			N->lastop=OP_UNDEFINED;
			return nesla_regnum(N, &N->g, "_retval", n_getnumber(N));
		} else {
			n_error(N, NE_SYNTAX, "n_evalsub", "expected data");
		}
	}
	if (OP_ISMATH(N->lastop)) {
		if ((N->lastop==OP_MADDADD)||(N->lastop==OP_MSUBSUB)) preop=N->lastop;
		if ((N->lastop==OP_MADD)||(N->lastop==OP_MSUB)) preop=N->lastop;
		n_getop(N);
	}
	if ((N->lastop==OP_DATA)&&isdigit(*N->readptr)) {
		rval=n_getnumber(N);
		if (preop==OP_MSUB) rval=-rval;
		N->lastop=OP_UNDEFINED;
		return nesla_regnum(N, &N->g, "_retval", rval);
	}
	if (N->lastop!=OP_LABEL) {
		n_ungetop(N);
		return NULL;
	}
	n_getlabel(N, tmpnam);
	cobj=nesla_getobj(N, &N->l, N->lastname);
	if (cobj->type==NT_TABLE) {
		if (N->debug) n_warn(N, "n_evalsub", "[%d '%s' '%s']", cobj->type, cobj->name, N->lastname);
		for (;;) {
			cobj2=cobj;
			if (strchr("[.", *N->readptr)==NULL) break;
			cobj=n_getindex(N, cobj2, tmpnam);
			if (cobj->type!=NT_TABLE) break;
		}
		if (tmpnam[0]!='\0') {
			cobj=nesla_getobj(N, cobj2, tmpnam);
		}
		if ((cobj->type!=NT_NFUNC)&&(cobj->type!=NT_CFUNC)) {
			return cobj;
		}
	}
	if ((cobj->type==NT_NFUNC)||(cobj->type==NT_CFUNC)) {
		return ne_execfunction(N, cobj);
	} else if (cobj->type==NT_NUMBER) {
		if ((strncmp(N->readptr, "++", 2)==0)||(strncmp(N->readptr, "--", 2)==0)) {
			n_getop(N);
		}
		if (preop==OP_MADDADD) {
			cobj->d.number++;
		} else if (preop==OP_MSUBSUB) {
			cobj->d.number--;
		}
		rval=cobj->d.number;
		if (preop==OP_MSUB) rval=-rval;
		if (N->lastop==OP_MADDADD) {
			cobj->d.number++;
		} else if (N->lastop==OP_MSUBSUB) {
			cobj->d.number--;
		}
		N->lastop=OP_UNDEFINED;
		return nesla_regnum(N, &N->g, "_retval", rval);
	} else if (cobj->type==NT_STRING) {
		return nesla_regstr(N, &N->g, "_retval", cobj->d.string);
	} else if ((cobj2=ne_macros(N))!=NULL) {
		return cobj2;
	} else if (cobj->type==NT_NULL) {
		return nesla_regnull(N, &N->g, "_retval");
	} else {
		n_error(N, NE_SYNTAX, "n_evalsub", "unhandled type [%s]", N->lastname);
	}
	return NULL;
}

obj_t *n_eval(nesla_state *N, char *end)
{
	obj_t *cobj1, *cobj2;
	num_t val=0, val2=0;
	short op=0;

	sanetest(N, "n_eval");
	if (*N->readptr=='\0') return NULL;
	n_getop(N);
	if (N->lastop==OP_POPAREN) {
		N->lastop=OP_UNDEFINED;
		cobj1=n_eval(N, ")");
		if (N->lastop!=OP_PCPAREN) n_warn(N, "n_eval", "expected a closing ')'");
		n_getop(N);
	} else {
		cobj1=n_evalsub(N, end);
	}
	if (cobj1==NULL) return NULL;
	if (N->lastop==OP_UNDEFINED) n_getop(N);
	if (IS_STMTEND(N->lastop)) return cobj1;
	if (cobj1->type!=NT_NUMBER) return cobj1;
	val=cobj1->d.number;
	for (;;) {
		if (*N->readptr=='\0') break;
		op=N->lastop;
		if (op==OP_UNDEFINED) op=n_getop(N);
		if (IS_STMTEND(N->lastop)) break;
		if (!OP_ISMATH(N->lastop)) break;
		if (n_getop(N)==OP_POPAREN) {
			N->lastop=OP_UNDEFINED;
			cobj2=n_eval(N, ")");
			if (N->lastop!=OP_PCPAREN) n_warn(N, "n_eval", "2 expected a closing ')'");
			n_getop(N);
		} else {
			cobj2=n_evalsub(N, end);
		}
		if (cobj2==NULL) break;
		if (cobj2->type==NT_NUMBER) {
			val2=cobj2->d.number;
		}
		val=n_domath(N, op, val, val2);
	}
	return nesla_regnum(N, &N->g, "_retval", val);
}

obj_t *n_evalargs(nesla_state *N, char *a0name)
{
	char name[MAX_OBJNAMLEN+1];
	char tname[MAX_OBJNAMLEN+1];
	obj_t pobj;
	obj_t *cobj;
	unsigned int i;

	sanetest(N, "n_evalargs");
	if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, "n_evalargs", "missing bracket");
	memset((char *)&pobj, 0, sizeof(pobj));
	pobj.type=NT_TABLE;
	nesla_regstr(N, &pobj, "!0", a0name);
	for (i=1;;) {
		n_snprintf(N, name, MAX_OBJNAMLEN, "!%d", i);
		N->lastop=OP_UNDEFINED;
		cobj=n_eval(N, ",)");
		if ((N->lastop!=OP_PCPAREN)&&(N->lastop!=OP_PCOMMA)) n_error(N, NE_SYNTAX, "n_evalargs", "expected a closing ')'");
		if (N->lastop==OP_UNDEFINED) n_getop(N);
_retry:
		if (cobj!=NULL) {
			if (cobj->type==NT_STRING) {
				nesla_regstr(N, &pobj, name, cobj->d.string);
			} else if (cobj->type==NT_NUMBER) {
				nesla_regnum(N, &pobj, name, cobj->d.number);
			} else if (cobj->type==NT_NULL) {
				nesla_regnull(N, &pobj, name);
			} else if ((cobj->type==NT_NFUNC)||(cobj->type==NT_CFUNC)) {
				n_error(N, NE_SYNTAX, "n_evalargs", "storing a value as a func? %s", name);
			} else if (cobj->type==NT_TABLE) {
				for (;;) {
					if (strchr("[.", *N->readptr)==NULL) break;
					cobj=n_getindex(N, cobj, tname);
				}
				if (N->debug) n_warn(N, "n_evalargs", "a[%s][%d][%s][%s]", name, cobj->type, cobj->name, nesla_tostr(N, cobj));
				if (cobj->type!=NT_TABLE) goto _retry;
			} else {
				n_error(N, NE_SYNTAX, "n_evalargs", "b[%s][%d][%s][%s]", name, cobj->type, cobj->name, nesla_tostr(N, cobj));
			}
		}
		if (N->debug) n_warn(N, "n_evalargs", "[%s]", name);
		if ((N->lastop!=OP_PCPAREN)&&(N->lastop!=OP_PCOMMA)) n_error(N, NE_SYNTAX, "n_evalargs", "i'm confused [%d][%s]", N->lastop, name);
		if (N->lastop==OP_PCPAREN) break;
		if (N->lastop==OP_PCOMMA) i++;
	}
	return pobj.d.table;
}

/* store the following function */
int n_storefunction(nesla_state *N)
{
	char tmpnam[MAX_OBJNAMLEN+1];
	char *argptr;
	char *blockstart;
	char *blockend;
	obj_t *cobj;
	unsigned int len;

	sanetest(N, "n_storefunction");
	n_getop(N);
	if (N->lastop!=OP_LABEL) n_error(N, NE_SYNTAX, "n_storefunction", "expected a name for this function");
	n_getlabel(N, tmpnam);
	if (N->debug) n_warn(N, "n_storefunction", "snarfing function [%s]", tmpnam);
	argptr=N->readptr;
	n_getop(N);
	if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, "n_storefunction", "expected a '('");
	n_skipto(N, ')');
	blockstart=N->readptr;
	n_getop(N);
	if (N->lastop!=OP_POBRACE) n_error(N, NE_SYNTAX, "n_storefunction", "expected a '('");
	n_skipto(N, '}');
	blockend=N->readptr;
	len=blockend-blockstart+2;
	if (len<1) return 0;
	cobj=nesla_regnfunc(N, &N->g, tmpnam, NULL);
	cobj->d.function=n_alloc(N, (len+1)*sizeof(char));
	cobj->size=len;
	memcpy(cobj->d.function, argptr, len);
	cobj->d.function[len]='\0';
	return 0;
}
/* store the following value in the supplied table */
int n_storetable(nesla_state *N, obj_t *tobj)
{
	char name[MAX_OBJNAMLEN+1];
	obj_t *cobj;
	unsigned int i;

	sanetest(N, "n_storetable");
	if (N->lastop!=OP_POBRACE) n_error(N, NE_SYNTAX, "n_storetable", "expected an opening brace");
	n_getop(N);
	for (i=0;;) {
		if (*N->readptr=='\0') break;
		name[0]='\0';
		/* first get a name */
		if (N->lastop==OP_POBRACKET) {
			n_getop(N);
			if (N->lastop==OP_LABEL) {
				n_getlabel(N, name);
			} else if (isdigit(*N->readptr)) {
				n_snprintf(N, name, MAX_OBJNAMLEN, "%d", (int)n_getnumber(N));
			}
			n_getop(N);
			if (N->lastop!=OP_PCBRACKET) n_error(N, NE_SYNTAX, "n_storetable", "expected a ']'");
			n_getop(N);
		} else if (N->lastop==OP_LABEL) {
			n_getlabel(N, name);
			if (*N->readptr=='(') { /* this looks like a function */
				name[0]='\0';
				n_ungetop(N);
			} else {
				n_getop(N);
			}
		} else if (N->lastop==OP_POBRACE) {
		} else if (N->lastop==OP_PCBRACE) {
		} else if (N->lastop==OP_PCOMMA) {
		} else if (N->lastop==OP_DATA) {
		} else {
			n_warn(N, "n_storetable", "unhandled data.  probably an error [%d]", N->lastop);
		}
		if (name[0]=='\0') n_snprintf(N, name, MAX_OBJNAMLEN, "%d", i++);
		/* one way or another, we have a name.  now figure out what the val is. */
		if (N->debug) n_warn(N, "n_storetable", "[%s]", name);
		if (N->lastop==OP_MEQ) n_getop(N);
		if ((N->lastop<=OP_MADDEQ)&&(N->lastop>=OP_MSUBSUB)) {
			n_assign2(N, tobj, name, N->lastop);
		} else if (N->lastop==OP_POBRACE) {
			cobj=nesla_getobj(N, tobj, name);
			if (cobj->type!=NT_TABLE) {
				cobj=nesla_regtable(N, tobj, name);
				n_storetable(N, cobj);
			} else {
				n_storetable(N, tobj);
			}
		} else if ((N->lastop==OP_PCOMMA)||(N->lastop==OP_PSEMICOL)||(N->lastop==OP_PCBRACE)) {
		} else {
			if (N->lastop==OP_LABEL) n_ungetop(N);
			n_storevar(N, tobj, name);
		}
		if ((N->lastop==OP_PCOMMA)||(N->lastop==OP_PSEMICOL)) {
			n_getop(N);
			continue;
		} else if (N->lastop==OP_PCBRACE) {
			n_getop(N);
			break;
		} else {
			n_error(N, NE_SYNTAX, "n_storetable", "i'm confused ... [%s][%s]", name, N->lastname);
		}
	}
	return 0;
}

/* store the following value in the supplied var */
int n_storevar(nesla_state *N, obj_t *tobj, char *name)
{
	obj_t *cobj;

	sanetest(N, "n_storevar");
	cobj=n_eval(N, ";");
	if (*N->readptr!='\0') {
		/* the closing brace and comma here are only legal in table allocs ..... */
		/* added cparen for for() loops */
		if ((N->lastop!=OP_PSEMICOL)&&(N->lastop!=OP_PCOMMA)&&(N->lastop!=OP_PCBRACE)&&(N->lastop!=OP_PCPAREN)) n_error(N, NE_SYNTAX, "n_storevar", "expected a ';' got %d %s", N->lastop, N->lastname);
	}
	if (cobj==NULL) return 0;
	if (cobj->type==NT_STRING) {
		nesla_regstr(N, tobj, name, cobj->d.string);
	} else if (cobj->type==NT_NUMBER) {
		nesla_regnum(N, tobj, name, cobj->d.number);
	} else if (cobj->type==NT_NULL) {
		nesla_regnull(N, tobj, name);
	} else if ((cobj->type==NT_NFUNC)||(cobj->type==NT_CFUNC)) {
		n_error(N, NE_SYNTAX, "n_storevar", "storing a value in eval? %s", name);
	} else {
		n_error(N, NE_SYNTAX, "n_storevar", "odd value type? %s", name);
	}
	return 0;
}

int n_assign2(nesla_state *N, obj_t *tobj, char *name, short op)
{
	obj_t *cobj, *oobj;

	sanetest(N, "n_assign2");
	oobj=nesla_getobj(N, tobj, name);
	if (oobj->type!=NT_NUMBER) return -1; /* probably an error...  */
	if (op==OP_MADDADD) {
		n_getop(N); /* expecting a ;    ...   make this conditional? */
		nesla_regnum(N, tobj, name, oobj->d.number+1);
	} else if (op==OP_MSUBSUB) {
		n_getop(N);
		nesla_regnum(N, tobj, name, oobj->d.number-1);
	} else {
		cobj=n_eval(N, ";");
		if (*N->readptr!='\0') {
			/* the closing brace and comma here are only legal in table allocs ..... */
			/* added cparen for for() loops */
			if ((N->lastop!=OP_PSEMICOL)&&(N->lastop!=OP_PCOMMA)&&(N->lastop!=OP_PCBRACE)&&(N->lastop!=OP_PCPAREN)) n_error(N, NE_SYNTAX, "n_assign2", "expected a ';'");
		}
		if (cobj==NULL) return 0; /* should be an error */
		if (cobj->type!=NT_NUMBER) {
			n_error(N, NE_SYNTAX, "n_assign2", "odd value type? %s", name);
		}
		if (op==OP_MADDEQ) {
			nesla_regnum(N, tobj, name, oobj->d.number+cobj->d.number);
		} else if (op==OP_MSUBEQ) {
			nesla_regnum(N, tobj, name, oobj->d.number-cobj->d.number);
		} else if (op==OP_MMULEQ) {
			nesla_regnum(N, tobj, name, oobj->d.number*cobj->d.number);
		} else if (op==OP_MDIVEQ) {
			nesla_regnum(N, tobj, name, oobj->d.number/cobj->d.number);
		} else {
			n_warn(N, "n_assign2", "unhandled op %d", op);
		}
	}
	return 0;
}

int n_assign(nesla_state *N, obj_t *tobj)
{
	char name[MAX_OBJNAMLEN+1];
	obj_t *cobj;
	short reset=0;
	short preop=0;
	short z=0;

	sanetest(N, "n_assign");
	if (n_getop(N)==OP_UNDEFINED) return 0;
	if ((N->lastop==OP_MADDADD)||(N->lastop==OP_MSUBSUB)) {
		preop=N->lastop; n_getop(N);
	}
	if (N->lastop!=OP_LABEL) n_error(N, NE_SYNTAX, "n_assign", "expected a label");
	cobj=nesla_getobj(N, tobj, n_getlabel(N, name));
	for (;;) {
		if (cobj->type!=NT_TABLE) break;
		tobj=cobj;
		if (strchr("[.", *N->readptr)==NULL) { if (!z) name[0]='\0'; break; }
		cobj=n_getindex(N, tobj, name);
		z=1;
	}
	/* if it already exists... */
	if ((cobj->type!=NT_NULL)&&(name[0]=='\0')) {
		reset=1;
		if (cobj->type==NT_TABLE) {
			nesla_freetable(N, cobj);
		}
	}
	if (cobj->type==NT_NUMBER) {
		if (preop==OP_MADDADD) {
			cobj->d.number++;
		} else if (preop==OP_MSUBSUB) {
			cobj->d.number--;
		}
	}
	if (n_getop(N)==OP_UNDEFINED) return 0;
	if (N->lastop==OP_MEQ) {
		if (*N->readptr=='{') {
			n_getop(N);
			if (!reset) tobj=nesla_regtable(N, tobj, name);
			n_storetable(N, tobj);
		} else {
			if (reset) {
				strncpy(name, cobj->name, MAX_OBJNAMLEN);
				tobj=cobj->parent;
			}
			n_storevar(N, tobj, name);
		}
	} else if ((N->lastop<=OP_MADDEQ)&&(N->lastop>=OP_MSUBSUB)) {
		n_assign2(N, tobj, name, N->lastop);
	} else if (N->lastop==OP_PCPAREN) {
	} else {
		n_warn(N, "n_assign", "unhandled op [%s]", N->lastname);
	}
	if (*N->readptr==';') n_getop(N);
	return 0;
}
