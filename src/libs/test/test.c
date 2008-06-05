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

static NES_FUNCTION(neslatest_hello)
{
	nes_setstr(N, &N->r, "", "This is a working shared library.", -1);
	return 0;
}

static int neslatest_register_all(nes_state *N)
{
	nes_setcfunc(N, &N->g, "dltest", (NES_CFUNC)neslatest_hello);
	return 0;
}

#ifdef PIC
DllExport int neslalib_init(nes_state *N)
{
	neslatest_register_all(N);
	return 0;
}
#endif
