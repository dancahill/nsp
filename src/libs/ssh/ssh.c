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
#include "nesla/libnesla.h"

#ifdef HAVE_SSH2

//#define PPMETER /* progress meter for downloads */

#include "nesla/libssh.h"

#if defined(_MSC_VER) || defined(__BORLANDC__)
#include <io.h>
#elif !defined( __TURBOC__)
#include <unistd.h>
#endif

#ifdef WIN32
#define snprintf _snprintf
#define sleep(x) Sleep(x*1000)
#define msleep(x) Sleep(x)
#define strcasecmp stricmp
typedef int socklen_t;
#else
#include <unistd.h>
#include <utime.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define closesocket close
#define msleep(x) usleep(x*1000)
#ifdef MISSING_SOCKLEN
typedef int socklen_t;
#endif
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAXWAIT 10

#ifndef O_BINARY
#define O_BINARY 0
#endif

int ssh_close(nes_state *N, SSH_CONN *sshconn, short int owner_killed);

/*
 * this is the function that terminates orphans
 */
void ssh_murder(nes_state *N, obj_t *cobj)
{
#define __FUNCTION__ __FILE__ ":ssh_murder()"
	SSH_CONN *sshconn;

	n_warn(N, __FUNCTION__, "reaper is claiming another lost soul");
	if ((cobj->val->type!=NT_CDATA)||(cobj->val->d.str==NULL)||(strcmp(cobj->val->d.str, "ssh-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sshconn");
	sshconn=(SSH_CONN *)cobj->val->d.str;
	if (sshconn!=NULL) {	
		ssh_close(N, sshconn, 1);
		n_free(N, (void *)&cobj->val->d.str);
	}
	return;
#undef __FUNCTION__
}

static void ssh_lasterr(nes_state *N, SSH_CONN *sshconn, char *msg)
{
	char *err=NULL;

	if (sshconn->session!=NULL) {
		libssh2_session_last_error(sshconn->session, &err, NULL, 0);
		nes_setstr(N, nes_settable(N, &N->g, "ssh"), "last_err", err, -1);
		if (err!=NULL) free(err);
	}
	return;
}

static int setfiletime(nes_state *N, char *fname, time_t ftime)
{
#ifdef WIN32
	static int isWinNT=-1;
	SYSTEMTIME st;
	FILETIME locft, modft;
	struct tm *loctm;
	HANDLE hFile;
	int result;

	loctm=localtime(&ftime);
	if (loctm==NULL) return -1;
	st.wYear         = (WORD)loctm->tm_year+1900;
	st.wMonth        = (WORD)loctm->tm_mon+1;
	st.wDayOfWeek    = (WORD)loctm->tm_wday;
	st.wDay          = (WORD)loctm->tm_mday;
	st.wHour         = (WORD)loctm->tm_hour;
	st.wMinute       = (WORD)loctm->tm_min;
	st.wSecond       = (WORD)loctm->tm_sec;
	st.wMilliseconds = 0;
	if (!SystemTimeToFileTime(&st, &locft)||!LocalFileTimeToFileTime(&locft, &modft)) return -1;
	if (isWinNT<0) isWinNT=(GetVersion()<0x80000000)?1:0;
	hFile=CreateFile(fname, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, (isWinNT ? FILE_FLAG_BACKUP_SEMANTICS : 0), NULL);
	if (hFile==INVALID_HANDLE_VALUE) return -1;
	result=SetFileTime(hFile, &modft, &modft, &modft)?0:-1;
	CloseHandle(hFile);
	/*
	 * mtime on fat filesystems is only stored with a 2 second precision.
	 * thanks, microsoft.  your api didn't already have enough pitfalls.
	 */
	return result;
#else
	struct utimbuf settime;

	settime.actime=settime.modtime=ftime;
	return utime(fname, &settime);
#endif
}

static int ssh_conn(nes_state *N, SSH_CONN *sshconn, const struct sockaddr_in *serv_addr, socklen_t addrlen)
{
	struct sockaddr_in host;
	struct sockaddr_in peer;
	socklen_t fromlen;
	int rc;

	rc=connect(sshconn->socket, (struct sockaddr *)serv_addr, addrlen);
	if (rc<0) {
		sshconn->LocalPort=0;
		sshconn->RemotePort=0;
		return -1;
	}
	fromlen=sizeof(host);
	getsockname(sshconn->socket, (struct sockaddr *)&host, &fromlen);
	nc_strncpy(sshconn->LocalAddr, inet_ntoa(host.sin_addr), sizeof(sshconn->LocalAddr)-1);
	sshconn->LocalPort=ntohs(host.sin_port);
	fromlen=sizeof(peer);
	getpeername(sshconn->socket, (struct sockaddr *)&peer, &fromlen);
	nc_strncpy(sshconn->RemoteAddr, inet_ntoa(peer.sin_addr), sizeof(sshconn->RemoteAddr)-1);
	sshconn->RemotePort=ntohs(peer.sin_port);
	return rc;
}

int ssh_connect(nes_state *N, SSH_CONN *sshconn, char *host, unsigned short port)
{
#define __FUNCTION__ __FILE__ ":ssh_connect()"
	int rc;
	struct hostent *hp;
	struct sockaddr_in serv;
	const char *fingerprint;

	if ((hp=gethostbyname(host))==NULL) {
		/* n_warn(N, __FUNCTION__, "Host lookup error for %s", host); */
		return -1;
	}
	nc_memset((char *)&serv, 0, sizeof(serv));
	nc_memcpy((char *)&serv.sin_addr, hp->h_addr, hp->h_length);
	serv.sin_family=hp->h_addrtype;
	serv.sin_port=htons(port);
	if ((sshconn->socket=socket(AF_INET, SOCK_STREAM, 0))<0) return -2;
	if (ssh_conn(N, sshconn, &serv, sizeof(serv))<0) {
		/* n_warn(N, __FUNCTION__, "Error connecting to %s:%d", host, port); */
		return -2;
	}
	sshconn->session=libssh2_session_init();
	if (!sshconn->session) return -1;
	rc=libssh2_session_startup(sshconn->session, sshconn->socket);
	if (rc) {
		/* n_warn(N, __FUNCTION__, "Failure establishing SSH session: %d", rc); */
		return -3;
	}
	fingerprint=libssh2_hostkey_hash(sshconn->session, LIBSSH2_HOSTKEY_HASH_MD5);
	memcpy(sshconn->HostKey, fingerprint, 16);
	return 0;
#undef __FUNCTION__
}

int ssh_close(nes_state *N, SSH_CONN *sshconn, short int owner_killed)
{
	if (sshconn==NULL) return 0;
	if (!owner_killed) {
//		sshconn->want_close=1;
	} else {
//		sshconn->want_close=0;
		if (sshconn->session) {
			libssh2_session_disconnect(sshconn->session, "Normal Shutdown, Thank you for playing");
			libssh2_session_free(sshconn->session);
			sshconn->session=NULL;
		}

		if (sshconn->socket>-1) {
			/* shutdown(x,0=recv, 1=send, 2=both) */
			shutdown(sshconn->socket, 2);
			closesocket(sshconn->socket);
			sshconn->socket=-1;
		}
	}
	return 0;
}

/*
 * start nesla ssh script functions here
 */
NES_FUNCTION(neslassh_ssh_open)
{
#define __FUNCTION__ __FILE__ ":neslassh_ssh_open()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* host */
	obj_t *cobj2=nes_getobj(N, &N->l, "2"); /* port */
	unsigned short port=22;
	SSH_CONN *sshconn;
	int rc;

	if ((cobj1->val->type!=NT_STRING)||(cobj1->val->d.str==NULL))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg1");
	if (cobj2->val->type==NT_NUMBER) port=(unsigned short)cobj2->val->d.num;
	sshconn=calloc(1, sizeof(SSH_CONN)+1);
	if (sshconn==NULL) {
		n_warn(N, __FUNCTION__, "couldn't alloc %d bytes", sizeof(SSH_CONN)+1);
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
	nc_strncpy(sshconn->obj_type, "ssh-conn", sizeof(sshconn->obj_type)-1);
	sshconn->obj_term=(NES_CFREE)ssh_murder;
	if ((rc=ssh_connect(N, sshconn, cobj1->val->d.str, port))<0) {
		nes_setnum(N, &N->r, "", -1);
		n_free(N, (void *)&sshconn);
		return -1;
	}
	/* nes_setcdata(N, &N->r, "", sshconn, sizeof(SSH_CONN)+1); */
	nes_setcdata(N, &N->r, "", NULL, 0);
	N->r.val->d.str=(void *)sshconn;
	N->r.val->size=sizeof(SSH_CONN)+1;
	return 0;
#undef __FUNCTION__
}

NES_FUNCTION(neslassh_ssh_close)
{
#define __FUNCTION__ __FILE__ ":neslassh_ssh_close()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* sshconn */
	SSH_CONN *sshconn;

	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "ssh-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sshconn for arg1");
	sshconn=(SSH_CONN *)cobj1->val->d.str;
	if (sshconn->session==NULL) n_warn(N, __FUNCTION__, "this session is dead");
	if (sshconn->sh_channel!=NULL) {
		libssh2_channel_close(sshconn->sh_channel);
		libssh2_channel_free(sshconn->sh_channel);
		sshconn->sh_channel=NULL;
	}
	ssh_close(N, sshconn, 1);
	n_free(N, (void *)&cobj1->val->d.str);
	nes_setnum(N, &N->r, "", 0);
	return 0;
#undef __FUNCTION__
}

NES_FUNCTION(neslassh_ssh_hostkey)
{
#define __FUNCTION__ __FILE__ ":neslassh_ssh_hostkey()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* sshconn */
	SSH_CONN *sshconn;
	char fpbuf[50];
	uchar *p;

	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "ssh-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sshconn for arg1");
	sshconn=(SSH_CONN *)cobj1->val->d.str;
	if (sshconn->session==NULL) {
		n_warn(N, __FUNCTION__, "this session is dead");
		return -1;
	}
	p=(uchar *)sshconn->HostKey;
	memset(fpbuf, 0, sizeof(fpbuf));
	snprintf(fpbuf, sizeof(fpbuf)-1, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]
	);
	nes_setstr(N, &N->r, "", fpbuf, -1);
	return 0;
#undef __FUNCTION__
}

