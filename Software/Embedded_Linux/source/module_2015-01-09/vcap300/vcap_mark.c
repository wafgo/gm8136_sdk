/**
 * @file vcap_mark.c
 *  vcap300 Mark Window Control Interface
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.5 $
 * $Date: 2014/09/05 02:48:53 $
 *
 * ChangeLog:
 *  $Log: vcap_mark.c,v $
 *  Revision 1.5  2014/09/05 02:48:53  jerson_l
 *  1. support osd force frame mode for GM8136
 *  2. support osd edge smooth edge mode for GM8136
 *  3. support osd auto color change scheme for GM8136
 *
 *  Revision 1.4  2013/05/10 05:45:43  jerson_l
 *  1. return align setting when get mark window config
 *
 *  Revision 1.3  2013/01/02 07:34:58  jerson_l
 *  1. support osd/mark window auto align feature
 *
 *  Revision 1.2  2012/12/18 11:55:12  jerson_l
 *  1. add get mark dimension api
 *
 *  Revision 1.1  2012/12/11 09:29:39  jerson_l
 *  1. add mark support
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
#include "vcap_mark.h"

struct vcap_mark_entity_t {
    u8              valid;  ///< indicate mark data valid or not
    u16             addr;   ///< mark start address int mark ram
    VCAP_MARK_DIM_T x_dim;  ///< mark x dimension
    VCAP_MARK_DIM_T y_dim;  ///< mark y dimension
};

struct vcap_mark_data_t {
    struct vcap_mark_entity_t mark[VCAP_MARK_IMG_MAX];
    struct semaphore          lock;
};

static struct vcap_mark_data_t vcap_mark_data[VCAP_DEV_MAX];

static inline struct vcap_mark_entity_t *vcap_mark_get_entity(struct vcap_dev_info_t *pdev_info, int mark_id)
{
    if(mark_id >= VCAP_MARK_IMG_MAX)
        return NULL;
    else
        return &vcap_mark_data[pdev_info->index].mark[mark_id];
}

int vcap_mark_get_info(struct vcap_dev_info_t *pdev_info, int mark_id, struct vcap_mark_info_t *mark_info)
{
    unsigned long flags;
    struct vcap_mark_entity_t *pmark;

    if(!mark_info || mark_id >= VCAP_MARK_IMG_MAX)
        return -1;

    spin_lock_irqsave(&pdev_info->lock, flags); ///< this api would be used in lli tasklet, need spin lock to protect vcap_mark_data

    pmark = &vcap_mark_data[pdev_info->index].mark[mark_id];
    if(pmark->valid) {
        mark_info->addr   = pmark->addr;
        mark_info->x_dim  = pmark->x_dim;
        mark_info->y_dim  = pmark->y_dim;
    }
    else {
        mark_info->addr = mark_info->x_dim = mark_info->y_dim = 0;
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_mark_init(struct vcap_dev_info_t *pdev_info)
{
    struct vcap_mark_data_t *pmark_data = &vcap_mark_data[pdev_info->index];

    /* clear mark data */
    memset(pmark_data, 0, sizeof(struct vcap_mark_data_t));

    /* init semaphore lock */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
    sema_init(&pmark_data->lock, 1);
#else
    init_MUTEX(&pmark_data->lock);
#endif

    return 0;
}

