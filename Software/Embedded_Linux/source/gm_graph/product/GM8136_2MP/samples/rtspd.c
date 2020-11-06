/**
 * @file rtspd.c
 *  Simple RTSP server demo
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.5 $
 * $Date: 2014/12/30 05:37:57 $
 *
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <dirent.h>


#include "librtsp.h"
#include "gmlib.h"

#define DVR_ENC_EBST_ENABLE     0x55887799
#define DVR_ENC_EBST_DISABLE    0

#define ENC_TYPE_H264           0
#define ENC_TYPE_MPEG4          1
#define ENC_TYPE_MJPEG          2

#define CAP_CH_NUM          1
#define RTSP_NUM_PER_CAP    4
#define CAP_PATH_NUM        4
#define ENC_TRACK_NUM       4

#define SDPSTR_MAX      128
#define SR_MAX          64
#define VQ_MAX          (SR_MAX)
#define VQ_LEN          5
#define AQ_MAX          64            /* 1 MP2 and 1 AMR for live streaming, another 2 for file streaming. */
#define AQ_LEN          2            /* 1 MP2 and 1 AMR for live streaming, another 2 for file streaming. */
#define AV_NAME_MAX     127

#define RTP_HZ          90000

#define ERR_GOTO(x, y)      do { ret = x; goto y; } while(0)
#define MUTEX_FAILED(x)     (x == ERR_MUTEX)
#define VIDEO_FRAME_NUMBER VQ_LEN+1

#define NONE_BS_EVENT    0
#define START_BS_EVENT   1
#define STOP_BS_EVENT    2

#define CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num)    \
    do {    \
        if((ch_num >= CAP_CH_NUM || ch_num < 0) || \
            (sub_num >= RTSP_NUM_PER_CAP || sub_num < 0)) {    \
            fprintf(stderr, "%s: ch_num=%d, sub_num=%d error!\n",__FUNCTION__, ch_num, sub_num);    \
            return -1; \
        }    \
    } while(0)    \

typedef struct {
    void *obj;
    gm_cap_attr_t cap_attr;
    gm_3dnr_attr_t dnr_attr;
} gm_cap_info_t;

typedef struct {
    void *obj;
    int enc_type;
    union {
        gm_h264e_attr_t h264e_attr;
        gm_mpeg4e_attr_t mpeg4e_attr;
        gm_mjpege_attr_t mjpege_attr;
    } codec;
} gm_enc_info_t;

typedef struct {
    gm_cap_info_t cap;
    gm_enc_info_t enc[ENC_TRACK_NUM];
    void *bindfd[ENC_TRACK_NUM];
} gm_enc_t;

void *enc_groupfd;
gm_enc_t enc_param[CAP_CH_NUM][CAP_PATH_NUM];

typedef int (*open_container_fn)(int ch_num, int sub_num);
typedef int (*close_container_fn)(int ch_num, int sub_num);

typedef enum st_opt_type {
    OPT_NONE=0,
    RTSP_LIVE_STREAMING,
} opt_type_t;

typedef struct st_vbs {
    int enabled; //DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
    int enc_type;    // 0:ENC_TYPE_H264, 1:ENC_TYPE_MPEG4, 2:ENC_TYPE_MJPEG
} vbs_t;

typedef struct st_priv_vbs {
    char sdpstr[SDPSTR_MAX];
    int qno;
    int offs;
    int len;
    unsigned int tv_ms;
    int cap_ch;
    int cap_path;
    int rec_track;
    char *bs_buf;
    unsigned int bs_buf_len;
    pthread_mutex_t priv_vbs_mutex;
} priv_vbs_t;

typedef struct st_bs {
    int event; // config change please set 1 for enqueue_thread to config this
    int enabled; //DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
    opt_type_t opt_type;  /* 1:rtsp_live_streaming, 2: file_avi_recording 3:file_h264_recording */
    vbs_t video;  /* VIDEO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
} avbs_t;

typedef struct st_priv_bs {
    int play;
    int congest;
    int sr;
     char name[AV_NAME_MAX];
    open_container_fn open;
    close_container_fn close;
    priv_vbs_t video;  /* VIDEO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
} priv_avbs_t;

typedef struct st_av {
    /* public data */
    avbs_t bs[RTSP_NUM_PER_CAP];  /* VIDEO, 0: main-bitstream, 1: sub1-bitstream, 2:sub2-bitstream */
    /* update date */
    pthread_mutex_t ubs_mutex;

    /* private data */
    int enabled;      //DVR_ENC_EBST_ENABLE: enabled, DVR_ENC_EBST_DISABLE: disabled
    priv_avbs_t priv_bs[RTSP_NUM_PER_CAP];
} av_t;

pthread_t enqueue_thread_id = 0;
pthread_t encode_thread_id = 0;
unsigned int sys_tick = 0;
struct timeval sys_sec = {-1, -1};
int sys_port = 554;
char *ipptr = NULL;
static int rtspd_sysinit=0;
static int rtspd_set_event=0;
static int rtspd_set_1ch=0;
static int rtspd_set_enc_type=0;
static int rtspd_avail_ch=0;

pthread_mutex_t stream_queue_mutex;
av_t enc[CAP_CH_NUM];

gm_system_t gm_system;
void *groupfd;   // return of gm_new_groupfd()
void *bindfd;    // return of gm_bind()
void *capture_object;
void *encode_object;

static char *rtsp_enc_type_str[] = {
    "H264 ",
    "MPEG4",
    "MJPEG"
};

static char getch(void)
{
    int n = 1;
    unsigned char ch;
    struct timeval tv;
    fd_set rfds;

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    n = select(1, &rfds, NULL, NULL, &tv);
    if (n > 0) {
        n = read(0, &ch, 1);
        if (n == 1)
            return ch;
        return n;
    }
    return -1;
}

static int do_queue_alloc(int type)
{
    int    rc;
    do {
        rc = stream_queue_alloc(type);
    } while MUTEX_FAILED(rc);

    return rc;
}

static unsigned int get_tick_gm(unsigned int tv_ms)
{
    sys_tick=tv_ms*(RTP_HZ/1000);
    return sys_tick;
}

static int convert_gmss_media_type(int type)
{
    int media_type;

    switch(type) {
        case ENC_TYPE_H264:
            media_type = GM_SS_TYPE_H264;
            break;
        case ENC_TYPE_MPEG4:
            media_type = GM_SS_TYPE_MP4;
            break;
        case ENC_TYPE_MJPEG:
            media_type = GM_SS_TYPE_MJPEG;
            break;
        default:
            media_type  = -1;
            fprintf(stderr, "convert_gmss_media_type: type=%d, error!\n", type);
            break;
    }
    return media_type;
}

