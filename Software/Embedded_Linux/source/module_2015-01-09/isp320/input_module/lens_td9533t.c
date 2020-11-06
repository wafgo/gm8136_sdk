/**
 * @file lens_td9533t.c
 *  driver of td9533t lens.
 * Copyright (C) 2010 GM Corp. (http://www.grain-media.com)*
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stddef.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include "spi_common.h"
#include "lens_td9533t.h"
//#include "gpcn_drv.h"

#define LENS_NAME       "TD9533T"
#define MIN(x,y) ((x)<(y) ? (x) : (y))
#define MAX(x,y) ((x)>(y) ? (x) : (y))
#define ABS(x) ((x)<0 ? (-1 * (x)):(x))

#define AF_LENS2
#ifdef AF_LENS2
#define NEAR_BOUND              0
#define INF_BOUND               1900
#define TELE_BOUND              (750)
#define WIDE_BOUND              0
#else
#define NEAR_BOUND              0
#define INF_BOUND               3300
#define TELE_BOUND              (1410)
#define WIDE_BOUND              0
#endif
#define PI_FIND_TIMEOUT         300   //ticks
#define WAIT_TIMEOUT_MS         3000   //msec

#define PI_STATE_INIT           0
#define PI_STATE_ROUGH          1
#define PI_STATE_FINE           2
#define PI_STATE_DONE           3
#define PI_DIR_INF              0
#define PI_DIR_NEAR             1

#define AF_FOCUS_SPEED          32
#define AF_ZOOM_SPEED           32
#define PULSE_PER_MICROSTEPS    64
#define AF_BACKLASH_CORR        9         //Crrection for backlash deviation
#define ZOOM_BACKLASH_CORR      22     //Crrection for backlash deviation

static int zoom_step_index = 0;
static int lens_focus_pos = 0;
static int lens_zoom_pos = 0;
static bool bZoomPI_Found = false;
static bool bFocusPI_Found = false;

#define FOCUS_DIR_INIT      0
#define FOCUS_DIR_INF       1
#define FOCUS_DIR_NEAR      (-1)
#define FOCUS_POS_STEPS     300
#define FOCUS_POS_MAX       1180
#define FOCUS_POS_MIN       1130

#define FOCUS_POS_DEF       1000

static int current_fdir = FOCUS_DIR_INIT;

#define ZOOM_DIR_INIT      0
#define ZOOM_DIR_INF       1
#define ZOOM_DIR_NEAR      (-1)

static int current_zdir = ZOOM_DIR_INIT;


#define DELAY_WAIT
//#define SLEEP_WAIT
#define CHECK_BUSY_WAIT

enum zoom_step
{
    ZOOM_X_10 = 0,
    ZOOM_X_12,
    ZOOM_X_14,
    ZOOM_X_16,
    ZOOM_X_17,
    ZOOM_X_19,
    ZOOM_X_21,
    ZOOM_X_23,
    ZOOM_X_25,
    ZOOM_X_27
};

LENS_Z_F_TAB zf_tab[] =
{
#ifdef AF_LENS2
#if 0
    // Most accuracy
    {WIDE_BOUND, 1120, 1220},   //1150
    {40, 1260, 1360},     //1290
    {80, 1380, 1480},     //1410
    {120, 1480, 1580},     //1510
    {160, 1560, 1660},     //1590
    {200, 1640, 1770},     //1650
    {240, 1650, 1740},     //1700
    {280, 1680, 1790},     //1750
    {300, 1760, 1830},     //1790
    {320, 1800, 1870},     //1830
    {TELE_BOUND,   0, INF_BOUND}        //for PI
#else
    // little tolerance
    {WIDE_BOUND, 1000, 1350},   //1150
    {40, 1150, 1600},     //1290
    {70, 1200, 1650},     //1410
    {100, 1350, 1700},     //1410
    {130, 1400, 1700},     //1510
    {160, 1450, 1760},     //1590
    {180, 1500, 1880},     //1650
    {210, 1540, 1880},     //1650
    {240, 1550, 1880},     //1700
    {270, 1580, 1880},     //1750
    {TELE_BOUND,   0, INF_BOUND}        //for PI

/*
    {WIDE_BOUND, 1000, 1300},   //1150
    {40, 1100, 1450},     //1290
    {70, 1200, 1550},     //1410
    {100, 1350, 1600},     //1410
    {130, 1400, 1600},     //1510
    {160, 1500, 1760},     //1590
    {180, 1540, 1880},     //1650
    {210, 1540, 1880},     //1650
    {240, 1550, 1880},     //1700
    {270, 1580, 1880},     //1750
    {TELE_BOUND,   0, INF_BOUND}        //for PI
*/    
/*
    {WIDE_BOUND, 1050, 1320},   //1150
    {40, 1160, 1460},     //1290
    {80, 1280, 1580},     //1410
    {120, 1380, 1680},     //1510
    {160, 1460, 1760},     //1590
    {200, 1570, 1870},     //1650
    {240, 1570, 1870},     //1700
    {280, 1620, 1870},     //1750
    {300, 1660, 1870},     //1790
    {320, 1700, 1870},     //1830
    {TELE_BOUND,   0, INF_BOUND}        //for PI
*/
#endif
#else
    {WIDE_BOUND, 2000, 2400},   //1300, 1550
    {82*1, 2000, 2400},   //1450, 1700
    {82*2, 2000, 2400},   //1550, 1780
    {82*3, 2000, 2400},
    {82*4, 2300, 2800},
    {82*5, 2300, 2800},
    {82*6, 2600, 3000},
    {82*7, 2600, 3000},
    {82*8, 2800, 3300},
    {82*9, 2800, 3300},
    {TELE_BOUND,   0, INF_BOUND}        //for PI
