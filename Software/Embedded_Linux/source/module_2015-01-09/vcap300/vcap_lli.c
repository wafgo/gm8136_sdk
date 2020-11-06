/**
 * @file vcap_lli.c
 *  vcap300 link-list driver
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.61 $
 * $Date: 2015/01/07 06:02:58 $
 *
 * ChangeLog:
 *  $Log: vcap_lli.c,v $
 *  Revision 1.61  2015/01/07 06:02:58  jerson_l
 *  1. fix dma overflow to trigger hardware fatal reset condition.
 *
 *  Revision 1.60  2014/12/16 09:26:17  jerson_l
 *  1. add md gaussian value check for detect gaussian vaule update error problem when md region switched.
 *  2. add md get all channel tamper status api.
 *
 *  Revision 1.59  2014/11/28 08:52:17  jerson_l
 *  1. support frame 60I feature.
 *
 *  Revision 1.58  2014/11/27 01:59:21  jerson_l
 *  1. add heartbeat timer for watch capture hardware enable status to prevent capture
 *     auto shut down.
 *
 *  Revision 1.57  2014/11/11 05:37:24  jerson_l
 *  1. support group job pending mechanism
 *
 *  Revision 1.56  2014/10/30 01:47:32  jerson_l
 *  1. support vi 2ch dual edge hybrid channel mode.
 *  2. support grab channel horizontal blanking data.
 *
 *  Revision 1.55  2014/09/05 02:48:53  jerson_l
 *  1. support osd force frame mode for GM8136
 *  2. support osd edge smooth edge mode for GM8136
 *  3. support osd auto color change scheme for GM8136
 *
 *  Revision 1.54  2014/08/27 08:40:47  jerson_l
 *  1. mask pixel lack status report in GM8136 ISP interface
 *
 *  Revision 1.53  2014/08/27 02:02:03  jerson_l
 *  1. support GM8136 platform VCAP300 feature
 *
 *  Revision 1.52  2014/08/18 02:13:56  jerson_l
 *  1. fix scaler output image over buffer range problem if scaling size too small
 *
 *  Revision 1.51  2014/07/02 08:53:04  jerson_l
 *  1. disable 2frame_mode to grab 2 frame mechanism if vi speed is VCAP_VI_SPEED_2P
 *
 *  Revision 1.50  2014/06/05 02:26:02  jerson_l
 *  1. support channel path emergency stop
 *
 *  Revision 1.49  2014/05/13 04:01:06  jerson_l
 *  1. change time base frame rate control to frame base
 *
 *  Revision 1.48  2014/05/05 09:31:19  jerson_l
 *  1. support vi signal probe for vi signal debug
 *
 *  Revision 1.47  2014/04/18 01:44:11  jerson_l
 *  1. modify channel timeout detect procedure without using kernel timer
 *
 *  Revision 1.46  2014/04/14 02:51:37  jerson_l
 *  1. support grab_filter to filter fail frame drop rule
 *
 *  Revision 1.45  2014/03/20 05:59:33  jerson_l
 *  1. fix md_enb register not sync with md_enb config problem.
 *
 *  Revision 1.44  2014/03/11 13:53:02  jerson_l
 *  1. support switch VI parameter immediately if front_end output format changed
 *
 *  Revision 1.43  2014/03/06 04:01:58  jerson_l
 *  1. add ext_irq_src module parameter to support extra ll_done interrupt source
 *
 *  Revision 1.42  2014/02/25 07:35:11  jerson_l
 *  1. turn off LL update pool not enough message print out when dbg_mode < 2
 *
 *  Revision 1.41  2014/02/06 02:09:46  jerson_l
 *  1. fix sometime enable/disable mask window fail problem
 *
 *  Revision 1.40  2014/01/24 07:33:10  jerson_l
 *  1. support isp dynamic switch data range
 *
 *  Revision 1.39  2014/01/24 04:12:00  jerson_l
 *  1. support md tamper detection
 *
 *  Revision 1.38  2014/01/24 03:00:39  jerson_l
 *  1. job callback fail when this frame meet sd fail
 *
 *  Revision 1.37  2014/01/22 04:14:44  jerson_l
 *  1. add printk_ratelimit to limit error message print out
 *
 *  Revision 1.36  2014/01/20 02:54:16  jerson_l
 *  1. add capture reset workaround for md error and dma overflow cause dma channel not work problem
 *
 *  Revision 1.35  2013/12/23 03:55:35  jerson_l
 *  1. add no job alarm count debug message
 *
 *  Revision 1.34  2013/12/20 05:23:47  jerson_l
 *  1. correct frame rate check procedure if no apply frame rate control
 *
 *  Revision 1.33  2013/10/28 11:47:29  jerson_l
 *  1. report VI no clock alarm when VI busy and no any channel LLI ready
 *
 *  Revision 1.32  2013/10/25 10:52:14  jerson_l
 *  1. fix frame_rate divide zero problem
 *
 *  Revision 1.31  2013/10/22 06:05:32  jerson_l
 *  1. trigger fatal reset when hardware meet sd job count overflow.
 *
 *  Revision 1.30  2013/10/14 04:02:51  jerson_l
 *  1. support GM8139 platform
 *
 *  Revision 1.29  2013/10/04 02:46:27  jerson_l
 *  1. add register sync procedure when channel timeout happen
 *
 *  Revision 1.28  2013/10/01 06:33:06  jerson_l
 *  1. increase timeout dummy delay time for first job
 *
 *  Revision 1.27  2013/08/26 12:23:00  jerson_l
 *  1. fix md ground not running problem.
 *
 *  Revision 1.26  2013/08/23 09:16:59  jerson_l
 *  1. support specify channel timeout threshold for different front_end device source
 *
 *  Revision 1.25  2013/08/13 07:31:21  jerson_l
 *  1. provide clear hardware frame count monitor counter in set frame count monitor channel api
 *
 *  Revision 1.24  2013/08/05 02:19:05  jerson_l
 *  1. fix md event not update problem in sdi 8 channel platform
 *
 *  Revision 1.23  2013/07/24 02:21:53  jerson_l
 *  1. fix compile error in A369 platfrom
 *
 *  Revision 1.22  2013/07/22 10:12:22  jerson_l
 *  1. fix sometime md event not update problem
 *
 *  Revision 1.21  2013/07/16 03:23:45  jerson_l
 *  1. support scaler presmooth control
 *
 *  Revision 1.20  2013/07/16 02:52:26  jerson_l
 *  1. error message printk use rate control in ISR
 *
 *  Revision 1.19  2013/07/08 03:43:27  jerson_l
 *  1. support auto osd image border property
 *
 *  Revision 1.18  2013/06/24 08:36:17  jerson_l
 *  1. support md grouping
 *
 *  Revision 1.17  2013/06/18 02:17:41  jerson_l
 *  1. modify md buffer mechanism
 *
 *  Revision 1.16  2013/06/04 11:35:43  jerson_l
 *  1. add hardware frame count monitor channel switch if monitor channel disabled
 *
 *  Revision 1.15  2013/05/08 03:29:27  jerson_l
 *  1. disable capture information message output when dbg_mode=0
 *
 *  Revision 1.14  2013/04/30 09:36:34  jerson_l
 *  1. fix hardware error status lose problem in ISR
 *  2. modify md reset procedure when md meet job count overflow
 *  3. remove DMA address and offset comman in dummy table
 *  4. add hardware frame count monitor for debugging
 *
 *  Revision 1.13  2013/04/26 05:27:24  jerson_l
 *  1. fix frame control miss ratio problem
 *
 *  Revision 1.12  2013/04/08 12:15:04  jerson_l
 *  1. add v_flip/h_flip set and get export api
 *
 *  Revision 1.11  2013/03/29 03:42:00  jerson_l
 *  1. improve LLI ISR latency
 *
 *  Revision 1.10  2013/03/26 02:32:25  jerson_l
 *  1. fix hszie of scaler path calculated incorrect issue
 *  2. allocate md statistic buffer after module insert
 *  3. fix split channel DMA address incorrect issue
 *
 *  Revision 1.9  2013/03/19 05:45:50  jerson_l
 *  1. remove field swap if capture style set to evne field first
 *  2. enable dma channel control if capture style set to even field first
 *
 *  Revision 1.8  2013/03/18 05:51:45  jerson_l
 *  1. add diagnostic proc node for error log monitor
 *  2. add dbg_mode proc node for disable/enable error message output control
 *  3. add md_dxdy dynamic switch base on md window x and y size
 *  4. add fatal software reset procedure for recover capture if capture hardware meet fatal error
 *  5. add hw_delay and no_job error notify
 *
 *  Revision 1.7  2013/03/11 08:14:47  jerson_l
 *  1. add P2I support
 *  2. add Cascade P2I workaround, path#3 as workaround path
 *
 *  Revision 1.6  2013/03/04 08:39:07  jerson_l
 *  1. remove IRQF_DISABLED mask in request_irq api
 *
 *  Revision 1.5  2013/01/02 07:34:58  jerson_l
 *  1. support osd/mark window auto align feature
 *
 *  Revision 1.4  2012/12/11 10:35:39  jerson_l
 *  1. add software frame rate control support
 *  2. add MD, FCS, Sharpness, Denoise, OSD, Mask, Mark control procedure
 *
 *  Revision 1.3  2012/10/24 07:12:04  jerson_l
 *  add channel timeout timer
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
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <asm/io.h>

#include "bits.h"
#include "vcap_dev.h"
#include "vcap_input.h"
#include "vcap_proc.h"
#include "vcap_vg.h"
#include "vcap_palette.h"
#include "vcap_fcs.h"
#include "vcap_dn.h"
#include "vcap_presmo.h"
#include "vcap_sharp.h"
#include "vcap_osd.h"
#include "vcap_mark.h"
#include "vcap_md.h"
#include "vcap_lli.h"
#include "vcap_dbg.h"

//#define VCAP_FRAME_RATE_FIELD_MISS_ORDER_BUG
#define VCAP_EMERGENCY_STOP

static inline int VCAP_LLI_ADD_CMD_NULL(struct vcap_lli_table_t *ll_table, int ch, int path)
{
    volatile u32 *ptr;

    if(ll_table->curr_entry >= ll_table->max_entries) {
        vcap_err("Link-List table is out of range!(max=%d)\n", ll_table->max_entries);
        return -1;
    }

    ptr = (volatile u32 *)(ll_table->addr+(ll_table->curr_entry<<3));

    ptr[0] = 0;
    ptr[1] = (VCAP_LLI_DESC_NULL<<29) | (((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CASCADE_LLI_CH_ID : ch)<<23) | (0x1<<(path+19)) | \
             ((ch == VCAP_CASCADE_CH_NUM) ? (VCAP_CAS_SUBCH_OFFSET(0x00)&0x7ffff) : (VCAP_CH_SUBCH_OFFSET(ch, 0x00)&0x7ffff));  ///< to clear Sub_CH offset 0x00 for that path

    ll_table->curr_entry++;

    return 0;
}

static inline int VCAP_LLI_ADD_CMD_STATUS(struct vcap_lli_table_t *ll_table, int ch, int path, u8 split)
{
    volatile u32 *ptr;

    if(ll_table->curr_entry >= ll_table->max_entries) {
        vcap_err("Link-List table is out of range!(max=%d)\n", ll_table->max_entries);
        return -1;
    }

    ptr = (volatile u32 *)(ll_table->addr+(ll_table->curr_entry<<3));

    /* Clear Status Bit */
    ptr[0] = 0;
    ptr[1] = (VCAP_LLI_DESC_STATUS<<29) | (((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CASCADE_LLI_CH_ID : ch)<<23) | (((path & 0x3)<<21) | (split & 0x1)<<20);

    ll_table->curr_entry++;

    return 0;
}

static inline int VCAP_LLI_ADD_CMD_UPDATE(struct vcap_lli_table_t *ll_table, int ch, u8 b_en, u32 reg_ofs, u32 reg_val)
{
    volatile u32 *ptr;

    if(ll_table->curr_entry >= ll_table->max_entries) {
        vcap_err("Link-List table is out of range!(max=%d)\n", ll_table->max_entries);
        return -1;
    }

    ptr = (volatile u32 *)(ll_table->addr+(ll_table->curr_entry<<3));

    ptr[0] = reg_val;
    ptr[1] = (VCAP_LLI_DESC_UPDATE<<29) | (((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CASCADE_LLI_CH_ID : ch)<<23) | ((b_en & 0xf)<<19) | (reg_ofs & 0x7ffff);

    ll_table->curr_entry++;

    return 0;
}

static inline int VCAP_LLI_ADD_CMD_NEXT_LLI(struct vcap_lli_table_t *ll_table, int ch, int path, u8 split, u32 next_p)
{
    volatile u32 *ptr;

    if(ll_table->curr_entry >= ll_table->max_entries) {
        vcap_err("Link-List table is out of range!(max=%d)\n", ll_table->max_entries);
        return -1;
    }

    ptr  = (volatile u32 *)(ll_table->addr+(ll_table->curr_entry<<3));

    ptr[0] = 0;
    ptr[1] = (VCAP_LLI_DESC_NEXT_LLI<<29) | (((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CASCADE_LLI_CH_ID : ch)<<23) | ((path & 0x3)<<21) | ((split & 0x1)<<20) | (next_p & 0x7ffff);

    ll_table->curr_entry++;

    return 0;
}

static inline int VCAP_LLI_ADD_CMD_NEXT_CMD(struct vcap_lli_table_t *ll_table, int ch, int path, u8 split, u32 next_p)
{
    volatile u32 *ptr;

    if(ll_table->curr_entry >= ll_table->max_entries) {
        vcap_err("Link-List table is out of range!(max=%d)\n", ll_table->max_entries);
        return -1;
    }

    ptr  = (volatile u32 *)(ll_table->addr+(ll_table->curr_entry<<3));

    ptr[0] = 0;
    ptr[1] = (VCAP_LLI_DESC_NEXT_CMD<<29) | (((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CASCADE_LLI_CH_ID : ch)<<23) | ((path & 0x3)<<21) | ((split & 0x1)<<20) | (next_p & 0x7ffff);

    ll_table->curr_entry++;

    return 0;
}

static inline int VCAP_LLI_ADD_CMD_INFO(struct vcap_lli_table_t *ll_table, int ch, int path, u8 split, u32 d1, u32 d0)
{
    volatile u32 *ptr;

    if(ll_table->curr_entry >= ll_table->max_entries) {
        vcap_err("Link-List table is out of range!(max=%d)\n", ll_table->max_entries);
        return -1;
    }

    ptr  = (volatile u32 *)(ll_table->addr+(ll_table->curr_entry*8));

    ptr[0] = d0;
    ptr[1] = (VCAP_LLI_DESC_INFO<<29) | (((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CASCADE_LLI_CH_ID : ch)<<23) | ((path & 0x3)<<21) | ((split & 0x1)<<20) | (d1 & 0x7ffff);

    ll_table->curr_entry++;

    return 0;
}

static inline void vcap_lli_add_element(struct vcap_lli_pool_t *pool, void *element)
{
    pool->elements[pool->curr_idx++] = element;
}

static inline void *vcap_lli_remove_element(struct vcap_lli_pool_t *pool)
{
    if((pool->curr_idx-1) >= 0)
        return pool->elements[--pool->curr_idx];

    return NULL;
}

static inline struct vcap_lli_table_t *vcap_lli_alloc_normal_table(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
    unsigned long flags;
    struct vcap_lli_table_t *ll_table = NULL;
    struct vcap_lli_path_t *plli_path = &pdev_info->lli.ch[ch].path[path];

    spin_lock_irqsave(&plli_path->ll_pool.lock, flags);
    if(plli_path->ll_pool.curr_idx) {
        ll_table = (struct vcap_lli_table_t *)vcap_lli_remove_element(&plli_path->ll_pool);
        ll_table->curr_entry = 0;
        ll_table->job        = NULL;
        ll_table->type       = VCAP_LLI_TAB_TYPE_NORMAL;
        INIT_LIST_HEAD(&ll_table->list);
        INIT_LIST_HEAD(&ll_table->update_head);
    }
    spin_unlock_irqrestore(&plli_path->ll_pool.lock, flags);

    return ll_table;
}

static inline struct vcap_lli_table_t *vcap_lli_alloc_null_table(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
    unsigned long flags;
    struct vcap_lli_table_t *ll_table = NULL;
    struct vcap_lli_path_t *plli_path = &pdev_info->lli.ch[ch].path[path];

    spin_lock_irqsave(&plli_path->null_pool.lock, flags);
    if(plli_path->null_pool.curr_idx) {
        ll_table = (struct vcap_lli_table_t *)vcap_lli_remove_element(&plli_path->null_pool);
        ll_table->curr_entry = 0;
        ll_table->job        = NULL;
        ll_table->type       = VCAP_LLI_TAB_TYPE_NORMAL;
        INIT_LIST_HEAD(&ll_table->list);
        INIT_LIST_HEAD(&ll_table->update_head);
    }
    spin_unlock_irqrestore(&plli_path->null_pool.lock, flags);

    return ll_table;
}

static inline struct vcap_lli_table_t *vcap_lli_alloc_update_table(struct vcap_dev_info_t *pdev_info)
{
    unsigned long flags;
    struct vcap_lli_table_t *ll_table = NULL;
    struct vcap_lli_t *plli = &pdev_info->lli;

    spin_lock_irqsave(&plli->update_pool.lock, flags);
    if(plli->update_pool.curr_idx) {
        ll_table = (struct vcap_lli_table_t *)vcap_lli_remove_element(&plli->update_pool);
        ll_table->curr_entry = 0;
        ll_table->job        = NULL;
        ll_table->type       = VCAP_LLI_TAB_TYPE_NORMAL;
        INIT_LIST_HEAD(&ll_table->list);
        INIT_LIST_HEAD(&ll_table->update_head);
    }
    spin_unlock_irqrestore(&plli->update_pool.lock, flags);

    return ll_table;
}

static inline int vcap_lli_free_null_table(struct vcap_dev_info_t *pdev_info, int ch, int path, struct vcap_lli_table_t *ll_table)
{
    unsigned long flags;
    struct vcap_lli_t *plli = &pdev_info->lli;

    /* Free Null Table */
    spin_lock_irqsave(&plli->ch[ch].path[path].null_pool.lock, flags);
    list_del(&ll_table->list);
    vcap_lli_add_element(&plli->ch[ch].path[path].null_pool, (void *)ll_table);
    spin_unlock_irqrestore(&plli->ch[ch].path[path].null_pool.lock, flags);

    return 0;
}

static inline int vcap_lli_free_normal_table(struct vcap_dev_info_t *pdev_info, int ch, int path, struct vcap_lli_table_t *ll_table)
{
    unsigned long flags;
    struct vcap_lli_table_t *curr_up_table, *next_up_table;
    struct vcap_lli_t *plli = &pdev_info->lli;

    /* Free Update Table */
    spin_lock_irqsave(&plli->update_pool.lock, flags);
    list_for_each_entry_safe(curr_up_table, next_up_table, &ll_table->update_head, list) {
        list_del(&curr_up_table->list);
        vcap_lli_add_element(&plli->update_pool, (void *)curr_up_table);
    }
    spin_unlock_irqrestore(&plli->update_pool.lock, flags);

    /* Free Normal Table */
    spin_lock_irqsave(&plli->ch[ch].path[path].ll_pool.lock, flags);
    list_del(&ll_table->list);
    vcap_lli_add_element(&plli->ch[ch].path[path].ll_pool, (void *)ll_table);
    spin_unlock_irqrestore(&plli->ch[ch].path[path].ll_pool.lock, flags);

    return 0;
}

static inline int vcap_lli_set_ll_current(struct vcap_dev_info_t *pdev_info, int ch, int path, u32 curr_addr)
{
    u32 tmp;

    if(ch == VCAP_CASCADE_CH_NUM) {
        tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_LL_OFFSET((0x4*(path/2))));
        tmp &= ~(0xffff<<((path%2)*16));
        tmp |= ((curr_addr&0xfffe)<<((path%2)*16));
        VCAP_REG_WR(pdev_info, VCAP_CAS_LL_OFFSET((0x4*(path/2))), tmp);
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, VCAP_CH_LL_OFFSET(ch, (0x4*(path/2))));
        tmp &= ~(0xffff<<((path%2)*16));
        tmp |= ((curr_addr&0xfffe)<<((path%2)*16));
        VCAP_REG_WR(pdev_info, VCAP_CH_LL_OFFSET(ch, (0x4*(path/2))), tmp);
    }

    return 0;
}

static inline int vcap_lli_set_ll_next(struct vcap_dev_info_t *pdev_info, int ch, int path, u32 next_addr)
{
    u32 tmp;

    if(ch == VCAP_CASCADE_CH_NUM) {
        tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_LL_OFFSET(0x8+(0x4*(path/2))));
        tmp &= ~(0xffff<<((path%2)*16));
        tmp |= ((next_addr&0xfffe)<<((path%2)*16));  ///< clear next stop bit
        VCAP_REG_WR(pdev_info, VCAP_CAS_LL_OFFSET(0x8+(0x4*(path/2))), tmp);
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, VCAP_CH_LL_OFFSET(ch, 0x8+(0x4*(path/2))));
        tmp &= ~(0xffff<<((path%2)*16));
        tmp |= ((next_addr&0xfffe)<<((path%2)*16));  ///< clear next stop bit
        VCAP_REG_WR(pdev_info, VCAP_CH_LL_OFFSET(ch, 0x8+(0x4*(path/2))), tmp);
    }

    return 0;
}

static inline int vcap_lli_set_next_stop(struct vcap_dev_info_t *pdev_info, int ch, int path, u8 stop)
{
    u32 tmp;

    if(ch == VCAP_CASCADE_CH_NUM) {
        tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_LL_OFFSET(0x8+(0x4*(path/2))));
        if(stop)
            tmp |= (0x1<<((path%2)*16));
        else
            tmp &= ~(0x1<<((path%2)*16));
        VCAP_REG_WR(pdev_info, VCAP_CAS_LL_OFFSET(0x8+(0x4*(path/2))), tmp);
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, VCAP_CH_LL_OFFSET(ch, (0x8+(0x4*(path/2)))));
        if(stop)
            tmp |= (0x1<<((path%2)*16));
        else
            tmp &= ~(0x1<<((path%2)*16));
        VCAP_REG_WR(pdev_info, VCAP_CH_LL_OFFSET(ch, (0x8+(0x4*(path/2)))), tmp);
    }

    return 0;
}

static int vcap_lli_ll_init(struct vcap_dev_info_t *pdev_info)
{
    int i, j;
    struct vcap_lli_table_t *null_table;
    struct vcap_lli_t *plli = &pdev_info->lli;

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        for(j=0; j<VCAP_SCALER_MAX; j++) {
            vcap_lli_set_ll_current(pdev_info, i, j, 0);

            if(plli->ch[i].path[j].null_pool.curr_idx) {
                /* Allocate Null Table */
                null_table = vcap_lli_alloc_null_table(pdev_info, i, j);
                if(null_table) {
                    /* Add NULL_CMD to Null Table */
                    VCAP_LLI_ADD_CMD_NULL(null_table, i, j);

                    /* Add Null Table to list */
                    list_add_tail(&null_table->list, &plli->ch[i].path[j].null_head);

                    vcap_lli_set_ll_next(pdev_info, i, j, null_table->addr);
                }
                else {
                    vcap_lli_set_ll_next(pdev_info, i, j, 0);
                    vcap_lli_set_next_stop(pdev_info, i, j, 1);
                }
            }
            else {
                vcap_lli_set_ll_next(pdev_info, i, j, 0);
                vcap_lli_set_next_stop(pdev_info, i, j, 1);
            }
        }
    }

    return 0;
}

