/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2009 Dan Cahill

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
#include "nesla/libnesla.h"
#include "nesla/libcdb.h"
#include "nesla/libcrypt.h"
#include "nesla/libdl.h"
#include "nesla/libext.h"
#include "nesla/libmath.h"
#include "nesla/libodbc.h"
#include "nesla/libpipe.h"
#include "nesla/libregex.h"
#include "nesla/libtcp.h"
#include "nesla/libwinapi.h"
#include "nesla/libzip.h"
#include <direct.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <windowsx.h>
#include <sys/stat.h>
#include "resource.h"

//#pragma comment(lib, "dnsapi.lib")
#pragma comment(lib, "winmm.lib")
//#pragma comment(lib, "xyssl.lib")

#define CONFFILE "nestray.conf"
#define IDT_TIMER1 6901
#define IDT_TIMER2 6902

typedef struct GLOBALS {
	HWND wnd;
	HINSTANCE instance;
	time_t lastpoll;
	time_t lastfile;
	int iconstatus;
	int index;
	char noticetext[240];
} GLOBALS;

GLOBALS G;
nes_state *N;

NES_FUNCTION(nes_textinput);
NES_FUNCTION(nes_passinput);
NES_FUNCTION(nes_traynotice);

void preppath(nes_state *N, char *name)
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
	nes_setstr(N, &N->g, "_filename", p, -1);
	nes_setstr(N, &N->g, "_filepath", buf, -1);
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
	neslacdb_register_all(N);
	neslacrypto_register_all(N);
	nesladl_register_all(N);
	neslaext_register_all(N);
	neslamath_register_all(N);
	neslaodbc_register_all(N);
	neslapipe_register_all(N);
	neslaregex_register_all(N);
	neslatcp_register_all(N);
	neslawinapi_register_all(N);
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
		nes_setstr(N, tobj, tmpbuf, p, -1);
	}
	preppath(N, CONFFILE);
	nes_setcfunc(N, &N->g, "TextInput",  nes_textinput);
	nes_setcfunc(N, &N->g, "PassInput",  nes_passinput);
	nes_setcfunc(N, &N->g, "TrayNotice", nes_traynotice);
	if (stat(CONFFILE, &sb)==0) {
		G.lastfile=sb.st_mtime;
		nes_execfile(N, CONFFILE);
		if (N->err) {
			MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
		} else {
			mobj=nes_getobj(N, &N->g, "MENUITEMS");
			if (!nes_istable(mobj)) new_menu(N);
		}
	} else {
		G.lastfile=0;
	}
	G.lastpoll=time(NULL);
	return;
}

int winsystem(WORD show_hide, const char *format, ...)
{
//	DWORD exitcode=0;
//	HANDLE hMyProcess=GetCurrentProcess();
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	char command[512];
	va_list ap;
//	int pid;

	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&command, sizeof(command));
	va_start(ap, format);
	_vsnprintf(command, sizeof(command)-1, format, ap);
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
//	pid=pi.dwProcessId;
//	CloseHandle(si.hStdInput);
//	CloseHandle(si.hStdOutput);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return 0;
}

obj_t *getindex(obj_t *tobj, int i)
{
	obj_t *cobj;

	if (!nes_istable(tobj)) return NULL;
	for (tobj=tobj->val->d.table.f;tobj;tobj=tobj->next) {
		if (!nes_istable(tobj)) continue;
		cobj=nes_getobj(N, tobj, "index");
		if (!nes_isnum(cobj)) {
			cobj=nes_getobj(N, tobj, "table");
			if (!nes_istable(cobj)) continue;
			if ((cobj=getindex(cobj, i))==NULL) continue;
			return cobj;
		}
		if (nes_tonum(N, cobj)==i) return tobj;
	}
	return NULL;
}

BOOL IconNotify(DWORD dwMessage)
{
	obj_t *mobj=nes_getobj(N, &N->g, "PROGNAME");
        BOOL res;
	HICON hIcon;
	NOTIFYICONDATA tnd;

	if (G.iconstatus) {
		hIcon=LoadImage(G.instance, MAKEINTRESOURCE(IDI_ICON2), IMAGE_ICON, 16, 16, 0);
	} else {
		hIcon=LoadImage(G.instance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);
	}
	tnd.cbSize		= sizeof(tnd);
	tnd.hWnd		= G.wnd;
	tnd.uID			= 0;
	tnd.uFlags		= NIF_MESSAGE|NIF_ICON|NIF_TIP;
	tnd.uCallbackMessage	= IDM_STATE;
	tnd.hIcon		= hIcon;
	_snprintf(tnd.szTip, sizeof(tnd.szTip)-1, "%s", nes_isstr(mobj)?nes_tostr(N, mobj):"Nesla SysTray Host");
	res=Shell_NotifyIcon(dwMessage, &tnd);
	if (hIcon) DestroyIcon(hIcon);
	return res;
}

