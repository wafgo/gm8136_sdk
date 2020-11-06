#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <linux/platform_device.h>
#include <mach/platform/platform_io.h>
#include "ffb.h"
#include "dev.h"
#include "proc.h"
#include "debug.h"
#include "platform.h"

extern int lcd_disable;

#define MAX_CURS_WIDTH  64
#define MAX_CURS_HEIGHT 64
#define MAX_CURS_COLOR  15  /* offset 0x1204 ~ 0x123C */

#ifndef LCD200_V3
#error "The header file path is wrong!"
#endif

static int g_lcd200_idx;

static struct ffb_rgb rgb_1 = {
    .blue = {.offset = 0,.length = 1,},
    .green = {.offset = 0,.length = 0,},
    .red = {.offset = 0,.length = 0,},
    .transp = {.offset = 0,.length = 0,},
};

static struct ffb_rgb rgb_2 = {
    .blue = {.offset = 0,.length = 2,},
    .green = {.offset = 0,.length = 0,},
    .red = {.offset = 0,.length = 0,},
    .transp = {.offset = 0,.length = 0,},
};

static struct ffb_rgb rgb_8 = {
    .blue = {.offset = 0,.length = 3,},
    .green = {.offset = 3,.length = 3,},
    .red = {.offset = 6,.length = 2,},
    .transp = {.offset = 0,.length = 0,},
};

static struct ffb_rgb def_rgb_16 = {
    .blue = {.offset = 0,.length = 5,.msb_right = 0},
    .green = {.offset = 5,.length = 6,.msb_right = 0},
    .red = {.offset = 11,.length = 5,.msb_right = 0},
    .transp = {.offset = 0,.length = 0,.msb_right = 0},
};

static struct ffb_rgb def_rgb_32 = {
    .blue = {.offset = 0,.length = 8,.msb_right = 0},
    .green = {.offset = 8,.length = 8,.msb_right = 0},
    .red = {.offset = 16,.length = 8,.msb_right = 0},
    .transp = {.offset = 24,.length = 0,.msb_right = 0},
};

static struct ffb_rgb def_argb = {
    .blue = {.offset = 0,.length = 8,.msb_right = 0},
    .green = {.offset = 8,.length = 8,.msb_right = 0},
    .red = {.offset = 16,.length = 8,.msb_right = 0},
    .transp = {.offset = 24,.length = 4,.msb_right = 0},
};

/**
 *@brief Set and get main clock,base on pixel clock,from PMU
 */
static inline void lcd200_schedule_work(struct ffb_dev_info *info, u_int state);

/* workaround for LCD210, this workaround is only for TV-OUT */
void lcd210_set_prst(struct ffb_dev_info *ff_dev_info)
{
#ifdef LCD210
    u32 tmp;

    if (ff_dev_info->video_output_type < VOT_CCIR656)
        return;

    tmp = ioread32(ff_dev_info->io_base + PANEL_PARAM);
    tmp |= 0x1 << 19;
    iowrite32(tmp, ff_dev_info->io_base + PANEL_PARAM);
    tmp &= ~(0x1 << 19);
    iowrite32(tmp, ff_dev_info->io_base + PANEL_PARAM);
#endif
}

/**
 *@fn dev_lcd_scalar
 *@update lcd pannel scalar.
 * Note: Only for LCD pannel (RGB out). TV-out is not supported.
 * return 0 for success, others for fail
 */
int dev_lcd_scalar(struct lcd200_dev_info *lcd200_dev_info)
{
    struct ffb_dev_info *ff_dev_info = (struct ffb_dev_info *)lcd200_dev_info;
    u32 scal_hor_num = 0, scal_ver_num = 0, tmp;
    struct scalar_info scalar;
    int fir_sel = 0, sec_bypass_mode = 1;
    int factor;

    if (lcd200_dev_info == NULL)
        return -EINVAL;

    memcpy(&scalar, &lcd200_dev_info->scalar, sizeof(struct scalar_info));
    if (scalar.hor_no_in > 2047 || scalar.ver_no_in > 2047 || scalar.hor_no_out > 4097
        || scalar.ver_no_out > 4097)
        return -EINVAL;

    /* sanity check, the max capability of scaling is 2x2 */
    if ((scalar.hor_no_out > (scalar.hor_no_in << 1)) || (scalar.ver_no_out > (scalar.ver_no_in << 1))) {
        printk("LCD%d: The max scaling capability is only 2x2! \n", lcd200_dev_info->lcd200_idx);
        return -EINVAL;
    }

    if ((scalar.hor_no_out == scalar.hor_no_in) && (scalar.ver_no_out == scalar.ver_no_in)) {
        tmp = ioread32(ff_dev_info->io_base + FEATURE);
        if (tmp & (0x1 << 5)) {
            tmp &= (~(0x1 << 5));
            iowrite32(tmp, ff_dev_info->io_base + FEATURE);

            lcd200_schedule_work(ff_dev_info, C_REENABLE);
        }
        return 0;
    }

    /* Process first stage parameter
     */
    if ((scalar.hor_no_out >= scalar.hor_no_in) && (scalar.ver_no_out >= scalar.ver_no_in)) {
        // scaling up, bypass the first stage
        fir_sel = 0;
    } else if ((scalar.hor_no_out < scalar.hor_no_in) && (scalar.ver_no_out < scalar.ver_no_in)) {
        int tmp = 1;

        fir_sel = 0;

        /* scaling down, we need to know 1/4, 1/16 ...., but currently only 1/4 is supported. */
        for (factor = 2; factor <= 128; factor *= 2) {
            /* scaling down, we need to know 1/4, 1/16 ...., but currently only 1/4 is supported. */
            if ((factor * scalar.hor_no_out == scalar.hor_no_in)
                && (factor * scalar.ver_no_out == scalar.ver_no_in)) {
                fir_sel = tmp;
                break;
            }
            tmp++;
        }
    }

    /* only 1/4 is supported due to scalr clocking issue */
    if (unlikely(fir_sel > 1))
        return -EINVAL;

    /* 1st state is bypass, fill in the coefficients of the 2nd-stage scalar
     */
    sec_bypass_mode = 0;        //1 for bypass

    /* hor scaling up */
    if (scalar.hor_no_out >= scalar.hor_no_in)
        scal_hor_num = (scalar.hor_no_in + 1) * 256 / scalar.hor_no_out;
    else                        /* scaling down */
        scal_hor_num = (((scalar.hor_no_in + 1) % scalar.hor_no_out) * 256) / scalar.hor_no_out;

    /* ver scaling up */
    if (scalar.ver_no_out >= scalar.ver_no_in)
        scal_ver_num = (scalar.ver_no_in + 1) * 256 / scalar.ver_no_out;
    else                        /* scaling down */
        scal_ver_num = (((scalar.ver_no_in + 1) % scalar.ver_no_out) * 256) / scalar.ver_no_out;

    /* Hor_no_in */
    tmp = lcd200_dev_info->scalar.hor_no_in & 0xfff;
    iowrite32(tmp - 1, ff_dev_info->io_base + LCD_SCALAR_HIN);  // minus 1

    /* Ver_no_in */
    tmp = lcd200_dev_info->scalar.ver_no_in & 0xfff;
    iowrite32(tmp - 1, ff_dev_info->io_base + LCD_SCALAR_VIN);  // minus 1

    /* Hor_no_out */
    tmp = lcd200_dev_info->scalar.hor_no_out & 0xfff;
    iowrite32(tmp, ff_dev_info->io_base + LCD_SCALAR_HOUT);

    /* Hor_no_out */
    tmp = lcd200_dev_info->scalar.ver_no_out & 0xfff;
    iowrite32(tmp, ff_dev_info->io_base + LCD_SCALAR_VOUT);

    /* Miscellaneous Control Register */
    tmp =
        (fir_sel & 0x7) << 6 | (scalar.hor_inter_mode & 0x3) << 3 | (scalar.
                                                                     ver_inter_mode & 0x3) << 1 |
        sec_bypass_mode;
    iowrite32(tmp, ff_dev_info->io_base + LCD_SCALAR_MISC);

    /* Scalar Resolution Parameters */
    tmp = (scal_hor_num & 0xff) << 8 | (scal_ver_num & 0xff);
    iowrite32(tmp, ff_dev_info->io_base + LCD_SCALAR_PARM);

    /* enable/disable scalar */
#if 0
    // FIXME, will cause underrun
    tmp = ioread32(ff_dev_info->io_base + 0x54);
    tmp &= (~(0x1 << 2));
    if (scalar.g_enable)
        tmp |= (0x1 << 2);      // bypass sharpness
    iowrite32(tmp, ff_dev_info->io_base + 0x54);
#endif

    tmp = ioread32(ff_dev_info->io_base + FEATURE);
    tmp &= (~(0x1 << 5));
    if (scalar.g_enable)
        tmp |= (0x1 << 5);
    iowrite32(tmp, ff_dev_info->io_base + FEATURE);

    lcd200_schedule_work(ff_dev_info, C_REENABLE);

    return 0;
}

/**
 *@brief Switch LCD200 interrupt mask
 */
static void LCD200_INT_Ctrl(struct ffb_dev_info *dev_info, int on)
{
    if (on) {
        u32 value = 0xF;

        iowrite32(0xF, dev_info->io_base + INT_CLEAR);

        value = (0x1 << 2);
        iowrite32(value, dev_info->io_base + INT_MASK);
    } else
        iowrite32(0, dev_info->io_base + INT_MASK);
}

static uint32_t LCD200_RREG(struct ffb_dev_info *dev_info, int offset)
{
    uint32_t tmp;
    //down(&dev_info->dev_lock);
    tmp = ioread32(dev_info->io_base + offset);
    //up(&dev_info->dev_lock);
    return tmp;
}

static void LCD200_WREG(struct ffb_dev_info *dev_info, int offset, uint32_t value, uint32_t mask)
{
    u32 tmp;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)dev_info;

    //down(&dev_info->dev_lock);
    switch (offset) {
    case PANEL_PARAM:
        info->LCD.Parameter &= ~mask;
        info->LCD.Parameter |= (value & mask);
        tmp = info->LCD.Parameter;
        break;
    default:
        tmp = ioread32(dev_info->io_base + offset);
        tmp &= ~mask;
        tmp |= (value & mask);
        break;
    }

    iowrite32(tmp, dev_info->io_base + offset);
    //up(&dev_info->dev_lock);
}

/**
 *@brief Switch cursor enable/disable.
 *
 *@param 1:Enable,0:Disable
 */
static void LCD200_Cursor_Ctrl(struct ffb_dev_info *dev_info, int on)
{
    if (on)
        LCD200_WREG(dev_info, FEATURE, 0x1000, 0x1000);
    else
        LCD200_WREG(dev_info, FEATURE, 0, 0x1000);

#ifdef LCD210
    lcd210_set_prst(dev_info);
#endif
}

