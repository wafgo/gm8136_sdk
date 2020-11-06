/**
 * @file template.c
 *  The videograph interface of driver template.
 * Copyright (C) 2011 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.4 $
 * $Date: 2012/11/01 06:48:03 $
 *
 * ChangeLog:
 *  $Log: fmjpeg_enc_vg2.c,v $
 *  Revision 1.4  2012/11/01 06:48:03  ivan
 *  take off snapshoot count
 *
 *  Revision 1.3  2012/10/30 03:22:53  ivan
 *  take off MAX_VE_ID_NUM instead of only MAX_PROPERTYS definition needed
 *
 *  Revision 1.2  2012/10/29 05:11:14  ivan
 *  fix type error
 *
 *  Revision 1.1.1.1  2012/10/12 08:35:59  ivan
 *
 *
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/miscdevice.h>
#include <linux/kallsyms.h>
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/string.h>

#include "log.h"
#include "video_entity.h"
#include "rotation_vg2.h"
#include "rotation_node.h"
#include "ioctl_rotation.h"
#include "../fmcp.h"
#undef  PFX
#define PFX	        "RT"
#include "debug.h"


#define MAX_NAME    50
#define MAX_README  100
//#define DUMP_FRAMERATE

struct rt_property_map_t {
    int id;
    char name[MAX_NAME];
    char readme[MAX_README];
};

enum st_property_id {
    ID_SAMPLE,
    ID_RESTART_INTERVAL,
    ID_IMAGE_QUALITY,
};

struct rt_property_map_t rt_property_map[] = {
    {ID_SRC_FMT,"src_fmt",""},              // in prop: source format
    {ID_SRC_DIM,"src_dim",""},              // in prop: source dim
    {ID_CLOCKWISE,"clockwise",""},          // in prop: sample
    {ID_DST_DIM,"dst_dim",""},              // out prop: output dim
};

extern int rt_proc_init(void);
extern void rt_proc_exit(void);
extern unsigned int video_gettime_us(void);
extern int FRotation_Trigger(FMCP_RT_PARAM *rt_param);

/* utilization */
unsigned int rt_utilization_period = 5; //5sec calculation
unsigned int rt_utilization_start = 0, rt_utilization_record= 0;
unsigned int rt_engine_start = 0, rt_engine_end = 0;
unsigned int rt_engine_time = 0;
#ifdef DUMP_FRAMERATE
    unsigned int rt_frame_cnt = 0;
#endif

/* property lastest record */
struct rt_property_record_t *rt_property_record;

/* log & debug message */
unsigned int rt_log_level = LOG_WARNING;    //larger level, more message

/* variable */
struct rt_data_t   rt_private_data[RT_MAX_CHANNEL];

