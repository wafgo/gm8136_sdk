#ifndef __OSD_API_H__
#define __OSD_API_H__

/*************************************************************************************
 *  OSD Type Definition
 *************************************************************************************/
#define OSD_TYPE_CAP        0       ///< capture
#define OSD_TYPE_SCL        1       ///< scaler

/*************************************************************************************
 * Hardware Ability Definition
 *************************************************************************************/
typedef struct hw_ability {
    u8       ch_osd_win_max;            ///< maximum number of osd window for each channel
    u8       ch_mask_win_max;           ///< maximum number of mask window for each channel
    u8       ch_mark_win_max;           ///< maximum number of mark window for each channel
    u8       dev_palette_max;           ///< maximum number of palette for this device, all channel share
    u8       dev_osd_mask_palette_max;  ///< maximum number of osd mask palette for this device, all channel share
    u16      dev_osd_char_max;          ///< maximum osd font character for this device, all channel share
    u16      dev_osd_disp_max;          ///< maximum osd display character for this device, all channel share
    u16      dev_mark_ram_size;         ///< mark ram size for this device, all channel share
    u8       dev_per_ch_path_max;       ///< maximum number of output path for each channel
    u8       dev_mask_border;           ///< mask window border support, 0: no, 1: yes
} hw_ability_t;

/*************************************************************************************
 *  OSD Parameter Definition
 *************************************************************************************/
#define OSD_FONT_H_SIZE   12        ///< 12 pixel, column
#define OSD_FONT_V_SIZE   18        ///< 18 line,  row
#define OSD_H_SP_SIZE     1         ///< 1 pixel
#define OSD_V_SP_SIZE     1         ///< 1 line

/*************************************************************************************
 *  Window Auto Alignment Definition
 *************************************************************************************/
typedef enum {
    WIN_ALIGN_NONE = 0,             ///< display window on absolute position
    WIN_ALIGN_TOP_LEFT,             ///< display window on relative position, base on output image size
    WIN_ALIGN_TOP_CENTER,           ///< display window on relative position, base on output image size
    WIN_ALIGN_TOP_RIGHT,            ///< display window on relative position, base on output image size
    WIN_ALIGN_BOTTOM_LEFT,          ///< display window on relative position, base on output image size
    WIN_ALIGN_BOTTOM_CENTER,        ///< display window on relative position, base on output image size
    WIN_ALIGN_BOTTOM_RIGHT,         ///< display window on relative position, base on output image size
    WIN_ALIGN_CENTER,               ///< display window on relative position, base on output image size
    WIN_ALIGN_MAX
} WIN_ALIGN_T;

typedef struct osd_char {
    u16     font;                   ///< font index, 0 ~ (osd_char_max - 1)
    u16     fbitmap[18];            ///< font bitmap, 12 bits per row, total 18 rows.
} osd_char_t;

typedef struct img_border_param {
    u8      width;                  ///< image border width 0~7, border width = 4x(n+1) pixels
    u8      color;                  ///< image border color, palette index 0~15
} img_border_param_t;

typedef enum {
    OSD_PRIORITY_MARK_ON_OSD = 0,   ///< Mark above OSD window
    OSD_PRIORITY_OSD_ON_MARK,       ///< Mark below OSD window
    OSD_PRIORITY_MAX
} osd_priority_t;

typedef enum {
    OSD_FONT_SMOOTH_LEVEL_WEAK = 0,     ///< weak smoothing effect
    OSD_FONT_SMOOTH_LEVEL_STRONG,       ///< strong smoothing effect
    OSD_FONT_SMOOTH_LEVEL_MAX
} osd_font_smooth_level_t;

typedef struct osd_smooth {
    u8                          onoff;  ///< OSD font smooth enable/disable 0 : enable, 1 : disable
    osd_font_smooth_level_t     level;  ///< OSD font smooth level
} osd_smooth_t;

typedef enum {
    OSD_MARQUEE_MODE_NONE = 0,         ///< no marueee effect
    OSD_MARQUEE_MODE_HLINE,            ///< one horizontal line marquee effect
    OSD_MARQUEE_MODE_VLINE,            ///< one vertical line marquee effect
    OSD_MARQUEE_MODE_HFLIP,            ///< one horizontal line flip effect
    OSD_MARQUEE_MODE_MAX
} osd_marquee_mode_t;

