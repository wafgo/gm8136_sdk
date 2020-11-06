/**
 * @file vcap_md.c
 *  vcap300 MD Control Interface
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.16 $
 * $Date: 2014/12/16 09:26:17 $
 *
 * ChangeLog:
 *  $Log: vcap_md.c,v $
 *  Revision 1.16  2014/12/16 09:26:17  jerson_l
 *  1. add md gaussian value check for detect gaussian vaule update error problem when md region switched.
 *  2. add md get all channel tamper status api.
 *
 *  Revision 1.15  2014/10/30 01:47:32  jerson_l
 *  1. support vi 2ch dual edge hybrid channel mode.
 *  2. support grab channel horizontal blanking data.
 *
 *  Revision 1.14  2014/02/20 06:19:18  jerson_l
 *  1. correct tamper status "Variance" display string
 *
 *  Revision 1.13  2014/02/10 09:52:53  jerson_l
 *  1. support tamper detection new feature for sensitive_black and sensitive_homogeneous threshold
 *
 *  Revision 1.12  2014/01/24 04:12:00  jerson_l
 *  1. support md tamper detection
 *
 *  Revision 1.11  2014/01/20 02:52:56  jerson_l
 *  1. remove md_reset control api
 *
 *  Revision 1.10  2013/12/04 10:03:45  jerson_l
 *  1. fix md enable status report incorrect when md not enable grouping
 *
 *  Revision 1.9  2013/07/22 10:12:22  jerson_l
 *  1. fix sometime md event not update problem
 *
 *  Revision 1.8  2013/06/24 08:36:17  jerson_l
 *  1. support md grouping
 *
 *  Revision 1.7  2013/06/18 02:14:05  jerson_l
 *  1. modify md buffer allocation to allocate all active channel md buffer
 *  2. support api to get all channel md information
 *
 *  Revision 1.6  2013/04/30 10:02:12  jerson_l
 *  1. modify md reset procedure
 *
 *  Revision 1.5  2013/03/26 02:25:23  jerson_l
 *  1. disable MD shadow if MD Revision = 0
 *  2. fix frammap free wrong memory address issue
 *
 *  Revision 1.4  2013/01/31 04:12:36  jerson_l
 *  1. fixed md event data copy from incorrect buffer address issue
 *
 *  Revision 1.3  2013/01/23 03:07:28  jerson_l
 *  1. add interlace to md region structure for specify md operation mode
 *
 *  Revision 1.2  2013/01/02 07:31:03  jerson_l
 *  1. add md region getting api
 *
 *  Revision 1.1  2012/12/11 09:30:38  jerson_l
 *  1. add motion detection support
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
#include "vcap_proc.h"
#include "vcap_dev.h"
#include "vcap_dbg.h"
#include "vcap_md.h"
#include "vcap_vg.h"
#include "frammap_if.h"

static int md_proc_ch[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = 0};

static struct proc_dir_entry *vcap_md_proc_root[VCAP_DEV_MAX]   = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_md_proc_ch[VCAP_DEV_MAX]     = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_md_proc_region[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_md_proc_event[VCAP_DEV_MAX]  = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_md_proc_param[VCAP_DEV_MAX]  = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *vcap_md_proc_tamper[VCAP_DEV_MAX] = {[0 ... (VCAP_DEV_MAX - 1)] = NULL};

static void vcap_md_dump_event(struct vcap_dev_info_t *pdev_info, int ch, struct seq_file *sfile)
{
    int  i, j, k;
    int  x_num, y_num;
    u32  *pgau, *pevent;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;
    int  mb_num     = 0;
    int  count      = 1;

    if(!pdev_info->ch[ch].md_gau_buf.vaddr || !pdev_info->ch[ch].md_event_buf.vaddr)
        return;

    if(ch == pdev_info->split.ch)
        count = pdev_info->split.x_num*pdev_info->split.y_num;

    x_num = pdev_info->ch[ch].param.md.x_num;
    y_num = pdev_info->ch[ch].param.md.y_num;

    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n=== [CH#%02d] MD Event ===\n", ch);
    for(k=0; k<count; k++) {
        if(ch == pdev_info->split.ch)
            mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n<< Split_CH#%02d >>\n", k);

        pgau   = (u32 *)((u32)(pdev_info->ch[ch].md_gau_buf.vaddr + VCAP_MD_GAU_SIZE*k));
        pevent = (u32 *)((u32)(pdev_info->ch[ch].md_event_buf.vaddr + VCAP_MD_EVENT_SIZE*k));

        for(i=0; i<y_num; i++) {
            for(j=0; j<x_num; j++) {
                if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD) {
                    if(sfile)
                        seq_printf(sfile, mbuf);
                    else
                        printk(mbuf);
                    mbuf_point = 0;
                }

                if(mb_num%x_num == 0) {
                    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "MB[%05d]: ", mb_num);
                }

#ifdef VCAP_MD_EVENT_96_MB_BUG
                if((j >= 96) && pdev_info->capability.bug_md_event_96_mb) {
                    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%d",
                                           (pgau[(((i*x_num)+j)*4)+1]>>29) & 0x3);
                }
                else {
                    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%d",
                                           (pevent[(mb_num%x_num)/16]>>(((mb_num%x_num)%16)*2)) & 0x3);
                }
#else
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "%d",
                                       (pevent[(mb_num%x_num)/16]>>(((mb_num%x_num)%16)*2)) & 0x3);
#endif

                if(((mb_num+1)%x_num) == 0) {
                    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
                }

                mb_num++;
            }
            pevent += 8;    ///< One row store 128 motion block event data
        }
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");

    if(sfile)
        seq_printf(sfile, mbuf);
    else
        printk(mbuf);
}

void vcap_md_init(struct vcap_dev_info_t *pdev_info)
{
    int i;
    u32 tmp;

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(!pdev_info->ch[i].active)
            continue;

        /* ALPHA, TBG */
        tmp = (VCAP_MD_ALPHA_DEFAULT) | (VCAP_MD_TBG_DEFAULT<<16);
        VCAP_REG_WR(pdev_info, ((i == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x08) : VCAP_CH_MD_OFFSET(i, 0x08)), tmp);

        /* INIT_VAL, TB, SIGMA, PRUNE, TAU */
        tmp = (VCAP_MD_INIT_VAL_DEFAULT)  |
              (VCAP_MD_TB_DEFAULT<<8)     |
              (VCAP_MD_SIGMA_DEFAULT<<12) |
              (VCAP_MD_PRUNE_DEFAULT<<20) |
              (VCAP_MD_TAU_DEFAULT<<24);
        VCAP_REG_WR(pdev_info, ((i == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x0c) : VCAP_CH_MD_OFFSET(i, 0x0c)), tmp);

        /* ALPHA_ACCURACY, TG */
        tmp = (VCAP_MD_ALPHA_ACCURACY_DEFAULT) | (VCAP_MD_TG_DEFAULT<<28);
        VCAP_REG_WR(pdev_info, ((i == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x10) : VCAP_CH_MD_OFFSET(i, 0x10)), tmp);

        /* DXDY, ONE_MIN_ALPHA, SHADOW */
        tmp = (VCAP_MD_DXDY_DEFAULT) | (VCAP_MD_ONE_MIN_ALPHA_DEFAULT<<16);
        if(pdev_info->capability.md_rev)    ///< not support shadow if md revision = 0
           tmp |= (VCAP_MD_SHADOW_DEFAULT<<31);
        VCAP_REG_WR(pdev_info, ((i == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x14) : VCAP_CH_MD_OFFSET(i, 0x14)), tmp);

        /* Tamper Parameter */
        pdev_info->ch[i].md_tamper.sensitive_b_th  = VCAP_MD_TAMPER_SENSITIVE_B_DEFAULT;
        pdev_info->ch[i].md_tamper.sensitive_h_th  = VCAP_MD_TAMPER_SENSITIVE_H_DEFAULT;
        pdev_info->ch[i].md_tamper.histogram_idx   = VCAP_MD_TAMPER_HISTOGRAM_DEFAULT;
        pdev_info->ch[i].md_tamper.alarm_on        = 0;
        pdev_info->ch[i].md_tamper.state           = VCAP_MD_TAMPER_STATE_IDLE;
        pdev_info->ch[i].md_tamper.md_path         = 0;
    }

    /* wake up md task for tamper detection */
    if((pdev_info->m_param)->cap_md && pdev_info->md_task) {
        wake_up_process(pdev_info->md_task);
    }
}

