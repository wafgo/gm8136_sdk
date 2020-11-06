/*
 * @file dh9901_tw2964_hybrid_drv.c
 * DaHua 4CH HDCVI & Intersil TW2964 4-CH SD Hybrid Receiver Driver
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.3 $
 * $Date: 2014/10/22 09:14:29 $
 *
 * ChangeLog:
 *  $Log: dh9901_tw2964_hybrid_drv.c,v $
 *  Revision 1.3  2014/10/22 09:14:29  jerson_l
 *  1. modify DH9901_VOUT_FORMAT_BT656 to DH9901_VOUT_FORMAT_BT656_DUAL_HEADER
 *
 *  Revision 1.2  2014/08/05 10:47:12  shawn_hu
 *  1. Update the sources for HD/SD hybrid frontend driver design (only for 8210).
 *  2. Please attach the DH9901/TW2964 daughterboard to GM8210 socket J77.
 *
 *  Revision 1.1  2014/08/05 09:32:30  shawn_hu
 *  Add hybrid (HDCVI/D1) frontend driver design (DH9901/TW2964).
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <mach/ftpmu010.h>


#include "platform.h"
#include "dh9901_lib.h"                ///< from module/front_end/hdcvi/dahua/dh9901_lib
#include "dahua/dh9901.h"              ///< from module/include/front_end/hdcvi/dahua
#include "intersil/tw2964.h"           ///< from module/include/front_end/decoder
#include "dh9901_tw2964_hybrid_drv.h"
#include "dh9901_dev.h"
#include "tw2964_dev.h"

/*************************************************************************************
 *  Module Parameters
 *************************************************************************************/
/* device number */
int dh9901_dev_num = DH9901_DEV_MAX;
module_param(dh9901_dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dh9901_dev_num, "DH9901 Device Number: 1~4");

int tw2964_dev_num = TW2964_DEV_MAX;
module_param(tw2964_dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tw2964_dev_num, "TW2964 Device Number: 1~4");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
ushort dh9901_iaddr[DH9901_DEV_MAX] = {0x60, 0x62, 0x64, 0x66};
module_param_array(dh9901_iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dh9901_iaddr, "DH9901 Device I2C Address");

ushort tw2964_iaddr[TW2964_DEV_MAX] = {0x50, 0x52, 0x54, 0x56};
module_param_array(tw2964_iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tw2964_iaddr, "TW2964 Device I2C Address");

/* device video mode */
int dh9901_vout_format[DH9901_DEV_MAX] = {[0 ... (DH9901_DEV_MAX - 1)] = DH9901_VOUT_FORMAT_BT656_DUAL_HEADER};
module_param_array(dh9901_vout_format, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dh9901_vout_format, "Video Output Format: 0:BT656_DUAL_HEADER, 1:BT1120");

int tw2964_vmode[TW2964_DEV_MAX] = {[0 ... (TW2964_DEV_MAX - 1)] = TW2964_VMODE_NTSC_720H_4CH};
module_param_array(tw2964_vmode, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tw2964_vmode, "Video Mode");

/* audio sample rate */
int dh9901_sample_rate = DH9901_AUDIO_SAMPLERATE_8K;
module_param(dh9901_sample_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dh9901_sample_rate, "audio sample rate: 0:8k  1:16k");

int tw2964_sample_rate = TW2964_AUDIO_SAMPLERATE_8K;
module_param(tw2964_sample_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tw2964_sample_rate, "audio sample rate: 0:8k  1:16k  2:32k 3:44.1k 4:48k");

/* audio sample size */
static int dh9901_sample_size = DH9901_AUDIO_SAMPLESIZE_16BITS;
module_param(dh9901_sample_size, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dh9901_sample_size, "audio sample size: 0:8bit  1:16bit");

int tw2964_sample_size = TW2964_AUDIO_BITS_16B;
module_param(tw2964_sample_size, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tw2964_sample_size, "audio sample size: 0:16bit  1:8bit");

/* TW2964 audio channel number */
int audio_chnum = 4;
module_param(audio_chnum, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audio_chnum, "audio channel number");

/* clock port used */
static int clk_used = 0;
module_param(clk_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_used, "External Clock Port: BIT0:EXT_CLK0, BIT1:EXT_CLK1");

/* clock source */
static int clk_src = PLAT_EXTCLK_SRC_PLL3;
module_param(clk_src, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_src, "External Clock Source: 0:PLL1OUT1 1:PLL1OUT1/2 2:PLL4OUT2 3:PLL4OUT1/2 4:PLL3");

/* clock frequency */
static int clk_freq = CLKIN;   ///< 27MHz
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

/* device use gpio as rstb pin */
static int rstb_used = 0;
module_param(rstb_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rstb_used, "Use GPIO as RSTB Pin: 0:None, 1:X_CAP_RST");

/* device channel mapping */
static int ch_map = 0;
module_param(ch_map, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_map, "channel mapping");