#endif
};

#define MIN_ZOOM_STEP   ZOOM_X_10
#define MAX_ZOOM_STEP   ZOOM_X_27

static lens_dev_t *g_plens = NULL;

static int isp_spi_pmu_fd = 0;

static pmuReg_t isp_spi_pmu_reg_8139[] = {
    /*
     * Multi-Function Port Setting Register 1 [offset=0x54]
     * ----------------------------------------------------------------
     *  [13:12] 0:GPIO_0[13],   1:CAP0_D4,  2:Bayer_D0  3:x
     *  [11:10] 0:GPIO_0[12],   1:CAP0_D5,  2:Bayer_D1  3:x
     *  [9:8]    0:GPIO_0[11],   1:CAP0_D6,  2:Bayer_D2   3:x
     *  [7:6]    0:GPIO_0[10],   1:CAP0_D7,  2:Bayer_D3   3:x
     *  [5:4]    0:GPIO_0[09],   1:CAP0_CLK, 2:x              3:x
     */
    {
     .reg_off   = 0x54,
     .bits_mask = BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8 | BIT7 | BIT6 | BIT5 | BIT4,
     .lock_bits = BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8 | BIT7 | BIT6 | BIT5 | BIT4,
     .init_val  = 0x00000000,
     .init_mask = BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8 | BIT7 | BIT6 | BIT5 | BIT4,
    },
    /*
     * Multi-Function Port Setting Register 2 [offset=0x58]
     * ----------------------------------------------------------------
     *  [17:16]    0:GPIO_0[31],   1:PWM3,  2:x               3:x
     */
    {
     .reg_off   = 0x58,
     .bits_mask = BIT17 | BIT16,
     .lock_bits = BIT17 | BIT16,
     .init_val  = 0x00000000,
     .init_mask = BIT17 | BIT16,
    },
    /*
     * Multi-Function Port Setting Register 5 [offset=0x64]
     * ----------------------------------------------------------------
     *  [17:16]    0:GPIO_0[30],   1:PWM2,  2:x               3:x
     *  [15:14]    0:GPIO_1[29],   1:PWM1, 2:DMIC_CLK    3:x
     *  [11:8]     0:GPIO_1[27],   1:SSP1_SCLK, 
     *  [7:6]       0:GPIO_1[26],   1:SSP1_RXD, 
     *  [5:4]       0:GPIO_1[25],   1:SSP1_TXD, 
     *  [3:2]       0:GPIO_1[24],   1:SSP1_FS
     */
    {
     .reg_off   = 0x64,
     .bits_mask = BIT17 | BIT16 | BIT15 | BIT14 | BIT11 | BIT10 | BIT9 | BIT8 | BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2,
     .lock_bits = BIT17 | BIT16 | BIT15 | BIT14 | BIT11 | BIT10 | BIT9 | BIT8 | BIT7 | BIT6,
     .init_val  = 0x00000000,
     .init_mask = BIT17 | BIT16 | BIT15 | BIT14 | BIT11 | BIT10 | BIT9 | BIT8 | BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2,
    },
};

static pmuRegInfo_t isp_spi_pmu_reg_info_8139 = {
    "ISP_SPI_8139",
    ARRAY_SIZE(isp_spi_pmu_reg_8139),
    ATTR_TYPE_AHB,
    &isp_spi_pmu_reg_8139[0]
};

