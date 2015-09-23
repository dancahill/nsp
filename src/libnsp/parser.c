/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2015 Dan Cahill

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
#ifndef _NSPLIB_H
#include "nsp/nsplib.h"
#endif
#include "opcodes.h"
#include <math.h>

static void n_evalobj(nsp_state *N, obj_t *cobj, uchar isnewobject);

/* read a function from N->readptr */
void n_getfunction(nsp_state *N, obj_t *cobj)
{
#define __FN__ __FILE__ ":n_getfunction()"
	uchar *as, *be;

	settrace();
	//	n_warn(N, __FN__, "line %d", __LINE__);
	n_expect(N, __FN__, OP_LABEL);
	as = N->readptr;
	//N->readptr++;
	n_getlabel(N, NULL);



	/* if (N->debug) n_warn(N, __FN__, "snorting lines from %s()", cobj->name); */
	n_expect(N, __FN__, OP_POPAREN);
	//	as = N->readptr;
	N->readptr += readi2((N->readptr + 1)) + 3;
	n_expect(N, __FN__, OP_PCPAREN);
	N->readptr++;
	/* bs=N->readptr; */
	n_expect(N, __FN__, OP_POBRACE);
	N->readptr = n_seekop(N, N->readptr, 0);
	n_skipto(N, __FN__, OP_PCBRACE);
	be = ++N->readptr;
	nsp_setnfunc(N, cobj, NULL, (char *)as, be - as);
	//	n_warn(N, __FN__, "line %d", __LINE__);
	return;
#undef __FN__
}

/* read a label from N->readptr */
char *n_getlabel(nsp_state *N, char *buf)
{
#define __FN__ __FILE__ ":n_getlabel()"
	char *p = (char *)N->readptr + 2;

	settrace();
	N->readptr += 3 + N->readptr[1];
	if (buf) p = nc_strncpy(buf, p, MAX_OBJNAMELEN);
	return p;
#undef __FN__
}

/* read a number val from N->readptr */
num_t n_getnumber(nsp_state *N)
{
#define __FN__ __FILE__ ":n_getnumber()"
	char *s = (char *)N->readptr + 2;
	num_t rval = 0;
	num_t rbase = 10;
	num_t rdot = 0.1;

	settrace();
	N->readptr += 3 + N->readptr[1];
	while (nc_isdigit(*s)) {
		rval = rval*rbase + (*s++ - '0');
	}
	if (*s != '.') return rval;
	s++;
	while (nc_isdigit(*s)) {
		rval = rval + rdot*(*s++ - '0');
		rdot *= 0.1;
	}
	/* if (N->debug) n_warn(N, __FN__, "[%f]", rval); */
	return rval;
#undef __FN__
}

/* read a string val from N->readptr */
void n_getstring(nsp_state *N, obj_t *cobj)
{
#define __FN__ __FILE__ ":n_getstring()"
	int size = readi4((N->readptr + 1));

	settrace();
	nsp_setstr(N, cobj, "", (char *)N->readptr + 5, size);
	N->readptr += 6 + size;
	/* if (N->debug) n_warn(N, __FN__, "%d '%s'", size, nsp_tostr(N, cobj)); */
	return;
#undef __FN__
}









/* return the next table index */
obj_t *n_readindex(nsp_state *N, obj_t *tobj, char *lastname, unsigned short *z)
{
#define __FN__ __FILE__ ":n_readindex()"
	obj_t *cobj;
	char *p;

	DEBUG_IN();
	settrace();
	sanetest();
	if (z) *z = 0;
	if (lastname) lastname[0] = 0;
	if (n_peekop(N) == OP_PDOT) {
		N->readptr++;
		n_expect(N, __FN__, OP_LABEL);
		p = n_getlabel(N, NULL);
	}
	else if (n_peekop(N) == OP_POBRACKET) {
		N->readptr++;
		cobj = nsp_eval(N, (char *)N->readptr);
		n_expect(N, __FN__, OP_PCBRACKET);
		N->readptr++;
		p = nsp_isnull(cobj) ? "" : nsp_tostr(N, cobj);
	}
	else {
		cobj = tobj;
		goto end;
	}
	cobj = nsp_getobj_ex(N, tobj, p, 1, z);
	//	if (lastname&&cobj->val->type!=NT_TABLE) nc_strncpy(lastname, p, MAX_OBJNAMELEN);
	if (lastname) nc_strncpy(lastname, p, MAX_OBJNAMELEN);
end:
	/* if (N->debug) n_warn(N, __FN__, "[%d '%s' '%s']", cobj->val->type, cobj->name, lastname?lastname:""); */
	DEBUG_OUT();
	return cobj;
#undef __FN__
}