NES_FUNCTION(neslassh_ssh_auth)
{
#define __FUNCTION__ __FILE__ ":neslassh_ssh_auth()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* sshconn */
	obj_t *cobj2=nes_getobj(N, &N->l, "2"); /* user */
	obj_t *cobj3=nes_getobj(N, &N->l, "3"); /* pass or keyset */
	obj_t *cobj;
	SSH_CONN *sshconn;
	char rsapub[256];
	char rsaprv[256];
	char rsapass[64];
	char *userauthlist;
	int rc;

	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "ssh-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sshconn for arg1");
	if ((cobj2->val->type!=NT_STRING)||(cobj2->val->d.str==NULL))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");

	sshconn=(SSH_CONN *)cobj1->val->d.str;
	if (sshconn->session==NULL) {
		n_warn(N, __FUNCTION__, "this session is dead");
		return -1;
	}
	userauthlist=libssh2_userauth_list(sshconn->session, cobj2->val->d.str, cobj2->val->size);
	if (N->debug) n_warn(N, __FUNCTION__, "Available authentication methods: %s", userauthlist);
	/* should userauthlist be free()d? */
	if (cobj3->val->type==NT_TABLE) {
		if (strstr(userauthlist, "publickey")==NULL) {
			n_warn(N, __FUNCTION__, "Authentication method not available");
			nes_setnum(N, &N->r, "", 0);
			N->r.val->type=NT_BOOLEAN;
			return -1;
		}
		cobj=nes_getobj(N, cobj3, "pub");
		if ((cobj->val->type!=NT_STRING)||(cobj->val->d.str==NULL)) {
			n_error(N, NE_SYNTAX, __FUNCTION__, "missing public key filename");
		}
		snprintf(rsapub, sizeof(rsapub)-1, "%s", nes_tostr(N, cobj));
		cobj=nes_getobj(N, cobj3, "prv");
		if ((cobj->val->type!=NT_STRING)||(cobj->val->d.str==NULL)) {
			n_error(N, NE_SYNTAX, __FUNCTION__, "missing private key filename");
		}
		snprintf(rsaprv, sizeof(rsaprv)-1, "%s", nes_tostr(N, cobj));
		cobj=nes_getobj(N, cobj3, "pass");
		if ((cobj->val->type!=NT_STRING)||(cobj->val->d.str==NULL)) {
			n_error(N, NE_SYNTAX, __FUNCTION__, "missing private key password");
		}
		snprintf(rsapass, sizeof(rsapass)-1, "%s", nes_tostr(N, cobj));
		/* auth via public key */
		rc=libssh2_userauth_publickey_fromfile(sshconn->session, cobj2->val->d.str, rsapub, rsaprv, rsapass);
		if (rc) {
			ssh_lasterr(N, sshconn, NULL);
			n_warn(N, __FUNCTION__, "Authentication by public key failed");
			ssh_close(N, sshconn, 1);
			nes_setnum(N, &N->r, "", 0);
			N->r.val->type=NT_BOOLEAN;
			return -1;
		}
	} else if ((cobj3->val->type==NT_STRING)&&(cobj3->val->d.str!=NULL)) {
		if (strstr(userauthlist, "password")==NULL) {
			n_warn(N, __FUNCTION__, "Authentication method not available");
			nes_setnum(N, &N->r, "", 0);
			N->r.val->type=NT_BOOLEAN;
			return -1;
		}
		/* auth via password */
		rc=libssh2_userauth_password(sshconn->session, cobj2->val->d.str, cobj3->val->d.str);
		if (rc) {
			n_warn(N, __FUNCTION__, "Authentication by password failed");
			ssh_close(N, sshconn, 1);
			nes_setnum(N, &N->r, "", 0);
			N->r.val->type=NT_BOOLEAN;
			return -1;
		}
	} else {
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string or table for arg3");
	}
	nes_setnum(N, &N->r, "", 1);
	N->r.val->type=NT_BOOLEAN;
	return 0;
