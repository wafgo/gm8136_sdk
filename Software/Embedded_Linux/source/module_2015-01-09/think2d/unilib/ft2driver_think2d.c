/*  Revise History
 *  2013/11/15 : Add Constant Alpha Blend function (Kevin)
 *	2013/11/21 : Add VERSION CONTROL and t2d_video_mode fix
 *	2014/08/28 : 1.2 version  add write unlock bit
 */
#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include "ft2d_gfx.h"
#include "ft2driver_think2d.h"

/*Definition*/
#define T2DGE_VERSION    "1.2"
/// Reads a device register.
#define T2D_GETREG(sdrv, reg) (*(volatile unsigned int *)((unsigned int)sdrv->mmio_base + reg))

/// Writes a device register.
#define T2D_SETREG(sdrv, reg, val) do { *(volatile unsigned int  *)((unsigned int)sdrv->mmio_base + reg) = (val); } while (0)
//#define T2D_SETREG(sdrv, reg, val) do { *(volatile u32 *)((unsigned long)sdrv->io_mem + reg) = (val); printf("%%%%%%%%%%%%%% '%02lx'=0x%08x\n", reg, val); } while (0)
/// Packs a pair of 16-bit coordinates in a 32-bit word as used by hardware.
#define T2D_XYTOREG(x, y) ((y) << 16 | ((x) & 0xffff))

#define T2D_CHECK_CMD_IS_ENOUGH(m ,n) (((m + n) > T2D_CWORDS) ? 0 : 1) 

#define COLOUR5551(r, g, b, a)  (((r) & 0x000000f8) << 8 | ((g) & 0x000000f8) << 3 | ((b) & 0x000000f8) >> 2 | ((a) & 0x00000080) >> 7)
#define COLOUR8888(r, g, b, a)  (((r) & 0x000000ff) << 24 | ((g) & 0x000000ff) << 16 | ((b) & 0x000000ff) << 8 | ((a) & 0x000000ff))

#define T2DGE_SUPPORTED_ACCEL_DRAWING_FUNC    (FT2D_DRAWLINE|FT2D_DRAWRECT|FT2D_FILLINGRECT)
#define T2DGE_SUPPORTED_ACCEL_BLITTING_FUNC   (FT2D_BLITTING | FT2D_STRETCHBLITTING)
#define T2DGE_SUPPORTED_DRAWINGFLAGS  (FT2D_COLOR_SET | FT2D_DSTINATION_SET | FT2D_CLIP_SET | FT2D_DST_COLORKEY_SET | FT2D_COLORBLEND_SET)
#define T2DGE_SUPPORTED_BLITTINGFLAGS (FT2D_COLOR_SET | FT2D_DSTINATION_SET | FT2D_SOURCE_SET | FT2D_COLORBLEND_SET |FT2D_DST_COLORKEY_SET | FT2D_SRC_COLORKEY_SET | FT2D_FG_COLORIZE_SET \
                                        | FT2D_DEG090_SET | FT2D_DEG180_SET | FT2D_DEG270_SET |FT2D_FLIP_HORIZONTAL_SET |FT2D_FLIP_VERTICAL_SET |FT2D_FORCE_FG_ALPHA | FT2D_CLIP_SET)


#define THINK2D_CLEAR_MODE     (FT2D_T2D_BLEND_ZERO_MODE << 16 | FT2D_T2D_BLEND_ZERO_MODE)
#define THINK2D_SRCCOPY_MODE   (FT2D_T2D_BLEND_ZERO_MODE << 16 | FT2D_T2D_BLEND_ONE_MODE)
#define THINK2D_SRCALPHA_MODE  (FT2D_T2D_BLEND_INVSRCALPHAR_MODE << 16 | FT2D_T2D_BLEND_ONE_MODE)

//#define DEBUG_OPERATIONS                                        
#ifdef DEBUG_OPERATIONS
#define DEBUG_OP(args...)	printf(args)
#else
#define DEBUG_OP(args...)
#endif

/// This enables or disables command list support in driver.
#define USE_CLIST

/// Enables debug messages on procedure entry.
//#define DEBUG_PROCENTRY

#ifdef DEBUG_PROCENTRY
#define DEBUG_PROC_ENTRY	do { printf("* %s\n", __FUNCTION__); } while (0)
#else
#define DEBUG_PROC_ENTRY
#endif
                                        
/**
 * Returns the bytes per pixel for a mode.
 *
 * @param mode Surface pixel format
 */
inline unsigned int T2D_modesize(unsigned int mode)
{
	switch (mode)
	{
		case FT2D_RGB_5650:
		case FT2D_RGBA_5551:
		case FT2D_RGBA4444:
		case FT2D_ARGB_1555:
			return 2;

		case FT2D_RGB_888:
		case FT2D_ARGB_8888:
		case FT2D_RGBA_8888:
			return 4;
		default : 
		    return 0;	
	}
	return 0;
}
/**
 * Converts to Think2D surface pixel format constants
 * and also performs validation for unsupported source or * destination formats.
 *
 */
