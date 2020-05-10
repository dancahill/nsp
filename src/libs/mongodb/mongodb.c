/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2020 Dan Cahill

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
#include "mongodb.h"

#ifdef HAVE_MONGODB

void mongodb_murder(nsp_state *N, obj_t *cobj);

typedef struct MONGODB_CONN {
	/* standard header info for CDATA object */
	char      obj_type[16]; /* tell us all about yourself in 15 characters or less */
	NSP_CFREE obj_term;     /* now tell us how to kill you */
	/* now begin the stuff that's socket-specific */
	mongoc_client_t *client;
	mongoc_database_t *database;
	mongoc_collection_t *collection;
	mongoc_cursor_t *cursor;
} MONGODB_CONN;

static MONGODB_CONN *getconn(nsp_state *N, obj_t *thisobj)
{
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
	if (conn->cursor) {
		mongoc_cursor_destroy(conn->cursor);
		conn->cursor = NULL;
	}
	if (conn->collection) {
		mongoc_collection_destroy(conn->collection);
		conn->collection = NULL;
	}
	if (conn->database) {
		mongoc_database_destroy(conn->database);
		conn->database = NULL;
	}
	if (conn->client) {
		mongoc_client_destroy(conn->client);
		conn->client = NULL;
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
	if (!conn->client) n_error(N, NE_SYNTAX, __FN__, "mongoc_client_new() returned NULL");
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
	} else if (nsp_isstr((cobj = nsp_getobj(N, nsp_getobj(N, thisobj, "db"), "name")))) {
		db = cobj->val->d.str;
	} else {
		n_error(N, NE_SYNTAX, __FN__, "expected a string for db");
	}
	if (nsp_isstr((cobj = nsp_getobj(N, &N->context->l, "3")))) {
		coll = cobj->val->d.str;
	} else if (nsp_isstr((cobj = nsp_getobj(N, nsp_getobj(N, thisobj, "collection"), "name")))) {
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
	obj_t *cobj, *tobj;
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

	tobj = nsp_getobj(N, thisobj, "db");
	if (nsp_istable(tobj)) {
		//printf("found 'db' in 'this', freeing\r\n");
		nsp_unlinkval(N, tobj);
	}
	tobj = nsp_getobj(N, thisobj, "collection");
	if (nsp_istable(tobj)) {
		//printf("found 'collection' in 'this', freeing\r\n");
		nsp_unlinkval(N, tobj);
	}

	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_db_use)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_db_use()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *clientobj = nsp_getobj(N, thisobj, "client");
	obj_t *cobj1, *cobj2;
	MONGODB_CONN *conn = getconn(N, clientobj);

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj1 = nsp_getobj(N, &N->context->l, "1");
	n_expect_argtype(N, NT_STRING, 1, cobj1, 0);
	cobj2 = nsp_getobj(N, &N->context->l, "2");
	n_expect_argtype(N, NT_STRING, 2, cobj2, 0);
	// close existing handles before changing db
	if (conn->cursor) {
		mongoc_cursor_destroy(conn->cursor);
		conn->cursor = NULL;
	}
	if (conn->database && conn->collection) {
		const char *olddb = mongoc_database_get_name(conn->database);
		const char *oldcoll = mongoc_collection_get_name(conn->collection);

		if (strcmp(nsp_tostr(N, cobj1), olddb) != 0 || strcmp(nsp_tostr(N, cobj2), oldcoll) != 0) {
			mongoc_collection_destroy(conn->collection);
			conn->collection = NULL;
		}
		if (strcmp(nsp_tostr(N, cobj1), olddb) != 0) {
			mongoc_database_destroy(conn->database);
			conn->database = NULL;
		}
	}
	if (conn->database == NULL) {
		nsp_setstr(N, nsp_getobj(N, clientobj, "db"), "name", nsp_tostr(N, cobj1), -1);
		conn->database = mongoc_client_get_database(conn->client, nsp_tostr(N, cobj1));
	}
	if (conn->collection == NULL) {
		nsp_setstr(N, nsp_getobj(N, clientobj, "collection"), "name", nsp_tostr(N, cobj2), -1);
		conn->collection = mongoc_client_get_collection(conn->client, nsp_tostr(N, cobj1), nsp_tostr(N, cobj2));
	}
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_clientcommand)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_clientcommand()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	MONGODB_CONN *conn = getconn(N, thisobj);
	bson_t *command;
	bson_t reply;
	bool rc; /* bool defined in bson-compat.h */
	bson_error_t error;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	command = paramtobson(N, nsp_getobj(N, &N->context->l, "1"));
	if (command == NULL) n_error(N, NE_SYNTAX, __FN__, "error parsing command");
	rc = mongoc_client_command_simple(conn->client, "admin", command, NULL, &reply, &error);
	if (rc) {
		//char *string = bson_as_json(&reply, NULL);
		//nsp_setstr(N, &N->r, "", string, -1);
		//printf("string = [%s]\r\n", string);
		//bson_free(string);
		bsontoret(N, &reply);
	}
	bson_destroy(command);
	bson_destroy(&reply);
	if (!rc) n_error(N, NE_SYNTAX, __FN__, "%s", error.message);
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_collection_command)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_collection_command()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *clientobj = nsp_getobj(N, thisobj, "client");
	MONGODB_CONN *conn = getconn(N, clientobj);
	bson_t *command;
	bson_t reply;
	bool rc;
	bson_error_t error;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	command = paramtobson(N, nsp_getobj(N, &N->context->l, "1"));
	if (command == NULL) n_error(N, NE_SYNTAX, __FN__, "error parsing command");
	rc = mongoc_collection_command_simple(conn->collection, command, NULL, &reply, &error);
	if (rc) {
		//char *string = bson_as_json(&reply, NULL);
		//nsp_setstr(N, &N->r, "", string, -1);
		//printf("collectioncommand = [%s]\r\n", string);
		//bson_free(string);
		bsontoret(N, &reply);
	}
	bson_destroy(command);
	bson_destroy(&reply);
	if (!rc) n_error(N, NE_SYNTAX, __FN__, "%s", error.message);
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_collection_newoid)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_collection_newoid()"
	char str[25];
	bson_oid_t oid;

	bson_oid_init(&oid, NULL);
	bson_oid_to_string(&oid, str);
	nsp_setstr(N, &N->r, "", str, -1);
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_collection_insert)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_collection_insert()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *clientobj = nsp_getobj(N, thisobj, "client");
	MONGODB_CONN *conn = getconn(N, clientobj);
	bson_t *command;
	bool rc;
	bson_error_t error;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	command = paramtobson(N, nsp_getobj(N, &N->context->l, "1"));
	if (command == NULL) n_error(N, NE_SYNTAX, __FN__, "error parsing command");
	// need to use mongoc_collection_insert_one to get result data, but it
	// doesn't exist in the standard include files that come with CentOS 7
	rc = mongoc_collection_insert(conn->collection, MONGOC_INSERT_NONE, command, NULL, &error);
	if (rc) { }
	bson_destroy(command);
	nsp_setbool(N, &N->r, "", rc);
	if (!rc) n_error(N, NE_SYNTAX, __FN__, "%s", error.message);
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_collection_update)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_collection_update()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *clientobj = nsp_getobj(N, thisobj, "client");
	MONGODB_CONN *conn = getconn(N, clientobj);
	bson_t *selector;
	bson_t *update;
	bool rc;
	bson_error_t error;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	selector = paramtobson(N, nsp_getobj(N, &N->context->l, "1"));
	if (selector == NULL) n_error(N, NE_SYNTAX, __FN__, "error parsing command");
	update = paramtobson(N, nsp_getobj(N, &N->context->l, "2"));
	if (update == NULL) {
		bson_destroy(selector);
		n_error(N, NE_SYNTAX, __FN__, "error parsing command");
	}
	//MONGOC_UPDATE_MULTI_UPDATE
	rc = mongoc_collection_update(conn->collection, MONGOC_UPDATE_NONE, selector, update, NULL, &error);
	if (rc) {}
	bson_destroy(selector);
	bson_destroy(update);
	nsp_setbool(N, &N->r, "", rc);
	if (!rc) n_error(N, NE_SYNTAX, __FN__, "%s", error.message);
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_collection_remove)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_collection_remove()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *clientobj = nsp_getobj(N, thisobj, "client");
	MONGODB_CONN *conn = getconn(N, clientobj);
	bson_t *command;
	bool rc;
	bson_error_t error;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	command = paramtobson(N, nsp_getobj(N, &N->context->l, "1"));
	if (command == NULL) n_error(N, NE_SYNTAX, __FN__, "error parsing command");
	rc = mongoc_collection_remove(conn->collection, MONGOC_REMOVE_SINGLE_REMOVE, command, NULL, &error);
	if (rc) { }
	bson_destroy(command);
	nsp_setbool(N, &N->r, "", rc);
	if (!rc) n_error(N, NE_SYNTAX, __FN__, "%s", error.message);
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_collection_find)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_collection_find()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *clientobj = nsp_getobj(N, thisobj, "client");
	MONGODB_CONN *conn = getconn(N, clientobj);
	bson_t *command;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	command = paramtobson(N, nsp_getobj(N, &N->context->l, "1"));
	if (command == NULL) n_error(N, NE_SYNTAX, __FN__, "error parsing command");
	conn->cursor = mongoc_collection_find(conn->collection, MONGOC_QUERY_NONE, 0, 0, 0, command, NULL, NULL);
	nsp_setbool(N, &N->r, "", conn->cursor ? 1 : 0);
	bson_destroy(command);
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_collection_getnext)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_collection_getnext()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *clientobj = nsp_getobj(N, thisobj, "client");
	MONGODB_CONN *conn = getconn(N, clientobj);
	const bson_t *doc;
	bool rc;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	rc = mongoc_cursor_next(conn->cursor, &doc);
	if (rc) {
		char *string = bson_as_json(doc, NULL);
		nsp_setstr(N, &N->r, "", string, -1);
		bson_free(string);
		bsontoret(N, (bson_t *)doc);
	} else {
		if (conn->cursor) {
			mongoc_cursor_destroy(conn->cursor);
			conn->cursor = NULL;
		}
	}
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_data_mongodb_collection_endfind)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_collection_endfind()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *clientobj = nsp_getobj(N, thisobj, "client");
	MONGODB_CONN *conn = getconn(N, clientobj);

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	if (conn->cursor) {
		mongoc_cursor_destroy(conn->cursor);
		conn->cursor=NULL;
	}
	return 0;
