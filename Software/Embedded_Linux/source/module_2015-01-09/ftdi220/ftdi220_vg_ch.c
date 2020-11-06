#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/hardirq.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <asm/io.h>
#include <mach/platform/platform_io.h>
#include "log.h"    //include log system "printm","damnit"...
#include "video_entity.h"   //include video entity manager
#include "common.h"
#include "ftdi220_vg.h"

unsigned int  debug_value = 0x0;
unsigned int  job_seqid[ENTITY_MINORS];

#ifdef USE_KTHREAD
static int ftdi220_cb_thread(void *data);
static void ftdi220_cb_wakeup(void);
static struct task_struct *cb_thread = NULL;
static wait_queue_head_t cb_thread_wait;
static int cb_wakeup_event = 0;
static volatile int cb_thread_ready = 0, cb_thread_reset = 0;
#endif /* USE_KTHREAD */

/* The case that in_buf = out_buf in frame mode, the driver must collect FRAME_COUNT_HYBRID_DI/DN frames
 * to fire the job. It is for both de-interlace and denoise.
 */
#define FRAME_COUNT_HYBRID_DI   4

extern int debug_mode;
#define is_parent(x)    ((x)->parent_node == (x))

typedef struct {
    u16 drv_chnum;
    u16 minor;
    int vch;    /* -1 means unused */
    struct list_head list;
} drv_chnum_t;

/* a database describe the driver layer channel number */
static drv_chnum_t  drv_chnum_db[MAX_CHAN_NUM];

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
static struct video_entity_t driver_entity[ENTITY_MINORS];

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
    {ID_SPEICIAL_FUNC, "feature", "bit0: 60p"},
    {ID_SOURCE_FIELD, "src field", "0 for top, 1 for bottom"},
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
    int i, chnum, engine, minor;
    unsigned long flags;
    struct video_entity_job_t *job = NULL;
    job_node_t  *parent_node = NULL, *child_node, *node;
    char *str, *path_str[] = {"normal", "di_sharebuf"};

    printm("DI", "------ %s START ------ \n", __func__);

    DRV_VGLOCK(flags);
    for (i = 0; i < priv.eng_num; i ++)
        printm("DI", "engine:%d, busy: %d \n", i, ENGINE_BUSY(i));

    for (i = 0; i < ENTITY_MINORS; i ++) {
        if (!ftdi220_joblist_cnt(i))
            continue;

        printm("DI", "minor: %-2d, job count: %d \n", i, ftdi220_joblist_cnt(i));
        list_for_each_entry(node, &chan_job_list[i], list) {
            job = (struct video_entity_job_t *)node->private;
            str = (node->di_sharebuf == 1) ? path_str[1] : path_str[0];
            printm("DI", "  ===>drvch:%d, status:0x%x, jid:%d, split:%d(%d), seqid:%d, cmd:0x%x, ftype:%d, refer_cnt:%d, %s, feature:0x%x, src_field:%d \n",
                    node->drv_chnum, node->status, job->id, node->nr_splits, node->nr_splits_keep,
                    node->job_seqid, node->chan_param.command, node->chan_param.frame_type, atomic_read(&node->ref_cnt),
                    str, node->chan_param.feature, node->chan_param.src_field);
        }
    }

    /* print nodes in channel list */
    for (chnum = 0; chnum < ENTITY_MINORS; chnum ++) {
        parent_node = g_info.ref_node[chnum];
        if (parent_node == NULL)
            continue;

        job = (struct video_entity_job_t *)parent_node->private;
        engine = ENTITY_FD_ENGINE(job->fd);
        minor = ENTITY_FD_MINOR(job->fd);
        str = (parent_node->di_sharebuf == 1) ? path_str[1] : path_str[0];
        printm("DI", "refer node: drvch:%d, status:0x%x, eng:%d, minor:%d, jid:%d, split:%d(%d), seqid:%d, cmd:0x%x, c_cmd:0x%x, ftype:%d, refer_cnt:%d, %s\n",
                parent_node->drv_chnum, parent_node->status, parent_node->dev, minor, job->id, parent_node->nr_splits,
                parent_node->nr_splits_keep, parent_node->job_seqid, parent_node->chan_param.command, parent_node->collect_command,
                parent_node->chan_param.frame_type, atomic_read(&parent_node->ref_cnt), str);
        /* child */
        list_for_each_entry(child_node, &parent_node->child_list, child_list) {
            printm("DI", "  --->child: drvch:%d, status:0x%x, cmd:0x%x \n",
                child_node->drv_chnum, child_node->status, child_node->chan_param.command);
        }
    }

    printm("DI", "memory: minor_node_count: %d\n", atomic_read(&g_info.vg_node_count));
    printm("DI", "memory: alloc_node_count: %d\n", atomic_read(&g_info.alloc_node_count));
    printm("DI", "version change: %d(vg:%d)\n", atomic_read(&g_info.ver_chg_count), atomic_read(&g_info.vg_chg_count));
    printm("DI", "vg stop count: %d\n", atomic_read(&g_info.stop_count));
    printm("DI", "hybrid job count: %d \n", atomic_read(&g_info.hybrid_job_count));
    printm("DI", "debug value: 0x%x \n", debug_value);

    DRV_VGUNLOCK(flags);

    printm("DI", "------ %s END ------ \n", __func__);

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

/* Get a driver layer channel number.
 * This function should be executed under protection
 */
unsigned short get_drv_chnum(int minor, int vch)
{
    drv_chnum_t *drv_chnum_node;
    int i, bFound = -1;

    if (minor >= ENTITY_MINORS)
        panic("%s, invalid minor: %d, out of range: %d! \n", __func__, minor, ENTITY_MINORS);

    list_for_each_entry(drv_chnum_node, &drv_chnum_list[minor], list) {
        if ((drv_chnum_node->minor != (u16)minor) || (drv_chnum_node->vch != (u16)vch))
            continue;
        bFound = drv_chnum_node->drv_chnum;
        break;
    }

    if (bFound != -1)
        return bFound;

    /* create a new */
    for (i = 0; i < MAX_CHAN_NUM; i ++) {
        if (drv_chnum_db[i].vch != -1)
            continue;
        drv_chnum_node = &drv_chnum_db[i];

        drv_chnum_node->minor = (u16)minor;
        drv_chnum_node->vch = vch;
        INIT_LIST_HEAD(&drv_chnum_node->list);
        list_add_tail(&drv_chnum_node->list, &drv_chnum_list[minor]);
        bFound = drv_chnum_node->drv_chnum;
        break;
    }

    if (bFound == -1) {
        for (i = 0; i < MAX_CHAN_NUM; i ++)
            printk("3DI: [%d], minor: %d, vch: %d, drvnum: %d \n", i, drv_chnum_db[i].minor,
                                            drv_chnum_db[i].vch, drv_chnum_db[i].drv_chnum);
        panic("Driver channel is not enough! Please redefine MAX_CHAN_NUM(%d)!\n", MAX_CHAN_NUM);
    }
    return bFound;
}

/* how many free driver channels */
int get_free_drvchum(void)
{
    int i, free_cnt = 0;

    for (i = 0; i < MAX_CHAN_NUM; i ++) {
        if (drv_chnum_db[i].vch != -1)
            continue;
        free_cnt ++;
    }

    return free_cnt;
}

/* Free driver layer channels by minor.
 * This function should be executed under protection */
void free_minor_drv_chnum(int minor)
{
    drv_chnum_t *drv_chnum_node, *ne;

    list_for_each_entry_safe(drv_chnum_node, ne, &drv_chnum_list[minor], list) {
        drv_chnum_node->vch = -1;
        list_del_init(&drv_chnum_node->list);
    }
}

void release_node_memory(job_node_t *parent_node)
{
    job_node_t  *ne, *child_node;

    list_for_each_entry_safe(child_node, ne, &parent_node->child_list, child_list) {
        list_del_init(&child_node->child_list);
        kmem_cache_free(g_info.job_cache, child_node);
        atomic_dec(&g_info.alloc_node_count);
    }

    kmem_cache_free(g_info.job_cache, parent_node);
    atomic_dec(&g_info.alloc_node_count);
}

/* ref_node must be top frame
 */
