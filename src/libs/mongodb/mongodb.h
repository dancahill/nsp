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
//#define HAVE_MONGODB 1
#ifdef HAVE_MONGODB

#include <libbson-1.0/bson.h>
#include <libmongoc-1.0/mongoc.h>

int nspmongodb_register_all(nsp_state *N);

void nsptobson(nsp_state *N, obj_t *tobj, bson_t *command);
void bsontonsp(nsp_state *N, bson_t *command, obj_t *tobj);
bson_t *paramtobson(nsp_state *N, obj_t *cobj);

#endif /* HAVE_MONGODB */
