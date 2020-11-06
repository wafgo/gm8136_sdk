/**
 * @file vcap_lib.c
 *  vcap300 device control library
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.77 $
 * $Date: 2014/12/16 09:26:17 $
 *
 * ChangeLog:
 *  $Log: vcap_lib.c,v $
 *  Revision 1.77  2014/12/16 09:26:17  jerson_l
 *  1. add md gaussian value check for detect gaussian vaule update error problem when md region switched.
 *  2. add md get all channel tamper status api.
 *
 *  Revision 1.76  2014/12/02 07:58:34  jerson_l
 *  1. force to adjust md x and y block number to prevent md region over image size.
 *
 *  Revision 1.75  2014/11/28 08:52:17  jerson_l
 *  1. support frame 60I feature.
 *
 *  Revision 1.74  2014/11/27 01:59:21  jerson_l
 *  1. add heartbeat timer for watch capture hardware enable status to prevent capture
 *     auto shut down.
 *
 *  Revision 1.73  2014/11/11 05:37:24  jerson_l
 *  1. support group job pending mechanism
 *
 *  Revision 1.72  2014/10/30 01:47:32  jerson_l
 *  1. support vi 2ch dual edge hybrid channel mode.
 *  2. support grab channel horizontal blanking data.
 *
 *  Revision 1.71  2014/09/05 02:48:53  jerson_l
 *  1. support osd force frame mode for GM8136
 *  2. support osd edge smooth edge mode for GM8136
 *  3. support osd auto color change scheme for GM8136
 *
 *  Revision 1.70  2014/08/27 08:40:47  jerson_l
 *  1. mask pixel lack status report in GM8136 ISP interface
 *
 *  Revision 1.69  2014/08/27 06:43:16  jerson_l
 *  1. fix image border width >= 8 not work problem in GM8136
 *
 *  Revision 1.68  2014/08/27 02:02:03  jerson_l
 *  1. support GM8136 platform VCAP300 feature
 *
 *  Revision 1.67  2014/08/15 03:02:06  jerson_l
 *  1. change alignment caculation mechanism for osd_mask window
 *
 *  Revision 1.66  2014/07/02 08:53:04  jerson_l
 *  1. disable 2frame_mode to grab 2 frame mechanism if vi speed is VCAP_VI_SPEED_2P
 *
 *  Revision 1.65  2014/06/05 02:26:34  jerson_l
 *  1. support osd_mask non-align
 *
 *  Revision 1.64  2014/05/22 06:33:07  jerson_l
 *  1. fix complie warning
 *
 *  Revision 1.63  2014/05/22 06:23:40  jerson_l
 *  1. add ch14 sc rolling eco check
 *
 *  Revision 1.62  2014/05/13 04:01:06  jerson_l
 *  1. change time base frame rate control to frame base
 *
 *  Revision 1.61  2014/05/05 09:31:19  jerson_l
 *  1. support vi signal probe for vi signal debug
 *
 *  Revision 1.60  2014/04/28 06:09:57  jerson_l
 *  1. fix array bounds compile warning message
 *
 *  Revision 1.59  2014/04/25 07:39:35  jerson_l
 *  1. correct GM8287 VI#3 channel#14 sc rolling to 1024 for 2ch byte interleave mode(hardware issue)
 *
 *  Revision 1.58  2014/04/21 04:11:11  jerson_l
 *  1. modify vg_info and ability read proc node from seq_read to read_proc to prevent kernel bug
 *
 *  Revision 1.57  2014/04/14 02:51:37  jerson_l
 *  1. support grab_filter to filter fail frame drop rule
 *
 *  Revision 1.56  2014/04/07 03:40:06  jerson_l
 *  1. fix image paste to wrong position when enable auto aspect ratio and target cropping
 *
 *  Revision 1.55  2014/03/26 05:48:16  jerson_l
 *  1. add vi_max_w module parameter for control sd line buffer calculation mechanism
 *
 *  Revision 1.54  2014/03/20 05:59:01  jerson_l
 *  1. add md fatal reset debug command
 *
 *  Revision 1.53  2014/03/11 13:52:11  jerson_l
 *  1. error message output with ratelimit control in vcap_dev_param_setup api
 *
 *  Revision 1.52  2014/02/06 02:08:33  jerson_l
 *  1. fix sometime enable/disable mask window fail problem
 *
 *  Revision 1.51  2014/01/24 07:33:10  jerson_l
 *  1. support isp dynamic switch data range
 *
 *  Revision 1.50  2014/01/20 02:54:16  jerson_l
 *  1. add capture reset workaround for md error and dma overflow cause dma channel not work problem
 *
 *  Revision 1.49  2013/12/23 03:55:35  jerson_l
 *  1. add no job alarm count debug message
 *
 *  Revision 1.48  2013/12/20 05:23:47  jerson_l
 *  1. correct frame rate check procedure if no apply frame rate control
 *
 *  Revision 1.47  2013/12/12 02:26:56  jerson_l
 *  1. add default pclk divide value to 0x8
 *
 *  Revision 1.46  2013/11/20 09:00:28  jerson_l
 *  1. fix frame mode enable v_flip cause lcd display top/bottom sequence incorrect problem
 *
 *  Revision 1.45  2013/11/06 08:47:25  jerson_l
 *  1. output property src_interlace=2 if 2frames mode as progressive top/bottom fromat.
 *
 *  Revision 1.44  2013/11/06 01:39:47  jerson_l
 *  1. support 2Frame mode as progressive top/bottom format
 *
 *  Revision 1.43  2013/10/28 07:40:02  jerson_l
 *  1. set ISP interface as progressive mode
 *
 *  Revision 1.42  2013/10/17 04:16:34  jerson_l
 *  1. support TC_OSD_SWAP and TC_X_Align=2 hardware feature.
 *
 *  Revision 1.41  2013/10/14 05:28:39  jerson_l
 *  1. add VI_MD_Win_X_Num and MD_IMG_SRC hardware ability to ability display table
 *  2. support src_type output property
 *
 *  Revision 1.40  2013/10/14 04:02:51  jerson_l
 *  1. support GM8139 platform
 *
 *  Revision 1.39  2013/10/04 02:47:12  jerson_l
 *  1. add scaler up/dwon ability information to ability display table
 *
 *  Revision 1.38  2013/10/01 06:17:35  jerson_l
 *  1. add VCH information to vg_info display table
 *
 *  Revision 1.37  2013/09/05 02:33:16  jerson_l
 *  1. fix osd/mark/osd_mask window position incorrect problem in field mode buffer
 *
 *  Revision 1.36  2013/08/26 12:20:15  jerson_l
 *  1. show mask border config if mask window support hollow feature
 *
 *  Revision 1.35  2013/08/23 09:16:59  jerson_l
 *  1. support specify channel timeout threshold for different front_end device source
 *
 *  Revision 1.34  2013/08/13 07:31:21  jerson_l
 *  1. provide clear hardware frame count monitor counter in set frame count monitor channel api
 *
 *  Revision 1.33  2013/07/23 05:58:20  jerson_l
 *  1. clear compile warning message if no define PLAT_CH_MODE_SEL
 *
 *  Revision 1.32  2013/07/16 03:23:45  jerson_l
 *  1. support scaler presmooth control
 *
 *  Revision 1.31  2013/07/08 03:42:23  jerson_l
 *  1. adjust sd_base assignment
 *  2. support auto osd image border property
 *
 *  Revision 1.30  2013/06/24 08:36:17  jerson_l
 *  1. support md grouping
 *
 *  Revision 1.29  2013/06/18 02:17:41  jerson_l
 *  1. modify md buffer mechanism
 *
 *  Revision 1.28  2013/06/05 04:00:57  jerson_l
 *  1. fix osd and mark output position incorrect problem if buffer type is field
 *
 *  Revision 1.27  2013/06/04 11:41:05  jerson_l
 *  1. support auto aspect ratio of output image
 *  2. support windoiw size auto adjustment for osd_mask window
 *
 *  Revision 1.26  2013/05/27 05:55:55  jerson_l
 *  1. fix osd marquee and image border config not sync problem when channel from idle to start
 *  2. support target cropping
 *  3. modify osd and marker window align control base on target crop window
 *
 *  Revision 1.25  2013/05/10 05:47:42  jerson_l
 *  1. add window align config display at osd and mark config table
 *  2. fix osd and mark window config not sync problem when channel from stop to start
 *
 *  Revision 1.24  2013/05/08 08:18:16  jerson_l
 *  1. add sdi8bit format check, cascade not support this fromat
 *
 *  Revision 1.23  2013/05/08 03:34:40  jerson_l
 *  1. fix frame rate miss problem when p2i enable
 *  2. support cropping rule to apply to all channel source cropping
 *  3. add resolution information to vg_info display table
 *
 *  Revision 1.22  2013/04/30 10:01:05  jerson_l
 *  1. support SDI8BIT format for capture hardware revision#1
 *  2. modify motion block number limitation
 *
 *  Revision 1.21  2013/04/26 05:27:24  jerson_l
 *  1. fix frame control miss ratio problem
 *
 *  Revision 1.20  2013/04/22 09:05:17  jerson_l
 *  1. fix fps_ratio parameter setup incorrect problem
 *
 *  Revision 1.19  2013/04/08 12:15:04  jerson_l
 *  1. add v_flip/h_flip set and get export api
 *
 *  Revision 1.18  2013/03/29 03:37:07  jerson_l
 *  1. add md x_size and y_size property check
 *
 *  Revision 1.17  2013/03/26 02:40:38  jerson_l
 *  1. add 96 motion block event bug check
 *  2. add cap_md module parameter check to off/on MD function support
 *  3. fix hsize of scaler path calcaulaion
 *
 *  Revision 1.16  2013/03/19 05:47:34  jerson_l
 *  1. add dma field offset shift base hardware ECO revision check
 *
 *  Revision 1.15  2013/03/18 05:51:45  jerson_l
 *  1. add diagnostic proc node for error log monitor
 *  2. add dbg_mode proc node for disable/enable error message output control
 *  3. add md_dxdy dynamic switch base on md window x and y size
 *  4. add fatal software reset procedure for recover capture if capture hardware meet fatal error
 *  5. add hw_delay and no_job error notify
 *
 *  Revision 1.14  2013/03/11 08:14:47  jerson_l
 *  1. add P2I support
 *  2. add Cascade P2I workaround, path#3 as workaround path
 *
 *  Revision 1.13  2013/03/05 08:12:19  jerson_l
 *  1. fix PAL mode software frame rate control incorrect problem
 *
 *  Revision 1.12  2013/02/18 04:50:44  jerson_l
 *  1. fix md disable via other path if one path enable md
 *
 *  Revision 1.11  2013/02/07 12:30:36  jerson_l
 *  1. fixed cascade sd_hratio not apply to source crop problem
 *
 *  Revision 1.10  2013/01/31 05:39:10  jerson_l
 *  1. change VI TDM mode display message
 *
 *  Revision 1.9  2013/01/16 07:23:54  jerson_l
 *  1. In interlace mode the md region y start and size must divide 2.(property base on frame size)
 *
 *  Revision 1.8  2013/01/15 09:45:10  jerson_l
 *  1. replace platform limitation macro string
 *
 *  Revision 1.7  2013/01/02 07:34:58  jerson_l
 *  1. support osd/mark window auto align feature
 *
 *  Revision 1.6  2012/12/18 12:01:35  jerson_l
 *  1. add debugging proc node
 *
 *  Revision 1.5  2012/12/11 10:37:15  jerson_l
 *  1. add FCS, Denoise, Sharpness, OSD, Mark, Mask, MD control procedure
 *
 *  Revision 1.4  2012/12/03 08:01:44  jerson_l
 *  1. add device vi mode getting api
 *
 *  Revision 1.3  2012/11/01 06:23:11  waynewei
 *  1.sync with videograph_20121101
 *  2.fixed for build fail due to include path
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#include "bits.h"
#include "vcap_plat.h"
#include "vcap_proc.h"
#include "vcap_dev.h"
#include "vcap_fcs.h"
#include "vcap_dn.h"
#include "vcap_presmo.h"
#include "vcap_sharp.h"
#include "vcap_md.h"
#include "vcap_osd.h"
#include "vcap_mask.h"
#include "vcap_mark.h"
#include "vcap_input.h"
#include "vcap_vg.h"
#include "vcap_dbg.h"
#include "frammap_if.h"

static int proc_dump_ch[VCAP_DEV_MAX]    = {[0 ... (VCAP_DEV_MAX - 1)] = 0};
static int proc_grab_hb_ch[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = 0};

/* Device common proc node */
static struct proc_dir_entry *vcap_dev_proc_root[VCAP_DEV_MAX]        = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_dbg_mode[VCAP_DEV_MAX]    = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_ability[VCAP_DEV_MAX]     = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_dump_reg[VCAP_DEV_MAX]    = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_dump_ch[VCAP_DEV_MAX]     = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_status[VCAP_DEV_MAX]      = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_lli_info[VCAP_DEV_MAX]    = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_dump_lli[VCAP_DEV_MAX]    = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_vg_info[VCAP_DEV_MAX]     = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_jobq[VCAP_DEV_MAX]        = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_crop_rule[VCAP_DEV_MAX]   = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_grab_filter[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_vi_probe[VCAP_DEV_MAX]    = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_grab_hblank[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_vup_min[VCAP_DEV_MAX]     = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
#ifdef PLAT_SPLIT
static struct proc_dir_entry *vcap_dev_proc_dump_split[VCAP_DEV_MAX]  = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
#endif
#ifdef CFG_HEARTBEAT_TIMER
static struct proc_dir_entry *vcap_dev_proc_heartbeat[VCAP_DEV_MAX]   = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
#endif

/* Device config proc node */
static struct proc_dir_entry *vcap_dev_proc_cfg_root[VCAP_DEV_MAX]    = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_cfg_global[VCAP_DEV_MAX]  = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_cfg_channel[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_cfg_mask[VCAP_DEV_MAX]    = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_cfg_mark[VCAP_DEV_MAX]    = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_cfg_osd[VCAP_DEV_MAX]     = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};

/* Device diagnostic proc node */
static struct proc_dir_entry *vcap_dev_proc_diag_root[VCAP_DEV_MAX]     = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_diag_global[VCAP_DEV_MAX]   = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_diag_channel[VCAP_DEV_MAX]  = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_diag_framecnt[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_dev_proc_diag_clear[VCAP_DEV_MAX]    = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};

static void *g_dev_info[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};

void vcap_dev_set_dev_info(int id, void *info)
{
    if(id < VCAP_DEV_MAX)
        g_dev_info[id] = info;
}

void *vcap_dev_get_dev_info(int id)
{
    if(id < VCAP_DEV_MAX)
        return g_dev_info[id];
    else
        return NULL;
}

void *vcap_dev_get_dev_proc_root(int id)
{
    return (void *)vcap_dev_proc_root[id];
}

static void vcap_dev_dump_reg(struct vcap_dev_info_t *pdev_info, struct seq_file *sfile)
{
#define VCAP_DEV_REG_SIZE   0x600
    int i, j;
    volatile u32 *preg = (volatile u32 *)(pdev_info->vbase + VCAP_VI_OFFSET(0));
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    for (i=0, j=0; i<(VCAP_DEV_REG_SIZE>>2); i++, j++, preg++) {
        if((i>=(0x60>>2) && i<(0x100>>2))    || ((i>=(0x210>>2) && i<(0x300>>2))) ||
           ((i>=(0x340>>2) && i<(0x400>>2))) || ((i>=(0x480>>2) && i<(0x500>>2))))
            continue;

        if(mbuf_point>=VCAP_DEV_MSG_THRESHOLD){
            if(sfile)
                seq_printf(sfile, mbuf);
            else
                printk(mbuf);
            mbuf_point = 0;
        }

        if (j == 0 || j >= 4) {
            mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
            mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "[%d]:0x%04x:", pdev_info->index, (VCAP_VI_OFFSET(0) + (i<<2)));
            j = 0;
        }
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "  %08x", *preg);
    }

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    if(sfile)
        seq_printf(sfile, mbuf);
    else
        printk(mbuf);
}

static void vcap_dev_dump_ch(struct vcap_dev_info_t *pdev_info, int ch, struct seq_file *sfile)
{
    int i, j;
    volatile u32 *preg;
    u32  ch_offset;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    if(ch == VCAP_CASCADE_CH_NUM)
        ch_offset = VCAP_CAS_SUBCH_OFFSET(0);
    else
        ch_offset = VCAP_CH_REG_OFFSET(ch);

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n=== CH#%d ===", ch);

    preg = (volatile u32 *)(pdev_info->vbase + ch_offset);
    for (i=0, j=0; i<(VCAP_CH_REG_SIZE>>2); i++, j++, preg++) {
        if(mbuf_point>=VCAP_DEV_MSG_THRESHOLD){
            if(sfile)
                seq_printf(sfile, mbuf);
            else
                printk(mbuf);
            mbuf_point = 0;
        }

        if (j == 0 || j >= 4) {
            mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
            mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "[%d]:0x%04x:", pdev_info->index, (ch_offset + (i<<2)));
            j = 0;
        }
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "  %08x", *preg);
    }

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    if(sfile)
        seq_printf(sfile, mbuf);
    else
        printk(mbuf);
}

#ifdef PLAT_SPLIT
static void vcap_dev_dump_split(struct vcap_dev_info_t *pdev_info, struct seq_file *sfile)
{
#define VCAP_DEV_SPLIT_REG_SIZE   (0x18*VCAP_SPLIT_CH_MAX)
    int i, j;
    volatile u32 *preg = (volatile u32 *)(pdev_info->vbase + VCAP_SPLIT_CH_OFFSET(0, 0));
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    for (i=0, j=0; i<(VCAP_DEV_SPLIT_REG_SIZE>>2); i++, j++, preg++) {
        if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD){
            if(sfile)
                seq_printf(sfile, mbuf);
            else
                printk(mbuf);
            mbuf_point = 0;
        }

        if (j == 0 || j >= 4) {
            mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
            mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "[%d]:0x%04x:", pdev_info->index, (VCAP_SPLIT_CH_OFFSET(0, 0) + (i<<2)));
            j = 0;
        }
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "  %08x", *preg);
    }

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    if(sfile)
        seq_printf(sfile, mbuf);
    else
        printk(mbuf);
}
#endif

static void vcap_dev_dump_lli(struct vcap_dev_info_t *pdev_info, int ch, struct seq_file *sfile)
{
    int i, j;
    int cnt, up_cnt, null_cnt;
    volatile u32 *ptr;
    struct vcap_lli_table_t *curr_table, *next_table;
    struct vcap_lli_table_t *curr_up_table, *next_up_table;
    struct vcap_lli_table_t *null_table[VCAP_LLI_PATH_NULL_TAB_MAX];
    struct vcap_lli_t *plli = &pdev_info->lli;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "[CH#%02d]\n", ch);

    /* List Null Table */
    for(i=0; i<VCAP_SCALER_MAX; i++) {
        if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD){
            if(sfile)
                seq_printf(sfile, mbuf);
            else
                printk(mbuf);
            mbuf_point = 0;
        }

        null_cnt = 0;
        list_for_each_entry_safe(curr_table, next_table, &plli->ch[ch].path[i].null_head, list) {
            null_table[null_cnt++] = curr_table;
        }

        /* List Normal Table */
        cnt = 0;
        mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, ">> Path[%d] Link List Table ========\n", i);
        list_for_each_entry_safe(curr_table, next_table, &plli->ch[ch].path[i].ll_head, list) {
            if(null_cnt) {
                ptr = (volatile u32 *)null_table[cnt]->addr;
                mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "[N : 0x%08x]\n", (u32)ptr);
                mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\t(00) 0x%08x %08x\n", ptr[1], ptr[0]);
                null_cnt--;
            }

            up_cnt = 0;
            ptr = (volatile u32 *)curr_table->addr;
            mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "[%02d: 0x%08x]\n", cnt++, (u32)ptr);
            for(j=0; j<curr_table->curr_entry; j++) {
                mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\t(%02d) 0x%08x %08x\n", j, ptr[(j*2)+1], ptr[j*2]);
            }

            /* Update Table */
            list_for_each_entry_safe(curr_up_table, next_up_table, &curr_table->update_head, list) {
                ptr = (volatile u32 *)curr_up_table->addr;
                mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\t[%02d: 0x%08x]\n", up_cnt++, (u32)ptr);
                for(j=0; j<curr_up_table->curr_entry; j++) {
                    mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\t\t(%02d) 0x%08x %08x\n", j, ptr[(j*2)+1], ptr[j*2]);
                }
            }
        }

        if(null_cnt && (cnt < VCAP_LLI_PATH_NULL_TAB_MAX)) {
            ptr = (volatile u32 *)null_table[cnt]->addr;
            mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "[N : 0x%08x]\n", (u32)ptr);
            mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\t(00) 0x%08x %08x\n", ptr[1], ptr[0]);
            null_cnt--;
        }
    }

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    if(sfile)
        seq_printf(sfile, mbuf);
    else
        printk(mbuf);
}

static void vcap_dev_dump_jobq(struct vcap_dev_info_t *pdev_info, int ch, struct seq_file *sfile)
{
    int i, j, job_cnt;
    void *job[VCAP_JOB_MAX];
    struct vcap_path_t *ppath_info;
    struct vcap_lli_table_t   *curr_table, *next_table;
    struct vcap_vg_job_item_t *job_item, *root_job_item;
    struct vcap_lli_t *plli = &pdev_info->lli;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "[CH#%02d]\n", ch);

    mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Path  Job_ID\n");
    mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                          "====================================================================");

    for(i=0; i<VCAP_SCALER_MAX; i++) {
        if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD){
            if(sfile)
                seq_printf(sfile, mbuf);
            else
                printk(mbuf);
            mbuf_point = 0;
        }

        ppath_info = &pdev_info->ch[ch].path[i];

        /* Ongoing Job */
        mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n%-6d", i);
        mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Ongoing=> ");
        list_for_each_entry_safe(curr_table, next_table, &plli->ch[ch].path[i].ll_head, list) {
            if(curr_table->type == VCAP_LLI_TAB_TYPE_NORMAL) {
                job_item      = (struct vcap_vg_job_item_t *)curr_table->job;
                root_job_item = (struct vcap_vg_job_item_t *)job_item->root_job;
                mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                      "%d[r:%d] ", job_item->job_id, root_job_item->job_id);
            }
            else if(curr_table->type == VCAP_LLI_TAB_TYPE_DUMMY) {
                mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "[dummy] ");
            }
            else if(curr_table->type == VCAP_LLI_TAB_TYPE_DUMMY_LOOP) {
                mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "[dummy_loop] ");
            }
            else if(curr_table->type == VCAP_LLI_TAB_TYPE_HBLANK) {
                mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "[hblank] ");
            }
        }

        /* Pending Job */
        mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n      Pending=> ");
        if(ppath_info->active_job) {
            job_item      = (struct vcap_vg_job_item_t *)ppath_info->active_job;
            root_job_item = (struct vcap_vg_job_item_t *)job_item->root_job;
            mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                  ((job_item->status == VCAP_VG_JOB_STATUS_KEEP) ? "%d[r:%d](K) " : "%d[r:%d] "),
                                  job_item->job_id, root_job_item->job_id);
        }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
        /* peek job from fifo queue without remove job */
        job_cnt = kfifo_out_peek(ppath_info->job_q, (void *)&job[0], sizeof(unsigned int)*VCAP_JOB_MAX)/sizeof(unsigned int);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
        /* peek job from fifo queue without remove job */
        job_cnt = kfifo_out_peek(ppath_info->job_q, (void *)&job[0], sizeof(unsigned int)*VCAP_JOB_MAX, 0)/sizeof(unsigned int);
#else
        /* get job from fifo queue */
        job_cnt = 0;
        for(j=0; j<VCAP_JOB_MAX; j++) {
            if(kfifo_get(ppath_info->job_q, (void *)&job[j], sizeof(unsigned int))< sizeof(unsigned int))
                break;
            job_cnt++;
        }

        /* put job back to fifo queue */
        for(j=0; j<job_cnt; j++) {
            kfifo_put(ppath_info->job_q, (unsigned char *)&job[j], sizeof(unsigned int));
        }
#endif

        for(j=0; j<job_cnt; j++) {
            job_item      = (struct vcap_vg_job_item_t *)job[j];
            root_job_item = (struct vcap_vg_job_item_t *)job_item->root_job;
            mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                  ((job_item->status == VCAP_VG_JOB_STATUS_KEEP) ? "%d[r:%d](K) " : "%d[r:%d] "),
                                  job_item->job_id, root_job_item->job_id);
            if(((j+1)%5) == 0)
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n                ");
        }
    }

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    if(sfile)
        seq_printf(sfile, mbuf);
    else
        printk(mbuf);
}

static void vcap_dev_dump_global_cfg(struct vcap_dev_info_t *pdev_info, struct seq_file *sfile)
{
    int  i;
    u32  tmp = 0;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;
    char *ch_mode_msg[]   = {"Normal", "N/A", "16CH", "32CH"};
    char *cap_mode_msg[]  = {"Single Fire", "Link List", "Continue"};
#ifdef PLAT_OSD_COLOR_SCHEME
    char *accs_func_msg[] = {"Disable", "Half Mark Memory", "Full Mark Memory"};
#endif

    /* display global config */
    mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                          ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                          "CH_Mode      : %s\n", ch_mode_msg[pdev_info->ch_mode]);
    mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                          "Capture_Mode : %s\n", cap_mode_msg[pdev_info->dev_type]);
    mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                          "SD_Base      : ");
    for(i=0; i<VCAP_VI_MAX; i++) {
        if(i == VCAP_CASCADE_VI_NUM) {
            tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x58));
            mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-5d ", (tmp&0xffff));
        }
        else {
            tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x48 + ((i/2)*4)));
            mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-5d ", ((tmp>>((i%2)*16))&0xffff));
        }
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                          "Frame_Cnt_CH : %d\n", vcap_dev_get_frame_cnt_monitor_ch(pdev_info));

#ifdef PLAT_MD_GROUPING
    mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                          "MD_GROUP_MAX : %d (%d)\n", pdev_info->md_grp_max, pdev_info->md_enb_grp);
#endif

#ifdef PLAT_OSD_COLOR_SCHEME
    mbuf_point+= snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                          "OSD_ACCS_FUNC: %s\n", accs_func_msg[(pdev_info->m_param)->accs_func]);
#endif

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    if(sfile)
        seq_printf(sfile, mbuf);
    else
        printk(mbuf);
}

