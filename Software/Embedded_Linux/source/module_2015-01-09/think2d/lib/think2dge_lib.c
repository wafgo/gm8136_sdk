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
#include <think2d_lib.h>
#include <frammap/frammap_if.h>
#include <linux/fb.h>

/*Definition*/
/// Reads a device register.
#define T2D_GETREG(sdrv, reg) (*(volatile unsigned int *)((unsigned long)sdrv->io_mem + reg))

/// Writes a device register.
#define T2D_SETREG(sdrv, reg, val) do { *(volatile unsigned int  *)((unsigned long)sdrv->io_mem + reg) = (val); } while (0)
//#define T2D_SETREG(sdrv, reg, val) do { *(volatile u32 *)((unsigned long)sdrv->io_mem + reg) = (val); printf("%%%%%%%%%%%%%% '%02lx'=0x%08x\n", reg, val); } while (0)
/// Packs a pair of 16-bit coordinates in a 32-bit word as used by hardware.
#define T2D_XYTOREG(x, y) ((y) << 16 | ((x) & 0xffff))

#define T2D_CHECK_CMD_IS_ENOUGH(m ,n) (((m + n) > T2D_CWORDS) ? 0 : 1) 

#define COLOUR5551(r, g, b, a)  (((r) & 0x000000f8) << 8 | ((g) & 0x000000f8) << 3 | ((b) & 0x000000f8) >> 2 | ((a) & 0x00000080) >> 7)
#define COLOUR8888(r, g, b, a)  (((r) & 0x000000ff) << 24 | ((g) & 0x000000ff) << 16 | ((b) & 0x000000ff) << 8 | ((a) & 0x000000ff))


//#define DEBUG_PROCENTRY

#ifdef DEBUG_PROCENTRY
#define DEBUG_PROC_ENTRY	do { printf("* %s\n", __FUNCTION__); } while (0)
#else
#define DEBUG_PROC_ENTRY
#endif

#define WORD_SIZE_NUMBER   (sizeof(unsigned int) * 8)


/**
 * 
 */
void Think2D_EngineReset( think2dge_desc_t *tdrv )
{
    DEBUG_PROC_ENTRY;

    int timeout = 5;

    do
    {
        /* try to reset the device */
	T2D_SETREG(tdrv, T2D_STATUS, (1 << 31));
    }
    while (--timeout && T2D_GETREG(tdrv, T2D_STATUS));

    if (!timeout)
        /* print an error message if unsuccessful */
	printf("Think2D: Failed to reset the device!\n");
    else
        /* invalidate DirectFB's saved state if successful; this is because all device registers were wiped out */
        printf("Think2D: success to reset the device!\n");
}

/**
 * think2dge_desc_t *tdrv to get shared commands pool
 * b_wait : to wait for command list to execute
 */
int Think2D_EmitCommands(think2dge_desc_t *tdrv ,int b_wait)
{
    int i = 0;
    unsigned int ** p_buffer;
    struct think2d_shared *sharemem = (struct think2d_shared *)tdrv->sharemem;
	
    DEBUG_PROC_ENTRY;

		
    if (sharemem->hw_running || T2D_GETREG(tdrv,T2D_STATUS) )
    {
        printf("%s list hw=%x state=%x\n",__FUNCTION__,sharemem->hw_running,T2D_GETREG(tdrv,T2D_STATUS));
	return THINK2D_FAIL;
    }

	//lock semaphore
    pthread_mutex_lock(&tdrv->mutex);

    sharemem->sw_now = &sharemem->cbuffer[sharemem->sw_buffer][0];

    p_buffer = &sharemem->sw_now;
    /*move cmd buffer to driver shared memory*/
    for(i = 0 ; i < tdrv->llst->cmd_cnt ; i++)
    {
        (*p_buffer)[i] = tdrv->llst->cmd[i];	
    }

    sharemem->sw_preparing = tdrv->llst->cmd_cnt;
    sharemem->sw_ready += sharemem->sw_preparing;

    /*Emit command*/
	
    if (!sharemem->hw_running && !T2D_GETREG(tdrv, T2D_STATUS))
    {
        //printf("%s emit command NUMBER=%d" ,__FUNCTION__ , sharemem->sw_ready);
	ioctl(tdrv->fd, T2D_START_BLOCK, 0);
	/*clear cmd count*/
	tdrv->llst->cmd_cnt = 0;
    }
    else
        printf("hw is running %2x %2x\n",sharemem->hw_running,T2D_GETREG(tdrv, T2D_STATUS));
	
    //unlock semaphore
    pthread_mutex_unlock(&tdrv->mutex);

    if(b_wait){
        while (T2D_GETREG(tdrv, T2D_STATUS) && ioctl(tdrv->fd, T2D_WAIT_IDLE) < 0)
	{
	    printf("emit command is still busy\n");
	    break;
	}

    }
    return THINK2D_SUCCESS;
}
/**
 * draws a line.
 *
 * @param drv Driver data
 * @param line Line region
 */