static int open_live_streaming(int ch_num, int sub_num)
{
    int media_type;
    avbs_t *b;
    priv_avbs_t *pb;
    char livename[64];

    CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
    b = &enc[ch_num].bs[sub_num];
    pb = &enc[ch_num].priv_bs[sub_num];
    media_type = convert_gmss_media_type(b->video.enc_type);
    pb->video.qno = do_queue_alloc(media_type);

    sprintf(livename, "live/ch%02d_%d", ch_num, sub_num);
    pb->sr = stream_reg(livename, pb->video.qno, pb->video.sdpstr,
            0, 0, 1, 0, 0, 0, 0, 0, 0);

    if (pb->sr < 0)
        fprintf(stderr, "open_live_streaming: ch_num=%d, sub_num=%d setup error\n", ch_num, sub_num);
    strcpy(pb->name, livename);
    return 0;
}

#define TIMEVAL_DIFF(start, end) (((end.tv_sec)-(start.tv_sec))*1000000+((end.tv_usec)-(start.tv_usec)))
//struct timeval test_st_tv={0,0},test_ed_tv={0,0};
static int write_rtp_frame_ext(int ch_num, int sub_num, void *data,
        int data_len, unsigned int tv_ms) //struct timeval *tv)
{
    int ret, media_type;
    avbs_t *b;
    priv_avbs_t *pb;
    gm_ss_entity entity;
    struct timeval curr_tval; //,test_st_tv={0,0},test_ed_tv={0,0};
    static struct timeval err_print_tval;
    //static unsigned int prv_stamp=0;

    pb = &enc[ch_num].priv_bs[sub_num];
    b = &enc[ch_num].bs[sub_num];

    if( pb->play == 0 || (b->event != NONE_BS_EVENT) )
    {
        ret = 1;
        goto exit_free_as_buf;
    }

    entity.data = (char *) data;
    entity.size = data_len;
    //entity.timestamp = get_tick(NULL );
    entity.timestamp = get_tick_gm(tv_ms);
    media_type = convert_gmss_media_type(b->video.enc_type);
    //gettimeofday(&test_st_tv,NULL);
    pthread_mutex_lock(&stream_queue_mutex);
    ret = stream_media_enqueue(media_type, pb->video.qno, &entity);
    pthread_mutex_unlock(&stream_queue_mutex);
    //gettimeofday(&test_ed_tv,NULL);
    //if(prv_stamp==0)   prv_stamp= entity.timestamp;
    //printf("dstamp=%d\n",entity.timestamp-prv_stamp);
    //prv_stamp= entity.timestamp;
    if( ret < 0 )
    {
        gettimeofday(&curr_tval, NULL );
        if( ret == ERR_FULL)
        {
            pb->congest = 1;
            if( TIMEVAL_DIFF(err_print_tval, curr_tval) > 5000000 )
            {
                fprintf(stderr,
                        "ext enqueue queue ch_num=%d, sub_num=%d full\n",
                        ch_num, sub_num);
            }
        }
        else if((ret != ERR_NOTINIT)&& (ret != ERR_MUTEX) && (ret != ERR_NOTRUN))
        {
            if (TIMEVAL_DIFF(err_print_tval, curr_tval) > 5000000)
            {
                fprintf(stderr, "ext enqueue queue ch_num=%d, sub_num=%d error %d\n",
                        ch_num, sub_num, ret);
            }
        }
        if( TIMEVAL_DIFF(err_print_tval, curr_tval) > 5000000)
        {
            fprintf(stderr,
                    "ext enqueue queue ch_num=%d, sub_num=%d error %d\n",
                    ch_num, sub_num, ret);
            gettimeofday(&err_print_tval, NULL );
        }
        goto exit_free_audio_buf;
    }
    return 0;

exit_free_audio_buf:
    //put_video_frame(pb->video.qno, fs);
exit_free_as_buf:
    //free_bs_data(ch_num, sub_num, q);
    return 1;
}

static int close_live_streaming(int ch_num, int sub_num)
{
    avbs_t *b;
    priv_avbs_t *pb;
    int ret = 0;

    CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
    b = &enc[ch_num].bs[sub_num];
    pb = &enc[ch_num].priv_bs[sub_num];
    if (pb->sr >= 0) {
        ret = stream_dereg(pb->sr, 1);
        if (ret < 0)
            goto err_exit;
        pb->sr = -1;
        pb->video.qno = -1;
        pb->play = 0;
    }

err_exit:
    if(ret < 0)
        fprintf(stderr, "%s: stream_dereg(%d) err %d\n", __func__, pb->sr, ret);
    return ret;
}

int open_bs(int ch_num, int sub_num)
{
    avbs_t *b;
    priv_avbs_t *pb;

    CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
    pb = &enc[ch_num].priv_bs[sub_num];
    b = &enc[ch_num].bs[sub_num];

    enc[ch_num].enabled = DVR_ENC_EBST_ENABLE;
    enc[ch_num].bs[sub_num].enabled = DVR_ENC_EBST_ENABLE;
    enc[ch_num].bs[sub_num].video.enabled = DVR_ENC_EBST_ENABLE;

    switch (b->opt_type) {
        case RTSP_LIVE_STREAMING:
            //set_sdpstr(pb->video.sdpstr, b->video.enc_type);
            pb->open = open_live_streaming;
            pb->close = close_live_streaming;
            break;
        case OPT_NONE:
        default:
            break;
    }
    return 0;
}

int close_bs(int ch_num, int sub_num)
{
    av_t *e;
    int sub, is_close_channel = 1;

    CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
    e = &enc[ch_num];

    e->bs[sub_num].video.enabled = DVR_ENC_EBST_DISABLE;
    e->bs[sub_num].enabled = DVR_ENC_EBST_DISABLE;
    for (sub = 0; sub < RTSP_NUM_PER_CAP; sub++) {
        if (e->bs[sub].video.enabled == DVR_ENC_EBST_ENABLE) {
            is_close_channel = 0;
            break;
        }
    }
    if (is_close_channel == 1)
        enc[ch_num].enabled = DVR_ENC_EBST_DISABLE;
    return 0;
}

static int bs_check_event(void)
{
    int ch_num, sub_num, ret = 0;
    avbs_t *b;

    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        for (sub_num = 0; sub_num<RTSP_NUM_PER_CAP; sub_num++) {
            b = &enc[ch_num].bs[sub_num];
            if (b->event != NONE_BS_EVENT) {
                ret = 1;
                break;
            }
        }
    }
    return ret;
}

