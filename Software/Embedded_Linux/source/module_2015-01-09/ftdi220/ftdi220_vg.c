/* This file is only for GM8287
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/hardirq.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <mach/platform/platform_io.h>
#include "log.h"    //include log system "printm","damnit"...
#include "video_entity.h"   //include video entity manager
#include "common.h"
#include "ftdi220_vg.h"

int chan_threshold = 4;
unsigned int  debug_value = 0x80000000;
unsigned int  job_seqid[ENTITY_MINORS];

/* The case that in_buf = out_buf in frame mode, the driver must collect FRAME_COUNT_HYBRID_DI/DN frames
 * to fire the job. It is for both de-interlace and denoise.
 */
#define FRAME_COUNT_HYBRID_DI   4

module_param(chan_threshold, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(chan_threshold, "nr chan to do 3di");

extern int debug_mode;

#define is_parent(x)    ((x)->parent_node == (x))

/*
 * Main structure
 */
static g_info_t  g_info;

#define DRV_VGLOCK(x)   spin_lock_irqsave(&g_info.lock, x)
#define DRV_VGUNLOCK(x) spin_unlock_irqrestore(&g_info.lock, x)

/* scheduler */
static DECLARE_DELAYED_WORK(process_callback_work, 0);

/* driver entity declaration
*/
static struct video_entity_t driver_entity[MAX_CHAN_NUM];

/* proc system */
static struct proc_dir_entry *entity_proc, *infoproc, *cbproc, *utilproc;
static struct proc_dir_entry *propertyproc, *jobproc, *filterproc, *levelproc;
static unsigned int callback_period = 0;    //ticks

/* utilization */

util_t g_utilization[FTDI220_MAX_NUM];
/* property lastest record */
struct property_record_t *property_record;

/* log & debug message */
#define MAX_FILTER  5
unsigned int log_level = LOG_QUIET;
static int include_filter_idx[MAX_FILTER]; //init to -1, ENGINE << 16 | MINOR
static int exclude_filter_idx[MAX_FILTER]; //init to -1, ENGINE << 16 | MINOR

/* function declaration */
void ftdi220_callback_process(void);
static void ftdi220_joblist_add(int chan_id, job_node_t *node);
int ftdi220_get_drv_chnum_cnt(void);
static unsigned int ftdi220_joblist_cnt(int chan_id);

/* A map table used to be query
 */
enum property_id {
    /* when when function_auto = 1 and the incoming frame SRC_FMT is:
     *      TWO FRAMES: deinterlace + denoise
     *      FIELD / FRAME: denoise only
     *      TWO FRAMES + ALL progressive ==> COPY only
     * when function_auto = 0, it means 3di is disabled.
     */
    ID_FUNCTION_AUTO = MAX_WELL_KNOWN_ID_NUM +1    /* 1 for enabled, 0 for disabled. */
};

struct property_map_t property_map[] = {
    {ID_SRC_FMT, "src_fmt", "YUV422_FIELDS, YUV422_FRAME, YUV422_TWO_FRAMES"},
    {ID_SRC_DIM, "src_dim", "source (width << 16 | height)"},
    {ID_SRC_XY, "src_xy", "source (x << 16 | y)"},
    {ID_SRC_BG_DIM, "src_bg_dim", "the source background dimension, it is used for 2D purpose"},
    {ID_SRC_INTERLACE, "src_interlace", "1 for interlace, 0 for progressive"},
    {ID_FUNCTION_AUTO, "function_auto", "1 for auto check the working mode of 3di. 0 for all diabled"},
    {ID_DI_RESULT, "di_result", "the result to latter module"},
    {ID_DIDN_MODE, "didn_mode", "didn mode. 0x00: DiDn disabled, 0x03: Di enabled, Dn disabled, 0x0C: Dn enabled, Di disabled. 0x0F: both enabled"},
    {ID_NULL, ""}
};

int is_print(int engine,int minor)
{
    int i;

    if ( include_filter_idx[0] >= 0) {
        for (i = 0; i < MAX_FILTER; i++)
            if (include_filter_idx[i] == ((engine << 16) | minor))
                return 1;
    }

    if (exclude_filter_idx[0] >= 0) {
        for (i = 0; i < MAX_FILTER; i++)
            if (exclude_filter_idx[i] == ((engine << 16) | minor))
                return 0;
    }
    return 1;
}

int ftdi220_vg_panic_notifier(int data)
{
    return 0;
}

int ftdi220_vg_print_notifier(int data)
{
    int i, dev, chnum, engine, minor;
    unsigned long flags;
    struct video_entity_job_t *job = NULL;
    job_node_t  *vg_node = NULL;
    ftdi220_job_t *hw_job = NULL;

    printm("DI", " ------ ftdi220_vg_print_notifier, START --------- \n");

    DRV_VGLOCK(flags);
    for (i = 0; i < ENTITY_MINORS; i ++) {
        if (ftdi220_joblist_cnt(i)) {
            printm("DI", "minor: %-2d, job count = %d, ch_div = %d, skip = %d \n", i,
                    ftdi220_joblist_cnt(i), g_info.ref_node[i] ? g_info.ref_node[i]->nr_splits_backup : -1,
                    g_info.ref_node[i] ? g_info.ref_node[i]->nr_skip_chan : -1);
        }
    }

    printm("DI", "memory: minor_node_count %d\n", g_info.vg_node_count);
    printm("DI", "memory: alloc_node_count %d\n", g_info.alloc_node_count);
    printm("DI", "memory: drv_chum_used_count %d\n", ftdi220_get_drv_chnum_cnt());

    /* print nodes in channel list */
    for (chnum = 0; chnum < MAX_CHAN_NUM; chnum ++) {
		vg_node = g_info.ref_node[chnum];
		if (vg_node != NULL) {
        	job = (struct video_entity_job_t *)vg_node->private;

            engine = ENTITY_FD_ENGINE(job->fd);
            minor = ENTITY_FD_MINOR(job->fd);

	        printm("DI", "refer ftdi220 ch%d, status 0x%02X, eng:%d, jid: %d, split:%d\n", chnum,
	                        vg_node->status, vg_node->dev, job->id, vg_node->parent_node->nr_splits);
		}

		/* print nodes in channel queue list */
        list_for_each_entry(vg_node, &chan_job_list[chnum], list) {
            job = (struct video_entity_job_t *)vg_node->private;

            engine = ENTITY_FD_ENGINE(job->fd);
            minor = ENTITY_FD_MINOR(job->fd);

            printm("DI", "ftdi220 ch:%d, status:0x%02X, eng:%d, jid:%d, split:%d, refer_cnt:%d\n", chnum, vg_node->status,
                                vg_node->dev, job->id, vg_node->parent_node->nr_splits, atomic_read(&vg_node->parent_node->ref_cnt));
        }
    }


    for (dev = 0; dev < priv.eng_num; dev ++) {
        list_for_each_entry(hw_job, &priv.engine[dev].list, list) {
			vg_node = (job_node_t  *)hw_job->private;
			if (vg_node) {
	            printm("DI", "ftdi220 qlist, dev: %d, status: 0x%02X, split: %d, refer_cnt: %d\n", dev,
	                    vg_node->status, vg_node->parent_node->nr_splits, atomic_read(&vg_node->parent_node->ref_cnt));
            }
        }
    }

    DRV_VGUNLOCK(flags);

    printm("DI", " ------ ftdi220_vg_print_notifier, END --------- \n");

    return 0;
}

static void print_property(struct video_entity_job_t *job,struct video_property_t *property)
{
    int i;
    int engine = ENTITY_FD_ENGINE(job->fd);
    int minor = ENTITY_FD_MINOR(job->fd);

    for (i = 0; i < MAX_PROPERTYS; i++) {
        if (property[i].id == ID_NULL)
            break;
        if (i == 0)
            DEBUG_M(LOG_DEBUG, engine, minor,"{%d,%d} job %d property:", engine, minor, job->id);
        DEBUG_M(LOG_DEBUG, engine, minor,"  ID:%d, Value:%d\n",property[i].id, property[i].value);
    }
}

struct chnum_array {
    int minor;
    int ch;
} chnum_resource[MAX_CHAN_NUM];

/* calculate how many channels are used now
*/
int ftdi220_get_drv_chnum_cnt(void)
{
    unsigned long flags;
    int i, count = 0;

    /* save irq */
    DRV_VGLOCK(flags);

    for (i = 0; i < MAX_CHAN_NUM; i ++) {
        if (chnum_resource[i].minor != -1)
            count ++;
    }
    /* restore irq */
    DRV_VGUNLOCK(flags);

    return count;
}

/* Two indince: minor and vch */
int ftdi220_get_drv_chnum(int minor, int vch)
{
    unsigned long flags;
    int i, free_ch = -1, bfound = 0;

    /* save irq */
    DRV_VGLOCK(flags);

    for (i = 0; i < MAX_CHAN_NUM; i ++) {
        if ((free_ch == -1) && (chnum_resource[i].minor == -1))
            free_ch = i;

        if ((chnum_resource[i].minor == minor) && (chnum_resource[i].ch == vch)) {
            bfound = 1;
            break;
        }
    }

    /* restore irq */
    DRV_VGUNLOCK(flags);

    if (bfound)
        return i;

    if (free_ch == -1) {
        int i;

        for (i = 0; i < MAX_CHAN_NUM; i ++)
            printk("array[%d] = %d, %d \n", i, chnum_resource[i].minor, chnum_resource[i].ch);

        panic("%s, no drv chnum available! \n", __func__);
    }

    chnum_resource[free_ch].minor = minor;
    chnum_resource[free_ch].ch = vch;

    //printk("ftdi220_get_drv_chnum : %d, vch = %d ----------------> \n", free_ch, vch);

    return free_ch;
}

int ftdi220_find_drv_chnum(int minor, int vch)
{
    unsigned long flags;
    int i, bfound = 0;

    /* save irq */
    DRV_VGLOCK(flags);

    for (i = 0; i < MAX_CHAN_NUM; i ++) {
        if ((chnum_resource[i].minor == minor) && (chnum_resource[i].ch == vch)) {
            bfound = 1;
            break;
        }
    }

    /* restore irq */
    DRV_VGUNLOCK(flags);

    if (bfound)
        return i;

    return -1;
}

void ftdi220_del_drv_chnum(int minor, int vch)
{
    unsigned long flags;
    int i, bfound = 0;

    /* save irq */
    DRV_VGLOCK(flags);

    for (i = 0; i < MAX_CHAN_NUM; i ++) {
        if ((chnum_resource[i].minor == minor) && ((chnum_resource[i].ch == vch) || (vch == -1))) {
            chnum_resource[i].minor = chnum_resource[i].ch = -1;
            bfound = 1;
            break;
        }
    }

    /* restore irq */
    DRV_VGUNLOCK(flags);

    if (!bfound && (vch != -1))
        panic("%s, bug happen, can't find the drv chnum(%d, %d)! \n", __func__, minor, vch);
}

/* chan_param is driver level instead of vg level
 *
 * opt_write_newbuf indicates OPT_WRITE_NEWBUF
 *
 * New Action:
 * In frame mode, in_buf = out_buf is allowed. In this way, either interlace src or pure progressive src is possible.
 * We must support this two functions.
 */
void assign_chan_parameters(chan_param_t *chan_param /* DRV */, int func_auto, int src_fmt, int is_progressive, u32 bg_wh, int didn, u32 opt_write_newbuf)
{
    /* get background width and height */
    chan_param->bg_width = EM_PARAM_WIDTH(bg_wh);
    chan_param->bg_height = EM_PARAM_HEIGHT(bg_wh);
    chan_param->didn = didn;
    chan_param->is_progressive = is_progressive;

    if (src_fmt == TYPE_YUV422_FIELDS)
        chan_param->frame_type = FRAME_TYPE_2FIELDS;
    if (src_fmt == TYPE_YUV422_FRAME || src_fmt == TYPE_YUV422_FRAME_2FRAMES || src_fmt == TYPE_YUV422_RATIO_FRAME  || src_fmt == TYPE_YUV422_FRAME_FRAME)
        chan_param->frame_type = FRAME_TYPE_YUV422FRAME;
    if (src_fmt == TYPE_YUV422_2FRAMES || src_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        chan_param->frame_type = FRAME_TYPE_2FRAMES;

    if (src_fmt == -1)  /* default value due to videograph special case that only function_auto=0 has been set, others are empty. */
        chan_param->frame_type = FRAME_TYPE_YUV422FRAME;

    //do nothing for this skip channel
    if (chan_param->skip) {
        chan_param->command = (OPT_CMD_SKIP | OPT_CMD_DISABLED);
        return;
    }

    chan_param->command = opt_write_newbuf;

    if (func_auto == 1) {
        switch (src_fmt) {
          case TYPE_YUV422_FIELDS:
            chan_param->command |= (OPT_CMD_DN_TEMPORAL | OPT_CMD_DN_SPATIAL); /* denoise */
            break;

          case TYPE_YUV422_RATIO_FRAME:
          case TYPE_YUV422_FRAME:
          case TYPE_YUV422_FRAME_2FRAMES:
          case TYPE_YUV422_FRAME_FRAME:
            if (chan_param->didn & 0x0C) //denoise
                chan_param->command |= (OPT_CMD_DN_TEMPORAL | OPT_CMD_DN_SPATIAL); /* denoise on */

            if (is_progressive == 0) {
                /* interlace */
                if (chan_param->didn & 0x03)    /* deinterlace on */
                    chan_param->command |= OPT_CMD_DI;
                else
                    chan_param->command &= ~OPT_CMD_DI; /* deinterlace off */
            } else {
                /* progressive */
                if ((chan_param->command & OPT_CMD_DIDN) == 0)
                    chan_param->command |= OPT_CMD_FRAME_COPY;

                /* Avoid videograph bug, too many illeagal cases  */
                chan_param->command &= ~OPT_CMD_DI;
            }
            break;

          case TYPE_YUV422_2FRAMES:
          case TYPE_YUV422_2FRAMES_2FRAMES:
            if (is_progressive == 1) {
                chan_param->command |= OPT_CMD_OPP_COPY;

                if (chan_param->didn & 0x0C) //denoise, new.
                    chan_param->command |= (OPT_CMD_DN_TEMPORAL | OPT_CMD_DN_SPATIAL);

                /* Avoid videograph bug, too many illeagal cases  */
                chan_param->command &= ~OPT_CMD_DI;
            }
            else {
                if (chan_param->didn & 0x0C)
                    chan_param->command |= (OPT_CMD_DN_TEMPORAL | OPT_CMD_DN_SPATIAL);
                if (chan_param->didn & 0x03)
                    chan_param->command |= OPT_CMD_DI;
            }
            break;
          default:
            /* As Foster request, when src_fmt = -1, do nothing */
            chan_param->command = OPT_CMD_DISABLED;
            break;
        }
    } else {
        chan_param->command |= OPT_CMD_DISABLED;
        switch (src_fmt) {
          case TYPE_YUV422_FIELDS:
          case TYPE_YUV422_FRAME:
          case TYPE_YUV422_FRAME_2FRAMES:
          case TYPE_YUV422_FRAME_FRAME:
            /* if in_buf = out_buf, we just return this job back */
            if (chan_param->command & OPT_WRITE_NEWBUF)
                chan_param->command |= OPT_CMD_FRAME_COPY;
            break;
          default:
            break;
        }
    }
}

/* when when function_auto = 1 and the incoming frame SRC_FMT is:
 *  Interlace = 0:
 *      TWO FRAMES: COPY ONLY
 *      FRAME:  Denoise only
 *      FIELDS: Denoise only
 *  Interlace = 1:
 *      TWO FRAMES: Deinterlace + Denoise
 *      FRAME:  Deinterlace + Denoise
 *      FIELDS: Denoise only
 *
 * New function: DIDN (minor level) which is suitable for TWO_FRAME and FRAME mode
 *      if (all_progressive == 0) {     ==> it is interlace case
 *          if (DI == 0)  then not de-interlace
 *          if (DI == 1)  then de-interlace
 *          if (DN == 0)  then not denoise
 *          if (DN == 1)  then denoise
 *      } else {    ==> it is progressive case
 *          if (DN == 0)  then don't denoise
 *          if (DN == 1)  then denoise
 *          if (DI == 1)  then don't de-interlace. It is normal case stated from Foster.
 *      }
 *      default: 0x0F: di and dn enabled
 *      0x00: DIDN disabled
 *      0x03: DI enabled, DN disabled
 *      0x0C: DN enabled, DI disabled
 *      others: invalid
 *
 * when function_auto = 0, it means 3di is disabled.
 *
 * New Action 2014/7/31 03:22:
 * In frame mode, in_buf = out_buf is allowed. In this way, either interlace src or pure progressive src is possible.
 * We must support this two functions.
 */
static int driver_parse_in_property(job_node_t *parent_node, struct video_entity_job_t *job, int nr_splits, int nr_skip, int didn, int all_progressive)
{
    int minor_idx = MAKE_IDX(ENTITY_MINORS, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
    struct video_property_t *property = job->in_prop;
    ftdi220_param_t *param = (ftdi220_param_t *)&parent_node->param;
    chan_param_t  *chan_param;
    job_node_t    *node;
    int i, func_auto = -1, src_fmt = -1, first_ch = -1, drv_chnum;
    unsigned int bg_wh = 0, opt_write_newbuf = 0;
    unsigned int alloc_ch[(MAX_CHAN_NUM + 7) >> 3];
    int skip_chan = 0;

    memset(alloc_ch, 0, sizeof(alloc_ch));

    i = 0;
    /* fill up the input parameters */
    while (property[i].id != 0) {
        /* channel number */
        if (first_ch == -1) {
            first_ch = property[i].ch;
            parent_node->chnum = property[i].ch;
            parent_node->drv_chnum = ftdi220_get_drv_chnum(minor_idx, property[i].ch);  /* new */
            parent_node->parent_node = parent_node;
            INIT_LIST_HEAD(&parent_node->child_list);
            parent_node->minor_idx = minor_idx;
            parent_node->nr_splits_backup = nr_splits;
            parent_node->nr_skip_chan = nr_skip;
            set_bit(property[i].ch, (void *)alloc_ch);
            parent_node->nr_splits ++;

            if (debug_value == 0xFFFFFFFF)
                printm("DI", "1st, minor=%d, vch=%d, drv=%d, backup_split=%d \n", minor_idx, first_ch, parent_node->drv_chnum, parent_node->nr_splits_backup);
            else if (debug_value & (0x1 << minor_idx))
                printm("DI", "1st, minor=%d, vch=%d, drv=%d, backup_split=%d \n", minor_idx, first_ch, parent_node->drv_chnum, parent_node->nr_splits_backup);
        }

        /* Allocate all the child nodes.
         * Note: the parent node was allocated in upper function.
         * nr_splits and nr_skip is constant value in the function.
         */
        if ((nr_splits - nr_skip) <= chan_threshold) {
            if (!test_bit(property[i].ch, (void *)alloc_ch)) {
                job_node_t *node_item;

                node_item = (job_node_t *)kmem_cache_alloc(g_info.job_cache, GFP_KERNEL);
                if (node_item == NULL)
                    DI_PANIC("%s, no memory! \n", __func__);
                else
                    g_info.alloc_node_count ++;

                memset(node_item, 0, sizeof(job_node_t));
                node_item->chnum = property[i].ch;

                node_item->drv_chnum = ftdi220_get_drv_chnum(minor_idx, property[i].ch);    /* new a channel */
                node_item->parent_node = parent_node;
                INIT_LIST_HEAD(&node_item->child_list);
                node_item->minor_idx = minor_idx;
                list_add_tail(&node_item->child_list, &parent_node->child_list);
                parent_node->nr_splits ++;
                set_bit(property[i].ch, (void *)alloc_ch);

                if (debug_value == 0xFFFFFFFF)
                    printm("DI", "AllocDrvCh, minor=%d, vch=%d, drv=%d, nr_splits=%d \n", minor_idx, property[i].ch, node_item->drv_chnum, parent_node->nr_splits);
                else if (debug_value & (0x1 << minor_idx))
                    printm("DI","AllocDrvCh, minor=%d, vch=%d, drv=%d, nr_splits=%d \n", minor_idx, property[i].ch, node_item->drv_chnum, parent_node->nr_splits);
            }
        }

        /* If nr_splits > chan_threshold, only one drv channel is allocated. It is valid to goto loop */
        if ((drv_chnum = ftdi220_find_drv_chnum(minor_idx, property[i].ch)) < 0)
            goto loop;

        /*
         * From now on, we use driver layer channel number
         */
        chan_param = &g_info.chan_param[drv_chnum];

        switch(property[i].id) {
          case ID_SRC_BG_DIM:
            chan_param->in_pro[0].id = property[i].id;
            chan_param->in_pro[0].value = property[i].value;
            bg_wh = property[i].value;
            break;

          case ID_FUNCTION_AUTO: /* 1 for auto checking the working mode. */
            chan_param->in_pro[1].id = property[i].id;
            chan_param->in_pro[1].value = property[i].value;
            func_auto = property[i].value;
            break;

          case ID_SRC_INTERLACE: /* chx_interlace */
            if (property[i].value == 1) /* 0:prograssive/1:interlace */
                all_progressive = 0;
            chan_param->skip = 0;
            /* workaround, when only parent left and first vch is skip, the interlaced frame will be skipped. */
            if (nr_splits == -1) {
                /* new function */
                if (property[i].value == 2) {
                    chan_param->skip = 1;
                    skip_chan ++;
                }
            }
            /* update the in property value */
            chan_param->in_pro[2].id = ID_SRC_INTERLACE;
            chan_param->in_pro[2].value = all_progressive;
            break;

          case ID_SRC_FMT: /* chx_src_fmt, TYPE_YUV422_FIELDS, TYPE_YUV422_FRAME, TYPE_YUV422_2FRAMES */
            chan_param->in_pro[3].id = property[i].id;
            chan_param->in_pro[3].value = property[i].value;
            src_fmt = property[i].value;
            break;

          case ID_SRC_XY:   //per channel
            chan_param->in_pro[4].id = property[i].id;
            chan_param->in_pro[4].value = property[i].value;
            chan_param->offset_x = EM_PARAM_X(chan_param->in_pro[4].value);
            chan_param->offset_y = EM_PARAM_Y(chan_param->in_pro[4].value);
            break;

          case ID_SRC_DIM:  //per channel
            chan_param->in_pro[5].id = property[i].id;
            chan_param->in_pro[5].value = property[i].value;
            chan_param->ch_width = EM_PARAM_WIDTH(property[i].value);
            chan_param->ch_height = EM_PARAM_HEIGHT(property[i].value);
            break;

          case ID_DIDN_MODE:
            chan_param->in_pro[6].id = property[i].id;
            chan_param->in_pro[6].value = property[i].value;
            didn = property[i].value;
            if ((didn != 0x0F) && (didn != 0x00) && (didn != 0x03) && (didn != 0x0C)) {
                printk("3DI: Warning, ID_DIDN_MODE is invalid: 0x%x \n", didn);
                didn = 0x0F; //default value
            }
            break;

          default:
            break;
        }
loop:
        i ++;
    }

    /* keep the result */
    if (nr_splits == -1) {
        /* nr_splits_backup includes nr_skip_chan */
        parent_node->nr_splits_backup = parent_node->nr_splits;
        parent_node->nr_skip_chan = skip_chan;
    }

    if (debug_value == 0xFFFFFFFF)
        printm("DI", "While-End, minor=%d, nr_splits=%d, nr_splits_backup=%d, nr_skip=%d \n", minor_idx,
            parent_node->nr_splits, parent_node->nr_splits_backup, parent_node->nr_skip_chan);
    else if (debug_value & (0x1 << minor_idx))
        printk("While-End, minor=%d, nr_splits=%d, nr_splits_backup=%d, nr_skip=%d \n", minor_idx,
            parent_node->nr_splits, parent_node->nr_splits_backup, parent_node->nr_skip_chan);

    /*
     * step1: update parent node's parameters
     */
    param = (ftdi220_param_t *)&parent_node->param;
    chan_param = &g_info.chan_param[parent_node->drv_chnum];

    /* the result needs to be outputed to different buffer */
    if (job->in_buf.buf[0].addr_pa != job->out_buf.buf[0].addr_pa)
        opt_write_newbuf = OPT_WRITE_NEWBUF;

    /* assign per channel parameters */
    assign_chan_parameters(chan_param, func_auto, src_fmt, all_progressive, bg_wh, didn, opt_write_newbuf);

    if ((chan_param->frame_type == FRAME_TYPE_YUV422FRAME) && (chan_param->command & OPT_CMD_DI))
        parent_node->di_sharebuf = (opt_write_newbuf & OPT_WRITE_NEWBUF) ? 0 : 1;

    param->chan_id = parent_node->drv_chnum;   /* driver channel number */
    param->width = chan_param->bg_width;
    param->height = chan_param->bg_height;
    param->limit_width = chan_param->ch_width;
    param->limit_height = chan_param->ch_height;
    param->frame_type = chan_param->frame_type;
    param->command = chan_param->command;
    param->offset_x = chan_param->offset_x;
    param->offset_y = chan_param->offset_y;
    print_property(job, job->in_prop);

    /*
     * step2: update child nodes' parameters
     */
    list_for_each_entry(node, &parent_node->child_list, child_list) {
        /* follow parent setting */
        node->di_sharebuf = parent_node->di_sharebuf;
        /* driver layer param */
        param = (ftdi220_param_t *)&node->param;
        chan_param = &g_info.chan_param[node->drv_chnum];

        /* assign per channel parameters */
        assign_chan_parameters(chan_param, func_auto, src_fmt, all_progressive, bg_wh, didn, opt_write_newbuf);

        param->chan_id = node->drv_chnum;   /* driver channel number */
        param->width = chan_param->bg_width;
        param->height = chan_param->bg_height;
        param->limit_width = chan_param->ch_width;
        param->limit_height = chan_param->ch_height;
        param->frame_type = chan_param->frame_type;
        param->command = chan_param->command;
        param->offset_x = chan_param->offset_x;
        param->offset_y = chan_param->offset_y;

        print_property(job, job->in_prop);
    }

    /* do global, free ALL the childs. Only left the parent node.
     */
    if (parent_node->nr_splits_backup - parent_node->nr_skip_chan > chan_threshold) {
        job_node_t  *ne;

        parent_node->param.offset_x = 0;
        parent_node->param.offset_y = 0;
        parent_node->param.limit_width = parent_node->param.width;
        parent_node->param.limit_height = parent_node->param.height;

        /* free ALL the childs. In the next comming job, the following action is not done
         * because only parent exists and no childs.
         */
        list_for_each_entry_safe(node, ne, &parent_node->child_list, child_list) {
            /* Bug Fix:
             * The case, when the parent node is skip, whole frame will be skipped as well. But other
             * splits still need deinterlace/denoise.
             */
            if (g_info.chan_param[parent_node->drv_chnum].skip == 1) {
                if (g_info.chan_param[node->drv_chnum].skip == 0) {
                    g_info.chan_param[parent_node->drv_chnum].command = g_info.chan_param[node->drv_chnum].command;
                    g_info.chan_param[parent_node->drv_chnum].skip = 0;
                }
            }

            list_del_init(&node->child_list);
            ftdi220_del_drv_chnum(minor_idx, node->chnum);
            kmem_cache_free(g_info.job_cache, node);
            g_info.alloc_node_count --;

            parent_node->nr_splits --;

            if (debug_value == 0xFFFFFFFF)
                printm("DI","ChanDel, minor=%d, vch=%d, nr_splits=%d, nr_splits_backup=%d\n", minor_idx, node->chnum, parent_node->nr_splits, parent_node->nr_splits_backup);
            else if (debug_value & (0x1 << minor_idx))
                printm("DI", "ChanDel, minor=%d, vch=%d, nr_splits=%d, nr_splits_backup=%d\n", minor_idx, node->chnum, parent_node->nr_splits, parent_node->nr_splits_backup);
        }
        /* self check */
        if (parent_node->nr_splits != 1) {
            printk("%s, bug happen! nr_splits != 1 \n", __func__);
            damnit("DI");
        }
    } else {
        if (debug_value == 0xFFFFFFFF)
            printm("DI", "Final, no del. minor=%d, vch=%d, nr_splits=%d, nr_splits_backup=%d\n", minor_idx, node->chnum, parent_node->nr_splits, parent_node->nr_splits_backup);
        else if (debug_value & (0x1 << minor_idx))
            printm("DI", "Final, no del. minor=%d, vch=%d, nr_splits=%d, nr_splits_backup=%d\n", minor_idx, node->chnum, parent_node->nr_splits, parent_node->nr_splits_backup);
    }

    if (parent_node->nr_splits <= 0)
        printk("%s, bug in 3di.................. nr_splits = %d \n", __func__, parent_node->nr_splits);

    return 0;
}

/*
 * param: private data
 */
static int driver_set_out_property(void *param, struct video_entity_job_t *job)
{
    ftdi220_job_t   *job_item = (ftdi220_job_t *)param;
    u32 command = job_item->param.command;

    /* assign id */
    job->out_prop[0].id = ID_DI_RESULT;

    /* 0:do nothing  1:deinterlace  2:copy line, 3:denoise only, 4:frame copy */
    if ((command & OPT_CMD_DISABLED) && !(command & OPT_CMD_FRAME_COPY))
        job->out_prop[0].value = 0; /* do nothing */
    else if (command & OPT_CMD_OPP_COPY)
        job->out_prop[0].value = 2; /* copy line */
    else if (command & OPT_CMD_DI)
        job->out_prop[0].value = 1; /* deinterlace */
    else if (command & OPT_CMD_DN)
        job->out_prop[0].value = 3; /* denoise only */
    else if (command & OPT_CMD_FRAME_COPY)
        job->out_prop[0].value = 4; /* frame copy */
    else
        printk("%s, error command: 0x%x \n", __func__, command);

    if (debug_mode >= DEBUG_VG)
        printk("%s, jobid: %d, value = %d \n", __func__, job_item->jobid, job->out_prop[0].value);

    /* terminate */
    job->out_prop[1].id = ID_NULL;
    job->out_prop[1].value = 0;

    print_property(job, job->out_prop);
    return 1;
}

static void driver_mark_engine_start(ftdi220_job_t *job_item, void *private)
{
    unsigned int engine = job_item->dev;
    job_node_t  *node_item = (job_node_t *)private;
    struct video_entity_job_t   *job = (struct video_entity_job_t *)node_item->private;
    struct video_property_t     *property = job->in_prop;
    int idx = MAKE_IDX(ENTITY_MINORS, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
    int i = 0;

    if (!is_parent(node_item))
        return;


    if (debug_value == 0xFFFFFFFF)
        printm("DI", "engine_start dev%d, minor=%d, drvch=%d, vch=%d, command:0x%x, status: 0x%x, jid: %d \n",
            node_item->dev, node_item->minor_idx, node_item->drv_chnum, node_item->chnum, node_item->param.command,
            node_item->status, job->id);
    else if (debug_value & (0x1 << node_item->minor_idx))
        printm("DI", "engine_start dev%d, minor=%d, drvch=%d, vch=%d, command:0x%x, status: 0x%x, jid: %d \n",
            node_item->dev, node_item->minor_idx, node_item->drv_chnum, node_item->chnum, node_item->param.command,
            node_item->status, job->id);

    while (property[i].id != 0) {
        if (i < MAX_RECORD) {
            property_record[idx].job_id = job->id;
            property_record[idx].property[i].id = property[i].id;
            property_record[idx].property[i].value = property[i].value;
        } else {
            printk("Warning! FTDI220: The array is too small, array size is %d! \n", MAX_RECORD);
            break;
        }

        i ++;
    }

    if (engine > FTDI220_MAX_NUM)
        return;

    node_item->starttime = jiffies;
    ENGINE_START(engine) = node_item->starttime;
    ENGINE_END(engine) = 0;
    if(UTIL_START(engine) == 0)
	{
        UTIL_START(engine) = ENGINE_START(engine);
        ENGINE_TIME(engine) = 0;
    }
}

static void driver_mark_engine_finish(ftdi220_job_t *job_item, void *private)
{
    unsigned long flags;
    unsigned int engine = job_item->dev;
    job_node_t  *parent_node, *node_item = (job_node_t *)private;
    struct video_entity_job_t   *job = (struct video_entity_job_t *)node_item->private;

    if (engine > FTDI220_MAX_NUM) {
        panic("%s, error! dev=%d \n", __func__, engine);
        return;
    }

    /* if all child nodes are done, fall through utilization code */
    DRV_VGLOCK(flags);
    parent_node = node_item->parent_node;

    if (debug_value == 0xFFFFFFFF)
        printm("DI", "engine_finish dev%d, minor=%d, drvch=%d, vch=%d, command:0x%x, nr_splits:%d, status: 0x%x, jid: %d \n",
            node_item->dev, node_item->minor_idx, node_item->drv_chnum, node_item->chnum, node_item->param.command,
            parent_node->nr_splits, node_item->status, job->id);
    else if (debug_value & (0x1 << node_item->minor_idx))
        printm("DI","engine_finish dev%d, minor=%d, drvch=%d, vch=%d, command:0x%x, nr_splits:%d, status: 0x%x, jid: %d \n",
            node_item->dev, node_item->minor_idx, node_item->drv_chnum, node_item->chnum, node_item->param.command,
            parent_node->nr_splits, node_item->status, job->id);

    if (parent_node->nr_splits > 0)
        parent_node->nr_splits --;
    DRV_VGUNLOCK(flags);

    if (parent_node->nr_splits < 0)
        panic("FTDI220: %s bug, minor_idx: %d, value = %d \n", __func__, parent_node->minor_idx,
                                parent_node->nr_splits);

    if (parent_node->nr_splits)
        return;

    /* fall through if all child nodes are done */
    parent_node->finishtime = jiffies;

    /* not done by hardware such as flush job or drifing status */
    if (job_item->hardware == 0)
        return;

    ENGINE_END(engine) = parent_node->finishtime;

    if (ENGINE_END(engine) > ENGINE_START(engine))
        ENGINE_TIME(engine) += ENGINE_END(engine) - ENGINE_START(engine);

    if (UTIL_START(engine) > ENGINE_END(engine)) {
        UTIL_START(engine) = 0;
        ENGINE_TIME(engine) = 0;
    } else if ((UTIL_START(engine) <= ENGINE_END(engine)) &&
        (ENGINE_END(engine) - UTIL_START(engine) >= UTIL_PERIOD(engine) * HZ)) {
        unsigned int utilization;
		if ((ENGINE_END(engine) - UTIL_START(engine)) == 0)
		{
			panic("div by 0!!\n");
		}
		else
		{
        	utilization = (unsigned int)((100*ENGINE_TIME(engine)) / (ENGINE_END(engine) - UTIL_START(engine)));
		}
        if (utilization)
            UTIL_RECORD(engine) = utilization;
        UTIL_START(engine) = 0;
        ENGINE_TIME(engine) = 0;

    }
    ENGINE_START(engine)=0;
}

/* VideoGraph related function
 */
static int driver_preprocess(ftdi220_job_t *job_item, void *private)
{
    job_node_t  *node_item = (job_node_t *)private;

    if (job_item->dev != 0)
        return 0;

    if (!is_parent(node_item))
        return 0;

    return video_preprocess(node_item->private, NULL);
}

/* VideoGraph related function
 */
static int driver_postprocess(ftdi220_job_t *job_item, void *private)
{
    job_node_t  *node_item = (job_node_t *)private;

    if (job_item->dev != 0)
        return 0;

    /* if all child nodes are done, fall through the post_process */
    if (node_item->parent_node->nr_splits)
        return 0;

    return video_postprocess(node_item->private, NULL);
}

/* return the job to the videograph
 */
void ftdi220_callback_process(void)
{
    unsigned long flags;
    job_node_t  *node, *ne, *child_node;
    int         chnum;
    struct video_entity_job_t *job;

    for (chnum = 0; chnum < MAX_CHAN_NUM; chnum ++) {
        /* process this channel */
        do {
            DRV_VGLOCK(flags);

            if (list_empty(&chan_job_list[chnum])) {
                DRV_VGUNLOCK(flags);
                break;
            }

            /* take one from the list */
            node = list_entry(chan_job_list[chnum].next, job_node_t, list);

            /* If this job is process done or flushable, we need to return it back except it is referencing.
             * In this case, the reference bit will be removed after finishing the reference or stop.
             */
            if (!(node->status & JOB_DONE)) {
                DEBUG_M(LOG_DEBUG, node->dev, chnum, "3DI-MSG:NO JOB_DONE, job->id=%d,(CH=%d, status=%d) \n",
                            ((struct video_entity_job_t *)node->private)->id, chnum, node->status);
                DRV_VGUNLOCK(flags);
                break;
            }

            /* this job is still referenced */
            if (atomic_read(&node->ref_cnt) > 0) {
                DRV_VGUNLOCK(flags);
                break;
            }

            if (node->status & JOB_STS_QUEUE) {
                DEBUG_M(LOG_DEBUG, node->dev, chnum, "3DI-MSG:IN FIFO, job->id=%d,(CH=%d, status=%d) \n",
                                ((struct video_entity_job_t *)node->private)->id, chnum, node->status);
                DRV_VGUNLOCK(flags);
                break;
            }

            list_del_init(&node->list);
            DRV_VGUNLOCK(flags);


            DEBUG_M(LOG_DEBUG, node->dev, chnum, "%s, CB, job->id = %d \n", __func__,
                        ((struct video_entity_job_t *)node->private)->id);

            job = (struct video_entity_job_t *)node->private;

            if (node->first_node_return_fail == 1)
                job->status = JOB_STATUS_FAIL;
			else if (node->status & JOB_STS_DONE )
                job->status = JOB_STATUS_FINISH;
            else
                job->status = JOB_STATUS_FAIL;

            if (debug_mode >= DEBUG_VG)
                printk("%s \n", __func__);

            job->callback(job);

            DRV_VGLOCK(flags);
            list_for_each_entry_safe(child_node, ne, &node->child_list, child_list) {
                list_del_init(&child_node->child_list);
                kmem_cache_free(g_info.job_cache, child_node);
                g_info.alloc_node_count --;
            }
            DRV_VGUNLOCK(flags);

            if (node->nr_splits != 0)
                panic("%s, bug, nr_splits = %d, status: 0x%x, seqid: %d \n", __func__, node->nr_splits, node->status, node->job_seqid);

            kmem_cache_free(g_info.job_cache, node);
            g_info.alloc_node_count --;
			g_info.vg_node_count--;
        } while(1);
    } /* chnum */
}

/* callback function for job finish. In ISR.
 * We must save the last job for de-interlace reference.
 */
static void driver_callback(ftdi220_job_t *job_item, void *private)
{
    unsigned long flags;
    job_node_t    *parent_node, *node = (job_node_t *)private;
    struct video_entity_job_t   *job = (struct video_entity_job_t *)node->private;

    DRV_VGLOCK(flags);
    /* if all child nodes are done, fall through the orginal code */
    if (node->parent_node->nr_splits) {
        if (debug_value == 0xFFFFFFFF)
            printm("DI", "driver_callback_0: dev:%d, status: 0x%x, jid: %d \n", node->dev, node->status, job->id);
        DRV_VGUNLOCK(flags);
        return;
    }
    DRV_VGUNLOCK(flags);

    parent_node = node->parent_node;
    parent_node->status = job_item->status;
    parent_node->dev = job_item->dev;

    /* go through all childs to come out the exactly status */
    if (1) {
        job_node_t *child_node;
        u32 command = parent_node->param.command;

        DRV_VGLOCK(flags);
        list_for_each_entry(child_node, &parent_node->child_list, child_list)
            command |= child_node->param.command;
        DRV_VGUNLOCK(flags);

        /* give the result by priority. In order to meet driver_set_out_property() */
        if (parent_node->param.command != command) {
            if (command & OPT_CMD_DI) command = OPT_CMD_DI;
            if (command & OPT_CMD_OPP_COPY) command = OPT_CMD_OPP_COPY;
            if (command & OPT_CMD_FRAME_COPY) command = OPT_CMD_FRAME_COPY;
            if (command & OPT_CMD_DISABLED) command = OPT_CMD_DISABLED;

            parent_node->param.command = command;
        }
	}

    if (debug_value == 0xFFFFFFFF)
        printm("DI", "driver_callback, dev:%d, minor=%d, drvch=%d, vch=%d, command:0x%x, status: 0x%x, jid: %d, hw_done:%d\n",
            parent_node->dev, parent_node->minor_idx, parent_node->drv_chnum, parent_node->chnum,
            parent_node->param.command, parent_node->status, job->id, job_item->hardware);
    else if (debug_value & (0x1 << parent_node->minor_idx))
        printm("DI","driver_callback, dev:%d, minor=%d, drvch=%d, vch=%d, command:0x%x, status: 0x%x, jid: %d, hw_done: %d\n",
            parent_node->dev, parent_node->minor_idx, parent_node->drv_chnum, parent_node->chnum,
            parent_node->param.command, parent_node->status, job->id, job_item->hardware);

    driver_set_out_property(job_item, parent_node->private);

    /* decrease the reference count */
	DRV_VGLOCK(flags);
	if (parent_node->di_sharebuf == 1) {
	    /* The case that, the first three frames were not triggered by hardware. Thus we don't have
         * way to update their status.
         */
        if (parent_node->r_refer_to->status & JOB_STS_DRIFTING) {
            if (debug_value == 0xFFFFFFFF) {
                struct video_entity_job_t   *job2 = (struct video_entity_job_t *)parent_node->r_refer_to->private;

                printm("DI", "driver_callback(drifting): dev:%d, status: 0x%x, jid: %d(seqid:%d) \n", node->dev, node->status, job->id, parent_node->job_seqid);
                printm("DI", "decrease refer_node cnt, jid: %d(seqid:%d) \n", job2->id, parent_node->r_refer_to->job_seqid);
            }

            /* Important Notice: reset to zero.
             * The nr_splits of drifting job cannot relate to this job due to poential different version with different split_nr
             * Thus we cannot use split_nr of this job to decrease its split_nr of drifting job. It will be mismatch.
             */
            parent_node->r_refer_to->nr_splits = 0;
            driver_set_out_property(job_item, parent_node->r_refer_to->private);
            parent_node->r_refer_to->status = parent_node->status;
        }

        REFCNT_DEC(parent_node->r_refer_to);
        parent_node->r_refer_to = NULL;
	} else {
	     if (parent_node->refer_to)
            REFCNT_DEC(parent_node->refer_to);
	}

	DRV_VGUNLOCK(flags);

    /* schedule the delay workq */
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)ftdi220_callback_process);
    queue_delayed_work(g_info.workq, &process_callback_work,  callback_period);
}


/* callback functions called from the core
 */
struct f_ops callback_ops = {
    callback:           &driver_callback,
    pre_process:        &driver_preprocess,
    post_process:       &driver_postprocess,
    mark_engine_start:  &driver_mark_engine_start,
    mark_engine_finish: &driver_mark_engine_finish,
};

/*
 * Add a node to joblist of this minor
 */
static void ftdi220_joblist_add(int chan_id, job_node_t *node)
{
    unsigned long flags;

    INIT_LIST_HEAD(&node->list);

    DRV_VGLOCK(flags);
    list_add_tail(&node->list, &chan_job_list[chan_id]);
    DRV_VGUNLOCK(flags);
}

/* Get number of nodes in the list. chan_id is minor_idx
 */
static unsigned int ftdi220_joblist_cnt(int chan_id)
{
    unsigned long flags;
    unsigned int    count = 0;
    job_node_t      *node;

    /* save irq */
    DRV_VGLOCK(flags);

    if (!list_empty(&chan_job_list[chan_id])) {
        list_for_each_entry(node, &chan_job_list[chan_id], list)
            count ++;
    }

    /* restore irq */
    DRV_VGUNLOCK(flags);

    return count;
}

/*
 * This function assign the FF, FW, CR, NT ... buffer address for TWOFRAMES type
 */
static inline int assign_2frame_buffers(struct video_entity_job_t *prev,
                                        struct video_entity_job_t *cur, ftdi220_param_t *param)
{
    u32 command = param->command, line_ofs, frame_ofs;
    int meet = 0;

    if (param->frame_type != FRAME_TYPE_2FRAMES)
        return 0;

    if (debug_mode >= DEBUG_VG)  printk("%s \n", __func__);

    frame_ofs = param->width * param->height << 1;  //yuv422
    line_ofs = param->width << 1;    //yuv422

    /* Two Frames case, 60i -> 60p
     */
    if (prev) {
        param->ff_y_ptr = prev->in_buf.buf[0].addr_pa;  // previous top frame
        param->fw_y_ptr = prev->in_buf.buf[0].addr_pa + frame_ofs + line_ofs;  //previous bottom frame
        param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;   // current top frame
        param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + frame_ofs + line_ofs;   //current bottom frame
        meet = 1;
    } else {
        param->ff_y_ptr = cur->in_buf.buf[0].addr_pa;    // previous top frame
        param->fw_y_ptr = cur->in_buf.buf[0].addr_pa + frame_ofs + line_ofs;    //previous bottom frame
        param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;    // current top frame
        param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + frame_ofs + line_ofs;    //current bottom frame
        meet = 1;
    }

    param->di0_ptr = param->cr_y_ptr + line_ofs;
    param->di1_ptr = cur->in_buf.buf[0].addr_pa + frame_ofs;

    if (command & OPT_WRITE_NEWBUF) {
        /* bug in videograph */
        printk("3DI ERROR -> write to new buffer. %s, wrong command: 0x%x \n", __func__, command);
        meet = 0;
    }

    if (command & OPT_CMD_SKIP) {
        //just random assignment
        param->ff_y_ptr = cur->in_buf.buf[0].addr_pa;    // previous top frame
        param->fw_y_ptr = cur->in_buf.buf[0].addr_pa + frame_ofs + line_ofs;    //previous bottom frame
        param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;    // current top frame
        param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + frame_ofs + line_ofs;    //current bottom frame
        meet = 1;
    }

    return meet;
}

/*
 * This function assign the FF, FW, CR, NT ... buffer address for 2Fields type
 */
static inline int assign_2fields_buffers(struct video_entity_job_t *prev,
                                        struct video_entity_job_t *cur, ftdi220_param_t *param)
{
    u32 field_ofs, line_ofs, command = param->command;
    int meet = 0;

    if (param->frame_type != FRAME_TYPE_2FIELDS)
        return 0;

    if (debug_mode >= DEBUG_VG)  printk("%s \n", __func__);

    line_ofs = param->width << 1;    //yuv422
    field_ofs = param->width * param->height;

    /* two fields only supports denoise at most
     */
    if (command & OPT_CMD_DI) {
        printk("%s, wrong command: 0x%x \n", __func__, command);
        damnit("DI");
    }

    if (command & OPT_CMD_DN) {
        /*
         * only support denoise
         */
        if (prev) {
            if (command & OPT_WRITE_NEWBUF) {
                /* output to different buffer */
                param->ff_y_ptr = prev->out_buf.buf[0].addr_pa; // previous top field
                param->fw_y_ptr = prev->out_buf.buf[0].addr_pa + field_ofs; // previous bottom field
                param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;               // current top field
                param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + field_ofs;   // current bottom field
                param->crwb_ptr = cur->out_buf.buf[0].addr_pa;
                param->ntwb_ptr = cur->out_buf.buf[0].addr_pa + field_ofs;
                meet = 1;
            } else {
                param->ff_y_ptr = prev->in_buf.buf[0].addr_pa;              // previous top field
                param->fw_y_ptr = prev->in_buf.buf[0].addr_pa + field_ofs;  // previous bottom field
                param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;               // current top field
                param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + field_ofs;   // current bottom field
                meet = 1;
            }
        } else {
            param->ff_y_ptr = cur->in_buf.buf[0].addr_pa;               // previous top field
            param->fw_y_ptr = cur->in_buf.buf[0].addr_pa + field_ofs;   // previous bottom field
            param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;               // current top field
            param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + field_ofs;   // current bottom field
            if (command & OPT_WRITE_NEWBUF) {
                param->crwb_ptr = cur->out_buf.buf[0].addr_pa;
                param->ntwb_ptr = cur->out_buf.buf[0].addr_pa + field_ofs;
            }
            meet = 1;
        }
    }

    if (command & OPT_CMD_FRAME_COPY) {
        if (unlikely(meet)) {
            printk("3DI ERROR, %s, command: 0x%x \n", __func__, command);
            damnit("DI");
        }

        param->ff_y_ptr = cur->in_buf.buf[0].addr_pa;   //don't care
        param->fw_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;    //don't care
        param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;
        param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;
        param->di0_ptr = cur->out_buf.buf[0].addr_pa + line_ofs;
        param->di1_ptr = cur->out_buf.buf[0].addr_pa;
        meet = 1;
    }

    if (command & OPT_CMD_SKIP) {
        //random assignment
        param->ff_y_ptr = cur->in_buf.buf[0].addr_pa;               // previous top field
        param->fw_y_ptr = cur->in_buf.buf[0].addr_pa + field_ofs;   // previous bottom field
        param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;               // current top field
        param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + field_ofs;   // current bottom field
    }

    return meet;
}

/*
 * This function assign the FF, FW, CR, NT ... buffer address for interlace frame type
 */
static inline int assign_yuv422frame_buffers(struct video_entity_job_t *prev,
                                        struct video_entity_job_t *cur, ftdi220_param_t *param)
{
    job_node_t  *node;
    u32 line_ofs, command = param->command;
    int meet = 0;

    if (param->frame_type != FRAME_TYPE_YUV422FRAME)
        return 0;

    node = container_of(param, job_node_t, param);

    /* assign_yuv422frame_buffers_hybrid will be adpoted later */
    if (node->di_sharebuf == 1)
        return 1;

    if (debug_mode >= DEBUG_VG)  printk("%s \n", __func__);

    line_ofs = param->width << 1;    //yuv422

    if (prev) {
        if (command & OPT_CMD_DI) {
            /*
             * deinterlace
             */
            param->ff_y_ptr = prev->out_buf.buf[0].addr_pa; /* result buffer */
            param->fw_y_ptr = prev->in_buf.buf[0].addr_pa + line_ofs;   /* input buffer */
            param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;
            param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;
            param->di0_ptr = cur->out_buf.buf[0].addr_pa + line_ofs;
            if (command & OPT_WRITE_NEWBUF) {
                param->crwb_ptr = cur->out_buf.buf[0].addr_pa;
                param->ntwb_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;
            }
            meet = 1;
        } else if (command & OPT_CMD_DN) {
            /*
             * denoise only (progressive)
             */
            if (command & OPT_WRITE_NEWBUF) {
                /* output to different buffer */
                param->ff_y_ptr = prev->out_buf.buf[0].addr_pa; /* result buffer */
                param->fw_y_ptr = prev->out_buf.buf[0].addr_pa + line_ofs;   /* result buffer */
                param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;
                param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;
                param->crwb_ptr = cur->out_buf.buf[0].addr_pa;
                param->ntwb_ptr = cur->out_buf.buf[0].addr_pa + line_ofs;
            } else {
                /* output to current buffer */
                param->ff_y_ptr = prev->in_buf.buf[0].addr_pa;  /* previous buffer */
                param->fw_y_ptr = prev->in_buf.buf[0].addr_pa + line_ofs;   /* previous buffer */
                param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;
                param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;
            }
            meet = 1;
        }
    } else {
        /* this is first frame */
        if (command & OPT_CMD_DI) {
            param->ff_y_ptr = cur->in_buf.buf[0].addr_pa;               // previous top field
            param->fw_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;    // previous bottom field
            param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;               // current top field
            param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;    // current bottom field
            param->di0_ptr = cur->out_buf.buf[0].addr_pa + line_ofs;
            /* If deinterlace frame, new buffer is MUST due to chip limitation */
            if (command & OPT_WRITE_NEWBUF) {
                param->crwb_ptr = cur->out_buf.buf[0].addr_pa;
                param->ntwb_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;
            } else {
                printk("3DI ERROR, %s, DI without new buffer! Command: 0x%x \n", __func__, command);
                return 0;   /* no case meet */
            }

            meet = 1;
        } else if (command & OPT_CMD_DN) {
            /*
             * denoise only (progressive)
             */
            param->ff_y_ptr = cur->in_buf.buf[0].addr_pa;               // previous top field
            param->fw_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;    // previous bottom field
            param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;               // current top field
            param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;    // current bottom field
            if (command & OPT_WRITE_NEWBUF) {
                param->crwb_ptr = cur->out_buf.buf[0].addr_pa;
                param->ntwb_ptr = cur->out_buf.buf[0].addr_pa + line_ofs;
            }
            meet = 1;
        }
    }

    if (command & OPT_CMD_FRAME_COPY) {
        if (unlikely(meet)) {
            printk("3DI ERROR, %s, command: 0x%x \n", __func__, command);
            return 0;
        }

        param->ff_y_ptr = cur->in_buf.buf[0].addr_pa;   //don't care
        param->fw_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;    //don't care
        param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;
        param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;

        param->di0_ptr = cur->out_buf.buf[0].addr_pa + line_ofs;
        param->di1_ptr = cur->out_buf.buf[0].addr_pa;

        meet = 1;
    }

    if (command == OPT_CMD_DISABLED) {
        /* function_auto = 0 but with or without frame-copy case
         * The following assignment just wants to pass drv sanity check
         */
        param->ff_y_ptr = cur->in_buf.buf[0].addr_pa;   //don't care
        param->fw_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;    //don't care
        param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;
        param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;

        param->di0_ptr = cur->out_buf.buf[0].addr_pa + line_ofs;
        param->di1_ptr = cur->out_buf.buf[0].addr_pa;

        meet = 1;
    }

    if (command & OPT_CMD_SKIP) {
        //just random assign value
        param->ff_y_ptr = cur->in_buf.buf[0].addr_pa;   //don't care
        param->fw_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;    //don't care
        param->cr_y_ptr = cur->in_buf.buf[0].addr_pa;
        param->nt_y_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;

        param->di0_ptr = cur->out_buf.buf[0].addr_pa + line_ofs;
        param->di1_ptr = cur->out_buf.buf[0].addr_pa;

        meet = 1;
    }

    return meet;
}

/* only for frame mode. di_sharebuf means input buffer identical to output buffer
 */
static int assign_yuv422frame_buffers_hybrid(struct video_entity_job_t *r,
                                                    struct video_entity_job_t *n,
                                                    struct video_entity_job_t *p,
                                                    struct video_entity_job_t *c,
                                                    ftdi220_param_t *param)
{
    u32 line_ofs, command = param->command;

    if (param->frame_type != FRAME_TYPE_YUV422FRAME) {
        printk("%s, bug in 3di! command:0x%x, frame_type is not FRAME:%d \n", __func__, command, param->frame_type);
        return -1;
    }

    line_ofs = param->width << 1;    //yuv422

    if (!(command & OPT_CMD_DI)) {
        printk("%s, bug in 3di! command:0x%x \n", __func__, command);
        return -1;
    }
    if (command & OPT_WRITE_NEWBUF) {
        printk("%s, bug in 3di 2! command:0x%x \n", __func__, command);
        return -1;
    }

    if (!r || !n || !p || !c) {
        printk("3di error! %p %p %p %p \n", r, n, p, c);
        return -1;
    }

    param->ff_y_ptr = r->out_buf.buf[0].addr_pa; /* result buffer */
    param->fw_y_ptr = p->in_buf.buf[0].addr_pa + line_ofs;   /* input buffer */
    param->cr_y_ptr = c->in_buf.buf[0].addr_pa;
    param->nt_y_ptr = c->in_buf.buf[0].addr_pa + line_ofs;
    param->di0_ptr = n->out_buf.buf[0].addr_pa + line_ofs;
    param->crwb_ptr = n->out_buf.buf[0].addr_pa;
    param->ntwb_ptr = c->in_buf.buf[0].addr_pa + line_ofs;

    return 0;
}

/* return 0 for ok */
static inline int assign_drvcore_parameters(job_node_t *parent_node, job_node_t *node, job_node_t *ref_node_item,
                                    struct video_entity_job_t *cur_job, struct video_entity_job_t *ref_vjob)
{
    int ret, minor_idx;

    minor_idx = MAKE_IDX(ENTITY_MINORS, ENTITY_FD_ENGINE(cur_job->fd), ENTITY_FD_MINOR(cur_job->fd));

    ret = assign_2frame_buffers(ref_vjob, cur_job, &node->param);
    ret |= assign_2fields_buffers(ref_vjob, cur_job, &node->param);
    ret |= assign_yuv422frame_buffers(ref_vjob, cur_job, &node->param);

    node->private = cur_job;
    node->refer_to = ref_node_item;
    node->puttime = jiffies;
    node->starttime = 0;
    node->finishtime = 0;
    node->status = JOB_STS_QUEUE;

    /* New function, when function_auto=0 and without frame-copy, then we just return this job ASAP */
    if ((node->param.command & OPT_CMD_DISABLED) && ((node->param.command & OPT_CMD_FRAME_COPY) == 0))
        node->status |= JOB_STS_DONOTHING;

    if (node->param.command & OPT_CMD_SKIP)
        node->status |= JOB_STS_DONOTHING;

    if ((node->param.frame_type == FRAME_TYPE_YUV422FRAME) && (node->param.command & OPT_CMD_DI) && (ref_node_item == NULL))
		node->first_node_return_fail = 1;
	else
		node->first_node_return_fail = 0;

#if 0 /* bug fixed. With multiple sub-jobs, when some jobs are do-nothing, it may clear
       * g_info.ref_node[minor_idx] to NULL. This cause later job doesn't have ref_node and no way
       * to decrease count and no way to have refered job to be callbacked. The code is moved to
       * drv_putjob().
       */
    /* without job reference */
    if ((node->param.command & OPT_CMD_DISABLED) || (node->param.command & OPT_CMD_FRAME_COPY)) {
        g_info.ref_node[minor_idx] = NULL;
    } else {
        REFCNT_INC(node);   //add the reference count of this node
        g_info.ref_node[minor_idx] = parent_node;
    }
#endif

    if (is_parent(node)) {
        if (debug_value == 0xFFFFFFFF)
            printm("DI", "drvcore, minor=%d, command: 0x%x \n", minor_idx, node->param.command);
        else if (debug_value & (0x1 << minor_idx))
            printm("DI","drvcore, minor=%d, command: 0x%x \n", minor_idx, node->param.command);
    }

    if (unlikely(!ret)) {
        printk("%s, no case meet, frame type = 0x%x, command: 0x%x \n", __func__,
            parent_node->param.frame_type, parent_node->param.command);

        return -1;
    }

    return 0;
}

/* this function should be called in atomic context */
static inline void release_refered_node(job_node_t *ref_node)
{
    if (ref_node == NULL)
        return;

    /* status with JOB_STS_DRIFTING is not in drv-core, thus we must flush them here.
     */
    if (ref_node->status & JOB_STS_DRIFTING) {
        ref_node->status = JOB_STS_FLUSH;
        ref_node->nr_splits = 0;
        //printk("%s, the seqid: %d \n", __func__, ref_node->job_seqid);
    }

    REFCNT_DEC(ref_node);
}

/*
 * put JOB
 * We must save the last job for de-interlace reference.
 */
static int driver_putjob(void *parm)
{
    unsigned long flags;
    struct video_entity_job_t *cur_job = (struct video_entity_job_t *)parm;
    struct video_entity_job_t *ref_vjob = NULL;
    job_node_t  *node, *parent_node, *prev_node, *ref_node_item;
    int ret = 0, minor_idx, nr_splits = -1, nr_skips = 0, ver_change = 0, didn = 0xF, all_progressive = 1;
    u32 command = 0;

    if (debug_mode >= DEBUG_VG)
        printk("%s, in_phys=0x%x(va=0x%x), out_phys=0x%x(va=0x%x) \n", __func__,
                cur_job->in_buf.buf[0].addr_pa, cur_job->in_buf.buf[0].addr_va,
                cur_job->out_buf.buf[0].addr_pa, cur_job->out_buf.buf[0].addr_va);

    minor_idx = MAKE_IDX(ENTITY_MINORS, ENTITY_FD_ENGINE(cur_job->fd), ENTITY_FD_MINOR(cur_job->fd));

    if (minor_idx > ENTITY_MINORS) {
        DI_PANIC("Error to use %s engine %d, max is %d\n",MODULE_NAME, minor_idx, ENTITY_MINORS);
    }

    parent_node = (job_node_t *)kmem_cache_alloc(g_info.job_cache, GFP_KERNEL);
    if (parent_node == NULL) {
        DI_PANIC("%s, no memory! \n", __func__);
    }
    g_info.alloc_node_count ++;

    /* basic init the parent parent_node
     */
    memset(parent_node, 0, sizeof(job_node_t));

    g_info.vg_node_count ++;

    DRV_VGLOCK(flags);
    if (g_info.ref_node[minor_idx]) {
        chan_param_t *param = &g_info.chan_param[g_info.ref_node[minor_idx]->drv_chnum];

        nr_splits = g_info.ref_node[minor_idx]->nr_splits_backup;
        nr_skips = g_info.ref_node[minor_idx]->nr_skip_chan;
        ref_vjob = g_info.ref_node[minor_idx]->private;
        didn = param->didn;
        all_progressive = param->is_progressive;

        if (ref_vjob->ver != cur_job->ver) {
            g_info.ver_change_count ++;
            ver_change = 1;
            all_progressive = 1;
            didn = 0x0F;
            if (debug_value == 0xFFFFFFFF)
                printm("DI", " minor=%d, -------------> VER change! <----------------- \n", minor_idx);
            else if (debug_value & (0x1 << minor_idx))
                printm("DI", "minor=%d, -------------> VER change! <----------------- \n", minor_idx);

            nr_splits = -1; //need to re-count
            nr_skips = 0;
            prev_node = g_info.ref_node[minor_idx];
            //parent
            ftdi220_del_drv_chnum(minor_idx, prev_node->chnum);
            //childs
            list_for_each_entry(node, &prev_node->child_list, child_list)
                ftdi220_del_drv_chnum(minor_idx, node->chnum);

            //printk("<------ ver change -----> \n");
        }
    }
    DRV_VGUNLOCK(flags);

    if (debug_value == 0xFFFFFFFF)
        printm("DI", " putjob, jid: %d \n", cur_job->id);
    else if (debug_value & (0x1 << minor_idx))
        printm("DI", "putjob, jid: %d \n", cur_job->id);

    /*
     * parse the parameters and assign to param structure
     */
    ret = driver_parse_in_property(parent_node, cur_job, nr_splits, nr_skips, didn, all_progressive);
    if (ret < 0) {
        printm("DI", "%s, ftdi220: parse property error! \n", __func__);
        return JOB_STATUS_FAIL;
    }

    if (g_info.ref_node[minor_idx] && (g_info.ref_node[minor_idx]->param.frame_type == FRAME_TYPE_YUV422FRAME)) {
        if (g_info.ref_node[minor_idx]->param.command != parent_node->param.command) {
            job_seqid[minor_idx] = 0;

            /* N */
            release_refered_node(g_info.n_ref_node[minor_idx]);
            g_info.n_ref_node[minor_idx] = NULL;
            /* P */
            release_refered_node(g_info.p_ref_node[minor_idx]);
            g_info.p_ref_node[minor_idx] = NULL;
            /* C */
            release_refered_node(g_info.ref_node[minor_idx]);
            g_info.ref_node[minor_idx] = NULL;

            parent_node->job_seqid = 0;

            if (debug_value == 0xFFFFFFFF)
                printm("DI", "drop the reference \n");
            else if (debug_value & (0x1 << minor_idx))
                printm("DI", "drop the reference \n");

            //printk("---> drop the reference \n");
        }
    }

    DRV_VGLOCK(flags);
    ref_node_item = g_info.ref_node[minor_idx];
    if (ref_node_item)
        ref_vjob = (struct video_entity_job_t *)ref_node_item->private;

    parent_node->job_seqid = job_seqid[minor_idx];

    /* parent node */
    if (!assign_drvcore_parameters(parent_node, parent_node, ref_node_item, cur_job, ref_vjob)) {
        command = parent_node->param.command;
        /* child nodes */
        list_for_each_entry(node, &parent_node->child_list, child_list) {
            /* update the share buffer and seqid */
            node->di_sharebuf = parent_node->di_sharebuf;
            node->job_seqid = parent_node->job_seqid;
            assign_drvcore_parameters(parent_node, node, ref_node_item, cur_job, ref_vjob);
            command |= node->param.command;
        }
#if 1  /* With multiple sub-jobs, when some jobs are do-nothing, it may clear g_info.ref_node[minor_idx]
        * to NULL. This cause later job doesn't have ref_node and no way to decrease count and no
        * way to have refered job to be callbacked. So the code is moved from assign_drvcore_parameters()
        */
        if (command & OPT_CMD_DIDN) {
            REFCNT_INC(parent_node);   //add the reference count of this node
            g_info.ref_node[minor_idx] = parent_node;
        } else {
            if (g_info.ref_node[minor_idx] != NULL)
                REFCNT_DEC(g_info.ref_node[minor_idx]);

            g_info.ref_node[minor_idx] = NULL;
            job_seqid[minor_idx] = 0;   //bug fix while switching between deinterlace and no-action
        }
#endif
    } else {
        DRV_VGUNLOCK(flags);
        printm("DI", "%s, ftdi220: assign_drvcore_parameters error! \n", __func__);
        return JOB_STATUS_FAIL;
    }

    /* Add to per minor link list */
    ftdi220_joblist_add(minor_idx, parent_node);

    /* Important Notice:
     * When the job is OPT_CMD_DISABLED, the job may be very fast to reach ftdi220_callback_process(),
     * in which the child_list may be walked through and list_del. It will case the child_list corruption.
     * Thus we add protection for the child_list.
     */
#if 1 /* New, it must be interlaced FRAME_TYPE_YUV422FRAME. In this case, g_info.ref_node[minor_idx]
       * is only used to keep the last information of split, skip ... when the version is not changed.
       */
    if (parent_node->di_sharebuf == 1) {
        job_node_t  *r = NULL, *n = NULL, *p = NULL, *c = NULL;
        job_node_t  *child_node;
        int ret;

        c = parent_node;
        switch (parent_node->job_seqid) {
          case 0:
            /* one node in chan list only and this node should be return fail due to no reference node */
            parent_node->first_node_return_fail = 1;
            break;
          case 1:
            /* two node in chan list only */
            p = list_entry(c->list.prev, job_node_t, list);
            break;
          case 2:
            p = list_entry(c->list.prev, job_node_t, list);
            n = list_entry(p->list.prev, job_node_t, list);
            break;
          default:
            p = list_entry(c->list.prev, job_node_t, list);
            n = list_entry(p->list.prev, job_node_t, list);
            r = list_entry(n->list.prev, job_node_t, list);
            break;
        }
        g_info.r_ref_node[minor_idx] = r;
        g_info.n_ref_node[minor_idx] = n;
        g_info.p_ref_node[minor_idx] = p;

        parent_node->r_refer_to = r;
        if (parent_node->job_seqid < (FRAME_COUNT_HYBRID_DI - 1)) {
            parent_node->status |= JOB_STS_DRIFTING;    //Not in driver core.

            if (debug_value == 0xFFFFFFFF)
                printm("DI", "parent_node->job_seqid: %d exit... \n", parent_node->job_seqid);
            else if (debug_value & (0x1 << minor_idx))
                printm("DI", "parent_node->job_seqid: %d exit... \n", parent_node->job_seqid);

            goto exit;
        }

        if (debug_value == 0xFFFFFFFF)
            printm("DI", "parent_node->job_seqid: %d queue... \n", parent_node->job_seqid);
        else if (debug_value & (0x1 << minor_idx))
            printm("DI", "parent_node->job_seqid: %d queue... \n", parent_node->job_seqid);

        ret = assign_yuv422frame_buffers_hybrid((struct video_entity_job_t *)r->private,
                                                (struct video_entity_job_t *)n->private,
                                                (struct video_entity_job_t *)p->private,
                                                (struct video_entity_job_t *)c->private,
                                                &parent_node->param);
        if (ret < 0) {
            DRV_VGUNLOCK(flags);
            return -1;
        }
        /* child */
        list_for_each_entry(child_node, &parent_node->child_list, child_list) {
            assign_yuv422frame_buffers_hybrid((struct video_entity_job_t *)r->private,
                                              (struct video_entity_job_t *)n->private,
                                              (struct video_entity_job_t *)p->private,
                                              (struct video_entity_job_t *)c->private,
                                              &child_node->param);
        }
    }
#endif

    /* put parent node to job queue now
     */
    ret = ftdi220_put_job(parent_node->drv_chnum, &parent_node->param, &callback_ops, parent_node, parent_node->status);
    if (!ret) {
        /* put child nodes to job queue now
         */
        list_for_each_entry(node, &parent_node->child_list, child_list) {
            ret = ftdi220_put_job(node->drv_chnum, &node->param, &callback_ops, node, node->status);
            if (ret)
                DI_PANIC("%s, put job fail! \n", __func__);
        }
    } else {
        DI_PANIC("%s, put job fail! \n", __func__);
    }

exit:
    DRV_VGUNLOCK(flags);

    if (parent_node->di_sharebuf == 1) {
        job_seqid[minor_idx] = job_seqid[minor_idx] > 0x0FFFFFFF ?
                            FRAME_COUNT_HYBRID_DI : job_seqid[minor_idx] + 1;
    }

    return JOB_STATUS_ONGOING;
}


/* Stop a channel
 */
static int driver_stop(void *parm, int engine, int minor)
{
    unsigned long flags;
    int minor_idx = MAKE_IDX(ENTITY_MINORS, engine, minor);
    job_node_t  *parent_node, *node;

    //printk("------ %s -------- \n", __func__);

    if (debug_mode >= DEBUG_VG)
        printk("%s \n", __func__);

    if (debug_value == 0xFFFFFFFF)
        printm("DI", "STOP0, minor=%d \n", minor_idx);
    else if (debug_value & (0x1 << minor_idx))
        printm("DI","STOP0, minor=%d \n", minor_idx);

    g_info.stop_count ++;

    DRV_VGLOCK(flags);
    job_seqid[minor_idx] = 0;

    if (g_info.ref_node[minor_idx]) {
        job_node_t  *node_item = g_info.ref_node[minor_idx];
        struct video_entity_job_t *job = (struct video_entity_job_t *)node_item->private;

        /* Note: g_info.r_ref_node[minor_idx] will follow the drv core path to process. We can't
         * handle it here. g_info.r_ref_node[minor_idx] maybe return to videograph already.
         */

        /* R
         * ==> just follow the orginal path in drv-core
         */
        /* N */
        release_refered_node(g_info.n_ref_node[minor_idx]);
        g_info.n_ref_node[minor_idx] = NULL;
        /* P */
        release_refered_node(g_info.p_ref_node[minor_idx]);
        g_info.p_ref_node[minor_idx] = NULL;
        /* C */
        parent_node = g_info.ref_node[minor_idx];
        release_refered_node(g_info.ref_node[minor_idx]);
        g_info.ref_node[minor_idx] = NULL;

        /* decrease the reference count of this node */
        release_refered_node(g_info.ref_node[minor_idx]);
        g_info.ref_node[minor_idx] = NULL;

        /* debug only */
        if (debug_value == 0xFFFFFFFF)
            printm("DI", "STOP1, minor=%d, nr_splits=%d, backup_split=%d, drvcnt=%d \n", minor_idx,
                    parent_node->nr_splits, parent_node->nr_splits_backup, ftdi220_get_drv_chnum_cnt());
        else if (debug_value & (0x1 << minor_idx))
            printm("DI", "STOP1, minor=%d, nr_splits=%d, backup_split=%d, drvcnt=%d \n", minor_idx,
                        parent_node->nr_splits, parent_node->nr_splits_backup, ftdi220_get_drv_chnum_cnt());

        /* stop */
        ftdi220_stop_channel(parent_node->drv_chnum);
        ftdi220_del_drv_chnum(minor_idx, parent_node->chnum);

        /* child */
        list_for_each_entry(node, &parent_node->child_list, child_list) {
            ftdi220_stop_channel(node->drv_chnum);
            /* free drv channel number */
            ftdi220_del_drv_chnum(minor_idx, node->chnum);
        }

        if (debug_value == 0xFFFFFFFF)
            printm("DI", "STOP2, dev%d, minor=%d, nr_splits=%d, backup_split=%d, drvcnt=%d, status:0x%x, jid:%d, ref_cnt:%d \n",
                        node_item->dev, minor_idx,
                        parent_node->nr_splits, parent_node->nr_splits_backup, ftdi220_get_drv_chnum_cnt(),
                        node_item->status, job->id, node_item->ref_cnt);
        else if (debug_value & (0x1 << minor_idx))
            printm("DI", "STOP2, dev%d, minor=%d, nr_splits=%d, backup_split=%d, drvcnt=%d, status:0x%x, jid:%d, ref_cnt:%d \n",
                        node_item->dev, minor_idx,
                        parent_node->nr_splits, parent_node->nr_splits_backup, ftdi220_get_drv_chnum_cnt(),
                        node_item->status, job->id, node_item->ref_cnt);
    } else {
        /* The case that the job doesn't have the reference such as frame copy. Thus we don't have
         * any way to release the driver channel.
         */
        ftdi220_del_drv_chnum(minor_idx, -1);
    }

    DRV_VGUNLOCK(flags);

    /* schedule the delay workq */
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)ftdi220_callback_process);
    queue_delayed_work(g_info.workq, &process_callback_work, callback_period);

    if (in_interrupt() || in_irq() || irqs_disabled())
        printk("%s, EXIT !!!!!!!!!!!!! \n", __func__);

    return 0;
}