static int spiInit(void)
{
    //u32 spiPiRegValue = 0;
    struct gpio_interrupt_mode mode;
    int fd, ret=0;
        
    isp_info("spiInit\n");

    // request GPIO
    
    ret = gpio_request(SPI_RENESAS_CS0_PIN, "SPI_CS0");
    if (ret < 0)
        return ret;
    ret = gpio_request(SPI_RENESAS_CLK_PIN, "SPI_CLK");
    if (ret < 0)
        return ret;
    ret = gpio_request(SPI_RENESAS_DAT_PIN, "SPI_DAT");
    if (ret < 0)
        return ret;
    ret = gpio_request(SPI_RENESAS_RESET_PIN, "RESET");
    if (ret < 0)
        return ret;
    ret = gpio_request(MOT_ZEXT_PIN, "ZOOM_EXT");
    if (ret < 0)
        return ret;
    ret = gpio_request(MOT_ZMOB_PIN, "ZOOM_MOB");
    if (ret < 0)
        return ret;
    ret = gpio_request(MOT_FEXT_PIN, "FOCUS_EXT");
    if (ret < 0)
        return ret;
    ret = gpio_request(MOT_FMOB_PIN, "FOCUS_MOB");
    if (ret < 0)
        return ret;

    //Init SPI GPIO
    gpio_direction_output(SPI_RENESAS_CS0_PIN, 1);	//TVT
    gpio_direction_output(SPI_RENESAS_CLK_PIN, 1);
    gpio_direction_output(SPI_RENESAS_DAT_PIN, 0);

    //Reset Motor IC
    gpio_direction_output(SPI_RENESAS_RESET_PIN, 1);	//TVT
    gpio_direction_output(SPI_RENESAS_RESET_PIN, 0);	//TVT
    gpio_direction_output(SPI_RENESAS_RESET_PIN, 1);	//TVT
    
        //Init Zoom GPIO
    gpio_direction_input(MOT_ZEXT_PIN);
    gpio_direction_input(MOT_ZMOB_PIN);

    mode.trigger_method = GPIO_INT_TRIGGER_EDGE;
    mode.trigger_edge_nr = GPIO_INT_SINGLE_EDGE;
    mode.trigger_rise_neg = GPIO_INT_FALLING;
    gpio_interrupt_setup(MOT_ZEXT_PIN, &mode);

    mode.trigger_method = GPIO_INT_TRIGGER_EDGE;
    mode.trigger_edge_nr = GPIO_INT_SINGLE_EDGE;
    mode.trigger_rise_neg = GPIO_INT_RISE;
    gpio_interrupt_setup(MOT_ZMOB_PIN, &mode);

    gpio_interrupt_enable(MOT_ZEXT_PIN);
    gpio_interrupt_enable(MOT_ZMOB_PIN);


    //Init AF GPIO
    gpio_direction_input(MOT_FEXT_PIN);
    gpio_direction_input(MOT_FMOB_PIN);

    mode.trigger_method = GPIO_INT_TRIGGER_EDGE;
    mode.trigger_edge_nr = GPIO_INT_SINGLE_EDGE;
    mode.trigger_rise_neg = GPIO_INT_FALLING;
    gpio_interrupt_setup(MOT_FEXT_PIN, &mode);

    mode.trigger_method = GPIO_INT_TRIGGER_EDGE;
    mode.trigger_edge_nr = GPIO_INT_SINGLE_EDGE;
    mode.trigger_rise_neg = GPIO_INT_RISE;
    gpio_interrupt_setup(MOT_FMOB_PIN, &mode);


    gpio_interrupt_enable(MOT_FEXT_PIN);
    gpio_interrupt_enable(MOT_FMOB_PIN);

    //Jtest
    //spiPiRegValue = (spiPiRegValue & (~ MOTOR_PI_MASK(MOTOR_REG_FOCUS_PI))) | MOTOR_PI_EN(MOTOR_REG_FOCUS_PI);
    //spiIfFuncRenesas(MOTOR_REG_PI,spiPiRegValue);

    // set pinmux
    if ((fd = ftpmu010_register_reg(&isp_spi_pmu_reg_info_8139)) < 0) {
        isp_err("register pmu failed!!\n");
        ret = -1;
    }
    isp_spi_pmu_fd = fd;

    
//    isp_info("spiInit done\n");
    return ret;

}

static int spiExit(void)
{
    // release GPIO
    gpio_free(SPI_RENESAS_CS0_PIN);
    gpio_free(SPI_RENESAS_CLK_PIN);
    gpio_free(SPI_RENESAS_DAT_PIN);
    gpio_free(SPI_RENESAS_RESET_PIN);
    gpio_free(MOT_ZEXT_PIN);
    gpio_free(MOT_ZMOB_PIN);
    gpio_free(MOT_FEXT_PIN);
    gpio_free(MOT_FMOB_PIN);

    if (isp_spi_pmu_fd) {
        ftpmu010_deregister_reg(isp_spi_pmu_fd);
        isp_spi_pmu_fd = 0;
    }

    return 0;
}

static int current_finf_bound(void)
{
    return zf_tab[zoom_step_index].focus_max;
}

static int current_fnear_bound(void)
{
    return zf_tab[zoom_step_index].focus_min;
}

