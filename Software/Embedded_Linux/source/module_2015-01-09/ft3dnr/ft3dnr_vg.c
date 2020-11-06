#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/list.h>
#include <linux/version.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "ft3dnr_vg.h"
#include "ft3dnr_dbg.h"
#include "util.h"
#include <api_if.h>

#include <log.h>    //include log system "printm","damnit"...
#include <video_entity.h>   //include video entity manager

#define MODULE_NAME         "DN"    //<<<<<<<<<< Modify your module name (two bytes) >>>>>>>>>>>>>>

extern int platform_clock_on(int on);

int clock_on = 0;
int clock_off = 0;

global_info_t global_info;
vg_proc_t     vg_proc_info;
#ifdef USE_WQ
struct workqueue_struct *callback_workq;
static DECLARE_DELAYED_WORK(process_callback_work, 0);
#endif
static void ft3dnr_callback_process(void);

#ifdef USE_KTHREAD
wait_queue_head_t cb_thread_wait;
int cb_wakeup_event = 0;
static void callback_wake_up(void);
#endif

extern int max_minors;

/* utilization */
static unsigned int utilization_period = 5; //5sec calculation

static unsigned int utilization_start[1], utilization_record[1];
static unsigned int engine_start[1], engine_end[1];
static unsigned int engine_time[1];

/* proc system */
unsigned int callback_period = 0;    //ticks

/* property lastest record */
struct property_record_t *property_record;

#define LOG_ERROR       0
#define LOG_WARNING     1

/* log & debug message */
#define MAX_FILTER  5
static unsigned int log_level = LOG_ERROR;
static int include_filter_idx[MAX_FILTER]; //init to -1, ENGINE<<16|MINOR
static int exclude_filter_idx[MAX_FILTER]; //init to -1, ENGINE<<16|MINOR

struct property_map_t property_map[] = {
    {ID_SRC_FMT,            "src_fmt",          "source format"},
    {ID_SRC_XY,             "src_xy",           "source x and y position"},
    {ID_SRC_BG_DIM,         "src_bg_dim",       "source background width/height"},
    {ID_SRC_DIM,            "src_dim",          "source width/height"},
    {ID_SRC_TYPE,           "src_type",          "source type, 0: decoder 1: from isp 2: sensor"},
    {ID_ORG_DIM,            "org_dim",          "original width/height"},
};

int is_print(int engine,int minor)
{
    int i;
    if(include_filter_idx[0] >= 0) {
        for(i = 0; i < MAX_FILTER; i++)
            if(include_filter_idx[i] == ((engine << 16) | minor))
                return 1;
    }

    if(exclude_filter_idx[0] >= 0) {
        for(i = 0; i < MAX_FILTER; i++)
            if(exclude_filter_idx[i] == ((engine << 16) | minor))
                return 0;
    }
    return 1;
}

#if 1
#define DEBUG_M(level,engine,minor,fmt,args...) { \
    if((log_level>=level)&&is_print(engine,minor)) \
        printm(MODULE_NAME,fmt, ## args); }
#else
#define DEBUG_M(level,engine,minor,fmt,args...) { \
    if((log_level>=level)&&is_print(engine,minor)) \
        printk(fmt,## args);}
#endif

static int driver_parse_in_property(job_node_t *node, struct video_entity_job_t *job)
{
    int i = 0, chan_id = 0;
    struct video_property_t *property = job->in_prop;
    job_node_t *ref_node = NULL;

    chan_id = node->chan_id;

    /* fill up the input parameters */
    while(property[i].id != 0) {
        switch (property[i].id) {
            case ID_SRC_FMT:
                    node->param.src_fmt = property[i].value;
                break;
            case ID_SRC_XY:
                    node->param.dim.src_x = EM_PARAM_X(property[i].value);
                    node->param.dim.src_y = EM_PARAM_Y(property[i].value);
                break;
            case ID_SRC_DIM:
                    node->param.dim.nr_width = EM_PARAM_WIDTH(property[i].value);
                    node->param.dim.nr_height = EM_PARAM_HEIGHT(property[i].value);
                break;
            case ID_SRC_BG_DIM:
                    node->param.dim.src_bg_width = EM_PARAM_WIDTH(property[i].value);
                    node->param.dim.src_bg_height = EM_PARAM_HEIGHT(property[i].value);
                break;
            case ID_SRC_TYPE:
                    node->param.src_type = property[i].value;
                break;
            case ID_ORG_DIM:
                    node->param.dim.org_width = EM_PARAM_WIDTH(property[i].value);
                    node->param.dim.org_height = EM_PARAM_HEIGHT(property[i].value);
                break;
            default:
                break;
        }
        i++;
    }

    /* because the mrnr parameter is configured for main stream,
     * but we have no idea which one of resolutions is main stream,
     * so we guess that the largest one is it.
     */
    if (node->param.dim.src_bg_width > priv.curr_max_dim.src_bg_width) {
        memcpy(&priv.curr_max_dim, &node->param.dim, sizeof(ft3dnr_dim_t));
        priv.isp_param.mrnr_id++; // force to update mrnr parameter
    }

    /* if src/dst/roi width/height is changed, do not reference to previous job */
    ref_node = global_info.ref_node[chan_id];
    if (ref_node) {
        if (memcmp(&ref_node->param.dim, &node->param.dim, sizeof(ft3dnr_dim_t)))
            return 1;
    }

    return 0;
}

static int driver_stop(void *parm, int engine, int minor)
{
    int idx = MAKE_IDX(ENTITY_MINORS, engine, minor);
    job_node_t *node, *ne;

    if (idx > ENTITY_MINORS)
        panic("Error!! DN driver stop minor %d, max is %d\n", idx, ENTITY_MINORS);

    //printk("%s minor %d, idx %d\n", __func__, minor, idx);
    ft3dnr_stop_channel(idx);

    down(&global_info.sema_lock);

    list_for_each_entry_safe(node, ne, &global_info.node_list, plist) {
        if (node->chan_id == idx) {
            /* record job stop, ongoing job should also stop, but maybe in link list sram, can't stop */
            node->stop_flag = 1;

            if (node->refer_to) {
                REFCNT_DEC(node->refer_to);
                node->refer_to = NULL;
            }
        }
    }

    if (global_info.ref_node[idx]) {
        /* decrease the reference count of this node */
        REFCNT_DEC(global_info.ref_node[idx]);
        global_info.ref_node[idx] = NULL;
    }

    up(&global_info.sema_lock);

#ifdef USE_KTHREAD
    callback_wake_up();
#endif
#ifdef USE_WQ
    /* schedule the delay workq */
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)ft3dnr_callback_process);
    queue_delayed_work(callback_workq, &process_callback_work, callback_period);
#endif
    return 0;
}

static int driver_queryid(void *parm,char *str)
{
    int i;

    for (i = 0; i < sizeof(property_map) / sizeof(struct property_map_t); i++) {
        if (strcmp(property_map[i].name,str) == 0) {
            return property_map[i].id;
        }
    }
    printk("driver_queryid: Error to find name %s\n",str);
    return -1;
}

static int driver_querystr(void *parm,int id,char *string)
{
    int i;

    for (i = 0; i < sizeof(property_map) / sizeof(struct property_map_t); i++) {
        if (property_map[i].id == id) {
            memcpy(string,property_map[i].name,sizeof(property_map[i].name));
            return 0;
        }
    }
    printk("driver_querystr: Error to find id %d\n",id);
    return -1;
}

