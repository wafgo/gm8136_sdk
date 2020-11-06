#ifndef __API_IF_H__
#define __API_IF_H__

#include <osd_api.h>

extern int scl_osd_set_char(int fd, osd_char_t *osd_char);
extern int scl_osd_set_disp_string(int fd, u32 start, u16 *font_data, u16 font_num);

extern int scl_osd_set_smooth(int fd, osd_smooth_t *param);
extern int scl_osd_get_smooth(int fd, osd_smooth_t *param);
extern int scl_osd_win_enable(int fd, int win_idx);
extern int scl_osd_win_disable(int fd, int win_idx);
extern int scl_osd_set_win_param(int fd, int win_idx, osd_win_param_t *param);
extern int scl_osd_get_win_param(int fd, int win_idx, osd_win_param_t *param);
extern int scl_osd_set_font_zoom(int fd, int win_idx, osd_font_zoom_t zoom);
extern int scl_osd_get_font_zoom(int fd, int win_idx, osd_font_zoom_t *zoom);
extern int scl_osd_set_color(int fd, int win_idx, osd_color_t *param);
extern int scl_osd_get_color(int fd, int win_idx, osd_color_t *param);
extern int scl_osd_set_alpha(int fd, int win_idx, osd_alpha_t *param);
extern int scl_osd_get_alpha(int fd, int win_idx, osd_alpha_t *param);
extern int scl_osd_set_border_enable(int fd, int win_idx);
extern int scl_osd_set_border_disable(int fd, int win_idx);
extern int scl_osd_set_border_param(int fd, int win_idx, osd_border_param_t *param);
extern int scl_osd_get_border_param(int fd, int win_idx, osd_border_param_t *param);

extern int scl_mask_win_enable(int fd, int win_idx);
extern int scl_mask_win_disable(int fd, int win_idx);
extern int scl_mask_set_param(int fd, int win_idx, mask_win_param_t *param);
extern int scl_mask_get_param(int fd, int win_idx, mask_win_param_t *param);
extern int scl_mask_set_alpha(int fd, int win_idx, alpha_t alpha);
extern int scl_mask_get_alpha(int fd, int win_idx, alpha_t *alpha);
extern int scl_mask_set_border(int fd, int win_idx, mask_border_t *param);
extern int scl_mask_get_border(int fd, int win_idx, mask_border_t *param);

extern int scl_set_palette(int fd, int idx, int crcby);
extern int scl_get_palette(int fd, int idx, int *crcby);

extern int scl_img_border_enable(int fd);
extern int scl_img_border_disable(int fd);
extern int scl_set_img_border_color(int fd, int color);
extern int scl_get_img_border_color(int fd, int *color);
extern int scl_set_img_border_width(int fd, int width);
extern int scl_get_img_border_width(int fd, int *width);

extern int scl_get_hw_ability(hw_ability_t *ability);
#endif
