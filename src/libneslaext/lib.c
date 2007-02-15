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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "libnesla.h"

#ifdef WIN32
#include <io.h>
#else
#include <dirent.h>
#endif
#include <sys/stat.h>

static const char b64chars[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *fixslashes(char *s)
{
	char *p=s;

	while (*p) { if (*p=='\\') *p='/'; p++; }
	return s;
}

static int n_dirlist(nesla_state *N, obj_t *dobj, const char *dirname)
{
#ifdef WIN32
	struct  _finddata_t dentry;
	long    handle;
#else
	struct dirent *dentry;
	DIR    *handle;
#endif
	struct stat sb;
	char   dir[512];
	char   file[512];
	char   *name;
	obj_t  *tobj2;
	int    sym;

	n_snprintf(N, dir, sizeof(dir)-1, dirname);
	fixslashes(dir);
	if (dir[strlen(dir)-1]=='/') dir[strlen(dir)-1]='\0';
	if (stat(dir, &sb)!=0) return -1;
	if (!(sb.st_mode&S_IFDIR)) return 0;
#ifdef WIN32
	n_snprintf(N, file, sizeof(file)-1, "%s/*.*", dir);
	if ((handle=_findfirst(file, &dentry))<0) return 0;
	do {
		name=dentry.name;
		n_snprintf(N, file, sizeof(file)-1, "%s/%s", dir, name);
		sym=0;
		if (stat(file, &sb)!=0) continue;
#else
	handle=opendir(dir);
	while ((dentry=readdir(handle))!=NULL) {
		name=dentry->d_name;
		n_snprintf(N, file, sizeof(file)-1, "%s/%s", dir, name);
		sym=0;
		if (lstat(file, &sb)!=0) continue;
		if (!(~sb.st_mode&S_IFLNK)) {
			sym=1;
			if (stat(file, &sb)!=0) sym=2;
		}
#endif
		tobj2=nesla_regtable(N, dobj, name);
		nesla_regnum(N, tobj2, "mtime", sb.st_mtime);
		if (sym==2) {
			nesla_regnum(N, tobj2, "size", 0);
			nesla_regstr(N, tobj2, "type", "broken");
		} else if ((sb.st_mode&S_IFDIR)) {
			nesla_regnum(N, tobj2, "size", 0);
			nesla_regstr(N, tobj2, "type", sym?"dirp":"dir");
		} else {
			nesla_regnum(N, tobj2, "size", sb.st_size);
			nesla_regstr(N, tobj2, "type", sym?"filep":"file");
		}
#ifdef WIN32
	} while (_findnext(handle, &dentry)==0);
	_findclose(handle);
#else
	}
	closedir(handle);
#endif
	return 0;
}

int neslaext_dirlist(nesla_state *N)
{
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");
	obj_t *cobj2=nesla_getobj(N, &N->l, "!2");
	obj_t *tobj;
	int rc;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a string for arg1");
	if (cobj2->type!=NT_STRING) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a string for arg2");
	tobj=nesla_regtable(N, &N->g, cobj1->d.string);
	rc=n_dirlist(N, tobj, cobj2->d.string);
	nesla_sorttable(N, tobj, 1);
	return rc;
}

int neslaext_system(nesla_state *N)
{
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");
	int n=0;

	if (cobj1->type==NT_STRING) {
		n=system(cobj1->d.string);
		nesla_regnum(N, &N->g, "_retval", n);
	}
	return n;
}

int neslaext_base64_decode(nesla_state *N)
{
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");
	obj_t *robj;
	char *src;
	char *dest;
	char *pos;
	int destidx;
	int state;
	int ch;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a string for arg1");
	src=cobj1->d.string;
	robj=nesla_regstr(N, &N->g, "_retval", NULL);
	if (cobj1->size<1) return 0;
	/* should actually be about 3/4 the size of cobj1->size */
	if ((dest=n_alloc(N, (cobj1->size)*sizeof(char)))==NULL) return -1;
	state=0;
	destidx=0;
	while ((ch=*src++)!='\0') {
		if (isspace(ch)) continue;
		if (ch=='=') break;
		pos=strchr(b64chars, ch);
		if (pos==0) return 0;
		switch (state) {
		case 0:
			if (dest) {
				dest[destidx]=(pos-b64chars)<<2;
			}
			state=1;
			break;
		case 1:
			if (dest) {
				dest[destidx]|=(pos-b64chars)>>4;
				dest[destidx+1]=((pos-b64chars)&0x0f)<<4;
			}
			destidx++;
			state=2;
			break;
		case 2:
			if (dest) {
				dest[destidx]|=(pos-b64chars)>>2;
				dest[destidx+1]=((pos-b64chars)&0x03)<<6;
			}
			destidx++;
			state=3;
			break;
		case 3:
			if (dest) {
				dest[destidx]|=(pos-b64chars);
			}
			destidx++;
			state=0;
			break;
		}
	}
	dest[destidx]='\0';
	robj->d.string=dest;
	robj->size=destidx-1;
	return 0;
}

int neslaext_base64_encode(nesla_state *N)
{
	obj_t *cobj1=nesla_getobj(N, &N->l, "!1");
	obj_t *cobj2=nesla_getobj(N, &N->l, "!2");
	obj_t *robj;
	char *dest;
	unsigned char a, b, c, d, *cp;
	int dst, i, enclen, remlen, linelen, maxline=0;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nesla_getrstr(N, &N->l, "!0"), "expected a string for arg1");
	cp=(unsigned char *)cobj1->d.string;
	robj=nesla_regstr(N, &N->g, "_retval", NULL);
	if (cobj1->size<1) return 0;
	dst=0;
	linelen=0;
	if (cobj2->type==NT_NUMBER) maxline=(int)cobj2->d.number;
	enclen=cobj1->size/3;
	remlen=cobj1->size-3*enclen;
	/* should actually be about 4/3 the size of cobj1->size */
	i=maxline?(((cobj1->size+3)/3*4)/maxline):5;
	i=i*2+5;
	if ((dest=n_alloc(N, (enclen*4+i)*sizeof(char)))==NULL) return -1;
	robj->d.string=dest;
	for (i=0;i<enclen;i++) {
		a=(cp[0]>>2);
	        b=(cp[0]<<4)&0x30;
		b|=(cp[1]>>4);
		c=(cp[1]<<2)&0x3c;
		c|=(cp[2]>>6);
		d=cp[2]&0x3f;
		cp+=3;
		dest[dst+0]=b64chars[a];
		dest[dst+1]=b64chars[b];
		dest[dst+2]=b64chars[c];
		dest[dst+3]=b64chars[d];
		dst+=4;
		linelen+=4;
		if ((maxline>0)&&(linelen>=maxline)) {
			dest[dst+0]='\r';
			dest[dst+1]='\n';
			dst+=2;
			linelen=0;
		}
	}
	if (remlen==1) {
		a=(cp[0]>>2);
		b=(cp[0]<<4)&0x30;
		dest[dst+0]=b64chars[a];
		dest[dst+1]=b64chars[b];
		dest[dst+2]='=';
		dest[dst+3]='=';
		dst+=4;
	} else if (remlen==2) {
		a=(cp[0]>>2);
		b=(cp[0]<<4)&0x30;
		b|=(cp[1]>>4);
		c=(cp[1]<<2)&0x3c;
		dest[dst+0]=b64chars[a];
		dest[dst+1]=b64chars[b];
		dest[dst+2]=b64chars[c];
		dest[dst+3]='=';
		dst+=4;
	}
	dest[dst]='\0';
	robj->size=dst;
	return 0;
}

int neslaext_register_all(nesla_state *N)
{
	obj_t *tobj;

	tobj=nesla_regtable(N, &N->g, "base64");
	tobj->mode|=NST_HIDDEN;
	nesla_regcfunc(N, tobj, "decode", (void *)neslaext_base64_decode);
	nesla_regcfunc(N, tobj, "encode", (void *)neslaext_base64_encode);
	nesla_regcfunc(N, &N->g, "dirlist", (void *)neslaext_dirlist);
	nesla_regcfunc(N, &N->g, "system",  (void *)neslaext_system);
	return 0;
}