int Think2D_DrawLine( think2dge_desc_t *tdrv,think2d_region_t  *line)
{
    DEBUG_PROC_ENTRY;

    //printf("DrawLine (%d, %d)-(%d, %d)\n", line->x1, line->y1, line->x2, line->y2);

    //stat_lines++;
    if(tdrv->llst->cmd_cnt + 6 > T2D_CWORDS)
        Think2D_EmitCommands((think2dge_desc_t*)tdrv, 1);
    //lock semaphore
    pthread_mutex_lock(&tdrv->mutex);

    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_STARTXY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(line->x1, line->y1);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_ENDXY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(line->x2, line->y2);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = 0x01;

    //unlock semaphore
    pthread_mutex_unlock(&tdrv->mutex);

    return THINK2D_SUCCESS;
}
/**
 * draws a rectangle.
 *
 * @param drv Driver data
 * @param rect rectangle region
 */

int Think2D_DrawRectangle( think2dge_desc_t *tdrv , think2d_retangle_t *rect  )
{
    DEBUG_PROC_ENTRY;

    int x1 = rect->x;
    int y1 = rect->y;
    int x2 = rect->x + rect->w - 1;
    int y2 = rect->y + rect->h - 1;


    if(tdrv->llst->cmd_cnt + 24 > T2D_CWORDS)
        Think2D_EmitCommands((think2dge_desc_t*)tdrv, 1);

    //lock semaphore
    pthread_mutex_lock(&tdrv->mutex);

    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_STARTXY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(x1, y1);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_ENDXY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(x1, y2);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = 0x01;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_STARTXY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(x1, y2);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_ENDXY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(x2, y2);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = 0x01;         
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_STARTXY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(x1, y1);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_ENDXY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(x2, y1);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = 0x01;         
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_STARTXY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(x2, y1);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_ENDXY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(x2, y2);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = 0x01;         

    //unlock semaphore
    pthread_mutex_unlock(&tdrv->mutex);

    return THINK2D_SUCCESS;
}

/**
 * @param think2dge_desc_t *tdrv
 * @param rect for target
 */
int Think2D_FillRectangle(think2dge_desc_t *tdrv , think2d_retangle_t *rect )
{

    DEBUG_PROC_ENTRY;
	//printf("FillRectangle (%d, %d), %dx%d\n", rect->x, rect->y, rect->w, rect->h);

    if(tdrv->llst->cmd_cnt + 6 > T2D_CWORDS)
        Think2D_EmitCommands((think2dge_desc_t*)tdrv, 1);

	//lock semaphore
    pthread_mutex_lock(&tdrv->mutex);

    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_STARTXY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(rect->x, rect->y);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_ENDXY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(rect->x + rect->w - 1, rect->y + rect->h - 1);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = 0x02;

    //unlock semaphore
    pthread_mutex_unlock(&tdrv->mutex);

    return THINK2D_SUCCESS;
} 
/**
 * Sets destination surface registers.
 *
 * @param think2dge_desc_t *tdrv
 * @param struct t2d_target_surface_t *tdata
 */
int Think2d_Target_Set_Mode( think2dge_desc_t *tdrv,struct t2d_target_surface_t *tdata )
{
    unsigned int i,n ,j;
    DEBUG_PROC_ENTRY;
	 
    for(i = 0 , n = 0 ; i < WORD_SIZE_NUMBER ; i++) {
        if ((tdata->command & (1 << i)))
            n++;
    }
	
    n*=2; /*ONE COMMAND USE TWO ARRAY entry*/
	
    if(!n || !T2D_CHECK_CMD_IS_ENOUGH(tdrv->llst->cmd_cnt , n))
    {
        printf("T2D_CHECK_CMD_IS_ENOUGH %s" , __FUNCTION__);
	    return THINK2D_FAIL;
    }
	
	//lock semaphore
    pthread_mutex_lock(&tdrv->mutex);
	 
    for(i = 0 , n = 0 ; i < WORD_SIZE_NUMBER ; i++) {
        
        n = (tdata->command & (1 << i));

        if(!n)
            continue;
        j++;
		 
        switch(n) {
            case TARGET_MODE_SET:
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_TARGET_MODE;
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = tdata->target_mode;
		        break;
            case TARGET_BLEND_SET:
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_TARGET_BLEND;
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = tdata->target_blend;
                break;
            case TARGET_BADDR_SET:
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_TARGET_BAR;
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = tdata->target_baddr;
                break;
            case TARGET_STRIDE_SET:
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_TARGET_STRIDE;
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = tdata->target_pitch;
                break;
            case TARGET_RESOLXY_SET:
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_TARGET_RESOLXY;
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = tdata->target_res_xy;
                break;
            case TARGET_DST_COLORKEY_SET:
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DST_COLORKEY;
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = tdata->target_dstcolor_key;
                break;
            case TARGET_CLIP_MIN_SET:
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_CLIPMIN;
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = tdata->target_clip_min;
                break;
            case TARGET_CLIP_MAX_SET:
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_CLIPMAX;
                tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = tdata->target_clip_max;
                break;
            default:
                printf("%s there is no supported type %2x",__FUNCTION__ , n);
                break;
        }		 
    }
	
    //unlock semaphore
    pthread_mutex_unlock(&tdrv->mutex);
    return THINK2D_SUCCESS;
}

