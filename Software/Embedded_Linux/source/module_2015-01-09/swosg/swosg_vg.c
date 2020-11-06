#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/hardirq.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <mach/platform/platform_io.h>
#include "log.h"    //include log system "printm","damnit"...
#include "video_entity.h"   //include video entity manager
#include "common.h"
#include "swosg_vg.h"

/*
 * Main structure
 */
static g_info_t  g_info;

#define DRV_VGLOCK(x)          spin_lock_irqsave(&g_info.lock, x)
#define DRV_VGUNLOCK(x)        spin_unlock_irqrestore(&g_info.lock, x)

#define DRV_PARAMLOCK(x)       spin_lock_irqsave(&g_info.para_lock, x)
#define DRV_PARAMUNLOCK(x)     spin_unlock_irqrestore(&g_info.para_lock, x)

#define DRV_MULTILOCK(x)       spin_lock_irqsave(&g_info.multi_lock, x)
#define DRV_MULTIUNLOCK(x)     spin_unlock_irqrestore(&g_info.multi_lock, x)

#define OSG_JOBFAIL_INC(x,ch)  do{ DRV_VGLOCK(x); \
                                   atomic_inc(&g_info.job_fail_cnt[ch]); \
                                   DRV_VGUNLOCK(x); \
                                 }while(0)

/* scheduler */
static DECLARE_DELAYED_WORK(process_callback_work, 0);
static unsigned int callback_period = 2;    //ticks


/* proc system */
static struct proc_dir_entry *entity_proc, *infoproc, *cbproc, *utilproc;
static struct proc_dir_entry *propertyproc, *jobproc, *filterproc, *levelproc;

/* utilization */

util_t g_utilization[OSG_FIRE_MAX_NUM];
/* property lastest record */
struct property_record_t *property_record;

/* log & debug message */
#define MAX_FILTER  5
unsigned int log_level = LOG_QUIET;
static int include_filter_idx[MAX_FILTER]; //init to -1, ENGINE << 16 | MINOR
static int exclude_filter_idx[MAX_FILTER]; //init to -1, ENGINE << 16 | MINOR

/* function declaration */
void osg_callback_process(void);
static void osg_joblist_add(int chan_id, osg_job_t *node);

/*define macro*/
#define OSG_GET_MODE_PARAM_ALIGN(value)         ((value&0xFF00)>>8)
#define OSG_GET_MODE_PARAM_MODE(value)          (value&0xFF)

#define OSG_GET_BORDER_PARAM_PALETTE(value)     ((value&0xFF0000)>>16)
#define OSG_GET_BORDER_PARAM_WIDTH(value)       ((value&0xFF00)>>8) 
#define OSG_GET_BORDER_PARAM_TYPE(value)        (value&0xFF) 

#define OSG_GET_BLENDING_PARAM(value)           (value&0xFF) 


#define OSG_RECT_MUST_PAPAMETER  (JOB_PARA_DRAWING_TYPE | JOB_PARA_BORDER_TYPE \
                                    | JOB_PARA_TAR_XY_TYPE | JOB_PARA_TAR_DIM_TYPE)

#define OSG_BLITTER_MUST_PAPAMETER  (JOB_PARA_DRAWING_TYPE | JOB_PARA_BLIT_XY_TYPE \
                                     | JOB_PARA_BLIT_DIM_TYPE | JOB_PARA_BLIT_BGCL_TYPE)

#define OSG_GOBEL_PROPERTY_PARAM     (JOB_PARA_SRCBG_DIM_TYPE | JOB_PARA_SRCFMT_TYPE \
                                      | JOB_PARA_SRCDIM_TYPE | JOB_PARA_SRCXY_TYPE)
/*private struct definition*/
struct osg_property_param {
     struct video_property_t *property ;
     unsigned char property_mask[MAX_PROPERTYS];
};

/* A map table used to be query
 */
enum property_id {
     ID_OSG_MODE          = (MAX_WELL_KNOWN_ID_NUM +1),
     ID_OSG_BORDER        ,
     ID_OSG_BLENDING      ,
     ID_OSG_TARGET_XY     ,  
     ID_OSG_TARGET_DIM    ,
     ID_OSG_BLIT_DIM      ,
     ID_OSG_BLIT_BGCOLOR  ,
     ID_OSG_BLIT_XY       ,
     ID_OSG_BLIT_BLEND
};


struct property_map_t property_map[] = {
    /*globle property*/
    {ID_SRC_FMT, "src_fmt", "YUV422_FIELDS, YUV422_FRAME, YUV422_TWO_FRAMES"},
    {ID_SRC_DIM, "src_dim", "source (width << 16 | height)"},
    {ID_SRC_XY, "src_xy", "source (x << 16 | y)"},
    {ID_SRC_BG_DIM, "src_bg_dim", "the source background dimension, it is used for 2D purpose"},
    /*drawing property*/
    {ID_OSG_MODE, "osg_mode", "mode (Reserved<<16 | align << 8 | mode(0x1 is rect ,0x2 blitter)"},
    {ID_OSG_BORDER,"osg_border","border (Reserved<<24 | palette_idx << 16 | border_width << 8 | type(horrow:2, true:1))"},
    {ID_OSG_BLENDING, "osg_blending", "Reserved<<8 alpha blending (0:0% 1:25% 2:37% 3:50% 4:62% 5:75% 6:87% 7:100%)"},
    {ID_OSG_TARGET_XY,"osg_target_xy", "target (x << 16 | y)"},
    {ID_OSG_TARGET_DIM,"osg_target_dim", "target (width << 16 | height)"},
    /*font blit property*/
    {ID_OSG_BLIT_DIM,"osg_blit_dim", "bliter source (width << 16 | height)"},
    {ID_OSG_BLIT_BGCOLOR,"osg_blit_bcolor", "background color (UYVY U<<24|Y<<16|V<<8|Y)"},
    {ID_OSG_BLIT_XY,"osg_blit_xy", "bliter destination xy (x << 16 | y)"},
    {ID_OSG_BLIT_BLEND,"osg_blit_blend", "Reserved<<8 BLIT alpha blending (0:0% 1:25% 2:37% 3:50% 4:62% 5:75% 6:87% 7:100%)"},
    {ID_NULL, "",""},
};

//#define OSG_GET_Y_PALETEE(idx)       (g_osg_palette_default[idx]&0xFF)
//#define OSG_GET_U_PALETEE(idx)       ((g_osg_palette_default[idx]&0xFF00)>>8)
//#define OSG_GET_V_PALETEE(idx)       ((g_osg_palette_default[idx]&0xFF0000)>>16)
#define OSG_GET_PALETEE(idx)           (g_osg_palette_default[idx])
static u32 g_osg_palette_default[OSG_PALETTE_MAX] = {
    OSG_PALETTE_COLOR_WHITE,
    OSG_PALETTE_COLOR_BLACK ,
    OSG_PALETTE_COLOR_RED,
    OSG_PALETTE_COLOR_BLUE,
    OSG_PALETTE_COLOR_YELLOW,
    OSG_PALETTE_COLOR_GREEN,
    OSG_PALETTE_COLOR_BROWN,
    OSG_PALETTE_COLOR_DODGERBLUE,
    OSG_PALETTE_COLOR_GRAY,
    OSG_PALETTE_COLOR_KHAKI,
    OSG_PALETTE_COLOR_LIGHTGREEN,
    OSG_PALETTE_COLOR_MAGENTA,
    OSG_PALETTE_COLOR_ORANGE,
    OSG_PALETTE_COLOR_PINK,
    OSG_PALETTE_COLOR_SLATEBLUE,
    OSG_PALETTE_COLOR_AQUA
};

static unsigned int g_osg_vg_alpha[OSG_VG_ALPHA_MAX] = {
    0,
    25,
    37,
    50,
    62,
    75,
    87,
    100    
};

/*
 * Function Definition 
 *
 */
 
int is_print(int engine,int minor)
{
    int i;

    if ( include_filter_idx[0] >= 0) {
        for (i = 0; i < MAX_FILTER; i++)
            if (include_filter_idx[i] == ((engine << 16) | minor))
                return 1;
    }

    if (exclude_filter_idx[0] >= 0) {
        for (i = 0; i < MAX_FILTER; i++)
            if (exclude_filter_idx[i] == ((engine << 16) | minor))
                return 0;
    }
    return 1;
}

/*function definition*/
static void osg_swap(int *aPtr, int *bPtr)
{
    int temp;
    temp = *aPtr;
    *aPtr = *bPtr;
    *bPtr = temp;
} 
static int osg_draw_line_true_frame(struct __osg_point* startPoint
                                   ,struct __osg_point* endPoint
                                   ,osg_param_t *para, struct __osg_rect * rect_tm)
{
    struct __osg_surface    *dst_suf = &para->dst_suf;
    struct __osg_surface    *bg_suf = &para->bg_suf;
    unsigned char* imgdata = (unsigned char*)bg_suf->t_addr;
    int            pitch,x,y,x0,y0,x1,y1;
    unsigned int* tmp_imgdata; 
    unsigned int  d_yuv;
    
    DEBUG_M(LOG_DEBUG,0,para->chan_id,"%s a=%x,wh=%dx%d,s=%dx%d,e=%dx%d pidx=%d\n",__func__,(unsigned \
    int)imgdata,dst_suf->width,dst_suf->height,startPoint->x,startPoint->y,endPoint->x,endPoint->y,rect_tm->palette_idx); 

    if (!imgdata || !rect_tm || !startPoint ||!endPoint)                                                                           
    {     
        printk("%s %d\n",__func__,__LINE__);
        return -1;                                                                          
    }
  
    if (dst_suf->width < 0 || dst_suf->height < 0 )                                                           
    { 
        printk("%s %d\n",__func__,__LINE__);
        return -1;                                                                          
    }
    
    if (startPoint->x < 0 || startPoint->x > (dst_suf->x + dst_suf->width) 
        || startPoint->y < 0 || startPoint->y > (dst_suf->y + dst_suf->height) 
        || endPoint->x < 0 || endPoint->x > (dst_suf->x +dst_suf->width)  
        || endPoint->y < 0 || endPoint->y > (dst_suf->y +dst_suf->height))   
    { 
         printk("%s %d\n",__func__,__LINE__);
        DEBUG_M(LOG_WARNING,0,para->chan_id,"osg:%s drawline range error s=%dx%d e=%dx%d wh=%dx%d\n",__func__, \
        startPoint->x,startPoint->y,endPoint->x,endPoint->y, \
        (dst_suf->x +dst_suf->width) ,(dst_suf->y +dst_suf->height));                                         
        return -1;                                                                          
    }                                                                                       

    if(rect_tm->palette_idx >= OSG_PALETTE_MAX){
         printk("%s %d\n",__func__,__LINE__);
        DEBUG_M(LOG_WARNING,0,para->chan_id,"osg:%s palette index is over %d\n",__func__,rect_tm->palette_idx);
        return -1;
    }  

    if( startPoint->x > endPoint->x || startPoint->y != endPoint->y){
         printk("%s %d\n",__func__,__LINE__);
        DEBUG_M(LOG_WARNING,0,para->chan_id,"osg:%s start %dx%d and end %dx%d point error\n",__func__, \
        startPoint->x,startPoint->y,endPoint->x,endPoint->y);
        return -1;        
    }
        
    pitch = bg_suf->width *2;
    x0 = startPoint->x, x1 = endPoint->x;                                                 
    y0 = startPoint->y, y1 = endPoint->y;                                       
                                        
    y = y0;   


    for (x = x0; x <= x1; ) //x++)                                                           
    {
        if((x%2) == 0)
            tmp_imgdata =(unsigned int*) &imgdata[y*pitch + x*2];
        else
            tmp_imgdata =(unsigned int*) &imgdata[(y*pitch + x*2)-2];

        
        d_yuv = (*tmp_imgdata);
        (*tmp_imgdata) =(((d_yuv >> 1) & 0xff000000) + (((OSG_GET_PALETEE(rect_tm->palette_idx)) >> 1)& 0xff000000))| 
                        ((((d_yuv & 0x00ff0000) >> 1 )& 0x00ff0000 ) + 
                        (((OSG_GET_PALETEE(rect_tm->palette_idx)&0x00ff0000) >> 1) & 0x00ff0000)) |
                        ((((d_yuv & 0x0000ff00) >> 1) & 0x0000ff00 ) + 
                        (((OSG_GET_PALETEE(rect_tm->palette_idx)&0x0000ff00) >> 1) & 0x0000ff00)) |
                        ((((d_yuv & 0x000000ff) >> 1) & 0x000000ff) +
                        (((OSG_GET_PALETEE(rect_tm->palette_idx)&0x000000ff) >> 1) & 0x000000ff)) ;


        if((x % 2) == 1 && x == x0)
            x++;
        else
            x+=2;
    }
    return 0;

}