//void ft3dnr_callback_process(void *parm)
void ft3dnr_callback_process(void)
{
    job_node_t  *node, *ne, *curr, *ne1;
    struct video_entity_job_t *job, *child;
    //int callback = 0;
    int ret = 0;
    //printk("%s\n", __func__);

    down(&global_info.sema_lock);
    list_for_each_entry_safe(node, ne, &global_info.node_list, plist) {
        //callback = 0;
        job = (struct video_entity_job_t *)node->private;
        /* this job is still referenced */
        if (atomic_read(&node->ref_cnt) > 0) {
            // reserve output buffer
            if (node->ref_buf == NULL) {
                node->ref_buf = video_reserve_buffer(job, 1);
                atomic_inc(&priv.buf_cnt);
            }
        }

        if (node->status == JOB_STS_DONE || node->status == JOB_STS_FLUSH || node->status == JOB_STS_DONOTHING) {
            //job = (struct video_entity_job_t *)node->private;
            if (node->status == JOB_STS_DONE || node->status == JOB_STS_DONOTHING) {
                job->status = JOB_STATUS_FINISH;
                /* this job still ongoing when vg call driver_stop, callback fail here */
                if (node->stop_flag == 1)
                    job->status = JOB_STATUS_FAIL;
            }
            else
                job->status = JOB_STATUS_FAIL;

            if (node->need_callback) {
                //printk("**callback [%d] %d %d **\n", node->job_id, node->status, job->status);
                job->callback(node->private); //callback root job
                //callback = 1;
                node->need_callback = 0;
            }
            /* callback & free child node*/
            list_for_each_entry_safe(curr, ne1, &node->clist ,clist) {
                job = (struct video_entity_job_t *)curr->private;
                /* this job is still referenced */
                if (atomic_read(&curr->ref_cnt) > 0) {
                    // reserve output buffer
                    if (curr->ref_buf == NULL) {
                        curr->ref_buf = video_reserve_buffer(job, 1);
                        atomic_inc(&priv.buf_cnt);
                    }
                }

                child = (struct video_entity_job_t *)curr->private;
                if (curr->status == JOB_STS_DONE || curr->status == JOB_STS_DONOTHING)
                    child->status = JOB_STATUS_FINISH;
                else
                    child->status = JOB_STATUS_FAIL;

                if (curr->need_callback) {
                    //printk("**callback [%d] %d**\n", curr->job_id, job->status);
                    job->callback(node->private); //callback root node
                    //callback = 1;
                    curr->need_callback = 0;
                }
            }

            //if (callback == 1)
            if (atomic_read(&node->ref_cnt) == 0) {
                /* free child node */
                list_for_each_entry_safe(curr, ne1, &node->clist ,clist) {
                    // if child reference count != 0, can't free, goto exit
                    if (atomic_read(&curr->ref_cnt) != 0)
                        goto exit;
                    // free node's reserved buffer
                    if (curr->ref_buf) {
                        ret = video_free_buffer(curr->ref_buf);
                        if (ret < 0)
                            printk("[DN] video free buf fail\n");
                        curr->ref_buf = NULL;
                        atomic_dec(&priv.buf_cnt);
                    }

                    list_del(&curr->clist);
                    kmem_cache_free(global_info.node_cache, curr);
                    MEMCNT_SUB(&priv, 1);
                }
                // free node's reserved buffer
                if (node->ref_buf) {
                    ret = video_free_buffer(node->ref_buf);
                    if (ret < 0)
                        printk("[DN] video free buf fail\n");
                    node->ref_buf = NULL;
                    atomic_dec(&priv.buf_cnt);
                }

                /* free parent node */
                list_del(&node->plist);
                kmem_cache_free(global_info.node_cache, node);
                MEMCNT_SUB(&priv, 1);
            }
        }
        else
            break;
    }
exit:
    if (list_empty(&global_info.node_list)) {
        clock_off++;
        platform_clock_on(0);
    }

    up(&global_info.sema_lock);
}

#ifdef USE_KTHREAD
int cb_running = 0;
static int cb_thread(void *__cwq)
{
    int status;
    cb_running = 1;

    do {
        status = wait_event_timeout(cb_thread_wait, cb_wakeup_event, msecs_to_jiffies(1000));
        if (status == 0)
            continue;   /* timeout */

        cb_wakeup_event = 0;
        /* callback process */
        ft3dnr_callback_process();
    } while(!kthread_should_stop());

    __set_current_state(TASK_RUNNING);
    cb_running = 0;
    return 0;
}

static void callback_wake_up(void)
{
    cb_wakeup_event = 1;
    wake_up(&cb_thread_wait);
}
#endif

/*
 * param: private data
 */
static int driver_set_out_property(void *param, struct video_entity_job_t *job)
{
    //ft3dnr_job_t   *job_item = (ft3dnr_job_t *)param;

    return 1;
}

/*
 * callback function for job finish. In ISR.
 */
static void driver_callback(ft3dnr_job_t *job)
{
    job_node_t    *node, *parent, *curr, *ne;//*ref_node;
    //printk("%s job id %d\n", __func__, job->job_id);

    if (job->job_type == MULTI_JOB) {
        node = (job_node_t *)job->private;
        driver_set_out_property(job, node->private);
        parent = list_entry(node->clist.next, job_node_t, clist);
        parent->status = job->status;
        //printk("parent id %d node id %d\n", parent->job_id, node->job_id);

        if (parent->refer_to) {
            REFCNT_DEC(parent->refer_to);
            parent->refer_to = NULL;
        }

        list_for_each_entry_safe(curr, ne, &parent->clist, clist) {
            curr->status = job->status;
            driver_set_out_property(job, curr->private);

            if (curr->refer_to) {
                REFCNT_DEC(curr->refer_to);
                curr->refer_to = NULL;
            }
        }
    }
    else {
        node = (job_node_t *)job->private;
        node->status = job->status;
        driver_set_out_property(job, node->private);

        if (node->refer_to) {
            REFCNT_DEC(node->refer_to);
            node->refer_to = NULL;
        }
    }
#ifdef USE_KTHREAD
    callback_wake_up();
#endif
#ifdef USE_WQ
    /* schedule the delay workq */
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)ft3dnr_callback_process);
    queue_delayed_work(callback_workq, &process_callback_work, callback_period);
#endif
}

static void driver_update_status(ft3dnr_job_t *job, int status)
{
    job_node_t *node;

    node = (job_node_t *)job->private;
    node->status = status;
}

static int driver_preprocess(ft3dnr_job_t *job)
{
    return 0;
}

static int driver_postprocess(ft3dnr_job_t *job)
{
    return 0;
}

static void driver_mark_engine_start(ft3dnr_job_t *job_item)
{
    int j = 0;
    job_node_t  *node = NULL;
    struct video_entity_job_t *job;
    struct video_property_t *property;
    int idx;

    node = (job_node_t *)job_item->private;
    job = (struct video_entity_job_t *)node->private;
    property = job->in_prop;
    idx = MAKE_IDX(max_minors,ENTITY_FD_ENGINE(job->fd),ENTITY_FD_MINOR(job->fd));
    while (property[j].id != 0) {
        if (j < MAX_RECORD) {
            property_record[idx].job_id = job->id;
            property_record[idx].property[j].id = property[j].id;
            property_record[idx].property[j].ch = property[j].ch;
            property_record[idx].property[j].value = property[j].value;
        }
        j++;
    }

}

static void driver_mark_engine_finish(ft3dnr_job_t *job_item)
{
}

static int driver_flush_check(ft3dnr_job_t *job_item)
{
    return 0;
}

void mark_engine_start(void)
{
    int dev = 0;
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);

    if (engine_start[dev] != 0)
        printk("[FT3DNR]Warning to nested use dev%d mark_engine_start function!\n",dev);

    engine_start[dev] = jiffies;

    engine_end[dev] = 0;
    if (utilization_start[dev] == 0) {
        utilization_start[dev] = engine_start[dev];
        engine_time[dev] = 0;
    }

    spin_unlock_irqrestore(&priv.lock, flags);
}

