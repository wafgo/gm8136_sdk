/**
 * @file osd_dispatch.c
 *  osd dispatcher interface
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision$
 * $Date$
 *
 * ChangeLog:
 *  $Log$
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <osd_api.h>
#include <api_if.h>
#include <vcap_api.h>

int api_hw_get_ability(int type, hw_ability_t *ability)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_get_hw_ability(ability);
            break;
        case OSD_TYPE_SCL:
            ret = scl_get_hw_ability(ability);
            break;
        default:
            return -1;
    }

    return ret;
}

int api_osd_set_char(int fd, int type, osd_char_t *osd_char)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_char(fd, osd_char);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_set_char(fd, osd_char);
            break;
        default:
            return -1;
    }

    return ret;
}

int api_osd_remove_char(int fd, int type, int font)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_remove_char(fd, font);
            break;
        default:
            return -1;
    }

    return ret;
}

int api_osd_set_disp_string(int fd, int type, u32 start, u16 *font_data, u16 font_num)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_disp_string(fd, start, font_data, font_num);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_set_disp_string(fd, start, font_data, font_num);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set mark & osd priority */
int api_osd_set_priority(int fd, int type, osd_priority_t priority)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_priority(fd, priority);
            break;
        default:
            return -1;
    }

    return ret;
}