static int osg_draw_line_one_frame(struct __osg_point* startPoint
                                   ,struct __osg_point* endPoint
                                   ,osg_param_t *para, struct __osg_rect * rect_tm)    
{    
    struct __osg_surface    *dst_suf = &para->dst_suf;
    struct __osg_surface    *bg_suf = &para->bg_suf;
    unsigned char* imgdata = (unsigned char*)bg_suf->t_addr;
    //unsigned char  yuv_temp,yuv_temp1; 
    int            pitch,x,y,x0,y0,x1,y1,dx,dy,steep,alpha = 255;
    unsigned int* tmp_imgdata; 
    unsigned int  d_yuv;
    
    DEBUG_M(LOG_DEBUG,0,para->chan_id,"%s a=%x,wh=%dx%d,s=%dx%d,e=s=%dx%d pidx=%d\n",__func__,(unsigned \
    int)imgdata,dst_suf->width,dst_suf->height,startPoint->x,startPoint->y,endPoint->x,endPoint->y,rect_tm->palette_idx); 

    if (!imgdata || !rect_tm || !startPoint ||!endPoint)                                                                           
    {                                                                                       
        return -1;                                                                          
    }
  
    if (dst_suf->width < 0 || dst_suf->height < 0 )                                                           
    {                                                                                       
        return -1;                                                                          
    }
    
    if (startPoint->x < 0 || startPoint->x > (dst_suf->x + dst_suf->width) 
        || startPoint->y < 0 || startPoint->y > (dst_suf->y + dst_suf->height) 
        || endPoint->x < 0 || endPoint->x > (dst_suf->x +dst_suf->width)  
        || endPoint->y < 0 || endPoint->y > (dst_suf->y +dst_suf->height))   
    {                                                                                       
        DEBUG_M(LOG_WARNING,0,para->chan_id,"osg:%s drawline range error s=%dx%d e=%dx%d wh=%dx%d\n",__func__, \
        startPoint->x,startPoint->y,endPoint->x,endPoint->y, \
        (dst_suf->x +dst_suf->width) ,(dst_suf->y +dst_suf->height));                                         
        return -1;                                                                          
    }                                                                                       

    if(rect_tm->palette_idx >= OSG_PALETTE_MAX){
        DEBUG_M(LOG_WARNING,0,para->chan_id,"osg:%s palette index is over %d\n",__func__,rect_tm->palette_idx);
        return -1;
    }                                                                                            
    pitch = bg_suf->width *2;
    x0 = startPoint->x, x1 = endPoint->x;                                                 
    y0 = startPoint->y, y1 = endPoint->y;                                                 
    dy = abs(y1 - y0);                                                                  
    dx = abs(x1 - x0);                                                                  
    steep = dy>dx ?1:0; 
    //printk("XXXXa=%x,wh=%dx%d,s=%dx%d,e=s=%dx%d pidx=%d\n",(unsigned \int)imgdata,dst_suf->width,dst_suf->height,x0,y0,x1,y1,rect_tm->palette_idx);                                                                                        
    if (steep)                                                                              
    {                                                                                       
        osg_swap(&x0, &y0);                                                                       
        osg_swap(&x1, &y1);                                                                       
    }  
    
    if (x0 > x1)                                                                            
    {                                                                                       
        osg_swap(&x0, &x1);                                                                       
        osg_swap(&y0, &y1);                                                                       
    } 
                                          
    y = y0;
    
    if(rect_tm->is_blending)
        alpha = rect_tm->alpha;
                                                          
    //printk("steep=%d,width=%d,height=%d\n",steep,width,height);
    
    for (x = x0; x <= x1; ) //x++)                                                           
    {                                                                                       
        if (steep)                                                                          
        {  
            if((y%2) == 0)
                tmp_imgdata =(unsigned int*) &imgdata[x*pitch + y*2];
            else
                tmp_imgdata =(unsigned int*) &imgdata[(x*pitch + y*2)-2];

            if(!rect_tm->is_blending){
              (*tmp_imgdata) = OSG_GET_PALETEE(rect_tm->palette_idx);
            }
            else{
                d_yuv = (*tmp_imgdata);
                (*tmp_imgdata) =(((d_yuv >> 1) & 0xff000000) + (((OSG_GET_PALETEE(rect_tm->palette_idx)) >> 1)& 0xff000000))| 
                        ((((d_yuv & 0x00ff0000) >> 1 )& 0x00ff0000 ) + 
                        (((OSG_GET_PALETEE(rect_tm->palette_idx)&0x00ff0000) >> 1) & 0x00ff0000)) |
                        ((((d_yuv & 0x0000ff00) >> 1) & 0x0000ff00 ) + 
                        (((OSG_GET_PALETEE(rect_tm->palette_idx)&0x0000ff00) >> 1) & 0x0000ff00)) |
                        ((((d_yuv & 0x000000ff) >> 1) & 0x000000ff) +
                        (((OSG_GET_PALETEE(rect_tm->palette_idx)&0x000000ff) >> 1) & 0x000000ff)) ;              
            }
                
            x++;             
           
        }                                                                                   
        else                                                                                
        {

            if((x%2) == 0)
                tmp_imgdata =(unsigned int*) &imgdata[y*pitch + x*2];
            else
                tmp_imgdata =(unsigned int*) &imgdata[(y*pitch + x*2)-2];

            

            if(!rect_tm->is_blending){
              (*tmp_imgdata) = OSG_GET_PALETEE(rect_tm->palette_idx);
            }
            else{
                d_yuv = (*tmp_imgdata);
                (*tmp_imgdata) =(((d_yuv >> 1) & 0xff000000) + (((OSG_GET_PALETEE(rect_tm->palette_idx)) >> 1)& 0xff000000))| 
                        ((((d_yuv & 0x00ff0000) >> 1 )& 0x00ff0000 ) + 
                        (((OSG_GET_PALETEE(rect_tm->palette_idx)&0x00ff0000) >> 1) & 0x00ff0000)) |
                        ((((d_yuv & 0x0000ff00) >> 1) & 0x0000ff00 ) + 
                        (((OSG_GET_PALETEE(rect_tm->palette_idx)&0x0000ff00) >> 1) & 0x0000ff00)) |
                        ((((d_yuv & 0x000000ff) >> 1) & 0x000000ff) +
                        (((OSG_GET_PALETEE(rect_tm->palette_idx)&0x000000ff) >> 1) & 0x000000ff)) ;           
            }
 
            
            if((x % 2) == 1 && x == x0)
            {
               x++;
               //printk("x++ =%x\n",x);
            }
            else
                x+=2;               

        }                                                                     
                                                                
    }/*END  for (x = x0; x < x1; x++)   */                                                                                       
    return 0;                                                                               
} 

int osg_vg_check_border(struct __osg_rect* rect,int border_size)
{
    int x1 = rect->x;
    int y1 = rect->y;
    int x2 = rect->x + rect->w - 1;
    int y2 = rect->y + rect->h - 1;

    if(y1 + border_size > y2 - border_size)
        return -1;
    if(x1 + border_size > x2 - border_size)
        return -1;  

    return 0;
    
}

static int osg_vg_process_blit(osg_job_t *job,struct __osg_rect * rect_tm)    
{
    osg_param_t *tmp_para =(osg_param_t*) &job->param;
    struct __osg_surface    *dst_suf = &tmp_para->dst_suf;
    struct __osg_surface    *bg_suf = &tmp_para->bg_suf;
    unsigned char* imgdata = (unsigned char*)bg_suf->t_addr;
    unsigned int dx,dy,dw,dh,dpitch,spitch,d_yuv,s_yuv;
    unsigned int *dst_data ,*src_data;
    int i,j;

    if (!job || !tmp_para || !rect_tm)                                                                           
    {                                                                                       
        return -1;                                                                          
    }

    if (tmp_para->dst_suf.width<= 0 || tmp_para->dst_suf.height <= 0 )                                                           
    {                                                                                       
        return -1;                                                                          
    }

    if((rect_tm->x + rect_tm->w) > tmp_para->dst_suf.width  || (rect_tm->y  + rect_tm->h) > tmp_para->dst_suf.height)
    {
        DEBUG_M(LOG_WARNING,0,job->chan_id,"osg:blit param is error\n");
        return -1;
    }
    
    dx = dst_suf->x+rect_tm->x;
    dy = dst_suf->y+rect_tm->y;
    dw = rect_tm->w;
    dh = rect_tm->h;
    dpitch = bg_suf->width*2;
    spitch = rect_tm->w*2;
    
    if(dx%2 == 1){
        if(dx - 1 < dst_suf->x){
            dx+=1;
            dw-=1;
        }
        else
            dx-=1;            
    }

    if((dx+dw) > (dst_suf->x+dst_suf->width) || (dy+dh) > (dst_suf->y+dst_suf->height)){

        DEBUG_M(LOG_WARNING,0,job->chan_id,"osg:blit range is error\n");
        return -1;
    }

     for(i = 0 ; i < dh ; i++){
        for(j = 0 ; j < dw ; j+=2){
            dst_data = (unsigned int*)&imgdata[((dy+i)*dpitch)+((dx+j)*2)];
            src_data = (unsigned int*)&imgdata[(i*spitch)+(j*2)];
            
            if((*src_data) == rect_tm->blit_color)
                continue;

            if(!rect_tm->is_blending){
                (*dst_data) = (*src_data);
            }
            else{
                d_yuv = (*dst_data);
                s_yuv = (*src_data);
                (*dst_data) =(((d_yuv >> 1) & 0xff000000) + ((s_yuv >> 1)& 0xff000000))| 
                        ((((d_yuv & 0x00ff0000) >> 1 )& 0x00ff0000 ) + 
                        (((s_yuv&0x00ff0000) >> 1) & 0x00ff0000)) |
                        ((((d_yuv & 0x0000ff00) >> 1) & 0x0000ff00 ) + 
                        (((s_yuv&0x0000ff00) >> 1) & 0x0000ff00)) |
                        ((((d_yuv & 0x000000ff) >> 1) & 0x000000ff) +
                        (((s_yuv&0x000000ff) >> 1) & 0x000000ff)) ;              
            }
            
        }
     }
     return 0;
}


