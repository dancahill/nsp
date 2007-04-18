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
#include "libnesla.h"
#include "libneslaext.h"
#include <stdlib.h>

int neslaext_system(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	int n=-1;

	if (cobj1->val->type==NT_STRING) {
		nl_flush(N);
		n=system(cobj1->val->d.str);
	}
	nes_setnum(N, &N->r, "", n);
	return 0;
}
