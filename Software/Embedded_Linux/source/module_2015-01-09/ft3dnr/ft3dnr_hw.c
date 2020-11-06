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

#include "ft3dnr.h"

#ifdef VIDEOGRAPH_INC
#include "ft3dnr_vg.h"
#include <log.h>    //include log system "printm","damnit"...
#include <video_entity.h>   //include video entity manager
#define MODULE_NAME         "DN"    //<<<<<<<<<< Modify your module name (two bytes) >>>>>>>>>>>>>>
#endif

void ft3dnr_init_global(void)
{
    u32 base = 0, tmp = 0;
    int i;
    tmnr_param_t *tmnr = &priv.default_param.tmnr;
    ft3dnr_rvlut_t *rvlut = &priv.default_param.rvlut;

    base = priv.engine.ft3dnr_reg;

    /* init register memory to 0 */
    for (i = 0; i < FT3DNR_REG_MEM_LEN; i = i + 4)
        *(volatile unsigned int *)(base + i) = 0;
    /* init link list memory to 0 */
    for (i = 0; i < FT3DNR_LL_MEM; i = i + 4)
        *(volatile unsigned int *)(base + FT3DNR_LL_MEM_BASE + i) = 0;

    /* wc/rc wait value */
    tmp = priv.engine.wc_wait_value | (priv.engine.rc_wait_value << 16);
    *(volatile unsigned int *)(base + 0xdc) = tmp;

    /* var addr */
    //tmp = priv.engine.var_buf.paddr;
    //*(volatile unsigned int *)(base + 0xd8) = tmp;

    tmp = tmnr->learn_rate | (tmnr->his_factor << 8) | (tmnr->edge_th << 16);
    *(volatile unsigned int *)(base + 0x98) = tmp;
    /* rlut */
    tmp = rvlut->rlut[0];
    *(volatile unsigned int *)(base + 0x9c) = tmp;
    tmp = rvlut->rlut[1];
    *(volatile unsigned int *)(base + 0xa0) = tmp;
    tmp = rvlut->rlut[2];
    *(volatile unsigned int *)(base + 0xa4) = tmp;
    tmp = rvlut->rlut[3];
    *(volatile unsigned int *)(base + 0xa8) = tmp;
    tmp = rvlut->rlut[4];
    *(volatile unsigned int *)(base + 0xac) = tmp;
    tmp = rvlut->rlut[5];
    *(volatile unsigned int *)(base + 0xb0) = tmp;
    /* vlut */
    tmp = rvlut->vlut[0];
    *(volatile unsigned int *)(base + 0xb4) = tmp;
    tmp = rvlut->vlut[1];
    *(volatile unsigned int *)(base + 0xb8) = tmp;
    tmp = rvlut->vlut[2];
    *(volatile unsigned int *)(base + 0xbc) = tmp;
    tmp = rvlut->vlut[3];
    *(volatile unsigned int *)(base + 0xc0) = tmp;
    tmp = rvlut->vlut[4];
    *(volatile unsigned int *)(base + 0xc4) = tmp;
    tmp = rvlut->vlut[5];
    *(volatile unsigned int *)(base + 0xc8) = tmp;
}

