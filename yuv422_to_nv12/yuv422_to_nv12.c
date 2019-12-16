#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int yuv422_to_nv12(unsigned char *input_buf, int w, int h, unsigned char *output_buf)
{
	int i = 0, j = 0;

	for(j=0; j<h; j++) {
		for(i=0; i<w; i++) {
			*(output_buf + w*j + i) = *(input_buf + 2*(w*j + i));
			if(j%2 == 0)
				*(output_buf + (w*h) + (w*j/2 + i)) = *(input_buf + 2*(w*j + i) +1);
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 5) {
		printf("Usage:%s w h sfilename dfilename\n", argv[0]);
		return 0;
	} else {
		int w = atoi(argv[1]);
		int h = atoi(argv[2]);
		char * sfilename = argv[3];
		char * dfilename = argv[4];
		int sfd = -1, dfd = -1;
		char *i_framebuf = NULL;
		char *o_framebuf = NULL;
		long long int filesize = -1;
		int framesize = w * h * 2;
		int framecnt = 0;
		int i = 0;

		printf("w = %d, h = %d, sfilename = %s, dfilename = %s\n", w, h, sfilename, dfilename);

		if ((i_framebuf = malloc(w * h * 2)) == NULL) {
			printf("malloc input framebuf failed\n");
			goto err_malloc_framebuf;
		}

		if ((o_framebuf = malloc(w * h * 3 / 2)) == NULL) {
			printf("malloc output framebuf failed\n");
			goto err_malloc_framebuf;
		}

		if ((sfd = open(sfilename, O_RDONLY)) < 0) {
			printf("open %s faied\n", sfilename);
			goto err_open_sourcefile;
		}

		if ((filesize = lseek(sfd, 0, SEEK_END)) < 0) {
			printf("lseek %s failed\n", sfilename);
			goto err_lseek_sourcefile_end;
		}

		if (lseek(sfd, 0, SEEK_SET) < 0) {
			printf("lseek %s failed\n", sfilename);
			goto err_lseek_sourcefile_begin;
		}

		framecnt = filesize / framesize;

		if ((dfd = open(dfilename, O_WRONLY | O_CREAT | O_TRUNC, 0777)) < 0) {
			printf("open %s faied\n", dfilename);
			goto err_open_dstfile;
		}

		for (i = 0; i < framecnt; i++) {
			memset(i_framebuf, 0, framesize);
			memset(o_framebuf, 0, w*h*3/2);
			if (framesize != read(sfd, i_framebuf, framesize)) {
				printf("read %s %d bytes failed\n", sfilename, framesize);
				goto err_read_frame;
			}

			yuv422_to_nv12(i_framebuf, w, h, o_framebuf);

			if ((w * h * 3 / 2) != write(dfd, o_framebuf, w * h * 3 / 2)) {
				printf("write y plane to %s %d bytes filed\n", dfilename, w * h);
				goto err_write_y_plane;
			}
		}

		close(dfd);
		close(sfd);

		return 0;

err_write_v_plane:
err_write_u_plane:
err_write_y_plane:
err_read_frame:
		close(dfd);
err_open_dstfile:
err_lseek_sourcefile_begin:
err_lseek_sourcefile_end:
		close(sfd);
err_open_sourcefile:
		free(i_framebuf);
		free(o_framebuf);
err_malloc_framebuf:
		return -1;
	}
}
