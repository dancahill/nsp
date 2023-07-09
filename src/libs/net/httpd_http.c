/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2023 Dan Cahill

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

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

static void conn_murder(nsp_state *N, obj_t *cobj)
{
#define __FN__ __FILE__ ":conn_murder()"
	//TCP_SOCKET *sock;

	n_warn(N, __FN__, "reaper is claiming another lost soul");
	if (cobj->val->type != NT_CDATA || cobj->val->d.str == NULL || strcmp(cobj->val->d.str, "httpconn") != 0)
		n_error(N, NE_SYNTAX, __FN__, "expected a httpconn");
	cobj->val->d.str = NULL;
	cobj->val->size = 0;
	return;
#undef __FN__
}

void httpd_read_postdata(CONN *conn)
{
	return;
}

int httpd_read_vardata(CONN *conn)
{
	return 0;
}

int httpd_read_header(CONN *conn)
{
#define __FN__ __FILE__ ":read_header()"
	char line[2048];
	obj_t *cobj;//, *tobj;
	obj_t *hcobj, *hrobj, *hsobj;
	char varname[64];

	memset(line, 0, sizeof(line));

	conn->N = nsp_newstate();
	htnsp_initenv(conn);

	if (tcp_fgets(conn->N, &conn->socket, line, sizeof(line) - 1) < 0) return -1;
	striprn(line);








	//if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	//if ((sock = n_alloc(N, sizeof(TCP_SOCKET) + 1, 1)) == NULL) {
	//	n_warn(N, __FN__, "couldn't alloc %d bytes", sizeof(TCP_SOCKET) + 1);
	//	return -1;
	//}
	nc_strncpy(conn->obj_type, "httpconn", sizeof(conn->obj_type) - 1);
	conn->obj_term = (NSP_CFREE)conn_murder;
	cobj = nsp_setcdata(conn->N, &conn->N->g, "CONN", NULL, 0);
	cobj->val->d.str = (void *)conn;
	cobj->val->size = sizeof(CONN) + 1;






	//httpd_prints(conn, "[[%s]]<br/>", line);

	conn->N->warnformat = 'h';
	hcobj = nsp_settable(conn->N, &conn->N->g, "_COOKIE");
	hcobj->val->attr |= NST_AUTOSORT;
	hrobj = nsp_settable(conn->N, &conn->N->g, "_HEADER");
	hrobj->val->attr |= NST_AUTOSORT;
	hsobj = nsp_settable(conn->N, &conn->N->g, "_SERVER");
	hsobj->val->attr |= NST_AUTOSORT;
	nsp_setstr(conn->N, hsobj, "CONTENT_TYPE", "application/x-www-form-urlencoded", -1);
	nsp_setstr(conn->N, hsobj, "GATEWAY_INTERFACE", "CGI/1.1", -1);
	nsp_setstr(conn->N, hsobj, "REMOTE_ADDR", conn->socket.RemoteAddr, -1);
	nsp_setnum(conn->N, hsobj, "REMOTE_PORT", conn->socket.RemotePort);
	nsp_setstr(conn->N, hsobj, "REMOTE_USER", "nobody", -1);
	nsp_setstr(conn->N, hsobj, "SERVER_ADDR", conn->socket.LocalAddr, -1);
	nsp_setnum(conn->N, hsobj, "SERVER_PORT", conn->socket.LocalPort);
	snprintf(varname, sizeof(varname) - 1, "%s %s", SERVER_NAME, PACKAGE_VERSION);
	nsp_setstr(conn->N, hsobj, "SERVER_SOFTWARE", varname, -1);
	nsp_setstr(conn->N, hsobj, "NSP_VERSION", NSP_VERSION, -1);
	conn->state = 1;

	while (strlen(line) > 0) {
		if (tcp_fgets(conn->N, &conn->socket, line, sizeof(line) - 1) < 0) return -1;
		striprn(line);
		if (!strlen(line)) break;
		//n_warn(conn->N, __FN__, "[%s]", line);
		//httpd_prints(conn, "[%s]<br/>", line);
	}
	return 0;
#undef __FN__
}

void httpd_send_header(CONN *conn, int cacheable, int status, char *extra_header, char *mime_type, int length, time_t mod)
{
	obj_t *hrobj = nsp_getobj(conn->N, &conn->N->g, "_HEADER");
	char timebuf[100];
	time_t now;

	if (status) {
		conn->dat->out_status = status;
	} else if ((status = (int)nsp_getnum(conn->N, hrobj, "STATUS")) != 0) {
		conn->dat->out_status = status;
	} else {
		conn->dat->out_status = 200;
	}
	if (length >= 0) {
		conn->dat->out_ContentLength = length;
	}
	if (mod != (time_t)-1) {
		strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&mod));
		nsp_setstr(conn->N, hrobj, "LAST_MODIFIED", timebuf, strlen(timebuf));
	}
	now = time(NULL);
	strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
	nsp_setstr(conn->N, hrobj, "DATE", timebuf, strlen(timebuf));
	if (cacheable) {
		nsp_setstr(conn->N, hrobj, "CACHE_CONTROL", "public", -1);
		nsp_setstr(conn->N, hrobj, "PRAGMA", "public", -1);
	} else {
		nsp_setstr(conn->N, hrobj, "CACHE_CONTROL", "no-cache, no-store, must-revalidate", -1);
		nsp_setstr(conn->N, hrobj, "EXPIRES", timebuf, -1);
		nsp_setstr(conn->N, hrobj, "PRAGMA", "no-cache", -1);
	}
	if (extra_header != (char*)0) {
		nsp_setstr(conn->N, hrobj, "CONTENT_TYPE", mime_type, -1);
	} else {
		nsp_setstr(conn->N, hrobj, "CONTENT_TYPE", "text/html", -1);
	}
}
