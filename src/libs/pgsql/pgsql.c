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

#ifdef HAVE_PGSQL

#include "nesla/libpgsql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void pgsqlDisconnect(nes_state *N, PGSQL_CONN *conn)
{
	if (conn->pgconn!=NULL) {
		PQfinish(conn->pgconn);
		conn->pgconn=NULL;
	}
	return;
}

static int pgsqlConnect(nes_state *N, PGSQL_CONN *conn, char *host, char *user, char *pass, char *db, char *port)
{
	conn->pgconn=PQsetdbLogin(host, port, NULL, NULL, db, user, pass);
	if (PQstatus(conn->pgconn)!=CONNECTION_OK) {
		n_warn(N, "pgsqlConnect", "PGSQL Connect - %s", PQerrorMessage(conn->pgconn));
		return -1;
	}
	return 0;
}

void pgsql_murder(nes_state *N, obj_t *cobj)
{
	PGSQL_CONN *conn;

	n_warn(N, "pgsql_murder", "reaper is claiming another lost soul");
	if ((cobj->val->type!=NT_CDATA)||(cobj->val->d.str==NULL)||(nc_strcmp(cobj->val->d.str, "pgsql-conn")!=0))
		n_error(N, NE_SYNTAX, "pgsql_murder", "expected a pgsql conn");
	conn=(PGSQL_CONN *)cobj->val->d.str;
	pgsqlDisconnect(N, conn);
	n_free(N, (void *)&cobj->val->d.str);
	return;
}
/*
static int pgsqlUpdate(nes_state *N, PGSQL_CONN *conn, char *sqlquery)
{
	return 0;
}
*/
static int pgsqlQuery(nes_state *N, PGSQL_CONN *conn, char *sqlquery, obj_t *qobj)
{
	char *q;
	obj_t *robj, *tobj;
	char *p;
	char name[8];
	unsigned int field, tuple;
	unsigned int numfields, numtuples;

	conn->pgres=PQexec(conn->pgconn, "BEGIN");
	if (PQresultStatus(conn->pgres)!=PGRES_COMMAND_OK) {
		n_warn(N, "pgsqlQuery", "PGSQL BEGIN failed: %s", PQerrorMessage(conn->pgconn));
		PQclear(conn->pgres);
		pgsqlDisconnect(N, conn);
		return -1;
	}
	PQclear(conn->pgres);

	if ((q=calloc(1, strlen(sqlquery)+42))==NULL) {
		n_warn(N, "pgsqlQuery", "calloc error");
		pgsqlDisconnect(N, conn);
		return -1;
	}
	sprintf(q, "DECLARE myportal CURSOR FOR %s", sqlquery);
	conn->pgres=PQexec(conn->pgconn, q);
	free(q);

	if (PQresultStatus(conn->pgres)!=PGRES_COMMAND_OK) {
		n_warn(N, "pgsqlQuery", "PGSQL QUERY failed: %s", PQerrorMessage(conn->pgconn));
		PQclear(conn->pgres);
		pgsqlDisconnect(N, conn);
		return -1;
	}
	PQclear(conn->pgres);

	conn->pgres=PQexec(conn->pgconn, "FETCH ALL IN myportal");
	if (PQresultStatus(conn->pgres)!=PGRES_TUPLES_OK) {
		n_warn(N, "pgsqlQuery", "PGSQL FETCH failed: %s", PQerrorMessage(conn->pgconn));
		PQclear(conn->pgres);
		pgsqlDisconnect(N, conn);
		return -1;
	}
	numfields=(int)PQnfields(conn->pgres);
	numtuples=(int)PQntuples(conn->pgres);
	nes_setstr(NULL, qobj, "_query", sqlquery, strlen(sqlquery));
	nes_setnum(NULL, qobj, "_fields", numfields);
	nes_setnum(NULL, qobj, "_tuples", numtuples);
	robj=nes_settable(NULL, qobj, "_rows");
	for (tuple=0;tuple<numtuples;tuple++) {
		memset(name, 0, sizeof(name));
		sprintf(name, "%d", tuple);
		tobj=nes_settable(NULL, robj, name);
		tobj->val->attr&=~NST_AUTOSORT;
		for (field=0;field<numfields;field++) {
			p=PQgetvalue(conn->pgres, tuple, field);
			p=p?p:"NULL";
			nes_setstr(NULL, tobj, PQfname(conn->pgres, field), p, PQgetlength(conn->pgres, tuple, field));
		}
	}
	nes_setnum(NULL, qobj, "_tuples", numtuples);
	PQclear(conn->pgres);

	conn->pgres=PQexec(conn->pgconn, "CLOSE myportal");
	PQclear(conn->pgres);

	conn->pgres=PQexec(conn->pgconn, "END");
	PQclear(conn->pgres);
	return 0;
}

NES_FUNCTION(neslapgsql_query)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t tobj;
	PGSQL_CONN *conn;
	int rc;

	if (cobj1->val->type!=NT_TABLE) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a table for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	conn=calloc(1, sizeof(PGSQL_CONN)+1);
	strcpy(conn->obj_type, "pgsql-conn");
	rc=pgsqlConnect(N, conn, nes_getstr(N, cobj1, "host"), nes_getstr(N, cobj1, "user"), nes_getstr(N, cobj1, "pass"), nes_getstr(N, cobj1, "db"), nes_getstr(N, cobj1, "port"));
	if (rc<0) {
		nes_setstr(N, &N->r, "", "pgsql connection error", strlen("pgsql connection error"));
		n_free(N, (void *)&conn);
		return -1;
	}
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nes_linkval(N, &tobj, NULL);
	tobj.val->type=NT_TABLE;
	tobj.val->attr&=~NST_AUTOSORT;
	pgsqlQuery(N, conn, cobj2->val->d.str, &tobj);
	nes_linkval(N, &N->r, &tobj);
	nes_unlinkval(N, &tobj);
	pgsqlDisconnect(N, conn);
	n_free(N, (void *)&conn);
	return 0;
}

int neslapgsql_register_all(nes_state *N)
{
	obj_t *tobj;

	tobj=nes_settable(N, &N->g, "pgsql");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj, "query", (NES_CFUNC)neslapgsql_query);
	return 0;
}

#ifdef PIC
DllExport int neslalib_init(nes_state *N)
{
	neslapgsql_register_all(N);
	return 0;
}
#endif

#endif /* HAVE_PGSQL */
