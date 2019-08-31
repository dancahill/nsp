/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2019 Dan Cahill

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
#include "nsp/nsplib.h"

#ifdef HAVE_MONGODB

#include <libbson-1.0/bson.h>
#include <libmongoc-1.0/mongoc.h>

void mongodb_murder(nsp_state *N, obj_t *cobj);

typedef struct MONGODB_CONN {
	/* standard header info for CDATA object */
	char      obj_type[16]; /* tell us all about yourself in 15 characters or less */
	NSP_CFREE obj_term;     /* now tell us how to kill you */
	/* now begin the stuff that's socket-specific */
	mongoc_client_t *client;
	mongoc_database_t *database;
	mongoc_collection_t *collection;
} MONGODB_CONN;

static MONGODB_CONN *getconn(nsp_state *N)
{
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *cobj;
	MONGODB_CONN *conn;

	cobj = nsp_getobj(N, thisobj, "connection");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "mongodb-conn") != 0))
		n_error(N, NE_SYNTAX, "getconn", "expected a mongodb-conn");
	conn = (MONGODB_CONN *)cobj->val->d.str;
	return conn;
}

static void mongodb_disconnect(nsp_state *N, MONGODB_CONN *conn)
{
	if (conn->collection) {
		mongoc_collection_destroy(conn->collection);
		conn->collection=NULL;
	}
	if (conn->database) {
		mongoc_database_destroy(conn->database);
		conn->database=NULL;
	}
	if (conn->client) {
		mongoc_client_destroy(conn->client);
		conn->client=NULL;
	}
	mongoc_cleanup();
	return;
}

void mongodb_murder(nsp_state *N, obj_t *cobj)
{
#define __FN__ __FILE__ ":mongodb_murder()"
	MONGODB_CONN *conn;

	n_warn(N, __FN__, "reaper is claiming another lost soul");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (nc_strcmp(cobj->val->d.str, "mongodb-conn") != 0))
		n_error(N, NE_SYNTAX, __FN__, "expected a mongodb conn");
	conn = (MONGODB_CONN *)cobj->val->d.str;
	mongodb_disconnect(N, conn);
	n_free(N, (void *)& cobj->val->d.str, sizeof(MONGODB_CONN) + 1);
	cobj->val->size = 0;
	return;
#undef __FN__
}

