/*
 * Revision History
 * VER0.0.1 2014/08/05 driven from 8287 code 
 * 
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/slab.h>
//#include <asm/cacheflush.h>
#include <asm/uaccess.h>
#include "swosg_eng.h"
#include "swosg/swosg_if.h"
#include "think2d/think2d_osg_if.h"
#include "frammap/frammap_if.h"   

#define SWOSG8136_VERSION   ((0 << 16) + (0 << 8) + 1) 

#define DMA_CACHE_LINE_SZ   32

#define COLOUR8888(r, g, b, a)  (((r) & 0x000000ff) << 24 | ((g) & 0x000000ff) << 16 | ((b) & 0x000000ff) << 8 | ((a) & 0x000000ff))
#define RGB565_MASK_RED        0xF800   
#define RGB565_MASK_GREEN      0x07E0   
#define RGB565_MASK_BLUE       0x001F  

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
    SWOSG_PALETTE_COLOR_BLACK,   
    SWOSG_PALETTE_COLOR_GRAY    
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

struct swosg_eng_canvas_desc g_swosg_eng_canvas_array[SWOSG_CANVAS_MAX];

extern unsigned int log_level;
extern unsigned int g_limit_printk;

struct semaphore    palette_sem; 
struct semaphore    canvas_sem;

unsigned int g_dma_allo_count = 0;

#ifdef SUPPORT_SWOSG_CANVAS_STATIC
struct swosg_eng_canvas_pool_desc* g_p_canvas_pool = NULL;
unsigned int g_canvas_runmode = SWOSG_ENG_CANVAS_DYAMIC;
#endif

#define RGB1555_MASK_ALPHA_HI      0x80000000 
#define RGB1555_MASK_RED_HI        0x7C000000   
#define RGB1555_MASK_GREEN_HI      0x03E00000
#define RGB1555_MASK_BLUE_HI       0x001F0000 

#define RGB1555_MASK_ALPHA_LO      0x00008000 
#define RGB1555_MASK_RED_LO        0x00007C00   
#define RGB1555_MASK_GREEN_LO      0x000003E0   
#define RGB1555_MASK_BLUE_LO       0x0000001F 

/*function definition*/
void swosg_eng_lock_pallete_mutex(void)
{
    down(&palette_sem);    
}

void swosg_eng_unlock_pallete_mutex(void)
{
    up(&palette_sem);    
}

void swosg_eng_lock_canvas_mutex(void)
{
    down(&canvas_sem);    
}

void swosg_eng_unlock_canvas_mutex(void)
{
    up(&canvas_sem);    
}


static void swosg_swap(int *aPtr, int *bPtr)
{
    int temp;
    temp = *aPtr;
    *aPtr = *bPtr;
    *bPtr = temp;
} 

int swosg_eng_get_dma_count(void)
{
    return g_dma_allo_count;
}

int swosg_eng_get_dma_increase(void)
{
    g_dma_allo_count++;
    return 0;
}

int swosg_eng_get_dma_decrease(void)
{
    g_dma_allo_count--;
    return 0;
}


int swosg_eng_palette_set( int id, u32 crycby)
{
    if(id >= SWOSG_PALETTE_MAX || id < 0){
        DEBUG_M(LOG_WARNING,"swosg:set palette id is out of range(%d)\n",id);
        return -1;
    }
    swosg_eng_lock_pallete_mutex();
    g_swosg_palette_default[id] = crycby;
    swosg_eng_unlock_pallete_mutex();
    return 0;
}

int swosg_eng_palette_get( int id, u32* crycby)
{
    if(id >= SWOSG_PALETTE_MAX || id < 0){
        DEBUG_M(LOG_WARNING,"swosg:get palette id is out of range(%d)\n",id);
        return -1;
    }
    swosg_eng_lock_pallete_mutex();
    *crycby = g_swosg_palette_default[id];
    swosg_eng_unlock_pallete_mutex();
    return 0;
}

unsigned int swosg_eng_get_version(void)
{
    return SWOSG8136_VERSION;   
}

int swosg_eng_transfer_toyuv(swosg_if_canvas_t * src_cav,struct swosg_eng_canvas_cont *dest_cav)
{
    int i , t0,t1;
    unsigned int real_sz = 0 , tmp_data;
    unsigned char* src_image = (unsigned char*)src_cav->src_cava_addr;
    unsigned char* dst_image = (unsigned char*)dest_cav->canvas_vaddr;
    unsigned int  *src_data , *dst_data;
    unsigned char  Y0,Y1;     

    real_sz = src_cav->src_w*src_cav->src_h*2;
    
    for(i = 0 ; i< real_sz ; i+=4)
    {
        src_data = (unsigned int*)&src_image[i];
        dst_data = (unsigned int*)&dst_image[i];
        tmp_data = (*src_data);
        
        t0 = (tmp_data&RGB1555_MASK_ALPHA_HI) ? 1 : 0;
        t1 = (tmp_data&RGB1555_MASK_ALPHA_LO) ? 1 : 0;
        
        //tmp_data = htonl(tmp_data);
        
        if(t0)
            Y1 = (unsigned char)(((tmp_data&RGB1555_MASK_GREEN_HI) >> 21 ) << 3);
        else
            Y1 = 0xff;

        
        if(t1)
            Y0 = (unsigned char)(((tmp_data&RGB1555_MASK_GREEN_LO) >> 5 ) << 3);           
        else
            Y0 = 0xff;

        if(!t0 && !t1)
            *dst_data = 0xffffffff;    
        else if(!t0 && t1)
           *dst_data =  ntohl(((0x80 <<24) | (Y0 <<16) | (0xff <<8) | (Y1)));  
        else if(!t1 && t0)
            *dst_data = ntohl(((0xff <<24) | (Y0 <<16) | (0x80 <<8) | (Y1)));  
        else    
            *dst_data = ntohl(((0x80 <<24) | (Y0 <<16) | (0x80 <<8) | (Y1)));  
#if 0/*debug usage*/
        if(i % 32 == 0)                     
            printk("\r\n");
        printk("%08x ",*src_data);
#endif       
        
    }
#if 0/*debug usage*/    
    printk("%s %d ral=%d\n",__func__,__LINE__,real_sz);
    for(i = 0 ; i < real_sz; i+=4)
    {       
        if(i % 32 == 0)                     
            printk("\r\n");
        
        printk("%08x",(*(unsigned int*)&src_image[i]));                 
    }

    for(i = 0 ; i < real_sz; i+=4)
    {       
        if(i % 32 == 0)                     
            printk("\r\n");
        
        printk("%08x",(*(unsigned int*)&dst_image[i]));                 
    }
#endif 
#if 0
    if(dest_cav->canvas_size%DMA_CACHE_LINE_SZ){
        DEBUG_M(LOG_WARNING,"canvas size isn't align cache size(%d)\n",dest_cav->canvas_size);
        return -1;    
    }

    if (!((unsigned int)dest_cav->canvas_vaddr%DMA_CACHE_LINE_SZ)) {
        __cpuc_flush_dcache_area((void*)dest_cav->canvas_vaddr, dest_cav->canvas_size);
        printk("canvas __cpuc_flush_dcache_area\n");
    }
#endif    
    return 0;

}
#ifdef SUPPORT_SWOSG_CANVAS_STATIC
#if 0
int swosg_eng_copy_canvas_channel_info(u32 *idx_ifno)
{
    int i;

    if(g_p_canvas_pool == NULL)
        panic("%s g_p_canvas_pool is NULL impossible case\n",__func__);
    
    for(i = 0;i<SWOSG_ENG_OSG_NUM;i++){
        g_p_canvas_pool->osg_canvas_ch_info[i] =  idx_ifno[i];      
    }
    return 0;    
}