/*
 * @This function is used to open a think2d channel for drawing.
 *
 * @function think2dge_desc_t *think2dge_lib_open
 * @Parmameter reb_type indicates which RGB is used in this framebuffer
 * @return non zero means success, null for fail
 */
think2dge_desc_t *Think2dge_Lib_Open(THINK2D_MODE_T rgb_type)
{
    int byte_per_pixel, mapped_sz,iomem_sz;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo fix;
    think2dge_desc_t *desc;

    desc = malloc(sizeof(think2dge_desc_t));
    if (!desc) {
        printf("%s, allocate memory fail! \n", __func__);
        return NULL;
    }

    memset(desc, 0, sizeof(think2dge_desc_t));
    
    desc->lcd_fd = open("/dev/fb1", O_RDWR);
    if (desc->lcd_fd < 0) {
        printf("Error to open LCD fd! \n");
        return NULL;
    }

    desc->fd = open("/dev/think2d", O_RDWR);
    if (desc->fd < 0) {
        printf("Error to open THINK2D fd! \n");
        close(desc->lcd_fd);
        return NULL;
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
                                        PROT_READ | PROT_WRITE, MAP_SHARED,desc->lcd_fd, 0);
    //printf("desc->fb_vaddr =%x\n",desc->fb_vaddr);

    printf("thin2D Engine GUI: xres = %d, yres = %d, bits_per_pixel = %d, size = %d \n",
            vinfo.xres, vinfo.yres, vinfo.bits_per_pixel,
            vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8));
    
    desc->fb_paddr = (unsigned int)fix.smem_start;
    //printf("desc->fb_paddr =%x\n",desc->fb_paddr);

    if ((int)desc->fb_vaddr < 0) {
        printf("Error to mmap lcd fb! \n");
        goto err_exit;
    }   

    desc->width = vinfo.xres;
    desc->height = vinfo.yres;
    
    byte_per_pixel = T2D_modesize(rgb_type);
    
    if (byte_per_pixel != (vinfo.bits_per_pixel / 8)) {
        printf("LCD byte_per_pixel = %d, but users pass byte_per_pixel: %d \n",vinfo.bits_per_pixel, byte_per_pixel);
        goto err_exit;
    }
       
    desc->bpp_type = rgb_type;

    desc->llst = (think2dge_llst_t *)malloc(sizeof(think2dge_llst_t));
    if (!desc->llst) {
        printf("%s, allocate llst fail! \n", __func__);
        goto err_exit;
    }
    memset((void *)desc->llst, 0, sizeof(think2dge_llst_t));

    /* mapping the share memory and register I/O */
    mapped_sz = ((sizeof(struct think2d_shared) + 4095) >> 12) << 12;   //page alignment
    iomem_sz  = ((T2D_REGFILE_SIZE + 4095) >> 12) << 12;
	
    desc->mapped_sz = mapped_sz;
    desc->iomem_sz = iomem_sz;
	
    desc->sharemem = (void *)mmap(NULL, desc->mapped_sz + iomem_sz, PROT_READ | PROT_WRITE, MAP_SHARED, desc->fd, 0);

    if (!desc->sharemem) {
        printf("%s, mapping share memory fail! \n", __func__);
        goto err_exit;
    }
	
    /* the base address of the register file is a known offset into the area the kernel mapped for us... */
    desc->io_mem = (void*)((unsigned int)desc->sharemem + mapped_sz);

    if (!desc->io_mem) {
        printf("%s, mapping io_mem fail! \n", __func__);
        goto err_exit;
    }

	
    /* confirm that we have a Think2D accelerator in place */
    if (T2D_GETREG(desc, T2D_IDREG) != T2D_MAGIC)
    {
        printf("Think2D: Accelerator not found!\n");
        goto err_exit;
    }
    /* create semphore */
    pthread_mutex_init(&desc->mutex, NULL);

    return desc;

err_exit:
    if (desc->lcd_fd >= 0)
        close(desc->lcd_fd);
    if (desc->fd >= 0)
        close(desc->fd);
    if (desc && desc->sharemem)
        munmap(desc->sharemem, desc->mapped_sz+desc->iomem_sz);
    if (desc && desc->llst)
        free(desc->llst);
    if (desc)
        free(desc);
    return NULL;
}

/*
 * @This function is used to close a think2d channel.
 *
 * @void think2dge_lib_close(void *descriptor));
 * @Parmameter desc indicates the descritor given by open stage
 * @return none
 */
void Think2dge_Lib_Close(void *descriptor)
{
    int byte_per_pixel;
    think2dge_desc_t *desc = (think2dge_desc_t *)descriptor;

    /* release semphore */
    pthread_mutex_destroy(&desc->mutex);

    byte_per_pixel = T2D_modesize(desc->bpp_type);

    /* close 2denge fd */
	close(desc->lcd_fd);
    close(desc->fd);
    free(desc->llst);
    munmap((void *)desc->fb_vaddr, desc->width * desc->height * byte_per_pixel);
    munmap(desc->sharemem, desc->mapped_sz + desc->iomem_sz);
    free(desc);
}

/**
 * Sets the color for drawing operations.
 *
 * @param tdrv Driver data
 * @param color
 */
