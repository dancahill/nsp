/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2022 Dan Cahill

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <emscripten/emscripten.h>

int nspbase_register_all(nsp_state *N);

nsp_state *N;
extern char **environ;

/*
https://developer.mozilla.org/en-US/docs/WebAssembly/C_to_wasm
https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html#calling-compiled-c-functions-from-javascript-using-ccall-cwrap
https://emscripten.org/docs/tools_reference/emsdk.html#emsdk-install-old-tools
https://emscripten.org/docs/api_reference/emscripten.h.html

to write to idbfs with stdio:
https://stackoverflow.com/questions/54617194/how-to-save-files-from-c-to-browser-storage-with-emscripten
*/

void jsinit()
{
	EM_ASM(
		nsp = {};
		nsp.runscript = function(target, script) {
			/* (name of C function), return type, argument types, arguments */
			return Module.ccall("runscript", "number", ["string", "string"], [target, script]);;
		};
		nsp.destroystate = function() {
			return Module.ccall("destroystate", "number", [], []);
		};
	);
}

EM_JS(void, divappend, (const char *div, const char *str), {
	try {
		document.getElementById(UTF8ToString(div)).insertAdjacentHTML("beforeend", UTF8ToString(str));
	} catch (ex) {
		console.warn("divappend() exception: ", ex);
	}
});

static int wasm_flush(nsp_state * N)
{
	obj_t *oobj = nsp_getobj(N, &N->g, "io");
	obj_t *cobj = nsp_getobj(N, oobj, "outputid");

	if (N == NULL || N->outbuflen == 0) return 0;
	if (!nsp_isstr(cobj)) {
		cobj = nsp_setstr(N, oobj, "outputid", "output", -1);
	}
	N->outbuffer[N->outbuflen] = '\0';
	divappend(nsp_tostr(N, cobj), N->outbuffer);
	//write(STDOUT_FILENO, N->outbuffer, N->outbuflen);
	N->outbuflen = 0;
	return 0;
}

void do_banner() {
	printf("\r\nNullLogic Embedded Scripting Language Version " NSP_VERSION);
	printf("\r\nCopyright (C) 2007-2021 Dan Cahill\r\n\r\n");
	return;
}

#define COMMABUFSIZE 32
static char *printcommas(int num, char *buf, short clear)
{
	/* unsafe cosmetic debut output */
	if (clear) buf[0] = 0;
	if (num > 999) printcommas(num / 1000, buf, 0);
	sprintf(buf + strlen(buf), "%0*d%s", num > 999 ? 3 : 1, num % 1000, clear ? "" : ",");
	return buf;
}

static void printstate(nsp_state *N, char *fn)
{
	char buf1[COMMABUFSIZE];
	char buf2[COMMABUFSIZE];

	printf("\r\nINTERNAL STATE REPORT %s\r\n", fn);
	if (N->allocs)   printf("\tallocs   = %s (%s bytes)\r\n", printcommas(N->allocs, buf1, 1), printcommas(N->allocmem, buf2, 1));
	if (N->frees)    printf("\tfrees    = %s (%s bytes)\r\n", printcommas(N->frees, buf1, 1), printcommas(N->freemem, buf2, 1));
	if (N->allocs)   printf("\tdiff     = %s (%s bytes)\r\n", printcommas(abs(N->allocs - N->frees), buf1, 1), printcommas(abs(N->allocmem - N->freemem), buf2, 1));
	if (N->peakmem)  printf("\tpeak     = %s bytes\r\n", printcommas(N->peakmem, buf1, 1));
	if (N->counter1) printf("\tcounter1 = %s\r\n", printcommas(N->counter1, buf1, 1));
	printf("\tsizeof(nsp_state)  = %d\r\n", (int)sizeof(nsp_state));
	printf("\tsizeof(nsp_objrec) = %d\r\n", (int)sizeof(nsp_objrec));
	printf("\tsizeof(nsp_valrec) = %d\r\n", (int)sizeof(nsp_valrec));
	return;
}

int nsp_createstate()
{
	char tmpbuf[MAX_OBJNAMELEN + 1];
	obj_t *tobj;
	int i;
	char *p;

	if (N != NULL) return 0;
	if ((N = nsp_newstate()) == NULL) {
		printf("WebAssembly nsp_createstate() failed to create new state\n");
		return -1;
	}
	//setsigs();
	N->debug = 0;
	nsp_setstr(N, nsp_getobj(N, nsp_settable(N, &N->g, "lib"), "io"), "outputid", "output", -1);
	nsp_setcfunc(N, nsp_settable(N, nsp_settable(N, &N->g, "lib"), "io"), "flush", wasm_flush);
	nspbase_register_all(N);
	/* add env */
	tobj = nsp_settable(N, &N->g, "_ENV");
	for (i = 0; environ[i] != NULL; i++) {
		strncpy(tmpbuf, environ[i], MAX_OBJNAMELEN);
		p = strchr(tmpbuf, '=');
		if (!p) continue;
		*p = '\0';
		p = strchr(environ[i], '=') + 1;
		nsp_setstr(N, tobj, tmpbuf, p, -1);
	}
	return 0;
}

int nsp_destroystate()
{
	if (N == NULL) return 0;
	wasm_flush(N);
	if (N->err) printf("N->err: %s\r\n", N->errbuf);
	//if (N->err) divappend("output", N->errbuf);
	nsp_freestate(N);
	//if (N->allocs != N->frees) printstate(N, fn);
	N = nsp_endstate(N);
	return 0;
}

EMSCRIPTEN_KEEPALIVE int setoutput(const char *id)
{
	obj_t *oobj = nsp_getobj(N, &N->g, "io");
	obj_t *cobj = nsp_getobj(N, oobj, "outputid");

	if (!nsp_isstr(cobj)) {
		nsp_setstr(N, oobj, "outputid", (char *)id, -1);
	} else {
		if (nc_strcmp(nsp_tostr(N, cobj), id) != 0) {
			wasm_flush(N);
			nsp_setstr(N, oobj, "outputid", (char *)id, -1);
		}
	}
	return 0;
}

EMSCRIPTEN_KEEPALIVE int runscript(const char *id, const char *str)
{
	//EM_ASM(
		//alert('hello world!');
		//throw 'all done';
	//);
	nsp_createstate();
	nsp_exec(N, str);
	wasm_flush(N);
	//nsp_destroystate();
	return 0;
}

EMSCRIPTEN_KEEPALIVE int destroystate(void)
{
	nsp_destroystate();
	return 0;
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	do_banner();
	N = NULL;
	//printf("WebAssembly module loaded\r\n");
	jsinit();
	nsp_createstate();
	nsp_exec(N, "printf('Hello World!');\r\n");
	wasm_flush(N);
	//nsp_destroystate();
	return 0;
}
