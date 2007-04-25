/*
    NullLogic Groupware - Copyright (C) 2007 Dan Cahill

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
#define SRVMOD_MAIN 1
#include "nullgw/ghttpd/mod.h"
#include "nesla/nesla.h"

#ifdef WIN32
#define pthread_self() GetCurrentThreadId()
#endif

static CONN *get_sid()
{
	int sid;

	if (http_proc->conn==NULL) return NULL;
	for (sid=0;sid<http_proc->config.http_maxconn;sid++) {
		if (http_proc->conn[sid].id==pthread_self()) break;
	}
	if ((sid<0)||(sid>=http_proc->config.http_maxconn)) return NULL;
	return &http_proc->conn[sid];
}

static int mod_nes_write(nes_state *N)
{
	CONN *sid=get_sid();
	int len;

	N->outbuf[N->outbuflen]='\0';
	len=prints(sid, "%s", N->outbuf);
	N->outbuflen=0;
	return len;
}

static int mod_nes_system(nes_state *N)
{
	CONN *sid=get_sid();
	char tempname[255];
	char line[512];
	short int err;
	FILE *fp;
	obj_t *cobj;

	if ((cobj=nes_getobj(N, &N->l, "!1"))==NULL) {
		prints(sid, "no arg1\r\n");
		return 0;
	}
	if (cobj->val->type!=NT_STRING) {
		prints(sid, "!1 is not a string\r\n");
		return 0;
	}
	snprintf(tempname, sizeof(tempname)-1, "%s/exec-%d.tmp", config->dir_var_tmp, (int)(time(NULL)%999));
	fixslashes(tempname);
#ifdef WIN32
	snprintf(line, sizeof(line)-1, "\"%s\" > \"%s\"", cobj->val->d.str, tempname);
#else
	snprintf(line, sizeof(line)-1, "%s > %s 2>&1", cobj->val->d.str, tempname);
#endif
	flushbuffer(sid);
	err=sys_system(line);
	if ((fp=fopen(tempname, "r"))!=NULL) {
		while (fgets(line, sizeof(line)-1, fp)!=NULL) {
			prints(sid, "%s", line);
		}
		fclose(fp);
	}
	unlink(tempname);
	return 0;
}

static void mod_nes_register_variables(nes_state *N)
{
	CONN *sid=get_sid();
	obj_t *cobj;
	char *ptemp;
	unsigned int i;

	if (sid==NULL) return;
	cobj=nes_settable(N, &N->g, "_SERVER");
	nes_setnum(N, cobj, "CONTENT_LENGTH",    sid->dat->in_ContentLength);
	if (!strlen(sid->dat->in_ContentType)) {
		nes_setstr(N, cobj, "CONTENT_TYPE", "application/x-www-form-urlencoded", strlen("application/x-www-form-urlencoded"));
	} else {
		nes_setstr(N, cobj, "CONTENT_TYPE", sid->dat->in_ContentType, strlen(sid->dat->in_ContentType));
	}
	nes_setstr(N, cobj, "GATEWAY_INTERFACE", "CGI/1.1", strlen("CGI/1.1"));
	nes_setstr(N, cobj, "HTTP_COOKIE",       sid->dat->in_Cookie, strlen(sid->dat->in_Cookie));
	nes_setstr(N, cobj, "HTTP_USER_AGENT",   sid->dat->in_UserAgent, strlen(sid->dat->in_UserAgent));
	nes_setstr(N, cobj, "PATH_TRANSLATED",   "?", strlen("?"));
	nes_setstr(N, cobj, "NESLA_SELF",        sid->dat->in_CGIScriptName, strlen(sid->dat->in_CGIScriptName));
	nes_setstr(N, cobj, "QUERY_STRING",      sid->dat->in_QueryString, strlen(sid->dat->in_QueryString));
	nes_setstr(N, cobj, "REMOTE_ADDR",       sid->dat->in_RemoteAddr, strlen(sid->dat->in_RemoteAddr));
	nes_setnum(N, cobj, "REMOTE_PORT",       sid->dat->in_RemotePort);
	nes_setstr(N, cobj, "REMOTE_USER",       sid->dat->user_username, strlen(sid->dat->user_username));
	nes_setstr(N, cobj, "REQUEST_METHOD",    sid->dat->in_RequestMethod, strlen(sid->dat->in_RequestMethod));
	nes_setstr(N, cobj, "REQUEST_URI",       sid->dat->in_RequestURI, strlen(sid->dat->in_RequestURI));
	nes_setstr(N, cobj, "SCRIPT_NAME",       sid->dat->in_CGIScriptName, strlen(sid->dat->in_CGIScriptName));
	nes_setstr(N, cobj, "SERVER_PROTOCOL",   "HTTP/1.1", strlen("HTTP/1.1"));
	nes_setstr(N, cobj, "SERVER_SOFTWARE",   PACKAGE_NAME, strlen(PACKAGE_NAME));
	nes_settable(N, &N->g, "_GET");
	nes_settable(N, &N->g, "_POST");
	if (strlen(sid->dat->in_QueryString)>=0) {
		ptemp=sid->dat->in_QueryString;
		if ((cobj=nes_getobj(N, &N->g, "_GET"))==NULL) {
			/* good place for an error */
			return;
		}
		if (cobj->val->type!=NT_TABLE) {
			/* good place for an error */
			return;
		}
		while (*ptemp!='\0') {
			for (i=0;;i++) {
				if (*ptemp=='\0') { sid->dat->smallbuf[0][i]='\0'; break; }
				if (*ptemp=='=') { sid->dat->smallbuf[0][i]='\0'; ptemp++; break; }
				sid->dat->smallbuf[0][i]=*ptemp;
				ptemp++;
			}
			for (i=0;;i++) {
				if (*ptemp=='\0') { sid->dat->smallbuf[1][i]='\0'; break; }
				if (*ptemp=='&') { sid->dat->smallbuf[1][i]='\0'; ptemp++; break; }
				sid->dat->smallbuf[1][i]=*ptemp;
				ptemp++;
			}
			decodeurl(sid->dat->smallbuf[1]);
			nes_setstr(N, cobj, sid->dat->smallbuf[0], sid->dat->smallbuf[1], strlen(sid->dat->smallbuf[1]));
		}
	}
	if (strcmp(sid->dat->in_RequestMethod, "POST")==0) {
		nes_setstr(N, &N->g, "RAWPOSTDATA", sid->PostData, strlen(sid->PostData));
		ptemp=sid->PostData;
		if ((cobj=nes_getobj(N, &N->g, "_POST"))==NULL) {
			/* good place for an error */
			return;
		}
		if (cobj->val->type!=NT_TABLE) {
			/* good place for an error */
			return;
		}
		while (*ptemp!='\0') {
			for (i=0;;i++) {
				if (*ptemp=='\0') { sid->dat->smallbuf[0][i]='\0'; break; }
				if (*ptemp=='=') { sid->dat->smallbuf[0][i]='\0'; ptemp++; break; }
				sid->dat->smallbuf[0][i]=*ptemp;
				ptemp++;
			}
			for (i=0;;i++) {
				if (*ptemp=='\0') { sid->dat->smallbuf[1][i]='\0'; break; }
				if (*ptemp=='&') { sid->dat->smallbuf[1][i]='\0'; ptemp++; break; }
				sid->dat->smallbuf[1][i]=*ptemp;
				ptemp++;
			}
			decodeurl(sid->dat->smallbuf[1]);
			nes_setstr(N, cobj, sid->dat->smallbuf[0], sid->dat->smallbuf[1], strlen(sid->dat->smallbuf[1]));
		}
	}
	return;
}

