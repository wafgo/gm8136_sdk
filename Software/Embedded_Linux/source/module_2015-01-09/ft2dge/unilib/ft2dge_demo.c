/*  Revise History
 *  2013/11/15 : Add Constant Alpha Blend function (Kevin)
 *  2013/11/21 : fix drawing function doesn't driver_close_device after check fail (v1-1)
 *	2014/03/20 : support srocolor key example
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <frammap/frammap_if.h>
#include "ft2d_gfx.h"

static const char *g_imagefile;
int frammap_fd;  

static unsigned int rand_pool = 0x12345678;
static unsigned int rand_add  = 0x87654321;
#define COLOUR5551(r, g, b, a)  (((r) & 0x000000f8) << 8 | ((g) & 0x000000f8) << 3 | ((b) & 0x000000f8) >> 2 | ((a) & 0x00000080) >> 7)
#define COLOUR565(r, g, b)  (((r) & 0x000000f8) << 8 | ((g) & 0x000000fc) << 3 | ((b) & 0x000000f8) >> 3)

#define FT2D_DEMO_VERSION "1-2"
inline unsigned int demo_modesize(unsigned int mode)
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

static inline unsigned int myrand()
{
     rand_pool ^= ((rand_pool << 7) | (rand_pool >> 25));
     rand_pool += rand_add;
      rand_add  += rand_pool;

     return rand_pool;
}

static void print_usage()
{
     int i;

     printf (" ft2d Demo version  %s \n\n",FT2D_DEMO_VERSION);
     printf ("Usage: ft2d_demo [options]\n\n");
     printf ("Options:\n\n");
     printf ("  --fillrects  fill rectangle \n");
     printf ("  --drawline   draw line \n");
     printf ("  --drawrect   draw rectangle \n");
     printf ("  --blitter    <filename>  blitter filename , file format is WxH header and  RAW data(RAW565).\n");
     printf ("  --blit-alpha <filename> <alpha>  blitter filename , file format is WxH header and  RAW data(RAW565).\n");
     printf ("  --blit-srccolor <filename>  blitter filename , file format is WxH header and  RAW data(RAW565).\n");
     printf ("                                   alpha ,constant Alpha blend value.\n");
 
}

int draw_lines(void)
{
    FT2D_GfxDevice           gfx_device;
    FT2D_GraphicsDeviceFuncs* gfx_funcs =(FT2D_GraphicsDeviceFuncs*) &gfx_device.funcs;
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&gfx_device.drv_data ;
    FT2D_GFX_Setting_Data     *accel_data = (FT2D_GFX_Setting_Data*)&gfx_device.accel_setting_data ;
    FT2DRegion      line;
    FT2DRectangle      rect;
    int SW ,SH,SX = 100,SY=100 ,i,j,x,y,dx,dy;
             
    memset(&gfx_device,0x0 ,sizeof(FT2D_GfxDevice));

    if(driver_init_device(&gfx_device , gfx_funcs , FT2D_RGB_5650)< 0){
        printf("%s init device fail\n",__func__);
        return -1;    
    }    
    /*target surface is fb*/
    memcpy(&drv_data->target_sur_data , &drv_data->fb_data ,sizeof(FT2D_Surface_Data));
    gfx_device.accel_do = FT2D_FILLINGRECT;
    gfx_device.accel_param = FT2D_COLOR_SET | FT2D_DSTINATION_SET;
    accel_data->color      = 0x0;
     
    if(!gfx_funcs->CheckState(&gfx_device) && !gfx_funcs->SetState(&gfx_device)){
         rect.x = 0 , rect.y = 0;
         SW = rect.w = drv_data->target_sur_data.width;
         SH = rect.h = drv_data->target_sur_data.height;  
         gfx_funcs->FillRectangle(&gfx_device , &rect);
         gfx_funcs->EmitCommands(&gfx_device);
         
         gfx_device.accel_do = FT2D_DRAWLINE;  
         gfx_device.accel_param |= FT2D_LINE_STYLE_SET;       
         for (i=1; i%100 ; i++) {        
              
          for (j=0; j<100; j++) {
               accel_data->color = COLOUR5551(myrand()&0xFE, myrand()&0xFE, myrand()&0xFE,0xFF);
               accel_data->line_style = FT2D_STYLE_SOLID;//myrand()%FT2D_MAX_LINESTYLE;
               if( gfx_funcs->CheckState(&gfx_device) < 0 || gfx_funcs->SetState(&gfx_device) < 0){
                   printf("%s setstate or check state fail %s\n",__func__,__LINE__); 
                   driver_close_device(&gfx_device);  
                   return -1;
               }
               x  = myrand() % (SW-SX) + SX/2;
               y  = myrand() % (SH-SY) + SY/2;
               dx = myrand() % (2*SX) - SX;
               dy = myrand() % (2*SY) - SY;

               line.x1 = x - dx/2;
               line.y1 = y - dy/2;
               line.x2 = x + dx/2;
               line.y2 = y + dy/2;
     
               gfx_funcs->DrawLine(&gfx_device , &line);
          }
          gfx_funcs->EmitCommands(&gfx_device);
     
       }
     }
     else
        printf("%s setstate or check state fail %s\n",__func__,__LINE__);  

     driver_close_device(&gfx_device);  
     return 0;   
}

