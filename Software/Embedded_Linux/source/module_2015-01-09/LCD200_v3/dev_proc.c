#include <linux/seq_file.h>
#include <mach/ftpmu010.h>
#include "ffb.h"
#include "dev.h"
#include "proc.h"
#include "platform.h"

static struct proc_dir_entry *saturation_proc = NULL;
static struct proc_dir_entry *brightness_proc = NULL;
static struct proc_dir_entry *hue_proc = NULL;
static struct proc_dir_entry *sharpness_proc = NULL;
static struct proc_dir_entry *contrast_proc = NULL;
static struct proc_dir_entry *register_proc = NULL;
static struct proc_dir_entry *dac_inputsel_proc = NULL;
static struct proc_dir_entry *pattern_gen_proc = NULL;
static struct proc_dir_entry *lcd_switch_proc = NULL;
static struct proc_dir_entry *cvbs_screen_sz_proc = NULL;
static struct proc_dir_entry *cvbs_screen_pos_proc = NULL;
static struct proc_dir_entry *mouse_posscale_proc = NULL;
static struct proc_dir_entry *dac_onoff_proc = NULL;

extern unsigned int TVE100_BASE;

/*
 * mouse position scaling
 */
static int mouse_posscale_show(struct seq_file *sfile, void *v)
{
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)p_dev_info;

    if (v) {
    }
    /* avoid warning */
    seq_printf(sfile, "Cursor Position Scale: %s \n",
               dev_info->cursor_pos_scale ? "Enabled" : "Disabled");

    return 0;
}

static int mouse_posscale_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, mouse_posscale_show, NULL);
}

static ssize_t mouse_posscale_proc_write(struct file *file, const char __user * buf, size_t size,
                                         loff_t * off)
{
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)p_dev_info;
    int len = size;
    char value[20];
    int mouse_scale;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';
    sscanf(value, "%u \n", &mouse_scale);
    dev_info->cursor_pos_scale = (mouse_scale >= 1) ? 1 : 0;

    return len;
}

static struct file_operations mouse_posscale_proc_ops = {
    .owner = THIS_MODULE,
    .open = mouse_posscale_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = mouse_posscale_proc_write,
};

/*
 * Saturation proc related functions.
 */
static int saturation_show(struct seq_file *sfile, void *v)
{
    struct lcd200_dev_info *info = p_dev_info;
    int value;

    if (v) {
    }
    /* avoid warning */
    value = (info->color_mgt_p0 >> 8) & 0x3F;

    seq_printf(sfile, "The saturation range is 0 ~ 63. \n");
    seq_printf(sfile, "Current saturation value is %d. \n", value);

    return 0;
}

static int saturation_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, saturation_show, NULL);
}

static ssize_t saturation_proc_write(struct file *file, const char __user * buf, size_t size,
                                     loff_t * off)
{
    struct lcd200_dev_info *info = p_dev_info;
    int len = size;
    char value[20];
    int sat_value;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';
    sscanf(value, "%u \n", &sat_value);
    if (sat_value > 63)
        printk("Out of range! The valid range is 0 - 63! \n");
    else {
        u32 value;

        value = info->ops->reg_read((struct ffb_dev_info *)info, LCD_COLOR_MP0);
        value &= ~0x3F00;       /* maskout SatValue bit 13-8 */
        value |= (sat_value << 8);
        info->ops->reg_write((struct ffb_dev_info *)info, LCD_COLOR_MP0, value, 0x3F00);
        info->color_mgt_p0 = value;
    }

    return size;
}

static struct file_operations saturation_proc_ops = {
    .owner = THIS_MODULE,
    .open = saturation_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = saturation_proc_write,
};

/*
 * Brightness proc related functions.
 */
static int brightness_show(struct seq_file *sfile, void *v)
{
    struct lcd200_dev_info *info = p_dev_info;
    int value;

    if (v) {
    }
    /* avoid warning */
    value = info->color_mgt_p0 & 0x7F;
    if ((info->color_mgt_p0 >> 7) & 0x1)
        value *= -1;

    seq_printf(sfile, "The brightness range is -127 ~ +127. \n");
    seq_printf(sfile, "Current brightness value is %d. \n", value);

    return 0;
}