static void preppath(nes_state *N, char *name)
{
	char buf[512];
	char *p;
	unsigned int j;

	p=name;
	if ((name[0]=='/')||(name[0]=='\\')||(name[1]==':')) {
		/* it's an absolute path.... probably... */
		strncpy(buf, name, sizeof(buf)-1);
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

DllExport int mod_main(CONN *sid)
{
	nes_state *N;
	struct stat sb;
	char filename[512];
	char *ptemp;
	char *ext;
	obj_t *cobj;

	if (strncmp(sid->dat->in_CGIScriptName, "/nesla/", 5)!=0) { return -1; }
	ptemp=sid->dat->in_CGIScriptName;
	memset(filename, 0, sizeof(filename));
	snprintf(filename, sizeof(filename)-1, "%s/%04d/htdocs%s", config->dir_var_domains, sid->dat->user_did, ptemp);
	fixslashes(filename);
#ifdef WIN32 /* this probably needs a much better fix than this */
	if (filename[strlen(filename)-1]=='\\') filename[strlen(filename)-1]='\0';
#endif
	/* if it's not in the private htdocs, try the public */
	if (stat(filename, &sb)!=0) {
		memset(filename, 0, sizeof(filename));
		snprintf(filename, sizeof(filename)-1, "%s/share/htdocs%s", config->dir_var, ptemp);
		fixslashes(filename);
#ifdef WIN32 /* this probably needs a much better fix than this */
		if (filename[strlen(filename)-1]=='\\') filename[strlen(filename)-1]='\0';
#endif
		if (stat(filename, &sb)!=0) { return -1; }
	}
	if (sb.st_mode&S_IFDIR) {
		htpage_dirlist(sid);
		return 0;
	}
	ext=strrchr(sid->dat->in_CGIScriptName, '.');
	if (ext==NULL) {
		filesend(sid, filename);
		return 0;
	}
	if ((strcmp(ext, ".nes")!=0)&&(strcmp(ext, ".n")!=0)) {
		filesend(sid, filename);
		return 0;
	}
	send_header(sid, 0, 200, "1", "text/html", -1, -1);
	N=nes_newstate();
	nes_setcfunc(N, &N->g, "system", (NES_CFUNC)mod_nes_system);
	cobj=nes_getobj(N, &N->g, "io");
	nes_setcfunc(N, cobj, "write",   (NES_CFUNC)mod_nes_write);
	mod_nes_register_variables(N);
	preppath(N, filename);
	if (setjmp(N->savjmp)==0) {
		nes_execfile(N, filename);
		mod_nes_write(N);
	}
	if (N->err) {
		prints(sid, "<HR><B>[errno=%d :: %s]</B>\r\n", N->err, N->errbuf);
	}
	nes_endstate(N);
	return 0;
}

DllExport int mod_exit()
{
	return 0;
}

DllExport int mod_init(_PROC *_proc, HTTP_PROC *_http_proc, FUNCTION *_functions)
{
	MODULE_MENU newmod = {
		"mod_nesla",		/* mod_name     */
		0,			/* mod_submenu  */
		"",			/* mod_menuname */
		"",			/* mod_menupic  */
		"",			/* mod_menuuri  */
		"",			/* mod_menuperm */
		"mod_main",		/* fn_name      */
		"/nesla/",		/* fn_uri       */
		mod_init,		/* fn_init      */
		mod_main,		/* fn_main      */
		mod_exit		/* fn_exit      */
	};

	proc=_proc;
	http_proc=_http_proc;
	config=&proc->config;
	functions=_functions;
	if (mod_import()!=0) return -1;
	if (mod_export_main(&newmod)!=0) return -1;
	return 0;
}
