/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2015 Dan Cahill

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
#include "nsp/nsplib.h"
#include "base.h"

int nspbase_register_all(nsp_state *N)
{
	obj_t *tobj;

	tobj = nsp_settable(N, &N->g, "base64");
	tobj->val->attr |= NST_HIDDEN;
	nsp_setcfunc(N, tobj, "decode", (NSP_CFUNC)libnsp_base_base64_decode);
	nsp_setcfunc(N, tobj, "encode", (NSP_CFUNC)libnsp_base_base64_encode);

	nsp_setcfunc(N, &N->g, "dirlist", (NSP_CFUNC)libnsp_base_dirlist);

	tobj = nsp_getobj(N, &N->g, "file");
	if (!nsp_istable(tobj)) tobj = nsp_settable(N, &N->g, "file");
	tobj->val->attr |= NST_HIDDEN;
	//nsp_setcfunc(N, tobj, "append", (NSP_CFUNC)libnsp_base_file_writeall);
	//nsp_setcfunc(N, tobj, "chdir", (NSP_CFUNC)libnsp_base_file_chdir);
	//nsp_setcfunc(N, tobj, "mkdir", (NSP_CFUNC)libnsp_base_file_mkdir);
	//nsp_setcfunc(N, tobj, "readall", (NSP_CFUNC)libnsp_base_file_readall);
	//nsp_setcfunc(N, tobj, "rename", (NSP_CFUNC)libnsp_base_file_rename);
	//nsp_setcfunc(N, tobj, "stat", (NSP_CFUNC)libnsp_base_file_stat);
	//nsp_setcfunc(N, tobj, "unlink", (NSP_CFUNC)libnsp_base_file_unlink);
	//nsp_setcfunc(N, tobj, "writeall", (NSP_CFUNC)libnsp_base_file_writeall);

	tobj = nsp_settable(N, &N->g, "pipe");
	tobj->val->attr |= NST_HIDDEN;
	nsp_setcfunc(N, tobj, "open", (NSP_CFUNC)libnsp_base_pipe_open);
	nsp_setcfunc(N, tobj, "read", (NSP_CFUNC)libnsp_base_pipe_read);
	nsp_setcfunc(N, tobj, "write", (NSP_CFUNC)libnsp_base_pipe_write);
	nsp_setcfunc(N, tobj, "close", (NSP_CFUNC)libnsp_base_pipe_close);

	nsp_setcfunc(N, &N->g, "rot13", (NSP_CFUNC)libnsp_base_rot13);

	tobj = nsp_settable(N, &N->g, "regex");
	tobj->val->attr |= NST_HIDDEN;
	nsp_setcfunc(N, tobj, "match", (NSP_CFUNC)libnsp_regex_match);
	nsp_setcfunc(N, tobj, "replace", (NSP_CFUNC)libnsp_regex_replace);


	tobj = nsp_settable(N, &N->g, "sort");
	tobj->val->attr |= NST_HIDDEN;
	nsp_setcfunc(N, tobj, "byname", (NSP_CFUNC)libnsp_base_sort_byname);
	nsp_setcfunc(N, tobj, "bykey", (NSP_CFUNC)libnsp_base_sort_bykey);

#ifdef HAVE_THREADS
	tobj = nsp_settable(N, &N->g, "thread");
	tobj->val->attr |= NST_HIDDEN;
	nsp_setcfunc(N, tobj, "thread", (NSP_CFUNC)libnsp_base_thread_thread);
	nsp_setcfunc(N, tobj, "start",  (NSP_CFUNC)libnsp_base_thread_start);
	nsp_setcfunc(N, tobj, "finish", (NSP_CFUNC)libnsp_base_thread_finish);
#endif

#ifdef WIN32
	tobj = nsp_settable(N, &N->g, "win");
	tobj->val->attr |= NST_HIDDEN;
	nsp_setcfunc(N, tobj, "Beep", (NSP_CFUNC)libnsp_winapi_beep);
	nsp_setcfunc(N, tobj, "CreateProcess", (NSP_CFUNC)libnsp_winapi_createprocess);
	nsp_setcfunc(N, tobj, "MessageBox", (NSP_CFUNC)libnsp_winapi_messagebox);
	nsp_setcfunc(N, tobj, "PlaySound", (NSP_CFUNC)libnsp_winapi_playsound);
	nsp_setcfunc(N, tobj, "ShellExecute", (NSP_CFUNC)libnsp_winapi_shellexecute);
#endif

	return 0;
}

#ifdef PIC
DllExport int nsplib_init(nsp_state *N)
{
	nspbase_register_all(N);
	return 0;
}
#endif