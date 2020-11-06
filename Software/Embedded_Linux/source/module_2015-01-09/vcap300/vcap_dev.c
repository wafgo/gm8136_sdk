/**
 * @file vcap_dev.c
 *  vcap300 device
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.10 $
 * $Date: 2014/11/27 01:53:59 $
 *
 * ChangeLog:
 *  $Log: vcap_dev.c,v $
 *  Revision 1.10  2014/11/27 01:53:59  jerson_l
 *  1. add vcrop_rule and vup_min module parameter
 *
 *  Revision 1.9  2014/09/05 02:48:53  jerson_l
 *  1. support osd force frame mode for GM8136
 *  2. support osd edge smooth edge mode for GM8136
 *  3. support osd auto color change scheme for GM8136
 *
 *  Revision 1.8  2014/04/14 02:51:37  jerson_l
 *  1. support grab_filter to filter fail frame drop rule
 *
 *  Revision 1.7  2014/03/26 05:48:16  jerson_l
 *  1. add vi_max_w module parameter for control sd line buffer calculation mechanism
 *
 *  Revision 1.6  2014/03/06 04:01:58  jerson_l
 *  1. add ext_irq_src module parameter to support extra ll_done interrupt source
 *
 *  Revision 1.5  2013/05/08 03:31:09  jerson_l
 *  1. add module parameter of source cropping rule support
 *
 *  Revision 1.4  2013/03/26 02:36:05  jerson_l
 *  1. add cap_md module parameter for control MD function support
 *
 *  Revision 1.3  2013/03/11 08:11:17  jerson_l
 *  1. add sync_time_div module parameter for setup capture hardware sync timer timeout value.
 *
 *  Revision 1.2  2012/10/24 06:47:51  jerson_l
 *
 *  switch file to unix format
 *
 *  Revision 1.1.1.1  2012/10/23 08:16:31  jerson_l
 *  import vcap300 driver
 *
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "vcap_dev.h"
#include "vcap_dbg.h"

#define  VCAP_DEV_ID                    0
#define  VCAP_DEFAULT_SYNC_TIME_DIV     60

/*
 * Module Parameter
 */