/* get mark & osd priority */
int api_osd_get_priority(int fd, int type, osd_priority_t *priority)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_get_priority(fd, priority);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd font smooth enable*/
int api_osd_set_smooth(int fd, int type, osd_smooth_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_font_smooth_param(fd, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_set_smooth(fd, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* get osd font smooth */
int api_osd_get_smooth(int fd, int type, osd_smooth_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_get_font_smooth_param(fd, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_get_smooth(fd, param);
            break;
        default:
            return -1;
    }

    return ret;
}


/* set osd marquee */
int api_osd_set_marquee_param(int fd, int type, osd_marquee_param_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_font_marquee_param(fd, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd marquee */
int api_osd_get_marquee_param(int fd, int type, osd_marquee_param_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_get_font_marquee_param(fd, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd window enable*/
int api_osd_win_enable(int fd, int type, int win_idx)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_win_enable(fd, win_idx);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_win_enable(fd, win_idx);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd window disable*/
int api_osd_win_disable(int fd, int type, int win_idx)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_win_disable(fd, win_idx);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_win_disable(fd, win_idx);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd window position */
int api_osd_set_win_param(int fd, int type, int win_idx, osd_win_param_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_win(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_set_win_param(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* get osd window position */
int api_osd_get_win_param(int fd, int type, int win_idx, osd_win_param_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_get_win(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_get_win_param(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd font zoom */
int api_osd_set_font_zoom(int fd, int type, int win_idx, osd_font_zoom_t zoom)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_font_zoom(fd, win_idx, zoom);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_set_font_zoom(fd, win_idx, zoom);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd font zoom */
int api_osd_get_font_zoom(int fd, int type, int win_idx, osd_font_zoom_t *zoom)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_get_font_zoom(fd, win_idx, zoom);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_get_font_zoom(fd, win_idx, zoom);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd font foreground/background color */
int api_osd_set_color(int fd, int type, int win_idx, osd_color_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_color(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_set_color(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd font foreground/background color */
int api_osd_get_color(int fd, int type, int win_idx, osd_color_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_get_color(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_get_color(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd font/window transparency */
int api_osd_set_alpha(int fd, int type, int win_idx, osd_alpha_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_alpha(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_set_alpha(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd font/window transparency */
int api_osd_get_alpha(int fd, int type, int win_idx, osd_alpha_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_get_alpha(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_get_alpha(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd marquee */
int api_osd_set_marquee_mode(int fd, int type, int win_idx, osd_marquee_mode_t mode)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_font_marquee_mode(fd, win_idx, mode);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd marquee */
int api_osd_get_marquee_mode(int fd, int type, int win_idx, osd_marquee_mode_t *mode)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_get_font_marquee_mode(fd, win_idx, mode);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd border enable */
int api_osd_set_border_enable(int fd, int type, int win_idx)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_border_enable(fd, win_idx);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_set_border_enable(fd, win_idx);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd border disable */
int api_osd_set_border_disable(int fd, int type, int win_idx)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_border_disable(fd, win_idx);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_set_border_disable(fd, win_idx);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd border param */
int api_osd_set_border_param(int fd, int type, int win_idx, osd_border_param_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_border_param(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_set_border_param(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* get osd border param */
int api_osd_get_border_param(int fd, int type, int win_idx, osd_border_param_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_get_border_param(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_osd_get_border_param(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set mask window on/off*/
int api_mask_win_enable(int fd, int type, int win_idx)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mask_enable(fd, win_idx);
            break;
        case OSD_TYPE_SCL:
            ret = scl_mask_win_enable(fd, win_idx);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set mask window on/off*/
int api_mask_win_disable(int fd, int type, int win_idx)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mask_disable(fd, win_idx);
            break;
        case OSD_TYPE_SCL:
            ret = scl_mask_win_disable(fd, win_idx);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set mask window */
int api_mask_set_win_param(int fd, int type, int win_idx, mask_win_param_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mask_set_win(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_mask_set_param(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* get mask window */
int api_mask_get_win_param(int fd, int type, int win_idx, mask_win_param_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mask_get_win(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_mask_get_param(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set mask alpha */
int api_mask_set_alpha(int fd, int type, int win_idx, alpha_t alpha)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mask_set_alpha(fd, win_idx, alpha);
            break;
        case OSD_TYPE_SCL:
            ret = scl_mask_set_alpha(fd, win_idx, alpha);
            break;
        default:
            return -1;
    }

    return ret;
}

/* get mask alpha */
int api_mask_get_alpha(int fd, int type, int win_idx, alpha_t *alpha)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mask_get_alpha(fd, win_idx, alpha);
            break;
        case OSD_TYPE_SCL:
            ret = scl_mask_get_alpha(fd, win_idx, alpha);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set mask border */
int api_mask_set_border(int fd, int type, int win_idx, mask_border_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mask_set_border(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_mask_set_border(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set mask border */
int api_mask_get_border(int fd, int type, int win_idx, mask_border_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mask_get_border(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
            ret = scl_mask_get_border(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set palette */
int api_set_palette(int fd, int type, int idx, int crcby)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_palette_set(fd, idx, crcby);
            break;
        case OSD_TYPE_SCL:
            ret = scl_set_palette(fd, idx, crcby);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set palette */
int api_get_palette(int fd, int type, int idx, int *crcby)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_palette_get(fd, idx, crcby);
            break;
        case OSD_TYPE_SCL:
            ret = scl_get_palette(fd, idx, crcby);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set image border enable */
int api_img_border_enable(int fd, int type)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_img_border_enable(fd);
            break;
        case OSD_TYPE_SCL:
            ret = scl_img_border_enable(fd);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set image border disable */
int api_img_border_disable(int fd, int type)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_img_border_disable(fd);
            break;
        case OSD_TYPE_SCL:
            ret = scl_img_border_disable(fd);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set image border color */
int api_img_border_set_color(int fd, int type, int color)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_img_border_color(fd, color);
            break;
        case OSD_TYPE_SCL:
            ret = scl_set_img_border_color(fd, color);
            break;
        default:
            return -1;
    }

    return ret;
}

/* get image border color */
int api_img_border_get_color(int fd, int type, int *color)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_get_img_border_color(fd, color);
            break;
        case OSD_TYPE_SCL:
            ret = scl_get_img_border_color(fd, color);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set image border width */
int api_img_border_set_width(int fd, int type, int width)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_set_img_border_width(fd, width);
            break;
        case OSD_TYPE_SCL:
            ret = scl_set_img_border_width(fd, width);
            break;
        default:
            return -1;
    }

    return ret;
}

/* get image border width */
int api_img_border_get_width(int fd, int type, int *width)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_get_img_border_width(fd, width);
            break;
        case OSD_TYPE_SCL:
            ret = scl_get_img_border_width(fd, width);
            break;
        default:
            return -1;
    }

    return ret;
}

/* mark enable */
int api_mark_enable(int fd, int type, int win_idx)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mark_win_enable(fd, win_idx);
            break;
        default:
            return -1;
    }

    return ret;
}

/* mark disable */
int api_mark_disable(int fd, int type, int win_idx)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mark_win_disable(fd, win_idx);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set mark window param */
int api_mark_set_win_param(int fd, int type, int win_idx, mark_win_param_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mark_set_win(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* get mark window param */
int api_mark_get_win_param(int fd, int type, int win_idx, mark_win_param_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mark_get_win(fd, win_idx, param);
            break;
        default:
            return -1;
    }

    return ret;
}

/* load mark image */
int api_mark_load_image(int fd, int type, mark_img_t *img)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_mark_load_image(fd, img);
            break;
        default:
            return -1;
    }

    return ret;
}

/* set osd_mask window enable*/
int api_osd_mask_win_enable(int fd, int type, int win_idx)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_mask_win_enable(fd, win_idx);
            break;
        case OSD_TYPE_SCL:
        default:
            return -1;
    }

    return ret;
}

/* set osd_mask window disable*/
int api_osd_mask_win_disable(int fd, int type, int win_idx)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_mask_win_disable(fd, win_idx);
            break;
        case OSD_TYPE_SCL:
        default:
            return -1;
    }

    return ret;
}

/* set osd_mask window parameter */
int api_osd_mask_set_win_param(int fd, int type, int win_idx, osd_mask_win_param_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_mask_set_win(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
        default:
            return -1;
    }

    return ret;
}

/* get osd_mask window parameter */
int api_osd_mask_get_win_param(int fd, int type, int win_idx, osd_mask_win_param_t *param)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_mask_get_win(fd, win_idx, param);
            break;
        case OSD_TYPE_SCL:
        default:
            return -1;
    }

    return ret;
}

/* set osd_mask alpha */
int api_osd_mask_set_alpha(int fd, int type, int win_idx, alpha_t alpha)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_mask_set_alpha(fd, win_idx, alpha);
            break;
        case OSD_TYPE_SCL:
        default:
            return -1;
    }

    return ret;
}

/* get osd_mask alpha */
int api_osd_mask_get_alpha(int fd, int type, int win_idx, alpha_t *alpha)
{
    int ret;

    switch(type) {
        case OSD_TYPE_CAP:
            ret = vcap_api_osd_mask_get_alpha(fd, win_idx, alpha);
            break;
        case OSD_TYPE_SCL:
        default:
            return -1;
    }

    return ret;
}

static int __init osd_dispatch_init(void)
{
    return 0;
}

static void __exit osd_dispatch_exit(void)
{
    return;
}

EXPORT_SYMBOL(api_hw_get_ability);
EXPORT_SYMBOL(api_osd_set_char);
EXPORT_SYMBOL(api_osd_set_disp_string);
EXPORT_SYMBOL(api_osd_set_priority);
EXPORT_SYMBOL(api_osd_get_priority);
EXPORT_SYMBOL(api_osd_set_smooth);
EXPORT_SYMBOL(api_osd_get_smooth);
EXPORT_SYMBOL(api_osd_set_marquee_mode);
EXPORT_SYMBOL(api_osd_get_marquee_mode);
EXPORT_SYMBOL(api_osd_set_marquee_param);
EXPORT_SYMBOL(api_osd_get_marquee_param);
EXPORT_SYMBOL(api_osd_win_enable);
EXPORT_SYMBOL(api_osd_win_disable);
EXPORT_SYMBOL(api_osd_set_win_param);
EXPORT_SYMBOL(api_osd_get_win_param);
EXPORT_SYMBOL(api_osd_set_font_zoom);
EXPORT_SYMBOL(api_osd_get_font_zoom);
EXPORT_SYMBOL(api_osd_set_color);
EXPORT_SYMBOL(api_osd_get_color);
EXPORT_SYMBOL(api_osd_set_alpha);
EXPORT_SYMBOL(api_osd_get_alpha);
EXPORT_SYMBOL(api_osd_set_border_enable);
EXPORT_SYMBOL(api_osd_set_border_disable);
EXPORT_SYMBOL(api_osd_set_border_param);
EXPORT_SYMBOL(api_osd_get_border_param);

EXPORT_SYMBOL(api_osd_mask_win_enable);
EXPORT_SYMBOL(api_osd_mask_win_disable);
EXPORT_SYMBOL(api_osd_mask_set_win_param);
EXPORT_SYMBOL(api_osd_mask_get_win_param);
EXPORT_SYMBOL(api_osd_mask_set_alpha);
EXPORT_SYMBOL(api_osd_mask_get_alpha);

EXPORT_SYMBOL(api_mask_win_enable);
EXPORT_SYMBOL(api_mask_win_disable);
EXPORT_SYMBOL(api_mask_set_win_param);
EXPORT_SYMBOL(api_mask_get_win_param);
EXPORT_SYMBOL(api_mask_set_alpha);
EXPORT_SYMBOL(api_mask_get_alpha);
EXPORT_SYMBOL(api_mask_set_border);
EXPORT_SYMBOL(api_mask_get_border);

EXPORT_SYMBOL(api_set_palette);
EXPORT_SYMBOL(api_get_palette);

EXPORT_SYMBOL(api_img_border_enable);
EXPORT_SYMBOL(api_img_border_disable);
EXPORT_SYMBOL(api_img_border_set_color);
EXPORT_SYMBOL(api_img_border_get_color);
EXPORT_SYMBOL(api_img_border_set_width);
EXPORT_SYMBOL(api_img_border_get_width);

EXPORT_SYMBOL(api_mark_enable);
EXPORT_SYMBOL(api_mark_disable);
EXPORT_SYMBOL(api_mark_set_win_param);
EXPORT_SYMBOL(api_mark_get_win_param);
EXPORT_SYMBOL(api_mark_load_image);

module_init(osd_dispatch_init);
module_exit(osd_dispatch_exit);

MODULE_DESCRIPTION("Grain Media OSD Dispatcher Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