#undef __FUNCTION__
}

NES_FUNCTION(neslassh_ssh_cmd)
{
#define __FUNCTION__ __FILE__ ":neslassh_ssh_cmd()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* sshconn */
	obj_t *cobj2=nes_getobj(N, &N->l, "2"); /* command */
	obj_t *cobj3=nes_getobj(N, &N->l, "3"); /* max milliseconds*10 to wait */
	obj_t *cobj=&N->r;
	SSH_CONN *sshconn;
	int rc=0;
	char buf[512];
	unsigned short retries;
	unsigned short maxtries=20;

	nes_unlinkval(N, cobj);
	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "ssh-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sshconn for arg1");
	if ((cobj2->val->type!=NT_STRING)||(cobj2->val->d.str==NULL))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");
	if (cobj3->val->type==NT_NUMBER) maxtries=(unsigned short)cobj3->val->d.num;
	sshconn=(SSH_CONN *)cobj1->val->d.str;
	if (sshconn->session==NULL) {
		n_warn(N, __FUNCTION__, "this session is dead");
		return -1;
	}
	cobj=nes_setstr(N, cobj, "", NULL, 0);
	nes_strcat(N, cobj2, "\n", 1);
	if (sshconn->sh_channel==NULL) {
		if (!(sshconn->sh_channel=libssh2_channel_open_session(sshconn->session))) {
			n_warn(N, __FUNCTION__, "Unable to open a session");
			ssh_close(N, sshconn, 1);
			return -1;
		}
		/* libssh2_channel_setenv(channel, "FOO", "bar"); */
		if (libssh2_channel_request_pty(sshconn->sh_channel, "vanilla")) {
			n_warn(N, __FUNCTION__, "Failed requesting pty");
			libssh2_channel_free(sshconn->sh_channel);
			sshconn->sh_channel=NULL;
			ssh_close(N, sshconn, 1);
			return -1;
		}
		if (libssh2_channel_shell(sshconn->sh_channel)) {
			n_warn(N, __FUNCTION__, "Unable to request shell on allocated pty");
			libssh2_channel_free(sshconn->sh_channel);
			sshconn->sh_channel=NULL;
			ssh_close(N, sshconn, 1);
			return -1;
		}
		libssh2_session_set_blocking(sshconn->session, 0);
		/* eat the preamble */
		retries=0;
		do {
			memset(buf, 0, sizeof(buf));
			rc=libssh2_channel_read(sshconn->sh_channel, buf, sizeof(buf)-1);
			if (rc==LIBSSH2_ERROR_EAGAIN&&retries<maxtries) {
				msleep(MAXWAIT);
				retries++;
				continue;
			}
			if (rc<0) break;
			retries=0;
		} while (1);
		libssh2_session_set_blocking(sshconn->session, 1);
	}
	/* At this point the shell can be interacted with using
	 * libssh2_channel_read()
	 * libssh2_channel_read_stderr()
	 * libssh2_channel_write()
	 * libssh2_channel_write_stderr()
	 *
	 * Blocking mode may be (en|dis)abled with: libssh2_channel_set_blocking()
	 * If the server send EOF, libssh2_channel_eof() will return non-0
	 * To send EOF to the server use: libssh2_channel_send_eof()
	 * A channel can be closed with: libssh2_channel_close()
	 * A channel can be freed with: libssh2_channel_free()
	 */
	libssh2_session_set_blocking(sshconn->session, 0);
	libssh2_channel_write(sshconn->sh_channel, cobj2->val->d.str, cobj2->val->size);
	retries=0;
	do {
		memset(buf, 0, sizeof(buf));
		rc=libssh2_channel_read(sshconn->sh_channel, buf, sizeof(buf)-1);
		if (rc==LIBSSH2_ERROR_EAGAIN&&retries<maxtries) {
			msleep(MAXWAIT);
			retries++;
			continue;
		}
		if (rc<0) break;
		retries=0;
		nes_strcat(N, cobj, buf, rc);
	} while (1);
	libssh2_session_set_blocking(sshconn->session, 1);
	return rc;
