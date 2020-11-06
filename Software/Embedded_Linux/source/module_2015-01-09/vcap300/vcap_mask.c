/**
 * @file vcap_mark.c
 *  vcap300 Mark Window Control Interface
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.7 $
 * $Date: 2014/10/30 01:47:32 $
 *
 * ChangeLog:
 *  $Log: vcap_mask.c,v $
 *  Revision 1.7  2014/10/30 01:47:32  jerson_l
 *  1. support vi 2ch dual edge hybrid channel mode.
 *  2. support grab channel horizontal blanking data.
 *
 *  Revision 1.6  2013/10/14 04:02:51  jerson_l
 *  1. support GM8139 platform
 *
 *  Revision 1.5  2013/08/26 12:21:42  jerson_l
 *  1. reject mask border setting if mask window not support hollow feature
 *
 *  Revision 1.4  2013/05/08 08:20:04  jerson_l
 *  1. change mask x position and width alignment from 4 to 2
 *
 *  Revision 1.3  2013/04/30 10:03:03  jerson_l
 *  1. support SDI8BIT format
 *
 *  Revision 1.2  2012/12/18 11:59:58  jerson_l
 *  1. remove parameter check for setup mark window
 *
 *  Revision 1.1  2012/12/11 09:29:58  jerson_l
 *  1. add mask support
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#include "bits.h"
#include "vcap_dev.h"
#include "vcap_dbg.h"
#include "vcap_input.h"
#include "vcap_mask.h"

int vcap_mask_set_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_mask_win_t *win)
{
    u32 tmp;
    int vi_prog;
    u16 x_start, y_start, x_end, y_end, width, height;
#ifdef VCAP_MASK_PARAM_CHECK
    u16 sc_w, sc_h;
#endif

    if((!win) || (win_idx >= VCAP_MASK_WIN_MAX)) {
        vcap_err("[%d] ch#%d set mask window failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

#ifdef VCAP_MASK_PARAM_CHECK
    /* check vi have init or not */
    if(GET_DEV_VI_GST(pdev_info, CH_TO_VI(ch)) != VCAP_DEV_STATUS_INITED) {
        vcap_err("[%d] ch#%d setup mask window#%d failed!(channel not initial)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    vi_prog = pdev_info->vi[CH_TO_VI(ch)].ch_param[SUBCH_TO_VICH(ch)].prog;
    sc_w    = pdev_info->ch[ch].param.sc.width;
    sc_h    = pdev_info->ch[ch].param.sc.height;
#else
    if(GET_DEV_VI_GST(pdev_info, CH_TO_VI(ch)) != VCAP_DEV_STATUS_INITED) { ///< vi not init, should get information form input module
        struct vcap_input_dev_t *pinput = vcap_input_get_info(CH_TO_VI(ch));

        if(pinput) {
            if(pinput->mode == VCAP_INPUT_MODE_2CH_BYTE_INTERLEAVE_HYBRID)
                vi_prog = pinput->ch_param[SUBCH_TO_VICH(ch)].prog;
            else {            
                if((pinput->interface == VCAP_INPUT_INTF_BT656_INTERLACE)     ||
                   (pinput->interface == VCAP_INPUT_INTF_BT1120_INTERLACE)    ||
                   (pinput->interface == VCAP_INPUT_INTF_SDI8BIT_INTERLACE)   ||
                   (pinput->interface == VCAP_INPUT_INTF_BT601_8BIT_INTERLACE)||
                   (pinput->interface == VCAP_INPUT_INTF_BT601_16BIT_INTERLACE)) {
                    vi_prog = VCAP_VI_PROG_INTERLACE;
                }
                else
                    vi_prog = VCAP_VI_PROG_PROGRESSIVE;
            }
        }
        else
            vi_prog = VCAP_VI_PROG_INTERLACE;   ///< no input device, default use interlace mode
    }
    else
        vi_prog = pdev_info->vi[CH_TO_VI(ch)].ch_param[SUBCH_TO_VICH(ch)].prog;
#endif

    x_start = ALIGN_2(win->x_start);
    width   = ALIGN_2(win->width);
    y_start = (vi_prog == VCAP_VI_PROG_INTERLACE) ? (win->y_start>>1) : win->y_start;
    height  = (vi_prog == VCAP_VI_PROG_INTERLACE) ? (win->height>>1)  : win->height;
    x_end   = ((x_start + width)  > 0) ? (x_start + width  - 1) : 0;
    y_end   = ((y_start + height) > 0) ? (y_start + height - 1) : 0;

#ifdef VCAP_MASK_PARAM_CHECK
    if((!width) || (!height) || ((x_start + width) > sc_w) || ((y_start + height) > sc_h)) {
        vcap_err("[%d] ch#%d setup mask window failed!(over range), SC(%d, %d) MASK_XY(%d, %d)MASK_SIZE(%d, %d)\n",
                 pdev_info->index, ch, sc_w, sc_h, win->x_start, win->y_start, win->width, win->height);
        return -1;
    }
#endif

    /* window x_start, x_end, color */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET(0x08*win_idx) : VCAP_CH_MASK_OFFSET(ch, 0x08*win_idx)));
    tmp &= ~0x0fffffff;
    tmp |= ((x_start & 0xfff) | ((win->color & 0xf)<<12) | (((x_end & 0xfff)<<16)));
    VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET(0x08*win_idx) : VCAP_CH_MASK_OFFSET(ch, 0x08*win_idx)), tmp);

    /* window y_start y_end */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET((0x4+(0x08*win_idx))) : VCAP_CH_MASK_OFFSET(ch, (0x4+(0x08*win_idx)))));
    tmp &= ~0x0fff0fff;
    tmp |= ((y_start & 0xfff) | ((y_end & 0xfff)<<16));
    VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET((0x4+(0x08*win_idx))) : VCAP_CH_MASK_OFFSET(ch, (0x4+(0x08*win_idx)))), tmp);

    return 0;
}