static int cap_mode = VCAP_CAP_MODE_LLI;
module_param(cap_mode, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(cap_mode, "Capture Mode=> 0:SSF, 1:LLI");

static int sync_time_div = VCAP_DEFAULT_SYNC_TIME_DIV;  ///< Sync Timer Timeout 1/60(s)
module_param(sync_time_div, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(sync_time_div, "Sync Timer Divide Value");

static int vi_mode[VCAP_VI_MAX] = {[0 ... (VCAP_VI_MAX - 1)] = 0};
module_param_array(vi_mode, int, NULL, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(vi_mode, "VI Mode => 0:Disable, 1:Bypass, 2:2CH, 3:4CH, 4:Split");

static int vi_max_w[VCAP_VI_MAX] = {[0 ... (VCAP_VI_MAX - 1)] = 0};
module_param_array(vi_max_w, int, NULL, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(vi_max_w, "VI Max Width => 0~4096, 0:Default sd line buffer calculation mechanism");

static int split_xy[2] = {1, 1};
module_param_array(split_xy, int, NULL, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(split_xy, "(SPLIT_X_NUM, SPLIT_Y_NUM)");

static int cap_md = 1;
module_param(cap_md, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(cap_md, "Capture Motion Detection => 0:Off, 1:On");

static int hcrop_rule[VCAP_SC_RULE_MAX*2] = {[0 ... ((VCAP_SC_RULE_MAX*2) - 1)] = 0};
module_param_array(hcrop_rule, int, NULL, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(hcrop_rule, "Capture Horizontal Source Cropping Rule");

static int vcrop_rule[VCAP_SC_RULE_MAX*2] = {[0 ... ((VCAP_SC_RULE_MAX*2) - 1)] = 0};
module_param_array(vcrop_rule, int, NULL, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(vcrop_rule, "Capture Vertical Source Cropping Rule");

static uint ext_irq_src = VCAP_EXTRA_IRQ_SRC_NONE;
module_param(ext_irq_src, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ext_irq_src, "Capture Extra Interrupt Source: BIT0: LL_Done");

static uint grab_filter = VCAP_GRAB_FILTER_SD_LINE_LACK;
module_param(grab_filter, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(grab_filter, "Grab filter for fail frame drop");

static int accs_func = 0;
module_param(accs_func, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(accs_func, "OSD Auto Color Change Scheme => 0:Disable, 1:Half_Mark_Memory, 2:Full_Mark_Memory");

static int vup_min = 0;
module_param(vup_min, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(vup_min, "Vertical minimal scaling up line number");

static const char vcap_ssf_str[]="vcap_ssf";
static const char vcap_lli_str[]="vcap_lli";

static struct vcap_dev_module_param_t vcap_dev_m_param;

static void vcap_device_release(struct device *dev)
{
    return;
}

static struct resource vcap_dev_resource[] = {
    [0] = {
        .start = VCAP_PHY_ADDR_BASE,
        .end   = (VCAP_PHY_ADDR_BASE + VCAP_REG_SIZE),
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = VCAP_IRQ_NUM,
        .end   = VCAP_IRQ_NUM,
        .flags = IORESOURCE_IRQ,
    }
};

static u64 dmamask = ~(u32) 0;
static struct platform_device vcap_device = {
    .id            = VCAP_DEV_ID,
    .num_resources = ARRAY_SIZE(vcap_dev_resource),
    .resource      = vcap_dev_resource,
    .dev = {
        .dma_mask          = &dmamask,
        .coherent_dma_mask = 0xffffffff,
        .release           = vcap_device_release,
    }
};

static int __init vcap_dev_init(void)
{
    int i;
    int ret = 0;
#ifdef PLAT_SPLIT
    int split_cnt = 0;
#endif

    /* check module parameter */
    if(cap_mode > VCAP_DEV_MODE_LLI)
        cap_mode = VCAP_DEV_MODE_LLI;

    /* check sync timer divide value */
    if(sync_time_div <= 0)
        sync_time_div = VCAP_DEFAULT_SYNC_TIME_DIV;

    /* check vi mode */
    for(i=0; i<VCAP_VI_MAX; i++) {
        if(vi_mode[i] >= VCAP_VI_RUN_MODE_MAX) {
            vcap_err("[%d] vi#%d vi_mode=%d invalid!(0:Disable, 1:Bypass, 2:2CH, 3:4CH, 4:Split)\n", vcap_device.id, i, vi_mode[i]);
            ret = -EINVAL;
            goto end;
        }

#ifdef PLAT_SPLIT
        if(vi_mode[i] == VCAP_VI_RUN_MODE_SPLIT) {
            split_cnt++;
            if(split_cnt >= 2) {
                vcap_err("[%d] only one vi can run in split mode\n", vcap_device.id);
                ret = -EINVAL;
                goto end;
            }
        }
#else
        if(vi_mode[i] == VCAP_VI_RUN_MODE_SPLIT) {
            vcap_err("[%d] vi#%d not support split mode!\n", vcap_device.id, i);
            ret = -EINVAL;
            goto end;
        }
#endif

        if((i == VCAP_CASCADE_VI_NUM) && (vi_mode[i] == VCAP_VI_RUN_MODE_2CH || vi_mode[i] == VCAP_VI_RUN_MODE_4CH)) {
            vcap_err("[%d] vi#%d is cascade, not support 2CH or 4CH mode!\n", vcap_device.id, i);
            ret = -EINVAL;
            goto end;
        }
    }

    /* check split */
#ifdef PLAT_SPLIT
    if(split_cnt) {
        if(!split_xy[0])
            split_xy[0] = 1;

        if(!split_xy[1])
            split_xy[1] = 1;

        if((split_xy[0] > VCAP_SPLIT_X_MAX) || (split_xy[0]*split_xy[1] > VCAP_SPLIT_CH_MAX)) {
            vcap_err("[%d] split parameter invalid!(X_MAX:%d SPLIT_MAX:%d)\n", vcap_device.id, VCAP_SPLIT_X_MAX, VCAP_SPLIT_CH_MAX);
            ret = -EINVAL;
            goto end;
        }
    }
    else {
        split_xy[0] = split_xy[1] = 1;
    }
#else
    split_xy[0] = 1;
    split_xy[1] = 1;
#endif

    vcap_dev_m_param.split_x = split_xy[0];
    vcap_dev_m_param.split_y = split_xy[1];

    for(i=0; i<VCAP_VI_MAX; i++) {
        vcap_dev_m_param.vi_mode[i]  = vi_mode[i];
        vcap_dev_m_param.vi_max_w[i] = ((vi_max_w[i] <= 0) || (vi_max_w[i] > 4096)) ? 0 : vi_max_w[i];
    }

    vcap_dev_m_param.sync_time_div = sync_time_div;
    vcap_dev_m_param.cap_md        = (cap_md <= 0) ? 0 : 1;
    vcap_dev_m_param.ext_irq_src   = (cap_mode == VCAP_CAP_MODE_LLI) ? ext_irq_src : 0;
    vcap_dev_m_param.grab_filter   = grab_filter;

#ifdef PLAT_OSD_COLOR_SCHEME
    if(accs_func < 0 || accs_func >= VCAP_ACCS_FUNC_MAX)
        vcap_dev_m_param.accs_func = VCAP_ACCS_FUNC_DISABLE;
    else
        vcap_dev_m_param.accs_func = accs_func;
#else
    vcap_dev_m_param.accs_func = VCAP_ACCS_FUNC_DISABLE;
#endif

    /* check source cropping rule */
    for(i=0; i<VCAP_SC_RULE_MAX; i++) {
        /* horizontal */
        if((hcrop_rule[i*2] <= 0) || (hcrop_rule[(i*2)+1] <= 0))
            vcap_dev_m_param.hcrop_rule[i].in_size = vcap_dev_m_param.hcrop_rule[i].out_size = 0;
        else {
            vcap_dev_m_param.hcrop_rule[i].in_size  = hcrop_rule[i*2];
            vcap_dev_m_param.hcrop_rule[i].out_size = hcrop_rule[(i*2)+1];

            if(vcap_dev_m_param.hcrop_rule[i].out_size > vcap_dev_m_param.hcrop_rule[i].in_size)
                vcap_dev_m_param.hcrop_rule[i].out_size = vcap_dev_m_param.hcrop_rule[i].in_size;
        }

        /* vertical */
        if((vcrop_rule[i*2] <= 0) || (vcrop_rule[(i*2)+1] <= 0))
            vcap_dev_m_param.vcrop_rule[i].in_size = vcap_dev_m_param.vcrop_rule[i].out_size = 0;
        else {
            vcap_dev_m_param.vcrop_rule[i].in_size  = vcrop_rule[i*2];
            vcap_dev_m_param.vcrop_rule[i].out_size = vcrop_rule[(i*2)+1];

            if(vcap_dev_m_param.vcrop_rule[i].out_size > vcap_dev_m_param.vcrop_rule[i].in_size)
                vcap_dev_m_param.vcrop_rule[i].out_size = vcap_dev_m_param.vcrop_rule[i].in_size;
        }
    }

    /* check minimal scaling up line number */
    vcap_dev_m_param.vup_min = (vup_min < 0) ? 0 : vup_min;

    vcap_device.dev.platform_data = (void *)&vcap_dev_m_param;

    if(cap_mode == VCAP_CAP_MODE_LLI)
        vcap_device.name = vcap_lli_str;
    else
        vcap_device.name = vcap_ssf_str;

    if (platform_device_register(&vcap_device)) {
        vcap_err("register device(%d) failed!", vcap_device.id);
        ret = -ENODEV;
    }

end:
    return ret;
}

static void __exit vcap_dev_exit(void)
{
    platform_device_unregister(&vcap_device);
}

module_init(vcap_dev_init);
module_exit(vcap_dev_exit);

MODULE_DESCRIPTION("Grain Media Video Capture300 Device");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
