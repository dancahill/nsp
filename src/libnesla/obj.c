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

static val_t __null = { NT_NULL, 0, 0, { 0 } };
static obj_t _null = { NULL, NULL, NULL, &__null, 0, "null" };

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

void n_copyval(nes_state *N, obj_t *cobj1, obj_t *cobj2)
{
	if (cobj1==cobj2) return;
	if (cobj1->val) nes_unlinkval(N, cobj1);
	cobj1->val=n_newval(N, cobj2->val->type);
	switch (cobj2->val->type) {
	case NT_NUMBER:
		cobj1->val->d.num=cobj2->val->d.num;
		break;
	case NT_STRING:
	case NT_NFUNC:
		cobj1->val->size=cobj2->val->size;
		if (cobj2->val->size) {
			cobj1->val->d.str=n_alloc(N, (cobj2->val->size+1)*sizeof(char));
			nc_memcpy(cobj1->val->d.str, cobj2->val->d.str, cobj2->val->size);
			cobj1->val->d.str[cobj1->val->size]=0;
		} else {
			cobj1->val->d.str=NULL;
		}
		break;
	case NT_CFUNC :
		cobj1->val->d.cfunc=cobj2->val->d.cfunc;
		break;
	case NT_NULL :
		break;
	case NT_BOOLEAN :
		cobj1->val->d.num=cobj2->val->d.num;
		break;
	default:
		n_error(N, NE_SYNTAX, "n_copyval", "unhandled object type");
	}
	return;
}

void n_joinstr(nes_state *N, obj_t *cobj, char *str, int len)
{
	int olen, tlen;
	char *p;

	if (cobj->val==NULL) return;
	if (cobj->val->type!=NT_STRING) return;
	olen=cobj->val->size;
	tlen=olen+len;
	if (tlen==0) {
		cobj->val->d.str=NULL;
		cobj->val->size=0;
	} else {
		p=realloc(cobj->val->d.str, (tlen+1)*sizeof(char));
		if (p==NULL) n_error(N, NE_MEM, "n_joinstr", "can't alloc %d bytes", tlen+1);
		cobj->val->d.str=p;
		if (str) nc_memcpy(cobj->val->d.str+olen, str, len);
		cobj->val->d.str[tlen]=0;
		cobj->val->size=tlen;
	}
	return;
}

void n_freeval(nes_state *N, obj_t *cobj)
{
	if ((cobj==NULL)||(cobj->val==NULL)) return;
	if ((cobj->val->type==NT_STRING)||(cobj->val->type==NT_NFUNC)) {
		if (cobj->mode&NST_LINK) {
			cobj->mode^=NST_LINK;
			cobj->val->d.str=NULL;
		} else if (cobj->val->d.str!=NULL) {
			n_free(N, (void *)&cobj->val->d.str);
		}
	} else if (cobj->val->type==NT_TABLE) {
		nes_freetable(N, cobj);
	}
	cobj->val->type=NT_NULL;
	cobj->val->size=0;
	return;
}

val_t *n_newval(nes_state *N, unsigned short type)
{
	val_t *val=n_alloc(N, sizeof(val_t));

	/* nc_memset(val, 0, sizeof(val_t)); */
	val->type=type;
	val->refs=1;
	val->size=0;
	val->d.num=0;
	return val;
}

obj_t *n_newiobj(nes_state *N, int index)
{
	obj_t *obj=n_alloc(N, sizeof(obj_t));

	/* nc_memset(obj, 0, sizeof(obj_t)); */
	obj->parent=NULL;
	obj->prev=NULL;
	obj->next=NULL;
	obj->val=NULL;
	obj->mode=0;
	nes_ntoa(N, obj->name, index, 10, 0);
	return obj;
}

/*
 * the following functions are public API functions
 */

void nes_linkval(nes_state *N, obj_t *cobj1, obj_t *cobj2)
{
	if (cobj1==cobj2) return;
	nes_unlinkval(N, cobj1);
	if (cobj2) {
		cobj1->val=cobj2->val;
	} else {
		cobj1->val=n_alloc(N, sizeof(val_t));
		nc_memset(cobj1->val, 0, sizeof(val_t));
	}
	if (cobj1->val) cobj1->val->refs++;
	return;
}

void nes_unlinkval(nes_state *N, obj_t *cobj)
{
	if ((cobj==NULL)||(cobj->val==NULL)) return;
	if (--cobj->val->refs<1) {
		n_freeval(N, cobj);
		n_free(N, (void *)&cobj->val);
	}
	cobj->val=NULL;
	return;
}

