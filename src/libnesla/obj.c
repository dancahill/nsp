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

static obj_t null = { NULL, NULL, NULL, "", NT_NULL, 0, 0, { "" } };

void *n_alloc(nesla_state *N, int size)
{
	char *p=calloc(1, size);

	if (p==NULL) n_error(N, NE_MEM, "n_alloc", "can't alloc %d bytes", size);
	return p;
}

void n_free(nesla_state *N, void **p)
{
	if (*p==NULL) n_error(N, NE_MEM, "n_free", "freeing 0x%08X a second time?", *p);
	free(*p); *p=NULL;
	return;
}

/* change or create an object and return it */
obj_t *n_setobject(nesla_state *N, obj_t *tobj, obj_t *object)
{
	obj_t *oobj;
	obj_t *cobj;

	if (tobj==NULL) n_error(N, NE_MEM, "n_setobject", "can't free a NULL table");
	if (tobj->type!=NT_TABLE) n_error(N, NE_MEM, "n_setobject", "this isn't a table (%d)%s", tobj->type, tobj->name);
	if (object->name[0]=='\0') return &null;
	if (tobj->d.table==NULL) {
		tobj->d.table=n_alloc(N, sizeof(nesla_objrec));
		cobj=tobj->d.table;
	} else {
		oobj=tobj->d.table;
		for (cobj=oobj; cobj; oobj=cobj,cobj=cobj->next) {
			if (strcmp(object->name, cobj->name)==0) break;
		}
		if (cobj==NULL) {
			cobj=n_alloc(N, sizeof(nesla_objrec));
			if (oobj!=NULL) {
				oobj->next=cobj;
				cobj->prev=oobj;
			}
		}
		if (cobj->type==NT_STRING) {
			if (cobj->d.string!=NULL) n_free(N, (void *)&cobj->d.string);
		} else if (cobj->type==NT_NFUNC) {
			if (cobj->d.function!=NULL) n_free(N, (void *)&cobj->d.function);
		} else if (cobj->type==NT_TABLE) {
			/* nesla_freetable(N, cobj); */
		}
	}
	cobj->parent=tobj;
	cobj->type=object->type;
	strncpy(cobj->name, object->name, sizeof(cobj->name)-1);
	if (object->type==NT_NULL) {
		cobj->d.null=object->d.null;
	} else if (object->type==NT_BOOLEAN) {
		cobj->d.boolean=object->d.boolean;
	} else if (object->type==NT_NUMBER) {
		cobj->d.number=object->d.number;
	} else if (object->type==NT_STRING) {
		cobj->size=0; cobj->d.string=NULL;
		if (object->d.string!=NULL) {
			cobj->size=strlen(object->d.string);
			cobj->d.string=n_alloc(N, (cobj->size+1)*sizeof(char));
			strncpy(cobj->d.string, object->d.string, cobj->size);
		}
	} else if (object->type==NT_TABLE) {
	} else if (object->type==NT_NFUNC) {
		cobj->size=0; cobj->d.function=NULL;
		if (object->d.function!=NULL) {
			cobj->size=strlen(object->d.function);
			cobj->d.function=n_alloc(N, (cobj->size+1)*sizeof(char));
			strncpy(cobj->d.function, object->d.function, cobj->size);
		}
	} else if (object->type==NT_CFUNC) {
		cobj->d.cfunction=object->d.cfunction;
	}
	return cobj;
}

/*
 * the following functions are part of the api
 */