static int brightness_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, brightness_show, NULL);
}

static ssize_t brightness_proc_write(struct file *file, const char __user * buf, size_t size,
                                     loff_t * off)
{
    struct lcd200_dev_info *info = p_dev_info;
    int len = size;
    char value[20];
    int brightness_value;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';
    /* read brightness value */
    sscanf(value, "%d\n", &brightness_value);
    if (abs(brightness_value) > 127)
        printk("Out of range! The valid range is -127 ~ +127! \n");
    else {
        u32 value;

        value = info->ops->reg_read((struct ffb_dev_info *)info, LCD_COLOR_MP0);
        value &= ~0xFF;         /* maskout Brightness bit 0-7 */
        value |= abs(brightness_value);
        value |= ((brightness_value >= 0) ? (0x0 << 7) : (0x1 << 7));
        info->ops->reg_write((struct ffb_dev_info *)info, LCD_COLOR_MP0, value, 0xFF);
        info->color_mgt_p0 = value;
    }

    return size;
}

static struct file_operations brightness_proc_ops = {
    .owner = THIS_MODULE,
    .open = brightness_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = brightness_proc_write,
};

/*
 * Sharpness proc related functions.
 */
static int sharpness_show(struct seq_file *sfile, void *v)
{
    struct lcd200_dev_info *info = p_dev_info;
    int k1, k0, ShTh1, ShTh0;

    if (v) {
    }
    /* avoid warning */
    k1 = (info->color_mgt_p2 >> 20) & 0xF;
    k0 = (info->color_mgt_p2 >> 16) & 0xF;
    ShTh1 = (info->color_mgt_p2 >> 8) & 0xFF;
    ShTh0 = info->color_mgt_p2 & 0xFF;

    seq_printf(sfile,
               "Range for K0 and K1 is 0 - 15, range for threshold0 and threshold1 is 0 - 255 \n");
    seq_printf(sfile, "K0   K1   Threshold0   Threshold1 \n");
    seq_printf(sfile, "%-2d   %-2d   %-10d   %-10d \n", k0, k1, ShTh0, ShTh1);

    return 0;
}

static int sharpness_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, sharpness_show, NULL);
}

static ssize_t sharpness_proc_write(struct file *file, const char __user * buf, size_t size,
                                    loff_t * off)
{
    struct lcd200_dev_info *info = p_dev_info;
    int len = size;
    char value[60];
    int k1, k0, ShTh1, ShTh0;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';
    /* read brightness value */
    sscanf(value, "%u %u %u %u\n", &k0, &k1, &ShTh0, &ShTh1);
    if (k0 > 15 || k1 > 15 || ShTh0 > 255 || ShTh1 > 255)
        printk("Range for K0 and K1 is 0 - 15, range for threshold0 and threshold1 is 0 - 255 \n");
    else {
        u32 value;

        value = k1 << 20 | k0 << 16 | ShTh1 << 8 | ShTh0;

        info->ops->reg_write((struct ffb_dev_info *)info, LCD_COLOR_MP2, value, 0xFFFFFF);
        info->color_mgt_p2 = value;
    }

    return size;
}

static struct file_operations sharpness_proc_ops = {
    .owner = THIS_MODULE,
    .open = sharpness_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = sharpness_proc_write,
};

/*
 * Contrast proc related functions.
 */
static int contrast_show(struct seq_file *sfile, void *v)
{
    struct lcd200_dev_info *info = p_dev_info;
    u32 value;

    if (v) {
    }
    /* avoid warning */
    value = (info->color_mgt_p3 >> 16) & 0x1F;
    seq_printf(sfile, "The contrast slope range is 1 ~ 31. \n");
    seq_printf(sfile, "Current contrast slope is %d. \n", value);

    return 0;
}

static int contrast_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, contrast_show, NULL);
}