u32 swosg_eng_calcu_canvas_total_mem(void)
{
    int i;
    u32 total_len=0;
    if(g_p_canvas_pool == NULL)
        panic("%s g_p_canvas_pool is NULL impossible case\n",__func__);

    for(i = SWOSG_ENG_OSG_CH_BASE;i<SWOSG_ENG_OSG_NUM;i++){
        total_len +=  g_p_canvas_pool->osg_canvas_ch_info[i]; 
    }

    return (total_len*SWOSG_ENG_KSIZE_BASE);
}
#endif
int swosg_eng_init_canvas_pool_mem(void)
{
    u32 i;
    
    if(g_p_canvas_pool == NULL || g_p_canvas_pool->cavas_pool_mem == NULL)
        panic("g_p_canvas_pool is NULL impossible case\n");
    
    for(i = 0; i< g_p_canvas_pool->osg_canvas_total_num;i++){       
        g_p_canvas_pool->cavas_pool_mem[i].vbase   = 0;
        g_p_canvas_pool->cavas_pool_mem[i].pbase   = 0;
        g_p_canvas_pool->cavas_pool_mem[i].mem_len = 0;
        g_p_canvas_pool->cavas_pool_mem[i].usage   = 0;
        g_p_canvas_pool->cavas_pool_mem[i].setten  = 0;      
    }
    g_p_canvas_pool->osg_canvas_curvaddr = (u32)g_p_canvas_pool->canvas_buf_info.va_addr;
    g_p_canvas_pool->osg_canvas_curpaddr = (u32)g_p_canvas_pool->canvas_buf_info.phy_addr;
    g_p_canvas_pool->osg_canvas_alloc_size = 0;
    g_p_canvas_pool->osg_canvas_remaind_size = g_p_canvas_pool->osg_canvas_totalsize;
    return 0;
}

int swosg_eng_config_canvas_pool_mem(int ch, int mem)
{
   
    if(ch < SWOSG_ENG_DH_IDX_MIN || ch >= g_p_canvas_pool->osg_canvas_total_num){
        printk("%s set fail(ch(%d))\n",__func__,ch);
		return -1;    
    }

    if((mem*SWOSG_ENG_KSIZE_BASE) > g_p_canvas_pool->osg_canvas_remaind_size){
        printk("%s set fail ch(%d) mem(%d)\n",__func__,ch,mem);
		return -1;           
    }

    if( g_p_canvas_pool->cavas_pool_mem[ch].setten){
        printk("%s this pool have set vadd-size-ch(%x-%d-%d))\n",__func__,
                g_p_canvas_pool->cavas_pool_mem[ch].vbase,
                g_p_canvas_pool->cavas_pool_mem[ch].mem_len,ch);
		return -1;    
    }
    swosg_eng_lock_canvas_mutex();
    g_p_canvas_pool->cavas_pool_mem[ch].vbase = g_p_canvas_pool->osg_canvas_curvaddr ;
    g_p_canvas_pool->cavas_pool_mem[ch].pbase = g_p_canvas_pool->osg_canvas_curpaddr ;
    g_p_canvas_pool->cavas_pool_mem[ch].setten = 1;
    g_p_canvas_pool->cavas_pool_mem[ch].mem_len = (mem*SWOSG_ENG_KSIZE_BASE);
    g_p_canvas_pool->osg_canvas_curvaddr = g_p_canvas_pool->osg_canvas_curvaddr+(mem*SWOSG_ENG_KSIZE_BASE);
    g_p_canvas_pool->osg_canvas_curpaddr = g_p_canvas_pool->osg_canvas_curpaddr+(mem*SWOSG_ENG_KSIZE_BASE);
    g_p_canvas_pool->osg_canvas_remaind_size = g_p_canvas_pool->osg_canvas_remaind_size - (mem*SWOSG_ENG_KSIZE_BASE);
    g_p_canvas_pool->osg_canvas_alloc_size += (mem*SWOSG_ENG_KSIZE_BASE);
    g_p_canvas_pool->osg_canvas_num++;
    swosg_eng_unlock_canvas_mutex();
    return 0;
}

int swosg_eng_get_canvas_pool_mem(struct swosg_eng_canvas_pool_mem * pool_meminfo
                                  ,u32 src_w,u32 src_h,int idx)
{
    
    if(pool_meminfo == NULL){
        DEBUG_M(LOG_WARNING,"pool_meminfo is NULL\n");    
        return -1;
    }
    
    if(idx < SWOSG_ENG_DH_IDX_MIN || idx >= g_p_canvas_pool->osg_canvas_total_num){
        DEBUG_M(LOG_WARNING,"idx is illegle (4>id(%d)>=%d)\n",idx,g_p_canvas_pool->osg_canvas_total_num);    
        return -1;
    }
    
    if(g_p_canvas_pool->cavas_pool_mem[idx].setten == 0)
        return -2;
   
    if(g_p_canvas_pool->cavas_pool_mem[idx].setten == 0 || g_p_canvas_pool->cavas_pool_mem[idx].usage == 1){
        DEBUG_M(LOG_WARNING,"this (id=%d) pool isn't set pa-va-sz-u-s(%x-%x-%d-%d-%d)\n"
                ,idx,g_p_canvas_pool->cavas_pool_mem[idx].pbase
                ,g_p_canvas_pool->cavas_pool_mem[idx].vbase,g_p_canvas_pool->cavas_pool_mem[idx].mem_len
                ,g_p_canvas_pool->cavas_pool_mem[idx].usage,g_p_canvas_pool->cavas_pool_mem[idx].setten);    
        return -1;
    }
    if((src_w*src_h*2) > g_p_canvas_pool->cavas_pool_mem[idx].mem_len){
        DEBUG_M(LOG_WARNING,"id(%d) srcmemsize(%d) larger than canvas size w-h-sz(%d-%d-%d)\n"
                ,idx,(src_w*src_h*2),src_w,src_h,g_p_canvas_pool->cavas_pool_mem[idx].mem_len);    
        return -1;
    }

    g_p_canvas_pool->cavas_pool_mem[idx].usage = 1;
    memcpy(pool_meminfo,&g_p_canvas_pool->cavas_pool_mem[idx],sizeof(struct swosg_eng_canvas_pool_mem));
    return 0;
}

int swosg_eng_release_canvas_pool_mem(struct swosg_eng_canvas_cont * canvas_info,int idx)                                 
{
    
    if(canvas_info == NULL){
        DEBUG_M(LOG_WARNING,"release pool canvas_info is NULL\n");    
        return -1;
    }

    if(idx < SWOSG_ENG_DH_IDX_MIN || idx >= g_p_canvas_pool->osg_canvas_total_num){
        DEBUG_M(LOG_WARNING,"idx is illegle (4>id(%d)>=36)\n",idx);    
        return -1;
    }
    
    if(g_p_canvas_pool->cavas_pool_mem[idx].setten == 0 || g_p_canvas_pool->cavas_pool_mem[idx].usage == 0){
        DEBUG_M(LOG_WARNING,"release this (id=%d) pool isn't set pa-va-sz-u-s(%x-%x-%d-%d-%d)\n"
                ,idx,g_p_canvas_pool->cavas_pool_mem[idx].pbase
                ,g_p_canvas_pool->cavas_pool_mem[idx].vbase,g_p_canvas_pool->cavas_pool_mem[idx].mem_len
                ,g_p_canvas_pool->cavas_pool_mem[idx].usage,g_p_canvas_pool->cavas_pool_mem[idx].setten);    
        return -1;
    }

    if((u32)canvas_info->canvas_vaddr != g_p_canvas_pool->cavas_pool_mem[idx].vbase){
         DEBUG_M(LOG_WARNING,"release this (id=%d) vaddr isn't equal src pa-va(%x-%x) pa-va-sz-u-s(%x-%x-%d-%d-%d)\n"
                ,idx,(u32)canvas_info->canvas_paddr,(u32)canvas_info->canvas_vaddr,g_p_canvas_pool->cavas_pool_mem[idx].pbase
                ,g_p_canvas_pool->cavas_pool_mem[idx].vbase,g_p_canvas_pool->cavas_pool_mem[idx].mem_len
                ,g_p_canvas_pool->cavas_pool_mem[idx].usage,g_p_canvas_pool->cavas_pool_mem[idx].setten);    
        return -1;      
    }
    
    g_p_canvas_pool->cavas_pool_mem[idx].usage = 0;
    return 0;
}