static struct property_map_t *driver_get_propertymap(int id)
{
    int i;

    for(i=0;i<sizeof(property_map)/sizeof(struct property_map_t);i++) {
        if(property_map[i].id==id) {
            return &property_map[i];
        }
    }
    return 0;
}


static int driver_queryid(void *parm,char *str)
{
    int i;

    for(i=0;i<sizeof(property_map)/sizeof(struct property_map_t);i++) {
        if(strcmp(property_map[i].name,str)==0) {
            return property_map[i].id;
        }
    }
    printk("driver_queryid: Error to find name %s\n",str);

    return -1;
}

/* pass the id and return the string
 */
static int driver_querystr(void *parm,int id,char *string)
{
    int i;

    for(i=0;i<sizeof(property_map)/sizeof(struct property_map_t);i++) {
        if(property_map[i].id==id) {
            strcpy(string, property_map[i].name);

            return 0;
        }
    }
    printk("driver_querystr: Error to find id %d\n",id);
    return -1;
}


void print_info(void)
{
	//len += sprintf(page + len, "mem inc cnt %d\n", g_mem_access);
    return;
}

static int proc_info_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    print_info();
    return count;
}


static int proc_info_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    print_info();
    *eof = 1;
    return 0;
}


static int proc_cb_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int mode_set=0;
    sscanf(buffer, "%d",&mode_set);
    callback_period=mode_set;
    printk("\nCallback Period =%d (ticks)\n",callback_period);
    return count;
}


