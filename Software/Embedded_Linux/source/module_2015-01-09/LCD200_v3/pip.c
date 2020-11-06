#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fb.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <linux/platform_device.h>
#include <mach/fmem.h>
#include <mach/platform/platform_io.h>

#include "ffb.h"
#include "dev.h"
#include "proc.h"
#include "debug.h"
#include "simple_osd.h"
#include "codec.h"
#include "pip.h"
#include "platform.h"
#include "cursor_comm.h"

#define LCD_VERION  "1.2.2"

#ifdef LCD_DEV2
#define LCD200_ID   LCD2_ID
#elif defined(LCD_DEV1)
#define LCD200_ID   LCD1_ID
#else
#define LCD200_ID   LCD_ID
#endif

#ifdef PIP_FB0_NUM
static ushort fb0_num = PIP_FB0_NUM;
#elif defined(PIP1_FB0_NUM)
static ushort fb0_num = PIP1_FB0_NUM;
#elif defined(PIP2_FB0_NUM)
static ushort fb0_num = PIP2_FB0_NUM;
#else
static ushort fb0_num = 10;
#endif
module_param(fb0_num, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fb0_num, "Frame buffer0's number");

#ifdef PIP_FB1_NUM
static ushort fb1_num = PIP_FB1_NUM;
#elif  defined(PIP1_FB1_NUM)
static ushort fb1_num = PIP1_FB1_NUM;
#elif  defined(PIP2_FB1_NUM)
static ushort fb1_num = PIP2_FB1_NUM;
#else
static ushort fb1_num = 1;
#endif
module_param(fb1_num, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fb1_num, "Frame buffer1's number");

#ifdef PIP_FB2_NUM
static ushort fb2_num = PIP_FB2_NUM;
#elif  defined(PIP1_FB2_NUM)
static ushort fb2_num = PIP1_FB2_NUM;
#elif  defined(PIP2_FB2_NUM)
static ushort fb2_num = PIP2_FB2_NUM;
#else
static ushort fb2_num = 1;
#endif
module_param(fb2_num, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fb2_num, "Frame buffer2's number");

static ushort fb3_num = 1;

#ifdef PIP_OUTPUT_TYPE
static ushort output_type = PIP_OUTPUT_TYPE;
#elif defined(PIP1_OUTPUT_TYPE)
static ushort output_type = PIP1_OUTPUT_TYPE;
#elif defined(PIP2_OUTPUT_TYPE)
static ushort output_type = PIP2_OUTPUT_TYPE;
#else
static ushort output_type = 0;
#endif