static void vcap_dev_dump_channel_cfg(struct vcap_dev_info_t *pdev_info, int ch, struct seq_file *sfile)
{
    u32  tmp;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;
    int  vi_idx     = CH_TO_VI(ch);
    int  vi_ch      = SUBCH_TO_VICH(ch);
    char *vi_format_msg[]    = {"BT656", "BT1120", "SDI8BIT", "RGB888", "BT601_8BIT", "BT601_16BIT", "ISP"};
    char *vi_prog_msg[]      = {"Interlace", "Progressive"};
    char *vi_tdm_msg[]       = {"Bypass", "Frame_Interleave", "2CH_Byte_Interleave", "4CH_Byte_Interleave", "2CH_Byte_Interleave_Hybrid"};
    char *vi_cap_style_msg[] = {"Skip_All", "Anyone", "Skip_All", "Anyone", "Odd_Field_Only", "Even_Field_Only",
                                "Odd_Field_First", "Even_Field_First"};
    struct vcap_ch_param_t *pcurr_param = &pdev_info->ch[ch].param;

    /* display channel config */
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "[CH#%02d]\n", ch);

    /* VI */
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "VI_Format    : %s\n", vi_format_msg[pdev_info->vi[vi_idx].format]);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "VI_Prog      : %s\n", vi_prog_msg[pdev_info->vi[vi_idx].prog]);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "VI_TDM       : %s\n", vi_tdm_msg[pdev_info->vi[vi_idx].tdm_mode]);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "VI_Cap_Style : %s\n", vi_cap_style_msg[pdev_info->vi[vi_idx].cap_style]);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "VI_Src_W     : %d\n", pdev_info->vi[vi_idx].src_w);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "VI_Src_H     : %d\n", pdev_info->vi[vi_idx].src_h);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "VI_Grab_Mode : %s\n", ((pdev_info->vi[vi_idx].grab_mode == VCAP_VI_GRAB_MODE_HBLANK) ? "HBLANK" : "Normal"));

    /* Channel Parameter */
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "CH_Grab_HB   : %s\n", ((pdev_info->vi[vi_idx].grab_hblank & (0x1<<vi_ch)) ? "On" : "Off"));
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "CH_Prog      : %s\n", vi_prog_msg[pdev_info->vi[vi_idx].ch_param[vi_ch].prog]);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "CH_Src_W     : %d\n", pdev_info->vi[vi_idx].ch_param[vi_ch].src_w);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "CH_Src_H     : %d\n", pdev_info->vi[vi_idx].ch_param[vi_ch].src_h);

    /* Source Crop */
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "SC_Type      : %s\n", (pdev_info->ch[ch].sc_type == VCAP_SC_TYPE_SPLIT) ? "Split" : "Normal");
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "SC_Roing     : %d\n", (1024<<pdev_info->ch[ch].sc_rolling));
    if((pdev_info->ch[ch].sc_type == VCAP_SC_TYPE_SPLIT) && (ch == pdev_info->split.ch)) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "Split_X_NUM  : %d\n", pdev_info->split.x_num);
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "Split_Y_NUM  : %d\n", pdev_info->split.y_num);
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "Split_W      : %d\n", pcurr_param->sc.width);
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "Split_H      : %d\n", pcurr_param->sc.height);
    }
    else {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "SC_X         : %d\n", pcurr_param->sc.x_start);
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "SC_Y         : %d\n", pcurr_param->sc.y_start);
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "SC_W         : %d\n", pcurr_param->sc.width);
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "SC_H         : %d\n", pcurr_param->sc.height);
    }

    /* Flip */
    if(ch == VCAP_CASCADE_CH_NUM)
        tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_SUBCH_OFFSET(0x08));
    else
        tmp = VCAP_REG_RD(pdev_info, VCAP_CH_SUBCH_OFFSET(ch, 0x08));
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "H_Flip       : %d\n", ((tmp&BIT28)  ? 1 : 0));
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "V_Flip       : %d\n", ((tmp&BIT29)  ? 1 : 0));

    /* Scaler */
    if(ch == VCAP_CASCADE_CH_NUM)
        tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_SUBCH_OFFSET(0x00));
    else
        tmp = VCAP_REG_RD(pdev_info, VCAP_CH_SUBCH_OFFSET(ch, 0x00));

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "Scaler_Enable: %-8d %-8d %-8d %-8d\n",
                           ((tmp&BIT1)  ? 1 : 0),
                           ((tmp&BIT9)  ? 1 : 0),
                           ((tmp&BIT17) ? 1 : 0),
                           ((tmp&BIT25) ? 1 : 0));
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "Scaler_Bypass: %-8d %-8d %-8d %-8d\n",
                           ((tmp&BIT0)  ? 1 : 0),
                           ((tmp&BIT8)  ? 1 : 0),
                           ((tmp&BIT16) ? 1 : 0),
                           ((tmp&BIT24) ? 1 : 0));
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "Scaler_W     : %-8d %-8d %-8d %-8d\n",
                           pcurr_param->sd[0].out_w,
                           pcurr_param->sd[1].out_w,
                           pcurr_param->sd[2].out_w,
                           pcurr_param->sd[3].out_w);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "Scaler_H     : %-8d %-8d %-8d %-8d\n",
                           pcurr_param->sd[0].out_h,
                           pcurr_param->sd[1].out_h,
                           pcurr_param->sd[2].out_h,
                           pcurr_param->sd[3].out_h);

    /* Target Crop */
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "TC_Enable    : %-8d %-8d %-8d %-8d\n",
                           ((tmp&BIT2)  ? 1 : 0),
                           ((tmp&BIT10) ? 1 : 0),
                           ((tmp&BIT18) ? 1 : 0),
                           ((tmp&BIT26) ? 1 : 0));
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "TC_X         : %-8d %-8d %-8d %-8d\n",
                           pcurr_param->tc[0].x_start,
                           pcurr_param->tc[1].x_start,
                           pcurr_param->tc[2].x_start,
                           pcurr_param->tc[3].x_start);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "TC_Y         : %-8d %-8d %-8d %-8d\n",
                           pcurr_param->tc[0].y_start,
                           pcurr_param->tc[1].y_start,
                           pcurr_param->tc[2].y_start,
                           pcurr_param->tc[3].y_start);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "TC_W         : %-8d %-8d %-8d %-8d\n",
                           pcurr_param->tc[0].width,
                           pcurr_param->tc[1].width,
                           pcurr_param->tc[2].width,
                           pcurr_param->tc[3].width);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "TC_H         : %-8d %-8d %-8d %-8d\n",
                           pcurr_param->tc[0].height,
                           pcurr_param->tc[1].height,
                           pcurr_param->tc[2].height,
                           pcurr_param->tc[3].height);

    /* Grab Field Pair */
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "Grab_Pair    : %-8d %-8d %-8d %-8d\n",
                           ((tmp&BIT3)  ? 1 : 0),
                           ((tmp&BIT11) ? 1 : 0),
                           ((tmp&BIT19) ? 1 : 0),
                           ((tmp&BIT27) ? 1 : 0));

    /* DMA Channel */
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "DMA_Channel  : %-8d %-8d %-8d %-8d\n",
                           ((tmp&BIT4)  ? 1 : 0),
                           ((tmp&BIT12) ? 1 : 0),
                           ((tmp&BIT20) ? 1 : 0),
                           ((tmp&BIT28) ? 1 : 0));

    /* Frame to Field */
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "Frame2Field  : %-8d %-8d %-8d %-8d\n",
                           ((tmp&BIT5)  ? 1 : 0),
                           ((tmp&BIT13) ? 1 : 0),
                           ((tmp&BIT21) ? 1 : 0),
                           ((tmp&BIT29) ? 1 : 0));

    /* P2I */
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "P2I          : %-8d %-8d %-8d %-8d\n",
                           ((tmp&BIT6)  ? 1 : 0),
                           ((tmp&BIT14) ? 1 : 0),
                           ((tmp&BIT22) ? 1 : 0),
                           ((tmp&BIT30) ? 1 : 0));

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    if(sfile)
        seq_printf(sfile, mbuf);
    else
        printk(mbuf);
}

static void vcap_dev_dump_mask_cfg(struct vcap_dev_info_t *pdev_info, int ch, struct seq_file *sfile)
{
    int  i;
    u32  tmp;
    struct vcap_mask_win_t mask_win[VCAP_MASK_WIN_MAX];
    struct vcap_mask_border_t mask_border[VCAP_MASK_WIN_MAX];
    VCAP_MASK_ALPHA_T mask_alpha[VCAP_MASK_WIN_MAX];
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    /* get mask config from hardware register */
    if(ch == VCAP_CASCADE_CH_NUM)
        tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_SUBCH_OFFSET(0x04));
    else
        tmp = VCAP_REG_RD(pdev_info, VCAP_CH_SUBCH_OFFSET(ch, 0x04));

    for(i=0; i<VCAP_MASK_WIN_MAX; i++) {
        vcap_mask_get_win(pdev_info, ch, i, &mask_win[i]);
        vcap_mask_get_alpha(pdev_info, ch, i, &mask_alpha[i]);
        vcap_mask_get_border(pdev_info, ch, i, &mask_border[i]);
    }

    /* display mask config */
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "[CH#%02d]\n", ch);

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mask_Enable  : ");
    for(i=0; i<VCAP_MASK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", ((tmp>>i) & 0x1));
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mask_X       : ");
    for(i=0; i<VCAP_MASK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", mask_win[i].x_start);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mask_Y       : ");
    for(i=0; i<VCAP_MASK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", mask_win[i].y_start);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mask_W       : ");
    for(i=0; i<VCAP_MASK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", mask_win[i].width);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mask_H       : ");
    for(i=0; i<VCAP_MASK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", mask_win[i].height);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mask_Color   : ");
    for(i=0; i<VCAP_MASK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", mask_win[i].color);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mask_Alpha   : ");
    for(i=0; i<VCAP_MASK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", mask_alpha[i]);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

#ifdef PLAT_MASK_WIN_HOLLOW
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mask_Border_W: ");
    for(i=0; i<VCAP_MASK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", mask_border[i].width);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mask_Border_T: ");
    for(i=0; i<VCAP_MASK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8s ",
                               (mask_border[i].type == VCAP_MASK_BORDER_TYPE_TRUE) ? "True" : "Hollow");
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
#endif

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    if(sfile)
        seq_printf(sfile, mbuf);
    else
        printk(mbuf);
}

static void vcap_dev_dump_mark_cfg(struct vcap_dev_info_t *pdev_info, int ch, struct seq_file *sfile)
{
    int  i;
    u32  tmp;
    struct vcap_mark_win_t mark_win[VCAP_MARK_WIN_MAX];
    int x_dim[VCAP_MARK_WIN_MAX];
    int y_dim[VCAP_MARK_WIN_MAX];
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;
    char *align_msg[] = {"None", "TOP_L", "TOP_C", "TOP_R", "BOTTOM_L", "BOTTOM_C", "BOTTOM_R", "CENTER"};

    /* get mark config from hardware register */
    if(ch == VCAP_CASCADE_CH_NUM)
        tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_SUBCH_OFFSET(0x08));
    else
        tmp = VCAP_REG_RD(pdev_info, VCAP_CH_SUBCH_OFFSET(ch, 0x08));

    for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
        vcap_mark_get_win(pdev_info, ch, i, &mark_win[i]);
        vcap_mark_get_dim(pdev_info, ch, i, &x_dim[i], &y_dim[i]);
    }

    /* display mark config */
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "[CH#%02d]\n", ch);

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mark_Enable  : ");
    for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", ((tmp>>(i+8)) & 0x1));
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mark_Align   : ");
    for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8s ", align_msg[mark_win[i].align]);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mark_Path    : ");
    for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", mark_win[i].path);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mark_X       : ");
    for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", mark_win[i].x_start);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mark_Y       : ");
    for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", mark_win[i].y_start);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mark_X_DIM   : ");
    for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ",
                               (x_dim[i] >= 2) ? (0x1<<(x_dim[i]+2)) : 64);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mark_Y_DIM   : ");
    for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ",
                               (y_dim[i] >= 2) ? (0x1<<(y_dim[i]+2)) : 64);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mark_Zoom    : ");
    for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%d%-7s ", 0x1<<mark_win[i].zoom, "x");
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Mark_Alpha   : ");
    for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", mark_win[i].alpha);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    if(sfile)
        seq_printf(sfile, mbuf);
    else
        printk(mbuf);
}

static void vcap_dev_dump_osd_cfg(struct vcap_dev_info_t *pdev_info, int ch, struct seq_file *sfile)
{
    int  i;
    u32  tmp;
    int  img_border_color, img_border_width[VCAP_SCALER_MAX];
    VCAP_OSD_FONT_EDGE_MODE_T font_edge_mode;
    VCAP_OSD_PRIORITY_T priority;
    struct vcap_osd_font_smooth_param_t osd_sm;
    struct vcap_osd_marquee_param_t marq_param;
    VCAP_OSD_WIN_TYPE_T win_type[VCAP_OSD_WIN_MAX];
    VCAP_OSD_MARQUEE_MODE_T marq_mode[VCAP_OSD_WIN_MAX];
    VCAP_OSD_FONT_ZOOM_T osd_zoom[VCAP_OSD_WIN_MAX];
    struct vcap_osd_win_t osd_win[VCAP_OSD_WIN_MAX];
    struct vcap_osd_alpha_t osd_alpha[VCAP_OSD_WIN_MAX];
    struct vcap_osd_border_param_t border_param[VCAP_OSD_WIN_MAX];
    struct vcap_osd_color_t win_color[VCAP_OSD_WIN_MAX];
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;
    char *zoom_msg[]      = {"1x", "2x", "3x", "4x", "1/2x", "", "", "",
                             "H2x_V1x", "H4x_V1x", "H4x_V2x", "",
                             "H1x_V2x", "H1x_V4x", "H2x_V4x", ""};
    char *marq_mode_msg[] = {"None", "HLINE", "VLINE", "HFLIP"};
    char *align_msg[]     = {"None", "TOP_L", "TOP_C", "TOP_R", "BOTTOM_L", "BOTTOM_C", "BOTTOM_R", "CENTER"};

    if(ch == VCAP_CASCADE_CH_NUM)
        tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_SUBCH_OFFSET(0x08));
    else
        tmp = VCAP_REG_RD(pdev_info, VCAP_CH_SUBCH_OFFSET(ch, 0x08));

    /* get osd config from hardware register */
    vcap_osd_get_priority(pdev_info, ch, &priority);
    vcap_osd_get_font_smooth_param(pdev_info, ch, &osd_sm);
    vcap_osd_get_font_marquee_param(pdev_info, ch, &marq_param);
    vcap_osd_get_font_edge_mode(pdev_info, ch, &font_edge_mode);

    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        win_type[i] = pdev_info->ch[ch].param.osd[i].win_type;
        vcap_osd_get_win(pdev_info, ch, i, &osd_win[i]);
        vcap_osd_get_win_color(pdev_info, ch, i, &win_color[i]);
        vcap_osd_get_alpha(pdev_info, ch, i, &osd_alpha[i]);
        vcap_osd_get_border_param(pdev_info, ch, i, &border_param[i]);
        vcap_osd_get_font_zoom(pdev_info, ch, i, &osd_zoom[i]);
        vcap_osd_get_font_marquee_mode(pdev_info, ch, i, &marq_mode[i]);
    }

    /* get osd image border config from hardware register */
    vcap_osd_get_img_border_color(pdev_info, ch, &img_border_color);
    for(i=0; i<VCAP_SCALER_MAX; i++)
        vcap_osd_get_img_border_width(pdev_info, ch, i, &img_border_width[i]);

    /* display osd config */
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "[CH#%02d]\n", ch);

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Priority    : %s\n",
                           (priority == VCAP_OSD_PRIORITY_OSD_ON_MARK) ? "OSD_ON_MARK" : "MARK_ON_OSD");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Smooth      : %s[%d]\n",
                           (osd_sm.enable == 1) ? "Enable" : "Disable", osd_sm.level);

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Marq_Length : %d\n",
                           (marq_param.length >= VCAP_OSD_MARQUEE_LENGTH_MAX) ? 1024 : (8192/(0x1<<marq_param.length)));

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Marq_Speed  : %d\n", marq_param.speed);

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Marq_Mode   : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8s ", marq_mode_msg[marq_mode[i]]);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_WIN_Type    : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8s ", ((win_type[i] == VCAP_OSD_WIN_TYPE_MASK) ? "Mask" : "Font"));
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Enable      : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", ((tmp>>i) & 0x1));
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Align       : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8s ",
                               align_msg[osd_win[i].align]);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Path        : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", osd_win[i].path);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Zoom        : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8s ", zoom_msg[osd_zoom[i]]);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_X           : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", osd_win[i].x_start);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Y           : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", osd_win[i].y_start);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_W           : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", osd_win[i].width);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_H           : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", osd_win[i].height);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_H_SP        : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", osd_win[i].col_sp);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_V_SP        : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", osd_win[i].row_sp);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_H_NUM       : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", osd_win[i].h_wd_num);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_V_NUM       : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", osd_win[i].v_wd_num);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Word_Addr   : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", osd_win[i].wd_addr);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Win_Color_FG: ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        if((PLAT_OSD_PIXEL_BITS == 4) && (win_type[i] == VCAP_OSD_WIN_TYPE_MASK))
            mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", win_color[i].bg_color);
        else
            mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", win_color[i].fg_color);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Win_Color_BG: ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", win_color[i].bg_color);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Alpha_Font  : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", osd_alpha[i].font);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Alpha_BG    : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", osd_alpha[i].background);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Border_Width: ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", border_param[i].width);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Border_Color: ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", border_param[i].color);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Border_Type : ");
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8s ",
                               border_param[i].type == VCAP_OSD_BORDER_TYPE_FONT ? "Font" : "BG");
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Img_Border_Color: %d\n", img_border_color);

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Img_Border_Enb  : ");
    for(i=0; i<VCAP_SCALER_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", (tmp>>(12+i))&0x1);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "Img_Border_Width: ");
    for(i=0; i<VCAP_SCALER_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", img_border_width[i]);
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Frame_Mode  : ");
    for(i=0; i<VCAP_SCALER_MAX; i++) {
#ifdef PLAT_OSD_FRAME_MODE
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", ((tmp>>(16+i))&0x1));
#else
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", 0);
#endif
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_Edge_Mode   : ");
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8s ",
                           ((font_edge_mode == VCAP_OSD_FONT_EDGE_MODE_STANDARD) ? "Standard" : "TwoPixel"));
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

#ifdef PLAT_OSD_COLOR_SCHEME
    {
        int accs_thres;

        vcap_osd_get_accs_data_thres(pdev_info, ch, &accs_thres);

        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_ACCS_THRES  : %d\n", accs_thres);

        if(ch == VCAP_CASCADE_CH_NUM)
            tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_SUBCH_OFFSET(0x0c));
        else
            tmp = VCAP_REG_RD(pdev_info, VCAP_CH_SUBCH_OFFSET(ch, 0x0c));

        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "OSD_ACCS_Enable : ");
        for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
            mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%-8d ", ((tmp>>i) & 0x1));
        }
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    }
#endif

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    if(sfile)
        seq_printf(sfile, mbuf);
    else
        printk(mbuf);
}

static ssize_t vcap_dev_proc_dump_ch_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ch;
    char value_str[8] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    unsigned long flags = 0;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &ch);

    if(ch != proc_dump_ch[pdev_info->index]) {

        spin_lock_irqsave(&pdev_info->lock, flags);

        proc_dump_ch[pdev_info->index] = ch;

        spin_unlock_irqrestore(&pdev_info->lock, flags);
    }

    return count;
}

static ssize_t vcap_dev_proc_diag_clear_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, j, type, ch;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    unsigned long flags = 0;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &type, &ch);

    spin_lock_irqsave(&pdev_info->lock, flags);

    switch(type) {
        case 1: /* clear all */
            memset(&pdev_info->diag, 0, sizeof(struct vcap_diag_t));

            for(i=0; i<VCAP_CHANNEL_MAX; i++) {
                for(j=0; j<VCAP_SCALER_MAX; j++) {
                    pdev_info->ch[i].path[j].bot_cnt = pdev_info->ch[i].path[j].top_cnt = 0;
                }
                pdev_info->ch[i].hw_count = 0;
            }
            break;
        case 2: /* clear global */
            memset(&pdev_info->diag.global, 0, sizeof(struct vcap_global_diag_t));
            memset(&pdev_info->diag.vi[0],  0, sizeof(struct vcap_vi_diag_t)*VCAP_VI_MAX);
            break;
        case 3: /* clear channel */
            if(ch < 0 || ch >= VCAP_CHANNEL_MAX)
                memset(&pdev_info->diag.ch[0], 0, sizeof(struct vcap_ch_diag_t)*VCAP_CHANNEL_MAX);  ///< clear all channel
            else
                memset(&pdev_info->diag.ch[ch], 0, sizeof(struct vcap_ch_diag_t));
            break;
        case 4: /* clear frame count */
            if(ch < 0 || ch >= VCAP_CHANNEL_MAX) {
                for(i=0; i<VCAP_CHANNEL_MAX; i++) {
                    for(j=0; j<VCAP_SCALER_MAX; j++) {
                        pdev_info->ch[i].path[j].bot_cnt = pdev_info->ch[i].path[j].top_cnt = 0;
                    }
                    pdev_info->ch[i].hw_count = 0;
                }
            }
            else {
                for(i=0; i<VCAP_SCALER_MAX; i++) {
                    pdev_info->ch[ch].path[i].bot_cnt = pdev_info->ch[ch].path[i].top_cnt = 0;
                }
                pdev_info->ch[ch].hw_count = 0;
            }
            break;
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return count;
}

static ssize_t vcap_dev_proc_dbg_mode_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  mode;
    char value_str[8] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    unsigned long flags = 0;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &mode);

    spin_lock_irqsave(&pdev_info->lock, flags);

    if((mode <= 2) && (pdev_info->dbg_mode != mode))
        pdev_info->dbg_mode = mode;

    switch(mode) {
        case 98:    ///< Trigger MD Reset
            pdev_info->dev_md_fatal++;
            break;
        case 99:    ///< Force Fatal Reset
            if(pdev_info->reset)
                pdev_info->reset(pdev_info);
            break;
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return count;
}

static ssize_t vcap_dev_proc_grab_filter_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    u32  filter;
    char value_str[16] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    unsigned long flags = 0;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%x\n", &filter);

    spin_lock_irqsave(&pdev_info->lock, flags);

    (pdev_info->m_param)->grab_filter = filter;

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return count;
}

static ssize_t vcap_dev_proc_vi_probe_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned long flags;
    int probe_mode, probe_vi;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &probe_mode, &probe_vi);

    spin_lock_irqsave(&pdev_info->lock, flags);

    if((probe_mode >= VCAP_VI_PROBE_MODE_NONE) && (probe_mode <= VCAP_VI_PROBE_MODE_ACTIVE_REGION))
        pdev_info->vi_probe.mode = probe_mode;

    if((probe_vi >= 0) && (probe_vi < VCAP_VI_MAX))
        pdev_info->vi_probe.vi_idx = probe_vi;

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return count;
}

static ssize_t vcap_dev_proc_grab_hblank_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i, ch, enb;
    unsigned long flags;
    char value_str[16] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &ch, &enb);

    spin_lock_irqsave(&pdev_info->lock, flags);

    proc_grab_hb_ch[pdev_info->index] = ch;

    if(enb != 0 && enb != 1)
        goto exit;

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if((i == ch) || (ch >= VCAP_CHANNEL_MAX)) {
            if(!pdev_info->ch[i].active)
                continue;

            if(enb && vcap_input_get_info(CH_TO_VI(i))) {
                pdev_info->vi[CH_TO_VI(i)].grab_hblank |= (0x1<<SUBCH_TO_VICH(i));

                if(!IS_DEV_CH_PATH_BUSY(pdev_info, i, VCAP_CH_HBLANK_PATH))
                    pdev_info->start(pdev_info, i, VCAP_CH_HBLANK_PATH);
            }
            else
                pdev_info->vi[CH_TO_VI(i)].grab_hblank &= ~(0x1<<SUBCH_TO_VICH(i));
        }
    }

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return count;
}

static ssize_t vcap_dev_proc_vup_min_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int vup_value;
    unsigned long flags;
    char value_str[16] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &vup_value);

    spin_lock_irqsave(&pdev_info->lock, flags);

    if((vup_value >= 0) && (vup_value != (pdev_info->m_param)->vup_min))
        (pdev_info->m_param)->vup_min = vup_value;

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return count;
}

static int vcap_dev_proc_dbg_mode_show(struct seq_file *sfile, void *v)
{
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    down(&pdev_info->sema_lock);

    seq_printf(sfile, "Debug_Mode: %d\n", pdev_info->dbg_mode);
    seq_printf(sfile, "0: disable error message output\n");
    seq_printf(sfile, "1: enable  error message output\n");

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_dev_proc_ability_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int i, md_win_x_num;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)data;
    struct vcap_dev_capability_t *pcap = &pdev_info->capability;
    int len = 0;

    if(off > 0)
        goto end;

    down(&pdev_info->sema_lock);

    len += sprintf(page+len, "HW_Version         : %08x\n", VCAP_HW_VER(pdev_info));
    len += sprintf(page+len, "HW_Revision        : %03x\n", VCAP_HW_REV(pdev_info));
    len += sprintf(page+len, "VI_Count           : %d\n", pcap->vi_num);
    len += sprintf(page+len, "Cascade_Count      : %d\n", pcap->cas_num);
    len += sprintf(page+len, "Mask_Win_Count     : %d\n", pcap->mask_win_num);
    len += sprintf(page+len, "OSD_Win_Count      : %d\n", pcap->osd_win_num);
    len += sprintf(page+len, "Mark_Win_Count     : %d\n", pcap->mark_win_num);
    len += sprintf(page+len, "Scaler_Count       : %d\n", pcap->scaler_num);
    len += sprintf(page+len, "Scaler_Ability_UP  : %s %s %s %s\n", ((pcap->scaling_cap & BIT0) && (pdev_info->ch_mode != VCAP_CH_MODE_32CH)) ? "Yes" : "No ",
                                                                   ((pcap->scaling_cap & BIT1) && (pdev_info->ch_mode != VCAP_CH_MODE_32CH)) ? "Yes" : "No ",
                                                                   ((pcap->scaling_cap & BIT2) && (pdev_info->ch_mode != VCAP_CH_MODE_32CH)) ? "Yes" : "No ",
                                                                   ((pcap->scaling_cap & BIT3) && (pdev_info->ch_mode != VCAP_CH_MODE_32CH)) ? "Yes" : "No ");
    len += sprintf(page+len, "Scaler_Ability_DOWN: %s %s %s %s\n", (pcap->scaling_cap & BIT0) ? "Yes" : "No ",
                                                                   (pcap->scaling_cap & BIT1) ? "Yes" : "No ",
                                                                   (pcap->scaling_cap & BIT2) ? "Yes" : "No ",
                                                                   (pcap->scaling_cap & BIT3) ? "Yes" : "No ");
    len += sprintf(page+len, "FCS_Support        : %s\n", pcap->fcs ? "Yes" : "No");
    len += sprintf(page+len, "Denoise_Support    : %s\n", pcap->denoise ? "Yes" : "No");
    len += sprintf(page+len, "Sharpness_Support  : %s\n", pcap->sharpness ? "Yes" : "No");
    len += sprintf(page+len, "VI_MD_Win_X_Num    : ");
    for(i=0; i<VCAP_VI_MAX; i++) {
        switch((pdev_info->m_param)->vi_mode[i]) {
            case VCAP_VI_RUN_MODE_BYPASS:
                md_win_x_num = VCAP_MD_VI_1CH_X_MAX;
                break;
            case VCAP_VI_RUN_MODE_2CH:
                if(pdev_info->ch_mode == VCAP_CH_MODE_16CH)
                    md_win_x_num = VCAP_MD_VI_1CH_X_MAX;
                else
                    md_win_x_num = VCAP_MD_VI_2CH_X_MAX;
                break;
            case VCAP_VI_RUN_MODE_4CH:
                if(pdev_info->vi[i].tdm_mode == VCAP_VI_TDM_MODE_FRAME_INTERLEAVE) {
                    md_win_x_num = VCAP_MD_VI_4CH_FRAME_X_MAX;
                }
                else {
                    if(pdev_info->ch_mode == VCAP_CH_MODE_16CH)
                        md_win_x_num = VCAP_MD_VI_2CH_X_MAX;
                    else
                        md_win_x_num = VCAP_MD_VI_4CH_BYTE_X_MAX;
                }
                break;
            case VCAP_VI_RUN_MODE_SPLIT:
                md_win_x_num = VCAP_MD_SPLIT_CH_X_MAX;
                break;
            default:
                md_win_x_num = 0;
                break;
        }
        len += sprintf(page+len, "%d ", md_win_x_num);
    }
    len += sprintf(page+len, "\n");

#ifdef PLAT_OSD_TC_SWAP
    len += sprintf(page+len, "MD_IMG_SRC         : TC_Out\n");
#else
    len += sprintf(page+len, "MD_IMG_SRC         : SD_Out\n");
#endif

    len += sprintf(page+len, "TC_X_Align         : %d\n", PLAT_TC_X_ALIGN);
    len += sprintf(page+len, "OSD_Border_Pixel   : %d\n", VCAP_OSD_BORDER_PIXEL);

#ifdef PLAT_OSD_FRAME_MODE
    len += sprintf(page+len, "OSD_Frame_Mode     : Yes\n");
#else
    len += sprintf(page+len, "OSD_Frame_Mode     : No\n");
#endif

#ifdef PLAT_OSD_EDGE_SMOOTH
    len += sprintf(page+len, "OSD_Edge_Smooth    : Yes\n");
#else
    len += sprintf(page+len, "OSD_Edge_Smooth    : No\n");
#endif

#ifdef PLAT_OSD_FRAME_MODE
    len += sprintf(page+len, "OSD_Color_Scheme   : Yes\n");
#else
    len += sprintf(page+len, "OSD_Color_Scheme   : No\n");
#endif

#ifdef PLAT_REAL_TIME_CH
    len += sprintf(page+len, "Real_Time_CH       : Yes\n");
#else
    len += sprintf(page+len, "Real_Time_CH       : No\n");
#endif

    up(&pdev_info->sema_lock);

    *eof = 1;

end:
    return len;
}

static int vcap_dev_proc_dump_reg_show(struct seq_file *sfile, void *v)
{
    unsigned long flags = 0;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    spin_lock_irqsave(&pdev_info->lock, flags);

    vcap_dev_dump_reg(pdev_info, sfile);

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static int vcap_dev_proc_dump_ch_show(struct seq_file *sfile, void *v)
{
    int i;
    unsigned long flags = 0;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    spin_lock_irqsave(&pdev_info->lock, flags);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(proc_dump_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || proc_dump_ch[pdev_info->index] == i)
            vcap_dev_dump_ch(pdev_info, i, sfile);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

#ifdef PLAT_SPLIT
static int vcap_dev_proc_dump_split_show(struct seq_file *sfile, void *v)
{
    unsigned long flags = 0;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    spin_lock_irqsave(&pdev_info->lock, flags);

    vcap_dev_dump_split(pdev_info, sfile);

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static int vcap_dev_proc_dump_split_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_dump_split_show, PDE(inode)->data);
}

static struct file_operations vcap_dev_proc_dump_split_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_dump_split_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};
#endif

#ifdef CFG_HEARTBEAT_TIMER
static ssize_t vcap_dev_proc_heartbeat_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int time_ms;
    char value_str[16] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &time_ms);

    down(&pdev_info->sema_lock);

    if((time_ms >= 0) && (pdev_info->hb_timeout != time_ms)) {
        if(time_ms == 0)
            del_timer(&pdev_info->hb_timer);    ///< disable heartbeat timer
        else
            mod_timer(&pdev_info->hb_timer, jiffies+msecs_to_jiffies(time_ms));

        pdev_info->hb_timeout = time_ms;
        pdev_info->hb_count   = 0;
    }

    up(&pdev_info->sema_lock);

    return count;
}

static int vcap_dev_proc_heartbeat_show(struct seq_file *sfile, void *v)
{
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    down(&pdev_info->sema_lock);

    seq_printf(sfile, "HeartBeat Time(ms): %d [0: for disable]\n", pdev_info->hb_timeout);
    seq_printf(sfile, "HeartBeat Count   : %u\n", pdev_info->hb_count);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_dev_proc_heartbeat_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_heartbeat_show, PDE(inode)->data);
}

static struct file_operations vcap_dev_proc_heartbeat_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_heartbeat_open,
    .write  = vcap_dev_proc_heartbeat_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};
#endif

