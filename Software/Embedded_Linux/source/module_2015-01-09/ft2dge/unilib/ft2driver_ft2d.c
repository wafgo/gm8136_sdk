/*  Revise History
 *  2013/11/15 : Add Constant Alpha Blend function (Kevin)
 *	2014/03/20 : support srccolor key function
 */
#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include "ft2d_gfx.h"
#include "ft2driver_ft2d.h"


//#define DEBUG_OPERATIONS                                        
#ifdef DEBUG_OPERATIONS
#define DEBUG_OP(args...)	printf(args)
#else
#define DEBUG_OP(args...)
#endif
#define COLOUR565(r, g, b)  (((r) & 0x000000f8) << 8 | ((g) & 0x000000fc) << 3 | ((b) & 0x000000f8) >> 3)
/// Enables debug messages on procedure entry.
//#define DEBUG_PROCENTRY

#ifdef DEBUG_PROCENTRY
#define DEBUG_PROC_ENTRY	do { printf("* %s\n", __FUNCTION__); } while (0)
#else
#define DEBUG_PROC_ENTRY
#endif
                                        
#define STYLE_SOLID				0x00000000
#define STYLE_DASH              0x00FFFFC0      //  DASH         -------
#define STYLE_DOT               0x00E38E38      //  DOT          .......
#define STYLE_DASH_DOT          0x00FF81C0      //  DASHDOT      _._._._
#define STYLE_DASH_DOT_DOT      0x00FF8E38      //  DASHDOTDOT   _.._.._
#define INV_STYLE_DASH          0x0000003F
#define INV_STYLE_DOT           0x001C71C7
#define INV_STYLE_DASH_DOT      0x00007E3F
#define INV_STYLE_DASH_DOT_DOT  0x000071C7

/* frequently used ROP3 code for ft2d only*/
#define ROP_BALCKNESS 	    0x00 	 	// 0x00 BLACKNESS
#define ROP_NOTSRCERASE     0x11 		// 0x11 NOTSRCERASE
#define ROP_NOTSRCCOPY 	    0x33  		// 0x33 NOTSRCCOPY
#define ROP_SRCERASE 	    0x44    	// 0x44 SRCERASE
#define ROP_DSTINVERT 	    0x55   		// 0x55 DSTINVERT
#define ROP_PATINVERT 	    0x5A   		// 0x5A PATINVERT
#define ROP_SRCINVERT 	    0x55 		// 0x66 SRCINVERT
#define ROP_SRCAND 		    0x88 		// 0x88 SRCAND
#define ROP_MERGEPAINT 	    0xBB 		// 0xBB MERGEPAINT
#define ROP_MERGECOPY 	    0xC0 		// 0xC0 MERGECOPY
#define ROP_SRCCOPY 	    0xCC 		// 0xCC SRCCOPY
#define ROP_SRCPAINT 	    0xEE 		// 0xEE SRCPAINT
#define ROP_PATCOPY 	    0xF0 		// 0xF0 PATCOPY
#define ROP_PATPAINT 	    0xFB 		// 0xFB PATPAINT
#define ROP_WHITENESS 	    0xFF 		// 0xFF WHITENESS
static unsigned int  g_line_style[FT2D_MAX_LINESTYLE]=
{
    STYLE_SOLID, //STYLE_SOLID
    STYLE_SOLID, //STYLE_SOLID
    STYLE_DASH, 
    STYLE_DOT,
    STYLE_DASH_DOT,
    STYLE_DASH_DOT_DOT,
    INV_STYLE_DASH,
    INV_STYLE_DOT,
    INV_STYLE_DASH_DOT,
    INV_STYLE_DASH_DOT_DOT    
};

