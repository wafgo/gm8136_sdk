#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <mach/ftpmu010_pcie.h>
#include <asm/uaccess.h>
#include <cpu_comm/cpu_comm.h>
#include <lcd200_v3/lcd_fb.h>
#include "framebuffer.h"

#define CPU_COMM_LCD0    CHAN_0_USR6
#define CPU_COMM_LCD2    CHAN_0_USR7
#define CPU_COMM_HDMI    CHAN_0_USR4

#define LCD_SCREEN_ON   1
#define LCD_SCREEN_OFF  0

#define HDMI_CMD_SET_ONOFF      0x100
#define HDMI_CMD_GET_ONOFF      0x104
#define HDMI_CMD_REPLY_ONOFF    0x108

static struct proc_dir_entry *dac_onoff_proc[LCD_IP_NUM] = {NULL};
static struct proc_dir_entry *saturation_proc[LCD_IP_NUM] = {NULL};
static struct proc_dir_entry *brightness_proc[LCD_IP_NUM] = {NULL};
static struct proc_dir_entry *hue_proc[LCD_IP_NUM] = {NULL};
static struct proc_dir_entry *sharpness_proc[LCD_IP_NUM] = {NULL};
static struct proc_dir_entry *contrast_proc[LCD_IP_NUM] = {NULL};
static struct proc_dir_entry *pattern_gen_proc[LCD_IP_NUM] = {NULL};
static struct proc_dir_entry *pip_root[LCD_IP_NUM] = {NULL};
static struct proc_dir_entry *blend_param[LCD_IP_NUM] = {NULL};
static struct proc_dir_entry *pip_mode[LCD_IP_NUM] = {NULL};
static struct proc_dir_entry *input_res_proc[LCD_IP_NUM] = {NULL};
static struct proc_dir_entry *output_type_proc[LCD_IP_NUM] = {NULL};
static struct proc_dir_entry *flcd200_cmn_proc = NULL;
static struct proc_dir_entry *hdmi_onoff_proc = NULL;

#ifdef CONFIG_GM8312
extern int pcie_lcd_fd;
#endif

/*
 * Saturation proc related functions.
 */
static int saturation_show(struct seq_file *sfile, void *v)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)sfile->private;
    int value;

    if (v) {}

    /* avoid warning */
    value = ioread32(lcd_info->io_base + 0x400);
    value = (value >> 8) & 0x3F;

    seq_printf(sfile, "The saturation range is 0 ~ 63. \n");
    seq_printf(sfile, "Current saturation value is %d. \n", value);

    return 0;
}

static int saturation_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, saturation_show, PDE(inode)->data);
}

static ssize_t saturation_proc_write(struct file *file, const char __user * buf, size_t size,
                                     loff_t * off)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)((struct seq_file *)file->private_data)->private;
    int len = size;
    char value[20];
    int sat_value;

    if (copy_from_user(value, buf, len))
        return 0;

    value[len] = '\0';
    sscanf(value, "%u \n", &sat_value);
    if (sat_value > 63) {
        printk("Out of range! The valid range is 0 - 63! \n");
    } else {
        u32 value;

        value = ioread32(lcd_info->io_base + 0x400);
        value &= ~0x3F00;       /* maskout SatValue bit 13-8 */
        value |= (sat_value << 8);
        iowrite32(value, lcd_info->io_base + 0x400);
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
    lcd_info_t  *lcd_info = (lcd_info_t *)sfile->private;
    int value, tmp;

    if (v) {}  /* avoid warning */

    tmp = ioread32(lcd_info->io_base + 0x400) & 0xFF;
    value = tmp & 0x7F;

    if ((tmp >> 7) & 0x1)
        value *= -1;

    seq_printf(sfile, "The brightness range is -127 ~ +127. \n");
    seq_printf(sfile, "Current brightness value is %d. \n", value);

    return 0;
}

static int brightness_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, brightness_show, PDE(inode)->data);
}