/* read the following list of values into the supplied table */
void n_readtable(nsp_state *N, obj_t *tobj)
{
#define __FN__ __FILE__ ":n_readtable()"
	char namebuf[MAX_OBJNAMELEN + 1];
	unsigned int i;
	obj_t *cobj;

	DEBUG_IN();
	settrace();
	sanetest();
	n_expect(N, __FN__, OP_POBRACE);
	N->readptr = n_seekop(N, N->readptr, 0);
	i = 0;
	while (*N->readptr) {
		namebuf[0] = 0;
		/* first get a name */
		if (n_peekop(N) == OP_POBRACE) goto data;
		if (n_peekop(N) == OP_LABEL) {
			/* either a label to use, or a function to run... */
			uchar *p = N->readptr;

			n_getlabel(N, namebuf);
			/* if it's a function, do something useful... */
			if (n_peekop(N) == OP_POPAREN || n_peekop(N) == OP_POBRACKET || n_peekop(N) == OP_PDOT) {
				/* either a function or some kind of reference - NOT an lval */
				namebuf[0] = 0;
				N->readptr = p;
			}
		}
		else if (n_peekop(N) == OP_POBRACKET) {
			/* [ ] */
			N->readptr++;
			if (n_peekop(N) == OP_LABEL) {
				n_getlabel(N, namebuf);
			}
			else if (n_peekop(N) == OP_NUMDATA) {
				n_ntoa(N, namebuf, n_getnumber(N), 10, 0);
			}
			else if (n_peekop(N) == OP_STRDATA) {
				obj_t sobj;

				sobj.val = NULL;
				n_getstring(N, &sobj);
				nc_strncpy(namebuf, sobj.val->d.str, MAX_OBJNAMELEN);
				nsp_unlinkval(N, &sobj);
			}
			n_expect(N, __FN__, OP_PCBRACKET);
			N->readptr++;
		}
		else if (n_peekop(N) == OP_PCBRACE) {
			goto end;
		}
		else if (n_peekop(N) == OP_NUMDATA || n_peekop(N) == OP_STRDATA) {
			goto data;
		}
		else if ((n_peekop(N) == OP_MSUB || n_peekop(N) == OP_MADD) && N->readptr[1] == OP_NUMDATA) {
			goto data;
		}
		else if (n_peekop(N) == OP_KCLASS) {
			N->readptr++;
			if (n_peekop(N) == OP_LABEL) {
				n_getlabel(N, namebuf);
			}
			n_expect(N, __FN__, OP_POBRACE);
			goto data;
		}
		else if (n_peekop(N) == OP_KFUNC) {
			uchar *p;

			N->readptr++;
			n_expect(N, __FN__, OP_LABEL);
			p = N->readptr;
			n_getlabel(N, namebuf);
			if (n_peekop(N) != OP_LABEL) {
				N->readptr = p;
				n_ntoa(N, namebuf, i++, 10, 0);
			}
			cobj = nsp_setnfunc(N, tobj, namebuf, NULL, 0);
			n_getfunction(N, cobj);
		}
		else {
			n_warn(N, __FN__, "unhandled data.  probably an error [%d]", *N->readptr);
		}
		if (n_peekop(N) == OP_MEQ || n_peekop(N) == OP_MCOLON) N->readptr++;
	data:
		if (namebuf[0] == 0) n_ntoa(N, namebuf, i++, 10, 0);
		/* one way or another, we have a name.  now figure out what the val is. */
		/* if (N->debug) n_warn(N, __FN__, "[%s][%s]%d", namebuf, n_getsym(N, *N->readptr), *N->readptr); */
		if (n_peekop(N) == OP_POBRACE) {
			cobj = nsp_getobj(N, tobj, namebuf);
			if (cobj->val->type != NT_TABLE) {
				cobj = nsp_settable(N, tobj, namebuf);
			}
			else {
				cobj = tobj;
			}
			n_readtable(N, cobj);
			n_expect(N, __FN__, OP_PCBRACE);
			N->readptr++;
		}
		else if (*N->readptr != OP_PCOMMA&&*N->readptr != OP_PSEMICOL&&*N->readptr != OP_PCBRACE) {
			cobj = nsp_getobj(N, tobj, namebuf);
			if (nsp_isnull(cobj)) cobj = nsp_setnum(N, tobj, namebuf, 0);
			n_storeval(N, cobj);
		}
		if (n_peekop(N) == OP_PCOMMA || n_peekop(N) == OP_PSEMICOL) {
			N->readptr++;
			continue;
		}
		n_expect(N, __FN__, OP_PCBRACE);
		break;
	}
end:
	DEBUG_OUT();
	return;
#undef __FN__
}