obj_t *nes_getobj(nes_state *N, obj_t *tobj, char *oname)
{
	obj_t *cobj;

	if (tobj==&N->r) return tobj;
	if ((tobj==NULL)||(tobj->val->type!=NT_TABLE)) return &_null;
	if (N==NULL) {
		for (cobj=tobj->val->d.table; cobj; cobj=cobj->next) {
			if (cobj->name[0]!=oname[0]) continue;
			if (nc_strcmp(cobj->name, oname)==0) return cobj;
		}
	} else if ((tobj!=&N->l)&&(tobj!=&N->g)) {
		for (cobj=tobj->val->d.table; cobj; cobj=cobj->next) {
			if (cobj->name[0]!=oname[0]) continue;
			if (nc_strcmp(cobj->name, oname)==0) return cobj;
		}
	} else {
		for (cobj=N->l.val->d.table; cobj; cobj=cobj->next) {
			if (cobj->name[0]!=oname[0]) continue;
			if (nc_strcmp(cobj->name, oname)==0) return cobj;
		}
		for (cobj=N->g.val->d.table; cobj; cobj=cobj->next) {
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

	if ((tobj==NULL)||(tobj->val->type!=NT_TABLE)) return &_null;
	for (i=0,cobj=tobj->val->d.table; cobj; i++,cobj=cobj->next) {
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
	char *ostr=NULL;

	if (tobj==&N->r) {
		cobj=tobj;
		nes_unlinkval(N, cobj);
		if (cobj->val==NULL) {
			cobj->val=n_newval(N, NT_NULL);
		} else if ((cobj->val->type==NT_STRING)||(cobj->val->type==NT_NFUNC)) {
			ostr=cobj->val->d.str;
			cobj->val->size=0;
			cobj->val->d.str=NULL;
		}
		cobj->val->type=otype;
	} else {
		if ((tobj==NULL)||(tobj->val==NULL)||(tobj->val->type!=NT_TABLE)) return &_null;
		if (oname[0]=='\0') { return &_null; }
		if (tobj->val->d.table==NULL) {
			tobj->val->d.table=cobj=n_alloc(N, sizeof(obj_t));
			nc_memset(cobj, 0, sizeof(obj_t));
			nc_strncpy(cobj->name, oname, MAX_OBJNAMELEN);
		} else {
			oobj=tobj->val->d.table;
			for (cobj=oobj; cobj; oobj=cobj,cobj=cobj->next) {
				if (nc_isdigit(cobj->name[0])&&nc_isdigit(oname[0])) {
					cmp=(int)(nes_aton(N, cobj->name)-nes_aton(N, oname));
				} else {
					if (cobj->name[0]!=oname[0]) cmp=cobj->name[0]-oname[0];else
					cmp=nc_strcmp(cobj->name, oname);
				}
				if (cmp==0) break;
				if ((cmp>0)&&(tobj->mode&NST_AUTOSORT)) break;
			}
			if ((cmp>0)&&(tobj->mode&NST_AUTOSORT)) {
				oobj=cobj;
				cobj=n_alloc(N, sizeof(obj_t));
				nc_memset(cobj, 0, sizeof(obj_t));
				nc_strncpy(cobj->name, oname, MAX_OBJNAMELEN);
				if (oobj==tobj->val->d.table) tobj->val->d.table=cobj;
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
				if (cobj->val!=NULL) {
					if ((cobj->val->type==NT_STRING)||(cobj->val->type==NT_NFUNC)) {
						ostr=cobj->val->d.str;
						cobj->val->size=0;
						cobj->val->d.str=NULL;
					}
				}
			}
		}
		if (cobj->val==NULL) {
			cobj->val=n_newval(N, otype);
		}
		cobj->val->type=otype;
		cobj->parent=tobj;
		if ((cobj->val->type==NT_TABLE)&&(tobj->mode&NST_AUTOSORT)) cobj->mode|=NST_AUTOSORT;
	}
	switch (otype) {
	case NT_NUMBER:
		cobj->val->d.num=_num;
		break;
	case NT_STRING:
	case NT_NFUNC:
		if (_slen) {
			cobj->val->size=_slen;
			cobj->val->d.str=n_alloc(N, (cobj->val->size+1)*sizeof(char));
			if (_str) {
				nc_memcpy(cobj->val->d.str, _str, cobj->val->size);
				cobj->val->d.str[cobj->val->size]=0;
			}
		} else {
			cobj->val->size=0;
			cobj->val->d.str=NULL;
		}
		break;
	case NT_CFUNC :
		cobj->val->d.cfunc=_fptr;
		break;
	}
	if (ostr) n_free(N, (void *)&ostr);
	return cobj;
}

void nes_freetable(nes_state *N, obj_t *tobj)
{
	obj_t *cobj, *oobj;

	if ((tobj==NULL)||(tobj->val==NULL)||(tobj->val->type!=NT_TABLE)) n_error(N, NE_MEM, "nes_freetable", "tobj is not a table");
	cobj=tobj->val->d.table;
	while (cobj!=NULL) {
		oobj=cobj;
		cobj=cobj->next;
		nes_unlinkval(N, oobj);
		n_free(N, (void *)&oobj);
	}
	tobj->val->d.table=NULL;
	return;
}
