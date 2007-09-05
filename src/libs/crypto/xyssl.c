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

#ifdef HAVE_XYSSL

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include "xyssl/aes.h"
#include "xyssl/md5.h"
#include "xyssl/sha1.h"
#include "xyssl/sha2.h"

#include <stdio.h>
#include <string.h>

/*
 * dunno how much bigger dest will be, so just overalloc by:
 * would the absolute max needed be 64? verify later..
 */
#define CHICKENBUF 64

NES_FUNCTION(neslaxyssl_aes_encrypt)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1); /* source data */
	obj_t *cobj2=nes_getiobj(N, &N->l, 2); /* key */
	obj_t *robj;
	uchar IV[16];
	uchar key[512];
	uchar digest[32];
	uchar buffer[1024];
	char *p;
	int keylen, lastn;
	int i;
	unsigned int n;
	char *iptr;
	char *optr;
	int osize;
	unsigned int offset;
	aes_context aes_ctx;
	sha2_context sha_ctx;

	if ((cobj1->val->type!=NT_STRING)||(cobj1->val->size<1)) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if ((cobj2->val->type!=NT_STRING)||(cobj2->val->size<1)) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	if ( memcmp( cobj2->val->d.str, "hex:", 4 ) == 0 ) {
		p=cobj2->val->d.str+4;
		keylen=0;
		while ( sscanf( p, "%02X", &n ) > 0 && keylen < (int) sizeof( key ) ) {
			key[keylen++] = n;
			p += 2;
		}
	} else {
		keylen=cobj2->val->size>sizeof(key)?sizeof(key):cobj2->val->size;
		memcpy(key, cobj2->val->d.str, keylen);
	}
	robj=nes_setstr(N, &N->r, "", NULL, 0);
	if ((robj->val->d.str=n_alloc(N, cobj1->val->size+1+CHICKENBUF, 0))==NULL) return 0;
	robj->val->size=0;
	iptr=cobj1->val->d.str;
	optr=robj->val->d.str;
	osize=0;
	/*
	 * Generate the initialization vector as:
	 * IV = SHA-256( filesize || filename )[0..15]
	 */
	for( i = 0; i < 8; i++ )
		buffer[i] = (unsigned char)( cobj1->val->size >> ( i << 3 ) );

	sha2_starts( &sha_ctx, 0 );
	sha2_update( &sha_ctx, buffer, 8 );
	/*
	 * but we don't _have_ a filename..
	 * should something else be used? or do we care?
	 */
	/* sha2_update( &sha_ctx, (unsigned char *) p, strlen( p ) ); */
	sha2_finish( &sha_ctx, digest );

	memcpy( IV, digest, 16 );

	/*
	 * The last four bits in the IV are actually used
	 * to store the file size modulo the AES block size.
	 */
	lastn = (int) ( cobj1->val->size & 0x0F );

	IV[15] &= 0xF0;
	IV[15] |= lastn;

	/*
	 * Append the IV at the beginning of the output.
	 */
	memcpy(optr, IV, 16);
	optr+=16;
	osize+=16;

	/*
	 * Hash the IV and the secret key together 8192 times
	 * using the result to setup the AES context and HMAC.
	 */
	memset( digest, 0,  32 );
	memcpy( digest, IV, 16 );

	for (i=0;i<8192;i++) {
		sha2_starts(&sha_ctx, 0);
		sha2_update(&sha_ctx, digest, 32);
		sha2_update(&sha_ctx, key, keylen);
		sha2_finish(&sha_ctx, digest);
	}

	memset( key, 0, sizeof( key ) );
	aes_set_key( &aes_ctx, digest, 256 );
	sha2_hmac_starts( &sha_ctx, 0, digest, 32 );

	/*
	 * Encrypt and write the ciphertext.
	 */
	for ( offset = 0; offset < cobj1->val->size; offset += 16 ) {
		n = ( cobj1->val->size - offset > 16 ) ? 16 : (int) ( cobj1->val->size - offset );

		memcpy(buffer, iptr, n);
		iptr+=n;

		for ( i = 0; i < 16; i++ ) buffer[i] ^= IV[i];

		aes_encrypt( &aes_ctx, buffer, buffer );
		sha2_hmac_update( &sha_ctx, buffer, 16 );

		memcpy(optr, buffer, 16);
		optr+=16;
		osize+=16;

		memcpy( IV, buffer, 16 );
	}

	/*
	 * Finally write the HMAC.
	 */
	sha2_hmac_finish( &sha_ctx, digest );

	memcpy(optr, digest, 32);
	optr+=32;
	osize+=32;

	memset( buffer, 0, sizeof( buffer ) );
	memset( digest, 0, sizeof( digest ) );
	memset( &aes_ctx, 0, sizeof(  aes_context ) );
	memset( &sha_ctx, 0, sizeof( sha2_context ) );

	robj->val->size=osize;
	robj->val->d.str[osize]='\0';
	return 0;
}

