/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2008 Dan Cahill

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

#if defined HAVE_OPENSSL

int ossl_init(nes_state *N, TCP_SOCKET *sock, short srvmode, char *certfile, char *keyfile)
{
#define __FUNCTION__ __FILE__ ":ossl_init()"
	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
	if (srvmode) {
		sock->ssl_meth=SSLv23_server_method();
	} else {
		sock->ssl_meth=SSLv23_client_method();
	}
	sock->ssl_ctx=SSL_CTX_new(sock->ssl_meth);
	if (!sock->ssl_ctx) {
		n_warn(N, __FUNCTION__, "SSL Error");
		return -1;
	}
	if ((certfile==NULL)||(keyfile==NULL)) return 0;
	if (SSL_CTX_use_certificate_file(sock->ssl_ctx, certfile, SSL_FILETYPE_PEM)<=0) {
		n_warn(N, __FUNCTION__, "SSL Error loading certificate '%s'", certfile);
		return -1;
	}
	if (SSL_CTX_use_PrivateKey_file(sock->ssl_ctx, keyfile, SSL_FILETYPE_PEM)<=0) {
		n_warn(N, __FUNCTION__, "SSL Error loading private key '%s'", keyfile);
		return -1;
	}
	if (!SSL_CTX_check_private_key(sock->ssl_ctx)) {
		n_warn(N, __FUNCTION__, "Private key does not match the public certificate");
		return -1;
	}
	return 0;
#undef __FUNCTION__
}

int ossl_accept(nes_state *N, TCP_SOCKET *bsock, TCP_SOCKET *asock)
{
	if ((asock->ssl=SSL_new(bsock->ssl_ctx))==NULL) {
		return -1;
	}
/*	SSL_clear(asock->ssl); */
	SSL_set_fd(asock->ssl, asock->socket);
	if (SSL_accept(asock->ssl)==-1) {
		return -1;
	}
	return 0;
}

