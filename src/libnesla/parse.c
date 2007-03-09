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
#include <math.h>

typedef struct {
	char *name;
	short val;
} optab;

static optab oplist_p[] = {
	/* PUNCTUATION */
	{ ";",        OP_PSEMICOL  },
	{ "{",        OP_POBRACE   },
	{ "}",        OP_PCBRACE   },
	{ "(",        OP_POPAREN   },
	{ ")",        OP_PCPAREN   },
	{ "[",        OP_POBRACKET },
	{ "]",        OP_PCBRACKET },
	{ ",",        OP_PCOMMA    },
	{ ".",        OP_PDOT      },
	{ "\'",       OP_PSQUOTE   },
	{ "\"",       OP_PDQUOTE   },
	{ "#",        OP_PHASH     },
	{ NULL,       0            }
};
static optab oplist_m[] = {
	/* MATH */
	{ "=",        OP_MEQ       },
	{ "+",        OP_MADD      },
	{ "++",       OP_MADDADD   },
	{ "+=",       OP_MADDEQ    },
	{ "-",        OP_MSUB      },
	{ "--",       OP_MSUBSUB   },
	{ "-=",       OP_MSUBEQ    },
	{ "*",        OP_MMUL      },
	{ "*=",       OP_MMULEQ    },
	{ "/",        OP_MDIV      },
	{ "/=",       OP_MDIVEQ    },
	{ "==",       OP_MCEQ      },
	{ "!=",       OP_MCNE      },
	{ "<=",       OP_MCLE      },
	{ ">=",       OP_MCGE      },
	{ "<",        OP_MCLT      },
	{ ">",        OP_MCGT      },
	{ "%",        OP_MMOD      },
	{ "&",        OP_MAND      },
	{ "|",        OP_MOR       },
	{ "^",        OP_MXOR      },
	{ "&&",       OP_MLAND     },
	{ "||",       OP_MLOR      },
	{ "!",        OP_MLNOT     },
	{ NULL,       0            }
};
static optab oplist_k[] = {
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

/* Advance readptr to next non-blank */
#define n_skipblank(N) \
	while (*N->readptr) { \
		if (*N->readptr=='#') { \
			while (*N->readptr) { \
				if (*N->readptr=='\r'||*N->readptr=='\n') break; \
				N->readptr++; \
			} \
		} \
		if (!nc_isspace(*N->readptr)) break; \
		N->readptr++; \
	}

/* Advance readptr to next matching quote */
#define n_skipquote(N,c) \
	while (*N->readptr) { \
		if (*N->readptr=='\\') { \
			N->readptr++; \
		} else if (*N->readptr==c) { \
			N->readptr++; \
			break; \
		} \
		N->readptr++; \
	}

/* Advance readptr to next specified char */
void n_skipto(nes_state *N, unsigned short c)
{
	if (N->readptr==NULL) return;
	while (*N->readptr) {
		if (*N->readptr==c) break;
		if (*N->readptr<128) {
			if ((*N->readptr==';')&&(c==OP_PSEMICOL)) break;
			if ((*N->readptr==')')&&(c==OP_PCPAREN)) break;
			if ((*N->readptr=='}')&&(c==OP_PCBRACE)) break;
			switch (*N->readptr++) {
			case '#' : n_skipblank(N); break;
			case '\'': n_skipquote(N, '\''); break;
			case '\"': n_skipquote(N, '\"'); break;
			case '{' : n_skipto(N, OP_PCBRACE); break;
			case '(' : n_skipto(N, OP_PCPAREN); break;
			}
		} else {
			switch (*N->readptr++) {
			case OP_PSQUOTE: n_skipquote(N, OP_PSQUOTE); break;
			case OP_PDQUOTE: n_skipquote(N, OP_PDQUOTE); break;
			case OP_POBRACE: n_skipto(N, OP_PCBRACE); break;
			case OP_POPAREN: n_skipto(N, OP_PCPAREN); break;
			}
		}
	}
	if (*N->readptr==0) n_error(N, NE_SYNTAX, "n_skipto", "\\0 error %c %d", c, c);
	N->readptr++;
	return;
}

/* return the next op/cmp and advance the readptr */
short n_getop(nes_state *N)
{
	short i;

	if (N->readptr==NULL) n_error(N, NE_SYNTAX, "n_getop", "NULL readptr");
	N->lastptr=N->readptr;
	N->lastname[0]=0;
	if (*N->readptr>127) {
		N->lastop=*N->readptr++;
		if (N->debug) n_warn(N, "n_getop", "[%s][%d]", N->lastname, N->lastop);
		return N->lastop;
	}
	n_skipblank(N);
	N->lastop=OP_UNDEFINED;
	if (IS_DATA(*N->readptr)) {
		N->lastop=OP_DATA;
		return N->lastop;
	} else if (IS_LABEL(*N->readptr)) {
		N->lastname[0]=*N->readptr++;
		for (i=1;i<MAX_OBJNAMELEN;i++) {
			if (!IS_LABEL(*N->readptr)&&!nc_isdigit(*N->readptr)) break;
			N->lastname[i]=*N->readptr++;
		}
		N->lastname[i]=0;
		N->lastop=OP_LABEL;
		for (i=0;oplist_k[i].name!=NULL;i++) {
			if (oplist_k[i].name[0]!=N->lastname[0]) continue;
			if (nc_strcmp(oplist_k[i].name, N->lastname)==0) {
				N->lastop=oplist_k[i].val;
				break;
			}
		}
	} else if (IS_PUNCOP(*N->readptr)) {
		N->lastname[0]=*N->readptr++;
		N->lastname[1]=0;
		for (i=0;oplist_p[i].name!=NULL;i++) {
			if (oplist_p[i].name[0]!=N->lastname[0]) continue;
			if (nc_strcmp(oplist_p[i].name, N->lastname)==0) {
				N->lastop=oplist_p[i].val;
				break;
			}
		}
	} else if (IS_MATHOP(*N->readptr)) {
		i=0;
		N->lastname[i++]=*N->readptr++;
		if (IS_MATHOP(*N->readptr)) N->lastname[i++]=*N->readptr++;
		N->lastname[i]=0;
		for (i=0;oplist_m[i].name!=NULL;i++) {
			if (oplist_m[i].name[0]!=N->lastname[0]) continue;
			if (nc_strcmp(oplist_m[i].name, N->lastname)==0) {
				N->lastop=oplist_m[i].val;
				break;
			}
		}
	} else if (*N->readptr!=0) {
		n_warn(N, "n_getop", "[%s][%d]", N->lastname, N->lastop);
	}
	n_skipblank(N);
	if (N->debug) n_warn(N, "n_getop", "[%s][%d]", N->lastname, N->lastop);
	return N->lastop;
}

/* return a number val from readptr */
num_t n_getnumber(nes_state *N)
{
	static char *fn="n_getnumber";
	num_t rval=0;
	num_t dot;

	sanetest();
	N->lastop=OP_UNDEFINED;
	N->lastname[0]=0;
	if (!nc_isdigit(*N->readptr)) n_error(N, NE_SYNTAX, fn, "expected a number");
	while (nc_isdigit(*N->readptr)) {
		rval=10*rval+(*N->readptr-'0');
		N->readptr++;
	}
	if (*N->readptr=='.') {
		dot=1;
		N->readptr++;
		while (nc_isdigit(*N->readptr)) {
			dot*=0.1;
			rval+=(*N->readptr-'0')*dot;
			N->readptr++;
		}
	}
	n_skipblank(N);
	if (N->debug) n_warn(N, fn, "[%f]", rval);
	return rval;
}

/* return the next quoted block */
obj_t *n_getquote(nes_state *N)
{
	static char *fn="n_getquote";
	obj_t *cobj=NULL;
	char q=*N->readptr;
	char *qs, *qe;

	DEBUG_IN();
	sanetest();
	if (N->debug) n_warn(N, fn, "snarfing quote");
	N->lastop=OP_UNDEFINED;
	if ((q=='\'')||(q=='\"')) {
		N->readptr++;
		qs=(char *)N->readptr;
		n_skipquote(N, q);
		qe=(char *)N->readptr;
		cobj=nes_setstr(N, &N->r, "", qs, qe-qs-1);
		n_skipblank(N);
	}
	DEBUG_OUT();
	return cobj;
}

/* return the next table index */
obj_t *n_getindex(nes_state *N, obj_t *tobj, char *lastname)
{
	static char *fn="n_getindex";
	obj_t *cobj;

	DEBUG_IN();
	sanetest();
	nextop();
	if (lastname) lastname[0]=0;
	if (N->lastop==OP_PDOT) {
		nextop();
		if (N->lastop!=OP_LABEL) n_error(N, NE_SYNTAX, fn, "expected a label");
		cobj=nes_getobj(N, tobj, N->lastname);
		if ((cobj->type!=NT_TABLE)&&(lastname)) nc_strncpy(lastname, N->lastname, MAX_OBJNAMELEN);
	} else if (N->lastop==OP_POBRACKET) {
		if (*N->readptr=='@') {
			N->readptr++;
			cobj=n_eval(N);
			if ((cobj==NULL)||(cobj->type!=NT_NUMBER)) n_error(N, NE_SYNTAX, fn, "no index");
			if (N->lastop!=OP_PCBRACKET) n_error(N, NE_SYNTAX, fn, "expected a closing ']'");
			cobj=nes_getiobj(N, tobj, (unsigned int)cobj->d.num);
			cobj=nes_setstr(N, &N->r, "", cobj->name, nc_strlen(cobj->name));
			if (lastname) lastname[0]=0;
		} else {
			cobj=n_eval(N);
			if (N->lastop!=OP_PCBRACKET) n_error(N, NE_SYNTAX, fn, "expected a closing ']'");
			if ((cobj->type!=NT_TABLE)&&(lastname)) {
				nc_strncpy(lastname, nes_tostr(N, cobj), MAX_OBJNAMELEN);
				cobj=nes_getobj(N, tobj, lastname);
			} else {
				cobj=nes_getobj(N, tobj, nes_tostr(N, cobj));
			}
		}
	} else {
		cobj=tobj;
		ungetop();
	}
	if (N->debug) n_warn(N, fn, "[%d '%s' '%s']", cobj->type, cobj->name, lastname?lastname:"");
	N->lastop=OP_UNDEFINED;
	DEBUG_OUT();
	return cobj;
}

void n_prechew(nes_state *N, uchar *rawtext)
{
	/* static char *fn="n_prechew"; */
	uchar *d=rawtext, *o;
	char *p;
	char q;
	unsigned short op;

	o=N->readptr;
	N->readptr=rawtext;
	while (*N->readptr) {
		nextop();
		op=N->lastop;
		if (op==OP_UNDEFINED) break;
		if (OP_ISMATH(op)||OP_ISKEY(op)||OP_ISPUNC(op)) {
			*d++=(uchar)op;
		} else if (op==OP_LABEL) {
			p=N->lastname;
			while (*p) *d++=*p++;
		} else if (nc_isdigit(*N->readptr)) {
			/* while ((*N->readptr==OP_PDOT)||(*N->readptr=='.')||(nc_isdigit(*N->readptr))) *d++=*N->readptr++; */
			while ((*N->readptr=='.')||(nc_isdigit(*N->readptr))) *d++=*N->readptr++;
		} else {
			*d++=(uchar)op;
		}
x:
		if (!*N->readptr) break;
		n_skipblank(N);
		if ((*N->readptr=='\"')||(*N->readptr=='\'')) {
			q=*N->readptr;
			*d++=*N->readptr++;
			while (*N->readptr) {
				if (*N->readptr=='\\') {
					*d++=*N->readptr++;
					if (*N->readptr) *d++=*N->readptr++;
				} else if (*N->readptr==q) {
					*d++=*N->readptr++;
					break;
				} else {
					*d++=*N->readptr++;
				}
			}
			goto x;
		} else if (nc_isdigit(*N->readptr)) {
			while (nc_isdigit(*N->readptr)||*N->readptr=='.') {
				*d++=*N->readptr++;
			}
			goto x;
		/* } else if ((strchr(",;(){}[].@", *N->readptr)!=0)||(nc_isdigit(*N->readptr))) { */
		} else if (*N->readptr=='@') {
			*d++=*N->readptr++;
			goto x;
		}
	}
	*d=0;
	N->readptr=o;
	N->lastop=OP_UNDEFINED;
	return;
}

/*
 above was basic tokenizing stuff.
 below is actual processing.
 */

obj_t *n_evalsub(nes_state *N)
{
	static char *fn="n_evalsub";
	obj_t *cobj, *robj;
	short preop;

	DEBUG_IN();
	nextop();
	if (N->lastop==OP_POPAREN) {
		N->lastop=OP_UNDEFINED;
		cobj=n_eval(N);
		if (N->lastop!=OP_PCPAREN) n_warn(N, fn, "2 expected a closing ')'");
		nextop();
		DEBUG_OUT();
		return cobj;
	} else if (OP_ISEND(N->lastop)) {
		cobj=NULL;
		DEBUG_OUT();
		return cobj;
	}
	if (*N->readptr==0) { DEBUG_OUT(); return NULL; }
	preop=0;
	if (OP_ISMATH(N->lastop)) {
		preop=N->lastop;
		nextop();
	}
	if (N->lastop==OP_DATA) {
		N->lastop=OP_UNDEFINED;
		if (nc_isdigit(*N->readptr)) {
			cobj=&N->r;
			n_freestr(cobj);
			cobj->type=NT_NUMBER;
			if (preop!=OP_MSUB) {
				cobj->d.num=n_getnumber(N);
			} else {
				cobj->d.num=-n_getnumber(N);
			}
		} else if ((*N->readptr=='\"')||(*N->readptr=='\'')) {
			cobj=n_getquote(N);
		} else {
			cobj=NULL;
			n_error(N, NE_SYNTAX, fn, "expected data");
		}
	} else if (N->lastop==OP_LABEL) {
		cobj=nes_getobj(N, &N->l, N->lastname);
		while (*N->readptr==OP_POBRACKET||*N->readptr==OP_PDOT||*N->readptr=='['||*N->readptr=='.') {
			if (cobj->type==NT_TABLE) {
				cobj=n_getindex(N, cobj, NULL);
			} else { /* bad code, but it eats the index */
				cobj=n_getindex(N, NULL, NULL);
			}
		}
		N->lastop=OP_UNDEFINED;
		if (cobj->type==NT_NUMBER) {
			robj=&N->r;
			n_freestr(robj);
			robj->type=NT_NUMBER;
			if (preop==0) {
				robj->d.num=cobj->d.num;
			} else {
				switch (preop) {
				case OP_MSUB    : robj->d.num=-cobj->d.num; break;
				case OP_MADDADD : robj->d.num=++cobj->d.num; break;
				case OP_MSUBSUB : robj->d.num=--cobj->d.num; break;
				}
			}
			if ((*N->readptr==OP_MADDADD)||(N->readptr[0]=='+'&&N->readptr[1]=='+')) {
				nextop();
				cobj->d.num++;
			} else if ((*N->readptr==OP_MSUBSUB)||(N->readptr[0]=='-'&&N->readptr[1]=='-')) {
				nextop();
				cobj->d.num--;
			}
			N->lastop=OP_UNDEFINED;
			cobj=robj;
		} else if (cobj->type==NT_STRING) {
			if (cobj!=&N->r) cobj=nes_setstr(N, &N->r, "", cobj->d.str, cobj->size);
		} else if ((cobj->type==NT_NFUNC)||(cobj->type==NT_CFUNC)) {
			cobj=n_execfunction(N, cobj);
		} else if (cobj->type==NT_TABLE) {
			cobj=nes_setstr(N, &N->r, "", "", 0);
		} else if (cobj->type==NT_BOOLEAN) {
		} else if ((robj=n_macros(N))!=NULL) {
			cobj=robj;
		} else if (cobj->type==NT_NULL) {
		} else {
			n_error(N, NE_SYNTAX, fn, "unhandled type %d [%s]", cobj->type, N->lastname);
		}
	} else {
		cobj=NULL;
		ungetop();
		n_warn(N, fn, "premature end of useful data?");
	}
	DEBUG_OUT();
	return cobj;
}

obj_t *n_eval(nes_state *N)
{
	static char *fn="n_eval";
	obj_t *cobj, *robj;
	char *str, *p1, *p2;
	unsigned int len;
	num_t val;
	short op, type, rtype;

	DEBUG_IN();
	sanetest();
	if (*N->readptr==0) { DEBUG_OUT(); return NULL; }
	cobj=n_evalsub(N);
	if (cobj==NULL) { DEBUG_OUT(); return cobj; }
	if (N->lastop==OP_UNDEFINED) nextop();
	if (OP_ISEND(N->lastop)) { DEBUG_OUT(); return cobj; }
	if (cobj->type==NT_NUMBER) {
		str=NULL;
		len=0;
		val=cobj->d.num;
	} else if (cobj->type==NT_STRING) {
		if (cobj==&N->r) {
			str=cobj->d.str;
			cobj->d.str=NULL;
		} else {
			str=n_alloc(N, (cobj->size+1)*sizeof(char));
			nc_strncpy(str, cobj->d.str, cobj->size+1);
		}
		len=cobj->size;
		val=0;
	} else {
		DEBUG_OUT();
		return cobj;
	}
	type=cobj->type;
	rtype=NT_NUMBER;
	while (*N->readptr) {
		op=N->lastop;
		if (op==OP_UNDEFINED) { nextop(); op=N->lastop; }
		if (OP_ISEND(N->lastop)) break;
		if (!OP_ISMATH(N->lastop)) break;
		cobj=n_evalsub(N);
		if (cobj==NULL) break;
		if (type==NT_NUMBER) {
			if (cobj->type==NT_NUMBER) {
				switch (op) {
					case OP_MCLT   : val=val <  cobj->d.num;break;
					case OP_MCLE   : val=val <= cobj->d.num;break;
					case OP_MCGT   : val=val >  cobj->d.num;break;
					case OP_MCGE   : val=val >= cobj->d.num;break;
					case OP_MCEQ   : val=val == cobj->d.num;break;

					case OP_MADD   :
					case OP_MADDADD:
					case OP_MADDEQ : val=val+cobj->d.num;break;
					case OP_MSUB   :
					case OP_MSUBSUB:
					case OP_MSUBEQ : val=val-cobj->d.num;break;
					case OP_MMUL   :
					case OP_MMULEQ : val=val*cobj->d.num;break;
					case OP_MDIV   :
					case OP_MDIVEQ : val=val/cobj->d.num;break;

					case OP_MMOD   : val=fmod(val, cobj->d.num);break;
					case OP_MAND   : val=(int)val & (int)cobj->d.num;break;
					case OP_MOR    : val=(int)val | (int)cobj->d.num;break;
					case OP_MXOR   : val=(int)val ^ (int)cobj->d.num;break;
					case OP_MLAND  : val=val && cobj->d.num;break;
					case OP_MLOR   : val=val || cobj->d.num;break;
					case OP_MCNE   : val=val != cobj->d.num;break;
				}
			} else if (cobj->type==NT_BOOLEAN) {
				if (nc_strcmp(cobj->name, "true")==0) val=val?1:0; else val=val?0:1;
			} else {
				n_warn(N, fn, "mixed types %d %s", cobj->type, cobj->name, cobj->d.str);
			}
		} else if (type==NT_STRING) {
			if (cobj->type==NT_STRING) {
				p1=str?str:"";
				p2=(char *)cobj->d.str?(char *)cobj->d.str:"";
				switch (op) {
				case OP_MCEQ : val=nc_strcmp(p1, p2)?0:1;    break;
				case OP_MCNE : val=nc_strcmp(p1, p2)?1:0;    break;
				case OP_MCLE : val=nc_strcmp(p1, p2)<=0?1:0; break;
				case OP_MCGE : val=nc_strcmp(p1, p2)>=0?1:0; break;
				case OP_MCLT : val=nc_strcmp(p1, p2)<0?1:0;  break;
				case OP_MCGT : val=nc_strcmp(p1, p2)>0?1:0;  break;
				case OP_MADD :
					rtype=NT_STRING;
					p1=n_alloc(N, (len+cobj->size+1)*sizeof(char));
					if (str) nc_strncpy(p1, str, len+1);
					nc_strncpy(p1+len, p2, cobj->size+1);
					if (str) n_free(N, (void *)&str);
					str=p1;
					len+=cobj->size;
					break;
				default : n_warn(N, fn, "invalid op for string math");
				}
			} else if (cobj->type==NT_BOOLEAN) {
				p1=str?str:"";
				if (nc_strcmp(cobj->name, "true")==0) val=p1[0]?1:0; else val=p1[0]?0:1;
			} else {
				n_warn(N, fn, "mixed types in math %d %s", cobj->type, cobj->name);
			}
		} else {
			n_warn(N, fn, "mixed types in math %d %s", cobj->type, cobj->name);
		}
	}
	if (rtype==NT_STRING) {
		robj=nes_setstr(N, &N->r, "", NULL, 0);
		robj->size=len;
		robj->d.str=str;
	} else {
		if (str) n_free(N, (void *)&str);
		robj=&N->r;
		n_freestr(robj);
		robj->type=NT_NUMBER;
		robj->d.num=val;
	}
	DEBUG_OUT();
	return robj;
}

obj_t *n_evalargs(nes_state *N, char *fname)
{
	static char *fn="n_evalargs";
	char namebuf[MAX_OBJNAMELEN+1];
	obj_t pobj;
	obj_t *cobj;
	obj_t *nobj;
	unsigned short i;

	DEBUG_IN();
	sanetest();
	if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, fn, "missing bracket");
	pobj.type=NT_TABLE;
	pobj.d.table=NULL;
	nes_setstr(N, &pobj, "0", fname, nc_strlen(fname));
	for (i=1;;) {
		N->lastop=OP_UNDEFINED;
		cobj=n_eval(N);
		if ((N->lastop!=OP_PCPAREN)&&(N->lastop!=OP_PCOMMA)) n_error(N, NE_SYNTAX, fn, "expected a closing ')'");
		if (N->lastop==OP_UNDEFINED) nextop();
		if (cobj!=NULL) {
			nes_ntoa(N, namebuf, i, 10, 0);
			if (cobj->type==NT_NUMBER) {
				nes_setnum(N, &pobj, namebuf, cobj->d.num);
			} else if (cobj->type==NT_STRING) {
				nes_setstr(N, &pobj, namebuf, cobj->d.str, cobj->size);
			} else if (cobj->type==NT_BOOLEAN) {
				nobj=nes_setnum(N, &pobj, namebuf, 0);
				nobj->type=NT_BOOLEAN; nobj->d.num=cobj->d.num;
			} else if (cobj->type==NT_NULL) {
				nobj=nes_setnum(N, &pobj, namebuf, 0);
				nobj->type=NT_NULL;
			} else if ((cobj->type==NT_NFUNC)||(cobj->type==NT_CFUNC)) {
				n_error(N, NE_SYNTAX, fn, "storing a value as a func? '%s'", namebuf);
			} else if (cobj->type==NT_TABLE) {
				n_error(N, NE_SYNTAX, fn, "can't handle tables properly '%s'", namebuf);
			} else {
				n_error(N, NE_SYNTAX, fn, "b[%s][%d][%s][%s]", namebuf, cobj->type, cobj->name, nes_tostr(N, cobj));
			}
		}
		if (N->debug) n_warn(N, fn, "[%d]", i);
		if ((N->lastop!=OP_PCPAREN)&&(N->lastop!=OP_PCOMMA)) n_error(N, NE_SYNTAX, fn, "i'm confused [%d][%d]", N->lastop, i);
		if (N->lastop==OP_PCPAREN) break;
		if (N->lastop==OP_PCOMMA) i++;
	}
	DEBUG_OUT();
	return pobj.d.table;
}