static int proc_cb_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    unsigned int len = 0;
    len += sprintf(page + len, "\nCallback Period =%d (ticks)\n",callback_period);
    *eof = 1;
    return len;
}


static int proc_util_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
	int i=0;
    int mode_set=0;
    sscanf(buffer, "%d", &mode_set);
    for(i=0;i<FTDI220_MAX_NUM;i++)
	{
    	UTIL_PERIOD(i) = mode_set;
    	printk("\nUtilization Period[%d] =%d(sec)\n", i, UTIL_PERIOD(i));
	}
    return count;
}


static int proc_util_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
	int i, len = 0;

    for(i=0;i<FTDI220_MAX_NUM;i++)
	{
        len += sprintf(page + len, "\nEngine%d HW Utilization Period=%d(sec) Utilization=%d\n",
            i,UTIL_PERIOD(i),UTIL_RECORD(i));
    }

    *eof = 1;
    return len;
}


static unsigned int property_engine=0,property_minor=0;
static int proc_property_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int mode_engine=0,mode_minor=0;

    sscanf(buffer, "%d %d",&mode_engine,&mode_minor);
    property_engine=mode_engine;
    property_minor=mode_minor;

    printk("\nLookup engine=%d minor=%d\n",property_engine,property_minor);
    return count;
}