static int vcap_dev_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j;
    unsigned long flags = 0;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    char *sts_msg[] = {"IDLE", "INITED", "START", "STOP"};

    spin_lock_irqsave(&pdev_info->lock, flags);

    seq_printf(sfile, "Dev      VI#        CH#        P0     P1     P2     P3\n");
    seq_printf(sfile, "--------------------------------------------------------\n");
    seq_printf(sfile, "(%-6s)\n", sts_msg[GET_DEV_ST(pdev_info)]);
    for(i=0; i<VCAP_VI_MAX; i++) {
        for(j=0; j<VCAP_VI_CH_MAX; j++) {
            if(j == 0) {
                seq_printf(sfile, "         %d(%-6s)  %d(%-6s)  %-6s %-6s %-6s %-6s\n",
                                  i,
                                  sts_msg[GET_DEV_VI_ST(pdev_info, i)],
                                  j,
                                  sts_msg[GET_DEV_CH_ST(pdev_info, SUB_CH(i, j))],
                                  sts_msg[GET_DEV_CH_PATH_ST(pdev_info, SUB_CH(i, j), 0)],
                                  sts_msg[GET_DEV_CH_PATH_ST(pdev_info, SUB_CH(i, j), 1)],
                                  sts_msg[GET_DEV_CH_PATH_ST(pdev_info, SUB_CH(i, j), 2)],
                                  sts_msg[GET_DEV_CH_PATH_ST(pdev_info, SUB_CH(i, j), 3)] );
            }
            else {
                seq_printf(sfile, "                    %d(%-6s)  %-6s %-6s %-6s %-6s\n",
                                  j,
                                  sts_msg[GET_DEV_CH_ST(pdev_info, SUB_CH(i, j))],
                                  sts_msg[GET_DEV_CH_PATH_ST(pdev_info, SUB_CH(i, j), 0)],
                                  sts_msg[GET_DEV_CH_PATH_ST(pdev_info, SUB_CH(i, j), 1)],
                                  sts_msg[GET_DEV_CH_PATH_ST(pdev_info, SUB_CH(i, j), 2)],
                                  sts_msg[GET_DEV_CH_PATH_ST(pdev_info, SUB_CH(i, j), 3)] );
            }

            if(i == VCAP_CASCADE_VI_NUM)    ///< Cascade only one channel
                break;
        }
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static int vcap_dev_proc_lli_info_show(struct seq_file *sfile, void *v)
{
    int i, j;
    unsigned long flags = 0;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    struct vcap_lli_t *plli = &pdev_info->lli;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    spin_lock_irqsave(&pdev_info->lock, flags);

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "[vcap#%d]\n", pdev_info->index);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "Global_Update: %d\n", plli->update_pool.curr_idx);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "--------------------------------------------------------\n");
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "CH#    PATH#    Normal    NULL     Start    End\n");
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "========================================================\n");

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(proc_dump_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || proc_dump_ch[pdev_info->index] == i) {
            for(j=0; j<VCAP_SCALER_MAX; j++) {
                if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD) {
                    seq_printf(sfile, mbuf);
                    mbuf_point = 0;
                }

                if(j == 0) {
                    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                           "%-7d %-8d %-8d %-8d 0x%04x   0x%04x\n",
                                           i, j,
                                           plli->ch[i].path[j].ll_pool.curr_idx,
                                           plli->ch[i].path[j].null_pool.curr_idx,
                                           plli->ch[i].path[j].ll_mem_start,
                                           plli->ch[i].path[j].ll_mem_end);
                }
                else {
                    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                           "        %-8d %-8d %-8d 0x%04x   0x%04x\n",
                                           j,
                                           plli->ch[i].path[j].ll_pool.curr_idx,
                                           plli->ch[i].path[j].null_pool.curr_idx,
                                           plli->ch[i].path[j].ll_mem_start,
                                           plli->ch[i].path[j].ll_mem_end);
                }
            }
        }
    }

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    seq_printf(sfile, mbuf);

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static int vcap_dev_proc_dump_lli_show(struct seq_file *sfile, void *v)
{
    int i;
    unsigned long flags = 0;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    spin_lock_irqsave(&pdev_info->lock, flags);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(proc_dump_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || proc_dump_ch[pdev_info->index] == i)
            vcap_dev_dump_lli(pdev_info, i, sfile);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static int vcap_dev_proc_vg_info_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
#define VCAP_PROC_VG_INFO_PAGE_DISP_CH_MAX   24 ///< one page, only display 24 channel info because page buffer size 3KB limitation
    int i, j;
    u32 fd;
    char intf_str[32];
    int split_num, path_num, vi_mode, width, height, ch_id, ch_vlos, fps_c, fps_m;
    struct vcap_input_dev_t *pinput;
    struct vcap_dev_info_t  *pdev_info = (struct vcap_dev_info_t *)data;
    int len = 0;
    int ch_cnt = 0;
    int page_disp_cnt = 0;

    if(off >= VCAP_CHANNEL_MAX)
        return 0;

    if(off == 0) {  ///< start page
        len += sprintf(page+len, "|VCH#  VCAP_CH#  #Split  #Path  VI_Mode  FD_Start    Resolution  VLOS  FPS_C  FPS_M  Interface  \n");
        len += sprintf(page+len, "|-----------------------------------------------------------------------------------------------\n");
    }

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(pdev_info->ch[i].active) {
            if(ch_cnt++ < off)  ///< go to next display channel, off => displayed channel count
                continue;

            if(page_disp_cnt >= VCAP_PROC_VG_INFO_PAGE_DISP_CH_MAX)   ///< diplay channel count matched of this page
                goto exit;

            if(i == pdev_info->split.ch) {
                split_num = pdev_info->split.x_num*pdev_info->split.y_num;
                path_num  = VCAP_SPLIT_CH_SCALER_MAX;
                vi_mode   = VCAP_VG_VIMODE_SPLIT;
            }
            else {
                split_num = 0;
                path_num  = VCAP_SCALER_MAX;
            }
            switch((pdev_info->m_param)->vi_mode[CH_TO_VI(i)]) {
                case VCAP_VI_RUN_MODE_2CH:
                    vi_mode = VCAP_VG_VIMODE_2CH;
                    break;
                case VCAP_VI_RUN_MODE_4CH:
                    vi_mode = VCAP_VG_VIMODE_4CH;
                    break;
                case VCAP_VI_RUN_MODE_SPLIT:
                    vi_mode = VCAP_VG_VIMODE_SPLIT;
                    break;
                default:
                    vi_mode = VCAP_VG_VIMODE_BYPASS;
                    break;
            }
            fd = VCAP_VG_FD(vi_mode, i, 0, 0);

            /* resolution & frame rate */
            pinput = vcap_input_get_info(CH_TO_VI(i));
            if(!pinput) {
                ch_id   = i;
                ch_vlos = 1;
                width = height = fps_c = fps_m = 0;
                sprintf(intf_str, "progressive");
            }
            else {
                ch_id   = pinput->ch_id[i%VCAP_VI_CH_MAX];
                ch_vlos = pinput->ch_vlos[i%VCAP_VI_CH_MAX];

                if((pinput->interface == VCAP_INPUT_INTF_RGB888) && pinput->rgb.sd_hratio)
                    width = pinput->norm.width/pinput->rgb.sd_hratio;
                else {
                    if((pinput->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID) && pinput->ch_param[SUBCH_TO_VICH(i)].width)
                        width = pinput->ch_param[SUBCH_TO_VICH(i)].width;
                    else
                        width = pinput->norm.width;
                }

                if((pinput->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID) && pinput->ch_param[SUBCH_TO_VICH(i)].height)
                    height = pinput->ch_param[SUBCH_TO_VICH(i)].height;
                else
                    height = pinput->norm.height;

                /* apply horizontal source cropping rule */
                for(j=0; j<VCAP_SC_RULE_MAX; j++) {
                    if(width == ((pdev_info->m_param)->hcrop_rule[j].in_size)) {
                        width = (pdev_info->m_param)->hcrop_rule[j].out_size;
                        break;
                    }
                }

                /* apply vertical source cropping rule */
                for(j=0; j<VCAP_SC_RULE_MAX; j++) {
                    if(height == ((pdev_info->m_param)->vcrop_rule[j].in_size)) {
                        height = (pdev_info->m_param)->vcrop_rule[j].out_size;
                        break;
                    }
                }

                /* frame rate */
                if((pinput->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID) && pinput->ch_param[SUBCH_TO_VICH(i)].frame_rate) {
                    fps_c = fps_m = pinput->ch_param[SUBCH_TO_VICH(i)].frame_rate;
                }
                else {
                    fps_c = pinput->frame_rate;
                    fps_m = pinput->max_frame_rate;
                }

                /* interface */
                if(pinput->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID) {
                    if(pinput->ch_param[SUBCH_TO_VICH(i)].prog)
                        sprintf(intf_str, "progressive");
                    else
                        sprintf(intf_str, "interlace");
                }
                else {
                    switch(pinput->interface) {
                        case VCAP_INPUT_INTF_BT656_INTERLACE:
                        case VCAP_INPUT_INTF_BT1120_INTERLACE:
                        case VCAP_INPUT_INTF_SDI8BIT_INTERLACE:
                        case VCAP_INPUT_INTF_BT601_8BIT_INTERLACE:
                        case VCAP_INPUT_INTF_BT601_16BIT_INTERLACE:
                            sprintf(intf_str, "interlace");
                            break;
                        case VCAP_INPUT_INTF_RGB888:
                            sprintf(intf_str, "rgb888");
                            break;
                        case VCAP_INPUT_INTF_ISP:
                            sprintf(intf_str, "isp");
                            break;
                        default:
                            sprintf(intf_str, "progressive");
                            break;
                    }
                }
            }

            len += sprintf(page+len, " %-5d %-9d %-7d %-6d %-8d 0x%08x  %4dx%-6d %-5s %-6d %-6d %-14s\n",
                           ch_id, i, split_num, path_num, vi_mode, fd, width, height, ((ch_vlos == 0) ? "No" : "Yes"), fps_c, fps_m, intf_str);

            page_disp_cnt++;
        }
    }

exit:
    up(&pdev_info->sema_lock);

    *start = (char *)page_disp_cnt; ///< set page display channel count to *start for next time used

    if(i >= VCAP_CHANNEL_MAX)       ///< all channel display done
        *eof = 1;

    return len;
}

static int vcap_dev_proc_jobq_show(struct seq_file *sfile, void *v)
{
    int i;
    unsigned long flags;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    spin_lock_irqsave(&pdev_info->lock, flags);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(proc_dump_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || proc_dump_ch[pdev_info->index] == i)
            if(pdev_info->ch[i].active)
                vcap_dev_dump_jobq(pdev_info, i, sfile);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static int vcap_dev_proc_cfg_global_show(struct seq_file *sfile, void *v)
{
    unsigned long flags;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    spin_lock_irqsave(&pdev_info->lock, flags);

    vcap_dev_dump_global_cfg(pdev_info, sfile);

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static int vcap_dev_proc_cfg_channel_show(struct seq_file *sfile, void *v)
{
    int i;
    unsigned long flags;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    spin_lock_irqsave(&pdev_info->lock, flags);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(proc_dump_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || proc_dump_ch[pdev_info->index] == i)
            if(pdev_info->ch[i].active)
                vcap_dev_dump_channel_cfg(pdev_info, i, sfile);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static int vcap_dev_proc_cfg_mask_show(struct seq_file *sfile, void *v)
{
    int i;
    unsigned long flags;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    spin_lock_irqsave(&pdev_info->lock, flags);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(proc_dump_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || proc_dump_ch[pdev_info->index] == i)
            if(pdev_info->ch[i].active)
                vcap_dev_dump_mask_cfg(pdev_info, i, sfile);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static int vcap_dev_proc_cfg_mark_show(struct seq_file *sfile, void *v)
{
    int i;
    unsigned long flags;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    spin_lock_irqsave(&pdev_info->lock, flags);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(proc_dump_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || proc_dump_ch[pdev_info->index] == i)
            if(pdev_info->ch[i].active)
                vcap_dev_dump_mark_cfg(pdev_info, i, sfile);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static int vcap_dev_proc_cfg_osd_show(struct seq_file *sfile, void *v)
{
    int i;
    unsigned long flags;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    spin_lock_irqsave(&pdev_info->lock, flags);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(proc_dump_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || proc_dump_ch[pdev_info->index] == i)
            if(pdev_info->ch[i].active)
                vcap_dev_dump_osd_cfg(pdev_info, i, sfile);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static int vcap_dev_proc_diag_global_show(struct seq_file *sfile, void *v)
{
    int i;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    down(&pdev_info->sema_lock);

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "\n===== [Global Diagnostic] =====");
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "\nSD job count overflow       : %u", pdev_info->diag.global.sd_job_cnt_ovf);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "\nMD miss statistics done     : %u", pdev_info->diag.global.md_miss_sts);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "\nMD job count overflow       : %u", pdev_info->diag.global.md_job_cnt_ovf);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "\nLLI channel id mismatch     : %u", pdev_info->diag.global.ll_id_mismatch);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "\nLLI command load too late   : %u", pdev_info->diag.global.ll_cmd_too_late);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "\nFatal reset                 : %u", pdev_info->diag.global.fatal_reset);
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                           "\nMD    reset                 : %u", pdev_info->diag.global.md_reset);

    /* DMA */
    for(i=0; i<2; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "\nDMA#%d overflow              : %u", i, pdev_info->diag.global.dma[i].ovf);
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "\nDMA#%d job count overflow    : %u", i, pdev_info->diag.global.dma[i].job_cnt_ovf);
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "\nDMA#%d write response fail   : %u", i, pdev_info->diag.global.dma[i].write_resp_fail);
        if(i == 0) {
            mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "\nDMA#%d read  response fail   : %u", i, pdev_info->diag.global.dma[i].read_resp_fail);
        }
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "\nDMA#%d commad prefix error   : %u", i, pdev_info->diag.global.dma[i].cmd_prefix_err);
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "\nDMA#%d write block width zero: %u", i, pdev_info->diag.global.dma[i].write_blk_zero);
    }

    /* VI */
    for(i=0; i<VCAP_VI_MAX; i++) {
        mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                               "\nVI#%d no clock               : %u", i, pdev_info->diag.vi[i].no_clock);
    }

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n\n");
    seq_printf(sfile, mbuf);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_dev_proc_diag_channel_show(struct seq_file *sfile, void *v)
{
    int i;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(proc_dump_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || proc_dump_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\n===== [CH#%d Diagnostic] =====", i);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nVI fifo full          : %u", pdev_info->diag.ch[i].vi_fifo_full);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nPixel lack            : %u", pdev_info->diag.ch[i].pixel_lack);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nLine lack             : %u", pdev_info->diag.ch[i].line_lack);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nSD timeout            : %u", pdev_info->diag.ch[i].sd_timeout);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nSD job overflow       : %u", pdev_info->diag.ch[i].sd_job_ovf);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nSD SC memory overflow : %u", pdev_info->diag.ch[i].sd_sc_mem_ovf);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nSD line lack          : %u", pdev_info->diag.ch[i].sd_line_lack);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nSD pixel lack         : %u", pdev_info->diag.ch[i].sd_pixel_lack);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nSD parameter error    : %u", pdev_info->diag.ch[i].sd_param_err);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nSD prefix decode error: %u", pdev_info->diag.ch[i].sd_prefix_err);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nSD 1st field          : %u", pdev_info->diag.ch[i].sd_1st_field_err);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nMD read traffic jam   : %u", pdev_info->diag.ch[i].md_traffic_jam);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nMD gaussian error     : %u", pdev_info->diag.ch[i].md_gau_err);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nLL id mismatch        : %u", pdev_info->diag.ch[i].ll_sts_id_mismatch);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nLL DMA no done        : %u", pdev_info->diag.ch[i].ll_dma_no_done);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nLL Split DMA no done  : %u", pdev_info->diag.ch[i].ll_split_dma_no_done);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nLL jump table update  : %u", pdev_info->diag.ch[i].ll_jump_table);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nLL Null table mismatch: %u", pdev_info->diag.ch[i].ll_null_mismatch);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nLL Null table not zero: %u", pdev_info->diag.ch[i].ll_null_not_zero);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nLL table lack         : %u\t %u\t %u\t %u",
                                       pdev_info->diag.ch[i].ll_tab_lack[0], pdev_info->diag.ch[i].ll_tab_lack[1],
                                       pdev_info->diag.ch[i].ll_tab_lack[2], pdev_info->diag.ch[i].ll_tab_lack[3]);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nNo_Job alarm          : %u\t %u\t %u\t %u",
                                       pdev_info->diag.ch[i].no_job_alm[0], pdev_info->diag.ch[i].no_job_alm[1],
                                       pdev_info->diag.ch[i].no_job_alm[2], pdev_info->diag.ch[i].no_job_alm[3]);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nJob Timeout           : %u", pdev_info->diag.ch[i].job_timeout);

                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
            }

            if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD) {
                seq_printf(sfile, mbuf);
                mbuf_point = 0;
            }
        }
    }

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    seq_printf(sfile, mbuf);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_dev_proc_diag_framecnt_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(proc_dump_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || proc_dump_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\n[CH#%02d]", i);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nPath    top        bottom");
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\n===============================");
                for(j=0; j<VCAP_SCALER_MAX; j++) {
                    if((pdev_info->split.ch == i) && (j >= VCAP_SPLIT_CH_SCALER_MAX))
                        continue;

                    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                           "\n%-7d %-10u %-10u", j, pdev_info->ch[i].path[j].top_cnt, pdev_info->ch[i].path[j].bot_cnt);
                }

                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point,
                                       "\nHW_Count: %u", pdev_info->ch[i].hw_count);

                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
            }

            if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD) {
                seq_printf(sfile, mbuf);
                mbuf_point = 0;
            }
        }
    }

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    seq_printf(sfile, mbuf);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_dev_proc_diag_clear_show(struct seq_file *sfile, void *v)
{
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    down(&pdev_info->sema_lock);

    seq_printf(sfile, "Clear Diagnostic Data\n");
    seq_printf(sfile, "1: clear all\n");
    seq_printf(sfile, "2: clear global\n");
    seq_printf(sfile, "3: clear channel\n");

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_dev_proc_crop_rule_show(struct seq_file *sfile, void *v)
{
    int i;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    struct vcap_dev_module_param_t *pm_param = pdev_info->m_param;

    down(&pdev_info->sema_lock);

    seq_printf(sfile, "Rule#  H_In    H_Out    V_In    V_Out \n");
    seq_printf(sfile, "--------------------------------------\n");
    for(i=0; i<VCAP_SC_RULE_MAX; i++) {
        if(pm_param->hcrop_rule[i].in_size)
            seq_printf(sfile, "%-6d %-7d %-9d", i, pm_param->hcrop_rule[i].in_size, pm_param->hcrop_rule[i].out_size);
        else
            seq_printf(sfile, "%-6d %-7s %-9s", i, "-", "-");

        if(pm_param->vcrop_rule[i].in_size)
            seq_printf(sfile, "%-7d %-8d\n", pm_param->vcrop_rule[i].in_size, pm_param->vcrop_rule[i].out_size);
        else
            seq_printf(sfile, "%-7s %-8s\n", "-", "-");
    }

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_dev_proc_grab_filter_show(struct seq_file *sfile, void *v)
{
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    struct vcap_dev_module_param_t *pm_param = pdev_info->m_param;

    down(&pdev_info->sema_lock);

    seq_printf(sfile, "Grab Filter => 0x%08x\n", pm_param->grab_filter);
    seq_printf(sfile, "-----------------------------\n");
    seq_printf(sfile, "%s[BIT0]: SD_JOB_OVF\n", ((pm_param->grab_filter & VCAP_GRAB_FILTER_SD_JOB_OVF) ? "*" : "-"));
    seq_printf(sfile, "%s[BIT1]: SD_SC_MEM_OVF\n", ((pm_param->grab_filter & VCAP_GRAB_FILTER_SD_SC_MEM_OVF) ? "*" : "-"));
    seq_printf(sfile, "%s[BIT2]: SD_PARAM_ERR\n", ((pm_param->grab_filter & VCAP_GRAB_FILTER_SD_PARAM_ERR) ? "*" : "-"));
    seq_printf(sfile, "%s[BIT3]: SD_PREFIX_ERR\n", ((pm_param->grab_filter & VCAP_GRAB_FILTER_SD_PREFIX_ERR) ? "*" : "-"));
    seq_printf(sfile, "%s[BIT4]: SD_TIMEOUT\n", ((pm_param->grab_filter & VCAP_GRAB_FILTER_SD_TIMEOUT) ? "*" : "-"));
    seq_printf(sfile, "%s[BIT5]: SD_PIXEL_LACK\n", ((pm_param->grab_filter & VCAP_GRAB_FILTER_SD_PIXEL_LACK) ? "*" : "-"));
    seq_printf(sfile, "%s[BIT6]: SD_LINE_LACK\n", ((pm_param->grab_filter & VCAP_GRAB_FILTER_SD_LINE_LACK) ? "*" : "-"));

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_dev_proc_vi_probe_show(struct seq_file *sfile, void *v)
{
    unsigned long flags;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    char *mode_msg[] = {"NONE", "ACTIVE_REGION"};

    spin_lock_irqsave(&pdev_info->lock, flags);

    seq_printf(sfile, "Mode: %-13s [0:%s 1:%s]\n",
               ((pdev_info->vi_probe.mode <= VCAP_VI_PROBE_MODE_ACTIVE_REGION) ? mode_msg[pdev_info->vi_probe.mode] : "UNKNOWN"),
               mode_msg[0], mode_msg[1]);
    seq_printf(sfile, "VI  : %-13d [0 ~ %d]\n", pdev_info->vi_probe.vi_idx, (VCAP_VI_MAX-1));

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

static int vcap_dev_proc_grab_hblank_show(struct seq_file *sfile, void *v)
{
    int i, j;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if((i == proc_grab_hb_ch[pdev_info->index]) || (proc_grab_hb_ch[pdev_info->index] >= VCAP_CHANNEL_MAX)) {
            if(!pdev_info->ch[i].active)
                continue;

            if(!pdev_info->ch[i].hblank_buf.vaddr)
                continue;

            seq_printf(sfile, "==== [CH#%d] ====\n", i);
            for(j=0; j<VCAP_CH_HBLANK_BUF_SIZE; j++) {
                if((j%16) == 0)
                    seq_printf(sfile, "%08x: ", ((u32)(pdev_info->ch[i].hblank_buf.paddr))+((j>>4)<<4));

                seq_printf(sfile, "%02x%s", ((volatile u8 *)(pdev_info->ch[i].hblank_buf.vaddr))[j], ((((j+1)%16) == 0) ? "\n" : " "));
            }
            seq_printf(sfile, "\n");
        }
    }
    seq_printf(sfile, "===========================================================\n");
    seq_printf(sfile, "echo [CH#] [On(1)/off(0)] to trigger grab channel hblanking\n\n");

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_dev_proc_vup_min_show(struct seq_file *sfile, void *v)
{
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    struct vcap_dev_module_param_t *pm_param = pdev_info->m_param;

    down(&pdev_info->sema_lock);

    seq_printf(sfile, "Vertical minimal scaling up line number: %d\n", pm_param->vup_min);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_dev_proc_dbg_mode_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_dbg_mode_show, PDE(inode)->data);
}

static int vcap_dev_proc_dump_reg_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_dump_reg_show, PDE(inode)->data);
}

static int vcap_dev_proc_dump_ch_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_dump_ch_show, PDE(inode)->data);
}

static int vcap_dev_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_status_show, PDE(inode)->data);
}

static int vcap_dev_proc_lli_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_lli_info_show, PDE(inode)->data);
}

static int vcap_dev_proc_dump_lli_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_dump_lli_show, PDE(inode)->data);
}

static int vcap_dev_proc_jobq_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_jobq_show, PDE(inode)->data);
}

static int vcap_dev_proc_cfg_global_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_cfg_global_show, PDE(inode)->data);
}

static int vcap_dev_proc_cfg_channel_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_cfg_channel_show, PDE(inode)->data);
}

static int vcap_dev_proc_cfg_mask_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_cfg_mask_show, PDE(inode)->data);
}

static int vcap_dev_proc_cfg_mark_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_cfg_mark_show, PDE(inode)->data);
}

static int vcap_dev_proc_cfg_osd_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_cfg_osd_show, PDE(inode)->data);
}

static int vcap_dev_proc_diag_global_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_diag_global_show, PDE(inode)->data);
}

static int vcap_dev_proc_diag_channel_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_diag_channel_show, PDE(inode)->data);
}

static int vcap_dev_proc_diag_framecnt_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_diag_framecnt_show, PDE(inode)->data);
}

static int vcap_dev_proc_diag_clear_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_diag_clear_show, PDE(inode)->data);
}

static int vcap_dev_proc_crop_rule_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_crop_rule_show, PDE(inode)->data);
}

static int vcap_dev_proc_grab_filter_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_grab_filter_show, PDE(inode)->data);
}

static int vcap_dev_proc_vi_probe_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_vi_probe_show, PDE(inode)->data);
}

static int vcap_dev_proc_grab_hblank_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_grab_hblank_show, PDE(inode)->data);
}

static int vcap_dev_proc_vup_min_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_dev_proc_vup_min_show, PDE(inode)->data);
}

static struct file_operations vcap_dev_proc_dbg_mode_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_dbg_mode_open,
    .write  = vcap_dev_proc_dbg_mode_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_dump_reg_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_dump_reg_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_dump_ch_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_dump_ch_open,
    .write  = vcap_dev_proc_dump_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_lli_info_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_lli_info_open,
    .write  = vcap_dev_proc_dump_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_dump_lli_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_dump_lli_open,
    .write  = vcap_dev_proc_dump_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_jobq_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_jobq_open,
    .write  = vcap_dev_proc_dump_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_cfg_global_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_cfg_global_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_cfg_channel_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_cfg_channel_open,
    .write  = vcap_dev_proc_dump_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_cfg_mask_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_cfg_mask_open,
    .write  = vcap_dev_proc_dump_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_cfg_mark_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_cfg_mark_open,
    .write  = vcap_dev_proc_dump_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_cfg_osd_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_cfg_osd_open,
    .write  = vcap_dev_proc_dump_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_diag_global_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_diag_global_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_diag_channel_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_diag_channel_open,
    .write  = vcap_dev_proc_dump_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_diag_framecnt_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_diag_framecnt_open,
    .write  = vcap_dev_proc_dump_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_diag_clear_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_diag_clear_open,
    .write  = vcap_dev_proc_diag_clear_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_crop_rule_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_crop_rule_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_grab_filter_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_grab_filter_open,
    .write  = vcap_dev_proc_grab_filter_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_vi_probe_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_vi_probe_open,
    .write  = vcap_dev_proc_vi_probe_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_grab_hblank_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_grab_hblank_open,
    .write  = vcap_dev_proc_grab_hblank_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_dev_proc_vup_min_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_dev_proc_vup_min_open,
    .write  = vcap_dev_proc_vup_min_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

