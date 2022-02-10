/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2022 Dan Cahill

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
#include "net.h"
#include "httpd.h"

int htnsp_flush(nsp_state *N)
{
	CONN *conn;// = get_conn();
	obj_t *cobj;
	int b;
	int o = 0;


	cobj = nsp_getobj(N, &N->g, "CONN");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "httpconn") != 0))
		n_error(N, NE_SYNTAX, "htnsp_flush", "expected a httpconn");
	conn = (CONN *)cobj->val->d.str;

	do {
		conn->socket.mtime = time(NULL);
		b = sizeof(conn->dat->replybuf) - conn->dat->replybuflen - 1;
		if (b < 512) { httpd_flushbuffer(conn); continue; }
		if (b > N->outbuflen) b = N->outbuflen;
		memcpy(conn->dat->replybuf + conn->dat->replybuflen, N->outbuffer + o, b);
		conn->dat->replybuflen += b;
		conn->dat->replybuf[conn->dat->replybuflen] = '\0';
		conn->dat->out_bytecount += b;
		N->outbuflen -= b;
		o += b;
	} while (N->outbuflen);
	if (conn->dat->replybuflen) httpd_flushbuffer(conn);
	return 0;
}

int htnsp_initenv(CONN *conn)
{
	//char libbuf[80];
	obj_t *tobj;//, *tobj2;

	tobj = nsp_settable(conn->N, nsp_settable(conn->N, &conn->N->g, "lib"), "io");
	nsp_setcfunc(conn->N, tobj, "flush", (NSP_CFUNC)htnsp_flush);

	return 0;
}