static int proc_property_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    unsigned long flags;
    int i=0;
    struct property_map_t *map;
    unsigned int id,value,ch;
    int idx = MAKE_IDX(ENTITY_MINORS, property_engine, property_minor);
    unsigned int len = 0;

    DRV_VGLOCK(flags);

    len += sprintf(page + len, "\n%s engine:%d minor:%d job:%d\n",MODULE_NAME,
        property_engine,property_minor,property_record[idx].job_id);
    len += sprintf(page + len, "=============================================================\n");
    len += sprintf(page + len, "CH  ID  Name(string) Value(hex)  Readme\n");
    do {
        id=property_record[idx].property[i].id;
        if(id==ID_NULL)
            break;
        ch=property_record[idx].property[i].ch;
        value=property_record[idx].property[i].value;
        map=driver_get_propertymap(id);
        if(map) {
            len += sprintf(page + len, "%02d  %03d  %15s  %08x  %s\n",
                ch,id,map->name,value,map->readme);
        }
        i++;
    } while(1);

    DRV_VGUNLOCK(flags);

    *eof = 1;
    return len;
}

static int proc_job_read_mode(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    unsigned long flags;
    struct video_entity_job_t *job;
    job_node_t  *node;
    int chnum;
    char *st_string[] = {"QUEUE", "ONGOING", " FINISH", "FLUSH"}, *string = NULL;
    unsigned int len = 0;

    len += sprintf(page + len, "\nSystem ticks=0x%x\n", (int)jiffies & 0xffff);

    len += sprintf(page + len, "Chnum  Job_ID     Status     Puttime      start    end \n");
    len += sprintf(page + len, "===========================================================\n");

    for (chnum = 0; chnum < MAX_CHAN_NUM; chnum ++) {
		DRV_VGLOCK(flags);
        if (list_empty(&chan_job_list[chnum]))
        {
        	DRV_VGUNLOCK(flags);
            continue;
        }

        list_for_each_entry(node, &chan_job_list[chnum], list) {
            if (node->status & JOB_STS_QUEUE)   string = st_string[0];
            if (node->status & JOB_STS_ONGOING) string = st_string[1];
            if (node->status & JOB_STS_DONE)    string = st_string[2];
            if (node->status & JOB_STS_FLUSH)   string = st_string[3];
            job = (struct video_entity_job_t *)node->private;
            len += sprintf(page + len, "%-5d  %-9d  %-9s  0x%04x  0x%04x  0x%04x \n", chnum,
                job->id, string, node->puttime, node->starttime, node->finishtime);
        }
    	DRV_VGUNLOCK(flags);
    }

    *eof = 1;
    return len;
}

