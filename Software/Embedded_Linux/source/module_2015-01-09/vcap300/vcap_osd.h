#ifndef _VCAP_OSD_H_
#define _VCAP_OSD_H_

/*************************************************************************************
 *  VCAP OSD & Mark Priority Definition
 *************************************************************************************/
typedef enum {
    VCAP_OSD_PRIORITY_MARK_ON_OSD = 0,          ///< Mark above OSD window
    VCAP_OSD_PRIORITY_OSD_ON_MARK,              ///< Mark below OSD window
    VCAP_OSD_PRIORITY_MAX
} VCAP_OSD_PRIORITY_T;

/*************************************************************************************
 *  VCAP OSD Font Smooth Definition
 *************************************************************************************/
typedef enum {
    VCAP_OSD_FONT_SMOOTH_LEVEL_WEAK = 0,        ///< weak smoothing effect
    VCAP_OSD_FONT_SMOOTH_LEVEL_STRONG,          ///< strong smoothing effect
    VCAP_OSD_FONT_SMOOTH_LEVEL_MAX
} VCAP_OSD_FONT_SMOOTH_LEVEL_T;

struct vcap_osd_font_smooth_param_t {
    u8                              enable;     ///< OSD font smooth enable/disable control
    VCAP_OSD_FONT_SMOOTH_LEVEL_T    level;      ///< OSD font smooth level control
};

/*************************************************************************************
 *  VCAP OSD Font Edge Definition
 *************************************************************************************/
typedef enum {
    VCAP_OSD_FONT_EDGE_MODE_STANDARD = 0,       ///< Standard zoom-in mode
    VCAP_OSD_FONT_EDGE_MODE_2PIXEL,             ///< Two-pixel based zoom-in mode
    VCAP_OSD_FONT_EDGE_MODE_MAX
} VCAP_OSD_FONT_EDGE_MODE_T;

/*************************************************************************************
 *  VCAP OSD Font Zoom Definition
 *************************************************************************************/
typedef enum {
    VCAP_OSD_FONT_ZOOM_NONE = 0,        ///< disable zoom
    VCAP_OSD_FONT_ZOOM_2X,              ///< horizontal and vertical zoom in 2x
    VCAP_OSD_FONT_ZOOM_3X,              ///< horizontal and vertical zoom in 3x
    VCAP_OSD_FONT_ZOOM_4X,              ///< horizontal and vertical zoom in 4x
    VCAP_OSD_FONT_ZOOM_ONE_HALF,        ///< horizontal and vertical zoom out 2x

    VCAP_OSD_FONT_ZOOM_H2X_V1X = 8,     ///< horizontal zoom in 2x
    VCAP_OSD_FONT_ZOOM_H4X_V1X,         ///< horizontal zoom in 4x
    VCAP_OSD_FONT_ZOOM_H4X_V2X,         ///< horizontal zoom in 4x and vertical zoom in 2x

    VCAP_OSD_FONT_ZOOM_H1X_V2X = 12,    ///< vertical zoom in 2x
    VCAP_OSD_FONT_ZOOM_H1X_V4X,         ///< vertical zoom in 4x
    VCAP_OSD_FONT_ZOOM_H2X_V4X,         ///< horizontal zoom in 2x and vertical zoom in 4x
    VCAP_OSD_FONT_ZOOM_MAX
} VCAP_OSD_FONT_ZOOM_T;

/*************************************************************************************
 *  VCAP OSD Alpha Definition
 *************************************************************************************/
typedef enum {
    VCAP_OSD_ALPHA_0 = 0,               ///< alpha 0%
    VCAP_OSD_ALPHA_25,                  ///< alpha 25%
    VCAP_OSD_ALPHA_37_5,                ///< alpha 37.5%
    VCAP_OSD_ALPHA_50,                  ///< alpha 50%
    VCAP_OSD_ALPHA_62_5,                ///< alpha 62.5%
    VCAP_OSD_ALPHA_75,                  ///< alpha 75%
    VCAP_OSD_ALPHA_87_5,                ///< alpha 87.5%
    VCAP_OSD_ALPHA_100,                 ///< alpha 100%
    VCAP_OSD_ALPHA_MAX
} VCAP_OSD_ALPHA_T;

struct vcap_osd_alpha_t {
    VCAP_OSD_ALPHA_T    font;           ///< OSD window font transparency
    VCAP_OSD_ALPHA_T    background;     ///< OSD window background transparency
};

/*************************************************************************************
 *  VCAP OSD Marquee Definition
 *************************************************************************************/
