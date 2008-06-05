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

/*
 * untgz.c -- Display contents and extract files from a gzip'd TAR file
 *
 * written by Pedro A. Aranda Gutierrez <paag@tid.es>
 * adaptation to Unix by Jean-loup Gailly <jloup@gzip.org>
 * various fixes by Cosmin Truta <cosmint@cs.ubbcluj.ro>
 * hacked at extensively by Dan Cahill <nulllogic@gmail.com>
 */

#include "nesla/libnesla.h"
#include "nesla/libzip.h"
#include <stdio.h>
#include <string.h>

#ifdef HAVE_ZLIB
#include "zlib.h"
#endif

#ifdef WIN32
#include <io.h>
#define strcasecmp stricmp
#else
#ifdef __TURBOC__
#else
#include <unistd.h>
#endif
#endif

#ifndef F_OK
#define F_OK  0
#endif

#define TSUID		04000
#define TSGID		02000
#define TSVTX		01000
#define TUREAD		00400
#define TUWRITE		00200
#define TUEXEC		00100
#define TGREAD		00040
#define TGWRITE		00020
#define TGEXEC		00010
#define TOREAD		00004
#define TOWRITE		00002
#define TOEXEC		00001

#define REGTYPE		'0'	/* Regular file (preferred code).  */
#define AREGTYPE	'\0'	/* Regular file (alternate code).  */
#define LNKTYPE		'1'	/* Hard link.  */
#define SYMTYPE		'2'	/* Symbolic link (hard if not supported).  */
#define CHRTYPE		'3'	/* Character special.  */
#define BLKTYPE		'4'	/* Block special.  */
#define DIRTYPE		'5'	/* Directory.  */
#define FIFOTYPE	'6'	/* Named pipe.  */
#define CONTTYPE	'7'	/* Contiguous file */

#define TMAGIC		"ustar"
#define TMAGLEN		6

#define TVERSION	"00"
#define TVERSLEN	2

#define BLOCKSIZE	512
#define SHORTNAMESIZE	100

struct tar_header
{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
};

union tar_buffer
{
	char              buffer[BLOCKSIZE];
	struct tar_header header;
};

struct attr_item
{
	struct attr_item  *next;
	char              *fname;
	int                mode;
	time_t             time;
};

enum { TAR_CACHE, TAR_EXTRACT, TAR_LIST, TAR_INVALID };

#define AR_RAW  0
#define AR_GZIP 1

static int getoct(char *p, int width)
{
	int result = 0;
	char c;

	while (width--) {
		c = *p++;
		if (c == 0) break;
		if (c == ' ') continue;
		if (c < '0' || c > '7') return -1;
		result = result * 8 + (c - '0');
	}
	return result;
}

static obj_t *subdir(nes_state *N, obj_t *tobj, char *dirname)
{
	char *p1, *p2;
	char namebuf[MAX_OBJNAMELEN+1];

	p1=dirname;
	while (*p1=='/') p1++;
	namebuf[MAX_OBJNAMELEN]='\0';
	for (;;) {
		if ((p2=strchr(p1, '/'))!=NULL) {
			*p2++='\0';
			strncpy(namebuf, p1, MAX_OBJNAMELEN);
			tobj=nes_settable(N, tobj, namebuf);
			p1=p2;
		} else if (strlen(p1)>0) {
			strncpy(namebuf, p1, MAX_OBJNAMELEN);
			tobj=nes_settable(N, tobj, namebuf);
			p1+=strlen(p1);
		} else {
			break;
		}
	}
	return tobj;
}

