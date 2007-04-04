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
#include "libneslatcp.h"

#ifdef WIN32
#else
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#define closesocket close
#endif
#include <string.h>

/* my crappiest tcp client ever..  i'd beer better if i had more program to drink. */

static int http_connect(nes_state *N, char *host, unsigned short port)
{
	struct hostent *hp;
	struct sockaddr_in server;
	int sock=0;

	if ((hp=gethostbyname(host))==NULL) {
		n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "Host lookup error for %s", host);
		return -1;
	}
	memset((char *)&server, 0, sizeof(server));
	memmove((char *)&server.sin_addr, hp->h_addr, hp->h_length);
	server.sin_family=hp->h_addrtype;
	server.sin_port=htons(port);
	if ((sock=socket(AF_INET, SOCK_STREAM, 0))<0) return -1;
	if (connect(sock, (struct sockaddr *)&server, sizeof(server))<0) {
		n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "Connect error for %s:%d", host, port);
		return -1;
	}
	return sock;
}

int neslatcp_http_get(nes_state *N)
{
#ifdef HAVE_SSL
	SSL_METHOD *meth;
	X509 *server_cert;
#endif
	obj_t *cobj1=nes_getiobj(N, &N->l, 1); /* SSL */
	obj_t *cobj2=nes_getiobj(N, &N->l, 2); /* host */
	obj_t *cobj3=nes_getiobj(N, &N->l, 3); /* port*/
	obj_t *cobj4=nes_getiobj(N, &N->l, 4); /* uri */
	char tmpbuf[8192];
	int sock;
	int len;
	int rc;

	if (cobj1->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number for arg1");
	if (cobj2->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	if (cobj3->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number for arg3");
	if (cobj4->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg4");
#ifdef HAVE_SSL
	SSLeay_add_ssl_algorithms();
	meth=SSLv2_client_method();
	SSL_load_error_strings();
	conn[sid].ctx=SSL_CTX_new(meth);
	if ((sock=http_connect(N, cobj2->d.str, (int)cobj3->d.num))<0) {
		return -1;
	}
	sid->ssl=SSL_new(sid->ctx);
	SSL_set_fd(sid->ssl, sid->socket);
	if (SSL_connect(sid->ssl)==-1) { perror("socket: "); exit -1; }
	/* the rest is optional */
	if ((server_cert=SSL_get_peer_certificate(sid->ssl))==NULL) exit-1;
	X509_free(server_cert);
#else
	if ((sock=http_connect(N, cobj2->d.str, (unsigned short)cobj3->d.num))<0) {
		return -1;
	}
#endif

	len=nc_snprintf(N, tmpbuf, sizeof(tmpbuf), "GET %s HTTP/1.1\r\n" \
		"User-Agent: Nesla_httpcli/" NESLA_VERSION "\r\n" \
		"Connection: Keep-Alive\r\n" \
		"Host: %s\r\n" \
		"Accept: */*\r\n" \
		"\r\n", cobj4->d.str, cobj2->d.str
	);
#ifdef HAVE_SSL
	SSL_write(sid->ssl, tmpbuf, len);
#else
	send(sock, tmpbuf, len, 0);
#endif

#ifdef HAVE_SSL
	rc=SSL_read(sid->ssl, tmpbuf, sizeof(tmpbuf)-1);
#else
	rc=recv(sock, tmpbuf, sizeof(tmpbuf)-1, 0);
#endif

/*
//		wmprintf(&conn[sid], "%s", postbuf);
//		printf("\r\n\r\n");
//		for (;;) {
//			rc=wmfgets(&conn[sid], inbuffer, sizeof(inbuffer)-1, conn[sid].socket);
//			if (rc<0) { printf("x"); break; }
//			if (strncmp(inbuffer, "Content-Length: ", 16)==0) { insize=atoi(inbuffer+16); }
//			if (strlen(inbuffer)<3) break;
//			striprn(inbuffer);
//			printf("%s\r\n", inbuffer);
//		}
//		printf("%s\r\n", inbuffer);
//		for (;;) {
//			rc=wmfgets(&conn[sid], inbuffer, sizeof(inbuffer)-1, conn[sid].socket);
//			printf("pid=%d rc=%d insize=%d\r\n", getpid(), rc, insize);
//			if (rc<0) { printf("x"); break; }
//			insize-=rc;
//			if (insize<1) break;

//			if (wmfgets(&conn[sid], inbuffer, sizeof(inbuffer)-1, conn[sid].socket)<0) break;
//			if (!strlen(inbuffer)) break;
//			printf("%s\r\n", inbuffer);
//		}
//		printf(".");
//	}
*/

#ifdef HAVE_SSL
	SSL_shutdown(conn[sid].ssl);
	closesocket(sock);
	SSL_free(conn[sid].ssl);
	SSL_CTX_free(conn[sid].ctx);
#else
	closesocket(sock);
#endif
	nes_setstr(N, &N->r, "", tmpbuf, rc);
	return 0;
}