/* store a val in the supplied object */
obj_t *n_storeval(nes_state *N, obj_t *cobj)
{
	static char *fn="n_storeval";
	obj_t *nobj;

	switch (N->lastop) {
	case OP_MADDADD:
		if (cobj->type!=NT_NUMBER) n_error(N, NE_SYNTAX, fn, "cobj is not a number");
		nextop();
		cobj->d.num++;
		break;
	case OP_MSUBSUB:
		if (cobj->type!=NT_NUMBER) n_error(N, NE_SYNTAX, fn, "cobj is not a number");
		nextop();
		cobj->d.num--;
		break;
	case OP_MADDEQ :
		if (cobj->type!=NT_NUMBER) n_error(N, NE_SYNTAX, fn, "cobj is not a number");
		nobj=n_eval(N);
		if (!OP_ISEND(N->lastop)) n_error(N, NE_SYNTAX, fn, "expected a ';'");
		if ((nobj==NULL)||(nobj->type!=NT_NUMBER)) n_error(N, NE_SYNTAX, fn, "nobj is not a number");
		cobj->d.num=cobj->d.num+nobj->d.num;
		break;
	case OP_MSUBEQ :
		if (cobj->type!=NT_NUMBER) n_error(N, NE_SYNTAX, fn, "cobj is not a number");
		nobj=n_eval(N);
		if (!OP_ISEND(N->lastop)) n_error(N, NE_SYNTAX, fn, "expected a ';'");
		if ((nobj==NULL)||(nobj->type!=NT_NUMBER)) n_error(N, NE_SYNTAX, fn, "nobj is not a number");
		cobj->d.num=cobj->d.num-nobj->d.num;
		break;
	case OP_MMULEQ :
		if (cobj->type!=NT_NUMBER) n_error(N, NE_SYNTAX, fn, "cobj is not a number");
		nobj=n_eval(N);
		if (!OP_ISEND(N->lastop)) n_error(N, NE_SYNTAX, fn, "expected a ';'");
		if ((nobj==NULL)||(nobj->type!=NT_NUMBER)) n_error(N, NE_SYNTAX, fn, "nobj is not a number");
		cobj->d.num=cobj->d.num*nobj->d.num;
		break;
	case OP_MDIVEQ :
		if (cobj->type!=NT_NUMBER) n_error(N, NE_SYNTAX, fn, "cobj is not a number");
		nobj=n_eval(N);
		if (!OP_ISEND(N->lastop)) n_error(N, NE_SYNTAX, fn, "expected a ';'");
		if ((nobj==NULL)||(nobj->type!=NT_NUMBER)) n_error(N, NE_SYNTAX, fn, "nobj is not a number");
		cobj->d.num=cobj->d.num/nobj->d.num;
		break;
	default: /* OP_MEQ */
		nobj=n_eval(N);
		if (!OP_ISEND(N->lastop)) n_error(N, NE_SYNTAX, fn, "expected a ';'");
		if (nobj==NULL) break;
		n_freestr(cobj);
		if (nobj->type==NT_NUMBER) {
			cobj->type=NT_NUMBER;
			cobj->d.num=nobj->d.num;
		} else if (nobj->type==NT_STRING) {
			cobj->type=NT_STRING;
			cobj->size=nobj->size;
			cobj->d.str=nobj->d.str;
			nobj->size=0;
			nobj->d.str=NULL;
		} else if (nobj->type==NT_NULL) {
			cobj->type=NT_NULL;
		} else if (nobj->type==NT_BOOLEAN) {
			cobj->type=NT_BOOLEAN;
			cobj->d.num=nobj->d.num;
		} else {
			n_error(N, NE_SYNTAX, fn, "unhandled object type");
		}
	}
	return cobj;
}

