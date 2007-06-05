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
void tcp_murder(nes_state *N, obj_t *cobj)
{
	TCP_SOCKET *sock;

	n_warn(N, "tcp_murder", "reaper is claiming another lost soul");
	if ((cobj->val->type!=NT_CDATA)||(cobj->val->d.str==NULL)||(strcmp(cobj->val->d.str, "sock4")!=0))
		n_error(N, NE_SYNTAX, "tcp_murder", "expected a socket");
	sock=(TCP_SOCKET *)cobj->val->d.str;
	tcp_close(N, sock, 1);
	n_free(N, (void *)&cobj->val->d.str);
	return;
}

static void striprn(char *string)
{
	int i=strlen(string)-1;

	while (i>-1) {
		if ((string[i]!='\r')&&(string[i]!='\n')) break;
		string[i--]='\0';
	}
}

int tcp_bind(nes_state *N, char *ifname, unsigned short port)
{
	struct hostent *hp;
	struct sockaddr_in sin;
	int i;
	int option;
	int sock;

	memset((char *)&sin, 0, sizeof(sin));
	sock=socket(AF_INET, SOCK_STREAM, 0);
	sin.sin_family=AF_INET;
	if (strcasecmp("INADDR_ANY", ifname)==0) {
		sin.sin_addr.s_addr=htonl(INADDR_ANY);
	} else {
		if ((hp=gethostbyname(ifname))==NULL) {
			n_warn(N, "tcp_bind", "Host lookup error for %s", ifname);
			return -1;
		}
		memmove((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
	}
	sin.sin_port=htons(port);
	option=1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&option, sizeof(option));
	i=0;
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin))<0) {
		n_warn(N, "tcp_bind", "bind() error [%s:%d]", ifname, port);
		return -1;
	}
	if (listen(sock, 50)<0) {
		n_warn(N, "tcp_bind", "listen() error");
		closesocket(sock);
		return -1;
	}
	return sock;
}

int tcp_accept(nes_state *N, int listensock, TCP_SOCKET *sock)
{
	struct sockaddr addr;
	struct sockaddr_in host;
	struct sockaddr_in peer;
	int clientsock;
	socklen_t fromlen;
/*
	int lowat=1;
	struct timeval timeout;
*/
	fromlen=sizeof(addr);
	clientsock=accept(listensock, &addr, &fromlen);
	if (clientsock<0) {
		sock->LocalPort=0;
		sock->RemotePort=0;
		n_warn(N, "tcp_accept", "failed tcp_accept");
		return -1;
	}
	sock->socket=clientsock;
	fromlen=sizeof(host);
	getsockname(sock->socket, (struct sockaddr *)&host, &fromlen);
	strncpy(sock->LocalAddr, inet_ntoa(host.sin_addr), sizeof(sock->LocalAddr)-1);
	sock->LocalPort=ntohs(host.sin_port);
	fromlen=sizeof(peer);
	getpeername(sock->socket, (struct sockaddr *)&peer, &fromlen);
	strncpy(sock->RemoteAddr, inet_ntoa(peer.sin_addr), sizeof(sock->RemoteAddr)-1);
	sock->RemotePort=ntohs(peer.sin_port);
/*
	n_warn(N, "tcp_accept", "[%s:%d] tcp_accept: new connection", sock->RemoteAddr, sock->RemotePort);
	timeout.tv_sec=1;
	timeout.tv_usec=0;
	setsockopt(clientsock, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
	setsockopt(clientsock, SOL_SOCKET, SO_RCVLOWAT, (void *)&lowat, sizeof(lowat));
*/
	return clientsock;
}

static int tcp_conn(nes_state *N, TCP_SOCKET *sock, const struct sockaddr_in *serv_addr, socklen_t addrlen, short int use_ssl)
{
	struct sockaddr_in host;
	struct sockaddr_in peer;
	socklen_t fromlen;
	int rc;

	rc=connect(sock->socket, (struct sockaddr *)serv_addr, addrlen);
#ifdef HAVE_SSL
	if ((rc==0)&&(use_ssl)) {
		rc=ssl_connect(N, sock);
	}
#endif
	if (rc<0) {
		sock->LocalPort=0;
		sock->RemotePort=0;
		return -1;
	}
	fromlen=sizeof(host);
	getsockname(sock->socket, (struct sockaddr *)&host, &fromlen);
	strncpy(sock->LocalAddr, inet_ntoa(host.sin_addr), sizeof(sock->LocalAddr)-1);
	sock->LocalPort=ntohs(host.sin_port);
	fromlen=sizeof(peer);
	getpeername(sock->socket, (struct sockaddr *)&peer, &fromlen);
	strncpy(sock->RemoteAddr, inet_ntoa(peer.sin_addr), sizeof(sock->RemoteAddr)-1);
	sock->RemotePort=ntohs(peer.sin_port);
	return rc;
}

