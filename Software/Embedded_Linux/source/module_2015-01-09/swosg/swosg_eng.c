#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include "swosg_eng.h"
#include "swosg/swosg_if.h"

#define SWOSG_ALPHA_MAX       8

#define SWOSG_PALETTE_COLOR_AQUA              0xCA48CA93//0x4893CA        /*  UYVY */
#define SWOSG_PALETTE_COLOR_BLACK             0x10801080//0x808010
#define SWOSG_PALETTE_COLOR_BLUE              0x296e29f0//0x6ef029
#define SWOSG_PALETTE_COLOR_BROWN             0x51a1515b//0xA15B51
#define SWOSG_PALETTE_COLOR_DODGERBLUE        0x693f69cb//0x3FCB69
#define SWOSG_PALETTE_COLOR_GRAY              0xb580b580//0x8080B5
#define SWOSG_PALETTE_COLOR_GREEN             0x515b5151//0x515B51
#define SWOSG_PALETTE_COLOR_KHAKI             0x72897248//0x894872
#define SWOSG_PALETTE_COLOR_LIGHTGREEN        0x90229036//0x223690
#define SWOSG_PALETTE_COLOR_MAGENTA           0x6ede6eca//0xDECA6E
#define SWOSG_PALETTE_COLOR_ORANGE            0x98bc9851//0xBC5198
#define SWOSG_PALETTE_COLOR_PINK              0xa5b3a589//0xB389A5
#define SWOSG_PALETTE_COLOR_RED               0x52f0525a//0xF05A52 
#define SWOSG_PALETTE_COLOR_SLATEBLUE         0x3d603da6//0x60A63D
#define SWOSG_PALETTE_COLOR_WHITE             0xeb80eb80//0x8080EB
#define SWOSG_PALETTE_COLOR_YELLOW            0xd292d210//0x9210D2

#define OSG_GET_PALETEE(idx)           (g_swosg_palette_default[idx])
static u32 g_swosg_palette_default[SWOSG_PALETTE_MAX] = {
    SWOSG_PALETTE_COLOR_WHITE,
    SWOSG_PALETTE_COLOR_BLACK ,
    SWOSG_PALETTE_COLOR_RED,
    SWOSG_PALETTE_COLOR_BLUE,
    SWOSG_PALETTE_COLOR_YELLOW,
    SWOSG_PALETTE_COLOR_GREEN,
    SWOSG_PALETTE_COLOR_BROWN,
    SWOSG_PALETTE_COLOR_DODGERBLUE,
    SWOSG_PALETTE_COLOR_GRAY,
    SWOSG_PALETTE_COLOR_KHAKI,
    SWOSG_PALETTE_COLOR_LIGHTGREEN,
    SWOSG_PALETTE_COLOR_MAGENTA,
    SWOSG_PALETTE_COLOR_ORANGE,
    SWOSG_PALETTE_COLOR_PINK,
    SWOSG_PALETTE_COLOR_SLATEBLUE,
    SWOSG_PALETTE_COLOR_AQUA
};

static unsigned int g_swosg_eng_alpha[SWOSG_ALPHA_MAX] = {
    0,
    25,
    37,
    50,
    62,
    75,
    87,
    100    
};
extern unsigned int log_level;

/*function definition*/
static void swosg_swap(int *aPtr, int *bPtr)
{
    int temp;
    temp = *aPtr;
    *aPtr = *bPtr;
    *bPtr = temp;
} 

int swosg_eng_palette_set( int id, u32 crycby)
{
    if(id >= SWOSG_PALETTE_MAX || id < 0)
        return -1;
    
    g_swosg_palette_default[id] = crycby;
    return 0;
}

int swosg_eng_palette_get( int id, u32* crycby)
{
    if(id >= SWOSG_PALETTE_MAX || id < 0)
        return -1;
    
    *crycby = g_swosg_palette_default[id];
    return 0;
}


