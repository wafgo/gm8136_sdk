#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <sys/signal.h>
#include <sys/mman.h>
#include "dma_util/dma_util.h"
#include "frammap/frammap_if.h"

int main(int argc, char *argv[])
{
    alloc_type_t alloc_type = ALLOC_CACHEABLE;
    int dma_fd = -1, fram_fd = -1;
    unsigned char *mmap_src, *mmap_dst;
    int ret = -1, i, j, test_round = 10, test_size = 512 * 1024;
    unsigned char pettern = 0x11;
    unsigned int *ptr1, *ptr2;
    dma_util_t dma_util;

    dma_fd = open("/dev/dma_util", O_WRONLY);
    if (dma_fd < 0) {
        perror("Can't open /dev/dma_util\n");
        return -1;
    }

    fram_fd = open("/dev/frammap0", O_RDWR);
    if (fram_fd < 0) {
        perror("Can't open /dev/frammap0! \n");
        close(dma_fd);
        return -1;
    }

    /*
     * The following case what I test is cache memory. Actually, the memory can be allocated to
     * non-cache.
     */
    /* set memory type */
    if (ioctl(fram_fd, FRM_ALLOC_TYPE, &alloc_type) < 0) {
        perror("configure memory type fail! \n");
        goto exit;
    }

    mmap_src = (unsigned char *)mmap(0, test_size, PROT_READ | PROT_WRITE, MAP_SHARED, fram_fd, 0);
	if (mmap_src == (unsigned char *)(-1)) {
	    printf("Error \n");
	    goto exit;
	}

    mmap_dst = (unsigned char *)mmap(0, test_size, PROT_READ | PROT_WRITE, MAP_SHARED, fram_fd, 0);
	if (mmap_dst == (unsigned char *)(-1)) {
	    printf("Error \n");
	    goto exit;
	}

    memset(&dma_util, 0, sizeof(dma_util_t));
    dma_util.src_vaddr = (void *)mmap_src;
    dma_util.dst_vaddr = (void *)mmap_dst;
    dma_util.length = test_size;

    for (i = 0; i < test_round; i ++) {
        memset(mmap_src, pettern, test_size);
        if (ioctl(dma_fd, DMA_UTIL_DO_DMA, &dma_util)) {
            printf("DMA test error \n");
	        goto exit;
	    }
	    ptr1 = (unsigned int *)mmap_src;
	    ptr2 = (unsigned int *)mmap_dst;
	    for (j = 0; j < test_size / 4; j ++) {
            if (ptr1[j] != ptr2[j]) {
                printf("Data compare error!!!! \n");
	            goto exit;
	        }
	    }
	    pettern += 0x11;
    }

    printf("DMA test ok. \n");
    ret = 0;
exit:
    if (fram_fd >= 0)
        close(fram_fd);

    if (dma_fd >= 0)
        close(dma_fd);

    return ret;
}
