/**
 *@file main.cpp
 *@author  Francis Huang
 *@version 1.0.0
 *@date 2008-2009
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include "lcd_fb.h"

static int fb_test_bit(unsigned int nr, unsigned int *addr)
{
	return ((0x1 << (nr & 0x1F)) & (((unsigned long *)addr)[nr >> 5])) != 0;
}

int main(int argc, char *argv[])
{
	int i, fbfd;
	struct edid_table   edid;
	
	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd  == 0) {
	    printf("LCD Error: cannot open framebuffer device. \n");
	    exit(0);
	}
	if (ioctl(fbfd, FFB_GET_EDID, &edid) < 0) {
	    printf("LCD Error: read EDID table fail! \n");
	    exit(0);
	}
	
	for (i = 0; i < OUTPUT_TYPE_LAST; i ++) {
	    if (fb_test_bit(i, edid.edid))
	        printf("LCD support resolution output_type id: %d \n", i);
	}
	
	if (fb_test_bit(OUTPUT_TYPE_SLI10121_1280x1024_50, edid.edid)) {
	    int output_type = OUTPUT_TYPE_SLI10121_1280x1024_50;
	    
	    if (ioctl(fbfd, FFB_SET_EDID, &output_type) < 0)
	        printf("LCD changes resolution to 1280x1024 fail. \n");
	    else
	        printf("LCD changes resolution to 1280x1024 success. \n");
	}
	
	close(fbfd);

    return 0;
}