static  int
t2d_video_mode( FT2D_BPP_T format )
{
    int ret = -1;
	
	switch (format)
	{
		case FT2D_RGB_5650:	
		{
			ret = T2D_RGBA5650;
			break;
		}
		case FT2D_RGBA_5551:	
		{
			ret = T2D_RGBA5551;
			break;
		}
		case FT2D_RGB_888:
		{
			ret = T2D_RGBX8888;
			break;
		}
		case FT2D_ARGB_8888:
		{
			ret = T2D_ARGB8888;
			break;
		}
		case FT2D_RGBA_8888:
		{
			ret = T2D_RGBA8888;
			break;
		}
		case FT2D_XRGB_8888:
		{
		    ret = T2D_XRGB8888;
		    break;    
		}
		default:		
		{
			ret = -1;
			break;
		}
	}
	//DEBUG_OP("t2d_video_mode %d\n", ret);
	return ret;
}
/**
 * think2dge_desc_t *tdrv to get shared commands pool
 * b_wait : to wait for command list to execute
 */
int T2D_EmitCommands( void *device )
{
    DEBUG_PROC_ENTRY;
    int i = 0;
    unsigned int ** p_buffer;
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
	FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
	think2dge_llst_t * cmd_list = (think2dge_llst_t*)drv_data->gfx_local;
    struct think2d_shared *sharemem = (struct think2d_shared *)drv_data->gfx_shared ;	
		
    if (sharemem->hw_running || T2D_GETREG(drv_data,T2D_STATUS) )
    {
        printf("%s list hw=%x state=%x\n",__FUNCTION__,sharemem->hw_running,T2D_GETREG(drv_data,T2D_STATUS));
        return FT2D_FAIL;
    }

    sharemem->sw_now = &sharemem->cbuffer[sharemem->sw_buffer][0];

    p_buffer = &sharemem->sw_now;
    /*move cmd buffer to driver shared memory*/
    for(i = 0 ; i < cmd_list->cmd_cnt ; i++)
    {
        (*p_buffer)[i] = cmd_list->cmd[i];	
    }

    sharemem->sw_preparing = cmd_list->cmd_cnt;
    sharemem->sw_ready += sharemem->sw_preparing;

    /*Emit command*/
	
    if (!sharemem->hw_running && !T2D_GETREG(drv_data, T2D_STATUS))
    {
        //printf("%s emit command NUMBER=%d" ,__FUNCTION__ , sharemem->sw_ready);
	    ioctl(drv_data->gfx_fd, T2D_START_BLOCK, 0);
	    /*clear cmd count*/
	    cmd_list->cmd_cnt = 0;
    }
    else
        printf("hw is running %2x %2x\n",sharemem->hw_running,T2D_GETREG(drv_data, T2D_STATUS));


    while (T2D_GETREG(drv_data, T2D_STATUS) && ioctl(drv_data->gfx_fd, T2D_WAIT_IDLE) < 0)
	{
	    printf("emit command is still busy\n");
	    break;
	} 
    return FT2D_SUCCESS;
}

/**
 * DirectFB callback that does a stretched blit.
 *
 * We use information saved on t2d_set_src() to also set the
 * source address register. See t2d_set_src(). We also
 * compute the scaling coefficients in the fixed point
 * format expected by the hardware.
 *
 * @param drv Driver data
 * @param dev Device data
 * @param srect Source rectangle
 * @param drect Destination rectangle
 */
