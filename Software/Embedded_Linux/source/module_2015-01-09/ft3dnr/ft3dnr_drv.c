#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/times.h>
#include <linux/synclink.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include "frammap_if.h"
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "ft3dnr.h"
#include "util.h"

#ifdef VIDEOGRAPH_INC
#include "ft3dnr_vg.h"
#endif

#include <log.h>    //include log system "printm","damnit"...
#include <video_entity.h>   //include video entity manager

#define MODULE_NAME         "DN"    //<<<<<<<<<< Modify your module name (two bytes) >>>>>>>>>>>>>>

/* module parameter */
/* declare YCSWAP number */
int src_yc_swap = 1;
module_param(src_yc_swap, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(src_yc_swap, "source yc swap");

int src_cbcr_swap = 0;
module_param(src_cbcr_swap, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(src_cbcr_swap, "source cbcr swap");

int dst_yc_swap = 1;
module_param(dst_yc_swap, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(dst_yc_swap, "destination yc swap");

int dst_cbcr_swap = 0;
module_param(dst_cbcr_swap, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(dst_cbcr_swap, "destination cbcr swap");

int ref_yc_swap = 1;
module_param(ref_yc_swap, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ref_yc_swap, "reference yc swap");

int ref_cbcr_swap = 0;
module_param(ref_cbcr_swap, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ref_cbcr_swap, "reference cbcr swap");

int pwm = 0;
module_param(pwm, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(pwm, "pwm");

char *res_cfg = "";
module_param(res_cfg, charp, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(res_cfg, "set resolution configuration");

int max_minors = 0;
module_param(max_minors, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(max_minors, "set max minors driver supported");

char *config_path = "/mnt/mtd/";
module_param(config_path, charp, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(config_path, "set path for vg config file");

extern int platform_init(void);
extern int platform_exit(void);
extern int platform_set_pwm(u32 pwm);

ft3dnr_priv_t priv;
#ifdef USE_WQ
struct workqueue_struct *job_workq;
static DECLARE_DELAYED_WORK(process_job_work, 0);
#endif
extern irqreturn_t ft3dnr_interrupt(int irq, void *devid);
extern unsigned int callback_period;
#ifdef USE_KTHREAD
wait_queue_head_t add_thread_wait;
int add_wakeup_event = 0;
#endif
static struct proc_dir_entry *entity_proc;

/* each resolution's width/height, reference from vg ioctl/spec_parser.c */
static struct res_base_info_t res_base_info[MAX_RES_IDX] = {
    { "8M",    ALIGN16_UP(3840), ALIGN16_UP(2160)},  // 3840 x 2160  : 8294400
    { "7M",    ALIGN16_UP(3264), ALIGN16_UP(2176)},  // 3264 x 2176  : 7102464
    { "6M",    ALIGN16_UP(3072), ALIGN16_UP(2048)},  // 3072 x 2048  : 6291456
    { "5M",    ALIGN16_UP(2592), ALIGN16_UP(1944)},  // 2596 x 1952  : 5067392
    { "4M",    ALIGN16_UP(2304), ALIGN16_UP(1728)},  // 2304 x 1728  : 3981312
    { "3M",    ALIGN16_UP(2048), ALIGN16_UP(1536)},  // 2048 x 1536  : 3145728
    { "2M",    ALIGN16_UP(1920), ALIGN16_UP(1080)},  // 1920 x 1088  : 2088960
    { "1080P", ALIGN16_UP(1920), ALIGN16_UP(1080)},  // 1920 x 1088  : 2088960
    { "1.3M",  ALIGN16_UP(1280), ALIGN16_UP(1024)},  // 1280 x 1024  : 1310720
    { "1.2M",  ALIGN16_UP(1280), ALIGN16_UP(960)},   // 1280 x 960   : 1228800
    { "1080I", ALIGN16_UP(1920), ALIGN16_UP(540)},   // 1920 x 544   : 1044480
    { "1M",    ALIGN16_UP(1280), ALIGN16_UP(800)},   // 1280 x 800   : 1024000
    { "720P",  ALIGN16_UP(1280), ALIGN16_UP(720)},   // 1280 x 720   : 921600
    { "960H",  ALIGN16_UP(960),  ALIGN16_UP(576)},   // 960  x 576   : 552960
    { "SVGA",  ALIGN16_UP(800),  ALIGN16_UP(600)},   // 800  x 608   : 486400
    { "720I",  ALIGN16_UP(1280), ALIGN16_UP(360)},   // 1280 x 368   : 471040
    { "D1",    ALIGN16_UP(720),  ALIGN16_UP(576)},   // 720  x 576   : 414720
    { "VGA",   ALIGN16_UP(640),  ALIGN16_UP(480)},   // 640  x 480   : 307200
    { "nHD",   ALIGN16_UP(640),  ALIGN16_UP(360)},   // 640  x 368   : 235520
    { "2CIF",  ALIGN16_UP(360),  ALIGN16_UP(596)},   // 368  x 596   : 219328
    { "HD1",   ALIGN16_UP(720),  ALIGN16_UP(288)},   // 720  x 288   : 207360
    { "CIF",   ALIGN16_UP(360),  ALIGN16_UP(288)},   // 368  x 288   : 105984
    { "QCIF",  ALIGN16_UP(176),  ALIGN16_UP(144)},   // 176  x 144   : 25344
    { "NONE",                0,                0},   // reserve
    { "NONE",                0,                0},   // reserve
    { "NONE",                0,                0},   // reserve
    { "NONE",                0,                0},   // reserve
    { "NONE",                0,                0}    // reserve
};

/*
 * Create engine and FIFO
 */
int add_engine(unsigned long addr, unsigned long irq)
{
    priv.engine.ft3dnr_reg = addr;
    priv.engine.irq = irq;

    return 0;
}

/* Note: ft3dnr_init_one() will be run before ft3dnr_drv_init() */
static int ft3dnr_init_one(struct platform_device *pdev)
{
	struct resource *mem, *irq;
	unsigned long   vaddr;
    printk("ft3dnr_init_one [%d]\n", pdev->id);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		printk("no mem resource?\n");
		return -ENODEV;
	}

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq) {
		printk("no irq resource?\n");
		return -ENODEV;
	}

    vaddr = (unsigned long)ioremap_nocache(mem->start, PAGE_ALIGN(mem->end - mem->start));
    if (unlikely(!vaddr))
        panic("%s, no virtual address! \n", __func__);

	add_engine(vaddr, irq->start);

    return 0;
}

static int ft3dnr_remove_one(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver ft3dnr_driver = {
    .probe	= ft3dnr_init_one,
    .remove	=  __devexit_p(ft3dnr_remove_one),
	.driver = {
	        .name = "FT3DNR",
	        .owner = THIS_MODULE,
	    },
};

void ft3dnr_init_default_param(void)
{
    /* dma wc/rc wait value */
    priv.engine.wc_wait_value = 0;
    priv.engine.rc_wait_value = 0;

    /* ctrl */
    priv.default_param.ctrl.spatial_en = 0;
    priv.default_param.ctrl.temporal_en = 1;
    priv.default_param.ctrl.tnr_edg_en = 1;
    priv.default_param.ctrl.tnr_learn_en = 0;
    priv.engine.ycbcr.src_yc_swap = src_yc_swap;
    priv.engine.ycbcr.src_cbcr_swap = src_cbcr_swap;
    priv.engine.ycbcr.dst_yc_swap = dst_yc_swap;
    priv.engine.ycbcr.dst_cbcr_swap = dst_cbcr_swap;
    priv.engine.ycbcr.ref_yc_swap = ref_yc_swap;
    priv.engine.ycbcr.ref_cbcr_swap = ref_cbcr_swap;

    /*********** MRNR ************/
    /* Y L0 edg threshold*/
    priv.default_param.mrnr.Y_L_edg_th[0][0] = 208;
    priv.default_param.mrnr.Y_L_edg_th[0][1] = 260;
    priv.default_param.mrnr.Y_L_edg_th[0][2] = 224;
    priv.default_param.mrnr.Y_L_edg_th[0][3] = 192;
    priv.default_param.mrnr.Y_L_edg_th[0][4] = 187;
    priv.default_param.mrnr.Y_L_edg_th[0][5] = 172;
    priv.default_param.mrnr.Y_L_edg_th[0][6] = 172;
    priv.default_param.mrnr.Y_L_edg_th[0][7] = 172;
    /* Y L1 edg threshold*/
    priv.default_param.mrnr.Y_L_edg_th[1][0] = 93;
    priv.default_param.mrnr.Y_L_edg_th[1][1] = 115;
    priv.default_param.mrnr.Y_L_edg_th[1][2] = 99;
    priv.default_param.mrnr.Y_L_edg_th[1][3] = 82;
    priv.default_param.mrnr.Y_L_edg_th[1][4] = 75;
    priv.default_param.mrnr.Y_L_edg_th[1][5] = 74;
    priv.default_param.mrnr.Y_L_edg_th[1][6] = 74;
    priv.default_param.mrnr.Y_L_edg_th[1][7] = 74;
    /* Y L2 edg threshold*/
    priv.default_param.mrnr.Y_L_edg_th[2][0] = 43;
    priv.default_param.mrnr.Y_L_edg_th[2][1] = 52;
    priv.default_param.mrnr.Y_L_edg_th[2][2] = 43;
    priv.default_param.mrnr.Y_L_edg_th[2][3] = 36;
    priv.default_param.mrnr.Y_L_edg_th[2][4] = 32;
    priv.default_param.mrnr.Y_L_edg_th[2][5] = 31;
    priv.default_param.mrnr.Y_L_edg_th[2][6] = 31;
    priv.default_param.mrnr.Y_L_edg_th[2][7] = 31;
    /* Y L3 edg threshold*/
    priv.default_param.mrnr.Y_L_edg_th[3][0] = 21;
    priv.default_param.mrnr.Y_L_edg_th[3][1] = 23;
    priv.default_param.mrnr.Y_L_edg_th[3][2] = 19;
    priv.default_param.mrnr.Y_L_edg_th[3][3] = 16;
    priv.default_param.mrnr.Y_L_edg_th[3][4] = 14;
    priv.default_param.mrnr.Y_L_edg_th[3][5] = 13;
    priv.default_param.mrnr.Y_L_edg_th[3][6] = 13;
    priv.default_param.mrnr.Y_L_edg_th[3][7] = 13;
    /* Cb L edg threshold */
    priv.default_param.mrnr.Cb_L_edg_th[0] = 202;
    priv.default_param.mrnr.Cb_L_edg_th[1] = 186;
    priv.default_param.mrnr.Cb_L_edg_th[2] = 133;
    priv.default_param.mrnr.Cb_L_edg_th[3] = 81;
    /* Cr L edg threshold */
    priv.default_param.mrnr.Cr_L_edg_th[0] = 119;
    priv.default_param.mrnr.Cr_L_edg_th[1] = 110;
    priv.default_param.mrnr.Cr_L_edg_th[2] = 80;
    priv.default_param.mrnr.Cr_L_edg_th[3] = 49;
    /* Y L0 smooth threshold */
    priv.default_param.mrnr.Y_L_smo_th[0][0] = 109;
    priv.default_param.mrnr.Y_L_smo_th[0][1] = 137;
    priv.default_param.mrnr.Y_L_smo_th[0][2] = 118;
    priv.default_param.mrnr.Y_L_smo_th[0][3] = 101;
    priv.default_param.mrnr.Y_L_smo_th[0][4] = 98;
    priv.default_param.mrnr.Y_L_smo_th[0][5] = 91;
    priv.default_param.mrnr.Y_L_smo_th[0][6] = 91;
    priv.default_param.mrnr.Y_L_smo_th[0][7] = 91;
    /* Y L1 smooth threshold */
    priv.default_param.mrnr.Y_L_smo_th[1][0] = 66;
    priv.default_param.mrnr.Y_L_smo_th[1][1] = 82;
    priv.default_param.mrnr.Y_L_smo_th[1][2] = 70;
    priv.default_param.mrnr.Y_L_smo_th[1][3] = 59;
    priv.default_param.mrnr.Y_L_smo_th[1][4] = 53;
    priv.default_param.mrnr.Y_L_smo_th[1][5] = 53;
    priv.default_param.mrnr.Y_L_smo_th[1][6] = 53;
    priv.default_param.mrnr.Y_L_smo_th[1][7] = 53;
    /* Y L2 smooth threshold */
    priv.default_param.mrnr.Y_L_smo_th[2][0] = 36;
    priv.default_param.mrnr.Y_L_smo_th[2][1] = 43;
    priv.default_param.mrnr.Y_L_smo_th[2][2] = 35;
    priv.default_param.mrnr.Y_L_smo_th[2][3] = 30;
    priv.default_param.mrnr.Y_L_smo_th[2][4] = 26;
    priv.default_param.mrnr.Y_L_smo_th[2][5] = 26;
    priv.default_param.mrnr.Y_L_smo_th[2][6] = 26;
    priv.default_param.mrnr.Y_L_smo_th[2][7] = 26;
    /* Y L3 smooth threshold */
    priv.default_param.mrnr.Y_L_smo_th[3][0] = 21;
    priv.default_param.mrnr.Y_L_smo_th[3][1] = 23;
    priv.default_param.mrnr.Y_L_smo_th[3][2] = 19;
    priv.default_param.mrnr.Y_L_smo_th[3][3] = 16;
    priv.default_param.mrnr.Y_L_smo_th[3][4] = 14;
    priv.default_param.mrnr.Y_L_smo_th[3][5] = 13;
    priv.default_param.mrnr.Y_L_smo_th[3][6] = 13;
    priv.default_param.mrnr.Y_L_smo_th[3][7] = 13;
    /* Cb L smooth threshold */
    priv.default_param.mrnr.Cb_L_smo_th[0] = 126;
    priv.default_param.mrnr.Cb_L_smo_th[1] = 116;
    priv.default_param.mrnr.Cb_L_smo_th[2] = 83;
    priv.default_param.mrnr.Cb_L_smo_th[3] = 50;
    /* Cr L smooth threshold */
    priv.default_param.mrnr.Cr_L_smo_th[0] = 75;
    priv.default_param.mrnr.Cr_L_smo_th[1] = 69;
    priv.default_param.mrnr.Cr_L_smo_th[2] = 50;
    priv.default_param.mrnr.Cr_L_smo_th[3] = 30;
    /* Y L nr strength */
    priv.default_param.mrnr.Y_L_nr_str[0] = 11;
    priv.default_param.mrnr.Y_L_nr_str[1] = 11;
    priv.default_param.mrnr.Y_L_nr_str[2] = 11;
    priv.default_param.mrnr.Y_L_nr_str[3] = 11;
    /* C L nr strength */
    priv.default_param.mrnr.C_L_nr_str[0] = 13;
    priv.default_param.mrnr.C_L_nr_str[1] = 13;
    priv.default_param.mrnr.C_L_nr_str[2] = 13;
    priv.default_param.mrnr.C_L_nr_str[3] = 13;

    /*********** TMNR ************/
    priv.default_param.tmnr.Y_var = 8;
    priv.default_param.tmnr.Cb_var = 6;
    priv.default_param.tmnr.Cr_var = 6;
    priv.default_param.tmnr.dc_offset = 6;
    priv.default_param.tmnr.auto_recover = 0;
    priv.default_param.tmnr.rapid_recover = 0;
    priv.default_param.tmnr.learn_rate = 32;
    priv.default_param.tmnr.his_factor = 8;
    priv.default_param.tmnr.edge_th = 40;

    priv.default_param.rvlut.rlut[0] = 0x04000000;
    priv.default_param.rvlut.rlut[1] = 0x08070605;
    priv.default_param.rvlut.rlut[2] = 0x0c0b0a09;
    priv.default_param.rvlut.rlut[3] = 0x100f0e0d;
    priv.default_param.rvlut.rlut[4] = 0x14131211;
    priv.default_param.rvlut.rlut[5] = 0x00000015;

    priv.default_param.rvlut.vlut[0] = 0x06060606;
    priv.default_param.rvlut.vlut[1] = 0x0e0c0a08;
    priv.default_param.rvlut.vlut[2] = 0x15141210;
    priv.default_param.rvlut.vlut[3] = 0x1c1b1917;
    priv.default_param.rvlut.vlut[4] = 0x2322201f;
    priv.default_param.rvlut.vlut[5] = 0x00000025;

    memcpy(&priv.isp_param, &priv.default_param, sizeof(ft3dnr_global_t));
    priv.isp_param.ctrl.spatial_en = 1;
    priv.isp_param.ctrl.tnr_learn_en = 1;

    ft3dnr_init_global();

}

void ft3dnr_joblist_add(ft3dnr_job_t *job)
{
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);
    list_add_tail(&job->job_list, &priv.engine.qlist);
    spin_unlock_irqrestore(&priv.lock, flags);
}

void ft3dnr_set_mrnr_free(ft3dnr_job_t *job)
{
    int blk = job->ll_blk.mrnr_num;
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);

    if ((priv.engine.mrnr_table >> blk & 0x1) == 0)
        printk("[3DNR] error!! this mrnr %d should be used\n", blk);

    priv.engine.mrnr_table ^= (0x1 << blk);
    priv.engine.mrnr_cnt++;

    spin_unlock_irqrestore(&priv.lock, flags);
}

/*
 *  set ft3dnr link list memory block as used
 */
void ft3dnr_set_mrnr_used(ft3dnr_job_t *job, int blk)
{
    priv.engine.mrnr_table |= (0x1 << blk);
    priv.engine.mrnr_cnt--;
    job->ll_blk.mrnr_num = blk;
}

/*
 * get ft3dnr link list memory block number, blk = 0 --> free, blk = 1 --> used
 */
int ft3dnr_get_free_mrnr(ft3dnr_job_t *job)
{
    int blk = -1;
    int i;
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);

    for (i = 0; i < 2; i++) {
        if ((priv.engine.mrnr_table >> i & 0x1) == 0) {
            blk = i;
            break;
        }
    }

    if (blk == -1) {
        printk("[3DNR] error, can't get free mrnr\n");
        spin_unlock_irqrestore(&priv.lock, flags);
        return -1;
    }

    ft3dnr_set_mrnr_used(job, blk);

    spin_unlock_irqrestore(&priv.lock, flags);

    return 0;
}

void ft3dnr_set_blk_free(ft3dnr_job_t *job)
{
    int blk = job->ll_blk.blk_num;
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);

    if ((priv.engine.sram_table >> blk & 0x1) == 0)
        printk("[3DNR] error!! this blk %d should be used\n", blk);

    priv.engine.sram_table ^= (0x1 << blk);
    priv.engine.blk_cnt++;

    spin_unlock_irqrestore(&priv.lock, flags);
}

/*
 *  set ft3dnr link list memory block as used
 */
void ft3dnr_set_blk_used(ft3dnr_job_t *job, int blk)
{
    priv.engine.sram_table |= (0x1 << blk);
    priv.engine.blk_cnt--;
    job->ll_blk.blk_num = blk;
}

/*
 * get ft3dnr link list memory block number, blk = 0 --> free, blk = 1 --> used
 */
int ft3dnr_get_next_free_blk(ft3dnr_job_t *job)
{
    int blk, i;
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);

    blk = priv.engine.null_blk;
    if ((priv.engine.sram_table >> blk & 0x1) == 1) {
        printk("[3DNR] error!! this blk %d should be free\n", blk);
        spin_unlock_irqrestore(&priv.lock, flags);
        return -1;
    }

    job->ll_blk.status_addr = blk * 0x80;
    ft3dnr_set_blk_used(job, blk);

    blk = -1;

    for (i = 0; i < 4; i++) {
        if ((priv.engine.sram_table >> i & 0x1) == 0) {
            blk = i;
            break;
        }
    }

    if (blk == -1) {
        printk("[3DNR] error, can't get free blk\n");
        spin_unlock_irqrestore(&priv.lock, flags);
        return -1;
    }

    job->ll_blk.next_blk = blk;
    priv.engine.null_blk = blk;

    spin_unlock_irqrestore(&priv.lock, flags);

    return 0;
}

int ft3dnr_write_table(ft3dnr_job_t *job, int chain_cnt, int last_job)
{
    int ret = 0;
    unsigned long flags;

    ret = ft3dnr_get_next_free_blk(job);
    if (ret < 0)
        return -1;
    ret = ft3dnr_get_free_mrnr(job);
    if (ret < 0)
        return -1;

    ft3dnr_set_lli_blk(job, chain_cnt, last_job);

    spin_lock_irqsave(&priv.lock, flags);
    list_move_tail(&job->job_list, &priv.engine.slist);
    spin_unlock_irqrestore(&priv.lock, flags);
    job->status = JOB_STS_SRAM;

    /* update node from V.G status */
    if (job->fops && job->fops->update_status)
        job->fops->update_status(job, JOB_STS_SRAM);

    return 0;
}

int get_job_cnt(void)
{
    int blk_cnt = 0, mrnr_cnt = 0, job_cnt = 0;

    blk_cnt = priv.engine.blk_cnt;
    mrnr_cnt = priv.engine.mrnr_cnt;

    if (mrnr_cnt == 0)
        job_cnt = 0;
    if (mrnr_cnt >= 1 && blk_cnt >= 2)
        job_cnt = 1;
    if (mrnr_cnt >= 2 && blk_cnt >= 3)
        job_cnt = 2;

    return job_cnt;
}

/*
 * workqueue to add jobs to link list memory
 */
void ft3dnr_add_table(void)
{
    ft3dnr_job_t *job, *ne;
    int job_cnt = 0, last_job = 0, chain_cnt = 0;
    int chain_job = 0;
    int ret = 0;
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);

    job_cnt = get_job_cnt();

    if (job_cnt > 0) {
        list_for_each_entry_safe(job, ne, &priv.engine.qlist, job_list) {
            if (job->status != JOB_STS_QUEUE)
                continue;

            chain_job++;

            if (chain_job < job_cnt)
                continue;
            else
                break;
        }
        chain_cnt = chain_job;

        if (chain_job > 0) {
            list_for_each_entry_safe(job, ne, &priv.engine.qlist, job_list) {
                if (job->status != JOB_STS_QUEUE)
                    continue;

                chain_job--;

                if (chain_job == 0)
                    last_job = 1;
                else
                    last_job = 0;

                ft3dnr_write_table(job, chain_cnt, last_job);

                if (chain_job == 0)
                    break;
            }
        }

        if (!ENGINE_BUSY()) {
            if (!list_empty(&priv.engine.slist)) {
                if (priv.engine.ll_mode == 1) {
                    mark_engine_start();
                    ft3dnr_write_status();
                }
                /* first time to trigger 3DNR, set ll_fire */
                if (priv.engine.ll_mode == 0) {
                    ft3dnr_write_status();
                    mark_engine_start();
                    ret = ft3dnr_fire();
                    if (ret < 0)
                        printk("fail to fire\n");
                    else
                        priv.engine.ll_mode = 1;
                }
                // move jobs from sram list into working list
                list_for_each_entry_safe(job, ne, &priv.engine.slist, job_list) {
                    list_move_tail(&job->job_list, &priv.engine.wlist);
                    job->status = JOB_STS_ONGOING;
                    /* update node status */
                    if (job->fops && job->fops->update_status)
                        job->fops->update_status(job, JOB_STS_ONGOING);
                }
                LOCK_ENGINE();
            }
        }
    }

    spin_unlock_irqrestore(&priv.lock, flags);
}

#ifdef USE_KTHREAD
int add_table = 0;
static int add_thread(void *__cwq)
{
    int status;
    add_table = 1;
    set_current_state(TASK_INTERRUPTIBLE);

    do {
        status = wait_event_timeout(add_thread_wait, add_wakeup_event, msecs_to_jiffies(1000));
        if (status == 0)
            continue;   /* timeout */

        add_wakeup_event = 0;
        /* add table */
        ft3dnr_add_table();
    } while(!kthread_should_stop());

    __set_current_state(TASK_RUNNING);
    add_table = 0;
    return 0;
}

static void add_wake_up(void)
{
    add_wakeup_event = 1;
    wake_up(&add_thread_wait);
}
#endif

void ft3dnr_stop_channel(int chn)
{
    ft3dnr_job_t    *job, *ne, *parent, *job1, *ne1;
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);

    list_for_each_entry_safe(job, ne, &priv.engine.qlist, job_list) {
        if (job->chan_id == chn) {
            parent = job->parent;
            if (parent->status != JOB_STS_QUEUE)
                continue;

            list_for_each_entry_safe(job1, ne1, &priv.engine.qlist, job_list) {
                if (job1->parent == parent)
                    job1->status = JOB_STS_FLUSH;
            }
        }
    }

    list_for_each_entry_safe(job, ne, &priv.engine.qlist, job_list) {
        if (job->status == JOB_STS_FLUSH) {
            if (job->fops && job->fops->update_status)
                job->fops->update_status(job, JOB_STS_FLUSH);
            list_del(&job->job_list);
            kmem_cache_free(priv.job_cache, job);
            MEMCNT_SUB(&priv, 1);
        }
    }

    spin_unlock_irqrestore(&priv.lock, flags);
}



