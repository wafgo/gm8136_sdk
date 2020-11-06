#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <ft2dge/ft2dge_lib.h>
#include "../ft2dge.h"
#include <linux/fb.h>

#define f2dge_readl(a)	    (*(volatile unsigned int *)(a))
#define f2dge_writel(v,a)	(*(volatile unsigned int *)(a) = (v))

/* Information maintained by every user process
 */
typedef struct {
    struct {
        unsigned int ofs;
        unsigned int val;
    } cmd[LLST_MAX_CMD];
    int cmd_cnt;    //how many commands are active
    /* TODO semaphore */
} ft2dge_llst_t;

typedef struct {
    int ft2dge_fd;
    int width;
    int height;
    int lcd_fd;
    unsigned int fb_paddr;  /* frame buffer physical address */
    unsigned int fb_vaddr;  /* frame buffer virtual address */
    ft2dge_bpp_t bpp_type;
    ft2dge_llst_t *llst;  //local allocate memory
    void *sharemem; //this memory comes from kernel through mapping
    void *io_mem;   //2dge io register
    int mapped_sz;
    pthread_mutex_t  mutex;
} ft2dge_desc_t;


/* just add a draw line command but not fire yet */
int ft2dge_lib_line_add(void *descriptor, int src_x, int src_y, int dst_x, int dst_y, unsigned int color)
{
    ft2dge_desc_t *desc = (ft2dge_desc_t *)descriptor;
    ft2dge_cmd_t  command;

    command.value = 0;
    command.bits.command = FT2DGE_CMD_LINE_DRAWING;
    command.bits.bpp = desc->bpp_type;
    command.bits.no_source_image = 1;

    //lock semaphore
    pthread_mutex_lock(&desc->mutex);

    ft2dge_lib_set_param(desc, FT2DGE_CMD, command.value);
    /* LCD PIP frame buffer address */
    ft2dge_lib_set_param(desc, FT2DGE_DSTSA, desc->fb_paddr);
    /* source pitch and destination pitch */
    ft2dge_lib_set_param(desc, FT2DGE_SPDP, FT2DGE_PITCH(desc->width, desc->width));
    /* pattern foreground color */
    ft2dge_lib_set_param(desc, FT2DGE_FPGCOLR, color);
    /* Endpoint-A X/Y coordinates for line drawing */
    ft2dge_lib_set_param(desc, FT2DGE_SRCXY, FT2DGE_XY(src_x, src_y));
    /* Endpoint-B X/Y coordinates for line drawing */
    ft2dge_lib_set_param(desc, FT2DGE_DSTXY, FT2DGE_XY(dst_x, dst_y));
    /* forground color */
    //ft2dge_lib_set_param(desc, FT2DGE_SFGCOLR, color);
    /* when all parameters are updated, this is the final step */
    ft2dge_lib_set_param(desc, FT2DGE_TRIGGER, 0x1);

    //unlock semaphore
    pthread_mutex_unlock(&desc->mutex);

    return 0;
}

/* rectangle fill. source is from internal register or display memory
 */
int ft2dge_lib_rectangle_add(void *descriptor, int x, int y, int width, int height, unsigned int color)
{
    ft2dge_desc_t *desc = (ft2dge_desc_t *)descriptor;
    ft2dge_cmd_t  command;

    /* command action */
    command.value = 0;
    command.bits.rop = ROP_SRCCOPY;
    command.bits.command = FT2DGE_CMD_BITBLT;
    command.bits.bpp = desc->bpp_type;
    command.bits.no_source_image = 1;

    //lock semaphore
    pthread_mutex_lock(&desc->mutex);

    ft2dge_lib_set_param(desc, FT2DGE_CMD, command.value);
    /* LCD PIP frame buffer address */
    ft2dge_lib_set_param(desc, FT2DGE_DSTSA, desc->fb_paddr);
    /* source pitch and destination pitch */
    ft2dge_lib_set_param(desc, FT2DGE_SPDP, FT2DGE_PITCH(desc->width, desc->width));
    /* forground color */
    ft2dge_lib_set_param(desc, FT2DGE_SFGCOLR, color);
    /* destination object upper-left X/Y coordinates */
    ft2dge_lib_set_param(desc, FT2DGE_DSTXY, FT2DGE_XY(x, y));
    /* destination object rectanlge width and height */
    ft2dge_lib_set_param(desc, FT2DGE_DSTWH, FT2DGE_WH(width, height));
    /* when all parameters are updated, this is the final step */
    ft2dge_lib_set_param(desc, FT2DGE_TRIGGER, 0x1);

    //unlock semaphore
    pthread_mutex_unlock(&desc->mutex);

    /* Note, all parameters are updated into DRAM only, you should call fire function to active them.
     */
    return 0;
}