u32 swosg_eng_judge_canvas_runmode(int chan_num , int total_mem)
{
    if(chan_num == 0 || chan_num > SWOSG_CANVAS_MAX || total_mem == 0 )
        return SWOSG_ENG_CANVAS_DYAMIC;
    
    return SWOSG_ENG_CANVAS_STATIC;
}

unsigned int swosg_eng_get_canvas_runmode(void)
{
    return g_canvas_runmode;
}
    
void swosg_eng_set_canvas_runmode(u32 runmode)
{
    if(runmode != SWOSG_ENG_CANVAS_DYAMIC && runmode != SWOSG_ENG_CANVAS_STATIC)
    {
        panic("canvas_runmode not support (%d)\n",runmode);
    }
    g_canvas_runmode = runmode;
    
}

int swosg_eng_canvas_set_static( int id, swosg_if_canvas_t * canvas)
{
    int ret ;
    struct swosg_eng_canvas_cont * canvas_dat;
    struct swosg_eng_canvas_pool_mem  static_pool;

    swosg_eng_lock_canvas_mutex();
    
    canvas_dat =(struct swosg_eng_canvas_cont*)&g_swosg_eng_canvas_array[id].canvas_desc;
    if((canvas->src_w*canvas->src_h*2)%4 !=0){
        DEBUG_M(LOG_WARNING,"static canvas size align error(%dx%d)\n",canvas->src_w,canvas->src_h);    
        goto s_err;
    }        
    
    if(g_swosg_eng_canvas_array[id].usage == 0)
    {
        memset(&static_pool,0x0,sizeof(struct swosg_eng_canvas_pool_mem));
        if((ret = swosg_eng_get_canvas_pool_mem(&static_pool,canvas->src_w,canvas->src_h ,id)) < 0)
        {
            if(ret == -2){
                swosg_eng_unlock_canvas_mutex();
                return 0;    
            }
                
            DEBUG_M(LOG_WARNING,"swosg_eng_get_canvas_pool_mem fail(%d)\n",id);
            goto s_err;
        }
        canvas_dat->canvas_w = canvas->src_w ;
        canvas_dat->canvas_h = canvas->src_h ;
        canvas_dat->canvas_size = static_pool.mem_len;
        canvas_dat->canvas_paddr = (dma_addr_t)static_pool.pbase;
        canvas_dat->canvas_vaddr =(void*)static_pool.vbase;

        swosg_eng_transfer_toyuv(canvas,canvas_dat);
        g_swosg_eng_canvas_array[id].usage = 1;
        
    }
    else /*remove and update*/
    {
        g_swosg_eng_canvas_array[id].usage = 0;
        if((canvas->src_w*canvas->src_h*2) > canvas_dat->canvas_size){
            DEBUG_M(LOG_WARNING,"update size is large id-w-h(%d-%d-%d) pool_sz(%d)\n"
                    ,id,canvas->src_w,canvas->src_h,canvas_dat->canvas_size);
            if(swosg_eng_release_canvas_pool_mem(canvas_dat,id) < 0){
                DEBUG_M(LOG_WARNING,"static del canvas error id-va-pa(%d-%x-%x)\n"
                        ,id,(u32)canvas_dat->canvas_vaddr ,(u32)canvas_dat->canvas_paddr);
            }
            goto s_err;
        }        
        else{/*use original buffer*/
            canvas_dat->canvas_w = canvas->src_w;
            canvas_dat->canvas_h = canvas->src_h;
        }
        
        swosg_eng_transfer_toyuv(canvas,canvas_dat);
        
        g_swosg_eng_canvas_array[id].usage = 1;
    }
    
    swosg_eng_unlock_canvas_mutex();
    return 0;
s_err:
    swosg_eng_unlock_canvas_mutex();
    return -1;
}

int swosg_eng_canvas_del_static( int id)
{
    struct swosg_eng_canvas_cont * canvas_dat;

    if(id >= SWOSG_CANVAS_MAX || id < 0){
        DEBUG_M(LOG_WARNING,"static canvas_del id out of range(%d)\n",id);    
        return -1;
    } 
    
    swosg_eng_lock_canvas_mutex();
    
    if(g_swosg_eng_canvas_array[id].usage == 0){
        swosg_eng_unlock_canvas_mutex();
        return 0; 
        //DEBUG_M(LOG_WARNING,"static can't delete canvas (isn't using) (%d)\n",id);
        //goto sdel_err;       
    }
        
    canvas_dat =(struct swosg_eng_canvas_cont*)&g_swosg_eng_canvas_array[id].canvas_desc;

    if(canvas_dat->canvas_vaddr){
        if(swosg_eng_release_canvas_pool_mem(canvas_dat,id) < 0){
            DEBUG_M(LOG_WARNING,"static del canvas error id-va-pa(%d-%x-%x)\n"
                    ,id,(u32)canvas_dat->canvas_vaddr ,(u32)canvas_dat->canvas_paddr);
            goto sdel_err;     
        }      
    }
    else{
        DEBUG_M(LOG_WARNING,"canvas memory isn't exist id-va(%d-%x)\n",id,(u32)canvas_dat->canvas_vaddr);
        goto sdel_err;    
    }
        
    memset(&g_swosg_eng_canvas_array[id],0x0,sizeof(struct swosg_eng_canvas_desc));
    swosg_eng_unlock_canvas_mutex();
    return 0; 
sdel_err:
    swosg_eng_unlock_canvas_mutex();
    return -1;
}

int swosg_eng_proc_write_canvas_pool_info(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
    int len = size;
	unsigned char value[60];
	int ch = 0,mem = 0;

    if(swosg_eng_get_canvas_runmode() == SWOSG_ENG_CANVAS_DYAMIC){
        printk("osg canvas runmode:%s\n", "dymanic");
        return size;
    }

    if(g_p_canvas_pool == NULL || g_p_canvas_pool->cavas_pool_mem == NULL ){
        printk("g_p_canvas_pool is null(%x-%x)\n",(u32)g_p_canvas_pool,(u32)g_p_canvas_pool->cavas_pool_mem);
        return size;
    }
    
    memset(value,0x0,60);
    if(len >= 60-1 ){
        printk("set canvas static pool fail(len(%d))\n",len);
        return size;
    }
        
	if(copy_from_user(value, buf, len)){
        printk("set canvas static pool fail(copy_from_user)\n");
		return size;
    }
   	value[len] = '\0';
    sscanf(value, "%d %d\n",&ch,&mem);
   
    if(ch <= 0 || mem <= 0){
        printk("set canvas static pool fail(ch(%d)mem(%d))\n",ch,mem);
		return size; 
    }
    
    if(ch < SWOSG_ENG_DH_IDX_MIN || ch >= g_p_canvas_pool->osg_canvas_total_num){
        printk("set canvas static pool fail(ch(%d))\n",ch);
		return size;    
    }

    if((mem*SWOSG_ENG_KSIZE_BASE) > g_p_canvas_pool->osg_canvas_remaind_size){
        printk("set canvas static pool fail ch(%d) mem(%d)\n",ch,mem);
		return size;           
    }

    if(swosg_eng_config_canvas_pool_mem(ch,mem) < 0){
        return size;
    }

    //printk("(%d, %d)\n",ch,mem);
    return size;
    
}

