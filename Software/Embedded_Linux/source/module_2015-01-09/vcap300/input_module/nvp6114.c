/**
 * @file nvp6114.c
 * VCAP300 NVP6114 Input Device Driver
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.4 $
 * $Date: 2014/10/30 01:47:32 $
 *
 * ChangeLog:
 *  $Log: nvp6114.c,v $
 *  Revision 1.4  2014/10/30 01:47:32  jerson_l
 *  1. support vi 2ch dual edge hybrid channel mode.
 *  2. support grab channel horizontal blanking data.
 *
 *  Revision 1.3  2014/07/15 08:00:29  jerson_l
 *  1. modify mode module parameter help message
 *
 *  Revision 1.2  2014/06/05 02:24:58  jerson_l
 *  1. support 720H/960H/Hibrid video mode and dynamic norm switch
 *
 *  Revision 1.1  2014/04/22 08:31:57  jerson_l
 *  1. add nvp6114 decoder support
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
#include "nextchip/nvp6114.h"   ///< from module/include/front_end/ahd

#define DEV_MAX                 NVP6114_DEV_MAX
#define DEV_VPORT_MAX           NVP6114_DEV_VPORT_MAX                   ///< VPORT_A, VPORT_B
#define DEV_CH_MAX              NVP6114_DEV_CH_MAX
#define DEFAULT_TYPE            VCAP_INPUT_TYPE_AHD
#define DEFAULT_DATA_RANGE      VCAP_INPUT_DATA_RANGE_256
#define DEFAULT_CH_ID_MODE      VCAP_INPUT_CH_ID_EAV_SAV
#define DEFAULT_NTSC_TIMEOUT    66                                      ///< 33(ms)*2(frame), 2 frame delay time
#define DEFAULT_PAL_TIMEOUT     80                                      ///< 40(ms)*2(frame), 2 frame delay time

static ushort vport[DEV_MAX] = {0x0021, 0x0043, 0x0065, 0x0087};
module_param_array(vport, short, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vport, "VPort Link => VPortA[0:3], VPortB[3:7], VPortC[8:11], VPortD[12:15], 0:None, 1~8:X_CAP#0~7, 9:X_CAPCAS");

static int mode[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 1};
module_param_array(mode, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "Video Output Mode=> 0:1CH, 1:2CH, 2:4CH");

static int norm[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 5};
module_param_array(norm, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(norm, "Video Norm=> 0:PAL, 1:NTSC, 2:960H_PAL, 3:960H_NTSC, 4:720P_PAL, 5:720P_NTSC");

static int order[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 1};
module_param_array(order, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(order, "Field Order=> 0:Anyone, 1:Odd First, 2:Even First");

static int inv_clk[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = 0};
module_param_array(inv_clk, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(inv_clk, "Invert Clock, 0:Disable, 1: Enable");

struct nvp6114_dev_t {
    int                     index;                  ///< device index
    struct semaphore        lock;                   ///< device locker
    int                     norm;                   ///< device operation norm mode
    int                     vmode;                  ///< device operation video mode
    int                     vlos[DEV_CH_MAX];       ///< device channel video loss status
    struct vcap_input_dev_t *port[DEV_VPORT_MAX];
};

static struct nvp6114_dev_t   nvp6114_dev[DEV_MAX];
static struct proc_dir_entry *nvp6114_proc_root[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp6114_proc_norm[DEV_MAX]    = {[0 ... (DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *nvp6114_proc_timeout[DEV_MAX] = {[0 ... (DEV_MAX - 1)] = NULL};

/****************************************************************************************
 *  NVP6114 VPort and X_CAP# Port Mapping for GM8210/GM8287 Socket Board
 *
 *  NVP6114#0(0x60) --->VPortA ---- [VIN#0,1] 72MHz ---> X_CAP#0
 *                  --->VPortB ---- [VIN#2,3] 72MHz ---> X_CAP#1
 *
 *  NVP6114#1(0x62) --->VPortA ---- [VIN#0,1] 72MHz ---> X_CAP#2
 *                  --->VPortB ---- [VIN#2,3] 72MHz ---> X_CAP#3
 *
 *  NVP6114#2(0x64) --->VPortA ---- [VIN#0,1] 72MHz ---> X_CAP#4
 *                  --->VPortB ---- [VIN#2,3] 72MHz ---> X_CAP#5
 *
 *  NVP6114#3(0x66) --->VPortA ---- [VIN#0,1] 72MHz ---> X_CAP#6
 *                  --->VPortB ---- [VIN#2,3] 72MHz ---> X_CAP#7
 *
 *****************************************************************************************/

static int nvp6114_module_get(void)
{
    return try_module_get(THIS_MODULE);
}

