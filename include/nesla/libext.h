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
/* base64.c */
int neslaext_base64_decode(nes_state *N);
int neslaext_base64_encode(nes_state *N);
/* dir.c */
int neslaext_dirlist(nes_state *N);
/* rot13.c */
int neslaext_rot13(nes_state *N);
/* xml.c */
int neslaext_xml_read(nes_state *N);

int neslaext_register_all(nes_state *N);