/* not yet */
int ft2dge_lib_srccopy_add(void *descriptor, int src_x, int src_y, int dst_x, int dst_y, int width, int height, unsigned int src_paddr)
{
    ft2dge_desc_t *desc = (ft2dge_desc_t *)descriptor;
    ft2dge_cmd_t  command;

    /* command action */
    command.value = 0;
    command.bits.rop = ROP_SRCCOPY;
    command.bits.command = FT2DGE_CMD_BITBLT;
    command.bits.bpp = desc->bpp_type;
    command.bits.no_source_image = 0;

    //lock semaphore
    pthread_mutex_lock(&desc->mutex);

    ft2dge_lib_set_param(desc, FT2DGE_CMD, command.value);
    /* Source pixel data start address */
    ft2dge_lib_set_param(desc, FT2DGE_SRCSA, src_paddr);
    /* LCD PIP frame buffer address */
    ft2dge_lib_set_param(desc, FT2DGE_DSTSA, desc->fb_paddr);
    /* source pitch and destination pitch */
    ft2dge_lib_set_param(desc, FT2DGE_SPDP, FT2DGE_PITCH(desc->width, desc->width));
    /* srouce object start x/y */
    ft2dge_lib_set_param(desc, FT2DGE_SRCXY, FT2DGE_XY(src_x, src_y));
    /* Source Rectangle Width and Height */
    ft2dge_lib_set_param(desc, FT2DGE_SRCWH, FT2DGE_WH(width, height));
    /* Destination object upper-left X/Y coordinates */
	ft2dge_lib_set_param(desc, FT2DGE_DSTXY, FT2DGE_XY(dst_x, dst_y));
	/* Destination rectangle width and height */
	ft2dge_lib_set_param(desc, FT2DGE_DSTWH, FT2DGE_WH(width, height));
	/* when all parameters are updated, this is the final step */
    ft2dge_lib_set_param(desc, FT2DGE_TRIGGER, 0x1);

    //unlock semaphore
    pthread_mutex_unlock(&desc->mutex);

    /* Note, all parameters are updated into DRAM only, you should call fire function to active them.
     */
    return 0;
}

/* fire all added commands
 * b_wait = 0: not wait for complete
 * b_wait = 1:  wait for complete
 * return value: 0 for success, -1 for fail
 */
int ft2dge_lib_fire(void *descriptor, int b_wait)
{
    ft2dge_desc_t *desc = (ft2dge_desc_t *)descriptor;
    ft2dge_llst_t *llst = desc->llst;
    void *io_mem = desc->io_mem;
    struct ft2dge_idx two_idx;
    ft2dge_sharemem_t *sharemem = (ft2dge_sharemem_t *)desc->sharemem;
    unsigned int write_idx, read_idx;
    int i, idx, cmd_count;

    if (!llst->cmd_cnt)
        return 0;

    if (llst->cmd_cnt >= LLST_MAX_CMD) {
        printf("%s, command count: %d is over than %d!\n", __func__, llst->cmd_cnt, LLST_MAX_CMD);
        return -1;
    }

    //lock semaphore
    pthread_mutex_lock(&desc->mutex);

    /* sanity check */
    if (!(sharemem->status & STATUS_DRV_BUSY)) {
        while (FT2DGE_BUF_COUNT(sharemem->write_idx, sharemem->read_idx) != 0) {
            printf(">>>>>>>>>>> BUG1! >>>>>>>>>>>>>>>>>>> \n");
            usleep(1);
        }
    }

    while (FT2DGE_BUF_SPACE(sharemem->write_idx, sharemem->read_idx) < llst->cmd_cnt) {
        if (!(sharemem->status & STATUS_DRV_BUSY)) {
            printf(">>>>>>>>>>> Need to trigger hw! >>>>>>>>>>>>>>>>>>> \n");
            sharemem->status |= STATUS_DRV_BUSY;
            //f2dge_writel(0x1, io_mem + 0xFC);
            /* drain the write buffer */
            //f2dge_readl(io_mem + 0xFC);
        }
        usleep(1);
    }

    /* copy local Q to share memory */
    write_idx = sharemem->write_idx;
    for (i = 0; i < llst->cmd_cnt; i ++) {
        idx = FT2DGE_GET_BUFIDX(write_idx + i);
        sharemem->cmd[idx].ofs = llst->cmd[i].ofs;
        sharemem->cmd[idx].val = llst->cmd[i].val;
    }
    //update the share memory index
    sharemem->write_idx += llst->cmd_cnt;

    /* keep the index for ioctl */
    two_idx.read_idx = sharemem->read_idx;
    two_idx.write_idx = sharemem->write_idx;
    if (two_idx.read_idx == two_idx.write_idx) {
        for (;;) {
            printf("read_idx == write_idx.... \n");
            sleep(1);
        }
    }

    /* fire the hardware? */
    if (!(sharemem->status & STATUS_DRV_BUSY)) {
        sharemem->status |= STATUS_DRV_BUSY;
        /* update to hw directly */
        sharemem->fire_count = llst->cmd_cnt;
        read_idx = sharemem->read_idx;
        sharemem->status |= STATUS_DRV_INTR_FIRE;
        sharemem->usr_fire ++;

        for (i = 0; i < llst->cmd_cnt; i ++) {
            idx = FT2DGE_GET_BUFIDX(read_idx + i);
            f2dge_writel(sharemem->cmd[idx].val, io_mem + sharemem->cmd[idx].ofs);
        }

        /* fire the hardware */
        f2dge_writel(0x1, io_mem + 0xFC);
        /* drain the write buffer */
        f2dge_readl(io_mem + 0xFC);
    }
    llst->cmd_cnt = 0;  //reset

    //unlock semaphore
    pthread_mutex_unlock(&desc->mutex);

    if (b_wait)
        ioctl(desc->ft2dge_fd, FT2DGE_CMD_SYNC, &two_idx);

    return 0;
}

