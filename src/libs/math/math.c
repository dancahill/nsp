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
#ifdef __TURBOC__
#include "nesla/libnes~1.h"
#else
#include "nesla/libneslamath.h"
#endif
#include <math.h>
int neslamath_math2(nes_state *N)
{
	obj_t *cobj0=nes_getiobj(N, &N->l, 0);
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	num_t n;

	if (cobj1->val->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_tostr(N, cobj0), "expected a number");
	if (nc_strcmp(cobj0->val->d.str, "ceil")==0) {
		n=ceil(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "floor")==0) {
		n=floor(cobj1->val->d.num);
	/* add trig functions */
	} else if (nc_strcmp(cobj0->val->d.str, "acos")==0) {
		n=acos(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "asin")==0) {
		n=asin(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "atan")==0) {
		n=atan(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "atan2")==0) {
		if (cobj2->val->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_tostr(N, cobj0), "expected a number for arg2");
		n=atan2(cobj1->val->d.num, cobj2->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "cos")==0) {
		n=cos(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "sin")==0) {
		n=sin(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "tan")==0) {
		n=tan(cobj1->val->d.num);
	/* add exp and log functions */
	} else if (nc_strcmp(cobj0->val->d.str, "exp")==0) {
		n=exp(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "log")==0) {
		if (cobj1->val->type<=0) n_error(N, NE_SYNTAX, nes_tostr(N, cobj0), "arg must not be zero");
		n=log(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "log10")==0) {
		if (cobj1->val->type<=0) n_error(N, NE_SYNTAX, nes_tostr(N, cobj0), "arg must not be zero");
		n=log10(cobj1->val->d.num);
	/* add hyperbolic functions */
	} else if (nc_strcmp(cobj0->val->d.str, "cosh")==0) {
		n=cosh(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "sinh")==0) {
		n=sinh(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "tanh")==0) {
		n=tanh(cobj1->val->d.num);
	/* add other functions */
	} else if (nc_strcmp(cobj0->val->d.str, "sqrt")==0) {
		n=sqrt(cobj1->val->d.num);
	} else {
		n=0;
	}
	nes_setnum(N, &N->r, "", n);
	return 0;
}
int neslamath_register_all(nes_state *N)
{
	obj_t *tobj;

	tobj=nes_settable(N, &N->g, "math");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj, "acos",  (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "asin",  (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "atan",  (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "atan2", (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "cos",   (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "sin",   (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "tan",   (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "exp",   (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "log",   (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "log10", (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "cosh",  (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "sinh",  (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "tanh",  (NES_CFUNC)neslamath_math2);
	nes_setcfunc(N, tobj, "sqrt",  (NES_CFUNC)neslamath_math2);
	return 0;
}