static int osg_vg_process_rect(osg_job_t *job,struct __osg_rect * rect_tm)    
{
    osg_param_t *tmp_para =(osg_param_t*) &job->param;
    int x1,y1,x2,y2,i,border_s = 0;
    struct __osg_point startPoint ,endpoint;

    if (!job || !tmp_para || !rect_tm)                                                                           
    {                                                                                       
        return -1;                                                                          
    }

    if (tmp_para->dst_suf.width<= 0 || tmp_para->dst_suf.height <= 0 )                                                           
    {                                                                                       
        return -1;                                                                          
    }

    if((rect_tm->x + rect_tm->w) > tmp_para->dst_suf.width  || (rect_tm->y  + rect_tm->h) > tmp_para->dst_suf.height)
    {
        DEBUG_M(LOG_WARNING,0,job->chan_id,"osg:rect param is error\n");
        return -1;
    }

    if(osg_vg_check_border(rect_tm,rect_tm->border_size) < 0){
        DEBUG_M(LOG_WARNING,0,job->chan_id,"osg:check border is over range\n");
        return -1;
    }
    
    if(rect_tm->border_type == OSG_BORDER_TRUE_TYPE){
        x1 = tmp_para->dst_suf.x + rect_tm->x+rect_tm->border_size;
        y1 = tmp_para->dst_suf.y + rect_tm->y+rect_tm->border_size;
        x2 = (x1+ rect_tm->w - 1)- rect_tm->border_size;
        y2 = (y1 + rect_tm->h - 1)- rect_tm->border_size;  
        border_s = y2-y1 +1;
    }
    else if(rect_tm->border_type == OSG_BORDER_HORROW_TYPE){
        x1 = tmp_para->dst_suf.x + rect_tm->x ,y1 = tmp_para->dst_suf.y + rect_tm->y;
        x2 = x1 + rect_tm->w - 1 , y2 = y1 + rect_tm->h - 1;
        border_s = (rect_tm->border_size == 0) ? 1: rect_tm->border_size;            
    }
    else{
        rect_tm->border_type = OSG_BORDER_HORROW_TYPE;
        x1 = tmp_para->dst_suf.x + rect_tm->x ,y1 = tmp_para->dst_suf.y + rect_tm->y;
        x2 = x1 + rect_tm->w - 1 , y2 = y1 + rect_tm->h - 1;
        border_s = 1; 
    }
    DEBUG_M(LOG_DEBUG,0,job->chan_id,"%s id=%d rect xy=%dx%d wh=%dx%d add=%x size=%x type=%d\n",__func__,job->jobid, x1, \
            y1,x2,y2,(unsigned int)tmp_para->dst_suf.t_addr,border_s,rect_tm->border_type);

    for( i = 0 ; i < border_s ; i++){
        // horizontal line at top
        if( rect_tm->border_type == OSG_BORDER_HORROW_TYPE){
            startPoint.x = x1 , startPoint.y = y1+i;
            endpoint.x = x2 , endpoint.y = y1+i;
            if(tmp_para->frame_type == FRAME_TYPE_YUV422FRAME){
                if(osg_draw_line_one_frame(&startPoint,&endpoint,tmp_para,rect_tm)<0){
                    DEBUG_M(LOG_ERROR,0,job->chan_id,"osg:osg_draw_line_one_frame is error %d\n",__LINE__);
                    return -1;
                }
            }
            else{
                DEBUG_M(LOG_WARNING,0,job->chan_id,"osg:frame type isn't supported yet\n");
                return -1;
            }
        }else if(rect_tm->border_type == OSG_BORDER_TRUE_TYPE){
            startPoint.x = x1 , startPoint.y = y1+i;
            endpoint.x = x2 , endpoint.y = y1+i;
            if(tmp_para->frame_type == FRAME_TYPE_YUV422FRAME){
                if(osg_draw_line_true_frame(&startPoint,&endpoint,tmp_para,rect_tm)<0){
                    DEBUG_M(LOG_ERROR,0,job->chan_id,"osg:osg_draw_line_true_frame is error %d\n",__LINE__);
                    return -1;
                }
            }
            else{
                DEBUG_M(LOG_WARNING,0,job->chan_id,"osg:frame type isn't supported yet\n");
                return -1;
            }

        }
           
        // left and right
        if(rect_tm->border_type == OSG_BORDER_HORROW_TYPE){
            startPoint.x = x1+i , startPoint.y = y1+border_s;
            endpoint.x = x1+i , endpoint.y = y2-border_s;
            if(tmp_para->frame_type == FRAME_TYPE_YUV422FRAME){
                if(osg_draw_line_one_frame(&startPoint,&endpoint,tmp_para,rect_tm)<0){
                    DEBUG_M(LOG_ERROR,0,job->chan_id,"osg:osg_draw_line_one_frame is error %d\n",__LINE__);
                    return -1;
                }
            }
            else{
                DEBUG_M(LOG_WARNING,0,job->chan_id,"osg:frame type isn't supported yet\n");
                return -1;
            }
            
            startPoint.x = x2-i , startPoint.y = y1+border_s;
            endpoint.x = x2-i , endpoint.y = y2-border_s;
            if(tmp_para->frame_type == FRAME_TYPE_YUV422FRAME){
                if(osg_draw_line_one_frame(&startPoint,&endpoint,tmp_para,rect_tm)<0){
                    DEBUG_M(LOG_ERROR,0,job->chan_id,"osg:osg_draw_line_one_frame is error %d\n",__LINE__);
                    return -1;
                }
            }
            else{
                DEBUG_M(LOG_WARNING,0,job->chan_id,"osg:frame type isn't supported yet\n");
                return -1;
            }
        }
        // horizontal line at bottom
        if(rect_tm->border_type == OSG_BORDER_HORROW_TYPE){
            startPoint.x = x1 , startPoint.y = y2-i;
            endpoint.x = x2 , endpoint.y = y2-i;
            if(tmp_para->frame_type == FRAME_TYPE_YUV422FRAME){
                if(osg_draw_line_one_frame(&startPoint,&endpoint,tmp_para,rect_tm)<0){
                    DEBUG_M(LOG_ERROR,0,job->chan_id,"osg:osg_draw_line_one_frame is error %d\n",__LINE__);
                    return -1;
                }
            }
            else{
                DEBUG_M(LOG_WARNING,0,job->chan_id,"osg:frame type isn't supported yet\n");
                return -1;
            }
        }
    }
    return 0;
}

int osg_vg_process_job(osg_job_t   *job)
{
    struct __osg_rect * rect_tm;
    unsigned long       flags;
    osg_param_t *tmp_para ;
    //int i;
    if (!job)                                                                           
    {   
        DEBUG_M(LOG_ERROR,0,job->chan_id,"%s job is NULL\n",__func__);
        return -1;                                                                          
    }
    
    tmp_para =(osg_param_t*) &job->param;
    DRV_PARAMLOCK(flags);
    list_for_each_entry(rect_tm, &tmp_para->dst_rect_list,list) {     
        if(rect_tm->drawing_type == DRAWING_RECT_JOB){
           // for(i=0;i<1;i++){
            if(osg_vg_process_rect(job,rect_tm) < 0){
                DEBUG_M(LOG_ERROR,0,job->chan_id,"osg:osg_vg_process_rect error\n");
                DRV_PARAMUNLOCK(flags);
                return -1;
            }
           //     }
            
        }
        else if(rect_tm->drawing_type == BLITTER_JOB){
           if(osg_vg_process_blit(job,rect_tm) < 0){
                DEBUG_M(LOG_ERROR,0,job->chan_id,"osg:osg_vg_process_rect error\n");
                DRV_PARAMUNLOCK(flags);
                return -1;
            }
        }
        else{
            DEBUG_M(LOG_WARNING,0,job->chan_id,"osg:drawing type is not support %d %d\n",__LINE__,rect_tm->drawing_type);
            DRV_PARAMUNLOCK(flags);
            return -1;
        }         
    }
    DRV_PARAMUNLOCK(flags);
    return 0;    
}
int osg_vg_panic_notifier(int data)
{
    printm(MODULE_NAME, "PANIC!! Processing Start\n");
  

    printm(MODULE_NAME, "PANIC!! Processing End\n");
    return 0;
}

int osg_vg_print_notifier(int data)
{
    unsigned long flags;
    struct video_entity_job_t *job;
    osg_job_t *node;
    char *st_string[] = {"QUEUE", "ONGOING", " FINISH", "FLUSH"}, *string = NULL;
   
    DRV_VGLOCK(flags);
    list_for_each_entry(node, &g_info.fire_job_list, list) {
        if (node->status & JOB_STS_QUEUE)   string = st_string[0];
        if (node->status & JOB_STS_ONGOING) string = st_string[1];
        if (node->status & JOB_STS_DONE)    string = st_string[2];
        if (node->status & JOB_STS_FLUSH)   string = st_string[3];
        job = (struct video_entity_job_t *)node->private;
        printm(MODULE_NAME,"chan:%d  jobid:%d  stats:%s  pt:0x%04x  st:0x%04x  ft:0x%04x \n", node->chan_id, job->id, string,
            node->puttime, node->starttime, node->finishtime);
    }

    list_for_each_entry(node, &g_info.going_job_list, list) {
        if (node->status & JOB_STS_QUEUE)   string = st_string[0];
        if (node->status & JOB_STS_ONGOING) string = st_string[1];
        if (node->status & JOB_STS_DONE)    string = st_string[2];
        if (node->status & JOB_STS_FLUSH)   string = st_string[3];
        job = (struct video_entity_job_t *)node->private;
        printm(MODULE_NAME,"chan:%d  jobid:%d  stats:%s  pt:0x%04x  st:0x%04x  ft:0x%04x \n", node->chan_id, job->id, string,
            node->puttime, node->starttime, node->finishtime);
    }

    list_for_each_entry(node, &g_info.finish_job_list, list) {
        if (node->status & JOB_STS_QUEUE)   string = st_string[0];
        if (node->status & JOB_STS_ONGOING) string = st_string[1];
        if (node->status & JOB_STS_DONE)    string = st_string[2];
        if (node->status & JOB_STS_FLUSH)   string = st_string[3];
        job = (struct video_entity_job_t *)node->private;
        printm(MODULE_NAME,"chan:%d  jobid:%d  stats:%s  pt:0x%04x  st:0x%04x  ft:0x%04x \n", node->chan_id, job->id, string,
            node->puttime, node->starttime, node->finishtime);
    }
	DRV_VGUNLOCK(flags);

    return 0;
}

static void print_property(struct video_entity_job_t *job,struct video_property_t *property)
{
    int i;
    int engine = ENTITY_FD_ENGINE(job->fd);
    int minor = ENTITY_FD_MINOR(job->fd);

    for (i = 0; i < MAX_PROPERTYS; i++) {
        if (property[i].id == ID_NULL)
            break;
        if (i == 0)
            DEBUG_M(LOG_DEBUG, engine, minor,"{%d,%d} job %d property:", engine, minor, job->id);
        DEBUG_M(LOG_DEBUG, engine, minor,"ch:%d  ID:%d,Value:%d\n",property[i].ch,property[i].id, property[i].value);
    }
}
/*
 * osg_assign_frame_buffer
 */

