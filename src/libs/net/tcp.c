/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2015 Dan Cahill

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
#include "libnet.h"

#ifdef WIN32
#define sleep(x) Sleep(x*1000)
#define msleep(x) Sleep(x)
#define strcasecmp stricmp
typedef int socklen_t;
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define closesocket close
#define msleep(x) usleep(x*1000)
#ifdef MISSING_SOCKLEN
typedef int socklen_t;
#endif
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAXWAIT 10

/*
 * this is the function that terminates orphans
 */
void tcp_murder(nsp_state *N, obj_t *cobj)
{
#define __FN__ __FILE__ ":tcp_murder()"
	TCP_SOCKET *sock;

	n_warn(N, __FN__, "reaper is claiming another lost soul");
	if (cobj->val->type != NT_CDATA || cobj->val->d.str == NULL || strcmp(cobj->val->d.str, "sock4") != 0)
		n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	sock = (TCP_SOCKET *)cobj->val->d.str;
	tcp_close(N, sock, 1);
	n_free(N, (void *)&cobj->val->d.str, sizeof(TCP_SOCKET) + 1);
	cobj->val->size = 0;
	return;
#undef __FN__
}

int tcp_bind(nsp_state *N, char *ifname, unsigned short port)
{
#define __FN__ __FILE__ ":tcp_bind()"
	struct hostent *hp;
	struct sockaddr_in sin;
	int option;
	int sock;

	nc_memset((char *)&sin, 0, sizeof(sin));
	sock = socket(AF_INET, SOCK_STREAM, 0);
	sin.sin_family = AF_INET;
	if (strcasecmp("INADDR_ANY", ifname) == 0) {
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else {
		if ((hp = gethostbyname(ifname)) == NULL) {
			n_warn(N, __FN__, "Host lookup error for %s", ifname);
			return -1;
		}
		nc_memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
	}
	sin.sin_port = htons(port);
	option = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&option, sizeof(option));
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		n_warn(N, __FN__, "bind() error [%s:%d]", ifname, port);
		return -1;
	}
	if (listen(sock, 50) < 0) {
		n_warn(N, __FN__, "listen() error");
		closesocket(sock);
		return -1;
	}
	return sock;
#undef __FN__
}

int tcp_accept(nsp_state *N, TCP_SOCKET *bsock, TCP_SOCKET *asock)
{
#define __FN__ __FILE__ ":tcp_accept()"
	struct sockaddr addr;
	struct sockaddr_in host;
	struct sockaddr_in peer;
	int clientsock;
	socklen_t fromlen;

	/*
		int lowat=1;
		struct timeval timeout;
	*/
	fromlen = sizeof(addr);
	clientsock = accept(bsock->socket, &addr, &fromlen);
	if (clientsock < 0) {
		asock->LocalPort = 0;
		asock->RemotePort = 0;
		n_warn(N, __FN__, "failed tcp_accept");
		return -1;
	}
	asock->socket = clientsock;
#ifdef HAVE_SSL
	if (bsock->use_ssl) {
		_ssl_accept(N, bsock, asock);
		asock->use_ssl = 1;
	}
#endif
	fromlen = sizeof(host);
	getsockname(asock->socket, (struct sockaddr *)&host, &fromlen);
	nc_strncpy(asock->LocalAddr, inet_ntoa(host.sin_addr), sizeof(asock->LocalAddr) - 1);
	asock->LocalPort = ntohs(host.sin_port);
	fromlen = sizeof(peer);
	getpeername(asock->socket, (struct sockaddr *)&peer, &fromlen);
	nc_strncpy(asock->RemoteAddr, inet_ntoa(peer.sin_addr), sizeof(asock->RemoteAddr) - 1);
	asock->RemotePort = ntohs(peer.sin_port);
	/*
		n_warn(N, __FN__, "[%s:%d] new connection", asock->RemoteAddr, asock->RemotePort);
		timeout.tv_sec=1;
		timeout.tv_usec=0;
		setsockopt(clientsock, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
		setsockopt(clientsock, SOL_SOCKET, SO_RCVLOWAT, (void *)&lowat, sizeof(lowat));
	*/
	return clientsock;
#undef __FN__
}

