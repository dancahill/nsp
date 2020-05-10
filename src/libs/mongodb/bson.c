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

void nsptobson(nsp_state *N, obj_t *tobj, bson_t *command)
{
#define __FN__ __FILE__ ":nsptobson()"
	obj_t *cobj;

	for (cobj = tobj->val->d.table.f; cobj; cobj = cobj->next) {
		switch (cobj->val->type) {
		case NT_BOOLEAN:
			BSON_APPEND_BOOL(command, cobj->name, nsp_tobool(N, cobj) ? true : false);
			break;
		case NT_NUMBER:
			BSON_APPEND_INT32(command, cobj->name, nsp_tonum(N, cobj));
			break;
		case NT_STRING:
			BSON_APPEND_UTF8(command, cobj->name, nsp_tostr(N, cobj));
			break;
		case NT_NFUNC:
			n_warn(N, __FN__, "nsptobson NT_NFUNC");
			break;
		case NT_CFUNC:
			n_warn(N, __FN__, "nsptobson NT_CFUNC");
			break;
		case NT_TABLE: {
			bson_t child;

			bson_append_document_begin(command, cobj->name, -1, &child);
			nsptobson(N, cobj, &child);
			bson_append_document_end(command, &child);
			break;
		}
		case NT_CDATA:
			n_warn(N, __FN__, "nsptobson NT_CDATA");
			break;
		}
	}
	//char *string = bson_as_json(command, NULL);
	//printf("nsptobson output = [%s];\r\n", string);
	//bson_free(string);
#undef __FN__
}

void bsontonsp(nsp_state *N, bson_iter_t *iter, obj_t *tobj)
{
	while (bson_iter_next(iter)) {
		//printf("bsontonsp() Found element key: \"%s\", type = %d\n", bson_iter_key(iter), bson_iter_type(iter));
		/*
		typedef enum {
		   BSON_TYPE_EOD = 0x00,
		   BSON_TYPE_DOUBLE = 0x01,
		   BSON_TYPE_UTF8 = 0x02,
		   BSON_TYPE_DOCUMENT = 0x03,
		   BSON_TYPE_ARRAY = 0x04,
		   BSON_TYPE_BINARY = 0x05,
		   BSON_TYPE_UNDEFINED = 0x06,
		   BSON_TYPE_OID = 0x07,
		   BSON_TYPE_BOOL = 0x08,
		   BSON_TYPE_DATE_TIME = 0x09,
		   BSON_TYPE_NULL = 0x0A,
		   BSON_TYPE_REGEX = 0x0B,
		   BSON_TYPE_DBPOINTER = 0x0C,
		   BSON_TYPE_CODE = 0x0D,
		   BSON_TYPE_SYMBOL = 0x0E,
		   BSON_TYPE_CODEWSCOPE = 0x0F,
		   BSON_TYPE_INT32 = 0x10,
		   BSON_TYPE_TIMESTAMP = 0x11,
		   BSON_TYPE_INT64 = 0x12,
		   BSON_TYPE_MAXKEY = 0x7F,
		   BSON_TYPE_MINKEY = 0xFF,
		} bson_type_t;
		*/
		switch (bson_iter_type(iter)) {
		case BSON_TYPE_DOUBLE:
			nsp_setnum(N, tobj, (char *)bson_iter_key(iter), bson_iter_double(iter));
			break;
		case BSON_TYPE_UTF8:
			nsp_setstr(N, tobj, (char *)bson_iter_key(iter), (char *)bson_iter_utf8(iter, NULL), -1);
			break;
		case BSON_TYPE_DOCUMENT: {
			bson_iter_t child;

			if (bson_iter_recurse(iter, &child)) {
				obj_t *cobj = nsp_settable(N, tobj, (char *)bson_iter_key(iter));
				bsontonsp(N, &child, cobj);
			}
			break;
		}
		case BSON_TYPE_OID: {
			char str[25];

			bson_oid_to_string(bson_iter_oid(iter), str);
			nsp_setstr(N, tobj, (char *)bson_iter_key(iter), str, -1);
			break;
		}
		case BSON_TYPE_BOOL:
			nsp_setbool(N, tobj, (char *)bson_iter_key(iter), bson_iter_bool(iter));
			break;
		case BSON_TYPE_DATE_TIME:
			printf("bsontonsp() don't know how to process dates yet\n");
			break;
		case BSON_TYPE_INT32:
			nsp_setnum(N, tobj, (char *)bson_iter_key(iter), bson_iter_int32(iter));
			break;
		case BSON_TYPE_INT64:
			nsp_setnum(N, tobj, (char *)bson_iter_key(iter), bson_iter_int64(iter));
			break;
		default:
			n_warn(N, "bsontonsp()", "\"%s\" unknown type: %d\n", bson_iter_key(iter), bson_iter_type(iter));
		}
	}
}

bson_t *paramtobson(nsp_state *N, obj_t *cobj)
{
#define __FN__ __FILE__ ":paramtobson()"
	bson_t *command;
	bson_error_t error;

	if (nsp_istable(cobj)) {
		command = bson_new();
		nsptobson(N, cobj, command);
	} else {
		n_expect_argtype(N, NT_STRING, 1, cobj, 0);
		command = bson_new_from_json((const uint8_t *)cobj->val->d.str, -1, &error);
		if (command == NULL) n_error(N, NE_SYNTAX, __FN__, "%s", error.message);
	}
	return command;
#undef __FN__
}

void bsontoret(nsp_state *N, bson_t *command)
{
	obj_t tobj;
	bson_iter_t iter;

	//char *string = bson_as_json(command, NULL);
	//printf("bsontoret() [%s]\r\n", string);
	//bson_free(string);
	if (bson_iter_init(&iter, command)) {
		nc_memset((void *)& tobj, 0, sizeof(obj_t));
		tobj.val = n_newval(N, NT_TABLE);
		tobj.val->attr &= ~NST_AUTOSORT;
		bsontonsp(N, &iter, &tobj);
		nsp_linkval(N, &N->r, &tobj);
		nsp_unlinkval(N, &tobj);
	}
}

#endif