static ssize_t brightness_proc_write(struct file *file, const char __user * buf, size_t size,
                                     loff_t * off)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)((struct seq_file *)file->private_data)->private;
    int len = size;
    char value[20];
    int brightness_value;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';
    /* read brightness value */
    sscanf(value, "%d\n", &brightness_value);
    if (abs(brightness_value) > 127) {
        printk("Out of range! The valid range is -127 ~ +127! \n");
    } else {
        u32 value;

        value = ioread32(lcd_info->io_base + 0x400);
        value &= ~0xFF;         /* maskout Brightness bit 0-7 */
        value |= abs(brightness_value);
        value |= ((brightness_value >= 0) ? (0x0 << 7) : (0x1 << 7));
        iowrite32(value, lcd_info->io_base + 0x400);
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
    lcd_info_t  *lcd_info = (lcd_info_t *)sfile->private;
    u32 tmp;
    int k1, k0, ShTh1, ShTh0;

    if (v) { } /* avoid warning */

    tmp = ioread32(lcd_info->io_base + 0x408);

    k1 = (tmp >> 20) & 0xF;
    k0 = (tmp >> 16) & 0xF;
    ShTh1 = (tmp >> 8) & 0xFF;
    ShTh0 = tmp & 0xFF;

    seq_printf(sfile,
               "Range for K0 and K1 is 0 - 15, range for threshold0 and threshold1 is 0 - 255 \n");
    seq_printf(sfile, "K0   K1   Threshold0   Threshold1 \n");
    seq_printf(sfile, "%-2d   %-2d   %-10d   %-10d \n", k0, k1, ShTh0, ShTh1);

    return 0;
}

static int sharpness_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, sharpness_show, PDE(inode)->data);
}

static ssize_t sharpness_proc_write(struct file *file, const char __user * buf, size_t size,
                                    loff_t * off)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)((struct seq_file *)file->private_data)->private;
    int len = size;
    char value[60];
    int k1, k0, ShTh1, ShTh0;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';
    /* read brightness value */
    sscanf(value, "%u %u %u %u\n", &k0, &k1, &ShTh0, &ShTh1);
    if (k0 > 15 || k1 > 15 || ShTh0 > 255 || ShTh1 > 255) {
        printk("Range for K0 and K1 is 0 - 15, range for threshold0 and threshold1 is 0 - 255 \n");
    } else {
        u32 value;

        value = k1 << 20 | k0 << 16 | ShTh1 << 8 | ShTh0;
        iowrite32(value, lcd_info->io_base + 0x408);
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
    lcd_info_t  *lcd_info = (lcd_info_t *)sfile->private;
    u32 value;

    if (v) { }  /* avoid warning */

    value = (ioread32(lcd_info->io_base + 0x40C) >> 16) & 0x1F;
    seq_printf(sfile, "The contrast slope range is 1 ~ 31. \n");
    seq_printf(sfile, "Current contrast slope is %d. \n", value);

    return 0;
}

static int contrast_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, contrast_show, PDE(inode)->data);
}

static ssize_t contrast_proc_write(struct file *file, const char __user * buf, size_t size,
                                   loff_t * off)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)((struct seq_file *)file->private_data)->private;
    int len = size;
    char value[20];
    int contrast_slope;

    if (copy_from_user(value, buf, len))
        return 0;

    value[len] = '\0';
    /* read contrast value */
    sscanf(value, "%u\n", &contrast_slope);
    if (contrast_slope == 0 || contrast_slope > 31) {
        printk("Out of range! The valid slope range is 1 - 31! \n");
    } else {
        u32 regVal = 0, sign, contr_offset;

        sign = (contrast_slope * 128) > 512 ? 1 : 0;
        contr_offset = abs(contrast_slope * 128 - 512) & 0xFFF;
        regVal = (contrast_slope << 16) | (sign << 12) | contr_offset;

        iowrite32(regVal, lcd_info->io_base + 0x40C);
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
    lcd_info_t  *lcd_info = (lcd_info_t *)sfile->private;
    u32 tmp;
    int HuCosValue, HuSinValue;

    if (v) {}  /* avoid warning */

    tmp = ioread32(lcd_info->io_base + 0x404);
    HuCosValue = (tmp >> 8) & 0x3F;
    HuCosValue = ((tmp >> 14) & 0x01) ? -HuCosValue : HuCosValue;

    HuSinValue = tmp & 0x3F;
    HuSinValue = ((tmp >> 6) & 0x01) ? -HuSinValue : HuSinValue;

    seq_printf(sfile, "Valid range for both HueCosValue and HueSinValue is -32 ~ +32. \n");
    seq_printf(sfile, "Cos@ = (HueCosValue / 32). @ is the rotating degreee from -180 ~ 180. \n");
    seq_printf(sfile, "Sin@ = (HueSinValue / 32). @ is the rotating degreee from -180 ~ 180. \n");
    seq_printf(sfile, "Current HueCosValue = %d, HueSinValue = %d \n", HuCosValue, HuSinValue);

    return 0;
}