/* read a var (or table) from N->readptr */
obj_t *n_readvar(nsp_state *N, obj_t *tobj, obj_t *cobj)
{
#define __FN__ __FILE__ ":n_readvar()"
	char namebuf[MAX_OBJNAMELEN + 1];
	char *nameptr = namebuf;
	uchar preop = 0;
	uchar op = *N->readptr;
	unsigned short z;

	DEBUG_IN();
	settrace();
	sanetest();
	if (op == OP_KCLASS) {
		N->readptr++;
		//		n_expect(N, __FN__ "1", OP_LABEL);
		//		nsp_settable(N, cobj, n_getlabel(N, NULL));
		//		n_expect(N, __FN__ "1", OP_POBRACE);
		//		n_evalobj(N, cobj, 0);

		n_expect(N, __FN__ "1", OP_LABEL);
		cobj = nsp_settable(N, tobj, n_getlabel(N, NULL));
		n_expect(N, __FN__ "1", OP_POBRACE);
		n_readtable(N, cobj);
		n_expect(N, __FN__, OP_PCBRACE);
		N->readptr++;

		DEBUG_OUT();
		return cobj;
	}
	if (op == OP_KFUNC) {
		N->readptr++;
		n_expect(N, __FN__ "1", OP_LABEL);
		cobj = nsp_setnfunc(N, tobj, n_getlabel(N, NULL), NULL, 0);
		n_getfunction(N, cobj);
		DEBUG_OUT();
		return cobj;
	}
	if (cobj == NULL) {
		if (n_peekop(N) == '\0') { DEBUG_OUT(); return NULL; }
		if (n_peekop(N) == OP_MADDADD || n_peekop(N) == OP_MSUBSUB) preop = *N->readptr++;
		n_expect(N, __FN__ "2", OP_LABEL);
		nameptr = n_getlabel(N, NULL);
		cobj = nsp_getobj(N, tobj, nameptr);
	}
	while (cobj->val->type == NT_TABLE) {
		tobj = cobj;
		if (*N->readptr != OP_POBRACKET&&*N->readptr != OP_PDOT) break;
		cobj = n_readindex(N, tobj, namebuf, &z);
		nameptr = namebuf;
	}
	if (nsp_isnull(cobj)) cobj = nsp_setnum(N, tobj, nameptr, 0);
	if (cobj->val->type == NT_NUMBER) {
		if (preop == OP_MADDADD) {
			cobj->val->d.num++;
		}
		else if (preop == OP_MSUBSUB) {
			cobj->val->d.num--;
		}
	}
	if (OP_ISMATH(*N->readptr)) n_storeval(N, cobj);
	DEBUG_OUT();
	return cobj;
#undef __FN__
}

/* store a val in the supplied object */
void n_storeval(nsp_state *N, obj_t *cobj)
{
#define __FN__ __FILE__ ":n_storeval()"
	obj_t *nobj;
	uchar op;

	settrace();
	switch (n_peekop(N)) {
	case OP_MADDADD:
		N->readptr++;
		if (!nsp_isnum(cobj)) n_error(N, NE_SYNTAX, __FN__, "object is not a number");
		cobj->val->d.num++;
		return;
	case OP_MSUBSUB:
		N->readptr++;
		if (!nsp_isnum(cobj)) n_error(N, NE_SYNTAX, __FN__, "object is not a number");
		cobj->val->d.num--;
		return;
	case OP_MADDEQ:
		if (nsp_isstr(cobj)) {
			N->readptr++;
			nobj = nsp_eval(N, (char *)N->readptr);
			if (nsp_isstr(nobj)) {
				nsp_strcat(N, cobj, nobj->val->d.str, nobj->val->size);
			}
			else {
				nsp_strcat(N, cobj, nsp_tostr(N, nobj), -1);
			}
			return;
		}
	case OP_MMULEQ:
		if (nsp_isstr(cobj)) {
			N->readptr++;
			nobj = nsp_eval(N, (char *)N->readptr);
			if (nobj->val->d.num < 0) nsp_strmul(N, cobj, 0);
			else nsp_strmul(N, cobj, (int)nobj->val->d.num);
			return;
		}
	case OP_MSUBEQ:
	case OP_MDIVEQ:
		op = *N->readptr++;
		if (nsp_typeof(cobj) != NT_NUMBER) n_error(N, NE_SYNTAX, __FN__, "object is not a number");
		nobj = nsp_eval(N, (char *)N->readptr);
		if (nsp_typeof(nobj) != NT_NUMBER) n_error(N, NE_SYNTAX, __FN__, "object is not a number");
		if (!OP_ISEND(*N->readptr)) n_error(N, NE_SYNTAX, __FN__, "expected ';'");
		switch (op) {
		case OP_MADDEQ: cobj->val->d.num += nobj->val->d.num; return;
		case OP_MSUBEQ: cobj->val->d.num -= nobj->val->d.num; return;
		case OP_MMULEQ: cobj->val->d.num *= nobj->val->d.num; return;
		case OP_MDIVEQ: cobj->val->d.num /= nobj->val->d.num; return;
		}
		break;
	case OP_MEQ:
		op = *N->readptr++;
		if (n_peekop(N) == OP_KNEW) {
			N->readptr++;
			n_evalobj(N, cobj, 1);
			return;
		}
		else if (n_peekop(N) == OP_MAND) {
			N->readptr++;
			n_expect(N, __FN__, OP_LABEL);
			nsp_linkval(N, cobj, nsp_getobj(N, NULL, n_getlabel(N, NULL)));
			return;
		}
		else if (n_peekop(N) == OP_MMUL) {
			N->readptr++;
			n_expect(N, __FN__, OP_LABEL);
			nsp_linkval(N, cobj, nsp_eval(N, (char *)N->readptr));
			return;
		}
		break;
	case OP_LABEL:
	case OP_NUMDATA:
	case OP_STRDATA:
	case OP_PSEMICOL:
	case OP_MADD:
	case OP_MSUB:
	case OP_POBRACE:
	case OP_POPAREN:
	case OP_KFUNC:
		break;
	default:
		n_warn(N, __FN__, "confusing op '%s'", n_getsym(N, *N->readptr));
	}
	if (n_peekop(N) == OP_KFUNC) {
		N->readptr++;
		n_getfunction(N, cobj);
		if (n_peekop(N) == OP_POPAREN) {
			n_execfunction(N, cobj, NULL, 0);
			n_moveval(N, cobj, &N->r);
		}
		return;
	}
	nobj = nsp_eval(N, (char *)N->readptr);
	switch (nsp_typeof(nobj)) {
	case NT_TABLE:
	case NT_CDATA:
		nsp_linkval(N, cobj, nobj);
		break;
	default:
		if (cobj->val->refs > 1 && (nsp_typeof(cobj) == NT_TABLE || nsp_typeof(cobj) == NT_CDATA)) {
			nsp_unlinkval(N, cobj);
		}
		n_moveval(N, cobj, nobj);
	}
	return;
#undef __FN__
}