static int
T2D_StretchBlit( void *device , FT2DRectangle *srect, FT2DRectangle *drect)
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
	FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
	think2dge_llst_t * cmd_list = (think2dge_llst_t*)drv_data->gfx_local;
	FT2D_SettingFlags_T  set_flag = device_data->accel_param;
	FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;    
	FT2D_Surface_Data *src_data = (FT2D_Surface_Data*) &device_data->drv_data.source_sur_data ; 
	
	DEBUG_PROC_ENTRY;
	DEBUG_OP("StretchBlit (%d, %d), %dx%d\n", drect->x, drect->y, drect->w, drect->h);

	int rotation = 0;
	int src_colorkey = 0;
	int color_alpha = 0;
	int scale_x = T2D_NOSCALE, scale_xfn = (1 << T2D_SCALER_BASE_SHIFT);
	int scale_y = T2D_NOSCALE, scale_yfn = (1 << T2D_SCALER_BASE_SHIFT);
	int dx = drect->x, dy = drect->y;
	unsigned long srcAddr;
	
	if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 14)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}
	if(!T2D_modesize(src_data->bpp) || t2d_video_mode(src_data->bpp) < 0){
	    printf("%s source bpp is invalide %d\n",__func__,src_data->bpp);
	    return FT2D_FAIL;
	}
	
	if(!src_data->surface_paddr){
	    printf("%s source paddr is invalide\n",__func__);
	    return FT2D_FAIL;    
	}
	
	srcAddr = src_data->surface_paddr + srect->x * T2D_modesize(src_data->bpp) + srect->y * (src_data->width *T2D_modesize(src_data->bpp));

    /* will we do rotation? */
	if (set_flag & FT2D_DEG090_SET)
		rotation = T2D_DEG090, dy += srect->h - 1;
	else if (set_flag & FT2D_DEG180_SET)
		rotation = T2D_DEG180, dx += srect->w - 1, dy += srect->h - 1;
	else if (set_flag & FT2D_DEG270_SET)
		rotation = T2D_DEG270, dx += srect->h - 1;
	else if (set_flag & FT2D_FLIP_HORIZONTAL_SET)
		rotation = T2D_MIR_X, dx += srect->w - 1;
	else if (set_flag & FT2D_FLIP_VERTICAL_SET)
		rotation = T2D_MIR_Y, dy += srect->h - 1;
		
	/* will we do SRC_COLORKEY? */
	if (set_flag & FT2D_SRC_COLORKEY_SET)
		src_colorkey = 1;

	/* will we do COLORALPHA? */
	if (set_flag & FT2D_FORCE_FG_ALPHA)
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


	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_BLIT_SRCADDR;
	cmd_list->cmd[cmd_list->cmd_cnt++] = srcAddr;
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_BLIT_SRCRESOL;
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(srect->w, srect->h);
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_BLIT_DSTADDR;
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(dx, dy);
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_BLIT_DSTYXSIZE;
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(drect->w, drect->h);
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_BLIT_SCALE_XFN;
	cmd_list->cmd[cmd_list->cmd_cnt++] = scale_xfn;
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_BLIT_SCALE_YFN;
	cmd_list->cmd[cmd_list->cmd_cnt++] = scale_yfn;
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_BLIT_CMD;
	cmd_list->cmd[cmd_list->cmd_cnt++] = (color_alpha << 28) | (src_colorkey << 27) | (scale_y << 25) | (scale_x << 23) | (rotation & 0x07) << 20 | t2d_video_mode(src_data->bpp);

    return FT2D_SUCCESS;
}


static int 
T2D_Blit( void *device , FT2DRectangle *rect, int dx, int dy )
{
	FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
	FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
	think2dge_llst_t * cmd_list = (think2dge_llst_t*)drv_data->gfx_local;
	FT2D_SettingFlags_T  set_flag = device_data->accel_param;
	FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;    
	FT2D_Surface_Data *src_data = (FT2D_Surface_Data*) &device_data->drv_data.source_sur_data ; 

	int rotation = 0;
	int src_colorkey = 0;
	int color_alpha = 0;
	unsigned long srcAddr;
	
    DEBUG_PROC_ENTRY;
	//DEBUG_OP("Blit (%s%s%s%s%s) (%d, %d), %dx%d\n", set_flag & (DSBLIT_ROTATE90 | DSBLIT_ROTATE180 | DSBLIT_ROTATE270 | DSBLIT_FLIP_HORIZONTAL | DSBLIT_FLIP_VERTICAL) ? "R" : "", tdev->blittingflags & DSBLIT_SRC_COLORKEY ? "K" : "", tdev->blittingflags & DSBLIT_DST_COLORKEY ? "D" : "", tdev->blittingflags & (DSBLIT_COLORIZE | DSBLIT_SRC_PREMULTCOLOR) ? "C" : "", tdev->blittingflags & (DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL) ? "A" : "", dx, dy, rect->w, rect->h);
    
    if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 8)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}
    
    if(!T2D_modesize(src_data->bpp) || t2d_video_mode(src_data->bpp) < 0){
	    printf("%s source bpp is invalide %d\n",__func__,src_data->bpp);
	    return FT2D_FAIL;
	}
	
	if(!src_data->surface_paddr){
	    printf("%s source paddr is invalide\n",__func__);
	    return FT2D_FAIL;    
	}
	
	srcAddr = src_data->surface_paddr + rect->x * T2D_modesize(src_data->bpp) + rect->y * (src_data->width *T2D_modesize(src_data->bpp));

	/* will we do rotation? */
	if (set_flag & FT2D_DEG090_SET)
		rotation = T2D_DEG090, dy += rect->h - 1;
	else if (set_flag & FT2D_DEG180_SET)
		rotation = T2D_DEG180, dx += rect->w - 1, dy += rect->h - 1;
	else if (set_flag & FT2D_DEG270_SET)
		rotation = T2D_DEG270, dx += rect->h - 1;
	else if (set_flag & FT2D_FLIP_HORIZONTAL_SET)
		rotation = T2D_MIR_X, dx += rect->w - 1;
	else if (set_flag & FT2D_FLIP_VERTICAL_SET)
		rotation = T2D_MIR_Y, dy += rect->h - 1;

	/* will we do SRC_COLORKEY? */
	if (set_flag & FT2D_SRC_COLORKEY_SET)
		src_colorkey = 1;

	/* will we do COLORALPHA? */
	if (set_flag & FT2D_FORCE_FG_ALPHA)
		color_alpha = 1;

	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_BLIT_SRCADDR;
	cmd_list->cmd[cmd_list->cmd_cnt++] = srcAddr;
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_BLIT_SRCRESOL;
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(rect->w, rect->h);
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_BLIT_DSTADDR;
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(dx, dy);
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_BLIT_CMD;
	cmd_list->cmd[cmd_list->cmd_cnt++] = (color_alpha << 28) | (src_colorkey << 27) | (rotation & 0x07) << 20 | t2d_video_mode(src_data->bpp);
    
    return FT2D_SUCCESS;
}