static ssize_t contrast_proc_write(struct file *file, const char __user * buf, size_t size,
                                   loff_t * off)
{
    struct lcd200_dev_info *info = p_dev_info;
    int len = size;
    char value[20];
    int contrast_slope;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';
    /* read contrast value */
    sscanf(value, "%u\n", &contrast_slope);
    if (contrast_slope == 0 || contrast_slope > 31)
        printk("Out of range! The valid slope range is 1 - 31! \n");
    else {
        u32 regVal = 0, sign, contr_offset;

        sign = (contrast_slope * 128) > 512 ? 1 : 0;
        contr_offset = abs(contrast_slope * 128 - 512) & 0xFFF;
        regVal = (contrast_slope << 16) | (sign << 12) | contr_offset;
        info->ops->reg_write((struct ffb_dev_info *)info, LCD_COLOR_MP3, regVal, 0x1FFFFF);
        info->color_mgt_p3 = regVal;
    }

    return size;
}

static struct file_operations contrast_proc_ops = {
    .owner = THIS_MODULE,
    .open = contrast_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = contrast_proc_write,
};

/*
 * HUE proc related functions.
 */
static int hue_show(struct seq_file *sfile, void *v)
{
    struct lcd200_dev_info *info = p_dev_info;
    int HuCosValue, HuSinValue;

    if (v) {
    }
    /* avoid warning */
    HuCosValue = (info->color_mgt_p1 >> 8) & 0x3F;
    HuCosValue = ((info->color_mgt_p1 >> 14) & 0x01) ? -HuCosValue : HuCosValue;

    HuSinValue = info->color_mgt_p1 & 0x3F;
    HuSinValue = ((info->color_mgt_p1 >> 6) & 0x01) ? -HuSinValue : HuSinValue;

    seq_printf(sfile, "Valid range for both HueCosValue and HueSinValue is -32 ~ +32. \n");
    seq_printf(sfile, "Cos@ = (HueCosValue / 32). @ is the rotating degreee from -180 ~ 180. \n");
    seq_printf(sfile, "Sin@ = (HueSinValue / 32). @ is the rotating degreee from -180 ~ 180. \n");
    seq_printf(sfile, "Current HueCosValue = %d, HueSinValue = %d \n", HuCosValue, HuSinValue);

    return 0;
}

static int hue_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, hue_show, NULL);
}

static ssize_t hue_proc_write(struct file *file, const char __user * buf, size_t size, loff_t * off)
{
    struct lcd200_dev_info *info = p_dev_info;
    int len = size;
    char value[20];
    int HuCosValue, HuSinValue;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';

    /* read Hue cos value and Hue sin value */
    sscanf(value, "%d %d\n", &HuCosValue, &HuSinValue);
    if (abs(HuCosValue) > 32 || abs(HuSinValue) > 32)
        printk("Valid range for both HueCosValue and HueSinValue is from -32 ~ +32. \n");
    else {
        u32 regVal, SigHuCos, SigHuSin;

        SigHuCos = (HuCosValue >= 0) ? 0 : 1;   /* 0 is positive, otherwise 1 */
        SigHuSin = (HuSinValue >= 0) ? 0 : 1;   /* 0 is positive, otherwise 1 */

        regVal = (SigHuCos << 14) | (abs(HuCosValue) << 8) | (SigHuSin << 6) | abs(HuSinValue);

        info->ops->reg_write((struct ffb_dev_info *)info, LCD_COLOR_MP1, regVal, 0x7FFF);

        info->color_mgt_p1 = regVal;
    }

    return size;
}

static struct file_operations hue_proc_ops = {
    .owner = THIS_MODULE,
    .open = hue_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = hue_proc_write,
};

