#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <lcd200_v3/lcd_fb.h>
#include <mach/platform/platform_io.h>
#include <mach/fmem.h>
#include <mach/ftpmu010_pcie.h>
#include <frammap/frammap_if.h>
#include <cpu_comm/cpu_comm.h>
#include "framebuffer.h"
#include "scaler_trans.c"

extern void ffb_proc_init(lcd_info_t *lcd_info);
extern void ffb_proc_release(lcd_info_t *lcd_info);

static void ffb_set_fb_input(lcd_info_t *lcd_info, int fb_idx, u32 value);
static void ffb_polling_update(void);

#define MAX_CURS_WIDTH  64
#define MAX_CURS_HEIGHT 64
#define MAX_CURS_COLOR  15  /* offset 0x1204 ~ 0x123C */

#define CPU_COMM_LCD0    CHAN_0_USR6
#define CPU_COMM_LCD2    CHAN_0_USR7

static struct task_struct *lcd_thread = NULL;
static u32 io_base[LCD_IP_NUM] = {LCD_FTLCDC200_0_PA_BASE, LCD_FTLCDC200_1_PA_BASE, LCD_FTLCDC200_2_PA_BASE};
volatile int probe_done = 0;

#ifdef CONFIG_GM8312
/*
 * GM8312 PMU registers
 */
/* offset, bits_mask, lock_bits, init_val, init_mask */
static pmuPcieReg_t pmu_reg_8312[] = {
    /* GM8312 VIDEO_CTRL1, 2 VGA DAC and CVBS on */
    {0x44, 0x3F << 16, 0x3F << 16, 0, 0},
};

static pmuPcieRegInfo_t pmu_reg_info_8312 = {
    "LCD_8312",
    ARRAY_SIZE(pmu_reg_8312),
    ATTR_TYPE_PCIE_NONE,
    &pmu_reg_8312[0]
};
int pcie_lcd_fd = -1;
#endif /* CONFIG_GM8312 */

typedef struct {
    lcd_info_t  lcd_info[LCD_IP_NUM];   //lcd controller info
} soc_info_t;   //chip

/* Local variables
 */
static soc_info_t   soc_info;


#define FFB_DIVID(x,div) (((x)+(div-1))/div*div)
#define FRAME_SIZE_RGB(xres,yres,bpp)     (FFB_DIVID(xres,16) * FFB_DIVID(yres,16) * bpp/8)

/* Fake monspecs to fill in fbinfo structure */
static struct fb_monspecs monspecs = {
    .hfmin = 30000,
    .hfmax = 70000,
    .vfmin = 50,
    .vfmax = 65,
};

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
 *@fn ffb_check_var():
 *    Round up in the following order: bits_per_pixel, xres,
 *    yres, xres_virtual, yres_virtual, xoffset, yoffset, grayscale,
 *    bitfields, horizontal timing, vertical timing.
 */
static int ffb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    ffb_info_t  *fbi = (ffb_info_t *)info;
    lcd_info_t  *lcd_info = (lcd_info_t *)fbi->lcd_info;
    int rgbidx;

    if (var->xres < MIN_XRES)
        var->xres = MIN_XRES;
    if (var->yres < MIN_YRES)
        var->yres = MIN_YRES;
    if (var->xres > lcd_info->max_xres)
        var->xres = lcd_info->max_xres;
    if (var->yres > lcd_info->max_yres)
        var->yres = lcd_info->max_yres;
    var->xres_virtual = max(var->xres_virtual, var->xres);
    var->yres_virtual = max(var->yres_virtual, var->yres);

    if (fbi->video_input_mode >= VIM_RGB) {
        switch (var->bits_per_pixel) {
        case 1:
            rgbidx = RGB_1;
            break;
        case 2:
            rgbidx = RGB_2;
            break;
        case 4:
            rgbidx = RGB_8;
            break;
        case 8:
            rgbidx = RGB_8;
            break;
        case 16:
            rgbidx = RGB_16;
            break;
        case 32:
            if (fbi->video_input_mode == VIM_RGB888)
                rgbidx = RGB_32;
            else
                rgbidx = ARGB;
            break;
        default:
            return -EINVAL;
        }

        var->red = fbi->rgb[rgbidx]->red;
        var->green = fbi->rgb[rgbidx]->green;
        var->blue = fbi->rgb[rgbidx]->blue;
        var->transp = fbi->rgb[rgbidx]->transp;
    }

    return 0;
}

/**
 *@fn ffb_set_par():
 *	Set the user defined part of the display for the specified console
 */
static int ffb_set_par(struct fb_info *info)
{
    ffb_info_t  *fbi = (ffb_info_t *)info;
    struct fb_var_screeninfo *var = &info->var;

    if (fbi->video_input_mode >= VIM_RGB) {
        switch (var->bits_per_pixel) {
        case 32:
        case 16:
            fbi->fb.fix.visual = FB_VISUAL_TRUECOLOR;
            break;
        case 8:
        case 4:
        case 2:
        case 1:
        default:
            fbi->fb.fix.visual = FB_VISUAL_PSEUDOCOLOR;
            break;
        }
    }

    fbi->fb.fix.line_length = var->xres_virtual * var->bits_per_pixel / 8;

    //clean_screen(fbi);
    return 0;
}