/* device init */
int init = 1;
module_param(init, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init, "device init: 0:no-init  1:init");

/* notify */
static int notify = 1;
module_param(notify, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(notify, "Device Notify => 0:Disable 1:Enable");

/* device use gpio as video output clock enable pin */
static int gpio_used = 0;
module_param(gpio_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(gpio_used, "Use GPIO as output clock enable Pin: 0:None, 1:GPIO#4_5_6_7");

/* Hybrid VPORT Output mode */
static ushort hybrid_vport_out = 0xFFFF; // DEV_MAX = 4
module_param(hybrid_vport_out, short, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(notify, "Hybrid VPORT Output Mode (bitmap) => 0:SD (TW2964) 1:HD (DH9901)");
/************************************************************************************** 
  e.g. hybrid_vport_out = 0x000F = 16b'0000000000001111, that means:
  DH9901_Dev#0 HD_OUT[0~3] -> all enabled = 4 * 1-CH HD bypass output
  if hybrid_vport_out = 0x00F3 = 16b'0000000011110011, that means:
  DH9901_Dev#0 HD_OUT[0~1] -> enabled, TW2964_Dev#0_SD_OUT[2~3] -> enabled
  DH9901_Dev#1 HD_OUT[0~3] -> all enabled = 4 * 1-CH HD bypass output 
  = 2 * 1-CH HD bypass output + 2 * 1-CH SD bypass output + 4 * 1-CH HD bypass output
**************************************************************************************/

/* HD/SD automatic switch */
static int auto_switch = 0; // if enabled, notify should be also set to 1
module_param(auto_switch, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(auto_switch, "Auto HD/SD switching: 0:Disable  1:Enable");
/******************************************************************************
  If this feature is enabled, it will automatically switch to HD or SD mode
  according to the state of video present/lost on that capture port. And if 
  both HD/SD video are lost, just keep at current setting.
******************************************************************************/

/******************************************************************************
 GM8287 EVB Channel and CVI Mapping Table (System Board)

 VCH#0 ------> CVI#0.0 ---------> VOUT#0.0 -------> X_CAP#0
 VCH#1 ------> CVI#0.1 ---------> VOUT#0.1 -------> X_CAP#1
 VCH#2 ------> CVI#0.2 ---------> VOUT#0.2 -------> X_CAP#2
 VCH#3 ------> CVI#0.3 ---------> VOUT#0.3 -------> X_CAP#3

 ------------------------------------------------------------------------------
 GM8287 EVB Channel and CVI Mapping Table (Socket Board)

 VCH#0 ------> CVI#0.0 ---------> VOUT#0.0 -------> X_CAP#1
 VCH#1 ------> CVI#0.1 ---------> VOUT#0.1 -------> X_CAP#0
 VCH#2 ------> CVI#0.2 ---------> VOUT#0.2 -------> X_CAP#3
 VCH#3 ------> CVI#0.3 ---------> VOUT#0.3 -------> X_CAP#2

 ------------------------------------------------------------------------------
 GM8210/GM8287 EVB Channel and VIN Mapping Table (Socket Board)

 VCH#0 ------> VIN#0 -----------> VOUT#0.0 -------> X_CAP#3
 VCH#1 ------> VIN#1 -----------> VOUT#0.1 -------> X_CAP#2
 VCH#2 ------> VIN#2 -----------> VOUT#0.2 -------> X_CAP#1
 VCH#3 ------> VIN#3 -----------> VOUT#0.3 -------> X_CAP#0

*******************************************************************************/

/*************************************************************************************
 *  Original structure
 *************************************************************************************/
struct dh9901_dev_t dh9901_dev[DH9901_DEV_MAX];
struct tw2964_dev_t tw2964_dev[TW2964_DEV_MAX];
static audio_funs_t tw2964_audio_funs;
static audio_funs_t dh9901_audio_funs;

/*************************************************************************************
 *  Data structure or APIs for hybrid design
 *************************************************************************************/
static struct i2c_client  *dh9901_tw2964_i2c_client = NULL;
static struct task_struct *dh9901_tw2964_wd  = NULL;
static ushort last_hybrid_vport_out;
static struct proc_dir_entry *proc_hybrid_vport_out = NULL;

u32 DH9901_TW2964_VCH_MAP[DH9901_DEV_MAX*DH9901_DEV_CH_MAX];

enum {
    TW2964_VFMT_960H25 = DH9901_VFMT_MAX,
    TW2964_VFMT_960H30,
    TW2964_VFMT_720H25,
    TW2964_VFMT_720H30,
    DH9901_TW2964_VFMT_MAX
};

struct dh9901_video_fmt_t dh9901_tw2964_video_fmt_info[DH9901_TW2964_VFMT_MAX] = {
    /* Idx                    Width   Height  Prog    FPS */
    {  DH9901_VFMT_720P25,    1280,   720,    1,      25  },  ///< 720P @25
    {  DH9901_VFMT_720P30,    1280,   720,    1,      30  },  ///< 720P @30
    {  DH9901_VFMT_720P50,    1280,   720,    1,      50  },  ///< 720P @50
    {  DH9901_VFMT_720P60,    1280,   720,    1,      60  },  ///< 720P @60
    {  DH9901_VFMT_1080P25,   1920,   1080,   1,      25  },  ///< 1080P@25
    {  DH9901_VFMT_1080P30,   1920,   1080,   1,      30  },  ///< 1080P@30
    {  TW2964_VFMT_960H25,     960,    576,   0,      50  },  ///< 960H @25
    {  TW2964_VFMT_960H30,     960,    480,   0,      60  },  ///< 960H @30
    {  TW2964_VFMT_720H25,     720,    576,   0,      50  },  ///< 720H @25
    {  TW2964_VFMT_720H30,     720,    480,   0,      60  },  ///< 720H @30
};

int dh9901_get_device_num(void)
{
    // Let vcap300_dh9901 create maximum bypass video out path
    return (tw2964_dev_num >= dh9901_dev_num) ? tw2964_dev_num : dh9901_dev_num;
}
EXPORT_SYMBOL(dh9901_get_device_num);

static int dh9901_tw2964_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    dh9901_tw2964_i2c_client = client;
    return 0;
}

static int dh9901_tw2964_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id dh9901_tw2964_i2c_id[] = {
    { "dh9901_tw2964_i2c", 0 },
    { }
};

static struct i2c_driver dh9901_tw2964_i2c_driver = {
    .driver = {
        .name  = "DH9901_TW2964_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = dh9901_tw2964_i2c_probe,
    .remove   = dh9901_tw2964_i2c_remove,
    .id_table = dh9901_tw2964_i2c_id
};

static int dh9901_tw2964_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&dh9901_tw2964_i2c_driver);
    if(ret < 0) {
        printk("DH9901 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = dh9901_iaddr[0]>>1;
    strlcpy(info.type, "dh9901_tw2964_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("DH9901&TW2964 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("DH9901&TW2964 can't add i2c device!\n");
        goto err_driver;
    }
    
    return 0;

err_driver:
   i2c_del_driver(&dh9901_tw2964_i2c_driver);
   return -ENODEV;
}

static void dh9901_tw2964_i2c_unregister(void)
{
    i2c_unregister_device(dh9901_tw2964_i2c_client);
    i2c_del_driver(&dh9901_tw2964_i2c_driver);
    dh9901_tw2964_i2c_client = NULL;
}

int dh9901_tw2964_i2c_write(u8 addr, u8 reg, u8 data)
{
    u8 buf[2];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(!dh9901_tw2964_i2c_client) {
        printk("DH9901&TW2964 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(dh9901_tw2964_i2c_client->dev.parent);

    buf[0]     = reg;
    buf[1]     = data;
    msgs.addr  = addr>>1;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("DH9901 or TW2964 @ 0x%02x i2c write failed!!\n", addr);
        return -1;
    }

    return 0;
}

u8 dh9901_tw2964_i2c_read(u8 addr, u8 reg)
{
    u8 reg_data = 0;
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(!dh9901_tw2964_i2c_client) {
        printk("DH9901&TW2964 i2c_client not register!!\n");
        return 0;
    }

    adapter = to_i2c_adapter(dh9901_tw2964_i2c_client->dev.parent);

    msgs[0].addr  = addr>>1;
    msgs[0].flags = 0; /* write */
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = (addr + 1)>>1;
    msgs[1].flags = 1; /* read */
    msgs[1].len   = 1;
    msgs[1].buf   = &reg_data;

    if(i2c_transfer(adapter, msgs, 2) != 2)
        printk("DH9901 or TW2964 @ 0x%02x i2c read failed!!\n", addr);

    return reg_data;
}

static int dh9901_tw2964_create_ch_map(int map_id)
{
    int i;

    switch(map_id) {
        case 0: ///< for GM8210/GM8287 Socket Board
        default:
            for(i=0; i<dh9901_get_device_num(); i++) {
                DH9901_TW2964_VCH_MAP[(i*DH9901_DEV_CH_MAX)+0] = CH_IDENTIFY(i, 0, DH9901_DEV_VOUT_0);
                DH9901_TW2964_VCH_MAP[(i*DH9901_DEV_CH_MAX)+1] = CH_IDENTIFY(i, 1, DH9901_DEV_VOUT_1);
                DH9901_TW2964_VCH_MAP[(i*DH9901_DEV_CH_MAX)+2] = CH_IDENTIFY(i, 2, DH9901_DEV_VOUT_2);
                DH9901_TW2964_VCH_MAP[(i*DH9901_DEV_CH_MAX)+3] = CH_IDENTIFY(i, 3, DH9901_DEV_VOUT_3);
            }
            break;
    }

    return 0;
}

static void dh9901_tw2964_hw_reset(void)
{
    if(!rstb_used || !init)
        return;

    /* Active Low */
    plat_gpio_set_value(rstb_used, 0);

    udelay(100);

    /* Active High and release reset */
    plat_gpio_set_value(rstb_used, 1);

    udelay(100);
}

static void dh9901_tw2964_params_passing(void)
{
    struct dh9901_params_t dh9901_params;
    struct tw2964_params_t tw2964_params;

    dh9901_params.dh9901_dev_num = dh9901_dev_num;
    dh9901_params.dh9901_iaddr = &dh9901_iaddr[0];
    dh9901_params.dh9901_vout_format = &dh9901_vout_format[0];
    dh9901_params.dh9901_sample_rate = dh9901_sample_rate;
    dh9901_params.dh9901_dev = &dh9901_dev[0];
    dh9901_params.init = init;
    dh9901_params.dh9901_tw2964_video_fmt_info = &dh9901_tw2964_video_fmt_info[0];
    dh9901_params.DH9901_TW2964_VCH_MAP = &DH9901_TW2964_VCH_MAP[0];
    dh9901_set_params(&dh9901_params);

    tw2964_params.tw2964_dev_num = tw2964_dev_num;
    tw2964_params.tw2964_iaddr = &tw2964_iaddr[0];
    tw2964_params.tw2964_vmode = &tw2964_vmode[0];
    tw2964_params.tw2964_sample_rate = tw2964_sample_rate;
    tw2964_params.tw2964_sample_size = tw2964_sample_size;
    tw2964_params.tw2964_dev = &tw2964_dev[0];
    tw2964_params.init = init;
    tw2964_params.DH9901_TW2964_VCH_MAP = &DH9901_TW2964_VCH_MAP[0];
    tw2964_set_params(&tw2964_params);
}

static int dh9901_tw2964_watchdog_thread(void *data)
{
    int ret, i, ch;
    int dev_id, novid, vmode, total_ch_num, vlos;
    ushort hybrid_vport_out_mask, hybrid_vport_out_xor;
    u8 val;

    DH_VIDEO_STATUS_S cvi_sts;
    struct dh9901_dev_t *dh9901_pdev;
    struct tw2964_dev_t *tw2964_pdev;
    struct dh9901_video_fmt_t *p_vfmt;    

    total_ch_num = dh9901_get_device_num() * DH9901_DEV_CH_MAX;
    switch((dh9901_dev_num >= tw2964_dev_num) ? tw2964_dev_num : dh9901_dev_num){
        case 4:
            hybrid_vport_out_mask = 0xFFFF;
            break;
        case 3:
            hybrid_vport_out_mask = 0x0FFF;
            break;
        case 2:
            hybrid_vport_out_mask = 0x00FF;
            break;
        case 1:
            hybrid_vport_out_mask = 0x000F;
            break;
        default:
            hybrid_vport_out_mask = 0x0000;
            break;
    }

    do {
        hybrid_vport_out_xor = hybrid_vport_out ^ last_hybrid_vport_out;
        if(hybrid_vport_out_xor & hybrid_vport_out_mask){ // check hybrid vport output mode switching
            for(i=0; i < total_ch_num; i++) {
                if(hybrid_vport_out_xor & (0x1<<i)) {
                    dev_id = i / DH9901_DEV_CH_MAX;
                    ch = i % DH9901_DEV_CH_MAX;
                    dh9901_pdev = &dh9901_dev[dev_id];
                    tw2964_pdev = &tw2964_dev[dev_id];
                    down(&dh9901_pdev->lock);
                    down(&tw2964_pdev->lock);
                    /* HD/SD switching */
                    if(hybrid_vport_out & (0x1<<i)) { // HD enabled
                        /* Get current video status */
                        ret = DH9901_API_GetVideoStatus(dev_id, ch, &cvi_sts);
                        if(ret != 0) {
                            printk("Can't get video status at Dev#%d!\n", dev_id);
                            continue;
                        }
                        /* Get video format information */
                        if(cvi_sts.ucVideoFormat >= DH9901_VFMT_MAX)
                            p_vfmt = &dh9901_tw2964_video_fmt_info[DH9901_VFMT_720P25];
                        else
                            p_vfmt = &dh9901_tw2964_video_fmt_info[cvi_sts.ucVideoFormat];
                        /* Disable SD output from TW2964 */
                        val = tw2964_i2c_read(dev_id, 0x65);
                        ret = tw2964_i2c_write(dev_id, 0x65, (val | 0x1<<ch));
                        if(ret < 0){
                            printk("Can't disable SD video output at Dev#%d!\n", dev_id);
                            continue;
                        }
                        switch(gpio_used) {
                            case 1: ///< for GM8210 Socket Board, and daughterboard should be attached to J77
                                switch(ch) {
                                    case 0:
                                        plat_gpio_set_value(PLAT_GPIO_ID_7, 0);
                                        break;
                                    case 1:
                                        plat_gpio_set_value(PLAT_GPIO_ID_6, 0);
                                        break;
                                    case 2:
                                        plat_gpio_set_value(PLAT_GPIO_ID_5, 0);
                                        break;
                                    case 3:
                                        plat_gpio_set_value(PLAT_GPIO_ID_4, 0);
                                        break;
                                }
                                break;
                            case 0:
                            default:
                                break;
                        }
                        /* Enable HD output from DH9901 */
                        ret = DH9901_API_ReadReg(dev_id, 0, 0xBB, &val);
                        if(ret != 0){
                            printk("Can't enable HD video output at Dev#%d!\n", dev_id);
                            continue;
                        }
                        val &= ~(0x1<<ch);
                        ret = DH9901_API_WriteReg(dev_id, 0, 0xBB, &val);
                        if(ret != 0){
                            printk("Can't enable HD video output at Dev#%d!\n", dev_id);
                            continue;
                        }
                        ret = DH9901_API_WriteReg(dev_id, 0, 0xBC, &val);
                        if(ret != 0){
                            printk("Can't enable HD video output at Dev#%d!\n", dev_id);
                            continue;
                        }
                    }
                    else { // SD enabled
                        switch(tw2964_vmode[dev_id]) { // Set SD video format
                            case TW2964_VMODE_PAL_960H_1CH:
                                p_vfmt = &dh9901_tw2964_video_fmt_info[TW2964_VFMT_960H25];
                                break;
                            case TW2964_VMODE_NTSC_960H_1CH:
                                p_vfmt = &dh9901_tw2964_video_fmt_info[TW2964_VFMT_960H30];
                                break;
                            case TW2964_VMODE_PAL_720H_1CH:
                                p_vfmt = &dh9901_tw2964_video_fmt_info[TW2964_VFMT_720H25];
                                break;
                            case TW2964_VMODE_NTSC_720H_1CH:
                            default:
                                p_vfmt = &dh9901_tw2964_video_fmt_info[TW2964_VFMT_720H30];
                                break;
                        }
                        /* Disable HD output from DH9901 */
                        ret = DH9901_API_ReadReg(dev_id, 0, 0xBB, &val);
                        if(ret != 0){
                            printk("Can't enable HD video output at Dev#%d!\n", dev_id);
                            continue;
                        }
                        val |= 0x1<<ch;
                        ret = DH9901_API_WriteReg(dev_id, 0, 0xBB, &val);
                        if(ret != 0){
                            printk("Can't enable HD video output at Dev#%d!\n", dev_id);
                            continue;
                        }
                        ret = DH9901_API_WriteReg(dev_id, 0, 0xBC, &val);
                        if(ret != 0){
                            printk("Can't enable HD video output at Dev#%d!\n", dev_id);
                            continue;
                        }
                        /* Enable SD output from TW2964 */
                        val = tw2964_i2c_read(dev_id, 0x65);
                        ret = tw2964_i2c_write(dev_id, 0x65, (val & ~(0x1<<ch)));
                        if(ret < 0){
                            printk("Can't enable SD video output at Dev#%d!\n", dev_id);
                            continue;
                        }
                        switch(gpio_used) {
                            case 1: ///< for GM8210 Socket Board, and daughterboard should be attached to J77
                                switch(ch) {
                                    case 0:
                                        plat_gpio_set_value(PLAT_GPIO_ID_7, 1);
                                        break;
                                    case 1:
                                        plat_gpio_set_value(PLAT_GPIO_ID_6, 1);
                                        break;
                                    case 2:
                                        plat_gpio_set_value(PLAT_GPIO_ID_5, 1);
                                        break;
                                    case 3:
                                        plat_gpio_set_value(PLAT_GPIO_ID_4, 1);
                                        break;
                                }
                                break;
                            case 0:
                            default:
                                break;
                        }
                    }
                    if(notify && dh9901_pdev->vfmt_notify) {
                        dh9901_pdev->vfmt_notify(dev_id, ch, p_vfmt);
                        printk("Issue a notification for HD/SD switching on Dev#%d-CH%d!\n", dev_id, ch);
                    }
                    up(&tw2964_pdev->lock);
                    up(&dh9901_pdev->lock);
                }
            }
        last_hybrid_vport_out = hybrid_vport_out;
        }
        else { //check channel status
            for(i=0; i < total_ch_num; i++) {
                dev_id = i / DH9901_DEV_CH_MAX;
                ch = i % DH9901_DEV_CH_MAX;
                dh9901_pdev = &dh9901_dev[dev_id];
                tw2964_pdev = &tw2964_dev[dev_id];
                down(&dh9901_pdev->lock);
                down(&tw2964_pdev->lock);
                if((hybrid_vport_out & hybrid_vport_out_mask) & (0x1<<i)) { // HD enabled, check dh9901 channel status
                    ret = DH9901_API_GetVideoStatus(dev_id, ch, &cvi_sts);
                    if(ret != 0) {
                        printk("Can't get video status at Dev#%d!\n", dev_id);
                        continue;
                    }
                    /* get video format information */
                    if(cvi_sts.ucVideoFormat >= DH9901_VFMT_MAX)
                        p_vfmt = &dh9901_tw2964_video_fmt_info[DH9901_VFMT_720P25];
                    else
                        p_vfmt = &dh9901_tw2964_video_fmt_info[cvi_sts.ucVideoFormat];

                    dh9901_pdev->cur_plugin[ch] = (cvi_sts.ucVideoLost == 0) ? 1 : 0;
            
                    if((dh9901_pdev->cur_plugin[ch] == 1) && (dh9901_pdev->pre_plugin[ch] == 0)) {        ///< cable plugged-in
                        goto sts_update;
                    }
                    else if((dh9901_pdev->cur_plugin[ch] == 1) && (dh9901_pdev->pre_plugin[ch] == 1)) {   ///< cable is connected
                            /* notify current video present */
                            if(notify && dh9901_pdev->vlos_notify)
                                dh9901_pdev->vlos_notify(dev_id, ch, 0);
            
                            /* notify current video format */
                            if(notify && dh9901_pdev->vfmt_notify) 
                                dh9901_pdev->vfmt_notify(dev_id, ch, p_vfmt);
                    }
                    else {  ///< cable is plugged-out
                        /* notify current video loss */
                        if(notify && dh9901_pdev->vlos_notify)
                            dh9901_pdev->vlos_notify(dev_id, ch, 1);

                        /* notify current video format, video loss will switch to 720P25 */
                        if(notify && dh9901_pdev->vfmt_notify)
                            dh9901_pdev->vfmt_notify(dev_id, ch, p_vfmt);

                        if(auto_switch) {
                            novid = tw2964_status_get_novid(tw2964_pdev->index, ch);
                            if(!novid) // SD video present
                                hybrid_vport_out &= ~(0x1<<(ch+dev_id*DH9901_DEV_CH_MAX));
                        }
                    }
sts_update:     
                    dh9901_pdev->pre_plugin[ch] = dh9901_pdev->cur_plugin[ch];
                }                
                else { // SD enabled, check tw2964 video status
                    /* video signal check */
                    if(notify && dh9901_pdev->vlos_notify) {
                        novid = tw2964_status_get_novid(tw2964_pdev->index, ch);
                        
                        if(novid) {
                            dh9901_pdev->vlos_notify(dev_id, ch, 1);
                            if(auto_switch) {
                                ret = DH9901_API_GetVideoStatus(dh9901_pdev->index, ch, &cvi_sts);
                                vlos = (ret == 0) ? cvi_sts.ucVideoLost : 0;
                                if(!vlos) // HD video present
                                    hybrid_vport_out |= 0x1<<(ch+dev_id*DH9901_DEV_CH_MAX);
                            }
                        }
                        else
                            dh9901_pdev->vlos_notify(dev_id, ch, 0);
                    }
                    /* video format check */
                    if(notify && dh9901_pdev->vfmt_notify) {
                        vmode = tw2964_video_get_mode(tw2964_pdev->index);
                        switch(vmode) { // Set SD video format
                            case TW2964_VMODE_PAL_960H_1CH:
                                p_vfmt = &dh9901_tw2964_video_fmt_info[TW2964_VFMT_960H25];
                                break;
                            case TW2964_VMODE_NTSC_960H_1CH:
                                p_vfmt = &dh9901_tw2964_video_fmt_info[TW2964_VFMT_960H30];
                                break;
                            case TW2964_VMODE_PAL_720H_1CH:
                                p_vfmt = &dh9901_tw2964_video_fmt_info[TW2964_VFMT_720H25];
                                break;
                            case TW2964_VMODE_NTSC_720H_1CH:
                            default:
                                p_vfmt = &dh9901_tw2964_video_fmt_info[TW2964_VFMT_720H30];
                                break;
                        }
                        dh9901_pdev->vfmt_notify(dev_id, ch, p_vfmt);
                    }
                }
                up(&tw2964_pdev->lock);
                up(&dh9901_pdev->lock);
            }
        }
        /* sleep 1 second */
        schedule_timeout_interruptible(msecs_to_jiffies(500));
    } while (!kthread_should_stop());

    return 0;
}

static int proc_hybrid_vport_out_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "0x%x\n", hybrid_vport_out);   
    return 0;
}

static ssize_t proc_hybrid_vport_out_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int vport_out;
    char value_str[16] = {'\0'};
        
    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "0x%x\n", &vport_out);

    hybrid_vport_out = (ushort)vport_out;

    return count;
}

static int proc_hybrid_vport_out_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_hybrid_vport_out_show, PDE(inode)->data);
}

