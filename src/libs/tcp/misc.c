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
#include "nesla/libtcp.h"

int neslatcp_register_all(nes_state *N)
{
#ifdef WIN32
	static WSADATA wsaData;
#endif
	obj_t *tobj;

#ifdef WIN32
	if (WSAStartup(0x101, &wsaData)) {
		/* printf("Winsock init error\r\n"); */
		return -1;
	}
#endif
	tobj=nes_settable(N, &N->g, "http");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj,  "get", (NES_CFUNC)neslatcp_http_get);

	tobj=nes_settable(N, &N->g, "tcp");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj,  "bind",   (NES_CFUNC)neslatcp_tcp_bind);
	nes_setcfunc(N, tobj,  "accept", (NES_CFUNC)neslatcp_tcp_accept);
	nes_setcfunc(N, tobj,  "open",   (NES_CFUNC)neslatcp_tcp_open);
	nes_setcfunc(N, tobj,  "close",  (NES_CFUNC)neslatcp_tcp_close);
	nes_setcfunc(N, tobj,  "read",   (NES_CFUNC)neslatcp_tcp_read);
	nes_setcfunc(N, tobj,  "gets",   (NES_CFUNC)neslatcp_tcp_gets);
	nes_setcfunc(N, tobj,  "write",  (NES_CFUNC)neslatcp_tcp_write);
	return 0;
}