static int setpalettereg(u_int regno, u_int red, u_int green,  u_int blue, u_int trans, struct fb_info *info)
{
    ffb_info_t *fbi = (struct ffb_info *)info;
    lcd_info_t *lcd_info = (lcd_info_t *)fbi->lcd_info;
    u32 val, ret = 1, offset, tmp;

    if (regno < lcd_info->palette_size) {
        /* this value based on custom's parameter.
         */
        val = blue & 0x1F;       /* blue */
        val |= (green & 0x3F) << 5;      /* green */
        val |= (red & 0x1F) << 11;       /* red */

        lcd_info->palette_cpu[regno] = val;  //16bits
        ret = 0;

        /* write to hardware */
        if (regno % 2) {
            tmp = lcd_info->palette_cpu[regno - 1] & 0xFFFF;
            tmp |= (val & 0xFFFF) << 16;
        } else {
            tmp = lcd_info->palette_cpu[regno + 1] & 0xFFFF;
            tmp |= (val & 0xFFFF);
        }

        offset = regno / 2;

        iowrite32(tmp, lcd_info->io_base + 0xa00 + (offset * 4));
    }

    return ret;
}

static int ffb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
                         u_int trans, struct fb_info *info)
{

    ffb_info_t *fbi = (ffb_info_t *)info;
    int ret = -1;

    if (fbi->video_input_mode < VIM_RGB)
        panic("%s, fbi->video_input_mode = %d < VIM_RGB\n", __func__, fbi->video_input_mode);


    /**
	 * If greyscale is true, then we convert the RGB value
	 * to greyscale no mater what visual we are using.
	 */
    if (fbi->fb.var.grayscale)
        red = green = blue = (19595 * red + 38470 * green + 7471 * blue) >> 16;

    switch (fbi->fb.fix.visual) {
    case FB_VISUAL_TRUECOLOR:
        /**
		 * 12 or 16-bit True Colour.  We encode the RGB value
		 * according to the RGB bitfield information.
		 */
        if (regno < 16) {
            ret = -1;
        }
        break;
    case FB_VISUAL_DIRECTCOLOR:
    case FB_VISUAL_STATIC_PSEUDOCOLOR:
    case FB_VISUAL_PSEUDOCOLOR:
        ret = setpalettereg(regno, red, green, blue, trans, info);
        break;
    }

    return ret;
}

/**
 *@fn ffb_blank():
 *	Blank the display by setting all palette values to zero.  Note, the
 * 	12 and 16 bpp modes don't really use the palette, so this will not
 *      blank the display in all modes.
 */
int ffb_blank(int blank, struct fb_info *info)
{
    return 0;
}

/* vma->vm_pgoff: It indicates the offset from the start hardware address.
 * EX: mmap(0, ...)
 */
static int ffb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
    ffb_info_t *fbi = (ffb_info_t *)info;
    unsigned long off = vma->vm_pgoff << PAGE_SHIFT;
    int ret = -EINVAL;
    unsigned long total_size = vma->vm_end - vma->vm_start;
    unsigned long start = vma->vm_start;
    unsigned long pfn;
    unsigned long size;

    if (total_size > fbi->fb.fix.smem_len)
        return -1;

    if (off < fbi->fb.fix.smem_len) {
        size = fbi->fb.fix.smem_len - off;
        vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
        vma->vm_flags |= VM_RESERVED;
        if (total_size < fbi->fb.fix.smem_len) {
            size = total_size;
        }

        off += fbi->fb.fix.smem_start;
        pfn = off >> PAGE_SHIFT;
        ret = remap_pfn_range(vma, start, pfn, size, vma->vm_page_prot);
    } else {
        panic("%s, buffer mapping error !!\n", __func__);
    }

    return ret;
}

/**
 *@brief Switch cursor enable/disable.
 *
 *@param 1:Enable,0:Disable
 */