/**
 *@brief Switch LCD controller enable/disable.
 *
 *@param 1:Enable,0:Disable
 */
void LCD200_Ctrl(struct ffb_dev_info *dev_info, int on)
{
    u32 lcd200_idx;

    lcd200_idx = ((struct lcd200_dev_info *)dev_info)->lcd200_idx;
    if (on) {
        LCD200_WREG(dev_info, FEATURE, (lcd_disable ? 0 : 1), 1);
    } else {
        /* disable, MUST have UNLOCK command to be sent to bus first, then we just can disable controller  */
        platform_lock_ahbbus(lcd200_idx, 0);
        LCD200_WREG(dev_info, FEATURE, 0, 1);
    }
}

/**
 *@brief Set gammatale for LCD panel.
 */
static void lcd200_Set_Gamma_Table(struct lcd200_dev_info *info)
{
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;
    int i, j;
    u32 value;
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 256 / 4; i++) {
            value = info->GammaTable[j][i * 4 + 0] | info->GammaTable[j][i * 4 + 1] << 8 |
                info->GammaTable[j][i * 4 + 2] << 16 | info->GammaTable[j][i * 4 + 3] << 24;
            iowrite32(value, dev_info->io_base + LCD_GAMMA_RLTB + (j * 0x100) + i * 4);
        }
    }
}

/**
 *@brief Set Color Management parameters for LCD panel.
 */
static void lcd200_Set_Color_Management(struct lcd200_dev_info *info)
{
    /* Set Color Management Parameter0 */
    info->ops->reg_write((struct ffb_dev_info *)info, LCD_COLOR_MP0, info->color_mgt_p0, 0x3FFF);

    /* Set Color Management Parameter1 */
    info->ops->reg_write((struct ffb_dev_info *)info, LCD_COLOR_MP1, info->color_mgt_p1, 0x7FFF);

    /* Set Color Management Parameter2 */
    info->ops->reg_write((struct ffb_dev_info *)info, LCD_COLOR_MP2, info->color_mgt_p2, 0xFFFFFF);

    /* Set Color Management Parameter3 */
    /* can't configure 0 to Contr_slope field */
    if ((info->color_mgt_p3 >> 16) & 0x1F)
        info->ops->reg_write((struct ffb_dev_info *)info, LCD_COLOR_MP3, info->color_mgt_p3,
                             0x1F1FFF);
    else
        printk("skip! contrast slope cannot be zero! \n");
}

/**
 *@brief Enable and switch timming for LCD200 controller.
 */
static void lcd200_ctrl_enable(struct lcd200_dev_info *info)
{
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;
    u32 value;

    LCD200_Ctrl(dev_info, 0);

    if (dev_info->video_output_type >= VOT_CCIR656) {
        value = ioread32(dev_info->io_base + CCIR656_PARM) & (0x1 << 2);        /* Only ImgFormat */
        value |= info->CCIR656.Parameter;
        iowrite32(value, dev_info->io_base + CCIR656_PARM);
        iowrite32(info->CCIR656.Cycle_Num, dev_info->io_base + CCIR_RES);
        iowrite32(info->CCIR656.Field_Polarity, dev_info->io_base + CCIR_FIELD_PLY);
        iowrite32(info->CCIR656.V_Blanking01, dev_info->io_base + CCIR_VBLK01_PLY);
        iowrite32(info->CCIR656.V_Blanking23, dev_info->io_base + CCIR_VBLK23_PLY);
        iowrite32(info->CCIR656.V_Active, dev_info->io_base + CCIR_VACT_PLY);
        iowrite32(info->CCIR656.H_Blanking01, dev_info->io_base + CCIR_HBLK01_PLY);
        iowrite32(info->CCIR656.H_Blanking2, dev_info->io_base + CCIR_HBLK2_PLY);
        iowrite32(info->CCIR656.H_Active, dev_info->io_base + CCIR_HACT_PLY);
    } else {
        lcd200_Set_Gamma_Table(info);
        lcd200_Set_Color_Management(info);
        LCD200_WREG(dev_info, FEATURE, 2, 2);
        iowrite32(info->LCD.SPParameter, dev_info->io_base + SPANEL_PARAM);
    }

    iowrite32(info->LCD.Parameter, dev_info->io_base + PANEL_PARAM);
    iowrite32(info->LCD.H_Timing, dev_info->io_base + LCD_HTIMING);
    iowrite32(info->LCD.V_Timing0, dev_info->io_base + LCD_VTIMING0);
    iowrite32(info->LCD.V_Timing1, dev_info->io_base + LCD_VTIMING1);
    iowrite32(info->LCD.Configure, dev_info->io_base + LCD_POLARITY);
    iowrite32(info->LCD.HV_TimingEx, dev_info->io_base + LCD_HVTIMING_EX);

    if (info->ops->reset_buf_manage) {
        info->ops->reset_buf_manage(info);
    }

    /* update interrupt generate at */
    value = ioread32(info->dev_info.io_base + PANEL_PARAM);
    value &= ~(0x3 << 9);
    if (dev_info->output_progressive)
        value |= (0x1 << 9);    /* back porch */
    else
        value |= (0x2 << 9);    /* active image */
    iowrite32(value, dev_info->io_base + PANEL_PARAM);

    LCD200_Ctrl(dev_info, 1);

    if (dev_info->output_info->setting_ext_dev)
        dev_info->output_info->setting_ext_dev(dev_info);

    if (dev_info->video_output_type >= VOT_CCIR656) {
        /*
         * For TVE100
         */
        if (dev_info->video_output_type == VOT_PAL)
            tve100_config(info->lcd200_idx, VOT_PAL, 2);
        if (dev_info->video_output_type == VOT_NTSC)
            tve100_config(info->lcd200_idx, VOT_NTSC, 2);
    }

    LCD200_INT_Ctrl(dev_info, 1);
}

static void lcd200_ctrl_disable(struct lcd200_dev_info *info)
{
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;

    LCD200_INT_Ctrl(dev_info, 0);
    LCD200_WREG(dev_info, FEATURE, 0, 2);
    LCD200_Ctrl(dev_info, 0);
}

/**
 *@brief Set controller state
 *
 * This function must be called from task context only, since it will
 * sleep when disabling the LCD controller, or if we get two contending
 * processes trying to alter state.
 */
static void set_ctrlr_state(struct lcd200_dev_info *info, u_int state)
{
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;
    u_int old_state;

    DBGENTER(3);
    down(&dev_info->ctrlr_sem);
    old_state = dev_info->state;

        /**
	 * Hack around fbcon initialisation.
	 */
    if (old_state == C_STARTUP && state == C_REENABLE)
        state = C_ENABLE;

    switch (state) {
    case C_DISABLE_PM:
    case C_DISABLE:
                /**
		 * Disable controller
		 */
        if (old_state != C_DISABLE) {
            dev_info->state = state;
            lcd200_ctrl_disable(info);
        }
        break;

    case C_REENABLE:
                /**
		 * Re-enable the controller only if it was already
		 * enabled.  This is so we reprogram the control
		 * registers.
		 */
        if (old_state == C_ENABLE) {
            lcd200_ctrl_disable(info);
            lcd200_ctrl_enable(info);
        }
        break;

    case C_ENABLE_PM:
                /**
		 * Re-enable the controller after PM.  This is not
		 * perfect - think about the case where we were doing
		 * a clock change, and we suspended half-way through.
		 */
        if (old_state != C_DISABLE_PM)
            break;
        /* fall through */

    case C_ENABLE:
                /**
		 * Power up the LCD screen, enable controller, and
		 * turn on the backlight.
		 */
        if (old_state != C_ENABLE) {
            dev_info->state = C_ENABLE;
            lcd200_ctrl_enable(info);
        }
        break;
    }
    up(&dev_info->ctrlr_sem);
    DBGLEAVE(3);
}

/**
 *@brief Switch controller task state
 *
 * Our LCD controller task (which is called when we blank or unblank)
 * via keventd.
 */
static void lcd200_task(struct work_struct *work)
{
    struct ffb_dev_info *dev_info = container_of(work, struct ffb_dev_info, task);
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)dev_info;
    u_int state;

    DBGENTER(3);
    state = xchg(&dev_info->task_state, -1);
    set_ctrlr_state(info, state);
    DBGLEAVE(3);
}

static inline void lcd200_schedule_work(struct ffb_dev_info *info, u_int state)
{
    unsigned long flags;

    local_irq_save(flags);
        /**
	 * We need to handle two requests being made at the same time.
	 * There are two important cases:
	 *  1. When we are changing VT (C_REENABLE) while unblanking (C_ENABLE)
	 *     We must perform the unblanking, which will do our REENABLE for us.
	 *  2. When we are blanking, but immediately unblank before we have
	 *     blanked.  We do the "REENABLE" thing here as well, just to be sure.
	 */
    if (info->task_state == C_ENABLE && state == C_REENABLE)
        state = (u_int) - 1;
    if (info->task_state == C_DISABLE && state == C_ENABLE)
        state = C_REENABLE;

    if (state != (u_int) - 1) {
        info->task_state = state;
        schedule_work(&info->task);
    }
    local_irq_restore(flags);
}

#define FFB_DIVID(x,div) (((x)+(div-1))/div*div)
#define FRAME_SIZE_RGB(xres,yres,bpp)     ((FFB_DIVID(xres,16) * FFB_DIVID(yres,16) * bpp)/8)
#define FRAME_SIZE_YUV422(xres,yres)  (FFB_DIVID(xres,16) * FFB_DIVID(yres,16) * 2)
#define FRAME_SIZE_YUV420(xres,yres)  (FFB_DIVID(xres,16) * FFB_DIVID(yres,16) * 3 / 2)

/*
 * size MUST be PAGE_ALIGN
 */
