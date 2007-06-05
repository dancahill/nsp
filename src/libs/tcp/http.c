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
#include <stdlib.h>
#include <string.h>

/*
 * This client sucks.
 * Since there's no actual useful code here, this makes a decent demo.
 */

static void striprn(char *string)
{
	int i=strlen(string)-1;

	while (i>-1) {
		if ((string[i]!='\r')&&(string[i]!='\n')) break;
		string[i--]='\0';
	}
}

int neslatcp_http_get(nes_state *N)
{
	TCP_SOCKET sock;
	obj_t *cobj1=nes_getiobj(N, &N->l, 1); /* SSL */
	obj_t *cobj2=nes_getiobj(N, &N->l, 2); /* host */
	obj_t *cobj3=nes_getiobj(N, &N->l, 3); /* port*/
	obj_t *cobj4=nes_getiobj(N, &N->l, 4); /* uri */
	obj_t *cobj;
	int cl=-1, len=0, rc;
	char tmpbuf[8192];
	obj_t tobj;

	if (cobj1->val->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	if (cobj3->val->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number for arg3");
	if (cobj4->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg4");
	memset((char *)&sock, 0, sizeof(sock));
	if ((rc=tcp_connect(N, &sock, cobj2->val->d.str, (unsigned short)cobj3->val->d.num, 0))<0) {
		nes_setstr(N, &N->r, "", "tcp error", strlen("tcp error"));
		return -1;
	}
	/* why does php insert random data when i try to use HTTP/1.1? */
	tcp_fprintf(N, &sock, "GET %s HTTP/1.0\r\nUser-Agent: Nesla_httpcli/" NESLA_VERSION "\r\nConnection: Close\r\nHost: %s\r\nAccept: */*\r\n\r\n", cobj4->val->d.str, cobj2->val->d.str);
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	tobj.val=n_newval(N, NT_TABLE);
	rc=tcp_fgets(N, tmpbuf, sizeof(tmpbuf)-1, &sock);
	if (rc>0) {
		cobj=nes_setstr(N, &tobj, "status", tmpbuf, strlen(tmpbuf));
		while ((cobj->val->size>0)&&((cobj->val->d.str[cobj->val->size-1]=='\r')||(cobj->val->d.str[cobj->val->size-1]=='\n'))) cobj->val->d.str[--cobj->val->size]='\0';
	}
	cobj=nes_setstr(N, &tobj, "head", NULL, 0);
	for (;;) {
		rc=tcp_fgets(N, tmpbuf, sizeof(tmpbuf)-1, &sock);
		if (rc<1) { break; }
		/* slow, but at least it's safe */
		n_joinstr(N, cobj, tmpbuf, rc);
		striprn(tmpbuf);
		if (strlen(tmpbuf)<1) break;
		/* printf("[%s]\r\n", tmpbuf); */
		if (strncmp(tmpbuf, "Content-Length: ", 16)==0) { cl=atoi(tmpbuf+16); }
	}
	while ((cobj->val->size>0)&&((cobj->val->d.str[cobj->val->size-1]=='\r')||(cobj->val->d.str[cobj->val->size-1]=='\n'))) cobj->val->d.str[--cobj->val->size]='\0';
	cobj=nes_setstr(N, &tobj, "body", NULL, 0);
	for (;;) {
		rc=tcp_fgets(N, tmpbuf, sizeof(tmpbuf)-1, &sock);
		if (rc<1) { break; }
		len+=rc;
		/* slow, but at least it's safe */
		n_joinstr(N, cobj, tmpbuf, rc);
		striprn(tmpbuf);
		/* printf("{%s}\r\n", tmpbuf); */
		if ((cl>-1)&&(len>=cl)) break;
	}
	tcp_close(N, &sock, 1);
	if (N->debug) n_warn(N, "neslatcp_http_get", "Content-Length: %d/%d", len, cl);
	nes_linkval(N, &N->r, &tobj);
	nes_unlinkval(N, &tobj);
	return 0;
}
