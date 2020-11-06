/**
 * @file encode_with_snapshot.c
 *
 * Support platform: GM8210, GM8287, GM8139
 * This sample code is to snapshot JPEG from H.264 encode bindfd
 * (��ʾ��Ϊ������ץ�ĳ���)
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.37 $
 * $Date: 2014/04/03 06:45:55 $
 *
 */
/**
 * @example encode_with_snapshot.c
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include "gmlib.h"

#define BITSTREAM_LEN       (720 * 576 * 3 / 2)
#define MAX_BITSTREAM_NUM   1

gm_system_t gm_system;
void *groupfd;   // return of gm_new_groupfd()
void *bindfd;    // return of gm_bind()
void *capture_object;
void *encode_object;
pthread_t enc_thread_id;
int enc_exit = 0;   // Notify program exit

static void *encode_thread(void *arg)
{
    int i, ret;
    int ch = (int)arg;
    FILE *record_file;
    char filename[50];
    char *bitstream_data;
    gm_pollfd_t poll_fds[MAX_BITSTREAM_NUM];
    gm_enc_multi_bitstream_t multi_bs[MAX_BITSTREAM_NUM];

    // open file for write(��¼���ļ�)
    bitstream_data = (char *)malloc(BITSTREAM_LEN);
    memset(bitstream_data, 0, BITSTREAM_LEN);

    sprintf(filename, "ch%d_%dx%d_snapshot.264", ch, gm_system.cap[ch].dim.width / 2,
            gm_system.cap[ch].dim.height / 2);
    record_file = fopen(filename, "wb");
    if (record_file == NULL) {
        printf("open file error(%s)! \n", filename);
        exit(1);
    }

    memset(poll_fds, 0, sizeof(poll_fds));

    poll_fds[ch].bindfd = bindfd;
    poll_fds[ch].event = GM_POLL_READ;
    while (enc_exit == 0) {
        /** poll bitstream until 500ms timeout */
        ret = gm_poll(poll_fds, MAX_BITSTREAM_NUM, 500);
        if (ret == GM_TIMEOUT) {
            printf("Poll timeout!!");
            continue;
        }

        memset(multi_bs, 0, sizeof(multi_bs));  //clear all mutli bs
        for (i = 0; i < MAX_BITSTREAM_NUM; i++) {
            if (poll_fds[i].revent.event != GM_POLL_READ)
                continue;
            if (poll_fds[i].revent.bs_len > BITSTREAM_LEN) {
                printf("bitstream buffer length is not enough! %d, %d\n",
                    poll_fds[i].revent.bs_len, BITSTREAM_LEN);
                continue;
            }
            multi_bs[i].bindfd = poll_fds[i].bindfd;
            multi_bs[i].bs.bs_buf = bitstream_data;  // set buffer point(ָ���������ָ��λ��)
            multi_bs[i].bs.bs_buf_len = BITSTREAM_LEN;  // set buffer length(ָ�����峤��)
            multi_bs[i].bs.mv_buf = 0;  // not to recevie MV data
            multi_bs[i].bs.mv_buf_len = 0;  // not to recevie MV data
        }
        
        if ((ret = gm_recv_multi_bitstreams(multi_bs, MAX_BITSTREAM_NUM)) < 0) {
            printf("Error return value %d\n", ret);
        } else {
            for (i = 0; i < MAX_BITSTREAM_NUM; i++) {
                if ((multi_bs[i].retval < 0) && multi_bs[i].bindfd) {
                    printf("CH%d Error to receive bitstream. ret=%d\n", i, multi_bs[i].retval);
                } else if (multi_bs[i].retval == GM_SUCCESS) {
#if 0 //not to print
                    printf("<CH%d, mv_len=%d bs_len=%d, keyframe=%d, newbsflag=0x%x>\n",
                            i, multi_bs[i].bs.mv_len, multi_bs[i].bs.bs_len,
                            multi_bs[i].bs.keyframe, multi_bs[i].bs.newbs_flag);
#endif
                    fwrite(multi_bs[i].bs.bs_buf,1, multi_bs[i].bs.bs_len, record_file);
                    fflush(record_file);
                }
            }
        }
    }

    fclose(record_file);
    free(bitstream_data);
    return 0;
}

