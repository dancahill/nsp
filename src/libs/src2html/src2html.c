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
#include "../../libnesla/opcodes.h"

static void n_addblank(nes_state *N, obj_t *cobj)
{
	uchar *p=N->readptr;

	while (*p) {
		N->readptr=p;
		if (p[0]=='#') {
			p++;
			nes_strcat(N, cobj, "<FONT COLOR=GREEN><I>", -1);
			while (*p) {
				if (*p=='\r'||*p=='\n') { break; }
				if (*p=='<') {
					nes_strcat(N, cobj, (char *)N->readptr, p-N->readptr);
					nes_strcat(N, cobj, "&lt;", -1);
					N->readptr=++p;
				} else {
					p++;
				}
			}
			nes_strcat(N, cobj, (char *)N->readptr, p-N->readptr);
			nes_strcat(N, cobj, "</I></FONT>", -1);
		} else if (p[0]=='/'&&p[1]=='/') {
			p+=2;
			nes_strcat(N, cobj, "<FONT COLOR=GREEN><I>", -1);
			while (*p) {
				if (*p=='\r'||*p=='\n') { break; }
				if (*p=='<') {
					nes_strcat(N, cobj, (char *)N->readptr, p-N->readptr);
					nes_strcat(N, cobj, "&lt;", -1);
					N->readptr=++p;
				} else {
					p++;
				}
			}
			nes_strcat(N, cobj, (char *)N->readptr, p-N->readptr);
			nes_strcat(N, cobj, "</I></FONT>", -1);
		} else if (p[0]=='/'&&p[1]=='*') {
			p+=2;
			nes_strcat(N, cobj, "<FONT COLOR=GREEN><I>", -1);
			while (*p) {
				if (p[0]=='*'&&p[1]=='/') { p+=2; break; }
				if (*p=='<') {
					nes_strcat(N, cobj, (char *)N->readptr, p-N->readptr);
					nes_strcat(N, cobj, "&lt;", -1);
					N->readptr=++p;
				} else {
					p++;
				}
			}
			nes_strcat(N, cobj, (char *)N->readptr, p-N->readptr);
			nes_strcat(N, cobj, "</I></FONT>", -1);
		}
		if (!nc_isspace(*p)) break;
		if (*p=='\n') {
			nes_strcat(N, cobj, "\n", -1);
			p++;
			continue;
		} else if (*p=='\t') {
			nes_strcat(N, cobj, "        ", -1);
			p++;
			continue;
		}
		nes_strcat(N, cobj, (char *)N->readptr, ++p-N->readptr);
	}
	N->readptr=p;
	return;
}

static void n_addquote(nes_state *N, unsigned short c, obj_t *cobj)
{
#define __FUNCTION__ __FILE__ ":n_addquote()"
	uchar *p=N->readptr;

	p++;
	while (*p) {
		if (*p=='<') {
			nes_strcat(N, cobj, (char *)N->readptr, p-N->readptr);
			nes_strcat(N, cobj, "&lt;", -1);
			N->readptr=++p;
			continue;
		} else if (*p=='&') {
			nes_strcat(N, cobj, (char *)N->readptr, p-N->readptr);
			nes_strcat(N, cobj, "&amp;", -1);
			N->readptr=++p;
			continue;
		} else if (*p=='\\') {
			p++;
		} else if (*p==c) {
			p++;
			break;
		}
		p++;
		if (!*p) {
			n_warn(N, __FUNCTION__, "unterminated string");
			return;
		}
	}
	nes_strcat(N, cobj, (char *)N->readptr, p-N->readptr);
	N->readptr=p;
	return;
#undef __FUNCTION__
}

static obj_t *n_src2html(nes_state *N, uchar *rawtext)
{
#define __FUNCTION__ __FILE__ ":n_src2html()"
	char lastname[MAX_OBJNAMELEN+1];
	obj_t *cobj;
	char *p;
	short op;

	N->readptr=rawtext;
	cobj=nes_setstr(N, &N->r, "", "<PRE>", -1);
	while (*N->readptr) {
		n_addblank(N, cobj);
		op=n_getop(N, lastname);
		if (op==OP_UNDEFINED) {
			n_warn(N, __FUNCTION__, "bad op? index=x op=%d:%d name='%s'", op, N->readptr[0], lastname);
			nl_flush(N);
			/* printf("\n%.40s\n", N->readptr); */
			return NULL;
		}
		if (OP_ISMATH(op)) {
			nes_strcat(N, cobj, "<FONT COLOR=MAROON><B>", -1);
			if (op==OP_MCLT) {
				nes_strcat(N, cobj, "&lt;", -1);
			} else if (op==OP_MCLE) {
				nes_strcat(N, cobj, "&lt;=", -1);
			} else {
				nes_strcat(N, cobj, n_getsym(N, op), -1);
			}
			nes_strcat(N, cobj, "</B></FONT>", -1);
		} else if (OP_ISKEY(op)) {
			nes_strcat(N, cobj, "<B>", -1);
			nes_strcat(N, cobj, n_getsym(N, op), -1);
			nes_strcat(N, cobj, "</B>", -1);
		} else if (OP_ISPUNC(op)) {
			nes_strcat(N, cobj, "<FONT COLOR=MAROON><B>", -1);
			nes_strcat(N, cobj, n_getsym(N, op), -1);
			nes_strcat(N, cobj, "</B></FONT>", -1);
		} else if (op==OP_LABEL) {
			nes_strcat(N, cobj, lastname, nc_strlen(lastname));
		} else if (nc_isdigit(*N->readptr)) {
			nes_strcat(N, cobj, "<FONT COLOR=NAVY>", -1);
			p=(char *)N->readptr;
			while (nc_isdigit(*N->readptr)||*N->readptr=='.') N->readptr++;
			nes_strcat(N, cobj, p, (char *)N->readptr-p);
			nes_strcat(N, cobj, "</FONT>", -1);
		} else if ((*N->readptr=='\"')||(*N->readptr=='\'')) {
			nes_strcat(N, cobj, "<FONT COLOR=BLUE>", -1);
			n_addquote(N, *N->readptr, cobj);
			nes_strcat(N, cobj, "</FONT>", -1);
		}
		n_addblank(N, cobj);
	}
	nes_strcat(N, cobj, "</PRE>", -1);
	return cobj;
#undef __FUNCTION__
}

static NES_FUNCTION(nesla_src2html)
{
#define __FUNCTION__ __FILE__ ":nesla_src2html()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1");
	uchar *p=N->readptr;

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg1");
	n_src2html(N, (uchar *)nes_tostr(N, cobj1));
	N->readptr=p;
	return 0;
#undef __FUNCTION__
}

static int nesla_src2html_register_all(nes_state *N)
{
	nes_setcfunc(N, &N->g, "src2html", (NES_CFUNC)nesla_src2html);
	return 0;
}

#ifdef PIC
DllExport int neslalib_init(nes_state *N)
{
	nesla_src2html_register_all(N);
	return 0;
}
#endif
