/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2009 Dan Cahill

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
#include "nesla/libext.h"
#include <stdlib.h>

static void sort_byname(nes_state *N, obj_t *tobj, int recurse, int order)
{
	obj_t *cobj, *nobj, *robj=tobj->val->d.table.f;
	short change, swap;

reloop:
	if (nes_isnull(robj)) goto end;
	cobj=robj;
	change=0;
	for (;;) {
		if (nes_isnull(cobj)) break;
		if ((cobj->val->type==NT_TABLE)&&(recurse)&&(cobj->val!=N->g.val)) {
			sort_byname(N, cobj, recurse, order);
		}
		if ((nobj=cobj->next)==NULL) break;
		swap=0;
		if (nc_isdigit(cobj->name[0])&&nc_isdigit(nobj->name[0])) {
			if (order<0) {
				if (atoi(cobj->name)<atoi(nobj->name)) swap=1;
			} else {
				if (atoi(cobj->name)>atoi(nobj->name)) swap=1;
			}
		} else {
			if (order<0) {
				if (nc_strcmp(cobj->name, nobj->name)<0) swap=1;
			} else {
				if (nc_strcmp(cobj->name, nobj->name)>0) swap=1;
			}
		}
		if (swap) {
			if ((cobj->prev==NULL)&&(cobj==robj)) {
				robj=nobj;
			}
			nobj=cobj;
			cobj=cobj->next;
			cobj->prev=nobj->prev;
			nobj->next=cobj->next;
			cobj->next=nobj;
			nobj->prev=cobj;
			if (cobj->prev!=NULL) cobj->prev->next=cobj;
			if (nobj->next!=NULL) nobj->next->prev=nobj;
			change=1;
		}
		cobj=cobj->next;
	}
	if (change) goto reloop;
end:
	tobj->val->d.table.f=robj;
	return;
}

static void sort_bykey(nes_state *N, obj_t *tobj, char *key, int order)
{
	obj_t *cobj, *nobj, *robj=tobj->val->d.table.f;
	obj_t *sub1, *sub2;
	short change, swap;

reloop:
	if (nes_isnull(robj)) goto end;
	cobj=robj;
	change=0;
	for (;;) {
		if (nes_isnull(cobj)) break;
		if ((nobj=cobj->next)==NULL) break;
		if (cobj->val->type!=NT_TABLE) break;
		if (nobj->val->type!=NT_TABLE) break;
		sub1=nes_getobj(N, cobj, key);
		sub2=nes_getobj(N, nobj, key);
		swap=0;
		if ((sub1->val->type==NT_NUMBER)&&(sub2->val->type==NT_NUMBER)) {
			if (order<0) {
				if (sub1->val->d.num<sub2->val->d.num) swap=1;
			} else {
				if (sub1->val->d.num>sub2->val->d.num) swap=1;
			}
		} else {
			if (order<0) {
				if (nc_strcmp(nes_tostr(N, sub1), nes_tostr(N, sub2))<0) swap=1;
			} else {
				if (nc_strcmp(nes_tostr(N, sub1), nes_tostr(N, sub2))>0) swap=1;
			}
		}
		if (swap) {
			if ((cobj->prev==NULL)&&(cobj==robj)) {
				robj=nobj;
			}
			nobj=cobj;
			cobj=cobj->next;
			cobj->prev=nobj->prev;
			nobj->next=cobj->next;
			cobj->next=nobj;
			nobj->prev=cobj;
			if (cobj->prev!=NULL) cobj->prev->next=cobj;
			if (nobj->next!=NULL) nobj->next->prev=nobj;
			change=1;
		}
		cobj=cobj->next;
	}
	if (change) goto reloop;
end:
	tobj->val->d.table.f=robj;
	return;
}

NES_FUNCTION(neslaext_sort_byname)
{
#define __FUNCTION__ __FILE__ ":neslaext_sort_byname()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1");
	obj_t *cobj2=nes_getobj(N, &N->l, "2");
	obj_t *cobj3=nes_getobj(N, &N->l, "3");
	int order=1;
	int rec=0;

	if (cobj1->val->type!=NT_TABLE) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a table for arg1");
	if (cobj2->val->type==NT_NUMBER) rec=(int)nes_tonum(N, cobj2);
	if (cobj3->val->type==NT_NUMBER) order=(int)nes_tonum(N, cobj3);
	sort_byname(N, cobj1, rec, order);
	nes_setnum(N, &N->r, "", 0);
	return 0;
#undef __FUNCTION__
}

NES_FUNCTION(neslaext_sort_bykey)
{
#define __FUNCTION__ __FILE__ ":neslaext_sort_bykey()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1");
	obj_t *cobj2=nes_getobj(N, &N->l, "2");
	obj_t *cobj3=nes_getobj(N, &N->l, "3");
	int order=1;

	if (cobj1->val->type!=NT_TABLE) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a table for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");
	if (cobj3->val->type==NT_NUMBER) order=(int)nes_tonum(N, cobj3);
	sort_bykey(N, cobj1, cobj2->val->d.str, order);
	nes_setnum(N, &N->r, "", 0);
	return 0;
#undef __FUNCTION__
}