static int osg_assign_frame_buffer(osg_param_t* param,struct video_entity_job_t *job)
{
    int ret = 0;
    u32  line_ofs, frame_ofs;
    
    if(param == NULL)
        return -1; 
    
    if(param->bg_suf.width <= 0  || param->bg_suf.height <= 0){
        printk("osg:BG surface WxH is error\n");
        return -1;
    }   

    frame_ofs = (param->bg_suf.width * param->bg_suf.height) << 1;  //yuv422
    line_ofs  = param->bg_suf.width << 1;    //yuv422    
   

    if(param->frame_type == FRAME_TYPE_YUV422FRAME){
        param->bg_suf.t_addr = job->in_buf.buf[0].addr_va;
        param->dst_suf.t_addr = job->in_buf.buf[0].addr_va +(param->dst_suf.y*line_ofs)+(param->dst_suf.x*2); 
        DEBUG_M(LOG_DEBUG, 0, param->chan_id,"%s bg=%x ,dst=%x ,%dx%d\n",__func__,(unsigned int)param->bg_suf.t_addr,(unsigned int)param->dst_suf.t_addr,param->dst_suf.x,param->dst_suf.y);
    }
    else if(param->frame_type == FRAME_TYPE_2FRAMES){
        DEBUG_M(LOG_WARNING, 0, param->chan_id,"osg:doesn't support FRAME_TYPE_2FRAMES\n");
        ret = -1;
        //param->bg_suf.t_addr =  job->in_buf.buf[0].addr_va;
        //param->bg_suf.b_addr =  job->in_buf.buf[0].addr_va + frame_ofs + line_ofs;
        //param->dst_suf.t_addr = job->in_buf.buf[0].addr_va +(param->dst_suf.y*line_ofs)+(param->dst_suf.x*2);
        //param->dst_suf.b_addr = job->in_buf.buf[0].addr_va + frame_ofs + ((param->dst_suf.y+1)*line_ofs)+((param->dst_suf.x)*2);
    }
    else if(param->frame_type == FRAME_TYPE_2FIELDS){

        DEBUG_M(LOG_WARNING, 0, param->chan_id,"osg:doesn't support FRAME_TYPE_2FIELDS\n");
        ret = -1;
        //param->dst_suf.t_addr = job->in_buf.buf[0].addr_va;
        //param->dst_suf.b_addr = job->in_buf.buf[0].addr_va + frame_ofs + line_ofs;
    }
    else{
        DEBUG_M(LOG_WARNING, 0, param->chan_id,"osg:assign frame buffer type error\n");
        ret = -1;
    }

    return ret;
}

/*
 * osg_get_next_target_param
 */

static int osg_get_next_target_param(int ch ,struct osg_property_param * osg_pro_p,osg_param_t *param)
{
    struct __osg_rect *rect_tmp ;
    //int    finished = 0;
    unsigned int      para_supported = 0;
    unsigned long     flags;
    int     i= 0 ;

    if(!osg_pro_p)
        return -1;

    rect_tmp = (struct __osg_rect*)kmem_cache_alloc(g_info.job_param_cache, GFP_KERNEL);

    if (rect_tmp == NULL) {
        DEBUG_M(LOG_ERROR, 0, param->chan_id,"osg:allocate rect param fail %d\n",__LINE__);
        return -1;
    }
    
    DRV_VGLOCK(flags);
    g_info.alloc_param_count++;
    DRV_VGUNLOCK(flags);

    while (osg_pro_p->property[i].id != 0) {

        if(osg_pro_p->property_mask[i]){
            i++;
            continue;
        }
        //printk("osg_pro_p->property[i].ch =%d %d",osg_pro_p->property[i].ch,ch);
        if(osg_pro_p->property[i].ch == ch &&
           osg_pro_p->property[i].id != ID_SRC_FMT && osg_pro_p->property[i].id != ID_SRC_DIM &&
           osg_pro_p->property[i].id != ID_SRC_XY && osg_pro_p->property[i].id != ID_SRC_BG_DIM){

            switch(osg_pro_p->property[i].id){
                case ID_OSG_BORDER:
                    rect_tmp->border_size= OSG_GET_BORDER_PARAM_WIDTH(osg_pro_p->property[i].value);
                    rect_tmp->palette_idx= OSG_GET_BORDER_PARAM_PALETTE(osg_pro_p->property[i].value);
                    rect_tmp->border_type= OSG_GET_BORDER_PARAM_TYPE(osg_pro_p->property[i].value);               
                    osg_pro_p->property_mask[i] = 1; 
                    para_supported |= JOB_PARA_BORDER_TYPE;
                    break;
                case ID_OSG_MODE:
                    rect_tmp->drawing_type = OSG_GET_MODE_PARAM_MODE(osg_pro_p->property[i].value);
                    rect_tmp->align_type   = OSG_GET_MODE_PARAM_ALIGN(osg_pro_p->property[i].value);
                    para_supported |= JOB_PARA_DRAWING_TYPE;
                    osg_pro_p->property_mask[i] = 1;                 
                    break;
                case ID_OSG_BLENDING:
                    rect_tmp->is_blending = OSG_GET_BLENDING_PARAM(osg_pro_p->property[i].value) > 0 ?1:0;
                    rect_tmp->alpha       = OSG_GET_BLENDING_PARAM(osg_pro_p->property[i].value) ;
                    osg_pro_p->property_mask[i] = 1; 
                    para_supported |= JOB_PARA_BELNDING_TYPE;
                    break;
                case ID_OSG_TARGET_XY:
                    rect_tmp->x = EM_PARAM_X(osg_pro_p->property[i].value);
                    rect_tmp->y = EM_PARAM_Y(osg_pro_p->property[i].value);
                    osg_pro_p->property_mask[i] = 1;
                    para_supported |= JOB_PARA_TAR_XY_TYPE;
                    break;    
                case ID_OSG_TARGET_DIM:
                    rect_tmp->w = EM_PARAM_WIDTH(osg_pro_p->property[i].value);
                    rect_tmp->h = EM_PARAM_HEIGHT(osg_pro_p->property[i].value);
                    osg_pro_p->property_mask[i] = 1;
                    para_supported |= JOB_PARA_TAR_DIM_TYPE;
                    //finished = 1;
                    break;
                case ID_OSG_BLIT_DIM:
                    rect_tmp->w = EM_PARAM_WIDTH(osg_pro_p->property[i].value);
                    rect_tmp->h = EM_PARAM_HEIGHT(osg_pro_p->property[i].value);
                    osg_pro_p->property_mask[i] = 1;
                    para_supported |= JOB_PARA_BLIT_DIM_TYPE; 
                    break;
                case ID_OSG_BLIT_XY:
                    rect_tmp->x = EM_PARAM_X(osg_pro_p->property[i].value);
                    rect_tmp->y = EM_PARAM_Y(osg_pro_p->property[i].value);
                    osg_pro_p->property_mask[i] = 1;
                    para_supported |= JOB_PARA_BLIT_XY_TYPE;
                    break;
                case ID_OSG_BLIT_BGCOLOR:
                    rect_tmp->blit_color = osg_pro_p->property[i].value;
                    osg_pro_p->property_mask[i] = 1;
                    para_supported |= JOB_PARA_BLIT_BGCL_TYPE;
                    break;
                case ID_OSG_BLIT_BLEND:
                    rect_tmp->is_blending = OSG_GET_BLENDING_PARAM(osg_pro_p->property[i].value) > 0 ?1:0;
                    rect_tmp->alpha       = OSG_GET_BLENDING_PARAM(osg_pro_p->property[i].value) ;
                    osg_pro_p->property_mask[i] = 1; 
                    para_supported |= JOB_PARA_BLIT_BLEND_TYPE;
                default:
                    printk("%s doesn't support property type %d\n",__func__,osg_pro_p->property[i].id);
                    osg_pro_p->property_mask[i] = 1;
                break;
            }           
           
        }
        //if(finished)
        //    break;
        i++;
    }

    if(rect_tmp->align_type >= OSG_WIN_ALIGN_MAX ){
        DEBUG_M(LOG_WARNING, 0, param->chan_id,"osg:align type is error\n");
        goto err;        
    }
    
    if(rect_tmp->drawing_type == DRAWING_RECT_JOB){
        if((para_supported&OSG_RECT_MUST_PAPAMETER)!=OSG_RECT_MUST_PAPAMETER){
            DEBUG_M(LOG_WARNING, 0, param->chan_id,"osg:drawing rectangle param is not enough\n");
            goto err;
        }

        if(rect_tmp->align_type > OSG_WIN_ALIGN_TOP_LEFT ){
            DEBUG_M(LOG_WARNING, 0, param->chan_id,"osg:Rectangle align type isn't supported\n");
            goto err;
        }            
    
    }
    else if(rect_tmp->drawing_type == BLITTER_JOB){
        if((para_supported&OSG_BLITTER_MUST_PAPAMETER)!=OSG_BLITTER_MUST_PAPAMETER){
            DEBUG_M(LOG_WARNING, 0, param->chan_id,"osg:drawing rectangle param is not enough\n");
            goto err;
        }
        
        if(rect_tmp->align_type > OSG_WIN_ALIGN_TOP_LEFT ){
            DEBUG_M(LOG_WARNING, 0, param->chan_id,"osg:Rectangle align type isn't supported\n");
            goto err;
        }     
        
        if(!(para_supported & JOB_PARA_BLIT_BLEND_TYPE))
            rect_tmp->is_blending = 0;   
        
    }
    else{
        DEBUG_M(LOG_WARNING, 0, param->chan_id,"osg:unknow drawing type\n");
        goto err;    
    } 
    
    if(para_supported&JOB_PARA_BORDER_TYPE){
        if(rect_tmp->palette_idx >= OSG_PALETTE_MAX){
            DEBUG_M(LOG_WARNING, 0, param->chan_id,"osg:palette index is over %d\n",rect_tmp->palette_idx);
            goto err;    
        }
    }

    if(para_supported&JOB_PARA_BELNDING_TYPE){
        if(rect_tmp->is_blending){
            if(rect_tmp->alpha >= OSG_VG_ALPHA_MAX){
                DEBUG_M(LOG_WARNING, 0, param->chan_id,"osg:ALPHA index is over %d\n",rect_tmp->alpha);
                goto err;  
            }
            rect_tmp->alpha = 255 - (255 * g_osg_vg_alpha[rect_tmp->alpha]/100);
        }
    }
    DRV_PARAMLOCK(flags);
    INIT_LIST_HEAD(&rect_tmp->list);
    DEBUG_M(LOG_DEBUG, 0, param->chan_id,"osg:%s %x %x %x\n",__func__,(unsigned int)param ,(unsigned int) &param->dst_rect_list,(unsigned int)rect_tmp);
    /* fill up the input parameters */
    list_add_tail(&rect_tmp->list, &param->dst_rect_list);
    DRV_PARAMUNLOCK(flags); 
    return 0;
err :
    kmem_cache_free(g_info.job_param_cache, rect_tmp);
    DRV_VGLOCK(flags);
	g_info.alloc_param_count--;
    DRV_VGUNLOCK(flags);
    return -1;
    
   
}