int one_frame_mode_60P_check(job_node_t *node, job_node_t *top_frame, int minor_idx, struct video_entity_job_t *cur_job)
{
    if (!ONE_FRAME_MODE_60P(node))
        return 0;

    if (!(node->chan_param.command & OPT_CMD_DIDN)) {
        printk("%s, one_frame_60P with DIDN = 0! (src_interlace: %d, didn = 0x%x)\n", __func__,
                                    node->chan_param.src_interlace, node->chan_param.didn);
        return -1;
    }

    if (expect_field[minor_idx] != ONE_FRAME_SRC_FIELD(node)) {
        if (debug_value) {
            printm("DI", "minor:%d, jid:%d, wrong src_field: %d, expected: %d, DROP! \n", minor_idx,
                    cur_job->id, ONE_FRAME_SRC_FIELD(node), expect_field[minor_idx]);
            if (debug_value == 0xF)
                printk("DI, minor:%d, jid:%d, wrong src_field: %d, expected: %d DROP! \n", minor_idx,
                    cur_job->id, ONE_FRAME_SRC_FIELD(node), expect_field[minor_idx]);
        }

        release_node_memory(node);
        atomic_inc(&g_info.wrong_field_count);
        atomic_inc(&g_info.put_fail_count);
        return -1;
    }

    if (I_AM_BOT(node)) {
        job_node_t  *bottom_frame = node;

        if (top_frame == NULL) {
            printk("Bug in 3di for 60P, ref_node is NULL! \n");
            release_node_memory(node);
            return -1;
        }
        /* form the link */
        top_frame->pair_frame = bottom_frame;
        bottom_frame->pair_frame = top_frame;
    }

    return 0;
}

/* chan_param is driver level instead of vg level
 *
 * opt_write_newbuf indicates OPT_WRITE_NEWBUF
 *
 * New Action:
 * In frame mode, in_buf = out_buf is allowed. In this way, either interlace src or pure progressive src is possible.
 * We must support this two functions.
 */
