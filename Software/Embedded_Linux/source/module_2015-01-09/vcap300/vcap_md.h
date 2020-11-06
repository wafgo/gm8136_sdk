#ifndef _VCAP_MD_H_
#define _VCAP_MD_H_

struct vcap_md_region_t {
    u8   enable;
    u16  x_start;
    u16  y_start;
    u8   x_num;
    u8   y_num;
    u8   x_size;        ///< 16, 20, 24, 28, 32
    u8   y_size;        ///< 2 ~ 32
    u8   interlace;
};

struct vcap_md_ch_info_t {
    u8   enable;
    u16  x_start;
    u16  y_start;
    u8   x_num;
    u8   y_num;
    u8   x_size;                        ///< 16, 20, 24, 28, 32
    u8   y_size;                        ///< 2 ~ 32
    u8   interlace;
    u32  event_va;                      ///< channel event buffer virtual  address
    u32  event_pa;                      ///< channel event buffer physical address
    u32  event_size;                    ///< channel event buffer size
};

struct vcap_md_ch_tamper_t {
    int md_enb;                         ///< md 0:disable 1:enable
    int alarm;                          ///< current tamper alarm, 0:alarm 1:alarm release
    int sensitive_b_th;                 ///< tamper detection sensitive black threshold, 1 ~ 100, 0:for disable
    int sensitive_h_th;                 ///< tamper detection sensitive homogeneous threshold, 1 ~ 100, 0:for disable
    int histogram_idx;                  ///< tamper detection histogram index, 1 ~ 255
};

#define VCAP_MD_ALL_INFO_CH_MAX     33  ///< 32 channel + 1 Cascade

struct vcap_md_all_info_t {
    u32 dev_event_va;                                       ///< device event buffer start virtual  address
    u32 dev_event_pa;                                       ///< device event buffer start physical address
    u32 dev_event_size;                                     ///< device event buffer size
    struct vcap_md_ch_info_t ch[VCAP_MD_ALL_INFO_CH_MAX];   ///< channel information
};

struct vcap_md_all_tamper_t {
    struct vcap_md_ch_tamper_t ch[VCAP_MD_ALL_INFO_CH_MAX]; ///< channel information
};

#define VCAP_MD_ALPHA_DEFAULT               32
#define VCAP_MD_TBG_DEFAULT                 7371
#define VCAP_MD_INIT_VAL_DEFAULT            7
#define VCAP_MD_TB_DEFAULT                  9
#define VCAP_MD_SIGMA_DEFAULT               11
#define VCAP_MD_PRUNE_DEFAULT               15
#define VCAP_MD_TAU_DEFAULT                 204
#define VCAP_MD_ALPHA_ACCURACY_DEFAULT      0x9ffb0
#define VCAP_MD_TG_DEFAULT                  9
#define VCAP_MD_DXDY_DEFAULT                32
#define VCAP_MD_ONE_MIN_ALPHA_DEFAULT       0x7fe0
#define VCAP_MD_SHADOW_DEFAULT              1
#define VCAP_MD_TAMPER_SENSITIVE_B_DEFAULT  20      ///< sensitive black
#define VCAP_MD_TAMPER_SENSITIVE_H_DEFAULT  0       ///< sensitive homogeneous
#define VCAP_MD_TAMPER_HISTOGRAM_DEFAULT    80

#define VCAP_MD_TAMPER_SENSITIVE_MIN        0       ///< 0 for disable tamper detection
#define VCAP_MD_TAMPER_SENSITIVE_MAX        100
#define VCAP_MD_TAMPER_HISTOGRAM_MIN        1
#define VCAP_MD_TAMPER_HISTOGRAM_MAX        255
#define VCAP_MD_TAMPER_X_UNIT               2
#define VCAP_MD_TAMPER_Y_UNIT               2
#define VCAP_MD_TAMPER_X_NUM_MAX            (128/VCAP_MD_TAMPER_X_UNIT)
#define VCAP_MD_TAMPER_Y_NUM_MAX            (128/VCAP_MD_TAMPER_Y_UNIT)
#define VCAP_MD_TAMPER_GAU_MAX              3

#define VCAP_MD_GAU_GET_MU(x)               ((x) & 0xff)
#define VCAP_MD_GAU_GET_WEIGHT(x)           (((x)>>16) & 0x1fff)
#define VCAP_MD_GAU_GET_MODE(x)             (((x)>>29) & 0x3)

typedef enum {
    VCAP_MD_PARAM_ALPHA = 0,
    VCAP_MD_PARAM_TBG,
    VCAP_MD_PARAM_INIT_VAL,
    VCAP_MD_PARAM_TB,
    VCAP_MD_PARAM_SIGMA,
    VCAP_MD_PARAM_PRUNE,
    VCAP_MD_PARAM_TAU,
    VCAP_MD_PARAM_ALPHA_ACCURACY,
    VCAP_MD_PARAM_TG,
    VCAP_MD_PARAM_DXDY,
    VCAP_MD_PARAM_ONE_MIN_ALPHA,
    VCAP_MD_PARAM_SHADOW,
    VCAP_MD_PARAM_TAMPER_SENSITIVE_B,       ///< Tamper detection sensitive black threshold
    VCAP_MD_PARAM_TAMPER_SENSITIVE_H,       ///< Tamper detection sensitive homogeneous threshold
    VCAP_MD_PARAM_TAMPER_HISTOGRAM,         ///< Tamper detection histogram index
    VCAP_MD_PARAM_MAX
} VCAP_MD_PARAM_T;

#define VCAP_MD_DXDY(x, y)                  ((32*16*16)/(x*y))  ///< x: win_x_size, y: win_y_size

void vcap_md_init(struct vcap_dev_info_t *pdev_info);
int  vcap_md_proc_init(int id, void *private);
void vcap_md_proc_remove(int id);
int  vcap_md_buf_alloc(struct vcap_dev_info_t *pdev_info);
void vcap_md_buf_free(struct vcap_dev_info_t *pdev_info);
int  vcap_md_get_event(struct vcap_dev_info_t *pdev_info, int ch, u32 *event_buf, u32 buf_size);
int  vcap_md_set_param(struct vcap_dev_info_t *pdev_info, int ch, VCAP_MD_PARAM_T param_id, u32 data);
u32  vcap_md_get_param(struct vcap_dev_info_t *pdev_info, int ch, VCAP_MD_PARAM_T param_id);
int  vcap_md_get_region(struct vcap_dev_info_t *pdev_info, int ch, struct vcap_md_region_t *region);
int  vcap_md_get_all_info(struct vcap_dev_info_t *pdev_info, struct vcap_md_all_info_t *md_info);
int  vcap_md_get_all_tamper(struct vcap_dev_info_t *pdev_info, struct vcap_md_all_tamper_t *md_tamper);
int  vcap_md_create_task(struct vcap_dev_info_t *pdev_info);
void vcap_md_remove_task(struct vcap_dev_info_t *pdev_info);

#endif  /* _VCAP_MD_H_ */
