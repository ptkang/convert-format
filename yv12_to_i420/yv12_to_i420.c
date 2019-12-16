#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

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
		char *framebuf = NULL;
		long long int filesize = -1;
		int framesize = w * h * 3 / 2;
		int framecnt = 0;
		int i = 0;

		printf("w = %d, h = %d, sfilename = %s, dfilename = %s\n", w, h, sfilename, dfilename);

		if ((framebuf = malloc(w * h * 3 / 2)) == NULL) {
			printf("malloc framebuf failed\n");
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
			char *uplane = framebuf + w * h * 5 / 4;
			char *vplane = framebuf + w * h;
			memset(framebuf, 0, framesize);
			if (framesize != read(sfd, framebuf, framesize)) {
				printf("read %s %d bytes failed\n", sfilename, framesize);
				goto err_read_frame;
			}

			if ((w * h) != write(dfd, framebuf, w * h)) {
				printf("write y plane to %s %d bytes filed\n", dfilename, w * h);
				goto err_write_y_plane;
			}

			if ((w * h / 4) != write(dfd, uplane, w * h / 4)) {
				printf("write u plane to %s %d bytes filed\n", dfilename, w * h / 4);
				goto err_write_u_plane;
			}

			if ((w * h / 4) != write(dfd, vplane, w * h / 4)) {
				printf("write v plane to %s %d bytes filed\n", dfilename, w * h / 4);
				goto err_write_v_plane;
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
		free(framebuf);
err_malloc_framebuf:
		return -1;
	}
}
