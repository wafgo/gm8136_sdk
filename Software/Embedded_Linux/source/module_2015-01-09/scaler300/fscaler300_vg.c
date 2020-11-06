#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/list.h>
#include <linux/version.h>
#include <linux/seq_file.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <cpu_comm.h>
#include <mach/fmem.h>
#include "frammap_if.h"
#include "fscaler300_vg.h"
#include "fscaler300_dbg.h"
#include "util.h"
#if GM8210
#include "scl_dma/scl_dma.c"
#endif
#include <mach/ftpmu010.h>
#include <mach/fmem.h>

#include <log.h>    //include log system "printm","damnit"...
#include <video_entity.h>   //include video entity manager
#include <mach/gm_jiffies.h>
#define MODULE_NAME         "SL"    //<<<<<<<<<< Modify your module name (two bytes) >>>>>>>>>>>>>>

#define I_AM_RC(x)    ((x) & (0x1 << 4))
#define I_AM_EP(x)    ((x) & (0x1 << 5))
#define I_AM_INVISIBLE(x)    ((x) & (0x1 << 6))
#define WIN2_SUPPORT_XY

global_info_t global_info;
vg_proc_t     vg_proc_info;

#ifdef USE_WQ
struct workqueue_struct *callback_workq;
static DECLARE_DELAYED_WORK(process_callback_work, 0);
static DECLARE_DELAYED_WORK(process_2ddma_callback_work, 0);
static DECLARE_DELAYED_WORK(process_ep_work, 0);
#endif

wait_queue_head_t cb_2ddma_thread_wait;
wait_queue_head_t cb_thread_wait;
wait_queue_head_t eptx_thread_wait;
int cb_2ddma_wakeup_event = 0;
int cb_wakeup_event = 0;
int eptx_wakeup_event = 0;

static void fscaler300_callback_process(void);
static void fscaler300_2ddma_callback_process(void);

#if GM8210
#ifdef TEST_WQ
struct workqueue_struct *test_workq;
static DECLARE_DELAYED_WORK(process_test_work, 0);
static void fscaler300_test_process(void);
#endif
#ifdef USE_KTHREAD
static void callback_wake_up(void);
static void callback_2ddma_wake_up(void);
#endif
static void fscaler300_ep_process(void);
static int fscaler300_ep_rx_process(void *);
static int fscaler300_rc_rx_process(void *);
void fscaler300_set_node_timeout(job_node_t *node);
int fscaler300_rc_io_tx(job_node_t *node);
int fscaler300_rc_io_rx(snd_info_t *rcv_info, int timeout);
int fscaler300_ep_io_tx(job_node_t *node);
int fscaler300_ep_io_rx(int timeout);
#endif
int virtual_chn_num;
extern int max_minors;
extern int max_vch_num;
extern int temp_width;
extern int temp_height;
extern int pip1_width;
extern int pip1_height;
extern int timeout;
extern int damnitoff;

/* utilization */
static unsigned int utilization_period = 5; //5sec calculation
perf_info_t		vg_perf;
int debug_mode = 0;
int dual_debug = 0;
int rc_fail = 0;
int ep_rx_timeout = 70;
int rc_ep_diff_cnt = 3;
int rc_drop_cnt = 0;
int ep_drop_cnt = 0;
int rxtx_max = 0;
int rc_tx_threshold = 9;
int flow_check = 0;

static unsigned int utilization_start[SCALER300_ENGINE_NUM], utilization_record[SCALER300_ENGINE_NUM];
static unsigned int engine_start[SCALER300_ENGINE_NUM], engine_end[SCALER300_ENGINE_NUM];
static unsigned int engine_time[SCALER300_ENGINE_NUM];
static int frame_cnt_start[SCALER300_ENGINE_NUM], frame_cnt_end[SCALER300_ENGINE_NUM];
static int frame_cnt[SCALER300_ENGINE_NUM];
static int dummy_cnt_start[SCALER300_ENGINE_NUM];
static int dummy_cnt[SCALER300_ENGINE_NUM];
// add TYPE_RGB555 define, if TYPE_RGB555 define at em/common.h, remove it.
#ifndef TYPE_RGB555
#define TYPE_RGB555 0x503
#endif

static int ep_rx_vg_serial=0;
static int ep_rx_timeout_minor_nr = -1;
#define EP_NOT_ON_GOING_TIMEOUT     30
#define EP_ON_GOING_TIMEOUT         60

/* proc system */
unsigned int callback_period = 0;    //ticks
unsigned int ep_rx_period = 10;      //msec
unsigned int ratio = 0;
unsigned int ep_pcie_addr_timeout_cnt = 0;
unsigned int ep_job_timeout_cnt = 0;
static int rc_node_mem_cnt = 0;
static int rc_tx_node_cnt = 0;

static unsigned int ex_rx_timeout = EP_NOT_ON_GOING_TIMEOUT;

#define EP_NEED_TX_DONE     1

#define EP_MATCH_OK                     0
#define EP_MATCH_EP_LIST_EMPTY         -1
#define EP_MATCH_RC_RCNODE_LIST_EMPTY  -2
#define EP_MATCH_FAILED                -3

#define RC_MATCH_STATUS_DONE         0
#define RC_MATCH_STATUS_FAIL        -1
#define RC_MATCH_LIST_EMPTY         -2
#define RC_MATCH_EP_SN_SMALLER      -3
#define RC_MATCH_EP_SN_BIGGER       -4
#define RC_MATCH_EP_VG_SN_SMALLER   -5
#define RC_MATCH_EP_VG_SN_BIGGER    -6

/* property lastest record */
struct property_record_t *property_record;

#define LOG_ERROR       0
#define LOG_WARNING     1

/* log & debug message */
#define MAX_FILTER  5
static unsigned int log_level = LOG_ERROR;
static int include_filter_idx[MAX_FILTER]; //init to -1, ENGINE<<16|MINOR
static int exclude_filter_idx[MAX_FILTER]; //init to -1, ENGINE<<16|MINOR

int BUF_CLEAN_COLOR = OSD_PALETTE_COLOR_BLACK;

enum property_id {
    ID_START = (MAX_WELL_KNOWN_ID_NUM + 1),
    ID_PIP0_FROM_XY,
    ID_PIP0_FROM_DIM,
    ID_PIP0_TO_XY,
    ID_PIP0_TO_DIM,
    ID_PIP0_SCL_INFO,

    ID_PIP1_FROM_XY,
    ID_PIP1_FROM_DIM,
    ID_PIP1_TO_XY,
    ID_PIP1_TO_DIM,
    ID_PIP1_SCL_INFO,

    ID_PIP2_FROM_XY,
    ID_PIP2_FROM_DIM,
    ID_PIP2_TO_XY,
    ID_PIP2_TO_DIM,
    ID_PIP2_SCL_INFO,

    ID_PIP3_FROM_XY,
    ID_PIP3_FROM_DIM,
    ID_PIP3_TO_XY,
    ID_PIP3_TO_DIM,
    ID_PIP3_SCL_INFO,

    ID_PIP4_FROM_XY,
    ID_PIP4_FROM_DIM,
    ID_PIP4_TO_XY,
    ID_PIP4_TO_DIM,
    ID_PIP4_SCL_INFO,

    ID_PIP5_FROM_XY,
    ID_PIP5_FROM_DIM,
    ID_PIP5_TO_XY,
    ID_PIP5_TO_DIM,
    ID_PIP5_SCL_INFO,

    ID_PIP6_FROM_XY,
    ID_PIP6_FROM_DIM,
    ID_PIP6_TO_XY,
    ID_PIP6_TO_DIM,
    ID_PIP6_SCL_INFO,

    ID_PIP7_FROM_XY,
    ID_PIP7_FROM_DIM,
    ID_PIP7_TO_XY,
    ID_PIP7_TO_DIM,
    ID_PIP7_SCL_INFO,

    ID_PIP8_FROM_XY,
    ID_PIP8_FROM_DIM,
    ID_PIP8_TO_XY,
    ID_PIP8_TO_DIM,
    ID_PIP8_SCL_INFO,

    ID_PIP9_FROM_XY,
    ID_PIP9_FROM_DIM,
    ID_PIP9_TO_XY,
    ID_PIP9_TO_DIM,
    ID_PIP9_SCL_INFO,

    ID_MAX
};

struct property_map_t property_map[] = {
    {ID_SRC_FMT,            "src_fmt",          "source format"},
    {ID_SRC_XY,             "src_xy",           "source x and y position"},
    {ID_SRC_BG_DIM,         "src_bg_dim",       "source background width/height"},
    {ID_SRC_DIM,            "src_dim",          "source width/height"},
    {ID_SRC_INTERLACE,      "src_interlace",    "0: progressive, 1: interlace"},
    {ID_DST_FMT,            "dst_fmt",          "destination format"},
    {ID_DST_XY,             "dst_xy",           "destination x and y position"},
    {ID_DST_BG_DIM,         "dst_bg_dim",       "destination background width/height"},
    {ID_DST_DIM,            "dst_dim",          "destination width/height"},
    {ID_DI_RESULT,          "di_result",        "from 3di, 0:do nothing  1:deinterlace  2:copy line, 3:denoise only"},
    {ID_SUB_YUV,            "sub_yuv",          "0: no ratio frame, 1: has ratio frame"},
    {ID_SCL_FEATURE,        "scl_feature",      "0: do nothing, 1: scaling, 2:line correction"},
    {ID_SCL_RESULT,         "scl_result",       "0: do nothing, 1: source from top, 2: source from bottom"},
    {ID_AUTO_ASPECT_RATIO,  "auto aspect ratio","0: donothing, 1: keep ratio"},
    {ID_AUTO_BORDER,        "border enable",    "0: donothing, 1: enable border"},
    {ID_DST2_BG_DIM,        "dst2_bg_dim",      "CVBS frame's destination background width/height"},
    {ID_SRC2_BG_DIM,        "src2_bg_dim",      "CVBS frame's source background width/height"},
    {ID_DST2_XY,            "dst2_xy",          "CVBS frame's destination x, y"},
    {ID_SRC2_XY,            "src2_xy",          "CVBS frame's source x, y"},
    {ID_DST2_DIM,           "dst2_dim",         "CVBS frame's destination width/height"},
    {ID_SRC2_DIM,           "src2_dim",         "CVBS frame's source width/height"},
    {ID_WIN2_BG_DIM,        "win2_bg_dim",      "CVBS frame's zoom width/height"},
    {ID_WIN2_XY,            "win2_xy",          "CVBS frame's zoom x, y"},

    {ID_PIP0_FROM_XY,       "pip0_from_xy",     "pip0 source xy"},
    {ID_PIP0_FROM_DIM,      "pip0_from_dim",    "pip0 source width/height"},
    {ID_PIP0_TO_XY,         "pip0_to_xy",       "pip0 destination xy"},
    {ID_PIP0_TO_DIM,        "pip0_to_dim",      "pip0 destination width/height"},
    {ID_PIP0_SCL_INFO,      "pip0_scl_info",    "pip0 scl info, bit[7:0]=from frmX, bit[15:8]=to frmX, bit[23:16]=scl layout"},

    {ID_PIP1_FROM_XY,       "pip1_from_xy",     "pip1 source xy"},
    {ID_PIP1_FROM_DIM,      "pip1_from_dim",    "pip1 source width/height"},
    {ID_PIP1_TO_XY,         "pip1_to_xy",       "pip1 destination xy"},
    {ID_PIP1_TO_DIM,        "pip1_to_dim",      "pip1 destination width/height"},
    {ID_PIP1_SCL_INFO,      "pip1_scl_info",    "pip1 scl info, bit[7:0]=from frmX, bit[15:8]=to frmX, bit[23:16]=scl layout"},

    {ID_PIP2_FROM_XY,       "pip2_from_xy",     "pip2 source xy"},
    {ID_PIP2_FROM_DIM,      "pip2_from_dim",    "pip2 source width/height"},
    {ID_PIP2_TO_XY,         "pip2_to_xy",       "pip2 destination xy"},
    {ID_PIP2_TO_DIM,        "pip2_to_dim",      "pip2 destination width/height"},
    {ID_PIP2_SCL_INFO,      "pip2_scl_info",    "pip2 scl info, bit[7:0]=from frmX, bit[15:8]=to frmX, bit[23:16]=scl layout"},

    {ID_PIP3_FROM_XY,       "pip3_from_xy",     "pip3 source xy"},
    {ID_PIP3_FROM_DIM,      "pip3_from_dim",    "pip3 source width/height"},
    {ID_PIP3_TO_XY,         "pip3_to_xy",       "pip3 destination xy"},
    {ID_PIP3_TO_DIM,        "pip3_to_dim",      "pip3 destination width/height"},
    {ID_PIP3_SCL_INFO,      "pip3_scl_info",    "pip3 scl info, bit[7:0]=from frmX, bit[15:8]=to frmX, bit[23:16]=scl layout"},

    {ID_PIP4_FROM_XY,       "pip4_from_xy",     "pip4 source xy"},
    {ID_PIP4_FROM_DIM,      "pip4_from_dim",    "pip4 source width/height"},
    {ID_PIP4_TO_XY,         "pip4_to_xy",       "pip4 destination xy"},
    {ID_PIP4_TO_DIM,        "pip4_to_dim",      "pip4 destination width/height"},
    {ID_PIP4_SCL_INFO,      "pip4_scl_info",    "pip4 scl info, bit[7:0]=from frmX, bit[15:8]=to frmX, bit[23:16]=scl layout"},

    {ID_PIP5_FROM_XY,       "pip5_from_xy",     "pip5 source xy"},
    {ID_PIP5_FROM_DIM,      "pip5_from_dim",    "pip5 source width/height"},
    {ID_PIP5_TO_XY,         "pip5_to_xy",       "pip5 destination xy"},
    {ID_PIP5_TO_DIM,        "pip5_to_dim",      "pip5 destination width/height"},
    {ID_PIP5_SCL_INFO,      "pip5_scl_info",    "pip5 scl info, bit[7:0]=from frmX, bit[15:8]=to frmX, bit[23:16]=scl layout"},

    {ID_PIP6_FROM_XY,       "pip6_from_xy",     "pip6 source xy"},
    {ID_PIP6_FROM_DIM,      "pip6_from_dim",    "pip6 source width/height"},
    {ID_PIP6_TO_XY,         "pip6_to_xy",       "pip6 destination xy"},
    {ID_PIP6_TO_DIM,        "pip6_to_dim",      "pip6 destination width/height"},
    {ID_PIP6_SCL_INFO,      "pip6_scl_info",    "pip6 scl info, bit[7:0]=from frmX, bit[15:8]=to frmX, bit[23:16]=scl layout"},

    {ID_PIP7_FROM_XY,       "pip7_from_xy",     "pip7 source xy"},
    {ID_PIP7_FROM_DIM,      "pip7_from_dim",    "pip7 source width/height"},
    {ID_PIP7_TO_XY,         "pip7_to_xy",       "pip7 destination xy"},
    {ID_PIP7_TO_DIM,        "pip7_to_dim",      "pip7 destination width/height"},
    {ID_PIP7_SCL_INFO,      "pip7_scl_info",    "pip7 scl info, bit[7:0]=from frmX, bit[15:8]=to frmX, bit[23:16]=scl layout"},

    {ID_PIP8_FROM_XY,       "pip8_from_xy",     "pip8 source xy"},
    {ID_PIP8_FROM_DIM,      "pip8_from_dim",    "pip8 source width/height"},
    {ID_PIP8_TO_XY,         "pip8_to_xy",       "pip8 destination xy"},
    {ID_PIP8_TO_DIM,        "pip8_to_dim",      "pip8 destination width/height"},
    {ID_PIP8_SCL_INFO,      "pip8_scl_info",    "pip8 scl info, bit[7:0]=from frmX, bit[15:8]=to frmX, bit[23:16]=scl layout"},

    {ID_PIP9_FROM_XY,       "pip9_from_xy",     "pip9 source xy"},
    {ID_PIP9_FROM_DIM,      "pip9_from_dim",    "pip9 source width/height"},
    {ID_PIP9_TO_XY,         "pip9_to_xy",       "pip9 destination xy"},
    {ID_PIP9_TO_DIM,        "pip9_to_dim",      "pip9 destination width/height"},
    {ID_PIP9_SCL_INFO,      "pip9_scl_info",    "pip9 scl info, bit[7:0]=from frmX, bit[15:8]=to frmX, bit[23:16]=scl layout"},
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

void print_property(struct video_entity_job_t *job,struct video_property_t *property)
{
    int i;
    int engine = ENTITY_FD_ENGINE(job->fd);
    int minor = ENTITY_FD_MINOR(job->fd);

    for(i = 0; i < MAX_PROPERTYS; i++) {
        if(property[i].id == ID_NULL)
            break;
        if(i == 0)
            DEBUG_M(LOG_WARNING,engine,minor,"{%d,%d} job %d property:",engine,minor,job->id);
        DEBUG_M(LOG_WARNING,engine,minor,"  ID:%d,Value:%d\n",property[i].id,property[i].value);
        printk("{%d,%d} job %d property:\n",engine,minor,job->id);
        printk("  ID:%d,Value:%d\n",property[i].id,property[i].value);
    }
}

/*
 * pip mode, src format = dst format = TYPE_YUV422_2FRAMES_2FRAMES,
 * to save property count, vg do not send frame2 property to scaler
 * scaler have to duplicate (frame1 -> buf1) property to (frame2 -> buf1)
 * and (buf1 -> frame1) property to (buf1 -> frame2)
 */
static int dup_pip_frm2_property(job_node_t *node)
{
    int i = 0;
    int win_cnt = 0;
    fscaler300_property_t *param = &node->property;

    win_cnt = node->property.pip.win_cnt;
    // frame1 -> buf1, duplicate frame2 -> buf1
    for (i = 0; i < win_cnt; i++) {
        if ((node->property.pip.src_frm[i] == 0x1) && (node->property.pip.dst_frm[i] == 0x11)) {
            node->property.pip.win_cnt++;
            if (node->property.pip.win_cnt > PIP_COUNT) {
                printk("[%s] pip window count %d is over max\n", __func__, node->property.pip.win_cnt);
                printm(MODULE_NAME, "[%s] pip window count %d is over max\n", __func__, node->property.pip.win_cnt);
                return -1;
            }

            param->pip.src_frm[node->property.pip.win_cnt - 1] = 0x2;
            param->pip.dst_frm[node->property.pip.win_cnt - 1] = 0x11;
            param->pip.src_x[node->property.pip.win_cnt - 1] = param->pip.src_x[i];
            param->pip.src_y[node->property.pip.win_cnt - 1] = param->pip.src_y[i];
            param->pip.dst_x[node->property.pip.win_cnt - 1] = param->pip.dst_x[i];
            param->pip.dst_y[node->property.pip.win_cnt - 1] = param->pip.dst_y[i];
            param->pip.src_width[node->property.pip.win_cnt - 1] = param->pip.src_width[i];
            param->pip.src_height[node->property.pip.win_cnt - 1] = param->pip.src_height[i];
            param->pip.dst_width[node->property.pip.win_cnt - 1] = param->pip.dst_width[i];
            param->pip.dst_height[node->property.pip.win_cnt - 1] = param->pip.dst_height[i];
            param->pip.scl_info[node->property.pip.win_cnt - 1] = param->pip.scl_info[i];
        }
    }
    // buf1 -> frame1, duplicate buf1 -> frame2
    for (i = 0; i < win_cnt; i++) {
        if ((node->property.pip.src_frm[i] == 0x11) && (node->property.pip.dst_frm[i] == 0x1)) {
            node->property.pip.win_cnt++;
            if (node->property.pip.win_cnt > PIP_COUNT) {
                printk("[%s] pip window count %d is over max\n", __func__, node->property.pip.win_cnt);
                return -1;
            }

            param->pip.src_frm[node->property.pip.win_cnt - 1] = 0x11;
            param->pip.dst_frm[node->property.pip.win_cnt - 1] = 0x2;
            param->pip.src_x[node->property.pip.win_cnt - 1] = param->pip.src_x[i];
            param->pip.src_y[node->property.pip.win_cnt - 1] = param->pip.src_y[i];
            param->pip.dst_x[node->property.pip.win_cnt - 1] = param->pip.dst_x[i];
            param->pip.dst_y[node->property.pip.win_cnt - 1] = param->pip.dst_y[i];
            param->pip.src_width[node->property.pip.win_cnt - 1] = param->pip.src_width[i];
            param->pip.src_height[node->property.pip.win_cnt - 1] = param->pip.src_height[i];
            param->pip.dst_width[node->property.pip.win_cnt - 1] = param->pip.dst_width[i];
            param->pip.dst_height[node->property.pip.win_cnt - 1] = param->pip.dst_height[i];
            param->pip.scl_info[node->property.pip.win_cnt - 1] = param->pip.scl_info[i];
        }
    }

    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s win cont %d\n", __func__, node->property.pip.win_cnt);
        for (i = 0; i < node->property.pip.win_cnt; i++) {
            printm(MODULE_NAME, "src frm [%d] dst frm [%d]\n", param->pip.src_frm[i], param->pip.dst_frm[i]);
            printm(MODULE_NAME, "src x [%d] src y [%d]\n", param->pip.src_x[i], param->pip.src_y[i]);
            printm(MODULE_NAME, "dst x [%d] dst y [%d]\n", param->pip.dst_x[i], param->pip.dst_y[i]);
            printm(MODULE_NAME, "src width [%d] src height [%d]\n", param->pip.src_width[i], param->pip.src_height[i]);
            printm(MODULE_NAME, "dst width [%d] dst height [%d]\n", param->pip.dst_width[i], param->pip.dst_height[i]);
        }
    }

    return 0;
}

/*
 * parse vg pip property
 */
static int driver_parse_pip_property(job_node_t *node, struct video_entity_job_t *job, int idx)
{
    struct video_property_t *property = job->in_prop;

    switch (property[idx].id) {
        case ID_PIP0_FROM_XY:
                node->property.pip.src_x[0] = EM_PARAM_X(property[idx].value);
                node->property.pip.src_y[0] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src0 x %d y %d\n", node->property.pip.src_x[0], node->property.pip.src_y[0]);
            break;
        case ID_PIP0_FROM_DIM:
                node->property.pip.src_width[0] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.src_height[0] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src0 w %d h %d\n", node->property.pip.src_width[0], node->property.pip.src_height[0]);
            break;
        case ID_PIP0_TO_XY:
                node->property.pip.dst_x[0] = EM_PARAM_X(property[idx].value);
                node->property.pip.dst_y[0] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst0 x %d y %d\n", node->property.pip.dst_x[0], node->property.pip.dst_y[0]);
            break;
        case ID_PIP0_TO_DIM:
                node->property.pip.dst_width[0] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.dst_height[0] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst0 w %d h %d\n", node->property.pip.dst_width[0], node->property.pip.dst_height[0]);
            break;
        case ID_PIP0_SCL_INFO:
                node->property.pip.src_frm[0] = property[idx].value & 0xff;
                node->property.pip.dst_frm[0] = (property[idx].value & 0xff00) >> 8;
                node->property.pip.scl_info[0] = (property[idx].value & 0xffff0000) >> 16;
                node->property.pip.win_cnt++;
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src/dst0 frm %d/%d  scl info %d count %d\n", node->property.pip.src_frm[0], node->property.pip.dst_frm[0], node->property.pip.scl_info[0], node->property.pip.win_cnt);
            break;
        case ID_PIP1_FROM_XY:
                node->property.pip.src_x[1] = EM_PARAM_X(property[idx].value);
                node->property.pip.src_y[1] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src1 x %d y %d\n", node->property.pip.src_x[1], node->property.pip.src_y[1]);
            break;
        case ID_PIP1_FROM_DIM:
                node->property.pip.src_width[1] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.src_height[1] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src1 w %d h %d\n", node->property.pip.src_width[1], node->property.pip.src_height[1]);
            break;
        case ID_PIP1_TO_XY:
                node->property.pip.dst_x[1] = EM_PARAM_X(property[idx].value);
                node->property.pip.dst_y[1] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst1 x %d y %d\n", node->property.pip.dst_x[1], node->property.pip.dst_y[1]);
            break;
        case ID_PIP1_TO_DIM:
                node->property.pip.dst_width[1] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.dst_height[1] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst1 w %d h %d\n", node->property.pip.dst_width[1], node->property.pip.dst_height[1]);
            break;
        case ID_PIP1_SCL_INFO:
                node->property.pip.src_frm[1] = property[idx].value & 0xff;
                node->property.pip.dst_frm[1] = (property[idx].value & 0xff00) >> 8;
                node->property.pip.scl_info[1] = (property[idx].value & 0xffff0000) >> 16;
                node->property.pip.win_cnt++;
                if (node->property.pip.win_cnt > PIP_COUNT)
                    return -1;
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src/dst1 frm %d/%d  scl info %d count %d\n", node->property.pip.src_frm[1], node->property.pip.dst_frm[1], node->property.pip.scl_info[1], node->property.pip.win_cnt);
            break;
        case ID_PIP2_FROM_XY:
                node->property.pip.src_x[2] = EM_PARAM_X(property[idx].value);
                node->property.pip.src_y[2] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src2 x %d y %d\n", node->property.pip.src_x[2], node->property.pip.src_y[2]);
            break;
        case ID_PIP2_FROM_DIM:
                node->property.pip.src_width[2] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.src_height[2] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src2 w %d h %d\n", node->property.pip.src_width[2], node->property.pip.src_height[2]);
            break;
        case ID_PIP2_TO_XY:
                node->property.pip.dst_x[2] = EM_PARAM_X(property[idx].value);
                node->property.pip.dst_y[2] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst2 x %d y %d\n", node->property.pip.dst_x[2], node->property.pip.dst_y[2]);
            break;
        case ID_PIP2_TO_DIM:
                node->property.pip.dst_width[2] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.dst_height[2] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst2 w %d h %d\n", node->property.pip.dst_width[2], node->property.pip.dst_height[2]);
            break;
        case ID_PIP2_SCL_INFO:
                node->property.pip.src_frm[2] = property[idx].value & 0xff;
                node->property.pip.dst_frm[2] = (property[idx].value & 0xff00) >> 8;
                node->property.pip.scl_info[2] = (property[idx].value & 0xffff0000) >> 16;
                node->property.pip.win_cnt++;
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src/dst2 frm %d/%d  scl info %d count %d\n", node->property.pip.src_frm[2], node->property.pip.dst_frm[2], node->property.pip.scl_info[2], node->property.pip.win_cnt);
            break;
        case ID_PIP3_FROM_XY:
                node->property.pip.src_x[3] = EM_PARAM_X(property[idx].value);
                node->property.pip.src_y[3] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src3 x %d y %d\n", node->property.pip.src_x[3], node->property.pip.src_y[3]);
            break;
        case ID_PIP3_FROM_DIM:
                node->property.pip.src_width[3] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.src_height[3] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src3 w %d h %d\n", node->property.pip.src_width[3], node->property.pip.src_height[3]);
            break;
        case ID_PIP3_TO_XY:
                node->property.pip.dst_x[3] = EM_PARAM_X(property[idx].value);
                node->property.pip.dst_y[3] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst3 x %d y %d\n", node->property.pip.dst_x[3], node->property.pip.dst_y[3]);
            break;
        case ID_PIP3_TO_DIM:
                node->property.pip.dst_width[3] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.dst_height[3] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst3 w %d h %d\n", node->property.pip.dst_width[3], node->property.pip.dst_height[3]);
            break;
        case ID_PIP3_SCL_INFO:
                node->property.pip.src_frm[3] = property[idx].value & 0xff;
                node->property.pip.dst_frm[3] = (property[idx].value & 0xff00) >> 8;
                node->property.pip.scl_info[3] = (property[idx].value & 0xffff0000) >> 16;
                node->property.pip.win_cnt++;
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src/dst3 frm %d/%d  scl info %d count %d\n", node->property.pip.src_frm[3], node->property.pip.dst_frm[3], node->property.pip.scl_info[3], node->property.pip.win_cnt);
            break;
        case ID_PIP4_FROM_XY:
                node->property.pip.src_x[4] = EM_PARAM_X(property[idx].value);
                node->property.pip.src_y[4] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src4 x %d y %d\n", node->property.pip.src_x[4], node->property.pip.src_y[4]);
            break;
        case ID_PIP4_FROM_DIM:
                node->property.pip.src_width[4] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.src_height[4] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src4 w %d h %d\n", node->property.pip.src_width[4], node->property.pip.src_height[4]);
            break;
        case ID_PIP4_TO_XY:
                node->property.pip.dst_x[4] = EM_PARAM_X(property[idx].value);
                node->property.pip.dst_y[4] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst4 x %d y %d\n", node->property.pip.dst_x[4], node->property.pip.dst_y[4]);
            break;
        case ID_PIP4_TO_DIM:
                node->property.pip.dst_width[4] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.dst_height[4] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst4 w %d h %d\n", node->property.pip.dst_width[4], node->property.pip.dst_height[4]);
            break;
        case ID_PIP4_SCL_INFO:
                node->property.pip.src_frm[4] = property[idx].value & 0xff;
                node->property.pip.dst_frm[4] = (property[idx].value & 0xff00) >> 8;
                node->property.pip.scl_info[4] = (property[idx].value & 0xffff0000) >> 16;
                node->property.pip.win_cnt++;
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src/dst4 frm %d/%d  scl info %d count %d\n", node->property.pip.src_frm[4], node->property.pip.dst_frm[4], node->property.pip.scl_info[4], node->property.pip.win_cnt);
            break;
        case ID_PIP5_FROM_XY:
                node->property.pip.src_x[5] = EM_PARAM_X(property[idx].value);
                node->property.pip.src_y[5] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src5 x %d y %d\n", node->property.pip.src_x[5], node->property.pip.src_y[5]);
            break;
        case ID_PIP5_FROM_DIM:
                node->property.pip.src_width[5] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.src_height[5] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src5 w %d h %d\n", node->property.pip.src_width[5], node->property.pip.src_height[5]);
            break;
        case ID_PIP5_TO_XY:
                node->property.pip.dst_x[5] = EM_PARAM_X(property[idx].value);
                node->property.pip.dst_y[5] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst5 x %d y %d\n", node->property.pip.dst_x[5], node->property.pip.dst_y[5]);
            break;
        case ID_PIP5_TO_DIM:
                node->property.pip.dst_width[5] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.dst_height[5] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst5 w %d h %d\n", node->property.pip.dst_width[5], node->property.pip.dst_height[5]);
            break;
        case ID_PIP5_SCL_INFO:
                node->property.pip.src_frm[5] = property[idx].value & 0xff;
                node->property.pip.dst_frm[5] = (property[idx].value & 0xff00) >> 8;
                node->property.pip.scl_info[5] = (property[idx].value & 0xffff0000) >> 16;
                node->property.pip.win_cnt++;
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src/dst5 frm %d/%d  scl info %d count %d\n", node->property.pip.src_frm[5], node->property.pip.dst_frm[5], node->property.pip.scl_info[5], node->property.pip.win_cnt);
            break;
        case ID_PIP6_FROM_XY:
                node->property.pip.src_x[6] = EM_PARAM_X(property[idx].value);
                node->property.pip.src_y[6] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src6 x %d y %d\n", node->property.pip.src_x[6], node->property.pip.src_y[6]);
            break;
        case ID_PIP6_FROM_DIM:
                node->property.pip.src_width[6] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.src_height[6] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src6 w %d h %d\n", node->property.pip.src_width[6], node->property.pip.src_height[6]);
            break;
        case ID_PIP6_TO_XY:
                node->property.pip.dst_x[6] = EM_PARAM_X(property[idx].value);
                node->property.pip.dst_y[6] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst6 x %d y %d\n", node->property.pip.dst_x[6], node->property.pip.dst_y[6]);
            break;
        case ID_PIP6_TO_DIM:
                node->property.pip.dst_width[6] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.dst_height[6] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst6 w %d h %d\n", node->property.pip.dst_width[6], node->property.pip.dst_height[6]);
            break;
        case ID_PIP6_SCL_INFO:
                node->property.pip.src_frm[6] = property[idx].value & 0xff;
                node->property.pip.dst_frm[6] = (property[idx].value & 0xff00) >> 8;
                node->property.pip.scl_info[6] = (property[idx].value & 0xffff0000) >> 16;
                node->property.pip.win_cnt++;
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src/dst6 frm %d/%d  scl info %d count %d\n", node->property.pip.src_frm[6], node->property.pip.dst_frm[6], node->property.pip.scl_info[6], node->property.pip.win_cnt);
            break;
        case ID_PIP7_FROM_XY:
                node->property.pip.src_x[7] = EM_PARAM_X(property[idx].value);
                node->property.pip.src_y[7] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src7 x %d y %d\n", node->property.pip.src_x[7], node->property.pip.src_y[7]);
            break;
        case ID_PIP7_FROM_DIM:
                node->property.pip.src_width[7] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.src_height[7] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src7 w %d h %d\n", node->property.pip.src_width[7], node->property.pip.src_height[7]);
            break;
        case ID_PIP7_TO_XY:
                node->property.pip.dst_x[7] = EM_PARAM_X(property[idx].value);
                node->property.pip.dst_y[7] = EM_PARAM_Y(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst6 x %d y %d\n", node->property.pip.dst_x[6], node->property.pip.dst_y[6]);
            break;
        case ID_PIP7_TO_DIM:
                node->property.pip.dst_width[7] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.dst_height[7] = EM_PARAM_HEIGHT(property[idx].value);
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "dst7 w %d h %d\n", node->property.pip.dst_width[7], node->property.pip.dst_height[7]);
            break;
        case ID_PIP7_SCL_INFO:
                node->property.pip.src_frm[7] = property[idx].value & 0xff;
                node->property.pip.dst_frm[7] = (property[idx].value & 0xff00) >> 8;
                node->property.pip.scl_info[7] = (property[idx].value & 0xffff0000) >> 16;
                node->property.pip.win_cnt++;
                if (debug_mode >= 1)
                    printm(MODULE_NAME, "src/dst7 frm %d/%d  scl info %d count %d\n", node->property.pip.src_frm[7], node->property.pip.dst_frm[7], node->property.pip.scl_info[7], node->property.pip.win_cnt);
            break;
        case ID_PIP8_FROM_XY:
                node->property.pip.src_x[8] = EM_PARAM_X(property[idx].value);
                node->property.pip.src_y[8] = EM_PARAM_Y(property[idx].value);
            break;
        case ID_PIP8_FROM_DIM:
                node->property.pip.src_width[8] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.src_height[8] = EM_PARAM_HEIGHT(property[idx].value);
            break;
        case ID_PIP8_TO_XY:
                node->property.pip.dst_x[8] = EM_PARAM_X(property[idx].value);
               node->property.pip.dst_y[8] = EM_PARAM_Y(property[idx].value);
            break;
        case ID_PIP8_TO_DIM:
                node->property.pip.dst_width[8] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.dst_height[8] = EM_PARAM_HEIGHT(property[idx].value);
            break;
        case ID_PIP8_SCL_INFO:
                node->property.pip.src_frm[8] = property[idx].value & 0xff;
                node->property.pip.dst_frm[8] = (property[idx].value & 0xff00) >> 8;
                node->property.pip.scl_info[8] = (property[idx].value & 0xffff0000) >> 16;
                node->property.pip.win_cnt++;
            break;
        case ID_PIP9_FROM_XY:
                node->property.pip.src_x[9] = EM_PARAM_X(property[idx].value);
                node->property.pip.src_y[9] = EM_PARAM_Y(property[idx].value);
            break;
        case ID_PIP9_FROM_DIM:
                node->property.pip.src_width[9] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.src_height[9] = EM_PARAM_HEIGHT(property[idx].value);
            break;
        case ID_PIP9_TO_XY:
                node->property.pip.dst_x[9] = EM_PARAM_X(property[idx].value);
                node->property.pip.dst_y[9] = EM_PARAM_Y(property[idx].value);
            break;
        case ID_PIP9_TO_DIM:
                node->property.pip.dst_width[9] = EM_PARAM_WIDTH(property[idx].value);
                node->property.pip.dst_height[9] = EM_PARAM_HEIGHT(property[idx].value);
            break;
        case ID_PIP9_SCL_INFO:
                node->property.pip.src_frm[9] = property[idx].value & 0xff;
                node->property.pip.dst_frm[9] = (property[idx].value & 0xff00) >> 8;
                node->property.pip.scl_info[9] = (property[idx].value & 0xffff0000) >> 16;
                node->property.pip.win_cnt++;
            break;
        default:
            break;
    }

    return 0;
}

/*
 * param: private data
 */
static int driver_parse_in_property(job_node_t *node, struct video_entity_job_t *job)
{
    int i = 0, ret = 0, is_dup_pip = 0;
    struct video_property_t *property = job->in_prop;

    /* fill up the input parameters */
    while(property[i].id != 0) {
        if (property[i].ch > virtual_chn_num) {
            scl_err("Error! property channel number [%d] over MAX\n", property[i].ch);
            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! property channel number over MAX\n");
            return -1;
        }

        if (property[i].id >= ID_START) {
            ret = driver_parse_pip_property(node, job, i);
            if (ret < 0) {
                printk("[%s] driver_parse_pip_property fail\n", __func__);
                printm(MODULE_NAME, "[%s] driver_parse_pip_property fail\n", __func__);
                return -1;
                if (node->property.pip.win_cnt > PIP_COUNT) {
                    printk("pip win count %d over MAX %d\n", node->property.pip.win_cnt, PIP_COUNT);
                    printm(MODULE_NAME, "pip win count %d over MAX %d\n", node->property.pip.win_cnt, PIP_COUNT);
                    return -1;
                }
            }
        }
        else {
            switch (property[i].id) {
                case ID_SRC_FMT:
                        node->property.src_fmt = property[i].value;
                    break;
                case ID_SRC_XY:
                        node->property.vproperty[property[i].ch].src_x = EM_PARAM_X(property[i].value);
                        node->property.vproperty[property[i].ch].src_y = EM_PARAM_Y(property[i].value);
                        node->property.vproperty[property[i].ch].vch_enable = 1;
                        node->ch_id = property[i].ch;
                        node->ch_cnt++;
                    break;
                case ID_SRC_BG_DIM:
                        node->property.src_bg_width = EM_PARAM_WIDTH(property[i].value);
                        node->property.src_bg_height = EM_PARAM_HEIGHT(property[i].value);
                    break;
                case ID_SRC_DIM:
                        node->property.vproperty[property[i].ch].src_width = EM_PARAM_WIDTH(property[i].value);
                        node->property.vproperty[property[i].ch].src_height = EM_PARAM_HEIGHT(property[i].value);
                    break;
                case ID_DI_RESULT:
                       node->property.di_result = property[i].value;
                    break;
                case ID_SRC_INTERLACE:
                       node->property.vproperty[property[i].ch].src_interlace = property[i].value;
                    break;
                case ID_DST_FMT:
                        node->property.dst_fmt = property[i].value;
                    break;
                case ID_DST_XY:
                        node->property.vproperty[property[i].ch].dst_x = EM_PARAM_X(property[i].value);
                        node->property.vproperty[property[i].ch].dst_y = EM_PARAM_Y(property[i].value);
                    break;
                case ID_DST_BG_DIM:
                        node->property.dst_bg_width = EM_PARAM_WIDTH(property[i].value);
                        node->property.dst_bg_height = EM_PARAM_HEIGHT(property[i].value);
                    break;
                case ID_DST_DIM:
                        node->property.vproperty[property[i].ch].dst_width = EM_PARAM_WIDTH(property[i].value);
                        node->property.vproperty[property[i].ch].dst_height = EM_PARAM_HEIGHT(property[i].value);
                    break;
                case ID_SCL_FEATURE:
                        node->property.vproperty[property[i].ch].scl_feature = property[i].value;
                        if (!is_dup_pip && (property[i].value & (1 << 3)))
                            is_dup_pip = 1;
                    break;
                case ID_SUB_YUV:
                        node->property.sub_yuv = property[i].value;
                    break;
                case ID_AUTO_ASPECT_RATIO:
                        node->property.vproperty[property[i].ch].aspect_ratio.pal_idx = EM_PARAM_X(property[i].value);
                        node->property.vproperty[property[i].ch].aspect_ratio.enable = EM_PARAM_Y(property[i].value);
                    break;
                case ID_AUTO_BORDER:
                        node->property.vproperty[property[i].ch].border.pal_idx = (property[i].value & 0xffff0000) >> 16;
                        node->property.vproperty[property[i].ch].border.width = (property[i].value & 0xff00) >> 8;
                        node->property.vproperty[property[i].ch].border.enable = property[i].value & 0xff;
                    break;
                case ID_DST2_BG_DIM:
                        node->property.dst2_bg_width = EM_PARAM_WIDTH(property[i].value);
                        node->property.dst2_bg_height = EM_PARAM_HEIGHT(property[i].value);
                    break;
                case ID_SRC2_BG_DIM:
                        node->property.src2_bg_width = EM_PARAM_WIDTH(property[i].value);
                        node->property.src2_bg_height = EM_PARAM_HEIGHT(property[i].value);
                    break;
                case ID_DST2_XY:
                        node->property.vproperty[property[i].ch].dst2_x = EM_PARAM_WIDTH(property[i].value);
                        node->property.vproperty[property[i].ch].dst2_y = EM_PARAM_HEIGHT(property[i].value);
                    break;
                case ID_DST2_DIM:
                        node->property.vproperty[property[i].ch].dst2_width = EM_PARAM_WIDTH(property[i].value);
                        node->property.vproperty[property[i].ch].dst2_height = EM_PARAM_HEIGHT(property[i].value);
                    break;
                case ID_SRC2_XY:
                        node->property.vproperty[property[i].ch].src2_x = EM_PARAM_WIDTH(property[i].value);
                        node->property.vproperty[property[i].ch].src2_y = EM_PARAM_HEIGHT(property[i].value);
                    break;
                case ID_SRC2_DIM:
                        node->property.vproperty[property[i].ch].src2_width = EM_PARAM_WIDTH(property[i].value);
                        node->property.vproperty[property[i].ch].src2_height = EM_PARAM_HEIGHT(property[i].value);
                    break;
                case ID_WIN2_BG_DIM:
                        node->property.win2_bg_width = EM_PARAM_WIDTH(property[i].value);
                        node->property.win2_bg_height = EM_PARAM_HEIGHT(property[i].value);
                    break;
#ifdef WIN2_SUPPORT_XY
                case ID_WIN2_XY:
                        node->property.win2_x = EM_PARAM_X(property[i].value);
                        node->property.win2_y = EM_PARAM_Y(property[i].value);
                    break;
#endif
                case ID_CAS_SCL_RATIO:
                        node->property.cas_ratio = property[i].value;
                    break;
                case ID_BUF_CLEAN:
                        node->property.vproperty[property[i].ch].buf_clean = property[i].value;
                    break;
                case ID_VG_SERIAL:
                        node->property.vg_serial = property[i].value;
                    break;
                default:
                    break;
            }
        }
        i++;
    }

    if (is_dup_pip) {
        if ((node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES) && (node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
            ret = dup_pip_frm2_property(node);
            if (ret < 0) {
                printk("[%s] dup_pip_frm2_property fail\n", __func__);
                printm(MODULE_NAME, "[%s] dup_pip_frm2_property fail\n", __func__);
                return -1;
            }
        }
    }

    return 0;
}

#if GM8210
int ep_process_state = 0;

/* callback EP's rc_node_list to RC */
void fscaler300_ep_process(void)
{
    int ret = 0, loop_cnt = 0;
    job_node_t  *node, *ne;
    int curr_jiffies, diff;

check_again:
    if (dual_debug == 1) printm(MODULE_NAME, "%s down before %d\n", __func__, get_gm_jiffies());

    ep_process_state = 1;
    down(&global_info.sema_lock);
    ep_process_state = 2;

    if (dual_debug == 1) printm(MODULE_NAME, "%s down after %d\n", __func__, get_gm_jiffies());

    list_for_each_entry_safe(node, ne, &global_info.ep_rcnode_cb_list, plist) {
        if ((node->status == JOB_STS_QUEUE) || (node->status == JOB_STS_MATCH))
            break;
        list_del(&node->plist);
        up(&global_info.sema_lock);
        goto handle_ep_process;
    }
    if (dual_debug == 1) printm(MODULE_NAME, "%s out\n", __func__);
    up(&global_info.sema_lock);
    ep_process_state = 0;
    return;

handle_ep_process:

    ep_process_state = 3;
    loop_cnt = 0;
    if (node->status == JOB_STS_TX) {
        while (!scl_dma_check_job_done(node->job_id)) {
            set_current_state(TASK_INTERRUPTIBLE);
            schedule_timeout(msecs_to_jiffies(10));
            if ((loop_cnt ++) > 300) {
                printk("%s, jobid: %d enter infinite loop: %d, ep_process_state: %d \n", __func__,
                                                            node->job_id, loop_cnt, ep_process_state);
                printm(MODULE_NAME, "%s, jobid: %d enter infinite loop: %d, ep_process_state: %d \n",
                                                __func__, node->job_id, loop_cnt, ep_process_state);
                if (loop_cnt == 301)
                    scl_dma_dump(NULL);

                schedule_timeout(msecs_to_jiffies(100));
            }
        }
    }

    ep_process_state = 4;

    curr_jiffies = get_gm_jiffies();
    ret = fscaler300_ep_io_tx(node);
    if (ret < 0)
        damnit(MODULE_NAME);

    diff = get_gm_jiffies() - curr_jiffies;
    if (dual_debug == 1) printm(MODULE_NAME, "ep process %d\n", diff);
    kmem_cache_free(global_info.node_cache, node);

    rc_node_mem_cnt --;

    goto check_again;
}
#ifdef USE_KTHREAD
#if GM8210
int ep_running = 0;
/* callback to RC */
static int eptx_thread(void *__cwq)
{
    int status;
    int ep_running = 1;
    set_current_state(TASK_INTERRUPTIBLE);
    set_user_nice(current, -19);

    do {
        status = wait_event_timeout(eptx_thread_wait, eptx_wakeup_event, msecs_to_jiffies(1000));

        eptx_wakeup_event = 0;
        /* ep tx process */
        fscaler300_ep_process();
    } while(!kthread_should_stop());

    __set_current_state(TASK_RUNNING);
    ep_running = 0;
    return 0;
}

static void eptx_wake_up(void)
{
    eptx_wakeup_event = 1;
    wake_up(&eptx_thread_wait);
}
#endif
#endif
#endif
#if GM8210
#ifdef TEST_WQ
int test_jiffies = 0, test_diff = 0, temp_jiffies = 0, temp_diff = 0;
void fscaler300_test_process(void)
{
	int current_jiffies = get_gm_jiffies();
	unsigned int diff = 0;
	int diffs = 0;

	if (temp_jiffies)
		diffs = jiffies - temp_jiffies;
	temp_jiffies = jiffies;
	if (temp_diff < diffs)
	    temp_diff = diffs;
	if (test_jiffies)
		diff = current_jiffies - test_jiffies;
	test_jiffies = current_jiffies;
	if (test_diff < diff)
		test_diff = diff;
	PREPARE_DELAYED_WORK(&process_test_work,(void *)fscaler300_test_process);
    queue_delayed_work(test_workq, &process_test_work, msecs_to_jiffies(100));
}
#endif
#endif
/*
 * return the job to videograph
 */
void fscaler300_callback_process(void)
{
    job_node_t  *node, *ne, *curr, *ne1;
    struct video_entity_job_t *job, *child;
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    set_user_nice(current, -20);

    fmem_get_identifier(&pci_id, &cpu_id);

	if (dual_debug || flow_check) printm(MODULE_NAME,"%s in\n", __func__);
    down(&global_info.sema_lock);
    list_for_each_entry_safe(node, ne, &global_info.node_list, plist) {
        if (node->status == JOB_STS_DONE || node->status == JOB_STS_FLUSH || node->status == JOB_STS_DONOTHING ||
            node->status == JOB_STS_FAIL || node->status == JOB_STS_MARK_SUCCESS) {
            job = (struct video_entity_job_t *)node->private;

            if (node->status == JOB_STS_DONE || node->status == JOB_STS_DONOTHING || node->status == JOB_STS_MARK_SUCCESS)
                job->status = JOB_STATUS_FINISH;
            else
                job->status = JOB_STATUS_FAIL;

            if (node->need_callback) {
#if GM8210
                // at dual 8210, trigger dma first, then callback
                if (I_AM_EP(node->property.vproperty[node->ch_id].scl_feature))
                    panic("%s, bug1, get ep job for 2ddma! \n", __func__);
#endif /* GM8210 */
                job->callback(node->private); //callback root job
                if (dual_debug || flow_check)printm(MODULE_NAME, "**%s, callback0 [%u] sts %x**\n", __func__, node->job_id, job->status);
                if (dual_debug == 1) {
                    if (job->status == JOB_STATUS_FINISH)
                        printm(MODULE_NAME, "callback id %u status finish\n", job->id);
                    if (job->status == JOB_STATUS_FAIL)
                        printm(MODULE_NAME, "callback id %u status fail\n", job->id);
                }
            }
            /* callback & free child node*/
            list_for_each_entry_safe(curr, ne1, &node->clist ,clist) {
                child = (struct video_entity_job_t *)curr->private;
                if (curr->status == JOB_STS_DONE || curr->status == JOB_STS_DONOTHING)
                    child->status = JOB_STATUS_FINISH;
                else
                    child->status = JOB_STATUS_FAIL;

                if (curr->need_callback) {
#if GM8210
                    if (I_AM_EP(node->property.vproperty[node->ch_id].scl_feature))
                        panic("%s, bug2, get ep job for 2ddma! \n", __func__);
#endif
                    job->callback(node->private); //callback root node
                    if (dual_debug || flow_check)printm(MODULE_NAME, "**%s, callback1 [%u] sts %x**\n", __func__, node->job_id, job->status);
                    if (dual_debug == 1) {
                        if (job->status == JOB_STATUS_FINISH)
                            printm(MODULE_NAME, "callback id %u status finish\n", job->id);
                        if (job->status == JOB_STATUS_FAIL)
                            printm(MODULE_NAME, "callback id %u status fail\n", job->id);
                    }
                }
                kmem_cache_free(global_info.vproperty_cache, curr->property.vproperty);
                MEMCNT_SUB(&priv, 1);
                kmem_cache_free(global_info.node_cache, curr);
                MEMCNT_SUB(&priv, 1);
            }
            /* free parent node */
            list_del(&node->plist);
            kmem_cache_free(global_info.vproperty_cache, node->property.vproperty);
            MEMCNT_SUB(&priv, 1);
            kmem_cache_free(global_info.node_cache, node);
            MEMCNT_SUB(&priv, 1);
        } else {
            break;
        }
    }

    up(&global_info.sema_lock);
    /* go through the ep_rcnode_cb_list list
     */
#ifdef USE_KTHREAD
#if GM8210
    if ((pci_id != FMEM_PCI_HOST) && (cpu_id == FMEM_CPU_FA626)) {
	    if (dual_debug == 1) printm(MODULE_NAME, "+++++++ wake up tx\n");
	    eptx_wake_up();
    }
#endif
#endif
#ifdef USE_WQ
#if GM8210
    if (dual_debug == 1) printm(MODULE_NAME, "+++++++ wake up tx\n");
    PREPARE_DELAYED_WORK(&process_ep_work,(void *)fscaler300_ep_process);
    queue_delayed_work(callback_workq, &process_ep_work, msecs_to_jiffies(0));
#endif
    /* schedule the delay workq */
    if (dual_debug == 1) printm(MODULE_NAME, "waku up callback %d\n", __LINE__);
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)fscaler300_callback_process);
    queue_delayed_work(callback_workq, &process_callback_work, msecs_to_jiffies(20));

    PREPARE_DELAYED_WORK(&process_2ddma_callback_work,(void *)fscaler300_2ddma_callback_process);
    queue_delayed_work(callback_workq, &process_2ddma_callback_work, msecs_to_jiffies(20));

#endif
    if (dual_debug || flow_check) printm(MODULE_NAME,"%s out\n", __func__);
}

/*
 * return the job to videograph
 */
void fscaler300_2ddma_callback_process(void)
{
    job_node_t  *node, *ne, *curr, *ne1;
    struct video_entity_job_t *job, *child;
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
#if GM8210
    int ret = 0;
#endif

    fmem_get_identifier(&pci_id, &cpu_id);

    if (dual_debug || flow_check) printm(MODULE_NAME,"%s in\n", __func__);
    down(&global_info.sema_lock_2ddma);

    list_for_each_entry_safe(node, ne, &global_info.node_list_2ddma, plist) {
        if (node->status == JOB_STS_DONE || node->status == JOB_STS_FLUSH || node->status == JOB_STS_DONOTHING ||
            node->status == JOB_STS_FAIL || node->status == JOB_STS_MARK_SUCCESS) {
            job = (struct video_entity_job_t *)node->private;

            if (node->status == JOB_STS_DONE || node->status == JOB_STS_DONOTHING)
                job->status = JOB_STATUS_FINISH;
            else
                job->status = JOB_STATUS_FAIL;

            if (node->need_callback) {
#if GM8210
                // at dual 8210, trigger dma first, then callback
                // finish , trigger dma
                if (job->status == JOB_STATUS_FINISH) {
                    // set node's rc_node status as JOB_STS_TX, means done
                    if (node->rc_node) {
                        node->rc_node->status = JOB_STS_TX;

                        if (dual_debug == 1)
                            printm(MODULE_NAME, "ep node %u trig dma sn %d version %d diff %d\n", node->job_id, node->rc_node->serial_num, node->rc_node->property.vg_serial, (int)get_gm_jiffies() - (int)node->rc_node->timer);
                    }
                    /* if node has already trigger dma at irq, trigger dma again here is fine, will skip this time */
                    ret = scl_dma_trigger(node->job_id);
                    if (ret < 0) {
                        scl_err("[SL]  id %u trigger DMA fail in %s\n", node->job_id, __func__);
                        printm(MODULE_NAME, "[SL] trigger DMA fail\n");
                        damnit(MODULE_NAME);
                    }
                }

                if (job->status == JOB_STATUS_FAIL)
                    if (node->rc_node)
                        node->rc_node->status = JOB_STS_FAIL;

                if (node->status == JOB_STS_MARK_SUCCESS)
                    job->status = JOB_STATUS_FINISH;
#endif /* GM8210 */
                job->callback(node->private); //callback root job
                if (dual_debug || flow_check)printm(MODULE_NAME, "**%s, callback0 [%u]**\n", __func__, node->job_id);
                if (dual_debug == 1) {
                    if (job->status == JOB_STATUS_FINISH)
                        printm(MODULE_NAME, "callback id %u status finish\n", job->id);
                    if (job->status == JOB_STATUS_FAIL)
                        printm(MODULE_NAME, "callback id %u status fail\n", job->id);
                }
            }
            /* callback & free child node*/
            list_for_each_entry_safe(curr, ne1, &node->clist ,clist) {
                child = (struct video_entity_job_t *)curr->private;
                if (curr->status == JOB_STS_DONE || curr->status == JOB_STS_DONOTHING)
                    child->status = JOB_STATUS_FINISH;
                else
                    child->status = JOB_STATUS_FAIL;

                if (curr->need_callback) {
#if GM8210
                    // finish , trigger dma
                    if (job->status == JOB_STATUS_FINISH) {
                        ret = scl_dma_trigger(node->job_id);
                        if (ret < 0) {
                            scl_err("[SL] trigger DMA fail\n");
                            printm(MODULE_NAME, "[SL] trigger DMA fail\n");
                            damnit(MODULE_NAME);
                        }
                    }
#endif
                    job->callback(node->private); //callback root node
                    if (dual_debug || flow_check)printm(MODULE_NAME, "**%s, callback1 [%u]**\n", __func__, node->job_id);
                    if (dual_debug == 1) {
                        if (job->status == JOB_STATUS_FINISH)
                            printm(MODULE_NAME, "callback id %u status finish\n", job->id);
                        if (job->status == JOB_STATUS_FAIL)
                            printm(MODULE_NAME, "callback id %u status fail\n", job->id);
                    }
                }
                kmem_cache_free(global_info.vproperty_cache, curr->property.vproperty);
                MEMCNT_SUB(&priv, 1);
                kmem_cache_free(global_info.node_cache, curr);
                MEMCNT_SUB(&priv, 1);
            }
            /* free parent node */
            list_del(&node->plist);
            kmem_cache_free(global_info.vproperty_cache, node->property.vproperty);
            MEMCNT_SUB(&priv, 1);
            kmem_cache_free(global_info.node_cache, node);
            MEMCNT_SUB(&priv, 1);
        } else {
            break;
        }
    }

    up(&global_info.sema_lock_2ddma);
    /* go through the ep_rcnode_cb_list list
     */
#ifdef USE_KTHREAD
#if GM8210
    if ((pci_id != FMEM_PCI_HOST) && (cpu_id == FMEM_CPU_FA626)) {
	    if (dual_debug == 1) printm(MODULE_NAME, "+++++++ wake up tx\n");
	    eptx_wake_up();
    }
#endif
#endif
#ifdef USE_WQ
#if GM8210
    if (dual_debug == 1) printm(MODULE_NAME, "+++++++ wake up tx\n");
    PREPARE_DELAYED_WORK(&process_ep_work,(void *)fscaler300_ep_process);
    queue_delayed_work(callback_workq, &process_ep_work, msecs_to_jiffies(0));
#endif
    /* schedule the delay workq */
    if (dual_debug == 1) printm(MODULE_NAME, "waku up callback %d\n", __LINE__);
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)fscaler300_callback_process);
    queue_delayed_work(callback_workq, &process_callback_work, msecs_to_jiffies(20));

    PREPARE_DELAYED_WORK(&process_2ddma_callback_work,(void *)fscaler300_2ddma_callback_process);
    queue_delayed_work(callback_workq, &process_2ddma_callback_work, msecs_to_jiffies(20));
#endif
    if (dual_debug || flow_check) printm(MODULE_NAME,"%s out\n", __func__);
}

#ifdef USE_KTHREAD
int cb_running = 0;
static int cb_thread(void *__cwq)
{
    int status;
    cb_running = 1;

    do {
        status = wait_event_timeout(cb_thread_wait, cb_wakeup_event, msecs_to_jiffies(1000));

        cb_wakeup_event = 0;
        /* callback process */
        fscaler300_callback_process();
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

int cb_2ddma_running = 0;
static int cb_2ddma_thread(void *__cwq)
{
    int status;
    cb_2ddma_running = 1;

    do {
        status = wait_event_timeout(cb_2ddma_thread_wait, cb_2ddma_wakeup_event, msecs_to_jiffies(1000));

        cb_2ddma_wakeup_event = 0;
        /* callback process */
        fscaler300_2ddma_callback_process();
    } while(!kthread_should_stop());

    __set_current_state(TASK_RUNNING);
    cb_2ddma_running = 0;
    return 0;
}

static void callback_2ddma_wake_up(void)
{
    cb_2ddma_wakeup_event = 1;
    wake_up(&cb_2ddma_thread_wait);
}
#endif /* USE_KTHREAD */
/*
 * param: private data
 */
static int driver_set_out_property(void *param, struct video_entity_job_t *job)
{
    fscaler300_job_t   *job_item = (fscaler300_job_t *)param;

    /* assign id */
    job->out_prop[0].id = ID_SCL_RESULT;
    job->out_prop[0].value = 0;

    /* 0:do nothing  1:source from top  2:source from bottom */
    if (job_item->status == JOB_STS_DONOTHING)
        job->out_prop[0].value = 0; /* do nothing */
    else {
        if (job_item->param.ratio == 0)
            job->out_prop[0].value = 1; /* source from top */
        if (job_item->param.ratio == 1)
            job->out_prop[0].value = 2; /* source from bottom */
    }

    /* terminate */
    job->out_prop[1].id = ID_NULL;
    job->out_prop[1].value = 0;

    return 1;
}

/*
 * callback function for job finish. In ISR.
 */
static void driver_callback(fscaler300_job_t *job)
{
    job_node_t    *root, *node, *ne;

    root = (job_node_t *)job->input_node[0].private;

    driver_set_out_property(job, root->private);
    /* set root node status */
    root->status = job->status;
    if (flow_check) printm(MODULE_NAME, "%s id %u sts %x\n", __func__, job->job_id, root->status);
    /* set child node status */
    list_for_each_entry_safe(node, ne, &root->clist, clist)
        node->status = root->status;
#ifdef USE_KTHREAD
    if (dual_debug == 1) printm(MODULE_NAME, "waku up callback %d\n", __LINE__);
	callback_wake_up();
	callback_2ddma_wake_up();
#endif
#ifdef USE_WQ
    /* schedule the delay workq */
    if (dual_debug == 1) printm(MODULE_NAME, "waku up callback %d\n", __LINE__);
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)fscaler300_callback_process);
    queue_delayed_work(callback_workq, &process_callback_work, callback_period);

    PREPARE_DELAYED_WORK(&process_2ddma_callback_work,(void *)fscaler300_2ddma_callback_process);
    queue_delayed_work(callback_workq, &process_2ddma_callback_work, callback_period);
#endif
}

static void driver_update_status(fscaler300_job_t *job, int status)
{
    fscaler300_job_t *parent;
    job_node_t *node, *curr, *ne;

    parent = job->parent;
    if (parent == job) {
        node = (job_node_t *)parent->input_node[0].private;
        node->status = status;
        list_for_each_entry_safe(curr, ne, &node->clist, clist)
            curr->status = status;
    }

}

static int driver1_preprocess(fscaler300_job_t *job)
{
    int i, mpath, ret = 0;
    job_node_t  *node;

    if (job->job_type == MULTI_SJOB)
        mpath = 4;
    else
        mpath = 1;

    for (i = 0; i < mpath; i++) {
        if (job->input_node[i].enable == 1) {
            node = (job_node_t *)job->input_node[i].private;
            ret = video_preprocess(node->private, NULL);
        }
    }

    return ret;
}

static int driver_postprocess(fscaler300_job_t *job)
{
    int i, mpath, ret = 0;
    job_node_t  *node;

    if (job->job_type == MULTI_SJOB)
        mpath = 4;
    else
        mpath = 1;

    for (i = 0; i < mpath; i++) {
        if (job->input_node[i].enable == 1) {
            node = (job_node_t *)job->input_node[i].private;
            ret = video_postprocess(node->private, NULL);
        }
    }

    return ret;
}

void mark_engine_start(int dev)
{
    u32 base = 0;
    int cnt = 0;
    int dummy = 0;
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);

    if (engine_start[dev] != 0)
        printk("[SCALER]Warning to nested use dev%d mark_engine_start function!\n",dev);

    engine_start[dev] = jiffies;

    base = priv.engine[dev].fscaler300_reg;

    cnt = *(volatile unsigned int *)(base + 0x55d8);
    dummy = atomic_read(&priv.engine[dev].dummy_cnt);
    engine_end[dev] = 0;
    if (utilization_start[dev] == 0) {
        utilization_start[dev] = engine_start[dev];
        engine_time[dev] = 0;
        frame_cnt_start[dev] = cnt;
        dummy_cnt_start[dev] = dummy;
    }

    spin_unlock_irqrestore(&priv.lock, flags);
}

void mark_engine_finish(int dev)
{
    u32 base = 0;
    int dummy = 0;
    unsigned long flags;

    spin_lock_irqsave(&priv.lock, flags);

    if (engine_end[dev] != 0)
        printk("[SCALER]Warning to nested use dev%d mark_engine_finish function!\n",dev);

    engine_end[dev] = jiffies;

    if (engine_end[dev] > engine_start[dev])
        engine_time[dev] += engine_end[dev] - engine_start[dev];

    if (utilization_start[dev] > engine_end[dev]) {
        utilization_start[dev] = 0;
        engine_time[dev] = 0;
    } else if ((utilization_start[dev] <= engine_end[dev]) &&
        (engine_end[dev] - utilization_start[dev] >= utilization_period * HZ)) {
        unsigned int utilization;

        base = priv.engine[dev].fscaler300_reg;
        frame_cnt_end[dev] = *(volatile unsigned int *)(base + 0x55d8);
        dummy = atomic_read(&priv.engine[dev].dummy_cnt);

        utilization = (unsigned int)((100*engine_time[dev]) / (engine_end[dev] - utilization_start[dev]));
        if (utilization)
            utilization_record[dev] = utilization;
        utilization_start[dev] = 0;
        engine_time[dev] = 0;
        frame_cnt[dev] = (frame_cnt_end[dev] - frame_cnt_start[dev]);
        dummy_cnt[dev] = dummy - dummy_cnt_start[dev];

    }
    engine_start[dev]=0;
    spin_unlock_irqrestore(&priv.lock, flags);
}

static void driver_mark_engine_start(int dev, fscaler300_job_t *job_item)
{
    int i = 0, j = 0;
    int mpath = 0;
    job_node_t  *node = NULL;
    struct video_entity_job_t *job;
    struct video_property_t *property;
    int idx;

    if (job_item->job_type == MULTI_SJOB)
        mpath = 4;
    else
        mpath = 1;

    for (i = 0; i < mpath; i++) {
        if (!job_item->input_node[i].enable)
            continue;

        node = (job_node_t *)job_item->input_node[i].private;
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
}

static void driver_mark_engine_finish(int dev, fscaler300_job_t *job_item)
{
}

static int driver_flush_check(fscaler300_job_t *job_item)
{
    return 0;
}

/* callback functions called from the core
 */
struct f_ops callback_ops = {
    callback:           &driver_callback,
    update_status:      &driver_update_status,
    pre_process:        &driver1_preprocess,
    //pre_process:        &driver_preprocess,
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
    // at dual 8210, RC job has already add to node_list at driver_put_job()
    if (!I_AM_RC(node->property.vproperty[node->ch_id].scl_feature) &&
        !I_AM_EP(node->property.vproperty[node->ch_id].scl_feature)) {
        list_add_tail(&node->plist, &global_info.node_list);
    }
}

int sanity_check_pip(job_node_t *node)
{
    int ret = 0;
    int i = 0;
    fscaler300_property_t *param = &node->property;

    if (param->pip.win_cnt > PIP_COUNT) {
        scl_err("Error! pip window count %d is over max %d\n", param->pip.win_cnt, PIP_COUNT);
        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window count %d is over max %d\n", param->pip.win_cnt, PIP_COUNT);
        ret = -1;
    }

    for (i = 0; i < param->pip.win_cnt; i++) {
        if (param->pip.src_frm[i] == 0) {
            scl_err("Error! pip window %d source frame type is 0\n", i);
            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d source frame type is 0\n", i);
            ret = -1;
        }

        if (param->pip.dst_frm[i] == 0) {
            scl_err("Error! pip window %d destination frame type is 0\n", i);
            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d destination frame type is 0\n", i);
            ret = -1;
        }

        if (param->pip.src_width[i] % 4 != 0) {
            scl_err("Error! pip window %d source width not 4 alignment\n", i);
            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d source width [%d] not 4 alignment\n", i, param->pip.src_width[i]);
            ret = -1;
        }

        if (param->pip.src_width[i] == 0) {
            scl_err("Error! pip window %d source width can't be 0\n", i);
            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d source width can't be 0\n", i);
            ret = -1;
        }

        if (param->pip.src_height[i] == 0) {
            scl_err("Error! pip window %d source height can't be 0\n", i);
            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d source height can't be 0\n", i);
            ret = -1;
        }

        if (param->pip.dst_width[i] % 4 != 0) {
            scl_err("Error! pip window %d destination width not 4 alignment\n", i);
            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d destination width [%d] not 4 alignment\n", i, param->pip.dst_width[i]);
            ret = -1;
        }

        if ((param->pip.scl_info[i] != 0) && (param->pip.scl_info[i] != 0x1)) {
            scl_err("Error! pip window %d scl_info %d not allowed\n", i, param->pip.scl_info[i]);
            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d scl_info [%d] not allowed\n", i, param->pip.scl_info[i]);
            ret = -1;
        }

        // copy whole, vg do not set src x/y, dst x/y, dst width/height
        if (param->pip.scl_info[i] == 0x1) {
            param->pip.src_x[i] = 0;
            param->pip.src_y[i] = 0;
            param->pip.dst_x[i] = 0;
            param->pip.dst_y[i] = 0;
            param->pip.dst_width[i] = param->pip.src_width[i];
            param->pip.dst_height[i] = param->pip.src_height[i];
        }

        if (param->pip.src_x[i] % 4 != 0) {
            scl_err("Error! pip window %d source x position not 4 alignment\n", i);
            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d source x [%d] position not 4 alignment\n", i, param->pip.src_x[i]);
            ret = -1;
        }

        if (param->pip.dst_x[i] % 4 != 0) {
            scl_err("Error! pip window %d destination x position not 4 alignment\n", i);
            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d destination x [%d] position not 4 alignment\n", i, param->pip.src_x[i]);
            ret = -1;
        }

        if (param->pip.dst_width[i] == 0) {
            scl_err("Error! pip window %d destination width not 4 alignment\n", i);
            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d destination height can't be 0\n", i);
            ret = -1;
        }

        if (param->pip.dst_height[i] == 0) {
            scl_err("Error! pip window %d destination height can't be 0\n", i);
            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d destination height can't be 0\n", i);
            ret = -1;
        }

        // source from FRM1 or FRM2
        if ((param->pip.src_frm[i] == 0x1) || (param->pip.src_frm[i] == 0x2)) {
            if (param->pip.src_x[i] + param->pip.src_width[i] > param->src_bg_width) {
                scl_err("Error! pip window %d x + source crop width over source background width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d x + source crop width over source width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "src_x %d src_width [%d] src_bg_width [%d]\n", param->pip.src_x[i], param->pip.src_width[i], param->src_bg_width);
                ret = -1;
            }

            if (param->pip.src_y[i] + param->pip.src_height[i] > param->src_bg_height) {
                scl_err("Error! pip window %d y + source crop height over source background height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d y + source crop height over source height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "src_y %d src_height [%d] src_bg_height [%d]\n", param->pip.src_y[i], param->pip.src_height[i], param->src_bg_height);
                ret = -1;
            }
        }
        // source from FRM3 or FRM4
        if ((param->pip.src_frm[i] == 0x4) || (param->pip.src_frm[i] == 0x8)) {
            if (param->pip.src_x[i] + param->pip.src_width[i] > param->src2_bg_width) {
                scl_err("Error! pip window %d x + source crop width over source background width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d x + source crop width over source width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "src_x %d src_width [%d] src_bg_width [%d]\n", param->pip.src_x[i], param->pip.src_width[i], param->src2_bg_width);
                ret = -1;
            }

            if (param->pip.src_y[i] + param->pip.src_height[i] > param->src2_bg_height) {
                scl_err("Error! pip window %d y + source crop height over source background height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d y + source crop height over source height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "src_y %d src_height [%d] src_bg_height [%d]\n", param->pip.src_y[i], param->pip.src_height[i], param->src2_bg_height);
                ret = -1;
            }
        }
        // source from BUF1
        if (param->pip.src_frm[i] == 0x11) {
            if (param->pip.src_x[i] + param->pip.src_width[i] > temp_width) {
                scl_err("Error! pip window %d x + source crop width over source background width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d x + source crop width over source width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "src_x %d src_width [%d] src_bg_width [%d]\n", param->pip.src_x[i], param->pip.src_width[i], temp_width);
                ret = -1;
            }

            if (param->pip.src_y[i] + param->pip.src_height[i] > temp_height) {
                scl_err("Error! pip window %d y + source crop height over source background height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d y + source crop height over source height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "src_y %d src_height [%d] src_bg_height [%d]\n", param->pip.src_y[i], param->pip.src_height[i], temp_height);
                ret = -1;
            }
        }
        // source from BUF2
        if (param->pip.src_frm[i] == 0x12) {
            if (param->pip.src_x[i] + param->pip.src_width[i] > pip1_width) {
                scl_err("Error! pip window %d x + source crop width over source background width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d x + source crop width over source width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "src_x %d src_width [%d] src_bg_width [%d]\n", param->pip.src_x[i], param->pip.src_width[i], pip1_width);
                ret = -1;
            }

            if (param->pip.src_y[i] + param->pip.src_height[i] > pip1_height) {
                scl_err("Error! pip window %d y + source crop height over source background height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d y + source crop height over source height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "src_y %d src_height [%d] src_bg_height [%d]\n", param->pip.src_y[i], param->pip.src_height[i], pip1_height);
                ret = -1;
            }
        }
        // destination to FRM1 or FRM2
        if ((param->pip.dst_frm[i] == 0x1) || (param->pip.dst_frm[i] == 0x2)) {
            if (param->pip.dst_x[i] + param->pip.dst_width[i] > param->src_bg_width) {
                scl_err("Error! pip window %d x + destination crop width over source background width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d x + destination crop width over destination background width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "dst_x %d dst_width [%d] dst_bg_width [%d]\n", param->pip.dst_x[i], param->pip.dst_width[i], param->src_bg_width);
                ret = -1;
            }
            if (param->pip.dst_y[i] + param->pip.dst_height[i] > param->src_bg_height) {
                scl_err("Error! pip window %d y + destination crop height over destination background height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d y + destination crop height over destination background height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "dst_y %d dst_height [%d] dst_bg_height [%d]\n", param->pip.dst_y[i], param->pip.dst_height[i], param->src_bg_height);
                ret = -1;
            }
        }
        // destination to FRM3 or FRM4
        if ((param->pip.dst_frm[i] == 0x4) || (param->pip.dst_frm[i] == 0x8)) {
            if (param->pip.dst_x[i] + param->pip.dst_width[i] > param->src2_bg_width) {
                scl_err("Error! pip window %d x + destination crop width over destination background width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d x + destination crop width over destination background width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "dst_x %d dst_width [%d] dst_bg_width [%d]\n", param->pip.dst_x[i], param->pip.dst_width[i], param->src2_bg_width);
                ret = -1;
            }
            if (param->pip.dst_y[i] + param->pip.dst_height[i] > param->src2_bg_height) {
                scl_err("Error! pip window %d y + destination crop height over destination background height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d y + destination crop height over destination height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "dst_y %d dst_height [%d] dst_bg_height [%d]\n", param->pip.dst_y[i], param->pip.dst_height[i], param->src2_bg_height);
                ret = -1;
            }
        }
        // destination to BUF1
        if (param->pip.dst_frm[i] == 0x11) {
            if (param->pip.dst_x[i] + param->pip.dst_width[i] > temp_width) {
                scl_err("Error! pip window %d x + destination crop width over destination background width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d x + destination crop width over destination background width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "dst_x %d dst_width [%d] dst_bg_width [%d]\n", param->pip.dst_x[i], param->pip.dst_width[i], temp_width);
                ret = -1;
            }

            if (param->pip.dst_y[i] + param->pip.dst_height[i] > temp_height) {
                scl_err("Error! pip window %d y + destination crop height over destination background height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d y + destination crop height over destination background height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "dst_y %d dst_height [%d] dst_bg_height [%d]\n", param->pip.dst_y[i], param->pip.dst_height[i], temp_height);
                ret = -1;
            }
        }
        // destination to BUF2
        if (param->pip.dst_frm[i] == 0x12) {
            if (param->pip.dst_x[i] + param->pip.dst_width[i] > pip1_width) {
                scl_err("Error! pip window %d x + destination crop width over destination background width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d x + destination crop width over destination background width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "dst_x %d dst_width [%d] dst_bg_width [%d]\n", param->pip.dst_x[i], param->pip.dst_width[i], pip1_width);
                ret = -1;
            }

            if (param->pip.dst_y[i] + param->pip.dst_height[i] > pip1_height) {
                scl_err("Error! pip window %d y + destination crop height over destination background height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip window %d y + destination crop height over destination background height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "dst_y %d dst_height [%d] dst_bg_height [%d]\n", param->pip.dst_y[i], param->pip.dst_height[i], pip1_height);
                ret = -1;
            }
        }
    }

    return ret;
}

/*
 * do basic parameters check
 */
int sanity_check(job_node_t *node)
{
    int ret = 0;
    int i, j = 0;
    fscaler300_property_t *param = &node->property;
    struct video_entity_job_t *job;
    struct video_property_t *property;
    int idx;
    int pip_flag = 0;

    for (i = 0; i <= virtual_chn_num; i++) {
        if (param->vproperty[i].vch_enable) {
            if (param->vproperty[i].src_width % 4 != 0) {
                scl_err("Error! source width not 4 alignment\n");
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source width [%d] not 4 alignment\n", i, param->vproperty[i].src_width);
                ret = -1;
            }

            if (param->vproperty[i].src2_width % 4 != 0) {
                scl_err("Error! source2 width not 4 alignment\n");
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source2 width [%d] not 4 alignment\n", i, param->vproperty[i].src2_width);
                ret = -1;
            }

            if (param->vproperty[i].src_width == 0) {
                scl_err("Error! source width can't be 0\n");
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source width can't be 0\n", i);
                ret = -1;
            }

            if (param->vproperty[i].src_height == 0) {
                scl_err("Error! source height can't be 0\n");
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source height can't be 0\n", i);
                ret = -1;
            }

            if (param->vproperty[i].src_width > SCALER300_MAX_RESOLUTION) {
                scl_err("Error! source width can't over %d\n", SCALER300_MAX_RESOLUTION);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! source width can't over %d\n", SCALER300_MAX_RESOLUTION);
                ret = -1;
            }

            if (param->vproperty[i].src_height > SCALER300_MAX_RESOLUTION) {
                scl_err("Error! source height can't over %d\n", SCALER300_MAX_RESOLUTION);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! source height can't over %d\n", SCALER300_MAX_RESOLUTION);
                ret = -1;
            }

            if (param->vproperty[i].src_x % 4 != 0) {
                scl_err("Error! source x position not 4 alignment\n");
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] src x [%d] not 4 alignment\n", i, param->vproperty[i].src_x);
                ret = -1;
            }

            if (param->vproperty[i].src_x + param->vproperty[i].src_width > param->src_bg_width) {
                scl_err("Error! x + source crop width over source width\n");
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] x + source crop width over source width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "src_x [%d] src_width [%d] src_bg_width [%d]\n", param->vproperty[i].src_x, param->vproperty[i].src_width, param->src_bg_width);
                master_print("Error! ch [%d] x + source crop width over source width\n", i);
                master_print("src_x [%d] src_width [%d] src_bg_width [%d]\n", param->vproperty[i].src_x, param->vproperty[i].src_width, param->src_bg_width);
                param->vproperty[i].src_x = 0;
                param->vproperty[i].src_width = param->src_bg_width;
                //ret = -1;
            }

            if (param->vproperty[i].src2_x + param->vproperty[i].src2_width > param->src2_bg_width) {
                scl_err("Error! source2 crop width over source width\n");
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] source2 crop width over source2 width\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "src2_x [%d] src2_width [%d] src2_bg_width [%d]\n", param->vproperty[i].src_x, param->vproperty[i].src2_width, param->src2_bg_width);
                ret = -1;
            }

            if (param->vproperty[i].src_y + param->vproperty[i].src_height > param->src_bg_height) {
                scl_err("Error! y + source crop height over source height\n");
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] y + source crop height over source height\n", i);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "src_y [%d] src_height [%d] src_bg_height [%d]\n", param->vproperty[i].src_y, param->vproperty[i].src_height, param->src_bg_height);
                master_print("Error! ch [%d] y + source crop height over source height\n", i);
                master_print("src_y [%d] src_height [%d] src_bg_height [%d]\n", param->vproperty[i].src_y, param->vproperty[i].src_height, param->src_bg_height);
                //ret = -1;
                param->vproperty[i].src_y = 0;
                param->vproperty[i].src_height = param->src_bg_height;
            }

            if (param->vproperty[i].aspect_ratio.pal_idx >= SCALER300_PALETTE_MAX) {
                printk("[SCL_WARN] palette idx %d over MAX %d\n", param->vproperty[i].aspect_ratio.pal_idx, SCALER300_PALETTE_MAX);
                param->vproperty[i].aspect_ratio.pal_idx = 0;
            }

            if (param->vproperty[i].border.pal_idx >= SCALER300_PALETTE_MAX) {
                printk("[SCL_WARN] palette idx %d over MAX %d\n", param->vproperty[i].border.pal_idx, SCALER300_PALETTE_MAX);
                param->vproperty[i].border.pal_idx = 0;
            }

            if (param->src_fmt != TYPE_YUV422_RATIO_FRAME && param->src_fmt != TYPE_YUV422_FRAME) {
                if ((param->dst2_bg_width != 0) && (param->dst2_bg_height != 0)) {
                    if (param->src2_bg_width != param->dst2_bg_width) {
                        if ((param->src2_bg_width - param->dst2_bg_width) < 8) {
                            scl_err("Error! CVBS source2_bg_width  - dst2_bg_width must > 8 \n");
                            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! CVBS source2_bg_width  - dst2_bg_width must > 8\n", i);
                            DEBUG_M(LOG_ERROR, node->engine, node->minor, "src2_bg_width [%d] dst2_bg_width [%d]\n", param->src2_bg_width, param->dst2_bg_width);
                            printk("src2_bg_width [%d] dst2_bg_width [%d] \n", param->src2_bg_width, param->dst2_bg_width);
                            ret = -1;
                        }
                    }
                    if (param->src2_bg_height != param->dst2_bg_height) {
                        if ((param->src2_bg_height - param->dst2_bg_height) < 8) {
                            scl_err("Error! CVBS source2_bg_height  - dst2_bg_height must > 8 \n");
                            DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! CVBS source2_bg_height  - dst2_bg_height must > 8\n", i);
                            DEBUG_M(LOG_ERROR, node->engine, node->minor, "src2_bg_height [%d] dst2_bg_height [%d]\n", param->src2_bg_height, param->dst2_bg_height);
                            printk("src2_bg_height [%d] dst2_bg_height [%d] \n", param->src2_bg_height, param->dst2_bg_height);
                            ret = -1;
                        }
                    }
                }
            }
            /* line correct path, do scaling */
            if ((param->vproperty[i].scl_feature & (1 << 0))) {
                if (param->vproperty[i].dst_width % 4 != 0) {
                    scl_err("Error! destination width not 4 alignment\n");
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] destination width [%d] not 4 alignment\n", i, param->vproperty[i].dst_width);
                    ret = -1;
                }

                if (param->vproperty[i].dst2_width % 4 != 0) {
                    scl_err("Error! destination2 width not 4 alignment\n");
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] destination2 width [%d] not 4 alignment\n", i, param->vproperty[i].dst2_width);
                    ret = -1;
                }

                if (param->vproperty[i].dst_width == 0) {
                    scl_err("Error! destination width can't be 0\n");
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] destination width can't be 0\n", i);
                    ret = -1;
                }

                if (param->vproperty[i].dst_height == 0) {
                    scl_err("Error! destination height can't be 0\n");
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] destination height can't be 0\n", i);
                    ret = -1;
                }

                if (param->vproperty[i].dst_width > SCALER300_MAX_RESOLUTION) {
                    scl_err("Error! destination width can't over %d\n", SCALER300_MAX_RESOLUTION);
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! destination width can't over %d\n", SCALER300_MAX_RESOLUTION);
                    ret = -1;
                }

                if (param->vproperty[i].dst_height > SCALER300_MAX_RESOLUTION) {
                    scl_err("Error! destination height can't over %d\n", SCALER300_MAX_RESOLUTION);
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! destination height can't over %d\n", SCALER300_MAX_RESOLUTION);
                    ret = -1;
                }

                if (param->vproperty[i].dst_x % 4 != 0) {
                    scl_err("Error! destination x position not 4 alignment\n");
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] dst x [%d] not 4 alignment\n", i, param->vproperty[i].dst_x);
                    ret = -1;
                }

                if (param->vproperty[i].dst_x + param->vproperty[i].dst_width > param->dst_bg_width) {
                    scl_err("Error! destination frame width over destination background width\n");
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] destination frame width over destination background width\n", i);
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "dst_x [%d] dst_width [%d] dst_bg_width [%d]\n", node->property.vproperty[i].dst_x, node->property.vproperty[i].dst_width, param->dst_bg_width);
                    ret = -1;
                }

                if (node->property.vproperty[i].dst_y + node->property.vproperty[i].dst_height > param->dst_bg_height) {
                    scl_err("Error! destination frame height over destination background height\n");
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] destination frame height over destination background height\n", i);
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "dst_y [%d] dst_height [%d] dst_bg_height [%d]\n", node->property.vproperty[i].dst_y, node->property.vproperty[i].dst_height, param->dst_bg_height);
                    ret = -1;
                }
            }
            /* do cvbs zoom */
            if ((param->vproperty[i].scl_feature & (1 << 2))) {
                if (param->win2_bg_width == 0) {
                    scl_err("Error! win2 bg width can't be 0\n");
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] win2 bg width [%d] can't be 0\n", i, param->win2_bg_width);
                    ret = -1;
                }

                if (param->win2_bg_height == 0) {
                    scl_err("Error! win2 bg height can't be 0\n");
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] win2 bg height [%d] can't be 0\n", i, param->win2_bg_height);
                    ret = -1;
                }

                if (param->win2_bg_width % 4 != 0) {
                    scl_err("Error! win2 bg width not 4 alignment\n");
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! ch [%d] win2 bg width [%d] not 4 alignment\n", i, param->win2_bg_width);
                    ret = -1;
                }

                if ((param->win2_x + param->win2_bg_width) > SCALER300_MAX_RESOLUTION) {
                    scl_err("Error! win2 bg width can't over %d\n", SCALER300_MAX_RESOLUTION);
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! win2 bg width can't over %d\n", SCALER300_MAX_RESOLUTION);
                    ret = -1;
                }

                if ((param->win2_y + param->win2_bg_height) > SCALER300_MAX_RESOLUTION) {
                    scl_err("Error! win2 bg height can't over %d\n", SCALER300_MAX_RESOLUTION);
                    DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! win2 bg height can't over %d\n", SCALER300_MAX_RESOLUTION);
                    ret = -1;
                }
            }
            /* set dma parameter for EP */
            if ((param->vproperty[i].scl_feature & (1 << 5))) {
                param->vproperty[i].scl_dma.job_id = node->job_id;
                param->vproperty[i].scl_dma.src_addr = node->property.in_addr_pa;
                param->vproperty[i].scl_dma.dst_addr = node->property.pcie_addr;
                // with scaling, set dst width/height as dma destination width/height
                if ((param->vproperty[i].scl_feature & (1 << 0))) {
                    param->vproperty[i].scl_dma.x_offset = param->vproperty[i].dst_x;
                    param->vproperty[i].scl_dma.y_offset = param->vproperty[i].dst_y;
                    param->vproperty[i].scl_dma.width = param->vproperty[i].dst_width;
                    param->vproperty[i].scl_dma.height = param->vproperty[i].dst_height;
                    param->vproperty[i].scl_dma.bg_width = param->dst_bg_width;
                    param->vproperty[i].scl_dma.bg_height = param->dst_bg_height;
                }
                // without scaling, set src width/height as dma destination width/height
                else {
                    param->vproperty[i].scl_dma.x_offset = param->vproperty[i].src_x;
                    param->vproperty[i].scl_dma.y_offset = param->vproperty[i].src_y;
                    param->vproperty[i].scl_dma.width = param->vproperty[i].src_width;
                    param->vproperty[i].scl_dma.height = param->vproperty[i].src_height;
                    param->vproperty[i].scl_dma.bg_width = param->src_bg_width;
                    param->vproperty[i].scl_dma.bg_height = param->src_bg_height;
                }
            }

            /* do pip */
            if (pip_flag == 0) {
                if ((param->vproperty[i].scl_feature & (1 << 3))) {
                    if (param->pip.win_cnt == 0) {
                        scl_err("Error! pip1 window count can't 0\n");
                        DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! pip1 window count can't be 0\n");
                        ret = -1;
                    }

                    if (sanity_check_pip(node) < 0)
                        ret = -1;
                }
                pip_flag = 1;
            }
        }
    }

    if (ret == 0) {
        if ((param->src_fmt == TYPE_YUV422_FRAME && param->dst_fmt == TYPE_YUV422_FRAME) ||
            (param->src_fmt == TYPE_YUV422_FRAME && param->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
            (param->src_fmt == TYPE_YUV422_RATIO_FRAME && param->dst_fmt == TYPE_YUV422_2FRAMES) ||
            (param->src_fmt == TYPE_YUV422_RATIO_FRAME && param->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
            (param->src_fmt == TYPE_YUV422_2FRAMES && param->dst_fmt == TYPE_YUV422_2FRAMES) ||
            (param->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && param->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
            (param->src_fmt == TYPE_YUV422_FRAME && param->dst_fmt == TYPE_RGB565) ||
            (param->src_fmt == TYPE_YUV422_FRAME_2FRAMES && param->dst_fmt == TYPE_YUV422_FRAME_2FRAMES) ||
            (param->src_fmt == TYPE_YUV422_RATIO_FRAME && param->dst_fmt == TYPE_YUV422_FRAME_2FRAMES) ||
            (param->src_fmt == TYPE_YUV422_FIELDS && param->dst_fmt == TYPE_YUV422_FRAME) ||
            (param->src_fmt == TYPE_RGB565 && param->dst_fmt == TYPE_RGB565) ||
            (param->src_fmt == TYPE_YUV422_FRAME && param->dst_fmt == TYPE_YUV422_FRAME_2FRAMES) ||
            (param->src_fmt == TYPE_YUV422_RATIO_FRAME && param->dst_fmt == TYPE_YUV422_FRAME_FRAME) ||
            (param->src_fmt == TYPE_YUV422_FRAME && param->dst_fmt == TYPE_YUV422_FRAME_FRAME) ||
            (param->src_fmt == TYPE_YUV422_FRAME_FRAME && param->dst_fmt == TYPE_YUV422_FRAME_FRAME) ||
            (param->src_fmt == TYPE_YUV422_FRAME && param->dst_fmt == TYPE_YUV422_CASCADE_SCALING_FRAME) ||
            (param->src_fmt == TYPE_YUV422_RATIO_FRAME && param->dst_fmt == TYPE_YUV422_FRAME) ||
            (param->src_fmt == TYPE_YUV422_FRAME && param->dst_fmt == TYPE_YUV422_2FRAMES) ||
            (param->src_fmt == TYPE_RGB555 && param->dst_fmt == TYPE_RGB555))
            ret = 0;
        else {
            if ((param->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES ||
                 param->src_fmt == TYPE_YUV422_FRAME_2FRAMES ||
                 param->src_fmt == TYPE_YUV422_FRAME_FRAME) &&
                 param->dst_fmt == TYPE_YUV422_FRAME) {
                //can support by changing src_fmt to TYPE_YUV422_FRAME!
            } else {
                scl_err("Error! wrong src/dst format combination, src=[%x], dst=[%x]\n", param->src_fmt, param->dst_fmt);
                DEBUG_M(LOG_ERROR, node->engine, node->minor, "Error! wrong src/dst format combination, src=[%x], dst=[%x]\n", param->src_fmt, param->dst_fmt);
                ret = -1;
            }
        }
    }

    if (ret == -1) {
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
    return ret;
}

int fscaler300_select_ratio_source(job_node_t *node, int ch_id)
{
    int src_sum0 = 0, src_sum1 = 0, dst_sum = 0, ratio = 0;
    fscaler300_property_t *property = &node->property;

    if (property->sub_yuv == 1) {
        src_sum0 = property->vproperty[ch_id].src_width * property->vproperty[ch_id].src_height << 1;            // source top frame's area
        src_sum1 = (property->vproperty[ch_id].src_width >> 1) * (property->vproperty[ch_id].src_height >> 1) << 1;    // source bottom frame's area
        dst_sum = property->vproperty[ch_id].dst_width * property->vproperty[ch_id].dst_height << 1;                   // destination frame's area
        src_sum0 = dst_sum - src_sum0 + property->ratio;        // destination frame - source top frame's area + ratio value from proc "ratio"
        src_sum1 = dst_sum - src_sum1 + property->ratio;        // destination frame - source bottom frame's area + ratio value from proc "ratio"

        /* get absolute value of src_sum0 & src_sum1,
           compare src_sum0 & src_sum1, set ratio to smaller one */
        if (abs(src_sum1) < abs(src_sum0))
            ratio = 1;  // source from bottom frame
        else
            ratio = 0;  // source from top frame
    }
    else
        ratio = 0;  // source from top frame

    return ratio;
}

int fscaler300_set_cvbs_border_width(job_node_t *node, int ch_id)
{
    int border_width;
    fscaler300_property_t *property = &node->property;

    border_width = property->vproperty[ch_id].border.width;

    if (border_width == 0)
        return 0;

    if ((property->vproperty[ch_id].dst_width >= (property->vproperty[ch_id].dst2_width << 1)) ||
        (property->vproperty[ch_id].dst_height >= (property->vproperty[ch_id].dst2_height << 1))) {
        if (border_width >= 1)
            border_width--;
    }

    return border_width;
}

/*
 * use fix pattern to draw vertical border when border enable
 */
int fscaler300_set_border_v_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    u32 crcby = 0;
    int i, pal_idx = 0;
    fscaler300_property_t *property = &node->property;
    int border_width, fix_width, cvbs_border_width = 0;
    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    border_width = (property->vproperty[ch_id].border.width + 1) << 2;

    /* border width must be 4 align, if fix pattern width < 16, image has something wrong --> osd issue
       sw workaround method : fix pattern width must >= 16,
       if ap border width < 16, use scaler target cropping, crop ap border width */
    if (border_width < 16)
        fix_width = 16;
    else
        fix_width = border_width;

    job->param.src_width = fix_width;
    /* use frm2field to draw top/bottom frame's left/right border
       src is ratio frame, border will jump line, so height = border width
       src is frame, border will not jump line, so height = border width * 2
    */
    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))
        job->param.src_height = property->vproperty[ch_id].dst_height << 1;
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES)
        job->param.src_height = property->vproperty[ch_id].dst_height;
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.src_height = property->vproperty[ch_id].dst_height;
    if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.src_height = property->vproperty[ch_id].dst_height << 1;

    job->param.src_bg_width = job->param.src_width;
    job->param.src_bg_height = job->param.src_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.src_x = 0;
    job->param.src_y = 0;
    /* to meet 8287 & 8219 path 2&3 only bypass, use path 2/3 bypass to draw border*/
    // path 2 draw top/bottom frame's left border
    job->param.dst_width[2] = job->param.src_width;
    job->param.dst_height[2] = job->param.src_height;
    job->param.dst_x[2] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[2] = property->vproperty[ch_id].dst_y;
    // path 3 draw top/bottom frame's right border
    job->param.dst_width[3] = job->param.src_width;
    job->param.dst_height[3] = job->param.src_height;
    job->param.dst_x[3] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].dst_width - border_width;
    job->param.dst_y[3] = property->vproperty[ch_id].dst_y;

    // ratio to 3fram/frame to 3frame, use path 0/1 scaling down to draw CVBS frame left/right border
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (property->vproperty[ch_id].dst2_width != 0 && property->vproperty[ch_id].dst2_height != 0) {
            cvbs_border_width = fscaler300_set_cvbs_border_width(node, ch_id);
            cvbs_border_width = (cvbs_border_width + 1) << 2;
            // path 0 draw cvbs frame left border
            job->param.dst_width[0] = job->param.src_width;
            job->param.dst_height[0] = property->vproperty[ch_id].dst2_height;
            job->param.dst_x[0] = property->vproperty[ch_id].dst2_x;
            job->param.dst_y[0] = property->vproperty[ch_id].dst2_y;
            // path 1 draw cvbs frame right border
            job->param.dst_width[1] = job->param.src_width;
            job->param.dst_height[1] = property->vproperty[ch_id].dst2_height;
            job->param.dst_x[1] = property->vproperty[ch_id].dst2_x + property->vproperty[ch_id].dst2_width - cvbs_border_width;
            job->param.dst_y[1] = property->vproperty[ch_id].dst2_y;
        }
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;

    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        job->param.dest_dma[2].field_offset = job->param.dest_dma[3].field_offset =
        ((property->dst_bg_width * property->dst_bg_height) << 1) >> 5;
        if ((property->dst_bg_width * property->dst_bg_height << 1) % 32 != 0) {
            scl_err("field offset not 32's multiple\n");
            return -1;
        }
    }

    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES) {
        job->param.dest_dma[2].field_offset = job->param.dest_dma[3].field_offset =
        ((property->dst_bg_width * property->dst_bg_height + property->dst_bg_width) << 1) >> 5;
        if (((property->dst_bg_width * property->dst_bg_height + property->dst_bg_width) << 1) % 32 != 0) {
            scl_err("field offset not 32's multiple\n");
            return -1;
        }
    }

    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        job->param.dest_dma[2].field_offset = job->param.dest_dma[3].field_offset =
        ((property->dst_bg_width * property->dst_bg_height + property->dst_bg_width) << 1) >> 5;
        if (((property->dst_bg_width * property->dst_bg_height + property->dst_bg_width) << 1) % 32 != 0) {
            scl_err("field offset not 32's multiple\n");
            return -1;
        }
    }

    if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        job->param.dest_dma[2].field_offset = job->param.dest_dma[3].field_offset =
        ((property->dst_bg_width * property->dst_bg_height) << 1) >> 5;
        if ((property->dst_bg_width * property->dst_bg_height << 1) % 32 != 0) {
            scl_err("field offset not 32's multiple\n");
            return -1;
        }
    }

    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "border width %d fix width %d\n", border_width, fix_width);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width 2[%d]\n", job->param.dst_width[2]);
        printm(MODULE_NAME, "dst_height 2[%d]\n", job->param.dst_height[2]);
        printm(MODULE_NAME, "dst_x 2[%d]\n", job->param.dst_x[2]);
        printm(MODULE_NAME, "dst_y 2[%d]\n", job->param.dst_y[2]);
        printm(MODULE_NAME, "dst_width 3[%d]\n", job->param.dst_width[3]);
        printm(MODULE_NAME, "dst_height 3[%d]\n", job->param.dst_height[3]);
        printm(MODULE_NAME, "dst_x 3[%d]\n", job->param.dst_x[3]);
        printm(MODULE_NAME, "dst_y 3[%d]\n", job->param.dst_y[3]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "pal idx [%d]\n", property->vproperty[ch_id].border.pal_idx);
        printm(MODULE_NAME, "out addr pa [%d]\n", job->param.out_addr_pa[0]);
        printm(MODULE_NAME, "out addr pa 1[%d]\n", job->param.out_addr_pa[1]);
        printm(MODULE_NAME, "dst 2 dma field offset [%d]\n", job->param.dest_dma[2].field_offset);
        printm(MODULE_NAME, "dst 3 dma field offset [%d]\n", job->param.dest_dma[3].field_offset);
        printm(MODULE_NAME, "ratio to 3 frame\n");
        printm(MODULE_NAME, "dst_width 0[%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height 0[%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x 0[%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y 0[%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_width 1[%d]\n", job->param.dst_width[1]);
        printm(MODULE_NAME, "dst_height 1[%d]\n", job->param.dst_height[1]);
        printm(MODULE_NAME, "dst_x 1[%d]\n", job->param.dst_x[1]);
        printm(MODULE_NAME, "dst_y 1[%d]\n", job->param.dst_y[1]);
        printm(MODULE_NAME, "cvbs_border_width [%d]\n", cvbs_border_width);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
    job->param.global.sca_src_format = YUV422;
    job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    //path = 2;
    // bypass, path 2 & 3 for bypass, due to 8287 & 8129 path 0/1 can scaling, path 2/3 only bypass
    for (i = 2; i < 4; i++) {
        job->param.sd[i].bypass = 1;
        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 1;
    }
    job->param.frm2field.res2_frm2field = 1;
    job->param.frm2field.res3_frm2field = 1;

    // ratio to 3 frame, use path 0/1 scaling down to CVBS frame height
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (property->vproperty[ch_id].dst2_width != 0 && property->vproperty[ch_id].dst2_height != 0) {
            job->param.sd[0].bypass = job->param.sd[1].bypass = 0;
            job->param.sd[0].enable = job->param.sd[1].enable = 1;
            job->param.tc[0].enable = job->param.tc[1].enable = 1;
        }
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    pal_idx = property->vproperty[ch_id].border.pal_idx;
    crcby = priv.global_param.init.palette[pal_idx].crcby;
    job->param.sc.fix_pat_en = 1;
    job->param.sc.fix_pat_cr = (crcby >> 16);
    job->param.sc.fix_pat_cb = ((crcby & 0xff00) >> 8);
    job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (crcby & 0xff);

    /*================= SD =================*/
    for (i = 2; i < 4; i++) {
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = 256;
        job->param.sd[i].vstep = 256;
    }

    /*================= TC =================*/
    for (i = 2; i < 4; i++) {
        job->param.tc[i].width = border_width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = border_width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
    }

    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (property->vproperty[ch_id].dst2_width != 0 && property->vproperty[ch_id].dst2_height != 0) {
            for (i = 0; i < 2; i++) {
                if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
                    scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
                    return -1;
                }
                // width/height
                job->param.sd[i].width = job->param.dst_width[i];
                job->param.sd[i].height = job->param.dst_height[i];
                // hstep/vstep
                job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
                job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);
            }
            /*================= TC =================*/
            for (i = 0; i < 2; i++) {
                job->param.tc[i].width = cvbs_border_width;
                job->param.tc[i].height = job->param.sd[i].height;
                job->param.tc[i].x_start = 0;
                job->param.tc[i].x_end = cvbs_border_width - 1;
                job->param.tc[i].y_start = 0;
                job->param.tc[i].y_end = job->param.sd[i].height - 1;
                /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
                scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
                if ((job->param.src_width != job->param.dst_width[i]) ||
                    (job->param.src_height != job->param.dst_height[i]) ||
                    ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
                    job->param.tc[i].enable = 1;
            }
        }
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/

    /*================= Destination DMA =================*/
    // path 2 dma
    job->param.dest_dma[2].addr = job->param.out_addr_pa[0] +
                                ((job->param.dst_y[2] * job->param.dst_bg_width + job->param.dst_x[2]) << 1);

    if (job->param.dest_dma[2].addr == 0) {
        scl_err("dst dma2 addr = 0");
        return -1;
    }

    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES)
        job->param.dest_dma[2].line_offset = (job->param.dst_bg_width - border_width + job->param.dst_bg_width) >> 2;

    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES))
        job->param.dest_dma[2].line_offset = (job->param.dst_bg_width - border_width) >> 2;

    if ((property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))
        job->param.dest_dma[2].line_offset = (job->param.dst_bg_width - border_width) >> 2;

    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.dest_dma[2].line_offset = (job->param.dst_bg_width - border_width + job->param.dst_bg_width) >> 2;

    if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.dest_dma[2].line_offset = (job->param.dst_bg_width - border_width) >> 2;
    // path 3 dma
    job->param.dest_dma[3].addr = job->param.out_addr_pa[0] +
                                ((job->param.dst_y[3] * job->param.dst_bg_width + job->param.dst_x[3]) << 1);

    if (job->param.dest_dma[3].addr == 0) {
        scl_err("dst dma3 addr = 0");
        return -1;
    }

    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES)
        job->param.dest_dma[3].line_offset = (job->param.dst_bg_width - border_width + job->param.dst_bg_width) >> 2;

    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES))
        job->param.dest_dma[3].line_offset = (job->param.dst_bg_width - border_width) >> 2;

    if ((property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))
        job->param.dest_dma[3].line_offset = (job->param.dst_bg_width - border_width) >> 2;

    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.dest_dma[3].line_offset = (job->param.dst_bg_width - border_width + job->param.dst_bg_width) >> 2;

    if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.dest_dma[3].line_offset = (job->param.dst_bg_width - border_width) >> 2;
    // path 0/1 dma for CVBS frame
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (property->vproperty[ch_id].dst2_width != 0 && property->vproperty[ch_id].dst2_height != 0) {
            // path 0
            job->param.dest_dma[0].addr = job->param.out_addr_pa[1] +
                                        ((job->param.dst_y[0] * job->param.dst2_bg_width + job->param.dst_x[0]) << 1);
            // path 1
            job->param.dest_dma[1].addr = job->param.out_addr_pa[1] +
                                        ((job->param.dst_y[1] * job->param.dst2_bg_width + job->param.dst_x[1]) << 1);

            job->param.dest_dma[0].line_offset = (property->dst2_bg_width - cvbs_border_width) >> 2;
            job->param.dest_dma[1].line_offset = (property->dst2_bg_width - cvbs_border_width) >> 2;

            if ((job->param.dest_dma[0].addr == 0) || (job->param.dest_dma[1].addr == 0)) {
               scl_err("dst dma0/1 addr = 0");
               return -1;
            }
        }
    }

    return 0;
}

int fscaler300_set_ratio_1frame1frame_border_h_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    u32 crcby = 0;
    int i, pal_idx = 0;
    fscaler300_property_t *property = &node->property;
    int border_width;
    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    border_width = (property->vproperty[ch_id].border.width + 1) << 2;

    job->param.src_width = property->vproperty[ch_id].dst_width;
    job->param.src_height = border_width;

    job->param.src_bg_width = job->param.src_width;
    job->param.src_bg_height = job->param.src_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.src_x = 0;
    job->param.src_y = 0;
    // path 0 draw upper border
    job->param.dst_width[0] = job->param.src_width;
    job->param.dst_height[0] = job->param.src_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;
    // path 1 draw lower border
    job->param.dst_width[1] = job->param.src_width;
    job->param.dst_height[1] = job->param.src_height;
    job->param.dst_x[1] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[1] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].dst_height - border_width;

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;

    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "border width %d\n", border_width);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_width 1[%d]\n", job->param.dst_width[1]);
        printm(MODULE_NAME, "dst_height 1[%d]\n", job->param.dst_height[1]);
        printm(MODULE_NAME, "dst_x 1[%d]\n", job->param.dst_x[1]);
        printm(MODULE_NAME, "dst_y 1[%d]\n", job->param.dst_y[1]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "out addr pa [%d]\n", job->param.out_addr_pa[0]);
        printm(MODULE_NAME, "out addr pa 1[%d]\n", job->param.out_addr_pa[1]);
        printm(MODULE_NAME, "pal idx [%d]\n", property->vproperty[ch_id].border.pal_idx);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
    job->param.global.sca_src_format = YUV422;
    job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    // use path 0/1 to draw border
    // bypass
    for (i = 0; i < 2; i++) {
        job->param.sd[i].bypass = 1;
        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    pal_idx = property->vproperty[ch_id].border.pal_idx;
    crcby = priv.global_param.init.palette[pal_idx].crcby;
    job->param.sc.fix_pat_en = 1;
    job->param.sc.fix_pat_cr = (crcby >> 16);
    job->param.sc.fix_pat_cb = ((crcby & 0xff00) >> 8);
    job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (crcby & 0xff);

    /*================= SD =================*/
    // path 0/1
    for (i = 0; i < 2; i++) {
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = 256;
        job->param.sd[i].vstep = 256;
    }

    /*================= TC =================*/
    for (i = 0; i < 2; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/

    /*================= Destination DMA =================*/
    // path 0 dma
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] +
                                ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    // path 1 dma
    job->param.dest_dma[1].addr = job->param.out_addr_pa[0] +
                                ((job->param.dst_y[1] * job->param.dst_bg_width + job->param.dst_x[1]) << 1);
    job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - job->param.dst_width[1]) >> 2;

    if (job->param.dest_dma[1].addr == 0) {
        scl_err("dst dma1 addr = 0");
        return -1;
    }
    return 0;
}

int fscaler300_set_ratio_1frame1frame_border_v_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    u32 crcby = 0;
    int i, pal_idx = 0;
    fscaler300_property_t *property = &node->property;
    int border_width, fix_width;
    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    border_width = (property->vproperty[ch_id].border.width + 1) << 2;

    /* border width must be 4 align, if fix pattern width < 16, image has something wrong --> osd issue
       sw workaround method : fix pattern width must >= 16,
       if ap border width < 16, use scaler target cropping, crop ap border width */
    if (border_width < 16)
        fix_width = 16;
    else
        fix_width = border_width;

    job->param.src_width = fix_width;
    job->param.src_height = property->vproperty[ch_id].dst_height;

    job->param.src_bg_width = job->param.src_width;
    job->param.src_bg_height = job->param.src_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.src_x = 0;
    job->param.src_y = 0;

    // path 0 draw frame's left border
    job->param.dst_width[0] = job->param.src_width;
    job->param.dst_height[0] = job->param.src_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;
    // path 1 draw frame's right border
    job->param.dst_width[1] = job->param.src_width;
    job->param.dst_height[1] = job->param.src_height;
    job->param.dst_x[1] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].dst_width - border_width;
    job->param.dst_y[1] = property->vproperty[ch_id].dst_y;

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;

    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "border width %d fix width %d\n", border_width, fix_width);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_width 1[%d]\n", job->param.dst_width[1]);
        printm(MODULE_NAME, "dst_height 1[%d]\n", job->param.dst_height[1]);
        printm(MODULE_NAME, "dst_x 1[%d]\n", job->param.dst_x[1]);
        printm(MODULE_NAME, "dst_y 1[%d]\n", job->param.dst_y[1]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "out addr pa [%d]\n", job->param.out_addr_pa[0]);
        printm(MODULE_NAME, "out addr pa 1[%d]\n", job->param.out_addr_pa[1]);
        printm(MODULE_NAME, "pal idx [%d]\n", property->vproperty[ch_id].border.pal_idx);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
    job->param.global.sca_src_format = YUV422;
    job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    //path = 2;
    // bypass, path 0 & 1 for bypass
    for (i = 0; i < 2; i++) {
        job->param.sd[i].bypass = 1;
        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 1;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    pal_idx = property->vproperty[ch_id].border.pal_idx;
    crcby = priv.global_param.init.palette[pal_idx].crcby;
    job->param.sc.fix_pat_en = 1;
    job->param.sc.fix_pat_cr = (crcby >> 16);
    job->param.sc.fix_pat_cb = ((crcby & 0xff00) >> 8);
    job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (crcby & 0xff);

    /*================= SD =================*/
    for (i = 0; i < 2; i++) {
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = 256;
        job->param.sd[i].vstep = 256;
    }

    /*================= TC =================*/
    for (i = 0; i < 2; i++) {
        job->param.tc[i].width = border_width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = border_width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/

    /*================= Destination DMA =================*/
    // path 0 dma
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] +
                                ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);

    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - border_width) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }
    // path 1 dma
    job->param.dest_dma[1].addr = job->param.out_addr_pa[0] +
                                ((job->param.dst_y[1] * job->param.dst_bg_width + job->param.dst_x[1]) << 1);

    job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - border_width) >> 2;

    if (job->param.dest_dma[1].addr == 0) {
        scl_err("dst dma1 addr = 0");
        return -1;
    }
    return 0;
}

/*
 * use fix pattern frm2field to draw horizontal border when border enable
 * use path 0 frm2field to draw top/bottom frame's upper border
 * use path 1 frm2field to draw top/bottom frame's lower border
 * if 3 frame's width <= 4096, use path 0 to draw CVBS frame's upper & lower border
 */
int fscaler300_set_border_h_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    u32 crcby = 0;
    int i, pal_idx = 0;
    fscaler300_property_t *property = &node->property;
    int border_width, cvbs_border_width = 0;
    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    border_width = (property->vproperty[ch_id].border.width + 1) << 2;

    job->param.src_width = property->vproperty[ch_id].dst_width;
    // border height must be 4 align
    // draw frame's border, so border will not jump line, ,and because of frm2field, top/bottom height = border width, so height * 2
    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))
        job->param.src_height = border_width << 1;
    if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.src_height = border_width << 1;
    // draw ratio frame's border, so border will be jump line
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES)
        job->param.src_height = border_width;
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.src_height = border_width;

    job->param.src_bg_width = job->param.src_width;
    job->param.src_bg_height = job->param.src_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.src_x = 0;
    job->param.src_y = 0;
    // path 2 draw upper border
    job->param.dst_width[2] = job->param.src_width;
    job->param.dst_height[2] = job->param.src_height;
    job->param.dst_x[2] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[2] = property->vproperty[ch_id].dst_y;
    // path 3 draw lower border
    job->param.dst_width[3] = job->param.src_width;
    job->param.dst_height[3] = job->param.src_height;
    job->param.dst_x[3] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[3] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].dst_height - border_width;
    // path 0 draw CVBS border
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (property->vproperty[ch_id].dst2_width != 0 && property->vproperty[ch_id].dst2_height != 0) {
            cvbs_border_width = fscaler300_set_cvbs_border_width(node, ch_id);
            job->param.dst_width[0] = property->vproperty[ch_id].dst2_width;
            border_width = (cvbs_border_width + 1) << 2;
            job->param.dst_height[0] = border_width << 1;
            job->param.dst_x[0] = property->vproperty[ch_id].dst2_x;
            job->param.dst_y[0] = property->vproperty[ch_id].dst2_y;
        }
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;

    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        job->param.dest_dma[2].field_offset = job->param.dest_dma[3].field_offset =
        (property->dst_bg_width * property->dst_bg_height << 1) >> 5;
        if ((property->dst_bg_width * property->dst_bg_height << 1) % 32 != 0) {
            scl_err("field offset not 32's multiple\n");
            return -1;
        }
    }

    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES) {
        job->param.dest_dma[2].field_offset = job->param.dest_dma[3].field_offset =
        ((property->dst_bg_width * property->dst_bg_height + property->dst_bg_width) << 1) >> 5;
        if ((property->dst_bg_width * property->dst_bg_height << 1) % 32 != 0) {
            scl_err("field offset not 32's multiple\n");
            return -1;
        }
    }

    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        job->param.dest_dma[2].field_offset = job->param.dest_dma[3].field_offset =
        ((property->dst_bg_width * property->dst_bg_height + property->dst_bg_width) << 1) >> 5;
        if (((property->dst_bg_width * property->dst_bg_height + property->dst_bg_width) << 1) % 32 != 0) {
            scl_err("field offset not 32's multiple\n");
            return -1;
        }
        // CVBS frame
        if (property->vproperty[ch_id].dst2_width != 0 && property->vproperty[ch_id].dst2_height != 0) {
            job->param.dest_dma[0].field_offset = (property->dst2_bg_width * (property->vproperty[ch_id].dst2_height - border_width) << 1) >> 5;
            if ((property->dst2_bg_width * (property->vproperty[ch_id].dst2_height - border_width) << 1) % 32 != 0) {
                scl_err("field offset not 32's multiple\n");
                return -1;
            }
        }
    }

    if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        job->param.dest_dma[2].field_offset = job->param.dest_dma[3].field_offset =
        (property->dst_bg_width * property->dst_bg_height << 1) >> 5;
        if ((property->dst_bg_width * property->dst_bg_height << 1) % 32 != 0) {
            scl_err("field offset not 32's multiple\n");
            return -1;
        }
    }

    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "border width %d \n", border_width);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width 2[%d]\n", job->param.dst_width[2]);
        printm(MODULE_NAME, "dst_height 2[%d]\n", job->param.dst_height[2]);
        printm(MODULE_NAME, "dst_x 2[%d]\n", job->param.dst_x[2]);
        printm(MODULE_NAME, "dst_y 2[%d]\n", job->param.dst_y[2]);
        printm(MODULE_NAME, "dst_width 3[%d]\n", job->param.dst_width[3]);
        printm(MODULE_NAME, "dst_height 3[%d]\n", job->param.dst_height[3]);
        printm(MODULE_NAME, "dst_x 3[%d]\n", job->param.dst_x[3]);
        printm(MODULE_NAME, "dst_y 3[%d]\n", job->param.dst_y[3]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "out addr pa [%d]\n", job->param.out_addr_pa[0]);
        printm(MODULE_NAME, "out addr pa 1[%d]\n", job->param.out_addr_pa[1]);
        printm(MODULE_NAME, "pal idx [%d]\n", property->vproperty[ch_id].border.pal_idx);
        printm(MODULE_NAME, "ratio to 3 frame\n");
        printm(MODULE_NAME, "dst_width 0[%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height 0[%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x 0[%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y 0[%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "cvbs_border_width [%d]\n", cvbs_border_width);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
    job->param.global.sca_src_format = YUV422;
    job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    // use path 2/3 to draw border, use frm2field & bypass
    // bypass
    for (i = 2; i < 4; i++) {
        job->param.sd[i].bypass = 1;
        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }
    job->param.frm2field.res2_frm2field = 1;
    job->param.frm2field.res3_frm2field = 1;
    // if 3 frame's width <= 4096, draw CVBS frame's border here, use path 0 scaling down
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (property->vproperty[ch_id].dst2_width != 0 && property->vproperty[ch_id].dst2_height != 0) {
            if ((property->vproperty[ch_id].dst_width << 1) + property->vproperty[ch_id].dst2_width <= SCALER300_TOTAL_RESOLUTION) {
                job->param.sd[0].bypass = 0;
                job->param.sd[0].enable = 1;
                job->param.tc[0].enable = 0;
                job->param.frm2field.res0_frm2field = 1;
            }
        }
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    pal_idx = property->vproperty[ch_id].border.pal_idx;
    crcby = priv.global_param.init.palette[pal_idx].crcby;
    job->param.sc.fix_pat_en = 1;
    job->param.sc.fix_pat_cr = (crcby >> 16);
    job->param.sc.fix_pat_cb = ((crcby & 0xff00) >> 8);
    job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (crcby & 0xff);

    /*================= SD =================*/
    // path 2/3
    for (i = 2; i < 4; i++) {
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = 256;
        job->param.sd[i].vstep = 256;
    }

    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (property->vproperty[ch_id].dst2_width != 0 && property->vproperty[ch_id].dst2_height != 0) {
            if ((property->vproperty[ch_id].dst_width << 1) + property->vproperty[ch_id].dst2_width <= SCALER300_TOTAL_RESOLUTION) {
                if ((job->param.dst_width[0] == 0) || (job->param.dst_height[0] == 0)) {
                    scl_err("dst width %d/height %d is zero\n", job->param.dst_width[0], job->param.dst_height[0]);
                    return -1;
                }
                // SD
                job->param.sd[0].width = job->param.dst_width[0];
                job->param.sd[0].height = job->param.dst_height[0];
                // hstep/vstep
                job->param.sd[0].hstep = ((job->param.src_width << 8) / job->param.dst_width[0]);
                job->param.sd[0].vstep = ((job->param.src_height << 8) / job->param.dst_height[0]);
                // TC
                job->param.tc[0].width = job->param.sd[0].width;
                job->param.tc[0].height = job->param.sd[0].height;
                job->param.tc[0].x_start = 0;
                job->param.tc[0].x_end = job->param.sd[0].width - 1;
                job->param.tc[0].y_start = 0;
                job->param.tc[0].y_end = job->param.sd[0].height - 1;
            }
        }
    }

    /*================= TC =================*/
    for (i = 2; i < 4; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/

    /*================= Destination DMA =================*/
    // path 2 dma
    job->param.dest_dma[2].addr = job->param.out_addr_pa[0] +
                                ((job->param.dst_y[2] * job->param.dst_bg_width + job->param.dst_x[2]) << 1);

    if (job->param.dest_dma[2].addr == 0) {
        scl_err("dst dma2 addr = 0");
        return -1;
    }

    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES))
        job->param.dest_dma[2].line_offset = (job->param.dst_bg_width - job->param.dst_width[2]) >> 2;
    if ((property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))
        job->param.dest_dma[2].line_offset = (job->param.dst_bg_width - job->param.dst_width[2]) >> 2;
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES)
        job->param.dest_dma[2].line_offset = (job->param.dst_bg_width - job->param.dst_width[2] + job->param.dst_bg_width) >> 2;
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.dest_dma[2].line_offset = (job->param.dst_bg_width - job->param.dst_width[2] + job->param.dst_bg_width) >> 2;
    if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.dest_dma[2].line_offset = (job->param.dst_bg_width - job->param.dst_width[2]) >> 2;

    // path 3 dma
    job->param.dest_dma[3].addr = job->param.out_addr_pa[0] +
                                ((job->param.dst_y[3] * job->param.dst_bg_width + job->param.dst_x[3]) << 1);

    if (job->param.dest_dma[3].addr == 0) {
        scl_err("dst dma3 addr = 0");
        return -1;
    }

    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES))
        job->param.dest_dma[3].line_offset = (job->param.dst_bg_width - job->param.dst_width[3]) >> 2;
    if ((property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))
        job->param.dest_dma[3].line_offset = (job->param.dst_bg_width - job->param.dst_width[3]) >> 2;
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES)
        job->param.dest_dma[3].line_offset = (job->param.dst_bg_width - job->param.dst_width[3] + job->param.dst_bg_width) >> 2;
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.dest_dma[3].line_offset = (job->param.dst_bg_width - job->param.dst_width[3] + job->param.dst_bg_width) >> 2;
    if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.dest_dma[3].line_offset = (job->param.dst_bg_width - job->param.dst_width[3]) >> 2;
    // path 0 dma
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (property->vproperty[ch_id].dst2_width != 0 && property->vproperty[ch_id].dst2_height != 0) {
            if ((property->vproperty[ch_id].dst_width << 1) + property->vproperty[ch_id].dst2_width <= SCALER300_TOTAL_RESOLUTION) {
                job->param.dest_dma[0].addr = job->param.out_addr_pa[1] +
                                            ((job->param.dst_y[0] * job->param.dst2_bg_width + job->param.dst_x[0]) << 1);
                job->param.dest_dma[0].line_offset = (job->param.dst2_bg_width - job->param.dst_width[0]) >> 2;

                if (job->param.dest_dma[0].addr == 0) {
                    scl_err("dst dma0 addr = 0");
                    return -1;
                }
            }
        }
    }

    return 0;
}

/*
 * use fix pattern to draw CVBS frame's vertical border when border enable
 */
int fscaler300_set_cvbs_border_v_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    u32 crcby = 0;
    int i, pal_idx = 0;
    fscaler300_property_t *property = &node->property;
    int border_width, fix_width;
    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    border_width = (property->vproperty[ch_id].border.width + 1) << 2;

    /* border width must be 4 align, if fix pattern width < 16, image has something wrong --> osd issue
       sw workaround method : fix pattern width must >= 16,
       if ap border width < 16, use scaler target cropping, crop ap border width */
    if (border_width < 16)
        fix_width = 16;
    else
        fix_width = border_width;

    job->param.src_width = fix_width;
    job->param.src_height = property->vproperty[ch_id].dst2_height;

    job->param.src_bg_width = property->src2_bg_width;
    job->param.src_bg_height = property->src2_bg_height;
    job->param.dst_bg_width = property->src2_bg_width;
    job->param.dst_bg_height = property->src2_bg_height;

    job->param.src_x = 0;
    job->param.src_y = 0;
    // path 0 draw cvbs frame's left border
    job->param.dst_width[0] = job->param.src_width;
    job->param.dst_height[0] = job->param.src_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst2_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst2_y;
    // path 1 draw cvbs frame's right border
    job->param.dst_width[1] = job->param.src_width;
    job->param.dst_height[1] = job->param.src_height;
    job->param.dst_x[1] = property->vproperty[ch_id].dst2_x + property->vproperty[ch_id].dst2_width - border_width;
    job->param.dst_y[1] = property->vproperty[ch_id].dst2_y;

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;

#if 0
    printk("****property [%d]***\n", job->job_id);
    printk("src_fmt [%d][%d]\n", job->param.src_format, ch_id);
    printk("dst_fmt [%d]\n", job->param.dst_format);
    printk("src_width [%d]\n", job->param.src_width);
    printk("src_height [%d]\n", job->param.src_height);
    printk("src_bg_width [%d]\n", job->param.src_bg_width);
    printk("src_bg_height [%d]\n", job->param.src_bg_height);
    printk("dst_width 2[%d]\n", job->param.dst_width[0]);
    printk("dst_height 2[%d]\n", job->param.dst_height[0]);
    printk("dst_x [%d]\n", job->param.dst_x[0]);
    printk("dst_y [%d]\n", job->param.dst_y[0]);
    printk("dst_width 3[%d]\n", job->param.dst_width[1]);
    printk("dst_height 3[%d]\n", job->param.dst_height[1]);
    printk("dst_x 1[%d]\n", job->param.dst_x[1]);
    printk("dst_y 1[%d]\n", job->param.dst_y[1]);
    printk("dst_bg_width [%d]\n", job->param.dst_bg_width);
    printk("dst_bg_height [%d]\n", job->param.dst_bg_height);
    printk("scl_feature [%d]\n", job->param.scl_feature);
#endif

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
    job->param.global.sca_src_format = YUV422;
    job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    //path = 2;
    for (i = 0; i < 2; i++) {
        job->param.sd[i].bypass = 1;
        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 1;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    pal_idx = property->vproperty[ch_id].border.pal_idx;
    crcby = priv.global_param.init.palette[pal_idx].crcby;
    job->param.sc.fix_pat_en = 1;
    job->param.sc.fix_pat_cr = (crcby >> 16);
    job->param.sc.fix_pat_cb = ((crcby & 0xff00) >> 8);
    job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (crcby & 0xff);

    /*================= SD =================*/
    for (i = 0; i < 2; i++) {
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = 256;
        job->param.sd[i].vstep = 256;
    }

    /*================= TC =================*/
    for (i = 0; i < 2; i++) {
        job->param.tc[i].width = border_width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = border_width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/

    /*================= Destination DMA =================*/
    if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.out_addr_pa[1] = job->param.out_addr_pa[0] + (property->dst_bg_width * property->dst_bg_height << 2);

   // path 0
    job->param.dest_dma[0].addr = job->param.out_addr_pa[1] +
                                ((job->param.dst_y[0] * job->param.src_bg_width + job->param.dst_x[0]) << 1);

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }
    // path 1
    job->param.dest_dma[1].addr = job->param.out_addr_pa[1] +
                                ((job->param.dst_y[1] * job->param.src_bg_width + job->param.dst_x[1]) << 1);

    if (job->param.dest_dma[1].addr == 0) {
        scl_err("dst dma1 addr = 0");
        return -1;
    }
    job->param.dest_dma[0].line_offset = (property->src2_bg_width - border_width) >> 2;
    job->param.dest_dma[1].line_offset = (property->src2_bg_width - border_width) >> 2;

    return 0;
}


/*
 * use fix pattern to draw CVBS frame's horizontal border when border enable
 */
int fscaler300_set_cvbs_border_h_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    u32 crcby = 0;
    int i, path = 0, pal_idx = 0;
    fscaler300_property_t *property = &node->property;
    int border_width;
    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    border_width = (property->vproperty[ch_id].border.width + 1) << 2;

    job->param.src_width = property->vproperty[ch_id].dst2_width;
    job->param.src_height = border_width << 1;

    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME) {
        job->param.src_bg_width = property->dst2_bg_width;
        job->param.src_bg_height = property->dst2_bg_height;
        job->param.dst_bg_width = property->dst2_bg_width;
        job->param.dst_bg_height = property->dst2_bg_height;
    }
    if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        job->param.src_bg_width = property->src2_bg_width;
        job->param.src_bg_height = property->src2_bg_height;
        job->param.dst_bg_width = property->src2_bg_width;
        job->param.dst_bg_height = property->src2_bg_height;
    }
    job->param.src_x = 0;
    job->param.src_y = 0;
    // path 0
    job->param.dst_width[0] = property->vproperty[ch_id].dst2_width;
    job->param.dst_height[0] = border_width << 1;
    job->param.dst_x[0] = property->vproperty[ch_id].dst2_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst2_y;

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME) {
        job->param.dest_dma[0].field_offset = (property->dst2_bg_width * (property->vproperty[ch_id].dst2_height - border_width) << 1) >> 5;
        if ((property->dst2_bg_width * (property->vproperty[ch_id].dst2_height - border_width) << 1) % 32 != 0) {
            scl_err("field offset not 32's multiple\n");
            return -1;
        }
    }
    if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        job->param.dest_dma[0].field_offset = (property->src2_bg_width * (property->vproperty[ch_id].dst2_height - border_width) << 1) >> 5;
        if ((property->src2_bg_width * (property->vproperty[ch_id].dst2_height - border_width) << 1) % 32 != 0) {
            scl_err("field offset not 32's multiple\n");
            return -1;
        }
    }

#if 0
    printk("****property [%d]***\n", job->job_id);
    printk("src_fmt [%d][%d]\n", job->param.src_format, ch_id);
    printk("dst_fmt [%d]\n", job->param.dst_format);
    printk("src_width [%d]\n", job->param.src_width);
    printk("src_height [%d]\n", job->param.src_height);
    printk("src_bg_width [%d]\n", job->param.src_bg_width);
    printk("src_bg_height [%d]\n", job->param.src_bg_height);
    printk("dst_width 0[%d]\n", job->param.dst_width[0]);
    printk("dst_height 0[%d]\n", job->param.dst_height[0]);
    printk("dst_x 0[%d]\n", job->param.dst_x[0]);
    printk("dst_y 0[%d]\n", job->param.dst_y[0]);
    printk("dst_bg_width [%d]\n", job->param.dst_bg_width);
    printk("dst_bg_height [%d]\n", job->param.dst_bg_height);
    printk("scl_feature [%d]\n", job->param.scl_feature);
#endif

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
    job->param.global.sca_src_format = YUV422;
    job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    path = 1;
    // bypass
    for (i = 0; i < path; i++) {
        job->param.sd[i].bypass = 1;
        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }
    job->param.frm2field.res0_frm2field = 1;

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    pal_idx = property->vproperty[ch_id].border.pal_idx;
    crcby = priv.global_param.init.palette[pal_idx].crcby;
    job->param.sc.fix_pat_en = 1;
    job->param.sc.fix_pat_cr = (crcby >> 16);
    job->param.sc.fix_pat_cb = ((crcby & 0xff00) >> 8);
    job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (crcby & 0xff);

    /*================= SD =================*/
    for (i = 0; i < path; i++) {
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = 256;
        job->param.sd[i].vstep = 256;
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/

    /*================= Destination DMA =================*/
    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME) {
        job->param.dest_dma[0].addr = job->param.out_addr_pa[1];
        job->param.dest_dma[0].line_offset = 0;
    }
    if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        job->param.out_addr_pa[1] = job->param.out_addr_pa[0] + (property->dst_bg_width * property->dst_bg_height << 2);
        job->param.dest_dma[0].addr = job->param.out_addr_pa[1] +
                                    ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
        job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;
    }

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    return 0;
}

/*
 * use fix pattern to draw background when aspect ratio enable and border enable
 */
int fscaler300_set_frame_fix_bg_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    u32 crcby = 0;
    int i, path = 0, pal_idx = 0;
    fscaler300_property_t *property = &node->property;
    int fix_width = 0;
    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    if (property->vproperty[ch_id].aspect_ratio.type == H_TYPE) {
        job->param.src_width = property->vproperty[ch_id].dst_width;
        job->param.src_height = property->vproperty[ch_id].aspect_ratio.dst_y;
    }
    if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE) {
        job->param.src_width = property->vproperty[ch_id].aspect_ratio.dst_x;
        job->param.src_height = property->vproperty[ch_id].dst_height;

        /*  if fix pattern width < 16, image has something wrong --> osd issue
            sw workaround method : fix pattern width must >= 16,
            if ap border width < 16, use scaler target cropping, crop ap border width */
        if (job->param.src_width < 16) {
            fix_width = job->param.src_width;
            job->param.src_width = 16;
        }
        else
            fix_width = job->param.src_width;
    }

    job->param.src_bg_width = job->param.src_width;
    job->param.src_bg_height = job->param.src_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.src_x = 0;
    job->param.src_y = 0;
    // path 0
    job->param.dst_width[0] = job->param.src_width;
    job->param.dst_height[0] = job->param.src_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;
    // path 1
    job->param.dst_width[1] = job->param.src_width;
    job->param.dst_height[1] = job->param.src_height;
    if (property->vproperty[ch_id].aspect_ratio.type == H_TYPE) {
        job->param.dst_x[1] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].aspect_ratio.dst_x;
        job->param.dst_y[1] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].dst_height - job->param.src_height;
    }
    if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE) {
        job->param.dst_x[1] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].dst_width - fix_width;
        job->param.dst_y[1] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].aspect_ratio.dst_y;
    }
    job->param.scl_feature = property->vproperty[ch_id].scl_feature;

    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_x1 [%d]\n", job->param.dst_x[1]);
        printm(MODULE_NAME, "dst_y1 [%d]\n", job->param.dst_y[1]);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "out addr pa [%x]\n", property->out_addr_pa);
        printm(MODULE_NAME, "fix_width [%d]\n", fix_width);
        printm(MODULE_NAME, "ratio type [%d]\n", property->vproperty[ch_id].aspect_ratio.type);
        printm(MODULE_NAME, "pal idx [%d]\n", property->vproperty[ch_id].aspect_ratio.pal_idx);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
    job->param.global.sca_src_format = YUV422;
    job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    path = 2;

    // bypass
    for (i = 0; i < path; i++) {
        job->param.sd[i].bypass = 1;
        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }
    /* fix pattern width < 16, enable target cropping */
    if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE) {
        if (fix_width != job->param.src_width)
            job->param.tc[0].enable = 1;
            job->param.tc[1].enable = 1;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    pal_idx = property->vproperty[ch_id].aspect_ratio.pal_idx;
    crcby = priv.global_param.init.palette[pal_idx].crcby;
    job->param.sc.fix_pat_en = 1;
    job->param.sc.fix_pat_cr = (crcby >> 16);
    job->param.sc.fix_pat_cb = ((crcby & 0xff00) >> 8);
    job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (crcby & 0xff);

    /*================= SD =================*/
    for (i = 0; i < path; i++) {
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = 256;
        job->param.sd[i].vstep = 256;
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
    }
    /* fix pattern width < 16, enable target cropping to crop original size */
    for (i = 0; i < path; i++) {
        if (job->param.tc[i].enable == 1) {
            job->param.tc[i].width = fix_width;
            job->param.tc[i].height = job->param.sd[i].height;
            job->param.tc[i].x_start = 0;
            job->param.tc[i].x_end = fix_width - 1;
            job->param.tc[i].y_start = 0;
            job->param.tc[i].y_end = job->param.sd[i].height - 1;
        }
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/

    /*================= Destination DMA =================*/
    // path 0 dma
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] +
                                ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;
    if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE)
        job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - fix_width) >> 2;

    // path 1 dma
    job->param.dest_dma[1].addr = job->param.out_addr_pa[0] +
                                    ((job->param.dst_y[1] * job->param.dst_bg_width + job->param.dst_x[1]) << 1);

    if (job->param.dest_dma[1].addr == 0) {
        scl_err("dst dma1 addr = 0");
        return -1;
    }

    job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - job->param.dst_width[1]) >> 2;
    if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE)
        job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - fix_width) >> 2;

    return 0;
}


/*
 * use fix pattern to draw background when aspect ratio enable and border enable
 */
int fscaler300_set_fix_bg_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    u32 crcby = 0;
    int i, path = 0, pal_idx = 0;
    fscaler300_property_t *property = &node->property;
    int fix_width = 0;
    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    if (property->vproperty[ch_id].aspect_ratio.type == H_TYPE) {
        job->param.src_width = property->vproperty[ch_id].dst_width;
        job->param.src_height = property->vproperty[ch_id].aspect_ratio.dst_y;
        if ((property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
            (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
            if (property->vproperty[ch_id].border.enable)
                job->param.src_height += ((property->vproperty[ch_id].border.width + 1) << 2);
        }
    }
    if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE) {
        job->param.src_width = property->vproperty[ch_id].aspect_ratio.dst_x;
        if (job->param.src_width % 4) {
            if ((property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
                (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))
                job->param.src_width += 2;  // fix pattern width must bigger than original width & 4 align
            else
                job->param.src_width -= 2;
        }

        job->param.src_height = property->vproperty[ch_id].dst_height;
        // if border enable, veritcal mask width must be 4 align, one side should not visible, use original frame to cover it
        if ((property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
            (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
            if (property->vproperty[ch_id].border.enable)
                job->param.src_width += ((property->vproperty[ch_id].border.width + 1) << 2);
        }
        /*  if fix pattern width < 16, image has something wrong --> osd issue
            sw workaround method : fix pattern width must >= 16,
            if ap border width < 16, use scaler target cropping, crop ap border width */
        if (job->param.src_width < 16) {
            fix_width = job->param.src_width;
            job->param.src_width = 16;
        }
        else
            fix_width = job->param.src_width;
    }

    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))
        job->param.src_height <<= 1;
    if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.src_height <<= 1;

    job->param.src_bg_width = job->param.src_width;
    job->param.src_bg_height = job->param.src_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.src_x = 0;
    job->param.src_y = 0;
    // path 0
    job->param.dst_width[0] = job->param.src_width;
    job->param.dst_height[0] = job->param.src_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;
    // path 1
    job->param.dst_width[1] = job->param.src_width;
    job->param.dst_height[1] = job->param.src_height;
    if (property->vproperty[ch_id].aspect_ratio.type == H_TYPE) {
        job->param.dst_x[1] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].aspect_ratio.dst_x;
        job->param.dst_y[1] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].dst_height - job->param.src_height;
        if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
            (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))
            job->param.dst_y[1] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].dst_height - (job->param.src_height >> 1);
        if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
            job->param.dst_y[1] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].dst_height - (job->param.src_height >> 1);
    }
    if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE) {
        job->param.dst_x[1] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].dst_width - fix_width;
        job->param.dst_y[1] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].aspect_ratio.dst_y;
    }
    job->param.scl_feature = property->vproperty[ch_id].scl_feature;

    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_x1 [%d]\n", job->param.dst_x[1]);
        printm(MODULE_NAME, "dst_y1 [%d]\n", job->param.dst_y[1]);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "out addr pa [%x]\n", property->out_addr_pa);
        printm(MODULE_NAME, "fix_width [%d]\n", fix_width);
        printm(MODULE_NAME, "ratio type [%d]\n", property->vproperty[ch_id].aspect_ratio.type);
        printm(MODULE_NAME, "border enable [%d]\n", property->vproperty[ch_id].border.enable);
        printm(MODULE_NAME, "pal idx [%d]\n", property->vproperty[ch_id].aspect_ratio.pal_idx);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;

    job->param.dest_dma[0].field_offset = job->param.dest_dma[1].field_offset =
    ((property->dst_bg_width << 1) + (property->dst_bg_width * property->dst_bg_height << 1)) >> 5;
    if ((((property->dst_bg_width << 1) + (property->dst_bg_width * property->dst_bg_height << 1)) % 32) != 0) {
        scl_err("field offset not 32's multiple\n");
        return -1;
    }

    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        job->param.dest_dma[0].field_offset = job->param.dest_dma[1].field_offset =
        ((property->dst_bg_width * property->dst_bg_height << 1)) >> 5;
        if ((property->dst_bg_width * property->dst_bg_height << 1) % 32 != 0) {
            scl_err("field offset not 32's multiple\n");
            return -1;
        }
    }

    if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        job->param.dest_dma[0].field_offset = job->param.dest_dma[1].field_offset =
        ((property->dst_bg_width * property->dst_bg_height << 1)) >> 5;
        if ((property->dst_bg_width * property->dst_bg_height << 1) % 32 != 0) {
            scl_err("field offset not 32's multiple\n");
            return -1;
        }
    }

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
    job->param.global.sca_src_format = YUV422;
    job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES)           // 1 input 2 output
        path = 2;   // path 0/1 do frm2field
    if ((property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))   // 1 input 2 output
        path = 2;   // path 0/1 do frm2field
    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES) ||          // 1 input 2 output
        (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))
        path = 2;   // path 0/1 do frm2field
    if ((property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))   // 1 input 2 output
        path = 2;   // path 0/1 do frm2field

    // bypass
    for (i = 0; i < path; i++) {
        job->param.sd[i].bypass = 1;
        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }
    /* fix pattern width < 16, enable target cropping */
    if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE) {
        if (fix_width != job->param.src_width)
            job->param.tc[0].enable = 1;
            job->param.tc[1].enable = 1;
    }

    job->param.frm2field.res0_frm2field = 1;
    job->param.frm2field.res1_frm2field = 1;

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    pal_idx = property->vproperty[ch_id].aspect_ratio.pal_idx;
    crcby = priv.global_param.init.palette[pal_idx].crcby;
    job->param.sc.fix_pat_en = 1;
    job->param.sc.fix_pat_cr = (crcby >> 16);
    job->param.sc.fix_pat_cb = ((crcby & 0xff00) >> 8);
    job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (crcby & 0xff);

    /*================= SD =================*/
    for (i = 0; i < path; i++) {
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = 256;
        job->param.sd[i].vstep = 256;
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
    }
    /* fix pattern width < 16, enable target cropping to crop original size */
    for (i = 0; i < path; i++) {
        if (job->param.tc[i].enable == 1) {
            job->param.tc[i].width = fix_width;
            job->param.tc[i].height = job->param.sd[i].height;
            job->param.tc[i].x_start = 0;
            job->param.tc[i].x_end = fix_width - 1;
            job->param.tc[i].y_start = 0;
            job->param.tc[i].y_end = job->param.sd[i].height - 1;
        }
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/

    /*================= Destination DMA =================*/
    // path 0 dma
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] +
                                ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0] + job->param.dst_bg_width) >> 2;
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE)
            job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - fix_width + job->param.dst_bg_width) >> 2;
    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE)
            job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - fix_width) >> 2;
        else
            job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;
        }

    if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE)
            job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - fix_width) >> 2;
        else
            job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;
        }

    // path 1 dma
    job->param.dest_dma[1].addr = job->param.out_addr_pa[0] +
                                    ((job->param.dst_y[1] * job->param.dst_bg_width + job->param.dst_x[1]) << 1);

    if (job->param.dest_dma[1].addr == 0) {
        scl_err("dst dma1 addr = 0");
        return -1;
    }

    job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - job->param.dst_width[1] + job->param.dst_bg_width) >> 2;
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE)
            job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - fix_width + job->param.dst_bg_width) >> 2;
    if ((property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE)
            job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - fix_width) >> 2;
        else
            job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - job->param.dst_width[1]) >> 2;
    }
    if (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE)
            job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - fix_width) >> 2;
        else
            job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - job->param.dst_width[1]) >> 2;
    }
    /*================= MASK =================*/
    /* dst_fmt is 2 frame, use mask to draw border */
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES) {
        if (property->vproperty[ch_id].border.enable) {
            // mask 0
            if (property->vproperty[ch_id].aspect_ratio.type == H_TYPE) {
                job->param.mask[0].enable = 1;
                job->param.mask[0].x_start = 0;
                job->param.mask[0].x_end = job->param.dst_width[0] - 1;
                job->param.mask[0].y_start = 0;
                job->param.mask[0].y_end = ((property->vproperty[ch_id].border.width + 1) << 2) - 1;
                if (property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES)
                    job->param.mask[0].y_end = (((property->vproperty[ch_id].border.width + 1) << 2) << 1) - 1;
                job->param.mask[0].alpha = 7;
                job->param.mask[0].true_hollow = 1;
                job->param.mask[0].color = property->vproperty[ch_id].border.pal_idx;
                // mask 1
                job->param.mask[1].enable = 1;
                job->param.mask[1].x_start = 0;
                job->param.mask[1].x_end = job->param.dst_width[1] - 1;
                job->param.mask[1].y_start = job->param.dst_height[1] - ((property->vproperty[ch_id].border.width + 1) << 2);
                if (property->src_fmt == TYPE_YUV422_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES)
                    job->param.mask[1].y_start = job->param.dst_height[1] - (((property->vproperty[ch_id].border.width + 1) << 2) << 1);
                job->param.mask[1].y_end = job->param.dst_height[1] - 1;
                job->param.mask[1].alpha = 7;
                job->param.mask[1].true_hollow = 1;
                job->param.mask[1].color = property->vproperty[ch_id].border.pal_idx;
            }

            if (property->vproperty[ch_id].aspect_ratio.type == V_TYPE) {
                job->param.mask[0].enable = 1;
                job->param.mask[0].x_start = 0;
                job->param.mask[0].x_end = ((property->vproperty[ch_id].border.width + 1) << 2) - 1;
                job->param.mask[0].y_start = 0;
                job->param.mask[0].y_end = job->param.dst_height[0];
                job->param.mask[0].alpha = 7;
                job->param.mask[0].true_hollow = 1;
                job->param.mask[0].color = property->vproperty[ch_id].border.pal_idx;
                // mask 1
                job->param.mask[1].enable = 1;
                job->param.mask[1].x_start = job->param.dst_width[0] - ((property->vproperty[ch_id].border.width + 1) << 2);
                job->param.mask[1].x_end = job->param.dst_width[0] - 1;
                job->param.mask[1].y_start = 0;
                job->param.mask[1].y_end = job->param.dst_height[0];
                job->param.mask[1].alpha = 7;
                job->param.mask[1].true_hollow = 1;
                job->param.mask[1].color = property->vproperty[ch_id].border.pal_idx;
            }
        }
    }
    return 0;
}

/* source frame is progressive, use frame to field function to top/bottom, then scaling up/down
 * translate V.G single node with ratio frame to 2 frame with frm2field to fscaler300 hw parameter
 */
int fscaler300_set_frm2field_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    int i, path = 0;
    fscaler300_property_t *property = &node->property;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.ratio = fscaler300_select_ratio_source(node, ch_id);

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    /* sub_yuv = 1 means we have ratio frame, ratio = 1 means select source from ratio frame
       ratio frame's width/height is 1/2 source's width/height */
    if (property->sub_yuv == 1 && job->param.ratio == 1) {
        job->param.src_width = property->vproperty[ch_id].src_width >> 1;
        job->param.src_height = property->vproperty[ch_id].src_height >> 1;
        job->param.src_bg_width = property->src_bg_width >> 1;
        job->param.src_bg_height = property->src_bg_height >> 1;
        job->param.src_x = property->vproperty[ch_id].src_x >> 1;
        job->param.src_y = property->vproperty[ch_id].src_y >> 1;
        job->param.src_width = ALIGN_4(job->param.src_width);
        job->param.src_x = ALIGN_4(job->param.src_x);
        job->param.src_bg_width = ALIGN_4(job->param.src_bg_width);
    }

    job->param.dst_width[0] = property->vproperty[ch_id].dst_width;
    job->param.dst_height[0] = property->vproperty[ch_id].dst_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;

    if (property->vproperty[ch_id].aspect_ratio.enable == 1) {
        job->param.dst_width[0] = property->vproperty[ch_id].aspect_ratio.dst_width;
        job->param.dst_height[0] = property->vproperty[ch_id].aspect_ratio.dst_height;
        job->param.dst_x[0] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].aspect_ratio.dst_x;
        job->param.dst_y[0] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].aspect_ratio.dst_y;
    }
    // path 1 do CVBS frame scaling
    if ((property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (property->vproperty[ch_id].dst2_width != 0 && property->vproperty[ch_id].dst2_height != 0) {
            job->param.dst_width[1] = property->vproperty[ch_id].dst2_width;
            job->param.dst_height[1] = property->vproperty[ch_id].dst2_height;
            job->param.dst_x[1] = property->vproperty[ch_id].dst2_x;
            job->param.dst_y[1] = property->vproperty[ch_id].dst2_y;
        }
    }

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_width 1[%d]\n", job->param.dst_width[1]);
        printm(MODULE_NAME, "dst_height 1[%d]\n", job->param.dst_height[1]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_x1 [%d]\n", job->param.dst_x[1]);
        printm(MODULE_NAME, "dst_y1 [%d]\n", job->param.dst_y[1]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "dst_bg_width 2[%d]\n", job->param.dst2_bg_width);
        printm(MODULE_NAME, "dst_bg_height 2[%d]\n", job->param.dst2_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa [%x]\n", property->in_addr_pa);
        printm(MODULE_NAME, "in addr pa 1[%x]\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa [%x]\n", property->out_addr_pa);
        printm(MODULE_NAME, "out addr pa 1[%x]\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "sub yuv %d ratio %d\n", property->sub_yuv, job->param.ratio);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    /* source from ratio frame */
    if (property->sub_yuv == 1 && job->param.ratio == 1)
        job->param.in_addr_pa = property->in_addr_pa_1;

// ratio to 2frame
    /* job 2 input start from line 1, output to line 1*/
    job->param.dest_dma[0].field_offset = ((property->dst_bg_width << 1) +
                                          (property->dst_bg_width * property->dst_bg_height << 1)) >> 5;

    if (((property->dst_bg_width << 1) + (property->dst_bg_width * property->dst_bg_height << 1)) % 32 != 0) {
        scl_err("field offset not 32's multiple\n");
        return -1;
    }

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    if ((property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {  // 1 input 2 output
        if (node->property.vproperty[ch_id].dst2_width != 0 && node->property.vproperty[ch_id].dst2_height != 0)
            path = 2;   // path 0 do top/bottom frame frm2field, path 1 do CVBS frame scaling
        else
            path = 1;   // no CVBS frame
    }
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES)           // 1 input 1 output
        path = 1;   // path 0 do frm2field
    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    job->param.frm2field.res0_frm2field = 1;

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }
    if (job->param.frm2field.res0_frm2field == 1)
        job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0] + job->param.dst_bg_width) >> 2;
    else
        job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;
    /* CVBS frame scaling */
    if ((property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (node->property.vproperty[ch_id].dst2_width != 0 && node->property.vproperty[ch_id].dst2_height != 0) {
            job->param.dest_dma[1].addr = job->param.out_addr_pa[1] +
                                        ((job->param.dst_y[1] * job->param.dst2_bg_width + job->param.dst_x[1]) << 1);
            job->param.dest_dma[1].line_offset = (job->param.dst2_bg_width - job->param.dst_width[1]) >> 2;
            if (job->param.dest_dma[1].addr == 0) {
                scl_err("dst dma1 addr = 0");
                return -1;
            }
        }
    }

    return 0;
}

/* source frame is interlace, only get even line, then scaling up/down
 * translate V.G single node with frame to frame with get even line then scaling up/down to fscaler300 hw parameter
 */
int fscaler300_set_interlace_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    int i, path = 0;
    fscaler300_property_t *property = &node->property;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height >> 1;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    job->param.dst_width[0] = property->vproperty[ch_id].dst_width;
    job->param.dst_height[0] = property->vproperty[ch_id].dst_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "dst_bg_width 2[%d]\n", job->param.dst2_bg_width);
        printm(MODULE_NAME, "dst_bg_height 2[%d]\n", job->param.dst2_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa [%x]\n", property->in_addr_pa);
        printm(MODULE_NAME, "in addr pa 1[%x]\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa [%x]\n", property->out_addr_pa);
        printm(MODULE_NAME, "out addr pa 1[%x]\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "sub yuv %d ratio %d\n", property->sub_yuv, job->param.ratio);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    path = 1;   // path 0 do scaling
    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width + job->param.src_bg_width;

    /*================= Destination DMA =================*/
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma2 addr = 0");
        return -1;
    }

    return 0;
}

/* source frame is interlace, get top/bottom, then scaling up/down
 * translate V.G single node with ratio frame to 2 frame with get top/bottom line then scaling up/down to fscaler300 hw parameter
 */
int fscaler300_set_ratio_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    int i, path = 0;
    fscaler300_property_t *property = &node->property;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);

    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.ratio = fscaler300_select_ratio_source(node, ch_id);

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height >> 1;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    /* sub_yuv = 1 means we have ratio frame, ratio = 1 means select source from ratio frame
       ratio frame's width/height is 1/2 source's width/height */
    if (property->sub_yuv == 1 && job->param.ratio == 1) {
        job->param.src_width = property->vproperty[ch_id].src_width >> 1;
        job->param.src_height = (property->vproperty[ch_id].src_height >> 1) >> 1;
        job->param.src_bg_width = property->src_bg_width >> 1;
        job->param.src_bg_height = property->src_bg_height >> 1;
        job->param.src_x = property->vproperty[ch_id].src_x >> 1;
        job->param.src_y = property->vproperty[ch_id].src_y >> 1;
        job->param.src_width = ALIGN_4(job->param.src_width);
        job->param.src_x = ALIGN_4(job->param.src_x);
        job->param.src_bg_width = ALIGN_4(job->param.src_bg_width);
    }

    job->param.dst_width[0] = property->vproperty[ch_id].dst_width;
    job->param.dst_height[0] = property->vproperty[ch_id].dst_height >> 1;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;

    if (property->vproperty[ch_id].aspect_ratio.enable == 1) {
        job->param.dst_width[0] = property->vproperty[ch_id].aspect_ratio.dst_width;
        job->param.dst_height[0] = property->vproperty[ch_id].aspect_ratio.dst_height >> 1;
        job->param.dst_x[0] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].aspect_ratio.dst_x;
        job->param.dst_y[0] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].aspect_ratio.dst_y;
    }

    // path 1 do CVBS frame scaling
    if ((property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        job->param.dst_width[1] = property->vproperty[ch_id].dst2_width;
        job->param.dst_height[1] = property->vproperty[ch_id].dst2_height >> 1;
        job->param.dst_x[1] = property->vproperty[ch_id].dst2_x;
        job->param.dst_y[1] = property->vproperty[ch_id].dst2_y;
    }

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_width 1[%d]\n", job->param.dst_width[1]);
        printm(MODULE_NAME, "dst_height 1[%d]\n", job->param.dst_height[1]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_x1 [%d]\n", job->param.dst_x[1]);
        printm(MODULE_NAME, "dst_y1 [%d]\n", job->param.dst_y[1]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "dst_bg_width 2[%d]\n", job->param.dst2_bg_width);
        printm(MODULE_NAME, "dst_bg_height 2[%d]\n", job->param.dst2_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa [%x]\n", property->in_addr_pa);
        printm(MODULE_NAME, "in addr pa 1[%x]\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa [%x]\n", property->out_addr_pa);
        printm(MODULE_NAME, "out addr pa 1[%x]\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "sub yuv %d ratio %d\n", property->sub_yuv, job->param.ratio);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    /* source from ratio frame */
    if (property->sub_yuv == 1 && job->param.ratio == 1)
        job->param.in_addr_pa = property->in_addr_pa_1;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    if ((property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))   // 1 input 2 output
        path = 2;   // path 0 do scaling, path 1 do scaling
    if (property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES)           // 1 input 1 output
        path = 1;   // path 0 do scaling
    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    if (job_idx == 0)
        job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    else
        job->param.src_dma.addr = job->param.in_addr_pa + (job->param.src_bg_width << 1) +
                                  ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width + job->param.src_bg_width;

    /*================= Destination DMA =================*/
    if (job_idx == 0)
        job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    else
        job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + (job->param.dst_bg_width << 1) + ((job->param.dst_bg_width * job->param.dst_bg_height) << 1)
                                      + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);

    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0] + job->param.dst_bg_width) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }
    /* CVBS frame scalig*/
    if ((property->src_fmt == TYPE_YUV422_RATIO_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (job_idx == 0)
            job->param.dest_dma[1].addr = job->param.out_addr_pa[1] +
                                        ((job->param.dst_y[1] * job->param.dst2_bg_width + job->param.dst_x[1]) << 1);
        else
            job->param.dest_dma[1].addr = job->param.out_addr_pa[1] + (job->param.dst2_bg_width << 1) +
                                        ((job->param.dst_y[1] * job->param.dst2_bg_width + job->param.dst_x[1]) << 1);

        job->param.dest_dma[1].line_offset = ((job->param.dst2_bg_width) - job->param.dst_width[1] + job->param.dst2_bg_width) >> 2;

        if (job->param.dest_dma[1].addr == 0) {
            scl_err("dst dma1 addr = 0");
            return -1;
        }
    }

    return 0;
}

// 8*4
#ifdef SPLIT_SUPPORT
int fscaler300_set_split_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;

    //scl_dbg("%s\n", __func__);

    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    job->param.dst_width[0] = property->vproperty[ch_id].dst_width;
    job->param.dst_height[0] = property->vproperty[ch_id].dst_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
    job->param.global.sca_src_format = YUV422;
    job->param.global.src_split_en = 1;
    job->param.split.split_en_15_00 = 0xffffffff;
    job->param.split.split_en_31_16 = 0xffffffff;
    job->param.split.split_bypass_15_00 = 0xffffff55;
    job->param.split.split_bypass_31_16 = 0xffffffff;
    job->param.split.split_sel_07_00 = 0x0000c840;
    job->param.split.split_sel_15_08 = 0x0;
    job->param.split.split_sel_23_16 = 0;
    job->param.split.split_sel_31_24 = 0;
    job->param.split.split_blk_num = 32;
    job->param.global.tc_rb_swap = 0;
    job->param.global.tc_format = YUV422;

    job->param.frm2field.res0_frm2field = 0;

    /*================= SC =================*/
    // channel source type
    job->param.sc.type    = 1;
    job->param.sc.x_start = 8;
    job->param.sc.x_end   = 84;
    job->param.sc.y_start = 4;
    job->param.sc.y_end   = 120;
    job->param.sc.frame_type   = 2;
    job->param.sc.sca_src_width   = 720;
    job->param.sc.sca_src_height   = 480;
    /*================= SD =================*/
    // width/height
    /*  SD0  */
    job->param.sd[0].enable = 0;
    job->param.sd[0].bypass = 0;
    job->param.sd[0].width  = 720;
    job->param.sd[0].height = 480;
    job->param.sd[0].hstep = ((job->param.sc.x_end << 8) / job->param.sd[0].width);
    job->param.sd[0].vstep = ((job->param.sc.y_end << 8) / job->param.sd[0].height);
    /*  SD1  */
    job->param.sd[1].enable = 0;
    job->param.sd[1].bypass = 0;
    job->param.sd[1].width  = 352;
    job->param.sd[1].height = 240;
    job->param.sd[1].hstep = ((job->param.sc.x_end << 8) / job->param.sd[1].width);
    job->param.sd[1].vstep = ((job->param.sc.y_end << 8) / job->param.sd[1].height);
    /*  SD2  */
    job->param.sd[2].enable = 0;
    job->param.sd[2].bypass = 0;
    job->param.sd[2].width  = 176;
    job->param.sd[2].height = 144;
    job->param.sd[2].hstep = ((job->param.sc.x_end << 8) / job->param.sd[2].width);
    job->param.sd[2].vstep = ((job->param.sc.y_end << 8) / job->param.sd[2].height);
    /*  SD3  */
    job->param.sd[3].enable = 0;
    job->param.sd[3].bypass = 0;
    job->param.sd[3].width  = 176;
    job->param.sd[3].height = 120;
    job->param.sd[3].hstep = ((job->param.sc.x_end << 8) / job->param.sd[3].width);
    job->param.sd[3].vstep = ((job->param.sc.y_end << 8) / job->param.sd[3].height);

    /*================= TC =================*/
    /* TC0, Disable */
    job->param.tc[0].enable = 0;
    job->param.tc[0].width  = job->param.sd[0].width;     ///< if tc disbale, tc_wdith  = sd_width
    job->param.tc[0].height = job->param.sd[0].height;    ///< if tc disbale, tc_height = sd_height
    /* TC1, Disable */
    job->param.tc[1].enable = 0;
    job->param.tc[1].width  = job->param.sd[1].width;     ///< if tc disbale, tc_wdith  = sd_width
    job->param.tc[1].height = job->param.sd[1].height;    ///< if tc disbale, tc_height = sd_height
    /* TC2, Disable */
    job->param.tc[2].enable = 0;
    job->param.tc[2].width  = job->param.sd[2].width;     ///< if tc disbale, tc_wdith  = sd_width
    job->param.tc[2].height = job->param.sd[2].height;    ///< if tc disbale, tc_height = sd_height
    /* TC3, Disable */
    job->param.tc[3].enable = 0;
    job->param.tc[3].width  = job->param.sd[3].width;     ///< if tc disbale, tc_wdith  = sd_width
    job->param.tc[3].height = job->param.sd[3].height;    ///< if tc disbale, tc_height = sd_height

    /*================= Link List register ==========*/
    //job->param.lli.job_count = job->job_cnt;
    job->param.lli.job_count = 1;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa;
    job->param.src_dma.line_offset = 0;

    /*================= Destination DMA =================*/
    //ch32 res0 Split mode DMA
    job->param.split_dma[0].res0_addr = job->param.out_addr_pa[0] + 720*360*2 + 84*2*7;
    job->param.split_dma[0].res0_offset = (720-84)/4;
    //ch32 res1 Split mode DMA
    job->param.split_dma[0].res1_addr = job->param.out_addr_pa[0] + 720*480*2;
    job->param.split_dma[0].res1_offset = 0;
    //job->param.split_dma[0].res1_field_offset = (720*240*2) >> 5;
    //ch33 res0 Split mode DMA
    job->param.split_dma[1].res0_addr = job->param.out_addr_pa[0] + 720*360*2 + 84*2*6;
    job->param.split_dma[1].res0_offset = (720-84)/4;
    //ch33 res1 Split mode DMA
    job->param.split_dma[1].res1_addr = job->param.out_addr_pa[0] + 720*480*2*2;
    job->param.split_dma[1].res1_offset = (720-352)/4;
    //ch34 res0 Split mode DMA
    job->param.split_dma[2].res0_addr = job->param.out_addr_pa[0] + 720*360*2 + 84*2*5;
    job->param.split_dma[2].res0_offset = (720-84)/4;
    //ch34 res1 Split mode DMA
    job->param.split_dma[2].res1_addr = job->param.out_addr_pa[0] + 720*480*2*2 + 352*2;
    job->param.split_dma[2].res1_offset = (720-176)/4;
    //ch35 res0 Split mode DMA
    job->param.split_dma[3].res0_addr = job->param.out_addr_pa[0] + 720*360*2 + 84*2*4;
    job->param.split_dma[3].res0_offset = (720-84)/4;
    //ch35 res1 Split mode DMA
    job->param.split_dma[3].res1_addr = job->param.out_addr_pa[0] + 720*480*2*2 + 720*240*2;
    job->param.split_dma[3].res1_offset = (720-176)/4;
    //ch36 res0 Split mode DMA
    job->param.split_dma[4].res0_addr = job->param.out_addr_pa[0] + 720*360*2 + 84*2*3;
    job->param.split_dma[4].res0_offset = (720-84)/4;
    //ch36 res1 Split mode DMA
    job->param.split_dma[4].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3;
    job->param.split_dma[4].res1_offset = (720-84)/4;
    //ch37 res0 Split mode DMA
    job->param.split_dma[5].res0_addr = job->param.out_addr_pa[0] + 720*360*2 + 84*2*2;
    job->param.split_dma[5].res0_offset = (720-84)/4;
    //ch37 res1 Split mode DMA
    job->param.split_dma[5].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 84*2;
    job->param.split_dma[5].res1_offset = (720-84)/4;
    //ch38 res0 Split mode DMA
    job->param.split_dma[6].res0_addr = job->param.out_addr_pa[0] + 720*360*2 + 84*2;
    job->param.split_dma[6].res0_offset = (720-84)/4;
    //ch38 res1 Split mode DMA
    job->param.split_dma[6].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 84*2*2;
    job->param.split_dma[6].res1_offset = (720-84)/4;
    //ch39 res0 Split mode DMA
    job->param.split_dma[7].res0_addr = job->param.out_addr_pa[0] + 720*360*2;
    job->param.split_dma[7].res0_offset = (720-84)/4;
    //ch39 res1 Split mode DMA
    job->param.split_dma[7].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 84*2*3;
    job->param.split_dma[7].res1_offset = (720-84)/4;
    //ch40 res0 Split mode DMA
    job->param.split_dma[8].res0_addr = job->param.out_addr_pa[0] + 720*240*2 + 84*2*7;
    job->param.split_dma[8].res0_offset = (720-84)/4;
    //ch40 res1 Split mode DMA
    job->param.split_dma[8].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 84*2*4;
    job->param.split_dma[8].res1_offset = (720-84)/4;
    //ch41 res0 Split mode DMA
    job->param.split_dma[9].res0_addr = job->param.out_addr_pa[0] + 720*240*2 + 84*2*6;
    job->param.split_dma[9].res0_offset = (720-84)/4;
    //ch41 res1 Split mode DMA
    job->param.split_dma[9].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 84*2*5;
    job->param.split_dma[9].res1_offset = (720-84)/4;
    //ch42 res0 Split mode DMA
    job->param.split_dma[10].res0_addr = job->param.out_addr_pa[0] + 720*240*2 + 84*2*5;
    job->param.split_dma[10].res0_offset = (720-84)/4;
    //ch42 res1 Split mode DMA
    job->param.split_dma[10].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 84*2*6;
    job->param.split_dma[10].res1_offset = (720-84)/4;
    //ch43 res0 Split mode DMA
    job->param.split_dma[11].res0_addr = job->param.out_addr_pa[0] + 720*240*2 + 84*2*4;
    job->param.split_dma[11].res0_offset = (720-84)/4;
    //ch43 res1 Split mode DMA
    job->param.split_dma[11].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 84*2*7;
    job->param.split_dma[11].res1_offset = (720-84)/4;
    //ch44 res0 Split mode DMA
    job->param.split_dma[12].res0_addr = job->param.out_addr_pa[0] + 720*240*2 + 84*2*3;
    job->param.split_dma[12].res0_offset = (720-84)/4;
    //ch44 res1 Split mode DMA
    job->param.split_dma[12].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*120*2;
    job->param.split_dma[12].res1_offset = (720-84)/4;
    //ch45 res0 Split mode DMA
    job->param.split_dma[13].res0_addr = job->param.out_addr_pa[0] + 720*240*2 + 84*2*2;
    job->param.split_dma[13].res0_offset = (720-84)/4;
    //ch45 res1 Split mode DMA
    job->param.split_dma[13].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*120*2 + 84*2;
    job->param.split_dma[13].res1_offset = (720-84)/4;
    //ch46 res0 Split mode DMA
    job->param.split_dma[14].res0_addr = job->param.out_addr_pa[0] + 720 *240*2 + 84*2;
    job->param.split_dma[14].res0_offset = (720-84)/4;
    //ch46 res1 Split mode DMA
    job->param.split_dma[14].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*120*2 + 84*2*2;
    job->param.split_dma[14].res1_offset = (720-84)/4;
    //ch47 res0 Split mode DMA
    job->param.split_dma[15].res0_addr = job->param.out_addr_pa[0] + 720*240*2;
    job->param.split_dma[15].res0_offset = (720-84)/4;
    //ch47 res1 Split mode DMA
    job->param.split_dma[15].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*120*2 + 84*2*3;
    job->param.split_dma[15].res1_offset = (720-84)/4;
    //ch48 res0 Split mode DMA
    job->param.split_dma[16].res0_addr = job->param.out_addr_pa[0] + 720*120*2 + 84*2*7;
    job->param.split_dma[16].res0_offset = (720-84)/4;
    //ch48 res1 Split mode DMA
    job->param.split_dma[16].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*120*2 + 84*2*4;
    job->param.split_dma[16].res1_offset = (720-84)/4;
    //ch49 res0 Split mode DMA
    job->param.split_dma[17].res0_addr = job->param.out_addr_pa[0] + 720*120*2 + 84*2*6;
    job->param.split_dma[17].res0_offset = (720-84)/4;
    //ch49 res1 Split mode DMA
    job->param.split_dma[17].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*120*2 + 84*2*5;
    job->param.split_dma[17].res1_offset = (720-84)/4;
    //ch50 res0 Split mode DMA
    job->param.split_dma[18].res0_addr = job->param.out_addr_pa[0] + 720*120*2 + 84*2*5;
    job->param.split_dma[18].res0_offset = (720-84)/4;
    //ch50 res1 Split mode DMA
    job->param.split_dma[18].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*120*2 + 84*2*6;
    job->param.split_dma[18].res1_offset = (720-84)/4;
    //ch51 res0 Split mode DMA
    job->param.split_dma[19].res0_addr = job->param.out_addr_pa[0] + 720*120*2 + 84*2*4;
    job->param.split_dma[19].res0_offset = (720-84)/4;
    //ch51 res1 Split mode DMA
    job->param.split_dma[19].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*120*2 + 84*2*7;
    job->param.split_dma[19].res1_offset = (720-84)/4;
    //ch52 res0 Split mode DMA
    job->param.split_dma[20].res0_addr = job->param.out_addr_pa[0] + 720*120*2 + 84*2*3;
    job->param.split_dma[20].res0_offset = (720-84)/4;
    //ch52 res1 Split mode DMA
    job->param.split_dma[20].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*240*2;
    job->param.split_dma[20].res1_offset = (720-84)/4;
    //ch53 res0 Split mode DMA
    job->param.split_dma[21].res0_addr = job->param.out_addr_pa[0] + 720*120*2 + 84*2*2;
    job->param.split_dma[21].res0_offset = (720-84)/4;
    //ch53 res1 Split mode DMA
    job->param.split_dma[21].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*240*2 + 84*2;
    job->param.split_dma[21].res1_offset = (720-84)/4;
    //ch54 res0 Split mode DMA
    job->param.split_dma[22].res0_addr = job->param.out_addr_pa[0] + 720*120*2 + 84*2;
    job->param.split_dma[22].res0_offset = (720-84)/4;
    //ch54 res1 Split mode DMA
    job->param.split_dma[22].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*240*2 + 84*2*2;
    job->param.split_dma[22].res1_offset = (720-84)/4;
    //ch55 res0 Split mode DMA
    job->param.split_dma[23].res0_addr = job->param.out_addr_pa[0] + 720*120*2;
    job->param.split_dma[23].res0_offset = (720-84)/4;
    //ch55 res1 Split mode DMA
    job->param.split_dma[23].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*240*2 + 84*2*3;
    job->param.split_dma[23].res1_offset = (720-84)/4;
    //ch56 res0 Split mode DMA
    job->param.split_dma[24].res0_addr = job->param.out_addr_pa[0] + 84*2*7;
    job->param.split_dma[24].res0_offset = (720-84)/4;
    //ch56 res1 Split mode DMA
    job->param.split_dma[24].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*240*2 + 84*2*4;
    job->param.split_dma[24].res1_offset = (720-84)/4;
    //ch57 res0 Split mode DMA
    job->param.split_dma[25].res0_addr = job->param.out_addr_pa[0] + 84*2*6;
    job->param.split_dma[25].res0_offset = (720-84)/4;
    //ch57 res1 Split mode DMA
    job->param.split_dma[25].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*240*2 + 84*2*5;
    job->param.split_dma[25].res1_offset = (720-84)/4;
    //ch58 res0 Split mode DMA
    job->param.split_dma[26].res0_addr = job->param.out_addr_pa[0] + 84*2*5;
    job->param.split_dma[26].res0_offset = (720-84)/4;
    //ch58 res1 Split mode DMA
    job->param.split_dma[26].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*240*2 + 84*2*6;
    job->param.split_dma[26].res1_offset = (720-84)/4;
    //ch59 res0 Split mode DMA
    job->param.split_dma[27].res0_addr = job->param.out_addr_pa[0] + 84*2*4;
    job->param.split_dma[27].res0_offset = (720-84)/4;
    //ch59 res1 Split mode DMA
    job->param.split_dma[27].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*240*2 + 84*2*7;
    job->param.split_dma[27].res1_offset = (720-84)/4;
    //ch60 res0 Split mode DMA
    job->param.split_dma[28].res0_addr = job->param.out_addr_pa[0] + 84*2*3;
    job->param.split_dma[28].res0_offset = (720-84)/4;
    //ch60 res1 Split mode DMA
    job->param.split_dma[28].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*360*2;
    job->param.split_dma[28].res1_offset = (720-84)/4;
    //ch61 res0 Split mode DMA
    job->param.split_dma[29].res0_addr = job->param.out_addr_pa[0] + 84*2*2;
    job->param.split_dma[29].res0_offset = (720-84)/4;
    //ch61 res1 Split mode DMA
    job->param.split_dma[29].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*360*2 + 84*2;
    job->param.split_dma[29].res1_offset = (720-84)/4;
    //ch62 res0 Split mode DMA
    job->param.split_dma[30].res0_addr = job->param.out_addr_pa[0] + 84*2;
    job->param.split_dma[30].res0_offset = (720-84)/4;
    //ch62 res1 Split mode DMA
    job->param.split_dma[30].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*360*2 + 84*2*2;
    job->param.split_dma[30].res1_offset = (720-84)/4;
    //ch63 res0 Split mode DMA
    job->param.split_dma[31].res0_addr = job->param.out_addr_pa[0];
    job->param.split_dma[31].res0_offset = (720-84)/4;
    //ch63 res1 Split mode DMA
    job->param.split_dma[31].res1_addr = job->param.out_addr_pa[0] + 720*480*2*3 + 720*360*2 + 84*2*3;
    job->param.split_dma[31].res1_offset = (720-84)/4;
    return 0;
}
#endif

/*
 * set pip parameter
 */
int fscaler300_set_pip_parameter(job_node_t *node, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;
    int i, path = 0;
    int ch_id = node->ch_id;
    if (flow_check)printm(MODULE_NAME, "%s id %u idx %d\n", __func__, node->job_id, job_idx);
    job->param.src_format = property->pip.src_frm[job_idx];
    job->param.dst_format = property->pip.dst_frm[job_idx];

    job->param.src_width = property->pip.src_width[job_idx];
    job->param.src_height = property->pip.src_height[job_idx];
    // FRM1/FRM2
    if ((property->pip.src_frm[job_idx] == 0x1) || (property->pip.src_frm[job_idx] == 0x2)) {
        job->param.src_bg_width = property->src_bg_width;
        job->param.src_bg_height = property->src_bg_height;
    }
    // FRM3/FRM4
    if ((property->pip.src_frm[job_idx] == 0x4) || (property->pip.src_frm[job_idx] == 0x8)) {
        job->param.src_bg_width = property->src2_bg_width;
        job->param.src_bg_height = property->src2_bg_height;
    }
    // BUF1
    if (property->pip.src_frm[job_idx] == 0x11) {
        job->param.src_bg_width = temp_width;
        job->param.src_bg_height = temp_height;
    }
    // BUF2
    if (property->pip.src_frm[job_idx] == 0x12) {
        job->param.src_bg_width = pip1_width;
        job->param.src_bg_height = pip1_height;
    }

    job->param.src_x = property->pip.src_x[job_idx];
    job->param.src_y = property->pip.src_y[job_idx];

    job->param.dst_width[0] = property->pip.dst_width[job_idx];
    job->param.dst_height[0] = property->pip.dst_height[job_idx];

    // FRM1/FRM2
    if ((property->pip.dst_frm[job_idx] == 0x1) || (property->pip.dst_frm[job_idx] == 0x2)) {
        job->param.dst_bg_width = property->src_bg_width;
        job->param.dst_bg_height = property->src_bg_height;
    }
    // FRM3/FRM4
    if ((property->pip.dst_frm[job_idx] == 0x4) || (property->pip.dst_frm[job_idx] == 0x8)) {
        job->param.dst_bg_width = property->src2_bg_width;
        job->param.dst_bg_height = property->src2_bg_height;
    }
    // BUF1
    if (property->pip.dst_frm[job_idx] == 0x11) {
        job->param.dst_bg_width = temp_width;
        job->param.dst_bg_height = temp_height;
    }
    // BUF2
    if (property->pip.dst_frm[job_idx] == 0x12) {
        job->param.dst_bg_width = pip1_width;
        job->param.dst_bg_height = pip1_height;
    }

    job->param.dst_x[0] = property->pip.dst_x[job_idx];
    job->param.dst_y[0] = property->pip.dst_y[job_idx];

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d idx %d\n", __func__, node->job_id, job_idx);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa %x\n", property->in_addr_pa);
        printm(MODULE_NAME, "in addr pa 1 %x\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa %x\n", property->out_addr_pa);
        printm(MODULE_NAME, "out addr pa 1 %x\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "border enable %x\n", property->vproperty[ch_id].border.enable);
        printm(MODULE_NAME, "border width %x\n", property->vproperty[ch_id].border.width);
        printm(MODULE_NAME, "border pal_idx %x\n", property->vproperty[ch_id].border.pal_idx);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    path = 1;

    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    // width/height
    for (i = 0; i < path; i++) {
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    if ((property->pip.src_dma[job_idx].addr == 0x11) || (property->pip.src_dma[job_idx].addr == 0x12))
        job->param.src_dma.addr = property->pip.src_dma[job_idx].addr;
    else
        job->param.src_dma.addr = property->pip.src_dma[job_idx].addr + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    if ((property->pip.dest_dma[job_idx].addr == 0x11) || (property->pip.dest_dma[job_idx].addr == 0x12))
        job->param.dest_dma[0].addr = property->pip.dest_dma[job_idx].addr;
    else
        job->param.dest_dma[0].addr = property->pip.dest_dma[job_idx].addr + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    return 0;
}

/*  job_idx = 0 : draw horizontal border
 *  job_idx = 1 : draw vertical border
 *  job_idx = 2 : scaling even line
 *  job_idx = 3 : scaling odd line
 *  job_idx = 4 : bypass
 *  job_idx = 5 : scaling entire frame
 */
int fscaler300_set_cvbs_zoom_parameter(job_node_t *node, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;
    int i, path = 0, tmp = 0;
    int border_width = 0;
    int ch_id = node->ch_id;

    if (tmp) {}
    if (flow_check) printm(MODULE_NAME, "%s id %u idx %d\n", __func__, node->job_id, job_idx);

    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;
    /* scaling cvbs frame even line */
    if (job_idx == 2) {
        job->param.src_width = property->src2_bg_width;
        job->param.src_height = property->src2_bg_height >> 1;
        job->param.src_bg_width = property->src2_bg_width;
        job->param.src_bg_height = property->src2_bg_height;
        job->param.src_x = 0;
        job->param.src_y = 0;

        job->param.dst_width[0] = property->win2_bg_width;
        job->param.dst_height[0] = property->win2_bg_height >> 1;
        job->param.dst_bg_width = property->src2_bg_width;
        job->param.dst_bg_height = property->src2_bg_height;
#ifdef WIN2_SUPPORT_XY
        job->param.dst_x[0] = property->win2_x;
        job->param.dst_y[0] = property->win2_y;
#else
        job->param.dst_x[0] = (property->src2_bg_width - property->win2_bg_width) >> 1;
        job->param.dst_y[0] = (property->src2_bg_height - property->win2_bg_height) >> 1;
#endif
    }
    /* scaling cvbs frame odd line */
    if (job_idx == 3) {
        job->param.src_width = property->src2_bg_width;
        job->param.src_height = property->src2_bg_height >> 1;
        job->param.src_bg_width = property->src2_bg_width;
        job->param.src_bg_height = property->src2_bg_height;
        job->param.src_x = 0;
        job->param.src_y = 0;

        job->param.dst_width[0] = property->win2_bg_width;
        job->param.dst_height[0] = property->win2_bg_height >> 1;
        job->param.dst_bg_width = property->src2_bg_width;
        job->param.dst_bg_height = property->src2_bg_height;
#ifdef WIN2_SUPPORT_XY
        job->param.dst_x[0] = property->win2_x;
        job->param.dst_y[0] = property->win2_y;
#else
        job->param.dst_x[0] = (property->src2_bg_width - property->win2_bg_width) >> 1;
        job->param.dst_y[0] = (property->src2_bg_height - property->win2_bg_height) >> 1;
#endif
    }

    /* 1frame2frame/2frame2frame bypass */
    if (job_idx == 4) {
        job->param.src_width = property->src2_bg_width;
        job->param.src_height = property->src2_bg_height;
        job->param.src_bg_width = property->src2_bg_width;
        job->param.src_bg_height = property->src2_bg_height;
        job->param.src_x = 0;
        job->param.src_y = 0;

        job->param.dst_width[0] = property->win2_bg_width;
        job->param.dst_height[0] = property->win2_bg_height;
        job->param.dst_bg_width = property->src2_bg_width;
        job->param.dst_bg_height = property->src2_bg_height;
        job->param.dst_x[0] = (property->src2_bg_width - property->win2_bg_width) >> 1;
        job->param.dst_y[0] = (property->src2_bg_height - property->win2_bg_height) >> 1;
    }
    /* scaling entire frame, source is from 1frame1frame's top frame*/
    if (job_idx == 5) {
        job->param.src_width = property->src_bg_width;
        job->param.src_height = property->src_bg_height;
        job->param.src_bg_width = property->src_bg_width;
        job->param.src_bg_height = property->src_bg_height;
        job->param.src_x = 0;
        job->param.src_y = 0;

        job->param.dst_width[0] = property->win2_bg_width;
        job->param.dst_height[0] = property->win2_bg_height;
        job->param.dst_bg_width = property->src2_bg_width;
        job->param.dst_bg_height = property->src2_bg_height;
#ifdef WIN2_SUPPORT_XY
        job->param.dst_x[0] = property->win2_x;
        job->param.dst_y[0] = property->win2_y;
#else
        job->param.dst_x[0] = (property->src2_bg_width - property->win2_bg_width) >> 1;
        job->param.dst_y[0] = (property->src2_bg_height - property->win2_bg_height) >> 1;
#endif
    }

    /* draw cvbs frame horizontal border */
    if (job_idx == 0) {
        job->param.src_width = property->src2_bg_width;
#ifdef WIN2_SUPPORT_XY
        /* take the max height */
        tmp = property->src2_bg_height - property->win2_bg_height - property->win2_y;
        border_width = (tmp >= property->win2_y) ? tmp : property->win2_y;
        border_width += 2;  /* more pixels for safe due to interlace scaling line by line */
#else
        border_width = (property->src2_bg_height - property->win2_bg_height) >> 1;
        if (border_width % 2 != 0)
            border_width += 2;
#endif
        job->param.src_height = border_width;   //use pattern generation, thus we just tell the height.
        job->param.src_bg_width = property->src2_bg_width;
        job->param.src_bg_height = property->src2_bg_height;
        job->param.src_x = 0;
        job->param.src_y = 0;

        job->param.dst_width[0] = job->param.dst_width[1] = job->param.src_width;
        job->param.dst_height[0] = job->param.dst_height[1] = job->param.src_height;
        job->param.dst_bg_width = property->src2_bg_width;
        job->param.dst_bg_height = property->src2_bg_height;
        job->param.dst_x[0] = 0;
        job->param.dst_y[0] = 0;
        job->param.dst_x[1] = 0;
        job->param.dst_y[1] = property->src2_bg_height - border_width;
    }

    /* draw cvbs frame vertical border */
    if (job_idx == 1) {
#ifdef WIN2_SUPPORT_XY
        /* take the max height */
        tmp = property->src2_bg_width - property->win2_bg_width - property->win2_x;
        border_width = (tmp >= property->win2_x) ? tmp : property->win2_x;
        border_width += 2;  /* more pattern region to avoid interlace line scaling ..... */
#else
        border_width = (property->src2_bg_width - property->win2_bg_width) >> 1;
#endif
        if (border_width % 4 != 0)
            border_width += 2;  /* more lines for safe due to interlace scaling line by line */
        if (border_width < 16)
            job->param.src_width = 16;
        else
            job->param.src_width = border_width;

        job->param.src_height = property->src2_bg_height;
        job->param.src_bg_width = property->src2_bg_width;
        job->param.src_bg_height = property->src2_bg_height;
        job->param.src_x = 0;
        job->param.src_y = 0;

        job->param.dst_width[0] = job->param.dst_width[1] = job->param.src_width;
        job->param.dst_height[0] = job->param.dst_height[1] = job->param.src_height;
        job->param.dst_bg_width = property->src2_bg_width;
        job->param.dst_bg_height = property->src2_bg_height;
        job->param.dst_x[0] = 0;
        job->param.dst_y[0] = 0;
        job->param.dst_x[1] = property->src2_bg_width - job->param.src_width;
        job->param.dst_y[1] = 0;
    }

    job->param.scl_feature = 1;

    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d job idx %d\n", __func__, node->job_id, job_idx);
        printm(MODULE_NAME, "****property [%d] ch %d***\n", job->job_id, ch_id);
        printm(MODULE_NAME, "src_fmt [%d]\n", job->param.src_format);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_width 1[%d]\n", job->param.dst_width[1]);
        printm(MODULE_NAME, "dst_height 1[%d]\n", job->param.dst_height[1]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_x 1[%d]\n", job->param.dst_x[1]);
        printm(MODULE_NAME, "dst_y 1[%d]\n", job->param.dst_y[1]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "dst_win2_x [%d]\n", property->win2_x);
        printm(MODULE_NAME, "dst_win2_y [%d]\n", property->win2_y);
        printm(MODULE_NAME, "scl_feature [%x]\n", property->vproperty[ch_id].scl_feature);
        printm(MODULE_NAME, "in addr pa %x\n", property->in_addr_pa);
        printm(MODULE_NAME, "out addr pa %x\n", property->out_addr_pa);
        printm(MODULE_NAME, "in addr pa 1 %x\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa 1 %x\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "border width %x\n", border_width);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa_1;
    job->param.out_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa_1;
    job->param.in_size = property->in_1_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    if ((job_idx == 0) || (job_idx == 1))
        path = 2;
    else
        path = 1;

    // bypass
    for (i = 0; i < path; i++) {
        if (job->param.src_width == job->param.dst_width[i] && job->param.src_height == job->param.dst_height[i])
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;
        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    if ((job_idx == 0) || (job_idx == 1)) {
        job->param.sc.fix_pat_en = 1;
        job->param.sc.fix_pat_cr = 0x80;
        job->param.sc.fix_pat_cb = 0x80;
        job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = 0x10;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    // width/height
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }

        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
    // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    if (job_idx == 2)
        job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width + job->param.src_bg_width;

    if (job_idx == 3) {
        job->param.src_dma.addr = job->param.in_addr_pa + (job->param.src_bg_width << 1) +
                                ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
        job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width + job->param.src_bg_width;
    }

    /*================= Destination DMA =================*/
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;
    if (job_idx == 0) {
        job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
        job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;
        job->param.dest_dma[1].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[1] * job->param.dst_bg_width + job->param.dst_x[1]) << 1);
        job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - job->param.dst_width[1]) >> 2;

    }
    if (job_idx == 1) {
        job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
        job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;
        job->param.dest_dma[1].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[1] * job->param.dst_bg_width + job->param.dst_x[1]) << 1);
        job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - job->param.dst_width[1]) >> 2;
    }
    if (job_idx == 2) {
        job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
        job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0] + job->param.dst_bg_width) >> 2;
    }
    if (job_idx == 3) {
        job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + (job->param.dst_bg_width << 1) +
                                    ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
        job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0] + job->param.dst_bg_width) >> 2;
    }

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    return 0;
}

/*
 * set rgb565 to rgb565 parameter
 */
int fscaler300_set_rgb565_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;
    int i, path = 0;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);

    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    job->param.dst_width[0] = property->vproperty[ch_id].dst_width;
    job->param.dst_height[0] = property->vproperty[ch_id].dst_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa %x\n", property->in_addr_pa);
        printm(MODULE_NAME, "out addr pa %x\n", property->out_addr_pa);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = RGB565;
    job->param.global.tc_format = RGB565;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    path = 1;

    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    /* hw bug !! when input is rgb555 & rgb565, need to do rbswap at sc level */
    job->param.sc.rb_swap = 1;
    // field ??
#if GM8287  // work around for 8287 scaler line buffer only 1920, but rgb to rgb behavior is rgb to argb to rgb, argb width only 1920/2=960
            // so use rgb to yuv on sc stage, yuv to rgb on tc stage to meet rgb to rgb function
    job->param.sc.rgb2yuv_en = 1;
#endif
    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    // width/height
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    return 0;
}

/*
 * set rgb555 to rgb555 parameter
 */
int fscaler300_set_rgb555_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;
    int i, path = 0;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);

    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    job->param.dst_width[0] = property->vproperty[ch_id].dst_width;
    job->param.dst_height[0] = property->vproperty[ch_id].dst_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa %x\n", property->in_addr_pa);
        printm(MODULE_NAME, "out addr pa %x\n", property->out_addr_pa);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = RGB555;
    job->param.global.tc_format = RGB555;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    path = 1;

    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    /* hw bug !! when input is rgb555 & rgb565, need to do rbswap at sc level */
    job->param.sc.rb_swap = 1;
    // field ??
#if GM8287  // work around for 8287 scaler line buffer only 1920, but rgb to rgb behavior is rgb to argb to rgb, argb width only 1920/2=960
            // so use rgb to yuv on sc stage, yuv to rgb on tc stage to meet rgb to rgb function
    job->param.sc.rgb2yuv_en = 1;
#endif
    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    // width/height
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    return 0;
}

int fscaler300_set_cascade_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx, cas_info_t *cas_info)
{
    fscaler300_property_t *property = &node->property;
    int i, path = 1;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = 0;
    job->param.src_y = 0;


    job->param.dst_width[0] = cas_info->width[ch_id + (job_idx << 1)];
    job->param.dst_height[0] = cas_info->height[ch_id + (job_idx << 1)];
    job->param.dst_bg_width = job->param.dst_width[0];
    job->param.dst_bg_height = job->param.dst_height[0];
    job->param.dst_x[0] = 0;
    job->param.dst_y[0] = 0;

    if (cas_info->width[ch_id + (job_idx << 1) + 1] != 0 && cas_info->height[ch_id + (job_idx << 1) + 1] != 0 ) {
        job->param.dst_width[1] = cas_info->width[ch_id + (job_idx << 1) + 1];
        job->param.dst_height[1] = cas_info->height[ch_id + (job_idx << 1) + 1];
        job->param.dst_bg_width = job->param.dst_width[1];
        job->param.dst_bg_height = job->param.dst_height[1];
        job->param.dst_x[1] = 0;
        job->param.dst_y[1] = 0;
        path++;
    }

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d idx %d\n", __func__, node->job_id, job_idx);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "ratio [%d]\n", property->cas_ratio);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[1]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[1]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa %x\n", property->in_addr_pa);
        printm(MODULE_NAME, "out addr pa %x\n", property->out_addr_pa);
    }

    job->param.in_addr_pa = property->in_addr_pa;

    /* calculate sd0 output address */
    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    property->out_addr_pa = job->param.out_addr_pa[0] +
        ((cas_info->width[ch_id + (job_idx << 1)] * cas_info->height[ch_id + (job_idx << 1)]) << 1);

    /* calculate sd1 output address */
    if (path == 2) {
        job->param.out_addr_pa[1] = property->out_addr_pa;
        job->param.out_1_size = property->out_1_size;
        property->out_addr_pa = job->param.out_addr_pa[1] +
        ((cas_info->width[ch_id + (job_idx << 1) + 1] * cas_info->height[ch_id + (job_idx << 1) + 1]) << 1);
    }

    job->param.in_size = property->in_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    // width/height
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0];
    job->param.dest_dma[0].line_offset = 0;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    job->param.dest_dma[1].addr = job->param.out_addr_pa[1];
    job->param.dest_dma[1].line_offset = 0;

    if (job->param.dest_dma[1].addr == 0) {
        scl_err("dst dma1 addr = 0");
        return -1;
    }

    return 0;
}

/*
 * translate V.G single node property to fscaler300 hw parameter
 */
int fscaler300_set_temporal_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;
    int i, path = 0;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    job->param.dst_width[0] = property->vproperty[ch_id].dst_width;
    job->param.dst_height[0] = property->vproperty[ch_id].dst_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;
    // move source(top/bottom) to temporal buf -- bypass
    if ((job_idx == 0) || (job_idx == 2)) {
        // check boundary
        if (property->vproperty[ch_id].src_width > temp_width) {
            scl_err("src width %d > temp width %d\n", property->vproperty[ch_id].src_width, temp_width);
            printm(MODULE_NAME, "src width %d > temp width %d\n", property->vproperty[ch_id].src_width, temp_width);
            return -1;
        }
        if (property->vproperty[ch_id].src_height > temp_height) {
            scl_err("src height %d > temp height %d\n", property->vproperty[ch_id].src_height, temp_height);
            printm(MODULE_NAME, "src height %d > temp height %d\n", property->vproperty[ch_id].src_height, temp_height);
            return -1;
        }

        job->param.dst_width[0] = property->vproperty[ch_id].src_width;
        job->param.dst_height[0] = property->vproperty[ch_id].src_height;
        job->param.dst_bg_width = temp_width;
        job->param.dst_bg_height = temp_height;
        job->param.dst_x[0] = 0;
        job->param.dst_y[0] = 0;
    }
    // scaling temporal buf to destination(top/bottom) -- scaling up
    if ((job_idx == 1) || (job_idx == 3)) {
        job->param.src_bg_width = temp_width;
        job->param.src_bg_height = temp_height;
        job->param.src_x = 0;
        job->param.src_y = 0;
    }

    /* 2frame to 2frame & frame_frame to frame_frame scaling has aspect ratio */
    if ((job_idx == 1) || (job_idx == 3)) {
        if ((property->src_fmt == TYPE_YUV422_2FRAMES  && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
            (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
            (property->src_fmt == TYPE_YUV422_FRAME_2FRAMES && property->dst_fmt == TYPE_YUV422_FRAME_2FRAMES) ||
            (property->src_fmt == TYPE_YUV422_FRAME_FRAME && property->dst_fmt == TYPE_YUV422_FRAME_FRAME) ||
            (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_FRAME)) {
            if (property->vproperty[ch_id].aspect_ratio.enable == 1) {
                job->param.dst_width[0] = property->vproperty[ch_id].aspect_ratio.dst_width;
                job->param.dst_height[0] = property->vproperty[ch_id].aspect_ratio.dst_height;
                job->param.dst_x[0] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].aspect_ratio.dst_x;
                job->param.dst_y[0] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].aspect_ratio.dst_y;
            }
        }
    }

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;

    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa %x\n", property->in_addr_pa);
        printm(MODULE_NAME, "in addr pa 1 %x\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa %x\n", property->out_addr_pa);
        printm(MODULE_NAME, "out addr pa 1 %x\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "border enable %x\n", property->vproperty[ch_id].border.enable);
        printm(MODULE_NAME, "border width %x\n", property->vproperty[ch_id].border.width);
        printm(MODULE_NAME, "border pal_idx %x\n", property->vproperty[ch_id].border.pal_idx);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }
    // move source to temporal buf
    if ((job_idx == 0) || (job_idx == 2)) {
        job->param.out_addr_pa[0] = 0x82878287;
        job->param.out_size = property->out_size;
        job->param.out_addr_pa[1] = property->out_addr_pa_1;
        job->param.out_1_size = property->out_1_size;
        job->param.in_addr_pa = property->in_addr_pa;
        job->param.in_size = property->in_size;
        job->param.in_addr_pa_1 = property->in_addr_pa_1;
        job->param.in_1_size = property->in_1_size;
    }
    // move temporal buf to destination
    if ((job_idx == 1) || (job_idx == 3)) {
        job->param.out_addr_pa[0] = property->out_addr_pa;
        job->param.out_size = property->out_size;
        job->param.out_addr_pa[1] = property->out_addr_pa_1;
        job->param.out_1_size = property->out_1_size;
        job->param.in_addr_pa = 0x82878287;
        job->param.in_size = property->in_size;
        job->param.in_addr_pa_1 = property->in_addr_pa_1;
        job->param.in_1_size = property->in_1_size;
    }

// 2frame to 2frame scaling
    /* job 2 input from bottom frame, job 1 input from top frame
       job 2 output to bottom frame, job 1 output to top frame */
    if ((property->src_fmt == TYPE_YUV422_2FRAMES  && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (job_idx == 2)
            job->param.in_addr_pa = property->in_addr_pa + (property->src_bg_width * property->src_bg_height << 1);
        if (job_idx == 3)
            job->param.out_addr_pa[0] = property->out_addr_pa + (property->dst_bg_width * property->dst_bg_height << 1);
    }

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    if ((property->src_fmt == TYPE_YUV422_FRAME  && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (job_idx == 0)
            path = 2;
        if (job_idx == 1)
            path = 1;
    }
    else
        path = 1;

    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    // width/height
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    if ((property->src_fmt == TYPE_YUV422_FRAME_2FRAMES  && property->dst_fmt == TYPE_YUV422_FRAME_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_FRAME_FRAME && property->dst_fmt == TYPE_YUV422_FRAME_FRAME) ||
        (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_FRAME)) {
        if (job_idx == 1) {
            if (property->vproperty[ch_id].border.enable == 1) {
                job->param.tc[0].enable = 1;
                job->param.img_border.border_en[0] = 1;
                job->param.img_border.border_width[0] = property->vproperty[ch_id].border.width;
                job->param.img_border.color = property->vproperty[ch_id].border.pal_idx;
            }
        }
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    if (job->param.in_addr_pa != 0x82878287)
        job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    if (job->param.in_addr_pa == 0x82878287)
        job->param.src_dma.addr = 0x82878287;
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    if (job->param.out_addr_pa[0] != 0x82878287)
        job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    if (job->param.out_addr_pa[0] == 0x82878287)
        job->param.dest_dma[0].addr = 0x82878287;

    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }
    return 0;
}

/*
 * when scl_feature = 0 & buf_clean = 1, do buf clean
 */
int fscaler300_set_buf_clean_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;
    int i, path = 0;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    job->param.dst_width[0] = property->vproperty[ch_id].src_width;
    job->param.dst_height[0] = property->vproperty[ch_id].src_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->src_bg_height;
    job->param.dst_x[0] = property->vproperty[ch_id].src_x;
    job->param.dst_y[0] = property->vproperty[ch_id].src_y;

    if ((property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES  && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        job->param.dst_width[1] = property->vproperty[ch_id].src_width;
        job->param.dst_height[1] = property->vproperty[ch_id].src_height;
        job->param.dst_bg_width = property->dst_bg_width;
        job->param.dst_bg_height = property->src_bg_height;
        job->param.dst_x[1] = property->vproperty[ch_id].src_x;
        job->param.dst_y[1] = property->vproperty[ch_id].src_y;
    }

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa %x\n", property->in_addr_pa);
        printm(MODULE_NAME, "in addr pa 1 %x\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa %x\n", property->out_addr_pa);
        printm(MODULE_NAME, "out addr pa 1 %x\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "border enable %x\n", property->vproperty[ch_id].border.enable);
        printm(MODULE_NAME, "border width %x\n", property->vproperty[ch_id].border.width);
        printm(MODULE_NAME, "border pal_idx %x\n", property->vproperty[ch_id].border.pal_idx);
        printm(MODULE_NAME, "buf clean %x\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->in_addr_pa;
    job->param.out_size = property->in_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    if ((property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES  && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES))
        path = 2;
    else
        path = 1;

    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;
    // if buf_clean = 1, use fix pattern to draw frame to black
    job->param.sc.fix_pat_en = 1;
    job->param.sc.fix_pat_cr = (BUF_CLEAN_COLOR >> 16);
    job->param.sc.fix_pat_cb = ((BUF_CLEAN_COLOR & 0xff00) >> 8);
    job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (BUF_CLEAN_COLOR & 0xff);

    /*================= SD =================*/
    // width/height
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    if ((property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        job->param.dest_dma[1].addr = job->param.out_addr_pa[0] + (job->param.dst_bg_width * job->param.dst_bg_height << 1) +
                                        ((job->param.dst_y[1] * job->param.dst_bg_width + job->param.dst_x[1]) << 1);
        job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - job->param.dst_width[1]) >> 2;

        if (job->param.dest_dma[1].addr == 0) {
            scl_err("dst dma1 addr = 0");
            return -1;
        }
    }
    return 0;
}

/*
 * translate V.G single node property to fscaler300 hw parameter
 */
int fscaler300_set_frame_2frame_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;
    int i, path = 0;
    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    job->param.dst_width[0] = property->vproperty[ch_id].dst_width;
    job->param.dst_height[0] = property->vproperty[ch_id].dst_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;

    job->param.dst_width[1] = job->param.dst_width[0];
    job->param.dst_height[1] = job->param.dst_height[0];
    job->param.dst_x[1] = job->param.dst_x[0];
    job->param.dst_y[1] = job->param.dst_y[0];

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_width1 [%d]\n", job->param.dst_width[1]);
        printm(MODULE_NAME, "dst_height1 [%d]\n", job->param.dst_height[1]);
        printm(MODULE_NAME, "dst_x1 [%d]\n", job->param.dst_x[1]);
        printm(MODULE_NAME, "dst_y1 [%d]\n", job->param.dst_y[1]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa %x\n", property->in_addr_pa);
        printm(MODULE_NAME, "in addr pa 1 %x\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa %x\n", property->out_addr_pa);
        printm(MODULE_NAME, "out addr pa 1 %x\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "border enable %x\n", property->vproperty[ch_id].border.enable);
        printm(MODULE_NAME, "border width %x\n", property->vproperty[ch_id].border.width);
        printm(MODULE_NAME, "border pal_idx %x\n", property->vproperty[ch_id].border.pal_idx);
        printm(MODULE_NAME, "buf clean %x\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    path = 2;

    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;
    // if buf_clean = 1, use fix pattern to draw frame to black
    if (property->vproperty[ch_id].buf_clean == 1) {
        job->param.sc.fix_pat_en = 1;
        job->param.sc.fix_pat_cr = (BUF_CLEAN_COLOR >> 16);
        job->param.sc.fix_pat_cb = ((BUF_CLEAN_COLOR & 0xff00) >> 8);
        job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (BUF_CLEAN_COLOR & 0xff);
    }

    /*================= SD =================*/
    // width/height
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    job->param.dest_dma[1].addr = job->param.out_addr_pa[0] + (job->param.dst_bg_width * job->param.dst_bg_height << 1) +
                                    ((job->param.dst_y[1] * job->param.dst_bg_width + job->param.dst_x[1]) << 1);
    job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - job->param.dst_width[1]) >> 2;

    if (job->param.dest_dma[1].addr == 0) {
        scl_err("dst dma1 addr = 0");
        return -1;
    }

    return 0;
}

/*
 * translate V.G single node property to fscaler300 hw parameter
 */
int fscaler300_set_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;
    int i, path = 0;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    job->param.dst_width[0] = property->vproperty[ch_id].dst_width;
    job->param.dst_height[0] = property->vproperty[ch_id].dst_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;

    /* 2frame to 2frame & frame_frame to frame_frame scaling has aspect ratio */
    if ((property->src_fmt == TYPE_YUV422_2FRAMES  && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_FRAME_2FRAMES && property->dst_fmt == TYPE_YUV422_FRAME_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_FRAME_FRAME && property->dst_fmt == TYPE_YUV422_FRAME_FRAME) ||
        (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_FRAME)) {
        if (property->vproperty[ch_id].aspect_ratio.enable == 1) {
            job->param.dst_width[0] = property->vproperty[ch_id].aspect_ratio.dst_width;
            job->param.dst_height[0] = property->vproperty[ch_id].aspect_ratio.dst_height;
            job->param.dst_x[0] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].aspect_ratio.dst_x;
            job->param.dst_y[0] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].aspect_ratio.dst_y;
        }
    }
    /* frame to 3frame scaling has aspect ratio */
    if ((property->src_fmt == TYPE_YUV422_FRAME  && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (job_idx == 0) {
            if (property->vproperty[ch_id].aspect_ratio.enable == 1) {
                job->param.dst_width[0] = property->vproperty[ch_id].aspect_ratio.dst_width;
                job->param.dst_height[0] = property->vproperty[ch_id].aspect_ratio.dst_height;
                job->param.dst_x[0] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].aspect_ratio.dst_x;
                job->param.dst_y[0] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].aspect_ratio.dst_y;
            }
            job->param.dst_width[1] = job->param.dst_width[0];
            job->param.dst_height[1] = job->param.dst_height[0];
            job->param.dst_x[1] = job->param.dst_x[0];
            job->param.dst_y[1] = job->param.dst_y[0];
        }
    }

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa %x\n", property->in_addr_pa);
        printm(MODULE_NAME, "in addr pa 1 %x\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa %x\n", property->out_addr_pa);
        printm(MODULE_NAME, "out addr pa 1 %x\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "border enable %x\n", property->vproperty[ch_id].border.enable);
        printm(MODULE_NAME, "border width %x\n", property->vproperty[ch_id].border.width);
        printm(MODULE_NAME, "border pal_idx %x\n", property->vproperty[ch_id].border.pal_idx);
        printm(MODULE_NAME, "buf clean %x\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

// 2frame to 2frame scaling
    /* job 2 input from bottom frame, job 1 input from top frame
       job 2 output to bottom frame, job 1 output to top frame */
    if ((property->src_fmt == TYPE_YUV422_2FRAMES  && property->dst_fmt == TYPE_YUV422_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (job_idx == 1) {
            job->param.in_addr_pa = property->in_addr_pa + (property->src_bg_width * property->src_bg_height << 1);
            job->param.out_addr_pa[0] = property->out_addr_pa + (property->dst_bg_width * property->dst_bg_height << 1);
        }
    }

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    if ((property->src_fmt == TYPE_YUV422_FRAME  && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (job_idx == 0)
            path = 2;
        if (job_idx == 1)
            path = 1;
    }
    else
        path = 1;

    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;
    // if buf_clean = 1, use fix pattern to draw frame to black
    if (property->vproperty[ch_id].buf_clean == 1) {
        job->param.sc.fix_pat_en = 1;
        job->param.sc.fix_pat_cr = (BUF_CLEAN_COLOR >> 16);
        job->param.sc.fix_pat_cb = ((BUF_CLEAN_COLOR & 0xff00) >> 8);
        job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (BUF_CLEAN_COLOR & 0xff);
    }

    /*================= SD =================*/
    // width/height
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    if ((property->src_fmt == TYPE_YUV422_FRAME_2FRAMES  && property->dst_fmt == TYPE_YUV422_FRAME_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_FRAME_FRAME && property->dst_fmt == TYPE_YUV422_FRAME_FRAME) ||
        (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_FRAME)) {
        if (property->vproperty[ch_id].border.enable == 1) {
            job->param.tc[0].enable = 1;
            job->param.img_border.border_en[0] = 1;
            job->param.img_border.border_width[0] = property->vproperty[ch_id].border.width;
            job->param.img_border.color = property->vproperty[ch_id].border.pal_idx;
        }
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    if ((property->src_fmt == TYPE_YUV422_FRAME  && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (job_idx == 0) {
            job->param.dest_dma[1].addr = job->param.out_addr_pa[0] + (job->param.dst_bg_width * job->param.dst_bg_height << 1) +
                                            ((job->param.dst_y[1] * job->param.dst_bg_width + job->param.dst_x[1]) << 1);
            job->param.dest_dma[1].line_offset = (job->param.dst_bg_width - job->param.dst_width[1]) >> 2;

            if (job->param.dest_dma[1].addr == 0) {
                scl_err("dst dma1 addr = 0");
                return -1;
            }
        }
        if (job_idx == 1) {
            job->param.dest_dma[0].addr = job->param.out_addr_pa[1] +
                                        ((job->param.dst_y[0] * job->param.dst2_bg_width + job->param.dst_x[0]) << 1);
            job->param.dest_dma[0].line_offset = (job->param.dst2_bg_width - job->param.dst_width[0]) >> 2;

            if (job->param.dest_dma[0].addr == 0) {
                scl_err("dst dma0 addr = 0");
                return -1;
            }
        }
    }

#if 0
    atomic_set(&priv.global_param.chn_global[0].osd_pp_sel[0], 1);
    priv.global_param.chn_global[0].osd[0][0].enable = 1;
    priv.global_param.chn_global[0].osd[0][0].task_sel = 0;
    priv.global_param.chn_global[0].osd[0][0].h_wd_num = 5-1;
    priv.global_param.chn_global[0].osd[0][0].v_wd_num = 0;
    priv.global_param.chn_global[0].osd[0][0].x_start = 0;
    priv.global_param.chn_global[0].osd[0][0].x_end = 12*(5+1)*2-1;
    priv.global_param.chn_global[0].osd[0][0].y_start = 0;
    priv.global_param.chn_global[0].osd[0][0].y_end = 18*2-1;
    priv.global_param.chn_global[0].osd[0][0].row_sp = 0;
    priv.global_param.chn_global[0].osd[0][0].col_sp = 0;
    priv.global_param.chn_global[0].osd[0][0].font_alpha = 4;
    priv.global_param.chn_global[0].osd[0][0].bg_alpha = 5;
    priv.global_param.chn_global[0].osd[0][0].bg_color = 12;
    priv.global_param.chn_global[0].osd[0][0].border_en = 1;
    priv.global_param.chn_global[0].osd[0][0].border_width = 1;
    priv.global_param.chn_global[0].osd[0][0].border_color = 2;
    priv.global_param.chn_global[0].osd[0][0].border_type = 0;
    priv.global_param.chn_global[0].osd[0][0].font_zoom = 0;
#endif

#if 0
    atomic_set(&priv.global_param.chn_global[0].mask_pp_sel[0], 1);
    priv.global_param.chn_global[0].mask[0][0].enable = 1;
    priv.global_param.chn_global[0].mask[0][0].x_start = 0;
    priv.global_param.chn_global[0].mask[0][0].x_end = 199;
    priv.global_param.chn_global[0].mask[0][0].y_start = 0;
    priv.global_param.chn_global[0].mask[0][0].y_end = 199;
    priv.global_param.chn_global[0].mask[0][0].color = 1;
    priv.global_param.chn_global[0].mask[0][0].alpha = 7;
    priv.global_param.chn_global[0].mask[0][0].true_hollow = 1;
    priv.global_param.chn_global[0].mask[0][0].border_width = 7;
#endif

    return 0;
}

/*
 * set 1frame_1frame to 1frame_1frame buffer clean parameter
 * use fix pattern to draw lcd0 frame to black
 */
int fscaler300_set_1frame1frame_1frame1frame_buf_clean_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;
    int i, path = 0;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    job->param.dst_width[0] = job->param.src_width;
    job->param.dst_height[0] = job->param.src_height;
    job->param.dst_bg_width = job->param.src_bg_width;
    job->param.dst_bg_height = job->param.src_bg_height;
    job->param.dst_x[0] = job->param.src_x;
    job->param.dst_y[0] = job->param.src_y;

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa %x\n", property->in_addr_pa);
        printm(MODULE_NAME, "in addr pa 1 %x\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa %x\n", property->out_addr_pa);
        printm(MODULE_NAME, "out addr pa 1 %x\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "border enable %x\n", property->vproperty[ch_id].border.enable);
        printm(MODULE_NAME, "border width %x\n", property->vproperty[ch_id].border.width);
        printm(MODULE_NAME, "border pal_idx %x\n", property->vproperty[ch_id].border.pal_idx);
        printm(MODULE_NAME, "buf clean %x\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    path = 1;

    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;
    // if buf_clean = 1, use fix pattern to draw frame to black
    if (property->vproperty[ch_id].buf_clean == 1) {
        job->param.sc.fix_pat_en = 1;
        job->param.sc.fix_pat_cr = (BUF_CLEAN_COLOR >> 16);
        job->param.sc.fix_pat_cb = ((BUF_CLEAN_COLOR & 0xff00) >> 8);
        job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (BUF_CLEAN_COLOR & 0xff);
    }

    /*================= SD =================*/
    // width/height
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma1 addr = 0");
        return -1;
    }

    return 0;
}


/*
 * 2frame_2frame to 2frame_2frame line correction with buffer clean
 */
int fscaler300_set_vch_buf_clean_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;
    int i, path = 0;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    job->param.dst_width[0] = job->param.src_width;
    job->param.dst_height[0] = job->param.src_height;
    job->param.dst_bg_width = job->param.src_bg_width;
    job->param.dst_bg_height = job->param.src_bg_height;
    job->param.dst_x[0] = job->param.src_x;
    job->param.dst_y[0] = job->param.src_y;

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa %x\n", property->in_addr_pa);
        printm(MODULE_NAME, "in addr pa 1 %x\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa %x\n", property->out_addr_pa);
        printm(MODULE_NAME, "out addr pa 1 %x\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "border enable %x\n", property->vproperty[ch_id].border.enable);
        printm(MODULE_NAME, "border width %x\n", property->vproperty[ch_id].border.width);
        printm(MODULE_NAME, "border pal_idx %x\n", property->vproperty[ch_id].border.pal_idx);
        printm(MODULE_NAME, "buf clean %x\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    if (job_idx == 1) {
        job->param.in_addr_pa = property->in_addr_pa + (property->src_bg_width * property->src_bg_height << 1);
        job->param.out_addr_pa[0] = property->out_addr_pa + (property->dst_bg_width * property->dst_bg_height << 1);
    }

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    path = 1;

    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;
    // if buf_clean = 1, use fix pattern to draw frame to black
    if (property->vproperty[ch_id].buf_clean == 1) {
        job->param.sc.fix_pat_en = 1;
        job->param.sc.fix_pat_cr = (BUF_CLEAN_COLOR >> 16);
        job->param.sc.fix_pat_cb = ((BUF_CLEAN_COLOR & 0xff00) >> 8);
        job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (BUF_CLEAN_COLOR & 0xff);
    }

    /*================= SD =================*/
    // width/height
    for (i = 0; i < path; i++) {
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    return 0;
}

/*
 * translate V.G multiple window property to fscaler300 hw parameter
 */
int fscaler300_set_vch_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    job->param.dst_width[0] = job->param.src_width;
    job->param.dst_height[0] = job->param.src_height;
    job->param.dst_bg_width = job->param.src_bg_width;
    job->param.dst_bg_height = job->param.src_bg_height;
    job->param.dst_x[0] = job->param.src_x;
    job->param.dst_y[0] = job->param.src_y;

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

// 2frame to 2frame line correct
    if (job_idx == 0) {
        job->param.in_addr_pa = property->in_addr_pa;
        job->param.out_addr_pa[0] = property->in_addr_pa + (property->src_bg_width * property->src_bg_height << 1);
    }
    else {
        job->param.in_addr_pa = property->in_addr_pa + (property->src_bg_width * property->src_bg_height << 1)
                                + (property->src_bg_width << 1);
        job->param.out_addr_pa[0] = property->in_addr_pa + (property->src_bg_width << 1);
    }
#if 0
    printk("****vch property***\n");
    printk("src_fmt [%d][%d]\n", job->param.src_format, node->job_id);
    printk("dst_fmt [%d]\n", job->param.dst_format);
    printk("src_width [%d]\n", job->param.src_width);
    printk("src_height [%d]\n", job->param.src_height);
    printk("src_bg_width [%d]\n", job->param.src_bg_width);
    printk("src_bg_height [%d]\n", job->param.src_bg_height);
    printk("dst_width [%d]\n", job->param.dst_width[0]);
    printk("dst_height [%d]\n", job->param.dst_height[0]);
    printk("dst_bg_width [%d]\n", job->param.dst_bg_width);
    printk("dst_bg_height [%d]\n", job->param.dst_bg_height);
    printk("scl_feature [%d]\n", job->param.scl_feature);
#endif
    /* ================ scaler300 ==============*/
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    // bypass
    if ((job->param.src_width == job->param.dst_width[0]) && (job->param.src_height == job->param.dst_height[0]))
        job->param.sd[0].bypass = 1;
    else
        job->param.sd[0].bypass = 0;

    job->param.sd[0].enable = 1;
    job->param.tc[0].enable = 0;

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height / 2 - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    // width/height
    if ((job->param.dst_width[0] == 0) || (job->param.dst_height[0] == 0)) {
        scl_err("dst width %d/height %d is zero\n", job->param.dst_width[0], job->param.dst_height[0]);
        return -1;
    }

    job->param.sd[0].width = job->param.src_width;
    job->param.sd[0].height = job->param.src_height;
    // hstep/vstep
    job->param.sd[0].hstep = ((job->param.src_width << 8) / job->param.dst_width[0]);
    job->param.sd[0].vstep = ((job->param.src_height << 8) / job->param.dst_height[0]);

    if (job->param.sc.frame_type != PROGRESSIVE) {
        // top vertical field offset
        if(job->param.src_height > job->param.dst_height[0])
            job->param.sd[0].topvoft = 0;
        else
            job->param.sd[0].topvoft = (256 - job->param.sd[0].vstep) >> 1;
        // bottom vertical field offset
        if(job->param.src_height > job->param.dst_height[0])
            job->param.sd[0].botvoft = (job->param.sd[0].vstep - 256) >> 1;
        else
            job->param.sd[0].botvoft = 0;
    }

    /*================= TC =================*/
    job->param.tc[0].width = job->param.sd[0].width;
    job->param.tc[0].height = job->param.sd[0].height;
    job->param.tc[0].x_start = 0;
    job->param.tc[0].x_end = job->param.sd[0].width - 1;
    job->param.tc[0].y_start = 0;
    job->param.tc[0].y_end = job->param.sd[0].height - 1;

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width + job->param.src_bg_width;

    /*================= Destination DMA =================*/
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0] +
                                          job->param.dst_bg_width) >> 2;
    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }
#if 0   // if line correct need osd, use sd1 to move top/bottom data from source to source
    job->param.sd[1].bypass = 1;
    job->param.sd[1].enable = 1;
    job->param.tc[1].enable = 0;
    /*================= SD =================*/
    // width/height
    job->param.sd[1].width = job->param.src_width;
    job->param.sd[1].height = job->param.src_height;
    // hstep/vstep
    job->param.sd[1].hstep = ((job->param.src_width << 8) / job->param.dst_width);
    job->param.sd[1].vstep = ((job->param.src_height << 8) / job->param.dst_height);

    if(job->param.sc.frame_type != PROGRESSIVE) {
        // top vertical field offset
        if(job->param.src_height > job->param.dst_height)
            job->param.sd[1].topvoft = 0;
        else
            job->param.sd[1].topvoft = (256 - job->param.sd[1].vstep) >> 1;
        // bottom vertical field offset
        if(job->param.src_height > job->param.dst_height)
            job->param.sd[1].botvoft = (job->param.sd[1].vstep - 256) >> 1;
        else
            job->param.sd[1].botvoft = 0;
    }

    /*================= TC =================*/
    job->param.tc[1].width = job->param.sd[1].width;
    job->param.tc[1].height = job->param.sd[1].height;
    job->param.tc[1].x_start = 0;
    job->param.tc[1].x_end = job->param.sd[1].width - 1;
    job->param.tc[1].y_start = 0;
    job->param.tc[1].y_end = job->param.sd[1].height - 1;
    /*================= Destination DMA =================*/
    job->param.dest_dma[1].addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) * 2);
    job->param.dest_dma[1].line_offset = (job->param.src_bg_width - job->param.src_width + job->param.src_bg_width) >> 2;
#endif

#if 0
    printk("****vch scaler300***[%d] job [%d]\n", ch_id, node->job_id);
    printk("src_fmt [%d][%d]\n", job->param.src_format, job_idx);
    printk("dst_fmt [%d]\n", job->param.dst_format);
    printk("src_width [%d]\n", job->param.sc.sca_src_width);
    printk("src_height [%d]\n", job->param.sc.sca_src_height);
    printk("src_bg_width [%d]\n", job->param.src_bg_width);
    printk("src_bg_height [%d]\n", job->param.src_bg_height);
    printk("sc x_start[%d]\n", job->param.sc.x_start);
    printk("sc x_end[%d]\n", job->param.sc.x_end);
    printk("sc y_start[%d]\n", job->param.sc.y_start);
    printk("sc y_end[%d]\n", job->param.sc.y_end);
    printk("dst_width [%d]\n", job->param.tc[0].width);
    printk("dst_height [%d]\n", job->param.tc[0].height);
    printk("dst x_start[%d]\n", job->param.dst_x);
    printk("dst y_start[%d]\n", job->param.dst_y);
    printk("dst_x_end [%d]\n", job->param.tc[0].x_end);
    printk("dst_y_end [%d]\n", job->param.tc[0].y_end);
    printk("source dma [%x]\n", job->param.src_dma.addr);
    printk("source dma line offset [%d]\n", job->param.src_dma.line_offset);
    printk("dst dma [%x]\n", job->param.dest_dma[0].addr);
    printk("dst dma line offset [%d]\n", job->param.dest_dma[0].line_offset);
    printk("scl_feature [%d]\n", job->param.scl_feature);
#endif
    return 0;
}

/*
 * translate V.G multiple path node property to fscaler300 hw parameter
 */
int fscaler300_set_mpath_parameter(job_node_t *mnode[], fscaler300_job_t *job)
{
    int i;
    int ch_id = mnode[0]->ch_id;
    fscaler300_property_t *property[SD_MAX] = {NULL};
    if (flow_check)printm(MODULE_NAME, "%s id %u\n", __func__, job->job_id);
    for (i = 0; i < SD_MAX; i++) {
        if (mnode[i] != NULL)
            property[i] = &mnode[i]->property;
    }

    job->param.src_format = property[0]->src_fmt;
    job->param.dst_format = property[0]->dst_fmt;

    job->param.src_width = property[0]->vproperty[ch_id].src_width;
    job->param.src_height = property[0]->vproperty[ch_id].src_height;
    job->param.src_bg_width = property[0]->src_bg_width;
    job->param.src_bg_height = property[0]->src_bg_height;
    job->param.src_x = property[0]->vproperty[ch_id].src_x;
    job->param.src_y = property[0]->vproperty[ch_id].src_y;

    for (i = 0; i < SD_MAX; i++) {
        if (property[i] != NULL) {
            job->param.dst_width[i] = property[i]->vproperty[mnode[i]->ch_id].dst_width;
            job->param.dst_height[i] = property[i]->vproperty[mnode[i]->ch_id].dst_height;
            job->param.dst_x[i] = property[i]->vproperty[mnode[i]->ch_id].dst_x;
            job->param.dst_y[i] = property[i]->vproperty[mnode[i]->ch_id].dst_y;
        }
    }
    job->param.dst_bg_width = property[0]->dst_bg_width;
    job->param.dst_bg_height = property[0]->dst_bg_height;

    job->param.scl_feature = property[0]->vproperty[ch_id].scl_feature;
    job->param.di_result = property[0]->di_result;
#if 0
    printk("****property***\n");
    printk("src_fmt [%d][%d]\n", job->param.src_format, i);
    printk("dst_fmt [%d]\n", job->param.dst_format);
    printk("src_width [%d]\n", job->param.src_width);
    printk("src_height [%d]\n", job->param.src_height);
    printk("src_bg_width [%d]\n", job->param.src_bg_width);
    printk("src_bg_height [%d]\n", job->param.src_bg_height);
    printk("dst_width [%d]\n", job->param.dst_width);
    printk("dst_height [%d]\n", job->param.dst_height);
    printk("dst_bg_width [%d]\n", job->param.dst_bg_width);
    printk("dst_bg_height [%d]\n", job->param.dst_bg_height);
    printk("scl_feature [%d]\n", job->param.scl_feature);
#endif
    for (i = 0; i < SD_MAX; i++) {
        if (property[i] != NULL)
            job->param.out_addr_pa[i] = property[i]->out_addr_pa;
    }

    //job->param.out_addr_pa = property[0]->out_addr_pa;
    job->param.out_size = property[0]->out_size;
    job->param.in_addr_pa = property[0]->in_addr_pa;
    job->param.in_size = property[0]->in_size;
    job->param.in_addr_pa_1 = property[0]->in_addr_pa_1;
    job->param.in_1_size = property[0]->in_1_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    // bypass
    for (i = 0; i < SD_MAX; i++) {
        if (property[i] != NULL) {
            if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
                job->param.sd[i].bypass = 1;
            else
                job->param.sd[i].bypass = 0;

            job->param.sd[i].enable = 1;
            job->param.tc[i].enable = 0;
        }
    }
    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    for (i = 0; i < SD_MAX; i++) {
        if (property[i] != NULL) {
            if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
                scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
                return -1;
            }
            // width/height
            job->param.sd[i].width = job->param.dst_width[i];
            job->param.sd[i].height = job->param.dst_height[i];
            // hstep/vstep
            job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
            job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

            if (job->param.sc.frame_type != PROGRESSIVE) {
                // top vertical field offset
                if (job->param.src_height > job->param.dst_height[i])
                    job->param.sd[i].topvoft = 0;
                else
                    job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
                // bottom vertical field offset
                if (job->param.src_height > job->param.dst_height[i])
                    job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
                else
                    job->param.sd[i].botvoft = 0;
            }

            /*================= TC =================*/
            job->param.tc[i].width = job->param.sd[i].width;
            job->param.tc[i].height = job->param.sd[i].height;
            job->param.tc[i].x_start = 0;
            job->param.tc[i].x_end = job->param.sd[i].width - 1;
            job->param.tc[i].y_start = 0;
            job->param.tc[i].y_end = job->param.sd[i].height - 1;
        }
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    for (i = 0; i < SD_MAX; i++) {
        if (property[i] != NULL) {
            job->param.dest_dma[i].addr = job->param.out_addr_pa[i] + ((job->param.dst_y[i] * job->param.dst_bg_width + job->param.dst_x[i]) << 1);
            job->param.dest_dma[i].line_offset = (job->param.dst_bg_width - job->param.dst_width[i]) >> 2;

            if (job->param.dest_dma[i].addr == 0) {
                scl_err("dst dma%d addr = 0", i);
                return -1;
            }
        }
    }
#if 0
    printk("****scaler300***\n");
    printk("src_fmt [%d][%d]\n", job->param.src_format, i);
    printk("dst_fmt [%d]\n", job->param.dst_format);
    printk("src_width [%d]\n", job->param.sc.sca_src_width);
    printk("src_height [%d]\n", job->param.sc.sca_src_height);
    printk("src_bg_width [%d]\n", job->param.src_bg_width);
    printk("src_bg_height [%d]\n", job->param.src_bg_height);
    printk("sc x_start[%d]\n", job->param.sc.x_start);
    printk("sc x_end[%d]\n", job->param.sc.x_end);
    printk("sc y_start[%d]\n", job->param.sc.y_start);
    printk("sc y_end[%d]\n", job->param.sc.y_end);
    printk("dst_width [%d]\n", job->param.tc[0].width);
    printk("dst_height [%d]\n", job->param.tc[0].height);
    printk("dst x_start[%d]\n", job->param.dst_x);
    printk("dst y_start[%d]\n", job->param.dst_y);
    printk("dst_x_end [%d]\n", job->param.tc[0].x_end);
    printk("dst_y_end [%d]\n", job->param.tc[0].y_end);
    printk("source dma [%x]\n", job->param.src_dma.addr);
    printk("source dma line offset [%d]\n", job->param.src_dma.line_offset);
    printk("dst dma [%x]\n", job->param.dest_dma[0].addr);
    printk("dst dma line offset [%d]\n", job->param.dest_dma[0].line_offset);
    printk("scl_feature [%d]\n", job->param.scl_feature);
#endif
    return 0;
}

int fscaler300_set_ratio_1frame1frame_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    int i, path = 0;
    fscaler300_property_t *property = &node->property;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);

    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.ratio = fscaler300_select_ratio_source(node, ch_id);

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    /* sub_yuv = 1 means we have ratio frame, ratio = 1 means select source from ratio frame
       ratio frame's width/height is 1/2 source's width/height */
    if (property->sub_yuv == 1 && job->param.ratio == 1) {
        job->param.src_width = property->vproperty[ch_id].src_width >> 1;
        job->param.src_height = property->vproperty[ch_id].src_height >> 1;
        job->param.src_bg_width = property->src_bg_width >> 1;
        job->param.src_bg_height = property->src_bg_height >> 1;
        job->param.src_x = property->vproperty[ch_id].src_x >> 1;
        job->param.src_y = property->vproperty[ch_id].src_y >> 1;
        job->param.src_width = ALIGN_4(job->param.src_width);
        job->param.src_x = ALIGN_4(job->param.src_x);
        job->param.src_bg_width = ALIGN_4(job->param.src_bg_width);
    }
    // path 0 do top frame scaling
    job->param.dst_width[0] = property->vproperty[ch_id].dst_width;
    job->param.dst_height[0] = property->vproperty[ch_id].dst_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;

    if (property->vproperty[ch_id].aspect_ratio.enable == 1) {
        job->param.dst_width[0] = property->vproperty[ch_id].aspect_ratio.dst_width;
        job->param.dst_height[0] = property->vproperty[ch_id].aspect_ratio.dst_height;
        job->param.dst_x[0] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].aspect_ratio.dst_x;
        job->param.dst_y[0] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].aspect_ratio.dst_y;
    }

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;

    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "dst_bg_width 2[%d]\n", job->param.dst2_bg_width);
        printm(MODULE_NAME, "dst_bg_height 2[%d]\n", job->param.dst2_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa [%x]\n", property->in_addr_pa);
        printm(MODULE_NAME, "in addr pa 1[%x]\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa [%x]\n", property->out_addr_pa);
        printm(MODULE_NAME, "out addr pa 1[%x]\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "sub yuv %d ratio %d\n", property->sub_yuv, job->param.ratio);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    /* source from ratio frame */
    if (property->sub_yuv == 1 && job->param.ratio == 1)
        job->param.in_addr_pa = property->in_addr_pa_1;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    path = 1;   // path 0 do scaling
    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    if (property->vproperty[ch_id].border.enable == 1) {
        job->param.tc[0].enable = 1;
        job->param.img_border.border_en[0] = 1;
        job->param.img_border.border_width[0] = property->vproperty[ch_id].border.width;
        job->param.img_border.color = property->vproperty[ch_id].border.pal_idx;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    /* top frame scaling */
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] +
                                ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);

    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    return 0;
}

int fscaler300_set_ratio_1frame2frame_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    int i, path = 0;
    fscaler300_property_t *property = &node->property;
    int border_width = 0;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);

    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;

    job->param.ratio = fscaler300_select_ratio_source(node, ch_id);

    job->param.src_width = property->vproperty[ch_id].src_width;
    job->param.src_height = property->vproperty[ch_id].src_height;
    job->param.src_bg_width = property->src_bg_width;
    job->param.src_bg_height = property->src_bg_height;
    job->param.src_x = property->vproperty[ch_id].src_x;
    job->param.src_y = property->vproperty[ch_id].src_y;

    /* sub_yuv = 1 means we have ratio frame, ratio = 1 means select source from ratio frame
       ratio frame's width/height is 1/2 source's width/height */
    if (property->sub_yuv == 1 && job->param.ratio == 1) {
        job->param.src_width = property->vproperty[ch_id].src_width >> 1;
        job->param.src_height = property->vproperty[ch_id].src_height >> 1;
        job->param.src_bg_width = property->src_bg_width >> 1;
        job->param.src_bg_height = property->src_bg_height >> 1;
        job->param.src_x = property->vproperty[ch_id].src_x >> 1;
        job->param.src_y = property->vproperty[ch_id].src_y >> 1;
        job->param.src_width = ALIGN_4(job->param.src_width);
        job->param.src_x = ALIGN_4(job->param.src_x);
        job->param.src_bg_width = ALIGN_4(job->param.src_bg_width);
    }
    // path 0 do top frame scaling
    job->param.dst_width[0] = property->vproperty[ch_id].dst_width;
    job->param.dst_height[0] = property->vproperty[ch_id].dst_height;
    job->param.dst_bg_width = property->dst_bg_width;
    job->param.dst_bg_height = property->dst_bg_height;
    job->param.dst2_bg_width = property->dst2_bg_width;
    job->param.dst2_bg_height = property->dst2_bg_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst_y;

    if (property->vproperty[ch_id].aspect_ratio.enable == 1) {
        job->param.dst_width[0] = property->vproperty[ch_id].aspect_ratio.dst_width;
        job->param.dst_height[0] = property->vproperty[ch_id].aspect_ratio.dst_height;
        job->param.dst_x[0] = property->vproperty[ch_id].dst_x + property->vproperty[ch_id].aspect_ratio.dst_x;
        job->param.dst_y[0] = property->vproperty[ch_id].dst_y + property->vproperty[ch_id].aspect_ratio.dst_y;
    }

    // path 1 do CVBS frame scaling
    job->param.dst_width[1] = property->vproperty[ch_id].dst2_width;
    job->param.dst_height[1] = property->vproperty[ch_id].dst2_height;
    job->param.dst_x[1] = property->vproperty[ch_id].dst2_x;
    job->param.dst_y[1] = property->vproperty[ch_id].dst2_y;

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;

    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_width 1[%d]\n", job->param.dst_width[1]);
        printm(MODULE_NAME, "dst_height 1[%d]\n", job->param.dst_height[1]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_x1 [%d]\n", job->param.dst_x[1]);
        printm(MODULE_NAME, "dst_y1 [%d]\n", job->param.dst_y[1]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "dst_bg_width 2[%d]\n", job->param.dst2_bg_width);
        printm(MODULE_NAME, "dst_bg_height 2[%d]\n", job->param.dst2_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", job->param.scl_feature);
        printm(MODULE_NAME, "in addr pa [%x]\n", property->in_addr_pa);
        printm(MODULE_NAME, "in addr pa 1[%x]\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa [%x]\n", property->out_addr_pa);
        printm(MODULE_NAME, "out addr pa 1[%x]\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "sub yuv %d ratio %d\n", property->sub_yuv, job->param.ratio);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    /* source from ratio frame */
    if (property->sub_yuv == 1 && job->param.ratio == 1)
        job->param.in_addr_pa = property->in_addr_pa_1;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    path = 2;   // path 0 do scaling, path 1 do scaling
    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    /*================= SD =================*/
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        // width/height
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    if (property->vproperty[ch_id].border.enable == 1) {
        job->param.tc[0].enable = 1;
        job->param.img_border.border_en[0] = 1;
        job->param.img_border.border_width[0] = property->vproperty[ch_id].border.width;

        job->param.tc[1].enable = 1;
        job->param.img_border.border_en[1] = 1;
        border_width = fscaler300_set_cvbs_border_width(node, ch_id);
        job->param.img_border.border_width[1] = border_width;
        job->param.img_border.color = property->vproperty[ch_id].border.pal_idx;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    job->param.src_dma.addr = job->param.in_addr_pa + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    /* top frame scaling */
    job->param.dest_dma[0].addr = job->param.out_addr_pa[0] +
                                ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);

    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }
    /* CVBS frame scalig */
    job->param.dest_dma[1].addr = job->param.out_addr_pa[1] +
                                ((job->param.dst_y[1] * job->param.dst2_bg_width + job->param.dst_x[1]) << 1);

    job->param.dest_dma[1].line_offset = (job->param.dst2_bg_width - job->param.dst_width[1]) >> 2;

    if (job->param.dest_dma[1].addr == 0) {
        scl_err("dst dma1 addr = 0");
        return -1;
    }

    return 0;
}


int fscaler300_set_cvbs_parameter(job_node_t *node, int ch_id, fscaler300_job_t *job, int job_idx)
{
    fscaler300_property_t *property = &node->property;
    int i, path = 0, border_width = 0;

    if (flow_check)printm(MODULE_NAME, "%s id %u ch %d idx %d scl %d\n", __func__, node->job_id, ch_id, job_idx, job->param.scl_feature);
    job->param.src_format = property->src_fmt;
    job->param.dst_format = property->dst_fmt;
    if ((property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_FRAME_2FRAMES && property->dst_fmt == TYPE_YUV422_FRAME_2FRAMES)) {
        job->param.src_width = property->vproperty[ch_id].src2_width;
        job->param.src_height = property->vproperty[ch_id].src2_height;
        job->param.src_bg_width = property->src2_bg_width;
        job->param.src_bg_height = property->src2_bg_height;
        job->param.src_x = property->vproperty[ch_id].src2_x;
        job->param.src_y = property->vproperty[ch_id].src2_y;
    }

    if ((property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_FRAME_2FRAMES)) {
        job->param.src_width = property->vproperty[ch_id].src_width;
        job->param.src_height = property->vproperty[ch_id].src_height;
        job->param.src_bg_width = property->src_bg_width;
        job->param.src_bg_height = property->src_bg_height;
        job->param.src_x = property->vproperty[ch_id].src_x;
        job->param.src_y = property->vproperty[ch_id].src_y;
    }

    job->param.dst_width[0] = property->vproperty[ch_id].dst2_width;
    job->param.dst_height[0] = property->vproperty[ch_id].dst2_height;
    job->param.dst_x[0] = property->vproperty[ch_id].dst2_x;
    job->param.dst_y[0] = property->vproperty[ch_id].dst2_y;

    if ((property->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_FRAME_2FRAMES && property->dst_fmt == TYPE_YUV422_FRAME_2FRAMES)) {
        job->param.dst_bg_width = property->src2_bg_width;
        job->param.dst_bg_height = property->src2_bg_height;
    }
    if ((property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
        (property->src_fmt == TYPE_YUV422_FRAME && property->dst_fmt == TYPE_YUV422_FRAME_2FRAMES)) {
        job->param.dst_bg_width = property->dst2_bg_width;
        job->param.dst_bg_height = property->dst2_bg_height;
    }

    job->param.scl_feature = property->vproperty[ch_id].scl_feature;
    job->param.di_result = property->di_result;
    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d\n", __func__, node->job_id);
        printm(MODULE_NAME, "****property [%d]***\n", job->job_id);
        printm(MODULE_NAME, "src_fmt [%d][%d]\n", job->param.src_format, ch_id);
        printm(MODULE_NAME, "dst_fmt [%d]\n", job->param.dst_format);
        printm(MODULE_NAME, "src_width [%d]\n", job->param.src_width);
        printm(MODULE_NAME, "src_x [%d]\n", job->param.src_x);//by WayneWei
        printm(MODULE_NAME, "src_y [%d]\n", job->param.src_y);//by WayneWei
    	printm(MODULE_NAME, "src_height [%d]\n", job->param.src_height);//by WayneWei
        printm(MODULE_NAME, "src_bg_width [%d]\n", job->param.src_bg_width);
        printm(MODULE_NAME, "src_bg_height [%d]\n", job->param.src_bg_height);
        printm(MODULE_NAME, "dst_width [%d]\n", job->param.dst_width[0]);
        printm(MODULE_NAME, "dst_height [%d]\n", job->param.dst_height[0]);
        printm(MODULE_NAME, "dst_x [%d]\n", job->param.dst_x[0]);
        printm(MODULE_NAME, "dst_y [%d]\n", job->param.dst_y[0]);
        printm(MODULE_NAME, "dst_bg_width [%d]\n", job->param.dst_bg_width);
        printm(MODULE_NAME, "dst_bg_height [%d]\n", job->param.dst_bg_height);
        printm(MODULE_NAME, "scl_feature [%d]\n", property->vproperty[ch_id].scl_feature);
        printm(MODULE_NAME, "in addr pa %x\n", property->in_addr_pa);
        printm(MODULE_NAME, "out addr pa %x\n", property->out_addr_pa);
        printm(MODULE_NAME, "in addr pa 1 %x\n", property->in_addr_pa_1);
        printm(MODULE_NAME, "out addr pa 1 %x\n", property->out_addr_pa_1);
        printm(MODULE_NAME, "buf clean %d\n", property->vproperty[ch_id].buf_clean);
    }

    job->param.out_addr_pa[0] = property->out_addr_pa;
    job->param.out_size = property->out_size;
    job->param.out_addr_pa[1] = property->out_addr_pa_1;
    job->param.out_1_size = property->out_1_size;
    job->param.in_addr_pa = property->in_addr_pa;
    job->param.in_size = property->in_size;
    job->param.in_addr_pa_1 = property->in_addr_pa_1;
    job->param.in_1_size = property->in_1_size;

    /* ================ scaler300 ============== */
    /* global */
    job->param.global.capture_mode = LINK_LIST_MODE;
#ifdef SINGLE_FIRE
    job->param.global.capture_mode = SINGLE_FRAME_MODE;
#endif
    job->param.global.sca_src_format = YUV422;
    if (job->param.dst_format == TYPE_RGB565)
        job->param.global.tc_format = RGB565;
    else
        job->param.global.tc_format = YUV422;
    job->param.global.src_split_en = 0;
    job->param.global.dma_job_wm = 8;
    job->param.global.dma_fifo_wm = 16;

    /* ================ subchannel ==============*/
    path = 1;

    // bypass
    for (i = 0; i < path; i++) {
        if ((job->param.src_width == job->param.dst_width[i]) && (job->param.src_height == job->param.dst_height[i]))
            job->param.sd[i].bypass = 1;
        else
            job->param.sd[i].bypass = 0;

        job->param.sd[i].enable = 1;
        job->param.tc[i].enable = 0;
    }

    if (property->vproperty[ch_id].border.enable == 1) {
        job->param.tc[0].enable = 1;
        job->param.img_border.border_en[0] = 1;
        border_width = fscaler300_set_cvbs_border_width(node, ch_id);
        job->param.img_border.border_width[0] = border_width;
        job->param.img_border.color = property->vproperty[ch_id].border.pal_idx;
    }

    /*================= SC =================*/
    // channel source type
    job->param.sc.type = 0;
    // frame type
    job->param.sc.frame_type = PROGRESSIVE;
    // field ??

    // x position
    job->param.sc.x_start = 0;
    job->param.sc.x_end = job->param.src_width - 1;
    // y position
    job->param.sc.y_start = 0;
    job->param.sc.y_end = job->param.src_height - 1;
    // source
    job->param.sc.sca_src_width = job->param.src_width;
    job->param.sc.sca_src_height = job->param.src_height;

    // if buf_clean = 1, use fix pattern to draw frame to black
    if (property->vproperty[ch_id].buf_clean == 1) {
        job->param.sc.fix_pat_en = 1;
        job->param.sc.fix_pat_cr = (BUF_CLEAN_COLOR >> 16);
        job->param.sc.fix_pat_cb = ((BUF_CLEAN_COLOR & 0xff00) >> 8);
        job->param.sc.fix_pat_y0 = job->param.sc.fix_pat_y1 = (BUF_CLEAN_COLOR & 0xff);
    }

    /*================= SD =================*/
    // width/height
    for (i = 0; i < path; i++) {
        if ((job->param.dst_width[i] == 0) || (job->param.dst_height[i] == 0)) {
            scl_err("dst width %d/height %d is zero\n", job->param.dst_width[i], job->param.dst_height[i]);
            return -1;
        }
        job->param.sd[i].width = job->param.dst_width[i];
        job->param.sd[i].height = job->param.dst_height[i];
        // hstep/vstep
        job->param.sd[i].hstep = ((job->param.src_width << 8) / job->param.dst_width[i]);
        job->param.sd[i].vstep = ((job->param.src_height << 8) / job->param.dst_height[i]);

        if (job->param.sc.frame_type != PROGRESSIVE) {
            // top vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].topvoft = 0;
            else
                job->param.sd[i].topvoft = (256 - job->param.sd[i].vstep) >> 1;
            // bottom vertical field offset
            if (job->param.src_height > job->param.dst_height[i])
                job->param.sd[i].botvoft = (job->param.sd[i].vstep - 256) >> 1;
            else
                job->param.sd[i].botvoft = 0;
        }
    }

    /*================= TC =================*/
    for (i = 0; i < path; i++) {
        job->param.tc[i].width = job->param.sd[i].width;
        job->param.tc[i].height = job->param.sd[i].height;
        job->param.tc[i].x_start = 0;
        job->param.tc[i].x_end = job->param.sd[i].width - 1;
        job->param.tc[i].y_start = 0;
        job->param.tc[i].y_end = job->param.sd[i].height - 1;
        /* when scaling size is too small (ex: height 1080->1078) will cause hstep/vstep still = 256,
           scaler will still output 1080 line, we have to use target cropping to crop necessary line (ex: 1078) */
        if ((job->param.src_width != job->param.dst_width[i]) ||
            (job->param.src_height != job->param.dst_height[i]) ||
            ((job->param.sd[i].bypass == 0) && (job->param.sd[i].hstep == 256) && (job->param.sd[i].vstep == 256)))
            job->param.tc[i].enable = 1;
    }

    /*================= Link List register ==========*/
    job->param.lli.job_count = job->job_cnt;

    /*================= Source DMA =================*/
    //if (job->param.src_format == TYPE_YUV422_FRAME)
      //  job->param.in_addr_pa_1 = job->param.in_addr_pa;

    job->param.src_dma.addr = job->param.in_addr_pa_1 + ((job->param.src_y * job->param.src_bg_width + job->param.src_x) << 1);
    job->param.src_dma.line_offset = job->param.src_bg_width - job->param.src_width;

    /*================= Destination DMA =================*/
    if (job->param.src_format == TYPE_YUV422_2FRAMES_2FRAMES)
        job->param.out_addr_pa[1] = job->param.out_addr_pa[0] + (property->dst_bg_width * property->dst_bg_height << 2);

    if (job->param.src_format == TYPE_YUV422_FRAME_2FRAMES)
        job->param.out_addr_pa[1] = job->param.out_addr_pa[0] + (property->dst_bg_width * property->dst_bg_height << 1);

    job->param.dest_dma[0].addr = job->param.out_addr_pa[1] + ((job->param.dst_y[0] * job->param.dst_bg_width + job->param.dst_x[0]) << 1);
    job->param.dest_dma[0].line_offset = (job->param.dst_bg_width - job->param.dst_width[0]) >> 2;

    if (job->param.dest_dma[0].addr == 0) {
        scl_err("dst dma0 addr = 0");
        return -1;
    }

    return 0;
}
#if 0   /* mask & osd external api do not use, remove it for performance */
void fscaler300_get_mask_info(fscaler300_job_t *job)
{
    int i, chn, mpath, pp_sel, win_idx;
    fscaler300_global_param_t *global_param = &priv.global_param;

    if (job->param.src_format == TYPE_RGB565)
        return;

    if (job->job_type == MULTI_SJOB)
        mpath = 4;
    else
        mpath = 1;
    /* get current use mask ping-pong buffer */
    for (i = 0; i < mpath; i++) {
        if (!job->input_node[i].enable)
            continue;

        chn = job->input_node[i].minor;
        for (win_idx = 0; win_idx < MASK_MAX; win_idx++) {
            job->mask_pp_sel[i][win_idx] = atomic_read(&global_param->chn_global[chn].mask_pp_sel[win_idx]);
            if (!job->mask_pp_sel[i][win_idx])
                continue;

            pp_sel = job->mask_pp_sel[i][win_idx] - 1;
            /* if mask window enable, this job is reference to mask ping-pong buffer */
            if (global_param->chn_global[chn].mask[pp_sel][win_idx].enable)
                MASK_REF_INC(&global_param->chn_global[chn], pp_sel, win_idx);
        }
    }
}

void fscaler300_get_osd_info(fscaler300_job_t *job)
{
    int i, chn, mpath, pp_sel, win_idx;
    fscaler300_global_param_t *global_param = &priv.global_param;

    if (job->param.src_format == TYPE_RGB565 || job->param.src_format == TYPE_RGB555)
        return;

    if (job->job_type == MULTI_SJOB)
        mpath = 4;
    else
        mpath = 1;
    /* get current use osd ping-pong buffer */
    for (i = 0; i < mpath; i++) {
        if (!job->input_node[i].enable)
            continue;

        chn = job->input_node[i].minor;
        for (win_idx = 0; win_idx < OSD_MAX; win_idx++) {
            job->osd_pp_sel[i][win_idx] = atomic_read(&global_param->chn_global[chn].osd_pp_sel[win_idx]);
            if (!job->osd_pp_sel[i][win_idx])
                continue;

            pp_sel = job->osd_pp_sel[i][win_idx] - 1;
            /* if osd window enable, this job is reference to osd ping-pong buffer */
            if (global_param->chn_global[chn].osd[pp_sel][win_idx].enable)
                OSD_REF_INC(&global_param->chn_global[chn], pp_sel, win_idx);
        }
    }
}
#endif // end of if 0
#ifdef SPLIT_SUPPORT
void fscaler300_get_split_job_info(fscaler300_job_t *job)
{
    int i = 0;

    job->split_flow.count = 3;

    /* get split 7~13 count */
    for (i = 7; i < 14; i++) {
        if ((job->param.split.split_en_15_00 >> (i * 2)) & 0x3)
            job->split_flow.ch_7_13 = 1;
    }
    if (job->split_flow.ch_7_13)
        job->split_flow.count++;
    /* get split 14~20 count */
    for (i = 14; i < 16; i++) {
        if ((job->param.split.split_en_15_00 >> (i * 2)) & 0x3)
            job->split_flow.ch_14_20 = 1;
    }
    for (i = 0; i < 5; i++) {
        if ((job->param.split.split_en_31_16 >> (i * 2)) & 0x3)
            job->split_flow.ch_14_20 = 1;
    }
    if (job->split_flow.ch_14_20)
        job->split_flow.count++;
    /* get split 21~27 count */
    for (i = 5; i < 12; i++) {
        if ((job->param.split.split_en_31_16 >> (i * 2)) & 0x3)
            job->split_flow.ch_21_27 = 1;
    }
    if (job->split_flow.ch_21_27)
        job->split_flow.count++;
    /* get split 28~31 count */
    for (i = 12; i < 16; i++) {
        if ((job->param.split.split_en_31_16 >> (i * 2)) & 0x3)
            job->split_flow.ch_28_31 = 1;
    }
    if (job->split_flow.ch_28_31)
        job->split_flow.count++;
}
#endif

void fscaler300_get_job_info(fscaler300_job_t *job)
{
    int i;//, chn, mpath, pp_sel, win_idx;
    //fscaler300_global_param_t *global_param = &priv.global_param;
#if 0   /* mask & osd external api do not use, remove it for performance */
    /* selete mask ping-pong buffer & virtual job no mask */
    if ((job->param.sc.fix_pat_en == 0) && (job->param.scl_feature & (1 << 0)))
        fscaler300_get_mask_info(job);

    /* selete osd ping-pong buffer & virtual job no osd */
#if GM8210
    if ((job->param.sc.fix_pat_en == 0) && (job->param.scl_feature & (1 << 0)))
        fscaler300_get_osd_info(job);
#endif
#endif // end of if 0
    job->ll_flow.count = 1;
    /* get sd123 count */
    for (i = 1; i < SD_MAX; i++)
        if (job->param.sd[i].enable == 1)
            job->ll_flow.res123 = 1;
#if 0   /* mask & osd external api do not use, remove it for performance */
    /* get mask4567 count */
    /* if multi job with same source address, 4 channel merge into 1 job, mask setting maybe wrong!!!
       because each channel all have mask, output will show each channel mask!!! */
    if (job->param.sc.fix_pat_en == 0) {
        if (job->job_type == MULTI_SJOB)
            mpath = 4;
        else
            mpath = 1;
        /* get mask count */
        /* virtual job & cvbs zoom no mask */
        if (job->param.scl_feature & (3 << 1))
            mpath = 0;

        for (i = 0; i < mpath; i++) {
            if (!job->input_node[i].enable)
                continue;

            chn = job->input_node[i].minor;
            for (win_idx = 4; win_idx < MASK_MAX; win_idx++) {
                if (!job->mask_pp_sel[i][win_idx])
                    continue;

                pp_sel = job->mask_pp_sel[i][win_idx] - 1;
                if (global_param->chn_global[chn].mask[pp_sel][win_idx].enable)
                    job->ll_flow.mask4567 = 1;
            }
        }
        /* get osd count */
        /* virtual job & cvbs zoom no osd */
#if GM8210
        if (job->param.scl_feature & (3 << 1))
            mpath = 0;

        for (i = 0; i < mpath; i++) {
            if (!job->input_node[i].enable)
                continue;

            chn = job->input_node[i].minor;
            for (win_idx = 0; win_idx < OSD_MAX; win_idx++) {
                if (!job->osd_pp_sel[i][win_idx])
                    continue;

                pp_sel = job->osd_pp_sel[i][win_idx] - 1;
                if (global_param->chn_global[chn].osd[pp_sel][win_idx].enable) {
                    if (win_idx <= 6)
                        job->ll_flow.osd0123456 = 1;
                    else
                        job->ll_flow.osd7 = 1;
                }
            }
        }
#endif
    }
#endif // end of if 0
    // total blocks of this job
    if (job->ll_flow.res123 || job->ll_flow.mask4567)
        job->ll_flow.count++;
#if GM8210
    if (job->ll_flow.osd0123456)
        job->ll_flow.count++;
    if (job->ll_flow.osd7)
        job->ll_flow.count++;
#endif
}

/*
 * alloc fscaler_job_t & translate property to fscaler300 register for single path when border enable
 */
int fscaler300_init_aspect_ratio_border_job(fscaler300_job_t *job, job_node_t *node, int ch_id, int job_idx)
{
    int ret = 0;

    if (debug_mode >= 2)
        printm(MODULE_NAME, "%s %d job idx %d\n", __func__, node->job_id, job_idx);

    INIT_LIST_HEAD(&job->plist);
    INIT_LIST_HEAD(&job->clist);

    /* hook fops to job */
    job->input_node[0].enable = 1;
    job->input_node[0].idx = node->idx;
    job->input_node[0].engine = node->engine;
    job->input_node[0].minor = node->minor;
    job->input_node[0].fops = &callback_ops;
    job->input_node[0].private = node;
    job->status = JOB_STS_QUEUE;
    job->priority = node->priority;
    job->job_id = node->job_id;

    /* translate V.G property to fscaler300 hw parameter */
    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES) {
        if (node->property.vproperty[ch_id].src_interlace == 0) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_frm2field_parameter(node, ch_id, job, job_idx);
                    break;
                case 2:
                    if (node->property.vproperty[ch_id].aspect_ratio.type == H_TYPE)
                        ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    if (node->property.vproperty[ch_id].aspect_ratio.type == V_TYPE)
                        ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
            if (ret < 0)
                return -1;
        }
        if (node->property.vproperty[ch_id].src_interlace == 1) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_ratio_parameter(node, ch_id, job, 0);
                    break;
                case 2:
                    ret = fscaler300_set_ratio_parameter(node, ch_id, job, 1);
                    break;
                case 3:
                    if (node->property.vproperty[ch_id].aspect_ratio.type == H_TYPE)
                        ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    if (node->property.vproperty[ch_id].aspect_ratio.type == V_TYPE)
                        ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
            if (ret < 0)
                return -1;
        }
    }

    if ((node->property.src_fmt == TYPE_YUV422_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES) ||
        (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        switch (job_idx) {
            case 0:
                ret = fscaler300_set_parameter(node, ch_id, job, 0);
                break;
            case 1:
                ret = fscaler300_set_parameter(node, ch_id, job, 1);
                break;
            case 2:
                ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                break;
            case 3:
                ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                break;
            case 4:
                ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                break;
            case 5:
                ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                break;
            default :
                break;
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (node->property.vproperty[ch_id].src_interlace == 0) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_frm2field_parameter(node, ch_id, job, job_idx);
                    break;
                case 2:
                    ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                    break;
                case 3:
                    if ((node->property.vproperty[ch_id].dst_width << 1) + node->property.vproperty[ch_id].dst2_width > SCALER300_TOTAL_RESOLUTION)
                        ret = fscaler300_set_cvbs_border_h_parameter(node, ch_id, job, job_idx);
                    else
                        ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                case 4:
                    ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
            if (ret < 0)
                return -1;
        }
        if (node->property.vproperty[ch_id].src_interlace == 1) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_ratio_parameter(node, ch_id, job, 0);
                    break;
                case 2:
                    ret = fscaler300_set_ratio_parameter(node, ch_id, job, 1);
                    break;
                case 3:
                    ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                    break;
                case 4:
                    if ((node->property.vproperty[ch_id].dst_width << 1) + node->property.vproperty[ch_id].dst2_width > SCALER300_TOTAL_RESOLUTION)
                        ret = fscaler300_set_cvbs_border_h_parameter(node, ch_id, job, job_idx);
                    else
                        ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                case 5:
                    ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
            if (ret < 0)
                return -1;
        }
    }

    if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        switch (job_idx) {
            case 0:
                ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                break;
            case 1:
                ret = fscaler300_set_parameter(node, ch_id, job, 0);
                break;
            case 2:
                ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                break;
            case 3:
                ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                break;
            case 4:
                ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                break;
            default :
                break;
        }
        if (ret < 0)
            return -1;
    }

    fscaler300_get_job_info(job);

    return 0;
}

/*
 * alloc fscaler_job_t & translate property to fscaler300 register for single path when border enable
 */
int fscaler300_init_aspect_ratio_job(fscaler300_job_t *job, job_node_t *node, int ch_id, int job_idx)
{
    int ret = 0;

    if (debug_mode >= 2)
        printm(MODULE_NAME, "%s %d job_idx %d\n", __func__, node->job_id, job_idx);

    INIT_LIST_HEAD(&job->plist);
    INIT_LIST_HEAD(&job->clist);

    /* hook fops to job */
    job->input_node[0].enable = 1;
    job->input_node[0].idx = node->idx;
    job->input_node[0].engine = node->engine;
    job->input_node[0].minor = node->minor;
    job->input_node[0].fops = &callback_ops;
    job->input_node[0].private = node;
    job->status = JOB_STS_QUEUE;
    job->priority = node->priority;
    job->job_id = node->job_id;

    /* translate V.G property to fscaler300 hw parameter */
    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES) {
        if (node->property.vproperty[ch_id].src_interlace == 0) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_frm2field_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
        }
        if (node->property.vproperty[ch_id].src_interlace == 1) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_ratio_parameter(node, ch_id, job, 0);
                    break;
                case 2:
                    ret = fscaler300_set_ratio_parameter(node, ch_id, job, 1);
                    break;
                default :
                    break;
            }
        }
        if (ret < 0)
            return -1;
    }

    if ((node->property.src_fmt == TYPE_YUV422_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES) ||
        (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (node->property.in_addr_pa == node->property.out_addr_pa) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 1);
                    break;
                case 2:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 2);
                    break;
                case 3:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 3);
                    break;
                case 4:
                    ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                case 5:
                    ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
        }
        else {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_parameter(node, ch_id, job, 1);
                    break;
                case 2:
                    ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                case 3:
                    ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (node->property.vproperty[ch_id].src_interlace == 0) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_frm2field_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
        }
        if (node->property.vproperty[ch_id].src_interlace == 1) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_ratio_parameter(node, ch_id, job, 0);
                    break;
                case 2:
                    ret = fscaler300_set_ratio_parameter(node, ch_id, job, 1);
                    break;
                default :
                    break;
            }
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        switch (job_idx) {
            case 0:
                ret = fscaler300_set_fix_bg_parameter(node, ch_id, job, job_idx);
                break;
            case 1:
                ret = fscaler300_set_parameter(node, ch_id, job, 0);
                break;
            case 2:
                ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                break;
            default :
                break;
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) {
        switch (job_idx) {
            case 0:
                ret = fscaler300_set_frame_fix_bg_parameter(node, ch_id, job, job_idx);
                break;
            case 1:
                ret = fscaler300_set_ratio_1frame2frame_parameter(node, ch_id, job, job_idx);
                break;
            default :
                break;
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_FRAME_2FRAMES && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) {
        switch (job_idx) {
            case 0:
                ret = fscaler300_set_parameter(node, ch_id, job, 0);
                break;
            case 1:
                ret = fscaler300_set_frame_fix_bg_parameter(node, ch_id, job, job_idx);
                break;
            case 2:
                ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                break;
            default :
                break;
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
        switch (job_idx) {
            case 0:
                ret = fscaler300_set_frame_fix_bg_parameter(node, ch_id, job, job_idx);
                break;
            case 1:
                ret = fscaler300_set_ratio_1frame1frame_parameter(node, ch_id, job, job_idx);
                break;
            default :
                break;
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME) {
        switch (job_idx) {
            case 0:
                ret = fscaler300_set_frame_fix_bg_parameter(node, ch_id, job, job_idx);
                break;
            case 1:
                ret = fscaler300_set_ratio_1frame1frame_parameter(node, ch_id, job, job_idx);
                break;
            default :
                break;
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
        if (node->property.in_addr_pa == node->property.out_addr_pa) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 1);
                    break;
                case 2:
                    ret = fscaler300_set_frame_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
        }
        else {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_frame_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME) {
        if (node->property.in_addr_pa == node->property.out_addr_pa) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 1);
                    break;
                case 2:
                    ret = fscaler300_set_frame_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
        }
        else {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_frame_fix_bg_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
        }
        if (ret < 0)
            return -1;
    }


    fscaler300_get_job_info(job);

    return 0;
}

/*
 * alloc fscaler_job_t & translate property to fscaler300 register for single path when border enable
 */
int fscaler300_init_border_job(fscaler300_job_t *job, job_node_t *node, int ch_id, int job_idx)
{
    int ret = 0;

    if (debug_mode >= 2)
        printm(MODULE_NAME, "%s %d job_idx %d\n", __func__, node->job_id, job_idx);

    INIT_LIST_HEAD(&job->plist);
    INIT_LIST_HEAD(&job->clist);

    /* hook fops to job */
    job->input_node[0].enable = 1;
    job->input_node[0].idx = node->idx;
    job->input_node[0].engine = node->engine;
    job->input_node[0].minor = node->minor;
    job->input_node[0].fops = &callback_ops;
    job->input_node[0].private = node;
    job->status = JOB_STS_QUEUE;
    job->priority = node->priority;
    job->job_id = node->job_id;

    /* translate V.G property to fscaler300 hw parameter */
    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES) {
        if (node->property.vproperty[ch_id].src_interlace == 0) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_frm2field_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                    break;
                case 2:
                    ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
        }
        if (node->property.vproperty[ch_id].src_interlace == 1) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_ratio_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_ratio_parameter(node, ch_id, job, job_idx);
                    break;
                case 2:
                    ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                    break;
                case 3:
                    ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
        }
        if (ret < 0)
            return -1;
    }

    if ((node->property.src_fmt == TYPE_YUV422_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES) ||
        (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
        if (node->property.vproperty[ch_id].scl_feature & (1 << 0)) {
            if (node->property.in_addr_pa == node->property.out_addr_pa) {
                switch (job_idx) {
                    case 0:
                        ret = fscaler300_set_temporal_parameter(node, ch_id, job, 0);
                        break;
                    case 1:
                        ret = fscaler300_set_temporal_parameter(node, ch_id, job, 1);
                        break;
                    case 2:
                        ret = fscaler300_set_temporal_parameter(node, ch_id, job, 2);
                        break;
                    case 3:
                        ret = fscaler300_set_temporal_parameter(node, ch_id, job, 3);
                        break;
                    case 4:
                        ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                        break;
                    case 5:
                        ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                        break;
                    case 6:
                        if ((node->property.vproperty[ch_id].src2_width == node->property.vproperty[ch_id].dst2_width) &&
                            (node->property.vproperty[ch_id].src2_height == node->property.vproperty[ch_id].dst2_height))
                            ret = fscaler300_set_cvbs_border_h_parameter(node, ch_id, job, job_idx);
                        else
                            ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                        break;
                    case 7:
                        ret = fscaler300_set_cvbs_border_v_parameter(node, ch_id, job, job_idx);
                        break;
                    default :
                        break;
                }
            }
            else {
                switch (job_idx) {
                    case 0:
                        ret = fscaler300_set_parameter(node, ch_id, job, job_idx);
                        break;
                    case 1:
                        ret = fscaler300_set_parameter(node, ch_id, job, job_idx);
                        break;
                    case 2:
                        ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                        break;
                    case 3:
                        ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                        break;
                    case 4:
                        if ((node->property.vproperty[ch_id].src2_width == node->property.vproperty[ch_id].dst2_width) &&
                            (node->property.vproperty[ch_id].src2_height == node->property.vproperty[ch_id].dst2_height))
                            ret = fscaler300_set_cvbs_border_h_parameter(node, ch_id, job, job_idx);
                        else
                            ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                        break;
                    case 5:
                        ret = fscaler300_set_cvbs_border_v_parameter(node, ch_id, job, job_idx);
                        break;
                    default :
                        break;
                }
            }
            if (ret < 0)
                return -1;
        }
        if (node->property.vproperty[ch_id].scl_feature & (1 << 1)) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_vch_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_vch_parameter(node, ch_id, job, job_idx);
                    break;
                case 2:
                    ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                    break;
                case 3:
                    ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
            if (ret < 0)
                return -1;
        }
    }

    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        if (node->property.vproperty[ch_id].src_interlace == 0) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_frm2field_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                    break;
                case 2:
                    if ((node->property.vproperty[ch_id].dst_width << 1) + node->property.vproperty[ch_id].dst2_width > SCALER300_TOTAL_RESOLUTION)
                        ret = fscaler300_set_cvbs_border_h_parameter(node, ch_id, job, job_idx);
                    else
                        ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                case 3:
                    ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
            if (ret < 0)
                return -1;
        }
        if (node->property.vproperty[ch_id].src_interlace == 1) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_ratio_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_ratio_parameter(node, ch_id, job, job_idx);
                    break;
                case 2:
                    ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                    break;
                case 3:
                    if ((node->property.vproperty[ch_id].dst_width << 1) + node->property.vproperty[ch_id].dst2_width > SCALER300_TOTAL_RESOLUTION)
                        ret = fscaler300_set_cvbs_border_h_parameter(node, ch_id, job, job_idx);
                    else
                        ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                case 4:
                    ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
            if (ret < 0)
                return -1;
        }
    }

    if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        switch (job_idx) {
                case 0:
                    ret = fscaler300_set_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_border_h_parameter(node, ch_id, job, job_idx);
                    break;
                case 2:
                    ret = fscaler300_set_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                case 3:
                    ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) {
        ret = fscaler300_set_ratio_1frame2frame_parameter(node, ch_id, job, job_idx);
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_FRAME_2FRAMES && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) {
        switch (job_idx) {
            case 0:
                ret = fscaler300_set_parameter(node, ch_id, job, 0);
                break;
            case 1:
                ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                break;
            default :
                break;
        }
        if (ret < 0)
            return -1;
    }
#if GM8210
    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
        ret = fscaler300_set_ratio_1frame1frame_parameter(node, ch_id, job, job_idx);
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME) {
        ret = fscaler300_set_ratio_1frame1frame_parameter(node, ch_id, job, job_idx);
        if (ret < 0)
            return -1;
    }
#else
    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
        switch (job_idx) {
            case 0:
                ret = fscaler300_set_ratio_1frame1frame_parameter(node, ch_id, job, job_idx);
                break;
            case 1:
                ret = fscaler300_set_ratio_1frame1frame_border_h_parameter(node, ch_id, job, job_idx);
                break;
            case 2:
                ret = fscaler300_set_ratio_1frame1frame_border_v_parameter(node, ch_id, job, job_idx);
                break;
            default :
                break;
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME) {
        switch (job_idx) {
            case 0:
                ret = fscaler300_set_ratio_1frame1frame_parameter(node, ch_id, job, job_idx);
                break;
            case 1:
                ret = fscaler300_set_ratio_1frame1frame_border_h_parameter(node, ch_id, job, job_idx);
                break;
            case 2:
                ret = fscaler300_set_ratio_1frame1frame_border_v_parameter(node, ch_id, job, job_idx);
                break;
            default :
                break;
        }
        if (ret < 0)
            return -1;
    }
#endif
    if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
        if (node->property.in_addr_pa == node->property.out_addr_pa) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 1);
                    break;
                default :
                    break;
                }
        }
        else
            ret = fscaler300_set_parameter(node, ch_id, job, job_idx);
        if (ret < 0)
            return -1;
    }
#if GM8210
    if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME) {
        if (node->property.in_addr_pa == node->property.out_addr_pa) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 1);
                    break;
                default :
                    break;
                }
        }
        else
            ret = fscaler300_set_parameter(node, ch_id, job, job_idx);
        if (ret < 0)
            return -1;
    }
#else
    if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME) {
        if (node->property.in_addr_pa == node->property.out_addr_pa) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_temporal_parameter(node, ch_id, job, 1);
                    break;
                case 2:
                    ret = fscaler300_set_ratio_1frame1frame_border_h_parameter(node, ch_id, job, job_idx);
                    break;
                case 3:
                    ret = fscaler300_set_ratio_1frame1frame_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
                }
        }
        else {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_parameter(node, ch_id, job, job_idx);
                    break;
                case 1:
                    ret = fscaler300_set_ratio_1frame1frame_border_h_parameter(node, ch_id, job, job_idx);
                    break;
                case 2:
                    ret = fscaler300_set_ratio_1frame1frame_border_v_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
        }
        if (ret < 0)
            return -1;
    }
#endif

    fscaler300_get_job_info(job);

    return 0;
}

int fscaler300_init_pip_job(fscaler300_job_t *job, job_node_t *node, int job_idx)
{
    int ret = 0;

    INIT_LIST_HEAD(&job->plist);
    INIT_LIST_HEAD(&job->clist);

    /* hook fops to job */
    job->input_node[0].enable = 1;
    job->input_node[0].idx = node->idx;
    job->input_node[0].engine = node->engine;
    job->input_node[0].minor = node->minor;
    job->input_node[0].fops = &callback_ops;
    job->input_node[0].private = node;
    job->status = JOB_STS_QUEUE;
    job->priority = node->priority;
    job->job_id = node->job_id;

    ret = fscaler300_set_pip_parameter(node, job, job_idx);

    fscaler300_get_job_info(job);

    return 0;
}

int fscaler300_init_cvbs_zoom_job(fscaler300_job_t *job, job_node_t *node, int job_idx)
{
    int ret = 0;

    if (debug_mode >= 2)
        printm(MODULE_NAME, "%s %d job_idx %d\n", __func__, node->job_id, job_idx);

    INIT_LIST_HEAD(&job->plist);
    INIT_LIST_HEAD(&job->clist);

    /* hook fops to job */
    job->input_node[0].enable = 1;
    job->input_node[0].idx = node->idx;
    job->input_node[0].engine = node->engine;
    job->input_node[0].minor = node->minor;
    job->input_node[0].fops = &callback_ops;
    job->input_node[0].private = node;
    job->status = JOB_STS_QUEUE;
    job->priority = node->priority;
    job->job_id = node->job_id;

    /* 1frame1frame --> source from top frame */
    /* 1frame2frame/2frame2frame --> source from cvbs frame */
    if (node->property.src2_bg_width == node->property.win2_bg_width && node->property.src2_bg_height == node->property.win2_bg_height) {
        if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)
            ret = fscaler300_set_cvbs_zoom_parameter(node, job, 5);
        else
            ret = fscaler300_set_cvbs_zoom_parameter(node, job, 4);
    }

    if (node->property.src2_bg_width != node->property.win2_bg_width && node->property.src2_bg_height != node->property.win2_bg_height) {
        switch (job_idx) {
            case 0:     // draw cvbs horizontal border
                ret = fscaler300_set_cvbs_zoom_parameter(node, job, 0);
                break;
            case 1:     // draw cvbs vertical border
                ret = fscaler300_set_cvbs_zoom_parameter(node, job, 1);
                break;
            case 2:
                /* 1frame1frame to 1frame1frame --> scaling entire frame */
                /* 1frame2frame / 2frame2frame --> scaling cvbs even line */
                if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)
                    ret = fscaler300_set_cvbs_zoom_parameter(node, job, 5);
                else
                    ret = fscaler300_set_cvbs_zoom_parameter(node, job, 2);
                break;
            case 3:     // scaling cvbs odd line
                ret = fscaler300_set_cvbs_zoom_parameter(node, job, 3);
                break;
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src2_bg_width != node->property.win2_bg_width && node->property.src2_bg_height == node->property.win2_bg_height) {
        switch (job_idx) {
            case 0:     // draw cvbs vertical border
                ret = fscaler300_set_cvbs_zoom_parameter(node, job, 1);
                break;
            case 1:
                /* 1frame1frame to 1frame1frame --> scaling entire frame */
                /* 1frame2frame / 2frame2frame --> scaling cvbs even line */
                if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)
                    ret = fscaler300_set_cvbs_zoom_parameter(node, job, 5);
                else
                    ret = fscaler300_set_cvbs_zoom_parameter(node, job, 2);
                break;
            case 2:     // scaling cvbs odd line
                ret = fscaler300_set_cvbs_zoom_parameter(node, job, 3);
                break;
        }
        if (ret < 0)
            return -1;
    }

    if (node->property.src2_bg_width == node->property.win2_bg_width && node->property.src2_bg_height != node->property.win2_bg_height) {
        switch (job_idx) {
            case 0:     // draw cvbs horizontal border
                ret = fscaler300_set_cvbs_zoom_parameter(node, job, 0);
                break;
            case 1:
                /* 1frame1frame to 1frame1frame --> scaling entire frame */
                /* 1frame2frame to 1frame2frame / 2frame2frame to 2frame2frame --> scaling cvbs even line */
                if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)
                    ret = fscaler300_set_cvbs_zoom_parameter(node, job, 5);
                else
                    ret = fscaler300_set_cvbs_zoom_parameter(node, job, 2);
                break;
            case 2:     // scaling cvbs odd line
                ret = fscaler300_set_cvbs_zoom_parameter(node, job, 3);
                break;
        }
        if (ret < 0)
            return -1;
    }

    fscaler300_get_job_info(job);

    return 0;
}

int fscaler300_init_cas_job(fscaler300_job_t *job, job_node_t *node, int ch_id, int job_idx, cas_info_t *cas_info)
{
    int ret = 0;

    INIT_LIST_HEAD(&job->plist);
    INIT_LIST_HEAD(&job->clist);

    /* hook fops to job */
    job->input_node[0].enable = 1;
    job->input_node[0].idx = node->idx;
    job->input_node[0].engine = node->engine;
    job->input_node[0].minor = node->minor;
    job->input_node[0].fops = &callback_ops;
    job->input_node[0].private = node;
    job->status = JOB_STS_QUEUE;
    job->priority = node->priority;
    job->job_id = node->job_id;

    ret = fscaler300_set_cascade_parameter(node, ch_id, job, job_idx, cas_info);
    if (ret < 0)
        return -1;

    fscaler300_get_job_info(job);

    return 0;
}

/*
 * alloc fscaler_job_t & translate property to fscaler300 register for single path
 */
int fscaler300_init_job(fscaler300_job_t *job, job_node_t *node, int ch_id, int job_idx)
{
    int ret = 0;

    if (debug_mode >= 2)
        printm(MODULE_NAME, "%s %d job_idx %d\n", __func__, node->job_id, job_idx);

    INIT_LIST_HEAD(&job->plist);
    INIT_LIST_HEAD(&job->clist);

    /* hook fops to job */
    job->input_node[0].enable = 1;
    job->input_node[0].idx = node->idx;
    job->input_node[0].engine = node->engine;
    job->input_node[0].minor = node->minor;
    job->input_node[0].fops = &callback_ops;
    job->input_node[0].private = node;
    job->status = JOB_STS_QUEUE;
    job->priority = node->priority;
    job->job_id = node->job_id;

    /* translate V.G property to fscaler300 hw parameter */
    if ((node->property.vproperty[ch_id].scl_feature == 0 && node->property.vproperty[ch_id].buf_clean == 1) ||
        (node->property.vproperty[ch_id].scl_feature == (1 << 4) && node->property.vproperty[ch_id].buf_clean == 1) ||
        (node->property.vproperty[ch_id].scl_feature == (1 << 5) && node->property.vproperty[ch_id].buf_clean == 1)) {
        ret = fscaler300_set_buf_clean_parameter(node, ch_id, job, job_idx);
        if (ret < 0)
            return -1;
    }

    if (node->property.vproperty[ch_id].scl_feature & (1 << 0)) {
        if ((node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES) ||
            (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
            if (node->property.vproperty[ch_id].src_interlace == 0)     // progressive
                ret = fscaler300_set_frm2field_parameter(node, ch_id, job, job_idx);
            if (node->property.vproperty[ch_id].src_interlace == 1)     // interlace
                ret = fscaler300_set_ratio_parameter(node, ch_id, job, job_idx);
            if (ret < 0)
                return -1;
        }

        if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME) {
            if (node->property.vproperty[ch_id].src_interlace == 0) {    // progressive
#if 1 /* Harry, bug fix: 2014/11/17 08:45¤U¤È. Just follow the path as TYPE_YUV422_FRAME_FRAME */
                if (node->property.in_addr_pa == node->property.out_addr_pa) {
                    if (node->property.vproperty[ch_id].buf_clean == 0)
                        ret = fscaler300_set_temporal_parameter(node, ch_id, job, job_idx);
                    else
                        ret = fscaler300_set_parameter(node, ch_id, job, job_idx);
                } else {
                    ret = fscaler300_set_parameter(node, ch_id, job, job_idx);
                }
#else /* original code */
                ret = fscaler300_set_parameter(node, ch_id, job, job_idx);
#endif
            }
            if (node->property.vproperty[ch_id].src_interlace == 1) {    // interlace
#if 1 /* Harry, bug fix */
                if (node->property.in_addr_pa == node->property.out_addr_pa) {
                    if (node->property.vproperty[ch_id].buf_clean == 0)
                        ret = fscaler300_set_temporal_parameter(node, ch_id, job, job_idx);
                    else
                        ret = fscaler300_set_interlace_parameter(node, ch_id, job, job_idx);
                } else {
                    ret = fscaler300_set_interlace_parameter(node, ch_id, job, job_idx);
                }
#else /* original code */
                ret = fscaler300_set_interlace_parameter(node, ch_id, job, job_idx);
#endif
            }

            if (ret < 0)
                return -1;
        }

        if (node->property.src_fmt == TYPE_RGB565 && node->property.dst_fmt == TYPE_RGB565) {
            ret = fscaler300_set_rgb565_parameter(node, ch_id, job, job_idx);
            if (ret < 0)
                return -1;
        }

        if (node->property.src_fmt == TYPE_RGB555 && node->property.dst_fmt == TYPE_RGB555) {
            ret = fscaler300_set_rgb555_parameter(node, ch_id, job, job_idx);
            if (ret < 0)
                return -1;
        }

        if ((node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_RGB565) ||
            //(node->property.src_fmt == TYPE_YUV422_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES) ||
            (node->property.src_fmt == TYPE_YUV422_FIELDS && node->property.dst_fmt == TYPE_YUV422_FRAME)) {
            ret = fscaler300_set_parameter(node, ch_id, job, job_idx);
            if (ret < 0)
                return -1;
        }

        if (node->property.src_fmt == TYPE_YUV422_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES) {
            if (node->property.in_addr_pa == node->property.out_addr_pa) {
                if (node->property.vproperty[ch_id].buf_clean == 0) {
                    switch (job_idx) {
                        case 0:
                            ret = fscaler300_set_temporal_parameter(node, ch_id, job, 0);
                            break;
                        case 1:
                            ret = fscaler300_set_temporal_parameter(node, ch_id, job, 1);
                            break;
                        case 2:
                            ret = fscaler300_set_temporal_parameter(node, ch_id, job, 2);
                            break;
                        case 3:
                            ret = fscaler300_set_temporal_parameter(node, ch_id, job, 3);
                            break;
                        default :
                            break;
                    }
                }
                if (node->property.vproperty[ch_id].buf_clean == 1) {
                    switch (job_idx) {
                        case 0:
                            ret = fscaler300_set_parameter(node, ch_id, job, 0);
                            break;
                        case 1:
                            ret = fscaler300_set_parameter(node, ch_id, job, 1);
                            break;
                        default :
                            break;
                    }
                }
                if (ret < 0)
                    return -1;
            }
            else {
                switch (job_idx) {
                    case 0:
                        ret = fscaler300_set_parameter(node, ch_id, job, 0);
                        break;
                    case 1:
                        ret = fscaler300_set_parameter(node, ch_id, job, 1);
                        break;
                    default :
                        break;
                }
                if (ret < 0)
                    return -1;
            }
        }

        if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_cvbs_parameter(node, ch_id, job, 1);
                    break;
                default :
                    break;
            }
            if (ret < 0)
                return -1;
        }

        if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
            if (node->property.in_addr_pa == node->property.out_addr_pa) {
                if (node->property.vproperty[ch_id].buf_clean == 0) {
                    switch (job_idx) {
                        case 0:
                            ret = fscaler300_set_temporal_parameter(node, ch_id, job, 0);
                            break;
                        case 1:
                            ret = fscaler300_set_temporal_parameter(node, ch_id, job, 1);
                            break;
                        case 2:
                            ret = fscaler300_set_temporal_parameter(node, ch_id, job, 2);
                            break;
                        case 3:
                            ret = fscaler300_set_temporal_parameter(node, ch_id, job, 3);
                            break;
                        case 4:
                            ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                            break;
                        default :
                            break;
                    }
                }
                if (node->property.vproperty[ch_id].buf_clean == 1) {
                    switch (job_idx) {
                        case 0:
                            ret = fscaler300_set_parameter(node, ch_id, job, 0);
                            break;
                        case 1:
                            ret = fscaler300_set_parameter(node, ch_id, job, 1);
                            break;
                        case 2:
                            ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                            break;
                        default :
                            break;
                    }
                }
                if (ret < 0)
                    return -1;
            }
            else {
                switch (job_idx) {
                    case 0:
                        ret = fscaler300_set_parameter(node, ch_id, job, 0);
                        break;
                    case 1:
                        ret = fscaler300_set_parameter(node, ch_id, job, 1);
                        break;
                    case 2:
                        ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                        break;
                    default :
                        break;
                }
                if (ret < 0)
                    return -1;
            }
        }

        if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) {
            ret = fscaler300_set_ratio_1frame2frame_parameter(node, ch_id, job, job_idx);
            if (ret < 0)
                return -1;
        }

        if (node->property.src_fmt == TYPE_YUV422_FRAME_2FRAMES && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_cvbs_parameter(node, ch_id, job, job_idx);
                    break;
                default :
                    break;
            }
            if (ret < 0)
                return -1;
        }

        if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) {
            switch (job_idx) {
                case 0:
                    ret = fscaler300_set_parameter(node, ch_id, job, 0);
                    break;
                case 1:
                    ret = fscaler300_set_cvbs_parameter(node, ch_id, job, 1);
                    break;
                default :
                    break;
            }
            if (ret < 0)
                return -1;
        }

        if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
            ret = fscaler300_set_ratio_1frame1frame_parameter(node, ch_id, job, job_idx);
            if (ret < 0)
                return -1;
        }

        if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
            ret = fscaler300_set_parameter(node, ch_id, job, 0);
            if (ret < 0)
                return -1;
        }

        if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
            if (node->property.in_addr_pa == node->property.out_addr_pa) {
                if (node->property.vproperty[ch_id].buf_clean == 0) {
                    switch (job_idx) {
                        case 0:
                            ret = fscaler300_set_temporal_parameter(node, ch_id, job, 0);
                            break;
                        case 1:
                            ret = fscaler300_set_temporal_parameter(node, ch_id, job, 1);
                            break;
                        default :
                            break;
                    }
                }
                if (node->property.vproperty[ch_id].buf_clean == 1)
                    ret = fscaler300_set_parameter(node, ch_id, job, 0);
            }
            else
                ret = fscaler300_set_parameter(node, ch_id, job, 0);
            if (ret < 0)
                return -1;
        }

        if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME) {
            ret = fscaler300_set_ratio_1frame1frame_parameter(node, ch_id, job, job_idx);
            if (ret < 0)
                return -1;
        }

        if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES) {
            ret = fscaler300_set_frame_2frame_parameter(node, ch_id, job, job_idx);
            if (ret < 0)
                return -1;
        }
    }

    if (node->property.vproperty[ch_id].scl_feature & (1 << 1)) {
        if (node->property.vproperty[ch_id].buf_clean == 1)
            ret = fscaler300_set_vch_buf_clean_parameter(node, ch_id, job, job_idx);
        else
            ret = fscaler300_set_vch_parameter(node, ch_id, job, job_idx);
        if (ret < 0)
            return -1;
    }

    if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
        if (node->property.vproperty[ch_id].scl_feature == (1 << 2)) {
            if (node->property.vproperty[ch_id].buf_clean == 1)
                ret = fscaler300_set_1frame1frame_1frame1frame_buf_clean_parameter(node, ch_id, job, 0);
            if (ret < 0)
                return -1;
        }
    }

    fscaler300_get_job_info(job);

#ifdef SPLIT_SUPPORT
    if (node->property.vproperty[ch_id].scl_feature == 4) {
        ret = fscaler300_set_split_parameter(node, ch_id, job, job_idx);
        if (ret < 0)
            return -1;
        /* get blk count & mask, osd ping pong buffer select */
        fscaler300_get_split_job_info(job);
    }
#endif

    return 0;
}

/*
 * alloc fscaler_job_t & translate property to fscaler300 register for multiple path
 */
int fscaler300_init_mjob(fscaler300_job_t *job, job_node_t *mnode[], int last_job)
{
    int i, ret = 0;

    if (debug_mode >= 2)
        printm(MODULE_NAME, "%s\n", __func__);

    INIT_LIST_HEAD(&job->plist);
    INIT_LIST_HEAD(&job->clist);

    /* hook fops to job */
    for (i = 0; i < SD_MAX; i++) {
        if (mnode[i] != NULL) {
            job->input_node[i].enable = 1;
            job->input_node[i].idx = mnode[i]->idx;
            job->input_node[i].engine = mnode[i]->engine;
            job->input_node[i].minor = mnode[i]->minor;
            job->input_node[i].fops = &callback_ops;
            job->input_node[i].private = mnode[i];
        }
    }
    job->status = JOB_STS_QUEUE;
    job->job_type = MULTI_SJOB;
    job->perf_type = TYPE_ALL;
    job->priority = mnode[0]->priority;
    job->job_id = mnode[0]->job_id;

    /* translate V.G property to fscaler300 hw parameter */
    ret = fscaler300_set_mpath_parameter(mnode, job);
    if (ret < 0)
        return -1;
    /* get blk count & mask, osd ping pong buffer select */
    fscaler300_get_job_info(job);

    /* for performance used */
    if (last_job == 1)
        job->need_callback = 1;
    else
        job->need_callback = 0;

    return 0;
}

/*
 * set aspect ratio
 */
void fscaler300_set_aspect_ratio(job_node_t *node, int ch_id)
{
    int src_width, src_height, dst_width, dst_height, dst_x, dst_y, dst_bg_width, dst_bg_height;
    int ratio_width = 0, ratio_height = 0;
    int ratio = 0;

    /* if have sub frame, select source from main frame or sub frame */
    if (node->property.sub_yuv == 1)
        ratio = fscaler300_select_ratio_source(node, ch_id);

    src_width = node->property.vproperty[ch_id].src_width;
    src_height = node->property.vproperty[ch_id].src_height;
    /* sub_yuv = 1 means we have ratio frame, ratio = 1 means select source from ratio frame
       ratio frame's width/height is 1/2 source's width/height */
    if (node->property.sub_yuv == 1 && ratio == 1) {
        src_width = node->property.vproperty[ch_id].src_width >> 1;
        src_height = node->property.vproperty[ch_id].src_height >> 1;
    }

    dst_width = node->property.vproperty[ch_id].dst_width;
    dst_height = node->property.vproperty[ch_id].dst_height;
    dst_bg_width = node->property.dst_bg_width;
    dst_bg_height = node->property.dst_bg_height;

    if (src_width * dst_height > src_height * dst_width) {
        // height must be 2 align, to make sure frm2field, top/bottom field should have same line
        ratio_height = ALIGN_2(dst_width * src_height / src_width);
        /* if aspect ratio result is too small ( dst_height - ratio_height <= 2), disable aspect ratio */
        if (abs(dst_height - ratio_height) <= 2)
            node->property.vproperty[ch_id].aspect_ratio.enable = 0;

        if (ratio_height >= dst_bg_height)
            node->property.vproperty[ch_id].aspect_ratio.enable = 0;

        if (node->property.vproperty[ch_id].aspect_ratio.enable) {
            if (dst_height >= ratio_height)
                dst_y = (dst_height - ratio_height) >> 1;
            else
                dst_y = (ratio_height - dst_height) >> 1;
            // dst_y must bigger than original height & 2 align
            if (dst_y % 2 != 0)
                dst_y--;

            ratio_height = dst_height - (dst_y << 1);

            node->property.vproperty[ch_id].aspect_ratio.dst_width = dst_width;
            node->property.vproperty[ch_id].aspect_ratio.dst_height = ratio_height;
            node->property.vproperty[ch_id].aspect_ratio.dst_x = 0;
            node->property.vproperty[ch_id].aspect_ratio.dst_y = dst_y;
            node->property.vproperty[ch_id].aspect_ratio.type = H_TYPE;
        }
    }
    if (src_width * dst_height < src_height * dst_width) {
        ratio_width = ALIGN_4(src_width * dst_height / src_height);

        if ((((abs(dst_width - ratio_width)) >> 1) % 4) != 0)
            ratio_width += 4;

        if (abs(ratio_width - dst_width) <= 2)
            node->property.vproperty[ch_id].aspect_ratio.enable = 0;

        if (ratio_width >= dst_bg_width)
            node->property.vproperty[ch_id].aspect_ratio.enable = 0;

        if (node->property.vproperty[ch_id].aspect_ratio.enable) {
            if (dst_width >= ratio_width)
                dst_x = (dst_width - ratio_width) >> 1;
            else
                dst_x = (ratio_width - dst_width) >> 1;

            node->property.vproperty[ch_id].aspect_ratio.dst_width = ratio_width;
            node->property.vproperty[ch_id].aspect_ratio.dst_height = dst_height;
            node->property.vproperty[ch_id].aspect_ratio.dst_x = dst_x;
            node->property.vproperty[ch_id].aspect_ratio.dst_y = 0;
            node->property.vproperty[ch_id].aspect_ratio.type = V_TYPE;
        }
    }
    if (src_width * dst_height == src_height * dst_width)
        node->property.vproperty[ch_id].aspect_ratio.enable = 0;

    if (debug_mode >= 1) {
        printm(MODULE_NAME, "%s job id %d ch_id %d\n", __func__, node->job_id, ch_id);
        printm(MODULE_NAME, "dst_width %d\n", node->property.vproperty[ch_id].aspect_ratio.dst_width);
        printm(MODULE_NAME, "dst_height %d\n", node->property.vproperty[ch_id].aspect_ratio.dst_height);
        printm(MODULE_NAME, "dst_x %d\n", node->property.vproperty[ch_id].aspect_ratio.dst_x);
        printm(MODULE_NAME, "dst_y %d\n", node->property.vproperty[ch_id].aspect_ratio.dst_y);
        printm(MODULE_NAME, "type %d\n", node->property.vproperty[ch_id].aspect_ratio.type);
    }
}

/*
 * get cascade frame job count
 */
int fscaler300_get_frame_to_cascade_jobcnt(job_node_t *node, cas_info_t *cas_info)
{
    int width = 0, height = 0;
    int ch_id = node->ch_id;
    int ratio = 0;
    int frame = 1, job_cnt = 0;

    if (ch_id != 0) {
        scl_err("Error! cascade ch id != 0 %d\n", ch_id);
        return -1;
    }

    ratio = node->property.cas_ratio;

    /* first job width/height = original width/height */
    width = node->property.vproperty[ch_id].src_width;
    height = node->property.vproperty[ch_id].src_height;
    cas_info->width[ch_id] = width;
    cas_info->height[ch_id] = height;
    /* calculate cascade width/height */
    while (width >= 64 && height >= 36) {
        width = ALIGN_4_UP(width * ratio / 100);
        height = ALIGN_2_UP(height * ratio / 100);

        if (width < 64 || height < 36)
            break;

        cas_info->width[frame] = width;
        cas_info->height[frame] = height;

        frame++;
    }

    /* calculate job count */
    job_cnt = frame >> 1;
    if (frame % 2)
        job_cnt++;

    node->node_info.job_cnt = job_cnt;
    node->node_info.blk_cnt = job_cnt * 2;

    return 0;
}

int fscaler300_get_pip_jobcnt(job_node_t *node)
{
    node->node_info.job_cnt = node->property.pip.win_cnt;
    node->node_info.blk_cnt = node->property.pip.win_cnt;

    return 0;
}

/*
 * get cvbs zoom frame job count
 */
int fscaler300_get_cvbs_zoom_jobcnt(job_node_t *node)
{
    /* 1frame1frame to 1frame1frame only scaling entire frame */
    if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
        /* bypass, move from top frame to zoom frame */
        if (node->property.src2_bg_width == node->property.win2_bg_width && node->property.src2_bg_height == node->property.win2_bg_height) {
            node->node_info.job_cnt = 1;
            node->node_info.blk_cnt = 1;
        }
        /* scaling */
        if (node->property.src2_bg_width != node->property.win2_bg_width && node->property.src2_bg_height != node->property.win2_bg_height) {
            node->node_info.job_cnt = 1 + 1 + 1;
            node->node_info.blk_cnt = 2 + 2 + 1;
        }
        /* only do height scaling */
        if (node->property.src2_bg_width == node->property.win2_bg_width && node->property.src2_bg_height != node->property.win2_bg_height) {
            node->node_info.job_cnt = 1 + 1;
            node->node_info.blk_cnt = 2 + 1;
        }
        /* only do width scaling */
        if (node->property.src2_bg_width != node->property.win2_bg_width && node->property.src2_bg_height == node->property.win2_bg_height) {
            node->node_info.job_cnt = 1 + 1;
            node->node_info.blk_cnt = 2 + 1;
        }
    }
    /* 1frame2frame/2frame2frame scaling need to divide to even & odd line */
    else {
        /* bypass, move from third frame to fourth frame */
        if (node->property.src2_bg_width == node->property.win2_bg_width && node->property.src2_bg_height == node->property.win2_bg_height) {
            node->node_info.job_cnt = 1;
            node->node_info.blk_cnt = 1;
        }
        /* scaling
        1. draw horizontal border 2. draw vertical border 3. scaling even line 4. scaling odd line */
        if (node->property.src2_bg_width != node->property.win2_bg_width && node->property.src2_bg_height != node->property.win2_bg_height) {
            node->node_info.job_cnt = 1 + 1 + 1 + 1;
            node->node_info.blk_cnt = 2 + 2 + 1 + 1;
        }
        /* only do height scaling
        1. draw horizontal border 2. scaling even line 3. scaling odd line */
        if (node->property.src2_bg_width == node->property.win2_bg_width && node->property.src2_bg_height != node->property.win2_bg_height) {
            node->node_info.job_cnt = 1 + 1 + 1;
            node->node_info.blk_cnt = 2 + 1 + 1;
        }
            /* only do width scaling
            1. draw vertical border 2. scaling even line 3. scaling odd line */
        if (node->property.src2_bg_width != node->property.win2_bg_width && node->property.src2_bg_height == node->property.win2_bg_height) {
            node->node_info.job_cnt = 1 + 1 + 1;
            node->node_info.blk_cnt = 2 + 1 + 1;
        }
    }
    return 0;
}

/*
 * get virtual node 1frame_1frame to 1frame_1frame virtual channel job count with temporal buffer
 * virtual node input & output address are the same,
 * when scaling, we need to move source to temporal buffer, then scaling back to prevent scaling image over source image
 */
int fscaler300_get_vframe_to_1frame1frame_temporal_jobcnt(job_node_t *node, int vch_id)
{
    if (node->property.vproperty[vch_id].scl_feature & (1 << 0)) {
        /* just scaling,
        1. move source(top) to temporal buf 2. scaling temporal buf(top) to top frame */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1;
        }
        /* enable aspect ratio, no border
           1. move source(top) to temporal buf 2. scaling temporal buf(top) to top frame
           3. fix pattern draw top frame background (1 input 2 output) */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 1) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 2;
        }
        /* enable border, no aspect ratio
           1. move source(top) to temporal buf 2. scaling temporal buf(top) to top frame & border */
        if (node->property.vproperty[vch_id].border.enable == 1 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1;
        }
    }
    return 0;
}

/*
 * get virtual node 1frame_1frame to 1frame_1frame buffer clean job count
 * only do fix pattern draw lcd0 frame to black
 */
int fscaler300_get_vframe_to_1frame1frame_buf_clean_jobcnt(job_node_t *node, int vch_id)
{
    if ((node->property.vproperty[vch_id].scl_feature == 0 && node->property.vproperty[vch_id].buf_clean == 1) ||
        (node->property.vproperty[vch_id].scl_feature == (1 << 4) && node->property.vproperty[vch_id].buf_clean == 1) ||
        (node->property.vproperty[vch_id].scl_feature == (1 << 5) && node->property.vproperty[vch_id].buf_clean == 1)) {
        /* just scaling, 1. top frame scaling */
        // for virtual job
        node->property.vproperty[vch_id].vnode_info.job_cnt = 1;
        node->property.vproperty[vch_id].vnode_info.blk_cnt = 1;
        // for single job
        node->node_info.job_cnt = 1;
        node->node_info.blk_cnt = 1;
    }

    if (node->property.vproperty[vch_id].scl_feature & (1 << 2)) {
        /* just scaling, 1. top frame scaling */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1;
        }
    }
    return 0;
}

/*
 * get virtual node frame to frame temporal buffer virtual channel job count
 */
int fscaler300_get_vframe_to_frame_temporal_jobcnt(job_node_t *node, int vch_id)
{
    if (node->property.vproperty[vch_id].scl_feature & (1 << 0)) {
        /* 1. move source(top) to temporal buf 2. scaling temporal buf(top) to top frame */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1;
            node->node_info.job_cnt = 1 + 1;
            node->node_info.blk_cnt = 1 + 1;
        }
        /* enable aspect ratio, no border
           1. move source(top) to temporal buf 2. scaling temporal buf(top) to top frame
           3. fix pattern draw top frame background (1 input 2 output) */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 1) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 2;
            node->node_info.job_cnt = 1 + 1 + 1;
            node->node_info.blk_cnt = 1 + 1 + 2;
        }
        /* enable border, no aspect ratio */
#if GM8210
        /* 1. move source(top) to temporal buf
           2. scaling temporal buf(top) to top frame & border */
        if (node->property.vproperty[vch_id].border.enable == 1 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1;
            node->node_info.job_cnt = 1 + 1;
            node->node_info.blk_cnt = 1 + 1;
        }
#else
        /* 1. move source(top) to temporal buf 2. scaling temporal buf(top) to top frame
           3. fix pattern draw horizontal border (1 input 2 output)
           4. fix pattern draw vertical border (1 input 2 output) */
        if (node->property.vproperty[vch_id].border.enable == 1 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 2 + 2;
            node->node_info.job_cnt = 1 + 1 + 1 + 1;
            node->node_info.blk_cnt = 1 + 1 + 2 + 2;
        }
#endif
    }
    return 0;
}

/*
 * get virtual node frame to frame virtual channel job count
 */
int fscaler300_get_vframe_to_frame_jobcnt(job_node_t *node, int vch_id)
{
    if (node->property.vproperty[vch_id].scl_feature & (1 << 0)) {
        /* just scaling, 1. top frame scaling */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1;
            node->node_info.job_cnt = 1;
            node->node_info.blk_cnt = 1;
        }
        /* enable aspect ratio, no border
           1. top frame scaling,
           2. fix pattern draw top frame background (1 input 2 output) */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 1) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 2;
            node->node_info.job_cnt = 1 + 1;
            node->node_info.blk_cnt = 1 + 2;
        }
        /* enable border, no aspect ratio */
#if GM8210
        /* 1. top frame scaling & border */
        if (node->property.vproperty[vch_id].border.enable == 1 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1;
            node->node_info.job_cnt = 1;
            node->node_info.blk_cnt = 1;
        }
#else
        /* 1. top frame scaling
           2. fix pattern draw horizontal border (1 input 2 output)
           3. fix pattern draw vertical border (1 input 2 output) */
        if (node->property.vproperty[vch_id].border.enable == 1 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 2 + 2;
            node->node_info.job_cnt = 1 + 1 + 1;
            node->node_info.blk_cnt = 1 + 2 + 2;
        }
#endif
    }
    return 0;
}

/*
 * get virtual node 1frame_1frame to 1frame_1frame virtual channel job count
 */
int fscaler300_get_vframe_to_1frame1frame_jobcnt(job_node_t *node, int vch_id)
{
    if (node->property.vproperty[vch_id].scl_feature & (1 << 0)) {
        /* just scaling, 1. top frame scaling */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1;
        }
        /* enable aspect ratio, no border
           1. top frame scaling,
           2. fix pattern draw top frame background (1 input 2 output) */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 1) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 2;
        }
        /* enable border, no aspect ratio
           1. top frame scaling & border */
        if (node->property.vproperty[vch_id].border.enable == 1 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1;
        }
    }
    return 0;
}

/*
 * get virtual node 2frame to 2frame virtual channel job count
 */
int fscaler300_get_vframe_to_1frame2frame_jobcnt(job_node_t *node, int vch_id)
{
    if (node->property.vproperty[vch_id].scl_feature & (1 << 0)) {
        /* just scaling
           1. top frame scaling, 2. cvbs frame scaling */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1;
        }
        /* enable aspect ratio, no border
           1. top frame scaling,
           2. fix pattern draw top frame background (1 input 2 output)
           3. cvbs frame scaling */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 1) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 2 + 1;
        }
        /* enable border, no aspect ratio
           1. top frame scaling & border
           2. cvbs frame scaling & border */
        if (node->property.vproperty[vch_id].border.enable == 1 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1;
        }
    }
    return 0;
}

/*
 * get virtual node 2frame to 2frame virtual channel job count with temporal buffer
 * virtual node input & output address are the same,
 * when scaling, we need to move source to temporal buffer, then scaling back to prevent scaling image over source image
 */
int fscaler300_get_v2frame_to_2frame_temporal_jobcnt(job_node_t *node, int vch_id)
{
    if (node->property.vproperty[vch_id].scl_feature & (1 << 0)) {
        /* just scaling
            1. move source(top) to temporal buf 2. scaling temporal buf(top) to top frame
            3. move source(bottom)to temporal buf 4. scaling temporal buf(bottom) to bottom frame */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 1 + 1;
            node->node_info.job_cnt = 1 + 1 + 1 + 1;
            node->node_info.blk_cnt = 1 + 1 + 1 + 1;
            /* 5. CVBS frame scaling */
            if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
                if (node->property.vproperty[vch_id].src2_width != 0 && node->property.vproperty[vch_id].dst2_width != 0) {
                    node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1 + 1;
                    node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 1 + 1 + 1;
                    node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1;
                    node->node_info.blk_cnt = 1 + 1 + 1 + 1 + 1;
                }
            }
        }

        /* enable aspect ratio, no border
           1. move source(top) to temporal buf 2. scaling temporal buf(top) to top frame
           3. move source(bottom)to temporal buf 4. scaling temporal buf(bottom) to bottom frame
           5. fix pattern draw background(4 blocks to draw), use frm2field, line offset = 0 (1 input 2 output) */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 1) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 1 + 1 + 2;
            node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1;
            node->node_info.blk_cnt = 1 + 1 + 1 + 1 + 2;
            /* 6. CVBS frame scaling */
            if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
                if (node->property.vproperty[vch_id].src2_width != 0 && node->property.vproperty[vch_id].dst2_width != 0) {
                    node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1 + 1 + 1;
                    node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 1 + 1 + 2 + 1;
                    node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1 + 1;
                    node->node_info.blk_cnt = 1 + 1 + 1 + 1 + 2 + 1;
                }
            }
        }
        /* enable border, no aspect ratio
           1. move source(top) to temporal buf 2. scaling temporal buf(top) to top frame
           3. move source(bottom)to temporal buf 4. scaling temporal buf(bottom) to bottom frame
           5. fix pattern draw horizontal border (frm2field)(1 input 2 output)
           6. fix pattern draw vertical border (frm2field, line offset = 0)(1 input 2 output) */
        if (node->property.vproperty[vch_id].border.enable == 1 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1 + 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 1 + 1 + 2 + 2;
            node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1 + 1;
            node->node_info.blk_cnt = 1 + 1 + 1 + 1 + 2 + 2;
            /* 7. CVBS frame scaling */
            /* 8. if src2_dim == dst2_dim, job 5/6 only draw cvbs frame's horizontal/vertical border, not scaling graph, save bus bandwidth */
            if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
                if (node->property.vproperty[vch_id].src2_width != 0 && node->property.vproperty[vch_id].dst2_width != 0) {
                    if ((node->property.vproperty[vch_id].src2_width == node->property.vproperty[vch_id].dst2_width) &&
                        (node->property.vproperty[vch_id].src2_height == node->property.vproperty[vch_id].dst2_height)) {
                        node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1;
                        node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 1 + 1 + 2 + 2 + 1 + 2;
                        node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1;
                        node->node_info.blk_cnt = 1 + 1 + 1 + 1 + 2 + 2 + 1 + 2;
                    }
                    else {
                        node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1 + 1 + 1 + 1;
                        node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 1 + 1 + 2 + 2 + 1;
                        node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1 + 1 + 1;
                        node->node_info.blk_cnt = 1 + 1 + 1 + 1 + 2 + 2 + 1;
                    }
                }
            }
        }
    }

    return 0;
}

int fscaler300_get_v2frame_to_2frame_buf_clean_jobcnt(job_node_t *node, int vch_id)
{
    /* 1. sd0 do top frame scaling, sd1 do bottom frame scaling */
    if (node->property.vproperty[vch_id].scl_feature == 0 && node->property.vproperty[vch_id].buf_clean == 1) {
        // for virtual job
        node->property.vproperty[vch_id].vnode_info.job_cnt = 1;
        node->property.vproperty[vch_id].vnode_info.blk_cnt = 1;
        // for single job
        node->node_info.job_cnt = 1;
        node->node_info.blk_cnt = 1;
    }
    return 0;
}

/*
 * get virtual node 2frame to 2frame virtual channel job count
 */
int fscaler300_get_v2frame_to_2frame_jobcnt(job_node_t *node, int vch_id)
{
    if (node->property.vproperty[vch_id].scl_feature & (1 << 0)) {
        /* just scaling
           1. top frame scaling, 2. bottom frame scaling */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1;
            node->node_info.job_cnt = 1 + 1;
            node->node_info.blk_cnt = 1 + 1;
            /* 3. CVBS frame scaling */
            if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
                if (node->property.vproperty[vch_id].src2_width != 0 && node->property.vproperty[vch_id].dst2_width != 0) {
                    node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1;
                    node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 1;
                    node->node_info.job_cnt = 1 + 1 + 1;
                    node->node_info.blk_cnt = 1 + 1 + 1;
                }
            }
        }
        /* enable aspect ratio, no border
           1. top frame scaling, 2. bottom frame scaling
           3. fix pattern draw background(4 blocks to draw), use frm2field, line offset = 0 (1 input 2 output) */
        if (node->property.vproperty[vch_id].border.enable == 0 && node->property.vproperty[vch_id].aspect_ratio.enable == 1) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 2;
            node->node_info.job_cnt = 1 + 1 + 1;
            node->node_info.blk_cnt = 1 + 1 + 2;
            /* 4. CVBS frame scaling */
            if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
                if (node->property.vproperty[vch_id].src2_width != 0 && node->property.vproperty[vch_id].dst2_width != 0) {
                    node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1;
                    node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 2 + 1;
                    node->node_info.job_cnt = 1 + 1 + 1 + 1;
                    node->node_info.blk_cnt = 1 + 1 + 2 + 1;
                }
            }
        }
        /* enable border, no aspect ratio
           1. top frame scaling, 2. bottom frame scaling
           3. fix pattern draw horizontal border (frm2field)(1 input 2 output)
           4. fix pattern draw vertical border (frm2field, line offset = 0)(1 input 2 output) */
        if (node->property.vproperty[vch_id].border.enable == 1 && node->property.vproperty[vch_id].aspect_ratio.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 2 + 2;
            node->node_info.job_cnt = 1 + 1 + 1 + 1;
            node->node_info.blk_cnt = 1 + 1 + 2 + 2;
            /* 5. CVBS frame scaling */
            /* 6. if src2_dim == dst2_dim, job 5/6 only draw cvbs frame's horizontal/vertical border, not scaling graph, save bus bandwidth */
            if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
                if (node->property.vproperty[vch_id].src2_width != 0 && node->property.vproperty[vch_id].dst2_width != 0) {
                    if ((node->property.vproperty[vch_id].src2_width == node->property.vproperty[vch_id].dst2_width) &&
                        (node->property.vproperty[vch_id].src2_height == node->property.vproperty[vch_id].dst2_height)) {
                        node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1 + 1 + 1;
                        node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 2 + 2 + 1 + 2;
                        node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1 + 1;
                        node->node_info.blk_cnt = 1 + 1 + 2 + 2 + 1 + 2;
                    }
                    else {
                        node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1 + 1;
                        node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 2 + 2 + 1;
                        node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1;
                        node->node_info.blk_cnt = 1 + 1 + 2 + 2 + 1;
                    }
                }
            }
        }
        /* enable border & aspect ratio
           1. top frame scaling, 2. bottom frame scaling
           3. fix pattern draw background(4 blocks to draw) use frm2field, line offset = 0 (1 input 2 output)
           4. fix pattern draw horizontal border (frm2field)(1 input 2 output)
           5. fix pattern draw vertial border (frm2field, line offset = 0)(1 input 2 output) */
        if (node->property.vproperty[vch_id].border.enable == 1 && node->property.vproperty[vch_id].aspect_ratio.enable == 1) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 2 + 2 + 2;
            node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1;
            node->node_info.blk_cnt = 1 + 1 + 2 + 2 + 2;
            /* 6. CVBS frame scaling */
            if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
                if (node->property.vproperty[vch_id].src2_width != 0 && node->property.vproperty[vch_id].dst2_width != 0) {
                    node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1 + 1 + 1;
                    node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 2 + 2 + 2 + 1;
                    node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1 + 1;
                    node->node_info.blk_cnt = 1 + 1 + 2 + 2 + 2 + 1;
                }
            }
        }
    }

    if (node->property.vproperty[vch_id].scl_feature & (1 << 1)) {
        /* just scaling
            1. top frame scaling, 2. bottom frame scaling */
        if (node->property.vproperty[vch_id].border.enable == 0) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1;
        }
        /* enable border, no aspect ratio
           1. top frame scaling, 2. bottom frame scaling
           3. fix pattern draw horizontal border (frm2field)(1 input 2 output)
           4. fix pattern draw vertical border (frm2field, line offset = 0)(1 input 2 output) */
        if (node->property.vproperty[vch_id].border.enable == 1) {
            node->property.vproperty[vch_id].vnode_info.job_cnt = 1 + 1 + 1 + 1;
            node->property.vproperty[vch_id].vnode_info.blk_cnt = 1 + 1 + 2 + 2;
        }
        if (node->property.vproperty[vch_id].aspect_ratio.enable) {
            scl_err("Error! line correct should not do aspect ratio\n");
            return -1;
        }
    }
    return 0;
}

/*
 * get rgb565 to rgb565 node job count
 */
int fscaler300_get_rgb565_to_rgb565_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;

    blk_cnt = node->node_info.blk_cnt;
    node->node_info.job_cnt = 1;
    node->node_info.blk_cnt = 1 * blk_cnt;

    return 0;
}

/*
 * get rgb555 to rgb555 node job count
 */
int fscaler300_get_rgb555_to_rgb555_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;

    blk_cnt = node->node_info.blk_cnt;
    node->node_info.job_cnt = 1;
    node->node_info.blk_cnt = 1 * blk_cnt;

    return 0;
}

int fscaler300_get_vframe_to_frame_buf_clean_jobcnt(job_node_t *node, int vch_id)
{
    // for single job
    node->node_info.job_cnt = 1;
    node->node_info.blk_cnt = 1;
    // for virtual job
    node->property.vproperty[vch_id].vnode_info.job_cnt = 1;
    node->property.vproperty[vch_id].vnode_info.blk_cnt = 1;

    return 0;
}
/*
 * get frame to frame node job count
 */
int fscaler300_get_frame_to_frame_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;
    int ch_id = node->ch_id;

    blk_cnt = node->node_info.blk_cnt;
    /*  source is progerssive
        1. frame scaling */
    if (node->property.vproperty[ch_id].src_interlace == 0) {
        node->node_info.job_cnt = 1;
        node->node_info.blk_cnt = 1 * blk_cnt;
    }
    /*  source is interlace
        1. even line scaling
        2. odd ling scaling  */
    if (node->property.vproperty[ch_id].src_interlace == 1) {
        node->node_info.job_cnt = 1;
        node->node_info.blk_cnt = 1 * blk_cnt;
    }
    return 0;
}

/*
 * get 2frame to 2frame node job count
 */
int fscaler300_get_2frame_to_2frame_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;
    int ch_id = node->ch_id;

    blk_cnt = node->node_info.blk_cnt;
    /* just scaling
       1. top frame scaling, 2. bottom frame scaling */
    if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
        node->node_info.job_cnt = 1 + 1;
        node->node_info.blk_cnt = 1 * blk_cnt + 1 * blk_cnt;
    }
    /* enable aspect ratio, no border
       1. top frame scaling, 2. bottom frame scaling
       3. fix pattern draw background(4 blocks to draw), use frm2field, line offset = 0 (1 input 2 output) */
    if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
        node->node_info.job_cnt = 1 + 1 + 1;
        node->node_info.blk_cnt = 1 * blk_cnt + 1 * blk_cnt + 2;
    }
    /* enable border, no aspect ratio
       1. top frame scaling, 2. bottom frame scaling
       3. fix pattern draw horizontal border (frm2field)(1 input 2 output)
       4. fix pattern draw vertical border (frm2field, line offset = 0)(1 input 2 output) */
    if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
        node->node_info.job_cnt = 1 + 1 + 1 + 1;
        node->node_info.blk_cnt = 1 * blk_cnt + 1 * blk_cnt + 2 + 2;
    }
    /* enable border & aspect ratio
       1. top frame scaling, 2. bottom frame scaling
       3. fix pattern draw background(4 blocks to draw) use frm2field, line offset = 0 (1 input 2 output)
       4. fix pattern draw horizontal border (frm2field)(1 input 2 output)
       5. fix pattern draw vertial border (frm2field, line offset = 0)(1 input 2 output) */
    if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
        node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1;
        node->node_info.blk_cnt = 1 * blk_cnt + 1 * blk_cnt + 2 + 2 + 2;
    }
    return 0;
}

/*
 * get ratio to 2frame node job count
 */
int fscaler300_get_ratio_to_2frame_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;
    int ch_id = node->ch_id;

    blk_cnt = node->node_info.blk_cnt;
    /* input is progressive */
    if (node->property.vproperty[ch_id].src_interlace == 0) {
        /* just scaling 1. frm2field */
        if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
            node->node_info.job_cnt = 1;
            node->node_info.blk_cnt = 1 * blk_cnt;
        }
        /*  enable aspect ratio, no border
            1. fix pattern draw background (frm2field)(1 input 2 output)
            2. frame frm2field */
        if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
            node->node_info.job_cnt = 1 + 1;
            node->node_info.blk_cnt = 2 + 1 * blk_cnt;
        }
        /*  enable border, no aspect ratio
            1. frame frm2field
            2. fix pattern draw horizontal border (frm2field)(1 input 2 output)
            3. fix pattern draw vertical border (frm2field)(1 input 2 output) */
        if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
            node->node_info.job_cnt = 1 + 1 + 1;
            node->node_info.blk_cnt = 1 * blk_cnt + 2 + 2;
        }
        /*  enable border & aspect ratio
            1. fix pattern draw background and use mask to draw border (frm2field) (1 input 2 output)
               H_TYPE, mask draw horizontal line
               V_TYPE, mask draw vertical line
            2. frame frm2field
            3. H_TYPE, fix pattern draw vertical border (frm2field)(1 input 2 output)
               V_TYPE, fix pattern draw horizontal border (frm2field)(1 input 2 output) */
        if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
            node->node_info.job_cnt = 1 + 1 + 1;
            node->node_info.blk_cnt = 2 + 1 * blk_cnt + 2;
        }
    }
    /* input is interlace */
    if (node->property.vproperty[ch_id].src_interlace == 1) {
        /* just scaling 1. top frame scaling 2. bottom frame scaling */
        if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
            node->node_info.job_cnt = 1 + 1;
            node->node_info.blk_cnt = 1 * blk_cnt + 1 * blk_cnt;
        }
        /*  enable aspect ratio, no border
            1. fix pattern draw background (frm2field)(1 input 2 output)
            2. top frame scaling 3. bottom frame scaling */
        if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
            node->node_info.job_cnt = 1 + 1 + 1;
            node->node_info.blk_cnt = 2 + 1 * blk_cnt + 1 * blk_cnt;
        }
        /*  enable border, no aspect ratio
            1. top frame scaling 2. bottom frame scaling
            3. fix pattern draw horizontal border (frm2field)(1 input 2 output)
            4. fix pattern draw vertical border (frm2field)(1 input 2 output) */
        if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
            node->node_info.job_cnt = 1 + 1 + 1 + 1;
            node->node_info.blk_cnt = 1 * blk_cnt + 1 * blk_cnt + 2 + 2;
        }
        /*  enable border & aspect ratio
            1. fix pattern draw background and use mask to draw border (frm2field) (1 input 2 output)
            2. top frame scaling 3. bottom frame scaling
            4. H_TYPE, fix pattern draw vertical border (frm2field)(1 input 2 output)
               V_TYPE, fix pattern draw horizontal border (frm2field)(1 input 2 output) */
        if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
            node->node_info.job_cnt = 1 + 1 + 1 + 1;
            node->node_info.blk_cnt = 2 + 1 * blk_cnt + 1 * blk_cnt + 2;
        }
    }
    return 0;
}

/*
 * get frame to 2frame_2frame node job count
 */
int fscaler300_get_frame_to_2frame2frame_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;
    int ch_id = node->ch_id;

    blk_cnt = node->node_info.blk_cnt;

    /* just scaling 1. top/bottom frame scaling (1 input 2 output) 2. CVBS frame scaling (1 input 1 output) */
    if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
        node->node_info.job_cnt = 1;
        node->node_info.blk_cnt = 1 * blk_cnt;
        if (node->property.vproperty[ch_id].dst2_width != 0 && node->property.vproperty[ch_id].dst2_height != 0) {
            node->node_info.job_cnt = 1 + 1;
            node->node_info.blk_cnt = 1 * blk_cnt + 1 * blk_cnt;
        }
    }
    /*  enable aspect ratio, no border
        1. fix pattern draw background (frm2field)(1 input 2 output)
        2. top/bottom frame scaling (1 input 2 output) 3. CVBS frame scaling (1 input 1 output) */
    if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
        node->node_info.job_cnt = 1 + 1;
        node->node_info.blk_cnt = 2 + 1 * blk_cnt;
        if (node->property.vproperty[ch_id].dst2_width != 0 && node->property.vproperty[ch_id].dst2_height != 0) {
            node->node_info.job_cnt = 1 + 1 + 1;
            node->node_info.blk_cnt = 2 + 1 * blk_cnt + 1 * blk_cnt;
        }
    }
    /*  enable border, no aspect ratio
        1. top/bottom frame scaling (1 input 2 output)
        2. fix pattern draw top/bottom horizontal border
        3. fix pattern draw top/bottom vertical border (2 frm2field bypass)(1 input 2 output)
        4. CVBS frame scaling (1 input 1 output) */
    if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
        node->node_info.job_cnt = 1 + 1 + 1;
        node->node_info.blk_cnt = 1 * blk_cnt + 2 + 2;
        if (node->property.vproperty[ch_id].dst2_width != 0 && node->property.vproperty[ch_id].dst2_height != 0) {
            node->node_info.job_cnt = 1 + 1 + 1 + 1;
            node->node_info.blk_cnt = 1 * blk_cnt + 2 + 2 + 1 * blk_cnt;
        }
    }

    /*  enable border & aspect ratio
        1. fix pattern draw background (frm2field) (1 input 2 output)
        2. top/bottom frame scaling (1 input 2 output)
        3. fix pattern draw top/bottom horizontal border
        4. fix pattern draw top/bottom vertical border (2 frm2field bypass)(1 input 2 output)
        5. CVBS frame scaling (1 input 1 output) */
    if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
        node->node_info.job_cnt = 1 + 1 + 1 + 1;
        node->node_info.blk_cnt = 2 + 1 * blk_cnt + 2 + 2;
        if (node->property.vproperty[ch_id].dst2_width != 0 && node->property.vproperty[ch_id].dst2_height != 0) {
            node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1;
            node->node_info.blk_cnt = 2 + 1 * blk_cnt + 2 + 2 + 1 * blk_cnt;
        }
    }
    return 0;
}

/*
 * get ratio to 2frame_1frame node job count
 */
int fscaler300_get_ratio_to_2frame1frame_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;
    int ch_id = node->ch_id;

    blk_cnt = node->node_info.blk_cnt;

    /* input is progressive */
    if (node->property.vproperty[ch_id].src_interlace == 0) {
        /* just scaling 1. frm2field(1 input 2 output) */
        if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
            node->node_info.job_cnt = 1;
            node->node_info.blk_cnt = 1 * blk_cnt;
        }
        /*  enable aspect ratio, no border
            1. fix pattern draw background (frm2field)(1 input 2 output)
            2. top/bottom frame frm2field, CVBS frame scaling(1 input 2 output)*/
        if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
            node->node_info.job_cnt = 1 + 1;
            node->node_info.blk_cnt = 2 + 1 * blk_cnt;
        }
        /*  enable border, no aspect ratio
            1. top/bottom frame frm2field, CVBS frame scaling(1 input 2 output)
            2. fix pattern draw horizontal border
               if 3 frame's total width > 4096, split to 2 jobs
                (1) draw top/bottom frame (frm2field, 1 input 2 output)
                (2) draw CVBS frame (frm2field, 1 input 1 output)
               else
                1 job (frm2field, 1 input 3 output, 2 bypass, 1 scaling)
            3. fix pattern draw vertical border (2 frm2field bypass draw top/bottom frame & 2 scaling draw CVBS frame)(1 input 4 output) */
        if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
            if ((node->property.vproperty[ch_id].dst_width << 1) + node->property.vproperty[ch_id].dst2_width > SCALER300_TOTAL_RESOLUTION) {    // if 3 frame width > 4096, split to 2 jobs
                node->node_info.job_cnt = 1 + 2 + 1;
                node->node_info.blk_cnt = 1 * blk_cnt + (2 + 1) + 2;
            }
            else {
                node->node_info.job_cnt = 1 + 1 + 1;
                node->node_info.blk_cnt = 1 * blk_cnt + 2 + 2;
            }
        }
        /*  enable border & aspect ratio
            1. fix pattern draw background (frm2field) (1 input 2 output)
            2. top/bottom frame frm2field, CVBS frame scaling (1 input 2 output)
            3. fix pattern draw horizontal border
               if 3 frame's total width > 4096, split to 2 jobs
                (1) draw top/bottom frame (frm2field, 1 input 2 output)
                (2) draw CVBS frame (frm2field, 1 input 1 output)
               else
                1 job (frm2field, 1 input 3 output, 2 bypass, 1 scaling)
            4. fix pattern draw vertical border (2 frm2field bypass & 2 scaling)(1 input 4 output) */
        if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
            if ((node->property.vproperty[ch_id].aspect_ratio.dst_width << 1) + node->property.vproperty[ch_id].dst2_width > SCALER300_TOTAL_RESOLUTION) {   // if 3 frame width > 4096, split to 2 jobs
                node->node_info.job_cnt = 1 + 1 + 2 + 1;
                node->node_info.blk_cnt = 2 + 1 * blk_cnt + (2 + 1) + 2;
            }
            else {
                node->node_info.job_cnt = 1 + 1 + 1 + 1;
                node->node_info.blk_cnt = 2 + 1 * blk_cnt + 2 + 2;
            }
        }
    }
    /* input is interlace */
    if (node->property.vproperty[ch_id].src_interlace == 1) {
        /* just scaling 1. top frame scaling, CVBS even frame scaling (1 input 2 output) 2. bottom frame scaling, CVBS odd frame scaling (1 input 2 output) */
        if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
            node->node_info.job_cnt = 1 + 1;
            node->node_info.blk_cnt = 1 * blk_cnt + 1 * blk_cnt;
        }
        /*  enable aspect ratio, no border
            1. fix pattern draw background (frm2field)(1 input 2 output)
            2. top frame scaling, CVBS even frame scaling(1 input 2 output) 3. bottom frame scaling, CVBS odd frame scaling (1 input 2 output) */
        if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
            node->node_info.job_cnt = 1 + 1 + 1;
            node->node_info.blk_cnt = 2 + 1 * blk_cnt + 1 * blk_cnt;
        }
        /*  enable border, no aspect ratio
            1. top frame scaling, CVBS even frame scaling (1 input 2 output) 2. bottom frame scaling, CVBS odd frame scaling (1 input 2 output)
            3. fix pattern draw horizontal border
               if 3 frame's total width > 4096, split to 2 jobs
                (1) draw top/bottom frame (frm2field, 1 input 2 output)
                (2) draw CVBS frame (frm2field, 1 input 1 output)
               else
                1 job (frm2field, 1 input 3 output, 2 bypass, 1 scaling)
            4. fix pattern draw vertical border (2 frm2field bypass & 2 scaling)(1 input 4 output) */
        if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
            if ((node->property.vproperty[ch_id].dst_width << 1) + node->property.vproperty[ch_id].dst2_width > SCALER300_TOTAL_RESOLUTION) {   // if 3 frame width > 4096, split to 2 jobs
                node->node_info.job_cnt = 1 + 1 + 2 + 1;
                node->node_info.blk_cnt = 1 * blk_cnt + 1 * blk_cnt + (2 + 1) + 2;
            }
            else {
                node->node_info.job_cnt = 1 + 1 + 1 + 1;
                node->node_info.blk_cnt = 1 * blk_cnt + 1 * blk_cnt + 2 + 2;
            }
        }
        /*  enable border & aspect ratio
            1. fix pattern draw background (frm2field) (1 input 2 output)
            2. top frame scaling, CVBS even frame scaling(1 input 2 output) 3. bottom frame scaling, CVBS odd frame scaling (1 input 2 output)
            4. fix pattern draw horizontal border
               if 3 frame's total width > 4096, split to 2 jobs
                (1) draw top/bottom frame (frm2field, 1 input 2 output)
                (2) draw CVBS frame (frm2field, 1 input 1 output)
               else
                1 job (frm2field, 1 input 3 output, 2 bypass, 1 scaling)
            5. fix pattern draw vertical border (2 frm2field bypass & 2 scaling)(1 input 4 output) */
        if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
            if ((node->property.vproperty[ch_id].aspect_ratio.dst_width << 1) + node->property.vproperty[ch_id].dst2_width > SCALER300_TOTAL_RESOLUTION) {   // if 3 frame width > 4096, split to 2 jobs
                node->node_info.job_cnt = 1 + 1 + 1 + 2 + 1;
                node->node_info.blk_cnt = 2 + 1 * blk_cnt + 1 * blk_cnt + (2 + 1) + 2;
            }
            else {
                node->node_info.job_cnt = 1 + 1 + 1 + 1 + 1;
                node->node_info.blk_cnt = 2 + 1 * blk_cnt + 1 * blk_cnt + 2 + 2;
            }
        }
    }
    return 0;
}

/*
 * get frame to 1frame_1frame node job count --> clear win
 */
int fscaler300_get_frame_to_1frame1frame_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;

    blk_cnt = node->node_info.blk_cnt;

    /* clear win 1. top frame scaling */
    node->node_info.job_cnt = 1;
    node->node_info.blk_cnt = 1 * blk_cnt;

    return 0;
}

/*
 * get frame to 2frame node job count --> clear win
 */
int fscaler300_get_frame_to_2frame_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;

    blk_cnt = node->node_info.blk_cnt;

    /* clear win 1. top frame scaling + bottom frame scaling */
    node->node_info.job_cnt = 1;
    node->node_info.blk_cnt = 1 + 1;

    return 0;
}

/*
 * get frame to 1frame_2frame node job count --> clear win
 */
int fscaler300_get_frame_to_1frame2frame_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;
    int ch_id = node->ch_id;

    blk_cnt = node->node_info.blk_cnt;

    /* clear win 1. top frame scaling + cvbs frame scaling */
    node->node_info.job_cnt = 1;
    node->node_info.blk_cnt = 1 * blk_cnt;
    if (node->property.vproperty[ch_id].dst2_width != 0 && node->property.vproperty[ch_id].dst2_height != 0) {
        node->node_info.job_cnt = 1 + 1;
        node->node_info.blk_cnt = 1 * blk_cnt + 1 * blk_cnt;
    }

    return 0;
}

/*
 * get ratio to 1frame_1frame node job count
 */
int fscaler300_get_ratio_to_1frame1frame_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;
    int ch_id = node->ch_id;

    blk_cnt = node->node_info.blk_cnt;

    /* just scaling 1. top frame scaling (1 input 1 output) */
    if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
        node->node_info.job_cnt = 1;
        node->node_info.blk_cnt = 1 * blk_cnt;
    }
    /*  enable aspect ratio, no border
        1. fix pattern draw background (1 input 2 output)
        2. top frame scaling (1 input 1 output) */
    if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
        node->node_info.job_cnt = 1 + 1;
        node->node_info.blk_cnt = 2 + 1 * blk_cnt;
    }
    /*  enable border, no aspect ratio
        1. top frame scaling & border */
#if GM8210
    if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
        node->node_info.job_cnt = 1 ;
        node->node_info.blk_cnt = 1 * blk_cnt;
    }
#else
    if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
        node->node_info.job_cnt = 1 + 1 + 1;
        node->node_info.blk_cnt = 2 + 2 + 1 * blk_cnt;
    }
#endif
    return 0;
}

/*
 * get ratio to 1frame_2frame node job count
 */
int fscaler300_get_ratio_to_1frame2frame_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;
    int ch_id = node->ch_id;

    blk_cnt = node->node_info.blk_cnt;

    /* just scaling 1. top frame scaling + cvbs frame scaling (1 input 2 output) */
    if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
        node->node_info.job_cnt = 1;
        node->node_info.blk_cnt = 1 * blk_cnt;
    }
    /*  enable aspect ratio, no border
        1. fix pattern draw background (frm2field)(1 input 2 output)
        2. top frame scaling + cvbs frame scaling (1 input 2 output) */
    if (node->property.vproperty[ch_id].border.enable == 0 && node->property.vproperty[ch_id].aspect_ratio.enable == 1) {
        node->node_info.job_cnt = 1 + 1;
        node->node_info.blk_cnt = 2 + 1 * blk_cnt;
    }
    /*  enable border, no aspect ratio
        1. top frame scaling & border + cvbs frame scaling & border */
    if (node->property.vproperty[ch_id].border.enable == 1 && node->property.vproperty[ch_id].aspect_ratio.enable == 0) {
        node->node_info.job_cnt = 1 ;
        node->node_info.blk_cnt = 1 * blk_cnt;
    }

    return 0;
}

/*
 * get single node job count
 */
int fscaler300_get_snode_jobcnt(job_node_t *node)
{
    int blk_cnt = 0;
    int ch_id = node->ch_id, ret = 0;

    blk_cnt = node->node_info.blk_cnt;

    /* get job_cnt of this node */
    if (node->property.in_addr_pa != node->property.out_addr_pa) {
        if (node->property.vproperty[ch_id].scl_feature == 0) {
            node->node_info.job_cnt = 0;
            return 0;
        }
    }
    // buffer clean only do at line correct stage (in addr = out addr)
    if ((node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
        (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)||
        (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME)) {
        if (node->property.in_addr_pa == node->property.out_addr_pa) {
            if (node->property.vproperty[ch_id].scl_feature == 0 && node->property.vproperty[ch_id].buf_clean == 0) {   // 0 : do nothing
                node->node_info.job_cnt = 0;
                return 0;
            }

            if (node->property.vproperty[ch_id].scl_feature == (1 << 4) && node->property.vproperty[ch_id].buf_clean == 0)   // 0 : do nothing
                node->node_info.job_cnt = 0;

            if (node->property.vproperty[ch_id].scl_feature == (1 << 5) && node->property.vproperty[ch_id].buf_clean == 0) {   // 0 : do nothing
                node->node_info.job_cnt = 0;
#if GM8210
                // scl_feature = 1 << 5 : EP set dma param
                ret = scl_dma_add(&node->property.vproperty[ch_id].scl_dma);
                if (ret < 0) {
                    printk("%s scl_dma_add fail\n", __func__);
                    printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                    return -1;
                }
#endif
                return 0;
            }

            /* if buf clean = 1, do fix pattern, draw frame to black, don't need to do aspect ratio & border */
            if (node->property.vproperty[ch_id].buf_clean == 1) {
                node->property.vproperty[ch_id].border.enable = 0;
                node->property.vproperty[ch_id].aspect_ratio.enable = 0;
            }

            if ((node->property.vproperty[ch_id].scl_feature == 0 && node->property.vproperty[ch_id].buf_clean == 1) ||
                (node->property.vproperty[ch_id].scl_feature == (1 << 4) && node->property.vproperty[ch_id].buf_clean == 1) ||
                (node->property.vproperty[ch_id].scl_feature == (1 << 5) && node->property.vproperty[ch_id].buf_clean == 1)) {   // do buffer clean
                if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME)
                    fscaler300_get_vframe_to_frame_buf_clean_jobcnt(node, ch_id);
                if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)
                    fscaler300_get_vframe_to_1frame1frame_buf_clean_jobcnt(node, ch_id);
                if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
                    fscaler300_get_v2frame_to_2frame_buf_clean_jobcnt(node, ch_id);
#if GM8210
                // buffer clean also need to do : EP set dma param
                if (node->property.vproperty[ch_id].scl_feature == (1 << 5)) {
                    ret = scl_dma_add(&node->property.vproperty[ch_id].scl_dma);
                    if (ret < 0) {
                        printk("%s scl_dma_add fail\n", __func__);
                        printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                        return -1;
                    }
                }
#endif
            }
        }
    }

    /* if auto aspect ratio is set, calculate aspect ratio width/height */
    if (node->property.vproperty[ch_id].aspect_ratio.enable)
        fscaler300_set_aspect_ratio(node, ch_id);

    if (node->property.vproperty[ch_id].scl_feature & (1 << 0)) {          // 1 : do scaling
        /* frame to frame, frame to rgb565 no border & no aspect ratio */
        if ((node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_RGB565) ||
            (node->property.src_fmt == TYPE_YUV422_FIELDS && node->property.dst_fmt == TYPE_YUV422_FRAME) ||
            (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) ||
            (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) ||
            (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_CASCADE_SCALING_FRAME) ||
            (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES)) {
            if ((node->property.vproperty[ch_id].border.enable == 1) || (node->property.vproperty[ch_id].aspect_ratio.enable == 1)) {
                scl_err("Error! src_fmt %x dst_fmt %x not support border & aspect ratio\n", node->property.src_fmt, node->property.dst_fmt);
                return -1;
            }
        }
        /* frame to frame, frame to rgb565 */
            node->node_info.job_cnt = 1;
            node->node_info.blk_cnt = 1 * blk_cnt;
        /* frame to frame */
        if (node->property.src_fmt == TYPE_YUV422_FIELDS && node->property.dst_fmt == TYPE_YUV422_FRAME)
            fscaler300_get_frame_to_frame_jobcnt(node);
        /* frame to frame */
        if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME) {
            if (node->property.in_addr_pa == node->property.out_addr_pa) {
                if (node->property.vproperty[ch_id].buf_clean == 0)
                    fscaler300_get_vframe_to_frame_temporal_jobcnt(node, ch_id);
                if (node->property.vproperty[ch_id].buf_clean == 1)
                    fscaler300_get_vframe_to_frame_jobcnt(node, ch_id);
            }
            else
                fscaler300_get_vframe_to_frame_jobcnt(node, ch_id);
        }

        /* 2 frame to 2 frame */
        if (node->property.src_fmt == TYPE_YUV422_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES) {
            if (node->property.in_addr_pa == node->property.out_addr_pa) {
                if (node->property.vproperty[ch_id].buf_clean == 0)
                    fscaler300_get_v2frame_to_2frame_temporal_jobcnt(node, ch_id);
                if (node->property.vproperty[ch_id].buf_clean == 1)
                    fscaler300_get_v2frame_to_2frame_jobcnt(node, ch_id);
            }
            else
                fscaler300_get_v2frame_to_2frame_jobcnt(node, ch_id);
        }
        /* ratio to 2 frame */
        if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES)
            fscaler300_get_ratio_to_2frame_jobcnt(node);
        /* ratio to 3 frame */
        if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
            fscaler300_get_ratio_to_2frame1frame_jobcnt(node);
        /* frame to 4 frame --> clear win */
        if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
            fscaler300_get_frame_to_2frame2frame_jobcnt(node);
        /* 3 frame to 3 frame */
        if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
            if (node->property.in_addr_pa == node->property.out_addr_pa) {
                if (node->property.vproperty[ch_id].buf_clean == 0)
                    fscaler300_get_v2frame_to_2frame_temporal_jobcnt(node, ch_id);
                if (node->property.vproperty[ch_id].buf_clean == 1)
                    fscaler300_get_v2frame_to_2frame_jobcnt(node, ch_id);
            }
            else
                fscaler300_get_v2frame_to_2frame_jobcnt(node, ch_id);
#if GM8210
            // EP set dma param
            if (node->property.vproperty[ch_id].scl_feature & (1 << 5)) {
                ret = scl_dma_add(&node->property.vproperty[ch_id].scl_dma);
                if (ret < 0) {
                    printk("%s scl_dma_add fail\n", __func__);
                    printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                    return -1;
                }
                // transfer bottom frame to RC
                node->property.vproperty[ch_id].scl_dma.src_addr = node->property.in_addr_pa +
                    (node->property.src_bg_width * node->property.src_bg_height << 1);
                node->property.vproperty[ch_id].scl_dma.dst_addr = node->property.pcie_addr +
                    (node->property.src_bg_width * node->property.src_bg_height << 1);
                ret = scl_dma_add(&node->property.vproperty[ch_id].scl_dma);
                if (ret < 0) {
                    printk("%s scl_dma_add fail\n", __func__);
                    printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                    return -1;
                }
            }
#endif
        }
        /* ratio to 1frame_2frame */
        if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES)
            fscaler300_get_ratio_to_1frame2frame_jobcnt(node);
        /* rgb565 to rgb565 */
        if (node->property.src_fmt == TYPE_RGB565 && node->property.dst_fmt == TYPE_RGB565)
            fscaler300_get_rgb565_to_rgb565_jobcnt(node);
        /* rgb555 to rgb555 */
        if (node->property.src_fmt == TYPE_RGB555 && node->property.dst_fmt == TYPE_RGB555)
            fscaler300_get_rgb555_to_rgb555_jobcnt(node);
        /* frame to 3 frame --> clear win */
        if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES)
            fscaler300_get_frame_to_1frame2frame_jobcnt(node);
        /* ratio to 1frame_1frame */
        if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)
            fscaler300_get_ratio_to_1frame1frame_jobcnt(node);
        /* frame to 1frame_1frame --> clear win */
        if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)
            fscaler300_get_frame_to_1frame1frame_jobcnt(node);
        /* frame to 2frame --> clear win */
        if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES)
            fscaler300_get_frame_to_2frame_jobcnt(node);
        /* ratio to 1frame */
        if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME)
            fscaler300_get_ratio_to_1frame1frame_jobcnt(node);
        /* 1frame_1frame to 1frame_1frame */
        if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
            if (node->property.in_addr_pa == node->property.out_addr_pa) {
                if (node->property.vproperty[ch_id].buf_clean == 0)
                    ret = fscaler300_get_vframe_to_1frame1frame_temporal_jobcnt(node, ch_id);
                if (node->property.vproperty[ch_id].buf_clean == 1)
                    ret = fscaler300_get_vframe_to_1frame1frame_jobcnt(node, ch_id);
            }
            else
                ret = fscaler300_get_vframe_to_1frame1frame_jobcnt(node, ch_id);
            if (ret < 0)
                return -1;
#if GM8210
            // EP set dma param
            if (node->property.vproperty[ch_id].scl_feature & (1 << 5)) {
                ret = scl_dma_add(&node->property.vproperty[ch_id].scl_dma);
                if (ret < 0) {
                    printk("%s scl_dma_add fail\n", __func__);
                    printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                    return -1;
                }
            }
#endif
        }
    }

    if (node->property.vproperty[ch_id].scl_feature & (1 << 1)) {          // 2 : do line correct
        /* 2 frame to 2 frame */
        if ((node->property.src_fmt == TYPE_YUV422_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES) ||
            (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
#if GM8210
            if (node->property.vproperty[ch_id].scl_feature & (1 << 5)) {
                ret = scl_dma_add(&node->property.vproperty[ch_id].scl_dma);
                if (ret < 0) {
                    printk("%s scl_dma_add fail\n", __func__);
                    printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                    return -1;
                }
                // transfer bottom frame to RC
                if ((node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
                    node->property.vproperty[ch_id].scl_dma.src_addr = node->property.in_addr_pa +
                        (node->property.src_bg_width * node->property.src_bg_height << 1);
                    node->property.vproperty[ch_id].scl_dma.dst_addr = node->property.pcie_addr +
                        (node->property.src_bg_width * node->property.src_bg_height << 1);
                    ret = scl_dma_add(&node->property.vproperty[ch_id].scl_dma);
                    if (ret < 0) {
                        printk("%s scl_dma_add fail\n", __func__);
                        printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                        return -1;
                    }
                }
            }
#endif
            if (node->property.di_result == 1 && node->property.vproperty[ch_id].src_interlace == 0) {
                ret = fscaler300_get_v2frame_to_2frame_jobcnt(node, ch_id);
                if (ret < 0)
                    return -1;
            }
            else { // not meet di_result = 1 & src_interlace = 0, but buffer clean = 1, still need to do buffer clean
                if (node->property.vproperty[ch_id].buf_clean == 1) {
                    ret = fscaler300_get_v2frame_to_2frame_jobcnt(node, ch_id);
                    if (ret < 0)
                        return -1;
                }
            }
        }
        /*else if ((node->property.vproperty[ch_id].scl_feature & (1 << 4)) && (node->property.src_fmt == 0)) {
            // special case, RC job a lot of property do not pass to scaler, it is donothing job.
            node->node_info.job_cnt = 0;
        }*/
        else {
            scl_err("[SL] don't support scl_feature = %d src_fmt [%x] dst_fmt [%x] job\n",
                     node->property.vproperty[ch_id].scl_feature, node->property.src_fmt, node->property.dst_fmt);
            DEBUG_M(LOG_ERROR, node->engine, node->minor,
                    "[SL] V.G JOB ID(%x) scl_feature = %d, src_fmt (%x) dst_fmt (%x) FAIL\n",
                    node->job_id, node->property.vproperty[ch_id].scl_feature, node->property.src_fmt, node->property.dst_fmt);
            return -1;
        }
    }

    if (node->property.vproperty[ch_id].scl_feature & (1 << 2)) {          // 4 : do cvbs zoom
        if ((node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
            (node->property.src_fmt == TYPE_YUV422_FRAME_2FRAMES && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) ||
            (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME))
            fscaler300_get_cvbs_zoom_jobcnt(node);
    }

#ifdef SPLIT_SUPPORT
    if (node->property.vproperty[ch_id].scl_feature == 4)           // 4 : split
        job_cnt = 1;
#endif
    return 0;
}

/*
 * get multiple window job count
 */
int fscaler300_get_vnode_jobcnt(job_node_t *node, int *zoom_flag, int *pip_flag)
{
    int i, ret = 0;
    int vch_cnt = 0;

    *zoom_flag = 0;
    *pip_flag = 0;
    /* summarize how many virtual channels have to do */
    for (i = 0; i <= virtual_chn_num; i++) {
        if (node->property.vproperty[i].vch_enable == 1) {
            if (node->property.vproperty[i].scl_feature & (1 << 2)) {    // 2: zoom
                if ((node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
                    (node->property.src_fmt == TYPE_YUV422_FRAME_2FRAMES && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) ||
                    (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)) {
                    *zoom_flag = 1;
                }
            }

            if (node->property.vproperty[i].scl_feature == 0 && node->property.vproperty[i].buf_clean == 0) {    // 0 : disable
                node->property.vproperty[i].vch_enable = 0;
                continue;
            }

            if ((node->property.vproperty[i].scl_feature == (1 << 4) && node->property.vproperty[i].buf_clean == 0) ||
                (node->property.vproperty[i].scl_feature == (0x14) && node->property.vproperty[i].buf_clean == 0)) {    // 0 : disable
                node->property.vproperty[i].vch_enable = 0;
                continue;
            }

            if ((node->property.vproperty[i].scl_feature == (1 << 5) && node->property.vproperty[i].buf_clean == 0) ||
                (node->property.vproperty[i].scl_feature == (0x24) && node->property.vproperty[i].buf_clean == 0)) {    // 0 : disable
                node->property.vproperty[i].vch_enable = 0;
#if GM8210
                // EP set dma param
                ret = scl_dma_add(&node->property.vproperty[i].scl_dma);
                if (ret < 0) {
                    printk("%s scl_dma_add fail\n", __func__);
                    printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                    return -1;
                }
#endif
                continue;
            }

            /* if buf clean = 1, do fix pattern, draw frame to black, don't need to do aspect ratio & border */
            if (node->property.vproperty[i].buf_clean == 1) {
                node->property.vproperty[i].border.enable = 0;
                node->property.vproperty[i].aspect_ratio.enable = 0;
            }

            if (node->property.vproperty[i].scl_feature & (1 << 3)) {    // 3: pip_mode
                if ((node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
                    (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME))
                    *pip_flag = 1;
            }

            if ((node->property.vproperty[i].scl_feature == 0 && node->property.vproperty[i].buf_clean == 1) ||
                (node->property.vproperty[i].scl_feature == (1 << 4) && node->property.vproperty[i].buf_clean == 1) ||
                (node->property.vproperty[i].scl_feature == (1 << 5) && node->property.vproperty[i].buf_clean == 1)) {   // do buffer clean
                if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME)
                    ret = fscaler300_get_vframe_to_frame_buf_clean_jobcnt(node, i);
                if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)
                    ret = fscaler300_get_vframe_to_1frame1frame_buf_clean_jobcnt(node, i);
                if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
                    ret = fscaler300_get_v2frame_to_2frame_buf_clean_jobcnt(node, i);
                if (ret < 0)
                    return -1;
                vch_cnt++;
#if GM8210
                // buffer clean also need to do : EP set dma param
                if (node->property.vproperty[i].scl_feature == (1 << 5)) {
                    ret = scl_dma_add(&node->property.vproperty[i].scl_dma);
                    if (ret < 0) {
                        printk("%s scl_dma_add fail\n", __func__);
                        printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                        return -1;
                    }
                }
#endif
            }

            if (node->property.vproperty[i].scl_feature & (1 << 0)) {    // 1 : do scaling
                if ((node->property.src_fmt == TYPE_YUV422_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES) ||
                    (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
                    if (node->property.vproperty[i].aspect_ratio.enable)
                        fscaler300_set_aspect_ratio(node, i);
                    if (node->property.in_addr_pa == node->property.out_addr_pa) {
                        if (node->property.vproperty[i].buf_clean == 0)
                            ret = fscaler300_get_v2frame_to_2frame_temporal_jobcnt(node, i);
                        if (node->property.vproperty[i].buf_clean == 1)
                            ret = fscaler300_get_v2frame_to_2frame_jobcnt(node, i);
                    }
                    else
                        ret = fscaler300_get_v2frame_to_2frame_jobcnt(node, i);
                    if (ret < 0)
                        return -1;
                    vch_cnt++;
#if GM8210
                    if (node->property.vproperty[i].scl_feature & (1 << 5)) {
                        ret = scl_dma_add(&node->property.vproperty[i].scl_dma);
                        if (ret < 0) {
                            printk("%s scl_dma_add fail\n", __func__);
                            printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                            return -1;
                        }
                        // transfer bottom frame to RC
                        if ((node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
                            node->property.vproperty[i].scl_dma.src_addr = node->property.in_addr_pa +
                                (node->property.src_bg_width * node->property.src_bg_height << 1);
                            node->property.vproperty[i].scl_dma.dst_addr = node->property.pcie_addr +
                                (node->property.src_bg_width * node->property.src_bg_height << 1);
                            ret = scl_dma_add(&node->property.vproperty[i].scl_dma);
                            if (ret < 0) {
                                printk("%s scl_dma_add fail\n", __func__);
                                printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                                return -1;
                            }
                        }
                    }
#endif
                }
                else if (node->property.src_fmt == TYPE_YUV422_FRAME_2FRAMES && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) {
                    if (node->property.vproperty[i].aspect_ratio.enable)
                        fscaler300_set_aspect_ratio(node, i);
                    ret = fscaler300_get_vframe_to_1frame2frame_jobcnt(node, i);
                    if (ret < 0)
                        return -1;
                    vch_cnt++;
                }
                else if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
                    if (node->property.vproperty[i].aspect_ratio.enable)
                        fscaler300_set_aspect_ratio(node, i);
                    if (node->property.in_addr_pa == node->property.out_addr_pa) {
                        if (node->property.vproperty[i].buf_clean == 0)
                            ret = fscaler300_get_vframe_to_1frame1frame_temporal_jobcnt(node, i);
                        if (node->property.vproperty[i].buf_clean == 1)
                            ret = fscaler300_get_vframe_to_1frame1frame_jobcnt(node, i);
                    }
                    else
                        ret = fscaler300_get_vframe_to_1frame1frame_jobcnt(node, i);
                    if (ret < 0)
                        return -1;
                    vch_cnt++;
#if GM8210
                    if (node->property.vproperty[i].scl_feature & (1 << 5)) {
                        ret = scl_dma_add(&node->property.vproperty[i].scl_dma);
                        if (ret < 0) {
                            printk("%s scl_dma_add fail\n", __func__);
                            printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                            return -1;
                        }
                    }
#endif
                }
                else if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME) {
                    if (node->property.vproperty[i].aspect_ratio.enable)
                        fscaler300_set_aspect_ratio(node, i);
                    if (node->property.in_addr_pa == node->property.out_addr_pa) {
                        if (node->property.vproperty[i].buf_clean == 0)
                            ret = fscaler300_get_vframe_to_frame_temporal_jobcnt(node, i);
                        if (node->property.vproperty[i].buf_clean == 1)
                            ret = fscaler300_get_vframe_to_frame_jobcnt(node, i);
                    }
                    else
                        ret = fscaler300_get_vframe_to_frame_jobcnt(node, i);
                    if (ret < 0)
                        return -1;
                    vch_cnt++;
                }
                else
                    node->property.vproperty[i].vch_enable = 0;
                continue;
            }

            if (node->property.vproperty[i].scl_feature & (1 << 1)) {    // 2 : do line correct
                if ((node->property.src_fmt == TYPE_YUV422_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES) ||
                    (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
                    if (node->property.di_result == 1 && node->property.vproperty[i].src_interlace == 0) {  // di_result = 1, do line correct
                        ret = fscaler300_get_v2frame_to_2frame_jobcnt(node, i);                             // src_interlace = 0 --> progressive, need to do line correct
                        if (ret < 0)                                                                        // di_result = 0, do nothing
                            return -1;
                        vch_cnt++;
                    }
                    else { // not meet di_result = 1 & src_interlace = 0, but buffer clean = 1, still need to do buffer clean
                        if (node->property.vproperty[i].buf_clean == 1) {
                            ret = fscaler300_get_v2frame_to_2frame_jobcnt(node, i);
                            if (ret < 0)                                                                        // di_result = 0, do nothing
                                return -1;
                            vch_cnt++;
                        }
                        else
                            node->property.vproperty[i].vch_enable = 0;
                    }
#if GM8210
                    if (node->property.vproperty[i].scl_feature & (1 << 5)) {
                        ret = scl_dma_add(&node->property.vproperty[i].scl_dma);
                        if (ret < 0) {
                            printk("%s scl_dma_add fail\n", __func__);
                            printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                            return -1;
                        }
                        // transfer bottom frame to RC
                        if ((node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)) {
                            node->property.vproperty[i].scl_dma.src_addr = node->property.in_addr_pa +
                                (node->property.src_bg_width * node->property.src_bg_height << 1);
                            node->property.vproperty[i].scl_dma.dst_addr = node->property.pcie_addr +
                                (node->property.src_bg_width * node->property.src_bg_height << 1);
                            ret = scl_dma_add(&node->property.vproperty[i].scl_dma);
                            if (ret < 0) {
                                printk("%s scl_dma_add fail\n", __func__);
                                printm(MODULE_NAME, "%s scl_dma_add fail\n", __func__);
                                return -1;
                            }
                        }
                    }
#endif
                    continue;
                }
                /*else if ((node->property.vproperty[i].scl_feature & (1 << 4)) && (node->property.src_fmt == 0)) {
                    // special case, RC job a lot of property do not pass to scaler, it is donothing job.
                    node->property.vproperty[i].vnode_info.job_cnt = 0;
                    continue;
                }*/
                else {
                    scl_err("[SL] don't support scl_feature = %d src_fmt [%x] dst_fmt [%x] job\n",
                            node->property.vproperty[i].scl_feature, node->property.src_fmt, node->property.dst_fmt);
                    DEBUG_M(LOG_ERROR, node->engine, node->minor,
                            "[SL] V.G JOB ID(%x) scl_feature = %d, src_fmt (%x) dst_fmt (%x) FAIL\n",
                            node->job_id, node->property.vproperty[i].scl_feature, node->property.src_fmt, node->property.dst_fmt);
                    return -1;
                }
            }
            // when 1frame_1frame to 1frame_1frame only do cvbs zoom, but also need to do buffer clean,
            // add job to do fix pattern draw lcd0 frame to black
            if ((node->property.vproperty[i].scl_feature == (1 << 2)) && (node->property.vproperty[i].buf_clean == 1)) {
                if ((node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)) {
                    ret = fscaler300_get_vframe_to_1frame1frame_buf_clean_jobcnt(node, i);
                    if (ret < 0)
                        return -1;
                    vch_cnt++;
                }
            }
        }
    }
    return vch_cnt;
}

/*
 * set mulitple node channel bitmask
 */
int fscaler300_set_ch_bitmask(job_node_t *node, ch_bitmask_t *ch_bitmask)
{
    int chmask_base, chmask_idx;
    job_node_t *curr, *ne;

    /* set parent node ch_bitmask */
    chmask_base = BITMASK_BASE(node->idx);
    chmask_idx = BITMASK_IDX(node->idx);
    ch_bitmask->mask[chmask_base] |= (0x1 << chmask_idx);

    /* set child node ch_bitmask */
    list_for_each_entry_safe(curr, ne, &node->clist, clist) {
        chmask_base = BITMASK_BASE(curr->idx);
        chmask_idx = BITMASK_IDX(curr->idx);
        ch_bitmask->mask[chmask_base] |= (0x1 << chmask_idx);
    }

    return 0;
}

/*
 * calculate this node need how many blocks, offer block info to fscaler300_set_node_chain to set chain
 */
int fscaler300_get_mask_osd_blk_cnt(job_node_t *node)
{
    int chn;//, pp_sel, win_idx;
    int res123 = 0, mask5678 = 0;
#if GM8210
    int osd0123456 = 0, osd7 = 0;
#endif
    //fscaler300_global_param_t *global_param = &priv.global_param;

    node->node_info.blk_cnt = 1;
    chn = node->minor;
    // if frame to cascade frame --> use sd 1 to scaling another frame
    //if ((node->property.src_fmt == TYPE_YUV422_RATIO_FRAME) || (node->property.src_fmt == TYPE_YUV422_FRAME))
      //  res123 = 1;
    // if ratio to 3 frame --> src is progressive : use sd 1 frm2field to generate CVBS frame
    // if ratio to 3 frame --> src is interlace : use sd 1 scaling to generate CVBS frame
    // if frame to 3 frame --> use sd 1 scaling to generate bottom frame
    if ((node->property.src_fmt == TYPE_YUV422_RATIO_FRAME) || (node->property.src_fmt == TYPE_YUV422_FRAME)) {
        if (node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
            res123 = 1;
    }
    // if ratio to 1frame2frame --> use sd 1 scaling to generate cvbs frame
    if ((node->property.src_fmt == TYPE_YUV422_RATIO_FRAME) && (node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES))
        res123 = 1;
    /* 3 frame to 3 frame --> virtual job no mask & osd */
    /* 1frame2frame to 1frame2frame --> virtual job no mask & osd */
    /* 1frame1frame to 1frame1frame --> virtual job no mask & osd */
    /* frame to 1frame1frame --> clear win no mask & osd */
    /* frame to 1frame2frame --> clear win no mask & osd */
    /* rgb565 to rgb565 no mask & osd */
    /* frame to cascade frame no mask & osd */
#if 0   /* mask & osd external api do not use, remove it for performance */
    if (((node->property.src_fmt != TYPE_YUV422_2FRAMES_2FRAMES) && (node->property.dst_fmt != TYPE_YUV422_2FRAMES_2FRAMES)) ||
        ((node->property.src_fmt != TYPE_YUV422_FRAME_2FRAMES) && (node->property.dst_fmt != TYPE_YUV422_FRAME_2FRAMES)) ||
        ((node->property.src_fmt != TYPE_YUV422_FRAME_FRAME) && (node->property.dst_fmt != TYPE_YUV422_FRAME_FRAME)) ||
        ((node->property.src_fmt != TYPE_YUV422_FRAME) && (node->property.dst_fmt != TYPE_YUV422_FRAME_2FRAMES)) ||
        ((node->property.src_fmt != TYPE_YUV422_FRAME) && (node->property.dst_fmt != TYPE_YUV422_FRAME_FRAME)) ||
        ((node->property.src_fmt != TYPE_RGB565) && (node->property.dst_fmt != TYPE_RGB565)) ||
        ((node->property.src_fmt != TYPE_YUV422_FRAME) && (node->property.dst_fmt != TYPE_YUV422_CASCADE_SCALING_FRAME))) {
    //if mask 5678 enable, need 1 block
    for (win_idx = 4; win_idx < MASK_MAX; win_idx++) {
        pp_sel = atomic_read(&global_param->chn_global[chn].mask_pp_sel[win_idx]);
        if (!pp_sel)
            continue;

        if (global_param->chn_global[chn].mask[pp_sel - 1][win_idx].enable)
            mask5678 = 1;
    }
#if GM8210
    //if osd 1~7 enable, need 1 block, if osd 8 enable, need 1 block
    for (win_idx = 0; win_idx < OSD_MAX; win_idx++) {
        pp_sel = atomic_read(&global_param->chn_global[chn].osd_pp_sel[win_idx]);
        if (!pp_sel)
            continue;

        if (global_param->chn_global[chn].osd[pp_sel - 1][win_idx].enable) {
            if (win_idx <= 6)
                osd0123456 = 1;
            else
                osd7 = 1;
        }
    }
#endif
    }
#endif  // end of if 0
    if (res123 || mask5678)
        node->node_info.blk_cnt++;
#if GM8210
    if (osd0123456)
        node->node_info.blk_cnt++;
    if (osd7)
        node->node_info.blk_cnt++;
#endif
    return 0;
}

/*
 * set cascade node parameter
 */
int fscaler300_put_cascade_node(job_node_t *node)
{
    int i, j, k;
    int chain_job_cnt = 0, chain_blk_cnt = 0, ret = 0;
    int vjob_cnt = 0, vblk_cnt = 0, total_jobs = 0;
    fscaler300_job_t *parent = 0;
    fscaler300_job_t *sub_parent = NULL;
    fscaler300_job_t *child, *child1;
    int chmask_base, chmask_idx;
    int chain_list_cnt = 0;
    int tmp_chain_list_cnt = 0;
    struct list_head vnode_list;
    struct list_head chain_list;
    int job_cnt = 0, blk_cnt = 0;
    int ch_id = node->ch_id;
    cas_info_t *cas_info;

    cas_info = kmalloc(sizeof(cas_info_t), GFP_KERNEL);
    memset(cas_info, 0x0, sizeof(cas_info_t));

    //scl_dbg("%s %d\n", __func__, node->job_id);
    INIT_LIST_HEAD(&vnode_list);
    INIT_LIST_HEAD(&chain_list);

    /* get job_cnt of this node */
    ret = fscaler300_get_frame_to_cascade_jobcnt(node, cas_info);
    if (ret < 0)
        return -1;

    job_cnt = node->node_info.job_cnt;
    blk_cnt = node->node_info.blk_cnt;

    chain_list_cnt = 1;
    /* calculate how many chain list we need */
    for (i = 0; i < job_cnt; i++) {
        vjob_cnt += 1;
        vblk_cnt += 2;

        if (vjob_cnt > MAX_CHAIN_JOB || vblk_cnt > MAX_CHAIN_LL_BLK) {
            chain_list_cnt++;
            vjob_cnt = 0;
            vblk_cnt = 0;
        }
    }

    /* tmp_chain_list_cnt used for cvbs frame tiny scaling */
    tmp_chain_list_cnt = chain_list_cnt;
    /* do nothing for this job */
    if (job_cnt == 0) {
        node->status = JOB_STS_DONOTHING;
        vg_nodelist_add(node);
#ifdef USE_KTHREAD
        callback_wake_up();
        callback_2ddma_wake_up();
#endif
#ifdef USE_WQ
        /* schedule the delay workq */
        PREPARE_DELAYED_WORK(&process_callback_work,(void *)fscaler300_callback_process);
        queue_delayed_work(callback_workq, &process_callback_work, callback_period);

        PREPARE_DELAYED_WORK(&process_2ddma_callback_work,(void *)fscaler300_2ddma_callback_process);
        queue_delayed_work(callback_workq, &process_2ddma_callback_work, callback_period);
#endif
        return 0;
    }
    /* do line correct & scaling */
    for (i = 0; i < job_cnt; i++) {
        fscaler300_job_t    *job = NULL;
        /* get virtual channel number */
        vjob_cnt = 1;
        vblk_cnt = 2;
        chain_job_cnt += vjob_cnt;
        chain_blk_cnt += vblk_cnt;

        if (chain_job_cnt > MAX_CHAIN_JOB || chain_blk_cnt > MAX_CHAIN_LL_BLK) {
            chain_job_cnt -= vjob_cnt;
            chain_blk_cnt -= vblk_cnt;
            break;
        }

        /* init job & set job paramter */
        if (in_interrupt() || in_atomic())
            job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
        else
            job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
        if (unlikely(!job))
            panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
        memset(job, 0x0, sizeof(fscaler300_job_t));

        if (chain_list_cnt > 1)         // more than 1 chain list
            job->job_type = VIRTUAL_MJOB;
        else                            // only 1 chain list
            job->job_type = VIRTUAL_JOB;

        ret = fscaler300_init_cas_job(job, node, ch_id, i, cas_info);
        if (ret < 0)
            return -1;

        /* set last job need_callback = 1 */
        if (chain_list_cnt > 1)         // only last chain list's last job callback = 1
            job->need_callback = 0;
        else {                          // set last job need_callback = 1
            if (i == job_cnt - 1)
                job->need_callback = 1;
            else
                job->need_callback = 0;
        }

        /* for job preformance used */
        if (i == 0) {                     // parent job
            job->parent = job;
            parent = job;
            job->perf_type = TYPE_FIRST;
        }
        else if (i == job_cnt - 1) {  // last job, child job
            job->parent = parent;
            job->perf_type = TYPE_LAST;
            list_add_tail(&job->clist, &parent->clist);
        }
        else {                                      // middle job, child job
            job->parent = parent;
            job->perf_type = TYPE_MIDDLE;
            list_add_tail(&job->clist, &parent->clist);
        }
        /* add ref_cnt to parent job */
        REFCNT_INC(parent);
    }

    /* set parent job job_count */
    parent->param.lli.job_count = parent->job_cnt = chain_job_cnt;
    /* set child job job_count */
    list_for_each_entry_safe(child, child1, &parent->clist, clist)
        child->param.lli.job_count = child->job_cnt = chain_job_cnt;

    list_add_tail(&parent->plist, &chain_list);
    chain_list_cnt--;
    /* set ch_bitmask to parent job */
    chmask_base = BITMASK_BASE(node->idx);
    chmask_idx = BITMASK_IDX(node->idx);
    parent->ch_bitmask.mask[chmask_base] |= (0x1 << chmask_idx);

    // if more than 1 chain list, generate second/third.... chain list
    for (k = 0; k < chain_list_cnt; k++) {
        // get how many jobs have to be in second/third.... chain list
        total_jobs = job_cnt - i;
        chain_job_cnt = chain_blk_cnt = 0;

        for (j = 0; j < total_jobs; j++) {
            fscaler300_job_t    *job = NULL;
            vjob_cnt = 1;
            vblk_cnt = 2;
            chain_job_cnt += vjob_cnt;
            chain_blk_cnt += vblk_cnt;

            if (chain_job_cnt > MAX_CHAIN_JOB || chain_blk_cnt > MAX_CHAIN_LL_BLK) {
                chain_job_cnt -= vjob_cnt;
                chain_blk_cnt -= vblk_cnt;
                break;
            }
            /* init job & set job paramter */
            if (in_interrupt() || in_atomic())
                job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
            else
                job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
            if (unlikely(!job))
                panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
            memset(job, 0x0, sizeof(fscaler300_job_t));

            job->job_type = VIRTUAL_MJOB;       // more than 1 chain list, jobs are all VIRTUAL_MJOB

            ret = fscaler300_init_cas_job(job, node, ch_id, i + j, cas_info);
            if (ret < 0)
                return -1;

            /* set last job need_callback = 1 */
            if (j == total_jobs - 1)
                 job->need_callback = 1;
            else
                job->need_callback = 0;

            job->parent = parent;

            /* for job preformance used */
            if (j == 0) {                     // sub parent job
                sub_parent = job;
                job->perf_type = TYPE_MIDDLE;
                list_add_tail(&job->plist, &chain_list);
            }
            else if (j == total_jobs - 1) {  // last job, child job
                if (chain_list_cnt > 1)             // more than 1 chain list, last job in this chain list are TYPE_MIDDLE
                    job->perf_type = TYPE_MIDDLE;
                else
                    job->perf_type = TYPE_LAST;
                list_add_tail(&job->clist, &sub_parent->clist);
            }
            else {                                      // middle job, child job
                job->perf_type = TYPE_MIDDLE;
                list_add_tail(&job->clist, &sub_parent->clist);
            }
            /* add ref_cnt to parent job */
            REFCNT_INC(parent);

        }

        /* set sub parent job job_count */
        sub_parent->param.lli.job_count = sub_parent->job_cnt = chain_job_cnt;
        /* set child job job_count */
        list_for_each_entry_safe(child, child1, &sub_parent->clist, clist)
            child->param.lli.job_count = child->job_cnt = chain_job_cnt;
        /* set ch_bitmask to sub parent job */
        sub_parent->ch_bitmask.mask[chmask_base] |= (0x1 << chmask_idx);
    }

    /* add new job to engine */
    if (node->priority == 0){
        fscaler300_chainlist_add(&chain_list);
        /* add to node list */
        vg_nodelist_add(node);
    }
    else
        fscaler300_prioritylist_add(parent);

    /* schedule job to hardware engine */
    fscaler300_put_job();

    kfree(cas_info);

    return 0;
}

/*
 * set single node parameter
 */
int fscaler300_put_single_node(job_node_t *node)
{
    int i;
    int job_cnt = 0, ret = 0;
    int chmask_base, chmask_idx;
    fscaler300_job_t *parent = 0;
    int ch_id = node->ch_id;

    if (flow_check)printm(MODULE_NAME, "%s %u\n", __func__, node->job_id);
    /* get job_cnt of this node */
    fscaler300_get_mask_osd_blk_cnt(node);
    ret = fscaler300_get_snode_jobcnt(node);
    if (ret < 0)
        return -1;

    if ((node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
        (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)) {
        if (!(node->property.vproperty[ch_id].scl_feature & (1 << 2)))
            job_cnt = node->property.vproperty[ch_id].vnode_info.job_cnt;
    }
    else
        job_cnt = node->node_info.job_cnt;

    if (job_cnt < 0) {
        printk("[%s] job_cnt < 0\n", __func__);
        return -1;
    }
    /* do nothing for this job */
    if (job_cnt == 0) {
        node->status = JOB_STS_DONOTHING;
        vg_nodelist_add(node);
#ifdef USE_KTHREAD
        callback_wake_up();
        callback_2ddma_wake_up();
#endif
#ifdef USE_WQ
        /* schedule the delay workq */
        if (dual_debug == 1) printm(MODULE_NAME, "waku up callback %d\n", __LINE__);
        PREPARE_DELAYED_WORK(&process_callback_work,(void *)fscaler300_callback_process);
        queue_delayed_work(callback_workq, &process_callback_work, callback_period);

        PREPARE_DELAYED_WORK(&process_2ddma_callback_work,(void *)fscaler300_2ddma_callback_process);
        queue_delayed_work(callback_workq, &process_2ddma_callback_work, callback_period);
#endif
        return 0;
    }
    /* do normal scaling or line correct for this job */
    for (i = 0; i < job_cnt; i++) {
        fscaler300_job_t    *job = NULL;
        if (in_interrupt() || in_atomic())
            job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
        else
            job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
        if (unlikely(!job))
            panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
        memset(job, 0x0, sizeof(fscaler300_job_t));

        job->job_cnt = job_cnt;
        job->job_type = SINGLE_JOB;

        if (node->property.vproperty[ch_id].aspect_ratio.enable == 0 && node->property.vproperty[ch_id].border.enable == 0)
            ret = fscaler300_init_job(job, node, ch_id, i);
        if (node->property.vproperty[ch_id].aspect_ratio.enable == 0 && node->property.vproperty[ch_id].border.enable == 1)
            ret = fscaler300_init_border_job(job, node, ch_id, i);
        if (node->property.vproperty[ch_id].aspect_ratio.enable == 1 && node->property.vproperty[ch_id].border.enable == 0)
            ret = fscaler300_init_aspect_ratio_job(job, node, ch_id, i);
        if (node->property.vproperty[ch_id].aspect_ratio.enable == 1 && node->property.vproperty[ch_id].border.enable == 1)
            ret = fscaler300_init_aspect_ratio_border_job(job, node, ch_id, i);
        if (ret < 0) {
            printk("[%s] init job fail\n", __func__);
            return -1;
        }

        /* set last job need_callback = 1 */
        if (i == job_cnt - 1)
            job->need_callback = 1;
        else
            job->need_callback = 0;

        /* for job performance used */
        /* parent job */
        if (job_cnt == 1) {
            job->parent = job;
            parent = job;
            job->perf_type = TYPE_ALL;
        }
        if (job_cnt > 1) {
            /* parent job */
            if (i == 0) {
                job->parent = job;
                parent = job;
                job->perf_type = TYPE_FIRST;
            }
            /* child job */
            if ((i > 0) && (i < job_cnt - 1)) {
                job->parent = parent;
                job->perf_type = TYPE_MIDDLE;
                list_add_tail(&job->clist, &parent->clist);
            }
            if (i == job_cnt - 1) {
                job->parent = parent;
                job->perf_type = TYPE_LAST;
                list_add_tail(&job->clist, &parent->clist);
            }
        }
        /* add ref_cnt to parent job */
        REFCNT_INC(parent);
    }

    chmask_base = BITMASK_BASE(node->idx);
    chmask_idx = BITMASK_IDX(node->idx);
    parent->ch_bitmask.mask[chmask_base] |= (0x1 << chmask_idx);

    /* add new job to engine */
    if (node->priority == 0) {
        fscaler300_joblist_add(parent);
        /* add to node list */
        vg_nodelist_add(node);
    }
    else
        fscaler300_prioritylist_add(parent);

    /* schedule job to hardware engine */
    fscaler300_put_job();

    return 0;
}

int fscaler300_set_cvbs_zoom_job(job_node_t *node)
{
    int i = 0;
    fscaler300_job_t *parent = NULL;
    fscaler300_job_t *child, *child1;
    int chmask_base, chmask_idx;
    int job_cnt;

    if (debug_mode >= 2)
        printm(MODULE_NAME, "%s %d\n", __func__, node->job_id);

    fscaler300_get_cvbs_zoom_jobcnt(node);
    job_cnt = node->node_info.job_cnt;
    /* init job & set job paramter */
    for (i = 0; i < job_cnt; i++) {
        fscaler300_job_t    *job = NULL;

        if (in_interrupt() || in_atomic())
            job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
        else
            job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
        if (unlikely(!job))
            panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
        memset(job, 0x0, sizeof(fscaler300_job_t));

        job->job_type = VIRTUAL_JOB;

        fscaler300_init_cvbs_zoom_job(job, node, i);

        /* set last job need_callback = 1 */
        if (i == job_cnt - 1)
            job->need_callback = 1;
        else
            job->need_callback = 0;

        /* for job preformance used */
        if (job_cnt == 1) {
            job->parent = job;
            parent = job;
            job->perf_type = TYPE_ALL;
        }
        if (job_cnt > 1) {
            if (i == 0) {
                job->parent = job;
                parent = job;
                if (job_cnt == 1)
                    job->perf_type = TYPE_ALL;
                else
                    job->perf_type = TYPE_FIRST;
            }
            if ((i > 0) && (i < job_cnt - 1)) {
                job->parent = parent;
                job->perf_type = TYPE_MIDDLE;
                list_add_tail(&job->clist, &parent->clist);
            }
            if (i == job_cnt - 1) {
                job->parent = parent;
                job->perf_type = TYPE_LAST;
                list_add_tail(&job->clist, &parent->clist);
            }
        }
        /* add ref_cnt to parent job */
        REFCNT_INC(parent);
    }

    /* set tmp parent job job_count */
    parent->param.lli.job_count = parent->job_cnt = job_cnt;
    /* set child job job_count */
    list_for_each_entry_safe(child, child1, &parent->clist, clist)
        child->param.lli.job_count = child->job_cnt = job_cnt;

    chmask_base = BITMASK_BASE(node->idx);
    chmask_idx = BITMASK_IDX(node->idx);
    parent->ch_bitmask.mask[chmask_base] |= (0x1 << chmask_idx);

    /* add new job to engine */
    if (node->priority == 0){
        fscaler300_joblist_add(parent);
        /* add to node list */
        vg_nodelist_add(node);
    }
    else
        fscaler300_prioritylist_add(parent);

    return 0;
}

/*
 * pip jobs are after line correct or scaling
 */
int fscaler300_set_pip_job0(job_node_t *node, fscaler300_job_t *parent, fscaler300_job_t *sub_parent, int chain_list_cnt)
{
    int i = 0;
    int chain_job_cnt = 0;
    fscaler300_job_t *last = NULL;
    fscaler300_job_t *child, *child1;
    int job_cnt = 0;

    fscaler300_get_pip_jobcnt(node);
    job_cnt = node->node_info.job_cnt;
    /* add pip job count to parent */
    chain_job_cnt = sub_parent->job_cnt;
    chain_job_cnt += job_cnt;

    /* modify parent's last job parameter as middle job */
    last = list_entry(sub_parent->clist.prev, fscaler300_job_t, clist);
    last->perf_type = TYPE_MIDDLE;
    last->need_callback = 0;

    /* init job & set job paramter */
    for (i = 0; i < job_cnt; i++) {
        fscaler300_job_t    *job = NULL;

        if (in_interrupt() || in_atomic())
            job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
        else
            job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
        if (unlikely(!job))
            panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
        memset(job, 0x0, sizeof(fscaler300_job_t));

        if (chain_list_cnt > 1)         // more than 1 chain list
            job->job_type = VIRTUAL_MJOB;
        else                            // only 1 chain list
            job->job_type = VIRTUAL_JOB;

        fscaler300_init_pip_job(job, node, i);

        /* set last job need_callback = 1 */
        if (i == job_cnt - 1)
            job->need_callback = 1;
        else
            job->need_callback = 0;

        job->parent = parent;

        /* for job preformance used */
        if (i != job_cnt - 1) {                     // sub parent job
            job->perf_type = TYPE_MIDDLE;
            list_add_tail(&job->clist, &sub_parent->clist);
        }
        else {
            job->perf_type = TYPE_LAST;
            list_add_tail(&job->clist, &sub_parent->clist);
        }
        /* add ref_cnt to parent job */
        REFCNT_INC(parent);
    }

    /* set tmp parent job job_count */
    sub_parent->param.lli.job_count = sub_parent->job_cnt = chain_job_cnt;
    /* set child job job_count */
    list_for_each_entry_safe(child, child1, &sub_parent->clist, clist)
        child->param.lli.job_count = child->job_cnt = chain_job_cnt;

    return 0;
}

/*
 * cvbs jobs are after line correct or scaling and pip
 */
int fscaler300_set_cvbs_zoom_job0(job_node_t *node, fscaler300_job_t *parent, fscaler300_job_t *sub_parent, int chain_list_cnt)
{
    int i = 0;
    int chain_job_cnt = 0;
    fscaler300_job_t *last = NULL;
    fscaler300_job_t *child, *child1;
    int job_cnt = 0;

    fscaler300_get_cvbs_zoom_jobcnt(node);
    job_cnt = node->node_info.job_cnt;
    /* add cvbs tiny scaling job count to parent */
    chain_job_cnt = sub_parent->job_cnt;
    chain_job_cnt += job_cnt;

    /* modify parent's last job parameter as middle job */
    last = list_entry(sub_parent->clist.prev, fscaler300_job_t, clist);
    last->perf_type = TYPE_MIDDLE;
    last->need_callback = 0;

    /* init job & set job paramter */
    for (i = 0; i < job_cnt; i++) {
        fscaler300_job_t    *job = NULL;

        if (in_interrupt() || in_atomic())
            job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
        else
            job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
        if (unlikely(!job))
            panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
        memset(job, 0x0, sizeof(fscaler300_job_t));

        if (chain_list_cnt > 1)         // more than 1 chain list
            job->job_type = VIRTUAL_MJOB;
        else                            // only 1 chain list
            job->job_type = VIRTUAL_JOB;

        fscaler300_init_cvbs_zoom_job(job, node, i);

        /* set last job need_callback = 1 */
        if (i == job_cnt - 1)
            job->need_callback = 1;
        else
            job->need_callback = 0;

        job->parent = parent;

        /* for job preformance used */
        if (i != job_cnt - 1) {                     // sub parent job
            job->perf_type = TYPE_MIDDLE;
            list_add_tail(&job->clist, &sub_parent->clist);
        }
        else {
            job->perf_type = TYPE_LAST;
            list_add_tail(&job->clist, &sub_parent->clist);
        }
        /* add ref_cnt to parent job */
        REFCNT_INC(parent);
    }

    /* set tmp parent job job_count */
    sub_parent->param.lli.job_count = sub_parent->job_cnt = chain_job_cnt;
    /* set child job job_count */
    list_for_each_entry_safe(child, child1, &sub_parent->clist, clist)
        child->param.lli.job_count = child->job_cnt = chain_job_cnt;

    return 0;
}

/*
 * pip jobs are first job
 */
int fscaler300_set_pip_job(job_node_t *node, int zoom_flag)
{
    int i = 0;
    fscaler300_job_t *parent = NULL;
    fscaler300_job_t *child, *child1;
    int chmask_base, chmask_idx;
    int job_cnt = 0;

    fscaler300_get_pip_jobcnt(node);
    job_cnt = node->node_info.job_cnt;

    /* init job & set job paramter */
    for (i = 0; i < job_cnt; i++) {
        fscaler300_job_t    *job = NULL;

        if (in_interrupt() || in_atomic())
            job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
        else
            job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
        if (unlikely(!job))
            panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
        memset(job, 0x0, sizeof(fscaler300_job_t));

        job->job_type = VIRTUAL_JOB;

        fscaler300_init_pip_job(job, node, i);

        /* set last job need_callback = 1 */
        if (i == job_cnt - 1)
            job->need_callback = 1;
        else
            job->need_callback = 0;

        /* for job preformance used */
        if (job_cnt == 1) {
            job->parent = job;
            parent = job;
            job->perf_type = TYPE_ALL;
        }
        if (job_cnt > 1) {
            if (i == 0) {
                job->parent = job;
                parent = job;
                if (job_cnt == 1)
                    job->perf_type = TYPE_ALL;
                else
                    job->perf_type = TYPE_FIRST;
            }
            if ((i > 0) && (i < job_cnt - 1)) {
                job->parent = parent;
                job->perf_type = TYPE_MIDDLE;
                list_add_tail(&job->clist, &parent->clist);
            }
            if (i == job_cnt - 1) {
                job->parent = parent;
               job->perf_type = TYPE_LAST;
                list_add_tail(&job->clist, &parent->clist);
            }
        }
        /* add ref_cnt to parent job */
        REFCNT_INC(parent);
    }

    /* set tmp parent job job_count */
    parent->param.lli.job_count = parent->job_cnt = job_cnt;
    /* set child job job_count */
    list_for_each_entry_safe(child, child1, &parent->clist, clist)
        child->param.lli.job_count = child->job_cnt = job_cnt;

    chmask_base = BITMASK_BASE(node->idx);
    chmask_idx = BITMASK_IDX(node->idx);
    parent->ch_bitmask.mask[chmask_base] |= (0x1 << chmask_idx);
    // zoom
    if (zoom_flag == 1)
        fscaler300_set_cvbs_zoom_job0(node, parent, parent, 1);

    /* add new job to engine */
    if (node->priority == 0) {
        fscaler300_joblist_add(parent);
        /* add to node list */
        vg_nodelist_add(node);
    }
    else
        fscaler300_prioritylist_add(parent);

    return 0;
}

/*
 * set multiple window node parameter
 */
int fscaler300_put_virtual_node(job_node_t *node)
{
    int i, j, k, ch_id;
    int vch_cnt = 0, chain_job_cnt = 0, chain_blk_cnt = 0, ret = 0;
    int vjob_cnt = 0, vblk_cnt = 0, total_jobs = 0;
    fscaler300_job_t *parent = 0;
    fscaler300_job_t *sub_parent = NULL;
    fscaler300_job_t *child, *child1;
    int chmask_base, chmask_idx;
    int chain_list_cnt = 0;
    int tmp_chain_list_cnt = 0;
    int tmp_chain_job_cnt = 0;
    fscaler300_job_t *tmp_parent = NULL;
    struct list_head vnode_list;
    struct list_head chain_list;
    int flag = 0;
    int zoom_flag = 0;
    int pip_flag = 0;

    if (flow_check)printm(MODULE_NAME, "%s %u\n", __func__, node->job_id);
    INIT_LIST_HEAD(&vnode_list);
    INIT_LIST_HEAD(&chain_list);

    /* get job_cnt of this node */
    vch_cnt = fscaler300_get_vnode_jobcnt(node, &zoom_flag, &pip_flag);
    if (vch_cnt < 0) {
        printk("[%s] vch_cnt < 0\n", __func__);
        return -1;
    }

    chain_list_cnt = 1;
    /* calculate how many chain list we need */
    for (ch_id = 0; ch_id <= virtual_chn_num; ch_id++) {
        if (node->property.vproperty[ch_id].vch_enable == 1) {
            vjob_cnt += node->property.vproperty[ch_id].vnode_info.job_cnt;
            vblk_cnt += node->property.vproperty[ch_id].vnode_info.blk_cnt;

            if (vjob_cnt > MAX_CHAIN_JOB || vblk_cnt > MAX_CHAIN_LL_BLK) {
                chain_list_cnt++;
                vjob_cnt = node->property.vproperty[ch_id].vnode_info.job_cnt;
                vblk_cnt = node->property.vproperty[ch_id].vnode_info.blk_cnt;
            }
        }
    }
    /* tmp_chain_list_cnt used for cvbs frame tiny scaling */
    tmp_chain_list_cnt = chain_list_cnt;
    /* do nothing for this job */
    if ((vch_cnt == 0) && (zoom_flag == 0) && (pip_flag == 0)) {
        node->status = JOB_STS_DONOTHING;
        vg_nodelist_add(node);
#ifdef USE_KTHREAD
        callback_wake_up();
        callback_2ddma_wake_up();
#endif
#ifdef USE_WQ
        /* schedule the delay workq */
        if (dual_debug == 1) printm(MODULE_NAME, "waku up callback %d\n", __LINE__);
        PREPARE_DELAYED_WORK(&process_callback_work,(void *)fscaler300_callback_process);
        queue_delayed_work(callback_workq, &process_callback_work, callback_period);

        PREPARE_DELAYED_WORK(&process_2ddma_callback_work,(void *)fscaler300_2ddma_callback_process);
        queue_delayed_work(callback_workq, &process_2ddma_callback_work, callback_period);
#endif
        return 0;
    }

    if (vch_cnt == 0) {
    /* only do cvbs frame zoom, no virtual channel scaling or line correct */
        if ((zoom_flag == 1) && (pip_flag == 0)) {
            ret = fscaler300_set_cvbs_zoom_job(node);
            /* schedule job to hardware engine */
            fscaler300_put_job();
        }
        /* only do pip, no virtual channel scaling or line correct */
        if ((zoom_flag == 0) && (pip_flag == 1)) {
            ret = fscaler300_set_pip_job(node, zoom_flag);
            /* schedule job to hardware engine */
            fscaler300_put_job();
        }
        /* do pip + cvbs zoom, no virtual channel scaling or line correct */
        if ((zoom_flag == 1) && (pip_flag == 1)) {
            ret = fscaler300_set_pip_job(node, zoom_flag);
            /* schedule job to hardware engine */
            fscaler300_put_job();
        }
        return 0;
    }

    /* do line correct & scaling */
    for (i = 0; i < vch_cnt; i++) {
        flag = 0;
        /* get virtual channel number */
        for (ch_id = 0; ch_id <= virtual_chn_num; ch_id++) {
            if ((node->property.vproperty[ch_id].vch_enable == 1) && (node->property.vproperty[ch_id].vnode_info.job_cnt)) {
                vjob_cnt = node->property.vproperty[ch_id].vnode_info.job_cnt;
                vblk_cnt = node->property.vproperty[ch_id].vnode_info.blk_cnt;
                chain_job_cnt += vjob_cnt;
                chain_blk_cnt += vblk_cnt;

                if (chain_job_cnt > MAX_CHAIN_JOB || chain_blk_cnt > MAX_CHAIN_LL_BLK) {
                    chain_job_cnt -= vjob_cnt;
                    chain_blk_cnt -= vblk_cnt;
                    flag = 1;
                    break;
                }
                node->property.vproperty[ch_id].vch_enable = 0;
                break;
            }
        }
        /* if chain job cnt or chain blk cnt over limit --> next chain list */
        if (flag)
            break;

        /* init job & set job paramter */
        for (j = 0; j < vjob_cnt; j++) {
            fscaler300_job_t    *job = NULL;

            if (in_interrupt() || in_atomic())
                job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
            else
                job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
            if (unlikely(!job))
                panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
            memset(job, 0x0, sizeof(fscaler300_job_t));

            if (chain_list_cnt > 1)         // more than 1 chain list
                job->job_type = VIRTUAL_MJOB;
            else                            // only 1 chain list
                job->job_type = VIRTUAL_JOB;
            // only do buffer clean when scl_feature = 0
            if ((node->property.vproperty[ch_id].scl_feature == 0 && node->property.vproperty[ch_id].buf_clean == 1) ||
                (node->property.vproperty[ch_id].scl_feature == (1 << 4) && node->property.vproperty[ch_id].buf_clean == 1) ||
                (node->property.vproperty[ch_id].scl_feature == (1 << 5) && node->property.vproperty[ch_id].buf_clean == 1))
                ret = fscaler300_init_job(job, node, ch_id, j);
            if ((node->property.vproperty[ch_id].scl_feature & (1 << 0)) || (node->property.vproperty[ch_id].scl_feature & (1 << 1)) ||
                (node->property.vproperty[ch_id].scl_feature & (1 << 2))) {
                if (node->property.vproperty[ch_id].aspect_ratio.enable == 0 && node->property.vproperty[ch_id].border.enable == 0)
                    ret = fscaler300_init_job(job, node, ch_id, j);
                if (node->property.vproperty[ch_id].aspect_ratio.enable == 0 && node->property.vproperty[ch_id].border.enable == 1)
                    ret = fscaler300_init_border_job(job, node, ch_id, j);
            }
            if (node->property.vproperty[ch_id].scl_feature & (1 << 0)) {
                if (node->property.vproperty[ch_id].aspect_ratio.enable == 1 && node->property.vproperty[ch_id].border.enable == 0)
                    ret = fscaler300_init_aspect_ratio_job(job, node, ch_id, j);
                if (node->property.vproperty[ch_id].aspect_ratio.enable == 1 && node->property.vproperty[ch_id].border.enable == 1)
                    ret = fscaler300_init_aspect_ratio_border_job(job, node, ch_id, j);
            }
            if (ret < 0) {
                printk("[%s] init job fail\n", __func__);
                return -1;
            }

            /* set last job need_callback = 1 */
            if (chain_list_cnt > 1)         // only last chain list's last job callback = 1
                job->need_callback = 0;
            else {                          // set last job need_callback = 1
                if ((i == vch_cnt - 1) && (j == vjob_cnt - 1))
                    job->need_callback = 1;
                else
                    job->need_callback = 0;
            }

            /* for job preformance used */
            if (i == 0 && j == 0) {                     // parent job
                job->parent = job;
                parent = job;
                job->perf_type = TYPE_FIRST;
            }
            else if ((i == vch_cnt - 1) && (j == chain_job_cnt - 1)) {  // last job, child job
                job->parent = parent;
                if (chain_list_cnt > 1)             // more than 1 chain list, last job in first chain list are TYPE_MIDDLE
                    job->perf_type = TYPE_MIDDLE;
                else
                    job->perf_type = TYPE_LAST;

                list_add_tail(&job->clist, &parent->clist);
            }
            else {                                      // middle job, child job
                job->parent = parent;
                job->perf_type = TYPE_MIDDLE;
                list_add_tail(&job->clist, &parent->clist);
            }
            /* add ref_cnt to parent job */
            REFCNT_INC(parent);
        }
    }

    /* set parent job job_count */
    parent->param.lli.job_count = parent->job_cnt = chain_job_cnt;
    /* set child job job_count */
    list_for_each_entry_safe(child, child1, &parent->clist, clist)
        child->param.lli.job_count = child->job_cnt = chain_job_cnt;

    list_add_tail(&parent->plist, &chain_list);
    chain_list_cnt--;
    /* set ch_bitmask to parent job */
    chmask_base = BITMASK_BASE(node->idx);
    chmask_idx = BITMASK_IDX(node->idx);
    parent->ch_bitmask.mask[chmask_base] |= (0x1 << chmask_idx);

    // if more than 1 chain list, generate second/third.... chain list
    for (k = 0; k < chain_list_cnt; k++) {
        // get how many jobs have to be in second/third.... chain list
        total_jobs = vch_cnt = vch_cnt - i;
        chain_job_cnt = chain_blk_cnt = 0;

        for (i = 0; i < total_jobs; i++) {
            flag = 0;
            /* get virtula channel number */
            for (ch_id = 0; ch_id <= virtual_chn_num; ch_id++) {
                if ((node->property.vproperty[ch_id].vch_enable == 1) && (node->property.vproperty[ch_id].vnode_info.job_cnt)) {
                    vjob_cnt = node->property.vproperty[ch_id].vnode_info.job_cnt;
                    vblk_cnt = node->property.vproperty[ch_id].vnode_info.blk_cnt;
                    chain_job_cnt += vjob_cnt;
                    chain_blk_cnt += vblk_cnt;

                    if (chain_job_cnt > MAX_CHAIN_JOB || chain_blk_cnt > MAX_CHAIN_LL_BLK) {
                        chain_job_cnt -= vjob_cnt;
                        chain_blk_cnt -= vblk_cnt;
                        flag = 1;
                        break;
                    }
                    node->property.vproperty[ch_id].vch_enable = 0;
                    break;
                }
            }
            /* if chain job cnt or chain blk cnt over limit --> next chain list */
            if (flag)
                break;

            /* init job & set job paramter */
            for (j = 0; j < vjob_cnt; j++) {
                fscaler300_job_t    *job = NULL;

                if (in_interrupt() || in_atomic())
                    job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
                else
                    job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
                if (unlikely(!job))
                    panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
                memset(job, 0x0, sizeof(fscaler300_job_t));

                job->job_type = VIRTUAL_MJOB;       // more than 1 chain list, jobs are all VIRTUAL_MJOB

                // only do buffer clean when scl_feature = 0
                if ((node->property.vproperty[ch_id].scl_feature == 0 && node->property.vproperty[ch_id].buf_clean == 1) ||
                    (node->property.vproperty[ch_id].scl_feature == (1 << 4) && node->property.vproperty[ch_id].buf_clean == 1) ||
                    (node->property.vproperty[ch_id].scl_feature == (1 << 5) && node->property.vproperty[ch_id].buf_clean == 1))
                    ret = fscaler300_init_job(job, node, ch_id, j);
                if ((node->property.vproperty[ch_id].scl_feature & (1 << 0)) || (node->property.vproperty[ch_id].scl_feature & (1 << 1)) ||
                    (node->property.vproperty[ch_id].scl_feature & (1 << 2))) {
                    if (node->property.vproperty[ch_id].aspect_ratio.enable == 0 && node->property.vproperty[ch_id].border.enable == 0)
                        ret = fscaler300_init_job(job, node, ch_id, j);
                    if (node->property.vproperty[ch_id].aspect_ratio.enable == 0 && node->property.vproperty[ch_id].border.enable == 1)
                        ret = fscaler300_init_border_job(job, node, ch_id, j);
                }
                if (node->property.vproperty[ch_id].scl_feature & (1 << 0)) {
                    if (node->property.vproperty[ch_id].aspect_ratio.enable == 1 && node->property.vproperty[ch_id].border.enable == 0)
                        ret = fscaler300_init_aspect_ratio_job(job, node, ch_id, j);
                    if (node->property.vproperty[ch_id].aspect_ratio.enable == 1 && node->property.vproperty[ch_id].border.enable == 1)
                        ret = fscaler300_init_aspect_ratio_border_job(job, node, ch_id, j);
                }
                if (ret < 0) {
                    printk("[%s] init job1 fail\n", __func__);
                    return -1;
                }

                /* set last job need_callback = 1 */
                if ((i == total_jobs - 1) && (j == vjob_cnt - 1))
                     job->need_callback = 1;
                else
                    job->need_callback = 0;

                job->parent = parent;

                /* for job preformance used */
                if (i == 0 && j == 0) {                     // sub parent job
                    sub_parent = job;
                    job->perf_type = TYPE_MIDDLE;
                    list_add_tail(&job->plist, &chain_list);
                }
                else if ((i == total_jobs - 1) && (j == chain_job_cnt - 1)) {  // last job, child job
                    if (chain_list_cnt > 1)             // more than 1 chain list, last job in this chain list are TYPE_MIDDLE
                        job->perf_type = TYPE_MIDDLE;
                    else
                        job->perf_type = TYPE_LAST;
                    list_add_tail(&job->clist, &sub_parent->clist);
                }
                else {                                      // middle job, child job
                    job->perf_type = TYPE_MIDDLE;
                    list_add_tail(&job->clist, &sub_parent->clist);
                }
                /* add ref_cnt to parent job */
                REFCNT_INC(parent);
            }
        }
        /* set sub parent job job_count */
        sub_parent->param.lli.job_count = sub_parent->job_cnt = chain_job_cnt;
        /* set child job job_count */
        list_for_each_entry_safe(child, child1, &sub_parent->clist, clist)
            child->param.lli.job_count = child->job_cnt = chain_job_cnt;
        /* set ch_bitmask to sub parent job */
        sub_parent->ch_bitmask.mask[chmask_base] |= (0x1 << chmask_idx);
    }

    if (pip_flag == 1) {
        if (tmp_chain_list_cnt == 1)
            tmp_parent = parent;
        if (tmp_chain_list_cnt > 1)
            tmp_parent = sub_parent;

        tmp_chain_job_cnt = tmp_parent->job_cnt;
        fscaler300_set_pip_job0(node, parent, tmp_parent, tmp_chain_list_cnt);
    }

    /*chmask_base = BITMASK_BASE(node->idx);
    chmask_idx = BITMASK_IDX(node->idx);
    parent->ch_bitmask.mask[chmask_base] |= (0x1 << chmask_idx);*/
    if ((node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) ||
        (node->property.src_fmt == TYPE_YUV422_FRAME_2FRAMES && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) ||
        (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)) {
        if (zoom_flag == 1) {
            if (tmp_chain_list_cnt == 1)
                tmp_parent = parent;
            if (tmp_chain_list_cnt > 1)
                tmp_parent = sub_parent;

            tmp_chain_job_cnt = tmp_parent->job_cnt;
            fscaler300_set_cvbs_zoom_job0(node, parent, tmp_parent, tmp_chain_list_cnt);
        }
    }
    /* add new job to engine */
    if (node->priority == 0){
        fscaler300_chainlist_add(&chain_list);
        /* add to node list */
        vg_nodelist_add(node);
    }
    else {
        if (tmp_chain_list_cnt > 1)
            fscaler300_pchainlist_add(&chain_list);
        else
            fscaler300_prioritylist_add(parent);
    }

    /* schedule job to hardware engine */
    fscaler300_put_job();

    return 0;
}

/*
 * set different source address mutliple node's child node job parameter
 */
int fscaler300_set_child_job(job_node_t *node, fscaler300_job_t *parent, fscaler300_job_t *sub_parent, int last_job)
{
    int i, ret = 0;
    int job_cnt = 0;
    int ch_id = node->ch_id;

    /* set node job parameter */
    /* get job_cnt of node */
    job_cnt = node->node_info.job_cnt;
    if (job_cnt <= 0) {
        printk("[%s] job_cnt < 0\n", __func__);
        return -1;
    }

    for (i = 0; i < job_cnt; i++) {
        fscaler300_job_t    *job = NULL;

        if (in_interrupt() || in_atomic())
            job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
        else
            job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
        if (unlikely(!job))
            panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
        memset(job, 0x0, sizeof(fscaler300_job_t));

        job->job_type = MULTI_DJOB;

        if (node->property.vproperty[ch_id].scl_feature & (1 << 2))
            ret = fscaler300_init_cvbs_zoom_job(job, node, i);
        else {
            if (node->property.vproperty[ch_id].aspect_ratio.enable == 0 && node->property.vproperty[ch_id].border.enable == 0)
                ret = fscaler300_init_job(job, node, ch_id, i);
            if (node->property.vproperty[ch_id].aspect_ratio.enable == 0 && node->property.vproperty[ch_id].border.enable == 1)
                ret = fscaler300_init_border_job(job, node, ch_id, i);
            if (node->property.vproperty[ch_id].aspect_ratio.enable == 1 && node->property.vproperty[ch_id].border.enable == 0)
                ret = fscaler300_init_aspect_ratio_job(job, node, ch_id, i);
            if (node->property.vproperty[ch_id].aspect_ratio.enable == 1 && node->property.vproperty[ch_id].border.enable == 1)
                ret = fscaler300_init_aspect_ratio_border_job(job, node, ch_id, i);
        }
        if (ret < 0)
            return -1;

        /* for performance used */
        if (job_cnt == 1)
            job->perf_type = TYPE_ALL;
        else {
            if (i == 0)                        // parent job
                job->perf_type = TYPE_FIRST;
            if ((i > 0) && (i < job_cnt - 1))
                job->perf_type = TYPE_MIDDLE;
            if (i == job_cnt - 1)
                job->perf_type = TYPE_LAST;
        }
        job->parent = parent;
        list_add_tail(&job->clist, &sub_parent->clist);
        /* set last job need_callback = 1 */
        if (last_job == 1 && i == job_cnt - 1)
            job->need_callback = 1;
        else
            job->need_callback = 0;

        /* add ref_cnt to parent job */
        REFCNT_INC(parent);
    }

    return 0;
}

int fscaler300_set_node_chain(job_node_t *node, struct list_head *node_chain_list, int node_cnt)
{
    int i, k, handled, ret = 0;
    job_node_t *curr, *next = NULL, *ne, *subparent;
    //job_node_t *t0, *t1;
    int job_cnt = 0, blk_cnt = 0;

    // get parent node job count & block count
    fscaler300_get_mask_osd_blk_cnt(node);
    ret = fscaler300_get_snode_jobcnt(node);
    if (ret < 0)
        return -1;
    // get child node blk count
    list_for_each_entry_safe(curr, ne, &node->clist, clist) {
        fscaler300_get_mask_osd_blk_cnt(curr);
        ret = fscaler300_get_snode_jobcnt(curr);
        if (ret < 0)
            return -1;
    }

    list_add_tail(&node->vg_plist, node_chain_list);
    handled = 1;
    job_cnt = node->node_info.job_cnt;
    blk_cnt = node->node_info.blk_cnt;

    curr = node;
    for (i = 1; i < node_cnt; i++) {    // i = 1 --> first node has handled
        next = list_entry(curr->clist.next, job_node_t, clist);
        job_cnt += next->node_info.job_cnt;
        blk_cnt += next->node_info.blk_cnt;

        if (job_cnt <= MAX_CHAIN_JOB && blk_cnt <= MAX_CHAIN_LL_BLK) {
            list_add_tail(&next->vg_clist, &node->vg_clist);
            curr = next;
            handled++;
        }
        else
            break;
    }

    while (handled < node_cnt) { // handled --> record how many node has handled
        subparent = next;
        list_add_tail(&subparent->vg_plist, node_chain_list);
        job_cnt = subparent->node_info.job_cnt;
        blk_cnt = subparent->node_info.blk_cnt;
        curr = subparent;
        handled++;    // subparent node has handled
        for (k = handled; k < node_cnt; k++) {
            next = list_entry(curr->clist.next, job_node_t, clist);
            job_cnt += next->node_info.job_cnt;
            blk_cnt += next->node_info.blk_cnt;

            if (job_cnt <= MAX_CHAIN_JOB && blk_cnt <= MAX_CHAIN_LL_BLK) {
                list_add_tail(&next->vg_clist, &subparent->vg_clist);
                curr = next;
                handled++;
            }
            else
                break;
        }
    }

    /*list_for_each_entry_safe(curr, ne, node_chain_list, vg_plist) {
        printk("id %d\n", curr->job_id);
        list_for_each_entry_safe(t0, t1, &curr->vg_clist, vg_clist)
            printk("child id %d\n", t0->job_id);
    }*/
    return 0;
}

#if GM8210
    #define SD_PATH     4
#else
    #define SD_PATH     2
#endif
/*
 *  multiple node with same source address
 */
int fscaler300_put_same_mnode(job_node_t *node, int node_cnt)
{
    int i, j, last_job, ret = 0;
    job_node_t *curr, *ne;
    job_node_t *mnode[4] = {NULL};
    job_node_t *next = NULL;
    fscaler300_job_t *parent = NULL;
    fscaler300_job_t *job = NULL;
    ch_bitmask_t ch_bitmask;
    int total_width = 0;
    int chain_list_cnt = 0;
    int chain_node = 0;
    struct list_head chain_list;
    int chain_job_cnt = 0;
    fscaler300_job_t *child, *child1;

    INIT_LIST_HEAD(&chain_list);
    memset(&ch_bitmask, 0x0, sizeof(ch_bitmask_t));
    if (flow_check)printm(MODULE_NAME, "%s id %u cnt %d\n", __func__, node->job_id, node_cnt);

    /* need to do !!!!!!!! */
    /* if multi job with same source address, 4 channel merge into 1 job, mask setting maybe wrong!!!
       because each channel all have mask, output will show each channel mask!!! */

    /* check parent node parameter */
    if (sanity_check(node) < 0) {
        printk("[%s] sanity_check fail\n", __func__);
        return -1;
    }
    /* check child node parameter */
    list_for_each_entry_safe(curr, ne, &node->clist, clist)
        if (sanity_check(curr) < 0) {
            printk("[%s] sanity_check fail\n", __func__);
            return -1;
        }

    /* check how many chain list we need */
    chain_list_cnt = node_cnt / SCALER300_MAX_PATH_PER_CHAIN;
    if (node_cnt % SCALER300_MAX_PATH_PER_CHAIN)
        chain_list_cnt++;

    /* get V.G multiple node ch_bit_mask */
    ret = fscaler300_set_ch_bitmask(node, &ch_bitmask);
    if (ret < 0) {
        printk("[%s] fscaler300_set_ch_bitmask fail\n", __func__);
        return -1;
    }

    /* calculate sum of src_width can not over 4096 */
    total_width += node->property.vproperty[node->ch_id].dst_width;
    curr = mnode[0] = node;
    chain_node++;
    /* get next 3 nodes to merge into 1 job, limitation is sum of destination width can not over 4096 */
    for (i = 0; i < (SD_PATH - 1); i++) {
        next = list_entry(curr->clist.next, job_node_t, clist);
        if (list_is_last(&next->clist, &node->clist))
            last_job = 1;
        else
            last_job = 0;

        total_width += next->property.vproperty[next->ch_id].dst_width;
        if (total_width > SCALER300_TOTAL_RESOLUTION) {
            if (i == 0)
                mnode[1] = NULL;
            else if (i == 1)
                mnode[2] = NULL;
            else
                mnode[3] = NULL;
            break;
        }
        else {
            if (i == 0)
                mnode[1] = next;
            else if (i == 1)
                mnode[2] = next;
            else
                mnode[3] = next;
            chain_node++;
            curr = next;
        }

        if (last_job == 1)
            break;
    }

    if (in_interrupt() || in_atomic())
        job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
    else
        job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
    if (unlikely(!job))
       panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
    memset(job, 0x0, sizeof(fscaler300_job_t));

    ret = fscaler300_init_mjob(job, mnode, last_job);
    if (ret < 0) {
        printk("[%s] fscaler300_init_mjob fail\n", __func__);
        return -1;
    }

    parent = job;
    job->parent = parent;
    /* set ch_bitmask to parent job */
    memcpy(&parent->ch_bitmask, &ch_bitmask, sizeof(ch_bitmask_t));
    /* add ref_cnt to parent job */
    REFCNT_INC(parent);
    chain_job_cnt++;

    //if (chain_node == SCALER300_MAX_PATH_PER_CHAIN) {
    if (chain_node == node_cnt) {
        /* set parent job job_count */
        parent->param.lli.job_count = parent->job_cnt = chain_job_cnt;
    }
    else {
        /* set child node job parameter */
        while (chain_node < SCALER300_MAX_PATH_PER_CHAIN) {
            fscaler300_job_t *job = NULL;
            job_node_t *mnode[4] = {NULL};
            /* get next 4 nodes to merge into 1 job, limitation is sum of destination width can not over 4096 */
            total_width = 0;
            for (i = 0; i < SD_PATH; i++) {
                next = list_entry(curr->clist.next, job_node_t, clist);
                if (list_is_last(&next->clist, &node->clist))
                    last_job = 1;
                else
                    last_job = 0;

                total_width += next->property.vproperty[next->ch_id].dst_width;
                if (total_width > SCALER300_TOTAL_RESOLUTION) {
                    if (i == 0)
                        mnode[0] = NULL;
                    else if (i == 1)
                        mnode[1] = NULL;
                    else if (i == 2)
                        mnode[2] = NULL;
                    else
                        mnode[3] = NULL;
                    break;
                }
                else {
                    if (i == 0)
                        mnode[0] = next;
                    else if (i == 1)
                        mnode[1] = next;
                    else if (i == 2)
                        mnode[2] = next;
                    else
                        mnode[3] = next;

                    curr = next;
                    chain_node++;
                    /* this chain's last node */
                    if (chain_node == SCALER300_MAX_PATH_PER_CHAIN)
                        last_job = 1;
                }
                if (last_job == 1)
                    break;
            }

            if (in_interrupt() || in_atomic())
                job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
            else
                job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
            if (unlikely(!job))
                panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
            memset(job, 0x0, sizeof(fscaler300_job_t));

            ret = fscaler300_init_mjob(job, mnode, last_job);
            if (ret < 0) {
                printk("[%s] fscaler300_init_mjob fail\n", __func__);
                return -1;
            }

            job->parent = parent;
            list_add_tail(&job->clist, &parent->clist);

            /* add ref_cnt to parent job */
            REFCNT_INC(parent);
            chain_job_cnt++;

            if (last_job == 1)
                break;
        }

        /* set parent job job_count */
        parent->param.lli.job_count = parent->job_cnt = chain_job_cnt;
        /* set child job job_count */
        list_for_each_entry_safe(child, child1, &parent->clist, clist)
            child->param.lli.job_count = child->job_cnt = chain_job_cnt;

    }
    list_add_tail(&parent->plist, &chain_list);

    chain_list_cnt--;
    /* set multiple chain list */
    for (i = 0; i < chain_list_cnt; i++) {
        fscaler300_job_t *sub_parent = NULL;
        chain_node = chain_job_cnt = 0;

        while (chain_node < SCALER300_MAX_PATH_PER_CHAIN) {
            fscaler300_job_t *job = NULL;
            job_node_t *mnode[4] = {NULL};
            /* get next 4 nodes to merge into 1 job, limitation is sum of destination width can not over 4096 */
            total_width = 0;
            for (j = 0; j < SD_PATH; j++) {
                next = list_entry(curr->clist.next, job_node_t, clist);
                if (list_is_last(&next->clist, &node->clist))
                    last_job = 1;
                else
                    last_job = 0;

                total_width += next->property.vproperty[next->ch_id].dst_width;
                if (total_width > SCALER300_TOTAL_RESOLUTION) {
                    if (j == 0)
                        mnode[0] = NULL;
                    else if (j == 1)
                        mnode[1] = NULL;
                    else if (j == 2)
                        mnode[2] = NULL;
                    else
                        mnode[3] = NULL;
                    break;
                }
                else {
                    if (j == 0)
                        mnode[0] = next;
                    else if (j == 1)
                        mnode[1] = next;
                    else if (j == 2)
                        mnode[2] = next;
                    else
                        mnode[3] = next;

                    curr = next;
                    chain_node++;
                    /* this chain's last node */
                    if (chain_node == SCALER300_MAX_PATH_PER_CHAIN)
                        last_job = 1;
                }
                if (last_job == 1)
                    break;
            }

            if (in_interrupt() || in_atomic())
                job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
            else
                job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
            if (unlikely(!job))
                panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
            memset(job, 0x0, sizeof(fscaler300_job_t));

            ret = fscaler300_init_mjob(job, mnode, last_job);
            if (ret < 0) {
                printk("[%s] fscaler300_init_mjob fail\n", __func__);
                return -1;
            }

            job->parent = parent;

            if (sub_parent == NULL)
                sub_parent = job;
            else
                list_add_tail(&job->clist, &sub_parent->clist);

            /* add ref_cnt to parent job */
            REFCNT_INC(parent);
            chain_job_cnt++;

            if (last_job == 1)
                break;
        }
        /* set parent job job_count */
        sub_parent->param.lli.job_count = sub_parent->job_cnt = chain_job_cnt;
        /* set child job job_count */
        list_for_each_entry_safe(child, child1, &sub_parent->clist, clist)
            child->param.lli.job_count = child->job_cnt = chain_job_cnt;

        /* set ch_bitmask to sub parent job */
        memcpy(&sub_parent->ch_bitmask, &ch_bitmask, sizeof(ch_bitmask_t));
        list_add_tail(&sub_parent->plist, &chain_list);
    }

    /* add new job to engine */
    if (node->priority == 0)
        fscaler300_chainlist_add(&chain_list);
    else
        fscaler300_pchainlist_add(&chain_list);

    /* add to node list */
    if (node->priority == 0)
        vg_nodelist_add(node);

    /* schedule job to hardware engine */
    fscaler300_put_job();

    return 0;
}

/*
 *  multiple node with different source address
 */
int fscaler300_put_diff_mnode(job_node_t *node, int node_cnt)
{
    int i, j, last_job, ret = 0;
    int chain_job_cnt = 0, pjob_cnt = 0, cjob_cnt = 0;
    job_node_t *curr, *ne, *sub_pnode;
    fscaler300_job_t *parent = NULL;
    fscaler300_job_t *sub_parent = NULL;
    fscaler300_job_t *child, *child1;
    int ch_id = node->ch_id;
    int chain_list_cnt = 0;
    ch_bitmask_t ch_bitmask;
    struct list_head node_list;
    struct list_head chain_list;

    INIT_LIST_HEAD(&node_list);
    INIT_LIST_HEAD(&chain_list);
    memset(&ch_bitmask, 0x0, sizeof(ch_bitmask_t));

    if (flow_check)printm(MODULE_NAME, "%s id %u cnt %d\n", __func__, node->job_id, node_cnt);
    /* check parent node parameter */
    if (sanity_check(node) < 0) {
        printk("[%s] sanity_check fail\n", __func__);
        return -1;
    }
    /* check child node parameter */
    list_for_each_entry_safe(curr, ne, &node->clist, clist)
        if (sanity_check(curr) < 0) {
            printk("[%s] sanity_check fail\n", __func__);
            return -1;
        }

    ret = fscaler300_set_node_chain(node, &node_list, node_cnt);
    if (ret < 0) {
        printk("[%s] fscaler300_set_node_chain fail\n", __func__);
        return -1;
    }

    /* get V.G multiple node ch_bit_mask */
    ret = fscaler300_set_ch_bitmask(node, &ch_bitmask);
    if (ret < 0) {
        printk("[%s] fscaler300_set_ch_bitmask fail\n", __func__);
        return -1;
    }

    list_for_each_entry_safe(curr, ne, &node_list, vg_plist)
        chain_list_cnt++;

    /* get job_cnt of parent node */
    pjob_cnt = node->node_info.job_cnt;
    if (pjob_cnt <= 0) {
        printk("[%s] pjob_cnt < 0\n", __func__);
        return -1;
    }

    for (i = 0; i < pjob_cnt; i++) {
        fscaler300_job_t    *job = NULL;

        if (in_interrupt() || in_atomic())
            job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
        else
            job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
        if (unlikely(!job))
            panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
        memset(job, 0x0, sizeof(fscaler300_job_t));

        job->job_type = MULTI_DJOB;

        if (node->property.vproperty[ch_id].scl_feature & (1 << 2))
            ret = fscaler300_init_cvbs_zoom_job(job, node, i);
        else {
            if (node->property.vproperty[ch_id].aspect_ratio.enable == 0 && node->property.vproperty[ch_id].border.enable == 0)
                ret = fscaler300_init_job(job, node, ch_id, i);
            if (node->property.vproperty[ch_id].aspect_ratio.enable == 0 && node->property.vproperty[ch_id].border.enable == 1)
                ret = fscaler300_init_border_job(job, node, ch_id, i);
            if (node->property.vproperty[ch_id].aspect_ratio.enable == 1 && node->property.vproperty[ch_id].border.enable == 0)
                ret = fscaler300_init_aspect_ratio_job(job, node, ch_id, i);
            if (node->property.vproperty[ch_id].aspect_ratio.enable == 1 && node->property.vproperty[ch_id].border.enable == 1)
                ret = fscaler300_init_aspect_ratio_border_job(job, node, ch_id, i);
        }
        if (ret < 0) {
            printk("[%s] fscaler300_init_job fail\n", __func__);
            return -1;
        }

        /* for job performance used */
        if (pjob_cnt == 1) {                    // parent job
            job->parent = job;
            parent = job;
            job->perf_type = TYPE_ALL;
        }
        if (pjob_cnt > 1) {
            /* parent job */
            if (i == 0) {
                job->parent = job;
                parent = job;
                job->perf_type = TYPE_FIRST;
            }
            /* child job */
            if ((i > 0) && (i < pjob_cnt - 1)) {
                job->parent = parent;
                job->perf_type = TYPE_MIDDLE;
                list_add_tail(&job->clist, &parent->clist);
            }
            if (i == pjob_cnt - 1) {
                job->parent = parent;
                job->perf_type = TYPE_LAST;
                list_add_tail(&job->clist, &parent->clist);
            }
        }
        /* add ref_cnt to parent job */
        REFCNT_INC(parent);
    }
    chain_job_cnt += pjob_cnt;
    /* set ch_bitmask to parent job */
    memcpy(&parent->ch_bitmask, &ch_bitmask, sizeof(ch_bitmask_t));
    // if this chain only 1 node, have to set this job's job cnt
    if (list_is_last(&node->vg_clist, &node->vg_clist))
        parent->param.lli.job_count = parent->job_cnt = chain_job_cnt;

    /* set child node parameter */
    list_for_each_entry_safe(curr, ne, &node->vg_clist, vg_clist) {
        if (list_is_last(&curr->clist, &node->clist))
            last_job = 1;
        else
            last_job = 0;

        cjob_cnt = curr->node_info.job_cnt;
        if (cjob_cnt <= 0) {
            printk("[%s] cjob cnt < 0\n", __func__);
            return -1;
        }

        ret = fscaler300_set_child_job(curr, parent, parent, last_job);
        if (ret < 0) {
            printk("[%s] fscaler300_set_child_job fail\n", __func__);
            return -1;
        }

        chain_job_cnt += cjob_cnt;
        /* last job at this node chain, set each job's job_cnt = chain_job_cnt */
        if (list_is_last(&curr->vg_clist, &node->vg_clist)) {
            /* set parent job job_count */
            parent->param.lli.job_count = parent->job_cnt = chain_job_cnt;
            /* set child job job_count */
            list_for_each_entry_safe(child, child1, &parent->clist, clist)
                child->param.lli.job_count = child->job_cnt = chain_job_cnt;
            break;
        }
    }
    list_add_tail(&parent->plist, &chain_list);
    list_del(&node->vg_plist);
    chain_list_cnt--;
/*************************** set sub node_list chain job *********************************/
    for (i = 0; i < chain_list_cnt; i++) {
        chain_job_cnt = 0;
        /* set sub parent node job parameter */
        sub_pnode = list_first_entry(&node_list, job_node_t, vg_plist);

        /* check this node is last node or not */
        if (list_is_last(&sub_pnode->clist, &node->clist))
            last_job = 1;
        else
            last_job = 0;

        ch_id = sub_pnode->ch_id;
        /* get job_cnt of sub parent node */
        pjob_cnt = sub_pnode->node_info.job_cnt;
        if (pjob_cnt <= 0) {
            printk("[%s] pjob_cnt < 0\n", __func__);
            return -1;
        }

        for (j = 0; j < pjob_cnt; j++) {
            fscaler300_job_t    *job = NULL;
            if (in_interrupt() || in_atomic())
                job = kmem_cache_alloc(priv.job_cache, GFP_ATOMIC);
            else
                job = kmem_cache_alloc(priv.job_cache, GFP_KERNEL);
            if (unlikely(!job))
                panic("%s, No memory for job! \n", __func__);
MEMCNT_ADD(&priv, 1);
            memset(job, 0x0, sizeof(fscaler300_job_t));

            job->job_type = MULTI_DJOB;

            if (sub_pnode->property.vproperty[ch_id].scl_feature & (1 << 2))
                ret = fscaler300_init_cvbs_zoom_job(job, sub_pnode, j);
            else {
                if (sub_pnode->property.vproperty[ch_id].aspect_ratio.enable == 0 && sub_pnode->property.vproperty[ch_id].border.enable == 0)
                    ret = fscaler300_init_job(job, sub_pnode, ch_id, j);
                if (sub_pnode->property.vproperty[ch_id].aspect_ratio.enable == 0 && sub_pnode->property.vproperty[ch_id].border.enable == 1)
                    ret = fscaler300_init_border_job(job, sub_pnode, ch_id, j);
                if (sub_pnode->property.vproperty[ch_id].aspect_ratio.enable == 1 && sub_pnode->property.vproperty[ch_id].border.enable == 0)
                    ret = fscaler300_init_aspect_ratio_job(job, sub_pnode, ch_id, j);
                if (sub_pnode->property.vproperty[ch_id].aspect_ratio.enable == 1 && sub_pnode->property.vproperty[ch_id].border.enable == 1)
                    ret = fscaler300_init_aspect_ratio_border_job(job, sub_pnode, ch_id, j);
            }
            if (ret < 0) {
                printk("[%s] fscaler300_init_job fail\n", __func__);
                return -1;
            }

            job->parent = parent;
            /* for performance used */
            if (pjob_cnt == 1) {                    // sub parent job
                sub_parent = job;
                job->perf_type = TYPE_ALL;
                list_add_tail(&sub_parent->plist, &chain_list);
                /* set ch_bitmask to sub parent job */
                memcpy(&job->ch_bitmask, &ch_bitmask, sizeof(ch_bitmask_t));
            }
            else {
                if (j == 0) {                       // sub parent job
                    sub_parent = job;
                    job->perf_type = TYPE_FIRST;
                    list_add_tail(&sub_parent->plist, &chain_list);
                    /* set ch_bitmask to sub parent job */
                    memcpy(&job->ch_bitmask, &ch_bitmask, sizeof(ch_bitmask_t));
                }
                if ((j > 0) && (j < pjob_cnt - 1)) {
                    job->perf_type = TYPE_MIDDLE;
                    list_add_tail(&job->clist, &sub_parent->clist);
                }
                if (j == pjob_cnt - 1) {                              // child job
                    job->perf_type = TYPE_LAST;
                    list_add_tail(&job->clist, &sub_parent->clist);
                }
            }
            if ((last_job == 1) && (j == pjob_cnt - 1))
                job->need_callback = 1;
            /* add ref_cnt to parent job */
            REFCNT_INC(parent);
        }
        chain_job_cnt += pjob_cnt;

        // if this chain only 1 node, have to set this job's job cnt
        if (last_job == 1 || list_is_last(&sub_pnode->vg_clist, &sub_pnode->vg_clist)) {
            /* set sub parent job job_count */
            sub_parent->param.lli.job_count = sub_parent->job_cnt = chain_job_cnt;
            /* set sub parent's child job job_count */
            list_for_each_entry_safe(child, child1, &sub_parent->clist, clist)
                child->param.lli.job_count = child->job_cnt = chain_job_cnt;
        }

        /* set sub parent's child node job parameter */
        list_for_each_entry_safe(curr, ne, &sub_pnode->vg_clist, vg_clist) {
            if (list_is_last(&curr->clist, &node->clist))
                last_job = 1;
            else
                last_job = 0;

            cjob_cnt = curr->node_info.job_cnt;
            if (cjob_cnt <= 0) {
                printk("[%s] cjob cnt < 0\n", __func__);
                return -1;
            }

            ret = fscaler300_set_child_job(curr, parent, sub_parent, last_job);
            if (ret < 0) {
                printk("[%s] fscaler300_set_child_job fail\n", __func__);
                return -1;
            }

            chain_job_cnt += cjob_cnt;

            /* last job , set each job's job_cnt = chain_job */
            if (list_is_last(&curr->vg_clist, &sub_pnode->vg_clist)) {
                /* set sub parent job job_count */
                sub_parent->param.lli.job_count = sub_parent->job_cnt = chain_job_cnt;
                /* set child job job_count */
                list_for_each_entry_safe(child, child1, &sub_parent->clist, clist)
                    child->param.lli.job_count = child->job_cnt = chain_job_cnt;
                break;
            }
        }
        list_del(&sub_pnode->vg_plist);
    }

    /* add new job to engine */
    if (node->priority == 0)
        fscaler300_chainlist_add(&chain_list);
    else
        fscaler300_pchainlist_add(&chain_list);

    /* add to node list */
    if (node->priority == 0)
        vg_nodelist_add(node);

    /* schedule job to hardware engine */
    fscaler300_put_job();

    return 0;
}

/*
 * Put single node to fscaler300 module core
 */
int vg_put_single_node(job_node_t *node)
{
    int ret = 0;
    int ch_id = node->ch_id;
    fscaler300_property_t *param = &node->property;

    if (node->ch_cnt > 1)
        node->type = VIRTUAL_NODE;
    if (flow_check) printm(MODULE_NAME, "%s id %u\n", __func__, node->job_id);
    /* check parameter */
    if (sanity_check(node) < 0) {
        printk("[%s] sanity_check fail\n", __func__);
        return -1;
    }

/*
 * For the following non-supported transformation, pretend the source format as TYPE_YUV422_FRAME
 *  which can do frame-to-frame conversion.
 */
    if (param->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && param->dst_fmt == TYPE_YUV422_FRAME) {
        param->src_fmt = TYPE_YUV422_FRAME;
    } else if (param->src_fmt == TYPE_YUV422_FRAME_2FRAMES && param->dst_fmt == TYPE_YUV422_FRAME) {
        param->src_fmt = TYPE_YUV422_FRAME;
    } else if (param->src_fmt == TYPE_YUV422_FRAME_FRAME && param->dst_fmt == TYPE_YUV422_FRAME) {
        param->src_fmt = TYPE_YUV422_FRAME;
    }

    if (node->type == SINGLE_NODE) {          // SINGLE_NODE
        /* do cvbs zoom & pip, go this path */
        if ((node->property.vproperty[ch_id].scl_feature & (1 << 2)) || (node->property.vproperty[ch_id].scl_feature & (1 << 3))) {
            if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES || node->property.src_fmt == TYPE_YUV422_FRAME_2FRAMES ||
                node->property.src_fmt == TYPE_YUV422_FRAME_FRAME) {
                ret = fscaler300_put_virtual_node(node);
                if (ret < 0)
                    printk("[%s] fscaler300_put_virtual_node fail\n", __func__);
            }
            // dual core RC will issue src_fmt = 0 job to scaler, scaler only transfer pcie address and donothing
            /*if (node->property.src_fmt == 0 && node->property.vproperty[ch_id].scl_feature & (1 << 4)) {
                ret = fscaler300_put_virtual_node(node);
                if (ret < 0)
                    printk("[%s] fscaler300_put_virtual_node fail\n", __func__);
            }*/
        }
        else {
            if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_CASCADE_SCALING_FRAME) {
                ret = fscaler300_put_cascade_node(node);
                if (ret < 0)
                    printk("[%s] fscaler300_put_cascade_node fail\n", __func__);
            }
            else {
                ret = fscaler300_put_single_node(node);
                if (ret < 0)
                    printk("[%s] fscaler300_put_single_node fail\n", __func__);
            }
        }
    }
    else {                                    // VIRTUAL_NODE
        ret = fscaler300_put_virtual_node(node);
        if (ret < 0)
            printk("[%s] fscaler300_put_virtual_node fail\n", __func__);
    }

    return ret;
}

/*
 * Put multiple node to fscaler300 module core
 */
int vg_put_multi_node(job_node_t *node, int node_cnt)
{
    int ret = 0;
    job_node_t *next_node = NULL;
    job_node_t *curr, *ne;
    int flag = 0;

    if (flow_check) printm(MODULE_NAME, "%s id %u cnt %d\n", __func__, node->job_id, node_cnt);

    /* get next node */
    next_node = list_first_entry(&node->clist, job_node_t, clist);

    /* check source address is same or different */
    if (node->property.in_addr_pa == next_node->property.in_addr_pa) {
        flag = 1;
        list_for_each_entry_safe(curr, ne, &node->clist, clist) {
            if (curr->property.in_addr_pa == ne->property.in_addr_pa) {
                flag = 1;
                continue;
            }
            else {
                flag = 0;
                break;
            }
        }
        if (flag == 1) { // input source are all the same
            ret = fscaler300_put_same_mnode(node, node_cnt);
            if (ret < 0)
                printk("[%s] fscaler300_put_same_mnode < 0\n", __func__);
        }
        if (flag == 0) { // input source are not same
            ret = fscaler300_put_diff_mnode(node, node_cnt);
            if (ret < 0)
                printk("[%s] fscaler300_put_diff_mnode < 0\n", __func__);
        }
    }
    else {
        ret = fscaler300_put_diff_mnode(node, node_cnt);
        if (ret < 0)
            printk("[%s] fscaler300_put_diff_mnode < 0\n", __func__);
    }

    return ret;
}

int fscaler300_rc_io_tx(job_node_t *node)
{
#if GM8210
    u32 pcie_addr = 0;
    directio_info_t temp_info;
    snd_info_t  snd_info;
    int ret = 0;

    pcie_addr = fmem_get_pcie_addr(node->property.out_addr_pa);
    if (pcie_addr == 0xFFFFFFFF) {
        printk("[SL] transfer axi to pcie addr fail\n");
        return -1;
    }

    snd_info.serial_num = node->serial_num;
    snd_info.out_addr = pcie_addr;  /* valid */
    snd_info.vg_serial = node->property.vg_serial;  /* valid */
    snd_info.timer = node->timer = get_gm_jiffies();

    memcpy(&temp_info.data[0], &snd_info, sizeof(snd_info_t));

    if (dual_debug == 1) {
        printm(MODULE_NAME, "%s start node id %u sn %d version %d addr %x\n", __func__, node->job_id, snd_info.serial_num, snd_info.vg_serial, snd_info.out_addr);
    }

    ret = scaler_directio_snd(&temp_info, sizeof(snd_info_t));
    if (ret < 0)
        panic("RX direct io send fail id %u sn %d\n", node->job_id, node->serial_num);
#endif
    return 0;
}

int fscaler300_rc_io_rx(snd_info_t *rcv_info, int timeout)
{
#if GM8210
    directio_info_t temp_info;
    unsigned int expire_time;

    expire_time = get_gm_jiffies();    //ms
    while (scaler_directio_rcv(&temp_info, sizeof(snd_info_t)) < 0) {
        if ((int)get_gm_jiffies() - (int)expire_time > timeout)
            return -1;
        msleep(10);
    }

    memcpy(rcv_info, &temp_info.data[0], sizeof(snd_info_t));
#endif
    return 0;
}

int fscaler300_ep_io_tx(job_node_t *node)
{
#if GM8210
    snd_info_t snd_info;
    directio_info_t temp_info;
    int ret;
	int diff = 0;
    memset(&snd_info, 0, sizeof(snd_info_t));

    snd_info.serial_num = node->serial_num;
    snd_info.vg_serial = node->property.vg_serial;  /* valid */
    snd_info.status = node->status;
    snd_info.timer = get_gm_jiffies();

    memcpy(&temp_info.data[0], &snd_info, sizeof(snd_info_t));

    diff = (int)get_gm_jiffies() - (int)node->timer;
    if (rxtx_max < diff)
    	rxtx_max = diff;

    if (dual_debug == 1) {
        if (node->status == JOB_STS_FAIL)
            printm(MODULE_NAME, "%s start node id %u sn %d version %d addr %x fail\n", __func__, node->job_id, snd_info.serial_num, snd_info.vg_serial, snd_info.out_addr);
        else if (node->status == JOB_STS_TX)
            printm(MODULE_NAME, "%s start node id %u sn %d version %d addr %x done\n", __func__, node->job_id, snd_info.serial_num,  snd_info.vg_serial, snd_info.out_addr);
        else if (node->status == JOB_STS_TIMEOUT)
            printm(MODULE_NAME, "%s start node id %u sn %d version %d addr %x timeout\n", __func__, node->job_id, snd_info.serial_num,  snd_info.vg_serial, snd_info.out_addr);
        else
            printm(MODULE_NAME, "%s start node id %u sn %d version %d addr %x sts %x\n", __func__, node->job_id, snd_info.serial_num,  snd_info.vg_serial, snd_info.out_addr, node->status);

        printm(MODULE_NAME, "ep %u rx->tx diff %d\n", node->job_id, (int)get_gm_jiffies() - (int)node->timer);
        printm(MODULE_NAME, "EP TX sn %d time %d\n", snd_info.serial_num, snd_info.timer);
    }

    if ((ret = scaler_directio_snd(&temp_info, sizeof(snd_info_t))) < 0) {
        printk("%s node id %d sn %d ret %d\n", __func__, node->job_id, snd_info.serial_num, ret);
        printm(MODULE_NAME, "%s node id %d sn %d ret %d\n", __func__, node->job_id, snd_info.serial_num, ret);
        panic("EP direct io send fail id %u sn %d\n", node->job_id, node->serial_num);
        return -1;
    }
#endif
    return 0;
}
int fscaler300_ep_io_rx(int timeout)
{
#if GM8210
    job_node_t *rc_node;
    directio_info_t temp_info;
    snd_info_t rcv_info;
    unsigned int curr_jiffies, expire_time;

    expire_time = get_gm_jiffies();    //ms
    while (scaler_directio_rcv(&temp_info, sizeof(snd_info_t)) < 0) {
       if ((int)get_gm_jiffies() - (int)expire_time > timeout)
            return -1;
        if (dual_debug == 1) printm(MODULE_NAME, "sleep \n");
        msleep(10);
    }

    memcpy(&rcv_info, &temp_info.data[0], sizeof(snd_info_t));

    curr_jiffies = get_gm_jiffies();

    down(&global_info.sema_lock);
    if (flow_check) printm(MODULE_NAME, "%s sema count = %d\n", __func__, global_info.sema_lock.count);
    rc_node = kmem_cache_alloc(global_info.node_cache, GFP_KERNEL);

    INIT_LIST_HEAD(&rc_node->plist);
    list_add_tail(&rc_node->plist, &global_info.ep_rcnode_list);
    rc_node->status = JOB_STS_QUEUE;
    rc_node->serial_num = rcv_info.serial_num;
    rc_node->property.pcie_addr = rcv_info.out_addr;
    rc_node->property.vg_serial = rcv_info.vg_serial;
    rc_node_mem_cnt ++;
    rc_node->timer = curr_jiffies;
    fscaler300_set_node_timeout(rc_node);
    up(&global_info.sema_lock);

    if (dual_debug == 1) {
        printm(MODULE_NAME, "%s done node sn %d version %d addr %x time %d rc_cnt %d\n", __func__, rc_node->serial_num, rc_node->property.vg_serial,rcv_info.out_addr, rc_node->timer, rc_node_mem_cnt);
        printm(MODULE_NAME, "EP RX sn %d time %d\n", rc_node->serial_num, rc_node->timer);
    }
#endif
    return 0;
}


int vg_get_serial_num(job_node_t *node)
{
#if GM8210
    static int cpu_comm_sn = 0;

    if((++cpu_comm_sn) == 0)
        ++cpu_comm_sn;  // let cpu_comm_sn is not 0
    node->serial_num = cpu_comm_sn;
#endif
    return 0;
}

static void vg_cal_pip_addr(job_node_t *node)
{
    int i = 0;
    int frm1_addr = 0, frm2_addr = 0, frm3_addr = 0, frm4_addr = 0;
    int dfrm1_addr = 0, dfrm2_addr = 0, dfrm3_addr = 0, dfrm4_addr = 0;
    fscaler300_property_t *param = &node->property;

    if (param->src_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        frm1_addr = param->in_addr_pa;
        frm2_addr = param->in_addr_pa + (param->src_bg_width * param->src_bg_height << 1);
        frm3_addr = param->in_addr_pa + (param->src_bg_width * param->src_bg_height << 2);
        frm4_addr = param->in_addr_pa + (param->src_bg_width * param->src_bg_height << 2) +
                    (param->src2_bg_width * param->src2_bg_height << 1);
        dfrm1_addr = param->out_addr_pa;
        dfrm2_addr = param->out_addr_pa + (param->src_bg_width * param->src_bg_height << 1);
        dfrm3_addr = param->out_addr_pa + (param->src_bg_width * param->src_bg_height << 2);
        dfrm4_addr = param->out_addr_pa + (param->src_bg_width * param->src_bg_height << 2) +
                    (param->src2_bg_width * param->src2_bg_height << 1);
    }

    if (param->src_fmt == TYPE_YUV422_FRAME_FRAME) {
        frm1_addr = param->in_addr_pa;
        frm2_addr = 0;
        frm3_addr = param->in_addr_pa + (param->src_bg_width * param->src_bg_height << 1);
        frm4_addr = 0;

        dfrm1_addr = param->out_addr_pa;
        dfrm2_addr = 0;
        dfrm3_addr = param->out_addr_pa + (param->src_bg_width * param->src_bg_height << 1);
        dfrm4_addr = 0;
    }

    for (i = 0; i < param->pip.win_cnt; i++) {
        if (param->pip.src_frm[i] == 0x1)
            param->pip.src_dma[i].addr = frm1_addr;
        if (param->pip.src_frm[i] == 0x2)
            param->pip.src_dma[i].addr = frm2_addr;
        if (param->pip.src_frm[i] == 0x4)
            param->pip.src_dma[i].addr = frm3_addr;
        if (param->pip.src_frm[i] == 0x8)
            param->pip.src_dma[i].addr = frm4_addr;
        // buf1 & buf2 address will know after assign to scaler engineX
        if (param->pip.src_frm[i] == 0x11)
            param->pip.src_dma[i].addr = 0x11;
        if (param->pip.src_frm[i] == 0x12)
            param->pip.src_dma[i].addr = 0x12;

        if (param->pip.dst_frm[i] == 0x1)
            param->pip.dest_dma[i].addr = dfrm1_addr;
        if (param->pip.dst_frm[i] == 0x2)
            param->pip.dest_dma[i].addr = dfrm2_addr;
        if (param->pip.dst_frm[i] == 0x4)
            param->pip.dest_dma[i].addr = dfrm3_addr;
        if (param->pip.dst_frm[i] == 0x8)
            param->pip.dest_dma[i].addr = dfrm4_addr;
        // buf1 & buf2 address will know after assign to scaler engineX
        if (param->pip.dst_frm[i] == 0x11)
            param->pip.dest_dma[i].addr = 0x11;
        if (param->pip.dst_frm[i] == 0x12)
            param->pip.dest_dma[i].addr = 0x12;
    }
}

static void vg_cal_buf1_addr(job_node_t *node)
{
    /* calculate in_addr_pa_1 */
    /* 4 frame to 4 frame */
    if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        node->property.in_addr_pa_1 = node->property.in_addr_pa +
                                    (node->property.src_bg_width * node->property.src_bg_height << 2);
    }
    /* 1frame2frame to 1frame2frame */
    if (node->property.src_fmt == TYPE_YUV422_FRAME_2FRAMES && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) {
        node->property.in_addr_pa_1 = node->property.in_addr_pa +
                                    (node->property.src_bg_width * node->property.src_bg_height << 1);
    }
    /* frame to 4 frame */
    if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES)
        node->property.in_addr_pa_1 = node->property.in_addr_pa;
    /* frame to 1frame2frame --> clear win */
    if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES)
        node->property.in_addr_pa_1 = node->property.in_addr_pa;
    /* 1frame1frame to 1frame1frame */
    if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME)
        node->property.in_addr_pa_1 = node->property.in_addr_pa;

    /* calculate out_addr_pa[1] */
    /* ratio frame to 4 frame */
    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        node->property.out_addr_pa_1 = node->property.out_addr_pa +
                                        (node->property.dst_bg_width * node->property.dst_bg_height << 2);
    }
    /* frame to 4 frame */
    if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        node->property.out_addr_pa_1 = node->property.out_addr_pa +
                                        (node->property.dst_bg_width * node->property.dst_bg_height << 2);
    }
    /* 4 frame to 4 frame */
    if (node->property.src_fmt == TYPE_YUV422_2FRAMES_2FRAMES && node->property.dst_fmt == TYPE_YUV422_2FRAMES_2FRAMES) {
        node->property.out_addr_pa_1 = node->property.out_addr_pa +
                                        (node->property.dst_bg_width * node->property.dst_bg_height << 2) +
                                        (node->property.src2_bg_width * node->property.src2_bg_height << 1);
    }
    /* ratio to 1frame2frame */
    if (node->property.src_fmt == TYPE_YUV422_RATIO_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) {
        node->property.out_addr_pa_1 = node->property.out_addr_pa +
                                        (node->property.dst_bg_width * node->property.dst_bg_height << 1);
    }
    /* 1frame2frame to 1frame2frame */
    if (node->property.src_fmt == TYPE_YUV422_FRAME_2FRAMES && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) {
        node->property.out_addr_pa_1 = node->property.out_addr_pa +
                                        (node->property.dst_bg_width * node->property.dst_bg_height << 1) +
                                        (node->property.src2_bg_width * node->property.src2_bg_height << 1);
    }
    /* frame to 1frame2frame --> clear win */
    if (node->property.src_fmt == TYPE_YUV422_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_2FRAMES) {
        node->property.out_addr_pa_1 = node->property.out_addr_pa +
                                        (node->property.dst_bg_width * node->property.dst_bg_height << 1);
    }
    /* 1frame1frame to 1frame1frame */
    if (node->property.src_fmt == TYPE_YUV422_FRAME_FRAME && node->property.dst_fmt == TYPE_YUV422_FRAME_FRAME) {
        node->property.out_addr_pa_1 = node->property.out_addr_pa +
                                        (node->property.dst_bg_width * node->property.dst_bg_height << 1);
    }

    if (node->property.pip.win_cnt > 0)
        vg_cal_pip_addr(node);
}

#if GM8210 /* harry */

void fscaler300_set_node_timeout(job_node_t *node)
{
    node->expires = get_gm_jiffies();
}

int fscaler300_is_node_timeout(void)
{
    job_node_t  *node, *ne;
    int is_timeout = 0;
    unsigned long cur_jiffies = get_gm_jiffies();

    int rc_cnt = 0, ep_cnt = 0, i;

    /* compare rc_node list & ep_list count, if diff over rc_ep_diff_cnt,
       drop rc or ep list dummy job, to balance rc & ep job count */
    list_for_each_entry_safe(node, ne, &global_info.ep_rcnode_list, plist)
        rc_cnt++;
    list_for_each_entry_safe(node, ne, &global_info.ep_list, eplist)
        ep_cnt++;
    // if rc > ep
    if ((rc_cnt - ep_cnt) > rc_ep_diff_cnt) {
        for (i = 0; i < (rc_cnt - ep_cnt) - rc_ep_diff_cnt ; i++) {
            node = list_entry(global_info.ep_rcnode_list.next, job_node_t, plist);
            node->status = JOB_STS_FAIL;
            list_del(&node->plist);
            list_add_tail(&node->plist, &global_info.ep_rcnode_cb_list);
            rc_drop_cnt ++;
            is_timeout = 1;
        }
    }
    // if ep > rc
    if ((ep_cnt - rc_cnt) > rc_ep_diff_cnt) {
        for (i = 0; i < (ep_cnt - rc_cnt) - rc_ep_diff_cnt ; i++) {
            node = list_entry(global_info.ep_list.next, job_node_t, eplist);
            node->status = JOB_STS_FAIL;
            list_del(&node->eplist);
            vg_nodelist_add(node);
            ep_drop_cnt ++;
            is_timeout = 1;
        }
    }

    /* remote: rc_node_list timeout */
    list_for_each_entry_safe(node, ne, &global_info.ep_rcnode_list, plist) {
        unsigned int diff = (int)cur_jiffies - (int)node->expires;
        if (diff > ep_rx_timeout) {
            /* when videograph change layout, and timeout occur, return JOB_STS_TIMEOUT */
            if (node->property.vg_serial != ep_rx_vg_serial) {
                node->status = JOB_STS_TIMEOUT;
            } else {
                node->status = JOB_STS_FAIL;
            }
            if (dual_debug == 1)
                printm(MODULE_NAME, "rc_node_list timeout sn %d version %d sts %x\n",node->serial_num, node->property.vg_serial, node->status);
            list_del(&node->plist);
            list_add_tail(&node->plist, &global_info.ep_rcnode_cb_list);
            is_timeout |= 0x1;
            ep_pcie_addr_timeout_cnt ++;
        }
    }
    /* local: ep_list timeout */
    list_for_each_entry_safe(node, ne, &global_info.ep_list, eplist) {
        unsigned int diff = (int)cur_jiffies - (int)node->expires;
        if (diff > ep_rx_timeout) {
            node->status = JOB_STS_MARK_SUCCESS;
            if (dual_debug == 1)
                printm(MODULE_NAME, "ep_list timeout ep id %u version %d sts %x\n", node->job_id, node->property.vg_serial, node->status);
            list_del(&node->eplist);
            vg_nodelist_add(node);
            is_timeout |= 0x2;
            ep_job_timeout_cnt ++;
        }
    }

    return is_timeout;
}

int fscaler300_is_node_matched(job_node_t *rc_node, job_node_t *ep_node)
{
    int is_matched = 1, ret;

    /* compare the version */
    if (ep_node->property.vg_serial > rc_node->property.vg_serial) {
        /* if EP is newer, callback RC job fail, and keeps current job and waits next RC job
         */
        if (dual_debug == 1)
            printm(MODULE_NAME, "match fail !! ep id %u sn %d version %d > rc version %d, set rc node fail %x\n", ep_node->job_id, ep_node->serial_num, ep_node->property.vg_serial, rc_node->property.vg_serial, rc_node);
        rc_node->status = JOB_STS_FAIL;
        list_del(&rc_node->plist);
        /* move to callback list */
        list_add_tail(&rc_node->plist, &global_info.ep_rcnode_cb_list);
        is_matched = 0;
        goto ext;
    }

    /* compare the version */
    if (ep_node->property.vg_serial < rc_node->property.vg_serial) {
        /*
         * RC is newer and wait local job to match. Note: the rc timer has started already.
         */

        /* local callback fail */
        if (dual_debug == 1)
            printm(MODULE_NAME, "match fail !! ep id %u sn %d version %d < rc version %d, set ep node fail  %x\n", ep_node->job_id, ep_node->serial_num, ep_node->property.vg_serial, rc_node->property.vg_serial, rc_node);
        ep_node->status = JOB_STS_MARK_SUCCESS;
        list_del(&ep_node->eplist);
        vg_nodelist_add(ep_node);
        is_matched = 0;
        goto ext;
    }

    /*
     * Match ok. The version is consistent.
     */
    list_del(&rc_node->plist);
    /* move to callback list */
    if (dual_debug == 1)
        printm(MODULE_NAME, "match ok !! ep id %u sn %d version %d put job diff %d %x\n", ep_node->job_id, rc_node->serial_num, rc_node->property.vg_serial, (int)get_gm_jiffies() - (int)rc_node->timer, rc_node);
    /* if rc_node->status == JOB_STS_QUEUE, it will block the ep_tx command  */
    rc_node->status = JOB_STS_MATCH; /* NOTE: set rc_node status as JOB_STS_MATCH, when ep_node job done,
                                        chaneg rc_node status to JOB_STS_TX, and tx back to RC */
    list_add_tail(&rc_node->plist, &global_info.ep_rcnode_cb_list);
    /* update the pcie address */
    ep_node->property.pcie_addr = rc_node->property.pcie_addr;
    /* keep the rc node in order to update its status while the scaler job finished. */
    ep_node->rc_node = rc_node;
    /* record rc_node match ep_node's job id*/
    rc_node->job_id = ep_node->job_id;
    // delete node from ep_list
    list_del(&ep_node->eplist);
    if (flow_check) printm(MODULE_NAME, "%s put ep id %u\n", __func__, ep_node->job_id);
    // put node to scaler
    ret = vg_put_single_node(ep_node);
    if (ret < 0) {
        printk("[%s] vg_put_single_node fail\n", __func__);
        kmem_cache_free(global_info.vproperty_cache, ep_node->property.vproperty);
        kmem_cache_free(global_info.node_cache, ep_node);
        printm(MODULE_NAME, "PANIC!! put ep job error\n");
        damnit(MODULE_NAME);
    }
ext:
    return is_matched;
}

int fscaler300_ep_do_node_matched(void)
{
    /*
       ret = 0, match ok. --> EP_MATCH_OK
       ret = -1, ep_list is empty. --> EP_MATCH_EP_LIST_EMPTY
       ret = -2, rc_rcnode_list is empty. --> EP_MATCH_RC_RCNODE_LIST_EMPTY
       ret = -3, node not matched. --> EP_MATCH_FAILED
     */
    job_node_t *ep_node, *rc_node;
    int ret = EP_MATCH_OK, loop_cnt = 0, matched = 0;

    while (1) {
        if(list_empty(&global_info.ep_list)) {
            ret = EP_MATCH_EP_LIST_EMPTY;
            goto ext;
        }
        if(list_empty(&global_info.ep_rcnode_list)) {
            ret = EP_MATCH_RC_RCNODE_LIST_EMPTY;
            goto ext;
        }

        /* match rc_node and ep_node  */
        ep_node = list_entry(global_info.ep_list.next, job_node_t, eplist);
        rc_node = list_entry(global_info.ep_rcnode_list.next, job_node_t, plist);
        if (fscaler300_is_node_matched(rc_node, ep_node) == 0) {  // match fail
            ret = EP_MATCH_FAILED;
            goto ext;
        }

        /* err check to avoid infinite loop */
        if (++loop_cnt >= 100) {
            printm(MODULE_NAME, "%s: PANIC!! infinite loop!!\n", __func__);
            damnit(MODULE_NAME);
            goto ext;
        }
        matched = 1; /* matched flag, we need to chk it and return EP_MATCH_OK */
    }

ext:
    if (matched == 1)
        return EP_MATCH_OK;
    else
        return ret;
}

int fscaler300_rc_check_node_matched(snd_info_t *rcv_info, job_node_t *rc_node)
{
    int match_result;

    match_result = RC_MATCH_STATUS_DONE;

    /* match rc and ep, err checking */
    if (rcv_info->serial_num < rc_node->serial_num) {
        /* skip it */
        printk("%s [skip] rc_job_id(%u), ep_sn(%d) < rc_sn(%d).\n", __func__, rc_node->job_id, rcv_info->serial_num, rc_node->serial_num);
        printm(MODULE_NAME, "%s [skip] rc_job_id(%u), ep_sn(%d) < rc_sn(%d).\n", __func__, rc_node->job_id, rcv_info->serial_num, rc_node->serial_num);
        match_result = RC_MATCH_EP_SN_SMALLER;
    }
    if (rcv_info->serial_num > rc_node->serial_num) {
        /* error case, FIXME! */
        printk("%s [err] rc_job_id(%u), ep_sn(%d) > rc_sn(%d). FIXME!\n", __func__, rc_node->job_id, rcv_info->serial_num, rc_node->serial_num);
        printm(MODULE_NAME, "%s [err] rc_job_id(%u), ep_sn(%d) > rc_sn(%d). FIXME!\n", __func__, rc_node->job_id, rcv_info->serial_num, rc_node->serial_num);
        match_result = RC_MATCH_EP_SN_BIGGER;
    }
    /* compare the vg version */
    if (rcv_info->vg_serial < rc_node->property.vg_serial) {
        /* skip it */
        printk("%s [skip] rc_job_id(%u), ep_vg_serial(%d) < rc_vg_serial(%d).\n", __func__, rc_node->job_id, rcv_info->vg_serial, rc_node->property.vg_serial);
        printm(MODULE_NAME, "%s [skip] rc_job_id(%u), ep_vg_serial(%d) < rc_vg_serial(%d).\n", __func__, rc_node->job_id, rcv_info->vg_serial, rc_node->property.vg_serial);
        match_result = RC_MATCH_EP_VG_SN_SMALLER;
    }
    if (rcv_info->vg_serial > rc_node->property.vg_serial) {
        /* immediately callback fail */
        printk("%s [skip] rc_job_id(%u), ep_vg_serial(%d) > rc_vg_serial(%d).\n", __func__, rc_node->job_id, rcv_info->vg_serial, rc_node->property.vg_serial);
        printm(MODULE_NAME, "%s [skip] rc_job_id(%u), ep_vg_serial(%d) > rc_vg_serial(%d).\n", __func__, rc_node->job_id, rcv_info->vg_serial, rc_node->property.vg_serial);
        match_result = RC_MATCH_EP_VG_SN_BIGGER;
    }
    if (rcv_info->status == JOB_STS_FAIL) {
        /* immediately callback fail */
        match_result = RC_MATCH_STATUS_FAIL;
        rc_fail++;
    }
    /* when timeout, if rc node is invisible, don't scaling and return fail, else rc node still have to do scaling */
    if (rcv_info->status == JOB_STS_TIMEOUT) {
        /* immediately callback fail */
        if (I_AM_INVISIBLE(rc_node->property.vproperty[rc_node->ch_id].scl_feature)) {
            match_result = RC_MATCH_STATUS_FAIL;
            rc_fail++;
        }
    }

    return match_result;
}

void fscaler300_put_rc_node(job_node_t *rc_node)
{
    int ret;

    if (dual_debug == 1)
        printm(MODULE_NAME, "cpu_comm_rc_rx rc id %u match, put job\n", rc_node->job_id);

    /* triggle HW */
    list_del(&rc_node->rclist);
    if (flow_check) printm(MODULE_NAME, "%s rc id %u\n", __func__, rc_node->job_id);
    ret = vg_put_single_node(rc_node);
    if (ret < 0) {
        printk("[%s] vg_put_single_node fail\n", __func__);
        kmem_cache_free(global_info.vproperty_cache, rc_node->property.vproperty);
        kmem_cache_free(global_info.node_cache, rc_node);
        printm(MODULE_NAME, "PANIC!! put rc job error\n");
        damnit(MODULE_NAME);
    }
}

void fscaler300_do_rc_node_fail(job_node_t *rc_node)
{
    if (dual_debug == 1)
        printm(MODULE_NAME, "cpu_comm_rc_rx rc %u match fail\n", rc_node->job_id);

    /* do callback fail */
    rc_node->status = JOB_STS_FAIL;
    list_del(&rc_node->rclist);
#ifdef USE_KTHREAD
    callback_wake_up();
    callback_2ddma_wake_up();
#endif
#ifdef USE_WQ
        /* schedule the delay workq */
    if (dual_debug == 1) printm(MODULE_NAME, "waku up callback %d\n", __LINE__);
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)fscaler300_callback_process);
    queue_delayed_work(callback_workq, &process_callback_work, 0);

    PREPARE_DELAYED_WORK(&process_2ddma_callback_work,(void *)fscaler300_2ddma_callback_process);
    queue_delayed_work(callback_workq, &process_2ddma_callback_work, 0);
#endif
}

int fscaler300_rc_rx_process(void *parm)
{
    int match_result = RC_MATCH_STATUS_DONE, timeout = 50;
    job_node_t *rc_node;
    snd_info_t rcv_info;

    do {

        /* step_1.  receive the data from EP */
        if (fscaler300_rc_io_rx(&rcv_info, timeout) < 0)
            continue;
        if (dual_debug == 1)
            printm(MODULE_NAME, "RC RX sn %d time %d\n", rcv_info.serial_num, get_gm_jiffies());
        down(&global_info.sema_lock);
        if (flow_check) printm(MODULE_NAME, "%s sema count = %d\n", __func__, global_info.sema_lock.count);
        rc_tx_node_cnt--;
        /* step_2. get rc node and set match result */
        if(list_empty(&global_info.rc_list)) {
            printk("%s: [err] no rc_list. FIXME!\n", __func__);
            printm(MODULE_NAME, "%s: [err] no rc_list. FIXME!\n", __func__);
            match_result = RC_MATCH_LIST_EMPTY;
        } else {
            rc_node = list_entry(global_info.rc_list.next, job_node_t, rclist);
            if(dual_debug == 1)
                printm(MODULE_NAME, "rc id %u  tx->rx diff %d\n", rc_node->job_id, (int)get_gm_jiffies() - (int)rc_node->timer);
            match_result = fscaler300_rc_check_node_matched(&rcv_info, rc_node);
        }

        /* step_3. do match */
        switch (match_result) {
            case RC_MATCH_STATUS_DONE:
                /* triggle HW */
                fscaler300_put_rc_node(rc_node);
                break;
            case RC_MATCH_STATUS_FAIL:
            case RC_MATCH_EP_VG_SN_BIGGER:
                /* need callback fail */
                fscaler300_do_rc_node_fail(rc_node);
                break;
            case RC_MATCH_EP_SN_SMALLER:
            case RC_MATCH_LIST_EMPTY:/* error case, FIXME!  */
            case RC_MATCH_EP_SN_BIGGER: /* error case, FIXME!  */
            case RC_MATCH_EP_VG_SN_SMALLER:
                printm(MODULE_NAME, "error case !! FIXME\n");
            default:
                /* skip it, so do nothing */
                break;
        }

        up(&global_info.sema_lock);

    } while(!kthread_should_stop());


    return 0;
}

int fscaler300_ep_rx_process(void *parm)
{
    int ret = 0, timeout = 15;
    volatile int need_callback = 0, rc_callback_timeout = 0;
    int count = 0;

    do {
        set_user_nice(current, -19);
    	need_callback = 0;
        /* receive the data from RC */
        /* step_1.  receive the data from EP */
        ret = fscaler300_ep_io_rx(timeout);

        count = 0;
        down(&global_info.sema_lock);
        if (flow_check) printm(MODULE_NAME, "%s sema count = %d\n", __func__, global_info.sema_lock.count);
        if (ret >= 0) { /* received rc_node --> do match */
            int timeout_value = 0;

            timeout_value = fscaler300_is_node_timeout();
            if (timeout_value & 0x1) {
                if (dual_debug == 1)printm("SL","rc_callback_timeout111\n");
                rc_callback_timeout = 1;
            } else if (timeout_value &0x2) {
                if (dual_debug == 1)printm("SL","need_callback111\n");
                need_callback = 1;
            }

            ret = fscaler300_ep_do_node_matched();
            if (ret == EP_MATCH_FAILED) {
                rc_callback_timeout = need_callback = 1;
            }
        } else {  /* received timeout */
            int timeout_value = 0;
            ret = fscaler300_ep_do_node_matched();
            if (ret == EP_MATCH_FAILED) {
                rc_callback_timeout = need_callback = 1;
            }
            timeout_value = fscaler300_is_node_timeout();
            if (timeout_value & 0x1) {
                if (dual_debug == 1)printm("SL","rc_callback_timeout222\n");
                rc_callback_timeout = 1;
            } else if (timeout_value &0x2) {
                if (dual_debug == 1)printm("SL","need_callback222\n");
                need_callback = 1;
            }
        }
        up(&global_info.sema_lock);

        if (rc_callback_timeout) {
            if (dual_debug == 1) printm(MODULE_NAME, "+++++++ wake up tx2\n");
#ifdef USE_KTHREAD
#if GM8210
            /* callback to RC */
            eptx_wake_up();
#endif
#endif
#ifdef USE_WQ
            PREPARE_DELAYED_WORK(&process_ep_work,(void *)fscaler300_ep_process);
            queue_delayed_work(callback_workq, &process_ep_work, msecs_to_jiffies(0));
#endif
        }
        if (need_callback) {
            if (dual_debug == 1) printm(MODULE_NAME, "---- cb wake\n");
#ifdef USE_KTHREAD
            /* callback to local videograph */
            callback_wake_up();
            /* trigger 2ddma and callback to RC */
            callback_2ddma_wake_up();
#endif
#ifdef USE_WQ
            /* schedule the delay workq */
            if (dual_debug == 1) printm(MODULE_NAME, "waku up callback %d\n", __LINE__);
            PREPARE_DELAYED_WORK(&process_callback_work,(void *)fscaler300_callback_process);
            queue_delayed_work(callback_workq, &process_callback_work, 0);

            PREPARE_DELAYED_WORK(&process_2ddma_callback_work,(void *)fscaler300_2ddma_callback_process);
            queue_delayed_work(callback_workq, &process_2ddma_callback_work, 0);
#endif
        }

    } while(!kthread_should_stop());

    return 0;
}
#endif /* GM8210 */

/*
 * put JOB
 */
static int driver_putjob(void *parm)
{
    struct video_entity_job_t *job = (struct video_entity_job_t *)parm;
    struct video_entity_job_t *next_job = 0;
    job_node_t  *node_item = 0, *root_node_item = 0;
    int multi_jobs = 0, idx = 0;
    int ret = 0;
#if GM8210
    int ep_flag = 0, rc_flag = 0;
    int rc_fail_flag = 0;
#endif

    down(&global_info.sema_lock);
    if (flow_check) printm(MODULE_NAME, "%s sema count = %d\n", __func__, global_info.sema_lock.count);
    do {
        idx = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
        /*
         * parse the parameters and assign to param structure
         */
        node_item = kmem_cache_alloc(global_info.node_cache, GFP_KERNEL);
        if (node_item == NULL)
            panic("%s, no memory! \n", __func__);

        MEMCNT_ADD(&priv, 1);
        memset(node_item, 0x0, sizeof(job_node_t));

        node_item->property.vproperty = kmem_cache_zalloc(global_info.vproperty_cache, GFP_KERNEL);
        if (node_item->property.vproperty == NULL)
            panic("%s, no memory! \n", __func__);

        MEMCNT_ADD(&priv, 1);
        node_item->engine = ENTITY_FD_ENGINE(job->fd);
        if (node_item->engine > ENTITY_ENGINES) {
            printk("Error to use %s engine %d, max is %d\n",MODULE_NAME,node_item->engine,ENTITY_ENGINES);
            goto err;
        }

        node_item->minor = ENTITY_FD_MINOR(job->fd);
        if (node_item->minor > max_minors) {
            printk("Error to use %s minor %d, max is %d\n",MODULE_NAME,node_item->minor,max_minors);
            goto err;
        }

        node_item->job_id = job->id;
        ret = driver_parse_in_property(node_item, job);
        if (ret < 0) {
            printk("[%s] driver_parse_in_property fail\n", __func__);
            goto err;
        }
        if (flow_check) {
            if (I_AM_EP(node_item->property.vproperty[node_item->ch_id].scl_feature))
                printm(MODULE_NAME, "%s EP id %u\n", __func__, job->id);
            else if (I_AM_RC(node_item->property.vproperty[node_item->ch_id].scl_feature))
                printm(MODULE_NAME, "%s RC id %u\n", __func__, job->id);
            else
                printm(MODULE_NAME, "%s normal id %u\n", __func__, job->id);
        }

        node_item->property.in_addr_pa = job->in_buf.buf[0].addr_pa;
        node_item->property.in_size = job->in_buf.buf[0].size;
        node_item->property.in_addr_pa_1 = job->in_buf.buf[1].addr_pa;
        node_item->property.in_1_size = job->in_buf.buf[1].size;

        node_item->property.out_addr_pa = job->out_buf.buf[0].addr_pa;
        node_item->property.out_size = job->out_buf.buf[0].size;
        node_item->property.out_addr_pa_1 = job->out_buf.buf[1].addr_pa;
        node_item->property.out_1_size = job->out_buf.buf[1].size;

        INIT_LIST_HEAD(&node_item->plist);
        INIT_LIST_HEAD(&node_item->clist);
        INIT_LIST_HEAD(&node_item->vg_plist);
        INIT_LIST_HEAD(&node_item->vg_clist);
        INIT_LIST_HEAD(&node_item->rclist);
        INIT_LIST_HEAD(&node_item->eplist);
#if GM8210
        //  RC tx pci address to EP
        if (I_AM_RC(node_item->property.vproperty[node_item->ch_id].scl_feature)) {
            if (dual_debug == 1)
                printm(MODULE_NAME, "%s put rc job %u\n", __func__, node_item->job_id);
            rc_flag = 1;
            if (rc_tx_node_cnt <= rc_tx_threshold) {
                rc_tx_node_cnt++;
                // add to rclist
                list_add_tail(&node_item->rclist, &global_info.rc_list);
                // add to node_list
                list_add_tail(&node_item->plist, &global_info.node_list);
                vg_get_serial_num(node_item);
                /* RC sends pcie_add and vg_serial to EP */
                ret = fscaler300_rc_io_tx(node_item);
                if (ret < 0)
                    damnit(MODULE_NAME);
            } else {
                if (dual_debug == 1)
                    printm(MODULE_NAME, "%s put rc job %u fail\n", __func__, node_item->job_id);
                rc_fail_flag = 1;
                list_add_tail(&node_item->plist, &global_info.node_list);
            }
        }

        if (I_AM_EP(node_item->property.vproperty[node_item->ch_id].scl_feature)) {
            if (dual_debug == 1)
                printm(MODULE_NAME, "%s put ep job %u\n", __func__, node_item->job_id);
            /* I am EP */
            list_add_tail(&node_item->eplist, &global_info.ep_list);
            down(&global_info.sema_lock_2ddma);
            list_add_tail(&node_item->plist, &global_info.node_list_2ddma);
            up(&global_info.sema_lock_2ddma);

            node_item->rc_node = NULL;

            fscaler300_set_node_timeout(node_item);
            ep_flag = 1;

            /* change the ex_rx_timeout and record minor number */
            if (ep_rx_vg_serial != node_item->property.vg_serial) {
                ep_rx_vg_serial = node_item->property.vg_serial;
                ex_rx_timeout = EP_ON_GOING_TIMEOUT;
                ep_rx_timeout_minor_nr = node_item->minor;
            }
        }
#endif /* GM8210 */
        vg_cal_buf1_addr(node_item);

        node_item->property.ratio = ratio;
        node_item->idx = idx;
        node_item->job_id = job->id;
        node_item->status = JOB_STS_QUEUE;
        node_item->private = job;
        node_item->puttime = jiffies;
        node_item->starttime = 0;
        node_item->finishtime = 0;
        node_item->priority = 0;//job->priority;
#if GM8210
        if (rc_fail_flag == 1)
            node_item->status = JOB_STS_FAIL;
#endif
        if (dual_debug == 1) {
            if (!I_AM_EP(node_item->property.vproperty[node_item->ch_id].scl_feature) &&
                !I_AM_RC(node_item->property.vproperty[node_item->ch_id].scl_feature))
                printm(MODULE_NAME, "%s put normal job %u\n", __func__, node_item->job_id);
        }

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
            node_item->parent = node_item;
            node_item->type = SINGLE_NODE;
        }
    } while((job = next_job));
#if GM8210
    if (ep_flag) {
        int timeout_value = 0;
        int need_callback = 0, rc_callback_timeout = 0;
        if (node_item->type != SINGLE_NODE) {
            /* harry: how about child ? */
            kmem_cache_free(global_info.node_cache, node_item);
            goto err;
        }

        timeout_value = fscaler300_is_node_timeout();
        if (timeout_value & 0x1) {
            if (dual_debug == 1)printm("SL","rc_callback_timeout111\n");
            rc_callback_timeout = 1;
        } else if (timeout_value &0x2) {
            if (dual_debug == 1)printm("SL","need_callback111\n");
            need_callback = 1;
        }

        ret = fscaler300_ep_do_node_matched();
        if (ret == EP_MATCH_FAILED || ret == EP_MATCH_OK) {
            need_callback = rc_callback_timeout = 1;
        }

        if (rc_callback_timeout == 1) {
            if (dual_debug == 1) printm(MODULE_NAME, "+++++++ wake up tx3\n");
#ifdef USE_KTHREAD
#if GM8210
            eptx_wake_up();
#endif
#endif
#ifdef USE_WQ
            PREPARE_DELAYED_WORK(&process_ep_work,(void *)fscaler300_ep_process);
            queue_delayed_work(callback_workq, &process_ep_work, msecs_to_jiffies(0));
#endif
        }
        if (need_callback == 1) {
#ifdef USE_KTHREAD
            callback_wake_up();
            callback_2ddma_wake_up();
#endif
#ifdef USE_WQ
            /* schedule the delay workq */
            PREPARE_DELAYED_WORK(&process_callback_work,(void *)fscaler300_callback_process);
            queue_delayed_work(callback_workq, &process_callback_work, 0);

            PREPARE_DELAYED_WORK(&process_2ddma_callback_work,(void *)fscaler300_2ddma_callback_process);
            queue_delayed_work(callback_workq, &process_2ddma_callback_work, 0);
#endif
        }
        /* don't put job to driver core */
        goto exit;
    }

    if (rc_flag) {
        if (node_item->type != SINGLE_NODE) {
            kmem_cache_free(global_info.node_cache, node_item);
            goto err;
        }
        /* don't put job to driver core */
        goto exit;
    }
#if GM8210
    if (rc_fail_flag)
        goto exit;
#endif
#endif
    if (node_item->type == SINGLE_NODE) {
        ret = vg_put_single_node(node_item);
        if (ret < 0) {
            printk("[%s] vg_put_single_node fail\n", __func__);
            goto err;
        }
    }
    else {
        ret = vg_put_multi_node(root_node_item, multi_jobs);
        if (ret < 0) {
            printk("[%s] vg_put_multi_node fail\n", __func__);
            goto err;
        }
    }
#if GM8210
exit:
#endif
    up(&global_info.sema_lock);
    return JOB_STATUS_ONGOING;

err:
    printk("[SCALER300] putjob error\n");
    if (multi_jobs == 0) {
        kmem_cache_free(global_info.vproperty_cache, node_item->property.vproperty);
        MEMCNT_SUB(&priv, 1);
        kmem_cache_free(global_info.node_cache, node_item);
        MEMCNT_SUB(&priv, 1);
    }
    else {
        job_node_t *curr, *ne;
        /* free child node */
        list_for_each_entry_safe(curr, ne, &root_node_item->clist, clist) {
            kmem_cache_free(global_info.vproperty_cache, curr->property.vproperty);
            MEMCNT_SUB(&priv, 1);
            kmem_cache_free(global_info.node_cache, curr);
            MEMCNT_SUB(&priv, 1);
        }
        /* free parent node */
        kmem_cache_free(global_info.vproperty_cache, root_node_item->property.vproperty);
        MEMCNT_SUB(&priv, 1);
        kmem_cache_free(global_info.node_cache, root_node_item);
        MEMCNT_SUB(&priv, 1);
    }
    up(&global_info.sema_lock);
    printm(MODULE_NAME, "PANIC!! putjob error\n");
    if (damnitoff == 0)
        damnit(MODULE_NAME);
    return JOB_STATUS_FAIL;
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

void driver_stop_eplist(int minor)
{
    job_node_t *node, *ne;

    /* go through all EP jobs in Q.*/
    list_for_each_entry_safe(node, ne, &global_info.ep_list, eplist) {
        if (node->minor != minor)
            continue;
        node->status = JOB_STS_FAIL;
        list_del(&node->eplist);
        vg_nodelist_add(node);
    }

    /* go through all rc_nodes still stay in ep_rcnode_list */
    list_for_each_entry_safe(node, ne, &global_info.ep_rcnode_list, plist) {
        node->status = JOB_STS_FAIL;
        /* move to callback list */
        list_del(&node->plist);
        list_add_tail(&node->plist, &global_info.ep_rcnode_cb_list);
    }

    /* reset the ex_rx_timeout value */
    if (minor == ep_rx_timeout_minor_nr) {
        ep_rx_vg_serial = 0;
        ex_rx_timeout = EP_NOT_ON_GOING_TIMEOUT;
    }
}

static int driver_stop(void *parm, int engine, int minor)
{
    job_node_t *node, *curr, *ne;

    if (flow_check) printm(MODULE_NAME, "driver stop minor [%d]\n", minor);

    fscaler300_stop_channel(minor);

    down(&global_info.sema_lock);

    list_for_each_entry_safe(node, ne, &global_info.node_list ,plist) {
        if ((node->status == JOB_STS_DONOTHING) && (node->minor == minor)) {
            node->status = JOB_STS_FLUSH;
            list_for_each_entry_safe(curr, ne, &node->clist, clist)
                curr->status = JOB_STS_FLUSH;
        }
    }

    driver_stop_eplist(minor);

    up(&global_info.sema_lock);
#ifdef USE_KTHREAD
    callback_wake_up();
    callback_2ddma_wake_up();
#endif
#ifdef USE_WQ
    /* schedule the delay workq */
    if (dual_debug == 1) printm(MODULE_NAME, "waku up callback %d\n", __LINE__);
    PREPARE_DELAYED_WORK(&process_callback_work,(void *)fscaler300_callback_process);
    queue_delayed_work(callback_workq, &process_callback_work, callback_period);

    PREPARE_DELAYED_WORK(&process_2ddma_callback_work,(void *)fscaler300_2ddma_callback_process);
    queue_delayed_work(callback_workq, &process_2ddma_callback_work, callback_period);

#endif
    return 0;
}


static int driver_queryid(void *parm,char *str)
{
    int i;

    for(i = 0; i < sizeof(property_map) / sizeof(struct property_map_t); i++) {
        if(strcmp(property_map[i].name,str) == 0) {
            return property_map[i].id;
        }
    }
    printk("driver_queryid: Error to find name %s\n",str);
    return -1;
}

static int driver_querystr(void *parm,int id,char *string)
{
    int i;

    for(i = 0; i < sizeof(property_map) / sizeof(struct property_map_t); i++) {
        if(property_map[i].id == id) {
            memcpy(string,property_map[i].name,sizeof(property_map[i].name));
            return 0;
        }
    }
    printk("driver_querystr: Error to find id %d\n",id);
    return -1;
}

struct video_entity_ops_t driver_ops = {
    putjob:      &driver_putjob,
    stop:        &driver_stop,
    queryid:     &driver_queryid,
    querystr:    &driver_querystr,
};

struct video_entity_t fscaler300_entity={
    type:       TYPE_SCALER,
    name:       "scaler",
    engines:    ENTITY_ENGINES,
    minors:     ENTITY_MINORS,
    ops:        &driver_ops
};

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

    address = priv.engine[0].fscaler300_reg;   ////<<<<<<<< Update your register dump here >>>>>>>>>//issue
    size=0x1B0;  //<<<<<<<< Update your register dump here >>>>>>>>>
    for (i = 0; i < size; i = i + 0x10) {
        seq_printf(sfile, "0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address+i),*(unsigned int *)(address+i),
            *(unsigned int *)(address+i+4),*(unsigned int *)(address+i+8),*(unsigned int *)(address+i+0xc));
    }
    seq_printf(sfile, "\n");
    address += 0x5100;
    size = 0x10;
    seq_printf(sfile, "0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address),*(unsigned int *)(address),
        *(unsigned int *)(address+4),*(unsigned int *)(address+8),*(unsigned int *)(address+0xc));

    seq_printf(sfile, "\n");
    address += 0x100;
    size = 0x10;
    seq_printf(sfile, "0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address),*(unsigned int *)(address),
        *(unsigned int *)(address+4),*(unsigned int *)(address+8),*(unsigned int *)(address+0xc));
    seq_printf(sfile, "\n");
    address += 0x200;
    size = 0x10;
    seq_printf(sfile, "0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address),*(unsigned int *)(address),
        *(unsigned int *)(address+4),*(unsigned int *)(address+8),*(unsigned int *)(address+0xc));
    seq_printf(sfile, "\n");
    address += 0x1d0;
    size = 0x10;
    seq_printf(sfile, "0x%08x  0x%08x 0x%08x 0x%08x 0x%08x\n",(address),*(unsigned int *)(address),
        *(unsigned int *)(address+4),*(unsigned int *)(address+8),*(unsigned int *)(address+0xc));
    seq_printf(sfile, "\n");

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

static int vg_proc_ratio_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\nratio = %d\n", ratio);

    return 0;
}

static ssize_t vg_proc_ratio_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int mode_set = 0;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d \n", &mode_set);
    ratio = mode_set;

    seq_printf(sfile, "ratio = %d\n", ratio);

    return count;
}

static int vg_proc_util_show(struct seq_file *sfile, void *v)
{
    int i;

    for (i = 0; i < priv.eng_num; i++) {
        //if (utilization_start[i] == 0)
          //  continue;
        //if (jiffies > utilization_start[i])
          seq_printf(sfile, "\nEngine%d HW Utilization Period=%d(sec) Utilization=%d frame_cnt=%d dummy_cnt=%d\n",
                i,utilization_period,utilization_record[i], frame_cnt[i], dummy_cnt[i]);
    }

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

static int vg_proc_debug_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\ndebug_mode = %d\n", debug_mode);

    return 0;
}

static ssize_t vg_proc_debug_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int mode_set = 0;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d \n", &mode_set);
    debug_mode = mode_set;

    seq_printf(sfile, "debug_mode = %d\n", debug_mode);

    return count;
}

static int vg_proc_version_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\ndriver version = %d\n", VERSION);

    return 0;
}

static int vg_proc_buf_clean_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\nbuf clean = black\n");

    return 0;
}

static int vg_proc_dual_debug_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\ndual_debug = %d\n", dual_debug);

    return 0;
}

static ssize_t vg_proc_dual_debug_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int mode_set = 0;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d \n", &mode_set);
    dual_debug = mode_set;

    seq_printf(sfile, "dual_debug = %d\n", dual_debug);

    return count;
}

static int vg_proc_rc_tx_threshold_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\nrc_tx_threshold = %d\n", rc_tx_threshold);

    return 0;
}

static ssize_t vg_proc_rc_tx_threshold_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int mode_set = 0;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d \n", &mode_set);
    rc_tx_threshold = mode_set;

    seq_printf(sfile, "rc_tx_threshold = %d\n", rc_tx_threshold);

    return count;
}

static int vg_proc_ep_rx_timeout_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\nep_rx_timeout = %d\n", ep_rx_timeout);

    return 0;
}

static ssize_t vg_proc_ep_rx_timeout_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int mode_set = 0;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d \n", &mode_set);
    ep_rx_timeout = mode_set;

    seq_printf(sfile, "ep_rx_timeout = %d\n", ep_rx_timeout);

    return count;
}

static int vg_proc_ep_timeout_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\nep_pcie_addr_timeout: got job but no pcie addr, ep_job_timeout: got pcie addr but no job\n");
    seq_printf(sfile, "EP rc node timeout cnt = %u\n", ep_pcie_addr_timeout_cnt);
    seq_printf(sfile, "EP ep_list timeout cnt = %u\n", ep_job_timeout_cnt);
    seq_printf(sfile, "ex_rx_timeout : %d msec. \n", ex_rx_timeout);
    seq_printf(sfile, "rc_node_mem_cnt : %d \n", rc_node_mem_cnt);
    seq_printf(sfile, "rc_tx_node_cnt : %d \n", rc_tx_node_cnt);
    seq_printf(sfile, "RC node fail : %d \n", rc_fail);
    seq_printf(sfile, "rc_drop_cnt : %d \n", rc_drop_cnt);
    seq_printf(sfile, "ep_drop_cnt : %d \n", ep_drop_cnt);
    return 0;
}

static ssize_t vg_proc_buf_clean_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int mode_set = 0;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d \n", &mode_set);

    if (mode_set == 0)
        BUF_CLEAN_COLOR = OSD_PALETTE_COLOR_BLACK;
    if (mode_set == 1)
        BUF_CLEAN_COLOR = OSD_PALETTE_COLOR_RED;

    if (mode_set == 0)
        seq_printf(sfile, "buf clean = black\n");
    if (mode_set == 1)
        seq_printf(sfile, "buf clean = red\n");

    return count;
}

static int vg_proc_rc_ep_diff_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\nrc_ep_diff = %d\n", rc_ep_diff_cnt);

    return 0;
}

static ssize_t vg_proc_rc_ep_diff_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int mode_set = 0;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d \n", &mode_set);
    ep_rx_timeout = mode_set;

    seq_printf(sfile, "rc_ep_diff = %d\n", rc_ep_diff_cnt);

    return count;
}

static int vg_proc_rxtx_max_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "rxtx_max = %d\n", rxtx_max);
#if GM8210
#ifdef TEST_WQ
    seq_printf(sfile, "test_diff = %d\n", test_diff);
	seq_printf(sfile, "temp_diff = %d\n", temp_diff);
#endif
#endif
	rxtx_max = 0;
#if GM8210
#ifdef TEST_WQ
	test_diff = 0;
	temp_diff = 0;
#endif
#endif
    return 0;
}

static int vg_proc_flow_check_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "\nflow_check = %d\n", flow_check);

    return 0;
}

static ssize_t vg_proc_flow_check_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int mode_set = 0;
    char value_str[16] = {'\0'};
    struct seq_file       *sfile    = (struct seq_file *)file->private_data;

    if (copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d \n", &mode_set);
    flow_check = mode_set;

    seq_printf(sfile, "flow_check = %d\n", flow_check);

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

static int vg_proc_ratio_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_ratio_show, PDE(inode)->data);
}

static int vg_proc_util_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_util_show, PDE(inode)->data);
}

static int vg_proc_debug_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_debug_show, PDE(inode)->data);
}

static int vg_proc_version_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_version_show, PDE(inode)->data);
}

static int vg_proc_buf_clean_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_buf_clean_show, PDE(inode)->data);
}

static int vg_proc_ep_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_ep_timeout_show, PDE(inode)->data);
}

static int vg_proc_dual_debug_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_dual_debug_show, PDE(inode)->data);
}

static int vg_proc_rc_tx_threshold_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_rc_tx_threshold_show, PDE(inode)->data);
}

static int vg_proc_ep_rx_timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_ep_rx_timeout_show, PDE(inode)->data);
}

static int vg_proc_rc_ep_diff_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_rc_ep_diff_show, PDE(inode)->data);
}

static int vg_proc_rxtx_max_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_rxtx_max_show, PDE(inode)->data);
}

static int vg_proc_flow_check_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_proc_flow_check_show, PDE(inode)->data);
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

static struct file_operations vg_proc_ratio_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_ratio_open,
    .write  = vg_proc_ratio_write,
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

static struct file_operations vg_proc_debug_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_debug_open,
    .write  = vg_proc_debug_write,
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

static struct file_operations vg_proc_buf_clean_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_buf_clean_open,
    .write  = vg_proc_buf_clean_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_ep_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_ep_timeout_open,
//    .write  = vg_proc_ep_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_dual_debug_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_dual_debug_open,
    .write  = vg_proc_dual_debug_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_rc_tx_threshold_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_rc_tx_threshold_open,
    .write  = vg_proc_rc_tx_threshold_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_ep_rx_timeout_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_ep_rx_timeout_open,
    .write  = vg_proc_ep_rx_timeout_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_rc_ep_diff_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_rc_ep_diff_open,
    .write  = vg_proc_rc_ep_diff_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_rxtx_max_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_rxtx_max_open,
    //.write  = vg_proc_rxtx_max_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations vg_proc_flow_check_ops = {
    .owner  = THIS_MODULE,
    .open   = vg_proc_flow_check_open,
    .write  = vg_proc_flow_check_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

#define ENTITY_PROC_NAME "videograph/scaler300"

static int vg_proc_init(void)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
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

    /* buffer threshold for decoder */
    vg_proc_info.ratio = create_proc_entry("ratio", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.ratio == NULL) {
        printk("error to create %s/ratio proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.ratio->proc_fops  = &vg_proc_ratio_ops;
    vg_proc_info.ratio->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.ratio->owner      = THIS_MODULE;
#endif

    /* debug_mode */
    vg_proc_info.debug = create_proc_entry("debug_mode", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.debug == NULL) {
        printk("error to create %s/debug_mode proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.debug->proc_fops  = &vg_proc_debug_ops;
    vg_proc_info.debug->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.debug->owner      = THIS_MODULE;
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

    /* debug buf clean */
    vg_proc_info.buf_clean = create_proc_entry("buf_clean", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.buf_clean == NULL) {
        printk("error to create %s/buf_clean proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.buf_clean->proc_fops  = &vg_proc_buf_clean_ops;
    vg_proc_info.buf_clean->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.buf_clean->owner      = THIS_MODULE;
#endif

    /* ep_timeout */
    fmem_get_identifier(&pci_id, &cpu_id);
    if ((pci_id == FMEM_PCI_DEV0) && (cpu_id == FMEM_CPU_FA626)) {
        vg_proc_info.ep_timeout = create_proc_entry("ep_timeout_read", S_IRUGO|S_IXUGO, vg_proc_info.root);
        if(vg_proc_info.ep_timeout == NULL) {
            printk("error to create %s/ep_timeout_read proc\n", ENTITY_PROC_NAME);
            ret = -EFAULT;
        }
        (vg_proc_info.ep_timeout)->proc_fops  = &vg_proc_ep_timeout_ops;
        (vg_proc_info.ep_timeout)->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
        vg_proc_info.ep_timeout->owner      = THIS_MODULE;
#endif
    }

    /* debug dual 8210 */
    vg_proc_info.dual_debug = create_proc_entry("dual_debug", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.dual_debug == NULL) {
        printk("error to create %s/dual_debug proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.dual_debug->proc_fops  = &vg_proc_dual_debug_ops;
    vg_proc_info.dual_debug->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.dual_debug->owner      = THIS_MODULE;
#endif

    /* rc tx threshold */
    vg_proc_info.rc_tx_threshold = create_proc_entry("rc_tx_threshold", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.rc_tx_threshold == NULL) {
        printk("error to create %s/threshold proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.rc_tx_threshold->proc_fops  = &vg_proc_rc_tx_threshold_ops;
    vg_proc_info.rc_tx_threshold->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.rc_tx_threshold->owner      = THIS_MODULE;
#endif

    /* ep rx timeout */
    vg_proc_info.ep_rx_timeout = create_proc_entry("ep_timeout_write", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.ep_rx_timeout == NULL) {
        printk("error to create %s/ep_timeout_write proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.ep_rx_timeout->proc_fops  = &vg_proc_ep_rx_timeout_ops;
    vg_proc_info.ep_rx_timeout->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.ep_rx_timeout->owner      = THIS_MODULE;
#endif

    /* rc_ep_diff */
    vg_proc_info.rc_ep_diff = create_proc_entry("rc_ep_diff", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.rc_ep_diff == NULL) {
        printk("error to create %s/rc_ep_diff proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.rc_ep_diff->proc_fops  = &vg_proc_rc_ep_diff_ops;
    vg_proc_info.rc_ep_diff->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.rc_ep_diff->owner      = THIS_MODULE;
#endif

	/* rxtx_max */
    vg_proc_info.rxtx_max = create_proc_entry("rxtx_max", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.rxtx_max == NULL) {
        printk("error to create %s/rxtx_max proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.rxtx_max->proc_fops  = &vg_proc_rxtx_max_ops;
    vg_proc_info.rxtx_max->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.rxtx_max->owner      = THIS_MODULE;
#endif

    /* flow_check */
    vg_proc_info.flow_check = create_proc_entry("flow_check", S_IRUGO|S_IXUGO, vg_proc_info.root);
    if(vg_proc_info.flow_check == NULL) {
        printk("error to create %s/flow_check proc\n", ENTITY_PROC_NAME);
        ret = -EFAULT;
    }
    vg_proc_info.flow_check->proc_fops  = &vg_proc_flow_check_ops;
    vg_proc_info.flow_check->data       = (void *)&vg_proc_info;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vg_proc_info.flow_check->owner      = THIS_MODULE;
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
        if (vg_proc_info.ratio!=0)
            remove_proc_entry(vg_proc_info.ratio->name, vg_proc_info.root);
        if (vg_proc_info.debug!=0)
            remove_proc_entry(vg_proc_info.debug->name, vg_proc_info.root);
        if (vg_proc_info.version!=0)
            remove_proc_entry(vg_proc_info.version->name, vg_proc_info.root);
        if (vg_proc_info.buf_clean!=0)
            remove_proc_entry(vg_proc_info.buf_clean->name, vg_proc_info.root);
        if (vg_proc_info.ep_timeout!=0)
            remove_proc_entry(vg_proc_info.ep_timeout->name, vg_proc_info.root);
        if (vg_proc_info.dual_debug!=0)
            remove_proc_entry(vg_proc_info.dual_debug->name, vg_proc_info.root);
        if (vg_proc_info.rc_tx_threshold!=0)
            remove_proc_entry(vg_proc_info.rc_tx_threshold->name, vg_proc_info.root);
        if (vg_proc_info.ep_rx_timeout!=0)
            remove_proc_entry(vg_proc_info.ep_rx_timeout->name, vg_proc_info.root);
        if (vg_proc_info.rc_ep_diff!=0)
            remove_proc_entry(vg_proc_info.rc_ep_diff->name, vg_proc_info.root);
        if (vg_proc_info.rxtx_max!=0)
            remove_proc_entry(vg_proc_info.rxtx_max->name, vg_proc_info.root);
        if (vg_proc_info.flow_check!=0)
            remove_proc_entry(vg_proc_info.flow_check->name, vg_proc_info.root);

        remove_proc_entry(vg_proc_info.root->name, vg_proc_info.root->parent);
    }
}

int fscaler300_log_panic_handler(int data)
{
    printm(MODULE_NAME, "PANIC!! Processing Start\n");
    //scaler300_disable();
    // disable scaler300
    // delete timeout timer

    //printm(MODULE_NAME, "panic handle scaler300 disable\n");

    printm(MODULE_NAME, "PANIC!! Processing End\n");
    return 0;
}

int fscaler300_log_printout_handler(int data)
{
    job_node_t  *node, *ne, *curr, *ne1;

    printm(MODULE_NAME, "PANIC!! PrintOut Start\n");

    list_for_each_entry_safe(node, ne, &global_info.node_list ,plist) {
        if(node->status == JOB_STS_ONGOING)
            printm(MODULE_NAME, "ONGOING JOB ID(%u) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback);
        else if(node->status == JOB_STS_QUEUE)
            printm(MODULE_NAME, "QUEUE JOB ID(%u) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback);
        else if(node->status == JOB_STS_SRAM)
            printm(MODULE_NAME, "SRAM JOB ID(%u) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback);
        else if(node->status == JOB_STS_DONOTHING)
            printm(MODULE_NAME, "DONOTHING JOB ID(%u) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback);
        else if(node->status == JOB_STS_FAIL)
            printm(MODULE_NAME, "FAIL JOB ID(%u) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback);
        else if(node->status == JOB_STS_DONE)
            printm(MODULE_NAME, "DONE JOB ID(%u) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback);
        else
            printm(MODULE_NAME, "??? status %x JOB ID (%u) TYPE(%x) need_callback(%d)\n", node->job_id, node->type, node->need_callback, node->status);

        list_for_each_entry_safe(curr, ne1, &node->clist ,clist) {
        if(curr->status == JOB_STS_ONGOING)
            printm(MODULE_NAME, "ONGOING JOB ID(%u) TYPE(%x) need_callback(%d)\n", curr->job_id, curr->type, curr->need_callback);
        else if(curr->status == JOB_STS_QUEUE || node->status == JOB_STS_SRAM)
            printm(MODULE_NAME, "PENDING JOB ID(%u) TYPE(%x) need_callback(%d)\n", curr->job_id, curr->type, curr->need_callback);
        else if(curr->status == JOB_STS_DONOTHING)
            printm(MODULE_NAME, "DONOTHING JOB ID(%u) TYPE(%x) need_callback(%d)\n", curr->job_id, curr->type, curr->need_callback);
        else
            printm(MODULE_NAME, "FLUSH JOB ID (%u) TYPE(%x) need_callback(%d)\n", curr->job_id, curr->type, curr->need_callback);
        }
    }
    printm(MODULE_NAME, "EP rcnode cb list\n");
    list_for_each_entry_safe(node, ne, &global_info.ep_rcnode_cb_list, plist) {
        if(node->status == JOB_STS_ONGOING)
            printm(MODULE_NAME, "ONGOING JOB TYPE(%x) sn %d version %d curr jiff %d stamp %d\n", node->type, node->serial_num, node->property.vg_serial, jiffies, node->timer);
        else if(node->status == JOB_STS_QUEUE || node->status == JOB_STS_SRAM)
            printm(MODULE_NAME, "PENDING JOB TYPE(%x) sn %d version %d curr jiff %d stamp %d\n", node->type, node->serial_num, node->property.vg_serial, jiffies, node->timer);
        else if(node->status == JOB_STS_DONOTHING)
            printm(MODULE_NAME, "DONOTHING JOB TYPE(%x) sn %d version %d curr jiff %d stamp %d\n", node->type, node->serial_num, node->property.vg_serial, jiffies, node->timer);
        else if(node->status == JOB_STS_FAIL)
            printm(MODULE_NAME, "FAIL JOB  TYPE(%x) sn %d version %d curr jiff %d stamp %d\n", node->type, node->serial_num, node->property.vg_serial, jiffies, node->timer);
        else if(node->status == JOB_STS_DONE)
            printm(MODULE_NAME, "DONE JOB  TYPE(%x) sn %d version %d curr jiff %d stamp %d\n", node->type, node->serial_num, node->property.vg_serial, jiffies, node->timer);
        else if(node->status == JOB_STS_MATCH)
            printm(MODULE_NAME, "MATCH JOB  TYPE(%x) sn %d version %d curr jiff %d stamp %d\n", node->type, node->serial_num, node->property.vg_serial, jiffies, node->timer);
        else if(node->status == JOB_STS_TX)
            printm(MODULE_NAME, "TX JOB  TYPE(%x) sn %d version %d curr jiff %d stamp %d\n", node->type, node->serial_num, node->property.vg_serial, jiffies, node->timer);
        else if(node->status == JOB_STS_TIMEOUT)
            printm(MODULE_NAME, "TIMEOUT JOB  TYPE(%x) sn %d version %d curr jiff %d stamp %d\n", node->type, node->serial_num, node->property.vg_serial, jiffies, node->timer);
        else
            printm(MODULE_NAME, "FLUSH JOB  TYPE(%x) sn %d version %d curr jiff %d stamp %d\n", node->type, node->serial_num, node->property.vg_serial, jiffies, node->timer);
    }
    printm(MODULE_NAME, "RC list\n");
    list_for_each_entry_safe(node, ne, &global_info.rc_list, rclist) {
        if(node->status == JOB_STS_ONGOING)
            printm(MODULE_NAME, "ONGOING JOB ID(%u) TYPE(%x) sn %d version %d\n", node->job_id, node->type, node->serial_num, node->property.vg_serial);
        else if(node->status == JOB_STS_QUEUE || node->status == JOB_STS_SRAM)
            printm(MODULE_NAME, "PENDING JOB ID(%u) TYPE(%x) sn %d version %d\n", node->job_id, node->type, node->serial_num, node->property.vg_serial);
        else if(node->status == JOB_STS_DONOTHING)
            printm(MODULE_NAME, "DONOTHING JOB ID(%u) TYPE(%x) sn %d version %d\n", node->job_id, node->type, node->serial_num, node->property.vg_serial);
        else if(node->status == JOB_STS_FAIL)
            printm(MODULE_NAME, "FAIL JOB ID(%u) TYPE(%x) sn %d version %d\n", node->job_id, node->type, node->serial_num, node->property.vg_serial);
        else if(node->status == JOB_STS_DONE)
            printm(MODULE_NAME, "DONE JOB ID(%u) TYPE(%x) sn %d version %d\n", node->job_id, node->type, node->serial_num, node->property.vg_serial);
        else if(node->status == JOB_STS_MATCH)
            printm(MODULE_NAME, "MATCH JOB ID(%u) TYPE(%x) sn %d version %d\n", node->job_id, node->type, node->serial_num, node->property.vg_serial);
        else if(node->status == JOB_STS_TX)
            printm(MODULE_NAME, "TX JOB ID(%u) TYPE(%x) sn %d version %d\n", node->job_id, node->type, node->serial_num, node->property.vg_serial);
        else if(node->status == JOB_STS_TIMEOUT)
            printm(MODULE_NAME, "TIMEOUT JOB  TYPE(%x) sn %d version %d curr jiff %d stamp %d\n", node->type, node->serial_num, node->property.vg_serial, jiffies, node->timer);
        else
            printm(MODULE_NAME, "FLUSH JOB ID (%u) TYPE(%x) sn %d version %d\n", node->job_id, node->type, node->serial_num, node->property.vg_serial);
    }

#if GM8210
    if (1) {
        fmem_pci_id_t   pci_id;
        fmem_cpu_id_t   cpu_id;

        fmem_get_identifier(&pci_id, &cpu_id);
        if ((pci_id != FMEM_PCI_HOST) && (cpu_id == FMEM_CPU_FA626)) {
            printm(MODULE_NAME, "ep_process_state: %d \n", ep_process_state);
            scl_dma_dump(MODULE_NAME);
        }
    }
#endif

    printm(MODULE_NAME, "PANIC!! PrintOut End\n");
    return 0;
}

char *wname = "scaler_wq";
/*
 * Entry point of video graph
 */
int fscaler300_vg_init(void)
{
#if GM8210
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
#endif
    int ret = 0;

    fscaler300_entity.minors = max_minors;
    virtual_chn_num = max_vch_num;

    property_record = kzalloc(sizeof(struct property_record_t) * ENTITY_ENGINES * max_minors, GFP_KERNEL);
    if (property_record == NULL)
        panic("%s no memory!\n", __func__);

    /* global information */
    memset(&global_info, 0x0, sizeof(global_info_t));
    INIT_LIST_HEAD(&global_info.node_list);
    INIT_LIST_HEAD(&global_info.node_list_2ddma);
    INIT_LIST_HEAD(&global_info.ep_list);
    INIT_LIST_HEAD(&global_info.ep_rcnode_list);   //EP used to keep the job from cpucomm
    INIT_LIST_HEAD(&global_info.ep_rcnode_cb_list);
    INIT_LIST_HEAD(&global_info.rc_list);

    video_entity_register(&fscaler300_entity);

    /* spinlock */
    spin_lock_init(&global_info.lock);

    /* init semaphore lock */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
    sema_init(&global_info.sema_lock, 1);
    sema_init(&global_info.sema_lock_2ddma, 1);
#else
    init_MUTEX(&global_info.sema_lock);
    init_MUTEX(&global_info.sema_lock_2ddma);
#endif

    /* create memory cache */
    global_info.node_cache = kmem_cache_create("scaler300_vg", sizeof(job_node_t), 0, 0, NULL);
    if (global_info.node_cache == NULL)
        panic("fscaler300: fail to create cache!");
    /* create virtual channel property cache, virtual channel index is from 1 to virtual_ch_num, so allocate (virtual_chn_num + 1) */
    global_info.vproperty_cache = kmem_cache_create("scaler300_vproperty", sizeof(fscaler300_vproperty_t) * (virtual_chn_num + 1), 0, 0, NULL);
    if (global_info.vproperty_cache == NULL)
        panic("fscaler300: fail to create cache 1!");
#ifdef USE_WQ
    /* create workqueue */
    callback_workq = create_singlethread_workqueue("scaler300_callback");
    if (callback_workq == NULL) {
        printk("FSCALER300: error in create callback workqueue! \n");
        return -1;
    }
#endif

#ifdef USE_KTHREAD
    init_waitqueue_head(&cb_thread_wait);
    init_waitqueue_head(&cb_2ddma_thread_wait);

    global_info.cb_task = kthread_create(cb_thread, NULL, "scaler_callback");
    if (IS_ERR(global_info.cb_task))
        panic("%s, create ep_task fail! \n", __func__);
    wake_up_process(global_info.cb_task);

    /* 2ddma callback thread */
    global_info.cb_2ddma_task = kthread_create(cb_2ddma_thread, NULL, "scaler_2ddma_callback");
    if (IS_ERR(global_info.cb_2ddma_task))
        panic("%s, create cb_2ddma_task fail! \n", __func__);
    wake_up_process(global_info.cb_2ddma_task);

#endif

#if GM8210
#ifdef TEST_WQ
    /* create workqueue */
    test_workq = create_workqueue("scaler300_test");
    if (test_workq == NULL) {
        printk("FSCALER300: error in create callback workqueue! \n");
        return -1;
    }

    PREPARE_DELAYED_WORK(&process_test_work,(void *)fscaler300_test_process);
    queue_delayed_work(test_workq, &process_test_work, msecs_to_jiffies(1));
#endif
#endif
#if GM8210
    scaler_trans_init();
#ifdef USE_KTHREAD
	fmem_get_identifier(&pci_id, &cpu_id);
    if ((pci_id != FMEM_PCI_HOST) && (cpu_id == FMEM_CPU_FA626)) {
	    init_waitqueue_head(&eptx_thread_wait);

	    global_info.tx_task = kthread_create(eptx_thread, NULL, "scaler_ep_tx");
        if (IS_ERR(global_info.tx_task))
            panic("%s, create ep_task fail! \n", __func__);
        wake_up_process(global_info.tx_task);
    }
#endif
    fmem_get_identifier(&pci_id, &cpu_id);
    if ((pci_id != FMEM_PCI_HOST) && (cpu_id == FMEM_CPU_FA626)) {
        global_info.ep_task = kthread_create(fscaler300_ep_rx_process, NULL, "scaler_ep_rx");
        if (IS_ERR(global_info.ep_task))
            panic("%s, create ep_task fail! \n", __func__);
        wake_up_process(global_info.ep_task);
    }
#endif /* GM8210 */

    /* register log system callback function */
    ret = register_panic_notifier(fscaler300_log_panic_handler);
    if(ret < 0) {
        printk("scaler300 register log system panic notifier failed!\n");
        return -1;
    }

    ret = register_printout_notifier(fscaler300_log_printout_handler);
    if(ret < 0) {
        printk("scaler300 register log system printout notifier failed!\n");
        return -1;
    }

    /* vg proc init */
    ret = vg_proc_init();
    if(ret < 0)
        scl_err("videograph proc node init failed!\n");
#if GM8210
    /* scl dma init */
    fmem_get_identifier(&pci_id, &cpu_id);
    if ((pci_id == FMEM_PCI_DEV0) && (cpu_id == FMEM_CPU_FA626)) {
        ret = scl_dma_init();
        if (ret < 0)
            scl_err("DUAL core dma channel init fail\n");
    }
    else if ((pci_id == FMEM_PCI_HOST) && (cpu_id == FMEM_CPU_FA626)) {
        if (ftpmu010_get_attr(ATTR_TYPE_EPCNT) > 0) {
            ret = scl_dma_init();
            if (ret < 0)
                scl_err("DUAL core dma channel init fail\n");
            global_info.rc_task = kthread_create(fscaler300_rc_rx_process, NULL, "scaler_rc_rx");
            if (IS_ERR(global_info.rc_task))
                panic("%s, create rc_task fail! \n", __func__);
            wake_up_process(global_info.rc_task);
        }
    }
#endif

    memset(engine_time, 0, sizeof(unsigned int)*SCALER300_ENGINE_NUM);
    memset(engine_start, 0, sizeof(unsigned int)*SCALER300_ENGINE_NUM);
    memset(engine_end, 0, sizeof(unsigned int)*SCALER300_ENGINE_NUM);
    memset(utilization_start, 0, sizeof(unsigned int)*SCALER300_ENGINE_NUM);
    memset(utilization_record, 0, sizeof(unsigned int)*SCALER300_ENGINE_NUM);
    memset(frame_cnt_start, 0, sizeof(int)*SCALER300_ENGINE_NUM);
    memset(frame_cnt_end, 0, sizeof(int)*SCALER300_ENGINE_NUM);
    memset(frame_cnt, 0, sizeof(int)*SCALER300_ENGINE_NUM);
    memset(dummy_cnt, 0, sizeof(int)*SCALER300_ENGINE_NUM);
    memset(dummy_cnt_start, 0, sizeof(int)*SCALER300_ENGINE_NUM);

    return 0;
}

/*
 * Exit point of video graph
 */
void fscaler300_vg_driver_clearnup(void)
{
#ifdef USE_KTHREAD
    if (global_info.cb_task)
        kthread_stop(global_info.cb_task);
    if (global_info.cb_2ddma_task)
        kthread_stop(global_info.cb_2ddma_task);
    if (global_info.tx_task)
        kthread_stop(global_info.tx_task);
    if (global_info.ep_task)
        kthread_stop(global_info.ep_task);
    if (global_info.rc_task)
        kthread_stop(global_info.rc_task);

    while (cb_running == 1)
        msleep(10);
    while (cb_2ddma_running == 1)
        msleep(10);
#if GM8210
    while (ep_running == 1)
        msleep(10);
#endif
#endif
#if GM8210
    scaler_trans_exit();
#endif
    /* cancel all works */
#ifdef USE_WQ
    cancel_delayed_work(&process_callback_work);
#endif
#if GM8210
#ifdef TEST_WQ
    cancel_delayed_work(&process_test_work);
#endif
#endif
    /* scl dma remove */
#if GM8210
    scl_dma_exit();
#endif
    /* vg proc remove */
    vg_proc_remove();

    video_entity_deregister(&fscaler300_entity);
    /* destroy workqueue */
#ifdef USE_WQ
    destroy_workqueue(callback_workq);
#endif
#if GM8210
#ifdef TEST_WQ
    destroy_workqueue(test_workq);
#endif
#endif
    kfree(property_record);
    kmem_cache_destroy(global_info.vproperty_cache);
    kmem_cache_destroy(global_info.node_cache);
}