/*
 */
static int driver_parse_in_property(void *parm, struct video_entity_job_t *job)
{
    unsigned int i = 0 ,src_fmt;
    unsigned int para_supported = 0 ;
    struct osg_property_param pro_para;
    osg_param_t *param = (osg_param_t *)parm;

    memset(&pro_para,0x0,sizeof(struct osg_property_param));
    pro_para.property = job->in_prop;
    
    INIT_LIST_HEAD(&param->dst_rect_list);
    DEBUG_M(LOG_DEBUG, 0, param->chan_id,"osg:%s %x %x\n",__func__,(unsigned int)param ,(unsigned int) &param->dst_rect_list);
    /* fill up the input parameters */
    while (pro_para.property[i].id != 0) {

        if(pro_para.property_mask[i]){
            i++;
            continue;
        }

        switch(pro_para.property[i].id) {
          case ID_SRC_BG_DIM:
            if(pro_para.property[i].ch == 0){
                param->bg_suf.x = 0;
                param->bg_suf.y = 0;
                param->bg_suf.width= EM_PARAM_WIDTH(pro_para.property[i].value);
                param->bg_suf.height = EM_PARAM_HEIGHT(pro_para.property[i].value);
                para_supported |= JOB_PARA_SRCBG_DIM_TYPE;
                pro_para.property_mask[i] = 1;
            }
            break;
          case ID_SRC_FMT: /* chx_src_fmt, TYPE_YUV422_FIELDS, TYPE_YUV422_FRAME, TYPE_YUV422_2FRAMES */
            if(pro_para.property[i].ch == 0){
                param->frame_type = pro_para.property[i].value;
                para_supported |= JOB_PARA_SRCFMT_TYPE;
                pro_para.property_mask[i] = 1;
            }
            break;
          case ID_SRC_DIM:
            if(pro_para.property[i].ch == 0){
                param->dst_suf.width  = EM_PARAM_WIDTH(pro_para.property[i].value);
                param->dst_suf.height = EM_PARAM_HEIGHT(pro_para.property[i].value);
                para_supported |= JOB_PARA_SRCDIM_TYPE;
                pro_para.property_mask[i] = 1; 
            }
            break;  
          case ID_SRC_XY:
            if(pro_para.property[i].ch == 0){
                param->dst_suf.x = EM_PARAM_X(pro_para.property[i].value);
                param->dst_suf.y = EM_PARAM_Y(pro_para.property[i].value);
                para_supported |= JOB_PARA_SRCXY_TYPE;
                pro_para.property_mask[i] = 1; 
            }
            break;
          case ID_OSG_BORDER:
          case ID_OSG_BLENDING:
          case ID_OSG_MODE:
          case ID_OSG_TARGET_XY:           
          case ID_OSG_TARGET_DIM:         
          case ID_OSG_BLIT_DIM:
            {
                if(osg_get_next_target_param(pro_para.property[i].ch,&pro_para,param) < 0)
                {                   
                    DEBUG_M(LOG_DEBUG, 0, param->chan_id,"osg:get next target param error %d\n",pro_para.property[i].id);
                    return -1;                    
                }         
            }
            break;
          default:
            DEBUG_M(LOG_DEBUG, 0, param->chan_id,"%s doesn't support property type %d\n",__func__,pro_para.property[i].id);
            pro_para.property_mask[i] = 1;
            break;
        }
        i++;
    }

    if((para_supported & OSG_GOBEL_PROPERTY_PARAM) != OSG_GOBEL_PROPERTY_PARAM){
        DEBUG_M(LOG_ERROR, 0, param->chan_id,"osg:grobal parameter isn't enough %d\n",__LINE__);
        return -1;                    
    }
    src_fmt = param->frame_type;

    if (src_fmt == TYPE_YUV422_FIELDS){
        param->frame_type = FRAME_TYPE_2FIELDS;
    }

    if (src_fmt == TYPE_YUV422_FRAME || src_fmt == TYPE_YUV422_FRAME_2FRAMES || src_fmt == TYPE_YUV422_RATIO_FRAME){
        param->frame_type = FRAME_TYPE_YUV422FRAME;        
    }    
    if (src_fmt == TYPE_YUV422_2FRAMES || src_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        param->frame_type = FRAME_TYPE_2FRAMES;

    
    if(osg_assign_frame_buffer(param,job) < 0){
        DEBUG_M(LOG_ERROR, 0, param->chan_id,"osg: assign frame buffer error\n");
        return -1;
    }
    
    print_property(job, job->in_prop);
    return 0;
}

/*
 * param: private data
 */
static int driver_set_out_property(void *param, struct video_entity_job_t *job)
{
    int i = 0;
    struct video_property_t *property = job->out_prop;

    if (param)  {}

    property[i].id = ID_NULL;
    property[i].value = 0;    //data->xxxx

    print_property(job, job->out_prop);
    return 0;  
}

static void driver_mark_engine_start(osg_job_t *job_item, void *private)
{
    struct video_entity_job_t   *job = (struct video_entity_job_t *)job_item->private;
    struct video_property_t     *property = job->in_prop;
    int idx = MAKE_IDX(ENTITY_MINORS, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
    int i = 0;

#if 1 /* move from driver_parse_in_property() */
    while (property[i].id != 0) {
        if (i < MAX_RECORD) {
            property_record[idx].job_id = job->id;
            property_record[idx].property[i].id = property[i].id;
            property_record[idx].property[i].value = property[i].value;
        } else {
            DEBUG_M(LOG_ERROR, 0, job_item->chan_id,"Warning! The array is too small, array size is %d! \n", MAX_RECORD);
            break;
        }

        i ++;
    }
#endif

    job_item->starttime = jiffies;
    ENGINE_START(0) = job_item->starttime;
    ENGINE_END(0) = 0;
    if(UTIL_START(0) == 0)
	{
        UTIL_START(0) = ENGINE_START(0);
        ENGINE_TIME(0) = 0;
    }
}

static void driver_mark_engine_finish(osg_job_t *job_item, void *private)
{
    job_item->finishtime = jiffies;

    ENGINE_END(0) = job_item->finishtime;

    if (ENGINE_END(0) > ENGINE_START(0))
        ENGINE_TIME(0) += ENGINE_END(0) - ENGINE_START(0);

    if (UTIL_START(0) > ENGINE_END(0)) {
        UTIL_START(0) = 0;
        ENGINE_TIME(0) = 0;
    } else if ((UTIL_START(0) <= ENGINE_END(0)) &&
        (ENGINE_END(0) - UTIL_START(0) >= UTIL_PERIOD(0) * HZ)) {
        unsigned int utilization;
		if ((ENGINE_END(0) - UTIL_START(0)) == 0)
		{
			OSG_PANIC("%s div by 0!!\n",__func__);
		}
		else
		{
        	utilization = (unsigned int)((100*ENGINE_TIME(0)) / (ENGINE_END(0) - UTIL_START(0)));
		}
        if (utilization)
            UTIL_RECORD(0) = utilization;
        UTIL_START(0) = 0;
        ENGINE_TIME(0) = 0;

    }
    ENGINE_START(0)=0;   
}

/* VideoGraph related function
 */
static int driver_preprocess(osg_job_t *job_item, void *private)
{
    if(job_item == NULL || private == NULL)
        return 0;

    return video_preprocess(job_item->private, NULL);
}

/* VideoGraph related function
 */
static int driver_postprocess(osg_job_t *job_item, void *private)
{
    static int job_counter = 0;
    if(job_item == NULL || private == NULL)
        return 0;
    ++job_counter;

    printk("job %u st=%u,ft=%u,dt=%u\n",job_counter,job_item->starttime,job_item->finishtime 
            ,job_item->finishtime-job_item->starttime);
    printk("job %u pt=%u,ft=%u,dt=%u\n",job_counter,job_item->puttime,job_item->finishtime
            ,job_item->finishtime-job_item->puttime);
    return video_postprocess(job_item->private, NULL);
}

/*
 * release all node from param listst
 */

static void osg_release_param_list(osg_param_t* param)
{
    unsigned long flags;
    struct __osg_rect  *cur_rect;

    if(param == NULL)
        return ;        
    do{
        DRV_PARAMLOCK(flags);
        if(list_empty(&param->dst_rect_list)){
            DRV_PARAMUNLOCK(flags);
            break;
        }
        /* take one from the list */
        cur_rect = list_entry(param->dst_rect_list.next, struct __osg_rect,list);
        list_del_init(&cur_rect->list);
        kmem_cache_free(g_info.job_param_cache, cur_rect);
        DRV_PARAMUNLOCK(flags);

        DRV_VGLOCK(flags);
		g_info.alloc_param_count--;
        DRV_VGUNLOCK(flags);
    }while(1);        
}


/* return the job to the videograph
 */
void osg_callback_process(void)
{
    unsigned long flags;
    osg_job_t   *job_node;
    struct video_entity_job_t * job_entity;
    /* process this channel */
    do {
            DRV_VGLOCK(flags);

            if (list_empty(&g_info.finish_job_list)) {
                DRV_VGUNLOCK(flags);
                break;
            }

            /* take one from the list */
            job_node = list_entry(g_info.finish_job_list.next, osg_job_t, list);
            //printk("osg:callback process id=%d stat=%d %x\n",job_node->jobid,job_node->status,(unsigned int)job_node);
            if (!(job_node->status & JOB_DONE)) {

                printk("osg:callback process handle error status %d\n",job_node->status);
                DRV_VGUNLOCK(flags);
                continue;               
            }

            list_del_init(&job_node->list);
            DEBUG_M(LOG_DEBUG, 0, job_node->chan_id,"osg:del callback process id=%d stat=%d %x\n",job_node->jobid,job_node->status,(unsigned int)job_node);
            atomic_dec(&g_info.list_finish_cnt[job_node->chan_id]);
            DRV_VGUNLOCK(flags);

            osg_release_param_list(&job_node->param);
            DEBUG_M(LOG_DEBUG, 0, job_node->chan_id, "remove finish_job %s, CB, job->id = %d \n", __func__,((struct video_entity_job_t *)job_node->private)->id);
            
            if(job_node->private == NULL){
                printk("osg:callback process private is NULL\n");
                kmem_cache_free(g_info.job_cache, job_node);
                DRV_VGLOCK(flags);
			    g_info.vg_node_count--;
                DRV_VGUNLOCK(flags);
                continue;
            }

            if(job_node->need_to_callback){
                //printk("job_node->need_to_callback\n");
                DEBUG_M(LOG_DEBUG, 0, job_node->chan_id,"osg:job_entity callback process job_type=%d\n" ,job_node->job_type);

                if(job_node->job_type == OSG_JOB_MULTI_TYPE){
                    job_entity = (struct video_entity_job_t *)job_node->parent_entity_job;             
                    job_entity->callback(job_entity);
                }
                else{
                    job_entity = (struct video_entity_job_t *)job_node->private;             
                    job_entity->callback(job_entity);
                }
            }
            
            kmem_cache_free(g_info.job_cache, job_node);
            
            DRV_VGLOCK(flags);
			g_info.vg_node_count--;
            //printk("free job =%d\n",g_info.vg_node_count);
            DRV_VGUNLOCK(flags);
            
    } while(1);
}

/* callback function for job finish. In ISR.
 * We must save the last job for de-interlace reference.
 */
static void driver_callback(osg_job_t  *job_item)
{
    unsigned long flags;
    
    DRV_VGLOCK(flags);
    INIT_LIST_HEAD(&job_item->list);
    list_add_tail(&job_item->list, &g_info.finish_job_list);
    atomic_inc(&g_info.list_finish_cnt[job_item->chan_id]);
    DRV_VGUNLOCK(flags); 
    DEBUG_M(LOG_DEBUG, 0, job_item->chan_id, "finish job %s, CB, job->id = %d \n", __func__,((struct video_entity_job_t *)job_item->private)->id);
    
    driver_set_out_property(NULL,job_item->private);
     /* schedule the delay workq */
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)osg_callback_process);
    queue_delayed_work(g_info.workq, &process_callback_work,  callback_period);
}