typedef enum {
    OSD_MARQUEE_LENGTH_8192 = 0,       ///< 8192 step
    OSD_MARQUEE_LENGTH_4096,           ///< 4096 step
    OSD_MARQUEE_LENGTH_2048,           ///< 2048 step
    OSD_MARQUEE_LENGTH_1024,           ///< 1024 step
    OSD_MARQUEE_LENGTH_512,            ///< 512  step
    OSD_MARQUEE_LENGTH_256,            ///< 256  step
    OSD_MARQUEE_LENGTH_128,            ///< 128  step
    OSD_MARQUEE_LENGTH_64,             ///< 64   step
    OSD_MARQUEE_LENGTH_32,             ///< 32   step
    OSD_MARQUEE_LENGTH_16,             ///< 16   step
    OSD_MARQUEE_LENGTH_8,              ///< 8    step
    OSD_MARQUEE_LENGTH_4,              ///< 4    step
    OSD_MARQUEE_LENGTH_MAX
} osd_marquee_length_t;

typedef struct osd_marquee_param {
    osd_marquee_length_t        length; ///< OSD marquee length control
    u8                          speed;  ///< OSD marquee speed  control, 0~3, 0:fastest, 3:slowest
} osd_marquee_param_t;

typedef struct osd_win_param {
    WIN_ALIGN_T align;        ///< OSD window auto align type
    u16         x_start;      ///< OSD window X start position, 0~4095, must be multiple of 4
    u16         y_start;      ///< OSD window Y start position, 0~4095
    u16         width;        ///< OSD window width, 1~4096, must be multiple of 4
    u16         height;       ///< OSD window height,1~4096
    u16         h_sp;         ///< OSD window row space, 0~4095 pixel, must 4 align
    u16         v_sp;         ///< OSD window column space, 0~4095 line
    u8          h_wd_num;     ///< OSD window horizontal word number minus one, 0~63, 0: means 1 word
    u8          v_wd_num;     ///< OSD window vertical word number minus one, 0~63, 0: means 1 word
    u32         wd_addr;      ///< OSD window word address at osd font display ram
} osd_win_param_t;

typedef enum {
    OSD_FONT_ZOOM_NONE = 0,        ///< disable zoom
    OSD_FONT_ZOOM_2X,              ///< horizontal and vertical zoom in 2x
    OSD_FONT_ZOOM_3X,              ///< horizontal and vertical zoom in 3x
    OSD_FONT_ZOOM_4X,              ///< horizontal and vertical zoom in 4x
    OSD_FONT_ZOOM_ONE_HALF,        ///< horizontal and vertical zoom out 2x

    OSD_FONT_ZOOM_H2X_V1X = 8,     ///< horizontal zoom in 2x
    OSD_FONT_ZOOM_H4X_V1X,         ///< horizontal zoom in 4x
    OSD_FONT_ZOOM_H4X_V2X,         ///< horizontal zoom in 4x and vertical zoom in 2x

    OSD_FONT_ZOOM_H1X_V2X = 12,    ///< vertical zoom in 2x
    OSD_FONT_ZOOM_H1X_V4X,         ///< vertical zoom in 4x
    OSD_FONT_ZOOM_H2X_V4X,         ///< horizontal zoom in 2x and vertical zoom in 4x
    OSD_FONT_ZOOM_MAX
} osd_font_zoom_t;

typedef struct osd_color {
    u8      fg_color;           ///< OSD font foreground color, palette index 0~15
    u8      bg_color;           ///< OSD font background color, palette index 0~15
} osd_color_t;

typedef enum {
    ALPHA_0 = 0,               ///< alpha 0%
    ALPHA_25,                  ///< alpha 25%
    ALPHA_37_5,                ///< alpha 37.5%
    ALPHA_50,                  ///< alpha 50%
    ALPHA_62_5,                ///< alpha 62.5%
    ALPHA_75,                  ///< alpha 75%
    ALPHA_87_5,                ///< alpha 87.5%
    ALPHA_100,                 ///< alpha 100%
    ALPHA_MAX
} alpha_t;