void bs_new_event(void)
{
    int ch_num, sub_num;
    avbs_t *b;
    priv_avbs_t *pb;

    if (bs_check_event() == 0) {
        rtspd_set_event = 0;
        return;
    }

    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        pthread_mutex_lock(&enc[ch_num].ubs_mutex);
        for (sub_num = 0; sub_num < RTSP_NUM_PER_CAP; sub_num++) {
            b = &enc[ch_num].bs[sub_num];
            pb = &enc[ch_num].priv_bs[sub_num];
            switch (b->event) {
                case START_BS_EVENT:
                    open_bs(ch_num, sub_num);
                    if(pb->open) pb->open(ch_num, sub_num);
                    b->event = NONE_BS_EVENT;
                    break;
                case STOP_BS_EVENT:
                    pb->open = NULL;
                    if(pb->close) {  /* for recording */
                        pb->close(ch_num, sub_num);
                        pb->close = NULL;
                        close_bs(ch_num, sub_num);
                    }
                    b->event = NONE_BS_EVENT;
                    break;
                default:
                    break;
            }
        }
        pthread_mutex_unlock(&enc[ch_num].ubs_mutex);
    }
}

int env_set_bs_new_event(int ch_num, int sub_num, int event)
{
    avbs_t *b;
    int ret = 0;

    CHECK_CHANNUM_AND_SUBNUM(ch_num, sub_num);
    b = &enc[ch_num].bs[sub_num];

    switch (event) {
        case START_BS_EVENT:
            if (b->opt_type == OPT_NONE)
                goto err_exit;
            if (b->enabled == DVR_ENC_EBST_ENABLE) {
                fprintf(stderr, "Already enabled: ch_num=%d, sub_num=%d\n", ch_num, sub_num);
                ret = -1;
                goto err_exit;
            }
            break;
        case STOP_BS_EVENT:
            if (b->enabled != DVR_ENC_EBST_ENABLE) {
                fprintf(stderr, "Already disabled: ch_num=%d, sub_num=%d\n", ch_num, sub_num);
                ret = -1;
                goto err_exit;
            }
            break;
        default:
            fprintf(stderr, "env_set_bs_new_event: ch_num=%d, sub_num=%d, event=%d, error\n",
                            ch_num, sub_num, event);
            ret = -1;
            goto err_exit;
    }
    b->event = event;
    rtspd_set_event = 1;

err_exit:
    return ret;
}

int set_poll_event(void)
{
    int ch_num, sub_num, ret = -1;
    av_t *e;
    avbs_t *b;

    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        e = &enc[ch_num];
        if (e->enabled != DVR_ENC_EBST_ENABLE)
            continue;
        for (sub_num = 0; sub_num < RTSP_NUM_PER_CAP; sub_num++) {
            b = &e->bs[sub_num];
            if (b->video.enabled == DVR_ENC_EBST_ENABLE) {
                ret = 0;
            }
        }
    }
    return ret;
}

void get_enc_res(gm_enc_info_t *enc, int *enc_type, int *width, int *height)
{
    int w, h;
    gm_h264e_attr_t *h264e_attr;
    gm_mpeg4e_attr_t *mpeg4e_attr;
    gm_mjpege_attr_t *mjpege_attr;
    
    switch (enc->enc_type) {
        case ENC_TYPE_H264:
            h264e_attr = &enc->codec.h264e_attr;
            w = h264e_attr->dim.width;
            h = h264e_attr->dim.height;
            break;
        case ENC_TYPE_MPEG4:
            mpeg4e_attr = &enc->codec.mpeg4e_attr;
            w = mpeg4e_attr->dim.width;
            h = mpeg4e_attr->dim.height;
            break;
        case ENC_TYPE_MJPEG:
            mjpege_attr = &enc->codec.mjpege_attr;
            w = mjpege_attr->dim.width;
            h = mjpege_attr->dim.height;
            break;
    }
    if (enc_type)
        *enc_type = enc->enc_type;
    if (width)
        *width = w;
    if (height)
        *height = h;
}

#define PRINT_INTERVAL_MS    5000    /* millisecond */
static unsigned int frame_counts[CAP_CH_NUM][RTSP_NUM_PER_CAP] = {{0}};
static unsigned int rec_bs_len[CAP_CH_NUM][RTSP_NUM_PER_CAP] = {{0}};
static void print_enc_average(int ch_num, int sub_num, int bs_len, struct timeval *cur_timeval)
{
    int enc_type, w, h;
    static struct timeval last_timeval;
    static unsigned int total_ms, print_init = 0;
    unsigned int diff_ms, i, j;
    char res_str[20];
    priv_avbs_t *pb;
    gm_enc_info_t *gm_enc;

    frame_counts[ch_num][sub_num]++;
    rec_bs_len[ch_num][sub_num] += bs_len;

    if (print_init == 0) {
        last_timeval.tv_sec = cur_timeval->tv_sec;
        last_timeval.tv_usec = cur_timeval->tv_usec;
        print_init = 1;
        total_ms = 0;
        return;
    }

    /* get diff time */
    if (cur_timeval->tv_sec > last_timeval.tv_sec) {
        diff_ms = 1000 + (cur_timeval->tv_usec / 1000) - (last_timeval.tv_usec / 1000);
        diff_ms += (cur_timeval->tv_sec - last_timeval.tv_sec - 1) * 1000;
    } else {
        diff_ms = (cur_timeval->tv_usec - last_timeval.tv_usec) / 1000;
    }
    total_ms += diff_ms;

    /* show statistic */
    if (total_ms >= PRINT_INTERVAL_MS) {  
        for (i = 0; i < CAP_CH_NUM; i++) {
            for (j = 0; j < RTSP_NUM_PER_CAP; j++) {
                if (frame_counts[i][j] == 0)
                    continue;
                pb = &enc[i].priv_bs[j];
                gm_enc = &enc_param[pb->video.cap_ch][pb->video.cap_path].enc[pb->video.rec_track];
                get_enc_res(gm_enc, &enc_type, &w, &h);
                sprintf(res_str, "%dx%d", w, h);
                printf("/live/ch%02d_%d: cap%d_%d %9s %s %d.%02dfps %dkbps\n", i, j,
                            pb->video.cap_ch, pb->video.cap_path,
                            res_str,
                            rtsp_enc_type_str[enc_type],
                            (frame_counts[i][j] * 1000 / total_ms),
                            (frame_counts[i][j] * 100000 / total_ms) % 100,
                            (rec_bs_len[i][j] * 8 / 1024) * 1000 / total_ms);
                frame_counts[i][j] = 0;
                rec_bs_len[i][j] = 0;
            }
        }
        printf("\n");
        total_ms = 0;
    }
    last_timeval.tv_sec = cur_timeval->tv_sec;
    last_timeval.tv_usec = cur_timeval->tv_usec;
}

