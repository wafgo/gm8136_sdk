#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/interrupt.h>

#include <linux/ioport.h>
#include <linux/platform_device.h>

#include "fscaler300.h"
#include <api_if.h>
#include "fscaler300_osd.h"

#ifdef VIDEOGRAPH_INC
#include "fscaler300_vg.h"
#include <video_entity.h>   //include video entity manager
#include <log.h>    //include log system "printm","damnit"...
#define MODULE_NAME         "SL"    //<<<<<<<<<< Modify your module name (two bytes) >>>>>>>>>>>>>>
#endif

extern int max_minors;

int scl_osd_set_char(int fd, osd_char_t *osd_char)
{
    int dev;
    scl_osd_char_t o_char;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    o_char.font = osd_char->font;
    memcpy(o_char.fbitmap, osd_char->fbitmap, sizeof(o_char.fbitmap));

    for (dev = 0; dev < priv.eng_num; dev++)
        fscaler300_osd_set_char(dev, &o_char);

    return 0;
}

int scl_osd_set_disp_string(int fd, u32 start, u16 *font_data, u16 font_num)
{
    int dev;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    for (dev = 0; dev < priv.eng_num; dev++)
        fscaler300_osd_set_disp_string(dev, start, font_data, font_num);

    return 0;
}

/* set image border enable */
int scl_img_border_enable(int fd)
{
    //priv.global_param.init.img_border.res_border_en[0] = 1;

    return 0;
}

/* set image border disable */
int scl_img_border_disable(int fd)
{
    //priv.global_param.init.img_border.res_border_en[0] = 0;

    return 0;
}

/* set image border color*/
int scl_set_img_border_color(int fd, int color)
{
#if 0
    int dev;

    for (dev = 0; dev < priv.eng_num; dev++)
        fscaler300_set_img_border_color(dev, color);
#endif
    return 0;
}

/* get image border color*/
int scl_get_img_border_color(int fd, int *color)
{
    //*color = priv.global_param.init.img_border.color;

    return 0;
}

/* set image border width*/
int scl_set_img_border_width(int fd, int width)
{
    //priv.global_param.init.img_border.res_border_width[0] = width;

    return 0;
}

/* get image border width*/
int scl_get_img_border_width(int fd, int *width)
{
    //*width = priv.global_param.init.img_border.res_border_width[0];

    return 0;
}

/* set osd font smooth enable */
int scl_osd_set_smooth(int fd, osd_smooth_t *param)
{
    int dev;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    for (dev = 0; dev < priv.eng_num; dev++)
        fscaler300_osd_set_smooth(dev, param);

    return 0;
}


/* get osd font smooth level */
int scl_osd_get_smooth(int fd, osd_smooth_t *param)
{
    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    param->onoff = priv.global_param.init.osd_smooth.onoff;
    param->level = priv.global_param.init.osd_smooth.level;

    return 0;
}

/* set osd window enable */
int scl_osd_win_enable(int fd, int win_idx)
{
    int engine, minor, index;
    int osd_tmp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    down(&priv.sema_lock);

    /* get temporal ping pong buffer */
    osd_tmp_sel = atomic_read(&priv.global_param.chn_global[index].osd_tmp_sel[win_idx]);

    if (osd_tmp_sel == 0) {
        printk("[SCL_OSD] can't get temp ping pong buffer select\n");
        return - 1;
    }

    atomic_set(&priv.global_param.chn_global[index].osd_pp_sel[win_idx], (osd_tmp_sel - 1));

    priv.global_param.chn_global[index].osd[osd_tmp_sel - 1][win_idx].enable = 1;
    /* set osd_tmp_sel  = 0 */
    atomic_set(&priv.global_param.chn_global[index].osd_tmp_sel[win_idx], 0);

    up(&priv.sema_lock);

    return 0;
}