static inline int vcap_lli_link_list(struct vcap_dev_info_t *pdev_info, int ch, int path, int split, struct vcap_lli_table_t *ll_table)
{
    int ret = 0;
    struct vcap_lli_table_t *last_n_table, *last_up_table, *last_table;
    struct vcap_lli_table_t *null_table = NULL;
    struct vcap_lli_t *plli = &pdev_info->lli;

    /* Allocate Null Table */
    null_table = vcap_lli_alloc_null_table(pdev_info, ch, path);
    if(!null_table) {
        vcap_err("[%d] ch#%d-p%d allocate null table failed!\n", pdev_info->index, ch, path);
        ret = -1;
        goto exit;
    }

    if(ll_table->type == VCAP_LLI_TAB_TYPE_DUMMY_LOOP) {
        /* Add NEXT_LL_CMD to loop to last Null Table */
        if(!list_empty(&plli->ch[ch].path[path].null_head)) {
            /* Get last Null Table */
            last_n_table = list_entry(plli->ch[ch].path[path].null_head.prev, typeof(*null_table), list);
            ret = VCAP_LLI_ADD_CMD_NEXT_LLI(null_table, ch, path, split, last_n_table->addr);
            if(ret < 0) {
                vcap_err("[%d] ch#%d-p%d Add Next_LL Command failed!!\n", pdev_info->index, ch, path);
                goto exit;
            }
        }
        else {
            vcap_err("[%d] ch#%d-p%d Null List is empty!!\n", pdev_info->index, ch, path);
            ret = -1;
            goto exit;
        }
    }
    else {
        /* Add NULL_CMD to Null Table */
        ret = VCAP_LLI_ADD_CMD_NULL(null_table, ch, path);
        if(ret < 0) {
            vcap_err("[%d] ch#%d-p%d Add Null Command failed!\n", pdev_info->index, ch, path);
            goto exit;
        }
    }

    /* Add NEXT_LL_CMD to link to Null Table */
    if(list_empty(&ll_table->update_head)) {
        ret = VCAP_LLI_ADD_CMD_NEXT_LLI(ll_table, ch, path, split, null_table->addr);
        if(ret < 0) {
            vcap_err("[%d] ch#%d-p%d Add Next_LL Command failed!!\n", pdev_info->index, ch, path);
            goto exit;
        }
    }
    else {
        last_up_table = list_entry(ll_table->update_head.prev, typeof(*ll_table), list);
        ret = VCAP_LLI_ADD_CMD_NEXT_LLI(last_up_table, ch, path, split, null_table->addr);
        if(ret < 0) {
            vcap_err("[%d] ch#%d-p%d Add Next_LL Command failed!!\n", pdev_info->index, ch, path);
            goto exit;
        }
    }

    /* Modify Last NULL_CMD of Null Table to NEXT_CMD to link to Normal Table */
    if(!list_empty(&plli->ch[ch].path[path].null_head)) {
        /* Get last Null Table */
        last_n_table = list_entry(plli->ch[ch].path[path].null_head.prev, typeof(*null_table), list);
        last_n_table->curr_entry--;
        ret = VCAP_LLI_ADD_CMD_NEXT_CMD(last_n_table, ch, path, split, ll_table->addr);

        /* Get last Normal Table */
        last_table = list_entry(plli->ch[ch].path[path].ll_head.prev, typeof(*last_table), list);
        if(last_table->type == VCAP_LLI_TAB_TYPE_DUMMY_LOOP)
            last_table->type = VCAP_LLI_TAB_TYPE_DUMMY;     ///< change table type, the IRQ handler will free this table
    }
    else {
        vcap_err("[%d] ch#%d-p%d Null List is empty!!\n", pdev_info->index, ch, path);
        ret = -1;
    }

    if(ret == 0) {
        /* Add Normal Table to list */
        list_add_tail(&ll_table->list, &plli->ch[ch].path[path].ll_head);

        /* Add Null Table to list */
        list_add_tail(&null_table->list, &plli->ch[ch].path[path].null_head);
    }

exit:
    /* Free Null Table */
    if(ret < 0 && null_table) {
        vcap_lli_free_null_table(pdev_info, ch, path, null_table);
    }

    return ret;
}

static inline int vcap_lli_link_update_list(int ch, int path, int split, struct vcap_lli_table_t *ll_table, struct vcap_lli_table_t *update_table)
{
    int ret = 0;
    struct vcap_lli_table_t *last_up_table;

    if(ll_table == NULL || update_table == NULL)
        return -1;

    /* Link update table to list */
    if(!list_empty(&ll_table->update_head)) {
        last_up_table = list_entry(ll_table->update_head.prev, typeof(*ll_table), list);
        ret = VCAP_LLI_ADD_CMD_NEXT_CMD(last_up_table, ch, path, split, update_table->addr);
    }
    else {
        ret = VCAP_LLI_ADD_CMD_NEXT_CMD(ll_table, ch, path, split, update_table->addr);
    }

    if(ret == 0)
        list_add_tail(&update_table->list, &ll_table->update_head);

    return ret;
}

static inline int vcap_lli_add_update_reg(struct vcap_dev_info_t *pdev_info, int ch, struct vcap_reg_data_t *reg_data)
{
    struct vcap_lli_ch_t *plli_ch = &pdev_info->lli.ch[ch];

    /* Set update state and clear update count */
    if(plli_ch->update_reg.state == VCAP_UPDATE_IDLE) {
        plli_ch->update_reg.state = VCAP_UPDATE_PREPARE;
        plli_ch->update_reg.num   = 0;
    }

    /* Add update command register */
    if(plli_ch->update_reg.state == VCAP_UPDATE_PREPARE) {
        if(plli_ch->update_reg.num >= VCAP_UPDATE_REG_MAX) {
            vcap_err("[%d] ch#%d update register count over range\n", pdev_info->index, ch);
            return -1;
        }
        else {
            memcpy(&plli_ch->update_reg.reg[plli_ch->update_reg.num++], reg_data, sizeof(struct vcap_reg_data_t));
        }
    }
    else if(plli_ch->update_reg.state == VCAP_UPDATE_GO) {
        vcap_err("[%d] ch#%d update command ongoing\n", pdev_info->index, ch);
        return -1;
    }

    return 0;
}

static inline void vcap_lli_update_reg_go(struct vcap_dev_info_t *pdev_info, int ch)
{
    struct vcap_lli_ch_t *plli_ch = &pdev_info->lli.ch[ch];

    if((plli_ch->update_reg.state == VCAP_UPDATE_PREPARE) && plli_ch->update_reg.num)
        plli_ch->update_reg.state = VCAP_UPDATE_GO;
}

static inline void vcap_lli_flush_update_reg(struct vcap_dev_info_t *pdev_info, int ch)
{
    struct vcap_lli_ch_t *plli_ch = &pdev_info->lli.ch[ch];

    if(plli_ch->update_reg.state != VCAP_UPDATE_IDLE) {
        plli_ch->update_reg.state = VCAP_UPDATE_IDLE;
        plli_ch->update_reg.num   = 0;
    }
}

static inline int vcap_lli_prepare_update_reg(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
    int i, ret = 0;
    int frm_ctrl_switch = 0;
    u32 param_sync = 0;
    struct vcap_reg_data_t reg_data;
    struct vcap_ch_param_t *pcurr_param = &pdev_info->ch[ch].param;
    struct vcap_ch_param_t *ptemp_param = &pdev_info->ch[ch].temp_param;

    /* SC */
    if((pcurr_param->sc.x_start != ptemp_param->sc.x_start) ||
       (pcurr_param->sc.width   != ptemp_param->sc.width)) {
        reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x00) : VCAP_SC_OFFSET(ch*0x8);
        reg_data.mask   = 0xf;
        reg_data.value  = (ptemp_param->sc.x_start) |
                          ((ptemp_param->sc.x_start + ptemp_param->sc.width - 1)<<16) |
                          (pdev_info->ch[ch].sc_rolling<<14);
        ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
        if(ret < 0)
            goto exit;

        param_sync |= VCAP_PARAM_SYNC_SC;
    }

    if((pcurr_param->sc.y_start != ptemp_param->sc.y_start) ||
       (pcurr_param->sc.height  != ptemp_param->sc.height)) {
        reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x04) : VCAP_SC_OFFSET(0x04 + ch*0x8);
        reg_data.mask   = 0xf;
        reg_data.value  = (ptemp_param->sc.y_start) |
                          ((ptemp_param->sc.y_start + ptemp_param->sc.height - 1)<<16) |
                          (pdev_info->ch[ch].sc_type<<14);
        ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
        if(ret < 0)
            goto exit;

        param_sync |= VCAP_PARAM_SYNC_SC;
    }

    /* SD */
    if((pcurr_param->sd[path].out_w != ptemp_param->sd[path].out_w) ||
       (pcurr_param->sd[path].out_h != ptemp_param->sd[path].out_h)) {
        reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x10+(path*0x4)) : VCAP_CH_SUBCH_OFFSET(ch, 0x10+(path*0x4));
        reg_data.mask   = 0xf;
        reg_data.value  = ptemp_param->sd[path].out_w | (ptemp_param->sd[path].out_h<<16);

        ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
        if(ret < 0)
            goto exit;

        param_sync |= VCAP_PARAM_SYNC_SD;
    }

    if((pcurr_param->sd[path].hstep != ptemp_param->sd[path].hstep) ||
       (pcurr_param->sd[path].vstep != ptemp_param->sd[path].vstep)) {
        reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x8+(path*0x10)) : VCAP_CH_DNSD_OFFSET(ch, (0x8+(path*0x10)));
        reg_data.mask   = 0xf;
        reg_data.value  = ptemp_param->sd[path].hstep | (ptemp_param->sd[path].vstep<<16);

        ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
        if(ret < 0)
            goto exit;

        param_sync |= VCAP_PARAM_SYNC_SD;
    }

    if((pcurr_param->sd[path].topvoft  != ptemp_param->sd[path].topvoft)  ||
       (pcurr_param->sd[path].botvoft  != ptemp_param->sd[path].botvoft)  ||
       (pcurr_param->sd[path].hsmo_str != ptemp_param->sd[path].hsmo_str) ||
       (pcurr_param->sd[path].vsmo_str != ptemp_param->sd[path].vsmo_str)) {
        reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0xc+(path*0x10)) : VCAP_CH_DNSD_OFFSET(ch, (0xc+(path*0x10)));
        reg_data.mask   = 0xf;
        reg_data.value  = (ptemp_param->sd[path].topvoft)      |
                          (ptemp_param->sd[path].botvoft<<13)  |
                          (ptemp_param->sd[path].hsmo_str<<26) |
                          (ptemp_param->sd[path].vsmo_str<<29);

        ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
        if(ret < 0)
            goto exit;

        param_sync |= VCAP_PARAM_SYNC_SD;
    }

    if((pcurr_param->sd[path].frm_ctrl != ptemp_param->sd[path].frm_ctrl) ||
       (pcurr_param->sd[path].frm_wrap != ptemp_param->sd[path].frm_wrap)) {
        reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x10+(path*0x10)) : VCAP_CH_DNSD_OFFSET(ch, (0x10+(path*0x10)));
        reg_data.mask   = 0xf;
        reg_data.value  = ptemp_param->sd[path].frm_ctrl;

        ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
        if(ret < 0)
            goto exit;

        frm_ctrl_switch++;

        param_sync |= VCAP_PARAM_SYNC_SD;
    }

    if((pcurr_param->sd[path].frm_wrap     != ptemp_param->sd[path].frm_wrap)     ||
       (pcurr_param->sd[path].sharp.adp    != ptemp_param->sd[path].sharp.adp)    ||
       (pcurr_param->sd[path].sharp.radius != ptemp_param->sd[path].sharp.radius) ||
       (pcurr_param->sd[path].sharp.amount != ptemp_param->sd[path].sharp.amount) ||
       (pcurr_param->sd[path].sharp.thresh != ptemp_param->sd[path].sharp.thresh) ||
       (pcurr_param->sd[path].sharp.start  != ptemp_param->sd[path].sharp.start)  ||
       (pcurr_param->sd[path].sharp.step   != ptemp_param->sd[path].sharp.step) ) {

        reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x14+(path*0x10)) : VCAP_CH_DNSD_OFFSET(ch, 0x14+(path*0x10));
        reg_data.mask   = 0xf;
        reg_data.value  = (ptemp_param->sd[path].frm_wrap)         |
                          (ptemp_param->sd[path].sharp.adp<<5)     |
                          (ptemp_param->sd[path].sharp.radius<<6)  |
                          (ptemp_param->sd[path].sharp.amount<<9)  |
                          (ptemp_param->sd[path].sharp.thresh<<15) |
                          (ptemp_param->sd[path].sharp.start<<21)  |
                          (ptemp_param->sd[path].sharp.step<<27);

        ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
        if(ret < 0)
            goto exit;

        param_sync |= VCAP_PARAM_SYNC_SD;
    }

    if((pcurr_param->sd[path].hsize != ptemp_param->sd[path].hsize) ||
       (pcurr_param->sd[path].src_w != ptemp_param->sd[path].src_w) ||
       (pcurr_param->sd[path].src_h != ptemp_param->sd[path].src_h)) {
        param_sync |= VCAP_PARAM_SYNC_SD;
    }

    if(frm_ctrl_switch || ptemp_param->frame_rate_reinit) {
        for(i=0; i<VCAP_SCALER_MAX; i++) {
            if(!ptemp_param->frame_rate_reinit && (i != path))
                continue;

            /* Frame Control ReInit ID */
            reg_data.offset = VCAP_GLOBAL_OFFSET(0x04);
            reg_data.mask   = 0x2;
            reg_data.value  = (((ch<<2)|i)<<8);
            ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
            if(ret < 0)
                goto exit;

            /* Frame Control ReInit */
            reg_data.offset = VCAP_GLOBAL_OFFSET(0x00);
            reg_data.mask   = 0x1;
            reg_data.value  = BIT4;     ///< auto clear
            ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
            if(ret < 0)
                goto exit;
        }
    }

    /* TC */
    if(pcurr_param->tc[path].width != ptemp_param->tc[path].width) {
        reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x20+((path/2)*0x4)) : VCAP_CH_SUBCH_OFFSET(ch, 0x20+((path/2)*0x4));
        reg_data.mask   = 0x3<<((path%2)*2);
        reg_data.value  = ptemp_param->tc[path].width<<((path%2)*16);

        ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
        if(ret < 0)
            goto exit;

        param_sync |= VCAP_PARAM_SYNC_TC;
    }

    if((pcurr_param->tc[path].x_start != ptemp_param->tc[path].x_start) ||
       (pcurr_param->tc[path].width   != ptemp_param->tc[path].width)) {
        reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_TC_OFFSET(path*0x8) : VCAP_CH_TC_OFFSET(ch, path*0x8);
        reg_data.mask   = 0xf;
        reg_data.value  = (ptemp_param->tc[path].x_start) |
                          ((ptemp_param->tc[path].x_start + ptemp_param->tc[path].width - 1))<<16;

        ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
        if(ret < 0)
            goto exit;

        param_sync |= VCAP_PARAM_SYNC_TC;
    }

    if((pcurr_param->tc[path].y_start  != ptemp_param->tc[path].y_start) ||
       (pcurr_param->tc[path].height   != ptemp_param->tc[path].height)  ||
       (pcurr_param->tc[path].border_w != ptemp_param->tc[path].border_w)) {
        reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_TC_OFFSET(0x4+(path*0x8)) : VCAP_CH_TC_OFFSET(ch, 0x4+(path*0x8));
        reg_data.mask   = 0xf;
        reg_data.value  = (ptemp_param->tc[path].y_start)            |
                          ((ptemp_param->tc[path].border_w&0x7)<<13) |
                          ((ptemp_param->tc[path].y_start + ptemp_param->tc[path].height - 1)<<16) |
                          (((ptemp_param->tc[path].border_w>>3)&0x1)<<29);

        ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
        if(ret < 0)
            goto exit;

        param_sync |= VCAP_PARAM_SYNC_TC;
    }

    /* MD */
    if((pdev_info->m_param)->cap_md) {
        if((pcurr_param->md.x_start != ptemp_param->md.x_start) ||
           (pcurr_param->md.y_start != ptemp_param->md.y_start) ) {
            reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x00) : VCAP_CH_MD_OFFSET(ch, 0x00);
            reg_data.mask   = 0xf;
            reg_data.value  = ptemp_param->md.x_start | (ptemp_param->md.y_start<<16);

            ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
            if(ret < 0)
                goto exit;

            param_sync |= VCAP_PARAM_SYNC_MD;
        }

        if((pcurr_param->md.x_size != ptemp_param->md.x_size)   ||
           (pcurr_param->md.y_size != ptemp_param->md.y_size)   ||
           (pcurr_param->md.x_num  != ptemp_param->md.x_num)    ||
           (pcurr_param->md.y_num  != ptemp_param->md.y_num)) {
            reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x04) : VCAP_CH_MD_OFFSET(ch, 0x04);
            reg_data.mask   = 0xf;
            reg_data.value  = ptemp_param->md.x_size | (ptemp_param->md.y_size<<8) | (ptemp_param->md.x_num<<16) | (ptemp_param->md.y_num<<24);

            ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
            if(ret < 0)
                goto exit;

            /* switch md_dxdy base on x_size and y_size */
            if((pcurr_param->md.x_size != ptemp_param->md.x_size) || (pcurr_param->md.y_size != ptemp_param->md.y_size)) {
                reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x14) : VCAP_CH_MD_OFFSET(ch, 0x14);
                reg_data.mask   = 0x3;
                reg_data.value  = VCAP_MD_DXDY(ptemp_param->md.x_size, ptemp_param->md.y_size);

                ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
                if(ret < 0)
                    goto exit;
            }

            param_sync |= VCAP_PARAM_SYNC_MD;
        }

        if((pcurr_param->md.gau_addr   != ptemp_param->md.gau_addr) ||
           (pcurr_param->md.event_addr != ptemp_param->md.event_addr)) {
            if(ch == pdev_info->split.ch) {
                int i;

                for(i=0; i<pdev_info->split.x_num*pdev_info->split.y_num; i++) {
                    reg_data.offset = VCAP_SPLIT_CH_OFFSET(i, 0x10);
                    reg_data.mask   = 0xf;
                    reg_data.value  = ptemp_param->md.gau_addr + (VCAP_MD_GAU_SIZE*i);     ///< gau address

                    ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
                    if(ret < 0)
                        goto exit;

                    reg_data.offset = VCAP_SPLIT_CH_OFFSET(i, 0x14);
                    reg_data.mask   = 0xf;
                    reg_data.value  = ptemp_param->md.event_addr + (VCAP_MD_EVENT_SIZE*i); ///< event address

                    ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
                    if(ret < 0)
                        goto exit;
                }
            }
            else {
                reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DMA_OFFSET(0x20) : VCAP_CH_DMA_OFFSET(ch, 0x20);
                reg_data.mask   = 0xf;
                reg_data.value  = ptemp_param->md.gau_addr;     ///< gau address

                ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
                if(ret < 0)
                    goto exit;

                reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DMA_OFFSET(0x24) : VCAP_CH_DMA_OFFSET(ch, 0x24);
                reg_data.mask   = 0xf;
                reg_data.value  = ptemp_param->md.event_addr;   ///< event address

                ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
                if(ret < 0)
                    goto exit;
            }

            param_sync |= VCAP_PARAM_SYNC_MD;
        }
    }

    /* Common */
    if(pdev_info->ch[ch].comm_path == path) {
        if((pcurr_param->comm.mask_en != ptemp_param->comm.mask_en) ||
           (pcurr_param->comm.fcs_en  != ptemp_param->comm.fcs_en)  ||
           (pcurr_param->comm.dn_en   != ptemp_param->comm.dn_en)) {
            reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x04) : VCAP_CH_SUBCH_OFFSET(ch, 0x04);
            reg_data.mask   = 0x3;
            reg_data.value  = (ptemp_param->comm.mask_en)  |
                              (ptemp_param->comm.fcs_en<<8)|
                              (ptemp_param->comm.dn_en<<9);

            ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
            if(ret < 0)
                goto exit;

            param_sync |= VCAP_PARAM_SYNC_COMM;
        }

        if((pcurr_param->comm.osd_en          != ptemp_param->comm.osd_en)          ||
           (pcurr_param->comm.mark_en         != ptemp_param->comm.mark_en)         ||
           (pcurr_param->comm.h_flip          != ptemp_param->comm.h_flip)          ||
           (pcurr_param->comm.v_flip          != ptemp_param->comm.v_flip)          ||
           (pcurr_param->comm.border_en       != ptemp_param->comm.border_en)       ||
           (pcurr_param->comm.osd_frm_mode    != ptemp_param->comm.osd_frm_mode)    ||
#ifdef PLAT_OSD_COLOR_SCHEME
           (pcurr_param->comm.accs_data_thres != ptemp_param->comm.accs_data_thres) ||
#endif
           (pcurr_param->comm.md_en           != ptemp_param->comm.md_en)           ||
           (pcurr_param->comm.md_src          != ptemp_param->comm.md_src)) {
            reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08);
            reg_data.mask   = 0xf;
            reg_data.value  = (ptemp_param->comm.osd_en)             |
                              (ptemp_param->comm.mark_en<<8)         |
                              (ptemp_param->comm.border_en<<12)      |
                              (ptemp_param->comm.osd_frm_mode<<16)   |
#ifdef PLAT_OSD_COLOR_SCHEME
                              (ptemp_param->comm.accs_data_thres<<20)|
#endif
                              (ptemp_param->comm.md_src<<24)         |
                              (ptemp_param->comm.md_en<<27)          |
                              (ptemp_param->comm.h_flip<<28)         |
                              (ptemp_param->comm.v_flip<<29);

            ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
            if(ret < 0)
                goto exit;

            param_sync |= VCAP_PARAM_SYNC_COMM;
        }

        /* OSD Common */
        if((pcurr_param->osd_comm.border_color   != ptemp_param->osd_comm.border_color)  ||
           (pcurr_param->osd_comm.marquee_speed  != ptemp_param->osd_comm.marquee_speed) ||
           (pcurr_param->osd_comm.font_edge_mode != ptemp_param->osd_comm.font_edge_mode)) {
            reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x00) : VCAP_CH_OSD_OFFSET(ch, 0x00);
            reg_data.mask   = 0x6;
            reg_data.value  = (ptemp_param->osd_comm.marquee_speed<<8)    |
                              (ptemp_param->osd_comm.border_color<<12)    |
                              (ptemp_param->osd_comm.font_edge_mode<<16)  |
#ifdef PLAT_OSD_COLOR_SCHEME
                              (VCAP_OSD_ACCS_HYSTERESIS_RANGE_DEFAULT<<10)|
                              (VCAP_OSD_ACCS_ADJUST_ENB_DEFAULT<<17)      |