/* callback functions called from the core
 */
struct f_ops callback_ops = {
    callback:           &driver_callback,
    pre_process:        &driver_preprocess,
    post_process:       &driver_postprocess,
    mark_engine_start:  &driver_mark_engine_start,
    mark_engine_finish: &driver_mark_engine_finish,
};

/*
 * Add a node to joblist
 */
static void osg_joblist_add(int chan_id, osg_job_t *node)
{
    unsigned long flags;

    if(node == NULL || chan_id < 0 || chan_id >= ENTITY_MINORS){
        OSG_PANIC("%s node is NULL or chan_id is illigle %d\n",__func__,chan_id);
    }
    
    INIT_LIST_HEAD(&node->list);

    DRV_VGLOCK(flags);
    list_add_tail(&node->list, &g_info.fire_job_list);
    atomic_inc(&g_info.list_cnt[chan_id]);
    DRV_VGUNLOCK(flags);
}
static void osg_multi_add_to_joblist(osg_job_t *rt_node)
{
    unsigned long flags;
    osg_job_t  *job_node,*nex_job;

    if(rt_node == NULL ){
        OSG_PANIC("%s node is NULL %d\n",__func__,__LINE__);
        return;
    } 

    list_for_each_entry_safe(job_node, nex_job, &rt_node->mjob_head,mjob_list) {

        INIT_LIST_HEAD(&job_node->list);
        DRV_VGLOCK(flags);
        list_add_tail(&job_node->list, &g_info.fire_job_list);
        atomic_inc(&g_info.list_cnt[job_node->chan_id]);
        DRV_VGUNLOCK(flags);
    }
}
/*
 * Add a node to ongoing joblist
 */
static void osg_ongoing_joblist_add(int chan_id, osg_job_t *node)
{
    unsigned long flags;

    if(node == NULL || chan_id < 0 || chan_id >= ENTITY_MINORS){
        OSG_PANIC("%s node is NULL or chan_id is illigle %d\n",__func__,chan_id);
    }
    
    INIT_LIST_HEAD(&node->list);

    DRV_VGLOCK(flags);
    list_add_tail(&node->list, &g_info.going_job_list);
    atomic_inc(&g_info.list_going_cnt[chan_id]);
    DRV_VGUNLOCK(flags);
}

/*
 * Remove a node to ongoing joblist
 */
static int osg_ongoing_joblist_remove(int chan_id, osg_job_t *node)
{
    unsigned long flags;
    int           bfound = 0;
    osg_job_t  *job_node,*nex_job;

    if(node == NULL || chan_id < 0 || chan_id >= ENTITY_MINORS){
        OSG_PANIC("%s node is NULL or chan_id is illigle %d\n",__func__,chan_id);
    }
    
    DRV_VGLOCK(flags);

    list_for_each_entry_safe(job_node, nex_job, &g_info.going_job_list, list) {

        if(job_node == node){
            list_del_init(&job_node->list);
            atomic_dec(&g_info.list_going_cnt[job_node->chan_id]);
            bfound = 1;
            DEBUG_M(LOG_DEBUG, 0, job_node->chan_id,"remove from going joblist id=%d\n",job_node->jobid);
            break;
        }

    }
    
    DRV_VGUNLOCK(flags);

    if(!bfound)
        return -1;
    return 0;
}

/* Get number of nodes in the list
 */
static unsigned int osg_joblist_cnt(int chan_id)
{
    unsigned long flags;
    unsigned int    count = 0;
    
    if(chan_id < 0 || chan_id >= ENTITY_MINORS){
        OSG_PANIC("%s chan_id is illigle\n",__func__);
    }
    /* save irq */
    DRV_VGLOCK(flags);
    count = atomic_read(&g_info.list_cnt[chan_id]); 
    /* restore irq */
    DRV_VGUNLOCK(flags);

    return count;
}

static unsigned int osg_goinglist_cnt(int chan_id)
{
    unsigned long flags;
    unsigned int    count = 0;
    
    if(chan_id < 0 || chan_id >= ENTITY_MINORS){
        OSG_PANIC("%s chan_id is illigle\n",__func__);
    }
    /* save irq */
    DRV_VGLOCK(flags);
    count = atomic_read(&g_info.list_going_cnt[chan_id]); 
    /* restore irq */
    DRV_VGUNLOCK(flags);

    return count;
}

static unsigned int osg_finishlist_cnt(int chan_id)
{
    unsigned long flags;
    unsigned int    count = 0;
    
    if(chan_id < 0 || chan_id >= ENTITY_MINORS){
        OSG_PANIC("%s chan_id is illigle\n",__func__);
    }
    /* save irq */
    DRV_VGLOCK(flags);
    count = atomic_read(&g_info.list_finish_cnt[chan_id]); 
    /* restore irq */
    DRV_VGUNLOCK(flags);

    return count;
}

static unsigned int osg_jobfail_cnt(int chan_id)
{
    unsigned long flags;
    unsigned int    count = 0;
    
    if(chan_id < 0 || chan_id >= ENTITY_MINORS){
        OSG_PANIC("%s chan_id is illigle\n",__func__);
    }
    /* save irq */
    DRV_VGLOCK(flags);
    count = atomic_read(&g_info.job_fail_cnt[chan_id]); 
    /* restore irq */
    DRV_VGUNLOCK(flags);

    return count;
}

static unsigned int osg_do_jobfail_cnt(int chan_id)
{
    unsigned long flags;
    unsigned int    count = 0;
    
    if(chan_id < 0 || chan_id >= ENTITY_MINORS){
        OSG_PANIC("%s chan_id is illigle\n",__func__);
    }
    /* save irq */
    DRV_VGLOCK(flags);
    count = atomic_read(&g_info.job_do_fail_cnt[chan_id]); 
    /* restore irq */
    DRV_VGUNLOCK(flags);

    return count;
}

/*
 * Get last multi job 
 * 
 */
osg_job_t  *osg_multi_get_last_job(osg_job_t  *root_node)
{
    unsigned long flags;
    osg_job_t *curr,*ne;
    if(root_node == NULL)
        return NULL;
    
    DRV_MULTILOCK(flags);
    list_for_each_entry_safe(curr, ne, &root_node->mjob_head, mjob_list) {
            if (list_is_last(&curr->mjob_list, &root_node->mjob_head))
                return curr;
    }
    DRV_MULTIUNLOCK(flags); 
    return NULL;
}

/*
 * Get next multi job 
 * 
 */
struct video_entity_job_t *osg_get_next_job(struct video_entity_job_t * cur_jb)
{
    if(cur_jb == NULL)
        return NULL;

    if(cur_jb->next_job == NULL)
        return NULL;

    return (struct video_entity_job_t*) cur_jb->next_job;
    
}

int osg_is_multi_job(struct video_entity_job_t * ft_jb)
{
    if(ft_jb == NULL)
        return 0;

    if(ft_jb->next_job == NULL)
        return 0;

    return 1;
    
}

int osg_is_multi_last_job(struct video_entity_job_t * ft_jb)
{
    if(ft_jb == NULL)
        return 1;

    if(ft_jb->next_job == NULL)
        return 1;

    return 0;    
}

/*
 * put JOB
 * We must save the last job for de-interlace reference.
 */
static int driver_putjob(void *parm)
{
    unsigned long flags;
    struct video_entity_job_t *cur_job = (struct video_entity_job_t *)parm;  
    osg_job_t  *job_node = NULL,*root_job_item = NULL,*last_job_item = NULL;
    int         ret = 0, chan_id;
    int         multi_job_ct = 0 , is_multi_job = 0;

    is_multi_job = osg_is_multi_job(cur_job);
    
    do{    
        chan_id =  MAKE_IDX(ENTITY_MINORS, ENTITY_FD_ENGINE(cur_job->fd), ENTITY_FD_MINOR(cur_job->fd));

        if (chan_id > ENTITY_MINORS) {
            OSG_PANIC("Error to use %s engine %d, max is %d\n",MODULE_NAME, chan_id, ENTITY_MINORS);
        }

        job_node = (osg_job_t *)kmem_cache_alloc(g_info.job_cache, GFP_KERNEL);
        if (job_node == NULL) {
            OSG_PANIC("%s, no memory! \n", __func__);
            return JOB_STATUS_FAIL;
        }
        
        memset(job_node, 0x0, sizeof(osg_job_t));
        job_node->chan_id  = job_node->param.chan_id = chan_id;
        
        INIT_LIST_HEAD(&job_node->mjob_head);
        INIT_LIST_HEAD(&job_node->mjob_list);

        if(is_multi_job){            
            job_node->job_type = OSG_JOB_MULTI_TYPE;
            if(osg_is_multi_last_job(cur_job)){
                job_node->need_to_callback = 1;
                job_node->is_last_job = 1;
            }else{
                job_node->need_to_callback = 0;
                job_node->is_last_job = 0;
            }            
        }
        else{
            job_node->job_type = OSG_JOB_SINGLE_TYPE;
            job_node->need_to_callback = 1;
            job_node->is_last_job = 1;
        }

        /*
         * parse the parameters and assign to param structure
         */
        ret= driver_parse_in_property(&job_node->param, cur_job);
        multi_job_ct++;

        job_node->jobid   = cur_job->id;
        job_node->private = cur_job;
        job_node->fops    = &callback_ops;
        //memcpy(&job_node->param, &param, sizeof(osg_param_t));
        job_node->puttime = jiffies;
        job_node->starttime = 0;
        job_node->finishtime = 0;
        job_node->status = JOB_STS_QUEUE;
        
        if(multi_job_ct == 1){    
            if (ret < 0){
                osg_release_param_list(&job_node->param);
                kmem_cache_free(g_info.job_cache, job_node);
                OSG_JOBFAIL_INC(flags,chan_id);
                //printk("%s %d\n",__func__,__LINE__);
                return JOB_STATUS_FAIL;
            }
            
            root_job_item = job_node;
            job_node->parent_entity_job = (struct video_entity_job_t *)job_node->private;             
                
        }else{
            if (ret < 0){
                if(job_node->is_last_job && (last_job_item = osg_multi_get_last_job(root_job_item))){
                    last_job_item->need_to_callback = 1;
                    last_job_item->is_last_job = 1;
                }
                else
                    OSG_PANIC("last job can't find error %d\n",__LINE__);
                //printk("!!!!!!!!!!%s %d\n",__func__,__LINE__);
                osg_release_param_list(&job_node->param);
                kmem_cache_free(g_info.job_cache, job_node);
                OSG_JOBFAIL_INC(flags,chan_id);
                break;
            }
            job_node->parent_entity_job = (struct video_entity_job_t *)root_job_item->private;           
        }
        
        /* save irq */
        DRV_VGLOCK(flags);
    	g_info.vg_node_count++;
        //printk("%d g_info.vg_node_count=%d\n",is_multi_job,g_info.vg_node_count);
        /* restore irq */
        DRV_VGUNLOCK(flags);        

        DRV_MULTILOCK(flags);
        list_add_tail(&job_node->mjob_list, &root_job_item->mjob_head);
        DRV_MULTIUNLOCK(flags);   
        
        DEBUG_M(LOG_DEBUG, 0, job_node->chan_id, "%s, %x, job->id = %d \n", __func__,(unsigned int)job_node,((struct video_entity_job_t *)job_node->private)->id);

        if(!is_multi_job){
            osg_joblist_add(chan_id, job_node); 
            wake_up_process(g_info.osg_thread);
        }
        
    }while((cur_job = osg_get_next_job(cur_job))!=NULL);

    // printk("final g_info.vg_node_count=%d %d\n",g_info.vg_node_count,multi_job_ct);
    if(is_multi_job)
    {
        osg_multi_add_to_joblist(root_job_item);
        wake_up_process(g_info.osg_thread);
    }
    
    return JOB_STATUS_ONGOING;
    
}

