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
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "libnesla.h"

#ifdef WIN32
#define strcasecmp stricmp
#define strncasecmp strnicmp
#else
#ifdef __TURBOC__
#else
#include <unistd.h>
#endif
#endif

/*
 * basic i/o functions
 */
int nl_write(nesla_state *N)
{
	NESLA_CFUNC cfunc=nl_write;
	obj_t *cobj;

	cobj=nesla_getobj(N, &N->g, "io");
	if (((cobj=nesla_getobj(N, cobj, "write"))!=NULL)&&(cobj->type==NT_CFUNC)) {
		cfunc=(NESLA_CFUNC)cobj->d.cfunction;
	}
	if (cfunc!=nl_write) {
		cfunc(N);
	} else {
		fputs(N->txtbuf, stdout);
	}
	return strlen(N->txtbuf);
}

int nl_print(nesla_state *N)
{
	obj_t *cobj;

	for (cobj=N->l.d.table; cobj; cobj=cobj->next) {
		if (strcmp("_retval", cobj->name)==0) continue;
		if (strcmp("!0", cobj->name)==0) continue;
		nesla_tostr(N, cobj);
		nl_write(N);
	}
	return 0;
}

/*
 * math functions
 */
int nl_math1(nesla_state *N)
{
	obj_t *cobj0=nesla_getobj(N, &N->l, "!0");
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");
	num_t n;

	if (strcmp(cobj0->d.string, "rand")==0) {
		n=rand();
		if (cobj1->type==NT_NUMBER) {
			n=fmod(n, cobj1->d.number);
		}
		nesla_regnum(N, &N->g, "_retval", n);
		return 0;
	}
	if (cobj1->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a number");
	if (strcmp(cobj0->d.string, "abs")==0) {
		n=fabs(cobj1->d.number);
	} else if (strcmp(cobj0->d.string, "ceil")==0) {
		n=ceil(cobj1->d.number);
	} else if (strcmp(cobj0->d.string, "floor")==0) {
		n=floor(cobj1->d.number);
	} else if (strcmp(cobj0->d.string, "acos")==0) {
		n=acos(cobj1->d.number);
	} else if (strcmp(cobj0->d.string, "asin")==0) {
		n=asin(cobj1->d.number);
	} else if (strcmp(cobj0->d.string, "atan")==0) {
		n=atan(cobj1->d.number);
	} else if (strcmp(cobj0->d.string, "cos")==0) {
		n=cos(cobj1->d.number);
	} else if (strcmp(cobj0->d.string, "sin")==0) {
		n=sin(cobj1->d.number);
	} else if (strcmp(cobj0->d.string, "tan")==0) {
		n=tan(cobj1->d.number);
	} else if (strcmp(cobj0->d.string, "sqrt")==0) {
		n=sqrt(cobj1->d.number);
	} else {
		n=0;
	}
	nesla_regnum(N, &N->g, "_retval", n);
	return 0;
}

int nl_number(nesla_state *N)
{
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");
	num_t n=0;

	if (cobj1->type==NT_NUMBER) {
		n=cobj1->d.number;
	} else if (cobj1->type==NT_STRING) {
		n=atof(cobj1->d.string);
	}
	nesla_regnum(N, &N->g, "_retval", n);
	return 0;
}

/*
 * string functions
 */
int nl_strcat(nesla_state *N)
{
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");
	obj_t *cobj2=nesla_getobj(N, &N->l, "!2");
	obj_t *robj;
	char *p;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a string for arg1");
	if (cobj2->type!=NT_STRING) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a string for arg2");
	p=n_alloc(N, (cobj1->size+cobj2->size+1)*sizeof(char));
	if (p==NULL) return -1;
	strncpy(p, cobj1->d.string, cobj1->size);
	strncpy(p+cobj1->size, cobj2->d.string, cobj2->size);
	robj=nesla_regstr(N, &N->g, "_retval", NULL);
	robj->d.string=p;
	robj->size=cobj1->size+cobj2->size;
	return 0;
}

int nl_strcmp(nesla_state *N)
{
	obj_t *cobj0=nesla_getobj(N, &N->l, "!0");
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");
	obj_t *cobj2=nesla_getobj(N, &N->l, "!2");
	obj_t *cobj3=nesla_getobj(N, &N->l, "!3");
	int i=0;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a string for arg1");
	if (cobj2->type!=NT_STRING) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a string for arg2");
	if (strcmp(cobj0->d.string, "cmp")==0) {
		i=strcmp(cobj1->d.string, cobj2->d.string);
	} else if (strcmp(cobj0->d.string, "icmp")==0) {
		i=strcasecmp(cobj1->d.string, cobj2->d.string);
	} else if (strcmp(cobj0->d.string, "ncmp")==0) {
		if (cobj3->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a number for arg3");
		i=strncmp(cobj1->d.string, cobj2->d.string, (int)cobj3->d.number);
	} else if (strcmp(cobj0->d.string, "nicmp")==0) {
		if (cobj3->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a number for arg3");
		i=strncasecmp(cobj1->d.string, cobj2->d.string, (int)cobj3->d.number);
	}
	nesla_regnum(N, &N->g, "_retval", i);
	return i;
}

int nl_strlen(nesla_state *N)
{
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a string for arg1");
	nesla_regnum(N, &N->g, "_retval", cobj1->size);
	return cobj1->size;
}

int nl_strstr(nesla_state *N)
{
	obj_t *cobj0=nesla_getobj(N, &N->l, "!0");
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");
	obj_t *cobj2=nesla_getobj(N, &N->l, "!2");
	obj_t *robj;
	char *p;
	unsigned int i=0, j=0;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a string for arg1");
	if (cobj2->type!=NT_STRING) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a string for arg2");
	if (cobj2->size<1) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "zero length arg2");
	if (strcmp(cobj0->d.string, "str")==0) {
		for (i=0,j=0;i<cobj1->size;i++,j++) {
			if (j==cobj2->size) break;
			if (cobj2->d.string[j]=='\0') { j=0; break; }
			if (cobj2->d.string[j]!=cobj1->d.string[i]) j=-1;
		}
	} else if (strcmp(cobj0->d.string, "istr")==0) {
		for (i=0,j=0;i<cobj1->size;i++,j++) {
			if (j==cobj2->size) break;
			if (cobj2->d.string[j]=='\0') { j=0; break; }
			if (tolower(cobj2->d.string[j])!=tolower(cobj1->d.string[i])) j=-1;
		}
	} else {
		n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "................");
	}
	if ((i<cobj1->size)&&(j==cobj2->size)) {
		i=i-j;
		if ((p=n_alloc(N, (cobj1->size-i+1)*sizeof(char)))==NULL) return -1;
		strncpy(p, cobj1->d.string+i, cobj1->size-i);
		robj=nesla_regstr(N, &N->g, "_retval", NULL);
		robj->d.string=p;
		robj->size=i;
	} else {
		robj=nesla_regstr(N, &N->g, "_retval", NULL);
	}
	return i;
}

int nl_strsub(nesla_state *N)
{
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");
	obj_t *cobj2=nesla_getobj(N, &N->l, "!2");
	obj_t *cobj3=nesla_getobj(N, &N->l, "!3");
	obj_t *robj;
	unsigned int offset, max=0;
	char *p;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a string for arg1");
	if (cobj2->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a number for arg2");
	if (cobj2->d.number<0) {
		offset=cobj1->size-abs((int)cobj2->d.number);
	} else {
		offset=(int)cobj2->d.number;
	}
	if (cobj3->type==NT_NUMBER) max=(int)cobj3->d.number;
	if (offset>cobj1->size) offset=cobj1->size;
	if (max>cobj1->size-offset) max=cobj1->size-offset;
	p=n_alloc(N, (max+1)*sizeof(char));
	strncpy(p, cobj1->d.string+offset, max);
	robj=nesla_regstr(N, &N->g, "_retval", NULL);
	robj->d.string=p;
	robj->size=max;
	return 0;
}

int nl_datetime(nesla_state *N)
{
	obj_t *cobj0=nesla_getobj(N, &N->l, "!0");
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");
	char timebuf[64];
	time_t t;

	if (cobj0->type!=NT_STRING) return 0;
	if (cobj1->type==NT_NUMBER) {
		t=(time_t)cobj1->d.number;
	} else {
		t=time(NULL);
	}
	memset(timebuf, 0, sizeof(timebuf));
	if (strcmp(cobj0->d.string, "date")==0) {
		strftime(timebuf, sizeof(timebuf)-1, "%Y-%m-%d", localtime(&t));
	} else if (strcmp(cobj0->d.string, "time")==0) {
		strftime(timebuf, sizeof(timebuf)-1, "%H:%M:%S", localtime(&t));
	}
	nesla_regstr(N, &N->g, "_retval", timebuf);
	return 0;	
}

int nl_runtime(nesla_state *N)
{
	struct timeval ttime;
	int totaltime;
	num_t n;

	gettimeofday(&ttime, NULL);
	totaltime=((ttime.tv_sec-N->ttime.tv_sec)*1000000)+(ttime.tv_usec-N->ttime.tv_usec);
	n=(num_t)totaltime/1000000;
	nesla_regnum(N, &N->g, "_retval", n);
	return 0;
}

int nl_sleep(nesla_state *N)
{
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");
	int n=1;

	if (cobj1->type==NT_NUMBER) n=(int)cobj1->d.number;
#ifdef WIN32
		_sleep(n);
#else
		sleep(n);
#endif
	return 0;
}

int nl_include(nesla_state *N)
{
	obj_t *cobj;
	char *ptemp;

	if ((cobj=nesla_getobj(N, &N->l, "!1"))!=NULL) {
		ptemp=N->readptr;
		nesla_execfile(N, cobj->d.string);
		N->readptr=ptemp;
	}
	return 0;
}
