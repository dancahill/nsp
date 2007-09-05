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
#include "nesla/nesla.h"
#ifdef HAVE_CRYPTO
#include "nesla/libcrypt.h"
#endif
#ifdef HAVE_DL
#include "nesla/libdl.h"
#endif
#include "nesla/libext.h"
#ifdef HAVE_LDAP
#include "nesla/libldap.h"
#endif
#ifdef HAVE_MATH
#include "nesla/libmath.h"
#endif
#ifdef HAVE_MYSQL
#include "nesla/libmysql.h"
#endif
#ifdef HAVE_ODBC
#include "nesla/libodbc.h"
#endif
#ifdef HAVE_SQLITE3
#include "nesla/libsqlite3.h"
#endif
#ifndef __TURBOC__
#include "nesla/libtcp.h"
#endif
#ifdef HAVE_ZLIB
#include "nesla/libzip.h"
#endif
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <direct.h>
#include <io.h>
#include <process.h>
#else
#include <stdlib.h>
#ifdef __TURBOC__
#else
#include <unistd.h>
#endif
#endif

#include <signal.h>
nes_state *N;

extern char **environ;

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
static int flush(nes_state *N)
{
	N->outbuf[N->outbuflen]='\0';
	write(STDOUT_FILENO, N->outbuf, N->outbuflen);
	N->outbuflen=0;
	return 0;
}

static void sig_trap(int sig)
{
	flush(N); /* if we die here, we should flush the buffer first */
	switch (sig) {
	case 11:
		printf("Segmentation Violation\r\n");
		if ((N)&&(N->readptr)) printf("[%.40s]\r\n", N->readptr);
		exit(-1);
	default:
		printf("Unexpected signal [%d] received\r\n", sig);
	}
}

static void setsigs(void)
{
#ifdef WIN32
	signal(SIGSEGV, sig_trap);
#else
#ifndef __TURBOC__
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler=sig_trap;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGSEGV, &sa, NULL);
#endif
#endif
	return;
}

static void preppath(nes_state *N, char *name)
{
	char buf[1024];
	char *p;
	unsigned int j;

	p=name;
	if ((name[0]=='/')||(name[0]=='\\')||(name[1]==':')) {
		/* it's an absolute path.... probably... */
		strncpy(buf, name, sizeof(buf));
	} else if (name[0]=='.') {
		/* looks relative... */
		getcwd(buf, sizeof(buf)-strlen(name)-2);
		strcat(buf, "/");
		strcat(buf, name);
	} else {
		getcwd(buf, sizeof(buf)-strlen(name)-2);
		strcat(buf, "/");
		strcat(buf, name);
	}
	for (j=0;j<strlen(buf);j++) {
		if (buf[j]=='\\') buf[j]='/';
	}
	for (j=strlen(buf)-1;j>0;j--) {
		if (buf[j]=='/') { buf[j]='\0'; p=buf+j+1; break; }
	}
	nes_setstr(N, &N->g, "_filename", p, strlen(p));
	nes_setstr(N, &N->g, "_filepath", buf, strlen(buf));
	return;
}

int main(int argc, char *argv[])
{
	char tmpbuf[MAX_OBJNAMELEN+1];
	obj_t *cobj, *tobj;
	int i;
	char *p;

	setvbuf(stdout, NULL, _IONBF, 0);
/*
	if (argc<2) {
		printf("\r\nNullLogic Embedded Scripting Language Version " NESLA_VERSION);
		printf("\r\nCopyright (C) 2007 Dan Cahill\r\n\r\n");
		printf("\tno script was specified.  go away.\r\n\r\n");
		return -1;
	}
*/
	if ((N=nes_newstate())==NULL) return -1;
	setsigs();
	N->debug=0;
#ifdef HAVE_CRYPTO
	neslacrypto_register_all(N);
#endif
#ifdef HAVE_DL
	nesladl_register_all(N);
#endif
	neslaext_register_all(N);
#ifdef HAVE_LDAP
	neslaldap_register_all(N);
#endif
#ifdef HAVE_MATH
	neslamath_register_all(N);
#endif
#ifdef HAVE_MYSQL
	neslamysql_register_all(N);
#endif
#ifdef HAVE_ODBC
	neslaodbc_register_all(N);
#endif
#ifdef HAVE_SQLITE3
	neslasqlite3_register_all(N);
#endif
#ifndef __TURBOC__
#ifndef TINYCC
	neslatcp_register_all(N);
#endif
#endif
#ifdef HAVE_ZLIB
	neslazip_register_all(N);
#endif
	/* add env */
	tobj=nes_settable(N, &N->g, "_ENV");
	for (i=0;environ[i]!=NULL;i++) {
		strncpy(tmpbuf, environ[i], MAX_OBJNAMELEN);
		p=strchr(tmpbuf, '=');
		if (!p) continue;
		*p='\0';
		p=strchr(environ[i], '=')+1;
		nes_setstr(N, tobj, tmpbuf, p, strlen(p));
	}
	/* add args */
	tobj=nes_settable(N, &N->g, "_ARGS");
	for (i=0;i<argc;i++) {
		sprintf(tmpbuf, "%d", i);
		nes_setstr(N, tobj, tmpbuf, argv[i], strlen(argv[i]));
	}

	/* BEGIN CRASH TESTS */
	tobj=nes_settable(N, &N->g, "_TEST1");
	cobj=nes_evalf(N, "{ name='[SHOW TABLES] (SQLITE)', query='SELECT tbl_name FROM sqlite_master WHERE type = \\'table\\'' }");
	nes_linkval(N, tobj, cobj);
	if (N->err) { printf("errno=%d :: \r\n%s\r\n", N->err, N->errbuf); N->err=0; }
	cobj=nes_getobj(N, tobj, "query");
	tobj=nes_settable(N, &N->g, "_TEST2");
	cobj=nes_evalf(N, "{ a=' }");
	nes_linkval(N, tobj, cobj);
	if (N->err) { printf("errno=%d :: \r\n%s\r\n", N->err, N->errbuf); N->err=0; }
	nes_exec(N, "print(_TEST1['query'],\"\\n\");");
	nes_exec(N, " asdf laiur oiqwur9yh ishdzfu z98xfnszd9 m9sdf7 nasdfyq90E RUIZJZXCH GFKJSAEHF WEYR 8768&SZTdg f98as 8fbsa");
	if (N->err) { printf("errno=%d :: \r\n%s\r\n", N->err, N->errbuf); N->err=0; }
	/*
	 * This block repeatedly feeds the parser random strings of garbage.
	 * If this program doesn't segfault, the test is a success.
	 * BTW.  The output _will_ be ugly.
	 */
	N->strict=0;
	{
		char xbuf[512];
		int j;

		for (i=0;;i++) {
			for (j=0;j<512;j++) { xbuf[j]=rand(); }
			xbuf[511]=0;
			printf("\n%d ", i);
			nes_exec(N, xbuf);
			if (N->err) {
				N->err=0;
				nes_endstate(N);
				if ((N=nes_newstate())==NULL) return -1;
			}
		}
		printf("\r\ndone random feed crashtest.\r\n");
	}
	tobj=nes_settable(N, &N->g, "_TEST1");
	nes_exec(N, "printvar();");
	/* END CRASH TESTS */

	if (argc>1) {
		preppath(N, argv[1]);
		if (setjmp(N->savjmp)==0) {
			N->jmpset=1;
			nes_execfile(N, argv[1]);
		}
		N->jmpset=0;
	}
	if (N->err) printf("errno=%d (%d) :: \r\n%s", N->err, N->warnings, N->errbuf);
	nes_endstate(N);
	return 0;
}
