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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define HAVE_THREADS
#include <process.h>
#endif

#ifdef HAVE_THREADS
typedef struct {
	/* standard header info for CDATA object */
	char      obj_type[16]; /* tell us all about yourself in 15 characters or less */
	NSP_CFREE obj_term;     /* now tell us how to kill you */
				/* now begin the stuff that's thread-specific */
#ifdef WIN32
	HANDLE handle;
	unsigned int id;
#else
	pthread_t handle;
	pthread_t id;
#endif
	nsp_state *N;
	nsp_state *parentN;
	obj_t this;
	//int socket;
	//short use_tls;
	//char LocalAddr[16];
	//int  LocalPort;
	//char RemoteAddr[16];
	//int  RemotePort;
	//unsigned int bytes_in;
	//unsigned int bytes_out;
	//short int want_close;
} OS_THREAD;
#endif

/* base64.c */
NSP_FUNCTION(libnsp_base_base64_decode);
NSP_FUNCTION(libnsp_base_base64_encode);
/* dir.c */
NSP_FUNCTION(libnsp_base_dirlist);
/* file.c */
NSP_FUNCTION(libnsp_base_file_chdir);
NSP_FUNCTION(libnsp_base_file_mkdir);
NSP_FUNCTION(libnsp_base_file_readall);
NSP_FUNCTION(libnsp_base_file_rename);
NSP_FUNCTION(libnsp_base_file_stat);
NSP_FUNCTION(libnsp_base_file_unlink);
NSP_FUNCTION(libnsp_base_file_writeall);
/* pipe.c */
#ifdef HAVE_PIPE
NSP_FUNCTION(libnsp_base_pipe_open);
NSP_FUNCTION(libnsp_base_pipe_read);
NSP_FUNCTION(libnsp_base_pipe_write);
NSP_FUNCTION(libnsp_base_pipe_close);
#endif
/* regex.c */
NSP_FUNCTION(libnsp_regex_match);
NSP_FUNCTION(libnsp_regex_replace);
/* rot13.c */
NSP_FUNCTION(libnsp_base_rot13);
/* sort.c */
NSP_FUNCTION(libnsp_base_sort_byname);
NSP_FUNCTION(libnsp_base_sort_bykey);
/* thread.c */
NSP_FUNCTION(libnsp_base_thread_finish);
NSP_FUNCTION(libnsp_base_thread_kill);
NSP_FUNCTION(libnsp_base_thread_start);
NSP_FUNCTION(libnsp_base_thread_thread);
/* winapi.c */
#ifdef WIN32
NSP_FUNCTION(libnsp_winapi_beep);
NSP_FUNCTION(libnsp_winapi_createprocess);
NSP_FUNCTION(libnsp_winapi_messagebox);
NSP_FUNCTION(libnsp_winapi_playsound);
NSP_FUNCTION(libnsp_winapi_shellexecute);
#endif

int nspbase_register_all(nsp_state *N);

#ifdef __cplusplus
}
#endif