/**
 * draws a line.
 *
 * @param drv Driver data
 * @param line Line region
 */
int T2D_DrawLine( void *device,FT2DRegion *line)
{
    DEBUG_PROC_ENTRY;
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
	think2dge_llst_t * cmd_list = (think2dge_llst_t*)drv_data->gfx_local;
    DEBUG_OP("DrawLine (%d, %d)-(%d, %d)\n", line->x1, line->y1, line->x2, line->y2);

   if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 6)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}

    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_STARTXY;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(line->x1, line->y1);
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_ENDXY;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(line->x2, line->y2);
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
    cmd_list->cmd[cmd_list->cmd_cnt++] = 0x01;

    return FT2D_SUCCESS;
}
/**
 * draws a rectangle.
 *
 * @param drv Driver data
 * @param rect rectangle region
 */

int T2D_DrawRectangle( void *device,FT2DRectangle *rect   )
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
	think2dge_llst_t * cmd_list = (think2dge_llst_t*)drv_data->gfx_local;
    
    int x1 = rect->x;
    int y1 = rect->y;
    int x2 = rect->x + rect->w - 1;
    int y2 = rect->y + rect->h - 1;

    DEBUG_PROC_ENTRY;
    
    if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 24)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}

    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_STARTXY;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(x1, y1);
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_ENDXY;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(x1, y2);
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
    cmd_list->cmd[cmd_list->cmd_cnt++] = 0x01;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_STARTXY;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(x1, y2);
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_ENDXY;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(x2, y2);
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
    cmd_list->cmd[cmd_list->cmd_cnt++] = 0x01;         
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_STARTXY;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(x1, y1);
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_ENDXY;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(x2, y1);
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
    cmd_list->cmd[cmd_list->cmd_cnt++] = 0x01;         
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_STARTXY;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(x2, y1);
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_ENDXY;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(x2, y2);
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
    cmd_list->cmd[cmd_list->cmd_cnt++] = 0x01;         

    return FT2D_SUCCESS;
}

/**
 * @param  void *device
 * @param rect for target
 */
int T2D_FillRectangle( void *device,FT2DRectangle *rect )
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
	think2dge_llst_t * cmd_list = (think2dge_llst_t*)drv_data->gfx_local;
	
	DEBUG_PROC_ENTRY;
	DEBUG_OP("FillRectangle (%d, %d), %dx%d\n", rect->x, rect->y, rect->w, rect->h);

    if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 6)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}

    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_STARTXY;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(rect->x, rect->y);
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_ENDXY;
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(rect->x + rect->w - 1, rect->y + rect->h - 1);
    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
    cmd_list->cmd[cmd_list->cmd_cnt++] = 0x02;
    
    return FT2D_SUCCESS;
} 
/**
 * Sets destination surface registers.
 *
 * @param tdrv Driver data
 * @param tdev Device data
 * @param state Card state
 */
static int
t2d_set_dest(think2dge_llst_t * cmd_list ,void * device )
{
	DEBUG_PROC_ENTRY;
	FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
	FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*) &device_data->drv_data.target_sur_data ;  

	if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 8)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}
    if(!tar_data->surface_paddr || !tar_data->width || !tar_data->height 
       || !T2D_modesize(tar_data->bpp) || t2d_video_mode(tar_data->bpp) < 0){
        DEBUG_OP("%s invalide parameter\n",__func__);
        return  FT2D_FAIL;            
    }
    
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_TARGET_BAR;
	cmd_list->cmd[cmd_list->cmd_cnt++] = tar_data->surface_paddr;
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_TARGET_STRIDE;
	cmd_list->cmd[cmd_list->cmd_cnt++] = ((tar_data->width * T2D_modesize(tar_data->bpp)) & 0xffff);
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_TARGET_RESOLXY;
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(tar_data->width , tar_data->height);
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_TARGET_MODE;
	cmd_list->cmd[cmd_list->cmd_cnt++] = (1 << 31) | (0 << 29) | t2d_video_mode(tar_data->bpp);
	//cmd_list->cmd[cmd_list->cmd_cnt++] = (T2D_modesize(tar_data->bpp) == 4 ? 0 : ((1 << 31) | (1 << 23))) | (0 << 29) | t2d_video_mode(tar_data->bpp);  // IS:bit 29 disables 16->32 packing, bits 31/23 set on 32-bit mode

    return FT2D_SUCCESS;
}
/**
 * Sets the blit source.
 *
 * @param command list
 * @param tdev Device data
 */