int vcap_md_buf_alloc(struct vcap_dev_info_t *pdev_info)
{
#define VCAP_MD_BUF_DDR_IDX DDR_ID_SYSTEM
    int i;
    int ret = 0;
    u32 buf_offset = 0;
    char buf_name[20];
    struct frammap_buf_info buf_info;

    if(pdev_info->md_buf.vaddr) {  ///< buffer have allocated
        goto exit;
    }

    memset(&buf_info, 0, sizeof(struct frammap_buf_info));

    /* Caculate require memory size */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(pdev_info->ch[i].active) {
            if(pdev_info->split.ch == i)
                buf_info.size += (VCAP_MD_GAU_SIZE + VCAP_MD_EVENT_SIZE)*pdev_info->split.x_num*pdev_info->split.y_num;
            else
                buf_info.size += (VCAP_MD_GAU_SIZE + VCAP_MD_EVENT_SIZE);
        }
    }

    if(buf_info.size == 0)
        goto exit;

    buf_info.alloc_type = ALLOC_NONCACHABLE;
    sprintf(buf_name, "vcap_md");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
    buf_info.align = VCAP_DMA_ADDR_ALIGN_SIZE;      ///< hardware dma start address must 8 byte alignment
    buf_info.name  = buf_name;
#endif
    ret = frm_get_buf_ddr(VCAP_MD_BUF_DDR_IDX, &buf_info);
    if(ret < 0) {
        vcap_err("[%d] md buffer allocate failed!\n", pdev_info->index);
        goto exit;
    }

    pdev_info->md_buf.vaddr = (void *)buf_info.va_addr;
    pdev_info->md_buf.paddr = buf_info.phy_addr;
    pdev_info->md_buf.size  = buf_info.size;
    strcpy(pdev_info->md_buf.name, buf_name);

    /* setup md gaussian buffer information for each channel */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(pdev_info->ch[i].active) {
            sprintf(pdev_info->ch[i].md_gau_buf.name, "vcap_md_gau.%d", i);
            pdev_info->ch[i].md_gau_buf.vaddr = (void *)(((u32)pdev_info->md_buf.vaddr) + buf_offset);
            pdev_info->ch[i].md_gau_buf.paddr = pdev_info->md_buf.paddr + buf_offset;
            if(pdev_info->split.ch == i)
                pdev_info->ch[i].md_gau_buf.size = VCAP_MD_GAU_SIZE*pdev_info->split.x_num*pdev_info->split.y_num;
            else
                pdev_info->ch[i].md_gau_buf.size = VCAP_MD_GAU_SIZE;

            buf_offset += pdev_info->ch[i].md_gau_buf.size;
        }
    }

    /* setup md event buffer information for each channel */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(pdev_info->ch[i].active) {
            sprintf(pdev_info->ch[i].md_event_buf.name, "vcap_md_evt.%d", i);
            pdev_info->ch[i].md_event_buf.vaddr = (void *)(((u32)pdev_info->md_buf.vaddr) + buf_offset);
            pdev_info->ch[i].md_event_buf.paddr = pdev_info->md_buf.paddr + buf_offset;
            if(pdev_info->split.ch == i)
                pdev_info->ch[i].md_event_buf.size = VCAP_MD_EVENT_SIZE*pdev_info->split.x_num*pdev_info->split.y_num;
            else
                pdev_info->ch[i].md_event_buf.size = VCAP_MD_EVENT_SIZE;

            buf_offset += pdev_info->ch[i].md_event_buf.size;
        }
    }

    /* clear buffer */
    memset(pdev_info->md_buf.vaddr, 0, pdev_info->md_buf.size);

exit:
    return ret;
}

void vcap_md_buf_free(struct vcap_dev_info_t *pdev_info)
{
    if(pdev_info->md_buf.vaddr) {
        int i;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
        frm_free_buf_ddr(pdev_info->md_buf.vaddr);
#else
        struct frammap_buf_info buf_info;

        buf_info.va_addr  = (u32)pdev_info->md_buf.vaddr;
        buf_info.phy_addr = pdev_info->md_buf.paddr;
        buf_info.size     = pdev_info->md_buf.size;
        frm_free_buf_ddr(&buf_info);
#endif

        /* clear buffer information */
        for(i=0; i<VCAP_CHANNEL_MAX; i++) {
            if(pdev_info->ch[i].active) {
                /* Gaussian Buffer */
                pdev_info->ch[i].md_gau_buf.vaddr = 0;
                pdev_info->ch[i].md_gau_buf.paddr = 0;
                pdev_info->ch[i].md_gau_buf.size  = 0;
                memset(pdev_info->ch[i].md_gau_buf.name, 0, sizeof(pdev_info->ch[i].md_gau_buf.name));

                /* Event Buffer */
                pdev_info->ch[i].md_event_buf.vaddr = 0;
                pdev_info->ch[i].md_event_buf.paddr = 0;
                pdev_info->ch[i].md_event_buf.size  = 0;
                memset(pdev_info->ch[i].md_event_buf.name, 0, sizeof(pdev_info->ch[i].md_event_buf.name));
            }
        }
    }
}