static int tcp_conn(nsp_state *N, TCP_SOCKET *sock, const struct sockaddr_in *serv_addr, socklen_t addrlen, short int use_ssl)
{
	struct sockaddr_in host;
	struct sockaddr_in peer;
	socklen_t fromlen;
	int rc;

	rc = connect(sock->socket, (struct sockaddr *)serv_addr, addrlen);
#ifdef HAVE_SSL
	if ((rc == 0) && (use_ssl)) {
		rc = _ssl_connect(N, sock);
		sock->use_ssl = 1;
	}
#endif
	if (rc < 0) {
		sock->LocalPort = 0;
		sock->RemotePort = 0;
		return -1;
	}
	fromlen = sizeof(host);
	getsockname(sock->socket, (struct sockaddr *)&host, &fromlen);
	nc_strncpy(sock->LocalAddr, inet_ntoa(host.sin_addr), sizeof(sock->LocalAddr) - 1);
	sock->LocalPort = ntohs(host.sin_port);
	fromlen = sizeof(peer);
	getpeername(sock->socket, (struct sockaddr *)&peer, &fromlen);
	nc_strncpy(sock->RemoteAddr, inet_ntoa(peer.sin_addr), sizeof(sock->RemoteAddr) - 1);
	sock->RemotePort = ntohs(peer.sin_port);
	return rc;
}