int Think2d_Set_Drawing_Color( think2dge_desc_t *tdrv,think2d_color_t* color)
{
    DEBUG_PROC_ENTRY;

    if(tdrv->llst->cmd_cnt + 2 > T2D_CWORDS)
        Think2D_EmitCommands((think2dge_desc_t*)tdrv, 1);

    //lock semaphore
    pthread_mutex_lock(&tdrv->mutex);

    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DRAW_COLOR;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] =  COLOUR8888(color->r,color->g,color->b,color->a);

    pthread_mutex_unlock(&tdrv->mutex);
    return THINK2D_SUCCESS;

}


/**
 * Sets destination surface registers.
 * This function will set TARGET_MODE_SET,TARGET_BADDR_SET,TARGET_STRIDE_SET,TARGET_RESOLXY_SET
 */
int Think2d_Set_Dest_Surface( think2dge_desc_t *tdrv)
{
    struct t2d_target_surface_t tdata;

    DEBUG_PROC_ENTRY;

    memset(&tdata,0x0,sizeof(struct t2d_target_surface_t));

    tdata.command =	TARGET_MODE_SET|TARGET_BADDR_SET|TARGET_STRIDE_SET|TARGET_RESOLXY_SET;
    tdata.target_baddr = tdrv->fb_paddr;
    tdata.target_pitch = ((tdrv->width * T2D_modesize(tdrv->bpp_type)) & 0xffff);
    tdata.target_res_xy = T2D_XYTOREG(tdrv->width,tdrv->height);
    tdata.target_mode = (1 << 31) | (0 << 29) | tdrv->bpp_type;
    //tdata.target_mode = (T2D_modesize(tdrv->bpp_type) == 4 ? 0 : ((1 << 31) | (1 << 23))) | (0 << 29) | tdrv->bpp_type; // IS:bit 29 disables 16->32 packing , bits 31/23 set on 32-bit mode

    if(Think2d_Target_Set_Mode(tdrv , &tdata) < 0)
    {   
    	printf("Think2d_Target_Set_Mode fail\n");
        return THINK2D_FAIL;
    }
    return THINK2D_SUCCESS;
}

/**
 * Think2d_Set_Clip_Window.
 * @param tdrv Driver data
 * @param rect clip window
 */
int Think2d_Set_Clip_Window( think2dge_desc_t *tdrv,think2d_retangle_t *rect)
{
    struct t2d_target_surface_t tdata;

    DEBUG_PROC_ENTRY;

    memset(&tdata,0x0,sizeof(struct t2d_target_surface_t));

    tdata.command =	TARGET_CLIP_MIN_SET|TARGET_CLIP_MAX_SET;	
    tdata.target_clip_min = T2D_XYTOREG(rect->x, rect->y);
    tdata.target_clip_max = T2D_XYTOREG(rect->w, rect->h);

    if(Think2d_Target_Set_Mode(tdrv , &tdata) < 0)
    {   
    	printf("Think2d_Target_Set_Mode fail\n");
        return THINK2D_FAIL;
    }
    return THINK2D_SUCCESS;
}

/**
 * Sets Drawing_Blend for line , rectangle.
 * @param tdrv Driver data
 */
int Think2d_Set_Drawing_Blend(think2dge_desc_t *tdrv)
{
    int dst_colorkey = 0;
    struct t2d_target_surface_t tdata;

    DEBUG_PROC_ENTRY;

    memset(&tdata,0x0,sizeof(struct t2d_target_surface_t));

    if (tdrv->drawing_flags & THINK2D_DRAW_DST_COLORKEY)
        dst_colorkey = 1;

    tdata.command =	TARGET_BLEND_SET;

    if (tdrv->drawing_flags & THINK2D_DRAW_DSBLEND)
    	tdata.target_blend = (dst_colorkey << 31) | (tdrv->v_dstBlend << 16) | (tdrv->v_srcBlend);
    else
    	tdata.target_blend = (dst_colorkey << 31) | THINK2D_SRCCOPY_MODE;

    if(Think2d_Target_Set_Mode(tdrv , &tdata) < 0)
    {   
    	printf("Think2d_Target_Set_Mode fail\n");
        return THINK2D_FAIL;
    }

    return THINK2D_SUCCESS;	
}

/**
 * Think2d_Set_Blit_Blend
 * 
 * @param tdrv Driver data
 */
int Think2d_Set_Blit_Blend( think2dge_desc_t *tdrv)
{
    int dst_colorkey = 0;
    int colorize = 0;
    struct t2d_target_surface_t tdata;

    DEBUG_PROC_ENTRY;

    memset(&tdata,0x0,sizeof(struct t2d_target_surface_t));

    /* will we do DST_COLORKEY? */
    if (tdrv->src_blt.blt_src_flags & THINK2D_BLIT_DST_COLORKEY)
    	dst_colorkey = 1;
    /* will we be using the FG_COLOR register for COLORIZE _or_ SRC_PREMULTIPLY? */
    if (tdrv->src_blt.blt_src_flags & THINK2D_BLIT_COLORIZE)
    	colorize = 1;


    tdata.command = TARGET_BLEND_SET;

    if (tdrv->src_blt.blt_src_flags & (THINK2D_BLIT_BLEND_COLORALPHA | THINK2D_BLIT_BLEND_ALPHACHANNEL))
        tdata.target_blend = (dst_colorkey << 31) | (tdrv->v_dstBlend << 16) | (tdrv->v_srcBlend);
    else
        tdata.target_blend = (dst_colorkey << 31) | (colorize << 30) | THINK2D_SRCCOPY_MODE;
        
    if(Think2d_Target_Set_Mode(tdrv , &tdata) < 0)
    {   
        printf("Think2d_Target_Set_Mode fail\n");
        return THINK2D_FAIL;
    }
    return THINK2D_SUCCESS;
}

