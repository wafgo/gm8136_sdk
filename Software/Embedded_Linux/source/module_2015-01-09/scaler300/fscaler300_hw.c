#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/hdreg.h>	/* HDIO_GETGEO */
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/proc_fs.h>
#include <linux/kfifo.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/synclink.h>
#include <linux/time.h>

#include "fscaler300.h"
#include "common.h"

#ifdef VIDEOGRAPH_INC
#include "fscaler300_vg.h"
#include <mach/gm_jiffies.h>
#include <log.h>    //include log system "printm","damnit"...
#endif
#define I_AM_EP(x)    ((x) & (0x1 << 5))
extern fscaler300_param_t dummy;
extern int use_ioremap_wc;
extern int flow_check;
/*
 * Job finish, called from ISR
 */
int fscaler300_job_finish(int dev, fscaler300_job_t *job)
{
    int schedule = 1;
    int mpath, i;
    fscaler300_job_t *parent;
    fscaler300_job_t *curr, *ne;

    //scl_dbg("%s %d\n", __func__, job->job_id);
    /* get parent job */
    if (list_empty(&job->clist))
        parent = job;
    else
        parent = list_entry(job->clist.next, fscaler300_job_t, clist);

    //measure function
    if (parent->job_type & SINGLE_JOB) {    // SINGLE_JOB, VIRTUAL_JOB
        if (parent->input_node[0].fops && parent->input_node[0].fops->post_process)
            parent->input_node[0].fops->post_process(parent);
    }
    else if (parent->job_type == VIRTUAL_MJOB) {
        if (job->need_callback == 1) {
            if (parent->input_node[0].fops && parent->input_node[0].fops->post_process)
                parent->input_node[0].fops->post_process(parent);
        }
    }
    else {                                  // MULTI_DJOB, MULTI_SJOB
        if (parent->job_type == MULTI_SJOB)
            mpath = 4;
        else
            mpath = 1;
        /* parent job */
        if (parent->job_type == TYPE_ALL || parent->job_type == TYPE_LAST) {
            for (i = 0; i < mpath; i++) {
                if (parent->input_node[i].fops && parent->input_node[i].fops->post_process)
                    parent->input_node[i].fops->post_process(parent);
            }
        }
        /* child job */
        list_for_each_entry_safe(curr, ne, &parent->clist, clist) {
            if (curr->job_type == TYPE_ALL || curr->job_type == TYPE_LAST) {
                for (i = 0; i < mpath; i++) {
                    if (curr->input_node[i].fops && curr->input_node[i].fops->post_process)
                        curr->input_node[i].fops->post_process(curr);
                }
            }
        }
    }

    fscaler300_job_free(dev, job, schedule);

    return 0;
}

/*
 * fscaler300 ISR
 */
irqreturn_t fscaler300_interrupt(int irq, void *devid)
{
    int dev = (int)devid;
    int base = 0, addr = 0, i = 0;
    fscaler300_job_t    *job, *ne, *last_job;
    int status0 = 0, status1 = 0;
    int reg_0x55a8, reg_0x55b0, reg_0x55b8, frame_cnt;
    int wjob_cnt = 0;//sjob_cnt = 0;
#if GM8210
    job_node_t *node;
    int ret = 0;
#endif
#ifdef SINGLE_FIRE
    int reg_0x55c4;
#endif
    //printk(KERN_DEBUG"**IRQ**\n");
    base = priv.engine[dev].fscaler300_reg;

    /* Status */
    // dma_ovf, dma_job_ovf, dma_dis_status, dma_wc_respfail, dma_rc_respfail, sd_job_ovf, dma_wc_pfx_err
    reg_0x55a8 = *(volatile unsigned int *)(base + 0x55a8);
    // ll_done
    reg_0x55b0 = *(volatile unsigned int *)(base + 0x55b0);
    // sd_fail
    reg_0x55b8 = *(volatile unsigned int *)(base + 0x55b8);
    // frame count
    frame_cnt = *(volatile unsigned int *)(base + 0x55d8) & 0xffff;
#ifdef SINGLE_FIRE
    reg_0x55c4 = *(volatile unsigned int *)(base + 0x55c4);
#endif
    // clear status
    *(volatile unsigned int *)(base + 0x55a8) = 0xfffffff;
    *(volatile unsigned int *)(base + 0x55b0) = 0xfffffff;
    *(volatile unsigned int *)(base + 0x55b8) = 0xfffffff;
    // read back last write register for NCB
    if (use_ioremap_wc == 1)
        status0 = *(volatile unsigned int *)(base + 0x55b8);
#ifdef SINGLE_FIRE
    *(volatile unsigned int *)(base + 0x55c4) = 0xfffffff;
#endif
    /* Check Error Status */
    if(reg_0x55a8 & 0x1)
        printk("[SCALER300_IRQ] DMA overflow!!(%d)\n", frame_cnt);

    if(reg_0x55a8 & 0x2)
        printk("[SCALER300_IRQ] DMA job count overflow!!(%d)\n", frame_cnt);

    if(reg_0x55a8 & 0x4)
        printk("[SCALER300_IRQ] DMA Disable!!(%u)\n", frame_cnt);

    if(reg_0x55a8 & 0x8)
        printk("[SCALER300_IRQ] DMA write channel response fail!!(%d)\n", frame_cnt);

    if(reg_0x55a8 & 0x10)
        printk("[SCALER300_IRQ] DMA read channel response fail!!(%d)\n", frame_cnt);

    if(reg_0x55a8 & 0x20)
        printk("[SCALER300_IRQ] SD job count overflow!!(%d)\n", frame_cnt);

    if(reg_0x55a8 & 0x40)
        printk("[SCALER300_IRQ] DMA command prefix error!!(%d)\n", frame_cnt);

    if(reg_0x55b8 & 0x1)
        printk("[SCALER300_IRQ] SD process fail!!(%d)\n", frame_cnt);

    //if(reg_0x55b0 & 0x1)
      //  printk(KERN_DEBUG "[SCALER300_IRQ] LL Done (%d)\n", frame_cnt);
#ifdef SINGLE_FIRE
    if(reg_0x55c4 & BIT31)
        printk(KERN_DEBUG "[SCALER300_IRQ] frame end (%d)\n", frame_cnt);
#endif
    /* if this interrupt is dummy job interrupt, don't need to read link list memory  */
    if (list_empty(&priv.engine[dev].wlist))
        goto exit;

    // find out which jobs have done, callback to V.G
    list_for_each_entry_safe(job, ne, &priv.engine[dev].wlist, plist) {
        int res_rdy = 0;
        if (job->status == JOB_STS_DONE)
            continue;
        if (job->job_cnt > 1)
            last_job = list_entry(job->clist.prev, fscaler300_job_t, clist);
        else
            last_job = job;

        addr = last_job->ll_blk.status_addr;
        status0 = *(volatile unsigned int *)(base + SCALER300_LINK_LIST_MEM_BASE + last_job->ll_blk.status_addr);
        status1 = *(volatile unsigned int *)(base + SCALER300_LINK_LIST_MEM_BASE + last_job->ll_blk.status_addr + 4);

        if ((status1 & 0x20000000) != 0x20000000) // not status command
            printk("[SCALER300_IRQ] it's not status command, something wrong !!\n");
        else {
            status1 &= 0x1ffff;

            for (i = 0; i < SD_MAX; i++) {
                if (last_job->param.sd[i].enable == 1)
                    res_rdy += (0x10 << i);
            }
            if ((status1 & 0x1) == 0x1) {
                if ((status1 & res_rdy) == res_rdy) {
                    //printk(KERN_DEBUG"id [%d] sts0 [%x] sts1 [%x] addr [%x]\n", job->job_id, status0, status1, job->ll_blk.status_addr);
                    if (flow_check)printm("SL", "[IRQ] id [%u] cnt %d\n", job->job_id, job->job_cnt);
                    job->status = JOB_STS_DONE;
#if GM8210
                    node = (job_node_t *)job->input_node[0].private;
                    if (I_AM_EP(node->property.vproperty[node->ch_id].scl_feature)) {
                        ret = scl_dma_trigger(node->job_id);
                        if (ret < 0) {
                            scl_err("[SL]  id %u trigger DMA fail\n", node->job_id);
                            printm("SL", "[SL] trigger DMA fail\n");
                            damnit("SL");
                        }
                    }
#endif
                    // callback the job
                    fscaler300_job_finish(dev, last_job);
                    mod_timer(&priv.engine[dev].timer, jiffies + priv.engine[dev].timeout);

                    // check error status
                    if (status0 & 0x1000000) {
                        printk("[SCALER300_IRQ] sd fail");
                        if (status1 & 0x100)
                            printk("[SCALER300_IRQ] sd timeout\n");
                        if (status1 & 0x2000)
                            printk("[SCALER300_IRQ] sd prefix decode err\n");
                        if (status1 & 0x4000)
                            printk("[SCALER300_IRQ] sd param err\n");
                        if (status1 & 0x8000)
                            printk("[SCALER300_IRQ] sd mem ovf");
                        if (status1 & 0x10000)
                            printk("[SCALER300_IRQ] sd job ovf");
                    }
                    if (status0 & 0x2000000)
                        printk("[SCALER300_IRQ] sd job ovf");
                    if (status0 & 0x4000000)
                        printk("[SCALER300_IRQ] dma ovf");
                    if (status0 & 0x8000000)
                        printk("[SCALER300_IRQ] dma job ovf");
                }
            }
            else if (status1 == 0x0) { // job not yet do
                //printk(KERN_DEBUG "[SCALER300_IRQ] job not yet do [%d]\n", job->job_id);
                break;
            }
            else {    // here means this job some resX_rdy = 1, some resX_rdy != 1
                printk("[SCALER300_IRQ] some res_rdy not done, something wrong\n");
                break;
            }
        }
    }
    // find out how many jobs still in working list & chained list
    list_for_each_entry_safe(job, ne, &priv.engine[dev].wlist, plist) {
        if (job->status != JOB_STS_DONE)
            wjob_cnt++;
    }

    //list_for_each_entry_safe(job, ne, &priv.engine[dev].slist, plist)
      //  sjob_cnt++;

    // no jobs in working list, HW idle
    if (wjob_cnt == 0) {
        //printk(KERN_DEBUG"[IRQ]unlock [%d]\n", sjob_cnt);
        UNLOCK_ENGINE(dev);
        mark_engine_finish(dev);
        del_timer(&priv.engine[dev].timer);
    }

#ifdef SINGLE_FIRE
    if (!list_empty(&priv.engine[dev].wlist)) {
        last_job = list_entry(priv.engine[dev].wlist.next, fscaler300_job_t, plist);
        last_job->status = JOB_STS_DONE;
        // callback the job
        fscaler300_job_finish(dev, last_job);
        UNLOCK_ENGINE(dev);
    }
    /* mask frame end interrupt mask bit */
    *(volatile unsigned int *)(base + 0x5544) = 0xffffffff;
#endif
exit:
    return IRQ_HANDLED;
}

void fscaler300_sw_reset(int dev)
{
    int base = 0;
    int tmp = 0;
    int cnt = 10;

    base = priv.engine[dev].fscaler300_reg;

    /* disable DMA */
    tmp = *(volatile unsigned int *)(base + 0x5400);
    tmp |= BIT31;
    *(volatile unsigned int *)(base + 0x5400) = tmp;

    /* polling DMA disable status */
    while (((*(volatile unsigned int *)(base + 0x55a8) & BIT2) != BIT2) && (cnt-- > 0))
        udelay(5);

    if ((*(volatile unsigned int *)(base + 0x55a8)& BIT2) != BIT2)
        printk("DMA disable failed!!\n");
    else
        *(volatile unsigned int *)(base + 0x55a8) = BIT2;

    /* Clear fire bit and disable capture */
    tmp = *(volatile unsigned int *)(base + 0x5404);
    tmp &= ~(BIT28|BIT31);
    *(volatile unsigned int *)(base + 0x5404) = tmp;

    /* set software reset */
    tmp = *(volatile unsigned int *)(base + 0x5404);
    tmp |= BIT0;
    *(volatile unsigned int *)(base + 0x5404) = tmp;

    udelay(5);

    /* free software reset */
    tmp &= ~BIT0;
    *(volatile unsigned int *)(base + 0x5404) = tmp;

    /* Clear Status */
    *(volatile unsigned int *)(base + 0x55a8) = BIT2;
    // read back last write register for NCB
    if (use_ioremap_wc == 1)
        tmp = *(volatile unsigned int *)(base + 0x55a8);

    printk("SCALER300 SW RESET\n");
}

