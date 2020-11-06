#ifndef __FT2D_GFX_H__
#define __FT2D_GFX_H__

/*
 * A rectangle specified by a point and a dimension.
 */
typedef struct {
     int            x;   /* X coordinate of its top-left point */
     int            y;   /* Y coordinate of its top-left point */
     int            w;   /* width of it */
     int            h;   /* height of it */
} FT2DRectangle;

/*
 * A region specified by two points.
 *
 * The defined region includes both endpoints.
 */
typedef struct {
     int            x1;  /* X coordinate of top-left point */
     int            y1;  /* Y coordinate of top-left point */
     int            x2;  /* X coordinate of lower-right point */
     int            y2;  /* Y coordinate of lower-right point */
} FT2DRegion;

typedef enum {
     FT2D_FILLINGRECT           = 0x00000001,/*FT2D ,THINK2D*/
     FT2D_DRAWLINE              = 0x00000002,/*FT2D ,THINK2D*/
     FT2D_DRAWRECT              = 0x00000004,/*FT2D ,THINK2D*/
     FT2D_BLITTING              = 0x00000008,/*FT2D ,THINK2D*/
     FT2D_STRETCHBLITTING       = 0x00000010,/*THINK2D*/
} FT2D_AccelFlags_T;

typedef enum {
     FT2D_CLIPPING_FUNC           = 0x00000001,
     FT2D_ROP_FUNC                = 0x00000002,
     FT2D_POTERDUFF_FUNC          = 0x00000004,
     FT2D_SOURCEALPHA_FUNC        = 0x00000008,
     FT2D_DIFFSURFACE_TRANS_FUNC  = 0x00000010,
     FT2D_DRAWING_FUNC            = 0x00000020,
     FT2D_BLITTER_FUNC            = 0x00000040,
} FT2D_CapabilitiesFlags_T;
typedef enum {
     FT2D_COLOR_SET             = 0x00000001, /*FT2D ,THINK2D*/
     FT2D_COLORBLEND_SET        = 0x00000002, /*THINK2D*/
     FT2D_FG_COLORIZE_SET       = 0x00000004, /*THINK2D*/
     FT2D_DSTINATION_SET        = 0x00000008, /*FT2D ,THINK2D*/
     FT2D_SOURCE_SET            = 0x00000008, /*FT2D ,THINK2D*/
     FT2D_CLIP_SET              = 0x00000010, /*THINK2D*/
     FT2D_DEG090_SET            = 0x00000020, /*THINK2D*/
     FT2D_DEG180_SET            = 0x00000040, /*THINK2D*/
     FT2D_DEG270_SET            = 0x00000080, /*THINK2D*/
     FT2D_FLIP_HORIZONTAL_SET   = 0x00000100, /*THINK2D*/
     FT2D_FLIP_VERTICAL_SET     = 0x00000200, /*THINK2D*/
     FT2D_SRC_COLORKEY_SET      = 0x00000400, /*THINK2D*/
     FT2D_DST_COLORKEY_SET      = 0x00000800, /*THINK2D*/
     FT2D_LINE_STYLE_SET        = 0x00000400, /*FT2D*/
     FT2D_ROP_METHOD_SET        = 0x00000800, /*FT2D*/
     FT2D_FORCE_FG_ALPHA        = 0x00001000, /*THINK2D*/
} FT2D_SettingFlags_T;

typedef enum {
	FT2D_RGB_5650  ,	//16bits, DSPF_RGB16
	FT2D_ARGB_1555 ,	//16bits, DSPF_ARGB1555
	FT2D_RGBA_5551 ,	//16bits, DSPF_ARGB1555
	FT2D_RGBA4444  ,
	FT2D_RGB_888   ,	//32bits, DSPF_RGB32
	FT2D_ARGB_8888 ,	//32bits, DSPF_ARGB
	FT2D_RGBA_8888 , 	//32bits, DSPF_ARGB
	FT2D_XRGB_8888 	//32bits, DSPF_ARGB
} FT2D_BPP_T;

typedef struct _FT2D_GFX_Setting_Data{
    unsigned int   color;         /*for FT2D_COLOR_SET , FT2D_FG_COLORIZE_SET*/
    unsigned short src_blend;     /*for FT2D_COLORBLEND_SET*/
    unsigned short dst_blend;     /*for FT2D_COLORBLEND_SET*/
    FT2DRegion     clip;          /*FT2D_CLIP_SET*/
    unsigned int   src_color;     /*FT2D_SRC_COLORKEY_SET*/
    unsigned int   dst_color;     /*FT2D_DST_COLORKEY_SET*/
    unsigned int   line_style;    /*FT2D_LINE_STYLE_SET*/
    unsigned int   rop_method ;   /*FT2D_FORCE_FG_ALPHA*/
}FT2D_GFX_Setting_Data;

typedef struct _FT2D_GraphicsDeviceFuncs {

    void (*EngineReset)(void *device );  
    /*
     * emit any buffered commands, i.e. trigger processing
     */
    int (*EmitCommands)  ( void *device );    /*FT2D ,THINK2D*/
     /*
      * Check if the function 'accel' can be accelerated and parameter can be set.
      */
    int (*CheckState)( void *device);         /*FT2D ,THINK2D*/

     /*
      * This function will follow device->accel_do to set parameter and value.  
      */
    int (*SetState)  ( void *device);         /*FT2D ,THINK2D*/
    int (*GetSupportFeature)    ( void *device );
    /*
     * drawing functions
     */
    int (*FillRectangle) ( void *device,FT2DRectangle *rect );  /*FT2D ,THINK2D*/

    int (*DrawRectangle) ( void *device,FT2DRectangle *rect );  /*FT2D ,THINK2D*/

    int (*DrawLine)      ( void *device,FT2DRegion *line );     /*FT2D ,THINK2D*/ 
    
    /*
     * blitting functions
     */
    int (*Blit)         ( void *device , FT2DRectangle *rect, int dx, int dy );  /*FT2D ,THINK2D*/

 
    int (*StretchBlit)  ( void *device , FT2DRectangle *srect, FT2DRectangle *drect );  /*THINK2D*/         

}FT2D_GraphicsDeviceFuncs;