void print_filter(void)
{
    int i;
    printk("\nUsage:\n#echo [0:exclude/1:include] Engine Minor > filter\n");
    printk("Driver log Include:");
    for(i=0;i<MAX_FILTER;i++)
        if(include_filter_idx[i]>=0)
            printk("{%d,%d},",IDX_ENGINE(include_filter_idx[i],ENTITY_MINORS),
                IDX_MINOR(include_filter_idx[i],ENTITY_MINORS));

    printk("\nDriver log Exclude:");
    for(i=0;i<MAX_FILTER;i++)
        if(exclude_filter_idx[i]>=0)
            printk("{%d,%d},",IDX_ENGINE(exclude_filter_idx[i],ENTITY_MINORS),
                IDX_MINOR(exclude_filter_idx[i],ENTITY_MINORS));
    printk("\n");
}

//echo [0:exclude/1:include] Engine Minor > filter
static int proc_filter_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int i;
    int engine,minor,mode;
    sscanf(buffer, "%d %d %d",&mode,&engine,&minor);

    if(mode==0) { //exclude
        for(i=0;i<MAX_FILTER;i++) {
            if(exclude_filter_idx[i]==-1) {
                exclude_filter_idx[i]=(engine<<16)|(minor);
                break;
            }
        }
    } else if(mode==1) {
        for(i=0;i<MAX_FILTER;i++) {
            if(include_filter_idx[i]==-1) {
                include_filter_idx[i]=(engine<<16)|(minor);
                break;
            }
        }
    }
    print_filter();
    return count;
}