void mark_engine_finish(void)
{
    int dev = 0;
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);

    if (engine_end[dev] != 0)
        printk("[FT3DNR]Warning to nested use dev%d mark_engine_finish function!\n",dev);

    engine_end[dev] = jiffies;

    if (engine_end[dev] > engine_start[dev])
        engine_time[dev] += engine_end[dev] - engine_start[dev];

    if (utilization_start[dev] > engine_end[dev]) {
        utilization_start[dev] = 0;
        engine_time[dev] = 0;
    } else if ((utilization_start[dev] <= engine_end[dev]) &&
        (engine_end[dev] - utilization_start[dev] >= utilization_period * HZ)) {
        unsigned int utilization;

        utilization = (unsigned int)((100*engine_time[dev]) / (engine_end[dev] - utilization_start[dev]));
        if (utilization)
            utilization_record[dev] = utilization;
        utilization_start[dev] = 0;
        engine_time[dev] = 0;
    }
    engine_start[dev]=0;
    spin_unlock_irqrestore(&priv.lock, flags);
}


/* callback functions called from the core
 */
struct f_ops callback_ops = {
    callback:           &driver_callback,
    update_status:      &driver_update_status,
    pre_process:        &driver_preprocess,
    post_process:       &driver_postprocess,
    mark_engine_start:  &driver_mark_engine_start,
    mark_engine_finish: &driver_mark_engine_finish,
    flush_check:        &driver_flush_check,
};

/*
 * Add a node to nodelist
 */
static void vg_nodelist_add(job_node_t *node)
{
    list_add_tail(&node->plist, &global_info.node_list);
}

int cal_mrnr_param(job_node_t *node)
{
    int i, j, idx;
    unsigned int dist0, dist1, layerRatio, scalingRatio;

    if (priv.curr_max_dim.src_bg_width)
        scalingRatio = (node->param.dim.src_bg_width << 6) / priv.curr_max_dim.src_bg_width;
    else
        panic("%s(%s): division by zero", DRIVER_NAME, __func__);

    if (scalingRatio >= 64) {
        memcpy(&node->param.mrnr, &priv.isp_param.mrnr, sizeof(mrnr_param_t));
        return 0;
    }

    for (i = 0; i < 4; i++) //layer
    {
        layerRatio = scalingRatio >> i;

        if (layerRatio <= 8) {
        	dist0 = 1;
        	dist1 = 0;
        	idx = 4;
        }
        else if (layerRatio <= 16) {
        	dist0 = 16 - layerRatio;
        	dist1 = layerRatio - 8;
        	idx = 3;
        }
        else if (layerRatio <= 32) {
        	dist0 = 32 - layerRatio;
        	dist1 = layerRatio - 16;
        	idx = 2;
        }
        else {
        	dist0 = 64 - layerRatio;
        	dist1 = layerRatio - 32;
        	idx = 1;
        }

        //Y channel thresholds
        for (j = 0; j < 8; j++) {
            node->param.mrnr.Y_L_edg_th[i][j] = ((priv.isp_param.mrnr.Y_L_edg_th[(min(idx,3) - 1)][j] * dist1 +
                                                priv.isp_param.mrnr.Y_L_edg_th[min(idx,3)][j] * dist0) / (dist0 + dist1));
            node->param.mrnr.Y_L_smo_th[i][j] = ((priv.isp_param.mrnr.Y_L_smo_th[(min(idx,3) - 1)][j] * dist1 +
                                                priv.isp_param.mrnr.Y_L_smo_th[min(idx,3)][j] * dist0) / (dist0 + dist1));
        }
        //Cb channel thresholds
        node->param.mrnr.Cb_L_edg_th[i] = (unsigned int)((priv.isp_param.mrnr.Cb_L_edg_th[min(idx,3) - 1] * dist1 +
                                            priv.isp_param.mrnr.Cb_L_edg_th[min(idx,3)] * dist0) / (dist0 + dist1));
        node->param.mrnr.Cb_L_smo_th[i] = (unsigned int)((priv.isp_param.mrnr.Cb_L_smo_th[min(idx,3) - 1] * dist1 +
                                            priv.isp_param.mrnr.Cb_L_smo_th[min(idx,3)] * dist0) / (dist0 + dist1));

        //Cr channel thresholds
        node->param.mrnr.Cr_L_edg_th[i] = (unsigned int)((priv.isp_param.mrnr.Cr_L_edg_th[min(idx,3) - 1] * dist1 +
                                            priv.isp_param.mrnr.Cr_L_edg_th[min(idx,3)] * dist0) / (dist0 + dist1));
        node->param.mrnr.Cr_L_smo_th[i] = (unsigned int)((priv.isp_param.mrnr.Cr_L_smo_th[min(idx,3) - 1] * dist1 +
                                            priv.isp_param.mrnr.Cr_L_smo_th[min(idx,3)] * dist0) / (dist0 + dist1));

        /* copy noise reduction strength */
        node->param.mrnr.Y_L_nr_str[i] = priv.isp_param.mrnr.Y_L_nr_str[i];
        node->param.mrnr.C_L_nr_str[i] = priv.isp_param.mrnr.C_L_nr_str[i];
    }
    return 0;
}

int get_mrnr_param(job_node_t *node, job_node_t *ref_node)
{
    /* channel from decoder/sensor, copy default mrnr */
    if (node->param.src_type == SRC_TYPE_DECODER || node->param.src_type == SRC_TYPE_SENSOR)
        memcpy(&node->param.mrnr, &priv.default_param.mrnr, sizeof(mrnr_param_t));

    /* channel from isp */
    if (node->param.src_type == SRC_TYPE_ISP) {
        /* need to reference to previous job, and mrnr id not changed, copy ref_node mrnr */
        if (ref_node != NULL) {
            if (ref_node->param.mrnr_id == priv.isp_param.mrnr_id) {
                memcpy(&node->param.mrnr, &ref_node->param.mrnr, sizeof(mrnr_param_t));
                return 0;
            }
        }
        /* ref_node = NULL or mrnr_id is changed, need to re-calculate mrnr */
        /* calculate mrnr param */
        cal_mrnr_param(node);
    }

    return 0;
}

int get_tmnr_param(job_node_t *node, job_node_t *ref_node)
{
    /* channel from decoder/sensor, copy default tmnr */
    if (node->param.src_type == SRC_TYPE_DECODER || node->param.src_type == SRC_TYPE_SENSOR)
        memcpy(&node->param.tmnr, &priv.default_param.tmnr, sizeof(tmnr_param_t));

    /* channel from isp */
    if (node->param.src_type == SRC_TYPE_ISP)
        memcpy(&node->param.tmnr, &priv.isp_param.tmnr, sizeof(tmnr_param_t));
#if 0
    /* calculate tmnr param */
    if (ref_node->tmnr_id == priv.isp_param.tmnr_id)
        memcpy(&node->param.tmnr, &ref_node->param.tmnr, sizeof(tmnr_param_t));
    else
        cal_tmnr_param(node);
#endif

    return 0;
}

int cal_his_param(job_node_t *node)
{
    unsigned int width_constraint  = 0;
	unsigned int height_constraint = 0;
    ft3dnr_ctrl_t   *ctrl = &node->param.ctrl;
    ft3dnr_dim_t    *dim = &node->param.dim;
    ft3dnr_his_t    *his = &node->param.his;

	/* his th active only when tnr_learn_en == 1 */
	if ((ctrl->temporal_en == 0) || (ctrl->tnr_learn_en == 0))
		return 0;

    /* get width_constraint */
	if ((dim->nr_width % VARBLK_WIDTH) == 0)
		width_constraint = VARBLK_WIDTH;
	else
		width_constraint = dim->nr_width % VARBLK_WIDTH;

    /* get height_constraint */
	if ((dim->nr_height % VARBLK_HEIGHT) == 0)
		height_constraint = VARBLK_HEIGHT;
	else
		height_constraint = dim->nr_height % VARBLK_HEIGHT;

    /* cal th */
    his->norm_th = WIDTH_LIMIT * HEIGHT_LIMIT * TH_RETIO / 10;

	his->right_th = (width_constraint - 2) * HEIGHT_LIMIT * TH_RETIO / 10;

    his->bot_th = WIDTH_LIMIT * (height_constraint - 2) * TH_RETIO / 10;

    his->corner_th = (width_constraint - 2) * (height_constraint - 2) * TH_RETIO / 10;

    return 0;
}