static int hue_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, hue_show, PDE(inode)->data);
}

static ssize_t hue_proc_write(struct file *file, const char __user * buf, size_t size, loff_t * off)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)((struct seq_file *)file->private_data)->private;
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

        iowrite32(regVal, lcd_info->io_base + 0x404);
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

/*
 * Pattern Generation
 */
static ssize_t pattern_gen_proc_write(struct file *file, const char __user * buf, size_t size,
                                      loff_t * off)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)((struct seq_file *)file->private_data)->private;
    char value[10];
    int len = size;
    u32 tmp, tmp2;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';

    /* read value */
    sscanf(value, "%u\n", &tmp);
    tmp2 = ioread32(lcd_info->io_base);

    if (tmp > 0)
        tmp2 |= (0x1 << 14);
    else
        tmp2 &= ~(0x1 << 14);

    iowrite32(tmp2, lcd_info->io_base + 0x0);

    return size;
}

static int pattern_gen_show(struct seq_file *sfile, void *v)
{
    u32 tmp;
    lcd_info_t  *lcd_info = (lcd_info_t *)sfile->private;

    seq_printf(sfile, "0: disabled, 1: enabled \n");

    tmp = ioread32(lcd_info->io_base);

    if (tmp & (0x1 << 14))
        seq_printf(sfile, "Pattern Generation enabled. \n");
    else
        seq_printf(sfile, "Pattern Generation disabled. \n");

    return 0;
}

static int pattern_gen_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, pattern_gen_show, PDE(inode)->data);
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

static int pip_mode_show(struct seq_file *sfile, void *v)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)sfile->private;
    u32 value, blend, pip;
    int ret = 0;

    value = ioread32(lcd_info->io_base + 0x0);
    blend = (value >> 8) & 0x3;
    pip = (value >> 10) & 0x3;

    seq_printf(sfile, "PIP:0-Disable 1-Single PIP Win 2-Double PIP Win\n");
    seq_printf(sfile, "PIP=%d (Blend=%d)\n", pip, blend);
    return ret;
}

static int pip_mode_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, pip_mode_show, PDE(inode)->data);
}

static ssize_t pip_mode_proc_write(struct file *file, const char __user * buf, size_t size,
                                   loff_t * off)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)((struct seq_file *)file->private_data)->private;
    cpu_comm_msg_t      msg;
    int len = size;
    unsigned int command[3];
    unsigned char value[60];
    u32 v0;
    if (copy_from_user(value, buf, len))
        return 0;

    value[len] = '\0';
    sscanf(value, "%u\n", &v0);
    if (v0 > 2) {
        printk("Error value: %d \n", v0);
        return size;
    }

    command[0] = FFB_PIP_MODE;
    command[1] = sizeof(u32);
    command[2] = v0;

    memset(&msg, 0, sizeof(msg));
    msg.target = (lcd_info->index == 0) ? CPU_COMM_LCD0 : CPU_COMM_LCD2;
    msg.length = sizeof(command);
    msg.msg_data = (unsigned char *)&command[0];
    if (cpu_comm_msg_snd(&msg, -1))
        panic("LCD: Error to send command1! \n");

    return size;
}

/* PIP mode
 */
static struct file_operations pip_mode_proc_ops = {
    .owner = THIS_MODULE,
    .open = pip_mode_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = pip_mode_proc_write,
};

/* get input_res and output_type from the FA626 */
static u32 __get_resolution_value(lcd_info_t  *lcd_info)
{
    cpu_comm_msg_t  msg;
    unsigned int command, value;

    command = FFB_GET_INPUTRES_OUTPUTTYPE;
    memset(&msg, 0, sizeof(msg));
    msg.target = (lcd_info->index == 0) ? CPU_COMM_LCD0 : CPU_COMM_LCD2;
    msg.length = sizeof(command);
    msg.msg_data = (unsigned char *)&command;
    if (cpu_comm_msg_snd(&msg, -1))
        panic("LCD Fail to send data to cpucomm \n");

    msg.length = sizeof(u32);
    msg.msg_data = (unsigned char *)&value;
    cpu_comm_msg_rcv(&msg, -1);

    return value;
}

/* input_res
 */