int swosg_eng_get_align_blit_xy(swosg_if_blit_param_t * blit_param,int *out_x,int *out_y)
{
    if(blit_param == NULL)
        return -1;

    switch(blit_param->align_type) {
        case SWOSG_WIN_ALIGN_TOP_LEFT:
            *out_x = blit_param->target_x;
            *out_y = blit_param->target_y;
            break;
        case SWOSG_WIN_ALIGN_TOP_CENTER:
            if (blit_param->dest_bg_w >= blit_param->src_w)
                *out_x = (blit_param->dest_bg_w - blit_param->src_w) / 2;
            else{
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_width(%d) < obj_width(%d)\n",
                        blit_param->dest_bg_w,  blit_param->src_w);
                return -1;
            }
            *out_y = blit_param->target_y;
            break;
        case SWOSG_WIN_ALIGN_TOP_RIGHT:
            if (blit_param->dest_bg_w >= (blit_param->target_x + blit_param->src_w))
                *out_x = blit_param->dest_bg_w - blit_param->target_x - blit_param->src_w;
            else{
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_width(%d) < (in_x(%d) + obj_width(%d))\n",
                        blit_param->dest_bg_w , blit_param->target_x,blit_param->src_w);
                return -1;
            }
            *out_y = blit_param->target_y;
            break;
        case SWOSG_WIN_ALIGN_BOTTOM_LEFT:
            *out_x = blit_param->target_x;
            if (blit_param->dest_bg_h >= (blit_param->target_y + blit_param->src_h))
                *out_y = blit_param->dest_bg_h - blit_param->target_y - blit_param->src_h;
            else{ 
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_height(%d) < (in_y(%d) + obj_height(%d))\n",
                        blit_param->dest_bg_h, blit_param->target_y,blit_param->src_h);
                return -1;
            }    
            break;
        case SWOSG_WIN_ALIGN_BOTTOM_CENTER:
            if (blit_param->dest_bg_w  >= blit_param->src_w)
                *out_x = (blit_param->dest_bg_w - blit_param->src_w) / 2;
            else{
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_width(%d) < obj_width(%d)\n",
                        blit_param->dest_bg_w, blit_param->src_w);
                return -1;
            }
                
            if (blit_param->dest_bg_h >= (blit_param->target_y + blit_param->src_h))
                *out_y = blit_param->dest_bg_h - blit_param->target_y - blit_param->src_h;
            else {
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_height(%d) < (in_y(%d) + obj_height(%d))\n",
                        blit_param->dest_bg_h, blit_param->target_y, blit_param->src_h);
                return -1;
            }
            break;
       case SWOSG_WIN_ALIGN_BOTTOM_RIGHT:
            if (blit_param->dest_bg_w  >= (blit_param->target_x + blit_param->src_w))
                *out_x = blit_param->dest_bg_w - blit_param->target_x - blit_param->src_w;
            else{
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_width(%d) < (in_x(%d) + obj_width(%d))\n",
                        blit_param->dest_bg_w , blit_param->target_x,blit_param->src_w);
                return -1;
            }    
            if (blit_param->dest_bg_h >= (blit_param->target_y  + blit_param->src_h))
                *out_y = blit_param->dest_bg_h - blit_param->target_y - blit_param->src_h;
            else{ 
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_height(%d) < (in_y(%d) + obj_height(%d))\n",
                        blit_param->dest_bg_h, blit_param->target_y, blit_param->src_h);
                return -1;
            }
            break;
        case SWOSG_WIN_ALIGN_CENTER:
            if (blit_param->dest_bg_w >= blit_param->src_w)
                *out_x = (blit_param->dest_bg_w - blit_param->src_w) / 2;
            else{
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_width(%d) < obj_width(%d)\n",
                        blit_param->dest_bg_w, blit_param->src_w);
                return -1;
            }
            if (blit_param->dest_bg_h >= blit_param->src_h)
                *out_y = (blit_param->dest_bg_h - blit_param->src_h) / 2;
            else{
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_height(%d) < obj_height(%d)\n",
                        blit_param->dest_bg_h, blit_param->src_h);
                return -1;
            }
            break;
        default:
            DEBUG_M(LOG_WARNING,"Error,blit get_align_xy, align_type(undefined)=%d\n",blit_param->align_type);
            return -1;
    }
    return 0;    
}

