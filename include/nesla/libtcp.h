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

#ifdef HAVE_XYSSL
/* uncomment this to prioritize xyssl over openssl */
/* #undef HAVE_OPENSSL */
#endif

#ifdef HAVE_OPENSSL
	#include <openssl/ssl.h>
/*
	#include <openssl/rsa.h>
	#include <openssl/crypto.h>
	#include <openssl/x509.h>
	#include <openssl/pem.h>
	#include <openssl/err.h>
*/
#else
#ifdef HAVE_XYSSL
	#include "xyssl/ssl.h"
	#include "xyssl/havege.h"
#endif
#endif

void tcp_murder(nes_state *N, obj_t *cobj);

typedef struct {
	/* standard header info for CDATA object */
	char      obj_type[16]; /* tell us all about yourself in 15 characters or less */
	NES_CFREE obj_term;     /* now tell us how to kill you */
	/* now begin the stuff that's socket-specific */
	short int socket;
	short use_ssl;
#ifdef HAVE_OPENSSL
	SSL *ssl;
	SSL_CTX *ssl_ctx;
	SSL_METHOD *ssl_meth;
#else
#ifdef HAVE_XYSSL
	havege_state hs;
	ssl_context ssl;
	/* server stuff */
	x509_cert srvcert;
	rsa_context rsa;
	unsigned char session_table[SSL_SESSION_TBL_LEN];
#endif
#endif
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
	char      recvbuf[4096];
} TCP_SOCKET;

/* http.c */
NES_FUNCTION(neslatcp_http_get);

int neslatcp_register_all(nes_state *N);

/* ssl.c functions */
#ifdef HAVE_OPENSSL
int     ossl_init    (nes_state *N, TCP_SOCKET *sock, short srvmode, char *certfile, char *keyfile);
int     ossl_accept  (nes_state *N, TCP_SOCKET *bsock, TCP_SOCKET *asock);
int     ossl_connect (nes_state *N, TCP_SOCKET *sock);
int     ossl_read    (nes_state *N, TCP_SOCKET *sock, void *buf, int max);
int     ossl_write   (nes_state *N, TCP_SOCKET *sock, const void *buf, int len);
int     ossl_close   (nes_state *N, TCP_SOCKET *sock);
int     ossl_shutdown(nes_state *N, TCP_SOCKET *sock);
#else
#ifdef HAVE_XYSSL
int     xyssl_init    (nes_state *N, TCP_SOCKET *sock, short srvmode, char *certfile, char *keyfile);
int     xyssl_accept  (nes_state *N, TCP_SOCKET *bsock, TCP_SOCKET *asock);
int     xyssl_connect (nes_state *N, TCP_SOCKET *sock);
int     xyssl_read    (nes_state *N, TCP_SOCKET *sock, void *buf, int max);
int     xyssl_write   (nes_state *N, TCP_SOCKET *sock, const void *buf, int len);
int     xyssl_close   (nes_state *N, TCP_SOCKET *sock);
int     xyssl_shutdown(nes_state *N, TCP_SOCKET *sock);
#endif /* HAVE_XYSSL */
#endif /* HAVE_OPENSSL */

/* tcp.c functions */
int     tcp_bind   (nes_state *N, char *ifname, unsigned short port);
int     tcp_accept (nes_state *N, TCP_SOCKET *bsock, TCP_SOCKET *asock);
int     tcp_connect(nes_state *N, TCP_SOCKET *socket, char *host, unsigned short port, short int use_ssl);
int     tcp_fgets  (nes_state *N, TCP_SOCKET *socket, char *buffer, int max);
int     tcp_fprintf(nes_state *N, TCP_SOCKET *socket, const char *format, ...);
int     tcp_recv   (nes_state *N, TCP_SOCKET *socket, char *buffer, int max, int flags);
int     tcp_send   (nes_state *N, TCP_SOCKET *socket, const char *buffer, int len, int flags);
int     tcp_close  (nes_state *N, TCP_SOCKET *socket, short int owner_killed);

NES_FUNCTION(neslatcp_tcp_bind);
NES_FUNCTION(neslatcp_tcp_accept);
NES_FUNCTION(neslatcp_tcp_open);
NES_FUNCTION(neslatcp_tcp_close);
NES_FUNCTION(neslatcp_tcp_read);
NES_FUNCTION(neslatcp_tcp_gets);
NES_FUNCTION(neslatcp_tcp_write);