NES_FUNCTION(neslaxyssl_aes_decrypt)
{
	obj_t *cobj1=nes_getiobj(N, &N->l, 1); /* source data */
	obj_t *cobj2=nes_getiobj(N, &N->l, 2); /* key */
	obj_t *robj;
	uchar IV[16];
	uchar key[512];
	uchar digest[32];
	uchar buffer[1024];
	char *p;
	unsigned int keylen, lastn;
	int i;
	unsigned int n;
	char *iptr;
	char *optr;
	int osize;
	unsigned int offset;
	aes_context aes_ctx;
	sha2_context sha_ctx;
	uchar tmp[16];

	if ((cobj1->val->type!=NT_STRING)||(cobj1->val->size<1)) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg1");
	if ((cobj2->val->type!=NT_STRING)||(cobj2->val->size<1)) n_error(N, NE_SYNTAX, nes_getstr(N, &N->l, "0"), "expected a string for arg2");
	if ( memcmp( cobj2->val->d.str, "hex:", 4 ) == 0 ) {
		p=cobj2->val->d.str+4;
		keylen=0;
		while ( sscanf( p, "%02X", &n ) > 0 && keylen < (int) sizeof( key ) ) {
			key[keylen++] = n;
			p += 2;
		}
	} else {
		keylen=cobj2->val->size>sizeof(key)?sizeof(key):cobj2->val->size;
		memcpy(key, cobj2->val->d.str, keylen);
	}

	/*
	 *  The encrypted file must be structured as follows:
	 *
	 *        00 .. 15              Initialization Vector
	 *        16 .. 31              AES Encrypted Block #1
	 *           ..
	 *      N*16 .. (N+1)*16 - 1    AES Encrypted Block #N
	 *  (N+1)*16 .. (N+1)*16 + 32   HMAC-SHA-256(ciphertext)
	 */
	if (cobj1->val->size<48) {
		n_warn(N, "xyssl.aes_decrypt", "File too short to be encrypted." );
		return 0;
	}

	if ( ( cobj1->val->size & 0x0F ) != 0 ) {
		n_warn(N, "xyssl.aes_decrypt", "File size not a multiple of 16." );
		return 0;
	}

	robj=nes_setstr(N, &N->r, "", NULL, 0);
	if ((robj->val->d.str=n_alloc(N, cobj1->val->size+1+CHICKENBUF, 0))==NULL) return 0;
	robj->val->size=0;
	iptr=cobj1->val->d.str;
	optr=robj->val->d.str;
	osize=0;

	/*
	 * Substract the IV + HMAC length.
	 */
	cobj1->val->size -= ( 16 + 32 );

	/*
	 * Read the IV and original filesize modulo 16.
	 */
	memcpy(buffer, iptr, 16);
	iptr+=16;

	memcpy( IV, buffer, 16 );
	lastn = IV[15] & 0x0F;

	/*
	 * Hash the IV and the secret key together 8192 times
	 * using the result to setup the AES context and HMAC.
	 */
	memset( digest, 0,  32 );
	memcpy( digest, IV, 16 );

	for( i = 0; i < 8192; i++ ) {
		sha2_starts( &sha_ctx, 0 );
		sha2_update( &sha_ctx, digest, 32 );
		sha2_update( &sha_ctx, key, keylen );
		sha2_finish( &sha_ctx, digest );
	}

	memset( key, 0, sizeof( key ) );
	aes_set_key( &aes_ctx, digest, 256 );
	sha2_hmac_starts( &sha_ctx, 0, digest, 32 );

	/*
	 * Decrypt and write the plaintext.
	 */
	for( offset = 0; offset < cobj1->val->size; offset += 16 ) {
		memcpy(buffer, iptr, 16);
		iptr+=16;

		memcpy( tmp, buffer, 16 );

		sha2_hmac_update( &sha_ctx, buffer, 16 );
		aes_decrypt( &aes_ctx, buffer, buffer );

		for ( i = 0; i < 16; i++ ) buffer[i] ^= IV[i];

		memcpy( IV, tmp, 16 );

		n = ( lastn > 0 && offset == cobj1->val->size - 16 ) ? lastn : 16;

		memcpy(optr, buffer, n);
		optr+=n;
		osize+=n;
	}

	/*
	 * Verify the message authentication code.
	 */
	sha2_hmac_finish( &sha_ctx, digest );

	memcpy(buffer, iptr, 32);
	iptr+=32;

	if ( memcmp( digest, buffer, 32 ) != 0 ) {
		n_warn(N, "xyssl.aes_decrypt", "HMAC check failed: wrong key, or file corrupted." );
	}

	memset( buffer, 0, sizeof( buffer ) );
	memset( digest, 0, sizeof( digest ) );
	memset( &aes_ctx, 0, sizeof(  aes_context ) );
	memset( &sha_ctx, 0, sizeof( sha2_context ) );

	robj->val->size=osize;
	robj->val->d.str[osize]='\0';
	return 0;
}

#endif /* HAVE_XYSSL */
