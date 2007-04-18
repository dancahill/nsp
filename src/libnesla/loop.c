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
		t=cobj->val->d.num?1:0;
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
	}
	N->readptr=be;
	DEBUG_OUT();
	return;
}

void n_do(nes_state *N)
{
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
