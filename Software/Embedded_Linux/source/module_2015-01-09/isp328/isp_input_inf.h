/**
 * @file isp_input_inf.h
 * ISP input module interface header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#ifndef __ISP_INPUT_INF_H__
#define __ISP_INPUT_INF_H__

#include "isp_common.h"
#include <linux/semaphore.h>

//=============================================================================
// interface version
//=============================================================================
#define ISP_INPUT_INF_VER   ((1<<16)|(0<<8)|(0<<4)|0)  // 1.0.0.0

//=============================================================================
// struct & flag definition
//=============================================================================
#define DEV_MAX_NAME_SIZE   32

// sensor capability flag
#define SUPPORT_MIRROR      BIT0
#define SUPPORT_FLIP        BIT1
#define SUPPORT_AE          BIT2
#define SUPPORT_AWB         BIT3
#define SUPPORT_WDR         BIT4

// sensor interface
enum SENSOR_IF {
    IF_PARALLEL = 0,
    IF_MIPI,
    IF_HiSPi,
    IF_LVDS,
    IF_LVDS_PANASONIC,
};

// sensor property commands
enum SENSOR_CMD {
    ID_MIRROR = 0,
    ID_FLIP,
    ID_FPS,
    ID_AE_EN,
    ID_AWB_EN,
    ID_WDR_EN,
    ID_AESTA,
    ID_RESET,
    ID_A_GAIN,
    ID_MIN_A_GAIN,
    ID_MAX_A_GAIN,
    ID_D_GAIN,
    ID_MIN_D_GAIN,
    ID_MAX_D_GAIN,
    ID_EXP_US,
    NUM_OF_SENSOR_CMD
};

enum HISPI_PROTOCOL {
    Streaming_S = 0,
    Streaming_SP,
    Packetized_SP,
    ActiveStart_SP8,
};

//
// sensor exposure time unit is 1/10000 second
// sensor gain precision is UFIX7.6
//
#define GAIN_SHIFT      6
#define GAIN_1X         (1L << GAIN_SHIFT)

typedef struct sensor_dev {
    char name[DEV_MAX_NAME_SIZE];
    void *private;
    int capability;
    u32 xclk;               // master clock frequency
    u32 pclk;               // pixel clock frequency
    enum SENSOR_IF interface;
    int  num_of_lane;       // the number of lanes of HiSPi and MIPI, or channels of sub-LVDS
    enum HISPI_PROTOCOL protocol;
    enum SRC_FMT fmt;
    enum BAYER_TYPE bayer_type;
    enum POLARITY hs_pol;
    enum POLARITY vs_pol;
    bool inv_clk;           // pixel clock invert control
    win_rect_t out_win;     // output window
    win_rect_t img_win;     // valid image window (equal to active window of ISP)

    u32 min_exp;
    u32 max_exp;
    u32 min_gain;
    u32 max_gain;
    u32 curr_exp;
    u32 curr_gain;
    int exp_latency;        // latency of exposure work
    int gain_latency;       // latency of gain setting work
    int fps;

    // member functions
    int  (*fn_init)(void);
    int  (*fn_read_reg)(u32 addr, u32 *pval);
    int  (*fn_write_reg)(u32 addr, u32 val);
    int  (*fn_set_size)(u32 width, u32 height);
    u32  (*fn_get_exp)(void);
    int  (*fn_set_exp)(u32 exp);
    u32  (*fn_get_gain)(void);
    int  (*fn_set_gain)(u32 gain);
    int  (*fn_get_property)(enum SENSOR_CMD cmd, unsigned long arg);
    int  (*fn_set_property)(enum SENSOR_CMD cmd, unsigned long arg);
} sensor_dev_t;

// lens capability flag
#define LENS_SUPPORT_FOCUS  BIT0
#define LENS_SUPPORT_ZOOM   BIT1

// lens property commands
enum LENS_CMD {
    NUM_OF_LENS_CMD
};

typedef struct lens_dev {
    void *private;
    char name[DEV_MAX_NAME_SIZE];
    int capability;

    // member functions
    int (*fn_init)(void);
    int (*fn_focus_move_to_pi)(void);
    s32 (*fn_focus_get_pos)(void);
    s32 (*fn_focus_set_pos)(s32 pos);
    s32 (*fn_focus_move)(s32 steps);
    s32 (*fn_check_busy)(void);

    s32 (*fn_zoom_move)(s32 steps);
    s32 (*fn_zoom_move_to_pi)(void);
    s32 (*fn_zoom_get_pos)(void);
    s32 (*fn_zoom_set_pos)(s32 steps);
    s32 focus_low_bound;
    s32 focus_hi_bound;
    s32 zoom_stage_cnt;
    s32 curr_zoom_index;
    s32 curr_zoom_x10;
    struct semaphore lock;    
} lens_dev_t;

typedef struct iris_dev {
    void *private;
    char name[DEV_MAX_NAME_SIZE];

    // member functions
    int (*fn_init)(void);
    int (*fn_close_to_open)(u32 ratio, u32 scale);
} iris_dev_t;

//=============================================================================
// extern functions
//=============================================================================
extern u32 isp_get_input_inf_ver(void);

extern int isp_register_sensor(sensor_dev_t *dev);
extern int isp_unregister_sensor(sensor_dev_t *dev);
extern sensor_dev_t* isp_get_sensor_dev(void);

extern int isp_register_lens(lens_dev_t *dev);
extern int isp_unregister_lens(lens_dev_t *dev);
extern lens_dev_t* isp_get_lens_dev(void);

extern int isp_register_iris(iris_dev_t *dev);
extern int isp_unregister_iris(iris_dev_t *dev);
extern iris_dev_t* isp_get_iris_dev(void);

#endif /* __ISP_INPUT_INF_H__ */
