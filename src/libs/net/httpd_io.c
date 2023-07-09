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
static char *p_strcasestr(char *src, char *query)
{
	char *pToken;
	char Buffer[8192];
	char Query[64];
	int loop;

	if (strlen(src) == 0) return NULL;
	memset(Buffer, 0, sizeof(Buffer));
	strncpy(Buffer, src, sizeof(Buffer) - 1);
	strncpy(Query, query, sizeof(Query) - 1);
	loop = 0;
	while (Buffer[loop]) {
		Buffer[loop] = toupper(Buffer[loop]);
		loop++;
	}
	loop = 0;
	while (Query[loop]) {
		Query[loop] = toupper(Query[loop]);
		loop++;
	}
	pToken = strstr(Buffer, Query);
	if (pToken != NULL) {
		return src + (pToken - (char *)&Buffer);
	}
	return NULL;
}

char *httpd_getbuffer(CONN *conn)
{
	conn->dat->lastbuf++;
	if (conn->dat->lastbuf > 3) conn->dat->lastbuf = 0;
	memset(conn->dat->smallbuf[conn->dat->lastbuf], 0, sizeof(conn->dat->smallbuf[conn->dat->lastbuf]));
	return conn->dat->smallbuf[conn->dat->lastbuf];
}

void httpd_flushheader(CONN *conn)
{
	obj_t *htobj, *hrobj, *cobj;
	char *Protocol;
	char line[256];
	char tmpnam[64];
	char *p;
	short int ctype = 0;

	if (conn->N == NULL) goto err;
	if (conn->dat->out_headdone) return;
	htobj = nsp_getobj(conn->N, &conn->N->g, "_SERVER");
	hrobj = nsp_getobj(conn->N, &conn->N->g, "_HEADER");
	if ((htobj->val->type != NT_TABLE) || (hrobj->val->type != NT_TABLE)) goto err;
	if (!conn->dat->out_status) {
		conn->dat->out_headdone = 1;
		return;
	}
	Protocol = nsp_getstr(conn->N, htobj, "SERVER_PROTOCOL");
	if ((conn->dat->out_bodydone) && (!conn->dat->out_flushed)) {
		conn->dat->out_ContentLength = conn->dat->out_bytecount;
	}
	if ((conn->dat->out_status < 200) || (conn->dat->out_status >= 400)) {
		nsp_setstr(conn->N, hrobj, "CONNECTION", "Close", strlen("Close"));
	} else if ((strcasecmp(nsp_getstr(conn->N, htobj, "HTTP_CONNECTION"), "Keep-Alive") == 0) && (conn->dat->out_bodydone)) {
		nsp_setstr(conn->N, hrobj, "CONNECTION", "Keep-Alive", strlen("Keep-Alive"));
	} else if ((strlen(nsp_getstr(conn->N, htobj, "HTTP_CONNECTION")) == 0) && (conn->dat->out_bodydone) && (strcasecmp(Protocol, "HTTP/1.1") == 0)) {
		nsp_setstr(conn->N, hrobj, "CONNECTION", "Keep-Alive", strlen("Keep-Alive"));
	} else {
		nsp_setstr(conn->N, hrobj, "CONNECTION", "Close", strlen("Close"));
	}
	if (p_strcasestr(Protocol, "HTTP/1.1") != NULL) {
		nsp_setstr(conn->N, hrobj, "PROTOCOL", "HTTP/1.1", strlen("HTTP/1.1"));
	} else {
		nsp_setstr(conn->N, hrobj, "PROTOCOL", "HTTP/1.0", strlen("HTTP/1.0"));
	}
	memset(line, 0, sizeof(line));
	if (tcp_fprintf(conn->N, &conn->socket, "%s %d OK\r\n", nsp_getstr(conn->N, hrobj, "PROTOCOL"), conn->dat->out_status) < 0) goto err;
	/* log_error(proc->N, MODSHORTNAME "responses", __FILE__, __LINE__, 1, "%s:%d %s %d OK\r\n", conn->socket.RemoteAddr, conn->socket.RemotePort, nsp_getstr(conn->N, hrobj, "PROTOCOL"), conn->dat->out_status); */
	for (cobj = hrobj->val->d.table.f; cobj; cobj = cobj->next) {
		if ((cobj->val->type != NT_STRING) && (cobj->val->type != NT_NUMBER)) continue;
		strncpy(tmpnam, cobj->name, sizeof(tmpnam) - 1);
		if (strcmp(tmpnam, "CONTENT_TYPE") == 0) ctype = 1;
		for (p = tmpnam; p[0]; p++) {
			if (p[0] == '_') {
				p[0] = '-';
				if (isalpha(p[1])) { p++; continue; }
			} else {
				*p = tolower(*p);
			}
		}
		tmpnam[0] = toupper(tmpnam[0]);
		if (cobj->val->type == NT_STRING) {
			if (tcp_fprintf(conn->N, &conn->socket, "%s: %s\r\n", tmpnam, cobj->val->d.str ? cobj->val->d.str : "") < 0) goto err;
		} else {
			if (tcp_fprintf(conn->N, &conn->socket, "%s: %s\r\n", tmpnam, nsp_tostr(conn->N, cobj)) < 0) goto err;
		}
		/* log_error(proc->N, MODSHORTNAME "responses", __FILE__, __LINE__, 1, "[%s][%s]", tmpnam, nsp_tostr(conn->N, cobj)); */
	}
	if (tcp_fprintf(conn->N, &conn->socket, "Server: %s %s\r\n", SERVER_NAME, PACKAGE_VERSION) < 0) goto err;
	if (tcp_fprintf(conn->N, &conn->socket, "Status: %d\r\n", conn->dat->out_status) < 0) goto err;
	if (conn->dat->out_bodydone) {
		if (tcp_fprintf(conn->N, &conn->socket, "Content-Length: %d\r\n", conn->dat->out_ContentLength) < 0) goto err;
	}
	if (!ctype) {
		if (tcp_fprintf(conn->N, &conn->socket, "Content-Type: text/plain\r\n") < 0) goto err;
	}
	if (tcp_fprintf(conn->N, &conn->socket, "\r\n") < 0) goto err;
	conn->dat->out_headdone = 1;
	return;
err:
	tcp_close(conn->N, &conn->socket, 1);
	conn->dat->out_headdone = 1;
	return;
}

