/**
 * @file generic.c
 * VCAP300 Generic Input Device Driver
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.13 $
 * $Date: 2014/10/30 01:47:32 $
 *
 * ChangeLog:
 *  $Log: generic_comm.c,v $
 *  Revision 1.13  2014/10/30 01:47:32  jerson_l
 *  1. support vi 2ch dual edge hybrid channel mode.
 *  2. support grab channel horizontal blanking data.
 *
 *  Revision 1.12  2014/08/07 11:29:49  jerson_l
 *  1. support interface and bt601_param proc node to swicth VI setting
 *
 *  Revision 1.11  2014/03/31 01:44:47  jerson_l
 *  1. support dynamic switch resolution without stop channel
 *  2. support notify when frame rate or resolution switched
 *
 *  Revision 1.10  2014/02/06 02:58:22  jerson_l
 *  1. modify version compare rule of input module driver
 *
 *  Revision 1.9  2013/11/07 06:39:59  jerson_l
 *  1. support max_frame_rate infomation
 *
 *  Revision 1.8  2013/10/14 04:04:04  jerson_l
 *  1. modify data swap definition
 *
 *  Revision 1.7  2013/10/01 06:18:19  jerson_l
 *  1. support new channel mapping mechanism
 *
 *  Revision 1.6  2013/08/23 09:18:52  jerson_l
 *  1. support timeout/frame_rate/speed/rgb_param control proc node
 *
 *  Revision 1.5  2013/04/30 09:29:07  jerson_l
 *  1. support SDI8BIT format
 *
 *  Revision 1.4  2013/01/15 09:40:42  jerson_l
 *  1. add vi_src and data_swap for capture port pinmux control
 *
 *  Revision 1.3  2012/12/11 09:17:42  jerson_l
 *  1. add RGB888 module parameter for support RGB888 interface
 *  2. use semaphore to protect module resource
 *
 *  Revision 1.2  2012/10/24 06:47:51  jerson_l
 *
 *  switch file to unix format
 *
 *  Revision 1.1.1.1  2012/10/23 08:16:31  jerson_l
 *  import vcap300 driver
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#include "vcap_input.h"
#include "vcap_dbg.h"

#define DEFAULT_TYPE            VCAP_INPUT_TYPE_GENERIC
#define DEFAULT_NORM_WIDTH      640
#define DEFAULT_NORM_HEIGHT     480
#define DEFAULT_FRAME_RATE      30
#define DEFAULT_INTERFACE       VCAP_INPUT_INTF_BT656_PROGRESSIVE
#define DEFAULT_MODE            VCAP_INPUT_MODE_BYPASS
#define DEFAULT_SPEED           VCAP_INPUT_SPEED_P
#define DEFAULT_ORDER           VCAP_INPUT_ORDER_ANYONE
#define DEFAULT_DATA_RANGE      VCAP_INPUT_DATA_RANGE_256
#define DEFAULT_TIMEOUT_MS      1000                          ///< 1000ms

static int vi = GENERIC_ID;
module_param(vi, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vi, "VI Number");

static int vi_src = -1;     ///< -1: means source from X_CAP#vi
module_param(vi_src, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vi_src, "VI Source, 0~7:X_CAP0~7, 8:X_CAPCAS, 9:LCD0, 10:LCD1, 11:LCD2, 12:ISP");

static int interface = DEFAULT_INTERFACE;
module_param(interface, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(interface, "0:BT656_I, 1:BT656_P, 2:BT1120_I, 3:BT1120_P, 4:RGB888, 5:SDI8BIT_I, 6:SDI8BIT_P, 7:BT601_8BIT_I, 8:BT601_8BIT_P, 9:BT601_16BIT_I, 10:BT601_16BIT_P, 11:ISP");

static int mode = DEFAULT_MODE;
module_param(mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "0:Bypass, 1:4CH Frame Interleave, 2:2CH Byte Intereave, 3:4CH Byte Interleave, 4:2CH Byte Intereave Hybrid");

static int width = DEFAULT_NORM_WIDTH;
module_param(width, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(width, "Source Width");

static int height = DEFAULT_NORM_HEIGHT;
module_param(height, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(height, "Source Height");

static int order = DEFAULT_ORDER;
module_param(order, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(order, "Field Order=> 0:Anyone, 1:Odd First, 2:Even First");

static int range = DEFAULT_DATA_RANGE;
module_param(range, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(range, "Data Range=> 0:256 Level, 1:240 Level");

static int inv_clk = 0;
module_param(inv_clk, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "Invert Clock, 0:Disable, 1: Enable");

static int yc_swap = 0;
module_param(yc_swap, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(yc_swap, "0: None, 1:YC Swap, 2:CbCr Swap, 3:YC and CbCr Swap");

static int data_swap = 0;
module_param(data_swap, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(data_swap, "0: None, 1:Lo8Bit, 2:Byte, 3:Lo8Bit+Byte, 4:Hi8Bit, 5:LoHi8Bit, 6:Hi8Bit+Byte, 7:LoHi8Bit+Byte");

static int frame_rate = DEFAULT_FRAME_RATE;
module_param(frame_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(frame_rate, "Source Frame Rate");

static int speed = DEFAULT_SPEED;
module_param(speed, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(speed, "Speed Type: 0:50/60I, 1:25/30P, 2:50/60P");

static int timeout = DEFAULT_TIMEOUT_MS;
module_param(timeout, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(timeout, "Signal Timeout Threshold(ms)");

static int rgb_param[5] = {0, 0, 0, 0, 0};
module_param_array(rgb_param, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rgb_param, "RGB_Param=> [VS_POL(0/1), HS_POL(0/1), DE_POL(0/1), WATCH_DE(0/1), H_RATIO(0~15)]");

static int bt601_param[5] = {0, 0, 0, 0, 0};
module_param_array(bt601_param, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(bt601_param, "BT601_Param=> [VS_POL(0/1), HS_POL(0/1), SYNC_MODE(0/1), X_OFFSET(0~255), Y_OFFSET(0~255)]");

static int ch_id[VCAP_INPUT_DEV_CH_MAX] = {[0 ... (VCAP_INPUT_DEV_CH_MAX - 1)] = -1};    ///<default is base on vi number
module_param_array(ch_id, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_id, "Video Channel Index");

static int ch_norm[VCAP_INPUT_DEV_CH_MAX] = {[0 ... (VCAP_INPUT_DEV_CH_MAX - 1)] = -1};  ///<default is base on vi config
module_param_array(ch_norm, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_norm, "VI Channel Norm Type, 0:PAL, 1:NTSC, 2:960H_PAL, 3:960H_NTSC, 4:720P@25, 5:720P@30, 6:720P@50, 7:720P@60, 8:1080P@25, 9:1080P@30");

static int probe_chid = VCAP_INPUT_PROBE_CHID_DISABLE;
module_param(probe_chid, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(probe_chid, "0:Disable 1: CHID From HBlank");

static int generic_norm = 0;
static struct vcap_input_dev_t *generic_dev              = NULL;
static struct proc_dir_entry   *generic_proc_root        = NULL;
static struct proc_dir_entry   *generic_proc_norm        = NULL;
static struct proc_dir_entry   *generic_proc_interface   = NULL;
static struct proc_dir_entry   *generic_proc_framerate   = NULL;
static struct proc_dir_entry   *generic_proc_speed       = NULL;
static struct proc_dir_entry   *generic_proc_timeout     = NULL;
static struct proc_dir_entry   *generic_proc_rgb_param   = NULL;
static struct proc_dir_entry   *generic_proc_bt601_param = NULL;
static struct proc_dir_entry   *generic_proc_ch_param    = NULL;

typedef enum {
    GENERIC_CH_PARAM_ID_ALL = 0,
    GENERIC_CH_PARAM_ID_NORM,
    GENERIC_CH_PARAM_ID_PROG,
    GENERIC_CH_PARAM_ID_SPEED,
    GENERIC_CH_PARAM_ID_FPS,
    GENERIC_CH_PARAM_ID_TIMEOUT,
    GENERIC_CH_PARAM_ID_MAX
} GENERIC_CH_PARAM_ID_T;

/* generic channel parameter definition */
static struct vcap_input_ch_param_t generic_ch_param_data[VCAP_INPUT_NORM_MAX] = {
    /* mode                     width  height  prog  fps  time  speed */
    {VCAP_INPUT_NORM_PAL,        720,    576,    0,  25,   80,  VCAP_INPUT_SPEED_I },
    {VCAP_INPUT_NORM_NTSC,       720,    480,    0,  30,   66,  VCAP_INPUT_SPEED_I },
    {VCAP_INPUT_NORM_PAL_960H,   960,    576,    0,  25,   80,  VCAP_INPUT_SPEED_I },
    {VCAP_INPUT_NORM_NTSC_960H,  960,    480,    0,  30,   66,  VCAP_INPUT_SPEED_I },
    {VCAP_INPUT_NORM_720P25,    1280,    720,    1,  25,   80,  VCAP_INPUT_SPEED_P },
    {VCAP_INPUT_NORM_720P30,    1280,    720,    1,  30,   66,  VCAP_INPUT_SPEED_P },
    {VCAP_INPUT_NORM_720P50,    1280,    720,    1,  50,   40,  VCAP_INPUT_SPEED_2P},
    {VCAP_INPUT_NORM_720P60,    1280,    720,    1,  60,   40,  VCAP_INPUT_SPEED_2P},
    {VCAP_INPUT_NORM_1080P25,   1920,   1080,    1,  25,   80,  VCAP_INPUT_SPEED_P },
    {VCAP_INPUT_NORM_1080P30,   1920,   1080,    1,  30,   66,  VCAP_INPUT_SPEED_P },
    {VCAP_INPUT_NORM_1080P50,   1920,   1080,    1,  50,   40,  VCAP_INPUT_SPEED_2P},
    {VCAP_INPUT_NORM_1080P60,   1920,   1080,    1,  60,   40,  VCAP_INPUT_SPEED_2P}
};

