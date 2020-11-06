#ifndef __FSCALER300_SHAPR_H__
#define __FSCALER300_SHARP_H__

typedef enum {
    SHARP_ADAPTIVE_ENB = 0,
    SHARP_RADIUS,
    SHARP_AMOUNT,
    SHARP_THRED,
    SHARP_ADAPTIVE_START,
    SHARP_ADAPTIVE_STEP,
    SHARP_PARAM_MAX
} SHARP_PARAM_ID_T;

typedef struct sharp_param {
    u8          adp;                ///< enable adaptive sharpness
    u8          radius;             ///< sharpness radius, 0: means bypass
    u8          amount;             ///< sharpness amount
    u8          threshold;          ///< sharpness dn level
    u8          adp_start;          ///< sharpness adaptive start strength
    u8          adp_step;           ///< sharpness adaptive step size
} sharp_param_t;

int fscaler300_sharp_proc_init(struct proc_dir_entry *entity_proc);
void fscaler300_sharp_proc_remove(struct proc_dir_entry *entity_proc);

#endif