static void spiIfFuncRenesas(u32 Address, u32 Data)
{
    u32 i;
    u32 BitValue;

    u32 Buffer;
    u32 AddressMask, DataMask;
    u32 TotalBitNumberOfSend;
    u32 BitNumberOfData=10;
    u32 BitNumberOfAddress=6;

//    isp_info("spi addr=%x, data=%x\n", Address, Data);
    TotalBitNumberOfSend = BitNumberOfData + BitNumberOfAddress;

    AddressMask = 0;
    for(i=0;i<BitNumberOfAddress;i++)
        AddressMask |= (0x00000001 << i);

    Buffer = Address & AddressMask;
    Buffer = Buffer << BitNumberOfData;

    DataMask = 0;
    for(i=0;i<BitNumberOfData;i++)
        DataMask |= (0x00000001 << i);

    Buffer = Buffer | (Data & DataMask);

//    isp_info("Addrss:0x%02x  ,Data:0x%03x\n", Buffer>>BitNumberOfData, Buffer&0x3ff);

    gpio_direction_output(SPI_RENESAS_CS0_PIN, 0);  //TVT
    gpio_direction_output(SPI_RENESAS_CLK_PIN, 1);

    for(i=0;i<TotalBitNumberOfSend;i++)
    {
        BitValue = (Buffer & (0x00000001 << (TotalBitNumberOfSend - 1 - i))) >> (TotalBitNumberOfSend - 1 - i);

        if(BitValue)
            gpio_direction_output(SPI_RENESAS_DAT_PIN, 1);
        else
            gpio_direction_output(SPI_RENESAS_DAT_PIN, 0);

        gpio_direction_output(SPI_RENESAS_CLK_PIN, 0);
        gpio_direction_output(SPI_RENESAS_CLK_PIN, 1);
    }

    gpio_direction_output(SPI_RENESAS_CS0_PIN, 1);  //TVT
    gpio_direction_output(SPI_RENESAS_CLK_PIN, 1);
}
//-------------------------------------------------------------------------
static s32 check_busy_all(void)
{
    return (gpio_get_value(MOT_ZEXT_PIN) | gpio_get_value(MOT_FEXT_PIN));
}
#ifndef CHECK_BUSY_WAIT
static int wait_move_done(int pin)
{
    int cnt;
    
    cnt = 0;
    while(!gpio_get_value(pin) && cnt++ < WAIT_TIMEOUT_MS){
#ifdef DELAY_WAIT
        mdelay(1);
#else
        msleep(1);
#endif
    }
    if(cnt >= WAIT_TIMEOUT_MS){        
        isp_err("wait_move_done wait high timeout pin(%d)\n", pin);
        return -1;
    }
    cnt = 0;
    while(gpio_get_value(pin) && cnt++ < WAIT_TIMEOUT_MS){
#ifdef DELAY_WAIT
        mdelay(1);
#else
        msleep(1);
#endif
    }
    if(cnt >= WAIT_TIMEOUT_MS){        
        isp_err("wait_move_done wait low timeout pin(%d)\n", pin);
        return -1;
    }

}
#endif
static int wait_move_start(int pin, int timeout_ms)
{
    int cnt;
    
    cnt = 0;
    while(!gpio_get_value(pin)){
#ifdef DELAY_WAIT
        mdelay(1);
#else
        msleep(1);
#endif
        if(++cnt >= timeout_ms){        
            isp_err("wait_move_start timeout pin(%d)\n", pin);
            return -1;
        }
    }
    return 0;

}
static int wait_move_end(int pin, int timeout_ms)
{
    int cnt;
    cnt = 0;
    while(check_busy_all()){
        if(cnt++ >= timeout_ms){
            isp_err("wait_move_end timeout \n");
            return -1;
        }
#ifdef DELAY_WAIT
        mdelay(1);
#else
        msleep(1);
#endif            
    }

    return 0;
}

static s32 inf_focus(int steps, u32 FocusSpeed, int check_bound)
{
    int inf_bound = current_finf_bound();
    int step_corr = 0;
    
    wait_move_end(MOT_FEXT_PIN, 2000);

//    isp_info("inf_focus %d\n ",steps);
    if(check_bound && (lens_focus_pos + steps) > inf_bound){
        steps = inf_bound - lens_focus_pos;
    }
    if(current_fdir != FOCUS_DIR_INF){
        step_corr = AF_BACKLASH_CORR;
        //step_corr = 0;
        //isp_info("add backlash %d\n", AF_BACKLASH_CORR);
    }
    current_fdir = FOCUS_DIR_INF;

    if(steps+step_corr <= 0)
        return 0;

    //Send Focus out

    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_EN, MOTOR_OPER_DIS);
    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_CC, MOTOR_DIR(MOTOR_FOCUS_INFI_DIRECTION) | MOTOR_MODE_MICRO_256 | MOTOR_POS_INIT(MOTOR_FOCUS_OPEN_INIT_POSITON) | MOTOR_POS_DIS);

    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_NOP_L, MOTOR_NOP_L((steps+step_corr) * PULSE_PER_MICROSTEPS));
    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_NOP_H, MOTOR_NOP_H((steps+step_corr) * PULSE_PER_MICROSTEPS));

    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_PR0, FocusSpeed);

    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_PRR_SP, MOTOR_MOVE_PRR | MOTOR_SP_12_PHASE);
    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_VC, MOTOR_VC_X1(504));    //TVT

    spiIfFuncRenesas(MOTOR_REG_PS, MOTOR_PS_EN | MOTOR_PS12_POW_ON| MOTOR_PS34_POW_OFF);    //TVT

    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_EN, MOTOR_OPER_EN | MOTOR_EN_DIR(MOTOR_FOCUS_INFI_DIRECTION));

