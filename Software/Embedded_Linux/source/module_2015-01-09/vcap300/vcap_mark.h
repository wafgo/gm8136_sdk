#ifndef _VCAP_MARK_H_
#define _VCAP_MARK_H_

/*************************************************************************************
 *  VCAP Mark Definition
 *************************************************************************************/
typedef enum {
    VCAP_MARK_DIM_16PIXEL = 2,
    VCAP_MARK_DIM_32PIXEL,
    VCAP_MARK_DIM_64PIXEL,
    VCAP_MARK_DIM_128PIXEL,
    VCAP_MARK_DIM_256PIXEL,
    VCAP_MARK_DIM_512PIXEL,
    VCAP_MARK_DIM_MAX
} VCAP_MARK_DIM_T;

typedef enum {
    VCAP_MARK_ZOOM_1X = 0,
    VCAP_MARK_ZOOM_2X,
    VCAP_MARK_ZOOM_4X,
    VCAP_MARK_ZOOM_MAX
} VCAP_MARK_ZOOM_T;

typedef enum {
    VCAP_MARK_ALPHA_0 = 0,        ///< alpha 0%
    VCAP_MARK_ALPHA_25,           ///< alpha 25%
    VCAP_MARK_ALPHA_37_5,         ///< alpha 37.5%
    VCAP_MARK_ALPHA_50,           ///< alpha 50%
    VCAP_MARK_ALPHA_62_5,         ///< alpha 62.5%
    VCAP_MARK_ALPHA_75,           ///< alpha 75%
    VCAP_MARK_ALPHA_87_5,         ///< alpha 87.5%
    VCAP_MARK_ALPHA_100,          ///< alpha 100%
    VCAP_MARK_ALPHA_MAX
} VCAP_MARK_ALPHA_T;

struct vcap_mark_win_t {
    u8                align;      ///< mark window auto align type
    u8                path;       ///< mark window display on which path
    u8                mark_id;    ///< mark image index
    u16               x_start;    ///< x start position of mark window
    u16               y_start;    ///< y start position of mark window
    VCAP_MARK_ZOOM_T  zoom;       ///< mark zoom control
    VCAP_MARK_ALPHA_T alpha;      ///< mark alpha control
};

struct vcap_mark_img_t {
    u8               id;           ///< mark index,
    u16              start_addr;   ///< mark memory address in mark ram
    VCAP_MARK_DIM_T  x_dim;        ///< mark x dimension
    VCAP_MARK_DIM_T  y_dim;        ///< mark y dimension
    void             *img_buf;     ///< mark image buffer, data must be YUV422 format
};

/*************************************************************************************
 *  VCAP Mark Zoom Threshold Definition
 *************************************************************************************/
#define VCAP_MARK_ZOOM_ADJUST_ENB_DEFAULT       0x1
#define VCAP_MARK_ZOOM_ADJUST_CR_DEFAULT        0x0C
#define VCAP_MARK_ZOOM_ADJUST_CB_DEFAULT        0x0C
#define VCAP_MARK_ZOOM_ADJUST_Y_DEFAULT         0x03

/*************************************************************************************
 *  VCAP Mark Information
 *************************************************************************************/
struct vcap_mark_info_t {
    u16              addr;         ///< mark memory address in mark ram
    VCAP_MARK_DIM_T  x_dim;        ///< mark x dimension
    VCAP_MARK_DIM_T  y_dim;        ///< mark y dimension
};

/*************************************************************************************
 *  Public Function Prototype
 *************************************************************************************/
int vcap_mark_init(struct vcap_dev_info_t *pdev_info);
int vcap_mark_set_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_mark_win_t *win);
int vcap_mark_get_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_mark_win_t *win);
int vcap_mark_load_image(struct vcap_dev_info_t *pdev_info, struct vcap_mark_img_t *img);
int vcap_mark_win_enable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx);
int vcap_mark_win_disable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx);
int vcap_mark_get_dim(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, int *x_dim, int *y_dim);
int vcap_mark_get_info(struct vcap_dev_info_t *pdev_info, int mark_id, struct vcap_mark_info_t *mark_info);

#endif  /* _VCAP_MARK_H_ */