/*
 * free job from ISR, free blocks, and callback to V.G
 */
void ft3dnr_job_free(ft3dnr_job_t *job)
{
    unsigned long flags;

    ft3dnr_set_blk_free(job);
    ft3dnr_set_mrnr_free(job);

    if (job->need_callback) {
        if (job->fops && job->fops->callback)
            job->fops->callback(job);
    }

    spin_lock_irqsave(&priv.lock, flags);
    list_del(&job->job_list);
    spin_unlock_irqrestore(&priv.lock, flags);
    kmem_cache_free(priv.job_cache, job);
    MEMCNT_SUB(&priv, 1);
#ifdef USE_KTHREAD
	add_wake_up();
#endif
#ifdef USE_WQ
    PREPARE_DELAYED_WORK(&process_job_work, (void *)ft3dnr_add_table);
    queue_delayed_work(job_workq, &process_job_work, callback_period);
#endif
}

void ft3dnr_put_job(void)
{
    ft3dnr_add_table();
/*#ifdef USE_KTHREAD
	add_wake_up();
#endif
#ifdef USE_WQ
    PREPARE_DELAYED_WORK(&process_job_work, (void *)ft3dnr_add_table);
    queue_delayed_work(job_workq, &process_job_work, callback_period);
#endif*/
}
EXPORT_SYMBOL(ft3dnr_put_job);