#define POLL_WAIT_TIME 15000 /* microseconds */
static unsigned int poll_wait_time = 0;


static void env_release_resources(void)
{
    int ret, ch_num;
    av_t *e;

    if ((ret = stream_server_stop()))
        fprintf(stderr, "stream_server_stop() error %d\n", ret);
    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        e = &enc[ch_num];
        pthread_mutex_destroy(&e->ubs_mutex);
    }
}

static int frm_cb(int type, int qno, gm_ss_entity *entity)
{
    priv_avbs_t *pb;
    int ch_num, sub_num;
    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        for (sub_num = 0; sub_num < RTSP_NUM_PER_CAP; sub_num++) {    	       
            pb = &enc[ch_num].priv_bs[sub_num];
            if (pb->video.offs == (int)(entity->data)
                    && pb->video.len == entity->size && pb->video.qno==qno) {
                pthread_mutex_lock(&pb->video.priv_vbs_mutex);
                pb->video.offs = 0;
                pb->video.len = 0;
                pthread_mutex_unlock(&pb->video.priv_vbs_mutex);
            }
        }
    }
    return 0;
}

priv_avbs_t *find_file_sr(char *name, int srno)
{
    int ch_num, sub_num, hit=0;
    priv_avbs_t *pb;

    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        for (sub_num = 0; sub_num < RTSP_NUM_PER_CAP; sub_num++) {
            pb = &enc[ch_num].priv_bs[sub_num];
            if ((pb->sr == srno) && (pb->name) && (strcmp(pb->name, name) == 0)) {
                hit = 1;
                break;
            }
        }
        if (hit)
            break;
    }
    return (hit ? pb : NULL);
}

static int cmd_cb(char *name, int sno, int cmd, void *p)
{
    int ret = -1;
    priv_avbs_t *pb;

    switch(cmd)
    {
        case GM_STREAM_CMD_OPTION:
            ret = 0;
            break;
        case GM_STREAM_CMD_DESCRIBE:
            ret = 0;
            break;
        case GM_STREAM_CMD_OPEN:
            printf("%s:%d <GM_STREAM_CMD_OPEN>\n", __FUNCTION__, __LINE__);
            ERR_GOTO(-10, cmd_cb_err);
            break;
        case GM_STREAM_CMD_SETUP:
            ret = 0;
            break;
        case GM_STREAM_CMD_PLAY:
            if( strncmp(name, "live/", 5) == 0 ) {
                if ((pb = find_file_sr(name, sno)) == NULL)
                    ERR_GOTO(-1, cmd_cb_err);
                if (pb->video.qno >= 0)
                    pb->play = 1;
            }
            ret = 0;
            break;
        case GM_STREAM_CMD_PAUSE:
            printf("%s:%d <GM_STREAM_CMD_PAUSE>\n", __FUNCTION__, __LINE__);
            ret = 0;
            break;
        case GM_STREAM_CMD_TEARDOWN:
            if( strncmp(name, "live/", 5) == 0 ) {
                if ((pb = find_file_sr(name, sno)) == NULL)
                    ERR_GOTO(-1, cmd_cb_err);
                pb->play = 0;
            }
            ret = 0;
            break;
        default:
            fprintf(stderr, "%s: not support cmd %d\n", __func__, cmd);
            break;
    }

cmd_cb_err:
    if( ret < 0 ) {
        fprintf(stderr, "%s: cmd %d error %d\n", __func__, cmd, ret);
    }
    return ret;
}

void *enqueue_thread(void *ptr)
{
    while (rtspd_sysinit) {
        if (rtspd_set_event)
            bs_new_event();
        if (set_poll_event() < 0) {
            sleep(1);
            continue;
        }
        usleep(1000);
    }
    env_release_resources();
    pthread_exit(NULL);
    return NULL;
}

void gm_update_bs_info(void)
{
	int cap_ch,cap_path,rec_track;
	int ch=0;
 	avbs_t *avbs;
 	gm_enc_t *param;
 	
    for (cap_ch = 0; cap_ch < CAP_CH_NUM; cap_ch++) {
        for (cap_path = 0; cap_path < CAP_PATH_NUM; cap_path++) {
            param = &enc_param[cap_ch][cap_path];
            for (rec_track = 0; rec_track < ENC_TRACK_NUM; rec_track++) {
                if (param->bindfd[rec_track]) {
                    avbs = &enc[cap_ch].bs[ch];
					avbs->video.enc_type = param->enc[rec_track].enc_type;
                    ch++;
                }
            }
        }
    }
}

int env_init(void)
{
    int ret = 0;
    int ch_num, sub_num;
    av_t *e;
    avbs_t *b;
    priv_avbs_t *pb;

    memset(enc,0,sizeof(enc));

    /* private data initial */
    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        e = &enc[ch_num];
        if (pthread_mutex_init(&e->ubs_mutex, NULL)) {
            perror("env_init: mutex init failed:");
            exit(-1);
        }
        for (sub_num = 0; sub_num < RTSP_NUM_PER_CAP; sub_num++) {
            b = &e->bs[sub_num];
            b->opt_type = RTSP_LIVE_STREAMING;
            b->video.enc_type = ENC_TYPE_H264;
            b->event = NONE_BS_EVENT;
            b->enabled = DVR_ENC_EBST_DISABLE;
            b->video.enabled = DVR_ENC_EBST_DISABLE;

            pb = &e->priv_bs[sub_num];
            pb->video.qno = -1;
            pb->video.offs=0;
            pb->video.len=0;
            pb->sr = -1;
            if (pthread_mutex_init(&pb->video.priv_vbs_mutex, NULL)) {
                perror("env_enc_init: priv_vbs mutex init failed:");
                exit(-1);
            }
        }
    }
	//update bs info from decoder
	gm_update_bs_info();

    srand((unsigned int)time(NULL));
    if ((ret = stream_server_init(ipptr, (int) sys_port, 0, 1444, 256, SR_MAX, VQ_MAX, VQ_LEN,
                                  AQ_MAX, AQ_LEN, frm_cb, cmd_cb)) < 0)
        fprintf(stderr, "stream_server_init, ret %d\n", ret);
    if ((ret = stream_server_start()) < 0)
        fprintf(stderr, "stream_server_start, ret %d\n", ret);
    return ret;
}