static int input_res_show(struct seq_file *sfile, void *v)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)sfile->private;
    unsigned int input_res_val, val;

    val = __get_resolution_value(lcd_info);

    input_res_val = val & 0xFFFF;

    seq_printf(sfile, "input resolution value: %d \n", input_res_val);
    return 0;
}

static int input_res_open(struct inode *inode, struct file *file)
{
    return single_open(file, input_res_show, PDE(inode)->data);
}

static ssize_t input_res_write(struct file *file, const char __user * buf, size_t size,
                               loff_t * off)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)((struct seq_file *)file->private_data)->private;
    int len = size;
    unsigned char value[20];
    int input_res_tmp, l_output_type;
    int output_type, input_res;
    cpu_comm_msg_t  msg;
    unsigned int command[3], tmp;

    if (copy_from_user(value, buf, len))
        return 0;

    value[len] = '\0';
    sscanf(value, "%d", &input_res_tmp);

    tmp = __get_resolution_value(lcd_info);
    output_type = tmp >> 16;
    input_res = tmp & 0xFFFF;

    l_output_type = output_type;
    if (input_res_tmp == VIN_NTSC) {
        switch (l_output_type) {
        case OUTPUT_TYPE_PAL_1:
            l_output_type = OUTPUT_TYPE_NTSC_0;
            break;
        case OUTPUT_TYPE_CAT6611_576P_21:
            l_output_type = OUTPUT_TYPE_CAT6611_480P_20;
            break;
        case OUTPUT_TYPE_SLI10121_576P_43:
            l_output_type = OUTPUT_TYPE_SLI10121_480P_42;
            break;
        default:
            break;
        }
    }

    if (input_res_tmp == VIN_PAL) {
        switch (l_output_type) {
        case OUTPUT_TYPE_NTSC_0:
            l_output_type = OUTPUT_TYPE_PAL_1;
            break;
        case OUTPUT_TYPE_CAT6611_480P_20:
            l_output_type = OUTPUT_TYPE_CAT6611_576P_21;
            break;
        case OUTPUT_TYPE_SLI10121_480P_42:
            l_output_type = OUTPUT_TYPE_SLI10121_576P_43;
            break;
        default:
            break;
        }
    }

    /* send to FA626 */
     /* format: command(4), length of body(4), data(length) */
    command[0] = FFB_PIP_INPUT_RES;
    command[1] = sizeof(int);
    command[2] = (l_output_type << 16) | input_res_tmp;

    memset(&msg, 0, sizeof(msg));
    msg.target = (lcd_info->index == 0) ? CPU_COMM_LCD0 : CPU_COMM_LCD2;
    msg.length = sizeof(command);
    msg.msg_data = (unsigned char *)&command[0];
    if (cpu_comm_msg_snd(&msg, -1))
        panic("LCD: Error to send input_res command1! \n");

    return size;
}

/* input_res
 */
static struct file_operations input_res_ops = {
    .owner = THIS_MODULE,
    .open = input_res_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = input_res_write,
};

/* output_type
 */
static int output_type_show(struct seq_file *sfile, void *v)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)sfile->private;
    unsigned int output_type_val, val;

    val = __get_resolution_value(lcd_info);

    output_type_val = (val >> 16) & 0xFFFF;

    seq_printf(sfile, "output type value: %d \n", output_type_val);
    return 0;
}

static int output_type_open(struct inode *inode, struct file *file)
{
    return single_open(file, output_type_show, PDE(inode)->data);
}

static ssize_t output_type_write(struct file *file, const char __user * buf, size_t size,
                                 loff_t * off)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)((struct seq_file *)file->private_data)->private;
    int len = size;
    unsigned char value[20];
    int tmp, l_input_res, ouput_type_tmp;
    unsigned int val;
    cpu_comm_msg_t  msg;
    unsigned int command[3];

    if (copy_from_user(value, buf, len))
        return 0;

    val = __get_resolution_value(lcd_info);

    value[len] = '\0';
    sscanf(value, "%d", &ouput_type_tmp);

    tmp = l_input_res = val & 0xFFFF;
    switch (ouput_type_tmp) {
    case OUTPUT_TYPE_NTSC_0:
    case OUTPUT_TYPE_CAT6611_480P_20:
    case OUTPUT_TYPE_SLI10121_480P_42:
        l_input_res = (l_input_res == VIN_PAL) ? VIN_NTSC : l_input_res;
        break;
    case OUTPUT_TYPE_PAL_1:
    case OUTPUT_TYPE_CAT6611_576P_21:
    case OUTPUT_TYPE_SLI10121_576P_43:
        l_input_res = (l_input_res == VIN_NTSC) ? VIN_PAL : l_input_res;
        break;
    default:
        break;
    }

    /* send to FA626 */
     /* format: command(4), length of body(4), data(length) */
    command[0] = FFB_PIP_OUTPUT_TYPE;
    command[1] = sizeof(int);
    command[2] = (ouput_type_tmp << 16) | l_input_res;

    memset(&msg, 0, sizeof(msg));
    msg.target = (lcd_info->index == 0) ? CPU_COMM_LCD0 : CPU_COMM_LCD2;
    msg.length = sizeof(command);
    msg.msg_data = (unsigned char *)&command[0];
    if (cpu_comm_msg_snd(&msg, -1))
        panic("LCD: Error to send output_type command1! \n");

    return size;
}