static void ft3dnr_lli_timeout(unsigned long data)
{
    int base = 0;
    //unsigned long flags;

    base = priv.engine.ft3dnr_reg;

    printk("FT3DNR timeout !!\n");

    ft3dnr_dump_reg();

    ft3dnr_sw_reset();

    UNLOCK_ENGINE();
    mark_engine_finish();

    damnit(MODULE_NAME);
}

int ft3dnr_get_var_size(int width, int height)
{
    int var_size = 0;
    unsigned int var_w = 0;
    unsigned int var_h = 0;
    unsigned int var_h_sz = 0;

    var_w = (width + VARBLK_WIDTH - 1) / VARBLK_WIDTH;
    var_h = (height +  VARBLK_HEIGHT - 1) / VARBLK_HEIGHT;
    var_h_sz = (var_h +VAR_BIT_COUNT - 1) / VAR_BIT_COUNT;
    var_size = var_h_sz * var_w * VAR_BIT_COUNT;
    //printk("FT3DNR var size %d\n", var_size);
    return var_size;
}

static int getline(char *line, int size, struct file *fd, unsigned long long *offset)
{
    char ch;
    int lineSize = 0, ret;

    memset(line, 0, size);
    while ((ret = (int)vfs_read(fd, &ch, 1, offset)) == 1) {
        if (ch == 0xD)
            continue;
        if (lineSize >= MAX_LINE_CHAR) {
            printk("Line buf is not enough %d! (%s)\n", MAX_LINE_CHAR, line);
            break;
        }
        line[lineSize++] = ch;
        if (ch == 0xA)
            break;
    }
    return lineSize;
}