void IconStatus(int newstatus)
{
	if (G.iconstatus!=newstatus) {
		G.iconstatus=newstatus;
		IconNotify(NIM_MODIFY);
	}
	return;
}

void PopupTimer(void)
{
	obj_t *cobj;
	struct stat sb;
	time_t t;

	if (stat(CONFFILE, &sb)==0&&sb.st_mtime>G.lastfile) {
		G.lastfile=sb.st_mtime;
		/* sometimes it's easier to destroy the world and start over... */
		nes_endstate(N);
		if ((N=nes_newstate())==NULL) return;
		init_stuff(N);
		if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
		cobj=nes_getobj(N, nes_getobj(N, &N->g, "NesTray"), "onreload");
		IconStatus(1);
		if (nes_typeof(cobj)==NT_NFUNC) {
			nes_exec(N, "NesTray.onreload();");
			if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
		}
		IconStatus(0);
	} else {
		/* no config? */
		cobj=nes_getobj(N, &N->g, "MENUITEMS");
		if (!nes_istable(cobj)) {
			new_menu(N);
			MessageBox(NULL, "missing config", "Script Error", MB_ICONSTOP);
		}
	}
	t=time(NULL);
	if ((t%60)<(G.lastpoll%60)) {
		G.lastpoll=t;
		IconStatus(1);
		cobj=nes_getobj(N, nes_getobj(N, &N->g, "NesTray"), "ontimer");
		if (nes_typeof(cobj)==NT_NFUNC) {
			nes_exec(N, "NesTray.ontimer();");
			if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
		}
		IconStatus(0);
	} else {
		G.lastpoll=t;
	}
	return;
}

BOOL PopupMenuDrawSub(HMENU hMenu, obj_t *tobj)
{
	obj_t *cobj;
	char *p;
	HMENU hMenuSub;
	BOOL ret;

	if (!nes_istable(tobj)) return FALSE;
	for (tobj=tobj->val->d.table.f;tobj;tobj=tobj->next) {
		if (!nes_istable(tobj)) continue;
		cobj=nes_getobj(N, tobj, "type");
		if (nes_isnull(cobj)) continue;
		p=nes_tostr(N, cobj);
		if (stricmp(p, "menu")==0) {
			if ((hMenuSub=CreateMenu())==NULL) continue;
			PopupMenuDrawSub(hMenuSub, nes_getobj(N, tobj, "table"));
			ret=AppendMenu(hMenu, MF_POPUP|MF_BYPOSITION, (DWORD)hMenuSub, nes_getstr(N, tobj, "name"));
		} else if (stricmp(p, "separator")==0) {
			ret=AppendMenu(hMenu, MF_SEPARATOR, 0, "");
		} else {
			p=nes_getstr(N, tobj, "name");
			ret=AppendMenu(hMenu, MF_STRING, IDM_STATE+G.index, p);
			nes_setnum(N, tobj, "index", G.index++);
		}
	}
	if (!ret) DestroyMenu(hMenu);
	return FALSE;
}

BOOL APIENTRY PopupMenuDraw(POINT point)
{
	HMENU hMenu;
	obj_t *mobj;

	mobj=nes_getobj(N, &N->g, "MENUITEMS");
	if (!nes_istable(mobj)) {
		new_menu(N);
		MessageBox(NULL, "missing config", "Script Error", MB_ICONSTOP);
	}
	hMenu=CreatePopupMenu();
	if (hMenu) {
		G.index=0;
		PopupMenuDrawSub(hMenu, mobj);
		SetForegroundWindow(G.wnd);
		TrackPopupMenu(hMenu, TPM_LEFTBUTTON|TPM_RIGHTBUTTON, point.x, point.y, 0, G.wnd, NULL);
		PostMessage(G.wnd, WM_USER, 0, 0);
	}
	return FALSE;
}