#endif
                              (VCAP_MARK_ZOOM_ADJUST_CR_DEFAULT<<18);
            ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
            if(ret < 0)
                goto exit;

            param_sync |= VCAP_PARAM_SYNC_OSD_COMM;
        }
    }

    /* OSD */
    for(i=0; i<VCAP_OSD_WIN_MAX; i++) {
        if((ptemp_param->comm.osd_en & (0x1<<i)) && (ptemp_param->osd[i].path == path)) {
            /* Reg#0 */
            if((pcurr_param->osd[i].wd_addr != ptemp_param->osd[i].wd_addr) ||
               (pcurr_param->osd[i].fg_color!= ptemp_param->osd[i].fg_color)||
               (pcurr_param->osd[i].row_sp  != ptemp_param->osd[i].row_sp)  ||
               (pcurr_param->osd[i].zoom    != ptemp_param->osd[i].zoom)) {
                reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x08+(0x10*i)) : VCAP_CH_OSD_OFFSET(ch, 0x08+(0x10*i));
                reg_data.mask   = 0xf;
                if(PLAT_OSD_PIXEL_BITS == 4) {  ///< GM8210
                    reg_data.value  = (ptemp_param->osd[i].wd_addr)    |
                                      (ptemp_param->osd[i].row_sp<<16) |
                                      (ptemp_param->osd[i].zoom<<28);
                }
                else {
                    reg_data.value  = (ptemp_param->osd[i].wd_addr)      |
                                      (ptemp_param->osd[i].fg_color<<14) |
                                      (ptemp_param->osd[i].row_sp<<16)   |
                                      (ptemp_param->osd[i].zoom<<28);
                }
                ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
                if(ret < 0)
                    goto exit;

                param_sync |= (VCAP_PARAM_SYNC_OSD<<i);
            }

            /* Reg#1 */
            if((pcurr_param->osd[i].col_sp      != ptemp_param->osd[i].col_sp)       ||
               (pcurr_param->osd[i].bg_color    != ptemp_param->osd[i].bg_color)     ||
               (pcurr_param->osd[i].h_wd_num    != ptemp_param->osd[i].h_wd_num)     ||
               (pcurr_param->osd[i].v_wd_num    != ptemp_param->osd[i].v_wd_num)     ||
               (pcurr_param->osd[i].marquee_mode!= ptemp_param->osd[i].marquee_mode) ||
               (pcurr_param->osd[i].path        != ptemp_param->osd[i].path)) {
                reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x0c+(0x10*i)) : VCAP_CH_OSD_OFFSET(ch, 0x0c+(0x10*i));
                reg_data.mask   = 0xf;
                reg_data.value  = (ptemp_param->osd[i].col_sp)          |
                                  (ptemp_param->osd[i].bg_color<<12)    |
                                  (ptemp_param->osd[i].h_wd_num<<16)    |
                                  (ptemp_param->osd[i].marquee_mode<<22)|
                                  (ptemp_param->osd[i].v_wd_num<<24)    |
                                  (ptemp_param->osd[i].path<<30);

                ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
                if(ret < 0)
                    goto exit;

                param_sync |= (VCAP_PARAM_SYNC_OSD<<i);
            }

            /* Reg#2 */
            if((pcurr_param->osd[i].x_start            != ptemp_param->osd[i].x_start)            ||
               (pcurr_param->osd[i].x_end              != ptemp_param->osd[i].x_end)              ||
               (pcurr_param->osd[i].border_type        != ptemp_param->osd[i].border_type)        ||
               ((pcurr_param->osd[i].border_width&0x7) != (ptemp_param->osd[i].border_width&0x7)) ||
               (pcurr_param->osd[i].border_color       != ptemp_param->osd[i].border_color)) {
                reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x10+(0x10*i)) : VCAP_CH_OSD_OFFSET(ch, 0x10+(0x10*i));
                reg_data.mask   = 0xf;
                reg_data.value  = (ptemp_param->osd[i].x_start)                |
                                  ((ptemp_param->osd[i].border_width&0x7)<<12) |
                                  (ptemp_param->osd[i].border_type<<15)        |
                                  (ptemp_param->osd[i].x_end<<16)              |
                                  (ptemp_param->osd[i].border_color<<28);

                ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
                if(ret < 0)
                    goto exit;

                param_sync |= (VCAP_PARAM_SYNC_OSD<<i);
            }

            /* Reg#3 */
            if((pcurr_param->osd[i].y_start            != ptemp_param->osd[i].y_start)            ||
               (pcurr_param->osd[i].y_end              != ptemp_param->osd[i].y_end)              ||
               (pcurr_param->osd[i].font_alpha         != ptemp_param->osd[i].font_alpha)         ||
               ((pcurr_param->osd[i].border_width&0x8) != (ptemp_param->osd[i].border_width&0x8)) ||
               (pcurr_param->osd[i].bg_alpha           != ptemp_param->osd[i].bg_alpha)           ||
               (pcurr_param->osd[i].border_en          != ptemp_param->osd[i].border_en)) {
                reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x14+(0x10*i)) : VCAP_CH_OSD_OFFSET(ch, 0x14+(0x10*i));
                reg_data.mask   = 0xf;
                reg_data.value  = (ptemp_param->osd[i].y_start)                     |
                                  (ptemp_param->osd[i].font_alpha<<12)              |
                                  (((ptemp_param->osd[i].border_width>>3)&0x1)<<15) |
                                  (ptemp_param->osd[i].y_end<<16)                   |
                                  (ptemp_param->osd[i].bg_alpha<<28)                |
                                  (ptemp_param->osd[i].border_en<<31);

                ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
                if(ret < 0)
                    goto exit;

                param_sync |= (VCAP_PARAM_SYNC_OSD<<i);
            }
        }
    }

    /* Mark */
    for(i=0; i<VCAP_MARK_WIN_MAX; i++) {
        if((ptemp_param->comm.mark_en & (0x1<<i)) && (ptemp_param->mark[i].path == path)) {
            /* Reg#0 */
            if((pcurr_param->mark[i].x_start != ptemp_param->mark[i].x_start) ||
               (pcurr_param->mark[i].y_start != ptemp_param->mark[i].y_start) ||
               (pcurr_param->mark[i].x_dim   != ptemp_param->mark[i].x_dim)   ||
               (pcurr_param->mark[i].y_dim   != ptemp_param->mark[i].y_dim)) {
                reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x88+(0x08*i)) : VCAP_CH_OSD_OFFSET(ch, 0x88+(0x08*i));
                reg_data.mask   = 0xf;
                reg_data.value  = (ptemp_param->mark[i].x_start)     |
                                  (ptemp_param->mark[i].x_dim<<12)   |
                                  (ptemp_param->mark[i].y_start<<16) |
                                  (ptemp_param->mark[i].y_dim<<28);

                ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
                if(ret < 0)
                    goto exit;

                param_sync |= (VCAP_PARAM_SYNC_MARK<<i);
            }

            /* Reg#1 */
            if((pcurr_param->mark[i].alpha != ptemp_param->mark[i].alpha) ||
               (pcurr_param->mark[i].addr  != ptemp_param->mark[i].addr)  ||
               (pcurr_param->mark[i].zoom  != ptemp_param->mark[i].zoom)  ||
               (pcurr_param->mark[i].path  != ptemp_param->mark[i].path)) {
                reg_data.offset = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET(0x8c+(0x08*i)) : VCAP_CH_OSD_OFFSET(ch, 0x8c+(0x08*i));
                reg_data.mask   = 0xf;
                reg_data.value  = (ptemp_param->mark[i].alpha)    |
                                  (ptemp_param->mark[i].addr<<16) |
                                  (ptemp_param->mark[i].zoom<<28) |
                                  (ptemp_param->mark[i].path<<30);

                ret = vcap_lli_add_update_reg(pdev_info, ch, &reg_data);
                if(ret < 0)
                    goto exit;

                param_sync |= (VCAP_PARAM_SYNC_MARK<<i);
            }
        }
    }

    /* Update Param Sync Flag */
    pdev_info->ch[ch].param_sync = param_sync;

exit:
    if(ret < 0)
        vcap_lli_flush_update_reg(pdev_info, ch);

    return ret;
}

static inline int vcap_lli_wrap_dummy(struct vcap_dev_info_t *pdev_info, int ch, int path, int loop)
{
    int ret             = 0;
    u8  split           = 0;
    u8  grab_field_pair = 0;
    u8  frm2field       = 0;
    u8  p2i             = 0;
    u8  dma_block       = 1;    ///< dummy table must block DMA
    u8  dma_sel         = 0;
    int vi_ch           = SUBCH_TO_VICH(ch);
    u8  sd_bypass, sd_enable, tc_enable;
    struct vcap_lli_table_t *ll_table;
    struct vcap_vi_t  *pvi  = &pdev_info->vi[CH_TO_VI(ch)];
    struct vcap_ch_t  *pch  = &pdev_info->ch[ch];
    struct vcap_lli_t *plli = &pdev_info->lli;
#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
    int vi_ch0_subch = SUB_CH(CH_TO_VI(ch), 0);
    u8  sync_vi_ch0_sc_param = 0;
#endif

    /* Check normal table count */
    if(!plli->ch[ch].path[path].ll_pool.curr_idx) {
        ret = -1;
        goto exit;
    }

    /* Channel is split or not */
    if(pdev_info->split.ch == ch)
        split = 1;

    /* Channel Control */
    if(pvi->ch_param[vi_ch].prog == VCAP_VI_PROG_INTERLACE) {
        if((pvi->cap_style == VCAP_VI_CAP_STYLE_ANYONE)    ||
           (pvi->cap_style == VCAP_VI_CAP_STYLE_ODD_FIRST) ||
           (pvi->cap_style == VCAP_VI_CAP_STYLE_EVEN_FIRST)) {
            grab_field_pair = 1;    ///< grab top and bottom field at one fire
        }
    }

    /* SD */
    sd_enable = 1;
    if((pch->temp_param.sd[path].src_w != pch->temp_param.sd[path].out_w) ||
       (pch->temp_param.sd[path].src_h != pch->temp_param.sd[path].out_h))
       sd_bypass = 0;
    else
       sd_bypass = 1;

    /* TC */
    if((pch->temp_param.sd[path].out_w != pch->temp_param.tc[path].width) ||
       (pch->temp_param.sd[path].out_h != pch->temp_param.tc[path].height)||
       ((sd_bypass == 0) && (pch->temp_param.sd[path].vstep == 256) && (pch->temp_param.sd[path].hstep == 256))) ///< prevent scaler output size over range if scaling ratio too small
       tc_enable = 1;
    else
       tc_enable = 0;

    ////////////////////////// Add LL Command to Table //////////////////////////////////////
    /* Allocate Normal Table */
    ll_table = vcap_lli_alloc_normal_table(pdev_info, ch, path);
    if(!ll_table) {
        vcap_err("[%d] ch#%d-p%d LL allocate normal table failed\n", pdev_info->index, ch, path);
        ret = -1;
        goto exit;
    }

    if(loop)
        ll_table->type = VCAP_LLI_TAB_TYPE_DUMMY_LOOP;
    else
        ll_table->type = VCAP_LLI_TAB_TYPE_DUMMY;

    /* Status */
    VCAP_LLI_ADD_CMD_STATUS(ll_table, ch, path, split);

    /* Sub-Channel register 0x00 */
    VCAP_LLI_ADD_CMD_UPDATE(ll_table, ch, (0x1<<path),
                            (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x00) : VCAP_CH_SUBCH_OFFSET(ch, 0x00),
                            ((sd_bypass)                          |
                             (sd_enable<<1)                       |
                             (tc_enable<<2)                       |
                             (grab_field_pair<<3)                 |
                             (dma_sel<<4)                         |
                             (frm2field<<5)                       |
                             (p2i<<6)                             |
                             (dma_block<<7))<<(path*8));

#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
    /* VI_CH#0 SC x start and end position */
    if((ll_table->type == VCAP_LLI_TAB_TYPE_DUMMY) &&
       (pvi->tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) &&
       ((ch%VCAP_VI_CH_MAX) == 2) &&
       IS_DEV_CH_BUSY(pdev_info, vi_ch0_subch) &&
       ((pdev_info->ch[vi_ch0_subch].param.sc.x_start != pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start) ||
        (pdev_info->ch[vi_ch0_subch].param.sc.width   != pdev_info->ch[vi_ch0_subch].temp_param.sc.width))) {
        VCAP_LLI_ADD_CMD_UPDATE(ll_table,
                                ch,
                                0xf,
                                (vi_ch0_subch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x00) : VCAP_SC_OFFSET(vi_ch0_subch*0x8),
                                ((pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start) |
                                ((pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start + pdev_info->ch[vi_ch0_subch].temp_param.sc.width - 1)<<16) |
                                (pdev_info->ch[vi_ch0_subch].sc_rolling<<14)));
        sync_vi_ch0_sc_param++;
    }
#endif

    /* Link to list */
    ret = vcap_lli_link_list(pdev_info, ch, path, split, ll_table);
    if(ret < 0) {
        /* Free Table */
        vcap_lli_free_normal_table(pdev_info, ch, path, ll_table);
        ret = -1;
        goto exit;
    }

#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
    /* sync vi-ch#0 sc parameter */
    if(sync_vi_ch0_sc_param) {
        pdev_info->ch[vi_ch0_subch].param.sc.x_start = pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start;
        pdev_info->ch[vi_ch0_subch].param.sc.width   = pdev_info->ch[vi_ch0_subch].temp_param.sc.width;
    }
#endif

    /* update link-list table count, for debug monitor */
    pdev_info->ch[ch].path[path].ll_tab_cnt++;

exit:
    return ret;
}

static inline int vcap_lli_wrap_md_dummy(struct vcap_dev_info_t *pdev_info, int ch, int path, int md_enb)
{
    int ret             = 0;
    u8  split           = 0;
    u8  grab_field_pair = 0;
    u8  frm2field       = 0;
    u8  p2i             = 0;
    u8  dma_block       = 1;    ///< dummy table must block DMA
    u8  dma_sel         = 0;
    int vi_ch           = SUBCH_TO_VICH(ch);
    u8  sd_bypass, sd_enable, tc_enable;
    struct vcap_lli_table_t *ll_table;
    struct vcap_vi_t  *pvi  = &pdev_info->vi[CH_TO_VI(ch)];
    struct vcap_lli_t *plli = &pdev_info->lli;
    struct vcap_ch_param_t *pcurr_param = &pdev_info->ch[ch].param;
    struct vcap_ch_param_t *ptemp_param = &pdev_info->ch[ch].temp_param;
#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
    int vi_ch0_subch = SUB_CH(CH_TO_VI(ch), 0);
    u8  sync_vi_ch0_sc_param = 0;
#endif

    /* Check md buffer ready or not? */
    if(!pcurr_param->md.gau_addr || !pcurr_param->md.event_addr) {
        ret = -1;
        goto exit;
    }

    /* Check normal table count */
    if(!plli->ch[ch].path[path].ll_pool.curr_idx) {
        ret = -1;
        goto exit;
    }

    /* Check path parameter must be ready to prevent md region over image size */
    if(md_enb && ((pcurr_param->sd[path].src_w != pcurr_param->sc.width) || (pcurr_param->sd[path].src_h != pcurr_param->sc.height))) {
        ret = -1;
        goto exit;
    }

    /* Channel is split or not */
    if(pdev_info->split.ch == ch)
        split = 1;

    /* Channel Control */
    if(pvi->ch_param[vi_ch].prog == VCAP_VI_PROG_INTERLACE) {
        if((pvi->cap_style == VCAP_VI_CAP_STYLE_ANYONE)    ||
           (pvi->cap_style == VCAP_VI_CAP_STYLE_ODD_FIRST) ||
           (pvi->cap_style == VCAP_VI_CAP_STYLE_EVEN_FIRST)) {
            grab_field_pair = 1;    ///< grab top and bottom field at one fire
        }
    }

    /* SD */
    sd_enable = 1;
    if((pcurr_param->sd[path].src_w != pcurr_param->sd[path].out_w) ||
       (pcurr_param->sd[path].src_h != pcurr_param->sd[path].out_h))
       sd_bypass = 0;
    else
       sd_bypass = 1;

    /* TC */
    if((pcurr_param->sd[path].out_w != pcurr_param->tc[path].width) ||
       (pcurr_param->sd[path].out_h != pcurr_param->tc[path].height)||
       ((sd_bypass == 0) && (pcurr_param->sd[path].vstep == 256) && (pcurr_param->sd[path].hstep == 256))) ///< prevent scaler output size over range if scaling ratio too small
       tc_enable = 1;
    else
       tc_enable = 0;

    ////////////////////////// Add LL Command to Table //////////////////////////////////////
    /* Allocate Normal Table */
    ll_table = vcap_lli_alloc_normal_table(pdev_info, ch, path);
    if(!ll_table) {
        vcap_err("[%d] ch#%d-p%d LL allocate normal table failed\n", pdev_info->index, ch, path);
        ret = -1;
        goto exit;
    }

    ll_table->type = VCAP_LLI_TAB_TYPE_DUMMY;

    /* Status */
    VCAP_LLI_ADD_CMD_STATUS(ll_table, ch, path, split);

    /* Sub-Channel register 0x00 */
    VCAP_LLI_ADD_CMD_UPDATE(ll_table, ch, (0x1<<path),
                            (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x00) : VCAP_CH_SUBCH_OFFSET(ch, 0x00),
                            ((sd_bypass)          |
                             (sd_enable<<1)       |
                             (tc_enable<<2)       |
                             (grab_field_pair<<3) |
                             (dma_sel<<4)         |
                             (frm2field<<5)       |
                             (p2i<<6)             |
                             (dma_block<<7))<<(path*8));

    /* Sub-Channel register 0x08 */
    VCAP_LLI_ADD_CMD_UPDATE(ll_table, ch, 0x8,
                            (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08),
                            ((path<<24)                     |
                             (md_enb<<27)                   |
                             (pcurr_param->comm.h_flip<<28) |
                             (pcurr_param->comm.v_flip<<29)));

#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
    /* VI_CH#0 SC x start and end position */
    if((pvi->tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) &&
       ((ch%VCAP_VI_CH_MAX) == 2) &&
       IS_DEV_CH_BUSY(pdev_info, vi_ch0_subch) &&
       ((pdev_info->ch[vi_ch0_subch].param.sc.x_start != pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start) ||
        (pdev_info->ch[vi_ch0_subch].param.sc.width   != pdev_info->ch[vi_ch0_subch].temp_param.sc.width))) {
        VCAP_LLI_ADD_CMD_UPDATE(ll_table,
                                ch,
                                0xf,
                                (vi_ch0_subch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x00) : VCAP_SC_OFFSET(vi_ch0_subch*0x8),
                                ((pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start) |
                                ((pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start + pdev_info->ch[vi_ch0_subch].temp_param.sc.width - 1)<<16) |
                                (pdev_info->ch[vi_ch0_subch].sc_rolling<<14)));
        sync_vi_ch0_sc_param++;
    }
#endif

    /* Link to list */
    ret = vcap_lli_link_list(pdev_info, ch, path, split, ll_table);
    if(ret < 0) {
        /* Free Table */
        vcap_lli_free_normal_table(pdev_info, ch, path, ll_table);
        ret = -1;
        goto exit;
    }

    /* update parameter */
    pcurr_param->comm.md_src = ptemp_param->comm.md_src = path;
    pcurr_param->comm.md_en  = ptemp_param->comm.md_en  = md_enb;

#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
    /* sync vi-ch#0 sc parameter */
    if(sync_vi_ch0_sc_param) {
        pdev_info->ch[vi_ch0_subch].param.sc.x_start = pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start;
        pdev_info->ch[vi_ch0_subch].param.sc.width   = pdev_info->ch[vi_ch0_subch].temp_param.sc.width;
    }
#endif

    /* update link-list table count, for debug monitor */
    pdev_info->ch[ch].path[path].ll_tab_cnt++;

exit:
    return ret;
}