static void n_evalobj(nsp_state *N, obj_t *cobj, uchar isnewobject)
{
#define __FN__ __FILE__ ":n_evalobj()"
	short preop;

	settrace();
	if (n_peekop(N) == OP_POBRACE) {
		nsp_setvaltype(N, cobj, NT_TABLE);
		n_readtable(N, cobj);
		n_expect(N, __FN__, OP_PCBRACE);
		N->readptr++;
		return;
	}
	else if (n_peekop(N) == OP_POPAREN) {
		N->readptr = n_seekop(N, N->readptr, 0);
		nsp_linkval(N, cobj, nsp_eval(N, (char *)N->readptr));
		/* n_moveval(N, cobj, nsp_eval(N, (char *)N->readptr)); */
		n_expect(N, __FN__, OP_PCPAREN);
		N->readptr++;
		return;
	}
	preop = OP_ISMATH(*N->readptr) ? *N->readptr++ : 0;
	if (n_peekop(N) == OP_STRDATA) {
		n_getstring(N, cobj);
		return;
	}
	else if (n_peekop(N) == OP_ESTRDATA) {
		//		n_getexstring(N, cobj);
		n_getstring(N, cobj);
		return;
	}
	else if (n_peekop(N) == OP_NUMDATA) {
		nsp_setnum(N, cobj, NULL, (preop != OP_MSUB) ? +n_getnumber(N) : -n_getnumber(N));
		return;
	}
	else if (n_peekop(N) == OP_LABEL) {
		uchar *p = N->readptr;
		uchar *e;
		char *l = n_getlabel(N, NULL);
		obj_t *nobj = nsp_getobj(N, NULL, l);
		obj_t *pobj = NULL;
		char namebuf[MAX_OBJNAMELEN + 1];

	x:
		namebuf[0] = '\0';
		while (n_peekop(N) == OP_POBRACKET || n_peekop(N) == OP_PDOT) {
			pobj = nobj;
			nobj = n_readindex(N, (nobj->val->type == NT_TABLE) ? nobj : NULL, namebuf, NULL);
		}
		switch (*N->readptr) {
		case OP_POPAREN:
			if (nobj->val->type == NT_NFUNC || nobj->val->type == NT_CFUNC) {
				//n_expect(N, __FN__, OP_POPAREN);
				nobj = n_execfunction(N, nobj, isnewobject ? NULL : pobj, isnewobject);
				if (n_peekop(N) == OP_PDOT) goto x;
				isnewobject = 0;
				break;
			}
			else if (nobj->val->type == NT_TABLE && isnewobject) {
				nsp_setvaltype(N, cobj, NT_TABLE);
				nsp_zlink(N, cobj, nobj);
				obj_t *xobj = nsp_getobj(N, cobj, nobj->name);
				if (!nsp_isnull(xobj)) {
					nobj = n_execfunction(N, xobj, cobj, isnewobject);
				}
				else {
					xobj = nsp_getobj(N, cobj, "_constructor");
					if (!nsp_isnull(xobj)) {
						nobj = n_execfunction(N, xobj, cobj, isnewobject);
					}
					else if (n_peekop(N) == OP_POPAREN) {
						N->readptr += readi2((N->readptr + 1)) + 3;
						n_expect(N, __FN__, OP_PCPAREN);
						N->readptr++;
					}
				}
				isnewobject = 0;
				return;
			}
			else {
				char errbuf[80];

				///* base class method hack */
				//obj_t tobj;
				//nc_memset((void *)&tobj, 0, sizeof(obj_t));
				//nsp_setcfunc(N, &tobj, "base_method", (NSP_CFUNC)nl_base_method);
				//nc_strncpy(tobj.name, namebuf, MAX_OBJNAMELEN);
				//n_expect(N, __FN__, OP_POPAREN);
				//nobj = n_execfunction(N, &tobj, pobj, 0);
				//nsp_unlinkval(N, &tobj);
				nobj = n_execbasemethod(N, namebuf, pobj, 0);
				if (n_peekop(N) == OP_PDOT) goto x;
				break;
				//				}
				e = N->readptr - 1;
				nc_memset(errbuf, 0, sizeof(errbuf));
				n_decompile(N, p, e, errbuf, sizeof(errbuf) - 1);
				n_error(N, NE_SYNTAX, __FN__, "'%s' is not a function", errbuf);
			}
		case OP_MADDEQ:
		case OP_MSUBEQ:
		case OP_MMULEQ:
		case OP_MDIVEQ: if (nsp_isnull(nobj)) n_error(N, NE_SYNTAX, __FN__, "null object");
		case OP_MEQ: N->readptr = p; nobj = n_readvar(N, &N->l, NULL); break;
		}
		switch (nsp_typeof(nobj)) {
		case NT_NULL:
			nsp_setvaltype(N, cobj, NT_NULL); break;
		case NT_NUMBER:
			nsp_setvaltype(N, cobj, NT_NUMBER);
			switch (preop) {
			case OP_MSUB: cobj->val->d.num = -nobj->val->d.num; break;
			case OP_MADDADD: cobj->val->d.num = ++nobj->val->d.num; break;
			case OP_MSUBSUB: cobj->val->d.num = --nobj->val->d.num; break;
			default: cobj->val->d.num = nobj->val->d.num;
			}
			switch (*N->readptr) {
			case OP_MADDADD: N->readptr++; nobj->val->d.num++; break;
			case OP_MSUBSUB: N->readptr++; nobj->val->d.num--; break;
			}
			break;
		case NT_STRING: if (nobj != cobj) n_copyval(N, cobj, nobj); break;
		case NT_TABLE: {
			if (isnewobject) {
				nsp_setvaltype(N, cobj, NT_TABLE);
				nsp_zlink(N, cobj, nobj);
				obj_t *xobj = nsp_getobj(N, cobj, nobj->name);
				if (!nsp_isnull(xobj)) {
					n_execfunction(N, xobj, cobj, isnewobject);
				}
				else {
					xobj = nsp_getobj(N, cobj, "_constructor");
					if (!nsp_isnull(xobj)) {
						n_execfunction(N, xobj, cobj, isnewobject);
					}
					else if (n_peekop(N) == OP_POPAREN) {
						N->readptr += readi2((N->readptr + 1)) + 3;
						n_expect(N, __FN__, OP_PCPAREN);
						N->readptr++;
					}
				}
			}
			else {
				nsp_linkval(N, cobj, nobj); break;
			}
		}
		case NT_CDATA: nsp_linkval(N, cobj, nobj); break;
		default: n_copyval(N, cobj, nobj);
		}
		if (preop == OP_MLNOT) nsp_setbool(N, cobj, "", !nsp_tobool(N, cobj));
	}
	return;
#undef __FN__
}