/* set osd window disable */
int scl_osd_win_disable(int fd, int win_idx)
{
    int engine, minor, index;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    down(&priv.sema_lock);

    atomic_set(&priv.global_param.chn_global[index].osd_pp_sel[win_idx], 0);

    up(&priv.sema_lock);

    return 0;
}

/* set osd window position */
int scl_osd_set_win_param(int fd, int win_idx, osd_win_param_t *param)
{
    int engine, minor, index;
    int osd_pp_sel, temp;
    int ref_cnt1 = 0, ref_cnt2 = 0;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    /* get current ping pong buffer info */
    ref_cnt1 = atomic_read(&priv.global_param.chn_global[index].osd_ref_cnt[0][win_idx]);
    ref_cnt2 = atomic_read(&priv.global_param.chn_global[index].osd_ref_cnt[1][win_idx]);
    /* if both of they are reference by jobs, return fail */
    if (ref_cnt1 != 0 && ref_cnt2 != 0) {
        printk("[SCL_OSD] no ping pong buffer to set\n");
        return -1;
    }
    /* get current ping pong buffer & set next ping pong buffer */
    osd_pp_sel = atomic_read(&priv.global_param.chn_global[index].osd_pp_sel[win_idx]);

    if (osd_pp_sel == 0) {
        if (ref_cnt1 == 0) {
            atomic_set(&priv.global_param.chn_global[index].osd_tmp_sel[win_idx], 1);
            temp = 1;
        }
        else {
            atomic_set(&priv.global_param.chn_global[index].osd_tmp_sel[win_idx], 2);
            temp = 2;
        }
    }
    else if (osd_pp_sel == 1) {
        if (ref_cnt2 != 0) {
            printk("[SCL_OSD] no ping pong buffer to set\n");
            return -1;
        }
        else {
            atomic_set(&priv.global_param.chn_global[index].osd_tmp_sel[win_idx], 2);
            temp = 2;
        }
    }
    else {
        if (ref_cnt1 != 0) {
            printk("[SCL_OSD] no ping pong buffer to set\n");
            return -1;
        }
        else {
            atomic_set(&priv.global_param.chn_global[index].osd_tmp_sel[win_idx], 1);
            temp = 1;
        }
    }

    /* auto align */
    priv.global_param.chn_global[index].osd[temp - 1][win_idx].align_type = param->align;
    /* x pos */
    priv.global_param.chn_global[index].osd[temp - 1][win_idx].x_start = param->x_start;
    priv.global_param.chn_global[index].osd[temp - 1][win_idx].x_end = param->x_start + param->width - 1;
    /* y pos */
    priv.global_param.chn_global[index].osd[temp - 1][win_idx].y_start = param->y_start;
    priv.global_param.chn_global[index].osd[temp - 1][win_idx].y_end = param->y_start + param->height - 1;
    /* row space */
    priv.global_param.chn_global[index].osd[temp - 1][win_idx].row_sp = param->v_sp;
    /* column space */
    priv.global_param.chn_global[index].osd[temp - 1][win_idx].col_sp = param->h_sp;
    /* horizontal word number */
    priv.global_param.chn_global[index].osd[temp - 1][win_idx].h_wd_num = param->h_wd_num;
    /* vertical word number */
    priv.global_param.chn_global[index].osd[temp - 1][win_idx].v_wd_num = param->v_wd_num;

    return 0;
}