static void lcd200_cursor_ctrl(lcd_info_t *lcd_info, int on)
{
    u32 tmp;

    tmp = ioread32(lcd_info->io_base + 0x0);

    if (on) {
        tmp |= (0x1 << 12);
    } else {
        tmp &= ~(0x1 << 12);
    }

    iowrite32(tmp, lcd_info->io_base + 0x0);
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
    ffb_info_t *fbi = (ffb_info_t *)info;
    lcd_info_t *lcd_info = (lcd_info_t *)fbi->lcd_info;
    int ret = 0;
    int i, set = cursor->set;
    unsigned long retVal;

    if (cursor->image.width > MAX_CURS_WIDTH || cursor->image.height > MAX_CURS_HEIGHT)
        return -ENXIO;

    if (set & (FB_CUR_SETSHAPE | FB_CUR_SETIMAGE | FB_CUR_SETCMAP)) {
        lcd200_cursor_ctrl(lcd_info, 0);
        lcd_info->cursor.enable = 0;
    }

    if (set & FB_CUR_SETPOS) {
        unsigned int dx, dy;

        lcd_info->cursor.dy = dy = cursor->image.dy - info->var.yoffset;
        lcd_info->cursor.dx = dx = cursor->image.dx - info->var.xoffset;

        if (lcd_info->scalar.enable) {
            dx = (dx * lcd_info->scalar.out_xres) / fbi->xres;
            dy = (dy * lcd_info->scalar.out_yres) / fbi->yres;
        }

        iowrite32(((dx & 0xFFF) << 16) | (dy & 0xFFF) | (0x1 << 28), lcd_info->io_base + 0x1200);
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
            iowrite32(tmp_color, lcd_info->io_base + 0x1204 + (i << 2));
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

        if (cursor->image.width == 64) {
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

        if ((cursor->image.width == 64) && (cursor->image.height == 64) && (cursor->image.depth == 4)) {
            int i, size = 64 * 64 * 4 / 8;
            unsigned int value;

            memset_io((void *)(lcd_info->io_base + 0x1600), 0x0, size);
            dev_cursor64x64_translate(data, (u8 *)cursor->image.data, size);

            for (i = 0; i < size; i += 4) {
                value = ((data[i+3] << 24) | (data[i+2] << 16) | (data[i+1] << 8) | data[i]);
                iowrite32(value, lcd_info->io_base + 0x1600 + i);
            }
        } else {
            panic("%s, only supprt 64x64 cursor! \n", __func__);
        }

        if (mask) kfree(mask);
        if (rawdata) kfree(rawdata);
        if (data) kfree(data);
    } /* set & (FB_CUR_SETSHAPE | FB_CUR_SETIMAGE */

    if (cursor->enable) {
        if (!lcd_info->cursor.enable) {
            lcd200_cursor_ctrl(lcd_info, 1);
            lcd_info->cursor.enable = 1;
        }
    } else {
        lcd200_cursor_ctrl(lcd_info, 0);
        lcd_info->cursor.enable = 0;
    }

end:
    return ret;
}

/**
 * This interlace also support buffer queue to manage buffer.
 */
static int ffb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    unsigned int tmp;
    int ret = 0;
    ffb_info_t *fbi = (ffb_info_t *)info;
    lcd_info_t  *lcd_info = (lcd_info_t *)fbi->lcd_info;

    switch (cmd) {
      case FLCD_GET_DATA_SEP:
      case FFB_GET_DATA_SEP:
        {
            struct flcd_data f_data;

            memset(&f_data, 0, sizeof(struct flcd_data));
            f_data.buf_len = fbi->fb.fix.smem_len;
            f_data.uv_offset = fbi->fb.var.xres * fbi->fb.var.yres;
            f_data.frame_no = 1;
            f_data.mp4_map_dma[0] = fbi->fb.fix.smem_start;

            if (copy_to_user((void __user *)arg, &f_data, sizeof(struct flcd_data)))
                ret = -EFAULT;
        }
        break;
      case FLCD_SET_FB_NUM:      //0,1.. n
      case FFB_SET_FB_NUM:
      case FLCD_SET_SPECIAL_FB:  //0,1.. n
      case FFB_SET_SPECIAL_FB:
        break;
      case FFB_IOCURSOR:
      {
            struct fb_cursor fbc;

            if (copy_from_user(&fbc, (unsigned int *)arg, sizeof(struct fb_cursor))) {
                ret = -EFAULT;
                break;
            }

            ret = dev_cursor(info, &fbc);
            break;
      }
      case COLOR_KEY1:
        if (copy_from_user(&tmp, (unsigned int *)arg, sizeof(unsigned int))) {
            ret = -EFAULT;
            break;
        }
        if (tmp > 0xffffff) {
            ret = -EINVAL;
            break;
        }
        lcd_info->color_key1 = tmp;
        break;

      case COLOR_KEY1_EN:
        if (copy_from_user(&tmp, (unsigned int *)arg, sizeof(unsigned int))) {
            ret = -EFAULT;
            break;
        }
        if (tmp == 1)
            iowrite32((0x1 << 24) | lcd_info->color_key1, lcd_info->io_base + 0x320);
        else
            iowrite32(lcd_info->color_key1, lcd_info->io_base + 0x320);
        break;

      case COLOR_KEY2:
        if (copy_from_user(&tmp, (unsigned int *)arg, sizeof(unsigned int))) {
            ret = -EFAULT;
            break;
        }
        lcd_info->color_key2 = tmp;
        break;
      case COLOR_KEY2_EN:
        if (copy_from_user(&tmp, (unsigned int *)arg, sizeof(unsigned int))) {
            ret = -EFAULT;
            break;
        }
        if (tmp == 1)
            iowrite32((0x1 << 24) | lcd_info->color_key1, lcd_info->io_base + 0x324);
        else
            iowrite32(lcd_info->color_key1, lcd_info->io_base + 0x324);
        break;
      case FFB_SET_PALETTE:
        {
            struct palette  data;
            u_int red, green, blue, regno;

            if (copy_from_user(&data, (unsigned int *)arg, sizeof(struct palette))) {
                ret = -EFAULT;
                break;
            }
            regno = data.entry_idx;
            red = (data.color >> 11) & 0x1F;
            green = (data.color >> 5) & 0x3F;
            blue = data.color & 0x1F;
            if (setpalettereg(regno, red, green, blue, 0, &fbi->fb))
                ret = -1;
            break;
        }
      case FFB_GET_EDID:
      {
        struct edid_table   edid;
        cpu_comm_msg_t      msg;
        unsigned int        command = FFB_GET_EDID;

        memset(&edid, 0, sizeof(edid));
        memset(&msg, 0, sizeof(msg));
        msg.target = (lcd_info->index == 0) ? CPU_COMM_LCD0 : CPU_COMM_LCD2;
        msg.length = sizeof(command);
        msg.msg_data = (unsigned char *)&command;
        if (cpu_comm_msg_snd(&msg, -1))
            panic("LCD Fail to send data to cpucomm \n");
        msg.length = sizeof(edid);
        msg.msg_data = (unsigned char *)&edid;
        cpu_comm_msg_rcv(&msg, -1);
        if (copy_to_user((void __user *)arg, &edid, sizeof(edid)))
            ret = -EFAULT;
        break;
      }

      case FFB_SET_EDID:
      {
        unsigned int output_type;
        unsigned int command[3];
        cpu_comm_msg_t  msg;

        if (copy_from_user(&output_type, (unsigned int *)arg, sizeof(unsigned int))) {
            ret = -EFAULT;
            break;
        }
        /* format: command(4), length of body(4), data(length) */
        command[0] = FFB_SET_EDID;
        command[1] = sizeof(output_type);
        command[2] = output_type;

        memset(&msg, 0, sizeof(msg));
        msg.target = (lcd_info->index == 0) ? CPU_COMM_LCD0 : CPU_COMM_LCD2;
        msg.length = sizeof(command);
        msg.msg_data = (unsigned char *)&command[0];
        if (cpu_comm_msg_snd(&msg, -1))
            panic("LCD: Error to send command1! \n");
        break;
      }

      case FFB_GUI_CLONE:
      {
        unsigned int command[3];
        cpu_comm_msg_t  msg;
        u32 bitmap, tmp = 0;
        int i;

        if (copy_from_user(&bitmap, (unsigned int *)arg, sizeof(unsigned int))) {
            ret = -EFAULT;
            break;
        }

        for (i = 0; i < LCD_IP_NUM; i ++)
            set_bit(i, (void *)&tmp);

        if (bitmap > tmp) {
            ret = -EFAULT;
            break;
        }
        printk("LCD: GUI content shared with LCD: ");
        for (i = 0; i < LCD_IP_NUM; i ++) {
            if (!test_bit(i, (void *)&bitmap))
                continue;
            printk("%d ", i);
        }
        printk("\n");

        /* format: command(4), length of body(4), data(length) */
        command[0] = FFB_GUI_CLONE;
        command[1] = sizeof(bitmap);
        command[2] = bitmap;

        memset(&msg, 0, sizeof(msg));
        msg.target = (lcd_info->index == 0) ? CPU_COMM_LCD0 : CPU_COMM_LCD2;
        msg.length = sizeof(command);
        msg.msg_data = (unsigned char *)&command[0];

        if (cpu_comm_msg_snd(&msg, -1))
            panic("LCD: Error to send command2! \n");
        break;
      }
      case FFB_GUI_CLONE_ADJUST:
      {
        unsigned int command[20];
        cpu_comm_msg_t  msg;

        if (sizeof(command) - 8 < sizeof(struct gui_clone_adjust))
            panic("array is too small! \n");

        if (lcd_info->index != 0) {
            printk("Error! This function is only for LCD0! \n");
            ret = -EFAULT;
            break;
        }

        memset(command, 0, sizeof(command));
        command[0] = FFB_GUI_CLONE_ADJUST;
        command[1] = sizeof(struct gui_clone_adjust);
        if (copy_from_user(&command[2], (unsigned int *)arg, sizeof(struct gui_clone_adjust))) {
            ret = -EFAULT;
            break;
        }
        memset(&msg, 0, sizeof(msg));
        msg.target = CPU_COMM_LCD0;
        msg.length = sizeof(command);
        msg.msg_data = (unsigned char *)&command[0];
        if (cpu_comm_msg_snd(&msg, -1))
            panic("LCD: Error to send command3! \n");
        break;
      }
      default:
        printk("%s, command: 0x%x is not supported! \n", __func__, cmd);
        ret = -EFAULT;
        break;
    }

    return ret;
}