static int readline(struct file *fd, int size, char *line, unsigned long long *offset)
{
    int ret = 0;
    do {
        ret = getline(line, size, fd, offset);
    } while (((line[0] == 0xa)) && (ret != 0));
    return ret;
}

static int get_res_idx(char *res_name)
{
    int i;
    for (i = 0; i < MAX_RES_IDX; i++) {
        if (strcmp(res_base_info[i].name, res_name) == 0)
            return i;
    }
    return -1;
}

static int get_config_res_name(char *name, char *line, int offset, int size)
{
    int i = 0;
    while (',' == line[offset] || ' ' == line[offset]) {
    	if (offset >= size)
    		return -1;
    	offset++;
    }
    while ('/' != line[offset]) {
    	name[i] = line[offset];
    	i++;
    	offset++;
    }
    name[i] = '\0';
    return offset;
}

static int ft3dnr_parse_spec_cfg(void)
{
    char line[MAX_LINE_CHAR];
    char filename[80];
    mm_segment_t fs;
    struct file *fd = NULL;
    unsigned long long offset = 0;
    int ret = 0;
    int i = 0, j = 0;
    int channel;
    int idx, l_res_cfg_idx = 0;
    res_cfg_t   *l_res_cfg = NULL, *f_res_cfg = NULL;
    int         l_res_cfg_cnt = 0, f_res_cfg_cnt = 0, res_cfg_array_sz;
    struct {
        char res_type[7];
        int  res_chs;
        int  res_fps;
    } res_item[MAX_RES_CNT];

    fs = get_fs();
    set_fs(get_ds());
    sprintf(filename, "%s/%s", config_path, SPEC_FILENAME);
    fd = filp_open(filename, O_RDONLY, S_IRWXU);
    if (IS_ERR(fd)) {
        printk("%s(%s): file %s not found\n", DRIVER_NAME, __func__, filename);
        return -ENOENT;
    }

    res_cfg_array_sz = sizeof(res_cfg_t) * MAX_CH_NUM;

    l_res_cfg = (res_cfg_t *) kzalloc(res_cfg_array_sz, GFP_KERNEL);
    if (ZERO_OR_NULL_PTR(l_res_cfg))
        panic("%s: allocate memory fail for l_res_cfg(0x%p)!\n", __func__, l_res_cfg);

    f_res_cfg = (res_cfg_t *) kzalloc(res_cfg_array_sz, GFP_KERNEL);
    if (ZERO_OR_NULL_PTR(f_res_cfg))
        panic("%s: allocate memory fail for f_res_cfg(0x%p)!\n", __func__, f_res_cfg);

    f_res_cfg_cnt = 0;

    while (readline(fd, sizeof(line), line, &offset)) {
        if (strstr(line, "[ENC 3DI]")) {
            readline(fd, sizeof(line), line, &offset);
            sscanf(line, "      %s%s%s%s%s%s%s%s%s%s",
                    res_item[0].res_type, res_item[1].res_type,
                    res_item[2].res_type, res_item[3].res_type,
                    res_item[4].res_type, res_item[5].res_type,
                    res_item[6].res_type, res_item[7].res_type,
                    res_item[8].res_type, res_item[9].res_type);

            while (readline(fd, sizeof(line), line, &offset)) {
                if (strstr(line, "CH_")) {
                    sscanf(line, "CH_%02d %02d/%03d %02d/%03d %02d/%03d %02d/%03d %02d/%03d %02d/%03d %02d/%03d"
                           " %02d/%03d %02d/%03d %02d/%03d", &channel,
                            &res_item[0].res_chs, &res_item[0].res_fps,
                            &res_item[1].res_chs, &res_item[1].res_fps,
                            &res_item[2].res_chs, &res_item[2].res_fps,
                            &res_item[3].res_chs, &res_item[3].res_fps,
                            &res_item[4].res_chs, &res_item[4].res_fps,
                            &res_item[5].res_chs, &res_item[5].res_fps,
                            &res_item[6].res_chs, &res_item[6].res_fps,
                            &res_item[7].res_chs, &res_item[7].res_fps,
                            &res_item[8].res_chs, &res_item[8].res_fps,
                            &res_item[9].res_chs, &res_item[9].res_fps);

                    l_res_cfg_idx = 0;

                    for (i = 0; i < MAX_RES_CNT; i++) {
                        if ((idx = get_res_idx(res_item[i].res_type)) < 0)
                            panic("%s(%s): res type \"%s\" not support\n", DRIVER_NAME, __func__, res_item[i].res_type);
                        for (j = 0; j < res_item[i].res_chs; j++) {
                            if (l_res_cfg_idx < MAX_CH_NUM) {
                                memcpy(l_res_cfg[l_res_cfg_idx].res_type, res_item[i].res_type, sizeof(res_item[i].res_type));
                                l_res_cfg[l_res_cfg_idx].width = res_base_info[idx].width;
                                l_res_cfg[l_res_cfg_idx].height = res_base_info[idx].height;
                                l_res_cfg[l_res_cfg_idx].size = (u32) res_base_info[idx].width * (u32) res_base_info[idx].height;
                                l_res_cfg[l_res_cfg_idx].map_chan = MAP_CHAN_NONE;
                                l_res_cfg_idx++;
                            } else {
                                panic("%s(%s): required channel number exceeds max %d\n", DRIVER_NAME, __func__, MAX_CH_NUM);
                                ret = -EINVAL;
                                break;
                            }
                        }
                        if (ret)
                            break;
                    }
                    if (ret)
                        break;

                    l_res_cfg_cnt = l_res_cfg_idx;

                    // sort size of temp res_cfg from big to little
                    for (i = 0; i < (l_res_cfg_cnt - 1); i++) {
                        res_cfg_t   tmp_res, *res_big;

                        res_big = &l_res_cfg[i];
                        for (j = i + 1; j < l_res_cfg_cnt; j++) {
                            if (l_res_cfg[j].size > res_big->size)
                                res_big = &l_res_cfg[j];
                        }
                        memcpy(&tmp_res, &l_res_cfg[i], sizeof(res_cfg_t));
                        memcpy(&l_res_cfg[i], res_big, sizeof(res_cfg_t));
                        memcpy(res_big, &tmp_res, sizeof(res_cfg_t));
                    }

                    // update temp res_cfg to final result
                    for (i = 0; i < l_res_cfg_cnt; i++) {
                        if (l_res_cfg[i].size > f_res_cfg[i].size)
                            memcpy(&f_res_cfg[i], &l_res_cfg[i], sizeof(res_cfg_t));
                    }

                    if (l_res_cfg_cnt > f_res_cfg_cnt)
                        f_res_cfg_cnt = l_res_cfg_cnt;
                } else {
                    break;
                } // if (strstr(line, "CH_"))
            } // while ()
            if (ret)
                break;
        } // if (strstr(line, "[ENC 3DI]"))
    } // while ()
    filp_close(fd, NULL);
    set_fs(fs);

    priv.res_cfg_cnt = f_res_cfg_cnt;

    if (priv.res_cfg_cnt == 0)
        panic("%s: falied to parse vg config \"%s\", priv.res_cfg_cnt = 0\n", __func__, SPEC_FILENAME);
    priv.res_cfg = (res_cfg_t *) kzalloc(sizeof(res_cfg_t) * priv.res_cfg_cnt, GFP_KERNEL);
    if (ZERO_OR_NULL_PTR(priv.res_cfg))
        panic("%s: allocate memory fail for priv.res_cfg(0x%p)\n", __func__, priv.res_cfg);

    // update final res_cfg to priv
    memcpy(priv.res_cfg, f_res_cfg, sizeof(res_cfg_t) * priv.res_cfg_cnt);

    kfree(l_res_cfg);
    kfree(f_res_cfg);

    return 0;
}