static int mongodb_connect(nsp_state *N, MONGODB_CONN *conn, char *url, char *db, char *collection)
{
#define __FN__ __FILE__ ":mongodbConnect()"
	mongoc_init();
	conn->client = mongoc_client_new(url);
	conn->database = mongoc_client_get_database(conn->client, db);
	conn->collection = mongoc_client_get_collection(conn->client, db, collection);
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_open)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_open()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *cobj;
	MONGODB_CONN *conn;
	char *url = NULL;
	char *db = NULL;
	char *coll = NULL;
	int rc;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	if (nsp_isstr((cobj = nsp_getobj(N, &N->context->l, "1")))) {
		url = cobj->val->d.str;
	} else if (nsp_isstr((cobj = nsp_getobj(N, thisobj, "url")))) {
		url = cobj->val->d.str;
	} else {
		n_error(N, NE_SYNTAX, __FN__, "expected a string for url");
	}
	if (nsp_isnum((cobj = nsp_getobj(N, &N->context->l, "2")))) {
		db = cobj->val->d.str;
	} else if (nsp_isstr((cobj = nsp_getobj(N, thisobj, "database")))) {
		db = cobj->val->d.str;
	} else {
		n_error(N, NE_SYNTAX, __FN__, "expected a string for db");
	}
	if (nsp_isstr((cobj = nsp_getobj(N, &N->context->l, "3")))) {
		coll = cobj->val->d.str;
	} else if (nsp_isstr((cobj = nsp_getobj(N, thisobj, "collection")))) {
		coll = cobj->val->d.str;
	} else {
		n_error(N, NE_SYNTAX, __FN__, "expected a string for collection");
	}
	conn = n_alloc(N, sizeof(MONGODB_CONN) + 1, 1);
	strcpy(conn->obj_type, "mongodb-conn");
	rc = mongodb_connect(N, conn, url, db, coll);
	if (rc < 0) {
		nsp_setstr(N, &N->r, "", "mongodb connection error", -1);
		n_free(N, (void *)&conn, sizeof(MONGODB_CONN) + 1);
		return -1;
	}
	cobj = nsp_setcdata(N, thisobj, "connection", NULL, 0);
	cobj->val->d.str = (void *)conn;
	cobj->val->size = sizeof(MONGODB_CONN) + 1;
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_close)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_close()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *cobj;
	MONGODB_CONN *conn;

	cobj = nsp_getobj(N, thisobj, "connection");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "mongodb-conn") != 0))
		return 0;
	/// n_error(N, NE_SYNTAX, __FN__, "expected a mongodb-conn");
	conn = (MONGODB_CONN *)cobj->val->d.str;
	mongodb_disconnect(N, conn);
	n_free(N, (void *)&cobj->val->d.str, sizeof(MONGODB_CONN) + 1);
	cobj->val->size = 0;
	nsp_setbool(N, thisobj, "connection", 0);
	nsp_setnum(N, &N->r, "", 0);
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_query)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_query()"
/*
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	MONGODB_CONN *conn = getconn(N);
	obj_t *cobj;
	char *sqlquery = NULL;
	//short expect_results = 1;

	if (nsp_isstr((cobj = nsp_getobj(N, &N->context->l, "1")))) {
		sqlquery = cobj->val->d.str;
	}
	else if (nsp_isstr((cobj = nsp_getobj(N, thisobj, "sqlquery")))) {
		sqlquery = cobj->val->d.str;
	}
	else {
		n_error(N, NE_SYNTAX, __FN__, "expected a string for sqlquery");
	}
	//if (nsp_isbool((cobj = nsp_getobj(N, &N->context->l, "2")))) {
	//	expect_results = nsp_tobool(N, cobj);
	//}
	if (mongodb_ping(conn->mysock) != 0) {
		n_warn(N, __FN__, "connection lost");
		return -1;
	}
	if (mongodb_query(conn->mysock, sqlquery)) {
		n_warn(N, __FN__, "query error '%s'\r\n%s", mongodb_error(conn->mysock), sqlquery);
		return -1;
	}
	nsp_setstr(N, thisobj, "last_query", sqlquery, -1);
	nsp_setnum(N, thisobj, "changes", (long)mongodb_affected_rows(&conn->mongodb));
*/
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_getnext)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_getnext()"
/*
	MONGODB_CONN *conn = getconn(N);
	MONGODB_ROW MYrow;
	MONGODB_FIELD *MYfield;
	obj_t tobj;
	char *p;
	unsigned int field, numfields;

	if (conn->myres == NULL) {
		if (!(conn->myres = mongodb_use_result(conn->mysock))) {
			n_warn(N, __FN__, "mongodb_use_result error '%s'", mongodb_error(conn->mysock));
		}
	}
	if ((MYrow = mongodb_fetch_row(conn->myres)) == NULL) {
		if (conn->myres != NULL) {
			mongodb_free_result(conn->myres);
			conn->myres = NULL;
		}
		return 0;
	}
	numfields = (int)mongodb_num_fields(conn->myres);
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	tobj.val = n_newval(N, NT_TABLE);
	tobj.val->attr &= ~NST_AUTOSORT;
	for (field = 0;field < numfields;field++) {
		p = MYrow[field] ? MYrow[field] : "NULL";
		MYfield = mongodb_fetch_field_direct(conn->myres, field);
		switch (MYfield->type) {
		case MONGODB_TYPE_DECIMAL:
		case MONGODB_TYPE_LONG:
			nsp_setnum(N, &tobj, MYfield->name, atoi(p));
			break;
		case MONGODB_TYPE_FLOAT:
			nsp_setstr(N, &tobj, MYfield->name, p, -1);
			break;
		case MONGODB_TYPE_VAR_STRING:
			nsp_setstr(N, &tobj, MYfield->name, p, -1);
			break;
		case MONGODB_TYPE_NULL:
			nsp_setstr(N, &tobj, MYfield->name, "NULL", -1);
			break;
		default:
			nsp_setstr(N, &tobj, MYfield->name, p, -1);
			n_warn(N, __FN__, "Unhandled type %d for %s!", MYfield->type, MYfield->name);
			break;
		}
	}
	nsp_linkval(N, &N->r, &tobj);
	nsp_unlinkval(N, &tobj);
*/
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_endquery)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_endquery()"
/*
	MONGODB_CONN *conn = getconn(N);

	if (conn->myres != NULL) {
		mongodb_free_result(conn->myres);
		conn->myres = NULL;
	}
*/
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_version)
{
/*
	char buf[64];

	snprintf(buf, sizeof(buf) - 1, "%s / %s", "SQLITE_VERSION", mongodb_get_client_info());
	nsp_setstr(N, &N->r, "", buf, -1);
*/
	return 0;
}

