#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zlib.h"

#define IN_BUFF_SIZE 8
#define OUT_BUFF_SIZE 64

void test()
{
	z_stream *Z=calloc(1, sizeof(z_stream));
	unsigned char in_buff[IN_BUFF_SIZE];
	unsigned char out_buff[OUT_BUFF_SIZE];
	FILE *fin, *fout;
	int status;

	if (Z==NULL) {
		printf("can't alloc Z\r\n");
		return;
	}
	inflateInit2(Z, -MAX_WBITS);
	fin=fopen("nesla-current.tar.gz", "rb");
	fout=fopen("nesla-current.tar", "wb");
	fread(out_buff, 1, 10, fin);
	while (!feof(fin)) {
		const size_t read=fread(in_buff, 1, IN_BUFF_SIZE, fin);
		if (ferror(fin)) break;
		Z->next_in=in_buff;
		Z->avail_in=read;
		do {
			Z->next_out=out_buff;
			Z->avail_out=OUT_BUFF_SIZE;
			status=inflate(Z, Z_NO_FLUSH);
			switch (status) {
			case Z_OK:
				printf("Z_OK\r\n");
				fwrite(out_buff, 1, sizeof(out_buff)-Z->avail_out, fout);
				break;
			case Z_STREAM_END:
				printf("Z_STREAM_END\r\n");
				fwrite(out_buff, 1, sizeof(out_buff)-Z->avail_out, fout);
				goto end;
			case Z_BUF_ERROR:
				printf("Z_BUF_ERROR\r\n");
				break;
			case Z_DATA_ERROR:
				printf("Z_DATA_ERROR\r\n");
				break;
			default:
				printf("some other error %d\r\n", status);
				break;
			}
		} while (Z->avail_out==0);
	}
end:
	inflateEnd(Z);
	free(Z);
	fclose(fin);
	fclose(fout);
}

int main(int argc, char **argv)
{
	test();
	return 0;
}
