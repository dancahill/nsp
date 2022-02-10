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
#include <stdlib.h>
#ifdef WIN32
#include <stdio.h>
//#define vsnprintf _vsnprintf
#else
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#endif


#define MAX_FIELD_SIZE		32768
#define MAX_REPLYSIZE		32768 /* arbitrary 32 KB limit for http reply buffering */

#define SERVER_NAME	"NullLogic GroupServer"
#define PACKAGE_VERSION "1.3.30"

//#if defined(CONFIG_HAVE_THREADS) && !defined(HAVE_THREADS)
#define HAVE_THREADS
//#endif

#ifdef HAVE_THREADS

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#endif

#ifdef WIN32
/* pthread_ typedefs */
typedef HANDLE pthread_t;
typedef struct thread_attr {
	DWORD dwStackSize;
	DWORD dwCreatingFlag;
	int priority;
} pthread_attr_t;
typedef struct {
	int dummy;
} pthread_condattr_t;
typedef unsigned int uint;
typedef struct {
	uint waiting;
	HANDLE semaphore;
} pthread_cond_t;
typedef CRITICAL_SECTION pthread_mutex_t;
#endif
#ifdef WIN32
/* The following is to emulate the posix thread interface */
#define pthread_mutex_init(A,B)  InitializeCriticalSection(A)
#define pthread_mutex_lock(A)    (EnterCriticalSection(A),0)
#define pthread_mutex_unlock(A)  LeaveCriticalSection(A)
#define pthread_mutex_destroy(A) DeleteCriticalSection(A)
#define pthread_handler_decl(A,B) unsigned __cdecl A(void *B)
#define pthread_self() GetCurrentThreadId()
#define pthread_exit(A) _endthread()
typedef unsigned(__cdecl *pthread_handler)(void *);
int pthread_attr_init(pthread_attr_t *connect_att);
int pthread_attr_setstacksize(pthread_attr_t *connect_att, DWORD stack);
int pthread_attr_setprio(pthread_attr_t *connect_att, int priority);
int pthread_attr_destroy(pthread_attr_t *connect_att);
int pthread_create(pthread_t *thread_id, pthread_attr_t *attr, unsigned(__stdcall *func)(void *), void *param);
int pthread_kill(pthread_t handle, int sig);
#endif
#endif /* HAVE_THREADS */

typedef struct {
	struct timeval runtime;
	/* USER DATA */
	char       username[64];
	char       token[64];
	char       domainname[64];
	short int  uid;
	short int  gid;
	short int  did;
	short int  maxlist;
	short int  timezone;
	char       language[4];
	char       theme[40];
	//EMAIL      *wm;
	/* OUTGOING DATA */
	short int out_status;
	short int out_headdone;
	short int out_bodydone;
	short int out_flushed;
	int       out_bytecount;
	int       out_ContentLength;
	/* BUFFERS */
	short int lastbuf;
	char      smallbuf[4][4096];
	char      largebuf[MAX_FIELD_SIZE];
	char      replybuf[MAX_REPLYSIZE];
	unsigned long replybuflen;
} CONNDATA;
typedef struct {
	/* standard header info for CDATA object */
	char      obj_type[16]; /* tell us all about yourself in 15 characters or less */
	NSP_CFREE obj_term;     /* now tell us how to kill you */
	/* now the rest of the struct */
	pthread_t handle;
#ifdef WIN32
	unsigned int id;
#else
	pthread_t id;
#endif
	short int state;
	TCP_SOCKET socket;
	nsp_state *N;
	obj_t *httpd;
	CONNDATA *dat;
} CONN;


/* httpd_http.c */
void httpd_read_postdata(CONN *conn);
int httpd_read_vardata(CONN *conn);
int httpd_read_header(CONN *conn);
void httpd_send_header(CONN *conn, int cacheable, int status, char *extra_header, char *mime_type, int length, time_t mod);
/* httpd_io.c */
void httpd_flushbuffer(CONN *conn);
int httpd_prints(CONN *conn, const char *format, ...);
/* httpd_nsp.c */
int htnsp_flush(nsp_state *N);
int htnsp_initenv(CONN *conn);