int sanity_check(job_node_t *node)
{
    int chn = node->chan_id;
    ft3dnr_param_t *param = &node->param;
    int ret = 0;

    if (param->src_type >= SRC_TYPE_UNKNOWN) {
        dn_err("Error! source type not from decoder/isp/sensor %d\n", param->src_type);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source type [%d] not from decoder/isp/sensor\n", chn, param->src_type);
        ret = -1;
    }

    if (param->dim.src_bg_width % 4 != 0) {
        dn_err("Error! source background width [%d] not 4 alignment\n", param->dim.src_bg_width);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source background width [%d] not 4 alignment\n", chn, param->dim.src_bg_width);
        ret = -1;
    }

    if (param->dim.src_bg_width == 0) {
        dn_err("Error! source background width [%d] is zero\n", param->dim.src_bg_width);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source background width [%d] is zero\n", chn, param->dim.src_bg_width);
        ret = -1;
    }

    if (param->dim.src_bg_width < 132) {
        dn_err("Error! source background width [%d] < 132\n", param->dim.src_bg_width);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source background width [%d] < 132\n", chn, param->dim.src_bg_width);
        ret = -1;
    }

    if (param->dim.src_bg_height % 2 != 0) {
        dn_err("Error! source background height [%d] not 2 alignment\n", param->dim.src_bg_height);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source background height [%d] not 2 alignment\n", chn, param->dim.src_bg_height);
        ret = -1;
    }

    if (param->dim.src_bg_height == 0) {
        dn_err("Error! source background height [%d] is zero\n", param->dim.src_bg_height);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source background height [%d] is zero\n", chn, param->dim.src_bg_height);
        ret = -1;
    }

    if (param->dim.src_bg_height < 66) {
        dn_err("Error! source background height [%d] < 66\n", param->dim.src_bg_height);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source background height [%d] < 66\n", chn, param->dim.src_bg_height);
        ret = -1;
    }

    if (param->dim.src_x % 4 != 0) {
        dn_err("Error! source x [%d] not 4 alignment\n", param->dim.src_x);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source x [%d] not 4 alignment\n", chn, param->dim.src_x);
        ret = -1;
    }

    if (param->dim.src_y % 2 != 0) {
        dn_err("Error! source y [%d] not 2 alignment\n", param->dim.src_y);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source y [%d] not 2 alignment\n", chn, param->dim.src_y);
        ret = -1;
    }

    if (param->dim.nr_width % 4 != 0) {
        dn_err("Error! noise width [%d] not 4 alignment\n", param->dim.nr_width);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] noise width [%d] not 4 alignment\n", chn, param->dim.nr_width);
        ret = -1;
    }

    if (param->dim.nr_width == 0) {
        dn_err("Error! noise width [%d] is zero\n", param->dim.nr_width);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] noise width [%d] is zero\n", chn, param->dim.nr_width);
        ret = -1;
    }

    if (param->dim.nr_width < 132) {
        dn_err("Error! noise width [%d] < 132\n", param->dim.nr_width);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] noise width [%d] < 132\n", chn, param->dim.nr_width);
        ret = -1;
    }

    if (param->dim.nr_height % 2 != 0) {
        dn_err("Error! noise height [%d] not 2 alignment\n", param->dim.nr_height);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] noise height [%d] not 2 alignment\n", chn, param->dim.nr_height);
        ret = -1;
    }

    if (param->dim.nr_height == 0) {
        dn_err("Error! noise height [%d] is zero\n", param->dim.nr_height);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] noise height [%d] is zero\n", chn, param->dim.nr_height);
        ret = -1;
    }

    if (param->dim.nr_height < 66) {
        dn_err("Error! noise height [%d] < 66\n", param->dim.nr_height);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] noise height [%d] < 66\n", chn, param->dim.nr_height);
        ret = -1;
    }

    if ((param->dim.src_x + param->dim.nr_width) > param->dim.src_bg_width) {
        dn_err("Error! (src x [%d] + nr width [%d]) over src background width [%d]\n", param->dim.src_x, param->dim.nr_width, param->dim.src_bg_width);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] src x [%d] nr width [%d] src bg width [%d]\n", chn, param->dim.src_x, param->dim.nr_width, param->dim.src_bg_width);
        ret = -1;
    }

    if ((param->dim.src_y + param->dim.nr_height) > param->dim.src_bg_height) {
        dn_err("Error! (src y [%d] + nr height [%d]) over src background height [%d]\n", param->dim.src_y, param->dim.nr_height, param->dim.src_bg_height);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] src y [%d] nr height [%d] src bg height [%d]\n", chn, param->dim.src_y, param->dim.nr_height, param->dim.src_bg_height);
        ret = -1;
    }

    if (param->dim.org_width == 0) {
        dn_err("Error! original height [%d] is zero\n", param->dim.org_width);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] original width [%d] is zero\n", chn, param->dim.org_width);
        ret = -1;
    }

    if (param->dim.org_height == 0) {
        dn_err("Error! original height [%d] is zero\n", param->dim.org_height);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] original height [%d] is zero\n", chn, param->dim.org_height);
        ret = -1;
    }

    if (param->src_fmt != TYPE_YUV422_FRAME) {
        dn_err("Error! 3DNR not support source format %x\n", param->src_fmt);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] 3DNR not support source format [%x]\n", chn, param->src_fmt);
        ret = -1;
    }

    return ret;
}

int vg_set_dn_param(job_node_t *node)
{
    int chan_id = 0;
    job_node_t *ref_node = NULL;
    int ret = 0;

    chan_id = node->chan_id;
    ref_node = global_info.ref_node[chan_id];

    ret = sanity_check(node);
    if (ret < 0) {
        dn_err("Error! sanity check fail\n");
        return -1;
    }

    // channel source is from decoder/sensor
    if (node->param.src_type == SRC_TYPE_DECODER || node->param.src_type == SRC_TYPE_SENSOR) {
        /* copy global default ctrl param */
        memcpy(&node->param.ctrl, &priv.default_param.ctrl, sizeof(ft3dnr_ctrl_t));
    }
    // channel source is from isp
    if (node->param.src_type == SRC_TYPE_ISP) {
        /* copy global isp ctrl param */
        memcpy(&node->param.ctrl, &priv.isp_param.ctrl, sizeof(ft3dnr_ctrl_t));
        node->param.mrnr_id = priv.isp_param.mrnr_id;
        node->param.tmnr_id = priv.isp_param.tmnr_id;
    }

    /* parameter changed, ref_node = NULL, don't reference to previous job */
    if (ref_node == NULL || node->ref_res_changed) {
        /* parameter changed, temporal reference should be disable */
        node->param.ctrl.temporal_en = 0;
    }

    /* calculate histrogram threshold */
    cal_his_param(node);
    /* get mrnr param */
    get_mrnr_param(node, ref_node);
    /* get tmnr param */
    get_tmnr_param(node, ref_node);

    return 0;
}