int horizon_shift = 0;
int vertical_shift = 0;
module_param(output_type, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(output_type,
                 "0:SA7121, 1:CS4954, 2:ADV739X,3:PVI_2003A,4:FS453, 5:MIN200 HD, 6:MDIN200 SD, 7:640x480 VGA DAC, 8:1024x768 VGA DAC, 9:1280x800 VGA DAC");

/* input resolution */
#ifdef PIP_INPUT_RES
static ushort input_res = PIP_INPUT_RES;
#elif defined(PIP1_INPUT_RES)
static ushort input_res = PIP1_INPUT_RES;
#elif defined(PIP2_INPUT_RES)
static ushort input_res = PIP2_INPUT_RES;
#else
static ushort input_res = VIN_NONE;
#endif

unsigned int denominator = 30, numerator = 30;

int lcd_disable = 0;
module_param(lcd_disable, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(lcd_disable, "lcd disable");

module_param(input_res, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(input_res,
                 "0:D1, 1:PAL, 2:640x480, 3:1024x768 VGA DAC, 4:1440x1080, 5:1280x800, 6:1280x960, 7:1280x1024, 8:1360x768, 9:720P, 15:1920x1080");

int fb0_fb1_share = 0;
module_param(fb0_fb1_share, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fb0_fb1_share, "fb0-fb1 share the same frame buffer");

int fb1_pan_display = 0;
module_param(fb1_pan_display, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fb1_pan_display, "linux fb_pan_display for fb1 only");

/*
 * FB0 input format support
 */
#if defined(PIP_FB0_SUPPORT_YUV422) || defined(PIP1_FB0_SUPPORT_YUV422) || defined(PIP2_FB0_SUPPORT_YUV422)
#define FB0_SUP_IN_YUV422  FFB_SUPORT_CFG(VIM_YUV422)
#else
#define FB0_SUP_IN_YUV422  0
#endif

#if defined(PIP_FB0_SUPPORT_ARGB) || defined(PIP1_FB0_SUPPORT_ARGB) || defined(PIP2_FB0_SUPPORT_ARGB)
#define FB0_SUP_IN_ARGB  FFB_SUPORT_CFG(VIM_ARGB)
#else
#define FB0_SUP_IN_ARGB  0
#endif

#if defined(PIP_FB0_SUPPORT_RGB888) || defined(PIP1_FB0_SUPPORT_RGB888) || defined(PIP2_FB0_SUPPORT_RGB888)
#define FB0_SUP_IN_RGB888  FFB_SUPORT_CFG(VIM_RGB888)
#else
#define FB0_SUP_IN_RGB888  0
#endif

#if defined(PIP_FB0_SUPPORT_RGB565) || defined(PIP1_FB0_SUPPORT_RGB565) || defined(PIP2_FB0_SUPPORT_RGB565)
#define FB0_SUP_IN_RGB565  FFB_SUPORT_CFG(VIM_RGB565)
#else
#define FB0_SUP_IN_RGB565  0
#endif

#if defined(PIP_FB0_SUPPORT_RGB8) || defined(PIP1_FB0_SUPPORT_RGB8) || defined(PIP2_FB0_SUPPORT_RGB8)
#define FB0_SUP_IN_RGB8  FFB_SUPORT_CFG(VIM_RGB8)
#else
#define FB0_SUP_IN_RGB8  0
#endif

#if defined(PIP_FB0_SUPPORT_RGB1555) || defined(PIP1_FB0_SUPPORT_RGB1555) || defined(PIP2_FB0_SUPPORT_RGB1555)
#define FB0_SUP_IN_RGB1555  FFB_SUPORT_CFG(VIM_RGB1555)
#else
#define FB0_SUP_IN_RGB1555  0
#endif

#if defined(PIP_FB0_SUPPORT_RGB1BPP) || defined(PIP1_FB0_SUPPORT_RGB1BPP) || defined(PIP2_FB0_SUPPORT_RGB1BPP)
#define FB0_SUP_IN_RGB1BPP  FFB_SUPORT_CFG(VIM_RGB1BPP)
#else
#define FB0_SUP_IN_RGB1BPP  0
#endif

#if defined(PIP_FB0_SUPPORT_RGB2BPP) || defined(PIP1_FB0_SUPPORT_RGB2BPP) || defined(PIP2_FB0_SUPPORT_RGB2BPP)
#define FB0_SUP_IN_RGB2BPP  FFB_SUPORT_CFG(VIM_RGB2BPP)
#else
#define FB0_SUP_IN_RGB2BPP  0
#endif


#define FB0_SUP_IN_FORMAT (FB0_SUP_IN_YUV422|FB0_SUP_IN_ARGB|FB0_SUP_IN_RGB888|FB0_SUP_IN_RGB565|FB0_SUP_IN_RGB8|FB0_SUP_IN_RGB1555|FB0_SUP_IN_RGB1BPP|FB0_SUP_IN_RGB2BPP)

/*
 * FB1 input format support
 */
#if defined(PIP_FB1_SUPPORT_YUV422) || defined(PIP1_FB1_SUPPORT_YUV422) || defined(PIP2_FB1_SUPPORT_YUV422)
#define FB1_SUP_IN_YUV422  FFB_SUPORT_CFG(VIM_YUV422)
#else
#define FB1_SUP_IN_YUV422  0
#endif

#if defined(PIP_FB1_SUPPORT_ARGB) || defined(PIP1_FB1_SUPPORT_ARGB) || defined(PIP2_FB1_SUPPORT_ARGB)
#define FB1_SUP_IN_ARGB  FFB_SUPORT_CFG(VIM_ARGB)
#else
#define FB1_SUP_IN_ARGB  0
#endif

#if defined(PIP_FB1_SUPPORT_RGB888) || defined(PIP1_FB1_SUPPORT_RGB888) || defined(PIP2_FB1_SUPPORT_RGB888)
#define FB1_SUP_IN_RGB888  FFB_SUPORT_CFG(VIM_RGB888)
#else
#define FB1_SUP_IN_RGB888  0
#endif

#if defined(PIP_FB1_SUPPORT_RGB565) || defined(PIP1_FB1_SUPPORT_RGB565) || defined(PIP2_FB1_SUPPORT_RGB565)
#define FB1_SUP_IN_RGB565  FFB_SUPORT_CFG(VIM_RGB565)
#else
#define FB1_SUP_IN_RGB565  0
#endif

#if defined(PIP_FB1_SUPPORT_RGB8) || defined(PIP1_FB1_SUPPORT_RGB8) || defined(PIP2_FB1_SUPPORT_RGB8)
#define FB1_SUP_IN_RGB8  FFB_SUPORT_CFG(VIM_RGB8)
#else
#define FB1_SUP_IN_RGB8  0
#endif

#if defined(PIP_FB1_SUPPORT_RGB1555) || defined(PIP1_FB1_SUPPORT_RGB1555) || defined(PIP2_FB1_SUPPORT_RGB1555)
#define FB1_SUP_IN_RGB1555  FFB_SUPORT_CFG(VIM_RGB1555)
#else
#define FB1_SUP_IN_RGB1555  0
#endif

#if defined(PIP_FB1_SUPPORT_RGB1BPP) || defined(PIP1_FB1_SUPPORT_RGB1BPP) || defined(PIP2_FB1_SUPPORT_RGB1BPP)
#define FB1_SUP_IN_RGB1BPP  FFB_SUPORT_CFG(VIM_RGB1BPP)
#else
#define FB1_SUP_IN_RGB1BPP  0
#endif

#if defined(PIP_FB1_SUPPORT_RGB2BPP) || defined(PIP1_FB1_SUPPORT_RGB2BPP) || defined(PIP2_FB1_SUPPORT_RGB2BPP)
#define FB1_SUP_IN_RGB2BPP  FFB_SUPORT_CFG(VIM_RGB2BPP)
#else
#define FB1_SUP_IN_RGB2BPP  0
#endif

#define FB1_SUP_IN_FORMAT (FB1_SUP_IN_YUV422|FB1_SUP_IN_ARGB|FB1_SUP_IN_RGB888|FB1_SUP_IN_RGB565|FB1_SUP_IN_RGB8|FB1_SUP_IN_RGB1555|FB1_SUP_IN_RGB1BPP|FB1_SUP_IN_RGB2BPP)

/*
 * FB2 input format support
 */
#if defined(PIP_FB2_SUPPORT_YUV422) || defined(PIP1_FB2_SUPPORT_YUV422) || defined(PIP2_FB2_SUPPORT_YUV422)
#define FB2_SUP_IN_YUV422  FFB_SUPORT_CFG(VIM_YUV422)
#else
#define FB2_SUP_IN_YUV422  0
#endif

#if defined(PIP_FB2_SUPPORT_ARGB) || defined(PIP1_FB2_SUPPORT_ARGB) || defined(PIP2_FB2_SUPPORT_ARGB)
#define FB2_SUP_IN_ARGB  FFB_SUPORT_CFG(VIM_ARGB)
#else
#define FB2_SUP_IN_ARGB  0
#endif

#if defined(PIP_FB2_SUPPORT_RGB888) || defined(PIP1_FB2_SUPPORT_RGB888) || defined(PIP2_FB2_SUPPORT_RGB888)
#define FB2_SUP_IN_RGB888  FFB_SUPORT_CFG(VIM_RGB888)
#else
#define FB2_SUP_IN_RGB888  0
#endif

#if defined(PIP_FB2_SUPPORT_RGB565) || defined(PIP1_FB2_SUPPORT_RGB565) || defined(PIP2_FB2_SUPPORT_RGB565)
#define FB2_SUP_IN_RGB565  FFB_SUPORT_CFG(VIM_RGB565)
#else
#define FB2_SUP_IN_RGB565  0
#endif

#if defined(PIP_FB2_SUPPORT_RGB8) || defined(PIP1_FB2_SUPPORT_RGB8) || defined(PIP2_FB2_SUPPORT_RGB8)
#define FB2_SUP_IN_RGB8  FFB_SUPORT_CFG(VIM_RGB8)
#else
#define FB2_SUP_IN_RGB8  0
#endif

#if defined(PIP_FB2_SUPPORT_RGB1555) || defined(PIP1_FB2_SUPPORT_RGB1555) || defined(PIP2_FB2_SUPPORT_RGB1555)
#define FB2_SUP_IN_RGB1555  FFB_SUPORT_CFG(VIM_RGB1555)
#else
#define FB2_SUP_IN_RGB1555  0
#endif

#if defined(PIP_FB2_SUPPORT_RGB1BPP) || defined(PIP1_FB2_SUPPORT_RGB1BPP) || defined(PIP2_FB2_SUPPORT_RGB1BPP)
#define FB2_SUP_IN_RGB1BPP  FFB_SUPORT_CFG(VIM_RGB1BPP)
#else
#define FB2_SUP_IN_RGB1BPP  0
#endif

#if defined(PIP_FB2_SUPPORT_RGB2BPP) || defined(PIP1_FB2_SUPPORT_RGB2BPP) || defined(PIP2_FB2_SUPPORT_RGB2BPP)
#define FB2_SUP_IN_RGB2BPP  FFB_SUPORT_CFG(VIM_RGB2BPP)
#else
#define FB2_SUP_IN_RGB2BPP  0
#endif

#define FB2_SUP_IN_FORMAT (FB2_SUP_IN_YUV422|FB2_SUP_IN_ARGB|FB2_SUP_IN_RGB888|FB2_SUP_IN_RGB565|FB2_SUP_IN_RGB8|FB2_SUP_IN_RGB1555|FB2_SUP_IN_RGB1BPP|FB2_SUP_IN_RGB2BPP)

#ifdef PIP_FB0_DEF_MODE
static ushort fb0_def_mode = PIP_FB0_DEF_MODE;
#elif defined(PIP1_FB0_DEF_MODE)
static ushort fb0_def_mode = PIP1_FB0_DEF_MODE;
#elif defined(PIP2_FB0_DEF_MODE)
static ushort fb0_def_mode = PIP2_FB0_DEF_MODE;
#else
static ushort fb0_def_mode = 0;
#endif

#ifdef PIP_FB1_DEF_MODE
static ushort fb1_def_mode = PIP_FB1_DEF_MODE;
#elif defined(PIP1_FB1_DEF_MODE)
static ushort fb1_def_mode = PIP1_FB1_DEF_MODE;
#elif defined(PIP2_FB1_DEF_MODE)
static ushort fb1_def_mode = PIP2_FB1_DEF_MODE;
#else
static ushort fb1_def_mode = 0;
#endif

#ifdef PIP_FB2_DEF_MODE
static ushort fb2_def_mode = PIP_FB2_DEF_MODE;
#elif defined(PIP1_FB2_DEF_MODE)
static ushort fb2_def_mode = PIP1_FB2_DEF_MODE;
#elif defined(PIP2_FB2_DEF_MODE)
static ushort fb2_def_mode = PIP2_FB2_DEF_MODE;
#else
static ushort fb2_def_mode = 0;
#endif

#ifdef LCD210                   // workaround
extern void lcd210_set_prst(struct ffb_dev_info *ff_dev_info);
#endif

static int vs_width = 0;
module_param(vs_width, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vs_width, "width in virtual screen");

static int vs_height = 0;
module_param(vs_height, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vs_height, "height in virtual screen");

static struct task_struct *lcd0_thread = NULL;
static struct task_struct *lcd2_thread = NULL;
volatile int thread_init = 0;

/* lcdc bit map for GUI share */
static u32 lcd_gui_bitmap = 0;
static struct gui_clone_adjust  gui_clone_adjust = {0};

/* extern functions
 */
extern struct ffb_timing_info *get_output_timing(struct OUTPUT_INFO *output, u_char type);
extern int dev_setting_ffb_output(struct lcd200_dev_info *info);
int pip_config_resolution(ushort input_res, ushort output_type);

static void __exit pip_cleanup(void);
static u32 pip_frame_cnt = 0;
static int probe_ret = -1;
/*
 * main structure of PIP
 */
struct pip_dev_info {
    struct lcd200_dev_info lcd200_info;
    u_char PIP_En:2;
    u_char Blend_En:2;
    codec_setup_t codec_device;
    u32 edid_array[(OUTPUT_TYPE_LAST + 7) / 8];
    u32 fb0_fb1_share;
};

static DECLARE_WAIT_QUEUE_HEAD(lcd_wq);
unsigned int TVE100_BASE = 0;

static struct pip_dev_info g_dev_info = { {{0}} };
static struct pip_vg_cb *lcdvg_cb_fn = NULL;

/* local function declaration */
static unsigned int pip_get_phyaddr(struct lcd200_dev_info *info, int idx);

#ifdef CONFIG_PLATFORM_GM8210
static int lcd_polling_thread(void *private);
#endif /* CONFIG_PLATFORM_GM8210 */

/* If the scalar is on, the cusor need to adjust to meet the output resolution
 */
inline void get_hardware_cusor_xy(struct lcd200_dev_info *dev_info)
{
    struct ffb_dev_info *fdev_info = (struct ffb_dev_info *)dev_info;
    int dx, dy, xres = 0, yres = 0, xpos = 0, ypos = 0;
    u32 priv_data;
    struct gui_clone_adjust  *clone_adjust;

    /*
     * In dual cpu environment(GM8210), the user application only operate the FA726 cpu but pip driver runs
     * in FA626. Thus dev_info->cursor.enable is always 0.
     *
     * The path:
     *  1. FA726 framebuffer.c will update the cursor position to hardware.
     *  2. lcdvg_process_gui_clone() of lcd_vg.c will read x/y from the hardware and call
     *      lcd_update_cursor_info() to update the cursor information about x,y ... every interrupt.
     *  3. get_hardware_cusor_xy() is also to get the cursor info and update the owned hardware if
     *      I am the one of targets.
     */
    lcd_get_cursor_info(dev_info->lcd200_idx, &xres, &yres, &xpos, &ypos, &priv_data);
#ifdef CONFIG_PLATFORM_GM8210
    /* I am not the target */
    if (!xres || !yres)
        return;
#else
    if (dev_info->cursor.enable == 0)
        return;
#endif

    /* if the main LCD update its position, then receiver must adopt this position corresponding. */
    if (xres && yres) {
        clone_adjust = (struct gui_clone_adjust *)priv_data;

        if (clone_adjust->width && clone_adjust->height ){
           dx = clone_adjust->x + (xpos * clone_adjust->width ) / xres;
           dy = clone_adjust->y + (ypos * clone_adjust->height) / yres;
        } else {
            dx = (xpos * fdev_info->input_res->xres) / xres;
            dy = (ypos * fdev_info->input_res->yres) / yres;
        }
    } else {
        /* from IOCTL directly */
        dx = dev_info->cursor.dx;
        dy = dev_info->cursor.dy;
    }

#ifdef CURSOR_64x64
    iowrite32(((dx & 0xFFF) << 16) | (dy & 0xFFF) | (0x1 << 28), fdev_info->io_base + LCD_CURSOR_POS);
#else
    iowrite32((dev_info->cursor_32x32 << 29) | ((dx & 0xFFF) << 16) | (dy & 0xFFF) | (0x1 << 28),
              fdev_info->io_base + LCD_CURSOR_POS);
#endif /* CURSOR_64x64 */

#ifdef LCD210
    lcd210_set_prst(fdev_info);
#endif
}

/*
 * Some TVs don't follow standard timing. Here we provide the function to adjust the screen poistion
 */
static int calculate_tvscreen_shift(struct lcd200_dev_info *dev_info, int fb_idx)
{
    struct ffb_dev_info *fdev_info = (struct ffb_dev_info *)dev_info;
    struct ffb_timing_info *timing;
    int shift, pixel_per_bytes;

    if ((fdev_info->video_output_type != VOT_PAL) && (fdev_info->video_output_type != VOT_NTSC))
        return 0;

    if (!horizon_shift && !vertical_shift)
        return 0;

    pixel_per_bytes = dev_info->fb[fb_idx]->fb.var.bits_per_pixel / 8;
    timing = get_output_timing(fdev_info->output_info, fdev_info->video_output_type);
    shift =
        horizon_shift * pixel_per_bytes +
        timing->data.ccir656.in.xres * pixel_per_bytes * vertical_shift;

    return shift;
}

static int underrun = 0;
static irqreturn_t pip_handle_irq(int irq, void *dev_id)
{
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)dev_id;
    struct ffb_dev_info *fdev_info = (struct ffb_dev_info *)dev_id;
    u32 status, phyaddr;
    int shift = 0;

    status = ioread32(fdev_info->io_base + INT_STATUS);

    if (unlikely(status & 0x08)) {
        if (printk_ratelimit())
            err("LCD%d: AHB master error !", dev_info->lcd200_idx);
    }

    if (unlikely(status & 0x01)) {
        underrun++;
        if (printk_ratelimit())
            err("LCD%d: FIFO under-run!", dev_info->lcd200_idx);
    } else {
        underrun = 0;
    }

    /* hardware cursor update */
    get_hardware_cusor_xy(dev_info);

    if (likely(status & 0x2)) {  //IntNxtBase
    }

    if (likely(status & 0x4)) {  //IntVstatus
        int idx;

        for (idx = 0; idx < PIP_NUM; idx++) {
            if (dev_info->fb[idx] == NULL)
                continue;

            if ((g_dev_info.PIP_En == 0) && (idx > 0))
                break;

            if ((g_dev_info.PIP_En == 1) && (idx > 1))
                break;

            if ((g_dev_info.PIP_En == 2) && (idx > 2))
                break;

            shift = calculate_tvscreen_shift(dev_info, idx);

            /* get one frame for display
             */
            phyaddr = pip_get_phyaddr(dev_info, idx);
            if (phyaddr != (u32) (-1))
                iowrite32(phyaddr + shift, fdev_info->io_base + FB0_BASE + 0x0C * idx);
        }

        pip_frame_cnt ++;
        wake_up_interruptible(&lcd_wq);
    }

    /* update osd if changed */
    fosd_handler();
    iowrite32(status, fdev_info->io_base + INT_CLEAR);
    return IRQ_HANDLED;
}

/**
 *@brief Set image color format that will be put on frame buffer,when PIP/POP enable.
 */
static void set_color_format(int index, u_char input_mode, uint32_t *value)
{
    uint32_t tmp = *value;

    tmp &= ~((0xf << (4 * index)) | (0x03 << (24 + 2 * index)));

    switch (input_mode) {
    case VIM_YUV422:
        tmp |= (0x8 << (4 * index));
        DBGPRINT(4, "Fb%d Switch to YUV422\n", index);
        break;
    case VIM_ARGB:
        tmp |= (0x6 << (4 * index));
        DBGPRINT(4, "Fb%d Switch to ARGB\n", index);
        break;
    case VIM_RGB888:
        tmp |= (0x5 << (4 * index));
        DBGPRINT(4, "Fb%d Switch to RGB888\n", index);
        break;
    case VIM_RGB565:
        tmp |= (0x04 << (4 * index));
        DBGPRINT(4, "Fb%d Switch to RGB565\n", index);
        break;
    case VIM_RGB8:
        tmp |= (0x03 << (4 * index));
        DBGPRINT(4, "Fb%d Switch to RGB8\n", index);
        break;
    case VIM_RGB1555:
        tmp |= (0x7 << (4 * index));
        tmp |= (0x1 << (24 + 2 * index));   /* RGB555 */
        DBGPRINT(4, "Fb%d Switch to RGB1555\n", index);
        break;
    case VIM_RGB1BPP:
        tmp |= (0x0 << (4 * index));
        DBGPRINT(4, "Fb%d Switch to RGB1BPP\n", index);
        break;
    case VIM_RGB2BPP:
        tmp |= (0x1 << (4 * index));
        DBGPRINT(4, "Fb%d Switch to RGB2BPP\n", index);
        break;
    default:
        return;
    }
    *value = tmp;
}

/**
 *@brief Set position and dimination of PIP sub-window
 */
static int pip_set_window(struct pip_dev_info *pip_info, struct ffb_rect *rect, int index)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)pip_info;
    int ret = 0, osd_line;
    u32 tmp, height;

    if (index < 1 || index > 3)
        return -1;

    if ((rect->x + rect->width) > info->fb[0]->fb.var.xres) {
        ret = -1;
        goto err;
    }
    if ((rect->y + rect->height) > info->fb[0]->fb.var.yres) {
        ret = -1;
        goto err;
    }

    osd_line = platform_get_osdline();

    if (index == 2) {
        info->ops->reg_write((struct ffb_dev_info *)info, PIP_PICT2_POS,
                             rect->x << 16 | rect->y, 0x7ff07ff);
        info->ops->reg_write((struct ffb_dev_info *)info, PIP_PICT2_DIM,
                             rect->width << 16 | rect->height, 0x7ff07ff);
    } else {
        info->ops->reg_write((struct ffb_dev_info *)info, PIP_PICT1_POS,
                             rect->x << 16 | rect->y, 0x7ff07ff);

        /* FIXME, rect->height-1 avoid heavy uderrun to freeze LCD
         */
        if (osd_line == 1) {
            /* FIXME, osd_line = 1 means it is 8181, 2 means 8181T 90nm */
            height =
                (((struct ffb_dev_info *)info)->video_output_type >=
                 VOT_CCIR656) ? rect->height : rect->height - 1;
        } else {
            /* 8181T 90nm */
            height =
                (((struct ffb_dev_info *)info)->video_output_type >=
                 VOT_CCIR656) ? rect->height - osd_line : rect->height - osd_line;
        }

#ifdef LCD210
        info->ops->reg_write((struct ffb_dev_info *)info, PIP_PICT1_DIM,
                             rect->width << 16 | height | 0x1 << 27, 0x7ff07ff);
#else
        info->ops->reg_write((struct ffb_dev_info *)info, PIP_PICT1_DIM,
                             rect->width << 16 | height, 0x7ff07ff);
#endif
    }

#ifdef LCD210
    tmp = info->ops->reg_read((struct ffb_dev_info *)info, PIP_PICT1_DIM);
    info->ops->reg_write((struct ffb_dev_info *)info, PIP_PICT1_DIM, tmp | (0x1 << 27), tmp);
    lcd210_set_prst((struct ffb_dev_info *)info);
#else
    tmp = info->ops->reg_read((struct ffb_dev_info *)info, PIP_PICT1_POS);
    /* PiPUpdate(bit 28), when set to 1, actually update to hardware */
    info->ops->reg_write((struct ffb_dev_info *)info, PIP_PICT1_POS, (tmp | 0x1 << 28), tmp);
#endif

    return ret;
  err:
    err("Out of window!(x=%d,y=%d,w=%d,h=%d)(max_xres=%d, max_yres=%d)",
        rect->x, rect->y, rect->width, rect->height, info->fb[0]->fb.var.xres,
        info->fb[0]->fb.var.yres);
    return ret;
}

/**
 *@brief Get position and dimination of PIP sub-window
 */
