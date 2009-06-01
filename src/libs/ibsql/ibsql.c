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

#ifdef HAVE_IBSQL

#include "nesla/libmysql.h"
#include <stdio.h>
#include <string.h>

static void mysqlDisconnect(nes_state *N, MYSQL_CONN *conn)
{
	if (conn->mysock!=NULL) {
		mysql_close(conn->mysock);
		conn->mysock=NULL;
	}
	return;
}

static int mysqlConnect(nes_state *N, MYSQL_CONN *conn, char *host, char *user, char *pass, char *db, int port)
{
#define __FUNCTION__ __FILE__ ":mysqlConnect()"
	mysql_init(&conn->mysql);
	conn->mysock=mysql_real_connect(&conn->mysql, host, user, pass, db, port, NULL, 0);
	if (conn->mysock==NULL) {
		n_warn(N, __FUNCTION__, "connection error");
		return -1;
	}
	return 0;
#undef __FUNCTION__
}

void mysql_murder(nes_state *N, obj_t *cobj)
{
#define __FUNCTION__ __FILE__ ":mysqlConnect()"
	MYSQL_CONN *conn;

	n_warn(N, __FUNCTION__, "reaper is claiming another lost soul");
	if ((cobj->val->type!=NT_CDATA)||(cobj->val->d.str==NULL)||(nc_strcmp(cobj->val->d.str, "mysql-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a mysql conn");
	conn=(MYSQL_CONN *)cobj->val->d.str;
	mysqlDisconnect(N, conn);
	n_free(N, (void *)&cobj->val->d.str);
	return;
#undef __FUNCTION__
}
/*
static int mysqlUpdate(nes_state *N, MYSQL_CONN *conn, char *sqlquery)
{
	return 0;
}
*/
static int mysqlQuery(nes_state *N, MYSQL_CONN *conn, char *sqlquery, obj_t *qobj)
{
#define __FUNCTION__ __FILE__ ":mysqlQuery()"
	MYSQL_RES *myres;
	MYSQL_ROW MYrow;
	MYSQL_FIELD *MYfield;
	obj_t *robj, *tobj;
	char *p;
	char name[8];
	unsigned int field, numfields, numtuples;

	if (mysql_ping(conn->mysock)!=0) {
		n_warn(N, __FUNCTION__, "connection lost");
		return -1;
	}
	if (mysql_query(conn->mysock, sqlquery)) {
		n_warn(N, __FUNCTION__, "query error");
		return -1;
	}
	if (!(myres=mysql_use_result(conn->mysock))) {
		n_warn(N, __FUNCTION__, "connection error");
		return -1;
	}
	numfields=(int)mysql_num_fields(myres);
	numtuples=0;
	nes_setstr(NULL, qobj, "_query", sqlquery, -1);
	nes_setnum(NULL, qobj, "_fields", numfields);
	nes_setnum(NULL, qobj, "_tuples", numtuples);
	robj=nes_settable(NULL, qobj, "_rows");
	for (numtuples=0;;numtuples++) {
		if ((MYrow=mysql_fetch_row(myres))==NULL) break;
		memset(name, 0, sizeof(name));
		n_ntoa(N, name, numtuples, 10, 0);
		tobj=nes_settable(NULL, robj, name);
		tobj->val->attr&=~NST_AUTOSORT;
		for (field=0;field<numfields;field++) {
			p=MYrow[field]?MYrow[field]:"NULL";
			MYfield=mysql_fetch_field_direct(myres, field);
			nes_setstr(NULL, tobj, MYfield->name, p, -1);
		}
	}
	nes_setnum(NULL, qobj, "_tuples", numtuples);
	mysql_free_result(myres);
	myres=NULL;
	return 0;
#undef __FUNCTION__
}

NES_FUNCTION(neslamysql_query)
{
#define __FUNCTION__ __FILE__ ":neslamysql_query()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1");
	obj_t *cobj2=nes_getobj(N, &N->l, "2");
	obj_t tobj;
	MYSQL_CONN *conn;
	int rc;

	if (cobj1->val->type!=NT_TABLE) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a table for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");
	conn=calloc(1, sizeof(MYSQL_CONN)+1);
	strcpy(conn->obj_type, "mysql-conn");
	rc=mysqlConnect(N, conn, nes_getstr(N, cobj1, "host"), nes_getstr(N, cobj1, "user"), nes_getstr(N, cobj1, "pass"), nes_getstr(N, cobj1, "db"), (int)nes_getnum(N, cobj1, "port"));
	if (rc<0) {
		nes_setstr(N, &N->r, "", "mysql connection error", -1);
		n_free(N, (void *)&conn);
		return -1;
	}
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nes_linkval(N, &tobj, NULL);
	tobj.val->type=NT_TABLE;
	tobj.val->attr&=~NST_AUTOSORT;
	mysqlQuery(N, conn, cobj2->val->d.str, &tobj);
	nes_linkval(N, &N->r, &tobj);
	nes_unlinkval(N, &tobj);
	mysqlDisconnect(N, conn);
	n_free(N, (void *)&conn);
	return 0;
#undef __FUNCTION__
}

int neslamysql_register_all(nes_state *N)
{
	obj_t *tobj;

	tobj=nes_settable(N, &N->g, "mysql");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj, "query", (NES_CFUNC)neslamysql_query);
	return 0;
}

#ifdef PIC
DllExport int neslalib_init(nes_state *N)
{
	neslamysql_register_all(N);
	return 0;
}
#endif

#endif /* HAVE_IBSQL */