static int generic_module_get(void)
{
    return try_module_get(THIS_MODULE);
}

static void generic_module_put(void)
{
    module_put(THIS_MODULE);
}

static int generic_device_init_ch_param(int id)
{
    int i;

    if(generic_dev == NULL)
        return -1;

    /*
     * for 2ch dual edge hybrid mode need to init each channel parameter.
     * for other modes all channel parameter will be reference vi setting.
     */
    if(generic_dev->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID) {
        for(i=0; i<VCAP_INPUT_DEV_CH_MAX; i++) {
            if((i%2) != 0)
                continue;

            if((ch_norm[i] >= VCAP_INPUT_NORM_PAL) && (ch_norm[i] < VCAP_INPUT_NORM_MAX)) {
                generic_dev->ch_param[i].width      = generic_ch_param_data[ch_norm[i]].width;
                generic_dev->ch_param[i].height     = generic_ch_param_data[ch_norm[i]].height;
                generic_dev->ch_param[i].prog       = generic_ch_param_data[ch_norm[i]].prog;
                generic_dev->ch_param[i].frame_rate = generic_ch_param_data[ch_norm[i]].frame_rate;
                generic_dev->ch_param[i].timeout_ms = generic_ch_param_data[ch_norm[i]].timeout_ms;
                generic_dev->ch_param[i].speed      = generic_ch_param_data[ch_norm[i]].speed;
                generic_dev->ch_param[i].mode       = generic_ch_param_data[ch_norm[i]].mode;
            }
            else {  ///< channel parameter base on device vi setting
                if((generic_dev->interface == VCAP_INPUT_INTF_BT656_INTERLACE)      ||
                   (generic_dev->interface == VCAP_INPUT_INTF_BT1120_INTERLACE)     ||
                   (generic_dev->interface == VCAP_INPUT_INTF_SDI8BIT_INTERLACE)    ||
                   (generic_dev->interface == VCAP_INPUT_INTF_BT601_8BIT_INTERLACE) ||
                   (generic_dev->interface == VCAP_INPUT_INTF_BT601_16BIT_INTERLACE)) {
                    generic_dev->ch_param[i].prog = 0;
                }
                else
                    generic_dev->ch_param[i].prog = 1;

                generic_dev->ch_param[i].width      = generic_dev->norm.width;
                generic_dev->ch_param[i].height     = generic_dev->norm.height;
                generic_dev->ch_param[i].frame_rate = generic_dev->frame_rate;
                generic_dev->ch_param[i].timeout_ms = generic_dev->timeout_ms;
                generic_dev->ch_param[i].speed      = generic_dev->speed;
                generic_dev->ch_param[i].mode       = generic_dev->norm.mode;
            }
        }
    }

    return 0;
}