/* read a function from the readptr */
obj_t *n_readfunction(nes_state *N)
{
	static char *fn="n_readfunction";
	char namebuf[MAX_OBJNAMELEN+1];
	uchar *as, *be;

	DEBUG_IN();
	sanetest();
	nextop();
	if (N->lastop!=OP_LABEL) n_error(N, NE_SYNTAX, fn, "expected a name for this function");
	nc_strncpy(namebuf, N->lastname, MAX_OBJNAMELEN);
	N->lastop=OP_UNDEFINED;
	if (N->debug) n_warn(N, fn, "snarfing function [%s]", namebuf);
	as=N->readptr;
	nextop();
	if (N->lastop!=OP_POPAREN) n_error(N, NE_SYNTAX, fn, "expected a '('");
	n_skipto(N, OP_PCPAREN);
	/* bs=N->readptr; */
	nextop();
	if (N->lastop!=OP_POBRACE) n_error(N, NE_SYNTAX, fn, "expected a '{'");
	n_skipto(N, OP_PCBRACE);
	be=N->readptr;
	nes_setnfunc(N, &N->g, namebuf, (char *)as, be-as);
	/* n_prechew(N, (uchar *)cobj->d.str); */
	DEBUG_OUT();
	return 0;
}

/* read the following list of values into the supplied table */
obj_t *n_readtable(nes_state *N, obj_t *tobj)
{
	static char *fn="n_readtable";
	char namebuf[MAX_OBJNAMELEN+1];
	obj_t *cobj;
	unsigned short i=0;

	DEBUG_IN();
	sanetest();
	if (N->lastop!=OP_POBRACE) n_error(N, NE_SYNTAX, fn, "expected an opening brace");
	nextop();
	while (*N->readptr) {
		namebuf[0]=0;
		/* first get a name */
		if (N->lastop==OP_LABEL) {
			if (*N->readptr==OP_POPAREN||*N->readptr=='(') { /* this looks like a function */
				ungetop();
			} else {
				nc_strncpy(namebuf, N->lastname, MAX_OBJNAMELEN);
				N->lastop=OP_UNDEFINED;
				nextop();
			}
		} else if (N->lastop==OP_POBRACKET) {
			nextop();
			if (N->lastop==OP_LABEL) {
				nc_strncpy(namebuf, N->lastname, MAX_OBJNAMELEN);
				N->lastop=OP_UNDEFINED;
			} else if (nc_isdigit(*N->readptr)) {
				nes_ntoa(N, namebuf, n_getnumber(N), 10, 0);
			}
			nextop();
			if (N->lastop!=OP_PCBRACKET) n_error(N, NE_SYNTAX, fn, "expected a ']'");
			nextop();
		} else if ((N->lastop==OP_POBRACE)||(N->lastop==OP_PCBRACE)) {
		} else if ((N->lastop==OP_PCOMMA)||(N->lastop==OP_DATA)) {
		} else {
			n_warn(N, fn, "unhandled data.  probably an error [%d]", N->lastop);
		}
		if (namebuf[0]==0) {
			nes_ntoa(N, namebuf, i++, 10, 0);
		} else if (N->lastop==OP_MEQ) {
			nextop();
		}
		/* one way or another, we have a name.  now figure out what the val is. */
		if (N->debug) n_warn(N, fn, "[%s]", namebuf);
		if ((N->lastop==OP_PCOMMA)||(N->lastop==OP_PSEMICOL)||(N->lastop==OP_PCBRACE)) {
		} else if (N->lastop==OP_POBRACE) {
			cobj=nes_getobj(N, tobj, namebuf);
			if (cobj->type!=NT_TABLE) {
				cobj=nes_settable(N, tobj, namebuf);
			} else {
				cobj=tobj;
			}
			n_readtable(N, cobj);
		} else {
			if (N->lastop==OP_LABEL) ungetop();
			cobj=nes_getobj(N, tobj, namebuf);
			if (cobj->type==NT_NULL) cobj=nes_setnum(N, tobj, namebuf, 0);
			n_storeval(N, cobj);
		}
		if ((N->lastop==OP_PCOMMA)||(N->lastop==OP_PSEMICOL)) {
			nextop();
			continue;
		} else if (N->lastop==OP_PCBRACE) {
			nextop();
			break;
		} else {
			n_error(N, NE_SYNTAX, fn, " ... [%s][%s]", namebuf, N->lastname);
		}
	}
	DEBUG_OUT();
	return tobj;
}