static int t2d_set_src( think2dge_llst_t * cmd_list ,void * device )
{
	DEBUG_PROC_ENTRY;
	FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device; 
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    FT2D_Surface_Data *src_data = (FT2D_Surface_Data*) &device_data->drv_data.source_sur_data ; 

	if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 2)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}
	
	if(!T2D_modesize(src_data->bpp)){
	    printf("%s source stride is invalide\n",__func__);
	    return FT2D_FAIL;
	}
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_BLIT_SRCSTRIDE;
	cmd_list->cmd[cmd_list->cmd_cnt++] = ((src_data->width * T2D_modesize(src_data->bpp)) & 0xffff);
}

/**
 * Sets the blend for drawing operations.
 *
 * @param command list
 * @param tdev Device data
 */
static int
t2d_set_blit_blend(think2dge_llst_t * cmd_list ,void * device)
{
    DEBUG_PROC_ENTRY;
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
	FT2D_SettingFlags_T  set_flag = device_data->accel_param;
	FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;    
	int dst_colorkey = 0;
	int colorize = 0;
	
	if(set_flag & (FT2D_COLORBLEND_SET | FT2D_FORCE_FG_ALPHA)){
	    if(set_data->dst_blend > FT2D_T2D_BLEND_INVDESTCOLOR_MODE || set_data->src_blend > FT2D_T2D_BLEND_INVDESTCOLOR_MODE)
	    {
	        printf("%s blend %d %d over range\n",__func__,set_data->dst_blend,set_data->src_blend);
	        return -1;    
	    }
	}
	/* will we do DST_COLORKEY? */
	if (set_flag& FT2D_DST_COLORKEY_SET)
		dst_colorkey = 1;
	/* will we be using the FG_COLOR register for COLORIZE _or_ SRC_PREMULTIPLY? */
	if (set_flag & FT2D_FG_COLORIZE_SET)
		colorize = 1;

    if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 2)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}

    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_TARGET_BLEND;
    
    if(set_flag & (FT2D_COLORBLEND_SET | FT2D_FORCE_FG_ALPHA))
	    cmd_list->cmd[cmd_list->cmd_cnt++] = (dst_colorkey << 31) | (set_data->dst_blend << 16) | (set_data->src_blend);
	else
	    cmd_list->cmd[cmd_list->cmd_cnt++] = (dst_colorkey << 31) | (colorize << 30)|THINK2D_SRCCOPY_MODE;

	return FT2D_SUCCESS;
}

/**
 * Sets the blend for drawing operations.
 *
 * @param command list
 * @param tdev Device data
 */
static int
t2d_set_draw_blend( think2dge_llst_t * cmd_list ,void * device )
{
    DEBUG_PROC_ENTRY;
    int dst_colorkey = 0; 
	FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
	FT2D_SettingFlags_T  set_flag = device_data->accel_param;
	FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;

    if(set_flag & (FT2D_COLORBLEND_SET | FT2D_FORCE_FG_ALPHA)){
	    if(set_data->dst_blend > FT2D_T2D_BLEND_INVDESTCOLOR_MODE || set_data->src_blend > FT2D_T2D_BLEND_INVDESTCOLOR_MODE)
	    {
	        printf("%s blend %d %d over range\n",__func__,set_data->dst_blend,set_data->src_blend);
	        return -1;    
	    }
	}
	
	if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 2)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}

    if (set_flag & FT2D_DST_COLORKEY_SET)
	    dst_colorkey = 1;

    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_TARGET_BLEND;
    if(set_flag & FT2D_COLORBLEND_SET)
	    cmd_list->cmd[cmd_list->cmd_cnt++] = (dst_colorkey << 31) | (set_data->dst_blend << 16) | (set_data->src_blend);
	else
	    cmd_list->cmd[cmd_list->cmd_cnt++] = (dst_colorkey << 31) | THINK2D_SRCCOPY_MODE;
	    
	return FT2D_SUCCESS;
}
/**
 * Sets colorization color.
 *
 * Sets the color to be used for colorizing _or_ color modulation
 * with alpha value. The latter is essentially identical to colorizing
 * but using the alpha value instead of an RGB color.
 *
 * @param tdrv Driver data
 * @param tdev Device data
 * @param state Card state
 */