void vcap_dev_proc_remove(int id)
{
    vcap_sharp_proc_remove(id);

    vcap_presmo_proc_remove(id);

    vcap_dn_proc_remove(id);

    vcap_fcs_proc_remove(id);

    vcap_md_proc_remove(id);

    if(vcap_dev_proc_cfg_root[id]) {
        vcap_proc_remove_entry(vcap_dev_proc_cfg_root[id], vcap_dev_proc_cfg_global[id]);

        vcap_proc_remove_entry(vcap_dev_proc_cfg_root[id], vcap_dev_proc_cfg_channel[id]);

        vcap_proc_remove_entry(vcap_dev_proc_cfg_root[id], vcap_dev_proc_cfg_mask[id]);

        vcap_proc_remove_entry(vcap_dev_proc_cfg_root[id], vcap_dev_proc_cfg_mark[id]);

        vcap_proc_remove_entry(vcap_dev_proc_cfg_root[id], vcap_dev_proc_cfg_osd[id]);

        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_cfg_root[id]);
    }

    if(vcap_dev_proc_diag_root[id]) {
        vcap_proc_remove_entry(vcap_dev_proc_diag_root[id], vcap_dev_proc_diag_global[id]);

        vcap_proc_remove_entry(vcap_dev_proc_diag_root[id], vcap_dev_proc_diag_channel[id]);

        vcap_proc_remove_entry(vcap_dev_proc_diag_root[id], vcap_dev_proc_diag_framecnt[id]);

        vcap_proc_remove_entry(vcap_dev_proc_diag_root[id], vcap_dev_proc_diag_clear[id]);

        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_diag_root[id]);
    }

    if(vcap_dev_proc_jobq[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_jobq[id]);

    if(vcap_dev_proc_vg_info[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_vg_info[id]);

    if(vcap_dev_proc_dump_lli[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_dump_lli[id]);

    if(vcap_dev_proc_lli_info[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_lli_info[id]);

    if(vcap_dev_proc_status[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_status[id]);

#ifdef PLAT_SPLIT
    if(vcap_dev_proc_dump_split[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_dump_split[id]);
#endif

    if(vcap_dev_proc_dump_ch[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_dump_ch[id]);

    if(vcap_dev_proc_dump_reg[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_dump_reg[id]);

    if(vcap_dev_proc_ability[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_ability[id]);

    if(vcap_dev_proc_dbg_mode[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_dbg_mode[id]);

    if(vcap_dev_proc_crop_rule[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_crop_rule[id]);

    if(vcap_dev_proc_grab_filter[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_grab_filter[id]);

    if(vcap_dev_proc_vi_probe[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_vi_probe[id]);

    if(vcap_dev_proc_grab_hblank[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_grab_hblank[id]);

    if(vcap_dev_proc_vup_min[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_vup_min[id]);

#ifdef CFG_HEARTBEAT_TIMER
    if(vcap_dev_proc_heartbeat[id])
        vcap_proc_remove_entry(vcap_dev_proc_root[id], vcap_dev_proc_heartbeat[id]);
#endif

    if(vcap_dev_proc_root[id])
        vcap_proc_remove_entry(NULL, vcap_dev_proc_root[id]);
}

int vcap_dev_proc_init(int id, const char *name, void *private)
{
    int ret = 0;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)private;

    if(id >= VCAP_DEV_MAX) {
        vcap_err("device number is out of range!\n");
        ret = -EINVAL;
        goto end;
    }

    /* device root */
    vcap_dev_proc_root[id] = vcap_proc_create_entry(name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!vcap_dev_proc_root[id]) {
        vcap_err("create proc node '%s' failed!\n", name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_root[id]->owner = THIS_MODULE;
#endif

    /* cfg root */
    vcap_dev_proc_cfg_root[id] = vcap_proc_create_entry("cfg", S_IFDIR|S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_cfg_root[id]) {
        vcap_err("create proc node '%s/cfg' failed!\n", name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_cfg_root[id]->owner = THIS_MODULE;
#endif

    /* diagnostic root */
    vcap_dev_proc_diag_root[id] = vcap_proc_create_entry("diagnostic", S_IFDIR|S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_diag_root[id]) {
        vcap_err("create proc node '%s/diagnostic' failed!\n", name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_diag_root[id]->owner = THIS_MODULE;
#endif

    /* dbg_mode */
    vcap_dev_proc_dbg_mode[id] = vcap_proc_create_entry("dbg_mode", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_dbg_mode[id]) {
        vcap_err("create proc node '%s/dbg_mode' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_dbg_mode[id]->proc_fops = &vcap_dev_proc_dbg_mode_ops;
    vcap_dev_proc_dbg_mode[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_dbg_mode[id]->owner     = THIS_MODULE;
#endif

    /* ability */
    vcap_dev_proc_ability[id] = vcap_proc_create_entry("ability", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_ability[id]) {
        vcap_err("create proc node '%s/ability' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_ability[id]->read_proc = vcap_dev_proc_ability_read;
    vcap_dev_proc_ability[id]->data      = private;

    /* dump_reg */
    vcap_dev_proc_dump_reg[id] = vcap_proc_create_entry("dump_reg", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_dump_reg[id]) {
        vcap_err("create proc node '%s/dump_reg' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_dump_reg[id]->proc_fops = &vcap_dev_proc_dump_reg_ops;
    vcap_dev_proc_dump_reg[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_dump_reg[id]->owner     = THIS_MODULE;
#endif

    /* dump_ch */
    vcap_dev_proc_dump_ch[id] = vcap_proc_create_entry("dump_ch", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_dump_ch[id]) {
        vcap_err("create proc node '%s/dump_ch' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_dump_ch[id]->proc_fops = &vcap_dev_proc_dump_ch_ops;
    vcap_dev_proc_dump_ch[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_dump_ch[id]->owner     = THIS_MODULE;
#endif

#ifdef PLAT_SPLIT
    /* dump_split */
    vcap_dev_proc_dump_split[id] = vcap_proc_create_entry("dump_split", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_dump_split[id]) {
        vcap_err("create proc node '%s/dump_split' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_dump_split[id]->proc_fops = &vcap_dev_proc_dump_split_ops;
    vcap_dev_proc_dump_split[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_dump_split[id]->owner     = THIS_MODULE;
#endif
#endif

    /* status */
    vcap_dev_proc_status[id] = vcap_proc_create_entry("status", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_status[id]) {
        vcap_err("create proc node '%s/status' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_status[id]->proc_fops = &vcap_dev_proc_status_ops;
    vcap_dev_proc_status[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_status[id]->owner     = THIS_MODULE;
#endif

    if(pdev_info->dev_type == VCAP_DEV_DRV_TYPE_LLI) {
        /* lli_info */
        vcap_dev_proc_lli_info[id] = vcap_proc_create_entry("lli_info", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
        if(!vcap_dev_proc_lli_info[id]) {
            vcap_err("create proc node '%s/lli_info' failed!\n", name);
            ret = -EINVAL;
            goto err;
        }
        vcap_dev_proc_lli_info[id]->proc_fops = &vcap_dev_proc_lli_info_ops;
        vcap_dev_proc_lli_info[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
        vcap_dev_proc_lli_info[id]->owner     = THIS_MODULE;
#endif

        /* dump_lli */
        vcap_dev_proc_dump_lli[id] = vcap_proc_create_entry("dump_lli", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
        if(!vcap_dev_proc_dump_lli[id]) {
            vcap_err("create proc node '%s/dump_lli' failed!\n", name);
            ret = -EINVAL;
            goto err;
        }
        vcap_dev_proc_dump_lli[id]->proc_fops = &vcap_dev_proc_dump_lli_ops;
        vcap_dev_proc_dump_lli[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
        vcap_dev_proc_dump_lli[id]->owner     = THIS_MODULE;
#endif
    }

    /* video graph information */
    vcap_dev_proc_vg_info[id] = vcap_proc_create_entry("vg_info", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_vg_info[id]) {
        vcap_err("create proc node '%s/vg_info' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_vg_info[id]->read_proc = vcap_dev_proc_vg_info_read;
    vcap_dev_proc_vg_info[id]->data      = private;

    /* job queue */
    vcap_dev_proc_jobq[id] = vcap_proc_create_entry("jobq", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_jobq[id]) {
        vcap_err("create proc node '%s/jobq' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_jobq[id]->proc_fops = &vcap_dev_proc_jobq_ops;
    vcap_dev_proc_jobq[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_jobq[id]->owner     = THIS_MODULE;
#endif

    /* global config */
    vcap_dev_proc_cfg_global[id] = vcap_proc_create_entry("global", S_IRUGO|S_IXUGO, vcap_dev_proc_cfg_root[id]);
    if(!vcap_dev_proc_cfg_global[id]) {
        vcap_err("create proc node '%s/cfg/global' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_cfg_global[id]->proc_fops = &vcap_dev_proc_cfg_global_ops;
    vcap_dev_proc_cfg_global[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_cfg_global[id]->owner     = THIS_MODULE;
#endif

    /* channel config */
    vcap_dev_proc_cfg_channel[id] = vcap_proc_create_entry("channel", S_IRUGO|S_IXUGO, vcap_dev_proc_cfg_root[id]);
    if(!vcap_dev_proc_cfg_channel[id]) {
        vcap_err("create proc node '%s/cfg/channel' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_cfg_channel[id]->proc_fops = &vcap_dev_proc_cfg_channel_ops;
    vcap_dev_proc_cfg_channel[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_cfg_channel[id]->owner     = THIS_MODULE;
#endif

    /* mask config */
    vcap_dev_proc_cfg_mask[id] = vcap_proc_create_entry("mask", S_IRUGO|S_IXUGO, vcap_dev_proc_cfg_root[id]);
    if(!vcap_dev_proc_cfg_mask[id]) {
        vcap_err("create proc node '%s/cfg/mask' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_cfg_mask[id]->proc_fops = &vcap_dev_proc_cfg_mask_ops;
    vcap_dev_proc_cfg_mask[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_cfg_mask[id]->owner     = THIS_MODULE;
#endif

    /* mark config */
    vcap_dev_proc_cfg_mark[id] = vcap_proc_create_entry("mark", S_IRUGO|S_IXUGO, vcap_dev_proc_cfg_root[id]);
    if(!vcap_dev_proc_cfg_mark[id]) {
        vcap_err("create proc node '%s/cfg/mark' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_cfg_mark[id]->proc_fops = &vcap_dev_proc_cfg_mark_ops;
    vcap_dev_proc_cfg_mark[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_cfg_mark[id]->owner     = THIS_MODULE;
#endif

    /* osd config */
    vcap_dev_proc_cfg_osd[id] = vcap_proc_create_entry("osd", S_IRUGO|S_IXUGO, vcap_dev_proc_cfg_root[id]);
    if(!vcap_dev_proc_cfg_osd[id]) {
        vcap_err("create proc node '%s/cfg/osd' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_cfg_osd[id]->proc_fops = &vcap_dev_proc_cfg_osd_ops;
    vcap_dev_proc_cfg_osd[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_cfg_osd[id]->owner     = THIS_MODULE;
#endif

    /* diagnostic global */
    vcap_dev_proc_diag_global[id] = vcap_proc_create_entry("global", S_IRUGO|S_IXUGO, vcap_dev_proc_diag_root[id]);
    if(!vcap_dev_proc_diag_global[id]) {
        vcap_err("create proc node '%s/diagnostic/global' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_diag_global[id]->proc_fops = &vcap_dev_proc_diag_global_ops;
    vcap_dev_proc_diag_global[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_diag_global[id]->owner     = THIS_MODULE;
#endif

    /* diagnostic channel */
    vcap_dev_proc_diag_channel[id] = vcap_proc_create_entry("channel", S_IRUGO|S_IXUGO, vcap_dev_proc_diag_root[id]);
    if(!vcap_dev_proc_diag_channel[id]) {
        vcap_err("create proc node '%s/diagnostic/channel' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_diag_channel[id]->proc_fops = &vcap_dev_proc_diag_channel_ops;
    vcap_dev_proc_diag_channel[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_diag_channel[id]->owner     = THIS_MODULE;
#endif

    /* diagnostic frame count */
    vcap_dev_proc_diag_framecnt[id] = vcap_proc_create_entry("frame_cnt", S_IRUGO|S_IXUGO, vcap_dev_proc_diag_root[id]);
    if(!vcap_dev_proc_diag_framecnt[id]) {
        vcap_err("create proc node '%s/diagnostic/frame_cnt' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_diag_framecnt[id]->proc_fops = &vcap_dev_proc_diag_framecnt_ops;
    vcap_dev_proc_diag_framecnt[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_diag_framecnt[id]->owner     = THIS_MODULE;
#endif

    /* diagnostic clear */
    vcap_dev_proc_diag_clear[id] = vcap_proc_create_entry("clear", S_IRUGO|S_IXUGO, vcap_dev_proc_diag_root[id]);
    if(!vcap_dev_proc_diag_clear[id]) {
        vcap_err("create proc node '%s/diagnostic/clear' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_diag_clear[id]->proc_fops = &vcap_dev_proc_diag_clear_ops;
    vcap_dev_proc_diag_clear[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_diag_clear[id]->owner     = THIS_MODULE;
#endif

    /* crop_rule */
    vcap_dev_proc_crop_rule[id] = vcap_proc_create_entry("crop_rule", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_crop_rule[id]) {
        vcap_err("create proc node '%s/ability' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_crop_rule[id]->proc_fops = &vcap_dev_proc_crop_rule_ops;
    vcap_dev_proc_crop_rule[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_crop_rule[id]->owner     = THIS_MODULE;
#endif

    /* grab_filter */
    vcap_dev_proc_grab_filter[id] = vcap_proc_create_entry("grab_filter", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_grab_filter[id]) {
        vcap_err("create proc node '%s/grab_filter' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_grab_filter[id]->proc_fops = &vcap_dev_proc_grab_filter_ops;
    vcap_dev_proc_grab_filter[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_grab_filter[id]->owner     = THIS_MODULE;
#endif

    /* vi_probe */
    vcap_dev_proc_vi_probe[id] = vcap_proc_create_entry("vi_probe", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_vi_probe[id]) {
        vcap_err("create proc node '%s/vi_probe' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_vi_probe[id]->proc_fops = &vcap_dev_proc_vi_probe_ops;
    vcap_dev_proc_vi_probe[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_vi_probe[id]->owner     = THIS_MODULE;
#endif

    /* grab_hblank */
    vcap_dev_proc_grab_hblank[id] = vcap_proc_create_entry("grab_hblank", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_grab_hblank[id]) {
        vcap_err("create proc node '%s/grab_hblank' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_grab_hblank[id]->proc_fops = &vcap_dev_proc_grab_hblank_ops;
    vcap_dev_proc_grab_hblank[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_grab_hblank[id]->owner     = THIS_MODULE;
#endif

    /* vup_min */
    vcap_dev_proc_vup_min[id] = vcap_proc_create_entry("vup_min", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_vup_min[id]) {
        vcap_err("create proc node '%s/vup_min' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_vup_min[id]->proc_fops = &vcap_dev_proc_vup_min_ops;
    vcap_dev_proc_vup_min[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_vup_min[id]->owner     = THIS_MODULE;
#endif

#ifdef CFG_HEARTBEAT_TIMER
    /* heartbeat */
    vcap_dev_proc_heartbeat[id] = vcap_proc_create_entry("heartbeat", S_IRUGO|S_IXUGO, vcap_dev_proc_root[id]);
    if(!vcap_dev_proc_heartbeat[id]) {
        vcap_err("create proc node '%s/heartbeat' failed!\n", name);
        ret = -EINVAL;
        goto err;
    }
    vcap_dev_proc_heartbeat[id]->proc_fops = &vcap_dev_proc_heartbeat_ops;
    vcap_dev_proc_heartbeat[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_dev_proc_heartbeat[id]->owner     = THIS_MODULE;
#endif
#endif

    /* FCS Proc */
    ret = vcap_fcs_proc_init(id, private);
    if(ret < 0)
       goto err;

    /* DeNoise Proc */
    ret = vcap_dn_proc_init(id, private);
    if(ret < 0)
       goto err;

    /* Presmooth Proc */
    ret = vcap_presmo_proc_init(id, private);
    if(ret < 0)
       goto err;

    /* Sharpness Proc */
    ret = vcap_sharp_proc_init(id, private);
    if(ret < 0)
       goto err;

    /* MD Proc */
    ret = vcap_md_proc_init(id, private);
    if(ret < 0)
       goto err;

end:
    return ret;

err:
    vcap_dev_proc_remove(id);
    return ret;
}

void vcap_dev_config_hw_capability(struct vcap_dev_info_t *pdev_info)
{
    u32 tmp;
    u32 chip_ver = (vcap_plat_chip_version() & 0xff);
    struct vcap_dev_capability_t *pdev_cab = &pdev_info->capability;

    tmp = VCAP_REG_RD(pdev_info, VCAP_STATUS_OFFSET(0xec));

    pdev_cab->sharpness    = (tmp & BIT27) ? 1 : 0;
    pdev_cab->denoise      = (tmp & BIT26) ? 1 : 0;
    pdev_cab->fcs          = (tmp & BIT25) ? 1 : 0;
    pdev_cab->cas_num      = (tmp>>24) & 0x1;
    pdev_cab->vi_num       = (tmp>>20) & 0xf;
    pdev_cab->osd_win_num  = (tmp>>16) & 0xf;
    pdev_cab->mark_win_num = (tmp>>12) & 0xf;
    pdev_cab->mask_win_num = (tmp>>8)  & 0xf;
    pdev_cab->scaling_cap  = (tmp>>4)  & 0xf;
    pdev_cab->scaler_num   = tmp & 0xf;

    if((VCAP_HW_VER(pdev_info) == 0x20120606) && (VCAP_HW_REV(pdev_info) == 0)) { ///< GM8210 Capture REV#0
        pdev_cab->bug_p2i_miss_top     = 1;
        pdev_cab->bug_two_dma_ch       = 1;
        pdev_cab->bug_md_event_96_mb   = 1;
    }
    else {
        pdev_cab->dma_field_offset_rev = 1;
        pdev_cab->vi_sdi8bit           = 1;
    }

    if((VCAP_HW_VER(pdev_info) == 0x20120831) && (chip_ver < 0x03)) {    ///< GM8287, ECO on VER.E
        pdev_cab->bug_ch14_rolling = 1;
    }

    if((VCAP_HW_VER(pdev_info) == 0x20140311) && (VCAP_HW_REV(pdev_info) == 0)) {   ///< GM8136
        pdev_cab->bug_isp_pixel_lack = 1;
    }

#ifdef PLAT_VI_BT601
    pdev_cab->vi_bt601 = 1;     ///< GM8139 support
#endif

#ifdef PLAT_CASCADE_ISP
    pdev_cab->cas_isp = 1;      ///< GM8139 support
#endif

#ifdef PLAT_CASCADE_SDI8BIT
    pdev_cab->cas_sdi8bit = 1;  ///< GM8136 support
#endif
}

void vcap_dev_reset(struct vcap_dev_info_t *pdev_info)
{
    u32 tmp;
    int cnt = 10;
    u32 dma_status_mask = BIT10|BIT2;

    /* Disable DMA */
    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x00));
    tmp |= BIT31;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x00), tmp);

    /* polling DMA disable status */
    while(((VCAP_REG_RD(pdev_info, VCAP_STATUS_OFFSET(0xa8)) & dma_status_mask)!=dma_status_mask) && (cnt-- > 0))
        udelay(5);

    if(IS_DEV_BUSY(pdev_info)) {
        if((VCAP_REG_RD(pdev_info, VCAP_STATUS_OFFSET(0xa8)) & dma_status_mask) != dma_status_mask)
            vcap_err("[%d] DMA disable failed!\n", pdev_info->index);
    }

    /* Disable Meassure Enable Bit */
    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x68));
    tmp &= ~BIT31;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x68), tmp);

    /* Clear fire bit and disable capture */
    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x04));
    tmp &= ~(BIT28|BIT31);
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x04), tmp);

    /* set software reset */
    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x04));
    tmp |= BIT0;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x04), tmp);

    udelay(5);

    /* free software reset */
    tmp &= ~BIT0;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x04), tmp);

    /* Clear hardware frame count monitor */
    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x00));
    tmp |= BIT0;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x00), tmp);  ///< auto clear

    /* Clear Status */
    VCAP_REG_WR(pdev_info, VCAP_STATUS_OFFSET(0xa8), dma_status_mask);
}

void vcap_dev_demux_reset(struct vcap_dev_info_t *pdev_info, int vi_idx)
{
    u32 tmp;

    if((vi_idx < VCAP_VI_MAX) && (vi_idx != VCAP_CASCADE_VI_NUM)) { ///< cascade no demux
        tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x00));
        tmp |= (0x1<<(8+vi_idx));
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x00), tmp);

        udelay(5);

        tmp &= ~(0x1<<(8+vi_idx));
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x00), tmp);
    }
}

void vcap_dev_set_reg_default(struct vcap_dev_info_t *pdev_info)
{
    int i;

    /* Global */
    for(i=0; i<VCAP_GLOBAL_REG_SIZE; i+=4)
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(i), ((i == 0x3c) ? 0x08100000 : 0));

    /* VI */
    for(i=0; i<VCAP_VI_REG_SIZE; i+=4) {
        if(i < 0x40) {
            VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(i), ((i%8 == 0) ? 0x08002000 : 0));
        }
        else if(i < 0x48) {
            VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(i), ((i%8 == 0) ? 0x08000000 : 0));
        }
        else if(i < 0x50) {
            VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(i), ((i%8 == 0) ? 0x10000000 : 0));
        }
        else
            VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(i), 0);
    }

    /* SC */
    memset((void *)(pdev_info->vbase+VCAP_SC_OFFSET(0)), 0, VCAP_SC_REG_SIZE);

    /* Palette */
    memset((void *)(pdev_info->vbase+VCAP_PALETTE_OFFSET(0)), 0, VCAP_PALETTE_REG_SIZE);

    /* Mask all Status and clear status */
    memset((void *)(pdev_info->vbase+VCAP_STATUS_OFFSET(0)), 0xffffffff, VCAP_STATUS_REG_SIZE);

    /* Clear Channel Parameter memory */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(i == VCAP_CASCADE_CH_NUM)
            memset((void *)(pdev_info->vbase+VCAP_CAS_SUBCH_OFFSET(0)), 0, VCAP_CH_REG_SIZE);
        else
            memset((void *)(pdev_info->vbase+VCAP_CH_REG_OFFSET(i)), 0, VCAP_CH_REG_SIZE);
    }

#ifdef PLAT_SPLIT
    /* Clear Split Parameter memory */
    memset((void *)(pdev_info->vbase+VCAP_SPLIT_CH_OFFSET(0, 0)), 0, VCAP_SPLIT_REG_SIZE*VCAP_SPLIT_CH_MAX);
#endif
}

int vcap_dev_putjob(void *param)
{
    int len;
    int ret = 0;
    struct vcap_vg_job_item_t  *job = (struct vcap_vg_job_item_t *)param;
    struct vcap_vg_job_param_t *job_param = &job->param;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)job->private;
    struct vcap_path_t *ppath = &pdev_info->ch[job_param->ch].path[job_param->path];

    /* Check channel is active or not? */
    if(!pdev_info->ch[job_param->ch].active) {
        vcap_err("[%d] ch%d-p#%d put job failed!(channel not active)\n", pdev_info->index, job_param->ch, job_param->path);
        ret = -1;
        goto exit;
    }

    /*
     *  P2I Miss Top Field Bug, need to use path#3 as workaround path, so any path#3 job will be
     *  reject.
     */
    if(pdev_info->capability.bug_p2i_miss_top && (job_param->ch == VCAP_CASCADE_CH_NUM) && (job_param->path == 3)) {
        vcap_err("[%d] ch%d-p#%d put job failed!(path reserved)\n", pdev_info->index, job_param->ch, job_param->path);
        ret = -1;
        goto exit;
    }

    /* Check split channel job count */
    if(pdev_info->split.ch == job_param->ch) {
        int job_cnt = 0;
        struct vcap_vg_job_item_t *curr_item, *next_item;

        if(job_param->path >= VCAP_SPLIT_CH_SCALER_MAX) {
            vcap_err("[%d] ch%d-p#%d put job failed!(split channel path number invalid)\n", pdev_info->index, job_param->ch, job_param->path);
            ret = -1;
            goto exit;
        }

        list_for_each_entry_safe(curr_item, next_item, &job->m_job_head, m_job_list) {
            if(curr_item->param.split_ch > (pdev_info->split.x_num*pdev_info->split.y_num)) {
                vcap_err("[%d] ch%d-p#%d put job failed!(split channel number invalid)\n", pdev_info->index, job_param->ch, job_param->path);
                ret = -1;
                goto exit;
            }
            job_cnt++;
        }
        if(job_cnt != (pdev_info->split.x_num*pdev_info->split.y_num)) {
            vcap_err("[%d] ch%d-p#%d put job failed!(split job count not match %d != %d)\n",
                     pdev_info->index, job_param->ch, job_param->path, job_cnt, (pdev_info->split.x_num*pdev_info->split.y_num));
            ret = -1;
            goto exit;
        }
    }

    if(GET_DEV_CH_PATH_ST(pdev_info, job_param->ch, job_param->path) == VCAP_DEV_STATUS_IDLE ||
       GET_DEV_CH_PATH_ST(pdev_info, job_param->ch, job_param->path) == VCAP_DEV_STATUS_STOP) {
        ret = pdev_info->start(pdev_info, job_param->ch, job_param->path);
        if(ret < 0)
            goto exit;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
    len = kfifo_in_spinlocked(ppath->job_q, (unsigned char *)&job, sizeof(unsigned int), &ppath->job_q_lock);
#else
    len = kfifo_put(ppath->job_q, (unsigned char *)&job, sizeof(unsigned int));
#endif
    if(len != sizeof(int)) {
        vcap_err("[%d] ch%d-p#%d put job failed!\n", pdev_info->index, job_param->ch, job_param->path);
        ret = -1;
        goto exit;
    }

exit:
    return ret;
}

void *vcap_dev_getjob(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
    void *job;
    struct vcap_path_t *ppath_info = &pdev_info->ch[ch].path[path];

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
    if(kfifo_out_locked(ppath_info->job_q, (void *)&job, sizeof(unsigned int), &ppath_info->job_q_lock)< sizeof(unsigned int)) {
        vcap_err("[%d] ch#%d-p%d, FIFO is null!", pdev_info->index, ch, path);
        job = NULL;
    }
#else
    if(kfifo_get(ppath_info->job_q, (void *)&job, sizeof(unsigned int))< sizeof(unsigned int)) {
        vcap_err("[%d] ch#%d-p%d, FIFO is null!", pdev_info->index, ch, path);
        job = NULL;
    }
#endif

    return job;
}

int vcap_dev_stop(void *param, int ch, int path)
{
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)param;

    return pdev_info->stop(pdev_info, ch, path);
}

int vcap_dev_get_vi_mode(void *param, int ch)
{
    int vi_mode;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)param;
    struct vcap_dev_module_param_t *pm_param = pdev_info->m_param;

    switch(pm_param->vi_mode[CH_TO_VI(ch)]) {
        case VCAP_VI_RUN_MODE_2CH:
            vi_mode = VCAP_VG_VIMODE_2CH;
            break;
        case VCAP_VI_RUN_MODE_4CH:
            vi_mode = VCAP_VG_VIMODE_4CH;
            break;
        case VCAP_VI_RUN_MODE_SPLIT:
            vi_mode = VCAP_VG_VIMODE_SPLIT;
            break;
        case VCAP_VI_RUN_MODE_BYPASS:
        case VCAP_VI_RUN_MODE_DISABLE:
        default:
            vi_mode = VCAP_VG_VIMODE_BYPASS;
            break;
    }
    return vi_mode;
}

int vcap_dev_vi_ch_param_update(struct vcap_dev_info_t *pdev_info, int vi_idx, int vi_ch)
{
    int ret = 0;
    struct vcap_input_dev_t *pinput;
    struct vcap_vi_t        *pvi = &pdev_info->vi[vi_idx];

    /* update vi channel parameter */
    switch(pvi->tdm_mode) {
        case VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID:
            /* get vi input device information */
            pinput = vcap_input_get_info(vi_idx);
            if(!pinput)
                goto non_hybrid;

            pvi->ch_param[vi_ch].input_norm = pinput->ch_param[vi_ch].mode;
            pvi->ch_param[vi_ch].prog       = (pinput->ch_param[vi_ch].prog == 0) ? VCAP_VI_PROG_INTERLACE : VCAP_VI_PROG_PROGRESSIVE;
            pvi->ch_param[vi_ch].speed      = (pinput->ch_param[vi_ch].speed < VCAP_INPUT_SPEED_MAX) ? pinput->ch_param[vi_ch].speed : pvi->speed;
            pvi->ch_param[vi_ch].frame_rate = (pinput->ch_param[vi_ch].frame_rate > 0) ? pinput->ch_param[vi_ch].frame_rate : pvi->frame_rate;
            if(pinput->ch_param[vi_ch].width > 0)
                pvi->ch_param[vi_ch].src_w = ALIGN_4(pinput->ch_param[vi_ch].width);
            else
                pvi->ch_param[vi_ch].src_w = pvi->src_w;
            if(pinput->ch_param[vi_ch].height > 0)
                pvi->ch_param[vi_ch].src_h = (pinput->ch_param[vi_ch].prog == 0) ? (pinput->ch_param[vi_ch].height>>1) : pinput->ch_param[vi_ch].height;
            else
                pvi->ch_param[vi_ch].src_h = pvi->src_h;

            /* Update pixel/line lack error skip flag */
            pdev_info->ch[SUB_CH(vi_idx, vi_ch)].skip_pixel_lack = (pvi->src_w > pvi->ch_param[vi_ch].src_w) ? 1 : 0;
            pdev_info->ch[SUB_CH(vi_idx, vi_ch)].skip_line_lack  = (pvi->src_h > pvi->ch_param[vi_ch].src_h) ? 1 : 0;
            break;
        default:
non_hybrid:
            /* all vi channel parameter follow vi setting */
            pvi->ch_param[vi_ch].input_norm = pvi->input_norm;
            pvi->ch_param[vi_ch].src_w      = pvi->src_w;
            pvi->ch_param[vi_ch].src_h      = pvi->src_h;
            pvi->ch_param[vi_ch].prog       = pvi->prog;
            pvi->ch_param[vi_ch].speed      = pvi->speed;
            pvi->ch_param[vi_ch].frame_rate = pvi->frame_rate;

            /* Update pixel/line lack error skip flag */
            pdev_info->ch[SUB_CH(vi_idx, vi_ch)].skip_pixel_lack = 0;
            pdev_info->ch[SUB_CH(vi_idx, vi_ch)].skip_line_lack  = 0;
            break;
    }

    return ret;
}

int vcap_dev_vi_grab_hblank_setup(struct vcap_dev_info_t *pdev_info, int vi_idx, int grab_on)
{
    int vi_src_w, ret = 0;
    u32 tmp;
    struct vcap_vi_t *pvi = &pdev_info->vi[vi_idx];

    if(GET_DEV_VI_GST(pdev_info, vi_idx) == VCAP_DEV_STATUS_IDLE) {
        vcap_err("[%d] Grab hblank failed...VI#%d not inited!\n", pdev_info->index, vi_idx);
        return -1;
    }

    if(IS_DEV_VI_BUSY(pdev_info, vi_idx)) {
        vcap_err("[%d] Grab hblank failed...VI#%d is busy!\n", pdev_info->index, vi_idx);
        return -1;
    }

    if(((pdev_info->m_param)->vi_mode[vi_idx] == VCAP_VI_RUN_MODE_SPLIT) || (pdev_info->vi[vi_idx].format == VCAP_VI_FMT_RGB888)) {
        vcap_err("[%d] VI#%d format not support to grab horizontal blanking!\n", pdev_info->index, vi_idx);
        return -1;
    }

    /* VI source width */
    vi_src_w = (grab_on == 0) ? pvi->src_w : (ALIGN_4(pvi->src_w + VCAP_CH_HBLANK_DUMMY_PIXEL) & 0x1FFF);
    tmp = VCAP_REG_RD(pdev_info, ((vi_idx == VCAP_CASCADE_VI_NUM) ? VCAP_CAS_VI_OFFSET(0x04) : VCAP_VI_OFFSET(0x04 + vi_idx*0x8)));
    tmp &= ~0x1FFF;
    tmp |= vi_src_w;
    VCAP_REG_WR(pdev_info, ((vi_idx == VCAP_CASCADE_VI_NUM) ? VCAP_CAS_VI_OFFSET(0x04) : VCAP_VI_OFFSET(0x04 + vi_idx*0x8)), tmp);

    if(vi_idx == VCAP_RGB_VI_IDX) {
        /* Reciprocal for horizontal integer scaling */
        if(pvi->sd_hratio) {
            tmp = VCAP_REG_RD(pdev_info, VCAP_VI_OFFSET(0x48));
            tmp &= (~0xf0ff0000);
            tmp |= (((256/pvi->sd_hratio)<<16) | (pvi->sd_hratio<<28));
            VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(0x48), tmp);
        }

        /* After Size Down Width */
        tmp = VCAP_REG_RD(pdev_info, VCAP_VI_OFFSET(0x4c));
        tmp &= ~0x1fff;
        tmp |= vi_src_w;
        VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(0x4c), tmp);
    }

    /* Update VI Grab Mode */
    pvi->grab_mode = (grab_on == 0) ? VCAP_VI_GRAB_MODE_NORMAL : VCAP_VI_GRAB_MODE_HBLANK;

    return ret;
}

int vcap_dev_vi_setup(struct vcap_dev_info_t *pdev_info, int vi_idx)
{
    u32 tmp;
    int i, ch, ret = 0;
    int cfg_lcd_reg = 0;
    struct vcap_input_dev_t *pinput;
    struct vcap_vi_t        *pvi = &pdev_info->vi[vi_idx];

    /* get vi input device information */
    pinput = vcap_input_get_info(vi_idx);
    if(!pinput) {
        vcap_err("[%d] vi#%d input device not exit!\n", pdev_info->index, vi_idx);
        return -1;
    }

    /* check vi source width & height */
    if(!pinput->norm.width || !pinput->norm.height) {
        vcap_err("[%d] vi#%d input device source width/height invalid(%d, %d)!\n", pdev_info->index, vi_idx, pinput->norm.width, pinput->norm.height);
        return -1;
    }

    /* check vi RGB888 format support */
    if((pinput->interface == VCAP_INPUT_INTF_RGB888) &&
       ((vi_idx != VCAP_CASCADE_VI_NUM && vi_idx != VCAP_RGB_VI_IDX) ||
        (vi_idx == VCAP_CASCADE_VI_NUM && pdev_info->capability.cas_isp))) {
        vcap_err("[%d] vi#%d not support RGB888 interface format!\n", pdev_info->index, vi_idx);
        return -1;
    }

    /* check vi SDI8BIT format support */
    if(((pinput->interface == VCAP_INPUT_INTF_SDI8BIT_INTERLACE) || (pinput->interface == VCAP_INPUT_INTF_SDI8BIT_PROGRESSIVE)) &&
       (((pdev_info->capability.vi_sdi8bit == 0)  && (vi_idx != VCAP_CASCADE_VI_NUM)) ||
        ((pdev_info->capability.cas_sdi8bit == 0) && (vi_idx == VCAP_CASCADE_VI_NUM)))) {
        vcap_err("[%d] vi#%d not support SDI8BIT interface format!\n", pdev_info->index, vi_idx);
        return -1;
    }

    /* check vi BT601 format support */
    if(((pinput->interface >= VCAP_INPUT_INTF_BT601_8BIT_INTERLACE) && (pinput->interface <= VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE)) &&
       ((pdev_info->capability.vi_bt601 == 0) || (pdev_info->capability.vi_bt601 && (vi_idx == VCAP_CASCADE_VI_NUM)))) {
        vcap_err("[%d] vi#%d not support BT601 interface format!\n", pdev_info->index, vi_idx);
        return -1;
    }

    /* check ISP format support */
    if((pinput->interface == VCAP_INPUT_INTF_ISP) &&
       ((vi_idx != VCAP_CASCADE_VI_NUM) || ((vi_idx == VCAP_CASCADE_VI_NUM) && !pdev_info->capability.cas_isp))) {
        vcap_err("[%d] vi#%d not support ISP interface format!\n", pdev_info->index, vi_idx);
        return -1;
    }

    /* Interface */
    tmp = VCAP_REG_RD(pdev_info, ((vi_idx == VCAP_CASCADE_VI_NUM) ? VCAP_CAS_VI_OFFSET(0x00) : VCAP_VI_OFFSET(0x8*vi_idx)));
    tmp &= ~(BIT0|BIT1|BIT4|BIT23|BIT30);
    switch(pinput->interface) {
        case VCAP_INPUT_INTF_BT656_PROGRESSIVE:
            tmp |= BIT4;
            break;
        case VCAP_INPUT_INTF_BT1120_INTERLACE:
            tmp |= BIT0;
            break;
        case VCAP_INPUT_INTF_BT1120_PROGRESSIVE:
            tmp |= (BIT0|BIT4);
            break;
        case VCAP_INPUT_INTF_RGB888:
            if(pdev_info->capability.vi_sdi8bit)
                tmp |= (BIT0|BIT1|BIT4);
            else
                tmp |= (BIT1|BIT4);
            break;
        case VCAP_INPUT_INTF_SDI8BIT_INTERLACE:
            tmp |= BIT1;
            break;
        case VCAP_INPUT_INTF_SDI8BIT_PROGRESSIVE:
            tmp |= (BIT1|BIT4);
            break;
        case VCAP_INPUT_INTF_BT601_8BIT_INTERLACE:
        case VCAP_INPUT_INTF_BT601_16BIT_INTERLACE:
            tmp |= (BIT0|BIT1);
            if(pinput->bt601.vs_pol)
                tmp |= BIT23;
            if(pinput->bt601.hs_pol)
                tmp |= BIT30;
            break;
        case VCAP_INPUT_INTF_BT601_8BIT_PROGRESSIVE:
        case VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE:
            tmp |= (BIT0|BIT1|BIT4);
            if(pinput->bt601.vs_pol)
                tmp |= BIT23;
            if(pinput->bt601.hs_pol)
                tmp |= BIT30;
            break;
        case VCAP_INPUT_INTF_ISP:
            tmp |= BIT4;
            if(pdev_info->capability.cas_isp && pdev_info->capability.cas_sdi8bit)
                tmp |= (BIT0|BIT1);
            break;
        case VCAP_INPUT_INTF_BT656_INTERLACE:
        default:
            break;
    }

    /* Channel Reorder */
    tmp &= ~(0x3<<2);
    tmp |= (pinput->ch_reorder<<2);

    /* YC Swap */
    tmp &= ~(BIT5|BIT6);
    switch(pinput->yc_swap) {
        case VCAP_INPUT_SWAP_YC:
            tmp |= BIT5;
            break;
        case VCAP_INPUT_SWAP_CbCr:
            tmp |= BIT6;
            break;
        case VCAP_INPUT_SWAP_YC_CbCr:
            tmp |= (BIT5|BIT6);
            break;
        default:
            break;
    }

    /* Data Range */
    if(pinput->data_range == VCAP_INPUT_DATA_RANGE_240)
        tmp |= BIT7;
    else
        tmp &= ~BIT7;

    if(vi_idx != VCAP_CASCADE_VI_NUM) { ///< Cascade only support one channel
        /* TDM Mode */
        tmp &= ~(0x3<<8);
        tmp |= ((pinput->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID) ? (VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE<<8) : (pinput->mode<<8));

        /* Enable Auto Channel Switch */
        tmp |= BIT13;

        /* Channel ID Extra Mode */
        tmp &= ~(0x3<<14);
        tmp |= (pinput->ch_id_mode<<14);
    }
    else
        tmp &= ~(0xfff<<8);

    /* Capture Style */
    tmp &= ~(0x7<<20);
    if((pinput->interface == VCAP_INPUT_INTF_BT656_INTERLACE)     ||
       (pinput->interface == VCAP_INPUT_INTF_BT1120_INTERLACE)    ||
       (pinput->interface == VCAP_INPUT_INTF_SDI8BIT_INTERLACE)   ||
       (pinput->interface == VCAP_INPUT_INTF_BT601_8BIT_INTERLACE)||
       (pinput->interface == VCAP_INPUT_INTF_BT601_16BIT_INTERLACE)) {
        switch(pinput->field_order) {
            case VCAP_INPUT_ORDER_ODD_FIRST:
                tmp |= (VCAP_VI_CAP_STYLE_ODD_FIRST<<20);
                break;
            case VCAP_INPUT_ORDER_EVEN_FIRST:
                tmp |= (VCAP_VI_CAP_STYLE_EVEN_FIRST<<20);
                break;
            default:
                tmp |= (VCAP_VI_CAP_STYLE_ANYONE<<20);
                break;
        }
    }
    else
        tmp |= (VCAP_VI_CAP_STYLE_ANYONE<<20);

    /* PCLK Divide Value */
    pvi->pclk_div = 0x8;
    tmp &= ~(0x3f<<24);
    tmp |= (pvi->pclk_div<<24);

    /* VBI Extract */
    if(pinput->vbi.enable)
        tmp |= BIT31;
    else
        tmp &= ~BIT31;

    VCAP_REG_WR(pdev_info, ((vi_idx == VCAP_CASCADE_VI_NUM) ? VCAP_CAS_VI_OFFSET(0x00) : VCAP_VI_OFFSET(0x8*vi_idx)), tmp);

    /* Source Width and Height */
    if((pinput->interface == VCAP_INPUT_INTF_BT656_INTERLACE)     ||
       (pinput->interface == VCAP_INPUT_INTF_BT1120_INTERLACE)    ||
       (pinput->interface == VCAP_INPUT_INTF_SDI8BIT_INTERLACE)   ||
       (pinput->interface == VCAP_INPUT_INTF_BT601_8BIT_INTERLACE)||
       (pinput->interface == VCAP_INPUT_INTF_BT601_16BIT_INTERLACE)) {
        tmp = ((((pinput->norm.height>>1) & 0x1FFF)<<16) | (ALIGN_4(pinput->norm.width) & 0x1FFF));
    }
    else
        tmp = (((pinput->norm.height & 0x1FFF)<<16) | (ALIGN_4(pinput->norm.width) & 0x1FFF));

    /* BT601 parameter */
    if((pinput->interface >= VCAP_INPUT_INTF_BT601_8BIT_INTERLACE) && (pinput->interface <= VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE)) {
        /* sync mode */
        if(pinput->bt601.sync_mode)
            tmp |= BIT30;

        /* 8/16bit format */
        if((pinput->interface == VCAP_INPUT_INTF_BT601_16BIT_INTERLACE) || (pinput->interface == VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE))
            tmp |= BIT31;
    }
    VCAP_REG_WR(pdev_info, ((vi_idx == VCAP_CASCADE_VI_NUM) ? VCAP_CAS_VI_OFFSET(0x04) : VCAP_VI_OFFSET(0x04 + vi_idx*0x8)), tmp);

    if(vi_idx != VCAP_CASCADE_VI_NUM) {
        /* VBI Start Line */
        tmp = VCAP_REG_RD(pdev_info, VCAP_VI_OFFSET(0x58 + ((vi_idx/4)*0x4)));
        tmp &= ~(0xff<<((vi_idx%4)*0x8));
        if(pinput->vbi.enable)
            tmp |= (pinput->vbi.start_line<<((vi_idx%4)*0x8));
        VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(0x58 + ((vi_idx/4)*0x4)), tmp);

        /* BT601 H/V Valid Data Offset */
        if((pinput->interface >= VCAP_INPUT_INTF_BT601_8BIT_INTERLACE) && (pinput->interface <= VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE)) {
            tmp = VCAP_REG_RD(pdev_info, VCAP_VI_OFFSET(0x68));
            tmp &= (~0xffff);
            tmp |= ((pinput->bt601.x_offset) | (pinput->bt601.y_offset<<8));
            VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(0x68), tmp);
        }

        if(vi_idx == VCAP_RGB_VI_IDX)
            cfg_lcd_reg = 1;
    }
    else {
        if(!pdev_info->capability.cas_isp)
            cfg_lcd_reg = 1;
        else if(pdev_info->capability.cas_sdi8bit) {
            /* VBI Start Line */
            tmp = VCAP_REG_RD(pdev_info, VCAP_VI_OFFSET(0x60));
            tmp &= (~0xff);
            if(pinput->vbi.enable)
                tmp |= pinput->vbi.start_line;
            VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(0x60), tmp);
        }
    }

    if(cfg_lcd_reg) {   ///< only Cascade support RGB888 format
        tmp = 0;
        /* V/S Polarity */
        if(pinput->rgb.vs_pol)
            tmp |= BIT0;

        /* H/S Polarity */
        if(pinput->rgb.hs_pol)
            tmp |= BIT1;

        /* Data Enable Polarity */
        if(pinput->rgb.de_pol)
            tmp |= BIT2;

        /* Watch Data Enable */
        if(pinput->rgb.watch_de)
            tmp |= BIT3;

        /* Reciprocal for horizontal integer scaling */
        if(pinput->rgb.sd_hratio == 0)
            pinput->rgb.sd_hratio++;
        pinput->rgb.sd_hratio &= 0xf;
        tmp |= ((256/pinput->rgb.sd_hratio)<<16);

        /* Horizontal integer scaling ratio */
        tmp |= (pinput->rgb.sd_hratio<<28);
        VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(0x48), tmp);

        /* After Size Down Width */
        tmp = VCAP_REG_RD(pdev_info, VCAP_VI_OFFSET(0x4c));
        tmp &= ~0x1fff;
        tmp |= (ALIGN_4(pinput->norm.width)/pinput->rgb.sd_hratio);

        /* VBI Start Line */
        tmp &= (~(0xf<<16));
        if(pinput->vbi.enable)
            tmp |= (pinput->vbi.start_line<<16);
        VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(0x4c), tmp);
    }
    else {
        if((vi_idx == VCAP_RGB_VI_IDX) &&
           ((pinput->interface == VCAP_INPUT_INTF_SDI8BIT_INTERLACE) ||
            (pinput->interface == VCAP_INPUT_INTF_SDI8BIT_INTERLACE))) {    ///< fix GM8287 RGB_VI SDI8BIT format not work problem
            /* After Size Down Width */
            tmp = VCAP_REG_RD(pdev_info, VCAP_VI_OFFSET(0x4c));
            tmp &= ~0x1fff;
            tmp |= ALIGN_4(pinput->norm.width);
            VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(0x4c), tmp);
        }
    }

    /* vi speed type */
    pvi->speed = pinput->speed;

    /* vi interface format */
    switch(pinput->interface) {
        case VCAP_INPUT_INTF_BT656_PROGRESSIVE:
            pvi->format = VCAP_VI_FMT_BT656;
            pvi->prog   = VCAP_VI_PROG_PROGRESSIVE;
            break;
        case VCAP_INPUT_INTF_BT1120_INTERLACE:
            pvi->format = VCAP_VI_FMT_BT1120;
            pvi->prog   = VCAP_VI_PROG_INTERLACE;
            break;
        case VCAP_INPUT_INTF_BT1120_PROGRESSIVE:
            pvi->format = VCAP_VI_FMT_BT1120;
            pvi->prog   = VCAP_VI_PROG_PROGRESSIVE;
            break;
        case VCAP_INPUT_INTF_RGB888:
            pvi->format = VCAP_VI_FMT_RGB888;
            pvi->prog   = VCAP_VI_PROG_PROGRESSIVE;
            break;
        case VCAP_INPUT_INTF_SDI8BIT_INTERLACE:
            pvi->format = VCAP_VI_FMT_SDI8BIT;
            pvi->prog   = VCAP_VI_PROG_INTERLACE;
            break;
        case VCAP_INPUT_INTF_SDI8BIT_PROGRESSIVE:
            pvi->format = VCAP_VI_FMT_SDI8BIT;
            pvi->prog   = VCAP_VI_PROG_PROGRESSIVE;
            break;
        case VCAP_INPUT_INTF_BT601_8BIT_INTERLACE:
            pvi->format = VCAP_VI_FMT_BT601_8BIT;
            pvi->prog   = VCAP_VI_PROG_INTERLACE;
            break;
        case VCAP_INPUT_INTF_BT601_8BIT_PROGRESSIVE:
            pvi->format = VCAP_VI_FMT_BT601_8BIT;
            pvi->prog   = VCAP_VI_PROG_PROGRESSIVE;
            break;
        case VCAP_INPUT_INTF_BT601_16BIT_INTERLACE:
            pvi->format = VCAP_VI_FMT_BT601_16BIT;
            pvi->prog   = VCAP_VI_PROG_INTERLACE;
            break;
        case VCAP_INPUT_INTF_BT601_16BIT_PROGRESSIVE:
            pvi->format = VCAP_VI_FMT_BT601_16BIT;
            pvi->prog   = VCAP_VI_PROG_PROGRESSIVE;
            break;
        case VCAP_INPUT_INTF_ISP:
            pvi->format = VCAP_VI_FMT_ISP;
            pvi->prog   = VCAP_VI_PROG_PROGRESSIVE;
            break;
        default:
            pvi->format = VCAP_VI_FMT_BT656;
            pvi->prog   = VCAP_VI_PROG_INTERLACE;
            break;
    }

    /* vi source width and height */
    pvi->src_w = ALIGN_4(pinput->norm.width);
    pvi->src_h = (pvi->prog == VCAP_VI_PROG_INTERLACE) ? (pinput->norm.height>>1) : pinput->norm.height;

    /* Capture Style */
    if(pvi->prog == VCAP_VI_PROG_INTERLACE) {
        switch(pinput->field_order) {
            case VCAP_INPUT_ORDER_ODD_FIRST:
                pvi->cap_style = VCAP_VI_CAP_STYLE_ODD_FIRST;
                break;
            case VCAP_INPUT_ORDER_EVEN_FIRST:
                pvi->cap_style = VCAP_VI_CAP_STYLE_EVEN_FIRST;
                break;
            default:
                pvi->cap_style = VCAP_VI_CAP_STYLE_ANYONE;
                break;
        }
    }
    else
        pvi->cap_style = VCAP_VI_CAP_STYLE_ANYONE;

    /* TDM Mode */
    pvi->tdm_mode = pinput->mode;

    /* Data Range */
    pvi->data_range = pinput->data_range;

    /* VBI */
    pvi->vbi_enable     = pinput->vbi.enable;
    pvi->vbi_width      = ALIGN_4(pinput->vbi.width);
    pvi->vbi_height     = pinput->vbi.height;
    pvi->vbi_start_line = pinput->vbi.start_line;

    /* Cascade SD Ratio */
    pvi->sd_hratio = pinput->rgb.sd_hratio;

    /* Norm Mode */
    pvi->input_norm = pinput->norm.mode;

    /* Frame Rate */
    if(!pinput->frame_rate)
        pvi->frame_rate = 30;       ///< default frame rate 30fps
    else
        pvi->frame_rate = pinput->frame_rate;

    /* reset demux for re-sync */
    if(pvi->tdm_mode == VCAP_VI_TDM_MODE_4CH_BYTE_INTERLEAVE)
        vcap_dev_demux_reset(pdev_info, vi_idx);

    /* Channel SC Mode & VI Channel Parameter */
    for(i=0; i<VCAP_VI_CH_MAX; i++) {
        ch = SUB_CH(vi_idx, i);
        if(ch >= VCAP_CHANNEL_MAX || !pdev_info->ch[ch].active)
            continue;

        /* VI channel parameter */
        vcap_dev_vi_ch_param_update(pdev_info, vi_idx, i);

        if(((pvi->prog == VCAP_VI_PROG_PROGRESSIVE) && ((pvi->tdm_mode != VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE) && (pvi->tdm_mode != VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID))) ||
           (ch == VCAP_CASCADE_CH_NUM) ||
           (pdev_info->split.ch == ch)) {
            if((pvi->format == VCAP_VI_FMT_ISP) || ((ch == VCAP_CASCADE_CH_NUM) && pdev_info->capability.cas_isp))
                pdev_info->ch[ch].sc_rolling = VCAP_SC_ROLLING_1024;    ///< ISP VI must set rolling to 1024
            else
                pdev_info->ch[ch].sc_rolling = VCAP_SC_ROLLING_4096;
        }
        else {
            switch(pvi->tdm_mode) {
                case VCAP_VI_TDM_MODE_BYPASS:
                    pdev_info->ch[ch].sc_rolling = VCAP_SC_ROLLING_4096;
                    break;
                case VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE:
                case VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID:
                    if(pdev_info->capability.bug_ch14_rolling && (CH_TO_VI(ch) == VCAP_RGB_VI_IDX) && (ch == 14))
                        pdev_info->ch[ch].sc_rolling = VCAP_SC_ROLLING_1024;    ///< GM8287, VI#3=> CH#14 sc rolling must set 1024, hardware bug
                    else
                        pdev_info->ch[ch].sc_rolling = VCAP_SC_ROLLING_2048;
                    break;
                default:
                    pdev_info->ch[ch].sc_rolling = VCAP_SC_ROLLING_1024;
                    break;
            }
        }

        if(pdev_info->split.ch == ch)
            pdev_info->ch[ch].sc_type = VCAP_SC_TYPE_SPLIT;
        else
            pdev_info->ch[ch].sc_type = VCAP_SC_TYPE_INPUT;
    }

    /* VI Grab Mode */
    pvi->grab_mode = VCAP_VI_GRAB_MODE_NORMAL;

    return ret;
}

void vcap_dev_sync_timer_init(struct vcap_dev_info_t *pdev_info)
{
    u32 tmp;
    struct vcap_dev_module_param_t *pm_param = pdev_info->m_param;

    /* Enable sync timer clock gating */
    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x04));
    tmp |= BIT1;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x04), tmp);

    /* disable sync timer */
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x60), 0);

    /* sync timer div */
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x60), (VCAP_CLK_IN/pm_param->sync_time_div));  ///< default, 60 fps

#ifdef PLAT_SYNC_CLK_SEL
    /* sync timer source */
    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x64));
    tmp &= (~0xf);
    tmp |= VCAP_SYNC_TIMER_SRC_ACLK;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x64), tmp);
#endif

    /* Disable sync timer clock gating */
    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x04));
    tmp &= (~BIT1);
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x04), tmp);
}

void vcap_dev_capture_enable(struct vcap_dev_info_t *pdev_info)
{
    u32 tmp;

    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x04));
    tmp |= BIT31;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x04), tmp);
}

void vcap_dev_capture_disable(struct vcap_dev_info_t *pdev_info)
{
    u32 tmp;

    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x04));
    tmp &= ~BIT31;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x04), tmp);
}

void vcap_dev_ch_enable(struct vcap_dev_info_t *pdev_info, int ch)
{
    u32 tmp;

    if(ch == VCAP_CASCADE_CH_NUM) {
        tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x0c));
        tmp |= BIT0;
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x0c), tmp);
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x08));
        tmp |= 0x1<<ch;
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x08), tmp);
    }
}

void vcap_dev_ch_disable(struct vcap_dev_info_t *pdev_info, int ch)
{
    u32 tmp;

    if(ch == VCAP_CASCADE_CH_NUM) {
        tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x0c));
        tmp &= ~BIT0;
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x0c), tmp);
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x08));
        tmp &= ~(0x1<<ch);
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x08), tmp);
    }
}

void vcap_dev_rgb2yuv_matrix_init(struct vcap_dev_info_t *pdev_info)
{
#ifdef PLAT_RGB2YUV
    u32 tmp;
    u8  a0[3] = {0x28, 0x48, 0x10};
    u8  a1[3] = {0xea, 0xd9, 0x3d};
    u8  a2[3] = {0x52, 0xc0, 0xee};

    tmp = VCAP_REG_RD(pdev_info, VCAP_VI_OFFSET(0x4c));
    tmp &= ~(0x3f<<24);
    tmp |= (a0[0]<<24);
    VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(0x4c), tmp);

    tmp = (a1[1]<<24) | (a1[0]<<16) | (a0[2]<<8) | a0[1];
    VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(0x50), tmp);

    tmp = (a2[2]<<24) | (a2[1]<<16) | (a2[0]<<8) | a1[2];
    VCAP_REG_WR(pdev_info, VCAP_VI_OFFSET(0x54), tmp);
#endif
    return;
}

void vcap_dev_update_status(struct vcap_dev_info_t *pdev_info, int ch)
{
    int i, j, sub_ch;
    int p_idle, p_stop, p_start;
    int ch_idle, ch_stop, ch_start;
    int vi_idle, vi_stop, vi_start;
    int vi_idx = CH_TO_VI(ch);

    ch_idle = ch_stop = ch_start = 0;
    for(i=0; i<VCAP_VI_CH_MAX; i++) {
        sub_ch = SUB_CH(vi_idx, i);
        if(sub_ch >= VCAP_CHANNEL_MAX)
            break;

        p_idle = p_start = p_stop = 0;
        for(j=0; j<VCAP_SCALER_MAX; j++) {
            switch(GET_DEV_CH_PATH_ST(pdev_info, sub_ch, j)) {
                case VCAP_DEV_STATUS_IDLE:
                    p_idle++;
                    break;
                case VCAP_DEV_STATUS_START:
                    p_start++;
                    goto ch_update;
                case VCAP_DEV_STATUS_STOP:
                    p_stop++;
                    break;
            }
        }

ch_update:
        if(p_start) {
            if(GET_DEV_CH_ST(pdev_info, sub_ch) != VCAP_DEV_STATUS_START)
                SET_DEV_CH_ST(pdev_info, sub_ch, VCAP_DEV_STATUS_START);
            ch_start++;
        }
        else {
            if(p_stop) {
                if(GET_DEV_CH_ST(pdev_info, sub_ch) != VCAP_DEV_STATUS_STOP)
                    SET_DEV_CH_ST(pdev_info, sub_ch, VCAP_DEV_STATUS_STOP);
                ch_stop++;
            }
            else {
                if(GET_DEV_CH_ST(pdev_info, sub_ch) != VCAP_DEV_STATUS_IDLE)
                    SET_DEV_CH_ST(pdev_info, sub_ch, VCAP_DEV_STATUS_IDLE);
                ch_idle++;
            }
        }
    }

    /* VI status update */
    if(ch_start) {
        if(GET_DEV_VI_ST(pdev_info, vi_idx) != VCAP_DEV_STATUS_START)
            SET_DEV_VI_ST(pdev_info, vi_idx, VCAP_DEV_STATUS_START);
    }
    else {
        if(ch_stop) {
            if(GET_DEV_VI_ST(pdev_info, vi_idx) != VCAP_DEV_STATUS_STOP)
                SET_DEV_VI_ST(pdev_info, vi_idx, VCAP_DEV_STATUS_STOP);
        }
        else {
            if(GET_DEV_VI_ST(pdev_info, vi_idx) != VCAP_DEV_STATUS_IDLE)
                SET_DEV_VI_ST(pdev_info, vi_idx, VCAP_DEV_STATUS_IDLE);
        }
    }

    /* update device status */
    vi_idle = vi_stop = vi_start = 0;
    for(i=0; i<VCAP_VI_MAX; i++) {
        switch(GET_DEV_VI_ST(pdev_info, i)) {
            case VCAP_DEV_STATUS_IDLE:
                vi_idle++;
                break;
            case VCAP_DEV_STATUS_START:
                vi_start++;
                goto dev_update;
            case VCAP_DEV_STATUS_STOP:
                vi_stop++;
                break;
        }
    }

dev_update:
    if(vi_start) {
        if(GET_DEV_ST(pdev_info) != VCAP_DEV_STATUS_START)
            SET_DEV_ST(pdev_info, VCAP_DEV_STATUS_START);
    }
    else {
        if(vi_stop) {
            if(GET_DEV_ST(pdev_info) != VCAP_DEV_STATUS_STOP)
                SET_DEV_ST(pdev_info, VCAP_DEV_STATUS_STOP);
        }
        else {
            if(GET_DEV_ST(pdev_info) != VCAP_DEV_STATUS_IDLE)
                SET_DEV_ST(pdev_info, VCAP_DEV_STATUS_IDLE);
        }
    }
}

void vcap_dev_single_fire(struct vcap_dev_info_t *pdev_info)
{
    u32  tmp;

    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x04));
    if(tmp & BIT28)
        vcap_err("[%d] fire bit not clear before next fire\n", pdev_info->index);

    tmp |= BIT28;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x04), tmp);
}