static int pip_get_window(struct pip_dev_info *pip_info, struct ffb_rect *rect, int index)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)pip_info;
    int ret = 0, osd_line;
    u32 tmp;

    if (index < 1 || index > 2)
        return -1;

    osd_line = platform_get_osdline();

    if (index == 2) {
        tmp = info->ops->reg_read((struct ffb_dev_info *)info, PIP_PICT2_POS);
        rect->x = (tmp >> 16) & 0x7ff;
        rect->y = (tmp) & 0x7ff;
        tmp = info->ops->reg_read((struct ffb_dev_info *)info, PIP_PICT2_DIM);
        rect->width = (tmp >> 16) & 0x7ff;
        rect->height = (tmp) & 0x7ff;
    } else {
        tmp = info->ops->reg_read((struct ffb_dev_info *)info, PIP_PICT1_POS);
        rect->x = (tmp >> 16) & 0x7ff;
        rect->y = (tmp) & 0x7ff;
        tmp = info->ops->reg_read((struct ffb_dev_info *)info, PIP_PICT1_DIM);
        rect->width = (tmp >> 16) & 0x7ff;
        rect->height = (tmp) & 0x7ff;

#if 1                           /* FIXME, rect->height-1 avoid heavy uderrun to freeze LCD */
        if (osd_line == 1) {
            /* FIXME, osd_line = 1 means it is 8181, 2 means 8181T 90nm */
            rect->height =
                (((struct ffb_dev_info *)info)->video_output_type >=
                 VOT_CCIR656) ? rect->height : rect->height + 1;
        } else {
            /* 8181T 90nm */
            rect->height =
                (((struct ffb_dev_info *)info)->video_output_type >=
                 VOT_CCIR656) ? rect->height + osd_line : rect->height + osd_line;
        }
#endif
    }
    return ret;
}

static int pip_get_blend_value(struct pip_dev_info *pip_info, int *blend_h, int *blend_l)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)pip_info;
    int ret = 0;
    uint32_t tmp;

    DBGENTER(1);
    tmp = info->ops->reg_read((struct ffb_dev_info *)info, PIP_BLEND);

#ifdef LCD210
    *blend_h = (tmp >> 8) & 0xff;
    *blend_l = (tmp) & 0xff;
#else
    *blend_h = (tmp >> 8) & 0x1f;
    *blend_l = (tmp) & 0x1f;
#endif
    DBGLEAVE(1);
    return ret;
}

static int pip_set_blend_value(struct pip_dev_info *pip_info, int blend_h, int blend_l)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)pip_info;
    int ret = 0;
    uint32_t tmp;

    DBGENTER(1);

    tmp = info->ops->reg_read((struct ffb_dev_info *)info, PIP_BLEND);

#ifdef LCD210
    if (blend_h < 0)
        blend_h = (tmp >> 8) & 0xff;
    if (blend_h > 256)
        blend_h = 256;

    if (blend_l < 0)
        blend_l = (tmp) & 0xff;

    if (blend_l > 256)
        blend_l = 256;

    info->ops->reg_write((struct ffb_dev_info *)info, PIP_BLEND, (blend_h << 8 | blend_l), 0xffff);
#else
    if (blend_h < 0)
        blend_h = (tmp >> 8) & 0x1f;
    if (blend_h > 16)
        blend_h = 16;

    if (blend_l < 0)
        blend_l = (tmp) & 0x1f;

    if (blend_l > 16)
        blend_l = 16;

    info->ops->reg_write((struct ffb_dev_info *)info, PIP_BLEND, (blend_h << 8 | blend_l), 0x1f1f);
#endif

    DBGLEAVE(1);
    return ret;
}

/**
 *@brief Switch blend mode(REG:offset 0:bit9-8).
 *
 * This value must be 2, when either frame buffer's color format is ARGB
 */
static int pip_switch_blend_mode(struct pip_dev_info *pip_info)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)pip_info;
    int ret = 0;

    DBGENTER(1);
    if (pip_info->PIP_En != 0) {
        if ((info->fb[0] && info->fb[0]->video_input_mode == VIM_ARGB) ||
            (info->fb[0] && info->fb[0]->video_input_mode == VIM_RGB1555) ||
            (info->fb[1] && info->fb[1]->video_input_mode == VIM_ARGB) ||
            (info->fb[1] && info->fb[1]->video_input_mode == VIM_RGB1555) ||
            (info->fb[2] && info->fb[2]->video_input_mode == VIM_ARGB) ||
            (info->fb[2] && info->fb[2]->video_input_mode == VIM_RGB1555)) {
            pip_info->Blend_En = 2; /* Alpha Blending */
        } else {
            pip_info->Blend_En = 1;
        }
    }
    info->ops->reg_write((struct ffb_dev_info *)info, FEATURE, pip_info->Blend_En << 8, 0x300);
    DBGLEAVE(1);
    return ret;
}

/**
 *@brief Set image color format that will be put on frame buffer.
 */
static int pip_switch_input_mode(struct pip_dev_info *pip_info)
{
    int ret = 0;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)pip_info;

    DBGENTER(1);
    if (pip_info == NULL) {
        err("pip_dev_info is NULL");
        return -1;
    }
    if (pip_info->PIP_En == 0) {
        switch (info->fb[0]->video_input_mode) {
        case VIM_YUV422:
            info->ops->reg_write((struct ffb_dev_info *)info, FEATURE, 0x8, 0xC);
            info->ops->reg_write((struct ffb_dev_info *)info, PANEL_PARAM, 0x04, 0x7);
            DBGPRINT(4, "Fb%d Switch to YUV422\n", 0);
            break;
        case VIM_RGB888:
            info->ops->reg_write((struct ffb_dev_info *)info, FEATURE, 0x0, 0xC);
            info->ops->reg_write((struct ffb_dev_info *)info, PANEL_PARAM, 0x05, 0x7);
            DBGPRINT(4, "Fb%d Switch to RGB888\n", 0);
            break;
        case VIM_RGB565:
            info->ops->reg_write((struct ffb_dev_info *)info, FEATURE, 0x0, 0xC);
            info->ops->reg_write((struct ffb_dev_info *)info, PANEL_PARAM, 0x04, 0x187);
            DBGPRINT(4, "Fb%d Switch to RGB565\n", 0);
            break;
        case VIM_RGB8:
            info->ops->reg_write((struct ffb_dev_info *)info, FEATURE, 0x0, 0xC);
            info->ops->reg_write((struct ffb_dev_info *)info, PANEL_PARAM, 0x03, 0x7);
            DBGPRINT(4, "Fb%d Switch to RGB8\n", 0);
            break;
        case VIM_RGB1BPP:
            info->ops->reg_write((struct ffb_dev_info *)info, FEATURE, 0x0, 0xC);
            info->ops->reg_write((struct ffb_dev_info *)info, PANEL_PARAM, 0x0, 0x7); /* 1 bpp */
            DBGPRINT(4, "Fb%d Switch to RGB-1BPP\n", 0);
            break;
        case VIM_RGB2BPP:
            info->ops->reg_write((struct ffb_dev_info *)info, FEATURE, 0x0, 0xC);
            info->ops->reg_write((struct ffb_dev_info *)info, PANEL_PARAM, 0x1, 0x7); /* 2 bpp */
            DBGPRINT(4, "Fb%d Switch to RGB-2BPP\n", 0);
            break;
        }
        info->ops->switch_input_mode(info->fb[0]);
    } else {
        u32 tmp = 0;
        if (info->fb[0]) {
            set_color_format(0, info->fb[0]->video_input_mode, &tmp);
            info->ops->switch_input_mode(info->fb[0]);
        }
        if (info->fb[1]) {
            set_color_format(1, info->fb[1]->video_input_mode, &tmp);
            info->ops->switch_input_mode(info->fb[1]);
        }
        if (info->fb[2]) {
            set_color_format(2, info->fb[2]->video_input_mode, &tmp);
            info->ops->switch_input_mode(info->fb[2]);
        }
        info->ops->reg_write((struct ffb_dev_info *)info, PIOP_IMG_FMT1, tmp, 0xFFFFFFFF);
    }
    pip_switch_blend_mode(pip_info);
    DBGLEAVE(1);
    return ret;
}

/**
 *@brief Switch PIP mode.
 *
 *@param pip_mode 0: PIP disable,1: Single PIP window,2: Double PIP window
 */
static int pip_switch_pip_mode(struct pip_dev_info *pip_info, u32 pip_mode)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)pip_info;
    int ret = 0;

    DBGENTER(1);
    if (info->fb[1] && info->fb[2])
        pip_info->PIP_En = pip_mode;
    else if (info->fb[1])
        pip_info->PIP_En = (pip_mode > 1) ? 1 : pip_mode;

    DBGPRINT(2, "PIP_MODE=%d(%d)\n", pip_info->PIP_En, pip_mode);
    info->ops->reg_write((struct ffb_dev_info *)info, FEATURE, pip_info->PIP_En << 10, 0xC00);
    ret = pip_switch_input_mode(pip_info);

#ifdef LCD210
    lcd210_set_prst((struct ffb_dev_info *)info);
#endif

    DBGLEAVE(1);
    return ret;
}

/**
 *@brief Set data arrange about progreee or interlace.
 */
static int pip_set_data_arrange(struct pip_dev_info *pip_info, int idx, unsigned int type)
{
    int ret = 0;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)pip_info;

    if (pip_info->PIP_En) {
        info->ops->reg_write((struct ffb_dev_info *)info, PIOP_IMG_FMT2, type << idx, 0x1 << idx);
                /**
		 * FIXME: The image will glide after switch data arrange.
		 *        We used reset device as workaround.
		 */
        info->ops->reset_dev((struct ffb_dev_info *)info);
    } else {
        if (type)
            info->ops->reg_write((struct ffb_dev_info *)info, CCIR656_PARM, 0x04, 0x04);
        else
            info->ops->reg_write((struct ffb_dev_info *)info, CCIR656_PARM, 0x0, 0x04);
#if 0
        /**
		 * FIXME: The image will glide after switch data arrange.
		 *        We used reset device as workaround.
		 */
        info->ops->reset_dev((struct ffb_dev_info *)info);
#endif
    }

    return ret;
}

/*
 * @This function configure the output_type
 *
 * @function int pip_config_edid(output_type_t l_output_type);
 * @param l_output_type indicates the output_type
 * @return 0 on success, <0 on error
 */
int pip_config_edid(output_type_t l_output_type)
{
    int l_input_res, ret = 0;
    struct ffb_dev_info *fdev_info = (struct ffb_dev_info *)&g_dev_info;

    l_input_res = fdev_info->input_res->input_res;
    if ((l_output_type == OUTPUT_TYPE_NTSC_0) || (l_output_type == OUTPUT_TYPE_CAT6611_480P_20) ||
                            (l_output_type == OUTPUT_TYPE_SLI10121_480P_42))
        l_input_res = (l_input_res == VIN_PAL) ? VIN_NTSC : l_input_res;

    if ((l_output_type == OUTPUT_TYPE_PAL_1) || (l_output_type == OUTPUT_TYPE_CAT6611_576P_21) ||
                            (l_output_type == OUTPUT_TYPE_SLI10121_576P_43))
        l_input_res = (l_input_res == VIN_NTSC) ? VIN_PAL : l_input_res;

    if (pip_config_resolution(l_input_res, l_output_type) < 0)
        ret = -EFAULT;

    return ret;
}

/*
 * @This function get the edit table supported by LCD
 *
 * @function int pip_get_edid(unsigned int *edid_table);
 * @param edid indicates the edid table. When the corresponding bit in output_type_t
 *                  in the edid array, it means available.
 * @return 0 on success, <0 on error
 */
int pip_get_edid(struct edid_table *edid)
{
    memcpy((void *)&edid->edid[0], (void *)&g_dev_info.edid_array[0], sizeof(g_dev_info.edid_array));

    return 0;
}

/**
 *@brief Get data arrange about progreee or interlace.
 */
static int pip_get_data_arrange(struct pip_dev_info *pip_info, int idx, unsigned int *type)
{
    int ret = 0;
    unsigned int tmp;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)pip_info;

    if (pip_info->PIP_En) {
        tmp = info->ops->reg_read((struct ffb_dev_info *)info, PIOP_IMG_FMT2);
        *type = (tmp >> idx) & 0x01;
        ret = 0;
    } else {
        tmp = info->ops->reg_read((struct ffb_dev_info *)info, CCIR656_PARM);
        *type = (tmp >> 2) & 0x01;
    }
    return ret;
}

#ifdef VIDEOGRAPH_INC
#include "video_entity.h"   //include video entity manager
#endif

