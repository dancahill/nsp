/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2022 Dan Cahill

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
//#include "nsp/nsp.h"
#include "net.h"
#include "httpd.h"

#ifdef WIN32

/* #define PTHREAD_STACK_SIZE	65536L */
#define PTHREAD_STACK_SIZE	98304L
/* #define PTHREAD_STACK_SIZE	131072L */

int pthread_attr_init(pthread_attr_t *connect_att)
{
	connect_att->dwStackSize = 0;
	connect_att->dwCreatingFlag = 0;
	connect_att->priority = 0;
	return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *connect_att, DWORD stack)
{
	connect_att->dwStackSize = stack;
	return 0;
}

int pthread_attr_setprio(pthread_attr_t *connect_att, int priority)
{
	connect_att->priority = priority;
	return 0;
}

int pthread_attr_destroy(pthread_attr_t *connect_att)
{
	return 0;
}

int pthread_create(pthread_t *thread_id, pthread_attr_t *attr, unsigned(__stdcall *func)(void *), void *param)
{
	HANDLE hThread;
	unsigned long int id;

	hThread = (HANDLE)_beginthreadex(NULL, attr->dwStackSize ? attr->dwStackSize : PTHREAD_STACK_SIZE, func, param, 0, &id);
	if ((long)hThread == -1L) return (errno ? errno : -1);
	*thread_id = hThread;
	return id;
}

int pthread_kill(pthread_t handle, int sig)
{
	int rc;

	TerminateThread(handle, (DWORD)&rc);
	CloseHandle(handle);
	return 0;
}
#endif




/*
char *strncatf(char *dest, int maxlen, const char *format, ...)
{
	char *ptemp;
	va_list ap;

	ptemp = dest + strlen(dest);
	va_start(ap, format);
	vsnprintf(ptemp, maxlen, format, ap);
	va_end(ap);
	ptemp[maxlen - 1] = '\0';
	return dest;
}
*/

static void http_dorequest(CONN *conn)
{
#define __FN__ __FILE__ ":http_dorequest()"
	obj_t *htobj, *hrobj, *tobj, *cobj;
	//TCP_SOCKET *asock;
	//obj_t *cobj;

	//cobj = nsp_getobj(N, &N->g, "_socket");
	//if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (nc_strcmp(cobj->val->d.str, "sock4") != 0))
	//	n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	//sock = (TCP_SOCKET *)cobj->val->d.str;

	if (httpd_read_header(conn) < 0) {
		//closeconnect(conn, 1);
		return;
	}

	//gettimeofday(&conn->dat->runtime, NULL);
	htobj = nsp_getobj(conn->N, &conn->N->g, "_SERVER");
	hrobj = nsp_getobj(conn->N, &conn->N->g, "_HEADER");
	nsp_setstr(conn->N, hrobj, "CONTENT_TYPE", "text/html", -1);

	httpd_read_vardata(conn);

	//n_warn(conn->N, __FN__, "http_dorequest");
	//tcp_fprintf(conn->N, &conn->socket, "HTTP/1.0 200 OK\r\n");
	//tcp_fprintf(conn->N, &conn->socket, "Connection: close\r\n");
	//tcp_fprintf(conn->N, &conn->socket, "Content-Type: text/html\r\n");
	//tcp_fprintf(conn->N, &conn->socket, "\r\n");
	//conn->dat->out_headdone = 1;
	//tcp_fprintf(conn->N, &conn->socket, "this web page was intentionally left blank\r\n");


	httpd_send_header(conn, 0, 200, "1", "text/html", -1, -1);
	{
		char hackbuf[512];

		tobj = nsp_settable(conn->N, &conn->N->g, "httpd");
		obj_t *fobj = nsp_getobj(NULL, conn->httpd, "onAccept");
		if (nsp_typeof(fobj) == NT_NFUNC) {
			//snprintf(hackbuf, sizeof(hackbuf) - 1, "exec(convertnsp(file.readall(\"%s\")));", file);
			cobj = nsp_setstr(conn->N, tobj, "onAccept", fobj->val->d.str, fobj->val->size);
			cobj->val->type = NT_NFUNC;
			snprintf(hackbuf, sizeof(hackbuf) - 1, "httpd.onAccept(42);");
			nsp_exec(conn->N, hackbuf);

			//obj_t *robj = nsp_eval(conn->N, "httpd.onAccept();");
			//if (nsp_isnum(robj)) return (int)nsp_tonum(conn->N, robj);
		}
		//obj_t *fobj = nsp_getobj(conn->N, conn->httpd, "accept");
		//	n_execfunction(conn->N, fobj, NULL, function);
		//}
	}


	httpd_prints(conn, "[%s]", "this page was intentionally left blank");
	conn->dat->out_bodydone = 1;
	httpd_send_header(conn, 0, 200, "1", "text/html", -1, -1);

	htnsp_flush(conn->N);
	httpd_flushbuffer(conn);
#undef __FN__
}

