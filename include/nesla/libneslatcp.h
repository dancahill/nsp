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
#  pragma comment(lib, "ws2_32.lib")
#  ifdef __CYGWIN__
#    include <ws2tcpip.h>
#  endif
#endif
/* typedef int(*NES_CFUNC)(nes_state *); */

#include "sslstub.h"

typedef struct {
	short int socket;
	SSL *ssl;
	char LocalAddr[16];
	int  LocalPort;
	char RemoteAddr[16];
	int  RemotePort;
	time_t ctime; /* Creation time */
	time_t mtime; /* Last Modified time */
	unsigned int bytes_in;
	unsigned int bytes_out;
	short int want_close;
	/* TCP INPUT BUFFER */
	short int recvbufsize;
	short int recvbufoffset;
	char      recvbuf[2048];
} TCP_SOCKET;

/* http.c */
int neslatcp_http_get(nes_state *N);

int neslatcp_register_all(nes_state *N);

/* ssl.c functions */
#ifdef HAVE_SSL
int     ssl_init(nes_state *N);
int     ssl_accept(nes_state *N, TCP_SOCKET *sock);
int     ssl_connect(nes_state *N, TCP_SOCKET *sock);
int     ssl_read(nes_state *N, SSL *ssl, void *buf, int len);
int     ssl_write(nes_state *N, SSL *ssl, const void *buf, int len);
int     ssl_close(nes_state *N, TCP_SOCKET *sock);
int     ssl_shutdown(nes_state *N);
#endif
/* tcp.c functions */
int     tcp_bind(nes_state *N, char *ifname, unsigned short port);
int     tcp_accept(nes_state *N, int listensock, TCP_SOCKET *sock);
int     tcp_connect(nes_state *N, TCP_SOCKET *socket, char *host, unsigned short port, short int use_ssl);
int     tcp_fgets(nes_state *N, char *buffer, int max, TCP_SOCKET *socket);
int     tcp_fprintf(nes_state *N, TCP_SOCKET *socket, const char *format, ...);
int     tcp_recv(nes_state *N, TCP_SOCKET *socket, char *buffer, int len, int flags);
int     tcp_send(nes_state *N, TCP_SOCKET *socket, const char *buffer, int len, int flags);
int     tcp_close(nes_state *N, TCP_SOCKET *socket, short int owner_killed);