static int parse_res_cfg_line(char *line, int offset, int line_size, res_cfg_t *res_cfg_p, int *res_cfg_cnt)
{
    char res_name[7];
    int i, ii, ret = 0, idx;
    int res_chs, res_fps, ddr_idx;
    int res_cfg_idx = 0;

    i = offset;

    while (1) {
        i = get_config_res_name(res_name, line, i, line_size);
        if (i >= line_size)
            break;
        sscanf(&line[i], "/%d/%d/%d", &res_chs, &res_fps, &ddr_idx);
        idx = get_res_idx(res_name);

        if (idx >= 0) {
            for (ii = 0; ii < res_chs; ii++) {
                if (res_cfg_idx < MAX_CH_NUM) {
                    memcpy(res_cfg_p[res_cfg_idx].res_type, &res_name, sizeof(res_name));
                    res_cfg_p[res_cfg_idx].width = res_base_info[idx].width;
                    res_cfg_p[res_cfg_idx].height = res_base_info[idx].height;
                    res_cfg_p[res_cfg_idx].size = (u32) res_base_info[idx].width * (u32) res_base_info[idx].height;
                    res_cfg_p[res_cfg_idx].map_chan = MAP_CHAN_NONE;
                    res_cfg_idx++;
                } else {
                    panic("%s(%s): required channel number exceeds max %d\n", DRIVER_NAME, __func__, MAX_CH_NUM);
                    ret = -EINVAL;
                    break;
                }
            }
        } else {
            // if res_name not matched
            panic("%s(%s): res name \"%s\" not support\n", DRIVER_NAME, __func__, res_name);
        }
        while (',' != line[i] && i < line_size)  i++;
        if (i == line_size)
            break;
        if (ret)
            break;
    }

    if (ret == 0)
        *res_cfg_cnt = res_cfg_idx;

    return ret;
}