static void n_evalsub(nsp_state *N, uchar op1, obj_t *obj1);

static void n_evalop(nsp_state *N, obj_t *cobj, uchar op, obj_t *nobj)
{
#define __FN__ __FILE__ ":n_evalop()"
	unsigned short n, t;

	settrace();
	if (op == OP_MCEEQ) {
		if (nsp_typeof(cobj) != nsp_typeof(nobj)) {
			nsp_setvaltype(N, cobj, NT_BOOLEAN);
			cobj->val->d.num = 0;
			return;
		}
	}
	if (nsp_isnull(nobj)) {
		n = nsp_isnull(cobj);
		t = nsp_istrue(cobj);
		if (nsp_istable(cobj)) nsp_unlinkval(N, cobj);
		nsp_setvaltype(N, cobj, NT_BOOLEAN);
		switch (op) {
		case OP_MCEEQ:
		case OP_MCEQ: cobj->val->d.num = n ? 1 : 0; break;
		case OP_MCNE: cobj->val->d.num = n ? 0 : 1; break;
		case OP_MLAND: cobj->val->d.num = 0;     break;
		case OP_MLOR: cobj->val->d.num = n ? 0 : t; break;
		default: nsp_unlinkval(N, cobj); goto unhandled;
		}
		return;
	}
	switch (nsp_typeof(cobj)) {
	case NT_NULL:
		n = nsp_isnull(nobj);
		// t=nsp_istrue(nobj);
		nsp_setvaltype(N, cobj, NT_BOOLEAN);
		switch (op) {
		case OP_MCEEQ:
		case OP_MCEQ: cobj->val->d.num = n ? 1 : 0; break;
		case OP_MCNE: cobj->val->d.num = n ? 0 : 1; break;
		case OP_MLAND: cobj->val->d.num = 0; break;
		case OP_MLOR: cobj->val->d.num = nsp_tonum(N, nobj) ? 1 : 0; break;
		default: goto unhandled;
		}
		return;
	case NT_BOOLEAN:
		switch (nsp_typeof(nobj)) {
		case NT_NUMBER:
		case NT_BOOLEAN:
			switch (op) {
			case OP_MCEEQ:
			case OP_MCEQ: cobj->val->d.num = (cobj->val->d.num ? 1 : 0) == (nobj->val->d.num ? 1 : 0) ? 1 : 0; break;
			case OP_MCNE: cobj->val->d.num = (cobj->val->d.num ? 1 : 0) != (nobj->val->d.num ? 1 : 0) ? 1 : 0; break;
			case OP_MLAND: cobj->val->d.num = cobj->val->d.num && nobj->val->d.num; break;
			case OP_MLOR: cobj->val->d.num = cobj->val->d.num || nobj->val->d.num; break;
			default: goto unhandled;
			}
			return;
		case NT_STRING:
			switch (op) {
			case OP_MCEEQ:
			case OP_MCEQ: cobj->val->d.num = (cobj->val->d.num && (nobj->val->d.str&&nobj->val->d.str[0])) ? 1 : 0; break;
			default: goto unhandled;
			}
			return;
		case NT_TABLE:
			switch (op) {
			case OP_MCEEQ:
			case OP_MCEQ: cobj->val->d.num = (cobj->val->d.num ? 1 : 0) == (nobj->val->d.table.f ? 1 : 0) ? 1 : 0; break;
			case OP_MCNE: cobj->val->d.num = (cobj->val->d.num ? 1 : 0) != (nobj->val->d.table.f ? 1 : 0) ? 1 : 0; break;
			default: goto unhandled;
			}
			return;
		default: goto unhandled;
		}
		return;
	case NT_NUMBER:
		switch (nsp_typeof(nobj)) {
		case NT_STRING:
			nsp_setnum(N, nobj, "", n_aton(N, nobj->val->d.str));
		case NT_NUMBER:
			switch (op) {
			case OP_MCEEQ:
			case OP_MCEQ: cobj->val->type = NT_BOOLEAN; cobj->val->d.num = cobj->val->d.num == nobj->val->d.num; break;
			case OP_MCNE: cobj->val->type = NT_BOOLEAN; cobj->val->d.num = cobj->val->d.num != nobj->val->d.num; break;
			case OP_MCLT: cobj->val->type = NT_BOOLEAN; cobj->val->d.num = cobj->val->d.num < nobj->val->d.num; break;
			case OP_MCLE: cobj->val->type = NT_BOOLEAN; cobj->val->d.num = cobj->val->d.num <= nobj->val->d.num; break;
			case OP_MCGT: cobj->val->type = NT_BOOLEAN; cobj->val->d.num = cobj->val->d.num > nobj->val->d.num; break;
			case OP_MCGE: cobj->val->type = NT_BOOLEAN; cobj->val->d.num = cobj->val->d.num >= nobj->val->d.num; break;
			case OP_MLAND: cobj->val->type = NT_BOOLEAN; cobj->val->d.num = cobj->val->d.num && nobj->val->d.num; break;
			case OP_MLOR: cobj->val->type = NT_BOOLEAN; cobj->val->d.num = cobj->val->d.num || nobj->val->d.num; break;
			case OP_MADD:
			case OP_MADDADD:
			case OP_MADDEQ: cobj->val->d.num = cobj->val->d.num + nobj->val->d.num; break;
			case OP_MSUB:
			case OP_MSUBSUB:
			case OP_MSUBEQ: cobj->val->d.num = cobj->val->d.num - nobj->val->d.num; break;
			case OP_MMUL:
			case OP_MMULEQ: cobj->val->d.num = cobj->val->d.num * nobj->val->d.num; break;
			case OP_MDIV:
			case OP_MDIVEQ:
				if (nobj->val->d.num == 0) { cobj->val->type = NT_NULL; break; }
				cobj->val->d.num = cobj->val->d.num / nobj->val->d.num; break;
			case OP_MMOD:
				if (nobj->val->d.num == 0) { cobj->val->type = NT_NULL; break; }
				cobj->val->d.num = (int)cobj->val->d.num % (int)nobj->val->d.num; break;
			case OP_MAND: cobj->val->d.num = (int)cobj->val->d.num & (int)nobj->val->d.num; break;
			case OP_MOR: cobj->val->d.num = (int)cobj->val->d.num | (int)nobj->val->d.num; break;
			case OP_MXOR: cobj->val->d.num = (int)cobj->val->d.num ^ (int)nobj->val->d.num; break;
			case OP_MSHIFTL: cobj->val->d.num = (int)cobj->val->d.num << (int)nobj->val->d.num; break;
			case OP_MSHIFTR: cobj->val->d.num = (int)cobj->val->d.num >> (int)nobj->val->d.num; break;
			default: goto unhandled;
			}
			return;
		case NT_BOOLEAN:
			/* nsp_setvaltype(N, cobj, NT_BOOLEAN); */
			cobj->val->type = NT_BOOLEAN;
			switch (op) {
			case OP_MCEEQ:
			case OP_MCEQ: cobj->val->d.num = (cobj->val->d.num ? 1 : 0) == (nobj->val->d.num ? 1 : 0) ? 1 : 0; break;
			default: goto unhandled;
			}
			return;
		default: goto unhandled;
		}
		return;
	case NT_STRING:
		switch (nsp_typeof(nobj)) {
		case NT_NUMBER:
			if (op == OP_MMUL) {
				nsp_strmul(N, cobj, nobj->val->d.num < 0 ? 0 : (int)nobj->val->d.num);
				return;
			}
			else {
				nsp_setstr(N, nobj, "", n_ntoa(N, N->numbuf, nobj->val->d.num, 10, 6), -1);
			}
		case NT_STRING:
			if (op == OP_MADD) {
				nsp_strcat(N, cobj, nobj->val->d.str, nobj->val->size);
			}
			else {
				int cmp = nc_strcmp(cobj->val->d.str, nobj->val->d.str);

				nsp_setvaltype(N, cobj, NT_BOOLEAN);
				switch (op) {
				case OP_MCEEQ:
				case OP_MCEQ: cobj->val->d.num = cmp ? 0 : 1;    break;
				case OP_MCNE: cobj->val->d.num = cmp ? 1 : 0;    break;
				case OP_MCLE: cobj->val->d.num = cmp <= 0 ? 1 : 0; break;
				case OP_MCGE: cobj->val->d.num = cmp >= 0 ? 1 : 0; break;
				case OP_MCLT: cobj->val->d.num = cmp < 0 ? 1 : 0;  break;
				case OP_MCGT: cobj->val->d.num = cmp > 0 ? 1 : 0;  break;
				default: goto unhandled;
				}
			}
			return;
		case NT_BOOLEAN:
			switch (op) {
			case OP_MCEEQ:
			case OP_MCEQ:
				nsp_setnum(N, cobj, "", ((cobj->val->d.str&&cobj->val->d.str[0]) ? 1 : 0) == (nobj->val->d.num ? 1 : 0) ? 1 : 0);
				cobj->val->type = NT_BOOLEAN;
				break;
			default: goto unhandled;
			}
			return;
		default: goto unhandled;
		}
		return;
	case NT_TABLE:
		switch (nsp_typeof(nobj)) {
		case NT_TABLE: {
			int cmp = (cobj->val->d.table.f == nobj->val->d.table.f);

			nsp_setvaltype(N, cobj, NT_BOOLEAN);
			switch (op) {
			case OP_MCEEQ:
			case OP_MCEQ: cobj->val->d.num = cmp ? 1 : 0; break;
			case OP_MCNE: cobj->val->d.num = cmp ? 0 : 1; break;
			default: goto unhandled;
			}
			return;
		}
		default: goto unhandled;
		}
		return;
	default: goto unhandled;
	}
	return;
unhandled:
	//n_warn(N, __FN__, "unhandled operator '%s' %s '%s'", n_gettype(N, (short)nsp_typeof(cobj)), n_getsym(N, op), n_gettype(N, (short)nsp_typeof(nobj)));
	n_error(N, NE_SYNTAX, __FN__, "unhandled operator '%s' %s '%s'", n_gettype(N, (short)nsp_typeof(cobj)), n_getsym(N, op), n_gettype(N, (short)nsp_typeof(nobj)));
	return;
#undef __FN__
}