static int proc_filter_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    print_filter();
    *eof = 1;
    return 0;
}


static int proc_level_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int level;

    sscanf(buffer,"%d",&level);
    log_level=level;
    printk("\nLog level =%d (1:emerge 1:error 2:debug)\n",log_level);
    return count;
}


static int proc_level_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    printk("\nLog level =%d (1:emerge 1:error 2:debug)\n",log_level);
    *eof = 1;
    return 0;
}

struct video_entity_ops_t driver_ops = {
    putjob:      &driver_putjob,
    stop:        &driver_stop,
    queryid:     &driver_queryid,
    querystr:    &driver_querystr,
};

struct video_entity_t ftdi220_entity = {
    type:       TYPE_3DI,
    name:       "3DI",
    engines:    ENTITY_ENGINES,
    minors:     ENTITY_MINORS,
    ops:        &driver_ops
};

/* Print debug message
 */
void ftdi220_vg_print(char *page, int *len, void *data)
{
    int i, dev, chnum, engine, minor;
    unsigned long flags;
    struct video_entity_job_t *job = NULL;
    job_node_t  *vg_node = NULL;
    ftdi220_job_t *hw_job = NULL;
    int len2 = *len, ref_seqid;
    char *str, *path_str[] = {"normal", "di_sharebuf"};

    DRV_VGLOCK(flags);
    for (i = 0; i < ENTITY_MINORS; i ++) {
        if (ftdi220_joblist_cnt(i))
            len2 += sprintf(page + len2, "minor: %-2d, job count = %d, ch_div = %d, skip = %d \n", i,
                    ftdi220_joblist_cnt(i), g_info.ref_node[i] ? g_info.ref_node[i]->nr_splits_backup : -1,
                    g_info.ref_node[i] ? g_info.ref_node[i]->nr_skip_chan : -1);
    }


    /* print nodes in channel list */
    for (chnum = 0; chnum < MAX_CHAN_NUM; chnum ++) {
		vg_node = g_info.ref_node[chnum];
		if (vg_node != NULL) {
        	job = (struct video_entity_job_t *)vg_node->private;

            engine = ENTITY_FD_ENGINE(job->fd);
            minor = ENTITY_FD_MINOR(job->fd);

            str = (vg_node->di_sharebuf == 1) ? path_str[1] : path_str[0];
	        len2 += sprintf(page + len2, "Refer: ftdi220 ch%d, status 0x%02X, eng:%d, jid: %d, split:%d, %s, seqid:%d\n", chnum,
	                        vg_node->status, vg_node->dev, job->id, vg_node->parent_node->nr_splits, str, vg_node->job_seqid);
		}

		/* print nodes in channel queue list */
        list_for_each_entry(vg_node, &chan_job_list[chnum], list) {
            job = (struct video_entity_job_t *)vg_node->private;

            engine = ENTITY_FD_ENGINE(job->fd);
            minor = ENTITY_FD_MINOR(job->fd);

            str = (vg_node->di_sharebuf == 1) ? path_str[1] : path_str[0];
            /* when you see -1 which means the refer job was returned to videograph already. */
            ref_seqid = vg_node->r_refer_to ? vg_node->r_refer_to->job_seqid : -1;
            len2 += sprintf(page + len2, "ftdi220 ch:%d, status:0x%02X, eng:%d, jid:%d, split:%d, refer_cnt:%d, %s, seqid:%d, ref_seqid:%d, command:0x%x\n",
                    chnum, vg_node->status, vg_node->dev, job->id, vg_node->parent_node->nr_splits,
                    atomic_read(&vg_node->parent_node->ref_cnt), str, vg_node->job_seqid, ref_seqid, g_info.chan_param[vg_node->drv_chnum].command);
        }
    }

    for (dev = 0; dev < priv.eng_num; dev ++) {
		u32 i, base = priv.engine[dev].ftdi220_reg;
		/* print cur job in engine */
		hw_job =  &priv.engine[dev].cur_job;

		if (hw_job->private) {
    		vg_node = (job_node_t  *)hw_job->private;

            len2 += sprintf(page + len2, "ftdi220 engine: %d, hw_job status: 0x%02x, split: %d, refer_cnt: %d\n", dev,
                            hw_job->status, vg_node->parent_node->nr_splits, atomic_read(&vg_node->parent_node->ref_cnt));

            len2 += sprintf(page + len2, "register dump: \n");
            for (i = 0x0; i <= 0x30; i += 0x4) {
                if (!(i % 0x10))
                    printm("DI", "\n");
                len2 += sprintf(page + len2, "reg0x%x=0x%x, ", i, ioread32(base + i));
            }
            len2 += sprintf(page + len2, "reg0x6c=0x%x, reg0x70=0x%x, reg0x80=0x%x \n", ioread32(base + 0x6c),
                                                    ioread32(base + 0x70), ioread32(base + 0x80));
        }

        list_for_each_entry(hw_job, &priv.engine[dev].list, list) {
			vg_node = (job_node_t  *)hw_job->private;
	        len2 += sprintf(page + len2, "ftdi220 qlist, dev: %d, status: 0x%02X, split: %d, refer_cnt: %d\n", dev,
	                    vg_node->status, vg_node->parent_node->nr_splits, atomic_read(&vg_node->parent_node->ref_cnt));
        }
    }

    len2 += sprintf(page + len2, "memory: minor_node_count %d\n", g_info.vg_node_count);
    len2 += sprintf(page + len2, "memory: alloc_node_count %d\n", g_info.alloc_node_count);
    len2 += sprintf(page + len2, "memory: drv_chum_used_count %d\n", ftdi220_get_drv_chnum_cnt());
    len2 += sprintf(page + len2, "vg ver change count: %d\n", g_info.ver_change_count);
    len2 += sprintf(page + len2, "vg stop count: %d\n", g_info.stop_count);
    len2 += sprintf(page + len2, "debug value: 0x%x \n", debug_value);

    DRV_VGUNLOCK(flags);

    *len = len2;
}