static void parse_additional_resolution(char *line, int offset, int line_size)
{
    char res_name[7];
    int res_width, res_height;
    int i, j;

    i = offset;

    while (1) {
        i = get_config_res_name(res_name, line, i, line_size);
        if (i >= line_size)
            break;
        sscanf(&line[i], "/%d/%d", &res_width, &res_height);
        res_width = ALIGN16_UP(res_width);
        res_height = ALIGN16_UP(res_height);

        for (j = 0; j < MAX_RES_IDX; j++) {
            if (strcmp(res_base_info[j].name, res_name) == 0) {
                if ((res_width * res_height) > (res_base_info[j].width * res_base_info[j].height)) {
                    res_base_info[j].width = res_width;
                    res_base_info[j].height = res_height;
                }
                break;
            } else if (strcmp(res_base_info[j].name, "NONE") == 0) {
                strcpy(res_base_info[j].name, res_name);
                res_base_info[j].width = res_width;
                res_base_info[j].height = res_height;
                break;
            }
        }
        if (j >= MAX_RES_IDX)
            panic("%s(%s): too many additional resolution!\n", DRIVER_NAME, __func__);

        while (',' != line[i] && i < line_size)  i++;
        if (i == line_size)
            break;
    }
}

static int ft3dnr_parse_gmlib_cfg(void)
{
    char line[MAX_LINE_CHAR];
    char filename[80];
    mm_segment_t fs;
    struct file *fd = NULL;
    unsigned long long offset = 0;
    int i, line_size;
    res_cfg_t   *l_res_cfg = NULL, *f_res_cfg = NULL;
    int         l_res_cfg_cnt = 0, f_res_cfg_cnt = 0, res_cfg_array_sz;
    int         ii, jj, ret = 0;

    fs = get_fs();
    set_fs(get_ds());
    sprintf(filename, "%s/%s", config_path, GMLIB_FILENAME);
    fd = filp_open(filename, O_RDONLY, S_IRWXU);
    if (IS_ERR(fd)) {
        printk("%s(%s): file %s not found\n", DRIVER_NAME, __func__, filename);
        return -ENOENT;
    }

    // parse [RESOLUTION] first
    while (readline(fd, sizeof(line), line, &offset)) {
        if (strstr(line, "[RESOLUTION]")) {
            while (readline(fd, sizeof(line), line, &offset)) {
                if ('\n' == line[0] || '\0' == line[0] || '[' == line[0]) {
                    break;
                }
                line_size = strlen(line);
                if (';' == line[0])
                    continue;
                i = 0;
                while ('=' != line[i])  i++;
                i++;
                while (' ' == line[i])  i++;

                parse_additional_resolution(line, i, line_size);
            }
        }
    }

    vfs_llseek(fd, 0, 0);
    offset = 0;

    res_cfg_array_sz = sizeof(res_cfg_t) * MAX_CH_NUM;

    l_res_cfg = (res_cfg_t *) kzalloc(res_cfg_array_sz, GFP_KERNEL);
    if (ZERO_OR_NULL_PTR(l_res_cfg))
        panic("%s: allocate memory fail for l_res_cfg(0x%p)!\n", __func__, l_res_cfg);

    f_res_cfg = (res_cfg_t *) kzalloc(res_cfg_array_sz, GFP_KERNEL);
    if (ZERO_OR_NULL_PTR(f_res_cfg))
        panic("%s: allocate memory fail for f_res_cfg(0x%p)!\n", __func__, f_res_cfg);

    f_res_cfg_cnt = 0;

    while (readline(fd, sizeof(line), line, &offset)) {
        if (strstr(line, "[ENCODE_DIDN]")) {
            while (readline(fd, sizeof(line), line, &offset)) {
                if ('\n' == line[0] || '\0' == line[0] || '[' == line[0]) {
                    break;
                }
                line_size = strlen(line);
                if (';' == line[0])
                    continue;
                i = 0;
                while ('=' != line[i])  i++;
                i++;
                while (' ' == line[i])  i++;

                memset(l_res_cfg, 0, res_cfg_array_sz);
                l_res_cfg_cnt = 0;

                ret = parse_res_cfg_line(line, i, line_size, l_res_cfg, &l_res_cfg_cnt);
                if (ret)
                    break;

                // sort size of temp res_cfg from big to little
                for (ii = 0; ii < (l_res_cfg_cnt - 1); ii++) {
                    res_cfg_t   tmp_res, *res_big;

                    res_big = &l_res_cfg[ii];
                    for (jj = ii + 1; jj < l_res_cfg_cnt; jj++) {
                        if (l_res_cfg[jj].size > res_big->size)
                            res_big = &l_res_cfg[jj];
                    }
                    memcpy(&tmp_res, &l_res_cfg[ii], sizeof(res_cfg_t));
                    memcpy(&l_res_cfg[ii], res_big, sizeof(res_cfg_t));
                    memcpy(res_big, &tmp_res, sizeof(res_cfg_t));
                }

                // update temp res_cfg to final result
                for (ii = 0; ii < l_res_cfg_cnt; ii++) {
                    if (l_res_cfg[ii].size > f_res_cfg[ii].size)
                        memcpy(&f_res_cfg[ii], &l_res_cfg[ii], sizeof(res_cfg_t));
                }

                if (l_res_cfg_cnt > f_res_cfg_cnt)
                    f_res_cfg_cnt = l_res_cfg_cnt;
            }
        }
    }
    filp_close(fd, NULL);
    set_fs(fs);

    priv.res_cfg_cnt = f_res_cfg_cnt;

    if (priv.res_cfg_cnt == 0)
        panic("%s: falied to parse vg config \"%s\", priv.res_cfg_cnt = 0\n", __func__, GMLIB_FILENAME);
    priv.res_cfg = (res_cfg_t *) kzalloc(sizeof(res_cfg_t) * priv.res_cfg_cnt, GFP_KERNEL);
    if (ZERO_OR_NULL_PTR(priv.res_cfg))
        panic("%s: allocate memory fail for priv.res_cfg(0x%p)\n", __func__, priv.res_cfg);

    // update final res_cfg to priv
    memcpy(priv.res_cfg, f_res_cfg, sizeof(res_cfg_t) * priv.res_cfg_cnt);

    kfree(l_res_cfg);
    kfree(f_res_cfg);

    return ret;
}

