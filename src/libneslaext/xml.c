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
#include "libneslaext.h"

/*
 * do NOT use this. this is TEST code.
 * this code is dangerous, sloppy and exists solely for test purposes
 */
static char *neslaext_xml_readsub(nes_state *N, obj_t *tobj, char *ptr)
{
	char namebuf[MAX_OBJNAMELEN+1];
	obj_t *cobj;
	obj_t *nobj;
	obj_t *aobj;
	int i, j;
	char *b=ptr, *e, *t, *tb;
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
			while (nc_isalnum(*t)||(*t==':')||(*t=='_')) namebuf[j++]=*t++;
			namebuf[j]='\0';
			nobj=nes_settable(N, tobj, namebuf);
			j=-1;
			for (cobj=nobj->d.table; cobj; cobj=cobj->next) {
				if (nc_isdigit(cobj->name[0])) j=(int)nes_aton(N, cobj->name);
			}
			nobj=nes_settable(N, nobj, nes_ntoa(N, namebuf, ++j, 10, 0));
			aobj=NULL;
			for (;;) {
				while (nc_isspace(*t)) t++;
				if ((*t=='\0')||(*t=='>')) break;
				j=0;
				while (nc_isalnum(*t)||(*t==':')||(*t=='_')) namebuf[j++]=*t++;
				namebuf[j]='\0';
				while (nc_isspace(*t)) t++;
				if ((*t=='\0')||(*t=='>')) break;
				if (*t=='=') {
					t++;
					while (nc_isspace(*t)) t++;
					if ((*t=='\0')||(*t=='>')) break;
					if ((*t=='\'')||(*t=='\'')) {
						q=*t;
						tb=++t;
						while (*t&&*t!=q) t++;
						if (*t) {
							if (aobj==NULL) aobj=nes_settable(N, nobj, "!attributes");
							nes_setstr(N, aobj, namebuf, tb, t-tb); t++;
						}
					}
				}
				if (*t=='/') break;
			}
		}
		b=e;
		t=b;
		while (nc_isspace(*t)) t++;
		e=nc_strchr(t, '<');
		if ((*t!='\0')&&(*t!='<')&&(e!=NULL)) { /* might be actual data */
			nes_setstr(N, nobj, "value", b, e-b);
			/* if (b[e-b-2]=='/') end=1; */ /* this block is complete */
			b=e;
		}
		/* if no closing tag, then recurse for the next tree level */
		if (!end) b=neslaext_xml_readsub(N, nobj, b);
		if (b==NULL) break;
	}
	return b;
}

int neslaext_xml_read(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t *tobj=NULL;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	tobj=nes_settable(NULL, &N->g, cobj1->d.str);
	neslaext_xml_readsub(N, tobj, cobj2->d.str);
	return 0;
}