static int pip_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct ffb_info *fbi = (struct ffb_info *)info;
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)fbi->dev_info;
    DBGENTER(1);
    switch (cmd) {
    case FFB_IOSBUFFMT:
        {
            unsigned int tmp;
            DBGPRINT(3, "FFB_IOSBUFFMT:\n");
            if (copy_from_user(&tmp, (unsigned int *)arg, sizeof(unsigned int))) {
                ret = -EFAULT;
                break;
            }
            ret = pip_set_data_arrange(&g_dev_info, fbi->index, tmp);
            break;
        }
    case FFB_IOGBUFFMT:
        {
            unsigned int tmp;
            DBGPRINT(3, "FFB_IOGBUFFMT:\n");
            if (ret < 0)
                break;
            if (pip_get_data_arrange(&g_dev_info, fbi->index, &tmp)) {
                ret = -EFAULT;
                break;
            }
            ret = copy_to_user((void __user *)arg, &tmp, sizeof(tmp)) ? -EFAULT : 0;
            break;
        }

    case FFB_FRAME_CTL:
        {
            unsigned int tmp;
            DBGPRINT(3, "FFB_FRAME_CTL:\n");
            if (copy_from_user(&tmp, (unsigned int *)arg, sizeof(unsigned int))) {
                ret = -EFAULT;
                break;
            }
            if (tmp == 0) {     // init time
                // wait one frame
                interruptible_sleep_on(&lcd_wq);

                ret = copy_to_user((void __user *)arg, &pip_frame_cnt,
                                   sizeof(pip_frame_cnt)) ? -EFAULT : 0;
            } else {
                if (pip_frame_cnt > tmp) {
                    if ((pip_frame_cnt - tmp) > 0xffff) //AP already wraped around, AP is faster.
                    {
                        ret = wait_event_interruptible(lcd_wq, pip_frame_cnt == tmp);
                        break;
                    }
                } else if (pip_frame_cnt < tmp) {
                    if ((tmp - pip_frame_cnt) > 0xffff) //Interrupt already wraped around, AP is slower
                        break;
                    // AP is faster, block AP
                    ret = wait_event_interruptible(lcd_wq, pip_frame_cnt >= tmp);
                }
            }

            break;
        }
    case FFB_GET_EDID:
        if (copy_to_user((void __user *)arg, &g_dev_info.edid_array[0], sizeof(g_dev_info.edid_array)))
            ret = -EFAULT;
        break;
    case FFB_SET_EDID:
    {
        unsigned int tmp;

        if (copy_from_user(&tmp, (unsigned int *)arg, sizeof(unsigned int))) {
            ret = -EFAULT;
            break;
        }
        if (pip_config_edid(tmp) < 0)
            ret = -EFAULT;
        break;
    }

    case FFB_GUI_CLONE:
    {
        u32 bitmap, tmp = 0;
        int i;

        if (copy_from_user(&bitmap, (unsigned int *)arg, sizeof(unsigned int))) {
            ret = -EFAULT;
            break;
        }

        for (i = 0; i < LCD_MAX_ID; i ++)
            set_bit(i, (void *)&tmp);

        if (bitmap > tmp) {
            ret = -EFAULT;
            break;
        }
        printk("LCD%d: GUI content shared with LCD: ", LCD200_ID);
        for (i = 0; i < LCD_MAX_ID; i ++) {
            if (!test_bit(i, (void *)&bitmap))
                continue;
            printk("%d ", i);
        }
        printk("\n");
        lcd_gui_bitmap = bitmap;

        break;
    }

#ifdef VIDEOGRAPH_INC
    case FFB_GUI_CLONE2:
    {
        struct gui_clone2   clone2;
        proc_lcdinfo_t      info;
        int lcd_idx;

        if (copy_from_user(&clone2, (unsigned int *)arg, sizeof(struct gui_clone2))) {
            ret = -EFAULT;
            break;
        }
        if (!clone2.src_pic_pa || !clone2.src_width || !clone2.src_height) {
            ret = -EFAULT;
            break;
        }

        for (lcd_idx = 0; lcd_idx < LCD_MAX_ID; lcd_idx ++) {
            if (!(clone2.dst_lcd_bitmap & (0x1 << lcd_idx)))
                continue;
            if (ffb_proc_get_lcdinfo(lcd_idx, &info)) {
                printk("%s, FFB_GUI_CLONE2, lcd:%d doesn't exist! \n", __func__, lcd_idx);
                continue;
            }
            ret = video_scaling(TYPE_RGB565,
                                clone2.src_pic_pa,
                                clone2.src_width,
                                clone2.src_height,
                                info.gui_info.gui[1].phys_addr,
                                info.gui_info.gui[1].xres,
                                info.gui_info.gui[1].yres);

        }
        break;
    }
#endif /* VIDEOGRAPH_INC */

    case FFB_GUI_CLONE_ADJUST:
        if (copy_from_user(&gui_clone_adjust, (unsigned int *)arg, sizeof(struct gui_clone_adjust))) {
            ret = -EFAULT;
            break;
        }
        printk("LCD%d adjust clone. x: %d, y:%d, width:%d, height: %d \n", LCD200_ID,
                gui_clone_adjust.x, gui_clone_adjust.y, gui_clone_adjust.width, gui_clone_adjust.height);
        break;

    default:
        /* device ioctl: dev_ioctl */
        ret = dev_info->ops->ioctl(info, cmd, arg);
        break;
    }
    DBGLEAVE(1);
    return ret;
}

u32 pip_parking_frame(int plane_idx)
{
    struct ffb_dev_info *fdev_info = (struct ffb_dev_info *)&g_dev_info;
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)&g_dev_info;

    if (plane_idx == -1)
        return -1;              //do nothing

    if (g_dev_info.lcd200_info.fb[plane_idx]) {
        iowrite32(dev_info->fb[plane_idx]->ppfb_dma[0],
                  fdev_info->io_base + FB0_BASE + plane_idx * 0xC);
    }

    return (dev_info->fb[plane_idx]->ppfb_dma[0]);
}

int pip_update_vs(struct ffb_info *fb)
{
    struct ffb_dev_info   *fdev_info = fb->dev_info;
    u32 val, mask, ofs;


    /* do the boundary check */
    if (fb->vs.enable == 1) {
        int ofs;

        ofs = fb->vs.offset_x + fdev_info->input_res->xres;
        if (ofs > fb->vs.width) {
            printk("Hor virutal Screen is out of boundary! \n");
            return -1;
        }

        ofs = fb->vs.offset_y + fdev_info->input_res->yres;
        if (ofs > fb->vs.height) {
            printk("Height virutal Screen is out of boundary! \n");
            return -1;
        }

        fb->vs.frambase_ofs = (fb->vs.offset_y * fb->vs.width + fb->vs.offset_x) * (fb->fb.var.bits_per_pixel >> 3);
    }

    ofs = VIRTUAL_SCREEN0 + fb->index * 4;
    val = ((fb->vs.enable & 0x1) << 20) | (fb->vs.width & 0xFFF);
    mask = (0x1 << 20) | 0xFFF;

    ((struct lcd200_dev_info *)fdev_info)->ops->reg_write(fdev_info, ofs, val, mask);

    return 0;
}

#include "ffb_inres.c"
#include "ffb_output.c"

#ifdef VIDEOGRAPH_INC
#include "lcd_vg.c"
#endif

#include "pip_proc.c"

static int pip_remove(struct platform_device *pdev)
{
    struct lcd200_dev_info *dev_info;
    int irq;

    DBGENTER(1);
    dev_info = (struct lcd200_dev_info *)platform_get_drvdata(pdev);

    codec_driver_deregister(&g_dev_info.codec_device.driver);
    codec_pip_remove_device(g_dev_info.lcd200_info.lcd200_idx, &g_dev_info.codec_device);

    if (dev_info != NULL) {
        pip_proc_remove();
        fsosd_deconstruct(dev_info);
        dev_deconstruct(dev_info);
        irq = platform_get_irq(pdev, 0);
        if (irq >= 0) {
            free_irq(irq, dev_info);
        }
        if (dev_info->fb[0]) {
   		    dev_info->fb[0]->vs.enable = 0;
		    pip_update_vs(dev_info->fb[0]);

            if (dev_info->fb[0]->fci.displayfifo != NULL)
                kfree(dev_info->fb[0]->fci.displayfifo);
            dev_info->fb[0]->fci.displayfifo = NULL;
            kfree(dev_info->fb[0]);
            dev_info->fb[0] = NULL;
        }

        if (dev_info->fb[1]) {
            dev_info->fb[1]->vs.enable = 0;
		    pip_update_vs(dev_info->fb[1]);

            if (dev_info->fb[1]->fci.displayfifo != NULL)
                kfree(dev_info->fb[1]->fci.displayfifo);
            dev_info->fb[1]->fci.displayfifo = NULL;
            kfree(dev_info->fb[1]);
            dev_info->fb[1] = NULL;
        }
        if (dev_info->fb[2]) {
            dev_info->fb[2]->vs.enable = 0;
		    pip_update_vs(dev_info->fb[2]);

            if (dev_info->fb[2]->fci.displayfifo != NULL)
                kfree(dev_info->fb[2]->fci.displayfifo);
            dev_info->fb[2]->fci.displayfifo = NULL;
            kfree(dev_info->fb[2]);
            dev_info->fb[2] = NULL;
        }
        platform_set_drvdata(pdev, NULL);
    }

    DBGLEAVE(1);
    return 0;
}

#include "ccir656_info.c"
#include "lcd_info.c"

/* Assign input reolution information
 * return: VIN_NONE if fail. Or input resolution type if success.
 */
static VIN_RES_T pip_setting_input_resinfo(struct ffb_dev_info *fdev_info, ushort * input_res_p)
{
    struct lcd200_dev_info  *dev_info = (struct lcd200_dev_info *)fdev_info;
    VIN_RES_T res_idx, resolution = VIN_NONE;
    int i;

    resolution = *input_res_p;

    if (resolution == VIN_NONE) {
        switch (fdev_info->video_output_type) {
        case VOT_640x480:
            resolution = VIN_640x480;
            break;
        case VOT_800x600:
            resolution = VIN_800x600;
            break;
        case VOT_1280x800:
            resolution = VIN_1280x800;
            break;
        case VOT_1280x960:
            resolution = VIN_1280x960;
            break;
        case VOT_1280x1024_30I:
        case VOT_1280x1024_25I:
        case VOT_1280x1024:
        case VOT_1280x1024P:
            resolution = VIN_1280x1024;
            break;
        case VOT_1360x768:
            resolution = VIN_1360x768;
            break;
        case VOT_1600x1200:
            resolution = VIN_1600x1200;
            break;
        case VOT_1440x900:
        case VOT_1440x900P:
            resolution = VIN_1440x900;
            break;
        case VOT_1680x1050:
        case VOT_1680x1050P:
            resolution = VIN_1680x1050;
            break;
        case VOT_PAL:
        case VOT_576P:
            resolution = VIN_PAL;
            break;
        case VOT_NTSC:
        case VOT_480P:
            resolution = VIN_D1;
            break;
        case VOT_720P:
        case VOT_1280x720:
            resolution = VIN_720P;
            break;
        case VOT_1440x1080I:
            resolution = VIN_1440x1080;
            break;
        case VOT_1920x1080:
        case VOT_1920x1080I:
        case VOT_1920x1080P:
            resolution = VIN_1920x1080;
            break;
        case VOT_1024x768:
        case VOT_1024x768_30I:
        case VOT_1024x768_25I:
        case VOT_1024x768P:
            resolution = VIN_1024x768;
            break;
        case VOT_1440x960_30I:
            resolution = VIN_1440x960;
            break;
        case VOT_1440x1152_25I:
            resolution = VIN_1440x1152;
            break;
        default:
            for (;;)
                printk("%s, unknown type: %d \n", __FUNCTION__, output_type);
            break;
        }
    } else {
        /* check special case: Input is NSTC/PAL, output_type=PAL/NTSC. For this D1 case, both
         * input/output should be consistent.
         */
        if (resolution == VIN_NTSC) {
            switch (fdev_info->video_output_type) {
            case VOT_PAL:
            case VOT_576P:
                resolution = VIN_PAL;
                break;
            default:
                break;
            }
        } else if (resolution == VIN_PAL) {
            switch (fdev_info->video_output_type) {
            case VOT_NTSC:
            case VOT_480P:
                resolution = VIN_NTSC;
                break;
            default:
                break;
            }
        }
    }

    /* Find the corresponding input_res entry and check if the input resolution is supported as well.
     */
    fdev_info->input_res = NULL;
    for (res_idx = VIN_D1; res_idx < ARRAY_SIZE(ffb_input_res); res_idx++) {
        if (ffb_input_res[res_idx].input_res == resolution) {
            if (platform_check_input_res(dev_info->lcd200_idx, resolution) < 0) {
                printk("LCD%d: Input resolution %s is not configured! \n",
                       dev_info->lcd200_idx, ffb_input_res[res_idx].name);
                return VIN_NONE;
            } else
                printk("LCD%d: Input resolution is %s \n", dev_info->lcd200_idx,
                    ffb_input_res[res_idx].name);

            fdev_info->input_res = &ffb_input_res[res_idx];
            break;
        }
    }

    /* Can't find a match input_res from DATABASE.
     */
    if (fdev_info->input_res == NULL) {
        printk("The input resolution %d is not supported! \n", resolution);
        return VIN_NONE;
    }

    /* check if the input resolution is valid or not */
    for (i = 0; i < FB_NUM; i ++) {
        if (!dev_info->fb[i])
            continue;

        if (!dev_info->fb[i]->vs.enable)
            continue;

        if ((dev_info->fb[i]->vs.width < fdev_info->input_res->xres) ||
            (dev_info->fb[i]->vs.height < fdev_info->input_res->yres)) {

            printk("LCD Error! input resolution(%d,%d) of fb%d is not valid for virtual screen(%d,%d)! \n",
                fdev_info->input_res->xres, fdev_info->input_res->yres, i, dev_info->fb[i]->vs.width,
                dev_info->fb[i]->vs.height);

            return VIN_NONE;
        }
    }

    /* assign timing for input
     */
    if (fdev_info->video_output_type >= VOT_CCIR656) {
        /* CCIR656
         */
        for (i = 0; i < fdev_info->output_info->timing_num; i++) {
            fdev_info->output_info->timing_array[i].data.ccir656.in.xres =
                fdev_info->input_res->xres;
            fdev_info->output_info->timing_array[i].data.ccir656.in.yres =
                fdev_info->input_res->yres;

            if (fdev_info->output_info->timing_array[i].otype == VOT_PAL)
	        {
	            //D1 Case
	            if ((fdev_info->output_info->timing_array[i].data.ccir656.in.yres == 480) &&
#ifdef D1_WIDTH_704
	                (fdev_info->output_info->timing_array[i].data.ccir656.in.xres == 704))
#else
                    (fdev_info->output_info->timing_array[i].data.ccir656.in.xres == 720))
#endif
	                fdev_info->output_info->timing_array[i].data.ccir656.in.yres = 576;
	        }
        }
    } else {
        /* LCD
         */
        for (i = 0; i < fdev_info->output_info->timing_num; i++) {
            fdev_info->output_info->timing_array[i].data.lcd.in.xres = fdev_info->input_res->xres;
            fdev_info->output_info->timing_array[i].data.lcd.in.yres = fdev_info->input_res->yres;
        }
    }

    *input_res_p = (u_short) resolution;

    return resolution;
}

