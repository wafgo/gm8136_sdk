#ifndef _VCAP_MASK_H_
#define _VCAP_MASK_H_

/*************************************************************************************
 *  VCAP Mask Definition
 *************************************************************************************/
struct vcap_mask_win_t {
    u16  x_start;    ///< x start position of mask window
    u16  y_start;    ///< y start position of mark window
    u16  width;      ///< mask window width
    u16  height;     ///< mask window height
    u8   color;      ///< mask window color, palette index 0 ~ 15
};

typedef enum {
    VCAP_MASK_ALPHA_0 = 0,               ///< alpha 0%
    VCAP_MASK_ALPHA_25,                  ///< alpha 25%
    VCAP_MASK_ALPHA_37_5,                ///< alpha 37.5%
    VCAP_MASK_ALPHA_50,                  ///< alpha 50%
    VCAP_MASK_ALPHA_62_5,                ///< alpha 62.5%
    VCAP_MASK_ALPHA_75,                  ///< alpha 75%
    VCAP_MASK_ALPHA_87_5,                ///< alpha 87.5%
    VCAP_MASK_ALPHA_100,                 ///< alpha 100%
    VCAP_MASK_ALPHA_MAX
} VCAP_MASK_ALPHA_T;

typedef enum {
    VCAP_MASK_BORDER_TYPE_HOLLOW = 0,
    VCAP_MASK_BORDER_TYPE_TRUE,
    VCAP_MASK_BORDER_TYPE_MAX
} VCAP_MASK_BORDER_TYPE_T;

struct vcap_mask_border_t {
    u8                          width;      ///< mask window border width when hollow on, border width = 2x(n+1) pixels
    VCAP_MASK_BORDER_TYPE_T     type;       ///< mask window hollow/true
};

/*************************************************************************************
 *  Public Function Prototype
 *************************************************************************************/
int vcap_mask_set_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_mask_win_t *win);
int vcap_mask_get_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_mask_win_t *win);
int vcap_mask_set_alpha(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_MASK_ALPHA_T alpha);
int vcap_mask_get_alpha(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_MASK_ALPHA_T *alpha);
int vcap_mask_set_border(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_mask_border_t *border);
int vcap_mask_get_border(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_mask_border_t *border);
int vcap_mask_enable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx);
int vcap_mask_disable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx);

#endif /* _VCAP_MASK_H_ */
