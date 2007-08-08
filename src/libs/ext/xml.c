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
#include "nesla/libext.h"
#include <string.h>
#ifdef WIN32
#define strcasecmp stricmp
#endif
#ifdef __TURBOC__
#define strcasecmp stricmp
#endif

/*
 * do NOT use this. this is TEST code.
 * this code is dangerous, sloppy and exists solely for test purposes.
 * please either ignore it until it goes away, or help fix it.
 */
static char *neslaext_xml_readsub(nes_state *N, obj_t *tobj, char *ptr)
{
	char namebuf[MAX_OBJNAMELEN+1];
	obj_t *cobj;
	obj_t *nobj;
	obj_t *aobj;
	int i, j;
	char *b=ptr, *e, *t, *tb;
	char *p;
	int end;
	char q;

	for (i=0;;i++) {
		nobj=tobj;
		while (nc_isspace(*b)) b++;
		if (*b=='\0') break;
		if ((e=nc_strchr(b, '>'))==NULL) break;
		e++;
		end=0;
		if ((t=nc_strchr(b, '<'))!=NULL) if (t[1]=='/') {
			b=e; break;
		} /* this tag probably closes a previously opened block */
		if (b[e-b-2]=='/') end=1; /* this block is complete */
		/* parse the tag name and attributes here */
		t=b;
		if ((t[0]=='<')&&(t[1]!='?')&&(t[1]!='!')) {
			t++;
			j=0;
			while (nc_isspace(*t)) t++;
			while (nc_isalnum(*t)||(*t==':')||(*t=='_')||(*t=='-')) namebuf[j++]=*t++;
			namebuf[j]='\0';
			nobj=nes_settable(N, tobj, namebuf);
			if (N->debug) n_warn(N, "neslaext_xml_readsub", "new label '%s'", namebuf);
			j=-1;
			for (cobj=nobj->val->d.table; cobj; cobj=cobj->next) {
				if (nc_isdigit(cobj->name[0])) j=(int)n_aton(N, cobj->name);
			}
			nobj=nes_settable(N, nobj, n_ntoa(N, namebuf, ++j, 10, 0));
			if (N->debug) n_warn(N, "neslaext_xml_readsub", "new node '%s'", namebuf);
			aobj=NULL;
			for (;;) {
				while (nc_isspace(*t)) t++;
				if ((*t=='\0')||(*t=='>')) break;
				j=0;
				while (nc_isalnum(*t)||(*t==':')||(*t=='_')||(*t=='-')) namebuf[j++]=*t++;
				namebuf[j]='\0';
				while (nc_isspace(*t)) t++;
				if ((*t=='\0')||(*t=='>')) break;
				if (*t=='=') {
					t++;
					while (nc_isspace(*t)) t++;
					if ((*t=='\0')||(*t=='>')) break;
					if ((*t=='\'')||(*t=='\"')) {
						q=*t;
						tb=++t;
						while ((*t)&&(*t!=q)) t++;
					} else {
						q='\0';
						tb=t;
						while ((*t)&&(*t!='>')&&(!nc_isspace(*t))) t++;
					}
					if (*t) {
						if (aobj==NULL) aobj=nes_settable(N, nobj, "!attributes");
						cobj=nes_setstr(N, aobj, namebuf, tb, t-tb);
						if ((q)&&(*t==q)) t++;
						/* if (N->debug) n_warn(N, "neslaext_xml_readsub", "new attr  '%s->%s' = '%s'", cobj->parent->parent->parent->name, cobj->name, nes_tostr(N, cobj)); */
					}
				}
				while (nc_isspace(*t)) t++;
				if (*t=='/') {
					end=1;
					while (nc_isspace(*t)) t++;
					break;
				}
			}
		}
		b=e;
		t=b;
		while (nc_isspace(*t)) t++;
		e=nc_strchr(t, '<');
		if ((*t)&&(*t!='<')&&(e!=NULL)) { /* might be actual data */
			nes_setstr(N, nobj, "value", b, e-b);
			/* if (b[e-b-2]=='/') end=1; */ /* this block is complete */
			b=e;
		}
		/* if no closing tag, then recurse for the next tree level */
		if ((tobj!=NULL)&&(tobj->name[0]!='!')) {
			/* this is a HACK. */
			/* i'm too lazy to research *ml, but these tags aren't expected to have a closing / */
			p=tobj->name;
			if (strcasecmp(p, "META")==0) end=1;
			if (strcasecmp(p, "LINK")==0) end=1;
			if (strcasecmp(p, "IMG")==0) end=1;
			if (strcasecmp(p, "BR")==0) end=1;
			if (strcasecmp(p, "HR")==0) end=1;
		}
		if (N->debug) n_warn(N, "neslaext_xml_readsub", "... '%s'", tobj->name);
		if (!end) b=neslaext_xml_readsub(N, nobj, b);
		if (b==NULL) break;
	}
	return b;
}

NES_FUNCTION(neslaext_xml_read)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t tobj;

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nes_linkval(N, &tobj, NULL);
	tobj.val->type=NT_TABLE;
	neslaext_xml_readsub(N, &tobj, cobj1->val->d.str);
	nes_linkval(N, &N->r, &tobj);
	nes_unlinkval(N, &tobj);
	return 0;
}
