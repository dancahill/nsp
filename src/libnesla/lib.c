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
#include "libnesla.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#ifdef WIN32
#include <io.h>
#else
#ifndef __TURBOC__
#include <unistd.h>
#endif
#endif

static int dumpwrite(nes_state *N, const char *str, int len)
{
	int i;

	for (i=0;i<len;i++) {
		if (MAX_OUTBUFLEN-N->outbuflen<32) { nl_flush(N); continue; }
		switch (str[i]) {
		case '\"':
			N->outbuf[N->outbuflen++]='\\';
			N->outbuf[N->outbuflen++]='\"';
			break;
/*
		case '\'':
			N->outbuf[N->outbuflen++]='\\';
			N->outbuf[N->outbuflen++]='\'';
			break;
*/
		default:
			N->outbuf[N->outbuflen++]=str[i];
		}
	}
	N->outbuf[N->outbuflen]='\0';
	return len;
}
static void dumpvars(nes_state *N, obj_t *tobj, int depth)
{
	obj_t *cobj=tobj;
	char indent[20];
	int i;
	char b;
	char *g;
	char *l;

	for (i=0;i<depth;i++) indent[i]='\t';
	indent[i]='\0';
	for (;cobj;cobj=cobj->next) {
		if ((cobj->mode&NST_HIDDEN)||(cobj->mode&NST_SYSTEM)) continue;
		if ((depth==1)&&(nc_strcmp(cobj->name, "filename")==0)) continue;
		g=(depth<1)?"global ":"";
		l=(depth<1)?";":(cobj->next)?",":"";
		if (nc_isdigit(cobj->name[0])) b=1; else b=0;
		if ((cobj->type==NT_NULL)||(cobj->type==NT_BOOLEAN)||(cobj->type==NT_NUMBER)) {
			nc_printf(N, "%s%s%s%s%s = ", indent, g, b?"[":"", cobj->name, b?"]":"");
			nc_printf(N, "%s%s\n", nes_tostr(N, cobj), l);
		} else if (cobj->type==NT_STRING) {
			nc_printf(N, "%s%s%s%s%s = \"", indent, g, b?"[":"", cobj->name, b?"]":"");
			dumpwrite(N, cobj->d.str?cobj->d.str:"", cobj->size);
			nc_printf(N, "\"%s\n", l);
		} else if (cobj->type==NT_TABLE) {
			if (nc_strcmp(cobj->name, "_globals_")==0) continue;
			nc_printf(N, "%s%s%s%s%s = {\n", indent, g, b?"[":"", cobj->name, b?"]":"");
			dumpvars(N, cobj->d.table, depth+1);
			nc_printf(N, "%s}%s\n", indent, l);
		}
	}
	nl_flush(N);
	return;
}

void n_printvars(nes_state *N, obj_t *tobj)
{
	dumpvars(N, tobj, 0);
}

/*
 * basic i/o functions
 */
/* windows is retarded..  from unistd.. */
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
int nl_flush(nes_state *N)
{
	obj_t *cobj=nes_getobj(N, &N->g, "io");
	NES_CFUNC cfunc=nl_flush;

	if (cobj->type==NT_TABLE) {
		cobj=nes_getobj(N, cobj, "flush");
		if (cobj->type==NT_CFUNC) {
			if ((cfunc=(NES_CFUNC)cobj->d.cfunc)!=nl_flush) {
				cfunc(N);
				return 0;
			}
		}
	}
	N->outbuf[N->outbuflen]='\0';
	/* fputs(N->outbuf, stdout); */
	write(STDOUT_FILENO, N->outbuf, N->outbuflen);
	N->outbuflen=0;
	return 0;
}

int nl_print(nes_state *N)
{
	obj_t *cobj=N->l.d.table;
	unsigned int len;
	unsigned int i;
	char e=0;
	char *p;

	if (N->outbuflen>OUTBUFLOWAT) nl_flush(N);
	if ((cobj==NULL)||(nc_strcmp("0", cobj->name)!=0)) return -1;
	for (cobj=cobj->next; cobj; cobj=cobj->next) {
		if (cobj->type!=NT_STRING) {
			p=nes_tostr(N, cobj);
			len=nc_strlen(p);
			for (i=0;i<len;i++) {
				N->outbuf[N->outbuflen++]=p[i];
			}
			N->outbuf[N->outbuflen]='\0';
			continue;
		}
		for (i=0;i<cobj->size;i++) {
			if (!e) {
				if (cobj->d.str[i]=='\\') {
					e=1;
				} else {
					N->outbuf[N->outbuflen++]=cobj->d.str[i];
				}
				continue;
			}
			switch (cobj->d.str[i]) {
				case 'a'  : N->outbuf[N->outbuflen++]='\a'; break;
				case 't'  : N->outbuf[N->outbuflen++]='\t'; break;
				case 'f'  : N->outbuf[N->outbuflen++]='\f'; break;
				case 'e'  : N->outbuf[N->outbuflen++]=27;   break;
				case 'r'  : N->outbuf[N->outbuflen++]='\r'; break;
				case 'n'  : N->outbuf[N->outbuflen++]='\n'; break;
				case '\'' : N->outbuf[N->outbuflen++]='\''; break;
				case '\"' : N->outbuf[N->outbuflen++]='\"'; break;
				case '\\' : N->outbuf[N->outbuflen++]='\\'; break;
				default   : break;
			}
			e=0;
		}
		N->outbuf[N->outbuflen]='\0';
	}
#ifdef DEBUG
	if (N->debug) nl_flush(N);
#endif
	return 0;
}

