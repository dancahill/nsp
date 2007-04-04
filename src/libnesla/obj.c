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
#include <stdlib.h>

static obj_t _null = { NULL, NULL, NULL, NT_NULL, 0, 0, 0, "null", { NULL } };

void *n_alloc(nes_state *N, int size)
{
	char *p=malloc(size);

	if (p==NULL) n_error(N, NE_MEM, "n_alloc", "can't alloc %d bytes", size);
	return p;
}

void n_free(nes_state *N, void **p)
{
	if (*p==NULL) n_error(N, NE_MEM, "n_free", "freeing 0x%08X a second time?", *p);
	free(*p); *p=NULL;
	return;
}

/*
 * the following functions are public API functions
 */

obj_t *nes_getobj(nes_state *N, obj_t *tobj, char *oname)
{
	obj_t *cobj;

	if (tobj==&N->r) return tobj;
	if ((tobj==NULL)||(tobj->type!=NT_TABLE)) return &_null;
	if (N==NULL) {
		for (cobj=tobj->d.table; cobj; cobj=cobj->next) {
			if (cobj->name[0]!=oname[0]) continue;
			if (nc_strcmp(cobj->name, oname)==0) return cobj;
		}
	} else if ((tobj!=&N->l)&&(tobj!=&N->g)) {
		for (cobj=tobj->d.table; cobj; cobj=cobj->next) {
			if (cobj->name[0]!=oname[0]) continue;
			if (nc_strcmp(cobj->name, oname)==0) return cobj;
		}
	} else {
		for (cobj=N->l.d.table; cobj; cobj=cobj->next) {
			if (cobj->name[0]!=oname[0]) continue;
			if (nc_strcmp(cobj->name, oname)==0) return cobj;
		}
		for (cobj=N->g.d.table; cobj; cobj=cobj->next) {
			if (cobj->name[0]!=oname[0]) continue;
			if (nc_strcmp(cobj->name, oname)==0) return cobj;
		}
	}
	return &_null;
}

obj_t *nes_getiobj(nes_state *N, obj_t *tobj, int oindex)
{
	obj_t *cobj;
	int i;

	if ((tobj==NULL)||(tobj->type!=NT_TABLE)) return &_null;
	for (i=0,cobj=tobj->d.table; cobj; i++,cobj=cobj->next) {
		if (cobj->mode&NST_SYSTEM) { i--; continue; }
		if (i!=oindex) continue;
		return cobj;
	}
	return &_null;
}

/* change or create an object and return it */
obj_t *nes_setobj(nes_state *N, obj_t *tobj, char *oname, unsigned short otype, void *_fptr, num_t _num, char *_str, int _slen)
{
	obj_t *oobj, *cobj;
	int cmp=-1;

	if (tobj==&N->r) {
		cobj=tobj;
		n_freestr(cobj);
		cobj->type=otype;
	} else {
		if ((tobj==NULL)||(tobj->type!=NT_TABLE)) return &_null;
		if (oname[0]=='\0') { return &_null; }
		if (tobj->d.table==NULL) {
			cobj=n_alloc(N, sizeof(obj_t));
			nc_memset(cobj, 0, sizeof(obj_t));
			nc_strncpy(cobj->name, oname, MAX_OBJNAMELEN);
			tobj->d.table=cobj;
			cobj->type=otype;
			cobj->parent=tobj;
			if (cobj->type==NT_TABLE) cobj->sort=tobj->sort;
		} else {
			oobj=tobj->d.table;
			for (cobj=oobj; cobj; oobj=cobj,cobj=cobj->next) {
				if (nc_isdigit(cobj->name[0])&&nc_isdigit(oname[0])) {
					cmp=(int)(nes_aton(N, cobj->name)-nes_aton(N, oname));
				} else {
					if (cobj->name[0]!=oname[0]) cmp=cobj->name[0]-oname[0];else
					cmp=nc_strcmp(cobj->name, oname);
				}
				if (cmp==0) break;
				if ((cmp>0)&&(tobj->sort)) break;
			}
			if ((cmp>0)&&(tobj->sort)) {
				oobj=cobj;
				cobj=n_alloc(N, sizeof(obj_t));
				nc_memset(cobj, 0, sizeof(obj_t));
				nc_strncpy(cobj->name, oname, MAX_OBJNAMELEN);
				if (oobj==tobj->d.table) tobj->d.table=cobj;
				cobj->prev=oobj->prev;
				if (cobj->prev) cobj->prev->next=cobj;
				cobj->next=oobj;
				oobj->prev=cobj;
			} else if (cobj==NULL) {
				cobj=n_alloc(N, sizeof(obj_t));
				nc_memset(cobj, 0, sizeof(obj_t));
				nc_strncpy(cobj->name, oname, MAX_OBJNAMELEN);
				if (oobj!=NULL) {
					oobj->next=cobj;
					cobj->prev=oobj;
				}
			} else {
				n_freestr(cobj);
			}
			cobj->type=otype;
			cobj->parent=tobj;
			if (cobj->type==NT_TABLE) cobj->sort=tobj->sort;
		}
	}
	switch (otype) {
	case NT_NUMBER:
		cobj->d.num=_num;
		break;
	case NT_STRING:
	case NT_NFUNC:
		cobj->size=_slen;
		if (_slen) {
			cobj->d.str=n_alloc(N, (cobj->size+1)*sizeof(char));
			nc_strncpy(cobj->d.str, _str, cobj->size+1);
		} else {
			cobj->d.str=NULL;
		}
		break;
	case NT_CFUNC :
		cobj->d.cfunc=_fptr;
		break;
	}
	return cobj;
}

void nes_freetable(nes_state *N, obj_t *tobj)
{
	obj_t *cobj, *oobj;

	if ((tobj==NULL)||(tobj->type!=NT_TABLE)) n_error(N, NE_MEM, "nes_freetable", "tobj is not a table");
	cobj=tobj->d.table;
	while (cobj!=NULL) {
		n_freestr(cobj);
		if (nes_istable(cobj)&&(nc_strcmp(cobj->name, "_globals_")!=0)) {
			if (cobj->mode&NST_LINK) { cobj->mode^=NST_LINK; cobj->d.table=NULL; } else nes_freetable(N, cobj);
		}
		oobj=cobj;
		cobj=cobj->next;
		n_free(N, (void *)&oobj);
	}
	tobj->d.table=NULL;
	return;
}