static void nvp6114_module_put(void)
{
    module_put(THIS_MODULE);
}

static int nvp6114_device_init(int id, int norm)
{
    int i, vmode, ret = 0;
    int port_norm[DEV_VPORT_MAX]   = {[0 ... (DEV_VPORT_MAX - 1)] = 0};
    int width[DEV_VPORT_MAX]       = {[0 ... (DEV_VPORT_MAX - 1)] = 0};
    int height[DEV_VPORT_MAX]      = {[0 ... (DEV_VPORT_MAX - 1)] = 0};
    int frame_rate[DEV_VPORT_MAX]  = {[0 ... (DEV_VPORT_MAX - 1)] = 0};
    int speed[DEV_VPORT_MAX]       = {[0 ... (DEV_VPORT_MAX - 1)] = 0};
    int interface[DEV_VPORT_MAX]   = {[0 ... (DEV_VPORT_MAX - 1)] = 0};
    int field_order[DEV_VPORT_MAX] = {[0 ... (DEV_VPORT_MAX - 1)] = 0};
    u32 timeout[DEV_VPORT_MAX]     = {[0 ... (DEV_VPORT_MAX - 1)] = 0};
    struct vcap_input_dev_t *pinput;

    if((id >= DEV_MAX) || (id >= nvp6114_get_device_num()))
        return -1;

    if(nvp6114_dev[id].norm != norm) {
        switch(norm) {
            case VCAP_INPUT_NORM_PAL:
                if(mode[id] == 0)
                    vmode = NVP6114_VMODE_PAL_720H_1CH;
                else if(mode[id] == 1)
                    vmode = NVP6114_VMODE_PAL_720H_2CH;
                else if(mode[id] == 2)
                    vmode = NVP6114_VMODE_PAL_720H_4CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    width[i]       = 720;
                    height[i]      = 576;
                    frame_rate[i]  = 25;
                    timeout[i]     = DEFAULT_PAL_TIMEOUT;
                    speed[i]       = VCAP_INPUT_SPEED_I;
                    interface[i]   = VCAP_INPUT_INTF_BT656_INTERLACE;
                    field_order[i] = order[id];
                    port_norm[i]   = VCAP_INPUT_NORM_PAL;
                }
                break;
            case VCAP_INPUT_NORM_NTSC:
                if(mode[id] == 0)
                    vmode = NVP6114_VMODE_NTSC_720H_1CH;
                else if(mode[id] == 1)
                    vmode = NVP6114_VMODE_NTSC_720H_2CH;
                else if(mode[id] == 2)
                    vmode = NVP6114_VMODE_NTSC_720H_4CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    width[i]       = 720;
                    height[i]      = 480;
                    frame_rate[i]  = 30;
                    timeout[i]     = DEFAULT_NTSC_TIMEOUT;
                    speed[i]       = VCAP_INPUT_SPEED_I;
                    interface[i]   = VCAP_INPUT_INTF_BT656_INTERLACE;
                    field_order[i] = order[id];
                    port_norm[i]   = VCAP_INPUT_NORM_NTSC;
                }
                break;
            case VCAP_INPUT_NORM_PAL_960H:
                if(mode[id] == 0)
                    vmode = NVP6114_VMODE_PAL_960H_1CH;
                else if(mode[id] == 1)
                    vmode = NVP6114_VMODE_PAL_960H_2CH;
                else if(mode[id] == 2)
                    vmode = NVP6114_VMODE_PAL_960H_4CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    width[i]       = 960;
                    height[i]      = 576;
                    frame_rate[i]  = 25;
                    timeout[i]     = DEFAULT_PAL_TIMEOUT;
                    speed[i]       = VCAP_INPUT_SPEED_I;
                    interface[i]   = VCAP_INPUT_INTF_BT656_INTERLACE;
                    field_order[i] = order[id];
                    port_norm[i]   = VCAP_INPUT_NORM_PAL_960H;
                }
                break;
            case VCAP_INPUT_NORM_NTSC_960H:
                if(mode[id] == 0)
                    vmode = NVP6114_VMODE_NTSC_960H_1CH;
                else if(mode[id] == 1)
                    vmode = NVP6114_VMODE_NTSC_960H_2CH;
                else if(mode[id] == 2)
                    vmode = NVP6114_VMODE_NTSC_960H_4CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    width[i]       = 960;
                    height[i]      = 480;
                    frame_rate[i]  = 30;
                    timeout[i]     = DEFAULT_NTSC_TIMEOUT;
                    speed[i]       = VCAP_INPUT_SPEED_I;
                    interface[i]   = VCAP_INPUT_INTF_BT656_INTERLACE;
                    field_order[i] = order[id];
                    port_norm[i]   = VCAP_INPUT_NORM_NTSC_960H;
                }
                break;
            case VCAP_INPUT_NORM_720P25:
                if(mode[id] == 0)
                    vmode = NVP6114_VMODE_PAL_720P_1CH;
                else if(mode[id] == 1)
                    vmode = NVP6114_VMODE_PAL_720P_2CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    width[i]       = 1280;
                    height[i]      = 720;
                    frame_rate[i]  = 25;
                    timeout[i]     = DEFAULT_PAL_TIMEOUT;
                    speed[i]       = VCAP_INPUT_SPEED_P;
                    interface[i]   = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                    field_order[i] = VCAP_INPUT_ORDER_ANYONE;
                    port_norm[i]   = VCAP_INPUT_NORM_720P25;
                }
                break;
            case 6: ///< PAL 720H + 720P Hybrid 2CH Mode
                if(mode[id] == 1)
                    vmode = NVP6114_VMODE_PAL_720H_720P_2CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    if(i == NVP6114_DEV_VPORT_A) {
                        width[i]       = 720;
                        height[i]      = 576;
                        frame_rate[i]  = 25;
                        timeout[i]     = DEFAULT_PAL_TIMEOUT;
                        speed[i]       = VCAP_INPUT_SPEED_I;
                        interface[i]   = VCAP_INPUT_INTF_BT656_INTERLACE;
                        field_order[i] = order[id];
                        port_norm[i]   = VCAP_INPUT_NORM_PAL;
                    }
                    else {
                        width[i]       = 1280;
                        height[i]      = 720;
                        frame_rate[i]  = 25;
                        timeout[i]     = DEFAULT_PAL_TIMEOUT;
                        interface[i]   = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                        speed[i]       = VCAP_INPUT_SPEED_P;
                        field_order[i] = VCAP_INPUT_ORDER_ANYONE;
                        port_norm[i]   = VCAP_INPUT_NORM_720P25;
                    }
                }
                break;
            case 7: ///< NTSC 720H + 720P Hybrid Mode
                if(mode[id] == 1)
                    vmode = NVP6114_VMODE_NTSC_720H_720P_2CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    if(i == NVP6114_DEV_VPORT_A) {
                        width[i]       = 720;
                        height[i]      = 480;
                        frame_rate[i]  = 30;
                        timeout[i]     = DEFAULT_NTSC_TIMEOUT;
                        speed[i]       = VCAP_INPUT_SPEED_I;
                        interface[i]   = VCAP_INPUT_INTF_BT656_INTERLACE;
                        field_order[i] = order[id];
                        port_norm[i]   = VCAP_INPUT_NORM_NTSC;
                    }
                    else {
                        width[i]       = 1280;
                        height[i]      = 720;
                        frame_rate[i]  = 30;
                        timeout[i]     = DEFAULT_NTSC_TIMEOUT;
                        interface[i]   = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                        speed[i]       = VCAP_INPUT_SPEED_P;
                        field_order[i] = VCAP_INPUT_ORDER_ANYONE;
                        port_norm[i]   = VCAP_INPUT_NORM_720P30;
                    }
                }
                break;
            case 8: ///< PAL 720P + 720H Hybrid 2CH Mode
                if(mode[id] == 1)
                    vmode = NVP6114_VMODE_PAL_720P_720H_2CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    if(i == NVP6114_DEV_VPORT_B) {
                        width[i]       = 720;
                        height[i]      = 576;
                        frame_rate[i]  = 25;
                        timeout[i]     = DEFAULT_PAL_TIMEOUT;
                        speed[i]       = VCAP_INPUT_SPEED_I;
                        interface[i]   = VCAP_INPUT_INTF_BT656_INTERLACE;
                        field_order[i] = order[id];
                        port_norm[i]   = VCAP_INPUT_NORM_PAL;
                    }
                    else {
                        width[i]       = 1280;
                        height[i]      = 720;
                        frame_rate[i]  = 25;
                        timeout[i]     = DEFAULT_PAL_TIMEOUT;
                        interface[i]   = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                        speed[i]       = VCAP_INPUT_SPEED_P;
                        field_order[i] = VCAP_INPUT_ORDER_ANYONE;
                        port_norm[i]   = VCAP_INPUT_NORM_720P25;
                    }
                }
                break;
            case 9: ///< NTSC 720P + 720H Hybrid Mode
                if(mode[id] == 1)
                    vmode = NVP6114_VMODE_NTSC_720P_720H_2CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    if(i == NVP6114_DEV_VPORT_B) {
                        width[i]       = 720;
                        height[i]      = 480;
                        frame_rate[i]  = 30;
                        timeout[i]     = DEFAULT_NTSC_TIMEOUT;
                        speed[i]       = VCAP_INPUT_SPEED_I;
                        interface[i]   = VCAP_INPUT_INTF_BT656_INTERLACE;
                        field_order[i] = order[id];
                        port_norm[i]   = VCAP_INPUT_NORM_NTSC;
                    }
                    else {
                        width[i]       = 1280;
                        height[i]      = 720;
                        frame_rate[i]  = 30;
                        timeout[i]     = DEFAULT_NTSC_TIMEOUT;
                        interface[i]   = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                        speed[i]       = VCAP_INPUT_SPEED_P;
                        field_order[i] = VCAP_INPUT_ORDER_ANYONE;
                        port_norm[i]   = VCAP_INPUT_NORM_720P30;
                    }
                }
                break;
            case 10: ///< PAL 960H + 720P Hybrid 2CH Mode
                if(mode[id] == 1)
                    vmode = NVP6114_VMODE_PAL_960H_720P_2CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    if(i == NVP6114_DEV_VPORT_A) {
                        width[i]       = 960;
                        height[i]      = 576;
                        frame_rate[i]  = 25;
                        timeout[i]     = DEFAULT_PAL_TIMEOUT;
                        speed[i]       = VCAP_INPUT_SPEED_I;
                        interface[i]   = VCAP_INPUT_INTF_BT656_INTERLACE;
                        field_order[i] = order[id];
                        port_norm[i]   = VCAP_INPUT_NORM_PAL_960H;
                    }
                    else {
                        width[i]       = 1280;
                        height[i]      = 720;
                        frame_rate[i]  = 25;
                        timeout[i]     = DEFAULT_PAL_TIMEOUT;
                        interface[i]   = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                        speed[i]       = VCAP_INPUT_SPEED_P;
                        field_order[i] = VCAP_INPUT_ORDER_ANYONE;
                        port_norm[i]   = VCAP_INPUT_NORM_720P25;
                    }
                }
                break;
            case 11: ///< NTSC 960H + 720P Hybrid Mode
                if(mode[id] == 1)
                    vmode = NVP6114_VMODE_NTSC_960H_720P_2CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    if(i == NVP6114_DEV_VPORT_A) {
                        width[i]       = 960;
                        height[i]      = 480;
                        frame_rate[i]  = 30;
                        timeout[i]     = DEFAULT_NTSC_TIMEOUT;
                        speed[i]       = VCAP_INPUT_SPEED_I;
                        interface[i]   = VCAP_INPUT_INTF_BT656_INTERLACE;
                        field_order[i] = order[id];
                        port_norm[i]   = VCAP_INPUT_NORM_NTSC_960H;
                    }
                    else {
                        width[i]       = 1280;
                        height[i]      = 720;
                        frame_rate[i]  = 30;
                        timeout[i]     = DEFAULT_NTSC_TIMEOUT;
                        interface[i]   = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                        speed[i]       = VCAP_INPUT_SPEED_P;
                        field_order[i] = VCAP_INPUT_ORDER_ANYONE;
                        port_norm[i]   = VCAP_INPUT_NORM_720P30;
                    }
                }
                break;
            case 12: ///< PAL 720P + 960H Hybrid 2CH Mode
                if(mode[id] == 1)
                    vmode = NVP6114_VMODE_PAL_720P_960H_2CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    if(i == NVP6114_DEV_VPORT_B) {
                        width[i]       = 960;
                        height[i]      = 576;
                        frame_rate[i]  = 25;
                        timeout[i]     = DEFAULT_PAL_TIMEOUT;
                        speed[i]       = VCAP_INPUT_SPEED_I;
                        interface[i]   = VCAP_INPUT_INTF_BT656_INTERLACE;
                        field_order[i] = order[id];
                        port_norm[i]   = VCAP_INPUT_NORM_PAL_960H;
                    }
                    else {
                        width[i]       = 1280;
                        height[i]      = 720;
                        frame_rate[i]  = 25;
                        timeout[i]     = DEFAULT_PAL_TIMEOUT;
                        interface[i]   = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                        speed[i]       = VCAP_INPUT_SPEED_P;
                        field_order[i] = VCAP_INPUT_ORDER_ANYONE;
                        port_norm[i]   = VCAP_INPUT_NORM_720P25;
                    }
                }
                break;
            case 13: ///< NTSC 720P + 960H Hybrid Mode
                if(mode[id] == 1)
                    vmode = NVP6114_VMODE_NTSC_720P_960H_2CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    if(i == NVP6114_DEV_VPORT_B) {
                        width[i]       = 960;
                        height[i]      = 480;
                        frame_rate[i]  = 30;
                        timeout[i]     = DEFAULT_NTSC_TIMEOUT;
                        speed[i]       = VCAP_INPUT_SPEED_I;
                        interface[i]   = VCAP_INPUT_INTF_BT656_INTERLACE;
                        field_order[i] = order[id];
                        port_norm[i]   = VCAP_INPUT_NORM_NTSC_960H;
                    }
                    else {
                        width[i]       = 1280;
                        height[i]      = 720;
                        frame_rate[i]  = 30;
                        timeout[i]     = DEFAULT_NTSC_TIMEOUT;
                        interface[i]   = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                        speed[i]       = VCAP_INPUT_SPEED_P;
                        field_order[i] = VCAP_INPUT_ORDER_ANYONE;
                        port_norm[i]   = VCAP_INPUT_NORM_720P30;
                    }
                }
                break;
            default: /* NTSC 720P */
                if(mode[id] == 0)
                    vmode = NVP6114_VMODE_NTSC_720P_1CH;
                else if(mode[id] == 1)
                    vmode = NVP6114_VMODE_NTSC_720P_2CH;
                else {
                    ret = -1;
                    goto end;
                }

                for(i=0; i<DEV_VPORT_MAX; i++) {
                    width[i]       = 1280;
                    height[i]      = 720;
                    frame_rate[i]  = 30;
                    timeout[i]     = DEFAULT_NTSC_TIMEOUT;
                    interface[i]   = VCAP_INPUT_INTF_BT656_PROGRESSIVE;
                    speed[i]       = VCAP_INPUT_SPEED_P;
                    field_order[i] = VCAP_INPUT_ORDER_ANYONE;
                    port_norm[i]   = VCAP_INPUT_NORM_720P30;
                }
                break;
        }
        if(ret < 0)
            goto end;

        /* Update device norm */
        nvp6114_dev[id].norm  = norm;
        nvp6114_dev[id].vmode = vmode;
        for(i=0; i<DEV_VPORT_MAX; i++) {
            pinput = nvp6114_dev[id].port[i];
            if(pinput && (pinput->norm.mode != port_norm[i])) {
                pinput->norm.mode   = port_norm[i];
                pinput->norm.width  = width[i];
                pinput->norm.height = height[i];
                pinput->frame_rate  = pinput->max_frame_rate = frame_rate[i];
                pinput->timeout_ms  = timeout[i];
                pinput->speed       = speed[i];
                pinput->interface   = interface[i];
                pinput->field_order = field_order[i];
            }
        }
    }