static int generic_device_init(int id, int norm)
{
    if(generic_dev == NULL)
        return -1;

    if(generic_dev->norm.mode != norm) {
        /* VI source resolution */
        generic_dev->norm.width  = width;
        generic_dev->norm.height = height;

        /* VI Interface */
        if(generic_dev->interface != interface) {
            switch(interface) {
                case VCAP_INPUT_INTF_BT656_INTERLACE:
                case VCAP_INPUT_INTF_BT1120_INTERLACE:
                case VCAP_INPUT_INTF_SDI8BIT_INTERLACE:
                case VCAP_INPUT_INTF_BT601_8BIT_INTERLACE:
                case VCAP_INPUT_INTF_BT601_16BIT_INTERLACE:
                    if((order == VCAP_INPUT_ORDER_ODD_FIRST) || (order == VCAP_INPUT_ORDER_EVEN_FIRST))
                        generic_dev->field_order = order;
                    else
                        generic_dev->field_order = VCAP_INPUT_ORDER_ANYONE;
                    break;
                default:
                    generic_dev->field_order = VCAP_INPUT_ORDER_ANYONE;
                    break;
            }
            generic_dev->interface = interface;
        }

        /* RGB888 Parameter */
        if(generic_dev->interface == VCAP_INPUT_INTF_RGB888) {
            generic_dev->rgb.vs_pol    = rgb_param[0];
            generic_dev->rgb.hs_pol    = rgb_param[1];
            generic_dev->rgb.de_pol    = rgb_param[2];
            generic_dev->rgb.watch_de  = rgb_param[3];
            generic_dev->rgb.sd_hratio = rgb_param[4];
        }

        /* BT601 Parameter */
        if((generic_dev->interface >= VCAP_INPUT_INTF_BT601_8BIT_INTERLACE) &&
           (generic_dev->interface <= VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE)) {
            generic_dev->bt601.vs_pol    = bt601_param[0];
            generic_dev->bt601.hs_pol    = bt601_param[1];
            generic_dev->bt601.sync_mode = bt601_param[2];
            generic_dev->bt601.x_offset  = bt601_param[3];
            generic_dev->bt601.y_offset  = bt601_param[4];
        }

        /* Update norm */
        generic_dev->norm.mode = generic_norm = norm;
    }

    return 0;
}

static int generic_proc_norm_show(struct seq_file *sfile, void *v)
{
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    down(&pdev->sema_lock);

    seq_printf(sfile, "Norm: %d x %d\n", width, height);

    up(&pdev->sema_lock);

    return 0;
}

static int generic_proc_interface_show(struct seq_file *sfile, void *v)
{
    int i;
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;
    char *intf_msg[] = {"BT656_INTERLACE",       "BT656_PROGRESSIVE",
                        "BT1120_INTERLACE",      "BT1120_PROGRESSIVE",
                        "RGB888",
                        "SDI8BIT_INTERLACE",     "SDI8BIT_PROGRESSIVE",
                        "BT601_8BIT_INTERLACE",  "BT601_8BIT_PROGRESSIVE",
                        "BT601_16BIT_INTERLACE", "BT601_16BIT_PROGRESSIVE",
                        "ISP"};

    down(&pdev->sema_lock);

    for(i=0; i<VCAP_INPUT_INTF_MAX; i++) {
        seq_printf(sfile, "[%02d]: %s\n", i, intf_msg[i]);
    }
    seq_printf(sfile, "===============================\n");
    seq_printf(sfile, "Current: %s\n", intf_msg[interface]);

    up(&pdev->sema_lock);

    return 0;
}

static int generic_proc_framerate_show(struct seq_file *sfile, void *v)
{
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    down(&pdev->sema_lock);

    seq_printf(sfile, "Source Frame Rate: %d FPS\n", frame_rate);

    up(&pdev->sema_lock);

    return 0;
}

static int generic_proc_speed_show(struct seq_file *sfile, void *v)
{
    int i;
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;
    char *speed_msg[] = {"Interlace   => as 50/60I",
                         "Progressive => as 25/30P",
                         "Progressive => as 50/60P"};

    down(&pdev->sema_lock);

    seq_printf(sfile, "Source Speed Type: %d\n", speed);
    for(i=0; i<VCAP_INPUT_SPEED_MAX; i++)
        seq_printf(sfile, "[%02d]: %s\n", i, speed_msg[i]);

    seq_printf(sfile, "\n");

    up(&pdev->sema_lock);

    return 0;
}

static int generic_proc_timeout_show(struct seq_file *sfile, void *v)
{
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    down(&pdev->sema_lock);

    seq_printf(sfile, "Timeout Threshold: %d (ms)\n", timeout);

    up(&pdev->sema_lock);

    return 0;
}

static int generic_proc_rgb_param_show(struct seq_file *sfile, void *v)
{
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    down(&pdev->sema_lock);

    seq_printf(sfile, "VS_POL  : %d\n", rgb_param[0]);
    seq_printf(sfile, "HS_POL  : %d\n", rgb_param[1]);
    seq_printf(sfile, "DE_POL  : %d\n", rgb_param[2]);
    seq_printf(sfile, "WATCH_DE: %d\n", rgb_param[3]);
    seq_printf(sfile, "H_RATIO : %d\n", rgb_param[4]);

    up(&pdev->sema_lock);

    return 0;
}

static int generic_proc_bt601_param_show(struct seq_file *sfile, void *v)
{
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    down(&pdev->sema_lock);

    seq_printf(sfile, "VS_POL   : %d\n", bt601_param[0]);
    seq_printf(sfile, "HS_POL   : %d\n", bt601_param[1]);
    seq_printf(sfile, "SYNC_MODE: %d\n", bt601_param[2]);
    seq_printf(sfile, "X_OFFSET : %d\n", bt601_param[3]);
    seq_printf(sfile, "Y_OFFSET : %d\n", bt601_param[4]);

    up(&pdev->sema_lock);

    return 0;
}