BOOL CALLBACK PopupMenuExec(HWND wnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int lock=0;

	G.wnd=wnd;
	switch (uMsg) {
	case WM_INITDIALOG:
		IconNotify(NIM_ADD);
		SetTimer(wnd, IDT_TIMER1, 1000, NULL);
		break;
	case WM_TIMER:
		if (lock>0) return FALSE;
		lock++;
		PopupTimer();
		lock--;
		break;
	case WM_COMMAND: {
		obj_t *cobj, *cobj2, *mobj, *tobj;
		UINT nNewMode;
		char *p, *t;

		while (lock>0) Sleep(1);
		lock++;
		mobj=nes_getobj(N, &N->g, "MENUITEMS");
		if (!nes_istable(mobj)) {
			new_menu(N);
			MessageBox(NULL, "missing config", "Script Error", MB_ICONSTOP);
		}
		nNewMode=GET_WM_COMMAND_ID(wParam, lParam)-(IDM_STATE);
		tobj=getindex(mobj, nNewMode);
		if (!nes_istable(tobj)) {
			lock--;
			break;
		}
		cobj=nes_getobj(N, tobj, "type");
		if (!nes_isstr(cobj)||cobj->val->size<1) {
			lock--;
			break;
		}
		t=nes_tostr(N, cobj);
		if (stricmp(t, "exit")==0) {
			PostMessage(wnd, WM_CLOSE, 0, 0);
			lock--;
			break;
		}
		IconStatus(1);
		if (stricmp(t, "message")==0) {
			cobj=nes_getobj(N, tobj, "text");
			if (nes_isstr(cobj)&&cobj->val->size>0) {
				cobj2=nes_getobj(N, tobj, "title");
				if ((cobj2->val->type==NT_STRING)&&(cobj2->val->d.str!=NULL)) p=cobj2->val->d.str; else p="";
				MessageBox(NULL, nes_tostr(N, cobj), p, MB_OK);
			}
		} else if (stricmp(t, "script")==0) {
			cobj=nes_getobj(N, tobj, "command");
			if (nes_isstr(cobj)&&cobj->val->size>0) {
				nes_exec(N, nes_tostr(N, cobj));
				if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
			}
		} else if (stricmp(t, "CreateProcess")==0) {
			cobj=nes_getobj(N, tobj, "command");
			if (nes_isstr(cobj)&&cobj->val->size>0) {
				winsystem(SW_SHOW, nes_tostr(N, cobj));
			}
		} else if (stricmp(t, "ShellExecute")==0) {
			cobj=nes_getobj(N, tobj, "command");
			if (nes_isstr(cobj)&&cobj->val->size>0) {
				ShellExecute(NULL, "open", nes_tostr(N, cobj), NULL, NULL, SW_SHOWMAXIMIZED);
			}
		}
		IconStatus(0);
		lock--;
		break;
	}
	case IDM_STATE: {
		obj_t *cobj;
		POINT point;

		switch (lParam) {
		case WM_LBUTTONDBLCLK:
			while (lock>0) Sleep(1);
			lock++;
			cobj=nes_getobj(N, nes_getobj(N, &N->g, "NesTray"), "onclick");
			IconStatus(1);
			if (nes_typeof(cobj)==NT_NFUNC) {
				nes_exec(N, "NesTray.onclick();");
				if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
			}
			IconStatus(0);
			lock--;
			break;
		case WM_LBUTTONDOWN:
			break;
		case WM_RBUTTONDOWN:
			GetCursorPos(&point);
			while (lock>0) Sleep(1);
			lock++;
			PopupMenuDraw(point);
			lock--;
			break;
		default:
			break;
		}
		break;
	}
	case WM_CLOSE:
	case WM_QUIT:
	case WM_DESTROY:
		IconNotify(NIM_DELETE);
		EndDialog(wnd, TRUE);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK DlgTextInput(HWND wnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG: {
		obj_t *cobj1=nes_getobj(N, &N->l, "1");
		obj_t *cobj2=nes_getobj(N, &N->l, "2");

		SetWindowText(wnd, nes_tostr(N, cobj2));
		SetDlgItemText(wnd, IDC_EDIT1, nes_tostr(N, cobj1));
		break;
	}
	case WM_COMMAND: {
		char buffer[512];

		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemText(wnd, IDC_EDIT1, buffer, sizeof(buffer)-1);
			EndDialog(wnd, 0);
			nes_setstr(N, &N->r, "", buffer, -1);
			return TRUE;
		case IDCANCEL:
			EndDialog(wnd, 0);
			nes_setnum(N, &N->r, "", -1);
			return TRUE;
		}
		break;
	}
	case WM_CLOSE:
	case WM_QUIT:
	case WM_DESTROY:
		EndDialog(wnd, 0);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK DlgTrayNotice(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_PAINT: {
		HDC dc;
		HFONT font;
		PAINTSTRUCT ps;
		RECT rect;

		GetClientRect(hwnd, &rect);
		rect.top+=5; rect.left+=5;
		rect.bottom-=5; rect.right-=5;
		dc=BeginPaint(hwnd, &ps);
		font=CreateFont(14,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_DONTCARE,"arial");
		SelectObject(dc, font);
		SetBkMode(dc, TRANSPARENT);
		SetTextColor(dc,0xFF0000);
		DrawText(dc, G.noticetext, -1, &rect, DT_TOP|DT_LEFT|DT_WORDBREAK|DT_EXPANDTABS);
		DeleteObject(font);
		EndPaint(hwnd, &ps);
		break;
	}
	case WM_TIMER:
		KillTimer(hwnd, IDT_TIMER2);
		DestroyWindow(hwnd);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return TRUE;
}

NES_FUNCTION(nes_textinput)
{
	IconStatus(1);
	DialogBox(G.instance, MAKEINTRESOURCE(IDD_TEXTINPUT1), NULL, DlgTextInput);
	IconStatus(0);
	return 0;
}

NES_FUNCTION(nes_passinput)
{
	IconStatus(1);
	DialogBox(G.instance, MAKEINTRESOURCE(IDD_TEXTINPUT2), NULL, DlgTextInput);
	IconStatus(0);
	return 0;
}

NES_FUNCTION(nes_traynotice)
{
	obj_t *cobj1, *cobj2, *cobj3;
	HWND hwnd;
	WNDCLASSEX WndClsEx;
	static int init=0;
	int x, y;
	int w, h;
	int tbh;
	short int t;

	IconStatus(1);
	cobj1=nes_getobj(N, &N->l, "1");
	cobj2=nes_getobj(N, &N->l, "2");
	cobj3=nes_getobj(N, &N->l, "3");
	t=nes_isnum(cobj3)?(int)nes_tonum(N, cobj3):5;
	if (t<1) {
		IconStatus(0);
		return 0;
	}
	x=GetSystemMetrics(SM_CXSCREEN);
	y=GetSystemMetrics(SM_CYSCREEN);
	w=220; h=110;
	/* how do we get the height of the taskbar? */
	tbh=30;
	if (!init) {
		memset(&WndClsEx, 0, sizeof(WndClsEx));
		WndClsEx.cbSize        = sizeof(WNDCLASSEX);
		WndClsEx.style         = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
		WndClsEx.lpfnWndProc   = DlgTrayNotice;
		WndClsEx.hInstance     = G.instance;
		WndClsEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
		WndClsEx.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
		WndClsEx.lpszClassName = "TrayNotice";
		if (!RegisterClassEx(&WndClsEx)) return 0;
		init=1;
	}
	hwnd=CreateWindowEx(
		WS_EX_PALETTEWINDOW,
		"TrayNotice",
		"Tray Notification",
		WS_POPUPWINDOW,
		x-w,
		y-h-tbh,
		w,
		h,
		NULL,
		NULL,
		G.instance,
		NULL
	);
//	SetWindowPos(hwnd, HWND_TOPMOST, x-w, y-h-tbh, w, h, SWP_SHOWWINDOW);
//	SetWindowPos(hwnd, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	_snprintf(G.noticetext, sizeof(G.noticetext)-1, "%s", nes_tostr(N, cobj1));
	SetWindowText(hwnd, nes_tostr(N, cobj2));
	SetTimer(hwnd, IDT_TIMER2, t*1000, NULL);
	ShowWindow(hwnd, SW_SHOW);
	IconStatus(0);
	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HANDLE mutex;
	obj_t *cobj;

	memset(&G, 0, sizeof(G));
	G.instance=hInstance;
	if ((N=nes_newstate())==NULL) return -1;
	init_stuff(N);
	cobj=nes_getobj(N, &N->g, "MUTEXNAME");
	if (nes_isstr(cobj)&&cobj->val->size>0) {
		mutex=CreateMutex(NULL, FALSE, nes_tostr(N, cobj));
	} else {
		mutex=CreateMutex(NULL, FALSE, "NESTRAY_MUTEX");
	}
	if ((mutex==NULL)||(GetLastError()==ERROR_ALREADY_EXISTS)) {
		if (mutex) CloseHandle(mutex);
		nes_endstate(N);
		return 0;
	}
	cobj=nes_getobj(N, nes_getobj(N, &N->g, "NesTray"), "onload");
	if (nes_typeof(cobj)==NT_NFUNC) {
		nes_exec(N, "NesTray.onload();");
		if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
	}
	DialogBox(G.instance, MAKEINTRESOURCE(IDD_NULLDIALOG), NULL, PopupMenuExec);
	cobj=nes_getobj(N, nes_getobj(N, &N->g, "NesTray"), "onexit");
	if (nes_typeof(cobj)==NT_NFUNC) {
		nes_exec(N, "NesTray.onexit();");
		if (N->err) MessageBox(NULL, N->errbuf, "Script Error", MB_ICONSTOP);
	}
	CloseHandle(mutex);
	nes_endstate(N);
	return 0;
}
#endif