static unsigned int  g_rop_method[FT2D_ROP_MAX]=
{
 ROP_SRCCOPY 	    ,
 ROP_BALCKNESS 	    ,	 	// 0x00 BLACKNESS
 ROP_NOTSRCERASE    , 		// 0x11 NOTSRCERASE
 ROP_NOTSRCCOPY     ,  		// 0x33 NOTSRCCOPY
 ROP_SRCERASE 	    ,   	// 0x44 SRCERASE
 ROP_DSTINVERT 	    ,  		// 0x55 DSTINVERT
 ROP_PATINVERT 	    ,  		// 0x5A PATINVERT
 ROP_SRCINVERT 	    , 		// 0x66 SRCINVERT
 ROP_SRCAND 	    , 		// 0x88 SRCAND
 ROP_MERGEPAINT     , 		// 0xBB MERGEPAINT
 ROP_MERGECOPY 	    , 		// 0xC0 MERGECOPY
 ROP_SRCCOPY 	    , 		// 0xCC SRCCOPY
 ROP_SRCPAINT 	    , 		// 0xEE SRCPAINT
 ROP_PATCOPY 	    , 		// 0xF0 PATCOPY
 ROP_PATPAINT 	    , 		// 0xFB PATPAINT
 ROP_WHITENESS 	     		// 0xFF WHITENESS
};
int ft2dge_lib_fire(void *device);
int ft2d_SetState(void *device);
inline unsigned int ft2dge_modesize(unsigned int mode)
{
	switch (mode)
	{
	    case FT2D_RGB_888:
	    case FT2D_ARGB_8888: 
	        return 4;
		
        case FT2D_RGB_5650:
	    case FT2D_ARGB_1555:
	        return 2;
		default:
			return 0;		
	}
}

static inline int ft2dge_video_mode( FT2D_BPP_T format )
{
    int ret = -1;
	
	switch (format)
	{
		case FT2D_RGB_5650:	
		{
			ret = FT2DGE_RGB_565;
			break;
		}
		case FT2D_ARGB_1555:	
		{
			ret = FT2DGE_ARGB_1555;
			break;
		}
		case FT2D_RGB_888:
		{
			ret = FT2DGE_RGB_888;
			break;
		}
		case FT2D_ARGB_8888:
		{
			ret = FT2DGE_ARGB_8888;
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

/* When this function is called, the semaphore should be called to protect the database */
void ft2dge_lib_set_param(ft2dge_llst_t *cmd_list, unsigned int ofs, unsigned int val)
{
    cmd_list->cmd[cmd_list->cmd_cnt].ofs = ofs;
    cmd_list->cmd[cmd_list->cmd_cnt].val = val;
    cmd_list->cmd_cnt ++;
}

int ft2dge_set_patten_forground_color(ft2dge_llst_t *cmd_list , int color)
{
    DEBUG_OP("%s pcolor=%x\n",__func__,color); 		
    ft2dge_lib_set_param(cmd_list, FT2DGE_FPGCOLR,color);  
    return FT2D_SUCCESS;      
}

int ft2dge_set_source_forground_color(ft2dge_llst_t *cmd_list , int color)
{
    DEBUG_OP("%s color=%x\n",__func__,color); 	
    ft2dge_lib_set_param(cmd_list, FT2DGE_SFGCOLR,color);
    return FT2D_SUCCESS;      
}

int ft2dge_set_target_address(ft2dge_llst_t * cmd_list , void *device)
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;      
    FT2D_GfxDrv_Data *drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*) &device_data->drv_data.target_sur_data ;
    
    if(!tar_data->surface_paddr){
        printf("source surface address is NULL\n");
        return -1;
    }
    DEBUG_OP("%s target address=%x\n",__func__,tar_data->surface_paddr); 
    ft2dge_lib_set_param(cmd_list, FT2DGE_DSTSA,tar_data->surface_paddr); 
    return FT2D_SUCCESS;               
}

int ft2dge_set_line_style(ft2dge_llst_t * cmd_list , int style)
{
    if(style >= FT2D_MAX_LINESTYLE){
        printf("line style exceed range\n");   
        return -1;
    }
  
    unsigned int value = (g_line_style[style]<< 8);
    DEBUG_OP("%s line style=%x\n",__func__,value); 
    ft2dge_lib_set_param(cmd_list, FT2DGE_LSTYLE,value);    
    return FT2D_SUCCESS;                   
}

int ft2dge_set_force_alpha(ft2dge_llst_t * cmd_list , void *device)
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;  
    FT2D_GfxDrv_Data *drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;
    unsigned int value = (set_data->color & 0xFF);   
    
    DEBUG_OP("%s force_alpha=%x\n",__func__,value); 	
    ft2dge_lib_set_param(cmd_list, FT2DGE_LSTYLE,value); 
    return FT2D_SUCCESS;                       
}

