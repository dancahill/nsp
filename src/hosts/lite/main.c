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
#ifdef HAVE_DL
#include "nesla/libdl.h"
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

#if defined(__BORLANDC__)
extern char **_environ;
#define environ _environ
#else
extern char **environ;
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
static int flush(nes_state *N)
{
	if (N==NULL||N->outbuflen==0) return 0;
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

#define striprn(s) { int n=strlen(s)-1; while (n>-1&&(s[n]=='\r'||s[n]=='\n')) s[n--]='\0'; }

static NES_FUNCTION(neslib_io_gets)
{
	char buf[1024];

	flush(N);
	fgets(buf, sizeof(buf)-1, stdin);
	striprn(buf);
	nes_setstr(N, &N->r, "", buf, -1);
	return 0;
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
		strncat(buf, "/", sizeof(buf)-strlen(buf));
		strncat(buf, name, sizeof(buf)-strlen(buf));
	} else {
		getcwd(buf, sizeof(buf)-strlen(name)-2);
		strncat(buf, "/", sizeof(buf)-strlen(buf));
		strncat(buf, name, sizeof(buf)-strlen(buf));
	}
	for (j=0;j<strlen(buf);j++) {
		if (buf[j]=='\\') buf[j]='/';
	}
	for (j=strlen(buf)-1;j>0;j--) {
		if (buf[j]=='/') { buf[j]='\0'; p=buf+j+1; break; }
	}
	nes_setstr(N, &N->g, "_filename", p, -1);
	nes_setstr(N, &N->g, "_filepath", buf, -1);
	return;
}

void do_banner() {
	printf("\r\nNullLogic Embedded Scripting Language Version " NESLA_VERSION);
	printf("\r\nCopyright (C) 2007-2008 Dan Cahill\r\n\r\n");
	return;
}

void do_help(char *arg0) {
	printf("Usage : %s [-e] [-f] file.nes\r\n", arg0);
	printf("  -e  execute string\r\n");
	printf("  -f  execute file\r\n\r\n");
	return;
}

int main(int argc, char *argv[])
{
	char tmpbuf[MAX_OBJNAMELEN+1];
	obj_t *tobj;
	int i;
	char *p;
	char c;

	setvbuf(stdout, NULL, _IONBF, 0);
	if (argc<2) {
		do_banner();
		do_help(argv[0]);
		return -1;
	}
	if ((N=nes_newstate())==NULL) return -1;
	setsigs();
	N->debug=0;
	/*
	 * registering functions would go here in a normal cli
	 */
#ifdef HAVE_DL
	nesladl_register_all(N);
#endif
	/* add env */
	tobj=nes_settable(N, &N->g, "_ENV");
	for (i=0;environ[i]!=NULL;i++) {
		strncpy(tmpbuf, environ[i], MAX_OBJNAMELEN);
		p=strchr(tmpbuf, '=');
		if (!p) continue;
		*p='\0';
		p=strchr(environ[i], '=')+1;
		nes_setstr(N, tobj, tmpbuf, p, -1);
	}
	/* add args */
	tobj=nes_settable(N, &N->g, "_ARGS");
	for (i=0;i<argc;i++) {
		n_ntoa(N, tmpbuf, i, 10, 0);
		nes_setstr(N, tobj, tmpbuf, argv[i], -1);
	}
	tobj=nes_settable(N, &N->g, "io");
	nes_setcfunc(N, tobj, "gets", (NES_CFUNC)neslib_io_gets);
	for (i=1;i<argc;i++) {
		if (argv[i]==NULL) break;
		if (argv[i][0]=='-') {
			c=argv[i][1];
			if (!c) {
				break;
			} else if ((c=='d')||(c=='D')) {
				N->debug=1;
			} else if ((c=='e')||(c=='E')) {
				if (++i<argc) {
					nes_exec(N, argv[i]);
					if (N->err) goto err;
				}
			} else if ((c=='f')||(c=='F')) {
				if (++i<argc) {
					preppath(N, argv[i]);
					nes_execfile(N, argv[i]);
					if (N->err) goto err;
				}
			} else {
				do_help(argv[0]);
				return -1;
			}
		} else {
			preppath(N, argv[i]);
			nes_execfile(N, argv[i]);
			if (N->err) goto err;
		}
	}
err:
	if (N->err) {
		printf("errno=%d (%d) :: \r\n%s", N->err, N->warnings, N->errbuf);
	}
	nes_endstate(N);
	return 0;
}