u32 cal_frame_buf_size(struct ffb_info *fbi)
{
    u32 szfb_rgb = 0, szfb_yuv420 = 0, szfb_yuv422 = 0, szfb;
    int max_bpp = 0;
    struct ffb_dev_info *dinfo = fbi->dev_info;

    if (unlikely(!(fbi->support_imodes & (0x1 << fbi->video_input_mode)))) {
        printk("LCD: Input_mode = 0x%x is not configured in InputSupportList = 0x%x\n",
               fbi->video_input_mode, fbi->support_imodes);
        panic("LCD: Error configuration file");
    }

    if (fbi->support_imodes & FFB_SUPORT_CFG(VIM_ARGB))
        max_bpp = max(max_bpp, 32);
    if (fbi->support_imodes & FFB_SUPORT_CFG(VIM_RGB888))
        max_bpp = max(max_bpp, 32);
    if (fbi->support_imodes & (FFB_SUPORT_CFG(VIM_RGB565) | FFB_SUPORT_CFG(VIM_RGB555) |
                          FFB_SUPORT_CFG(VIM_RGB1555) | FFB_SUPORT_CFG(VIM_RGB444)))
        max_bpp = max(max_bpp, 16);
    if (fbi->support_imodes & (1 << VIM_RGB8))
        max_bpp = max(max_bpp, 8);

    if (fbi->support_imodes & (1 << VIM_RGB1BPP))
        max_bpp = max(max_bpp, 1);

    if (fbi->support_imodes & (1 << VIM_RGB2BPP))
        max_bpp = max(max_bpp, 2);

    if (fbi->vs.width && fbi->vs.height) {
        dinfo->max_xres = fbi->vs.width > dinfo->max_xres ? fbi->vs.width : dinfo->max_xres;
        dinfo->max_yres = fbi->vs.height > dinfo->max_yres ? fbi->vs.height : dinfo->max_yres;
    }

    szfb_rgb = FRAME_SIZE_RGB(dinfo->max_xres, dinfo->max_yres, max_bpp);

    if (fbi->support_imodes & (1 << VIM_YUV422))
        szfb_yuv422 = FRAME_SIZE_YUV422(dinfo->max_xres, dinfo->max_yres);
    if (fbi->support_imodes & (1 << VIM_YUV420))
        szfb_yuv420 = FRAME_SIZE_YUV420(dinfo->max_xres, dinfo->max_yres);

    szfb = max(szfb_yuv422, szfb_yuv420);
    szfb = max(szfb, szfb_rgb);

    return PAGE_ALIGN(szfb);
}

/**
 *@brief Initiate Gamma Table to linear curve
 */
static void dev_init_gamma_table(struct lcd200_dev_info *dinfo)
{
    int i;

    /* R */
    for (i = 0; i <= 1; i++)
        dinfo->GammaTable[0][i] = i;
    for (i = 2; i <= 3; i++)
        dinfo->GammaTable[0][i] = i + 1;
    for (i = 4; i <= 6; i++)
        dinfo->GammaTable[0][i] = i + 2;
    for (i = 7; i <= 29; i++)
        dinfo->GammaTable[0][i] = i + 3;
    for (i = 30; i <= 98; i++)
        dinfo->GammaTable[0][i] = i + 2;
    for (i = 99; i <= 152; i++)
        dinfo->GammaTable[0][i] = i + 1;
    for (i = 153; i <= 179; i++)
        dinfo->GammaTable[0][i] = i + 2;
    for (i = 180; i <= 235; i++)
        dinfo->GammaTable[0][i] = i + 3;
    for (i = 236; i <= 245; i++)
        dinfo->GammaTable[0][i] = i + 2;
    for (i = 246; i <= 251; i++)
        dinfo->GammaTable[0][i] = i + 1;
    for (i = 252; i <= 255; i++)
        dinfo->GammaTable[0][i] = i;

    /* G */
    for (i = 0; i <= 29; i++)
        dinfo->GammaTable[1][i] = i;
    for (i = 30; i <= 179; i++)
        dinfo->GammaTable[1][i] = i - 1;
    for (i = 180; i <= 235; i++)
        dinfo->GammaTable[1][i] = i;
    for (i = 236; i <= 248; i++)
        dinfo->GammaTable[1][i] = i - 1;
    for (i = 249; i <= 255; i++)
        dinfo->GammaTable[1][i] = i;

    /* B */
    for (i = 0; i <= 1; i++)
        dinfo->GammaTable[2][i] = i;
    for (i = 2; i <= 3; i++)
        dinfo->GammaTable[2][i] = i + 1;
    for (i = 4; i <= 6; i++)
        dinfo->GammaTable[2][i] = i + 2;
    for (i = 7; i <= 29; i++)
        dinfo->GammaTable[2][i] = i + 3;
    for (i = 30; i <= 179; i++)
        dinfo->GammaTable[2][i] = i + 2;
    for (i = 180; i <= 193; i++)
        dinfo->GammaTable[2][i] = i + 3;
    for (i = 194; i <= 207; i++)
        dinfo->GammaTable[2][i] = i + 4;
    for (i = 208; i <= 235; i++)
        dinfo->GammaTable[2][i] = i + 3;
    for (i = 236; i <= 245; i++)
        dinfo->GammaTable[2][i] = i + 2;
    for (i = 246; i <= 252; i++)
        dinfo->GammaTable[2][i] = i + 1;
    for (i = 253; i <= 255; i++)
        dinfo->GammaTable[2][i] = i;
}

/**
 *@brief Initiate LCD Color Management Parameters to default value. These values will take effect while enabling controlloer.
 */
static void dev_init_lcd_color_mgmt(struct lcd200_dev_info *dinfo)
{
    u32 k0 = 2, k1 = 4, shth0 = 8, shth1 = 32;  //weak sharpness

    /* LCD200 color management default value */
    dinfo->color_mgt_p0 = 0x2000;
    dinfo->color_mgt_p1 = 0x2000;
    dinfo->color_mgt_p2 = ((k0 & 0xF) << 16) | ((k1 & 0xF) << 20) | (shth0 & 0xFF) | ((shth1 & 0xFF) << 8);
    dinfo->color_mgt_p3 = 0x40000;

    return;
}

/* The purpose of this function is used to calculate the max input x,y resolution for this device
 */
static void init_ffb_dev_info(struct ffb_dev_info *info)
{
    int i, fb_pan_display = 0;
    u_short max_xres = 0;
    u_short max_yres = 0;
    ffb_input_res_t *resolution;
    struct lcd200_dev_info *devinfo = (struct lcd200_dev_info *)info;

    for (i = 0; i != (int)VIN_NONE; i++) {
        /* check if the input resolution we can support */
        if (platform_check_input_res(((struct lcd200_dev_info *)info)->lcd200_idx, (VIN_RES_T) i) < 0)
            continue;

        resolution = ffb_inres_get_info((VIN_RES_T) i);
        if (resolution == NULL)
            for (;;)
                printk("BUG in %s \n", __FUNCTION__);

        if ((max_xres * max_yres) > (resolution->xres * resolution->yres))
            continue;

        max_xres = resolution->xres;
        max_yres = resolution->yres;
    }

    info->max_xres = max_xres;
    info->max_yres = max_yres;

    /* new */
    for (i = 0; i < FB_NUM; i ++) {
        if (!devinfo->fb[i])
            continue;
        fb_pan_display |= devinfo->fb[i]->fb_pan_display;
    }
    /* pan_display will have double size */
    info->max_yres = (fb_pan_display == 1) ? info->max_yres << 1 : info->max_yres;
}

struct ffb_timing_info *get_output_timing(struct OUTPUT_INFO *output, u_char type)
{
    struct ffb_timing_info *timing = NULL;
    int i;

    for (i = 0; i < output->timing_num; i++) {
        if (output->timing_array[i].otype == type)
            break;
    }
    if (i < output->timing_num)
        timing = &output->timing_array[i];
    return timing;
}

/**
 * @fn dev_calc_pixclk
 * @calculate the frame period based on the real pll3
 */
static inline u32 dev_calc_pixclk(unsigned long pixclk, unsigned int bUse_Ext_Clk)
{
    u32 hardware_pclk;

    hardware_pclk = platform_pmu_get_clk(g_lcd200_idx, pixclk, bUse_Ext_Clk);

    return hardware_pclk;
}

/* calculte the frame period bases on the ideal pixel clock
 * return in 1000 * ms unit
 */
static inline int dev_cal_framerate(struct lcd200_dev_info *info, struct ffb_timing_info *timing,
                                    int progressive, int ccir656, unsigned long pclk)
{
    u32 h_cycle = 0, v_cycle = 0;
    int frame_rate;

    if (ccir656) {
        h_cycle = info->CCIR656.Cycle_Num & 0xFFF;
        v_cycle = (info->CCIR656.Cycle_Num >> 12) & 0xFFF;

        frame_rate = (pclk * FRAME_RATE_GRANULAR) / (h_cycle * v_cycle);        //times 10, ex:59.8hz => 598hz/10

#if 1 /* workaround for simulation */
        if ((timing->data.ccir656.out.xres == 1920) && (timing->data.ccir656.out.yres == 1080) &&
            (timing->data.ccir656.pixclock == 27000))
            frame_rate = 600;
#endif
    } else {
        h_cycle =
            timing->data.lcd.out.xres + timing->data.lcd.hsync_len + timing->data.lcd.left_margin +
            timing->data.lcd.right_margin;
        v_cycle =
            timing->data.lcd.out.yres + timing->data.lcd.vsync_len + timing->data.lcd.upper_margin +
            timing->data.lcd.lower_margin;

        frame_rate = (pclk * FRAME_RATE_GRANULAR) / (h_cycle * v_cycle);        //times 10, ex:59.8hz => 598hz/10
    }

    return frame_rate;
}

/**
 *@brief Translate device timing from ffb_timing_info.
 */
