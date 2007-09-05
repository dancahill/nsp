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

#include "nesla/libnesla.h"
#include "nesla/libcrypt.h"
/*
//#include <stdlib.h>
//#include <string.h>
//#include "rijndael.h"

static NES_FUNCTION(nesla_aes_decrypt)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	rijndael_ctx ctx;

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	rijndael_set_key(&ctx, (uchar *)cobj2->val->d.str, 256, 0);
//	rijndael_decrypt(&ctx,cryptpasswd,user.password);
	return 0;
}

static NES_FUNCTION(nesla_aes_encrypt)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1);
	obj_t *cobj2=nes_getiobj(N, &N->l, 2);
	rijndael_ctx ctx;

	if (cobj1->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if (cobj2->val->type!=NT_STRING) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	rijndael_set_key(&ctx, (uchar *)cobj2->val->d.str, 256, 1);
//	rijndael_encrypt(&ctx,user.password,cryptpasswd);
	return 0;
}
*/

int neslacrypto_register_all(nes_state *N)
{
	obj_t *tobj;

	tobj=nes_settable(N, &N->g, "crypto");
	tobj->val->attr|=NST_HIDDEN;
#ifdef HAVE_XYSSL
	nes_setcfunc(N, tobj, "aes_encrypt", (NES_CFUNC)neslaxyssl_aes_encrypt);
	nes_setcfunc(N, tobj, "aes_decrypt", (NES_CFUNC)neslaxyssl_aes_decrypt);
#endif
	nes_setcfunc(N, tobj, "md5_file",    (NES_CFUNC)neslacrypto_md5_file);
	nes_setcfunc(N, tobj, "md5_string",  (NES_CFUNC)neslacrypto_md5_string);
	nes_setcfunc(N, tobj, "md5_passwd",  (NES_CFUNC)neslacrypto_md5_passwd);
	nes_setcfunc(N, tobj, "sha1_file",   (NES_CFUNC)neslacrypto_sha1_file);
	nes_setcfunc(N, tobj, "sha1_string", (NES_CFUNC)neslacrypto_sha1_string);
	return 0;
}

#ifdef PIC
DllExport int neslalib_init(nes_state *N)
{
	neslacrypto_register_all(N);
	return 0;
}
#endif
