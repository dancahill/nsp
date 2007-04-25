/*
    nesla.cgi -- simple Nesla CGI host
    Copyright (C) 2000-2007 Dan Cahill

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
#include "main.h"
#ifdef WIN32
#include <direct.h>
#endif
#include <fcntl.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

nes_state *N;

static int nescgi_flush(nes_state *N)
{
	static short headersent=0;

	if (N->outbuflen==0) return 0;
	if (!headersent) {
		send_header(0, "text/html", -1, -1);
		headersent=1;
	}
	N->outbuf[N->outbuflen]='\0';
	write(STDOUT_FILENO, N->outbuf, N->outbuflen);
	N->outbuflen=0;
	return 0;
}

static int nescgi_sendfile(nes_state *N)
{
	char tmppath[512];
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *headobj=nes_settable(N, &N->g, "_HEADER");
	obj_t *cobj, *robj;
	struct stat sb;
	int bl;
	int fd;
	int r;
	char *p;

	if ((cobj1->val->type!=NT_STRING)||(cobj1->val->d.str==NULL)) {
		send_header(0, "text/html", -1, -1);
		printf("invalid filename [%s]\r\n\r\n", nes_tostr(N, cobj1));
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
	if (stat(cobj1->val->d.str, &sb)!=0) {
		send_header(0, "text/html", -1, -1);
		printf("couldn't stat [%s]\r\n\r\n", cobj1->val->d.str);
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
	if ((fd=open(cobj1->val->d.str, O_RDONLY|O_BINARY))<0) {
		send_header(0, "text/html", -1, -1);
		printf("couldn't open [%s]\r\n\r\n", cobj1->val->d.str);
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
	cobj=nes_getobj(N, headobj, "CONTENT_DISPOSITION");
	if (cobj->val->type==NT_NULL) {
		if ((p=strrchr(cobj1->val->d.str, '/'))!=NULL) p++; else p=cobj1->val->d.str;
		snprintf(tmppath, sizeof(tmppath)-1, "attachment; filename=\"%s\"", p);
		nes_setstr(N, headobj, "CONTENT_DISPOSITION", tmppath, strlen(tmppath));
	}
	cobj=nes_getobj(N, headobj, "CONTENT_LENGTH");
	if (cobj->val->type==NT_NULL) {
		nes_setnum(N, headobj, "CONTENT_LENGTH", sb.st_size);
	}
	cobj=nes_getobj(N, headobj, "CONTENT_TYPE");
	p=NULL;
	if (cobj->val->type==NT_NULL) {
		p=get_mime_type(cobj1->val->d.str);
		nes_setstr(N, headobj, "CONTENT_TYPE", p, strlen(p));
	}
	send_header(0, p, sb.st_size, sb.st_mtime);
	bl=sb.st_size;
	p=malloc(8192);
	for (;;) {
		r=read(fd, p, bl<8192?bl:8192);
		write(STDOUT_FILENO, p, r);
		bl-=r;
		if (bl<1) break;
	}
	free(p);
	close(fd);
	robj=nes_setnum(N, &N->r, "", sb.st_size);
	return 0;
}

static void preppath(nes_state *N, char *name)
{
	char buf[512];
	char *p;
	unsigned int j;

	p=name;
	if ((name[0]=='/')||(name[0]=='\\')||(name[1]==':')) {
		/* it's an absolute path.... probably... */
		strncpy(buf, name, sizeof(buf)-1);
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

static void sig_timeout()
{
	send_header(0, "text/html", -1, -1);
	printf("<B>Connection timed out.</B>\n");
#ifndef WIN32
	if (nes_getstr(N, nes_settable(N, &N->g, "_CONFIG"), "use_syslog")[0]=='y') closelog();
#endif
	exit(0);
}

static void sig_catchint(int sig)
{
	send_header(0, "text/html", -1, -1);
	nescgi_flush(N);
	printf("<B>Caught signal %d.</B>\n", sig);
#ifndef WIN32
	if (nes_getstr(N, nes_settable(N, &N->g, "_CONFIG"), "use_syslog")[0]=='y') closelog();
#endif
	exit(0);
}