int vcap_mask_get_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_mask_win_t *win)
{
    u32 tmp;

    if((!win) || (win_idx >= VCAP_MASK_WIN_MAX)) {
        vcap_err("[%d] ch#%d get mask window failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    /* window x_start, x_end, color */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET(0x08*win_idx) : VCAP_CH_MASK_OFFSET(ch, 0x08*win_idx)));
    win->x_start = tmp & 0xfff;
    if((tmp>>16) & 0xfff)
        win->width = (((tmp>>16) & 0xfff) + 1) - win->x_start;
    else
        win->width = 0;
    win->color = (tmp>>12) & 0xf;

    /* window y_start, y_end */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET((0x4+(0x08*win_idx))) : VCAP_CH_MASK_OFFSET(ch, (0x4+(0x08*win_idx)))));
    win->y_start = tmp & 0xfff;
    if((tmp>>16) & 0xfff)
        win->height = (((tmp>>16) & 0xfff) + 1) - win->y_start;
    else
        win->height = 0;

    return 0;
}

int vcap_mask_set_alpha(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_MASK_ALPHA_T alpha)
{
    u32 tmp;

    if((win_idx >= VCAP_MASK_WIN_MAX) || (alpha >= VCAP_MASK_ALPHA_MAX)) {
        vcap_err("[%d] ch#%d set mask window alpha failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET(0x08*win_idx) : VCAP_CH_MASK_OFFSET(ch, 0x08*win_idx)));
    tmp &= ~(0x7<<28);
    tmp |= ((alpha & 0x7)<<28);
    VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET(0x08*win_idx) : VCAP_CH_MASK_OFFSET(ch, 0x08*win_idx)), tmp);

    return 0;
}