int draw_rects( void )
{
    FT2D_GfxDevice           gfx_device;
    FT2D_GraphicsDeviceFuncs* gfx_funcs =(FT2D_GraphicsDeviceFuncs*) &gfx_device.funcs;
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&gfx_device.drv_data ;
    FT2D_GFX_Setting_Data     *accel_data = (FT2D_GFX_Setting_Data*)&gfx_device.accel_setting_data ;
    FT2DRectangle      rect;
    int SW ,SH,SX = 100,SY=100 ,i,j;
             
    memset(&gfx_device,0x0 ,sizeof(FT2D_GfxDevice));

    if(driver_init_device(&gfx_device , gfx_funcs , FT2D_RGB_5650)< 0){
        printf("%s init device fail\n",__func__);
        return -1;    
    }    
    /*target surface is fb*/
    memcpy(&drv_data->target_sur_data , &drv_data->fb_data ,sizeof(FT2D_Surface_Data));
    gfx_device.accel_do = FT2D_FILLINGRECT;
    gfx_device.accel_param = FT2D_COLOR_SET | FT2D_DSTINATION_SET;
    accel_data->color      = 0x0;
     
    if(!gfx_funcs->CheckState(&gfx_device) && !gfx_funcs->SetState(&gfx_device)){
         rect.x = 0 , rect.y = 0;
         SW = rect.w = drv_data->target_sur_data.width;
         SH = rect.h = drv_data->target_sur_data.height;  
         gfx_funcs->FillRectangle(&gfx_device , &rect);
         gfx_funcs->EmitCommands(&gfx_device);
         
         gfx_device.accel_do = FT2D_DRAWRECT; 
         gfx_device.accel_param |= FT2D_LINE_STYLE_SET;       
         for (i=1; i%5 ; i++) {        
              
          for (j=0; j<30; j++) {
               accel_data->color = COLOUR5551(myrand()&0xFE, myrand()&0xFE, myrand()&0xFE,0xFF);
               accel_data->line_style = FT2D_STYLE_DOT;//myrand()%FT2D_MAX_LINESTYLE;
               if( gfx_funcs->CheckState(&gfx_device) < 0 || gfx_funcs->SetState(&gfx_device) < 0){
                   printf("%s setstate or check state fail %s\n",__func__,__LINE__);
                   driver_close_device(&gfx_device);   
                   return -1;
               }
               rect.x = (SW!=SX)?(myrand()%(SW-SX)):0;
               rect.y = (SH!=SY)?(myrand()%(SH-SY)):0;
               rect.w = SX;
               rect.h = SY;
               gfx_funcs->DrawRectangle(&gfx_device , &rect);
          }
          gfx_funcs->EmitCommands(&gfx_device);
     
       }
     }
     else
        printf("%s setstate or check state fail %s\n",__func__,__LINE__);  

     driver_close_device(&gfx_device);
     return 0;    
}

