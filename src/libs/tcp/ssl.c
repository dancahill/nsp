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

#ifdef WIN32
#else
#include <unistd.h>
#include <sys/socket.h>
#define closesocket close
#endif

#ifdef HAVE_SSL

int ssl_init(nes_state *N, TCP_SOCKET *sock, short srvmode, char *certfile, char *keyfile)
{
	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
	if (srvmode) {
		sock->ssl_meth=SSLv23_server_method();
	} else {
		sock->ssl_meth=SSLv23_client_method();
	}
	sock->ssl_ctx=SSL_CTX_new(sock->ssl_meth);
	if (!sock->ssl_ctx) {
		n_warn(N, "ssl_init", "SSL Error");
		return -1;
	}
	if ((certfile==NULL)||(keyfile==NULL)) return 0;
	if (SSL_CTX_use_certificate_file(sock->ssl_ctx, certfile, SSL_FILETYPE_PEM)<=0) {
		n_warn(N, "ssl_init", "SSL Error loading certificate '%s'", certfile);
		return -1;
	}
	if (SSL_CTX_use_PrivateKey_file(sock->ssl_ctx, keyfile, SSL_FILETYPE_PEM)<=0) {
		n_warn(N, "ssl_init", "SSL Error loading private key '%s'", keyfile);
		return -1;
	}
	if (!SSL_CTX_check_private_key(sock->ssl_ctx)) {
		n_warn(N, "ssl_init", "Private key does not match the public certificate");
		return -1;
	}
	return 0;
}

int ssl_accept(nes_state *N, TCP_SOCKET *sock)
{
	if ((sock->ssl=SSL_new(sock->ssl_ctx))==NULL) {
		return -1;
	}
/*	SSL_clear(sock->ssl); */
	SSL_set_fd(sock->ssl, sock->socket);
	if (SSL_accept(sock->ssl)==-1) {
		return -1;
	}
	return 0;
}

int ssl_connect(nes_state *N, TCP_SOCKET *sock)
{
	/* X509 *server_cert; */
	int rc;

	ssl_init(N, sock, 0, NULL, NULL);
	sock->ssl=SSL_new(sock->ssl_ctx);
	SSL_set_fd(sock->ssl, sock->socket);
	if ((rc=SSL_connect(sock->ssl))==-1) {
		n_warn(N, "ssl_connect", "SSL_connect error %d", rc);
		return -1;
	}
	/* the rest is optional */
/*
	printf("SSL connection using %s\r\n", SSL_get_cipher(sock->ssl));
	if ((server_cert=SSL_get_peer_certificate(sock->ssl))!=NULL) {
		X509_free(server_cert);
	}
*/
	return 0;
}

int ssl_read(nes_state *N, TCP_SOCKET *sock, void *buf, int len)
{
	return SSL_read(sock->ssl, buf, len);
}

int ssl_write(nes_state *N, TCP_SOCKET *sock, const void *buf, int len)
{
	return SSL_write(sock->ssl, buf, len);
}

int ssl_close(nes_state *N, TCP_SOCKET *sock)
{
	if (sock->ssl!=NULL) {
		if (SSL_get_shutdown(sock->ssl)&SSL_RECEIVED_SHUTDOWN) {
			SSL_shutdown(sock->ssl);
		} else {
			SSL_clear(sock->ssl);
		}
	}
	if (sock->socket>-1) {
		shutdown(sock->socket, 2);
		closesocket(sock->socket);
		sock->socket=-1;
	}
	if (sock->ssl!=NULL) {
		SSL_free(sock->ssl);
		sock->ssl=NULL;
	}
	return 0;
}

int ssl_shutdown(nes_state *N, TCP_SOCKET *sock)
{
	if (sock->ssl_ctx) {
		SSL_CTX_free(sock->ssl_ctx);
		sock->ssl_ctx=NULL;
	}
	return 0;
}

#endif /* HAVE_SSL */