/**
 * t2d_open_src_surface
 *
 * @param tdrv Driver data
 * @param file_name open file name for image
 */
int Think2d_Open_Src_Surface(think2dge_desc_t *tdrv , char * file_name)
{
    FILE * filer;
    unsigned int tmp_src_add;
    DEBUG_PROC_ENTRY;

    if((filer=fopen(file_name,"rb"))==NULL){
    	printf("open read file error.\n");		
    	return -1;
    }
    /*read RGB565 HEADER weigh x heigh ,the bin file must has 8 bytes header*/
    /*to describe weigh x heigh ,please use pic2raw utility to translate*/
    fread((void*)&tdrv->src_blt.blt_src_width,1,4,filer);	
    fread((void*)&tdrv->src_blt.blt_src_height,1,4,filer);	


    tdrv->src_blt.blt_src_fd = open("/dev/frammap0", O_RDWR);
    if (tdrv->src_blt.blt_src_fd < 0) {
        printf("Error to open framap_fd fd! \n");
    	fclose(filer);
        return -1;
    }

    //printf("blt_src_width=%d\n",tdrv->blt_src_width);
    //printf("blt_src_blt_src_height=%d\n",tdrv->blt_src_height);

    tdrv->src_blt.blt_src_size  = (((tdrv->src_blt.blt_src_width * tdrv->src_blt.blt_src_height * T2D_modesize(tdrv->bpp_type))+4095) >>12)<<12;
    //printf("tdrv->blt_src_size=%d\n!",tdrv->blt_src_size);
    tdrv->src_blt.blt_src_vaddr = (unsigned int)mmap(NULL, tdrv->src_blt.blt_src_size,
                                            PROT_READ | PROT_WRITE, MAP_SHARED, tdrv->src_blt.blt_src_fd, 0);

    tmp_src_add = tdrv->src_blt.blt_src_vaddr;
    //printf("src vaddr=%x\n!",tmp_src_add);

    if (ioctl(tdrv->src_blt.blt_src_fd, FRM_ADDR_V2P, &tmp_src_add) < 0) {
        printf("Error to call FRM_ADDR_V2P! \n");
    	close(tdrv->src_blt.blt_src_fd);
    	fclose(filer);
        return -1;
    }
    //printf("src paddr=%x\n!",tmp_src_add);

    tdrv->src_blt.blt_src_paddr= tmp_src_add;

    if ((int)tdrv->src_blt.blt_src_paddr < 0) {
        printf("Error to mmap lcd fb! \n");
    	close(tdrv->src_blt.blt_src_fd);
    	fclose(filer);
        return -1;
    }

    while(1){
    	fread((void*)tdrv->src_blt.blt_src_vaddr++,1,1,filer);
    	if(feof(filer)){
            printf("feof!\n");
            break;		
    	}
    }  

    fclose(filer);
    return 0;
}
/**
 * Think2d_Close_Src_Surface
 *
 * @param tdrv Driver data
 */
void Think2d_Close_Src_Surface(think2dge_desc_t *tdrv)
{
    close(tdrv->src_blt.blt_src_fd);
    munmap((void *)tdrv->src_blt.blt_src_vaddr, tdrv->src_blt.blt_src_size);
}

/**
 * Think2d_Set_Src_Stride to set source stride
 *
 * @param tdrv Driver data
 */
void Think2d_Set_Src_Stride(think2dge_desc_t *tdrv )
{
    DEBUG_PROC_ENTRY;

    if(tdrv->llst->cmd_cnt + 2 > T2D_CWORDS)
    	Think2D_EmitCommands((think2dge_desc_t*)tdrv, 1);

    pthread_mutex_lock(&tdrv->mutex);

    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_BLIT_SRCSTRIDE;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = tdrv->src_blt.blt_src_stride;

    //unlock semaphore
    pthread_mutex_unlock(&tdrv->mutex);
}
/**
 * Sets destination color key register.
 *
 * @param tdrv Driver data
 * @param color RGBA
 */
void Think2d_Set_Dst_Colorkey( think2dge_desc_t *tdrv,think2d_color_t* color)
{
    DEBUG_PROC_ENTRY;

    if(tdrv->llst->cmd_cnt + 2 > T2D_CWORDS)
    	Think2D_EmitCommands((think2dge_desc_t*)tdrv, 1);

    //lock semaphore
    pthread_mutex_lock(&tdrv->mutex);

    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_DST_COLORKEY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] =  COLOUR8888(color->r,color->g,color->b,color->a);

    pthread_mutex_unlock(&tdrv->mutex);
}

