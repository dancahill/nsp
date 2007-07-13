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
#include "nesla/libzip.h"

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

void new_menu(nes_state *N);
void init_stuff(nes_state *N);
int winsystem(WORD show_hide, const char *format, ...);
obj_t *getindex(obj_t *tobj, int i);
BOOL submenu(HMENU hMenu, obj_t *tobj);

/* WIN32 DIALOG STUFF */
static BOOL TrayMessage(DWORD dwMessage)
{
	obj_t *mobj=nes_getobj(N, &N->g, "PROGNAME");
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
	snprintf(tnd.szTip, sizeof(tnd.szTip)-1, "%s", nes_isstr(mobj)?nes_tostr(N, mobj):"Nesla SysTray Host");
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

static BOOL APIENTRY DrawPopupMenu(POINT point)
{
	HMENU hMenu=CreatePopupMenu(); 
	obj_t *mobj=nes_getobj(N, &N->g, "MENUITEMS");

	if (!nes_istable(mobj)) {
		new_menu(N);
		MessageBox(NULL, "missing config", "Script Error", MB_ICONSTOP);
	}
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
	obj_t *mobj=nes_getobj(N, &N->g, "MENUITEMS");
	obj_t *cobj, *cobj2, *tobj;
	UINT nNewMode;
	POINT point;
	char *p;

	if (!nes_istable(mobj)) {
		new_menu(N);
		MessageBox(NULL, "missing config", "Script Error", MB_ICONSTOP);
	}
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
		tobj=getindex(mobj, nNewMode);
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
		} else if (strcasecmp(cobj->val->d.str, "CreateProcess")==0) {
			TrayIcon(1);
			cobj=nes_getobj(N, tobj, "command");
			if ((cobj->val->type==NT_STRING)&&(cobj->val->d.str!=NULL)) {
				winsystem(SW_SHOW, cobj->val->d.str);
			}
			TrayIcon(0);
		} else if (strcasecmp(cobj->val->d.str, "ShellExecute")==0) {
			TrayIcon(1);
			cobj=nes_getobj(N, tobj, "command");
			if ((cobj->val->type==NT_STRING)&&(cobj->val->d.str!=NULL)) {
				ShellExecute(NULL, "open", cobj->val->d.str, NULL, NULL, SW_SHOWMAXIMIZED);
			}
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
		break;
	default:
		return(FALSE);
	}
	return(TRUE);
}

void CALLBACK UpdateNotifyProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	obj_t *cobj;
	struct stat sb;
	time_t t;

	if (stat(conffile, &sb)==0) {
		if (sb.st_mtime>lastfile) {
			lastfile=sb.st_mtime;
			/* sometimes it's easier to destroy the world and start over... */
			nes_endstate(N);
			if ((N=nes_newstate())==NULL) return;
			init_stuff(N);
			if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
			cobj=nes_getobj(N, &N->g, "onreload");
			TrayIcon(1);
			if (cobj->val->type==NT_NFUNC) {
				nes_exec(N, "onreload();");
				if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
			}
			TrayIcon(0);
		}
	} else {
		/* no config? */
		cobj=nes_getobj(N, &N->g, "MENUITEMS");
		if (!nes_istable(cobj)) {
			new_menu(N);
			MessageBox(NULL, "missing config", "Script Error", MB_ICONSTOP);
		}
	}
	t=time(NULL);
	if ((t%60)<(lastpoll%60)) {
		lastpoll=t;
		TrayIcon(1);
		cobj=nes_getobj(N, &N->g, "cron");
		if (cobj->val->type==NT_NFUNC) {
			nes_exec(N, "cron();");
			if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
		}
		TrayIcon(0);
	} else {
		lastpoll=t;
	}
}
/* END WIN32 DIALOG STUFF */

/* START WIN32 MINI API */
static int nes_createprocess(nes_state *N)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);

	winsystem(SW_SHOW, cobj1->val->d.str);
	nes_setnum(N, &N->r, "", 0);
	return 0;
}