int tcp_connect(nes_state *N, TCP_SOCKET *sock, char *host, unsigned short port, short int use_ssl)
{
	struct hostent *hp;
	struct sockaddr_in serv;

	if ((hp=gethostbyname(host))==NULL) {
		n_warn(N, "tcp_connect", "Host lookup error for %s", host);
		return -1;
	}
	memset((char *)&serv, 0, sizeof(serv));
	memmove((char *)&serv.sin_addr, hp->h_addr, hp->h_length);
	serv.sin_family=hp->h_addrtype;
	serv.sin_port=htons(port);
	if ((sock->socket=socket(AF_INET, SOCK_STREAM, 0))<0) return -2;
/*	setsockopt(sock->socket, SOL_SOCKET, SO_KEEPALIVE, 0, 0); */
	if (tcp_conn(N, sock, &serv, sizeof(serv), use_ssl)<0) {
		/* n_warn(N, "tcp_connect", "Error connecting to %s:%d", host, port); */
		return -2;
	}
	return 0;
}

int tcp_recv(nes_state *N, TCP_SOCKET *socket, char *buffer, int len, int flags)
{
	int rc;

retry:
	if (socket->socket==-1) return -1;
	if (socket->want_close) {
		tcp_close(N, socket, 1);
		return -1;
	}
	if (len>16384) len=16384;
#ifdef HAVE_SSL
	if (socket->ssl) {
		rc=ssl_read(N, socket, buffer, len);
		if (rc==0) rc=-1;
	} else {
		rc=recv(socket->socket, buffer, len, flags);
	}
#else
	rc=recv(socket->socket, buffer, len, flags);
/*
	buffer[rc]=0;
	printf("\r\n--============================--\r\nrc=%d [%s]", rc, buffer);
*/
#endif
	if (rc<0) {
#ifdef WIN32
		return -1;
#else
		switch (errno) {
		case ECONNRESET:
			tcp_close(N, socket, 1); errno=0; break;
		case EWOULDBLOCK:
			msleep(MAXWAIT); errno=0; goto retry;
		default:
			if (N->debug) n_warn(N, "tcp_recv", "[%s:%d] tcp_recv: %.100s", socket->RemoteAddr, socket->RemotePort, strerror(errno));
			errno=0;
		}
		return -1;
#endif
/*
	} else if (rc==0) {
		msleep(MAXWAIT);
		goto retry;
*/
	} else {
		socket->mtime=time(NULL);
		socket->bytes_in+=rc;
	}
	if (N->debug) n_warn(N, "tcp_recv", "[%s:%d] tcp_recv: %d bytes of binary data", socket->RemoteAddr, socket->RemotePort, rc);
	return rc;
}

int tcp_send(nes_state *N, TCP_SOCKET *socket, const char *buffer, int len, int flags)
{
	int rc;

	if (socket->socket==-1) return -1;
	if (socket->want_close) {
		tcp_close(N, socket, 1);
		return -1;
	}
#ifdef HAVE_SSL
	if (socket->ssl) {
		rc=ssl_write(N, socket, buffer, len);
	} else {
		rc=send(socket->socket, buffer, len, flags);
	}
#else
	rc=send(socket->socket, buffer, len, flags);
#endif
	if (rc<0) {
#ifdef WIN32
		return rc;
#else
		if (errno==EWOULDBLOCK) {
			errno=0;
			msleep(MAXWAIT);
		} else if (errno) {
			if (N->debug) n_warn(N, "tcp_send", "[%s:%d] tcp_send: %d%.100s", socket->RemoteAddr, socket->RemotePort, errno, strerror(errno));
			errno=0;
		}
#endif
	} else if (rc==0) {
		msleep(MAXWAIT);
	} else {
		socket->mtime=time(NULL);
		socket->bytes_out+=rc;
	}
	return rc;
}

/* fix this */
int nc_vsnprintf(nes_state *N, char *dest, int max, const char *format, va_list ap);