int ft2dge_set_source_address(ft2dge_llst_t * cmd_list , void *device)
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;  
    FT2D_GfxDrv_Data *drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    FT2D_Surface_Data *src_data = (FT2D_Surface_Data*) &device_data->drv_data.source_sur_data ;
    
    if(!src_data->surface_paddr){
        printf("source surface address is NULL\n");
        return -1;
    }
    DEBUG_OP("%s source addr=%x\n",__func__,src_data->surface_paddr); 
    ft2dge_lib_set_param(cmd_list, FT2DGE_SRCSA,src_data->surface_paddr); 
    return FT2D_SUCCESS;                   
}

int ft2dge_set_pitch(ft2dge_llst_t * cmd_list , void *device)
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device; 
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*) &device_data->drv_data.target_sur_data ; 
    
    if(tar_data->width == 0)
        return FT2D_FAIL;
        
    DEBUG_OP("%s pitch=%x\n",__func__,FT2DGE_PITCH(tar_data->width, tar_data->width));             
    ft2dge_lib_set_param(cmd_list, FT2DGE_SPDP, FT2DGE_PITCH(tar_data->width, tar_data->width)); 
    return FT2D_SUCCESS;   
}

int ft2dge_checkstate(void *device )
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device; 
    FT2D_AccelFlags_T       accel = device_data->accel_do;
    FT2D_SettingFlags_T  set_flag = device_data->accel_param;
    FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*) &device_data->drv_data.target_sur_data ; 
    
    if(! accel || !set_flag){
        printf("Rejecting: no any param to check\n");
        return  FT2D_FAIL;   
    }
    /* reject if we don't support the destination format */
    if((set_flag & FT2D_DSTINATION_SET) == FT2D_DSTINATION_SET){
	    if (ft2dge_video_mode(tar_data->bpp) == -1) {
		    printf("Rejecting: bad destination video mode: %u\n", tar_data->bpp);
		    return FT2D_FAIL;
	    }
	}
	
	if(accel & ~(FT2DGE_SUPPORTED_ACCEL_DRAWING_FUNC |FT2DGE_SUPPORTED_ACCEL_BLITTING_FUNC
	   |FT2DGE_SUPPORTED_ACCEL_FT2D_FILLINGRECT)){
	     printf("Rejecting: doesn't suport accel function \n");
		 return FT2D_FAIL;
	}
	
	if(accel & FT2DGE_SUPPORTED_ACCEL_DRAWING_FUNC){
	    
	    if(set_flag & ~FT2DGE_SUPPORTED_DRAWINGFLAGS){
	        printf("Rejecting: doesn't suport drawing setting \n");
		    return FT2D_FAIL;    
	    }
	}
	if(accel & FT2DGE_SUPPORTED_ACCEL_FT2D_FILLINGRECT){
	    
	    if(set_flag & ~FT2DGE_SUPPORTED_FILLINGRECT_FLAG){
	        printf("Rejecting: doesn't suport FILLRECT setting \n");
		    return FT2D_FAIL;    
	    }
	}
    
    if(accel & FT2DGE_SUPPORTED_ACCEL_BLITTING_FUNC){
	    
	    if(set_flag & ~FT2DGE_SUPPORTED_BLITTINGFLAGS){
	        printf("Rejecting: doesn't suport blitting setting \n");
		    return FT2D_FAIL;    
	    }
	}
    return FT2D_SUCCESS;
    
}
int ft2d_SetState_dummy(void *device)
{
    if(device){}
    return 0 ;   
}
int ft2d_SetState(void *device)
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device; 
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    ft2dge_llst_t * cmd_list = (ft2dge_llst_t*)drv_data->gfx_local;
    FT2D_AccelFlags_T       accel = device_data->accel_do;
    FT2D_SettingFlags_T  set_flag = device_data->accel_param;
    FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;
    int ret = 0 ;
    
    if(!cmd_list){
        printf("local command list is NULL\n");
        return FT2D_FAIL;
    }
            
    if (set_flag & FT2D_DSTINATION_SET)	{
		if(ft2dge_set_pitch(cmd_list,device) < 0)
		    return FT2D_FAIL; 
		if(ft2dge_set_target_address(cmd_list,device) < 0)
		    return FT2D_FAIL;    
	} 
	
	switch (accel)
	{
	    case FT2D_DRAWLINE :
	    case FT2D_DRAWRECT :
	        if(set_flag & FT2D_COLOR_SET) {
	            ft2dge_set_patten_forground_color(cmd_list, set_data->color);
	        } 
	        if(set_flag & FT2D_LINE_STYLE_SET) {
	            if(ft2dge_set_line_style(cmd_list,set_data->line_style) < 0)
	               return FT2D_FAIL;  
	        }
	        	 
	        break;        
	    case FT2D_FILLINGRECT:
	        if(set_flag & FT2D_COLOR_SET){ 
	            ft2dge_set_source_forground_color(cmd_list, set_data->color);
	        }
	        break;           
	    case FT2D_BLITTING:            
	        if(set_flag & FT2D_SOURCE_SET){
	            if(ft2dge_set_source_address(cmd_list,device) < 0)
		            return FT2D_FAIL; 
     
	        }  
	        
	        if(set_flag & FT2D_FORCE_FG_ALPHA){
	            if(ft2dge_set_force_alpha(cmd_list,device) < 0)
		            return FT2D_FAIL; 
     
	        }        
	        break;           
	} 
	return ret;  
}
/* rectangle fill. source is from internal register or display memory
 */