#undef __FUNCTION__
}

NES_FUNCTION(neslassh_sftp_get)
{
#define __FUNCTION__ __FILE__ ":neslassh_sftp_get()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* sshconn */
	obj_t *cobj2=nes_getobj(N, &N->l, "2"); /* remote file */
	obj_t *cobj3=nes_getobj(N, &N->l, "3"); /* local file */
	SSH_CONN *sshconn;
	LIBSSH2_CHANNEL *channel;
	struct stat fileinfo;
	off_t got=0;
	char *p;
	int rc=0;

	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "ssh-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sshconn for arg1");
	if ((cobj2->val->type!=NT_STRING)||(cobj2->val->d.str==NULL))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");
	sshconn=(SSH_CONN *)cobj1->val->d.str;
	if (sshconn->session==NULL) {
		n_warn(N, __FUNCTION__, "this session is dead");
		return -1;
	}
	channel=libssh2_scp_recv(sshconn->session, cobj2->val->d.str, &fileinfo);
	if (!channel) {
		if (N->debug) n_warn(N, __FUNCTION__, "Unable to open a session");
		nes_setnum(N, &N->r, "", -1);
		return -1;
	}
	if ((cobj3->val->type==NT_STRING)&&(cobj3->val->d.str!=NULL)) {
		/* downloading to a file */
		char dlbuf[2048];
		int fd, max;
#ifdef PPMETER
		float pp=0, op=5;
#endif

		if ((fd=open(cobj3->val->d.str, O_WRONLY|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE))==-1) {
			n_warn(N, __FUNCTION__, "could not open file");
			libssh2_channel_free(channel);
			channel=NULL;
			ssh_close(N, sshconn, 1);
			return -1;
		}
		while (got<fileinfo.st_size) {
			max=fileinfo.st_size-got<sizeof(dlbuf)?fileinfo.st_size-got:sizeof(dlbuf);
			rc=libssh2_channel_read(channel, dlbuf, max);
			if (rc<1) {
				n_warn(N, __FUNCTION__, "libssh2_channel_read() failed: %d", rc);
				libssh2_channel_free(channel);
				channel=NULL;
				ssh_close(N, sshconn, 1);
				close(fd);
				return -1;
			}
#ifdef PPMETER
			if ((pp=(int)((float)got/(float)fileinfo.st_size*100))>op) printf("\r%d%%", (int)pp);
			op=pp;
#endif
			write(fd, dlbuf, rc);
			got+=rc;
		}
		close(fd);
		setfiletime(N, cobj3->val->d.str, fileinfo.st_mtime);
#ifdef PPMETER
		printf("\r%d%%\r", 100);
#endif
		nes_setnum(N, &N->r, "", fileinfo.st_size);
	} else {
		/* downloading to mem */
		obj_t *cobj=&N->r;

		nes_setstr(N, cobj, "", NULL, 0);
		if ((cobj->val->d.str=calloc(1, fileinfo.st_size+1))==NULL) {
			n_warn(N, __FUNCTION__, "OUT OF MEMORY");
			libssh2_channel_free(channel);
			channel=NULL;
			ssh_close(N, sshconn, 1);
			return -1;
		}
		while (got<fileinfo.st_size) {
			p=cobj->val->d.str+got;
			rc=libssh2_channel_read(channel, p, fileinfo.st_size-got);
			if (rc<1) {
				n_warn(N, __FUNCTION__, "libssh2_channel_read() failed: %d", rc);
				nes_unlinkval(N, cobj);
				break;
			}
			got+=rc;
			cobj->val->size=got;
		}
	}
	libssh2_channel_free(channel);
	channel=NULL;
	return rc;