static void setsigs()
{
#ifdef _NSIG
	short int numsigs=_NSIG;
#else
	short int numsigs=NSIG;
#endif
	short int i;

	if (numsigs>16) numsigs=16;
	for(i=0;i<numsigs;i++) signal(i, sig_catchint);
#ifdef WIN32
	SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
#else
	signal(SIGALRM, sig_timeout);
	alarm(nes_getnum(N, nes_settable(N, &N->g, "_CONFIG"), "max_runtime"));
#endif
}

int htnes_runscript(char *file)
{
	nes_setcfunc(N, nes_settable(N, &N->g, "io"), "flush", (void *)nescgi_flush);
	nes_setcfunc(N, &N->g, "sendfile", (void *)nescgi_sendfile);
	preppath(N, file);
	nes_execfile(N, file);
	if (N->err) printf("<HR /><B>[errno=%d :: %s]</B>\r\n", N->err, N->errbuf);
	return 0;
}

int main(int argc, char *argv[], char *envp[])
{
	char tmpbuf[MAX_OBJNAMELEN+1];
	obj_t *cobj, *confobj, *headobj, *servobj, *tobj;
	char *PathTranslated;
	char *p;
	int i;
	struct stat sb;

	if (getenv("REQUEST_METHOD")==NULL) {
		printf("This program must be run as a CGI.\r\n");
		exit(0);
	}
	if ((N=nes_newstate())==NULL) {
		printf("nes_newstate() error\r\n");
		return -1;
	}
	confobj=nes_settable(N, &N->g, "_CONFIG");
	servobj=nes_settable(N, &N->g, "_SERVER");
	headobj=nes_settable(N, &N->g, "_HEADER");
#ifdef HAVE_MATH
	neslamath_register_all(N);
#endif
	neslaext_register_all(N);
	neslatcp_register_all(N);
	/* add env */
	for (i=0;envp[i]!=NULL;i++) {
		strncpy(tmpbuf, envp[i], MAX_OBJNAMELEN);
		p=strchr(tmpbuf, '=');
		if (!p) continue;
		*p='\0';
		p=strchr(envp[i], '=')+1;
		nes_setstr(N, servobj, tmpbuf, p, strlen(p));
	}
	/* add args */
	tobj=nes_settable(N, &N->g, "_ARGS");
	for (i=0;i<argc;i++) {
		sprintf(tmpbuf, "%d", i);
		nes_setstr(N, tobj, tmpbuf, argv[i], strlen(argv[i]));
	}
	config_read();
	setsigs();
	setvbuf(stdout, NULL, _IONBF, 0);
#ifdef WIN32
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#else
	if (nes_getstr(N, confobj, "use_syslog")[0]=='y') openlog("nesla.cgi", LOG_PID, LOG_MAIL);
#endif
	cgi_readenv();
	PathTranslated=nes_getstr(N, servobj, "PATH_TRANSLATED");
	if ((PathTranslated)&&(strlen(PathTranslated))) {
		p=nes_getstr(N, servobj, "SCRIPT_NAME");
		nes_setstr(N, servobj, "SCRIPT_NAME_ORIG", p, strlen(p));
		cobj=nes_getobj(N, servobj, "PATH_INFO");
		if ((cobj->val->type!=NT_STRING)||(cobj->val->d.str==NULL)) {
			send_header(0, "text/html", -1, -1);
			printf("no PATH_INFO<BR>");
			goto err;
		}
		p=nes_tostr(N, cobj);
		nes_setstr(N, servobj, "SCRIPT_NAME", p, strlen(p));
		if (stat(PathTranslated, &sb)!=0) {
			send_header(0, "text/html", -1, -1);
			printf("no script found<BR>");
			goto err;
		}
		htnes_runscript(PathTranslated);
	} else {
err:
		send_header(0, "text/html", -1, -1);
		/* nes_exec(N, "print('<pre>');printvars();print('</pre>');"); */
		printf("Unknown command");
	}
	fflush(stdout);
#ifndef WIN32
	if (nes_getstr(N, confobj, "use_syslog")[0]=='y') closelog();
#endif
	N=nes_endstate(N);
	return 0;
}