int fill_rects( void )
{
    FT2D_GfxDevice           gfx_device;
    FT2D_GraphicsDeviceFuncs* gfx_funcs =(FT2D_GraphicsDeviceFuncs*) &gfx_device.funcs;
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&gfx_device.drv_data ;
    FT2D_GFX_Setting_Data     *accel_data = (FT2D_GFX_Setting_Data*)&gfx_device.accel_setting_data ;
    FT2DRectangle      rect;
    int SW ,SH,SX = 50,SY=50 ,i,j;
             
    memset(&gfx_device,0x0 ,sizeof(FT2D_GfxDevice));

    if(driver_init_device(&gfx_device , gfx_funcs , FT2D_RGB_5650)< 0){
        printf("%s init device fail\n",__func__);
        return -1;    
    }    
    /*target surface is fb*/
    memcpy(&drv_data->target_sur_data , &drv_data->fb_data ,sizeof(FT2D_Surface_Data));
    gfx_device.accel_do = FT2D_FILLINGRECT;
    gfx_device.accel_param = FT2D_COLOR_SET | FT2D_DSTINATION_SET;
    accel_data->color      = 0x0;
     
    if(!gfx_funcs->CheckState(&gfx_device) && !gfx_funcs->SetState(&gfx_device)){
         rect.x = 0 , rect.y = 0;
         SW = rect.w = drv_data->target_sur_data.width;
         SH = rect.h = drv_data->target_sur_data.height; 
         printf("SW=%d ,SH=%d\n" , SW,SH);
         printf("SW=%d ,SH=%d\n" ,rect.w,rect.h);
         gfx_funcs->FillRectangle(&gfx_device , &rect);
                 
         for (i=1; i%1000 ; i++) {        
              
          for (j=0; j<100; j++) {
               accel_data->color = COLOUR5551(myrand()&0xFF, myrand()&0xFF, myrand()&0xFF,0xFF);
               if( gfx_funcs->CheckState(&gfx_device) < 0 || gfx_funcs->SetState(&gfx_device) < 0){
                   printf("%s setstate or check state fail %s\n",__func__,__LINE__); 
                   driver_close_device(&gfx_device);  
                   return -1;
               }
               rect.x = (SW!=SX)?(myrand()%(SW-SX)):0;
               rect.y = (SH!=SY)?(myrand()%(SH-SY)):0;
               rect.w = SX;
               rect.h = SY;
               gfx_funcs->FillRectangle(&gfx_device , &rect);
          }
          
          gfx_funcs->EmitCommands(&gfx_device);
       }
     }
     else
        printf("%s setstate or check state fail %s\n",__func__,__LINE__);  

     driver_close_device(&gfx_device);
     return 0;
}

static int create_src_surface(FT2D_GfxDevice   *gfx_device,const char *filename ,FT2D_BPP_T rgb_t)
{
    FT2D_GfxDrv_Data  *drv_data = (FT2D_GfxDrv_Data*)&gfx_device->drv_data ;
    FT2D_Surface_Data *src_data = (FT2D_Surface_Data*)&drv_data->source_sur_data;    
    FILE * filer;
    unsigned int blt_src_size,tmp_src_add,i,j;
    unsigned int tmp_pix_da=0x0;
    unsigned char *tmp_da;
    //unsigned int *tmp_da;

    if((filer=fopen(filename,"rb"))==NULL){
    	printf("open read file error.\n");		
    	return -1;
    }
    /*read RGB565 HEADER weigh x heigh ,the bin file must has 8 bytes header*/
    /*to describe weigh x heigh ,please use pic2raw utility to translate*/
    fread((void*)&src_data->width,1,4,filer);	
    fread((void*)&src_data->height,1,4,filer);	
   
    frammap_fd = open("/dev/frammap0", O_RDWR);
    if (frammap_fd < 0) {
        printf("Error to open framap_fd fd! \n");
    	fclose(filer);
        return -1;
    }
    src_data->bpp = rgb_t;
    blt_src_size  = (((src_data->width * src_data->height * demo_modesize(rgb_t))+4095) >>12)<<12;
    
    src_data->surface_vaddr = (unsigned int)mmap(NULL, blt_src_size,
                                            PROT_READ | PROT_WRITE, MAP_SHARED, frammap_fd, 0);

    tmp_src_add = src_data->surface_vaddr;
    //printf("src vaddr=%x\n!",tmp_src_add);

    if (ioctl(frammap_fd, FRM_ADDR_V2P, &tmp_src_add) < 0) {
        printf("Error to call FRM_ADDR_V2P! \n");
    	close(frammap_fd);
    	fclose(filer);
        return -1;
    }

    src_data->surface_paddr= tmp_src_add;

    if ((int)src_data->surface_paddr <= 0) {
        printf("Error to mmap lcd fb! \n");
    	close(frammap_fd);
    	fclose(filer);
        return -1;
    }    

    tmp_da = (unsigned char *)src_data->surface_vaddr;
    while(1){        
    	fread((void*)tmp_da,1,1,filer);
        tmp_da ++;
            
    	if(feof(filer)){
            printf("feof!\n");
            break;		
    	}
    }  
    fclose(filer);
    return 0;    
    
}
static int destroy_src_surface(FT2D_GfxDevice   *gfx_device,FT2D_BPP_T rgb_t)
{
    FT2D_GfxDrv_Data  *drv_data = (FT2D_GfxDrv_Data*)&gfx_device->drv_data ;
    FT2D_Surface_Data *src_data = (FT2D_Surface_Data*)&drv_data->source_sur_data;
    int blt_src_size; 
    
    blt_src_size  = (((src_data->width * src_data->height * demo_modesize(rgb_t))+4095) >>12)<<12;     
    munmap((void *)src_data->surface_vaddr, blt_src_size); 
    close(frammap_fd);   
}