int swosg_eng_proc_read_canvas_pool_info(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    int i = 0 ;

    if(swosg_eng_get_canvas_runmode() == SWOSG_ENG_CANVAS_DYAMIC){
        return sprintf(page, "osg canvas runmode:%s\n", "dymanic");    
    }
#if 1
    len += sprintf(page + len,"runmode:%s\n", "static");   

    if(g_p_canvas_pool == NULL){
        len += sprintf(page + len, "g_p_canvas_pool is null\n");
        return len;
    }
        
    len += sprintf(page + len, "tnum-alnum: %d-%d\n", g_p_canvas_pool->osg_canvas_total_num,g_p_canvas_pool->osg_canvas_num);
    len += sprintf(page + len, "alsz-resz-tsz: %d-%d-%d\n",g_p_canvas_pool->osg_canvas_alloc_size,g_p_canvas_pool->osg_canvas_remaind_size,g_p_canvas_pool->osg_canvas_totalsize);
    len += sprintf(page + len, "frambuf info:%x-%x-%d-%d\n"
                   ,(u32)g_p_canvas_pool->canvas_buf_info.va_addr ,(u32)g_p_canvas_pool->canvas_buf_info.phy_addr
                   ,g_p_canvas_pool->canvas_buf_info.size,g_p_canvas_pool->canvas_buf_info.alloc_type);
      
    for(i = 0; i< g_p_canvas_pool->osg_canvas_total_num;i++){
        len += sprintf(page + len, "id(%d) info:%x-%dK-%d-%d\n",i
                       ,g_p_canvas_pool->cavas_pool_mem[i].vbase
                       ,g_p_canvas_pool->cavas_pool_mem[i].mem_len/SWOSG_ENG_KSIZE_BASE,g_p_canvas_pool->cavas_pool_mem[i].usage
                       ,g_p_canvas_pool->cavas_pool_mem[i].setten);  

    }

#else
    len += sprintf(page + len,"osg canvas runmode:%s\n", "static");  

    if(g_p_canvas_pool == NULL){
        len += sprintf(page + len, "g_p_canvas_pool is null\n");
        return len;
    }
        
    len += sprintf(page + len, "osg canvas tnum-alnum: %d-%d\n", g_p_canvas_pool->osg_canvas_total_num,g_p_canvas_pool->osg_canvas_num);
    len += sprintf(page + len, "osg canvas alsz-resz-tsz: %d-%d-%d\n",g_p_canvas_pool->osg_canvas_alloc_size,g_p_canvas_pool->osg_canvas_remaind_size,g_p_canvas_pool->osg_canvas_totalsize);
    len += sprintf(page + len, "frambuf info:%x-%x-%d-%d\n"
                   ,(u32)g_p_canvas_pool->canvas_buf_info.va_addr ,(u32)g_p_canvas_pool->canvas_buf_info.phy_addr
                   ,g_p_canvas_pool->canvas_buf_info.size,g_p_canvas_pool->canvas_buf_info.alloc_type);
      
    for(i = 0; i< g_p_canvas_pool->osg_canvas_total_num;i++){
        len += sprintf(page + len, "pool id(%d) info:%x-%x-%dk-%d-%d\n",i
                       ,g_p_canvas_pool->cavas_pool_mem[i].vbase,g_p_canvas_pool->cavas_pool_mem[i].pbase
                       ,g_p_canvas_pool->cavas_pool_mem[i].mem_len/SWOSG_ENG_KSIZE_BASE,g_p_canvas_pool->cavas_pool_mem[i].usage
                       ,g_p_canvas_pool->cavas_pool_mem[i].setten);  

    }
#endif    
    return len;    
}

int swosg_eng_canvas_get_static_pool( int id, swosg_if_static_info * canvas)
{
    if(swosg_eng_get_canvas_runmode() == SWOSG_ENG_CANVAS_DYAMIC){
        DEBUG_M(LOG_WARNING,"Now run dynamic mode\n");
        memset(canvas,0x0,sizeof(swosg_if_static_info));
        return -1;   
    }
    
    if(canvas == NULL){
        DEBUG_M(LOG_WARNING,"get static canvas pool input is NULL\n");    
        return -1;
    } 
    if(id >= g_p_canvas_pool->osg_canvas_total_num || id < SWOSG_ENG_DH_IDX_MIN){
        DEBUG_M(LOG_WARNING,"get static canvas pool id out of range(%d)\n",id);
        memset(canvas,0x0,sizeof(swosg_if_static_info));
        return -1;
    }
    swosg_eng_lock_canvas_mutex();
    
    canvas->vbase = g_p_canvas_pool->cavas_pool_mem[id].vbase;
    canvas->pbase= g_p_canvas_pool->cavas_pool_mem[id].pbase;
    canvas->mem_len= g_p_canvas_pool->cavas_pool_mem[id].mem_len;
    canvas->usage= g_p_canvas_pool->cavas_pool_mem[id].usage;
    canvas->setten= g_p_canvas_pool->cavas_pool_mem[id].setten;
    
    swosg_eng_unlock_canvas_mutex();
    return 0;
}

#endif

int swosg_eng_canvas_set( int id, swosg_if_canvas_t * canvas)
{
    struct swosg_eng_canvas_cont * canvas_dat;
    
    if(id >= SWOSG_CANVAS_MAX || id < 0){
        DEBUG_M(LOG_WARNING,"set canvas id out of range(%d)\n",id);    
        return -1;
    }
    
    if(!canvas->src_cava_addr){
        DEBUG_M(LOG_WARNING,"illegle memory %x\n",canvas->src_cava_addr);    
        return -1;
    }    
    if(canvas->src_w == 0|| canvas->src_h == 0||canvas->src_w%2 || canvas->src_h%2){
        DEBUG_M(LOG_WARNING,"can't transfer to(%dx%d)\n",canvas->src_w,canvas->src_h);    
        return -1;
    }
    
#ifdef SUPPORT_SWOSG_CANVAS_STATIC
    if(swosg_eng_get_canvas_runmode() == SWOSG_ENG_CANVAS_STATIC){
        return swosg_eng_canvas_set_static( id,canvas);
    }
#endif

    swosg_eng_lock_canvas_mutex();
    
    canvas_dat =(struct swosg_eng_canvas_cont*)&g_swosg_eng_canvas_array[id].canvas_desc;
    if((canvas->src_w*canvas->src_h*2)%4 !=0){
        DEBUG_M(LOG_WARNING,"canvas size align error(%dx%d)\n",canvas->src_w,canvas->src_h);    
        goto err;
    }        
    
    if(g_swosg_eng_canvas_array[id].usage == 0)
    {
        canvas_dat->canvas_size = PAGE_ALIGN(canvas->src_w*canvas->src_h*2);
        canvas_dat->canvas_w = canvas->src_w;
        canvas_dat->canvas_h = canvas->src_h;
        canvas_dat->canvas_vaddr = dma_alloc_coherent(NULL, canvas_dat->canvas_size,&canvas_dat->canvas_paddr, GFP_DMA |GFP_KERNEL);

        if(!canvas_dat->canvas_vaddr){
            DEBUG_M(LOG_WARNING,"create canvas memory fail (%d)\n",id);
            goto err;
        }
        //printk("%s %d\n",__func__,__LINE__);
        swosg_eng_transfer_toyuv(canvas,canvas_dat);

        swosg_eng_get_dma_increase();
        g_swosg_eng_canvas_array[id].usage = 1;
    }
    else /*remove and update*/
    {
        g_swosg_eng_canvas_array[id].usage = 0;
        if((canvas->src_w*canvas->src_h*2) > canvas_dat->canvas_size)
        {
            if(canvas_dat->canvas_vaddr){
                dma_free_coherent(NULL, canvas_dat->canvas_size, canvas_dat->canvas_vaddr,canvas_dat->canvas_paddr);
                canvas_dat->canvas_vaddr = NULL;
                swosg_eng_get_dma_decrease();
            }
            else{
                DEBUG_M(LOG_WARNING,"remove illegle canvas memory (%d)\n",id);
                goto err;
            }
            canvas_dat->canvas_size = PAGE_ALIGN(canvas->src_w*canvas->src_h*2);
            canvas_dat->canvas_w = canvas->src_w;
            canvas_dat->canvas_h = canvas->src_h;
            canvas_dat->canvas_vaddr = dma_alloc_coherent(NULL, canvas_dat->canvas_size,&canvas_dat->canvas_paddr, GFP_DMA |GFP_KERNEL);

            if(!canvas_dat->canvas_vaddr){
                DEBUG_M(LOG_WARNING,"create canvas memory fail w-h-id(%d-%d-%d) sz=%d\n"
                        ,canvas->src_w,canvas->src_h,id,canvas_dat->canvas_size);
                goto err;
            }
            swosg_eng_get_dma_increase();
        }
        else{/*use original buffer*/
            canvas_dat->canvas_w = canvas->src_w;
            canvas_dat->canvas_h = canvas->src_h;
        }
        
        swosg_eng_transfer_toyuv(canvas,canvas_dat);
        
        g_swosg_eng_canvas_array[id].usage = 1;
    }
    
    swosg_eng_unlock_canvas_mutex();
    return 0;
err:
    swosg_eng_unlock_canvas_mutex();
    return -1;

}

