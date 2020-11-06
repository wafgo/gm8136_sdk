/**
 * @file template.c
 *  The videograph interface of driver template.
 * Copyright (C) 2011 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.5 $
 * $Date: 2013/02/20 02:45:50 $
 *
 * ChangeLog:
 *
 *  Revision : 1.4  2013/2/27 7:29:2    ivan
 *  modify proc directory
 *
 *  Revision : 1.3  2013/2/26 14:47:22  julian
 *  add private output property for testing/debuging decoder driver
 *
 *  Revision : 1.2  2013/2/26 10:5:3    tuba
 *  remove header parasing & change common define
 *
 *  Revision : 1.1  2013/2/21 5:13:0    tuba
 *  decoder parser
 *
 *  Revision 1.3  2012/12/04 03:44:28  ivan
 *  take off max_recon_dim instead by dst_bg_dim
 *
 *  Revision 1.2  2012/10/29 05:04:38  ivan
 *  Fix type error
 *
 *  Revision 1.1.1.1  2012/10/12 08:35:53  ivan
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

#include "log.h"    //include log system "printm","damnit"...
#include "video_entity.h"   //include video entity manager
#include "decoder_vg.h"
#include "bitstream.h"
#include "header.h"

#define DECODER_MODULE_NAME     "DE"
#define ENTITY_PROC_NAME "videograph/decoder"
#define TRACE_PUT_JOB_CALLBACK 0

/* property */
//struct video_entity_t *driver_entity;

struct dec_property_map_t dec_common_property_map[]={
    /************ input property *************/
    {ID_BITSTREAM_SIZE,"bitstream_size","input bs size"},   // in_prop: bitstream size
    {ID_DST_FMT,"dst_fmt","output format"},                 // in_porp: output yuv format
    {ID_DST_XY,"dst_xy",""},                                // in_prop: output position
    {ID_DST_BG_DIM,"dst_bg_dim","output buffer size"},      // in_prop: size of output buffer
    {ID_YUV_WIDTH_THRESHOLD,"yuv_width_threshold",""},      // in_prop: condition of output scaling down frame
    {ID_SUB_YUV_RATIO,"sub_yuv_ratio","ratio value 1:N"},   // in_porp: scaling down factor

    /************ output property *************/
    {ID_SUB_YUV,"sub_yuv","if sub yuv is exist"},           // out_prop: flag of output scaling down frame
    {ID_DST_DIM,"dst_dim","output frame size"},             // out_prop: frame size
    {ID_SRC_INTERLACE,"src_interlace",""},                  // out_prop

#ifdef ENABLE_POC_SEQ_PROP
    {ID_POC, "poc", "picture display order"},               //Out_prop, 
    {ID_SEQ, "seq", "another picture display order(for debug)"},    //Out_prop, 
#endif // ENABLE_POC_SEQ_PROP

    /************ output property (for querying only)*************/
    {ID_SRC_FMT,"src_fmt","bitstream type"},                // query_prop: bitstream type
    {ID_SRC_FRAME_RATE,"src_frame_rate",""},                // query_prop: source frame rate
    {ID_FPS_RATIO,"fps_ratio",""},                          // query_prop: ratio of frame rate
    // frame rate = src_frame_rate * EM_PARAM_M(fps_ratio) / EM_PARAM_N(fps_ratio)
};

#ifdef HANDLE_PUTJOB_FAIL
struct job_cb_t {
    void                *job;
    struct list_head    cb_list;
};

static struct workqueue_struct *dec_cb_wq = NULL;
static DECLARE_DELAYED_WORK(process_callback_work, 0);
static struct semaphore dec_mutex;
static struct list_head dec_cb_head;
#endif

/* module parameter */
int dbg_mode = 0; // 0: normal mode, 1: call damnit and print bitstream at error 
module_param(dbg_mode, int, S_IRUGO|S_IWUSR);

int dec_dump_error_bs = 1;
module_param(dec_dump_error_bs, int, S_IRUGO|S_IWUSR);

/* proc system */
static struct proc_dir_entry *entity_proc,*levelproc;

/* proc */
int log_level=LOG_DEC_WARNING;
module_param(log_level, int, S_IRUGO|S_IWUSR);

