/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2008 Dan Cahill

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

#ifdef HAVE_SQLITE3

#include "nesla/libsqlite3.h"
#include <stdio.h>
#include <string.h>

static void sqlite3Disconnect(nes_state *N, SQLITE3_CONN *conn)
{
	if (conn->db3!=NULL) {
		sqlite3_close(conn->db3);
		conn->db3=NULL;
	}
	return;
}

static int sqlite3Connect(nes_state *N, SQLITE3_CONN *conn, char *dbname)
{
#define __FUNCTION__ __FILE__ ":sqlite3Connect()"
	char *zErrMsg=NULL;

	if (sqlite3_open(dbname, &conn->db3)!=SQLITE_OK) {
		n_warn(N, __FUNCTION__, "connection error - %s", zErrMsg);
		return -1;
	}
	sqlite3_exec(conn->db3, "PRAGMA empty_result_callbacks = ON;", NULL, 0, &zErrMsg);
	/* PRAGMA [database.]synchronous=OFF|ON|NORMAL|FULL */
	sqlite3_exec(conn->db3, "PRAGMA synchronous = OFF;", NULL, 0, &zErrMsg);
	/* PRAGMA temp_store = "default"|"memory"|"file" */
	sqlite3_exec(conn->db3, "PRAGMA temp_store = memory;", NULL, 0, &zErrMsg);
	return 0;
#undef __FUNCTION__
}

void sqlite3_murder(nes_state *N, obj_t *cobj)
{
#define __FUNCTION__ __FILE__ ":sqlite3Connect()"
	SQLITE3_CONN *conn;

	n_warn(N, __FUNCTION__, "reaper is claiming another lost soul");
	if ((cobj->val->type!=NT_CDATA)||(cobj->val->d.str==NULL)||(nc_strcmp(cobj->val->d.str, "sqlite3-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sqlite3 conn");
	conn=(SQLITE3_CONN *)cobj->val->d.str;
	sqlite3Disconnect(N, conn);
	n_free(N, (void *)&cobj->val->d.str);
	return;
#undef __FUNCTION__
}
/*
static int sqlite3Update(nes_state *N, SQLITE3_CONN *conn, char *sqlquery)
{
	return 0;
}
*/

static int sqlite3Callback(void *vptr, int argc, char **argv, char **azColName)
{
	obj_t *qobj=vptr;
	obj_t *tobj;
	char name[8];
	unsigned int field;
	unsigned int numfields;
	unsigned int numtuples;
	char *p;

	numfields=argc;
	numtuples=(int)nes_getnum(NULL, qobj, "_tuples");
	tobj=nes_getobj(NULL, qobj, "_rows");
	if (tobj->val->type!=NT_TABLE) return -1;
	memset(name, 0, sizeof(name));
	n_ntoa(NULL, name, numtuples, 10, 0);
	/* get pointer to this record table */
	tobj=nes_settable(NULL, tobj, name);
	tobj->val->attr&=~NST_AUTOSORT;
	if (numtuples==0) nes_setnum(NULL, qobj, "_fields", numfields);
	for (field=0;field<numfields;field++) {
		if (argv==NULL) continue;
		p=argv[field]?argv[field]:"NULL";
		nes_setstr(NULL, tobj, azColName[field], p, -1);
	}
	if (argv!=NULL) nes_setnum(NULL, qobj, "_tuples", numtuples+1);
	return 0;
}

static int sqlite3Query(nes_state *N, SQLITE3_CONN *conn, char *sqlquery, obj_t *qobj)
{
#define __FUNCTION__ __FILE__ ":sqlite3Query()"
	obj_t *tobj;
	char *zErrMsg=0;
	int rc;

	nes_setstr(NULL, qobj, "_query", sqlquery, -1);
	tobj=nes_settable(NULL, qobj, "_rows");
	if (tobj->val->type!=NT_TABLE) return -1;
	nes_setnum(NULL, qobj, "_fields", 0);
	nes_setnum(NULL, qobj, "_tuples", 0);
	rc=sqlite3_exec(conn->db3, sqlquery, sqlite3Callback, qobj, &zErrMsg);
	if (rc==SQLITE_OK) return 0;
	switch (rc) {
	case SQLITE_BUSY:
	case SQLITE_CORRUPT:
		n_warn(N, __FUNCTION__, "query error (busy or corrupt?) - %d %s", rc, zErrMsg);
		break;
	default:
		n_warn(N, __FUNCTION__, "query error - %d %s", rc, zErrMsg);
		break;
	}
	return -1;
#undef __FUNCTION__
}

NES_FUNCTION(neslasqlite3_query)
{
#define __FUNCTION__ __FILE__ ":neslasqlite3_query()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1");
	obj_t *cobj2=nes_getobj(N, &N->l, "2");
	obj_t tobj;
	SQLITE3_CONN *conn;
	int rc;

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");
	conn=calloc(1, sizeof(SQLITE3_CONN)+1);
	strcpy(conn->obj_type, "sqlite3-conn");
	rc=sqlite3Connect(N, conn, cobj1->val->d.str);
	if (rc<0) {
		nes_setstr(N, &N->r, "", "sqlite3 connection error", -1);
		n_free(N, (void *)&conn);
		return -1;
	}
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nes_linkval(N, &tobj, NULL);
	tobj.val->type=NT_TABLE;
	tobj.val->attr&=~NST_AUTOSORT;
	sqlite3Query(N, conn, cobj2->val->d.str, &tobj);
	nes_linkval(N, &N->r, &tobj);
	nes_unlinkval(N, &tobj);
	sqlite3Disconnect(N, conn);
	n_free(N, (void *)&conn);
	return 0;
#undef __FUNCTION__
}

int neslasqlite3_register_all(nes_state *N)
{
	obj_t *tobj;

	tobj=nes_settable(N, &N->g, "sqlite3");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj, "query", (NES_CFUNC)neslasqlite3_query);
	return 0;
}

#ifdef PIC
DllExport int neslalib_init(nes_state *N)
{
	neslasqlite3_register_all(N);
	return 0;
}
#endif

#endif /* HAVE_SQLITE3 */
