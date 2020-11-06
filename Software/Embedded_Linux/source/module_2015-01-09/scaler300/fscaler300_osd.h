#ifndef __FSCALER_OSD_H__
#define __FSCALER_OSD_H__

#include <osd_api.h>

typedef struct scl_osd_char {
    u16 font;                   ///< font index,
    u8  fbitmap[36];            ///< font bitmap, 12 bits per row, total 18 rows.
} scl_osd_char_t;

int fscaler300_set_palette(int dev, int idx, int crcby);
int fscaler300_osd_set_char(int dev, scl_osd_char_t *osd_char);
int fscaler300_osd_set_color(int fd, int dev, int win_idx, osd_color_t *osd_color);
int fscaler300_osd_set_disp_string(int dev, u32 start , u16 *font_data, u16 font_num);
int fscaler300_set_img_border_color(int dev, int color);
int fscaler300_osd_set_smooth(int dev, osd_smooth_t *param);
int fscaler300_osd_init(void);

#endif