static void n_evalsub(nsp_state *N, uchar op1, obj_t *obj1)
{
#define __FN__ __FILE__ ":n_evalsub()"
	val_t val2 = { NT_NULL, NST_STACKVAL, 1, 0, NULL, { 0 } };
	obj_t obj2;
	uchar op2;

	settrace();
	obj2.val = &val2;
	while (OP_ISMATH(n_peekop(N))) {
		op2 = *N->readptr++;


		/* short circuiting done easy (and wrong)
		 * this assumes an oparen began the block and that a cparen is the proper termination mark.
		 * this is likely true, but must be checked...
		 */
		if (op2 == OP_MLAND && !nsp_tobool(N, obj1)) {
			//n_warn(N, __FN__, "(op2==OP_MLAND && !nsp_tobool(N, obj1))");
			n_skipto(N, __FN__, OP_PCPAREN);
			continue;
		}


		if (op2 == OP_MQUESTION) {
			if (nsp_tobool(N, obj1)) {
				n_evalobj(N, &obj2, 0);
				n_moveval(N, obj1, &obj2);
				n_expect(N, __FN__, OP_MCOLON);
				N->readptr++;
				while (*N->readptr) {
					if (n_peekop(N) == OP_PCPAREN || n_peekop(N) == OP_PCOMMA || n_peekop(N) == OP_PSEMICOL) break;
					N->readptr = n_seekop(N, N->readptr, 1);
				}
			}
			else {
				n_skipto(N, __FN__, OP_MCOLON);
				N->readptr++;
				n_evalobj(N, &obj2, 0);
				n_evalsub(N, 0, &obj2);
				n_moveval(N, obj1, &obj2);
			}
		}
		else {
			n_evalobj(N, &obj2, 0);
			if (OP_ISMATH(*N->readptr) && oplist[(uchar)*N->readptr].priority > oplist[(uchar)op2].priority) {
				n_evalsub(N, op2, &obj2);
			}
			n_evalop(N, obj1, op2, &obj2);
		}
		if (obj2.val != &val2) {
			nsp_unlinkval(N, &obj2);
			obj2.val = &val2;
		}
		n_freeval(N, &obj2);
	}
	return;
#undef __FN__
}

