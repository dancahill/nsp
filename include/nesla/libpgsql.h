/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2009 Dan Cahill

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
#ifdef HAVE_PGSQL

#include <libpq-fe.h>

void pgsql_murder(nes_state *N, obj_t *cobj);

typedef struct PGSQL_CONN {
	/* standard header info for CDATA object */
	char      obj_type[16]; /* tell us all about yourself in 15 characters or less */
	NES_CFREE obj_term;     /* now tell us how to kill you */
	/* now begin the stuff that's socket-specific */
	PGconn   *pgconn;
	PGresult *pgres;
} PGSQL_CONN;

int neslapgsql_register_all(nes_state *N);

#endif /* HAVE_PGSQL */
