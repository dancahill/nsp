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

int neslacrypto_register_all(nes_state *N)
{
	obj_t *tobj;

	tobj=nes_settable(N, &N->g, "crypto");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj, "aes_cbc_encrypt", (NES_CFUNC)neslacrypto_aes_encrypt);
	nes_setcfunc(N, tobj, "aes_cbc_decrypt", (NES_CFUNC)neslacrypto_aes_decrypt);
	nes_setcfunc(N, tobj, "aes_ecb_encrypt", (NES_CFUNC)neslacrypto_aes_encrypt);
	nes_setcfunc(N, tobj, "aes_ecb_decrypt", (NES_CFUNC)neslacrypto_aes_decrypt);
	nes_setcfunc(N, tobj, "md5_passwd",      (NES_CFUNC)neslacrypto_md5_passwd);

	tobj=nes_settable(N, &N->g, "file");
	nes_setcfunc(N, tobj, "md5",             (NES_CFUNC)neslacrypto_md5_file);
	nes_setcfunc(N, tobj, "sha1",            (NES_CFUNC)neslacrypto_sha1_file);
	tobj=nes_settable(N, &N->g, "string");
	nes_setcfunc(N, tobj, "md5",             (NES_CFUNC)neslacrypto_md5_string);
	nes_setcfunc(N, tobj, "sha1",            (NES_CFUNC)neslacrypto_sha1_string);
	return 0;
}

#ifdef PIC
DllExport int neslalib_init(nes_state *N)
{
	neslacrypto_register_all(N);
	return 0;
}
#endif