static struct file_operations output_type_ops = {
    .owner = THIS_MODULE,
    .open = output_type_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = output_type_write,
};

static int blend_show(struct seq_file *sfile, void *v)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)sfile->private;
    int BlendH, BlendL;
    u32 Priority, blend;

    Priority = ioread32(lcd_info->io_base + 0x314);
    seq_printf(sfile, "Blend_H(0-255) Blend_L(0-255) Img0_P Img1_P Img2_P\n");
    blend = ioread32(lcd_info->io_base + 0x300);
    BlendH = (blend >> 8) & 0xFF;
    BlendL = blend & 0xFF;
    seq_printf(sfile, "%-14d %-14d %-6d %-6d %-6d\n", BlendH, BlendL,
               Priority & 0x3, (Priority >> 2) & 0x3, (Priority >> 4) & 0x3);

    return 0;
}

static int blend_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, blend_show, PDE(inode)->data);
}

#define check_blend_value(v)  ((v>0xff)?(0xff):v)
#define check_priority_value(v)  ((v>2)?(2):v)
static ssize_t blend_proc_write(struct file *file, const char __user * buf, size_t size,
                                loff_t * off)
{
    int len = size;
    unsigned char value[60];
    int bv0, bv1, pv0, pv1, pv2;
    lcd_info_t  *lcd_info = (lcd_info_t *)((struct seq_file *)file->private_data)->private;
    u32 Priority, tmp, blend;

    if (copy_from_user(value, buf, len))
        return 0;

    Priority = ioread32(lcd_info->io_base + 0x314);
    pv0 = Priority & 0x3;
    pv1 = (Priority >> 2) & 0x3;
    pv2 = (Priority >> 4) & 0x3;

    tmp = ioread32(lcd_info->io_base + 0x300);
    bv0 = (tmp >> 8) & 0xff;
    bv1 = tmp & 0xff;

    value[len] = '\0';
    sscanf(value, "%u %u %u %u %u\n", &bv0, &bv1, &pv0, &pv1, &pv2);

    bv0 = check_blend_value(bv0);
    bv1 = check_blend_value(bv1);
    pv0 = check_priority_value(pv0);
    pv1 = check_priority_value(pv1);
    pv2 = check_priority_value(pv2);

    Priority = ((pv2 << 4) | (pv1 << 2) | pv0) | (Priority & (0x3 << 6));
    iowrite32(Priority, lcd_info->io_base + 0x314);

    blend = (bv0 << 8) | bv1 | (tmp & (0xFF << 16));
    iowrite32(blend, lcd_info->io_base + 0x300);

    return size;
}

/* Blending
 */
static struct file_operations blend_proc_ops = {
    .owner = THIS_MODULE,
    .open = blend_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = blend_proc_write,
};


/* Proc function
 */
struct proc_dir_entry *ffb_create_proc_entry(const char *name,  mode_t mode, struct proc_dir_entry *parent)
{
    struct proc_dir_entry *p;

    p = create_proc_entry(name, mode, parent);

    return p;
}

void ffb_remove_proc_entry(struct proc_dir_entry *parent, struct proc_dir_entry *de)
{
    remove_proc_entry(de->name, parent);
}