int vcap_md_get_event(struct vcap_dev_info_t *pdev_info, int ch, u32 *event_buf, u32 buf_size)
{
    int ret = 0;
    unsigned long flags = 0;
    u32 even_size;

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(!event_buf || !pdev_info->ch[ch].md_event_buf.vaddr) {
        vcap_err("[%d] ch#%d get md event failed!(buffer invalid)\n", pdev_info->index, ch);
        ret = -1;
        goto exit;
    }

    if(ch == pdev_info->split.ch) {
        vcap_err("[%d] ch#%d is split channel, please use get_all_info api to get event!\n", pdev_info->index, ch);
        ret = -1;
        goto exit;
    }

    /* check input event buffer size */
    even_size = pdev_info->ch[ch].param.md.y_num*(VCAP_MD_X_NUM_MAX*VCAP_MD_EVENT_LEN/8);    ///< byte
    if(buf_size < even_size) {
        vcap_err("[%d] ch#%d get md event failed!(buffer not enough, %d < %d)\n", pdev_info->index, ch, buf_size, even_size);
        ret = -1;
        goto exit;
    }

    /* copy event value to event buffer */
    if(even_size)
        memcpy(event_buf, (void *)((u32)(pdev_info->ch[ch].md_event_buf.vaddr)), even_size);

exit:
    spin_unlock_irqrestore(&pdev_info->lock, flags);
    return ret;
}

u32 vcap_md_get_param(struct vcap_dev_info_t *pdev_info, int ch, VCAP_MD_PARAM_T param_id)
{
    u32 value;

    switch(param_id) {
        case VCAP_MD_PARAM_ALPHA:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x08) : VCAP_CH_MD_OFFSET(ch, 0x08)));
            value &= 0xffff;
            break;
        case VCAP_MD_PARAM_TBG:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x08) : VCAP_CH_MD_OFFSET(ch, 0x08)));
            value = (value>>16) & 0x1fff;
            break;
        case VCAP_MD_PARAM_INIT_VAL:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x0c) : VCAP_CH_MD_OFFSET(ch, 0x0c)));
            value &= 0xff;
            break;
        case VCAP_MD_PARAM_TB:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x0c) : VCAP_CH_MD_OFFSET(ch, 0x0c)));
            value = (value>>8) & 0xf;
            break;
        case VCAP_MD_PARAM_SIGMA:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x0c) : VCAP_CH_MD_OFFSET(ch, 0x0c)));
            value = (value>>12) & 0x1f;
            break;
        case VCAP_MD_PARAM_PRUNE:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x0c) : VCAP_CH_MD_OFFSET(ch, 0x0c)));
            value = (value>>20) & 0xf;
            break;
        case VCAP_MD_PARAM_TAU:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x0c) : VCAP_CH_MD_OFFSET(ch, 0x0c)));
            value = (value>>24) & 0xff;
            break;
        case VCAP_MD_PARAM_ALPHA_ACCURACY:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x10) : VCAP_CH_MD_OFFSET(ch, 0x10)));
            value &= 0xfffffff;
            break;
        case VCAP_MD_PARAM_TG:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x10) : VCAP_CH_MD_OFFSET(ch, 0x10)));
            value = (value>>28) & 0xf;
            break;
        case VCAP_MD_PARAM_DXDY:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x14) : VCAP_CH_MD_OFFSET(ch, 0x14)));
            value &= 0xffff;
            break;
        case VCAP_MD_PARAM_ONE_MIN_ALPHA:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x14) : VCAP_CH_MD_OFFSET(ch, 0x14)));
            value = (value>>16) & 0x7fff;
            break;
        case VCAP_MD_PARAM_SHADOW:
            value = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x14) : VCAP_CH_MD_OFFSET(ch, 0x14)));
            value = (value>>31) & 0x1;
            break;
        case VCAP_MD_PARAM_TAMPER_SENSITIVE_B:
            value = pdev_info->ch[ch].md_tamper.sensitive_b_th;
            break;
        case VCAP_MD_PARAM_TAMPER_SENSITIVE_H:
            value = pdev_info->ch[ch].md_tamper.sensitive_h_th;
            break;
        case VCAP_MD_PARAM_TAMPER_HISTOGRAM:
            value = pdev_info->ch[ch].md_tamper.histogram_idx;
            break;
        default:
            value = 0;
            break;
    }

    return value;
}

