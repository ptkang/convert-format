#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <math.h>
#include <sys/mman.h>
#include <linux/fb.h>
#ifdef HAVE_ANDROID_OS
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include "camera_test_app.h"

#define DUMP_YUV422_FILE  "i420.yuv" 
#define YUV "dump.tileyuv"
int w_size = 1280;
int h_size = 720;
static void yuv422_to_444(u_int8_t *preview_frame, cim_info_t *p)
{
	u_int32_t line, col,i=0,j=0,z=0,a=0,b=0,c=0,d=0,mby_w=0,mbu_w=0,mbv_w=0,mby_h=0,mbu_h=0,mbv_h=0,f=0;
	u_int8_t *yuyv = preview_frame;

	u_int8_t *ybuf_tmp = malloc(w_size*h_size);
	u_int8_t *ubuf_tmp = malloc(w_size*h_size/4);
	u_int8_t *vbuf_tmp = malloc(w_size*h_size/4);
//	printf("p->size_h= %d p->size_w = %d\n",p->size_h,p->size_w);
#if 0
	for (line = 0; line < (4*p->size_h-1); line++) {
		for (col = 0; col < (p->size_w); col += 1) {
			p->ybuf[line * p->size_w + col] = *(yuyv + 0); /* Y0 */
			
			if(col%2 == 0){
				p->ubuf[line * p->size_w + col/2] = *(yuyv + 1); /* U0 */
			}
			else{
				p->vbuf[line * p->size_w + (col-1)/2] = *(yuyv + 1); /* V0 */
			}
			yuyv += 2;
		}
	}
#endif
	for (line = 0 ; line < p->size_h; line++) {
		for (col = 0; col < (p->size_w); col ++) {
			ybuf_tmp[z++] = *(yuyv); /* Y0 */	
			yuyv += 1;
		}
	}
//	printf("1\n");
	z=0;
	mby_w = p->size_w/16;
	mby_h = p->size_h/16;
//	printf("2\n");
	for (a=0;a < mby_h;a++){
		for (b=0;b < 16 ;b++){

			for(c=0;c<mby_w;c++){

				for(d=0;d<16;d++){
					p->ybuf[z++] = *(ybuf_tmp++);
				}
				ybuf_tmp = ybuf_tmp + 16*16 -d;
			}

			ybuf_tmp= ybuf_tmp - mby_w*16*16 + d;
		}
		ybuf_tmp = ybuf_tmp+ 16*16*(mby_w -1);
}
	for (line = 0 ; line < p->size_h/2; line++) {
		for (col = 0; col < (p->size_w); col ++) {

			if((col/8)%2 == 0){
				ubuf_tmp[i++] = *(yuyv); /* U0 */
			}
			else{
				vbuf_tmp[j++] = *(yuyv); /* V0 */
			}
			yuyv+=1;
		}
	}

	i=0;
	mbu_w = p->size_w/16;
	mbu_h = p->size_h/16;
	for (a=0;a < mbu_h;a++){
		for (b=0;b < 8;b++){
			for(c=0;c<mbu_w;c++){

				for(d=0;d<8;d++){
					p->ubuf[i++] = *(ubuf_tmp++);
				}
				ubuf_tmp = ubuf_tmp + 8*8 -d;
			}
			ubuf_tmp= ubuf_tmp - mbu_w*8*8 + d;
		}
		ubuf_tmp = ubuf_tmp +8*8*(mbu_w-1);
}
	j=0;
	mbv_w = p->size_w/16;
	mbv_h = p->size_h/16;
	for (a=0;a < mbv_h;a++){
		for (b=0;b < 8;b++){
			for(c=0;c<mbv_w;c++){

				for(d=0;d<8;d++){
				
					p->vbuf[j++] = *(vbuf_tmp++);
//							printf("dddddj+_+ vbuf_tmp =========%d j = %d\n",vbuf_tmp,j);
				}
				vbuf_tmp = vbuf_tmp + 8*8 -d;
//					printf("c ccccc+_+ vbuf_tmp =========%d j = %d\n",vbuf_tmp,j);
			}
			vbuf_tmp= vbuf_tmp - mbv_w*8*8 + d;
//			printf("bbbbbbbbb+_+ vbuf_tmp =========%d,j=%d f==%d\n",vbuf_tmp,j,f++);
		}
					
		vbuf_tmp = vbuf_tmp+8*8*(mbv_w-1);
//			printf("aaaaaaaa++ ==== vbuf_tmp= %d\n",vbuf_tmp);
//	if (a ==1){
//		printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
//		while(1);

//}

}

//	printf("i =%d,j= %d,z= %d\n",i,j,z);
}


int main(int argc, char **argv) {
	FILE *fp = NULL;
	u_int8_t *buf =  malloc(w_size*h_size*3/2);      
	int debug_fd,length,frame,i,frame_all;
	printf("main start 1\n");
	cim_info_t *p1 = (cim_info_t *)
		malloc(sizeof(cim_info_t));
	memset(p1, 0, sizeof(p1));

	p1->buf_size = w_size*h_size*3/2;

	p1->ybuf = malloc(p1->buf_size * 2/3);
	p1->ubuf = malloc(p1->buf_size/6);
	p1->vbuf = malloc(p1->buf_size/6);
	p1->size_w = w_size;
	p1->size_h = h_size;
	printf("main start 2\n");
 
	if(((fp = fopen(YUV,"rb"))<=0)){
		printf("open error\n");
	}
	frame = w_size*h_size*3/2;
	fseek(fp,0L,SEEK_END);

	length=ftell(fp);
	frame_all = length/frame;

	fseek(fp,0L,SEEK_SET);
	printf("main start 3 length = %d,frame_all = %d\n",length,frame_all);
	for(i = 0; i < frame_all; i++){
		//printf("main start x\n");
		//fread(buf,8,1,fp);
		//printf("main start x\n");
		//while(1);
		if((fread(buf,frame,1,fp))!=1){
			printf("read error i frame = %d\n",i);
			return -1;
		}
		//	if(fseek(fp,frame,SEEK_CUR)){
		//	printf("seek error\n");
		//	return -1;
		//	}
		yuv422_to_444(buf,p1);
	
		if((debug_fd=open(DUMP_YUV422_FILE, O_RDWR | O_CREAT | O_APPEND, 0777)) <= 0) {
			printf("[dump] open file error    !");
			return -1;	
		}
		if (debug_fd > 0){
		
			if(write(debug_fd,p1->ybuf,frame*2/3) != frame*2/3){
				printf("write Ybuf error\n");
				return -1;			  
			}
			if(write(debug_fd,p1->ubuf,frame/6) != frame/6){
				printf("write Ubufs error\n");
				return -1;
			}
			if(write(debug_fd,p1->vbuf,frame/6) != frame/6){
				printf("write Vbuf error\n");
				return -1;
			}
			close(debug_fd);
		}
	}
	fclose(fp);
	printf("exit for\n");
	return 0;
}