int fscaler300_single_fire(int dev)
{
    int tmp = 0;
    int base = 0;
    unsigned long flags;

    base = priv.engine[dev].fscaler300_reg;

    /* trigger timer */
    spin_lock_irqsave(&priv.lock, flags);
    mod_timer(&priv.engine[dev].timer, jiffies + priv.engine[dev].timeout);
    spin_unlock_irqrestore(&priv.lock, flags);

    tmp = *(volatile unsigned int *)(base + 0x5404);

    if (tmp & 0x10000000) {
        printk("Fire Bit not clear!\n");
        return -1;
    }
    else {
        tmp |= (1 << 28);
        *(volatile unsigned int *)(base + 0x5404) = tmp;
    }
    return 0;
}

void fscaler300_set_osd_temp(int chn, int pp_sel, int win_idx, int path, temp_osd_t *temp_osd, fscaler300_job_t *job)
{
    u16 width = 0, height = 0;
    u16 x_start = 0, y_start = 0;

    width = priv.global_param.chn_global[chn].osd[pp_sel][win_idx].x_end -
            priv.global_param.chn_global[chn].osd[pp_sel][win_idx].x_start + 1;

    height = priv.global_param.chn_global[chn].osd[pp_sel][win_idx].y_end -
            priv.global_param.chn_global[chn].osd[pp_sel][win_idx].y_start + 1;

    x_start = priv.global_param.chn_global[chn].osd[pp_sel][win_idx].x_start;
    y_start = priv.global_param.chn_global[chn].osd[pp_sel][win_idx].y_start;

    /* Window Position Auto Align Ajustment */
    switch (priv.global_param.chn_global[chn].osd[pp_sel][win_idx].align_type) {
        case SCALER_ALIGN_TOP_CENTER:
            if (((job->param.sd[path].width / 2) - (width / 2)) > 0)
                temp_osd->x_start = ALIGN_4((job->param.sd[path].width / 2) - (width / 2));
            else
                temp_osd->x_start = 0;

            temp_osd->y_start = priv.global_param.chn_global[chn].osd[pp_sel][win_idx].y_start;
            break;
        case SCALER_ALIGN_TOP_RIGHT:
            if ((job->param.sd[path].width - width - x_start) > 0)
                temp_osd->x_start = ALIGN_4(job->param.sd[path].width - width - x_start);
            else
                temp_osd->x_start = 0;

            temp_osd->y_start = y_start;
            break;
        case SCALER_ALIGN_BOTTOM_LEFT:
            temp_osd->x_start = x_start;

            if ((job->param.sd[path].height - height - y_start) > 0)
                temp_osd->y_start = job->param.sd[path].height - height - y_start;
            else
                temp_osd->y_start = 0;
            break;
        case SCALER_ALIGN_BOTTOM_CENTER:
            if (((job->param.sd[path].width / 2) - (width / 2)) > 0)
                temp_osd->x_start = ALIGN_4((job->param.sd[path].width / 2) - (width / 2));
            else
                temp_osd->x_start = 0;

            if ((job->param.sd[path].height - height - y_start) > 0)
                temp_osd->y_start = job->param.sd[path].height - height - y_start;
            else
                temp_osd->y_start = 0;
            break;
        case SCALER_ALIGN_BOTTOM_RIGHT:
            if ((job->param.sd[path].width - width - x_start) > 0)
                temp_osd->x_start = ALIGN_4(job->param.sd[path].width - width - x_start);
            else
                temp_osd->x_start = 0;

            if ((job->param.sd[path].height - height - y_start) > 0)
                temp_osd->y_start = job->param.sd[path].height - height - y_start;
            else
                temp_osd->y_start = 0;
            break;
        case SCALER_ALIGN_CENTER:
            if (((job->param.sd[path].width / 2) - (width / 2)) > 0)
                temp_osd->x_start = ALIGN_4((job->param.sd[path].width / 2) - (width / 2));
            else
                temp_osd->x_start = 0;

            if (((job->param.sd[path].height / 2) - (height / 2)) > 0)
                temp_osd->y_start = (job->param.sd[path].height / 2) - (height / 2);
            else
                temp_osd->y_start = 0;
            break;
        default:    /* SCALER_ALIGN_NONE or SCALER_ALIGN_TOP_LEFT*/
            temp_osd->x_start = x_start;
            temp_osd->y_start = y_start;
            break;
    }

    if ((temp_osd->x_start + width - 1) > 0)
        temp_osd->x_end = temp_osd->x_start + width - 1;
    else
        temp_osd->x_end = 0;

    if ((temp_osd->y_start + height - 1) > 0)
        temp_osd->y_end = temp_osd->y_start + height - 1;
    else
        temp_osd->y_end = 0;

    temp_osd->x_start &= 0xfff;
    temp_osd->y_start &= 0xfff;
    temp_osd->x_end   &= 0xfff;
    temp_osd->y_end   &= 0xfff;
}