void vcap_dev_global_setup(struct vcap_dev_info_t *pdev_info)
{
    u32 tmp;
    int i;
    int ch_sd_base[VCAP_VI_MAX];
#ifdef PLAT_CH_MODE_SEL
    int vi_ch[VCAP_VI_MAX];
    struct vcap_dev_module_param_t *pm_param = pdev_info->m_param;
    int ch_cnt = 0, vi_cnt = 0;
#endif
    int sd_base = 0;

#ifdef PLAT_CH_MODE_SEL
    /* Determining channel running mode */
    for(i=0; i<VCAP_VI_MAX; i++) {
        switch(pm_param->vi_mode[i]) {
            case VCAP_VI_RUN_MODE_BYPASS:
            case VCAP_VI_RUN_MODE_SPLIT:
                vi_ch[i] = 1;
                break;
            case VCAP_VI_RUN_MODE_2CH:
                vi_ch[i] = 2;
                break;
            case VCAP_VI_RUN_MODE_4CH:
                vi_ch[i] = 4;
                break;
            default:
                vi_ch[i] = 0;
                break;
        }
        if(vi_ch[i]) {
            ch_cnt += ((i == VCAP_CASCADE_VI_NUM) ? 3 : vi_ch[i]);    ///< for cascade to max support 1920 width, need 3 channel sd line buffer
            vi_cnt += ((i <= 3) ? 1 : 5);
        }
    }

    if(ch_cnt > VCAP_SCALER_LINE_BUF_NUM)
        pdev_info->ch_mode = VCAP_CH_MODE_32CH;    ///< hardware line buffer limitation in CH32 Mode, not support scaling up
    else {
        if(vi_cnt <= 4) ///< only VI#0~3 can enable 16CH mode
            pdev_info->ch_mode = VCAP_CH_MODE_16CH;
        else
            pdev_info->ch_mode = VCAP_CH_MODE_NORMAL;
    }

    /* Determining channel sd line buffer base */
    switch(pdev_info->ch_mode) {
        case VCAP_CH_MODE_16CH:
            for(i=0; i<VCAP_VI_MAX; i++) {
                if(vi_ch[i] == 0)
                    ch_sd_base[i] = 0;
                else {
                    if(i < 4 || i == VCAP_CASCADE_VI_NUM) {
                        ch_sd_base[i] = sd_base;
                        sd_base += 960;     ///< default for max support 4CH 960 width
                    }
                    else
                        ch_sd_base[i] = 0;
                }
            }
            break;
        case VCAP_CH_MODE_32CH:
            for(i=0; i<VCAP_VI_MAX; i++)
                ch_sd_base[i] = 0;  ///< in ch32 mode, hardware will take care sd base, one channel use one line buffer
            break;
        default:
            for(i=0; i<VCAP_VI_MAX; i++) {
                if(vi_ch[i] == 0)
                    ch_sd_base[i] = 0;
                else {
                    ch_sd_base[i] = sd_base;

                    if(vi_ch[i] == 4)
                        sd_base += VCAP_SCALER_LINE_BUF_WIDTH;                                          ///< default for max support 4CH 720 width
                    else if(vi_ch[i] == 2)
                        sd_base += (((pm_param->vi_max_w[i] > 0) ? pm_param->vi_max_w[i] : 960)>>1);    ///< default for max support 2CH 960 width
                    else
                        sd_base += (((pm_param->vi_max_w[i] > 0) ? pm_param->vi_max_w[i] : 1920)>>2);   ///< default for max support 1CH 1920 width
                }
            }
            break;
    }

    /* check sd base over size or not? */
    if(sd_base > (VCAP_SCALER_LINE_BUF_SIZE>>3)) {
        pdev_info->ch_mode = VCAP_CH_MODE_32CH;
        for(i=0; i<VCAP_VI_MAX; i++)
            ch_sd_base[i] = 0;
    }
#else
    pdev_info->ch_mode = VCAP_CH_MODE_NORMAL;

    /* Determining channel sd line buffer base */
    for(i=0; i<VCAP_VI_MAX; i++) {
        ch_sd_base[i] = sd_base;
        sd_base += VCAP_SCALER_LINE_BUF_WIDTH;
    }
#endif

    /* Enable Info Keep Trick */
    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x00));
    tmp |= BIT24;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x00), tmp);

    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x04));

    /* Disable LL Channel Trick */
    tmp &= ~BIT23;

    /* Channel Mode */
    tmp &= ~(0x3<<24);
    tmp |= (pdev_info->ch_mode<<24);

    /* Capture Mode */
    tmp &= ~(0x3<<26);
    if(pdev_info->dev_type == VCAP_DEV_DRV_TYPE_LLI)
        tmp |= BIT26;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x04), tmp);

    /* SD Base */
    for(i=0; i<VCAP_VI_MAX; i++) {
        if(i == VCAP_CASCADE_VI_NUM) {
            tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x58));
            tmp &= (~0xffff);
            tmp |= ch_sd_base[i];
            VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x58), tmp);
        }
        else {
            tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x48+((i/2)*4)));
            tmp &= ~(0xffff<<((i%2)*16));
            tmp |= (ch_sd_base[i]<<((i%2)*16));
            VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x48+((i/2)*4)), tmp);
        }
    }

    /* Split */
    if(pdev_info->split.ch >= 0) {
        u32 sd_split_en[2]     = {0, 0};
        u32 sd_split_sel[4]    = {0, 0, 0, 0};

        for(i=0; i<pdev_info->split.x_num*pdev_info->split.y_num; i++) {
            sd_split_en[i/16]     |= (0x3<<((i%16)*2));
            sd_split_sel[(i/8)]   |= (0x4<<((i%8)*4));
        }

        /* SD Split Enable */
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x10), sd_split_en[0]);
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x14), sd_split_en[1]);

        /* SD Split Bypass */
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x18), 0);
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x1c), 0);

        /* SD Split Select */
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x20), sd_split_sel[0]);
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x24), sd_split_sel[1]);
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x28), sd_split_sel[2]);
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x2c), sd_split_sel[3]);

        /* Split Block Number, Split VI */
        tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x30));
        tmp &= ~0xf7f;
        tmp |= (pdev_info->split.x_num*pdev_info->split.y_num) | ((CH_TO_VI(pdev_info->split.ch)+1)<<8);
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x30), tmp);
    }

    /* DMA Overflow Hack Mode, 0:skip the new come in line 1:mask related channel in current frame */
    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x44));
    tmp &= ~BIT31;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x44), tmp);

