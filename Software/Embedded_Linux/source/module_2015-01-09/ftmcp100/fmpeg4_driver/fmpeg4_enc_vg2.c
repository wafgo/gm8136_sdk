/**
 * @file fmpeg4_enc_vg2.c
 *  The videograph interface of driver template.
 * Copyright (C) 2011 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.4 $
 * $Date: 2012/11/01 06:48:03 $
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

#include "fmpeg4_enc_entity.h"
#include "fmpeg4_enc_vg2.h"
#include "fmpeg4.h"

#include "fmcp.h"
#include "common/define.h"
#include "encoder.h"
#include "global_e.h"
#include "image_e.h"
#include "motion.h"
#include "bitstream.h"
#include "mbfunctions.h"
#include "dma_e.h"
#include "me_e.h"
#include "../common/quant_matrix.h"
#include "../common/dma.h"
#include "../common/vpe_m.h"
#include "../common/mp4.h"
#include "encoder_ext.h"
#include "fmcp_type.h"
#include "mp4e_param.h"
/*
#undef  PFX
#define PFX	        "FMPEG4_E"
#include "debug.h"
*/


static spinlock_t mpeg4_enc_lcok; //driver_lock;

#define MAX_NAME    50
#define MAX_README  100

#if 0
#define DEBUG_M(level, fmt, args...) { \
    if (mp4e_log_level >= level) \
        printm(MP4E_MODULE_NAME, fmt, ## args); }
#else
#define DEBUG_M(level, fmt, args...) { \
    if (mp4e_log_level == LOG_DIRECT) \
        printk(fmt, ## args); \
    else if (mp4e_log_level >= level) \
        printm(MP4E_MODULE_NAME, fmt, ## args); }
    
#define DEBUG_E(fmt, args...) { \
        printm(MP4E_MODULE_NAME,  fmt, ## args); \
        printk("[mp4e]" fmt, ##args); }
#endif


struct mp4e_property_map_t {
    int id;
    char name[MAX_NAME];
    char readme[MAX_README];
};

enum mp4e_property_id {
    ID_RC_MODE = (MAX_WELL_KNOWN_ID_NUM + 1),
    ID_MAX_BITRATE,
    ID_MB_INFO_OFFSET,
    ID_MB_INFO_LENGTH,
    ID_FORCE_INTRA,
#ifdef SUPPORT_2FRAME
    ID_USE_FRAME_NUM,
    ID_SRC_BG_SIZE,
#endif
};

struct mp4e_property_map_t mp4e_property_map[] = {
    {ID_SRC_FMT,"src_fmt",""},                  // in prop: source format
    {ID_SRC_XY,"src_xy",""},                    // in prop: source xy
    {ID_SRC_BG_DIM,"src_bg_dim",""},            // in prop: source bg dim
    {ID_SRC_DIM,"src_dim",""},                  // in prop: source dim
    {ID_IDR_INTERVAL,"idr_interval",""},        // in prop: gop idr period
    {ID_SRC_INTERLACE,"src_interlace",""},      // in prop: source type
    {ID_DI_RESULT,"di_result",""},              // in prop: source type

    /*********** rate control ***********/
    {ID_RC_MODE,"rc_mode",""},                  // in prop: rate control
    {ID_SRC_FRAME_RATE,"src_frame_rate",""},    // in prop: frame rate
    {ID_FPS_RATIO,"fps_ratio",""},              // in prop: frame rate
    {ID_INIT_QUANT,"init_quant",""},            // in prop: rate control
    {ID_MAX_QUANT,"max_quant",""},              // in prop: rate control
    {ID_MIN_QUANT,"min_quant",""},              // in prop: rate control
    {ID_BITRATE,"bitrate",""},                  // in prop: rate control
    {ID_MAX_BITRATE,"max_bitrate",""},          // in prop: rate control

    {ID_MOTION_ENABLE,"motion_en",""},          // in prop: output motion info
    {ID_FORCE_INTRA,"force_intra",""},          // in prop: force intra

    /********** output property **********/
    {ID_QUANT,"quant",""},                        // out prop: quant
    {ID_MB_INFO_OFFSET,"mb_info_offset",""},      // out prop: mb offset
    {ID_MB_INFO_LENGTH,"mb_info_length",""},      // out prop: mb length
    {ID_BITSTREAM_OFFSET,"bitstream_offset",""},  // out prop: bitstream offset
    {ID_SLICE_TYPE,"slice_type",""},              // out prop: slice type
    {ID_BITSTREAM_SIZE,"bitstream_size",""},      // out prop: bitstream

#ifdef SUPPORT_2FRAME
    {ID_SRC2_BG_DIM,"src2_bg_dim","source2 dim"},
    {ID_SRC2_XY,"src2_xy","roi2 xy"},
    {ID_SRC2_DIM,"src2_dim","encode resolution2"},
    {ID_USE_FRAME_NUM,"frame_idx","src frame idx"},
    {ID_SRC_BG_SIZE,"src_bg_frame_size","src bg frame size"},
#endif
};

extern int mp4e_proc_init(void);
extern void mp4e_proc_exit(void);
extern void FMpeg4EE_show(void *enc_handle);
extern int FMpeg4EE_Tri(void *enc_handle, FMP4_ENC_FRAME * pframe);
extern int FMpeg4EE_End(void *enc_handle);
extern void switch2mp4e(void * codec, ACTIVE_TYPE curr);
extern unsigned int video_gettime_us(void);

extern unsigned int mp4_max_width;
extern unsigned int mp4_max_height;
#ifdef MPEG4_COMMON_PRED_BUFFER
    extern struct buffer_info_t mp4e_pred_buf;
#endif

/* utilization */
unsigned int mp4e_utilization_period = 5; //5sec calculation
unsigned int mp4e_utilization_start = 0, mp4e_utilization_record = 0;
unsigned int mp4e_engine_start = 0, mp4e_engine_end = 0;
unsigned int mp4e_engine_time = 0;

/* property lastest record */
struct mp4e_property_record_t *mp4e_property_record = NULL;

/* log & debug message */
unsigned int mp4e_log_level = LOG_WARNING;    //larger level, more message

/* variable */
struct mp4e_data_t  mp4e_private_data[MAX_ENGINE_NUM][MP4E_MAX_CHANNEL];
#define DATA_SIZE   sizeof(struct mp4e_data_t)

struct rc_entity_t *mp4e_rc_dev = NULL;
unsigned int gOverflowReturnLength = 1024; // Tuba 20141113

#define BS_OFFSET 0

static void mp4e_print_property(struct video_entity_job_t *job, struct video_property_t *property)
{
    int i;
    int engine = ENTITY_FD_ENGINE(job->fd);
    int minor = ENTITY_FD_MINOR(job->fd);
    for (i = 0; i < MAX_PROPERTYS; i++) {
        if (property[i].id == ID_NULL)
            break;
        if (i == 0)
            DEBUG_M(LOG_INFO, "{%d,%d} job %d property:\n", engine,minor, job->id);
        DEBUG_M(LOG_INFO, "  ID:%d,Value:0x%x\n", property[i].id, property[i].value);
    }
}

int mp4e_parse_in_property(void *param, struct video_entity_job_t *job)
{
    int i = 0;
    struct mp4e_data_t *encode_data = (struct mp4e_data_t *)param;
    int idx = MAKE_IDX(MP4E_MAX_CHANNEL, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
    struct video_property_t *property = job->in_prop;
    //struct mp4e_data_t old_data;
#ifdef SUPPORT_2FRAME
    int frame_buf_idx = 0;
    unsigned int src_bg_dim[2] = {0,0}, enc_bg_dim = 0;
    unsigned int src_dim[2] = {0,0}, enc_dim = 0;
    unsigned int src_xy[2] = {0,0}, enc_xy = 0;
#endif

    //memcpy(&old_data, encode_data, sizeof(struct mp4e_data_t));
    //memset(encode_data, 0, DATA_SIZE);
    while (property[i].id != 0) {
        switch (property[i].id) {
            case ID_SRC_FMT:
                if (property[i].value != encode_data->src_fmt) {
                    encode_data->updated |= MP4E_REINIT;
                }
                encode_data->src_fmt = property[i].value;
                break;
            case ID_SRC_XY:
                //if (property[i].value != encode_data->src_xy)
                //    encode_data->updated |= MP4E_ROI_REINIT;
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
                    encode_data->updated |= MP4E_REINIT;
                encode_data->src_bg_dim = property[i].value;
            #endif
                break;
            case ID_SRC_DIM:
            #ifdef SUPPORT_2FRAME
                src_dim[0] = property[i].value;
            #else
                if (property[i].value != encode_data->src_dim) {
                    encode_data->updated |= MP4E_REINIT;
                }
                encode_data->src_dim = property[i].value;
            #endif
                break;
            case ID_IDR_INTERVAL:
                if (property[i].value != encode_data->idr_period)
                    encode_data->updated |= MP4E_GOP_REINIT;
                encode_data->idr_period = property[i].value;
                break;
            case ID_SRC_INTERLACE:
                encode_data->src_interlace = property[i].value;
                break;
            case ID_DI_RESULT:
                encode_data->di_result = property[i].value;
                break;
            case ID_RC_MODE:
                if (property[i].value != encode_data->rc_mode) {
                    //printk("update rc: rc_mode %d -> %d\n", encode_data.rc_mode, property[i].value);
                    encode_data->updated |= MP4E_RC_REINIT;
                }
                encode_data->rc_mode = property[i].value;
                break;
            case ID_SRC_FRAME_RATE:
                if (property[i].value != encode_data->src_frame_rate) {
                    //printk("update rc: src_frame_rate %d -> %d\n", encode_data.src_frame_rate, property[i].value);
                    encode_data->updated |= MP4E_RC_REINIT;
                }
                encode_data->src_frame_rate = property[i].value;
                break;
            case ID_FPS_RATIO:
                if (property[i].value != encode_data->fps_ratio) {
                    //printk("update rc: fps_ratio %d -> %d\n", encode_data.fps_ratio, property[i].value);
                    encode_data->updated |= MP4E_RC_REINIT;
                }
                encode_data->fps_ratio = property[i].value;
                break;
            case ID_INIT_QUANT:
                if (property[i].value != encode_data->init_quant) {
                    //printk("update rc: init_quant %d -> %d\n", encode_data.init_quant, property[i].value);
                    encode_data->updated |= MP4E_RC_REINIT;
                }
                encode_data->init_quant = property[i].value;
                break;
            case ID_MAX_QUANT:
                if (property[i].value != encode_data->max_quant) {
                    //printk("update rc: max_quant %d -> %d\n", encode_data.max_quant, property[i].value);
                    encode_data->updated |= MP4E_RC_REINIT;
                }
                encode_data->max_quant = property[i].value;
                break;
            case ID_MIN_QUANT:
                if (property[i].value != encode_data->min_quant) {
                    //printk("update rc: min_quant %d -> %d\n", encode_data.min_quant, property[i].value);
                    encode_data->updated |= MP4E_RC_REINIT;
                }
                encode_data->min_quant = property[i].value;
                break;
            case ID_BITRATE:
                if (property[i].value != encode_data->bitrate) {
                    //printk("update rc: bitrate %d -> %d\n", encode_data.bitrate, property[i].value);
                    encode_data->updated |= MP4E_RC_REINIT;
                }
                encode_data->bitrate = property[i].value;
                break;
            case ID_MAX_BITRATE:
                if (property[i].value != encode_data->max_bitrate) {
                    //printk("update rc: max_bitrate %d -> %d\n", encode_data.max_bitrate, property[i].value);
                    encode_data->updated |= MP4E_RC_REINIT;
                }
                encode_data->max_bitrate = property[i].value;
                break;
            case ID_MOTION_ENABLE:
                encode_data->motion_enable = property[i].value;
                break;
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
            default:
                //printk("input property %d, value %d\n", property[i].id, property[i].value);
                break;
        }
        if (i < MP4E_MAX_RECORD) {
            mp4e_property_record[idx].property[i].id = property[i].id;
            mp4e_property_record[idx].property[i].value = property[i].value;
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
        encode_data->updated |= MP4E_REINIT;
    encode_data->src_bg_dim = enc_bg_dim;
    encode_data->src_dim = enc_dim;
    encode_data->src_xy = enc_xy;
    encode_data->frame_buf_idx = frame_buf_idx;
#endif
    mp4e_property_record[idx].property[i].id = mp4e_property_record[idx].property[i].value = 0;
    mp4e_property_record[idx].entity = job->entity;
    mp4e_property_record[idx].job_id = job->id;
    mp4e_print_property(job, job->in_prop);
    return 1;
}


int mp4e_set_out_property(void *param, struct video_entity_job_t *job)
{
    int i = 0;
    struct mp4e_data_t *encode_data = (struct mp4e_data_t *)param;
    struct video_property_t *property = job->out_prop;

    property[i].id = ID_SLICE_TYPE;
    property[i].value = encode_data->slice_type;
    i++;
    
    property[i].id = ID_BITSTREAM_SIZE;
    if (0 == encode_data->bitstream_size)   // Tuba 20141113
        encode_data->bitstream_size = (gOverflowReturnLength < encode_data->out_size ? gOverflowReturnLength : encode_data->out_size);
    property[i].value = encode_data->bitstream_size;
    i++;

    property[i].id = ID_BITSTREAM_OFFSET;
    property[i].value = encode_data->bitstream_offset;
    i++;
     
    property[i].id = ID_NULL;
    i++;

    mp4e_print_property(job, job->out_prop);
    return 1;
}


//static void mp4e_mark_engine_start(void *param)
static void mp4e_mark_engine_start(void)
{
    //struct job_item_t *job_item = (struct job_item_t *)param;
    //int engine = job_item->engine;
    unsigned long flags;
    spin_lock_irqsave(&mpeg4_enc_lcok, flags);
    if (mp4e_engine_start != 0)
        DEBUG_M(LOG_WARNING, "Warning to nested use mark_engine_start function!\n");
    mp4e_engine_start = video_gettime_us();
    mp4e_engine_end = 0;
    if (mp4e_utilization_start == 0) {
        mp4e_utilization_start = mp4e_engine_start;
        mp4e_engine_time = 0;
    }
    spin_unlock_irqrestore(&mpeg4_enc_lcok, flags);
}

//static void mp4e_mark_engine_finish(void *job_param)
static void mp4e_mark_engine_finish(void)
{
    //int engine, minor;
    //struct job_item_t *job_item = (struct job_item_t *)job_param;
 
    //engine = job_item->engine;
    //minor = job_item->chn;
    //driver_data = mp4e_private_data + (ENTITY_ENGINES * engine) + minor;
    //mp4e_set_out_property(driver_data, job_item->job);
    
    if (mp4e_engine_end != 0)
        DEBUG_M(LOG_WARNING, "Warning to nested use mark_engine_end function!\n");
    mp4e_engine_end = video_gettime_us();
    if (mp4e_engine_end > mp4e_engine_start)
        mp4e_engine_time += mp4e_engine_end - mp4e_engine_start;
    if (mp4e_utilization_start > mp4e_engine_end) {
        mp4e_utilization_start = 0;
        mp4e_engine_time = 0;
    }
    else if ((mp4e_utilization_start <= mp4e_engine_end) &&
            (mp4e_engine_end - mp4e_utilization_start >= mp4e_utilization_period*1000000)) {
        unsigned int utilization;
        utilization = (unsigned int)((100 * mp4e_engine_time) /
            (mp4e_engine_end - mp4e_utilization_start));
        if (utilization)
            mp4e_utilization_record = utilization;
        mp4e_utilization_start = 0;
        mp4e_engine_time = 0;
    }        
    mp4e_engine_start = 0;
}


int mp4e_preprocess(void *parm, void *priv)
{
    struct job_item_t *job_item = (struct job_item_t *)parm;
    return video_preprocess(job_item->job, priv);
}


int mp4e_postprocess(void *parm, void *priv)
{
    struct job_item_t *job_item = (struct job_item_t *)parm;
    return video_postprocess(job_item->job, priv);
}

int fmp4e_parm_info(int engine, int chn)
{
	struct mp4e_data_t *encode_data;

    if (engine >= MAX_ENGINE_NUM) {
        printk("engine %d is over MAX engine\n", engine);
        return 0;
    }

	if (chn >= MP4E_MAX_CHANNEL) {
		printk("chn (%d) over MAX", chn);
		return 0;
	}
	encode_data = &mp4e_private_data[engine][chn];

	if (encode_data->handler) {
		printk("== chn (%d) info begin ==", chn);
		FMpeg4EE_show (encode_data->handler);
		printk("== chn (%d) info stop ==", chn);
	}
/*
	chn++;
	for (; chn < MP4E_MAX_CHANNEL; chn++) {
		encode_data = &mp4e_private_data[chn];
		if (encode_data->handler)
			return chn;
	}
*/
	return 0;
}


void fmp4e_parm_info_f(struct job_item_t *job_item)
{
	struct mp4e_data_t *encode_data = &mp4e_private_data[job_item->engine][job_item->chn];

	printk("====== fmp4e_parm_info_f ======");
	printk("dev = %d, job_id = %d, stop = %d", job_item->chn, 
        job_item->job_id, encode_data->stop_job);
	printk("encode_data->handler = 0x%08x", (int)encode_data->handler);
	//printk ("snapshot_cnt = %d", encode_data->snapshot_cnt);
}
/*
void fmp4e_frame_info(struct job_item_t *job, FJPEG_ENC_FRAME *encf)
{
	printk ("bitstream_size = %d", encf->bitstream_size);
	printk ("quant = %d", encf->u32ImageQuality);
	printk ("u8JPGPIC = %d", encf->u8JPGPIC);
	printk ("roi xywh = %d %d %d %d", encf->roi_x, encf->roi_y, encf->roi_w, encf->roi_h);
}
*/

static int fmp4e_param_vg_init (struct mp4e_data_t *encode_data, FMP4_ENC_PARAM_WRP *encp_w)
{
    FMP4_ENC_PARAM * encp = &encp_w->enc_p;

    memset(encp, 0, sizeof(FMP4_ENC_PARAM));
    encp->u32FrameWidth     = EM_PARAM_WIDTH(encode_data->src_bg_dim);
    encp->u32FrameHeight    = EM_PARAM_HEIGHT(encode_data->src_bg_dim);

    if (encp->u32FrameWidth & 0x0F || encp->u32FrameHeight & 0x0F) {
        DEBUG_E("source frame width(%d) or source frame height(%d) is not mutiple of 16!!\n", 
            encp->u32FrameWidth, encp->u32FrameHeight);
        damnit(MP4E_MODULE_NAME);
    }

    if (encode_data->src_bg_dim != encode_data->src_dim) {
        //printk("roi enable: xy = %x\n", encode_data->src_xy);
        // roi check & crop input
    #ifndef ENABLE_VARIOUS_RESOLUTION
        encode_data->frame_width = encp->roi_w = EM_PARAM_WIDTH(encode_data->src_dim);
        encode_data->frame_height = encp->roi_h = EM_PARAM_HEIGHT(encode_data->src_dim);
        encp->roi_x = EM_PARAM_X(encode_data->src_xy);
        encp->roi_y = EM_PARAM_Y(encode_data->src_xy);
        // if not crop input 
        encp->bROIEnable = 1;
    #else
        int width, height;
        width = EM_PARAM_WIDTH(encode_data->src_dim);
        if (width&0x0F)
            encp->u32CropWidth = 16 - (width&0x0F);
        else
            encp->u32CropWidth = 0;
        encode_data->frame_width = encp->roi_w = ((width + 15)>>4)<<4;
        if (encp->u32CropWidth & 0x01) {
            DEBUG_E("frame width must be multiple of 2 (%d), force to be %d\n", width, encode_data->frame_width);
            encp->u32CropWidth = 0;
        }
        if (encp->u32FrameWidth != encp->roi_w)
            encp->bROIEnable = 1;

        height = EM_PARAM_HEIGHT(encode_data->src_dim);
        if (height&0x0F)
            encp->u32CropHeight = 16 - (height&0x0F);
        else
            encp->u32CropHeight = 0;
        encode_data->frame_height = encp->roi_h = ((height+15)>>4)<<4;
        if (encp->u32CropHeight & 0x01) {
            DEBUG_E("frame height must be multiple of 2 (%d), force to be %d\n", height, encode_data->frame_height);
            encp->u32CropHeight = 0;
        }
        if (encp->u32FrameHeight != encp->roi_h)
            encp->bROIEnable = 1;

        encp->roi_x = EM_PARAM_X(encode_data->src_xy);
        encp->roi_y = EM_PARAM_Y(encode_data->src_xy);
    #endif
    }
    else {
        encode_data->frame_width = EM_PARAM_WIDTH(encode_data->src_dim);
        encode_data->frame_height = EM_PARAM_HEIGHT(encode_data->src_dim);
        encp->bROIEnable = 0;
        encp->roi_x = 0;
        encp->roi_y = 0;
    }
    encp->fincr = EM_PARAM_N(encode_data->fps_ratio);
    encp->fbase = encode_data->src_frame_rate * EM_PARAM_M(encode_data->fps_ratio);
    // Tuba 20140411: add divied by zero checker
    if (0 == encp->fincr || 0 == encp->fbase) {
        encp->fincr = 1;
        encp->fbase = 30;
    }
#if 1
    encp->u32MaxWidth = EM_PARAM_WIDTH(encode_data->src_bg_dim);
    encp->u32MaxHeight = EM_PARAM_HEIGHT(encode_data->src_bg_dim);
#else
    encp->u32MaxWidth = mp4_max_width;
    encp->u32MaxHeight = mp4_max_height;
#endif
    
    encp->u32IPInterval = encode_data->idr_period;
    encp->bShortHeader = 0;
    encp->bEnable4MV = 0;
    encp->bH263Quant = 1;
    encp->bResyncMarker = 0;
    encp->ac_disable = 0;

    /* image input format
     *   0: 2D format, CbCr interleave, named H264_2D
     *   1: 2D format, CbCr non-interleave, named MP4_2D
     *   2: 1D format, YUV420 planar, named RASTER_SCAN_420
     *   3: 1D format, YUV420 packed, named RASTER_SCAN_422
     */
    if (TYPE_YUV422_H264_2D_FRAME == encode_data->src_fmt)
        encp_w->img_fmt = 0;
    else if (TYPE_YUV422_MPEG4_2D_FRAME == encode_data->src_fmt) {
        //printk("mpeg4 2d\n");
        encp_w->img_fmt = 1;
    }
    else if (TYPE_YUV420_FRAME == encode_data->src_fmt)
        encp_w->img_fmt = 2;
    else if (TYPE_YUV422_FRAME == encode_data->src_fmt || TYPE_YUV422_FIELDS == encode_data->src_fmt)
        encp_w->img_fmt = 3;
#ifdef SUPPORT_2FRAME
    else if (encode_data->src_fmt >= TYPE_YUV422_2FRAMES_2FRAMES && encode_data->src_fmt <= TYPE_YUV422_FRAME_FRAME) {
        encp_w->img_fmt = 3;
        if (0 == encode_data->frame_buf_idx)
            encode_data->src_buf_offset = 0;
        else {
            /* not handle frame idx be bottom frame of 2 frame */
            if (0 == encode_data->src_bg_frame_size) {
                DEBUG_E("2 frame mode, src bg frame size can not be zero\n");
                damnit(MP4E_MODULE_NAME);
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
        DEBUG_E("not support this input format %d\n", encode_data->src_fmt);
        return -1;
    }
    encode_data->U_off = encp->u32FrameWidth * encp->u32FrameHeight;
    encode_data->V_off = encp->u32FrameWidth * encp->u32FrameHeight * 5 / 4;
    encode_data->stop_job = 0;

    encode_data->mbinfo_length = ((encode_data->frame_width+15)>>4) * ((encode_data->frame_height+15)>>4) * sizeof(MACROBLOCK_E);

    /* customer input: inter/intra quant
    if (mp4e_p[VGID_MP4E_customer_inter_quant]) {
        if (NULL == pv->customer_inter_quant) {
            pv->customer_inter_quant = kmalloc(sizeof(unsigned char)*QUANT_SIZE, GFP_ATOMIC);
            if (NULL == pv->customer_inter_quant) {
                err ("allocate memory for customer inter quant fail");
                return -1;
            }
        }
    }
    if (mp4e_p[VGID_MP4E_customer_intra_quant]) {
        if (NULL == pv->customer_intra_quant) {
            pv->customer_intra_quant = kmalloc(sizeof(unsigned char)*QUANT_SIZE, GFP_ATOMIC);
            if (NULL == pv->customer_intra_quant) {
                err ("allocate memory for customer intra quant fail");
                return -1;
            }
        }
    }
    */

    return 0;
}

static int fmp4e_create(struct mp4e_data_t *encode_data, FMP4_ENC_PARAM_WRP *encp_w)
{
    FMP4_ENC_PARAM_MY enc_param;
    memcpy(&enc_param.pub, encp_w, sizeof (FMP4_ENC_PARAM_WRP));
    memset(&enc_param.encp_a2, 0, sizeof(FMP4_ENC_PARAM_A2));
    enc_param.u32VirtualBaseAddr = mcp100_dev->va_base;
    enc_param.va_base_wrp = mcp100_dev->va_base_wrp;
    enc_param.u32CacheAlign= CACHE_LINE;
    enc_param.pu8ReConFrameCur=NULL;
    enc_param.pu8ReConFrameRef=NULL;
    enc_param.p16ACDC=NULL;
    enc_param.pfnDmaMalloc=fconsistent_alloc;
    enc_param.pfnDmaFree=fconsistent_free;
    enc_param.pfnMalloc = fkmalloc;
    enc_param.pfnFree = fkfree;
#ifdef MPEG4_COMMON_PRED_BUFFER // Tuba 20131108: use common pred buffer
    enc_param.mp4e_pred_virt = mp4e_pred_buf.addr_va;
    enc_param.mp4e_pred_phy = mp4e_pred_buf.addr_pa;
#endif
#if defined(REF_POOL_MANAGEMENT)&!defined(REF_BUFFER_FLOW)  // Tuba 20140409: reduce number of reference buffer
    enc_param.recon_buf = encode_data->recon_buf;
    enc_param.refer_buf = encode_data->refer_buf;
#endif                                            

    if (encode_data->handler) {
//    if (reinit)
        return FMpeg4EncReCreate(encode_data->handler , &enc_param);
    }
    else {
        /*
        if (encode_data->handler) {
            printk("[chn%d] previous one was not released", encode_data->chn);
            return -1;
        }
        */
        encode_data->handler = FMpeg4EncCreate(&enc_param);
        if(!encode_data->handler)
            return -1;
        else
            return 0;
    }

    return 0;
}

#ifdef REF_POOL_MANAGEMENT
static int release_ref_buffer(struct ref_buffer_info_t *buf, int chn)
{
    if (buf->ref_virt) {
    //#ifdef SHARED_POOL
        release_pool_buffer2(buf, chn, TYPE_MPEG4_ENC);
    //#else
    //    release_pool_buffer2(buf, chn);
    //#endif
        DEBUG_M(LOG_INFO, "{chn%d} release buffer 0x%x\n", chn, buf->ref_phy);
    }
    memset(buf, 0, sizeof(struct ref_buffer_info_t));
    return 0;
}

static int release_all_ref_buffer(struct mp4e_data_t *encode_data)
{
    release_ref_buffer(encode_data->refer_buf, encode_data->chn);
    release_ref_buffer(encode_data->recon_buf, encode_data->chn);
    DEBUG_M(LOG_DEBUG, "{chn%d} release all reference buffer\n", encode_data->chn);
    return 0;
}

int register_mem_pool(struct mp4e_data_t *encode_data)
{
    release_all_ref_buffer(encode_data);
    deregister_ref_pool(encode_data->chn, TYPE_MPEG4_ENC);
    encode_data->res_pool_type = register_ref_pool(encode_data->chn, EM_PARAM_WIDTH(encode_data->src_bg_dim), 
        EM_PARAM_HEIGHT(encode_data->src_bg_dim), TYPE_MPEG4_ENC);
    if (encode_data->res_pool_type < 0)
        return -1;
    return 0;
}
#endif

int fmp4e_rc_init_param(struct mp4e_data_t *encode_data)
{
    struct rc_init_param_t rc_param;
    
    rc_param.chn = encode_data->chn;
    rc_param.rc_mode = encode_data->rc_mode;
    if (0 == encode_data->fps_ratio) {
        rc_param.fincr = 1;
        rc_param.fbase = encode_data->src_frame_rate;
    }
    else {
        rc_param.fincr = EM_PARAM_N(encode_data->fps_ratio);
        rc_param.fbase = encode_data->src_frame_rate * EM_PARAM_M(encode_data->fps_ratio);
    }
    rc_param.bitrate = encode_data->bitrate * 1000;
    rc_param.max_bitrate = encode_data->max_bitrate * 1000;
    rc_param.qp_constant = encode_data->init_quant;
    rc_param.max_quant = encode_data->max_quant;
    rc_param.min_quant = encode_data->min_quant;

    if (mp4e_rc_dev) {
        return mp4e_rc_dev->rc_init(&encode_data->rc_handler, &rc_param);
    }

    return 0;
}

static int fmp4e_rc_update(struct mp4e_data_t *encode_data)
{
    struct rc_frame_info_t rc_data;
    if (mp4e_rc_dev && encode_data->rc_handler) {
        rc_data.slice_type = encode_data->slice_type;
        rc_data.frame_size = encode_data->bitstream_size;
        rc_data.avg_qp = encode_data->quant;
        mp4e_rc_dev->rc_update(encode_data->rc_handler, &rc_data);
    }
    return 0;
}

// Tuba 20140604: add force intra
static int parsing_preprocess_property(struct video_property_t *property)
{
    int i;
    int force_intra = 0;
    for (i = 0; i < MAX_PROCESS_PROPERTYS; i++) {
        if (ID_NULL == property[i].id)
            break;
        if (ID_FORCE_INTRA == property[i].id) {
            if (1 == property[i].value)
                force_intra = 1;
            break;
        }
    }
    return force_intra;
}

int fmp4e_encode_trigger(struct job_item_t *job_item)
{
	int engine, chn;
    struct video_entity_job_t *job;
	struct mp4e_data_t *encode_data;
	int ret = 0;
    FMP4_ENC_FRAME encf;
    struct video_property_t property[MAX_PROCESS_PROPERTYS];
#ifdef REF_POOL_MANAGEMENT
    unsigned long flags;
#endif

    job = (struct video_entity_job_t *)job_item->job;
    chn = job_item->chn;
    engine = job_item->engine;
    if (engine >= MAX_ENGINE_NUM) {
        DEBUG_E("engine %d is over MAX", engine);
        return -1;
    }
	if (chn >= MP4E_MAX_CHANNEL) {
		DEBUG_E("chn (%d) over MAX", chn);
		return -1;					// JOB_STATUS_FAIL
	}
    encode_data = &mp4e_private_data[engine][chn];

    mp4e_parse_in_property(encode_data, job);
    if (EM_PARAM_WIDTH(encode_data->src_bg_dim) > mp4_max_width || 
        EM_PARAM_HEIGHT(encode_data->src_bg_dim) > mp4_max_height) {
        DEBUG_E("{chn%d} resolution(%d x %d) is over maximal resolution(%d x %d)\n", encode_data->chn, 
            EM_PARAM_WIDTH(encode_data->src_bg_dim), EM_PARAM_HEIGHT(encode_data->src_bg_dim), mp4_max_width, mp4_max_height);
        damnit(MP4E_MODULE_NAME);
        return -1;
    }
    if (encode_data->updated) {
        FMP4_ENC_PARAM_WRP encp;
        DEBUG_M(LOG_INFO, "job %d: update = %x\n", job->id, encode_data->updated);
        if (encode_data->updated == MP4E_REINIT) {
        #ifdef REF_POOL_MANAGEMENT
            spin_lock_irqsave(&mpeg4_enc_lcok, flags);
            ret = register_mem_pool(encode_data);
            spin_unlock_irqrestore(&mpeg4_enc_lcok, flags);
            if (ret < 0)
                return ret;
            // allocate 2 buffer
            #ifndef REF_BUFFER_FLOW // Tuba 20140409: reduce number of reference buffer
                if (allocate_pool_buffer2(encode_data->recon_buf, encode_data->res_pool_type, encode_data->chn, TYPE_MPEG4_ENC) < 0) {
                    DEBUG_M(LOG_ERROR, "{chn%d} allocate refereence buffer fail\n", encode_data->chn);
                    return -1;
                }
                if (allocate_pool_buffer2(encode_data->refer_buf, encode_data->res_pool_type, encode_data->chn, TYPE_MPEG4_ENC) < 0) {
                    DEBUG_M(LOG_ERROR, "{chn%d} allocate refereence buffer fail\n", encode_data->chn);
                    return -1;
                }
            #endif
        #endif
            if (fmp4e_param_vg_init(encode_data, &encp) < 0) {
                DEBUG_E("mpeg4 encoder init fail\n");
                return -1;
            }
            if (fmp4e_create(encode_data, &encp) < 0)
                return -1;
            if (fmp4e_rc_init_param(encode_data) < 0) {
                DEBUG_E("{chn%d} rc init fail\n", job_item->chn);
                encode_data->rc_handler = NULL;
            }
        }
        else {
            if (encode_data->updated | MP4E_RC_REINIT) {
                if (fmp4e_rc_init_param(encode_data) < 0) {
                    DEBUG_E("{chn%d} rc init fail\n", job_item->chn);
                    encode_data->rc_handler = NULL;
                }
            }
            if (encode_data->updated | MP4E_GOP_REINIT) {   // Tuba 20140603: add gop reset function
                FMP4_ENC_PARAM gopParam;
                release_all_ref_buffer(encode_data);
                // reset gop
                gopParam.u32IPInterval = encode_data->idr_period;
                FMpeg4Enc_IFP(encode_data->handler, &gopParam);
            }
        }
        encode_data->updated = 0;
    }

	if (!encode_data->handler) {
		DEBUG_E("fatal error: put job but never init\n");
		return -1;					// JOB_STATUS_FAIL
	}
    if (0 == job->in_buf.buf[0].addr_va) {
        DEBUG_E("job in buffer is NULL\n");
        damnit(MP4E_MODULE_NAME);
        return -1;
    }
    /*
	if (encode_data->stop_job)
		return -1;					// FAIL
	*/
	if (mp4e_preprocess (job_item, property) < 0) {
		DEBUG_M(LOG_WARNING, "video_preprocess return fail");
		return -1;					// FAIL
	}
    // Tuba 20140603: parsing property
    if (parsing_preprocess_property(property) > 0)
        encf.intra = 1;
    else 
        encf.intra = -1;

    //if ((0 == job->in_buf.buf[0].addr_pa) || (0 == job->out_buf.buf[0].addr_pa)) {
    if (0 == job->out_buf.buf[0].addr_va) {
		DEBUG_M(LOG_WARNING, "job in pointer err\n");	
		return -1;
	}

    encode_data->in_addr_va = job->in_buf.buf[0].addr_va;
    encode_data->in_addr_pa = job->in_buf.buf[0].addr_pa;
    encode_data->in_size = job->in_buf.buf[0].size;
    encode_data->out_addr_va = job->out_buf.buf[0].addr_va;
    encode_data->out_addr_pa = job->out_buf.buf[0].addr_pa;
    encode_data->out_size = job->out_buf.buf[0].size;
#ifdef ENABLE_CHECKSUM
    job_item->check_sum_type = 0;
#endif
#ifdef REF_BUFFER_FLOW  // Tuba 20140409: reduce number of reference buffer
    if (encode_data->res_pool_type < 0) {
        DEBUG_E("buffer register initialization fail\n");
        return -1;
    }
    if (allocate_pool_buffer2(encode_data->recon_buf, encode_data->res_pool_type, encode_data->chn, TYPE_MPEG4_ENC) < 0) {
        DEBUG_M(LOG_ERROR, "{chn%d} allocate refereence buffer fail\n", encode_data->chn);
        return -1;
    }
    encf.pReconBuffer = encode_data->recon_buf;
#endif
#if 0
    encode_data->quant = encode_data->init_quant;
#else
    if (mp4e_rc_dev && encode_data->rc_handler)
        encode_data->quant = mp4e_rc_dev->rc_get_quant(encode_data->rc_handler, NULL);
    else
        encode_data->quant = encode_data->init_quant;
#endif
	encf.quant = encode_data->quant;    // get from rate control
//printk("input quant = %d\n", encf.quant);
	encf.quant_intra_matrix = NULL;
	encf.quant_inter_matrix = NULL;
    // force intra
	//encf.intra = -1;
	encf.module_time_base = 0;
#ifdef SUPPORT_2FRAME
    encf.pu8YFrameBaseAddr = (unsigned char *)(job->in_buf.buf[0].addr_pa + encode_data->src_buf_offset);
#else
	encf.pu8YFrameBaseAddr = (unsigned char *)(job->in_buf.buf[0].addr_pa);
#endif
	encf.pu8UFrameBaseAddr = encf.pu8YFrameBaseAddr + encode_data->U_off;
	encf.pu8VFrameBaseAddr = encf.pu8YFrameBaseAddr + encode_data->V_off;
    encf.roi_x = EM_PARAM_X(encode_data->src_xy);
    encf.roi_y = EM_PARAM_Y(encode_data->src_xy);

    if (encode_data->motion_enable)
        encf.bitstream = (unsigned char *)(job->out_buf.buf[0].addr_pa + encode_data->mbinfo_length);
    else 
        encf.bitstream = (unsigned char *)(job->out_buf.buf[0].addr_pa + BS_OFFSET);

	//mp4e_mark_engine_start(job_item);
	ret = FMpeg4EE_Tri(encode_data->handler, &encf);
    mp4e_mark_engine_start();
	//printk ("FMpeg4EE_Tri ret %d\n", ret);
	encode_data->quant = encf.quant;
	encode_data->slice_type = (encf.intra ? EM_SLICE_TYPE_I : EM_SLICE_TYPE_P);

	return ret;
}

static int fmp4e_process_done (struct job_item_t *job_item)
{
	int bs_len;
    struct mp4e_data_t *encode_data = &mp4e_private_data[job_item->engine][job_item->chn];
	//struct video_entity_t *entity = job->entity;
	//struct mp4e_vg_private * pv = (struct mp4e_vg_private *)entity->private;
	//struct v_graph_info * outb_hdr = (struct  v_graph_info *)job->out_header->addr_va;
	//mp4e_OutInfo *enc_out_info = (mp4e_OutInfo *)(job->out_header->addr_va +outb_hdr->drvinfo_offset);

    mp4e_mark_engine_finish();
	bs_len = FMpeg4EE_End(encode_data->handler);
	encode_data->bitstream_size = bs_len;
    if (bs_len < 0) {
        encode_data->bitstream_size = 0;
        mp4e_set_out_property(encode_data, (struct video_entity_job_t *)job_item->job);
    	if (mp4e_postprocess(job_item, (void *)NULL) < 0) {
    		DEBUG_E ("video_postprocess return fail\n");
    		return -1;
    	}
        return -1;
    }
#ifdef HANDLE_BS_CACHEABLE  // Tuba 20140225: handle bitstream buffer cacheable
    job_item->bs_size = encode_data->bitstream_size;
#endif
    // not fill HANDLE_LOSS_PERFORMANCE
    if (encode_data->motion_enable) {
        MACROBLOCK_INFO *mbinfo;
        encode_data->bitstream_offset = encode_data->mbinfo_length;
        if (encode_data->bitstream_offset + encode_data->bitstream_size > encode_data->out_size) {
            DEBUG_E("MPEG4 Encoder Alert! Ch:%d: mv_length:0x%x, bs_length:0x%x over buf size:0x%x\n", 
                  encode_data->chn, encode_data->bitstream_offset, encode_data->bitstream_size, encode_data->out_size);
            damnit(MP4E_MODULE_NAME);
            // set out property
            // postprocess
            //return -1;
        }
        mbinfo = FMpeg4EncGetMBInfo(encode_data->handler);
        memcpy((void *)encode_data->out_addr_va, (void *)mbinfo, encode_data->mbinfo_length);
        dma_map_single(NULL, (void *)encode_data->out_addr_va, encode_data->mbinfo_length, DMA_TO_DEVICE);
    }
    else {
        encode_data->bitstream_offset = BS_OFFSET;
        if (encode_data->bitstream_offset + encode_data->bitstream_size > encode_data->out_size) {
            DEBUG_E("MPEG4 Encoder Alert! Ch:%d: mv_length:0x%x, bs_length:0x%x over buf size:0x%x\n", 
                  encode_data->chn, encode_data->bitstream_offset, encode_data->bitstream_size, encode_data->out_size);
            damnit(MP4E_MODULE_NAME);
            // set out property
            // postprocess
            //return -1;
        }
    }

    fmp4e_rc_update(encode_data);

    //mp4e_mark_engine_finish(job_item);
    mp4e_set_out_property(encode_data, (struct video_entity_job_t *)job_item->job);

	if (mp4e_postprocess(job_item, (void *)NULL) < 0) {
		DEBUG_E ("video_postprocess return fail\n");
		return -1;
	}
#ifdef REF_BUFFER_FLOW  // Tuba 20140409: reduce number of reference buffer
    // swap refer & recon
{
    struct ref_buffer_info_t *tmp_buf;
    release_ref_buffer(encode_data->refer_buf, encode_data->chn);
    tmp_buf = encode_data->refer_buf;
    encode_data->refer_buf = encode_data->recon_buf;
    encode_data->recon_buf = tmp_buf;
}
#endif    

    if (encode_data->stop_job) {
    #ifdef REF_POOL_MANAGEMENT
        release_all_ref_buffer(encode_data);
        //#ifdef SHARED_POOL
            deregister_ref_pool(encode_data->chn, TYPE_MPEG4_ENC);
        //#else
        //    deregister_ref_pool(encode_data->chn);
        //#endif
        encode_data->res_pool_type = -1;
    #endif
        encode_data->stop_job = 0;
    }
    else {
        /*
    #ifdef REF_POOL_MANAGEMENT
        swap_ref_buffer(encode_data);
    #endif
        */
    }
    /*
	mcp100_proc_dump (1,
		entity->minor & 0xFF,
		TYPE_MPEG4_ENCODER,
		(unsigned char *)job->out_buf->addr_va,
		bs_len, job->swap, 1);
    */
	return 0;
}

static void fmp4e_isr(int irq, void *dev_id, unsigned int base)
{
	MP4_COMMAND type;
	volatile Share_Node *node = (volatile Share_Node *) (0x8000 + base);		 

	type = (node->command & 0xF0)>>4;
    DEBUG_M(LOG_INFO, "mp4 isr: type %d\n", type);
	if(( type == ENCODE_IFRAME) || (type == ENCODE_PFRAME) || (type == ENCODE_NFRAME) ) {
		#ifdef EVALUATION_PERFORMANCE
			encframe.hw_stop = get_counter();
		#endif
		#ifndef GM8120
			*(volatile unsigned int *)(base + 0x20098)=0x80000000;
		#endif
	}
}

static void fmp4e_release(struct mp4e_data_t *encode_data)
{
	FMpeg4EncDestroy(encode_data->handler );
    /*
	if(pv->customer_inter_quant)
       	fkfree(pv->customer_inter_quant);
	if(pv->customer_intra_quant)
		fkfree(pv->customer_intra_quant);
	pv->handler = NULL;
	pv->customer_inter_quant = NULL;
	pv->customer_intra_quant = NULL;
	*/
}

static int mp4e_putjob(void *parm)
{
    //mcp_putjob((struct video_entity_job_t*)parm, TYPE_MPEG4_ENCODER, 0, 0);
    mcp_putjob((struct video_entity_job_t*)parm, TYPE_MPEG4_ENCODER);
    return JOB_STATUS_ONGOING;
}


static int mp4e_stop(void *parm, int engine, int minor)
{
    struct mp4e_data_t *encode_data;
    int is_ongoing_job = 0;

    //if (minor >= MP4E_MAX_CHANNEL || engine >= MAX_ENGINE_NUM)
    //    printk("chn %d is over max channel %d or engine %d is over max engine %d\n", 
    //        minor, MP4E_MAX_CHANNEL, engine, MAX_ENGINE_NUM);
    if (minor >= MP4E_MAX_CHANNEL) {
        DEBUG_E("chn %d is over max channel %d\n", minor, MP4E_MAX_CHANNEL);
    }
    else {
        is_ongoing_job = mcp_stopjob(TYPE_MPEG4_ENCODER, engine, minor);
    }

    encode_data = &mp4e_private_data[engine][minor];
    if (!is_ongoing_job) {
    #ifdef REF_POOL_MANAGEMENT
        release_all_ref_buffer(encode_data);
        deregister_ref_pool(encode_data->chn, TYPE_MPEG4_ENC);
        encode_data->res_pool_type = -1;
    #endif
        DEBUG_M(LOG_DEBUG, "{chn%d} stop but job on going\n", minor);
    }
    else
        encode_data->stop_job = 1;
    //encode_data->stop_job = 1;
    encode_data->updated = MP4E_REINIT;

    return 0;
}

static struct mp4e_property_map_t *mp4e_get_propertymap(int id)
{
    int i;
    
    for (i = 0; i < sizeof(mp4e_property_map)/sizeof(struct mp4e_property_map_t); i++) {
        if (mp4e_property_map[i].id == id) {
            return &mp4e_property_map[i];
        }
    }
    return 0;
}


static int mp4e_queryid(void *parm, char *str)
{
    int i;
    
    for (i = 0; i < sizeof(mp4e_property_map)/sizeof(struct mp4e_property_map_t); i++) {
        if (strcmp(mp4e_property_map[i].name,str) == 0) {
            return mp4e_property_map[i].id;
        }
    }
    printk("mp4e_queryid: Error to find name %s\n", str);
    return -1;
}


static int mp4e_querystr(void *parm, int id, char *string)
{
    int i;
    for (i = 0; i < sizeof(mp4e_property_map)/sizeof(struct mp4e_property_map_t); i++) {
        if (mp4e_property_map[i].id == id) {
            memcpy(string, mp4e_property_map[i].name, sizeof(mp4e_property_map[i].name));
            return 0;
        }
    }
    printk("mp4e_querystr: Error to find id %d\n", id);
    return -1;
}

static int mp4e_getproperty(void *parm, int engine, int minor, char *string)
{
    int id,value = -1;
    struct mp4e_data_t *encode_data;
    //struct driver_data_t *driver_data;

    if (engine >= MAX_ENGINE_NUM) {
        printk("Error over engine number %d\n", MAX_ENGINE_NUM);
        return -1;
    }
    if (minor >= MP4E_MAX_CHANNEL) {
        printk("Error over minor number %d\n", MP4E_MAX_CHANNEL);
        return -1;
    }

    encode_data = &mp4e_private_data[engine][minor];
    encode_data->src_xy = 0;    // is not exist, set to be (0, 0)
    id = mp4e_queryid(parm, string);
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
        case ID_IDR_INTERVAL:
            value = encode_data->idr_period;
            break;
        case ID_SRC_INTERLACE:
            value = encode_data->src_interlace;
            break;
        case ID_DI_RESULT:
            value = encode_data->di_result;
            break;
        case ID_RC_MODE:
            value = encode_data->rc_mode;
            break;
        case ID_SRC_FRAME_RATE:
            value = encode_data->src_frame_rate;
            break;
        case ID_FPS_RATIO:
            value = encode_data->fps_ratio;
            break;
        case ID_MAX_QUANT:
            value = encode_data->max_quant;
            break;
        case ID_MIN_QUANT:
            value = encode_data->min_quant;
            break;
        case ID_BITRATE:
            value = encode_data->bitrate;
            break;
        case ID_MAX_BITRATE:
            value = encode_data->max_bitrate;
            break;
        case ID_MOTION_ENABLE:
            value = encode_data->motion_enable;
            break;
        case ID_QUANT:
            value = encode_data->quant;
            break;
        case ID_BITSTREAM_OFFSET:
            value = encode_data->bitstream_size;
            break;
        case ID_SLICE_TYPE:
            value = encode_data->slice_type;
            break;
        case ID_BITSTREAM_SIZE:
            value = encode_data->bitstream_offset;
            break;
        default:
            break;
    }
    return value;
}


int mp4e_dump_property_value(char *str, int chn)
{
    // dump property
    int i = 0, len = 0;
    struct mp4e_property_map_t *map;
    unsigned int id, value;
    unsigned long flags;
    int engine = 0;
    int idx = MAKE_IDX(MP4E_MAX_CHANNEL, engine, chn);
    
    spin_lock_irqsave(&mpeg4_enc_lcok, flags);
    
    len += sprintf(str+len, "MPEG4 encoder property channel %d\n", chn);
    len += sprintf(str+len, "=============================================================\n");
    len += sprintf(str+len, "ID    Name(string)  Value(hex)  Readme\n");
    do {
        id = mp4e_property_record[idx].property[i].id;
        if (id == ID_NULL)
            break;
        value = mp4e_property_record[idx].property[i].value;
        map = mp4e_get_propertymap(id);
        if (map) {
            len += sprintf(str+len, "%02d  %14s   %08x  %s\n", id, map->name, value, map->readme);
        }
        i++;
    } while (1);
    len += sprintf(str+len, "=============================================================\n");
    spin_unlock_irqrestore(&mpeg4_enc_lcok, flags);
    return len;
}

int mp4e_rc_register(struct rc_entity_t *entity)
{
    mp4e_rc_dev = entity;
    return 0;
}
int mp4e_rc_deregister(void)
{
    mp4e_rc_dev = NULL;
    return 0;
}

struct video_entity_ops_t mp4e_driver_ops ={
    putjob:      &mp4e_putjob,
    stop:        &mp4e_stop,
    queryid:     &mp4e_queryid,
    querystr:    &mp4e_querystr,
    getproperty: &mp4e_getproperty,
    //register_clock: &mp4e_register_clock,
};    


struct video_entity_t mp4e_entity={
    type:       TYPE_MPEG4_ENC,
    //type:       TYPE_JPEG_ENC,
    name:       "mp4e",
    engines:    MAX_ENGINE_NUM,
    minors:     MP4E_MAX_CHANNEL,
    ops:        &mp4e_driver_ops
};


static mcp100_rdev mp4edv = {
	.job_tri = fmp4e_encode_trigger,
	.job_done = fmp4e_process_done,
	.codec_type = FMPEG4_ENCODER_MINOR,
	.handler = fmp4e_isr,
	.dev_id = NULL,
	.switch22 = switch2mp4e,
	.parm_info = fmp4e_parm_info,
	.parm_info_frame = fmp4e_parm_info_f,
};

#define REGISTER_VG     0x01
#define REGISTER_MCP100 0x02
#define INIT_PROC       0x04
int mp4e_drv_init(void)
{
    int i, chn;
    int init = 0;

    video_entity_register(&mp4e_entity);
    init |= REGISTER_VG;

    spin_lock_init(&mpeg4_enc_lcok);

    mp4e_property_record = kzalloc(sizeof(struct mp4e_property_record_t) * MAX_ENGINE_NUM * MP4E_MAX_CHANNEL, GFP_KERNEL);
    if (mp4e_proc_init() < 0)
        goto mp4e_init_proc_fail;
    init |= INIT_PROC;

    //memset(mp4e_engine_time, 0, sizeof(mp4e_engine_time));
    //memset(mp4e_engine_start, 0, sizeof(mp4e_engine_start));
    //memset(mp4e_engine_end, 0, sizeof(mp4e_engine_end));
    //memset(mp4e_utilization_start, 0, sizeof(mp4e_utilization_start));
    //memset(mp4e_utilization_record, 0, sizeof(mp4e_utilization_record));

    memset(mp4e_private_data, 0, DATA_SIZE*MAX_ENGINE_NUM*MP4E_MAX_CHANNEL);
    for (i = 0; i < MAX_ENGINE_NUM; i++) {
        for (chn = 0; chn < MP4E_MAX_CHANNEL; chn++) {
            mp4e_private_data[i][chn].engine = i;
            mp4e_private_data[i][chn].chn = chn;
        #ifdef REF_POOL_MANAGEMENT
            mp4e_private_data[i][chn].res_pool_type = -1;
            mp4e_private_data[i][chn].recon_buf = &mp4e_private_data[i][chn].ref_buffer[0];
            mp4e_private_data[i][chn].refer_buf = &mp4e_private_data[i][chn].ref_buffer[1];
        #endif
        }
    }
    if (mcp100_register(&mp4edv, TYPE_MPEG4_ENCODER, MP4E_MAX_CHANNEL) < 0)
        goto mp4e_init_proc_fail;
    init |= REGISTER_MCP100;

    return 0;

mp4e_init_proc_fail:
    if (INIT_PROC | init)
        mp4e_proc_exit();
    if (REGISTER_VG | init)
        video_entity_deregister(&mp4e_entity);
    if (REGISTER_MCP100 | init)
        mcp100_deregister(TYPE_MPEG4_ENCODER);
    if (mp4e_property_record)
        kfree(mp4e_property_record);
    return -1;
}


void mp4e_drv_close(void)
{
    int i, j;
    struct mp4e_data_t *encode_data;

    video_entity_deregister(&mp4e_entity);
    if (mp4e_property_record)
        kfree(mp4e_property_record);
    mp4e_proc_exit();
    mcp100_deregister(TYPE_MPEG4_ENCODER);

    for (j = 0; j < MAX_ENGINE_NUM; j++) {
        for (i = 0; i < MP4E_MAX_CHANNEL; i++) {
            encode_data = &mp4e_private_data[j][i];
    		if (encode_data->handler != NULL) {
    			//info ("chn %d not released, automatically close", i);
    			release_all_ref_buffer(encode_data);
                deregister_ref_pool(encode_data->chn, TYPE_MPEG4_ENC);
    			fmp4e_release(encode_data);
                if (mp4e_rc_dev && encode_data->rc_handler)
                    mp4e_rc_dev->rc_clear(encode_data->rc_handler); 
    		}
        }
	}
}