/* register dump */
static int register_show(struct seq_file *sfile, void *v)
{
    struct lcd200_dev_info *info = p_dev_info;
    int offset;

    seq_printf(sfile, "========== PMU registers =========\n");
    offset = 0x38;
    seq_printf(sfile, "PMU offset[%2x] = 0x%x \n", offset, ftpmu010_read_reg(offset));

#ifdef CONFIG_PLATFORM_GM8181
    offset = 0x50;
    seq_printf(sfile, "PMU offset[%2x] = 0x%x \n", offset, ftpmu010_read_reg(offset));
    offset = 0x70;
    seq_printf(sfile, "PMU offset[%2x] = 0x%x \n", offset, ftpmu010_read_reg(offset));
#endif

    seq_printf(sfile, "LCD Global Parameters \n");
    for (offset = 0x0; offset <= 0x54; offset += 4) {
        if ((offset == 0x1C) || (offset == 0x20) || (offset == 0x28) || (offset == 0x2C)
            || (offset == 0x34) || (offset == 0x38) || (offset == 0x40) || (offset == 0x44))
            continue;

        seq_printf(sfile, "offset[%2x] = 0x%x \n", offset,
                   *((u32 *) (info->dev_info.io_base + offset)));
    }

    seq_printf(sfile, "\nLCD timing and polarity parameters\n");
    for (offset = 0x100; offset <= 0x10C; offset += 4) {
        seq_printf(sfile, "offset[%3x] = 0x%x \n", offset,
                   *((u32 *) (info->dev_info.io_base + offset)));
    }

    seq_printf(sfile, "\nLCD output format parameters\n");
    for (offset = 0x200; offset <= 0x224; offset += 4) {
        seq_printf(sfile, "offset[%3x] = 0x%x \n", offset,
                   *((u32 *) (info->dev_info.io_base + offset)));
    }

    seq_printf(sfile, "\nLCD image parameters\n");
    for (offset = 0x300; offset <= 0x320; offset += 4) {
        seq_printf(sfile, "offset[%3x] = 0x%x \n", offset,
                   *((u32 *) (info->dev_info.io_base + offset)));
    }

    seq_printf(sfile, "\nLCD image color management\n");
    for (offset = 0x400; offset <= 0x40C; offset += 4) {
        seq_printf(sfile, "offset[%3x] = 0x%x \n", offset,
                   *((u32 *) (info->dev_info.io_base + offset)));
    }

    seq_printf(sfile, "\nLCD gamma correction\n");
    for (offset = 0x600; offset <= 0x8fC; offset += 8) {
        seq_printf(sfile, "offset[%3x] = 0x%-8x  offset[%3x] = 0x%-8x  offset[%3x] = 0x%-8x \n",
                   offset, *((u32 *) (info->dev_info.io_base + offset)), offset + 4,
                   *((u32 *) (info->dev_info.io_base + offset + 4)), offset + 8,
                   *((u32 *) (info->dev_info.io_base + offset + 8)));
    }

    return 0;
}

static int register_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, register_show, NULL);
}

static struct file_operations register_proc_ops = {
    .owner = THIS_MODULE,
    .open = register_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = NULL,
};

/* switch LCD200-1 and LCD200-2 output
 */
static int dac_inputsel_show(struct seq_file *sfile, void *v)
{
    u32 dac, mode;
    struct ffb_dev_info *ff_dev_info = (struct ffb_dev_info *)p_dev_info;

    if (ff_dev_info->video_output_type >= VOT_CCIR656) {
        seq_printf(sfile, "Mode:  %d) for PAL, %d) for NTSC. \n", VOT_PAL, VOT_NTSC);
        dac = ioread32(TVE100_BASE);
        mode = (dac == 2) ? VOT_PAL : VOT_NTSC;
        seq_printf(sfile, "Current Mode = %d \n", mode);
    }
    return 0;
}

static ssize_t dac_inputsel_proc_write(struct file *file, const char __user * buf, size_t size,
                                       loff_t * off)
{
    char value[10];
    int len = size;
    u32 mode;
    struct ffb_dev_info *ff_dev_info = (struct ffb_dev_info *)p_dev_info;
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)p_dev_info;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';

    /* read value */
    sscanf(value, "%u\n", &mode);

    if (ff_dev_info->video_output_type >= VOT_CCIR656) {
        if ((mode != VOT_PAL) && (mode != VOT_NTSC))
            printk("Only support PAL or NTSC! \n");
        else
            tve100_config(dev_info->lcd200_idx, mode, 2);
    } else {
        printk("Not in CCIR656 mode! \n");
    }

    return size;
}