obj_t *nesla_getobj(nesla_state *N, obj_t *tobj, char *oname)
{
	obj_t *cobj;

	if (tobj==NULL) n_error(N, NE_MEM, "nesla_getobj", "can't read a NULL table");
	if (tobj->type!=NT_TABLE) n_error(N, NE_MEM, "nesla_getobj", "this isn't a table (%d)%s", tobj->type, tobj->name);
	if (N==NULL) {
		for (cobj=tobj->d.table; cobj; cobj=cobj->next) {
			if (strcmp(oname, cobj->name)==0) return cobj;
		}
	} else if ((tobj!=&N->l)&&(tobj!=&N->g)) {
		for (cobj=tobj->d.table; cobj; cobj=cobj->next) {
			if (strcmp(oname, cobj->name)==0) return cobj;
		}
	} else {
		for (cobj=N->l.d.table; cobj; cobj=cobj->next) {
			if (strcmp(oname, cobj->name)==0) return cobj;
		}
		for (cobj=N->g.d.table; cobj; cobj=cobj->next) {
			if (strcmp(oname, cobj->name)==0) return cobj;
		}
	}
	return &null;
}

obj_t *nesla_getiobj(nesla_state *N, obj_t *tobj, int oindex)
{
	obj_t *cobj;
	int i;

	for (i=0,cobj=tobj->d.table; cobj; i++,cobj=cobj->next) {
		if (i==oindex) return nesla_regstr(N, &N->g, "_retval", cobj->name);
	}
	return &null;
}

obj_t *nesla_regnull(nesla_state *N, obj_t *tobj, char *name)
{
	obj_t obj;

	obj.type=NT_NULL;
	strncpy(obj.name, name, sizeof(obj.name)-1);
	return n_setobject(N, tobj, &obj);
}

obj_t *nesla_regnum(nesla_state *N, obj_t *tobj, char *name, num_t data)
{
	obj_t obj;

	obj.type=NT_NUMBER;
	strncpy(obj.name, name, sizeof(obj.name)-1);
	obj.d.number=data;
	return n_setobject(N, tobj, &obj);
}

obj_t *nesla_regstr(nesla_state *N, obj_t *tobj, char *name, char *data)
{
	obj_t obj;

	obj.type=NT_STRING;
	strncpy(obj.name, name, sizeof(obj.name)-1);
	obj.d.string=data;
	return n_setobject(N, tobj, &obj);
}

obj_t *nesla_regtable(nesla_state *N, obj_t *tobj, char *name)
{
	obj_t obj;

	obj.type=NT_TABLE;
	strncpy(obj.name, name, sizeof(obj.name)-1);
	obj.d.table=NULL;
	return n_setobject(N, tobj, &obj);
}

obj_t *nesla_regnfunc(nesla_state *N, obj_t *tobj, char *name, char *data)
{
	obj_t obj;

	obj.type=NT_NFUNC;
	strncpy(obj.name, name, sizeof(obj.name)-1);
	obj.d.function=data;
	return n_setobject(N, tobj, &obj);
}

obj_t *nesla_regcfunc(nesla_state *N, obj_t *tobj, char *name, void *data)
{
	obj_t obj;

	obj.type=NT_CFUNC;
	strncpy(obj.name, name, sizeof(obj.name)-1);
	obj.d.cfunction=data;
	return n_setobject(N, tobj, &obj);
}

void nesla_freetable(nesla_state *N, obj_t *tobj)
{
	obj_t *cobj;
	obj_t *nobj;

	if (tobj==NULL) n_error(N, NE_MEM, "nesla_freetable", "can't free a NULL table");
	if (tobj->type!=NT_TABLE) n_error(N, NE_MEM, "nesla_freetable", "this isn't a table");
	cobj=tobj->d.table;
	for (;;) {
		if (cobj==NULL) break;
		if (cobj->type==NT_STRING) {
			if (cobj->d.string!=NULL) n_free(N, (void *)&cobj->d.string);
		} else if (cobj->type==NT_NFUNC) {
			if (cobj->d.function!=NULL) n_free(N, (void *)&cobj->d.function);
		} else if (cobj->type==NT_TABLE) {
			/* this may change */
			if (strcmp(cobj->name, "_globals_")!=0) {
				nesla_freetable(N, cobj);
			}
		}
		nobj=cobj->next;
		n_free(N, (void *)&cobj);
		cobj=nobj;
	}
	tobj->d.table=NULL;
	return;
}