void httpd_flushbuffer(CONN *conn)
{
	char *pTemp = conn->dat->replybuf;
	signed long str_len;
	signed long dcount;

	httpd_flushheader(conn);
	str_len = conn->dat->replybuflen;
	if (str_len < 1) return;
	conn->dat->out_flushed = 1;
	while (str_len > 0) {
		dcount = 4096;
		if (str_len < dcount) dcount = str_len;
		dcount = tcp_send(conn->N, &conn->socket, pTemp, dcount, 0);
		if (dcount < 0) break;
		str_len -= dcount;
		pTemp += dcount;
	}
	memset(conn->dat->replybuf, 0, sizeof(conn->dat->replybuf));
	conn->dat->replybuflen = 0;
	return;
}

int httpd_prints(CONN *conn, const char *format, ...)
{
	char *buffer = httpd_getbuffer(conn);
	unsigned long str_len1, str_len2;
	va_list ap;

	if (conn == NULL) return -1;
	conn->socket.mtime = time(NULL);
	va_start(ap, format);
	vsnprintf(buffer, sizeof(conn->dat->smallbuf[0]) - 1, format, ap);
	str_len1 = strlen(buffer);
	str_len2 = conn->dat->replybuflen;
	va_end(ap);
	buffer[sizeof(conn->dat->smallbuf[0]) - 1] = '\0';
	conn->dat->out_bytecount += str_len1;
	if (str_len2 + sizeof(conn->dat->smallbuf[0]) > MAX_REPLYSIZE - 2) {
		httpd_flushbuffer(conn);
		str_len2 = 0;
	}
	strcpy(conn->dat->replybuf + str_len2, buffer);
	conn->dat->replybuflen = str_len2 += str_len1;
	if (str_len2 + sizeof(conn->dat->smallbuf[0]) > MAX_REPLYSIZE - 2) {
		httpd_flushbuffer(conn);
	}
	return str_len1;
}
