#define _ZLIB_H
 
 #include "zlib.h"
// #include "C:\_77\contrib\minizip\zip.h"

unsigned char ib_buff[2048];
unsigned char ou_buff[2048];

int main()
{
	z_stream Z;

	Z.zalloc = Z_NULL;
	Z.zfree  = Z_NULL;

	deflateInit(&Z, Z_DEFAULT_COMPRESSION);

	int level = Z_DEFAULT_COMPRESSION; /* compression level */
	int strategy = Z_DEFAULT_STRATEGY; /* compression strategy */

//	deflateInit( &Z, Z_DEFAULT_COMPRESSION);

	Z.next_in  = ib_buff;
	Z.avail_in = 2048;

	Z.next_out = ou_buff;
	Z.avail_out = 2048;

	int status = deflate(&Z, Z_FINISH);
	if (status != Z_STREAM_END) {
		int xxx = 0; 
	}
	deflateEnd(&Z);
	return 0;
}
