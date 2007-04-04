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
#ifdef WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
/* typedef int(*NES_CFUNC)(nes_state *); */

#include "ssl.h"

typedef struct {
	short int socket;
	SSL *ssl;
	char RemoteAddr[16];
	int  RemotePort;
	char ServerAddr[16];
	int  ServerPort;
	time_t ctime; /* Creation time */
	time_t atime; /* Last Access time */
	unsigned int bytes_in;
	unsigned int bytes_out;
	short int want_close;
	/* TCP INPUT BUFFER */
	short int recvbufsize;
	short int recvbufoffset;
	char      recvbuf[4096];
} TCP_SOCKET;

/* http.c */
int neslatcp_http_get(nes_state *N);

int neslatcp_register_all(nes_state *N);

/* ssl.c functions */
#ifdef HAVE_SSL
int     ssl_init();
int     ssl_accept(TCP_SOCKET *sock);
int     ssl_connect(TCP_SOCKET *sock);
int     ssl_read(SSL *ssl, void *buf, int len);
int     ssl_write(SSL *ssl, const void *buf, int len);
int     ssl_close(TCP_SOCKET *sock);
int     ssl_shutdown();
#endif
/* tcp.c functions */
int     tcp_bind(char *ifname, unsigned short port);
int     tcp_accept(int listensock, TCP_SOCKET *sock);
int     tcp_connect(TCP_SOCKET *socket, char *host, unsigned short port, short int use_ssl);
int     tcp_fgets(char *buffer, int max, TCP_SOCKET *socket);
/*int     tcp_fprintf(TCP_SOCKET *socket, const char *format, ...);*/
int     tcp_recv(TCP_SOCKET *socket, char *buffer, int len, int flags);
int     tcp_send(TCP_SOCKET *socket, const char *buffer, int len, int flags);
int     tcp_close(TCP_SOCKET *socket, short int owner_killed);
/*
#define tcp_fprintf(S,f,...) \
{ \
	char *buffer=calloc(2048, sizeof(char))); \
	if ((buffer==NULL) { \
		rc=-1; \
	} else { \
		nc_snprintf(N, buffer, 2048, f, ap); \
		rc=tcp_send(S, buffer, strlen(buffer), 0); \
		free(buffer); \
	} \
}
*/