static int translate_timing(struct lcd200_dev_info *info, struct ffb_timing_info *timing)
{
    int ret = 0;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;
    u32 i, div, PiPEn, width, temp, bMultiple64, value = 0;
    u32 hardware_pclk;          //hz
    int LCDen = 0;

    DBGENTER(1);

    if (LCDen || value || bMultiple64 || temp || width || PiPEn || i) {}

    if (dev_info->video_output_type >= VOT_CCIR656) {
        u32 tmp_w, tmp_h, tmp, config, h_cycle = 0, v_cycle = 0, progressive;

        hardware_pclk =
            dev_calc_pixclk(timing->data.ccir656.pixclock, dev_info->output_info->config2 & 0x01);
        div = 0;
        config = dev_info->output_info->config;

        switch (dev_info->video_output_type) {
        case VOT_PAL:
            tmp_w = 720;
            tmp_h = 576;
            config |= CCIR656_OUTFMT_INTERLACE;
            break;
        case VOT_480P:
            tmp_w = 720;
            tmp_h = 480;
            config |= CCIR656_OUTFMT_PROGRESS;
            break;
        case VOT_576P:
            tmp_w = 720;
            tmp_h = 576;
            config |= CCIR656_OUTFMT_PROGRESS;
            break;
        case VOT_720P:
            tmp_w = 1280;
            tmp_h = 720;
            config |= CCIR656_OUTFMT_PROGRESS;
            break;
        case VOT_1920x1080P:
            tmp_w = 1920;
            tmp_h = 1080;
            config |= CCIR656_OUTFMT_PROGRESS;
            break;
        case VOT_1920x1080I:
            tmp_w = 1920;
            tmp_h = 1080;
            config |= CCIR656_OUTFMT_INTERLACE;
            break;
        case VOT_1440x1080I:
            tmp_w = 1440;
            tmp_h = 1080;
            config |= CCIR656_OUTFMT_INTERLACE;
            break;
        case VOT_1024x768_30I:
        case VOT_1024x768_25I:
            tmp_w = 1024;
            tmp_h = 768;
            config |= CCIR656_OUTFMT_INTERLACE;
            break;
        case VOT_1280x1024_30I:
        case VOT_1280x1024_25I:
            tmp_w = 1280;
            tmp_h = 1024;
            config |= CCIR656_OUTFMT_INTERLACE;
            break;
        case VOT_1024x768P:
            tmp_w = 1024;
            tmp_h = 768;
            config |= CCIR656_OUTFMT_PROGRESS;
            break;
        case VOT_1280x1024P:
            tmp_w = 1280;
            tmp_h = 1024;
            config |= CCIR656_OUTFMT_PROGRESS;
            break;
        case VOT_1680x1050P:
            tmp_w = 1680;
            tmp_h = 1050;
            config |= CCIR656_OUTFMT_PROGRESS;
            break;
        case VOT_1440x900P:
            tmp_w = 1440;
            tmp_h = 900;
            config |= CCIR656_OUTFMT_PROGRESS;
            break;
        case VOT_1440x960_30I:
            tmp_w = 1440;
            tmp_h = 960;
            config |= CCIR656_OUTFMT_INTERLACE;
            break;
        case VOT_1440x1152_25I:
            tmp_w = 1440;
            tmp_h = 1152;
            config |= CCIR656_OUTFMT_INTERLACE;
            break;
        case VOT_NTSC:
            tmp_w = 720;
            tmp_h = 480;
            config |= CCIR656_OUTFMT_INTERLACE;
            break;
        default:
            panic("%s, type = %d \n", __func__, dev_info->video_output_type);
        }

        info->CCIR656.Field_Polarity = CCIR656FIELDPLA(timing->data.ccir656.field0_switch,
                                                       timing->data.ccir656.field1_switch);
        info->CCIR656.V_Blanking01 =
            CCIR656VBLANK01(timing->data.ccir656.vblank0_len,
                            timing->data.ccir656.vblank1_len);

        tmp = tmp_h - timing->data.ccir656.out.yres;

        tmp = tmp >> 1;         /* V_BLK3 */
        if (config & CCIR656_OUTFMT_PROGRESS)   /* Progressive */
            info->CCIR656.V_Blanking23 = CCIR656VBLANK23(timing->data.ccir656.vblank2_len, tmp);
        else                    /* Interlace */
            info->CCIR656.V_Blanking23 =
                CCIR656VBLANK23(timing->data.ccir656.vblank2_len, tmp >> 1);

        tmp = tmp_h - (tmp << 1);       /* real active image */

        if ((config & CCIR656_OUTFMT_MASK) == CCIR656_OUTFMT_INTERLACE) {
            tmp = tmp >> 1;     /* if interlace, we divide the image into two fields */
            info->CCIR656.V_Active = CCIR656VACTIVE(tmp, tmp);
        }

        if ((config & CCIR656_OUTFMT_MASK) == CCIR656_OUTFMT_PROGRESS) {
            info->CCIR656.V_Active = CCIR656VACTIVE(tmp, 0);
        }

        tmp = tmp_w - timing->data.ccir656.out.xres;

        h_cycle = tmp_w + timing->data.ccir656.hblank_len;

        v_cycle = tmp_h + timing->data.ccir656.vblank0_len + timing->data.ccir656.vblank1_len +
            timing->data.ccir656.vblank2_len;
        if ((config & CCIR656_SYS_MASK) == CCIR656_SYS_HDTV) {
            info->CCIR656.H_Blanking01 =
                CCIR656HBLANK01(timing->data.ccir656.hblank_len, (tmp >> 1));
            info->CCIR656.H_Blanking2 = CCIR656HBLANK2(((tmp + 1) >> 1));
            info->CCIR656.H_Active = CCIR656HACTIVE(timing->data.ccir656.out.xres);
        } else {
            info->CCIR656.H_Blanking01 = CCIR656HBLANK01(timing->data.ccir656.hblank_len << 1, (tmp >> 1) << 1);        /* 2 cycles */
            info->CCIR656.H_Blanking2 = CCIR656HBLANK2(((tmp + 1) >> 1) << 1);
            info->CCIR656.H_Active = CCIR656HACTIVE(timing->data.ccir656.out.xres << 1);
            h_cycle <<= 1;      /* double */
        }

        info->CCIR656.Parameter = config;
        info->CCIR656.Cycle_Num = CCIR656CYC(h_cycle + 8, v_cycle);     /* Note:SAV/EAV */
        info->LCD.Configure &= ~(0x7F << 8);
        info->LCD.Configure |= (div & 0x7F) << 8;
        info->LCD.H_Timing = LCDHTIMING(0, 0, 0, timing->data.ccir656.out.xres);
        info->LCD.V_Timing0 = LCDVTIMING0(0, 0, timing->data.ccir656.out.yres);

        /* calculate the frame period based on clock */
        progressive = (config & CCIR656_OUTFMT_PROGRESS) ? 1 : 0;
        dev_info->frame_rate = dev_cal_framerate(info, timing, progressive, 1, timing->data.ccir656.pixclock * 1000);   //from khz to hz
        dev_info->hw_frame_rate = dev_cal_framerate(info, timing, progressive, 1, hardware_pclk);
        dev_info->output_progressive = progressive;
    } else {
        hardware_pclk =
            dev_calc_pixclk(timing->data.lcd.pixclock, dev_info->output_info->config2 & 0x01);
        div = 0;
        info->LCD.H_Timing = LCDHTIMING(timing->data.lcd.left_margin,
                                        timing->data.lcd.right_margin,
                                        timing->data.lcd.hsync_len, timing->data.lcd.out.xres);
        info->LCD.V_Timing0 = LCDVTIMING0(timing->data.lcd.lower_margin,
                                          timing->data.lcd.vsync_len, timing->data.lcd.out.yres);
        info->LCD.V_Timing1 = LCDVTIMING1(timing->data.lcd.upper_margin);
        info->LCD.Configure = (dev_info->output_info->config & 0xffff) | (div & 0x7F) << 8;
        info->LCD.Parameter &= ~(0x0870);
        info->LCD.Parameter |= ((dev_info->output_info->config >> 16) & 0xffff);
        info->LCD.SPParameter = timing->data.lcd.sp_param;
        info->LCD.HV_TimingEx = LCDHVTIMING_EX(timing->data.lcd.left_margin,    //HBP
                                        timing->data.lcd.right_margin,          //HFP
                                        timing->data.lcd.hsync_len,             //HW
                                        timing->data.lcd.lower_margin,          //VFP
                                        timing->data.lcd.vsync_len,             //VW
                                        timing->data.lcd.upper_margin);         //VBP

        /* calculate the frame period base on clock */
        dev_info->frame_rate = dev_cal_framerate(info, timing, 1, 0, timing->data.ccir656.pixclock * 1000);     //from khz to hz
        dev_info->hw_frame_rate = dev_cal_framerate(info, timing, 1, 0, hardware_pclk);
        dev_info->output_progressive = 1;
    }

#if defined(CONFIG_PLATFORM_GM8181)  || defined(CONFIG_PLATFORM_GM8126)
    /* FIXME. When the active period * sizeof pixel is not multiple of 64, we must makeout bit8 of ox50.
     * Otherwise, LCD210 will influence the DDR access and cause underrun.
     */
    if (dev_info->video_output_type >= VOT_CCIR656)
        width = timing->data.ccir656.in.xres;
    else
        width = timing->data.lcd.in.xres;

    /* read PiPEn */
    PiPEn = (ioread32(info->dev_info.io_base + FEATURE) >> 10) & 0x3;

    /* disable LCD controller */
    temp = ioread32(info->dev_info.io_base + FEATURE);
    if (temp & 0x1) {
        LCD200_Ctrl(&info->dev_info, 0);
        LCDen = 1;
        //iowrite32((temp & (~0x1)), info->dev_info.io_base + FEATURE);
    }

    value = ioread32(info->dev_info.io_base + 0x50);
    //value |= (0x3 << 9);    //bit9:lock ahb bus. Move to gm818x.c
    value |= (0x1 << 10);       //bit9:lock ahb bus, bit10: enable bandwidth control
    value &= ~(0x1 << 11);      //bit11 for accessing data 32bit
    value &= ~(0x1 << 12);
    value &= ~(0x1 << 13);      //bit 13 always 0

    //Bug: If 0x50:Bit8 be turned on, system would crash when DDR is very busy. //Francis 2009-12-17
    bMultiple64 = 0;            //bMultiple64 = 1;
    if (PiPEn == 1) {           /* Single PiP window */
        for (i = 0; i < 2; i++) {
            if (!info->fb[i])
                continue;

            switch (info->fb[i]->video_input_mode) {
            case VIM_ARGB:
            case VIM_RGB888:
                value |= (0x1 << 11);   //bit11 for accessing data 32bit
                if ((4 * width) % 64)
                    bMultiple64 = 0;
                break;
            default:
                if ((2 * width) % 64)
                    bMultiple64 = 0;
                break;
            }
        }
        if (bMultiple64)
            value |= (0x1 << 8);
        else
            value &= ~(0x1 << 8);
    } else if (PiPEn == 2) {    /* Double PiP window */
        for (i = 0; i < 3; i++) {
            if (!info->fb[i])
                continue;

            switch (info->fb[i]->video_input_mode) {
            case VIM_ARGB:
            case VIM_RGB888:
                if ((4 * width) % 64)
                    bMultiple64 = 0;
                for (;;)
                    printk("Error! PIP plane only allows 16 bits/pixel! \n");
                break;
            default:
                if ((2 * width) % 64)
                    bMultiple64 = 0;
                value |= (0x1 << 11);   //bit11 for accessing data 32bit
                value |= (0x1 << 12);   //bit12
                break;
            }
        }
        if (bMultiple64)
            value |= (0x1 << 8);
        else
            value &= ~(0x1 << 8);
    }

    iowrite32(value, info->dev_info.io_base + 0x50);

    //restore FEATURE register
    LCD200_Ctrl(&info->dev_info, LCDen);
    //iowrite32(temp, info->dev_info.io_base + FEATURE);
#endif /* defined(CONFIG_PLATFORM_GM8181)  || defined(CONFIG_PLATFORM_GM8126) */

    platform_pmu_switch_pinmux(info, 1);

    /* for Scalar
     */
    if (dev_info->video_output_type >= VOT_CCIR656) {
        memset(&info->scalar, 0, sizeof(info->scalar));
        info->scalar.hor_no_in = timing->data.ccir656.in.xres;
        info->scalar.hor_no_out = timing->data.ccir656.out.xres;
        info->scalar.ver_no_in = timing->data.ccir656.in.yres;
        info->scalar.ver_no_out = timing->data.ccir656.out.yres;

        if ((timing->data.ccir656.in.xres != timing->data.ccir656.out.xres) ||
            (timing->data.ccir656.in.yres != timing->data.ccir656.out.yres)) {
            info->scalar.g_enable = 1;
        } else {
            info->scalar.g_enable = 0;
        }

        dev_lcd_scalar(info);

    } else {
        memset(&info->scalar, 0, sizeof(info->scalar));
        info->scalar.hor_no_in = timing->data.lcd.in.xres;
        info->scalar.hor_no_out = timing->data.lcd.out.xres;
        info->scalar.ver_no_in = timing->data.lcd.in.yres;
        info->scalar.ver_no_out = timing->data.lcd.out.yres;

        if ((timing->data.lcd.in.xres != timing->data.lcd.out.xres) ||
            (timing->data.lcd.in.yres != timing->data.lcd.out.yres)) {
            info->scalar.g_enable = 1;
        } else {
            info->scalar.g_enable = 0;
        }

        dev_lcd_scalar(info);
    }

    DBGLEAVE(1);
    return ret;
}