typedef struct osd_alpha {
    alpha_t      fg_alpha;             ///< OSD window font transparency
    alpha_t      bg_alpha;             ///< OSD window background transparency
} osd_alpha_t;

typedef enum {
    OSD_BORDER_TYPE_BACKGROUND = 0,    ///< treat border as background
    OSD_BORDER_TYPE_FONT,              ///< treat border as font
    OSD_BORDER_TYPE_MAX
} osd_border_type_t;

typedef struct osd_border_param {
    u8                      width;     ///< OSD window border width, 0~7, border width = 4x(n+1) pixels
    osd_border_type_t       type;      ///< OSD window border transparency type
    u8                      color;     ///< OSD window border color, palette index 0~15
} osd_border_param_t;

/*************************************************************************************
 *  OSD_Mask Parameter Definition, OSD Window as Mask Window
 *************************************************************************************/
 typedef struct osd_mask_win_param {
    WIN_ALIGN_T align;              ///< OSD_Mask window auto align type
    u16         x_start;            ///< OSD_Mask window X start position, 0~4095, must be multiple of 4
    u16         y_start;            ///< OSD_Mask window Y start position, 0~4095
    u16         width;              ///< OSD_Mask window X end position,   1~4096, must be multiple of 4
    u16         height;             ///< OSD_Mask window Y start position, 1~4096
    u8          color;              ///< OSD_Mask window color, palette index 0~3, color max depend on dev_osd_mask_palette_max
} osd_mask_win_param_t;

/*************************************************************************************
 * Mask Parameter Definition, Channel Global Mask Window
 *************************************************************************************/
typedef struct mask_win_param {
    u16     x_start;            ///< Mask window X start position, 0~4095, must be multiple of 4
    u16     y_start;            ///< Mask window Y start position, 0~4095
    u16     width;              ///< Mask window X end position,   1~4096, must be multiple of 4
    u16     height;             ///< Mask window Y start position, 1~4096
    u8      color;              ///< Mask window color, palette index 0~15
} mask_win_param_t;

typedef enum {
    MASK_BORDER_TYPE_HOLLOW = 0,
    MASK_BORDER_TYPE_TRUE
} mask_border_type_t;

typedef struct mask_border {
    u8                      width;      ///< Mask window border width when hollow on, 0~15, border width = 2x(n+1) pixels
    mask_border_type_t      type;       ///< Mask window hollow/true
} mask_border_t;

/*************************************************************************************
 * Mark Parameter Definition
 *************************************************************************************/
typedef enum {
    MARK_ZOOM_1X = 0,      ///< zoom in lx
    MARK_ZOOM_2X,          ///< zoom in 2X
    MARK_ZOOM_4X,          ///< zoom in 4x
    MARK_ZOOM_MAX
} mark_zoom_t;

typedef enum {
    MARK_DIM_16 = 2,       ///< 16  pixel/line
    MARK_DIM_32,           ///< 32  pixel/line
    MARK_DIM_64,           ///< 64  pixel/line
    MARK_DIM_128,          ///< 128 pixel/line
    MARK_DIM_256,          ///< 256 pixel/line
    MARK_DIM_512,          ///< 512 pixel/line
    MARK_DIM_MAX
} mark_dim_t;

typedef struct mark_win_param {
    WIN_ALIGN_T align;              ///< mark window auto align type
    u8          mark_id;            ///< mark image index, 0~255
    u16         x_start;            ///< x start position of mark window, 0~4095, must be multiple of 4
    u16         y_start;            ///< y start position of mark window, 0~4095
    mark_zoom_t zoom;               ///< mark zoom control
    alpha_t     alpha;              ///< mark alpha control
} mark_win_param_t;

typedef struct mark_img {
    u8          id;                 ///< mark index, 0~255
    u16         start_addr;         ///< mark memory address in mark ram
    mark_dim_t  x_dim;              ///< mark x dimension
    mark_dim_t  y_dim;              ///< mark x dimension
    void        *img_buf;           ///< mark image buffer, data must be YUV422 format
} mark_img_t;

/*************************************************************************************
 *  Export Function Prototype
 *************************************************************************************/