/**
 *@brief Set output information by output_type
 *
 *@param type: who is the output target
 */
static int pip_setting_output_info(struct pip_dev_info *info, ushort type)
{
    struct ffb_dev_info *ffb_dev_info = (struct ffb_dev_info *)info;
    codec_driver_t *driver = &info->codec_device.driver;
    codec_device_t *device = &info->codec_device.device;
    codec_setup_t *codec_device = &info->codec_device;
    int i, b_pesudo_driver = 0;

    memset(&info->codec_device, 0, sizeof(info->codec_device));

    device->output_type = type;
    device->tunnel_data = &info->lcd200_info.dev_info;

    switch (type) {
    case OUTPUT_TYPE_MDIN340_720x480_58:
        ffb_dev_info->output_info = &stdtv_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(MDIN340): input resolution is 720x480i BT656\n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_MDIN340_720x576_59:
        ffb_dev_info->output_info = &stdtv_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(MDIN340): input resolution is 720x576i BT656\n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_MDIN340_480P_60:
        ffb_dev_info->output_info = &stdtv_info_p;
	    ffb_dev_info->output_info->setting_ext_dev = NULL;
	    printk("LCD%d(MDIN340): Output resolution is BT1120 480P. \n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_MDIN340_576P_61:
        ffb_dev_info->output_info = &stdtv_info_p;
	    ffb_dev_info->output_info->setting_ext_dev = NULL;
	    printk("LCD%d(MDIN340): Output resolution is BT1120 576P. \n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_MDIN340_1920x1080P_62:
        ffb_dev_info->output_info = &hdtv_1920x1080p_info;
	    ffb_dev_info->output_info->setting_ext_dev = NULL;
	    printk("LCD%d(MDIN340): Output resolution is BT1120 1920x1080P. \n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_MDIN340_1920x1080_63:
        ffb_dev_info->output_info = &hdtv_1920x1080i_info;
	    ffb_dev_info->output_info->setting_ext_dev = NULL;
	    printk("LCD%d(MDIN340): Output resolution is BT1120 1920x1080i. \n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_SLI10121_1280x720_53:
        ffb_dev_info->output_info = &VGA_1280x720_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(SLI10121): Output resolution is 1280x720/60HZ.\n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_SLI10121_1440x900P_52:
        ffb_dev_info->output_info = &hdtv_1440x900p_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(SLI10121): Output resolution is BT1120 1440x900/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_SLI10121_1920x1080_51:
        ffb_dev_info->output_info = &VGA_1920x1080_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(SLI10121): Output resolution is 1920x1080/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_SLI10121_1280x1024_50:
        ffb_dev_info->output_info = &VGA_1280x1024_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(SLI10121): Output resolution is 1280x1024/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_SLI10121_1680x1050P_49:
        ffb_dev_info->output_info = &hdtv_1680x1050p_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(SLI10121): Output resolution is BT1120 1680x1050/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_SLI10121_1280x1024P_48:
        ffb_dev_info->output_info = &hdtv_1280x1024p_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(SLI10121): Output resolution is BT1120 1280x1024/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_SLI10121_1024x768P_47:
        ffb_dev_info->output_info = &hdtv_1024x768p_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(SLI10121): Output resolution is BT1120 1024x768/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_SLI10121_1024x768_46:
        ffb_dev_info->output_info = &VGA_1024x768_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(SLI10121): Output resolution is 1024x768/60HZ. \n",
            info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_SLI10121_1080P_45:        //FullHD for HDMI, 1920x1080P
        ffb_dev_info->output_info = &hdtv_1920x1080p_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(SLI10121): Output resolution is BT1120 1920x1080P. \n",
            info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_SLI10121_720P_44: //1280x720P
        ffb_dev_info->output_info = &hdtv_1280x720p_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(SLI10121): Output resolution is BT1120 720P. \n",
            info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_SLI10121_576P_43: //576P
        ffb_dev_info->output_info = &stdtv_info_p;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(SLI10121): Output resolution is BT1120 576P. \n",
            info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_SLI10121_480P_42: //480P
        ffb_dev_info->output_info = &stdtv_info_p;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(SLI10121): Output resolution is BT1120 480P. \n",
            info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAS_1440x1152I_41:
        ffb_dev_info->output_info = &sdtv_1440x1152x25_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register device */
        strcpy(driver->name, "1440x1152x25-Cascade");
        b_pesudo_driver = 1;
        printk("LCD%d(Cascade): 1440x1152x25.\n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAS_1440x960I_40:
        ffb_dev_info->output_info = &sdtv_1440x960x30_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register device */
        strcpy(driver->name, "1440x960x30-Cascade");
        b_pesudo_driver = 1;
        printk("LCD%d(Cascade): 1440x960x30.\n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_MDIN240_1024x768I_38:
        ffb_dev_info->output_info = &hdtv_1024x768x30_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d: Output resolution is 1024x768_60I to MDIN240. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_MDIN240_SD_36:
        ffb_dev_info->output_info = &stdtv_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d: Output resolution is D1/30F to MDIN240. \n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_MDIN240_HD_35:
        ffb_dev_info->output_info = &hdtv_1440x1080_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d: Output resolution is 1440x1080/30F to MDIN240. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAS_1920x1080x25_34:   //8210 cascade
        ffb_dev_info->output_info = &hdtv_1920x1080px25_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d: Output resolution is 1920x1080x25 BT1120 cascade. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAS_1920x1080x30_33:   //8210 cascade
        ffb_dev_info->output_info = &hdtv_1920x1080px30_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d: Output resolution is 1920x1080x30 BT1120 cascade. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAS_1280x1024x30_32:
        ffb_dev_info->output_info = &sdtv_1280x1024x30_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register device */
        strcpy(driver->name, "1280x1024x30-Cascade");
        b_pesudo_driver = 1;
        printk("LCD%d(Cascade): 1280x1024x30.\n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAS_1280x1024x25_31:
        ffb_dev_info->output_info = &sdtv_1280x1024x25_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register device */
        strcpy(driver->name, "1280x1024x25-Cascade");
        b_pesudo_driver = 1;
        printk("LCD%d(Cascade): 1280x1024x25.\n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAT6611_1440x900P_30:
        ffb_dev_info->output_info = &hdtv_1440x900p_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(CAT6611): Output resolution is BT1120 1440x900/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAT6611_1680x1050P_27:
        ffb_dev_info->output_info = &hdtv_1680x1050p_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(CAT6611): Output resolution is BT1120 1680x1050/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAT6611_1280x1024_28:
        ffb_dev_info->output_info = &VGA_1280x1024_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(CAT6611): Output resolution is 1280x1024/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAT6611_1920x1080_29:
        ffb_dev_info->output_info = &VGA_1920x1080_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(CAT6611): Output resolution is 1920x1080/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAT6611_1280x1024P_26:
        ffb_dev_info->output_info = &hdtv_1280x1024p_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(CAT6611): Output resolution is BT1120 1280x1024/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAT6611_1024x768P_25:
        ffb_dev_info->output_info = &hdtv_1024x768p_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(CAT6611): Output resolution is BT1120 1024x768/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAT6611_1024x768_24:
        ffb_dev_info->output_info = &VGA_1024x768_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(CAT6611): Output resolution is 1024x768/60HZ. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAT6611_1080P_23: //FullHD for HDMI, 1920x1080P
        ffb_dev_info->output_info = &hdtv_1920x1080p_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(CAT6611): Output resolution is BT1120 1920x1080P. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAT6611_720P_22:  //1280x720P
        ffb_dev_info->output_info = &hdtv_1280x720p_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(CAT6611): Output resolution is BT1120 720P. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAT6611_480P_20:  //480P
        ffb_dev_info->output_info = &stdtv_info_p;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(CAT6611): Output resolution is BT1120 480P. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAT6611_576P_21:  //576P
        ffb_dev_info->output_info = &stdtv_info_p;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        printk("LCD%d(CAT6611): Output resolution is BT1120 576P. \n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_CAS_1024x768x30_19:       /* cascade */
        ffb_dev_info->output_info = &sdtv_1024x768x30_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register device */
        strcpy(driver->name, "1024x768x30-Cascade");
        b_pesudo_driver = 1;
        printk("LCD%d(Cascade): 1024x768x30.\n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_VGA_1280x720_18:
        ffb_dev_info->output_info = &VGA_1280x720_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register driver */
        driver->codec_id = type;
        strcpy(driver->name, "1280x720/60HZ");
        b_pesudo_driver = 1;
        printk("LCD%d: Output resolution is 1280x720/60HZ.\n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_VGA_1680x1050_17:
        ffb_dev_info->output_info = &VGA_1680x1050_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register driver */
        driver->codec_id = type;
        strcpy(driver->name, "1680x1050/60HZ");
        b_pesudo_driver = 1;
        printk("LCD%d: Output resolution is 1680x1050/60HZ(WSXGA+).\n",
               info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_VGA_1600x1200_16: /* 1600x1200 */
        ffb_dev_info->output_info = &VGA_1600x1200_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register driver */
        driver->codec_id = type;
        strcpy(driver->name, "1600x1200/60HZ");
        b_pesudo_driver = 1;
        printk("LCD%d: Output resolution is 1600x1200/60HZ(UXGA).\n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_VGA_800x600_15:   /* SVGA */
        ffb_dev_info->output_info = &VGA_800x600_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register driver */
        driver->codec_id = type;
        strcpy(driver->name, "800x600/60HZ");
        b_pesudo_driver = 1;
        printk("LCD%d: Output resolution is 800x600/60HZ.\n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_VGA_1360x768_14:  //1360x768
        ffb_dev_info->output_info = &VGA_1360x768_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register driver */
        driver->codec_id = type;
        strcpy(driver->name, "1360x768/60HZ");
        b_pesudo_driver = 1;
        printk("LCD%d: Output resolution is 1360x768/60HZ. \n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_VGA_1280x1024_13: //1280x1024
        ffb_dev_info->output_info = &VGA_1280x1024_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register driver */
        driver->codec_id = type;
        strcpy(driver->name, "1280x1024/60HZ");
        b_pesudo_driver = 1;
        printk("LCD%d: Output resolution is 1280x1024/60HZ. \n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_VGA_1440x900_12:
        ffb_dev_info->output_info = &VGA_1440x900_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register driver */
        driver->codec_id = type;
        strcpy(driver->name, "1440x900/60HZ");
        b_pesudo_driver = 1;
        printk("LCD%d: Output resolution is 1440x900/60HZ. \n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_VGA_1920x1080_11:
        ffb_dev_info->output_info = &VGA_1920x1080_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register driver */
        driver->codec_id = type;
        strcpy(driver->name, "1920x1080/60HZ");
        b_pesudo_driver = 1;
        printk("LCD%d: Output resolution is 1920x1080/60HZ. \n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_VGA_1280x960_10:  /* 1280x960 VGA DAC */
        ffb_dev_info->output_info = &VGA_1280x960_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register driver */
        driver->codec_id = type;
        strcpy(driver->name, "1280x960/60HZ");
        b_pesudo_driver = 1;
        printk("LCD%d: Output resolution is 1280x960/60HZ. \n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_VGA_1280x800_9:   /* 1280x800 VGA DAC */
        ffb_dev_info->output_info = &VGA_1280x800_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register driver */
        driver->codec_id = type;
        strcpy(driver->name, "1280x800/60HZ");
        b_pesudo_driver = 1;
        printk("LCD%d: Output resolution is 1280x800/60HZ. \n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_VGA_1024x768_8:   /* 1024x768 VGA DAC */
        ffb_dev_info->output_info = &VGA_1024x768_info;
        ffb_dev_info->output_info->setting_ext_dev = NULL;
        /* register driver */
        driver->codec_id = type;
        strcpy(driver->name, "1024x768/60HZ");
        b_pesudo_driver = 1;
        printk("LCD%d: Output resolution is 1024x768/60HZ. \n", info->lcd200_info.lcd200_idx);
        break;
    case OUTPUT_TYPE_PAL_1:
    case OUTPUT_TYPE_NTSC_0:
        ffb_dev_info->output_info = &stdtv_info;
        /* register driver */
        driver->codec_id = type;
        strcpy(driver->name, "TVE100");
        driver->setting = NULL;
        b_pesudo_driver = 1;
        if (type == OUTPUT_TYPE_PAL_1)
            printk("LCD%d: Output resolution is D1 PAL. \n", info->lcd200_info.lcd200_idx);
        else
            printk("LCD%d: Output resolution is D1 NTSC. \n", info->lcd200_info.lcd200_idx);
        break;
    default:
        printk("LCD%d: Not support %d! \n", info->lcd200_info.lcd200_idx, type);
        return -1;
    }

    ffb_dev_info->support_otypes = 0;
    /* CCIR656 device(PAL, NTSC...) or LCD device */
    if ((type == OUTPUT_TYPE_PAL_1) || (type == OUTPUT_TYPE_CAT6611_576P_21)
        || (type == OUTPUT_TYPE_SLI10121_576P_43) || (type == OUTPUT_TYPE_MDIN340_576P_61)
        || (type == OUTPUT_TYPE_MDIN340_720x576_59))
        ffb_dev_info->video_output_type = ffb_dev_info->output_info->timing_array[1].otype;
    else
        ffb_dev_info->video_output_type = ffb_dev_info->output_info->timing_array[0].otype;

    /* collect all support output types, ex: NTSC, PAL ... */
    for (i = 0; i < ffb_dev_info->output_info->timing_num; i++) {
        ffb_dev_info->support_otypes |=
            FFB_SUPORT_CFG(ffb_dev_info->output_info->timing_array[i].otype);
    }

    /* new a device
     */
    codec_pip_new_device(info->lcd200_info.lcd200_idx, codec_device);

    /* because we don't have real codec driver, so we prepare a pesudo driver
     */
    if (b_pesudo_driver)
        codec_driver_register(driver);

    return 0;
}

static int pip_config_win(struct lcd200_dev_info *dev_info)
{
    struct ffb_rect rect = { 0 };
    rect.width = dev_info->fb[0]->fb.var.xres;
    rect.height = dev_info->fb[0]->fb.var.yres;
    if (dev_info->fb[1]) {
        pip_set_window(&g_dev_info, &rect, 1);
    }
    if (dev_info->fb[2]) {
        pip_set_window(&g_dev_info, &rect, 2);
    }
    return 0;
}

/* This function will be envoked from client such as SLI, CAT6611...
 * edid_array will be add or remove into/from the local database.
 * error_checking: for compilation check only
 * add_remove: 1 for add, 0 for remove
 */
static int pip_update_edid_table(u32 *edid_array, int error_checking, int add_remove)
{
    ffb_output_name_t   *name;
    u32 i, outputtype, candidate = 0;
    output_type_t cand_output_type = 0;

    if (error_checking != OUTPUT_TYPE_LAST) {
        printk("Error! LCD%d, EDID client should be compiled again! \n", LCD200_ID);
        return -1;
    }

    for (i = 0; i < sizeof(g_dev_info.edid_array) / 4; i ++) {
        if (add_remove)
            g_dev_info.edid_array[i] |= edid_array[i];
        else
            g_dev_info.edid_array[i] &= ~edid_array[i];
    }

    if (add_remove == 0)
        return 0;   /* just the client offline */

    /* client is online.
     * 1024x768 has weight 1, 1280x720 has weight 10, 1280x1024 has weight 100, 1920x1080 has weight 1000
     */
    for (outputtype = 0; outputtype < OUTPUT_TYPE_LAST; outputtype ++) {
        if (!test_bit(outputtype, (void *)&g_dev_info.edid_array[0]))
            continue;

        switch (outputtype) {
          case OUTPUT_TYPE_SLI10121_1024x768_46:
            if (candidate < outputtype * 1) {
                candidate = outputtype * 1;
                cand_output_type = outputtype;
            }
            break;
          case OUTPUT_TYPE_SLI10121_1280x720_53:
            if (candidate < outputtype * 10) {
                candidate = outputtype * 10;
                cand_output_type = outputtype;
            }
            break;
          case OUTPUT_TYPE_SLI10121_1280x1024_50:
            if (candidate < outputtype * 100) {
                candidate = outputtype * 100;
                cand_output_type = outputtype;
            }
            break;
          case OUTPUT_TYPE_SLI10121_1920x1080_51:
            if (candidate < outputtype * 1000) {
                candidate = outputtype * 1000;
                cand_output_type = outputtype;
            }
            break;
          default:
            break;
        }
    }

    if (candidate == 0) {
        printk("LCD%d, error! No suitable resolution from EDID! \n", LCD200_ID);
        return -1;
    }

    name = ffb_get_output_name(cand_output_type);

    /* schedule a work to reconfigure the output_type? */
    printk("LCD%d, the best EDID resolution is %s(%d) \n", LCD200_ID, name->name, name->output_type);

    return 0;
}

static int pip_probe(struct platform_device *pdev)
{
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)&g_dev_info;
    struct ffb_dev_info *ffb_dev_info = (struct ffb_dev_info *)&g_dev_info;
    int ret = 0;
    union lcd200_config config;
    struct resource *dev_res;
    u32 compress_ip_base = 0, decompress_ip_base = 0, tmp = 0;
    int irq;

    if (compress_ip_base || decompress_ip_base || tmp)    {}

    DBGENTER(1);
    config.value = 0;

    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        ret = -EINVAL;
        goto err;
    }

    dev_info->lcd200_idx = LCD200_ID;

#if 0
    ret = request_irq(irq, pip_handle_irq, 0, pdev->name, dev_info);
    if (ret < 0) {
        err("request_irq failed: %d\n", ret);
        goto err;
    }
#endif

    platform_set_drvdata(pdev, dev_info);
    dev_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    ffb_dev_info->io_base =
        (u_long) ioremap_nocache(dev_res->start, dev_res->end - dev_res->start + 1);
    if (ffb_dev_info->io_base == 0)
        panic("LCD: no virtual address \n");
    else
        printk("LCD%d: PA = %#x, VA = %#x, size:%#x bytes \n", dev_info->lcd200_idx, dev_res->start,
                (u32)ffb_dev_info->io_base, dev_res->end - dev_res->start + 1);

    ffb_dev_info->fb0_fb1_share = fb0_fb1_share;

#ifdef GRAPHIC_COMPRESS
    /* graphic encoder */
    compress_ip_base = platform_get_compress_ipbase(dev_info->lcd200_idx);
    ffb_dev_info->compress_io_base = (u_long) ioremap_nocache(compress_ip_base, PAGE_SIZE);
    if (ffb_dev_info->compress_io_base == 0)
        panic("LCD: no virtual address for compressor! \n");

    decompress_ip_base = platform_get_decompress_ipbase(dev_info->lcd200_idx);
    ffb_dev_info->decompress_io_base = (u_long) ioremap_nocache(decompress_ip_base, PAGE_SIZE);
    if (ffb_dev_info->decompress_io_base == 0)
        panic("LCD: no virtual address for decompressor! \n");

    dev_info->support_ge = 1;
    //tmp = (0x1 | (0x1 << 4)); //enable, address, plane-1 */
    //tmp = (0x1 << 4); //address, plane-1 */
    //iowrite32(tmp, ffb_dev_info->decompress_io_base);   //enable is disabled
#endif /* GRAPHIC_COMPRESS */

    ffb_dev_info->dev = &pdev->dev;

    platform_set_lcd_vbase(dev_info->lcd200_idx, (unsigned int)ffb_dev_info->io_base);
    platform_set_lcd_pbase(dev_info->lcd200_idx, dev_res->start);

    /* step1: collect ffb device information based on output_type */
    if (pip_setting_output_info(&g_dev_info, output_type) < 0) {
        ret = -EFAULT;
        goto err;
    }

    /* step2: get input resolution information */
    if ((ret = pip_setting_input_resinfo(ffb_dev_info, &input_res)) == VIN_NONE) {
        ret = -EFAULT;
        goto err;
    }

    /*
     * starts to contruct fb
     */
    dev_info->fb[0] = kzalloc(sizeof(struct ffb_info), GFP_KERNEL);
    if (!dev_info->fb[0]) {
        err("Alloc  ffb_info failed for fb0");
        ret = -ENOMEM;
        goto err;
    }

    dev_info->fb[0]->index = 0;
    dev_info->fb[0]->fb_num = fb0_num;
    dev_info->fb[0]->video_input_mode = fb0_def_mode;
    dev_info->fb[0]->support_imodes = FB0_SUP_IN_FORMAT;
    dev_info->fb[0]->vs.width = vs_width;
    dev_info->fb[0]->vs.height = vs_height;

#ifdef FB0_USE_RAMMAP
    dev_info->fb[0]->use_rammap = 1;
#endif

    if (fb1_num) {
        dev_info->fb[1] = kzalloc(sizeof(struct ffb_info), GFP_KERNEL);
        if (!dev_info->fb[1]) {
            err("Alloc  ffb_info failed for fb1");
            ret = -ENOMEM;
            goto err;
        }
        dev_info->fb[1]->index = 1;
        dev_info->fb[1]->fb_num = fb1_num;
        dev_info->fb[1]->video_input_mode = fb1_def_mode;
        dev_info->fb[1]->support_imodes = FB1_SUP_IN_FORMAT;
        dev_info->fb[1]->vs.width = vs_width;
        dev_info->fb[1]->vs.height = vs_height;
        dev_info->fb[1]->fb_pan_display = fb1_pan_display;

#ifdef FB1_USE_RAMMAP
        dev_info->fb[1]->use_rammap = 1;
#endif
    }
    if (fb2_num) {
        dev_info->fb[2] = kzalloc(sizeof(struct ffb_info), GFP_KERNEL);
        if (!dev_info->fb[2]) {
            err("Alloc  ffb_info failed for fb2");
            ret = -ENOMEM;
            goto err;
        }
        dev_info->fb[2]->index = 2;
        dev_info->fb[2]->fb_num = fb2_num;
        dev_info->fb[2]->video_input_mode = fb2_def_mode;
        dev_info->fb[2]->support_imodes = FB2_SUP_IN_FORMAT;
        dev_info->fb[2]->vs.width = vs_width;
        dev_info->fb[2]->vs.height = vs_height;
#ifdef FB2_USE_RAMMAP
        dev_info->fb[2]->use_rammap = 1;
#endif
    }

    memset(&config, 0, sizeof(union lcd200_config));
    config.reg.YUV_En = 1;
    config.reg.PIP_En = g_dev_info.PIP_En = 0;
    config.reg.Blend_En = g_dev_info.Blend_En = 0;
    config.reg.CCIR_En = 0;
    if (ffb_dev_info->video_output_type >= VOT_CCIR656)
        config.reg.CCIR_En = 1;
    config.reg.AXI_En = 1;  //axi bus
    config.reg.Deflicker_En = 1;    //enable de-flicker
    ret = dev_construct(dev_info, config);
    if (ret < 0)
        goto err;

    dev_info->ops->config_input_misc = pip_config_win;

    /* EDID function will be called from client */
    ffb_dev_info->update_edid = pip_update_edid_table;

    pip_config_win(dev_info);

    /* entrance of AP ioctl */
    dev_info->fb[0]->fb.fbops->fb_ioctl = pip_ioctl;
    if (dev_info->fb[1]) {
        dev_info->fb[1]->fb.fbops->fb_ioctl = pip_ioctl;
    }
    if (dev_info->fb[2]) {
        dev_info->fb[2]->fb.fbops->fb_ioctl = pip_ioctl;
    }

    pip_switch_pip_mode(&g_dev_info, g_dev_info.PIP_En);

    /* default value */
    dev_info->ops->reg_write((struct ffb_dev_info *)ffb_dev_info, PANEL_PARAM, 0x400, 0x600);

    /* default image priority. Video is low priority. PIP1 is highest priority. PIP2 is middle
     */
    if (dev_info->fb[2]) {
        tmp = (0x2 << 2) | (0x1 << 4) | (0x3 << 6);
        dev_info->ops->reg_write((struct ffb_dev_info *)ffb_dev_info, PIP_IMG_PRI, tmp, 0xFF);

        /* PiPBlend_l to be 255 */
        dev_info->ops->reg_write((struct ffb_dev_info *)ffb_dev_info, PIP_BLEND, 255, 0xFF);

        /* set palette */
        dev_info->fb[2]->ops->setcolreg(0, 0, 0, 0, 0, &dev_info->fb[2]->fb);
        dev_info->fb[2]->ops->setcolreg(1, 0x1F, 0x3F, 0x1F, 0, &dev_info->fb[2]->fb); //gray color
        /* set color key to fill out 0 */
        dev_info->ops->reg_write((struct ffb_dev_info *)ffb_dev_info, PIP_COLOR_KEY2, (0x1 << 24), 0xFFFFFFFF);
    }

    if (pip_proc_init() < 0) {
        ret = -EFAULT;
        goto err;
    }

    dev_info->ops->set_par(ffb_dev_info);

    ret = fsosd_construct(dev_info);
    if (ret < 0) {
        goto err;
    }

#ifdef VIDEOGRAPH_INC
    if (lcdvg_driver_init() < 0)
        err("failed to register driver to VideoGraph!\n");
#endif

#if 1
    ret = request_irq(irq, pip_handle_irq, 0, pdev->name, dev_info);
    if (ret < 0) {
        panic("request_irq failed: %d\n", ret);
    }
#endif

    DBGLEAVE(1);

    probe_ret = ret;
    return ret;

  err:
    pip_remove(pdev);
    DBGLEAVE(1);

    /* in driver_probe_dev(), the ret value will be cleared to zero for next probe, thus we use the
     * global variable to keep the return value
     */
    probe_ret = ret;
    return ret;
}

#ifdef CONFIG_PM
/**
 *@brief suspend and resume support for the lcd controller
 */
int pip_suspend(struct platform_device *pdev, pm_message_t state, u32 level)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)platform_get_drvdata(pdev);
    if (info) {
        info->ops->suspend(info, state, level);
    }
    return 0;
}

int pip_resume(struct device *dev, u32 level)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)platform_get_drvdata(pdev);
    if (info) {
        info->ops->resume(info, level);
    }
    return 0;
}
#endif

static struct platform_driver pip_driver = {
    .probe = pip_probe,
    .remove = __devexit_p(pip_remove),
    .driver = {
#ifdef LCD_DEV2
            .name = "flcd200_2",
#elif defined(LCD_DEV1)
            .name = "flcd200_1",
#else
            .name = "flcd200",
#endif
	        .owner = THIS_MODULE,
	    },
#ifdef CONFIG_PM
    .suspend = pip_suspend,
    .resume = pip_resume,
#endif
};

int __init pip_init(void)
{
    int ret = 0;

    DBGENTER(1);
    if (fb3_num) {
    }

    memset(&g_dev_info, 0, sizeof(g_dev_info));

    platform_pmu_clock(LCD200_ID, 1, 0);        //turn on LCD clock

    if (!fb0_num || fb0_num > MAX_FRAME_NO) {
        err("fb0_num is bigger than %d. or equal to 0", MAX_FRAME_NO);
        ret = -EFAULT;
        goto exit;
    }

    if (fb1_num > MAX_FRAME_NO) {
        err("fb1_num is bigger than %d.", MAX_FRAME_NO);
        ret = -EFAULT;
        goto exit;
    }

    if (!fb1_num && fb2_num) {
        err("Does not support fb2 is on and fb1 is off");
        ret = -EFAULT;
        goto exit;
    }

    if (fb2_num > MAX_FRAME_NO) {
        err("fb2_num is bigger than %d.", MAX_FRAME_NO);
        ret = -EFAULT;
        goto exit;
    }

    if (lcd200_init() < 0) {
        err("error in lcd200_init %d", LCD200_ID);
        ret = -EFAULT;
        goto exit;
    }

    if (vs_width || vs_height) {
        printk("LCD%d: Virutal screen width:%d, height:%d \n", LCD200_ID, vs_width, vs_height);
    }

    /* Register the driver with LDM */
    if ((platform_driver_register(&pip_driver) < 0) || probe_ret) {
        panic("failed to register driver for PIP\n");
        ret = -ENODEV;
        goto exit;
    }

    TVE100_BASE =
        (unsigned int)ioremap_nocache((unsigned long)TVE_FTTVE100_PA_BASE,
                                      (unsigned long)TVE_FTTVE100_PA_SIZE);
    if (!TVE100_BASE)
        panic("LCD: No virtual addr for TVE100!");

  exit:
    if (ret < 0) {
        platform_pmu_clock(LCD200_ID, 0, 0);
    }

    DBGLEAVE(1);
    if (!ret)
        printk("LCD%d: Driver[Ver: %s] init ok. fb0_fb1_share:%d, fb1_pan_display:%d\n", LCD200_ID,
                                LCD_VERION, fb0_fb1_share, fb1_pan_display);

    if (lcd0_thread || lcd2_thread) {}

#ifdef CONFIG_PLATFORM_GM8210
    if (1) {
        fmem_pci_id_t   pci_id;
        fmem_cpu_id_t   cpu_id;

        fmem_get_identifier(&pci_id, &cpu_id);

        if (pci_id == FMEM_PCI_HOST) {
            /* create a thread for recive the request from FA726 */
            if (LCD200_ID == LCD_ID) {
                lcd0_thread = kthread_create(lcd_polling_thread, 0, "lcd0_poll");
                if (IS_ERR(lcd0_thread)) {
                    printk("LCD0: Error in creating kernel thread! \n");
                    lcd0_thread = NULL;
                } else {
                    wake_up_process(lcd0_thread);
                }
            }

            if (LCD200_ID == LCD2_ID) {
                lcd2_thread = kthread_create(lcd_polling_thread, (void *)2, "lcd2_poll");
                if (IS_ERR(lcd2_thread)) {
                    printk("LCD2: Error in creating kernel thread! \n");
                    lcd2_thread = NULL;
                } else {
                    wake_up_process(lcd2_thread);
                }
            }
        } /* PCIX_HOST */
    } /* if 1 */
#endif

    return ret;
}

void pip_reset_registers(void)
{
    struct ffb_dev_info *fdev_info = (struct ffb_dev_info *)&g_dev_info;
    u32 tmp;

    tmp = ioread32(fdev_info->io_base + FEATURE);
    if (tmp & 0x1) {
        //disable LCD controller first
        LCD200_Ctrl(fdev_info, 0);      //disable LCD
    }

    tmp &= ~(0x1 << 14);        //disable pattern generation

    //FIXME: disable workaround for underrun
    iowrite32(0, fdev_info->io_base + 0x50);

    iowrite32(tmp, fdev_info->io_base + FEATURE);
}

static void __exit pip_cleanup(void)
{
    DBGENTER(1);

    codec_driver_deregister(&g_dev_info.codec_device.driver);
    codec_pip_remove_device(g_dev_info.lcd200_info.lcd200_idx, &g_dev_info.codec_device);

    pip_reset_registers();

    platform_driver_unregister(&pip_driver);
    lcd200_cleanup();

    platform_pmu_clock(LCD200_ID, 0, 0);

    /*
     * Call lcdvg cleanup function
     */
    if (lcdvg_cb_fn && lcdvg_cb_fn->driver_clearnup)
        lcdvg_cb_fn->driver_clearnup();

    if (g_dev_info.lcd200_info.dev_info.io_base)
        __iounmap((void *)g_dev_info.lcd200_info.dev_info.io_base);

    if (TVE100_BASE)
        __iounmap((void *)TVE100_BASE);

#ifdef GRAPHIC_COMPRESS
    /* disable both compressor and decompressor */

    /* free addr */
    if (g_dev_info.lcd200_info.dev_info.compress_io_base)
        __iounmap((void *)g_dev_info.lcd200_info.dev_info.compress_io_base);

    if (g_dev_info.lcd200_info.dev_info.decompress_io_base)
        __iounmap((void *)g_dev_info.lcd200_info.dev_info.decompress_io_base);
#endif
#ifdef CONFIG_PLATFORM_GM8210
    if (lcd0_thread)
        kthread_stop(lcd0_thread);
    if (lcd2_thread)
        kthread_stop(lcd2_thread);

    while (thread_init)
        msleep(1);
#endif

    DBGLEAVE(1);
}

#ifdef GRAPHIC_COMPRESS

enum {
    FIRST_ENCODE = 0,
    FIRST_ENCODE_DONE,
    NEXT_ENCODE,
    ENCODE_DONE
} enstatus = FIRST_ENCODE;

u32 pip_compress_gui(struct ffb_info *fbi)
{
    struct ffb_dev_info     *fdev_info = (struct ffb_dev_info *)fbi->dev_info;
    unsigned int read_addr, read_size, write_addr, write_limit, byte_per_pixel;
    unsigned int compress_io_base, decompress_io_base;
    unsigned int tmp, encode_idx;
    static int  en_period = 0, last_byte_per_pixel = 0;
    static int dec_toggle_idx = 0;  //decode buffer idx and also playing idx

    if (fbi->ppfb_cpu[0] == NULL)   //gui is disabled.
        return -1;   //do nothing

    if (fbi->index != 1)
        return -1;   //do nothing

    if (en_period) {
        en_period --;
        return -1;  //do nothing
    }

    compress_io_base = fdev_info->compress_io_base;  //virtual address
    /* check encode done */
    tmp = ioread32(compress_io_base + 0x1C);
    if (tmp & 0x2) {
        panic("LCD%d, OH, NO! compress buffer ratio is 0.%d(too small!!!) \n", LCD200_ID,
                                                                        WRITE_LIMIT_RATIO);
        return -1;
    }

    if ((enstatus == FIRST_ENCODE_DONE) || (enstatus == ENCODE_DONE)) {
        if (!(tmp & 0x1))
            return -1; //do nothing
    }

    /*
     * dec_toggle_idx: current index which is playing.
     */
    byte_per_pixel = 2;
    if ((fbi->video_input_mode == VIM_ARGB) || (fbi->video_input_mode == VIM_RGB888))
        byte_per_pixel = 4;

    encode_idx = (dec_toggle_idx + 1) & 0x1;
    read_addr = (unsigned int)fbi->ppfb_dma[0];
    read_size = fdev_info->input_res->xres * fdev_info->input_res->yres * byte_per_pixel;
    write_addr = (unsigned int)fbi->gui_dma[encode_idx];
    write_limit = (read_size * WRITE_LIMIT_RATIO) / 100;

    /* trigger the compress
     */
    if ((enstatus == FIRST_ENCODE) || (enstatus == NEXT_ENCODE)) {
        //[0] : set 1 to fire, auto clear
        //[10:8] : index width (0:3-bit, 1:4-bit, 2:5-bit, 3:6-bit, 4:7-bit, 5~7:invalid)
        //[12:11]: source pixel data size (0:1-byte, 1:2-byte, 2:4-byte, 3:4-byte)
        //[31] : software reset, set 1 then set 0

        iowrite32(read_addr, compress_io_base + 0x4);
        iowrite32(read_size, compress_io_base + 0x8);
        iowrite32(write_addr, compress_io_base + 0xC);
        iowrite32(write_limit, compress_io_base + 0x10);

        tmp = (0x4 << 8); //index width is 7 bit;
        if (byte_per_pixel == 4)
            tmp |= (0x2 << 11); //data size is 4 bytes
        else
            tmp |= (0x1 << 11); //data size is 2 bytes
        tmp |= 0x1; /* fire */

        iowrite32(tmp, compress_io_base);
    }

    /* also config decompress
     */
// [0]     Enable (0:off, 1:on)
// [1]     Sync mode (0:by address, 1:by vsync)
// [2]     bypass buslock command for LCD (0:off, 1:on)
// [3]     DMA preload (0:off, 1:on)
// [7:4]   AXI ID of decoding plane
// [10:8]  RLC Index length (0:3bits, 1:4bits, 2:5bits, 3:6bits, 4:7bits)
// [12:11] RLC Code length (0:1byte, 1:2bytes, 2:4bytes, 3:4bytes)
// [16] Sync edge (0:posedge, 1:negedge)

    if (last_byte_per_pixel != byte_per_pixel) {
        last_byte_per_pixel = byte_per_pixel;   //update to new

        decompress_io_base = fdev_info->decompress_io_base;
        tmp = ioread32(decompress_io_base);
        tmp &= ~((0x7 << 8) | (0x3 << 11));
        tmp |= (0x4 << 8); //index width is 7 bit;
        if (byte_per_pixel == 4)
            tmp |= (0x2 << 11); //data size is 4 bytes
        else
            tmp |= (0x1 << 11); //data size is 2 bytes
        iowrite32(tmp, decompress_io_base);
    }

    //first time to use deocoder.
    if (enstatus == FIRST_ENCODE_DONE) {
        u32 tmp1 = ioread32(fdev_info->io_base), tmp2 = ioread32(fdev_info->decompress_io_base);
        //disalbe lcd
        tmp1 &= ~0x1;
        iowrite32(tmp1, fdev_info->io_base);
        //set plane id to decoder
        iowrite32(tmp2 | (0x1 << 4) | 0x1, fdev_info->decompress_io_base);   //plane-id 1
        //enable lcd
        iowrite32(tmp1 | 0x1, fdev_info->io_base);
    }

    /* update the status for the next coming interrupt */
    switch (enstatus) {
      case FIRST_ENCODE:
        enstatus = FIRST_ENCODE_DONE;
        break;
      case FIRST_ENCODE_DONE:
        dec_toggle_idx = (dec_toggle_idx + 1) & 0x1;
        if (fdev_info->hw_frame_rate <= 350)
            en_period = 3;  /* 30/3 per second */
        else if (fdev_info->hw_frame_rate <= 650)
            en_period = 6;  /* 60/6 per second */
        else
            en_period = 6;
        enstatus = NEXT_ENCODE;
        break;
      case NEXT_ENCODE:
        enstatus = ENCODE_DONE;
        break;
      case ENCODE_DONE:
        dec_toggle_idx = (dec_toggle_idx + 1) & 0x1;
        if (fdev_info->hw_frame_rate <= 350)
            en_period = 3;  /* 30/3 per second */
        else if (fdev_info->hw_frame_rate <= 650)
            en_period = 6;  /* 60/6 per second */
        else
            en_period = 6;
        enstatus = NEXT_ENCODE;
        break;
    }

    //encode done, now update to new address
    if ((enstatus == FIRST_ENCODE_DONE) || (enstatus == ENCODE_DONE))
        iowrite32(fbi->gui_dma[encode_idx], fdev_info->decompress_io_base + 0x4);

    return fbi->gui_dma[encode_idx];
}
#endif /* GRAPHIC_COMPRESS */

/*
 * Get physical address for display
 */
static unsigned int pip_get_phyaddr(struct lcd200_dev_info *dev_info, int idx)
{
    u32 phy_addr = -1;
    int fbnum;

    if (dev_info == NULL)
        return (u32) - 1;

    /*
     * Process job if it comes from videograph
     */
    if (lcdvg_cb_fn && lcdvg_cb_fn->process_frame) {
        phy_addr = lcdvg_cb_fn->process_frame(dev_info, idx);

        if (phy_addr == (u32) - 1) {
            goto exit;
        } else if (phy_addr > RET_SUCCESS) {
            goto exit;
        } else if (phy_addr == RET_NO_NEW_FRAME) {
            phy_addr = (u32) - 1;
            goto exit;
        } else if (phy_addr == RET_NOT_VGPLANE) {
            /* fall through */
        }
    }

//old_fb_ctl:
    fbnum = dev_info->fb[idx]->ops->switch_buffer(dev_info->fb[idx]);
    if (fbnum >= 0) {
        DBGPRINT(7, "LCD%d: fb[%d], fbnum %d update to hardware \n", dev_info->lcd200_idx, idx,
                 fbnum);
        phy_addr = dev_info->fb[idx]->ppfb_dma[fbnum];

        /* new */
        if (dev_info->fb[idx]->ypan_offset)
            phy_addr = phy_addr + dev_info->fb[idx]->ypan_offset * dev_info->fb[idx]->fb.fix.line_length;

#ifdef GRAPHIC_COMPRESS
        /* lcd2(CVBS doesn't support GE) */
        if ((idx == 1) && (LCD200_ID != LCD2_ID))
            phy_addr = pip_compress_gui(dev_info->fb[idx]);
#endif /* GRAPHIC_COMPRESS */
    }

  exit:
    return phy_addr;
}

/*
 * This function supports dynamically to change the new input resolution or output_type
 * input_res : VIN_RES_T
 * output_type: same as output_type is module parameter
 * return value: 0 for success, -1 for fail
 */
int pip_config_resolution(ushort input_res_l, ushort output_type_l)
{
    struct ffb_dev_info *fdev_info = (struct ffb_dev_info *)&g_dev_info;
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)&g_dev_info;
    int bFound = 0, ret, bChange = 0, not_clean_screen = 1;
    VIN_RES_T res_idx;
    u32 tmp, i;
    u_char video_output_type = fdev_info->video_output_type;

    /*
     * Do sanity check first
     */
    if (input_res_l != VIN_NONE) {
        /* For the first place, check if the input resoultion is valid and supported in this platform */
        for (res_idx = VIN_D1; res_idx < (sizeof(ffb_input_res) / sizeof(ffb_input_res_t));
             res_idx++) {
            if ((VIN_RES_T) input_res_l != ffb_input_res[res_idx].input_res)
                continue;

            if (platform_check_input_res(dev_info->lcd200_idx, (VIN_RES_T) input_res_l) < 0) {
                printk("LCD%d: Input resolution %s is not configured! \n",
                       dev_info->lcd200_idx, ffb_input_res[res_idx].name);
                return -1;
            }
            bFound = 1;
            break;
        }
        if (!bFound) {
            printk("The input resolution %d does not exist! \n", input_res_l);
            return -1;
        }
    }

    /* Do output first
     */
    if (g_dev_info.codec_device.device.output_type != output_type_l) {
        struct pip_dev_info *pip_info_dummy;

        pip_info_dummy = kmalloc(sizeof(struct pip_dev_info), GFP_KERNEL);
        if (pip_info_dummy == NULL) {
            printk("No memory for PIP! \n");
            return -1;
        }

        /* backup the pip info in order to test if the output_type_l is valid? */
        memcpy(pip_info_dummy, &g_dev_info, sizeof(struct pip_dev_info));

        /* test the output type is valid or not */
        if (pip_setting_output_info(pip_info_dummy, output_type_l) < 0) {
            printk("The output_type %d is not supported! \n", output_type_l);
            kfree(pip_info_dummy);
            return -1;
        } else {
            /* Note:
             * When call pip_setting_output_info(), a new driver and device node will be created.
             * So we need to remove them.
             */
            codec_driver_deregister(&pip_info_dummy->codec_device.driver);
            codec_pip_remove_device(pip_info_dummy->lcd200_info.lcd200_idx,
                                    &pip_info_dummy->codec_device);
        }

        /* the output is changed
         */
        if (g_dev_info.codec_device.device.output_type !=
            pip_info_dummy->codec_device.device.output_type) {
            /* check if the input is valid or not?
             * Note: if input_res_l is VIN_NONE, then input_res will be assigned to proper value according to output_type_l.
             */
            if (fdev_info->input_res->input_res != input_res_l) {
                if (pip_setting_input_resinfo((struct ffb_dev_info *)pip_info_dummy, &input_res_l)
                    == VIN_NONE) {
                    kfree(pip_info_dummy);
                    return -1;
                }
            }

            /*
             * From now on, we can assume the following procedure should be success. Otherwise it is bug!
             */

            /* remove old driver if it has */
            codec_driver_deregister(&g_dev_info.codec_device.driver);
            codec_pip_remove_device(g_dev_info.lcd200_info.lcd200_idx, &g_dev_info.codec_device);

            /* hook new encoder driver */
            if (pip_setting_output_info(&g_dev_info, output_type_l) < 0) {
                for (;;)
                    printk("BUG! %s\n", __FUNCTION__);
                kfree(pip_info_dummy);
                return -1;
            }

            /* indicate change */
            bChange = 1;
        }
        kfree(pip_info_dummy);
    }

    /* Second, do input
     */
    /* if the input resolution is changed, we need to process the frame buffer carefully!
     * A bug happen here: the case output change VGA=>compoiste, pip_setting_input_resinfo() will assign resultuion to
     * either ccir656 structure or lcd structure. So pip_setting_input_resinfo() is needed.
     */
    if (1) {                    /* fdev_info->input_res->input_res != input_res) is removed */
        /* If input_res_l is VIN_NONE, then input_res_l will be assigned to proper value according to output_type.
         */
        if (pip_setting_input_resinfo(fdev_info, &input_res_l) == VIN_NONE) {
            return -1;
        }

        /* If it is now in video graph, we must release the job and update the frame base to orginal
         */
        if (fdev_info->input_res->input_res != input_res) {
            printk("Input resolution is changed! \n");

            /* flush all jobs */
            if (lcdvg_cb_fn && lcdvg_cb_fn->flush_all_jobs)
                lcdvg_cb_fn->flush_all_jobs();

            /* indicate change */
            bChange = 1;
            not_clean_screen = 0;       /* input resolution is changed, we need to clear frame buffer */

            /* parking the hardware frame base to black screen */
            for (i = 0; i < 4; i++) {
                if (!dev_info->fb[i])
                    continue;

                if (dev_info->fb[i]->not_clean_screen)
                    iowrite32(dev_info->fb[i]->ppfb_dma[0],
                              fdev_info->io_base + FB0_BASE + i * 0xC);
            }
        } else {
            /* The case, fdev_info->video_output_type is VOT_PAL last time (of course, input resolution is PAL as well). Now output_type is
             * changed but input_res is still D1 (Note: NTSC is the first index in timming table), this will have in.xres and in.yres be assigned
             * to 720x480 in pip_setting_input_resinfo(). So, we need to restore the input resolution to PAL
             */
            if ((video_output_type == VOT_PAL) && fdev_info->input_res->input_res == VIN_D1)
                fdev_info->output_info->timing_array[0].data.ccir656.in.yres = 576;
        }
    }

    if (!bChange)
        return 0;               //nothing is changed

    /* if the input resolution is changed, we need to clear the frame buffer.
     */
    if (not_clean_screen) {
        for (i = 0; i < 4; i++) {
            if (!dev_info->fb[i])
                continue;

            /* if input resolution is not changed, we don't need to clear frame buffer */
            dev_info->fb[i]->not_clean_screen = 1;      //1 for not clear screen
        }
    }

    /* Start to change output timing
     */
    /* Disable LCD first */
    tmp = dev_info->ops->reg_read(fdev_info, FEATURE);
    tmp &= ~(0x1 << 13 | 0x1 << 1);
    if (fdev_info->video_output_type >= VOT_CCIR656)
        tmp |= (0x1 << 13);
    dev_info->ops->reg_write(fdev_info, FEATURE, tmp, (0x1 << 13 | 0x1 << 1));
    LCD200_Ctrl(fdev_info, 0);  //disable LCD

    /* second: set timing
     */
    ret = dev_setting_ffb_output(dev_info);
    if (ret < 0) {
        printk("Change resolution fail! input_res = %d, output_type = %d\n", input_res_l,
               output_type_l);
        return -1;
    }

    if (dev_info->ops->config_input_misc) {
        ret = dev_info->ops->config_input_misc(dev_info);
        if (ret < 0) {
            printk("%s, error in config_input_misc()! \n", __FUNCTION__);
            return -1;
        }

        ret = dev_info->ops->set_par(fdev_info);
        if (ret < 0) {
            printk("%s, error in dev_set_par()! \n", __FUNCTION__);
            return -1;
        }
    }

    LCD200_Ctrl(fdev_info, 1);  //enable LCD
    return 0;
}

/* called by vg layer to hook its callback functions
 */
void pip_register_callback(struct pip_vg_cb *callback)
{
    if (unlikely(!callback))
        return;

    lcdvg_cb_fn = callback;
}

/* called by vg layer to release its callback functions
 */
void pip_deregister_callback(struct pip_vg_cb *callback)
{
    if (unlikely(!callback))
        return;

    if (lcdvg_cb_fn->lcd_idx != callback->lcd_idx) {
        printk("LCD: Error in %s \n", __FUNCTION__);
        return;
    }

    lcdvg_cb_fn = NULL;
}

#ifdef CONFIG_PLATFORM_GM8210
#include <cpu_comm/cpu_comm.h>
#define CPU_COMM_LCD0   CHAN_0_USR6
#define CPU_COMM_LCD2   CHAN_0_USR7

static int lcd_polling_thread(void *private)
{
    u32 target, lcd_idx = (u32)private;
    cpu_comm_msg_t  msg;
    unsigned int    command[40];
    unsigned int    i, val;
    int length;
    char *lcd_str[] = {"lcd0_drv", "lcd1_drv", "lcd2_drv"};

    thread_init = 1;

    target = (lcd_idx == 0) ? CPU_COMM_LCD0 : CPU_COMM_LCD2;
    if (cpu_comm_open(target, lcd_str[lcd_idx]))
        panic("LCD%d: open cpucomm channel fail! \n", target);

    /* send all frame buffer address to the peer */
    memset(&msg, 0, sizeof(msg));
    msg.target = target;
    msg.msg_data = (unsigned char *)&command[0];
    command[0] = 0x3035;
    msg.length = 4;
    for (i = 0;i < FB_NUM; i ++) {
        if (!g_dev_info.lcd200_info.fb[i])
            break;
        command[i + 1] = g_dev_info.lcd200_info.fb[i]->ppfb_dma[0];
        msg.length += 4;
    }
    cpu_comm_msg_snd(&msg, -1);

    do {
        msg.target = (lcd_idx == 0) ? CPU_COMM_LCD0 : CPU_COMM_LCD2;
        msg.length = sizeof(command);
        msg.msg_data = (unsigned char *)&command[0];

        if (cpu_comm_msg_rcv(&msg, 900 /* ms */))
            continue;

        switch (command[0]) {
          case FFB_GET_EDID:
          {
            struct edid_table   edid;
            pip_get_edid(&edid);

            msg.target = (lcd_idx == 0) ? CPU_COMM_LCD0 : CPU_COMM_LCD2;
            msg.length = sizeof(struct edid_table);
            msg.msg_data = (unsigned char *)&edid;
            if (cpu_comm_msg_snd(&msg, -1))
                panic("LCD: Error to send command1! \n");
            break;
          }

          case FFB_SET_EDID:
            length = command[1];
            if (length != 4)
                panic("LCD interal error1! length = %d \n", length);
            val = command[2];
            pip_config_edid(val);
            break;

          case FFB_GUI_CLONE:
            length = command[1];
            if (length != 4)
                panic("LCD interal error2! length = %d \n", length);
            val = command[2];
            lcd_gui_bitmap = val;
            break;

          case FFB_GUI_CLONE_ADJUST:
            length = command[1];
            if (length != sizeof(struct gui_clone_adjust))
                panic("LCD interal error3! length = %d \n", length);
            memcpy(&gui_clone_adjust, &command[2], sizeof(struct gui_clone_adjust));
            break;

          case FFB_PIP_MODE:
          {
            struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;

            length = command[1];
            if (length != 4)
                panic("LCD interal error3! length = %d \n", length);
            val = command[2];
            if (val < 3) {
                pip_switch_pip_mode(&g_dev_info, val);
                info->ops->set_par((struct ffb_dev_info *)info);
            }
            break;
          }
          case FFB_GET_INPUTRES_OUTPUTTYPE:
          {
            u32 value = (output_type << 16 | input_res);

            msg.target = (lcd_idx == 0) ? CPU_COMM_LCD0 : CPU_COMM_LCD2;
            msg.length = sizeof(u32);
            msg.msg_data = (void *)&value;

            if (cpu_comm_msg_snd(&msg, -1))
                panic("LCD: Error to send command2! \n");
            break;
          }

          case FFB_PIP_INPUT_RES:
          {
            u32 val;
            int input_res_tmp, l_output_type;

            length = command[1];
            if (length != 4)
                panic("LCD interal error4! length = %d \n", length);
            val = command[2];
            input_res_tmp = val & 0xFFFF;
            l_output_type = val >> 16;
            if (pip_config_resolution((ushort) input_res_tmp, (ushort) l_output_type) < 0) {
                printk("lcd config input_res:%d(output:%d) Fail! \n", input_res_tmp, l_output_type);
            } else {
                if (input_res != input_res_tmp) {
                    input_res = input_res_tmp;
                    //notify the input resolution
                    if (lcdvg_cb_fn && lcdvg_cb_fn->res_change)
                        lcdvg_cb_fn->res_change();
                }
                if (l_output_type != output_type) {
                    output_type = l_output_type;
                    printk("Warning: lcd output_type is changed to NTSC or PAL! \n");
                }
            }
            break;
          }

          case FFB_PIP_OUTPUT_TYPE:
          {
            u32 val;
            int l_input_res, ouput_type_tmp;

            length = command[1];
            if (length != 4)
                panic("LCD interal error5! length = %d \n", length);
            val = command[2];
            l_input_res = val & 0xFFFF;
            ouput_type_tmp = val >> 16;

            if (pip_config_resolution((ushort) l_input_res, (ushort) ouput_type_tmp) < 0) {
                printk("lcd config output_type:%d(input_res:%d) Fail! \n", ouput_type_tmp, l_input_res);
            } else {
                if (input_res != l_input_res) {
                    input_res = l_input_res;
                    //notify the input resolution
                    if (lcdvg_cb_fn && lcdvg_cb_fn->res_change)
                        lcdvg_cb_fn->res_change();
                }
                output_type = ouput_type_tmp;
            }
            break;
          }

          default:
            printk("%s, unknown command: 0x%x \n", __func__, command[0]);
            break;
        }
    } while(!kthread_should_stop());

    cpu_comm_close(target);
    thread_init = 0;

    return 0;
}
#endif /* CONFIG_PLATFORM_GM8210 */

module_init(pip_init);
module_exit(pip_cleanup);

MODULE_DESCRIPTION("GM LCD200-PIP driver");
MODULE_AUTHOR("GM Technology Corp.");
MODULE_LICENSE("GPL");