#ifdef CHECK_BUSY_WAIT
    wait_move_start(MOT_FEXT_PIN, 1000);
#else
    wait_move_done(MOT_FEXT_PIN);
#endif
    lens_focus_pos += steps;

    //isp_info("Position:%d.\n", lens_focus_pos);

    return steps;

}

//-------------------------------------------------------------------------

static s32 near_focus(int steps, u32 FocusSpeed, int check_bound)
{
    int step_corr = 0;
    int near_bound = current_fnear_bound();
    
    wait_move_end(MOT_FEXT_PIN, 2000);

//    isp_info("near_focus %d\n ",steps);
    if(check_bound && (lens_focus_pos - steps) < near_bound){
        steps = lens_focus_pos - near_bound;
    }
    if(current_fdir != FOCUS_DIR_NEAR){
        step_corr = AF_BACKLASH_CORR;
        //isp_info("add backlash %d\n", AF_BACKLASH_CORR);
    }
    current_fdir = FOCUS_DIR_NEAR;

    if(steps+step_corr <= 0)
        return 0;

    //Send Focus In

    //Disable operation
    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_EN, MOTOR_OPER_DIS);
    //Direction,drive mode, acceleration/decleration, initial position
    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_CC, MOTOR_DIR(MOTOR_FOCUS_NEAR_DIRECTION) | MOTOR_MODE_MICRO_256 | MOTOR_POS_INIT(MOTOR_FOCUS_OPEN_INIT_POSITON) | MOTOR_POS_DIS);
    //number of pulse
    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_NOP_L, MOTOR_NOP_L((steps+step_corr)*PULSE_PER_MICROSTEPS));
    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_NOP_H, MOTOR_NOP_H((steps+step_corr)*PULSE_PER_MICROSTEPS));
    //pulse rate 1~1023: 0.2us ~ 204.8us/pulse
    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_PR0, FocusSpeed);
    //pulse rate range, number of step in microstep mode
    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_PRR_SP, MOTOR_MOVE_PRR | MOTOR_SP_12_PHASE);
    //voltage , gain
    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_VC, MOTOR_VC_X1(504));    //TVT
    //power on channel12, channel34
    spiIfFuncRenesas(MOTOR_REG_PS, MOTOR_PS_EN | MOTOR_PS12_POW_ON| MOTOR_PS34_POW_OFF);    //TVT

//    gpcn_start(MOT_FMOB_PIN, (steps+step_corr)* PULSE_PER_MICROSTEPS / 32);
    //enable operation
    spiIfFuncRenesas(MOTOR_REG_FOCUS_ID | MOTOR_REG_EN, MOTOR_OPER_EN | MOTOR_EN_DIR(MOTOR_FOCUS_NEAR_DIRECTION) | MOTOR_VD_STOP_EN);

#ifdef CHECK_BUSY_WAIT
    wait_move_start(MOT_FEXT_PIN, 1000);
#else
    wait_move_done(MOT_FEXT_PIN);
#endif
    lens_focus_pos -= steps;

    //    isp_info("Position:%d.\n", lens_focus_pos);

    return steps;

}

////////////////////////////////////////////////////////////////
//  ZOOM Control
////////////////////////////////////////////////////////////////
static s32 inf_zoom_wait(int steps, u32 ZoomSpeed, int check_bound)
{
    int step_corr = 0;
    
    wait_move_end(MOT_ZEXT_PIN, 2000);

//    isp_info("inf_zoom_wait %d\n ",steps);
    if (((lens_zoom_pos - steps) < WIDE_BOUND) && check_bound)
    {
        isp_info("Current Zoom out(Forward) over %d: %d\n ", WIDE_BOUND, lens_zoom_pos);
        steps = lens_zoom_pos - WIDE_BOUND;
    }

    if(current_zdir != ZOOM_DIR_INF){
        step_corr = ZOOM_BACKLASH_CORR;
        //isp_info("add backlash %d\n", AF_BACKLASH_CORR);
    }
    current_zdir = ZOOM_DIR_INF;

    if(steps <=0)
        return 0;


    //printf("Send Zoom out(Forward)(Speed=%2.3f ms)\n ", (float)((1.6 *ZoomSpeed * PULSE_PER_MICROSTEPS * 5 / 4) / 1000));

    //Send Zoom out
    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_EN, MOTOR_OPER_DIS);

    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_CC, MOTOR_DIR(MOTOR_ZOOM_MOVE_OUT_DIRECTION) | MOTOR_MODE_MICRO_256 | MOTOR_POS_INIT(MOTOR_ZOOM_OPEN_INIT_POSITON) | MOTOR_POS_DIS);

    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_NOP_L, MOTOR_NOP_L((steps+step_corr) *PULSE_PER_MICROSTEPS));
    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_NOP_H, MOTOR_NOP_H((steps+step_corr) *PULSE_PER_MICROSTEPS));

    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_PR0, ZoomSpeed);
    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_PRR_SP, MOTOR_MOVE_PRR | MOTOR_SP_12_PHASE);

    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_VC, MOTOR_VC_X1(504));    //TVT
    spiIfFuncRenesas(MOTOR_REG_PS, MOTOR_PS_EN | MOTOR_PS12_POW_OFF| MOTOR_PS34_POW_ON);    //TVT

    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_EN, MOTOR_OPER_EN  | MOTOR_EN_DIR(MOTOR_ZOOM_MOVE_OUT_DIRECTION) | MOTOR_VD_STOP_EN);