static void print_json(const bson_t *bson)
{
	char *string;

	string = bson_as_json(bson, NULL);
	printf("\t%s\n", string);
	bson_free(string);
}

NSP_CLASSMETHOD(libnsp_data_mongodb_test)
{
/*	
	client = mongoc_client_new("mongodb://localhost:27017/?appname=executing-example");
	database = mongoc_client_get_database(client, "nulltest");
	collection = mongoc_client_get_collection(client, "nulltest", "users");
	"db.users.insertOne( { name: "Bob", age: 42 } )"
	const char  *json = "{\"name\": {\"first\":\"Grace\", \"last\":\"Hopper\"}}";
	const char  *jsonquery = "db.users.find()";
	const char  *jsoninsert = "db.users.insert({\"name\": {\"first\":\"Grace\", \"last\":\"Hopper\"}})";
	const char  *jsoninsert = "{\"name\": {\"first\":\"Grace\", \"last\":\"Hopper\"}}";
	const char  *jsonquery = "db.users.find()";
	db.adminCommand( { "hostInfo" : 1 } )
*/
	MONGODB_CONN *conn = getconn(N);
	bson_error_t error;
	bson_t *command;
	bson_t reply;
	const bson_t *doc;
	mongoc_cursor_t *cursor;
	const char *insertjson = "{\"name\": {\"first\":\"Grace\", \"last\":\"Hopper\"}}";
	bson_oid_t oid;

	mongodb_connect(N, conn, "mongodb://localhost:27017/?appname=executing-example", "nulltest", "users");
	printf("-------------\n");

	// { "ping" : 1 }
	command = BCON_NEW("ping", BCON_INT32(1));
	print_json(command);
	if (mongoc_client_command_simple(conn->client, "admin", command, NULL, &reply, &error)) {
		print_json(&reply);
	} else {
		fprintf(stderr, "%s\n", error.message);
	}
	bson_destroy(command);
	bson_destroy(&reply);
	printf("-------------\n");

	// db.adminCommand( { "hostInfo" : 1 } )
	// { "hostInfo" : 1 }
	command = BCON_NEW("hostInfo", BCON_INT32(1));
	print_json(command);
	if (mongoc_client_command_simple(conn->client, "admin", command, NULL, &reply, &error)) {
		print_json(&reply);
	} else {
		fprintf(stderr, "%s\n", error.message);
	}
	bson_destroy(command);
	bson_destroy(&reply);
	printf("-------------\n");

	// { "collStats" : "users" }
	command = BCON_NEW("collStats", BCON_UTF8("users"));
	print_json(command);
	if (mongoc_collection_command_simple(conn->collection, command, NULL, &reply, &error)) {
		print_json(&reply);
	} else {
		fprintf(stderr, "Failed to run command: %s\n", error.message);
	}
	bson_destroy(command);
	bson_destroy(&reply);
	printf("-------------\n");

	// { "name" : { "first" : "Grace", "last" : "Hopper" } }
	command = bson_new_from_json((const uint8_t *)insertjson, -1, &error);
	print_json(command);
	if (mongoc_collection_insert(conn->collection, MONGOC_INSERT_NONE, command, NULL, &error)) {
		//print_json(&reply);
	} else {
		fprintf(stderr, "Failed to run command: %s\n", error.message);
	}
	bson_destroy(command);
	printf("-------------\n");

	command = bson_new();
	print_json(command);
	cursor = mongoc_collection_find(conn->collection, MONGOC_QUERY_NONE, 0, 0, 0, command, NULL, NULL);
	while (mongoc_cursor_next(cursor, &doc)) {
		print_json(doc);
	}
	bson_destroy(command);
	printf("-------------\n");
	mongoc_cursor_destroy(cursor);

	command = bson_new();
	bson_oid_init_from_string(&oid, "5d6ad3157c71de5036338641");
	BSON_APPEND_OID(command, "_id", &oid);
	print_json(command);
	if (mongoc_collection_remove(conn->collection, MONGOC_REMOVE_SINGLE_REMOVE, command, NULL, &error)) {
		printf("deleted?\n");
	} else {
		printf("Delete failed: %s\n", error.message);
	}
	bson_destroy(command);
	printf("-------------\n");

	mongodb_disconnect(N, conn);
	return 0;
}