int swosg_eng_canvas_get_info( int id, struct swosg_eng_canvas_desc * canvas)
{
    if(id >= SWOSG_CANVAS_MAX || id < 0){
        DEBUG_M(LOG_WARNING,"get canvas id out of range(%d)\n",id);    
        return -1;
    }
    swosg_eng_lock_canvas_mutex();
    memcpy(canvas,&g_swosg_eng_canvas_array[id],sizeof(struct swosg_eng_canvas_desc));
    swosg_eng_unlock_canvas_mutex();
    return 0;    
}

int swosg_eng_canvas_del( int id)
{
    struct swosg_eng_canvas_cont * canvas_dat;

    if(id >= SWOSG_CANVAS_MAX || id < 0){
        DEBUG_M(LOG_WARNING,"del canvas id out of range(%d)\n",id);    
        return -1;
    }
    
#ifdef SUPPORT_SWOSG_CANVAS_STATIC

    if(swosg_eng_get_canvas_runmode() == SWOSG_ENG_CANVAS_STATIC){
        return swosg_eng_canvas_del_static(id);
    }
#endif  

    swosg_eng_lock_canvas_mutex();
    
    if(g_swosg_eng_canvas_array[id].usage == 0){
        DEBUG_M(LOG_WARNING,"can't delete canvas (isn't using) (%d)\n",id);
        goto del_err;       
    }
        
    canvas_dat =(struct swosg_eng_canvas_cont*)&g_swosg_eng_canvas_array[id].canvas_desc;

    if(canvas_dat->canvas_vaddr){
        dma_free_coherent(NULL, canvas_dat->canvas_size, canvas_dat->canvas_vaddr,canvas_dat->canvas_paddr);
        canvas_dat->canvas_vaddr = NULL;
        swosg_eng_get_dma_decrease();
    }
    else{
        DEBUG_M(LOG_WARNING,"canvas memory isn't exist (%d)\n",id);
        goto del_err;    
    }
        
    memset(&g_swosg_eng_canvas_array[id],0x0,sizeof(struct swosg_eng_canvas_desc));
    swosg_eng_unlock_canvas_mutex();
    return 0; 
del_err:
    swosg_eng_unlock_canvas_mutex();
    return -1;
}


int swosg_eng_get_align_blit_xy(swosg_if_blit_param_t * blit_param,int *out_x,int *out_y
                                ,int src_w , int src_h)
{
    if(blit_param == NULL)
        return -1;

    switch(blit_param->align_type) {
        case SWOSG_WIN_ALIGN_TOP_LEFT:
            *out_x = blit_param->target_x;
            *out_y = blit_param->target_y;
            break;
        case SWOSG_WIN_ALIGN_TOP_CENTER:
            if (blit_param->dest_bg_w >= src_w)
                *out_x = (blit_param->dest_bg_w - src_w) / 2;
            else{
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_width(%d) < obj_width(%d)\n",
                        blit_param->dest_bg_w,  src_w);

                return -1;
            }
            *out_y = blit_param->target_y;
            break;
        case SWOSG_WIN_ALIGN_TOP_RIGHT:
            if (blit_param->dest_bg_w >= (blit_param->target_x + src_w))
                *out_x = blit_param->dest_bg_w - blit_param->target_x -src_w;
            else{
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_width(%d) < (in_x(%d) + obj_width(%d))\n",
                        blit_param->dest_bg_w , blit_param->target_x,src_w);
                return -1;
            }
            *out_y = blit_param->target_y;
            break;
        case SWOSG_WIN_ALIGN_BOTTOM_LEFT:
            *out_x = blit_param->target_x;
            if (blit_param->dest_bg_h >= (blit_param->target_y + src_h))
                *out_y = blit_param->dest_bg_h - blit_param->target_y - src_h;
            else{ 
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_height(%d) < (in_y(%d) + obj_height(%d))\n",
                        blit_param->dest_bg_h, blit_param->target_y,src_h);
                return -1;
            }    
            break;
        case SWOSG_WIN_ALIGN_BOTTOM_CENTER:
            if (blit_param->dest_bg_w  >= src_w)
                *out_x = (blit_param->dest_bg_w - src_w) / 2;
            else{
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_width(%d) < obj_width(%d)\n",
                        blit_param->dest_bg_w, src_w);
                return -1;
            }
                
            if (blit_param->dest_bg_h >= (blit_param->target_y +src_h))
                *out_y = blit_param->dest_bg_h - blit_param->target_y - src_h;
            else {
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_height(%d) < (in_y(%d) + obj_height(%d))\n",
                        blit_param->dest_bg_h, blit_param->target_y, src_h);
                return -1;
            }
            break;
       case SWOSG_WIN_ALIGN_BOTTOM_RIGHT:
            if (blit_param->dest_bg_w  >= (blit_param->target_x + src_w))
                *out_x = blit_param->dest_bg_w - blit_param->target_x - src_w;
            else{
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_width(%d) < (in_x(%d) + obj_width(%d))\n",
                        blit_param->dest_bg_w , blit_param->target_x,src_w);
                return -1;
            }    
            if (blit_param->dest_bg_h >= (blit_param->target_y  + src_h))
                *out_y = blit_param->dest_bg_h - blit_param->target_y - src_h;
            else{ 
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_height(%d) < (in_y(%d) + obj_height(%d))\n",
                        blit_param->dest_bg_h, blit_param->target_y, src_h);
                return -1;
            }
            break;
        case SWOSG_WIN_ALIGN_CENTER:
            if (blit_param->dest_bg_w >= src_w)
                *out_x = (blit_param->dest_bg_w - src_w) / 2;
            else{
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_width(%d) < obj_width(%d)\n",
                        blit_param->dest_bg_w, src_w);
                return -1;
            }
            if (blit_param->dest_bg_h >= src_h)
                *out_y = (blit_param->dest_bg_h - src_h) / 2;
            else{
                DEBUG_M(LOG_WARNING,"blit get_align_xy:src_height(%d) < obj_height(%d)\n",
                        blit_param->dest_bg_h, src_h);
                return -1;
            }
            break;
        default:
            DEBUG_M(LOG_WARNING,"Error,blit get_align_xy, align_type(undefined)=%d\n",blit_param->align_type);
            return -1;
    }
    return 0;    
}