#undef __FUNCTION__
}

NES_FUNCTION(neslassh_sftp_ls)
{
#define __FUNCTION__ __FILE__ ":neslassh_sftp_ls()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* sshconn */
	obj_t *cobj2=nes_getobj(N, &N->l, "2"); /* remote dir */
	obj_t *cobj=&N->r;
	obj_t tobj;
	obj_t *tobj2;
	SSH_CONN *sshconn;
	LIBSSH2_SFTP *sftp_session;
	LIBSSH2_SFTP_HANDLE *sftp_handle;
	int rc=0;
	int sym;
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	char rfname[512];
	char lstext[512];

	nes_unlinkval(N, cobj);
	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "ssh-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sshconn for arg1");
	if ((cobj2->val->type!=NT_STRING)||(cobj2->val->d.str==NULL))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");
	sshconn=(SSH_CONN *)cobj1->val->d.str;
	if (sshconn->session==NULL) {
		n_warn(N, __FUNCTION__, "this session is dead");
		return -1;
	}
	if (N->debug) n_warn(N, __FUNCTION__, "libssh2_sftp_init()");
	sftp_session=libssh2_sftp_init(sshconn->session);
	if (!sftp_session) {
		n_warn(N, __FUNCTION__, "Unable to init SFTP session");
		libssh2_sftp_shutdown(sftp_session);
		return -1;
	}
	libssh2_session_set_blocking(sshconn->session, 1);
	if (N->debug) n_warn(N, __FUNCTION__, "libssh2_sftp_opendir()");
	sftp_handle=libssh2_sftp_opendir(sftp_session, cobj2->val->d.str);
	if (!sftp_handle) {
		n_warn(N, __FUNCTION__, "Unable to open dir with SFTP");
		libssh2_sftp_closedir(sftp_handle);
		libssh2_sftp_shutdown(sftp_session);
		return -1;
	}
	if (N->debug) n_warn(N, __FUNCTION__, "libssh2_sftp_opendir() is done, now receive listing");
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nes_linkval(N, &tobj, NULL);
	tobj.val->type=NT_TABLE;
	tobj.val->attr|=NST_AUTOSORT;
	do {
		if ((rc=libssh2_sftp_readdir_ex(sftp_handle, rfname, sizeof(rfname), lstext, sizeof(lstext), &attrs))<1) break;
		tobj2=nes_settable(N, &tobj, rfname);
		sym=0;
		if (attrs.flags&LIBSSH2_SFTP_ATTR_SIZE) {
			nes_setnum(N, tobj2, "size", (unsigned long)attrs.filesize);
		}
		if (attrs.flags&LIBSSH2_SFTP_ATTR_UIDGID) {
			nes_setnum(N, tobj2, "uid", attrs.uid);
			nes_setnum(N, tobj2, "gid", attrs.gid);
		}
		if (attrs.flags&LIBSSH2_SFTP_ATTR_PERMISSIONS) {
			if (!(~attrs.permissions&LIBSSH2_SFTP_S_IFLNK)) sym=1;
			if (attrs.permissions&LIBSSH2_SFTP_S_IFDIR) {
				nes_setnum(N, tobj2, "size", 0);
				nes_setstr(N, tobj2, "type", sym?"dirp":"dir", sym?4:3);
			} else {
				nes_setstr(N, tobj2, "type", sym?"filep":"file", sym?5:4);
			}
		}
		if (attrs.flags&LIBSSH2_SFTP_ATTR_ACMODTIME) {
			nes_setnum(N, tobj2, "atime", attrs.atime);
			nes_setnum(N, tobj2, "mtime", attrs.mtime);
		}
	} while (1);
	libssh2_sftp_closedir(sftp_handle);
	libssh2_sftp_shutdown(sftp_session);
	nes_linkval(N, &N->r, &tobj);
	nes_unlinkval(N, &tobj);
	return rc;