static unsigned chipid;
void gm_enc_init(int cap_ch, int cap_path, int rec_track, int enc_type, int mode, int framerate,
                  int bitrate, int width, int height)
{
    gm_enc_t *param;
    DECLARE_ATTR(cap_attr, gm_cap_attr_t);
    DECLARE_ATTR(h264e_attr, gm_h264e_attr_t);
    DECLARE_ATTR(mpeg4e_attr, gm_mpeg4e_attr_t);
    DECLARE_ATTR(dnr_attr, gm_3dnr_attr_t);
    DECLARE_ATTR(mjpege_attr, gm_mjpege_attr_t);

    param = &enc_param[cap_ch][cap_path];
    if (param->cap.obj == NULL) {
        param->cap.obj = gm_new_obj(GM_CAP_OBJECT); // new capture object
        cap_attr.cap_vch = cap_ch;

        //GM813x capture path 0(liveview), 1(substream), 2(substream), 3(mainstream)
        cap_attr.path = cap_path;
        cap_attr.enable_mv_data = 0;
        gm_set_attr(param->cap.obj, &cap_attr); // set capture attribute

        //enable 3dnr if resolution > capture dim / 2 
        if ((width >= (gm_system.cap[0].dim.width / 2)) &&
            (height >= (gm_system.cap[0].dim.height / 2))) {
            dnr_attr.enabled = 1;
            gm_set_attr(param->cap.obj, &dnr_attr);
        }
        memcpy(&param->cap.cap_attr, &cap_attr, sizeof(gm_cap_attr_t));
        memcpy(&param->cap.dnr_attr, &dnr_attr, sizeof(gm_3dnr_attr_t));
    }
    
    param->enc[rec_track].obj = gm_new_obj(GM_ENCODER_OBJECT); // new encoder object 
    param->enc[rec_track].enc_type = enc_type;
    switch (enc_type) {
        case ENC_TYPE_H264:
            h264e_attr.dim.width = width;
            h264e_attr.dim.height = height;
            h264e_attr.frame_info.framerate = framerate;
            h264e_attr.ratectl.mode = mode;
            h264e_attr.ratectl.gop = 60;
            h264e_attr.ratectl.bitrate = bitrate;  
            h264e_attr.ratectl.bitrate_max = bitrate; 
            h264e_attr.b_frame_num = 0;  // B-frames per GOP (H.264 high profile)
            h264e_attr.enable_mv_data = 0;  // disable H.264 motion data output
            h264e_attr.ratectl.init_quant = 25;
            h264e_attr.ratectl.min_quant = 20;
            h264e_attr.ratectl.max_quant = 51;
            gm_set_attr(param->enc[rec_track].obj, &h264e_attr);
            memcpy(&param->enc[rec_track].codec.h264e_attr, &h264e_attr, sizeof(gm_h264e_attr_t));
            break;
        case ENC_TYPE_MPEG4:
            mpeg4e_attr.dim.width = width;
            mpeg4e_attr.dim.height = height;
            mpeg4e_attr.frame_info.framerate = framerate;
            mpeg4e_attr.ratectl.mode = mode;
            mpeg4e_attr.ratectl.gop = 60;
            mpeg4e_attr.ratectl.bitrate = bitrate;  
            mpeg4e_attr.ratectl.bitrate_max = bitrate; 
            gm_set_attr(param->enc[rec_track].obj, &mpeg4e_attr);
            memcpy(&param->enc[rec_track].codec.mpeg4e_attr, &mpeg4e_attr, sizeof(gm_mpeg4e_attr_t));
            break;
        case ENC_TYPE_MJPEG:
            mjpege_attr.dim.width = width;
            mjpege_attr.dim.height = height;
            mjpege_attr.frame_info.framerate = framerate;
            mjpege_attr.quality = 30;
            gm_set_attr(param->enc[rec_track].obj, &mjpege_attr);
            memcpy(&param->enc[rec_track].codec.mjpege_attr, &mjpege_attr, sizeof(gm_mjpege_attr_t));
            break;
        default: 
            printf("Not support %s\n", rtsp_enc_type_str[enc_type]);
            break;
    }
    // bind channel recording 
    param->bindfd[rec_track] = gm_bind(enc_groupfd, param->cap.obj, param->enc[rec_track].obj);
    rtspd_avail_ch++;
}

int gm_get_chipinfo(void)
{
    FILE *fp;
    char buffer[256];
    int i;
    int chipid;
    char *match;
	
    fp = fopen("/proc/pmu/chipver","r");
    i = fread(buffer,1,sizeof(buffer),fp);
    fclose(fp);
    if (i == 0)
        return 0;
    buffer[i] = '\0';
    match = strstr(buffer, "81");
    if (match == NULL)
        return 0;
    sscanf(match,"%X",&chipid);
    return chipid;
}

static int gm_get_max_bandwidth(char *list)
{
    int bandwidth=0;
    int i;
    int ch;
    int tmp;
    char token[] = "\n \t";
    int sensor_res[] = {800,500,400,300,200,130,100,34,30,7};
    char *str;
    str = strtok(list,token);
    
    for (i=0;i<5;i++) {
        if (str == NULL)
            break;
		sscanf(str,"%02d",&ch);
		str+=3;
		sscanf(str,"%03d",&tmp);
		str = strtok(NULL,token);
        if (ch == 0 || tmp == 0)
             continue;
        tmp = tmp * sensor_res[i];
        tmp = tmp / ch;
        if (i <= 3) {
		    bandwidth = 8*30;
            break;
		}
		if (bandwidth <= (tmp /100))
            bandwidth = tmp/100;
    }
    return bandwidth;
}

int gm_get_bandwidth_info(void) // 0:2m,1:8m
{
    FILE *fp;
    char buffer[2048];
    int i;
    char *match;
	
    fp = fopen("/proc/videograph/vpd/spec_info","r");
    i = fread(buffer,1,sizeof(buffer),fp);
    fclose(fp);
    if (i == 0)
        buffer[i] = '\0';
    match = strstr(buffer,"[ENC CAPTURE]");
    if (match == NULL)
        return 0;
    match = strstr(match,"CH_0");
    if (match == NULL)
        return 0;
    match += 4;
    sscanf(match,"%X",&i);
    if (i == 0) 
        return 0;
        
    if (gm_get_max_bandwidth((match+1)) > 62) 
        return 1;
    else 
        return 0;
}