int tcp_connect(nsp_state *N, TCP_SOCKET *sock, char *host, unsigned short port, short int use_ssl)
{
#define __FN__ __FILE__ ":tcp_connect()"
	struct hostent *hp;
	struct sockaddr_in serv;

	if ((hp = gethostbyname(host)) == NULL) {
		n_warn(N, __FN__, "Host lookup error for %s", host);
		return -1;
	}
	nc_memset((char *)&serv, 0, sizeof(serv));
	nc_memcpy((char *)&serv.sin_addr, hp->h_addr, hp->h_length);
	serv.sin_family = hp->h_addrtype;
	serv.sin_port = htons(port);
	if ((sock->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -2;
	/*	setsockopt(sock->socket, SOL_SOCKET, SO_KEEPALIVE, 0, 0); */
	if (tcp_conn(N, sock, &serv, sizeof(serv), use_ssl) < 0) {
		/* n_warn(N, __FN__, "Error connecting to %s:%d", host, port); */
		return -2;
	}
	return 0;
#undef __FN__
}

int tcp_recv(nsp_state *N, TCP_SOCKET *socket, char *buffer, int max, int flags)
{
#define __FN__ __FILE__ ":tcp_recv()"
	int rc;

#ifndef WIN32
	//	retry :
#endif
	if (socket->socket == -1) return -1;
	if (socket->want_close) {
		tcp_close(N, socket, 1);
		return -1;
	}
	if (max > MAX_TCP_READ_SIZE) max = MAX_TCP_READ_SIZE;
	if (socket->use_ssl) {
#ifdef HAVE_SSL
		rc = _ssl_read(N, socket, buffer, max);
		if (rc == 0) rc = -1;
#else
		rc = -1;
#endif
	}
	else {
		rc = recv(socket->socket, buffer, max, flags);
	}
	if (rc < 0) {
#ifdef WIN32
		//int ecode = 0;
		//int elen = sizeof(int);
		//int x2 = getsockopt(socket->socket, SOL_SOCKET, SO_ERROR, (void *)&ecode, &elen);
		//n_warn(N, __FN__, "[%s:%d] a %d", socket->RemoteAddr, socket->RemotePort, x2);
		int ec = WSAGetLastError();
		switch (ec) {
		case 0:
			return 0;
		case WSAETIMEDOUT:
			return 0;
		default:
			n_warn(N, __FN__, "[%s:%d] WSAGetLastError()=%d", socket->RemoteAddr, socket->RemotePort, ec);
			break;
		}
		return -1;
#else
		switch (errno) {
		case EWOULDBLOCK:
			return 0;
		case ECONNRESET:
			n_warn(N, __FN__, "ECONNRESET[%s:%d] %.100s", socket->RemoteAddr, socket->RemotePort, strerror(errno));
			tcp_close(N, socket, 1); errno = 0; break;
		default:
			if (N->debug) n_warn(N, __FN__, "[%s:%d] %.100s", socket->RemoteAddr, socket->RemotePort, strerror(errno));
			//n_warn(N, __FN__, "[%s:%d] %.100s", socket->RemoteAddr, socket->RemotePort, strerror(errno));
			errno = 0;
		}
		return -1;
#endif
		//} else if (rc==0) {
		//	msleep(MAXWAIT);
		//	goto retry;
	}
	else {
		socket->mtime = time(NULL);
		socket->bytes_in += rc;
	}
	if (N->debug) n_warn(N, __FN__, "[%s:%d] %d bytes of data", socket->RemoteAddr, socket->RemotePort, rc);
	return rc;
#undef __FN__
}

int tcp_send(nsp_state *N, TCP_SOCKET *socket, const char *buffer, int len, int flags)
{
#define __FN__ __FILE__ ":tcp_send()"
	int rc;

	if (socket->socket == -1) return -1;
	if (socket->want_close) {
		tcp_close(N, socket, 1);
		return -1;
	}
	if (socket->use_ssl) {
#ifdef HAVE_SSL
		rc = _ssl_write(N, socket, buffer, len);
#else
		rc = -1;
#endif
	}
	else {
		rc = send(socket->socket, buffer, len, flags);
	}
	if (rc < 0) {
#ifdef WIN32
		return rc;
#else
		if (errno == EWOULDBLOCK) {
			errno = 0;
			msleep(MAXWAIT);
		}
		else if (errno) {
			if (N->debug) n_warn(N, __FN__, "[%s:%d] %d%.100s", socket->RemoteAddr, socket->RemotePort, errno, strerror(errno));
			errno = 0;
		}
#endif
	}
	else if (rc == 0) {
		msleep(MAXWAIT);
	}
	else {
		socket->mtime = time(NULL);
		socket->bytes_out += rc;
	}
	return rc;
#undef __FN__
}

/* fix this */
int nc_vsnprintf(nsp_state *N, char *dest, int max, const char *format, va_list ap);

int tcp_fprintf(nsp_state *N, TCP_SOCKET *socket, const char *format, ...)
{
#define __FN__ __FILE__ ":tcp_fprintf()"
	char *buffer;
	va_list ap;
	int rc;

	if ((buffer = calloc(2048, sizeof(char))) == NULL) {
		n_warn(N, __FN__, "OUT OF MEMORY");
		return -1;
	}
	va_start(ap, format);
	nc_vsnprintf(N, buffer, 2047, format, ap);
	va_end(ap);
	if (N->debug) n_warn(N, __FN__, "[%s:%d] %s", socket->RemoteAddr, socket->RemotePort, buffer);
	rc = tcp_send(N, socket, buffer, strlen(buffer), 0);
	free(buffer);
	return rc;
#undef __FN__
}

int tcp_fgets(nsp_state *N, TCP_SOCKET *socket, char *buffer, int max)
{
#define __FN__ __FILE__ ":tcp_fgets()"
	char *pbuffer = buffer;
	char *obuffer;
	short int lf = 0;
	short int n = 0;
	int rc;
	int x;

retry:
	if (!socket->recvbufsize) {
		x = sizeof(socket->recvbuf) - socket->recvbufoffset - socket->recvbufsize - 2;
		if (x < 1) {
			nc_memset(socket->recvbuf, 0, sizeof(socket->recvbuf));
			socket->recvbufoffset = 0;
			socket->recvbufsize = 0;
			x = sizeof(socket->recvbuf) - socket->recvbufoffset - socket->recvbufsize - 2;
		}
		obuffer = socket->recvbuf + socket->recvbufoffset + socket->recvbufsize;
		if (x > max) x = max;
		if ((rc = tcp_recv(N, socket, obuffer, x, 0)) < 0) {
			return -1;
		}
		else if (rc < 1) {
			/* goto retry; */
			*pbuffer = '\0';
			return n;
		}
		socket->recvbufsize += rc;
	}
	obuffer = socket->recvbuf + socket->recvbufoffset;
	while ((n < max) && (socket->recvbufsize>0)) {
		socket->recvbufoffset++;
		socket->recvbufsize--;
		n++;
		if (*obuffer == '\n') lf = 1;
		*pbuffer++ = *obuffer++;
		if ((lf) || (*obuffer == '\0')) break;
	}
	*pbuffer = '\0';
	if (n > max - 1) {
		/* if (N->debug) n_warn(N, __FN__, "[%s:%d] %s", socket->RemoteAddr, socket->RemotePort, buffer); */
		return n;
	}
	if (!lf) {
		if (socket->recvbufsize > 0) {
			nc_memcpy(socket->recvbuf, socket->recvbuf + socket->recvbufoffset, socket->recvbufsize);
			nc_memset(socket->recvbuf + socket->recvbufsize, 0, sizeof(socket->recvbuf) - socket->recvbufsize);
			socket->recvbufoffset = 0;
		}
		else {
			nc_memset(socket->recvbuf, 0, sizeof(socket->recvbuf));
			socket->recvbufoffset = 0;
			socket->recvbufsize = 0;
		}
		goto retry;
	}
	/* if (N->debug) n_warn(N, __FN__, "[%s:%d] %s", socket->RemoteAddr, socket->RemotePort, buffer); */
	return n;
#undef __FN__
}

int tcp_close(nsp_state *N, TCP_SOCKET *socket, short int owner_killed)
{
	if (!owner_killed) {
		socket->want_close = 1;
	}
	else {
		socket->want_close = 0;
#ifdef HAVE_SSL
		if (socket->use_ssl) _ssl_close(N, socket);
#endif
		if (socket->socket > -1) {
			/* shutdown(x,0=recv, 1=send, 2=both) */
			shutdown(socket->socket, 2);
			closesocket(socket->socket);
			socket->socket = -1;
		}
	}
	return 0;
}

/*
 * start tcp script functions here
 */

NSP_FUNCTION(libnsp_net_tcp_close)
{
#define __FN__ __FILE__ ":libnsp_net_tcp_close()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *cobj;
	//	obj_t *cobj1=nsp_getobj(N, &N->l, "1");
	TCP_SOCKET *sock;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(N, thisobj, "socket");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "sock4") != 0)) {
		//n_warn(N, __FN__, "object is not an open socket");
		return 0;
	}

	sock = (TCP_SOCKET *)cobj->val->d.str;

	//	if (cobj1->val->type!=NT_CDATA||cobj1->val->d.str==NULL||strcmp(cobj1->val->d.str, "sock4")!=0)
	//		n_error(N, NE_SYNTAX, __FN__, "expected a socket for arg1");
	//	sock=(TCP_SOCKET *)cobj1->val->d.str;
	tcp_close(N, sock, 1);
	n_free(N, (void *)&cobj->val->d.str, sizeof(TCP_SOCKET) + 1);
	cobj->val->size = 0;
	nsp_setnum(N, &N->r, "", 0);
	return 0;