#ifdef CHECK_BUSY_WAIT
    wait_move_start(MOT_ZEXT_PIN, 1000);
#else
    wait_move_done(MOT_ZEXT_PIN);
#endif
    lens_zoom_pos -= steps;

//    isp_info("Position:%d.\n", lens_zoom_pos);

    return steps;

}
//-------------------------------------------------------------------------

static s32 near_zoom_wait(int steps, u32 ZoomSpeed, int check_bound)
{
    int step_corr = 0;
  
    wait_move_end(MOT_ZEXT_PIN, 2000);

//    isp_info("near_zoom_wait %d\n ",steps);
    if (((lens_zoom_pos + steps) > TELE_BOUND) && check_bound)
    {
        isp_info("Current Zoom in(Reverse) over %d: %d\n ",  TELE_BOUND, lens_zoom_pos);
        steps = TELE_BOUND - lens_zoom_pos;
    }

    if(current_zdir != ZOOM_DIR_NEAR){
        step_corr = ZOOM_BACKLASH_CORR;
        //isp_info("add backlash %d\n", AF_BACKLASH_CORR);
    }
    current_zdir = ZOOM_DIR_NEAR;

    if(steps <= 0)
        return 0;

    //printf("Send Zoom in(Reverse)(Speed=%2.3f ms)\n ", (float)((1.6 *ZoomSpeed * PULSE_PER_MICROSTEPS * 5 / 4) / 1000));

    //Send ZOOM In
    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_EN, MOTOR_OPER_DIS);

    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_CC, MOTOR_DIR(MOTOR_ZOOM_MOVE_IN_DIRECTION) | MOTOR_MODE_MICRO_256 | MOTOR_POS_INIT(MOTOR_ZOOM_OPEN_INIT_POSITON) | MOTOR_POS_DIS);

    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_NOP_L, MOTOR_NOP_L((steps+step_corr) *PULSE_PER_MICROSTEPS));
    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_NOP_H, MOTOR_NOP_H((steps+step_corr) *PULSE_PER_MICROSTEPS));

    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_PR0, ZoomSpeed);
    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_PRR_SP, MOTOR_MOVE_PRR | MOTOR_SP_12_PHASE);

    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_VC, MOTOR_VC_X1(504));    //TVT
    spiIfFuncRenesas(MOTOR_REG_PS, MOTOR_PS_EN | MOTOR_PS12_POW_OFF| MOTOR_PS34_POW_ON);    //TVT

    spiIfFuncRenesas(MOTOR_REG_ZOOM_ID | MOTOR_REG_EN, MOTOR_OPER_EN  | MOTOR_EN_DIR(MOTOR_ZOOM_MOVE_IN_DIRECTION) | MOTOR_VD_STOP_EN);

#ifdef CHECK_BUSY_WAIT
        wait_move_start(MOT_ZEXT_PIN, 1000);
#else
        wait_move_done(MOT_ZEXT_PIN);
#endif

    lens_zoom_pos += steps;
    
//    isp_info("Position:%d.\n", lens_zoom_pos);
    return lens_zoom_pos;

}

static int Focus_In(int steps)
{
//    isp_info("Focus_In\n");
    return near_focus(steps, AF_FOCUS_SPEED, 1);
}

static int Focus_Out(int steps)
{
//    isp_info("Focus_Out\n");
    return inf_focus(steps, AF_FOCUS_SPEED, 1);

}

static s32 Focus_Move(s32 steps)
{
    s32 ret = 0;
    lens_dev_t *pdev = g_plens;

    down(&pdev->lock);  
//    isp_info("Focus_Move %d\n", steps);
    if (steps >= 0) {
        ret = Focus_In(steps);
    } else {
        steps *= -1;
        ret = Focus_Out(steps);
    }
    up(&pdev->lock);
    return ret;
}

static int Zoom_In(int stages)
{
    int target_zoom_index = CLAMP((zoom_step_index + stages), MIN_ZOOM_STEP, MAX_ZOOM_STEP);
    int zoom_step = zf_tab[target_zoom_index].zoom_pos - zf_tab[zoom_step_index].zoom_pos;

//    isp_info("Zoom_In %d\n", steps);
    if(zoom_step_index == MAX_ZOOM_STEP){
        isp_info("already at max zoom in position\n");
        return 0;
    }
    //isp_info("zoomin step %d\n", zoom_step);
    near_zoom_wait(zoom_step, AF_ZOOM_SPEED, 0);
#ifdef CHECK_BUSY_WAIT
    wait_move_end(MOT_ZEXT_PIN, 2000);
#endif
    zoom_step_index = target_zoom_index;
    return zoom_step;
}