static int dac_inputsel_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, dac_inputsel_show, NULL);
}

static struct file_operations dac_inputsel_proc_ops = {
    .owner = THIS_MODULE,
    .open = dac_inputsel_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = dac_inputsel_proc_write,
};

/*
 * Pattern Generation
 */
static ssize_t pattern_gen_proc_write(struct file *file, const char __user * buf, size_t size,
                                      loff_t * off)
{
    char value[10];
    int len = size;
    u32 tmp, tmp2;
    struct lcd200_dev_info *info = p_dev_info;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';

    /* read value */
    sscanf(value, "%u\n", &tmp);
    tmp2 = info->ops->reg_read((struct ffb_dev_info *)info, FEATURE);
    if (tmp > 0)
        tmp2 |= (0x1 << 14);
    else
        tmp2 &= ~(0x1 << 14);

    info->ops->reg_write((struct ffb_dev_info *)info, FEATURE, tmp2, (0x1 << 14));

    return size;
}

static int pattern_gen_show(struct seq_file *sfile, void *v)
{
    u32 tmp;
    struct lcd200_dev_info *info = p_dev_info;

    seq_printf(sfile, "0: disabled, 1: enabled \n");

    tmp = info->ops->reg_read((struct ffb_dev_info *)info, FEATURE);

    if (tmp & (0x1 << 14))
        seq_printf(sfile, "Pattern Generation enabled. \n");
    else
        seq_printf(sfile, "Pattern Generation disabled. \n");

    return 0;
}

static int pattern_gen_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, pattern_gen_show, NULL);
}

/* Pattern Generator */
static struct file_operations pattern_gen_proc_ops = {
    .owner = THIS_MODULE,
    .open = pattern_gen_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = pattern_gen_proc_write,
};

/*
 * enable/disable lcd controller
 */
static int lcd_switch_show(struct seq_file *sfile, void *v)
{
    if (sfile || v) {
    }

    return 0;
}

static int lcd_switch_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, lcd_switch_show, NULL);
}

static ssize_t lcd_switch_proc_write(struct file *file, const char __user * buf, size_t size,
                                     loff_t * off)
{
    char value[10];
    int len = size;
    u32 tmp, tmp2;
    volatile u32 i;

    struct lcd200_dev_info *info = p_dev_info;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';

    /* read value */
    sscanf(value, "%u\n", &tmp);
    if (tmp > 0) {
        tmp2 = info->ops->reg_read((struct ffb_dev_info *)info, FEATURE);
        if (tmp2 & 0x1) {
            //tmp2 &= ~0x1;
            //info->ops->reg_write((struct ffb_dev_info*)info, FEATURE, tmp2, 0x1);
            LCD200_Ctrl((struct ffb_dev_info *)info, 0);
            for (i = 0; i < 0x10000; i++) {
            }
            //tmp2 |= 0x1;
            //info->ops->reg_write((struct ffb_dev_info*)info, FEATURE, tmp2, 0x1);
            LCD200_Ctrl((struct ffb_dev_info *)info, 1);
        }
    }

    return size;
}

/* enable/disable lcd controller */
static struct file_operations lcd_switch_proc_ops = {
    .owner = THIS_MODULE,
    .open = lcd_switch_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = lcd_switch_proc_write,
};

/*
 * TV screen position adjustment
 */
static int cvbs_screen_sz_show(struct seq_file *sfile, void *v)
{
    struct ffb_timing_info *timing;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)p_dev_info;

    timing = get_output_timing(dev_info->output_info, dev_info->video_output_type);
    printk("\n");
    printk("cvbs screen xres = %d \n", timing->data.ccir656.out.xres);
    printk("cvbs screen yres = %d \n", timing->data.ccir656.out.yres);
    return 0;
}