void gm_graph_init(void)
{
    int cap_fps;
    int cap_h;
    int cap_w;
    int cap_resolution;
    int cap_bandwidth;
    //int chipid;

    gm_init(); //gmlib initial
    gm_get_sysinfo(&gm_system);
    if (gm_system.cap[0].framerate) 
        poll_wait_time = 1000000 / (gm_system.cap[0].framerate + 2);
    else 
        poll_wait_time = 15000;  //microsecond
    cap_fps = gm_system.cap[0].framerate;
    cap_h = gm_system.cap[0].dim.height;
    cap_w = gm_system.cap[0].dim.width;
    cap_bandwidth = cap_fps * cap_h * cap_w;
    cap_resolution = cap_h * cap_w;
	
    memset(enc_param, 0, sizeof(enc_param));
    enc_groupfd = gm_new_groupfd();
    chipid = gm_get_chipinfo();
    chipid = (chipid >> 16) & 0x0000ffff;
    if (gm_get_bandwidth_info() == 1)
    	rtspd_set_1ch = 1;	
    rtspd_avail_ch = 0;

    if (rtspd_set_1ch == 1 && rtspd_set_enc_type == 0) {//single chaneel ,h264 case
        gm_enc_init(0, 3, 0, ENC_TYPE_H264, GM_CBR, cap_fps, 8192,
            gm_system.cap[0].dim.width, gm_system.cap[0].dim.height);   
    } else if (rtspd_set_1ch == 1 && rtspd_set_enc_type == 1) {
        //single chaneel ,mpeg4 case,mpeg4 max_fps=10 bs=2048
        gm_enc_init(0, 3, 0, ENC_TYPE_MPEG4, GM_CBR, 10, 2048,
            gm_system.cap[0].dim.width, gm_system.cap[0].dim.height);   
    } else if (rtspd_set_1ch == 1 && rtspd_set_enc_type == 2) {//single chaneel ,mjpeg case
        gm_enc_init(0, 3, 0, ENC_TYPE_MJPEG, GM_CBR, cap_fps, 8192,
            gm_system.cap[0].dim.width, gm_system.cap[0].dim.height);
    } else {    //mult-channel only support h264
        if (cap_resolution > (1920*1088)) {
        	gm_enc_init(0, 3, 0, ENC_TYPE_H264, GM_CBR, cap_fps, 8192,
        	    gm_system.cap[0].dim.width, gm_system.cap[0].dim.height);
        } else if (cap_bandwidth > (1920*1088*30)) {
        	gm_enc_init(0, 3, 0, ENC_TYPE_H264, GM_CBR, cap_fps, 8192,
        	    gm_system.cap[0].dim.width, gm_system.cap[0].dim.height);
        } else {
    	    if ((chipid == 0x8136) || (chipid == 0x8135)) {
    	        gm_enc_init(0, 0, 0, ENC_TYPE_H264, GM_CBR, cap_fps, 256, 320, 240);
    	        gm_enc_init(0, 0, 1, ENC_TYPE_H264, GM_CBR, cap_fps, 2048, 640, 480);
    	        gm_enc_init(0, 3, 0, ENC_TYPE_H264, GM_CBR, cap_fps, 4096,
    	            gm_system.cap[0].dim.width, gm_system.cap[0].dim.height);
    	    } else if (chipid == 0x8137) {
    	        gm_enc_init(0, 0, 0, ENC_TYPE_H264, GM_CBR, cap_fps, 128, 320, 240);
    	        gm_enc_init(0, 0, 1, ENC_TYPE_H264, GM_CBR, cap_fps, 512, 640, 480);
    	        gm_enc_init(0, 3, 0, ENC_TYPE_H264, GM_CBR, cap_fps, 4096,
    	            gm_system.cap[0].dim.width,gm_system.cap[0].dim.height);
    	    } else if (chipid == 0x8138) {
    	        gm_enc_init(0, 0, 0, ENC_TYPE_H264, GM_CBR, cap_fps, 128, 320, 240);
    	        gm_enc_init(0, 0, 1, ENC_TYPE_H264, GM_CBR, cap_fps, 2048, 1280, 720);
    	        gm_enc_init(0, 3, 0, ENC_TYPE_H264, GM_CBR, cap_fps, 4096,
    	            gm_system.cap[0].dim.width,gm_system.cap[0].dim.height);
    	    } else {
    	        gm_enc_init(0, 0, 0, ENC_TYPE_H264, GM_CBR, cap_fps, 128, 320, 240);
    	        gm_enc_init(0, 0, 1, ENC_TYPE_H264, GM_CBR, cap_fps, 2048, 1280, 720);
    	        gm_enc_init(0, 3, 0, ENC_TYPE_H264, GM_CBR, cap_fps, 4096,
    	            gm_system.cap[0].dim.width,gm_system.cap[0].dim.height);
    	    }
    	}
    }
    gm_apply(enc_groupfd); // active setting 
}

void gm_graph_release(void)
{
    gm_enc_t *param;
    int cap_ch, cap_path, rec_track; 

    for (cap_ch = 0; cap_ch < CAP_CH_NUM; cap_ch++) {
        for (cap_path = 0; cap_path < CAP_PATH_NUM; cap_path++) {
            param = &enc_param[cap_ch][cap_path];
            for (rec_track = 0; rec_track < ENC_TRACK_NUM; rec_track++) {
                if (param->bindfd[rec_track])
                    gm_unbind(param->bindfd[rec_track]);
            }
        }
    }
    gm_apply(enc_groupfd);

    for (cap_ch = 0; cap_ch < CAP_CH_NUM; cap_ch++) {
        for (cap_path = 0; cap_path < CAP_PATH_NUM; cap_path++) {
            param = &enc_param[cap_ch][cap_path];
            for (rec_track = 0; rec_track < ENC_TRACK_NUM; rec_track++) {
                if (param->enc[rec_track].obj)
                    gm_delete_obj(param->enc[rec_track].obj);
            }
            if (param->cap.obj)
                gm_delete_obj(param->cap.obj);
        }
    }
    gm_delete_groupfd(enc_groupfd);
    gm_release();
}