static struct file_operations proc_hybrid_vport_out_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_hybrid_vport_out_open,
    .write  = proc_hybrid_vport_out_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void hybrid_proc_remove(void)
{
    if(proc_hybrid_vport_out) {
        remove_proc_entry(proc_hybrid_vport_out->name, NULL);
        proc_hybrid_vport_out = NULL;
    }
}

static int hybrid_proc_init(void)
{
    int ret = 0;
    /* hybrid vport out */
    proc_hybrid_vport_out = create_proc_entry("hybrid_vport_out", S_IRUGO|S_IXUGO, NULL);
    if(!proc_hybrid_vport_out) {
        printk("create proc node /proc/hybrid_vport_out failed!\n");
        ret = -EINVAL;
        goto err;
    }
    proc_hybrid_vport_out->proc_fops = &proc_hybrid_vport_out_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    proc_hybrid_vport_out->owner     = THIS_MODULE;
#endif
    return ret;
err:
    hybrid_proc_remove();
    return ret;
}

static int __init dh9901_tw2964_init(void)
{
    int i, ret;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dh9901_dev_num > DH9901_DEV_MAX) {
        printk("DH9901 dh9901_dev_num=%d invalid!!(Max=%d)\n", dh9901_dev_num, DH9901_DEV_MAX);
        return -1;
    }
    if(tw2964_dev_num > TW2964_DEV_MAX) {
        printk("TW2964 tw2964_dev_num=%d invalid!!(Max=%d)\n", tw2964_dev_num, TW2964_DEV_MAX);
        return -1;
    }

    /* check video output format */
    for(i=0; i<dh9901_dev_num; i++) {
        if((dh9901_vout_format[i] < 0) || (dh9901_vout_format[i] >= DH9901_VOUT_FORMAT_MAX)) {
            printk("DH9901#%d vout_format=%d not support!!\n", i, dh9901_vout_format[i]);
            return -1;
        }
    }

    /* register i2c client for contol dh9901 & tw2964 */
    ret = dh9901_tw2964_i2c_register();
    if(ret < 0)
        return -1;

    /* register gpio pin for device rstb pin */
    if(rstb_used && init) {
        ret = plat_register_gpio_pin(rstb_used, PLAT_GPIO_DIRECTION_OUT, 0);
        if(ret < 0)
            goto err;
    }
    /* register gpio pin for TW2964 clk out */
    if(init) {
        switch(gpio_used) {
            case 1:  ///< for GM8210 Socket Board, and daughterboard should be attached to J77
                ret = plat_register_gpio_pin(PLAT_GPIO_ID_4, PLAT_GPIO_DIRECTION_OUT, 0);
                if(ret < 0)
                    goto err;
                ret = plat_register_gpio_pin(PLAT_GPIO_ID_5, PLAT_GPIO_DIRECTION_OUT, 0);
                if(ret < 0)
                    goto err;
                ret = plat_register_gpio_pin(PLAT_GPIO_ID_6, PLAT_GPIO_DIRECTION_OUT, 0);
                if(ret < 0)
                    goto err;
                ret = plat_register_gpio_pin(PLAT_GPIO_ID_7, PLAT_GPIO_DIRECTION_OUT, 0);
                if(ret < 0)
                    goto err;
                break;
            case 0:
            default:
                break;
        }
    }

    /* setup platfrom external clock */
    if((init == 1) && (clk_used >= 0)) {
        if(clk_used) {
            ret = plat_extclk_set_src(clk_src);
            if(ret < 0)
                goto err;

            if(clk_used & 0x1) {
                ret = plat_extclk_set_freq(0, clk_freq);
                if(ret < 0)
                    goto err;
            }

            if(clk_used & 0x2) {
                ret = plat_extclk_set_freq(1, clk_freq);
                if(ret < 0)
                    goto err;
            }

            ret = plat_extclk_onoff(1);
            if(ret < 0)
                goto err;

        }
        else {
            /* use external oscillator. disable SoC external clock output */
            ret = plat_extclk_onoff(0);
            if(ret < 0)
                goto err;
        }
    }

    /* create channel mapping table for different board */
    ret = dh9901_tw2964_create_ch_map(ch_map);
    if(ret < 0)
        goto err;

    /* hardware reset for all device */
    dh9901_tw2964_hw_reset();

    /* Passing required parameters to dh9901_dev.c & tw2964_dev.c */
    dh9901_tw2964_params_passing();

    /* DH9901 init */
    for(i=0; i<dh9901_dev_num; i++) {
        dh9901_dev[i].index = i;

        sprintf(dh9901_dev[i].name, "dh9901.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&dh9901_dev[i].lock, 1);
#else
        init_MUTEX(&dh9901_dev[i].lock);
#endif
        ret = dh9901_proc_init(i);
        if(ret < 0)
            goto err;

        ret = dh9901_miscdev_init(i);
        if(ret < 0)
            goto err;

    }
    /* dh9901 device register */
    ret = dh9901_device_register();
    if(ret != 0)
        goto err;

    /* TW2964 init */
    for(i=0; i<tw2964_dev_num; i++) {
        tw2964_dev[i].index = i;

        sprintf(tw2964_dev[i].name, "tw2964.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&tw2964_dev[i].lock, 1);
#else
        init_MUTEX(&tw2964_dev[i].lock);
#endif
        ret = tw2964_proc_init(i);
        if(ret < 0)
            goto err;

        ret = tw2964_miscdev_init(i);
        if(ret < 0)
            goto err;

        ret = tw2964_device_init(i);
        if(ret < 0)
            goto err;
    }

    /* create hybrid proc node */
    ret = hybrid_proc_init();
    if(ret < 0)
            goto err;

    /* set flag */
    last_hybrid_vport_out = hybrid_vport_out;

    if(init) {
        /* init dh9901_tw2964 watchdog thread for check video status */
        dh9901_tw2964_wd = kthread_create(dh9901_tw2964_watchdog_thread, NULL, "dh9901_tw2964_wd");
        if(!IS_ERR(dh9901_tw2964_wd))
            wake_up_process(dh9901_tw2964_wd);
        else {
            printk("create dh9901 watchdog thread failed!!\n");
            dh9901_tw2964_wd = 0;
            ret = -EFAULT;
            goto err;
        }
    }

    /* register audio function for platform used */
    dh9901_audio_funs.sound_switch = dh9901_sound_switch;
    dh9901_audio_funs.codec_name   = dh9901_dev[0].name;
    plat_audio_register_function(&dh9901_audio_funs);
    tw2964_audio_funs.sound_switch = tw2964_sound_switch;
    tw2964_audio_funs.codec_name   = tw2964_dev[0].name;
    plat_audio_register_function(&tw2964_audio_funs);

    return 0;

err:
    if(dh9901_tw2964_wd)
        kthread_stop(dh9901_tw2964_wd);

    dh9901_device_deregister();

    dh9901_tw2964_i2c_unregister();
    for(i=0; i<DH9901_DEV_MAX; i++) {
        if(dh9901_dev[i].miscdev.name)
            misc_deregister(&dh9901_dev[i].miscdev);

        dh9901_proc_remove(i);
    }
    for(i=0; i<TW2964_DEV_MAX; i++) {
        if(tw2964_dev[i].miscdev.name)
            misc_deregister(&tw2964_dev[i].miscdev);

        tw2964_proc_remove(i);
    }
    
    hybrid_proc_remove();

    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    if(init) {
        switch(gpio_used) {
            case 1:  ///< for GM8210 Socket Board, and daughterboard should be attached to J77
                plat_deregister_gpio_pin(PLAT_GPIO_ID_4);
                plat_deregister_gpio_pin(PLAT_GPIO_ID_5);
                plat_deregister_gpio_pin(PLAT_GPIO_ID_6);
                plat_deregister_gpio_pin(PLAT_GPIO_ID_7);
                break;
            case 0:
            default:
                break;
        }
    }

    return ret;
}

static void __exit dh9901_tw2964_exit(void)
{
    int i;

    /* stop dh9901_tw2964 watchdog thread */
    if(dh9901_tw2964_wd)
        kthread_stop(dh9901_tw2964_wd);

    dh9901_device_deregister();

    dh9901_tw2964_i2c_unregister();

    for(i=0; i<DH9901_DEV_MAX; i++) {
        /* remove device node */
        if(dh9901_dev[i].miscdev.name)
            misc_deregister(&dh9901_dev[i].miscdev);

        /* remove proc node */
        dh9901_proc_remove(i);
    }
    for(i=0; i<TW2964_DEV_MAX; i++) {
        /* remove device node */
        if(tw2964_dev[i].miscdev.name)
            misc_deregister(&tw2964_dev[i].miscdev);

        /* remove proc node */
        tw2964_proc_remove(i);
    }
    
    /* remove hybrid proc node */
    hybrid_proc_remove();

    /* release gpio pin */
    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    if(init) {
        switch(gpio_used) {
            case 1: ///< for GM8210 Socket Board, and daughterboard should be attached to J77
                plat_deregister_gpio_pin(PLAT_GPIO_ID_4);
                plat_deregister_gpio_pin(PLAT_GPIO_ID_5);
                plat_deregister_gpio_pin(PLAT_GPIO_ID_6);
                plat_deregister_gpio_pin(PLAT_GPIO_ID_7);
                break;
            case 0:
            default:
                break;
        }
    }

    plat_audio_deregister_function();
}

module_init(dh9901_tw2964_init);
module_exit(dh9901_tw2964_exit);

MODULE_DESCRIPTION("Grain Media - DAHUA DH9901 4CH HDCVI & Intersil TW2964 4CH SD Hybrid Receiver Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