static int nes_messagebox(nes_state *N)
{
	obj_t *cobj3=nes_getiobj(N, &N->l, 3);
	UINT uType=0;
	int rc;

	if (cobj3->val->type==NT_NUMBER) uType=(int)cobj3->val->d.num;
	rc=MessageBox(NULL, nes_getstr(N, &N->l, "1"), nes_getstr(N, &N->l, "2"), uType);
	nes_setnum(N, &N->r, "", rc);
	return 0;
}

static int nes_shellexecute(nes_state *N)
{
	ShellExecute(NULL, "open", nes_getstr(N, &N->l, "1"), NULL, NULL, SW_SHOWMAXIMIZED);
	nes_setnum(N, &N->r, "", 0);
	return 0;
}

static BOOL CALLBACK TextInputDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	char buffer[512];

	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowText(hDlg, nes_tostr(N, cobj2));
		SetDlgItemText(hDlg, IDC_EDIT1, nes_tostr(N, cobj1));
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemText(hDlg, IDC_EDIT1, buffer, sizeof(buffer)-1);
			EndDialog(hDlg, 0);
			nes_setstr(N, &N->r, "", buffer, strlen(buffer));
			return (TRUE);
		case IDCANCEL:
			EndDialog(hDlg, 0);
			nes_setnum(N, &N->r, "", -1);
			return (TRUE);
		}
		break;
	case WM_CLOSE:
	case WM_QUIT:
	case WM_DESTROY:
		EndDialog(hDlg, 0);
		break;
	default:
		return(FALSE);
	}
	return(TRUE);
}

static int nes_textinput(nes_state *N)
{
	TrayIcon(1);
	DialogBox(hInst, MAKEINTRESOURCE(IDD_TEXTINPUT1), NULL, TextInputDlgProc);
	TrayIcon(0);
	return 0;
}

void CALLBACK TrayNotifyProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	PostMessage(hwnd, WM_CLOSE, 0, 0);
}

static BOOL CALLBACK TrayNoticeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	obj_t *cobj3=nes_getiobj(N, &N->l, 3);
	int t;
	RECT rc;
	int x, y;
	int w, h;
	int tbh;

	switch (uMsg) {
	case WM_INITDIALOG:
		x=GetSystemMetrics(SM_CXSCREEN);
		y=GetSystemMetrics(SM_CYSCREEN);
		GetWindowRect(hDlg, &rc);
		w=rc.right-rc.left;
		h=rc.bottom-rc.top;
		/* how do we get the height of the taskbar? */
		tbh=30;
		MoveWindow(hDlg, x-w, y-h-tbh, w, h, TRUE);
		SetWindowText(hDlg, nes_tostr(N, cobj2));
		SetDlgItemText(hDlg, IDC_STATIC1, nes_tostr(N, cobj1));
		t=nes_isnum(cobj3)?(int)nes_tonum(N, cobj3):5;
		SetFocus(hDlg);
		if (t) SetTimer(hDlg, 12345, t*1000, TrayNotifyProc);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hDlg, 0);
			nes_setnum(N, &N->r, "", 0);
			return (TRUE);
		}
		break;
	case WM_CLOSE:
	case WM_QUIT:
	case WM_DESTROY:
		EndDialog(hDlg, 0);
		break;
	default:
		return(FALSE);
	}
	return(TRUE);
}

static int nes_traynotice(nes_state *N)
{
	TrayIcon(1);
	DialogBox(hInst, MAKEINTRESOURCE(IDD_TRAYNOTICE1), NULL, TrayNoticeDlgProc);
	// ABS_ALWAYSONTOP ?
	TrayIcon(0);
	return 0;
}
/* END WIN32 MINI API */

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