static int cvbs_screen_sz_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, cvbs_screen_sz_show, NULL);
}

static ssize_t cvbs_screen_sz_proc_write(struct file *file, const char __user * buf, size_t size,
                                         loff_t * off)
{
    char value[10];
    int len = size;
    u32 x, y;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)p_dev_info;
    struct ffb_timing_info *timing;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';

    /* read value */
    sscanf(value, "%u %u\n", &x, &y);
    if (x % 16) {
        printk("\n xres must be multiple of 16! \n");
        goto exit;
    }
    if (y % 4) {
        printk("\n yres must be multiple of 4! \n");
        goto exit;
    }

    timing = get_output_timing(dev_info->output_info, dev_info->video_output_type);
    if ((dev_info->video_output_type == VOT_PAL) || (dev_info->video_output_type == VOT_NTSC)) {
        /* only for NTSC or PAL system */
        if ((x <= timing->data.ccir656.in.xres) && (y <= timing->data.ccir656.in.yres)) {
            timing->data.ccir656.out.xres = x;
            timing->data.ccir656.out.yres = y;
            dev_set_par(dev_info);
        } else {
            printk("Invalid value! \n");
            goto exit;
        }
    } else {
        printk("Not NTSC or PAL! \n");
    }

  exit:
    return size;
}

/* cvbs screen size
 */
static struct file_operations cvbs_screen_sz_proc_ops = {
    .owner = THIS_MODULE,
    .open = cvbs_screen_sz_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = cvbs_screen_sz_proc_write,
};

/*
 * TV screen position adjustment
 */
extern int horizon_shift;
extern int vertical_shift;

static int cvbs_screen_pos_show(struct seq_file *sfile, void *v)
{
    struct ffb_timing_info *timing;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)p_dev_info;

    timing = get_output_timing(dev_info->output_info, dev_info->video_output_type);
    printk("\n");
    printk("Positive value means Shift Up or Left. Otherwise Shift Down or Right. \n");
    printk("screen horizon shift = %d \n", horizon_shift);
    printk("screen vertical shift = %d \n", vertical_shift);
    return 0;
}

static int cvbs_screen_pos_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, cvbs_screen_pos_show, NULL);
}

static ssize_t cvbs_screen_pos_proc_write(struct file *file, const char __user * buf, size_t size,
                                          loff_t * off)
{
    char value[10];
    int len = size;
    int h, v;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)p_dev_info;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';

    /* read value */
    sscanf(value, "%d %d\n", &h, &v);

    if (h & 0x3) {
        /* Due to axi bus, we have 8 bytes alignment */
        printk("Error, the horizon shift must be 4 alignment! \n");
        return size;
    }

    if ((dev_info->video_output_type == VOT_PAL) || (dev_info->video_output_type == VOT_NTSC)) {
        horizon_shift = h;
        vertical_shift = v;
    } else {
        printk("Not NTSC or PAL! \n");
    }

    return size;
}

/*
 * cvbs screen position
 */
static struct file_operations cvbs_screen_pos_proc_ops = {
    .owner = THIS_MODULE,
    .open = cvbs_screen_pos_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = cvbs_screen_pos_proc_write,
};

static int dac_onoff_show(struct seq_file *sfile, void *v)
{
    int status;
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)p_dev_info;

    if (platform_get_dac_onoff(dev_info->lcd200_idx, &status)) {
        seq_printf(sfile, "error! \n");
        return 0;
    }

    seq_printf(sfile, "0 for OFF, 1 for ON \n");
    seq_printf(sfile, "current value = %d \n", status);

    return 0;
}

static int dac_onoff_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, dac_onoff_show, NULL);
}

static ssize_t dac_onoff_proc_write(struct file *file, const char __user * buf, size_t size,
                                          loff_t * off)
{
    char value[10];
    int len = size, dac_onoff;
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)p_dev_info;

    if (copy_from_user(value, buf, len))
        return 0;

    value[len] = '\0';

    /* read value */
    sscanf(value, "%d\n", &dac_onoff);
    if (dac_onoff > 1) {
        printk("out of range! Please input 0 or 1 \n");
        return 0;
    }

    if (platform_set_dac_onoff(dev_info->lcd200_idx, dac_onoff)) {
        printk("write error! \n");
        return 0;
    }

    return size;
}