int ft2dge_lib_fill_rectangle( void *device,FT2DRectangle *rect)
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device; 
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    ft2dge_llst_t * cmd_list = (ft2dge_llst_t*)drv_data->gfx_local;
    FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*) &device_data->drv_data.target_sur_data ; 
    ft2dge_cmd_t  command;
    int bpp;

    /* command action */
    command.value = 0;
    command.bits.rop = ROP_SRCCOPY;
    command.bits.command = FT2DGE_CMD_BITBLT;
    if((bpp = ft2dge_video_mode(tar_data->bpp)) < 0)
        return -1;
    command.bits.bpp = bpp;
    command.bits.no_source_image = 1;

    if((cmd_list->cmd_cnt + 8) >= LLST_MAX_CMD)
    {
        DEBUG_OP("ft2dge_lib_drawrect command exceed\n");
        ft2dge_lib_fire(device);
    }
    
    if(ft2d_SetState(device_data) < 0){
        printf("%s ft2d_SetState fail\n",__func__);
        return -1;        
    }
    DEBUG_OP("%s command = %x , rect=(%d,%d,%d,%d)\n",__func__,command.value,rect->x,rect->y,rect->w,rect->h);
    ft2dge_lib_set_param(cmd_list, FT2DGE_CMD, command.value);
    
    /* destination object upper-left X/Y coordinates */
    ft2dge_lib_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(rect->x, rect->y));
    /* destination object rectanlge width and height */
    ft2dge_lib_set_param(cmd_list, FT2DGE_DSTWH, FT2DGE_WH(rect->w,rect->h));
    /* when all parameters are updated, this is the final step */
    ft2dge_lib_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);

    /* Note, all parameters are updated into DRAM only, you should call fire function to active them.
     */
    return 0;
}