static inline int vcap_lli_wrap_hblank_command(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
    int i, j, num = 0, ret = 0;
    int table_cnt       = 0;
    u8  split           = 0;
    u8  sd_enable       = 1;
    u8  sd_bypass       = 1;
    u8  tc_enable       = 1;
    u8  dma_block       = 0;
    int cmd_idx         = 0;
    int reg_cnt         = 0;
    int sync_comm_param = 0;
    int sync_sc_param   = 0;
    int sync_sd_param   = 0;
    int sync_tc_param   = 0;
    int sc_x, sc_y, sc_w, sc_h, sd_w, sd_h, tc_x, tc_y, tc_w, tc_h;
    struct vcap_reg_data_t  reg_data[10];    ///< SUBCH => 3 registers, SC => 2 register ,SD => 2 registers, TC => 2 registers, VI_CH#0 SC => 1 register
    struct vcap_lli_table_t *ll_table, *update_table;
    struct vcap_ch_t  *pch  = &pdev_info->ch[ch];
    struct vcap_lli_t *plli = &pdev_info->lli;
    struct vcap_ch_param_t *pcurr_param = &pdev_info->ch[ch].param;
    struct vcap_ch_param_t *ptemp_param = &pdev_info->ch[ch].temp_param;
#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
    int vi_ch0_subch = SUB_CH(CH_TO_VI(ch), 0);
    u8  sync_vi_ch0_sc_param = 0;
#endif

    /* Check VI Grab Mode */
    if(pdev_info->vi[CH_TO_VI(ch)].grab_mode != VCAP_VI_GRAB_MODE_HBLANK) {
        if(pdev_info->dbg_mode && printk_ratelimit())
            vcap_err("[%d] VI#%d not in grab horizontal blanking mode\n", pdev_info->index, CH_TO_VI(ch));
        ret = -1;
        goto exit;
    }

    if(!pch->hblank_buf.vaddr) {
        ret = -1;
        goto exit;
    }

    /* Check normal table count */
    if(!plli->ch[ch].path[path].ll_pool.curr_idx) {
        ret = -1;
        goto exit;
    }

    /* SC width, Bypass */
    sc_x = 0;
    sc_y = 0;
    sc_w = ALIGN_4(pdev_info->vi[CH_TO_VI(ch)].src_w + VCAP_CH_HBLANK_DUMMY_PIXEL);
    sc_h = 1;

    /* SD width, Bypass */
    sd_w = sc_w;
    sd_h = sc_h;

    /* TC, for crop horizontal blanking data */
    tc_h = 1;
    tc_y = 0;
    tc_w = VCAP_CH_HBLANK_DUMMY_PIXEL;
    if((sd_w - tc_w) >= 0) {
        tc_x = ALIGN_4(sd_w - tc_w);
    }
    else {
        tc_x = 0;
        tc_w = sd_w;
    }

#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
    if((pdev_info->vi[CH_TO_VI(ch)].tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) &&
       ((ch%VCAP_VI_CH_MAX) == 0) &&
       IS_DEV_CH_BUSY(pdev_info, SUB_CH(CH_TO_VI(ch), 2)) &&
       (sc_x != pcurr_param->sc.x_start || sc_w != pcurr_param->sc.width)) {
        ptemp_param->sc.x_start = sc_x;
        ptemp_param->sc.width   = sc_w;
        goto exit;
    }
#endif

    /* SUBCH disable image border */
    if(pcurr_param->comm.border_en & (0x1<<path)) {
        reg_data[reg_cnt].offset  = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08);
        reg_data[reg_cnt].mask    = 0x2;
        reg_data[reg_cnt++].value = (pcurr_param->comm.mark_en<<8) | ((pcurr_param->comm.border_en & (~(0x1<<path)))<<12);
        sync_comm_param++;
    }

    /* SUBCH target scaler width and height, Bypass */
    if(sd_w != pcurr_param->sd[path].out_w || sd_h != pcurr_param->sd[path].out_h) {
        reg_data[reg_cnt].offset  = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x10+(path*0x4)) : VCAP_CH_SUBCH_OFFSET(ch, 0x10+(path*0x4));
        reg_data[reg_cnt].mask    = 0xf;
        reg_data[reg_cnt++].value = sd_w | (sd_h<<16);
        sync_sd_param++;
    }

    /* SUBCH resolution output width */
    if(tc_w != pcurr_param->tc[path].width) {
        reg_data[reg_cnt].offset  = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x20+((path/2)*0x4)) : VCAP_CH_SUBCH_OFFSET(ch, 0x20+((path/2)*0x4));
        reg_data[reg_cnt].mask    = 0x3<<((path%2)*2);
        reg_data[reg_cnt++].value = tc_w<<((path%2)*16);
        sync_tc_param++;
    }

    /* SC x start and end position */
    if(sc_x != pcurr_param->sc.x_start || sc_w != pcurr_param->sc.width) {
        reg_data[reg_cnt].offset  = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x00) : VCAP_SC_OFFSET(ch*0x8);
        reg_data[reg_cnt].mask    = 0xf;
        reg_data[reg_cnt++].value = (sc_x) | ((sc_x + sc_w - 1)<<16) | (pdev_info->ch[ch].sc_rolling<<14);
        sync_sc_param++;
    }

    /* SC y start and end position */
    if(sc_y != pcurr_param->sc.y_start || sc_h != pcurr_param->sc.height) {
        reg_data[reg_cnt].offset  = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x04) : VCAP_SC_OFFSET(0x04 + ch*0x8);
        reg_data[reg_cnt].mask    = 0xf;
        reg_data[reg_cnt++].value = (sc_y) | ((sc_y + sc_h - 1)<<16) | (pdev_info->ch[ch].sc_type<<14);
        sync_sc_param++;
    }

    /* Scaler H & V Step, Bypass */
    if(pcurr_param->sd[path].hstep != 0x100 || pcurr_param->sd[path].vstep != 0x100) {
        reg_data[reg_cnt].offset  = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0x8+(path*0x10)) : VCAP_CH_DNSD_OFFSET(ch, (0x8+(path*0x10)));
        reg_data[reg_cnt].mask    = 0xf;
        reg_data[reg_cnt++].value = 0x100 | (0x100<<16);
        sync_sd_param++;
    }

    /* Scaler top & bot offset, Bypass */
    if(pcurr_param->sd[path].topvoft != 0 || pcurr_param->sd[path].botvoft != 0) {
        reg_data[reg_cnt].offset  = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DNSD_OFFSET(0xc+(path*0x10)) : VCAP_CH_DNSD_OFFSET(ch, (0xc+(path*0x10)));
        reg_data[reg_cnt].mask    = 0xf;
        reg_data[reg_cnt++].value = (pcurr_param->sd[path].hsmo_str<<26) | (pcurr_param->sd[path].vsmo_str<<29);
        sync_sd_param++;
    }

    /* TC x start and end position */
    if(tc_x != pcurr_param->tc[path].x_start || tc_w != pcurr_param->tc[path].width) {
        reg_data[reg_cnt].offset  = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_TC_OFFSET(path*0x8) : VCAP_CH_TC_OFFSET(ch, path*0x8);
        reg_data[reg_cnt].mask    = 0xf;
        reg_data[reg_cnt++].value = (tc_x) | ((tc_x + tc_w - 1))<<16;
        sync_tc_param++;
    }

    /* TC y start and end position */
    if(tc_y != pcurr_param->tc[path].y_start || tc_h != pcurr_param->tc[path].height) {
        reg_data[reg_cnt].offset  = (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_TC_OFFSET(0x4+(path*0x8)) : VCAP_CH_TC_OFFSET(ch, 0x4+(path*0x8));
        reg_data[reg_cnt].mask    = 0xf;
        reg_data[reg_cnt++].value = (tc_y)                                     |
                                    ((pcurr_param->tc[path].border_w&0x7)<<13) |
                                    ((tc_y + tc_h - 1)<<16)                    |
                                    (((pcurr_param->tc[path].border_w>>3)&0x1)<<29);
        sync_tc_param++;
    }

#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
    /* VI_CH#0 SC x start and end position */
    if((pdev_info->vi[CH_TO_VI(ch)].tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) &&
       ((ch%VCAP_VI_CH_MAX) == 2) &&
       IS_DEV_CH_BUSY(pdev_info, vi_ch0_subch) &&
       ((pdev_info->ch[vi_ch0_subch].param.sc.x_start != pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start) ||
        (pdev_info->ch[vi_ch0_subch].param.sc.width   != pdev_info->ch[vi_ch0_subch].temp_param.sc.width))) {
        reg_data[reg_cnt].offset  = (vi_ch0_subch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x00) : VCAP_SC_OFFSET(vi_ch0_subch*0x8);
        reg_data[reg_cnt].mask    = 0xf;
        reg_data[reg_cnt++].value = (pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start) |
                                    ((pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start + pdev_info->ch[vi_ch0_subch].temp_param.sc.width - 1)<<16) |
                                    (pdev_info->ch[vi_ch0_subch].sc_rolling<<14);
        sync_vi_ch0_sc_param++;
    }
#endif

    /* Check update table count */
    if((reg_cnt%(VCAP_LLI_UPDATE_TAB_CMD_MAX-1)) == 0)
        table_cnt = (reg_cnt/(VCAP_LLI_UPDATE_TAB_CMD_MAX-1));
    else
        table_cnt = ((reg_cnt/(VCAP_LLI_UPDATE_TAB_CMD_MAX-1)) + 1);

    if(plli->update_pool.curr_idx < table_cnt) {
        if((pdev_info->dbg_mode >= 2) && printk_ratelimit()) {
            vcap_info("[%d] ch#%d-p%d LL update pool not enough, remain:%d require:%d\n",
                      pdev_info->index, ch, path, plli->update_pool.curr_idx, table_cnt);
        }
        pdev_info->diag.ch[ch].ll_tab_lack[path]++;
        ret = -1;
        goto exit;
    }

    /* Allocate Normal Table */
    ll_table = vcap_lli_alloc_normal_table(pdev_info, ch, path);
    if(!ll_table) {
        vcap_err("[%d] ch#%d-p%d LL allocate normal table failed\n", pdev_info->index, ch, path);
        ret = -1;
        goto exit;
    }

    ll_table->type = VCAP_LLI_TAB_TYPE_HBLANK;

    /* Status */
    VCAP_LLI_ADD_CMD_STATUS(ll_table, ch, path, split);

    /* DMA Address */
    VCAP_LLI_ADD_CMD_UPDATE(ll_table, ch, 0xf,
                            (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DMA_OFFSET((path*0x8)) : VCAP_CH_DMA_OFFSET(ch, (path*0x8)),
                            VCAP_DMA_ADDR(pch->hblank_buf.paddr));

    /* DMA Offset */
    VCAP_LLI_ADD_CMD_UPDATE(ll_table, ch, 0xf,
                            (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DMA_OFFSET(0x4+(path*0x8)) : VCAP_CH_DMA_OFFSET(ch, 0x4+(path*0x8)), 0);

    /* Sub-Channel register 0x00 */
    VCAP_LLI_ADD_CMD_UPDATE(ll_table, ch, (0x1<<path),
                            (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x00) : VCAP_CH_SUBCH_OFFSET(ch, 0x00),
                            ((sd_bypass) | (sd_enable<<1) | (tc_enable<<2) | (dma_block<<7))<<(path*8));

    /* Add and Link Update Reg Command */
    for(i=0; i<table_cnt; i++) {
        /* Allocate update table */
        update_table = vcap_lli_alloc_update_table(pdev_info);

        /* add update command */
        for(j=0; j<(VCAP_LLI_UPDATE_TAB_CMD_MAX-1); j++) {
            VCAP_LLI_ADD_CMD_UPDATE(update_table, ch, reg_data[cmd_idx].mask, reg_data[cmd_idx].offset, reg_data[cmd_idx].value);

            cmd_idx++;

            if(cmd_idx >= reg_cnt)
                break;
        }

        /* link update command table */
        vcap_lli_link_update_list(ch, path, split, ll_table, update_table);
    }

    /* Link to list */
    ret = vcap_lli_link_list(pdev_info, ch, path, split, ll_table);
    if(ret < 0) {
        /* Free Table */
        vcap_lli_free_normal_table(pdev_info, ch, path, ll_table);
        ret = -1;
        goto exit;
    }

    /* update count */
    num++;

    /* update link-list table count, for debug monitor */
    pdev_info->ch[ch].path[path].ll_tab_cnt++;

    /* Sync Parameter */
    if(sync_comm_param) {
        pcurr_param->comm.border_en = ptemp_param->comm.border_en = pcurr_param->comm.border_en & (~(0x1<<path));   ///< Disable image border
    }

    if(sync_sc_param) {
        pcurr_param->sc.x_start     = ptemp_param->sc.x_start     = sc_x;
        pcurr_param->sc.y_start     = ptemp_param->sc.y_start     = sc_y;
        pcurr_param->sc.width       = ptemp_param->sc.width       = sc_w;
        pcurr_param->sc.height      = ptemp_param->sc.height      = sc_h;
        pcurr_param->sd[path].src_w = ptemp_param->sd[path].src_w = sc_w;
        pcurr_param->sd[path].src_h = ptemp_param->sd[path].src_h = sc_h;
    }

    if(sync_sd_param) {
        pcurr_param->sd[path].out_w   = ptemp_param->sd[path].out_w   = sd_w;
        pcurr_param->sd[path].out_h   = ptemp_param->sd[path].out_h   = sd_h;
        pcurr_param->sd[path].hsize   = ptemp_param->sd[path].hsize   = sd_w;
        pcurr_param->sd[path].hstep   = ptemp_param->sd[path].hstep   = 0x100;  ///< Scaler Bypass
        pcurr_param->sd[path].vstep   = ptemp_param->sd[path].vstep   = 0x100;  ///< Scaler BYpass
        pcurr_param->sd[path].topvoft = ptemp_param->sd[path].topvoft = 0;      ///< Scaler Bypass
        pcurr_param->sd[path].botvoft = ptemp_param->sd[path].botvoft = 0;      ///< Scaler Bypass
    }

    if(sync_tc_param) {
        pcurr_param->tc[path].x_start = ptemp_param->tc[path].x_start = tc_x;
        pcurr_param->tc[path].y_start = ptemp_param->tc[path].y_start = tc_y;
        pcurr_param->tc[path].width   = ptemp_param->tc[path].width   = tc_w;
        pcurr_param->tc[path].height  = ptemp_param->tc[path].height  = tc_h;
    }

#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
    if(sync_vi_ch0_sc_param) {
        pdev_info->ch[vi_ch0_subch].param.sc.x_start = pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start;
        pdev_info->ch[vi_ch0_subch].param.sc.width   = pdev_info->ch[vi_ch0_subch].temp_param.sc.width;
    }
#endif

exit:
    if(ret < 0)
        return ret;

    /* update enable state */
    pdev_info->ch[ch].hblank_enb = 1;

    return num;
}

static inline int vcap_lli_wrap_command(struct vcap_dev_info_t *pdev_info, int ch, int path, struct vcap_vg_job_item_t *job)
{
    int i, j, num = 0, ret = 0;
    u8  split           = 0;
    int reg_cnt         = 0;
    int table_cnt       = 0;
    u8  field_swap      = 0;
    u8  grab_field_pair = 0;
    u8  frm2field       = 0;
    u8  p2i             = 0;
    u8  dma_block       = 0;
    u8  dma_sel         = 0;
    int vi_ch           = SUBCH_TO_VICH(ch);
    u8  sd_bypass, sd_enable, tc_enable;
    u32 dma_start_addr, dma_line_offset, dma_field_offset;
    struct vcap_lli_table_t *ll_table, *update_table;
    struct vcap_vg_job_param_t *job_param = &job->param;
    struct vcap_vi_t  *pvi  = &pdev_info->vi[CH_TO_VI(ch)];
    struct vcap_ch_t  *pch  = &pdev_info->ch[ch];
    struct vcap_lli_t *plli = &pdev_info->lli;
#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
    int vi_ch0_subch = SUB_CH(CH_TO_VI(ch), 0);
    u8  sync_vi_ch0_sc_param = 0;
#endif

    /* Check request normal table count */
    if(plli->ch[ch].path[path].ll_pool.curr_idx < job_param->grab_num) {
        ret = -1;
        goto exit;
    }

    /* Check update command and request table count */
    if(plli->ch[ch].update_reg.state == VCAP_UPDATE_PREPARE) {
        vcap_lli_update_reg_go(pdev_info, ch);

        reg_cnt = plli->ch[ch].update_reg.num;

        if((reg_cnt%(VCAP_LLI_UPDATE_TAB_CMD_MAX-1)) == 0)
            table_cnt = (reg_cnt/(VCAP_LLI_UPDATE_TAB_CMD_MAX-1));
        else
            table_cnt = ((reg_cnt/(VCAP_LLI_UPDATE_TAB_CMD_MAX-1)) + 1);

        if(plli->update_pool.curr_idx < table_cnt) {
            if((pdev_info->dbg_mode >= 2) && printk_ratelimit()) {
                vcap_info("[%d] ch#%d-p%d LL update pool not enough, remain:%d require:%d\n",
                          pdev_info->index, ch, path, plli->update_pool.curr_idx, table_cnt);
            }
            pdev_info->diag.ch[ch].ll_tab_lack[path]++;
            ret = -1;
            goto exit;
        }
    }

    /* Channel is split or not */
    if(pdev_info->split.ch == ch)
        split = 1;

    /* dma channel selection */
    if(!pdev_info->capability.bug_two_dma_ch && job_param->dma_ch)
        dma_sel = 1;

    /* Channel Control */
    if(pvi->ch_param[vi_ch].prog == VCAP_VI_PROG_INTERLACE) {
        if((pvi->cap_style == VCAP_VI_CAP_STYLE_ANYONE)    ||
           (pvi->cap_style == VCAP_VI_CAP_STYLE_ODD_FIRST) ||
           (pvi->cap_style == VCAP_VI_CAP_STYLE_EVEN_FIRST)) {
            grab_field_pair = 1;    ///< grab top and bottom field at one fire
        }

        if(pch->temp_param.comm.v_flip && (job_param->dst_fmt == TYPE_YUV422_FRAME) && grab_field_pair) {
            field_swap = 1;
        }
    }
    else {
        if(job_param->p2i && !split) {  ///< split channel not support P2I
            p2i = 1;
            grab_field_pair = 1;
        }
        else {
#ifdef VCAP_SPEED_2P_SUPPORT
            if(pvi->ch_param[vi_ch].speed == VCAP_VI_SPEED_P && job_param->dst_fmt == TYPE_YUV422_2FRAMES)
                frm2field = 1;
#else
            if(job_param->dst_fmt == TYPE_YUV422_2FRAMES)
                frm2field = 1;
#endif
        }
    }

    if(job_param->frm_2frm_field && (job_param->dst_fmt == TYPE_YUV422_FRAME))  ///< 1frame mode grab top and bottom field to different job buffer
        grab_field_pair = 0;

    sd_enable = 1;
    if((pch->temp_param.sd[path].src_w != pch->temp_param.sd[path].out_w) ||
       (pch->temp_param.sd[path].src_h != pch->temp_param.sd[path].out_h))
       sd_bypass = 0;
    else
       sd_bypass = 1;

    if((pch->temp_param.sd[path].out_w != pch->temp_param.tc[path].width) ||
       (pch->temp_param.sd[path].out_h != pch->temp_param.tc[path].height)||
       ((sd_bypass == 0) && (pch->temp_param.sd[path].vstep == 256) && (pch->temp_param.sd[path].hstep == 256))) ///< prevent scaler output size over range if scaling ratio too small
       tc_enable = 1;
    else
       tc_enable = 0;

    ////////////////////////// Add LL Command to Table //////////////////////////////////////
    for(num=0; num<job_param->grab_num; num++) {
        /* Allocate Normal Table */
        ll_table = vcap_lli_alloc_normal_table(pdev_info, ch, path);
        if(!ll_table) {
            vcap_err("[%d] ch#%d-p%d LL allocate normal table failed\n", pdev_info->index, ch, path);
            ret = -1;
            goto exit;
        }

        /* link job to table */
        ll_table->job = (void *)job;

        /* Status */
        VCAP_LLI_ADD_CMD_STATUS(ll_table, ch, path, split);

        /* DMA */
        if(split) {
            struct vcap_vg_job_item_t *curr_item, *next_item;

            list_for_each_entry_safe(curr_item, next_item, &job->m_job_head, m_job_list) {
                /* get start address and offset */
                vcap_dev_get_offset(pdev_info, ch, path, (void *)&curr_item->param, num, &dma_start_addr, &dma_line_offset, &dma_field_offset);

                /* DMA Address */
                VCAP_LLI_ADD_CMD_UPDATE(ll_table, ch, 0xf,
                                        VCAP_SPLIT_CH_OFFSET(curr_item->param.split_ch, 0x00+(path*8)),
                                        (dma_start_addr|field_swap));

                /* DMA Offset */
                VCAP_LLI_ADD_CMD_UPDATE(ll_table, ch, 0xf,
                                        VCAP_SPLIT_CH_OFFSET(curr_item->param.split_ch, 0x04+(path*8)),
                                        (dma_line_offset & 0x1fff) | ((dma_field_offset & 0x7ffff)<<13));
            }
        }
        else {
            /* get start address and offset */
            vcap_dev_get_offset(pdev_info, ch, path, (void *)job_param, num, &dma_start_addr, &dma_line_offset, &dma_field_offset);

            /* DMA Address */
            VCAP_LLI_ADD_CMD_UPDATE(ll_table, ch, 0xf,
                                    (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DMA_OFFSET((path*0x8)) : VCAP_CH_DMA_OFFSET(ch, (path*0x8)),
                                    (dma_start_addr|field_swap));

            /* DMA Offset */
            VCAP_LLI_ADD_CMD_UPDATE(ll_table, ch, 0xf,
                                    (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_DMA_OFFSET(0x4+(path*0x8)) : VCAP_CH_DMA_OFFSET(ch, 0x4+(path*0x8)),
                                    (dma_line_offset & 0x1fff) | ((dma_field_offset & 0x7ffff)<<13));
        }

        /* Sub-Channel register 0x00 */
        VCAP_LLI_ADD_CMD_UPDATE(ll_table, ch, (0x1<<path),
                                (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x00) : VCAP_CH_SUBCH_OFFSET(ch, 0x00),
                                ((sd_bypass)                          |
                                 (sd_enable<<1)                       |
                                 (tc_enable<<2)                       |
                                 (grab_field_pair<<3)                 |
                                 (dma_sel<<4)                         |
                                 (frm2field<<5)                       |
                                 (p2i<<6)                             |
                                 (dma_block<<7))<<(path*8));

        /* Prepare and Link Update Reg Command */
        if(reg_cnt && num == 0) {
            int cmd_idx = 0;

            for(i=0; i<table_cnt; i++) {
                /* Allocate update table */
                update_table = vcap_lli_alloc_update_table(pdev_info);

                /* add update command */
                for(j=0; j<(VCAP_LLI_UPDATE_TAB_CMD_MAX-1); j++) {
                    VCAP_LLI_ADD_CMD_UPDATE(update_table,
                                            ch,
                                            plli->ch[ch].update_reg.reg[cmd_idx].mask,
                                            plli->ch[ch].update_reg.reg[cmd_idx].offset,
                                            plli->ch[ch].update_reg.reg[cmd_idx].value);
                    cmd_idx++;

                    if(cmd_idx >= reg_cnt)
                        break;
                }

                /* link update command table */
                vcap_lli_link_update_list(ch, path, split, ll_table, update_table);
            }
        }

#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
        sync_vi_ch0_sc_param = 0;
        /* VI_CH#0 sc register update for 2ch dual edge hybrid mode */
        if((num == 0) &&
           (pvi->tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) &&
           ((ch%VCAP_VI_CH_MAX) == 2) &&
           IS_DEV_CH_BUSY(pdev_info, vi_ch0_subch) &&
           ((pdev_info->ch[vi_ch0_subch].param.sc.x_start != pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start) ||
            (pdev_info->ch[vi_ch0_subch].param.sc.width   != pdev_info->ch[vi_ch0_subch].temp_param.sc.width))) {
            update_table = vcap_lli_alloc_update_table(pdev_info);
            if(update_table) {
                VCAP_LLI_ADD_CMD_UPDATE(update_table,
                                        ch,
                                        0xf,
                                        (vi_ch0_subch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x00) : VCAP_SC_OFFSET(vi_ch0_subch*0x8),
                                        ((pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start) |
                                        ((pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start + pdev_info->ch[vi_ch0_subch].temp_param.sc.width - 1)<<16) |
                                        (pdev_info->ch[vi_ch0_subch].sc_rolling<<14)));

                /* link update command table */
                vcap_lli_link_update_list(ch, path, split, ll_table, update_table);

                sync_vi_ch0_sc_param++;
            }
        }
#endif

        /* Link to list */
        ret = vcap_lli_link_list(pdev_info, ch, path, split, ll_table);
        if(ret < 0) {
            /* Free Table */
            vcap_lli_free_normal_table(pdev_info, ch, path, ll_table);
            ret = -1;
            goto exit;
        }

#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
        /* sync vi-ch#0 sc parameter */
        if(sync_vi_ch0_sc_param) {
            pdev_info->ch[vi_ch0_subch].param.sc.x_start = pdev_info->ch[vi_ch0_subch].temp_param.sc.x_start;
            pdev_info->ch[vi_ch0_subch].param.sc.width   = pdev_info->ch[vi_ch0_subch].temp_param.sc.width;
        }
#endif

#ifdef VCAP_MD_GAU_VALUE_CHECK
        if(num == 0) {
            /* md x and y number change must to trigger md gaussian value check */
            if((pch->param.md.x_num != pch->temp_param.md.x_num) || (pch->param.md.y_num != pch->temp_param.md.y_num)) {
                pdev_info->ch[ch].do_md_gau_chk = 1;
            }
        }
#endif

        /* update link-list table count, for debug monitor */
        pdev_info->ch[ch].path[path].ll_tab_cnt++;
    }

    /*
     * P2I Miss Top Field Workaround
     * Only for Cascade and use path#3 to create dummy loop table.
     * The dummy loop table will be terminate when all path idle.
     */
    if(pdev_info->capability.bug_p2i_miss_top && p2i && (ch == VCAP_CASCADE_CH_NUM)) {
        if(list_empty(&plli->ch[ch].path[3].ll_head))
            vcap_lli_wrap_dummy(pdev_info, ch, 3, 1);   ///< Add Dummy Loop Table
    }

exit:
    vcap_lli_flush_update_reg(pdev_info, ch);
    if(ret < 0) {
        if(num)
            job_param->grab_num = num;  ///< update real grab number count
        else
            return ret;
    }

    return num;
}

static void vcap_lli_fatal_sw_reset(struct vcap_dev_info_t *pdev_info)
{
    int i, ch, path;
    u32 tmp, value_mask;
    unsigned long flags = 0;
    struct vcap_lli_t *plli = &pdev_info->lli;
    struct vcap_lli_table_t *curr_table, *next_table;
    struct vcap_lli_table_t *curr_up_table, *next_up_table;
    struct vcap_lli_cmd_update_t cmd_update;

    if(pdev_info->dbg_mode)
        vcap_err("[%d] Fatal Reset!\n", pdev_info->index);

    spin_lock_irqsave(&pdev_info->lock, flags);

    /* stop all channel and path, all job callback */
    for(ch=0; ch<VCAP_CHANNEL_MAX; ch++) {
        if(!pdev_info->ch[ch].active)
            continue;

#ifdef CFG_TIMEOUT_KERNEL_TIMER
        del_timer(&pdev_info->ch[ch].timer);
#else
        pdev_info->ch[ch].timer_data.count = 0;
#endif

        /* stop ll hardware engine */
        for(path=0; path<VCAP_SCALER_MAX; path++)
            vcap_lli_set_next_stop(pdev_info, ch, path, 1);

        /* disable channel */
        vcap_dev_ch_disable(pdev_info, ch);

        /* disable channel MD */
        tmp = VCAP_REG_RD(pdev_info, (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08));
        if(tmp & BIT27) {
            tmp &= (~BIT27);
            VCAP_REG_WR(pdev_info, (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08), tmp);
        }
        pdev_info->ch[ch].param.comm.md_en = pdev_info->ch[ch].temp_param.comm.md_en = 0;

        /* clear sub-channel control bit */
        if(ch == VCAP_CASCADE_CH_NUM)
            VCAP_REG_WR(pdev_info, VCAP_CAS_SUBCH_OFFSET(0), 0);
        else
            VCAP_REG_WR(pdev_info, VCAP_CH_SUBCH_OFFSET(ch, 0), 0);

        for(path=0; path<VCAP_SCALER_MAX; path++) {
            /* issue error notify */
            if(IS_DEV_CH_PATH_BUSY(pdev_info, ch, path))
                vcap_vg_notify(pdev_info->index, ch, path, VCAP_VG_NOTIFY_HW_DELAY);

            /* stop path and callback all job */
            if(pdev_info->stop)
                pdev_info->stop(pdev_info, ch, path);

            /* free Normal Table */
            list_for_each_entry_safe(curr_table, next_table, &plli->ch[ch].path[path].ll_head, list) {
                /* use cpu to update register to prevent update table not load if hardware timeout */
                list_for_each_entry_safe(curr_up_table, next_up_table, &curr_table->update_head, list) {
                    for(i=0; i<(VCAP_LLI_UPDATE_TAB_CMD_MAX-1); i++) {
                        memcpy(&cmd_update, (void *)(curr_up_table->addr+(i*8)), sizeof(struct vcap_lli_cmd_update_t));
                        if(cmd_update.desc == VCAP_LLI_DESC_UPDATE) {
                            tmp = VCAP_REG_RD(pdev_info, cmd_update.reg_offset);
                            value_mask = 0;
                            if(cmd_update.byte_en & 0x1)
                                value_mask |= 0xff;
                            if(cmd_update.byte_en & 0x2)
                                value_mask |= (0xff<<8);
                            if(cmd_update.byte_en & 0x4)
                                value_mask |= (0xff<<16);
                            if(cmd_update.byte_en & 0x8)
                                value_mask |= (0xff<<24);
                            tmp &= ~value_mask;
                            tmp |= (cmd_update.reg_value & value_mask);
                            if(((ch == VCAP_CASCADE_CH_NUM) && (cmd_update.reg_offset == VCAP_CAS_SUBCH_OFFSET(0x08))) ||
                               ((ch != VCAP_CASCADE_CH_NUM) && (cmd_update.reg_offset == VCAP_CH_SUBCH_OFFSET(ch, 0x08)))) {
                                if(tmp & BIT27)
                                    tmp &= (~BIT27);    ///< disable MD
                            }
                            VCAP_REG_WR(pdev_info, cmd_update.reg_offset, tmp);
                        }
                        else
                            break;  ///< go to next update command
                    }
                }

                if(curr_table->type == VCAP_LLI_TAB_TYPE_NORMAL)
                    vcap_vg_trigger_callback_fail((void *)curr_table->job);
                vcap_lli_free_normal_table(pdev_info, ch, path, curr_table);
            }

            /* free Null Table */
            list_for_each_entry_safe(curr_table, next_table, &plli->ch[ch].path[path].null_head, list) {
                vcap_lli_free_null_table(pdev_info, ch, path, curr_table);
            }
        }
    }

    /* capture software reset */
    vcap_dev_reset(pdev_info);

    /* LL init */
    vcap_lli_ll_init(pdev_info);

    /* enable capture */
    vcap_dev_capture_enable(pdev_info);

    /* Toogle Fire bit to let LL hardware enter standby state */
    vcap_dev_single_fire(pdev_info);

    pdev_info->dev_fatal    = 0;   ///< clear fatal error status
    pdev_info->fatal_time   = 0;
    pdev_info->dev_md_fatal = 0;   ///< clear md fatal flag, software reset will also trigger md reset
    pdev_info->do_md_reset  = 0;

    pdev_info->dma_ovf_thres[0] = pdev_info->dma_ovf_thres[1] = 0;

    pdev_info->diag.global.fatal_reset++;

    spin_unlock_irqrestore(&pdev_info->lock, flags);
}

static void vcap_lli_md_reset(struct vcap_dev_info_t *pdev_info)
{
    int i;
    u32 tmp;
    unsigned long flags;
    int md_enb_cnt = 0;

    /*
     * must disable all channel MD before trigger MD reset
     */

    spin_lock_irqsave(&pdev_info->lock, flags);

    /* check all channel have disable md */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(pdev_info->ch[i].active) {
            tmp = VCAP_REG_RD(pdev_info, (i == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(i, 0x08));
            if(tmp & BIT27) {
                md_enb_cnt++;
                break;
            }
        }
    }

    /* some channel md not disable, must wait all channel md disable */
    if(md_enb_cnt)
        goto exit;

    /* trigger md reset at next checking time */
    if(!pdev_info->do_md_reset) {
        pdev_info->do_md_reset++;
        goto exit;
    }

    tmp = VCAP_REG_RD(pdev_info, VCAP_GLOBAL_OFFSET(0x00));
    tmp |= BIT30;
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x00), tmp);

    udelay(3);

    tmp &= (~BIT30);
    VCAP_REG_WR(pdev_info, VCAP_GLOBAL_OFFSET(0x00), tmp);

    pdev_info->dev_md_fatal = 0;
    pdev_info->do_md_reset  = 0;
    pdev_info->diag.global.md_reset++;

    if(pdev_info->dbg_mode && printk_ratelimit())
        vcap_err("[%d] MD Reset!!\n", pdev_info->index);

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);
}

static int vcap_lli_dev_init(struct vcap_dev_info_t *pdev_info)
{
    if(GET_DEV_GST(pdev_info) == VCAP_DEV_STATUS_IDLE) {
        int i;
        u32 tmp;

        /* After software reset, the hardware global register will return to default, others are not. */
        vcap_dev_reset(pdev_info);

        /* register set to default */
        vcap_dev_set_reg_default(pdev_info);

        /* fcs init */
        vcap_fcs_init(pdev_info);

        /* denoise init */
        vcap_dn_init(pdev_info);

        /* presmooth init */
        vcap_presmo_init(pdev_info);

        /* sharpness init */
        vcap_sharp_init(pdev_info);

        /* md init */
        vcap_md_init(pdev_info);

        /* palette init */
        vcap_palette_init(pdev_info);

        /* RGB2YUV matrix init */
        vcap_dev_rgb2yuv_matrix_init(pdev_info);

        /* LL init */
        vcap_lli_ll_init(pdev_info);

        /* osd init */
        vcap_osd_init(pdev_info);

        /* mark init */
        vcap_mark_init(pdev_info);

        /* global register setup */
        vcap_dev_global_setup(pdev_info);

        /* sync timer init */
        vcap_dev_sync_timer_init(pdev_info);

        /* enable capture */
        vcap_dev_capture_enable(pdev_info);

        /* Toogle Fire bit to let LL hardware enter standby state */
        vcap_dev_single_fire(pdev_info);

        /* unmask sync timer interrupt status for global interrupt source */
        tmp = VCAP_REG_RD(pdev_info, VCAP_STATUS_OFFSET(0x2c));
        tmp &= (~BIT0);
        VCAP_REG_WR(pdev_info,  VCAP_STATUS_OFFSET(0x2c), tmp);

        /* unmask channel ll_done interrupt status */
        if((pdev_info->m_param)->ext_irq_src & VCAP_EXTRA_IRQ_SRC_LL_DONE) {
            tmp = 0xffffffff;
            for(i=0; i<VCAP_CHANNEL_MAX; i++) {
                if(pdev_info->ch[i].active) {
                    if(i == VCAP_CASCADE_CH_NUM)
                        VCAP_REG_WR(pdev_info,  VCAP_STATUS_OFFSET(0x34), ~BIT0);
                    else
                        tmp &= ~(0x1<<i);
                }
            }
            VCAP_REG_WR(pdev_info,  VCAP_STATUS_OFFSET(0x30), tmp);
        }

        /* update device global status */
        SET_DEV_GST(pdev_info, VCAP_DEV_STATUS_INITED);

#ifdef CFG_HEARTBEAT_TIMER
        /* trigger heartbeat timer */
        if(pdev_info->hb_timeout > 0)
            mod_timer(&pdev_info->hb_timer, jiffies+msecs_to_jiffies(pdev_info->hb_timeout));
#endif
    }

    return 0;
}

static inline void vcap_lli_force_stop(struct vcap_dev_info_t *pdev_info, int ch)
{
    int i, j;
    u32 tmp, value_mask;
    unsigned long flags = 0;
    struct vcap_lli_table_t *curr_table, *next_table, *null_table;
    struct vcap_lli_table_t *curr_up_table, *next_up_table;
    struct vcap_lli_cmd_update_t cmd_update;
    struct vcap_lli_t *plli = &pdev_info->lli;

    spin_lock_irqsave(&pdev_info->lock, flags);

    /* stop ll hardware engine */
    for(i=0; i<VCAP_SCALER_MAX; i++)
        vcap_lli_set_next_stop(pdev_info, ch, i, 1);

    /* disable channel */
    vcap_dev_ch_disable(pdev_info, ch);

    /* disable channel MD */
    tmp = VCAP_REG_RD(pdev_info, (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08));
    if(tmp & BIT27) {
        tmp &= (~BIT27);
        VCAP_REG_WR(pdev_info, (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08), tmp);
    }
    pdev_info->ch[ch].param.comm.md_en = pdev_info->ch[ch].temp_param.comm.md_en = 0;

    /* clear md gaussion value check flag */
    pdev_info->ch[ch].do_md_gau_chk = 0;

    /* clear sub-channel control bit */
    if(ch == VCAP_CASCADE_CH_NUM)
        VCAP_REG_WR(pdev_info, VCAP_CAS_SUBCH_OFFSET(0), 0);
    else
        VCAP_REG_WR(pdev_info, VCAP_CH_SUBCH_OFFSET(ch, 0), 0);

    for(i=0; i<VCAP_SCALER_MAX; i++) {
        /* stop path and callback all pending job */
        if(pdev_info->stop)
            pdev_info->stop(pdev_info, ch, i);

        /* free Normal Table (ongoing job callback fail) */
        list_for_each_entry_safe(curr_table, next_table, &plli->ch[ch].path[i].ll_head, list) {
            /* use cpu to update register to prevent update table not load if hardware timeout */
            list_for_each_entry_safe(curr_up_table, next_up_table, &curr_table->update_head, list) {
                for(j=0; j<(VCAP_LLI_UPDATE_TAB_CMD_MAX-1); j++) {
                    memcpy(&cmd_update, (void *)(curr_up_table->addr+(j*8)), sizeof(struct vcap_lli_cmd_update_t));
                    if(cmd_update.desc == VCAP_LLI_DESC_UPDATE) {
                        tmp = VCAP_REG_RD(pdev_info, cmd_update.reg_offset);
                        value_mask = 0;
                        if(cmd_update.byte_en & 0x1)
                            value_mask |= 0xff;
                        if(cmd_update.byte_en & 0x2)
                            value_mask |= (0xff<<8);
                        if(cmd_update.byte_en & 0x4)
                            value_mask |= (0xff<<16);
                        if(cmd_update.byte_en & 0x8)
                            value_mask |= (0xff<<24);
                        tmp &= ~value_mask;
                        tmp |= (cmd_update.reg_value & value_mask);
                        if(((ch == VCAP_CASCADE_CH_NUM) && (cmd_update.reg_offset == VCAP_CAS_SUBCH_OFFSET(0x08))) ||
                           ((ch != VCAP_CASCADE_CH_NUM) && (cmd_update.reg_offset == VCAP_CH_SUBCH_OFFSET(ch, 0x08)))) {
                            if(tmp & BIT27)
                                tmp &= (~BIT27);    ///< disable MD
                        }
                        VCAP_REG_WR(pdev_info, cmd_update.reg_offset, tmp);
                    }
                    else
                        break;  ///< go to next update command
                }
            }

            if(curr_table->type == VCAP_LLI_TAB_TYPE_NORMAL)
                vcap_vg_trigger_callback_fail((void *)curr_table->job);
            vcap_lli_free_normal_table(pdev_info, ch, i, curr_table);
        }

        /* free Null Table */
        list_for_each_entry_safe(curr_table, next_table, &plli->ch[ch].path[i].null_head, list) {
            vcap_lli_free_null_table(pdev_info, ch, i, curr_table);
        }

        /* Allocate Null Table */
        null_table = vcap_lli_alloc_null_table(pdev_info, ch, i);
        if(null_table) {
            /* Add NULL_CMD to Null Table */
            VCAP_LLI_ADD_CMD_NULL(null_table, ch, i);

            /* Add Null Table to list */
            list_add_tail(&null_table->list, &plli->ch[ch].path[i].null_head);

            /* set ll_next to null table addr */
            vcap_lli_set_ll_next(pdev_info, ch, i, null_table->addr);

            /* set ll_curr to null table addr */
            vcap_lli_set_ll_current(pdev_info, ch, i, null_table->addr);
        }

        /* After channel force stop the hardware path may keep previous error status, we can ignore it for the first ready job */
        if(pdev_info->dbg_mode < 2)
            pdev_info->ch[ch].path[i].skip_err_once = 1;
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);
}

static void vcap_lli_fatal_stop(struct vcap_dev_info_t *pdev_info)
{
    int ch;

    /* focrce stop all channel */
    for(ch=0; ch<VCAP_CHANNEL_MAX; ch++) {
        if(!pdev_info->ch[ch].active)
            continue;

#ifdef CFG_TIMEOUT_KERNEL_TIMER
        del_timer(&pdev_info->ch[ch].timer);
#else
        pdev_info->ch[ch].timer_data.count = 0;
#endif

        vcap_lli_force_stop(pdev_info, ch);
    }
}

static int vcap_lli_start(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
    int i, ret = 0;
    int vi_idx = CH_TO_VI(ch);
    int vi_ch  = SUBCH_TO_VICH(ch);
    unsigned long flags = 0;
    struct vcap_input_dev_t *pinput;

    spin_lock_irqsave(&pdev_info->lock, flags);

    /* check path status */
    if(IS_DEV_CH_PATH_BUSY(pdev_info, ch, path)) {
        vcap_err("[%d] ch%d-p%d not start from IDLE/STOP!\n", pdev_info->index, ch, path);
        ret = -1;
        goto end;
    }

    /* get input device information */
    pinput = vcap_input_get_info(vi_idx);
    if(!pinput) {
        vcap_err("[%d] vi#%d input device not exit!\n", pdev_info->index, vi_idx);
        ret = -1;
        goto end;
    }

    /* check input device switch or not */
    if(GET_DEV_VI_GST(pdev_info, vi_idx) == VCAP_DEV_STATUS_INITED && pinput->attached != vi_idx)
        SET_DEV_VI_GST(pdev_info, vi_idx, VCAP_DEV_STATUS_IDLE);    ///< input device switched need to reinit vi

    /* check VI init or not? */
    if(GET_DEV_VI_GST(pdev_info, vi_idx) == VCAP_DEV_STATUS_IDLE) {
        ret = vcap_dev_vi_setup(pdev_info, vi_idx);
        if(ret < 0)
            goto end;

        /* update vi global status */
        SET_DEV_VI_GST(pdev_info, vi_idx, VCAP_DEV_STATUS_INITED);

        /* input device attached to vi */
        pinput->attached = vi_idx;

        vcap_dev_param_set_default(pdev_info, vi_idx);
    }
    else {
        if(pdev_info->vi[vi_idx].input_norm != pinput->norm.mode) { ///< input device running mode switched
            if(!IS_DEV_VI_BUSY(pdev_info, vi_idx)) {
                ret = vcap_dev_vi_setup(pdev_info, vi_idx);
                if(ret < 0)
                    goto end;

                for(i=0; i<VCAP_VI_CH_MAX; i++) {
                    if(SUB_CH(vi_idx, i) >= VCAP_CHANNEL_MAX)
                        break;

                    if(!pdev_info->ch[SUB_CH(vi_idx, i)].active)
                        continue;

                    ret = vcap_dev_sc_setup(pdev_info, SUB_CH(vi_idx, i));
                    if(ret < 0)
                        goto end;
                }
            }
        }
        else if(pdev_info->vi[vi_idx].tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) {
            if(pdev_info->vi[vi_idx].ch_param[vi_ch].input_norm != pinput->ch_param[vi_ch].mode) { ///< input device channel running mode switched
                if(!IS_DEV_CH_BUSY(pdev_info, ch)) {
                    ret = vcap_dev_vi_ch_param_update(pdev_info, vi_idx, vi_ch);
                    if(ret < 0)
                        goto end;

                    ret = vcap_dev_sc_setup(pdev_info, ch);
                    if(ret < 0)
                        goto end;
                }
            }
        }
    }

    /* update use count of input device module */
    if(!pinput->used++) {
        if(pinput->module_get)
            pinput->module_get();
    }

    /* update path status */
    SET_DEV_CH_PATH_ST(pdev_info, ch, path, VCAP_DEV_STATUS_START);

end:
    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return ret;
}

static int vcap_lli_stop(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
    int i = VCAP_JOB_MAX;
    unsigned long flags = 0;
    struct vcap_lli_table_t *last_table, *last_n_table;
    struct vcap_path_t *ppath = &pdev_info->ch[ch].path[path];
    struct vcap_lli_t  *plli  = &pdev_info->lli;
    struct vcap_vg_job_item_t *job;
    struct vcap_input_dev_t *pinput;
#ifdef VCAP_EMERGENCY_STOP
    u8 tmp;
    struct vcap_lli_table_t *curr_table, *next_table;
#endif

    if(IS_DEV_CH_PATH_BUSY(pdev_info, ch, path)) {

        spin_lock_irqsave(&pdev_info->lock, flags);

        /* update path status */
        SET_DEV_CH_PATH_ST(pdev_info, ch, path, VCAP_DEV_STATUS_STOP);

        /* clear path current hszie */
        pdev_info->ch[ch].param.sd[path].hsize = pdev_info->ch[ch].temp_param.sd[path].hsize = 0;

        /* put job to callbackq */
        if(ppath->active_job) {
            vcap_vg_trigger_callback_fail((void *)ppath->active_job);
            ppath->active_job = NULL;
        }

        /* put job to callbackq */
        while(kfifo_len(ppath->job_q) && i--) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
            if(kfifo_out_locked(ppath->job_q, (void *)&job, sizeof(unsigned int), &ppath->job_q_lock) < sizeof(unsigned int))
                break;
#else
            if(kfifo_get(ppath->job_q, (void *)&job, sizeof(unsigned int)) < sizeof(unsigned int))
                break;
#endif
            vcap_vg_trigger_callback_fail((void *)job);
        }

#ifdef VCAP_EMERGENCY_STOP
        /*
         * Without break LLI, force to block DMA and put ongoing job to callbackq.
         * Job will be callback now, the LLI table will be free in ISR
         */
        if(!list_empty(&plli->ch[ch].path[path].ll_head)) {
            /* Enable DMA block bit in LLI command for each LLI Table */
            list_for_each_entry_safe(curr_table, next_table, &plli->ch[ch].path[path].ll_head, list) {
                if(curr_table->type == VCAP_LLI_TAB_TYPE_NORMAL || curr_table->type == VCAP_LLI_TAB_TYPE_HBLANK) {
                    *((volatile u32 *)(curr_table->addr+24)) = (*((volatile u32 *)(curr_table->addr+24))) | (BIT7<<(path*8));   ///< Enable DMA Block
                }
            }

            /* Use CPU to force enable DMA block bit */
            tmp = VCAP_REG_RD8(pdev_info, (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x00+path) : VCAP_CH_SUBCH_OFFSET(ch, (0x00+path)));
            if((tmp & BIT7) == 0) {
                tmp |= BIT7;
                VCAP_REG_WR8(pdev_info, (ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x00+path) : VCAP_CH_SUBCH_OFFSET(ch, (0x00+path)), tmp);
            }

            /* put job to callbackq */
            list_for_each_entry_safe(curr_table, next_table, &plli->ch[ch].path[path].ll_head, list) {
                if(curr_table->type == VCAP_LLI_TAB_TYPE_NORMAL) {
                    curr_table->type = VCAP_LLI_TAB_TYPE_DUMMY;     ///< change table type to dummy because job will be callback now
                    vcap_vg_trigger_callback_fail(curr_table->job);
                }
                else if(curr_table->type == VCAP_LLI_TAB_TYPE_HBLANK) {
                    curr_table->type = VCAP_LLI_TAB_TYPE_DUMMY;
                }
            }
        }
#endif

        /* check hardware link-list have dummy loop table, and terminate loop */
        if(!list_empty(&plli->ch[ch].path[path].ll_head)) {
            /* Get last Normal Table */
            last_table = list_entry(plli->ch[ch].path[path].ll_head.prev, typeof(*last_table), list);
            if(last_table->type == VCAP_LLI_TAB_TYPE_DUMMY_LOOP) {
                /* Get last Null Table */
                last_n_table = list_entry(plli->ch[ch].path[path].null_head.prev, typeof(*last_n_table), list);
                last_n_table->curr_entry--;
                VCAP_LLI_ADD_CMD_NULL(last_n_table, ch, path);
                last_table->type = VCAP_LLI_TAB_TYPE_DUMMY;     ///< change table type, the IRQ handler will free this table
            }
        }

#ifndef PLAT_SCALER_FRAME_RATE_CTRL
        ppath->frame_rate_cnt = ppath->frame_rate_check = 0;
#endif

        /* update use count of input device module */
        pinput = vcap_input_get_info(CH_TO_VI(ch));
        if(pinput && pinput->used > 0) {
            pinput->used--;
            if(!pinput->used) {
                if(pinput->module_put)
                    pinput->module_put();
            }
        }

        spin_unlock_irqrestore(&pdev_info->lock, flags);
    }

    return 0;
}