static int __swosg_eng_do_blit(swosg_if_blit_param_t * blit_param,int fire)
{   
    unsigned int dx,dy,dw,dh,i;
    struct swosg_eng_canvas_cont *canvas_info ;
    think2d_osg_blit_t  think2d_blit_data;
#if 1 /*8287 dest was flush by last job*/   
    unsigned int        byte_sz;    
    frammap_buf_t       fram_buf_in;
#endif    
    if (blit_param == NULL)                                                                           
    { 
        DEBUG_M(LOG_WARNING,"swosg:blit_param is NULL\n");
        return -1;                                                                          
    }

    if (blit_param->dest_bg_w <= 0 || blit_param->dest_bg_h <= 0 )                                                           
    {     
        DEBUG_M(LOG_WARNING,"swosg:dest W or H is zero %d-%d\n",blit_param->dest_bg_w,blit_param->dest_bg_h);
        return -1;                                                                          
    }

    if( blit_param->src_patten_idx >= SWOSG_CANVAS_MAX || blit_param->src_patten_idx < 0
        || g_swosg_eng_canvas_array[blit_param->src_patten_idx].usage == 0 )
    {
        DEBUG_M(LOG_WARNING,"swosg:src canvas index is illegle(%d)\n",blit_param->src_patten_idx);
        if(blit_param->src_patten_idx < SWOSG_CANVAS_MAX &&  blit_param->src_patten_idx >= 0)
            DEBUG_M(LOG_WARNING,"swosg:src canvas usage is illegle(%d)\n",g_swosg_eng_canvas_array[blit_param->src_patten_idx].usage);

        for(i=0;i<SWOSG_CANVAS_MAX;i++)
        {
            if(g_swosg_eng_canvas_array[i].usage){
                DEBUG_M(LOG_WARNING,"swosg:src canvas id (%d) was set w-h-sz(%d-%d-%d) pa-va(%x-%x) count=%d\n"
                        ,i,g_swosg_eng_canvas_array[i].canvas_desc.canvas_w,g_swosg_eng_canvas_array[i].canvas_desc.canvas_h
                        ,g_swosg_eng_canvas_array[i].canvas_desc.canvas_size,(u32)g_swosg_eng_canvas_array[i].canvas_desc.canvas_paddr
                        ,(u32)g_swosg_eng_canvas_array[i].canvas_desc.canvas_vaddr,swosg_eng_get_dma_count());
             
            }
        }
        return -1;
    }
    
    canvas_info = &g_swosg_eng_canvas_array[blit_param->src_patten_idx].canvas_desc;

    if(canvas_info->canvas_vaddr == NULL){    
        DEBUG_M(LOG_WARNING,"swosg:src canvas memory is illegle %x\n"
                ,(unsigned int)canvas_info->canvas_vaddr);
        return -1;
    }
        
    dw = canvas_info->canvas_w;
    dh = canvas_info->canvas_h;


    if(blit_param->align_type >= SWOSG_WIN_ALIGN_MAX){
        DEBUG_M(LOG_WARNING,"swosg:blit align type is error (%d)\n",blit_param->align_type);
        return -1;
    }        

    if(swosg_eng_get_align_blit_xy(blit_param,&dx,&dy,canvas_info->canvas_w,canvas_info->canvas_h) < 0){
        DEBUG_M(LOG_WARNING,"swosg:blit get align_xy type (%d ) is error target x-y(%d-%d) dest w-h(%d-%d) src w-h(%d-%d)\n"
                ,blit_param->align_type,blit_param->target_x  ,blit_param->target_y,blit_param->dest_bg_w,blit_param->dest_bg_h
                ,canvas_info->canvas_w,canvas_info->canvas_h);
        return -1;        
    } 
    
    if((dx+dw) > (blit_param->dest_bg_w) || (dy+dh) > (blit_param->dest_bg_h)){

        DEBUG_M(LOG_WARNING,"swosg:blit range is error dest x-y(%d-%d) src w-h(%d-%d) dest w-h(%d-%d)\n"
                ,dx,dy,dw,dh,blit_param->dest_bg_w,blit_param->dest_bg_h);
        return -1;
    }

    memset(&think2d_blit_data,0x0,sizeof(think2d_osg_blit_t));

#if 1 /*8287 dest was flush by last job*/        
    memset(&fram_buf_in,0x0,sizeof(frammap_buf_t));

    fram_buf_in.va_addr = (void*)blit_param->dest_bg_addr;  
    
    if(frm_get_addrinfo(&fram_buf_in,0) == 0){
        think2d_blit_data.dst_paddr = (unsigned int)frm_va2pa(blit_param->dest_bg_addr);        
    }else{
        DEBUG_M(LOG_WARNING,"blit dest_bg_addr isn't from frammap(%x)\n",blit_param->dest_bg_addr);
        return -1;    
    }
    
    byte_sz = (blit_param->dest_bg_w*blit_param->dest_bg_h*2);
    
    if(byte_sz%DMA_CACHE_LINE_SZ){
        DEBUG_M(LOG_WARNING,"blit dest_bg_size isn't align cache size(%d)\n",byte_sz);
        return -1;    
    }    
  
#endif

    think2d_blit_data.dst_w = blit_param->dest_bg_w;
    think2d_blit_data.dst_h = blit_param->dest_bg_h;
    think2d_blit_data.src_paddr = (unsigned int)canvas_info->canvas_paddr;
    think2d_blit_data.src_w = canvas_info->canvas_w;
    think2d_blit_data.src_h = canvas_info->canvas_h;
    think2d_blit_data.dx    = dx;
    think2d_blit_data.dy    = dy;
    think2d_blit_data.src_colorkey = COLOUR8888(0xff,0xff,0xff,0xff);
   
    if(think2d_osg_do_blit(&think2d_blit_data,fire) < 0){        
        DEBUG_M(LOG_WARNING,"think2d_osg_do_blit is error\n");
        return -1;
    }
    //if(dx%2 == 1)
    //    dx-=1;    

    return 0;

}