int ft2dge_lib_drawrect( void *device , FT2DRectangle *rect )
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;        
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    ft2dge_llst_t * cmd_list = (ft2dge_llst_t*)drv_data->gfx_local;
    FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*) &device_data->drv_data.target_sur_data ;
    FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data; 
    ft2dge_cmd_t  command;
    int bpp ;
    int x1 = rect->x;
    int y1 = rect->y;
    int x2 = rect->x + rect->w - 1;
    int y2 = rect->y + rect->h - 1;
   
    
    if((cmd_list->cmd_cnt + 20) >= LLST_MAX_CMD) /*16 + 5*/
    {
        DEBUG_OP("ft2dge_lib_drawrect command exceed\n");
        ft2dge_lib_fire(device);
    }
    
    if(ft2d_SetState(device_data) < 0){
        printf("%s ft2d_SetState fail\n",__func__);
        return -1;        
    }
     
    command.value = 0;
     
    if((device_data->accel_do & FT2D_DRAWRECT)){
        if((device_data->accel_param & FT2D_LINE_STYLE_SET) && 
            (set_data->line_style < FT2D_MAX_LINESTYLE &&  set_data->line_style > FT2D_STYLE_SOLID))
            command.bits.style_en = 1;
        else
            command.bits.style_en = 0;            
    } else{
        printf("%s don't support non drawrect\n",__func__);
        return -1;
    }
    
    command.bits.command = FT2DGE_CMD_LINE_DRAWING;
    if((bpp = ft2dge_video_mode(tar_data->bpp)) < 0)
        return -1;
    command.bits.bpp = bpp;
    command.bits.no_source_image = 1;

    DEBUG_OP("%s command = %x , rect=(%d,%d,%d,%d)\n",__func__,command.value,x1,y1,x2,y2); 	
    /* Endpoint-A X/Y coordinates for line drawing */
    ft2dge_lib_set_param(cmd_list, FT2DGE_SRCXY, FT2DGE_XY(x1, y1));
    /* Endpoint-B X/Y coordinates for line drawing */
    ft2dge_lib_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(x1, y2));
    ft2dge_lib_set_param(cmd_list, FT2DGE_CMD, command.value);
    ft2dge_lib_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);
    
      /* Endpoint-A X/Y coordinates for line drawing */
    ft2dge_lib_set_param(cmd_list, FT2DGE_SRCXY, FT2DGE_XY(x1, y2));
    /* Endpoint-B X/Y coordinates for line drawing */
    ft2dge_lib_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(x2, y2));

    ft2dge_lib_set_param(cmd_list, FT2DGE_CMD, command.value);
    ft2dge_lib_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);
    
    ft2dge_lib_set_param(cmd_list, FT2DGE_SRCXY, FT2DGE_XY(x2,y2));
    /* Endpoint-B X/Y coordinates for line drawing */
    ft2dge_lib_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(x2,y1));

    ft2dge_lib_set_param(cmd_list, FT2DGE_CMD, command.value);
    ft2dge_lib_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);
    
    ft2dge_lib_set_param(cmd_list, FT2DGE_SRCXY, FT2DGE_XY(x2, y1));
    /* Endpoint-B X/Y coordinates for line drawing */
    ft2dge_lib_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(x1,y1));

    ft2dge_lib_set_param(cmd_list, FT2DGE_CMD, command.value);
    ft2dge_lib_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);
    
    return 0;    
}