int vcap_md_get_region(struct vcap_dev_info_t *pdev_info, int ch, struct vcap_md_region_t *region)
{
    unsigned long flags;

    if(!region)
        return -1;

    spin_lock_irqsave(&pdev_info->lock, flags);

    region->enable  = pdev_info->ch[ch].md_active;
    region->x_start = pdev_info->ch[ch].param.md.x_start;
    region->y_start = pdev_info->ch[ch].param.md.y_start;
    region->x_num   = pdev_info->ch[ch].param.md.x_num;
    region->y_num   = pdev_info->ch[ch].param.md.y_num;
    region->x_size  = pdev_info->ch[ch].param.md.x_size;
    region->y_size  = pdev_info->ch[ch].param.md.y_size;

    if(pdev_info->vi[CH_TO_VI(ch)].ch_param[SUBCH_TO_VICH(ch)].prog == VCAP_VI_PROG_INTERLACE)
        region->interlace = 1;
    else
        region->interlace = 0;

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_md_get_all_info(struct vcap_dev_info_t *pdev_info, struct vcap_md_all_info_t *md_info)
{
    int i;
    unsigned long flags;

    if(!md_info)
        return -1;

    spin_lock_irqsave(&pdev_info->lock, flags);

    md_info->dev_event_va   = 0;
    md_info->dev_event_pa   = 0;
    md_info->dev_event_size = 0;

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(i >= VCAP_MD_ALL_INFO_CH_MAX)
            break;

        if(pdev_info->ch[i].active) {
            md_info->ch[i].enable     = pdev_info->ch[i].md_active;
            md_info->ch[i].x_start    = pdev_info->ch[i].param.md.x_start;
            md_info->ch[i].y_start    = pdev_info->ch[i].param.md.y_start;
            md_info->ch[i].x_num      = pdev_info->ch[i].param.md.x_num;
            md_info->ch[i].y_num      = pdev_info->ch[i].param.md.y_num;
            md_info->ch[i].x_size     = pdev_info->ch[i].param.md.x_size;
            md_info->ch[i].y_size     = pdev_info->ch[i].param.md.y_size;
            md_info->ch[i].interlace  = (pdev_info->vi[CH_TO_VI(i)].ch_param[SUBCH_TO_VICH(i)].prog == VCAP_VI_PROG_INTERLACE) ? 1 : 0;
            md_info->ch[i].event_va   = (u32)pdev_info->ch[i].md_event_buf.vaddr;
            md_info->ch[i].event_pa   = (u32)pdev_info->ch[i].md_event_buf.paddr;
            md_info->ch[i].event_size = (u32)pdev_info->ch[i].md_event_buf.size;

            if(!md_info->dev_event_va) {
                md_info->dev_event_va = md_info->ch[i].event_va;
                md_info->dev_event_pa = md_info->ch[i].event_pa;
            }

            md_info->dev_event_size += md_info->ch[i].event_size;
        }
        else {
            memset(&md_info->ch[i], 0, sizeof(struct vcap_md_ch_info_t));
        }
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_md_get_all_tamper(struct vcap_dev_info_t *pdev_info, struct vcap_md_all_tamper_t *md_tamper)
{
    int i;

    if(!md_tamper)
        return -1;

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(i >= VCAP_MD_ALL_INFO_CH_MAX)
            break;

        down(&pdev_info->ch[i].sema_lock);

        if(pdev_info->ch[i].active) {
            md_tamper->ch[i].md_enb         = pdev_info->ch[i].md_active;
            md_tamper->ch[i].alarm          = pdev_info->ch[i].md_tamper.alarm_on;
            md_tamper->ch[i].sensitive_b_th = pdev_info->ch[i].md_tamper.sensitive_b_th;
            md_tamper->ch[i].sensitive_h_th = pdev_info->ch[i].md_tamper.sensitive_h_th;
            md_tamper->ch[i].histogram_idx  = pdev_info->ch[i].md_tamper.histogram_idx;
        }
        else {
            memset(&md_tamper->ch[i], 0, sizeof(struct vcap_md_ch_tamper_t));
        }

        up(&pdev_info->ch[i].sema_lock);
    }

    return 0;
}

int vcap_md_set_param(struct vcap_dev_info_t *pdev_info, int ch, VCAP_MD_PARAM_T param_id, u32 data)
{
    int ret = 0;
    u32 tmp;

    switch(param_id) {
        case VCAP_MD_PARAM_ALPHA:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_MD_OFFSET(0x08));
                tmp &= (~0xffff);
                tmp |= (data & 0xffff);
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x08), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x08));
                tmp &= (~0xffff);
                tmp |= (data & 0xffff);
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x08), tmp);
            }
            break;
        case VCAP_MD_PARAM_TBG:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_MD_OFFSET(0x08));
                tmp &= ~(0x1fff<<16);
                tmp |= ((data & 0x1fff)<<16);
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x08), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x08));
                tmp &= ~(0x1fff<<16);
                tmp |= ((data & 0x1fff)<<16);
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x08), tmp);
            }
            break;
        case VCAP_MD_PARAM_INIT_VAL:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_MD_OFFSET(0x0c));
                tmp &= ~(0xff);
                tmp |= (data & 0xff);
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x0c), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x0c));
                tmp &= ~(0xff);
                tmp |= (data & 0xff);
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x0c), tmp);
            }
            break;
        case VCAP_MD_PARAM_TB:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_MD_OFFSET(0x0c));
                tmp &= ~(0xf<<8);
                tmp |= ((data & 0xf)<<8);
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x0c), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x0c));
                tmp &= ~(0xf<<8);
                tmp |= ((data & 0xf)<<8);
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x0c), tmp);
            }
            break;
        case VCAP_MD_PARAM_SIGMA:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_MD_OFFSET(0x0c));
                tmp &= ~(0x1f<<12);
                tmp |= ((data & 0x1f)<<12);
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x0c), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x0c));
                tmp &= ~(0x1f<<12);
                tmp |= ((data & 0x1f)<<12);
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x0c), tmp);
            }
            break;
        case VCAP_MD_PARAM_PRUNE:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_MD_OFFSET(0x0c));
                tmp &= ~(0xf<<20);
                tmp |= ((data & 0xf)<<20);
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x0c), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x0c));
                tmp &= ~(0xf<<20);
                tmp |= ((data & 0xf)<<20);
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x0c), tmp);
            }
            break;
        case VCAP_MD_PARAM_TAU:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_MD_OFFSET(0x0c));
                tmp &= ~(0xff<<24);
                tmp |= ((data & 0xff)<<24);
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x0c), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x0c));
                tmp &= ~(0xff<<24);
                tmp |= ((data & 0xff)<<24);
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x0c), tmp);
            }
            break;
        case VCAP_MD_PARAM_ALPHA_ACCURACY:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_MD_OFFSET(0x10));
                tmp &= ~0xfffffff;
                tmp |= (data & 0xfffffff);
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x10), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x10));
                tmp &= ~0xfffffff;
                tmp |= (data & 0xfffffff);
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x10), tmp);
            }
            break;
        case VCAP_MD_PARAM_TG:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_MD_OFFSET(0x10));
                tmp &= ~(0xf<<28);
                tmp |= ((data & 0xf)<<28);
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x10), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x10));
                tmp &= ~(0xf<<28);
                tmp |= ((data & 0xf)<<28);
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x10), tmp);
            }
            break;
        case VCAP_MD_PARAM_DXDY:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_MD_OFFSET(0x14));
                tmp &= ~0xffff;
                tmp |= (data & 0xffff);
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x14), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x14));
                tmp &= ~0xffff;
                tmp |= (data & 0xffff);
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x14), tmp);
            }
            break;
        case VCAP_MD_PARAM_ONE_MIN_ALPHA:
            if(ch == VCAP_CASCADE_CH_NUM) {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_MD_OFFSET(0x14));
                tmp &= ~(0x7fff<<16);
                tmp |= ((data & 0x7fff)<<16);
                VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x14), tmp);
            }
            else {
                tmp = VCAP_REG_RD(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x14));
                tmp &= ~(0x7fff<<16);
                tmp |= ((data & 0x7fff)<<16);
                VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x14), tmp);
            }
            break;
        case VCAP_MD_PARAM_SHADOW:
            if(pdev_info->capability.md_rev) {  ///< not support shadow if md revision = 0
                if(ch == VCAP_CASCADE_CH_NUM) {
                    tmp = VCAP_REG_RD(pdev_info, VCAP_CAS_MD_OFFSET(0x14));
                    tmp &= ~(0x1<<31);
                    tmp |= ((data & 0x1)<<31);
                    VCAP_REG_WR(pdev_info, VCAP_CAS_MD_OFFSET(0x14), tmp);
                }
                else {
                    tmp = VCAP_REG_RD(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x14));
                    tmp &= ~(0x1<<31);
                    tmp |= ((data & 0x1)<<31);
                    VCAP_REG_WR(pdev_info, VCAP_CH_MD_OFFSET(ch, 0x14), tmp);
                }
            }
            break;
        case VCAP_MD_PARAM_TAMPER_SENSITIVE_B:
            if((data >= VCAP_MD_TAMPER_SENSITIVE_MIN) && (data <= VCAP_MD_TAMPER_SENSITIVE_MAX))
                pdev_info->ch[ch].md_tamper.sensitive_b_th = (int)data;
            else
                ret = -1;
            break;
        case VCAP_MD_PARAM_TAMPER_SENSITIVE_H:
            if((data >= VCAP_MD_TAMPER_SENSITIVE_MIN) && (data <= VCAP_MD_TAMPER_SENSITIVE_MAX))
                pdev_info->ch[ch].md_tamper.sensitive_h_th = (int)data;
            else
                ret = -1;
            break;
        case VCAP_MD_PARAM_TAMPER_HISTOGRAM:
            if((data >= VCAP_MD_TAMPER_HISTOGRAM_MIN) && (data <= VCAP_MD_TAMPER_HISTOGRAM_MAX))
                pdev_info->ch[ch].md_tamper.histogram_idx = (int)data;
            else
                ret = -1;
            break;
        default:
            ret = -1;
            break;
    }

    return ret;
}

static ssize_t vcap_md_proc_ch_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  ch;
    char value_str[8] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &ch);

    down(&pdev_info->sema_lock);

    if(ch != md_proc_ch[pdev_info->index])
        md_proc_ch[pdev_info->index] = ch;

    up(&pdev_info->sema_lock);

    return count;
}

