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
#include "fmjpeg_enc_entity.h"
#include "fmjpeg_enc_vg2.h"
#include "fmjpeg.h"
#include "encoder/jpeg_enc.h"
#include "ioctl_je.h"
#include "../fmcp.h"
#undef  PFX
#define PFX	        "FMJPEG_E"
#include "debug.h"
#ifdef SUPPORT_MJE_RC
    #include "mje_rc_param.h"
#endif

//static spinlock_t jpeg_enc_lcok; //driver_lock;

/* property */
//struct video_entity_t *driver_entity;

#define MAX_NAME    50
#define MAX_README  100

struct je_property_map_t {
    int id;
    char name[MAX_NAME];
    char readme[MAX_README];
};

enum je_property_id {
    ID_SAMPLE = (MAX_WELL_KNOWN_ID_NUM + 1),
    ID_RESTART_INTERVAL,
    ID_IMAGE_QUALITY,
#ifdef SUPPORT_2FRAME
    ID_USE_FRAME_NUM,
    ID_SRC_BG_SIZE,
#endif
#ifdef SUPPORT_MJE_RC
    ID_RC_MODE,
    ID_MAX_BITRATE
#endif
};

struct je_property_map_t je_property_map[] = {
    {ID_SRC_FMT,"src_fmt",""},              // in prop: source format
    {ID_SRC_XY,"src_xy",""},                // in prop: source xy
    {ID_SRC_BG_DIM,"src_bg_dim",""},        // in prop: source bg dim
    {ID_SRC_DIM,"src_dim",""},              // in prop: source dim
    {ID_SAMPLE,"sample",""},                // in prop: sample
    {ID_RESTART_INTERVAL,"restart_interval",""},    // in porp: restart interval
    {ID_IMAGE_QUALITY,"image_quality",""},  // in porp: image quality
    {ID_CHECKSUM,"check_sum",""},      // in porp: check sum type
#ifdef SUPPORT_MJE_RC
    {ID_RC_MODE,"rc_mode","rate control mode"},             // in prop: rate control
    {ID_SRC_FRAME_RATE,"src_frame_rate","source frame rate"},   // in prop: frame rate
    {ID_FPS_RATIO,"fps_ratio","frame rate ratio"},          // in prop: frame rate
    {ID_BITRATE,"bitrate","target bitrate (Kb)"},           // in prop: rate control
    {ID_MAX_BITRATE,"max_bitrate","maximum bitrate (Kb)"},  // in prop: rate control
#endif

    {ID_BITSTREAM_SIZE,"bitstream_size",""},  // out prop: bitstream    
#ifdef SUPPORT_2FRAME
    {ID_SRC2_BG_DIM,"src2_bg_dim","source2 dim"},
    {ID_SRC2_XY,"src2_xy","roi2 xy"},
    {ID_SRC2_DIM,"src2_dim","encode resolution2"},
    {ID_SRC_BG_SIZE,"src_bg_frame_size","src bg frame size"},
    {ID_USE_FRAME_NUM,"frame_idx","src frame idx"},
#endif    
};

extern int mje_proc_init(void);
extern void mje_proc_exit(void);
extern unsigned int video_gettime_us(void);

/* utilization */
unsigned int mje_utilization_period = 5; //5sec calculation
unsigned int mje_utilization_start = 0, mje_utilization_record= 0;
unsigned int mje_engine_start = 0, mje_engine_end = 0;
unsigned int mje_engine_time = 0;

/* property lastest record */
struct mje_property_record_t *mje_property_record;

/* log & debug message */
unsigned int mje_log_level = LOG_WARNING;    //larger level, more message
unsigned int gOverflowReturnLength = 1024;

/* variable */
//struct mje_data_t   mje_private_data[MAX_ENGINE_NUM][MJE_MAX_CHANNEL];
struct mje_data_t   *mje_private_data = NULL;
#define DATA_SIZE   sizeof(struct mje_data_t)
#ifdef SUPPORT_MJE_RC
    static struct rc_entity_t *mje_rc_dev;
    //unsigned int gQualityStep = 2;
#endif