int ft2dge_lib_drawline( void *device , FT2DRegion *line )
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device ;       
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*) &device_data->drv_data.target_sur_data ; 
    ft2dge_llst_t * cmd_list = (ft2dge_llst_t*)drv_data->gfx_local;
    FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;
    ft2dge_cmd_t  command;
    int bpp ; 
   
    command.value = 0;
    
    if((cmd_list->cmd_cnt + 8) >= LLST_MAX_CMD) /*16 + 5*/
    {
        DEBUG_OP("ft2dge_lib_drawline command exceed\n");
        ft2dge_lib_fire(device);
    }
    
    if(ft2d_SetState(device_data) < 0){
        printf("%s ft2d_SetState fail\n",__func__);
        return -1;        
    }
    
    if((device_data->accel_do & FT2D_DRAWLINE)){
        if((device_data->accel_param & FT2D_LINE_STYLE_SET) && 
            (set_data->line_style < FT2D_MAX_LINESTYLE &&  set_data->line_style > FT2D_STYLE_SOLID))
            command.bits.style_en = 1;
        else
            command.bits.style_en = 0;            
    } else{
        printf("%s don't support non drawrect\n",__func__);
        return -1;
    }    

    command.bits.command = FT2DGE_CMD_LINE_DRAWING;
    if((bpp = ft2dge_video_mode(tar_data->bpp)) < 0)
        return -1;
    command.bits.bpp = bpp;
    command.bits.no_source_image = 1;
    
    DEBUG_OP("%s command = %x , rect=(%d,%d,%d,%d)\n",__func__,command.value,line->x1,line->y1,line->x2,line->y2); 
    /* Endpoint-A X/Y coordinates for line drawing */
    ft2dge_lib_set_param(cmd_list, FT2DGE_SRCXY, FT2DGE_XY(line->x1, line->y1));
    /* Endpoint-B X/Y coordinates for line drawing */
    ft2dge_lib_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(line->x2, line->y2));

    ft2dge_lib_set_param(cmd_list, FT2DGE_CMD, command.value);
    ft2dge_lib_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);
    
    return 0;    
}
/* not yet */
int ft2dge_lib_blit(void *device , FT2DRectangle *rect, int dx, int dy)
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device ;       
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*) &device_data->drv_data.target_sur_data ; 
    FT2D_Surface_Data *src_data = (FT2D_Surface_Data*) &device_data->drv_data.source_sur_data ;
    FT2D_GFX_Setting_Data* set_data =(FT2D_GFX_Setting_Data*) &device_data->accel_setting_data;
    ft2dge_llst_t * cmd_list = (ft2dge_llst_t*)drv_data->gfx_local;
    ft2dge_cmd_t  command;
    int bpp ;

    command.value = 0;
    /* command action */
    if((device_data->accel_do & FT2D_BLITTING)){
        if((device_data->accel_param & FT2D_ROP_METHOD_SET)&& device_data->accel_setting_data.rop_method !=0) {
            if(set_data->rop_method >=FT2D_ROP_MAX ){
                printf("%s don't support this ROP %d\n",__func__,set_data->rop_method);
                return -1;    
            }
            else
                command.bits.rop = g_rop_method[set_data->rop_method];
            
        }
        else
            command.bits.rop = ROP_SRCCOPY;            
    }
    else{
        printf("%s don't support non blitteing\n",__func__);
        return -1;
    }
    
    if(device_data->accel_param & FT2D_FORCE_FG_ALPHA)
    {
        command.bits.AlphaFormat = 0x00;
        command.bits.command = FT2DGE_CMD_ALPHA_BLENDING;
        command.bits.rop = ROP_SRCCOPY;
    }
    else if(device_data->accel_param & FT2D_SRC_COLORKEY_SET){
        command.bits.command = FT2DGE_CMD_COLORKEY;
        command.bits.mono_pattern_or_match_not_write = 1;
    }
    else
        command.bits.command = FT2DGE_CMD_BITBLT;
        
    if((cmd_list->cmd_cnt + 12) >= LLST_MAX_CMD) /*8 + 4*/
    {
        DEBUG_OP("ft2dge_lib_drawline command exceed\n");
        ft2dge_lib_fire(device);
    }
    
    if(ft2d_SetState(device_data) < 0){
        printf("%s ft2d_SetState fail\n",__func__);
        return -1;        
    }
    
    if((bpp = ft2dge_video_mode(tar_data->bpp)) < 0)
        return -1;    
   
    command.bits.bpp = bpp;
    command.bits.no_source_image = 0;
  
    DEBUG_OP("%s command = %x , rect=(%d,%d,%d,%d)\n",__func__,command.value,line->x1,line->y1,line->x2,line->y2); 
    ft2dge_lib_set_param(cmd_list, FT2DGE_CMD, command.value);
    if(device_data->accel_param & FT2D_SRC_COLORKEY_SET){
        ft2dge_lib_set_param(cmd_list, FT2DGE_TPRCOLR,set_data->src_color);
        ft2dge_lib_set_param(cmd_list, FT2DGE_TPRMASK,set_data->src_color);
    }
    /* source pitch and destination pitch */
    ft2dge_lib_set_param(cmd_list, FT2DGE_SPDP, FT2DGE_PITCH(src_data->width, tar_data->width));
    /* srouce object start x/y */
    ft2dge_lib_set_param(cmd_list, FT2DGE_SRCXY, FT2DGE_XY(rect->x, rect->y));
    /* Source Rectangle Width and Height */
    ft2dge_lib_set_param(cmd_list, FT2DGE_SRCWH, FT2DGE_WH(rect->w, rect->h));
    
    /* Destination object upper-left X/Y coordinates */
	ft2dge_lib_set_param(cmd_list, FT2DGE_DSTXY, FT2DGE_XY(dx, dy));
	/* Destination rectangle width and height */
	ft2dge_lib_set_param(cmd_list, FT2DGE_DSTWH, FT2DGE_WH(rect->w, rect->h));
	/* when all parameters are updated, this is the final step */
    ft2dge_lib_set_param(cmd_list, FT2DGE_TRIGGER, 0x1);


    /* Note, all parameters are updated into DRAM only, you should call fire function to active them.
     */
    return 0;
}
/* fire all added commands
 * b_wait = 0: not wait for complete
 * b_wait = 1:  wait for complete
 * return value: 0 for success, -1 for fail
 */