static inline int vcap_lli_framerate_check(struct vcap_dev_info_t *pdev_info, int ch, int path)
{
    int ret = 0;

    if(pdev_info->ch[ch].path[path].frame_rate_check) {
        if(pdev_info->ch[ch].path[path].frame_rate_thres == 0)  ///< thres=0 means no frame rate control
            pdev_info->ch[ch].path[path].frame_rate_check = 0;
        else {
            ret = -1;
            if(pdev_info->ch[ch].path[path].frame_rate_cnt >= pdev_info->ch[ch].path[path].frame_rate_thres) {
                pdev_info->ch[ch].path[path].frame_rate_cnt -= pdev_info->ch[ch].path[path].frame_rate_thres;
                ret = 0;
                pdev_info->ch[ch].path[path].frame_rate_check = 0;
            }
        }
    }

    return ret;
}


static inline int vcap_lli_sc_check_ready(struct vcap_dev_info_t *pdev_info, int ch)
{
    if(ch >= VCAP_CHANNEL_MAX)
        return 0;

    if((pdev_info->ch[ch].temp_param.sc.x_start != pdev_info->ch[ch].param.sc.x_start) ||
       (pdev_info->ch[ch].temp_param.sc.width   != pdev_info->ch[ch].param.sc.width)) {
        return 0;
    }
    else {
        u32 tmp;
        int sc_x, sc_w;

        tmp  = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x00) : VCAP_SC_OFFSET(0x00 + ch*0x8)));
        sc_x = tmp & 0x1fff;
        sc_w = ((tmp >>16) & 0x1fff) + 1 - sc_x;

        /* check register have update to new setting */
        if((sc_x != pdev_info->ch[ch].temp_param.sc.x_start) || (sc_w != pdev_info->ch[ch].temp_param.sc.width))
            return 0;
    }

    return 1;
}

static inline void vcap_lli_add_framecnt(struct vcap_dev_info_t *pdev_info, int ch, int path, int do_check)
{
    pdev_info->ch[ch].path[path].frame_rate_cnt += VCAP_FRAME_RATE_UNIT;
    if(do_check)
        pdev_info->ch[ch].path[path].frame_rate_check++;
}

#ifdef PLAT_MD_GROUPING
static inline void vcap_lli_md_to_next_group(struct vcap_dev_info_t *pdev_info)
{
    int i;
    int md_cnt        = 0;
    int md_grp_ch_cnt = 0;  ///< number of channel in md group
    int switch_grp    = 0;

    /* check all active member have enable md in current group */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(!pdev_info->ch[i].active)
            continue;

        if(pdev_info->ch[i].md_grp_id != pdev_info->md_enb_grp)
            continue;
        else
            md_grp_ch_cnt++;

        if((!pdev_info->ch[i].md_active) ||
           (pdev_info->vi_probe.mode && (pdev_info->vi_probe.vi_idx == CH_TO_VI(i))) ||
           (pdev_info->ch[i].md_active && (pdev_info->ch[i].param.comm.md_en || (!IS_DEV_CH_PATH_BUSY(pdev_info, i, pdev_info->ch[i].param.comm.md_src))))) {
            md_cnt++;
        }

        /* run to next md group if all member have enable md in current group */
        if(md_cnt == VCAP_MD_GROUP_CH_MAX) {
            switch_grp = 1;
            break;
        }
    }

    if(switch_grp || (md_cnt == md_grp_ch_cnt))
        pdev_info->md_enb_grp = (pdev_info->md_enb_grp + 1)%pdev_info->md_grp_max;
}
#endif