static int
t2d_set_color( think2dge_llst_t * cmd_list ,void * device  )
{
	DEBUG_PROC_ENTRY;
	FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
	FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;
    
	if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 2)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}

    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_BLIT_FG_COLOR;;
	cmd_list->cmd[cmd_list->cmd_cnt++] = set_data->color;
	return FT2D_SUCCESS;
}
/**
 * Sets the color for drawing operations.
 *
 * @param tdrv Driver data
 * @param tdev Device data
 * @param state Card state
 */
static int
t2d_set_drawing_color( think2dge_llst_t * cmd_list ,void * device )
{
	DEBUG_PROC_ENTRY;
	FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
	FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;

	if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 2)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}

    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DRAW_COLOR;;
	cmd_list->cmd[cmd_list->cmd_cnt++] = set_data->color;
	return FT2D_SUCCESS;
}
/**
 * Sets the source colorkey.
 *
 * @param command list
 * @param tdev Device data 
 */
static int
t2d_set_src_colorkey( think2dge_llst_t * cmd_list ,void * device )
{
    DEBUG_PROC_ENTRY;
	FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
    FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;

	if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 2)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}
   
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_BLIT_SRCCOLORKEY;;
	cmd_list->cmd[cmd_list->cmd_cnt++] = set_data->src_color;
    return FT2D_SUCCESS;
}

/**
 * Sets the destination colorkey.
 *
 * @param command list
 * @param tdev Device data 
 */
static int
t2d_set_dst_colorkey( think2dge_llst_t * cmd_list ,void * device )
{
    DEBUG_PROC_ENTRY;
	FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
    FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;

	if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 2)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}
   
	cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_DST_COLORKEY;;
	cmd_list->cmd[cmd_list->cmd_cnt++] = set_data->dst_color;
    return FT2D_SUCCESS;
}

/**
 * Sets the clipping registers.
 *
 * @param tdrv Driver data
 * @param tdev Device data
 * @param clip Clip region
 */
static int
t2d_set_clip( think2dge_llst_t * cmd_list ,void * device )
{
	DEBUG_PROC_ENTRY;
	FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;
	FT2D_SettingFlags_T  set_flag = device_data->accel_param;
	FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*) &device_data->drv_data.target_sur_data ; 
    FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;

	if(!T2D_CHECK_CMD_IS_ENOUGH(cmd_list->cmd_cnt , 4)){
	   DEBUG_OP("%s cmd is exceed then emit\n",__func__); 
	   T2D_EmitCommands(device);
	}
	
    if(set_flag & FT2D_CLIP_SET && !set_data->clip.x1 && !set_data->clip.y1 && !set_data->clip.x2 && !set_data->clip.y2 ){
        DEBUG_OP("%s invalide parameter\n",__func__);
        return  FT2D_FAIL;            
    }
    
    if(!tar_data->width || !tar_data->height ){
        DEBUG_OP("%s invalide target parameter\n",__func__);
        return  FT2D_FAIL;            
    }
    if(set_flag & FT2D_CLIP_SET){
	    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_CLIPMIN;;
	    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(set_data->clip.x1, set_data->clip.y1);
	    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_CLIPMAX;
	    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(set_data->clip.x2, set_data->clip.y2);
	}
	else{
	    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_CLIPMIN;;
	    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(0, 0);
	    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_CLIPMAX;
	    cmd_list->cmd[cmd_list->cmd_cnt++] = T2D_XYTOREG(tar_data->width, tar_data->height);
	    
	}
    return FT2D_SUCCESS;

}
int T2D_checkstate(void *device )
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device; 
    FT2D_AccelFlags_T       accel = device_data->accel_do;
    FT2D_SettingFlags_T  set_flag = device_data->accel_param;
  	FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*) &device_data->drv_data.target_sur_data ; 
    /* reject if we don't support the destination format */
    if((set_flag & FT2D_DSTINATION_SET) == FT2D_DSTINATION_SET){
    	if (t2d_video_mode(tar_data->bpp)==-1) {
	    printf("Rejecting: bad destination video mode: %u\n", tar_data->bpp);
	    return FT2D_FAIL;
	    }
    }
	
	if(accel & ~(T2DGE_SUPPORTED_ACCEL_DRAWING_FUNC |T2DGE_SUPPORTED_ACCEL_BLITTING_FUNC)){
	     printf("Rejecting: doesn't suport accel function \n");
		 return FT2D_FAIL;
	}
	
	if(accel & T2DGE_SUPPORTED_ACCEL_DRAWING_FUNC){
	    
	    if(set_flag & ~T2DGE_SUPPORTED_DRAWINGFLAGS){
	        printf("Rejecting: doesn't suport drawing setting \n");
		    return FT2D_FAIL;    
	    }
	}
	    
    if(accel & T2DGE_SUPPORTED_ACCEL_BLITTING_FUNC){
	    
	    if(set_flag & ~T2DGE_SUPPORTED_BLITTINGFLAGS){
	        printf("Rejecting: doesn't suport blitting setting \n");
		    return FT2D_FAIL;    
	    }
	}
    return FT2D_SUCCESS;
    
}