void assign_chan_parameters(chan_param_t *chan_param /* DRV */, int func_auto, int src_fmt, u32 bg_wh, int didn, u32 opt_write_newbuf)
{
    /* get background width and height */
    chan_param->bg_width = EM_PARAM_WIDTH(bg_wh);
    chan_param->bg_height = EM_PARAM_HEIGHT(bg_wh);
    chan_param->didn = didn;

    chan_param->command = opt_write_newbuf;

    if (func_auto == 1) {
        switch (src_fmt) {
          case TYPE_YUV422_FIELDS:
            chan_param->frame_type = FRAME_TYPE_YUV422FRAME;
            chan_param->command |= (OPT_CMD_DN_TEMPORAL | OPT_CMD_DN_SPATIAL); /* denoise */
            break;

          case TYPE_YUV422_RATIO_FRAME:
          case TYPE_YUV422_FRAME:
          case TYPE_YUV422_FRAME_2FRAMES:
          case TYPE_YUV422_FRAME_FRAME:
            chan_param->frame_type = (chan_param->feature & SPEICIAL_FUNC_60P) ?
                                                FRAME_TYPE_YUV422FRAME_60P : FRAME_TYPE_YUV422FRAME;

            if (chan_param->didn & 0x0C) //denoise
                chan_param->command |= (OPT_CMD_DN_TEMPORAL | OPT_CMD_DN_SPATIAL); /* denoise on */

            if (chan_param->src_interlace == 1) {
                /* interlace */
                if (chan_param->didn & 0x03)    /* deinterlace on */
                    chan_param->command |= OPT_CMD_DI;
                else
                    chan_param->command &= ~OPT_CMD_DI; /* deinterlace off */
            } else {
                /* progressive */
                if ((chan_param->command & OPT_CMD_DIDN) == 0)
                    chan_param->command |= OPT_CMD_DISABLED;
                /* Avoid videograph bug, too many illeagal cases  */
                chan_param->command &= ~OPT_CMD_DI;
            }
            /* only for deinterlace. 60P doesn't need OPT_HYBRID_WRITE_NEWBUF */
            if ((chan_param->command & OPT_CMD_DI) && (chan_param->frame_type == FRAME_TYPE_YUV422FRAME)) {
                /* for the inbuf != outbuf case. In this case we don't need to queue 3 frames */
                if (!(chan_param->command & OPT_WRITE_NEWBUF))
                    chan_param->command |= OPT_HYBRID_WRITE_NEWBUF;
            }
            break;

          case TYPE_YUV422_2FRAMES:
          case TYPE_YUV422_2FRAMES_2FRAMES:
            chan_param->frame_type = FRAME_TYPE_2FRAMES;
            if (chan_param->src_interlace == 0) {
#if 1 /* TBD, videograph ready? */
                chan_param->command |= OPT_CMD_OPP_COPY;    /* TBD, no DIDN? */
#else
                if (chan_param->didn == 0)
                    chan_param->command |= OPT_CMD_OPP_COPY;    /* TBD, no DIDN? */
                else
                    chan_param->command = OPT_CMD_DISABLED;
#endif
                if (chan_param->didn & 0x0C) //denoise only.
                    chan_param->command |= (OPT_CMD_DN_TEMPORAL | OPT_CMD_DN_SPATIAL);

                chan_param->command &= ~OPT_CMD_DI;
            } else {
                if (chan_param->didn & 0x0C)
                    chan_param->command |= (OPT_CMD_DN_TEMPORAL | OPT_CMD_DN_SPATIAL);
                if (chan_param->didn & 0x03)
                    chan_param->command |= OPT_CMD_DI;
            }
            break;
          default:
            /* As Foster request, when src_fmt = -1, do nothing */
            chan_param->frame_type = FRAME_TYPE_YUV422FRAME;
            chan_param->command = OPT_CMD_DISABLED;
            break;
        }
        /* channel is skip, do nothing */
        if (chan_param->skip) {
            chan_param->command = (OPT_CMD_SKIP | OPT_CMD_DISABLED);
            return;
        }
    } else {
        chan_param->command |= OPT_CMD_DISABLED;
        switch (src_fmt) {
          case TYPE_YUV422_FIELDS:
          case TYPE_YUV422_RATIO_FRAME:
          case TYPE_YUV422_FRAME:
          case TYPE_YUV422_FRAME_2FRAMES:
          case TYPE_YUV422_FRAME_FRAME:
            chan_param->frame_type = FRAME_TYPE_YUV422FRAME;
            break;
          case TYPE_YUV422_2FRAMES:
          case TYPE_YUV422_2FRAMES_2FRAMES:
            chan_param->frame_type = FRAME_TYPE_2FRAMES;
            /* if in_buf = out_buf, we just return this job back */
            if (chan_param->command & OPT_WRITE_NEWBUF)
                chan_param->command |= OPT_CMD_FRAME_COPY;
            break;
          default:
            chan_param->frame_type = FRAME_TYPE_YUV422FRAME;
            chan_param->command = OPT_CMD_DISABLED;
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
 *      if (src_interlace == 1) {     ==> it is interlace case
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
static int driver_parse_in_property(job_node_t *parent_node, struct video_entity_job_t *job)
{
    int minor_idx = MAKE_IDX(ENTITY_MINORS, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
    struct video_property_t *property = job->in_prop;
    ftdi220_param_t *param = (ftdi220_param_t *)&parent_node->param;
    chan_param_t  *chan_param;
    job_node_t    *node, *node_item;
    int i, func_auto = -1, src_fmt = -1, first_round = 0;
    unsigned int bg_wh = 0, opt_write_newbuf = 0;
    unsigned int alloc_ch[(MAX_VCH + 7) >> 3];
    int didn = 0x0F;

    memset(alloc_ch, 0, sizeof(alloc_ch));
    memset(parent_node, 0, sizeof(job_node_t));

    i = 0;
    /* fill up the input parameters */
    while (property[i].id != 0) {
        /* channel number */
        if (first_round == 0) {
            first_round = 1;
            parent_node->chnum = property[i].ch;
            parent_node->hash[parent_node->chnum & MAX_VCH_MASK] = (u32)parent_node;
            parent_node->drv_chnum = get_drv_chnum(minor_idx, parent_node->chnum);
            parent_node->parent_node = parent_node;
            INIT_LIST_HEAD(&parent_node->child_list);
            parent_node->minor_idx = minor_idx;
            if (property[i].ch >= MAX_VCH)
                panic("%s, vch: %d is out of range %d! \n", __func__, property[i].ch, MAX_VCH);
            set_bit(property[i].ch, (void *)alloc_ch);
            parent_node->nr_splits ++;
        }

        /* Allocate all the child nodes.
         * Note: the parent node was allocated in upper function.
         */
        if (!test_bit(property[i].ch, (void *)alloc_ch)) {
            job_node_t *node_item;

            node_item = (job_node_t *)kmem_cache_alloc(g_info.job_cache, GFP_KERNEL);
            if (node_item == NULL)
                DI_PANIC("%s, no memory! \n", __func__);
            else
                atomic_inc(&g_info.alloc_node_count);

            memset(node_item, 0, sizeof(job_node_t));
            node_item->chnum = property[i].ch;
            if (parent_node->hash[node_item->chnum & MAX_VCH_MASK] != 0)
                printk("%s, vch is not enough! \n", __func__);

            parent_node->hash[node_item->chnum & MAX_VCH_MASK] = (u32)node_item;
            node_item->drv_chnum = get_drv_chnum(minor_idx, node_item->chnum);
            node_item->parent_node = parent_node;
            INIT_LIST_HEAD(&node_item->child_list);
            node_item->minor_idx = minor_idx;
            list_add_tail(&node_item->child_list, &parent_node->child_list);
            parent_node->nr_splits ++;
            if (property[i].ch >= MAX_VCH)
                panic("%s, vch: %d is out of range %d! \n", __func__, property[i].ch, MAX_VCH);
            set_bit(property[i].ch, (void *)alloc_ch);
        }

        node_item = (job_node_t *)parent_node->hash[property[i].ch & MAX_VCH_MASK];
        if (node_item == NULL) {
            printk("%s, warning! can't find node by vch: %d \n", __func__, property[i].ch);
            continue;
        }

        chan_param = &node_item->chan_param;
        switch(property[i].id) {
          case ID_SRC_BG_DIM:
            /* global? */
            bg_wh = property[i].value;
            break;

          case ID_FUNCTION_AUTO: /* 1 for auto checking the working mode. */
            /* global */
            func_auto = property[i].value;
            break;

          case ID_SRC_INTERLACE: /* chx_interlace */
            if (property[i].value == 2)
                chan_param->skip = 1;
            chan_param->src_interlace = (property[i].value == 1) ? 1 : 0;
            if (debug_value == 0xff)
                printk("chan_param->src_interlace => %d \n", property[i].value);

            break;

          case ID_SRC_FMT: /* chx_src_fmt, TYPE_YUV422_FIELDS, TYPE_YUV422_FRAME, TYPE_YUV422_2FRAMES */
            src_fmt = property[i].value;
            break;

          case ID_SRC_XY:   //per channel
            chan_param->offset_x = EM_PARAM_X(property[i].value);
            chan_param->offset_y = EM_PARAM_Y(property[i].value);
            break;

          case ID_SRC_DIM:  //per channel
            chan_param->ch_width = EM_PARAM_WIDTH(property[i].value);
            chan_param->ch_height = EM_PARAM_HEIGHT(property[i].value);
            break;

          case ID_DIDN_MODE:
            /* global */
            didn = property[i].value;

            if ((didn != 0x0F) && (didn != 0x00) && (didn != 0x03) && (didn != 0x0C)) {
                printk("3DI: Warning, ID_DIDN_MODE is invalid: 0x%x \n", didn);
                didn = 0x0F; //default value
            }
            break;

          case ID_SPEICIAL_FUNC:    /*  bit0: 1frame mode 60i function. */
            chan_param->feature = property[i].value;
            break;

          case ID_SOURCE_FIELD:     /* 0: top field, 1: bottom field.  */
            chan_param->src_field = (property[i].value == 0) ? SRC_FIELD_TOP : SRC_FIELD_BOT;
            break;

          default:
            break;
        }

        /* keep the database */
        if (property[i].id != 0) {
            if (i < MAX_RECORD) {
                property_record[minor_idx].job_id = job->id;
                property_record[minor_idx].property[i].id = property[i].id;
                property_record[minor_idx].property[i].value = property[i].value;
            } else {
                printk("Warning! FTDI220: The array is too small, array size is %d! \n", MAX_RECORD);
            }
        }

        i ++;
    }

    if (i < MAX_RECORD)
        property_record[minor_idx].property[i].id = ID_NULL;

    /* nr_splits wil be decreased when job finish */
    parent_node->nr_splits_keep = parent_node->nr_splits;

    /*
     * step1: update parent node's parameters
     */
    param = (ftdi220_param_t *)&parent_node->param;
    chan_param = &parent_node->chan_param;

    /* the result needs to be outputed to different buffer */
    if (job->in_buf.buf[0].addr_pa != job->out_buf.buf[0].addr_pa)
        opt_write_newbuf = OPT_WRITE_NEWBUF;

    /*
     * Step1: update parent's parameters
     */
    /* assign per channel parameters */
    assign_chan_parameters(chan_param, func_auto, src_fmt, bg_wh, didn, opt_write_newbuf);

    parent_node->collect_command = chan_param->command;
    param->minor = minor_idx;
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
        /* driver layer param */
        param = (ftdi220_param_t *)&node->param;
        param->minor = minor_idx;
        chan_param = &node->chan_param;

        /* fixup, just follow parent's setting for one frame mode 60p */
        chan_param->feature = parent_node->chan_param.feature;
        chan_param->src_field = parent_node->chan_param.src_field;

        /*
         * assign per channel parameters
         */
        assign_chan_parameters(chan_param, func_auto, src_fmt, bg_wh, didn, opt_write_newbuf);
        parent_node->collect_command |= chan_param->command;
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

    if (debug_value == 0xff)
        printk("collect command: 0x%x \n", parent_node->collect_command);

    if (debug_value && ONE_FRAME_MODE_60P(parent_node)) {
        printm("DI", "minor%d, oneframe_60P, src_field: %d, didn: 0x%x, c_cmd: 0x%x \n", minor_idx,
                parent_node->chan_param.src_field, parent_node->chan_param.didn, parent_node->collect_command);
        if (debug_value == 0xf)
            printk("DI, minor%d, oneframe_60P, src_field: %d, didn: 0x%x, c_cmd: 0x%x \n", minor_idx,
                parent_node->chan_param.src_field, parent_node->chan_param.didn, parent_node->collect_command);
    }

    /* FIXUP:
     *
     * Update di_sharebuf flag and OPT_HYBRID_WRITE_NEWBUF. If one interlace channel exists, whole frame must be di_sharebuf and
     * command with OPT_HYBRID_WRITE_NEWBUF.
     */
    chan_param = &parent_node->chan_param;
    /* only for deinterlace only */
    if ((chan_param->frame_type == FRAME_TYPE_YUV422FRAME) && (parent_node->collect_command & OPT_CMD_DI)) {
        parent_node->di_sharebuf = (opt_write_newbuf & OPT_WRITE_NEWBUF) ? 0 : 1;

        if (parent_node->di_sharebuf) {
            chan_param->command |= OPT_HYBRID_WRITE_NEWBUF;

            param = (ftdi220_param_t *)&parent_node->param;
            param->command |= OPT_HYBRID_WRITE_NEWBUF;
        }
        /* childs */
        list_for_each_entry(node, &parent_node->child_list, child_list) {
            node->di_sharebuf = parent_node->di_sharebuf;
            if (node->di_sharebuf) {
                chan_param = &node->chan_param;
                chan_param->command |= OPT_HYBRID_WRITE_NEWBUF;

                param = (ftdi220_param_t *)&node->param;
                param->command |= OPT_HYBRID_WRITE_NEWBUF;
            }
        }
    }

    if (debug_value == 0xff) {
        printk("Parent AllocDrvCh, minor=%d, vch=%d, drv=%d, nr_splits=%d, cmd:0x%x, c_cmd:0x%x, ftype:%d, sharebuf:%d \n", minor_idx,
                parent_node->chnum, parent_node->drv_chnum, parent_node->nr_splits, parent_node->chan_param.command,
                parent_node->collect_command, parent_node->chan_param.frame_type, parent_node->di_sharebuf);

        list_for_each_entry(node, &parent_node->child_list, child_list) {
            printk("Child AllocDrvCh, minor=%d, vch=%d, drv=%d, cmd:0x%x, ftype:%d \n", minor_idx,
                node->chnum, node->drv_chnum, node->chan_param.command, node->chan_param.frame_type);
        }
    }

    return 0;
}

/*
 * param: private data
 */
static int driver_set_out_property(void *param, struct video_entity_job_t *job, u32 command)
{
    //ftdi220_job_t   *job_item = (ftdi220_job_t *)param;
    //u32 command = job_item->param.command;

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

    if (debug_value)
        printm("DI", "DI_RESULT jobid: %d, value = %d \n", job->id, job->out_prop[0].value);

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

    if (debug_value)
        printm("DI", "engine_start dev%d, minor=%d, drvch=%d, vch=%d, command:0x%x, status: 0x%x, jid: %d \n",
            node_item->dev, node_item->minor_idx, node_item->drv_chnum, node_item->chnum, node_item->param.command,
            node_item->status, job->id);

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

    if (parent_node->nr_splits > 0)
        parent_node->nr_splits --;

    if (debug_value) {
        printm("DI", "engine_finish dev%d, minor=%d, drvch=%d, vch=%d, command:0x%x, nr_splits:%d, status: 0x%x, jid:%d, hw:%d \n",
            node_item->dev, node_item->minor_idx, node_item->drv_chnum, node_item->chnum, node_item->param.command,
            parent_node->nr_splits, node_item->status, job->id, job_item->hardware);
    }
    DRV_VGUNLOCK(flags);

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
    job_node_t  *node, *ne, *child_node, *bot_frame = NULL;
    unsigned int chnum, jid, loop;
    struct video_entity_job_t *job;

    set_user_nice(current, -20);

    if (debug_value)
        printm("DI", "ftdi220_callback_process.... \n");

    for (chnum = 0; chnum < ENTITY_MINORS; chnum ++) {
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
                if (debug_value)
                    printm("DI", "%s, jid %d not job done. node_status: 0x%x\n", __func__,
                        ((struct video_entity_job_t *)node->private)->id, node->status);

                DRV_VGUNLOCK(flags);
                break;
            }

            /* this job is still referenced */
            if (atomic_read(&node->ref_cnt) > 0) {
                if (debug_value)
                    printm("DI", "%s, jid %d not job done. node_status: %d, ref_cnt: %d.\n", __func__,
                        ((struct video_entity_job_t *)node->private)->id, node->status, atomic_read(&node->ref_cnt));

                DRV_VGUNLOCK(flags);
                break;
            }

            if (node->status & JOB_STS_QUEUE) {
                if (debug_value)
                    printm("DI", "IN FIFO, job->id=%d,(CH=%d, node_status=%d) \n",
                                ((struct video_entity_job_t *)node->private)->id, chnum, node->status);
                DRV_VGUNLOCK(flags);
                break;
            }

            list_del_init(&node->list);

            loop = 1;
			bot_frame = NULL;
            if (ONE_FRAME_MODE_60P(node) && node->pair_frame) {
                bot_frame = node->pair_frame;
                bot_frame->status = node->status;
                bot_frame->nr_splits = node->nr_splits;
                bot_frame->first_node_return_fail = node->first_node_return_fail;
                atomic_set(&bot_frame->ref_cnt, 0);
                loop = 2;
            }
            DRV_VGUNLOCK(flags);
again:
            job = (struct video_entity_job_t *)node->private;
            jid = job->id;

            if (node->first_node_return_fail == 1)
                job->status = JOB_STATUS_FAIL;
			else if (node->status & JOB_STS_DONE )
                job->status = JOB_STATUS_FINISH;
            else
                job->status = JOB_STATUS_FAIL;

            if (debug_value)
                printm("DI", "%s, CB, job->id = %d, job_status: %d \n", __func__,
                        ((struct video_entity_job_t *)node->private)->id, job->status);

            if (job->status == JOB_STATUS_FAIL)
                atomic_inc(&g_info.cb_fail_count);

            job->callback(job);

            DRV_VGLOCK(flags);
            list_for_each_entry_safe(child_node, ne, &node->child_list, child_list) {
                list_del_init(&child_node->child_list);
                kmem_cache_free(g_info.job_cache, child_node);
                atomic_dec(&g_info.alloc_node_count);
            }
            DRV_VGUNLOCK(flags);

            if (node->nr_splits) {
                printm("DI", "%s, jid %d nr_split:%d(%d) not zero, di_sharebuf:%d!\n", __func__, jid, node->status,
                                                    node->nr_splits, node->nr_splits_keep, node->di_sharebuf);
                printk("%s, jid %d nr_split:%d(%d) not zero, di_sharebuf: %d!\n", __func__, jid, node->nr_splits,
                                                    node->nr_splits_keep, node->di_sharebuf);
                damnit("DI");
            }

            kmem_cache_free(g_info.job_cache, node);
            atomic_dec(&g_info.alloc_node_count);
            atomic_dec(&g_info.vg_node_count);

            loop --;
            if (loop) {
                node = bot_frame;
                goto again;
            }
        } while(1);
    } /* chnum */
}

/* callback function for job finish. In ISR.
 * We must save the last job for de-interlace reference.
 */
static void driver_callback(ftdi220_job_t *job_item, void *private)
{
    unsigned long flags;
    job_node_t *parent_node, *node = (job_node_t *)private;
    struct video_entity_job_t   *job = (struct video_entity_job_t *)node->private;
    u32 command;

    DRV_VGLOCK(flags);
    /* if all child nodes are done, fall through the orginal code */
    if (node->parent_node->nr_splits) {
        if (debug_value)
            printm("DI", "driver_callback_none: dev:%d, status: 0x%x, jid: %d, nr_splits: %d, sharebuf:%d \n",
                    node->dev, node->status, job->id, node->parent_node->nr_splits, node->di_sharebuf);
        DRV_VGUNLOCK(flags);
        return;
    }
    DRV_VGUNLOCK(flags);

    parent_node = node->parent_node;
    /* di_sharebuf case */
    parent_node->status = (parent_node->status == JOB_STS_MARK_FLUSH) ? JOB_STS_FLUSH : job_item->status;
    parent_node->dev = job_item->dev;
    command = parent_node->collect_command;

    /* give the result by priority. In order to meet driver_set_out_property() */
    if (command & OPT_CMD_DI) command = OPT_CMD_DI;
    if (command & OPT_CMD_OPP_COPY) command = OPT_CMD_OPP_COPY;
    if (command & OPT_CMD_FRAME_COPY) command = OPT_CMD_FRAME_COPY;
    if (command & OPT_CMD_DISABLED) command = OPT_CMD_DISABLED;

    if (debug_value)
        printm("DI", "driver_callback, dev:%d, minor=%d, drvch=%d, vch=%d, c-command:0x%x, status: 0x%x, jid: %d, hw_done:%d\n",
            parent_node->dev, parent_node->minor_idx, parent_node->drv_chnum, parent_node->chnum,
            command, parent_node->status, job->id, job_item->hardware);

    driver_set_out_property(job_item, parent_node->private, command);

    /* decrease the reference count */
	DRV_VGLOCK(flags);
	if (parent_node->di_sharebuf == 1) {
	    /* The case that, the first three frames were not triggered by hardware. Thus we don't have
         * way to update their status.
         */
        if (parent_node->r_refer_to->status & JOB_STS_DRIFTING) {
            if (debug_value) {
                struct video_entity_job_t   *job2 = (struct video_entity_job_t *)parent_node->r_refer_to->private;

                printm("DI", "driver_callback(drifting): dev:%d, status: 0x%x, jid: %d(seqid:%d), sharebuf:1 \n", node->dev, node->status, job->id, parent_node->job_seqid);
                printm("DI", "reset drifting refer_node cnt to zero, jid:%d (seqid:%d), sharebuf \n", job2->id, parent_node->r_refer_to->job_seqid);
            }

            /* Important Notice: reset to zero.
             * The nr_splits of drifting job cannot relate to this job due to poential different version with different split_nr
             * Thus we cannot use split_nr of this job to decrease its split_nr of drifting job. It will be mismatch.
             */
            parent_node->r_refer_to->nr_splits = 0;
            driver_set_out_property(job_item, parent_node->r_refer_to->private, command);
        }
        parent_node->r_refer_to->status = job_item->status; /* note: not parent status due to mark_flush possible */
        REFCNT_DEC(parent_node->r_refer_to);
        parent_node->r_refer_to = NULL;
	} else {
	     if (parent_node->refer_to)
            REFCNT_DEC(parent_node->refer_to);
         parent_node->refer_to = NULL;
	}

	DRV_VGUNLOCK(flags);

#ifdef USE_KTHREAD
    ftdi220_cb_wakeup();
#else
    /* schedule the delay workq */
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)ftdi220_callback_process);
    queue_delayed_work(g_info.workq, &process_callback_work,  callback_period);
#endif
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
            if (command & (OPT_WRITE_NEWBUF | OPT_HYBRID_WRITE_NEWBUF)) {
                param->crwb_ptr = cur->out_buf.buf[0].addr_pa;
                param->ntwb_ptr = cur->in_buf.buf[0].addr_pa + line_ofs;
            }
            meet = 1;
        } else if (command & OPT_CMD_DN) {
            /*
             * denoise only (progressive)
             */
            if (command & (OPT_WRITE_NEWBUF | OPT_HYBRID_WRITE_NEWBUF)) {
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
            if (command & (OPT_WRITE_NEWBUF | OPT_HYBRID_WRITE_NEWBUF)) {
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
            if (command & (OPT_WRITE_NEWBUF | OPT_HYBRID_WRITE_NEWBUF)) {
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

    if (command & OPT_CMD_DISABLED) {
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

/*
 * This function assign the FF, FW, CR, NT ... buffer address for interlace frame type
 * prev: refernce job. It must be top job.
 * cur: current job, it can be top/bottom job
 */
static inline int assign_yuv422frame_60p_buffers(job_node_t *prev_node,
                                        struct video_entity_job_t *cur, ftdi220_param_t *param)
{
    job_node_t  *cur_node, *cur_top_frame, *cur_bot_frame;
    job_node_t  *prev_top_frame, *prev_bot_frame;
    struct video_entity_job_t *prev_top_job, *prev_bot_job;
    struct video_entity_job_t *cur_top_job, *cur_bot_job;
    ftdi220_param_t *cur_top_param;
    u32 command = param->command, line_ofs;
    int meet = 0;

    if (param->frame_type != FRAME_TYPE_YUV422FRAME_60P)
        return 0;

    cur_node = container_of(param, job_node_t, param);
    if (I_AM_TOP(cur_node))
        return 1;   /* wait bottom frame coming */

    /* Tricky: Because now this node is bottom frame, prev_node will be CURRENT top frame. Beause in driver_pubjob(),
     * g_info.ref_node[minor_idx] keeps top frame but now this node is bottom one. Thus g_info.ref_node[minor_idx]
     * keeps CURRENT top frmame instead of previous top frame. So we must use topframe's refer_to
     * to get real previous top frame.
     */
    if (prev_node)
        prev_node = (job_node_t *)prev_node->refer_to;

    cur_bot_frame = cur_node;
    cur_top_frame = cur_bot_frame->pair_frame;

    cur_top_job = (struct video_entity_job_t *)cur_top_frame->private;
    cur_bot_job = (struct video_entity_job_t *)cur_bot_frame->private;
    cur_top_param = &cur_top_frame->param;

    prev_top_job = prev_bot_job = NULL;
    prev_top_frame = prev_bot_frame = NULL;

    if (prev_node) {
        prev_top_frame = prev_node;
        prev_bot_frame = prev_top_frame->pair_frame;
        prev_top_job = (struct video_entity_job_t *)prev_top_frame->private;
        prev_bot_job = (struct video_entity_job_t *)prev_bot_frame->private;
    }

    line_ofs = cur_top_param->width << 1;    //yuv422

    /* Two Frames case, 60i -> 60p
     */
    if (prev_top_frame) {
        cur_top_param->ff_y_ptr = prev_top_job->in_buf.buf[0].addr_pa;  // previous top frame
        cur_top_param->fw_y_ptr = prev_bot_job->in_buf.buf[0].addr_pa + line_ofs;  //previous bottom frame, must add one line
        cur_top_param->cr_y_ptr = cur_top_job->in_buf.buf[0].addr_pa;   // current top frame
        cur_top_param->nt_y_ptr = cur_bot_job->in_buf.buf[0].addr_pa;   //current bottom frame. can't offset one line
        meet = 1;
    } else {
        cur_top_param->ff_y_ptr = cur_top_job->in_buf.buf[0].addr_pa;    // previous top frame
        cur_top_param->fw_y_ptr = cur_bot_job->in_buf.buf[0].addr_pa;    //previous bottom frame
        cur_top_param->cr_y_ptr = cur_top_job->in_buf.buf[0].addr_pa;    // current top frame
        cur_top_param->nt_y_ptr = cur_bot_job->in_buf.buf[0].addr_pa;    //current bottom frame
        meet = 1;
    }

    cur_top_param->di0_ptr = cur_top_job->in_buf.buf[0].addr_pa + line_ofs;
    cur_top_param->di1_ptr = cur_bot_job->in_buf.buf[0].addr_pa;
    cur_top_param->crwb_ptr = cur_top_job->in_buf.buf[0].addr_pa;
    cur_top_param->ntwb_ptr = cur_bot_job->in_buf.buf[0].addr_pa + line_ofs;

    if (command & OPT_CMD_SKIP) {
        //just random assignment
        cur_top_param->ff_y_ptr = cur_top_job->in_buf.buf[0].addr_pa;    // previous top frame
        cur_top_param->fw_y_ptr = cur_bot_job->in_buf.buf[0].addr_pa + line_ofs;    //previous bottom frame
        cur_top_param->cr_y_ptr = cur_top_job->in_buf.buf[0].addr_pa;    // current top frame
        cur_top_param->nt_y_ptr = cur_bot_job->in_buf.buf[0].addr_pa + line_ofs;    //current bottom frame
        meet = 1;
    }

    if (!cur_top_param->ff_y_ptr || !cur_top_param->fw_y_ptr || !cur_top_param->cr_y_ptr || !cur_top_param->nt_y_ptr
        || !cur_top_param->di0_ptr || !cur_top_param->di1_ptr || !cur_top_param->crwb_ptr || !cur_top_param->ntwb_ptr) {
        printk("%s, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x! \n", __func__, cur_top_param->ff_y_ptr,
            cur_top_param->fw_y_ptr, cur_top_param->cr_y_ptr, cur_top_param->nt_y_ptr, cur_top_param->di0_ptr,
            cur_top_param->di1_ptr, cur_top_param->crwb_ptr, cur_top_param->ntwb_ptr);
        meet = 0;
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

    if (!(command & OPT_HYBRID_WRITE_NEWBUF)) {
        printk("%s, bug in 3di 2! command:0x%x \n", __func__, command);
        return -1;
    }

    if (!r || !n || !p || !c) {
        printk("3di error! %p %p %p %p \n", r, n, p, c);
        return -1;
    }

    if (command & OPT_CMD_DI) {
        param->ff_y_ptr = r->out_buf.buf[0].addr_pa; /* result buffer */
        param->fw_y_ptr = p->in_buf.buf[0].addr_pa + line_ofs;   /* input buffer */
        param->cr_y_ptr = c->in_buf.buf[0].addr_pa;
        param->nt_y_ptr = c->in_buf.buf[0].addr_pa + line_ofs;
        param->di0_ptr = n->out_buf.buf[0].addr_pa + line_ofs;
        param->crwb_ptr = n->out_buf.buf[0].addr_pa;
        param->ntwb_ptr = c->in_buf.buf[0].addr_pa + line_ofs;
    } else {
        param->ff_y_ptr = r->out_buf.buf[0].addr_pa;    /* result buffer */
        param->fw_y_ptr = r->in_buf.buf[0].addr_pa + line_ofs;   /* input buffer */
        param->cr_y_ptr = c->in_buf.buf[0].addr_pa;
        param->nt_y_ptr = c->in_buf.buf[0].addr_pa + line_ofs;
        param->crwb_ptr = n->out_buf.buf[0].addr_pa;
        param->ntwb_ptr = n->out_buf.buf[0].addr_pa + line_ofs;
    }

    return 0;
}

/* node: child nodes
 * return 0 for ok
 */
static inline int assign_drvcore_parameters(job_node_t *parent_node, job_node_t *node, job_node_t *ref_node_item,
                                    struct video_entity_job_t *cur_job, struct video_entity_job_t *ref_vjob)
{
    int ret, minor_idx;

    minor_idx = MAKE_IDX(ENTITY_MINORS, ENTITY_FD_ENGINE(cur_job->fd), ENTITY_FD_MINOR(cur_job->fd));

    node->private = cur_job;
    node->refer_to = ref_node_item;
    node->puttime = jiffies;
    node->starttime = 0;
    node->finishtime = 0;
    node->status = JOB_STS_QUEUE;

    ret = assign_2frame_buffers(ref_vjob, cur_job, &node->param);
    ret |= assign_2fields_buffers(ref_vjob, cur_job, &node->param);
    ret |= assign_yuv422frame_buffers(ref_vjob, cur_job, &node->param);
    ret |= assign_yuv422frame_60p_buffers(ref_node_item, cur_job, &node->param);

    /* New function, when function_auto=0 and without frame-copy, then we just return this job ASAP */
    if ((node->param.command & OPT_CMD_DISABLED) && ((node->param.command & OPT_CMD_FRAME_COPY) == 0))
        node->status |= JOB_STS_DONOTHING;

    if (node->param.command & OPT_CMD_SKIP)
        node->status |= JOB_STS_DONOTHING;

    if ((node->param.command & OPT_CMD_DI) && (ref_node_item == NULL))
		node->first_node_return_fail = 1;
	else
	    node->first_node_return_fail = 0; /* bottom frame will be updated in callback if have */

    if (debug_value)
        printm("DI", "drvcore, minor=%d, drvch:%d, vch:%d, command: 0x%x, ftype:%d, sharebuf:%d \n", minor_idx,
                node->drv_chnum, node->chnum, node->param.command, node->param.frame_type, node->di_sharebuf);

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

    if (ref_node->di_sharebuf) {
        /* status with JOB_STS_DRIFTING is not in drv-core, thus we must flush them here.
         */
        if (ref_node->status & JOB_STS_DRIFTING) {
            ref_node->nr_splits = 0;
            ref_node->status = JOB_STS_FLUSH;
        } else {
            /* the case that job with multiple splits is still ongoing. Thus we can't mark this job as finish.
             */
            ref_node->status = (ref_node->nr_splits > 0) ? JOB_STS_MARK_FLUSH : JOB_STS_FLUSH;

            /* Note: cannot reset ref_node->nr_splits to zero. It muse follow the normal path to decrease to zero.
             */
        }
    }

    REFCNT_DEC(ref_node);
}

/* check command whether the command is change. 1 for yes ,0 for not
 */
static int check_command_change(job_node_t *prev_node, job_node_t *cur_node, struct video_entity_job_t *cur_job)
{
    u32 prev_cmd, cur_cmd;
    job_node_t *node1, *node2;
    struct video_entity_job_t *prev_job;

    /* version change from videograph. such as split change ....*/
    prev_job = (struct video_entity_job_t *)prev_node->private;
    if (prev_job->ver != cur_job->ver) {
        atomic_inc(&g_info.vg_chg_count);
        if (debug_value)
            printm("DI", "version change due to videograph: %d, %d \n", prev_job->ver, cur_job->ver);

        if (debug_value == 0xfff)
            printk("DI, version change due to videograph: %d, %d \n", prev_job->ver, cur_job->ver);
        return 1;
    }

    /* compare the collect command first */
    prev_cmd = prev_node->collect_command & OPT_CMD_DIDN;
    cur_cmd = cur_node->collect_command & OPT_CMD_DIDN;

    if (prev_cmd != cur_cmd) {
        if (debug_value == 0xfff)
            printk("DI, prev_cmd1: 0x%x, cur_cmd1: 0x%x \n", prev_cmd, cur_cmd);
        return 1;
    }

    if (ONE_FRAME_MODE_60P(prev_node) != ONE_FRAME_MODE_60P(cur_node)) {
        if (debug_value == 0xfff)
            printk("DI, frame format is changed! 0x%x, 0x%x\n", prev_node->chan_param.frame_type,
                                cur_node->chan_param.frame_type);
        return 1;
    }

    /*
     * start to compare the content, and from the parent
     */

    /* take preious node as the base, current node will be the compare target. node1 is previous, node2 is current.
     */
    node1 = prev_node;
    node2 = (job_node_t *)cur_node->hash[node1->chnum & MAX_VCH_MASK];
    if (node2 != NULL) {
        prev_cmd = node1->chan_param.command & OPT_CMD_DIDN;
        cur_cmd = node2->chan_param.command & OPT_CMD_DIDN;
        if (prev_cmd != cur_cmd) {
            if (debug_value == 0xfff)
                printk("DI, prev_cmd2: 0x%x, cur_cmd2: 0x%x \n", prev_cmd, cur_cmd);
            return 1;
        }
    }

    /* compare the child */
    list_for_each_entry(node1, &prev_node->child_list, child_list) {
        node2 = (job_node_t *)cur_node->hash[node1->chnum & MAX_VCH_MASK];
        if (node2 == NULL)
            continue;

        prev_cmd = node1->chan_param.command & OPT_CMD_DIDN;
        cur_cmd = node2->chan_param.command & OPT_CMD_DIDN;
        if (prev_cmd != cur_cmd) {
            if (debug_value == 0xfff)
                printk("p_vch%d, cmd: 0x%x, n_vch:%d, cmd: 0x%x \n", node1->chnum, prev_cmd, node2->chnum, cur_cmd);
            return 1;
        }
    }

    return 0;
}

/*
 * put JOB
 * We must save the last job for de-interlace reference.
 */
static int driver_putjob(void *parm)
{
    unsigned long flags;
    struct video_entity_job_t *ref_vjob = NULL, *cur_job = (struct video_entity_job_t *)parm;
    job_node_t  *node, *parent_node, *ref_node_item;
    int ret = 0, minor_idx, need_wakeup = 0;

    if (irqs_disabled()) {
        printk("%s, 3DI detects irq_disabled! \n", __func__);
        printm("DI", "3DI detects irq_disabled! \n", __func__);
        damnit("DI");
    }

    if (debug_value)
        printm("DI", " putjob, jid: %d \n", cur_job->id);

    minor_idx = MAKE_IDX(ENTITY_MINORS, ENTITY_FD_ENGINE(cur_job->fd), ENTITY_FD_MINOR(cur_job->fd));
    if (minor_idx > ENTITY_MINORS)
        DI_PANIC("Error to use %s engine %d, max is %d\n",MODULE_NAME, minor_idx, ENTITY_MINORS);

    atomic_inc(&g_info.alloc_node_count);
    parent_node = (job_node_t *)kmem_cache_alloc(g_info.job_cache, GFP_KERNEL);
    if (parent_node == NULL)
        DI_PANIC("%s, no memory! \n", __func__);

    /*
     * parse the parameters and assign to param structure
     */
    ret = driver_parse_in_property(parent_node, cur_job);
    if (ret < 0) {
        printm("DI", "%s, ftdi220: parse property error! \n", __func__);
        if (irqs_disabled()) {
            printk("%s, 3DI detects irq_disabled2! \n", __func__);
            damnit("DI");
        }
        atomic_inc(&g_info.put_fail_count);
        return JOB_STATUS_FAIL;
    }
    if (debug_value) {
        if (ONE_FRAME_MODE_60P(parent_node))
            printm("DI", "minor: %d, this is 60P frame, field: %d \n", minor_idx, ONE_FRAME_SRC_FIELD(parent_node));
        else
            printm("DI", "minor: %d, this is not 60P frame. \n", minor_idx);
    }

    DRV_VGLOCK(flags);

    /* version is change */
    if (g_info.ref_node[minor_idx]) {
        /* check command change per channel */
        if (check_command_change(g_info.ref_node[minor_idx], parent_node, cur_job)) {
            atomic_inc(&g_info.ver_chg_count);
            if (debug_value) {
                printm("DI", "version change, old c_cmd: 0x%x, new c_cmd: 0x%x, minor_idx: %d \n",
                        g_info.ref_node[minor_idx]->collect_command, parent_node->collect_command,
                        minor_idx);
            }

            /* N */
            release_refered_node(g_info.n_ref_node[minor_idx]);
            g_info.n_ref_node[minor_idx] = NULL;
            /* P */
            release_refered_node(g_info.p_ref_node[minor_idx]);
            g_info.p_ref_node[minor_idx] = NULL;

            /* Because g_info.p_ref_node[] always keep the top frame. When bottom comes, g_info.p_ref_node[]
             * actually points the CURRENT top frame instead prvious top frame, thus we must treat it specially.
             */
            if (g_info.ref_node[minor_idx] && ONE_FRAME_MODE_60P(g_info.ref_node[minor_idx])) {
                job_node_t  *prev_ref_node = g_info.ref_node[minor_idx]->refer_to;
                struct video_entity_job_t *job;

                if (I_AM_BOT(parent_node) || /* this case only top frame in g_info.ref_node[minor_idx] */
                   (I_AM_TOP(parent_node) && !g_info.ref_node[minor_idx]->pair_frame)) /* the case: only top frame in g_info.ref_node[minor_idx] */
                {
                    /* step1: this job will not pair with top frame, thus we must set top frame as fail */
                    g_info.ref_node[minor_idx]->status = JOB_STS_FLUSH;
                    g_info.ref_node[minor_idx]->nr_splits = 0;
                    /* step2: and also decrease the real previous node's refernce cnt */
                    if (prev_ref_node) {
                        release_refered_node(prev_ref_node);

                        if (debug_value) {
                            job = (struct video_entity_job_t *)prev_ref_node->private;
                            printm("DI", "minor: %d, release 60P job_id: %d \n", minor_idx, job->id);
                        }
                    }
                }
            }

            /* C */
            release_refered_node(g_info.ref_node[minor_idx]);
            g_info.ref_node[minor_idx] = NULL;

            job_seqid[minor_idx] = 0;
            expect_field[minor_idx] = SRC_FIELD_TOP;
            parent_node->first_node_return_fail = 1;
            need_wakeup = 1;
        }
    } else {
        job_seqid[minor_idx] = 0;
    }

    /* assign job sequence id */
    parent_node->job_seqid = job_seqid[minor_idx];

    ref_node_item = g_info.ref_node[minor_idx];
    if (ref_node_item)
        ref_vjob = (struct video_entity_job_t *)ref_node_item->private;

    /* do one frame mode 60P checking */
    ret = one_frame_mode_60P_check(parent_node, ref_node_item, minor_idx, cur_job);
    if (ret < 0) {
        DRV_VGUNLOCK(flags);
        return JOB_STATUS_FAIL;
    }

    /* parent node */
    if (!assign_drvcore_parameters(parent_node, parent_node, ref_node_item, cur_job, ref_vjob)) {
        /* child nodes */
        list_for_each_entry(node, &parent_node->child_list, child_list) {
            /* update seqid */
            node->job_seqid = parent_node->job_seqid;
            assign_drvcore_parameters(parent_node, node, ref_node_item, cur_job, ref_vjob);
        }
        /* new reference node */
        if (parent_node->collect_command & OPT_CMD_DIDN) {
            /* only top_frame of 60P or non one_frame_mode_60p can be referenced job */
            if (!ONE_FRAME_MODE_60P(parent_node) || (ONE_FRAME_MODE_60P(parent_node) && I_AM_TOP(parent_node))) {
                /* add the reference count of this node. */
                REFCNT_INC(parent_node);
                g_info.ref_node[minor_idx] = parent_node;
            }
        }
    } else {
        DRV_VGUNLOCK(flags);
        printm("DI", "%s, ftdi220: assign_drvcore_parameters error! \n", __func__);
        printk("%s, ftdi220: assign_drvcore_parameters error! \n", __func__);
        if (irqs_disabled()) {
            printk("%s, 3DI detects irq_disabled3! \n", __func__);
            damnit("DI");
        }

        if (need_wakeup) {
#ifdef USE_KTHREAD
            ftdi220_cb_wakeup();
#else
            /* schedule the delay workq */
            PREPARE_DELAYED_WORK(&process_callback_work,(void *)ftdi220_callback_process);
            queue_delayed_work(g_info.workq, &process_callback_work, callback_period);
#endif
        }
        atomic_inc(&g_info.put_fail_count);
        release_node_memory(parent_node);
        return JOB_STATUS_FAIL;
    }

    /* Add to per minor link list.
     * only top_frame of 60P or non one_frame_mode_60p can be added to joblist
     */
    if (!ONE_FRAME_MODE_60P(parent_node) || (ONE_FRAME_MODE_60P(parent_node) && I_AM_TOP(parent_node)))
        ftdi220_joblist_add(minor_idx, parent_node);

    /* Important Notice:
     * When the job is OPT_CMD_DISABLED, the job may be very fast to reach ftdi220_callback_process(),
     * in which the child_list may be walked through and list_del. It will case the child_list corruption.
     * Thus we add protection for the child_list.
     */

     /* New, it must be interlaced FRAME_TYPE_YUV422FRAME. In this case, g_info.ref_node[minor_idx]
      * is only used to keep the last information of split, skip ... when the version is not changed.
      */
    if (parent_node->di_sharebuf) {
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

            if (debug_value)
                printm("DI", "parent_node->job_seqid: %d exit... \n", parent_node->job_seqid);

            goto exit;
        }

        if (debug_value)
            printm("DI", "parent_node->job_seqid: %d queue... \n", parent_node->job_seqid);

        ret = assign_yuv422frame_buffers_hybrid((struct video_entity_job_t *)r->private,
                                                (struct video_entity_job_t *)n->private,
                                                (struct video_entity_job_t *)p->private,
                                                (struct video_entity_job_t *)c->private,
                                                &parent_node->param);
        if (ret < 0) {
            DRV_VGUNLOCK(flags);
            if (irqs_disabled()) {
                printk("%s, 3DI detects irq_disabled4! \n", __func__);
                damnit("DI");
            }
            return -1;
        }

        /*
         * Child
         */
        list_for_each_entry(child_node, &parent_node->child_list, child_list) {
            assign_yuv422frame_buffers_hybrid((struct video_entity_job_t *)r->private,
                                              (struct video_entity_job_t *)n->private,
                                              (struct video_entity_job_t *)p->private,
                                              (struct video_entity_job_t *)c->private,
                                              &child_node->param);
        }
        atomic_inc(&g_info.hybrid_job_count);
    } /* parent_node->di_sharebuf */

    /* put parent node to job queue now
     */
    ret = 0;
    if (!ONE_FRAME_MODE_60P(parent_node) || (ONE_FRAME_MODE_60P(parent_node) && I_AM_BOT(parent_node))) {
        job_node_t *top_node = parent_node;

        /* use top frame to putjob */
        if (ONE_FRAME_MODE_60P(parent_node) && I_AM_BOT(parent_node))
            top_node = parent_node->pair_frame;

        ret = ftdi220_put_job(top_node->drv_chnum, &top_node->param, &callback_ops, top_node, top_node->status);
        if (!ret) {
            /* put child nodes to job queue now
             */
            list_for_each_entry(node, &top_node->child_list, child_list) {
                ret = ftdi220_put_job(node->drv_chnum, &node->param, &callback_ops, node, node->status);
                if (ret)
                    DI_PANIC("%s, put job fail! \n", __func__);
            }
        } else {
            DI_PANIC("%s, put job fail! \n", __func__);
        }
    }

exit:
    DRV_VGUNLOCK(flags);

    if (parent_node->di_sharebuf) {
        job_seqid[minor_idx] = job_seqid[minor_idx] > 0x0FFFFFFF ?
                            FRAME_COUNT_HYBRID_DI : job_seqid[minor_idx] + 1;
    }

    if (irqs_disabled()) {
        printk("%s, 3DI detects irq_disabled5! \n", __func__);
        damnit("DI");
    }

    atomic_inc(&g_info.vg_node_count);

    /* toggle the expected field */
    if (ONE_FRAME_MODE_60P(parent_node)) {
        expect_field[minor_idx] = (expect_field[minor_idx] == SRC_FIELD_TOP) ? SRC_FIELD_BOT : SRC_FIELD_TOP;
        if (debug_value) {
            printm("DI", "minor: %d, next expected field: %d \n", minor_idx, expect_field[minor_idx]);

            if (debug_value == 0xF)
                printk("DI, minor: %d, next expected field: %d \n", minor_idx, expect_field[minor_idx]);
        }
    }

    if (need_wakeup) {
#ifdef USE_KTHREAD
        ftdi220_cb_wakeup();
#else
        /* schedule the delay workq */
        PREPARE_DELAYED_WORK(&process_callback_work,(void *)ftdi220_callback_process);
        queue_delayed_work(g_info.workq, &process_callback_work, callback_period);
#endif
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

    if (debug_mode >= DEBUG_VG)
        printk("%s \n", __func__);

    if (debug_value)
        printm("DI", "STOP, minor=%d \n", minor_idx);

    atomic_inc(&g_info.stop_count);

    DRV_VGLOCK(flags);
    job_seqid[minor_idx] = 0;
    free_minor_drv_chnum(minor_idx);
    expect_field[minor_idx] = SRC_FIELD_TOP;

    if (g_info.ref_node[minor_idx]) {
        job_node_t  *node_item = g_info.ref_node[minor_idx];
        struct video_entity_job_t *job = (struct video_entity_job_t *)node_item->private;

        /* Note: g_info.r_ref_node[minor_idx] will follow the drv core path to process. We can't
         * handle it here. g_info.r_ref_node[minor_idx] maybe return to videograph already.
         */
        parent_node = g_info.ref_node[minor_idx];

        /* R
         * ==> just follow the orginal path in drv-core
         */
        /* N */
        release_refered_node(g_info.n_ref_node[minor_idx]);
        g_info.n_ref_node[minor_idx] = NULL;
        /* P */
        release_refered_node(g_info.p_ref_node[minor_idx]);
        g_info.p_ref_node[minor_idx] = NULL;

        /* 60P is a special case, only top frame exist in the g_info.ref_node[minor_idx] */
        if (g_info.ref_node[minor_idx] && ONE_FRAME_MODE_60P(g_info.ref_node[minor_idx]) && !g_info.ref_node[minor_idx]->pair_frame) {
            job_node_t  *prev_ref_node = g_info.ref_node[minor_idx]->refer_to;
            struct video_entity_job_t *job;

            /* step1: this job will not pair with top frame, thus we must set top frame as fail */
            g_info.ref_node[minor_idx]->status = JOB_STS_FLUSH;
            g_info.ref_node[minor_idx]->nr_splits = 0;

            if (prev_ref_node) {
                release_refered_node(prev_ref_node);
                if (debug_value) {
                    job = (struct video_entity_job_t *)prev_ref_node->private;
                    printm("DI", "STOP: minor: %d, release 60P job_id: %d \n", minor_idx, job->id);
                }
            }
        }

        /* C */
        release_refered_node(g_info.ref_node[minor_idx]);
        g_info.ref_node[minor_idx] = NULL;

        /* debug only */
        if (debug_value)
            printm("DI", "STOP1, minor=%d, nr_splits=%d, backup_split=%d\n", minor_idx,
                    parent_node->nr_splits, parent_node->nr_splits_keep);

        /* stop */
        ftdi220_stop_channel(parent_node->drv_chnum);

        /* child */
        list_for_each_entry(node, &parent_node->child_list, child_list)
            ftdi220_stop_channel(node->drv_chnum);

        if (debug_value)
            printm("DI", "STOP2, dev%d, minor=%d, nr_splits=%d, backup_split=%d, status:0x%x, jid:%d, ref_cnt:%d \n",
                        node_item->dev, minor_idx,
                        parent_node->nr_splits, parent_node->nr_splits_keep, node_item->status,
                        job->id, node_item->ref_cnt);
    }

    DRV_VGUNLOCK(flags);

#ifdef USE_KTHREAD
    ftdi220_cb_wakeup();
#else
    /* schedule the delay workq */
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)ftdi220_callback_process);
    queue_delayed_work(g_info.workq, &process_callback_work, callback_period);
#endif

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
    unsigned int len = 0, pos, begin = 0;

    DRV_VGLOCK(flags);

    if (off == 0) {
        len += sprintf(page + len, "\n%s engine:%d minor:%d job:%d\n",MODULE_NAME,
            property_engine,property_minor,property_record[idx].job_id);
        len += sprintf(page + len, "=============================================================\n");
        len += sprintf(page + len, "CH  ID  Name(string) Value(hex)  Readme\n");
    }

    do {
        id=property_record[idx].property[i].id;
        if(id==ID_NULL)
            break;

        pos = begin + len;
        if (pos < off) {
            len = 0;
            begin = pos;
        }

        if (pos > off + count)
            goto exit;

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

exit:
    /* unlock */
    DRV_VGUNLOCK(flags);

    *start = page + (off - begin);
    len -= (off - begin);

    if (len > count)
        len = count;
    else if (len < 0)
        len = 0;

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

    for (chnum = 0; chnum < ENTITY_MINORS; chnum ++) {
		DRV_VGLOCK(flags);
        if (list_empty(&chan_job_list[chnum])) {
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
    int i, chnum, engine, minor;
    unsigned long flags;
    struct video_entity_job_t *job = NULL;
    job_node_t  *parent_node = NULL, *child_node, *node;
    int len2 = *len;
    char *str, *path_str[] = {"normal", "di_sharebuf"};

    DRV_VGLOCK(flags);
    for (i = 0; i < ENTITY_MINORS; i ++) {
        if (!ftdi220_joblist_cnt(i))
            continue;

        len2 += sprintf(page + len2, "minor: %-2d, job count: %d \n", i, ftdi220_joblist_cnt(i));
        list_for_each_entry(node, &chan_job_list[i], list) {
            job = (struct video_entity_job_t *)node->private;
            str = (node->di_sharebuf == 1) ? path_str[1] : path_str[0];
            len2 += sprintf(page + len2, "  ===>drvch:%d, sts:0x%x, jid:%d, split:%d(%d), seqid:%d, cmd:0x%x, refer_cnt:%d, %s, feature:0x%x, src_fld: %d\n",
                    node->drv_chnum, node->status, job->id, node->nr_splits, node->nr_splits_keep,
                    node->job_seqid, node->chan_param.command, atomic_read(&node->ref_cnt), str, node->chan_param.feature, node->chan_param.src_field);
        }
    }

    /* print nodes in channel list */
    for (chnum = 0; chnum < ENTITY_MINORS; chnum ++) {
        parent_node = g_info.ref_node[chnum];
        if (parent_node == NULL)
            continue;

        job = (struct video_entity_job_t *)parent_node->private;
        engine = ENTITY_FD_ENGINE(job->fd);
        minor = ENTITY_FD_MINOR(job->fd);
        str = (parent_node->di_sharebuf == 1) ? path_str[1] : path_str[0];
        len2 += sprintf(page + len2, "refer node: drvch:%d, status:0x%x, eng:%d, minor:%d, jid:%d, split:%d(%d), seqid:%d, cmd:0x%x, c_cmd:0x%x, refer_cnt:%d, %s\n",
                parent_node->drv_chnum, parent_node->status, parent_node->dev, minor, job->id, parent_node->nr_splits,
                parent_node->nr_splits_keep, parent_node->job_seqid, parent_node->chan_param.command, parent_node->collect_command, atomic_read(&parent_node->ref_cnt), str);
        /* child */
        list_for_each_entry(child_node, &parent_node->child_list, child_list) {
            len2 += sprintf(page + len2, "  --->child: drvch:%d, status:0x%x, cmd:0x%x \n",
                child_node->drv_chnum, child_node->status, child_node->chan_param.command);
        }
    }

    len2 += sprintf(page + len2, "memory: minor_node_count: %d\n", atomic_read(&g_info.vg_node_count));
    len2 += sprintf(page + len2, "memory: alloc_node_count: %d\n", atomic_read(&g_info.alloc_node_count));
    len2 += sprintf(page + len2, "version change: %d(vg:%d)\n", atomic_read(&g_info.ver_chg_count),
                                                                    atomic_read(&g_info.vg_chg_count));
    len2 += sprintf(page + len2, "vg stop count: %d\n", atomic_read(&g_info.stop_count));
    len2 += sprintf(page + len2, "hybrid job count: %d \n", atomic_read(&g_info.hybrid_job_count));
    len2 += sprintf(page + len2, "putjob fail: %d, callback fail: %d \n", atomic_read(&g_info.put_fail_count),
                                                                    atomic_read(&g_info.cb_fail_count));
    len2 += sprintf(page + len2, "free drv chnum: %d, wrong field: %d \n", get_free_drvchum(), atomic_read(&g_info.wrong_field_count));
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

    /* register video entify */
    video_entity_register(&ftdi220_entity);

    /* function array registers to video graph */
    memset(&driver_entity[0], 0, ENTITY_MINORS * sizeof(struct video_entity_t));

    /* spinlock */
    spin_lock_init(&g_info.lock);
    /* semaphore for child_list protection */
    sema_init(&g_info.sema, 1);

    /* init list head of each channel */
    for (i = 0; i < ENTITY_MINORS; i ++) {
        INIT_LIST_HEAD(&chan_job_list[i]);
        INIT_LIST_HEAD(&drv_chnum_list[i]);
        expect_field[i] = SRC_FIELD_TOP;
    }

    /* driver layer channel init */
    for (i = 0; i < MAX_CHAN_NUM; i ++) {
        drv_chnum_db[i].drv_chnum = i;
        drv_chnum_db[i].vch = -1;
    }

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

#ifdef USE_KTHREAD
    init_waitqueue_head(&cb_thread_wait);
    cb_thread = kthread_create(ftdi220_cb_thread, NULL, "ftdi220_thread");
    if (IS_ERR(cb_thread))
        panic("%s, create cb_thread fail ! \n", __func__);
    wake_up_process(cb_thread);
#endif /* USE_KTHREAD */

    printk("\nFTDI220 registers %d entities to videograph. \n", ENTITY_MINORS);

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

#ifdef USE_KTHREAD
    if (cb_thread) {
        cb_thread_reset = 1;
        ftdi220_cb_wakeup();
        kthread_stop(cb_thread);
        /* wait thread to be terminated */
        while (cb_thread_ready) {
            msleep(10);
        }
    }
#endif

    kfree(property_record);
    kmem_cache_destroy(g_info.job_cache);
}

#ifdef USE_KTHREAD
static int ftdi220_cb_thread(void *data)
{
    int status;

    if (data) {}

    /* ready */
    cb_thread_ready = 1;

    do {
        status = wait_event_timeout(cb_thread_wait, cb_wakeup_event, msecs_to_jiffies(50*1000));
        if (status == 0)
            continue;   /* timeout */
        cb_wakeup_event = 0;
        /* callback process */
        ftdi220_callback_process();
        if (cb_thread_reset)
            msleep(500);
    } while(!kthread_should_stop());

    cb_thread_ready = 0;
    return 0;
}

static void ftdi220_cb_wakeup(void)
{
    if (debug_value)
        printm("DI", "ftdi220_cb_wakeup.... \n");

    cb_wakeup_event = 1;
    wake_up(&cb_thread_wait);
}
#endif /* USE_KTHREAD */