int ossl_connect(nes_state *N, TCP_SOCKET *sock)
{
#define __FUNCTION__ __FILE__ ":ossl_connect()"
	/* X509 *server_cert; */
	int rc;

	ossl_init(N, sock, 0, NULL, NULL);
	sock->ssl=SSL_new(sock->ssl_ctx);
	SSL_set_fd(sock->ssl, sock->socket);
	if ((rc=SSL_connect(sock->ssl))==-1) {
		n_warn(N, __FUNCTION__, "SSL_connect error %d", rc);
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
#undef __FUNCTION__
}

int ossl_read(nes_state *N, TCP_SOCKET *sock, void *buf, int max)
{
	return SSL_read(sock->ssl, buf, max);
}

int ossl_write(nes_state *N, TCP_SOCKET *sock, const void *buf, int max)
{
	return SSL_write(sock->ssl, buf, max);
}

int ossl_close(nes_state *N, TCP_SOCKET *sock)
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

int ossl_shutdown(nes_state *N, TCP_SOCKET *sock)
{
	if (sock->ssl_ctx) {
		SSL_CTX_free(sock->ssl_ctx);
		sock->ssl_ctx=NULL;
	}
	return 0;
}

#elif defined HAVE_XYSSL

#include "xyssl/certs.h"
#include "xyssl/net.h"

/*
 * Computing a safe DH-1024 prime takes ages, so it's faster
 * to use a precomputed value (provided below as an example).
 * Run the dh_genprime program to generate an acceptable P.
 */
char *dhm_P = 
 "B1D36BED209EFC365817CAAE55F4E5D7C67F84AC74343A1A546DF4BD7821C3BD48436476E02092ED85396EA953F44F1EDA389962A416242FDC798D2A12A86FED" \
 "655F4885D8BFCE85A866FB52D0BE97CFA780CFE42CD976A4ED1B199C5111096A0FD857B0D50F2BB003C023DA6597432F9EB47D437FF94998424370BB28EA7673";
char *dhm_G = "4";
//G = 04

/*
char *dhm_P = 
    "E4004C1F94182000103D883A448B3F80" \
    "2CE4B44A83301270002C20D0321CFD00" \
    "11CCEF784C26A400F43DFB901BCA7538" \
    "F2C6B176001CF5A0FD16D2C48B1D0C1C" \
    "F6AC8E1DA6BCC3B4E1F96B0564965300" \
    "FFA1D0B601EB2800F489AA512C4B248C" \
    "01F76949A60BB7F00A40B1EAB64BDD48" \
    "E8A700D60B7F1200FA8E77B0A979DABF";

char *dhm_G = "4";

*/
/*
 * sorted by order of preference
 */
int my_preferred_ciphers[] = {
	SSL_EDH_RSA_AES_256_SHA,
	SSL_EDH_RSA_DES_168_SHA,
	SSL_RSA_AES_256_SHA,
	SSL_RSA_DES_168_SHA,
	SSL_RSA_RC4_128_SHA,
	SSL_RSA_RC4_128_MD5,
	0
};

int xyssl_init(nes_state *N, TCP_SOCKET *sock, short srvmode, char *certfile, char *keyfile)
{
#define __FUNCTION__ __FILE__ ":xyssl_init()"
	int rc;

	if ((certfile==NULL)||(keyfile==NULL)) {
		rc=x509parse_crt(&sock->srvcert, (unsigned char *)test_srv_crt, nc_strlen(test_srv_crt));
		if (rc!=0) {
			n_warn(N, __FUNCTION__, "x509parse_crt returned %08x", rc);
			return -1;
		}
		rc=x509parse_crt(&sock->srvcert, (unsigned char *)test_ca_crt, nc_strlen(test_ca_crt));
		if (rc!=0) {
			n_warn(N, __FUNCTION__, "x509parse_crt returned %08x", rc);
			return -1;
		}
		rc=x509parse_key(&sock->rsa, (unsigned char *)test_srv_key, nc_strlen(test_srv_key), NULL, 0);
		if (rc!=0) {
			n_warn(N, __FUNCTION__, "x509_parse_key returned %08x", rc);
			return -1;
		}
	} else {
		rc=x509parse_crtfile(&sock->srvcert, certfile);
		if (rc!=0) {
			n_warn(N, __FUNCTION__, "x509_read_crtfile returned %08x", rc);
			return -1;
		}
		rc=x509parse_keyfile(&sock->rsa, keyfile, NULL);
		if (rc!=0) {
			n_warn(N, __FUNCTION__, "x509_read_keyfile returned %08x", rc);
			return -1;
		}
	}
#undef __FUNCTION__
	return 0;
}

int xyssl_accept(nes_state *N, TCP_SOCKET *bsock, TCP_SOCKET *asock)
{
#define __FUNCTION__ __FILE__ ":xyssl_accept()"
	int rc;

	havege_init(&asock->hs);
	if ((rc=ssl_init(&asock->ssl))!=0) {
		n_warn(N, __FUNCTION__, "ssl_init returned %08x", rc);
		return rc;
	}
//	ssl_set_debuglvl(&asock->ssl, 0);
	ssl_set_endpoint(&asock->ssl, SSL_IS_SERVER);
	ssl_set_authmode(&asock->ssl, SSL_VERIFY_NONE);
	ssl_set_rng(&asock->ssl, havege_rand, &asock->hs);
	ssl_set_bio(&asock->ssl, net_recv, &asock->socket, net_send, &asock->socket);
//	ssl_set_scb(&asock->ssl, my_get_session, my_set_session);
	ssl_set_ciphers(&asock->ssl, my_preferred_ciphers);

	nc_memset(&asock->ssn, 0, sizeof(ssl_session));
	ssl_set_session(&asock->ssl, 1, 0, &asock->ssn);

	ssl_set_ca_chain(&asock->ssl, bsock->srvcert.next, NULL);
	ssl_set_own_cert(&asock->ssl, &bsock->srvcert, &bsock->rsa);
//	ssl_set_sidtable(&asock->ssl, bsock->session_table);
	ssl_set_dh_param(&asock->ssl, dhm_P, dhm_G);

	while((rc=ssl_handshake(&asock->ssl))!=0) {
		if (rc!=XYSSL_ERR_NET_TRY_AGAIN) {
			n_warn(N, __FUNCTION__, "ssl_handshake returned %08x", rc);
			return rc;
		}
	}
	return 0;
#undef __FUNCTION__
}

int xyssl_connect(nes_state *N, TCP_SOCKET *sock)
{
#define __FUNCTION__ __FILE__ ":xyssl_connect()"
	int rc;

	havege_init(&sock->hs);
	if ((rc=ssl_init(&sock->ssl))!=0) {
		n_warn(N, __FUNCTION__, "ssl_init returned %08x", rc);
		return rc;
	}
//	ssl_set_debuglvl(&sock->ssl, 0);
	ssl_set_endpoint(&sock->ssl, SSL_IS_CLIENT);
	ssl_set_authmode(&sock->ssl, SSL_VERIFY_NONE);
	ssl_set_rng(&sock->ssl, havege_rand, &sock->hs);
	ssl_set_bio(&sock->ssl, net_recv, &sock->socket, net_send, &sock->socket);
	ssl_set_ciphers(&sock->ssl, ssl_default_ciphers);
	nc_memset(&sock->ssn, 0, sizeof(ssl_session));
	ssl_set_session(&sock->ssl, 1, 600, &sock->ssn);
	return 0;
#undef __FUNCTION__
}

int xyssl_read(nes_state *N, TCP_SOCKET *sock, void *buf, int max)
{
#define __FUNCTION__ __FILE__ ":xyssl_read()"
	int rc;

	while ((rc=ssl_read(&sock->ssl, (void *)buf, max))<=0) {
		if (rc==XYSSL_ERR_NET_TRY_AGAIN) continue;
		switch (rc) {
		case XYSSL_ERR_NET_RECV_FAILED:
			n_warn(N, __FUNCTION__, "XYSSL_ERR_NET_RECV_FAILED");
			break;
		case XYSSL_ERR_NET_CONN_RESET:
//			n_warn(N, __FUNCTION__, "XYSSL_ERR_NET_CONN_RESET");
			break;
		default:
			n_warn(N, __FUNCTION__, "%08x", rc);
		}
		return -1;
	}
	return rc;
#undef __FUNCTION__
}

int xyssl_write(nes_state *N, TCP_SOCKET *sock, const void *buf, int len)
{
#define __FUNCTION__ __FILE__ ":xyssl_write()"
	int rc;

	while ((rc=ssl_write(&sock->ssl, (void *)buf, len))<=0) {
		if (rc==XYSSL_ERR_NET_TRY_AGAIN) continue;
		switch (rc) {
		case XYSSL_ERR_NET_SEND_FAILED:
			n_warn(N, __FUNCTION__, "XYSSL_ERR_NET_SEND_FAILED");
			break;
		case XYSSL_ERR_NET_CONN_RESET:
//			n_warn(N, __FUNCTION__, "XYSSL_ERR_NET_CONN_RESET");
			break;
		default:
			n_warn(N, __FUNCTION__, "%08x", rc);
			break;
		}
		return -1;
	}
	return len;
#undef __FUNCTION__
}

int xyssl_close(nes_state *N, TCP_SOCKET *sock)
{
	ssl_close_notify(&sock->ssl);
	if (sock->socket>-1) {
		shutdown(sock->socket, 2);
		closesocket(sock->socket);
		sock->socket=-1;
	}
	return 0;
}

int xyssl_shutdown(nes_state *N, TCP_SOCKET *sock)
{
	if (sock->use_ssl) {
		/* x509 and rsa for server sockets */
		x509_free(&sock->srvcert);
		rsa_free(&sock->rsa);

		ssl_free(&sock->ssl);
		sock->use_ssl=0;
	}
	return 0;
}

#endif