/**
 * Sets source color key register.
 *
 * @param tdrv Driver data
 * @param color color type
 */
void Think2d_Set_Src_Colorkey( think2dge_desc_t *tdrv,think2d_color_t* color)
{
    DEBUG_PROC_ENTRY;

    if(tdrv->llst->cmd_cnt + 2 > T2D_CWORDS)
        Think2D_EmitCommands((think2dge_desc_t*)tdrv, 1);

    pthread_mutex_lock(&tdrv->mutex);

    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_BLIT_SRCCOLORKEY;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = COLOUR8888(color->r, color->g,color->b, color->a);

    //unlock semaphore
    pthread_mutex_unlock(&tdrv->mutex);
}
/**
 * Sets Foreground color register.
 *
 * @param tdrv Driver data
 * @param color color type
 */

void Think2d_Set_FG_Color(think2dge_desc_t *tdrv,think2d_color_t* color)
{
    DEBUG_PROC_ENTRY;


    if(tdrv->llst->cmd_cnt + 2 > T2D_CWORDS)
    	Think2D_EmitCommands((think2dge_desc_t*)tdrv, 1);

    pthread_mutex_lock(&tdrv->mutex);

    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_BLIT_FG_COLOR;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = COLOUR8888(color->r, color->g,color->b, color->a);

    //unlock semaphore
    pthread_mutex_unlock(&tdrv->mutex);
}

/**
 * @param drv Driver data
 * @param srect Source rectangle
 * @param drect Destination rectangle
 */
int Think2D_StretchBlit( think2dge_desc_t *tdrv,think2d_retangle_t*srect,think2d_retangle_t* drect)           
{
    DEBUG_PROC_ENTRY;

    int rotation = 0;
    int src_colorkey = 0;
    int color_alpha = 0;
    int scale_x = T2D_NOSCALE, scale_xfn = (1 << T2D_SCALER_BASE_SHIFT);
    int scale_y = T2D_NOSCALE, scale_yfn = (1 << T2D_SCALER_BASE_SHIFT);
    int dx = drect->x, dy = drect->y;
    unsigned long srcAddr = \
    tdrv->src_blt.blt_src_paddr+ srect->x *	T2D_modesize(tdrv->bpp_type) + srect->y * tdrv->src_blt.blt_src_stride;

    rotation = tdrv->src_blt.blt_src_rotation;
    /* will we do rotation? */
    if (rotation == T2D_DEG090)
        dy += srect->h - 1;
    else if (rotation == T2D_DEG180)
        dx += srect->w - 1, dy += srect->h - 1;
    else if (rotation == T2D_DEG270)
        dx += srect->h - 1;
    else if (rotation == T2D_MIR_X)/*FLIP_HORIZONTAL*/
        dx += srect->w - 1;
    else if (rotation == T2D_MIR_Y)/*FLIP_VERTICAL*/
        dy += srect->h - 1;
    else
        rotation = T2D_DEG000;


    /* will we do SRC_COLORKEY? */
    if (tdrv->src_blt.blt_src_flags & THINK2D_BLIT_SRC_COLORKEY)
    	src_colorkey = 1;

    /* will we do COLORALPHA? */
    if (tdrv->src_blt.blt_src_flags & THINK2D_BLIT_BLEND_COLORALPHA)
    	color_alpha = 1;

    /* compute the scaling coefficients for X */
    if (drect->w > srect->w)
    	scale_x = T2D_UPSCALE, scale_xfn = (srect->w << T2D_SCALER_BASE_SHIFT) / drect->w;
    else if (drect->w < srect->w)
    	scale_x = T2D_DNSCALE, scale_xfn = (drect->w << T2D_SCALER_BASE_SHIFT) / srect->w;

    /* compute the scaling coefficients for Y */
    if (drect->h > srect->h)
    	scale_y = T2D_UPSCALE, scale_yfn = (srect->h << T2D_SCALER_BASE_SHIFT) / drect->h;
    else if (drect->h < srect->h)
    	scale_y = T2D_DNSCALE, scale_yfn = (drect->h << T2D_SCALER_BASE_SHIFT) / srect->h;

    if(tdrv->llst->cmd_cnt + 14 > T2D_CWORDS)
    	Think2D_EmitCommands((think2dge_desc_t*)tdrv, 1);

    pthread_mutex_lock(&tdrv->mutex);		

    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_BLIT_SRCADDR;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = srcAddr;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_BLIT_SRCRESOL;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(tdrv->src_blt.blt_src_width,tdrv->src_blt.blt_src_height);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_BLIT_DSTADDR;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(dx,dy);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_BLIT_DSTYXSIZE;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(drect->w, drect->h);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_BLIT_SCALE_XFN;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = scale_xfn;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_BLIT_SCALE_YFN;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = scale_yfn;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_BLIT_CMD;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = \
     (color_alpha << 28) | (src_colorkey << 27) |(scale_y << 25) | (scale_x << 23) | (rotation & 0x07) << 20 | tdrv->bpp_type;

    //unlock semaphore
    pthread_mutex_unlock(&tdrv->mutex);

    return THINK2D_SUCCESS;
}

/**
 * @param drv Driver data
 * @param rect Source rectangle
 * @param dx Destination X
 * @param dy Destination Y
 */