#ifdef WIN32
static unsigned _stdcall httpd_accept_loop(void *x)
#else
static void *httpd_accept_loop(void *x)
#endif
{
#define __FN__ __FILE__ ":httpd_accept_loop()"
	//nsp_state *N = x;
	CONN *conn = x;
	//CONN *conn = calloc(1, sizeof(CONN));
	obj_t *cobj;
	short k;


	//TCP_SOCKET *sock = x;




	//cobj = nsp_getobj(N, &N->g, "_socket");
	//if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (nc_strcmp(cobj->val->d.str, "sock4") != 0))
	//	n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	//asock = (TCP_SOCKET *)cobj->val->d.str;



	//log_error(proc->N, "core", __FILE__, __LINE__, 2, "Starting htloop() thread");
	//if (conn == NULL) {
	//	log_error(proc->N, MODSHORTNAME, __FILE__, __LINE__, 0, "missing sid");
	//	return 0;
	//}
	//conn->id = pthread_self();
#ifndef WIN32
//	pthread_detach(conn->id);
#endif
	//log_error(proc->N, MODSHORTNAME, __FILE__, __LINE__, 4, "Opening connection [%s:%d]", conn->socket.RemoteAddr, conn->socket.RemotePort);
	//proc->stats.http_conns++;
//#ifdef WIN32
//	__try {
//#endif
	for (;;) {
		//conn->N = nsp_newstate();

		//cobj = nsp_setcdata(conn->N, &conn->N->g, "_socket", NULL, 0);
		//cobj->val->d.str = (void *)sock;
		//cobj->val->size = sizeof(TCP_SOCKET) + 1;

		if (conn->dat != NULL) { free(conn->dat); conn->dat = NULL; }
		conn->dat = calloc(1, sizeof(CONNDATA));
		conn->dat->out_ContentLength = -1;
		conn->socket.mtime = time(NULL);
		conn->socket.ctime = time(NULL);
		http_dorequest(conn);

		k = 0;
		cobj = nsp_getobj(conn->N, &conn->N->g, "_HEADER");
		if (cobj->val->type == NT_TABLE) {
			if (strcasecmp("Keep-Alive", nsp_getstr(conn->N, cobj, "CONNECTION")) == 0) k = 1;
		}



		//nc_strncpy(conn->obj_type, "httpconn", sizeof(conn->obj_type) - 1);
		//conn->obj_term = (NSP_CFREE)conn_murder;
		cobj = nsp_getobj(conn->N, &conn->N->g, "CONN");
		cobj->val->d.str = NULL;
		cobj->val->size = 0;



		conn->N = nsp_endstate(conn->N);
		//conn->state = 0;
		/* memset(conn->dat, 0, sizeof(CONNDATA)); */
		if (!k) break;
	}
//#ifdef WIN32
//	} __except (do_filter(GetExceptionInformation(), conn)) {
//	}
//#endif
//	log_error(proc->N, MODSHORTNAME, __FILE__, __LINE__, 4, "Closing connection [%s:%d]", conn->socket.RemoteAddr, conn->socket.RemotePort);
	/* closeconnect() cleans up our mess for us */
	//closeconnect(conn, 2);
	tcp_close(conn->N, &conn->socket, 1);

	//conn->socket.socket = -1;
	pthread_exit(0);
	return 0;
#undef __FN__
}