int blitter(const char *filename)
{
    FT2D_GfxDevice           gfx_device;
    FT2D_GraphicsDeviceFuncs* gfx_funcs =(FT2D_GraphicsDeviceFuncs*) &gfx_device.funcs;
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&gfx_device.drv_data ;
    FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*)&drv_data->target_sur_data;
    FT2D_GFX_Setting_Data     *accel_data = (FT2D_GFX_Setting_Data*)&gfx_device.accel_setting_data ;
    FT2DRectangle      rect;  
    int SW ,SH,SX = 100,SY=100 ,i;
    
    memset(&gfx_device,0x0 ,sizeof(FT2D_GfxDevice));

    if(driver_init_device(&gfx_device , gfx_funcs , FT2D_RGB_5650)< 0){
        printf("%s init device fail\n",__func__);
        return -1;    
    }
      
    if(create_src_surface(&gfx_device ,filename,FT2D_RGB_5650) < 0){
        printf("%s create src surface fail\n",__func__);
        goto err;    
    }
    
    /*target surface is fb*/
    memcpy(tar_data , &drv_data->fb_data ,sizeof(FT2D_Surface_Data));
    gfx_device.accel_do = FT2D_FILLINGRECT;
    gfx_device.accel_param = FT2D_COLOR_SET | FT2D_DSTINATION_SET;
    accel_data->color      = 0x0;
    if(!gfx_funcs->CheckState(&gfx_device) && !gfx_funcs->SetState(&gfx_device)){
         rect.x = 0 , rect.y = 0;
         SW = rect.w = drv_data->target_sur_data.width;
         SH = rect.h = drv_data->target_sur_data.height; 
         printf("SW=%d ,SH=%d\n" , SW,SH);
         gfx_funcs->FillRectangle(&gfx_device , &rect);
         gfx_funcs->EmitCommands(&gfx_device);
    }
    
    gfx_device.accel_do = FT2D_BLITTING;
    gfx_device.accel_param =  FT2D_DSTINATION_SET | FT2D_SOURCE_SET | FT2D_ROP_METHOD_SET ;
    accel_data->rop_method = FT2D_ROP_SRCCOPY;

    if(!gfx_funcs->CheckState(&gfx_device) && !gfx_funcs->SetState(&gfx_device)){
        rect.x = 0 , rect.y = 0;
        rect.w = drv_data->source_sur_data.width;
        rect.h = drv_data->source_sur_data.height; 
        for (i=1; i%2; i++) {
            gfx_funcs->Blit(&gfx_device, &rect,0,0);

            }
        gfx_funcs->EmitCommands(&gfx_device);
    }
    
    destroy_src_surface(&gfx_device,FT2D_RGB_5650);
        
    driver_close_device(&gfx_device);
    return 0;   
