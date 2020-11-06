// ------------------------------------------------------------------------------
// STDV
// ------------------------------------------------------------------------------
struct ffb_timing_info stdtv_timing[] = {
    {
     .otype = VOT_NTSC,
     .data.ccir656.pixclock = 27000,    //27000Khz
#ifdef D1_WIDTH_704
     .data.ccir656.out.xres = 704,
#else
     .data.ccir656.out.xres = 720,
#endif
     .data.ccir656.out.yres = 480,
     .data.ccir656.field0_switch = 4,
     .data.ccir656.field1_switch = 266,
     .data.ccir656.hblank_len = 134,    //without SAV/EAV
     .data.ccir656.vblank0_len = 22,
     .data.ccir656.vblank1_len = 23,
     .data.ccir656.vblank2_len = 0,
     },
    {
     .otype = VOT_PAL,
     .data.ccir656.pixclock = 27000,    //27000Khz
#ifdef D1_WIDTH_704
     .data.ccir656.out.xres = 704,
#else
     .data.ccir656.out.xres = 720,
#endif
     .data.ccir656.out.yres = 576,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 313,
     .data.ccir656.hblank_len = 140,    //without SAV/EAV
     .data.ccir656.vblank0_len = 22,
     .data.ccir656.vblank1_len = 25,
     .data.ccir656.vblank2_len = 2,
     },
};