static int untar(nes_state *N, char *TARfile, int artype, int action, obj_t *tobj)
{
#define __FUNCTION__ __FILE__ ":untar()"
	union  tar_buffer buffer;
	int    len;
	int    getheader = 1;
	int    remaining = 0;
	FILE   *outfile = NULL;
	char   *outptr = NULL;
	char   fname[BLOCKSIZE];
	int    tarmode=0;
	time_t tartime;
	/* struct attr_item *attributes = NULL; */
	obj_t  *cobj, *tobj2;
	FILE *f=NULL;
#ifdef HAVE_ZLIB
	gzFile *fg=NULL;
	int    err;
#endif

	if (artype==AR_RAW) {
		f=fopen(TARfile, "rb");
		if (f==NULL) {
			nes_setnum(N, &N->r, "", -1);
			return -1;
		}
#ifdef HAVE_ZLIB
	} else if (artype==AR_GZIP) {
		fg=gzopen(TARfile, "rb");
		if (fg==NULL) {
			nes_setnum(N, &N->r, "", -1);
			return -1;
		}
#endif
	} else {
		nes_unlinkval(N, tobj);
		n_error(N, NE_SYNTAX, __FUNCTION__, "unknown compression type");
	}
	while (1) {
		len=0;
		if (artype==AR_RAW) {
			len=fread(&buffer, BLOCKSIZE, 1, f);
			len*=BLOCKSIZE;
			if (len<BLOCKSIZE) {
				nes_unlinkval(N, tobj);
				n_error(N, NE_SYNTAX, __FUNCTION__, "fread error");
			}
#ifdef HAVE_ZLIB
		} else if (artype==AR_GZIP) {
			len=gzread(fg, &buffer, BLOCKSIZE);
			if (len<0) {
				nes_unlinkval(N, tobj);
				n_error(N, NE_SYNTAX, __FUNCTION__, gzerror(fg, &err));
			}
#endif
		}
		/*
		 * Always expect complete blocks to process
		 * the tar information.
		 */
		if (len != BLOCKSIZE) {
			action = TAR_INVALID; /* force error exit */
			remaining = 0;        /* force I/O cleanup */
		}
		/*
		 * If we have to get a tar header
		 */
		if (getheader >= 1) {
			/*
			 * if we met the end of the tar
			 * or the end-of-tar block,
			 * we are done
			 */
			if (len == 0 || buffer.header.name[0] == 0)
				break;
			tarmode = getoct(buffer.header.mode,8);
			tartime = (time_t)getoct(buffer.header.mtime,12);
			if (tarmode == -1 || tartime == (time_t)-1) {
				buffer.header.name[0] = 0;
				action = TAR_INVALID;
			}
			if (getheader == 1) {
				strncpy(fname,buffer.header.name,SHORTNAMESIZE);
				if (fname[SHORTNAMESIZE-1] != 0)
					fname[SHORTNAMESIZE] = 0;
			} else {
				/*
				 * The file name is longer than SHORTNAMESIZE
				 */
				if (strncmp(fname,buffer.header.name,SHORTNAMESIZE-1) != 0) {
					nes_unlinkval(N, tobj);
					n_error(N, NE_SYNTAX, __FUNCTION__, "bad long name");
				}
				getheader = 1;
			}
			/*
			 * Act according to the type flag
			 */
			switch (buffer.header.typeflag) {
			case DIRTYPE:
				if ((action==TAR_CACHE)||(action==TAR_LIST)) {
					/* printf(" %s     <dir> %s\n",strtime(&tartime),fname); */
					tobj2=subdir(N, tobj, fname);
					nes_setnum(N, tobj2, "mtime", tartime);
					nes_setnum(N, tobj2, "size", 0);
					nes_setstr(N, tobj2, "type", "dir", 3);
				} else if (action==TAR_EXTRACT) {
/*
					makedir(fname);
					push_attr(N, &attributes,fname,tarmode,tartime);
*/
				}
				break;
			case REGTYPE:
			case AREGTYPE:
				remaining = getoct(buffer.header.size,12);
				if (remaining == -1) {
					action = TAR_INVALID;
					break;
				}
				if ((action==TAR_CACHE)||(action==TAR_LIST)) {
					/* printf(" %s %9d %s\n",strtime(&tartime),remaining,fname); */
					tobj2=subdir(N, tobj, fname);
					nes_setnum(N, tobj2, "mtime", tartime);
					nes_setnum(N, tobj2, "size", remaining);
					nes_setstr(N, tobj2, "type", "file", 4);
					if (action == TAR_CACHE) {
						/* if (matchname(arg,argc,argv,fname)) { */
						if (1) {
							cobj=nes_setstr(N, tobj2, "data", NULL, 0);
							cobj->val->size=remaining;
							cobj->val->d.str=n_alloc(N, cobj->val->size+1, 0);
							if (cobj->val->d.str==NULL) {
								nes_unlinkval(N, tobj);
								n_error(N, NE_SYNTAX, __FUNCTION__, "malloc() error while creating tar cache");
							}
							cobj->val->d.str[0]=0;
							cobj->val->d.str[cobj->val->size]=0;
							outptr=cobj->val->d.str;
						} else {
							outptr = NULL;
						}
					}
				} else if (action==TAR_EXTRACT) {
					/* if (matchname(arg,argc,argv,fname)) { */
					if (0) {
						outfile = fopen(fname,"wb");
						if (outfile == NULL) {
							/* try creating directory */
							char *p = strrchr(fname, '/');
							if (p != NULL) {
								*p = '\0';
								/* makedir(fname); */
								*p = '/';
								outfile = fopen(fname,"wb");
							}
						}
						if (outfile==NULL) {
							nes_unlinkval(N, tobj);
							n_error(N, NE_SYNTAX, __FUNCTION__, "error create file");
						}
					} else {
						outfile = NULL;
					}
				}
				getheader = 0;
				break;
			default:
				break;
			}
		} else {
			unsigned int bytes = (remaining > BLOCKSIZE) ? BLOCKSIZE : remaining;

			if (outfile != NULL) {
				if (fwrite(&buffer,sizeof(char),bytes,outfile) != bytes) {
					fclose(outfile);
					outfile = NULL;
					remove(fname);
					nes_unlinkval(N, tobj);
					n_error(N, NE_SYNTAX, __FUNCTION__, "error writing file");
				}
			} else if (outptr != NULL) {
				memcpy(outptr, &buffer, bytes);
				outptr+=bytes;
			}
			remaining -= bytes;
		}
		if (remaining == 0) {
			getheader = 1;
			if (outfile != NULL) {
				fclose(outfile);
				outfile = NULL;
				/* if (action!=TAR_INVALID) push_attr(N, &attributes,fname,tarmode,tartime); */
			}
		}
		/*
		 * Abandon if errors are found
		 */
		if (action == TAR_INVALID) {
			nes_unlinkval(N, tobj);
			n_error(N, NE_SYNTAX, __FUNCTION__, "broken tar archive");
			break;
		}
	}
	/*
	 * Restore file modes and time stamps
	 */
	/* restore_attr(&attributes); */

	if (artype==AR_RAW) {
		if (fclose(f)!=0) {
			nes_unlinkval(N, tobj);
			n_error(N, NE_SYNTAX, __FUNCTION__, "failed fclose");
		}
#ifdef HAVE_ZLIB
	} else if (artype==AR_GZIP) {
		if (gzclose(fg)!=Z_OK) {
			nes_unlinkval(N, tobj);
			n_error(N, NE_SYNTAX, __FUNCTION__, "failed gzclose");
		}
#endif
	}

	return 0;
#undef __FUNCTION__
}