err:    
    driver_close_device(&gfx_device);
    return -1;
 
}

int blitter_srccolor(const char *filename)
{
    FT2D_GfxDevice           gfx_device;
    FT2D_GraphicsDeviceFuncs* gfx_funcs =(FT2D_GraphicsDeviceFuncs*) &gfx_device.funcs;
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&gfx_device.drv_data ;
    FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*)&drv_data->target_sur_data;
    FT2D_GFX_Setting_Data     *accel_data = (FT2D_GFX_Setting_Data*)&gfx_device.accel_setting_data ;
    FT2DRectangle      rect;  
    int SW ,SH,SX = 100,SY=100 ,i;
    
    memset(&gfx_device,0x0 ,sizeof(FT2D_GfxDevice));

    if(driver_init_device(&gfx_device , gfx_funcs , FT2D_RGB_5650)< 0){
        printf("%s init device fail\n",__func__);
        return -1;    
    }
      
    if(create_src_surface(&gfx_device ,filename,FT2D_RGB_5650) < 0){
        printf("%s create src surface fail\n",__func__);
        goto err;    
    }
    
    /*target surface is fb*/
    memcpy(tar_data , &drv_data->fb_data ,sizeof(FT2D_Surface_Data));
    gfx_device.accel_do = FT2D_FILLINGRECT;
    gfx_device.accel_param = FT2D_COLOR_SET | FT2D_DSTINATION_SET;
    accel_data->color      = 0x00;
    if(!gfx_funcs->CheckState(&gfx_device) && !gfx_funcs->SetState(&gfx_device)){
         rect.x = 0 , rect.y = 0;
         SW = rect.w = drv_data->target_sur_data.width;
         SH = rect.h = drv_data->target_sur_data.height; 
         printf("SW=%d ,SH=%d\n" , SW,SH);
         gfx_funcs->FillRectangle(&gfx_device , &rect);
         gfx_funcs->EmitCommands(&gfx_device);
    }
    
    gfx_device.accel_do = FT2D_BLITTING;
    gfx_device.accel_param =  FT2D_DSTINATION_SET | FT2D_SOURCE_SET |  FT2D_ROP_METHOD_SET | FT2D_SRC_COLORKEY_SET ;
    accel_data->rop_method = FT2D_ROP_SRCCOPY;
    accel_data->src_color = COLOUR565(0xff,0xff,0xff);/*please check your src color key format, our example is white*/

    if(!gfx_funcs->CheckState(&gfx_device) && !gfx_funcs->SetState(&gfx_device)){
        rect.x = 0 , rect.y = 0;
        rect.w = drv_data->source_sur_data.width;
        rect.h = drv_data->source_sur_data.height; 
        for (i=1; i%2; i++) {
            gfx_funcs->Blit(&gfx_device, &rect,0,0);

            }
        gfx_funcs->EmitCommands(&gfx_device);
    }
    
    destroy_src_surface(&gfx_device,FT2D_RGB_5650);
        
    driver_close_device(&gfx_device);
    return 0;   