/*
 * @This function is used to open a ft2dge channel for drawing.
 *
 * @function void *ft2dge_open(void);
 * @Parmameter reb_type indicates which RGB is used in this framebuffer
 * @return non zero means success, null for fail
 */
void *ft2dge_lib_open(ft2dge_bpp_t rgb_type)
{
    int byte_per_pixel, mapped_sz;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo fix;
    ft2dge_desc_t *desc;

    desc = malloc(sizeof(ft2dge_desc_t));
    if (!desc) {
        printf("%s, allocate memory fail! \n", __func__);
        return NULL;
    }

    memset(desc, 0, sizeof(ft2dge_desc_t));
    desc->lcd_fd = open("/dev/fb1", O_RDWR);
    if (desc->lcd_fd < 0) {
        printf("Error to open LCD fd! \n");
        goto err_exit;
    }

    if (ioctl(desc->lcd_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        printf("Error to call FBIOGET_VSCREENINFO! \n");
        goto err_exit;
    }

    if (ioctl(desc->lcd_fd, FBIOGET_FSCREENINFO, &fix) < 0) {
        printf("Error to call FBIOGET_FSCREENINFO! \n");
        goto err_exit;
    }
    desc->fb_vaddr = (unsigned int)mmap(NULL, vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8),
                                            PROT_READ | PROT_WRITE, MAP_SHARED, desc->lcd_fd, 0);
    if ((int)desc->fb_vaddr < 0) {
        printf("Error to mmap lcd fb! \n");
        goto err_exit;
    }

    printf("2D Engine GUI: xres = %d, yres = %d, bits_per_pixel = %d, size = %d \n",
            vinfo.xres, vinfo.yres, vinfo.bits_per_pixel,
            vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8));

    desc->ft2dge_fd = open("/dev/ft2dge", O_RDWR);
    if (desc->ft2dge_fd < 0) {
        printf("Error to open f2dge fd! \n");
        goto err_exit;
    }

    desc->width = vinfo.xres;
    desc->height = vinfo.yres;
    switch (rgb_type) {
      case FT2DGE_RGB_565:
      case FT2DGE_ARGB_1555:
        byte_per_pixel = 2;
        break;
      case FT2DGE_RGB_888:
      case FT2DGE_ARGB_8888:
        byte_per_pixel = 4;
        break;
      default:
        printf("Error RGB type: %d \n", rgb_type);
        goto err_exit;
        break;
    }
    if (byte_per_pixel != (vinfo.bits_per_pixel / 8)) {
        printf("LCD byte_per_pixel = %d, but users pass byte_per_pixel: %d \n",
                vinfo.bits_per_pixel, byte_per_pixel);
        goto err_exit;
    }
    desc->fb_paddr = (unsigned int)fix.smem_start;
    desc->bpp_type = rgb_type;

    desc->llst = (ft2dge_llst_t *)malloc(sizeof(ft2dge_llst_t));
    if (!desc->llst) {
        printf("%s, allocate llst fail! \n", __func__);
        goto err_exit;
    }
    memset((void *)desc->llst, 0, sizeof(ft2dge_llst_t));

    /* mapping the share memory and register I/O */
    mapped_sz = ((sizeof(ft2dge_sharemem_t) + 4095) >> 12) << 12;   //page alignment
    desc->mapped_sz = mapped_sz;
    desc->sharemem = (void *)mmap(NULL, desc->mapped_sz, PROT_READ | PROT_WRITE, MAP_SHARED, desc->ft2dge_fd, 0);
    if (!desc->sharemem) {
        printf("%s, mapping share memory fail! \n", __func__);
        goto err_exit;
    }

    /* register I/O */
    desc->io_mem = (void *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, desc->ft2dge_fd, desc->mapped_sz);
    if (!desc->io_mem) {
        printf("%s, mapping io_mem fail! \n", __func__);
        goto err_exit;
    }

    /* create semphore */
    pthread_mutex_init(&desc->mutex, NULL);

    return desc;