/* read a var (or table) from the readptr */
obj_t *n_readvar(nes_state *N, obj_t *tobj, obj_t *cobj)
{
	static char *fn="n_readvar";
	char namebuf[MAX_OBJNAMELEN+1];
	short preop=0;

	DEBUG_IN();
	sanetest();
	if (cobj==NULL) {
		nextop();
		if (N->lastop==OP_UNDEFINED) { DEBUG_OUT(); return NULL; }
		if ((N->lastop==OP_MADDADD)||(N->lastop==OP_MSUBSUB)) {
			preop=N->lastop; nextop();
		}
		if (N->lastop!=OP_LABEL) n_error(N, NE_SYNTAX, fn, "expected a label");
		nc_strncpy(namebuf, N->lastname, MAX_OBJNAMELEN);
		N->lastop=OP_UNDEFINED;
		cobj=nes_getobj(N, tobj, namebuf);
	}
	while (cobj->type==NT_TABLE) {
		tobj=cobj;
		if (*N->readptr!=OP_POBRACKET&&*N->readptr!=OP_PDOT&&*N->readptr!='['&&*N->readptr!='.') break;
		cobj=n_getindex(N, tobj, namebuf);
	}
	if (cobj->type==NT_NULL) {
		cobj=nes_setnum(N, tobj, namebuf, 0);
	} else if (cobj->type==NT_TABLE) {
		nes_freetable(N, cobj);
	}
	if (cobj->type==NT_NUMBER) {
		if (preop==OP_MADDADD) {
			cobj->d.num++;
		} else if (preop==OP_MSUBSUB) {
			cobj->d.num--;
		}
	}
	nextop();
	if (N->lastop==OP_UNDEFINED) { DEBUG_OUT(); return cobj; }
	if (*N->readptr==OP_POBRACE||*N->readptr=='{') {
		if (N->lastop==OP_MEQ) {
			nextop();
			if (cobj->type!=NT_TABLE) tobj=nes_settable(N, tobj, cobj->name);
			cobj=n_readtable(N, tobj);
		}
	} else if (OP_ISMATH(N->lastop)) {
		cobj=n_storeval(N, cobj);
	} else if (N->lastop!=OP_PCPAREN) {
		n_warn(N, fn, "unhandled op [%s]", N->lastname);
	}
	if (*N->readptr==OP_PSEMICOL||*N->readptr==';') nextop();
	DEBUG_OUT();
	return cobj;
}