void pip_proc_init(lcd_info_t *lcd_info)
{
    struct proc_dir_entry *root;

    root = ffb_create_proc_entry("pip", S_IFDIR | S_IRUGO | S_IXUGO, lcd_info->ffb_proc_root);
    if (root == NULL)
        panic("%s %d, fail! \n", __func__, lcd_info->index);

    /* blend
     */
    blend_param[lcd_info->index] = ffb_create_proc_entry("blend", S_IRUGO | S_IXUGO, root);
    if (blend_param[lcd_info->index] == NULL)
        panic("Fail to create proc mode!\n");

    blend_param[lcd_info->index]->proc_fops = &blend_proc_ops;
    blend_param[lcd_info->index]->data = (void *)lcd_info;

    /* pip_mode
     */
    pip_mode[lcd_info->index] = ffb_create_proc_entry("pip_mode", S_IRUGO | S_IXUGO, root);
    if (pip_mode[lcd_info->index] == NULL)
        panic("Fail to create proc mode!\n");

    pip_mode[lcd_info->index]->proc_fops = &pip_mode_proc_ops;
    pip_mode[lcd_info->index]->data = (void *)lcd_info;

    /* input_res resoultion proc.
     */
    input_res_proc[lcd_info->index] = ffb_create_proc_entry("input_res", S_IRUGO | S_IXUGO, root);
    if (input_res_proc[lcd_info->index] == NULL)
        panic("Fail to create input_res_proc!\n");

    input_res_proc[lcd_info->index]->proc_fops = &input_res_ops;
    input_res_proc[lcd_info->index]->data = (void *)lcd_info;

    /* output_typeresoultion proc.
     */
    output_type_proc[lcd_info->index] = ffb_create_proc_entry("output_type", S_IRUGO | S_IXUGO, root);
    if (output_type_proc[lcd_info->index] == NULL)
        panic("Fail to create output_type_proc!\n");

    output_type_proc[lcd_info->index]->proc_fops = &output_type_ops;
    output_type_proc[lcd_info->index]->data = (void *)lcd_info;

    /* keep the root */
    pip_root[lcd_info->index] = root;
}

/*
 * hdmi on/off
 */
static ssize_t hdmi_onoff_proc_write(struct file *file, const char __user * buf, size_t size,
                                      loff_t * off)
{
    u32 command[4];
    cpu_comm_msg_t  msg;
    int len = size;
	unsigned char value[10];
	int data;

	if(copy_from_user(value, buf, len))
		return 0;

	value[len] = '\0';
    sscanf(value, "%d\n", &data);
    if (data > 1) {
        printk("out of range! Please input 0 or 1 \n");
        return size;
    }

    printk("set value: %d \n", data);

    memset(&msg, 0, sizeof(cpu_comm_msg_t));
    msg.target = CPU_COMM_HDMI;
    msg.length = sizeof(command);
    msg.msg_data = (unsigned char *)&command[0];

    command[0] = HDMI_CMD_SET_ONOFF;
    command[1] = 4;
    command[2] = data;
    cpu_comm_msg_snd(&msg, -1);

    return size;
}

static int hdmi_onoff_show(struct seq_file *sfile, void *v)
{
    u32 command[4], value;
    cpu_comm_msg_t  msg;

    memset(&msg, 0, sizeof(cpu_comm_msg_t));
    msg.target = CPU_COMM_HDMI;
    msg.length = sizeof(command);
    msg.msg_data = (unsigned char *)&command[0];
    command[0] = HDMI_CMD_GET_ONOFF;
    command[1] = 4; /* length */
    cpu_comm_msg_snd(&msg, -1);
    cpu_comm_msg_rcv(&msg, -1);
    value = command[2];
    seq_printf(sfile, "0 for OFF, 1 for ON \n");
    seq_printf(sfile, "current value = %d \n", value);

    return 0;
}

static int hdmi_onoff_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, hdmi_onoff_show, PDE(inode)->data);
}

/* hdmi on/off */
static struct file_operations hdmi_onoff_proc_ops = {
    .owner = THIS_MODULE,
    .open = hdmi_onoff_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = hdmi_onoff_proc_write,
};

/*
 * dac on/off
 */
static ssize_t dac_onoff_proc_write(struct file *file, const char __user * buf, size_t size,
                                      loff_t * off)
{
    char value[10];
    u32 mask, val;
    lcd_info_t  *lcd_info = (lcd_info_t *)((struct seq_file *)file->private_data)->private;
    int len = size, dac_onoff;

    if (copy_from_user(value, buf, len))
        return 0;

    value[len] = '\0';

    /* read value */
    sscanf(value, "%d\n", &dac_onoff);
    if (dac_onoff > 1) {
        printk("out of range! Please input 0 or 1 \n");
        return 0;
    }

    switch (lcd_info->index) {
      case 0:
        mask = 1 << 21;
        val = 1 << 21;
        break;
      case 1:
        mask = 1 << 19;
        val = 1 << 19;
        break;
      case 2:
        mask = 1 << 17;
        val = 1 << 17;
        break;
      default:
        return size;
        break;
    }

    if (dac_onoff == LCD_SCREEN_ON) /* ON */
        val = 0;

    ftpmu010_pcie_write_reg(pcie_lcd_fd, 0x44, val, mask);

    printk("set value = %d \n", dac_onoff);
    return size;
}