/* enable jpeg decoding */
int en_jpeg=1;
module_param(en_jpeg, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(en_jpeg, "enable jpeg decoding");

/* enable mpeg4 decoding */
int en_mpeg4=1;
module_param(en_mpeg4, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(en_mpeg4, "enable mpeg4 decoding");

/* enable h264 decoding */
int en_h264=1;
module_param(en_h264, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(en_h264, "enable en_h264 decoding");

/* trace job */
int trace_job = TRACE_PUT_JOB_CALLBACK;
module_param(trace_job, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(trace_job, "print debug char at job processing");


#if 0
#define DEBUG_M(level,fmt,args...) { \
    if(log_level>=level) \
        printm(DECODER_MODULE_NAME,fmt, ## args);   \
    printk(fmt, ## args); }
#else
#define DEBUG_M(level,fmt,args...) { \
    if(log_level>=level) \
        printm(DECODER_MODULE_NAME,fmt, ## args); }
#define DEBUG_E(fmt,args...) {  \
    printm(DECODER_MODULE_NAME,  fmt, ## args); \
    printk("[dec]" fmt, ##args); }
#endif

/* local parameter */
struct decoder_entity_t *decdev[DEC_TYPE_NUM] = {0};
static char decoder_name[3][10] = {"JPEG", "MPEG4", "H264"};
//static int max_channel[3] = {MJD_MAX_CHANNEL, M4VD_MAX_CHANNEL, H264D_MAX_CHANNEL};

/*
 * only handle (1) put job: assign job to H264, MPEG4 or JPEG decoder
 *             (2) stop job: call stop job function of 3 decoders
 * maintain job list and callback by each decoder 
*/

static int getBitstreamLength(struct video_property_t *property)
{
    int i = 0;

    while(property[i].id!=0) {
        if (ID_BITSTREAM_SIZE == property[i].id) {
            return property[i].value;
        }
        i++;
    }
    return 0;
}

#ifdef HANDLE_PUTJOB_FAIL
static void callback_scheduler(void)
{
    struct video_entity_job_t *job;
    struct job_cb_t *job_cb, *job_cb_next;

    down(&dec_mutex);
    list_for_each_entry_safe (job_cb, job_cb_next, &dec_cb_head, cb_list) {
        job = (struct video_entity_job_t *)job_cb->job;
        job->status = JOB_STATUS_FAIL;
        list_del(&job_cb->cb_list);
        DEBUG_M(LOG_DEC_INFO, "job %d callback fail\n", job->id);
        job->callback(job);
        
        if(trace_job) /* for debug only */
        {
            static int cb_cnt = 0;
            cb_cnt++;
            printk("c%d", cb_cnt%10);
        }
        kfree(job_cb);
    }
    up(&dec_mutex);
}
#endif

/*
 * return 1 if dec_type is a supported
 * return 0 otherwise
 */
static int is_supported_type(DECODER_TYPE dec_type)
{
    switch(dec_type){
        case TYPE_JPEG:
            return en_jpeg;
        case TYPE_MPEG4:
            return en_mpeg4;
        case TYPE_H264:
            return en_h264;
        default:
            return 0;
    }
    return 0;
}

static int decoder_putjob(void *parm)
{
    struct video_entity_job_t *job=(struct video_entity_job_t *)parm;
    Bitstream stream;
    DECODER_TYPE dec_type;
    HeaderInfo header;
#ifdef PARSING_RESOLUTION
    int width, height;
#endif
    int len;
#ifdef HANDLE_PUTJOB_FAIL
    struct job_cb_t *job_cb;
#endif

    len = getBitstreamLength(job->in_prop);
    init_bitstream(&stream, (unsigned char *)job->in_buf.buf[0].addr_va, len);
    dec_type = parsing_header(&stream, &header);

    if(trace_job) /* for debug only */
    {
        static int pj_cnt = 0;
        pj_cnt++;
        printk("p%d", pj_cnt%10);

    }
    DEBUG_M(LOG_DEC_INFO, "put job %d to decoder %s\n", job->id, decoder_name[dec_type]);
#ifdef HANDLE_PUTJOB_FAIL
    if (is_supported_type(dec_type)) 
    {
        if (NULL == decdev[dec_type]) {
            DEBUG_M(LOG_DEC_ERROR, "%s decoder is not register\n", decoder_name[dec_type]);
            master_print("decoder: %s decoder is not register\n", decoder_name[dec_type]);
            goto pujob_fail;
        }
        if (decdev[dec_type]->put_job(job) < 0)
            goto pujob_fail;
        return JOB_STATUS_ONGOING;
    }
pujob_fail:

    // put fail job to list
    //DEBUG_M(LOG_DEC_ERROR, "can not identify bittsream type of job %d\n", job->id);
    DEBUG_E("can not identify bittsream type of job %d\n", job->id);
    if (dec_dump_error_bs) {
        unsigned char *va;
        unsigned char str[0x100] = {'\0'};
        int i;
        va = (unsigned char *)job->in_buf.buf[0].addr_va;
        for (i = 0; i < MAX_PARSING_BYTE; i+=4)
            sprintf(str, "%s%02x%02x%02x%02x", str, va[i], va[i+1], va[i+2], va[i+3]);
        DEBUG_E("%s\n", str);
    }
    // set wrong bitstream
    if(dbg_mode){
        unsigned char *va;
        int i;
        DEBUG_M(LOG_DEC_ERROR, "job_id:%04d bs size:%8d\n", job->id, len);

        /* dump bitstream */
        #define MAX_PRINT_BS_SIZE 64
        if(len >= MAX_PRINT_BS_SIZE){
            len = MAX_PRINT_BS_SIZE;
        }
        
        va = (unsigned char *)job->in_buf.buf[0].addr_va;
        printm(NULL, "input bs:");
        for(i = 0; i < len; i++){
            if((i % 16 == 0)){
                printm(NULL, "\n[%04d]", i);
            }
            printm(NULL, " %02X", va[i]);
        }
        printm(NULL, "\n");

        damnit(DECODER_MODULE_NAME);
        while(1){
            schedule();
        }
    }

    job_cb = kmalloc(sizeof(struct job_cb_t), GFP_KERNEL);
    job_cb->job = job;
    INIT_LIST_HEAD(&job_cb->cb_list);
    down(&dec_mutex);
    list_add_tail(&job_cb->cb_list, &dec_cb_head);
    up(&dec_mutex);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19))                    
    PREPARE_DELAYED_WORK(&process_callback_work, (void *)callback_scheduler);
#else
    PREPARE_WORK(&process_callback_work, (void *)callback_scheduler, (void *)0);
#endif
    queue_delayed_work(dec_cb_wq, &process_callback_work, 0);

    return JOB_STATUS_ONGOING;
#else
    if (is_supported_type(dec_type)) 
    {
        if (NULL == decdev[dec_type]) {
            DEBUG_M(LOG_DEC_ERROR, "%s decoder is not register\n", decoder_name[dec_type]);
            damnit(DECODER_MODULE_NAME);
            return JOB_STATUS_FAIL;
        }
    #ifdef PARSING_RESOLUTION
        if (header.resolution_exist) {
            width = header.width;
            height = header.height;
        }
        else
            width = height = 0;
        DEBUG_M(LOG_DEC_INFO, "put job %d to decoder %s: width %d, height %d\n", job->id, decoder_name[dec_type], width, height);
        if (decdev[dec_type]->put_job(job, width, height) < 0)
            return JOB_STATUS_FAIL;
        return JOB_STATUS_ONGOING;
    #else
        if (decdev[dec_type]->put_job(job) < 0)
            return JOB_STATUS_FAIL;
        return JOB_STATUS_ONGOING;
    #endif
    }

    return JOB_STATUS_FAIL;
#endif
}


static int decoder_stop(void *parm,int engine,int minor)
{
    int dec_type;
    for (dec_type = TYPE_JPEG; dec_type <= TYPE_H264; dec_type++) {
        if (decdev[dec_type])
            decdev[dec_type]->stop_job(parm, engine, minor);
    }
    return 0;
}

struct dec_property_map_t *decoder_get_propertymap(int id)
{
    int i;
    
    for(i=0;i<sizeof(dec_common_property_map)/sizeof(struct dec_property_map_t);i++) {
        if(dec_common_property_map[i].id==id) {
            return &dec_common_property_map[i];
        }
    }
    return 0;
}

int decoder_queryid(void *parm,char *str)
{
    int i;
    
    for(i=0;i<sizeof(dec_common_property_map)/sizeof(struct dec_property_map_t);i++) {
        if(strcmp(dec_common_property_map[i].name,str)==0) {
            return dec_common_property_map[i].id;
        }
    }
    printk("decoder_queryid: Error to find name %s\n",str);
    return -1;
}


static int decoder_querystr(void *parm,int id,char *string)
{
    int i;
    
    for(i=0;i<sizeof(dec_common_property_map)/sizeof(struct dec_property_map_t);i++) {
        if(dec_common_property_map[i].id==id) {
            memcpy(string,dec_common_property_map[i].name,sizeof(dec_common_property_map[i].name));
            return 0;
        }
    }
    printk("decoder_querystr: Error to find id %d\n",id);
    return -1;
}

static int decoder_getproperty(void *parm, int engine, int minor, char *string)
{
// need to mapping decoder & engine
    if (engine < TYPE_JPEG || engine > TYPE_H264)
        return 0;

    if (NULL == decdev[engine]) {
        DEBUG_M(LOG_DEC_ERROR, "%s decoder is not register\n", decoder_name[engine]);
        return 0;
    }
    return decdev[engine]->get_property(parm, engine, minor, string);
}

int decoder_register(struct decoder_entity_t *entity, DECODER_TYPE dec_type)
{
    switch (dec_type) {
        case TYPE_JPEG:
        case TYPE_MPEG4:
        case TYPE_H264:
            break;
        default:
            DEBUG_M(LOG_DEC_ERROR, "unknown decoder type %d\n", dec_type);
            damnit(DECODER_MODULE_NAME);
            return -1;
    }
    decdev[dec_type] = entity;
    DEBUG_M(LOG_DEC_INFO, "register %s decoder\n", decoder_name[dec_type]);
    return 0;
}

int decoder_deregister(DECODER_TYPE dec_type)
{
    switch (dec_type) {
        case TYPE_JPEG:
        case TYPE_MPEG4:
        case TYPE_H264:
            break;
        default:
            printk("unknown decoder type %d\n", dec_type);
            return -1;
    }
    decdev[dec_type] = NULL;
    DEBUG_M(LOG_DEC_INFO, "deregister %s decoder\n", decoder_name[dec_type]);
    return 0;
}

static int proc_level_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int level;

    sscanf(buffer,"%d",&level);
    log_level=level;
    printk("\nLog level =%d (0:error 1:warning 2:debug)\n",log_level);
    return count;
}


static int proc_level_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    printk("\nLog level =%d (1:emerge 1:error 2:debug)\n",log_level);
    *eof = 1;
    return 0;
}

struct video_entity_ops_t driver_ops ={
    putjob:      &decoder_putjob,
    stop:        &decoder_stop,
    queryid:     &decoder_queryid,
    querystr:    &decoder_querystr,
    getproperty: &decoder_getproperty,
};    


struct video_entity_t decode_entity={
    type:       TYPE_DECODER,
    name:       "decode",
    engines:    MAX_DECODE_ENGINE_NUM,
    minors:     MAX_DECODE_CHN_NUM,
    ops:        &driver_ops
};

static int __init decoder_vg_init(void)
{
    video_entity_register(&decode_entity);

#ifdef HANDLE_PUTJOB_FAIL
    dec_cb_wq = create_workqueue("favce_cb");
    if (!dec_cb_wq) {
        DEBUG_M(LOG_DEC_ERROR, "%s:Error to create workqueue dec_cb_wq\n", __FUNCTION__);
        damnit(DECODER_MODULE_NAME);
        return -EFAULT;
    }
    INIT_DELAYED_WORK(&process_callback_work, 0);

    #if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
        sema_init(&dec_mutex, 1);
    #else
        init_MUTEX(&dec_mutex);
    #endif

    INIT_LIST_HEAD(&dec_cb_head);
#endif

    entity_proc = create_proc_entry(ENTITY_PROC_NAME, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if(entity_proc==NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        return -EFAULT;
    }

    levelproc = create_proc_entry("level", S_IRUGO | S_IXUGO, entity_proc);
    if(levelproc==NULL)
        return -EFAULT;
    levelproc->read_proc = (read_proc_t *)proc_level_read_mode;
    levelproc->write_proc = (write_proc_t *)proc_level_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
    levelproc->owner = THIS_MODULE;    
#endif

    printk("Decoder v%d.%d.%d", DECODER_VER_MAJOR, DECODER_VER_MINOR, DECODER_VER_MINOR2);
    if (DECODER_VER_BRANCH)
        printk(".%d", DECODER_VER_BRANCH);
    printk(", built @ %s %s\n", __DATE__, __TIME__);

    return 0;
}


static void __exit decoder_vg_close(void)
{
    video_entity_deregister(&decode_entity);
#ifdef HANDLE_PUTJOB_FAIL
    destroy_workqueue(dec_cb_wq);
#endif
    if(levelproc!=0)
        remove_proc_entry(levelproc->name, entity_proc);
    if(entity_proc!=0)
        remove_proc_entry(ENTITY_PROC_NAME, 0);   
}


#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
//EXPORT_SYMBOL(dec_common_property_map);
EXPORT_SYMBOL(decoder_register);
EXPORT_SYMBOL(decoder_deregister);
EXPORT_SYMBOL(decoder_get_propertymap);
EXPORT_SYMBOL(decoder_queryid);
//EXPORT_SYMBOL(decoder_querystr);
#endif

module_init(decoder_vg_init);
module_exit(decoder_vg_close);

MODULE_AUTHOR("Grain Media Corp.");
MODULE_LICENSE("GPL");