int Think2D_Blit( think2dge_desc_t *tdrv,think2d_retangle_t* rect,int dx,int dy )
{
	
    DEBUG_PROC_ENTRY;
	
    int rotation = 0;
    int src_colorkey = 0;
    int color_alpha = 0;
    unsigned long srcAddr = \
    tdrv->src_blt.blt_src_paddr + rect->x * T2D_modesize(tdrv->bpp_type) + rect->y * tdrv->src_blt.blt_src_stride;

    rotation = tdrv->src_blt.blt_src_rotation;
    src_colorkey = 0;
    color_alpha = 0;

    /* will we do rotation? */
    if (rotation == T2D_DEG090)
        dy += rect->h - 1;
    else if (rotation == T2D_DEG180)
	dx += rect->w - 1, dy += rect->h - 1;
    else if (rotation == T2D_DEG270)
	dx += rect->h - 1;
    else if (rotation == T2D_MIR_X)/*FLIP_HORIZONTAL*/
	dx += rect->w - 1;
    else if (rotation == T2D_MIR_Y)/*FLIP_VERTICAL*/
	dy += rect->h - 1;
    else
        rotation = T2D_DEG000;

    /* will we do SRC_COLORKEY? */
    if (tdrv->src_blt.blt_src_flags & THINK2D_BLIT_SRC_COLORKEY)
        src_colorkey = 1;

    /* will we do COLORALPHA? */
    if (tdrv->src_blt.blt_src_flags & THINK2D_BLIT_BLEND_COLORALPHA)
        color_alpha = 1;
    
    if(tdrv->llst->cmd_cnt + 8 > T2D_CWORDS)
        Think2D_EmitCommands((think2dge_desc_t*)tdrv, 1);

    pthread_mutex_lock(&tdrv->mutex);		

    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_BLIT_SRCADDR;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = srcAddr;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_BLIT_SRCRESOL;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(tdrv->src_blt.blt_src_width,tdrv->src_blt.blt_src_height);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_BLIT_DSTADDR;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_XYTOREG(dx,dy);
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_BLIT_CMD;
    tdrv->llst->cmd[tdrv->llst->cmd_cnt++] = (color_alpha << 28) | (src_colorkey << 27) | (rotation & 0x07) << 20 | tdrv->bpp_type;
	
	//unlock semaphore
    pthread_mutex_unlock(&tdrv->mutex);

    return 0;
}

#if 0 /*Fill rectange example*/
void main(void)
{
    think2dge_desc_t            *desc;
    think2d_color_t             color;
    think2d_retangle_t          rect;
    int i, j;

    desc = Think2dge_Lib_Open(T2D_RGBA5650);

    if(desc == NULL)
        return;  
    /* Set target surface mode base address stride, resolution */                                                          
    Think2d_Set_Dest_Surface((think2dge_desc_t*)desc);

    /*Set Clip window*/
    rect.x = 0 ,rect.y = 0;
    rect.w = desc->width , rect.h = desc->height;
    Think2d_Set_Clip_Window(desc,&rect);

    /*Set drawing blend*/
    desc->drawing_flags &=~(THINK2D_DRAW_DST_COLORKEY|THINK2D_DRAW_DSBLEND);
    Think2d_Set_Drawing_Blend(desc);

    /*Set drawing color*/
    color.r = 0xff;
    color.g = 0x00;
    color.b = 0x00;    
    Think2d_Set_Drawing_Color((think2dge_desc_t*)desc ,&color);

    printf("think2d Start to test >>>>>>>>>> \n");
	
    /*FillRectangle*/	
    for (i = 0; i < 100; i ++) {
 	rect.x = 0 + i*10;
	rect.y = 0 + i *10;
	rect.w = 10;
	rect.h = 10;
	Think2D_FillRectangle((think2dge_desc_t*)desc ,&rect);   
    }   
    Think2D_EmitCommands((think2dge_desc_t*)desc, 1);
    printf("think2d Test complete. >>>>>>>>>> \n");
    
    Think2dge_Lib_Close(desc);
}
#endif

#if 0 /*Draw Line example*/
void main(void)
{
    think2dge_desc_t            *desc;
    think2d_color_t             color;
    think2d_retangle_t          rect;
    think2d_region_t            line;
    int i, j;

    desc = Think2dge_Lib_Open(T2D_RGBA5650);
    if(desc == NULL)
        return;   
	/* Set target surface mode base address stride, resolution */                                                          
    Think2d_Set_Dest_Surface((think2dge_desc_t*)desc);

    /*Set Clip window*/
    rect.x = 0 ,rect.y = 0;
    rect.w = desc->width , rect.h = desc->height;
    Think2d_Set_Clip_Window(desc,&rect);

    /*Set drawing blend*/
    desc->drawing_flags &=~(THINK2D_DRAW_DST_COLORKEY|THINK2D_DRAW_DSBLEND);
    Think2d_Set_Drawing_Blend(desc);
    
    printf("think2d Start to test >>>>>>>>>> \n");	
    /*draw line*/
    color.r = 0x00 , color.g = 0xff , color.b = 0x00;    
    Think2d_Set_Drawing_Color((think2dge_desc_t*)desc ,&color);
    
    line.x1 = 0 , line.y1 = 0;
    line.y2 = 1080 , line.x2 = 1920;        
    Think2D_DrawLine((think2dge_desc_t*)desc ,&line);
        
    color.r = 0x00 , color.g = 0x00 , color.b = 0xff;        
    Think2d_Set_Drawing_Color((think2dge_desc_t*)desc ,&color);
    
    line.x1 = 1920 , line.y1 = 0;
    line.y2 = 1080 , line.x2 = 0;        
    Think2D_DrawLine((think2dge_desc_t*)desc ,&line);
    
    Think2D_EmitCommands((think2dge_desc_t*)desc, 1);
    printf("think2d Test complete. >>>>>>>>>> \n");
    
    Think2dge_Lib_Close(desc);
}	
#endif