typedef struct __FT2D_Surface_Data{
    unsigned int surface_vaddr;  /* surface virtual address */
    unsigned int surface_paddr;  /* surface physical address */
    int width;                   /* surface width */
    int height;                  /* surface height */
    FT2D_BPP_T bpp;              /* surface format */
}FT2D_Surface_Data;

typedef struct __FT2D_GfxDrv_Data{
    int gfx_fd;         /*2D engine fd*/
    int fb_fd ;         /*frame buffer engine fd*/
    void * gfx_shared;  /*driver mode command memory*/
    int    gfx_map_sz;  /*driver mode command memory size*/
    void * gfx_local;   /*user mode local command memory*/
    volatile unsigned int  *mmio_base; /*driver io mapping base*/
    FT2D_Surface_Data  fb_data;         /*frame buffer surface data*/ 
    FT2D_Surface_Data  target_sur_data; /*target surface data*/ 
    FT2D_Surface_Data  source_sur_data; /*source surface data*/   
 
}FT2D_GfxDrv_Data;

typedef struct {
     FT2D_CapabilitiesFlags_T   flags;
     FT2D_AccelFlags_T          accel_func;
} FT2D_Capabilities;

typedef struct __FT2D_GfxDevice {
     FT2D_GfxDrv_Data          drv_data;    /*driver data*/ 
     FT2D_Capabilities         caps;        /*will show the 2D engine capability */
     FT2D_GraphicsDeviceFuncs  funcs;       /*2D engine support API*/
     FT2D_AccelFlags_T         accel_do;    /*Tell 2D engine what kind of function need to operate*/
     FT2D_SettingFlags_T       accel_param; /*Tell 2D engine what kind of function parameter need to operate*/
     FT2D_GFX_Setting_Data     accel_setting_data; /*Tell 2D engine function parameter value to set*/   
}FT2D_GfxDevice;

#define FT2D_SUCCESS                0
#define FT2D_FAIL                  -1      

/* line style for ft2d only*/
typedef enum {
     FT2D_STYLE_SOLID             = 1,
     FT2D_STYLE_DASH              ,
     FT2D_STYLE_DOT               ,
     FT2D_STYLE_DASH_DOT          ,
     FT2D_STYLE_DASH_DOT_DOT      ,
     FT2D_INV_STYLE_DASH              ,
     FT2D_INV_STYLE_DOT               ,
     FT2D_INV_STYLE_DASH_DOT          ,
     FT2D_INV_STYLE_DASH_DOT_DOT      ,
     FT2D_MAX_LINESTYLE
} FT2D_LINESTYLE_T;

/* frequently used ROP3 code for ft2d only*/
typedef enum {
     FT2D_ROP_BALCKNESS             = 1,
     FT2D_ROP_NOTSRCERASE              ,
     FT2D_ROP_NOTSRCCOPY               ,
     FT2D_ROP_SRCERASE                 ,
     FT2D_ROP_DSTINVERT                ,
     FT2D_ROP_PATINVERT              ,
     FT2D_ROP_SRCINVERT               ,
     FT2D_ROP_SRCAND          ,
     FT2D_ROP_MERGEPAINT      ,
     FT2D_ROP_MERGECOPY       ,
     FT2D_ROP_SRCCOPY         ,
     FT2D_ROP_SRCPAINT        ,
     FT2D_ROP_PATCOPY         ,
     FT2D_ROP_PATPAINT        , 
     FT2D_ROP_WHITENESS       ,
     FT2D_ROP_MAX
} FT2D_ROPMETHOD_T;
/*for think2d only*/
typedef enum {
    FT2D_T2D_BLEND_UNKNOWN_MODE      = 0x00,
    FT2D_T2D_BLEND_ZERO_MODE         = 0x01,
    FT2D_T2D_BLEND_ONE_MODE          = 0x02,
    FT2D_T2D_BLEND_SRCCOLOR_MODE     = 0x03,
    FT2D_T2D_BLEND_INVSRCCOLOR_MODE  = 0x04,
    FT2D_T2D_BLEND_SRCALPHA_MODE     = 0x05,
    FT2D_T2D_BLEND_INVSRCALPHAR_MODE = 0x06,
    FT2D_T2D_BLEND_DESTALPHA_MODE    = 0x07,
    FT2D_T2D_BLEND_INVDESTALPHA_MODE = 0x08,
    FT2D_T2D_BLEND_DESTCOLOR_MODE    = 0x09,
    FT2D_T2D_BLEND_INVDESTCOLOR_MODE = 0x0A
}FT2D_T2D_BLEND_MODE_T;

/*function definition*/
int driver_init_device( FT2D_GfxDevice           *device,
                        FT2D_GraphicsDeviceFuncs *funcs,
                        FT2D_BPP_T rgb_t );
                        
void driver_close_device(FT2D_GfxDevice   *device); 
#endif