static int ft3dnr_parse_module_param_res_cfg(char *line)
{
    int i, line_size;
    res_cfg_t   *l_res_cfg;
    int         l_res_cfg_cnt = 0, res_cfg_array_sz;
    int         ii, jj, ret = 0;

    line_size = strlen(line);

    i = 0;
    while (' ' == line[i])  i++;

    res_cfg_array_sz = sizeof(res_cfg_t) * MAX_CH_NUM;

    l_res_cfg = (res_cfg_t *) kzalloc(res_cfg_array_sz, GFP_KERNEL);
    if (ZERO_OR_NULL_PTR(l_res_cfg))
        panic("%s: allocate memory fail for l_res_cfg(0x%p)!\n", __func__, l_res_cfg);

    l_res_cfg_cnt = 0;

    ret = parse_res_cfg_line(line, i, line_size, l_res_cfg, &l_res_cfg_cnt);
    if (ret)
        goto exit;

    // sort size of temp res_cfg from big to little
    for (ii = 0; ii < (l_res_cfg_cnt - 1); ii++) {
        res_cfg_t   tmp_res, *res_big;

        res_big = &l_res_cfg[ii];
        for (jj = ii + 1; jj < l_res_cfg_cnt; jj++) {
            if (l_res_cfg[jj].size > res_big->size)
                res_big = &l_res_cfg[jj];
        }
        memcpy(&tmp_res, &l_res_cfg[ii], sizeof(res_cfg_t));
        memcpy(&l_res_cfg[ii], res_big, sizeof(res_cfg_t));
        memcpy(res_big, &tmp_res, sizeof(res_cfg_t));
    }

    priv.res_cfg_cnt = l_res_cfg_cnt;

    priv.res_cfg = (res_cfg_t *) kzalloc(sizeof(res_cfg_t) * priv.res_cfg_cnt, GFP_KERNEL);
    if (ZERO_OR_NULL_PTR(priv.res_cfg))
        panic("%s: allocate memory fail for priv.res_cfg(0x%p)\n", __func__, priv.res_cfg);

    // update final res_cfg to priv
    memcpy(priv.res_cfg, l_res_cfg, sizeof(res_cfg_t) * priv.res_cfg_cnt);

exit:
    kfree(l_res_cfg);

    return ret;
}

int ft3dnr_var_buf_alloc(void)
{
    int ret = 0;
    int res_idx=0;
    int var_sz;
    char buf_name[20];
    struct frammap_buf_info buf_info;
    ft3dnr_dma_buf_t *buf = NULL;

    for (res_idx = 0; res_idx < priv.res_cfg_cnt ; res_idx++) {
        buf = &(priv.res_cfg[res_idx].var_buf);

        var_sz = ft3dnr_get_var_size(priv.res_cfg[res_idx].width, priv.res_cfg[res_idx].height);

        memset(&buf_info, 0, sizeof(struct frammap_buf_info));

        buf_info.size = var_sz;
        buf_info.alloc_type = ALLOC_NONCACHABLE;
        sprintf(buf_name, "ft3dnr_var.%d", res_idx);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
        buf_info.name = buf_name;
#endif

        ret = frm_get_buf_ddr(DDR_ID_SYSTEM, &buf_info);
        if (ret < 0) {
            printk("FT3DNR var.%d buffer allocate failed!\n", res_idx);
            goto exit;
        }

        buf->vaddr = (void *)buf_info.va_addr;
        buf->paddr = buf_info.phy_addr;
        buf->size  = buf_info.size;
        strcpy(buf->name, buf_name);

        printk("FT3DNR var.%d buf size %#x width %d height %d phy addr %#x\n", res_idx, buf->size,
                priv.res_cfg[res_idx].width, priv.res_cfg[res_idx].height, buf->paddr);

        /* init buffer */
        memset(buf->vaddr, 0x8, buf->size);
    }

exit:
    return ret;
}

void ft3dnr_var_buf_free(void)
{
    int res_idx = 0;
    ft3dnr_dma_buf_t *buf = NULL;

    for (res_idx = 0; res_idx < priv.res_cfg_cnt; res_idx++) {
        buf = &(priv.res_cfg[res_idx].var_buf);
        if (buf->vaddr) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
            frm_free_buf_ddr(buf->vaddr);
#else
            struct frammap_buf_info info;

            memset(&info,0x0,sizeof(struct frammap_buf_info));
            info.va_addr  = (u32)(buf->vaddr);
            info.phy_addr = buf->paddr;
            info.size     = buf->size;
            frm_free_buf_ddr(&info);
#endif
        }
    }
}