#ifdef WIN32
static unsigned _stdcall httpd_listener(void *x)
#else
static void *httpd_listener(void *x)
#endif
{
#define __FN__ __FILE__ ":httpd_listener()"
	val_t *thisval = x;
	obj_t *cobj;
	obj_t tobj;
	TCP_SOCKET *bsock;

#ifndef WIN32
	pthread_detach(pthread_self());
#endif
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	tobj.val = thisval;
	if (!nsp_istable((&tobj))) n_error(NULL, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(NULL, &tobj, "_socket");

	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (nc_strcmp(cobj->val->d.str, "sock4") != 0))
		n_error(NULL, NE_SYNTAX, __FN__, "expected a socket");
	bsock = (TCP_SOCKET *)cobj->val->d.str;

	for (;;) {
		pthread_attr_t thr_attr;
		pthread_t ListenThread;
		CONN *conn = calloc(1, sizeof(CONN));
		conn->httpd = &tobj;
		uint64 rc;

		if ((rc = tcp_accept(NULL, bsock, &conn->socket)) < 0) {
			n_free(NULL, (void *)&conn->socket, sizeof(TCP_SOCKET) + 1);
			return 0;
		}
		if (pthread_attr_init(&thr_attr)) {
			//n_warn(N, __FN__, "libnsp_net_http_server_start() pthread_attr_init() error");
			return 0;
		}
#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
		if (pthread_attr_setstacksize(&thr_attr, PTHREAD_STACK_SIZE)) exit(-1);
#endif
		if (pthread_create(&ListenThread, &thr_attr, httpd_accept_loop, conn) == -1) {
			//n_warn(N, __FN__, "libnsp_net_http_server_start() httpd_listener() failed to start...");
			return 0;
		}
	}
	pthread_exit(0);
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_net_http_server_constructor)
{
#define __FN__ __FILE__ ":libnsp_net_http_server_server()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *cobj;
	TCP_SOCKET *sock;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	if ((sock = n_alloc(N, sizeof(TCP_SOCKET) + 1, 1)) == NULL) {
		n_warn(N, __FN__, "couldn't alloc %d bytes", sizeof(TCP_SOCKET) + 1);
		return -1;
	}
	nc_strncpy(sock->obj_type, "sock4", sizeof(sock->obj_type) - 1);
	sock->obj_term = (NSP_CFREE)tcp_murder;
	cobj = nsp_setcdata(N, thisobj, "_socket", NULL, 0);
	cobj->val->d.str = (void *)sock;
	cobj->val->size = sizeof(TCP_SOCKET) + 1;
	//nsp_setbool(N, thisobj, "use_tls", 0);
	nsp_setstr(N, thisobj, "host", "localhost", 9);
	nsp_setnum(N, thisobj, "port", 8080);
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_net_http_server_start)
{
#define __FN__ __FILE__ ":libnsp_net_http_server_start()"
	obj_t *thisobj = nsp_getobj(N, &N->context->l, "this");
	obj_t *cobj;
	TCP_SOCKET *bsock;
	uint64 rc;
	char *host;
	unsigned short port;

	pthread_attr_t thr_attr;
	pthread_t ListenThread;

#ifndef WIN32
	pthread_detach(pthread_self());
#endif
	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	cobj = nsp_getobj(N, thisobj, "_socket");

	if ((cobj->val->type != NT_CDATA) || (cobj->val->d.str == NULL) || (nc_strcmp(cobj->val->d.str, "sock4") != 0))
		n_error(N, NE_SYNTAX, __FN__, "expected a socket");
	bsock = (TCP_SOCKET *)cobj->val->d.str;

	if (!nsp_isstr((cobj = nsp_getobj(N, thisobj, "host")))) n_error(N, NE_SYNTAX, __FN__, "expected a string for host");
	host = nsp_tostr(N, cobj);
	if (!nsp_isnum((cobj = nsp_getobj(N, thisobj, "port")))) n_error(N, NE_SYNTAX, __FN__, "expected a number for port");
	port = (unsigned short)cobj->val->d.num;

	if ((rc = tcp_bind(N, bsock, host, port)) < 0) {
		n_free(N, (void *)&cobj->val->d.str, sizeof(TCP_SOCKET) + 1);
		cobj->val->size = 0;
		nsp_setstr(N, &N->r, "", "tcp error", -1);
		return -1;
	}
	bsock->socket = (int)rc;
	bsock->use_tls = 0;

	if (pthread_attr_init(&thr_attr)) {
		n_warn(N, __FN__, "libnsp_net_http_server_start() pthread_attr_init() error");
		return -1;
	}
#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
	if (pthread_attr_setstacksize(&thr_attr, PTHREAD_STACK_SIZE)) exit(-1);
#endif
	if (pthread_create(&ListenThread, &thr_attr, httpd_listener, thisobj->val) == -1) {
		n_warn(N, __FN__, "libnsp_net_http_server_start() httpd_listener() failed to start...");
		return -1;
	}
	return 0;
#undef __FN__
}

NSP_CLASSMETHOD(libnsp_net_http_server_stop)
{
#define __FN__ __FILE__ ":libnsp_net_http_server_stop()"
	n_warn(N, __FN__, "libnsp_net_http_server_stop()");
	return 0;
#undef __FN__
}