static int Zoom_Out(int stages)
{
    int target_zoom_index = CLAMP((zoom_step_index - stages), MIN_ZOOM_STEP, MAX_ZOOM_STEP);
    int zoom_step = zf_tab[zoom_step_index].zoom_pos - zf_tab[target_zoom_index].zoom_pos;

//    isp_info("Zoom_Out %d\n", steps);
    if(zoom_step_index == 0){
        isp_info("already at max zoom out position\n");
        return 0;
    }

    //isp_info("zoomout step %d\n", zoom_step);
    inf_zoom_wait(zoom_step, AF_ZOOM_SPEED, 0);
    wait_move_end(MOT_ZEXT_PIN, 2000);
    zoom_step_index = target_zoom_index;

    return zoom_step;
}

static void update_zoom_x(void)
{
    int index;
    
    index = ZOOM_X_10;
    while(index <= MAX_ZOOM_STEP){
        if (lens_zoom_pos <= zf_tab[index].zoom_pos){
            zoom_step_index = index;
//            isp_info("update_zoom_x %d\n", zoom_step_index);
            break;
        }
        else
            index++;
    }
}

static s32 focus_move_to_pos(s32 pos);
static void update_focus_pos(void)
{
    int index, pos;
    
    if (lens_focus_pos < zf_tab[zoom_step_index].focus_min || lens_focus_pos > zf_tab[zoom_step_index].focus_max){
        pos = (zf_tab[zoom_step_index].focus_max + zf_tab[zoom_step_index].focus_min) / 2;
        index = zoom_step_index;
        zoom_step_index = MAX_ZOOM_STEP+1;  // set to full focus range
        isp_info("update_focus_pos %d\n", pos);

        if(pos != lens_focus_pos){
            if(pos > lens_focus_pos)
                inf_focus(pos-lens_focus_pos, AF_FOCUS_SPEED, 1);
            else
                near_focus(lens_focus_pos - pos, AF_FOCUS_SPEED, 1);
       
            wait_move_end(MOT_FEXT_PIN, 3000);
            isp_info("update_focus_pos %d steps end\n", pos-lens_focus_pos);
        }
        zoom_step_index = index;    //update to new index
    }
}

static s32 Zoom_Move(s32 stage)
{
    s32 ret = 0;
    lens_dev_t *pdev = g_plens;

//    isp_info("Zoom_Move\n");
    down(&pdev->lock);
    if (stage >= 0) {
        ret = Zoom_In(stage);
    } else {
        stage *= -1;
        ret = Zoom_Out(stage);
    }

    pdev->focus_low_bound = zf_tab[zoom_step_index].focus_min;
    pdev->focus_hi_bound = zf_tab[zoom_step_index].focus_max;
#ifdef DELAY_WAIT
        mdelay(1);
#else
        msleep(1);
#endif

    update_focus_pos();

    //printk("current index %d pos = %d\n", zoom_step_index, lens_zoom_pos);
    up(&pdev->lock);
    
    return ret;
}

static s32 focus_move_to_pos(s32 pos)
{
    int ret = 0;    
    lens_dev_t *pdev = g_plens;
//    isp_info("focus_move_to_pos %d\n", pos);
    if(!bFocusPI_Found)
        return 0;
    down(&pdev->lock);    

    if(pos > lens_focus_pos){
//        isp_info("move to %d, step = %d\n", pos, pos - lens_focus_pos);
        ret = inf_focus(pos-lens_focus_pos, AF_FOCUS_SPEED, 1);
    }
    else{
//        isp_info("move to %d, step = %d\n", pos, lens_focus_pos - pos);
        ret = near_focus(lens_focus_pos - pos, AF_FOCUS_SPEED, 1);
    }
    up(&pdev->lock);
    return ret;
}

static s32 zoom_move_to_pos(s32 pos)
{
    int ret;
    lens_dev_t *pdev = g_plens;

//    isp_info("zoom_move_to_pos\n");
    if(!bZoomPI_Found)
        return 0;

    down(&pdev->lock);    
   
    if(pos > lens_zoom_pos){
        //isp_info("move to %d, step = %d\n", pos, pos - lens_zoom_pos);
        ret =  near_zoom_wait(pos-lens_zoom_pos, AF_ZOOM_SPEED, 1);
    }
    else{
        //isp_info("move to %d, step = %d\n", pos, lens_zoom_pos - pos);
        ret =  inf_zoom_wait(lens_zoom_pos - pos, AF_ZOOM_SPEED, 1);
    }
    update_zoom_x();
    wait_move_end(MOT_ZEXT_PIN, 2000);
    
    update_focus_pos();
    up(&pdev->lock);
    
    return ret;
}

static s32 get_fpos(void)
{
    return lens_focus_pos;
}