static int ft3dnr_proc_init(void)
{
    /* create proc */
    entity_proc = create_proc_entry(FT3DNR_PROC_NAME, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (entity_proc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        return -EFAULT;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    entity_proc->owner = THIS_MODULE;
#endif
    /* ctrl */
    ft3dnr_ctrl_proc_init(entity_proc);
    /* mrnr */
    ft3dnr_mrnr_proc_init(entity_proc);
    /* tmnr */
    ft3dnr_tmnr_proc_init(entity_proc);
    /* MEM */
    ft3dnr_mem_proc_init(entity_proc);
    /* DMA */
    ft3dnr_dma_proc_init(entity_proc);

    return 0;
}

void ft3dnr_proc_remove(void)
{
    /* ctrl */
    ft3dnr_ctrl_proc_remove(entity_proc);
    /* mrnr */
    ft3dnr_mrnr_proc_remove(entity_proc);
    /* tmnr */
    ft3dnr_tmnr_proc_remove(entity_proc);
    /* MEM */
    ft3dnr_mem_proc_remove(entity_proc);
    /* DMA */
    ft3dnr_dma_proc_remove(entity_proc);

    if (entity_proc)
        remove_proc_entry(entity_proc->name, entity_proc->parent);
}

char *name = "ft3dnr";
static int __init ft3dnr_drv_init(void)
{
    int dev = 0;
    int ret = 0;
    const char *devname[1] = {"ft3dnr-0"};

    printk("FT3DNR version %d\n", VERSION);

    /* global info */
    memset(&priv, 0x0, sizeof(ft3dnr_priv_t));

    INIT_LIST_HEAD(&priv.engine.qlist);
    INIT_LIST_HEAD(&priv.engine.slist);
    INIT_LIST_HEAD(&priv.engine.wlist);

    priv.engine.blk_cnt = 4;
    priv.engine.mrnr_cnt = 2;
    priv.engine.sram_table = 0xF0;
#ifdef USE_WQ
    /* create workqueue */
    job_workq = create_workqueue("ft3dnr");
    if (job_workq == NULL) {
        printk("FT3DNR: error in create workqueue! \n");
        return -1;
    }
#endif

#ifdef USE_KTHREAD
    init_waitqueue_head(&add_thread_wait);

	priv.add_task = kthread_create(add_thread, NULL, "3dnr_add_table");
    if (IS_ERR(priv.add_task))
        panic("%s, create ep_task fail! \n", __func__);
    wake_up_process(priv.add_task);
#endif

    /* init the clock and add the device .... */
    platform_init();

    /* Register the driver */
	if (platform_driver_register(&ft3dnr_driver)) {
		printk("Failed to register FT3DNR driver\n");
		return -ENODEV;
	}

    /* pwm */
    if (pwm != 0) {
        ret = platform_set_pwm(pwm);
        if (ret < 0)
            printk("failed to set pwm\n");
    }

	/* hook irq */
    if (request_irq(priv.engine.irq, &ft3dnr_interrupt, 0, devname[dev], (void *)dev) < 0)
        printk("Unable to allocate IRQ %d\n", priv.engine.irq);

    spin_lock_init(&priv.lock);

	/* init semaphore lock */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
    sema_init(&priv.sema_lock, 1);
#else
    init_MUTEX(&priv.sema_lock);
#endif

	/* create memory cache */
	priv.job_cache = kmem_cache_create(name, sizeof(ft3dnr_job_t), 0, 0, NULL);
    if (priv.job_cache == NULL)
        panic("%s, no memory for job_cache! \n", __func__);

    if (max_minors <= 0)
        max_minors = MAX_CH_NUM;

    if ((res_cfg != NULL) && (strlen(res_cfg) != 0)) {
        ret = ft3dnr_parse_module_param_res_cfg(res_cfg);
        if (ret)
            panic("%s(%s): incorrect value of module parameter res_cfg\n", DRIVER_NAME, __func__);
    } else {
        /* parse gmlib.cfg */
        ret = ft3dnr_parse_gmlib_cfg();
        if (ret) {
            if (ret == -ENOENT) {
                printk("%s(%s): try to parse another vg config \"%s\"\n", DRIVER_NAME, __func__, SPEC_FILENAME);
                /* parse spec.cfg */
                ret = ft3dnr_parse_spec_cfg();
                if (ret) {
                    panic("%s(%s): falied to parse vg config \"%s\"\n", DRIVER_NAME, __func__, SPEC_FILENAME);
                    return ret;
                }
            } else {
                panic("%s(%s): falied to parse vg config \"%s\"\n", DRIVER_NAME, __func__, GMLIB_FILENAME);
                return ret;
            }
        }
    }

    /* init current max dim as smallest one of res_cfg */
    priv.curr_max_dim.src_bg_width = priv.res_cfg[priv.res_cfg_cnt - 1].width;
    priv.curr_max_dim.src_bg_height = priv.res_cfg[priv.res_cfg_cnt - 1].height;

    priv.chan_param = (chan_param_t *) kzalloc(sizeof(chan_param_t) * (max_minors), GFP_KERNEL);
    if (ZERO_OR_NULL_PTR(priv.chan_param))
        panic("%s: allocate memory fail for priv.chan_param(0x%p)!\n", __func__, priv.chan_param);

    /* base on spec.cfg/gmlib.cfg enable resolution to allocate variance buffer */
    if (ft3dnr_var_buf_alloc() < 0) {
        printk("failed to alloc var buf!\n");
        return -ENOMEM;
    }
#ifdef VIDEOGRAPH_INC
    ft3dnr_vg_init();
#endif
    ft3dnr_init_default_param();

    /* init timer */
    init_timer(&priv.engine.timer);
    priv.engine.timer.function = ft3dnr_lli_timeout;
    priv.engine.timer.data = dev;
    priv.engine.timeout = msecs_to_jiffies(FT3DNR_LLI_TIMEOUT);

    /* init proc */
    ft3dnr_proc_init();

    return 0;
}

static void __exit ft3dnr_drv_exit(void)
{
#ifdef USE_WQ
    /* cancel all works */
    cancel_delayed_work(&process_job_work);
#endif
#ifdef USE_KTHREAD
    if (priv.add_task)
        kthread_stop(priv.add_task);

    while (add_table == 1)
        msleep(1);
#endif

    ft3dnr_proc_remove();

#ifdef VIDEOGRAPH_INC
    ft3dnr_vg_driver_clearnup();
#endif
#ifdef USE_WQ
    /* destroy workqueue */
    destroy_workqueue(job_workq);
#endif
    platform_driver_unregister(&ft3dnr_driver);

    /* delete timer */
    del_timer(&priv.engine.timer);

    /* destroy the memory cache */
    kmem_cache_destroy(priv.job_cache);

    /* free CVBS frame temporal buffer */
    ft3dnr_var_buf_free();

    kfree(priv.chan_param);
    kfree(priv.res_cfg);

    platform_exit();

    free_irq(priv.engine.irq, (void *)0);
    __iounmap((void *)priv.engine.ft3dnr_reg);
}

module_init(ft3dnr_drv_init);
module_exit(ft3dnr_drv_exit);
MODULE_LICENSE("GPL");