end:
    if(ret < 0)
        vcap_err("NVP6114#%d init failed!\n", id);

    return ret;
}

static int nvp6114_proc_norm_show(struct seq_file *sfile, void *v)
{
    int i;
    struct nvp6114_dev_t *pdev = (struct nvp6114_dev_t *)sfile->private;
    char *norm_str[] = {"720H_PAL", "720H_NTSC", "960H_PAL", "960H_NTSC", "720P_PAL", "720P_NTSC",
                        "720H_720P_PAL", "720H_720P_NTSC", "720P_720H_PAL", "720P_720H_NTSC",
                        "960H_720P_PAL", "960H_720P_NTSC", "720P_960H_PAL", "720P_960H_NTSC"};

    down(&pdev->lock);

    for(i=0; i<14; i++) {
        seq_printf(sfile, "[%02d]: %s\n", i, norm_str[i]);
    }
    seq_printf(sfile, "----------------------------------\n");
    seq_printf(sfile, "Current: %s\n", (pdev->norm>=0 && pdev->norm<=13) ? norm_str[pdev->norm] : "Unknown");

    up(&pdev->lock);

    return 0;
}

static int nvp6114_proc_timeout_show(struct seq_file *sfile, void *v)
{
    int i;
    u32 time_ms = 0;
    struct nvp6114_dev_t *pdev = (struct nvp6114_dev_t *)sfile->private;

    down(&pdev->lock);

    for(i=0; i<DEV_VPORT_MAX; i++) {
        if(pdev->port[i]) {
            time_ms = (pdev->port[i])->timeout_ms;
            break;
        }
    }
    seq_printf(sfile, "Timeout Threshold: %u (ms)\n", time_ms);

    up(&pdev->lock);

    return 0;
}

