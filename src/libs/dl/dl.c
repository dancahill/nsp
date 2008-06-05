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
#ifdef WIN32
#define LIBEXT "dll"
#else
#include <dlfcn.h>
#define LIBEXT "so"
#endif

static void *lib_open(const char *file)
{
#ifdef WIN32
	return LoadLibrary(file);
#else
	return dlopen(file, RTLD_NOW);
#endif
}

static void *lib_sym(void *handle, const char *name)
{
#ifdef WIN32
	return GetProcAddress(handle, name);
#else
	return dlsym(handle, name);
#endif
}

static char *lib_error(void)
{
#ifdef WIN32
	/* return GetProcAddress(handle, name); */
	return "";
#else
	return (char *)dlerror();
#endif
}

static int lib_close(void *handle)
{
#ifdef WIN32
	return FreeLibrary(handle);
#else
	return dlclose(handle);
#endif
}

NES_FUNCTION(nesladl_loadlib)
{
	obj_t *cobj1=nes_getobj(N, &N->l, "1");
	NES_CFUNC cfunc;
#ifdef WIN32
	HINSTANCE l;
#else
	void *l;
#endif

	if (cobj1->val->type!=NT_STRING||cobj1->val->size<1) {
		nes_setstr(N, &N->r, "", "missing lib name", -1);
		return 0;
	}
	if ((l=lib_open(cobj1->val->d.str))==NULL) {
		nes_setstr(N, &N->r, "", lib_error(), -1);
		return 0;
	}
	if ((cfunc=(NES_CFUNC)lib_sym(l, "neslalib_init"))==NULL) {
		nes_setstr(N, &N->r, "", lib_error(), -1);
		lib_close(l);
		return 0;
	}
	nes_setnum(N, &N->r, "", cfunc(N));
	return 0;
}

int nesladl_register_all(nes_state *N)
{
	obj_t *tobj;

	tobj=nes_settable(N, &N->g, "dl");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj, "loadlib", (NES_CFUNC)nesladl_loadlib);
	return 0;
}

#endif /* HAVE_DL */
