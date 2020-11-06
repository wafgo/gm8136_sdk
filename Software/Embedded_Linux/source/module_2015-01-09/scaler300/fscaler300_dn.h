#ifndef __FSCALER300_DN_H__
#define __FSCALER300_DN_H__

typedef enum {
    DN_GEOMATRIC = 0,
    DN_SIMILARITY,
    DN_ADAPTIVE_ENB,
    DN_ADAPTIVE_STEP,
    DN_PARAM_MAX
} DN_PARAM_ID_T;

typedef struct dn_param {
    int geomatric;
    int similarity;
    int adp_en;
    int adp_step;
} dn_param_t;

int fscaler300_dn_proc_init(struct proc_dir_entry *entity_proc);
void fscaler300_dn_proc_remove(struct proc_dir_entry *entity_proc);

#endif