#if 0
#define DEBUG_M(level, fmt, args...) { \
    if (rt_log_level >= level) \
        printm(RT_MODULE_NAME, fmt, ## args); }
#else
#define DEBUG_M(level, fmt, args...) { \
    if (rt_log_level >= level) \
        printk(fmt, ## args);}
#endif
#define DEBUG_E(fmt, args...) { \
    printm(RT_MODULE_NAME,  fmt, ## args); \
    printk("[rt]" fmt, ##args); }

static void rt_print_property(struct video_entity_job_t *job, struct video_property_t *property)
{
    int i;
    int minor = ENTITY_FD_MINOR(job->fd);
    
    for (i = 0; i < MAX_PROPERTYS; i++) {
        if (property[i].id == ID_NULL)
            break;
        if (i == 0)
            DEBUG_M(LOG_INFO, "{chn%d} job %d property:\n", minor, job->id);
        DEBUG_M(LOG_INFO, "  ID:%d,Value:%d\n", property[i].id, property[i].value);
    }
}

int rt_parse_in_property(struct rt_data_t *rt_data, struct video_entity_job_t *job)
{
    int i = 0;
    //struct rt_data_t *rt_data = (struct rt_data_t *)param;
    //int idx = MAKE_IDX(MJE_MAX_CHANNEL, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
    int idx = rt_data->chn;
    struct video_property_t *property = job->in_prop;

    rt_data->src_fmt = TYPE_YUV422_FRAME;
    rt_data->src_dim = 0;
    rt_data->clockwise = -1;

    while (property[i].id != 0) {
        switch (property[i].id) {
            case ID_SRC_FMT:
                rt_data->src_fmt = property[i].value;
                break;
            case ID_SRC_DIM:
                rt_data->src_dim = property[i].value;
                break;
            case ID_CLOCKWISE:
                rt_data->clockwise = property[i].value;
                break;
            default:
                break;
        }
        if (i < RT_MAX_RECORD) {
            rt_property_record[idx].property[i].id = property[i].id;
            rt_property_record[idx].property[i].value = property[i].value;
        }
        i++;
    }
    rt_property_record[idx].property[i].id = rt_property_record[idx].property[i].value = 0;
    rt_property_record[idx].entity = job->entity;
    rt_property_record[idx].job_id = job->id;
    rt_print_property(job,job->in_prop);
    
    /*
    if (0 == rt_data->src_dim) {
        DEBUG_E("src_dim muse be filled and can not be zero\n");
        return -1;
    }
    if (TYPE_YUV422_FRAME != rt_data->src_fmt) {
        DEBUG_E("only support src_fmt YUV422 FRAME, input is %d\n", rt_data->src_fmt);
        return -1;
    }
    if (0 != ID_CLOCKWISE && 1 != ID_CLOCKWISE) {
        DEBUG_E("closewise must be 0 or 1, input is %d\n", rt_data->clockwise);
        return -1;
    }
    */
    
    return 1;
}

int rt_set_out_property(struct rt_data_t *rt_data, struct video_entity_job_t *job)
{
    int i = 0;
    //struct rt_data_t *rt_data = (struct rt_data_t *)param;
    struct video_property_t *property = job->out_prop;

    property[i].id = ID_DST_DIM;
    property[i].value = EM_PARAM_DIM(rt_data->output_width, rt_data->output_height);
    i++;

    property[i].id = ID_NULL;
    property[i].value = 0;
    i++;
    
    rt_print_property(job, job->out_prop);
    return 1;
}

static void rt_mark_engine_start(void)
{
    if (rt_engine_start != 0)
        DEBUG_M(LOG_WARNING, "Warning to nested use rt_mark_engine_start function!\n");
    rt_engine_start = video_gettime_us();
    rt_engine_end = 0;
    if (rt_utilization_start == 0) {
        rt_utilization_start = rt_engine_start;
        rt_engine_time = 0;
    #ifdef DUMP_FRAMERATE
        rt_frame_cnt = 0;
    #endif
    }
}

static void rt_mark_engine_finish(void)
{
    if (rt_engine_end != 0)
        DEBUG_M(LOG_WARNING, "Warning to nested use rt_mark_engine_finish function!\n");
    rt_engine_end = video_gettime_us();
    if (rt_engine_end > rt_engine_start) {
        rt_engine_time += rt_engine_end - rt_engine_start;
    #ifdef DUMP_FRAMERATE
        rt_frame_cnt++;
    #endif
    }
    if (rt_utilization_start > rt_engine_end) {
        rt_utilization_start = 0;
        rt_engine_time = 0;
    #ifdef DUMP_FRAMERATE
        rt_frame_cnt = 0;
    #endif
    }
    else if ((rt_utilization_start <= rt_engine_end) &&
            (rt_engine_end - rt_utilization_start >= rt_utilization_period*1000000)) {
        unsigned int utilization;
        utilization = (unsigned int)((100 * rt_engine_time) /
            (rt_engine_end - rt_utilization_start));
        if (utilization)
            rt_utilization_record = utilization;
    #ifdef DUMP_FRAMERATE
        printk("%d fps (frame cnt = %d, %d sec)\n", rt_frame_cnt/rt_utilization_period, rt_frame_cnt, rt_utilization_period);
        rt_frame_cnt = 0;
    #endif
        rt_utilization_start = 0;
        rt_engine_time = 0;
    }        
    rt_engine_start = 0;
}

int rt_preprocess(void *parm, void *priv)
{
    struct job_item_t *job_item = (struct job_item_t *)parm;
    return video_preprocess(job_item->job, priv);
}


int rt_postprocess(void *parm, void *priv)
{
    struct job_item_t *job_item = (struct job_item_t *)parm;
    return video_postprocess(job_item->job, priv);
}

int rt_int;
//extern int FJpegEE_Tri(void *enc_handle, FJPEG_ENC_FRAME * ptFrame);
//extern int FJpegEE_End(void *enc_handle);
//extern void FJpegEE_show(void *dec_handle);

int frt_parm_info(int engine, int chn)
{
    /*
	struct rt_data_t *rt_data;

	if (chn >= RT_MAX_CHANNEL) {
		DEBUG_E ("chn (%d) over MAX", chn);
		return 0;
	}
	rt_data = &rt_private_data[chn];

	if (encode_data->handler) {
		info ("== chn (%d) info begin ==", chn);
		FJpegEE_show (encode_data->handler);
		info ("== chn (%d) info stop ==", chn);
	}
	*/
	return 0;
}

void frt_parm_info_f(struct job_item_t *job_item)
{
    /*
	struct rt_data_t *rt_data = &rt_private_data[job_item->chn];

	info ("====== rt_parm_info_f ======");
	info ("dev = %d, job_id = %d, stop = %d", job_item->chn, 
        job_item->job_id, encode_data->stop_job);
    */
	//info ("encode_data->handler = 0x%08x", (int)encode_data->handler);
	//info ("snapshot_cnt = %d", encode_data->snapshot_cnt);
}
/*
void fmje_frame_info(struct job_item_t *job, FJPEG_ENC_FRAME *encf)
{
	info ("bitstream_size = %d", encf->bitstream_size);
	info ("u32ImageQuality = %d", encf->u32ImageQuality);
	info ("u8JPGPIC = %d", encf->u8JPGPIC);
	info ("roi xywh = %d %d %d %d", encf->roi_x, encf->roi_y, encf->roi_w, encf->roi_h);
}
*/
#ifdef HOST_MODE
#define REC_PATH    "/mnt/nfs"
static struct workqueue_struct *rt_dump_buf_wq = NULL;
static DECLARE_DELAYED_WORK(process_rt_dump_work, 0);

unsigned int log_addr;
unsigned int log_size;
static void En_log_File(void)
{
	int ret;
	struct file * fd1 = NULL;
	mm_segment_t fs;
	unsigned char fn[0x80];
	unsigned long long offset = 0;

    //dump_log_busy = 1;

	fs = get_fs();
	set_fs(KERNEL_DS);

    sprintf(fn, "%s/log_buf.yuv", REC_PATH);
	fd1 = filp_open(fn, O_WRONLY|O_CREAT, 0777);
	if (IS_ERR(fd1)) {
		printk ("Error to open %s", fn);
		return;
	}

	ret = vfs_write(fd1, (unsigned char*)log_addr, log_size, &offset);
	if(ret <= 0)
		printk ("Error to write En_log_File %s\n", fn);
	filp_close(fd1, NULL);
	set_fs(fs);
	printk("dump buffer done\n");
}

#endif

static void rt_fill_parameter(struct rt_data_t *rt_data, FMCP_RT_PARAM *pParam)
{
    pParam->u32BaseAddress = (unsigned int)mcp100_dev->va_base;
    pParam->u32InputBufferPhy = rt_data->in_addr_pa;
	pParam->u32OutputBufferPhy = rt_data->out_addr_pa;
	pParam->u32Width = rt_data->width;
	pParam->u32Height = rt_data->height;
    pParam->u32Clockwise = rt_data->clockwise;
}

int frt_encode_trigger(struct job_item_t *job_item)
{
	int chn;
    struct video_entity_job_t *job;
	struct rt_data_t *rt_data;
	FMCP_RT_PARAM rt_param;

    DEBUG_M(LOG_DEBUG, "rt start job %d\n", job_item->job_id);
    job = (struct video_entity_job_t *)job_item->job;
    chn = job_item->chn;
	if (chn >= RT_MAX_CHANNEL) {
		DEBUG_E ("chn (%d) over MAX", chn);
		return -1;
	}
    rt_data = &rt_private_data[chn];

    rt_parse_in_property(rt_data, job);
    
#ifdef ENABLE_CHECKSUM
    job_item->check_sum_type = 0;
#endif

    // check parameter
    rt_data->width = EM_PARAM_WIDTH(rt_data->src_dim);
    rt_data->height = EM_PARAM_HEIGHT(rt_data->src_dim);
    if (0 == rt_data->width || 0 == rt_data->height) {
        DEBUG_E("input width and height can not be zero (%d x %d)\n", rt_data->width, rt_data->height);
        return -1;
    }
    if ((rt_data->width&0x0F) && (rt_data->height&0x0F)) {
        DEBUG_E("width and height must be mutiple of 16 (%d x %d)\n", rt_data->width, rt_data->height);
        return -1;
    }
    if (TYPE_YUV422_FRAME != rt_data->src_fmt) {
        DEBUG_E("only support src_fmt YUV422 FRAME, input is %d\n", rt_data->src_fmt);
        return -1;
    }
    if (0 != rt_data->clockwise && 1 != rt_data->clockwise) {
        DEBUG_E("closewise must be 0 or 1, input is %d\n", rt_data->clockwise);
        return -1;
    }
	if (0 == job->in_buf.buf[0].addr_va) {
        DEBUG_E("job input buffer is NULL\n");
        //damnit(RT_MODULE_NAME);
        return -1;
    }
    if (0 == job->out_buf.buf[0].addr_va) {
        DEBUG_E("job output buffer is NULL\n");
        return -1;
    }
    rt_data->in_addr_pa = job->in_buf.buf[0].addr_pa;
    rt_data->out_addr_pa = job->out_buf.buf[0].addr_pa;

	if (rt_preprocess (job_item, NULL) < 0) {
        DEBUG_M(LOG_WARNING, "video_preprocess return fail");
		return -1;					// FAIL
	}
	
	rt_fill_parameter(rt_data, &rt_param);
#ifdef HOST_MODE
    //memset(job->out_buf.buf[0].addr_va, 0, rt_data->width * rt_data->height * 2);
#endif
	if (FRotation_Trigger(&rt_param) < 0)
	    return -1;
//printk("out addr 0x%x/0x%x\n", job->out_buf.buf[0].addr_va, job->out_buf.buf[0].addr_pa);
#ifdef HOST_MODE
    log_addr = job->out_buf.buf[0].addr_va;
    log_size = rt_data->width * rt_data->height * 2;
    PREPARE_DELAYED_WORK(&process_rt_dump_work, (void *)En_log_File);
    queue_delayed_work(rt_dump_buf_wq, &process_rt_dump_work, 1);
    return -1;
#endif
	    
	rt_mark_engine_start();
	return 0;
}

static int frt_process_done(struct job_item_t *job_item)
{
    struct rt_data_t *rt_data = &rt_private_data[job_item->chn];
    rt_mark_engine_finish();
    
    rt_data->output_width = rt_data->height;
    rt_data->output_height = rt_data->width;

    rt_set_out_property(rt_data, (struct video_entity_job_t *)job_item->job);

	if (rt_postprocess(job_item, (void *)NULL) < 0) {
		DEBUG_E("video_postprocess return fail\n");
		return -1;
	}

	return 0;
}

static void frt_isr(int irq, void *dev_id, unsigned int base)
{
#if 1
    *(volatile unsigned int *)(base + 0x20098)=0x80000000;  // clear interrupt
    /*
    {
        volatile Share_Node_RT *node = (volatile Share_Node_RT *) (0x8000 + base);
        int i;
        for (i = 0; i < 4; i++)
            printk("%d\n", node->rthw.dbg[i]);
            //printk("%2d: 0x%08x\n", i, node->rthw.dbg[i]);
    }
    */
#else
	unsigned int int_sts;
    *(volatile unsigned int *)(base + 0x20098)	
	#define REG_OFFSET      0x20000
	#define INT_FLAG		(REG_OFFSET + 0x0098)
	#define mSEQ_INTFLG(base)	    (*(volatile unsigned int*)(INT_FLAG+ base))	// ben
	
	int_sts = mSEQ_INTFLG(base);
	// clr interrupt
	mSEQ_INTFLG(base) = int_sts;
#endif
}

static int rt_putjob(void *parm)
{
    mcp_putjob((struct video_entity_job_t*)parm, TYPE_MCP100_ROTATION);
    //mcp_putjob((struct video_entity_job_t*)parm, TYPE_MCP100_ROTATION);
    return JOB_STATUS_ONGOING;
}

static int rt_stop(void *parm, int engine, int minor)
{
    if (minor >= RT_MAX_CHANNEL) {
        DEBUG_E("chn %d is over max channel %d\n", minor, RT_MAX_CHANNEL);
    }
    else {
        mcp_stopjob(TYPE_MCP100_ROTATION, engine, minor);
        //mcp_stopjob(TYPE_MCP100_ROTATION, engine, minor);
    }
    //encode_data = &mje_private_data[minor];
    //encode_data->stop_job = 1;

    return 0;
}

static struct rt_property_map_t *rt_get_propertymap(int id)
{
    int i;
    
    for (i = 0; i < sizeof(rt_property_map)/sizeof(struct rt_property_map_t); i++) {
        if (rt_property_map[i].id == id) {
            return &rt_property_map[i];
        }
    }
    return 0;
}

static int rt_queryid(void *parm, char *str)
{
    int i;
    
    for (i = 0; i < sizeof(rt_property_map)/sizeof(struct rt_property_map_t); i++) {
        if (strcmp(rt_property_map[i].name,str) == 0) {
            return rt_property_map[i].id;
        }
    }
    printk("rt_queryid: Error to find name %s\n", str);
    return -1;
}


static int rt_querystr(void *parm, int id, char *string)
{
    int i;
    for (i = 0; i < sizeof(rt_property_map)/sizeof(struct rt_property_map_t); i++) {
        if (rt_property_map[i].id == id) {
            memcpy(string, rt_property_map[i].name, sizeof(rt_property_map[i].name));
            return 0;
        }
    }
    printk("rt_querystr: Error to find id %d\n", id);
    return -1;
}

static int rt_getproperty(void *parm, int engine, int minor, char *string)
{
    int id,value = -1;
    struct rt_data_t *rt_data;

    if (minor >= RT_MAX_CHANNEL) {
        printk("Error over minor number %d\n", RT_MAX_CHANNEL);
        return -1;
    }

    rt_data = &rt_private_data[minor];
    id = rt_queryid(parm, string);
    switch (id) {
        case ID_SRC_FMT:
            value = rt_data->src_fmt;
            break;
        case ID_SRC_DIM:
            value = rt_data->src_dim;
            break;
        case ID_CLOCKWISE:
            value = rt_data->clockwise;
            break;
        case ID_DST_DIM:
            value = EM_PARAM_DIM(rt_data->output_width, rt_data->output_height);
            break;
        default:
            break;
    }
    return value;
}

int rt_dump_property_value(char *str, int chn)
{
    // dump property
    int i = 0;
    struct rt_property_map_t *map;
    unsigned int id, value;
    //int engine = 0;
    int idx = chn;
    int len = 0;
    
    if (chn < 0 || chn >= RT_MAX_CHANNEL)
        return sprintf(str+len, "dump property: chn%d is out of range\n", chn);
    
    len += sprintf(str+len, "Rotation property channel %d\n", chn);
    len += sprintf(str+len, "=============================================================\n");
    len += sprintf(str+len, "ID  Name(string) Value(dec)  Readme\n");
    do {
        id = rt_property_record[idx].property[i].id;
        if (id == ID_NULL)
            break;
        value = rt_property_record[idx].property[i].value;
        map = rt_get_propertymap(id);
        if (map) {
            len += sprintf(str+len, "%02d  %12s  %09d  %s\n", id, map->name, value, map->readme);
        }
        i++;
    } while (1);
    len += sprintf(str+len, "=============================================================\n");
    return len;
}

#if defined(GM8210)
    #define RT_STRING "GM8210 Rotation"
#elif defined(GM8287)
    #define RT_STRING "GM8287 Rotation"
#elif defined(GM8139)
    #define RT_STRING "GM8139 Rotation"
#else
	#define RT_STRING "Rotation"
#endif

int rotation_msg(char *str)
{
    int len = 0;
    if (str) {
        len += sprintf(str+len, RT_STRING);
        len += sprintf(str+len, ", rotation v%d.%d.%d",ROTATION_VER_MAJOR,ROTATION_VER_MINOR, ROTATION_VER_MINOR2);
        if (ROTATION_VER_BRANCH)
            len += sprintf(str+len, ".%d", ROTATION_VER_BRANCH);
        len += sprintf(str+len, ", built @ %s %s\n", __DATE__, __TIME__);
    }
    else {
        printk(RT_STRING);
        printk(", rotation v%d.%d.%d",ROTATION_VER_MAJOR,ROTATION_VER_MINOR, ROTATION_VER_MINOR2);
        if (ROTATION_VER_BRANCH)
            printk(".%d", ROTATION_VER_BRANCH);
        printk(", built @ %s %s\n", __DATE__, __TIME__);
    }
    return len;
}


struct video_entity_ops_t rt_driver_ops ={
    putjob:      &rt_putjob,
    stop:        &rt_stop,
    queryid:     &rt_queryid,
    querystr:    &rt_querystr,
    getproperty: &rt_getproperty,
};    


struct video_entity_t rt_entity={
    //type:       TYPE_JPEG_ENC,
    type:       TYPE_ROTATION,
    name:       "rt",
    engines:    1,
    minors:     RT_MAX_CHANNEL,
    ops:        &rt_driver_ops
};


static mcp100_rdev rtdv = {
	.job_tri = frt_encode_trigger,
	.job_done = frt_process_done,
	.codec_type = FROTATION_MINOR,
	.handler = frt_isr,
	.dev_id = NULL,
	.switch22 = NULL,
	.parm_info = frt_parm_info,
	.parm_info_frame = frt_parm_info_f,
};

int rt_drv_init(void)
{
    int i;

    rt_property_record = kzalloc(sizeof(struct rt_property_record_t) * RT_MAX_CHANNEL, GFP_KERNEL);

    video_entity_register(&rt_entity);

    if (rt_proc_init() < 0)
        goto rt_init_fail;

    memset(rt_private_data, 0, sizeof(struct rt_data_t)*RT_MAX_CHANNEL);
    for (i = 0; i < RT_MAX_CHANNEL; i++) {
        rt_private_data[i].chn = i;
        //rt_private_data[i].engine = j;
    }
    if (mcp100_register(&rtdv, TYPE_MCP100_ROTATION, RT_MAX_CHANNEL) < 0)
        goto rt_init_fail;

#ifdef HOST_MODE
    rt_dump_buf_wq = create_workqueue("rt dump wq");
    if (!rt_dump_buf_wq) {
        printk("Error to create log workqueue\n");
        goto rt_init_fail;
    }
    INIT_DELAYED_WORK(&process_rt_dump_work, 0);
#endif
    rotation_msg(NULL);

    return 0;

rt_init_fail:
    rt_proc_exit();
    video_entity_deregister(&rt_entity);
#ifdef HOST_MODE
    if (rt_dump_buf_wq)
        destroy_workqueue(rt_dump_buf_wq);
#endif

    return -1;
}


void rt_drv_close(void)
{
    video_entity_deregister(&rt_entity);
    kfree(rt_property_record);

    rt_proc_exit();
#ifdef HOST_MODE
    if (rt_dump_buf_wq)
        destroy_workqueue(rt_dump_buf_wq);
#endif

    mcp100_deregister(TYPE_MCP100_ROTATION);
}

static int __init init_rt(void)
{
    return rt_drv_init();
}

static void __exit cleanup_fmjpeg(void)
{
    rt_drv_close();
}


module_init(init_rt);
module_exit(cleanup_fmjpeg);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("FTMCP100 rotation driver");
MODULE_LICENSE("GPL");

