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
#include "nesla/libext.h"

int neslaext_register_all(nes_state *N)
{
	obj_t *tobj;

	tobj=nes_settable(N, &N->g, "base64");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj,  "decode",  (NES_CFUNC)neslaext_base64_decode);
	nes_setcfunc(N, tobj,  "encode",  (NES_CFUNC)neslaext_base64_encode);

	nes_setcfunc(N, &N->g, "dirlist", (NES_CFUNC)neslaext_dirlist);
	nes_setcfunc(N, &N->g, "rot13",   (NES_CFUNC)neslaext_rot13);

	tobj=nes_settable(N, &N->g, "xml");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj,  "read",    (NES_CFUNC)neslaext_xml_read);
	return 0;
}