typedef enum {
    VCAP_OSD_MARQUEE_LENGTH_8192 = 0,           ///< 8192 step
    VCAP_OSD_MARQUEE_LENGTH_4096,               ///< 4096 step
    VCAP_OSD_MARQUEE_LENGTH_2048,               ///< 2048 step
    VCAP_OSD_MARQUEE_LENGTH_1024,               ///< 1024 step
    VCAP_OSD_MARQUEE_LENGTH_512,                ///< 512  step
    VCAP_OSD_MARQUEE_LENGTH_256,                ///< 256  step
    VCAP_OSD_MARQUEE_LENGTH_128,                ///< 128  step
    VCAP_OSD_MARQUEE_LENGTH_64,                 ///< 64   step
    VCAP_OSD_MARQUEE_LENGTH_32,                 ///< 32   step
    VCAP_OSD_MARQUEE_LENGTH_16,                 ///< 16   step
    VCAP_OSD_MARQUEE_LENGTH_8,                  ///< 8    step
    VCAP_OSD_MARQUEE_LENGTH_4,                  ///< 4    step
    VCAP_OSD_MARQUEE_LENGTH_MAX
} VCAP_OSD_MARQUEE_LENGTH_T;

typedef enum {
    VCAP_OSD_MARQUEE_MODE_NONE = 0,             ///< no marueee effect
    VCAP_OSD_MARQUEE_MODE_HLINE,                ///< one horizontal line marquee effect
    VCAP_OSD_MARQUEE_MODE_VLINE,                ///< one vertical line marquee effect
    VCAP_OSD_MARQUEE_MODE_HFLIP,                ///< one horizontal line flip effect
    VCAP_OSD_MARQUEE_MODE_MAX
} VCAP_OSD_MARQUEE_MODE_T;

struct vcap_osd_marquee_param_t {
    VCAP_OSD_MARQUEE_LENGTH_T       length;     ///< OSD marquee length control
    u8                              speed;      ///< OSD marquee speed control, 0:fastest, 3:slowest
};

/*************************************************************************************
 *  VCAP OSD Border Definition
 *************************************************************************************/
typedef enum {
    VCAP_OSD_BORDER_TYPE_BACKGROUND = 0,    ///< treat border as background
    VCAP_OSD_BORDER_TYPE_FONT,              ///< treat border as font
    VCAP_OSD_BORDER_TYPE_MAX
} VCAP_OSD_BORDER_TYPE_T;

struct vcap_osd_border_param_t {
    u8                      width;          ///< OSD window border width
    u8                      color;          ///< OSD window border color, palette index
    VCAP_OSD_BORDER_TYPE_T  type;           ///< OSD window border type
};

#define VCAP_OSD_BORDER_WIDTH_MAX           ((PLAT_OSD_BORDER_PIXEL == 2) ? 0xf : 0x7)

/*************************************************************************************
 *  VCAP OSD Color Definition
 *************************************************************************************/
struct vcap_osd_color_t {
    u8  fg_color;       ///< OSD foreground color, palette index
    u8  bg_color;       ///< OSD background color, palette index
};

/*************************************************************************************
 *  VCAP OSD Window Definition
 *************************************************************************************/
struct vcap_osd_win_t {
    u8      align;              ///< OSD window auto align type
    u8      path;               ///< OSD window display on which path
    u16     x_start;            ///< OSD window X start position
    u16     y_start;            ///< OSD window Y start position
    u16     width;              ///< OSD window width
    u16     height;             ///< OSD window height
    u16     row_sp;             ///< OSD window row space,    4 pixel  alignment
    u16     col_sp;             ///< OSD window column space, 1 line   alignment
    u8      h_wd_num;           ///< OSD window horizontal word number minus one
    u8      v_wd_num;           ///< OSD window vertical word number minus one
    u32     wd_addr;            ///< OSD window word address at osd font display ram
};

/*************************************************************************************
 *  VCAP OSD_Mask Window Definition, use OSD Window as Mask Window
 *************************************************************************************/
struct vcap_osd_mask_win_t {
    u8      align;              ///< OSD_Mask window auto align type
    u8      path;               ///< OSD_Mask window display on which path
    u16     x_start;            ///< OSD_Mask window X start position
    u16     y_start;            ///< OSD_Mask window Y start position
    u16     width;              ///< OSD_Mask window width
    u16     height;             ///< OSD_Mask window height
    u8      color;              ///< OSD_Mask window color, palette index
};

/*************************************************************************************
 *  VCAP OSD Character Definition
 *************************************************************************************/
struct vcap_osd_char_t {
    u16 font;                   ///< font index,
    u8  fbitmap[36];            ///< font bitmap, 12 bits per row, total 18 rows.
};

/*************************************************************************************
 *  OSD Automatic Color Change Scheme Definition
 *************************************************************************************/
#define VCAP_OSD_ACCS_DATA_THRES_MAX                    0xf

#define VCAP_OSD_ACCS_ADJUST_ENB_DEFAULT                0x1
#define VCAP_OSD_ACCS_HYSTERESIS_RANGE_DEFAULT          0x0
#define VCAP_OSD_ACCS_DATA_AVG_LEVEL_DEFAULT            0xB0
#define VCAP_OSD_ACCS_ACCUM_WHITE_THRES_DEFAULT         0x1
#define VCAP_OSD_ACCS_ACCUM_BLACK_THRES_DEFAULT         0xD
#define VCAP_OSD_ACCS_NO_HYSTERESIS_THRES_DEFAULT       0x1
#define VCAP_OSD_ACCS_UPDATE_FSM_ENB_DEFAULT            0x1

