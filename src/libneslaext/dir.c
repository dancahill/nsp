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
#include "libneslaext.h"
#ifdef WIN32
#include <io.h>
#else
#include <dirent.h>
#endif
#include <sys/stat.h>

static char *fixslashes(char *s)
{
	char *p=s;

	while (*p) { if (*p=='\\') *p='/'; p++; }
	return s;
}

static int n_dirlist(nes_state *N, obj_t *dobj, const char *dirname)
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
	int    len;

	nc_strncpy(dir, dirname, sizeof(dir));
	fixslashes(dir);
	len=nc_strlen(dir);
	if (dir[len-1]=='/') dir[len-1]='\0';
	if (stat(dir, &sb)!=0) return -1;
	if (!(sb.st_mode&S_IFDIR)) return 0;
#ifdef WIN32
	nc_snprintf(N, file, sizeof(file)-1, "%s/*.*", dir);
	if ((handle=_findfirst(file, &dentry))<0) return 0;
	do {
		name=dentry.name;
		nc_snprintf(N, file, sizeof(file)-1, "%s/%s", dir, name);
		sym=0;
		if (stat(file, &sb)!=0) continue;
#else
	handle=opendir(dir);
	while ((dentry=readdir(handle))!=NULL) {
		name=dentry->d_name;
		nc_snprintf(N, file, sizeof(file)-1, "%s/%s", dir, name);
		sym=0;
#ifdef __TURBOC__
		if (stat(file, &sb)!=0) continue;
#else
		if (lstat(file, &sb)!=0) continue;
		if (!(~sb.st_mode&S_IFLNK)) {
			sym=1;
			if (stat(file, &sb)!=0) sym=2;
		}
#endif
#endif
		tobj2=nes_settable(N, dobj, name);
		nes_setnum(N, tobj2, "mtime", sb.st_mtime);
		if (sym==2) {
			nes_setnum(N, tobj2, "size", 0);
			nes_setstr(N, tobj2, "type", "broken", 6);
		} else if ((sb.st_mode&S_IFDIR)) {
			nes_setnum(N, tobj2, "size", 0);
			nes_setstr(N, tobj2, "type", sym?"dirp":"dir", sym?4:3);
		} else {
			nes_setnum(N, tobj2, "size", sb.st_size);
			nes_setstr(N, tobj2, "type", sym?"filep":"file", sym?5:4);
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

int neslaext_dirlist(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t *tobj;

	if (cobj1->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	tobj=nes_settable(N, &N->g, cobj1->d.str);
	n_dirlist(N, tobj, cobj2->d.str);
	return 0;
}
