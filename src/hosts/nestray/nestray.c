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
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <direct.h>
#include <stdio.h>
#include <sys/stat.h>
#include "resource.h"
#include "nesla/nesla.h"
#include "nesla/libext.h"
#include "nesla/libmath.h"
#include "nesla/libodbc.h"
#include "nesla/libtcp.h"

#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp stricmp
#define strncasecmp strnicmp

static nes_state *N;
static HWND hDLG;
static int iconstatus=0;
static HINSTANCE hInst;
static UINT tc;
static time_t lastpoll;
static time_t lastfile;
static int index;

static char *conffile="nestray.conf";

static void preppath(nes_state *N, char *name)
{
	char buf[1024];
	char *p;
	unsigned int j;

	p=name;
	if ((name[0]=='/')||(name[0]=='\\')||(name[1]==':')) {
		/* it's an absolute path.... probably... */
		strncpy(buf, name, sizeof(buf));
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
	nes_setstr(N, &N->g, "_filename", p, strlen(p));
	nes_setstr(N, &N->g, "_filepath", buf, strlen(buf));
	return;
}

static int nes_messagebox(nes_state *N)
{
	MessageBox(NULL, nes_getstr(N, &N->l, "1"), nes_getstr(N, &N->l, "2"), MB_OK);
	nes_setnum(N, &N->r, "", 0);
	return 0;
}

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
	si.dwFlags=STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;
	si.wShowWindow=show_hide;
	si.hStdInput=NULL;
	si.hStdOutput=NULL;
	si.hStdError=NULL;
	if (!CreateProcess(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
		return -1;
	}
	pid=pi.dwProcessId;
	CloseHandle(si.hStdInput);
	CloseHandle(si.hStdOutput);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return 0;
}

static BOOL TrayMessage(DWORD dwMessage)
{
        BOOL res;
	HICON hIcon;
	NOTIFYICONDATA tnd;

	if (iconstatus) {
                hIcon=LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON2), IMAGE_ICON, 16, 16, 0);
	} else {
                hIcon=LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);
	}
	tnd.cbSize		= sizeof(NOTIFYICONDATA);
	tnd.hWnd		= hDLG;
	tnd.uID			= 0;
	tnd.uFlags		= NIF_MESSAGE|NIF_ICON|NIF_TIP;
	tnd.uCallbackMessage	= IDM_STATE;
	tnd.hIcon		= hIcon;
	snprintf(tnd.szTip, sizeof(tnd.szTip)-1, "Nesla SysTray Host");
	res=Shell_NotifyIcon(dwMessage, &tnd);
	if (hIcon) DestroyIcon(hIcon);
	return res;
}

static void TrayIcon(int newstatus)
{
	if (iconstatus!=newstatus) {
		iconstatus=newstatus;
		TrayMessage(NIM_MODIFY);
	}
	return;
}

static obj_t *getindex(obj_t *tobj, int i)
{
	obj_t *cobj;

	if (tobj->val->type!=NT_TABLE) return (FALSE);
	for (tobj=tobj->val->d.table;tobj;tobj=tobj->next) {
		if (tobj->val->type!=NT_TABLE) continue;
		cobj=nes_getobj(N, tobj, "index");
		if (cobj->val->type!=NT_NUMBER) {
			cobj=nes_getobj(N, tobj, "table");
			if (cobj->val->type!=NT_TABLE) continue;
			if ((cobj=getindex(cobj, i))==NULL) continue;
			return cobj;
		}
		if (cobj->val->d.num==i) return tobj;
	}
	return NULL;
}

static BOOL submenu(HMENU hMenu, obj_t *tobj)
{
	obj_t *cobj;
	char *p;
	HMENU hMenuSub;
	BOOL bRet;

	if (tobj->val->type!=NT_TABLE) return (FALSE);
	for (tobj=tobj->val->d.table;tobj;tobj=tobj->next) {
		if (tobj->val->type!=NT_TABLE) continue;
		cobj=nes_getobj(N, tobj, "type");
		if (cobj->val->type==NT_NULL) continue;
		p=nes_tostr(N, cobj);
		if (strcasecmp(p, "menu")==0) {
			if ((hMenuSub=CreateMenu())!=NULL) {
				submenu(hMenuSub, nes_getobj(N, tobj, "table"));
				bRet=AppendMenu(hMenu, MF_POPUP|MF_BYPOSITION, (DWORD)hMenuSub, nes_getstr(N, tobj, "name"));
			}
		} else if (strcasecmp(p, "separator")==0) {
			bRet=AppendMenu(hMenu, MF_SEPARATOR, 0, "");
		} else {
			p=nes_getstr(N, tobj, "name");
			bRet=AppendMenu(hMenu, MF_STRING, IDM_STATE+index, p);
			nes_setnum(N, tobj, "index", index++);
		}
	}
	if (!bRet) {
		DestroyMenu(hMenu);
		return (FALSE);
	}
	return (FALSE);
}

static BOOL APIENTRY DrawPopupMenu(POINT point)
{
	HMENU hMenu=CreatePopupMenu(); 
	obj_t *mobj=nes_settable(N, &N->g, "MENUITEMS");

	if (!hMenu) return (FALSE);
	index=0;
	submenu(hMenu, mobj);
	SetForegroundWindow(hDLG);
	TrackPopupMenu(hMenu, TPM_LEFTBUTTON|TPM_RIGHTBUTTON, point.x, point.y, 0, hDLG, NULL);
	PostMessage(hDLG, WM_USER, 0, 0);
	return (FALSE);
}