static inline int assign_frame_buffers(job_node_t *ref_node, job_node_t *node)
{
    int ch;
    int i, start_idx = -1;
    size_t dim_size;

    ch = node->chan_id;

    dim_size = (u32) node->param.dim.src_bg_width * (u32) node->param.dim.src_bg_height;

    if ((priv.chan_param[ch].ref_res_p == NULL) ||
            ((priv.chan_param[ch].ref_res_p) && (priv.chan_param[ch].ref_res_p->size < dim_size)))
    {
        if (priv.chan_param[ch].ref_res_p == NULL) {
            start_idx = priv.res_cfg_cnt - 1;
        } else {
            for (i = 0; i < priv.res_cfg_cnt; i++) {
                if (priv.res_cfg[i].map_chan == ch)
                    break;
            }
            start_idx = i - 1;
        }

        for (i = start_idx; i >= 0 ; i--) {
            // serach first unused and matched ref buffer from little-end
            if ((priv.res_cfg[i].map_chan == MAP_CHAN_NONE) && (priv.res_cfg[i].size >= dim_size))
                break;
        }

        if (i < 0) {
            for (i = start_idx; i >= 0 ; i--) {
                // serach first matched ref buffer from little-end, but mapped to other channel
                if (priv.res_cfg[i].size >= dim_size)
                    break;
            }
        }

        if (i < 0) {
            // specfied resolution buffer not available
            printk("%s: ERROR!!!\n", DRIVER_NAME);
            printk("%s: required resolution %dx%d is over spec\n", DRIVER_NAME,
                node->param.dim.src_bg_width, node->param.dim.src_bg_height);
            printk("%s: current supported resolution list:\n", DRIVER_NAME);
            for (i = 0; i < priv.res_cfg_cnt; i++)
                printk("%s: %d. %dx%d\n", DRIVER_NAME, i, priv.res_cfg[i].width, priv.res_cfg[i].height);

            return -1;
        } else {
            if (priv.res_cfg[i].map_chan != MAP_CHAN_NONE) {
                int ch_id = priv.res_cfg[i].map_chan;

                priv.chan_param[ch_id].ref_res_p = NULL;
                priv.chan_param[ch_id].var_buf_p = NULL;
                priv.res_cfg[i].map_chan = MAP_CHAN_NONE;
            }

            // release old res_cfg
            if (priv.chan_param[ch].ref_res_p)
                priv.chan_param[ch].ref_res_p->map_chan = MAP_CHAN_NONE;

            // allocate new res_cfg
            priv.chan_param[ch].ref_res_p = &priv.res_cfg[i];
            priv.chan_param[ch].var_buf_p = &priv.res_cfg[i].var_buf;
            priv.res_cfg[i].map_chan = ch;

            node->ref_res_changed = true;
        }
    }

    if (ref_node)
        node->param.dma.ref_addr = ref_node->param.dma.dst_addr;
    else
        node->param.dma.ref_addr = 0;

    // VAR buffer address
    node->param.dma.var_addr = priv.chan_param[ch].var_buf_p->paddr;

    return 0;
}

int ft3dnr_init_multi_job(job_node_t *node, ft3dnr_job_t *parent)
{
    job_node_t *curr, *ne, *last_node;

    last_node = list_entry(node->clist.prev, job_node_t, clist);

    list_for_each_entry_safe(curr, ne, &node->clist, clist) {
        ft3dnr_job_t *job = NULL;

        if (in_interrupt() || in_atomic())
            job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
        else
            job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
        if (unlikely(!job))
            panic("%s, No memory for job! \n", __func__);
        MEMCNT_ADD(&priv, 1);
        memset(job, 0x0, sizeof(ft3dnr_job_t));
        memcpy(&job->param, &curr->param, sizeof(ft3dnr_param_t));

        job->job_type = MULTI_JOB;
        job->perf_type = TYPE_MIDDLE;

        if (curr == last_node)
            job->perf_type = TYPE_LAST;

        INIT_LIST_HEAD(&job->job_list);
        job->chan_id = curr->chan_id;
        job->job_id = curr->job_id;
        job->status = JOB_STS_QUEUE;
        job->need_callback = curr->need_callback;
        job->fops = &callback_ops;
        job->parent = parent;
        job->private = curr;

        ft3dnr_joblist_add(job);
    }

    return 0;
}

int ft3dnr_init_job(job_node_t *node, int count)
{
    ft3dnr_job_t *job = NULL;
    ft3dnr_job_t *parent = NULL;

    if (in_interrupt() || in_atomic())
        job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
    else
        job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
    if (unlikely(!job))
        panic("%s, No memory for job! \n", __func__);
    MEMCNT_ADD(&priv, 1);
    memset(job, 0x0, sizeof(ft3dnr_job_t));
    memcpy(&job->param, &node->param, sizeof(ft3dnr_param_t));

    if (count > 0) {
        job->job_type = MULTI_JOB;
        job->perf_type = TYPE_FIRST;
    }
    else {
        job->job_type = SINGLE_JOB;
        job->perf_type = TYPE_ALL;
    }

    INIT_LIST_HEAD(&job->job_list);
    job->chan_id = node->chan_id;
    job->job_id = node->job_id;
    job->status = JOB_STS_QUEUE;
    job->need_callback = node->need_callback;
    job->fops = &callback_ops;
    parent = job;
    job->parent = parent;
    job->private = node;


    ft3dnr_joblist_add(parent);

    if (!list_empty(&node->clist))
        ft3dnr_init_multi_job(node, parent);

    /* add root job to node list */
    vg_nodelist_add(node);

    ft3dnr_put_job();

    return 0;
}

static int driver_putjob(void *parm)
{
    struct video_entity_job_t *job = (struct video_entity_job_t *)parm;
    struct video_entity_job_t *next_job = 0;
    job_node_t  *node_item = 0, *root_node_item = 0, *ref_node_item;
    int multi_jobs = 0, chan_id = 0;
    int ret = 0;

    down(&global_info.sema_lock);

    if (list_empty(&global_info.node_list)) {
        clock_on++;
        platform_clock_on(1);
    }

    down(&priv.sema_lock);
    do {
        chan_id = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
        /*
         * parse the parameters and assign to param structure
         */
        node_item = kmem_cache_alloc(global_info.node_cache, GFP_KERNEL);
        if (node_item == NULL)
            panic("%s, no memory! \n", __func__);
        MEMCNT_ADD(&priv, 1);
        memset(node_item, 0x0, sizeof(job_node_t));

        node_item->engine = ENTITY_FD_ENGINE(job->fd);
        if (node_item->engine > ENTITY_ENGINES)
            panic("Error to use %s engine %d, max is %d\n",MODULE_NAME,node_item->engine,ENTITY_ENGINES);

        node_item->minor = ENTITY_FD_MINOR(job->fd);
        if (node_item->minor > max_minors)
            panic("Error to use %s minor %d, max is %d\n",MODULE_NAME,node_item->minor,max_minors);

        node_item->chan_id = chan_id;

        ret = driver_parse_in_property(node_item, job);
        if (ret < 0)
            goto err;

        /* parameters are changed, should not reference to previous frame */
        if ((ret == 1) && global_info.ref_node[chan_id]) {
            REFCNT_DEC(global_info.ref_node[chan_id]);
            // free reference node's reserved buffer
            ref_node_item = global_info.ref_node[chan_id];

            if (ref_node_item->ref_buf) {
                ret = video_free_buffer(ref_node_item->ref_buf);
                if (ret < 0)
                    printk("[DN] video free buf fail\n");
                ref_node_item->ref_buf = NULL;
                atomic_dec(&priv.buf_cnt);
            }
            global_info.ref_node[chan_id] = NULL;
        }

        ref_node_item = global_info.ref_node[chan_id];

        node_item->job_id = job->id;
        node_item->status = JOB_STS_QUEUE;
        node_item->private = job;
        node_item->refer_to = ref_node_item;
        node_item->puttime = jiffies;
        node_item->starttime = 0;
        node_item->finishtime = 0;

        ret = assign_frame_buffers(ref_node_item, node_item);
        if (ret < 0)
            goto err;

        node_item->param.dma.src_addr = job->in_buf.buf[0].addr_pa;
        node_item->param.dma.dst_addr = job->out_buf.buf[0].addr_pa;

        ret = vg_set_dn_param(node_item);
        if (ret < 0) {
            dn_err("Error! set_dn_param fail\n");
            goto err;
        }

		/* update to new */
        global_info.ref_node[chan_id] = node_item;
        REFCNT_INC(node_item);   //add the reference count of this node

        INIT_LIST_HEAD(&node_item->plist);
        INIT_LIST_HEAD(&node_item->clist);

        if ((next_job = job->next_job) == 0) {    //last job
            node_item->need_callback = 1;
            /* multi job's last job */
            if (multi_jobs > 0) {
                multi_jobs++;
                list_add_tail(&node_item->clist, &root_node_item->clist);
            }
        }
        else {
            node_item->need_callback = 0;
            multi_jobs++;
            if (multi_jobs == 1)                //record root job
                root_node_item = node_item;
            else
                list_add_tail(&node_item->clist, &root_node_item->clist);
        }

        if (multi_jobs > 0) {                   // multi jobs
            node_item->parent = root_node_item;
            node_item->type = MULTI_NODE;
        }
        else {                                  // single job
            root_node_item = node_item;
            node_item->parent = node_item;
            node_item->type = SINGLE_NODE;
        }

        //ft3dnr_init_job(root_node_item, multi_jobs);

    } while((job = next_job));

    ft3dnr_init_job(root_node_item, multi_jobs);
    up(&priv.sema_lock);
    up(&global_info.sema_lock);

    return JOB_STATUS_ONGOING;

err:
    printk("ft3dnr putjob error\n");
    if (multi_jobs == 0) {
        kmem_cache_free(global_info.node_cache, node_item);
        MEMCNT_SUB(&priv, 1);
    }
    else {
        job_node_t *curr, *ne;
        /* free child node */
        list_for_each_entry_safe(curr, ne, &root_node_item->clist, clist) {
            kmem_cache_free(global_info.node_cache, curr);
            MEMCNT_SUB(&priv, 1);
        }
        /* free parent node */
        kmem_cache_free(global_info.node_cache, root_node_item);
        MEMCNT_SUB(&priv, 1);
    }
    up(&priv.sema_lock);
    up(&global_info.sema_lock);
    printm(MODULE_NAME, "PANIC!! putjob error\n");
    damnit(MODULE_NAME);
    return JOB_STATUS_FAIL;
}

