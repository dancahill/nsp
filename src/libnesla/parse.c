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
#include <math.h>

#define IS_MATHOP(c) (c=='='||c=='+'||c=='-'||c=='*'||c=='/'||c=='%'||c=='&'||c=='|'||c=='^'||c=='!'||c=='<'||c=='>')
#define IS_PUNCOP(c) (c=='('||c==')'||c==','||c=='{'||c=='}'||c==';'||c=='.'||c=='['||c==']')
#define IS_DATA(c)   (c=='\''||c=='\"'||nc_isdigit(c))
#define IS_LABEL(c)  (c=='_'||c=='$'||nc_isalpha(c))
/*
char *opmap="---------------------------------md-lmmdppmmpmpmdddddddddd-pmmm--llllllllllllllllllllllllllp-pml-llllllllllllllllllllllllllpmp--";
#define IS_MATHOP(c) (c<128?opmap[c]=='m':0)
#define IS_PUNCOP(c) (c<128?opmap[c]=='p':0)
#define IS_DATA(c)   (c<128?opmap[c]=='d':0)
#define IS_LABEL(c)  (c<128?opmap[c]=='l':0)
*/

typedef struct {
	char *name;
	short val;
} optab;

static char *typenames[] = { "null", "bool", "number", "string", "nfunc", "cfunc", "table", "cdata" };

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
	{ "do",       OP_KDO       },
	{ "while",    OP_KWHILE    },
	{ "exit",     OP_KEXIT     },
	{ NULL,       0            }
};