/* Calculate the frame buffer size
 */
static u32 cal_frame_buf_size(struct ffb_info *fbi)
{
    u32 fb_sz;
    //lcd_info_t  *lcd_info = (lcd_info_t *)fbi->lcd_info;

    fb_sz = FRAME_SIZE_RGB(fbi->xres, fbi->yres, fbi->fb.var.bits_per_pixel);

    return PAGE_ALIGN(fb_sz);
}

/* assign RGB info to ffb_info
 */
static int __init init_ffb_info(ffb_info_t *fbi)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)fbi->lcd_info;
    char tstr[16];

    switch (fbi->video_input_mode) {
      case VIM_ARGB:
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
      default:
        fbi->fb.var.bits_per_pixel = 8;
        break;
    }

    if (fbi->index == 0) {
        switch (lcd_info->index) {
          case 0:
            snprintf(tstr, 16, "flcd");
            break;
          case 1:
            snprintf(tstr, 16, "flcd1");
            break;
          case 2:
            snprintf(tstr, 16, "flcd2");
            break;
        }
    } else {
        switch (lcd_info->index) {
          case 0:
            snprintf(tstr, 16, "flcd_s%d", fbi->index);
            break;
          case 1:
            snprintf(tstr, 16, "flcd1_s%d", fbi->index);
            break;
          case 2:
            snprintf(tstr, 16, "flcd2_s%d", fbi->index);
            break;
        }
    } /* fbi->index */

    strcpy(fbi->fb.fix.id, tstr);

    fbi->fb.fix.smem_len = cal_frame_buf_size(fbi);

    fbi->rgb[RGB_1] = &rgb_1;
    fbi->rgb[RGB_2] = &rgb_2;
    fbi->rgb[RGB_8] = &rgb_8;
    fbi->rgb[RGB_16] = &def_rgb_16;
    fbi->rgb[RGB_32] = &def_rgb_32;
    fbi->rgb[ARGB] = &def_argb;

    return 0;
}

