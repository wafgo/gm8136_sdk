#ifndef _VCAP_DN_H_
#define _VCAP_DN_H_

#define VCAP_DN_GEOMATRIC_DEFAULT           0x00
#define VCAP_DN_SIMILARITY_DEFAULT          0x00
#define VCAP_DN_ADAPTIVE_DEFAULT            0x01
#define VCAP_DN_ADAPTIVE_STEP_DEFAULT       0x10

typedef enum {
    VCAP_DN_PARAM_GEOMATRIC = 0,
    VCAP_DN_PARAM_SIMILARITY,
    VCAP_DN_PARAM_ADAPTIVE_ENB,
    VCAP_DN_PARAM_ADAPTIVE_STEP,
    VCAP_DN_PARAM_MAX
} VCAP_DN_PARAM_T;

void vcap_dn_init(struct vcap_dev_info_t *pdev_info);
int  vcap_dn_proc_init(int id, void *private);
void vcap_dn_proc_remove(int id);
int  vcap_dn_enable(struct vcap_dev_info_t *pdev_info, int ch);
int  vcap_dn_disable(struct vcap_dev_info_t *pdev_info, int ch);
u32  vcap_dn_get_param(struct vcap_dev_info_t *pdev_info, int ch, VCAP_DN_PARAM_T param_id);
int  vcap_dn_set_param(struct vcap_dev_info_t *pdev_info, int ch, VCAP_DN_PARAM_T param_id, u32 data);

#endif  /* _VCAP_DN_H_ */