static ssize_t nvp6114_proc_timeout_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, time_value;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct nvp6114_dev_t *pdev = (struct nvp6114_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &time_value);

    down(&pdev->lock);

    if(time_value > 0) {
        for(i=0; i<DEV_VPORT_MAX; i++) {
            if(pdev->port[i] && ((pdev->port[i])->timeout_ms != time_value))
                (pdev->port[i])->timeout_ms = time_value;
        }
    }

    up(&pdev->lock);

    return count;
}

static int nvp6114_proc_norm_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp6114_proc_norm_show, PDE(inode)->data);
}

static int nvp6114_proc_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvp6114_proc_timeout_show, PDE(inode)->data);
}

static struct file_operations nvp6114_proc_norm_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp6114_proc_norm_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations nvp6114_proc_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = nvp6114_proc_timeout_open,
    .write  = nvp6114_proc_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void nvp6114_proc_remove(int id)
{
    if(nvp6114_proc_root[id]) {
        if(nvp6114_proc_timeout[id])
            vcap_input_proc_remove_entry(nvp6114_proc_root[id], nvp6114_proc_timeout[id]);

        if(nvp6114_proc_norm[id])
            vcap_input_proc_remove_entry(nvp6114_proc_root[id], nvp6114_proc_norm[id]);

        vcap_input_proc_remove_entry(NULL, nvp6114_proc_root[id]);
    }
}