void new_menu(nes_state *N)
{
	nes_exec(N,
"global MENUITEMS = {\n"\
"{ name=\"NullLogic &Nesla\", type=\"ShellExecute\",  command=\"http://nesla.sourceforge.net/nesla/\"         },\n"\
"{ name=\"separator\",        type=\"separator\"                                                              },\n"\
"{ name=\"&Configuration\",   type=\"CreateProcess\", command=\"rundll32 shell32,OpenAs_RunDLL nestray.conf\" },\n"\
"{ name=\"E&xit\",            type=\"Exit\",          command=\"Exit\"                                        }\n"\
"};\n\n"\
"#function onload() {\n\treturn;\n}\n\n"\
"#function onreload() {\n\treturn;\n}\n\n"\
"#function cron() {\n\treturn;\n}\n\n"\
"#function onexit() {\n\treturn;\n}\n"
	);
}

void init_stuff(nes_state *N)
{
	char tmpbuf[MAX_OBJNAMELEN+1];
	obj_t *mobj;
	obj_t *tobj;
	int i;
	char *p;
	struct stat sb;

	N->debug=0;
	neslaext_register_all(N);
	neslamath_register_all(N);
	neslaodbc_register_all(N);
	neslatcp_register_all(N);
	neslazip_register_all(N);
	/* add env */
	tobj=nes_settable(N, &N->g, "_ENV");
	for (i=0;environ[i]!=NULL;i++) {
		strncpy(tmpbuf, environ[i], MAX_OBJNAMELEN);
		p=strchr(tmpbuf, '=');
		if (!p) continue;
		*p='\0';

		/* env vars should ignore case, so force uniformity */
		p=tmpbuf; while (*p) *p++=toupper(*p);

		p=strchr(environ[i], '=')+1;
		nes_setstr(N, tobj, tmpbuf, p, strlen(p));
	}
	preppath(N, conffile);
	nes_setcfunc(N, &N->g, "CreateProcess", (void *)nes_createprocess);
	nes_setcfunc(N, &N->g, "MessageBox",    (void *)nes_messagebox);
	nes_setcfunc(N, &N->g, "ShellExecute",  (void *)nes_shellexecute);
	nes_setcfunc(N, &N->g, "TextInput",     (void *)nes_textinput);
	nes_setcfunc(N, &N->g, "TrayNotice",    (void *)nes_traynotice);
	if (stat(conffile, &sb)==0) {
		lastfile=sb.st_mtime;
		nes_execfile(N, conffile);
		if (N->err) {
			MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
		} else {
			mobj=nes_getobj(N, &N->g, "MENUITEMS");
			if (!nes_istable(mobj)) new_menu(N);
		}
	} else {
		lastfile=0;
	}
	lastpoll=time(NULL);
	return;
}

int winsystem(WORD show_hide, const char *format, ...)
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

obj_t *getindex(obj_t *tobj, int i)
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

BOOL submenu(HMENU hMenu, obj_t *tobj)
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

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HANDLE hMutex;
	obj_t *cobj;

	hInst=hInstance;
	if ((N=nes_newstate())==NULL) return -1;
	init_stuff(N);
	cobj=nes_getobj(N, &N->g, "MUTEXNAME");
	if ((cobj->val->type==NT_STRING)&&(cobj->val->size>0)) {
		hMutex=CreateMutex(NULL, FALSE, cobj->val->d.str);
	} else {
		hMutex=CreateMutex(NULL, FALSE, "NESTRAY_MUTEX");
	}
	if ((hMutex==NULL)||(GetLastError()==ERROR_ALREADY_EXISTS)) {
		if (hMutex) CloseHandle(hMutex);
		nes_endstate(N);
		return 0;
	}
	cobj=nes_getobj(N, &N->g, "onload");
	if (cobj->val->type==NT_NFUNC) {
		nes_exec(N, "onload();");
		if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
	}
	tc=RegisterWindowMessage("TaskbarCreated");
	SetTimer(0, 1234, 1000, UpdateNotifyProc);
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_NULLDIALOG), NULL, ExecPopupMenu);
	CloseHandle(hMutex);
	cobj=nes_getobj(N, &N->g, "onexit");
	if (cobj->val->type==NT_NFUNC) {
		nes_exec(N, "onexit();");
		if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
	}
	nes_endstate(N);
	return 0;
}
#endif