static ssize_t vcap_md_proc_param_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, param_id;
    u32  value;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %x\n", &param_id, &value);

    if(param_id >= VCAP_MD_PARAM_TAMPER_SENSITIVE_B)
        goto exit;

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(md_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || md_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                down(&pdev_info->ch[i].sema_lock);
                vcap_md_set_param(pdev_info, i, param_id, value);
                up(&pdev_info->ch[i].sema_lock);
            }
        }
    }

    up(&pdev_info->sema_lock);

exit:
    return count;
}

static ssize_t vcap_md_proc_tamper_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  i, param_id;
    int  value;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &param_id, &value);

    param_id += VCAP_MD_PARAM_TAMPER_SENSITIVE_B;

    if((param_id != VCAP_MD_PARAM_TAMPER_SENSITIVE_B) && (param_id != VCAP_MD_PARAM_TAMPER_SENSITIVE_H) && (param_id != VCAP_MD_PARAM_TAMPER_HISTOGRAM))
        goto exit;

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(md_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || md_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                down(&pdev_info->ch[i].sema_lock);
                vcap_md_set_param(pdev_info, i, param_id, value);
                up(&pdev_info->ch[i].sema_lock);
            }
        }
    }

exit:
    return count;
}

static int vcap_md_proc_ch_show(struct seq_file *sfile, void *v)
{
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    down(&pdev_info->sema_lock);

    seq_printf(sfile, "MD_Control_CH: %d (0~%d, other for all)\n", md_proc_ch[pdev_info->index], VCAP_CHANNEL_MAX-1);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_md_proc_region_show(struct seq_file *sfile, void *v)
{
    int i;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(md_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || md_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                down(&pdev_info->ch[i].sema_lock);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n=== [CH#%02d] MD Region ===\n", i);
#ifdef PLAT_MD_GROUPING
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " MD_GROUP  : %d\n", pdev_info->ch[i].md_grp_id);
#endif
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " MD_Active : %d\n",     pdev_info->ch[i].md_active);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " MD_Enable : %d\n",     pdev_info->ch[i].param.comm.md_en);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " MD_Src    : %d\n",     pdev_info->ch[i].param.comm.md_src);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " MD_X_Start: %d\n",     pdev_info->ch[i].param.md.x_start);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " MD_Y_Start: %d\n",     pdev_info->ch[i].param.md.y_start);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " MD_X_Size : %d\n",     pdev_info->ch[i].param.md.x_size);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " MD_Y_Size : %d\n",     pdev_info->ch[i].param.md.y_size);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " MD_X_Num  : %d\n",     pdev_info->ch[i].param.md.x_num);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " MD_Y_Num  : %d\n",     pdev_info->ch[i].param.md.y_num);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " MD_GAU_PA : 0x%08x\n", pdev_info->ch[i].md_gau_buf.paddr);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " MD_EVT_PA : 0x%08x\n", pdev_info->ch[i].md_event_buf.paddr);
                up(&pdev_info->ch[i].sema_lock);

                if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD) {
                    seq_printf(sfile, mbuf);
                    mbuf_point = 0;
                }
            }
        }
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    seq_printf(sfile, mbuf);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_md_proc_event_show(struct seq_file *sfile, void *v)
{
    int i;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(md_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || md_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                down(&pdev_info->ch[i].sema_lock);
                vcap_md_dump_event(pdev_info, i, sfile);
                up(&pdev_info->ch[i].sema_lock);
            }
        }
    }

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_md_proc_param_show(struct seq_file *sfile, void *v)
{
    int i;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    down(&pdev_info->sema_lock);

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(md_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || md_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                down(&pdev_info->ch[i].sema_lock);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n=== [CH#%02d] MD Parameter ===\n", i);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]Alpha         : 0x%x\n",
                                       VCAP_MD_PARAM_ALPHA, vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_ALPHA));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]TBG           : 0x%x\n",
                                       VCAP_MD_PARAM_TBG, vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_TBG));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]INIT_VAL      : 0x%x\n",
                                       VCAP_MD_PARAM_INIT_VAL, vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_INIT_VAL));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]TB            : 0x%x\n",
                                       VCAP_MD_PARAM_TB, vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_TB));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]SIGMA         : 0x%x\n",
                                       VCAP_MD_PARAM_SIGMA, vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_SIGMA));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]PRUNE         : 0x%x\n",
                                       VCAP_MD_PARAM_PRUNE, vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_PRUNE));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]TAU           : 0x%x\n",
                                       VCAP_MD_PARAM_TAU, vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_TAU));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]ALPHA_ACCURACY: 0x%x\n",
                                       VCAP_MD_PARAM_ALPHA_ACCURACY, vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_ALPHA_ACCURACY));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]TG            : 0x%x\n",
                                       VCAP_MD_PARAM_TG, vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_TG));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]DXDY          : 0x%x\n",
                                       VCAP_MD_PARAM_DXDY, vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_DXDY));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]ONE_MIN_ALPHA : 0x%x\n",
                                       VCAP_MD_PARAM_ONE_MIN_ALPHA, vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_ONE_MIN_ALPHA));

                if(pdev_info->capability.md_rev) {  ///< not support shadow if md revision = 0
                    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [%02d]SHADOW        : 0x%x\n",
                                           VCAP_MD_PARAM_SHADOW, vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_SHADOW));
                }

                up(&pdev_info->ch[i].sema_lock);

                if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD) {
                    seq_printf(sfile, mbuf);
                    mbuf_point = 0;
                }
            }
        }
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    seq_printf(sfile, mbuf);

    up(&pdev_info->sema_lock);

    return 0;
}

static int vcap_md_proc_tamper_show(struct seq_file *sfile, void *v)
{
    int i;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)sfile->private;
    char *mbuf = pdev_info->msg_buf;
    int  mbuf_point = 0;

    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(md_proc_ch[pdev_info->index] >= VCAP_CHANNEL_MAX || md_proc_ch[pdev_info->index] == i) {
            if(pdev_info->ch[i].active) {
                down(&pdev_info->ch[i].sema_lock);

                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n[CH#%02d]", i);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n=== Tamper Parameter ===\n");
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [00]SENSITIVE_BLACK      : %-3d [%d ~ %d]\n",
                                       vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_TAMPER_SENSITIVE_B), VCAP_MD_TAMPER_SENSITIVE_MIN, VCAP_MD_TAMPER_SENSITIVE_MAX);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [01]SENSITIVE_HOMOGENEOUS: %-3d [%d ~ %d]\n",
                                       vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_TAMPER_SENSITIVE_H), VCAP_MD_TAMPER_SENSITIVE_MIN, VCAP_MD_TAMPER_SENSITIVE_MAX);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " [02]Histogram            : %-3d [%d ~ %d]\n",
                                       vcap_md_get_param(pdev_info, i, VCAP_MD_PARAM_TAMPER_HISTOGRAM), VCAP_MD_TAMPER_HISTOGRAM_MIN, VCAP_MD_TAMPER_HISTOGRAM_MAX);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n=== Tamper Status ===\n");
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " Alarm         : %s\n", ((pdev_info->ch[i].md_tamper.alarm_on==0) ? "No" : "Yes"));
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " Less  Value   : %d\n", pdev_info->ch[i].md_tamper.less);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " Variance      : %d\n", pdev_info->ch[i].md_tamper.variance);
                mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, " Total Value   : %d\n", pdev_info->ch[i].md_tamper.total);

                up(&pdev_info->ch[i].sema_lock);

                if(mbuf_point >= VCAP_DEV_MSG_THRESHOLD) {
                    seq_printf(sfile, mbuf);
                    mbuf_point = 0;
                }
            }
        }
    }
    mbuf_point += snprintf(mbuf+mbuf_point, VCAP_DEV_MSG_BUF_SIZE-mbuf_point, "\n");
    seq_printf(sfile, mbuf);

    return 0;
}