err_exit:
    if (desc->lcd_fd >= 0)
        close(desc->lcd_fd);

    if (desc && desc->sharemem)
        munmap(desc->sharemem, desc->mapped_sz);
    if (desc && desc->llst)
        free(desc->llst);
    if (desc)
        free(desc);

    return NULL;
}

/*
 * @This function is used to close a ft2dge channel.
 *
 * @function void ft2dge_close(void *desc);
 * @Parmameter desc indicates the descritor given by open stage
 * @return none
 */
void ft2dge_lib_close(void *descriptor)
{
    int byte_per_pixel;
    ft2dge_desc_t *desc = (ft2dge_desc_t *)descriptor;

    /* release semphore */
    pthread_mutex_destroy(&desc->mutex);

    switch (desc->bpp_type) {
      default:
      case FT2DGE_RGB_565:
      case FT2DGE_ARGB_1555:
        byte_per_pixel = 2;
        break;
      case FT2DGE_RGB_888:
      case FT2DGE_ARGB_8888:
        byte_per_pixel = 4;
        break;
    }

    /* close 2denge fd */
    close(desc->ft2dge_fd);
    close(desc->lcd_fd);
    free(desc->llst);
    munmap((void *)desc->fb_vaddr, desc->width * desc->height * byte_per_pixel);
    munmap(desc->sharemem, desc->mapped_sz);
    munmap(desc->io_mem, 4096);
    free(desc);
}

/* When this function is called, the semaphore should be called to protect the database */
void ft2dge_lib_set_param(void *descriptor, unsigned int ofs, unsigned int val)
{
    ft2dge_llst_t *llst = ((ft2dge_desc_t *)descriptor)->llst;

    llst->cmd[llst->cmd_cnt].ofs = ofs;
    llst->cmd[llst->cmd_cnt].val = val;
    llst->cmd_cnt ++;
}

/* get frame buffer virtual address */
unsigned int ft2dge_lib_get_fbvaddr(void *descriptor)
{
    return ((ft2dge_desc_t *)descriptor)->fb_vaddr;
}

unsigned int ft2dge_lib_get_paddr(void *descriptor, unsigned int vaddr)
{
    ft2dge_desc_t *desc = (ft2dge_desc_t *)descriptor;
    unsigned int paddr = vaddr;

    if (ioctl(desc->ft2dge_fd, FT2DGE_CMD_VA2PA, &paddr) < 0)
        return 0xFFFFFFFF;

    return paddr;
}

#if 0
void main(void)
{
    void *desc;
    int i, j;

    desc = ft2dge_lib_open(FT2DGE_RGB_565);

    ft2dge_lib_rectangle_add(desc, 0, 0, 1920, 1080, 0x0);
    ft2dge_lib_fire(desc, 1);

    printf("Start to test >>>>>>>>>> \n");
    for (i = 0; i < 100; i ++) {
        ft2dge_lib_rectangle_add(desc, 0 + i*10, 0 + i *10, 10, 10, 0xF800);
        ft2dge_lib_fire(desc, 1);
    }

    printf("Test complete. >>>>>>>>>> \n");

    return;
}
#endif