/*
 * the following functions are public API functions
 */

obj_t *nsp_eval(nsp_state *N, const char *string)
{
#define __FN__ __FILE__ ":nsp_eval()"
	val_t val1 = { NT_NULL, NST_STACKVAL, 1, 0, NULL, { 0 } };
	obj_t obj1;

	DEBUG_IN();
	settrace();
	sanetest();
	obj1.val = &val1;
	if (N->savjmp != NULL) {
		N->readptr = (uchar *)string;
		n_evalobj(N, &obj1, 0);
		n_evalsub(N, 0, &obj1);
	}
	else {
		uchar *p;
		int psize;

		if (string == NULL || string[0] == 0) {
			nsp_unlinkval(N, &N->r);
			DEBUG_OUT();
			return &N->r;
		}
		n_decompose(N, NULL, (uchar *)string, &p, &psize);
		if (p) N->blockptr = p;
		N->blockend = N->blockptr + readi4((N->blockptr + 8));
		N->readptr = N->blockptr + readi4((N->blockptr + 12));
		N->savjmp = (jmp_buf *)n_alloc(N, sizeof(jmp_buf), 0);
		if (setjmp(*N->savjmp) == 0) {
			n_evalobj(N, &obj1, 0);
			n_evalsub(N, 0, &obj1);
		}
		n_free(N, (void *)&N->savjmp, sizeof(jmp_buf));
		if (p) n_free(N, (void *)&p, psize);
		N->blockend = NULL;
		N->readptr = NULL;
	}
	// switch (nsp_typeof((&obj1))) {
	switch (obj1.val == NULL ? NT_NULL : obj1.val->type) {
	case NT_TABLE:
	case NT_CDATA:
		if (obj1.val->refs > 1) {
			nsp_linkval(N, &N->r, &obj1);
		}
		else {
			n_moveval(N, &N->r, &obj1);
		}
		break;
	default:
		n_moveval(N, &N->r, &obj1);
	}
	if (obj1.val != &val1) {
		nsp_unlinkval(N, &obj1);
		obj1.val = &val1;
	}
	DEBUG_OUT();
	return &N->r;
#undef __FN__
}

#include <stdarg.h>
int nc_vsnprintf(nsp_state *N, char *dest, int max, const char *format, va_list ap);
#define MAXBUF 8192

obj_t *nsp_evalf(nsp_state *N, const char *fmt, ...)
{
#define __FN__ __FILE__ ":nsp_evalf()"
	va_list ap;
	uchar *buf;
	uchar *oldbptr = N->blockptr;
	uchar *oldbend = N->blockend;
	uchar *oldrptr = N->readptr;
	obj_t *cobj;

	settrace();
	if ((buf = (uchar *)n_alloc(N, MAXBUF, 1)) == NULL) return NULL;
	va_start(ap, fmt);
	nc_vsnprintf(N, (char *)buf, MAXBUF, fmt, ap);
	va_end(ap);
	N->blockptr = buf;
	cobj = nsp_eval(N, (char *)N->blockptr);
	n_free(N, (void *)&N->blockptr, MAXBUF);
	N->blockptr = oldbptr;
	N->blockend = oldbend;
	N->readptr = oldrptr;
	return cobj;
#undef __FN__
}