/* Advance readptr to next non-blank */
#define n_skipblank(N) \
	while (*N->readptr) { \
		if (*N->readptr=='#'||(N->readptr[0]=='/'&&N->readptr[1]=='/')) { \
			while (*N->readptr) { \
				if (*N->readptr=='\r'||*N->readptr=='\n') break; \
				N->readptr++; \
			} \
		} else if (N->readptr[0]=='/'&&N->readptr[1]=='*') { \
			while (*N->readptr) { \
				if (N->readptr[0]=='*'&&N->readptr[1]=='/') { N->readptr+=2; break; } \
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
		if (!*N->readptr) n_error(N, NE_SYNTAX, "n_skipquote", "unterminated string"); \
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
			if (*N->readptr=='/'&&(N->readptr[1]=='/'||N->readptr[1]=='*')) n_skipblank(N);
			if (*N->readptr=='#') n_skipblank(N);
			switch (*N->readptr++) {
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
		if (IS_MATHOP(*N->readptr)) {
			if (N->lastname[0]=='=') {
				if (N->readptr[0]=='=') N->lastname[i++]=*N->readptr++;
			} else {
				N->lastname[i++]=*N->readptr++;
			}
		}
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
	obj_t *cobj;
	char q=*N->readptr;
	char *qs, *qe;

	DEBUG_IN();
	sanetest();
	if (N->debug) n_warn(N, fn, "snarfing quote");
	N->lastop=OP_UNDEFINED;
	if ((q!='\'')&&(q!='\"')) {
		DEBUG_OUT();
		return NULL;
	}
	N->readptr++;
	qs=(char *)N->readptr;
	n_skipquote(N, q);
	qe=(char *)N->readptr;
	cobj=nes_setstr(N, &N->r, "", NULL, qe-qs-1);
	cobj->val->size=n_unescape(N, qs, cobj->val->d.str, qe-qs-1);
	n_skipblank(N);
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
		if ((cobj->val->type!=NT_TABLE)&&(lastname)) nc_strncpy(lastname, N->lastname, MAX_OBJNAMELEN);
	} else if (N->lastop==OP_POBRACKET) {
		if (*N->readptr=='@') {
			N->readptr++;
			cobj=nes_eval(N, (char *)N->readptr);
			if ((cobj==NULL)||(cobj->val->type!=NT_NUMBER)) n_error(N, NE_SYNTAX, fn, "no index");
			if (N->lastop!=OP_PCBRACKET) n_error(N, NE_SYNTAX, fn, "expected a closing ']'");
			cobj=nes_getiobj(N, tobj, (unsigned int)cobj->val->d.num);
			cobj=nes_setstr(N, &N->r, "", cobj->name, nc_strlen(cobj->name));
			if (lastname) lastname[0]=0;
		} else {
			cobj=nes_eval(N, (char *)N->readptr);
			if (N->lastop!=OP_PCBRACKET) n_error(N, NE_SYNTAX, fn, "expected a closing ']'");
			if (nes_isnull(cobj)) {
				cobj=nes_getobj(N, tobj, "");
			} else if ((cobj->val->type!=NT_TABLE)&&(lastname)) {
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
	if (N->debug) n_warn(N, fn, "[%d '%s' '%s']", cobj->val->type, cobj->name, lastname?lastname:"");
	N->lastop=OP_UNDEFINED;
	DEBUG_OUT();
	return cobj;
}

/* chew the raw script text and regurgitate a tokenized version */
void n_prechew(nes_state *N, uchar *rawtext)
{
	static char *fn="n_prechew";
	uchar *d=rawtext, *o;
	char *p;
	char q;
	unsigned short op;

	if (N->debug) n_warn(N, fn, "prechewing script text");
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
		} else if (*N->readptr=='@') {
			*d++=*N->readptr++;
			goto x;
		}
	}
	*d=0;
	N->readptr=o;
	N->lastop=OP_UNDEFINED;
	if (N->debug) n_warn(N, fn, "finished prechewing script text");
	return;
}

/*
 above was basic tokenizing stuff.
 below is actual processing.
 */

/* i'm probably wrong about this... */
static char oporder[] = {
	0, /* OP_MEQ     */
	3, /* OP_MADD    */
	3, /* OP_MSUB    */
	4, /* OP_MMUL    */
	4, /* OP_MDIV    */
	0, /* OP_MADDEQ  */
	0, /* OP_MSUBEQ  */
	0, /* OP_MMULEQ  */
	0, /* OP_MDIVEQ  */
	3, /* OP_MADDADD should already be done, but we need the placeholder */
	3, /* OP_MSUBSUB should already be done, but we need the placeholder */
	4, /* OP_MMOD    */
	5, /* OP_MAND    */
	5, /* OP_MOR     */
	5, /* OP_MXOR    */
	1, /* OP_MLAND   */
	1, /* OP_MLOR    */
	1, /* OP_MLNOT   */
	2, /* OP_MCEQ    */
	2, /* OP_MCNE    */
	2, /* OP_MCLE    */
	2, /* OP_MCGE    */
	2, /* OP_MCLT    */
	2, /* OP_MCGT    */
};

/* i think i hate this function */
#define MAXOPS 32
obj_t *nes_eval(nes_state *N, char *string)
{
	static char *fn="nes_eval";
	obj_t *cobj=NULL, *nobj, *nnobj;
	obj_t listobj;
	int cmp;
	short preop;
	short op;
	unsigned short i, j, k, l, r;
	unsigned short ops[MAXOPS];

	DEBUG_IN();
	N->readptr=(uchar *)string;
	sanetest();
	listobj.val=n_newval(N, NT_TABLE);
	listobj.val->attr=0;
	/* make a list of tokens */
	for (i=0;;) {
		nextop();
		if (OP_ISEND(N->lastop)) break;
		if (*N->readptr==0) break;
		if (N->lastop==OP_POPAREN) {
			N->lastop=OP_UNDEFINED;
			nobj=nes_eval(N, (char *)N->readptr);
			if (N->lastop!=OP_PCPAREN) n_error(N, NE_SYNTAX, fn, "expected a closing ')'");
			if (nobj==NULL) break;
			nextop();
			if (cobj==NULL) {
				cobj=listobj.val->d.table=n_newiobj(N, i);
			} else {
				cobj->next=n_newiobj(N, i);
				cobj->next->prev=cobj;
				cobj=cobj->next;
			}
			nes_linkval(N, cobj, nobj);
		} else {
			if (OP_ISMATH(N->lastop)) {
				preop=N->lastop;
				nextop();
			} else {
				preop=0;
			}
			if (cobj==NULL) {
				cobj=listobj.val->d.table=n_newiobj(N, i);
			} else {
				cobj->next=n_newiobj(N, i);
				cobj->next->prev=cobj;
				cobj=cobj->next;
			}
			if (N->lastop==OP_DATA) {
				if (nc_isdigit(*N->readptr)) {
					cobj->val=n_newval(N, NT_NUMBER);
					cobj->val->d.num=(preop!=OP_MSUB)?+n_getnumber(N):-n_getnumber(N);
				} else if ((*N->readptr=='\"')||(*N->readptr=='\'')) {
					nes_linkval(N, cobj, n_getquote(N));
				} else {
					n_error(N, NE_SYNTAX, fn, "expected data");
				}
			} else if (N->lastop==OP_LABEL) {
				nobj=nes_getobj(N, &N->l, N->lastname);
				while (*N->readptr==OP_POBRACKET||*N->readptr==OP_PDOT||*N->readptr=='['||*N->readptr=='.') {
					if (nobj->val->type==NT_TABLE) {
						nobj=n_getindex(N, nobj, NULL);
					} else { /* eat the index even if it's null */
						nobj=n_getindex(N, NULL, NULL);
					}
				}
				if (nobj->val->type==NT_NFUNC||nobj->val->type==NT_CFUNC) {
					if (*N->readptr==OP_POPAREN||*N->readptr=='(') {
						nobj=n_execfunction(N, nobj);
					}
				}
				if (nes_isnull(nobj)) {
					nes_unlinkval(N, cobj);
					cobj->val=n_newval(N, NT_NULL);
				} else if (nobj->val->type==NT_NUMBER) {
					cobj->val=n_newval(N, NT_NUMBER);
					switch (preop) {
					case OP_MSUB    : cobj->val->d.num=-nobj->val->d.num; break;
					case OP_MADDADD : cobj->val->d.num=++nobj->val->d.num; break;
					case OP_MSUBSUB : cobj->val->d.num=--nobj->val->d.num; break;
					default         : cobj->val->d.num=nobj->val->d.num; break;
					}
					if ((*N->readptr==OP_MADDADD)||(N->readptr[0]=='+'&&N->readptr[1]=='+')) {
						nextop();
						nobj->val->d.num++;
					} else if ((*N->readptr==OP_MSUBSUB)||(N->readptr[0]=='-'&&N->readptr[1]=='-')) {
						nextop();
						nobj->val->d.num--;
					}
				} else if (nobj->val->type==NT_STRING) {
					if (nobj==&N->r) {
						nes_linkval(N, cobj, &N->r);
						nes_unlinkval(N, &N->r);
					} else {
						n_copyval(N, cobj, nobj);
					}
				} else if (nobj->val->type==NT_TABLE) {
					nes_linkval(N, cobj, nobj);
				} else if (nobj->val->type==NT_CDATA) {
					nes_linkval(N, cobj, nobj);
				} else {
					n_copyval(N, cobj, nobj);
				}
			}
			N->lastop=OP_UNDEFINED;
		}
		if (N->debug) n_warn(N, fn, "[%d]", i);
		if (*N->readptr==0) break;
		if (N->lastop==OP_UNDEFINED) nextop();
		if (!OP_ISMATH(N->lastop)) break;
		ops[i++]=N->lastop;
		if (i>MAXOPS-1) n_error(N, NE_SYNTAX, fn, "too many math ops (%d)", MAXOPS);
	}
	ops[i]=0;
	if (listobj.val->d.table==NULL) {
		nes_unlinkval(N, &listobj);
		DEBUG_OUT();
		return NULL;
	}
	/* now _use_ the list of tokens */
	do {
		r=0;
		nobj=cobj=listobj.val->d.table;
		for (j=0;j<i;j++) {
			if (nobj) nobj=nobj->next;
			for (k=j;k<i;k++) {
				if (!nobj||ops[k]) break;
				nobj=nobj->next;
			}
			/* if there's no next op, there's no next object */ 
			if (!ops[k]) break;
			op=0;
			for (;;) {
				if (!nobj) break;
				nnobj=nobj->next;
				for (l=k+1;l<i;l++) {
					if (!nnobj||ops[l]) break;
					nnobj=nnobj->next;
				}
				if ((nnobj==NULL)||(ops[l]==0)||(oporder[OP_MEQ-ops[k]]>=oporder[OP_MEQ-ops[l]])) {
					op=ops[k];
					ops[k]=0;
					break;
				} else {
					/* forget the first one, promote the last two, and keep looking */
					j=k;
					k=l;
					cobj=nobj;
					nobj=nnobj;
					r=1;
				}
			}
			/* n_warn(N, fn, "[%s %s] k=%d", cobj->name, nobj->name, k); */
			/* n_warn(N, fn, "math (%s)%s (%s)%s", typenames[cobj->val->type], nes_tostr(N, cobj), typenames[nobj->val->type], nes_tostr(N, nobj)); */
			if (nes_isnull(cobj)) {
				if (op==OP_MCEQ) {
					nes_unlinkval(N, cobj);
					cobj->val=n_newval(N, NT_NUMBER);
					cobj->val->d.num=nes_isnull(nobj)?1:0;
					continue;
				} else if (op==OP_MCNE) {
					nes_unlinkval(N, cobj);
					cobj->val=n_newval(N, NT_NUMBER);
					cobj->val->d.num=nes_isnull(nobj)?0:1;
					continue;
				} else {
					n_error(N, NE_SYNTAX, fn, "unhandled null object");
				}
			} else if (cobj->val->type==NT_BOOLEAN) {
				if (nes_isnull(nobj)) {
					if (op==OP_MCEQ) {
						nes_unlinkval(N, cobj);
						cobj->val=n_newval(N, NT_NUMBER);
						cobj->val->d.num=0;
						continue;
					} else if (op==OP_MCNE) {
						nes_unlinkval(N, cobj);
						cobj->val=n_newval(N, NT_NUMBER);
						cobj->val->d.num=1;
						continue;
					}
				} else if (nobj->val->type==NT_NUMBER) {
					if (op==OP_MCEQ) {
						if (nobj->val->d.num) {
							cobj->val->d.num=cobj->val->d.num?1:0;
						} else {
							cobj->val->d.num=cobj->val->d.num?0:1;
						}
						continue;
					}
				} else if (nobj->val->type==NT_STRING) {
					if (op==OP_MCEQ) {
						cmp=(cobj->val->d.str&&cobj->val->d.str[0]);
						n_freeval(N, cobj);
						if (nobj->val->d.num) {
							cobj->val->d.num=cmp?1:0;
						} else {
							cobj->val->d.num=cmp?0:1;
						}
						continue;
					}
				}
			} else if (cobj->val->type==NT_NUMBER) {
				if (nes_isnull(nobj)) {
					if (op==OP_MCEQ) {
						nes_unlinkval(N, cobj);
						cobj->val=n_newval(N, NT_NUMBER);
						cobj->val->d.num=0;
						continue;
					} else if (op==OP_MCNE) {
						nes_unlinkval(N, cobj);
						cobj->val=n_newval(N, NT_NUMBER);
						cobj->val->d.num=1;
						continue;
					} else {
						n_error(N, NE_SYNTAX, fn, "unhandled null object");
					}
				}
				if (nobj->val->type==NT_NUMBER) {
					switch (op) {
					case OP_MCLT   : cobj->val->d.num=cobj->val->d.num <  nobj->val->d.num;break;
					case OP_MCLE   : cobj->val->d.num=cobj->val->d.num <= nobj->val->d.num;break;
					case OP_MCGT   : cobj->val->d.num=cobj->val->d.num >  nobj->val->d.num;break;
					case OP_MCGE   : cobj->val->d.num=cobj->val->d.num >= nobj->val->d.num;break;
					case OP_MCEQ   : cobj->val->d.num=cobj->val->d.num == nobj->val->d.num;break;

					case OP_MADD   :
					case OP_MADDADD:
					case OP_MADDEQ : cobj->val->d.num=cobj->val->d.num + nobj->val->d.num;break;
					case OP_MSUB   :
					case OP_MSUBSUB:
					case OP_MSUBEQ : cobj->val->d.num=cobj->val->d.num - nobj->val->d.num;break;
					case OP_MMUL   :
					case OP_MMULEQ : cobj->val->d.num=cobj->val->d.num * nobj->val->d.num;break;
					case OP_MDIV   :
					case OP_MDIVEQ : if (nobj->val->d.num) { cobj->val->d.num=cobj->val->d.num / nobj->val->d.num; break; }
						n_warn(N, fn, "division by zero %f div %f", cobj->val->d.num, nobj->val->d.num);
						cobj->val->type=NT_NULL;
						break;

					case OP_MMOD   : if ((int)nobj->val->d.num) { cobj->val->d.num=(int)cobj->val->d.num % (int)nobj->val->d.num; break; }
						n_warn(N, fn, "division by zero %f mod %f", cobj->val->d.num, nobj->val->d.num);
						cobj->val->type=NT_NULL;
						break;
					case OP_MAND   : cobj->val->d.num=(int)cobj->val->d.num & (int)nobj->val->d.num;break;
					case OP_MOR    : cobj->val->d.num=(int)cobj->val->d.num | (int)nobj->val->d.num;break;
					case OP_MXOR   : cobj->val->d.num=(int)cobj->val->d.num ^ (int)nobj->val->d.num;break;
					case OP_MLAND  : cobj->val->d.num=cobj->val->d.num && nobj->val->d.num;break;
					case OP_MLOR   : cobj->val->d.num=cobj->val->d.num || nobj->val->d.num;break;
					case OP_MCNE   : cobj->val->d.num=cobj->val->d.num != nobj->val->d.num;break;
					}
					continue;
				} else if (nobj->val->type==NT_BOOLEAN) {
					if (op==OP_MCEQ) {
						if (nobj->val->d.num) {
							cobj->val->d.num=cobj->val->d.num?1:0;
						} else {
							cobj->val->d.num=cobj->val->d.num?0:1;
						}
						continue;
					}
				}
			} else if (cobj->val->type==NT_STRING) {
				if (nes_isnull(nobj)) {
					if (op==OP_MCEQ) {
						nes_unlinkval(N, cobj);
						cobj->val=n_newval(N, NT_NUMBER);
						cobj->val->d.num=0;
						continue;
					} else if (op==OP_MCNE) {
						nes_unlinkval(N, cobj);
						cobj->val=n_newval(N, NT_NUMBER);
						cobj->val->d.num=1;
						continue;
					} else {
						n_error(N, NE_SYNTAX, fn, "unhandled null object");
					}
				}
				if (nobj->val->type==NT_NUMBER) {
					nes_ntoa(N, N->numbuf, nobj->val->d.num, 10, 6);
					nobj=nes_setstr(N, &listobj, nobj->name, N->numbuf, nc_strlen(N->numbuf));
				}
				if (nobj->val->type==NT_STRING) {
					if (op==OP_MADD) {
						n_joinstr(N, cobj, nobj->val->d.str, nobj->val->size);
					} else {
						cmp=nc_strcmp(cobj->val->d.str?cobj->val->d.str:"", nobj->val->d.str?nobj->val->d.str:"");
						n_freeval(N, cobj);
						cobj->val->type=NT_NUMBER;
						switch (op) {
						case OP_MCEQ : cobj->val->d.num=cmp?0:1;    break;
						case OP_MCNE : cobj->val->d.num=cmp?1:0;    break;
						case OP_MCLE : cobj->val->d.num=cmp<=0?1:0; break;
						case OP_MCGE : cobj->val->d.num=cmp>=0?1:0; break;
						case OP_MCLT : cobj->val->d.num=cmp<0?1:0;  break;
						case OP_MCGT : cobj->val->d.num=cmp>0?1:0;  break;
						default : n_warn(N, fn, "invalid op for string math");
						}
					}
					continue;
				} else if (nobj->val->type==NT_BOOLEAN) {
					if (op==OP_MCEQ) {
						cmp=(cobj->val->d.str&&cobj->val->d.str[0]);
						n_freeval(N, cobj);
						cobj->val->type=NT_NUMBER;
						if (nobj->val->d.num) {
							cobj->val->d.num=cmp?1:0;
						} else {
							cobj->val->d.num=cmp?0:1;
						}
						continue;
					}
				}
			}
			n_warn(N, fn, "invalid mix of types in math (%s) (%s)", typenames[nes_isnull(cobj)?NT_NULL:cobj->val->type], typenames[nes_isnull(nobj)?NT_NULL:nobj->val->type]);
		}
	} while (r);
	nes_linkval(N, &N->r, listobj.val->d.table);
	nes_unlinkval(N, &listobj);
	DEBUG_OUT();
	return &N->r;
}

/* store a val in the supplied object */
obj_t *n_storeval(nes_state *N, obj_t *cobj)
{
	static char *fn="n_storeval";
	obj_t *nobj;

	switch (N->lastop) {
	case OP_MADDADD:
		if (!nes_isnum(cobj)) n_error(N, NE_SYNTAX, fn, "object is not a number");
		nextop();
		cobj->val->d.num++;
		return cobj;
	case OP_MSUBSUB:
		if (!nes_isnum(cobj)) n_error(N, NE_SYNTAX, fn, "object is not a number");
		nextop();
		cobj->val->d.num--;
		return cobj;
	case OP_MADDEQ :
		if (!nes_isnum(cobj)) n_error(N, NE_SYNTAX, fn, "object is not a number");
		nobj=nes_eval(N, (char *)N->readptr);
		if (!nes_isnum(nobj)) n_error(N, NE_SYNTAX, fn, "object is not a number");
		if (!OP_ISEND(N->lastop)) n_error(N, NE_SYNTAX, fn, "expected a ';'");
		cobj->val->d.num=cobj->val->d.num+nobj->val->d.num;
		return cobj;
	case OP_MSUBEQ :
		if (!nes_isnum(cobj)) n_error(N, NE_SYNTAX, fn, "object is not a number");
		nobj=nes_eval(N, (char *)N->readptr);
		if (!nes_isnum(nobj)) n_error(N, NE_SYNTAX, fn, "object is not a number");
		if (!OP_ISEND(N->lastop)) n_error(N, NE_SYNTAX, fn, "expected a ';'");
		cobj->val->d.num=cobj->val->d.num-nobj->val->d.num;
		return cobj;
	case OP_MMULEQ :
		if (!nes_isnum(cobj)) n_error(N, NE_SYNTAX, fn, "object is not a number");
		nobj=nes_eval(N, (char *)N->readptr);
		if (!nes_isnum(nobj)) n_error(N, NE_SYNTAX, fn, "object is not a number");
		if (!OP_ISEND(N->lastop)) n_error(N, NE_SYNTAX, fn, "expected a ';'");
		cobj->val->d.num=cobj->val->d.num*nobj->val->d.num;
		return cobj;
	case OP_MDIVEQ :
		if (!nes_isnum(cobj)) n_error(N, NE_SYNTAX, fn, "object is not a number");
		nobj=nes_eval(N, (char *)N->readptr);
		if (!nes_isnum(nobj)) n_error(N, NE_SYNTAX, fn, "object is not a number");
		if (!OP_ISEND(N->lastop)) n_error(N, NE_SYNTAX, fn, "expected a ';'");
		cobj->val->d.num=cobj->val->d.num/nobj->val->d.num;
		return cobj;
	default:
		nobj=nes_eval(N, (char *)N->readptr);
		if (!OP_ISEND(N->lastop)) n_error(N, NE_SYNTAX, fn, "expected a ';'");
		if (nes_isnull(nobj)) return cobj;
		if ((nobj->val->type==NT_TABLE)||(nobj->val->type==NT_STRING)) {
			if (cobj!=nobj) {
				nes_linkval(N, cobj, nobj);
				nes_unlinkval(N, nobj);
			}
		} else if (nobj->val->type==NT_CDATA) {
			nes_linkval(N, cobj, nobj);
		} else {
			n_copyval(N, cobj, nobj);
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
	obj_t *cobj;

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
	n_skipblank(N);
	/* bs=N->readptr; */
	nextop();
	if (N->lastop!=OP_POBRACE) n_error(N, NE_SYNTAX, fn, "expected a '{'");
	n_skipto(N, OP_PCBRACE);
	be=N->readptr;
	cobj=nes_setnfunc(N, &N->g, namebuf, (char *)as, be-as);
	/* n_prechew(N, (uchar *)cobj->val->d.str); */
	DEBUG_OUT();
	return cobj;
}

/* read the following list of values into the supplied table */
obj_t *nes_readtable(nes_state *N, obj_t *tobj)
{
	static char *fn="n_readtable";
	char namebuf[MAX_OBJNAMELEN+1];
	obj_t *cobj;
	unsigned short i=0;

	DEBUG_IN();
	sanetest();
	if (N->lastop==OP_UNDEFINED||N->lastop==0) nextop();
	if (N->lastop!=OP_POBRACE) n_error(N, NE_SYNTAX, fn, "expected an opening brace");
	nextop();
	while (*N->readptr) {
		namebuf[0]=0;
		/* first get a name */
		if (N->lastop==OP_LABEL) {
			if (*N->readptr==OP_POPAREN||*N->readptr=='(') {
				/* either a function or an expression - NOT an lval */
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
		} else if (((N->lastop==OP_MSUB)||(N->lastop==OP_MADD))&&nc_isdigit(N->readptr[0])) {
			ungetop();
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
			if (cobj->val->type!=NT_TABLE) {
				cobj=nes_settable(N, tobj, namebuf);
			} else {
				cobj=tobj;
			}
			nes_readtable(N, cobj);
		} else {
			cobj=nes_getobj(N, tobj, namebuf);
			if (nes_isnull(cobj)) cobj=nes_setnum(N, tobj, namebuf, 0);
			if (((N->lastop==OP_MSUB)||(N->lastop==OP_MADD))&&nc_isdigit(N->readptr[0])) {
				ungetop();
			} else if (N->lastop==OP_LABEL) {
				ungetop();
			}
			n_storeval(N, cobj);
		}
		if ((N->lastop==OP_PCOMMA)||(N->lastop==OP_PSEMICOL)) {
			nextop();
			continue;
		} else if (N->lastop==OP_PCBRACE) {
			nextop();
			break;
		} else {
			n_error(N, NE_SYNTAX, fn, " error reading table var [%s][%s]", namebuf, N->lastname);
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
	while (cobj->val->type==NT_TABLE) {
		tobj=cobj;
		if (*N->readptr!=OP_POBRACKET&&*N->readptr!=OP_PDOT&&*N->readptr!='['&&*N->readptr!='.') break;
		cobj=n_getindex(N, tobj, namebuf);
	}
	if (nes_isnull(cobj)) {
		cobj=nes_setnum(N, tobj, namebuf, 0);
	} else if (cobj->val->type==NT_TABLE) {
		nes_freetable(N, cobj);
	}
	if (cobj->val->type==NT_NUMBER) {
		if (preop==OP_MADDADD) {
			cobj->val->d.num++;
		} else if (preop==OP_MSUBSUB) {
			cobj->val->d.num--;
		}
	}
	nextop();
	if (N->lastop==OP_UNDEFINED) { DEBUG_OUT(); return cobj; }
	if (*N->readptr==OP_POBRACE||*N->readptr=='{') {
		if (N->lastop==OP_MEQ) {
			nextop();
			if (cobj->val->type!=NT_TABLE) tobj=nes_settable(N, tobj, cobj->name);
			cobj=nes_readtable(N, tobj);
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