/**
 *@brief Translate the timing information from ffb_timing_info to fb.var.
 */
static void setting_fb_timing(struct ffb_info *fbi, struct ffb_timing_info *timing)
{
    DBGENTER(1);
    if (timing->otype >= VOT_CCIR656) {
        fbi->fb.var.xres = timing->data.ccir656.in.xres;
        fbi->fb.var.xres_virtual = timing->data.ccir656.in.xres;
        fbi->fb.var.yres = timing->data.ccir656.in.yres;
        fbi->fb.var.yres_virtual = fbi->fb_pan_display ? (timing->data.ccir656.in.yres << 1) : timing->data.ccir656.in.yres;

        fbi->fb.var.upper_margin = timing->data.ccir656.vblank0_len;
        fbi->fb.var.lower_margin = timing->data.ccir656.vblank1_len;
        fbi->fb.var.vsync_len = timing->data.ccir656.vblank2_len;
        fbi->fb.var.left_margin = timing->data.ccir656.field0_switch;
        fbi->fb.var.right_margin = timing->data.ccir656.field1_switch;
        fbi->fb.var.hsync_len = timing->data.ccir656.hblank_len;
        fbi->fb.var.pixclock = timing->data.ccir656.pixclock;
    } else {
        fbi->fb.var.xres = timing->data.lcd.in.xres;
        fbi->fb.var.xres_virtual = timing->data.lcd.in.xres;
        fbi->fb.var.yres = timing->data.lcd.in.yres;
        fbi->fb.var.yres_virtual = fbi->fb_pan_display ? (timing->data.lcd.in.yres << 1) : timing->data.lcd.in.yres;

        fbi->fb.var.upper_margin = timing->data.lcd.upper_margin;
        fbi->fb.var.lower_margin = timing->data.lcd.lower_margin;
        fbi->fb.var.vsync_len = timing->data.lcd.vsync_len;
        fbi->fb.var.left_margin = timing->data.lcd.left_margin;
        fbi->fb.var.right_margin = timing->data.lcd.right_margin;
        fbi->fb.var.hsync_len = timing->data.lcd.hsync_len;
        fbi->fb.var.pixclock = timing->data.lcd.pixclock;
    }

    DBGLEAVE(1);
}

/**
 *@brief Switch device output timing.
 */
static int dev_switch_output(struct lcd200_dev_info *info)
{
    int ret = 0;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;
    struct ffb_timing_info *timing;

    DBGENTER(1);

    timing = get_output_timing(dev_info->output_info, dev_info->video_output_type);
    if (timing == NULL) {
        ret = -EINVAL;
        err("The %s timing is not exist",
            (char *)ffb_get_param(FFB_PARAM_OUTPUT, NULL, dev_info->video_output_type));
        goto end;
    }

    ret = translate_timing(info, timing);
    if (ret < 0) {
        err("Translate %s timing failed",
            (char *)ffb_get_param(FFB_PARAM_OUTPUT, NULL, dev_info->video_output_type));
        goto end;

    }

  end:
    DBGLEAVE(1);
    return ret;
}

/**
 * @brief Setting fb timing information base on output type.
 * When either output_type or input_res is changed, this function must be executed again.
 */
int dev_setting_ffb_output(struct lcd200_dev_info *info)
{
    int ret = 0;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;
    struct ffb_timing_info *timing;
    DBGENTER(1);

    timing = get_output_timing(dev_info->output_info, dev_info->video_output_type);
    if (timing == NULL) {
        ret = -EINVAL;
        err("The %s timing is not exist",
            (char *)ffb_get_param(FFB_PARAM_OUTPUT, NULL, dev_info->video_output_type));
        goto end;
    }

    if (info->fb[0]) {
        setting_fb_timing(info->fb[0], timing);
        ret = info->fb[0]->fb.fbops->fb_check_var(&info->fb[0]->fb.var, &info->fb[0]->fb);
        if (ret < 0) {
            err("fb0 check_var failed");
            goto f0;
        }
        ret = info->fb[0]->fb.fbops->fb_set_par(&info->fb[0]->fb);
        if (ret < 0)
            err("fb0 set_par failed");
    }
  f0:
    if (info->fb[1]) {
        setting_fb_timing(info->fb[1], timing);
        ret = info->fb[1]->fb.fbops->fb_check_var(&info->fb[1]->fb.var, &info->fb[1]->fb);
        if (ret < 0) {
            err("fb1 check_var failed");
            goto f1;
        }
        ret = info->fb[1]->fb.fbops->fb_set_par(&info->fb[1]->fb);
        if (ret < 0)
            err("fb1 set_par failed");
    }
  f1:
    if (info->fb[2]) {
        setting_fb_timing(info->fb[2], timing);
        ret = info->fb[2]->fb.fbops->fb_check_var(&info->fb[2]->fb.var, &info->fb[2]->fb);
        if (ret < 0) {
            err("fb2 check_var failed");
            goto f2;
        }
        ret = info->fb[2]->fb.fbops->fb_set_par(&info->fb[2]->fb);
        if (ret < 0)
            err("fb2 set_par failed");
    }
  f2:
    if (info->fb[3]) {
        setting_fb_timing(info->fb[3], timing);
        ret = info->fb[3]->fb.fbops->fb_check_var(&info->fb[3]->fb.var, &info->fb[3]->fb);
        if (ret < 0) {
            err("fb3 check_var failed");
            goto f3;
        }
        ret = info->fb[3]->fb.fbops->fb_set_par(&info->fb[3]->fb);
        if (ret < 0)
            err("fb3 set_par failed");
    }
  f3:
  end:
    DBGLEAVE(1);
    return ret;
}

static int dev_switch_input_mode(struct ffb_info *fbi)
{
    switch (fbi->video_input_mode) {
    case VIM_ARGB:
        fbi->fb.var.bits_per_pixel = 32;
        break;
    case VIM_RGB888:
        fbi->fb.var.bits_per_pixel = 32;
        break;
    case VIM_RGB565:
    case VIM_RGB1555:
    case VIM_RGB555:
    case VIM_RGB444:
    case VIM_YUV422:
        fbi->fb.var.bits_per_pixel = 16;
        break;
    case VIM_RGB1BPP:
        fbi->fb.var.bits_per_pixel = 1;
        break;
    case VIM_RGB2BPP:
        fbi->fb.var.bits_per_pixel = 2;
        break;
    case VIM_RGB8:
    default:
        fbi->fb.var.bits_per_pixel = 8;
        break;
    }
    return 0;
}

static int init_ffb_info(struct ffb_info *fbi)
{
    struct ffb_dev_info *fd_info = fbi->dev_info;
    struct lcd200_dev_info *lcd200_info = (struct lcd200_dev_info *)fd_info;
    char tstr[16];

    /* assing bits_per_pixel for calculating frame buffer */
    dev_switch_input_mode(fbi);

    if (fd_info->video_output_type >= VOT_CCIR656) {
        if (lcd200_info->lcd200_idx == LCD_ID) {
            /* first lcd200 */
            if (!fbi->index)
                snprintf(tstr, 16, "ftve");
            else
                snprintf(tstr, 16, "ftve_s%d", fbi->index);
        } else if (lcd200_info->lcd200_idx == LCD1_ID) {
            /* second lcd200 */
            if (!fbi->index)
                snprintf(tstr, 16, "ftve1");
            else
                snprintf(tstr, 16, "ftve1_s%d", fbi->index);
        } else if (lcd200_info->lcd200_idx == LCD2_ID) {
            /* third lcd200 */
            if (!fbi->index)
                snprintf(tstr, 16, "ftve2");
            else
                snprintf(tstr, 16, "ftve2_s%d", fbi->index);
        } else {
            panic("%s, unknown lcd id: %d \n", __func__, lcd200_info->lcd200_idx);
        }
    } else {
        if (lcd200_info->lcd200_idx == LCD_ID) {
            /* first lcd200 */
            if (!fbi->index)
                snprintf(tstr, 16, "flcd");
            else
                snprintf(tstr, 16, "flcd_s%d", fbi->index);
        } else if (lcd200_info->lcd200_idx == LCD1_ID) {
            /* second flcd200 */
            if (!fbi->index)
                snprintf(tstr, 16, "flcd1");
            else
                snprintf(tstr, 16, "flcd1_s%d", fbi->index);
        } else if (lcd200_info->lcd200_idx == LCD2_ID) {
            /* third flcd200 */
            if (!fbi->index)
                snprintf(tstr, 16, "flcd2");
            else
                snprintf(tstr, 16, "flcd2_s%d", fbi->index);
        } else {
            panic("%s, unknown lcd id: %d \n", __func__, lcd200_info->lcd200_idx);
        }
    }
    strcpy(fbi->fb.fix.id, tstr);
    fbi->fb.fix.smem_len = cal_frame_buf_size(fbi);

    if (fd_info->fb0_fb1_share && (fbi->index == 1)) {
        struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)fd_info;

        /* memory size check */
        if (fbi->fb.fix.smem_len > dev_info->fb[0]->fb.fix.smem_len)
            panic("LCD%d, inconsistent fb size. fb0 size: %d, fb1 size: %d in fb share! \n",
                    lcd200_info->lcd200_idx, dev_info->fb[0]->fb.fix.smem_len, fbi->fb.fix.smem_len);

        if (fbi->fb_num > dev_info->fb[0]->fb_num)
            panic("LCD%d, inconsistent fb_num. fb0 fb_num: %d, fb1 fb_num: %d in fb share! \n",
                    lcd200_info->lcd200_idx, dev_info->fb[0]->fb_num, fbi->fb_num);
    }

    fbi->rgb[RGB_1] = &rgb_1;
    fbi->rgb[RGB_2] = &rgb_2;
    fbi->rgb[RGB_8] = &rgb_8;
    fbi->rgb[RGB_16] = &def_rgb_16;
    fbi->rgb[RGB_32] = &def_rgb_32;
    fbi->rgb[ARGB] = &def_argb;

    return 0;
}