static inline void vcap_lli_update_input_dev_param(struct vcap_dev_info_t *pdev_info)
{
    int vi, ch, sub_ch, ret;
    u32 tmp;
    u32 ch_timeout_ms;
    struct vcap_input_dev_t *pinput;

    /* check input device parameter for dynamic switch frame_rate/timeout/speed/interface */
    for(vi=0; vi<VCAP_VI_MAX; vi++) {
        if(GET_DEV_VI_GST(pdev_info, vi) == VCAP_DEV_STATUS_IDLE)
            continue;

        pinput = vcap_input_get_info(vi);
        if(!pinput) {
            if(pdev_info->vi[vi].grab_mode == VCAP_VI_GRAB_MODE_HBLANK) {
                pdev_info->vi[vi].grab_hblank = 0;  ///< force stop grab horizontal blanking
                goto vi_grab_chk;
            }
            else
                continue;
        }

        if(pdev_info->vi[vi].input_norm != pinput->norm.mode) { ///< input device running mode switched
            if(IS_DEV_VI_BUSY(pdev_info, vi)) {
                /* force stop all vi channel before switch VI config */
                for(ch=0; ch<VCAP_VI_CH_MAX; ch++) {
                    if(SUB_CH(vi, ch) >= VCAP_CHANNEL_MAX)
                        break;
                    if(pdev_info->ch[SUB_CH(vi, ch)].active) {
#ifdef CFG_TIMEOUT_KERNEL_TIMER
                        del_timer(&pdev_info->ch[SUB_CH(vi, ch)].timer);
#else
                        pdev_info->ch[SUB_CH(vi, ch)].timer_data.count = 0;
#endif
                        vcap_lli_force_stop(pdev_info, SUB_CH(vi, ch));
                    }
                }
            }

            /* VI setup */
            ret = vcap_dev_vi_setup(pdev_info, vi);
            if(ret < 0)
                goto update_misc;

            /* SC setup for all VI channel */
            for(ch=0; ch<VCAP_VI_CH_MAX; ch++) {
                if(SUB_CH(vi, ch) >= VCAP_CHANNEL_MAX)
                    break;
                if(!pdev_info->ch[SUB_CH(vi, ch)].active)
                    continue;
                ret = vcap_dev_sc_setup(pdev_info, SUB_CH(vi, ch));
                if(ret < 0)
                    goto update_misc;
            }
        }
        else if(pdev_info->vi[vi].tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) {
            /* Hybrid, need to check each channel parameter */
            for(ch=0; ch<VCAP_VI_CH_MAX; ch++) {
                if(SUB_CH(vi, ch) >= VCAP_CHANNEL_MAX)
                    break;

                if(!pdev_info->ch[SUB_CH(vi, ch)].active)
                    continue;

                if(pdev_info->vi[vi].ch_param[ch].input_norm != pinput->ch_param[ch].mode) { ///< input device channel running mode switched
                    /* force stop channel before switch channel config */
                    if(IS_DEV_CH_BUSY(pdev_info, SUB_CH(vi, ch))) {
#ifdef CFG_TIMEOUT_KERNEL_TIMER
                        del_timer(&pdev_info->ch[SUB_CH(vi, ch)].timer);
#else
                        pdev_info->ch[SUB_CH(vi, ch)].timer_data.count = 0;
#endif
                        vcap_lli_force_stop(pdev_info, SUB_CH(vi, ch));
                    }

                    /* VI channel parameter update */
                    ret = vcap_dev_vi_ch_param_update(pdev_info, vi, ch);
                    if(ret < 0)
                        continue;

                    /* SC setup for this channel */
                    ret = vcap_dev_sc_setup(pdev_info, SUB_CH(vi, ch));
                    if(ret < 0)
                        continue;
                }
            }

#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
            /* vi-ch#0 sc switch must wait vi-ch#2 in disable state for 2ch dual edge hybrid mode */
            sub_ch = SUB_CH(vi, 0);
            if(IS_DEV_CH_BUSY(pdev_info, sub_ch)           &&
               !vcap_lli_sc_check_ready(pdev_info, sub_ch) &&
               !VCAP_IS_CH_ENABLE(pdev_info, sub_ch)       &&
               !VCAP_IS_CH_ENABLE(pdev_info, SUB_CH(vi, 2))) {
                tmp = (pdev_info->ch[sub_ch].temp_param.sc.x_start) |
                      ((pdev_info->ch[sub_ch].temp_param.sc.x_start + pdev_info->ch[sub_ch].temp_param.sc.width - 1)<<16) |
                      (pdev_info->ch[sub_ch].sc_rolling<<14);
                VCAP_REG_WR(pdev_info, ((sub_ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SC_OFFSET(0x00) : VCAP_SC_OFFSET(0x00 + sub_ch*0x8)), tmp);

                /* sync sc parameter */
                pdev_info->ch[sub_ch].param.sc.x_start = pdev_info->ch[sub_ch].temp_param.sc.x_start;
                pdev_info->ch[sub_ch].param.sc.width   = pdev_info->ch[sub_ch].temp_param.sc.width;
            }
#endif
        }

update_misc:
        /* update vi data range */
        if(pinput->data_range != pdev_info->vi[vi].data_range) {
            tmp = VCAP_REG_RD(pdev_info, ((vi == VCAP_CASCADE_VI_NUM) ? VCAP_CAS_VI_OFFSET(0x00) : VCAP_VI_OFFSET(0x8*vi)));
            if(pinput->data_range == VCAP_VI_DATA_RANGE_240)
                tmp |= BIT7;
            else
                tmp &= ~BIT7;
            VCAP_REG_WR(pdev_info, ((vi == VCAP_CASCADE_VI_NUM) ? VCAP_CAS_VI_OFFSET(0x00) : VCAP_VI_OFFSET(0x8*vi)), tmp);
            pdev_info->vi[vi].data_range = pinput->data_range;
        }

        /* update vi source frame rate */
        if((pinput->frame_rate > 0) && (pdev_info->vi[vi].frame_rate != pinput->frame_rate))
            pdev_info->vi[vi].frame_rate = pinput->frame_rate;

        /* update vi source speed */
        if(pdev_info->vi[vi].speed != pinput->speed)
            pdev_info->vi[vi].speed = pinput->speed;

        /* update vi channel parameter */
        for(ch=0; ch<VCAP_VI_CH_MAX; ch++) {
            sub_ch = SUB_CH(vi, ch);
            if(sub_ch >= VCAP_CHANNEL_MAX)
                break;

            if(!pdev_info->ch[sub_ch].active)
                continue;

            /* vi channel frame rate and speed */
            if(pdev_info->vi[vi].tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) {
                if((pinput->ch_param[ch].frame_rate > 0) && (pdev_info->vi[vi].ch_param[ch].frame_rate != pinput->ch_param[ch].frame_rate))
                    pdev_info->vi[vi].ch_param[ch].frame_rate = pinput->ch_param[ch].frame_rate;

                if(pdev_info->vi[vi].ch_param[ch].speed != pinput->ch_param[ch].speed)
                    pdev_info->vi[vi].ch_param[ch].speed = pinput->ch_param[ch].speed;

                ch_timeout_ms = pinput->ch_param[ch].timeout_ms;
            }
            else {
                /* non-hybrid, all channel parameter follow vi setting */
                if(pdev_info->vi[vi].ch_param[ch].frame_rate != pdev_info->vi[vi].frame_rate)
                    pdev_info->vi[vi].ch_param[ch].frame_rate = pdev_info->vi[vi].frame_rate;

                if(pdev_info->vi[vi].ch_param[ch].speed != pdev_info->vi[vi].speed)
                    pdev_info->vi[vi].ch_param[ch].speed = pdev_info->vi[vi].speed;

                ch_timeout_ms = pinput->timeout_ms;
            }

            /* channel timeout second */
            if(ch_timeout_ms > 0) {
#ifdef CFG_TIMEOUT_KERNEL_TIMER
                pdev_info->ch[sub_ch].timer_data.timeout = msecs_to_jiffies(ch_timeout_ms);
#else
                pdev_info->ch[sub_ch].timer_data.timeout = ch_timeout_ms;
#endif
            }
            else {
#ifdef CFG_TIMEOUT_KERNEL_TIMER
                pdev_info->ch[sub_ch].timer_data.timeout = msecs_to_jiffies(VCAP_DEF_CH_TIMEOUT);
#else
                pdev_info->ch[sub_ch].timer_data.timeout = VCAP_DEF_CH_TIMEOUT;
#endif
            }
        }

vi_grab_chk:
        /* VI grab mode check */
        if(((pdev_info->vi[vi].grab_hblank != 0) && (pdev_info->vi[vi].grab_mode == VCAP_VI_GRAB_MODE_NORMAL)) ||
           ((pdev_info->vi[vi].grab_hblank == 0) && (pdev_info->vi[vi].grab_mode == VCAP_VI_GRAB_MODE_HBLANK))) {
            if(IS_DEV_VI_BUSY(pdev_info, vi)) {
                /* force stop all vi channel before switch VI config */
                for(ch=0; ch<VCAP_VI_CH_MAX; ch++) {
                    if(SUB_CH(vi, ch) >= VCAP_CHANNEL_MAX)
                        break;
                    if(pdev_info->ch[SUB_CH(vi, ch)].active) {
#ifdef CFG_TIMEOUT_KERNEL_TIMER
                        del_timer(&pdev_info->ch[SUB_CH(vi, ch)].timer);
#else
                        pdev_info->ch[SUB_CH(vi, ch)].timer_data.count = 0;
#endif
                        vcap_lli_force_stop(pdev_info, SUB_CH(vi, ch));
                    }
                }
            }

            /* VI grab horizontal blanking setup */
            if(pdev_info->vi[vi].grab_hblank) {
                vcap_dev_vi_grab_hblank_setup(pdev_info, vi, 1);
            }
            else {
                vcap_dev_vi_grab_hblank_setup(pdev_info, vi, 0);

                /* SC setup for all VI channel */
                for(ch=0; ch<VCAP_VI_CH_MAX; ch++) {
                    sub_ch = SUB_CH(vi, ch);
                    if(sub_ch >= VCAP_CHANNEL_MAX)
                        break;
                    if(!pdev_info->ch[sub_ch].active)
                        continue;

                    pdev_info->ch[sub_ch].hblank_enb = 0;

                    vcap_dev_sc_setup(pdev_info, sub_ch);
                }
            }
        }
        else if(pdev_info->vi[vi].grab_mode == VCAP_VI_GRAB_MODE_HBLANK) {
            for(ch=0; ch<VCAP_VI_CH_MAX; ch++) {
                sub_ch = SUB_CH(vi, ch);
                if(sub_ch >= VCAP_CHANNEL_MAX)
                    break;
                if(!pdev_info->ch[sub_ch].active)
                    continue;
                if(pdev_info->ch[sub_ch].hblank_enb && ((pdev_info->vi[vi].grab_hblank & (0x1<<ch)) == 0)) {
#ifdef CFG_TIMEOUT_KERNEL_TIMER
                    del_timer(&pdev_info->ch[SUB_CH(vi, ch)].timer);
#else
                    pdev_info->ch[SUB_CH(vi, ch)].timer_data.count = 0;
#endif
                    vcap_lli_force_stop(pdev_info, sub_ch);

                    pdev_info->ch[sub_ch].hblank_enb = 0;

                    vcap_dev_sc_setup(pdev_info, sub_ch);
                }
            }
        }
    }
}

static inline void vcap_lli_update_comm_path(struct vcap_dev_info_t *pdev_info, int ch)
{
    int i;
    int comm_path = VCAP_SCALER_MAX;

    /* comm_path is for common register update path, lookup the most LLI table path */
    for(i=0; i<VCAP_SCALER_MAX; i++) {
        if(!IS_DEV_CH_PATH_BUSY(pdev_info, ch, i))
            continue;
        if(comm_path == VCAP_SCALER_MAX)
            comm_path = i;
        else {
            if(pdev_info->lli.ch[ch].path[i].ll_pool.curr_idx < pdev_info->lli.ch[ch].path[comm_path].ll_pool.curr_idx)
                comm_path = i;
        }
    }
    if((comm_path != VCAP_SCALER_MAX) && (pdev_info->ch[ch].comm_path != comm_path)) {
        pdev_info->ch[ch].comm_path = comm_path;
    }
}

static int vcap_lli_trigger(struct vcap_dev_info_t *pdev_info)
{
    int ret = 0;
    int sc_ready = 1;
    int fcnt_ch_tmp = -1;
    int fcnt_ch_curr;
    int i, j, k, num;
    int vi_probe, ch_grab_hblank, job_drop;
    int add_cnt, ch_jobs, pending_cnt;
    unsigned long flags;
    struct vcap_path_t *ppath_info;
    struct vcap_lli_t *plli = &pdev_info->lli;
    struct vcap_vg_job_item_t *job;
#ifdef CFG_TIMEOUT_KERNEL_TIMER
    u32 timeout_dummy = 0;
#endif

    if(IS_DEV_BUSY(pdev_info)) {
        spin_lock_irqsave(&pdev_info->lock, flags);

#ifdef CFG_TIMEOUT_KERNEL_TIMER
        /* caculate channel timeout dummy ms delay time, the first job should plus dummy delay time */
        if((pdev_info->m_param)->sync_time_div)
            timeout_dummy = msecs_to_jiffies(((1000/((pdev_info->m_param)->sync_time_div))*2));
#endif

        /* check and update input device parameter */
        vcap_lli_update_input_dev_param(pdev_info);

        /* get hardware frame count monitor channel */
        fcnt_ch_curr = vcap_dev_get_frame_cnt_monitor_ch(pdev_info);

        for(i=0; i<VCAP_CHANNEL_MAX; i++) {
            if(!pdev_info->ch[i].active)
                continue;

#ifdef VCAP_SC_VICH0_UPDATE_BY_VICH2
            /* check sc register ready in 2ch dual edge hybrid mode */
            if((pdev_info->vi[CH_TO_VI(i)].tdm_mode == VCAP_VI_TDM_MODE_2CH_BYTE_INTERLEAVE_HYBRID) && ((i%VCAP_VI_CH_MAX) == 0))
                sc_ready = vcap_lli_sc_check_ready(pdev_info, i);
            else
                sc_ready = 1;
#endif

            /* check vi in probe mode or not ? */
            if(pdev_info->vi_probe.mode && (pdev_info->vi_probe.vi_idx == CH_TO_VI(i))) {
                vi_probe       = 1;
                ch_grab_hblank = 0;
            }
            else {
                vi_probe = 0;
                if((pdev_info->vi[CH_TO_VI(i)].grab_mode == VCAP_VI_GRAB_MODE_HBLANK) &&
                   (pdev_info->vi[CH_TO_VI(i)].grab_hblank & (0x1<<SUBCH_TO_VICH(i))) && sc_ready) {
                    ch_grab_hblank = 1;
                }
                else
                    ch_grab_hblank = 0;
            }

            /* job force drop check */
            if(vi_probe || ch_grab_hblank || !sc_ready)
                job_drop = 1;
            else
                job_drop = 0;

            /* update common path for common register update */
            vcap_lli_update_comm_path(pdev_info, i);

            ch_jobs = 0;
            for(j=0; j<VCAP_SCALER_MAX; j++) {
                /* split channel check */
                if((pdev_info->split.ch == i) && (j >= VCAP_SPLIT_CH_SCALER_MAX))
                    continue;

                if(IS_DEV_CH_PATH_BUSY(pdev_info, i, j)) {
                    ppath_info  = &pdev_info->ch[i].path[j];
                    add_cnt     = 0;
                    pending_cnt = 0;

                    for(k=0; k<2; k++) {
                        if(plli->ch[i].path[j].ll_pool.curr_idx == 0)   ///< no free LLI normal table
                            break;

#ifndef PLAT_SCALER_FRAME_RATE_CTRL
                        if((vcap_lli_framerate_check(pdev_info, i, j) < 0) && (!job_drop)) {
                            /* add dummy job if need to drop frame for frame rate control */
                            if((pdev_info->ch[i].md_active) && (pdev_info->ch[i].param.comm.md_src == j)) {
#ifdef PLAT_MD_GROUPING
                                if((!pdev_info->dev_md_fatal) && ((pdev_info->ch[i].md_grp_id == pdev_info->md_enb_grp) || (pdev_info->ch[i].md_grp_id < 0)))
                                    ret = vcap_lli_wrap_md_dummy(pdev_info, i, j, 1);
                                else
                                    ret = vcap_lli_wrap_md_dummy(pdev_info, i, j, 0);
#else
                                if(!pdev_info->dev_md_fatal)
                                    ret = vcap_lli_wrap_md_dummy(pdev_info, i, j, 1);
                                else
                                    ret = vcap_lli_wrap_md_dummy(pdev_info, i, j, 0);
#endif
                            }
                            else {
                                ret = vcap_lli_wrap_dummy(pdev_info, i, j, 0);
                            }
                            if(ret < 0)
                                break;

                            add_cnt++;
                            vcap_lli_add_framecnt(pdev_info, i, j, 1);

                            continue;
                        }
#endif

                        if(ppath_info->active_job || kfifo_len(ppath_info->job_q)) {
                            if(!ppath_info->active_job)
                                ppath_info->active_job = vcap_dev_getjob(pdev_info, i, j);

                            job = (struct vcap_vg_job_item_t *)ppath_info->active_job;

                            /* force to drop job */
                            if(job_drop) {
                                vcap_vg_trigger_callback_fail((void *)job);
                                ppath_info->active_job = NULL;
                                continue;
                            }

                            /* job pending for waiting old version job finish */
                            if(job->status == VCAP_VG_JOB_STATUS_KEEP) {
                                pending_cnt++;
                                break;
                            }

                            /* check job status */
                            if(job->status != VCAP_VG_JOB_STATUS_STANDBY) {
                                vcap_err("[%d] ch#%d-p%d job#%d status unexpected!\n", pdev_info->index, i, j, job->job_id);
                                vcap_vg_trigger_callback_fail((void *)job);
                                ppath_info->active_job = NULL;
                                continue;
                            }

                            /* check and setup job input & output parameter */
                            ret = vcap_dev_param_setup(pdev_info, i, j, (void *)&job->param);
                            if(ret < 0) {
                                vcap_vg_trigger_callback_fail((void *)job);
                                ppath_info->active_job = NULL;
                                continue;
                            }

                            /* prepare update reg */
                            ret = vcap_lli_prepare_update_reg(pdev_info, i, j);
                            if(ret < 0) {
                                vcap_vg_trigger_callback_fail((void *)job);
                                ppath_info->active_job = NULL;
                                continue;
                            }

                            /* wrap update command */
                            num = vcap_lli_wrap_command(pdev_info, i, j, job);
                            if(num <= 0)
                                break;  ///< goto next path

                            /* job start */
                            vcap_vg_mark_engine_start(job);

                            /* Sync temp param to current param */
                            vcap_dev_param_sync(pdev_info, i, j);

                            ppath_info->active_job = NULL;
                            add_cnt += num;

                            /* grab one field mode, do field rate check */
                            if(job->param.frm_2frm_field) {
                                if(!ppath_info->grab_one_field) {
                                    ppath_info->grab_one_field = 1;
                                    ppath_info->grab_field_cnt = 0;
                                }

                                ppath_info->grab_field_cnt++;

                                if((ppath_info->grab_field_cnt%2) != 0)
                                    continue;   ///< 2 field add one frame count for frame rate control
                                else
                                    ppath_info->grab_field_cnt = 0; ///< reset count
                            }
                            else if(ppath_info->grab_one_field) {
                                ppath_info->grab_one_field = 0;
                            }

                            vcap_lli_add_framecnt(pdev_info, i, j, 1);
                        }
                    }

                    if(job_drop)
                        continue;

                    /* issue no job error notify */
                    if((!add_cnt) && (!pending_cnt) && list_empty(&plli->ch[i].path[j].ll_head)) {
                        vcap_vg_notify(pdev_info->index, i, j, VCAP_VG_NOTIFY_NO_JOB);
                        pdev_info->diag.ch[i].no_job_alm[j]++;
                    }

                    /* add md dummy job if no job on hardware list */
                    if((!add_cnt)                                 &&
                       (list_empty(&plli->ch[i].path[j].ll_head)) &&
                       (pdev_info->ch[i].md_active)               &&
                       (pdev_info->ch[i].param.comm.md_src == j)) {
#ifdef PLAT_MD_GROUPING
                        if((!pdev_info->dev_md_fatal) && ((pdev_info->ch[i].md_grp_id == pdev_info->md_enb_grp) || (pdev_info->ch[i].md_grp_id < 0))) {
                            ret = vcap_lli_wrap_md_dummy(pdev_info, i, j, 1);
                            if(ret == 0)
                                add_cnt++;
                        }
                        else if(pdev_info->ch[i].param.comm.md_en) {
                            ret = vcap_lli_wrap_md_dummy(pdev_info, i, j, 0);
                            if(ret == 0)
                                add_cnt++;
                        }
#else
                        if(!pdev_info->dev_md_fatal) {
                            ret = vcap_lli_wrap_md_dummy(pdev_info, i, j, 1);
                            if(ret == 0)
                                add_cnt++;
                        }
                        else if(pdev_info->ch[i].param.comm.md_en) {
                            ret = vcap_lli_wrap_md_dummy(pdev_info, i, j, 0);
                            if(ret == 0)
                                add_cnt++;
                        }
#endif
                    }
                    ch_jobs += add_cnt;
                }
            }

            /* Wrap to grab horizontal blanking command table */
            if(ch_grab_hblank) {
                if(!IS_DEV_CH_PATH_BUSY(pdev_info, i, VCAP_CH_HBLANK_PATH))
                    ret = pdev_info->start(pdev_info, i, VCAP_CH_HBLANK_PATH);
                else
                    ret = 0;

                if(ret == 0) {
                    for(k=0; k<2; k++) {
                        num = vcap_lli_wrap_hblank_command(pdev_info, i, VCAP_CH_HBLANK_PATH);
                        if(num <= 0)
                            break;
                        ch_jobs++;
                    }
                }
                else
                    pdev_info->vi[CH_TO_VI(i)].grab_hblank &= ~(0x1<<SUBCH_TO_VICH(i)); ///< force stop grab hblank
            }

            if(ch_jobs && !VCAP_IS_CH_ENABLE(pdev_info, i)) {
                vcap_dev_ch_enable(pdev_info, i);   ///< In LL mode, use channel enable bit to trigger LL hardware enter running state
#ifdef CFG_TIMEOUT_KERNEL_TIMER
                mod_timer(&pdev_info->ch[i].timer, jiffies+pdev_info->ch[i].timer_data.timeout+timeout_dummy);
#else
                pdev_info->ch[i].timer_data.count = 0;
#endif
            }

            /* select enable channel as frame count monitor channel for osd marquee reference clock source */
            if((fcnt_ch_tmp < 0) || (fcnt_ch_curr == i)) {
                if((ch_jobs || VCAP_IS_CH_ENABLE(pdev_info, i)))
                    fcnt_ch_tmp = i;
            }
        }

        /* switch hardware frame count monitor channel */
        if((fcnt_ch_tmp >= 0) && (fcnt_ch_curr != fcnt_ch_tmp))
            vcap_dev_set_frame_cnt_monitor_ch(pdev_info, fcnt_ch_tmp, 0);

#ifdef PLAT_MD_GROUPING
        vcap_lli_md_to_next_group(pdev_info);
#endif

        spin_unlock_irqrestore(&pdev_info->lock, flags);
    }

    return ret;
}

static void vcap_lli_pool_destroy(struct vcap_dev_info_t *pdev_info)
{
    int  ch, path;
    void *ll_table;
    struct vcap_lli_table_t *curr_table, *next_table, *curr_up_table, *next_up_table;
    struct vcap_lli_table_t *curr_n_table, *next_n_table;
    struct vcap_lli_t *plli = &pdev_info->lli;

    /* Free Channel Normal Pool */
    for(ch=0; ch<VCAP_CHANNEL_MAX; ch++) {
        for(path=0; path<VCAP_SCALER_MAX; path++) {
            /* Free Normal Table */
            if(plli->ch[ch].path[path].ll_pool.elements) {
                list_for_each_entry_safe(curr_table, next_table, &plli->ch[ch].path[path].ll_head, list) {
                    /* Update Table */
                    list_for_each_entry_safe(curr_up_table, next_up_table, &curr_table->update_head, list) {
                        list_del(&curr_up_table->list);
                        vcap_lli_add_element(&plli->update_pool, curr_up_table);
                    }
                    list_del(&curr_table->list);
                    vcap_lli_add_element(&plli->ch[ch].path[path].ll_pool, curr_table);
                }

                /* Free Normal Pool */
                while(plli->ch[ch].path[path].ll_pool.curr_idx) {
                    ll_table = vcap_lli_remove_element(&plli->ch[ch].path[path].ll_pool);
                    kfree(ll_table);
                }
                kfree(plli->ch[ch].path[path].ll_pool.elements);
                plli->ch[ch].path[path].ll_pool.elements = 0;
            }

            /* Free Null Table */
            if(plli->ch[ch].path[path].null_pool.elements) {
                list_for_each_entry_safe(curr_n_table, next_n_table, &plli->ch[ch].path[path].null_head, list) {
                    list_del(&curr_n_table->list);
                    vcap_lli_add_element(&plli->ch[ch].path[path].null_pool, curr_n_table);
                }

                while(plli->ch[ch].path[path].null_pool.curr_idx) {
                    ll_table = vcap_lli_remove_element(&plli->ch[ch].path[path].null_pool);
                    kfree(ll_table);
                }
                kfree(plli->ch[ch].path[path].null_pool.elements);
                plli->ch[ch].path[path].null_pool.elements = 0;
            }
        }
    }

    /* Free Update Pool */
    if(plli->update_pool.elements) {
        while(plli->update_pool.curr_idx) {
            ll_table = vcap_lli_remove_element(&plli->update_pool);
            kfree(ll_table);
        }
        kfree(plli->update_pool.elements);
        plli->update_pool.elements = 0;
    }
}

static int vcap_lli_pool_create(struct vcap_dev_info_t *pdev_info)
{
    int ret = 0;
    int i, ch, path;
    u32 tab_cmd_max, lli_mem_idx = 0;
    struct vcap_lli_table_t *ll_table;
    struct vcap_lli_t *plli = &pdev_info->lli;

    /* clear LL memory */
    memset((void *)(pdev_info->vbase+VCAP_LLI_MEM_OFFSET(0)), 0, VCAP_LLI_MEM_SIZE);

    /* Allocate Normal/Null Pool for each resolution of each channel */
    for(ch=0; ch<VCAP_CHANNEL_MAX; ch++) {
        if(pdev_info->ch[ch].active) {
            if(pdev_info->split.ch == ch) { ///< split channel
                for(path=0; path<VCAP_SCALER_MAX; path++) {
                    /* locker init */
                    spin_lock_init(&plli->ch[ch].path[path].ll_pool.lock);
                    spin_lock_init(&plli->ch[ch].path[path].null_pool.lock);

                    if(path >= 2) {  ///< split channel only have 2 resolution
                        INIT_LIST_HEAD(&plli->ch[ch].path[path].ll_head);
                        INIT_LIST_HEAD(&plli->ch[ch].path[path].null_head);
                        continue;
                    }

                    /* Normal Table Pool */
                    plli->ch[ch].path[path].ll_pool.elements = kzalloc(VCAP_LLI_PATH_NORMAL_TAB_MAX*sizeof(void *), GFP_KERNEL);
                    if(!plli->ch[ch].path[path].ll_pool.elements) {
                        vcap_err("[%d] ch%d-p%d(split) allocate normal pool element buffer failed!\n", pdev_info->index, ch, path);
                        ret = -1;
                        goto err;
                    }
                    INIT_LIST_HEAD(&plli->ch[ch].path[path].ll_head);

                    plli->ch[ch].path[path].ll_mem_start = lli_mem_idx*8;

                    tab_cmd_max = (pdev_info->split.x_num*pdev_info->split.y_num*2) + 1 + 1 + 1;     ///< 2 DMA Register + 1 Status CMD + 1 Next CMD + Sub-Channel offset 0
                    for(i=0; i<VCAP_LLI_PATH_NORMAL_TAB_MAX; i++) {
                        if((lli_mem_idx + tab_cmd_max) <= VCAP_LLI_MEM_CMD_MAX) {
                            ll_table = (struct vcap_lli_table_t *)kzalloc(sizeof(struct vcap_lli_table_t), GFP_KERNEL);
                            if(ll_table) {
                                ll_table->addr        = pdev_info->vbase + VCAP_LLI_MEM_OFFSET(lli_mem_idx*8);
                                ll_table->max_entries = tab_cmd_max;
                                ll_table->curr_entry  = 0;
                                INIT_LIST_HEAD(&ll_table->update_head);
                                vcap_lli_add_element(&plli->ch[ch].path[path].ll_pool, (void *)ll_table);
                                lli_mem_idx += tab_cmd_max;
                            }
                            else {
                                vcap_err("[%d] ch%d-p%d(split) normal pool allocate element failed!!\n", pdev_info->index, ch, path);
                                ret = -1;
                                goto err;
                            }
                        }
                    }

                    /* Null Table Pool */
                    plli->ch[ch].path[path].null_pool.elements = kzalloc(VCAP_LLI_PATH_NULL_TAB_MAX*sizeof(void *), GFP_KERNEL);
                    if(!plli->ch[ch].path[path].null_pool.elements) {
                        vcap_err("[%d] ch%d-p%d(split) allocate null pool element buffer failed!!\n", pdev_info->index, ch, path);
                        ret = -1;
                        goto err;
                    }
                    INIT_LIST_HEAD(&plli->ch[ch].path[path].null_head);

                    for(i=0; i<VCAP_LLI_PATH_NULL_TAB_MAX; i++) {
                        if((lli_mem_idx + VCAP_LLI_NULL_TAB_CMD_MAX) <= VCAP_LLI_MEM_CMD_MAX) {
                            ll_table = (struct vcap_lli_table_t *)kzalloc(sizeof(struct vcap_lli_table_t), GFP_KERNEL);
                            if(ll_table) {
                                ll_table->addr         = pdev_info->vbase + VCAP_LLI_MEM_OFFSET(lli_mem_idx*8);
                                ll_table->max_entries  = VCAP_LLI_NULL_TAB_CMD_MAX;
                                ll_table->curr_entry   = 0;
                                INIT_LIST_HEAD(&ll_table->update_head);
                                vcap_lli_add_element(&plli->ch[ch].path[path].null_pool, (void *)ll_table);
                                lli_mem_idx += VCAP_LLI_NULL_TAB_CMD_MAX;
                            }
                            else {
                                vcap_err("[%d] ch%d-p%d(split) null pool allocate element failed!!\n", pdev_info->index, ch, path);
                                ret = -1;
                                goto err;
                            }
                        }
                    }
                    plli->ch[ch].path[path].ll_mem_end = (lli_mem_idx*8)-1;
                }
            }
            else {  ///< Normal Channel
                for(path=0; path<VCAP_SCALER_MAX; path++) {
                    /* locker init */
                    spin_lock_init(&plli->ch[ch].path[path].ll_pool.lock);
                    spin_lock_init(&plli->ch[ch].path[path].null_pool.lock);

                    /* Normal Table Pool */
                    plli->ch[ch].path[path].ll_pool.elements = kzalloc(VCAP_LLI_PATH_NORMAL_TAB_MAX*sizeof(void *), GFP_KERNEL);
                    if(!plli->ch[ch].path[path].ll_pool.elements) {
                        vcap_err("[%d] ch%d-p%d allocate Normal Pool element buffer failed!!\n", pdev_info->index, ch, path);
                        ret = -1;
                        goto err;
                    }
                    INIT_LIST_HEAD(&plli->ch[ch].path[path].ll_head);

                    plli->ch[ch].path[path].ll_mem_start = lli_mem_idx*8;

                    for(i=0; i<VCAP_LLI_PATH_NORMAL_TAB_MAX; i++) {
                        if((lli_mem_idx + VCAP_LLI_NORMAL_TAB_CMD_MAX) <= VCAP_LLI_MEM_CMD_MAX) {
                            ll_table = (struct vcap_lli_table_t *)kzalloc(sizeof(struct vcap_lli_table_t), GFP_KERNEL);
                            if(ll_table) {
                                ll_table->addr         = pdev_info->vbase + VCAP_LLI_MEM_OFFSET(lli_mem_idx*8);
                                ll_table->max_entries  = VCAP_LLI_NORMAL_TAB_CMD_MAX;
                                ll_table->curr_entry   = 0;
                                INIT_LIST_HEAD(&ll_table->update_head);
                                vcap_lli_add_element(&plli->ch[ch].path[path].ll_pool, (void *)ll_table);
                                lli_mem_idx += VCAP_LLI_NORMAL_TAB_CMD_MAX;
                            }
                            else {
                                vcap_err("[%d] ch%d-p%d normal pool allocate element failed!!\n", pdev_info->index, ch, path);
                                ret = -1;
                                goto err;
                            }
                        }
                    }

                    /* Null Table Pool */
                    plli->ch[ch].path[path].null_pool.elements = kzalloc(VCAP_LLI_PATH_NULL_TAB_MAX*sizeof(void *), GFP_KERNEL);
                    if(!plli->ch[ch].path[path].null_pool.elements) {
                        vcap_err("[%d] ch%d-p%d allocate null pool element buffer failed!!\n", pdev_info->index, ch, path);
                        ret = -1;
                        goto err;
                    }
                    INIT_LIST_HEAD(&plli->ch[ch].path[path].null_head);

                    for(i=0; i<VCAP_LLI_PATH_NULL_TAB_MAX; i++) {
                        if((lli_mem_idx + VCAP_LLI_NULL_TAB_CMD_MAX) <= VCAP_LLI_MEM_CMD_MAX) {
                            ll_table = (struct vcap_lli_table_t *)kzalloc(sizeof(struct vcap_lli_table_t), GFP_KERNEL);
                            if(ll_table) {
                                ll_table->addr         = pdev_info->vbase + VCAP_LLI_MEM_OFFSET(lli_mem_idx*8);
                                ll_table->max_entries  = VCAP_LLI_NULL_TAB_CMD_MAX;
                                ll_table->curr_entry   = 0;
                                INIT_LIST_HEAD(&ll_table->update_head);
                                vcap_lli_add_element(&plli->ch[ch].path[path].null_pool, (void *)ll_table);
                                lli_mem_idx += VCAP_LLI_NULL_TAB_CMD_MAX;
                            }
                            else {
                                vcap_err("[%d] ch%d-p%d null pool allocate element failed!!\n", pdev_info->index, ch, path);
                                ret = -1;
                                goto err;
                            }
                        }
                    }
                    plli->ch[ch].path[path].ll_mem_end = (lli_mem_idx*8)-1;
                }
            }
        }
        else {
            for(path=0; path<VCAP_SCALER_MAX; path++) {
                spin_lock_init(&plli->ch[ch].path[path].ll_pool.lock);
                spin_lock_init(&plli->ch[ch].path[path].null_pool.lock);
                INIT_LIST_HEAD(&plli->ch[ch].path[path].ll_head);
                INIT_LIST_HEAD(&plli->ch[ch].path[path].null_head);
            }
        }
    }

    /* Allocate Update Pool */
    if((lli_mem_idx + VCAP_LLI_UPDATE_TAB_CMD_MAX) <= VCAP_LLI_MEM_CMD_MAX) {
        plli->update_pool.elements = kzalloc(((VCAP_LLI_MEM_CMD_MAX-lli_mem_idx)/VCAP_LLI_UPDATE_TAB_CMD_MAX)*sizeof(void *), GFP_KERNEL);
        if(!plli->update_pool.elements) {
            vcap_err("[%d] allocate element buffer failed!!\n", pdev_info->index);
            ret = -1;
            goto err;
        }

        do {
                ll_table = (struct vcap_lli_table_t *)kzalloc(sizeof(struct vcap_lli_table_t), GFP_KERNEL);
                if(ll_table) {
                    ll_table->addr        = pdev_info->vbase + VCAP_LLI_MEM_OFFSET(lli_mem_idx*8);
                    ll_table->max_entries = VCAP_LLI_UPDATE_TAB_CMD_MAX;
                    ll_table->curr_entry  = 0;
                    INIT_LIST_HEAD(&ll_table->update_head);
                    vcap_lli_add_element(&plli->update_pool, (void *)ll_table);
                    lli_mem_idx += VCAP_LLI_UPDATE_TAB_CMD_MAX;
                }
                else {
                    vcap_err("[%d] update pool allocate element failed!!\n", pdev_info->index);
                    ret = -1;
                    goto err;
                }
        }while((lli_mem_idx + VCAP_LLI_UPDATE_TAB_CMD_MAX) <= VCAP_LLI_MEM_CMD_MAX);
    }
    else {
        vcap_err("[%d] no free LLI memory to allocate update pool\n", pdev_info->index);
        ret = -1;
        goto err;
    }

err:
    if(ret < 0)
        vcap_lli_pool_destroy(pdev_info);

    return ret;
}

static void vcap_lli_ch_timeout(unsigned long data)
{
    int i;
    unsigned long flags = 0;
    struct vcap_timer_data_t *timer_data = (struct vcap_timer_data_t *)data;
    struct vcap_dev_info_t   *pdev_info  = (struct vcap_dev_info_t *)timer_data->data;
    int ch = timer_data->ch;

#ifndef CFG_TIMEOUT_KERNEL_TIMER
    vcap_err_limit("[%d] ch#%d timeout!(T:%ums)\n", pdev_info->index, ch, timer_data->count);
#else
    vcap_err_limit("[%d] ch#%d timeout!\n", pdev_info->index, ch);
#endif

    spin_lock_irqsave(&pdev_info->lock, flags);

    /* issue error notify */
    for(i=0; i<VCAP_SCALER_MAX; i++) {
        if(IS_DEV_CH_PATH_BUSY(pdev_info, ch, i))
            vcap_vg_notify(pdev_info->index, ch, i, VCAP_VG_NOTIFY_HW_DELAY);
    }

    /* force stop channel */
    vcap_lli_force_stop(pdev_info, ch);

    pdev_info->ch[ch].timer_data.count = 0;

    pdev_info->diag.ch[ch].job_timeout++;

    spin_unlock_irqrestore(&pdev_info->lock, flags);
}

static inline void vcap_lli_timeout_check(struct vcap_dev_info_t *pdev_info)
{
    int ch;
    unsigned long flags = 0;

    spin_lock_irqsave(&pdev_info->lock, flags);

    for(ch=0; ch<VCAP_CHANNEL_MAX; ch++) {
        if(pdev_info->ch[ch].active) {
            if(pdev_info->ch[ch].timer_data.count >= pdev_info->ch[ch].timer_data.timeout) {
                vcap_lli_ch_timeout((unsigned long)&pdev_info->ch[ch].timer_data);
            }
        }
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);
}

static inline void vcap_lli_detect_vi_hblank_chid(struct vcap_dev_info_t *pdev_info)
{
    int i, ch, ret;
    unsigned long flags;
    struct vcap_input_dev_t *pinput;

    spin_lock_irqsave(&pdev_info->lock, flags);

    for(i=0; i<VCAP_VI_MAX; i++) {
        if((pdev_info->m_param)->vi_mode[i] != VCAP_VI_RUN_MODE_2CH)    ///< only support on 2ch dual edge mode
            continue;

        pinput = vcap_input_get_info(i);
        if(!pinput)
            continue;

        if(pinput->probe_chid != VCAP_INPUT_PROBE_CHID_HBLANK)
            continue;

        if(pinput->mode != VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE && pinput->mode != VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID)
            continue;

        if(pinput->chid_ready)
            continue;

        ch = SUB_CH(i, 0);

        if(!pdev_info->ch[ch].active || !pdev_info->ch[ch].hblank_buf.vaddr)
            goto chid_ready;

        if(pdev_info->vi[i].grab_mode == VCAP_VI_GRAB_MODE_HBLANK) {
            if(pdev_info->vi[i].chid_det_time++ >= (pdev_info->m_param)->sync_time_div) {
                vcap_err("[%d] VI#%d horizontal blanking channel id detect failed!\n", pdev_info->index, i);
                goto chid_ready;
            }

            if((((volatile u8 *)(pdev_info->ch[ch].hblank_buf.vaddr))[VCAP_CH_HBLANK_BUF_SIZE-1]) == 0) ///< blanking data not ready
                continue;

            if(((((volatile u8 *)(pdev_info->ch[ch].hblank_buf.vaddr))[VCAP_CH_HBLANK_BUF_SIZE-1]) & 0x01) != 0) { ///< id not match
                /* invert pixel clock for swap channel in 2ch dual edge mode */
                ret = vcap_dev_vi_clock_invert(pdev_info, i, ((pinput->inv_clk == 0) ? 1 : 0));
                if(ret == 0) {
                    pinput->inv_clk = (pinput->inv_clk == 0) ? 1 : 0;
                }
                pdev_info->vi[i].grab_hblank = 0;   ///< disable grab horizontal blanking and grab data again
                continue;
            }

chid_ready:
            pinput->chid_ready = 1;
            pdev_info->vi[i].grab_hblank = 0;   ///< disable grab horizontal blanking
        }
        else {
            /* start path for trigger job to grab horizontal blanking */
            if(!IS_DEV_CH_PATH_BUSY(pdev_info, ch, VCAP_CH_HBLANK_PATH)) {
                ret = pdev_info->start(pdev_info, ch, VCAP_CH_HBLANK_PATH);
                if(ret != 0)
                    continue;
            }

            /* clear buffer */
            memset(pdev_info->ch[ch].hblank_buf.vaddr, 0, pdev_info->ch[ch].hblank_buf.size);

            /* clear channel id detect time counter */
            pdev_info->vi[i].chid_det_time = 0;

            /* start grab horizontal blanking, using ch#0 */
            pdev_info->vi[i].grab_hblank |= 0x1;
        }
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);
}

static void vcap_lli_tasklet(unsigned long data)
{
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)data;
    struct vcap_dev_module_param_t *pm_param = pdev_info->m_param;

    /*
     * If device meet fatal error, the tasklet will be trigger software reset
     * after fatal timer timeout. All trigger job will pend before reset.
     */
    if(pdev_info->dev_fatal) {
        pdev_info->fatal_time++;
        if(pdev_info->fatal_time > ((VCAP_DEF_FATAL_RESET_TIMEOUT*pm_param->sync_time_div)/1000)) {
            vcap_lli_fatal_sw_reset(pdev_info);
        }
    }
    else {
#ifndef CFG_TIMEOUT_KERNEL_TIMER
        /* channel lli timeout check */
        vcap_lli_timeout_check(pdev_info);
#endif

        /*
         * If device meet md job count overflow, must trigger md reset to
         * recover md engine
         */
        if(pdev_info->dev_md_fatal)
            vcap_lli_md_reset(pdev_info);

        /* detect channel id from horizontal blanking in 2ch dual edge mode, for correct channel sequence */
        vcap_lli_detect_vi_hblank_chid(pdev_info);

        /* trigger */
        if(pdev_info->trigger)
            pdev_info->trigger(pdev_info);

        /* check vi probe mode enable or not? */
        if(pdev_info->vi_probe.mode)
            vcap_dev_signal_probe(pdev_info);
    }
}

