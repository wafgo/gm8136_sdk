/**
 * @file template.c
 *  The videograph interface of driver template.
 * Copyright (C) 2011 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.3 $
 * $Date: 2012/10/30 03:22:53 $
 *
 * ChangeLog:
 *  $Log: fmjpeg_dec_vg2.c,v $
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

#include "log.h"    //include log system "printm","damnit"...
#include "video_entity.h"   //include video entity manager
#include "fmjpeg_dec_entity.h"
#include "fmjpeg_dec_vg2.h"

#ifdef FRAMMAP
#include "frammap.h"
#endif
#include "fmjpeg.h"
#include "decoder/jpeg_dec.h"
#include "ioctl_jd.h"
#include "../fmcp.h"
#undef  PFX
#define PFX	        "FMJPEG_D"
#include "debug.h"

#ifdef MULTI_FORMAT_DECODER
#include "decoder_vg.h"
#endif

#ifdef USE_TRIGGER_WORKQUEUE
static spinlock_t jpeg_dec_lock;
#endif

#ifdef MULTI_FORMAT_DECODER
    extern struct dec_property_map_t dec_common_property_map[DECODER_MAX_MAP_ID];
#else   // MULTI_FORMAT_DECODER

    enum mjd_property_id {
        ID_YUV_WIDTH_THRESHOLD=(MAX_WELL_KNOWN_ID_NUM+1), //start from 100
        ID_SUB_YUV_RATIO,
       
        ID_COUNT,
    };

    struct dec_property_map_t mjd_property_map[]={  
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

extern int mjd_proc_init(void);
extern void mjd_proc_exit(void);
extern unsigned int video_gettime_us(void);
#ifdef MULTI_FORMAT_DECODER
extern struct dec_property_map_t *decoder_get_propertymap(int id);
extern int decoder_queryid(void *parm,char *str);
#endif

/* variable */
//struct mjd_data_t   mjd_private_data[MAX_ENGINE_NUM][MJD_MAX_CHANNEL];
struct mjd_data_t   *mjd_private_data = NULL;
#define DATA_SIZE   sizeof(struct mjd_data_t)

/* utilization */
unsigned int mjd_utilization_period = 5; //5sec calculation
unsigned int mjd_utilization_start = 0, mjd_utilization_record = 0;
unsigned int mjd_engine_start = 0, mjd_engine_end = 0;
unsigned int mjd_engine_time = 0;

/* property lastest record */
struct mjd_property_record_t *mjd_property_record;

/* log & debug message */
unsigned int mjd_log_level = LOG_WARNING;

/* proc */
//int entity_decode_dbg_mode=0;

#if defined(SUPPORT_VG_422T)||defined(SUPPORT_VG_422P)
unsigned int jpeg_raw420_virt = 0, jpeg_raw420_phy = 0;
extern unsigned int jpg_max_width;
extern unsigned int jpg_max_height;
#endif
extern unsigned int jpg_dec_max_chn;