static int dev_set_par(struct ffb_dev_info *info)
{
    int ret = 0;
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)info;
    DBGENTER(1);
    ret = dev_switch_output(dev_info);
    if (!ret) {
        lcd200_schedule_work(info, C_REENABLE);
    }
    DBGLEAVE(1);
    return ret;
}

static int dev_reset(struct ffb_dev_info *info)
{
    int ret = 0;

    DBGENTER(1);
    LCD200_Ctrl(info, 0);
#if 0
    ///FIXME: This is workaround for enable/disable device will randomly cause filed reverse.
    {
        u32 tmp;
        tmp = LCD200_RREG(info, PIOP_IMG_FMT2);
        LCD200_WREG(info, PIOP_IMG_FMT2, tmp ^ 0xf, 0xf);
        LCD200_WREG(info, PIOP_IMG_FMT2, tmp, 0xf);
        tmp = LCD200_RREG(info, CCIR656_PARM);
        LCD200_WREG(info, CCIR656_PARM, tmp ^ 0x4, 0x4);
        LCD200_WREG(info, CCIR656_PARM, tmp, 0x4);
    }
    ///
#endif
    LCD200_Ctrl(info, 1);
    DBGLEAVE(1);
    return ret;
}

/**
 *@brief Convert RGB to YCbCr
 *
 *@return Return the YCbCr and the format is (Y<<16|Cb<<8|Cr)
 */
static u32 Convert_RGB2YCbCr(u8 r, u8 g, u8 b)
{
    int tcolor;
    u32 YCbCr;
    ///Convert RGB to Y
    tcolor = (257 * r + 504 * g + 98 * b) / 1000 + 16;
    YCbCr = (tcolor & 0xff) << 16;
    ///Convert RGB to Cb
    tcolor = (int)(439 * b - 291 * g - 148 * r) / 1000 + 128;
    YCbCr |= (tcolor & 0xff) << 8;
    ///Convert RGB to Cr
    tcolor = (int)(439 * r - 368 * g - 71 * b) / 1000 + 128;
    YCbCr |= (tcolor & 0xff);
    return YCbCr;
}

static void dev_cursor64x64_translate(u8 *output, u8 *input, int size)
{
    int i;

    /* swap per byte */
    for (i = 0; i < size; i ++)
        output[i] = ((input[i] & 0xF) << 4) | ((input[i] & 0xF0) >> 4);
}

/**
 *@brief Hardware cursor function for LCD200
 *
 * The format about (struct fb_cursor).image.data
 * When depth is 1
 *             Byte 0   Byte 1
 *  Line  1:  11111111 1000xxxx
 *             Byte 2   Byte 3
 *  Line  2:  11111111 0000xxxx
 *             Byte 4   Byte 5
 *  Line  3:  11111110 0000xxxx
 *
 * When depth is 2
 *             Byte 0   Byte 1  Byte 3
 *  Line  1:    2222     2222    2000
 *             Byte 4   Byte 5  Byte 6
 *  Line  2:    2111     1112    0000
 *             Byte 7   Byte 8  Byte 9
 *  Line  3:    2111     1120    0000
 */
static int dev_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
    int ret = 0;
    struct ffb_info *finfo = (struct ffb_info *)info;
    struct ffb_dev_info *fd_info = finfo->dev_info;
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)fd_info;
    int i, set = cursor->set;
    unsigned long retVal;

    DBGENTER(1);

    if (cursor->image.width > MAX_CURS_WIDTH || cursor->image.height > MAX_CURS_HEIGHT) {
        ret = -ENXIO;
        goto end;
    }

    if (fd_info->cursor_reset) {
        set = FB_CUR_SETALL;
        fd_info->cursor_reset = 0;
    }

    if (set & (FB_CUR_SETSHAPE | FB_CUR_SETIMAGE | FB_CUR_SETCMAP)) {
        LCD200_Cursor_Ctrl(fd_info, 0);
        dev_info->cursor.enable = 0;
    }

    if (set & FB_CUR_SETPOS) {
        dev_info->cursor.dy = cursor->image.dy - info->var.yoffset;
        dev_info->cursor.dx = cursor->image.dx - info->var.xoffset;

        if (dev_info->scalar.g_enable) {
            dev_info->cursor.dx = (dev_info->cursor.dx * dev_info->scalar.hor_no_out) / dev_info->scalar.hor_no_in;
            dev_info->cursor.dy = (dev_info->cursor.dy * dev_info->scalar.ver_no_out) / dev_info->scalar.ver_no_in;
        }
    }

    if (set & FB_CUR_SETCMAP) {
        struct fb_cmap cmap = cursor->image.cmap;
        u32 tmp_color;
        int cmap_len;
        __u16 cmapr[MAX_CURS_COLOR];
        __u16 cmapg[MAX_CURS_COLOR];
        __u16 cmapb[MAX_CURS_COLOR];
        __u16 cmapt[MAX_CURS_COLOR];

        cmap_len =
            (cursor->image.cmap.len > MAX_CURS_COLOR) ? MAX_CURS_COLOR : cursor->image.cmap.len;
        retVal = copy_from_user(cmapr, cursor->image.cmap.red, cmap_len * sizeof(__u16));
        cmap.red = cmapr;
        retVal = copy_from_user(cmapg, cursor->image.cmap.green, cmap_len * sizeof(__u16));
        cmap.green = cmapg;
        retVal = copy_from_user(cmapb, cursor->image.cmap.blue, cmap_len * sizeof(__u16));
        cmap.blue = cmapb;
        retVal = copy_from_user(cmapt, cursor->image.cmap.transp, cmap_len * sizeof(__u16));
        cmap.transp = cmapt;

        for (i = 0; i < cmap_len; i++) {
            tmp_color = Convert_RGB2YCbCr(cmap.red[i], cmap.green[i], cmap.blue[i]);
            iowrite32(tmp_color, fd_info->io_base + LCD_CURSOR_COL1 + (i << 2));
        }
    }

    if (set & (FB_CUR_SETSHAPE | FB_CUR_SETIMAGE)) {
        u8 *mask = NULL, *rawdata = NULL, *data = NULL;
        /* how many bytes per line(width) for the input image */
        u32 s_pitch = (cursor->image.width * cursor->image.depth + 7) >> 3;
        /* how many bytes per line(width) for the hardware cursor database */
        u32 d_pitch = (cursor->image.width == 32) ? 64 >> 3 : 256 >> 3;

        if (d_pitch)    {}

        if ((cursor->image.depth != 2) && (cursor->image.depth != 4)) {
            printk("%s, the cursor only supports depth 2 or 4! \n", __func__);
            ret = -EFAULT;
            goto end;
        }

        if (cursor->image.width == 32) {
            mask = kmalloc(64 * 32 / 8, GFP_KERNEL);
            rawdata = kmalloc(64 * 32 / 8, GFP_KERNEL);
            data = kmalloc(64 * 32 / 8, GFP_KERNEL);
        } else if (cursor->image.width == 64) {
            mask = kmalloc(64 * 64 * 4 / 8, GFP_KERNEL);
            rawdata = kmalloc(64 * 64 * 4 / 8, GFP_KERNEL);
            data = kmalloc(64 * 64 * 4 / 8, GFP_KERNEL);
        } else {
            panic("%s, not support width:%d \n", __func__, cursor->image.width);
        }

        if (!mask || !rawdata || !data) {
            printk("%s, no memory!!! \n", __func__);
            ret = -EFAULT;

            if (mask) kfree(mask);
            if (rawdata) kfree(rawdata);
            if (data) kfree(data);
            goto end;
        }


        i = s_pitch * cursor->image.height;

        retVal = copy_from_user(mask, cursor->mask, i);
        cursor->mask = mask;

        retVal = copy_from_user(rawdata, cursor->image.data, i);
        cursor->image.data = rawdata;

#ifndef CURSOR_64x64
        if ((cursor->image.width == 32) && (cursor->image.height == 32) && (cursor->image.depth == 2)) {
            int loop, offset, row, index = 0, rtop = 0, rbom = 0, ltop = 0, lbom = 0;
            u32 *dat;
            u32 tmp;

            /* width = 64 bits, height=32 */
            memset_io((void *)(fd_info->io_base + LCD_CURSOR_DATA), 0x0, 64 * 32 / 8);

            /* fb_pad_aligned_buffer(u8 *dst, u32 d_pitch,u8 *src, u32 s_pitch, u32 height) */
            fb_pad_aligned_buffer(data, d_pitch, (u8 *) cursor->image.data, s_pitch,
                                      cursor->image.height);

            /* use 32x32 database */
            tmp = ioread32(fd_info->io_base + LCD_CURSOR_POS);
            tmp &= ~(0x1 << 29);
            iowrite32(tmp, fd_info->io_base + LCD_CURSOR_POS);
            dev_info->cursor_32x32 = 1;

            dat = (u32 *) data;
            for (row = 0; row < cursor->image.height; row++) {
                /* 64: depth is 2, width is 32 */
                for (loop = 0; loop < (64 / 32); loop++) {
                    tmp = swab32(dat[index]);

                    if (row <= 15) {
                        if (!(index & 0x1)) {
                            offset = LCD_CURSOR_DATA_LTOP + (ltop << 2);
                            ltop++;
                        } else {
                            offset = LCD_CURSOR_DATA_RTOP + (rtop << 2);
                            rtop++;
                        }
                    } else {
                        if (!(index & 0x1)) {
                            offset = LCD_CURSOR_DATA_LBOM + (lbom << 2);
                            lbom++;
                        } else {
                            offset = LCD_CURSOR_DATA_RBOM + (rbom << 2);
                            rbom++;
                        }
                    }

                    iowrite32(tmp, fd_info->io_base + offset);
                    index++;
                } /* loop */
            } /* row */
        }
#endif /* CURSOR_64x64 */

        if ((cursor->image.width == 64) && (cursor->image.height == 64) && (cursor->image.depth == 4)) {
            int i, size = 64 * 64 * 4 / 8;
            unsigned int value;

            memset_io((void *)(fd_info->io_base + LCD_CURSOR_DATA_64x64), 0x0, size);
            dev_cursor64x64_translate(data, (u8 *)cursor->image.data, size);

            for (i = 0; i < size; i += 4) {
                value = ((data[i+3] << 24) | (data[i+2] << 16) | (data[i+1] << 8) | data[i]);
                iowrite32(value, fd_info->io_base + 0x1600 + i);
            }
        } else {
#ifdef CURSOR_64x64
            printk("%s, only supprt 64x64 cursor! \n", __func__);
#endif
        }

        if (mask) kfree(mask);
        if (rawdata) kfree(rawdata);
        if (data) kfree(data);
    } /* set & (FB_CUR_SETSHAPE | FB_CUR_SETIMAGE */

    if (cursor->enable) {
        if (!dev_info->cursor.enable) {
            LCD200_Cursor_Ctrl(fd_info, 1);
            dev_info->cursor.enable = 1;
        }
    } else {
        LCD200_Cursor_Ctrl(fd_info, 0);
        dev_info->cursor.enable = 0;
    }
  end:
    DBGLEAVE(1);
    return ret;
}