static irqreturn_t vcap_lli_irq_handler(int irq, void *dev_id)
{
    int ch, path, ll_done, vi_idx, ch_id;
    int bRemove;
    volatile u32 status[2];
    int ch_ready       = 0;
    int check_reg_55a8 = 0;
    int check_reg_55ac = 0;
    int fatal_err, timer_reset;
    int frame_cnt, frame_fail;
    u32 bitmask;
    u32 reg_0x5568, reg_0x55a8;
    u32 reg_0x55ac, reg_0x55b0, reg_0x55b4, reg_0x55dc;
    struct vcap_lli_table_t *curr_table, *next_table;
    struct vcap_lli_table_t *null_table, *last_table;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)dev_id;
    struct vcap_lli_t *plli = &pdev_info->lli;

#ifdef CONFIG_PLATFORM_A369
    {
        u32 ahb_int_sts;
        ahb_int_sts = ioread32(AHBB_FTAHBB020_3_VA_BASE+0x414);
        if(!(ahb_int_sts & BIT0))   ///< VCAP300 IRQ Connect to Extrenal AHB BUS PIN EXT_INT[0]
            return IRQ_NONE;

        /* Clear EXTAHB3 Interrupt */
        iowrite32(BIT0, AHBB_FTAHBB020_3_VA_BASE+0x41C);
    }