int api_hw_get_ability(int type, hw_ability_t *ability);
int api_osd_set_char(int fd, int type, osd_char_t *osd_char);
int api_osd_remove_char(int fd, int type, int font);    ///< font: font index, -1 means to remove all character
int api_osd_set_disp_string(int fd, int type, u32 start, u16 *font_data, u16 font_num);
int api_osd_set_priority(int fd, int type, osd_priority_t priority);
int api_osd_get_priority(int fd, int type, osd_priority_t *priority);
int api_osd_set_smooth(int fd, int type, osd_smooth_t *param);
int api_osd_get_smooth(int fd, int type, osd_smooth_t *param);
int api_osd_set_marquee_mode(int fd, int type, int win_idx, osd_marquee_mode_t mode);
int api_osd_get_marquee_mode(int fd, int type, int win_idx, osd_marquee_mode_t *mode);
int api_osd_set_marquee_param(int fd, int type, osd_marquee_param_t *param);
int api_osd_get_marquee_param(int fd, int type, osd_marquee_param_t *param);
int api_osd_win_enable(int fd, int type, int win_idx);
int api_osd_win_disable(int fd, int type, int win_idx);
int api_osd_set_win_param(int fd, int type, int win_idx, osd_win_param_t *param);
int api_osd_get_win_param(int fd, int type, int win_idx, osd_win_param_t *param);
int api_osd_set_font_zoom(int fd, int type, int win_idx, osd_font_zoom_t zoom);
int api_osd_get_font_zoom(int fd, int type, int win_idx, osd_font_zoom_t *zoom);
int api_osd_set_color(int fd, int type, int win_idx, osd_color_t *param);
int api_osd_get_color(int fd, int type, int win_idx, osd_color_t *param);
int api_osd_set_alpha(int fd, int type, int win_idx, osd_alpha_t *param);
int api_osd_get_alpha(int fd, int type, int win_idx, osd_alpha_t *param);
int api_osd_set_border_enable(int fd, int type, int win_idx);
int api_osd_set_border_disable(int fd, int type, int win_idx);
int api_osd_set_border_param(int fd, int type, int win_idx, osd_border_param_t *param);
int api_osd_get_border_param(int fd, int type, int win_idx, osd_border_param_t *param);

int api_osd_mask_win_enable(int fd, int type, int win_idx);
int api_osd_mask_win_disable(int fd, int type, int win_idx);
int api_osd_mask_set_win_param(int fd, int type, int win_idx, osd_mask_win_param_t *param);
int api_osd_mask_get_win_param(int fd, int type, int win_idx, osd_mask_win_param_t *param);
int api_osd_mask_set_alpha(int fd, int type, int win_idx, alpha_t alpha);
int api_osd_mask_get_alpha(int fd, int type, int win_idx, alpha_t *alpha);

int api_mask_win_enable(int fd, int type, int win_idx);
int api_mask_win_disable(int fd, int type, int win_idx);
int api_mask_set_win_param(int fd, int type, int win_idx, mask_win_param_t *param);
int api_mask_get_win_param(int fd, int type, int win_idx, mask_win_param_t *param);
int api_mask_set_alpha(int fd, int type, int win_idx, alpha_t alpha);
int api_mask_get_alpha(int fd, int type, int win_idx, alpha_t *alpha);
int api_mask_set_border(int fd, int type, int win_idx, mask_border_t *param);
int api_mask_get_border(int fd, int type, int win_idx, mask_border_t *param);

int api_set_palette(int fd, int type, int idx, int crcby);
int api_get_palette(int fd, int type, int idx, int *crcby);

int api_img_border_enable(int fd, int type);
int api_img_border_disable(int fd, int type);
int api_img_border_set_color(int fd, int type, int color);
int api_img_border_get_color(int fd, int type, int *color);
int api_img_border_set_width(int fd, int type, int width);
int api_img_border_get_width(int fd, int type, int *width);

int api_mark_enable(int fd, int type, int win_idx);
int api_mark_disable(int fd, int type, int win_idx);
int api_mark_set_win_param(int fd, int type, int win_idx, mark_win_param_t *param);
int api_mark_get_win_param(int fd, int type, int win_idx, mark_win_param_t *param);
int api_mark_load_image(int fd, int type, mark_img_t *img);

#endif  /* __OSD_API_H__ */
