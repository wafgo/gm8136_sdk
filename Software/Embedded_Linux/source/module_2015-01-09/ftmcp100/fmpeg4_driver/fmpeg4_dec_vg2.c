/**
 * @file fmpeg4_dec_vg2.c
 *  The videograph interface of driver template.
 * Copyright (C) 2011 GM Corp. (http://www.grain-media.com)
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

#include "log.h"    //include log system "printm","damnit"...
#include "video_entity.h"   //include video entity manager
#include "frammap_if.h"

#include "fmpeg4_dec_entity.h"
#include "fmpeg4_dec_vg2.h"
#include "fmpeg4.h"
#include "Mp4Vdec.h"
#include "./common/share_mem.h"
#include "ioctl_mp4d.h"
#include "../fmcp.h"

#ifdef MULTI_FORMAT_DECODER
#include "decoder_vg.h"
#endif
#include "mp4e_param.h"

//#ifdef USE_TRIGGER_WORKQUEUE
static spinlock_t mpeg4_dec_lock;
//#endif

#ifdef MULTI_FORMAT_DECODER
    extern struct dec_property_map_t dec_common_property_map[DECODER_MAX_MAP_ID];
#else   // MULTI_FORMAT_DECODER
    enum mp4d_property_id {
        ID_YUV_WIDTH_THRESHOLD=(MAX_WELL_KNOWN_ID_NUM+1), //start from 100
        ID_SUB_YUV_RATIO,
    };

    struct dec_property_map_t mp4d_property_map[] = {
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
    
        /************ output property (for querying only)*************/
        {ID_SRC_FMT,"src_fmt","bitstream type"},                // query_prop: bitstream type
        {ID_SRC_FRAME_RATE,"src_frame_rate",""},                // query_prop: source frame rate
        {ID_FPS_RATIO,"fps_ratio",""},                          // query_prop: ratio of frame rate
    };
#endif  // MULTI_FORMAT_DECODER

extern int mp4d_proc_init(void);
extern void mp4d_proc_exit(void);
extern unsigned int video_gettime_us(void);
extern void switch2mp4d(void * codec, ACTIVE_TYPE curr);
extern int dd_tri(void * ptDecHandle, FMP4_DEC_RESULT * ptResult);
extern int dd_sync(void * ptDecHandle, FMP4_DEC_RESULT * ptResult);
extern void dd_show(void *dec_handle);
#ifdef MULTI_FORMAT_DECODER
extern struct dec_property_map_t *decoder_get_propertymap(int id);
extern int decoder_queryid(void *parm,char *str);
#endif

extern unsigned int mp4_max_width;
extern unsigned int mp4_max_height;
#ifdef MPEG4_COMMON_PRED_BUFFER
    extern struct buffer_info_t mp4d_acdc_buf;
    extern struct buffer_info_t mp4d_mb_buf;
    #ifdef ENABLE_DEBLOCKING
        extern struct buffer_info_t mp4d_deblock_buf;
    #endif
#endif

/* variable */
struct mp4d_data_t      mp4d_private_data[MAX_ENGINE_NUM][MP4D_MAX_CHANNEL];
#define DATA_SIZE       sizeof(struct mp4d_data_t)

/* utilization */
unsigned int mp4d_utilization_period = 5; //5sec calculation
unsigned int mp4d_utilization_start = 0, mp4d_utilization_record = 0;
unsigned int mp4d_engine_start = 0, mp4d_engine_end = 0;
unsigned int mp4d_engine_time = 0;

/* property lastest record */
struct mp4d_property_record_t *mp4d_property_record = NULL;

unsigned int mp4d_log_level = LOG_WARNING;    //larger level, more message