#if 1
#define DEBUG_M(level, fmt, args...) { \
    if (mje_log_level >= level) \
        printm(MJE_MODULE_NAME, fmt, ## args); }
#define DEBUG_E(fmt, args...) { \
    printm(MJE_MODULE_NAME,  fmt, ## args); \
    printk("[mje]" fmt, ##args); }
#else
#define DEBUG_M(level, fmt, args...) { \
    if (mje_log_level >= level) \
        printk(fmt, ## args);}
#endif

extern unsigned int jpg_enc_max_chn;

static void mje_print_property(struct video_entity_job_t *job, struct video_property_t *property, int out_prop)
{
    int i;
    int engine = ENTITY_FD_ENGINE(job->fd);
    int minor = ENTITY_FD_MINOR(job->fd);
    
    for (i = 0; i < MAX_PROPERTYS; i++) {
        if (property[i].id == ID_NULL)
            break;
        if (i == 0) {
            if (out_prop) {
                DEBUG_M(LOG_INFO, "{%d,%d} job %d out property:\n", engine,minor, job->id);
            }
            else {
                DEBUG_M(LOG_INFO, "{%d,%d} job %d in property:\n", engine,minor, job->id);
            }
        }
        DEBUG_M(LOG_INFO, "  ID:%d,Value:0x%x\n", property[i].id, property[i].value);
    }
}

int mje_parse_in_property(void *param, struct video_entity_job_t *job)
{
    int i = 0;
    struct mje_data_t *encode_data = (struct mje_data_t *)param;
    int idx = MAKE_IDX(jpg_enc_max_chn, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
    struct video_property_t *property = job->in_prop;
#ifdef SUPPORT_2FRAME
    int frame_buf_idx = 0;
    unsigned int src_bg_dim[2] = {0,0}, enc_bg_dim = 0;
    unsigned int src_dim[2] = {0,0}, enc_dim = 0;
    unsigned int src_xy[2] = {0,0}, enc_xy = 0;
#endif
#ifdef SUPPORT_MJE_RC
    int rc_mode = 0;
#endif

    //struct mje_data_t old_data;
    encode_data->check_sum = 0;
    //memcpy(&old_data, encode_data, DATA_SIZE);
    //memset(encode_data, 0, DATA_SIZE);
    encode_data->src_xy = 0;    // Tuba 20140711: if src_xy is not exist, it must be (0, 0)
    //encode_data->image_quality = old_data.image_quality;
    while (property[i].id != 0) {
        switch (property[i].id) {
            case ID_SRC_FMT:
                if (property[i].value != encode_data->src_fmt)
                    encode_data->updated |= MJE_REINIT;
                encode_data->src_fmt = property[i].value;
                break;
            case ID_SRC_XY:
                //if (property[i].value != old_data.src_xy)
                //    encode_data->updated |= MJE_REINIT;
            #ifdef SUPPORT_2FRAME
                src_xy[0] = property[i].value;
            #else
                encode_data->src_xy = property[i].value;
            #endif
                break;
            case ID_SRC_BG_DIM:
            #ifdef SUPPORT_2FRAME
                src_bg_dim[0] = property[i].value;
            #else
                if (property[i].value != encode_data->src_bg_dim)
                    encode_data->updated |= MJE_REINIT;
                encode_data->src_bg_dim = property[i].value;
            #endif
                break;
            case ID_SRC_DIM:
            #ifdef SUPPORT_2FRAME
                src_dim[0] = property[i].value;
            #else
                if (property[i].value != encode_data->src_dim)
                    encode_data->updated |= MJE_REINIT;
                encode_data->src_dim = property[i].value;
            #endif
                break;
            case ID_SAMPLE:
                if (property[i].value != encode_data->sample)
                    encode_data->updated |= MJE_REINIT;
                encode_data->sample = property[i].value;
                break;
            case ID_RESTART_INTERVAL:
                if (property[i].value != encode_data->restart_interval)
                    encode_data->updated |= MJE_REINIT;
                encode_data->restart_interval = property[i].value;                
                break;
            case ID_IMAGE_QUALITY:
                encode_data->image_quality = property[i].value;
                break;
        #ifdef ENABLE_CHECKSUM
            case ID_CHECKSUM:
                encode_data->check_sum = property[i].value;
                break;
        #endif
        #ifdef SUPPORT_2FRAME
            case ID_SRC2_BG_DIM:
                src_bg_dim[1] = property[i].value;
                break;
            case ID_SRC2_DIM:
                src_dim[1] = property[i].value;
                break;
            case ID_SRC2_XY:
                src_xy[1] = property[i].value;
                break;
            case ID_USE_FRAME_NUM:
                frame_buf_idx = property[i].value;
                break;
            case ID_SRC_BG_SIZE:
                encode_data->src_bg_frame_size = property[i].value;
                break;
        #endif
        #ifdef SUPPORT_MJE_RC
            case ID_RC_MODE:
                rc_mode = property[i].value;
                break;
            case ID_SRC_FRAME_RATE:
                if (encode_data->src_frame_rate != property[i].value)
                    encode_data->updated |= MJE_RC_UPDATE;
                encode_data->src_frame_rate = property[i].value;
                break;
            case ID_FPS_RATIO:
                if (encode_data->fps_ratio != property[i].value)
                    encode_data->updated |= MJE_RC_UPDATE;
                encode_data->fps_ratio = property[i].value;
                break;
            case ID_BITRATE:
                if (encode_data->bitrate != property[i].value)
                    encode_data->updated |= MJE_RC_UPDATE;
                encode_data->bitrate = property[i].value;
                break;
            case ID_MAX_BITRATE:
                if (encode_data->max_bitrate != property[i].value)
                    encode_data->updated |= MJE_RC_UPDATE;
                encode_data->max_bitrate = property[i].value;
                break;
        #endif
            default:
                break;
        }
        if (i < MJE_MAX_RECORD) {
            mje_property_record[idx].property[i].id = property[i].id;
            mje_property_record[idx].property[i].value = property[i].value;
        }
        i++;
    }
#ifdef SUPPORT_2FRAME
    switch (encode_data->src_fmt) {
        case TYPE_YUV422_2FRAMES_2FRAMES:   // [0,1],[2,3]
            if (frame_buf_idx < 2) {
                frame_buf_idx = 0;
                enc_bg_dim = src_bg_dim[0];
                enc_dim = src_dim[0];
                enc_xy = src_xy[0];
            }
            else {
                enc_bg_dim = src_bg_dim[1];
                enc_dim = src_dim[1];
                enc_xy = src_xy[1];
            }
            break;
        case TYPE_YUV422_FRAME_2FRAMES:     // [0],[1,2]
        case TYPE_YUV422_FRAME_FRAME:       // [0],[1]        
            if (frame_buf_idx < 1) {
                enc_bg_dim = src_bg_dim[0];
                enc_dim = src_dim[0];
                enc_xy = src_xy[0];
            }
            else {
                enc_bg_dim = src_bg_dim[1];
                enc_dim = src_dim[1];
                enc_xy = src_xy[1];
            }
            break;
        default:
            enc_bg_dim = src_bg_dim[0];
            enc_dim = src_dim[0];
            enc_xy = src_xy[0];
            break;
    }
    //if (encode_data->src_bg_dim != enc_bg_dim || encode_data->frame_buf_idx != frame_buf_idx)
    if (encode_data->src_bg_dim != enc_bg_dim || encode_data->src_dim != enc_dim || encode_data->frame_buf_idx != frame_buf_idx)
        encode_data->updated |= MJE_REINIT;
    encode_data->src_bg_dim = enc_bg_dim;
    encode_data->src_dim = enc_dim;
    encode_data->src_xy = enc_xy;
    encode_data->frame_buf_idx = frame_buf_idx;
#endif    
    mje_property_record[idx].property[i].id = mje_property_record[idx].property[i].value = 0;
    mje_property_record[idx].entity = job->entity;
    mje_property_record[idx].job_id = job->id;
    mje_print_property(job,job->in_prop, 0);
#ifdef SUPPORT_MJE_RC
    if (rc_mode != encode_data->rc_mode) {
        encode_data->updated |= MJE_RC_UPDATE;
        encode_data->rc_mode = rc_mode;
    }
#endif
    return 1;
}


int mje_set_out_property(void *param, struct video_entity_job_t *job)
{
    int i = 0;
    struct mje_data_t *encode_data = (struct mje_data_t *)param;
    struct video_property_t *property = job->out_prop;

    property[i].id = ID_BITSTREAM_SIZE;
    if (0 == encode_data->bitstream_size)   // Tuba 20141113
        encode_data->bitstream_size = (gOverflowReturnLength < encode_data->out_size ? gOverflowReturnLength : encode_data->out_size);
    property[i].value = encode_data->bitstream_size;
    i++;

#ifdef ENABLE_CHECKSUM
    if (encode_data->check_sum) {
        property[i].id = ID_CHECKSUM;
        //property[i].value = 0;
        i++;
    }
#endif

    property[i].id = ID_NULL;
    property[i].value = 0;    //data->xxxx
    i++;
    
    mje_print_property(job, job->out_prop, 1);
    return 1;
}


static void mje_mark_engine_start(void)
{
    //struct job_item_t *job_item = (struct job_item_t *)param;
    //int engine = job_item->engine;
    //unsigned long flags;

    //spin_lock_irqsave(&jpeg_enc_lcok, flags);
    if (mje_engine_start != 0)
        DEBUG_M(LOG_WARNING, "Warning to nested use mjd_mark_engine_start function!\n");
    mje_engine_start = video_gettime_us();
    mje_engine_end = 0;
    if (mje_utilization_start == 0) {
        mje_utilization_start = mje_engine_start;
        mje_engine_time = 0;
    }
    //spin_unlock_irqrestore(&jpeg_enc_lcok, flags);
}

static void mje_mark_engine_finish(void)
{
    //int engine, minor;
    //struct job_item_t *job_item = (struct job_item_t *)job_param;
 
    //engine = job_item->engine;
    //minor = job_item->chn;
    //driver_data = mje_private_data + (ENTITY_ENGINES * engine) + minor;

    //mje_set_out_property(driver_data, job_item->job);
    
    if (mje_engine_end != 0)
        DEBUG_M(LOG_WARNING, "Warning to nested use mark_engine_end function!\n");
    mje_engine_end = video_gettime_us();
    if (mje_engine_end > mje_engine_start)
        mje_engine_time += mje_engine_end - mje_engine_start;
    if (mje_utilization_start > mje_engine_end) {
        mje_utilization_start = 0;
        mje_engine_time = 0;
    }
    else if ((mje_utilization_start <= mje_engine_end) &&
            (mje_engine_end - mje_utilization_start >= mje_utilization_period*1000000)) {
        unsigned int utilization;
        utilization = (unsigned int)((100 * mje_engine_time) /
            (mje_engine_end - mje_utilization_start));
        if (utilization)
            mje_utilization_record = utilization;
        mje_utilization_start = 0;
        mje_engine_time = 0;
    }        
    mje_engine_start = 0;
}


int mje_preprocess(void *parm, void *priv)
{
    struct job_item_t *job_item = (struct job_item_t *)parm;
    return video_preprocess(job_item->job, priv);
}


int mje_postprocess(void *parm, void *priv)
{
    struct job_item_t *job_item = (struct job_item_t *)parm;
    return video_postprocess(job_item->job, priv);
}

int je_int;
//extern int dump_n_frame;
extern int FJpegEE_Tri(void *enc_handle, FJPEG_ENC_FRAME * ptFrame);
extern int FJpegEE_End(void *enc_handle);
extern void FJpegEE_show(void *dec_handle);

int fmje_parm_info(int engine, int chn)
{
	struct mje_data_t *encode_data;

    if (engine >= MAX_ENGINE_NUM) {
        DEBUG_E ("engine %d is over MAX engine\n", engine);
        return 0;
    }

	if (chn >= jpg_enc_max_chn) {
		DEBUG_E ("chn (%d) over MAX", chn);
		return 0;
	}
	//encode_data = &mje_private_data[engine][chn];
	encode_data = mje_private_data + chn;

	if (encode_data->handler) {
		info ("== chn (%d) info begin ==", chn);
		FJpegEE_show (encode_data->handler);
		info ("== chn (%d) info stop ==", chn);
	}
/*
	chn++;
	for (; chn < jpg_enc_max_chn; chn++) {
		encode_data = &mje_private_data[chn];
		if (encode_data->handler)
			return chn;
	}
*/
	return 0;
}


void fmje_parm_info_f(struct job_item_t *job_item)
{
	//struct mje_data_t *encode_data = &mje_private_data[job_item->engine][job_item->chn];
	struct mje_data_t *encode_data = mje_private_data + job_item->chn;

	info ("====== fmje_parm_info_f ======");
	info ("dev = %d, job_id = %d, stop = %d", job_item->chn, 
        job_item->job_id, encode_data->stop_job);
	info ("encode_data->handler = 0x%08x", (int)encode_data->handler);
	//info ("snapshot_cnt = %d", encode_data->snapshot_cnt);
}

void fmje_frame_info(struct job_item_t *job, FJPEG_ENC_FRAME *encf)
{
	info ("bitstream_size = %d", encf->bitstream_size);
	info ("u32ImageQuality = %d", encf->u32ImageQuality);
	info ("u8JPGPIC = %d", encf->u8JPGPIC);
	info ("roi xywh = %d %d %d %d", encf->roi_x, encf->roi_y, encf->roi_w, encf->roi_h);
}

#ifdef SUPPORT_MJE_RC
static int fmje_init_rc(struct mje_data_t *encode_data)
{
    struct rc_init_param_t rc_param;
    if (NULL == mje_rc_dev)
        return -1;
    // check parameter valid
    rc_param.chn = encode_data->chn;
    if (encode_data->rc_mode < EM_VRC_CBR || encode_data->rc_mode > EM_VRC_EVBR) {
        DEBUG_M(LOG_WARNING, "wrong rc mode %d\n", encode_data->rc_mode);
        return -1;
    }
    rc_param.rc_mode = encode_data->rc_mode;
    if (0 == encode_data->src_frame_rate || 0 == encode_data->fps_ratio) {
        DEBUG_M(LOG_WARNING, "wrong frame rate src_framerate = 0x%x, fps_ratio = 0x%x\n", 
            encode_data->src_frame_rate, encode_data->fps_ratio);
        return -1;
    }
    rc_param.fincr = EM_PARAM_N(encode_data->fps_ratio);
    rc_param.fbase = encode_data->src_frame_rate * EM_PARAM_M(encode_data->fps_ratio);
    rc_param.bitrate = encode_data->bitrate;
    rc_param.max_bitrate = encode_data->max_bitrate;
    rc_param.qp_constant = encode_data->image_quality;
    rc_param.max_quant = 100;
    rc_param.min_quant = 1;

    return mje_rc_dev->rc_init(&encode_data->rc_handler, &rc_param);
}

static int mje_rc_update(struct mje_data_t *encode_data)
{
    struct rc_frame_info_t rc_data;
    if (mje_rc_dev && encode_data->enable_rc && encode_data->rc_handler) {
        rc_data.slice_type = EM_SLICE_TYPE_I;
        rc_data.frame_size = encode_data->bitstream_size;
        rc_data.avg_qp = encode_data->cur_quality;
        mje_rc_dev->rc_update(encode_data->rc_handler, &rc_data);
    }
    return 0;
}
#endif

static int fmje_param_vg_init (struct mje_data_t *encode_data, FJPEG_ENC_PARAM * encp)
{
	encp->u32API_version        = 0;
	encp->u32ImageMotionDetection = 0;
	encp->sample                = encode_data->sample;
	encp->u32RestartInterval	= encode_data->restart_interval;
	encp->u32ImageWidth		    = EM_PARAM_WIDTH(encode_data->src_bg_dim);
	encp->u32ImageHeight	    = EM_PARAM_HEIGHT(encode_data->src_bg_dim);
    if (encp->u32ImageWidth & 0x0F || encp->u32ImageHeight & 0x0F) {
        printk("source frame width(%d) or source frame height(%d) is not mutiple of 16!!\n", 
            encp->u32ImageWidth, encp->u32ImageHeight);
        damnit(MJE_MODULE_NAME);
    }
    /*
     * 0: MP4 2D, 420p,
	 * 1: sequencial 1D,
	 * 2: H264 2D, 420p
	 * 3: sequencial 1D 420, one case of u82D=1, (only support when DMAWRP is configured)
	 * 4: sequencial 1D 422, (only support when DMAWRP is configured)
	*/
    if (TYPE_YUV422_MPEG4_2D_FRAME == encode_data->src_fmt)
        encode_data->input_format = 0;
    else if (TYPE_YUV420_FRAME == encode_data->src_fmt)
        encode_data->input_format = 1;  // 3
    else if (TYPE_YUV422_H264_2D_FRAME == encode_data->src_fmt)
        encode_data->input_format = 2;
    else if (TYPE_YUV422_FRAME == encode_data->src_fmt || TYPE_YUV422_FIELDS == encode_data->src_fmt)
        encode_data->input_format = 4;
#ifdef SUPPORT_2FRAME
    else if (encode_data->src_fmt >= TYPE_YUV422_2FRAMES_2FRAMES && encode_data->src_fmt <= TYPE_YUV422_FRAME_FRAME) {
        encode_data->input_format = 4;
        if (0 == encode_data->frame_buf_idx)
            encode_data->src_buf_offset = 0;
        else {
            /* not handle frame idx be bottom frame of 2 frame */
            if (0 == encode_data->src_bg_frame_size) {
                DEBUG_E("2 frame mode, src bg frame size can not be zero\n");
                damnit(MJE_MODULE_NAME);
                return -1;
            }
            if (TYPE_YUV422_2FRAMES_2FRAMES == encode_data->src_fmt)
                encode_data->src_buf_offset = encode_data->src_bg_frame_size * 2;
            else
                encode_data->src_buf_offset = encode_data->src_bg_frame_size;
        }
    }
#endif
    else {
        printk("unknown format 0x%x\n", encode_data->src_fmt);
        DEBUG_M(LOG_ERROR, "unknown format 0x%x\n", encode_data->src_fmt);
        damnit(MJE_MODULE_NAME);
        return -1;
    }
	encp->u82D					= encode_data->input_format;
    // handle not 16-bit align
    if (encode_data->src_dim != encode_data->src_bg_dim) {
        int width, height;
        // roi
        encp->roi_x                 = EM_PARAM_X(encode_data->src_xy);
        encp->roi_y                 = EM_PARAM_Y(encode_data->src_xy);
        // roi cropping
        width = EM_PARAM_WIDTH(encode_data->src_dim);
        height = EM_PARAM_HEIGHT(encode_data->src_dim);

        encp->roi_w = ((width+15)>>4)<<4;
        encp->crop_width = encp->roi_w - width;
        encp->roi_h = ((height+15)>>4)<<4;
        encp->crop_height = encp->roi_h - height;
        //encp->roi_w                 = EM_PARAM_WIDTH(encode_data->src_dim);
        //encp->roi_h                 = EM_PARAM_HEIGHT(encode_data->src_dim);
    }
    else {
        encp->roi_x                 = 0;
        encp->roi_y                 = 0;
        // because src_dim = src_bg_dim, src_dim is multiple of 16
        encp->roi_w                 = EM_PARAM_WIDTH(encode_data->src_dim);
        encp->roi_h                 = EM_PARAM_HEIGHT(encode_data->src_dim);
        encp->crop_width            = 0;
        encp->crop_height           = 0;
    }
	encode_data->U_off          = encp->u32ImageWidth * encp->u32ImageHeight;
	encode_data->V_off          = encode_data->U_off * 5 / 4;
    return 0;
}

static int fmje_create(struct mje_data_t *encode_data, FJPEG_ENC_PARAM * encp, int reinit)
{
	FJPEG_ENC_PARAM_MY enc_param;
	memcpy((void *)&enc_param.pub, (void *)encp, sizeof(FJPEG_ENC_PARAM));	

	enc_param.pfnDmaFree = fconsistent_free;
	enc_param.pfnDmaMalloc = fconsistent_alloc;
	enc_param.pfnMalloc = fkmalloc;
	enc_param.pfnFree = fkfree;
	enc_param.u32CacheAlign = CACHE_LINE;
	enc_param.pu32BaseAddr = (unsigned int*)mcp100_dev->va_base;

	if (reinit)
		return FJpegEncCreate_fake(&enc_param, encode_data->handler);
	else {
		if (encode_data->handler) {
			DEBUG_E("previous one ch %d was not released", encode_data->chn);
			return -1;
		}
		// to create the jpeg encoder object
		encode_data->handler = FJpegEncCreate(&enc_param);
		if (encode_data->handler == NULL )
			return -1;
		else
			return 0;
	}
    return 0;
}



int fmje_encode_trigger(struct job_item_t *job_item)
{
	int engine, chn;
    struct video_entity_job_t *job;
	struct mje_data_t *encode_data;
	int ret;
	FJPEG_ENC_FRAME encf;

    DEBUG_M(LOG_DEBUG, "mje start job %d\n", job_item->job_id);
    job = (struct video_entity_job_t *)job_item->job;
    chn = job_item->chn;
    engine = job_item->engine;
    if (engine >= MAX_ENGINE_NUM) {
        DEBUG_E ("engine %d is over MAX", engine);
        return -1;
    }
	if (chn >= jpg_enc_max_chn) {
		DEBUG_E ("chn (%d) over MAX", chn);
		return -1;					// JOB_STATUS_FAIL
	}
    //encode_data = &mje_private_data[engine][chn];
    encode_data = mje_private_data + job_item->chn;

    mje_parse_in_property(encode_data, job);
    if (encode_data->updated) {
    #ifdef SUPPORT_MJE_RC
        if (MJE_REINIT & encode_data->updated) {
            FJPEG_ENC_PARAM encp;
        
            if (fmje_param_vg_init(encode_data, &encp) < 0)
                return -1;
            if (fmje_create(encode_data, &encp, encode_data->reinit) < 0)
                return -1;
            encode_data->reinit = 1;
            //encode_data->updated = 0;
        }
        if (MJE_RC_UPDATE & encode_data->updated) {
            if (fmje_init_rc(encode_data) < 0) {
                encode_data->enable_rc = 0;
            }
            else
                encode_data->enable_rc = 1;
        }
        encode_data->updated = 0;
    #else
        FJPEG_ENC_PARAM encp;
        if (fmje_param_vg_init(encode_data, &encp) < 0)
            return -1;
        if (fmje_create(encode_data, &encp, encode_data->reinit) < 0)
            return -1;
        encode_data->reinit = 1;
        encode_data->updated = 0;
    #endif
    }

	if (!encode_data->handler) {
		DEBUG_E("fatal error: put job but never init");
		return -1;					// JOB_STATUS_FAIL
	}
    /*
	if (encode_data->stop_job)
		return -1;					// FAIL
	*/
	if (0 == job->in_buf.buf[0].addr_va) {
        DEBUG_E("job in buffer is NULL\n");
        damnit(MJE_MODULE_NAME);
        return -1;
    } 

	if (mje_preprocess (job_item, NULL) < 0) {
        DEBUG_M(LOG_WARNING, "video_preprocess return fail");
		//err ("video_preprocess return fail");
		return -1;					// FAIL
	}

    //if ((0 == job->in_buf.buf[0].addr_va) || (0 == job->out_buf.buf[0].addr_va)) {
    if (0 == job->out_buf.buf[0].addr_va) {
        DEBUG_M(LOG_WARNING, "job out pointer is NULL");
		//err("job in/out pointer err");	
		return -1;
	}
	/* preprocess:
	 * image quality (1~100)
	 *  1 (worst quality) and 100 (best quality).	
	*/
#ifdef SUPPORT_MJE_RC
    if (mje_rc_dev && encode_data->enable_rc) {
        encode_data->cur_quality = 
        encf.u32ImageQuality = mje_rc_dev->rc_get_quant(encode_data->rc_handler, NULL);
    }
    else {
        encode_data->cur_quality = 
        encf.u32ImageQuality = encode_data->image_quality;
    }
#else
	encf.u32ImageQuality = encode_data->image_quality;
#endif
	encf.u8JPGPIC = 1;
    if (encf.u32ImageQuality < 1 || encf.u32ImageQuality > 100) {
        DEBUG_M(LOG_WARNING, "image quality %d is out of range, force to be 50\n", encf.u32ImageQuality);
        encf.u32ImageQuality = 50;
        //return -1;
    }
    /*
	if (dump_n_frame) {
		info ("mje_proc.luma_qtbl = 0x%08x", (int)mje_proc.luma_qtbl);
		info ("mje_proc.chroma_qtbl = 0x%08x", (int)mje_proc.chroma_qtbl);
	}
	if (mje_proc.luma_qtbl && mje_proc.chroma_qtbl) {
		if (FJpegEncSetQTbls(pv->handler, mje_proc.luma_qtbl, mje_proc.chroma_qtbl) < 0)
			return -1;					// FAIL
	}
	*/
#ifdef SUPPORT_2FRAME
    encf.pu8YUVAddr[0] = (unsigned char *)job->in_buf.buf[0].addr_pa + encode_data->src_buf_offset;
#else
	encf.pu8YUVAddr[0] = (unsigned char *)job->in_buf.buf[0].addr_pa;
#endif
	encf.pu8YUVAddr[1] = encf.pu8YUVAddr[0] + encode_data->U_off;
	encf.pu8YUVAddr[2] = encf.pu8YUVAddr[0] + encode_data->V_off;

#ifdef ENABLE_CHECKSUM
    job_item->check_sum_type = encode_data->check_sum;
#endif
    /*
	if ((mje_proc.roi_x < 0) || (mje_proc.roi_y < 0)) {
		encf.roi_x = -1;	// keep original setting
		encf.roi_y = -1;	// keep original setting
	}
	else {
		encf.roi_x = mje_proc.roi_x;
		encf.roi_y = mje_proc.roi_y;
	}
	*/
	encf.roi_x = EM_PARAM_X(encode_data->src_xy);
	encf.roi_y = EM_PARAM_Y(encode_data->src_xy);
	encf.roi_w = -1;	// keep original setting
	encf.roi_h = -1;	// keep original setting
	/*
	enc_out_info = (mje_OutInfo *)(job->out_header->addr_va +outb_hdr->drvinfo_offset);
	enc_out_info->bs_offset = 0;
	*/
	encf.pu8BitstreamAddr = (unsigned char *)job->out_buf.buf[0].addr_pa;
    encode_data->out_size = 
	encf.bitstream_size = job->out_buf.buf[0].size;
	//if (dump_n_frame)
	//	fmje_parm_info_f1 (job, &encf);
	ret = FJpegEE_Tri(encode_data->handler, &encf);
	mje_mark_engine_start();
	return ret;
}

static int fmje_process_done (struct job_item_t *job_item)
{
	int bs_len;
    //struct mje_data_t *encode_data = &mje_private_data[job_item->engine][job_item->chn];
    struct mje_data_t *encode_data = mje_private_data + job_item->chn;
	//struct video_entity_t *entity = job->entity;
	//struct mje_vg_private * pv = (struct mje_vg_private *)entity->private;
	//struct v_graph_info * outb_hdr = (struct  v_graph_info *)job->out_header->addr_va;
	//mje_OutInfo *enc_out_info = (mje_OutInfo *)(job->out_header->addr_va +outb_hdr->drvinfo_offset);
    mje_mark_engine_finish();

	bs_len = FJpegEE_End(encode_data->handler);
	encode_data->bitstream_size = bs_len;
    if (bs_len <= 0 || bs_len > encode_data->out_size) {
		 int quality;
    #ifdef SUPPORT_MJE_RC
        quality = encode_data->cur_quality;
    #else
        quality = encode_data->image_quality;
    #endif
        DEBUG_E("jpeg encode bitstream buffer full: quality %d, buffer size 0x%x (len %d)\n", quality, encode_data->out_size, bs_len);
		encode_data->bitstream_size = 0;
        mje_set_out_property(encode_data, (struct video_entity_job_t *)job_item->job);
        mje_postprocess(job_item, (void *)NULL);    // Tuba 20141113: must postprocess
        damnit(MJE_MODULE_NAME);
		return -1;
    }
#ifdef HANDLE_BS_CACHEABLE  // Tuba 20140225: handle bitstream buffer cacheable
    job_item->bs_size = encode_data->bitstream_size;
#endif
	//enc_out_info->Tag = ENTITY_FD(entity->major, entity->minor);
	//enc_out_info->bs_length = bs_len;
	encode_data->bitstream_size = bs_len;
    /*
	if (encode_data->snapshot_cnt > 0)
		encode_data->snapshot_cnt--;
    */
#ifdef SUPPORT_MJE_RC
    mje_rc_update(encode_data);
#endif

    mje_set_out_property(encode_data, (struct video_entity_job_t *)job_item->job);

	if (mje_postprocess(job_item, (void *)NULL) < 0) {
		printk ("video_postprocess return fail\n");
		return -1;
	}

    /*
	mcp100_proc_dump (1,
		entity->minor & 0xFF,
		TYPE_JPG_ENCODER,
		(unsigned char *)job->out_buf->addr_va,
		bs_len, job->swap, 1);
    */
	return 0;
}

static void fmje_isr(int irq, void *dev_id, unsigned int base)
{
	if (je_int & IT_JPGE) {
		unsigned int int_sts;
		int_sts = mSEQ_INTFLG(base);
		// clr interrupt
		mSEQ_INTFLG(base) = int_sts;
		je_int = int_sts;
	}
}

static void fmje_release(struct mje_data_t *encode_data)
{
	if (encode_data->handler)
        FJpegEncDestroy(encode_data->handler);
	encode_data->handler = NULL;
}

static int mje_putjob(void *parm)
{
    //mcp_putjob((struct video_entity_job_t*)parm, TYPE_JPEG_ENCODER, 0, 0);
    mcp_putjob((struct video_entity_job_t*)parm, TYPE_JPEG_ENCODER);
    return JOB_STATUS_ONGOING;
}


static int mje_stop(void *parm, int engine, int minor)
{
    //struct mje_data_t *encode_data;

    if (minor >= jpg_enc_max_chn || engine >= MAX_ENGINE_NUM)
        printk("chn %d is over max channel %d or engine %d is over max engine %d\n", 
            minor, jpg_enc_max_chn, engine, MAX_ENGINE_NUM);
    else
        mcp_stopjob(TYPE_JPEG_ENCODER, engine, minor);
#ifdef SUPPORT_MJE_RC
    if (1) {
        struct mje_data_t *encode_data;
        //encode_data = &mje_private_data[engine][minor];
        encode_data = mje_private_data + minor;
        encode_data->enable_rc = 0;
    }
#endif

    //encode_data = &mje_private_data[minor];
    //encode_data->stop_job = 1;

    return 0;
}

static struct je_property_map_t *mje_get_propertymap(int id)
{
    int i;
    
    for (i = 0; i < sizeof(je_property_map)/sizeof(struct je_property_map_t); i++) {
        if (je_property_map[i].id == id) {
            return &je_property_map[i];
        }
    }
    return 0;
}


static int mje_queryid(void *parm, char *str)
{
    int i;
    
    for (i = 0; i < sizeof(je_property_map)/sizeof(struct je_property_map_t); i++) {
        if (strcmp(je_property_map[i].name,str) == 0) {
            return je_property_map[i].id;
        }
    }
    printk("mje_queryid: Error to find name %s\n", str);
    return -1;
}


static int mje_querystr(void *parm, int id, char *string)
{
    int i;
    for (i = 0; i < sizeof(je_property_map)/sizeof(struct je_property_map_t); i++) {
        if (je_property_map[i].id == id) {
            memcpy(string, je_property_map[i].name, sizeof(je_property_map[i].name));
            return 0;
        }
    }
    printk("mje_querystr: Error to find id %d\n", id);
    return -1;
}

static int mje_getproperty(void *parm, int engine, int minor, char *string)
{
    int id,value = -1;
    struct mje_data_t *encode_data;
    //struct driver_data_t *driver_data;

    if (engine >= MAX_ENGINE_NUM) {
        printk("Error over engine number %d\n", MAX_ENGINE_NUM);
        return -1;
    }
    if (minor >= jpg_enc_max_chn) {
        printk("Error over minor number %d\n", jpg_enc_max_chn);
        return -1;
    }

    //encode_data = &mje_private_data[engine][minor];
    encode_data = mje_private_data + minor;
    id = mje_queryid(parm, string);
    switch (id) {
        case ID_SRC_FMT:
            value = encode_data->src_fmt;
            break;
        case ID_SRC_XY:
            value = encode_data->src_xy;
            break;
        case ID_SRC_BG_DIM:
            value = encode_data->src_bg_dim;
            break;
        case ID_SRC_DIM:
            value = encode_data->src_dim;
            break;
        case ID_SAMPLE:
            value = encode_data->sample;
            break;
        case ID_RESTART_INTERVAL:
            value = encode_data->restart_interval;
            break;
        case ID_BITSTREAM_SIZE:
            value = encode_data->bitstream_size;
            break;
        case ID_IMAGE_QUALITY:
            value = encode_data->image_quality;
        default:
            break;
    }
    return value;
}


int mje_dump_property_value(char *str, int chn)
{
    // dump property
    int i = 0;
    struct je_property_map_t *map;
    unsigned int id, value;
    //unsigned long flags;
    int engine = 0;
    int idx = MAKE_IDX(jpg_enc_max_chn, engine, chn);
    int len = 0;
    
    //spin_lock_irqsave(&jpeg_enc_lcok, flags);

    len += sprintf(str+len, "Jpeg encoder property channel %d\n", chn);
    //printk("\n%s engine%d ch%d job %d\n", mje_property_record[idx].entity->name,
    //    engine, chn, mje_property_record[idx].job_id);
    len += sprintf(str+len, "=============================================================\n");
    len += sprintf(str+len, "ID    Name(string)  Value(hex)  Readme\n");
    do {
        id = mje_property_record[idx].property[i].id;
        if (id == ID_NULL)
            break;
        value = mje_property_record[idx].property[i].value;
        map = mje_get_propertymap(id);
        if (map) {
            len += sprintf(str+len, "%02d  %14s   %08x  %s\n", id, map->name, value, map->readme);
        }
        i++;
    } while (1);
    len += sprintf(str+len, "=============================================================\n");
    //spin_unlock_irqrestore(&jpeg_enc_lcok, flags);
    return len;
}

#ifdef SUPPORT_MJE_RC
int mje_rc_register(struct rc_entity_t *entity)
{
    mje_rc_dev = entity;
    return 0;
}

int mje_rc_deregister(void)
{
    mje_rc_dev = NULL;
    return 0;
}
#endif

struct video_entity_ops_t mje_driver_ops ={
    putjob:      &mje_putjob,
    stop:        &mje_stop,
    queryid:     &mje_queryid,
    querystr:    &mje_querystr,
    getproperty: &mje_getproperty,
    //register_clock: &mje_register_clock,
};    


struct video_entity_t mje_entity={
    type:       TYPE_JPEG_ENC,
    name:       "mje",
    engines:    MAX_ENGINE_NUM,
    minors:     MJE_MAX_CHANNEL,
    ops:        &mje_driver_ops
};


static mcp100_rdev mjedv = {
	.job_tri = fmje_encode_trigger,
	.job_done = fmje_process_done,
	.codec_type = FMJPEG_ENCODER_MINOR,
	.handler = fmje_isr,
	.dev_id = NULL,
	.switch22 = NULL,
	.parm_info = fmje_parm_info,
	.parm_info_frame = fmje_parm_info_f,
};

int mje_drv_init(char *name, int max_num, int behavior_bitmap)
{
    int i;//, j;

    mje_property_record = kzalloc(sizeof(struct mje_property_record_t) * MAX_ENGINE_NUM * jpg_enc_max_chn, GFP_KERNEL);

    // Tuba 20141229: set maximal channel id
    mje_entity.minors = jpg_enc_max_chn;
    video_entity_register(&mje_entity);

    //spin_lock_init(&jpeg_enc_lcok);

    if (mje_proc_init() < 0)
        goto mje_init_proc_fail;

    mje_private_data = kzalloc(sizeof(struct mje_data_t) * jpg_enc_max_chn, GFP_KERNEL);
    if (NULL == mje_private_data) {
        DEBUG_E("Fail to allocate jpeg enc private data\n");
        goto mje_init_proc_fail;
    }
    memset(mje_private_data, 0, sizeof(struct mje_data_t)*jpg_enc_max_chn);
    for (i = 0; i < jpg_enc_max_chn; i++) {
        struct mje_data_t *encode_data;
        encode_data = mje_private_data + i;
        encode_data->chn = i;
        encode_data->engine = 0;
    }
    if (mcp100_register(&mjedv, TYPE_JPEG_ENCODER, jpg_enc_max_chn) < 0)
        goto mje_init_proc_fail;
    
        
    return 0;

mje_init_proc_fail:
    mje_proc_exit();
    video_entity_deregister(&mje_entity);
    if (mje_private_data)
        kfree(mje_private_data);
    
    return -1;
}


void mje_drv_close(void)
{
    int i;//, j;

    video_entity_deregister(&mje_entity);
    kfree(mje_property_record);

    mje_proc_exit();

    mcp100_deregister(TYPE_JPEG_ENCODER);
	for (i = 0; i < jpg_enc_max_chn; i++) {
        struct mje_data_t *encode_data;
        encode_data = mje_private_data+ i;
        if (NULL != encode_data->handler) {
        }
        if (encode_data->handler != NULL) {
			fmje_release(encode_data);
        #ifdef SUPPORT_MJE_RC
            if (mje_rc_dev && encode_data->rc_handler)
                mje_rc_dev->rc_clear(encode_data->rc_handler);
        #endif
		}
    }
    if (mje_private_data)
        kfree(mje_private_data);
}