// subchannel, sd0 , sc , tc , global, dma, ll, mask0~3
u32 fscaler300_lli_blk0_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last)
{
    int i, mpath, win_idx;
#if GM8210
    int chn = 0, pp_sel;
#endif
    u32 base = 0, addr = 0, tmp = 0;
    u32 next_cmd_addr = 0;
    fscaler300_global_param_t *global_param = &priv.global_param;

    //scl_dbg("%s [%d][%x]\n", __func__, job->job_id, offset);

    if (job->job_type == MULTI_SJOB)
        mpath = 4;
    else
        mpath = 1;

    //measure function
    if (job->perf_type == TYPE_ALL || job->perf_type == TYPE_FIRST) {
        i = 0;
        if (job->input_node[i].fops && job->input_node[i].fops->mark_engine_start)
            job->input_node[i].fops->mark_engine_start(dev, job);
        //if (job->input_node[i].fops && job->input_node[i].fops->pre_process)
          //  job->input_node[i].fops->pre_process(job);
    }

    base = priv.engine[dev].fscaler300_reg + SCALER300_LINK_LIST_MEM_BASE;

    next_cmd_addr = next;
    // status
    *(volatile unsigned int *)(base + offset) = 0x00000000;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x20000000;

    offset += 8;
// SUBCHANNEL (5)
    // sca0~3 enable/bypass, tc0~3 enable 0x0
#if GM8210
    tmp = (job->param.img_border.border_en[0] << 3) |
          (job->param.frm2field.res0_frm2field << 5) |
          (job->param.img_border.border_en[1] << 11) |
          (job->param.frm2field.res1_frm2field << 13) |
          (job->param.img_border.border_en[2] << 19) |
          (job->param.frm2field.res2_frm2field << 21) |
          (job->param.img_border.border_en[3] << 27) |
          (job->param.frm2field.res3_frm2field << 29) ;
#else
    tmp = (job->param.frm2field.res0_frm2field << 5) |
          (job->param.frm2field.res1_frm2field << 13) |
          (job->param.frm2field.res2_frm2field << 21) |
          (job->param.frm2field.res3_frm2field << 29);
#endif
    for (i = 0; i < 4; i++) {
        if (job->param.sd[i].bypass)
            tmp |= (0x1 << (i * 0x8));

        if (job->param.sd[i].enable)
            tmp |= (0x1 << (i * 0x8 + 1));

        if (job->param.tc[i].enable)
            tmp |= (0x1 << (i * 0x8 + 2));
    }
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40780000;
    offset += 8;
    // mask 0~7 enable 0x4
    tmp = 0;
    if (job->param.sc.fix_pat_en) {
        for (win_idx = 0; win_idx < MASK_MAX; win_idx++) {
            if (job->param.mask[win_idx].enable)
                tmp |= (0x1 << win_idx);
        }
    }
#if 0   /* mask & osd external api do not use, remove it for performance */
    else {
        for (i = 0; i < 4; i++) {
            if (!job->input_node[i].enable)
                continue;

            chn = job->input_node[i].minor;
            for (win_idx = 0; win_idx < MASK_MAX; win_idx++) {
                if (!job->mask_pp_sel[i][win_idx])
                    continue;

                pp_sel = job->mask_pp_sel[i][win_idx] - 1;
                if (global_param->chn_global[chn].mask[pp_sel][win_idx].enable)
                    tmp |= (0x1 << win_idx);
            }
        }
    }
#endif
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40080004;
    offset += 8;
    // osd0~7 enable 0x8
    tmp = (job->param.mirror.h_flip << 28) |
          (job->param.mirror.v_flip << 29);
#if GM8210
    if (job->param.scl_feature & (1 << 0)) {
        for (i = 0; i < mpath; i++) {
            if (!job->input_node[i].enable)
                continue;

            chn = job->input_node[i].minor;
            for (win_idx = 0; win_idx < OSD_MAX; win_idx++) {
                if (!job->osd_pp_sel[i][win_idx])
                    continue;

                pp_sel = job->osd_pp_sel[i][win_idx] - 1;
                if (global_param->chn_global[chn].osd[pp_sel][win_idx].enable)
                    tmp |= (0x1 << win_idx);
            }
        }
    }
#else
    tmp |= ((job->param.img_border.border_en[0] << 12) |
            (job->param.img_border.border_en[1] << 13) |
            (job->param.img_border.border_en[2] << 14) |
            (job->param.img_border.border_en[3] << 15));
#endif
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40780008;
    offset += 8;
    // sd0 width/height 0x10
    tmp = (job->param.sd[0].width) | (job->param.sd[0].height << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40780010;
    offset += 8;
    // tc0 width 0x20
    tmp = job->param.tc[0].width;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40180020;
    offset += 8;
// SD (2)
    // sd0 hstep/vstep 0x78
    if (job->param.sc.frame_type == PROGRESSIVE) {
    tmp = (job->param.sd[0].hstep) | (job->param.sd[0].vstep << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40780078;
    offset += 8;
    }
    // sd0 topvoft/botvoft 0x7c
    if (job->param.sc.frame_type != PROGRESSIVE) {
        tmp = (job->param.sd[0].topvoft) | (job->param.sd[0].botvoft << 13) |
              (global_param->init.smooth[0].hsmo_str << 26) |
              (global_param->init.smooth[0].vsmo_str << 29);
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = 0x4078007c;
        offset += 8;
    }
// img border color
    if (job->param.tc[0].enable || job->param.img_border.border_en[0]) {
        tmp = job->param.img_border.color << 12;
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = 0x401000b8;
        offset += 8;
    }

// TC (2)
    // tc0 0x160
    if (job->param.tc[0].enable || job->param.img_border.border_en[0]) {
        tmp = (job->param.tc[0].x_start) | (job->param.tc[0].x_end << 16);
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = 0x40780160;
        offset += 8;
        // tc0 0x164
        tmp = (job->param.tc[0].y_start) | (job->param.tc[0].y_end << 16) |
              (job->param.img_border.border_width[0] << 13);
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = 0x40780164;
        offset += 8;
    }
// DMA (4)
    // dma0 0x180
    if (job->param.dest_dma[0].addr == 0x82878287) {
        job->param.dest_dma[0].addr = priv.engine[dev].temp_buf.paddr;
    }
    // destination to pip buffer
    if (job->param.dest_dma[0].addr == 0x11) {
        job->param.dest_dma[0].addr = priv.engine[dev].temp_buf.paddr;
        job->param.dest_dma[0].addr += ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    }
    // destination to pip1 buffer
    if (job->param.dest_dma[0].addr == 0x12) {
        job->param.dest_dma[0].addr = priv.engine[dev].pip1_buf.paddr;
        job->param.dest_dma[0].addr += ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    }
    tmp = job->param.dest_dma[0].addr;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40780180;
    offset += 8;
    // dma0 0x184
    tmp = (job->param.dest_dma[0].line_offset) | (job->param.dest_dma[0].field_offset << 13);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40780184;
    offset += 8;
    // src dma 0x1a0
    if (job->param.src_dma.addr == 0x82878287) {
        job->param.src_dma.addr = priv.engine[dev].temp_buf.paddr;
    }
    // source from pip buffer
    if (job->param.src_dma.addr == 0x11) {
        job->param.src_dma.addr = priv.engine[dev].temp_buf.paddr;
        job->param.src_dma.addr += ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    }
    // source from pip1 buffer
    if (job->param.src_dma.addr == 0x12) {
        job->param.src_dma.addr = priv.engine[dev].pip1_buf.paddr;
        job->param.src_dma.addr += ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    }
    tmp = job->param.src_dma.addr;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x407801a0;
    offset += 8;
    // src dma 0x1a4
    tmp = job->param.src_dma.line_offset;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x401801a4;
    offset += 8;
// LL (1)
    // command count
    tmp = job->param.lli.job_count << 16;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40205400;
    offset += 8;
// SC (5)
    // SC 0x5100
    tmp = (job->param.sc.x_start) | (job->param.sc.x_end << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785100;
    offset += 8;
    // SC 0x5104
    tmp = (job->param.sc.y_start) | (job->param.sc.type << 14) | (job->param.sc.y_end << 16) |
          (job->param.sc.frame_type << 30);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785104;
    offset += 8;
    // SC 0x5200
    tmp = (job->param.sc.sca_src_width) | (job->param.sc.sca_src_height << 16) |
          (job->param.sc.fix_pat_en << 31) ;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785200;
    offset += 8;
    // SC 0x5204
    tmp = (job->param.sc.fix_pat_cb) | (job->param.sc.fix_pat_y0 << 8) |
          (job->param.sc.fix_pat_cr << 16) | (job->param.sc.fix_pat_y1 << 24) ;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785204;
    offset += 8;
    // SC 0x5208
    tmp = (job->param.sc.dr_en) | (job->param.sc.rgb2yuv_en << 1) |
          (job->param.sc.rb_swap << 2);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40085208;
    offset += 8;
// GLOBAL (3)
    // global 0x5404
    tmp = job->param.global.sca_src_format << 20;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40205404;
    offset += 8;
    // global 0x5430
#if GM8210
    tmp = (job->param.global.src_split_en << 8);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40405430;
    offset += 8;
#endif
    // global 0x5470
    tmp = job->param.global.tc_format | (job->param.global.tc_rb_swap << 2);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40085470;
    offset += 8;

// MASK0~3(8)
    if (job->param.sc.fix_pat_en) {
        for (win_idx = 0; win_idx < 4; win_idx++) {
            if (job->param.mask[win_idx].enable) {
                tmp = job->param.mask[win_idx].color << 12;
                tmp |= job->param.mask[win_idx].x_start |
                       (job->param.mask[win_idx].x_end << 16) |
                       (job->param.mask[win_idx].alpha << 28) |
                       (job->param.mask[win_idx].true_hollow << 31);
                addr = 0x40780030 + win_idx * 0x8;
                *(volatile unsigned int *)(base + offset) = tmp;
                *(volatile unsigned int *)(base + offset + 4) = addr;
                offset += 8;
                // mask y 0x34
                tmp = job->param.mask[win_idx].border_width << 12;
                tmp |= job->param.mask[win_idx].y_start |
                       (job->param.mask[win_idx].y_end << 16);
                addr = 0x40780034 + win_idx * 0x8;
                *(volatile unsigned int *)(base + offset) = tmp;
                *(volatile unsigned int *)(base + offset + 4) = addr;
                offset += 8;
            }
        }
    }
#if 0   /* mask & osd external api do not use, remove it for performance */
    else {
        for (i = 0; i < mpath; i++) {
            if (!job->input_node[i].enable)
                continue;

            chn = job->input_node[i].minor;
            for (win_idx = 0; win_idx < 4; win_idx++) {
                if (!job->mask_pp_sel[i][win_idx])
                    continue;

                pp_sel = job->mask_pp_sel[i][win_idx] - 1;
                if (global_param->chn_global[chn].mask[pp_sel][win_idx].enable) {
                    // mask x 0x30
                    tmp = global_param->chn_global[chn].mask[pp_sel][win_idx].color << 12;
                    tmp |= global_param->chn_global[chn].mask[pp_sel][win_idx].x_start |
                           (global_param->chn_global[chn].mask[pp_sel][win_idx].x_end << 16) |
                           (global_param->chn_global[chn].mask[pp_sel][win_idx].alpha << 28) |
                           (global_param->chn_global[chn].mask[pp_sel][win_idx].true_hollow << 31);
                    addr = 0x40780030 + win_idx * 0x8;
                    *(volatile unsigned int *)(base + offset) = tmp;
                    *(volatile unsigned int *)(base + offset + 4) = addr;
                    offset += 8;
                    // mask y 0x34
                    tmp = global_param->chn_global[chn].mask[pp_sel][win_idx].border_width << 12;
                    tmp |= global_param->chn_global[chn].mask[pp_sel][win_idx].y_start |
                           (global_param->chn_global[chn].mask[pp_sel][win_idx].y_end << 16);
                    addr = 0x40780034 + win_idx * 0x8;
                    *(volatile unsigned int *)(base + offset) = tmp;
                    *(volatile unsigned int *)(base + offset + 4) = addr;
                    offset += 8;
                }
            }
        }
    }
#endif  // end of if 0
    *(volatile unsigned int *)(base + offset) = 0x0;
    if (job->ll_flow.count > 1) {
        // next command pointer
        tmp = 0xa0040000 + next_cmd_addr;
        *(volatile unsigned int *)(base + offset + 4) = tmp;
    }
    else {
        if (last == 1) {
            tmp = 0x000000;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
        else {
            // next link list pointer
            tmp = 0x80040000 + next_cmd_addr;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
    }
#if 0   // record job id in sram
    tmp = job->job_id;
    *(volatile unsigned int *)(base + offset + 8) = tmp;
    tmp = 0xe0000000;
    *(volatile unsigned int *)(base + offset + 4 + 8) = tmp;
#endif
#if 0
    printk("****property***base [%x]\n", base);
    printk("src_fmt [%d][%d]\n", job->param.src_format, i);
    printk("dst_fmt [%d]\n", job->param.dst_format);
    printk("src_width [%d]\n", job->param.sc.sca_src_width);
    printk("src_height [%d]\n", job->param.sc.sca_src_height);
    printk("src_bg_width [%d]\n", job->param.src_bg_width);
    printk("src_bg_height [%d]\n", job->param.src_bg_height);
    printk("sc x_start[%d]\n", job->param.sc.x_start);
    printk("sc x_end[%d]\n", job->param.sc.x_end);
    printk("sc y_start[%d]\n", job->param.sc.y_start);
    printk("sc y_end[%d]\n", job->param.sc.y_end);
    printk("dst_width [%d]\n", job->param.tc[0].width);
    printk("dst_height [%d]\n", job->param.tc[0].height);
    printk("dst_x_end [%d]\n", job->param.tc[0].x_end);
    printk("dst_y_end [%d]\n", job->param.tc[0].y_end);
    printk("source dma [%x]\n", job->param.src_dma.addr);
    printk("source dma line offset [%d]\n", job->param.src_dma.line_offset);
    printk("dst dma [%x]\n", job->param.dest_dma[0].addr);
    printk("dst dma line offset [%d]\n", job->param.dest_dma[0].line_offset);
    printk("scl_feature [%d]\n", job->param.scl_feature);
#endif

    return offset;
}

// sd1,2,3 + mask4~7
u32 fscaler300_lli_blk1_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last)
{
    int i, mpath;//, pp_sel, win_idx;
    //int chn = 0;
    u32 base = 0, addr = 0, tmp = 0;
    u32 next_cmd_addr = 0;
    fscaler300_global_param_t *global_param = &priv.global_param;

    //scl_dbg("%s [%d][%x]\n", __func__, job->job_id, offset);

    base = priv.engine[dev].fscaler300_reg + SCALER300_LINK_LIST_MEM_BASE;

    next_cmd_addr = next;

    if (job->job_type == MULTI_SJOB)
        mpath = 4;
    else
        mpath = 1;
#if 0   /* mask & osd external api do not use, remove it for performance */
// MASK4~7(8)
    if (job->param.sc.fix_pat_en == 0) {
        for (i = 0; i < mpath; i++) {
            if (!job->input_node[i].enable)
                continue;

            chn = job->input_node[i].minor;
            for (win_idx = 4; win_idx < MASK_MAX; win_idx++) {
                if (!job->mask_pp_sel[i][win_idx])
                    continue;
                pp_sel = job->mask_pp_sel[i][win_idx] - 1;
                if (global_param->chn_global[chn].mask[pp_sel][win_idx].enable) {
                    // mask x 0x30
                    tmp = global_param->chn_global[chn].mask[pp_sel][win_idx].color << 12;
                    tmp |= global_param->chn_global[chn].mask[pp_sel][win_idx].x_start |
                           (global_param->chn_global[chn].mask[pp_sel][win_idx].x_end << 16) |
                           (global_param->chn_global[chn].mask[pp_sel][win_idx].alpha << 28) |
                           (global_param->chn_global[chn].mask[pp_sel][win_idx].true_hollow << 31);
                    addr = 0x40780030 + win_idx * 0x8;
                    *(volatile unsigned int *)(base + offset) = tmp;
                    *(volatile unsigned int *)(base + offset + 4) = addr;
                    offset += 8;
                    // mask y 0x34
                    tmp = global_param->chn_global[chn].mask[pp_sel][win_idx].border_width << 12;
                    tmp |= global_param->chn_global[chn].mask[pp_sel][win_idx].y_start |
                           (global_param->chn_global[chn].mask[pp_sel][win_idx].y_end << 16);
                    addr = 0x40780034 + win_idx * 0x8;
                    *(volatile unsigned int *)(base + offset) = tmp;
                    *(volatile unsigned int *)(base + offset + 4) = addr;
                    offset += 8;
                }
            }
        }
    }
#endif // end of if 0
// subchannel sd width/height (3)
    for (i = 1; i < 4; i++){
        if (job->param.sd[i].enable) {
            // sd0 width/height 0x10
            tmp = 0;
            tmp |= job->param.sd[i].width | (job->param.sd[i].height << 16);
            addr = 0x40780010 + i * 0x4;
            *(volatile unsigned int *)(base + offset) = tmp;
            *(volatile unsigned int *)(base + offset + 4) = addr;
            offset += 8;
        }
    }

// subchannel res width/height (2)
    for (i = 1; i < 3; i++) {
        if (job->param.sd[i].enable) {
            // tc0 width 0x20
            tmp = 0;
            if (i == 1) {
                addr = 0x40600020 + (i >> 1) * 0x4;
                tmp = (job->param.tc[i].width << 16);
            }
            else {
                addr = 0x40780024;
                tmp = job->param.tc[i].width | (job->param.tc[i + 1].width << 16);
            }
            *(volatile unsigned int *)(base + offset) = tmp;
            *(volatile unsigned int *)(base + offset + 4) = addr;
            offset += 8;
        }
    }

// sd (2*3)
    for (i = 1; i < 4; i++) {
        if (job->param.sd[i].enable) {
            // sd0 hstep/vstep 0x78
            if (job->param.sc.frame_type == PROGRESSIVE) {
                tmp = 0;
                tmp |= job->param.sd[i].hstep | (job->param.sd[i].vstep << 16);
                addr = 0x40780078 + i * 0x10;
                *(volatile unsigned int *)(base + offset) = tmp;
                *(volatile unsigned int *)(base + offset + 4) = addr;
                offset += 8;
            }
            // sd0 topvoft/botvoft 0x7c
            if (job->param.sc.frame_type != PROGRESSIVE) {
                tmp = 0;
                tmp |= (global_param->init.smooth[i].hsmo_str << 26) | (global_param->init.smooth[i].vsmo_str << 29);
                tmp |= job->param.sd[i].topvoft | (job->param.sd[i].botvoft << 13);
                addr = 0x4078007c + i * 0x10;
                *(volatile unsigned int *)(base + offset) = tmp;
                *(volatile unsigned int *)(base + offset + 4) = addr;
                offset += 8;
            }
        }
    }

// tc (2*3)
    for (i = 1; i < 4; i++) {
        if (job->param.tc[i].enable || job->param.img_border.border_en[i]) {
            // tc0 0x160
            tmp = 0;
            tmp |= job->param.tc[i].x_start | (job->param.tc[i].x_end << 16);
            addr = 0x40780160 + i* 0x8;
            *(volatile unsigned int *)(base + offset) = tmp;
            *(volatile unsigned int *)(base + offset + 4) = addr;
            offset += 8;
            // tc0 0x164
            tmp = job->param.img_border.border_width[i];
            tmp |= job->param.tc[i].y_start | (job->param.tc[i].y_end << 16);
            addr = 0x40780164 + i* 0x8;
            *(volatile unsigned int *)(base + offset) = tmp;
            *(volatile unsigned int *)(base + offset + 4) = addr;
            offset += 8;
        }
    }

// DMA (2*3)
    for (i = 1; i < 4; i++) {
        if (job->param.sd[i].enable) {
            // dma0 0x180
            tmp = 0;
            tmp |= job->param.dest_dma[i].addr;
            addr = 0x40780180 + i * 0x8;
            *(volatile unsigned int *)(base + offset) = tmp;
            *(volatile unsigned int *)(base + offset + 4) = addr;
            offset += 8;
            // dma0 0x184
            tmp = 0;
            tmp |= job->param.dest_dma[i].line_offset | (job->param.dest_dma[i].field_offset << 13);
            addr = 0x40780184 + i * 0x8;
            *(volatile unsigned int *)(base + offset) = tmp;
            *(volatile unsigned int *)(base + offset + 4) = addr;
            offset += 8;
        }
    }

    // next command
    *(volatile unsigned int *)(base + offset) = 0x0;
    if (job->ll_flow.osd0123456 == 1) {
        // next command pointer
        tmp = 0xa0040000 + next_cmd_addr;
        *(volatile unsigned int *)(base + offset + 4) = tmp;
    }
    else {
        if (last == 1) {
            tmp = 0x000000;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
        else {
            // next link list pointer
            tmp = 0x80040000 + next_cmd_addr;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
    }
    return offset;
}

// osd0~6
u32 fscaler300_lli_blk2_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last)
{
    int i, mpath, pp_sel, win_idx;
    int chn = 0;
    u32 base = 0, addr = 0, tmp = 0;
    u32 next_cmd_addr = 0;
    fscaler300_global_param_t *global_param = &priv.global_param;

    base = priv.engine[dev].fscaler300_reg + SCALER300_LINK_LIST_MEM_BASE;

    next_cmd_addr = next;

    if (job->job_type == MULTI_SJOB)
        mpath = 4;
    else
        mpath = 1;

// osd0~6 (4*7)
    for (i = 0; i < mpath; i++) {
        if (!job->input_node[i].enable)
            continue;

        chn = job->input_node[i].minor;
        for (win_idx = 0; win_idx < 7; win_idx++) {
            if (!job->osd_pp_sel[i][win_idx])
                continue;

            pp_sel = job->osd_pp_sel[i][win_idx] - 1;
            if (global_param->chn_global[chn].osd[pp_sel][win_idx].enable) {
                temp_osd_t  temp_osd;
                memset(&temp_osd, 0x0, sizeof(temp_osd_t));
                fscaler300_set_osd_temp(chn, pp_sel, win_idx, i, &temp_osd, job);

                // osd0 0xc0
                tmp = 0;
                tmp = (global_param->chn_global[chn].osd[pp_sel][win_idx].wd_addr) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].row_sp << 16) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].font_zoom << 28);
                addr = 0x407800c0 + win_idx * 0x10;
                *(volatile unsigned int *)(base + offset) = tmp;
                *(volatile unsigned int *)(base + offset + 4) = addr;
                offset += 8;
                // osd0 0xc4
                tmp = 0;
                tmp = (global_param->chn_global[chn].osd[pp_sel][win_idx].col_sp) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].bg_color << 12) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].h_wd_num << 16) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].v_wd_num << 24) |
                      (i << 30);    // task select
                addr = 0x407800c4 + win_idx * 0x10;
                *(volatile unsigned int *)(base + offset) = tmp;
                *(volatile unsigned int *)(base + offset + 4) = addr;
                offset += 8;
                // osd0 0xc8
                tmp = 0;
                tmp = (temp_osd.x_start) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].border_width << 12) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].border_type << 15) |
                      (temp_osd.x_end << 16) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].border_color << 28);
                addr = 0x407800c8 + win_idx * 0x10;
                *(volatile unsigned int *)(base + offset) = tmp;
                *(volatile unsigned int *)(base + offset + 4) = addr;
                offset += 8;
                // osd0 0xcc
                tmp = (temp_osd.y_start) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].font_alpha << 12) |
                      (temp_osd.y_end << 16) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].bg_alpha << 28) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].border_en << 31);
                addr = 0x407800cc + win_idx * 0x10;
                *(volatile unsigned int *)(base + offset) = tmp;
                *(volatile unsigned int *)(base + offset + 4) = addr;
                offset += 8;

            }
        }
    }

    // next command
    *(volatile unsigned int *)(base + offset) = 0x0;
    if (job->ll_flow.osd7 == 1) {
        // next command pointer
        tmp = 0xa0040000 + next_cmd_addr;
        *(volatile unsigned int *)(base + offset + 4) = tmp;
    }
    else {
        if (last == 1) {
            tmp = 0x000000;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
        else {
            // next link list pointer
            tmp = 0x80040000 + next_cmd_addr;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
    }

    return offset;
}

// osd7
u32 fscaler300_lli_blk3_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last)
{
    int i, mpath, pp_sel, win_idx;
    int chn = 0;
    u32 base = 0, addr = 0, tmp = 0;
    u32 next_cmd_addr = 0;
    fscaler300_global_param_t *global_param = &priv.global_param;

    base = priv.engine[dev].fscaler300_reg + SCALER300_LINK_LIST_MEM_BASE;

    next_cmd_addr = next;

    if (job->job_type == MULTI_SJOB)
        mpath = 4;
    else
        mpath = 1;

// osd7 (4)
    for (i = 0; i < mpath; i++) {
        if (!job->input_node[i].enable)
            continue;

        chn = job->input_node[i].minor;
        for (win_idx = 7; win_idx < OSD_MAX; win_idx++) {
            if (!job->osd_pp_sel[i][win_idx])
                continue;

            pp_sel = job->osd_pp_sel[i][win_idx] - 1;
            if (global_param->chn_global[chn].osd[pp_sel][win_idx].enable) {
                temp_osd_t  temp_osd;
                memset(&temp_osd, 0x0, sizeof(temp_osd_t));
                fscaler300_set_osd_temp(chn, pp_sel, win_idx, i, &temp_osd, job);
                // osd0 0xc0
                tmp = 0;
                tmp = (global_param->chn_global[chn].osd[pp_sel][win_idx].wd_addr) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].row_sp << 16) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].font_zoom << 28);
                addr = 0x407800c0 + win_idx * 0x10;
                *(volatile unsigned int *)(base + offset) = tmp;
                *(volatile unsigned int *)(base + offset + 4) = addr;
                offset += 8;
                // osd0 0xc4
                tmp = 0;
                tmp = (global_param->chn_global[chn].osd[pp_sel][win_idx].col_sp) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].bg_color << 12) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].h_wd_num << 16) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].v_wd_num << 24) |
                      (i << 30);    //task select
                addr = 0x407800c4 + win_idx * 0x10;
                *(volatile unsigned int *)(base + offset) = tmp;
                *(volatile unsigned int *)(base + offset + 4) = addr;
                offset += 8;
                // osd0 0xc8
                tmp = 0;
                tmp = (temp_osd.x_start) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].border_width << 12) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].border_type << 15) |
                      (temp_osd.x_end << 16) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].border_color << 28);
                addr = 0x407800c8 + win_idx * 0x10;
                *(volatile unsigned int *)(base + offset) = tmp;
                *(volatile unsigned int *)(base + offset + 4) = addr;
                offset += 8;
                // osd0 0xcc
                tmp = (temp_osd.y_start) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].font_alpha << 12) |
                      (temp_osd.y_end << 16) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].bg_alpha << 28) |
                      (global_param->chn_global[chn].osd[pp_sel][win_idx].border_en << 31);
                addr = 0x407800cc + win_idx * 0x10;
                *(volatile unsigned int *)(base + offset) = tmp;
                *(volatile unsigned int *)(base + offset + 4) = addr;
                offset += 8;
            }
        }
    }

    *(volatile unsigned int *)(base + offset) = 0x0;
    if (last == 1) {
        // last command
        tmp = 0x000000;
        *(volatile unsigned int *)(base + offset + 4) = tmp;
    }
    else {
        // next link list pointer
        tmp = 0x80040000 + next_cmd_addr;
        *(volatile unsigned int *)(base + offset + 4) = tmp;
    }

    return offset;
}
#ifdef SPLIT_SUPPORT
// Subchannel, sd0~3, dma, ll, sc
u32 fscaler300_lli_split_blk0_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last)
{
    int i = 0, chn = 0;
    u32 base = 0, addr = 0, tmp = 0;
    u32 next_cmd_addr = 0;
    fscaler300_global_param_t *global_param = &priv.global_param;

    //printk("split block 0 setup [%x][%x]\n",offset, next);

    if (job->perf_type == TYPE_ALL || job->perf_type == TYPE_FIRST) {
        i = 0;
        if (job->input_node[i].fops && job->input_node[i].fops->mark_engine_start)
            job->input_node[i].fops->mark_engine_start(dev, job);
        //if (job->input_node[i].fops && job->input_node[i].fops->pre_process)
          //  job->input_node[i].fops->pre_process(job);
    }

    base = priv.engine[dev].fscaler300_reg + SCALER300_LINK_LIST_MEM_BASE;

    chn = job->input_node[0].minor;
    next_cmd_addr = next;
    // status
    *(volatile unsigned int *)(base + offset) = 0x00000000;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x20000000;

    offset += 8;
// SUBCHANNEL (9)
    // sca0~3 enable/bypass, tc0~3 enable 0x0
    tmp = (global_param->init.img_border.res_border_en[0] << 3) |
          (job->param.frm2field.res0_frm2field << 5) |
          (global_param->init.img_border.res_border_en[1] << 11) |
          (job->param.frm2field.res1_frm2field << 13) |
          (global_param->init.img_border.res_border_en[2] << 19) |
          (job->param.frm2field.res2_frm2field << 21) |
          (global_param->init.img_border.res_border_en[3] << 27) |
          (job->param.frm2field.res3_frm2field << 29) ;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40780000;
    offset += 8;
    // mask 0~7 enable 0x4
    tmp = 0;
    for(i = 0; i < 8; i++) {
        if(global_param->chn_global[chn].mask[0][i].enable)
            tmp |= (0x1 << i);
    }
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40080004;
    offset += 8;
    // osd0~7 enable 0x8, mirror flip
    tmp = (job->param.mirror.h_flip << 28) |
          (job->param.mirror.v_flip << 29);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40080008;
    offset += 8;
    // sd0 width/height 0x10
    for (i = 0; i < 4; i++) {
        tmp = 0;
        addr = 0x40780010 + i * 0x4;
        tmp = (job->param.sd[i].width) | (job->param.sd[i].height << 16);
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;
    }
    // tc 0/1 width 0x20
    tmp = (job->param.tc[0].width) | (job->param.tc[1].width << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40780020;
    offset += 8;
    // tc 2/3 width 0x24
    tmp = (job->param.tc[2].width) | (job->param.tc[3].width << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40780024;
    offset += 8;

    // GLOBAL (12)
    // global 0x5404
    tmp = job->param.global.sca_src_format << 20;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40205404;
    offset += 8;
    // global 0x5410
        tmp = 0;
    tmp |= job->param.split.split_en_15_00;
        *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785410;
    offset += 8;
    // global 0x5414
        tmp = 0;
    tmp |= job->param.split.split_en_31_16;
        *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785414;
    offset += 8;
    // global 0x5418
    tmp = 0;
    tmp |= job->param.split.split_bypass_15_00;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785418;
    offset += 8;
   // global 0x541c
    tmp = 0;
    tmp |= job->param.split.split_bypass_31_16;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x4078541c;
    offset += 8;
    // global 0x5420
    tmp = 0;
    tmp |= job->param.split.split_sel_07_00;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785420;
    offset += 8;
    // global 0x5424
    tmp = 0;
    tmp |= job->param.split.split_sel_15_08;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785424;
    offset += 8;
    // global 0x5428
    tmp = 0;
    tmp |= job->param.split.split_sel_23_16;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785428;
    offset += 8;
    // global 0x542c
    tmp = 0;
    tmp |= job->param.split.split_sel_31_24;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x4078542c;
    offset += 8;
    // global 0x5430
    tmp = 0;
    tmp |= (job->param.split.split_blk_num | BIT8);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785430;
    offset += 8;
    // global 0x5470
    tmp = job->param.global.tc_format | (job->param.global.tc_rb_swap << 2);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40085470;
    offset += 8;
// DMA (2)
    // src dma 0x1a0
    tmp = job->param.src_dma.addr;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x407801a0;
    offset += 8;
    // src dma 0x1a4
    tmp = job->param.src_dma.line_offset;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x401801a4;
    offset += 8;
// LL (1)
    // command count
    tmp = job->param.lli.job_count << 16;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40205400;
    offset += 8;
// SC (3)
    // SC 0x5100
    tmp = (job->param.sc.x_start) | (job->param.sc.x_end << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785100;
    offset += 8;
    // SC 0x5104
    tmp = (job->param.sc.y_start) | (job->param.sc.type << 14) | (job->param.sc.y_end << 16) |
          (job->param.sc.frame_type << 30);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785104;
    offset += 8;
    // SC 0x5200
    tmp = (job->param.sc.sca_src_width) | (job->param.sc.sca_src_height << 16) |
          (job->param.sc.fix_pat_en << 31) ;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 4) = 0x40785200;
    offset += 8;

    // next command pointer
    tmp = 0xa0040000 + next_cmd_addr;
    *(volatile unsigned int *)(base + offset) = 0;
    *(volatile unsigned int *)(base + offset + 4) = tmp;

    return offset;
}

// global, mask
u32 fscaler300_lli_split_blk1_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last)
{
    int i = 0, chn = 0;
    u32 base = 0, addr = 0, tmp = 0;
    u32 next_cmd_addr = 0;
    fscaler300_global_param_t *global_param = &priv.global_param;

    //printk("split block 1 setup [%x][%x]\n",offset, next);

    base = priv.engine[dev].fscaler300_reg + SCALER300_LINK_LIST_MEM_BASE;

    chn = job->input_node[0].minor;
    next_cmd_addr = next;

// SD (8)
    // sd0~3 hstep/vstep 0x78
    for (i = 0; i < 4; i++) {
        // sd0 hstep/vstep 0x78
        if (job->param.sc.frame_type == PROGRESSIVE) {
            tmp = 0;
            tmp |= job->param.sd[i].hstep | (job->param.sd[i].vstep << 16);
            addr = 0x40780078 + i * 0x10;
            *(volatile unsigned int *)(base + offset) = tmp;
            *(volatile unsigned int *)(base + offset + 4) = addr;
            offset += 8;
        }
        // sd0 topvoft/botvoft 0x7c
        else {
            tmp = 0;
            tmp |= (global_param->init.smooth[i].hsmo_str << 26) | (global_param->init.smooth[i].vsmo_str << 29);
            tmp |= job->param.sd[i].topvoft | (job->param.sd[i].botvoft << 13);
            addr = 0x4078007c + i * 0x10;
            *(volatile unsigned int *)(base + offset) = tmp;
            *(volatile unsigned int *)(base + offset + 4) = addr;
            offset += 8;
        }
    }
// TC (8)
#if 0
    // tc (2*4)
    for (i = 0; i < 4; i++) {
        //if (job->param.tc[i].enable || global_param->init.img_border.res_border_en[i]) {
            // tc0 0x160
            tmp = 0;
            tmp |= job->param.tc[i].x_start | (job->param.tc[i].x_end << 16);
            addr = 0x40780160 + i* 0x8;
            *(volatile unsigned int *)(base + offset) = tmp;
            *(volatile unsigned int *)(base + offset + 4) = addr;
            offset += 8;
            // tc0 0x164
            tmp = global_param->init.img_border.res_border_width[i];
            tmp |= job->param.tc[i].y_start | (job->param.tc[i].y_end << 16);
            addr = 0x40780164 + i* 0x8;
            *(volatile unsigned int *)(base + offset) = tmp;
            *(volatile unsigned int *)(base + offset + 4) = addr;
            offset += 8;
        //}
    }
#endif

// mask (16)
    for (i = 0; i < 8; i++) {
        if (global_param->chn_global[chn].mask[0][i].enable) {
            // mask x 0x30
            tmp = global_param->chn_global[chn].mask[0][i].color << 12;
            tmp |= global_param->chn_global[chn].mask[0][i].x_start |
                   (global_param->chn_global[chn].mask[0][i].x_end << 16) |
                   (global_param->chn_global[chn].mask[0][i].alpha << 28) |
                   (global_param->chn_global[chn].mask[0][i].true_hollow << 31);
            addr = 0x40780030 + i * 0x8;
            *(volatile unsigned int *)(base + offset) = tmp;
            *(volatile unsigned int *)(base + offset + 4) = addr;
            offset += 8;
            // mask y 0x34
            tmp = global_param->chn_global[chn].mask[0][i].border_width << 12;
            tmp |= global_param->chn_global[chn].mask[0][i].y_start |
                   (global_param->chn_global[chn].mask[0][i].y_end << 16);
            addr = 0x40780034 + i * 0x8;
            *(volatile unsigned int *)(base + offset) = tmp;
            *(volatile unsigned int *)(base + offset + 4) = addr;
            offset += 8;
        }
    }

    // next command pointer
    tmp = 0xa0040000 + next_cmd_addr;
    *(volatile unsigned int *)(base + offset) = 0;
    *(volatile unsigned int *)(base + offset + 4) = tmp;

    return offset;
}

// split dma 0~6
u32 fscaler300_lli_split_blk2_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last)
{
    int i = 0, chn = 0;
    u32 base = 0, addr = 0, tmp = 0;
    u32 next_cmd_addr = 0;

    //printk("split block 2 setup [%x][%x]\n",offset, next);

    base = priv.engine[dev].fscaler300_reg + SCALER300_LINK_LIST_MEM_BASE;

    chn = job->input_node[0].minor;
    next_cmd_addr = next;

    for (i = 0; i < 7; i++) {
        // dma0 0x1c0
        addr = 0x40780000 + 0x1c0 + i * 0x10;
        tmp = job->param.split_dma[i].res0_addr;
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1c4
        addr += 4;
        tmp = (job->param.split_dma[i].res0_offset | (job->param.split_dma[i].res0_field_offset << 13));
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1c8
        addr += 4;
        tmp = job->param.split_dma[i].res1_addr;
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1cc
        addr += 4;
        tmp = (job->param.split_dma[i].res1_offset | (job->param.split_dma[i].res1_field_offset << 13));
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;
    }
    // next command
    *(volatile unsigned int *)(base + offset) = 0x0;
    if (job->split_flow.ch_7_13) {
        // next command pointer
        tmp = 0xa0040000 + next_cmd_addr;
        *(volatile unsigned int *)(base + offset + 4) = tmp;
    }
    else {
        if (last == 1) {
            tmp = 0x000000;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
        else {
            // next link list pointer
            tmp = 0x80040000 + next_cmd_addr;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
    }

    return offset;
}

// split dma 7~13
u32 fscaler300_lli_split_blk3_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last)
{
    int i = 0, chn = 0;
    u32 base = 0, addr = 0, tmp = 0;
    u32 next_cmd_addr = 0;

    //printk("split block 3 setup [%x][%x]\n",offset, next);

    base = priv.engine[dev].fscaler300_reg + SCALER300_LINK_LIST_MEM_BASE;

    chn = job->input_node[0].minor;
    next_cmd_addr = next;

    for (i = 7; i < 14; i++) {
        // dma0 0x1c0
        tmp = 0;
        addr = 0x40780000 + 0x1c0 + i * 16;
        tmp |= job->param.split_dma[i].res0_addr;
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1c4
        tmp = 0;
        addr += 4;
        tmp |= job->param.split_dma[i].res0_offset | (job->param.split_dma[i].res0_field_offset << 13);
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1c8
        tmp = 0;
        addr += 4;
        tmp |= job->param.split_dma[i].res1_addr;
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1cc
        tmp = 0;
        addr += 4;
        tmp |= job->param.split_dma[i].res1_offset | (job->param.split_dma[i].res1_field_offset << 13);
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;
    }
    // next command
    *(volatile unsigned int *)(base + offset) = 0x0;
    if (job->split_flow.ch_14_20) {
        // next command pointer
        tmp = 0xa0040000 + next_cmd_addr;
        *(volatile unsigned int *)(base + offset + 4) = tmp;
    }
    else {
        if (last == 1) {
            tmp = 0x000000;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
        else {
            // next link list pointer
            tmp = 0x80040000 + next_cmd_addr;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
    }

    return offset;
}

// split dma 14~20
u32 fscaler300_lli_split_blk4_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last)
{
    int i = 0, chn = 0;
    u32 base = 0, addr = 0, tmp = 0;
    u32 next_cmd_addr = 0;

    //printk("split block 4 setup [%x]\n",offset);

    base = priv.engine[dev].fscaler300_reg + SCALER300_LINK_LIST_MEM_BASE;

    chn = job->input_node[0].minor;
    next_cmd_addr = next;

    for (i = 14; i < 21; i++) {
        // dma0 0x1c0
        tmp = 0;
        addr = 0x40780000 + 0x1c0 + i * 16;
        tmp |= job->param.split_dma[i].res0_addr;
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1c4
        tmp = 0;
        addr += 4;
        tmp |= job->param.split_dma[i].res0_offset | (job->param.split_dma[i].res0_field_offset << 13);
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1c8
        tmp = 0;
        addr += 4;
        tmp |= job->param.split_dma[i].res1_addr;
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1cc
        tmp = 0;
        addr += 4;
        tmp |= job->param.split_dma[i].res1_offset | (job->param.split_dma[i].res1_field_offset << 13);
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;
    }
    // next command
    *(volatile unsigned int *)(base + offset) = 0x0;
    if (job->split_flow.ch_21_27) {
        // next command pointer
        tmp = 0xa0040000 + next_cmd_addr;
        *(volatile unsigned int *)(base + offset + 4) = tmp;
    }
    else {
        if (last == 1) {
            tmp = 0x000000;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
        else {
            // next link list pointer
            tmp = 0x80040000 + next_cmd_addr;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
    }

    return offset;
}

// split dma 21~27
u32 fscaler300_lli_split_blk5_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last)
{
    int i = 0, chn = 0;
    u32 base = 0, addr = 0, tmp = 0;
    u32 next_cmd_addr = 0;

   // printk("split block 5 setup [%x]\n",offset);

    base = priv.engine[dev].fscaler300_reg + SCALER300_LINK_LIST_MEM_BASE;

    chn = job->input_node[0].minor;
    next_cmd_addr = next;

    for (i = 21; i < 28; i++) {
        // dma0 0x1c0
        tmp = 0;
        addr = 0x40780000 + 0x1c0 + i * 16;
        tmp |= job->param.split_dma[i].res0_addr;
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1c4
        tmp = 0;
        addr += 4;
        tmp |= job->param.split_dma[i].res0_offset | (job->param.split_dma[i].res0_field_offset << 16);
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1c8
        tmp = 0;
        addr += 4;
        tmp |= job->param.split_dma[i].res1_addr;
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1cc
        tmp = 0;
        addr += 4;
        tmp |= job->param.split_dma[i].res1_offset | (job->param.split_dma[i].res1_field_offset << 16);
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;
    }
    // next command
    *(volatile unsigned int *)(base + offset) = 0x0;
    if (job->split_flow.ch_28_31) {
        // next command pointer
        tmp = 0xa0040000 + next_cmd_addr;
        *(volatile unsigned int *)(base + offset + 4) = tmp;
    }
    else {
        if (last == 1) {
            tmp = 0x000000;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
        else {
            // next link list pointer
            tmp = 0x80040000 + next_cmd_addr;
            *(volatile unsigned int *)(base + offset + 4) = tmp;
        }
    }

    return offset;
}

// split dma 28~31
u32 fscaler300_lli_split_blk6_setup(int dev, fscaler300_job_t *job, u32 offset, u32 next, int last)
{
    int i = 0, chn = 0;
    u32 base = 0, addr = 0, tmp = 0;
    u32 next_cmd_addr = 0;

    //printk("split block 6 setup [%x]\n",offset);

    base = priv.engine[dev].fscaler300_reg + SCALER300_LINK_LIST_MEM_BASE;

    chn = job->input_node[0].minor;
    next_cmd_addr = next;

    for (i = 28; i < 32; i++) {
        // dma0 0x1c0
        tmp = 0;
        addr = 0x40780000 + 0x1c0 + i * 16;
        tmp |= job->param.split_dma[i].res0_addr;
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1c4
        tmp = 0;
        addr += 4;
        tmp |= job->param.split_dma[i].res0_offset | (job->param.split_dma[i].res0_field_offset << 16);
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1c8
        tmp = 0;
        addr += 4;
        tmp |= job->param.split_dma[i].res1_addr;
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;

        // dma0 0x1cc
        tmp = 0;
        addr += 4;
        tmp |= job->param.split_dma[i].res1_offset | (job->param.split_dma[i].res1_field_offset << 16);
        *(volatile unsigned int *)(base + offset) = tmp;
        *(volatile unsigned int *)(base + offset + 4) = addr;
        offset += 8;
    }
    // next command
    *(volatile unsigned int *)(base + offset) = 0x0;
    if (last == 1) {
        tmp = 0x000000;
        *(volatile unsigned int *)(base + offset + 4) = tmp;
    }
    else {
        // next link list pointer
        tmp = 0x80040000 + next_cmd_addr;
        *(volatile unsigned int *)(base + offset + 4) = tmp;
    }

    return offset;
}
#endif
int fscaler300_write_dummy(int dev, u32 offset, fscaler300_param_t *dummy, fscaler300_job_t *job)
{
    int tmp = 0, base = 0;
    fscaler300_global_param_t *global_param = &priv.global_param;

    //scl_dbg("%s\n", __func__);
    //printk("write dummy\n");
    base = priv.engine[dev].fscaler300_reg;

    /* clear ll_done interrupt mask */
    // need??
    //printk("fscaler300_write_dummy\n");
    /*================= Subchannel =================*/
    // sca0~3 enable/bypass, tc0~3 enable 0x0
    tmp = (dummy->sd[0].bypass) | (dummy->sd[0].enable << 1);
    *(volatile unsigned int *)(base + 0x0) = tmp;
    // mask0~7 disable, fcs/dn enable
    tmp = (global_param->init.fcs.enable << 8) | (global_param->init.dn.enable << 9);
    *(volatile unsigned int *)(base + 0x4) = tmp;
    // osd0~7 disable, mirror flip
    tmp = (dummy->mirror.h_flip << 28) | (dummy->mirror.v_flip << 29);
    *(volatile unsigned int *)(base + 0x8) = tmp;
    // sd0 width/height
    tmp = (dummy->sd[0].width) | (dummy->sd[0].height << 16);
    *(volatile unsigned int *)(base + 0x10) = tmp;
    // tc width
    tmp = dummy->tc[0].width;
    *(volatile unsigned int *)(base + 0x20) = tmp;
    /*================= SC =================*/
    tmp = (dummy->sc.x_start) | (dummy->sc.x_end << 16);
    *(volatile unsigned int *)(base + 0x5100) = tmp;

    tmp = (dummy->sc.y_start) | (dummy->sc.y_end << 16) | (dummy->sc.type << 14) |
          (dummy->sc.frame_type << 30);
    *(volatile unsigned int *)(base + 0x5104) = tmp;

    tmp = (dummy->sc.sca_src_width) | (dummy->sc.sca_src_height << 16) |
          (dummy->sc.fix_pat_en << 31);
    *(volatile unsigned int *)(base + 0x5200) = tmp;
    /*================= SD =================*/
    /* H&V Step */
    tmp = dummy->sd[0].hstep | (dummy->sd[0].vstep << 16);
    *(volatile unsigned int *)(base + 0x78) = tmp;
    /*================= Source DMA =================*/
    tmp = dummy->src_dma.addr;
    *(volatile unsigned int *)(base + 0x1a0) = tmp;

    tmp = dummy->src_dma.line_offset;
    *(volatile unsigned int *)(base + 0x1a4) = tmp;
    /*================= Destination DMA =================*/
    tmp = dummy->dest_dma[0].addr;
    *(volatile unsigned int *)(base + 0x180) = tmp;

    tmp = (dummy->dest_dma[0].line_offset) | (dummy->dest_dma[0].field_offset << 13);
    *(volatile unsigned int *)(base + 0x184) = tmp;
    /*=============== LLI Addr ===============*/
    // next command addr
    tmp = offset;
    *(volatile unsigned int *)(base + 0x1a8) = tmp;
    // current command addr
    tmp = (dummy->lli.curr_cmd_addr | (dummy->lli.next_cmd_null << 24));
    *(volatile unsigned int *)(base + 0x1ac) = tmp;
    // job count
    tmp = dummy->lli.job_count << 16;
    *(volatile unsigned int *)(base + 0x5400) = tmp;
    /*=============== Global ================*/
    // input format, capture mode, capture enable
    tmp = (dummy->global.sca_src_format << 20) | (dummy->global.capture_mode << 26) |
          (1 << 31);
    *(volatile unsigned int *)(base + 0x5404) = tmp;
    // source channel enable
    *(volatile unsigned int *)(base + 0x5408) = 1;
    // dma ctrl
    tmp = (global_param->init.dma_ctrl.dma_rc_wait_value) | (dummy->global.dma_fifo_wm << 16) |
          (dummy->global.dma_job_wm << 24);
    *(volatile unsigned int *)(base + 0x543c) = tmp;
    // output format
    tmp = (dummy->global.tc_format) | (dummy->global.tc_rb_swap << 2);
    *(volatile unsigned int *)(base + 0x5470) = tmp;

    // status
    *(volatile unsigned int *)(base + SCALER300_LINK_LIST_MEM_BASE + SCALER300_DUMMY_BASE) = 0x00000000;
    *(volatile unsigned int *)(base + SCALER300_LINK_LIST_MEM_BASE + SCALER300_DUMMY_BASE + 0x4) = 0x20000000;

    *(volatile unsigned int *)(base + SCALER300_LINK_LIST_MEM_BASE + SCALER300_DUMMY_BASE + 0x8) = offset;
    *(volatile unsigned int *)(base + SCALER300_LINK_LIST_MEM_BASE + SCALER300_DUMMY_BASE + 0xc) = 0xe0000000;

    return 0;
}
#ifdef SPLIT_SUPPORT
int fscaler300_write_split_register(int dev, fscaler300_job_t *job)
{
    int tmp = 0, i = 0;
    int base = 0;
    int chn = 0;
    int disp = 0;
    fscaler300_global_param_t *global_param = &priv.global_param;

    printk("******fscaler300_write_split register******\n");

    chn = job->input_node[0].minor;
    base = priv.engine[dev].fscaler300_reg;
    /* clear frame end interrupt mask */
    *(volatile unsigned int *)(base + 0x5544) = 0x7fffffff;

    /*================= Subchannel =================*/
    *(volatile unsigned int *)(base + 0x0) = 0;
    *(volatile unsigned int *)(base + 0x4) = 0;
    *(volatile unsigned int *)(base + 0x8) = 0;
    // sd width/height
    for (i = 0; i < 4; i++) {
        tmp = (job->param.sd[i].width) | (job->param.sd[i].height << 16);
        *(volatile unsigned int *)(base + 0x10 + (i * 0x4)) = tmp;
        if(disp)printk("0x[%x] [%x]\n", 0x10 + (i * 0x4),tmp);
    }
    // tc width
    for(i = 0; i < 4; i++) {
        if(i == 0 || i == 2) {
            tmp = (job->param.tc[i].width) | (job->param.tc[i+1].width << 16);
            *(volatile unsigned int *)(base + 0x20 + (i * 0x4)) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x20 + (i * 0x4),tmp);
        }
    }
    /*================= SC =================*/
    tmp = (job->param.sc.x_start) | (job->param.sc.x_end << 16);
    *(volatile unsigned int *)(base + 0x5100) = tmp;
    if(disp)printk("0x5100 [%x]\n", tmp);
    tmp = (job->param.sc.y_start) | (job->param.sc.y_end << 16) | (job->param.sc.type << 14) |
          (job->param.sc.frame_type << 30);
    *(volatile unsigned int *)(base + 0x5104) = tmp;
    if(disp)printk("0x5104 [%x]\n", tmp);
    tmp = (job->param.sc.sca_src_width) | (job->param.sc.sca_src_height << 16) |
          (job->param.sc.fix_pat_en << 31);
    *(volatile unsigned int *)(base + 0x5200) = tmp;
    if(disp)printk("0x5200 [%x]\n", tmp);
    if(job->param.sc.fix_pat_en) {
        tmp = (job->param.sc.fix_pat_cb) | (job->param.sc.fix_pat_y0 << 8) |
              (job->param.sc.fix_pat_cr << 16) | (job->param.sc.fix_pat_y1 << 24);
        *(volatile unsigned int *)(base + 0x5204) = tmp;
        if(disp)printk("0x5204 [%x]\n", tmp);
    }

    tmp = (job->param.sc.dr_en) | (job->param.sc.rgb2yuv_en << 1) | (job->param.sc.rb_swap << 2);
    *(volatile unsigned int *)(base + 0x5208) = tmp;
    if(disp)printk("0x5208 [%x]\n", tmp);
    /*================= SD =================*/
    for(i = 0; i < SD_MAX; i++) {
        //if(job->param.sd[i].enable) {
            /* H&V Step */
            tmp = job->param.sd[i].hstep | (job->param.sd[i].vstep << 16);
            *(volatile unsigned int *)(base + 0x78 + i * 0x10) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x78 + (i * 0x10),tmp);
            /* Top & Bottom offset, presmooth */
            tmp = (job->param.sd[i].topvoft) | (job->param.sd[i].botvoft << 13) |
                  (global_param->init.smooth[i].hsmo_str << 26) |
                  (global_param->init.smooth[i].vsmo_str << 29);
            *(volatile unsigned int *)(base + 0x7c + i * 0x10) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x7c + (i * 0x10),tmp);
        //}
    }
    /*================= TC =================*/
    for(i = 0; i < TC_MAX; i++) {
        //if(job->param.tc[i].enable || job->param.tc[i].border_width) {
            /* X Start & End */
            tmp = job->param.tc[i].x_start | (job->param.tc[i].x_end << 16);
            *(volatile unsigned int *)(base + 0x160 + i * 0x8) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x160 + (i * 0x8),tmp);
            /* Y Start & End */
            tmp = (job->param.tc[i].y_start) | (job->param.tc[i].border_width << 13) |
                  (job->param.tc[i].y_end << 16) | (job->param.tc[i].drc_en << 31);
            *(volatile unsigned int *)(base + 0x164 + i * 0x8) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x164 + (i * 0x8),tmp);
        //}
    }

    /*================= Source DMA =================*/
    tmp = job->param.src_dma.addr;
    *(volatile unsigned int *)(base + 0x1a0) = tmp;
    if(disp)printk("0x1a0 [%x]\n", tmp);
    tmp = job->param.src_dma.line_offset;
    *(volatile unsigned int *)(base + 0x1a4) = tmp;
    if(disp)printk("0x1a4 [%x]\n", tmp);
    /*================= Destination DMA =================*/
    for(i = 0; i < 32; i++) {
            tmp = job->param.split_dma[i].res0_addr;
            *(volatile unsigned int *)(base + 0x1c0 + (i * 0x10)) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x1c0 + (i * 0x10), tmp);
            tmp = job->param.split_dma[i].res0_offset | (job->param.split_dma[i].res0_field_offset << 16);
            *(volatile unsigned int *)(base + 0x1c4 + (i * 0x10)) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x1c4 + (i * 0x10), tmp);

            tmp = job->param.split_dma[i].res1_addr;
            *(volatile unsigned int *)(base + 0x1c8 + (i * 0x10)) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x1c8 + (i * 0x10), tmp);
            tmp = job->param.split_dma[i].res1_offset | (job->param.split_dma[i].res1_field_offset << 16);
            *(volatile unsigned int *)(base + 0x1cc + (i * 0x10)) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x1cc + (i * 0x10), tmp);
    }
    /*=============== Global ================*/
    // channel enable
    tmp = 0x1;
    *(volatile unsigned int *)(base + 0x5408) = tmp;
    if(disp)printk("0x5408 [%x]\n", tmp);
    // dma ctrl
    tmp = (global_param->init.dma_ctrl.dma_rc_wait_value) | (job->param.global.dma_fifo_wm << 16) |
          (job->param.global.dma_job_wm << 24);
    *(volatile unsigned int *)(base + 0x543c) = tmp;
    if(disp)printk("0x543c [%x]\n", tmp);
    // output format
    tmp = (job->param.global.tc_format) | (job->param.global.tc_rb_swap << 2);
    *(volatile unsigned int *)(base + 0x5470) = tmp;
    if(disp)printk("0x5470 [%x]\n", tmp);

    // input format, capture mode, capture enable
    tmp = (job->param.global.sca_src_format << 20) | (job->param.global.capture_mode << 26) |
          (1 << 31);
    *(volatile unsigned int *)(base + 0x5404) = tmp;
    printk("0x5404 [%x]\n", tmp);
    /*=============== split ====================*/
    // global 0x5410
    tmp = job->param.split.split_en_15_00;
    *(volatile unsigned int *)(base + 0x5410) = tmp;
    if(disp)printk("0x5410 [%x]\n", tmp);
    // global 0x5414
    tmp = 0;
    tmp |= job->param.split.split_en_31_16;
    *(volatile unsigned int *)(base + 0x5414) = tmp;
    if(disp)printk("5414 [%x]\n", tmp);
    // global 0x5418
    tmp = job->param.split.split_bypass_15_00;
    *(volatile unsigned int *)(base + 0x5418) = tmp;
    if(disp)printk("5418 [%x]\n", tmp);
    // global 0x541c
    tmp = job->param.split.split_bypass_31_16;
    *(volatile unsigned int *)(base + 0x541c) = tmp;
    if(disp)printk("541c [%x]\n", tmp);
    // global 0x5420
    tmp = job->param.split.split_sel_07_00;
    *(volatile unsigned int *)(base + 0x5420) = tmp;
    if(disp)printk("5420 [%x]\n", tmp);
    // global 0x5424
    tmp = job->param.split.split_sel_15_08;
    *(volatile unsigned int *)(base + 0x5424) = tmp;
    if(disp)printk("5424 [%x]\n", tmp);
    // global 0x5428
    tmp = job->param.split.split_sel_23_16;
    *(volatile unsigned int *)(base + 0x5428) = tmp;
    if(disp)printk("5428 [%x]\n", tmp);
    // global 0x542c
    tmp = job->param.split.split_sel_31_24;
    *(volatile unsigned int *)(base + 0x542c) = tmp;
    if(disp)printk("542c [%x]\n", tmp);
    // global 0x5430
    tmp = (job->param.split.split_blk_num | BIT8);
    *(volatile unsigned int *)(base + 0x5430) = tmp;
    if(disp)printk("5430 [%x]\n", tmp);
    return 0;
}
#endif
int fscaler300_write_register(int dev, fscaler300_job_t *job)
{
#if 1
    int tmp = 0, i = 0;
    int base = 0;
    int chn = 0;
    int disp = 0;
    fscaler300_global_param_t *global_param = &priv.global_param;

    //printk("******fscaler300_write_register******\n");

    chn = job->input_node[0].minor;
    base = priv.engine[dev].fscaler300_reg;
    /* clear frame end interrupt mask */
    *(volatile unsigned int *)(base + 0x5544) = 0x7fffffff;

#if 0
    printk("****property***base [%x]\n", base);
    printk("src_fmt [%d][%d]\n", job->param.src_format, i);
    printk("dst_fmt [%d]\n", job->param.dst_format);
    printk("src_width [%d]\n", job->param.sd[0].width);
    printk("src_height [%d]\n", job->param.sd[0].height);
    printk("src_bg_width [%d]\n", job->param.src_bg_width);
    printk("src_bg_height [%d]\n", job->param.src_bg_height);
    printk("sc x_start[%d]\n", job->param.sc.x_start);
    printk("sc x_end[%d]\n", job->param.sc.x_end);
    printk("sc y_start[%d]\n", job->param.sc.y_start);
    printk("sc y_end[%d]\n", job->param.sc.y_end);
    printk("dst_width [%d]\n", job->param.tc[0].width);
    printk("dst_height [%d]\n", job->param.tc[0].height);
    printk("dst_x_end [%d]\n", job->param.tc[0].x_end);
    printk("dst_y_end [%d]\n", job->param.tc[0].y_end);
    printk("source dma [%x]\n", job->param.src_dma.addr);
    printk("source dma line offset [%d]\n", job->param.src_dma.line_offset);
    printk("dst dma [%x]\n", job->param.dest_dma[0].addr);
    printk("dst dma line offset [%d]\n", job->param.dest_dma[0].line_offset);
    printk("scl_feature [%d]\n", job->param.scl_feature);
#endif

    /* clear ll_done interrupt mask */
    // need??

    /*================= Subchannel =================*/
    // sca0~3 enable/bypass, tc0~3 enable 0x0
    tmp = (job->param.img_border.border_en[0] << 3) |
          (job->param.frm2field.res0_frm2field << 5) |
          (job->param.img_border.border_en[1] << 11) |
          (job->param.frm2field.res1_frm2field << 13) |
          (job->param.img_border.border_en[2] << 19) |
          (job->param.frm2field.res2_frm2field << 21) |
          (job->param.img_border.border_en[3] << 27) |
          (job->param.frm2field.res3_frm2field << 29) ;

    for(i = 0; i <= 3; i++) {
        if(job->param.sd[i].bypass)
            tmp |= (0x1 << (i * 0x8));
        if(job->param.sd[i].enable)
            tmp |= (0x1 << (i * 0x8 + 1));
        if(job->param.tc[i].enable)
            tmp |= (0x1 << (i * 0x8 + 2));
    }
    *(volatile unsigned int *)(base + 0x0) = tmp;
    if(disp)printk("0x0 [%x]\n", tmp);
    // mask0~7 enable, fcs/dn enable
    tmp = 0;
    for(i = 0; i < 8; i++) {
        if(global_param->chn_global[chn].mask[0][i].enable)
            tmp |= (0x1 << i);
    }
    tmp |= (global_param->init.fcs.enable << 8) | (global_param->init.dn.enable << 9);
    *(volatile unsigned int *)(base + 0x4) = tmp;
    if(disp)printk("0x4 [%x]\n", tmp);
    // osd0~7 enable, mirror flip
    tmp = 0;
    for(i = 0; i < 8; i++) {
        if(global_param->chn_global[chn].osd[0][i].enable)
            tmp |= (0x1 << i);
    }
    tmp |= (job->param.mirror.h_flip << 28) | (job->param.mirror.v_flip << 29);
    *(volatile unsigned int *)(base + 0x8) = tmp;
    if(disp)printk("0x8 [%x]\n", tmp);
    // sd width/height
    for(i = 0; i < 4; i++) {
        if(job->param.sd[i].enable) {
            tmp = (job->param.sd[i].width) | (job->param.sd[i].height << 16);
            *(volatile unsigned int *)(base + 0x10 + (i * 0x4)) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x10 + (i * 0x4),tmp);
        }
    }
    // tc width
    for(i = 0; i < 4; i++) {
        if(i == 0 || i == 2) {
            tmp = (job->param.tc[i].width) | (job->param.tc[i+1].width << 16);
            *(volatile unsigned int *)(base + 0x20 + (i * 0x4)) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x20 + (i * 0x4),tmp);
        }
    }
    /*================= SC =================*/
    tmp = (job->param.sc.x_start) | (job->param.sc.x_end << 16);
    *(volatile unsigned int *)(base + 0x5100) = tmp;
    if(disp)printk("0x5100 [%x]\n", tmp);
    tmp = (job->param.sc.y_start) | (job->param.sc.y_end << 16) | (job->param.sc.type << 14) |
          (job->param.sc.frame_type << 30);
    *(volatile unsigned int *)(base + 0x5104) = tmp;
    if(disp)printk("0x5104 [%x]\n", tmp);
    tmp = (job->param.sc.sca_src_width) | (job->param.sc.sca_src_height << 16) |
          (job->param.sc.fix_pat_en << 31);
    *(volatile unsigned int *)(base + 0x5200) = tmp;
    if(disp)printk("0x5200 [%x]\n", tmp);
    if(job->param.sc.fix_pat_en) {
        tmp = (job->param.sc.fix_pat_cb) | (job->param.sc.fix_pat_y0 << 8) |
              (job->param.sc.fix_pat_cr << 16) | (job->param.sc.fix_pat_y1 << 24);
        *(volatile unsigned int *)(base + 0x5204) = tmp;
        if(disp)printk("0x5204 [%x]\n", tmp);
    }

    tmp = (job->param.sc.dr_en) | (job->param.sc.rgb2yuv_en << 1) | (job->param.sc.rb_swap << 2);
    *(volatile unsigned int *)(base + 0x5208) = tmp;
    if(disp)printk("0x5208 [%x]\n", tmp);
    /*================= MASK =================*/

    /*================= SD =================*/
    for(i = 0; i < SD_MAX; i++) {
        if(job->param.sd[i].enable) {
            /* H&V Step */
            tmp = job->param.sd[i].hstep | (job->param.sd[i].vstep << 16);
            *(volatile unsigned int *)(base + 0x78 + i * 0x10) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x78 + (i * 0x10),tmp);
            /* Top & Bottom offset, presmooth */
            tmp = (job->param.sd[i].topvoft) | (job->param.sd[i].botvoft << 13) |
                  (global_param->init.smooth[i].hsmo_str << 26) |
                  (global_param->init.smooth[i].vsmo_str << 29);
            *(volatile unsigned int *)(base + 0x7c + i * 0x10) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x7c + (i * 0x10),tmp);
        }
    }
    /*================= OSD =================*/

    /*================= TC =================*/
    for(i = 0; i < TC_MAX; i++) {
        //if(job->param.tc[i].enable || job->param.tc[i].border_width) {
            /* X Start & End */
            tmp = job->param.tc[i].x_start | (job->param.tc[i].x_end << 16);
            *(volatile unsigned int *)(base + 0x160 + i * 0x8) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x160 + (i * 0x8),tmp);
            /* Y Start & End */
            tmp = (job->param.tc[i].y_start) | (job->param.tc[i].border_width << 13) |
                  (job->param.tc[i].y_end << 16) | (job->param.tc[i].drc_en << 31);
            *(volatile unsigned int *)(base + 0x164 + i * 0x8) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x164 + (i * 0x8),tmp);
        //}
    }

    /*================= Source DMA =================*/
    tmp = job->param.src_dma.addr;
    *(volatile unsigned int *)(base + 0x1a0) = tmp;
    if(disp)printk("0x1a0 [%x]\n", tmp);
    tmp = job->param.src_dma.line_offset;
    *(volatile unsigned int *)(base + 0x1a4) = tmp;
    if(disp)printk("0x1a4 [%x]\n", tmp);
    /*================= Destination DMA =================*/
    for(i = 0; i < SD_MAX; i++) {
        if(job->param.sd[i].enable) {
            tmp = job->param.dest_dma[i].addr;
            *(volatile unsigned int *)(base + 0x180 + (i * 0x8)) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x180 + (i * 0x8), tmp);

            tmp = (job->param.dest_dma[i].line_offset) | (job->param.dest_dma[i].field_offset << 13);
            *(volatile unsigned int *)(base + 0x184 + (i * 0x8)) = tmp;
            if(disp)printk("0x[%x] [%x]\n", 0x184 + (i * 0x8), tmp);
        }
    }

    /*=============== LLI Addr ===============*/
    if(job->param.global.capture_mode == LINK_LIST_MODE) {
        // next command addr
        tmp = job->param.lli.next_cmd_addr;
        *(volatile unsigned int *)(base + 0x1a8) = tmp;
        // current command addr
        tmp = (job->param.lli.curr_cmd_addr | (job->param.lli.next_cmd_null << 24));
        *(volatile unsigned int *)(base + 0x1ac) = tmp;
        // job count
        tmp = job->param.lli.job_count << 16;
        *(volatile unsigned int *)(base + 0x5400) = tmp;
    }

    /*=============== Global ================*/

    // channel enable
    tmp = 0x1;
    *(volatile unsigned int *)(base + 0x5408) = tmp;
    if(disp)printk("0x5408 [%x]\n", tmp);
    // dma ctrl
    tmp = (global_param->init.dma_ctrl.dma_rc_wait_value) | (job->param.global.dma_fifo_wm << 16) |
          (job->param.global.dma_job_wm << 24);
    *(volatile unsigned int *)(base + 0x543c) = tmp;
    if(disp)printk("0x543c [%x]\n", tmp);
    // output format
    tmp = (job->param.global.tc_format) | (job->param.global.tc_rb_swap << 2);
    *(volatile unsigned int *)(base + 0x5470) = tmp;
    if(disp)printk("0x5470 [%x]\n", tmp);

    // input format, capture mode, capture enable
    tmp = (job->param.global.sca_src_format << 20) | (job->param.global.capture_mode << 26) |
          (1 << 31);
    *(volatile unsigned int *)(base + 0x5404) = tmp;
    printk("0x5404 [%x]\n", tmp);