/*
 * Entry point of video graph
 */
int ftdi220_vg_init(void)
{
    int    i;
    char   wname[30];

    memset(&g_info, 0, sizeof(g_info));
    memset(&job_seqid[0], 0, sizeof(job_seqid));

    /* register log system */
    register_panic_notifier(ftdi220_vg_panic_notifier);
    register_printout_notifier(ftdi220_vg_print_notifier);

    for (i = 0; i < MAX_CHAN_NUM; i ++)
        chnum_resource[i].minor = chnum_resource[i].ch = -1;

    /* register video entify */
    video_entity_register(&ftdi220_entity);

    /* function array registers to video graph */
    memset(&driver_entity[0], 0, MAX_CHAN_NUM * sizeof(struct video_entity_t));

    /* spinlock */
    spin_lock_init(&g_info.lock);
    /* semaphore for child_list protection */
    sema_init(&g_info.sema, 1);

    /* init list head of each channel */
    for (i = 0; i < MAX_CHAN_NUM; i ++)
        INIT_LIST_HEAD(&chan_job_list[i]);

    /* workqueue */
    sprintf(wname,"process_%s", DRIVER_NAME);
    g_info.workq = create_workqueue(wname);
    if (g_info.workq == NULL) {
        printk("FTDI220: error in create workqueue! \n");
        return -1;
    }

    /* create memory cache */
    g_info.job_cache = kmem_cache_create("di220_vg", sizeof(job_node_t), 0, 0, NULL);
    if (g_info.job_cache == NULL) {
        DI_PANIC("ftdi220: fail to create cache!");
    }

    entity_proc = create_proc_entry("videograph/ftdi220", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if(entity_proc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        return -EFAULT;
    }

    property_record = kzalloc(sizeof(struct property_record_t) * ENTITY_ENGINES * ENTITY_MINORS, GFP_KERNEL);
    if (property_record == NULL) {
        DI_PANIC("%s, no memory! \n", __func__);
    }

    infoproc = create_proc_entry("info", S_IRUGO | S_IXUGO, entity_proc);
    if(infoproc==NULL)
        return -EFAULT;
    infoproc->read_proc = (read_proc_t *)proc_info_read_mode;
    infoproc->write_proc = (write_proc_t *)proc_info_write_mode;

    cbproc = create_proc_entry("callback_period", S_IRUGO | S_IXUGO, entity_proc);
    if(cbproc == NULL)
        return -EFAULT;
    cbproc->read_proc = (read_proc_t *)proc_cb_read_mode;
    cbproc->write_proc = (write_proc_t *)proc_cb_write_mode;

    utilproc = create_proc_entry("utilization", S_IRUGO | S_IXUGO, entity_proc);
    if(utilproc == NULL)
        return -EFAULT;
    utilproc->read_proc = (read_proc_t *)proc_util_read_mode;
    utilproc->write_proc = (write_proc_t *)proc_util_write_mode;

    propertyproc = create_proc_entry("property", S_IRUGO | S_IXUGO, entity_proc);
    if(propertyproc == NULL)
        return -EFAULT;
    propertyproc->read_proc = (read_proc_t *)proc_property_read_mode;
    propertyproc->write_proc = (write_proc_t *)proc_property_write_mode;

    jobproc = create_proc_entry("job", S_IRUGO | S_IXUGO, entity_proc);
    if(jobproc==NULL)
        return -EFAULT;
    jobproc->read_proc = (read_proc_t *)proc_job_read_mode;

    filterproc = create_proc_entry("filter", S_IRUGO | S_IXUGO, entity_proc);
    if(filterproc == NULL)
        return -EFAULT;
    filterproc->read_proc = (read_proc_t *)proc_filter_read_mode;
    filterproc->write_proc = (write_proc_t *)proc_filter_write_mode;

    levelproc = create_proc_entry("level", S_IRUGO | S_IXUGO, entity_proc);
    if(levelproc == NULL)
        return -EFAULT;
    levelproc->read_proc = (read_proc_t *)proc_level_read_mode;
    levelproc->write_proc = (write_proc_t *)proc_level_write_mode;

    ftdi220_set_dbgfn(ftdi220_vg_print);

    memset(g_utilization, 0, sizeof(util_t)*FTDI220_MAX_NUM);

	for(i = 0; i < FTDI220_MAX_NUM; i++)
	{
		UTIL_PERIOD(i) = 5;
	}

    for(i = 0; i < MAX_FILTER; i++) {
        include_filter_idx[i] = -1;
        exclude_filter_idx[i] = -1;
    }

    printk("\nFTDI220 registers %d entities to video graph. \n", MAX_CHAN_NUM);

    return 0;
}

/*
 * Exit point of video graph
 */
void ftdi220_vg_driver_clearnup(void)
{
    char name[30];

    if (infoproc != 0)
        remove_proc_entry(infoproc->name, entity_proc);
    if (cbproc != 0)
        remove_proc_entry(cbproc->name, entity_proc);
    if (utilproc != 0)
        remove_proc_entry(utilproc->name, entity_proc);
    if (propertyproc != 0)
        remove_proc_entry(propertyproc->name, entity_proc);
    if (jobproc != 0)
        remove_proc_entry(jobproc->name, entity_proc);
    if (filterproc != 0)
        remove_proc_entry(filterproc->name, entity_proc);
    if (levelproc != 0)
        remove_proc_entry(levelproc->name, entity_proc);

    if (entity_proc != 0) {
        memset(&name[0], 0, sizeof(name));
        sprintf(name,"videograph/ftdi220");
        remove_proc_entry(name, 0);
    }

    video_entity_deregister(&ftdi220_entity);

    // remove workqueue
    destroy_workqueue(g_info.workq);
    kfree(property_record);
    kmem_cache_destroy(g_info.job_cache);
}
