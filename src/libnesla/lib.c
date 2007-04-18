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
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
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
		if (nes_isnull(cobj)||(cobj->val->type==NT_BOOLEAN)||(cobj->val->type==NT_NUMBER)) {
			nc_printf(N, "%s%s%s%s%s = ", indent, g, b?"[":"", cobj->name, b?"]":"");
			nc_printf(N, "%s%s\n", nes_tostr(N, cobj), l);
		} else if (cobj->val->type==NT_STRING) {
			nc_printf(N, "%s%s%s%s%s = \"", indent, g, b?"[":"", cobj->name, b?"]":"");
			dumpwrite(N, cobj->val->d.str?cobj->val->d.str:"", cobj->val->size);
			nc_printf(N, "\"%s\n", l);
		} else if (cobj->val->type==NT_TABLE) {
			if (nc_strcmp(cobj->name, "_GLOBALS")==0) continue;
			nc_printf(N, "%s%s%s%s%s = {\n", indent, g, b?"[":"", cobj->name, b?"]":"");
			dumpvars(N, cobj->val->d.table, depth+1);
			nc_printf(N, "%s}%s\n", indent, l);
		}
	}
	nl_flush(N);
	return;
}

int n_unescape(nes_state *N, char *src, char *dst, int len)
{
	int i, n;
	short e=0;

	if (dst==NULL) return 0;
	for (i=0,n=0;i<len;i++) {
		if (!e) {
			if (src[i]=='\\') {
				e=1;
			} else {
				dst[n++]=src[i];
			}
			continue;
		}
		switch (src[i]) {
			case 'a'  : dst[n++]='\a'; break;
			case 't'  : dst[n++]='\t'; break;
			case 'f'  : dst[n++]='\f'; break;
			case 'e'  : dst[n++]=27;   break;
			case 'r'  : dst[n++]='\r'; break;
			case 'n'  : dst[n++]='\n'; break;
			case '\'' : dst[n++]='\''; break;
			case '\"' : dst[n++]='\"'; break;
			case '\\' : dst[n++]='\\'; break;
			default   : break;
		}
		e=0;
	}
	dst[n]='\0';
	return n;
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

	if (cobj->val->type==NT_TABLE) {
		cobj=nes_getobj(N, cobj, "flush");
		if (cobj->val->type==NT_CFUNC) {
			if ((cfunc=(NES_CFUNC)cobj->val->d.cfunc)!=nl_flush) {
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
	obj_t *cobj=N->l.val->d.table;
	unsigned int len;
	unsigned int i;
	char *p;

	if (N->outbuflen>OUTBUFLOWAT) nl_flush(N);
	if ((cobj==NULL)||(nc_strcmp("0", cobj->name)!=0)) return -1;
	for (cobj=cobj->next; cobj; cobj=cobj->next) {
		if (cobj->val->type!=NT_STRING) {
			p=nes_tostr(N, cobj);
			len=nc_strlen(p);
			for (i=0;i<len;i++) {
				N->outbuf[N->outbuflen++]=p[i];
			}
			N->outbuf[N->outbuflen]='\0';
			continue;
		}
		for (i=0;i<cobj->val->size;i++) {
			if (N->outbuflen>sizeof(N->outbuf)) break;
			N->outbuf[N->outbuflen++]=cobj->val->d.str[i];
		}
		N->outbuf[N->outbuflen]='\0';
	}
#ifdef DEBUG
	if (N->debug) nl_flush(N);
#endif
	return 0;
}

/*
 * file functions
 */
#ifndef O_BINARY
#define O_BINARY 0
#endif
int nl_fileread(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *robj;
	struct stat sb;
	char *p;
	int bl;
	int fd;
	int r;

	if ((cobj1->val->type!=NT_STRING)||(cobj1->val->size<1)) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (stat(cobj1->val->d.str, &sb)!=0) {
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
	if ((fd=open(cobj1->val->d.str, O_RDONLY|O_BINARY))==-1) {
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
	robj=nes_setstr(N, &N->r, "", NULL, 0);
	robj->val->d.str=n_alloc(N, (sb.st_size+2)*sizeof(char));
	p=(char *)robj->val->d.str;
	bl=sb.st_size;
	for (;;) {
		r=read(fd, p, bl);
		p+=r;
		bl-=r;
		if (bl<1) break;
	}
	close(fd);
	robj->val->d.str[sb.st_size]='\0';
	robj->val->size=sb.st_size;
	return 0;
}

int nl_filewrite(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t *robj;
	int fd;
	int w=0;

	if ((cobj1->val->type!=NT_STRING)||(cobj1->val->size<1)) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if ((fd=open(cobj1->val->d.str, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC))==-1) {
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
	if (cobj2->val->type==NT_STRING) {
		w=write(fd, cobj2->val->d.str, cobj2->val->size);
	}
	close(fd);
	robj=nes_setnum(N, &N->r, "", w);
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

	if (nc_strcmp(cobj0->val->d.str, "rand")==0) {
		n=rand();
		if (cobj1->val->type==NT_NUMBER) {
			n=fmod(n, cobj1->val->d.num);
		}
		nes_setnum(N, &N->r, "", n);
		return 0;
	}
	if (cobj1->val->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number");
	if (nc_strcmp(cobj0->val->d.str, "abs")==0) {
		n=fabs(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "ceil")==0) {
		n=ceil(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "floor")==0) {
		n=floor(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "acos")==0) {
		n=acos(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "asin")==0) {
		n=asin(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "atan")==0) {
		n=atan(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "cos")==0) {
		n=cos(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "sin")==0) {
		n=sin(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "tan")==0) {
		n=tan(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "sqrt")==0) {
		n=sqrt(cobj1->val->d.num);
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

	if (cobj1->val->type==NT_NUMBER) {
		n=cobj1->val->d.num;
	} else if (cobj1->val->type==NT_STRING) {
		n=nes_aton(N, (char *)cobj1->val->d.str);
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

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	robj=nes_setstr(N, &N->r, "", NULL, 0);
	robj->val->size=cobj1->val->size+cobj2->val->size;
	robj->val->d.str=n_alloc(N, (robj->val->size+1)*sizeof(char));
	nc_strncpy(robj->val->d.str, cobj1->val->d.str, cobj1->val->size+1);
	nc_strncpy(robj->val->d.str+cobj1->val->size, cobj2->val->d.str, cobj2->val->size+1);
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

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	if (nc_strcmp(cobj0->val->d.str, "cmp")==0) {
		rval=nc_strcmp(cobj1->val->d.str, cobj2->val->d.str);
	} else if (nc_strcmp(cobj0->val->d.str, "ncmp")==0) {
		if (cobj3->val->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number for arg3");
		rval=nc_strncmp(cobj1->val->d.str, cobj2->val->d.str, (int)cobj3->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "icmp")==0) {
		/* rval=strcasecmp((char *)cobj1->val->d.str, (char *)cobj2->val->d.str); */
		s1=(uchar *)cobj1->val->d.str; s2=(uchar *)cobj2->val->d.str;
		do {
			if ((rval=(nc_tolower(*s1)-nc_tolower(*s2)))!=0) break;
			s1++; s2++;
		} while (*s1!=0);
	} else if (nc_strcmp(cobj0->val->d.str, "nicmp")==0) {
		if (cobj3->val->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number for arg3");
		/* rval=strncasecmp((char *)cobj1->val->d.str, (char *)cobj2->val->d.str, (int)cobj3->val->d.num); */
		s1=(uchar *)cobj1->val->d.str; s2=(uchar *)cobj2->val->d.str; i=(int)cobj3->val->d.num;
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

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	nes_setnum(N, &N->r, "", cobj1->val->size);
	return 0;
}

int nl_strsplit(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t tobj;
	char *ss, *se;
	char s;
	int i=0;
	char namebuf[MAX_OBJNAMELEN+1];

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nes_linkval(N, &tobj, NULL);
	tobj.val->type=NT_TABLE;
	if ((cobj1->val->d.str)&&(cobj2->val->d.str)) {
		s=cobj2->val->d.str[0];
		se=ss=cobj1->val->d.str;
		for (;*se;se++) {
			if (*se!=s) continue;
			nes_setstr(N, &tobj, nes_ntoa(N, namebuf, i++, 10, 0), ss, se-ss);
			ss=++se;
			if (!*se) {
				nes_setstr(N, &tobj, nes_ntoa(N, namebuf, i++, 10, 0), ss, se-ss);
				break;
			}
		}
		if (se>ss) {
			nes_setstr(N, &tobj, nes_ntoa(N, namebuf, i++, 10, 0), ss, se-ss);
		}
	}
	nes_linkval(N, &N->r, &tobj);
	nes_unlinkval(N, &tobj);
	return 0;
}

int nl_strstr(nes_state *N)
{
	obj_t *cobj0=nes_getiobj(N, &N->l, 0);
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t *robj;
	unsigned int i=0, j=0;

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	if (cobj2->val->size<1) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "zero length arg2");
	if (nc_strcmp(cobj0->val->d.str, "str")==0) {
		for (i=0,j=0;i<cobj1->val->size;i++,j++) {
			if (j==cobj2->val->size) break;
			if (cobj2->val->d.str[j]=='\0') { j=0; break; }
			if (cobj2->val->d.str[j]!=cobj1->val->d.str[i]) j=-1;
		}
	} else if (nc_strcmp(cobj0->val->d.str, "istr")==0) {
		for (i=0,j=0;i<cobj1->val->size;i++,j++) {
			if (j==cobj2->val->size) break;
			if (cobj2->val->d.str[j]=='\0') { j=0; break; }
			if (nc_tolower(cobj2->val->d.str[j])!=nc_tolower(cobj1->val->d.str[i])) j=-1;
		}
	} else {
		n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "................");
	}
	robj=nes_setstr(N, &N->r, "", NULL, 0);
	if ((i<cobj1->val->size)&&(j==cobj2->val->size)) {
		i=i-j;
		robj->val->size=cobj1->val->size-i;
		robj->val->d.str=n_alloc(N, (robj->val->size+1)*sizeof(char));
		nc_strncpy(robj->val->d.str, cobj1->val->d.str+i, robj->val->size+1);
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

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->val->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number for arg2");
	if (cobj2->val->d.num<0) {
		offset=cobj1->val->size-abs((int)cobj2->val->d.num);
	} else {
		offset=(int)cobj2->val->d.num;
	}
	if (cobj3->val->type==NT_NUMBER) max=(int)cobj3->val->d.num;
	if (offset>cobj1->val->size) offset=cobj1->val->size;
	if (max>cobj1->val->size-offset) max=cobj1->val->size-offset;
	robj=nes_setstr(N, &N->r, "", NULL, 0);
	if (cobj1->val->d.str!=NULL) {
		robj->val->size=max;
		robj->val->d.str=n_alloc(N, (robj->val->size+1)*sizeof(char));
		nc_strncpy(robj->val->d.str, cobj1->val->d.str+offset, robj->val->size+1);
	}
	return 0;
}

int nl_datetime(nes_state *N)
{
	obj_t *cobj0=nes_getiobj(N, &N->l, 0);
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	struct timeval ttime;
	char timebuf[16];

	if (cobj0->val->type!=NT_STRING) { return 0; }
	if (cobj1->val->type==NT_NUMBER) {
		ttime.tv_sec=(time_t)cobj1->val->d.num;
	} else {
		nc_gettimeofday(&ttime, NULL);
	}
	if (nc_strcmp(cobj0->val->d.str, "date")==0) {
		strftime(timebuf, sizeof(timebuf)-1, "%Y-%m-%d", localtime((time_t *)&ttime.tv_sec));
	} else if (nc_strcmp(cobj0->val->d.str, "time")==0) {
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

	if (cobj1->val->type==NT_NUMBER) n=(int)cobj1->val->d.num;
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
		nes_execfile(N, (char *)cobj->val->d.str);
		N->readptr=p;
	}
	return 0;
}