#ifdef PLAT_OSD_COLOR_SCHEME
    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x68));
    tmp &= ~(0x3<<4);
    switch((pdev_info->m_param)->accs_func) {
        case VCAP_ACCS_FUNC_HALF_MARK_MEM:
            tmp |= (0x2<<4);
            break;
        case VCAP_ACCS_FUNC_FULL_MARK_MEM:
            tmp |= (0x3<<4);
            break;
        default:
            break;
    }
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x68), tmp);
#endif
}

int vcap_dev_sc_setup(struct vcap_dev_info_t *pdev_info, int ch)
{
    u32    tmp;
    int    i, ch_src_w, ch_src_h, h_div;
    int    set_sc_x = 1;
    int    vi_idx   = CH_TO_VI(ch);
    int    vi_ch    = SUBCH_TO_VICH(ch);
    struct vcap_vi_t       *pvi = &pdev_info->vi[vi_idx];
    struct vcap_ch_param_t *pcurr_param = &pdev_info->ch[ch].param;
    struct vcap_ch_param_t *ptemp_param = &pdev_info->ch[ch].temp_param;

    if(ch >= VCAP_CHANNEL_MAX)
        return -1;

    if(GET_DEV_VI_GST(pdev_info, vi_idx) == VCAP_DEV_STATUS_IDLE)
        return -1;

    if(IS_DEV_CH_BUSY(pdev_info, ch))
        return -1;

    if((pdev_info->ch[ch].sc_type == VCAP_SC_TYPE_SPLIT) && (pdev_info->split.ch == ch)) {
        ptemp_param->sc.x_start = pdev_info->split.x_num;
        ptemp_param->sc.y_start = pdev_info->split.y_num;
        if(pvi->format == VCAP_VI_FMT_RGB888) {
            ptemp_param->sc.width  = ALIGN_4((pvi->src_w/pvi->sd_hratio)/pdev_info->split.x_num);
            ptemp_param->sc.height = pvi->src_h/pdev_info->split.y_num;
        }
        else {
            ptemp_param->sc.width  = ALIGN_4(pvi->src_w/pdev_info->split.x_num);
            ptemp_param->sc.height = pvi->src_h/pdev_info->split.y_num;
        }

        tmp = (ptemp_param->sc.x_start)   |
              (ptemp_param->sc.width<<16) |
              (pdev_info->ch[ch].sc_rolling<<14);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x00) : VCAP_SC_OFFSET(0x00 + ch*0x8)), tmp);

        tmp = (ptemp_param->sc.y_start)    |
              (ptemp_param->sc.height<<16) |
              (pdev_info->ch[ch].sc_type<<14);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x04) : VCAP_SC_OFFSET(0x04 + ch*0x8)), tmp);

        /* sync sc parameter */
        pcurr_param->sc.x_start = ptemp_param->sc.x_start;
        pcurr_param->sc.y_start = ptemp_param->sc.y_start;
        pcurr_param->sc.width   = ptemp_param->sc.width;
        pcurr_param->sc.height  = ptemp_param->sc.height;
    }
    else {
        ptemp_param->sc.x_start = 0;
        ptemp_param->sc.y_start = 0;
        if(pvi->format == VCAP_VI_FMT_RGB888) {
            ptemp_param->sc.width   = ALIGN_4(pvi->src_w/pvi->sd_hratio);
            ptemp_param->sc.height  = pvi->src_h;
        }
        else {
            ptemp_param->sc.width   = (pvi->tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) ? pvi->ch_param[vi_ch].src_w : pvi->src_w;
            ptemp_param->sc.height  = (pvi->tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) ? pvi->ch_param[vi_ch].src_h : pvi->src_h;
        }

        /* apply horizontal source cropping rule */
        ch_src_w = (pvi->tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) ? pvi->ch_param[vi_ch].src_w : pvi->src_w;
        for(i=0; i<VCAP_SC_RULE_MAX; i++) {
            if(ch_src_w == ((pdev_info->m_param)->hcrop_rule[i].in_size)) {
                ptemp_param->sc.width   = (pdev_info->m_param)->hcrop_rule[i].out_size;
                ptemp_param->sc.x_start = (ch_src_w - (pdev_info->m_param)->hcrop_rule[i].out_size)>>1;   ///< for center
                break;
            }
        }

        /* apply vertical source cropping rule */
        if(pvi->tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) {
            ch_src_h = pvi->ch_param[vi_ch].src_h;
            h_div    = (pvi->ch_param[vi_ch].prog == VCAP_VI_PROG_INTERLACE) ? 2 : 1;
        }
        else {
            ch_src_h = pvi->src_h;
            h_div    = (pvi->prog == VCAP_VI_PROG_INTERLACE) ? 2 : 1;
        }
        for(i=0; i<VCAP_SC_RULE_MAX; i++) {
            if(ch_src_h == ((pdev_info->m_param)->vcrop_rule[i].in_size/h_div)) {
                if(((pdev_info->m_param)->vcrop_rule[i].out_size/h_div) > 0) {
                    ptemp_param->sc.height = (pdev_info->m_param)->vcrop_rule[i].out_size/h_div;
                    break;
                }
            }
        }

        /* sc y start and end position */
        tmp = (ptemp_param->sc.y_start) |
              ((ptemp_param->sc.y_start + ptemp_param->sc.height - 1)<<16) |
              (pdev_info->ch[ch].sc_type<<14);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x04) : VCAP_SC_OFFSET(0x04 + ch*0x8)), tmp);

        /* sync sc parameter */
        pcurr_param->sc.y_start = ptemp_param->sc.y_start;
        pcurr_param->sc.height  = ptemp_param->sc.height;

#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
        /* 2ch dual edge mode, vi-ch#0 sc switch must wait vi-ch#2 in disable state,
         * hardware reference sc width to caculate line buffer offset address dynamicly
         */
        if((pdev_info->vi[vi_idx].tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) &&
           ((ch != VCAP_CASCADE_CH_NUM) && ((ch%VCAP_VI_CH_MAX) == 0)) &&
           VCAP_IS_CH_ENABLE(pdev_info, SUB_CH(vi_idx, 2))) {
            set_sc_x = 0;
        }
#endif

        /* sc x start and end position */
        if(set_sc_x) {
            tmp = (ptemp_param->sc.x_start) |
                  ((ptemp_param->sc.x_start + ptemp_param->sc.width - 1)<<16) |
                  (pdev_info->ch[ch].sc_rolling<<14);
            VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x00) : VCAP_SC_OFFSET(0x00 + ch*0x8)), tmp);

            /* sync sc parameter */
            pcurr_param->sc.x_start = ptemp_param->sc.x_start;
            pcurr_param->sc.width   = ptemp_param->sc.width;
        }
    }

    return 0;
}

int vcap_dev_param_set_default(struct vcap_dev_info_t *pdev_info, int vi_idx)
{
    u32 tmp;
    int i, j, ch;
    struct vcap_ch_param_t *pcurr_param;
    struct vcap_ch_param_t *ptemp_param;

    if(GET_DEV_VI_GST(pdev_info, vi_idx) == VCAP_DEV_STATUS_IDLE) {
        vcap_err("[%d] VI#%d not inited!\n", pdev_info->index, vi_idx);
        return -1;
    }

    if(IS_DEV_VI_BUSY(pdev_info, vi_idx)) {
        vcap_err("[%d] VI#%d is busy!\n", pdev_info->index, vi_idx);
        return -1;
    }

    for(i=0; i<VCAP_VI_CH_MAX; i++) {
        ch = SUB_CH(vi_idx, i);
        if(ch >= VCAP_CHANNEL_MAX)
            break;

        pcurr_param = &pdev_info->ch[ch].param;
        ptemp_param = &pdev_info->ch[ch].temp_param;

        /* Clear Parameter */
        memset(pcurr_param, 0, sizeof(struct vcap_ch_param_t));

        if(pdev_info->ch[ch].active) {
            /* Sub_CH */
            if(pdev_info->vi[vi_idx].cap_style == VCAP_VI_CAP_STYLE_EVEN_FIRST)
                pcurr_param->comm.field_order = 1;  ///< used for grab field pair in interlace mode

            /* Reg 0x00 */
            VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x00) : VCAP_CH_SUBCH_OFFSET(ch, 0x00)), 0);

            /* Reg 0x04 */
            tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x04) : VCAP_CH_SUBCH_OFFSET(ch, 0x04)));
            if(pcurr_param->comm.field_order)
                tmp |= BIT28;
            else
                tmp &= ~BIT28;
            if(tmp & BIT8)
                pcurr_param->comm.fcs_en = 1;
            if(tmp & BIT9)
                pcurr_param->comm.dn_en = 1;
            pcurr_param->comm.mask_en = (tmp & 0xff);
            VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x04) : VCAP_CH_SUBCH_OFFSET(ch, 0x04)), tmp);

            /* Reg 0x08 */
            tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
            tmp &= 0x3000ffff;
            pcurr_param->comm.osd_en    = (tmp & 0xff);
            pcurr_param->comm.mark_en   = ((tmp>>8)  & 0xf);
            pcurr_param->comm.border_en = ((tmp>>12) & 0xf);
            pcurr_param->comm.h_flip    = ((tmp>>28) & 0x1);
            pcurr_param->comm.v_flip    = ((tmp>>29) & 0x1);
            VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)), tmp);

            /* Reg 0x0c */
#ifdef PLAT_OSD_COLOR_SCHEME
            tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x0c) : VCAP_CH_SUBCH_OFFSET(ch, 0x0c)));
            tmp &= 0x000000ff;
            pcurr_param->comm.accs_win_en = tmp;
            VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x0c) : VCAP_CH_SUBCH_OFFSET(ch, 0x0c)), tmp);
#else
            VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x0c) : VCAP_CH_SUBCH_OFFSET(ch, 0x0c)), 0);
#endif

            /* SC */
            vcap_dev_sc_setup(pdev_info, ch);

            /* OSD Common */
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_OSD_OFFSET(0x00));

                /* keep user marquee_speed, img_border_color, font_edge_mode */
                pcurr_param->osd_comm.marquee_speed  = (tmp>>8)  & 0x3;
                pcurr_param->osd_comm.border_color   = (tmp>>12) & 0xf;
                pcurr_param->osd_comm.font_edge_mode = (tmp>>16) & 0x1;

                /* Mark Smoothing for 2x/4x enlarge */
                tmp &= ~(BIT3 | (0x3f<<18) | (0x3f<<26));
                tmp |= ((VCAP_MARK_ZOOM_ADJUST_ENB_DEFAULT<<3) | (VCAP_MARK_ZOOM_ADJUST_CR_DEFAULT<<18) | (VCAP_MARK_ZOOM_ADJUST_CB_DEFAULT<<26));
#ifdef PLAT_OSD_COLOR_SCHEME
                tmp &= ~(BIT17 | BIT11 | BIT10);
                tmp |= (VCAP_OSD_ACCS_HYSTERESIS_RANGE_DEFAULT<<10) | (VCAP_OSD_ACCS_ADJUST_ENB_DEFAULT<<17);   ///< Auto Color Scheme Adjust Parameter
#endif
                VCAP_REG_WR(pdev_info, VCAP_CAS_OSD_OFFSET(0x00), tmp);

                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_OSD_OFFSET(0x04));
                tmp &= ~(0x3f<<2);
                tmp |= (VCAP_MARK_ZOOM_ADJUST_Y_DEFAULT<<2);
#ifdef PLAT_OSD_COLOR_SCHEME
                tmp &= ~(0x3ffff<<8);
                tmp |= ((VCAP_OSD_ACCS_DATA_AVG_LEVEL_DEFAULT<<8)      |
                       (VCAP_OSD_ACCS_ACCUM_WHITE_THRES_DEFAULT<<16)   |
                       (VCAP_OSD_ACCS_ACCUM_BLACK_THRES_DEFAULT<<20)   |
                       (VCAP_OSD_ACCS_NO_HYSTERESIS_THRES_DEFAULT<<24) |
                       (VCAP_OSD_ACCS_UPDATE_FSM_ENB_DEFAULT<<25));         ///< Auto Color Scheme Adjust Parameter
#endif
                VCAP_REG_WR(pdev_info, VCAP_CAS_OSD_OFFSET(0x04), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_OSD_OFFSET(ch, 0x00));

                /* keep user marquee_speed, img_border_color, font_edge_mode */
                pcurr_param->osd_comm.marquee_speed  = (tmp>>8)  & 0x3;
                pcurr_param->osd_comm.border_color   = (tmp>>12) & 0xf;
                pcurr_param->osd_comm.font_edge_mode = (tmp>>16) & 0x1;

                /* Mark Smoothing for 2x/4x enlarge */
                tmp &= ~(BIT3 | (0x3f<<18) | (0x3f<<26));
                tmp |= ((VCAP_MARK_ZOOM_ADJUST_ENB_DEFAULT<<3) | (VCAP_MARK_ZOOM_ADJUST_CR_DEFAULT<<18) | (VCAP_MARK_ZOOM_ADJUST_CB_DEFAULT<<26));
#ifdef PLAT_OSD_COLOR_SCHEME
                tmp &= ~(BIT17 | BIT11 | BIT10);
                tmp |= (VCAP_OSD_ACCS_HYSTERESIS_RANGE_DEFAULT<<10) | (VCAP_OSD_ACCS_ADJUST_ENB_DEFAULT<<17);   ///< Auto Color Scheme Adjust Parameter
#endif
                VCAP_REG_WR(pdev_info, VCAP_CH_OSD_OFFSET(ch, 0x00), tmp);

                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_OSD_OFFSET(ch, 0x04));
                tmp &= ~(0x3f<<2);
                tmp |= (VCAP_MARK_ZOOM_ADJUST_Y_DEFAULT<<2);
#ifdef PLAT_OSD_COLOR_SCHEME
                tmp &= ~(0x3ffff<<8);
                tmp |= ((VCAP_OSD_ACCS_DATA_AVG_LEVEL_DEFAULT<<8)      |
                       (VCAP_OSD_ACCS_ACCUM_WHITE_THRES_DEFAULT<<16)   |
                       (VCAP_OSD_ACCS_ACCUM_BLACK_THRES_DEFAULT<<20)   |
                       (VCAP_OSD_ACCS_NO_HYSTERESIS_THRES_DEFAULT<<24) |
                       (VCAP_OSD_ACCS_UPDATE_FSM_ENB_DEFAULT<<25));         ///< Auto Color Scheme Adjust Parameter
#endif
                VCAP_REG_WR(pdev_info, VCAP_CH_OSD_OFFSET(ch, 0x04), tmp);
            }

            /* MD */
            if(ch == VCAP_CASCADE_CH_NUM) {
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x00), 0);
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x04), 0);
            }
            else {
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x00), 0);
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x04), 0);
            }

            for(j=0; j<VCAP_SCALER_MAX; j++) {
                if((pdev_info->split.ch == ch) && (j >= VCAP_SPLIT_CH_SCALER_MAX))
                    break;

                /* SD, Bypass */
                pcurr_param->sd[j].src_w        = pcurr_param->sc.width;
                pcurr_param->sd[j].src_h        = pcurr_param->sc.height;
                pcurr_param->sd[j].out_w        = pcurr_param->sd[j].src_w;
                pcurr_param->sd[j].out_h        = pcurr_param->sd[j].src_h;
                pcurr_param->sd[j].hstep        = 256;
                pcurr_param->sd[j].vstep        = 256;
                pcurr_param->sd[j].frm_ctrl     = 0x30000000;
                pcurr_param->sd[j].frm_wrap     = 2;
                pcurr_param->sd[j].hsize        = 0;

                tmp = pcurr_param->sd[j].src_w | (pcurr_param->sd[j].src_h<<16);
                if(ch == VCAP_CASCADE_CH_NUM)
                    VCAP_REG_WR(pdev_info, VCAP_CAS_SUBCH_OFFSET(0x10 + 0x4*j), tmp);
                else
                    VCAP_REG_WR(pdev_info, VCAP_CH_SUBCH_OFFSET(ch, (0x10 + 0x4*j)), tmp);

                tmp = pcurr_param->sd[j].hstep | (pcurr_param->sd[j].vstep<<16);
                if(ch == VCAP_CASCADE_CH_NUM)
                    VCAP_REG_WR(pdev_info, VCAP_CAS_DNSD_OFFSET(0x08 + 0x10*j), tmp);
                else
                    VCAP_REG_WR(pdev_info, VCAP_CH_DNSD_OFFSET(ch, (0x08 + 0x10*j)), tmp);

                /* Presmooth */
                pcurr_param->sd[j].none_auto_smo = ptemp_param->sd[j].none_auto_smo;    ///< keep user none_auto_smo setting

                if(ch == VCAP_CASCADE_CH_NUM)
                    tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_DNSD_OFFSET(0x0c + 0x10*j));
                else
                    tmp = VCAP_REG_RD(pdev_info, VCAP_CH_DNSD_OFFSET(ch, (0x0c + 0x10*j)));

                pcurr_param->sd[j].hsmo_str = (tmp>>26) & 0x7;      ///< keep user hsmo_str setting
                pcurr_param->sd[j].vsmo_str = (tmp>>29) & 0x7;      ///< keep user vsmo_str setting

                tmp = (pcurr_param->sd[j].topvoft)      |
                      (pcurr_param->sd[j].botvoft<<13)  |
                      (pcurr_param->sd[j].hsmo_str<<26) |
                      (pcurr_param->sd[j].vsmo_str<<29);
                if(ch == VCAP_CASCADE_CH_NUM)
                    VCAP_REG_WR(pdev_info, VCAP_CAS_DNSD_OFFSET(0x0c + 0x10*j), tmp);
                else
                    VCAP_REG_WR(pdev_info, VCAP_CH_DNSD_OFFSET(ch, (0x0c + 0x10*j)), tmp);

                tmp = pcurr_param->sd[j].frm_ctrl;
                if(ch == VCAP_CASCADE_CH_NUM)
                    VCAP_REG_WR(pdev_info, VCAP_CAS_DNSD_OFFSET(0x10 + 0x10*j), tmp);
                else
                    VCAP_REG_WR(pdev_info, VCAP_CH_DNSD_OFFSET(ch, (0x10 + 0x10*j)), tmp);

                /* Sharpness */
                tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*j) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*j)));
                pcurr_param->sd[j].sharp.adp    = (tmp>>5)  & 0x1;
                pcurr_param->sd[j].sharp.radius = (tmp>>6)  & 0x7;
                pcurr_param->sd[j].sharp.amount = (tmp>>9)  & 0x3f;
                pcurr_param->sd[j].sharp.thresh = (tmp>>15) & 0x3f;
                pcurr_param->sd[j].sharp.start  = (tmp>>21) & 0x3f;
                pcurr_param->sd[j].sharp.step   = (tmp>>27) & 0x1f;

                tmp &= ~0x1f;
                tmp |= pcurr_param->sd[j].frm_wrap;
                VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14 + 0x10*j) : VCAP_CH_DNSD_OFFSET(ch, 0x14 + 0x10*j)), tmp);

                /* TC, Bypass */
                pcurr_param->tc[j].x_start  = 0;
                pcurr_param->tc[j].y_start  = 0;
                pcurr_param->tc[j].width    = pcurr_param->sd[j].out_w;
                pcurr_param->tc[j].height   = pcurr_param->sd[j].out_h;

                /* Image Border */
                tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_TC_OFFSET(0x4 + (j*0x8)) : VCAP_CH_TC_OFFSET(ch, (0x4 + (j*0x8)))));
                pcurr_param->tc[j].border_w = (VCAP_OSD_BORDER_PIXEL == 2) ? (((tmp>>13) & 0x7) | (((tmp>>29)&0x1)<<3)) : ((tmp>>13) & 0x7);

                if(j%2) {
                    tmp = (pcurr_param->tc[j-1].width) | (pcurr_param->tc[j].width<<16);
                    if(ch == VCAP_CASCADE_CH_NUM)
                        VCAP_REG_WR(pdev_info, VCAP_CAS_SUBCH_OFFSET(0x20 + 0x4*(j/2)), tmp);
                    else
                        VCAP_REG_WR(pdev_info, VCAP_CH_SUBCH_OFFSET(ch, (0x20 + 0x4*(j/2))), tmp);
                }

                tmp = (pcurr_param->tc[j].x_start) |
                      ((pcurr_param->tc[j].x_start + pcurr_param->tc[j].width - 1)<<16);
                if(ch == VCAP_CASCADE_CH_NUM)
                    VCAP_REG_WR(pdev_info, VCAP_CAS_TC_OFFSET(0x08*j), tmp);
                else
                    VCAP_REG_WR(pdev_info, VCAP_CH_TC_OFFSET(ch, (0x08*j)), tmp);

                tmp = (pcurr_param->tc[j].y_start)            |
                      ((pcurr_param->tc[j].border_w&0x7)<<13) |
                      ((pcurr_param->tc[j].y_start + pcurr_param->tc[j].height - 1)<<16) |
                      (((pcurr_param->tc[j].border_w>>3)&0x1)<<29);
                if(ch == VCAP_CASCADE_CH_NUM)
                    VCAP_REG_WR(pdev_info, VCAP_CAS_TC_OFFSET(0x04 + 0x08*j), tmp);
                else
                    VCAP_REG_WR(pdev_info, VCAP_CH_TC_OFFSET(ch, (0x04 + 0x08*j)), tmp);
            }

            /* OSD */
            for(j=0; j<VCAP_OSD_WIN_MAX; j++) {
                memcpy(&pcurr_param->osd[j], &ptemp_param->osd[j], sizeof(struct vcap_osd_param_t));    ///< keep user osd config
            }

            /* Mark */
            for(j=0; j<VCAP_MARK_WIN_MAX; j++) {
                memcpy(&pcurr_param->mark[j], &ptemp_param->mark[j], sizeof(struct vcap_mark_param_t)); ///< keep user mark config
            }

            /* sync current param to temp */
            memcpy(ptemp_param, pcurr_param, sizeof(struct vcap_ch_param_t));
        }
    }

    return 0;
}

static inline int vcap_dev_md_enable_check(struct vcap_dev_info_t *pdev_info, int ch)
{
    int i, ret = 0;
    int split_md_en = 0;
    int ch_md_en    = 0;
    int cas_md_en   = 0;

    /* check and allocate md statistic buffer */
    if(!pdev_info->ch[ch].md_gau_buf.vaddr || !pdev_info->ch[ch].md_event_buf.vaddr)
        return 0;

    /* check current enable channel */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(pdev_info->ch[i].active) {
            if(!pdev_info->ch[i].md_active)
                continue;

            if(pdev_info->split.ch == i)
                split_md_en++;
            else {
                if(i == VCAP_CASCADE_CH_NUM)
                    cas_md_en++;
                else
                    ch_md_en++;
            }
        }
    }

    if(pdev_info->split.ch == ch) {
        if(!ch_md_en && !cas_md_en)
            return 1;
        else {
            vcap_err("[%d] ch#%d MD enable failed!(cann't both enable MD in split and %s channel)\n",
                     pdev_info->index, ch, ((ch_md_en > 0) ? "normal" : "cascade"));
        }
    }
    else {
        if(ch == VCAP_CASCADE_CH_NUM) {
            if((!ch_md_en && !split_md_en) || (ch_md_en && pdev_info->capability.cas_isp))
                return 1;
            else {
                vcap_err("[%d] ch#%d MD enable failed!(cann't both enable MD in cascade and %s channel)\n",
                         pdev_info->index, ch, ((ch_md_en > 0) ? "normal" : "split"));
            }
        }
        else {
            if((!cas_md_en && !split_md_en) || (cas_md_en && pdev_info->capability.cas_isp))
                return 1;
            else {
                vcap_err("[%d] ch#%d MD enable failed!(cann't both enable MD in normal and %s channel)\n",
                         pdev_info->index, ch, ((cas_md_en > 0) ? "cascade" : "split"));
            }
        }
    }

    return ret;
}