int swosg_eng_do_blit(swosg_if_blit_param_t * blit_param,int num)
{
    int i,ret = 0;
    swosg_if_blit_param_t *tmp_param;

    swosg_eng_lock_canvas_mutex();
    for( i = 0;i < num;i++){
        tmp_param = &blit_param[i];
        
        if(__swosg_eng_do_blit(tmp_param,(i == num-1)?1:0) < 0){
            DEBUG_M(LOG_WARNING,"swosg:do blit is error %d-%d\n",i,num); 
            if(i == num-1)
                think2d_osg_remainder_fire();
			ret = -1;
            continue;
        }
    }
    swosg_eng_unlock_canvas_mutex();
    return ret;
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

int swosg_eng_check_border(swosg_if_mask_param_t * mask_param)
{
    int x1 = mask_param->target_x;
    int y1 = mask_param->target_y;
    int x2 = mask_param->target_x + mask_param->target_w - 1;
    int y2 = mask_param->target_y + mask_param->target_h - 1;

    if(y1 + mask_param->border_size> y2 - mask_param->border_size)
        return -1;
    if(x1 + mask_param->border_size > x2 - mask_param->border_size)
        return -1;  

    return 0;
    
}

int swosg_eng_get_align_mask_xy(swosg_if_mask_param_t * mask_param,int *out_x,int *out_y)
{
    if(mask_param == NULL)
        return -1;

    switch(mask_param->align_type) {
        case SWOSG_WIN_ALIGN_TOP_LEFT:
            *out_x = mask_param->target_x;
            *out_y = mask_param->target_y;
            break;
        case SWOSG_WIN_ALIGN_TOP_CENTER:
            if (mask_param->dest_bg_w >= mask_param->target_w)
                *out_x = (mask_param->dest_bg_w - mask_param->target_w) / 2;
            else{
                DEBUG_M(LOG_WARNING,"get_align_xy:src_width(%d) < obj_width(%d)\n",
                        mask_param->dest_bg_w,  mask_param->target_w);

                return -1;
            }
            *out_y = mask_param->target_y;
            break;
        case SWOSG_WIN_ALIGN_TOP_RIGHT:
            if (mask_param->dest_bg_w >= (mask_param->target_x + mask_param->target_w))
                *out_x = mask_param->dest_bg_w - mask_param->target_x - mask_param->target_w;
            else{
                DEBUG_M(LOG_WARNING,"get_align_xy:src_width(%d) < (in_x(%d) + obj_width(%d))\n",
                        mask_param->dest_bg_w , mask_param->target_x,  mask_param->target_w);
                return -1;
            }
            *out_y = mask_param->target_y;
            break;
        case SWOSG_WIN_ALIGN_BOTTOM_LEFT:
            *out_x = mask_param->target_x;
            if (mask_param->dest_bg_h >= (mask_param->target_y + mask_param->target_h))
                *out_y = mask_param->dest_bg_h - mask_param->target_y - mask_param->target_h;
            else{ 
                DEBUG_M(LOG_WARNING,"get_align_xy:src_height(%d) < (in_y(%d) + obj_height(%d))\n",
                        mask_param->dest_bg_h, mask_param->target_y, mask_param->target_h);
                return -1;
            }    
            break;
        case SWOSG_WIN_ALIGN_BOTTOM_CENTER:
            if (mask_param->dest_bg_w  >= mask_param->target_w)
                *out_x = (mask_param->dest_bg_w - mask_param->target_w) / 2;
            else{
                DEBUG_M(LOG_WARNING,"get_align_xy:src_width(%d) < obj_width(%d)\n",
                        mask_param->dest_bg_w, mask_param->target_w);
                return -1;
            }
                
            if (mask_param->dest_bg_h >= (mask_param->target_y + mask_param->target_h))
                *out_y = mask_param->dest_bg_h - mask_param->target_y - mask_param->target_h;
            else {
                DEBUG_M(LOG_WARNING,"get_align_xy:src_height(%d) < (in_y(%d) + obj_height(%d))\n",
                        mask_param->dest_bg_h, mask_param->target_y, mask_param->target_h);
                return -1;
            }
            break;
       case SWOSG_WIN_ALIGN_BOTTOM_RIGHT:
            if (mask_param->dest_bg_w  >= (mask_param->target_x + mask_param->target_w))
                *out_x = mask_param->dest_bg_w - mask_param->target_x - mask_param->target_w;
            else{
                DEBUG_M(LOG_WARNING,"get_align_xy:src_width(%d) < (in_x(%d) + obj_width(%d))\n",
                        mask_param->dest_bg_w , mask_param->target_x, mask_param->target_w);
                return -1;
            }    
            if (mask_param->dest_bg_h >= (mask_param->target_y  + mask_param->target_h))
                *out_y = mask_param->dest_bg_h - mask_param->target_y - mask_param->target_h;
            else{ 
                DEBUG_M(LOG_WARNING,"get_align_xy:src_height(%d) < (in_y(%d) + obj_height(%d))\n",
                        mask_param->dest_bg_h, mask_param->target_y, mask_param->target_h);
                return -1;
            }
            break;
        case SWOSG_WIN_ALIGN_CENTER:
            if (mask_param->dest_bg_w >= mask_param->target_w)
                *out_x = (mask_param->dest_bg_w - mask_param->target_w) / 2;
            else{
                DEBUG_M(LOG_WARNING,"get_align_xy:src_width(%d) < obj_width(%d)\n",
                        mask_param->dest_bg_w, mask_param->target_w);
                return -1;
            }
            if (mask_param->dest_bg_h >= mask_param->target_h)
                *out_y = (mask_param->dest_bg_h - mask_param->target_h) / 2;
            else{
                DEBUG_M(LOG_WARNING,"get_align_xy:src_height(%d) < obj_height(%d)\n",
                        mask_param->dest_bg_h, mask_param->target_h);
                return -1;
            }
            break;
        default:
            DEBUG_M(LOG_WARNING,"Error, align_type(undefined)=%d\n",mask_param->align_type);
            return -1;
    }
    return 0;    
}

int __swosg_eng_do_mask(swosg_if_mask_param_t * mask_param,int fire)
{   
    int x1,y1,x2,y2,border_s = 0;
    think2d_osg_mask_t     t2d_osgmask; 
    unsigned char          R,G,B;
#if 1 /*8287 dest was flush by last job*/  
    unsigned int           byte_sz;
    frammap_buf_t          fram_buf_in;
#endif    
    if(mask_param == NULL){
        DEBUG_M(LOG_WARNING,"mask_param is NULL\n");
        return -1;
    }
    

    if (mask_param->dest_bg_w <= 0 || mask_param->dest_bg_h <= 0 
        ||mask_param->target_w == 0 ||  mask_param->target_h ==0 ) 
    {
        DEBUG_M(LOG_WARNING,"swosg:rect range is error target w-h(%d-%d) dest w-h(%d-%d)\n"
                ,mask_param->target_w ,mask_param->target_h,mask_param->dest_bg_w, mask_param->dest_bg_h);
        return -1;                                                                          
    }

    if((mask_param->target_x + mask_param->target_w) > mask_param->dest_bg_w  || 
       (mask_param->target_y + mask_param->target_h) > mask_param->dest_bg_h )
    {
        DEBUG_M(LOG_WARNING,"swosg:rect param is error tar x-y(%d-%d) tar w-h(%d-%d) dest w-h(%d-%d)\n"
                ,mask_param->target_x,mask_param->target_y,mask_param->target_w,mask_param->target_h
                ,mask_param->dest_bg_w, mask_param->dest_bg_h);
        return -1;
    }

    if(swosg_eng_check_border(mask_param) < 0){
        DEBUG_M(LOG_WARNING,"swosg:check border is over range tar x-y-w-h(%d-%d-%d-%d) border sz (%d)\n"
                ,mask_param->target_x,mask_param->target_y,mask_param->target_w,mask_param->target_h,mask_param->border_size);
        return -1;
    }

    if(mask_param->align_type >= SWOSG_WIN_ALIGN_MAX){
        DEBUG_M(LOG_WARNING,"swosg:check alogn type is over range (%d)\n",mask_param->align_type);
        return -1;
    }        

    if(swosg_eng_get_align_mask_xy(mask_param,&x1,&y1) < 0){
        DEBUG_M(LOG_WARNING,"swosg:get alignxy type(%d) is error tar x-y-w-h(%d-%d-%d-%d) dest w-h(%d-%d)\n"
                ,mask_param->align_type,mask_param->target_x,mask_param->target_y,mask_param->target_w,mask_param->target_h
                ,mask_param->dest_bg_w,mask_param->dest_bg_h);
        return -1;        
    }
    
    if(mask_param->palette_idx >= SWOSG_PALETTE_MAX){
        DEBUG_M(LOG_WARNING,"osg:%s palette index is over %d\n",__func__,mask_param->palette_idx);
        return -1;
    }
    
    if(x1 % 2 !=0)
        x1=x1-1;
  
    if(mask_param->target_w %2 !=0)
       mask_param->target_w -= 1; 
    
    if(mask_param->border_type == SWOSGIF_BORDER_TURE){
        x1 = x1 + mask_param->border_size;
        y1 = y1 + mask_param->border_size;
        x2 = (x1+ mask_param->target_w - 1)- mask_param->border_size;
        y2 = (y1 + mask_param->target_h - 1)- mask_param->border_size;  
        border_s = y2-y1 +1;
    }
    else if(mask_param->border_type == SWOSGIF_BORDER_HORROW){
        x1 = x1 ;
        y1 = y1 ;
        x2 = x1 + mask_param->target_w - 1 ;
        y2 = y1 + mask_param->target_h - 1;
        border_s = (mask_param->border_size == 0) ? 1: mask_param->border_size;            
    }
    else{
        mask_param->border_type = SWOSGIF_BORDER_TURE;
        x1 = x1 ;
        y1 = y1 ;
        x2 = x1 + mask_param->target_w - 1 ;
        y2 = y1 + mask_param->target_h - 1;
        border_s = 2; 
    }

    if(border_s%2 != 0)
        border_s += 1;
    
    DEBUG_M(LOG_DEBUG,"%s rect xy=%dx%d wh=%dx%d add=%x size=%x type=%d\n",__func__, x1, \
            y1,x2,y2,(unsigned int)mask_param->dest_bg_addr,border_s,mask_param->border_type);

    memset(&t2d_osgmask,0x0,sizeof(think2d_osg_mask_t));
#if 1 /*8287 dest was flush by last job*/   
    memset(&fram_buf_in,0x0,sizeof(frammap_buf_t));

    fram_buf_in.va_addr = (void*)mask_param->dest_bg_addr;
    
    if(frm_get_addrinfo(&fram_buf_in,0) == 0){
        t2d_osgmask.dst_paddr = (unsigned int)frm_va2pa(mask_param->dest_bg_addr);        
    }else{
        DEBUG_M(LOG_WARNING,"dest_bg_addr isn't from frammap(%x)\n",mask_param->dest_bg_addr);
        return -1;    
    }
    byte_sz = (mask_param->dest_bg_w*mask_param->dest_bg_h*2);
    
    if(byte_sz%DMA_CACHE_LINE_SZ){
        DEBUG_M(LOG_WARNING,"dest_bg_size isn't align cache size(%d)\n",byte_sz);
        return -1;    
    } 
#endif    
    t2d_osgmask.dst_w =  mask_param->dest_bg_w;
    t2d_osgmask.dst_h =  mask_param->dest_bg_h;
    t2d_osgmask.dx1   =  x1;
    t2d_osgmask.dy1   =  y1;
    t2d_osgmask.dx2   =  x2;
    t2d_osgmask.dy2   =  y2;
    t2d_osgmask.border_sz = border_s;
    R=(OSG_GET_PALETEE(mask_param->palette_idx) & RGB565_MASK_RED) >> 11; 
    G=(OSG_GET_PALETEE(mask_param->palette_idx) & RGB565_MASK_GREEN) >> 5;
    B=(OSG_GET_PALETEE(mask_param->palette_idx) & RGB565_MASK_BLUE);
    t2d_osgmask.color     = COLOUR8888(R<<3, G<<2, B<<3,0);
    t2d_osgmask.type      = mask_param->border_type;

    if(think2d_osg_do_mask(&t2d_osgmask,fire) < 0){        
        DEBUG_M(LOG_WARNING,"think2d_osg_do_mask is error\n");
        return -1;
    }
    
    return 0;
}


int swosg_eng_do_mask(swosg_if_mask_param_t * mask_param,int num)
{
    int i,ret;
    swosg_if_mask_param_t *tmp_param;

    if(mask_param == NULL)
        return -1;
    swosg_eng_lock_canvas_mutex();
    for(i = 0 ; i < num ; i++){
        tmp_param = &mask_param[i];
          if(__swosg_eng_do_mask(tmp_param,(i == num-1)?1:0) < 0){
            DEBUG_M(LOG_WARNING,"swosg:do mask is error %d-%d\n",i,num);

            if(i == num-1)
                think2d_osg_remainder_fire();
			ret = -1;
            continue;
        }
 
    }
    swosg_eng_unlock_canvas_mutex();
    return ret;

}
#ifdef SUPPORT_SWOSG_CANVAS_STATIC
int swosg_eng_init(int chan_num , int total_mem)
{
    char buf_name[20];
    //u32  buf_tmem = 0;
    if(swosg_eng_judge_canvas_runmode(chan_num,total_mem) == SWOSG_ENG_CANVAS_DYAMIC){
        printk("osg canvas is dynammic allocate mode\n");
        swosg_eng_set_canvas_runmode(SWOSG_ENG_CANVAS_DYAMIC);
    }
    else{
        
        if(g_p_canvas_pool == NULL){
            g_p_canvas_pool = kzalloc(sizeof(struct swosg_eng_canvas_pool_desc), GFP_KERNEL);
            if(g_p_canvas_pool == NULL){
                DEBUG_M(LOG_WARNING,"swosg_eng_int can't allocate g_p_canvas_pool\n");
                goto init_err;
            }
            memset(g_p_canvas_pool,0x0,sizeof(struct swosg_eng_canvas_pool_desc));
        }

        g_p_canvas_pool->osg_canvas_total_num = chan_num;
               
        if(total_mem == 0 || (total_mem*SWOSG_ENG_KSIZE_BASE) > SWOSG_ENG_OSG_MAX_MEM){
            DEBUG_M(LOG_WARNING,"swosg_eng_init allocate mem out of range(%d>%d)\n",total_mem,SWOSG_ENG_OSG_MAX_MEM);
            kfree(g_p_canvas_pool);
            goto init_err;           
        }
        
        g_p_canvas_pool->canvas_buf_info.alloc_type = ALLOC_NONCACHABLE;
        sprintf(buf_name, "swosg_m");
	
        #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
        g_p_canvas_pool->canvas_buf_info.name = buf_name;
        #endif

        g_p_canvas_pool->osg_canvas_totalsize=(total_mem*SWOSG_ENG_KSIZE_BASE);
        g_p_canvas_pool->canvas_buf_info.size = g_p_canvas_pool->osg_canvas_totalsize;
        if (frm_get_buf_ddr(DDR_ID_SYSTEM, &g_p_canvas_pool->canvas_buf_info)) 
        {
            DEBUG_M(LOG_WARNING,"%s Failed to allocate from frammap sz(%d)\n", __FUNCTION__,g_p_canvas_pool->canvas_buf_info.size);
            kfree(g_p_canvas_pool);
            goto init_err;
        }
                
        spin_lock_init(&g_p_canvas_pool->canvas_pool_lock);

        g_p_canvas_pool->cavas_pool_mem = kzalloc(sizeof(struct swosg_eng_canvas_pool_mem)*chan_num, GFP_KERNEL);
        
        if(g_p_canvas_pool->cavas_pool_mem == NULL){
            DEBUG_M(LOG_WARNING,"swosg_eng_int can't allocate cavas_pool_mem\n");          
            kfree(g_p_canvas_pool);
            g_p_canvas_pool = NULL;
            goto init_err;
        }
        swosg_eng_init_canvas_pool_mem();
        swosg_eng_set_canvas_runmode(SWOSG_ENG_CANVAS_STATIC);
        printk("osg canvas is static allocate mode\n");
    }
     
    init_MUTEX(&palette_sem);
    init_MUTEX(&canvas_sem);
    memset(&g_swosg_eng_canvas_array[0],0x0,sizeof(struct swosg_eng_canvas_desc)*SWOSG_CANVAS_MAX);
    return 0;
init_err: 
    init_MUTEX(&palette_sem);
    init_MUTEX(&canvas_sem);
    memset(&g_swosg_eng_canvas_array[0],0x0,sizeof(struct swosg_eng_canvas_desc)*SWOSG_CANVAS_MAX);
    swosg_eng_set_canvas_runmode(SWOSG_ENG_CANVAS_DYAMIC);
    printk("init error back to osg canvas dynammic allocate mode\n");
    return 0;
}
#else
int swosg_eng_init(void)
{
    init_MUTEX(&palette_sem);
    init_MUTEX(&canvas_sem);
    memset(&g_swosg_eng_canvas_array[0],0x0,sizeof(struct swosg_eng_canvas_desc)*SWOSG_CANVAS_MAX);
    return 0;
}
#endif
void swosg_eng_exit(void)
{
    int i;    
    struct swosg_eng_canvas_cont * canvas_dat;
    
    swosg_eng_lock_canvas_mutex();
    if( swosg_eng_get_canvas_runmode() == SWOSG_ENG_CANVAS_DYAMIC){
        for(i = 0;i<SWOSG_CANVAS_MAX;i++)
        {
            if(g_swosg_eng_canvas_array[i].usage != 0)
            {
                canvas_dat =(struct swosg_eng_canvas_cont*)&g_swosg_eng_canvas_array[i].canvas_desc;
                if(canvas_dat->canvas_vaddr){
                    dma_free_coherent(NULL, canvas_dat->canvas_size, canvas_dat->canvas_vaddr,canvas_dat->canvas_paddr);
                    swosg_eng_get_dma_decrease();
                }    
                else
                    DEBUG_M(LOG_WARNING,"canvas memory isn't exist (%d)%d\n",i,__LINE__);
            }

        } 
    }
    
#ifdef SUPPORT_SWOSG_CANVAS_STATIC
    if(g_p_canvas_pool && swosg_eng_get_canvas_runmode() == SWOSG_ENG_CANVAS_STATIC){
        
        frm_free_buf_ddr(g_p_canvas_pool->canvas_buf_info.va_addr);
        kfree (g_p_canvas_pool->cavas_pool_mem);
        kfree (g_p_canvas_pool);
        g_p_canvas_pool = NULL;
    }

#endif

    swosg_eng_unlock_canvas_mutex();
   
}

