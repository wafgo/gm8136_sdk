#ifndef __FSCALER300_FCS_H__
#define __FSCALER300_FCS_H__

typedef enum {
    LV0_THRED = 0,
    LV1_THRED,
    LV2_THRED,
    LV3_THRED,
    LV4_THRED,
    GREY_THRED,
    FCS_PARAM_MAX
} FCS_PARAM_ID_T;

typedef struct fcs_param {
    int lv0_th;
    int lv1_th;
    int lv2_th;
    int lv3_th;
    int lv4_th;
    int grey_th;
} fcs_param_t;

int fscaler300_fcs_proc_init(struct proc_dir_entry *entity_proc);
void fscaler300_fcs_proc_remove(struct proc_dir_entry *entity_proc);

#endif