int vcap_dev_param_setup(struct vcap_dev_info_t *pdev_info, int ch, int path, void *param)
{
#ifndef PLAT_SCALER_FRAME_RATE_CTRL
    u32 frame_rate_thres;
#endif
    int i;
    int ret          = 0;
    int h_div        = 1;
    int tc_h_div     = 1;
    int osd_h_div    = 1;
    int osd_y_div    = 1;
    int osd_frm_div  = 1;
    int osd_h_mul    = 1;
    int p2i          = 0;
    u16 scaler_hsize = 0;
    u16 bg_x_offset  = 0, bg_y_offset = 0;
    u16 src_w, src_h;
    u16 src_bg_w, src_bg_h;
    u16 sc_x, sc_y, sc_w, sc_h;
    u16 tc_x, tc_y, tc_w, tc_h;
    u16 sd_w, sd_h, ar_sd_w, ar_sd_h;
    u16 osd_w, osd_h, align_x_offset, align_y_offset;
    u16 md_x, md_y, md_x_size, md_y_size, md_x_num, md_y_num, md_src_w, md_src_h;
    u32 src_frame_rate, dst_frame_rate;
    int img_border_enb;
    struct vcap_mark_info_t mark_info;
    int vi_idx = CH_TO_VI(ch);
    int vi_ch  = SUBCH_TO_VICH(ch);
    struct vcap_vg_job_param_t *job_param = (struct vcap_vg_job_param_t *)param;
    struct vcap_ch_param_t *pcurr_param = &pdev_info->ch[ch].param;
    struct vcap_ch_param_t *ptemp_param = &pdev_info->ch[ch].temp_param;

    /* Video Source Width and Height */
    src_w = src_bg_w = pdev_info->vi[vi_idx].ch_param[vi_ch].src_w;
    src_h = src_bg_h = pdev_info->vi[vi_idx].ch_param[vi_ch].src_h;

    /* SC */
    if(pdev_info->split.ch == ch) {
        sc_x = sc_y = 0;
        sc_w = pcurr_param->sc.width;
        sc_h = pcurr_param->sc.height;
    }
    else {
        if(pdev_info->vi[vi_idx].format == VCAP_VI_FMT_RGB888) {
            src_w = src_bg_w = ALIGN_4(src_w/pdev_info->vi[vi_idx].sd_hratio);
        }

#ifdef VCAP_VG_SC_PROPERTY_SUPPORT
        sc_x = ALIGN_4(EM_PARAM_X(job_param->src_xy)) & 0x1fff;
        sc_y = EM_PARAM_Y(job_param->src_xy) & 0x1fff;
        sc_w = ALIGN_4(EM_PARAM_WIDTH(job_param->src_dim)) & 0x1fff;
        sc_h = EM_PARAM_HEIGHT(job_param->src_dim) & 0x1fff;

        if(!sc_w)
            sc_w = src_w;
        if(!sc_h)
            sc_h = src_h;

        /* check source crop window width and height range */
        if(((sc_x+sc_w) > src_w) || ((sc_y+sc_h) > src_h)) {
            vcap_err_limit("[%d] ch#%d-p%d source crop window is bigger than source.(SC_Crop=(%d,%d),(%d,%d), Source=(%d,%d))",
                           pdev_info->index, ch, path, sc_x, sc_y, sc_w, sc_h, src_w, src_h);
            ret = -1;
            goto exit;
        }

        if(sc_x != pcurr_param->sc.x_start ||
           sc_y != pcurr_param->sc.y_start ||
           sc_w != pcurr_param->sc.width   ||
           sc_h != pcurr_param->sc.height) {

            for(i=0; i<VCAP_SCALER_MAX; i++) {
                if(i == path)
                    continue;

                if(IS_DEV_CH_PATH_BUSY(pdev_info, ch, i)) {
                    vcap_err_limit("[%d] ch#%d-p%d switch source crop failed!(other path still busy)\n",
                                   pdev_info->index, ch, i);
                    ret = -1;
                    goto exit;
                }
            }
        }

        ptemp_param->sc.x_start = sc_x;
        ptemp_param->sc.y_start = sc_y;
        ptemp_param->sc.width   = sc_w;
        ptemp_param->sc.height  = sc_h;
#else
        sc_x = pcurr_param->sc.x_start;
        sc_y = pcurr_param->sc.y_start;
        sc_w = src_bg_w = pcurr_param->sc.width;
        sc_h = src_bg_h = pcurr_param->sc.height;
#endif
    }

    /* SD */
    sd_w = ALIGN_4(EM_PARAM_WIDTH(job_param->dst_dim)) & 0x1fff;
    sd_h = EM_PARAM_HEIGHT(job_param->dst_dim) & 0x1fff;

    if(pdev_info->vi[vi_idx].ch_param[vi_ch].prog == VCAP_VI_PROG_INTERLACE) {
        if(job_param->dst_fmt == TYPE_YUV422_FRAME) {
            sd_h >>= 1;
            h_div = 2;
        }
        else if(job_param->dst_fmt == TYPE_YUV422_2FRAMES) {
            if(!job_param->prog_2frm) {
                sd_h >>= 1;
                h_div = 2;
            }
            else {
                osd_y_div = 2;

                if(ptemp_param->comm.osd_frm_mode)
                    osd_frm_div = 2;
            }
        }
        else if(job_param->dst_fmt == TYPE_YUV422_FIELDS) {
            osd_h_div = 2;          ///< osd and mark position must base on frame size

            if(ptemp_param->comm.osd_frm_mode)
                osd_frm_div = 2;    ///< osd force frame mode, only for field mode
        }
    }
    else if(pdev_info->vi[vi_idx].prog == VCAP_VI_PROG_INTERLACE) {
        /* VI in interlace mode but channel in progressive mode, the osd y and height must multiply 2 */
        osd_h_mul = 2;
    }

    /* P2I */
    if((pdev_info->vi[vi_idx].ch_param[vi_ch].prog == VCAP_VI_PROG_PROGRESSIVE) && (pdev_info->split.ch != ch) && job_param->p2i) { ///< split channel not support P2I
        p2i = 1;
#ifndef PLAT_OSD_TC_SWAP
        tc_h_div = 2;   ///< P2I, the height and y_start of TC must divide 2
#endif
        if((job_param->dst_fmt == TYPE_YUV422_FIELDS) && ptemp_param->comm.osd_frm_mode)
            osd_frm_div = 2;    ///< osd force frame mode, only for field mode
    }

    /* check vertical minimal scaling up line number */
    if((sd_h > sc_h) && ((sd_h - sc_h) <= (pdev_info->m_param)->vup_min))
        sd_h = sc_h;    ///< bypass vertical scaling

    /* check scaler output window width and height */
    if(!sd_w || !sd_h) {
        vcap_err_limit("[%d] ch#%d-p%d scaler output size is not correct.(SD_Out=(%d,%d))\n",
                       pdev_info->index, ch, path, sd_w, sd_h);
        ret = -1;
        goto exit;
    }

    /* Auto Aspect Ratio */
    if(job_param->auto_aspect_ratio) {
        if((sc_w*sd_h) >= (sc_h*sd_w)) {
            ar_sd_h     = (sd_w*sc_h)/sc_w;
            bg_y_offset = ((((sd_h >= ar_sd_h) ? (sd_h - ar_sd_h) : (ar_sd_h - sd_h))>>1)*h_div); ///< must center output image when enable auto aspect ratio
            sd_h        = ar_sd_h;
        }
        else {
            ar_sd_w     =  ALIGN_4((sc_w*sd_h)/sc_h);
            bg_x_offset = (((sd_w >= ar_sd_w) ? (sd_w - ar_sd_w) : (ar_sd_w - sd_w))>>1);   ///< must center output image when enable auto aspect ratio
            sd_w        =  ar_sd_w;
        }
    }

    /* Scaling capability check */
    if(!(pdev_info->capability.scaling_cap & (0x1<<path)) && (sd_w != sc_w || sd_h != sc_h)) {
        vcap_err_limit("[%d] ch#%d-p%d not support scalling\n", pdev_info->index, ch, path);
        ret = -1;
        goto exit;
    }

    if((pdev_info->ch_mode == VCAP_CH_MODE_32CH) && ((sd_w > sc_w) || (sd_h > sc_h))) {
        vcap_err_limit("[%d] ch#%d-p%d not support scaling up(In CH32_MODE) (%d %d) (%d %d)\n", pdev_info->index, ch, path, sd_w, sd_h, sc_w, sc_h);
        ret = -1;
        goto exit;
    }

    /* check hsize of all enable path */
    for(i=0; i<VCAP_SCALER_MAX; i++) {
        if(i == path)
            scaler_hsize += sd_w;
        else if(IS_DEV_CH_PATH_BUSY(pdev_info, ch, i))
            scaler_hsize += pcurr_param->sd[i].hsize;
    }
    if(scaler_hsize > VCAP_SCALER_HORIZ_SIZE) {
        vcap_err_limit("[%d] ch#%d scaling capability is over spec.(hszie:%d, MAX=>%d)\n", pdev_info->index, ch, scaler_hsize, VCAP_SCALER_HORIZ_SIZE);
        ret = -1;
        goto exit;
    }

    ptemp_param->sd[path].src_w   = sc_w;
    ptemp_param->sd[path].src_h   = sc_h;
    ptemp_param->sd[path].out_w   = sd_w;
    ptemp_param->sd[path].out_h   = sd_h;
    ptemp_param->sd[path].hstep   = (sc_w*256)/sd_w;
    ptemp_param->sd[path].vstep   = (sc_h*256)/sd_h;
    ptemp_param->sd[path].topvoft = (sc_h > sd_h) ? 0 : ((256-ptemp_param->sd[path].vstep)>>1);
    ptemp_param->sd[path].botvoft = (sc_h > sd_h) ? ((ptemp_param->sd[path].vstep-256)>>1) : 0;
    ptemp_param->sd[path].hsize   = sd_w;

    /* SD presmooth adjustment, for improve scaling down image quality */
    if(ptemp_param->sd[path].none_auto_smo == 0) {
        if((ptemp_param->sd[path].hstep >= 256) && (ptemp_param->sd[path].hstep < 512))
            ptemp_param->sd[path].hsmo_str = 1;
        else if((ptemp_param->sd[path].hstep >= 512) && (ptemp_param->sd[path].hstep < 768))
            ptemp_param->sd[path].hsmo_str = 3;
        else if (ptemp_param->sd[path].hstep >= 768)
            ptemp_param->sd[path].hsmo_str = 5;
        else
            ptemp_param->sd[path].hsmo_str = 0;

        if((ptemp_param->sd[path].vstep >= 256) && (ptemp_param->sd[path].vstep < 512))
            ptemp_param->sd[path].vsmo_str = 3;
        else if((ptemp_param->sd[path].vstep >= 512) && (ptemp_param->sd[path].vstep < 768))
            ptemp_param->sd[path].vsmo_str = 5;
        else if (ptemp_param->sd[path].vstep >= 768)
            ptemp_param->sd[path].vsmo_str = 6;
        else
            ptemp_param->sd[path].vsmo_str = 0;     ///< in 32CH mode the hardware will force vsmo_str to 0
    }

    /* TC */
    tc_x = (PLAT_TC_X_ALIGN == 2) ? (ALIGN_2(EM_PARAM_X(job_param->dst_crop_xy)) & 0x1fff) : (ALIGN_4(EM_PARAM_X(job_param->dst_crop_xy)) & 0x1fff);
    tc_y = (EM_PARAM_Y(job_param->dst_crop_xy) & 0x1fff)/h_div;
    tc_w = ALIGN_4(EM_PARAM_WIDTH(job_param->dst_crop_dim)) & 0x1fff;
    tc_h = EM_PARAM_HEIGHT(job_param->dst_crop_dim) & 0x1fff;

    if((tc_h%h_div) != 0) {
        vcap_err_limit("[%d] ch#%d-p%d target crop height=%d must multiples of %d for interlace signal\n", pdev_info->index, ch, path, tc_h, h_div);
        ret = -1;
        goto exit;
    }
    else
        tc_h /= h_div;

    if(!tc_w)
        tc_w = sd_w;

    if(!tc_h)
        tc_h = sd_h;

    /*
     * when enable v_flip and out frame image on 2frames_2frames mode,
     * the target window height must minus one and offset one line to
     * paste image. Using to fix LCD display top/bottom field sequence
     * incorrect problem.
     */
    if((job_param->job_fmt == TYPE_YUV422_2FRAMES_2FRAMES) && (job_param->dst_fmt == TYPE_YUV422_FRAME) && ptemp_param->comm.v_flip) {
        if((tc_h - 1) > 0)
            tc_h--;
        else {
            vcap_err_limit("[%d] ch#%d-p%d target crop height=%d must >= 2\n", pdev_info->index, ch, path, tc_h);
            ret = -1;
            goto exit;
        }
    }

    if(job_param->auto_aspect_ratio) {
        /*
         * because auto aspect ratio will adjust sd output size,
         * must to check and adjust tc output size to prevent tc
         * region over sd region
         */
        if(tc_x >= sd_w)
            tc_x = (PLAT_TC_X_ALIGN == 2) ? ALIGN_2(sd_w-1) : ALIGN_4(sd_w-1);

        if(tc_y >= sd_h)
            tc_y = sd_h - 1;

        if((tc_x+tc_w) > sd_w) {
            bg_x_offset = ((tc_w - (sd_w - tc_x))>>1);
            tc_w        = sd_w - tc_x;
        }

        if((tc_y+tc_h) > sd_h) {
            bg_y_offset = (((tc_h - (sd_h - tc_y))>>1)*h_div*tc_h_div);
            tc_h        = sd_h - tc_y;
        }

        /* don't apply background position center offset when enable target croping */
        if((tc_w != sd_w) || (tc_h != sd_h))
            bg_x_offset = bg_y_offset = 0;
    }

    /* check target crop window width and height range */
    if(((tc_x+tc_w) > sd_w) || ((tc_y+tc_h) > sd_h)) {
        vcap_err_limit("[%d] ch#%d-p%d target crop window is bigger than scaler output.(TC_Crop=(%d,%d),(%d,%d), SD_Out=(%d,%d))",
                 pdev_info->index, ch, path, tc_x, tc_y, tc_w, tc_h, sd_w, sd_h);
        ret = -1;
        goto exit;
    }

    ptemp_param->tc[path].x_start = tc_x;
    ptemp_param->tc[path].y_start = tc_y/tc_h_div;
    ptemp_param->tc[path].width   = tc_w;
    ptemp_param->tc[path].height  = tc_h/tc_h_div;

    if((pdev_info->split.ch == ch) &&
       ((ptemp_param->tc[path].width != ptemp_param->sd[path].out_w) ||
       (ptemp_param->tc[path].height != ptemp_param->sd[path].out_h))) {
        vcap_err_limit("[%d] ch#%d-p%d split channel not support target crop\n", pdev_info->index, ch, path);
        ret = -1;
        goto exit;
    }

    /* Auto Border(OSD Image Border) */
    img_border_enb = job_param->auto_border & 0xff;
    if(img_border_enb && job_param->auto_aspect_ratio) {
        vcap_err_limit("[%d] ch#%d-p%d not support enable auto aspect ratio and auto border at the same time!\n", pdev_info->index, ch, path);
        ret = -1;
        goto exit;
    }
    else {
        if(img_border_enb) {
            ptemp_param->comm.border_en       |= (0x1<<path);
            ptemp_param->tc[path].border_w     = (VCAP_OSD_BORDER_PIXEL == 2) ? ((job_param->auto_border>>8) & 0xf) : ((job_param->auto_border>>8) & 0x7);
            ptemp_param->osd_comm.border_color = (job_param->auto_border>>16) & 0xf;
        }
        else
            ptemp_param->comm.border_en &= ~(0x1<<path);
    }

    /* MD */
    if((pdev_info->m_param)->cap_md) {
        if(job_param->md_enb) {
            md_x      = ALIGN_4((job_param->md_xy_start>>16) & 0xffff);
            md_y      = (job_param->md_xy_start & 0xffff)/h_div;
            md_x_size = ALIGN_4((job_param->md_xy_size>>16) & 0xffff);
            md_y_size = (job_param->md_xy_size & 0xffff)/h_div;
            md_x_num  = (job_param->md_xy_num>>16) & 0xffff;
            md_y_num  = job_param->md_xy_num & 0xffff;
#ifdef PLAT_OSD_TC_SWAP
            md_src_w  = tc_w;   ///< MD image from TC output
            md_src_h  = tc_h;
#else
            md_src_w  = sd_w;   ///< MD image from SD output
            md_src_h  = sd_h;
#endif

            /* check md x,y size */
            if((md_x_size < 16)     ||
               (md_x_size > 32)     ||
               ((md_x_size%4) != 0) ||
               (md_y_size < 2)      ||
               (md_y_size > 32)) {
                vcap_err_limit("[%d] ch#%d-p%d md_x_size=%d (16, 20, 24, 28, 32) md_y_size=%d (2~32)\n",
                               pdev_info->index, ch, path, md_x_size, md_y_size);
                job_param->md_enb = 0;
                goto md_disable;
            }

            /* check md x,y start position */
            if((md_x >= md_src_w) ||
               (md_y >= md_src_h)) {
                vcap_err_limit("[%d] ch#%d-p%d md start position invalid. md_src(%d, %d), md_xy(%d, %d)\n",
                               pdev_info->index, ch, path, md_src_w, md_src_h, md_x, md_y);
                job_param->md_enb = 0;
                goto md_disable;
            }

            /* check md y number */
            if(md_y_num > VCAP_MD_Y_NUM_MAX) {
                vcap_err_limit("[%d] ch#%d-p%d md_y_num=%d over spec.(MAX => %d)\n", pdev_info->index, ch, path, md_y_num, VCAP_MD_Y_NUM_MAX);
                job_param->md_enb = 0;
                goto md_disable;
            }

            /* check md x number */
            switch(pdev_info->vi[CH_TO_VI(ch)].tdm_mode) {
                case VCAP_VI_TDM_MODE_FRAME_INTERLEAVE:
                    if(md_x_num > VCAP_MD_VI_4CH_FRAME_X_MAX) {
                        vcap_err_limit("[%d] ch#%d-p%d md_x_num=%d over spec.(MAX => %d)\n", pdev_info->index, ch, path, md_x_num, VCAP_MD_VI_4CH_FRAME_X_MAX);
                        job_param->md_enb = 0;
                        goto md_disable;
                    }
                    break;
                case VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE:
                case VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID:
                    if(pdev_info->ch_mode == VCAP_CH_MODE_16CH) {
                        if(md_x_num > VCAP_MD_VI_1CH_X_MAX) {
                            vcap_err_limit("[%d] ch#%d-p%d md_x_num=%d over spec.(MAX => %d)\n", pdev_info->index, ch, path, md_x_num, VCAP_MD_VI_1CH_X_MAX);
                            job_param->md_enb = 0;
                            goto md_disable;
                        }
                    }
                    else {
                        if(md_x_num > VCAP_MD_VI_2CH_X_MAX) {
                            vcap_err_limit("[%d] ch#%d-p%d md_x_num=%d over spec.(MAX => %d)\n", pdev_info->index, ch, path, md_x_num, VCAP_MD_VI_2CH_X_MAX);
                            job_param->md_enb = 0;
                            goto md_disable;
                        }
                    }
                    break;
                case VCAP_VI_TDM_MODE_4CH_BYTE_INTERLEAVE:
                    if(pdev_info->ch_mode == VCAP_CH_MODE_16CH) {
                        if(md_x_num > VCAP_MD_VI_2CH_X_MAX) {
                            vcap_err_limit("[%d] ch#%d-p%d md_x_num=%d over spec.(MAX => %d)\n", pdev_info->index, ch, path, md_x_num, VCAP_MD_VI_2CH_X_MAX);
                            job_param->md_enb = 0;
                            goto md_disable;
                        }
                    }
                    else {
                        if(md_x_num > VCAP_MD_VI_4CH_BYTE_X_MAX) {
                            vcap_err_limit("[%d] ch#%d-p%d md_x_num=%d over spec.(MAX => %d)\n", pdev_info->index, ch, path, md_x_num, VCAP_MD_VI_4CH_BYTE_X_MAX);
                            job_param->md_enb = 0;
                            goto md_disable;
                        }
                    }
                    break;
                default:    /* Bypass */
                    if(ch == pdev_info->split.ch) {
                        if(md_x_num > VCAP_MD_SPLIT_CH_X_MAX) {
                            vcap_err_limit("[%d] ch#%d-p%d(split) md_x_num=%d over spec.(MAX => %d)\n", pdev_info->index, ch, path, md_x_num, VCAP_MD_SPLIT_CH_X_MAX);
                            job_param->md_enb = 0;
                            goto md_disable;
                        }
                    }
                    else {
                        if(md_x_num > VCAP_MD_VI_1CH_X_MAX) {
                            vcap_err_limit("[%d] ch#%d-p%d md_x_num=%d over spec.(MAX => %d)\n", pdev_info->index, ch, path, md_x_num, VCAP_MD_VI_1CH_X_MAX);
                            job_param->md_enb = 0;
                            goto md_disable;
                        }
                    }
                    break;
            }

            /*
             * check and adjust md block number to prevent md region over sd output size
             */
            while(md_x_num) {
                if((md_x + (md_x_size*md_x_num)) > md_src_w)
                    md_x_num--;
                else
                    break;
            }

            while(md_y_num) {
                if((md_y + (md_y_size*md_y_num)) > md_src_h)
                    md_y_num--;
                else
                    break;
            }

            if(!md_x_num || !md_y_num) {
                job_param->md_enb = 0;
                goto md_disable;
            }

            /* check md region */
            if(((md_x + (md_x_size*md_x_num)) > md_src_w) ||
               ((md_y + (md_y_size*md_y_num)) > md_src_h)) {
                vcap_err_limit("[%d] ch#%d-p%d md region invalid. md_src(%d, %d), md_xy(%d, %d), md_size(%d, %d), md_num(%d, %d)\n",
                               pdev_info->index, ch, path, md_src_w, md_src_h, md_x, md_y, md_x_size, md_y_size, md_x_num, md_y_num);
                job_param->md_enb = 0;
                goto md_disable;
            }

            /* check md can enable or not */
            if(!pcurr_param->comm.md_en) {
                if(!vcap_dev_md_enable_check(pdev_info, ch)) {
                    job_param->md_enb = 0;
                    goto md_disable;
                }
            }

            /* mark channel md as active */
            pdev_info->ch[ch].md_active = 1;

            if(pdev_info->dev_md_fatal) {   ///< force disable md if md meet fatal error
                ptemp_param->comm.md_en = 0;
            }
            else {
#ifdef PLAT_MD_GROUPING
                if((pdev_info->ch[ch].md_grp_id == pdev_info->md_enb_grp) || (pdev_info->ch[ch].md_grp_id < 0))
                    ptemp_param->comm.md_en = 1;
                else
                    ptemp_param->comm.md_en = 0;
#else
                ptemp_param->comm.md_en = 1;
#endif
            }

            ptemp_param->comm.md_src   = path;
            ptemp_param->md.x_start    = md_x;
            ptemp_param->md.y_start    = md_y;
            ptemp_param->md.x_size     = md_x_size;
            ptemp_param->md.y_size     = md_y_size;
            ptemp_param->md.x_num      = md_x_num;
            ptemp_param->md.y_num      = md_y_num;
            ptemp_param->md.gau_addr   = pdev_info->ch[ch].md_gau_buf.paddr;
            ptemp_param->md.event_addr = pdev_info->ch[ch].md_event_buf.paddr;
        }
        else {
            if(pdev_info->ch[ch].md_active && pcurr_param->comm.md_src == path)
                pdev_info->ch[ch].md_active = 0;
        }

md_disable:
        if(!job_param->md_enb && pcurr_param->comm.md_en && pcurr_param->comm.md_src == path) {
            ptemp_param->comm.md_en = 0;
        }
    }

    /* Frame Rate Control */
    if(job_param->fps_ratio) {
        src_frame_rate = job_param->fps_ratio & 0xffff;
        dst_frame_rate= (job_param->fps_ratio>>16) & 0xffff;

        if(!src_frame_rate)
            src_frame_rate = pdev_info->vi[vi_idx].ch_param[vi_ch].frame_rate;

        if(!dst_frame_rate || dst_frame_rate > src_frame_rate)
            dst_frame_rate = src_frame_rate;
    }
    else if(job_param->target_frame_rate) {
        src_frame_rate = (p2i == 0) ? pdev_info->vi[vi_idx].ch_param[vi_ch].frame_rate : (pdev_info->vi[vi_idx].ch_param[vi_ch].frame_rate>>1);
        dst_frame_rate = job_param->target_frame_rate;

        if(!src_frame_rate)
            src_frame_rate++;

        if(!dst_frame_rate || dst_frame_rate > src_frame_rate)
            dst_frame_rate = src_frame_rate;
    }
    else {
        src_frame_rate = dst_frame_rate = pdev_info->vi[vi_idx].ch_param[vi_ch].frame_rate;
    }

#ifdef PLAT_SCALER_FRAME_RATE_CTRL
    /* Use hardware scaler frame rate control */
    if((src_frame_rate == dst_frame_rate) || (pdev_info->split.ch == ch)) {   ///< disable frame rate control, split channel not support frame rate control
        ptemp_param->sd[path].frm_wrap = 0x2;
        ptemp_param->sd[path].frm_ctrl = 0x30000000;
    }
    else {
        if(PLAT_SCALER_FRAME_RATE_CTRL_VER <= 1) {
            int grab_cnt;
            u32 src_rate, dst_rate;

            if(job_param->fps_ratio) {
                src_rate = (p2i == 0) ? pdev_info->vi[vi_idx].ch_param[vi_ch].frame_rate : (pdev_info->vi[vi_idx].ch_param[vi_ch].frame_rate>>1);
                if(!src_rate)
                    src_rate++;
                dst_rate = (dst_frame_rate*src_rate)/src_frame_rate;
                if(!dst_rate)
                    dst_rate++;
            }
            else {
                src_rate = src_frame_rate;
                dst_rate = dst_frame_rate;
            }

            if((pdev_info->vi[vi_idx].ch_param[vi_ch].prog == VCAP_VI_PROG_INTERLACE || p2i) &&
               ((pdev_info->vi[vi_idx].cap_style == VCAP_VI_CAP_STYLE_ANYONE)    ||
                (pdev_info->vi[vi_idx].cap_style == VCAP_VI_CAP_STYLE_ODD_FIRST) ||
                (pdev_info->vi[vi_idx].cap_style == VCAP_VI_CAP_STYLE_EVEN_FIRST))) {
                ptemp_param->sd[path].frm_wrap = (src_rate > 15) ? 30 : (src_rate*2);
                ptemp_param->sd[path].frm_ctrl = 0;
                if(dst_rate%(src_rate/(ptemp_param->sd[path].frm_wrap/2)) != 0)
                    grab_cnt = (dst_rate/(src_rate/(ptemp_param->sd[path].frm_wrap/2))) + 1;
                else
                    grab_cnt = (dst_rate/(src_rate/(ptemp_param->sd[path].frm_wrap/2)));
                for(i=0; i<grab_cnt; i++)
                    ptemp_param->sd[path].frm_ctrl |= (0x3<<(28 - (i*2)));
            }
            else {
                ptemp_param->sd[path].frm_wrap = (src_rate > 30) ? 30 : src_rate;
                ptemp_param->sd[path].frm_ctrl = 0;
                if(dst_rate%(src_rate/ptemp_param->sd[path].frm_wrap) != 0)
                    grab_cnt = (dst_rate/(src_rate/ptemp_param->sd[path].frm_wrap)) + 1;
                else
                    grab_cnt = (dst_rate/(src_rate/ptemp_param->sd[path].frm_wrap));
                for(i=0; i<grab_cnt; i++)
                    ptemp_param->sd[path].frm_ctrl |= (0x1<<(29 - i));
            }
        }
        else {
            /* ToDo */
        }
    }
#else
    /* use software frame rate control */
    if(src_frame_rate == dst_frame_rate)    ///< no frame rate control
        frame_rate_thres = 0;
    else {
        frame_rate_thres = (1000000*src_frame_rate)/(VCAP_FRAME_RATE_BASE*dst_frame_rate);
        if(p2i)
            frame_rate_thres <<= 1;         ///< because grab field pair, two field one frame done
    }

    if(pdev_info->ch[ch].path[path].frame_rate_thres != frame_rate_thres) {
        pdev_info->ch[ch].path[path].frame_rate_cnt   = 0;
        pdev_info->ch[ch].path[path].frame_rate_thres = frame_rate_thres;
    }
#endif

    /* OSD */
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        if((ptemp_param->comm.osd_en & (0x1<<i)) && (ptemp_param->osd[i].path == path)) {
            /* Window size auto ajustment if window type is osd_mask */
            if(ptemp_param->osd[i].win_type == VCAP_OSD_WIN_TYPE_MASK) {
                align_x_offset = ALIGN_4((sd_w*ptemp_param->osd[i].align_x_offset)/sc_w);
                if(job_param->dst_fmt == TYPE_YUV422_FIELDS && p2i)
                    align_y_offset = ((sd_h*ptemp_param->osd[i].align_y_offset*tc_h_div)/sc_h)/osd_h_div/osd_frm_div;
                else
                    align_y_offset = ((sd_h*ptemp_param->osd[i].align_y_offset)/sc_h)/osd_h_div/osd_frm_div;

                osd_w = (sd_w*ptemp_param->osd[i].width)/sc_w;
                if(((sd_w*ptemp_param->osd[i].width)%sc_w) || (osd_w%4))
                    osd_w = ALIGN_4(osd_w) + 4; ///< round up to multiple of 4

                osd_h = (sd_h*ptemp_param->osd[i].height)/sc_h/osd_frm_div;
                if((sd_h*ptemp_param->osd[i].height)%(sc_h*osd_frm_div))
                    osd_h++;    ///< round up

                if(!osd_w)
                    osd_w = 4;  ///< min width
                if(!osd_h)
                    osd_h = 1;  ///< min height
            }
            else {
                align_x_offset = ptemp_param->osd[i].align_x_offset;
                if(job_param->dst_fmt == TYPE_YUV422_FIELDS && p2i) {
#ifdef PLAT_OSD_TC_SWAP
                    align_y_offset = (ptemp_param->osd[i].align_y_offset*2)*osd_y_div/osd_frm_div;
#else
                    align_y_offset = (ptemp_param->osd[i].align_y_offset*tc_h_div)*osd_y_div/osd_frm_div;
#endif
                }
                else
                    align_y_offset = ptemp_param->osd[i].align_y_offset*osd_y_div/osd_frm_div;

                osd_w = ptemp_param->osd[i].width;
                osd_h = ptemp_param->osd[i].height;
            }

            /* Window Position Auto Align Ajustment */
            switch(ptemp_param->osd[i].align_type) {
                case VCAP_ALIGN_TOP_CENTER:
                    if(((ptemp_param->tc[path].width/2) - (osd_w/2)) > 0)
                        ptemp_param->osd[i].x_start = ALIGN_4((ptemp_param->tc[path].width/2) - (osd_w/2));
                    else
                        ptemp_param->osd[i].x_start = 0;

                    ptemp_param->osd[i].y_start = align_y_offset;
                    break;
                case VCAP_ALIGN_TOP_RIGHT:
                    if((ptemp_param->tc[path].width - osd_w - align_x_offset) > 0)
                        ptemp_param->osd[i].x_start = ALIGN_4(ptemp_param->tc[path].width - osd_w - align_x_offset);
                    else
                        ptemp_param->osd[i].x_start = 0;

                    ptemp_param->osd[i].y_start = align_y_offset;
                    break;
                case VCAP_ALIGN_BOTTOM_LEFT:
                    ptemp_param->osd[i].x_start = align_x_offset;

                    if((ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div - (osd_h/osd_h_div) - align_y_offset) > 0)
                        ptemp_param->osd[i].y_start = ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div - (osd_h/osd_h_div) - align_y_offset;
                    else
                        ptemp_param->osd[i].y_start = 0;
                    break;
                case VCAP_ALIGN_BOTTOM_CENTER:
                    if(((ptemp_param->tc[path].width/2) - (osd_w/2)) > 0)
                        ptemp_param->osd[i].x_start = ALIGN_4((ptemp_param->tc[path].width/2) - (osd_w/2));
                    else
                        ptemp_param->osd[i].x_start = 0;

                    if((ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div - (osd_h/osd_h_div) - align_y_offset) > 0)
                        ptemp_param->osd[i].y_start = ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div - (osd_h/osd_h_div) - align_y_offset;
                    else
                        ptemp_param->osd[i].y_start = 0;
                    break;
                case VCAP_ALIGN_BOTTOM_RIGHT:
                    if((ptemp_param->tc[path].width - osd_w - align_x_offset) > 0)
                        ptemp_param->osd[i].x_start = ALIGN_4(ptemp_param->tc[path].width - osd_w - align_x_offset);
                    else
                        ptemp_param->osd[i].x_start = 0;

                    if((ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div - (osd_h/osd_h_div) - align_y_offset) > 0)
                        ptemp_param->osd[i].y_start = ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div - (osd_h/osd_h_div) - align_y_offset;
                    else
                        ptemp_param->osd[i].y_start = 0;
                    break;
                case VCAP_ALIGN_CENTER:
                    if(((ptemp_param->tc[path].width/2) - (osd_w/2)) > 0)
                        ptemp_param->osd[i].x_start = ALIGN_4((ptemp_param->tc[path].width/2) - (osd_w/2));
                    else
                        ptemp_param->osd[i].x_start = 0;

                    if((((ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div)/2) - ((osd_h/osd_h_div)/2)) > 0)
                        ptemp_param->osd[i].y_start = ((ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div)/2) - ((osd_h/osd_h_div)/2);
                    else
                        ptemp_param->osd[i].y_start = 0;
                    break;
                default:    /* VCAP_ALIGN_NONE or VCAP_ALIGN_TOP_LEFT*/
                    ptemp_param->osd[i].x_start = align_x_offset;
                    ptemp_param->osd[i].y_start = align_y_offset;
                    break;
            }

            /* offset to target output window */
#ifdef PLAT_OSD_TC_SWAP
            ptemp_param->osd[i].y_start *= osd_h_div;
#else
            if((ptemp_param->osd[i].win_type == VCAP_OSD_WIN_TYPE_FONT) ||
               ((ptemp_param->osd[i].win_type == VCAP_OSD_WIN_TYPE_MASK) && (ptemp_param->osd[i].align_type != VCAP_ALIGN_NONE))) {
                ptemp_param->osd[i].x_start += ptemp_param->tc[path].x_start;
                ptemp_param->osd[i].y_start = (ptemp_param->osd[i].y_start + ptemp_param->tc[path].y_start*h_div*tc_h_div*osd_y_div/osd_frm_div)*osd_h_div;
            }
#endif
            ptemp_param->osd[i].y_start *= osd_h_mul;
            osd_h *= osd_h_mul;

            if((ptemp_param->osd[i].x_start + osd_w - 1) > 0)
                ptemp_param->osd[i].x_end = ptemp_param->osd[i].x_start + osd_w - 1;
            else
                ptemp_param->osd[i].x_end = 0;

            if((ptemp_param->osd[i].y_start + osd_h - 1) > 0)
                ptemp_param->osd[i].y_end = ptemp_param->osd[i].y_start + osd_h - 1;
            else
                ptemp_param->osd[i].y_end = 0;

            ptemp_param->osd[i].x_start &= 0xfff;
            ptemp_param->osd[i].y_start &= 0xfff;
            ptemp_param->osd[i].x_end   &= 0xfff;
            ptemp_param->osd[i].y_end   &= 0xfff;
        }
    }

    /* Mark */
    for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
        if((ptemp_param->comm.mark_en & (0x1<<i)) && (ptemp_param->mark[i].path == path)) {
            /* get mark info and check dimension */
            vcap_mark_get_info(pdev_info, ptemp_param->mark[i].id, &mark_info);

            if(ptemp_param->mark[i].addr != (mark_info.addr>>3))
                ptemp_param->mark[i].addr = (mark_info.addr>>3);

            if(ptemp_param->mark[i].x_dim != mark_info.x_dim) {
                ptemp_param->mark[i].x_dim = mark_info.x_dim;
                ptemp_param->mark[i].width = ((mark_info.x_dim >= 2) ? (0x1<<(mark_info.x_dim+2)) : 64)*(0x1<<ptemp_param->mark[i].zoom);
            }

            if(ptemp_param->mark[i].y_dim != mark_info.y_dim) {
                ptemp_param->mark[i].y_dim  = mark_info.y_dim;
                ptemp_param->mark[i].height = ((mark_info.y_dim >= 2) ? (0x1<<(mark_info.y_dim+2)) : 64)*(0x1<<ptemp_param->mark[i].zoom);
            }

            align_x_offset = ptemp_param->mark[i].align_x_offset;
            if(job_param->dst_fmt == TYPE_YUV422_FIELDS && p2i) {
#ifdef PLAT_OSD_TC_SWAP
                align_y_offset = (ptemp_param->mark[i].align_y_offset*2)*osd_y_div/osd_frm_div;
#else
                align_y_offset = (ptemp_param->mark[i].align_y_offset*tc_h_div)*osd_y_div/osd_frm_div;
#endif
            }
            else
                align_y_offset = ptemp_param->mark[i].align_y_offset*osd_y_div/osd_frm_div;

            /* Auto Align Ajustment */
            switch(ptemp_param->mark[i].align_type) {
                case VCAP_ALIGN_TOP_CENTER:
                    if(((ptemp_param->tc[path].width/2) - (ptemp_param->mark[i].width/2)) > 0)
                        ptemp_param->mark[i].x_start = (ptemp_param->tc[path].width/2) - (ptemp_param->mark[i].width/2);
                    else
                        ptemp_param->mark[i].x_start = 0;

                    ptemp_param->mark[i].y_start = align_y_offset;
                    break;
                case VCAP_ALIGN_TOP_RIGHT:
                    if((ptemp_param->tc[path].width - ptemp_param->mark[i].width - align_x_offset) > 0)
                        ptemp_param->mark[i].x_start =  ptemp_param->tc[path].width - ptemp_param->mark[i].width - align_x_offset;
                    else
                        ptemp_param->mark[i].x_start = 0;

                    ptemp_param->mark[i].y_start = align_y_offset;
                    break;
                case VCAP_ALIGN_BOTTOM_LEFT:
                    ptemp_param->mark[i].x_start = align_x_offset;

                    if((ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div - (ptemp_param->mark[i].height/osd_h_div) - align_y_offset) > 0)
                        ptemp_param->mark[i].y_start = ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div - (ptemp_param->mark[i].height/osd_h_div) - align_y_offset;
                    else
                        ptemp_param->mark[i].y_start = 0;
                    break;
                case VCAP_ALIGN_BOTTOM_CENTER:
                    if(((ptemp_param->tc[path].width/2) - (ptemp_param->mark[i].width/2)) > 0)
                        ptemp_param->mark[i].x_start = (ptemp_param->tc[path].width/2) - (ptemp_param->mark[i].width/2);
                    else
                        ptemp_param->mark[i].x_start = 0;

                    if((ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div - (ptemp_param->mark[i].height/osd_h_div) - align_y_offset) > 0)
                        ptemp_param->mark[i].y_start = ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div - (ptemp_param->mark[i].height/osd_h_div) - align_y_offset;
                    else
                        ptemp_param->mark[i].y_start = 0;
                    break;
                case VCAP_ALIGN_BOTTOM_RIGHT:
                    if((ptemp_param->tc[path].width - ptemp_param->mark[i].width - align_x_offset) > 0)
                        ptemp_param->mark[i].x_start =  ptemp_param->tc[path].width - ptemp_param->mark[i].width - align_x_offset;
                    else
                        ptemp_param->mark[i].x_start = 0;

                    if((ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div - (ptemp_param->mark[i].height/osd_h_div) - align_y_offset) > 0)
                        ptemp_param->mark[i].y_start = ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div - (ptemp_param->mark[i].height/osd_h_div) - align_y_offset;
                    else
                        ptemp_param->mark[i].y_start = 0;
                    break;
                case VCAP_ALIGN_CENTER:
                    if(((ptemp_param->tc[path].width/2) - (ptemp_param->mark[i].width/2)) > 0)
                        ptemp_param->mark[i].x_start = (ptemp_param->tc[path].width/2) - (ptemp_param->mark[i].width/2);
                    else
                        ptemp_param->mark[i].x_start = 0;

                    if((((ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div)/2) - ((ptemp_param->mark[i].height/osd_h_div)/2)) > 0)
                        ptemp_param->mark[i].y_start = ((ptemp_param->tc[path].height*h_div*tc_h_div*osd_y_div/osd_frm_div)/2) - ((ptemp_param->mark[i].height/osd_h_div)/2);
                    else
                        ptemp_param->mark[i].y_start = 0;
                    break;
                default:    /* VCAP_ALIGN_NONE or VCAP_ALIGN_TOP_LEFT*/
                    ptemp_param->mark[i].x_start = align_x_offset;
                    ptemp_param->mark[i].y_start = align_y_offset;
                    break;
            }

            /* offset to target output window */
#ifdef PLAT_OSD_TC_SWAP
            ptemp_param->mark[i].y_start *= osd_h_div;
#else
            ptemp_param->mark[i].x_start += ptemp_param->tc[path].x_start;
            ptemp_param->mark[i].y_start = (ptemp_param->mark[i].y_start + ptemp_param->tc[path].y_start*h_div*tc_h_div*osd_y_div/osd_frm_div)*osd_h_div;
#endif
            ptemp_param->mark[i].y_start *= osd_h_mul;

            ptemp_param->mark[i].x_start &= 0xfff;
            ptemp_param->mark[i].y_start &= 0xfff;
        }
    }

    /* Grab number */
    job_param->grab_num = 1;