static int __init init_fbinfo(ffb_info_t *fbi)
{
    struct fb_ops *fbops = NULL;

    fbops = kzalloc(sizeof(struct fb_ops), GFP_KERNEL);
    if (fbops == NULL)
        panic("%s, no memory! \n", __func__);

    fbops->fb_check_var = ffb_check_var,
    fbops->fb_set_par = ffb_set_par,
    fbops->fb_setcolreg = ffb_setcolreg,
    fbops->fb_fillrect = cfb_fillrect,
    fbops->fb_copyarea = cfb_copyarea,
    fbops->fb_imageblit = cfb_imageblit,
    fbops->fb_blank = ffb_blank,
    //fbops->fb_cursor = ffb_soft_cursor,
    fbops->fb_mmap = ffb_mmap,
    fbops->fb_ioctl = ffb_ioctl,
    fbi->fb.fbops = fbops;

    fbi->fb.fix.type = FB_TYPE_PACKED_PIXELS;
    fbi->fb.fix.type_aux = 0;
    fbi->fb.fix.xpanstep = 0;
    fbi->fb.fix.ypanstep = 0;
    fbi->fb.fix.ywrapstep = 0;
    fbi->fb.fix.accel = FB_ACCEL_NONE;

    fbi->fb.var.nonstd = 0;
    fbi->fb.var.activate = FB_ACTIVATE_NOW;
    fbi->fb.var.height = -1;
    fbi->fb.var.width = -1;
    fbi->fb.var.accel_flags = 0;
    fbi->fb.var.vmode = FB_VMODE_NONINTERLACED;
    fbi->fb.flags = FBINFO_DEFAULT;
    fbi->fb.monspecs = monspecs;
    fbi->fb.pseudo_palette = NULL;

    return 0;
}

static inline void unmap_video_memory(ffb_info_t *fbi)
{
    if (fbi->active == 0)
        return;

    __iounmap(fbi->fb_addr_va);
}

/**
 *@fn map_video_memory():
 *      Allocates the DRAM memory for the frame buffer.  This buffer is
 *	remapped into a non-cached, non-buffered, memory region to
 *      allow palette and pixel writes to occur without flushing the
 *      cache.  Once this area is remapped, all virtual memory
 *      access to the video memory should occur at the new region.
 */
static int __init map_video_memory(ffb_info_t *fbi)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)fbi->lcd_info;
    u32  size = PAGE_ALIGN(fbi->fb.fix.smem_len);

    /* got from cpucomm in init stage already */
    if (fbi->fb_addr) {
        fbi->fb_addr_va = (void *)ioremap_nocache(fbi->fb_addr, size);
        if (fbi->fb_addr_va == NULL)
            panic("%s, No virtual memory! \n", __func__);

        fbi->fb.screen_base = (char __iomem *)fbi->fb_addr_va;
        fbi->fb.fix.smem_start = fbi->fb_addr;

        printk("FB%d, physical addr: 0x%x \n", fbi->index, fbi->fb_addr);
    }

    fbi->active = fbi->fb_addr ? 1 : 0;

    if (!lcd_info->palette_cpu) {
        lcd_info->palette_cpu = (u16 *)kmalloc(PAGE_SIZE, GFP_KERNEL);
        memset((void *)lcd_info->palette_cpu, 0, PAGE_SIZE);
        lcd_info->palette_size = 256;
    }

    return 0;
}


