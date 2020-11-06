#ifndef _VCAP_FCS_H_
#define _VCAP_FCS_H_

#define VCAP_FCS_LV0_THRED_DEFAULT            0xfa
#define VCAP_FCS_LV1_THRED_DEFAULT            0xc8
#define VCAP_FCS_LV2_THRED_DEFAULT            0xc8
#define VCAP_FCS_LV3_THRED_DEFAULT            0x32
#define VCAP_FCS_LV4_THRED_DEFAULT            0x28
#define VCAP_FCS_GREY_THRED_DEFAULT           0x384

typedef enum {
    VCAP_FCS_PARAM_LV0_THRED = 0,
    VCAP_FCS_PARAM_LV1_THRED,
    VCAP_FCS_PARAM_LV2_THRED,
    VCAP_FCS_PARAM_LV3_THRED,
    VCAP_FCS_PARAM_LV4_THRED,
    VCAP_FCS_PARAM_GREY_THRED,
    VCAP_FCS_PARAM_MAX
} VCAP_FCS_PARAM_T;

void vcap_fcs_init(struct vcap_dev_info_t *pdev_info);
int  vcap_fcs_proc_init(int id, void *private);
void vcap_fcs_proc_remove(int id);
int  vcap_fcs_enable(struct vcap_dev_info_t *pdev_info, int ch);
int  vcap_fcs_disable(struct vcap_dev_info_t *pdev_info, int ch);
u32  vcap_fcs_get_param(struct vcap_dev_info_t *pdev_info, int ch, VCAP_FCS_PARAM_T param_id);
int  vcap_fcs_set_param(struct vcap_dev_info_t *pdev_info, int ch, VCAP_FCS_PARAM_T param_id, u32 data);

#endif  /* _VCAP_FCS_H_ */