int osg_stop_channel(int chn)
{
    osg_job_t  *job_node,*nex_job;

    unsigned long flags;

    DRV_VGLOCK(flags);

    list_for_each_entry_safe(job_node, nex_job, &g_info.fire_job_list, list) {

        if(job_node->chan_id == chn){
            list_del_init(&job_node->list);
            atomic_dec(&g_info.list_cnt[job_node->chan_id]);

            job_node->status = JOB_STS_FLUSH;
                
            INIT_LIST_HEAD(&job_node->list);
            list_add_tail(&job_node->list, &g_info.finish_job_list);
            atomic_inc(&g_info.list_finish_cnt[job_node->chan_id]);
        }

    }
    
    DRV_VGUNLOCK(flags);
    return 0;
}


/* Stop a channel
 */
static int driver_stop(void *parm, int engine, int minor)
{
    int idx ;
    if(engine > 0){
        DEBUG_M(LOG_WARNING, engine, minor,"osg:engine parameter is error %dx%d\n",engine,minor);
        return -1;
    }
    
    idx = MAKE_IDX(ENTITY_MINORS, engine, minor);

    if(idx < 0 || idx >=ENTITY_MINORS){
        DEBUG_M(LOG_WARNING, engine, minor,"osg:idx parameter is error %d\n", idx);
        return -1;
    }
    
    //if (debug_mode >= DEBUG_VG)
    //    printk("%s \n", __func__);

    osg_stop_channel(idx);

    /* schedule the delay workq */
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)osg_callback_process);
    queue_delayed_work(g_info.workq, &process_callback_work, callback_period);

    return 0;
}

static struct property_map_t *driver_get_propertymap(int id)
{
    int i;

    for(i=0;i<sizeof(property_map)/sizeof(struct property_map_t);i++) {
        if(property_map[i].id==id) {
            return &property_map[i];
        }
    }
    return 0;  
  
}


static int driver_queryid(void *parm,char *str)
{
    int i;

    for(i=0;i<sizeof(property_map)/sizeof(struct property_map_t);i++) {
        if(strcmp(property_map[i].name,str)==0) {
            return property_map[i].id;
        }
    }
    printk("osg:driver_queryid: Error to find name %s\n",str);

    return -1;
}

/* pass the id and return the string
 */
static int driver_querystr(void *parm,int id,char *string)
{
    int i;

    for(i=0;i<sizeof(property_map)/sizeof(struct property_map_t);i++) {
        if(property_map[i].id==id) {
            strcpy(string, property_map[i].name);

            return 0;
        }
    }
    printk("osg:driver_querystr: Error to find id %d\n",id);
    return -1; 
}


void print_info(void)
{
	//len += sprintf(page + len, "mem inc cnt %d\n", g_mem_access);
    return;
}

static int proc_info_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    print_info();
    return count;
}


static int proc_info_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    print_info();
    *eof = 1;
    return 0;
}


static int proc_cb_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int mode_set=0;
    sscanf(buffer, "%d",&mode_set);
    callback_period=mode_set;
    printk("\nCallback Period =%d (ticks)\n",callback_period);
    return count;
}


static int proc_cb_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    printk("\nCallback Period =%d (ticks)\n",callback_period);
    *eof = 1;
    return 0;
}


static int proc_util_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
	int i=0;
    int mode_set=0;
    sscanf(buffer, "%d", &mode_set);
    for(i=0;i<OSG_FIRE_MAX_NUM;i++)
	{
    	UTIL_PERIOD(i) = mode_set;
    	printk("\nUtilization Period[%d] =%d(sec)\n", i, UTIL_PERIOD(i));
	}
    return count;
}


static int proc_util_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
	int i, len = 0;

    for(i=0;i<OSG_FIRE_MAX_NUM;i++)
	{
        len += sprintf(page + len, "\nEngine%d HW Utilization Period=%d(sec) Utilization=%d\n",
            i,UTIL_PERIOD(i),UTIL_RECORD(i));
    }

    *eof = 1;
    return len;
}


static unsigned int property_engine=0,property_minor=0;
static int proc_property_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int mode_engine=0,mode_minor=0;

    sscanf(buffer, "%d %d",&mode_engine,&mode_minor);
    /*property engine is alway 0*/
    property_engine=0;
    
    property_minor=mode_minor;

    printk("\nLookup engine=%d minor=%d\n",property_engine,property_minor);
    return count;
}


static int proc_property_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    unsigned long flags;
    int i=0;
    struct property_map_t *map;
    unsigned int id,value,ch;
    int idx = MAKE_IDX(ENTITY_MINORS, property_engine, property_minor);

    DRV_VGLOCK(flags);


    printk("\n%s engine:%d minor:%d job:%d\n",MODULE_NAME,
        property_engine,property_minor,property_record[idx].job_id);
    printk("=============================================================\n");
    printk("CH  ID  Name(string) Value(hex)  Readme\n");
    do {
        id=property_record[idx].property[i].id;
        if(id==ID_NULL)
            break;
        ch=property_record[idx].property[i].ch;
        value=property_record[idx].property[i].value;
        map=driver_get_propertymap(id);
        if(map) {
            printk("%02d  %03d  %15s  %08x  %s\n",ch,id,map->name,value,map->readme);
        }
        i++;
    } while(1);

    DRV_VGUNLOCK(flags);

    *eof = 1;
    return 0;
}

static int proc_job_read_mode(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    unsigned long flags;
    struct video_entity_job_t *job;
    osg_job_t *node;
    char *st_string[] = {"QUEUE", "ONGOING", " FINISH", "FLUSH"}, *string = NULL;

    printk("\nSystem ticks=0x%x\n", (int)jiffies & 0xffff);

    printk("Chnum  Job_ID     Status     Puttime      start    end \n");
    printk("===========================================================\n");

    DRV_VGLOCK(flags);
    list_for_each_entry(node, &g_info.fire_job_list, list) {
        if (node->status & JOB_STS_QUEUE)   string = st_string[0];
        if (node->status & JOB_STS_ONGOING) string = st_string[1];
        if (node->status & JOB_STS_DONE)    string = st_string[2];
        if (node->status & JOB_STS_FLUSH)   string = st_string[3];
        job = (struct video_entity_job_t *)node->private;
        printk("%-5d  %-9d  %-9s  0x%04x  0x%04x  0x%04x \n", node->chan_id, job->id, string,
            node->puttime, node->starttime, node->finishtime);
    }

    list_for_each_entry(node, &g_info.going_job_list, list) {
        if (node->status & JOB_STS_QUEUE)   string = st_string[0];
        if (node->status & JOB_STS_ONGOING) string = st_string[1];
        if (node->status & JOB_STS_DONE)    string = st_string[2];
        if (node->status & JOB_STS_FLUSH)   string = st_string[3];
        job = (struct video_entity_job_t *)node->private;
        printk("%-5d  %-9d  %-9s  0x%04x  0x%04x  0x%04x \n", node->chan_id, job->id, string,
            node->puttime, node->starttime, node->finishtime);
    }

    list_for_each_entry(node, &g_info.finish_job_list, list) {
        if (node->status & JOB_STS_QUEUE)   string = st_string[0];
        if (node->status & JOB_STS_ONGOING) string = st_string[1];
        if (node->status & JOB_STS_DONE)    string = st_string[2];
        if (node->status & JOB_STS_FLUSH)   string = st_string[3];
        job = (struct video_entity_job_t *)node->private;
        printk("%-5d  %-9d  %-9s  0x%04x  0x%04x  0x%04x \n", node->chan_id, job->id, string,
            node->puttime, node->starttime, node->finishtime);
    }
	DRV_VGUNLOCK(flags);


    *eof = 1;
    return 0;
}

void print_filter(void)
{
    int i;
    printk("\nUsage:\n#echo [0:exclude/1:include] Engine Minor > filter\n");
    printk("Driver log Include:");
    for(i=0;i<MAX_FILTER;i++)
        if(include_filter_idx[i]>=0)
            printk("{%d,%d},",IDX_ENGINE(include_filter_idx[i],ENTITY_MINORS),
                IDX_MINOR(include_filter_idx[i],ENTITY_MINORS));

    printk("\nDriver log Exclude:");
    for(i=0;i<MAX_FILTER;i++)
        if(exclude_filter_idx[i]>=0)
            printk("{%d,%d},",IDX_ENGINE(exclude_filter_idx[i],ENTITY_MINORS),
                IDX_MINOR(exclude_filter_idx[i],ENTITY_MINORS));
    printk("\n");
}

//echo [0:exclude/1:include] Engine Minor > filter
static int proc_filter_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int i;
    int engine,minor,mode;
    sscanf(buffer, "%d %d %d",&mode,&engine,&minor);

    if(mode==0) { //exclude
        for(i=0;i<MAX_FILTER;i++) {
            if(exclude_filter_idx[i]==-1) {
                exclude_filter_idx[i]=(engine<<16)|(minor);
                break;
            }
        }
    } else if(mode==1) {
        for(i=0;i<MAX_FILTER;i++) {
            if(include_filter_idx[i]==-1) {
                include_filter_idx[i]=(engine<<16)|(minor);
                break;
            }
        }
    }
    print_filter();
    return count;
}


static int proc_filter_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    print_filter();
    *eof = 1;
    return 0;
}


static int proc_level_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int level;

    sscanf(buffer,"%d",&level);
    log_level=level;
    printk("\nLog level =%d (1:emerge 1:error 2:debug)\n",log_level);
    return count;
}


static int proc_level_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    printk("\nLog level =%d (1:emerge 1:error 2:debug)\n",log_level);
    *eof = 1;
    return 0;
}

struct video_entity_ops_t driver_ops = {
    putjob:      &driver_putjob,
    stop:        &driver_stop,
    queryid:     &driver_queryid,
    querystr:    &driver_querystr,
};

struct video_entity_t osg_entity = {
    type:       TYPE_OSG,
    name:       "osg",
    engines:    ENTITY_ENGINES,
    minors:     ENTITY_MINORS,
    ops:        &driver_ops
};

/* Print debug message
 */