#undef __FUNCTION__
}

NES_FUNCTION(neslassh_sftp_mkdir)
{
#define __FUNCTION__ __FILE__ ":neslassh_sftp_mkdir()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* sshconn */
	obj_t *cobj2=nes_getobj(N, &N->l, "2"); /* remote dir */
	obj_t *cobj=&N->r;
	SSH_CONN *sshconn;
	LIBSSH2_SFTP *sftp_session;
	int rc=0;

	nes_unlinkval(N, cobj);
	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "ssh-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sshconn for arg1");
	if ((cobj2->val->type!=NT_STRING)||(cobj2->val->d.str==NULL))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");
	sshconn=(SSH_CONN *)cobj1->val->d.str;
	if (sshconn->session==NULL) {
		n_warn(N, __FUNCTION__, "this session is dead");
		return -1;
	}
	nes_setstr(N, cobj, "", NULL, 0);
	if (N->debug) n_warn(N, __FUNCTION__, "libssh2_sftp_init()");
	sftp_session=libssh2_sftp_init(sshconn->session);
	if (!sftp_session) {
		n_warn(N, __FUNCTION__, "Unable to init SFTP session");
		return -1;
	}
	libssh2_session_set_blocking(sshconn->session, 1);
	if (N->debug) n_warn(N, __FUNCTION__, "libssh2_sftp_mkdir()");
	rc=libssh2_sftp_mkdir(sftp_session, cobj2->val->d.str,
		LIBSSH2_SFTP_S_IRWXU|
		LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IXGRP|
		LIBSSH2_SFTP_S_IROTH|LIBSSH2_SFTP_S_IXOTH);
	libssh2_sftp_shutdown(sftp_session);
	return rc;