static int __init ffb_construct(ffb_info_t *fbi)
{
    /* Initiation frame buffer information that kernel's layer needs. */
    init_fbinfo(fbi);
    map_video_memory(fbi);

    return 0;
}

static int lcd_config_thread(void *private)
{
    cpu_comm_msg_t msg;
    lcd_info_t  *lcd_info;
    ffb_info_t  *ffb_info;
    int ret, lcd, fb_plane;
    u32 i, wait_count;
    u32 tmp, rcv_data[10]; /* 0x3035 + pip0 paddr + pip1 paddr + pip2 paddr and reserved */

    if (private)    {}

    if (cpu_comm_open(CPU_COMM_LCD0, "lcd0_drv"))
        panic("LCD0: open cpucomm channel fail! \n");

    if (cpu_comm_open(CPU_COMM_LCD2, "lcd2_drv"))
        panic("LCD2: open cpucomm channel fail! \n");

    printk("Initialize FA726 LCD driver ... \n");

    for (lcd = 0; lcd < LCD_IP_NUM; lcd ++) {
        if (lcd == 1)
            continue;

        lcd_info = &soc_info.lcd_info[lcd];
        if (lcd == 0)
            sprintf(lcd_info->name, "flcd200");
        else
            sprintf(lcd_info->name, "flcd200_%d", lcd);

        lcd_info->soc_info = (void *)&soc_info;
        lcd_info->index = lcd;

        lcd_info->io_base_pa = io_base[lcd];
        lcd_info->io_base = ioremap_nocache(lcd_info->io_base_pa, 0x2000);
        if (lcd_info->io_base == NULL)
            panic("LCD %d: no virtual address \n", lcd);

        printk("Wait FA626 LCD%d driver to be ready ...", lcd);
        wait_count = (lcd == 0) ? 100 : 20;  //10sec

        memset(&msg, 0, sizeof(msg));
        msg.target = (lcd_info->index == 0) ? CPU_COMM_LCD0 : CPU_COMM_LCD2;
        msg.length = sizeof(rcv_data);
        msg.msg_data = (unsigned char *)&rcv_data;

        for (i = 0; i < wait_count; i ++) {
            /* receive data from cpucomm */
            if (!cpu_comm_msg_rcv(&msg, 500))
                break;
            printk(".");
        }
        if (i >= wait_count) {
            printk("LCD%d not exists!\n", lcd);
            continue;
        }
        if (rcv_data[0] != 0x3035)
            panic("%s, the rcv_data[0]: 0x%x != 0x3035! \n", __func__, rcv_data[0]);
        if (msg.length < 16)    /* 0x3035 + 3 * fb_addr */
            panic("%s, the rcv length: %d is too small! \n", __func__, msg.length);
        printk(" done(%d)\n", i);

        lcd_info->active = 1;   //lcdc active
        /* max resolution */
        if (lcd == 2) {
            lcd_info->max_xres = 720;
            lcd_info->max_yres = 576;
        } else {
            lcd_info->max_xres = 1920;
            lcd_info->max_yres = 1080;
        }

        /* for each plane */
        for (fb_plane = 0; fb_plane < MAX_FBNUM; fb_plane ++) {
            u32 bpp;

            ffb_info = &lcd_info->fb[fb_plane];
            ffb_info->index = fb_plane;

            /* get frame buffer address from cpucomm */
            ffb_info->fb_addr = rcv_data[1 + fb_plane];

            tmp = ioread32(lcd_info->io_base + 0x318);
            /* input mode */
            switch (fb_plane) {
              case 0:
                ffb_info->video_input_mode = VIM_YUV422;
                break;
              case 1:
                bpp = (tmp >> 4) & 0x7;
                ffb_info->video_input_mode = (bpp <= 4) ? VIM_RGB565 : VIM_ARGB;
                break;
              case 2: /* plane 2 is a special plane */
                bpp = (tmp >> 8) & 0x7;
                switch (bpp) {
                  case 0:
                    ffb_info->video_input_mode = VIM_RGB1BPP;
                    break;
                  case 1:
                    ffb_info->video_input_mode = VIM_RGB2BPP;
                    break;
                  case 4:
                    ffb_info->video_input_mode = VIM_RGB565;
                    break;
                  case 5:
                  case 6:
                    ffb_info->video_input_mode = VIM_ARGB;
                    break;
                  default:
                    ffb_info->video_input_mode = VIM_RGB565;
                    panic("should not enter here! bpp value = %d \n", bpp);
                    break;
                } /* bpp */
                break;
              default:
                ffb_info->video_input_mode = VIM_RGB565;
                break;
            } /* fb_plane */

            /* ScalerEn */
            tmp = (ioread32(lcd_info->io_base) >> 5) & 0x1;
            if (tmp) {
                /* scaler enabled */
                tmp = ioread32(lcd_info->io_base + 0x1100) & 0xFFF;
                ffb_info->xres = tmp + 1;
                tmp = ioread32(lcd_info->io_base + 0x1104) & 0xFFF;
                ffb_info->yres = tmp + 1;

                lcd_info->scalar.enable = 1;
                lcd_info->scalar.out_xres = ioread32(lcd_info->io_base + 0x1108) & 0x3FFF;
                lcd_info->scalar.out_yres = ioread32(lcd_info->io_base + 0x110C) & 0x3FFF;
            } else {
                tmp = ioread32(lcd_info->io_base + 0x100) & 0xFF;
                ffb_info->xres = 16 * (tmp + 1);
                tmp = ioread32(lcd_info->io_base + 0x104) & 0xFFF;
                ffb_info->yres = tmp + 1;

                lcd_info->scalar.enable = 0;
                lcd_info->scalar.out_xres = ffb_info->xres;
                lcd_info->scalar.out_yres = ffb_info->yres;
            }
            ffb_info->fb.var.xres = ffb_info->fb.var.xres_virtual = ffb_info->xres;
            ffb_info->fb.var.yres = ffb_info->fb.var.yres_virtual = ffb_info->yres;

            ffb_info->lcd_info = (void *)lcd_info;

            /* assign RGB info to ffb_info */
            init_ffb_info(ffb_info);
            ffb_construct(ffb_info);

            if (ffb_info->active != 1)
                continue;

            ret = register_framebuffer(&ffb_info->fb);
            if (ret < 0)
                panic("register_framebuffer plane:%d failed\n", fb_plane);

            if (ffb_info->fb.fbops->fb_check_var(&ffb_info->fb.var, &ffb_info->fb) < 0)
                panic("LCD%d, fb_check_var for plane:%d failed\n", lcd, fb_plane);

            if (ffb_info->fb.fbops->fb_set_par(&ffb_info->fb) < 0)
                panic("LCD%d, fb_set_par for plane:%d failed\n", lcd, fb_plane);

            printk("LCD%d, plane%d, in_fmt: %d, xres: %d, yres: %d, bpp: %d \n", lcd, fb_plane,
                ffb_info->video_input_mode, ffb_info->fb.var.xres, ffb_info->fb.var.yres,
                ffb_info->fb.var.bits_per_pixel);
        } /* plane */

        ffb_proc_init(lcd_info);
    } /* lcd */

    probe_done = 1;

    do {
        ffb_polling_update();
        msleep(800);
    } while (!kthread_should_stop());

    cpu_comm_close(CPU_COMM_LCD0);
    cpu_comm_close(CPU_COMM_LCD2);

    return 0;
}

