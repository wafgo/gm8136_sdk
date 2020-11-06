#ifndef _VCAP_SHARP_H_
#define _VCAP_SHARP_H_

#define VCAP_SHARP_ADAPTIVE_ENB_DEFAULT         0x01
#define VCAP_SHARP_RADIUS_DEFAULT               0x00
#define VCAP_SHARP_AMOUNT_DEFAULT               0x16
#define VCAP_SHARP_THRED_DEFAULT                0x05
#define VCAP_SHARP_ADAPTIVE_START_DEFAULT       0x02
#define VCAP_SHARP_ADAPTIVE_STEP_DEFAULT        0x03

typedef enum {
    VCAP_SHARP_PARAM_ADAPTIVE_ENB = 0,
    VCAP_SHARP_PARAM_RADIUS,
    VCAP_SHARP_PARAM_AMOUNT,
    VCAP_SHARP_PARAM_THRED,
    VCAP_SHARP_PARAM_ADAPTIVE_START,
    VCAP_SHARP_PARAM_ADAPTIVE_STEP,
    VCAP_SHARP_PARAM_MAX
} VCAP_SHARP_PARAM_T;

void vcap_sharp_init(struct vcap_dev_info_t *pdev_info);
int  vcap_sharp_proc_init(int id, void *private);
void vcap_sharp_proc_remove(int id);
u32  vcap_sharp_get_param(struct vcap_dev_info_t *pdev_info, int ch, int path, VCAP_SHARP_PARAM_T param_id);
int  vcap_sharp_set_param(struct vcap_dev_info_t *pdev_info, int ch, int path, VCAP_SHARP_PARAM_T param_id, u32 data);

#endif  /* _VCAP_SHARP_H_ */