#ifdef VCAP_SPEED_2P_SUPPORT
    if(pdev_info->vi[vi_idx].ch_param[vi_ch].prog == VCAP_VI_PROG_PROGRESSIVE) {
        if(job_param->dst_fmt == TYPE_YUV422_2FRAMES && pdev_info->vi[vi_idx].ch_param[vi_ch].speed == VCAP_VI_SPEED_2P && !p2i) {
            job_param->grab_num++;
        }
    }
#endif

    /* Update Job Output Property */
    if(pdev_info->vi[vi_idx].ch_param[vi_ch].prog == VCAP_VI_PROG_INTERLACE) {
        if((job_param->dst_fmt == TYPE_YUV422_2FRAMES) && job_param->prog_2frm)
            job_param->src_interlace = 2;
        else
            job_param->src_interlace = 1;
    }
    else
        job_param->src_interlace = 0;

    if(!p2i)
        job_param->src_frame_rate = pdev_info->vi[vi_idx].ch_param[vi_ch].frame_rate;
    else {
        job_param->src_frame_rate = pdev_info->vi[vi_idx].ch_param[vi_ch].frame_rate>>1;
        if(!job_param->src_frame_rate)
            job_param->src_frame_rate++;
    }

    job_param->src_bg_dim         = EM_PARAM_DIM(src_bg_w, src_bg_h);
    job_param->src_type           = (pdev_info->vi[vi_idx].format == VCAP_VI_FMT_ISP) ? 1 : 0;
    job_param->target_frame_rate  = (job_param->src_frame_rate*dst_frame_rate)/src_frame_rate;
    job_param->fps_ratio          = ((dst_frame_rate<<16) | src_frame_rate);
    job_param->runtime_frame_rate = job_param->target_frame_rate;
    job_param->dst_xy_offset      = EM_PARAM_XY(bg_x_offset, bg_y_offset);  ///< auto aspect ratio must center output image, need to modify destination output position offset

exit:
    return ret;
}

void vcap_dev_param_sync(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
    if(pdev_info->ch[ch].param_sync) {
        int i;

        struct vcap_ch_param_t *pcurr_param = &pdev_info->ch[ch].param;
        struct vcap_ch_param_t *ptemp_param = &pdev_info->ch[ch].temp_param;

        /* Common */
        if(pdev_info->ch[ch].param_sync & VCAP_PARAM_SYNC_COMM)
            memcpy(&pcurr_param->comm, &ptemp_param->comm, sizeof(struct vcap_comm_param_t));

        /* SC */
        if(pdev_info->ch[ch].param_sync & VCAP_PARAM_SYNC_SC)
            memcpy(&pcurr_param->sc, &ptemp_param->sc, sizeof(struct vcap_sc_param_t));

        /* MD */
        if(pdev_info->ch[ch].param_sync & VCAP_PARAM_SYNC_MD)
            memcpy(&pcurr_param->md, &ptemp_param->md, sizeof(struct vcap_md_param_t));

        /* SD */
        if(pdev_info->ch[ch].param_sync & VCAP_PARAM_SYNC_SD)
            memcpy(&pcurr_param->sd[path], &ptemp_param->sd[path], sizeof(struct vcap_sd_param_t));

        /* TC */
        if(pdev_info->ch[ch].param_sync & VCAP_PARAM_SYNC_TC)
            memcpy(&pcurr_param->tc[path], &ptemp_param->tc[path], sizeof(struct vcap_tc_param_t));

        /* OSD */
        for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
            if(pdev_info->ch[ch].param_sync & (VCAP_PARAM_SYNC_OSD<<i)) {
                memcpy(&pcurr_param->osd[i], &ptemp_param->osd[i], sizeof(struct vcap_osd_param_t));
            }
        }

        /* Mark */
        for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
            if(pdev_info->ch[ch].param_sync & (VCAP_PARAM_SYNC_MARK<<i)) {
                memcpy(&pcurr_param->mark[i], &ptemp_param->mark[i], sizeof(struct vcap_mark_param_t));
            }
        }

        /* OSD_Common */
        if(pdev_info->ch[ch].param_sync & VCAP_PARAM_SYNC_OSD_COMM)
            memcpy(&pcurr_param->osd_comm, &ptemp_param->osd_comm, sizeof(struct vcap_osd_comm_param_t));

        if(ptemp_param->frame_rate_reinit)
            ptemp_param->frame_rate_reinit = 0;

        /* Clear Sync Flag */
        pdev_info->ch[ch].param_sync = 0;
    }
}

int vcap_dev_get_offset(struct vcap_dev_info_t *pdev_info, int ch, int path, void *param, int buf_num, u32 *start_addr, u32 *line_offset, u32 *field_offset)
{
    int p2i = 0;
    u16 bg_w, bg_h, bg_x, bg_y;
    int vi_ch = SUBCH_TO_VICH(ch);
    struct vcap_vi_t  *pvi  = &pdev_info->vi[CH_TO_VI(ch)];
    struct vcap_ch_t  *pch  = &pdev_info->ch[ch];
    struct vcap_vg_job_param_t *job_param = (struct vcap_vg_job_param_t *)param;

    bg_w = EM_PARAM_WIDTH(job_param->dst_bg_dim);
    bg_h = EM_PARAM_HEIGHT(job_param->dst_bg_dim);
    bg_x = EM_PARAM_X(job_param->dst_xy) + EM_PARAM_X(job_param->dst_xy_offset);
    bg_y = EM_PARAM_Y(job_param->dst_xy) + EM_PARAM_Y(job_param->dst_xy_offset);

    /* P2I */
    if(job_param->p2i && (pvi->ch_param[vi_ch].prog == VCAP_VI_PROG_PROGRESSIVE) && (pdev_info->split.ch != ch))    ///< split channel not support P2I
        p2i = 1;

    *start_addr = VCAP_DMA_ADDR(job_param->addr_pa[0] + (bg_w*bg_h*2*buf_num) + ((bg_y*bg_w + bg_x)*2)); ///< YUV422
    switch(job_param->dst_fmt) {
        case TYPE_YUV422_FRAME:
            if(pvi->ch_param[vi_ch].prog == VCAP_VI_PROG_PROGRESSIVE && !p2i) {
                *line_offset  = VCAP_DMA_LINE_OFFSET(bg_w - pch->temp_param.tc[path].width);
                *field_offset = 0;
            }
            else {
                if(job_param->frm_2frm_field) {    ///< grab top/bottom filed to 2frame format, one job buffer one field
                    *line_offset  = VCAP_DMA_LINE_OFFSET(bg_w - pch->temp_param.tc[path].width + bg_w);
                    *field_offset = 0;
                }
                else {
                    *line_offset  = VCAP_DMA_LINE_OFFSET(bg_w - pch->temp_param.tc[path].width + bg_w);
                    if(pdev_info->capability.dma_field_offset_rev == 0)
                        *field_offset = VCAP_DMA_FIELD_OFFSET(bg_w*2);
                    else
                        *field_offset = VCAP_DMA_FIELD_OFFSET_REV1(bg_w*2);

                    /*
                     * Fix v_flip image LCD display top/bottom sequence incorrect problem
                     * Offset one line to paste image. The target crop window must minus one line
                     */
                    if((job_param->job_fmt == TYPE_YUV422_2FRAMES_2FRAMES) && pch->temp_param.comm.v_flip)
                        *start_addr = *start_addr + bg_w*2;
                }
            }
            break;
        case TYPE_YUV422_2FRAMES:
            if(pvi->ch_param[vi_ch].prog == VCAP_VI_PROG_PROGRESSIVE && !p2i) {
#ifdef VCAP_SPEED_2P_SUPPORT
                if(pvi->ch_param[vi_ch].speed == VCAP_VI_SPEED_2P) {   ///< In 50/60P system, 1 job will grab 2 frame
                    *line_offset  = VCAP_DMA_LINE_OFFSET(bg_w - pch->temp_param.tc[path].width);
                    *field_offset = 0;
                }
                else {  ///< In 30P system 1 job will grab 1 frame and need to enable frm2field control
                    *line_offset  = VCAP_DMA_LINE_OFFSET(bg_w - pch->temp_param.tc[path].width + bg_w);
                    *field_offset = VCAP_DMA_FRM2FIELD_FIELD_OFFSET((bg_w*bg_h*2) + (bg_w*2));
                }
#else
                *line_offset  = VCAP_DMA_LINE_OFFSET(bg_w - pch->temp_param.tc[path].width + bg_w);
                *field_offset = VCAP_DMA_FRM2FIELD_FIELD_OFFSET((bg_w*bg_h*2) + (bg_w*2));
#endif
            }
            else {
                if(job_param->prog_2frm && (pvi->ch_param[vi_ch].prog == VCAP_VI_PROG_INTERLACE)) {
                    *line_offset = VCAP_DMA_LINE_OFFSET(bg_w - pch->temp_param.tc[path].width);
                    if(pdev_info->capability.dma_field_offset_rev == 0)
                        *field_offset = VCAP_DMA_FIELD_OFFSET(bg_w*bg_h*2);
                    else
                        *field_offset = VCAP_DMA_FIELD_OFFSET_REV1(bg_w*bg_h*2);
                }
                else {
                    *line_offset  = VCAP_DMA_LINE_OFFSET(bg_w - pch->temp_param.tc[path].width + bg_w);
                    if(pch->temp_param.comm.v_flip) {
                        /*
                         * Fix v_flip image incorrect problem after 3DI process.
                         * Paste first line of bottom field to the last line of top field
                         * (drop one line of bottom field).
                         */
                        if(pdev_info->capability.dma_field_offset_rev == 0)
                            *field_offset = VCAP_DMA_FIELD_OFFSET((bg_w*bg_h*2) - (bg_w*2));
                        else
                            *field_offset = VCAP_DMA_FIELD_OFFSET_REV1((bg_w*bg_h*2) - (bg_w*2));
                    }
                    else {
                        if(pdev_info->capability.dma_field_offset_rev == 0)
                            *field_offset = VCAP_DMA_FIELD_OFFSET((bg_w*bg_h*2) + (bg_w*2));
                        else
                            *field_offset = VCAP_DMA_FIELD_OFFSET_REV1((bg_w*bg_h*2) + (bg_w*2));
                    }
                }
            }
            break;
        default:
            *line_offset  = VCAP_DMA_LINE_OFFSET(bg_w - pch->temp_param.tc[path].width);         ///< pixel unit
            *field_offset = 0;                                                                   ///< byte  unit
            break;
    }

    return 0;
}

int vcap_dev_set_ch_flip(struct vcap_dev_info_t *pdev_info, int ch, int v_flip, int h_flip)
{
    unsigned long flags;

    spin_lock_irqsave(&pdev_info->lock, flags); ///< use spin lock because temp_param would be access in lli tasklet

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.comm.v_flip = v_flip;   ///< lli tasklet will apply this config and sync to current
        pdev_info->ch[ch].temp_param.comm.h_flip = h_flip;
    }
    else {
        /* ch is disable, direct write to register */
        u32 tmp;

        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
        if(h_flip)
            tmp |= BIT28;
        else
            tmp &= (~BIT28);

        if(v_flip)
            tmp |= BIT29;
        else
            tmp &= (~BIT29);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)), tmp);

        pdev_info->ch[ch].param.comm.v_flip = pdev_info->ch[ch].temp_param.comm.v_flip = v_flip;
        pdev_info->ch[ch].param.comm.h_flip = pdev_info->ch[ch].temp_param.comm.h_flip = h_flip;
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_dev_get_ch_flip(struct vcap_dev_info_t *pdev_info, int ch, int *v_flip, int *h_flip)
{
    u32 tmp;

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
    if(tmp & BIT28)
        *h_flip = 1;
    else
        *h_flip = 0;

    if(tmp & BIT29)
        *v_flip = 1;
    else
        *v_flip = 0;

    return 0;
}

int vcap_dev_set_frame_cnt_monitor_ch(struct vcap_dev_info_t *pdev_info, int ch, int clear)
{
    u32 tmp;

    if(ch == VCAP_CASCADE_CH_NUM) {
        tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x64));
        tmp &= ~(0x3f<<4);
        tmp |= ((VCAP_CASCADE_HW_CH_ID & 0x3f)<<4);
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x64), tmp);
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x64));
        tmp &= ~(0x3f<<4);
        tmp |= ((ch & 0x3f)<<4);
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x64), tmp);
    }

    /* Clear hardware frame count monitor */
    if(clear) {
        tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x00));
        tmp |= BIT0;
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x00), tmp);  ///< auto clear
    }

    return 0;
}

int vcap_dev_get_frame_cnt_monitor_ch(struct vcap_dev_info_t *pdev_info)
{
    int ch;
    u32 tmp;

    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x64));
    ch  = (tmp>>4) & 0x3f;

    return ch;
}

int vcap_dev_signal_probe(struct vcap_dev_info_t *pdev_info)
{
    unsigned long flags;
    u32 tmp;
    int i, ret = 0;
    int sub_ch;
    int hcnt;
    int vcnt;

    spin_lock_irqsave(&pdev_info->lock, flags);

    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x68));

    if(!pdev_info->vi_probe.mode) {
        if(tmp & BIT31) {
            tmp &= ~BIT31;  ///< disable vi probe
            VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x68), tmp);
        }
        goto exit;
    }

    if(tmp & BIT31) {       ///< BIT31 will auto clear when vi meassure done
        pdev_info->vi_probe.count++;
        if(pdev_info->vi_probe.count >= (pdev_info->m_param)->sync_time_div) {
            tmp &= ~BIT31;  ///< disable vi probe
            VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x68), tmp);
            pdev_info->vi_probe.count = 0;
            vcap_err_limit("VI#%d Signal Probe Timeout!\n", pdev_info->vi_probe.vi_idx);
        }
    }
    else {
        /* check VI init or not? */
        if(GET_DEV_VI_GST(pdev_info, pdev_info->vi_probe.vi_idx) == VCAP_DEV_STATUS_IDLE) {
            ret = vcap_dev_vi_setup(pdev_info, pdev_info->vi_probe.vi_idx);
            if(ret < 0)
                goto exit;

            /* update vi global status */
            SET_DEV_VI_GST(pdev_info, pdev_info->vi_probe.vi_idx, VCAP_DEV_STATUS_INITED);
        }

        /* check all channel disable */
        for(i=0; i<VCAP_VI_CH_MAX; i++) {
            sub_ch = SUB_CH(pdev_info->vi_probe.vi_idx, i);
            if(VCAP_IS_CH_ENABLE(pdev_info, sub_ch))
                goto exit;
            else if(pdev_info->ch[sub_ch].param.comm.md_en) {
                /* force disable channel MD */
                tmp = VCAP_REG_RD(pdev_info, (sub_ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(sub_ch, 0x08));
                tmp &= (~BIT27);
                VCAP_REG_WR(pdev_info, (sub_ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(sub_ch, 0x08), tmp);
                pdev_info->ch[sub_ch].param.comm.md_en = pdev_info->ch[sub_ch].temp_param.comm.md_en = 0;
            }
        }

        /* Read and Display Previous Probe Result */
        tmp = VCAP_REG_RD(pdev_info, VCAP_STATUS_OFFSET(0xe0));
        if(tmp & BIT31) {
            hcnt = tmp & 0x1fff;
            vcnt = (tmp>>16) & 0x1fff;
            if(pdev_info->vi_probe.mode == VCAP_VI_PROBE_MODE_BLANK_REGION) { ///< Blanking
                switch(pdev_info->vi[pdev_info->vi_probe.vi_idx].format) {
                    case VCAP_VI_FMT_BT1120:
                        hcnt = ((hcnt-5) <= 0) ? 0 : (hcnt-5);
                        break;
                    case VCAP_VI_FMT_SDI8BIT:
                        hcnt = ((hcnt-2) <= 0) ? 0 : (hcnt-2);
                        break;
                    default:
                        hcnt = ((hcnt-2) <= 0) ? 0 : (hcnt-2);
                        break;
                }
                vcap_info("VI#%d Signal Probe Blanking => Width:%d Height:%d (T:%d ms)\n", pdev_info->vi_probe.vi_idx, hcnt, vcnt, pdev_info->vi_probe.count*pdev_info->sync_time_unit);
            }
            else {
                switch(pdev_info->vi[pdev_info->vi_probe.vi_idx].format) {
                    case VCAP_VI_FMT_BT1120:
                        hcnt = ((hcnt-3) <= 0) ? 0 : (hcnt-3);
                        break;
                    case VCAP_VI_FMT_SDI8BIT:
                        hcnt = ((hcnt-1) <= 0) ? 0 : (hcnt-1);
                        break;
                    default:
                        hcnt = ((hcnt-1) <= 0) ? 0 : (hcnt-1);
                        break;
                }
                vcap_info("VI#%d Signal Probe Active => Width:%d Height:%d  (T:%d ms)\n", pdev_info->vi_probe.vi_idx, hcnt, vcnt, pdev_info->vi_probe.count*pdev_info->sync_time_unit);
            }
        }

        /* Select Probe VI */
        tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x6c));
        tmp &= ~(0xf<<4);
        tmp |= (pdev_info->vi_probe.vi_idx<<4);

        /* Select Probe Region */
        if(pdev_info->vi_probe.mode == VCAP_VI_PROBE_MODE_BLANK_REGION)
            tmp |= BIT0;
        else
            tmp &= (~BIT0);
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x6c), tmp);

        /* Clear Probe Result Status */
        VCAP_REG_WR(pdev_info, VCAP_STATUS_OFFSET(0xe0), 0xffffffff);

        /* Start Probe */
        tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x68));
        tmp |= BIT31;
        VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x68), tmp);

        pdev_info->vi_probe.count = 0;
    }

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return ret;
}

int vcap_dev_vi_clock_invert(struct vcap_dev_info_t *pdev_info, int vi_idx, int clk_invert)
{
   return vcap_plat_cap_clk_invert(vi_idx, clk_invert);
}

int vcap_blank_buf_alloc(struct vcap_dev_info_t *pdev_info)
{
#define VCAP_VI_BUF_DDR_IDX DDR_ID_SYSTEM
    int  i;
    int  ret = 0;
    u32  buf_offset = 0;
    char buf_name[20];
    struct frammap_buf_info buf_info;

    if(pdev_info->blank_buf.vaddr) {  ///< buffer have allocated
        goto exit;
    }

    memset(&buf_info, 0, sizeof(struct frammap_buf_info));

    /* Caculate require memory size */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(pdev_info->ch[i].active)
            buf_info.size += ALIGN(VCAP_CH_HBLANK_BUF_SIZE, VCAP_DMA_ADDR_ALIGN_SIZE);
    }

    if(buf_info.size == 0)
        goto exit;

    buf_info.alloc_type = ALLOC_NONCACHABLE;
    sprintf(buf_name, "vcap_blank");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
    buf_info.align = VCAP_DMA_ADDR_ALIGN_SIZE;
    buf_info.name  = buf_name;
#endif
    ret = frm_get_buf_ddr(VCAP_VI_BUF_DDR_IDX, &buf_info);
    if(ret < 0) {
        vcap_err("[%d] blank buffer allocate failed!\n", pdev_info->index);
        goto exit;
    }

    pdev_info->blank_buf.vaddr = (void *)buf_info.va_addr;
    pdev_info->blank_buf.paddr = buf_info.phy_addr;
    pdev_info->blank_buf.size  = buf_info.size;
    strcpy(pdev_info->blank_buf.name, buf_name);

    /* setup blank buffer information for each channel */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(pdev_info->ch[i].active) {
            sprintf(pdev_info->ch[i].hblank_buf.name, "vcap_hblank.%d", i);
            pdev_info->ch[i].hblank_buf.vaddr = (void *)(((u32)pdev_info->blank_buf.vaddr) + buf_offset);
            pdev_info->ch[i].hblank_buf.paddr = pdev_info->blank_buf.paddr + buf_offset;
            pdev_info->ch[i].hblank_buf.size  = ALIGN(VCAP_CH_HBLANK_BUF_SIZE, VCAP_DMA_ADDR_ALIGN_SIZE);

            buf_offset += pdev_info->ch[i].hblank_buf.size;
        }
    }

    /* clear buffer */
    memset(pdev_info->blank_buf.vaddr, 0, pdev_info->blank_buf.size);

exit:
    return ret;
}

void vcap_blank_buf_free(struct vcap_dev_info_t *pdev_info)
{
    if(pdev_info->blank_buf.vaddr) {
        int i;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
        frm_free_buf_ddr(pdev_info->blank_buf.vaddr);
#else
        struct frammap_buf_info buf_info;

        buf_info.va_addr  = (u32)pdev_info->blank_buf.vaddr;
        buf_info.phy_addr = pdev_info->blank_buf.paddr;
        buf_info.size     = pdev_info->blank_buf.size;
        frm_free_buf_ddr(&buf_info);
#endif

        /* clear buffer information */
        for(i=0; i<VCAP_CHANNEL_MAX; i++) {
            if(pdev_info->ch[i].active) {
                pdev_info->ch[i].hblank_buf.vaddr = 0;
                pdev_info->ch[i].hblank_buf.paddr = 0;
                pdev_info->ch[i].hblank_buf.size  = 0;
                memset(pdev_info->ch[i].hblank_buf.name, 0, sizeof(pdev_info->ch[i].hblank_buf.name));
            }
        }
    }
}

void vcap_dev_heartbeat_handler(unsigned long data)
{
#ifdef CFG_HEARTBEAT_TIMER
    u32 tmp;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)data;

    pdev_info->hb_count++;

    /*
     * To check capture enable bit, the hardware will auto shut down capture hardware
     * when detect DMA response fail. The heartbeat handler will trigger fatal reset
     * to try to recover capture hardware to work again. Because hardware not disable
     * DMA before shud down. The capture hardware recover may meet DMA overflow issue.
    */
    if(GET_DEV_GST(pdev_info) == VCAP_DEV_STATUS_INITED) {
        tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x04));
        if((tmp & BIT31) == 0) {
            vcap_err("[%d] Hardware Auto Shut Down!\n", pdev_info->index);
            pdev_info->reset(pdev_info);
        }
    }

    if(pdev_info->hb_timeout > 0)
        mod_timer(&pdev_info->hb_timer, jiffies+msecs_to_jiffies(pdev_info->hb_timeout));
#endif

    return;
}