static int nvp6114_proc_init(struct nvp6114_dev_t *pdev)
{
    int ret = 0;
    int id  = pdev->index;
    char name[32];

    /* root */
    sprintf(name, "nvp6114.%d", id);
    nvp6114_proc_root[id] = vcap_input_proc_create_entry(name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!nvp6114_proc_root[id]) {
        vcap_err("create proc node '%s' failed!\n", name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp6114_proc_root[id]->owner = THIS_MODULE;
#endif

    /* norm */
    nvp6114_proc_norm[id] = vcap_input_proc_create_entry("norm", S_IRUGO|S_IXUGO, nvp6114_proc_root[id]);
    if(!nvp6114_proc_norm[id]) {
        vcap_err("create proc node '%s/norm' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    nvp6114_proc_norm[id]->proc_fops = &nvp6114_proc_norm_ops;
    nvp6114_proc_norm[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp6114_proc_norm[id]->owner     = THIS_MODULE;
#endif

    /* timeout */
    nvp6114_proc_timeout[id] = vcap_input_proc_create_entry("timeout", S_IRUGO|S_IXUGO, nvp6114_proc_root[id]);
    if(!nvp6114_proc_timeout[id]) {
        vcap_err("create proc node '%s/timeout' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    nvp6114_proc_timeout[id]->proc_fops = &nvp6114_proc_timeout_ops;
    nvp6114_proc_timeout[id]->data      = (void *)pdev;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    nvp6114_proc_timeout[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    nvp6114_proc_remove(id);
    return ret;
}

static int nvp6114_vlos_notify_handler(int id, int vin, int vlos)
{
    int i, j, vch;
    struct nvp6114_dev_t *pdev = &nvp6114_dev[id];

    if((id >= DEV_MAX) || (vin >= DEV_CH_MAX))
        return -1;

    down(&pdev->lock);

    if(pdev->vlos[vin] != vlos) {
        pdev->vlos[vin] = vlos;

        vch = nvp6114_vin_to_vch(id, vin);
        for(i=0; i<DEV_VPORT_MAX; i++) {
            if(!pdev->port[i])
                continue;

            for(j=0; j<VCAP_INPUT_DEV_CH_MAX; j++) {
                if((pdev->port[i])->ch_id[j] == vch) {
                    (pdev->port[i])->ch_vlos[j] = vlos;
                    vcap_input_device_notify((pdev->port[i])->vi_idx, j, ((vlos == 0) ? VCAP_INPUT_NOTIFY_SIGNAL_PRESENT : VCAP_INPUT_NOTIFY_SIGNAL_LOSS));
                    goto exit;
                }
            }
        }
    }

exit:
    up(&pdev->lock);

    return 0;
}

static int nvp6114_vfmt_notify_handler(int id, int vmode)
{
    int i, input_norm, ret;
    int port_norm[DEV_VPORT_MAX], frame_rate[DEV_VPORT_MAX];
    struct nvp6114_dev_t *pdev = &nvp6114_dev[id];

    if(id >= DEV_MAX)
        return -1;

    down(&pdev->lock);

    if(pdev->vmode != vmode) {
        switch(vmode) {
            case NVP6114_VMODE_NTSC_720H_1CH:
            case NVP6114_VMODE_NTSC_720H_2CH:
            case NVP6114_VMODE_NTSC_720H_4CH:
                input_norm = VCAP_INPUT_NORM_NTSC;
                break;
            case NVP6114_VMODE_NTSC_960H_1CH:
            case NVP6114_VMODE_NTSC_960H_2CH:
            case NVP6114_VMODE_NTSC_960H_4CH:
                input_norm = VCAP_INPUT_NORM_NTSC_960H;
                break;
            case NVP6114_VMODE_NTSC_720P_1CH:
            case NVP6114_VMODE_NTSC_720P_2CH:
                input_norm = VCAP_INPUT_NORM_720P30;
                break;
            case NVP6114_VMODE_NTSC_720H_720P_2CH:
                input_norm = 7;
                break;
            case NVP6114_VMODE_NTSC_720P_720H_2CH:
                input_norm = 9;
                break;
            case NVP6114_VMODE_NTSC_960H_720P_2CH:
                input_norm = 11;
                break;
            case NVP6114_VMODE_NTSC_720P_960H_2CH:
                input_norm = 13;
                break;

            case NVP6114_VMODE_PAL_720H_1CH:
            case NVP6114_VMODE_PAL_720H_2CH:
            case NVP6114_VMODE_PAL_720H_4CH:
                input_norm = VCAP_INPUT_NORM_PAL;
                break;
            case NVP6114_VMODE_PAL_960H_1CH:
            case NVP6114_VMODE_PAL_960H_2CH:
            case NVP6114_VMODE_PAL_960H_4CH:
                input_norm = VCAP_INPUT_NORM_PAL_960H;
                break;
            case NVP6114_VMODE_PAL_720P_1CH:
            case NVP6114_VMODE_PAL_720P_2CH:
                input_norm = VCAP_INPUT_NORM_720P25;
                break;
            case NVP6114_VMODE_PAL_720H_720P_2CH:
                input_norm = 6;
                break;
            case NVP6114_VMODE_PAL_720P_720H_2CH:
                input_norm = 8;
                break;
            case NVP6114_VMODE_PAL_960H_720P_2CH:
                input_norm = 10;
                break;
            case NVP6114_VMODE_PAL_720P_960H_2CH:
                input_norm = 12;
                break;
            default:
                goto exit;
        }

        /* store current norm and frame rate value for notify compare */
        for(i=0; i<DEV_VPORT_MAX; i++) {
            if(!pdev->port[i])
                continue;

            port_norm[i]  = (pdev->port[i])->norm.mode;
            frame_rate[i] = (pdev->port[i])->frame_rate;
        }

        ret = nvp6114_device_init(id, input_norm);
        if(ret == 0) {
            for(i=0; i<DEV_VPORT_MAX; i++) {
                if(!pdev->port[i])
                    continue;

                switch((pdev->port[i])->mode) {
                    case VCAP_INPUT_MODE_BYPASS:
                        if(port_norm[i] != (pdev->port[i])->norm.mode) {
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                        }

                        if(frame_rate[i] != (pdev->port[i])->frame_rate) {
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                        }
                        break;
                    case VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE:
                        if(port_norm[i] != (pdev->port[i])->norm.mode) {
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 2, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                        }

                        if(frame_rate[i] != (pdev->port[i])->frame_rate) {
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 2, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                        }
                        break;
                    case VCAP_INPUT_MODE_4CH_BYTE_INTERLEAVE:
                        if(port_norm[i] != (pdev->port[i])->norm.mode) {
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 0, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 1, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 2, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 3, VCAP_INPUT_NOTIFY_NORM_CHANGE);
                        }

                        if(frame_rate[i] != (pdev->port[i])->frame_rate) {
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 0, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 1, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 2, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                            vcap_input_device_notify((pdev->port[i])->vi_idx, 3, VCAP_INPUT_NOTIFY_FRAMERATE_CHANGE);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

exit:
    up(&pdev->lock);

    return 0;
}

static int __init nvp6114_input_init(void)
{
    int i, j;
    int dev_num;
    struct vcap_input_dev_t *pinput;
    int ret = 0;

    if(vcap_input_get_version() != VCAP_INPUT_VERSION) {
        vcap_err("Input driver version(%08x) is not compatibility with vcap300_common.ko(%08x)!\n",
                 vcap_input_get_version(), VCAP_INPUT_VERSION);
        return -EFAULT;
    }

    /* clear buffer */
    memset(nvp6114_dev, 0, sizeof(struct nvp6114_dev_t)*DEV_MAX);

    /* Get NVP6114 Device Number */
    dev_num = nvp6114_get_device_num();

    for(i=0; i<DEV_MAX; i++) {
        nvp6114_dev[i].index = i;
        nvp6114_dev[i].norm  = -1;  ///< init value

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&nvp6114_dev[i].lock, 1);
#else
        init_MUTEX(&nvp6114_dev[i].lock);
#endif
        if(i >= dev_num)
            continue;

        if(vport[i]) {
            /* register input device */
            for(j=0; j<DEV_VPORT_MAX; j++) {
                if((vport[i]>>(4*j)) & 0xf) {   ///< VPort# Connect to X_CAP#
                    pinput = nvp6114_dev[i].port[j] = kzalloc(sizeof(struct vcap_input_dev_t), GFP_KERNEL);
                    if(pinput == NULL) {
                        vcap_err("NVP6114#%d.%d allocate vcap_input_dev_t failed\n", i, j);
                        ret = -ENOMEM;
                        goto err;
                    }

                    /* input device name */
                    snprintf(pinput->name, VCAP_INPUT_NAME_SIZE-1, "nvp6114.%d.%d", i, j);

                    /* input device parameter setup */
                    pinput->index       = (i*DEV_VPORT_MAX) + j;
                    pinput->vi_idx      = ((vport[i]>>(4*j)) & 0xf) - 1; ///< VI#
                    pinput->vi_src      = ((vport[i]>>(4*j)) & 0xf) - 1; ///< X_CAP#
                    pinput->type        = DEFAULT_TYPE;
                    pinput->data_range  = DEFAULT_DATA_RANGE;
                    pinput->yc_swap     = 0;
                    pinput->data_swap   = 0;
                    pinput->ch_id_mode  = DEFAULT_CH_ID_MODE;
                    pinput->inv_clk     = inv_clk[i];
                    pinput->init        = nvp6114_device_init;
                    pinput->norm.mode   = -1;                            ///< init value
                    pinput->module_get  = nvp6114_module_get;
                    pinput->module_put  = nvp6114_module_put;

                    switch(mode[i]) {
                        case 0:  /* 36MHz ==> 1CH */
                            pinput->mode     = VCAP_INPUT_MODE_BYPASS;
                            pinput->ch_id[0] = nvp6114_get_vch_id(i, j, 0);
                            break;
                        case 1:  /* 72MHz ==> 2CH */
                            pinput->mode     = VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE;
                            pinput->ch_id[0] = nvp6114_get_vch_id(i, j, 0);
                            pinput->ch_id[2] = nvp6114_get_vch_id(i, j, 1);
                            break;
                        default: /* 144MHz ==> 4CH */
                            pinput->mode     = VCAP_INPUT_MODE_4CH_BYTE_INTERLEAVE;
                            pinput->ch_id[0] = nvp6114_get_vch_id(i, j, 0);
                            pinput->ch_id[1] = nvp6114_get_vch_id(i, j, 1);
                            pinput->ch_id[2] = nvp6114_get_vch_id(i, j, 2);
                            pinput->ch_id[3] = nvp6114_get_vch_id(i, j, 3);
                            break;
                    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
                    sema_init(&pinput->sema_lock, 1);
#else
                    init_MUTEX(&pinput->sema_lock);
#endif
                    ret = vcap_input_device_register(pinput);
                    if(ret < 0) {
                        vcap_err("register NVP6114#%d.%d input device failed\n", i, j);
                        goto err;
                    }
                }
            }

            /* device proc init */
            ret = nvp6114_proc_init(&nvp6114_dev[i]);
            if(ret < 0)
                goto err;

            /* device init */
            ret = nvp6114_device_init(i, norm[i]);
            if(ret < 0)
                goto err;

            /* register nvp6114 video loss notify handler */
            nvp6114_notify_vlos_register(i, nvp6114_vlos_notify_handler);

            /* register nvp6114 video format notify handler */
            nvp6114_notify_vfmt_register(i, nvp6114_vfmt_notify_handler);

            vcap_info("Register NVP6114#%d Input Device\n", i);
        }
    }

    return ret;

err:
    for(i=0; i<DEV_MAX; i++) {
        nvp6114_notify_vlos_deregister(i);
        nvp6114_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(nvp6114_dev[i].port[j]) {
                vcap_input_device_unregister(nvp6114_dev[i].port[j]);
                kfree(nvp6114_dev[i].port[j]);
            }
        }
    }
    return ret;
}

static void __exit nvp6114_input_exit(void)
{
    int i, j;

    for(i=0; i<DEV_MAX; i++) {
        nvp6114_notify_vlos_deregister(i);
        nvp6114_notify_vfmt_deregister(i);
        nvp6114_proc_remove(i);
        for(j=0; j<DEV_VPORT_MAX; j++) {
            if(nvp6114_dev[i].port[j]) {
                vcap_input_device_unregister(nvp6114_dev[i].port[j]);
                kfree(nvp6114_dev[i].port[j]);
            }
        }
    }
}

module_init(nvp6114_input_init);
module_exit(nvp6114_input_exit);

MODULE_DESCRIPTION("Grain Media Video Capture300 NVP6114 Input Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