int vcap_mark_get_dim(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, int *x_dim, int *y_dim)
{
    u32 tmp;

    if((win_idx >= VCAP_MARK_WIN_MAX) || (!x_dim) || (!y_dim)) {
        vcap_err("[%d] ch#%d get mark dimension failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x88 + (win_idx*0x8))) : VCAP_CH_OSD_OFFSET(ch, (0x88 + (win_idx*0x8)))));
    *x_dim = (tmp>>12) & 0x7;
    *y_dim = (tmp>>28) & 0x7;

    return 0;
}

int vcap_mark_set_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_mark_win_t *win)
{
    int ret = 0;
    u32 tmp;
    unsigned long flags;
    struct vcap_mark_entity_t *mark_entity;

    if((!win)                               ||
       (win_idx      >= VCAP_MARK_WIN_MAX)  ||
       (win->align   >= VCAP_ALIGN_MAX)     ||
       (win->path    >= VCAP_SCALER_MAX)    ||
       (win->zoom    >= VCAP_MARK_ZOOM_MAX) ||
       (win->alpha   >= VCAP_MARK_ALPHA_MAX)||
       (win->mark_id >= VCAP_MARK_IMG_MAX)) {
        vcap_err("[%d] ch#%d set mark window failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    down(&vcap_mark_data[pdev_info->index].lock);   ///< protect vcap_mark_data

    /* get mark entity */
    mark_entity = vcap_mark_get_entity(pdev_info, win->mark_id);
    if(!mark_entity || !mark_entity->valid) {
        vcap_err("[%d] ch#%d set mark window failed!(mark entity#%d invalid)\n", pdev_info->index, ch, win->mark_id);
        ret = -1;
        goto end;
    }

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        spin_lock_irqsave(&pdev_info->lock, flags);

        /* window auto align parameter */
        pdev_info->ch[ch].temp_param.mark[win_idx].align_type     = win->align;
        pdev_info->ch[ch].temp_param.mark[win_idx].align_x_offset = ALIGN_4(win->x_start);
        pdev_info->ch[ch].temp_param.mark[win_idx].align_y_offset = win->y_start;
        pdev_info->ch[ch].temp_param.mark[win_idx].width          = ((mark_entity->x_dim >= 2) ? (0x1<<(mark_entity->x_dim+2)) : 64)*(0x1<<win->zoom);
        pdev_info->ch[ch].temp_param.mark[win_idx].height         = ((mark_entity->y_dim >= 2) ? (0x1<<(mark_entity->y_dim+2)) : 64)*(0x1<<win->zoom);

        /*
         * x_start, y_start will be re-caculate in lli tasklet if auto align enable.
         * default set mark window to none align position.
         */
        pdev_info->ch[ch].temp_param.mark[win_idx].x_start = ALIGN_4(win->x_start) & 0xfff;
        pdev_info->ch[ch].temp_param.mark[win_idx].y_start = win->y_start & 0xfff;

        pdev_info->ch[ch].temp_param.mark[win_idx].addr  = (mark_entity->addr>>3) & 0xfff;
        pdev_info->ch[ch].temp_param.mark[win_idx].x_dim = mark_entity->x_dim;
        pdev_info->ch[ch].temp_param.mark[win_idx].y_dim = mark_entity->y_dim;
        pdev_info->ch[ch].temp_param.mark[win_idx].alpha = win->alpha;
        pdev_info->ch[ch].temp_param.mark[win_idx].zoom  = win->zoom;
        pdev_info->ch[ch].temp_param.mark[win_idx].path  = win->path;
        pdev_info->ch[ch].temp_param.mark[win_idx].id    = win->mark_id;

        spin_unlock_irqrestore(&pdev_info->lock, flags);
    }
    else {
        /* X, Y Position and Dimention */
        tmp = (ALIGN_4(win->x_start & 0xfff)) |
              (mark_entity->x_dim<<12)        |
              ((win->y_start & 0xfff)<<16)    |
              (mark_entity->y_dim<<28);
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x88 + (win_idx*0x8))) : VCAP_CH_OSD_OFFSET(ch, (0x88 + (win_idx*0x8)))), tmp);

        /* Alpha, Zoom, Start_Addr, Task Select */
        tmp = (win->path<<30) | (win->zoom<<28) | (((mark_entity->addr>>3) & 0xfff)<<16) | win->alpha;
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x8c + (win_idx*0x8))) : VCAP_CH_OSD_OFFSET(ch, (0x8c + (win_idx*0x8)))), tmp);

        spin_lock_irqsave(&pdev_info->lock, flags);

        /* window auto align parameter */
        pdev_info->ch[ch].temp_param.mark[win_idx].align_type     = pdev_info->ch[ch].param.mark[win_idx].align_type     = win->align;
        pdev_info->ch[ch].temp_param.mark[win_idx].align_x_offset = pdev_info->ch[ch].param.mark[win_idx].align_x_offset = ALIGN_4(win->x_start);
        pdev_info->ch[ch].temp_param.mark[win_idx].align_y_offset = pdev_info->ch[ch].param.mark[win_idx].align_y_offset = win->y_start;
        pdev_info->ch[ch].temp_param.mark[win_idx].width          = pdev_info->ch[ch].param.mark[win_idx].width          = ((mark_entity->x_dim >= 2) ? (0x1<<(mark_entity->x_dim+2)) : 64)*(0x1<<win->zoom);
        pdev_info->ch[ch].temp_param.mark[win_idx].height         = pdev_info->ch[ch].param.mark[win_idx].height         = ((mark_entity->y_dim >= 2) ? (0x1<<(mark_entity->y_dim+2)) : 64)*(0x1<<win->zoom);

        /*
         * x_start, y_start will be re-caculate in lli tasklet if auto align enable.
         * default set mark window to none align position.
         */
        pdev_info->ch[ch].temp_param.mark[win_idx].x_start = pdev_info->ch[ch].param.mark[win_idx].x_start = ALIGN_4(win->x_start) & 0xfff;
        pdev_info->ch[ch].temp_param.mark[win_idx].y_start = pdev_info->ch[ch].param.mark[win_idx].y_start = win->y_start & 0xfff;

        pdev_info->ch[ch].temp_param.mark[win_idx].addr  = pdev_info->ch[ch].param.mark[win_idx].addr  = (mark_entity->addr>>3) & 0xfff;
        pdev_info->ch[ch].temp_param.mark[win_idx].x_dim = pdev_info->ch[ch].param.mark[win_idx].x_dim = mark_entity->x_dim;
        pdev_info->ch[ch].temp_param.mark[win_idx].y_dim = pdev_info->ch[ch].param.mark[win_idx].y_dim = mark_entity->y_dim;
        pdev_info->ch[ch].temp_param.mark[win_idx].alpha = pdev_info->ch[ch].param.mark[win_idx].alpha = win->alpha;
        pdev_info->ch[ch].temp_param.mark[win_idx].zoom  = pdev_info->ch[ch].param.mark[win_idx].zoom  = win->zoom;
        pdev_info->ch[ch].temp_param.mark[win_idx].path  = pdev_info->ch[ch].param.mark[win_idx].path  = win->path;
        pdev_info->ch[ch].temp_param.mark[win_idx].id    = pdev_info->ch[ch].param.mark[win_idx].id    = win->mark_id;

        spin_unlock_irqrestore(&pdev_info->lock, flags);
    }