/**
 *@fn dev_set_RB_swap
 *@brief Set RB or CbCr swap.
 */
static int dev_set_RB_swap(struct ffb_info *fbi, int on)
{
    int ret = 0;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)fbi->dev_info;
    if (on)
        LCD200_WREG(dev_info, PANEL_PARAM, 0x10, 0x10);
    else
        LCD200_WREG(dev_info, PANEL_PARAM, 0x0, 0x10);
    return ret;
}

/**
 *@fn dev_ioctl
 *@ioctl for device
 */
static int dev_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct ffb_info *fbi = (struct ffb_info *)info;
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)fbi->dev_info;

    down(&dev_info->cfg_sema);

    DBGENTER(1);
    switch (cmd) {
    case FFB_IOCURSOR:
        {
            struct fb_cursor fbc;

            DBGPRINT(3, "FFB_IOCURSOR:\n");
            if (copy_from_user(&fbc, (unsigned int *)arg, sizeof(struct fb_cursor))) {
                ret = -EFAULT;
                break;
            }

            ret = dev_cursor(info, &fbc);
            break;
        }
    case FFB_IOCURSOR_POS_SCAL:
        {
            struct ffb_dev_info *dev_info = (struct ffb_dev_info *)fbi->dev_info;
            unsigned int value;

            if (copy_from_user(&value, (unsigned int *)arg, sizeof(int))) {
                ret = -EFAULT;
                break;
            }
            dev_info->cursor_pos_scale = (value >= 1) ? 1 : 0;
            break;
        }
    case FFB_IOSRBSWAP:
        {
            int tmp;
            if (copy_from_user(&tmp, (int *)arg, sizeof(int))) {
                ret = -EFAULT;
                break;
            }
            ret = dev_set_RB_swap(fbi, tmp);
            break;
        }

    case FFB_BYPASS:
        {
            ret = -1;
            break;
        }
    case FFB_BYPASS_SRC:
        {
            ret = -1;
            break;
        }
    case FFB_BYPASS_INVERT_INPUT_CLK:
        {
            ret = -1;
            break;
        }
    case FFB_LCD_SCALAR:
        {
            struct ffb_dev_info *ff_dev_info = fbi->dev_info;
            struct lcd200_dev_info *lcd200_dev_info = (struct lcd200_dev_info *)ff_dev_info;

            DBGPRINT(3, "FFB_LCD_SCALAR:\n");
            if (copy_from_user
                (&lcd200_dev_info->scalar, (unsigned int *)arg, sizeof(struct scalar_info))) {
                ret = -EFAULT;
                break;
            }

            ret = dev_lcd_scalar(lcd200_dev_info);
            break;
        }
    case COLOR_KEY1:
        {
            unsigned int tmp;
            struct lcd200_dev_info *lcd200_dev_info = (struct lcd200_dev_info *)fbi->dev_info;

            if (copy_from_user(&tmp, (unsigned int *)arg, sizeof(unsigned int))) {
                ret = -EFAULT;
                break;
            }
            if (tmp > 0xffffff) {
                ret = -EINVAL;
                break;
            }
            lcd200_dev_info->color_key1 = tmp & 0xffffff;
            break;
        }
    case COLOR_KEY1_EN:
        {
            unsigned int tmp;
            struct lcd200_dev_info *lcd200_dev_info = (struct lcd200_dev_info *)fbi->dev_info;

            if (copy_from_user(&tmp, (unsigned int *)arg, sizeof(unsigned int))) {
                ret = -EFAULT;
                break;
            }
            if (tmp == 1)
                iowrite32(lcd200_dev_info->color_key1 | (0x1 << 24),
                          lcd200_dev_info->dev_info.io_base + PIP_COLOR_KEY);
            else
                iowrite32(lcd200_dev_info->color_key1,
                          lcd200_dev_info->dev_info.io_base + PIP_COLOR_KEY);

            break;
        }

#ifdef LCD210
    case COLOR_KEY2:
        {
            unsigned int tmp;
            struct lcd200_dev_info *lcd200_dev_info = (struct lcd200_dev_info *)fbi->dev_info;

            if (copy_from_user(&tmp, (unsigned int *)arg, sizeof(unsigned int))) {
                ret = -EFAULT;
                break;
            }
            if (tmp > 0xffffff) {
                ret = -EINVAL;
                break;
            }

            lcd200_dev_info->color_key2 = tmp & 0xffffff;
            break;
        }
    case COLOR_KEY2_EN:
        {
            unsigned int tmp;
            struct lcd200_dev_info *lcd200_dev_info = (struct lcd200_dev_info *)fbi->dev_info;

            if (copy_from_user(&tmp, (unsigned int *)arg, sizeof(unsigned int))) {
                ret = -EFAULT;
                break;
            }

            if (tmp == 1)
                iowrite32(lcd200_dev_info->color_key2 | (0x1 << 24),
                          lcd200_dev_info->dev_info.io_base + PIP_COLOR_KEY2);
            else
                iowrite32(lcd200_dev_info->color_key2,
                          lcd200_dev_info->dev_info.io_base + PIP_COLOR_KEY2);

            break;
        }
#endif /* LCD210 */
    default:
        /* calls ffb ioctl: ffb_ioctl */
        ret = fbi->ops->ioctl(info, cmd, arg);
        break;
    }

    up(&dev_info->cfg_sema);

    DBGLEAVE(1);
    return ret;
}

static int dev_reset_buf_manage(struct lcd200_dev_info *info)
{
    int ret = 0;
    int idx, *displayfifo;
    struct ffb_dev_info *fdev_info = (struct ffb_dev_info *)info;
#ifdef VIDEOGRAPH_INC
    static int  first_reset = 1;
#endif

    DBGENTER(1);

    for (idx = 0; idx < PIP_NUM; idx++) {
        if (unlikely(info->fb[idx] == NULL))
            continue;

        displayfifo = info->fb[idx]->fci.displayfifo;
        memset(&info->fb[idx]->fci, 0, sizeof(frame_ctrl_info_t));
        info->fb[idx]->fci.displayfifo = displayfifo;
        if (info->fb[idx]->fci.displayfifo != NULL) {
            memset(info->fb[idx]->fci.displayfifo, 0, info->fb[idx]->fb_num * sizeof(int));
            info->fb[idx]->fci.use_queue = 0;
            info->fb[idx]->fci.get_idx = info->fb[idx]->fci.put_idx = 0;
            info->fb[idx]->fci.set_idx = info->fb[idx]->fb_num - 1;     /* park in the last fb_num */
        }

#ifdef VIDEOGRAPH_INC
        /* For the video plane only. VideoGraph expects that the video plane keeps the last
         * frammebuffer which is provided by Videograph instead of LCD owned framebuffer.
         * 2013/5/13 10:52 AM.
         */
        if (idx == 0) {
            if (first_reset == 1) {
                first_reset = 0;
            } else {
                /* DO NOT update the framebuffer if videograph is running. */
                continue;
            }
        }
#endif

        /* update to hardware */
        iowrite32(info->fb[idx]->ppfb_dma[0], fdev_info->io_base + FB0_BASE + idx * 0xC);
    }

    DBGLEAVE(1);
    return ret;
}

/**
 *@brief suspend and resume support for the lcd controller
 */
static int dev_suspend(struct lcd200_dev_info *info, pm_message_t state, u32 level)
{
    if (level) {
    }

    set_ctrlr_state(info, C_DISABLE_PM);
    return 0;
}

static int dev_resume(struct lcd200_dev_info *info, u32 level)
{
    if (level) {
    }

    set_ctrlr_state(info, C_ENABLE_PM);
    return 0;
}

static struct lcd200_dev_ops lcd200_dev_ops = {
    .set_par = dev_set_par,
    .ioctl = dev_ioctl,
    .reset_buf_manage = dev_reset_buf_manage,
    .reg_write = LCD200_WREG,
    .reg_read = LCD200_RREG,
    .switch_input_mode = dev_switch_input_mode,
    .suspend = dev_suspend,
    .resume = dev_resume,
    .reset_dev = dev_reset,
};

static ssize_t lcd200_show_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%s:%s\n", FFB_MODULE_NAME, FFB_VERSION);
}

static DEVICE_ATTR(version, S_IRUGO, lcd200_show_version, NULL);

static ssize_t lcd200_show_output_type(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct ffb_dev_info *dev_info;
    int r = -ENODEV;

    dev_info = (struct ffb_dev_info *)dev_get_drvdata(dev);
    if (dev_info) {
        ssize_t bsize = PAGE_SIZE - 1;
        int i;
        r = 0;

        r += snprintf(buf + r, bsize, "Support type:\n");
        bsize = PAGE_SIZE - 1 - r;
        r += snprintf(buf + r, bsize, "--------------------\n");
        bsize = PAGE_SIZE - 1 - r;
        for (i = 0; i < 32; i++) {
            if (dev_info->support_otypes & (1 << i)) {
                r += snprintf(buf + r, bsize, "%d: %s\n", i,
                              (char *)ffb_get_param(FFB_PARAM_OUTPUT, NULL, i));
                bsize = PAGE_SIZE - 1 - r;
                if (!bsize)
                    goto end;
            }
        }
        r += snprintf(buf + r, bsize, "--------------------\n");
        bsize = PAGE_SIZE - 1 - r;
        if (!bsize)
            goto end;
        r += snprintf(buf + r, bsize, "Current type:%s\n",
                      (char *)ffb_get_param(FFB_PARAM_OUTPUT, NULL, dev_info->video_output_type));
    }
  end:
    return r;
}

static ssize_t lcd200_store_output_type(struct device *dev,
                                        struct device_attribute *attr, const char *buf, size_t size)
{
    struct ffb_dev_info *dev_info;
    int r = -ENODEV;