err:    
    driver_close_device(&gfx_device);
    return -1;
 
}
int blitter_alpha(const char *filename,int alp_val)
{
    FT2D_GfxDevice           gfx_device;
    FT2D_GraphicsDeviceFuncs* gfx_funcs =(FT2D_GraphicsDeviceFuncs*) &gfx_device.funcs;
    FT2D_GfxDrv_Data * drv_data = (FT2D_GfxDrv_Data*)&gfx_device.drv_data ;
    FT2D_Surface_Data *tar_data = (FT2D_Surface_Data*)&drv_data->target_sur_data;
    FT2D_GFX_Setting_Data     *accel_data = (FT2D_GFX_Setting_Data*)&gfx_device.accel_setting_data ;
    FT2DRectangle      rect;  
    int SW ,SH,SX = 100,SY=100 ,i;
    
    memset(&gfx_device,0x0 ,sizeof(FT2D_GfxDevice));

    if(driver_init_device(&gfx_device , gfx_funcs , FT2D_RGB_5650)< 0){
        printf("%s init device fail\n",__func__);
        return -1;    
    }
      
    if(create_src_surface(&gfx_device ,filename,FT2D_RGB_5650) < 0){
        printf("%s create src surface fail\n",__func__);
        goto err;    
    }
    
    /*target surface is fb*/
    memcpy(tar_data , &drv_data->fb_data ,sizeof(FT2D_Surface_Data));
    gfx_device.accel_do = FT2D_FILLINGRECT;
    gfx_device.accel_param = FT2D_COLOR_SET | FT2D_DSTINATION_SET;
    accel_data->color      = 0x0;
    if(!gfx_funcs->CheckState(&gfx_device) && !gfx_funcs->SetState(&gfx_device)){
         rect.x = 0 , rect.y = 0;
         SW = rect.w = drv_data->target_sur_data.width;
         SH = rect.h = drv_data->target_sur_data.height; 
         printf("SW=%d ,SH=%d\n" , SW,SH);
         gfx_funcs->FillRectangle(&gfx_device , &rect);
         gfx_funcs->EmitCommands(&gfx_device);
    }
    
    gfx_device.accel_do = FT2D_BLITTING;
    gfx_device.accel_param =  FT2D_DSTINATION_SET | FT2D_SOURCE_SET |FT2D_FORCE_FG_ALPHA;
    accel_data->color = (alp_val & 0xff);

    if(!gfx_funcs->CheckState(&gfx_device) && !gfx_funcs->SetState(&gfx_device)){
        rect.x = 0 , rect.y = 0;
        rect.w = drv_data->source_sur_data.width;
        rect.h = drv_data->source_sur_data.height; 
        for (i=1; i%2; i++) {
            gfx_funcs->Blit(&gfx_device, &rect,0,0);

            }
        gfx_funcs->EmitCommands(&gfx_device);
    }
    
    destroy_src_surface(&gfx_device,FT2D_RGB_5650);
        
    driver_close_device(&gfx_device);
    return 0;   
err:    
    driver_close_device(&gfx_device);
    return -1;
 
}
void main(int argc, char *argv[])
{
    int do_fillrects = 0  , do_drawline = 0, do_drawrect = 0 , do_blitter = 0 ,do_blitter_alpa=0;
    int n , al_v = 0,do_blitter_srccolor =0;
    char image_file[100];
       
    memset(image_file , 0x0 ,100);    
    for (n = 1; n < argc; n++) {
        if (strncmp (argv[n], "--", 2) == 0) {
            if (strcmp (argv[n] + 2, "fillrects") == 0 ){
                    do_fillrects = 1; 
                    continue; 
            }
            else if (strcmp (argv[n] + 2, "drawline") == 0 ){
                    do_drawline = 1; 
                    continue;
            }
            else if (strcmp (argv[n] + 2, "drawrect") == 0){ 
                    do_drawrect = 1;
                    continue;
            }
            else if (strcmp (argv[n] + 2, "blitter") == 0 &&
                     ++n < argc && argv[n]){
                    g_imagefile = argv[n];         
                    do_blitter = 1;
                    continue;
            }
            else if (strcmp (argv[n] + 2, "blit-alpha") == 0 &&
                    ++n < argc && argv[n]){
                    g_imagefile = argv[n] ;
                    ++n;  
                    sscanf (argv[n], "%d",&al_v);   
                    do_blitter_alpa = 1;
                    continue;   
            }
            else if (strcmp (argv[n] + 2, "blit-srccolor") == 0 &&
                    ++n < argc && argv[n]){
                    g_imagefile = argv[n] ;
                    do_blitter_srccolor = 1;
                    continue;   
            }
            else if (strcmp (argv[n] + 2, "help") == 0){
                    print_usage();
                    break;
                               
            }else
                print_usage();           
        }
        else
            print_usage();        
    }
    
    if(do_fillrects)
        fill_rects();
        
    if(do_drawrect)
        draw_rects(); 
    if(do_drawline)
        draw_lines();  
    if(do_blitter) 
        blitter(g_imagefile);  
    if(do_blitter_alpa) 
        blitter_alpha(g_imagefile,al_v); 
    if(do_blitter_srccolor)
        blitter_srccolor(g_imagefile);    
                      
        
}
