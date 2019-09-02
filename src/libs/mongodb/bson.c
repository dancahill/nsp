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
			//printf("[%s][%f]\r\n", cobj->name, nsp_tonum(N, cobj));
			BSON_APPEND_INT32(command, cobj->name, nsp_tonum(N, cobj));
			break;
		case NT_STRING:
			//printf("[%s][%s]\r\n", cobj->name, nsp_tostr(N, cobj));
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
	char *string = bson_as_json(command, NULL);
	printf("nsptobson output = [%s];\r\n", string);
	bson_free(string);
#undef __FN__
}

void bsontonsp(nsp_state *N, bson_t *command, obj_t *tobj)
{
//	char *string = bson_as_json(&reply, NULL);
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

#endif