static int generic_proc_ch_param_show(struct seq_file *sfile, void *v)
{
    int i;
    int ch_active;
    int prog, src_w, src_h, speed, frame_rate, timeout_ms;
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;
    char *speed_msg[] = {"50/60I", "25/30P", "50/60P"};

    down(&pdev->sema_lock);

    for(i=0; i<VCAP_INPUT_DEV_CH_MAX; i++) {
        ch_active = 0;
        switch(pdev->mode) {
            case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID:
                if(i%2 == 0) {
                    prog       = pdev->ch_param[i].prog;
                    src_w      = pdev->ch_param[i].width;
                    src_h      = pdev->ch_param[i].height;
                    speed      = pdev->ch_param[i].speed;
                    frame_rate = pdev->ch_param[i].frame_rate;
                    timeout_ms = pdev->ch_param[i].timeout_ms;
                    ch_active  = 1;
                }
                break;
            default:    ///< channel parameter reference device vi setting
                if(((pdev->mode == VCAP_INPUT_MODE_BYPASS) && (i == 0))                ||
                   ((pdev->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE) && (i%2 == 0)) ||
                   (pdev->mode == VCAP_INPUT_MODE_FRAME_INTERLEAVE)                    ||
                   (pdev->mode == VCAP_INPUT_MODE_4CH_BYTE_INTERLEAVE)) {
                    if((pdev->interface == VCAP_INPUT_INTF_BT656_INTERLACE)      ||
                       (pdev->interface == VCAP_INPUT_INTF_BT1120_INTERLACE)     ||
                       (pdev->interface == VCAP_INPUT_INTF_SDI8BIT_INTERLACE)    ||
                       (pdev->interface == VCAP_INPUT_INTF_BT601_8BIT_INTERLACE) ||
                       (pdev->interface == VCAP_INPUT_INTF_BT601_16BIT_INTERLACE)) {
                        prog = 0;
                    }
                    else {
                        prog = 1;
                    }

                    src_w      = pdev->norm.width;
                    src_h      = pdev->norm.height;
                    speed      = pdev->speed;
                    frame_rate = pdev->frame_rate;
                    timeout_ms = pdev->timeout_ms;
                    ch_active  = 1;
                }
                break;
        }

        if(ch_active) {
            seq_printf(sfile, "========================== [CH#%d] =====================================\n", i);
            seq_printf(sfile, "[%02d]Norm      : %d x %d\n", GENERIC_CH_PARAM_ID_NORM, src_w, src_h);
            seq_printf(sfile, "[%02d]Prog      : %s \t[0:Interlace 1:Progressive]\n", GENERIC_CH_PARAM_ID_PROG, ((prog == 0) ? "Interlace" : "Progressive"));
            seq_printf(sfile, "[%02d]Speed     : %s \t\t[0:50/60I 1:25/30P 2:50/60P]\n", GENERIC_CH_PARAM_ID_SPEED, ((speed >= VCAP_INPUT_SPEED_MAX) ? "Unknown" : speed_msg[speed]));
            seq_printf(sfile, "[%02d]Frame_Rate: %d\n", GENERIC_CH_PARAM_ID_FPS, frame_rate);
            seq_printf(sfile, "[%02d]Timeout   : %d(ms)\n", GENERIC_CH_PARAM_ID_TIMEOUT, timeout_ms);
        }
    }
    seq_printf(sfile, "-----------------------------------------------------------------------\n");
    seq_printf(sfile, "Echo [ch#] [param#] [value#0] [value#1] to setup parameter\n");
    seq_printf(sfile, "Param#0 for swicth all parameter to specify channel norm type\n");
    seq_printf(sfile, "Norm_Type:\n");
    seq_printf(sfile, "%2d: 720x576I@25   %2d: 720x480I@30   %2d: 960x576I@25   %2d: 960x480I@30\n",
               VCAP_INPUT_NORM_PAL, VCAP_INPUT_NORM_NTSC, VCAP_INPUT_NORM_PAL_960H, VCAP_INPUT_NORM_NTSC_960H);
    seq_printf(sfile, "%2d: 1280x720P@25  %2d: 1280x720P@30  %2d: 1280x720P@50  %2d: 1280x720P@60\n",
               VCAP_INPUT_NORM_720P25, VCAP_INPUT_NORM_720P30, VCAP_INPUT_NORM_720P50, VCAP_INPUT_NORM_720P60);
    seq_printf(sfile, "%2d: 1920x1080P@25 %2d: 1920x1080P@30 %2d: 1920x1080P@50 %2d: 1920x1080P@60\n",
               VCAP_INPUT_NORM_1080P25, VCAP_INPUT_NORM_1080P30, VCAP_INPUT_NORM_1080P50, VCAP_INPUT_NORM_1080P60);

    up(&pdev->sema_lock);

    return 0;
}