int tcp_fprintf(nes_state *N, TCP_SOCKET *socket, const char *format, ...)
{
	char *buffer;
	va_list ap;
	int rc;

	if ((buffer=calloc(2048, sizeof(char)))==NULL) {
		n_warn(N, "tcp_send", "OUT OF MEMORY");
		return -1;
	}
	va_start(ap, format);
	nc_vsnprintf(N, buffer, 2047, format, ap);
	va_end(ap);
	if (N->debug) n_warn(N, "tcp_fprintf", "[%s:%d] tcp_fprintf: %s", socket->RemoteAddr, socket->RemotePort, buffer);
	rc=tcp_send(N, socket, buffer, strlen(buffer), 0);
	free(buffer);
	return rc;
}

int tcp_fgets(nes_state *N, char *buffer, int max, TCP_SOCKET *socket)
{
	char *pbuffer=buffer;
	char *obuffer;
	short int lf=0;
	short int n=0;
	short int rc;
	short int x;

retry:
	if (!socket->recvbufsize) {
		x=sizeof(socket->recvbuf)-socket->recvbufoffset-socket->recvbufsize-2;
		if (x<1) {
			memset(socket->recvbuf, 0, sizeof(socket->recvbuf));
			socket->recvbufoffset=0;
			socket->recvbufsize=0;
			x=sizeof(socket->recvbuf)-socket->recvbufoffset-socket->recvbufsize-2;
		}
		obuffer=socket->recvbuf+socket->recvbufoffset+socket->recvbufsize;
		rc=tcp_recv(N, socket, obuffer, x, 0);
		if (rc<0) {
			return -1;
		} else if (rc<1) {
/*
			goto retry;
*/
			*pbuffer='\0';
			return n;
		}
		socket->recvbufsize+=rc;
	}
	obuffer=socket->recvbuf+socket->recvbufoffset;
	while ((n<max)&&(socket->recvbufsize>0)) {
		socket->recvbufoffset++;
		socket->recvbufsize--;
		n++;
		if (*obuffer=='\n') lf=1;
		*pbuffer++=*obuffer++;
		if ((lf)||(*obuffer=='\0')) break;
	}
	*pbuffer='\0';
	if (n>max-1) {
		if (N->debug) n_warn(N, "tcp_fgets", "[%s:%d] tcp_fgets: %s", socket->RemoteAddr, socket->RemotePort, buffer);
		return n;
	}
	if (!lf) {
		if (socket->recvbufsize>0) {
			memmove(socket->recvbuf, socket->recvbuf+socket->recvbufoffset, socket->recvbufsize);
			memset(socket->recvbuf+socket->recvbufsize, 0, sizeof(socket->recvbuf)-socket->recvbufsize);
			socket->recvbufoffset=0;
		} else {
			memset(socket->recvbuf, 0, sizeof(socket->recvbuf));
			socket->recvbufoffset=0;
			socket->recvbufsize=0;
		}
		goto retry;
	}
	if (N->debug) n_warn(N, "tcp_fgets", "[%s:%d] tcp_fgets: %s", socket->RemoteAddr, socket->RemotePort, buffer);
	return n;
}

int tcp_close(nes_state *N, TCP_SOCKET *socket, short int owner_killed)
{
	if (!owner_killed) {
		socket->want_close=1;
	} else {
		socket->want_close=0;
#ifdef HAVE_SSL
		if (socket->ssl!=NULL) ssl_close(N, socket);
#endif
		if (socket->socket>-1) {
			/* shutdown(x,0=recv, 1=send, 2=both) */
			shutdown(socket->socket, 2);
			closesocket(socket->socket);
			socket->socket=-1;
		}
	}
	return 0;
}

/*
 * start nesla tcp script functions here
 */
int neslatcp_tcp_bind(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1); /* host */
	obj_t *cobj2=nes_getiobj(N, &N->l, 2); /* port */
	TCP_SOCKET *bsock;
	int rc;

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->val->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number for arg2");
	bsock=calloc(1, sizeof(TCP_SOCKET)+1);
	if (bsock==NULL) {
		n_warn(N, "neslatcp_tcp_bind", "couldn't alloc %d crappy bytes", sizeof(TCP_SOCKET)+1);
		return -1;
	}
	strcpy(bsock->obj_type, "sock4");
	bsock->obj_term=(NES_CFREE)tcp_murder;
	if ((rc=tcp_bind(N, cobj1->val->d.str, (unsigned short)cobj2->val->d.num))<0) {
		nes_setstr(N, &N->r, "", "tcp error", strlen("tcp error"));
		n_free(N, (void *)&bsock);
		return -1;
	}
	bsock->socket=rc;
	nes_setcdata(N, &N->r, "", bsock, sizeof(TCP_SOCKET)+1);
	return 0;
}