struct video_entity_ops_t driver_ops = {
    putjob:      &driver_putjob,
    stop:        &driver_stop,
    queryid:     &driver_queryid,
    querystr:    &driver_querystr,
};

struct video_entity_t ft3dnr_entity={
    type:       TYPE_3DNR,
    name:       "3dnr",
    engines:    ENTITY_ENGINES,
    minors:     ENTITY_MINORS,
    ops:        &driver_ops
};

int ft3dnr_log_panic_handler(int data)
{
    printm(MODULE_NAME, "PANIC!! Processing Start\n");

    printm(MODULE_NAME, "PANIC!! Processing End\n");
    return 0;
}

int ft3dnr_log_printout_handler(int data)
{
    job_node_t  *node, *ne, *curr, *ne1;

    printm(MODULE_NAME, "PANIC!! PrintOut Start\n");

    list_for_each_entry_safe(node, ne, &global_info.node_list ,plist) {
        if(node->status == JOB_STS_ONGOING)
            printm(MODULE_NAME, "ONGOING JOB ID(%x) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback);
        else if(node->status == JOB_STS_QUEUE)
            printm(MODULE_NAME, "PENDING JOB ID(%x) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback);
        else if(node->status == JOB_STS_SRAM)
            printm(MODULE_NAME, "SRAM JOB ID(%x) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback);
        else if(node->status == JOB_STS_DONOTHING)
            printm(MODULE_NAME, "DONOTHING JOB ID(%x) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback);
        else if(node->status == JOB_STS_DONE)
            printm(MODULE_NAME, "DONE JOB ID(%x) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback);
        else
            printm(MODULE_NAME, "FLUSH JOB ID (%x) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback);

        list_for_each_entry_safe(curr, ne1, &node->clist ,clist) {
            if(curr->status == JOB_STS_ONGOING)
                printm(MODULE_NAME, "ONGOING JOB ID(%x) TYPE(%x) need_callback(%d)\n", curr->job_id, curr->type, curr->need_callback);
            else if(curr->status == JOB_STS_QUEUE)
                printm(MODULE_NAME, "PENDING JOB ID(%x) TYPE(%x) need_callback(%d)\n", curr->job_id, curr->type, curr->need_callback);
            else if(node->status == JOB_STS_SRAM)
                printm(MODULE_NAME, "SRAM JOB ID(%x) TYPE(%x) need_callback(%d)\n", curr->job_id, curr->type, curr->need_callback);
            else if(curr->status == JOB_STS_DONOTHING)
                printm(MODULE_NAME, "DONOTHING JOB ID(%x) TYPE(%x) need_callback(%d)\n", curr->job_id, curr->type, curr->need_callback);
            else if(curr->status == JOB_STS_DONE)
                printm(MODULE_NAME, "DONE JOB ID(%x) TYPE(%x) need_callback(%d)\n", curr->job_id, curr->type, curr->need_callback);
            else
                printm(MODULE_NAME, "FLUSH JOB ID (%x) TYPE(%x) need_callback(%d)\n", curr->job_id, curr->type, curr->need_callback);
        }
    }

    printm(MODULE_NAME, "PANIC!! PrintOut End\n");
    return 0;
}

void print_info(void)
{
    return;
}

static struct property_map_t *driver_get_propertymap(int id)
{
    int i;

    for(i = 0; i < sizeof(property_map) / sizeof(struct property_map_t); i++) {
        if(property_map[i].id == id) {
            return &property_map[i];
        }
    }
    return 0;
}


void print_filter(void)
{
    int i;

    printk("\nUsage:\n#echo [0:exclude/1:include] Engine Minor > filter\n");
    printk("Driver log Include:");

    for(i = 0;i < MAX_FILTER; i++)
        if(include_filter_idx[i] >= 0)
            printk("{%d,%d},",IDX_ENGINE(include_filter_idx[i],ENTITY_MINORS),
                IDX_MINOR(include_filter_idx[i],ENTITY_MINORS));

    printk("\nDriver log Exclude:");
    for(i = 0; i < MAX_FILTER; i++)
        if(exclude_filter_idx[i] >= 0)
            printk("{%d,%d},",IDX_ENGINE(exclude_filter_idx[i],ENTITY_MINORS),
                IDX_MINOR(exclude_filter_idx[i],ENTITY_MINORS));
    printk("\n");
}

static int vg_proc_cb_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\nCallback Period = %d (ticks)\n",callback_period);

    return 0;
}

static ssize_t vg_proc_cb_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  mode_set;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &mode_set);

    callback_period = mode_set;

    seq_printf(sfile, "\nCallback Period =%d (ticks)\n",callback_period);

    return count;
}

static int vg_proc_filter_show(struct seq_file *sfile, void *v)
{
    print_filter();

    return 0;
}

static ssize_t vg_proc_filter_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int i;
    int engine,minor,mode;

    sscanf(buffer, "%d %d %d",&mode,&engine,&minor);

    if (mode == 0) { //exclude
        for (i = 0; i < MAX_FILTER; i++) {
            if (exclude_filter_idx[i] == -1) {
                exclude_filter_idx[i] = (engine << 16) | (minor);
                break;
            }
        }
    } else if (mode == 1) {
        for (i = 0; i < MAX_FILTER; i++) {
            if (include_filter_idx[i] == -1) {
                include_filter_idx[i] = (engine << 16) | (minor);
                break;
            }
        }
    }
    print_filter();
    return count;
}

static int vg_proc_info_show(struct seq_file *sfile, void *v)
{
    print_info();

    return 0;
}

static ssize_t vg_proc_info_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    print_info();

    return count;
}

static int vg_proc_job_show(struct seq_file *sfile, void *v)
{
    unsigned long flags;
    struct video_entity_job_t *job;
    job_node_t  *node, *child;
    char *st_string[] = {"QUEUE", "SRAM", "ONGOING", "FINISH", "FLUSH", "DONOTHING"}, *string = NULL;
    char *type_string[]={"   ","(M)","(*)"}; //type==0,type==1 && need_callback=0, type==1 && need_callback=1

    seq_printf(sfile, "\nSystem ticks=0x%x\n", (int)jiffies & 0xffff);

    seq_printf(sfile, "Chnum  Job_ID     Status     Puttime      start    end \n");
    seq_printf(sfile, "===========================================================\n");

    spin_lock_irqsave(&global_info.lock, flags);

    list_for_each_entry(node, &global_info.node_list, plist) {
        if (node->status & JOB_STS_QUEUE)       string = st_string[0];
        if (node->status & JOB_STS_SRAM)        string = st_string[1];
        if (node->status & JOB_STS_ONGOING)     string = st_string[2];
        if (node->status & JOB_STS_DONE)        string = st_string[3];
        if (node->status & JOB_STS_FLUSH)       string = st_string[4];
        if (node->status & JOB_STS_DONOTHING)   string = st_string[5];
        job = (struct video_entity_job_t *)node->private;
        seq_printf(sfile, "%-5d %s %-9d  %-9s  %08x  %08x  %08x \n", node->minor,
        type_string[(node->type*0x3)&(node->need_callback+1)],
        job->id, string, node->puttime, node->starttime, node->finishtime);

        list_for_each_entry(child, &node->clist, clist) {
            if (child->status & JOB_STS_QUEUE)       string = st_string[0];
            if (child->status & JOB_STS_SRAM)        string = st_string[1];
            if (child->status & JOB_STS_ONGOING)     string = st_string[2];
            if (child->status & JOB_STS_DONE)        string = st_string[3];
            if (child->status & JOB_STS_FLUSH)       string = st_string[4];
            if (child->status & JOB_STS_DONOTHING)   string = st_string[5];
            job = (struct video_entity_job_t *)child->private;
            seq_printf(sfile, "%-5d %s %-9d  %-9s  %08x  %08x  %08x \n", child->minor,
            type_string[(child->type*0x3)&(child->need_callback+1)],
            job->id, string, child->puttime, child->starttime, child->finishtime);
        }
    }

    spin_unlock_irqrestore(&global_info.lock, flags);

    return 0;
}

static int vg_proc_level_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\nLog level = %d (0:emerge 1:error 2:debug)\n", log_level);

    return 0;
}

