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

#ifdef __cplusplus
extern "C" {
#endif

/* base64.c */
NES_FUNCTION(neslaext_base64_decode);
NES_FUNCTION(neslaext_base64_encode);
/* dir.c */
NES_FUNCTION(neslaext_dirlist);
/* mime.c */
NES_FUNCTION(neslaext_mime_read);
NES_FUNCTION(neslaext_mime_write);
NES_FUNCTION(neslaext_mime_qp_decode);
NES_FUNCTION(neslaext_mime_rfc2047_decode);
/* rot13.c */
NES_FUNCTION(neslaext_rot13);
/* sort.c */
NES_FUNCTION(neslaext_sort_byname);
NES_FUNCTION(neslaext_sort_bykey);
/* xml.c */
NES_FUNCTION(neslaext_xml_read);

int neslaext_register_all(nes_state *N);

#ifdef __cplusplus
}
#endif
