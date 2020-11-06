//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//-----------------------    PLL3 = 486MHZ     ----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
#define LCD_PANEL_TYPE_XBIT     LCD_PANEL_TYPE_6BIT
#else
#define LCD_PANEL_TYPE_XBIT     LCD_PANEL_TYPE_8BIT
#endif

struct ffb_timing_info PVI_2003A_timing[] = {
    {
     .otype = VOT_LCD,
     .data.lcd.pixclock = 25170,        //25170Khz
     .data.lcd.out.xres = 640,
     .data.lcd.out.yres = 480,
     .data.lcd.hsync_len = 96,
     .data.lcd.vsync_len = 2,
     .data.lcd.left_margin = 48,        //HBack Porch
     .data.lcd.right_margin = 16,       //HFront Porch
     .data.lcd.upper_margin = 33,       //VBack Porch
     .data.lcd.lower_margin = 10,       //VFront Porch
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO PVI_2003A_info = {
    .name = "PVI 2003A",
    .config = LCDCONFIG(LCD_PANEL_TYPE_6BIT, 1, 0, 0, 0, 1, 1), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(PVI_2003A_timing),
    .timing_array = PVI_2003A_timing,
};

struct ffb_timing_info FS453_timing[] = {
    {
     .otype = VOT_640x480,
     .data.lcd.pixclock = 25170,        //25170Khz
     .data.lcd.out.xres = 640,
     .data.lcd.out.yres = 480,
     .data.lcd.hsync_len = 96,
     .data.lcd.vsync_len = 2,
     .data.lcd.left_margin = 48,        //HBack Porch
     .data.lcd.right_margin = 16,       //HFront Porch
     .data.lcd.upper_margin = 33,       //VBack Porch
     .data.lcd.lower_margin = 10,       //VFront Porch
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO FS453_info = {
    .name = "FS453",
    .config = LCDCONFIG(LCD_PANEL_TYPE_8BIT, 0, 0, 0, 0, 1, 1), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(1),
    .timing_num = ARRAY_SIZE(FS453_timing),
    .timing_array = FS453_timing,
};

/*
 * Internal VGA DAC: 800x600@60HZ
 */
/* pixel clock is 40.000 MHz from vesa standard. Actually, Mhz
 * The prefered PLL3 is 648Mhz with M=54, N=1
 */
struct ffb_timing_info VGA_800x600_timing[] = {
    {
     .otype = VOT_800x600,
     .data.lcd.pixclock = 29232,
     .data.lcd.out.xres = 800,
     .data.lcd.out.yres = 480,
     .data.lcd.hsync_len = 48, //Hor Sync Time
     .data.lcd.vsync_len = 3,   //Ver Sync Time
     .data.lcd.left_margin = 40,        //HBack Porch + HLeft Border
     .data.lcd.right_margin = 40,       //HFront Porch + HRight Border
     .data.lcd.upper_margin = 29,       //VBack Porch + Top Border
     .data.lcd.lower_margin = 13,        //VFront Porch + Bottom Border
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO VGA_800x600_info = {
    .name = "VGA_800x600",
    .config = LCDCONFIG(LCD_PANEL_TYPE_8BIT, 0, 0, 0, 0, 1, 1), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(VGA_800x600_timing),
    .timing_array = VGA_800x600_timing,
};

/* pixel clock is 65.000 MHz from vesa standard. Actually, 64.8Mhz
 * The prefered PLL3 is 648Mhz with M=54, N=1, ok
 */
struct ffb_timing_info VGA_1024x768_timing[] = {
    {
     .otype = VOT_1024x768,
     .data.lcd.pixclock = 65000,        //65000Khz
     .data.lcd.out.xres = 1024,
     .data.lcd.out.yres = 768,
     .data.lcd.hsync_len = 136, //Hor Sync Time
     .data.lcd.vsync_len = 6,   //Ver Sync Time
     .data.lcd.left_margin = 160,       //HBack Porch + HLeft Border
     .data.lcd.right_margin = 24,       //HFront Porch + HRight Border
     .data.lcd.upper_margin = 29,       //VBack Porch + Top Border
     .data.lcd.lower_margin = 3,        //VFront Porch + Bottom Border
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO VGA_1024x768_info = {
    .name = "VGA_1024x768",
    .config = LCDCONFIG(LCD_PANEL_TYPE_8BIT, 0, 0, 0, 0, 1, 1), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(VGA_1024x768_timing),
    .timing_array = VGA_1024x768_timing,
};

/*
 * Internal VGA DAC: 1280x800@60HZ
 */
/* pixel clock is 83.5 MHz from vesa standard */
struct ffb_timing_info VGA_1280x800_timing[] = {
    {
     .otype = VOT_1280x800,
     .data.lcd.pixclock = 84000,        //83460Khz
     .data.lcd.out.xres = 1280,
     .data.lcd.out.yres = 800,
     .data.lcd.hsync_len = 128, //Hor Sync Time
     .data.lcd.vsync_len = 6,   //Ver Sync Time
     .data.lcd.left_margin = 200,       //HBack Porch + HLeft Border
     .data.lcd.right_margin = 72,       //HFront Porch + HRight Border
     .data.lcd.upper_margin = 22,       //VBack Porch + Top Border
     .data.lcd.lower_margin = 3,        //VFront Porch + Bottom Border
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO VGA_1280x800_info = {
    .name = "VGA_1280x800",
    .config = LCDCONFIG(LCD_PANEL_TYPE_8BIT, 0, 0, 0, 0, 1, 0), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(VGA_1280x800_timing),
    .timing_array = VGA_1280x800_timing,
};

/*
 * Internal VGA DAC: 1280x960@60HZ, ok
 */
/* pixel clock is 108.0 MHz from vesa standard */
struct ffb_timing_info VGA_1280x960_timing[] = {
    {
     .otype = VOT_1280x960,
     .data.lcd.pixclock = 108000,       //108Mhz
     .data.lcd.out.xres = 1280,
     .data.lcd.out.yres = 960,
     .data.lcd.hsync_len = 112, //Hor Sync Time
     .data.lcd.vsync_len = 3,   //Ver Sync Time
     .data.lcd.left_margin = 312,       //HBack Porch + HLeft Border
     .data.lcd.right_margin = 96,       //HFront Porch + HRight Border
     .data.lcd.upper_margin = 36,       //VBack Porch + Top Border
     .data.lcd.lower_margin = 1,        //VFront Porch + Bottom Border
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO VGA_1280x960_info = {
    .name = "VGA_1280x960",
    .config = LCDCONFIG(LCD_PANEL_TYPE_XBIT, 0, 0, 0, 0, 0, 0), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(VGA_1280x960_timing),
    .timing_array = VGA_1280x960_timing,
};

/* 1280x1024@60HZ
 */
/* pixel clock is 108 MHz from vesa standard, ok */
struct ffb_timing_info VGA_1280x1024_timing[] = {
    {
     .otype = VOT_1280x1024,
     .data.lcd.pixclock = 108000,       //108Khz
     .data.lcd.out.xres = 1280,
     .data.lcd.out.yres = 1024,
     .data.lcd.hsync_len = 112, //Hor Sync Time
     .data.lcd.vsync_len = 3,   //Ver Sync Time
     .data.lcd.left_margin = 248,       //HBack Porch + HLeft Border
     .data.lcd.right_margin = 48,       //HFront Porch + HRight Border
     .data.lcd.upper_margin = 38,       //VBack Porch + Top Border
     .data.lcd.lower_margin = 1,        //VFront Porch + Bottom Border
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO VGA_1280x1024_info = {
    .name = "VGA_1280x1024",
    .config = LCDCONFIG(LCD_PANEL_TYPE_XBIT, 0, 0, 0, 0, 0, 0), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(VGA_1280x1024_timing),
    .timing_array = VGA_1280x1024_timing,
};

/* 1360x768@60HZ
 */
/* pixel clock is 108 MHz from vesa standard, ok */
struct ffb_timing_info VGA_1360x768_timing[] = {
    {
     .otype = VOT_1360x768,
     .data.lcd.pixclock = 85500,        //85500Khz
     .data.lcd.out.xres = 1360,
     .data.lcd.out.yres = 768,
     .data.lcd.hsync_len = 112, //Hor Sync Time
     .data.lcd.vsync_len = 6,   //Ver Sync Time
     .data.lcd.left_margin = 256,       //HBack Porch + HLeft Border
     .data.lcd.right_margin = 64,       //HFront Porch + HRight Border
     .data.lcd.upper_margin = 18,       //VBack Porch + Top Border
     .data.lcd.lower_margin = 3,        //VFront Porch + Bottom Border
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO VGA_1360x768_info = {
    .name = "VGA_1360x768",
    .config = LCDCONFIG(LCD_PANEL_TYPE_XBIT, 0, 0, 0, 0, 0, 0), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(VGA_1360x768_timing),
    .timing_array = VGA_1360x768_timing,
};

/* 1440x900@60HZ
 */
/* pixel clock is 106.5 MHz from vesa standard */
struct ffb_timing_info VGA_1440x900_timing[] = {
    {
     .otype = VOT_1440x900,
     .data.lcd.pixclock = 106500,       //106500Khz
     .data.lcd.out.xres = 1440,
     .data.lcd.out.yres = 900,
     .data.lcd.hsync_len = 152, //Hor Sync Time
     .data.lcd.vsync_len = 6,   //Ver Sync Time
     .data.lcd.left_margin = 232,       //HBack Porch + HLeft Border
     .data.lcd.right_margin = 80,       //HFront Porch + HRight Border
     .data.lcd.upper_margin = 25,       //VBack Porch + Top Border
     .data.lcd.lower_margin = 3,        //VFront Porch + Bottom Border
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO VGA_1440x900_info = {
    .name = "VGA_1440x900",
    .config = LCDCONFIG(LCD_PANEL_TYPE_8BIT, 0, 0, 0, 0, 1, 0), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(VGA_1440x900_timing),
    .timing_array = VGA_1440x900_timing,
};

/* 1600x1200@60HZ
 */
/* pixel clock is 162 MHz from vesa standard, ok */
struct ffb_timing_info VGA_1600x1200_timing[] = {
    {
     .otype = VOT_1600x1200,
     .data.lcd.pixclock = 162000,       //162000Khz
     .data.lcd.out.xres = 1600,
     .data.lcd.out.yres = 1200,
     .data.lcd.hsync_len = 192, //Hor Sync Time
     .data.lcd.vsync_len = 3,   //Ver Sync Time
     .data.lcd.left_margin = 304,       //HBack Porch + HLeft Border
     .data.lcd.right_margin = 64,       //HFront Porch + HRight Border,
     .data.lcd.upper_margin = 46,       //VBack Porch + Top Border
     .data.lcd.lower_margin = 1,        //VFront Porch + Bottom Border
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO VGA_1600x1200_info = {
    .name = "VGA_1600x1200",
    .config = LCDCONFIG(LCD_PANEL_TYPE_8BIT, 0, 0, 0, 0, 0, 0), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(VGA_1600x1200_timing),
    .timing_array = VGA_1600x1200_timing,
};

/*
 * 1680 * 1050
 */
/* pixel clock is 146.25 MHz from vesa standard, ok */
struct ffb_timing_info VGA_1680x1050_timing[] = {
    {
     .otype = VOT_1680x1050,
     .data.lcd.pixclock = 145250,       //146250Khz
     .data.lcd.out.xres = 1680,
     .data.lcd.out.yres = 1050,
     .data.lcd.hsync_len = 176, //Hor Sync Time
     .data.lcd.vsync_len = 6,   //Ver Sync Time
     .data.lcd.left_margin = 280,   //HBack Porch + HLeft Border
     .data.lcd.right_margin = 104,      //HFront Porch + HRight Border,
     .data.lcd.upper_margin = 30,       //VBack Porch + Top Border
     .data.lcd.lower_margin = 3,        //VFront Porch + Bottom Border
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO VGA_1680x1050_info = {
    .name = "VGA_1680x1050",
    .config = LCDCONFIG(LCD_PANEL_TYPE_8BIT, 0, 0, 0, 0, 1, 0), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(VGA_1680x1050_timing),
    .timing_array = VGA_1680x1050_timing,
};

/*
 * 1920x1080
 */
/* pixel clock is 148.5 MHz from vesa standard, ok */
struct ffb_timing_info VGA_1920x1080_timing[] = {
    {
     .otype = VOT_1920x1080,
     .data.lcd.pixclock = 148500,       //148500Khz
     .data.lcd.out.xres = 1920,
     .data.lcd.out.yres = 1080,
     .data.lcd.hsync_len = 44,  //Hor Sync Time
     .data.lcd.vsync_len = 5,   //Ver Sync Time
     .data.lcd.left_margin = 148,       //HBack Porch + HLeft Border
     .data.lcd.right_margin = 88,       //HFront Porch + HRight Border,
     .data.lcd.upper_margin = 36,       //VBack Porch + Top Border
     .data.lcd.lower_margin = 4,        //VFront Porch + Bottom Border
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO VGA_1920x1080_info = {
    .name = "VGA_1920x1080",
    .config = LCDCONFIG(LCD_PANEL_TYPE_XBIT, 0, 0, 0, 0, 0, 0), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(VGA_1920x1080_timing),
    .timing_array = VGA_1920x1080_timing,
};

/*
 * 1280x720
 */
/* pixel clock is 74.25 MHz from vesa standard, ok */
struct ffb_timing_info VGA_1280x720_timing[] = {
    {
     .otype = VOT_1280x720,
     .data.lcd.pixclock = 74250,       //74250Khz
     .data.lcd.out.xres = 1280,
     .data.lcd.out.yres = 720,
     .data.lcd.hsync_len = 40,          //Hor Sync Time
     .data.lcd.vsync_len = 5,           //Ver Sync Time
     .data.lcd.left_margin = 220,       //HBack Porch + HLeft Border
     .data.lcd.right_margin = 110,      //HFront Porch + HRight Border,
     .data.lcd.upper_margin = 20,       //VBack Porch + Top Border
     .data.lcd.lower_margin = 5,        //VFront Porch + Bottom Border
     .data.lcd.sp_param = 0,
     },
};

struct OUTPUT_INFO VGA_1280x720_info = {
    .name = "VGA_1280x720",
    .config = LCDCONFIG(LCD_PANEL_TYPE_XBIT, 0, 0, 0, 0, 0, 0), //(PanelType,RGBSW,IPWR,IDE,ICK,IHS,IVS)
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(VGA_1280x720_timing),
    .timing_array = VGA_1280x720_timing,
};