#undef __FN__
}

NSP_CLASS(libnsp_data_mongodb_client)
{
#define __FN__ __FILE__ ":libnsp_data_mongodb_client()"
	obj_t *tobj, *cobj;

	nsp_setstr(N, &N->context->l, "url", "mongodb://localhost:27017/", -1);
	nsp_setbool(N, &N->context->l, "connection", 0);
	cobj = nsp_getobj(N, nsp_getobj(N, &N->g, "data"), "mongodb");
	if (nsp_istable(cobj)) nsp_zlink(N, &N->context->l, cobj);
	else n_warn(N, __FN__, "data.mongodb not found");
	// override the db with an empty table then inherit the original and add local properties
	tobj = nsp_settable(N, &N->context->l, "db");
	nsp_zlink(N, tobj, nsp_getobj(N, cobj, "db"));
	// client must be unlinked by calling close() or the script will leak memory
	// need to rethink this approach
	nsp_linkval(N, nsp_setbool(N, tobj, "client", 0), &N->context->l);
	nsp_setstr(N, tobj, "name", "testdb", -1);

	// same for collection as for db
	tobj = nsp_settable(N, &N->context->l, "collection");
	nsp_zlink(N, tobj, nsp_getobj(N, cobj, "collection"));
	nsp_linkval(N, nsp_setbool(N, tobj, "client", 0), &N->context->l);
	nsp_setstr(N, tobj, "name", "test", -1);

	if (nsp_istable((tobj = nsp_getobj(N, &N->context->l, "1")))) {
		if (nsp_isstr((cobj = nsp_getobj(N, tobj, "url")))) {
			nsp_setstr(N, &N->context->l, "url", cobj->val->d.str, cobj->val->size);
		}
		if (nsp_isstr((cobj = nsp_getobj(N, tobj, "database")))) {
			nsp_setstr(N, nsp_getobj(N, &N->context->l, "db"), "name", cobj->val->d.str, cobj->val->size);
		}
		if (nsp_isstr((cobj = nsp_getobj(N, tobj, "collection")))) {
			nsp_setstr(N, nsp_getobj(N, &N->context->l, "collection"), "name", cobj->val->d.str, cobj->val->size);
		}
	}
	return 0;
#undef __FN__
}

