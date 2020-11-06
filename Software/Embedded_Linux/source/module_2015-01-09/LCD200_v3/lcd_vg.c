#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/semaphore.h>
#include "frame_comm.h"
#include "proc.h"
#include "log.h"    //include log system "printm","damnit"...
#include "video_entity.h"   //include video entity manager
#include "lcd_vg_map.c"

#if (HZ == 1000)
    #define get_gm_jiffies()            (jiffies)
#else
    #include <mach/gm_jiffies.h>
#endif

#ifdef LCD_DEV2
#define DRIVER_TYPE     TYPE_DISPLAY2
#define DRIVER_NAME     "lcd2vg"
#elif defined LCD_DEV1
#define DRIVER_TYPE     TYPE_DISPLAY1
#define DRIVER_NAME     "lcd1vg"
#else
#define DRIVER_TYPE     TYPE_DISPLAY0
#define DRIVER_NAME     "lcd0vg"
#endif

#define MODULE_NAME         "LC"    //(two bytes)
#define ENTITY_ENGINES      1   /* from videograph view */
#define ENTITY_MINORS       1

#define DRV_LOCK(flags)     spin_lock_irqsave(&g_info.lock, flags)
#define DRV_UNLOCK(flags)   spin_unlock_irqrestore(&g_info.lock, flags)

#define LOG_ERROR     0
#define LOG_WARNING   1
#define LOG_DEBUG     2