#if 1
#define DEBUG_M(level, fmt, args...) { \
    if (mp4d_log_level >= level) \
        printm(MP4D_MODULE_NAME, fmt, ## args); }
#define DEBUG_E(fmt, args...) { \
        printm(MP4D_MODULE_NAME,  fmt, ## args); \
        printk("[mp4d]" fmt, ##args); }
#else
#define DEBUG_M(level, fmt, args...) { \
    if (mp4d_log_level == LOG_DIRECT) \
        printk(fmt, ## args); \
    else if (mp4d_log_level >= level) \
        printm(MP4D_MODULE_NAME, "[mp4d]" fmt, ## args); }
#define DEBUG_E(fmt, args...) { \
    printm(MP4D_MODULE_NAME,  fmt, ## args); \
    printk("[mp4d]" fmt, ##args); }
#endif


static void mp4d_print_property(struct video_entity_job_t *job, struct video_property_t *property, int input)
{
    int i;
    int engine = ENTITY_FD_ENGINE(job->fd);
    int minor = ENTITY_FD_MINOR(job->fd);
    
    for (i = 0; i < MAX_PROPERTYS; i++) {
        if (property[i].id == ID_NULL)
            break;
        if (i == 0) {
            if (input) {
                DEBUG_M(LOG_INFO, "{%d,%d} job %d in property:\n", engine,minor, job->id);
            }
            else {
                DEBUG_M(LOG_INFO, "{%d,%d} job %d out property:\n", engine,minor, job->id);
            }
        }
        DEBUG_M(LOG_INFO, "  ID:%d,Value:0x%x\n", property[i].id, property[i].value);
    }
}

static int mp4d_parse_in_property(struct mp4d_data_t *decode_data, struct video_entity_job_t *job)
{
    int i = 0;
    //struct mp4d_data_t *decode_data = (struct mp4d_data_t *)param;
    int idx = MAKE_IDX(MP4D_MAX_CHANNEL, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
    struct video_property_t *property = job->in_prop;
    //void *handler;
    //int dst_fmt, dst_bg_dim, dst_dim;

    //handler = decode_data->handler;
    //dst_fmt = decode_data->dst_fmt;
    //dst_bg_dim = decode_data->dst_bg_dim;
    //dst_dim = decode_data->dst_dim;
    //memset(decode_data, 0 , DATA_SIZE);
    // Tuba 20131126: resert frame width height because is must be resolution of bitstream header
    decode_data->frame_width = 0;
    decode_data->frame_height = 0;
    while (property[i].id != 0) {
        switch (property[i].id) {
            case ID_BITSTREAM_SIZE:
                decode_data->bitstream_size = property[i].value;
                break;
            case ID_DST_FMT:
                if (property[i].value != decode_data->dst_fmt)
                    decode_data->updated = 1;
                decode_data->dst_fmt = property[i].value;
                /*  if 0, OUTPUT_FMT_CbYCrY
                 *  if 4, OUTPUT_FMT_YUV    */
                if (TYPE_YUV420_FRAME == decode_data->dst_fmt)
                    decode_data->output_format = 4;
                else if (TYPE_YUV422_FRAME == decode_data->dst_fmt)
                    decode_data->output_format = 0;
                // Tuba 20141225: handle dst fmt TYPE_YUV422_RATIO_FRAME
                else if (TYPE_YUV422_RATIO_FRAME == decode_data->dst_fmt)
                    decode_data->output_format = 0;
                else {
                    DEBUG_M(LOG_ERROR, "unknown dst_fmt 0x%x\n", decode_data->dst_fmt);
                    damnit(MP4D_MODULE_NAME);
                    return -1;
                }
                break;
            case ID_DST_XY:
                decode_data->dst_xy = property[i].value;
                break;
            case ID_DST_BG_DIM:
                if (property[i].value != decode_data->dst_bg_dim)
                    decode_data->updated = 1;
                decode_data->dst_bg_dim = property[i].value;
                // remove max width/max height
                decode_data->buf_width = EM_PARAM_WIDTH(decode_data->dst_bg_dim);
                decode_data->buf_height = EM_PARAM_HEIGHT(decode_data->dst_bg_dim);
                decode_data->U_off = decode_data->buf_width * decode_data->buf_height;
                decode_data->V_off = decode_data->buf_width * decode_data->buf_height * 5/4;
                break;
            case ID_YUV_WIDTH_THRESHOLD:
                decode_data->yuv_width_threshold = property[i].value;
                break;
            case ID_SUB_YUV_RATIO:
                decode_data->sub_yuv_ratio = property[i].value;
                break;
            default:
                break;
        }
        if (i < MP4D_MAX_RECORD) {
            mp4d_property_record[idx].property[i].id = property[i].id;
            mp4d_property_record[idx].property[i].value = property[i].value;
        }
        i++;
    }
    mp4d_property_record[idx].property[i].id = mp4d_property_record[idx].property[i].value = 0;
    mp4d_property_record[idx].entity = job->entity;
    mp4d_property_record[idx].job_id = job->id;
    
    mp4d_print_property(job,job->in_prop, 1);
    //decode_data->handler = handler;
    return 1;
}


int mp4d_set_out_property(void *param, struct video_entity_job_t *job)
{
    int i = 0;
    struct mp4d_data_t *decode_data = (struct mp4d_data_t *)param;
    struct video_property_t *property = job->out_prop;

    property[i].id = ID_SRC_INTERLACE;
    property[i].value = 0;
    i++;

    property[i].id = ID_SUB_YUV;
    property[i].value = 0;
    i++;

    property[i].id = ID_DST_XY;
    property[i].value = 0;
    i++;

    property[i].id = ID_DST_BG_DIM;
    property[i].value = EM_PARAM_DIM(decode_data->frame_width, decode_data->frame_height);
    i++;

    property[i].id = ID_DST_DIM;
    property[i].value = EM_PARAM_DIM(decode_data->frame_width, decode_data->frame_height);
    i++;

    property[i].id = ID_NULL;
    i++;
    
    mp4d_print_property(job, job->out_prop, 0);
    return 1;
}


static void mp4d_mark_engine_start(void)
{
    unsigned long flags;
    spin_lock_irqsave(&mpeg4_dec_lock, flags);
    if (mp4d_engine_start != 0)
        DEBUG_M(LOG_WARNING, "Warning to nested use mp4d_mark_engine_start function!\n");
    mp4d_engine_start = video_gettime_us();
    mp4d_engine_end = 0;
    if (mp4d_utilization_start == 0) {
        mp4d_utilization_start = mp4d_engine_start;
        mp4d_engine_time = 0;
    }
    spin_unlock_irqrestore(&mpeg4_dec_lock, flags);
}

static void mp4d_mark_engine_finish(void)
{
    if (mp4d_engine_end != 0)
        DEBUG_M(LOG_WARNING, "Warning to nested use mp4d_mark_engine_end function!\n");
    mp4d_engine_end = video_gettime_us();
    if (mp4d_engine_end > mp4d_engine_start)
        mp4d_engine_time += mp4d_engine_end - mp4d_engine_start;
    if (mp4d_utilization_start > mp4d_engine_end) {
        mp4d_utilization_start = 0;
        mp4d_engine_time = 0;
    }
    else if ((mp4d_utilization_start <= mp4d_engine_end) &&
            (mp4d_engine_end - mp4d_utilization_start >= mp4d_utilization_period*1000000)) {
        unsigned int utilization;
        utilization = (unsigned int)((100 * mp4d_engine_time) /
            (mp4d_engine_end - mp4d_utilization_start));
        if (utilization)
            mp4d_utilization_record = utilization;
        mp4d_utilization_start = 0;
        mp4d_engine_time = 0;
    }        
    mp4d_engine_start = 0;
}

int mp4d_preprocess(void *parm, void *priv)
{
    struct job_item_t *job_item = (struct job_item_t *)parm;
    return video_preprocess(job_item->job, priv);
}


int mp4d_postprocess(void *parm, void *priv)
{
    struct job_item_t *job_item = (struct job_item_t *)parm;
    return video_postprocess(job_item->job, priv);
}

#ifdef REF_POOL_MANAGEMENT
static int mp4d_release_ref_buffer(struct ref_buffer_info_t *buf, int chn)
{
    if (buf->ref_virt) {
        release_pool_buffer2(buf, chn, TYPE_DECODER);
        DEBUG_M(LOG_INFO, "{chn%d} release buffer 0x%x\n", chn, buf->ref_phy);
    }
    memset(buf, 0, sizeof(struct ref_buffer_info_t));
    return 0;
}
static int mp4d_release_all_ref_buffer(struct mp4d_data_t *decode_data)
{
    mp4d_release_ref_buffer(decode_data->refer_buf, decode_data->chn);
    mp4d_release_ref_buffer(decode_data->recon_buf, decode_data->chn);
    DEBUG_M(LOG_DEBUG, "{chn%d} release all reference buffer\n", decode_data->chn);
    return 0;
}
static int mp4d_register_mem_pool(struct mp4d_data_t *decode_data)
{
    mp4d_release_all_ref_buffer(decode_data);
    deregister_ref_pool(decode_data->chn, TYPE_DECODER);
    decode_data->res_pool_type = register_ref_pool(decode_data->chn, EM_PARAM_WIDTH(decode_data->dst_bg_dim),
        EM_PARAM_HEIGHT(decode_data->dst_bg_dim), TYPE_DECODER);
    if (decode_data->res_pool_type < 0)
        return -1;
    return 0;
}
#endif

static void fmp4d_param_vg_init(struct mp4d_data_t *decode_data, FMP4_DEC_PARAM *decp)
{
#if 0
    decp->u32MaxWidth = EM_PARAM_WIDTH(decode_data->dst_bg_dim);
    decp->u32MaxHeight = EM_PARAM_HEIGHT(decode_data->dst_bg_dim);
#else
    decp->u32MaxWidth = mp4_max_width;
    decp->u32MaxHeight = mp4_max_height;
#endif
    decp->u32FrameWidth	= decode_data->frame_width;
	decp->u32FrameHeight = decode_data->frame_height;
	decp->output_image_format= decode_data->output_format;
}

static int fmp4d_create(struct mp4d_data_t *decode_data, FMP4_DEC_PARAM *decm)
{
    FMP4_DEC_PARAM_MY dec_param;

	memcpy (&dec_param.pub, decm, sizeof(FMP4_DEC_PARAM));
	dec_param.u32VirtualBaseAddr = (unsigned int)mcp100_dev->va_base;  
	dec_param.u32CacheAlign = 16;
	dec_param.pfnMalloc = fkmalloc; //we don't need it
	dec_param.pfnFree = fkfree; //we don't need it
	dec_param.pfnDmaMalloc = fconsistent_alloc;
	dec_param.pfnDmaFree = fconsistent_free;

#ifdef MPEG4_COMMON_PRED_BUFFER
    dec_param.pu16ACDC_phy = (uint16_t *)mp4d_acdc_buf.addr_pa;
    dec_param.pu32Mbs_phy = (uint32_t *)mp4d_mb_buf.addr_pa;
#endif
//#ifdef REF_POOL_MANAGEMENT
#if defined(REF_POOL_MANAGEMENT)&!defined(REF_BUFFER_FLOW)  // Tuba 20140725
    dec_param.pu8ReConFrameCur_phy = (uint8_t *)decode_data->recon_buf->ref_phy;
    dec_param.pu8ReConFrameRef_phy = (uint8_t *)decode_data->refer_buf->ref_phy;
#endif

    if (decode_data->handler)
        return Mp4VDec_ReInit(&dec_param, decode_data->handler);
	else {
        return Mp4VDec_Init(&dec_param, &decode_data->handler);
    }
}

static int fmp4d_decode_Tri(struct job_item_t *job_item)
{
    int engine, chn;
#ifdef REF_POOL_MANAGEMENT
    unsigned long flags;
#endif
    struct mp4d_data_t *decode_data;
	struct video_entity_job_t *job;
	FMP4_DEC_RESULT tResult;
    int ret = 0;

    job = (struct video_entity_job_t *)job_item->job;
    engine = job_item->engine;
    chn = job_item->chn;
    decode_data = &mp4d_private_data [engine][chn];

#ifdef MULTI_FORMAT_DECODER
    //decode_data->frame_width = job_item->frame_width;
    //decode_data->frame_height = job_item->frame_height;
#endif
    mp4d_parse_in_property(decode_data, job);
    if (decode_data->updated) {
        FMP4_DEC_PARAM decp;
    #ifdef REF_POOL_MANAGEMENT
        spin_lock_irqsave(&mpeg4_dec_lock, flags);
        ret = mp4d_register_mem_pool(decode_data);
        spin_unlock_irqrestore(&mpeg4_dec_lock, flags);
        if (ret < 0)
            return ret;
        #ifndef REF_BUFFER_FLOW
        if (allocate_pool_buffer2(decode_data->recon_buf, decode_data->res_pool_type, decode_data->chn, TYPE_DECODER) < 0) {
            DEBUG_M(LOG_ERROR, "{chn%d} allocate refereence buffer fail\n", decode_data->chn);
            return -1;
        }
        if (allocate_pool_buffer2(decode_data->refer_buf, decode_data->res_pool_type, decode_data->chn, TYPE_DECODER) < 0) {
            DEBUG_M(LOG_ERROR, "{chn%d} allocate refereence buffer fail\n", decode_data->chn);
            return -1;
        }
        #endif
    #endif
        fmp4d_param_vg_init(decode_data, &decp);
		if (fmp4d_create(decode_data, &decp) < 0)
			return -1;
		//mp4d_crop_vg2init (mp4d_p, pv);
		decode_data->updated = 0;
    }
    
    if (NULL == decode_data->handler) {
        DEBUG_E("mpeg4 decoder is not init!!\n");
        damnit(MP4D_MODULE_NAME);
        return -1;
    }

	if ((0 == job->in_buf.buf[0].addr_va) || (0 == job->out_buf.buf[0].addr_va)) {
	    DEBUG_E("job in/out pointer err");
        damnit(MP4D_MODULE_NAME);
        return -1;
    }
    
    decode_data->in_addr_va = job->in_buf.buf[0].addr_va;
    decode_data->in_addr_pa = job->in_buf.buf[0].addr_pa;
    decode_data->in_size = job->in_buf.buf[0].size;
    
    decode_data->out_addr_va = job->out_buf.buf[0].addr_va;
    decode_data->out_addr_pa = job->out_buf.buf[0].addr_pa;
    decode_data->out_size = job->out_buf.buf[0].size;

#ifdef REF_BUFFER_FLOW
    if (decode_data->res_pool_type < 0) {
        DEBUG_E("buffer register initialization fail\n");
        return -1;
    }
    if (allocate_pool_buffer2(decode_data->recon_buf, decode_data->res_pool_type, decode_data->chn, TYPE_DECODER) < 0) {
        DEBUG_M(LOG_ERROR, "{chn%d} allocate refereence buffer fail\n", decode_data->chn);
        return -1;
    }
    tResult.pReconBuffer = decode_data->recon_buf;
#endif
/*
	if (dump_n_frame)
		mp4d_parm_info_f (job);
*/
	Mp4VDec_AssignBS (decode_data->handler,
	    (unsigned char *)decode_data->in_addr_va,
	    (unsigned char *)decode_data->in_addr_pa, 
	    decode_data->bitstream_size);

	Mp4VDec_SetOutputAddr(decode_data->handler,
		(unsigned char *)decode_data->out_addr_pa,
		(unsigned char *)(decode_data->out_addr_pa + decode_data->U_off),
		(unsigned char *)(decode_data->out_addr_pa + decode_data->V_off));

    mp4d_mark_engine_start();

	if (dd_tri(decode_data->handler, &tResult) < 0) {
		ret = -1;
		/*
		mcp100_proc_dump (1,
			entity->minor & 0xFF,
			TYPE_MP4_DECODER,
			(unsigned char *)job->in_buf->addr_va,
			in_info->bs_length, job->swap, 1);
        */
	}
	//info("fmp4d_decode_Tri: %d", ret);

	return ret;
}

static int fmp4d_process_done(struct job_item_t *job_item)
{
	int ret;
	struct mp4d_data_t *decode_data = &mp4d_private_data[job_item->engine][job_item->chn];
	//struct v_graph_info * outb_hdr = (struct  v_graph_info *)job->out_header->addr_va;
	//mp4d_OutInfo *dec_out_info = (mp4d_OutInfo *)(job->out_header->addr_va + outb_hdr->drvinfo_offset);
	FMP4_DEC_RESULT tResult;
	//struct v_graph_info * inb_hdr = (struct  v_graph_info *)job->in_header->addr_va;
	//mp4d_InInfo * in_info = (mp4d_InInfo *)(job->in_header->addr_va + inb_hdr->drvinfo_offset);
	ret = dd_sync(decode_data->handler, &tResult);
	if (ret == 0) {
	    decode_data->frame_width = tResult.u32VopWidth;
	    decode_data->frame_height = tResult.u32VopHeight;
	}
	mp4d_mark_engine_finish();
	mp4d_set_out_property(decode_data, job_item->job);
#ifdef REF_BUFFER_FLOW
{
    struct ref_buffer_info_t *tmp_buf;
    mp4d_release_ref_buffer(decode_data->refer_buf, decode_data->chn);
    tmp_buf = decode_data->refer_buf;
    decode_data->refer_buf = decode_data->recon_buf;
    decode_data->recon_buf = tmp_buf;
}
#endif
#ifdef REF_POOL_MANAGEMENT
    if (decode_data->stop_job) {
        mp4d_release_all_ref_buffer(decode_data);
        deregister_ref_pool(decode_data->chn, TYPE_DECODER);
        decode_data->res_pool_type = -1;
        decode_data->stop_job = 0;
    }
    else {
        // swap reference buffer
    }
#endif
	/*
	mcp100_proc_dump (ret,
		entity->minor & 0xFF,
		TYPE_MP4_DECODER,
		(unsigned char *)job->in_buf->addr_va,
		in_info->bs_length, job->swap, 1);
	//info("fmp4d_process_done: %d", ret);
	*/
	return ret;
}

static void mp4d_isr(int irq, void *dev_id, unsigned int base)
{
	MP4_COMMAND type;
	volatile Share_Node *node = (volatile Share_Node *) (0x8000 + base);		 

	type = (node->command & 0xF0)>>4;
	if (( type == DECODE_IFRAME) || (type == DECODE_PFRAME) || (type == DECODE_NFRAME) ) {
		#ifdef EVALUATION_PERFORMANCE
			encframe.hw_stop = get_counter();
		#endif
		#ifndef GM8120
			*(volatile unsigned int *)(base + 0x20098)=0x80000000;
		#endif
	}
}

static void fmp4d_release(struct mp4d_data_t *decode_data)
{
    if (decode_data->handler) {
	    Mp4VDec_Release(decode_data->handler);
	    decode_data->handler= NULL;
	}
}

static int mp4d_parm_info(int engine, int chn)
{
	struct mp4d_data_t *decode_data;

    if (engine >= MAX_ENGINE_NUM) {
        DEBUG_M(LOG_ERROR, "engine (%d) over MAX\n", engine);
        return -1;
    }
    if (chn >= MP4D_MAX_CHANNEL) {
		DEBUG_M(LOG_ERROR, "chn (%d) over MAX", chn);
		return -1;
	}
	decode_data = &mp4d_private_data[engine][chn];

	if (decode_data->handler) {
		printk("== chn (%d) info begin ==", chn);
		dd_show (decode_data->handler);
		printk("== chn (%d) info end ==", chn);
	}

	return 0;
}

static void mp4d_parm_info_f(struct job_item_t *job_item)
{
	struct mp4d_data_t *decode_data = &mp4d_private_data[job_item->engine][job_item->chn];

	printk("====== [chn%d] mp4d_parm_info_f ======", job_item->chn);
	printk("job_id = %d, stop = %d", job_item->job_id, decode_data->stop_job);
	printk("handler = 0x%08x", (unsigned int)decode_data->handler);
	printk("bitstream size = %d", decode_data->bitstream_size);
}

static int mp4d_putjob(void *parm)
{
    //mcp_putjob((struct video_entity_job_t*)parm, TYPE_MPEG4_DECODER, 0, 0);
    mcp_putjob((struct video_entity_job_t*)parm, TYPE_MPEG4_DECODER);
    return JOB_STATUS_ONGOING;
}

static int mp4d_stop(void *parm, int engine, int minor)
{
#ifdef REF_POOL_MANAGEMENT
    struct mp4d_data_t *decode_data;
#endif
    int is_ongoing_job = 0;
    if (minor >= MP4D_MAX_CHANNEL || engine >= MAX_ENGINE_NUM)
        printk("mpeg4 chn %d is over max channel %d or engine %d is over max engine %d\n", 
            minor, MP4D_MAX_CHANNEL, engine, MAX_ENGINE_NUM);
    else {
        is_ongoing_job = mcp_stopjob(TYPE_MPEG4_DECODER, engine, minor);
    #ifdef REF_POOL_MANAGEMENT
        decode_data = &mp4d_private_data[engine][minor];
        if (!is_ongoing_job) {
            mp4d_release_all_ref_buffer(decode_data);
            deregister_ref_pool(decode_data->chn, TYPE_DECODER);
            decode_data->res_pool_type = -1;
            DEBUG_M(LOG_DEBUG, "{chn%d} stop but job on going\n", minor);
        }
        else
            decode_data->stop_job = 1;
    #endif
        decode_data->updated = 1;
    }
    return 0;
}

static struct dec_property_map_t *mp4d_get_propertymap(int id)
{
#ifndef MULTI_FORMAT_DECODER
    int i;
    for (i = 0; i < sizeof(mp4d_property_map)/sizeof(struct dec_property_map_t); i++) {
        if (mp4d_property_map[i].id==id) {
            return &mp4d_property_map[i];
        }
    }
    return 0;
#else
    return decoder_get_propertymap(id);
#endif
}

#ifndef MULTI_FORMAT_DECODER
static int mp4d_queryid(void *parm, char *str)
{
    int i;
    for (i = 0; i < sizeof(mp4d_property_map)/sizeof(struct dec_property_map_t); i++) {
        if (strcmp(mp4d_property_map[i].name,str)==0) {
            return mp4d_property_map[i].id;
        }
    }
    printk("mp4d_queryid: Error to find name %s\n", str);
    return -1;
}

static int mp4d_querystr(void *parm, int id,char *string)
{
    int i;
    for (i = 0; i < sizeof(mp4d_property_map)/sizeof(struct dec_property_map_t); i++) {
        if (mp4d_property_map[i].id==id) {
            memcpy(string, mp4d_property_map[i].name, sizeof(mp4d_property_map[i].name));
            return 0;
        }
    }
    printk("mp4d_querystr: Error to find id %d\n", id);
    return -1;
}
#endif

static int mp4d_getproperty(void *parm, int engine, int minor, char *string)
{
    int id,value = -1;
    struct mp4d_data_t *decode_data;
    if (engine > MAX_ENGINE_NUM) {
        printk("Error over engine number %d\n", MAX_ENGINE_NUM);
        return -1;
    }
    if (minor > MP4D_MAX_CHANNEL) {
        printk("Error over minor number %d\n", MP4D_MAX_CHANNEL);
        return -1;
    }

    decode_data = &mp4d_private_data[engine][minor];
#ifdef MULTI_FORMAT_DECODER
    id = decoder_queryid(parm, string);
#else
    id = mp4d_queryid(parm, string);
#endif
    switch (id) {
        case ID_BITSTREAM_SIZE:
            value = decode_data->bitstream_size;
            break;
        case ID_DST_FMT:
            value = decode_data->dst_fmt;
            break;
        case ID_DST_BG_DIM:
            value = decode_data->dst_bg_dim;
            break;
        case ID_YUV_WIDTH_THRESHOLD:
            value = decode_data->yuv_width_threshold;
            break;
        case ID_SUB_YUV_RATIO:
            value = decode_data->sub_yuv_ratio;
            break;
        case ID_SRC_FMT:
            value = TYPE_BITSTREAM_MPEG4;
            break;
        case ID_SRC_FRAME_RATE:
            value = 30;
            break;
        case ID_FPS_RATIO:
            value = 1;      // always be 30 fps
            break;
        case ID_DST_XY:
            value = decode_data->dst_xy;
            break;
        case ID_DST_DIM:
            value = EM_PARAM_DIM(decode_data->frame_width, decode_data->frame_height);;
            break;
        case ID_SRC_INTERLACE:
            //value = decode_data->src_interlace;
            value = 0;
            break;
        case ID_SUB_YUV:
            value = 0;
            break;
        default:
            break;
    }
    return value;
}

int mp4d_dump_property_value(char *str, int chn)
{
    int i = 0, len = 0;
    struct dec_property_map_t *map;
    unsigned int id, value;
    int engine = 0;
    int idx = MAKE_IDX(MP4D_MAX_CHANNEL, engine, chn);
//#ifdef USE_TRIGGER_WORKQUEUE
    unsigned long flags;

    spin_lock_irqsave(&mpeg4_dec_lock, flags);
//#endif
    len += sprintf(str+len, "MPEG4 decoder property channel %d\n", chn);
    len += sprintf(str+len, "=============================================================\n");
    len += sprintf(str+len, "ID  Name(string) Value(hex)  Readme\n");
    do {
        id = mp4d_property_record[idx].property[i].id;
        if (id == ID_NULL)
            break;
        value = mp4d_property_record[idx].property[i].value;
        map = mp4d_get_propertymap(id);
        if (map) {
            len += sprintf(str+len, "%02d  %12s  %08x  %s\n", id, map->name, value, map->readme);
        }
        i++;
    } while (1);
    len += sprintf(str+len, "=============================================================\n");
//#ifdef USE_TRIGGER_WORKQUEUE
    spin_unlock_irqrestore(&mpeg4_dec_lock, flags);
//#endif
    return len;
}

#ifndef MULTI_FORMAT_DECODER

struct video_entity_ops_t mp4d_ops = {
    putjob:         &mp4d_putjob,
    stop:           &mp4d_stop,
    queryid:        &mp4d_queryid,
    querystr:       &mp4d_querystr,
    getproperty:    &mp4d_getproperty,
};

struct video_entity_t mpeg4_dec_entity = {
    type:       TYPE_DECODER,
    name:       "mpeg4_decode",
    engines:    MAX_ENGINE_NUM,
    minors:     MP4D_MAX_CHANNEL,
    ops:        &mp4d_ops
};

#else   // MULTI_FORMAT_DECODER
/*
int tets_dun(void *parm)
{
return 0;
}
*/
struct decoder_entity_t mp4d_entity = {
    decoder_type:   TYPE_MPEG4,
    put_job:        mp4d_putjob,
    stop_job:       mp4d_stop,
    get_property:   mp4d_getproperty,
};

#endif  // MULTI_FORMAT_DECODER

static mcp100_rdev mp4d_dev = {
	.job_tri        = fmp4d_decode_Tri,
	.job_done       = fmp4d_process_done,
	.codec_type     = TYPE_MPEG4_DECODER,
	.handler        = mp4d_isr,
	.dev_id         = NULL,
	.switch22       = switch2mp4d,
	.parm_info      = mp4d_parm_info,
	.parm_info_frame = mp4d_parm_info_f,
};

#define REGISTER_VG     0x01
#define REGISTER_MCP100 0x02
#define INIT_PROC       0x04
int mp4d_drv_init(void)
{
    int i, chn;
    int init = 0;
#ifndef MULTI_FORMAT_DECODER
    video_entity_register(&mpeg4_dec_entity);  // register to mcp100
#else
    decoder_register(&mp4d_entity, TYPE_MPEG4);
#endif
    init |= REGISTER_VG;
    
    mp4d_property_record = kzalloc(sizeof(struct mp4d_property_record_t)*MAX_ENGINE_NUM*MP4D_MAX_CHANNEL,GFP_KERNEL);
    
    memset(mp4d_private_data, 0, DATA_SIZE * MAX_ENGINE_NUM * MP4D_MAX_CHANNEL);
    
    for (i = 0; i< MAX_ENGINE_NUM; i++) {
        for (chn = 0; chn < MP4D_MAX_CHANNEL; chn++) {
            mp4d_private_data[i][chn].engine = i;
            mp4d_private_data[i][chn].chn = chn;
        #ifdef REF_POOL_MANAGEMENT
            mp4d_private_data[i][chn].res_pool_type = -1;
            mp4d_private_data[i][chn].recon_buf = &mp4d_private_data[i][chn].ref_buffer[0];
            mp4d_private_data[i][chn].refer_buf = &mp4d_private_data[i][chn].ref_buffer[1];            
        #endif
        }
    }
    
    if (mcp100_register(&mp4d_dev, TYPE_MPEG4_DECODER, MP4D_MAX_CHANNEL) < 0)
        goto mp4d_vg_init_fail;
    init |= REGISTER_MCP100;
//#ifdef USE_TRIGGER_WORKQUEUE
    spin_lock_init(&mpeg4_dec_lock);
//#endif
    if (mp4d_proc_init() < 0)
        goto mp4d_vg_init_fail;

    return 0;

mp4d_vg_init_fail:
    if (REGISTER_VG | init){
    #ifndef MULTI_FORMAT_DECODER
        video_entity_deregister(&mpeg4_dec_entity);
    #else
        decoder_deregister(TYPE_MPEG4);
    #endif
    }
    if (REGISTER_MCP100 | init)
        mcp100_deregister(TYPE_MPEG4_DECODER);
    if (INIT_PROC | init)
        mp4d_proc_exit();
    if (mp4d_property_record)
        kfree(mp4d_property_record);
    return 0;
} 

void mp4d_drv_close(void)
{
    int i, chn;
    struct mp4d_data_t *decode_data;
#ifndef MULTI_FORMAT_DECODER
    video_entity_deregister(&mpeg4_dec_entity);
#else
    decoder_deregister(TYPE_MPEG4);
#endif
    if (mp4d_property_record)
        kfree(mp4d_property_record);
    mp4d_proc_exit();
    mcp100_deregister(TYPE_MPEG4_DECODER);

    for (i = 0; i<MAX_ENGINE_NUM; i++) {
        for (chn = 0; chn < MP4D_MAX_CHANNEL; chn++) {
            decode_data = &mp4d_private_data[i][chn];
    		if (decode_data->handler != NULL) {
    		// check handler NULL @ fmp4d_release function
    		    mp4d_release_all_ref_buffer(decode_data);
                deregister_ref_pool(decode_data->chn, TYPE_DECODER);
    			fmp4d_release(decode_data);
    		}
    	}
    }
}
