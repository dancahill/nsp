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
#include "nesla/libnesla.h"
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#ifdef WIN32
#include <io.h>
#else
#ifdef __TURBOC__
#include <stdio.h>
#else
#include <unistd.h>
#endif
#endif

static int dumpwrite(nes_state *N, const char *str, int len)
{
	int i;

	for (i=0;i<len;i++) {
		if (MAX_OUTBUFLEN-N->outbuflen<32) { nl_flush(N); continue; }
		switch (str[i]) {
		case '\a' : N->outbuf[N->outbuflen++]='\\'; N->outbuf[N->outbuflen++]='a'; break;
		case '\t' : N->outbuf[N->outbuflen++]='\\'; N->outbuf[N->outbuflen++]='t'; break;
		case '\f' : N->outbuf[N->outbuflen++]='\\'; N->outbuf[N->outbuflen++]='f'; break;
		case 27   : N->outbuf[N->outbuflen++]='\\'; N->outbuf[N->outbuflen++]='e'; break;
		case '\r' : N->outbuf[N->outbuflen++]='\\'; N->outbuf[N->outbuflen++]='r'; break;
		case '\n' : N->outbuf[N->outbuflen++]='\\'; N->outbuf[N->outbuflen++]='n'; break;
		case '\'' : N->outbuf[N->outbuflen++]='\\'; N->outbuf[N->outbuflen++]='\''; break;
		case '\"' : N->outbuf[N->outbuflen++]='\\'; N->outbuf[N->outbuflen++]='\"'; break;
		case '\\' : N->outbuf[N->outbuflen++]='\\'; N->outbuf[N->outbuflen++]='\\'; break;
		default: N->outbuf[N->outbuflen++]=str[i];
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
		if ((cobj->val->attr&NST_HIDDEN)||(cobj->val->attr&NST_SYSTEM)) continue;
		if ((depth==1)&&(nc_strcmp(cobj->name, "filename")==0)) continue;
		g=(depth<1)?"global ":"";
		l=(depth<1)?"":(cobj->next)?",\n":"\n";
		if (nc_isdigit(cobj->name[0])) b=1; else b=0;
		if (nes_isnull(cobj)||(cobj->val->type==NT_BOOLEAN)||(cobj->val->type==NT_NUMBER)) {
			if (depth) nc_printf(N, "%s%s%s%s%s = ", indent, g, b?"[":"", cobj->name, b?"]":"");
			nc_printf(N, "%s%s", nes_tostr(N, cobj), l);
		} else if (cobj->val->type==NT_STRING) {
			if (depth) nc_printf(N, "%s%s%s%s%s = ", indent, g, b?"[":"", cobj->name, b?"]":"");
			nc_printf(N, "\"");
			dumpwrite(N, cobj->val->d.str?cobj->val->d.str:"", cobj->val->size);
			nc_printf(N, "\"%s", l);
		} else if (cobj->val->type==NT_TABLE) {
			if (nc_strcmp(cobj->name, "_GLOBALS")==0) continue;
			if (depth) nc_printf(N, "%s%s%s%s%s = ", indent, g, b?"[":"", cobj->name, b?"]":"");
			nc_printf(N, "{");
			if (cobj->val->d.table) {
				nc_printf(N, "\n");
				dumpvars(N, cobj->val->d.table, depth+1);
				nc_printf(N, "%s}%s", indent, l);
			} else {
				nc_printf(N, " }%s", l);
			}
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
	NES_CFUNC cfunc=(NES_CFUNC)nl_flush;
	int rc;

	if (cobj->val->type!=NT_TABLE) {
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
	cobj=nes_getobj(N, cobj, "flush");
	if (cobj->val->type==NT_CFUNC) {
		cfunc=cobj->val->d.cfunc;
		if (cfunc!=(NES_CFUNC)nl_flush) {
			rc=cfunc(N);
			nes_setnum(N, &N->r, "", rc);
			return rc;
		}
	}
	N->outbuf[N->outbuflen]='\0';
	write(STDOUT_FILENO, N->outbuf, N->outbuflen);
	N->outbuflen=0;
	nes_setnum(N, &N->r, "", 0);
	return 0;
}

int nl_print(nes_state *N)
{
	obj_t *cobj=N->l.val->d.table;
	unsigned int len, tlen=0;
	unsigned int i;
	char *p;

	if (N->outbuflen>OUTBUFLOWAT) nl_flush(N);
	if ((cobj==NULL)||(nc_strcmp("0", cobj->name)!=0)) return -1;
	for (cobj=cobj->next; cobj; cobj=cobj->next) {
		if (cobj->val->type!=NT_STRING) {
			p=nes_tostr(N, cobj);
			len=nc_strlen(p);
			tlen+=len;
			for (i=0;i<len;i++) {
				N->outbuf[N->outbuflen++]=p[i];
			}
			N->outbuf[N->outbuflen]='\0';
			continue;
		}
		for (i=0;i<cobj->val->size;i++) {
			if (N->outbuflen>OUTBUFLOWAT) nl_flush(N);
			N->outbuf[N->outbuflen++]=cobj->val->d.str[i];
		}
		N->outbuf[N->outbuflen]='\0';
	}
	if (N->debug) nl_flush(N);
	nes_setnum(N, &N->r, "", tlen);
	return 0;
}

int nl_write(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	unsigned int len=0;
	unsigned int i;
	char *p=NULL;

	if (cobj1->val->type==NT_STRING) {
		p=cobj1->val->d.str;
		len=cobj1->val->size;
	} else {
		p=nes_tostr(N, cobj1);
		len=nc_strlen(p);
	}
	if (p) {
		for (i=0;i<len;i++) {
			if (N->outbuflen>OUTBUFLOWAT) nl_flush(N);
			N->outbuf[N->outbuflen++]=p[i];
		}
		N->outbuf[N->outbuflen]='\0';
	}
	nes_setnum(N, &N->r, "", len);
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
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t *robj;
	struct stat sb;
	char *p;
	int bl;
	int fd;
	int r;
	int offset=0;

	if ((cobj1->val->type!=NT_STRING)||(cobj1->val->size<1)) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (stat(cobj1->val->d.str, &sb)!=0) {
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
	if ((fd=open(cobj1->val->d.str, O_RDONLY|O_BINARY))==-1) {
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
	if (cobj2->val->type==NT_NUMBER) {
		offset=(int)cobj2->val->d.num;
		lseek(fd, offset, SEEK_SET);
	}
	robj=nes_setstr(N, &N->r, "", NULL, 0);
	bl=sb.st_size-offset;
	robj->val->d.str=n_alloc(N, (bl+2)*sizeof(char));
	robj->val->size=bl;
	p=(char *)robj->val->d.str;
	for (;;) {
		r=read(fd, p, bl);
		p+=r;
		bl-=r;
		if (bl<1) break;
	}
	close(fd);
	robj->val->d.str[sb.st_size]='\0';
	return 0;
}

int nl_filestat(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t tobj;
	struct stat sb;
	int rc;
	int sym=0;
	char *file;

	if ((cobj1->val->type!=NT_STRING)||(cobj1->val->size<1)) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	file=cobj1->val->d.str;
#ifdef WIN32
	rc=stat(file, &sb);
	if (rc!=0) {
		nes_setnum(N, &N->r, "", rc);
		return 0;
	}
#else
#ifdef __TURBOC__
	rc=stat(file, &sb);
	if (rc!=0) {
		nes_setnum(N, &N->r, "", rc);
		return 0;
	}
#else
	rc=lstat(file, &sb);
	if (rc!=0) {
		nes_setnum(N, &N->r, "", rc);
		return 0;
	}
	if (!(~sb.st_mode&S_IFLNK)) {
		sym=1;
		if (stat(file, &sb)!=0) sym=2;
	}
#endif
#endif
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nes_linkval(N, &tobj, NULL);
	tobj.val->type=NT_TABLE;
	tobj.val->attr|=NST_AUTOSORT;
	nes_setnum(N, &tobj, "mtime", sb.st_mtime);
	if (sym==2) {
		nes_setnum(N, &tobj, "size", 0);
		nes_setstr(N, &tobj, "type", "broken", 6);
	} else if ((sb.st_mode&S_IFDIR)) {
		nes_setnum(N, &tobj, "size", 0);
		nes_setstr(N, &tobj, "type", sym?"dirp":"dir", sym?4:3);
	} else {
		nes_setnum(N, &tobj, "size", sb.st_size);
		nes_setstr(N, &tobj, "type", sym?"filep":"file", sym?5:4);
	}
	nes_linkval(N, &N->r, &tobj);
	nes_unlinkval(N, &tobj);
	return 0;
}

int nl_fileunlink(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	int rc;

	if ((cobj1->val->type!=NT_STRING)||(cobj1->val->size<1)) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	rc=unlink(cobj1->val->d.str);
	nes_setnum(N, &N->r, "", rc);
	return 0;
}

int nl_filewrite(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t *cobj3=nes_getiobj(N, &N->l, 3);
	int fd;
	int w=0;
	int offset=0;

	/* umask(022); */
	if ((cobj1->val->type!=NT_STRING)||(cobj1->val->size<1)) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if ((fd=open(cobj1->val->d.str, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE))==-1) {
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
	if (cobj2->val->type==NT_STRING) {
		if (cobj3->val->type==NT_NUMBER) {
			offset=(int)cobj3->val->d.num;
			lseek(fd, offset, SEEK_SET);
		}
		w=write(fd, cobj2->val->d.str, cobj2->val->size);
	}
	close(fd);
	nes_setnum(N, &N->r, "", w);
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
		if ((cobj1->val->type==NT_NUMBER)&&(int)cobj1->val->d.num) {
			n=(int)n%(int)cobj1->val->d.num;
		}
		nes_setnum(N, &N->r, "", n);
		return 0;
	}
	if (cobj1->val->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number");
	if (nc_strcmp(cobj0->val->d.str, "abs")==0) {
		n=fabs(cobj1->val->d.num);
	} else if (nc_strcmp(cobj0->val->d.str, "ceil")==0) {
		n=cobj1->val->d.num;
		if ((int)n<n) n=(int)n+1; else n=(int)n;
	} else if (nc_strcmp(cobj0->val->d.str, "floor")==0) {
		n=(int)cobj1->val->d.num;
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
	char *p;

	if (cobj1->val->type==NT_NUMBER) {
		n=cobj1->val->d.num;
	} else if ((cobj1->val->type==NT_STRING)&&(cobj1->val->d.str!=NULL)) {
		p=cobj1->val->d.str;
		if (p[0]=='-') {
			p++;
			n=-nes_aton(N, (char *)p);
		} else {
			n=nes_aton(N, (char *)p);
		}
	}
	nes_setnum(N, &N->r, "", n);
	return 0;
}

int nl_tostring(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	unsigned short d;
	char *p;

	if ((cobj1->val->type==NT_NUMBER)&&(cobj2->val->type==NT_NUMBER)) {
		d=(unsigned short)cobj2->val->d.num;
		if (d>sizeof(N->numbuf)-2) d=sizeof(N->numbuf)-2;
		p=nes_ntoa(N, N->numbuf, cobj1->val->d.num, 10, d);
	} else {
		p=nes_tostr(N, cobj1);
	}
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
	nc_memcpy(robj->val->d.str, cobj1->val->d.str, cobj1->val->size);
	nc_memcpy(robj->val->d.str+cobj1->val->size, cobj2->val->d.str, cobj2->val->size);
	robj->val->d.str[robj->val->size]=0;
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
		s1=(uchar *)cobj1->val->d.str; s2=(uchar *)cobj2->val->d.str;
		do {
			if ((rval=(nc_tolower(*s1)-nc_tolower(*s2)))!=0) break;
			s1++; s2++;
		} while (*s1!=0);
	} else if (nc_strcmp(cobj0->val->d.str, "nicmp")==0) {
		if (cobj3->val->type!=NT_NUMBER) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a number for arg3");
		s1=(uchar *)cobj1->val->d.str; s2=(uchar *)cobj2->val->d.str; i=(int)cobj3->val->d.num;
		do {
			if ((rval=(nc_tolower(*s1)-nc_tolower(*s2)))!=0) break;
			if (--i<1) break;
			s1++; s2++;
		} while (*s1!=0);
	}
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
	int i=0;
	char *s2;
	int l2;
	char namebuf[MAX_OBJNAMELEN+1];

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nes_linkval(N, &tobj, NULL);
	tobj.val->type=NT_TABLE;
	if ((cobj1->val->d.str)&&(cobj2->val->d.str)) {
		se=ss=cobj1->val->d.str;
		s2=cobj2->val->d.str;
		l2=cobj2->val->size;
		for (;*se;se++) {
			if (nc_strncmp(se, s2, l2)!=0) continue;
			nes_setstr(N, &tobj, nes_ntoa(N, namebuf, i++, 10, 0), ss, se-ss);
			ss=se+=l2;
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
		nc_memcpy(robj->val->d.str, cobj1->val->d.str+i, robj->val->size+1);
		robj->val->d.str[robj->val->size]=0;
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
		nc_memcpy(robj->val->d.str, cobj1->val->d.str+offset, robj->val->size+1);
		robj->val->d.str[robj->val->size]=0;
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
	Sleep(n*1000);
#else
	sleep(n);
#endif
	nes_setnum(N, &N->r, "", 0);
	return 0;
}

int nl_include(nes_state *N)
{
	obj_t *cobj;
	uchar *p;
	int n=0;

	if ((cobj=nes_getiobj(N, &N->l, 1))!=NULL) {
		p=N->readptr;
		n=nes_execfile(N, (char *)cobj->val->d.str);
		N->readptr=p;
	}
	nes_setnum(N, &N->r, "", n);
	return n;
}

int nl_printvar(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);

	if (nes_isnull(cobj1)) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected an object for arg1");
	dumpvars(N, cobj1, 0);
	nes_setnum(N, &N->r, "", 0);
	return 0;
}

int nl_size(nes_state *N)
{
	obj_t *cobj=nes_getiobj(N, &N->l, 1);
	int size=0;

	if (nes_isnull(cobj)) {
		nes_setnum(N, &N->r, "", 0);
		return 0;
	}
	switch (cobj->val->type) {
	case NT_BOOLEAN :
	case NT_NUMBER  : size=1; break;
	case NT_STRING  :
	case NT_NFUNC   : size=cobj->val->size; break;
	case NT_CFUNC   : size=1; break;
	case NT_TABLE   :
		for (cobj=cobj->val->d.table;cobj;cobj=cobj->next) {
			if (!nes_isnull(cobj)) size++;
		}
		break;
	case NT_CDATA   : size=cobj->val->size; break;
	}
	nes_setnum(N, &N->r, "", size);
	return 0;
}

int nl_type(nes_state *N)
{
	obj_t *cobj=nes_getiobj(N, &N->l, 1);
	NES_CDATA *chead;
	char *p;

	switch (cobj->val->type) {
	case NT_BOOLEAN : p="boolean";  break;
	case NT_NUMBER  : p="number";   break;
	case NT_STRING  : p="string";   break;
	case NT_TABLE   : p="table";    break;
	case NT_NFUNC   :
	case NT_CFUNC   : p="function"; break;
	case NT_CDATA   :
		if ((chead=(NES_CDATA *)cobj->val->d.cdata)!=NULL) {
			p=chead->obj_type; break;
		}
	default         : p="null";     break;
	}
	nes_setstr(N, &N->r, "", p, nc_strlen(p));
	return 0;
}

int nl_system(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	int n=-1;

	if ((cobj1->val->type==NT_STRING)&&(cobj1->val->d.str!=NULL)) {
		nl_flush(N);
		n=system(cobj1->val->d.str);
	}
	nes_setnum(N, &N->r, "", n);
	return 0;
}
