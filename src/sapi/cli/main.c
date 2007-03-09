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
#include "nesla.h"
#include "libneslaext.h"
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
		printf("SIGSEGV [%d] Segmentation Violation\r\n", sig);
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

int main(int argc, char *argv[], char *envp[])
{
	char tmpnam[MAX_OBJNAMELEN+1];
	obj_t *tobj;
	int i;
	char *p;

	setvbuf(stdout, NULL, _IONBF, 0);
	if (argc<2) {
		printf("\r\nNullLogic Embedded Scripting Language Version " NESLA_VERSION);
		printf("\r\nCopyright (C) 2007 Dan Cahill\r\n\r\n");
		printf("\tno script was specified.  go away.\r\n\r\n");
		return -1;
	}
	if ((N=nes_newstate())==NULL) return -1;
	setsigs();
	N->debug=0;
	neslaext_register_all(N);
	/* add args */
	tobj=nes_settable(N, &N->g, "_ARGS");
	for (i=0;i<argc;i++) {
		sprintf(tmpnam, "%d", i);
		nes_setstr(N, tobj, tmpnam, argv[i], strlen(argv[i]));
	}
	/* add env */
	tobj=nes_settable(N, &N->g, "_ENV");
	for (i=0;envp[i]!=NULL;i++) {
		strncpy(tmpnam, envp[i], MAX_OBJNAMELEN);
		p=strchr(tmpnam, '=');
		if (!p) continue;
		*p='\0';
		p=strchr(envp[i], '=')+1;
		nes_setstr(N, tobj, tmpnam, p, strlen(p));
	}
	preppath(N, argv[1]);
	if (setjmp(N->savjmp)==0) {
		nes_execfile(N, argv[1]);
	}
	if (N->err) printf("errno=%d (%d) :: \r\n%s", N->err, N->warnings, N->errbuf);
	nes_endstate(N);
	return 0;
}