#define MAX_SNAPHSOT_LEN    (128*1024)
char    *snapshot_buf = 0;
void sample_get_snapshot(void)
{
    static int filecount = 0;
    int     snapshot_len = 0;
    FILE    *snapshot_fd = NULL;
    char    filename[40];
    snapshot_t snapshot;

    snapshot.bindfd = bindfd;
    snapshot.image_quality = 30;  // The value of image quality from 1(worst) ~ 100(best)
    snapshot.bs_buf = snapshot_buf;
    snapshot.bs_buf_len = MAX_SNAPHSOT_LEN;
    snapshot.bs_width = 176;
    snapshot.bs_height = 144;

    snapshot_len = gm_request_snapshot(&snapshot, 500); // Timeout value 500ms
    if (snapshot_len > 0) {
        sprintf(filename, "snapshot_%d.jpg", filecount++);
        printf("Get %s size %dbytes\n", filename, snapshot_len);
        snapshot_fd = fopen(filename, "wb");
        if (snapshot_fd == NULL) {
            printf("Fail to open file %s\n", filename);
            exit(1);
        }
        fwrite(snapshot_buf, 1, snapshot_len, snapshot_fd);
        fclose(snapshot_fd);
    }
}

int main(void)
{
    int ch, ret = 0;
    int key;
    DECLARE_ATTR(cap_attr, gm_cap_attr_t);
    DECLARE_ATTR(h264e_attr, gm_h264e_attr_t);

    snapshot_buf = (char *)malloc(MAX_SNAPHSOT_LEN);
    if (snapshot_buf == NULL) {
        perror("Error allocation\n");
        exit(1);
    }

    gm_init(); //gmlib initial(GM���ʼ��)
    gm_get_sysinfo(&gm_system);

    groupfd = gm_new_groupfd(); // create new record group fd (��ȡgroupfd)

    capture_object = gm_new_obj(GM_CAP_OBJECT); // new capture object(��ȡ��׽����)
    encode_object = gm_new_obj(GM_ENCODER_OBJECT);  // // create encoder object (��ȡ�������)

    ch = 0; // use capture virtual channel 0
    cap_attr.cap_vch = ch;

    //GM8210 capture path 0(liveview), 1(CVBS), 2(can scaling), 3(can scaling)
    //GM8139/GM8287 capture path 0(liveview), 1(CVBS), 2(can scaling), 3(can't scaling down)
    cap_attr.path = 2;
    cap_attr.enable_mv_data = 0;
    gm_set_attr(capture_object, &cap_attr); // set capture attribute (���ò�׽����)

    h264e_attr.dim.width = gm_system.cap[ch].dim.width / 2;
    h264e_attr.dim.height = gm_system.cap[ch].dim.height / 2;

    h264e_attr.frame_info.framerate = gm_system.cap[ch].framerate;
    h264e_attr.ratectl.mode = GM_CBR;
    h264e_attr.ratectl.gop = 60;
    h264e_attr.ratectl.bitrate = 2048;  // 2Mbps
    h264e_attr.b_frame_num = 0;  // B-frames per GOP (H.264 high profile)
    h264e_attr.enable_mv_data = 0;  // disable H.264 motion data output
    gm_set_attr(encode_object, &h264e_attr);

    // bind channel recording (�󶨲�׽���󵽱������)
    bindfd = gm_bind(groupfd, capture_object, encode_object);
    if (gm_apply(groupfd) < 0) { // active setting (ʹ��Ч)
        perror("Error! gm_apply fail, AP procedure something wrong!");
        exit(0);
    }

    ret = pthread_create(&enc_thread_id, NULL, encode_thread, (void *)ch);
    if (ret < 0) {
        perror("create encode thread failed");
        return -1;
    }

    printf("Enter y to get snapshot jpeg, q to quit\n");
    while (1) {
        key = getchar();
        if (key == 'y') {
            printf("Trigger snapshot\n");
            sample_get_snapshot();
        } else if(key == 'q') {
            enc_exit = 1;
            break;
        }
    }

    pthread_join(enc_thread_id, NULL);
    gm_unbind(bindfd);
    gm_apply(groupfd);
    gm_delete_obj(encode_object);
    gm_delete_obj(capture_object);
    gm_delete_groupfd(groupfd);
    gm_release();
    free(snapshot_buf);
    return ret;
}