#undef __FUNCTION__
}

NES_FUNCTION(neslassh_sftp_put)
{
#define __FUNCTION__ __FILE__ ":neslassh_sftp_put()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* sshconn */
	obj_t *cobj2=nes_getobj(N, &N->l, "2"); /* remote file */
	obj_t *cobj3=nes_getobj(N, &N->l, "3"); /* string */
	obj_t *cobj=&N->r;
	SSH_CONN *sshconn;
	LIBSSH2_CHANNEL *channel;
	int rc=0;
	struct stat fileinfo;
	unsigned long int nread;
	char *ptr;

	nes_unlinkval(N, cobj);
	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "ssh-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sshconn for arg1");
	if ((cobj2->val->type!=NT_STRING)||(cobj2->val->d.str==NULL))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");
	if ((cobj3->val->type!=NT_STRING)||(cobj3->val->d.str==NULL))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg3");
	sshconn=(SSH_CONN *)cobj1->val->d.str;
	if (sshconn->session==NULL) {
		n_warn(N, __FUNCTION__, "this session is dead");
		return -1;
	}
	nes_setstr(N, cobj, "", NULL, 0);
	fileinfo.st_mode=LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IWUSR|LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH;
	fileinfo.st_size=cobj3->val->size;
	channel=libssh2_scp_send(sshconn->session, cobj2->val->d.str, 0x1FF&fileinfo.st_mode, (unsigned long)fileinfo.st_size);
	if (!channel) {
		n_warn(N, __FUNCTION__, "Unable to open a session");
		ssh_close(N, sshconn, 1);
		return -1;
	}
	if (N->debug) n_warn(N, __FUNCTION__, "SCP session waiting to send file");
	nread=cobj3->val->size;
	ptr=cobj3->val->d.str;
	do {
		/* write data in a loop until we block */
		rc=libssh2_channel_write(channel, ptr, nread);
		ptr+=rc;
		nread-=rc;
	} while (rc>0);
	if (N->debug) n_warn(N, __FUNCTION__, "Sending EOF");
	libssh2_channel_send_eof(channel);
	if (N->debug) n_warn(N, __FUNCTION__, "Waiting for EOF");
	libssh2_channel_wait_eof(channel);
	if (N->debug) n_warn(N, __FUNCTION__, "Waiting for channel to close");
	libssh2_channel_wait_closed(channel);
	libssh2_channel_free(channel);
	channel=NULL;
	return rc;
#undef __FUNCTION__
}

NES_FUNCTION(neslassh_sftp_rename)
{
#define __FUNCTION__ __FILE__ ":neslassh_sftp_rename()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* sshconn */
	obj_t *cobj2=nes_getobj(N, &N->l, "2"); /* source */
	obj_t *cobj3=nes_getobj(N, &N->l, "3"); /* dest */
	obj_t *cobj=&N->r;
	SSH_CONN *sshconn;
	LIBSSH2_SFTP *sftp_session;
	int rc=0;

	nes_unlinkval(N, cobj);
	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "ssh-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sshconn for arg1");
	if ((cobj2->val->type!=NT_STRING)||(cobj2->val->d.str==NULL))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");
	sshconn=(SSH_CONN *)cobj1->val->d.str;
	if (sshconn->session==NULL) {
		n_warn(N, __FUNCTION__, "this session is dead");
		return -1;
	}
	nes_setstr(N, cobj, "", NULL, 0);
	if (N->debug) n_warn(N, __FUNCTION__, "libssh2_sftp_init()");
	sftp_session=libssh2_sftp_init(sshconn->session);
	if (!sftp_session) {
		n_warn(N, __FUNCTION__, "Unable to init SFTP session");
		return -1;
	}
	libssh2_session_set_blocking(sshconn->session, 1);
	if (N->debug) n_warn(N, __FUNCTION__, "libssh2_sftp_rename()");
	rc=libssh2_sftp_rename(sftp_session, cobj2->val->d.str, cobj3->val->d.str);
	libssh2_sftp_shutdown(sftp_session);
	return rc;
#undef __FUNCTION__
}