int vcap_mask_get_alpha(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_MASK_ALPHA_T *alpha)
{
    u32 tmp;

    if((win_idx >= VCAP_MASK_WIN_MAX) || !alpha) {
        vcap_err("[%d] ch#%d set mask window alpha failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET(0x08*win_idx) : VCAP_CH_MASK_OFFSET(ch, 0x08*win_idx)));
    *alpha = (tmp>>28) & 0x7;

    return 0;
}

int vcap_mask_set_border(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_mask_border_t *border)
{
#ifdef PLAT_MASK_WIN_HOLLOW
    u32 tmp;

    if((!border)                      ||
       (win_idx >= VCAP_MASK_WIN_MAX) ||
       (border->width > 0xf)          ||
       (border->type >= VCAP_MASK_BORDER_TYPE_MAX)) {
        vcap_err("[%d] ch#%d set mask window border failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    /* border type */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET(0x08*win_idx) : VCAP_CH_MASK_OFFSET(ch, 0x08*win_idx)));
    if(border->type == VCAP_MASK_BORDER_TYPE_TRUE)
        tmp |= BIT31;
    else
        tmp &= ~BIT31;
    VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET(0x08*win_idx) : VCAP_CH_MASK_OFFSET(ch, 0x08*win_idx)), tmp);

    /* border width */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET((0x4+(0x08*win_idx))) : VCAP_CH_MASK_OFFSET(ch, (0x4+(0x08*win_idx)))));
    tmp &= ~(0xf<<12);
    tmp |= ((border->width & 0xf)<<12);
    VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET((0x4+(0x08*win_idx))) : VCAP_CH_MASK_OFFSET(ch, (0x4+(0x08*win_idx)))), tmp);

    return 0;
#else
    vcap_err("[%d] ch#%d set mask window border failed!(hardware not support)\n", pdev_info->index, ch);
    return -1;
#endif
}

int vcap_mask_get_border(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_mask_border_t *border)
{
#ifdef PLAT_MASK_WIN_HOLLOW
    u32 tmp;

    if((!border) || (win_idx >= VCAP_MASK_WIN_MAX)) {
        vcap_err("[%d] ch#%d get mask window border failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    /* border type */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET(0x08*win_idx) : VCAP_CH_MASK_OFFSET(ch, 0x08*win_idx)));
    border->type = (tmp>>31) & 0x1;

    /* border width */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_MASK_OFFSET((0x4+(0x08*win_idx))) : VCAP_CH_MASK_OFFSET(ch, (0x4+(0x08*win_idx)))));
    border->width = (tmp>>12) & 0xf;
#else
    border->type  = VCAP_MASK_BORDER_TYPE_TRUE;
    border->width = 0;
#endif

    return 0;
}

int vcap_mask_enable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx)
{
    u32 tmp;
    unsigned long flags;

    if(win_idx >= VCAP_MASK_WIN_MAX) {
        vcap_err("[%d] enable ch#%d mask window#%d failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.comm.mask_en |= (0x1<<win_idx);      ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x04) : VCAP_CH_SUBCH_OFFSET(ch, 0x04)));
        tmp |= (0x1<<win_idx);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x04) : VCAP_CH_SUBCH_OFFSET(ch, 0x04)), tmp);
        pdev_info->ch[ch].temp_param.comm.mask_en |= (0x1<<win_idx);
        pdev_info->ch[ch].param.comm.mask_en      |= (0x1<<win_idx);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_mask_disable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx)
{
    u32 tmp;
    unsigned long flags;

    if(win_idx >= VCAP_MASK_WIN_MAX) {
        vcap_err("[%d] disable ch#%d mask window#%d failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags);

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.comm.mask_en &= ~(0x1<<win_idx);      ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x04) : VCAP_CH_SUBCH_OFFSET(ch, 0x04)));
        tmp &= ~(0x1<<win_idx);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x04) : VCAP_CH_SUBCH_OFFSET(ch, 0x04)), tmp);
        pdev_info->ch[ch].temp_param.comm.mask_en &= ~(0x1<<win_idx);
        pdev_info->ch[ch].param.comm.mask_en      &= ~(0x1<<win_idx);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}