#undef __FN__
}

NSP_FUNCTION(libnsp_net_tcp_read)
{
#define __FN__ __FILE__ ":libnsp_net_tcp_read()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *cobj;
	//	obj_t *cobj1=nsp_getobj(N, &N->l, "1");
	TCP_SOCKET *sock;
	int rc;
	//	char tmpbuf[2048];
	char *buf;
	char *p;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(N, thisobj, "socket");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "sock4") != 0))
		n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	sock = (TCP_SOCKET *)cobj->val->d.str;

	//	if (cobj1->val->type!=NT_CDATA||cobj1->val->d.str==NULL||strcmp(cobj1->val->d.str, "sock4")!=0)
	//		n_error(N, NE_SYNTAX, __FN__, "expected a socket for arg1");
	//	sock=(TCP_SOCKET *)cobj1->val->d.str;
	buf = n_alloc(N, MAX_TCP_READ_SIZE, 1);
	if (sock->recvbufsize > 0) {
		p = sock->recvbuf + sock->recvbufoffset;
		rc = sock->recvbufsize;
		sock->recvbufoffset = 0;
		sock->recvbufsize = 0;
	}
	else {
		p = buf;
		rc = tcp_recv(N, sock, buf, MAX_TCP_READ_SIZE - 1, 0);
	}
	if (rc > -1) nsp_setstr(N, &N->r, "", p, rc);
	n_free(N, (void *)&buf, MAX_TCP_READ_SIZE);
	return 0;