NES_FUNCTION(neslassh_sftp_rmdir)
{
#define __FUNCTION__ __FILE__ ":neslassh_sftp_rmdir()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* sshconn */
	obj_t *cobj2=nes_getobj(N, &N->l, "2"); /* remote dir */
	obj_t *cobj=&N->r;
	SSH_CONN *sshconn;
	LIBSSH2_SFTP *sftp_session;
	int rc=0;

	nes_unlinkval(N, cobj);
	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "ssh-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sshconn for arg1");
	if ((cobj2->val->type!=NT_STRING)||(cobj2->val->d.str==NULL))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");
	sshconn=(SSH_CONN *)cobj1->val->d.str;
	if (sshconn->session==NULL) {
		n_warn(N, __FUNCTION__, "this session is dead");
		return -1;
	}
	nes_setstr(N, cobj, "", NULL, 0);
	if (N->debug) n_warn(N, __FUNCTION__, "libssh2_sftp_init()");
	sftp_session=libssh2_sftp_init(sshconn->session);
	if (!sftp_session) {
		n_warn(N, __FUNCTION__, "Unable to init SFTP session");
		return -1;
	}
	libssh2_session_set_blocking(sshconn->session, 1);
	if (N->debug) n_warn(N, __FUNCTION__, "libssh2_sftp_rmdir()");
	rc=libssh2_sftp_rmdir(sftp_session, cobj2->val->d.str);
	libssh2_sftp_shutdown(sftp_session);
	return rc;
#undef __FUNCTION__
}

NES_FUNCTION(neslassh_sftp_unlink)
{
#define __FUNCTION__ __FILE__ ":neslassh_sftp_unlink()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1"); /* sshconn */
	obj_t *cobj2=nes_getobj(N, &N->l, "2"); /* remote file */
	obj_t *cobj=&N->r;
	SSH_CONN *sshconn;
	LIBSSH2_SFTP *sftp_session;
	int rc=0;

	nes_unlinkval(N, cobj);
	if ((cobj1->val->type!=NT_CDATA)||(cobj1->val->d.str==NULL)||(strcmp(cobj1->val->d.str, "ssh-conn")!=0))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a sshconn for arg1");
	if ((cobj2->val->type!=NT_STRING)||(cobj2->val->d.str==NULL))
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg2");
	sshconn=(SSH_CONN *)cobj1->val->d.str;
	if (sshconn->session==NULL) {
		n_warn(N, __FUNCTION__, "this session is dead");
		return -1;
	}
	nes_setstr(N, cobj, "", NULL, 0);
	if (N->debug) n_warn(N, __FUNCTION__, "libssh2_sftp_init()");
	sftp_session=libssh2_sftp_init(sshconn->session);
	if (!sftp_session) {
		n_warn(N, __FUNCTION__, "Unable to init SFTP session");
		return -1;
	}
	libssh2_session_set_blocking(sshconn->session, 1);
	if (N->debug) n_warn(N, __FUNCTION__, "libssh2_sftp_unlink()");
	rc=libssh2_sftp_unlink(sftp_session, cobj2->val->d.str);
	libssh2_sftp_shutdown(sftp_session);
	return rc;
#undef __FUNCTION__
}

int neslassh_register_all(nes_state *N)
{
	obj_t *tobj;
#ifdef WIN32
	static WSADATA wsaData;

	if (WSAStartup(0x101, &wsaData)) {
		n_warn(N, "neslassh_register_all", "Winsock init error");
		return -1;
	}
#endif
	tobj=nes_settable(N, &N->g, "ssh");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj, "open",        (NES_CFUNC)neslassh_ssh_open);
	nes_setcfunc(N, tobj, "close",       (NES_CFUNC)neslassh_ssh_close);
	nes_setcfunc(N, tobj, "hostkey",     (NES_CFUNC)neslassh_ssh_hostkey);
	nes_setcfunc(N, tobj, "auth",        (NES_CFUNC)neslassh_ssh_auth);
	nes_setcfunc(N, tobj, "cmd",         (NES_CFUNC)neslassh_ssh_cmd);
	nes_setcfunc(N, tobj, "sftp_get",    (NES_CFUNC)neslassh_sftp_get);
	nes_setcfunc(N, tobj, "sftp_ls",     (NES_CFUNC)neslassh_sftp_ls);
	nes_setcfunc(N, tobj, "sftp_mkdir",  (NES_CFUNC)neslassh_sftp_mkdir);
	nes_setcfunc(N, tobj, "sftp_put",    (NES_CFUNC)neslassh_sftp_put);
	nes_setcfunc(N, tobj, "sftp_rename", (NES_CFUNC)neslassh_sftp_rename);
	nes_setcfunc(N, tobj, "sftp_rmdir",  (NES_CFUNC)neslassh_sftp_rmdir);
	nes_setcfunc(N, tobj, "sftp_unlink", (NES_CFUNC)neslassh_sftp_unlink);
	return 0;
}

#ifdef PIC
DllExport int neslalib_init(nes_state *N)
{
	neslassh_register_all(N);
	return 0;
}
#endif

#endif /* HAVE_SSH2 */