int nspmongodb_register_all(nsp_state *N)
{
	obj_t *tobj, *tobj2;

	tobj = nsp_settable(N, &N->g, "data");
	tobj->val->attr |= NST_HIDDEN;
	tobj = nsp_settable(N, tobj, "mongodb");
	tobj->val->attr |= NST_HIDDEN;
	nsp_setcfunc(N, tobj, "client",        (NSP_CFUNC)libnsp_data_mongodb_client);
	nsp_setcfunc(N, tobj, "open",          (NSP_CFUNC)libnsp_data_mongodb_open);
	nsp_setcfunc(N, tobj, "close",         (NSP_CFUNC)libnsp_data_mongodb_close);

	nsp_setcfunc(N, tobj, "clientcommand", (NSP_CFUNC)libnsp_data_mongodb_clientcommand);

	tobj2 = nsp_settable(N, tobj, "db");
	tobj2->val->attr |= NST_HIDDEN;
	nsp_setcfunc(N, tobj2, "use",          (NSP_CFUNC)libnsp_data_mongodb_db_use);

	tobj2 = nsp_settable(N, tobj, "collection");
	tobj2->val->attr |= NST_HIDDEN;
	nsp_setcfunc(N, tobj2, "command",   (NSP_CFUNC)libnsp_data_mongodb_collection_command);
	nsp_setcfunc(N, tobj2, "newoid",    (NSP_CFUNC)libnsp_data_mongodb_collection_newoid);
	nsp_setcfunc(N, tobj2, "insert",    (NSP_CFUNC)libnsp_data_mongodb_collection_insert);
	nsp_setcfunc(N, tobj2, "update",    (NSP_CFUNC)libnsp_data_mongodb_collection_update);
	nsp_setcfunc(N, tobj2, "remove",    (NSP_CFUNC)libnsp_data_mongodb_collection_remove);
	nsp_setcfunc(N, tobj2, "find",      (NSP_CFUNC)libnsp_data_mongodb_collection_find);
	nsp_setcfunc(N, tobj2, "getnext",   (NSP_CFUNC)libnsp_data_mongodb_collection_getnext);
	nsp_setcfunc(N, tobj2, "endfind",   (NSP_CFUNC)libnsp_data_mongodb_collection_endfind);
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