/* set osd window position */
int scl_osd_get_win_param(int fd, int win_idx, osd_win_param_t *param)
{
    int engine, minor, index;
    int osd_pp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    osd_pp_sel = atomic_read(&priv.global_param.chn_global[index].osd_pp_sel[win_idx]);

    if (osd_pp_sel == 0) {
        /* x pos */
        param->x_start = 0;
        param->width = 0;
        /* y pos */
        param->y_start = 0;
        param->height = 0;
        /* row space */
        param->v_sp = 0;
        /* column space */
        param->h_sp = 0;
        /* horizontal word number */
        param->h_wd_num = 0;
        /* vertical word number */
        param->v_wd_num = 0;
    }
    else {
        /* x pos */
        param->x_start = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].x_start;
        param->width = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].x_end - param->x_start + 1;
        /* y pos */
        param->y_start =priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].y_start;
        param->height = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].y_end - param->y_start + 1;
        /* row space */
        param->v_sp = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].row_sp;
        /* column space */
        param->h_sp = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].col_sp;
        /* horizontal word number */
        param->h_wd_num = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].h_wd_num;
        /* vertical word number */
        param->v_wd_num = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].v_wd_num;
    }

    return 0;
}

/* set osd font zoom */
int scl_osd_set_font_zoom(int fd, int win_idx, osd_font_zoom_t zoom)
{
    int engine, minor, index;
    int osd_tmp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    /* get temporal ping pong buffer */
    osd_tmp_sel = atomic_read(&priv.global_param.chn_global[index].osd_tmp_sel[win_idx]);

    if (osd_tmp_sel == 0) {
        printk("[SCL_OSD] can't get temp ping pong buffer select\n");
        return - 1;
    }

    priv.global_param.chn_global[index].osd[osd_tmp_sel - 1][win_idx].font_zoom = zoom;

    return 0;
}

/* set osd font zoom */
int scl_osd_get_font_zoom(int fd, int win_idx, osd_font_zoom_t *zoom)
{
    int engine, minor, index;
    int osd_pp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    osd_pp_sel = atomic_read(&priv.global_param.chn_global[index].osd_pp_sel[win_idx]);

    if (osd_pp_sel == 0)
        *zoom = 0;
    else
        *zoom = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].font_zoom;

    return 0;
}

/* set osd font foreground/background color */
int scl_osd_set_color(int fd, int win_idx, osd_color_t *param)
{
    int i;
    int engine, minor, index;
    u8  fg_color, bg_color;
    int osd_tmp_sel;
    int dev;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    fg_color = priv.engine[engine].font_data.fg_color;
    bg_color = priv.engine[engine].font_data.bg_color;
    /* check if color does not change, don't need to modify HW */
    if (param->fg_color == fg_color && param->bg_color == bg_color)
        return 0;

    /* get temporal ping pong buffer */
    osd_tmp_sel = atomic_read(&priv.global_param.chn_global[index].osd_tmp_sel[win_idx]);

    if (osd_tmp_sel == 0) {
        printk("[SCL_OSD] can't get temp ping pong buffer select\n");
        return - 1;
    }

    for (i = 0; i < OSD_MAX; i++)
        priv.global_param.chn_global[index].osd[osd_tmp_sel - 1][i].bg_color = param->bg_color;

    for (dev = 0; dev < priv.eng_num; dev++)
        fscaler300_osd_set_color(fd, dev, win_idx, param);

    return 0;
}

/* get osd font foreground/background color */
int scl_osd_get_color(int fd, int win_idx, osd_color_t *param)
{
    int engine;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);

    param->bg_color = priv.engine[engine].font_data.bg_color;
    param->fg_color = priv.engine[engine].font_data.fg_color;

    return 0;
}

/* set osd font/window transparency */
int scl_osd_set_alpha(int fd, int win_idx, osd_alpha_t *param)
{
    int engine, minor, index;
    int osd_tmp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    /* get temporal ping pong buffer */
    osd_tmp_sel = atomic_read(&priv.global_param.chn_global[index].osd_tmp_sel[win_idx]);

    if (osd_tmp_sel == 0) {
        printk("[SCL_OSD] can't get temp ping pong buffer select\n");
        return - 1;
    }

    /* font transparency */
    priv.global_param.chn_global[index].osd[osd_tmp_sel - 1][win_idx].font_alpha = param->fg_alpha;
    /* window transparency */
    priv.global_param.chn_global[index].osd[osd_tmp_sel - 1][win_idx].bg_alpha = param->bg_alpha;

    return 0;
}