NSP_CLASS(libnsp_data_mongodb_client)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_client()"
	obj_t *tobj, *cobj;

//client = mongoc_client_new("mongodb://localhost:27017/?appname=executing-example");
//database = mongoc_client_get_database(client, "nulltest");
//collection = mongoc_client_get_collection(client, "nulltest", "users");

	nsp_setstr(N, &N->context->l, "url", "mongodb://localhost:27017/?appname=executing-example", -1);
	nsp_setstr(N, &N->context->l, "database", "nulltest", -1);
	nsp_setstr(N, &N->context->l, "collection", "users", -1);
//	nsp_setstr(N, &N->context->l, "username", "root", 4);
//	nsp_setstr(N, &N->context->l, "password", "", 0);
	nsp_setbool(N, &N->context->l, "connection", 0);
	if (nsp_istable((tobj = nsp_getobj(N, &N->context->l, "1")))) {
		//if (nsp_isstr((cobj = nsp_getobj(N, tobj, "username")))) {
		//	nsp_setstr(N, &N->context->l, "username", cobj->val->d.str, cobj->val->size);
		//}
		//if (nsp_isstr((cobj = nsp_getobj(N, tobj, "password")))) {
		//	nsp_setstr(N, &N->context->l, "password", cobj->val->d.str, cobj->val->size);
		//}
		//if (nsp_isstr((cobj = nsp_getobj(N, tobj, "hostname")))) {
		//	nsp_setstr(N, &N->context->l, "host", cobj->val->d.str, cobj->val->size);
		//}
		//if (nsp_isstr((cobj = nsp_getobj(N, tobj, "host")))) {
		//	nsp_setstr(N, &N->context->l, "host", cobj->val->d.str, cobj->val->size);
		//}
		//if (nsp_isstr((cobj = nsp_getobj(N, tobj, "database")))) {
		//	nsp_setstr(N, &N->context->l, "database", cobj->val->d.str, cobj->val->size);
		//}
	}

	cobj = nsp_getobj(N, nsp_getobj(N, &N->g, "data"), "mongodb");
	if (nsp_istable(cobj)) nsp_zlink(N, &N->context->l, cobj);
	else n_warn(N, __FN__, "data.mongodb not found");
	cobj = nsp_getobj(N, nsp_getobj(N, nsp_getobj(N, &N->g, "data"), "sql"), "common");
	if (nsp_istable(cobj)) nsp_zlink(N, &N->context->l, cobj);
	else n_warn(N, __FN__, "data.sql.common not found");
	return 0;
#undef __FN__
}

int nspmongodb_register_all(nsp_state *N)
{
	obj_t *tobj;

	tobj = nsp_settable(N, &N->g, "data");
	tobj->val->attr |= NST_HIDDEN;
	tobj = nsp_settable(N, tobj, "mongodb");
	tobj->val->attr |= NST_HIDDEN;
	nsp_setcfunc(N, tobj, "client", (NSP_CFUNC)libnsp_data_mongodb_client);
	nsp_setcfunc(N, tobj, "open", (NSP_CFUNC)libnsp_data_mongodb_open);
	nsp_setcfunc(N, tobj, "close", (NSP_CFUNC)libnsp_data_mongodb_close);
	nsp_setcfunc(N, tobj, "test", (NSP_CFUNC)libnsp_data_mongodb_test);
	nsp_setcfunc(N, tobj, "query", (NSP_CFUNC)libnsp_data_mongodb_query);
	nsp_setcfunc(N, tobj, "getnext", (NSP_CFUNC)libnsp_data_mongodb_getnext);
	nsp_setcfunc(N, tobj, "endquery", (NSP_CFUNC)libnsp_data_mongodb_endquery);
//	nsp_setcfunc(N, tobj, "version", (NSP_CFUNC)libnsp_data_mongodb_version);
	return 0;
}

#ifdef PIC
DllExport int nsplib_init(nsp_state *N)
{
	nspmongodb_register_all(N);
	return 0;
}
#endif

#endif /* HAVE_MONGODB */