int bf = 0;  //play bottom field first in two frames mode
module_param(bf, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(bf, "bottom field first");

int d1_3frame = 0;
module_param(d1_3frame, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(d1_3frame, "D1 in 3frame");

int gui_clone_fps = 10; /* number of cloned frames per second */
module_param(gui_clone_fps, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(gui_clone_fps, "gui clone fps");

/*************************************************************************************
 * Local Varaiables
 *************************************************************************************
 */
static u32 framerate_numerator = 0, framerate_denominator = 0, framerate_skip;
static int vg_kick_off = 0;
int lcdvg_dbg = 0;
/* scheduler */
static DECLARE_DELAYED_WORK(process_callback_work, 0);
static unsigned int g_jobid = 0;
/* proc system */
static struct proc_dir_entry *entity_proc = NULL, *infoproc = NULL, *cbproc = NULL;
static struct proc_dir_entry *propertyproc = NULL, *jobproc = NULL, *filterproc = NULL, *levelproc = NULL;
static struct proc_dir_entry *minor_framerate_proc = NULL;
static struct proc_dir_entry *max_threshold_proc = NULL;
static int max_qjob_threshold = 255;

static unsigned int callback_period = 0;    //ticks
static int minor_frame_rate = 0;
struct work_struct  gui_clone_work;

/* log & debug message */
#define MAX_FILTER  5
static unsigned int log_level = LOG_ERROR; //LOG_DEBUG;
static int include_filter_idx[MAX_FILTER]; //init to -1, ENGINE << 16 | MINOR
static int exclude_filter_idx[MAX_FILTER]; //init to -1, ENGINE << 16 | MINOR

#define MAX_RECORD 20
/* property array */
struct property_record_t {
    struct video_property_t property[MAX_RECORD];
    struct video_entity_t   *entity;
    int    job_id;
};

/* job status */
typedef enum {
    JOB_STS_QUEUE   = 0x1,      //not process yet
    JOB_STS_ONGOING = 0x2,
    JOB_STS_DONE    = 0x4,
    JOB_STS_FLUSH   = 0x8
} job_status_t;

/* input property */
typedef struct {
    int     src_frame_rate;
    int     videograph_plane;
    int     dst_dim_width;  //width<<16|height
    int     dst_dim_height; //width<<16|height
    int     src_bg_dim_width;   //width<<16|height
    int     src_bg_dim_height;  //width<<16|height
    int     src2_bg_dim_width;    //width<<16|height
    int     src2_bg_dim_height;   //width<<16|height
    int     src_dim_width;  //width<<16|height
    int     src_dim_height; //width<<16|height
    int     src_xy_x;       //x<<16|y
    int     src_xy_y;       //x<<16|y
    int     src_fmt;        //YUV422_FIELDS, YUV422_FRAME, YUV422_TWO_FRAMES
    int     change_flag;    //1 means the property change. 0 for not.
} in_prop_t;

typedef struct lcd_node_s {
    void                *private;
    struct list_head    list;   /* job list */
    job_status_t        status;
    int                 in_wlist;
    /* input property */
    in_prop_t           in_prop;
    int                 callback;       //whether the job can callback
    u32                 frame_addr;     //the physical address to display
    u32                 seq_id;         //sequence id
    int                 countdown;      //how many frames to wait before returning to videograph
    struct lcd_node_s   *parent;        //points to itself at the init time
    struct lcd_node_s   *sibling;       //NULL if no sibling exits
    unsigned int        puttime;        //putjob time
    unsigned int        starttime;      //start engine time
    unsigned int        finishtime;     //finish engine time
    unsigned int        d1_frame_addr;
} lcd_node_t;

/*
 * Global structures
 */
struct g_info_s {
    struct workqueue_struct *workq;
    struct kmem_cache   *job_cache;
    spinlock_t          lock;
    lcd_node_t          *cur_node;
    struct list_head    qlist;      /* job list */
    struct list_head    wlist;      /* wait to callback list */
    struct list_head    cblist;     /* job callback list */
    int                 qnum;
    int                 wnum;
    int                 cbnum;
    void                *dev_info;
    in_prop_t           in_prop;    /* a property job which is used to keep the current setting */
    int                 mem_alloc;
    u32                 put_job_cnt;
    u32                 stop_cnt;
    u32                 cb_job_cnt;
    u32                 empty_job_cnt;
    void                *ref_clk_priv;
} g_info;

typedef enum {
    FRAME_NORMAL = 0,
    FRAME_DUMMY,
    FRAME_DROP
} frame_status_t;

/*************************************************************************************
 * External Variables
 *************************************************************************************
 */
/*************************************************************************************
 * Local Functions and variables
 *************************************************************************************
 */
u32 put_3frame_fail_cnt = 0, put_scalerjob_ok_cnt = 0, put_scalerjob_fail_cnt = 0;
static int lcdvg_get_fbplane(void);
#define vg_check_blend_value(v)  ((v>0x1f)?(0x1f):v)
#define vg_check_priority_value(v)  ((v>2)?(2):v)
static RET_VAL_T lcdvg_process_job(struct lcd200_dev_info *dev_info, int plane_idx);
static RET_VAL_T lcdvg_get_3frame_d1job(struct lcd200_dev_info *dev_info, int plane_idx);
static void lcdvg_flush_alljob(void);
static void lcdvg_driver_clearnup(void);
static void lcdvg_inres_change(void);
static lcd_node_t *lcdvg_alloc_node_and_init(void *private, int init_state);
static void lcdvg_dealloc_node(void *memory);
static void callback_scheduler(void);
static int driver_stop(void *parm, int engine, int minor);
static int driver_preprocess(lcd_node_t *job_item, void *priv);
static int driver_postprocess(lcd_node_t *job_item, void *priv);
static int driver_set_out_property(void *param, struct video_entity_job_t *job);
static frame_status_t lcdvg_check_framerate(struct ffb_dev_info *fdev_info, lcd_node_t *node);
static void lcdvg_gui_clone_workfn(struct work_struct *work);

/* property lastest record */
struct property_record_t *property_record;
static int vg_panic = 0;
struct video_entity_t lcd_entity;

/* Handle last job, to keep buffer to prevent dirty frame by WayneWei*/
void *rev_handle = NULL;
void *lcd_playing_job = NULL;

#ifdef USE_KTRHEAD
static int lcdvg_cb_thread(void *data);
static int lcdvg_cb_wakeup(void);
static struct task_struct *cb_thread = NULL;
static wait_queue_head_t cb_thread_wait;
static cb_wakeup_event = 0;
static volatile int cb_thread_ready = 0, cb_thread_reset = 0;
#endif /* USE_KTRHEAD */

/*************************************************************************************
 * Functions Body
 *************************************************************************************
 */
static int to_string(int val)
{
    int ret;

    switch (val) {
      case JOB_STS_QUEUE:
        ret = 0;
        break;
      case JOB_STS_ONGOING:
        ret = 1;
        break;
      case JOB_STS_DONE:
        ret = 2;
        break;
      case JOB_STS_FLUSH:
      default:
        ret = 3;
        break;
    }
    return ret;
}

int is_print(int engine, int minor)
{
    return 1;
}

#define DEBUG_M(level, engine, minor, fmt, args...) { \
    if ((log_level >= level) && is_print(engine,minor)) \
        printm(MODULE_NAME,fmt, ## args); }

#define DEBUG_M2(level, fmt, args...) { \
    if ((log_level >= level)) \
        printm(MODULE_NAME,fmt, ## args); }

int lcdvg_panic_notifier(int data)
{
    if (data) {}

    return 0;
}

int lcdvg_print_notifier(int data)
{
    struct video_entity_job_t *job;
    lcd_node_t  *job_item;
    char *st_string[] = {"STANDBY","ONGOING"," FINISH","FLUSH"};

    if (data) {}

    list_for_each_entry(job_item, &g_info.qlist, list) {
        printm(MODULE_NAME, "LCD%d: qlist => job->id:%d, status:%s \n", LCD200_ID,
            st_string[to_string(job_item->status)]);
    }

    if (g_info.cur_node) {
        job_item = g_info.cur_node;
        job = (struct video_entity_job_t *)job_item->private;
        printm(MODULE_NAME, "LCD%d: curnode=> job->id:%d \n", LCD200_ID, job->id);
    }

    list_for_each_entry(job_item, &g_info.wlist, list) {
        job = (struct video_entity_job_t *)job_item->private;
        printm(MODULE_NAME, "LCD%d: wlist => job->id:%d, status:%s \n", LCD200_ID, job->id,
            st_string[to_string(job_item->status)]);
    }

    list_for_each_entry(job_item, &g_info.cblist, list) {
        job = (struct video_entity_job_t *)job_item->private;
        printm(MODULE_NAME, "LCD%d: cblist => job->id:%d, status:%s \n", LCD200_ID, job->id,
            st_string[to_string(job_item->status)]);
    }

    return 0;
}

/*
 * callback functions for LCD, LCD2
 */
static struct pip_vg_cb callback_fn[] = {
    {
        .lcd_idx = LCD_ID,
        .process_frame = lcdvg_process_job,
        .flush_all_jobs = lcdvg_flush_alljob,
        .driver_clearnup = lcdvg_driver_clearnup,
        .res_change = lcdvg_inres_change,
    },
    {
        .lcd_idx = LCD1_ID,
        .process_frame = lcdvg_process_job,
        .flush_all_jobs = lcdvg_flush_alljob,
        .driver_clearnup = lcdvg_driver_clearnup,
        .res_change = lcdvg_inres_change,
    },
    {
        .lcd_idx = LCD2_ID,
        .process_frame = lcdvg_process_job,
        .flush_all_jobs = lcdvg_flush_alljob,
        .driver_clearnup = lcdvg_driver_clearnup,
        .res_change = lcdvg_inres_change,
    },
};

/*
 * callback functions for LCD, LCD2
 */
static struct pip_vg_cb callback_3frame_fn[] = {
    {
        .lcd_idx = LCD_ID,
        .process_frame = lcdvg_get_3frame_d1job,
    },
    {
        .lcd_idx = LCD1_ID,
        .process_frame = lcdvg_get_3frame_d1job,
    },
    {
        .lcd_idx = LCD2_ID,
        .process_frame = lcdvg_get_3frame_d1job,
    },
};

int lcdvg_summary_callback(proc_lcdinfo_t *lcdinfo)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    struct ffb_dev_info    *dev_info = (struct ffb_dev_info*)info;
    struct ffb_info *fbi;
    struct ffb_timing_info *timing;
    ffb_output_name_t   *output_str;
    int i, video_plane = g_info.in_prop.videograph_plane;

    if (lcdinfo == NULL) {
        printk("%s, null pointer of lcdinfo! \n", __func__);
        return -1;
    }

    down(&dev_info->dev_lock);

    memset(lcdinfo, 0, sizeof(proc_lcdinfo_t));
    if (dev_info->output_info == NULL) {
        printk("%s, null output_info pointer, dev_info->video_output_type: %d, output_type:%d! \n",
                                                __func__, dev_info->video_output_type, output_type);
        up(&dev_info->dev_lock);
        return -1;
    }

    timing = get_output_timing(dev_info->output_info, dev_info->video_output_type);
    if (timing == NULL) {
        int i;
        struct OUTPUT_INFO *output = dev_info->output_info;

        printk("%s, null timing pointer, dev_info->video_output_type: %d, output_type: %d! \n",
                                                __func__, dev_info->video_output_type, output_type);

        printk("%s, output->timing_num: %d \n", __func__, output->timing_num);
        for (i = 0; i < output->timing_num; i ++) {
            printk("%d, otype = %d \n", i, output->timing_array[i].otype);
        }

        up(&dev_info->dev_lock);
        return -1;
    }
    lcdinfo->lcd_idx = LCD200_ID;
    lcdinfo->vg_type = DRIVER_TYPE;

    /* input resolution
     */
    if (dev_info->input_res == NULL) {
        printk("%s, null input_res pointer, input_res:%d! \n",  __func__, input_res);
        up(&dev_info->dev_lock);
        return -1;
    }
    lcdinfo->input_res = dev_info->input_res->input_res;
    if (strlen(dev_info->input_res->name) < sizeof(lcdinfo->input_res_desc))
        strcpy(lcdinfo->input_res_desc, dev_info->input_res->name);

    /* get resolution */
    if (dev_info->video_output_type >= VOT_CCIR656) {
        lcdinfo->in_x = timing->data.ccir656.in.xres;
        lcdinfo->in_y = timing->data.ccir656.in.yres;
    } else {
        lcdinfo->in_x = timing->data.lcd.in.xres;
        lcdinfo->in_y = timing->data.lcd.in.yres;
    }

    /* output information
     */
    lcdinfo->output_type = output_type;
    output_str = ffb_get_output_name(output_type);
    if (output_str == NULL) {
        printk("%s, null timing pointer, output_type: %d \n", __func__, output_type);
        up(&dev_info->dev_lock);
        return -1;
    }

    if (strlen(output_str->name) < sizeof(lcdinfo->output_type_desc))
        strcpy(lcdinfo->output_type_desc, output_str->name);
    /* get output resolution */
    if (dev_info->video_output_type >= VOT_CCIR656) {
        lcdinfo->out_x = timing->data.ccir656.out.xres;
        lcdinfo->out_y = timing->data.ccir656.out.yres;
    } else {
        lcdinfo->out_x = timing->data.lcd.out.xres;
        lcdinfo->out_y = timing->data.lcd.out.yres;
    }

    lcdinfo->engine_minor = ENTITY_FD(0, ENTITY_ENGINES, ENTITY_MINORS);
    /* 3 is forced over 300 or 600, it is a stuipd workaround */
    lcdinfo->hw_frame_rate = (dev_info->frame_rate + 3) / FRAME_RATE_GRANULAR;

    if (video_plane == -1)
        video_plane = 0; /* default */

    lcdinfo->fb_paddr = (dma_addr_t)info->fb[video_plane]->ppfb_dma[0];
    lcdinfo->fb_vaddr = (void *)info->fb[video_plane]->ppfb_cpu[0];

    /* for GUI info */
    for (i = 0; i < PIP_NUM; i ++) {
        if (info->fb[i] == NULL)
            continue;

        fbi = info->fb[i];
        lcdinfo->gui_info.gui[i].xres = dev_info->input_res->xres;
        lcdinfo->gui_info.gui[i].yres = dev_info->input_res->yres;
        lcdinfo->gui_info.gui[i].buff_len = fbi->fb.fix.smem_len;
        lcdinfo->gui_info.gui[i].format = fbi->video_input_mode;
        lcdinfo->gui_info.gui[i].phys_addr = fbi->ppfb_dma[0];
        lcdinfo->gui_info.gui[i].bits_per_pixel = fbi->fb.var.bits_per_pixel;
        lcdinfo->gui_info.gui[i].active = 1;
    }
    lcdinfo->gui_info.PIP_En = g_dev_info.PIP_En;
    lcdinfo->lcd_disable = lcd_disable & 0x1;

    up(&dev_info->dev_lock);

    return 0;
}

/* reset frame rate to initial value
 */
void lcdvg_reset_framerate(void)
{
    framerate_numerator = 0;
    framerate_denominator = 0;
    framerate_skip = 0;
}

int lcdvg_apply_new_param(lcd_node_t *node)
{
    in_prop_t *in_prop  = &node->in_prop;

    if (in_prop->change_flag == 0)
        return 0;   /* do nothing */

    in_prop->change_flag = 0;

    /* start to process the parameters */

    /* currently, only frame is necessary, others are not.
     */
    lcdvg_reset_framerate();

    return 0;
}

/*
 * Remove all flushable jobs.
 */
void remove_flush_jobs_all(void)
{
    unsigned long flags;
	lcd_node_t *node, *ne;

    DRV_LOCK(flags);

	/* check if any node is flushable.
     * If yes, give them back ASAP.
     * If no, the job is a new one to play.
     */
    list_for_each_entry_safe(node, ne, &g_info.qlist, list) {
		/* update new parameters even this job is flushable */
		lcdvg_apply_new_param(node);

		if (node->status & JOB_STS_FLUSH) {
			list_del_init(&node->list);
            if (node->callback) {
				list_add_tail(&node->parent->list, &g_info.wlist);
                g_info.wnum ++;
            }
            g_info.qnum --;
        } else {
            break;
        }
	}
	DRV_UNLOCK(flags);
}

/*
 * This function is only valid when the LCD is a main screen role. Otherwise lcd_gui_bitmap will be
 *  zero.
 */
void lcdvg_process_gui_clone(struct lcd200_dev_info *dev_info)
{
    static u32 gui_fps_countdown = 0;
    struct ffb_dev_info *info = (struct ffb_dev_info *)dev_info;
    u32 tmp;
    int lcd_idx, xpos, ypos;

    /* exclude myself */
    lcd_gui_bitmap &= ~(0x1 << LCD200_ID);

    if (lcd_gui_bitmap == 0)
        return;

    if (g_dev_info.PIP_En == 0)
        return;

    if (gui_clone_fps == 0)
        return;

    tmp = dev_info->ops->reg_read(info, LCD_CURSOR_POS);
    xpos = (tmp >> 16) & 0xFFF;
    ypos = tmp & 0xFFF;

    for (lcd_idx = 0; lcd_idx < LCD_IP_NUM; lcd_idx ++) {
        if (!test_bit(lcd_idx, (void *)&lcd_gui_bitmap))
            continue;
        //set the target
        /* 2014/12/22 04:07¤U¤È. When scaling is enabled, xpos and ypos adopts the scaled position. Thus we must use output res. */
        lcd_update_cursor_info(lcd_idx, info->output_info->timing_array->data.lcd.out.xres,
                                info->output_info->timing_array->data.lcd.out.yres,
                                xpos, ypos, (void *)&gui_clone_adjust);
    }

    if (gui_fps_countdown == 0) {
        gui_fps_countdown = (info->hw_frame_rate / 10) / gui_clone_fps;
        queue_work(g_info.workq, &gui_clone_work);
    }

    gui_fps_countdown --;
}

/*
 * Process videograph job. Called from PIP ISR
 */
RET_VAL_T lcdvg_process_job(struct lcd200_dev_info *dev_info, int plane_idx)
{
    u32 phy_addr = RET_NO_NEW_FRAME;
    lcd_node_t   *cur_node, *node, *ne, *play_node;
    int callback = 0, fbnum;
    frame_status_t  frame_status, log_frame_status;
    struct video_entity_job_t *job;
    unsigned int play_jid = 0;

    fbnum = lcdvg_get_fbplane();
    if (fbnum == -1)            /* LCD desn't execute video graph */
        return RET_NOT_VGPLANE;

    /* this plane is a video graph plane?
     */
    if (fbnum != plane_idx)
        return RET_NOT_VGPLANE;

    /* do nothing */
    if (vg_panic == 1)
        return RET_NO_NEW_FRAME;

    lcdvg_process_gui_clone(dev_info);

    /* this node is in hardware already due to vsync coming*/
    if (g_info.cur_node)
        g_info.cur_node->finishtime = jiffies;


    /* move from wlist to cblist */
    if (!list_empty(&g_info.wlist)) {
		node = list_entry(g_info.wlist.next, lcd_node_t, list);
		if ((-- node->countdown) <= 0) {
			list_for_each_entry_safe(node, ne, &g_info.wlist, list) {
				list_move_tail(&node->list, &g_info.cblist);
				g_info.cbnum ++;
				g_info.wnum --;
				callback = 1;
			}
		}
    }
    play_node = cur_node = g_info.cur_node;

    /* if it is flushable, return it ASAP */
	if (cur_node != NULL) {
		if (cur_node->status & JOB_STS_FLUSH) {
			if (cur_node->callback) {
			    if (cur_node->parent->in_wlist)
                    panic("%s, 1 \n", __func__);

				list_add_tail(&cur_node->parent->list, &g_info.wlist);
                cur_node->parent->in_wlist = 1;

				cur_node->countdown = 0;
				g_info.wnum ++;
				g_info.cur_node = cur_node = NULL;
			}
		}
	}

    /* check if the last frame needs to be played again due to frame rate control */
    if (g_info.qnum) {
        frame_status = lcdvg_check_framerate((struct ffb_dev_info *)dev_info,
                                        list_entry(g_info.qlist.next, lcd_node_t, list));
    } else {
        frame_status = lcdvg_check_framerate((struct ffb_dev_info *)dev_info, cur_node);
    }
    log_frame_status = frame_status;

    if (frame_status == FRAME_DUMMY) {
        goto exit;  /* do nothing */
    }

	if (unlikely(!g_info.qnum)) {
#if 0
		if (vg_kick_off && printk_ratelimit())
            printk(KERN_DEBUG "LCD-%d: job queue is empty!!! \n", dev_info->lcd200_idx);
#endif
        if (vg_kick_off) {
		    g_info.empty_job_cnt ++;
		    video_entity_notify(&lcd_entity, ENTITY_FD(lcd_entity.type, 0, 0), VG_NO_NEW_JOB, 1);
		}

		goto exit;
	}

	/* return the g_info.cur_node */
	if (g_info.cur_node && g_info.cur_node->callback) {
	    if (g_info.cur_node->parent->in_wlist)
            panic("%s, 2 \n", __func__);

		list_add_tail(&g_info.cur_node->parent->list, &g_info.wlist);
		g_info.cur_node->parent->in_wlist = 1;
		g_info.wnum ++;
	}
	g_info.cur_node = cur_node = NULL;

	list_for_each_entry_safe(node, ne, &g_info.qlist, list) {
		list_del_init(&node->list);
        g_info.qnum --;
		/* check if the parameters change */
        lcdvg_apply_new_param(node);

		if (node->status & JOB_STS_FLUSH) {
			if (node->callback) {
			    if (node->parent->in_wlist)
			        panic("%s, 3 \n", __func__);

    			list_add_tail(&node->parent->list, &g_info.wlist);
    			node->parent->in_wlist = 1;
    			node->countdown = 0;
    			g_info.wnum ++;
    		}

    		if (unlikely(!g_info.qnum)) {
#if 0
    		    if (vg_kick_off && printk_ratelimit())
                    printk(KERN_DEBUG "LCD-%d: Job queue is empty due to flush! \n",
                                                                dev_info->lcd200_idx);
#endif
                if (vg_kick_off) {
                    g_info.empty_job_cnt ++;
                    video_entity_notify(&lcd_entity, ENTITY_FD(lcd_entity.type, 0, 0), VG_NO_NEW_JOB, 1);
                }
                goto exit;
    		}
			continue;
		}

		if (frame_status == FRAME_DROP) {
			if (unlikely(!g_info.qnum)) {
#if 0
			    if (vg_kick_off && printk_ratelimit())
                    printk(KERN_DEBUG "LCD-%d: Need to drop frame but job queue is empty!!! \n",
                                dev_info->lcd200_idx);
#endif
                if (vg_kick_off) {
                    g_info.empty_job_cnt ++;
                    video_entity_notify(&lcd_entity, ENTITY_FD(lcd_entity.type, 0, 0), VG_NO_NEW_JOB, 1);
                }

				play_node = node;
				goto exit2;
			}

			if (node->callback) {
			    if (node->parent->in_wlist)
                    panic("%s, 4 \n", __func__);

                /* added to waiting list */
                list_add_tail(&node->parent->list, &g_info.wlist);
                node->parent->in_wlist = 1;
                g_info.wnum ++;

                if (lcdvg_dbg) {
                    job = (struct video_entity_job_t *)node->private;
        			printm(MODULE_NAME, "LCD%d: job->id: %d is dropped! \n", LCD200_ID, job->id);
                }
            }
		}

		if (frame_status == FRAME_NORMAL) {
			play_node = node;
			break;
		}

		frame_status = lcdvg_check_framerate((struct ffb_dev_info *)dev_info, node);
	}

//	if (play_node == NULL)
//		panic("LCD%d driver bug! \n", dev_info->lcd200_idx);

exit2:
    job = (struct video_entity_job_t *)play_node->private;
    lcd_playing_job = (void *)job;

	if (lcdvg_dbg)
	    printm(MODULE_NAME, "LCD%d: play job->id: %d\n", LCD200_ID, job->id);

    g_info.cur_node = play_node;
    play_node->status = JOB_STS_DONE;
    play_node->starttime = jiffies;
    phy_addr = play_node->frame_addr;
    if (play_node->callback)
        driver_preprocess(play_node, NULL);

    if (play_node->d1_frame_addr && lcd_frameaddr_add(play_node->d1_frame_addr))
        put_3frame_fail_cnt ++;
exit:
    /*
     * check again to see any flushable job inside
     */
    if (!list_empty(&g_info.wlist)) {
        list_for_each_entry_safe(node, ne, &g_info.wlist, list) {
            if (node->countdown > 0)
                break;
            list_move_tail(&node->list, &g_info.cblist);
            g_info.cbnum ++;
            g_info.wnum --;
            callback = 1;
        }
    }

    if (callback) {
#ifdef USE_KTRHEAD
        lcdvg_cb_wakeup();
#else
        PREPARE_DELAYED_WORK(&process_callback_work, (void *)callback_scheduler);
        queue_delayed_work(g_info.workq, &process_callback_work, callback_period);
#endif
    }

    /* notify the videograph about this job finish time even the duplicated displayed job */
    if (g_info.ref_clk_priv) {
        video_process_tick(g_info.ref_clk_priv, get_gm_jiffies(), play_node ? play_node->private : NULL);
    }




    if (play_node)
        play_jid = ((struct video_entity_job_t *)play_node->private)->id;

    if (lcdvg_dbg)
	    printm(MODULE_NAME, "LCD%d: frame_status(%d) jid(%d) phy_addr(0x%x)\n", LCD200_ID,
	         log_frame_status, play_jid, phy_addr);

    return phy_addr;
}

/* get d1 frame base address in 3 frame */
RET_VAL_T lcdvg_get_3frame_d1job(struct lcd200_dev_info * dev_info, int plane_idx)
{
    int fbnum;
    u32 phy_addr;

    fbnum = lcdvg_get_fbplane();
    if (fbnum == -1)            /* LCD desn't execute video graph */
        return RET_NOT_VGPLANE;

    /* this plane is a video graph plane?
     */
    if (fbnum != plane_idx)
        return RET_NOT_VGPLANE;

    phy_addr = lcd_frameaddr_get();
    if (phy_addr == 0)
        return RET_NO_NEW_FRAME;

    return phy_addr;
}

void lcdvg_flush_alljob(void)
{
    driver_stop(NULL, 0, 0);
}

/* Return value 0 for normal job displaying, others for drop
 * fdev_info->hw_frame_rate is real hardware frame rate. ex: 30fps
 * formula: src_frame_rate * framerate_numerator compare to hw_frame_rate * framerate_denominator
 */
static frame_status_t lcdvg_check_framerate(struct ffb_dev_info *fdev_info, lcd_node_t *node)
{
    u32 num1, num2;
    frame_status_t retVal;
    int src_frame_rate;

    /* not start yet or stop() is called */
    if (node == NULL) {
        lcdvg_reset_framerate();
        return FRAME_NORMAL;
    }

    if (node)
        lcdvg_apply_new_param(node);

    src_frame_rate = node->in_prop.src_frame_rate * FRAME_RATE_GRANULAR + minor_frame_rate;
    num1 = src_frame_rate * framerate_numerator;
    num2 = fdev_info->hw_frame_rate * framerate_denominator;

    if (src_frame_rate <= fdev_info->hw_frame_rate) {
        if (num1 >= num2) {
            retVal = FRAME_NORMAL;
            framerate_denominator++;
        } else {
            framerate_skip++;
            retVal = FRAME_DUMMY;
        }

        /* count per interrupt */
        framerate_numerator++;
    } else {
        /* number of jobs > hardware frame rate */
        if (num2 >= num1) {
            retVal = FRAME_NORMAL;
            framerate_numerator++;
        } else {
            framerate_skip++;
            retVal = FRAME_DROP;
        }

        /* count per interrupt */
        framerate_denominator++;
    }

    /* boundary check */
    if ((num1 >= (0xFFFFFFFF/src_frame_rate)) || (num2 >= (0xFFFFFFFF/fdev_info->hw_frame_rate)))
        lcdvg_reset_framerate();

#if 0
    if (vg_kick_off) {
        switch (retVal) {
          case FRAME_NORMAL:
            printk("normal \n");
            break;
          case FRAME_DUMMY:
            printk("dummy \n");
            break;
          case FRAME_DROP:
            printk("drop \n");
            break;
        }
    }
#endif
    return retVal;
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
            DEBUG_M(LOG_WARNING, engine, minor,"{%d,%d} job %d property:", engine, minor, job->id);
        DEBUG_M(LOG_WARNING, engine, minor,"  ID:%d, Value:%d\n",property[i].id, property[i].value);
    }
}

/*
 * param: private data
 */
static int driver_parse_in_property(void *parm, struct video_entity_job_t *job)
{
    int i = 0, idx = MAKE_IDX(ENTITY_MINORS, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
    struct video_property_t *property = job->in_prop;
    lcd_node_t  *job_node = (lcd_node_t *)parm;
    in_prop_t   *in_prop = &job_node->in_prop;

    if (idx != 0) {
        printk("LCD%d: VideoGraph bug! \n", LCD200_ID);
        printm(MODULE_NAME, "wrong entity index %d \n", idx);
        return -1;
    }

    //copy the last record to the current database and update these new changes
    memcpy(in_prop, &g_info.in_prop, sizeof(in_prop_t));

    /* fill up the input parameters */
    while (property[i].id != 0) {
        switch(property[i].id) {
          case ID_SRC_FRAME_RATE:
            in_prop->src_frame_rate = (int)property[i].value;
            break;
          case ID_VIDEOGRAPH_PLANE:
            in_prop->videograph_plane = (int)property[i].value;
            break;
          case ID_DST_DIM:
            in_prop->dst_dim_width = (int)EM_PARAM_WIDTH(property[i].value);
            in_prop->dst_dim_height = (int)EM_PARAM_HEIGHT(property[i].value);
            break;
          case ID_SRC_BG_DIM:
            in_prop->src_bg_dim_width = (int)EM_PARAM_WIDTH(property[i].value);
            in_prop->src_bg_dim_height = (int)EM_PARAM_HEIGHT(property[i].value);
            break;
          case ID_SRC2_BG_DIM:
            in_prop->src2_bg_dim_width = (int)EM_PARAM_WIDTH(property[i].value);
            in_prop->src2_bg_dim_height = (int)EM_PARAM_HEIGHT(property[i].value);
            break;
          case ID_SRC_DIM:
            in_prop->src_dim_width = (int)EM_PARAM_WIDTH(property[i].value);
            in_prop->src_dim_height = (int)EM_PARAM_HEIGHT(property[i].value);
            break;
          case ID_SRC_XY:
            in_prop->src_xy_x = (int)EM_PARAM_X(property[i].value);
            in_prop->src_xy_y = (int)EM_PARAM_Y(property[i].value);
            break;
          case ID_SRC_FMT:
            /* TYPE_YUV422_FRAME, TYPE_YUV422_2FRAMES, TYPE_YUV422_2FRAMES_FRAME */
            in_prop->src_fmt = (int)property[i].value;
            break;
          default:
            break;
        } // switch

        i ++;
    }

    if (memcmp(&g_info.in_prop, in_prop, sizeof(in_prop_t))) {
        in_prop->change_flag = 1;

        /* backup the current setting */
        memcpy(&g_info.in_prop, in_prop, sizeof(in_prop_t));
        /* reset the flag */
        g_info.in_prop.change_flag = 0;

        if (lcdvg_dbg)
            printm(MODULE_NAME, "LCD%d, job->id: %d are changed! \n", LCD200_ID, job->id);
    }

    /* top frame address */
    job_node->frame_addr = (u32)job->in_buf.buf[0].addr_pa;

    if ((g_info.in_prop.src_fmt == TYPE_YUV422_2FRAMES) ||
        (g_info.in_prop.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        u32 offset = g_info.in_prop.src_dim_width * g_info.in_prop.src_dim_height << 1; //yuv422

        /*
         * allocate the job again for the bottom
         */
        job_node->sibling = lcdvg_alloc_node_and_init(job, JOB_STS_QUEUE);
        if (job_node->sibling) {
            memcpy(&job_node->sibling->in_prop, in_prop, sizeof(in_prop_t));
            job_node->sibling->in_prop.change_flag = 0;  //prevent two jobs from rising change flag
            /* bottom frame */
            job_node->callback = 0; //update the callback status
            job_node->sibling->callback = 1;
            job_node->sibling->frame_addr = job_node->frame_addr + offset; //physical address
            job_node->sibling->parent = job_node;    //change the parent
            /* play bottom field first due to workaround */
            if (bf == 1) {
                u32 tmp;

                tmp = job_node->frame_addr;
                job_node->frame_addr = job_node->sibling->frame_addr;
                job_node->sibling->frame_addr = tmp;
            }
        }
    }

    if (LCD200_ID == LCD_ID) {
        u32 d1_addr_pa = 0;
        if (g_info.in_prop.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
            u32 top_frame_size = in_prop->src_bg_dim_width * in_prop->src_bg_dim_height * 2;
		    u32 bottom_frame_size = in_prop->src2_bg_dim_width * in_prop->src2_bg_dim_height * 2;

			d1_addr_pa = (u32) (job->in_buf.buf[0].addr_pa + (top_frame_size * 2) + bottom_frame_size);
    	}

    	if (g_info.in_prop.src_fmt == TYPE_YUV422_FRAME_2FRAMES) {
    		u32 top_frame_size = in_prop->src_bg_dim_width * in_prop->src_bg_dim_height * 2;
		    u32 bottom_frame_size = in_prop->src2_bg_dim_width * in_prop->src2_bg_dim_height * 2;

			d1_addr_pa = (u32) (job->in_buf.buf[0].addr_pa + (top_frame_size * 1) + bottom_frame_size);
    	}

        if (g_info.in_prop.src_fmt == TYPE_YUV422_FRAME_FRAME) {
    		u32 top_frame_size = in_prop->src_bg_dim_width * in_prop->src_bg_dim_height * 2;

			d1_addr_pa = (u32) (job->in_buf.buf[0].addr_pa + (top_frame_size * 1) + 0);
    	}

        job_node->d1_frame_addr = d1_addr_pa;
        if (job_node->sibling)
            job_node->sibling->d1_frame_addr = d1_addr_pa;
#if 0
        if (lcd_frameaddr_add(d1_addr_pa))
            put_3frame_fail_cnt ++;
#endif
    }

    property_record[idx].property[i].id = property_record[idx].property[i].value = 0;
    property_record[idx].entity = job->entity;
    property_record[idx].job_id = job->id;

    /* set the output property */
    driver_set_out_property(NULL, job);
    print_property(job, job->in_prop);

    return 0;
}

/*
 * param: private data
 */
static int driver_set_out_property(void *param, struct video_entity_job_t *job)
{
    int i = 0;
    struct video_property_t *property = job->out_prop;

    if (param)  {}

    property[i].id = ID_NULL;
    property[i].value = 0;    //data->xxxx

    print_property(job, job->out_prop);
    return 1;
}

/* VideoGraph related function
 */
static int driver_preprocess(lcd_node_t *job_item, void *priv)
{
    struct video_entity_job_t *job = (struct video_entity_job_t *)job_item->private;
    struct video_property_t     *property = job->in_prop;
    int i = 0, idx = MAKE_IDX(ENTITY_MINORS, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));

    while (property[i].id != 0) {
        if (i < MAX_RECORD) {
            property_record[idx].job_id = job->id;
            property_record[idx].property[i].id = property[i].id;
            property_record[idx].property[i].value = property[i].value;
        } else
            panic("LCD: The array is too small! \n");

        i ++;
    }

    return video_preprocess(job_item->private, NULL);
}

/* VideoGraph related function
 */
static int driver_postprocess(lcd_node_t *job_item, void *priv)
{
    return video_postprocess(job_item->private, NULL);
}

/* return the job to the videograph
 */
void callback_scheduler(void)
{
    unsigned long flags;
    lcd_node_t   *node;
    struct video_entity_job_t *job;
    int engine = 0, minor = 0;
    static int is_reserved = 0;
    /*<= Handle last job, to keep buffer to prevent dirty frame by WayneWei*/

    set_user_nice(current, -20);

    do {
        if (vg_panic == 1)
            return;

        /* protect the link list */
        DRV_LOCK(flags);

        if (list_empty(&g_info.cblist)) {
            /* release the irq */
            DRV_UNLOCK(flags);
            break;
        }
        /* take one from the list */
        node = list_entry(g_info.cblist.next, lcd_node_t, list);
        list_del_init(&node->list);
        g_info.cbnum --;

        /* release the irq */
        DRV_UNLOCK(flags);

        /* prepare to callback */
        job = (struct video_entity_job_t *)node->private;
        engine = ENTITY_FD_ENGINE(job->fd);
        minor = ENTITY_FD_MINOR(job->fd);

        if (node->status & JOB_STS_DONE) {
            job->status = JOB_STATUS_FINISH;

            if (lcdvg_dbg)
                printm(MODULE_NAME, "LCD%d, job->id: %d status %d callback\n",
                        LCD200_ID, job->id, job->status);
        } else {
            job->status = JOB_STATUS_FAIL;

            if (lcdvg_dbg)
                printm(MODULE_NAME, "LCD%d, job->id: %d status %d callback\n",
                        LCD200_ID, job->id, job->status);
        }

        driver_postprocess(node, NULL);
#if 1
        /* Handle current playing job, to keep buffer to prevent dirty frame by WayneWei*/
        DRV_LOCK(flags);
        if ((g_info.cur_node == NULL) && (lcd_playing_job == job)) {
            lcd_playing_job = NULL;
            DRV_UNLOCK(flags);
            if (is_reserved == 1) {
                video_free_buffer(rev_handle);
                rev_handle = NULL;
                is_reserved = 0;
            }
            if (rev_handle == NULL) {
                rev_handle = video_reserve_buffer(job, 0); //dir:0:input  1:output
                if (rev_handle == NULL) {
                    printm(MODULE_NAME, "Error to reserve buffer, job_id(0x%x)\n",
                                                      job->id);
                    printk("Error to reserve buffer, no free space!\n");
                    damnit(MODULE_NAME);
                    is_reserved = 0;
                } else {
                    is_reserved = 1;
                    if (lcdvg_dbg)
                        printm(MODULE_NAME, "LCD%d, video_reserve_buffer jobid: %d \n", LCD200_ID, job->id);
                }
            } else {
                printm(MODULE_NAME, "Error to multi-reserve buffer "
                                               "in LCD during handling last job. "
                                               "rev_handle(%p), is_reved(%d)\n",
                                               rev_handle, is_reserved);
                printk("Error to multi-reserve buffer,rev_handle(%p),is_revd(%d)\n",
                        rev_handle, is_reserved);
                damnit(MODULE_NAME);
            }
        } else {//<= is_last_job = TRUE
            DRV_UNLOCK(flags);
            if ((is_reserved == 1) && (job->status == JOB_STATUS_FINISH)) {
                video_free_buffer(rev_handle);
                rev_handle = NULL;
                is_reserved = 0;
                if (lcdvg_dbg)
                    printm(MODULE_NAME, "LCD%d, video_free_buffer.\n", LCD200_ID);
            }
        }//<= is_last_job = FALSE
#endif /* WayneWei */

        job->callback(job);

        g_info.cb_job_cnt ++;

        /* release job memory */
        if (node->sibling)
            lcdvg_dealloc_node((void *)node->sibling);

        lcdvg_dealloc_node((void *)node);
    } while(1);

    return;
}

/*
 * allocate a job memory and init its default value
 */
lcd_node_t *lcdvg_alloc_node_and_init(void *private, int init_state)
{
    lcd_node_t   *job_node;

    if (in_interrupt() || in_atomic())
        job_node = kmem_cache_alloc(g_info.job_cache, GFP_ATOMIC);
    else
        job_node = kmem_cache_alloc(g_info.job_cache, GFP_KERNEL);

    if (job_node) {
        memset(job_node, 0, sizeof(lcd_node_t));
        INIT_LIST_HEAD(&job_node->list);
        job_node->private = private;
        job_node->status = init_state;
        job_node->callback = 1;
        job_node->countdown = 1;    //will be updated for one frame case in driver_parse_in_property().
        job_node->parent = job_node;    //point to itself
        job_node->puttime = jiffies;
        job_node->starttime = job_node->finishtime = 0;
        job_node->seq_id = g_jobid ++;
        g_info.mem_alloc ++;
    }
    else
        printk("%s, no memory! \n", __func__);

    return job_node;
}

/*
 * de-allocate a job memory
 */
void lcdvg_dealloc_node(void *memory)
{
    kmem_cache_free(g_info.job_cache, memory);
    g_info.mem_alloc --;
}

/* flush all jobs on hand */
void lcdvg_flush_jobs(void)
{
    unsigned long flags;
    lcd_node_t   *job_item;
    struct video_entity_job_t *job = NULL;

    DRV_LOCK(flags);

    /* flush the jobs on hand */
    if (g_info.cur_node != NULL) {
        /* prepare to callback */
        job = (struct video_entity_job_t *)g_info.cur_node->private;

        g_info.cur_node->status |= JOB_STS_FLUSH;
        if (lcdvg_dbg)
            printm(MODULE_NAME, "LCD%d, job->id: %d is stopped!(curnode)\n",
                                LCD200_ID, job->id);
    }

    if (!list_empty(&g_info.wlist)) {
        job_item = list_entry(g_info.wlist.next, lcd_node_t, list);
        job_item->countdown = 0; //return to videograph asap
    }

    list_for_each_entry(job_item, &g_info.qlist, list) {
        if (job_item->status == JOB_STS_QUEUE) {
            job_item->status = JOB_STS_FLUSH; /* don't need to process this job any more */

            job = (struct video_entity_job_t *)job_item->private;
            if (lcdvg_dbg)
                printm(MODULE_NAME, "LCD%d, job->id: %d is stopped!\n", LCD200_ID, job->id);
        }
    }

    if (LCD200_ID == LCD_ID)
        lcd_frameaddr_clearall();

    DRV_UNLOCK(flags);

#ifdef USE_KTRHEAD
    lcdvg_cb_wakeup();
#else
    PREPARE_DELAYED_WORK(&process_callback_work, (void *)callback_scheduler);
    queue_delayed_work(g_info.workq, &process_callback_work, callback_period);
#endif
}

/*
 * put JOB
 * We must save the last job for de-interlace reference.
 */
static int driver_putjob(void *parm)
{
    unsigned long flags;
    struct video_entity_job_t *job = (struct video_entity_job_t *)parm;
    lcd_node_t  *job_node;
    int id = ENTITY_FD_MINOR(job->fd);

    if (!vg_kick_off)
        vg_kick_off = 1;

    if (id != 0) {
        printk("LCD: Error id:%d, bug in videograph! \n", id);
        printm(MODULE_NAME, "LCD: Error id:%d, bug in videograph! \n", id);
        return JOB_STATUS_FAIL;
    }

    job_node = lcdvg_alloc_node_and_init(job, JOB_STS_QUEUE);
    if (unlikely(!job_node)) {
        panic("%s, no memory! \n", __func__);
        return JOB_STATUS_FAIL;
    }

    if (lcdvg_dbg)
        printm(MODULE_NAME, "LCD%d, put job->id: %d \n", LCD200_ID, job->id);

    if (driver_parse_in_property(job_node, job) < 0) {
        printk("%s, parse parameters fail! \n", __func__);
        printm(MODULE_NAME, "driver_parse_in_property fail! \n");
        return JOB_STATUS_FAIL;
    }

    DRV_LOCK(flags);
    list_add_tail(&job_node->list, &g_info.qlist);
    g_info.qnum ++;
    if (g_info.qnum >= max_qjob_threshold)
        job_node->status |= JOB_STS_FLUSH;

    if (job_node->sibling) {
        list_add_tail(&job_node->sibling->list, &g_info.qlist);
        g_info.qnum ++;

        if (g_info.qnum >= max_qjob_threshold)
            job_node->status |= JOB_STS_FLUSH;
    }
    g_info.put_job_cnt ++;  //count 1 for two frame type

    DRV_UNLOCK(flags);

    return JOB_STATUS_ONGOING;
}

/* Get which frame-buffer plane the video graph engine acts on.
 */
int lcdvg_get_fbplane(void)
{
    return g_info.in_prop.videograph_plane;
}

/* Stop a channel
 */
static int driver_stop(void *parm, int engine, int minor)
{
    if (parm || engine || minor) {}

    DEBUG_M(LOG_WARNING, engine, minor, "LCD%d, driver_stop is called. \n", LCD200_ID);

    vg_kick_off = 0;

    lcdvg_flush_jobs();

    /* set the frame base to the one which is on hand in initialization time. */
    //pip_parking_frame(g_info.in_prop.videograph_plane);
    //g_info.ref_clk_priv = NULL; //mark it, it is suggest by KL

    g_info.stop_cnt ++;

    return 0;
}


int driver_register_clock(void *cb_priv)
{
    g_info.ref_clk_priv = cb_priv;
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
        if(strcmp(property_map[i].name, str)==0) {
            return property_map[i].id;
        }
    }
    printk("driver_queryid: Error to find name %s\n",str);

    return -1;
}


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
    int i=0;
    struct property_map_t *map;
    unsigned int id,value;
    unsigned long flags;
    int idx=MAKE_IDX(ENTITY_MINORS,property_engine,property_minor);
    unsigned int len = 0;

    DRV_LOCK(flags);

    len += sprintf(page + len, "\n%s engine%d ch%d job %d\n",property_record[idx].entity->name,
        property_engine,property_minor, property_record[idx].job_id);
    len += sprintf(page + len, "=============================================================\n");
    len += sprintf(page + len, "ID  Name(string) Value(hex)  Readme\n");
    do {
        id=property_record[idx].property[i].id;
        if(id==ID_NULL)
            break;
        value=property_record[idx].property[i].value;
        map=driver_get_propertymap(id);
        if(map) {
            len += sprintf(page + len, "%02d  %12s  %09x  %s\n",id,map->name,value,map->readme);
        }
        i++;
    } while(1);

    DRV_UNLOCK(flags);

    *eof = 1;
    return len;
}

static unsigned int job_engine=0,job_minor=0;
static int proc_job_read_mode(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    struct video_entity_job_t *job;
    lcd_node_t  *job_item;
    unsigned long flags;
    char *st_string[] = {"STANDBY","ONGOING"," FINISH","   FLUSH"};
    unsigned int len = 0;

    DRV_LOCK(flags);

    len += sprintf(page + len, "\nEngine=%d Minor=%d (999 means all) System ticks=0x%x\n",
        job_engine, job_minor, (int)jiffies & 0xffff);

    /* go through the global list */
    len += sprintf(page + len, "Job_ID   Status   Puttime    start   end\n");
    len += sprintf(page + len, "===========================================================\n");

    list_for_each_entry(job_item, &g_info.qlist, list) {
        job = (struct video_entity_job_t *)job_item->private;
        len += sprintf(page + len, "%04d   %s   0x%04x   0x%04x  0x%04x (qlist)\n",
            job->id,st_string[to_string(job_item->status)], job_item->puttime & 0xffff,
            job_item->starttime & 0xffff, (int)job_item->finishtime & 0xffff);
    }

    if (g_info.cur_node) {
        job_item = g_info.cur_node;
        job = (struct video_entity_job_t *)job_item->private;
        len += sprintf(page + len, "%04d   %s   0x%04x   0x%04x  0x%04x (curnode)\n",
            job->id, st_string[to_string(job_item->status)], job_item->puttime & 0xffff,
            job_item->starttime & 0xffff, (int)job_item->finishtime & 0xffff);
    }

    list_for_each_entry(job_item, &g_info.wlist, list) {
        job = (struct video_entity_job_t *)job_item->private;
        len += sprintf(page + len, "%04d   %s   0x%04x   0x%04x  0x%04x (wlist)\n",
            job->id,st_string[to_string(job_item->status)], job_item->puttime & 0xffff,
            job_item->starttime & 0xffff, (int)job_item->finishtime & 0xffff);
    }

    list_for_each_entry(job_item, &g_info.cblist, list) {
        job = (struct video_entity_job_t *)job_item->private;
        len += sprintf(page + len, "%04d   %s   0x%04x   0x%04x  0x%04x (cblist)\n",
            job->id,st_string[to_string(job_item->status)], job_item->puttime & 0xffff,
            job_item->starttime & 0xffff, (int)job_item->finishtime & 0xffff);
    }

    DRV_UNLOCK(flags);

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
    printk("\nLog level =%d (0:error 1:warning 2:debug)\n",log_level);
    *eof = 1;
    return 0;
}

/* Minor Frame rate adjustment
*/
static int proc_minor_framerate_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int tmp;
    char *ptr, *str[2] = {"+", "-"};

    sscanf(buffer,"%d", &tmp);
    if (tmp >= 600 || tmp <= -600) {
        printk("error, value %d out of range(600 ~ -600)! \n", tmp);
    } else {
        minor_frame_rate = tmp;
    }

    ptr = (minor_frame_rate >= 0) ? str[0] : str[1];

    printk("\nSet valus is %d, adjust minor framerate: %s%d.%d \n", tmp, ptr, (int)abs(minor_frame_rate) / FRAME_RATE_GRANULAR,
                                        (int)abs(minor_frame_rate) % FRAME_RATE_GRANULAR);

    return count;
}

static int proc_minor_framerate_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    char *ptr, *str[2] = {"+", "-"};

    ptr = (minor_frame_rate >= 0) ? str[0] : str[1];

    printk("The adjust step is 0.1, +1 means 0.1. -1 means -0.1 \n");
    printk("Current value: %d, adjust minor framerate: %s%d.%d \n", minor_frame_rate, ptr,
        (int)abs(minor_frame_rate) / FRAME_RATE_GRANULAR, (int)abs(minor_frame_rate) % FRAME_RATE_GRANULAR);

    *eof = 1;
    return 0;
}

