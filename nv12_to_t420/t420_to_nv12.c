#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

typedef char pixel;

static void c_copy_frame_t420_to_nv12(pixel *dst[2], int i_dst_stride[2],
			pixel *src[4], int i_src_stride[4], int width, int height)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	pixel *pysrc = NULL;
	pixel *pydst = NULL;
	pysrc = src[0];
	pydst = dst[0];
	/* copy y info */
	for (i = 0; i < height/16; i++) {
		pixel *tpydst = pydst;
		pixel *tpysrc = pysrc;
		for (j = 0; j < width/16; j++) {
			pixel *ttpydst = tpydst;
			for (k = 0; k < 16; k++) {
				memcpy(ttpydst, tpysrc, 16);
				tpysrc += 16;
				ttpydst += i_dst_stride[0];
			}
			tpydst += 16;
		}
		pysrc += i_src_stride[0];
		pydst += i_dst_stride[0]*16;
	}
	/* copy uv info */
	pixel *pusrc = src[1];
	pixel *puvdst = dst[1];
	for (i = 0; i < height/2/8; i++) {
		pixel *tpusrc = pusrc;
		pixel *tpuvdst = puvdst;
		for (j = 0; j < width/2/8; j++) {
			pixel *ttpusrc = tpusrc;
			pixel *ttpuvdst = tpuvdst;
			for (k = 0; k < 8; k++) {
				pixel *p1 = ttpusrc;
				pixel *p2 = ttpusrc + 8;
				pixel *p3 = ttpuvdst;
				for (l = 0; l < 8; l++) {
					*p3++ = *p1++;
					*p3++ = *p2++;
				}
				ttpusrc += 16;
				ttpuvdst += i_dst_stride[1];
			}
			tpusrc += 16*8;
			tpuvdst += 16; 
		}
		pusrc += i_src_stride[1];
		puvdst += i_dst_stride[1]*8;
	}
}

int main(int argc, char *argv[])
{
	if (argc != 5) {
		printf("Usage: %s w h sfilename\n", argv[0]);
		return 0;
	} else {
		int w = atoi(argv[1]);
		int h = atoi(argv[2]);
		char * sfilename = argv[3];
		char * dfilename = argv[4];
		int sfd = -1, dfd = -1;
		char *framebuf = NULL;
		long long int filesize = -1;
		int framesize = w * h * 3 / 2;
		int framecnt = 0;
		int i = 0, j = 0;
		char *yplane = NULL;
		char *uvplane = NULL;
		char *dst[2], *src[4];
		int i_dst_stride[2], i_src_stride[4];

		printf("w = %d, h = %d, sfilename = %s\n", w, h, sfilename);

		if ((framebuf = malloc(w * h * 3 / 2)) == NULL) {
			printf("malloc framebuf failed\n");
			goto err_malloc_framebuf;
		}

		if ((yplane = malloc(w * h)) == NULL) {
			printf("malloc u plane failed\n");
			goto err_malloc_yplane;
		}

		if ((uvplane = malloc(w * h / 2)) == NULL) {
			printf("malloc v plane failed\n");
			goto err_malloc_uvplane;
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
			memset(framebuf, 0, framesize);
			memset(yplane, 0, w * h);
			memset(uvplane, 0, w * h / 2);
			if (framesize != read(sfd, framebuf, framesize)) {
				printf("read %s %d bytes failed\n", sfilename, framesize);
				goto err_read_frame;
			}
			src[0] = framebuf;
			src[1] = framebuf + w * h;
			i_src_stride[0] = 16 * w;
			i_src_stride[1] = 8 * w;
			dst[0] = yplane;
			dst[1] = uvplane;
			i_dst_stride[0] = w;
			i_dst_stride[1] = w;
			c_copy_frame_t420_to_nv12(dst, i_dst_stride, src, i_src_stride, w, h);

			if ((w * h) != write(dfd, dst[0], w * h)) {
				printf("write y plane to %s %d bytes filed\n", dfilename, w * h);
				goto err_write_y_plane;
			}

			if ((w * h / 2) != write(dfd, dst[1], w * h / 2)) {
				printf("write uv plane to %s %d bytes filed\n", dfilename, w * h / 2);
				goto err_write_uv_plane;
			}

		}

		close(dfd);
		close(sfd);

		return 0;

err_write_uv_plane:
err_write_y_plane:
err_read_frame:
		close(dfd);
err_open_dstfile:
err_lseek_sourcefile_begin:
err_lseek_sourcefile_end:
		close(sfd);
err_open_sourcefile:
		free(uvplane);
err_malloc_uvplane:
		free(yplane);
err_malloc_yplane:
		free(framebuf);
err_malloc_framebuf:
		return -1;
	}
}