static BOOL CALLBACK ExecPopupMenu(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	obj_t *menuobj=nes_settable(N, &N->g, "MENUITEMS");
	obj_t *cobj, *cobj2, *tobj;
	UINT nNewMode;
	POINT point;
	char *p;

	hDLG=hDlg;
	if (uMsg==tc) {
		TrayMessage(NIM_ADD);
		return DefWindowProc(hDlg, uMsg, wParam, lParam);
	}
	switch (uMsg) {
	case WM_INITDIALOG:
		TrayMessage(NIM_ADD);
		break;
	case WM_COMMAND:
		nNewMode=GET_WM_COMMAND_ID(wParam, lParam)-(IDM_STATE);
		tobj=getindex(menuobj, nNewMode);
		if (nes_isnull(tobj)) break;
		if (tobj->val->type!=NT_TABLE) break;
		cobj=nes_getobj(N, tobj, "type");
		if ((cobj->val->type!=NT_STRING)||(cobj->val->d.str==NULL)) break;
		if (strcasecmp(cobj->val->d.str, "exit")==0) {
			PostMessage(hDLG, WM_CLOSE, 0, 0);
		} else if (strcasecmp(cobj->val->d.str, "message")==0) {
			TrayIcon(1);
			cobj=nes_getobj(N, tobj, "text");
			if ((cobj->val->type==NT_STRING)&&(cobj->val->d.str!=NULL)) {
				cobj2=nes_getobj(N, tobj, "title");
				if ((cobj2->val->type==NT_STRING)&&(cobj2->val->d.str!=NULL)) p=cobj2->val->d.str; else p="";
				MessageBox(NULL, cobj->val->d.str, p, MB_OK);
			}
			TrayIcon(0);
		} else if (strcasecmp(cobj->val->d.str, "script")==0) {
			TrayIcon(1);
			cobj=nes_getobj(N, tobj, "command");
			if ((cobj->val->type!=NT_STRING)||(cobj->val->d.str==NULL)) break;
			nes_exec(N, cobj->val->d.str);
			if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
			TrayIcon(0);
		} else if (strcasecmp(cobj->val->d.str, "ShellExecute")==0) {
			TrayIcon(1);
			cobj=nes_getobj(N, tobj, "command");
			if ((cobj->val->type!=NT_STRING)||(cobj->val->d.str==NULL)) break;
			ShellExecute(NULL, "open", cobj->val->d.str, NULL, NULL, SW_SHOWMAXIMIZED);
			TrayIcon(0);
		} else if (strcasecmp(cobj->val->d.str, "CreateProcess")==0) {
			TrayIcon(1);
			cobj=nes_getobj(N, tobj, "command");
			if ((cobj->val->type!=NT_STRING)||(cobj->val->d.str==NULL)) break;
			winsystem(SW_SHOW, cobj->val->d.str);
			TrayIcon(0);
		}
		break;
	case IDM_STATE:
		switch (lParam) {
		case WM_LBUTTONDOWN:
			break;
		case WM_RBUTTONDOWN:
			GetCursorPos(&point);
			DrawPopupMenu(point);
			break;
		default:
			break;
		}
		break;
	case WM_CLOSE:
	case WM_QUIT:
	case WM_DESTROY:
		TrayMessage(NIM_DELETE);
		EndDialog(hDLG, TRUE);
		exit(0);
		break;
	default:
		return(FALSE);
	}
	return(TRUE);
}

void CALLBACK UpdateNotifyProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	obj_t *cobj=nes_getobj(N, &N->g, "cron");
	struct stat sb;
	time_t t;

	if (stat(conffile, &sb)==0) {
		if (sb.st_mtime>lastfile) {
			lastfile=sb.st_mtime;
			nes_freetable(N, nes_settable(N, &N->g, "MENUITEMS"));
			nes_execfile(N, conffile);
			if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
			MessageBox(NULL, "A change in the configuration was detected and updated.", "Configuration File Updated", MB_OK);
		}
	}
	t=time(NULL);
	if ((t%60)<(lastpoll%60)) {
		lastpoll=t;
		TrayIcon(1);
		if (cobj->val->type==NT_NFUNC) {
			nes_exec(N, "cron();");
			if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
		}
		TrayIcon(0);
	} else {
		lastpoll=t;
	}
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HANDLE hMutex=CreateMutex(NULL, FALSE, "NESTRAY_MUTEX");
	obj_t *mobj;
	struct stat sb;

	hInst=hInstance;
	if ((hMutex==NULL)||(GetLastError()==ERROR_ALREADY_EXISTS)) {
		if (hMutex) CloseHandle(hMutex);
		return 0;
	}
	if ((N=nes_newstate())==NULL) return -1;
	N->debug=0;
	neslaext_register_all(N);
	neslamath_register_all(N);
	neslaodbc_register_all(N);
	neslatcp_register_all(N);
	preppath(N, conffile);
	nes_setcfunc(N, &N->g, "MessageBox", (void *)nes_messagebox);
	if (stat(conffile, &sb)==0) {
		lastfile=sb.st_mtime;
		mobj=nes_settable(N, &N->g, "MENUITEMS");
		mobj->val->attr^=NST_AUTOSORT;
		nes_execfile(N, conffile);
		if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
	} else {
		lastfile=0;
	}
	lastpoll=time(NULL);

	tc=RegisterWindowMessage("TaskbarCreated");
	SetTimer(0, 1234, 1000, UpdateNotifyProc);
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_NULLDIALOG), NULL, ExecPopupMenu);
	CloseHandle(hMutex);

	nes_endstate(N);
	return 0;
}
#endif
