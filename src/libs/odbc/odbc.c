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

#ifdef HAVE_ODBC

#include "nesla/libodbc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void odbcDisconnect(nes_state *N, ODBC_CONN *conn)
{
	SQLDisconnect(conn->hDBC);
	if (conn->hDBC) {
		SQLFreeHandle(SQL_HANDLE_DBC, conn->hDBC);
		conn->hDBC=NULL;
	}
	if (conn->hENV) {
		SQLFreeHandle(SQL_HANDLE_ENV, conn->hENV);
		conn->hENV=NULL;
	}
	return;
}

static int odbcConnect(nes_state *N, ODBC_CONN *conn, char *dsn)
{
#define __FUNCTION__ __FILE__ ":odbcConnect()"
	RETCODE rc;
	char sqlstate[15];
	char buf[250];

	rc=SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &conn->hENV);
	if ((rc!=SQL_SUCCESS)&&(rc!=SQL_SUCCESS_WITH_INFO)) {
		n_warn(N, __FUNCTION__, "Unable to allocate an environment handle.");
		return -1;
	}
	rc=SQLSetEnvAttr(conn->hENV, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
	if ((rc!=SQL_SUCCESS)&&(rc!=SQL_SUCCESS_WITH_INFO)) {
		SQLError(conn->hENV, conn->hDBC, conn->hSTMT, (SQLPOINTER)sqlstate, NULL, (SQLPOINTER)buf, sizeof(buf), NULL);
		n_warn(N, __FUNCTION__, "SQLSetEnvAttr %s", buf);
		odbcDisconnect(N, conn);
		return -1;
	}
	rc=SQLAllocHandle(SQL_HANDLE_DBC, conn->hENV, &conn->hDBC);
	if ((rc!=SQL_SUCCESS)&&(rc!=SQL_SUCCESS_WITH_INFO)) {
		SQLError(conn->hENV, conn->hDBC, conn->hSTMT, (SQLPOINTER)sqlstate, NULL, (SQLPOINTER)buf, sizeof(buf), NULL);
		n_warn(N, __FUNCTION__, "SQLAllocHandle %s", buf);
		odbcDisconnect(N, conn);
		return -1;
	}
	rc=SQLDriverConnect(conn->hDBC, NULL, (SQLPOINTER)dsn, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
	if ((rc!=SQL_SUCCESS)&&(rc!=SQL_SUCCESS_WITH_INFO)) {
		SQLError(conn->hENV, conn->hDBC, conn->hSTMT, (SQLPOINTER)sqlstate, NULL, (SQLPOINTER)buf, sizeof(buf), NULL);
		n_warn(N, __FUNCTION__, "SQLDriverConnect %s", buf);
		odbcDisconnect(N, conn);
		return -1;
	}
	return 0;
#undef __FUNCTION__
}

void odbc_murder(nes_state *N, obj_t *cobj)
{
#define __FUNCTION__ __FILE__ ":odbc_murder()"
	ODBC_CONN *conn;

	n_warn(N, __FUNCTION__, "reaper is claiming another lost soul");
	if ((cobj->val->type!=NT_CDATA)||(cobj->val->d.str==NULL)||(strcmp(cobj->val->d.str, "odbc-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected an odbc conn");
	conn=(ODBC_CONN *)cobj->val->d.str;
	odbcDisconnect(N, conn);
	n_free(N, (void *)&cobj->val->d.str);
	return;
#undef __FUNCTION__
}

#define FBUFSIZE 1024

static int odbcQuery(nes_state *N, ODBC_CONN *conn, char *sqlquery, obj_t *qobj)
{
#define __FUNCTION__ __FILE__ ":odbcQuery()"
	SQLSMALLINT pccol;
	SDWORD slen;
	RETCODE rc;
	char sqlstate[15];
	char buf[250];
	char name[8];
	char colname[MAX_OBJNAMELEN+1];
	unsigned int field, numfields, numtuples;
	obj_t *robj, *tobj, *cobj;
	char fbuf[FBUFSIZE];

	rc=SQLAllocHandle(SQL_HANDLE_STMT, conn->hDBC, &conn->hSTMT);
	if ((rc!=SQL_SUCCESS)&&(rc!=SQL_SUCCESS_WITH_INFO)) {
		SQLError(conn->hENV, conn->hDBC, conn->hSTMT, (SQLPOINTER)sqlstate, NULL, (SQLPOINTER)buf, sizeof(buf), NULL);
		n_warn(N, __FUNCTION__, "SQLAllocHandle %s", buf);
		return -1;
	}
	rc=SQLExecDirect(conn->hSTMT, (SQLPOINTER)sqlquery, SQL_NTS);
	if ((rc!=SQL_SUCCESS)&&(rc!=SQL_SUCCESS_WITH_INFO)) {
		SQLError(conn->hENV, conn->hDBC, conn->hSTMT, (SQLPOINTER)sqlstate, NULL, (SQLPOINTER)buf, sizeof(buf), NULL);
		n_warn(N, __FUNCTION__, "SQLExecDirect %s", buf);
		n_warn(N, __FUNCTION__, "[%s]", sqlquery);
		SQLFreeHandle(SQL_HANDLE_STMT, conn->hSTMT);
		conn->hSTMT=NULL;
		return -1;
	}
	rc=SQLNumResultCols(conn->hSTMT, &pccol);
	if ((rc!=SQL_SUCCESS)&&(rc!=SQL_SUCCESS_WITH_INFO)) {
		SQLError(conn->hENV, conn->hDBC, conn->hSTMT, (SQLPOINTER)sqlstate, NULL, (SQLPOINTER)buf, sizeof(buf), NULL);
		n_warn(N, __FUNCTION__, "SQLNumResultCols %s", buf);
		return -1;
	}
	numfields=pccol;
	numtuples=0;
	nes_setstr(NULL, qobj, "_query", sqlquery, -1);
	nes_setnum(NULL, qobj, "_fields", numfields);
	nes_setnum(NULL, qobj, "_tuples", numtuples);
	robj=nes_settable(NULL, qobj, "_rows");
	for (numtuples=0;;numtuples++) {
		rc=SQLFetch(conn->hSTMT);
		if ((rc!=SQL_SUCCESS)&&(rc!=SQL_SUCCESS_WITH_INFO)) break;
		memset(name, 0, sizeof(name));
		n_ntoa(N, name, numtuples, 10, 0);
		tobj=nes_settable(NULL, robj, name);
		tobj->val->attr&=~NST_AUTOSORT;
		for (field=0;field<numfields;field++) {
			rc=SQLDescribeCol(conn->hSTMT, (SQLSMALLINT)(field+1), (SQLPOINTER)colname, MAX_OBJNAMELEN, NULL, NULL, NULL, NULL, NULL);
			cobj=nes_setstr(NULL, tobj, colname, NULL, 0);
			do {
				rc=SQLGetData(conn->hSTMT, (SQLUSMALLINT)(field+1), SQL_C_CHAR, fbuf, FBUFSIZE, &slen);
				if ((rc!=SQL_SUCCESS)&&(rc!=SQL_SUCCESS_WITH_INFO)) {
					n_warn(N, __FUNCTION__, "Error %d retrieving field %d:%d", rc, numtuples, field);
					goto err;
				}
				if (slen>0) nes_strcat(NULL, cobj, fbuf, slen<FBUFSIZE?slen:FBUFSIZE-1);
			} while (rc==SQL_SUCCESS_WITH_INFO);
		}
	}
err:
	nes_setnum(NULL, qobj, "_tuples", numtuples);
	SQLFreeHandle(SQL_HANDLE_STMT, conn->hSTMT);
	conn->hSTMT=NULL;
	return 0;
#undef __FUNCTION__
}

NES_FUNCTION(neslaodbc_query)
{
#define __FUNCTION__ __FILE__ ":neslaodbc_query()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1");
	obj_t *cobj2=nes_getobj(N, &N->l, "2");
	ODBC_CONN *conn;
	int rc;
	obj_t tobj;

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");
	conn=calloc(1, sizeof(ODBC_CONN)+1);
	strcpy(conn->obj_type, "odbc-conn");
	rc=odbcConnect(N, conn, cobj1->val->d.str);
	if (rc<0) {
		nes_setstr(N, &N->r, "", "odbc connection error", -1);
		n_free(N, (void *)&conn);
		return -1;
	}
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nes_linkval(N, &tobj, NULL);
	tobj.val->type=NT_TABLE;
	tobj.val->attr&=~NST_AUTOSORT;
	odbcQuery(N, conn, cobj2->val->d.str, &tobj);
	nes_linkval(N, &N->r, &tobj);
	nes_unlinkval(N, &tobj);
	odbcDisconnect(N, conn);
	n_free(N, (void *)&conn);
	return 0;
#undef __FUNCTION__
}

int neslaodbc_register_all(nes_state *N)
{
	obj_t *tobj;

	tobj=nes_settable(N, &N->g, "odbc");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj, "query", (NES_CFUNC)neslaodbc_query);
	return 0;
}

#ifdef PIC
DllExport int neslalib_init(nes_state *N)
{
	neslaodbc_register_all(N);
	return 0;
}
#endif

#endif /* HAVE_ODBC */