void nesla_sorttable(nesla_state *N, obj_t *tobj, int recurse)
{
	obj_t *cobj, *nobj;
	obj_t *robj=tobj;
	short change;
	short swap;

reloop:
	cobj=robj;
	if (cobj==NULL) goto end;
	change=0;
	for (;;) {
		if (cobj==NULL) break;
		if ((cobj->type==NT_TABLE)&&(strcmp(cobj->name, "_globals_")!=0)&&(recurse)) {
			nesla_sorttable(N, cobj, recurse);
		}
		if ((nobj=cobj->next)==NULL) break;
		swap=0;
		if (isdigit(cobj->name[0])&&isdigit(nobj->name[0])) {
			if (atoi(cobj->name)>atoi(nobj->name)) swap=1;
		} else if (strcmp(cobj->name, nobj->name)>0) {
			swap=1;
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
	if (tobj->d.table==N->g.d.table) {
		cobj=nesla_getobj(N, &N->g, "_globals_");
		cobj->d.table=robj;
	}
	tobj->d.table=robj;
	return;
}

num_t nesla_tofloat(nesla_state *N, obj_t *cobj)
{
	num_t n=0;

	if (cobj==NULL) return 0;
	if (cobj->type==NT_NULL) {
		n=0;
	} else if (cobj->type==NT_BOOLEAN) {
		n=cobj->d.boolean?1:0;
	} else if (cobj->type==NT_NUMBER) {
		n=cobj->d.number;
	} else if (cobj->type==NT_STRING) {
		n=atof(cobj->d.string);
	} else if (cobj->type==NT_TABLE) {
		n=0;
	} else if (cobj->type==NT_NFUNC) {
		n=0;
	} else if (cobj->type==NT_CFUNC) {
		n=0;
	}
	return n;
}

int nesla_toint(nesla_state *N, obj_t *cobj)
{
	int n=0;

	if (cobj==NULL) return 0;
	if (cobj->type==NT_NULL) {
		n=0;
	} else if (cobj->type==NT_BOOLEAN) {
		n=cobj->d.boolean?1:0;
	} else if (cobj->type==NT_NUMBER) {
		n=(int)cobj->d.number;
	} else if (cobj->type==NT_STRING) {
		n=atoi(cobj->d.string);
	} else if (cobj->type==NT_TABLE) {
		n=0;
	} else if (cobj->type==NT_NFUNC) {
		n=0;
	} else if (cobj->type==NT_CFUNC) {
		n=0;
	}
	return n;
}

char *nesla_tostr(nesla_state *N, obj_t *cobj)
{
	unsigned int i, j;
	int escape;

	if (cobj==NULL) return "";
	N->txtbuf[0]='\0';
	if (cobj->type==NT_NULL) {
		n_printf(N, 0, "null");
	} else if (cobj->type==NT_BOOLEAN) {
		n_printf(N, 0, "%s", cobj->d.boolean?"true":"false");
	} else if (cobj->type==NT_NUMBER) {
		if (cobj->d.number==(int)cobj->d.number) {
			n_printf(N, 0, "%d", (int)cobj->d.number);
		} else {
			n_printf(N, 0, "%1.15f", cobj->d.number);
			while (N->txtbuf[strlen(N->txtbuf)-1]=='0') N->txtbuf[strlen(N->txtbuf)-1]='\0';
			if (N->txtbuf[strlen(N->txtbuf)-1]=='.') N->txtbuf[strlen(N->txtbuf)-1]='\0';
		}
	} else if (cobj->type==NT_STRING) {
		escape=0;
		for (i=0,j=0;i<cobj->size;i++) {
			if (escape) {
				switch (cobj->d.string[i]) {
				case 'r'  : N->txtbuf[j]='\r'; j++; break;
				case 'n'  : N->txtbuf[j]='\n'; j++; break;
				case 't'  : N->txtbuf[j]='\t'; j++; break;
				case '\'' : N->txtbuf[j]='\''; j++; break;
				case '\"' : N->txtbuf[j]='\"'; j++; break;
				case '\\' : N->txtbuf[j]='\\'; j++; break;
				default   : break;
				}
				escape=0;
			} else {
				if (cobj->d.string[i]=='\\') {
					escape=1;
				} else {
					N->txtbuf[j]=cobj->d.string[i]; j++;
				}
			}
		}
		N->txtbuf[j]='\0';
	} else if (cobj->type==NT_TABLE) {
		n_printf(N, 0, "[...]");
	} else if (cobj->type==NT_NFUNC) {
		n_printf(N, 0, "{...}");
	} else if (cobj->type==NT_CFUNC) {
		n_printf(N, 0, "{...}");
	}
	return N->txtbuf;
}

num_t nesla_getfloat(nesla_state *N, obj_t *tobj, char *oname)
{
	return nesla_tofloat(N, nesla_getobj(N, tobj, oname));
}

int nesla_getint(nesla_state *N, obj_t *tobj, char *oname)
{
	return nesla_toint(N, nesla_getobj(N, tobj, oname));
}

char *nesla_getstr(nesla_state *N, obj_t *tobj, char *oname)
{
	return nesla_tostr(N, nesla_getobj(N, tobj, oname));
}

char *nesla_getfstr(nesla_state *N, obj_t *tobj, char *oname)
{
	return nesla_tostr(N, nesla_getobj(N, tobj, oname));
}

char *nesla_getrstr(nesla_state *N, obj_t *tobj, char *oname)
{
	obj_t *cobj=nesla_getobj(N, tobj, oname);

	if (cobj->type==NT_STRING) return cobj->d.string;
	return "";
}

static void dumpvars(nesla_state *N, obj_t *tobj, int depth)
{
	char indent[20];
	int i;
	short b;
	char *g;
	char *l;
	obj_t *cobj=tobj;

	for (i=0;i<depth;i++) indent[i]='\t';
	indent[i]='\0';
	for ( ; cobj; cobj=cobj->next) {
		if (strcmp(cobj->name, "_retval")==0) continue;
		if ((depth==1)&&(strcmp(cobj->name, "filename")==0)) continue;
		g=(depth<1)?"global ":"";
		l=(depth<1)?";":(cobj->next)?",":"";
		if (cobj->mode&NST_HIDDEN) continue;
		if (isdigit(cobj->name[0])) b=1; else b=0;
		if (cobj->type==NT_NULL) {
			n_printf(N, 1, "%s%s%s%s%s = ", indent, g, b?"[":"", cobj->name, b?"]":"");
			nesla_tostr(N, cobj);
			nl_write(N);
			n_printf(N, 1, "%s\n", l);
		} else if (cobj->type==NT_BOOLEAN) {
			n_printf(N, 1, "%s%s%s%s%s = ", indent, g, b?"[":"", cobj->name, b?"]":"");
			nesla_tostr(N, cobj);
			nl_write(N);
			n_printf(N, 1, "%s\n", l);
		} else if (cobj->type==NT_NUMBER) {
			n_printf(N, 1, "%s%s%s%s%s = ", indent, g, b?"[":"", cobj->name, b?"]":"");
			nesla_tostr(N, cobj);
			nl_write(N);
			n_printf(N, 1, "%s\n", l);
		} else if (cobj->type==NT_STRING) {
			n_printf(N, 1, "%s%s%s%s%s = \"", indent, g, b?"[":"", cobj->name, b?"]":"");
			n_printf(N, 1, "%s", cobj->d.string);
			n_printf(N, 1, "\"%s\n", l);
		} else if (cobj->type==NT_TABLE) {
			if (strcmp(cobj->name, "_globals_")==0) continue;
			n_printf(N, 1, "%s%s%s%s%s = {\n", indent, g, b?"[":"", cobj->name, b?"]":"");
			dumpvars(N, cobj->d.table, depth+1);
			n_printf(N, 1, "%s}%s\n", indent, l);
		}
	}
	return;
}

void nesla_printvars(nesla_state *N, obj_t *tobj)
{
	dumpvars(N, tobj, 0);
}
