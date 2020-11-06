#ifndef _VCAP_PRESMO_H_
#define _VCAP_PRESMO_H_

typedef enum {
    VCAP_PRESMO_PARAM_NONE_AUTO = 0,
    VCAP_PRESMO_PARAM_H_STRENGTH,
    VCAP_PRESMO_PARAM_V_STRENGTH,
    VCAP_PRESMO_PARAM_MAX
} VCAP_PRESMO_PARAM_T;

void vcap_presmo_init(struct vcap_dev_info_t *pdev_info);
int  vcap_presmo_proc_init(int id, void *private);
void vcap_presmo_proc_remove(int id);
u32  vcap_presmo_get_param(struct vcap_dev_info_t *pdev_info, int ch, int path, VCAP_PRESMO_PARAM_T param_id);
int  vcap_presmo_set_param(struct vcap_dev_info_t *pdev_info, int ch, int path, VCAP_PRESMO_PARAM_T param_id, u32 data);

#endif  /* _VCAP_PRESMO_H_ */