#undef __FN__
}

NSP_FUNCTION(libnsp_net_tcp_gets)
{
#define __FN__ __FILE__ ":libnsp_net_tcp_gets()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *cobj;
	//	obj_t *cobj1=nsp_getobj(N, &N->l, "1");
	TCP_SOCKET *sock;
	int rc;
	char *buf;
	//	char tmpbuf[2048];

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(N, thisobj, "socket");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "sock4") != 0))
		n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	sock = (TCP_SOCKET *)cobj->val->d.str;

	//	if (cobj1->val->type!=NT_CDATA||cobj1->val->d.str==NULL||strcmp(cobj1->val->d.str, "sock4")!=0)
	//		n_error(N, NE_SYNTAX, __FN__, "expected a socket for arg1");
	//	sock=(TCP_SOCKET *)cobj1->val->d.str;
	buf = n_alloc(N, MAX_TCP_READ_SIZE, 1);
	rc = tcp_fgets(N, sock, buf, MAX_TCP_READ_SIZE - 1);
	if (rc > -1) {
		striprn(buf);
		nsp_setstr(N, &N->r, "", buf, -1);
	}
	else {
		nsp_setnum(N, &N->r, "", rc);
	}
	n_free(N, (void *)&buf, MAX_TCP_READ_SIZE);
	return 0;
#undef __FN__
}

NSP_FUNCTION(libnsp_net_tcp_write)
{
#define __FN__ __FILE__ ":libnsp_net_tcp_write()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *cobj;
	//	obj_t *cobj1=nsp_getobj(N, &N->l, "1");
	obj_t *cobj2 = nsp_getobj(N, &N->l, "1");
	TCP_SOCKET *sock;
	int rc;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(N, thisobj, "socket");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "sock4") != 0))
		n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	sock = (TCP_SOCKET *)cobj->val->d.str;

	//	if (cobj1->val->type!=NT_CDATA||cobj1->val->d.str==NULL||strcmp(cobj1->val->d.str, "sock4")!=0)
	//		n_error(N, NE_SYNTAX, __FN__, "expected a socket for arg1");
	if (cobj2->val->type != NT_STRING || cobj2->val->d.str == NULL)
		n_error(N, NE_SYNTAX, __FN__, "expected a string for arg1");
	//	sock=(TCP_SOCKET *)cobj1->val->d.str;
	rc = tcp_send(N, sock, cobj2->val->d.str, cobj2->val->size, 0);
	if (rc > -1) {
		nsp_setnum(N, &N->r, "", rc);
	}
	else {
		nsp_setnum(N, &N->r, "", rc);
	}
	return 0;
#undef __FN__
}