void osg_vg_debug_print(char *page, int *len, void *data)
{
       int i, len2 = *len;

    for (i = 0; i < MAX_CHAN_NUM; i ++) {
        if (osg_joblist_cnt(i))
            len2 += sprintf(page + len2, "job_list chan %-2d, job = %d \n", i,osg_joblist_cnt(i));

        if(osg_goinglist_cnt(i))
            len2 += sprintf(page + len2, "ongoing_job chan %-2d, job = %d \n", i,osg_goinglist_cnt(i));

        if(osg_finishlist_cnt(i))
            len2 += sprintf(page + len2, "finish_job chan %-2d, job = %d \n", i,osg_finishlist_cnt(i));

        if(osg_jobfail_cnt(i))
            len2 += sprintf(page + len2, "job_fail chan %-2d, job = %d \n", i,osg_jobfail_cnt(i));

        if(osg_do_jobfail_cnt(i))
            len2 += sprintf(page + len2, "do_job_fail chan %-2d, job = %d \n", i,osg_do_jobfail_cnt(i));   
        
    }
    
    len2 += sprintf(page + len2, "swosg vg_node_count %d\n", g_info.vg_node_count);
    len2 += sprintf(page + len2, "swosg param_count %d\n", g_info.alloc_param_count);

    *len = len2;
}

static int osg_process_thread(void *private)
{
    int is_empty;
    unsigned long flags; 
    osg_job_t   *job_node;
    
    if(private) {}
    
    g_info.osg_running_flag = 1;

    set_current_state(TASK_INTERRUPTIBLE);
    do {
        schedule();
        __set_current_state(TASK_RUNNING);
        do {
            DRV_VGLOCK(flags);
            if ((is_empty = list_empty(&g_info.fire_job_list))) {
                DRV_VGUNLOCK(flags);
                break;
            }
            /* take one from the list */
            job_node = list_entry(g_info.fire_job_list.next, osg_job_t, list);
            DEBUG_M(LOG_DEBUG, 0, job_node->chan_id,"%s %d\n",__func__,__LINE__);
            list_del_init(&job_node->list);
            if(job_node->chan_id < 0 || job_node->chan_id >= ENTITY_MINORS){
                DRV_VGUNLOCK(flags);
                OSG_PANIC("%s chan_id is illigle\n",__func__);
                break;   
            }                
            atomic_dec(&g_info.list_cnt[job_node->chan_id]);
            DRV_VGUNLOCK(flags);
            
            DEBUG_M(LOG_DEBUG, 0, job_node->chan_id, "going job %s, CB, job->id = %d \n", __func__,((struct video_entity_job_t *)job_node->private)->id);
            job_node->status = JOB_STS_ONGOING;
            osg_ongoing_joblist_add(job_node->chan_id,job_node);            
           
            if (job_node->fops && job_node->fops->pre_process)
                job_node->fops->pre_process(job_node, job_node->private);

            
            if (job_node->fops && job_node->fops->mark_engine_start)
                   job_node->fops->mark_engine_start(job_node, job_node->private);
            
            /*process draing rectangle*/    
            if(osg_vg_process_job(job_node) < 0){
                DRV_VGLOCK(flags);
                atomic_inc(&g_info.job_do_fail_cnt[job_node->chan_id]);
                DRV_VGUNLOCK(flags);    
                job_node->status = JOB_STS_FLUSH;        
            }else
                job_node->status = JOB_STS_DONE; 
            
            if (job_node->fops && job_node->fops->mark_engine_finish)
                job_node->fops->mark_engine_finish(job_node, job_node->private);

            if(osg_ongoing_joblist_remove(job_node->chan_id,job_node) < 0){
                OSG_PANIC("%s ongoing joblist is unhandle %d\n",__func__,job_node->chan_id);                    
            }
                
            /* measure the performance */
            if (job_node->fops && job_node->fops->post_process)
                job_node->fops->post_process(job_node, job_node->private);  
            job_node->fops->callback(job_node);              
            
		}while(!is_empty);
        
        set_current_state(TASK_INTERRUPTIBLE);        
    }while(!kthread_should_stop());

     __set_current_state(TASK_RUNNING);
    g_info.osg_running_flag = 0;
    return 0;
}
/*
 * Entry point of video graph
 */
int osg_vg_set_palette(int fd, int type, int idx, int crcby)
{
    if(fd || type){}

    if(idx >= OSG_PALETTE_MAX){
        printk("OSG:palette idx is over range\n");
        return -1;
    }

    g_osg_palette_default[idx] = crcby;
    return 0;
}
/*
 * Entry point of video graph
 */
int osg_vg_init(void)
{
    int    i;
    char   wname[30];

    /* register log system */
    register_panic_notifier(osg_vg_panic_notifier);
    register_printout_notifier(osg_vg_print_notifier);


    /* register video entify */
    video_entity_register(&osg_entity);

    /* global information */
    memset(&g_info, 0x0, sizeof(g_info));
  
    /* spinlock */
    spin_lock_init(&g_info.lock);
    spin_lock_init(&g_info.para_lock);
    spin_lock_init(&g_info.multi_lock);
#if 0    
    /* init list head of each channel */
    for(i = 0; i < MAX_CHAN_NUM; i ++){
        g_info.list_cnt[i] = ATOMIC_INIT(0);
        g_info.list_going_cnt[i] = ATOMIC_INIT(0);
        g_info.list_finish_cnt[i] = ATOMIC_INIT(0);
        g_info.job_fail_cnt[i] = ATOMIC_INIT(0);
        g_info.job_do_fail_cnt[i] = ATOMIC_INIT(0);
    }
#endif        
   
    /* init list head of fire channel */
    INIT_LIST_HEAD(&g_info.fire_job_list);
    INIT_LIST_HEAD(&g_info.going_job_list);
    INIT_LIST_HEAD(&g_info.finish_job_list);

    /* workqueue */
    sprintf(wname,"%s_tsk", OSG_DEV_NAME);
    g_info.workq = create_workqueue(wname);
    if (g_info.workq == NULL) {
        printk("osg: error in create workqueue! \n");
        return -EFAULT;
    }

    /* init wait quqeue head */
    init_waitqueue_head(&g_info.osg_queue);
  
    /* create memory cache */
    g_info.job_cache = kmem_cache_create("osg_vg", sizeof(osg_job_t), 0, 0, NULL);
    if (g_info.job_cache == NULL) {
        OSG_PANIC("osg_vg: fail to create cache!");
    }
    
    g_info.job_param_cache = kmem_cache_create("osg_para", sizeof(struct __osg_rect), 0, 0, NULL);
    if (g_info.job_param_cache == NULL) {
        OSG_PANIC("osg_vg: fail to param create cache!");
    }
    /*create thread*/
    g_info.osg_thread = kthread_create(osg_process_thread, NULL, "osg_thread");
    if (IS_ERR(g_info.osg_thread )){
        OSG_PANIC("osg: Error in creating kernel thread! \n");
    }

    //uniform
    entity_proc = create_proc_entry("videograph/osg", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if(entity_proc == NULL) {
        printk("osg:Error to create driver proc, please insert log.ko first.\n");
        return -EFAULT;
    }

    property_record = kzalloc(sizeof(struct property_record_t) * OSG_FIRE_MAX_NUM * ENTITY_MINORS, GFP_KERNEL);
    if (property_record == NULL) {
        OSG_PANIC("%s, no memory! \n", __func__);
    }

    infoproc = create_proc_entry("info", S_IRUGO | S_IXUGO, entity_proc);
    if(infoproc==NULL)
        return -EFAULT;
    infoproc->read_proc = (read_proc_t *)proc_info_read_mode;
    infoproc->write_proc = (write_proc_t *)proc_info_write_mode;

    cbproc = create_proc_entry("callback_period", S_IRUGO | S_IXUGO, entity_proc);
    if(cbproc == NULL)
        return -EFAULT;
    cbproc->read_proc = (read_proc_t *)proc_cb_read_mode;
    cbproc->write_proc = (write_proc_t *)proc_cb_write_mode;

    utilproc = create_proc_entry("utilization", S_IRUGO | S_IXUGO, entity_proc);
    if(utilproc == NULL)
        return -EFAULT;
    utilproc->read_proc = (read_proc_t *)proc_util_read_mode;
    utilproc->write_proc = (write_proc_t *)proc_util_write_mode;

    propertyproc = create_proc_entry("property", S_IRUGO | S_IXUGO, entity_proc);
    if(propertyproc == NULL)
        return -EFAULT;
    propertyproc->read_proc = (read_proc_t *)proc_property_read_mode;
    propertyproc->write_proc = (write_proc_t *)proc_property_write_mode;

    jobproc = create_proc_entry("job", S_IRUGO | S_IXUGO, entity_proc);
    if(jobproc==NULL)
        return -EFAULT;
    jobproc->read_proc = (read_proc_t *)proc_job_read_mode;

    filterproc = create_proc_entry("filter", S_IRUGO | S_IXUGO, entity_proc);
    if(filterproc == NULL)
        return -EFAULT;
    filterproc->read_proc = (read_proc_t *)proc_filter_read_mode;
    filterproc->write_proc = (write_proc_t *)proc_filter_write_mode;

    levelproc = create_proc_entry("level", S_IRUGO | S_IXUGO, entity_proc);
    if(levelproc == NULL)
        return -EFAULT;
    levelproc->read_proc = (read_proc_t *)proc_level_read_mode;
    levelproc->write_proc = (write_proc_t *)proc_level_write_mode;


    memset(g_utilization, 0, sizeof(util_t)*OSG_FIRE_MAX_NUM);

	for(i = 0; i < OSG_FIRE_MAX_NUM; i++)
	{
		UTIL_PERIOD(i) = 5;
	}

    for(i = 0; i < MAX_FILTER; i++) {
        include_filter_idx[i] = -1;
        exclude_filter_idx[i] = -1;
    }
    wake_up_process(g_info.osg_thread); 
    printk("\nosg registers %d entities to video graph. \n", MAX_CHAN_NUM);

    return 0;
}

/*
 * Exit point of video graph
 */
void osg_vg_driver_clearnup(void)
{
    char name[30];

    if (infoproc != 0)
        remove_proc_entry(infoproc->name, entity_proc);
    if (cbproc != 0)
        remove_proc_entry(cbproc->name, entity_proc);
    if (utilproc != 0)
        remove_proc_entry(utilproc->name, entity_proc);
    if (propertyproc != 0)
        remove_proc_entry(propertyproc->name, entity_proc);
    if (jobproc != 0)
        remove_proc_entry(jobproc->name, entity_proc);
    if (filterproc != 0)
        remove_proc_entry(filterproc->name, entity_proc);
    if (levelproc != 0)
        remove_proc_entry(levelproc->name, entity_proc);

    if (entity_proc != 0) {
        memset(&name[0], 0, sizeof(name));
        sprintf(name,"videograph/%s", entity_proc->name);
        remove_proc_entry(name, 0);
    }
    
    if (g_info.osg_thread)
        kthread_stop(g_info.osg_thread);
    
    while (g_info.osg_running_flag == 1)
        msleep(1);
        
    video_entity_deregister(&osg_entity);
    kfree(property_record);
    // remove workqueue
    destroy_workqueue(g_info.workq);
    kmem_cache_destroy(g_info.job_cache);
    kmem_cache_destroy(g_info.job_param_cache);
}