/*
 * screen on/off
 */
static struct file_operations dac_onoff_proc_ops = {
    .owner = THIS_MODULE,
    .open = dac_onoff_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = dac_onoff_proc_write,
};

/* dev_proc_remove
 */
static void dev_proc_remove(void)
{
    struct lcd200_dev_info *info = p_dev_info;
    void *parent;

    if (p_dev_info != NULL)
        parent = (void *)info->lcd200_idx;
    else
        return;                 // means dev_construct() not success!

    /* saturation */
    if (saturation_proc != NULL)
        ffb_remove_proc_entry(parent, saturation_proc);
    /* brightness */
    if (brightness_proc != NULL)
        ffb_remove_proc_entry(parent, brightness_proc);
    /* hue */
    if (hue_proc != NULL)
        ffb_remove_proc_entry(parent, hue_proc);
    /* sharpness */
    if (sharpness_proc != NULL)
        ffb_remove_proc_entry(parent, sharpness_proc);
    /* contrast */
    if (contrast_proc != NULL)
        ffb_remove_proc_entry(parent, contrast_proc);
    /* register */
    if (register_proc != NULL)
        ffb_remove_proc_entry(parent, register_proc);
    /* lcd input selection */
    if (dac_inputsel_proc != NULL)
        ffb_remove_proc_entry(parent, dac_inputsel_proc);
    /* pattern generation */
    if (pattern_gen_proc != NULL)
        ffb_remove_proc_entry(parent, pattern_gen_proc);
    /* enable/disalbe LCD controller */
    if (lcd_switch_proc != NULL)
        ffb_remove_proc_entry(parent, lcd_switch_proc);
    /* tv screen adjustmemnt */
    if (cvbs_screen_sz_proc != NULL)
        ffb_remove_proc_entry(parent, cvbs_screen_sz_proc);
    /* tv screen position shift */
    if (cvbs_screen_pos_proc != NULL)
        ffb_remove_proc_entry(parent, cvbs_screen_pos_proc);
    /* mouse position scaling */
    if (mouse_posscale_proc != NULL)
        ffb_remove_proc_entry(parent, mouse_posscale_proc);
    /* dac on/off */
    if (dac_onoff_proc != NULL)
        ffb_remove_proc_entry(parent, dac_onoff_proc);

    return;
}