NSP_FUNCTION(libnsp_net_tcp_setsockopt)
{
#define __FN__ __FILE__ ":libnsp_net_tcp_setsockopt()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *cobj1 = nsp_getobj(N, &N->l, "1");
	obj_t *cobj2 = nsp_getobj(N, &N->l, "2");
	obj_t *cobj;
	TCP_SOCKET *sock;
	char *opt;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(N, thisobj, "socket");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "sock4") != 0))
		n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	sock = (TCP_SOCKET *)cobj->val->d.str;
	if (!nsp_isstr(cobj1) || cobj1->val->d.str == NULL)
		n_error(N, NE_SYNTAX, __FN__, "expected a string for arg1");
	opt = cobj1->val->d.str;
	if (strcmp(opt, "SO_RCVTIMEO") == 0) {
		if (!nsp_isnum(cobj2)) n_error(N, NE_SYNTAX, __FN__, "expected a number for arg2");
		{
#ifdef WIN32
			DWORD to = (long)(cobj2->val->d.num);
			setsockopt(sock->socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&to, sizeof(to));
#else
			struct timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = (long)(cobj2->val->d.num) * 1000;
			setsockopt(sock->socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
#endif
		}
		{
			int lowat = 1;
			setsockopt(sock->socket, SOL_SOCKET, SO_RCVLOWAT, (void *)&lowat, sizeof(lowat));
		}
	}
	else if (strcmp(opt, "SO_KEEPALIVE") == 0) {
		int keepalive;

		if (!nsp_isbool(cobj2) && !nsp_isnum(cobj2)) n_error(N, NE_SYNTAX, __FN__, "expected a boolean for arg2");
		keepalive = cobj2->val->d.num ? 1 : 0;
		setsockopt(sock->socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
	}
	return 0;
#undef __FN__
}

NSP_FUNCTION(libnsp_net_tcp_info)
{
#define __FN__ __FILE__ ":libnsp_net_tcp_info()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	//obj_t *cobj1=nsp_getobj(N, &N->l, "1");
	obj_t *cobj;
	obj_t tobj;
	TCP_SOCKET *sock;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(N, thisobj, "socket");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "sock4") != 0))
		n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	sock = (TCP_SOCKET *)cobj->val->d.str;
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nsp_linkval(N, &tobj, NULL);
	tobj.val->type = NT_TABLE;
	tobj.val->attr |= NST_AUTOSORT;
	nsp_setnum(N, &tobj, "bytes_in", sock->bytes_in);
	nsp_setnum(N, &tobj, "bytes_out", sock->bytes_out);
	nsp_setnum(N, &tobj, "ctime", (num_t)sock->ctime);
	nsp_setnum(N, &tobj, "mtime", (num_t)sock->mtime);
	nsp_setstr(N, &tobj, "local_addr", sock->LocalAddr, -1);
	nsp_setnum(N, &tobj, "local_port", sock->LocalPort);
	nsp_setstr(N, &tobj, "remote_addr", sock->RemoteAddr, -1);
	nsp_setnum(N, &tobj, "remote_port", sock->RemotePort);
	cobj = nsp_setnum(N, &tobj, "use_ssl", sock->use_ssl ? 1 : 0);
	cobj->val->type = NT_BOOLEAN;
	nsp_linkval(N, &N->r, &tobj);
	nsp_unlinkval(N, &tobj);
	return 0;
#undef __FN__
}

NSP_FUNCTION(libnsp_net_tcp_accept)
{
#define __FN__ __FILE__ ":libnsp_net_tcp_accept()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *cobj;
	obj_t tobj;
	TCP_SOCKET *asock, *bsock;
	int rc;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(N, thisobj, "socket");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "sock4") != 0))
		n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	bsock = (TCP_SOCKET *)cobj->val->d.str;
	if ((asock = n_alloc(N, sizeof(TCP_SOCKET) + 1, 1)) == NULL) {
		n_warn(N, __FN__, "couldn't alloc %d bytes", sizeof(TCP_SOCKET) + 1);
		return -1;
	}
	nc_strncpy(asock->obj_type, "sock4", sizeof(asock->obj_type) - 1);
	asock->obj_term = (NSP_CFREE)tcp_murder;
	if ((rc = tcp_accept(N, bsock, asock)) < 0) {
		n_free(N, (void *)&asock, sizeof(TCP_SOCKET) + 1);
		return -1;
	}
	/* build a new tcp socket 'object' to return */
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nsp_setvaltype(N, &tobj, NT_TABLE);
	cobj = nsp_settable(N, &tobj, "this");
	cobj->val->attr &= ~NST_AUTOSORT;
	//cobj->val->attr|=NST_HIDDEN;
	nsp_linkval(N, cobj, &tobj);
	cobj = nsp_setcdata(N, &tobj, "socket", NULL, 0);
	cobj->val->d.str = (void *)asock;
	cobj->val->size = sizeof(TCP_SOCKET) + 1;
	cobj = nsp_getobj(N, nsp_getobj(N, &N->g, "net"), "tcp");
	if (nsp_istable(cobj)) nsp_zlink(N, &N->l, cobj);
	nsp_linkval(N, &N->r, &tobj);
	nsp_unlinkval(N, &tobj);
	return 0;