static s32 get_zpos(void)
{
    return lens_zoom_pos;
}
static int zoom_find_pi(void)
{
    lens_dev_t *pdev = g_plens;
    down(&pdev->lock);

    bZoomPI_Found = true;
    lens_zoom_pos = TELE_BOUND;
    zoom_step_index = ZOOM_X_10;
//    isp_info("move zoom to pi, step %d\n", WIDE_BOUND);

    inf_zoom_wait(TELE_BOUND+100, AF_ZOOM_SPEED, false);
    pdev->focus_low_bound = zf_tab[zoom_step_index].focus_min;
    pdev->focus_hi_bound = zf_tab[zoom_step_index].focus_max;
    wait_move_end(MOT_ZEXT_PIN, 10000);
    update_focus_pos();
    up(&pdev->lock);    
    return true;
}

static int focus_find_pi(void)
{
    lens_dev_t *pdev = g_plens;
    down(&pdev->lock);
    bFocusPI_Found = true;
    near_focus(INF_BOUND+100, AF_FOCUS_SPEED, false);
/*    
    zoom_step_index = MAX_ZOOM_STEP+1;
    lens_focus_pos = zf_tab[zoom_step_index].focus_max;
//    isp_info("move focus to pi, step %d\n", zf_tab[zoom_step_index].focus_min);
    focus_move_to_pos(zf_tab[zoom_step_index].focus_min);
*/
    wait_move_end(MOT_FEXT_PIN, 10000);
//    update_focus_pos();

    lens_focus_pos = 0;

    up(&pdev->lock);

    return true;
}

static int lens_init(void)
{
    isp_info("lens_init\n");

    if (spiInit() < 0){
        isp_err("spiInit fail\n");
        return -1;
    }

    if (check_busy_all()){  //chcek R2A30440NP exist
        isp_err("Motor controller is not found\n");
        return -1;
    }
    if(!focus_find_pi())
        return -1;

#ifdef DELAY_WAIT
        mdelay(5);
#else
        msleep(5);
#endif
    
    if(!zoom_find_pi())
        return -1;
#ifdef DELAY_WAIT
        mdelay(5);
#else
        msleep(5);
#endif    

    return 0;
}

static int lens_exit(void)
{
    isp_info("lens_exit\n");

    spiExit();

    return true;
}

//============================================================================
// external functions
//============================================================================
int lens_td9533t_construct(void)
{
    lens_dev_t *pdev = g_plens;
    snprintf(pdev->name, DEV_MAX_NAME_SIZE, LENS_NAME);
    pdev->capability = LENS_SUPPORT_FOCUS | LENS_SUPPORT_ZOOM;
    pdev->fn_init = lens_init;
    pdev->fn_focus_move_to_pi = focus_find_pi;
    pdev->fn_focus_get_pos = get_fpos;
    pdev->fn_focus_set_pos = focus_move_to_pos;
    pdev->fn_focus_move = Focus_Move;
    pdev->fn_check_busy = check_busy_all;
    pdev->focus_low_bound = FOCUS_POS_MIN;
    pdev->focus_hi_bound = FOCUS_POS_MAX;
    pdev->fn_zoom_move_to_pi = zoom_find_pi;
    pdev->fn_zoom_get_pos = get_zpos;
    pdev->fn_zoom_set_pos = zoom_move_to_pos;
    pdev->fn_zoom_move = Zoom_Move;
    pdev->zoom_stage_cnt = 0;
    pdev->curr_zoom_index = 0;
    pdev->curr_zoom_x10 = 0;  
    
    sema_init(&pdev->lock, 1);

    return 0;
}

void lens_td9533t_deconstruct(void)
{
//    gpcn_exit();
    lens_exit();
}

//============================================================================
// module initialization / finalization
//============================================================================
static int __init lens_td9533t_init(void)
{
    int ret = 0;

    // check ISP driver version
    if (isp_get_input_inf_ver() != ISP_INPUT_INF_VER) {
        ret = -EFAULT;
        isp_err("Input module version(%x) is not compatibility with fisp320.ko(%x).!",
             ISP_INPUT_INF_VER, isp_get_input_inf_ver());
        goto end;
    }

    // allocate lens device information
    g_plens = kzalloc(sizeof(lens_dev_t), GFP_KERNEL);
    if (!g_plens) {
        ret = -ENODEV;
        isp_err("[ERROR]fail to alloc dev_lens!\n");
        goto end;
    }

    // construct lens device information
    ret = lens_td9533t_construct();
    if (ret < 0) {
        ret = -ENODEV;
        isp_err("[ERROR]fail to construct dev_lens!\n");
        goto err_free_lens;
    }

    isp_register_lens(g_plens);
    
    goto end;

err_free_lens:
    kfree(g_plens);
end:
    return ret;
}

static void __exit lens_td9533t_exit(void)
{
    // remove lens
    lens_td9533t_deconstruct();
    kfree(g_plens);
}

module_init(lens_td9533t_init);
module_exit(lens_td9533t_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("LENS_td9533t");
MODULE_LICENSE("GPL");