#endif

    reg_0x5568 = VCAP_REG_RD(pdev_info, VCAP_STATUS_OFFSET(0x68));
    reg_0x55a8 = VCAP_REG_RD(pdev_info, VCAP_STATUS_OFFSET(0xa8));
    reg_0x55ac = VCAP_REG_RD(pdev_info, VCAP_STATUS_OFFSET(0xac));
    reg_0x55b0 = VCAP_REG_RD(pdev_info, VCAP_STATUS_OFFSET(0xb0));
    reg_0x55b4 = VCAP_REG_RD(pdev_info, VCAP_STATUS_OFFSET(0xb4));
    reg_0x55dc = VCAP_REG_RD(pdev_info, VCAP_STATUS_OFFSET(0xdc));

    /* Clear Status */
    VCAP_REG_WR(pdev_info, VCAP_STATUS_OFFSET(0x68), reg_0x5568);
    VCAP_REG_WR(pdev_info, VCAP_STATUS_OFFSET(0xa8), reg_0x55a8);
    VCAP_REG_WR(pdev_info, VCAP_STATUS_OFFSET(0xac), reg_0x55ac);
    VCAP_REG_WR(pdev_info, VCAP_STATUS_OFFSET(0xb0), reg_0x55b0);
    VCAP_REG_WR(pdev_info, VCAP_STATUS_OFFSET(0xb4), reg_0x55b4);
    VCAP_REG_WR(pdev_info, VCAP_STATUS_OFFSET(0xdc), reg_0x55dc);

    /* Check LLI Done Status */
    spin_lock(&pdev_info->lock);
    for(ch=0; ch<VCAP_CHANNEL_MAX; ch++) {
        ll_done = 0;
        if(ch == VCAP_CASCADE_CH_NUM) {
            ch_id = VCAP_CASCADE_LLI_CH_ID;
            if(reg_0x55b4 & BIT0)
                ll_done = 1;
        }
        else {
            ch_id = ch;
            if(reg_0x55b0 & (0x1<<ch))
                ll_done = 1;
        }

#ifndef CFG_TIMEOUT_KERNEL_TIMER
        if(reg_0x55ac & BIT0) {
            if(pdev_info->ch[ch].active && VCAP_IS_CH_ENABLE(pdev_info, ch))
                pdev_info->ch[ch].timer_data.count += pdev_info->sync_time_unit;
        }
#endif

        if(ll_done) {
            vi_idx      = CH_TO_VI(ch);
            fatal_err   = 0;  ///< reset fatal error counter
            timer_reset = 0;
            frame_cnt   = -1;
            ch_ready++;
            for(path=0; path<VCAP_SCALER_MAX; path++) {
                /* check job list */
                list_for_each_entry_safe(curr_table, next_table, &plli->ch[ch].path[path].ll_head, list) {
                    bRemove    = 0;
                    frame_fail = 0;
                    status[0]  = *((volatile u32 *)curr_table->addr);
                    status[1]  = *((volatile u32 *)(curr_table->addr+4));

                    /* Check Table Ready or not */
                    if(status[0] & BIT0) {
                        /* check ch and path id in ll status */
                        if((ch_id != VCAP_LLI_STATUS_CH_ID(status[1])) || (path != VCAP_LLI_STATUS_SUB_ID(status[1]))) {
                            if(pdev_info->dbg_mode && printk_ratelimit()) {
                                vcap_err("[%d] ch#%d-p%d status id mismatch!(%d, %d)\n", pdev_info->index, ch, path,
                                         VCAP_LLI_STATUS_CH_ID(status[1]), VCAP_LLI_STATUS_SUB_ID(status[1]));
                            }
                            pdev_info->diag.ch[ch].ll_sts_id_mismatch++;
                            goto update_cnt;
                        }

                        /* DMA Write Done */
                        if(((status[1] & BIT20) == 0) && ((status[1] & BIT1) == 0)) {
                            if(pdev_info->dbg_mode && printk_ratelimit())
                                vcap_err("[%d] ch#%d-p%d DMA not write done!\n", pdev_info->index, ch, path);
                            pdev_info->diag.ch[ch].ll_dma_no_done++;
                        }

                        /* Split Channel DMA Write Done */
                        if((status[1] & BIT20) && ((status[1] & BIT2) == 0)) {
                            if(pdev_info->dbg_mode && printk_ratelimit())
                                vcap_err("[%d] ch#%d-p%d(split) DMA not write done!\n", pdev_info->index, ch, path);
                            pdev_info->diag.ch[ch].ll_split_dma_no_done++;
                        }

                        /* SD Fail */
                        if(status[1] & BIT3) {
                            fatal_err++;
                            if(status[1] & BIT12) {
                                if(pdev_info->dbg_mode && !pdev_info->ch[ch].path[path].skip_err_once && printk_ratelimit())
                                    vcap_err("[%d] ch#%d-p%d SD process fail!(SD job overflow)\n", pdev_info->index, ch, path);
                                pdev_info->diag.ch[ch].sd_job_ovf++;
                                pdev_info->dev_fatal++;
                                if(!((pdev_info->m_param)->grab_filter & VCAP_GRAB_FILTER_SD_JOB_OVF))
                                    frame_fail++;
                            }

                            if(status[1] & BIT13) {
                                if(pdev_info->dbg_mode && !pdev_info->ch[ch].path[path].skip_err_once && printk_ratelimit())
                                    vcap_err("[%d] ch#%d-p%d SD process fail!(SC memory overflow)\n", pdev_info->index, ch, path);
                                pdev_info->diag.ch[ch].sd_sc_mem_ovf++;
                                if(!((pdev_info->m_param)->grab_filter & VCAP_GRAB_FILTER_SD_SC_MEM_OVF))
                                    frame_fail++;
                            }

                            if(status[1] & BIT14) {
                                if(pdev_info->dbg_mode && !pdev_info->ch[ch].path[path].skip_err_once && printk_ratelimit())
                                    vcap_err("[%d] ch#%d-p%d SD process fail!(SD parameter error)\n", pdev_info->index, ch, path);
                                pdev_info->diag.ch[ch].sd_param_err++;
                                if(!((pdev_info->m_param)->grab_filter & VCAP_GRAB_FILTER_SD_PARAM_ERR))
                                    frame_fail++;
                            }

                            if(status[1] & BIT15) {
                                if(pdev_info->dbg_mode && !pdev_info->ch[ch].path[path].skip_err_once && printk_ratelimit())
                                    vcap_err("[%d] ch#%d-p%d SD process fail!(SD prefix decode error)\n", pdev_info->index, ch, path);
                                pdev_info->diag.ch[ch].sd_prefix_err++;
                                if(!((pdev_info->m_param)->grab_filter & VCAP_GRAB_FILTER_SD_PREFIX_ERR))
                                    frame_fail++;
                            }

                            if(status[1] & BIT16) {
                                if(pdev_info->dbg_mode && !pdev_info->ch[ch].path[path].skip_err_once && printk_ratelimit())
                                    vcap_err("[%d] ch#%d-p%d SD process fail!(SD timeout)\n", pdev_info->index, ch, path);
                                pdev_info->diag.ch[ch].sd_timeout++;
                                if(!((pdev_info->m_param)->grab_filter & VCAP_GRAB_FILTER_SD_TIMEOUT))
                                    frame_fail++;
                            }

                            if(status[1] & BIT17) {
                                if(pdev_info->dbg_mode && !pdev_info->ch[ch].path[path].skip_err_once && printk_ratelimit())
                                    vcap_err("[%d] ch#%d-p%d SD process fail!(Pixel lack)\n", pdev_info->index, ch, path);
                                pdev_info->diag.ch[ch].sd_pixel_lack++;
                                if(!((pdev_info->m_param)->grab_filter & VCAP_GRAB_FILTER_SD_PIXEL_LACK))
                                    frame_fail++;
                            }

                            if((status[1] & (0x3f<<12)) == 0) {
                                if(pdev_info->dbg_mode && !pdev_info->ch[ch].path[path].skip_err_once && printk_ratelimit())
                                    vcap_err("[%d] ch#%d-p%d SD process fail!(SC Error[Line Lack])\n", pdev_info->index, ch, path);
                                pdev_info->diag.ch[ch].sd_line_lack++;
                                if(!((pdev_info->m_param)->grab_filter & VCAP_GRAB_FILTER_SD_LINE_LACK))
                                    frame_fail++;
                            }
                        }

                        /* FIFO Full */
                        if(status[1] & BIT4) {
                            if(pdev_info->dbg_mode && !pdev_info->ch[ch].path[path].skip_err_once && printk_ratelimit())
                                vcap_err("[%d] ch#%d-p%d vi fifo full!\n", pdev_info->index, ch, path);
                            pdev_info->diag.ch[ch].vi_fifo_full++;
                        }

                        /* Pixel Lack */
                        if((status[1] & BIT5) && (pdev_info->ch[ch].skip_pixel_lack == 0) && (pdev_info->vi[vi_idx].grab_mode == VCAP_VI_GRAB_MODE_NORMAL)) {
                            if((pdev_info->capability.bug_isp_pixel_lack) &&
                               (ch == VCAP_CASCADE_CH_NUM) &&
                               (pdev_info->vi[vi_idx].format == VCAP_VI_FMT_ISP)) {
                                /* GM8136 ISP interface bug, pixel lack is not real error status, it is frame end status */
                            }
                            else {
                                if(pdev_info->dbg_mode && !pdev_info->ch[ch].path[path].skip_err_once && printk_ratelimit())
                                    vcap_err("[%d] ch#%d-p%d pixel lack!\n", pdev_info->index, ch, path);
                                pdev_info->diag.ch[ch].pixel_lack++;
                            }
                        }

                        /* Line Lack */
                        if((status[1] & BIT6) && (pdev_info->ch[ch].skip_line_lack == 0) && (pdev_info->vi[vi_idx].grab_mode == VCAP_VI_GRAB_MODE_NORMAL)) {
                            if(pdev_info->dbg_mode && !pdev_info->ch[ch].path[path].skip_err_once && printk_ratelimit())
                                vcap_err("[%d] ch#%d-p%d line lack!\n", pdev_info->index, ch, path);
                            pdev_info->diag.ch[ch].line_lack++;
                        }

                        /* Register 0x55a8 Status */
                        if(status[1] & BIT7) {
                            check_reg_55a8++;
                            check_reg_55ac++;
                        }

update_cnt:
                        /* Increase software field counter */
                        if(curr_table->type == VCAP_LLI_TAB_TYPE_NORMAL || curr_table->type == VCAP_LLI_TAB_TYPE_HBLANK) {
                            if(!(status[0] & BIT1)) {   ///< field mode
                                if(status[0] & BIT2)
                                    pdev_info->ch[ch].path[path].bot_cnt++;
                                else
                                    pdev_info->ch[ch].path[path].top_cnt++;
                            }
                            else ///< frame mode
                                pdev_info->ch[ch].path[path].top_cnt++;
                        }

                        if(((int)VCAP_LLI_STATUS_FRAME_CNT(status[0])) > frame_cnt)    ///< all path watch the same hardware frame counter
                            frame_cnt = VCAP_LLI_STATUS_FRAME_CNT(status[0]);

                        bRemove = 1;

                        timer_reset++;  ///< job ready, reset channel timeout timer
                    }
                    else {
                        if(status[0] & BIT3) {  ///< 1st field ready in grab field pair mode
                            /* SD Fail */
                            if(status[1] & BIT3) {
                                if(pdev_info->dbg_mode && !pdev_info->ch[ch].path[path].skip_err_once && printk_ratelimit())
                                    vcap_err("[%d] ch#%d-p%d 1st field SD process fail!\n", pdev_info->index, ch, path);
                                fatal_err++;
                                pdev_info->diag.ch[ch].sd_1st_field_err++;
                            }
                        }

                        /*
                         * Check first table not updated but last table status have updated, something wrong!!
                         */
                        last_table = list_entry(plli->ch[ch].path[path].ll_head.prev, typeof(*last_table), list);
                        if(curr_table->addr != last_table->addr) {
                            if((*((u32 *)last_table->addr)) & BIT0) {
                                if(pdev_info->dbg_mode && printk_ratelimit()) {
                                    vcap_err("[%d] ch#%d-p%d jump table update, curr:%08x(%08x), last:%08x(%08x)!!\n", pdev_info->index, ch, path,
                                             curr_table->addr, status[0], last_table->addr, *((u32 *)last_table->addr));
                                }
                                pdev_info->diag.ch[ch].ll_jump_table++;
                                bRemove = 1;
                                frame_fail++;
                            }
                        }

                        if(!bRemove)
                            break;  ///< go to check next path
                    }

                    if(bRemove) {
                        struct vcap_vg_job_item_t *job_item = (struct vcap_vg_job_item_t *)curr_table->job;

                        /* clear error message disable flag */
                        if(pdev_info->ch[ch].path[path].skip_err_once)
                            pdev_info->ch[ch].path[path].skip_err_once = 0;

                        if(curr_table->type == VCAP_LLI_TAB_TYPE_NORMAL) {
                            job_item->param.grab_num--;

                            /* job callback */
                            if(job_item->param.grab_num <= 0) {
                                if(job_item->param.frm_2frm_field) {    ///< save field status for property output
                                    job_item->param.src_field = (status[0] & BIT2) ? 1 : 0;
                                }

                                vcap_vg_mark_engine_finish(curr_table->job);
                                if(frame_fail)
                                    vcap_vg_trigger_callback_fail(curr_table->job);
                                else
                                    vcap_vg_trigger_callback_finish(curr_table->job);
                            }
                        }

                        if(curr_table->type == VCAP_LLI_TAB_TYPE_DUMMY_LOOP) {
                            /*
                             * don't free Normal and Null table if table is Loop Dummy
                             * only clear status in LL Table
                             */
                            *((volatile u32 *)curr_table->addr) = 0;


                            /*
                             * P2I Miss Top Field Workaround
                             * Terminate Dummy Loop Table if Cascade path#0~2 idle
                             */
                             if((ch == VCAP_CASCADE_CH_NUM) && (path == 3) && pdev_info->capability.bug_p2i_miss_top) {
                                if(list_empty(&plli->ch[ch].path[0].ll_head) &&
                                   list_empty(&plli->ch[ch].path[1].ll_head) &&
                                   list_empty(&plli->ch[ch].path[2].ll_head)   ) {
                                    /* Get last Null Table */
                                    null_table = list_entry(plli->ch[ch].path[path].null_head.prev, typeof(*null_table), list);
                                    null_table->curr_entry--;
                                    VCAP_LLI_ADD_CMD_NULL(null_table, ch, path);
                                    curr_table->type = VCAP_LLI_TAB_TYPE_DUMMY;
                                }
                             }
                        }
                        else {
                            /* free Null Table */
                            if(!list_empty(&plli->ch[ch].path[path].null_head)) {
                                null_table = list_entry(plli->ch[ch].path[path].null_head.next, typeof(*null_table), list);
                                if(*((u32 *)null_table->addr)) {
                                    if(pdev_info->dbg_mode && printk_ratelimit()) {
                                        vcap_err("[%d] ch#%d-p%d Null not zero(%08x = %08x)!!\n", pdev_info->index, ch, path,
                                                 null_table->addr, *((u32 *)null_table->addr));
                                    }
                                    pdev_info->diag.ch[ch].ll_null_not_zero++;
                                }

                                if((*(((u32 *)null_table->addr)+1) & 0xffff) != (curr_table->addr&0xffff)) {
                                    if(pdev_info->dbg_mode && printk_ratelimit()) {
                                        vcap_err("[%d] ch#%d-p%d Null mismatch(%08x != %08x)!!\n", pdev_info->index, ch, path,
                                                 (*(((u32 *)null_table->addr)+1) & 0xffff), (curr_table->addr&0xffff));
                                    }
                                    pdev_info->diag.ch[ch].ll_null_mismatch++;
                                }
                                vcap_lli_free_null_table(pdev_info, ch, path, null_table);
                            }

                            /* free Normal Table */
                            vcap_lli_free_normal_table(pdev_info, ch, path, curr_table);
                        }
                    }
                }
            }

            /* update channel hardware frame count for debug monitor */
            if(frame_cnt >= 0) {
                if((frame_cnt - pdev_info->ch[ch].hw_count_t0) >= 0)
                    pdev_info->ch[ch].hw_count += (frame_cnt - pdev_info->ch[ch].hw_count_t0);
                else
                    pdev_info->ch[ch].hw_count += (0x10000 + frame_cnt - pdev_info->ch[ch].hw_count_t0);  ///< hardware counter overflow
                pdev_info->ch[ch].hw_count_t0 = frame_cnt;
            }

            /* reset channel timeout timer */
            if(timer_reset) {
#ifdef CFG_TIMEOUT_KERNEL_TIMER
                if(VCAP_IS_CH_ENABLE(pdev_info, ch))
                    mod_timer(&pdev_info->ch[ch].timer, jiffies+pdev_info->ch[ch].timer_data.timeout);
                else
                    del_timer(&pdev_info->ch[ch].timer);
#else
                pdev_info->ch[ch].timer_data.count = 0;
#endif
            }

#ifdef VCAP_FRAME_RATE_FIELD_MISS_ORDER_BUG
            /*
             * Hardware fatal error will cause scaler frame rate control miss order,
             * must reinit frame rate control engine
             */
            if(fatal_err)
                pdev_info->ch[ch].temp_param.frame_rate_reinit = 1;
#endif
        }
    }

    /* Check Global Error Status */
    if(check_reg_55a8 || (reg_0x55a8 & 0x1ff01bfb)) {
        if(reg_0x55a8 & BIT0) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] DMA#0 overflow!\n", pdev_info->index);
            pdev_info->diag.global.dma[0].ovf++;
            pdev_info->dma_ovf_thres[0]++;
            if(pdev_info->dma_ovf_thres[0] >= (pdev_info->m_param)->sync_time_div)
                pdev_info->dev_fatal++;
        }
        else if(pdev_info->dma_ovf_thres[0])
            pdev_info->dma_ovf_thres[0] = 0;

        if(reg_0x55a8 & BIT1) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] DMA#0 job count overflow!\n", pdev_info->index);
            pdev_info->diag.global.dma[0].job_cnt_ovf++;
            pdev_info->dev_fatal++;
        }

        if(reg_0x55a8 & BIT3) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] DMA#0 write channel response fail!\n", pdev_info->index);
            pdev_info->diag.global.dma[0].write_resp_fail++;
        }

        if(reg_0x55a8 & BIT4) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] DMA#0 read channel response fail!\n", pdev_info->index);
            pdev_info->diag.global.dma[0].read_resp_fail++;
        }

        if(reg_0x55a8 & BIT5) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] SD job count overflow!\n", pdev_info->index);
            pdev_info->diag.global.sd_job_cnt_ovf++;
            pdev_info->dev_fatal++;
        }

        if(reg_0x55a8 & BIT6) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] DMA#0 commad prefix error!\n", pdev_info->index);
            pdev_info->diag.global.dma[0].cmd_prefix_err++;
        }

        if(reg_0x55a8 & BIT7) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] DMA#0 write channel block width equal 0!\n", pdev_info->index);
            pdev_info->diag.global.dma[0].write_blk_zero++;
        }

        if(reg_0x55a8 & BIT8) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] DMA#1 overflow!\n", pdev_info->index);
            pdev_info->diag.global.dma[1].ovf++;
            pdev_info->dma_ovf_thres[1]++;
            if(pdev_info->dma_ovf_thres[1] >= (pdev_info->m_param)->sync_time_div)
                pdev_info->dev_fatal++;
        }
        else if(pdev_info->dma_ovf_thres[1])
            pdev_info->dma_ovf_thres[1] = 0;

        if(reg_0x55a8 & BIT9) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] DMA#1 job count overflow!\n", pdev_info->index);
            pdev_info->diag.global.dma[1].job_cnt_ovf++;
        }

        if(reg_0x55a8 & BIT11) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] DMA#1 write channel response fail!\n", pdev_info->index);
            pdev_info->diag.global.dma[1].write_resp_fail++;
        }

        if(reg_0x55a8 & BIT12) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] DMA#1 commad prefix error!\n", pdev_info->index);
            pdev_info->diag.global.dma[1].cmd_prefix_err++;
        }

        if(reg_0x55a8 & BIT13) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] DMA#1 write channel block width equal 0!\n", pdev_info->index);
            pdev_info->diag.global.dma[1].write_blk_zero++;
        }

        for(vi_idx=0; vi_idx<VCAP_VI_MAX; vi_idx++) {
            if(IS_DEV_VI_BUSY(pdev_info, vi_idx)) {    ///< VI is Enable
                bitmask = (vi_idx == VCAP_CASCADE_VI_NUM) ? BIT28 : (0x1<<(vi_idx+20));
                if(reg_0x55a8 & bitmask) {
                    if(pdev_info->dbg_mode && printk_ratelimit())
                        vcap_err("[%d] VI%d no clock!\n", pdev_info->index, vi_idx);
                    pdev_info->diag.vi[vi_idx].no_clock++;
                }
            }
        }
    }
    else if(ch_ready) {
        if(((reg_0x55a8 & BIT0) == 0) && pdev_info->dma_ovf_thres[0]) {
            pdev_info->dma_ovf_thres[0] = 0;    ///< clear DMA#0 overflow counter if no meet continuous DMA#0 overflow
        }
        if(((reg_0x55a8 & BIT8) == 0) && pdev_info->dma_ovf_thres[1]) {
            pdev_info->dma_ovf_thres[1] = 0;    ///< clear DMA#1 overflow counter if no meet continuous DMA#1 overflow
        }
    }

    if(check_reg_55ac || (reg_0x55ac & 0x0000003e)) {
        if(reg_0x55ac & BIT1) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] MD job count overflow!\n", pdev_info->index);
            pdev_info->dev_md_fatal++;
            pdev_info->diag.global.md_job_cnt_ovf++;
        }

        if(reg_0x55ac & BIT2) {
            for(ch=0; ch<32; ch++) {
                if(reg_0x5568 & (0x1<<ch)) {
                    if(pdev_info->dbg_mode && printk_ratelimit())
                        vcap_err("[%d] ch#%d MD read traffic jam!\n", pdev_info->index, ch);
                    pdev_info->diag.ch[ch].md_traffic_jam++;
                }
            }
        }

        if(reg_0x55ac & BIT3) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] MD miss statistics done!\n", pdev_info->index);
            pdev_info->diag.global.md_miss_sts++;
        }

        if(reg_0x55ac & BIT4) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] LL channel(%d) id mismatch!\n", pdev_info->index, ((reg_0x55dc>>24)&0x3f));
            pdev_info->diag.global.ll_id_mismatch++;
        }

        if(reg_0x55ac & BIT5) {
            if(pdev_info->dbg_mode && printk_ratelimit())
                vcap_err("[%d] LL command load too late!\n", pdev_info->index);
            pdev_info->diag.global.ll_cmd_too_late++;
        }
    }

    /* Update Status */
    if(reg_0x55ac & BIT0) {
        for(ch=0; ch<VCAP_CHANNEL_MAX; ch++) {
            if(pdev_info->ch[ch].active) {
                for(path=0; path<VCAP_SCALER_MAX; path++) {
                    if(GET_DEV_CH_PATH_ST(pdev_info, ch, path) == VCAP_DEV_STATUS_STOP) {
                        SET_DEV_CH_PATH_ST(pdev_info, ch, path, VCAP_DEV_STATUS_IDLE);
                    }
                }
            }
        }
    }

    spin_unlock(&pdev_info->lock);

    /* trigger lli tasklet to link next job */
    if(reg_0x55ac & BIT0)
        tasklet_hi_schedule(&plli->ll_tasklet);

    return IRQ_HANDLED;
}

static void vcap_lli_deconstruct(struct vcap_dev_info_t *pdev_info)
{
    int i, j;
    struct vcap_path_t *ppath;

#ifdef CFG_HEARTBEAT_TIMER
    /* delete heartbeat timer */
    del_timer(&pdev_info->hb_timer);
#endif

    /* stop device */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        for(j=0; j<VCAP_SCALER_MAX; j++) {
            pdev_info->stop(pdev_info, i, j);
        }
    }

    /* disable device */
    vcap_dev_capture_disable(pdev_info);

    /* kill LL tasklet */
    tasklet_kill(&pdev_info->lli.ll_tasklet);

    /* free irq */
    if(pdev_info->irq >= 0)
        free_irq(pdev_info->irq, pdev_info);

#ifdef CFG_TIMEOUT_KERNEL_TIMER
    /* delete timer */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(pdev_info->ch[i].active)
            del_timer(&pdev_info->ch[i].timer);
    }
#endif

    /* remove md task */
    vcap_md_remove_task(pdev_info);

    /* free md statistic buffer */
    vcap_md_buf_free(pdev_info);

    /* free blank buffer */
    vcap_blank_buf_free(pdev_info);

    /* free job fifo queue */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        for(j=0; j<VCAP_SCALER_MAX; j++) {
            ppath = &pdev_info->ch[i].path[j];
            if(ppath->job_q) {
                kfifo_free(ppath->job_q);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
                kfree(ppath->job_q);
#endif
            }
        }
    }

    /* free LLI pool */
    vcap_lli_pool_destroy(pdev_info);

    /* free message buffer */
    if(pdev_info->msg_buf)
        kfree(pdev_info->msg_buf);

    if(pdev_info->vbase)
        iounmap((void __iomem *)(pdev_info->vbase));
}

int vcap_lli_construct(struct vcap_dev_info_t *pdev_info, int index, u32 addr_base, int irq, const char *sname)
{
    int i, j;
    int ret = 0;
    int md_grp_id = 0;
    struct vcap_path_t *ppath;
    struct vcap_dev_module_param_t *pm_param = pdev_info->m_param;

    pdev_info->irq      = -1;
    pdev_info->index    = index;
    pdev_info->dev_type = VCAP_DEV_DRV_TYPE_LLI;

    /* config module parameter */
    pdev_info->split.ch = -1;
#ifdef PLAT_SPLIT
    for(i=0; i<VCAP_VI_MAX; i++) {
        if(pm_param->vi_mode[i] == VCAP_VI_RUN_MODE_SPLIT) {
            pdev_info->split.ch = SUB_CH(i, 0);
            break;
        }
    }
    pdev_info->split.x_num = pm_param->split_x;
    pdev_info->split.y_num = pm_param->split_y;
#endif

    for(i=0; i<VCAP_VI_MAX; i++) {
        switch(pm_param->vi_mode[i]) {
            case VCAP_VI_RUN_MODE_BYPASS:
            case VCAP_VI_RUN_MODE_SPLIT:
                pdev_info->ch[SUB_CH(i, 0)].active = 1;
                break;
            case VCAP_VI_RUN_MODE_2CH:
                pdev_info->ch[SUB_CH(i, 0)].active = 1;
                pdev_info->ch[SUB_CH(i, 2)].active = 1;
                break;
            case VCAP_VI_RUN_MODE_4CH:
                pdev_info->ch[SUB_CH(i, 0)].active = 1;
                pdev_info->ch[SUB_CH(i, 1)].active = 1;
                pdev_info->ch[SUB_CH(i, 2)].active = 1;
                pdev_info->ch[SUB_CH(i, 3)].active = 1;
                break;
            default:
                break;
        }
    }

    /* init device status */
    pdev_info->status = 0;
    for(i=0; i<VCAP_VI_MAX; i++) {
        pdev_info->vi[i].input_norm = -1;   ///< init value
        pdev_info->vi[i].status     = 0;

        for(j=0; j<VCAP_VI_CH_MAX; j++)
            pdev_info->vi[i].ch_param[j].input_norm = pdev_info->vi[i].input_norm;
    }

    /* time unit(ms) of sync timer */
    if(pm_param->sync_time_div)
        pdev_info->sync_time_unit = 1000/pm_param->sync_time_div;

    /* Mapping phyical address to kernel virtual address */
    pdev_info->vbase = (u32)ioremap_nocache(addr_base, (VCAP_REG_SIZE - 1));
    if (pdev_info->vbase == 0) {
        vcap_err("vcap#%d counld not do ioremap!\n", index);
        ret = -ENOMEM;
        goto err;
    }

    /* setup device hardware capability */
    vcap_dev_config_hw_capability(pdev_info);

    /* Request IRQ */
    ret = request_irq(irq, vcap_lli_irq_handler, /*IRQF_DISABLED|*/IRQF_SHARED, sname, pdev_info);
    if(ret < 0) {
        vcap_err("vcap#%d request irq %d failed!", index, irq);
        goto err;
    }
    pdev_info->irq = irq;

    /* init lock */
    spin_lock_init(&pdev_info->lock);

    /* init semaphore lock */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
    sema_init(&pdev_info->sema_lock, 1);
    for(i=0; i<VCAP_CHANNEL_MAX; i++)
        sema_init(&pdev_info->ch[i].sema_lock, 1);
#else
    init_MUTEX(&pdev_info->sema_lock);
    for(i=0; i<VCAP_CHANNEL_MAX; i++)
        init_MUTEX(&pdev_info->ch[i].sema_lock);
#endif

    /* init channel timeout timer */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(pdev_info->ch[i].active) {
#ifdef CFG_TIMEOUT_KERNEL_TIMER
            init_timer(&pdev_info->ch[i].timer);
            pdev_info->ch[i].timer_data.ch      = i;
            pdev_info->ch[i].timer_data.data    = (void *)pdev_info;
            pdev_info->ch[i].timer_data.timeout = msecs_to_jiffies(VCAP_DEF_CH_TIMEOUT);
            pdev_info->ch[i].timer.function     = vcap_lli_ch_timeout;
            pdev_info->ch[i].timer.data         = (unsigned long)&pdev_info->ch[i].timer_data;
#else
            pdev_info->ch[i].timer_data.ch      = i;
            pdev_info->ch[i].timer_data.data    = (void *)pdev_info;
            pdev_info->ch[i].timer_data.timeout = VCAP_DEF_CH_TIMEOUT;
#endif
        }
    }

    /* create LLI pool */
    ret = vcap_lli_pool_create(pdev_info);
    if(ret < 0)
        goto err;

    /* job queue fifo allocate */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        pdev_info->ch[i].status = 0;

        for(j=0; j<VCAP_SCALER_MAX; j++) {
            ppath = &pdev_info->ch[i].path[j];

            spin_lock_init(&ppath->job_q_lock);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
            ppath->job_q = kzalloc(sizeof(struct kfifo), GFP_KERNEL);
            if(!ppath->job_q || kfifo_alloc(ppath->job_q, sizeof(int)*VCAP_JOB_MAX, GFP_KERNEL)) {
                vcap_err("vcap#%d job queue fifo allocate failed!", index);
                ret = -ENOMEM;
                goto err;
            }
#else
            ppath->job_q = kfifo_alloc(sizeof(int)*VCAP_JOB_MAX, GFP_KERNEL, &ppath->job_q_lock);
            if(!ppath->job_q) {
                vcap_err("vcap#%d job queue fifo allocate failed!", index);
                ret = -ENOMEM;
                goto err;
            }
#endif
        }
    }

    /* allocate MD Gaussian and Event buffer, and create MD Task */
    if(pm_param->cap_md) {
        vcap_md_buf_alloc(pdev_info);
        vcap_md_create_task(pdev_info);
    }

    /* assign channel MD group */
#ifdef PLAT_MD_GROUPING
    j = 0;
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(pdev_info->ch[i].active) {
            if((i == pdev_info->split.ch) || (i == VCAP_CASCADE_CH_NUM)) {
                pdev_info->ch[i].md_grp_id = -1;    ///< split and cascade channel not in md group control
            }
            else {
                if(j >= VCAP_MD_GROUP_CH_MAX) {
                   j = 0;
                   md_grp_id++;
                }

                pdev_info->ch[i].md_grp_id = md_grp_id;
                j++;
            }
        }
    }
#endif
    pdev_info->md_grp_max = md_grp_id + 1;

    /* allocate blank buffer */
    vcap_blank_buf_alloc(pdev_info);

    /* init LL tasklet */
    tasklet_init(&pdev_info->lli.ll_tasklet, vcap_lli_tasklet, (unsigned long)pdev_info);

#ifdef CFG_HEARTBEAT_TIMER
    /* init heartbeat timer */
    init_timer(&pdev_info->hb_timer);
    pdev_info->hb_timer.function = vcap_dev_heartbeat_handler;
    pdev_info->hb_timer.data     = (unsigned long)pdev_info;
    pdev_info->hb_timeout        = VCAP_HEARTBEAT_DEFAULT_TIMEOUT;
#endif

    /* allocate debug message buffer */
    pdev_info->msg_buf = kmalloc(PAGE_ALIGN(VCAP_DEV_MSG_BUF_SIZE), GFP_KERNEL);
    if(!pdev_info->msg_buf){
        vcap_err("vcap#%d allocate message buffer failed!\n", index);
        goto err;
    }

    /* register device operation function */
    pdev_info->init          = vcap_lli_dev_init;
    pdev_info->start         = vcap_lli_start;
    pdev_info->stop          = vcap_lli_stop;
    pdev_info->reset         = vcap_lli_fatal_sw_reset;
    pdev_info->trigger       = vcap_lli_trigger;
    pdev_info->fatal_stop    = vcap_lli_fatal_stop;
    pdev_info->deconstruct   = vcap_lli_deconstruct;

    return ret;

err:
    vcap_lli_deconstruct(pdev_info);
    return ret;
}