void ffb_polling_update(void)
{
    lcd_info_t  *lcd_info;
    ffb_info_t  *ffb_info;
    int lcd_idx, plane, update;
    u32 tmp, value, bpp;

    for (lcd_idx = 0; lcd_idx < LCD_IP_NUM; lcd_idx ++) {
        lcd_info = &soc_info.lcd_info[lcd_idx];
        if (!lcd_info->active)
            continue;

        for (plane = 0; plane < MAX_FBNUM; plane ++) {
            ffb_info = &lcd_info->fb[plane];
            if (!ffb_info->active)
                continue;

            update = 0;
            tmp = (ioread32(lcd_info->io_base) >> 5) & 0x1;
            if (tmp) {
                /* scaler enabled */
                tmp = ioread32(lcd_info->io_base + 0x1100) & 0xFFF;
                if (ffb_info->xres != (tmp + 1)) {
                    ffb_info->xres = tmp + 1;
                    ffb_info->fb.var.xres = ffb_info->fb.var.xres_virtual = ffb_info->xres;
                    update = 1;
                }
                tmp = ioread32(lcd_info->io_base + 0x1104) & 0xFFF;
                if (ffb_info->yres != (tmp + 1)) {
                    ffb_info->yres = tmp + 1;
                    ffb_info->fb.var.yres = ffb_info->fb.var.yres_virtual = ffb_info->yres;
                    update = 1;
                }

                lcd_info->scalar.enable = 1;
                lcd_info->scalar.out_xres = ioread32(lcd_info->io_base + 0x1108) & 0x3FFF;
                lcd_info->scalar.out_yres = ioread32(lcd_info->io_base + 0x110C) & 0x3FFF;
            } else {
                /* scaler disabled */
                tmp = ioread32(lcd_info->io_base + 0x100) & 0xFF;
                if (ffb_info->xres != (16 * (tmp + 1))) {
                    ffb_info->xres = 16 * (tmp + 1);
                    ffb_info->fb.var.xres = ffb_info->fb.var.xres_virtual = ffb_info->xres;
                    update = 1;
                }
                tmp = ioread32(lcd_info->io_base + 0x104) & 0xFFF;
                if (ffb_info->yres != (tmp + 1)) {
                    ffb_info->yres = tmp + 1;
                    ffb_info->fb.var.yres = ffb_info->fb.var.yres_virtual = ffb_info->yres;
                    update = 1;
                }
                lcd_info->scalar.enable = 0;
                lcd_info->scalar.out_xres = ffb_info->xres;
                lcd_info->scalar.out_yres = ffb_info->yres;
            }

            tmp = ioread32(lcd_info->io_base + 0x318);
            value = ffb_info->video_input_mode;
            /* input mode */
            if (plane == 2) {
                bpp = (tmp >> 8) & 0x7;
                switch (bpp) {
                  case 0:
                    value = VIM_RGB1BPP;
                    break;
                  case 1:
                    value = VIM_RGB2BPP;
                    break;
                  default:
                    break;
                } /* bpp */
                if (value != ffb_info->video_input_mode)
                    update = 1;
            }

            if (update)
                ffb_set_fb_input(lcd_info, plane, value);
        } /* plane */
    } /* lcd */
}