/* get osd font/window transparency */
int scl_osd_get_alpha(int fd, int win_idx, osd_alpha_t *param)
{
    int engine, minor, index;
    int osd_pp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    osd_pp_sel = atomic_read(&priv.global_param.chn_global[index].osd_pp_sel[win_idx]);

    if (osd_pp_sel == 0) {
        /* font transparency */
        param->fg_alpha = 0;
        /* window transparency */
        param->bg_alpha = 0;
    }
    else {
        /* font transparency */
        param->fg_alpha = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].font_alpha;
        /* window transparency */
        param->bg_alpha = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].bg_alpha;
    }

    return 0;
}


/* set osd border onoff */
int scl_osd_set_border_enable(int fd, int win_idx)
{
    int engine, minor, index;
    int osd_tmp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    /* get temporal ping pong buffer */
    osd_tmp_sel = atomic_read(&priv.global_param.chn_global[index].osd_tmp_sel[win_idx]);

    if (osd_tmp_sel == 0) {
        printk("[SCL_OSD] can't get temp ping pong buffer select\n");
        return - 1;
    }

    /* onoff */
    priv.global_param.chn_global[index].osd[osd_tmp_sel - 1][win_idx].border_en = 1;

    return 0;
}

/* set osd border onoff */
int scl_osd_set_border_disable(int fd, int win_idx)
{
    int engine, minor, index;
    int osd_tmp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    /* get temporal ping pong buffer */
    osd_tmp_sel = atomic_read(&priv.global_param.chn_global[index].osd_tmp_sel[win_idx]);

    if (osd_tmp_sel == 0) {
        printk("[SCL_OSD] can't get temp ping pong buffer select\n");
        return - 1;
    }

    /* onoff */
    priv.global_param.chn_global[index].osd[osd_tmp_sel - 1][win_idx].border_en = 0;

    return 0;
}

/* set osd border param */
int scl_osd_set_border_param(int fd, int win_idx, osd_border_param_t *param)
{
    int engine, minor, index;
    int osd_tmp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    /* get temporal ping pong buffer */
    osd_tmp_sel = atomic_read(&priv.global_param.chn_global[index].osd_tmp_sel[win_idx]);

    if (osd_tmp_sel == 0) {
        printk("[SCL_OSD] can't get temp ping pong buffer select\n");
        return - 1;
    }

    /* width */
    priv.global_param.chn_global[index].osd[osd_tmp_sel - 1][win_idx].border_width = param->width;
    /* type */
    priv.global_param.chn_global[index].osd[osd_tmp_sel - 1][win_idx].border_type = param->type;
    /* color */
    priv.global_param.chn_global[index].osd[osd_tmp_sel - 1][win_idx].border_color = param->color;

    return 0;
}

/* get osd border param */
int scl_osd_get_border_param(int fd, int win_idx, osd_border_param_t *param)
{
    int engine, minor, index;
    int osd_pp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (GM8287 || GM8139) {
        printm(MODULE_NAME, "do not support OSD\n");
        return 0;
    }

    if (win_idx >= OSD_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, OSD_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    osd_pp_sel = atomic_read(&priv.global_param.chn_global[index].osd_pp_sel[win_idx]);

    if (osd_pp_sel == 0) {
        /* width */
        param->width = 0;
        /* type */
        param->type = 0;
        /* color */
        param->color = 0;
    }
    else {
        /* width */
        param->width = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].border_width;
        /* type */
        param->type = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].border_type;
        /* color */
        param->color = priv.global_param.chn_global[index].osd[osd_pp_sel - 1][win_idx].border_color;
    }

    return 0;
}