int swosg_eng_do_blit(swosg_if_blit_param_t * blit_param)
{   
    unsigned char* imgdata = (unsigned char*)blit_param->dest_bg_addr;
    unsigned char* src_imgdata = (unsigned char*)blit_param->src_addr;
    unsigned int dx,dy,dw,dh,dpitch,spitch,d_yuv,s_yuv;
    unsigned int *dst_data ,*src_data;
    int i,j,alpha,is_blending;

    if (blit_param == NULL)                                                                           
    {                                                                                       
        return -1;                                                                          
    }

    if (blit_param->dest_bg_w <= 0 || blit_param->dest_bg_h <= 0 )                                                           
    {                                                                                       
        return -1;                                                                          
    }

    if((blit_param->target_x+ blit_param->src_w) > blit_param->dest_bg_w  || (blit_param->target_y + blit_param->src_h) > blit_param->dest_bg_h)
    {
        DEBUG_M(LOG_WARNING,"swosg:blit param is error\n");
        return -1;
    }

    dx = blit_param->target_x;
    dy = blit_param->target_y;
    dw = blit_param->src_w;
    dh = blit_param->src_h;
    dpitch = blit_param->dest_bg_w*2;
    spitch = blit_param->src_w*2;

    if(dx%2 == 1)
        dx-=1;          
   

    if((dx+dw) > (blit_param->dest_bg_w) || (dy+dh) > (blit_param->dest_bg_h)){

        DEBUG_M(LOG_WARNING,"osg:blit range is error\n");
        return -1;
    }

    if(blit_param->align_type >= SWOSG_WIN_ALIGN_MAX){
        DEBUG_M(LOG_WARNING,"swosg:blit align type is error\n");
        return -1;
    }        

    if(swosg_eng_get_align_blit_xy(blit_param,&dx,&dy) < 0){
        DEBUG_M(LOG_WARNING,"swosg:blit get align_xy is error\n");
        return -1;        
    }
    
    if(blit_param->blending > 0 && blit_param->blending < SWOSG_ALPHA_MAX){
        alpha = 255 - (255 * g_swosg_eng_alpha[blit_param->blending]/100);
        is_blending = 1;
    }
    else
        is_blending = 0;

    for(i = 0 ; i < dh ; i++){
        for(j = 0 ; j < dw ; j+=2){
            dst_data = (unsigned int*)&imgdata[((dy+i)*dpitch)+((dx+j)*2)];
            src_data = (unsigned int*)&src_imgdata[(i*spitch)+(j*2)];
            //(i==0)
            //  printk("src_data=%x ,bg=%x\n",(*src_data),blit_param->bg_color)
            if((*src_data) == blit_param->bg_color)
                continue;

            if(!is_blending){
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

int swosg_eng_draw_line_true_frame(struct swosg_eng_point* startPoint
                               ,struct swosg_eng_point* endPoint
                               ,swosg_if_mask_param_t *param)
{
    unsigned char* imgdata = (unsigned char*)param->dest_bg_addr;
    int            pitch,x,y,x0,y0,x1,y1;
    unsigned int* tmp_imgdata; 
    unsigned int  d_yuv,alpha=255,is_blending;
    
    DEBUG_M(LOG_DEBUG,"%s a=%x,wh=%dx%d,s=%dx%d,e=%dx%d pidx=%d\n",__func__,(unsigned \
    int)imgdata,param->dest_bg_w,param->dest_bg_h,startPoint->x,startPoint->y,endPoint->x,endPoint->y,param->palette_idx); 

    if (!imgdata || !param || !startPoint ||!endPoint)                                                                           
    {     
        printk("%s %d\n",__func__,__LINE__);
        return -1;                                                                          
    }
  
    if (param->dest_bg_w <= 0 || param->dest_bg_h < 0 )                                                           
    { 
        printk("%s %d\n",__func__,__LINE__);
        return -1;                                                                          
    }
    
    if (startPoint->x < 0 || startPoint->x > (param->dest_bg_w) 
        || startPoint->y < 0 || startPoint->y > (param->dest_bg_h) 
        || endPoint->x < 0 || endPoint->x > (param->dest_bg_w)  
        || endPoint->y < 0 || endPoint->y > (param->dest_bg_h))   
    { 
        DEBUG_M(LOG_WARNING,"osg:%s drawline range error s=%dx%d e=%dx%d wh=%dx%d\n",__func__, \
        startPoint->x,startPoint->y,endPoint->x,endPoint->y, \
        (param->dest_bg_w) ,(param->dest_bg_h));                                         
        return -1;                                                                          
    }                                                                                       

    if(param->palette_idx >= SWOSG_PALETTE_MAX){
        DEBUG_M(LOG_WARNING,"osg:%s palette index is over %d\n",__func__,param->palette_idx);
        return -1;
    }  

    if( startPoint->x > endPoint->x || startPoint->y != endPoint->y){
        DEBUG_M(LOG_WARNING,"osg:%s start %dx%d and end %dx%d point error\n",__func__, \
        startPoint->x,startPoint->y,endPoint->x,endPoint->y);
        return -1;        
    }

    if(param->blending > 0 && param->blending < SWOSG_ALPHA_MAX){
        alpha = 255 - (255 * g_swosg_eng_alpha[param->blending]/100);
        is_blending = 1;
    }
    else
        is_blending = 0;
    
    pitch = param->dest_bg_w*2;
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
        
        if(!is_blending)
            (*tmp_imgdata) = OSG_GET_PALETEE(param->palette_idx);
        else{    
            (*tmp_imgdata) =(((d_yuv >> 1) & 0xff000000) + (((OSG_GET_PALETEE(param->palette_idx)) >> 1)& 0xff000000))| 
                            ((((d_yuv & 0x00ff0000) >> 1 )& 0x00ff0000 ) + 
                            (((OSG_GET_PALETEE(param->palette_idx)&0x00ff0000) >> 1) & 0x00ff0000)) |
                            ((((d_yuv & 0x0000ff00) >> 1) & 0x0000ff00 ) + 
                            (((OSG_GET_PALETEE(param->palette_idx)&0x0000ff00) >> 1) & 0x0000ff00)) |
                            ((((d_yuv & 0x000000ff) >> 1) & 0x000000ff) +
                            (((OSG_GET_PALETEE(param->palette_idx)&0x000000ff) >> 1) & 0x000000ff)) ;

        }
        
        if((x % 2) == 1 && x == x0)
            x++;
        else
            x+=2;
    }
    
    return 0;
}


int swosg_eng_draw_line_one_frame(struct swosg_eng_point* startPoint
                                   ,struct swosg_eng_point* endPoint
                                   ,swosg_if_mask_param_t *param) 
{
    unsigned char* imgdata = (unsigned char*)param->dest_bg_addr;

    int            pitch,x,y,x0,y0,x1,y1,dx,dy,steep,alpha = 255;
    unsigned int* tmp_imgdata; 
    unsigned int  d_yuv,is_blending = 1;
    
    DEBUG_M(LOG_DEBUG,"%s a=%x,wh=%dx%d,s=%dx%d,e=s=%dx%d pidx=%d\n",__func__,(unsigned \
    int)imgdata,param->dest_bg_w,param->dest_bg_h,startPoint->x,startPoint->y,endPoint->x,endPoint->y,param->palette_idx); 

    if (!imgdata || !param || !startPoint ||!endPoint)                                                                           
    {                                                                                       
        return -1;                                                                          
    }
  
    if (param->dest_bg_w <= 0 || param->dest_bg_h < 0 )                                                           
    {                                                                                       
        return -1;                                                                          
    }
    
    if (startPoint->x < 0 || startPoint->x > (param->dest_bg_w) 
        || startPoint->y < 0 || startPoint->y > (param->dest_bg_h) 
        || endPoint->x < 0 || endPoint->x > (param->dest_bg_w)  
        || endPoint->y < 0 || endPoint->y > (param->dest_bg_h))   
    {                                                                                       
        DEBUG_M(LOG_WARNING,"osg:%s drawline range error s=%dx%d e=%dx%d wh=%dx%d\n",__func__, \
        startPoint->x,startPoint->y,endPoint->x,endPoint->y, \
        (param->dest_bg_w) ,(param->dest_bg_h));                                         
        return -1;                                                                          
    }                                                                                       

    if(param->palette_idx >= SWOSG_PALETTE_MAX){
        DEBUG_M(LOG_WARNING,"osg:%s palette index is over %d\n",__func__,param->palette_idx);
        return -1;
    }                                                                                            
    pitch = param->dest_bg_w *2;
    x0 = startPoint->x, x1 = endPoint->x;                                                 
    y0 = startPoint->y, y1 = endPoint->y;                                                 
    dy = abs(y1 - y0);                                                                  
    dx = abs(x1 - x0);                                                                  
    steep = dy>dx ?1:0; 
    //printk("XXXXa=%x,wh=%dx%d,s=%dx%d,e=s=%dx%d pidx=%d\n",(unsigned \int)imgdata,dst_suf->width,dst_suf->height,x0,y0,x1,y1,rect_tm->palette_idx);                                                                                        
    if (steep)                                                                              
    {                                                                                       
        swosg_swap(&x0, &y0);                                                                       
        swosg_swap(&x1, &y1);                                                                       
    }  
    
    if (x0 > x1)                                                                            
    {                                                                                       
        swosg_swap(&x0, &x1);                                                                       
        swosg_swap(&y0, &y1);                                                                       
    } 
                                          
    y = y0;
    
    if(param->blending > 0 && param->blending < SWOSG_ALPHA_MAX){
        alpha = 255 - (255 * g_swosg_eng_alpha[param->blending]/100);
        is_blending = 1;
    }
    else
        is_blending = 0;
                                                          
    //printk("steep=%d,width=%d,height=%d\n",steep,width,height);
    
    for (x = x0; x <= x1; ) //x++)                                                           
    {                                                                                       
        if (steep)                                                                          
        {  
            if((y%2) == 0)
                tmp_imgdata =(unsigned int*) &imgdata[x*pitch + y*2];
            else
                tmp_imgdata =(unsigned int*) &imgdata[(x*pitch + y*2)-2];

            if(!is_blending){
              (*tmp_imgdata) = OSG_GET_PALETEE(param->palette_idx);
            }
            else{
                d_yuv = (*tmp_imgdata);
                (*tmp_imgdata) =(((d_yuv >> 1) & 0xff000000) + (((OSG_GET_PALETEE(param->palette_idx)) >> 1)& 0xff000000))| 
                        ((((d_yuv & 0x00ff0000) >> 1 )& 0x00ff0000 ) + 
                        (((OSG_GET_PALETEE(param->palette_idx)&0x00ff0000) >> 1) & 0x00ff0000)) |
                        ((((d_yuv & 0x0000ff00) >> 1) & 0x0000ff00 ) + 
                        (((OSG_GET_PALETEE(param->palette_idx)&0x0000ff00) >> 1) & 0x0000ff00)) |
                        ((((d_yuv & 0x000000ff) >> 1) & 0x000000ff) +
                        (((OSG_GET_PALETEE(param->palette_idx)&0x000000ff) >> 1) & 0x000000ff)) ;              
            }
                
            x++;             
           
        }                                                                                   
        else                                                                                
        {

            if((x%2) == 0)
                tmp_imgdata =(unsigned int*) &imgdata[y*pitch + x*2];
            else
                tmp_imgdata =(unsigned int*) &imgdata[(y*pitch + x*2)-2];

            

            if(!is_blending){
              (*tmp_imgdata) = OSG_GET_PALETEE(param->palette_idx);
            }
            else{
                d_yuv = (*tmp_imgdata);
                (*tmp_imgdata) =(((d_yuv >> 1) & 0xff000000) + (((OSG_GET_PALETEE(param->palette_idx)) >> 1)& 0xff000000))| 
                        ((((d_yuv & 0x00ff0000) >> 1 )& 0x00ff0000 ) + 
                        (((OSG_GET_PALETEE(param->palette_idx)&0x00ff0000) >> 1) & 0x00ff0000)) |
                        ((((d_yuv & 0x0000ff00) >> 1) & 0x0000ff00 ) + 
                        (((OSG_GET_PALETEE(param->palette_idx)&0x0000ff00) >> 1) & 0x0000ff00)) |
                        ((((d_yuv & 0x000000ff) >> 1) & 0x000000ff) +
                        (((OSG_GET_PALETEE(param->palette_idx)&0x000000ff) >> 1) & 0x000000ff)) ;           
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