void ffb_set_fb_input(lcd_info_t *lcd_info, int fb_plane, u32 value)
{
    ffb_info_t  *fbi;

    fbi = &lcd_info->fb[fb_plane];

    fbi->video_input_mode = value; /* VIM_RGB1BPP, .... */

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
        panic("Error in %s, value = %d \n", __func__, fbi->video_input_mode);
        break;
    }

    if (fbi->fb.fbops->fb_check_var(&fbi->fb.var, &fbi->fb) < 0)
                panic("LCD%d, fb_check_var for plane:%d failed\n", lcd_info->index, fb_plane);

    if (fbi->fb.fbops->fb_set_par(&fbi->fb) < 0)
        panic("LCD%d, fb_set_par for plane:%d failed\n", lcd_info->index, fb_plane);

    fbi->fb.fix.smem_len = cal_frame_buf_size(fbi);

    printk("LCD%d, plane%d, in_fmt: %d, xres: %d, yres: %d, bpp: %d, smem_len: %d \n", lcd_info->index, fb_plane,
        fbi->video_input_mode, fbi->fb.var.xres, fbi->fb.var.yres,
        fbi->fb.var.bits_per_pixel, fbi->fb.fix.smem_len);

}

/* Init function
 */
static int __init framebuffer_init(void)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);

    /* the driver only executed in FA726 of PCI Host */
    if ((pci_id != FMEM_PCI_HOST) || (cpu_id != FMEM_CPU_FA726))
        return 0;

#ifdef CONFIG_GM8312
    pcie_lcd_fd = ftpmu010_pcie_register_reg(&pmu_reg_info_8312);
    if (pcie_lcd_fd < 0)
        panic("lcd register pcie_pmu fail!");
#endif

    memset(&soc_info, 0, sizeof(soc_info_t));

    lcd_thread = kthread_create(lcd_config_thread, NULL, "lcd_poll");
    wake_up_process(lcd_thread);

    while (!probe_done) {
        msleep(100);
    }
    printk("probe done! \n");

    scaler_trans_init();

    return 0;
}

/* Cleanup function
 */
static void __exit framebuffer_cleanup(void)
{
    int lcd, fb_plane;
    lcd_info_t  *lcd_info;
    ffb_info_t  *ffb_info;
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);

    scaler_trans_exit();

    /* the driver only executed in FA726 of PCI Host */
    if ((pci_id != FMEM_PCI_HOST) || (cpu_id != FMEM_CPU_FA726))
        return;

#ifdef CONFIG_GM8312
    if (pcie_lcd_fd >= 0)
        ftpmu010_deregister_reg(pcie_lcd_fd);
    pcie_lcd_fd = -1;
#endif

    for (lcd = 0; lcd < LCD_IP_NUM; lcd ++) {
        lcd_info = &soc_info.lcd_info[lcd];
        if (!lcd_info->active)
            continue;

        kfree(lcd_info->palette_cpu);
        lcd_info->palette_cpu = NULL;

        for (fb_plane = 0; fb_plane < MAX_FBNUM; fb_plane ++) {
            ffb_info = &lcd_info->fb[fb_plane];
            if (ffb_info->fb.fbops)
                kfree(ffb_info->fb.fbops);
            ffb_info->fb.fbops = NULL;

            if (ffb_info->active != 1)
                continue;

            unmap_video_memory(ffb_info);
            unregister_framebuffer(&ffb_info->fb);
        }

        __iounmap(lcd_info->io_base);
        /* remove proc */
        ffb_proc_release(lcd_info);
    }

    if (lcd_thread)
        kthread_stop(lcd_thread);

    lcd_thread = NULL;

    return;
}

module_init(framebuffer_init);
module_exit(framebuffer_cleanup);

MODULE_DESCRIPTION("GM Framebuffer driver");
MODULE_AUTHOR("GM Technology Corp.");
MODULE_LICENSE("GPL");



