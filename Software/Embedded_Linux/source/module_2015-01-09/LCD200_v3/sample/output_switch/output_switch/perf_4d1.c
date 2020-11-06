/**
 * @file perf_4d1.c
 *
 * This sample demos 4D1 encode + 4CIF encode + (4D1 playback or 4D1 liveview)
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.15 $
 * $Date: 2013/11/27 02:47:38 $
 *
 */
/**
 * @example perf_4d1.c
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include "gmlib.h"

#define MAX_DISP_CH         4

#define MAX_MAIN_NUM        4
#define MAX_SUB_NUM         4
#define MAX_REC_NUM         (MAX_MAIN_NUM + MAX_SUB_NUM)

#define BITSTREAM_LEN       (int)(720 * 480 * 0.85)
#define INTERVAL_TIME_MS    5000

gm_system_t gm_system;
void *rec_groupfd;
void *rec_bindfd[MAX_REC_NUM];
void *rec_capture_object[MAX_REC_NUM];
void *rec_encode_object[MAX_REC_NUM];

char *record_buffer[MAX_REC_NUM];
pthread_t record_thread_id;
pthread_t poll_sys_thread_id;

void *display_groupfd = 0;
void *liveview_bindfd[MAX_DISP_CH];
void *playback_bindfd[MAX_DISP_CH];
void *win_object[MAX_DISP_CH];
void *liveview_capture_object[MAX_DISP_CH];
void *playback_file_object[MAX_DISP_CH];

char *playback_buffer;
pthread_t playback_thread_id;

static int is_playback = 0;
static int is_liveview = 0;
static int is_record = 0;

unsigned int time_to_ms(struct timeval *a)
{
    return (a->tv_sec * 1000) + (a->tv_usec / 1000);
}

static void *playback_thread(void *arg)
{
    int ret = 0, ch = 0;
    char filename[40];
    FILE *bs_fd, *len_fd;
    unsigned int length;
    unsigned int frame_period, channel_time, sleep_ms;
    unsigned int perf_start_ms = 0, frame_counts = 0, now_time_ms = 0;
    struct timeval time_start;
    gm_dec_multi_bitstream_t multi_dec[MAX_DISP_CH];

    frame_period = 1000 / 25;//assume playback frame rate is same as capture
//    frame_period = 1000 / gm_system.cap[0].framerate;//assume playback frame rate is same as capture

    sprintf((char *)filename, "D30.264");
    if ((bs_fd = fopen((char *)filename, "rb+")) == NULL) {
        printf("[ERROR] CH%d Open %s failed!!\n", ch, filename);
        exit(1);
    }
    printf("Playback %s frame_period %dms\n", filename, frame_period);

    sprintf((char *)filename, "D30.len");
    if ((len_fd = fopen((char *)filename, "rb+")) == NULL) {
        printf("[ERROR] CH%d Open %s failed!!\n", ch, filename);
        exit(1);
    }

    gettimeofday(&time_start, NULL);
    perf_start_ms = time_to_ms(&time_start);
    channel_time = perf_start_ms + frame_period;

    while (is_playback) {
        if (fscanf(len_fd, "%d\n", &length) == EOF) {
            fseek(bs_fd, 0, SEEK_SET);
            fseek(len_fd, 0, SEEK_SET);
            fscanf(len_fd, "%d\n", &length);
        }
        if (length == 0)
            break;
        fread(playback_buffer, 1, length, bs_fd);

        for (ch = 0; ch < MAX_DISP_CH; ch++) {
            multi_dec[ch].bindfd = playback_bindfd[ch];
            multi_dec[ch].bs_buf = playback_buffer;
            multi_dec[ch].bs_buf_len = length;
        }

        gettimeofday(&time_start, NULL);
        now_time_ms = time_to_ms(&time_start);

        if ((now_time_ms - perf_start_ms) > INTERVAL_TIME_MS) {
            printf("Playback %d frames in %dms (each %dfps)\n", frame_counts, now_time_ms -
                perf_start_ms, (frame_counts * 1000) / (now_time_ms - perf_start_ms));
            frame_counts = 0;
            perf_start_ms = now_time_ms;
        }

        sleep_ms = 0;
        if (channel_time > now_time_ms)
            sleep_ms = channel_time - now_time_ms;
        if (sleep_ms)
            usleep(sleep_ms * 1000); //no data playback

        while ((ret = gm_send_multi_bitstreams(multi_dec, MAX_DISP_CH, 500)) < 0) {
            if (ret == GM_TIMEOUT) {
                printf("<send bitstream timeout(%d)!>\n", ret);
                usleep(1000);
                continue;
            }
            printf("<send bitstream fail(%d)!>\n", ret);
            return 0;
        }
        channel_time += frame_period;
        frame_counts++;
    }
    return 0;
}

static void *record_thread(void *arg)
{
    int ch, fd_numbers = 0, ret;
    gm_pollfd_t poll_fds[MAX_REC_NUM];
    gm_enc_multi_bitstream_t multi_enc[MAX_REC_NUM];
    struct timeval time_start, poll_start;
    unsigned int perf_start_ms = 0, now_time_ms = 0, poll_time_ms = 0, sleep_ms = 0;
    unsigned int min_frame_period = 1000, total_time_ms = 0;
    unsigned int frame_counts[MAX_REC_NUM];
    unsigned int bs_len[MAX_REC_NUM];

    memset(poll_fds, 0, sizeof(poll_fds));
    for (ch = 0; ch < MAX_REC_NUM; ch++) {
        frame_counts[ch] = 0;
        bs_len[ch] = 0;
        poll_fds[ch].bindfd = rec_bindfd[ch];
        poll_fds[ch].event |= GM_POLL_READ;
        fd_numbers++;
    }

    for (ch = 0; ch < MAX_MAIN_NUM; ch++) {
        printf("Capture vch=%d framerate=%d\n",ch, gm_system.cap[ch % MAX_MAIN_NUM].framerate);
        if (min_frame_period > (1000 / gm_system.cap[ch % MAX_MAIN_NUM].framerate))
            min_frame_period = 1000 / gm_system.cap[ch % MAX_MAIN_NUM].framerate;
    }
    printf("Encode min_frame_period = %dms\n", min_frame_period);

    gettimeofday(&time_start, NULL);
    poll_time_ms = perf_start_ms = time_to_ms(&time_start);

    while (is_record) {
        gettimeofday(&poll_start, NULL);
        if (min_frame_period > (time_to_ms(&poll_start) - poll_time_ms)) {
            sleep_ms = min_frame_period - (time_to_ms(&poll_start) - poll_time_ms);
            usleep((sleep_ms * 1000) / 2);
        }

        poll_time_ms = time_to_ms(&poll_start);
        ret = gm_poll(poll_fds, fd_numbers, 500);
        if (ret == GM_TIMEOUT) {
            printf("Poll timeout!!");
            continue;
        }
        memset(multi_enc, 0, sizeof(multi_enc));
        for (ch = 0; ch < fd_numbers; ch++) {
            if (poll_fds[ch].revent.event != GM_POLL_READ)
                continue;
            if (poll_fds[ch].revent.bs_len > BITSTREAM_LEN) {
                printf("bitstream buffer length is not enough! %d, %d\n",
                       poll_fds[ch].revent.bs_len, BITSTREAM_LEN);
                continue;
            }
            multi_enc[ch].bindfd = rec_bindfd[ch];
            multi_enc[ch].bs.bs_buf = record_buffer[ch];
            multi_enc[ch].bs.bs_buf_len = BITSTREAM_LEN;
            multi_enc[ch].bs.mv_buf = 0;  // not to recevie MV data
            multi_enc[ch].bs.mv_buf_len = 0;  // not to recevie MV data
        }

        if ((ret = gm_recv_multi_bitstreams(multi_enc, fd_numbers)) < 0) {
            printf("<recv bitstream fail(%d)!>\n", ret);
            return 0;
        }

        for (ch = 0; ch < MAX_REC_NUM; ch++) {
            if (multi_enc[ch].retval < 0 && multi_enc[ch].bindfd)
                printf("CH%d Error to receive bitstream. ret=%d\n", ch, multi_enc[ch].retval);
            else if (multi_enc[ch].retval == GM_SUCCESS) {
                frame_counts[ch]++;
                bs_len[ch] += multi_enc[ch].bs.bs_len;
            }
        }

        gettimeofday(&time_start, NULL);
        now_time_ms = time_to_ms(&time_start);
        total_time_ms = now_time_ms - perf_start_ms;
        if (total_time_ms > INTERVAL_TIME_MS) {
            for (ch = 0; ch < MAX_REC_NUM; ch++) {
                printf("Record CH%2d %d.%02dfps %dkbps\n", ch,
                        (frame_counts[ch] * 1000 / total_time_ms),
                        (frame_counts[ch] * 100000 / total_time_ms) % 100,
                        (bs_len[ch] * 8 / 1024) * 1000 / total_time_ms);
                frame_counts[ch] = 0;
                bs_len[ch] = 0;
            }
            perf_start_ms = now_time_ms;
            printf("\n");
        }
    }
    return 0;
}

void start_liveview(void)
{
    int ch;
    DECLARE_ATTR(win_attr, gm_win_attr_t);
    DECLARE_ATTR(cap_attr, gm_cap_attr_t);

    if (is_liveview)
        return;
    for (ch = 0; ch < MAX_DISP_CH; ch++) {
        cap_attr.cap_vch = ch;

    //GM8210/GM8287 capture path 0(liveview), 1(reserved, don't use), 2(substream), 3(mainstream)
    //GM8139 capture path 0(liveview), 1(substream), 2(substream), 3(mainstream)
        cap_attr.path = 0;
        cap_attr.enable_mv_data = 0;
        gm_set_attr(liveview_capture_object[ch], &cap_attr);

        win_attr.lcd_vch = 0;
        win_attr.rect.x = (ch % 2) * (gm_system.lcd[0].dim.width / 2);
        win_attr.rect.y = (ch / 2) * (gm_system.lcd[0].dim.height / 2);
        win_attr.rect.width = gm_system.lcd[0].dim.width /2;
        win_attr.rect.height = gm_system.lcd[0].dim.height / 2;
        gm_set_attr(win_object[ch], &win_attr);

        liveview_bindfd[ch] = gm_bind(display_groupfd, liveview_capture_object[ch], win_object[ch]);
    }
    gm_apply(display_groupfd);
    is_liveview = 1;
}

void stop_liveview(void)
{
    int ch;
    if (is_liveview == 0)
        return;
    for (ch = 0; ch < MAX_DISP_CH; ch++)
        gm_unbind(liveview_bindfd[ch]);
    gm_apply(display_groupfd);
    is_liveview = 0;
}

void start_playback(void)
{
    int ch, ret;
    DECLARE_ATTR(win_attr, gm_win_attr_t);
    DECLARE_ATTR(file_attr, gm_file_attr_t);

    if (is_playback)
        return;
    for (ch = 0; ch < MAX_DISP_CH; ch++) {
        file_attr.vch = ch;
        file_attr.max_width = 720;
        file_attr.max_height = 480;
        gm_set_attr(playback_file_object[ch], &file_attr);

        win_attr.lcd_vch = 0;
        win_attr.rect.x = (ch % 2) * (gm_system.lcd[0].dim.width / 2);
        win_attr.rect.y = (ch / 2) * (gm_system.lcd[0].dim.height / 2);
        win_attr.rect.width = gm_system.lcd[0].dim.width / 2;
        win_attr.rect.height = gm_system.lcd[0].dim.height / 2;
        gm_set_attr(win_object[ch], &win_attr);

        playback_bindfd[ch] = gm_bind(display_groupfd, playback_file_object[ch], win_object[ch]);
    }
    gm_apply(display_groupfd);

    is_playback = 1;
    ret = pthread_create(&playback_thread_id, NULL, playback_thread, (void *)0);
    if (ret < 0)    {
        perror("create playback_thread failed!");
        return;
    }
}

void stop_playback(void)
{
    int ch;
    if (is_playback == 0)
        return;
    is_playback = 0;
    for (ch = 0; ch < MAX_DISP_CH; ch++)
        gm_unbind(playback_bindfd[ch]);
    gm_apply(display_groupfd);
    pthread_join(playback_thread_id, NULL);
}

void start_record(void)
{
    int ch, ret;
    DECLARE_ATTR(cap_attr, gm_cap_attr_t);
    DECLARE_ATTR(h264e_attr, gm_h264e_attr_t);
    DECLARE_ATTR(di_attr, gm_3di_attr_t);

    if (is_record)
        return;
	for (ch = 0; ch < MAX_REC_NUM; ch++) {
		if (ch < MAX_MAIN_NUM) {
			cap_attr.cap_vch = ch;
			//GM8210/GM8287 capture path 0(liveview), 1(reserved, don't use), 2(substream), 3(mainstream)
			//GM8139 capture path 0(liveview), 1(substream), 2(substream), 3(mainstream)
			cap_attr.path = 2;
			cap_attr.enable_mv_data = 0;
			gm_set_attr(rec_capture_object[ch], &cap_attr);
			di_attr.deinterlace = 1;
			di_attr.denoise = 1;
			gm_set_attr(rec_capture_object[ch], &di_attr);

			h264e_attr.ratectl.mode = GM_CBR;
			h264e_attr.ratectl.gop = 60;
			h264e_attr.frame_info.framerate = 30;
			h264e_attr.dim.width = 720;
			h264e_attr.dim.height = 480;
			h264e_attr.ratectl.bitrate = 1024;      // 2Mbps
			h264e_attr.b_frame_num = 0;
			h264e_attr.enable_mv_data = 0;
			gm_set_attr(rec_encode_object[ch], &h264e_attr);
			rec_bindfd[ch] = gm_bind(rec_groupfd, rec_capture_object[ch], rec_encode_object[ch]);
		} else {
			cap_attr.cap_vch = ch - MAX_MAIN_NUM;
			cap_attr.path = 2;
			cap_attr.enable_mv_data = 0;
			gm_set_attr(rec_capture_object[ch - MAX_MAIN_NUM], &cap_attr);

			h264e_attr.ratectl.mode = GM_CBR;
			h264e_attr.ratectl.gop = 60;
            h264e_attr.frame_info.framerate = 15;
            h264e_attr.dim.width = 352;
            h264e_attr.dim.height = 240;
            h264e_attr.ratectl.bitrate = 512;      // 128Kbps
			gm_set_attr(rec_encode_object[ch], &h264e_attr);
			rec_bindfd[ch] = gm_bind(rec_groupfd, rec_capture_object[ch - MAX_MAIN_NUM], rec_encode_object[ch]);
        }
    }

    gm_apply(rec_groupfd);

    is_record = 1;
    ret = pthread_create(&record_thread_id, NULL, record_thread, (void *)0);
    if (ret < 0)    {
        perror("create record_thread failed!");
        return;
    }
}

void stop_record(void)
{
    int ch;
    if (is_record == 0)
        return;
    is_record = 0;
    for (ch = 0; ch < MAX_REC_NUM; ch++)
        gm_unbind(rec_bindfd[ch]);
    gm_apply(rec_groupfd);
    pthread_join(record_thread_id, NULL);
}


void init_display(void)
{
    int ch;

    display_groupfd = gm_new_groupfd();
    for (ch = 0; ch < MAX_DISP_CH; ch++) {
        playback_file_object[ch] = gm_new_obj(GM_FILE_OBJECT);
        win_object[ch] = gm_new_obj(GM_WIN_OBJECT);
        liveview_capture_object[ch] = gm_new_obj(GM_CAP_OBJECT);
    }
    playback_buffer = malloc(BITSTREAM_LEN);
    if (playback_buffer == NULL)
        perror("Error allocation bitstream buffer\n");
}

void close_display(void)
{
    int ch;
    for (ch = 0; ch < MAX_DISP_CH; ch++) {
        gm_delete_obj(playback_file_object[ch]);
        gm_delete_obj(win_object[ch]);
        gm_delete_obj(liveview_capture_object[ch]);
    }
    gm_delete_groupfd(display_groupfd);
    free(playback_buffer);
}

void init_record(void)
{
    int ch;
    rec_groupfd = gm_new_groupfd();
    for (ch = 0; ch < MAX_MAIN_NUM; ch++)
        rec_capture_object[ch] = gm_new_obj(GM_CAP_OBJECT);

    for (ch = 0; ch < MAX_REC_NUM; ch++) {
        rec_encode_object[ch] = gm_new_obj(GM_ENCODER_OBJECT);
        record_buffer[ch] = malloc(BITSTREAM_LEN);
        if (record_buffer[ch] == NULL)
            perror("Error allocation bitstream buffer\n");
    }
}

void close_record(void)
{
    int ch;
    for (ch = 0; ch < MAX_MAIN_NUM; ch++)
        gm_delete_obj(rec_capture_object[ch]);

    for (ch = 0; ch < MAX_REC_NUM; ch++) {
        gm_delete_obj(rec_encode_object[ch]);
        free(record_buffer[ch]);
    }
    gm_delete_groupfd(rec_groupfd);
}


void show_menu(void)
{
    printf(" (L)Liveview\n");
    printf(" (P)Playback\n");
    printf(" (R)Record\n");
    printf(" (Q)Exit\n");
}

void *poll_sys_thread(void *arg)
{
	while(1)
	{
		usleep(500000);
		gm_get_sysinfo(&gm_system);
		printf("get_system_info;\n");
	}
}

int main(int argc, char *argv[])
{
    int is_exit = 0;
    int key;
	int ret;

    gm_init();
    gm_get_sysinfo(&gm_system);

    init_display();
    init_record();

    show_menu();

	printf("Start Liveview...\n");
	start_liveview();
	printf("Start Record...\n");
	start_record();

    ret = pthread_create(&poll_sys_thread_id, NULL, poll_sys_thread, (void *)0);
	while(1)
	{
        printf("xx > output_type\n");
		sleep(1);
		system("echo 51 > /proc/flcd200/pip/output_type");
		sleep(1);
		system("echo 46 > /proc/flcd200/pip/output_type");
		sleep(1);
		system("echo 50 > /proc/flcd200/pip/output_type");
	}

    while (!is_exit) {
        key = getchar();
        switch(key) {
            case 'L': case 'l':
                printf("Start Liveview...\n");
                stop_playback();
                start_liveview();
                break;
            case 'P': case 'p':
                printf("Start Playback...\n");
                stop_liveview();
                start_playback();
                break;
            case 'R': case 'r':
                printf("Start Record...\n");
                start_record();
                break;
            case 'Q': case 'q':
                stop_record();
                stop_liveview();
                stop_playback();
                is_exit = 1;
                break;
            default:
                show_menu();
                break;
        }
    }

    close_display();
    close_record();
    gm_release();
    return 0;
}