/* The threshold of job can be queued
 */
static int proc_max_threshold_write(struct file *file, const char *buffer,unsigned long count, void *data)
{
    unsigned char value[60];

    if (copy_from_user(value, buffer, count))
        return 0;

    sscanf(value, "%x\n", &max_qjob_threshold);
    printk("job queue threshold: %d \n", max_qjob_threshold);

    return count;
}

static int proc_max_threshold_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;

    len += sprintf(page + len, "job queue threshold: %d \n", max_qjob_threshold);

    *eof = 1;
    return len;
}

struct video_entity_ops_t driver_ops = {
    putjob:      &driver_putjob,
    stop:        &driver_stop,
    queryid:     &driver_queryid,
    querystr:    &driver_querystr,
    register_clock: &driver_register_clock
};

struct video_entity_t lcd_entity = {
#ifdef LCD_DEV2
    type:       TYPE_DISPLAY2,
#elif defined(LCD_DEV1)
    type:       TYPE_DISPLAY1,
#else
    type:       TYPE_DISPLAY0,
#endif
    name:       DRIVER_NAME,
    engines:    ENTITY_ENGINES,
    minors:     ENTITY_MINORS,
    ops:        &driver_ops
};

char   wname[30], cache_name[30];

/*
 * this function is called from pip.c
 */