void *encode_thread(void *ptr)
{
    int i, j, ch = 0, ret, cap_ch, cap_path, rec_track, rcv_nr, w, h;
    int first_play[CAP_CH_NUM][RTSP_NUM_PER_CAP];
    priv_avbs_t *pb;
    avbs_t *avbs;
    gm_enc_multi_bitstream_t bs[CAP_CH_NUM][RTSP_NUM_PER_CAP];
    gm_pollfd_t poll_fds[CAP_CH_NUM][RTSP_NUM_PER_CAP];
    gm_enc_t *param;
    static struct timeval prev;
    struct timeval cur, tout;
    static int timeval_init = 0;
    int diff;
    
    memset(poll_fds, 0, sizeof(poll_fds));

    for (cap_ch = 0; cap_ch < CAP_CH_NUM; cap_ch++) {
        for (cap_path = 0; cap_path < CAP_PATH_NUM; cap_path++) {
            param = &enc_param[cap_ch][cap_path];
            for (rec_track = 0; rec_track < ENC_TRACK_NUM; rec_track++) {
                if (param->bindfd[rec_track]) {
                    poll_fds[cap_ch][ch].bindfd = param->bindfd[rec_track];
                    poll_fds[cap_ch][ch].event = GM_POLL_READ;
                    
                    avbs = &enc[cap_ch].bs[ch];
                    get_enc_res(&param->enc[rec_track], NULL, &w, &h);
                    pb = &enc[cap_ch].priv_bs[ch];
                    pb->video.bs_buf_len = w * h * 3 / 2;
                    pb->video.bs_buf = malloc(pb->video.bs_buf_len);
                    pb->video.cap_ch = cap_ch;
                    pb->video.cap_path = cap_path;
                    pb->video.rec_track = rec_track;
                    ch++;
                }
            }
        }
    }

    while(1) {
        if (rtspd_sysinit == 0)
            break;

        if (rtspd_set_event) {
            usleep(2000);//sleep(1);
            continue;
        }

        if (set_poll_event() < 0) {
            usleep(2000);//sleep(1);
            continue;
        }
        gettimeofday(&cur, NULL);
        if (timeval_init == 0) {
            timeval_init = 1;
            tout.tv_sec = 0;
            tout.tv_usec = poll_wait_time;
        } else {
            diff = (cur.tv_usec < prev.tv_usec) ?
                (cur.tv_usec+1000000-prev.tv_usec) : (cur.tv_usec-prev.tv_usec);
            tout.tv_usec = (diff > poll_wait_time) ? (tout.tv_usec = 0) : (poll_wait_time - diff);
        }
        usleep(tout.tv_usec);
        gettimeofday(&prev, NULL);
        ret=0;
        if (rtspd_sysinit == 0)
			break;
        ret = gm_poll(&poll_fds[0][0], CAP_CH_NUM * RTSP_NUM_PER_CAP, 2000);
	    if (ret == GM_TIMEOUT) {
	    	printf("Poll timeout!!");
	        continue;
	    }
        rcv_nr = 0;
        memset(bs, 0, sizeof(bs));
        for (i = 0; i < CAP_CH_NUM; i++) {
            for (j = 0; j < RTSP_NUM_PER_CAP; j++) {
                pb = &enc[i].priv_bs[j];
                if (pb->video.offs || pb->video.len) 
                    continue;
                if (poll_fds[i][j].revent.event != GM_POLL_READ)
                    continue;
                if (poll_fds[i][j].revent.bs_len > pb->video.bs_buf_len) {
                    printf("%d_%d: bindfd(%p) bitstream buffer length is not enough! "
                           "(%d_bytes vs %d_bytes)\n", i, j, poll_fds[i][j].bindfd, 
                           poll_fds[i][j].revent.bs_len, pb->video.bs_buf_len);
                    continue;
                }
                rcv_nr++;
                bs[i][j].bindfd = poll_fds[i][j].bindfd;
                bs[i][j].bs.bs_buf = pb->video.bs_buf; // set buffer point
                bs[i][j].bs.bs_buf_len = pb->video.bs_buf_len; // set buffer length
                bs[i][j].bs.mv_buf = 0;  // not to recevie MV data
                bs[i][j].bs.mv_buf_len = 0;  // not to recevie MV data
                if(pb->play == 0)
                	first_play[i][j] = -1;
            }
        }
        if (rcv_nr == 0) 
            continue;
		
		if(rtspd_sysinit == 0)
    		break;

        if( (ret = gm_recv_multi_bitstreams(&bs[0][0],CAP_CH_NUM * RTSP_NUM_PER_CAP)) < 0 ) { 
            // <=-1:fail, 0:success
            printf("Error to receive bitstream. ret(%d)\n", ret);
            continue;  // while(1) {
        }
	            
        for (i = 0; i < CAP_CH_NUM; i++) {
            for (j = 0; j < RTSP_NUM_PER_CAP; j++) {
				if (rtspd_sysinit == 0)
    				continue;

                pb = &enc[i].priv_bs[j];
                avbs = &enc[i].bs[j];
                if ((bs[i][j].retval < 0) && bs[i][j].bindfd) 
                    printf("CH%2d_%d Error to receive bitstream. ret=%d\n", i, j, bs[i][j].retval);
                else if (bs[i][j].retval == GM_SUCCESS) {
                	if (avbs->video.enc_type != ENC_TYPE_MJPEG) {
                		if ((pb->play == 1) && (bs[i][j].bs.keyframe ==1))
                			first_play[i][j] = 1;
                	}
                	else
                		first_play[i][j] = 1;
                	if (first_play[i][j] == 1) {
						pthread_mutex_lock(&pb->video.priv_vbs_mutex);                		
						pb->video.offs = (int) (bs[i][j].bs.bs_buf);
                        pb->video.len = bs[i][j].bs.bs_len;
                        pb->video.tv_ms = bs[i][j].bs.timestamp;
                        pthread_mutex_unlock(&pb->video.priv_vbs_mutex);
	                	if (write_rtp_frame_ext(i, j, (void *)pb->video.offs, pb->video.len,bs[i][j].bs.timestamp) == 1) {
	    	            	pb->video.offs = (int)NULL;
                        	pb->video.len = 0;
                        }
	                }
	                print_enc_average(i, j, bs[i][j].bs.bs_len, &prev);
	            }
            }
        }     
    }

    pthread_exit(NULL);
    encode_thread_id = (pthread_t)NULL;
    return NULL;
}