/*
 * math functions
 */
int nl_math1(nes_state *N)
{
	obj_t *cobj0=nes_getiobj(N, &N->l, 0);
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	num_t n;

	if (nc_strcmp(cobj0->d.str, "rand")==0) {
		n=rand();
		if (cobj1->type==NT_NUMBER) {
			n=fmod(n, cobj1->d.num);
		}
		nes_setnum(N, &N->r, "", n);
		return 0;
	}
	if (cobj1->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number");
	if (nc_strcmp(cobj0->d.str, "abs")==0) {
		n=fabs(cobj1->d.num);
	} else if (nc_strcmp(cobj0->d.str, "ceil")==0) {
		n=ceil(cobj1->d.num);
	} else if (nc_strcmp(cobj0->d.str, "floor")==0) {
		n=floor(cobj1->d.num);
	} else if (nc_strcmp(cobj0->d.str, "acos")==0) {
		n=acos(cobj1->d.num);
	} else if (nc_strcmp(cobj0->d.str, "asin")==0) {
		n=asin(cobj1->d.num);
	} else if (nc_strcmp(cobj0->d.str, "atan")==0) {
		n=atan(cobj1->d.num);
	} else if (nc_strcmp(cobj0->d.str, "cos")==0) {
		n=cos(cobj1->d.num);
	} else if (nc_strcmp(cobj0->d.str, "sin")==0) {
		n=sin(cobj1->d.num);
	} else if (nc_strcmp(cobj0->d.str, "tan")==0) {
		n=tan(cobj1->d.num);
	} else if (nc_strcmp(cobj0->d.str, "sqrt")==0) {
		n=sqrt(cobj1->d.num);
	} else {
		n=0;
	}
	nes_setnum(N, &N->r, "", n);
	return 0;
}

int nl_tonumber(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	num_t n=0;

	if (cobj1->type==NT_NUMBER) {
		n=cobj1->d.num;
	} else if (cobj1->type==NT_STRING) {
		n=nes_aton(N, (char *)cobj1->d.str);
	}
	nes_setnum(N, &N->r, "", n);
	return 0;
}

int nl_tostring(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	char *p=nes_tostr(N, cobj1);

	nes_setstr(N, &N->r, "", p, p?nc_strlen(p):0);
	return 0;
}

/*
 * string functions
 */
int nl_strcat(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t *robj;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	robj=nes_setstr(N, &N->r, "", NULL, 0);
	robj->size=cobj1->size+cobj2->size;
	robj->d.str=n_alloc(N, (robj->size+1)*sizeof(char));
	nc_strncpy(robj->d.str, cobj1->d.str, cobj1->size+1);
	nc_strncpy(robj->d.str+cobj1->size, cobj2->d.str, cobj2->size+1);
	return 0;
}

int nl_strcmp(nes_state *N)
{
	obj_t *cobj0=nes_getiobj(N, &N->l, 0);
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t *cobj3=nes_getiobj(N, &N->l, 3);
	uchar *s1, *s2;
	int i, rval=0;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	if (nc_strcmp(cobj0->d.str, "cmp")==0) {
		rval=nc_strcmp(cobj1->d.str, cobj2->d.str);
	} else if (nc_strcmp(cobj0->d.str, "ncmp")==0) {
		if (cobj3->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number for arg3");
		rval=nc_strncmp(cobj1->d.str, cobj2->d.str, (int)cobj3->d.num);
	} else if (nc_strcmp(cobj0->d.str, "icmp")==0) {
		/* rval=strcasecmp((char *)cobj1->d.str, (char *)cobj2->d.str); */
		s1=(uchar *)cobj1->d.str; s2=(uchar *)cobj2->d.str;
		do {
			if ((rval=(nc_tolower(*s1)-nc_tolower(*s2)))!=0) break;
			s1++; s2++;
		} while (*s1!=0);
	} else if (nc_strcmp(cobj0->d.str, "nicmp")==0) {
		if (cobj3->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number for arg3");
		/* rval=strncasecmp((char *)cobj1->d.str, (char *)cobj2->d.str, (int)cobj3->d.num); */
		s1=(uchar *)cobj1->d.str; s2=(uchar *)cobj2->d.str; i=(int)cobj3->d.num;
		do {
			if ((rval=(nc_tolower(*s1)-nc_tolower(*s2)))!=0) break;
			if (--i<1) break;
			s1++; s2++;
		} while (*s1!=0);
	}
	/* if (rval) rval=rval/abs(rval); */
	nes_setnum(N, &N->r, "", rval);
	return 0;
}

int nl_strlen(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	nes_setnum(N, &N->r, "", cobj1->size);
	return 0;
}

int nl_strstr(nes_state *N)
{
	obj_t *cobj0=nes_getiobj(N, &N->l, 0);
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t *robj;
	unsigned int i=0, j=0;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	if (cobj2->size<1) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "zero length arg2");
	if (nc_strcmp(cobj0->d.str, "str")==0) {
		for (i=0,j=0;i<cobj1->size;i++,j++) {
			if (j==cobj2->size) break;
			if (cobj2->d.str[j]=='\0') { j=0; break; }
			if (cobj2->d.str[j]!=cobj1->d.str[i]) j=-1;
		}
	} else if (nc_strcmp(cobj0->d.str, "istr")==0) {
		for (i=0,j=0;i<cobj1->size;i++,j++) {
			if (j==cobj2->size) break;
			if (cobj2->d.str[j]=='\0') { j=0; break; }
			if (nc_tolower(cobj2->d.str[j])!=nc_tolower(cobj1->d.str[i])) j=-1;
		}
	} else {
		n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "................");
	}
	robj=nes_setstr(N, &N->r, "", NULL, 0);
	if ((i<cobj1->size)&&(j==cobj2->size)) {
		i=i-j;
		robj->size=cobj1->size-i;
		robj->d.str=n_alloc(N, (robj->size+1)*sizeof(char));
		nc_strncpy(robj->d.str, cobj1->d.str+i, robj->size+1);
	} else {
	}
	return 0;
}

int nl_strsub(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t *cobj3=nes_getiobj(N, &N->l, 3);
	obj_t *robj;
	unsigned int offset, max=0;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number for arg2");
	if (cobj2->d.num<0) {
		offset=cobj1->size-abs((int)cobj2->d.num);
	} else {
		offset=(int)cobj2->d.num;
	}
	if (cobj3->type==NT_NUMBER) max=(int)cobj3->d.num;
	if (offset>cobj1->size) offset=cobj1->size;
	if (max>cobj1->size-offset) max=cobj1->size-offset;
	robj=nes_setstr(N, &N->r, "", NULL, 0);
	if (cobj1->d.str!=NULL) {
		robj->size=max;
		robj->d.str=n_alloc(N, (robj->size+1)*sizeof(char));
		nc_strncpy(robj->d.str, cobj1->d.str+offset, robj->size+1);
	}
	return 0;
}

int nl_datetime(nes_state *N)
{
	obj_t *cobj0=nes_getiobj(N, &N->l, 0);
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	struct timeval ttime;
	char timebuf[16];

	if (cobj0->type!=NT_STRING) { return 0; }
	if (cobj1->type==NT_NUMBER) {
		ttime.tv_sec=(time_t)cobj1->d.num;
	} else {
		nc_gettimeofday(&ttime, NULL);
	}
	if (nc_strcmp(cobj0->d.str, "date")==0) {
		strftime(timebuf, sizeof(timebuf)-1, "%Y-%m-%d", localtime((time_t *)&ttime.tv_sec));
	} else if (nc_strcmp(cobj0->d.str, "time")==0) {
		strftime(timebuf, sizeof(timebuf)-1, "%H:%M:%S", localtime((time_t *)&ttime.tv_sec));
	}
	nes_setstr(N, &N->r, "", timebuf, nc_strlen(timebuf));
	return 0;	
}

int nl_runtime(nes_state *N)
{
	struct timeval ttime;
	int totaltime;

	nc_gettimeofday(&ttime, NULL);
	totaltime=((ttime.tv_sec-N->ttime.tv_sec)*1000000)+(ttime.tv_usec-N->ttime.tv_usec);
	nes_setnum(N, &N->r, "", (num_t)totaltime/1000000);
	return 0;
}

int nl_sleep(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	int n=1;

	if (cobj1->type==NT_NUMBER) n=(int)cobj1->d.num;
#ifdef WIN32
		_sleep(n);
#else
		sleep(n);
#endif
	return 0;
}

int nl_include(nes_state *N)
{
	obj_t *cobj;
	uchar *p;

	if ((cobj=nes_getiobj(N, &N->l, 1))!=NULL) {
		p=N->readptr;
		nes_execfile(N, (char *)cobj->d.str);
		N->readptr=p;
	}
	return 0;
}
