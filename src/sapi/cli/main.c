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
#include "config.h"
#include "libneslaext.h"
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <direct.h>
#include <process.h>
#else
#include <stdlib.h>
#ifdef __TURBOC__
#else
#include <unistd.h>
#endif
#endif
static void preppath(nesla_state *N, char *name)
{
	char buf[1024];
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
	nesla_regstr(N, &N->g, "_filename", p);
	nesla_regstr(N, &N->g, "_filepath", buf);
	return;
}

int main(int argc, char *argv[])
{
	nesla_state *N;
	int i;

	setvbuf(stdout, (char *)NULL, _IONBF, 0);
	printf("\r\nNullLogic Embedded Scripting Language Version " PACKAGE_VERSION);
	printf("\r\nCopyright (C) 2007 Dan Cahill\r\n\r\n");
	if (argc<2) return -1;
	if ((N=nesla_newstate())==NULL) return -1;
	N->debug=0;
/*	nesla_regcfunc(N, &N->g, "system", (void *)neslaext_system); */
	neslaext_register_all(N);
	if (setjmp(N->savjmp)==0) {
		for (i=1;i<argc;i++) {
			preppath(N, argv[i]);
			nesla_execfile(N, argv[i]);
		}
	}
	if (N->err) printf("errno=%d (%d) :: \r\n%s", N->err, N->warnings, N->errbuf);
	nesla_endstate(N);
	return 0;
}
