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

#ifdef HAVE_LDAP

#include "nesla/libldap.h"
#define LDAP_DEPRECATED 1
#include <ctype.h>
#include <ldap.h>
#include <stdio.h>

/*
 * someone who actually knows ldap may want to replace
 * this with code that doesn't suck quite so much...
 */
NES_FUNCTION(neslaldap_search)
{
#define __FUNCTION__ __FILE__ ":neslaldap_search()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* host   */
	obj_t *cobj2=nes_getobj(N, &N->l, "2"); /* port   */
	obj_t *cobj3=nes_getobj(N, &N->l, "3"); /* basedn */
	obj_t *cobj4=nes_getobj(N, &N->l, "4"); /* search */
	obj_t qobj, *robj, *tobj;
	LDAP *ld;
	LDAPMessage *e, *res;
	BerElement *ber;
	char *dn, *a, **vals;
	int i, numtuples, port;
	char name[8];
	char *host;
	char *basedn;
	char *search="o=gw.nulllogic.lan";

	char tmpbuf[MAX_OBJNAMELEN];
	char *p;

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg1");
	if (cobj2->val->type!=NT_NUMBER) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a number for arg2");
	if (cobj3->val->type!=NT_STRING) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg3");
	if (cobj4->val->type!=NT_STRING) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg4");
	host=cobj1->val->d.str;
	port=(int)cobj2->val->d.num;
	basedn=cobj3->val->d.str;
	search=cobj4->val->d.str;
/*	ldap_create */
	if ((ld=ldap_init(host, port))==NULL) {
		n_warn(N, __FUNCTION__, "error connecting to %s", host);
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
/*	ldap_sasl_bind_s */
	if (ldap_simple_bind_s(ld, NULL, NULL)!=LDAP_SUCCESS) {
		n_warn(N, __FUNCTION__, "error binding to %s", host);
		nes_setnum(N, &N->r, "", -2);
		return -1;
	}
/*	ldap_search_ext_s */
	if (ldap_search_s(ld, basedn, LDAP_SCOPE_SUBTREE, search, NULL, 0, &res)!=LDAP_SUCCESS) {
		n_warn(N, __FUNCTION__, "error searching %s", host);
		nes_setnum(N, &N->r, "", -2);
		return -1;
	}
	nc_memset((void *)&qobj, 0, sizeof(obj_t));
	nes_linkval(N, &qobj, NULL);
	qobj.val->type=NT_TABLE;
	qobj.val->attr&=~NST_AUTOSORT;
	numtuples=ldap_count_entries(ld, res);
	nes_setnum(NULL, &qobj, "_tuples", numtuples);
	robj=nes_settable(NULL, &qobj, "_rows");
	robj->val->attr&=~NST_AUTOSORT;
	numtuples=0;
	for (e=ldap_first_entry(ld, res); e!=NULL; e=ldap_next_entry(ld, e)) {
		memset(name, 0, sizeof(name));
		n_ntoa(N, name, numtuples++, 10, 0);
		tobj=nes_settable(NULL, robj, name);
		tobj->val->attr&=~NST_AUTOSORT;
		dn=ldap_get_dn(ld, e);
		nes_setstr(NULL, tobj, "dn", dn, -1);
		ldap_memfree(dn);
		for (a=ldap_first_attribute(ld, e, &ber); a!=NULL; a=ldap_next_attribute(ld, e, ber)) {
/*			ldap_get_values_len */
			if ((vals=ldap_get_values(ld, e, a))!=NULL) {

				p=a;
				for (i=0;i<MAX_OBJNAMELEN;i++) {
					if (*p=='\0') { tmpbuf[i]='\0'; break; }
					if (*p=='=') { tmpbuf[i]='\0'; p++; break; }
					tmpbuf[i]=tolower(*p);
					p++;
				}
				tmpbuf[MAX_OBJNAMELEN-1]='\0';

				for (i=0; vals[i]!=NULL; i++) {
					nes_setstr(NULL, tobj, tmpbuf, vals[i], -1);
				}
/*				ldap_values_free_len */
				ldap_value_free(vals);
			}
		}
		if (ber!=NULL) ber_free(ber, 0);
	}
	ldap_msgfree(res);
/*	ldap_unbind_ext_s */
	ldap_unbind(ld);
	nes_linkval(N, &N->r, &qobj);
	nes_unlinkval(N, &qobj);
	return 0;
#undef __FUNCTION__
}

int neslaldap_register_all(nes_state *N)
{
	obj_t *tobj;

	tobj=nes_settable(N, &N->g, "ldap");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj, "search",  (NES_CFUNC)neslaldap_search);
	return 0;
}

#ifdef PIC
DllExport int neslalib_init(nes_state *N)
{
	neslaldap_register_all(N);
	return 0;
}
#endif

#endif /* HAVE_LDAP */