#if 0 /*Blit example*/
void main(void)
{
    think2dge_desc_t            *desc;
    think2d_color_t             color;
    think2d_retangle_t          rect;
    int                         i,j;
    char                        file_name[20];

    desc = Think2dge_Lib_Open(T2D_RGBA5650);
    if(desc == NULL)
        return;   
    /* Set target surface mode base address stride, resolution */                                                          
    Think2d_Set_Dest_Surface((think2dge_desc_t*)desc);    
    /*Set Clip window*/
    rect.x = 0 ,rect.y = 0;
    rect.w = desc->width , rect.h = desc->height;
    Think2d_Set_Clip_Window(desc,&rect);
    /*Set source blit blend mode*/
    desc->src_blt.blt_src_flags = THINK2D_BLIT_COLORIZE;
    Think2d_Set_Blit_Blend((think2dge_desc_t*)desc);    
    /*Create source surface and open image file         */
    /*File format include 8bytes WxH and RGB565 RAW data*/
    sprintf(file_name,"%s","test.bin");
    Think2d_Open_Src_Surface((think2dge_desc_t*)desc,file_name);
    /*Set source stride*/
    desc->src_blt.blt_src_stride = desc->src_blt.blt_src_width * T2D_modesize(desc->bpp_type);  
    Think2d_Set_Src_Stride((think2dge_desc_t*)desc);
    /*Set FG color*/
    color.r =0x0,color.g =0xff,color.b=0x0;    
    Think2d_Set_FG_Color(desc,&color);    
    /*Set rotation*/
    desc->src_blt.blt_src_rotation = T2D_DEG000;    
    for(j = 0 ; j < desc->height ;j+=desc->src_blt.blt_src_height){
        for(i = 0 ; i < desc->width ; i+=desc->src_blt.blt_src_width){
            rect.x = 0;
            rect.y = 0;
            rect.w = desc->src_blt.blt_src_width;
            rect.h = desc->src_blt.blt_src_height;
            Think2D_Blit((think2dge_desc_t*)desc ,&rect,i,j);
        }
    }
    Think2D_EmitCommands((think2dge_desc_t*)desc, 1);
    Think2d_Close_Src_Surface(think2dge_desc_t * tdrv)((think2dge_desc_t*)desc);
    printf("think2d Test complete. >>>>>>>>>> \n");
    Think2dge_Lib_Close(desc);
    return;
}
#endif
#if 0 /*Blit stretch example*/
void main(void)
{
    think2dge_desc_t            *desc;
    think2d_color_t             color;
    think2d_retangle_t          srect ,drect;
    char                        file_name[20];
    
    desc = Think2dge_Lib_Open(T2D_RGBA5650);    
    if(desc == NULL)
        return;    
	/* Set target surface mode base address stride, resolution */                                                          
    Think2d_Set_Dest_Surface((think2dge_desc_t*)desc);    
    /*Set Clip window*/
    srect.x = 0 ,srect.y = 0;
    srect.w = desc->width , srect.h = desc->height;
    Think2d_Set_Clip_Window(desc,&srect);
    /*Set source blit blend mode*/
    desc->src_blt.blt_src_flags = 0;
    Think2d_Set_Blit_Blend((think2dge_desc_t*)desc);
    /*Create source surface and open image file         */
    /*File format include 8bytes WxH and RGB565 RAW data*/
    sprintf(file_name,"%s","test.bin");
	Think2d_Open_Src_Surface((think2dge_desc_t*)desc,file_name);
    /*Set source stride*/
    desc->src_blt.blt_src_stride = desc->src_blt.blt_src_width * T2D_modesize(desc->bpp_type);  
    Think2d_Set_Src_Stride((think2dge_desc_t*)desc);	
  
    /*upscaling*/
    srect.x = 0 , srect.y = 0;
    srect.w = desc->src_blt.blt_src_width , srect.h = desc->src_blt.blt_src_height;

    drect.x = 0 , drect.y = 0;
    drect.w = desc->width , drect.h = desc->height;
    Think2D_StretchBlit(desc ,&srect ,&drect);
	
    Think2D_EmitCommands((think2dge_desc_t*)desc, 1);
    Think2d_Close_Src_Surface((think2dge_desc_t*)desc);

    printf("think2d Test complete. >>>>>>>>>> \n");
    Think2dge_Lib_Close(desc);
    return;
}
#endif