void update_video_sdp(int cap_ch, int cap_path, int rec_track)
{
    char *bitstream_data = NULL;
    unsigned int bitstream_data_len;
    gm_enc_multi_bitstream_t bs;
    gm_pollfd_t poll_fds;
    int ret, cnt=0, w, h;
    gm_enc_t *param;
    priv_avbs_t *pb;

    memset(&poll_fds, 0, sizeof(poll_fds));
    param = &enc_param[cap_ch][cap_path];

    if (param->bindfd[rec_track]) {
        poll_fds.bindfd = param->bindfd[rec_track];
        poll_fds.event = GM_POLL_READ;
        get_enc_res(&param->enc[rec_track], NULL, &w, &h);
        pb = &enc[cap_ch].priv_bs[cap_path];
        bitstream_data_len = w * h * 3 / 2;
        bitstream_data = malloc(bitstream_data_len);
    }
    else
    {
        return;
    }

    while(1)
    {
        ret = gm_poll(&poll_fds, 1, 2000);
        if( ret == GM_TIMEOUT )
        {
            printf("Poll timeout!!");
            continue;
        }
        memset(&bs, 0, sizeof(bs));
        if( poll_fds.revent.event != GM_POLL_READ )
            continue;
        if( poll_fds.revent.bs_len > bitstream_data_len)
        {
            printf("bitstream buffer length is not enough! %d, %d\n",
                    poll_fds.revent.bs_len, bitstream_data_len);
            continue;
        }
        bs.bindfd = poll_fds.bindfd;
        bs.bs.bs_buf = bitstream_data;              // set buffer point
        bs.bs.bs_buf_len = bitstream_data_len;      // set buffer length
        bs.bs.mv_buf = 0;                           // not to recevie MV data
        bs.bs.mv_buf_len = 0;                       // not to recevie MV data

        ret = gm_recv_multi_bitstreams(&bs, 1); // -1:fail 0:scuess
        if( ret < 0 )
            printf("Error to receive bitstream.\n");
        else if( (bs.retval < 0) && bs.bindfd )
        {
            printf("CH0 Error to receive bitstream. ret=%d\n", bs.retval);
        }
        else if( ret == 0 && bs.retval == GM_SUCCESS )
        {
            if(bs.bs.keyframe == 1 )
            {
                switch (rtspd_set_enc_type)
                {
                case 0:
                    stream_sdp_parameter_encoder("H264",
                                                 (unsigned char *) bs.bs.bs_buf,
                                                 bs.bs.bs_len,
                                                 pb->video.sdpstr,
                                                 SDPSTR_MAX);
                case 1:
                    stream_sdp_parameter_encoder("H264",
                                                 (unsigned char *) bs.bs.bs_buf,
                                                 bs.bs.bs_len,
                                                 pb->video.sdpstr,
                                                 SDPSTR_MAX);
                case 2:
                    stream_sdp_parameter_encoder("H264",
                                                 (unsigned char *) bs.bs.bs_buf,
                                                 bs.bs.bs_len,
                                                 pb->video.sdpstr,
                                                 SDPSTR_MAX);
                }
                break;
            }
            else
            {
                if(++cnt>100)
                {
                    printf("wait key frame fail\n");
                    break;
                }
            }
        }
    }
    if (bitstream_data)
        free(bitstream_data);
}

static int rtspd_start(int port)
{
    int ret, ch_num, stream;
    pthread_attr_t attr;

    if(rtspd_sysinit == 1)
        return -1;
    if ((0 < port) && (port < 0x10000))
        sys_port = port;

    if ((ret = env_init()) < 0)
        return ret;

    if (pthread_mutex_init(&stream_queue_mutex, NULL)) {
        perror("rtspd_start: mutex init failed:");
        exit(-1);
    }

    rtspd_sysinit = 1;

    /* Record Thread */
    if (encode_thread_id == (pthread_t)NULL) {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        ret = pthread_create(&encode_thread_id, &attr, &encode_thread, NULL);
        pthread_attr_destroy(&attr);
    }

    if (enqueue_thread_id == (pthread_t)NULL) {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        ret = pthread_create(&enqueue_thread_id, &attr, &enqueue_thread, NULL);
        pthread_attr_destroy(&attr);
    }

    for (ch_num = 0; ch_num < CAP_CH_NUM; ch_num++) {
        pthread_mutex_lock(&enc[ch_num].ubs_mutex);
        for (stream = 0; stream < RTSP_NUM_PER_CAP; stream++)
            env_set_bs_new_event(ch_num, stream, START_BS_EVENT);
        pthread_mutex_unlock(&enc[ch_num].ubs_mutex);
    }
    return 0;
}

int is_bs_all_disable(void)
{
    av_t *e;
    int ch_num, sub_num;

    for (ch_num=0; ch_num < CAP_CH_NUM; ch_num++) {
        e = &enc[ch_num];
        for(sub_num=0; sub_num < RTSP_NUM_PER_CAP; sub_num++) {
            if(e->bs[sub_num].enabled == DVR_ENC_EBST_ENABLE)
                return 0;  /* already enabled */
        }
    }
    return 1;
}

static void rtspd_stop()
{
    pthread_mutex_destroy(&stream_queue_mutex);
    rtspd_sysinit = 0;
}

char *get_local_ip(void)
{
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}

int main(int argc, char *argv[])
{
    int i,j;
    char key;
    int cap_ch, cap_path, rec_track;

    printf("  Usage:\n");
    printf("  Multiple streams:\n");
    printf("    #./rtspd\n");
    printf("  Max single h264 stream:\n");
    printf("    #./rtspd -s\n");
    printf("  Max single mpeg4 stream:\n");
    printf("    #./rtspd -m\n");
    printf("  Max single mjpeg stream:\n");
    printf("    #./rtspd -j\n");
	rtspd_set_1ch = 0;
    if (argc > 1) {
        for (i = 1; i < argc; i++) {
            if (argv[i][0] != '-' ) {
                printf("argv error\n");
                return 1;
            } else {
                switch (argv[i][1]) {
                	case 's':
                		rtspd_set_1ch = 1;
                        rtspd_set_enc_type = 0;
                		break;
                    case 'm':
                		rtspd_set_1ch = 1;
                        rtspd_set_enc_type = 1;
                		break;
                    case 'j':
                		rtspd_set_1ch = 1;
                        rtspd_set_enc_type = 2;
                		break;    
                    default:
                        printf("argv error:%s\n", argv[i]);
                        return 1;
                }
            }
        }
    }
	
    
    gm_graph_init();
    for (cap_ch = 0; cap_ch < CAP_CH_NUM; cap_ch++) {
        for (cap_path = 0; cap_path < CAP_PATH_NUM; cap_path++) {
            for (rec_track = 0; rec_track < ENC_TRACK_NUM; rec_track++) {
                update_video_sdp(cap_ch, cap_path, rec_track);
            }
        }
    }

    rtspd_start(554);

    printf("Connect command:\n");
    for (i = 0; i < CAP_CH_NUM; i++) {
        for (j = 0; j < rtspd_avail_ch; j++) {
            	printf("    rtsp://%s/live/ch%02d_%d\n", get_local_ip(), i, j);
        }
    }
    printf("Press 'q' to exit.\n");
    
    while(1) {
        key = getch();
        if (key == 'q' || key == 'Q')
            break;
        sleep(1);
    }
    rtspd_stop();
    gm_graph_release();
    return 0;
}