static int lcdvg_driver_init(void)
{
    int     i;

    /* global information */
    memset(&g_info, 0x0, sizeof(g_info));

    /* register the callback to /proc/flcd200_cmmn/xxx, it will get the summary of this lcd */
    proc_register_lcdinfo(LCD200_ID, lcdvg_summary_callback);

    /* check if this LCD joins 3frame operation */
    if (d1_3frame) {
        /* only LCD_ID is the Master, others are slave */
        switch (LCD200_ID) {
          case LCD1_ID:
            pip_register_callback(&callback_3frame_fn[1]);
            printk("LCD1 joins D1 of 3frame. \n");
            break;
          case LCD2_ID:
            pip_register_callback(&callback_3frame_fn[2]);
            printk("LCD2 joins D1 of 3frame. \n");
            break;
          default:
            panic("%s, error! LCD%d should be not D1 in 3frame! \n", __func__, LCD200_ID);
            break;
        }
        return 0;
    }

    g_info.in_prop.videograph_plane = -1;

    g_info.dev_info = (void *)&g_dev_info.lcd200_info;
    video_entity_register(&lcd_entity);

    memset(cache_name, 0, sizeof(cache_name));
    sprintf(cache_name,"lcd_job%d", LCD200_ID);

    g_info.job_cache = kmem_cache_create(cache_name, sizeof(lcd_node_t), 0, 0, NULL);
    if (!g_info.job_cache)
        panic("LCD creates cache fail! \n");

    INIT_LIST_HEAD(&g_info.qlist);
    INIT_LIST_HEAD(&g_info.wlist);
    INIT_LIST_HEAD(&g_info.cblist);

    /* register log system */
    register_panic_notifier(lcdvg_panic_notifier);
    register_printout_notifier(lcdvg_print_notifier);

    property_record = kzalloc(sizeof(struct property_record_t) * ENTITY_ENGINES * ENTITY_MINORS, GFP_KERNEL);
    if (property_record == NULL)
        panic("%s, no memory! \n", __func__);

    /* spinlock */
    spin_lock_init(&g_info.lock);
    memset(wname, 0, sizeof(wname));
    sprintf(wname,"process_%s", DRIVER_NAME);
    g_info.workq = create_workqueue(wname);
    if (g_info.workq == NULL) {
        printk("LCD: error in create workqueue! \n");
        return -1;
    }

    INIT_WORK(&gui_clone_work, lcdvg_gui_clone_workfn);

    /*
     * register vg callback funtion to pip
     */
    if (g_dev_info.lcd200_info.lcd200_idx == LCD_ID)
        pip_register_callback(&callback_fn[0]);
    else if (g_dev_info.lcd200_info.lcd200_idx == LCD1_ID)
        pip_register_callback(&callback_fn[1]);
    else if (g_dev_info.lcd200_info.lcd200_idx == LCD2_ID)
        pip_register_callback(&callback_fn[2]);
    else
        panic("%s 111 \n", __func__);

#ifdef LCD_DEV2
    entity_proc = create_proc_entry("videograph/flcd200_2", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
#elif defined(LCD_DEV1)
    entity_proc = create_proc_entry("videograph/flcd200_1", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
#else
    entity_proc = create_proc_entry("videograph/flcd200", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
#endif

    if(entity_proc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        return -EFAULT;
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

    minor_framerate_proc = create_proc_entry("adjust_frame_rate", S_IRUGO | S_IXUGO, entity_proc);
    if (minor_framerate_proc == NULL)
        return -EFAULT;
    minor_framerate_proc->read_proc = (read_proc_t *)proc_minor_framerate_read_mode;
    minor_framerate_proc->write_proc = (write_proc_t *)proc_minor_framerate_write_mode;

    /* a proc used to configure the max threhold of queueing job */
    max_threshold_proc = create_proc_entry("max_threshold", S_IRUGO | S_IXUGO, entity_proc);
    if (max_threshold_proc == NULL)
        return -EFAULT;
    max_threshold_proc->read_proc = (read_proc_t *)proc_max_threshold_read;
    max_threshold_proc->write_proc = (write_proc_t *)proc_max_threshold_write;

    for(i = 0; i < MAX_FILTER; i++) {
        include_filter_idx[i] = -1;
        exclude_filter_idx[i] = -1;
    }

    printk("\nLCD%d registers %d entities to video graph! \n", LCD200_ID, ENTITY_MINORS);

    /* uninitilaize videoplane */
    g_info.in_prop.videograph_plane = 0;

#ifdef USE_KTRHEAD
    init_waitqueue_head(&cb_thread_wait);
    cb_thread = kthread_create(lcdvg_callback_thread, NULL, "lcdvg_thread");
    if (IS_ERR(cb_thread))
        panic("%s, create cb_thread fail ! \n", __func__);
    wake_up_process(cb_thread);
#endif /* USE_KTHREAD */

    return 0;
}

void lcdvg_inres_change(void)
{
    video_entity_notify(&lcd_entity, ENTITY_FD(lcd_entity.type, 0, 0), VG_HW_CONFIG_CHANGE, 1);
}

void lcdvg_driver_clearnup(void)
{
    char name[30];

    /* de-register the callback */
    proc_register_lcdinfo(LCD200_ID, NULL);

    /* Only LCD0 is the master for the 3 frame */
    if (d1_3frame == 1)
        return; //do nothing

    lcdvg_flush_jobs();
    msleep(500);
    /* At cleanup, no new job triggers to free reserved buffer, so do it manually. */
    if (rev_handle) {
        video_free_buffer(rev_handle);
        rev_handle = NULL;
    }
    video_entity_deregister(&lcd_entity);

    // remove workqueue
    if (g_info.workq)
        destroy_workqueue(g_info.workq);

#ifdef USE_KTRHEAD
    if (cb_thread) {
        cb_thread_reset = 1;
        lcdvg_cb_wakeup();
        kthread_stop(cb_thread);
        /* wait thread to be terminated */
        while (cb_thread_ready) {
            msleep(10);
        }
    }
#endif

    if (property_record)
        kfree(property_record);

    if (g_info.job_cache)
        kmem_cache_destroy(g_info.job_cache);

    if (cbproc!=0)
        remove_proc_entry(cbproc->name, entity_proc);
    if (propertyproc!=0)
        remove_proc_entry(propertyproc->name, entity_proc);
    if (jobproc!=0)
        remove_proc_entry(jobproc->name, entity_proc);
    if (filterproc!=0)
        remove_proc_entry(filterproc->name, entity_proc);
    if (levelproc!=0)
        remove_proc_entry(levelproc->name, entity_proc);
    if (infoproc!=0)
        remove_proc_entry(infoproc->name, entity_proc);
    if (minor_framerate_proc != 0)
        remove_proc_entry(minor_framerate_proc->name, entity_proc);
    if (max_threshold_proc != 0)
        remove_proc_entry(max_threshold_proc->name, entity_proc);

    memset(&name[0], 0, sizeof(name));
#ifdef LCD_DEV2
    sprintf(name,"videograph/flcd200_2");
#elif defined(LCD_DEV1)
    sprintf(name,"videograph/flcd200_1");
#else
    sprintf(name,"videograph/flcd200");
#endif

    if(entity_proc!=0)
        remove_proc_entry(name, 0);
}

proc_lcdinfo_t  src_lcd_info, dst_lcd_info;
void lcdvg_gui_clone_workfn(struct work_struct *work)
{
    int lcd_idx, ret;

    if (ffb_proc_get_lcdinfo(LCD200_ID, &src_lcd_info)) {
        printk(KERN_DEBUG"Error in getting src_lcd_info! \n");
        return;
    }

    for (lcd_idx = 0; lcd_idx < LCD_MAX_ID; lcd_idx ++) {
        if (!test_bit(lcd_idx, (void *)&lcd_gui_bitmap))
            continue;

        /* get dest information */
        if (ffb_proc_get_lcdinfo(lcd_idx, &dst_lcd_info))
            continue;

        ret = 0;
        if ((src_lcd_info.gui_info.PIP_En >= 1) && (dst_lcd_info.gui_info.PIP_En >= 1)) {
            struct gui_clone_adjust *adjust = &gui_clone_adjust;
            gui_info_t *gui_info = &dst_lcd_info.gui_info;
            u32 dst_phys_addr, dst_xres, dst_yres;

            /* scaler needs 4 byte alignment */
            src_lcd_info.gui_info.gui[1].xres = (src_lcd_info.gui_info.gui[1].xres >> 2) << 2;
            /* destination
             */
            dst_xres = gui_info->gui[1].xres;
            dst_yres = gui_info->gui[1].yres;
            /* sanity check */
            if (adjust->width + adjust->x > gui_info->gui[1].xres)
                ret = -1;
            if (adjust->height + adjust->y > gui_info->gui[1].yres)
                ret = -1;

            if (adjust->width)
                dst_xres = adjust->width;

            if (adjust->height)
                dst_yres = adjust->height;

            dst_xres = (dst_xres >> 2) << 2;
            dst_phys_addr = gui_info->gui[1].phys_addr + gui_info->gui[1].xres * (gui_info->gui[1].bits_per_pixel >> 3) *
                            adjust->y + adjust->x * (gui_info->gui[1].bits_per_pixel / 8);

            if (!ret) {
                ret = video_scaling2(TYPE_RGB565,
                                src_lcd_info.gui_info.gui[1].phys_addr,
                                src_lcd_info.gui_info.gui[1].xres,
                                src_lcd_info.gui_info.gui[1].yres,
                                src_lcd_info.gui_info.gui[1].xres,
                                src_lcd_info.gui_info.gui[1].yres,
                                dst_phys_addr,
                                dst_xres,
                                dst_yres,
                                dst_lcd_info.gui_info.gui[1].xres,
                                dst_lcd_info.gui_info.gui[1].yres);
            }

            if (ret)
                put_scalerjob_fail_cnt ++;
            else
                put_scalerjob_ok_cnt ++;
        } /* if */
    } /* for */
}

#ifdef USE_KTRHEAD
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
        if (cb_thread_reset)
            msleep(500);
        /* callback process */
        callback_scheduler();
    } while(!kthread_should_stop());

    cb_thread_ready = 0;
    return 0;
}

static void lcdvg_cb_wakeup(void)
{
    cb_wakeup_event = 1;
    wake_up(&cb_thread_wait);
}
#endif /* USE_KTRHEAD */