int T2D_SetState(void *device)
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device; 
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    think2dge_llst_t * cmd_list = (think2dge_llst_t*)drv_data->gfx_local;
    FT2D_AccelFlags_T       accel = device_data->accel_do;
    FT2D_SettingFlags_T  set_flag = device_data->accel_param;
    FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;
    int ret = 0 ;
    
    if(!cmd_list){
        printf("local command list is NULL\n");
        return FT2D_FAIL;
    }
            
    if (set_flag & FT2D_DSTINATION_SET)	{
		if(t2d_set_dest(cmd_list,device) < 0){	
		    printf("%s destination setting fail/n",__func__);
		    return FT2D_FAIL;   
		} 
	} 
	
	if (t2d_set_clip(cmd_list,device)<0){
	        printf("%s set clip fail\n",__func__);
	        return FT2D_FAIL;   
	}
	
	if ((set_flag & FT2D_SRC_COLORKEY_SET) &&
		t2d_set_src_colorkey(cmd_list,device) < 0){
		  	printf("%s set source colorkey fail\n",__func__);
	        return FT2D_FAIL;     
	}
	if ((set_flag & FT2D_DST_COLORKEY_SET) &&
		t2d_set_dst_colorkey(cmd_list,device) < 0){
	        printf("%s set destination colorkey fail\n",__func__);
	        return FT2D_FAIL;   
	}
	switch (accel)
	{
	    case FT2D_DRAWLINE :
	    case FT2D_DRAWRECT :
	    case FT2D_FILLINGRECT:    
	        if((set_flag & FT2D_COLOR_SET) && t2d_set_drawing_color(cmd_list,device) < 0) {
	            printf("%s darwing color fail\n",__func__);
	            return FT2D_FAIL;
	        } 
	        if(t2d_set_draw_blend(cmd_list,device) < 0 ) {
	            printf("%s darwing blend fail\n",__func__);
	            return FT2D_FAIL;  
	        }
	        	 
	        break;	 
	    case FT2D_BLITTING: 
	    case FT2D_STRETCHBLITTING:
	        if ((set_flag & FT2D_COLOR_SET) && t2d_set_color(cmd_list,device) < 0){
	            printf("%s blitting color fail\n",__func__);
	            return FT2D_FAIL;     
	        }	              
			          
	        if((set_flag & FT2D_SOURCE_SET) && t2d_set_src(cmd_list,device) < 0){
	            printf("%s blitting source fail\n",__func__);
	            return FT2D_FAIL;     
	        }  
	        
	        if(t2d_set_blit_blend(cmd_list,device) < 0 ) {
	            printf("%s blitter blend fail\n",__func__);
	            return FT2D_FAIL;  
	        }            
	        break;           
	} 
	return FT2D_SUCCESS;  
}

int driver_init_device_capability(FT2D_GfxDevice   *device)
{
    device->caps.flags = (FT2D_CLIPPING_FUNC|FT2D_POTERDUFF_FUNC|FT2D_DRAWING_FUNC \
                          |FT2D_BLITTER_FUNC|FT2D_SOURCEALPHA_FUNC|FT2D_DIFFSURFACE_TRANS_FUNC);
    device->caps.accel_func = (FT2D_FILLINGRECT|FT2D_DRAWLINE|FT2D_DRAWRECT|FT2D_BLITTING|FT2D_STRETCHBLITTING);     
}
/**
 *
 * @param device FT2D_GfxDevice device
 * @param funcs FT2D_GraphicsDeviceFuncs functions 
 */