static int dac_onoff_show(struct seq_file *sfile, void *v)
{
    lcd_info_t  *lcd_info = (lcd_info_t *)sfile->private;
    u32 tmp, value = LCD_SCREEN_ON;

    tmp = ftpmu010_pcie_read_reg(0x44);
    switch (lcd_info->index) {
      case 0:
        if (tmp & (1 << 21))
            value = LCD_SCREEN_OFF;
        break;
      case 1:
        if (tmp & (1 << 19))
            value = LCD_SCREEN_OFF;
        break;
      case 2:
        if (tmp & (1 << 17))
            value = LCD_SCREEN_OFF;
        break;
    }

    seq_printf(sfile, "0 for OFF, 1 for ON \n");
    seq_printf(sfile, "current value = %d \n", value);

    return 0;
}

static int dac_onoff_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, dac_onoff_show, PDE(inode)->data);
}

/* DAC on/off */
static struct file_operations dac_onoff_proc_ops = {
    .owner = THIS_MODULE,
    .open = dac_onoff_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = dac_onoff_proc_write,
};

void ffb_proc_init(lcd_info_t *lcd_info)
{
    struct proc_dir_entry *root;

    root = create_proc_entry(lcd_info->name, S_IFDIR | S_IRUGO | S_IXUGO, NULL);     /* /proc */
    if (root == NULL)
        panic("%s %d, fail! \n", __func__, lcd_info->index);

    lcd_info->ffb_proc_root = root;

    /* dac on/off */
    dac_onoff_proc[lcd_info->index] = ffb_create_proc_entry("dac_onoff", S_IRUGO | S_IXUGO, root);
    if (dac_onoff_proc[lcd_info->index] == NULL)
        panic("Fail to create dac_onoff_proc proc mode!\n");
    dac_onoff_proc[lcd_info->index]->proc_fops = &dac_onoff_proc_ops;
    dac_onoff_proc[lcd_info->index]->data = (void *)lcd_info;

    /* Saturation */
    saturation_proc[lcd_info->index] = ffb_create_proc_entry("saturation", S_IRUGO | S_IXUGO, root);
    if (saturation_proc[lcd_info->index] == NULL)
        panic("Fail to create saturation proc mode!\n");
    saturation_proc[lcd_info->index]->proc_fops = &saturation_proc_ops;
    saturation_proc[lcd_info->index]->data = (void *)lcd_info;

    /* Brightness */
    brightness_proc[lcd_info->index] = ffb_create_proc_entry("brightness", S_IRUGO | S_IXUGO, root);
    if (brightness_proc[lcd_info->index] == NULL)
        panic("Fail to create brightness proc mode!\n");

    brightness_proc[lcd_info->index]->proc_fops = &brightness_proc_ops;
    brightness_proc[lcd_info->index]->data = (void *)lcd_info;

    /* hue */
    hue_proc[lcd_info->index] = ffb_create_proc_entry("hue", S_IRUGO | S_IXUGO, root);
    if (hue_proc[lcd_info->index] == NULL)
        panic("Fail to create hue proc mode!\n");

    hue_proc[lcd_info->index]->proc_fops = &hue_proc_ops;
    hue_proc[lcd_info->index]->data = (void *)lcd_info;

    /* Sharpness */
    sharpness_proc[lcd_info->index] = ffb_create_proc_entry("sharpness", S_IRUGO | S_IXUGO, root);
    if (sharpness_proc[lcd_info->index] == NULL)
        panic("Fail to create sharpness proc mode!\n");

    sharpness_proc[lcd_info->index]->proc_fops = &sharpness_proc_ops;
    sharpness_proc[lcd_info->index]->data = (void *)lcd_info;

    /* Contrast */
    contrast_proc[lcd_info->index] = ffb_create_proc_entry("contrast", S_IRUGO | S_IXUGO, root);
    if (contrast_proc[lcd_info->index] == NULL)
        panic("Fail to create contrast proc mode!\n");

    contrast_proc[lcd_info->index]->proc_fops = &contrast_proc_ops;
    contrast_proc[lcd_info->index]->data = (void *)lcd_info;

    /* Pattern Generation */
    pattern_gen_proc[lcd_info->index] = ffb_create_proc_entry("pattern_gen", S_IRUGO | S_IXUGO, root);
    if (pattern_gen_proc[lcd_info->index] == NULL)
        panic("Fail to create Pattern Generation proc!\n");

    pattern_gen_proc[lcd_info->index]->proc_fops = &pattern_gen_proc_ops;
    pattern_gen_proc[lcd_info->index]->data = (void *)lcd_info;

    pip_proc_init(lcd_info);

    /* flcd200_cmn_proc */
    if (flcd200_cmn_proc)
        return;

    flcd200_cmn_proc = create_proc_entry("flcd200_cmn", S_IFDIR | S_IRUGO | S_IXUGO, NULL);     /* /proc */
    if (flcd200_cmn_proc == NULL)
        panic("%s 222, fail! \n", __func__);

    hdmi_onoff_proc = ffb_create_proc_entry("hdmi_onoff", S_IRUGO | S_IXUGO, flcd200_cmn_proc);
    if (hdmi_onoff_proc == NULL)
        panic("%s 333, fail! \n", __func__);

    hdmi_onoff_proc->proc_fops = &hdmi_onoff_proc_ops;
    hdmi_onoff_proc->data = (void *)lcd_info;

    if (cpu_comm_open(CPU_COMM_HDMI, "hdmi"))
        panic("%s, open cpucomm channel %d fail! \n", __func__, CPU_COMM_HDMI);
}

