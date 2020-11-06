#ifndef _AUDIO_FLOW_H_
#define _AUDIO_FLOW_H_

#define MAX_ENCODE_RATIO    3
#define MAX_RESAMPLE_RATIO  6
#define AVG_RESAMPLE_RATIO  3

enum in_dbg_stage {
    IN_REC_RUN   = 0,
    IN_REC_STOP  = 1,
    IN_LIVE_RUN  = 2,
    IN_LIVE_STOP = 3,
    IN_NO_STATUS = 4,
    IN_CASE_DONE = 5,
    IN_PCM_DONE  = 6,
    IN_PLAY_DONE = 7,
    IN_ENC_DONE  = 8,
    IN_JOB_BACK  = 9,
    IN_DBG_MAX
};

enum out_dbg_stage {
    OUT_PLAY_RUN  = 0,
    OUT_PLAY_STOP = 1,
    OUT_NO_STATUS = 2,
    OUT_CASE_DONE = 3,
    OUT_DEC_DONE  = 4,
    OUT_JOB_BACK  = 5,
    OUT_DBG_MAX
};

struct semaphore *get_rec_sem(void);
struct semaphore *get_play_sem(void);
int audio_flow_init(void);
void audio_flow_exit(void);
void audio_in_handler(struct work_struct *work);
void audio_out_handler(struct work_struct *work);

#endif