static int dev_proc_init(void)
{
    int ret = 0;
    struct lcd200_dev_info *info = p_dev_info;
    struct proc_dir_entry *parent = (struct proc_dir_entry *)(info->lcd200_idx);

    /* The folowings are for LCD Image Color Management
     */
    /* Saturation */
    saturation_proc = ffb_create_proc_entry("saturation", S_IRUGO | S_IXUGO, parent);   /* array in its parent */
    if (saturation_proc == NULL) {
        err("Fail to create saturation proc mode!\n");
        ret = -EINVAL;
        goto fail;
    }
    saturation_proc->proc_fops = &saturation_proc_ops;

    /* Brightness */
    brightness_proc = ffb_create_proc_entry("brightness", S_IRUGO | S_IXUGO, parent);
    if (brightness_proc == NULL) {
        err("Fail to create brightness proc mode!\n");
        ret = -EINVAL;
        goto fail;
    }
    brightness_proc->proc_fops = &brightness_proc_ops;

    /* hue */
    hue_proc = ffb_create_proc_entry("hue", S_IRUGO | S_IXUGO, parent);
    if (hue_proc == NULL) {
        err("Fail to create hue proc mode!\n");
        ret = -EINVAL;
        goto fail;
    }
    hue_proc->proc_fops = &hue_proc_ops;

    /* Sharpness */
    sharpness_proc = ffb_create_proc_entry("sharpness", S_IRUGO | S_IXUGO, parent);
    if (sharpness_proc == NULL) {
        err("Fail to create sharpness proc mode!\n");
        ret = -EINVAL;
        goto fail;
    }
    sharpness_proc->proc_fops = &sharpness_proc_ops;

    /* Contrast */
    contrast_proc = ffb_create_proc_entry("contrast", S_IRUGO | S_IXUGO, parent);
    if (contrast_proc == NULL) {
        err("Fail to create contrast proc mode!\n");
        ret = -EINVAL;
        goto fail;
    }
    contrast_proc->proc_fops = &contrast_proc_ops;

    /* Register dump */
    register_proc = ffb_create_proc_entry("dump_reg", S_IRUGO | S_IXUGO, parent);
    if (register_proc == NULL) {
        err("Fail to create contrast proc mode!\n");
        ret = -EINVAL;
        goto fail;
    }
    register_proc->proc_fops = &register_proc_ops;

    /* DAC Input selection */
    dac_inputsel_proc = ffb_create_proc_entry("dac_input_sel", S_IRUGO | S_IXUGO, parent);
    if (dac_inputsel_proc == NULL) {
        err("Fail to create DAC input selection proc mode!\n");
        ret = -EINVAL;
        goto fail;
    }
    dac_inputsel_proc->proc_fops = &dac_inputsel_proc_ops;

    /* Pattern Generation */
    pattern_gen_proc = ffb_create_proc_entry("pattern_gen", S_IRUGO | S_IXUGO, parent);
    if (pattern_gen_proc == NULL) {
        err("Fail to create Pattern Generation proc!\n");
        ret = -EINVAL;
        goto fail;
    }
    pattern_gen_proc->proc_fops = &pattern_gen_proc_ops;

    /* enable/disalbe lcd controller
     */
    lcd_switch_proc = ffb_create_proc_entry("lcd_on_off", S_IRUGO | S_IXUGO, parent);
    if (lcd_switch_proc == NULL) {
        err("Fail to create lcd_switch_proc!\n");
        ret = -EINVAL;
        goto fail;
    }
    lcd_switch_proc->proc_fops = &lcd_switch_proc_ops;

    /* tv screen size adjust
     */
    cvbs_screen_sz_proc = ffb_create_proc_entry("cvbs_screen_sz", S_IRUGO | S_IXUGO, parent);
    if (cvbs_screen_sz_proc == NULL) {
        err("Fail to create cvbs_screen_sz_proc!\n");
        ret = -EINVAL;
        goto fail;
    }
    cvbs_screen_sz_proc->proc_fops = &cvbs_screen_sz_proc_ops;

    /* tv screen position adjust
     */
    cvbs_screen_pos_proc = ffb_create_proc_entry("cvbs_screen_pos", S_IRUGO | S_IXUGO, parent);
    if (cvbs_screen_pos_proc == NULL) {
        err("Fail to create cvbs_screen_pos_proc!\n");
        ret = -EINVAL;
        goto fail;
    }
    cvbs_screen_pos_proc->proc_fops = &cvbs_screen_pos_proc_ops;

    /*
     * Mouse position scaling
     */
    mouse_posscale_proc = ffb_create_proc_entry("mouse_pos_scale", S_IRUGO | S_IXUGO, parent);
    if (mouse_posscale_proc == NULL) {
        err("Fail to create mouse_posscale_proc!\n");
        ret = -EINVAL;
        goto fail;
    }
    mouse_posscale_proc->proc_fops = &mouse_posscale_proc_ops;

    /*
     * dac on/off
     */
    dac_onoff_proc = ffb_create_proc_entry("dac_onoff", S_IRUGO | S_IXUGO, parent);
    if (dac_onoff_proc == NULL) {
        err("Fail to create dac_onoff_proc!\n");
        ret = -EINVAL;
        goto fail;
    }
    dac_onoff_proc->proc_fops = &dac_onoff_proc_ops;

    return ret;

  fail:
    dev_proc_remove();
    return ret;
}