end:
    up(&vcap_mark_data[pdev_info->index].lock);

    return 0;
}

int vcap_mark_get_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_mark_win_t *win)
{
    int i;
    u32 tmp;
    u16 start_addr;
    struct vcap_mark_entity_t *mark_entity;

    if(!win || win_idx >= VCAP_MARK_WIN_MAX) {
        vcap_err("[%d] ch#%d get mark window failed!(invalid parameter)\n", pdev_info->index, ch);
        return -1;
    }

    /* Align */
    win->align = pdev_info->ch[ch].param.mark[win_idx].align_type;

    /* X, Y Position */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x88 + (win_idx*0x8))) : VCAP_CH_OSD_OFFSET(ch, (0x88 + (win_idx*0x8)))));
    win->x_start = tmp & 0xfff;
    win->y_start = (tmp>>16) & 0xfff;

    /* Alpha, Zoom, Start_Addr, Task Select */
    tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_OSD_OFFSET((0x8c + (win_idx*0x8))) : VCAP_CH_OSD_OFFSET(ch, (0x8c + (win_idx*0x8)))));
    win->path    = (tmp>>30) & 0x3;
    win->alpha   = tmp & 0x7;
    win->zoom    = (tmp>>28) & 0x3;
    win->mark_id = 0;
    start_addr   = (tmp>>16) & 0xfff;

    /* find mark id */
    down(&vcap_mark_data[pdev_info->index].lock);   ///< protect vcap_mark_data
    for(i=0; i<VCAP_MARK_IMG_MAX; i++) {
        mark_entity = vcap_mark_get_entity(pdev_info, i);
        if(mark_entity && mark_entity->valid && (((mark_entity->addr>>3) & 0xfff) == start_addr)) {
            win->mark_id = i;
            break;
        }
    }
    up(&vcap_mark_data[pdev_info->index].lock);

    return 0;
}