/*************************************************************************************
 *  Public Function Prototype
 *************************************************************************************/
int vcap_osd_init(struct vcap_dev_info_t *pdev_info);

int vcap_osd_set_char_4bits(struct vcap_dev_info_t *pdev_info, struct vcap_osd_char_t *osd_char, u8 bg_color, u8 fg_color);
int vcap_osd_set_char(struct vcap_dev_info_t *pdev_info, struct vcap_osd_char_t *osd_char);
int vcap_osd_remove_char(struct vcap_dev_info_t *pdev_info, int font);
int vcap_osd_set_disp_string(struct vcap_dev_info_t *pdev_info, u32 offset, u16 *font_data, u16 font_num);

int vcap_osd_set_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_win_t *win);
int vcap_osd_get_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_win_t *win);
int vcap_osd_win_enable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx);
int vcap_osd_win_disable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx);

int vcap_osd_set_priority(struct vcap_dev_info_t *pdev_info, int ch, VCAP_OSD_PRIORITY_T priority);
int vcap_osd_get_priority(struct vcap_dev_info_t *pdev_info, int ch, VCAP_OSD_PRIORITY_T *priority);

int vcap_osd_set_font_smooth_param(struct vcap_dev_info_t *pdev_info, int ch, struct vcap_osd_font_smooth_param_t *param);
int vcap_osd_get_font_smooth_param(struct vcap_dev_info_t *pdev_info, int ch, struct vcap_osd_font_smooth_param_t *param);

int vcap_osd_set_font_marquee_param(struct vcap_dev_info_t *pdev_info, int ch, struct vcap_osd_marquee_param_t *param);
int vcap_osd_get_font_marquee_param(struct vcap_dev_info_t *pdev_info, int ch, struct vcap_osd_marquee_param_t *param);
int vcap_osd_set_font_marquee_mode(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_OSD_MARQUEE_MODE_T mode);
int vcap_osd_get_font_marquee_mode(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_OSD_MARQUEE_MODE_T *mode);

int vcap_osd_set_img_border_color(struct vcap_dev_info_t *pdev_info, int ch, int color);
int vcap_osd_get_img_border_color(struct vcap_dev_info_t *pdev_info, int ch, int *color);
int vcap_osd_set_img_border_width(struct vcap_dev_info_t *pdev_info, int ch, int path, int width);
int vcap_osd_get_img_border_width(struct vcap_dev_info_t *pdev_info, int ch, int path, int *width);
int vcap_osd_img_border_enable(struct vcap_dev_info_t *pdev_info, int ch, int path);
int vcap_osd_img_border_disable(struct vcap_dev_info_t *pdev_info, int ch, int path);
int vcap_osd_frame_mode_enable(struct vcap_dev_info_t *pdev_info, int ch, int path);
int vcap_osd_frame_mode_disable(struct vcap_dev_info_t *pdev_info, int ch, int path);
int vcap_osd_set_font_edge_mode(struct vcap_dev_info_t *pdev_info, int ch, VCAP_OSD_FONT_EDGE_MODE_T mode);
int vcap_osd_get_font_edge_mode(struct vcap_dev_info_t *pdev_info, int ch, VCAP_OSD_FONT_EDGE_MODE_T *mode);

int vcap_osd_set_font_zoom(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_OSD_FONT_ZOOM_T zoom);
int vcap_osd_get_font_zoom(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_OSD_FONT_ZOOM_T *zoom);

int vcap_osd_set_alpha(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_alpha_t *alpha);
int vcap_osd_get_alpha(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_alpha_t *alpha);

int vcap_osd_set_border_param(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_border_param_t *param);
int vcap_osd_get_border_param(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_border_param_t *param);
int vcap_osd_border_enable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx);
int vcap_osd_border_disable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx);

int vcap_osd_set_win_color(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_color_t *color);
int vcap_osd_get_win_color(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_color_t *color);

int vcap_osd_set_font_color(struct vcap_dev_info_t *pdev_info, struct vcap_osd_color_t *color);
int vcap_osd_get_font_color(struct vcap_dev_info_t *pdev_info, struct vcap_osd_color_t *color);

int vcap_osd_mask_set_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_mask_win_t *win);
int vcap_osd_mask_get_win(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, struct vcap_osd_mask_win_t *win);
int vcap_osd_mask_set_alpha(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_OSD_ALPHA_T alpha);
int vcap_osd_mask_get_alpha(struct vcap_dev_info_t *pdev_info, int ch, int win_idx, VCAP_OSD_ALPHA_T *alpha);

int vcap_osd_set_accs_data_thres(struct vcap_dev_info_t *pdev_info, int ch, int thres);
int vcap_osd_get_accs_data_thres(struct vcap_dev_info_t *pdev_info, int ch, int *thres);
int vcap_osd_accs_win_enable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx);
int vcap_osd_accs_win_disable(struct vcap_dev_info_t *pdev_info, int ch, int win_idx);

#endif  /* _VCAP_OSD_H_ */