static ssize_t vg_proc_level_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int  level;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &level);

    log_level = level;

    seq_printf(sfile, "\nLog level = %d (0:emerge 1:error 2:debug)\n", log_level);

    return count;
}
static unsigned int property_engine = 0, property_minor = 0;
static int vg_proc_property_show(struct seq_file *sfile, void *v)
{
    int i = 0;
    struct property_map_t *map;
    unsigned int id, value, ch;
    unsigned long flags;
    unsigned int address = 0, size = 0;

    int idx = MAKE_IDX(ENTITY_MINORS, property_engine, property_minor);

    spin_lock_irqsave(&global_info.lock, flags);

    seq_printf(sfile, "\n%s engine%d ch%d job %d\n",property_record[idx].entity->name,
        property_engine,property_minor,property_record[idx].job_id);
    seq_printf(sfile, "=============================================================\n");
    seq_printf(sfile, "ID  Name(string) Value(hex)  Readme\n");
    do {
        ch=property_record[idx].property[i].ch;
        id=property_record[idx].property[i].id;
        if(id==ID_NULL)
            break;
        value=property_record[idx].property[i].value;
        map=driver_get_propertymap(id);
        if(map) {
            //printk("%02d  %12s  %09d  %s\n",id,map->name,value,map->readme);
            seq_printf(sfile, "%02d %02d  %12s  %08x  %s\n",ch, id,map->name,value,map->readme);
        }
        i++;
    } while(1);
#if 1
    address = priv.engine.ft3dnr_reg;   ////<<<<<<<< Update your register dump here >>>>>>>>>//issue
    size=0xe4;  //<<<<<<<< Update your register dump here >>>>>>>>>
    for (i = 0; i < size; i = i + 0x10) {
        seq_printf(sfile, "0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address+i),*(unsigned int *)(address+i),
            *(unsigned int *)(address+i+4),*(unsigned int *)(address+i+8),*(unsigned int *)(address+i+0xc));
    }
    seq_printf(sfile, "\n");
#endif
    spin_unlock_irqrestore(&global_info.lock, flags);

    return 0;
}

static ssize_t vg_proc_property_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int mode_engine = 0, mode_minor = 0;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &mode_engine, &mode_minor);

    property_engine = mode_engine;
    property_minor = mode_minor;

    seq_printf(sfile, "\nLookup engine=%d minor=%d\n",property_engine,property_minor);

    return count;
}

static int vg_proc_util_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\n HW Utilization Period=%d(sec) Utilization=%d\n",
        utilization_period,utilization_record[0]);

    return 0;
}

static int vg_proc_version_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\nversion = %d\n", VERSION);
    seq_printf(sfile, "\nclock_on = %d\n", clock_on);
    seq_printf(sfile, "\nclock_off = %d\n", clock_off);
    seq_printf(sfile, "\nmem cnt = %d\n", atomic_read(&priv.mem_cnt));
    seq_printf(sfile, "\nbuf cnt = %d\n", atomic_read(&priv.buf_cnt));

    return 0;
}

static int vg_proc_reserved_buf_show(struct seq_file *sfile, void *v)
{
    job_node_t *node, *ne;
    int i = 0;
    unsigned long flags;

    spin_lock_irqsave(&global_info.lock, flags);

    seq_printf(sfile, "\n3dnr has %d var buf\n", priv.res_cfg_cnt);

    for (i = 0; i < priv.res_cfg_cnt; i++) {
        seq_printf(sfile, "\nvar buf %d w/h %d/%d\n", i, priv.res_cfg[i].width, priv.res_cfg[i].height);
    }

    for (i = 0; i < priv.res_cfg_cnt; i++) {
        if (priv.res_cfg[i].map_chan != MAP_CHAN_NONE)
            seq_printf(sfile, "\nvar buf %d belong to minor %d\n", i, priv.res_cfg[i].map_chan);
    }

    list_for_each_entry_safe(node, ne, &global_info.node_list, plist) {
        if (node->ref_buf) {
            seq_printf(sfile, "\nnode %d minor %d reserved buf w/h %d/%d\n", node->job_id, node->minor, node->param.dim.src_bg_width, node->param.dim.src_bg_height);
        }
    }

    seq_printf(sfile, "\nbuf cnt = %d\n", atomic_read(&priv.buf_cnt));

    spin_unlock_irqrestore(&global_info.lock, flags);

    return 0;
}

static ssize_t vg_proc_util_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int mode_set = 0;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d \n", &mode_set);
    utilization_period = mode_set;

    seq_printf(sfile, "\nUtilization Period =%d(sec)\n",utilization_period);

    return count;
}

static int vg_proc_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_info_show, PDE(inode)->data);
}

static int vg_proc_cb_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_cb_show, PDE(inode)->data);
}

static int vg_proc_filter_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_filter_show, PDE(inode)->data);
}

static int vg_proc_job_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_job_show, PDE(inode)->data);
}

static int vg_proc_level_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_level_show, PDE(inode)->data);
}

static int vg_proc_property_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_property_show, PDE(inode)->data);
}

static int vg_proc_util_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_util_show, PDE(inode)->data);
}

static int vg_proc_version_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_version_show, PDE(inode)->data);
}

static int vg_proc_reserved_buf_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_reserved_buf_show, PDE(inode)->data);
}