int driver_init_device( FT2D_GfxDevice           *device,
                        FT2D_GraphicsDeviceFuncs *funcs,
                        FT2D_BPP_T rgb_t )
{
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device->drv_data ;
    FT2D_Surface_Data *fb_data = (FT2D_Surface_Data*) &device->drv_data.fb_data ;   
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo fix;
    int    byte_per_pixel , mapped_sz,iomem_sz;
    
    drv_data->gfx_fd = open("/dev/think2d", O_RDWR);
    if (drv_data->gfx_fd < 0) {
        printf("Error to open f2dge fd! \n");
        goto err;
    }
    
    drv_data->fb_fd = open("/dev/fb1", O_RDWR);
    if (drv_data->fb_fd < 0) {
        printf("Error to open LCD fd! \n");
        goto err1;
    }
    
    if (ioctl(drv_data->fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        printf("Error to call FBIOGET_VSCREENINFO! \n");
        goto err2;
    }

    if (ioctl(drv_data->fb_fd, FBIOGET_FSCREENINFO, &fix) < 0) {
        printf("Error to call FBIOGET_FSCREENINFO! \n");
        goto err2;
    }
    
    fb_data->surface_vaddr = (unsigned int)mmap(NULL, vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8),
                                                                        PROT_READ | PROT_WRITE, MAP_SHARED, drv_data->fb_fd, 0);
    if (fb_data->surface_vaddr <= 0) {
        printf("Error to mmap lcd fb! \n");
        goto err2;
    }
    
    printf("THINK2D Engine GUI(%s): xres = %d, yres = %d, bits_per_pixel = %d, size = %d \n",
            T2DGE_VERSION,
            vinfo.xres, vinfo.yres, vinfo.bits_per_pixel,
            vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8));
    
    fb_data->width = vinfo.xres;
    fb_data->height = vinfo.yres;
    
    byte_per_pixel = T2D_modesize (rgb_t);
    if(byte_per_pixel == 0){
        printf("%s, FB format doesn't support! \n", __func__);
        goto err3;    
    }
    
    fb_data->surface_paddr = (unsigned int)fix.smem_start;
    fb_data->bpp = rgb_t;
    
    drv_data->gfx_local = (void *)malloc(sizeof(think2dge_llst_t));
    if (!drv_data->gfx_local) {
        printf("%s, allocate llst fail! \n", __func__);
        goto err3;
    }
    
    memset((void *)drv_data->gfx_local, 0, sizeof(think2dge_llst_t));

    /* mapping the share memory and register I/O */
    mapped_sz = ((sizeof(struct think2d_shared) + 4095) >> 12) << 12;   //page alignment
    iomem_sz  = ((T2D_REGFILE_SIZE + 4095) >> 12) << 12;
    drv_data->gfx_map_sz = mapped_sz+iomem_sz;
    drv_data->gfx_shared = (void *)mmap(NULL, drv_data->gfx_map_sz, PROT_READ | PROT_WRITE, MAP_SHARED, drv_data->gfx_fd, 0);
    if (!drv_data->gfx_shared) {
        printf("%s, mapping share memory fail! \n", __func__);
        goto err4;
    }	
    
    /* register I/O */
    drv_data->mmio_base =(unsigned int*)((unsigned int)drv_data->gfx_shared + mapped_sz);
    if (!drv_data->mmio_base) {
        printf("%s, mapping io_mem fail! \n", __func__);
        goto err5;
    }
    
       /* confirm that we have a Think2D accelerator in place */
    if (T2D_GETREG(drv_data, T2D_IDREG) != T2D_MAGIC)
    {
        printf("Think2D: Accelerator not found!\n");
        goto err5;
    }
    
    driver_init_device_capability(device);
    
    funcs->EngineReset       = NULL;
    funcs->EmitCommands      = T2D_EmitCommands;
    funcs->CheckState        = T2D_checkstate;
    funcs->SetState          = T2D_SetState;
    funcs->GetSupportFeature = NULL;
    funcs->FillRectangle     = T2D_FillRectangle;
    funcs->DrawRectangle     = T2D_DrawRectangle;
    funcs->DrawLine          = T2D_DrawLine;
    funcs->Blit              = T2D_Blit;
    funcs->StretchBlit       = T2D_StretchBlit;     
     
    return FT2D_SUCCESS;
    
    err5:       
    err4:
        free(drv_data->gfx_local);
        munmap((void *)drv_data->gfx_shared, drv_data->gfx_map_sz);    
    err3:
        munmap((void *)fb_data->surface_vaddr, fb_data->width * fb_data->height * byte_per_pixel);    
    err2:
        close(drv_data->fb_fd);
    err1:
        close(drv_data->gfx_fd);        
    err :
        return FT2D_FAIL;   
}

/*
 * @This function is used to close a ft2dge channel.
 *
 * @functiondriver_close_device(FT2D_GfxDevice   *device);
 * @Parmameter desc indicates the descritor given by open stage
 * @return none
 */
void driver_close_device(FT2D_GfxDevice   *device)
{
    int byte_per_pixel;
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device->drv_data ;
    FT2D_Surface_Data *fb_data = (FT2D_Surface_Data*) &device->drv_data.fb_data;   

    byte_per_pixel = T2D_modesize (fb_data->bpp);

    /* close 2denge fd */
    close(drv_data->gfx_fd);
    close(drv_data->fb_fd);
    free(drv_data->gfx_local);
    munmap((void *)fb_data->surface_vaddr, fb_data->width * fb_data->height * byte_per_pixel);
    munmap((void *)drv_data->gfx_shared, drv_data->gfx_map_sz); 
}