#if 1
#define DEBUG_M(level, fmt, args...) { \
    if (mjd_log_level >= level) \
        printm(MJD_MODULE_NAME, fmt, ## args); }
#define DEBUG_E(fmt, args...) { \
    printm(MJD_MODULE_NAME,  fmt, ## args); \
    printk("[mjd]" fmt, ##args); }
#else
#define DEBUG_M(level, fmt, args...) { \
    if (mjd_log_level == LOG_DIRECT) \
        printk(fmt, ## args); \
    else if (mjd_log_level >= level) \
        printm(MJD_MODULE_NAME, fmt, ## args); }
#define DEBUG_E(fmt, args...) { \
    printm(MJD_MODULE_NAME,  fmt, ## args); \
    printk("[mjd]" fmt, ##args); }
#endif


void mjd_print_property(struct video_entity_job_t *job, struct video_property_t *property, int out_prop)
{
    int i;
    int engine = ENTITY_FD_ENGINE(job->fd);
    int minor = ENTITY_FD_MINOR(job->fd);
    
    for (i = 0; i < MAX_PROPERTYS; i++) {
        if(property[i].id == ID_NULL)
            break;
        if(i==0) {
            if (out_prop) {
                DEBUG_M(LOG_INFO,"{%d,%d} job %d out property:\n", engine, minor, job->id);
            }
            else {
                DEBUG_M(LOG_INFO,"{%d,%d} job %d in property:\n", engine, minor, job->id);
            }
        }
        DEBUG_M(LOG_INFO, "  ID:%d,Value:0x%x\n", property[i].id, property[i].value);
    }
}

int mjd_parse_in_property(void *param, struct video_entity_job_t *job)
{
    int i = 0;
    struct mjd_data_t *decode_data=(struct mjd_data_t *)param; 
    int idx = MAKE_IDX(jpg_dec_max_chn, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
    struct video_property_t *property=job->in_prop;
    //int idx = ENTITY_FD_MINOR(job->fd);
    void *handler;
    int dst_fmt, dst_bg_dim, dst_dim, output_422, trans_422pack;

    handler = decode_data->handler;
    dst_fmt = decode_data->dst_fmt;
    dst_bg_dim = decode_data->dst_bg_dim;
    dst_dim = decode_data->dst_dim;
    output_422 = decode_data->output_422;
    trans_422pack = decode_data->trans_422pack;    
    memset(decode_data,0,sizeof(struct mjd_data_t));
    decode_data->handler = handler;
    decode_data->output_422 = output_422;
    decode_data->trans_422pack = trans_422pack;
    while(property[i].id!=0) {
        switch(property[i].id) {
            case  ID_BITSTREAM_SIZE:
                decode_data->bitstream_size = property[i].value;
                break;
            case  ID_DST_FMT:
                if (property[i].value != dst_fmt) {
                    decode_data->updated = 1;
                }
                decode_data->dst_fmt = property[i].value;
                if (TYPE_YUV420_FRAME == decode_data->dst_fmt)
                    decode_data->output_format = OUTPUT_420_YUV;
                else if (TYPE_YUV422_FRAME == decode_data->dst_fmt)
                    decode_data->output_format = OUTPUT_CbYCrY;
                else
                    decode_data->output_format = OUTPUT_CbYCrY; // OUTPUT_YUV
                break;
            case  ID_DST_BG_DIM:
                if (property[i].value != dst_bg_dim)
                    decode_data->updated = 1;
                decode_data->dst_bg_dim = property[i].value;
                decode_data->buf_width = EM_PARAM_WIDTH(decode_data->dst_bg_dim);
                decode_data->buf_height = EM_PARAM_HEIGHT(decode_data->dst_bg_dim);
                decode_data->U_off = decode_data->buf_width * decode_data->buf_height;
                decode_data->V_off = decode_data->buf_width * decode_data->buf_height * 5/4;
                break;
            /*
            case  ID_DST_DIM:
                if (property[i].value != dst_dim)
                    decode_data->updated = 1;
                decode_data->dst_dim = property[i].value;
                decode_data->frame_width = EM_PARAM_WIDTH(decode_data->dst_dim);
                decode_data->frame_height = EM_PARAM_HEIGHT(decode_data->dst_dim);
                break;
            */
            case  ID_YUV_WIDTH_THRESHOLD:
                decode_data->yuv_width_threshold = property[i].value;
                break;
            case  ID_SUB_YUV_RATIO:
                decode_data->sub_yuv_ratio = property[i].value;
                break;
            default:
                break;
        }
        if(i < MJD_MAX_RECORD) {
            mjd_property_record[idx].property[i].id = property[i].id;
            mjd_property_record[idx].property[i].value = property[i].value;
        }
        i++;
    }
    mjd_property_record[idx].property[i].id = mjd_property_record[idx].property[i].value=0;
    mjd_property_record[idx].entity = job->entity;
    mjd_property_record[idx].job_id = job->id;

    mjd_print_property(job,job->in_prop, 0);
    return 1;
}


int mjd_set_out_property(void *param, struct video_entity_job_t *job)
{
    int i=0;
    struct mjd_data_t *decode_data=(struct mjd_data_t *)param;
    struct video_property_t *property=job->out_prop;
//    int idx=MAKE_IDX(entity->minors, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
/*
    if(unlikely(MAX_CHANNELS <= idx)){
        panic("[ERROR][EM]idx is too bigger than MAX_CHANNELS.\n");
    }
    if(unlikely(0 == entity->ch_used[idx])){
        panic("[ERROR][EM] The channel(fd=%d,idx=%d) has not been used.!\n",job->fd,idx);
    }
*/
    property[i].id = ID_SRC_INTERLACE;
    property[i].value = decode_data->src_interlace;    //data->xxxx
    i++;

    property[i].id = ID_SUB_YUV;
    property[i].value = 0;
    i++;

    property[i].id = ID_DST_XY;
    property[i].value = decode_data->dst_xy;
    i++;

    property[i].id = ID_DST_DIM;
    property[i].value = EM_PARAM_DIM(decode_data->output_width, decode_data->output_height);
    i++;

    property[i].id = ID_DST_BG_DIM;
    property[i].value = EM_PARAM_DIM(decode_data->output_width, decode_data->output_height);
    i++;

    property[i].id = ID_NULL;
    i++;
    

    mjd_print_property(job,job->out_prop, 1);
    return 1;
}


static void mjd_mark_engine_start(void)
{
    if (mjd_engine_start != 0)
        DEBUG_M(LOG_WARNING, "Warning to nested use mjd_mark_engine_start function!\n");
    mjd_engine_start = video_gettime_us();
    mjd_engine_end = 0;
    if (mjd_utilization_start == 0) {
        mjd_utilization_start = mjd_engine_start;
        mjd_engine_time = 0;
    }
}


static void mjd_mark_engine_finish(void *job_param, void *data)
{
    struct job_item_t *job_item = (struct job_item_t *)job_param;
    mjd_set_out_property(data,job_item->job);
        
    if (mjd_engine_end != 0)
        DEBUG_M(LOG_WARNING, "Warning to nested use mjd_mark_engine_finish function!\n");
    mjd_engine_end = video_gettime_us();
    if (mjd_engine_end > mjd_engine_start)
        mjd_engine_time += mjd_engine_end - mjd_engine_start;
    if (mjd_utilization_start > mjd_engine_end) {
        mjd_utilization_start = 0;
        mjd_engine_time = 0;
    }
    else if((mjd_utilization_start <= mjd_engine_end)&&
        (mjd_engine_end - mjd_utilization_start >= mjd_utilization_period*1000000)) {
        unsigned int utilization;
        utilization = (unsigned int)((100*mjd_engine_time)/(mjd_engine_end-mjd_utilization_start));
        if (utilization)
            mjd_utilization_record = utilization;
        mjd_utilization_start = 0;
        mjd_engine_time = 0;
    }        
    mjd_engine_start = 0;
}


int mjd_preprocess(void *parm, void *priv)
{
    struct job_item_t *job_item = (struct job_item_t *)parm;
    return video_preprocess(job_item->job,priv);
}


int mjd_postprocess(void *parm, void *priv)
{
    struct job_item_t *job_item = (struct job_item_t *)parm;
    int ret = -1;
    ret = video_postprocess(job_item->job,priv);
    return ret;
}

static int fmjd_parm_info(int engine, int chn)
{
	struct mjd_data_t *pv;

    if (engine >= MAX_ENGINE_NUM) {
        DEBUG_M(LOG_ERROR, "engine (%d) over MAX\n", engine);
        damnit(MJD_MODULE_NAME);
        return 0;
    }

	if (chn >= jpg_dec_max_chn) {
		DEBUG_M(LOG_ERROR, "chn (%d) over MAX", chn);
        damnit(MJD_MODULE_NAME);
		return 0;
	}
	//pv = &mjd_private_data[engine][chn];
	pv = mjd_private_data + chn;

	if (pv->handler) {
		info ("== chn (%d) info begin ==", chn);
		FJpegDD_show (pv->handler);
		info ("== chn (%d) info end ==", chn);
	}
	return 0;
}

static void fmjd_parm_info_f(struct job_item_t *job_item)
{
	struct mjd_data_t *pv = mjd_private_data + job_item->chn;

	info ("====== fmjd_parm_info_f ======");
	info ("job_id = %d, stop = %d", job_item->job_id, pv->stop_job);
	info ("dev = %d", job_item->chn);
	info ("pv->handler = 0x%08x", (unsigned int)pv->handler);
	info ("bitstream size = %d", pv->bitstream_size);
}

static int fmjd_create(struct mjd_data_t *pv, FJPEG_DEC_PARAM *pParam, FJPEG_DEC_PARAM_A2 *pParamA2)
{
	FJPEG_DEC_PARAM_MY tDecParam;
	memcpy ((void *)&tDecParam.pub, (void *)pParam, sizeof(FJPEG_DEC_PARAM));	
	tDecParam.pu32BaseAddr = (unsigned int*)mcp100_dev->va_base;  
	tDecParam.pfnMalloc = fkmalloc;
	tDecParam.pfnFree = fkfree;
	tDecParam.u32CacheAlign = CACHE_LINE;

#if defined(SUPPORT_VG_422T) | defined(SUPPORT_VG_422P)
    // Tuba 20110817_1 start: add zero check
    if (tDecParam.pub.frame_width * tDecParam.pub.frame_height == 0)
        DEBUG_E("output resolution (%dx%d) is zero\n", tDecParam.pub.frame_width, tDecParam.pub.frame_height);
    // Tuba 20110817_1: add zero check
    if (tDecParam.pub.frame_width * tDecParam.pub.frame_height > jpg_max_height * jpg_max_width) {
        DEBUG_E("output resolution (%dx%d) is larger than max resolution (%dx%d)", tDecParam.pub.frame_width, tDecParam.pub.frame_height, jpg_max_height, jpg_max_width);
        return -1;
    }
#endif

	// to create the jpeg decoder object
	if (pv->handler == NULL) {
        pv->handler = FJpegDecCreate(&tDecParam);
        if (pv->handler == 0)
            return -1;
    }
    else
        FJpegDecReCreate(&tDecParam, pv->handler);
	FJpegDecParaA2(pv->handler, pParamA2);

	return 0;
}


static void mjd_param_vg_init(struct mjd_data_t *pv, FJPEG_DEC_PARAM *pParam, FJPEG_DEC_PARAM_A2 *pParamA2)
{
    pParam->u32API_version = 0;
//	pParam->frame_width	= EM_PARAM_WIDTH(pv->dst_bg_dim);
//	pParam->frame_height = EM_PARAM_HEIGHT(pv->dst_bg_dim);
    pParam->frame_width = pv->frame_width;
    pParam->frame_height = pv->frame_height;
	pParam->output_format = pv->output_format;
	pv->U_off = pv->U_off;
	pv->V_off = pv->V_off;
	pv->stop_job = 0;
#ifdef SUPPORT_VG_422T
	if (pv->output_format == 2) {
		pv->output_422 = 1;
		//pv->output_format = 0;
        pParam->output_format = 0;
	}
	else
		pv->output_422 = 0;
#endif
    //pv->frame_width = pParam->frame_width;
    //pv->frame_height = pParam->frame_height;

	memset (pParamA2, 0, sizeof (FJPEG_DEC_PARAM_A2));
	pParamA2->buf_width = pv->buf_width;
	pParamA2->buf_height = pv->buf_height;
}

static int fmjd_decode_trigger(struct job_item_t *job_item)
{
    int engine, chn;
    struct mjd_data_t *pv;
    struct video_entity_job_t *job;
    FJPEG_DEC_FRAME tFrame;
    int ret;

    job=(struct video_entity_job_t *)job_item->job;
    engine = job_item->engine;
    chn = job_item->chn;
    //pv = &mjd_private_data[engine][chn];
    pv = mjd_private_data + chn;

    if (job->in_buf.buf[0].addr_va == 0 || job->out_buf.buf[0].addr_va == 0) {
        //printk("job in/out pointer err");
        DEBUG_E("job in/out buffer is NULL\n");
        damnit(MJD_MODULE_NAME);
        return -1;
    }
    pv->in_addr_pa = job->in_buf.buf[0].addr_pa;
    pv->in_size = job->in_buf.buf[0].size;
    
    pv->out_addr_pa = job->out_buf.buf[0].addr_pa;
    pv->out_size = job->out_buf.buf[0].size;

    mjd_parse_in_property((void *)pv, job);
#ifdef MULTI_FORMAT_DECODER
/*
    if (job_item->frame_width && job_item->frame_height &&
        (pv->buf_width * pv->buf_height < job_item->frame_width * job_item->frame_height)) {
        err("decoder frame size(%d x %d) is over buffer size(%d x %d)\n", 
            job_item->frame_width, job_item->frame_height, pv->buf_width, pv->buf_height);
        return -1;
    }
    if (0 == pv->frame_width || 0 == pv->frame_height) {
        pv->frame_width = job_item->frame_width;
        pv->frame_height = job_item->frame_height;
    }
*/
#endif

    //if (pv->update_parm) {
    if (pv->updated) {
        FJPEG_DEC_PARAM decp;
		FJPEG_DEC_PARAM_A2 decp2;

		// open
		mjd_param_vg_init(pv, &decp, &decp2);
		if (fmjd_create(pv, &decp, &decp2) < 0)
			return -1;
        pv->updated = 0;
    }
    //mjd_preprocess(job_item,0);

	if ( !pv->handler ) {
		DEBUG_E("fatal error: put job but never init");
		return -1;
	}

        
    pv->engine = engine;
    pv->chn = chn;

    pv->in_addr_pa = job->in_buf.buf[0].addr_pa;
    pv->in_addr_va = job->in_buf.buf[0].addr_va;
    pv->in_size = job->in_buf.buf[0].size;

    pv->out_addr_pa = job->out_buf.buf[0].addr_pa;
    pv->out_addr_va = job->out_buf.buf[0].addr_va;
    pv->out_size = job->out_buf.buf[0].size;

    pv->data = (void *)job_item;
        
    DEBUG_M(LOG_INFO, "(%d,%d) job %d va 0x%x trigger\n",
        job_item->engine, job_item->chn, job->id, job->out_buf.buf[0].addr_va);

    if (pv->stop_job)
        return -1;
    mjd_mark_engine_start();

    tFrame.buf = (unsigned char *)pv->in_addr_pa;
    //tFrame.buf_size = pv->in_size;
    tFrame.buf_size = pv->bitstream_size;

#ifdef SUPPORT_VG_422T
	if (pv->output_422) {
        tFrame.pu8YUVAddr[0] = (unsigned char *)(jpeg_raw420_phy + 0);
        tFrame.pu8YUVAddr[1] = (unsigned char *)(jpeg_raw420_phy + jpg_max_width*jpg_max_height);
		tFrame.pu8YUVAddr[2] = (unsigned char *)(jpeg_raw420_phy + jpg_max_width*jpg_max_height*5/4);
	}
	else
#endif
	{
		tFrame.pu8YUVAddr[0] = (unsigned char *)(pv->out_addr_pa + 0);
		tFrame.pu8YUVAddr[1] = (unsigned char *)(pv->out_addr_pa + pv->U_off);
		tFrame.pu8YUVAddr[2] = (unsigned char *)(pv->out_addr_pa + pv->V_off);
	}
/*
	if (dump_n_frame)
		fmjd_parm_info_f (job);
*/
	FJpegDec_assign_bs(pv->handler,
			(unsigned char *)pv->in_addr_va,
			(unsigned char *)pv->in_addr_pa,
			tFrame.buf_size);
	if (FJpegDecReadHeader(pv->handler, &tFrame, 0) >= 0) {
        // Tuba 20110817_0 start: add frame size checking
        /*  // Tuba 20130214: no check because not ahndle transfromation
        if (tFrame.img_width != pv->frame_width || tFrame.img_height != pv->frame_height) {
            printk("header size(%dx%d) is different from initial size(%dx%d)\n", tFrame.img_width, tFrame.img_height, pv->frame_width, pv->frame_height);
            return -1;
        }
        */
        // Tuba 20110817_0 end: add frame size checking
#ifdef SUPPORT_VG_422T
		if (pv->output_422) {
			if ( (tFrame.img_width*tFrame.img_height) > (jpg_max_width*jpg_max_height) ) {
				DEBUG_E("@seq mode, output image (%d x %d) bigger than support (%d x %d)", tFrame.img_width, tFrame.img_height, jpg_max_width, jpg_max_height);
				return JDE_SETERR;
			}
        #ifdef SUPPORT_VG_422P
            if (JDS_422_1 == tFrame.sample) {
                pv->trans_422pack = 1;
                tFrame.pu8YUVAddr[0] = (unsigned char *)(jpeg_raw420_phy + 0);
                tFrame.pu8YUVAddr[1] = (unsigned char *)(jpeg_raw420_phy + jpg_max_width*jpg_max_height);
                tFrame.pu8YUVAddr[2] = (unsigned char *)(jpeg_raw420_phy + jpg_max_width*jpg_max_height*3/2);
            }
            else {
                pv->trans_422pack = 0;
    			if (tFrame.sample != JDS_420) {
    				DEBUG_E("@seq mode, output output 422 packet but sampling is not JDS_420");
    				return JDE_USPERR;
    			}
            }
        #else
            if (tFrame.sample != JDS_420) {
				DEBUG_E("@seq mode, output output 422 packet but sampling is not JDS_420");
				return JDE_USPERR;
			}
        #endif
		}
#endif
		tFrame.err = FJpegDD_Tri(pv->handler, &tFrame);
	}
	ret = tFrame.err;
/*
	if (ret < 0) {
		mcp100_proc_dump (1,
			chn & 0xFF,
			TYPE_JPG_DECODER,
			(unsigned char *)job->in_buf->addr_va,
			pv->in_size, job->swap, 1);
	}
*/
	return ret;
}

static int fmjd_process_done (struct job_item_t *job_item)
{
	int ret;
	//struct mjd_data_t *pv = &mjd_private_data[job_item->engine][job_item->chn];
	struct mjd_data_t *pv = mjd_private_data + job_item->chn;
//	struct v_graph_info * outb_hdr = (struct  v_graph_info *)job->out_header->addr_va;
//	struct v_graph_info * inb_hdr = (struct  v_graph_info *)job->in_header->addr_va;
//	mjd_InInfo * in_info = (mjd_InInfo *)(job->in_header->addr_va + inb_hdr->drvinfo_offset);
//	mjd_OutInfo *dec_out_info = (mjd_OutInfo *)(job->out_header->addr_va + outb_hdr->drvinfo_offset);

    //info("dev %d sync\n", entity->minor&0xFF);
	ret = FJpegDD_End (pv->handler);
	//printk("d: u = 0x%08x, v = 0x%08x\n",(int)u, (int) v);
	//printk("d: *u = 0x%08x, *v = 0x%08x\n", *u, * v);
	if (ret < 0 ) {     // Tuba 20110803_0: add sync error control
	    mjd_mark_engine_finish(job_item, pv);
        return ret;     // Tuba 20110803_0: add sync error control
    }
	if (ret >  0) {		// need to trigger another scan
		ret = FJpegDD_Tri (pv->handler, NULL);
		if (ret < 0)		// fail
			ret = -1;
		else				// not done, already trigger another scan
			return 1;
	}
	else {
		FJPEG_DEC_FRAME tFrame;
    #ifdef SUPPORT_VG_422T
		if (pv->output_422 && 0 == pv->trans_422pack) {
			ret = FJpegD_420to422 (pv->handler, jpeg_raw420_phy + 0,
								   jpeg_raw420_phy + jpg_max_width*jpg_max_height,
								   jpeg_raw420_phy + jpg_max_width*jpg_max_height*5/4,
								   pv->out_addr_pa);
            if (ret < 0)
                return ret;
		}
    #endif

		FJpegDD_GetInfo(pv->handler, &tFrame);

    #ifdef SUPPORT_VG_422P
        if (pv->trans_422pack) {
        //if (JDS_422_0 == decf.sample || JDS_422_1 == decf.sample) {
            ret = FJpegD_422Packed (pv->handler, jpeg_raw420_phy + 0,
                                    jpeg_raw420_phy + jpg_max_width*jpg_max_height,
                                    jpeg_raw420_phy + jpg_max_width*jpg_max_height*3/2,
                                    pv->out_addr_pa);
            if (ret < 0)
                return ret;
        }

    #endif

        //info("fmjd_process_done: out buffer = 0x%x\n", (int)dec_out_info);

        pv->NumofComponents = tFrame.NumofComponents;
        pv->output_width = tFrame.img_width;
        pv->output_height = tFrame.img_height;
        pv->sample = tFrame.sample;
        mjd_mark_engine_finish(job_item, pv);
	}
/*
	mcp100_proc_dump (ret,
			entity->minor & 0xFF,
			TYPE_JPG_DECODER,
			(unsigned char *)job->in_buf->addr_va,
			in_info->bs_length, job->swap, 1);
*/
    return ret;
}

static void fmjd_release (struct mjd_data_t *pv)
{
	FJpegDecDestroy(pv->handler);
	pv->handler = NULL;
}

static int mjd_putjob(void *parm)
{
    //mcp_putjob((struct video_entity_job_t*)parm, TYPE_JPEG_DECODER, 0, 0);
    mcp_putjob((struct video_entity_job_t*)parm, TYPE_JPEG_DECODER);
    return JOB_STATUS_ONGOING;
}

int jd_int;
static void mjd_isr(int irq, void *dev_id, unsigned int base)
{
	if (jd_int & IT_JPGD) {
		unsigned int int_sts;
		int_sts = mSEQ_INTFLG(base);
		// clr interrupt
		mSEQ_INTFLG(base) = int_sts;
		jd_int = int_sts;
	}
}

static int mjd_stop(void *parm, int engine, int minor)
{
    if (minor >= jpg_dec_max_chn || engine >= MAX_ENGINE_NUM)
        printk("chn %d is over max channel %d or engine %d is over max engine %d\n", 
            minor, jpg_dec_max_chn, engine, MAX_ENGINE_NUM);
    else
        mcp_stopjob(TYPE_JPEG_DECODER, engine, minor);

    return 0;
}

static struct dec_property_map_t *mjd_get_propertymap(int id)
{
#ifndef MULTI_FORMAT_DECODER
    int i;
    for(i = 0; i < sizeof(mjd_property_map)/sizeof(struct dec_property_map_t); i++) {
        if(mjd_property_map[i].id==id) {
            return &mjd_property_map[i];
        }
    }
    return 0;
#else
    return decoder_get_propertymap(id);
#endif
}


#ifndef MULTI_FORMAT_DECODER
static int mjd_queryid(void *parm,char *str)
{
    int i;
    for(i=0;i<sizeof(mjd_property_map)/sizeof(struct dec_property_map_t);i++) {
        if(strcmp(mjd_property_map[i].name,str)==0) {
            return mjd_property_map[i].id;
        }
    }
    printk("mjd_queryid: Error to find name %s\n",str);
    return -1;
}

static int mjd_querystr(void *parm,int id,char *string)
{
    int i;
    for(i=0;i<sizeof(mjd_property_map)/sizeof(struct dec_property_map_t);i++) {
        if (mjd_property_map[i].id==id) {
            memcpy(string, mjd_property_map[i].name, sizeof(mjd_property_map[i].name));
            return 0;
        }
    }
    printk("mjd_querystr: Error to find id %d\n",id);
    return -1;
}
#endif

static int mjd_getproperty(void *parm, int engine, int minor, char *string)
{
    int id,value = -1;
    struct mjd_data_t *decode_data;

    if (engine > MAX_ENGINE_NUM) {
        printk("Error over engine number %d\n", MAX_ENGINE_NUM);
        return -1;
    }

    if (minor > jpg_dec_max_chn) {
        printk("Error over minor number %d\n", jpg_dec_max_chn);
        return -1;
    }
    //decode_data = &(mjd_private_data[engine][minor]);
    decode_data = mjd_private_data + minor;
#ifdef MULTI_FORMAT_DECODER
    id = decoder_queryid(parm, string);
#else
    id = mjd_queryid(parm, string);
#endif
    switch (id) {              
        case  ID_BITSTREAM_SIZE:
            value = decode_data->bitstream_size;
            break;
        case  ID_DST_FMT:
            value = decode_data->dst_fmt;
            break;
        case  ID_DST_BG_DIM:
            value = decode_data->dst_bg_dim;
            break;
        case  ID_YUV_WIDTH_THRESHOLD:
            value = decode_data->yuv_width_threshold;
            break;
        case  ID_SUB_YUV_RATIO:
            value = decode_data->sub_yuv_ratio;
            break;
        case  ID_SRC_FMT:
            value = TYPE_BITSTREAM_JPEG;
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
            value = EM_PARAM_DIM(decode_data->output_width, decode_data->output_height);;
            break;
        case  ID_SRC_INTERLACE:
            value = decode_data->src_interlace;
            break;
        case  ID_SUB_YUV:
            value = 0;
            break;
        
        default:
            break;
    }
    return value;
}

int mjd_dump_property_value(char *str, int chn)
{
    int i=0, len = 0;
    struct dec_property_map_t *map;
    unsigned int id,value;
    int engine = 0;
    int idx = MAKE_IDX(jpg_dec_max_chn, engine, chn);;
#ifdef USE_TRIGGER_WORKQUEUE
    unsigned long flags;
    spin_lock_irqsave(&jpeg_dec_lock, flags);
#endif
    len += sprintf(str+len, "Jpeg encoder property channel %d\n", chn);
    len += sprintf(str+len, "=============================================================\n");
    len += sprintf(str+len, "ID    Name(string)  Value(hex)  Readme\n");
    do {
        id = mjd_property_record[idx].property[i].id;
        if (id == ID_NULL)
            break;
        value = mjd_property_record[idx].property[i].value;
        map = mjd_get_propertymap(id);
        if(map) {
            len += sprintf(str+len, "%02d  %14s   %08x  %s\n",id,map->name,value,map->readme);
        }
        i++;
    } while(1);
    len += sprintf(str+len, "=============================================================\n");
#ifdef USE_TRIGGER_WORKQUEUE
    spin_unlock_irqrestore(&jpeg_dec_lock, flags);
#endif
    return len;
}

#ifndef MULTI_FORMAT_DECODER

struct video_entity_ops_t mjd_ops ={
    putjob:      &mjd_putjob,
    stop:        &mjd_stop,
    queryid:     &mjd_queryid,
    querystr:    &mjd_querystr,
    getproperty: &mjd_getproperty,
};    


struct video_entity_t jpeg_dec_entity={
    type:       TYPE_DECODER,
    name:       "jpeg_decode",
    engines:    MAX_ENGINE_NUM,
    minors:     MJD_MAX_CHANNEL,
    ops:        &mjd_ops
};

#else   // MULTI_FORMAT_DECODER

struct decoder_entity_t mjd_entity = {
    decoder_type:   TYPE_JPEG,
    put_job:        mjd_putjob,
    stop_job:       mjd_stop,
    get_property:   mjd_getproperty
};

#endif  // MULTI_FORMAT_DECODER


static mcp100_rdev mjddv = {
    .job_tri = fmjd_decode_trigger,
	.job_done = fmjd_process_done,
	.codec_type = FMJPEG_DECODER_MINOR,
	.handler = mjd_isr,
	.dev_id = NULL,
	.switch22 = NULL,
	.parm_info = fmjd_parm_info,
	.parm_info_frame = fmjd_parm_info_f,
};



int mjd_drv_init(void)
{
    int chn;

    mjd_property_record = kzalloc(sizeof(struct mjd_property_record_t)*MAX_ENGINE_NUM*jpg_dec_max_chn,GFP_KERNEL);

#ifndef MULTI_FORMAT_DECODER
    jpeg_dec_entity.minors = jpg_dec_max_chn;
    video_entity_register(&jpeg_dec_entity);  // register to mcp100
#else
    decoder_register(&mjd_entity, TYPE_JPEG);
#endif

    mjd_private_data = kzalloc(sizeof(struct mjd_data_t) * jpg_dec_max_chn, GFP_KERNEL);
    if (NULL == mjd_private_data) {
        DEBUG_E("Fail to allocate jpeg dec private data\n");
        goto exit_mjd_init;
    }
    memset(mjd_private_data, 0, sizeof(struct mjd_data_t) * jpg_dec_max_chn);

    for (chn = 0; chn < jpg_dec_max_chn; chn ++) {
        struct mjd_data_t *decode_data = mjd_private_data + chn;
        decode_data->engine = 0;
        decode_data->chn = chn;
    }
    
    if (mcp100_register(&mjddv, TYPE_JPEG_DECODER, jpg_dec_max_chn) < 0)
        goto exit_mjd_init;

    #ifdef SUPPORT_VG_422T
    {
        int  sz422 = jpg_max_width * jpg_max_height * 2;
    #ifdef FRAMMAP
        {
            struct frammap_buf_info buf_info;
            memset(&buf_info, 0, sizeof(struct frammap_buf_info));
            buf_info.size = sz422;
            //buf_info.alloc_type = 0;
            if (frm_get_buf_ddr(DDR_ID1, &buf_info) < 0) {
                jpeg_raw420_phy = 0;
            }
            jpeg_raw420_phy = buf_info.phy_addr;
            jpeg_raw420_virt = buf_info.va_addr;
            //printk("allocate raw buffer %08x (%08x)\n", jpeg_raw420_virt, jpeg_raw420_phy);
        }
    #else
        jpeg_raw420_virt = (unsigned int)fcnsist_alloc(PAGE_ALIGN(sz422), (void **)&jpeg_raw420_phy, 0);//ncnb
		#endif
        if(0 == jpeg_raw420_virt) {
            printk("Fail to allocate 420/422 transform buffer, size 0x%x!\n", (int)PAGE_ALIGN(sz422));
            //drv_mjd_exit();
            goto exit_mjd_init;
        }
    }
	#endif
#ifdef USE_TRIGGER_WORKQUEUE
    spin_lock_init(&jpeg_dec_lock);
#endif
    mjd_proc_init();

    return 0;
exit_mjd_init:
#ifndef MULTI_FORMAT_DECODER
    video_entity_deregister(&jpeg_dec_entity);
#else
    decoder_deregister(TYPE_JPEG);
#endif
    if (mjd_property_record)
        kfree(mjd_property_record);
    if (mjd_private_data)
        kfree(mjd_private_data);
    return -EFAULT;    
}


void mjd_drv_close(void)
{
    int chn;
#ifndef MULTI_FORMAT_DECODER
    video_entity_deregister(&jpeg_dec_entity);
#else
    decoder_deregister(TYPE_JPEG);
#endif
    kfree(mjd_property_record);

#ifdef SUPPORT_VG_422T
	if(jpeg_raw420_virt) {
    #ifdef FRAMMAP
        struct frammap_buf_info buf_info;
		memset(&buf_info, 0, sizeof(struct frammap_buf_info));
		buf_info.va_addr = jpeg_raw420_virt;
		buf_info.phy_addr = jpeg_raw420_phy;
		frm_free_buf_ddr (&buf_info);
    #else
    	int sz422 = jpg_max_width * jpg_max_height * 2;
		fcnsist_free(PAGE_ALIGN(sz422), (void *)jpeg_raw420_virt, (void *)jpeg_raw420_phy);
    #endif
    }
#endif

    for (chn = 0; chn < jpg_dec_max_chn; chn ++) {
        struct mjd_data_t *decode_data = mjd_private_data + chn;
		if (decode_data->handler != NULL) {
			//printk ("chn %d not released, automatically close", chn);
			fmjd_release(decode_data);
		}
	}

    mjd_proc_exit();
    if (mjd_private_data)
        kfree(mjd_private_data);
}