struct OUTPUT_INFO stdtv_info = {
    .name = "STDTV",
    .config = CCIR656CONFIG(CCIR656_SYS_SDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(stdtv_timing),
    .timing_array = stdtv_timing,
};

// ------------------------------------------------------------------------------
// 480P / 576P for HDMI
// ------------------------------------------------------------------------------
struct ffb_timing_info stdtv_timing_p[] = {
    {
     .otype = VOT_480P,
     .data.ccir656.pixclock = 27000,    //27000Khz
#ifdef D1_WIDTH_704
     .data.ccir656.out.xres = 704,
#else
     .data.ccir656.out.xres = 720,
#endif
     .data.ccir656.out.yres = 480,
     .data.ccir656.field0_switch = 1,   //9,
     .data.ccir656.field1_switch = 526, //15,
     .data.ccir656.hblank_len = 130,    //without SAV/EAV
     .data.ccir656.vblank0_len = 45,
     .data.ccir656.vblank1_len = 0,
     .data.ccir656.vblank2_len = 0,
     },
    {
     .otype = VOT_576P,
     .data.ccir656.pixclock = 27000,    //27000Khz
#ifdef D1_WIDTH_704
     .data.ccir656.out.xres = 704,
#else
     .data.ccir656.out.xres = 720,
#endif
     .data.ccir656.out.yres = 576,
     .data.ccir656.field0_switch = 1,   //5,
     .data.ccir656.field1_switch = 626, //10,
     .data.ccir656.hblank_len = 136,    //without SAV/EAV
     .data.ccir656.vblank0_len = 49,
     .data.ccir656.vblank1_len = 0,
     .data.ccir656.vblank2_len = 0,
     },
};

struct OUTPUT_INFO stdtv_info_p = {
    .name = "HDMITV",
    .config = CCIR656CONFIG(CCIR656_SYS_HDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(stdtv_timing_p),
    .timing_array = stdtv_timing_p,
};

// ------------------------------------------------------------------------------
// 720P for HDMI. Pixel clock is 74.25Mhz
// ------------------------------------------------------------------------------
struct ffb_timing_info hdtv_1280x720p_timing[] = {
    {
     .otype = VOT_720P,
     .data.ccir656.pixclock = 74250,    //74250Khz
     .data.ccir656.out.xres = 1280,
     .data.ccir656.out.yres = 720,
     .data.ccir656.field0_switch = 1,   //12, //10,
     .data.ccir656.field1_switch = 752, //5,
     .data.ccir656.hblank_len = 362,    //without SAV/EAV
     .data.ccir656.vblank0_len = 25,
     .data.ccir656.vblank1_len = 0,
     .data.ccir656.vblank2_len = 5,
     }
};

struct OUTPUT_INFO hdtv_1280x720p_info = {
    .name = "720P",
    .config = CCIR656CONFIG(CCIR656_SYS_HDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(hdtv_1280x720p_timing),
    .timing_array = hdtv_1280x720p_timing,
};

// ------------------------------------------------------------------------------
// 1440x1080i
// ------------------------------------------------------------------------------
/*
 * 1440x1080i
 * Hor Total Time = 1904 pixel, Ver Total line = 1125, 30HZ
 * Ver Blank Time = 45 lines. (1080+45=1125)
 */
struct ffb_timing_info hdtv_1440x1080_timing[] = {
    {
     .otype = VOT_1440x1080I,
     .data.ccir656.pixclock = 60750,
     .data.ccir656.out.xres = 1440,
     .data.ccir656.out.yres = 1080,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 563,
     .data.ccir656.hblank_len = 352,    //468,
     .data.ccir656.vblank0_len = 20,
     .data.ccir656.vblank1_len = 23,
     .data.ccir656.vblank2_len = 2,
     },
};

struct OUTPUT_INFO hdtv_1440x1080_info = {
    .name = "1440x1080I",
    .config = CCIR656CONFIG(CCIR656_SYS_HDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(hdtv_1440x1080_timing),
    .timing_array = hdtv_1440x1080_timing,
};

// ------------------------------------------------------------------------------
// 1920x1080p
// ------------------------------------------------------------------------------
struct ffb_timing_info hdtv_1920x1080p_timing[] = {
    {
     .otype = VOT_1920x1080P,
     .data.ccir656.pixclock = 148500,   //74250,     //74250Khz
     .data.ccir656.out.xres = 1920,
     .data.ccir656.out.yres = 1080,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 1128,
     .data.ccir656.hblank_len = 272,    //without SAV/EAV
     .data.ccir656.vblank0_len = 41,
     .data.ccir656.vblank1_len = 0,
     .data.ccir656.vblank2_len = 4,
     },
};

struct OUTPUT_INFO hdtv_1920x1080p_info = {
    .name = "1920x1080P",
    .config = CCIR656CONFIG(CCIR656_SYS_HDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(hdtv_1920x1080p_timing),
    .timing_array = hdtv_1920x1080p_timing,
};

// ------------------------------------------------------------------------------
// 1920x1080px30
// ------------------------------------------------------------------------------
struct ffb_timing_info hdtv_1920x1080px30_timing[] = {
    {
     .otype = VOT_1920x1080P,
     .data.ccir656.pixclock = 74250,
     .data.ccir656.out.xres = 1920,
     .data.ccir656.out.yres = 1080,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 1128,
     .data.ccir656.hblank_len = 272,    //without SAV/EAV
     .data.ccir656.vblank0_len = 41,
     .data.ccir656.vblank1_len = 0,
     .data.ccir656.vblank2_len = 4,
     },
};

struct OUTPUT_INFO hdtv_1920x1080px30_info = {
    .name = "1920x1080Px30",
    .config = CCIR656CONFIG(CCIR656_SYS_HDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(hdtv_1920x1080px30_timing),
    .timing_array = hdtv_1920x1080px30_timing,
};

// ------------------------------------------------------------------------------
// 1920x1080px25
// ------------------------------------------------------------------------------
struct ffb_timing_info hdtv_1920x1080px25_timing[] = {
    {
     .otype = VOT_1920x1080P,
     .data.ccir656.pixclock = 74250,
     .data.ccir656.out.xres = 1920,
     .data.ccir656.out.yres = 1080,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 1202,
     .data.ccir656.hblank_len = 578,    //without SAV/EAV
     .data.ccir656.vblank0_len = 96,
     .data.ccir656.vblank1_len = 0,
     .data.ccir656.vblank2_len = 24,
     },
};

struct OUTPUT_INFO hdtv_1920x1080px25_info = {
    .name = "1920x1080Px25",
    .config = CCIR656CONFIG(CCIR656_SYS_HDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(hdtv_1920x1080px25_timing),
    .timing_array = hdtv_1920x1080px25_timing,
};

// ------------------------------------------------------------------------------
// 1920x1080i
// ------------------------------------------------------------------------------
struct ffb_timing_info hdtv_1920x1080i_timing[] = {
    {
        .otype  = VOT_1920x1080I,
        .data.ccir656.pixclock = 27000, //74250,     //74250Khz
        .data.ccir656.out.xres = 1920,
        .data.ccir656.out.yres = 1080,
        .data.ccir656.field0_switch = 1,
        .data.ccir656.field1_switch = 563,
        .data.ccir656.hblank_len = 272,     //without SAV/EAV
        .data.ccir656.vblank0_len = 20,
        .data.ccir656.vblank1_len = 23,
        .data.ccir656.vblank2_len = 2,
    },
};

struct OUTPUT_INFO hdtv_1920x1080i_info = {
    .name   = "1920x1080I",
    .config = CCIR656CONFIG(CCIR656_SYS_HDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(hdtv_1920x1080i_timing),
    .timing_array = hdtv_1920x1080i_timing,
};

/* ---------------------------------------------------------------------------------
 * For Cascade 1280x1024x25i
 * ---------------------------------------------------------------------------------
 */
struct ffb_timing_info sdtv_1280x1024_25_timing[] = {
    {
     .otype = VOT_1280x1024_25I,
     .data.ccir656.pixclock = 81000,
     .data.ccir656.out.xres = 1280,
     .data.ccir656.out.yres = 1024,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 534,
     .data.ccir656.hblank_len = 231,    //without SAV/EAV
     .data.ccir656.vblank0_len = 20,
     .data.ccir656.vblank1_len = 23,
     .data.ccir656.vblank2_len = 2,
     },
};

struct OUTPUT_INFO sdtv_1280x1024x25_info = {
    .name = "1280x1024Ix25",
    .config = CCIR656CONFIG(CCIR656_SYS_SDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(sdtv_1280x1024_25_timing),
    .timing_array = sdtv_1280x1024_25_timing,
};

/* ---------------------------------------------------------------------------------
 * For Cascade 1280x1024x30i
 * ---------------------------------------------------------------------------------
 */
struct ffb_timing_info sdtv_1280x1024_30_timing[] = {
    {
     .otype = VOT_1280x1024_30I,
     .data.ccir656.pixclock = 108000,   /* pixel clock is 108 MHz from vesa standard */
     .data.ccir656.out.xres = 1280,
     .data.ccir656.out.yres = 1024,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 534,
     .data.ccir656.hblank_len = 399,    //without SAV/EAV
     .data.ccir656.vblank0_len = 20,
     .data.ccir656.vblank1_len = 23,
     .data.ccir656.vblank2_len = 2,
     },
};

struct OUTPUT_INFO sdtv_1280x1024x30_info = {
    .name = "1280x1024Ix30",
    .config = CCIR656CONFIG(CCIR656_SYS_SDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(sdtv_1280x1024_30_timing),
    .timing_array = sdtv_1280x1024_30_timing,
};

/* ---------------------------------------------------------------------------------
 * for Cascade 1024x768_25i
 * ---------------------------------------------------------------------------------
 */
struct ffb_timing_info sdtv_1024x768_25_timing[] = {
    {
     .otype = VOT_1024x768_25I,
     .data.ccir656.pixclock = 65000,    //65M
     .data.ccir656.out.xres = 1024,
     .data.ccir656.out.yres = 768,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 423,
     .data.ccir656.hblank_len = 503,    //without SAV/EAV
     .data.ccir656.vblank0_len = 37,
     .data.ccir656.vblank1_len = 40,
     .data.ccir656.vblank2_len = 2,
     },
};

struct OUTPUT_INFO sdtv_1024x768x25_info = {
    .name = "1024x768Ix25",
    .config = CCIR656CONFIG(CCIR656_SYS_SDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(sdtv_1024x768_25_timing),
    .timing_array = sdtv_1024x768_25_timing,
};

/* ---------------------------------------------------------------------------------
 * for Cascade 1024x768_30i
 * ---------------------------------------------------------------------------------
 */
struct ffb_timing_info sdtv_1024x768_30_timing[] = {
    {
     .otype = VOT_1024x768_30I,
     .data.ccir656.pixclock = 64970,    //64.97M
     .data.ccir656.out.xres = 1024,
     .data.ccir656.out.yres = 768,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 406,
     .data.ccir656.hblank_len = 300,    //without SAV/EAV
     .data.ccir656.vblank0_len = 20,
     .data.ccir656.vblank1_len = 23,
     .data.ccir656.vblank2_len = 2,
     },
};

struct OUTPUT_INFO sdtv_1024x768x30_info = {
    .name = "1024x768Ix30",
    .config = CCIR656CONFIG(CCIR656_SYS_SDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(sdtv_1024x768_30_timing),
    .timing_array = sdtv_1024x768_30_timing,
};

/*
 * for 1024x768x60i BT1120
 */
struct ffb_timing_info hdtv_1024x768_30_timing[] = {
    {
     .otype = VOT_1024x768_30I, //actually 60I
     .data.ccir656.pixclock = 27000,    //27Mhz
     .data.ccir656.out.xres = 1024,
     .data.ccir656.out.yres = 768,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 406,
     .data.ccir656.hblank_len = 93,     //without SAV/EAV
     .data.ccir656.vblank0_len = 15,
     .data.ccir656.vblank1_len = 15,
     .data.ccir656.vblank2_len = 2,
     },
};

struct OUTPUT_INFO hdtv_1024x768x30_info = {
    .name = "1024x768Ix30",
    .config = CCIR656CONFIG(CCIR656_SYS_HDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(hdtv_1024x768_30_timing),
    .timing_array = hdtv_1024x768_30_timing,
};

/*
 * for BT1120 XGA timing
 */
struct ffb_timing_info hdtv_1024x768p_timing[] = {
    {
     .otype = VOT_1024x768P,
     .data.ccir656.pixclock = 65000,    //65M
     .data.ccir656.out.xres = 1024,
     .data.ccir656.out.yres = 768,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 800,
     .data.ccir656.hblank_len = 333,    //312,  //without SAV/EAV
     .data.ccir656.vblank0_len = 30,    //38,
     .data.ccir656.vblank1_len = 0,
     .data.ccir656.vblank2_len = 0,
     },
};

struct OUTPUT_INFO hdtv_1024x768p_info = {
    .name = "1024x768P",
    .config = CCIR656CONFIG(CCIR656_SYS_HDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(hdtv_1024x768p_timing),
    .timing_array = hdtv_1024x768p_timing,
};

/*
 * for BT1120 SXGA, 108Mhz
 */
struct ffb_timing_info hdtv_1280x1024p_timing[] = {
    {
     .otype = VOT_1280x1024P,
     .data.ccir656.pixclock = 108000,   //108M
     .data.ccir656.out.xres = 1280,
     .data.ccir656.out.yres = 1024,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 1068,
     .data.ccir656.hblank_len = 400,    //without SAV/EAV
     .data.ccir656.vblank0_len = 42,
     .data.ccir656.vblank1_len = 0,
     .data.ccir656.vblank2_len = 0,
     },
};

struct OUTPUT_INFO hdtv_1280x1024p_info = {
    .name = "1280x1024P",
    .config = CCIR656CONFIG(CCIR656_SYS_HDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(hdtv_1280x1024p_timing),
    .timing_array = hdtv_1280x1024p_timing,
};

/*
 * for BT1120 SXGA, 106.5Mhz
 */
struct ffb_timing_info hdtv_1440x900p_timing[] = {
    {
     .otype = VOT_1440x900P,
     .data.ccir656.pixclock = 106500,   //106.5M
     .data.ccir656.out.xres = 1440,
     .data.ccir656.out.yres = 900,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 936,
     .data.ccir656.hblank_len = 456,    //without SAV/EAV
     .data.ccir656.vblank0_len = 34,
     .data.ccir656.vblank1_len = 0,
     .data.ccir656.vblank2_len = 0,
     },
};

struct OUTPUT_INFO hdtv_1440x900p_info = {
    .name = "1440x900P",
    .config = CCIR656CONFIG(CCIR656_SYS_HDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(hdtv_1440x900p_timing),
    .timing_array = hdtv_1440x900p_timing,
};

/*
 * for BT1120 WSXGA+, 146.25Mhz
 */
struct ffb_timing_info hdtv_1680x1050p_timing[] = {
    {
     .otype = VOT_1680x1050P,
     .data.ccir656.pixclock = 146250,   //146.25M
     .data.ccir656.out.xres = 1680,
     .data.ccir656.out.yres = 1050,
     .data.ccir656.field0_switch = 10,  //1->0
     .data.ccir656.field1_switch = 4,   //0->1
     .data.ccir656.hblank_len = 552,    //without SAV/EAV
     .data.ccir656.vblank0_len = 39,
     .data.ccir656.vblank1_len = 0,
     .data.ccir656.vblank2_len = 0,
     },
};

struct OUTPUT_INFO hdtv_1680x1050p_info = {
    .name = "1680x1050P",
    .config = CCIR656CONFIG(CCIR656_SYS_HDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(hdtv_1680x1050p_timing),
    .timing_array = hdtv_1680x1050p_timing,
};

/*
 * For 4+1 PAL Cascade
 */
struct ffb_timing_info sdtv_1440x1152_25_timing[] = {
    {
     .otype = VOT_1440x1152_25I,
     .data.ccir656.pixclock = 108000,   //108M
     .data.ccir656.out.xres = 1440,
     .data.ccir656.out.yres = 1152,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 615,
     .data.ccir656.hblank_len = 310,    //without SAV/EAV
     .data.ccir656.vblank0_len = 37,
     .data.ccir656.vblank1_len = 40,
     .data.ccir656.vblank2_len = 2,
     },
};

struct OUTPUT_INFO sdtv_1440x1152x25_info = {
    .name = "1440x1125Ix25",
    .config = CCIR656CONFIG(CCIR656_SYS_SDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(sdtv_1440x1152_25_timing),
    .timing_array = sdtv_1440x1152_25_timing,
};

/*
 * For 4+1 NTSC Cascase
 */
struct ffb_timing_info sdtv_1440x960_30_timing[] = {
    {
     .otype = VOT_1440x960_30I,
     .data.ccir656.pixclock = 108000,   //108M
     .data.ccir656.out.xres = 1440,
     .data.ccir656.out.yres = 960,
     .data.ccir656.field0_switch = 1,
     .data.ccir656.field1_switch = 519,
     .data.ccir656.hblank_len = 288,    //without SAV/EAV
     .data.ccir656.vblank0_len = 37,
     .data.ccir656.vblank1_len = 40,
     .data.ccir656.vblank2_len = 2,
     },
};

struct OUTPUT_INFO sdtv_1440x960x30_info = {
    .name = "1440x960Ix30",
    .config = CCIR656CONFIG(CCIR656_SYS_SDTV, CCIR656_INFMT_PROGRESS, 1),
    .config2 = FFBCONFIG2(0),
    .timing_num = ARRAY_SIZE(sdtv_1440x960_30_timing),
    .timing_array = sdtv_1440x960_30_timing,
};