#undef __FN__
}

NSP_FUNCTION(libnsp_net_tcp_bind)
{
#define __FN__ __FILE__ ":libnsp_net_tcp_bind()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *cobj;
	obj_t *cobj1 = nsp_getobj(N, &N->l, "1"); /* host */
	obj_t *cobj2 = nsp_getobj(N, &N->l, "2"); /* port */
	obj_t *cobj3 = nsp_getobj(N, &N->l, "3"); /* SSL */
	TCP_SOCKET *bsock;
	int rc;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(N, thisobj, "socket");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "sock4") != 0))
		n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	bsock = (TCP_SOCKET *)cobj->val->d.str;

	if (cobj1->val->type != NT_STRING) n_error(N, NE_SYNTAX, __FN__, "expected a string for arg1");
	if (cobj2->val->type != NT_NUMBER) n_error(N, NE_SYNTAX, __FN__, "expected a number for arg2");
	//	bsock=n_alloc(N, sizeof(TCP_SOCKET)+1, 1);
	//	if (bsock==NULL) {
	//		n_warn(N, __FN__, "couldn't alloc %d bytes", sizeof(TCP_SOCKET)+1);
	//		return -1;
	//	}
	//	nc_strncpy(bsock->obj_type, "sock4", sizeof(bsock->obj_type)-1);
	//	bsock->obj_term=(NSP_CFREE)tcp_murder;
	if ((rc = tcp_bind(N, cobj1->val->d.str, (unsigned short)cobj2->val->d.num)) < 0) {
		n_free(N, (void *)&cobj->val->d.str, sizeof(TCP_SOCKET) + 1);
		cobj->val->size = 0;

		nsp_setstr(N, &N->r, "", "tcp error", -1);
		//		n_free(N, (void *)&bsock, sizeof(TCP_SOCKET)+1);
		return -1;
	}
	bsock->socket = rc;
	bsock->use_ssl = 0;
	if (nsp_tobool(N, cobj3)) {
#ifdef HAVE_SSL
		obj_t *tobj = nsp_getobj(N, &N->l, "4"); /* ssl opts */
		char *pc = NULL, *pk = NULL;

		if (nsp_istable(tobj)) {
			cobj1 = nsp_getobj(N, tobj, "certfile");
			cobj2 = nsp_getobj(N, tobj, "keyfile");
			if (cobj1->val->type == NT_STRING&&cobj2->val->type == NT_STRING&&cobj1->val->size > 0 && cobj2->val->size > 0) {
				pc = cobj1->val->d.str;
				pk = cobj2->val->d.str;
			}
		}
		rc = _ssl_init(N, bsock, 1, pc, pk);
		bsock->use_ssl = 1;
#else
		n_warn(N, __FN__, "SSL is not available");
#endif
	}
	/* nsp_setcdata(N, &N->r, "", bsock, sizeof(TCP_SOCKET)+1); */

//	cobj=nsp_setcdata(N, thisobj, "socket", NULL, 0);
//	cobj->val->d.str=(void *)bsock;
//	cobj->val->size=sizeof(TCP_SOCKET)+1;

//	nsp_setcdata(N, &N->r, "", NULL, 0);
//	N->r.val->d.str=(void *)bsock;
//	N->r.val->size=sizeof(TCP_SOCKET)+1;
	return 0;
#undef __FN__
}

