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
#include "nesla/libwinapi.h"
#ifdef WIN32
#include <mmsystem.h>
#include <shellapi.h>
#include <stdio.h>

#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp stricmp
#define strncasecmp strnicmp

static int winsystem(WORD show_hide, const char *format, ...)
{
	DWORD exitcode=0;
	HANDLE hMyProcess=GetCurrentProcess();
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	char command[256];
	va_list ap;
	int pid;

	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	memset(command, 0, sizeof(command));
	va_start(ap, format);
	vsnprintf(command, sizeof(command)-1, format, ap);
	va_end(ap);
	si.cb=sizeof(si);
//	si.dwFlags=STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;
	si.dwFlags=STARTF_USESHOWWINDOW;
	si.wShowWindow=show_hide;
//	si.hStdInput=NULL;
//	si.hStdOutput=NULL;
//	si.hStdError=NULL;
	if (!CreateProcess(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
		MessageBox(NULL, command, "CreateProcess error", MB_ICONSTOP);
		return -1;
	}
	pid=pi.dwProcessId;
//	CloseHandle(si.hStdInput);
//	CloseHandle(si.hStdOutput);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return 0;
}

NES_FUNCTION(neslawinapi_createprocess)
{
	obj_t *cobj1=nes_getobj(N, &N->l, "1");
	obj_t *cobj2=nes_getobj(N, &N->l, "2");
	WORD show_hide=SW_SHOW;

	if (cobj2!=NULL&&cobj2->val!=NULL) {
		if ((cobj2->val->type==NT_BOOLEAN)||(cobj2->val->type==NT_NUMBER)) {
			if (cobj2->val->d.num==0) show_hide=SW_HIDE;
		}
	}
	if ((cobj1->val->type==NT_STRING)&&(cobj1->val->d.str!=NULL)) {
		winsystem(show_hide, cobj1->val->d.str);
	}
	nes_setnum(N, &N->r, "", 0);
	return 0;
}

NES_FUNCTION(neslawinapi_messagebox)
{
	obj_t *cobj3=nes_getobj(N, &N->l, "3");
	UINT uType=0;
	int rc;

	if (cobj3->val->type==NT_NUMBER) uType=(int)cobj3->val->d.num;
	rc=MessageBox(NULL, nes_getstr(N, &N->l, "1"), nes_getstr(N, &N->l, "2"), uType);
	nes_setnum(N, &N->r, "", rc);
	return 0;
}

NES_FUNCTION(neslawinapi_playsound)
{
	obj_t *cobj1=nes_getobj(N, &N->l, "1");
	obj_t *cobj2=nes_getobj(N, &N->l, "2");
	int opt=SND_ASYNC|SND_NODEFAULT|SND_FILENAME;
	int rc=0;

	if ((cobj1->val->type!=NT_STRING)||(cobj1->val->size<1)) return 0;
	if (cobj2->val->type==NT_NUMBER) opt=(int)cobj2->val->d.num;
	rc=sndPlaySound(cobj1->val->d.str, opt);
	nes_setnum(N, &N->r, "", rc);
	return 0;
}

NES_FUNCTION(neslawinapi_shellexecute)
{
	ShellExecute(NULL, "open", nes_getstr(N, &N->l, "1"), NULL, NULL, SW_SHOWMAXIMIZED);
	nes_setnum(N, &N->r, "", 0);
	return 0;
}
#endif

int neslawinapi_register_all(nes_state *N)
{
#ifdef WIN32
	obj_t *tobj;

	tobj=nes_settable(N, &N->g, "win");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj, "CreateProcess", (NES_CFUNC)neslawinapi_createprocess);
	nes_setcfunc(N, tobj, "MessageBox",    (NES_CFUNC)neslawinapi_messagebox);
	nes_setcfunc(N, tobj, "PlaySound",     (NES_CFUNC)neslawinapi_playsound);
	nes_setcfunc(N, tobj, "ShellExecute",  (NES_CFUNC)neslawinapi_shellexecute);
#endif
	return 0;
}

#ifdef PIC
DllExport int neslalib_init(nes_state *N)
{
	neslawinapi_register_all(N);
	return 0;
}
#endif