int vcap_mark_load_image(struct vcap_dev_info_t *pdev_info, struct vcap_mark_img_t *img)
{
    u32 i;
    u32 start_addr, img_size;
    u32 *img_data;
    u32 mark_mem_size, addr_offset = 0;
    u16 dim_size[8] = {64, 64, 16, 32, 64, 128, 256, 512};

    if(!img || !img->img_buf) {
        vcap_err("[%d] load mark image failed!(invalid parameter)\n", pdev_info->index);
        return -1;
    }

    if(img->id >= VCAP_MARK_IMG_MAX) {
        vcap_err("[%d] load mark image failed!(mark id#%d over rang, max: %d)\n", pdev_info->index, img->id, VCAP_MARK_IMG_MAX);
        return -1;
    }

    if(img->x_dim >= VCAP_MARK_DIM_MAX || img->y_dim >= VCAP_MARK_DIM_MAX) {
        vcap_err("[%d] load mark image failed!(invalid image dimension)\n", pdev_info->index);
        return -1;
    }

    start_addr = ALIGN_4(img->start_addr);
    img_size   = dim_size[img->x_dim]*dim_size[img->y_dim]*2;   ///YUV422
    img_data   = (u32 *)img->img_buf;

#ifdef PLAT_OSD_COLOR_SCHEME
    switch((pdev_info->m_param)->accs_func) {
        case VCAP_ACCS_FUNC_HALF_MARK_MEM:
            mark_mem_size = addr_offset = VCAP_MARK_MEM_SIZE>>1;
            break;
        case VCAP_ACCS_FUNC_FULL_MARK_MEM:
            mark_mem_size = 0;
            break;
        default:
            mark_mem_size = VCAP_MARK_MEM_SIZE;
            break;
    }
#else
    mark_mem_size = VCAP_MARK_MEM_SIZE;
#endif

    if((start_addr + img_size) > mark_mem_size) {
        vcap_err("[%d] load mark image[start: 0x%04x, size: %u] over range!(MARK_RAM_SIZE: %u BYTE)\n",
                 pdev_info->index, start_addr, img_size, mark_mem_size);
        return -1;
    }

    start_addr += addr_offset;

    /* load mark image to mark ram */
    for(i=0; i<(img_size/4); i++) {
        VCAP_REG_WR(pdev_info, VCAP_MARK_MEM_OFFSET(start_addr + (i*4)), img_data[i]);
    }

    /* update mark entity to mark database */
    down(&vcap_mark_data[pdev_info->index].lock);   ///< protect vcap_mark_data
    vcap_mark_data[pdev_info->index].mark[img->id].addr  = start_addr;
    vcap_mark_data[pdev_info->index].mark[img->id].x_dim = img->x_dim;
    vcap_mark_data[pdev_info->index].mark[img->id].y_dim = img->y_dim;
    vcap_mark_data[pdev_info->index].mark[img->id].valid = 1;
    up(&vcap_mark_data[pdev_info->index].lock);

    return 0;
}

int vcap_mark_win_enable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx)
{
    u32 tmp;
    unsigned long flags;

    if(win_idx >= VCAP_MARK_WIN_MAX) {
        vcap_err("[%d] ch#%d enable mark window#%d failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags); ///< use spin lock because temp_param would be access in lli tasklet

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.comm.mark_en |= (0x1<<win_idx);    ///< lli tasklet will apply this config and sync to current
    }
    else {
        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
        tmp |= (0x1<<(8+win_idx));
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)), tmp);
        pdev_info->ch[ch].param.comm.mark_en      |= (0x1<<win_idx);
        pdev_info->ch[ch].temp_param.comm.mark_en |= (0x1<<win_idx);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}

int vcap_mark_win_disable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx)
{
    unsigned long flags;

    if(win_idx >= VCAP_MARK_WIN_MAX) {
        vcap_err("[%d] ch#%d disable mark window#%d failed!(invalid parameter)\n", pdev_info->index, ch, win_idx);
        return -1;
    }

    spin_lock_irqsave(&pdev_info->lock, flags); ///< use spin lock because temp_param would be access in lli tasklet

    if(VCAP_IS_CH_ENABLE(pdev_info, ch)) {
        pdev_info->ch[ch].temp_param.comm.mark_en &= ~(0x1<<win_idx);    ///< lli tasklet will apply this config and sync to current
    }
    else {
        u32 tmp;

        tmp = VCAP_REG_RD(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)));
        tmp &= ~(0x1<<(8+win_idx));
        VCAP_REG_WR(pdev_info, ((ch == VCAP_CASCADE_CH_NUM) ? VCAP_CAS_SUBCH_OFFSET(0x08) : VCAP_CH_SUBCH_OFFSET(ch, 0x08)), tmp);
        pdev_info->ch[ch].param.comm.mark_en      &= ~(0x1<<win_idx);
        pdev_info->ch[ch].temp_param.comm.mark_en &= ~(0x1<<win_idx);
    }

    spin_unlock_irqrestore(&pdev_info->lock, flags);

    return 0;
}