NES_FUNCTION(neslazip_untar_list)
{
#define __FUNCTION__ __FILE__ ":neslazip_untar_list()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1");
	obj_t tobj;
	char *p;

	if (cobj1->val->type!=NT_STRING||cobj1->val->d.str==NULL) {
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg1");
		return -1;
	}
	if (access(cobj1->val->d.str, F_OK)!=0) {
		nes_setnum(N, &N->r, "", -2);
		return -1;
	}
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nes_linkval(N, &tobj, NULL);
	tobj.val->type=NT_TABLE;
	tobj.val->attr&=~NST_AUTOSORT;
	p=strrchr(cobj1->val->d.str, '.');
	if (p) {
		if ((strcasecmp(p, ".gz")==0)||(strcasecmp(p, ".tgz")==0)) {
			untar(N, cobj1->val->d.str, AR_GZIP, TAR_LIST, &tobj);
		} else {
			untar(N, cobj1->val->d.str, AR_RAW, TAR_LIST, &tobj);
		}
	} else {
		untar(N, cobj1->val->d.str, AR_RAW, TAR_LIST, &tobj);
	} 
	nes_linkval(N, &N->r, &tobj);
	nes_unlinkval(N, &tobj);
	return 0;
#undef __FUNCTION__
}

NES_FUNCTION(neslazip_untar_cache)
{
#define __FUNCTION__ __FILE__ ":neslazip_untar_cache()"
	obj_t *cobj1=nes_getobj(N, &N->l, "1");
	obj_t tobj;
	char *p;

	if (cobj1->val->type!=NT_STRING||cobj1->val->d.str==NULL) {
		n_error(N, NE_SYNTAX, __FUNCTION__, "expected a string for arg1");
		return -1;
	}
	if (access(cobj1->val->d.str, F_OK)!=0) {
		nes_setnum(N, &N->r, "", -2);
		return -1;
	}
	nc_memset((void *)&tobj, 0, sizeof(obj_t));
	nes_linkval(N, &tobj, NULL);
	tobj.val->type=NT_TABLE;
	tobj.val->attr&=~NST_AUTOSORT;
	p=strrchr(cobj1->val->d.str, '.');
	if (p) {
		if ((strcasecmp(p, ".gz")==0)||(strcasecmp(p, ".tgz")==0)) {
			untar(N, cobj1->val->d.str, AR_GZIP, TAR_CACHE, &tobj);
		} else {
			untar(N, cobj1->val->d.str, AR_RAW, TAR_CACHE, &tobj);
		}
	} else {
		untar(N, cobj1->val->d.str, AR_RAW, TAR_CACHE, &tobj);
	} 
	nes_linkval(N, &N->r, &tobj);
	nes_unlinkval(N, &tobj);
	return 0;
#undef __FUNCTION__
}

int neslazip_register_all(nes_state *N)
{
	obj_t *tobj;

	tobj=nes_settable(N, &N->g, "zip");
	tobj->val->attr|=NST_HIDDEN;
	nes_setcfunc(N, tobj, "list",  (NES_CFUNC)neslazip_untar_list);
	nes_setcfunc(N, tobj, "cache", (NES_CFUNC)neslazip_untar_cache);
	return 0;
}

#ifdef PIC
DllExport int neslalib_init(nes_state *N)
{
	neslazip_register_all(N);
	return 0;
}
#endif