NSP_FUNCTION(libnsp_net_tcp_connect)
{
#define __FN__ __FILE__ ":libnsp_net_tcp_connect()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *cobj;
	obj_t *cobj1 = nsp_getobj(N, &N->l, "1"); /* host */
	obj_t *cobj2 = nsp_getobj(N, &N->l, "2"); /* port*/
	obj_t *cobj3 = nsp_getobj(N, &N->l, "3"); /* SSL */
	unsigned short use_ssl = 0;
	TCP_SOCKET *sock;
	int rc;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(N, thisobj, "socket");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "sock4") != 0))
		n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	sock = (TCP_SOCKET *)cobj->val->d.str;

	if (cobj1->val->type != NT_STRING || cobj1->val->d.str == NULL)
		n_error(N, NE_SYNTAX, __FN__, "expected a string for arg1");
	if (cobj2->val->type != NT_NUMBER)
		n_error(N, NE_SYNTAX, __FN__, "expected a number for arg2");
	if (cobj3->val->type == NT_BOOLEAN || cobj3->val->type == NT_NUMBER)
		use_ssl = cobj3->val->d.num ? 1 : 0;
	if ((rc = tcp_connect(N, sock, cobj1->val->d.str, (unsigned short)cobj2->val->d.num, use_ssl)) < 0) {
		n_free(N, (void *)&cobj->val->d.str, sizeof(TCP_SOCKET) + 1);
		cobj->val->size = 0;
		nsp_setstr(N, &N->r, "", "tcp error", -1);
		// n_free(N, (void *)&sock, sizeof(TCP_SOCKET)+1);
		return -1;
	}
	return 0;
#undef __FN__
}

NSP_FUNCTION(libnsp_net_tcp_tlsaccept)
{
#define __FN__ __FILE__ ":libnsp_net_tcp_tlsaccept()"
#ifdef HAVE_SSL
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *tobj = nsp_getobj(N, &N->l, "1"); /* ssl opts */
	obj_t *cobj;
	TCP_SOCKET *sock;
	int rc;
	char *pc = NULL, *pk = NULL;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(N, thisobj, "socket");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "sock4") != 0))
		n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	sock = (TCP_SOCKET *)cobj->val->d.str;


	if (nsp_istable(tobj)) {
		obj_t *cobj1 = nsp_getobj(N, tobj, "certfile");
		obj_t *cobj2 = nsp_getobj(N, tobj, "keyfile");
		if (cobj1->val->type == NT_STRING&&cobj2->val->type == NT_STRING&&cobj1->val->size > 0 && cobj2->val->size > 0) {
			pc = cobj1->val->d.str;
			pk = cobj2->val->d.str;
		}
	}
	rc = _ssl_init(N, sock, 1, pc, pk);
	_ssl_accept(N, sock, sock);
	sock->use_ssl = 1;
#else
	n_warn(N, __FN__, "SSL is not available");
#endif
#undef __FN__
	return 0;
}

NSP_FUNCTION(libnsp_net_tcp_tlsconnect)
{
#define __FN__ __FILE__ ":libnsp_net_tcp_tlsconnect()"
#ifdef HAVE_SSL
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *tobj = nsp_getobj(N, &N->l, "1"); /* ssl opts */
	obj_t *cobj;
	TCP_SOCKET *sock;
	int rc;
	//char *pc = NULL, *pk = NULL;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(N, thisobj, "socket");
	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (strcmp(cobj->val->d.str, "sock4") != 0))
		n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	sock = (TCP_SOCKET *)cobj->val->d.str;

	rc = _ssl_connect(N, sock);
	sock->use_ssl = 1;
#else
	n_warn(N, __FN__, "SSL is not available");
#endif
#undef __FN__
	return 0;
}

NSP_FUNCTION(libnsp_net_tcp_socket)
{
#define __FN__ __FILE__ ":libnsp_net_tcp_socket()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *cobj;
	TCP_SOCKET *sock;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	if ((sock = n_alloc(N, sizeof(TCP_SOCKET) + 1, 1)) == NULL) {
		n_warn(N, __FN__, "couldn't alloc %d bytes", sizeof(TCP_SOCKET) + 1);
		return -1;
	}
	nc_strncpy(sock->obj_type, "sock4", sizeof(sock->obj_type) - 1);
	sock->obj_term = (NSP_CFREE)tcp_murder;
	cobj = nsp_setcdata(N, thisobj, "socket", NULL, 0);
	cobj->val->d.str = (void *)sock;
	cobj->val->size = sizeof(TCP_SOCKET) + 1;
	cobj = nsp_getobj(N, nsp_getobj(N, &N->g, "net"), "tcp");
	if (nsp_istable(cobj)) nsp_zlink(N, &N->l, cobj);
	return 0;
#undef __FN__
}