void ft3dnr_set_lli_mrnr(ft3dnr_job_t *job, u32 ret_addr)
{
    u32 base = 0, tmp = 0, offset = 0;
    int mrnr_num = job->ll_blk.mrnr_num;
    mrnr_param_t *mrnr = NULL;
    int record_setting = 0;

    if (job->param.dim.src_bg_width == priv.curr_max_dim.src_bg_width)
        record_setting = 1;

    /* select mrnr from isp or decoder or sensor */
    if (job->param.src_type == SRC_TYPE_ISP)
        mrnr = &job->param.mrnr;
    if (job->param.src_type == SRC_TYPE_DECODER || job->param.src_type == SRC_TYPE_SENSOR)
        mrnr = &priv.default_param.mrnr;

    base = priv.engine.ft3dnr_reg;
    offset = FT3DNR_MRNR_MEM_BASE + 0x100 * mrnr_num;
    // 0x700
    tmp = mrnr->Y_L_edg_th[0][0] | (mrnr->Y_L_edg_th[0][1] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000010;
    if (record_setting) *(u32 *)&priv.curr_reg[0x10] = tmp;
    offset+= 8; // 0x708
    tmp = mrnr->Y_L_edg_th[0][2] | (mrnr->Y_L_edg_th[0][3] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000014;
    if (record_setting) *(u32 *)&priv.curr_reg[0x14] = tmp;
    offset+= 8; // 0x710
    tmp = mrnr->Y_L_edg_th[0][4] | (mrnr->Y_L_edg_th[0][5] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000018;
    if (record_setting) *(u32 *)&priv.curr_reg[0x18] = tmp;
    offset+= 8; // 0x718
    tmp = mrnr->Y_L_edg_th[0][6] | (mrnr->Y_L_edg_th[0][7] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e00001c;
    if (record_setting) *(u32 *)&priv.curr_reg[0x1c] = tmp;
    offset+= 8; // 0x720
    tmp = mrnr->Y_L_edg_th[1][0] | (mrnr->Y_L_edg_th[1][1] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000020;
    if (record_setting) *(u32 *)&priv.curr_reg[0x20] = tmp;
    offset+= 8; // 0x728
    tmp = mrnr->Y_L_edg_th[1][2] | (mrnr->Y_L_edg_th[1][3] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000024;
    if (record_setting) *(u32 *)&priv.curr_reg[0x24] = tmp;
    offset+= 8; // 0x730
    tmp = mrnr->Y_L_edg_th[1][4] | (mrnr->Y_L_edg_th[1][5] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000028;
    if (record_setting) *(u32 *)&priv.curr_reg[0x28] = tmp;
    offset+= 8; // 0x738
    tmp = mrnr->Y_L_edg_th[1][6] | (mrnr->Y_L_edg_th[1][7] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e00002c;
    if (record_setting) *(u32 *)&priv.curr_reg[0x2c] = tmp;
    offset+= 8; // 0x740
    tmp = mrnr->Y_L_edg_th[2][0] | (mrnr->Y_L_edg_th[2][1] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000030;
    if (record_setting) *(u32 *)&priv.curr_reg[0x30] = tmp;
    offset+= 8; // 0x748
    tmp = mrnr->Y_L_edg_th[2][2] | (mrnr->Y_L_edg_th[2][3] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000034;
    if (record_setting) *(u32 *)&priv.curr_reg[0x34] = tmp;
    offset+= 8; // 0x750
    tmp = mrnr->Y_L_edg_th[2][4] | (mrnr->Y_L_edg_th[2][5] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000038;
    if (record_setting) *(u32 *)&priv.curr_reg[0x38] = tmp;
    offset+= 8; // 0x758
    tmp = mrnr->Y_L_edg_th[2][6] | (mrnr->Y_L_edg_th[2][7] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e00003c;
    if (record_setting) *(u32 *)&priv.curr_reg[0x3c] = tmp;
    offset+= 8; // 0x760
    tmp = mrnr->Y_L_edg_th[3][0] | (mrnr->Y_L_edg_th[3][1] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000040;
    if (record_setting) *(u32 *)&priv.curr_reg[0x40] = tmp;
    offset+= 8; // 0x768
    tmp = mrnr->Y_L_edg_th[3][2] | (mrnr->Y_L_edg_th[3][3] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000044;
    if (record_setting) *(u32 *)&priv.curr_reg[0x44] = tmp;
    offset+= 8; // 0x770
    tmp = mrnr->Y_L_edg_th[3][4] | (mrnr->Y_L_edg_th[3][5] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000048;
    if (record_setting) *(u32 *)&priv.curr_reg[0x48] = tmp;
    offset+= 8; // 0x778
    tmp = mrnr->Y_L_edg_th[3][6] | (mrnr->Y_L_edg_th[3][7] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e00004c;
    if (record_setting) *(u32 *)&priv.curr_reg[0x4c] = tmp;
    offset+= 8; // 0x780
    tmp = mrnr->Cb_L_edg_th[0] | (mrnr->Cb_L_edg_th[1] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000050;
    if (record_setting) *(u32 *)&priv.curr_reg[0x50] = tmp;
    offset+= 8; // 0x788
    tmp = mrnr->Cb_L_edg_th[2] | (mrnr->Cb_L_edg_th[3] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000054;
    if (record_setting) *(u32 *)&priv.curr_reg[0x54] = tmp;
    offset+= 8; // 0x790
    tmp = mrnr->Cr_L_edg_th[0] | (mrnr->Cr_L_edg_th[1] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000058;
    if (record_setting) *(u32 *)&priv.curr_reg[0x58] = tmp;
    offset+= 8; // 0x798
    tmp = mrnr->Cr_L_edg_th[2] | (mrnr->Cr_L_edg_th[3] << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e00005c;
    if (record_setting) *(u32 *)&priv.curr_reg[0x5c] = tmp;
    offset+= 8; // 0x7a0
    tmp = mrnr->Y_L_smo_th[0][0] | (mrnr->Y_L_smo_th[0][1] << 8) | (mrnr->Y_L_smo_th[0][2] << 16) | (mrnr->Y_L_smo_th[0][3] << 24);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000060;
    if (record_setting) *(u32 *)&priv.curr_reg[0x60] = tmp;
    offset+= 8; // 0x7a8
    tmp = mrnr->Y_L_smo_th[0][4] | (mrnr->Y_L_smo_th[0][5] << 8) | (mrnr->Y_L_smo_th[0][6] << 16) | (mrnr->Y_L_smo_th[0][7] << 24);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000064;
    if (record_setting) *(u32 *)&priv.curr_reg[0x64] = tmp;
    offset+= 8; // 0x7b0
    tmp = mrnr->Y_L_smo_th[1][0] | (mrnr->Y_L_smo_th[1][1] << 8) | (mrnr->Y_L_smo_th[1][2] << 16) | (mrnr->Y_L_smo_th[1][3] << 24);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000068;
    if (record_setting) *(u32 *)&priv.curr_reg[0x68] = tmp;
    offset+= 8; // 0x7b8
    tmp = mrnr->Y_L_smo_th[1][4] | (mrnr->Y_L_smo_th[1][5] << 8) | (mrnr->Y_L_smo_th[1][6] << 16) | (mrnr->Y_L_smo_th[1][7] << 24);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e00006c;
    if (record_setting) *(u32 *)&priv.curr_reg[0x6c] = tmp;
    offset+= 8; // 0x7c0
    tmp = mrnr->Y_L_smo_th[2][0] | (mrnr->Y_L_smo_th[2][1] << 8) | (mrnr->Y_L_smo_th[2][2] << 16) | (mrnr->Y_L_smo_th[2][3] << 24);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000070;
    if (record_setting) *(u32 *)&priv.curr_reg[0x70] = tmp;
    offset+= 8; // 0x7c8
    tmp = mrnr->Y_L_smo_th[2][4] | (mrnr->Y_L_smo_th[2][5] << 8) | (mrnr->Y_L_smo_th[2][6] << 16) | (mrnr->Y_L_smo_th[2][7] << 24);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000074;
    if (record_setting) *(u32 *)&priv.curr_reg[0x74] = tmp;
    offset+= 8; // 0x7d0
    tmp = mrnr->Y_L_smo_th[3][0] | (mrnr->Y_L_smo_th[3][1] << 8) | (mrnr->Y_L_smo_th[3][2] << 16) | (mrnr->Y_L_smo_th[3][3] << 24);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000078;
    if (record_setting) *(u32 *)&priv.curr_reg[0x78] = tmp;
    offset+= 8; // 0x7d8
    tmp = mrnr->Y_L_smo_th[3][4] | (mrnr->Y_L_smo_th[3][5] << 8) | (mrnr->Y_L_smo_th[3][6] << 16) | (mrnr->Y_L_smo_th[3][7] << 24);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e00007c;
    if (record_setting) *(u32 *)&priv.curr_reg[0x7c] = tmp;
    offset+= 8; // 0x7e0
    tmp = mrnr->Cb_L_smo_th[0] | (mrnr->Cb_L_smo_th[1] << 8) | (mrnr->Cb_L_smo_th[2] << 16) | (mrnr->Cb_L_smo_th[3] << 24);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000080;
    if (record_setting) *(u32 *)&priv.curr_reg[0x80] = tmp;
    offset+= 8; // 0x7e8
    tmp = mrnr->Cr_L_smo_th[0] | (mrnr->Cr_L_smo_th[1] << 8) | (mrnr->Cr_L_smo_th[2] << 16) | (mrnr->Cr_L_smo_th[3] << 24);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000084;
    if (record_setting) *(u32 *)&priv.curr_reg[0x84] = tmp;
    offset+= 8; // 0x7f0
    tmp = mrnr->Y_L_nr_str[0] | (mrnr->Y_L_nr_str[1] << 4) | (mrnr->Y_L_nr_str[2] << 8) | (mrnr->Y_L_nr_str[3] << 12) |
            (mrnr->C_L_nr_str[0] << 16) | (mrnr->C_L_nr_str[1] << 20) | (mrnr->C_L_nr_str[2] << 24) | (mrnr->C_L_nr_str[3] << 28);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000088;
    if (record_setting) *(u32 *)&priv.curr_reg[0x88] = tmp;
    // jump to next update pointer cmd
    offset += 8;
    tmp = 0x60000000 + FT3DNR_LL_MEM_BASE + ret_addr;
    *(volatile unsigned int *)(base + offset) = 0;
    *(volatile unsigned int *)(base + offset + 0x4) = tmp;
}


int ft3dnr_set_lli_blk(ft3dnr_job_t *job, int chain_cnt, int last_job)
{
    u32 base = 0, offset = 0, tmp = 0;
    int blk = job->ll_blk.blk_num;
    int mrnr = job->ll_blk.mrnr_num;
    ft3dnr_ctrl_t   *ctrl = &job->param.ctrl;
    ft3dnr_dim_t    *dim = &job->param.dim;
    ft3dnr_his_t    *his = &job->param.his;
    ft3dnr_dma_t    *dma = &job->param.dma;
    tmnr_param_t   *tmnr = &job->param.tmnr;
    int record_setting = 0;

    if (dim->src_bg_width == priv.curr_max_dim.src_bg_width)
        record_setting = 1;

    if (job->fops && job->fops->mark_engine_start)
        job->fops->mark_engine_start(job);

    base = priv.engine.ft3dnr_reg + FT3DNR_LL_MEM_BASE;
    offset = 0x80 * blk;
    // spatial/temporal/edg/learn enable
    offset += 8;
    tmp = ctrl->spatial_en | (ctrl->temporal_en << 1) | (ctrl->tnr_edg_en << 2) | (ctrl->tnr_learn_en << 3) |
          (priv.engine.ycbcr.src_yc_swap << 4) | (priv.engine.ycbcr.src_cbcr_swap << 5) |
          (priv.engine.ycbcr.dst_yc_swap << 6) | (priv.engine.ycbcr.dst_cbcr_swap << 7) |
          (priv.engine.ycbcr.ref_yc_swap << 8) | (priv.engine.ycbcr.ref_cbcr_swap << 9);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x46000000;
    if (record_setting) *(u32 *)&priv.curr_reg[0x00] = tmp;
    // src width/height
    offset += 8;
    tmp = (dim->src_bg_width | dim->src_bg_height << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000004;
    if (record_setting) *(u32 *)&priv.curr_reg[0x04] = tmp;
    // start_x/start_y
    offset += 8;
    tmp = (dim->src_x | dim->src_y << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000008;
    if (record_setting) *(u32 *)&priv.curr_reg[0x08] = tmp;
    // ori width/height
    offset += 8;
    tmp = (dim->nr_width | dim->nr_height << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e00000c;
    if (record_setting) *(u32 *)&priv.curr_reg[0x0c] = tmp;
    // norm/right th
    offset += 8;
    tmp = (his->norm_th | his->right_th << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000090;
    if (record_setting) *(u32 *)&priv.curr_reg[0x90] = tmp;
    // bot/corner th
    offset += 8;
    tmp = (his->bot_th | his->corner_th << 16);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e000094;
    if (record_setting) *(u32 *)&priv.curr_reg[0x94] = tmp;
    // tmnr
    offset += 8;
    tmp = (tmnr->Y_var) | (tmnr->Cb_var << 8) | (tmnr->Cr_var << 16) | (tmnr->dc_offset << 24) | (tmnr->auto_recover << 30) | (tmnr->rapid_recover << 31);
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e00008c;
    if (record_setting) *(u32 *)&priv.curr_reg[0x8c] = tmp;
    // update pointer cmd
    offset += 8;
    tmp = 0x60000000 + FT3DNR_MRNR_MEM_BASE + (mrnr * 0x100);
    *(volatile unsigned int *)(base + offset) = 0;
    *(volatile unsigned int *)(base + offset + 0x4) = tmp;
    offset += 8;
    ft3dnr_set_lli_mrnr(job, offset);

    // src addr
    tmp = dma->src_addr;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e0000cc;
    // dst addr
    offset += 8;
    tmp = dma->dst_addr;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e0000d0;
    // ref addr
    offset += 8;
    tmp = dma->ref_addr;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e0000d4;
#if 1
    // var addr
    offset += 8;
    tmp = dma->var_addr;
    *(volatile unsigned int *)(base + offset) = tmp;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x5e0000d8;
#endif
    // jump to next cmd (null cmd)
    offset += 8;
    tmp = 0x80000000 + FT3DNR_LL_MEM_BASE + job->ll_blk.next_blk * 0x80;
    *(volatile unsigned int *)(base + offset) = 0x0;
    *(volatile unsigned int *)(base + offset + 0x4) = tmp;
    // null cmd
    offset = job->ll_blk.next_blk * 0x80;
    *(volatile unsigned int *)(base + offset) = 0;
    *(volatile unsigned int *)(base + offset + 0x4) = 0x00000000;
    /* this chain has 2 jobs, second job can write status register */
#if 0
    if (chain_cnt == 2 && last_job) {
        base = priv.engine.ft3dnr_reg + FT3DNR_LL_MEM_BASE + 0x80 * blk;
        *(volatile unsigned int *)(base + 0) = 0x00000000;
        *(volatile unsigned int *)(base + 0x4) = 0x20000000;
    }
#endif
    return 0;
}


void ft3dnr_job_finish(ft3dnr_job_t *job)
{
    if (job->fops && job->fops->post_process)
        job->fops->post_process(job);

     ft3dnr_job_free(job);
}

void ft3dnr_write_status(void)
{
    u32 base = 0, offset = 0;
    ft3dnr_job_t *job, *ne;
    unsigned long flags;

    base = priv.engine.ft3dnr_reg + FT3DNR_LL_MEM_BASE;

    /* trigger timer */
    spin_lock_irqsave(&priv.lock, flags);
    mod_timer(&priv.engine.timer, jiffies + priv.engine.timeout);

    list_for_each_entry_safe_reverse(job, ne, &priv.engine.slist, job_list) {
        offset = job->ll_blk.status_addr;
        *(volatile unsigned int *)(base + offset) = 0;
        *(volatile unsigned int *)(base + offset + 0x4) = 0x20000000;
    }
    spin_unlock_irqrestore(&priv.lock, flags);
}

int ft3dnr_fire(void)
{
    u32 tmp = 0;
    u32 base = 0;
    unsigned long flags;

    base = priv.engine.ft3dnr_reg;

    /* trigger timer */
    spin_lock_irqsave(&priv.lock, flags);
    mod_timer(&priv.engine.timer, jiffies + priv.engine.timeout);
    spin_unlock_irqrestore(&priv.lock, flags);

    tmp = *(volatile unsigned int *)(base);

    if (tmp & 0x1000000) {
        printk("[FT3DNR] Fire Bit not clear!\n");
        return -1;
    }
    else {
        tmp |= (1 << 24);
        *(volatile unsigned int *)(base) = tmp;
    }
    return 0;
}

void ft3dnr_dump_reg(void)
{
    int i = 0;
    unsigned int address = 0, size = 0;

    address = priv.engine.ft3dnr_reg;   ////<<<<<<<< Update your register dump here >>>>>>>>>//issue
    size=0xe0;  //<<<<<<<< Update your register dump here >>>>>>>>>
    for (i = 0; i < size; i = i + 0x10) {
        printk("0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address+i),*(unsigned int *)(address+i),
            *(unsigned int *)(address+i+4),*(unsigned int *)(address+i+8),*(unsigned int *)(address+i+0xc));
    }
    printk("\n");
    printk("link list memory content\n");
    address = priv.engine.ft3dnr_reg + 0x400;
    size = 0x400;
    for (i = 0; i < size; i = i + 0x10) {
        printk("0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address+i),*(unsigned int *)(address+i),
            *(unsigned int *)(address+i+4),*(unsigned int *)(address+i+8),*(unsigned int *)(address+i+0xc));
    }
}

irqreturn_t ft3dnr_interrupt(int irq, void *devid)
{
    int status0 = 0, status1 = 0;
    int reg_0xe0 = 0;
    u32 base = 0;
    ft3dnr_job_t *job, *ne;
    int wjob_cnt = 0;

    base = priv.engine.ft3dnr_reg;
    reg_0xe0 = *(volatile unsigned int *)(base + 0xe0);

    /* Clear Interrupt Status */
    *(volatile unsigned int *)(base + 0xe0) = 0xc8;

    //if ((reg_0xe0 & 0x8) == 0x8)
      //  printk("[FT3DRN_IRQ] frame done %08X\n", reg_0xe0);

    //if ((reg_0xe0 & 0x88) == 0x88)
      //  printk("[FT3DRN_IRQ] link list done %08X\n", reg_0xe0);

    /* Check Error Status */
    if((reg_0xe0 & BIT6))
        printk("[FT3DRN_IRQ] invalid link list cmd!!\n");

    // find out which jobs have done, callback to V.G
    list_for_each_entry_safe(job, ne, &priv.engine.wlist, job_list) {
        if (job->status == JOB_STS_DONE)
            continue;

        status0 = *(volatile unsigned int *)(base + FT3DNR_LL_MEM_BASE + job->ll_blk.status_addr);
        status1 = *(volatile unsigned int *)(base + FT3DNR_LL_MEM_BASE + job->ll_blk.status_addr + 4);

        if ((status1 & 0x20000000) != 0x20000000) {// not status command
            printk("[FT3DRN_IRQ] it's not status command [%x] value %x, something wrong !!\n", job->ll_blk.status_addr, status1);
            ft3dnr_dump_reg();
            damnit(MODULE_NAME);
        }
        else {
            if ((status1 & 0x1) == 0x1) {
                job->status = JOB_STS_DONE;
                // callback the job
                ft3dnr_job_finish(job);
                mod_timer(&priv.engine.timer, jiffies + priv.engine.timeout);
            }
            else { // job not yet do
                //printk(KERN_DEBUG "[FT3DRN_IRQ] job not yet do [%d]\n", job->job_id);
                break;
            }
        }
    }

    list_for_each_entry_safe(job, ne, &priv.engine.wlist, job_list) {
        if (job->status != JOB_STS_DONE)
            wjob_cnt++;
    }

    // no jobs in working list, HW idle
    if (wjob_cnt == 0) {
        UNLOCK_ENGINE();
        mark_engine_finish();
        del_timer(&priv.engine.timer);
    }

    return IRQ_HANDLED;
}

void ft3dnr_sw_reset(void)
{
    u32 base = 0, tmp = 0;
    int cnt = 10;

    base = priv.engine.ft3dnr_reg;

    /* DMA DISABLE */
    tmp = *(volatile unsigned int *)(base + 0xe0);
    tmp |= BIT1;
    *(volatile unsigned int *)(base + 0xe0) = tmp;

    /* polling DMA disable status */
    while (((*(volatile unsigned int *)(base + 0xe0) & BIT0) != BIT0) && (cnt-- > 0))
        udelay(10);

    if ((*(volatile unsigned int *)(base + 0xe0) & BIT0) != BIT0)
        printk("[FT3DNR] DMA disable failed!!\n");
    else
        printk("[FT3DNR] DMA disable done!!\n");

    /* clear dma disable */
    tmp = *(volatile unsigned int *)(base + 0xe0);
    tmp &= ~BIT1;
    *(volatile unsigned int *)(base + 0xe0) = tmp;

    /* sw reset */
    tmp = *(volatile unsigned int *)(base + 0xe0);
    tmp |= BIT2;
    *(volatile unsigned int *)(base + 0xe0) = tmp;

    udelay(10);

    if((*(volatile unsigned int *)(base + 0xe0) & BIT2) != 0x0)
        printk("[FT3DNR] SW rest failed!!\n");
    else
        printk("[FT3DNR] SW reset done!!\n");

    /* after software reset, 3dnr become normal mode */
    priv.engine.ll_mode = 0;
}

void ft3dnr_set_tmnr_rlut(void)
{
    u32 base = 0;
    int i = 0;

    base = priv.engine.ft3dnr_reg;

    for (i = 0; i < 6; i++) {
        *(volatile unsigned int *)(base + 0x9c + (i << 2)) = priv.isp_param.rvlut.rlut[i];
        *(u32 *)&priv.curr_reg[0x9c + (i << 2)] = priv.isp_param.rvlut.rlut[i];
    }
}

void ft3dnr_set_tmnr_vlut(void)
{
    u32 base = 0;
    int i = 0;

    base = priv.engine.ft3dnr_reg;

    for (i = 0; i < 6; i++) {
        *(volatile unsigned int *)(base + 0xb4 + (i << 2)) = priv.isp_param.rvlut.vlut[i];
        *(u32 *)&priv.curr_reg[0xb4 + (i << 2)] = priv.isp_param.rvlut.vlut[i];
    }
}

void ft3dnr_set_tmnr_learn_rate(u8 rate)
{
    u32 base = 0, tmp = 0;

    base = priv.engine.ft3dnr_reg;

    tmp = *(volatile unsigned int *)(base + 0x98);
    /* bit 0~6 */
    tmp &= (0xFFFFFF80);
    tmp |= rate;

    *(volatile unsigned int *)(base + 0x98) = tmp;
    *(u32 *)&priv.curr_reg[0x98] = tmp;
}

void ft3dnr_set_tmnr_his_factor(u8 his)
{
    u32 base = 0, tmp = 0;

    base = priv.engine.ft3dnr_reg;

    tmp = *(volatile unsigned int *)(base + 0x98);
    /* bit 8~11 */
    tmp &= (0xFFFFF0FF);
    tmp |= (his << 8);

    *(volatile unsigned int *)(base + 0x98) = tmp;
    *(u32 *)&priv.curr_reg[0x98] = tmp;
}

void ft3dnr_set_tmnr_edge_th(u16 threshold)
{
    u32 base = 0, tmp = 0;

    base = priv.engine.ft3dnr_reg;

    tmp = *(volatile unsigned int *)(base + 0x98);
    /* bit 16~31 */
    tmp &= (0x0000FFFF);
    tmp |= (threshold << 16);

    *(volatile unsigned int *)(base + 0x98) = tmp;
    *(u32 *)&priv.curr_reg[0x98] = tmp;
}

int ft3dnr_write_register(ft3dnr_job_t *job)
{
    u32 base = 0, tmp = 0;
    ft3dnr_ctrl_t   *ctrl = &job->param.ctrl;
    ft3dnr_dim_t    *dim = &job->param.dim;
    ft3dnr_his_t    *his = &job->param.his;
    ft3dnr_dma_t    *dma = &job->param.dma;

    base = priv.engine.ft3dnr_reg;

    tmp = ctrl->spatial_en | (ctrl->temporal_en << 1) | (ctrl->tnr_edg_en << 2) | (ctrl->tnr_learn_en << 3);
    *(volatile unsigned int *)(base + 0x0) = tmp;
    // src width/height
    tmp = (dim->src_bg_width | dim->src_bg_height << 16);
    *(volatile unsigned int *)(base + 0x4) = tmp;
    // start_x/start_y
    tmp = (dim->src_x | dim->src_y << 16);
    *(volatile unsigned int *)(base + 0x8) = tmp;
    // ori width/height
    tmp = (dim->nr_width | dim->nr_height << 16);
    *(volatile unsigned int *)(base + 0xc) = tmp;
    // norm/right th
    tmp = (his->norm_th | his->right_th << 16);
    *(volatile unsigned int *)(base + 0x90) = tmp;
    // bot/corner th
    tmp = (his->bot_th | his->corner_th << 16);
    *(volatile unsigned int *)(base + 0x94) = tmp;
    // src addr
    tmp = dma->src_addr;
    *(volatile unsigned int *)(base + 0xcc) = tmp;
    // dst addr
    tmp = dma->dst_addr;
    *(volatile unsigned int *)(base + 0xd0) = tmp;
    // ref addr
    tmp = dma->ref_addr;
    *(volatile unsigned int *)(base + 0xd4) = tmp;
    // var addr
    tmp = dma->var_addr;
    *(volatile unsigned int *)(base + 0xd8) = tmp;

    return 0;
}