void ffb_proc_release(lcd_info_t *lcd_info)
{
    struct proc_dir_entry *parent = lcd_info->ffb_proc_root;

    /* dac on/off */
    if (dac_onoff_proc[lcd_info->index] != NULL)
        ffb_remove_proc_entry(parent, dac_onoff_proc[lcd_info->index]);
    /* saturation */
    if (saturation_proc[lcd_info->index] != NULL)
        ffb_remove_proc_entry(parent, saturation_proc[lcd_info->index]);
    /* brightness */
    if (brightness_proc[lcd_info->index] != NULL)
        ffb_remove_proc_entry(parent, brightness_proc[lcd_info->index]);
    /* hue */
    if (hue_proc[lcd_info->index] != NULL)
        ffb_remove_proc_entry(parent, hue_proc[lcd_info->index]);
    /* sharpness */
    if (sharpness_proc[lcd_info->index] != NULL)
        ffb_remove_proc_entry(parent, sharpness_proc[lcd_info->index]);
    /* contrast */
    if (contrast_proc[lcd_info->index] != NULL)
        ffb_remove_proc_entry(parent, contrast_proc[lcd_info->index]);
    /* pattern generation */
    if (pattern_gen_proc[lcd_info->index] != NULL)
        ffb_remove_proc_entry(parent, pattern_gen_proc[lcd_info->index]);

    if (pip_root[lcd_info->index] != NULL) {
        struct proc_dir_entry *pip_parent = pip_root[lcd_info->index];

        if (blend_param[lcd_info->index] != NULL)
            ffb_remove_proc_entry(pip_parent, blend_param[lcd_info->index]);

        if (pip_mode[lcd_info->index] != NULL)
            ffb_remove_proc_entry(pip_parent, pip_mode[lcd_info->index]);

        if (input_res_proc[lcd_info->index] != NULL)
            ffb_remove_proc_entry(pip_parent, input_res_proc[lcd_info->index]);

        if (output_type_proc[lcd_info->index] != NULL)
            ffb_remove_proc_entry(pip_parent, output_type_proc[lcd_info->index]);

        ffb_remove_proc_entry(parent, pip_root[lcd_info->index]);
    }

    remove_proc_entry(lcd_info->ffb_proc_root->name, NULL);

    lcd_info->ffb_proc_root = NULL;

    if (hdmi_onoff_proc != NULL) {
        ffb_remove_proc_entry(flcd200_cmn_proc, hdmi_onoff_proc);
        hdmi_onoff_proc = NULL;

        remove_proc_entry(flcd200_cmn_proc->name, NULL);
        flcd200_cmn_proc = NULL;
        cpu_comm_close(CPU_COMM_HDMI);
    }
}