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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int nc_vsnprintf(nes_state *N, char *dest, int max, const char *format, va_list ap);

/*
 * these functions are _not_ used internally.
 */

#define MAXBUF 8192

obj_t *nes_readtablef(nes_state *N, obj_t *tobj, const char *fmt, ...)
{
	uchar *buf=calloc(MAXBUF, sizeof(char));
	short int jmp=N->jmpset;
	short int oldop;
	uchar *oldptr;
	va_list ap;

	if (buf==NULL) return NULL;
	if (jmp==0) {
		if (setjmp(N->savjmp)==0) {
			N->jmpset=1;
		} else {
			goto cleanup;
		}
	}
	va_start(ap, fmt);
	nc_vsnprintf(N, (char *)buf, MAXBUF, fmt, ap);
	va_end(ap);
	oldptr=N->readptr;
	oldop=N->lastop;
	N->readptr=buf;
	N->lastop=255;
	nes_readtable(N, tobj);
	N->readptr=oldptr;
	N->lastop=oldop;
cleanup:
	if (jmp==0) N->jmpset=0;
	n_free(N, (void *)&buf);
	return tobj;
}

/*
int nes_cwrap(nes_state *N, void *fn, char *arg0, ...)
{
	va_list ap;
	int x;

	va_start(ap, arg0);
	while ((x=va_arg(ap, int))!=0) {
		nes_printf(N, "[%d]\n", x);
	}
	va_end(ap);
	return 0;
}
*/