static ssize_t generic_proc_norm_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, ret = 0;
    int  norm_w, norm_h;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &norm_w, &norm_h);

    down(&pdev->sema_lock);

    if(norm_w == 0) {   ///< for specify generic norm video format
        if((norm_h >= VCAP_INPUT_NORM_PAL) && (norm_h < VCAP_INPUT_NORM_MAX)) {
            int norm_switch = 0;
            int fps_switch  = 0;

            /* interface & field order */
            switch(pdev->interface) {
                case VCAP_INPUT_INTF_RGB888:
                case VCAP_INPUT_INTF_ISP:
                    if(norm_h <= VCAP_INPUT_NORM_NTSC_960H) ///< not support
                        goto exit;
                    break;
                case VCAP_INPUT_INTF_BT656_INTERLACE:
                    if(generic_ch_param_data[norm_h].prog) {
                        interface = pdev->interface = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                        pdev->field_order = VCAP_INPUT_ORDER_ANYONE;
                        norm_switch++;
                    }
                    break;
                case VCAP_INPUT_INTF_BT656_PROGRESSIVE:
                    if(!generic_ch_param_data[norm_h].prog) {
                        interface = pdev->interface = VCAP_INPUT_INTF_BT656_INTERLACE;
                        pdev->field_order = VCAP_INPUT_ORDER_ODD_FIRST;
                        norm_switch++;
                    }
                    break;
                case VCAP_INPUT_INTF_SDI8BIT_INTERLACE:
                    if(generic_ch_param_data[norm_h].prog) {
                        interface = pdev->interface = VCAP_INPUT_INTF_SDI8BIT_PROGRESSIVE;
                        pdev->field_order = VCAP_INPUT_ORDER_ANYONE;
                        norm_switch++;
                    }
                    break;
                case VCAP_INPUT_INTF_SDI8BIT_PROGRESSIVE:
                    if(!generic_ch_param_data[norm_h].prog) {
                        interface = pdev->interface = VCAP_INPUT_INTF_SDI8BIT_INTERLACE;
                        pdev->field_order = VCAP_INPUT_ORDER_ODD_FIRST;
                        norm_switch++;
                    }
                    break;
                case VCAP_INPUT_INTF_BT1120_INTERLACE:
                    if(generic_ch_param_data[norm_h].prog) {
                        interface = pdev->interface = VCAP_INPUT_INTF_BT1120_PROGRESSIVE;
                        pdev->field_order = VCAP_INPUT_ORDER_ANYONE;
                        norm_switch++;
                    }
                    break;
                case VCAP_INPUT_INTF_BT1120_PROGRESSIVE:
                    if(!generic_ch_param_data[norm_h].prog) {
                        interface = pdev->interface = VCAP_INPUT_INTF_BT1120_INTERLACE;
                        pdev->field_order = VCAP_INPUT_ORDER_ODD_FIRST;
                        norm_switch++;
                    }
                    break;
                case VCAP_INPUT_INTF_BT601_8BIT_INTERLACE:
                    if(generic_ch_param_data[norm_h].prog) {
                        interface = pdev->interface = VCAP_INPUT_INTF_BT601_8BIT_PROGRESSIVE;
                        pdev->field_order = VCAP_INPUT_ORDER_ANYONE;
                        norm_switch++;
                    }
                    break;
                case VCAP_INPUT_INTF_BT601_8BIT_PROGRESSIVE:
                    if(!generic_ch_param_data[norm_h].prog) {
                        interface = pdev->interface = VCAP_INPUT_INTF_BT601_8BIT_INTERLACE;
                        pdev->field_order = VCAP_INPUT_ORDER_ODD_FIRST;
                        norm_switch++;
                    }
                    break;
                case VCAP_INPUT_INTF_BT601_16BIT_INTERLACE:
                    if(generic_ch_param_data[norm_h].prog) {
                        interface = pdev->interface = VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE;
                        pdev->field_order = VCAP_INPUT_ORDER_ANYONE;
                        norm_switch++;
                    }
                    break;
                case VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE:
                    if(!generic_ch_param_data[norm_h].prog) {
                        interface = pdev->interface = VCAP_INPUT_INTF_BT601_16BIT_INTERLACE;
                        pdev->field_order = VCAP_INPUT_ORDER_ODD_FIRST;
                        norm_switch++;
                    }
                    break;
                default:
                    break;
            }

            /* width */
            if(pdev->norm.width != generic_ch_param_data[norm_h].width) {
                width = pdev->norm.width = generic_ch_param_data[norm_h].width;
                norm_switch++;
            }

            /* height */
            if(pdev->norm.height != generic_ch_param_data[norm_h].height) {
                height = pdev->norm.height = generic_ch_param_data[norm_h].height;
                norm_switch++;
            }

            /* frame rate */
            if(pdev->frame_rate != generic_ch_param_data[norm_h].frame_rate) {
                frame_rate = pdev->frame_rate = pdev->max_frame_rate = generic_ch_param_data[norm_h].frame_rate;
                fps_switch++;
            }

            /* timeout */
            if(pdev->timeout_ms != generic_ch_param_data[norm_h].timeout_ms) {
                timeout = pdev->timeout_ms = generic_ch_param_data[norm_h].timeout_ms;
            }

            /* speed */
            if(pdev->speed != generic_ch_param_data[norm_h].speed) {
                speed = pdev->speed = generic_ch_param_data[norm_h].speed;
            }

            /* Update norm */
            if(norm_switch)
                pdev->norm.mode++;

            /* issue notify */
            switch(pdev->mode) {
                case VCAP_INPUT_MODE_BYPASS:
                    if(norm_switch)
                        vcap_input_device_notify(pdev->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);

                    if(fps_switch)
                        vcap_input_device_notify(pdev->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                    break;
                case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE:
                    if(norm_switch) {
                        vcap_input_device_notify(pdev->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                        vcap_input_device_notify(pdev->vi_idx, 2, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                    }

                    if(fps_switch) {
                        vcap_input_device_notify(pdev->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                        vcap_input_device_notify(pdev->vi_idx, 2, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                    }
                    break;
                case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID:
                    break;
                default:
                    for(i=0; i<VCAP_INPUT_DEV_CH_MAX; i++) {
                        if(norm_switch)
                            vcap_input_device_notify(pdev->vi_idx, i, VCAP_INPUT_NOTIFY_NORM_CHANGE);

                        if(fps_switch)
                            vcap_input_device_notify(pdev->vi_idx, i, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                    }
                    break;
            }
        }
    }
    else {
        if((norm_w <= 0) || (norm_h <= 0))
            goto exit;

        if(norm_w != width || norm_h != height) {
            width  = norm_w;
            height = norm_h;
            generic_device_init(pdev->index, (generic_norm+1));  ///< update norm value to inform driver to switch VI source

            /* issue notify */
            switch(pdev->mode) {
                case VCAP_INPUT_MODE_BYPASS:
                    vcap_input_device_notify(pdev->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                    break;
                case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE:
                    vcap_input_device_notify(pdev->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                    vcap_input_device_notify(pdev->vi_idx, 2, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                    break;
                case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID:
                    break;
                default:
                    for(i=0; i<VCAP_INPUT_DEV_CH_MAX; i++)
                        vcap_input_device_notify(pdev->vi_idx, i, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                    break;
            }
        }
    }

exit:
    up(&pdev->sema_lock);

    if(ret)
        return ret;

    return count;
}

static ssize_t generic_proc_interface_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, ret = 0;
    int  intf_value;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &intf_value);

    down(&pdev->sema_lock);

    if((intf_value != interface) && ((intf_value >= VCAP_INPUT_INTF_BT656_INTERLACE) && (intf_value < VCAP_INPUT_INTF_MAX))) {
        interface = intf_value;
        generic_device_init(pdev->index, (generic_norm+1));  ///< update norm value to inform driver to switch VI source

        /* issue notify */
        switch(pdev->mode) {
            case VCAP_INPUT_MODE_BYPASS:
                vcap_input_device_notify(pdev->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                break;
            case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE:
                vcap_input_device_notify(pdev->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                vcap_input_device_notify(pdev->vi_idx, 2, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                break;
            case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID:
                break;
            default:
                for(i=0; i<VCAP_INPUT_DEV_CH_MAX; i++)
                    vcap_input_device_notify(pdev->vi_idx, i, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                break;
        }
    }

    up(&pdev->sema_lock);

    if(ret)
        return ret;

    return count;
}

static ssize_t generic_proc_framerate_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, ret = 0;
    int  rate;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &rate);

    down(&pdev->sema_lock);

    if((rate >= 0) && (rate != frame_rate)) {
        frame_rate = pdev->frame_rate = pdev->max_frame_rate = rate;

        /* issue notify */
        switch(pdev->mode) {
            case VCAP_INPUT_MODE_BYPASS:
                vcap_input_device_notify(pdev->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                break;
            case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE:
                vcap_input_device_notify(pdev->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                vcap_input_device_notify(pdev->vi_idx, 2, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                break;
            case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID:
                break;
            default:
                for(i=0; i<VCAP_INPUT_DEV_CH_MAX; i++)
                    vcap_input_device_notify(pdev->vi_idx, i, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                break;
        }
    }

    up(&pdev->sema_lock);

    if(ret)
        return ret;

    return count;
}

static ssize_t generic_proc_speed_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ret = 0;
    int  speed_value;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &speed_value);

    down(&pdev->sema_lock);

    if((speed_value != speed) && ((speed_value >= 0) && (speed_value < VCAP_INPUT_SPEED_MAX))) {
        speed = pdev->speed = speed_value;
    }

    up(&pdev->sema_lock);

    if(ret)
        return ret;

    return count;
}

static ssize_t generic_proc_timeout_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ret = 0;
    int  time_ms;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &time_ms);

    down(&pdev->sema_lock);

    if((time_ms > 0) && (time_ms != timeout)) {
        timeout = pdev->timeout_ms = time_ms;
    }

    up(&pdev->sema_lock);

    if(ret)
        return ret;

    return count;
}

static ssize_t generic_proc_rgb_param_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ret = 0;
    int  param_value[5];
    char value_str[64] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %d %d %d\n", &param_value[0], &param_value[1], &param_value[2], &param_value[3], &param_value[4]);

    down(&pdev->sema_lock);

    if((param_value[0] != rgb_param[0]) || (param_value[1] != rgb_param[1]) ||
       (param_value[2] != rgb_param[2]) || (param_value[3] != rgb_param[3]) || (param_value[4] != rgb_param[4])) {
        rgb_param[0] = param_value[0];
        rgb_param[1] = param_value[1];
        rgb_param[2] = param_value[2];
        rgb_param[3] = param_value[3];
        rgb_param[4] = param_value[4];

        if(pdev->interface == VCAP_INPUT_INTF_RGB888)
            generic_device_init(pdev->index, (generic_norm+1));  ///< update norm value to inform driver to switch VI source
    }

    up(&pdev->sema_lock);

    if(ret)
        return ret;

    return count;
}

static ssize_t generic_proc_bt601_param_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ret = 0;
    int  param_value[5];
    char value_str[64] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %d %d %d\n", &param_value[0], &param_value[1], &param_value[2], &param_value[3], &param_value[4]);

    down(&pdev->sema_lock);

    if((param_value[0] != bt601_param[0]) || (param_value[1] != bt601_param[1]) ||
       (param_value[2] != bt601_param[2]) || (param_value[3] != bt601_param[3]) || (param_value[4] != bt601_param[4])) {
        bt601_param[0] = param_value[0];
        bt601_param[1] = param_value[1];
        bt601_param[2] = param_value[2];
        bt601_param[3] = param_value[3];
        bt601_param[4] = param_value[4];

        if((pdev->interface >= VCAP_INPUT_INTF_BT601_8BIT_INTERLACE) && (pdev->interface <= VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE))
            generic_device_init(pdev->index, (generic_norm+1));  ///< update norm value to inform driver to switch VI source
    }

    up(&pdev->sema_lock);

    if(ret)
        return ret;

    return count;
}

static ssize_t generic_proc_ch_param_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ret = 0;
    int  norm_switch = 0;
    int  fps_switch  = 0;
    int  ch, param_id, param_value[2];
    char value_str[64] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_input_dev_t *pdev = (struct vcap_input_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count)) {
        return -EFAULT;
    }

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %d %d\n", &ch, &param_id, &param_value[0], &param_value[1]);

    if(ch >= VCAP_INPUT_DEV_CH_MAX || param_id >= GENERIC_CH_PARAM_ID_MAX)
        return -EFAULT;

    if(pdev->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID) {
        if((ch%2) != 0)   ///< only device ch0 and ch2
            goto exit;

        switch(param_id) {
            case GENERIC_CH_PARAM_ID_ALL:
                {
                    int src_w, src_h, fps, speed, prog, timeout;

                    if((param_value[0] >= VCAP_INPUT_NORM_PAL) && (param_value[0] < VCAP_INPUT_NORM_MAX)) {
                        src_w   = generic_ch_param_data[param_value[0]].width;
                        src_h   = generic_ch_param_data[param_value[0]].height;
                        fps     = generic_ch_param_data[param_value[0]].frame_rate;
                        speed   = generic_ch_param_data[param_value[0]].speed;
                        prog    = generic_ch_param_data[param_value[0]].prog;
                        timeout = generic_ch_param_data[param_value[0]].timeout_ms;
                    }
                    else
                        goto exit;

                    if(src_w != pdev->ch_param[ch].width) {
                        pdev->ch_param[ch].width = src_w;
                        norm_switch++;
                    }

                    if(src_h != pdev->ch_param[ch].height) {
                        pdev->ch_param[ch].height = src_h;
                        norm_switch++;
                    }

                    if(prog != pdev->ch_param[ch].prog) {
                        pdev->ch_param[ch].prog = prog;
                        norm_switch++;
                    }

                    if(speed != pdev->ch_param[ch].speed) {
                        pdev->ch_param[ch].speed = speed;
                    }

                    if(fps != pdev->ch_param[ch].frame_rate) {
                        pdev->ch_param[ch].frame_rate = fps;
                        fps_switch++;
                    }

                    if(timeout != pdev->ch_param[ch].timeout_ms) {
                        pdev->ch_param[ch].timeout_ms = timeout;
                    }
                }
                break;
            case GENERIC_CH_PARAM_ID_NORM:
                if((param_value[0] > 0) && (param_value[0] != pdev->ch_param[ch].width)) {
                    pdev->ch_param[ch].width = param_value[0];
                    norm_switch++;
                }
                if((param_value[1] > 0) && (param_value[1] != pdev->ch_param[ch].height)) {
                    pdev->ch_param[ch].height = param_value[1];
                    norm_switch++;
                }
                break;
            case GENERIC_CH_PARAM_ID_PROG:
                if((param_value[0] >= 0 && param_value[0] <= 1) && (param_value[0] != pdev->ch_param[ch].prog)) {
                    pdev->ch_param[ch].prog = param_value[0];
                    norm_switch++;
                }
                break;
            case GENERIC_CH_PARAM_ID_SPEED:
                if((param_value[0] >= VCAP_INPUT_SPEED_I && param_value[0] < VCAP_INPUT_SPEED_MAX) && (param_value[0] != pdev->ch_param[ch].speed)) {
                    pdev->ch_param[ch].speed = param_value[0];
                }
                break;
            case GENERIC_CH_PARAM_ID_FPS:
                if((param_value[0] > 0) && (param_value[0] != pdev->ch_param[ch].frame_rate)) {
                    pdev->ch_param[ch].frame_rate = param_value[0];
                    fps_switch++;
                }
                break;
            case GENERIC_CH_PARAM_ID_TIMEOUT:
                if((param_value[0] > 0) && (param_value[0] != pdev->ch_param[ch].timeout_ms)) {
                    pdev->ch_param[ch].timeout_ms = param_value[0];
                }
                break;
            default:
                goto exit;
        }

        if(norm_switch) {
            pdev->ch_param[ch].mode++; ///< update mode value to inform driver to switch VI channel config
            vcap_input_device_notify(pdev->vi_idx, ch, VCAP_INPUT_NOTIFY_NORM_CHANGE);
        }

        if(fps_switch) {
            vcap_input_device_notify(pdev->vi_idx, ch, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
        }
    }

exit:
    up(&pdev->sema_lock);

    if(ret)
        return ret;

    return count;
}

static int generic_proc_norm_open(struct inode *inode, struct file *file)
{
    return single_open(file, generic_proc_norm_show, PDE(inode)->data);
}

static int generic_proc_interface_open(struct inode *inode, struct file *file)
{
    return single_open(file, generic_proc_interface_show, PDE(inode)->data);
}

static int generic_proc_framerate_open(struct inode *inode, struct file *file)
{
    return single_open(file, generic_proc_framerate_show, PDE(inode)->data);
}

static int generic_proc_speed_open(struct inode *inode, struct file *file)
{
    return single_open(file, generic_proc_speed_show, PDE(inode)->data);
}

static int generic_proc_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, generic_proc_timeout_show, PDE(inode)->data);
}

static int generic_proc_rgb_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, generic_proc_rgb_param_show, PDE(inode)->data);
}

static int generic_proc_bt601_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, generic_proc_bt601_param_show, PDE(inode)->data);
}