/* set mask window enable */
int scl_mask_win_enable(int fd, int win_idx)
{
    int engine, minor, index;
    int mask_tmp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (win_idx >= MASK_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, MASK_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    down(&priv.sema_lock);

    /* get temporal ping pong buffer */
    mask_tmp_sel = atomic_read(&priv.global_param.chn_global[index].mask_tmp_sel[win_idx]);

    if (mask_tmp_sel == 0) {
        printk("[SCL_OSD] can't get temp ping pong buffer select\n");
        return - 1;
    }

    atomic_set(&priv.global_param.chn_global[index].mask_pp_sel[win_idx], (mask_tmp_sel - 1));

    /* set mask_tmp_sel  = 0 */
    atomic_set(&priv.global_param.chn_global[index].mask_tmp_sel[win_idx], 0);

    priv.global_param.chn_global[index].mask[mask_tmp_sel - 1][win_idx].enable = 1;

    up(&priv.sema_lock);

    return 0;
}

/* set mask window disable */
int scl_mask_win_disable(int fd, int win_idx)
{
    int engine, minor, index;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (win_idx >= MASK_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, MASK_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    down(&priv.sema_lock);

    atomic_set(&priv.global_param.chn_global[index].mask_pp_sel[win_idx], 0);

    up(&priv.sema_lock);

    return 0;
}

/* set mask window */
int scl_mask_set_param(int fd, int win_idx, mask_win_param_t *param)
{
    int engine, minor, index;
    int mask_pp_sel, temp;
    int ref_cnt1 = 0, ref_cnt2 = 0;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (win_idx >= MASK_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, MASK_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    /* get current ping pong buffer info */
    ref_cnt1 = atomic_read(&priv.global_param.chn_global[index].mask_ref_cnt[0][win_idx]);
    ref_cnt2 = atomic_read(&priv.global_param.chn_global[index].mask_ref_cnt[1][win_idx]);
    /* if both of they are reference by jobs, return fail */
    if (ref_cnt1 != 0 && ref_cnt2 != 0) {
        printk("[SCL_MASK] no ping pong buffer to set\n");
        return -1;
    }
    /* get current ping pong buffer & set next ping pong buffer */
    mask_pp_sel = atomic_read(&priv.global_param.chn_global[index].mask_pp_sel[win_idx]);

    if (mask_pp_sel == 0) {
        if (ref_cnt1 == 0) {
            atomic_set(&priv.global_param.chn_global[index].mask_tmp_sel[win_idx], 1);
            temp = 1;
        }
        else {
            atomic_set(&priv.global_param.chn_global[index].mask_tmp_sel[win_idx], 2);
            temp = 2;
        }
    }
    else if (mask_pp_sel == 1) {
        if (ref_cnt2 != 0) {
            printk("[SCL_MASK] no ping pong buffer to set\n");
            return -1;
        }
        else {
            atomic_set(&priv.global_param.chn_global[index].mask_tmp_sel[win_idx], 2);
            temp = 2;
        }
    }
    else {
        if (ref_cnt1 != 0) {
            printk("[SCL_MASK] no ping pong buffer to set\n");
            return -1;
        }
        else {
            atomic_set(&priv.global_param.chn_global[index].mask_tmp_sel[win_idx], 1);
            temp = 1;
        }
    }

    /* x pos */
    priv.global_param.chn_global[index].mask[temp - 1][win_idx].x_start = param->x_start;
    priv.global_param.chn_global[index].mask[temp - 1][win_idx].x_end = param->x_start + param->width - 1;
    /* y pos */
    priv.global_param.chn_global[index].mask[temp - 1][win_idx].y_start = param->y_start;
    priv.global_param.chn_global[index].mask[temp - 1][win_idx].y_end = param->y_start + param->height - 1;
    /* color */
    priv.global_param.chn_global[index].mask[temp - 1][win_idx].color = param->color;

    return 0;
}

/* get mask window */
int scl_mask_get_param(int fd, int win_idx, mask_win_param_t *param)
{
    int engine, minor, index;
    int mask_pp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (win_idx >= MASK_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, MASK_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    mask_pp_sel = atomic_read(&priv.global_param.chn_global[index].mask_pp_sel[win_idx]);

    if (mask_pp_sel == 0) {
        param->x_start = 0;
        param->width = 0;
        param->y_start = 0;
        param->height = 0;
        param->color = 0;
    }
    else {
        /* x pos */
        param->x_start = priv.global_param.chn_global[index].mask[mask_pp_sel - 1][win_idx].x_start;
        param->width = priv.global_param.chn_global[index].mask[mask_pp_sel - 1][win_idx].x_end - param->x_start + 1;
        /* y pos */
        param->y_start = priv.global_param.chn_global[index].mask[mask_pp_sel - 1][win_idx].y_start;
        param->height = priv.global_param.chn_global[index].mask[mask_pp_sel - 1][win_idx].y_end - param->y_start + 1;
        /* color */
        param->color = priv.global_param.chn_global[index].mask[mask_pp_sel - 1][win_idx].color;
    }
    return 0;
}

/* set mask alpha */
int scl_mask_set_alpha(int fd, int win_idx, alpha_t alpha)
{
    int engine, minor, index;
    int mask_tmp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (win_idx >= MASK_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, MASK_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    /* get temporal ping pong buffer */
    mask_tmp_sel = atomic_read(&priv.global_param.chn_global[index].mask_tmp_sel[win_idx]);

    if (mask_tmp_sel == 0) {
        printk("[SCL_MASK] can't get temp ping pong buffer select\n");
        return - 1;
    }

    priv.global_param.chn_global[index].mask[mask_tmp_sel - 1][win_idx].alpha = alpha;

    return 0;
}

/* set mask alpha */
int scl_mask_get_alpha(int fd, int win_idx, alpha_t *alpha)
{
    int engine, minor, index;
    int mask_pp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (win_idx >= MASK_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, MASK_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    mask_pp_sel = atomic_read(&priv.global_param.chn_global[index].mask_pp_sel[win_idx]);

    if (mask_pp_sel == 0)
        *alpha = 0;
    else
        *alpha = priv.global_param.chn_global[index].mask[mask_pp_sel - 1][win_idx].alpha;

    return 0;
}

/* set mask border */
int scl_mask_set_border(int fd, int win_idx, mask_border_t *param)
{
    int engine, minor, index;
    int mask_tmp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (win_idx >= MASK_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, MASK_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    /* get temporal ping pong buffer */
    mask_tmp_sel = atomic_read(&priv.global_param.chn_global[index].mask_tmp_sel[win_idx]);

    if (mask_tmp_sel == 0) {
        printk("[SCL_MASK] can't get temp ping pong buffer select\n");
        return - 1;
    }

    /* border width */
    priv.global_param.chn_global[index].mask[mask_tmp_sel - 1][win_idx].border_width = param->width;
    /* true hollow */
    priv.global_param.chn_global[index].mask[mask_tmp_sel - 1][win_idx].true_hollow = param->type;

    return 0;
}

/* get mask border */
int scl_mask_get_border(int fd, int win_idx, mask_border_t *param)
{
    int engine, minor, index;
    int mask_pp_sel;

    /* mask & osd external api do not use, remove it for performance */
    return 0;

    if (win_idx >= MASK_MAX)
        panic("scaler win idx %d over MAX %d\n", win_idx, MASK_MAX);

    engine = ENTITY_FD_ENGINE(fd);
    minor = ENTITY_FD_MINOR(fd);
    index = MAKE_IDX(max_minors, ENTITY_FD_ENGINE(fd), ENTITY_FD_MINOR(fd));

    mask_pp_sel = atomic_read(&priv.global_param.chn_global[index].mask_pp_sel[win_idx]);

    if (mask_pp_sel == 0) {
        param->width = 0;
        param->type = 0;
    }
    else {
        /* border width */
        param->width = priv.global_param.chn_global[index].mask[mask_pp_sel - 1][win_idx].border_width;
        /* true hollow */
        param->type = priv.global_param.chn_global[index].mask[mask_pp_sel - 1][win_idx].true_hollow;
    }

    return 0;
}

/* set palette */
int scl_set_palette(int fd, int idx, int crcby)
{
    int dev = 0;

    if (idx >= SCALER300_PALETTE_MAX) {
        printm(MODULE_NAME, "palette idx %d over MAX %d\n", idx, SCALER300_PALETTE_MAX);
        return 0;
    }

    for (dev = 0; dev < priv.eng_num; dev++)
        fscaler300_set_palette(dev, idx, crcby);

    return 0;
}

/* get palette */
int scl_get_palette(int fd, int idx, int *crcby)
{
    if (idx >= SCALER300_PALETTE_MAX) {
        printm(MODULE_NAME, "palette idx %d over MAX %d\n", idx, SCALER300_PALETTE_MAX);
        return 0;
    }

    *crcby = priv.global_param.init.palette[idx].crcby;

    return 0;
}

int scl_get_hw_ability(hw_ability_t *ability)
{
    if (!ability)
        return -1;

    ability->ch_osd_win_max      = OSD_MAX;
    ability->ch_mask_win_max     = MASK_MAX;
    ability->ch_mark_win_max     = 0;
    ability->dev_palette_max     = SCALER300_PALETTE_MAX;
    ability->dev_osd_char_max    = SCALER300_OSD_FONT_CHAR_MAX;
    ability->dev_osd_disp_max    = SCALER300_OSD_DISPLAY_MAX;
    ability->dev_mark_ram_size   = 0;
    ability->dev_per_ch_path_max = SD_MAX;

    return 0;
}

EXPORT_SYMBOL(scl_osd_set_char);
EXPORT_SYMBOL(scl_osd_set_disp_string);
EXPORT_SYMBOL(scl_osd_set_smooth);
EXPORT_SYMBOL(scl_osd_get_smooth);
EXPORT_SYMBOL(scl_osd_win_enable);
EXPORT_SYMBOL(scl_osd_win_disable);
EXPORT_SYMBOL(scl_osd_set_win_param);
EXPORT_SYMBOL(scl_osd_get_win_param);
EXPORT_SYMBOL(scl_osd_set_font_zoom);
EXPORT_SYMBOL(scl_osd_get_font_zoom);
EXPORT_SYMBOL(scl_osd_set_color);
EXPORT_SYMBOL(scl_osd_get_color);
EXPORT_SYMBOL(scl_osd_set_alpha);
EXPORT_SYMBOL(scl_osd_get_alpha);
EXPORT_SYMBOL(scl_osd_set_border_enable);
EXPORT_SYMBOL(scl_osd_set_border_disable);
EXPORT_SYMBOL(scl_osd_set_border_param);
EXPORT_SYMBOL(scl_osd_get_border_param);

EXPORT_SYMBOL(scl_mask_win_enable);
EXPORT_SYMBOL(scl_mask_win_disable);
EXPORT_SYMBOL(scl_mask_set_param);
EXPORT_SYMBOL(scl_mask_get_param);
EXPORT_SYMBOL(scl_mask_set_alpha);
EXPORT_SYMBOL(scl_mask_get_alpha);
EXPORT_SYMBOL(scl_mask_set_border);
EXPORT_SYMBOL(scl_mask_get_border);

EXPORT_SYMBOL(scl_set_palette);
EXPORT_SYMBOL(scl_get_palette);

EXPORT_SYMBOL(scl_img_border_enable);
EXPORT_SYMBOL(scl_img_border_disable);
EXPORT_SYMBOL(scl_set_img_border_color);
EXPORT_SYMBOL(scl_get_img_border_color);
EXPORT_SYMBOL(scl_set_img_border_width);
EXPORT_SYMBOL(scl_get_img_border_width);

EXPORT_SYMBOL(scl_get_hw_ability);