static struct file_operations vg_proc_cb_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_cb_open,
    .write  = vg_proc_cb_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_info_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_info_open,
    .write  = vg_proc_info_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_filter_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_filter_open,
    .write  = vg_proc_filter_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_job_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_job_open,
    //.write  = vg_proc_job_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_level_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_level_open,
    .write  = vg_proc_level_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_property_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_property_open,
    .write  = vg_proc_property_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_util_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_util_open,
    .write  = vg_proc_util_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_version_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_version_open,
    //.write  = vg_proc_version_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_reserved_buf_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_reserved_buf_open,
    //.write  = vg_proc_reserved_buf_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

#define ENTITY_PROC_NAME "videograph/ft3dnr"

static int vg_proc_init(void)
{
    int ret = 0;

    /* create proc */
    vg_proc_info.root = create_proc_entry(ENTITY_PROC_NAME, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(vg_proc_info.root == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        ret = -EFAULT;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.root->owner = THIS_MODULE;
#endif

    /* info */
    vg_proc_info.info = create_proc_entry("info", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.info == NULL) {
        printk("error to create %s/info proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    (vg_proc_info.info)->proc_fops  = &vg_proc_info_ops;
    (vg_proc_info.info)->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    (vg_proc_info.info)->owner      = THIS_MODULE;
#endif

    /* cb */
    vg_proc_info.cb = create_proc_entry("callback_period", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.cb == NULL) {
        printk("error to create %s/cb proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    (vg_proc_info.cb)->proc_fops  = &vg_proc_cb_ops;
    (vg_proc_info.cb)->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.cb->owner      = THIS_MODULE;
#endif

    /* utilization */
    vg_proc_info.util = create_proc_entry("utilization", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.util == NULL) {
        printk("error to create %s/util proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.util->proc_fops  = &vg_proc_util_ops;
    vg_proc_info.util->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.util->owner      = THIS_MODULE;
#endif

    /* property */
    vg_proc_info.property = create_proc_entry("property", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.property == NULL) {
        printk("error to create %s/property proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.property->proc_fops  = &vg_proc_property_ops;
    vg_proc_info.property->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.property->owner      = THIS_MODULE;
#endif

    /* job */
    vg_proc_info.job = create_proc_entry("job", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.job == NULL) {
        printk("error to create %s/job proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.job->proc_fops  = &vg_proc_job_ops;
    vg_proc_info.job->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.job->owner      = THIS_MODULE;
#endif

    /* filter */
    vg_proc_info.filter = create_proc_entry("filter", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.filter == NULL) {
        printk("error to create %s/filter proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.filter->proc_fops  = &vg_proc_filter_ops;
    vg_proc_info.filter->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.filter->owner      = THIS_MODULE;
#endif

    /* level */
    vg_proc_info.level = create_proc_entry("level", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.level == NULL) {
        printk("error to create %s/level proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.level->proc_fops  = &vg_proc_level_ops;
    vg_proc_info.level->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.level->owner      = THIS_MODULE;
#endif

    /* version */
    vg_proc_info.version = create_proc_entry("version", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.version == NULL) {
        printk("error to create %s/version proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.version->proc_fops  = &vg_proc_version_ops;
    vg_proc_info.version->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.version->owner      = THIS_MODULE;
#endif

    /* reserved buffer */
    vg_proc_info.reserved_buf = create_proc_entry("reserved_buf", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.reserved_buf == NULL) {
        printk("error to create %s/reserved_buf proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.reserved_buf->proc_fops  = &vg_proc_reserved_buf_ops;
    vg_proc_info.reserved_buf->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.reserved_buf->owner      = THIS_MODULE;
#endif

    return 0;
}

void vg_proc_remove(void)
{
    if(vg_proc_info.root) {
        if (vg_proc_info.cb != 0)
            remove_proc_entry(vg_proc_info.cb->name, vg_proc_info.root);
        if (vg_proc_info.util != 0)
            remove_proc_entry(vg_proc_info.util->name, vg_proc_info.root);
        if (vg_proc_info.property != 0)
            remove_proc_entry(vg_proc_info.property->name, vg_proc_info.root);
        if (vg_proc_info.job != 0)
            remove_proc_entry(vg_proc_info.job->name, vg_proc_info.root);
        if (vg_proc_info.filter != 0)
            remove_proc_entry(vg_proc_info.filter->name, vg_proc_info.root);
        if (vg_proc_info.level != 0)
            remove_proc_entry(vg_proc_info.level->name, vg_proc_info.root);
        if (vg_proc_info.info!=0)
            remove_proc_entry(vg_proc_info.info->name, vg_proc_info.root);
        if (vg_proc_info.version!=0)
            remove_proc_entry(vg_proc_info.version->name, vg_proc_info.root);
        if (vg_proc_info.reserved_buf!=0)
            remove_proc_entry(vg_proc_info.reserved_buf->name, vg_proc_info.root);

        remove_proc_entry(vg_proc_info.root->name, vg_proc_info.root->parent);
    }
}


int ft3dnr_vg_init(void)
{
    int ret = 0;

    ft3dnr_entity.minors = max_minors;

    property_record = kzalloc(sizeof(struct property_record_t) * ENTITY_ENGINES * max_minors, GFP_KERNEL);
    if (ZERO_OR_NULL_PTR(property_record))
        panic("%s: allocate memory fail for property_record(0x%p)!\n", __func__, property_record);

    /* global information */
    memset(&global_info, 0x0, sizeof(global_info_t));
    INIT_LIST_HEAD(&global_info.node_list);

    video_entity_register(&ft3dnr_entity);

    /* spinlock */
    spin_lock_init(&global_info.lock);

    /* init semaphore lock */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
    sema_init(&global_info.sema_lock, 1);
#else
    init_MUTEX(&global_info.sema_lock);
#endif

    /* create memory cache */
    global_info.node_cache = kmem_cache_create("ft3dnr_vg", sizeof(job_node_t), 0, 0, NULL);
    if (global_info.node_cache == NULL)
        panic("ft3dnr: fail to create cache!");
#ifdef USE_WQ
    /* create workqueue */
    callback_workq = create_workqueue("ft3dnr_callback");
    if (callback_workq == NULL) {
        printk("ft3dnr: error in create callback workqueue! \n");
        return -1;
    }
#endif
#ifdef USE_KTHREAD
    init_waitqueue_head(&cb_thread_wait);
    global_info.cb_task = kthread_create(cb_thread, NULL, "cb_thread");
    if (IS_ERR(global_info.cb_task))
        panic("%s, create ep_task fail! \n", __func__);
    wake_up_process(global_info.cb_task);
#endif
    /* register log system callback function */
    ret = register_panic_notifier(ft3dnr_log_panic_handler);
    if(ret < 0) {
        printk("ft3dnr register log system panic notifier failed!\n");
        return -1;
    }

    ret = register_printout_notifier(ft3dnr_log_printout_handler);
    if(ret < 0) {
        printk("ft3dnr register log system printout notifier failed!\n");
        return -1;
    }

    /* vg proc init */
    ret = vg_proc_init();
    if(ret < 0)
        printk("ft3dnr videograph proc node init failed!\n");

    memset(engine_time, 0, sizeof(unsigned int));
    memset(engine_start, 0, sizeof(unsigned int));
    memset(engine_end, 0, sizeof(unsigned int));
    memset(utilization_start, 0, sizeof(unsigned int));
    memset(utilization_record, 0, sizeof(unsigned int));

    return 0;
}

void ft3dnr_vg_driver_clearnup(void)
{
#ifdef USE_WQ
    /* cancel all works */
    cancel_delayed_work(&process_callback_work);
#endif
#ifdef USE_KTHREAD
    if (global_info.cb_task)
        kthread_stop(global_info.cb_task);

    while (cb_running == 1)
        msleep(1);
#endif
    /* vg proc remove */
    vg_proc_remove();

    video_entity_deregister(&ft3dnr_entity);
#ifdef USE_WQ
    /* destroy workqueue */
    destroy_workqueue(callback_workq);
#endif
    kfree(property_record);
    kmem_cache_destroy(global_info.node_cache);
}
