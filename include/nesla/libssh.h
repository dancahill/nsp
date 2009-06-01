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
#ifdef HAVE_SSH2

#ifdef WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
#include <libssh2.h>
#include <libssh2_sftp.h>

typedef struct {
	/* standard header info for CDATA object */
	char      obj_type[16]; /* tell us all about yourself in 15 characters or less */
	NES_CFREE obj_term;     /* now tell us how to kill you */
	/* now begin the stuff that's libssh-specific */
	LIBSSH2_SESSION *session;
	LIBSSH2_CHANNEL *sh_channel;
	int socket;
	char LocalAddr[16];
	int  LocalPort;
	char RemoteAddr[16];
	int  RemotePort;
	char HostKey[16];
//	short int want_close;
} SSH_CONN;

int neslassh_register_all(nes_state *N);

#endif /* HAVE_SSH2 */