    dev_info = (struct ffb_dev_info *)dev_get_drvdata(dev);
    if (dev_info) {
        u_int ttype = 0;
        r = -EINVAL;
        if (sscanf(buf, "%d", &ttype) == 1) {
            if (dev_info->support_otypes & (1 << ttype)) {
                dev_info->video_output_type = ttype;
                r = dev_setting_ffb_output((struct lcd200_dev_info *)dev_info);
                if (r < 0)
                    goto end;
                if (((struct lcd200_dev_info *)dev_info)->ops->config_input_misc) {
                    r = ((struct lcd200_dev_info *)dev_info)->ops->
                        config_input_misc((struct lcd200_dev_info *)dev_info);
                    if (r < 0)
                        goto end;
                }
                r = dev_set_par(dev_info);
                if (r < 0)
                    goto end;
            } else {
                err("Device does not support %s type!",
                    (char *)ffb_get_param(FFB_PARAM_OUTPUT, NULL, ttype));
            }
        }
    }
  end:
    return r ? r : size;
}

static DEVICE_ATTR(output_type, 0664, lcd200_show_output_type, lcd200_store_output_type);

static struct attribute *lcd200_attrs[] = {
    &dev_attr_output_type.attr,
    NULL,
};

static struct attribute_group lcd200_attrs_grp = {
    .name = "lcd200_attrs",
    .attrs = lcd200_attrs,
};

static int lcd200_register_sysfs(struct device *dev)
{
    int ret;

    if ((ret = device_create_file(dev, &dev_attr_version)))
        goto fail0;

    if ((ret = sysfs_create_group(&dev->kobj, &lcd200_attrs_grp)))
        goto fail1;

    return 0;

    sysfs_remove_group(&dev->kobj, &lcd200_attrs_grp);
  fail1:
    device_remove_file(dev, &dev_attr_version);
  fail0:
    err("unable to register sysfs interface\n");
    return ret;
}

static void lcd200_unregister_sysfs(struct device *dev)
{
    sysfs_remove_group(&dev->kobj, &lcd200_attrs_grp);
    device_remove_file(dev, &dev_attr_version);
}

struct lcd200_dev_info *p_dev_info = NULL;

#include "dev_proc.c"

void dev_deconstruct(struct lcd200_dev_info *info)
{
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;

    DBGENTER(1);
    dev_proc_remove();
    //cancel_delayed_work(&dev_info->task);
    work_clear_pending(&dev_info->task);
    flush_scheduled_work();
    if (info->ops) {
        dev_info->task_state = C_DISABLE;
        lcd200_task(&dev_info->task);
        lcd200_unregister_sysfs(dev_info->dev);
    }
    if (info->fb[0]) {
        unregister_framebuffer(&info->fb[0]->fb);
        ffb_deconstruct(info->fb[0]);
    }
    if (info->fb[1]) {
        unregister_framebuffer(&info->fb[1]->fb);
        ffb_deconstruct(info->fb[1]);
    }
    if (info->fb[2]) {
        unregister_framebuffer(&info->fb[2]->fb);
        ffb_deconstruct(info->fb[2]);
    }
    if (info->fb[3]) {
        unregister_framebuffer(&info->fb[3]->fb);
        ffb_deconstruct(info->fb[3]);
    }
    if (info == p_dev_info) {
        p_dev_info = NULL;
        platform_pmu_switch_pinmux(info, 0);
        lcd200_dev_ops.config_input_misc = NULL;
    }

    DBGLEAVE(1);
}

//EXPORT_SYMBOL(dev_deconstruct);

int dev_construct(struct lcd200_dev_info *info, union lcd200_config config)
{
    int ret = 0;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;

    DBGENTER(1);
    if (p_dev_info != NULL) {
        err("Device is busy.");
        ret = -EBUSY;
        goto err;
    }

    /* keep the lcd200 idx */
    g_lcd200_idx = info->lcd200_idx;
    info->ops = &lcd200_dev_ops;

    /* Disable LCDen */
    config.value &= ~0x01;
    iowrite32(config.value, dev_info->io_base + FEATURE);

    /* find out the max input x/y resolution for this device */
    init_ffb_dev_info(dev_info);

    /* The following procedure only can be executed once
     */
    if (info->fb[0]) {
        info->fb[0]->dev_info = dev_info;
        /* assign RGB info to ffb_info */
        init_ffb_info(info->fb[0]);
        /* init the display queue and mmap */
        ret = ffb_construct(info->fb[0]);
        if (ret < 0)
            goto err;
    }
    if (info->fb[1]) {
        info->fb[1]->dev_info = dev_info;
        /* assign RGB info to ffb_info */
        init_ffb_info(info->fb[1]);
        /* init the display queue and mmap */
        ret = ffb_construct(info->fb[1]);
        if (ret < 0)
            goto err;
    }
    if (info->fb[2]) {
        info->fb[2]->dev_info = dev_info;
        /* assign RGB info to ffb_info */
        init_ffb_info(info->fb[2]);
        /* init the display queue and mmap */
        ret = ffb_construct(info->fb[2]);
        if (ret < 0)
            goto err;
    }
    if (info->fb[3]) {
        info->fb[3]->dev_info = dev_info;
        /* assign RGB info to ffb_info */
        init_ffb_info(info->fb[3]);
        /* init the display queue and mmap */
        ret = ffb_construct(info->fb[3]);
        if (ret < 0)
            goto err;
    }

    /* configure output timing according to the output_type and input_res
     * When either output_type or input_res is changed, this function must be executed again.
     */
    dev_setting_ffb_output(info);

    /* init gamma table
     */
    dev_init_gamma_table(info);

    /* init LCD color management
     */
    dev_init_lcd_color_mgmt(info);

    dev_info->state = C_STARTUP;
    dev_info->task_state = (u_char) - 1;
    INIT_WORK(&dev_info->task, lcd200_task);
    sema_init(&dev_info->ctrlr_sem, 1);
    sema_init(&dev_info->dev_lock, 1);
    sema_init(&info->cfg_sema, 1);

    /* device node will be created */
    if (info->fb[0]) {
        ret = register_framebuffer(&info->fb[0]->fb);
        if (ret < 0) {
            err("register_framebuffer failed\n");
            goto err;
        }
    }
    if (info->fb[1]) {
        ret = register_framebuffer(&info->fb[1]->fb);
        if (ret < 0) {
            err("register_framebuffer failed\n");
            goto err;
        }
    }
    if (info->fb[2]) {
        ret = register_framebuffer(&info->fb[2]->fb);
        if (ret < 0) {
            err("register_framebuffer failed\n");
            goto err;
        }
    }
    if (info->fb[3]) {
        ret = register_framebuffer(&info->fb[3]->fb);
        if (ret < 0) {
            err("register_framebuffer failed\n");
            goto err;
        }
    }
    //platform_pmu_switch_pinmux(info, 1); /* move to other place because info->CCIR656.Parameter is not assigned yet */

    ret = lcd200_register_sysfs(dev_info->dev);
    if (ret < 0)
        goto err;
    p_dev_info = info;

    ret = dev_proc_init();
    if (ret < 0)
        goto err;

    DBGLEAVE(1);
    return ret;
  err:
    dev_deconstruct(info);
    DBGLEAVE(1);
    return ret;
}

//EXPORT_SYMBOL(dev_construct);

static u64 lcd200_dmamask = ~(u32) 0;

static void lcd200_platform_release(struct device *dev)
{
}

#ifdef LCD_DEV2
static struct resource lcd200_resource[] = {
    [0] = {
           .start = LCD_FTLCDC200_2_PA_BASE,
           .end = LCD_FTLCDC200_2_PA_LIMIT,
           .flags = IORESOURCE_MEM,
           },
    [1] = {
           .start = LCD_FTLCDC200_2_IRQ,
           .end = LCD_FTLCDC200_2_IRQ,
           .flags = IORESOURCE_IRQ,
           },
};

static struct platform_device lcd200_device = {
    .name = "flcd200_2",
    .id = -1,
    .num_resources = ARRAY_SIZE(lcd200_resource),
    .resource = lcd200_resource,
    .dev = {
            .dma_mask = &lcd200_dmamask,
            .coherent_dma_mask = 0xffffffff,
            .release = lcd200_platform_release,
            }
};

#elif defined(LCD_DEV1)

static struct resource lcd200_resource[] = {
    [0] = {
           .start = LCD_FTLCDC200_1_PA_BASE,
           .end = LCD_FTLCDC200_1_PA_LIMIT,
           .flags = IORESOURCE_MEM,
           },
    [1] = {
           .start = LCD_FTLCDC200_1_IRQ,
           .end = LCD_FTLCDC200_1_IRQ,
           .flags = IORESOURCE_IRQ,
           },
};

static struct platform_device lcd200_device = {
    .name = "flcd200_1",
    .id = -1,
    .num_resources = ARRAY_SIZE(lcd200_resource),
    .resource = lcd200_resource,
    .dev = {
            .dma_mask = &lcd200_dmamask,
            .coherent_dma_mask = 0xffffffff,
            .release = lcd200_platform_release,
            }
};

#else

static struct resource lcd200_resource[] = {
    [0] = {
           .start = LCD_FTLCDC200_0_PA_BASE,
           .end = LCD_FTLCDC200_0_PA_LIMIT,
           .flags = IORESOURCE_MEM,
           },
    [1] = {
           .start = LCD_FTLCDC200_0_IRQ,
           .end = LCD_FTLCDC200_0_IRQ,
           .flags = IORESOURCE_IRQ,
           },
};

static struct platform_device lcd200_device = {
    .name = "flcd200",
    .id = -1,
    .num_resources = ARRAY_SIZE(lcd200_resource),
    .resource = lcd200_resource,
    .dev = {
            .dma_mask = &lcd200_dmamask,
            .coherent_dma_mask = 0xffffffff,
            .release = lcd200_platform_release,
            }
};
#endif /* LCD_DEV1 */

/**
 *@brief Register LCD200 device
 */
int lcd200_init(void)
{
    int ret = 0;

    DBGENTER(1);
    /// Register the device with LDM
    if (platform_device_register(&lcd200_device)) {
        err("failed to register LCD200 device\n");
        ret = -ENODEV;
        goto exit;
    }
    ret = ffb_proc_init(lcd200_device.name);
    if (ret < 0)
        goto err;
    if (ret == 0) {
        info("[ver:%s]INIT %s OK.", FFB_VERSION, lcd200_device.name);
    }

  exit:
    DBGLEAVE(1);
    return ret;
//  err1:
    ffb_proc_release((char *)lcd200_device.name);
  err:
    platform_device_unregister(&lcd200_device);
    DBGLEAVE(1);
    return ret;
}

int lcd200_cleanup(void)
{
    DBGENTER(1);
    ffb_proc_release((char *)lcd200_device.name);
    platform_device_unregister(&lcd200_device);
    DBGLEAVE(1);
    return 0;
}
