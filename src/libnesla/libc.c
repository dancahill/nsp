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
#include "libnesla.h"
#include <stdarg.h>
#ifdef WIN32
#include <time.h>
#include <sys/timeb.h>
#endif
#ifdef __TURBOC__
#include <time.h>
#include <sys/timeb.h>
#endif

/* *printf() */
static const char nchars[]="0123456789ABCDEF";

int nc_vsnprintf(nes_state *N, char *dest, int max, const char *format, va_list ap)
{
	char tmp[80];
	char *d, *s, *p;
	char esc=0;
	int len=0;

	for (s=(char *)format,d=dest;*s!='\0';s++) {
		if (!esc) {
			if (*s=='%') esc=1; else { *d++=*s; len++; }
			continue;
		}
		/* flags */
		if (*s=='-') s++;
		/* width */
		if (nc_isdigit(*s)) { while (nc_isdigit(*s)) s++; }
		/* precision */
		if (*s=='.') {
			s++;
			if (nc_isdigit(*s)) { while (nc_isdigit(*s)) s++; }
		}
		switch (*s) {
		case 'c': tmp[0]=(char)va_arg(ap, int); tmp[1]=0; goto end;
		case 'd': p=nes_ntoa(N, tmp, va_arg(ap, int),          -10, 0); goto out;
		case 'i': p=nes_ntoa(N, tmp, va_arg(ap, int),          -10, 0); goto out;
		case 'o': p=nes_ntoa(N, tmp, va_arg(ap, int),            8, 0); goto out;
		case 'u': p=nes_ntoa(N, tmp, va_arg(ap, unsigned int),  10, 0); goto out;
		case 'x': p=nes_ntoa(N, tmp, va_arg(ap, unsigned int),  16, 0); goto out;
		case 'f': p=nes_ntoa(N, tmp, va_arg(ap, double),       -10, 6); goto out;
		case 's': p=va_arg(ap, char *); if (p==NULL) p="(null)"; goto out;
		default : p="orphan %"; goto out;
		}
out:
		nc_strncpy(d, p, max-len); len+=nc_strlen(p); d=dest+len;
end:
		esc=0;
	}
	*d='\0';
	return len;
}

int nc_snprintf(nes_state *N, char *str, int size, const char *format, ...)
{
	va_list ap;
	int len;

	va_start(ap, format);
	len=nc_vsnprintf(N, str, size, format, ap);
	va_end(ap);
	return len;
}

int nc_printf(nes_state *N, const char *format, ...)
{
	va_list ap;
	int len;

	if (N->outbuflen>OUTBUFLOWAT) nl_flush(N);
	va_start(ap, format);
	len=nc_vsnprintf(N, N->outbuf+N->outbuflen, MAX_OUTBUFLEN-N->outbuflen, format, ap);
	N->outbuflen+=len;
	va_end(ap);
	return len;
}

/* time stuff */
int nc_gettimeofday(struct timeval *tv, void *tz)
{
#ifdef WIN32
	struct timeb tb;

	if (tv==NULL) return -1;
	ftime(&tb);
	tv->tv_sec=tb.time;
	tv->tv_usec=tb.millitm*1000;
#else
#ifdef __TURBOC__
	struct timeb tb;

	if (tv==NULL) return -1;
	ftime(&tb);
	tv->tv_sec=tb.time;
	tv->tv_usec=tb.millitm*1000;
#else
	gettimeofday(tv, tz);
#endif
#endif
	return 0;
}

int nc_strlen(char *s)
{
	int len=0;

	while (*s++!=0) len++;
	return len;
}

char *nc_strchr(const char *s, int c)
{
	uchar *a=(uchar *)s;

	do {
		if (*a==c) return (char *)a;
	} while (*a++);
	return NULL;
}

char *nc_strncpy(char *dst, const char *src, int n)
{
	uchar *s=(uchar *)src, *d=(uchar *)dst;

	if (n>0) {
		if (src!=NULL) {
			while (*s!=0&&--n>0) *d++=*s++;
		}
		*d=0;
	}
	return dst;
}

int nc_strcmp(const char *s1, const char *s2)
{
	uchar *a=(uchar *)s1, *b=(uchar *)s2;

	if (a==b) return 0;
	if (!a) a=(uchar *)"";
	do {
		if (*a!=*b) return *a-*b;
		if (*a==0) break;
		a++; b++;
	} while (1);
	return 0;
}