static int vcap_md_proc_ch_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_md_proc_ch_show, PDE(inode)->data);
}

static int vcap_md_proc_region_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_md_proc_region_show, PDE(inode)->data);
}

static int vcap_md_proc_event_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_md_proc_event_show, PDE(inode)->data);
}

static int vcap_md_proc_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_md_proc_param_show, PDE(inode)->data);
}

static int vcap_md_proc_tamper_open(struct inode *inode, struct file *file)
{
    return single_open(file, vcap_md_proc_tamper_show, PDE(inode)->data);
}

static struct file_operations vcap_md_proc_ch_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_md_proc_ch_open,
    .write  = vcap_md_proc_ch_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_md_proc_region_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_md_proc_region_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_md_proc_event_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_md_proc_event_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_md_proc_param_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_md_proc_param_open,
    .write  = vcap_md_proc_param_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vcap_md_proc_tamper_ops = {
    .owner  = THIS_MODULE,
    .open   = vcap_md_proc_tamper_open,
    .write  = vcap_md_proc_tamper_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

void vcap_md_proc_remove(int id)
{
    if(vcap_md_proc_root[id]) {
        struct proc_dir_entry *proc_root = (struct proc_dir_entry *)vcap_dev_get_dev_proc_root(id);

        if(vcap_md_proc_tamper[id])
            vcap_proc_remove_entry(vcap_md_proc_root[id], vcap_md_proc_tamper[id]);

        if(vcap_md_proc_param[id])
            vcap_proc_remove_entry(vcap_md_proc_root[id], vcap_md_proc_param[id]);

        if(vcap_md_proc_event[id])
            vcap_proc_remove_entry(vcap_md_proc_root[id], vcap_md_proc_event[id]);

        if(vcap_md_proc_region[id])
            vcap_proc_remove_entry(vcap_md_proc_root[id], vcap_md_proc_region[id]);

        if(vcap_md_proc_ch[id])
            vcap_proc_remove_entry(vcap_md_proc_root[id], vcap_md_proc_ch[id]);

        vcap_proc_remove_entry(proc_root, vcap_md_proc_root[id]);
    }
}

int vcap_md_proc_init(int id, void *private)
{
    int ret = 0;
    struct proc_dir_entry *proc_root = (struct proc_dir_entry *)vcap_dev_get_dev_proc_root(id);

    /* get device proc root */
    if(!proc_root) {
        ret = -EINVAL;
        goto end;
    }

    /* root */
    vcap_md_proc_root[id] = vcap_proc_create_entry("md", S_IFDIR|S_IRUGO|S_IXUGO, proc_root);
    if(!vcap_md_proc_root[id]) {
        vcap_err("create proc node %s/md failed!\n", proc_root->name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_md_proc_root[id]->owner = THIS_MODULE;
#endif

    /* ch */
    vcap_md_proc_ch[id] = vcap_proc_create_entry("ch", S_IRUGO|S_IXUGO, vcap_md_proc_root[id]);
    if(!vcap_md_proc_ch[id]) {
        vcap_err("create proc node 'md/ch' failed!\n");
        ret = -EINVAL;
        goto err;
    }
    vcap_md_proc_ch[id]->proc_fops = &vcap_md_proc_ch_ops;
    vcap_md_proc_ch[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_md_proc_ch[id]->owner     = THIS_MODULE;
#endif

    /* region */
    vcap_md_proc_region[id] = vcap_proc_create_entry("region", S_IRUGO|S_IXUGO, vcap_md_proc_root[id]);
    if(!vcap_md_proc_region[id]) {
        vcap_err("create proc node 'md/region' failed!\n");
        ret = -EINVAL;
        goto err;
    }
    vcap_md_proc_region[id]->proc_fops = &vcap_md_proc_region_ops;
    vcap_md_proc_region[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_md_proc_region[id]->owner     = THIS_MODULE;
#endif

    /* event */
    vcap_md_proc_event[id] = vcap_proc_create_entry("event", S_IRUGO|S_IXUGO, vcap_md_proc_root[id]);
    if(!vcap_md_proc_event[id]) {
        vcap_err("create proc node 'md/event' failed!\n");
        ret = -EINVAL;
        goto err;
    }
    vcap_md_proc_event[id]->proc_fops = &vcap_md_proc_event_ops;
    vcap_md_proc_event[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_md_proc_event[id]->owner     = THIS_MODULE;
#endif

    /* param */
    vcap_md_proc_param[id] = vcap_proc_create_entry("param", S_IRUGO|S_IXUGO, vcap_md_proc_root[id]);
    if(!vcap_md_proc_param[id]) {
        vcap_err("create proc node 'md/param' failed!\n");
        ret = -EINVAL;
        goto err;
    }
    vcap_md_proc_param[id]->proc_fops = &vcap_md_proc_param_ops;
    vcap_md_proc_param[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_md_proc_param[id]->owner     = THIS_MODULE;
#endif

    /* tamper */
    vcap_md_proc_tamper[id] = vcap_proc_create_entry("tamper", S_IRUGO|S_IXUGO, vcap_md_proc_root[id]);
    if(!vcap_md_proc_tamper[id]) {
        vcap_err("create proc node 'md/tamper' failed!\n");
        ret = -EINVAL;
        goto err;
    }
    vcap_md_proc_tamper[id]->proc_fops = &vcap_md_proc_tamper_ops;
    vcap_md_proc_tamper[id]->data      = private;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vcap_md_proc_tamper[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    vcap_md_proc_remove(id);
    return ret;
}

static inline int vcap_md_tamper_detection(struct vcap_dev_info_t *pdev_info)
{
    int ret = 0;
    int idx, j, k, x, y;
    int ch, n_x_mb, n_y_mb, match, fit;
    u8  n_mode, *muY_pre, *nmode_pre;
    u16 *weight_pre;
    u32 *p_gau;
    u8  muY_c, muY[3];
    u16 weight_c, weight[3];
    int large80, less80, total;
    int pre_frame, cur_frame, variance;

    if(!(pdev_info->m_param)->cap_md)
        goto exit;

    for(ch=0; ch<VCAP_CHANNEL_MAX; ch++) {
        down(&pdev_info->ch[ch].sema_lock);

        if(!pdev_info->ch[ch].active)
            goto ch_next;

        if((!pdev_info->ch[ch].md_active) ||
           ((pdev_info->ch[ch].md_tamper.sensitive_b_th == 0) && (pdev_info->ch[ch].md_tamper.sensitive_h_th == 0)) ||
           (!IS_DEV_CH_PATH_BUSY(pdev_info, ch, pdev_info->ch[ch].param.comm.md_src))) {
            if(pdev_info->ch[ch].md_tamper.state == VCAP_MD_TAMPER_STATE_START) {
                pdev_info->ch[ch].md_tamper.state    = VCAP_MD_TAMPER_STATE_IDLE;
                pdev_info->ch[ch].md_tamper.alarm_on = 0;
                pdev_info->ch[ch].md_tamper.less     = 0;
                pdev_info->ch[ch].md_tamper.variance = 0;
                pdev_info->ch[ch].md_tamper.total    = 0;
            }
            goto ch_next;
        }

        /* check md path switch */
        if(pdev_info->ch[ch].param.comm.md_src != pdev_info->ch[ch].md_tamper.md_path) {
            if(pdev_info->ch[ch].md_tamper.state == VCAP_MD_TAMPER_STATE_START) {
                pdev_info->ch[ch].md_tamper.state    = VCAP_MD_TAMPER_STATE_IDLE;
                pdev_info->ch[ch].md_tamper.alarm_on = 0;
                pdev_info->ch[ch].md_tamper.less     = 0;
                pdev_info->ch[ch].md_tamper.variance = 0;
                pdev_info->ch[ch].md_tamper.total    = 0;
            }
            pdev_info->ch[ch].md_tamper.md_path = pdev_info->ch[ch].param.comm.md_src;
        }

        if(pdev_info->ch[ch].md_gau_buf.vaddr     &&
           pdev_info->ch[ch].md_tamper.muY_pre    &&
           pdev_info->ch[ch].md_tamper.weight_pre &&
           pdev_info->ch[ch].md_tamper.nmode_pre) {
            large80 = less80 = total = 0;
            pre_frame = cur_frame = variance = 0;
            nmode_pre = pdev_info->ch[ch].md_tamper.nmode_pre;
            n_x_mb    = pdev_info->ch[ch].param.md.x_num;
            n_y_mb    = pdev_info->ch[ch].param.md.y_num;

            idx = 0;
            for(y=0; y<n_y_mb; y+=VCAP_MD_TAMPER_Y_UNIT) {
                for(x=0; x<n_x_mb; x+=VCAP_MD_TAMPER_X_UNIT) {
                    if(idx >= (VCAP_MD_TAMPER_X_NUM_MAX*VCAP_MD_TAMPER_Y_NUM_MAX))
                        break;

                    p_gau      = ((u32 *)pdev_info->ch[ch].md_gau_buf.vaddr)+(((y*n_x_mb)+x)*4);
                    muY_pre    = (u8  *)&pdev_info->ch[ch].md_tamper.muY_pre[idx*VCAP_MD_TAMPER_GAU_MAX];
                    weight_pre = (u16 *)&pdev_info->ch[ch].md_tamper.weight_pre[idx*VCAP_MD_TAMPER_GAU_MAX];

                    /* init previous data at first time */
                    if(pdev_info->ch[ch].md_tamper.state == VCAP_MD_TAMPER_STATE_IDLE) {
                         muY_pre[0]     = VCAP_MD_GAU_GET_MU(p_gau[0]);
                         weight_pre[0]  = VCAP_MD_GAU_GET_WEIGHT(p_gau[0]);
                         nmode_pre[idx] = VCAP_MD_GAU_GET_MODE(p_gau[0]);

                         if(muY_pre[0] >= pdev_info->ch[ch].md_tamper.histogram_idx)
                            large80++;
                         else
                            less80++;
                         total++;

                         /* homogeneous check */
                         if((x == 0) && (y == 0))
                            pre_frame = muY_pre[0];
                         else {
                            cur_frame = muY_pre[0];
                            if(abs(cur_frame - pre_frame) < 8)
                                variance++;
                            pre_frame = cur_frame;
                         }
                    }
                    else {
                        match = 0;
                        fit   = 0;

                        n_mode = VCAP_MD_GAU_GET_MODE(p_gau[0]);
                        for(j=0; j<n_mode; j++) {
                            muY_c    = muY[j]    = VCAP_MD_GAU_GET_MU(p_gau[j]);
                            weight_c = weight[j] = VCAP_MD_GAU_GET_WEIGHT(p_gau[j]);

                            for(k=0; k<nmode_pre[idx]; k++) {
                                if(match == 0)	{
                                    if(((muY_c-muY_pre[k])*(muY_c-muY_pre[k])) < 64) {
                                        fit = 1;
                                        if(weight_c >= weight_pre[k]) {
                                            match = 1;
                                            if(muY_c >= pdev_info->ch[ch].md_tamper.histogram_idx)
                                               large80++;
                                            else
                                               less80++;
                                            total++;

                                            /* homogeneous check */
                                            if((x == 0) && (y == 0))
                                                pre_frame = muY_c;
                                            else {
                                                cur_frame = muY_c;
                                                if(abs(cur_frame - pre_frame) < 8)
                                                    variance++;
                                                pre_frame = cur_frame;
                                            }
                                        }
                                    }
                                }
                            }
                        }

			            if((match == 0) && n_mode) {
			            	if(muY_c > pdev_info->ch[ch].md_tamper.histogram_idx)
			            		large80++;
			            	else
			            		less80++;
			            	total++;

			            	/* homogeneous check */
                            if((x == 0) && (y == 0))
                                pre_frame = muY_c;
                            else {
                                cur_frame = muY_c;
                                if(abs(cur_frame - pre_frame) < 8)
                                    variance++;
                                pre_frame = cur_frame;
                            }
			            }

			            nmode_pre[idx] = n_mode;
			            for(j=0; j<n_mode; j++) {
			            	muY_pre[j]    = muY[j];
			            	weight_pre[j] = weight[j];
			            }
                    }
                    idx++;
                }
            }

            if((pdev_info->ch[ch].md_tamper.sensitive_b_th && ((less80*100)   > (total*(100 - pdev_info->ch[ch].md_tamper.sensitive_b_th)))) ||
               (pdev_info->ch[ch].md_tamper.sensitive_h_th && ((variance*100) > (total*(100 - pdev_info->ch[ch].md_tamper.sensitive_h_th))))) {
                if(!pdev_info->ch[ch].md_tamper.alarm_on) {
                    if(vcap_vg_notify(pdev_info->index, ch, -1, VCAP_VG_NOTIFY_TAMPER_ALARM) == 0)
                        pdev_info->ch[ch].md_tamper.alarm_on = 1;
                }
            }
            else {
                if(pdev_info->ch[ch].md_tamper.alarm_on) {
                    if(vcap_vg_notify(pdev_info->index, ch, -1, VCAP_VG_NOTIFY_TAMPER_ALARM_RELEASE) == 0)
                        pdev_info->ch[ch].md_tamper.alarm_on = 0;
                }
            }

            /* record less and total value for monitor */
            pdev_info->ch[ch].md_tamper.less     = less80;
            pdev_info->ch[ch].md_tamper.variance = variance;
            pdev_info->ch[ch].md_tamper.total    = total;

            /* Mark tamper detection start */
            if(pdev_info->ch[ch].md_tamper.state == VCAP_MD_TAMPER_STATE_IDLE)
                pdev_info->ch[ch].md_tamper.state = VCAP_MD_TAMPER_STATE_START;
        }

ch_next:
        up(&pdev_info->ch[ch].sema_lock);
    }

exit:
    return ret;
}

#ifdef VCAP_MD_GAU_VALUE_CHECK
static inline void vcap_md_gau_check(struct vcap_dev_info_t *pdev_info)
{
    int ch, md_x_num, md_y_num;
    u32 tmp, gau_m0, gau_m1;
    unsigned long flags;

    if(!(pdev_info->m_param)->cap_md)
        return;

    spin_lock_irqsave(&pdev_info->lock, flags);

    for(ch=0; ch<VCAP_CHANNEL_MAX; ch++) {
        if(!pdev_info->ch[ch].active)
            continue;

        if(!pdev_info->ch[ch].md_active || !pdev_info->ch[ch].md_gau_buf.vaddr)
            continue;

        if(!pdev_info->ch[ch].do_md_gau_chk)
            continue;

        if(!IS_DEV_CH_PATH_BUSY(pdev_info, ch, pdev_info->ch[ch].param.comm.md_src))
            continue;

        if(pdev_info->ch[ch].do_md_gau_chk == 1) {    ///< clear gaussian value at first time
            /* get md x and y number form hardware register */
            tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MD_OFFSET(0x04) : VCAP_CH_MD_OFFSET(ch, 0x04)));
            md_x_num = (tmp>>16) & 0xff;
            md_y_num = (tmp>>24) & 0xff;

            /* check register have ready sync to new md parameter */
            if((md_x_num == pdev_info->ch[ch].param.md.x_num) && (md_y_num == pdev_info->ch[ch].param.md.y_num)) {
                if(md_x_num && md_y_num) {
                    /* clear mode 0 gaussian value */
                    if(md_y_num > 1) {
                        ((u32 *)pdev_info->ch[ch].md_gau_buf.vaddr)[(md_x_num*(md_y_num-1)*4)] = 0;   ///< GAU_M0
                    }

                    if(md_y_num < VCAP_MD_Y_NUM_MAX) {
                        ((u32 *)pdev_info->ch[ch].md_gau_buf.vaddr)[(md_x_num*md_y_num*4)] = 0;       ///< GAU_M0
                    }
                }
                pdev_info->ch[ch].do_md_gau_chk++;  ///< check gaussian value at next time
            }
        }
        else {
            /* get MB#0 gaussion value */
            gau_m0 = ((u32 *)pdev_info->ch[ch].md_gau_buf.vaddr)[0];
            gau_m1 = ((u32 *)pdev_info->ch[ch].md_gau_buf.vaddr)[1];

            if((gau_m0 != 0) && (gau_m1 == 0)) {      ///< md gaussian value incorrect, trigger md reset to recover md engine
                pdev_info->dev_md_fatal++;
                pdev_info->diag.ch[ch].md_gau_err++;
                if(pdev_info->dbg_mode >= 2) {
                    vcap_err("[%d] ch#%d MD gaussian value error!\n", pdev_info->index, ch);
                }
            }

            pdev_info->ch[ch].do_md_gau_chk = 0;
        }
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);
}
#endif

static int vcap_md_task_thread(void *data)
{
    int i;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)data;

    /* allocate buffer for record tamper data */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(!pdev_info->ch[i].active)
            continue;

        pdev_info->ch[i].md_tamper.weight_pre = kzalloc((VCAP_MD_TAMPER_X_NUM_MAX*VCAP_MD_TAMPER_Y_NUM_MAX*VCAP_MD_TAMPER_GAU_MAX*sizeof(u16)), GFP_KERNEL);
        if(!pdev_info->ch[i].md_tamper.weight_pre) {
            vcap_err("ch#%d allocate md tamper weight buffer fail!!\n", i);
        }

        pdev_info->ch[i].md_tamper.muY_pre = kzalloc((VCAP_MD_TAMPER_X_NUM_MAX*VCAP_MD_TAMPER_Y_NUM_MAX*VCAP_MD_TAMPER_GAU_MAX*sizeof(u8)), GFP_KERNEL);
        if(!pdev_info->ch[i].md_tamper.muY_pre) {
            vcap_err("ch#%d allocate md tamper muY buffer fail!!\n", i);
        }

        pdev_info->ch[i].md_tamper.nmode_pre = kzalloc(((VCAP_MD_TAMPER_X_NUM_MAX*VCAP_MD_TAMPER_Y_NUM_MAX)*sizeof(u8)), GFP_KERNEL);
        if(!pdev_info->ch[i].md_tamper.nmode_pre) {
            vcap_err("ch#%d allocate md tamper nmode buffer fail!!\n", i);
        }
    }

    /* do md tamper detection */
    do {
#ifdef VCAP_MD_GAU_VALUE_CHECK
        vcap_md_gau_check(pdev_info);
#endif

        vcap_md_tamper_detection(pdev_info);

        /* sleep 1 second */
        schedule_timeout_interruptible(msecs_to_jiffies(1000));
    } while (!kthread_should_stop());

    /* free buffer */
    for(i=0; i<VCAP_CHANNEL_MAX; i++) {
        if(!pdev_info->ch[i].active)
            continue;

        if(!pdev_info->ch[i].md_tamper.weight_pre) {
            kfree(pdev_info->ch[i].md_tamper.weight_pre);
            pdev_info->ch[i].md_tamper.weight_pre = NULL;
        }

        if(!pdev_info->ch[i].md_tamper.muY_pre) {
            kfree(pdev_info->ch[i].md_tamper.muY_pre);
            pdev_info->ch[i].md_tamper.muY_pre = NULL;
        }

        if(!pdev_info->ch[i].md_tamper.nmode_pre) {
            kfree(pdev_info->ch[i].md_tamper.nmode_pre);
            pdev_info->ch[i].md_tamper.nmode_pre = NULL;
        }
    }

    return 0;
}

int vcap_md_create_task(struct vcap_dev_info_t *pdev_info)
{
    int ret = 0;

    if(pdev_info->md_task) {
        ret = -1;
        goto exit;
    }

    pdev_info->md_task = kthread_create(vcap_md_task_thread, pdev_info, "vcap_md");
    if(IS_ERR(pdev_info->md_task)) {
        pdev_info->md_task = 0;
        ret = -1;
        goto exit;
    }

exit:
    if(ret < 0)
        vcap_err("create md task thread failed!\n");

    return ret;
}

void vcap_md_remove_task(struct vcap_dev_info_t *pdev_info)
{
    if(pdev_info->md_task) {
        kthread_stop(pdev_info->md_task);
    }
}