int ft2dge_lib_fire(void *device)
{
    FT2D_GfxDevice   *device_data = (FT2D_GfxDevice*) device;        
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&device_data->drv_data ;
    ft2dge_llst_t *llst = (ft2dge_llst_t*)drv_data->gfx_local;
    volatile void *io_mem = (void*)drv_data->mmio_base;
    struct ft2dge_idx two_idx;
    ft2dge_sharemem_t *sharemem = (ft2dge_sharemem_t *)drv_data->gfx_shared;
    unsigned int write_idx, read_idx;
    int i, idx, cmd_count;

    if (!llst->cmd_cnt)
        return 0;

    if (llst->cmd_cnt >= LLST_MAX_CMD) {
        printf("%s, command count: %d is over than %d!\n", __func__, llst->cmd_cnt, LLST_MAX_CMD);
        return -1;
    }
    DEBUG_OP("%s, command count: %d is over than %d!\n", __func__, llst->cmd_cnt, LLST_MAX_CMD);
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

    ioctl(drv_data->gfx_fd, FT2DGE_CMD_SYNC, &two_idx);

    return 0;
}
int driver_init_device_capability(FT2D_GfxDevice   *device)
{
    device->caps.flags = (FT2D_ROP_FUNC|FT2D_DRAWING_FUNC|FT2D_BLITTER_FUNC);
    device->caps.accel_func = (FT2D_FILLINGRECT|FT2D_DRAWLINE|FT2D_DRAWRECT|FT2D_BLITTING);     
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
    int    byte_per_pixel , mapped_sz;
    
    drv_data->gfx_fd = open("/dev/ft2dge", O_RDWR);
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
    
    printf("FT2D Engine GUI(1.0): xres = %d, yres = %d, bits_per_pixel = %d, size = %d \n",
            vinfo.xres, vinfo.yres, vinfo.bits_per_pixel,
            vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8));
    
    fb_data->width = vinfo.xres;
    fb_data->height = vinfo.yres;
    
    byte_per_pixel = ft2dge_modesize (rgb_t);
    if(byte_per_pixel == 0){
        printf("%s, FB format doesn't support! \n", __func__);
        goto err3;    
    }
    
    fb_data->surface_paddr = (unsigned int)fix.smem_start;
    fb_data->bpp = rgb_t;
    
    drv_data->gfx_local = (void *)malloc(sizeof(ft2dge_llst_t));
    if (!drv_data->gfx_local) {
        printf("%s, allocate llst fail! \n", __func__);
        goto err3;
    }
    
    memset((void *)drv_data->gfx_local, 0, sizeof(ft2dge_llst_t));

    /* mapping the share memory and register I/O */
    mapped_sz = ((sizeof(ft2dge_sharemem_t) + 4095) >> 12) << 12;   //page alignment

    drv_data->gfx_map_sz = mapped_sz;
    drv_data->gfx_shared = (void *)mmap(NULL, drv_data->gfx_map_sz , PROT_READ | PROT_WRITE, MAP_SHARED, drv_data->gfx_fd, 0);
    if (!drv_data->gfx_shared) {
        printf("%s, mapping share memory fail! \n", __func__);
        goto err4;
    }
    
    /* register I/O */
   
    drv_data->mmio_base = (void *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, drv_data->gfx_fd, drv_data->gfx_map_sz);
    if (!drv_data->mmio_base) {
        printf("%s, mapping io_mem fail! \n", __func__);
        goto err5;
    }
    
    driver_init_device_capability(device);
    
    funcs->EngineReset       = NULL;
    funcs->EmitCommands      = ft2dge_lib_fire;
    funcs->CheckState        = ft2dge_checkstate;
    funcs->SetState          = ft2d_SetState_dummy;
    funcs->GetSupportFeature = NULL;
    funcs->FillRectangle     = ft2dge_lib_fill_rectangle;
    funcs->DrawRectangle     = ft2dge_lib_drawrect;
    funcs->DrawLine          = ft2dge_lib_drawline;
    funcs->Blit              = ft2dge_lib_blit;
    funcs->StretchBlit       = NULL;     
     
    return FT2D_SUCCESS;
    
    err5:
        munmap((void *)drv_data->mmio_base , 4096);
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
    FT2D_Surface_Data *fb_data = (FT2D_Surface_Data*) &device->drv_data.fb_data ;   

    byte_per_pixel = ft2dge_modesize (fb_data->bpp);

    /* close 2denge fd */
    close(drv_data->gfx_fd);
    close(drv_data->fb_fd);
    free(drv_data->gfx_local);
    munmap((void *)fb_data->surface_vaddr, fb_data->width * fb_data->height * byte_per_pixel);
    munmap((void *)drv_data->gfx_shared, drv_data->gfx_map_sz); 
    munmap((void *)drv_data->mmio_base , 4096);
}