int nc_strncmp(const char *s1, const char *s2, int n)
{
	uchar *a=(uchar *)s1, *b=(uchar *)s2;

	if (a==b) return 0;
	do {
		if (*a!=*b) return *a-*b;
		if (*a==0) return 0;
		a++; b++;
	} while (--n>0);
	return 0;
}

void *nc_memset(void *s, int c, int n)
{
	uchar *a=s;

	while (n) a[n--]=c;
	return s;
	/* return memset(s, c, n); */
}

void n_error(nes_state *N, short int err, const char *fname, const char *format, ...)
{
	char ptrtxt[MAX_OBJNAMELEN+1];
	char *p=ptrtxt;
	va_list ap;
	int len;

	nc_strncpy(ptrtxt, (char *)N->readptr, MAX_OBJNAMELEN);
	N->err=err;
	if (N->err) {
		while (*p) {
			if ((*p=='\r')||(*p=='\n')||(*p=='\t')) *p=' ';
			p++;
		}
		len=nc_snprintf(N, N->errbuf, sizeof(N->errbuf)-1, "%-15s : ", fname);
		va_start(ap, format);
		len+=nc_vsnprintf(N, N->errbuf+len, sizeof(N->errbuf)-len-1, format, ap);
		va_end(ap);
		nc_snprintf(N, N->errbuf+len, sizeof(N->errbuf)-len-1, "\r\n\tN->readptr=\"%s\"", ptrtxt);
	}
	nl_flush(N);
	longjmp(N->savjmp, 1);
	return;
}

void n_warn(nes_state *N, const char *fname, const char *format, ...)
{
	va_list ap;
	short i;
	char *p;

	if (N->warnings++>10000) n_error(N, NE_SYNTAX, "n_warn", "too many warnings (%d)\n", N->warnings);
	if (N->outbuflen>OUTBUFLOWAT) nl_flush(N);
	nc_printf(N, "[01;33;40m%s\r\t\t : ", fname);
	va_start(ap, format);
	N->outbuflen+=nc_vsnprintf(N, N->outbuf+N->outbuflen, MAX_OUTBUFLEN-N->outbuflen, format, ap);
	va_end(ap);
	nc_printf(N, "\r\t\t\t\t\t\tN->readptr = ");
	for (i=0;i<40;i++) {
		p=(char *)N->readptr+i;
		if (!*p) break;
		N->outbuf[N->outbuflen++]=nc_isspace(*p)?' ':*p;
	}
	nc_printf(N, "\r\n[00m");
	nl_flush(N);
	return;
}


/*
 * the following functions are public API functions
 */
num_t nes_aton(nes_state *N, const char *str)
{
	char *s=(char *)str;
	num_t dot=0;
	num_t rval=0;

	while (nc_isdigit(*s)) {
		rval=10*rval+(*s-'0');
		s++;
	}
	if (*s=='.') {
		dot=1;
		s++;
		while (nc_isdigit(*s)) {
			dot*=0.1;
			rval+=(*s-'0')*dot;
			s++;
		}
	}
	return rval;
}

char *nes_ntoa(nes_state *N, char *str, num_t num, short base, unsigned short dec)
{
	int n=(int)num;
	num_t f=(num_t)num-(int)num;
	unsigned int i;
	char c, sign='+';
	char *p, *q;

	if ((num<0)&&(base<0)) {
		sign='-';
		n*=-1;
		f*=-1;
	}
	base=base<0?-base:base;
	p=q=str;
	do {
		*p++=nchars[n%base];
	} while ((n/=base)>0);
	if (sign=='-') *p++='-';
	*p='\0';
	/* and flip */
	while (q<--p) {
		c=*q;
		*q++=*p;
		*p=c;
	}
	if (dec) {
		while (*p) p++;
		*p++='.';
		for (i=0;i<dec;i++) {
			f*=base;
			*p++=nchars[(int)f%base];
		}
		*p='\0';
		/* insert rounding stuff around here.. */

		/* trim the zeros */
		i=nc_strlen(str)-1;
		while (str[i]=='0') str[i--]='\0';
		if (str[i]=='.') str[i--]='\0';
	}
	return str;
}
