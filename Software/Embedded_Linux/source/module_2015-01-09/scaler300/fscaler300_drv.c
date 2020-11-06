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
#include <mach/ftpmu010.h>
#include <mach/fmem.h>
#include "frammap_if.h"
#include <cpu_comm.h>
#include <linux/delay.h>

#include "fscaler300.h"
#include "fscaler300_osd.h"
#include "fscaler300_dbg.h"
#include "util.h"

#ifdef VIDEOGRAPH_INC
#include "fscaler300_vg.h"
#include <log.h>    //include log system "printm","damnit"...
#define MODULE_NAME         "SL"    //<<<<<<<<<< Modify your module name (two bytes) >>>>>>>>>>>>>>
#endif

/* module parameter */
/* declare max minor number */
int max_minors = MAX_CHAN_NUM;
module_param(max_minors, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(max_minors, "max minor number");
/* declare max virtual channel number */
int max_vch_num = MAX_VCH_NUM;
module_param(max_vch_num, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(max_vch_num, "max virtual channel number");

/* declare temporal frame width */
int temp_width = TEMP_WIDTH;
module_param(temp_width, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(temp_width, " temporal frame width");
/* declare temporal frame height */
int temp_height = TEMP_HEIGHT;
module_param(temp_height, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(temp_height, "temporal frame height");
/* declare 0 = NCNB or 1 = NCB */
int use_ioremap_wc = 0;
module_param(use_ioremap_wc, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(use_ioremap_wc, "set 1 as NCB");
/* declare PIP1 CVBS frame width */
int pip1_width = 0;
module_param(pip1_width, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(pip1_width, "pip1 frame width");
/* declare PIP CVBS frame height */
int pip1_height = 0;
module_param(pip1_height, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(pip1_height, "pip1 frame height");
/* max timeout ms */
int timeout = 30;
module_param(timeout, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(timeout, "timeout ms");
/* DDR index */
int ddr_idx = 0;
module_param(ddr_idx, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ddr_idx, "ddr index");
/* damnitoff */
int damnitoff = 1;
module_param(damnitoff, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(damnitoff, "damnitoff");

wait_queue_head_t add_thread_wait;
int add_wakeup_event = 0;

extern global_info_t global_info;
extern int flow_check;

/* Global declaration
 */
fscaler300_priv_t priv;
fscaler300_param_t dummy;
#define I_AM_EP(x)    ((x) & (0x1 << 5))
#ifdef USE_WQ
struct workqueue_struct *job_workq;
static DECLARE_DELAYED_WORK(process_job_work, 0);
#endif
static struct proc_dir_entry *entity_proc;

/*
 * Extern function declarations
 */
extern irqreturn_t fscaler300_interrupt(int irq, void *devid);
extern int platform_init(void);
extern int platform_exit(void);
extern unsigned int callback_period;

/*
 * function declaration
 */

/*
 * choose engine id to service this job
 */
int choose_engine_id(fscaler300_job_t *job)
{
    int i;
    int dev = 0;
    int job_cnt0 = 0, job_cnt1 = 0;
    unsigned long flags;

    //scl_dbg("%s\n", __func__);
    /* only 1 engine, return engine 0 */
    if (priv.eng_num == 1)
        return 0;

    /* if engine number > 1 , do following sequence */
    /* check engineX service this channel or not,
     * if engineX already service this channel,
     * this job can only select this engineX
     */
    spin_lock_irqsave(&priv.lock, flags);
    for (dev = 0; dev < priv.eng_num; dev++)
        for (i = 0; i < CH_BITMASK_NUM; i++)
            if (priv.engine[dev].ch_bitmask.mask[i] & job->ch_bitmask.mask[i]) {
                spin_unlock_irqrestore(&priv.lock, flags);
                return dev;
            }
    spin_unlock_irqrestore(&priv.lock, flags);
    /* if all engine do not service this channel,
     * we select engineX that service less job
     */
    job_cnt0 = atomic_read(&priv.engine[0].job_cnt);
    job_cnt1 = atomic_read(&priv.engine[1].job_cnt);

    if (job_cnt0 >= job_cnt1)
        dev = 1;
    else
        dev = 0;

    return dev;
}

/*
 * insert priority chain job(multi job) to qlist
 */
int fscaler300_insert_pchain(int dev, struct list_head *list)
{
    int i, has_job = 0, qjob = 0;
    fscaler300_job_t *parent, *curr, *ne, *first, *last, *job;
    job_node_t *node, *before2;
    fscaler300_job_t *before = NULL;
    int flag = 0;
    unsigned long flags;

    parent = list_first_entry(list, fscaler300_job_t, plist);
    spin_lock_irqsave(&priv.lock, flags);

    job = list_first_entry(list, fscaler300_job_t, plist);
    node = (job_node_t *)job->input_node[0].private;
    /* check this engine has service this channel or not */
    for (i = 0; i < CH_BITMASK_NUM; i++) {
        if (priv.engine[dev].ch_bitmask.mask[i] & parent->ch_bitmask.mask[i]) {
            has_job = 1;
            break;
        }
    }
    /* if this engine do not service this channel, insert to queue list head
     * if this engine has service this channel, insert to last job of this channel
     */
    if (has_job == 0) {
        if (list_empty(&priv.engine[dev].qlist))
            list_splice(list, &priv.engine[dev].qlist);
        else {
            first = list_first_entry(&priv.engine[dev].qlist, fscaler300_job_t, plist);
            if (first->parent != first) {
                list_splice(list, &first->plist);
                before = first->parent;
                flag = ADD_AFTER;
            }
            else {
                list_splice_tail(list, &first->plist);
                before = first->parent;
                flag = ADD_BEFORE;
            }
        }
    }
    else {
        list_for_each_entry_safe_reverse(curr, ne, &priv.engine[dev].qlist, plist) {
            for (i = 0; i < CH_BITMASK_NUM; i++) {
                if (curr->ch_bitmask.mask[i] & parent->ch_bitmask.mask[i]) {
                    list_splice(list, &curr->plist);
                    before = curr;
                    qjob = 1;
                    break;
                }
            }
            if (qjob == 1)
                break;
        }
        /* this channel has no job in qlist, insert as qlist first job */
        if (qjob == 0) {
            list_splice(list, &priv.engine[dev].qlist);
            if (list_empty(&priv.engine[dev].slist) && !list_empty(&priv.engine[dev].wlist)) {
                last = list_entry(priv.engine[dev].wlist.prev, fscaler300_job_t, plist);
                before = last->parent;
                flag = ADD_AFTER;
            }
            if (!list_empty(&priv.engine[dev].slist)) {
                last = list_entry(priv.engine[dev].slist.prev, fscaler300_job_t, plist);
                before = last->parent;
                flag = ADD_AFTER;
            }
        }
    }

    if (before == NULL)
        list_add_tail(&node->plist, &global_info.node_list);
    else {
        before2 = (job_node_t *)before->input_node[0].private;
        if (flag == ADD_AFTER)
            list_add(&node->plist, &before2->plist);
        else
            list_add_tail(&node->plist, &before2->plist);
    }

    spin_unlock_irqrestore(&priv.lock, flags);

    return 0;
}

/*
 * insert priority job(single job) to qlist
 */
int fscaler300_insert_priority(int dev, fscaler300_job_t *job)
{
    int i, has_job = 0, qjob = 0;
    fscaler300_job_t *curr, *ne, *first, *last;
    job_node_t *node, *before2;
    fscaler300_job_t *before = NULL;
    int flag = 0;
    unsigned long flags;

    //scl_dbg("%s\n", __func__);

    spin_lock_irqsave(&priv.lock, flags);

    node = (job_node_t *)job->input_node[0].private;
    /* check this engine has service this channel or not */
    for (i = 0; i < CH_BITMASK_NUM; i++) {
        if (priv.engine[dev].ch_bitmask.mask[i] & job->ch_bitmask.mask[i]) {
            has_job = 1;
            break;
        }
    }
    /* if this engine do not service this channel, insert to queue list head
     * if this engine has service this channel, insert to last job of this channel
     */
    if (has_job == 0) {
        if (list_empty(&priv.engine[dev].qlist))
            list_add(&job->plist, &priv.engine[dev].qlist);
        else {
            first = list_first_entry(&priv.engine[dev].qlist, fscaler300_job_t, plist);
            if (first->parent != first) {
                list_add(&job->plist, &first->plist);
                before = first->parent;
                flag = ADD_AFTER;
            }
            else {
                list_add_tail(&job->plist, &first->plist);
                before = first->parent;
                flag = ADD_BEFORE;
            }
        }
    }
    else {
        list_for_each_entry_safe_reverse(curr, ne, &priv.engine[dev].qlist, plist) {
            if (curr->ch_bitmask.mask[i] & job->ch_bitmask.mask[i]) {
                list_add(&job->plist, &curr->plist);
                before = curr->parent;
                flag = ADD_AFTER;
                qjob = 1;
                break;
            }
        }
        /* this channel has no job in qlist, insert as qlist first job */
        if (qjob == 0) {
            list_add(&job->plist, &priv.engine[dev].qlist);
            if (list_empty(&priv.engine[dev].slist) && !list_empty(&priv.engine[dev].wlist)) {
                last = list_entry(priv.engine[dev].wlist.prev, fscaler300_job_t, plist);
                before = last->parent;
                flag = ADD_AFTER;
            }
            if (!list_empty(&priv.engine[dev].slist)) {
                last = list_entry(priv.engine[dev].slist.prev, fscaler300_job_t, plist);
                before = last->parent;
                flag = ADD_AFTER;
            }
        }
    }

    if (before == NULL)
        list_add_tail(&node->plist, &global_info.node_list);
    else {
        before2 = (job_node_t *)before->input_node[0].private;
        if (flag == ADD_AFTER)
            list_add(&node->plist, &before2->plist);
        else
            list_add_tail(&node->plist, &before2->plist);
    }

    spin_unlock_irqrestore(&priv.lock, flags);

    return 0;
}

/*
 * add priority job(multi job) to engine queue list
 */
int fscaler300_pchainlist_add(struct list_head *list)
{
    int dev, i;
    fscaler300_job_t *parent;
    unsigned long flags;

    //scl_dbg("%s\n", __func__);

    parent = list_first_entry(list, fscaler300_job_t, plist);
    /* choose which engine to service this job */
    dev = choose_engine_id(parent);

    /* add job to engine's queue list */
    fscaler300_insert_pchain(dev, list);

    spin_lock_irqsave(&priv.lock, flags);
    /* add parent job's channel bitmask to engine's channel bitmask */
    for (i = 0; i < CH_BITMASK_NUM; i++)
        priv.engine[dev].ch_bitmask.mask[i] |= parent->ch_bitmask.mask[i];

    spin_unlock_irqrestore(&priv.lock, flags);

    /* add job count to engine's job count */
    JOBCNT_ADD(&priv.engine[dev], parent->job_cnt);

    return 0;
}

/*
 * add job(multi job) to engine queue list
 */
int fscaler300_chainlist_add(struct list_head *list)
{
    int dev, i;
    fscaler300_job_t *parent;
    unsigned long flags;

    //scl_dbg("%s\n", __func__);

    parent = list_first_entry(list, fscaler300_job_t, plist);
    /* choose which engine to service this job */
    dev = choose_engine_id(parent);

    spin_lock_irqsave(&priv.lock, flags);
    /* add job to engine's queue list */
    list_splice_tail(list, &priv.engine[dev].qlist);

    /* add parent job's channel bitmask to engine's channel bitmask */
    for (i = 0; i < CH_BITMASK_NUM; i++)
        priv.engine[dev].ch_bitmask.mask[i] |= parent->ch_bitmask.mask[i];

    spin_unlock_irqrestore(&priv.lock, flags);

    /* add job count to engine's job count */
    JOBCNT_ADD(&priv.engine[dev], parent->job_cnt);

    return 0;
}

/*
 * add priority job(single job) to engine queue list
 */
int fscaler300_prioritylist_add(fscaler300_job_t *job)
{
    int dev, i;
    unsigned long flags;

    //scl_dbg("%s [%d]\n", __func__, job->job_id);
    /* choose which engine to service this job */
    dev = choose_engine_id(job);

    /* add job to engine's queue list */
    fscaler300_insert_priority(dev, job);

    spin_lock_irqsave(&priv.lock, flags);
    /* add parent job's channel bitmask to engine's channel bitmask */
    for (i = 0; i < CH_BITMASK_NUM; i++)
        priv.engine[dev].ch_bitmask.mask[i] |= job->ch_bitmask.mask[i];

    spin_unlock_irqrestore(&priv.lock, flags);

    /* add job count to engine's job count */
    JOBCNT_ADD(&priv.engine[dev], job->job_cnt);

    return 0;
}

/*
 * add job(single job) to engine queue list
 */
int fscaler300_joblist_add(fscaler300_job_t *job)
{
    int dev, i;
    unsigned long flags;

    //scl_dbg("%s\n", __func__);
    /* choose which engine to service this job */
    dev = choose_engine_id(job);

    spin_lock_irqsave(&priv.lock, flags);
    /* add job to engine's queue list */
    /* single job --> direct add job to qlist */
    list_add_tail(&job->plist, &priv.engine[dev].qlist);

    /* add parent job's channel bitmask to engine's channel bitmask */
    for (i = 0; i < CH_BITMASK_NUM; i++)
        priv.engine[dev].ch_bitmask.mask[i] |= job->ch_bitmask.mask[i];

    spin_unlock_irqrestore(&priv.lock, flags);

    /* add job count to engine's job count */
    JOBCNT_ADD(&priv.engine[dev], job->job_cnt);

    return 0;
}

/*
 * get scaler300 link list memory block number, blk = 0 --> free, blk = 1 --> used
 */
int fscaler300_get_free_blk(int dev)
{
    int blk;
    int i, j;
    unsigned long flags;
#if GM8210
    int path = 4;
#elif GM8287
    int path = 2;
#else
    int path = 1;
#endif

    spin_lock_irqsave(&priv.lock, flags);

    for (i = 0; i < path; i++) {
        if (priv.engine[dev].sram_table[i] == 0xFFFFFFFF)
            continue;
        else
            break;
    }

    for (j = 0; j < 32; j++)
        if ((priv.engine[dev].sram_table[i] >> j & 0x1) == 0)
            break;

    blk = (i << 5) + j;

    spin_unlock_irqrestore(&priv.lock, flags);

    return blk;
}

/*
 *  set scaler300 link list memory block as used
 */
void fscaler300_set_blk_used(int dev, int blk)
{
    int base, index;
    unsigned long flags;

    base = blk >> 5;
    index = blk % 32;

    //printk("set blk [%d] used\n", blk);

    spin_lock_irqsave(&priv.lock, flags);

    if (((priv.engine[dev].sram_table[base] >> index) & 0x1) == 0x1)
        scl_err("error!! this blk should be free [%d]\n", blk);

    priv.engine[dev].sram_table[base] |= (0x1 << index);

    spin_unlock_irqrestore(&priv.lock, flags);
}

/*
 *  set scaler300 link list memory block as used
 */
void fscaler300_set_blk_free(int dev, fscaler300_job_t *job)
{
    int i, blk;
    int base, index;
    fscaler300_job_t *curr, *ne;
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);

    /* free all this chain list block */
    /* free parent job block */
    for (i = 0; i < BLK_MAX; i++) {
        if (job->ll_blk.blk_num[i] != -1) {
            blk = job->ll_blk.blk_num[i];
            base = blk >> 5;
            index = blk % 32;

            if (((priv.engine[dev].sram_table[base] >> index) & 0x1) == 0)
                scl_err("error!! this blk should be used [%d]\n", blk);

            priv.engine[dev].sram_table[base] ^= (0x1 << index);
            priv.engine[dev].blk_cnt++;
            job->ll_blk.blk_num[i] = -1;
        }
    }
    /* free child job block */
    list_for_each_entry_safe(curr, ne, &job->clist, clist) {
        for (i = 0; i < BLK_MAX; i++) {
            if (curr->ll_blk.blk_num[i] != -1) {
                blk = curr->ll_blk.blk_num[i];
                base = blk >> 5;
                index = blk % 32;

                if (((priv.engine[dev].sram_table[base] >> index) & 0x1) == 0)
                    scl_err("error!! this blk should be used [%d]\n", blk);

                priv.engine[dev].sram_table[base] ^= (0x1 << index);
                priv.engine[dev].blk_cnt++;
                curr->ll_blk.blk_num[i] = -1;
            }
        }
    }
    spin_unlock_irqrestore(&priv.lock, flags);
}
#ifdef SPLIT_SUPPORT
int fscaler300_set_split_lli_blk(int dev, fscaler300_job_t *job, int last)
{
    int ret = 0, i;
    u32 base = 0, next_base = 0, offset = 0;
    fscaler300_job_t    *last_job, *p_job;
    u32 addr = 0;
    int blk_num = 0;
    unsigned long flags;
    //scl_dbg("%s\n", __func__);
    for(i = 0; i < BLK_MAX; i++)
        job->ll_blk.blk_num[i] = -1;

    job->ll_blk.blk_count = job->split_flow.count;
    /* get free block */
    blk_num = fscaler300_get_free_blk(dev);
    base = job->ll_blk.status_addr = LL_ADDR(blk_num);
    job->ll_blk.blk_num[0] = blk_num;
    /* set this blk as used */
    fscaler300_set_blk_used(dev, blk_num);

    /* get next free block */
    blk_num = fscaler300_get_free_blk(dev);
    next_base = LL_ADDR(blk_num);

    spin_lock_irqsave(&priv.lock, flags);
    // in sram list,
    // if last job in sram list is "last job"(job's next link list pointer = 0),
    // need to change job's "next link list pointer" in link list memory to this job's start addr.
    if (!list_empty(&priv.engine[dev].slist)) {
        p_job = list_entry(priv.engine[dev].slist.prev, fscaler300_job_t, plist);
        if (p_job->job_cnt > 1)
            last_job = list_entry(p_job->clist.prev, fscaler300_job_t, clist);
        else
            last_job = p_job;

        if (last_job->ll_blk.is_last == 1) {
            addr = base;
            last_job->ll_blk.is_last = 0;
            ret = fscaler300_write_lli_cmd(dev, addr, last_job, 1);
            if (ret < 0)
                printk("write lli cmd fail\n");
        }
    }
    spin_unlock_irqrestore(&priv.lock, flags);

    offset = fscaler300_lli_split_blk0_setup(dev, job, base, next_base, last);

    // get next free block address in link list memory
    base = next_base;
    job->ll_blk.blk_num[1] = blk_num;
    /* set this blk as used */
    fscaler300_set_blk_used(dev, blk_num);
    /* get next free block */
    blk_num = fscaler300_get_free_blk(dev);
    next_base = LL_ADDR(blk_num);
    offset = fscaler300_lli_split_blk1_setup(dev, job, base, next_base, last);

    // get next free block address in link list memory
    base = next_base;
    job->ll_blk.blk_num[2] = blk_num;
    /* set this blk as used */
    fscaler300_set_blk_used(dev, blk_num);
    /* get next free block */
    blk_num = fscaler300_get_free_blk(dev);
    next_base = LL_ADDR(blk_num);
    offset = fscaler300_lli_split_blk2_setup(dev, job, base, next_base, last);

    if (job->split_flow.ch_7_13) {
        // get next free block address in link list memory
        base = next_base;
        job->ll_blk.blk_num[3] = blk_num;
        /* set this blk as used */
        fscaler300_set_blk_used(dev, blk_num);
        /* get next free block */
        blk_num = fscaler300_get_free_blk(dev);
        next_base = LL_ADDR(blk_num);
        offset = fscaler300_lli_split_blk3_setup(dev, job, base, next_base, last);
    }

    if (job->split_flow.ch_14_20) {
        // get next free block address in link list memory
        base = next_base;
        job->ll_blk.blk_num[4] = blk_num;
        /* set this blk as used */
        fscaler300_set_blk_used(dev, blk_num);
        /* get next free block */
        blk_num = fscaler300_get_free_blk(dev);
        next_base = LL_ADDR(blk_num);
        offset = fscaler300_lli_split_blk4_setup(dev, job, base, next_base, last);
    }

    if (job->split_flow.ch_21_27) {
        // get next free block address in link list memory
        base = next_base;
        job->ll_blk.blk_num[5] = blk_num;
        /* set this blk as used */
        fscaler300_set_blk_used(dev, blk_num);
        /* get next free block */
        blk_num = fscaler300_get_free_blk(dev);
        next_base = LL_ADDR(blk_num);
        offset = fscaler300_lli_split_blk5_setup(dev, job, base, next_base, last);
    }

    if (job->split_flow.ch_28_31) {
        // get next free block address in link list memory
        base = next_base;
        job->ll_blk.blk_num[6] = blk_num;
        /* set this blk as used */
        fscaler300_set_blk_used(dev, blk_num);
        /* get next free block */
        blk_num = fscaler300_get_free_blk(dev);
        next_base = LL_ADDR(blk_num);
        offset = fscaler300_lli_split_blk6_setup(dev, job, base, next_base, last);
    }

    spin_lock_irqsave(&priv.lock, flags);
    priv.engine[dev].blk_cnt -= job->split_flow.count;
    spin_unlock_irqrestore(&priv.lock, flags);

    job->ll_blk.last_cmd_addr = offset;
    job->ll_blk.is_last = last;
    job->status = JOB_STS_SRAM;

    return 0;
}
#endif

int fscaler300_set_lli_blk(int dev, fscaler300_job_t *job, int last)
{
    int ret = 0, i;
    u32 base = 0, next_base = 0, offset = 0;
    fscaler300_job_t    *last_job, *p_job;
    u32 addr = 0;
    int blk_num = 0;
    //int mpath, pp_sel, chn, win_idx;
    unsigned long flags;

    //scl_dbg("%s\n", __func__);
    for(i = 0; i < BLK_MAX; i++)
        job->ll_blk.blk_num[i] = -1;

    job->ll_blk.blk_count = job->ll_flow.count;
    /* get free block */
    blk_num = fscaler300_get_free_blk(dev);
    base = job->ll_blk.status_addr = LL_ADDR(blk_num);
    job->ll_blk.blk_num[0] = blk_num;
    /* set this blk as used */
    fscaler300_set_blk_used(dev, blk_num);

    /* get next free block */
    blk_num = fscaler300_get_free_blk(dev);
    next_base = LL_ADDR(blk_num);

    spin_lock_irqsave(&priv.lock, flags);
    // in sram list,
    // if last job in sram list is "last job"(job's next link list pointer = 0),
    // need to change job's "next link list pointer" in link list memory to this job's start addr.
    if (!list_empty(&priv.engine[dev].slist)) {
        p_job = list_entry(priv.engine[dev].slist.prev, fscaler300_job_t, plist);
        if (p_job->job_cnt > 1)
            last_job = list_entry(p_job->clist.prev, fscaler300_job_t, clist);
        else
            last_job = p_job;

        if (last_job->ll_blk.is_last == 1) {
            addr = base;
            last_job->ll_blk.is_last = 0;
            ret = fscaler300_write_lli_cmd(dev, addr, last_job, 1);
            if (ret < 0)
                printk("write lli cmd fail\n");
        }
    }
    spin_unlock_irqrestore(&priv.lock, flags);

    offset = fscaler300_lli_blk0_setup(dev, job, base, next_base, last);

    if (job->ll_flow.res123 == 1 || job->ll_flow.mask4567 == 1) {
        // get next free block address in link list memory
        base = next_base;
        job->ll_blk.blk_num[1] = blk_num;
        /* set this blk as used */
        fscaler300_set_blk_used(dev, blk_num);
        /* get next free block */
        blk_num = fscaler300_get_free_blk(dev);
        next_base = LL_ADDR(blk_num);
        offset = fscaler300_lli_blk1_setup(dev, job, base, next_base, last);
    }
#if GM8210
    if (job->ll_flow.osd0123456 == 1) {
        // get next free block address in link list memory
        base = next_base;
        job->ll_blk.blk_num[2] = blk_num;
        /* set this blk as used */
        fscaler300_set_blk_used(dev, blk_num);
        /* get next free block */
        blk_num = fscaler300_get_free_blk(dev);
        next_base = LL_ADDR(blk_num);
        offset = fscaler300_lli_blk2_setup(dev, job, base, next_base, last);
    }

    if (job->ll_flow.osd7 == 1) {
        // get next free block address in link list memory
        base = next_base;
        job->ll_blk.blk_num[3] = blk_num;
        /* set this blk as used */
        fscaler300_set_blk_used(dev, blk_num);
        /* get next free block */
        blk_num = fscaler300_get_free_blk(dev);
        next_base = LL_ADDR(blk_num);
        offset = fscaler300_lli_blk3_setup(dev, job, base, next_base, last);
    }
#endif

    spin_lock_irqsave(&priv.lock, flags);
    priv.engine[dev].blk_cnt -= job->ll_flow.count;
    spin_unlock_irqrestore(&priv.lock, flags);

#if 0   /* mask & osd external api do not use, remove it for performance */
    spin_lock_irqsave(&priv.lock, flags);
    priv.engine[dev].blk_cnt -= job->ll_flow.count;
    if (job->param.sc.fix_pat_en == 0) {
        if (job->job_type == MULTI_SJOB)
            mpath = 4;
        else
            mpath = 1;

        for (i = 0; i < mpath; i++) {
            if (!job->input_node[i].enable)
                continue;

            chn = job->input_node[i].minor;
            for (win_idx = 0; win_idx < MASK_MAX; win_idx++) {
                if (!job->mask_pp_sel[i][win_idx])
                    continue;

                pp_sel = job->mask_pp_sel[i][win_idx] - 1;
                if (priv.global_param.chn_global[chn].mask[pp_sel][win_idx].enable)
                    MASK_REF_DEC(&priv.global_param.chn_global[chn], pp_sel, win_idx);
            }
        }
#if GM8210
        /* virtual job & cvbs zoom no osd */
        if (job->param.scl_feature & (3 << 1))
            mpath = 0;

        for (i = 0; i < mpath; i++) {
            if (!job->input_node[i].enable)
                continue;

            chn = job->input_node[i].minor;
            for (win_idx = 0; win_idx < OSD_MAX; win_idx++) {
                if (!job->osd_pp_sel[i][win_idx])
                    continue;

                pp_sel = job->osd_pp_sel[i][win_idx] - 1;
                if (priv.global_param.chn_global[chn].osd[pp_sel][win_idx].enable)
                    OSD_REF_DEC(&priv.global_param.chn_global[chn], pp_sel, win_idx);
            }
        }
#endif
    }
    spin_unlock_irqrestore(&priv.lock, flags);
#endif  // end of if 0
    job->ll_blk.last_cmd_addr = offset;
    job->ll_blk.is_last = last;
    job->status = JOB_STS_SRAM;

    return 0;
}

/*
 * write job in sram list to link list memory
 */
#ifdef SPLIT_SUPPORT
void fscaler300_write_split_table(int dev, fscaler300_job_t *job, int last)
{
    unsigned long flags;
    //scl_dbg("%s\n", __func__);
    fscaler300_set_split_lli_blk(dev, job, last);

    // move job to sram list
    spin_lock_irqsave(&priv.lock, flags);
    list_move_tail(&job->plist, &priv.engine[dev].slist);
    spin_unlock_irqrestore(&priv.lock, flags);
}
#endif
/*
 * write job in sram list to link list memory
 */
void fscaler300_write_table(int dev, fscaler300_job_t *job, int last)
{
    fscaler300_job_t *curr, *ne;
    int last_job = 0;
    unsigned long flags;
    if (flow_check) printm("SL", "%s dev %d id %u jobcnt %d\n", __func__, dev, job->job_id, job->job_cnt);
    if (job->job_cnt > 1)   // if has child job --> not last job
        last_job = 0;
    else                    // don't has child job
        last_job = last;

    fscaler300_set_lli_blk(dev, job, last_job);
    /* write child job */
    if (job->job_cnt > 1) {
        list_for_each_entry_safe(curr, ne, &job->clist, clist) {
            last_job = 0;

            if (last == 1 && (list_is_last(&curr->clist, &job->clist) == 1))
                last_job = 1;

            fscaler300_set_lli_blk(dev, curr, last_job);
        }
    }

    // move job to sram list
    spin_lock_irqsave(&priv.lock, flags);
    list_move_tail(&job->plist, &priv.engine[dev].slist);
    spin_unlock_irqrestore(&priv.lock, flags);

}

int fscaler300_alloc_table(int dev, fscaler300_job_t *job, int last)
{
    /* write job to link list memory */
    //if (job->param.scl_feature != 4)  // if split enable, need to modify more
    fscaler300_write_table(dev, job, last);
#ifdef SPLIT_SUPPORT    // if split enable, need to modify more
    if (job->param.scl_feature == 4)
        fscaler300_write_split_table(dev, job, last);
#endif
    /* update node from V.G status */
    if (job->input_node[0].fops && job->input_node[0].fops->update_status)
        job->input_node[0].fops->update_status(job, JOB_STS_SRAM);

    return 0;
}

/*
 * workqueue to add jobs to link list memory
 * we add at most MAX_CHAIN_JOB jobs at a time to link list memory, job count could be modified.
 */
void fscaler300_add_table(void)
{
    int dev = 0;
    int j = 0;
    int blk_cnt[2] = {0};
    int job_blk_cnt = 0, last_job = 0, chain_job = 0;
    int ret = 0, offset = 0;
    fscaler300_job_t *job, *ne, *child, *ne1;
    fscaler300_job_t *first_job;
    unsigned long flags;
    //scl_dbg("%s\n", __func__);

    spin_lock_irqsave(&priv.lock, flags);
    if (flow_check) printm("SL", "%s\n", __func__);
    for (j = 0; j < priv.eng_num; j++) {
        blk_cnt[0] = priv.engine[0].blk_cnt;
#if GM8210
        if (j == 0) {
            if (priv.eng_num == 2) {
                blk_cnt[1] = priv.engine[1].blk_cnt;
                if (blk_cnt[0] > blk_cnt[1])
                    dev = 1;
                else
                    dev = 0;
            }
            else
                dev = 0;
        }
        else
            dev ^= 1;
#else
        dev = 0;
#endif

        job_blk_cnt = chain_job = 0;
        // find how many jobs still queue, choose up to MAX_CHAIN_JOB jobs, then stop, others wait to next time.
        list_for_each_entry_safe(job, ne, &priv.engine[dev].qlist, plist) {
            if (job->status != JOB_STS_QUEUE)
                continue;
            /* calculate this job need how many blocks in link list memory */
            /* add parent job block count */
            //if (job->param.scl_feature != 4)  // if split enable, need to modify more
            job_blk_cnt += job->ll_flow.count;
#ifdef SPLIT_SUPPORT
            else
                job_blk_cnt += job->split_flow.count;
#endif
            /* add child job block count */
            list_for_each_entry_safe(child, ne1, &job->clist, clist)
                job_blk_cnt += child->ll_flow.count;

            if (job_blk_cnt <= blk_cnt[dev])
                chain_job++;
            else
                break;
        }
        // add at most MAX_CHAIN_JOB jobs to link list memory
        if (chain_job > 0) {
            list_for_each_entry_safe(job, ne, &priv.engine[dev].qlist, plist) {
                if (job->status != JOB_STS_QUEUE)
                    continue;

                chain_job--;
                if (chain_job == 0)
                    last_job = 1;
                else
                    last_job = 0;
				//if (1) printm(MODULE_NAME, "write job %u\n", job->job_id);
                ret = fscaler300_alloc_table(dev, job, last_job);
                if (ret < 0)
                    break;

                if (last_job == 1)
                    break;
            }
        }

        // after write job to link list memory, check HW status,
        // if HW idle, trigger HW & start from dummy job, if HW is running, do nothing
        if (!ENGINE_BUSY(dev)) {
            if (!list_empty(&priv.engine[dev].slist)) {
                first_job = list_entry(priv.engine[dev].slist.next, fscaler300_job_t, plist);
                offset = first_job->ll_blk.status_addr;
                ret = fscaler300_write_dummy(dev, offset, &dummy, first_job);
                if (ret < 0)
                    printk("write dummy job fail\n");

                // move jobs from sram list into working list
                list_for_each_entry_safe(job, ne, &priv.engine[dev].slist, plist) {
                    list_move_tail(&job->plist, &priv.engine[dev].wlist);
                    job->status = JOB_STS_ONGOING;
                    /* update node status */
                    if (job->input_node[0].fops && job->input_node[0].fops->update_status)
                        job->input_node[0].fops->update_status(job, JOB_STS_ONGOING);
                }
                mark_engine_start(dev);
                // fire
                ret = fscaler300_single_fire(dev);
                if (ret < 0)
                    printk("fail to fire\n");
                LOCK_ENGINE(dev);
            }
        }
        //else
          //  printk(KERN_DEBUG"engine busy\n");
    }

#ifdef SINGLE_FIRE
    dev = 0;
    if (!ENGINE_BUSY(dev)) {
        if (!list_empty(&priv.engine[dev].qlist)) {
            first_job = list_entry(priv.engine[dev].qlist.next, fscaler300_job_t, plist);
            list_move_tail(&first_job->plist, &priv.engine[dev].wlist);
            first_job->status = JOB_STS_ONGOING;
            //if (first_job->param.scl_feature != 4)  // if split enable, need to modify more
                ret = fscaler300_write_register(dev, first_job);
#ifdef SPLIT_SUPPORT    // if split enable, need to modify more
            if (first_job->param.scl_feature == 4)
                ret = fscaler300_write_split_register(dev, first_job);
#endif
            if (ret < 0)
                printk("write register fail\n");
            // fire
            ret = fscaler300_single_fire(dev);
            if (ret < 0)
                printk("fail to fire\n");
            LOCK_ENGINE(dev);
        }
    }
#endif
    spin_unlock_irqrestore(&priv.lock, flags);
}
#ifdef USE_KTHREAD
int add_table = 0;
static int add_thread(void *__cwq)
{
    int status;
    add_table = 1;
    set_current_state(TASK_INTERRUPTIBLE);
    //set_user_nice(current, -20);

    do {
        status = wait_event_timeout(add_thread_wait, add_wakeup_event, msecs_to_jiffies(1000));
        //if (status == 0)
          //  continue;   /* timeout */

        add_wakeup_event = 0;
        /* add table */
        fscaler300_add_table();
    } while(!kthread_should_stop());

#if 0
    do {
        set_user_nice(current, -20);
        schedule();
        __set_current_state(TASK_RUNNING);

        if (!list_empty(&priv.engine[0].qlist) || !list_empty(&priv.engine[1].qlist))
            fscaler300_add_table();

        set_current_state(TASK_INTERRUPTIBLE);
    } while(!kthread_should_stop());
#endif
    __set_current_state(TASK_RUNNING);
    add_table = 0;
    return 0;
}

static void add_wake_up(void)
{
    add_wakeup_event = 1;
    wake_up(&add_thread_wait);
    //wake_up_process(priv.add_task);
}
#endif
/*
 * free job from ISR, free blocks, and callback to V.G
 */
void fscaler300_job_free(int dev, fscaler300_job_t *job, int schedule)
{
    int i, ref_cnt = 0;
    fscaler300_job_t *root, *curr, *ne, *child, *ne1, *parent;
    unsigned long flags;

    //scl_dbg("%s %d\n", __func__, job->job_id);

    root = job->parent;
    /* if VG multi-job has too much node, i have to separate to multi chain list jobs,
       one situation will happen, 1 VG multi job, separate to 3 multi chain list jobs,
       first chain list need_callback = 0,
       second chain list need_callback = 0,
       last chain list need_callback = 1,
       first chain list job done, the others still in qlist or slist,,
       but i can't remove first chain list job from wlist,
       because only callback = 1, i can remove root from wlist,
       so first chain list job done, but still on wlist, in interrupt, engine can't not be unlock,
       so if this job free, but need_callback = 0, check if this job is last job in wlist,
       is last job --> unlock engine,  not last job --> only wake up workqueue to service the other jobs
       call fscaler300_set_blk_free to free this chain blk at scalerX,
       if not free blk, it is not easy to choose this scaler engine at fscaler300_add_table,
       because this scaler engine's blk cnt will bigger than another engine at most time.
       */
    if (job->need_callback != 1) {
        /*spin_lock_irqsave(&priv.lock, flags);
        wlast_job = list_entry(priv.engine[dev].wlist.prev, fscaler300_job_t, plist);
        spin_unlock_irqrestore(&priv.lock, flags);*/
        parent = list_entry(job->clist.next, fscaler300_job_t, clist);
        /*if (parent == wlast_job) {
            UNLOCK_ENGINE(dev);
            mark_engine_finish(dev);
        }*/
        /* free this chain job block to release scaler engine link list memory */
        fscaler300_set_blk_free(dev, parent);
    }
    else {
        /* free child chain job block & free child chain job */
        list_for_each_entry_safe(curr, ne, &root->plist, plist) {
            if (curr->parent != root)
                break;
            else {
                fscaler300_set_blk_free(dev, curr);

                /* free child chain's child job */
                list_for_each_entry_safe(child, ne1, &curr->clist, clist) {
                    REFCNT_DEC(root);
                    kmem_cache_free(priv.job_cache, child);
MEMCNT_SUB(&priv, 1);
                }

                /* free child chain's parent job */
                REFCNT_DEC(root);
                /* decrease engine job cnt */
                JOBCNT_SUB(&priv.engine[dev], curr->job_cnt);
                list_del(&curr->plist);
                kmem_cache_free(priv.job_cache, curr);
MEMCNT_SUB(&priv, 1);
            }
        }

        /* free root job block */
        fscaler300_set_blk_free(dev, root);

            /* free root's child job */
        list_for_each_entry_safe(curr, ne, &root->clist, clist) {
            REFCNT_DEC(root);
            kmem_cache_free(priv.job_cache, curr);
MEMCNT_SUB(&priv, 1);
        }

        ref_cnt = atomic_read(&root->ref_cnt);
        /* if ref_cnt = 1, root itself, no others reference to root, we can callback to V.G now & free root */
        if (ref_cnt == 1) {
            if (root->input_node[0].fops && root->input_node[0].fops->callback)
                root->input_node[0].fops->callback(root);
            /* decrease engine job cnt */
            JOBCNT_SUB(&priv.engine[dev], root->job_cnt);
            // delete job
            list_del(&root->plist);
            /* free root job */
            kmem_cache_free(priv.job_cache, root);
MEMCNT_SUB(&priv, 1);
        }
        else
            scl_err("error !! free root job but still someone reference to root job ref_cnt [%d]\n", ref_cnt);

        spin_lock_irqsave(&priv.lock, flags);
        /* after free one node, reset engine ch_bitmask */
        memset(&priv.engine[dev].ch_bitmask, 0x0, sizeof(ch_bitmask_t));
        list_for_each_entry_safe(curr, ne, &priv.engine[dev].qlist, plist)
            for(i = 0; i < CH_BITMASK_NUM; i++)
                priv.engine[dev].ch_bitmask.mask[i] |=  curr->ch_bitmask.mask[i];

        list_for_each_entry_safe(curr, ne, &priv.engine[dev].slist, plist)
            for(i = 0; i < CH_BITMASK_NUM; i++)
                priv.engine[dev].ch_bitmask.mask[i] |=  curr->ch_bitmask.mask[i];

        list_for_each_entry_safe(curr, ne, &priv.engine[dev].wlist, plist)
            for(i = 0; i < CH_BITMASK_NUM; i++)
                priv.engine[dev].ch_bitmask.mask[i] |=  curr->ch_bitmask.mask[i];

        spin_unlock_irqrestore(&priv.lock, flags);
    }
    /* schedule */
    if (schedule) {
#ifdef USE_KTHREAD
        add_wake_up();
#endif
#ifdef USE_WQ
        PREPARE_DELAYED_WORK(&process_job_work, (void *)fscaler300_add_table);
        queue_delayed_work(job_workq, &process_job_work, callback_period);
#endif
    }
}

/*
 * stop channel's jobs
 */
int fscaler300_stop_channel(int chn)
{
    int i, j;
    int dev = -1;
    int ref_cnt;
    int chmask_base, chmask_idx;
    ch_bitmask_t ch_bitmask;
    fscaler300_job_t    *job, *ne, *curr, *temp, *temp1;
    fscaler300_job_t    *parent;
    unsigned long flags;

    //scl_dbg("%s\n", __func__);
    memset(&ch_bitmask, 0x0, sizeof(ch_bitmask_t));

    /* make channel bitmask */
    chmask_base = BITMASK_BASE(chn);
    chmask_idx = BITMASK_IDX(chn);
    ch_bitmask.mask[chmask_base] |= (0x1 << chmask_idx);

    spin_lock_irqsave(&priv.lock, flags);
    /* check this channel is service on which engine */
    for (i = 0; i < priv.eng_num; i++)
        for (j = 0; j < CH_BITMASK_NUM; j++)
            if (priv.engine[i].ch_bitmask.mask[j] & ch_bitmask.mask[j])
                dev = i;

    //printk(KERN_DEBUG"stop chn [%d] dev [%d]\n", chn, dev);

    /* search each job has this channel or not */
    if (dev != -1) {
        list_for_each_entry_safe(job, ne, &priv.engine[dev].qlist, plist) {
            if (job->ch_bitmask.mask[chmask_base] & ch_bitmask.mask[chmask_base]) {
                /* job->parent != job, it means this job is "someone's" child chain list job,
                   "someone" is already in slist or wlist or finish, we have to finish this job */
                parent = job->parent;
                if (job->parent != job) {
                    if (parent->status != JOB_STS_FLUSH)
                        continue;
                }
                job->status = JOB_STS_FLUSH;

                /* free child job */
                list_for_each_entry_safe(temp, temp1, &job->clist, clist) {
                    REFCNT_DEC(parent);
                    kmem_cache_free(priv.job_cache, temp);
MEMCNT_SUB(&priv, 1);
                }
                if (job->parent != job) {
                    REFCNT_DEC(parent);
                    list_del(&job->plist);
                    kmem_cache_free(priv.job_cache, job);
MEMCNT_SUB(&priv, 1);
                }
                /* free parent job */
                ref_cnt = atomic_read(&parent->ref_cnt);
                if (ref_cnt == 1) {
                    if (parent->input_node[0].fops && parent->input_node[0].fops->callback)
                        parent->input_node[0].fops->callback(job);
                    list_del(&parent->plist);
                    /* decrease engine job cnt */
                    JOBCNT_SUB(&priv.engine[dev], parent->job_cnt);
                    kmem_cache_free(priv.job_cache, parent);
MEMCNT_SUB(&priv, 1);
                }
                //else
                  //  scl_err("error !! free root job but still someone reference to root job ref_cnt [%d]\n", ref_cnt);
            }
        }

        /* after free one node, reset engine ch_bitmask */
        memset(&priv.engine[dev].ch_bitmask, 0x0, sizeof(ch_bitmask_t));
        list_for_each_entry_safe(curr, ne, &priv.engine[dev].qlist, plist) {
            for(i = 0; i < CH_BITMASK_NUM; i++)
                priv.engine[dev].ch_bitmask.mask[i] |=  curr->ch_bitmask.mask[i];
        }

        list_for_each_entry_safe(curr, ne, &priv.engine[dev].slist, plist) {
            for(i = 0; i < CH_BITMASK_NUM; i++)
                priv.engine[dev].ch_bitmask.mask[i] |=  curr->ch_bitmask.mask[i];
        }

        list_for_each_entry_safe(curr, ne, &priv.engine[dev].wlist, plist) {
            for(i = 0; i < CH_BITMASK_NUM; i++)
                priv.engine[dev].ch_bitmask.mask[i] |=  curr->ch_bitmask.mask[i];
        }
    }
    spin_unlock_irqrestore(&priv.lock, flags);

    return 0;
}

int fscaler300_put_job()
{
	//printm("SL","%s\n", __func__);
#ifdef USE_KTHREAD
	add_wake_up();
#endif
#ifdef USE_WQ
    PREPARE_DELAYED_WORK(&process_job_work, (void *)fscaler300_add_table);
    queue_delayed_work(job_workq, &process_job_work, callback_period);
#endif
#ifdef SINGLE_FIRE
    fscaler300_add_table();
#endif
    return 0;
}

/*
 * link list dummy job setting
 */
static void fscaler300_dummy_init(u32 src_addr, u32 dst_addr)
{
    memset(&dummy, 0x0, sizeof(fscaler300_param_t));

    /* global */
    dummy.global.capture_mode = LINK_LIST_MODE;
    dummy.global.sca_src_format = YUV422;
    dummy.global.src_split_en = 0;
    dummy.global.tc_format = 0;
    dummy.global.tc_rb_swap = 0;
    dummy.global.dma_fifo_wm = 0x10;
    dummy.global.dma_job_wm = 0x8;
    /* mirror */
    dummy.mirror.h_flip = 0;
    dummy.mirror.v_flip = 0;
    /* frm2field */
    dummy.frm2field.res0_frm2field = 0;
    dummy.frm2field.res1_frm2field = 0;
    dummy.frm2field.res2_frm2field = 0;
    dummy.frm2field.res3_frm2field = 0;
    /* sc */
    dummy.sc.type = 0;
    dummy.sc.x_start = 0;
    dummy.sc.x_end = 63;
    dummy.sc.y_start = 0;
    dummy.sc.y_end = 63;
    dummy.sc.frame_type = PROGRESSIVE;
    dummy.sc.sca_src_width = 64;
    dummy.sc.sca_src_height = 64;
    dummy.sc.fix_pat_en = 0;
    dummy.sc.dr_en = 0;
    dummy.sc.rgb2yuv_en = 0;
    dummy.sc.rb_swap = 0;
    /* sd */
    dummy.sd[0].enable = 1;
    dummy.sd[0].bypass = 1;
    dummy.sd[0].width = 64;
    dummy.sd[0].height = 64;
    dummy.sd[0].hstep = 256;
    dummy.sd[0].vstep = 256;
    /* tc */
    dummy.tc[0].enable = 0;
    dummy.tc[0].width = 64;
    dummy.tc[0].height = 64;
    dummy.tc[0].x_start = 0;
    dummy.tc[0].x_end = 63;
    dummy.tc[0].y_start = 0;
    dummy.tc[0].y_end = 63;
    /* src dma */
    dummy.src_dma.addr = src_addr;
    dummy.src_dma.line_offset = 0;
    /* dst dma */
    dummy.dest_dma[0].addr = dst_addr;
    dummy.dest_dma[0].line_offset = 0;
    /* link list cmd */
    dummy.lli.curr_cmd_addr = SCALER300_DUMMY_BASE;
    dummy.lli.next_cmd_addr = 0;
    dummy.lli.next_cmd_null = 0;
    dummy.lli.job_count = 1;
}

/*
 * init global parameter
 */
static void fscaler300_init_global_param(fscaler300_global_param_t *global_param)
{
    int i = 0;

    /* subchannel */
    /* fcs */
    global_param->init.fcs.enable = 0;
    global_param->init.fcs.lv0_th = 0xfa;
    global_param->init.fcs.lv1_th = 0xc8;
    global_param->init.fcs.lv2_th = 0xc8;
    global_param->init.fcs.lv3_th = 0x32;
    global_param->init.fcs.lv4_th = 0x28;
    global_param->init.fcs.grey_th = 0x384;
    /* denoise */
    global_param->init.dn.enable = 0;
    global_param->init.dn.adp = 1;
    global_param->init.dn.geomatric_str = 0;
    global_param->init.dn.similarity_str = 0;
    global_param->init.dn.adp_step = 0x10;
    /* smooth */
    for(i = 0; i < 4; i++) {
        global_param->init.smooth[i].hsmo_str = 0;
        global_param->init.smooth[i].vsmo_str = 0;
    }
    /* sharpness */
    for(i = 0; i < 4; i++) {
        global_param->init.sharpness[i].adp = 0;
        global_param->init.sharpness[i].amount = 0x16;
        global_param->init.sharpness[i].radius = 0;
        global_param->init.sharpness[i].threshold = 0x5;
        global_param->init.sharpness[i].adp_start = 0x2;
        global_param->init.sharpness[i].adp_step = 0x3;
    }
    /* rgb2yuv matrix */
    global_param->init.rgb2yuv_matrix.rgb2yuv_a00 = 0x28;
    global_param->init.rgb2yuv_matrix.rgb2yuv_a01 = 0x48;
    global_param->init.rgb2yuv_matrix.rgb2yuv_a02 = 0x10;
    global_param->init.rgb2yuv_matrix.rgb2yuv_a10 = 0xea;
    global_param->init.rgb2yuv_matrix.rgb2yuv_a11 = 0xd9;
    global_param->init.rgb2yuv_matrix.rgb2yuv_a12 = 0x3d;
    global_param->init.rgb2yuv_matrix.rgb2yuv_a20 = 0x52;
    global_param->init.rgb2yuv_matrix.rgb2yuv_a21 = 0xc0;
    global_param->init.rgb2yuv_matrix.rgb2yuv_a22 = 0xee;
    /* yuv2rgb matrix */
    global_param->init.yuv2rgb_matrix.yuv2rgb_a00 = 0x80;
    global_param->init.yuv2rgb_matrix.yuv2rgb_a01 = 0;
    global_param->init.yuv2rgb_matrix.yuv2rgb_a02 = 0xaf;
    global_param->init.yuv2rgb_matrix.yuv2rgb_a10 = 0x80;
    global_param->init.yuv2rgb_matrix.yuv2rgb_a11 = 0xd5;
    global_param->init.yuv2rgb_matrix.yuv2rgb_a12 = 0xa7;
    global_param->init.yuv2rgb_matrix.yuv2rgb_a20 = 0x80;
    global_param->init.yuv2rgb_matrix.yuv2rgb_a21 = 0xde;
    global_param->init.yuv2rgb_matrix.yuv2rgb_a22 = 0;
    /* palette */
    global_param->init.palette[WHITE_IDX].crcby      = OSD_PALETTE_COLOR_WHITE;
    global_param->init.palette[BLACK_IDX].crcby      = OSD_PALETTE_COLOR_BLACK;
    global_param->init.palette[RED_IDX].crcby        = OSD_PALETTE_COLOR_RED;
    global_param->init.palette[BLUE_IDX].crcby       = OSD_PALETTE_COLOR_BLUE;
    global_param->init.palette[YELLOW_IDX].crcby     = OSD_PALETTE_COLOR_YELLOW;
    global_param->init.palette[GREEN_IDX].crcby      = OSD_PALETTE_COLOR_GREEN;
    global_param->init.palette[BROWN_IDX].crcby      = OSD_PALETTE_COLOR_BROWN;
    global_param->init.palette[DODGERBLUE_IDX].crcby = OSD_PALETTE_COLOR_DODGERBLUE;
#if GM8210
    global_param->init.palette[GRAY_IDX].crcby       = OSD_PALETTE_COLOR_GRAY;
    global_param->init.palette[KHAKI_IDX].crcby      = OSD_PALETTE_COLOR_KHAKI;
    global_param->init.palette[LIGHTGREEN_IDX].crcby = OSD_PALETTE_COLOR_LIGHTGREEN;
    global_param->init.palette[MAGENTA_IDX].crcby    = OSD_PALETTE_COLOR_MAGENTA;
    global_param->init.palette[ORANGE_IDX].crcby     = OSD_PALETTE_COLOR_ORANGE;
    global_param->init.palette[PINK_IDX].crcby       = OSD_PALETTE_COLOR_PINK;
    global_param->init.palette[SLATEBLUE_IDX].crcby  = OSD_PALETTE_COLOR_SLATEBLUE;
    global_param->init.palette[AQUA_IDX].crcby       = OSD_PALETTE_COLOR_AQUA;
#endif
    /* interrupt mask */
    global_param->init.int_mask.dma_ovf         = 0;
    global_param->init.int_mask.dma_job_ovf     = 0;
    global_param->init.int_mask.dma_dis_status  = 0;
    global_param->init.int_mask.dma_wc_respfail = 0;
    global_param->init.int_mask.dma_rc_respfail = 0;
    global_param->init.int_mask.sd_job_ovf      = 0;
    global_param->init.int_mask.dma_wc_pfx_err  = 0;

    global_param->init.int_mask.ll_done         = 0;//1;
    global_param->init.int_mask.sd_fail         = 0;
    global_param->init.int_mask.dma_wc_done     = 1;
    global_param->init.int_mask.dma_rc_done     = 1;
    global_param->init.int_mask.frame_end       = 1;
#ifdef SINGLE_FIRE
    global_param->init.int_mask.ll_done         = 1;
    global_param->init.int_mask.sd_fail         = 0;
    global_param->init.int_mask.dma_wc_done     = 1;
    global_param->init.int_mask.dma_rc_done     = 1;
    global_param->init.int_mask.frame_end       = 0;
#endif
    /* dma info */
    global_param->init.dma_ctrl.dma_wc_wait_value = 0x158;
    global_param->init.dma_ctrl.dma_rc_wait_value = 0x158;
}

/*
 * Create engine and FIFO
 */
int add_engine(int dev, unsigned long addr, unsigned long irq)
{
    int idx, ret = 0;

    idx = priv.eng_num;

    priv.engine[idx].dev = idx;
    priv.engine[idx].fscaler300_reg = addr;
    priv.engine[idx].irq = irq;
    priv.eng_num++;

    return ret;
}

/* Note: fscaler300_init_one() will be run before fscaler300_drv_init() */
static int fscaler300_init_one(struct platform_device *pdev)
{
	struct resource *mem, *irq;
	unsigned long   vaddr;

    if (use_ioremap_wc == 0)
        printk("fscaler300_init_one [%d] NCNB\n", pdev->id);
    else
        printk("fscaler300_init_one [%d] NCB\n", pdev->id);

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

    if (use_ioremap_wc == 0)
        vaddr = (unsigned long)ioremap_nocache(mem->start, PAGE_ALIGN(mem->end - mem->start));
    else
        vaddr = (unsigned long)ioremap_wc(mem->start, PAGE_ALIGN(mem->end - mem->start));
    if (unlikely(!vaddr))
        panic("%s, no virtual address! \n", __func__);

	add_engine(pdev->id, vaddr, irq->start);

    return 0;
}

static int fscaler300_remove_one(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver fscaler300_driver = {
    .probe	= fscaler300_init_one,
    .remove	=  __devexit_p(fscaler300_remove_one),
	.driver = {
	        .name = "FSCALER300",
	        .owner = THIS_MODULE,
	    },
};

static void fscaler300_dump_reg(int dev)
{
    int i = 0;
    unsigned int address = 0, size = 0;

    address = priv.engine[dev].fscaler300_reg;   ////<<<<<<<< Update your register dump here >>>>>>>>>//issue
    size=0x1B0;  //<<<<<<<< Update your register dump here >>>>>>>>>
    for (i = 0; i < size; i = i + 0x10) {
        printk("0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address+i),*(unsigned int *)(address+i),
            *(unsigned int *)(address+i+4),*(unsigned int *)(address+i+8),*(unsigned int *)(address+i+0xc));
    }
    printk("\n");
    address += 0x5100;
    size = 0x10;
    printk("0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address),*(unsigned int *)(address),
        *(unsigned int *)(address+4),*(unsigned int *)(address+8),*(unsigned int *)(address+0xc));

    printk("\n");
    address += 0x100;
    size = 0x10;
    printk("0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address),*(unsigned int *)(address),
        *(unsigned int *)(address+4),*(unsigned int *)(address+8),*(unsigned int *)(address+0xc));
    printk("\n");
    address += 0x200;
    size = 0x10;
    printk("0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address),*(unsigned int *)(address),
        *(unsigned int *)(address+4),*(unsigned int *)(address+8),*(unsigned int *)(address+0xc));
    printk("\n");
    address += 0x1a8;
    size=0x40;  //<<<<<<<< Update your register dump here >>>>>>>>>
    for (i = 0; i < size; i = i + 0x10) {
        printk("0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address+i),*(unsigned int *)(address+i),
            *(unsigned int *)(address+i+4),*(unsigned int *)(address+i+8),*(unsigned int *)(address+i+0xc));
    }
    printk("\n");

    printk("link list memory content\n");
    address = priv.engine[dev].fscaler300_reg + 0x40000;
    size = SCALER300_LLI_MEMORY;
    for (i = 0; i < size; i = i + 0x10) {
        printk("0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address+i),*(unsigned int *)(address+i),
            *(unsigned int *)(address+i+4),*(unsigned int *)(address+i+8),*(unsigned int *)(address+i+0xc));
    }
}

static void fscaler300_dump_job_param(fscaler300_job_t *job)
{
    job_node_t *node;
    int ch_id = 0;
    fscaler300_property_t *property;
    fscaler300_job_t *parent;

    node = (job_node_t *)job->input_node[0].private;
    property = &node->property;
    ch_id = node->ch_id;
    parent = job->parent;
    printk("*******vg job id [%x] ch_id [%d]*******\n", node->job_id, ch_id);
    printk("src_format = %d\n", property->src_fmt);
    printk("dst_format = %d\n", property->dst_fmt);
    printk("src_width = %d\n", property->vproperty[ch_id].src_width);
    printk("src_height = %d\n", property->vproperty[ch_id].src_height);
    printk("src_bg_width = %d\n", property->src_bg_width);
    printk("src_bg_height = %d\n", property->src_bg_height);
    printk("src_x = %d\n", property->vproperty[ch_id].src_x);
    printk("src_y = %d\n", property->vproperty[ch_id].src_y);
    printk("dst_width = %d\n", property->vproperty[ch_id].dst_width);
    printk("dst_height = %d\n", property->vproperty[ch_id].dst_height);
    printk("dst_bg_width = %d\n", property->dst_bg_width);
    printk("dst_bg_height = %d\n", property->dst_bg_height);
    printk("dst_x = %d\n", property->vproperty[ch_id].dst_x);
    printk("dst_y = %d\n", property->vproperty[ch_id].dst_y);
    printk("scl_feature = %d\n", property->vproperty[ch_id].scl_feature);
    printk("src_interlace = %d\n", property->vproperty[ch_id].src_interlace);
    printk("di_result = %d\n", property->di_result);
    printk("in_addr_pa = %x\n", property->in_addr_pa);
    printk("out_addr_pa = %x\n", property->out_addr_pa);
    printk("in_addr_pa 1= %x\n", property->in_addr_pa_1);
    printk("out_addr_pa 1= %x\n", property->out_addr_pa_1);
    printk("aspect ratio %d\n", property->vproperty[ch_id].aspect_ratio.enable);
    printk("border %d\n", property->vproperty[ch_id].border.enable);

    printk("job id [%x]\n", job->job_id);
    printk("parent id %d\n", parent->job_id);
    printk("job type [%d]\n", job->job_type);
    printk("job cnt [%d]\n", job->job_cnt);
    printk("job link list offset [%x]\n", job->ll_blk.status_addr);
    printk("ratio [%d]\n", job->param.ratio);
    printk("src_fmt [%d]\n", job->param.src_format);
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
    printk("dst x_start[%d]\n", job->param.dst_x[0]);
    printk("dst y_start[%d]\n", job->param.dst_y[0]);
    printk("dst_x_end [%d]\n", job->param.tc[0].x_end);
    printk("dst_y_end [%d]\n", job->param.tc[0].y_end);
    printk("dst_width 1[%d]\n", job->param.tc[1].width);
    printk("dst_height 1[%d]\n", job->param.tc[1].height);
    printk("dst x_start 1[%d]\n", job->param.dst_x[1]);
    printk("dst y_start 1[%d]\n", job->param.dst_y[1]);
    printk("dst_x_end 1 [%d]\n", job->param.tc[1].x_end);
    printk("dst_y_end 1 [%d]\n", job->param.tc[1].y_end);
    printk("dst_bg_width [%d]\n", job->param.dst_bg_width);
    printk("dst_bg_height [%d]\n", job->param.dst_bg_height);
    printk("dst_bg_width 2[%d]\n", job->param.dst2_bg_width);
    printk("dst_bg_height 2[%d]\n", job->param.dst2_bg_height);
    printk("source dma [%x]\n", job->param.src_dma.addr);
    printk("source dma line offset [%d]\n", job->param.src_dma.line_offset);
    printk("dst dma [%x]\n", job->param.dest_dma[0].addr);
    printk("dst dma line offset [%d]\n", job->param.dest_dma[0].line_offset);
    printk("dst dma 1[%x]\n", job->param.dest_dma[1].addr);
    printk("dst dma 1 line offset [%d]\n", job->param.dest_dma[1].line_offset);
    printk("scl_feature [%d]\n", job->param.scl_feature);
    printk("need callback [%d]\n", job->need_callback);
    printk("perf type [%d]\n", job->perf_type);
    printk("ll_flow count %d res123 %d osd0 %d osd7 %d\n", job->ll_flow.count, job->ll_flow.res123, job->ll_flow.osd0123456, job->ll_flow.osd7);
    printk("priority %d\n", job->priority);
    printk("status %d\n", job->status);
}

static void fscaler300_lli_timeout(unsigned long data)
{
    int base = 0;
    unsigned long flags;
    fscaler300_job_t *job, *job1, *ne, *ne1, *parent;
    //fscaler300_job_t *curr, *last_job;
    //int schedule = 0;
    int dev = data;
    job_node_t *node, *node1, *node2, *node3;

    base = priv.engine[dev].fscaler300_reg;

    printk("SCALER300-%d timeout !!\n", dev);

    fscaler300_dump_reg(dev);

    /* disable channel */
    *(volatile unsigned int *)(base + 0x5408) = 0;

    spin_lock_irqsave(&priv.lock, flags);
    /* show each job parameter */
    list_for_each_entry_safe(job, ne, &priv.engine[dev].wlist, plist) {
        fscaler300_dump_job_param(job);
        list_for_each_entry_safe(job1, ne1, &job->clist, clist)
            fscaler300_dump_job_param(job1);
    }

    list_for_each_entry_safe(job, ne, &priv.engine[dev].qlist, plist) {
        parent = job->parent;
        scl_err("qlist root id %x type %d job cnt %d parnet %d status %d\n", job->job_id, job->job_type, job->job_cnt, parent->job_id, job->status);
        list_for_each_entry_safe(job1, ne1, &job->clist, clist)
            scl_err("child chain child id %x type %d job cnt %d status %d\n", job1->job_id, job1->job_type, job1->job_cnt, job1->status);
    }

    list_for_each_entry_safe(job, ne, &priv.engine[dev].slist, plist) {
        parent = job->parent;
        scl_err("slist root id %x type %d job cnt %d parnet %d status %d\n", job->job_id, job->job_type, job->job_cnt, parent->job_id, job->status);
        list_for_each_entry_safe(job1, ne1, &job->clist, clist)
            scl_err("child chain child id %x type %d job cnt %d status %d\n", job1->job_id, job1->job_type, job1->job_cnt, job1->status);
    }

    list_for_each_entry_safe(job, ne, &priv.engine[dev].wlist, plist) {
        parent = job->parent;
        scl_err("wlist root id %x type %d job cnt %d parnet %d status %d\n", job->job_id, job->job_type, job->job_cnt, parent->job_id, job->status);
        list_for_each_entry_safe(job1, ne1, &job->clist, clist)
            scl_err("child chain child id %x type %d job cnt %d status %d\n", job1->job_id, job1->job_type, job1->job_cnt, job1->status);
    }

    list_for_each_entry_safe(node, node1, &global_info.node_list, plist) {
        scl_err("node id %x, need callback %d status %d \n", node->job_id, node->need_callback, node->status);
        scl_err("src %d dst %d feature %d border %d aspect %d\n", node->property.src_fmt, node->property.dst_fmt, node->property.vproperty[node->ch_id].scl_feature, node->property.vproperty[node->ch_id].border.enable, node->property.vproperty[node->ch_id].aspect_ratio.enable);
        list_for_each_entry_safe(node2, node3, &node->clist, clist) {
            scl_err("node2 id %x, need callback %d status %d src %d dst %d feature %d\n", node2->job_id, node2->need_callback, node2->status, node2->property.src_fmt, node2->property.dst_fmt, node2->property.vproperty[node->ch_id].scl_feature);
            scl_err("src %d dst %d feature %d border %d aspect %d\n", node2->property.src_fmt, node2->property.dst_fmt, node2->property.vproperty[node->ch_id].scl_feature, node2->property.vproperty[node->ch_id].border.enable, node2->property.vproperty[node->ch_id].aspect_ratio.enable);
        }
    }


#if 0
    /* callback jobs' status "fail" in working list */
    list_for_each_entry_safe(job, ne, &priv.engine[dev].wlist, plist) {
        job->status = JOB_STS_FLUSH;
        if (job->job_cnt > 1)
            last_job = list_entry(job->clist.prev, fscaler300_job_t, clist);
        else
            last_job = job;

        fscaler300_job_free(dev, last_job, schedule);
    }

    /* callback jobs' status "fail" in sram list */
    list_for_each_entry_safe(job, ne, &priv.engine[dev].slist, plist) {
        job->status = JOB_STS_FLUSH;
        if (job->job_cnt > 1)
            last_job = list_entry(job->clist.prev, fscaler300_job_t, clist);
        else
            last_job = job;

        fscaler300_job_free(dev, last_job, schedule);
    }
#endif
    /* hardware reset */
    fscaler300_sw_reset(dev);

    UNLOCK_ENGINE(dev);
    mark_engine_finish(dev);

    spin_unlock_irqrestore(&priv.lock, flags);

    damnit(MODULE_NAME);

    /* wake up add_table workqueue */
    //fscaler300_put_job();
}

static int fscaler300_proc_init(void)
{
    /* create proc */
    entity_proc = create_proc_entry(SCALER_PROC_NAME, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (entity_proc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        return -EFAULT;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    entity_proc->owner = THIS_MODULE;
#endif
    /* fcs */
    fscaler300_fcs_proc_init(entity_proc);
    /* Denoise */
    fscaler300_dn_proc_init(entity_proc);
    /* Smooth */
    fscaler300_smooth_proc_init(entity_proc);
    /* Sharpness */
    fscaler300_sharp_proc_init(entity_proc);
    /* DMA */
    fscaler300_dma_proc_init(entity_proc);
    /* MEM */
    fscaler300_mem_proc_init(entity_proc);

    return 0;
}

void fscaler300_proc_remove(void)
{
    fscaler300_fcs_proc_remove(entity_proc);

    fscaler300_dn_proc_remove(entity_proc);

    fscaler300_smooth_proc_remove(entity_proc);

    fscaler300_sharp_proc_remove(entity_proc);

    fscaler300_dma_proc_remove(entity_proc);

    fscaler300_mem_proc_remove(entity_proc);

    if (entity_proc)
        remove_proc_entry(entity_proc->name, entity_proc->parent);
}
//#define DDR_IDX 0

int fscaler300_temp_buf_alloc(int id)
{
    int ret = 0;
    struct frammap_buf_info buf_info;
    char buf_name[20];

    if(priv.engine[id].temp_buf.vaddr)   ///< buffer have allocated
        goto exit;

    if (ddr_idx > 1)
        panic("scaler ddr index %d error\n", ddr_idx);

    memset(&buf_info, 0, sizeof(struct frammap_buf_info));

    temp_width = ALIGN_4_UP(temp_width);
    temp_height = ALIGN_2_UP(temp_height);

    buf_info.size = temp_width * temp_height * 2;
    buf_info.alloc_type = ALLOC_NONCACHABLE;
    sprintf(buf_name, "scl_temp.%d", id);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
    buf_info.name = buf_name;
#endif
    ret = frm_get_buf_ddr(ddr_idx, &buf_info);
    if(ret < 0) {
        scl_err("engine[%d] cvbs buffer allocate failed!\n", id);
        goto exit;
    }

    priv.engine[id].temp_buf.vaddr = (void *)buf_info.va_addr;
    priv.engine[id].temp_buf.paddr = buf_info.phy_addr;
    priv.engine[id].temp_buf.size  = buf_info.size;
    strcpy(priv.engine[id].temp_buf.name, buf_name);

    /* init buffer */
    memset(priv.engine[id].temp_buf.vaddr, 0, priv.engine[id].temp_buf.size);

exit:
    return ret;
}

void fscaler300_temp_buf_free(int id)
{
    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
        frm_free_buf_ddr(priv.engine[id].temp_buf.vaddr);
    #else
        struct frammap_buf_info buf_info;

        buf_info.va_addr  = (u32)priv.engine[id].temp_buf.vaddr;
        buf_info.phy_addr = priv.engine[id].temp_buf.paddr;
        buf_info.size     = priv.engine[id].temp_buf.size;
        frm_free_buf_ddr(&buf_info);
    #endif
}

int fscaler300_pip_buf_alloc(int id)
{
    int ret = 0;
    struct frammap_buf_info buf_info;
    char buf_name[20];

    if (priv.engine[id].pip1_buf.vaddr)   ///< buffer have allocated
        goto exit;

    if (ddr_idx > 1)
        panic("scaler ddr index %d error\n", ddr_idx);

    // PIP1 buffer
    memset(&buf_info, 0, sizeof(struct frammap_buf_info));

    pip1_width = ALIGN_4_UP(pip1_width);
    pip1_height = ALIGN_2_UP(pip1_height);

    buf_info.size = pip1_width * pip1_height * 2;
    buf_info.alloc_type = ALLOC_NONCACHABLE;
    sprintf(buf_name, "scl_pip1.%d", id);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
    buf_info.name = buf_name;
#endif
    ret = frm_get_buf_ddr(ddr_idx, &buf_info);
    if (ret < 0) {
        scl_err("engine[%d] pip1 buffer allocate failed!\n", id);
        goto exit;
    }

    priv.engine[id].pip1_buf.vaddr = (void *)buf_info.va_addr;
    priv.engine[id].pip1_buf.paddr = buf_info.phy_addr;
    priv.engine[id].pip1_buf.size  = buf_info.size;
    strcpy(priv.engine[id].pip1_buf.name, buf_name);

    /* init buffer */
    memset(priv.engine[id].pip1_buf.vaddr, 0, priv.engine[id].pip1_buf.size);

exit:
    return ret;
}

void fscaler300_pip_buf_free(int id)
{
    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
        frm_free_buf_ddr(priv.engine[id].pip1_buf.vaddr);
    #else
        struct frammap_buf_info buf_info;

        buf_info.va_addr  = (u32)priv.engine[id].pip1_buf.vaddr;
        buf_info.phy_addr = priv.engine[id].pip1_buf.paddr;
        buf_info.size     = priv.engine[id].pip1_buf.size;
        frm_free_buf_ddr(&buf_info);
    #endif
}
void *src_addr = 0, *dst_addr = 0;
#define DUMMY_SIZE  64*64*2
char *name = "scaler300";
static int __init fscaler300_drv_init(void)
{
#if CPUCOMM_SUPPORT
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
#endif
    int dev, ret = 0;
    int src_phy = 0, dst_phy = 0;
#if GM8210
    const char *devname[SCALER300_ENGINE_NUM] = {"fscaler300-0","fscaler300-1"};
#else
    const char *devname[SCALER300_ENGINE_NUM] = {"fscaler300-0"};
#endif
    printk("SCALER version %d, temp_width: %d, temp_height: %d \n", VERSION, temp_width, temp_height);

    /* global info */
    memset(&priv, 0x0, sizeof(fscaler300_priv_t));

    /* init each engine list head */
    for (dev = 0; dev < SCALER300_ENGINE_NUM ; dev++) {
        INIT_LIST_HEAD(&priv.engine[dev].qlist);
        INIT_LIST_HEAD(&priv.engine[dev].slist);
        INIT_LIST_HEAD(&priv.engine[dev].wlist);
    }

    /* assign max block number in link list memory */
    for (dev = 0; dev < SCALER300_ENGINE_NUM; dev++) {
        priv.engine[dev].blk_cnt = SCALER300_LLI_MAX_BLK;
#if GM8210
        priv.engine[dev].sram_table[3] = 0x80000000;    /* total 128, dummy job use sram block number 127, set as used */
#elif GM8287
        priv.engine[dev].sram_table[1] = 0xFFFFFF80;    /* total 40, dummy job use sram block number 40, set as used */
#else //GM8129
        priv.engine[dev].sram_table[0] = 0xFFFFE000;    /* total 13.75, dummy job use sram block number 14, set as used */
#endif
    }

    priv.global_param.chn_global = kzalloc(sizeof(fscaler300_chn_global_t)* max_minors, GFP_KERNEL);

    /* init global parameter */
    fscaler300_init_global_param(&priv.global_param);
#ifdef USE_WQ
    /* create workqueue */
    job_workq = create_singlethread_workqueue("scaler300");
    if (job_workq == NULL) {
        printk("FSCALER300: error in create workqueue! \n");
        return -1;
    }
#endif

#ifdef USE_KTHREAD
    init_waitqueue_head(&add_thread_wait);

	priv.add_task = kthread_create(add_thread, NULL, "scaler_add_table");
        if (IS_ERR(priv.add_task))
            panic("%s, create ep_task fail! \n", __func__);
        wake_up_process(priv.add_task);
#endif
    /* init the clock and add the device .... */
    platform_init();

    /* Register the driver */
	if (platform_driver_register(&fscaler300_driver)) {
		printk("Failed to register FSCALER300 driver\n");
		return -ENODEV;
	}
	/* hook irq */
	//irq 12,44
    for (dev = 0; dev < priv.eng_num; dev++) {
        if (request_irq(priv.engine[dev].irq, &fscaler300_interrupt, 0, devname[dev], (void *)dev) < 0)
            printk("Unable to allocate IRQ %d\n", priv.engine[dev].irq);
    }

	spin_lock_init(&priv.lock);

	/* init semaphore lock */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
    sema_init(&priv.sema_lock, 1);
#else
    init_MUTEX(&priv.sema_lock);
#endif

	/* create memory cache */
	priv.job_cache = kmem_cache_create(name, sizeof(fscaler300_job_t), 0, 0, NULL);
    if (priv.job_cache == NULL)
        panic("%s, no memory for job_cache! \n", __func__);

    /* init parameter memory to 0 & write global parameter to HW & set palette */
    for (dev = 0; dev < priv.eng_num; dev++) {
        ret = fscaler300_init_global(dev, &priv.global_param);
        if (ret < 0)
            printk("init scaler300 global register fail\n");
    }
#if CPUCOMM_SUPPORT
    fmem_get_identifier(&pci_id, &cpu_id);
    if ((pci_id == FMEM_PCI_HOST) && (cpu_id == FMEM_CPU_FA626)) {
        if (ftpmu010_get_attr(ATTR_TYPE_EPCNT) > 0) {
            /* open cpu comm */
            cpu_comm_channel = CHAN_0_USR8;
            cpu_comm_open(cpu_comm_channel, "SCL_RC");
        }
    } else if ((pci_id == FMEM_PCI_DEV0) && (cpu_id == FMEM_CPU_FA626)) {
        /* open cpu comm */
        cpu_comm_channel = CHAN_PCI0_1_USR8;
        cpu_comm_open(cpu_comm_channel, "SCL_EP");
    }
#endif
#ifdef VIDEOGRAPH_INC
    fscaler300_vg_init();
#endif
    /* get dummy job input/output buffer physical address */
    src_addr = kmalloc(DUMMY_SIZE, GFP_KERNEL);
    if (src_addr == NULL) {
        scl_err("alloc dummy src buffer fail\n");
        return -1;
    }
    dst_addr = kmalloc(DUMMY_SIZE, GFP_KERNEL);
    if (dst_addr == NULL) {
        scl_err("alloc dummy dst buffer fail\n");
        return -1;
    }
    src_phy = __pa(src_addr);
    dst_phy = __pa(dst_addr);

    /* get temporal buffer physical address */
    if (temp_width != 0 && temp_height != 0) {
        for (dev = 0; dev < SCALER300_ENGINE_NUM; dev++) {
            ret = fscaler300_temp_buf_alloc(dev);
            if (ret < 0) {
                scl_err("alloc temporal buffer fail\n");
                return -1;
            }
        }
    }

    /* get PIP1 frame temporal buffer physical address */
    if (pip1_width != 0 && pip1_height != 0) {
        for (dev = 0; dev < SCALER300_ENGINE_NUM; dev++)
            fscaler300_pip_buf_alloc(dev);
    }

    /* dummy job parameter init*/
    fscaler300_dummy_init(src_phy, dst_phy);
#if GM8210
    /* OSD Font setup */
    ret = fscaler300_osd_init();
    if (ret < 0)
        printk("init scaler300 OSD Font fail\n");
#endif
    /* init timer */
    for (dev = 0; dev < priv.eng_num; dev++) {
        init_timer(&priv.engine[dev].timer);
        priv.engine[dev].timer.function = fscaler300_lli_timeout;
        priv.engine[dev].timer.data = dev;
        priv.engine[dev].timeout = msecs_to_jiffies(SCL_LLI_TIMEOUT);
    }

    /* init proc */
    fscaler300_proc_init();

    return 0;
}

static void __exit fscaler300_drv_clearnup(void)
{
    int dev;

    /* cancel all works */
#ifdef USE_WQ
    cancel_delayed_work(&process_job_work);
#endif
#ifdef USE_KTHREAD
    if (priv.add_task)
        kthread_stop(priv.add_task);

    while (add_table == 1)
        msleep(10);
#endif
    fscaler300_proc_remove();

#ifdef VIDEOGRAPH_INC
    fscaler300_vg_driver_clearnup();
#endif

    /* destroy workqueue */
#ifdef USE_WQ
    destroy_workqueue(job_workq);
#endif
    platform_driver_unregister(&fscaler300_driver);

    /* delete timer */
    for (dev = 0; dev < priv.eng_num; dev++)
        del_timer(&priv.engine[dev].timer);

    kfree(priv.global_param.chn_global);
    /* destroy the memory cache */
    kmem_cache_destroy(priv.job_cache);

    kfree(src_addr);
    kfree(dst_addr);

    /* free temporal buffer */
    if (temp_width != 0 && temp_height != 0) {
        for (dev = 0; dev < SCALER300_ENGINE_NUM; dev++)
            fscaler300_temp_buf_free(dev);
    }

    /* free PIP frame temporal buffer */
    if (pip1_width != 0 && pip1_height != 0) {
        for (dev = 0; dev < SCALER300_ENGINE_NUM; dev++)
            fscaler300_pip_buf_free(dev);
    }

    platform_exit();

    for (dev = 0; dev < priv.eng_num; dev++) {
        free_irq(priv.engine[dev].irq, (void *)dev);
        __iounmap((void *)priv.engine[dev].fscaler300_reg);
    }
}

EXPORT_SYMBOL(fscaler300_put_job);

module_init(fscaler300_drv_init);
module_exit(fscaler300_drv_clearnup);
MODULE_LICENSE("GPL");