#endif
    return 0;
}

/*
 *  write value in link list memory
 *  type : 0 (last job)==> write job's next link list pointer as 0,
 *       : 1 (not last job)==> write job's next link list pointer as addr,
 */
int fscaler300_write_lli_cmd(int dev, int addr, fscaler300_job_t *job, int type)
{
    int base = 0, tmp = 0;

    base = priv.engine[dev].fscaler300_reg + SCALER300_LINK_LIST_MEM_BASE;

    if (type == 1) {
        tmp = 0x80040000 + addr;
        *(volatile unsigned int *)(base + job->ll_blk.last_cmd_addr + 4) = tmp;
    }
    else {
        tmp = 0;
        *(volatile unsigned int *)(base + job->ll_blk.last_cmd_addr + 4) = tmp;
    }

    return 0;
}

int fscaler300_init_global(int dev, fscaler300_global_param_t *global_param)
{
    int base = 0, i = 0;
    unsigned int tmp = 0;
#if GM8210
    int path = 4;
#else
    int path = 2;
#endif

    base = priv.engine[dev].fscaler300_reg;

    /* init parameter memory to 0 */
    for (i = 0; i < SCALER300_PARAM_MEMORY_LEN; i = i + 4)
        *(volatile unsigned int *)(base + i) = 0;
    /* init register memory to 0 */
    for (i = 0; i < SCALER300_REG_MEMORY_END; i = i + 4)
        *(volatile unsigned int *)(base + SCALER300_REG_MEMORY_START + i) = 0;
    /* init link list memory to 0 */
    for (i = 0; i < SCALER300_LLI_MEMORY; i = i + 4)
        *(volatile unsigned int *)(base + SCALER300_LINK_LIST_MEM_BASE + i) = 0;

    /*================= Subchannel =================*/
    /* fcs enable/denoise enable */
    tmp = (global_param->init.fcs.enable << 8) | (global_param->init.dn.enable << 9);
    *(volatile unsigned int *)(base + 0x4) = tmp;
    /*================= SC =================*/
    /* rgb2yuv matrix */
    tmp = global_param->init.rgb2yuv_matrix.rgb2yuv_a00 << 24;
    *(volatile unsigned int *)(base + 0x520c) = tmp;

    tmp = (global_param->init.rgb2yuv_matrix.rgb2yuv_a01) |
          (global_param->init.rgb2yuv_matrix.rgb2yuv_a02 << 8) |
          (global_param->init.rgb2yuv_matrix.rgb2yuv_a10 << 16) |
          (global_param->init.rgb2yuv_matrix.rgb2yuv_a11 << 24);
    *(volatile unsigned int *)(base + 0x5210) = tmp;

    tmp = (global_param->init.rgb2yuv_matrix.rgb2yuv_a12) |
          (global_param->init.rgb2yuv_matrix.rgb2yuv_a20 << 8) |
          (global_param->init.rgb2yuv_matrix.rgb2yuv_a21 << 16) |
          (global_param->init.rgb2yuv_matrix.rgb2yuv_a22 << 24);
    *(volatile unsigned int *)(base + 0x5214) = tmp;

    /*================= FCS =================*/
    /* lv0_th/lv1_th/lv2_th */
    tmp = (global_param->init.fcs.lv0_th) | (global_param->init.fcs.lv1_th << 12) |
          (global_param->init.fcs.lv2_th << 20);
    *(volatile unsigned int *)(base + 0x28) = tmp;
    /* lv3_th/lv4_th/lv2_th */
    tmp = (global_param->init.fcs.lv3_th) | (global_param->init.fcs.lv4_th << 8) |
          (global_param->init.fcs.grey_th << 16);
    *(volatile unsigned int *)(base + 0x2c) = tmp;
    /*================= Denoise =================*/
    /* geomatric/similarity strength/adaptive/adaptive step */
    tmp = (global_param->init.dn.geomatric_str) | (global_param->init.dn.similarity_str << 3) |
          (global_param->init.dn.adp << 6) | (global_param->init.dn.adp_step << 7);
    *(volatile unsigned int *)(base + 0x70) = tmp;
    /*================= Sharpness =================*/
    /* adaptive/radius/amount/threshold/adaaptive start/adaptive step */
    for (i = 0; i < path; i++) {
        tmp = (global_param->init.sharpness[i].adp << 5) |
              (global_param->init.sharpness[i].radius << 6) |
              (global_param->init.sharpness[i].amount << 9) |
              (global_param->init.sharpness[i].threshold << 15) |
              (global_param->init.sharpness[i].adp_start << 21) |
              (global_param->init.sharpness[i].adp_step << 27);
        *(volatile unsigned int *)(base + 0x84 + (i * 0x10)) = tmp;
    }
    /*================= OSD =================*/
#if GM8210
    // font smooth
    tmp = (global_param->init.osd_smooth.onoff) |
          (global_param->init.osd_smooth.level << 1);
    *(volatile unsigned int *)(base + 0xb8) = tmp;
#endif
    /*================= OSD Palette =================*/
    for (i = 0; i < SCALER300_PALETTE_MAX; i++) {
        tmp = global_param->init.palette[i].crcby;
        *(volatile unsigned int *)(base + 0x5300 + (i * 0x4)) = tmp;
    }
    /*================= global ================*/
    // channel enable
    tmp = 0x1;
    *(volatile unsigned int *)(base + 0x5408) = tmp;
    // dma control
    tmp = (global_param->init.dma_ctrl.dma_lnbst_itv) |
          (global_param->init.dma_ctrl.dma_wc_wait_value << 16);
    *(volatile unsigned int *)(base + 0x5438) = tmp;
    // dma control
    tmp = global_param->init.dma_ctrl.dma_rc_wait_value;
    *(volatile unsigned int *)(base + 0x543c) = tmp;
    /*=============== TC ================*/
    /* yuv2rgb matrix */
    tmp = global_param->init.yuv2rgb_matrix.yuv2rgb_a00 << 24;
    *(volatile unsigned int *)(base + 0x5474) = tmp;

    tmp = (global_param->init.yuv2rgb_matrix.yuv2rgb_a01) |
          (global_param->init.yuv2rgb_matrix.yuv2rgb_a02 << 8) |
          (global_param->init.yuv2rgb_matrix.yuv2rgb_a10 << 16) |
          (global_param->init.yuv2rgb_matrix.yuv2rgb_a11 << 24);
    *(volatile unsigned int *)(base + 0x5478) = tmp;

    tmp = (global_param->init.yuv2rgb_matrix.yuv2rgb_a12) |
          (global_param->init.yuv2rgb_matrix.yuv2rgb_a20 << 8) |
          (global_param->init.yuv2rgb_matrix.yuv2rgb_a21 << 16) |
          (global_param->init.yuv2rgb_matrix.yuv2rgb_a22 << 24);
    *(volatile unsigned int *)(base + 0x547c) = tmp;

    /*================= interrupt mask ================*/
    /* DMA Overflow/DMA Job Overflow/DMAC disable status/DMA write channel respose fail/
       DMA read channel respose fail/SD Job Overflow/DMA command prefix error */
    tmp = (global_param->init.int_mask.dma_ovf) |
          (global_param->init.int_mask.dma_job_ovf << 1) |
          (global_param->init.int_mask.dma_dis_status << 2) |
          (global_param->init.int_mask.dma_wc_respfail << 3) |
          (global_param->init.int_mask.dma_rc_respfail << 4) |
          (global_param->init.int_mask.sd_job_ovf << 5) |
          (global_param->init.int_mask.dma_wc_pfx_err << 6);
    *(volatile unsigned int *)(base + 0x5528) = tmp;
    /* Link List Done */
    *(volatile unsigned int *)(base + 0x5530) = global_param->init.int_mask.ll_done;
    /* SD Fail */
    *(volatile unsigned int *)(base + 0x5538) = global_param->init.int_mask.sd_fail;
    /* DMA Write Done */
    *(volatile unsigned int *)(base + 0x5540) = global_param->init.int_mask.dma_wc_done;
    /* DMA Read Done/Frame end */
    tmp = (global_param->init.int_mask.dma_rc_done << 16) |
          (global_param->init.int_mask.frame_end << 31);
    *(volatile unsigned int *)(base + 0x5544) = tmp;

    return 0;
}


