#ifndef __FSCALER300_SMOOTH_H__
#define __FSCALER300_SMOOTH_H__

typedef enum {
    SMOOTH_HORIZONTAL_STRENGTH = 0,
    SMOOTH_VERTICAL_STRENGTH,
    SMOOTH_PARAM_MAX
} SMOOTH_PARAM_ID_T;

typedef struct smooth_param {
    u8          hsmo;
    u8          vsmo;
} smooth_param_t;

int fscaler300_smooth_proc_init(struct proc_dir_entry *entity_proc);
void fscaler300_smooth_proc_remove(struct proc_dir_entry *entity_proc);

#endif