static int generic_proc_ch_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, generic_proc_ch_param_show, PDE(inode)->data);
}

static struct file_operations generic_proc_norm_ops = {
    .owner  = THIS_MODULE,
    .open   = generic_proc_norm_open,
    .write  = generic_proc_norm_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations generic_proc_interface_ops = {
    .owner  = THIS_MODULE,
    .open   = generic_proc_interface_open,
    .write  = generic_proc_interface_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations generic_proc_framerate_ops = {
    .owner  = THIS_MODULE,
    .open   = generic_proc_framerate_open,
    .write  = generic_proc_framerate_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations generic_proc_speed_ops = {
    .owner  = THIS_MODULE,
    .open   = generic_proc_speed_open,
    .write  = generic_proc_speed_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations generic_proc_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = generic_proc_timeout_open,
    .write  = generic_proc_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations generic_proc_rgb_param_ops = {
    .owner  = THIS_MODULE,
    .open   = generic_proc_rgb_param_open,
    .write  = generic_proc_rgb_param_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations generic_proc_bt601_param_ops = {
    .owner  = THIS_MODULE,
    .open   = generic_proc_bt601_param_open,
    .write  = generic_proc_bt601_param_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations generic_proc_ch_param_ops = {
    .owner  = THIS_MODULE,
    .open   = generic_proc_ch_param_open,
    .write  = generic_proc_ch_param_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void generic_proc_remove(void)
{
    if(generic_proc_ch_param)
        vcap_input_proc_remove_entry(generic_proc_root, generic_proc_ch_param);

    if(generic_proc_bt601_param)
        vcap_input_proc_remove_entry(generic_proc_root, generic_proc_bt601_param);

    if(generic_proc_rgb_param)
        vcap_input_proc_remove_entry(generic_proc_root, generic_proc_rgb_param);

    if(generic_proc_timeout)
        vcap_input_proc_remove_entry(generic_proc_root, generic_proc_timeout);

    if(generic_proc_speed)
        vcap_input_proc_remove_entry(generic_proc_root, generic_proc_speed);

    if(generic_proc_framerate)
        vcap_input_proc_remove_entry(generic_proc_root, generic_proc_framerate);

    if(generic_proc_interface)
        vcap_input_proc_remove_entry(generic_proc_root, generic_proc_interface);

    if(generic_proc_norm)
        vcap_input_proc_remove_entry(generic_proc_root, generic_proc_norm);

    if(generic_proc_root)
        vcap_input_proc_remove_entry(NULL, generic_proc_root);
}

static int generic_proc_init(struct vcap_input_dev_t *pdev)
{
    int ret = 0;

    /* root */
    generic_proc_root = vcap_input_proc_create_entry(pdev->name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!generic_proc_root) {
        vcap_err("create proc node '%s' failed!\n", pdev->name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    generic_proc_root->owner = THIS_MODULE;
#endif

    /* norm */
    generic_proc_norm = vcap_input_proc_create_entry("norm", S_IRUGO|S_IXUGO, generic_proc_root);
    if(!generic_proc_norm) {
        vcap_err("create proc node '%s/norm' failed!\n", pdev->name);
        ret = -EINVAL;
        goto err;
    }
    generic_proc_norm->proc_fops = &generic_proc_norm_ops;
    generic_proc_norm->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    generic_proc_norm->owner     = THIS_MODULE;
#endif

    /* interface */
    generic_proc_interface = vcap_input_proc_create_entry("interface", S_IRUGO|S_IXUGO, generic_proc_root);
    if(!generic_proc_interface) {
        vcap_err("create proc node '%s/interface' failed!\n", pdev->name);
        ret = -EINVAL;
        goto err;
    }
    generic_proc_interface->proc_fops = &generic_proc_interface_ops;
    generic_proc_interface->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    generic_proc_interface->owner     = THIS_MODULE;
#endif

    /* framerate */
    generic_proc_framerate = vcap_input_proc_create_entry("frame_rate", S_IRUGO|S_IXUGO, generic_proc_root);
    if(!generic_proc_framerate) {
        vcap_err("create proc node '%s/frame_rate' failed!\n", pdev->name);
        ret = -EINVAL;
        goto err;
    }
    generic_proc_framerate->proc_fops = &generic_proc_framerate_ops;
    generic_proc_framerate->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    generic_proc_framerate->owner     = THIS_MODULE;
#endif

    /* speed */
    generic_proc_speed = vcap_input_proc_create_entry("speed", S_IRUGO|S_IXUGO, generic_proc_root);
    if(!generic_proc_speed) {
        vcap_err("create proc node '%s/speed' failed!\n", pdev->name);
        ret = -EINVAL;
        goto err;
    }
    generic_proc_speed->proc_fops = &generic_proc_speed_ops;
    generic_proc_speed->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    generic_proc_speed->owner     = THIS_MODULE;
#endif

    /* timeout */
    generic_proc_timeout = vcap_input_proc_create_entry("timeout", S_IRUGO|S_IXUGO, generic_proc_root);
    if(!generic_proc_timeout) {
        vcap_err("create proc node '%s/timeout' failed!\n", pdev->name);
        ret = -EINVAL;
        goto err;
    }
    generic_proc_timeout->proc_fops = &generic_proc_timeout_ops;
    generic_proc_timeout->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    generic_proc_timeout->owner     = THIS_MODULE;
#endif

    /* RGB Param */
    generic_proc_rgb_param = vcap_input_proc_create_entry("rgb_param", S_IRUGO|S_IXUGO, generic_proc_root);
    if(!generic_proc_rgb_param) {
        vcap_err("create proc node '%s/rgb_param' failed!\n", pdev->name);
        ret = -EINVAL;
        goto err;
    }
    generic_proc_rgb_param->proc_fops = &generic_proc_rgb_param_ops;
    generic_proc_rgb_param->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    generic_proc_rgb_param->owner     = THIS_MODULE;
#endif

    /* BT601 Param */
    generic_proc_bt601_param = vcap_input_proc_create_entry("bt601_param", S_IRUGO|S_IXUGO, generic_proc_root);
    if(!generic_proc_bt601_param) {
        vcap_err("create proc node '%s/bt601_param' failed!\n", pdev->name);
        ret = -EINVAL;
        goto err;
    }
    generic_proc_bt601_param->proc_fops = &generic_proc_bt601_param_ops;
    generic_proc_bt601_param->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    generic_proc_bt601_param->owner     = THIS_MODULE;
#endif

    /* Channel Param */
    generic_proc_ch_param = vcap_input_proc_create_entry("ch_param", S_IRUGO|S_IXUGO, generic_proc_root);
    if(!generic_proc_ch_param) {
        vcap_err("create proc node '%s/ch_param' failed!\n", pdev->name);
        ret = -EINVAL;
        goto err;
    }
    generic_proc_ch_param->proc_fops = &generic_proc_ch_param_ops;
    generic_proc_ch_param->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    generic_proc_ch_param->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    generic_proc_remove();
    return ret;
}

static int __init generic_input_init(void)
{
    int i, ret = 0;

    if(vcap_input_get_version() != VCAP_INPUT_VERSION) {
        vcap_err("Input driver version(%08x) is not compatibility with vcap300_common.ko(%08x)!\n",
                 vcap_input_get_version(), VCAP_INPUT_VERSION);
        return -EFAULT;
    }

    if(vi >= 0) {
        generic_dev = kzalloc(sizeof(struct vcap_input_dev_t), GFP_KERNEL);
        if(generic_dev == NULL) {
            vcap_err("generic allocate vcap_input_dev_t failed\n");
            ret = -ENOMEM;
            goto err;
        }

        /* device name */
        snprintf(generic_dev->name, VCAP_INPUT_NAME_SIZE-1, "generic.%d", GENERIC_ID);

        /* device parameter setup */
        generic_dev->index       = GENERIC_ID;
        generic_dev->vi_idx      = vi;
        generic_dev->vi_src      = (vi_src < 0) ? vi : vi_src;
        generic_dev->type        = DEFAULT_TYPE;
        generic_dev->speed       = speed;
        generic_dev->interface   = interface;
        generic_dev->mode        = mode;
        generic_dev->field_order = order;
        generic_dev->data_range  = range;
        generic_dev->yc_swap     = yc_swap;
        generic_dev->data_swap   = data_swap;
        generic_dev->inv_clk     = inv_clk;
        generic_dev->frame_rate  = generic_dev->max_frame_rate = frame_rate;
        generic_dev->timeout_ms  = timeout;
        generic_dev->probe_chid  = probe_chid;
        generic_dev->init        = generic_device_init;
        generic_dev->norm.mode   = -1;          ///< init value
        generic_dev->module_get  = generic_module_get;
        generic_dev->module_put  = generic_module_put;

        switch(interface) {
            case VCAP_INPUT_INTF_RGB888:
                generic_dev->rgb.vs_pol    = rgb_param[0];
                generic_dev->rgb.hs_pol    = rgb_param[1];
                generic_dev->rgb.de_pol    = rgb_param[2];
                generic_dev->rgb.watch_de  = rgb_param[3];
                generic_dev->rgb.sd_hratio = rgb_param[4];
                break;
            case VCAP_INPUT_INTF_BT601_8BIT_INTERLACE:
            case VCAP_INPUT_INTF_BT601_8BIT_PROGRESSIVE:
            case VCAP_INPUT_INTF_BT601_16BIT_INTERLACE:
            case VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE:
                generic_dev->bt601.vs_pol    = bt601_param[0];
                generic_dev->bt601.hs_pol    = bt601_param[1];
                generic_dev->bt601.sync_mode = bt601_param[2];
                generic_dev->bt601.x_offset  = bt601_param[3];
                generic_dev->bt601.y_offset  = bt601_param[4];
                break;
            default:
                break;
        }

        /* assign channel id */
        switch(mode) {
            case VCAP_INPUT_MODE_BYPASS:
                if(ch_id[0] < 0)
                    generic_dev->ch_id[0] = vi*VCAP_INPUT_DEV_CH_MAX;
                else
                    generic_dev->ch_id[0] = ch_id[0];
                break;

            case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE:
            case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID:
                for(i=0; i<2; i++) {
                    if(ch_id[i] < 0)
                        generic_dev->ch_id[i*2] = (vi*VCAP_INPUT_DEV_CH_MAX)+i;
                    else
                        generic_dev->ch_id[i*2] = ch_id[i];
                }
                break;
            default:
                for(i=0; i<VCAP_INPUT_DEV_CH_MAX; i++) {
                    if(ch_id[i] < 0)
                        generic_dev->ch_id[i] = (vi*VCAP_INPUT_DEV_CH_MAX)+i;
                    else
                        generic_dev->ch_id[i] = ch_id[i];
                }
                break;
        }

        /* init semaphore lock */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&generic_dev->sema_lock, 1);
#else
        init_MUTEX(&generic_dev->sema_lock);
#endif

        ret = generic_device_init(GENERIC_ID, generic_norm);
        if(ret < 0) {
            vcap_err("generic device %d init failed\n", GENERIC_ID);
            goto err;
        }

        ret = generic_device_init_ch_param(GENERIC_ID);
        if(ret < 0) {
            vcap_err("generic device %d init channel parameter failed\n", GENERIC_ID);
            goto err;
        }

        ret = vcap_input_device_register(generic_dev);
        if(ret < 0) {
            vcap_err("register vcap generic input device %d failed\n", GENERIC_ID);
            goto err;
        }

        ret = generic_proc_init(generic_dev);
        if(ret < 0)
            goto err;

        vcap_info("Register Generic Input Device %d\n", GENERIC_ID);
    }

    return ret;

err:
    if(generic_dev) {
        vcap_input_device_unregister(generic_dev);
        kfree(generic_dev);
    }
    return ret;
}

static void __exit generic_input_exit(void)
{
    if(generic_dev) {
        generic_proc_remove();
        vcap_input_device_unregister(generic_dev);
        kfree(generic_dev);
    }
}

module_init(generic_input_init);
module_exit(generic_input_exit);

MODULE_DESCRIPTION("Grain Media Video Capture300 Generic Input Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