int neslatcp_tcp_accept(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	TCP_SOCKET *asock;
	TCP_SOCKET *bsock;
	int rc;

	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "sock4")!=0))
		n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a socket for arg1");
	bsock=(TCP_SOCKET *)cobj1->val->d.str;
	asock=calloc(1, sizeof(TCP_SOCKET)+1);
	if (asock==NULL) {
		n_warn(N, "neslatcp_tcp_accept", "couldn't alloc %d crappy bytes", sizeof(TCP_SOCKET)+1);
		return -1;
	}
	strcpy(asock->obj_type, "sock4");
	asock->obj_term=(NES_CFREE)tcp_murder;
	if ((rc=tcp_accept(N, bsock->socket, asock))<0) {
		nes_setstr(N, &N->r, "", "tcp error", strlen("tcp error"));
		n_free(N, (void *)&asock);
		return -1;
	}
	nes_setcdata(N, &N->r, "", asock, sizeof(TCP_SOCKET)+1);
	return 0;
}

int neslatcp_tcp_open(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1); /* host */
	obj_t *cobj2=nes_getiobj(N, &N->l, 2); /* port*/
	obj_t *cobj3=nes_getiobj(N, &N->l, 3); /* SSL */
	TCP_SOCKET *sock;
	int rc;

	sock=calloc(1, sizeof(TCP_SOCKET)+1);
	if (sock==NULL) {
		n_warn(N, "neslatcp_tcp_open", "couldn't alloc %d crappy bytes", sizeof(TCP_SOCKET)+1);
		return -1;
	}
	strcpy(sock->obj_type, "sock4");
	sock->obj_term=(NES_CFREE)tcp_murder;
	if ((rc=tcp_connect(N, sock, cobj1->val->d.str, (unsigned short)cobj2->val->d.num, (unsigned short)cobj3->val->d.num))<0) {
		nes_setstr(N, &N->r, "", "tcp error", strlen("tcp error"));
		n_free(N, (void *)&sock);
		return -1;
	}
	nes_setcdata(N, &N->r, "", sock, sizeof(TCP_SOCKET)+1);
	return 0;
}

int neslatcp_tcp_close(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	TCP_SOCKET *sock;

	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "sock4")!=0))
		n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a socket for arg1");
	sock=(TCP_SOCKET *)cobj1->val->d.str;
	tcp_close(N, sock, 1);
	n_free(N, (void *)&cobj1->val->d.str);
	nes_setnum(N, &N->r, "", 0);
	return 0;
}

int neslatcp_tcp_read(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	TCP_SOCKET *sock;
	int rc;
	char tmpbuf[8192];
	char *p;

	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "sock4")!=0))
		n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a socket for arg1");
	sock=(TCP_SOCKET *)cobj1->val->d.str;
	if (sock->recvbufsize>0) {
		p=sock->recvbuf+sock->recvbufoffset;
		rc=sock->recvbufsize;
		sock->recvbufoffset=0;
		sock->recvbufsize=0;
	} else {
		p=tmpbuf;
		rc=tcp_recv(N, sock, tmpbuf, sizeof(tmpbuf)-1, 0);
	}
	if (rc>-1) {
		nes_setstr(N, &N->r, "", p, rc);
	} else {
		nes_setnum(N, &N->r, "", rc);
	}
	return 0;
}

int neslatcp_tcp_gets(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	TCP_SOCKET *sock;
	int rc;
	char tmpbuf[8192];

	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "sock4")!=0))
		n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a socket for arg1");
	sock=(TCP_SOCKET *)cobj1->val->d.str;
	rc=tcp_fgets(N, tmpbuf, sizeof(tmpbuf)-1, sock);
	if (rc>-1) {
		striprn(tmpbuf);
		nes_setstr(N, &N->r, "", tmpbuf, rc);
	} else {
		nes_setnum(N, &N->r, "", rc);
	}
	return 0;
}

int neslatcp_tcp_write(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	TCP_SOCKET *sock;
	int rc;

	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "sock4")!=0))
		n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a socket for arg1");
	sock=(TCP_SOCKET *)cobj1->val->d.str;
	rc=tcp_send(N, sock, cobj2->val->d.str, cobj2->val->size, 0);
	if (rc>-1) {
		nes_setnum(N, &N->r, "", rc);
	} else {
		nes_setnum(N, &N->r, "", rc);
	}
	return 0;
}
