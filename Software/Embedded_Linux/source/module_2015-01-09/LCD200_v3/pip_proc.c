#include <linux/seq_file.h>
#include <linux/bitops.h>
#include "ffb.h"
#include "dev.h"
#include "proc.h"
#include "frame_comm.h"

static struct proc_dir_entry *root = NULL;
static struct proc_dir_entry *fb0_input_mode = NULL;
static struct proc_dir_entry *fb1_input_mode = NULL;
static struct proc_dir_entry *fb2_input_mode = NULL;
static struct proc_dir_entry *vg_debug = NULL;
static struct proc_dir_entry *property_debug = NULL;
static struct proc_dir_entry *output_type_proc = NULL;
static struct proc_dir_entry *input_res_proc = NULL;
static struct proc_dir_entry *fb0_vs_proc = NULL;
static struct proc_dir_entry *fb1_vs_proc = NULL;
static struct proc_dir_entry *fb2_vs_proc = NULL;

static int fb_input_show(struct seq_file *sfile, void *v, int idx)
{
    int ret = 0;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    int i;

    seq_printf(sfile, "Support type:\n");
    seq_printf(sfile, "--------------------\n");
    for (i = 0; i < 32; i++) {
        if (info->fb[idx]->support_imodes & (1 << i)) {
            seq_printf(sfile, "%d: %s\n", i, (char *)ffb_get_param(FFB_PARAM_INPUT, NULL, i));
        }
    }
    seq_printf(sfile, "--------------------\n");
    seq_printf(sfile, "Current mode is %s!\n", (char *)ffb_get_param(FFB_PARAM_INPUT, NULL,
                                                                     info->fb[idx]->
                                                                     video_input_mode));
    return ret;
}

static ssize_t fb_input_proc_write(struct file *file, const char __user * buf, size_t size,
                                   loff_t * off, int idx)
{
    int len = size;
    unsigned char value[60];
    u32 v0;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;

    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';
    sscanf(value, "%u\n", &v0);

    if (info->fb[idx] && info->fb[idx]->video_input_mode == v0)
        return size;

    if (info->fb[idx] && info->fb[idx]->support_imodes & (1 << v0)) {
        info->fb[idx]->video_input_mode = v0;
        pip_switch_input_mode(&g_dev_info);
        info->fb[idx]->fb.fbops->fb_check_var(&info->fb[idx]->fb.var, &info->fb[idx]->fb);
        info->fb[idx]->fb.fbops->fb_set_par(&info->fb[idx]->fb);
        info->ops->set_par((struct ffb_dev_info *)info);
    } else {
        printk("Not support Mode %d(%s)!\n", v0, (char *)ffb_get_param(FFB_PARAM_INPUT, NULL, v0));
    }
    return size;
}

static int fb0_input_show(struct seq_file *sfile, void *v)
{
    return fb_input_show(sfile, v, 0);
}

static int fb0_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, fb0_input_show, NULL);
}

static ssize_t fb0_input_proc_write(struct file *file, const char __user * buf, size_t size,
                                    loff_t * off)
{
    return fb_input_proc_write(file, buf, size, off, 0);
}

static struct file_operations fb0_input_proc_ops = {
    .owner = THIS_MODULE,
    .open = fb0_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = fb0_input_proc_write,
};

static int fb1_input_show(struct seq_file *sfile, void *v)
{
    return fb_input_show(sfile, v, 1);
}

static int fb1_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, fb1_input_show, NULL);
}

static ssize_t fb1_input_proc_write(struct file *file, const char __user * buf, size_t size,
                                    loff_t * off)
{
    return fb_input_proc_write(file, buf, size, off, 1);
}

static struct file_operations fb1_input_proc_ops = {
    .owner = THIS_MODULE,
    .open = fb1_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = fb1_input_proc_write,
};

static int fb2_input_show(struct seq_file *sfile, void *v)
{
    return fb_input_show(sfile, v, 2);
}

static int fb2_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, fb2_input_show, NULL);
}

static ssize_t fb2_input_proc_write(struct file *file, const char __user * buf, size_t size,
                                    loff_t * off)
{
    return fb_input_proc_write(file, buf, size, off, 2);
}

static struct file_operations fb2_input_proc_ops = {
    .owner = THIS_MODULE,
    .open = fb2_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = fb2_input_proc_write,
};

static struct proc_dir_entry *blend_param = NULL;

#ifdef LCD210
static int blend_show(struct seq_file *sfile, void *v)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    int BlendH, BlendL;
    u32 Priority;

    Priority = info->ops->reg_read((struct ffb_dev_info *)info, PIP_IMG_PRI);
    seq_printf(sfile, "Blend_H(0-255) Blend_L(0-255) Img0_P Img1_P Img2_P\n");
    pip_get_blend_value(&g_dev_info, &BlendH, &BlendL);
    seq_printf(sfile, "%-14d %-14d %-6d %-6d %-6d\n", BlendH, BlendL,
               Priority & 0x3, (Priority >> 2) & 0x3, (Priority >> 4) & 0x3);

    return 0;
}

static int blend_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, blend_show, NULL);
}

#define check_blend_value(v)  ((v>0xff)?(0xff):v)
#define check_priority_value(v)  ((v>2)?(2):v)
static ssize_t blend_proc_write(struct file *file, const char __user * buf, size_t size,
                                loff_t * off)
{
    int len = size;
    unsigned char value[60];
    int bv0, bv1, pv0, pv1, pv2;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    u32 Priority;

    if (copy_from_user(value, buf, len))
        return 0;

    Priority = info->ops->reg_read((struct ffb_dev_info *)info, PIP_IMG_PRI);
    pv0 = Priority & 0x3;
    pv1 = (Priority >> 2) & 0x3;
    pv2 = (Priority >> 4) & 0x3;
    pip_get_blend_value(&g_dev_info, &bv0, &bv1);
    value[len] = '\0';
    sscanf(value, "%u %u %u %u %u\n", &bv0, &bv1, &pv0, &pv1, &pv2);
    bv0 = check_blend_value(bv0);
    bv1 = check_blend_value(bv1);
    pv0 = check_priority_value(pv0);
    pv1 = check_priority_value(pv1);
    pv2 = check_priority_value(pv2);
    pip_set_blend_value(&g_dev_info, bv0, bv1);
    info->ops->reg_write((struct ffb_dev_info *)info, PIP_IMG_PRI, (pv2 << 4) | (pv1 << 2) | pv0,
                         0x3f);
    return size;
}
#else
const char *blend_str[] = {
    "0%",
    "6%",
    "12%",
    "18%",
    "25%",
    "31%",
    "37%",
    "43%",
    "50%",
    "56%",
    "62%",
    "68%",
    "75%",
    "81%",
    "87%",
    "93%",
    "100%",
};

static int blend_show(struct seq_file *sfile, void *v)
{
    int ret = 0;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    int BlendH, BlendL;
    u32 Priority;

    Priority = info->ops->reg_read((struct ffb_dev_info *)info, PIP_IMG_PRI);
    seq_printf(sfile, "Blend_X:");
    for (ret = 0; ret < 17; ret++) {
        seq_printf(sfile, "%d:%s,", ret, blend_str[ret]);
    }
    ret = 0;
    seq_printf(sfile, "\n");
    seq_printf(sfile, "Blend_H Blend_L Img0_P Img1_P Img2_P\n");
    pip_get_blend_value(&g_dev_info, &BlendH, &BlendL);
    seq_printf(sfile, "%-7s %-7s %-6d %-6d %-6d\n", blend_str[BlendH], blend_str[BlendL],
               Priority & 0x3, (Priority >> 2) & 0x3, (Priority >> 4) & 0x3);
    return ret;
}

static int blend_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, blend_show, NULL);
}

#define check_blend_value(v)  ((v>0x1f)?(0x1f):v)
#define check_priority_value(v)  ((v>2)?(2):v)
static ssize_t blend_proc_write(struct file *file, const char __user * buf, size_t size,
                                loff_t * off)
{
    int len = size;
    unsigned char value[60];
    int bv0, bv1, pv0, pv1, pv2;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    u32 Priority;

    if (copy_from_user(value, buf, len))
        return 0;

    Priority = info->ops->reg_read((struct ffb_dev_info *)info, PIP_IMG_PRI);
    pv0 = Priority & 0x3;
    pv1 = (Priority >> 2) & 0x3;
    pv2 = (Priority >> 4) & 0x3;
    pip_get_blend_value(&g_dev_info, &bv0, &bv1);
    value[len] = '\0';
    sscanf(value, "%u %u %u %u %u\n", &bv0, &bv1, &pv0, &pv1, &pv2);
    bv0 = check_blend_value(bv0);
    bv1 = check_blend_value(bv1);
    pv0 = check_priority_value(pv0);
    pv1 = check_priority_value(pv1);
    pv2 = check_priority_value(pv2);
    pip_set_blend_value(&g_dev_info, bv0, bv1);
    info->ops->reg_write((struct ffb_dev_info *)info, PIP_IMG_PRI, (pv2 << 4) | (pv1 << 2) | pv0,
                         0x3f);
    return size;
}
#endif /* LCD210 */

static struct file_operations blend_proc_ops = {
    .owner = THIS_MODULE,
    .open = blend_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = blend_proc_write,
};

static struct proc_dir_entry *pip_mode = NULL;

static int pip_mode_show(struct seq_file *sfile, void *v)
{
    int ret = 0;
    seq_printf(sfile, "PIP:0-Disable 1-Single PIP Win 2-Double PIP Win\n");
    seq_printf(sfile, "PIP=%d (Blend=%d)\n", g_dev_info.PIP_En, g_dev_info.Blend_En);
    return ret;
}

static int pip_mode_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, pip_mode_show, NULL);
}

static ssize_t pip_mode_proc_write(struct file *file, const char __user * buf, size_t size,
                                   loff_t * off)
{
    int len = size;
    unsigned char value[60];
    u32 v0;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    if (copy_from_user(value, buf, len))
        return 0;
    value[len] = '\0';
    sscanf(value, "%u\n", &v0);
    if (v0 < 3) {
        pip_switch_pip_mode(&g_dev_info, v0);
        info->ops->set_par((struct ffb_dev_info *)info);
    }
    return size;
}

static struct file_operations pip_mode_proc_ops = {
    .owner = THIS_MODULE,
    .open = pip_mode_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = pip_mode_proc_write,
};

static struct proc_dir_entry *fb1_win = NULL;
static struct proc_dir_entry *fb2_win = NULL;

static int fb_win_show(struct seq_file *sfile, void *v, int idx)
{
    int ret = 0;
    struct ffb_rect rect;

    ret = pip_get_window(&g_dev_info, &rect, idx);
    if (ret < 0)
        return ret;

    seq_printf(sfile, "X    Y    W    H\n");
    seq_printf(sfile, "%-4d %-4d %-4d %-4d\n", rect.x, rect.y, rect.width, rect.height);
    return ret;
}

static ssize_t fb_win_proc_write(struct file *file, const char __user * buf, size_t size,
                                 loff_t * off, int idx)
{
    int len = size;
    unsigned char value[60];
    struct ffb_rect rect;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;

    if (copy_from_user(value, buf, len))
        return 0;

    pip_get_window(&g_dev_info, &rect, idx);
    value[len] = '\0';
    sscanf(value, "%u %u %u %u\n", &rect.x, &rect.y, &rect.width, &rect.height);
    pip_set_window(&g_dev_info, &rect, idx);
    info->ops->set_par((struct ffb_dev_info *)info);
    return size;
}

static int fb1_win_show(struct seq_file *sfile, void *v)
{
    return fb_win_show(sfile, v, 1);
}

static int fb1_win_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, fb1_win_show, NULL);
}

static ssize_t fb1_win_proc_write(struct file *file, const char __user * buf, size_t size,
                                  loff_t * off)
{
    return fb_win_proc_write(file, buf, size, off, 1);
}

static struct file_operations fb1_win_proc_ops = {
    .owner = THIS_MODULE,
    .open = fb1_win_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = fb1_win_proc_write,
};

static int fb2_win_show(struct seq_file *sfile, void *v)
{
    return fb_win_show(sfile, v, 2);
}

static int fb2_win_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, fb2_win_show, NULL);
}

static ssize_t fb2_win_proc_write(struct file *file, const char __user * buf, size_t size,
                                  loff_t * off)
{
    return fb_win_proc_write(file, buf, size, off, 2);
}

static struct file_operations fb2_win_proc_ops = {
    .owner = THIS_MODULE,
    .open = fb2_win_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = fb2_win_proc_write,
};

static int vg_dbg_show(struct seq_file *sfile, void *v)
{
#ifdef VIDEOGRAPH_INC
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;
    unsigned int read_idx, write_idx;

    lcd_frame_get_rwidx(&read_idx, &write_idx);

    seq_printf(sfile, "jobs on qlist: %d \n", g_info.qnum);
    seq_printf(sfile, "jobs on cb_list: %d \n", g_info.cbnum);
    seq_printf(sfile, "jobs on wlist: %d \n", g_info.wnum);
    seq_printf(sfile, "playing job: %d \n", g_info.cur_node ? 1 : 0);
    seq_printf(sfile, "memory alloc count: %d \n", g_info.mem_alloc);
    seq_printf(sfile, "callback count: %d \n", g_info.cb_job_cnt);
    seq_printf(sfile, "job queue empty: %d \n", g_info.empty_job_cnt);
    seq_printf(sfile, "stop count: %d \n", g_info.stop_cnt);
    seq_printf(sfile, "src_frame_rate: %d, hw_denominator(*%d): %d \n",
               g_info.in_prop.src_frame_rate, FRAME_RATE_GRANULAR, dev_info->hw_frame_rate);
    seq_printf(sfile, "lcdvg: fr_numerator: %d, fr_denominator: %d, skip: %d \n",
               framerate_numerator, framerate_denominator, framerate_skip);
    seq_printf(sfile, "3frame: read_idx: %d, write_idx: %d, putfail: %d \n", read_idx, write_idx,
                put_3frame_fail_cnt);
    seq_printf(sfile, "scaler_job_success: %d, scaler_job_fail: %d \n", put_scalerjob_ok_cnt, put_scalerjob_fail_cnt);
    seq_printf(sfile, "gui_clone_fps: %d \n", gui_clone_fps);
    seq_printf(sfile, "lcd_gui_bitmap: 0x%x \n", lcd_gui_bitmap);
    seq_printf(sfile, "lcdvg_dbg: %d \n", lcdvg_dbg);
#endif
    return 0;
}

static int vg_dbg_open(struct inode *inode, struct file *file)
{
    return single_open(file, vg_dbg_show, NULL);
}

static ssize_t vg_dbg_write(struct file *file, const char __user * buf, size_t size, loff_t * off)
{
    int len = size;
    unsigned char value[10];
    extern int lcdvg_dbg;

    if (copy_from_user(value, buf, len))
        return 0;

    value[len] = '\0';
    sscanf(value, "%d", &lcdvg_dbg);

    printk("lcdvg_dbg: %d \n", lcdvg_dbg);

    return size;
}

static struct file_operations vg_dbg_ops = {
    .owner = THIS_MODULE,
    .open = vg_dbg_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = vg_dbg_write,
};

/* output_type
 */
static int output_type_show(struct seq_file *sfile, void *v)
{
    u32 tmp, i, j = 0;
    struct ffb_timing_info *timing;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;
    ffb_output_name_t *output_name;

    /* List all output_type name
     */
    seq_printf(sfile, "\n");
    for (i = 0; i < OUTPUT_TYPE_LAST; i++) {
        output_name = ffb_get_output_name(i);
        if (output_name == NULL)
            continue;
        seq_printf(sfile, "id = %2d, %-20s ", output_name->output_type, output_name->name);

        if (j & 0x1)
            seq_printf(sfile, "\n");
        j++;
    }

    seq_printf(sfile, "\n\nCurrent output_type value = %d \n\n", output_type);

    timing = get_output_timing(dev_info->output_info, dev_info->video_output_type);

    seq_printf(sfile, "     Input Res.       Output Res.       Max Res.   \n");
    seq_printf(sfile, "------------------------------------------------   \n");

    if (dev_info->video_output_type >= VOT_CCIR656) {
        seq_printf(sfile, "xres:%-10d       %-11d       %-d\n", timing->data.ccir656.in.xres,
                   timing->data.ccir656.out.xres, dev_info->max_xres);
        seq_printf(sfile, "yres:%-10d       %-11d       %-d\n", timing->data.ccir656.in.yres,
                   timing->data.ccir656.out.yres, dev_info->max_yres);
    } else {
        seq_printf(sfile, "xres:%-10d       %-11d       %-d\n", timing->data.lcd.in.xres,
                   timing->data.lcd.out.xres, dev_info->max_xres);
        seq_printf(sfile, "yres:%-10d       %-11d       %-d\n", timing->data.lcd.in.yres,
                   timing->data.lcd.out.yres, dev_info->max_yres);
    }

    seq_printf(sfile, "(input_res = %s, output_type = %s)\n", dev_info->input_res->name,
               (char *)ffb_get_param((u_char) FFB_PARAM_OUTPUT, (const char *)NULL,
                                     (int)dev_info->video_output_type));

    seq_printf(sfile, "\nEDID support list: ");
    for (i = 0; i < OUTPUT_TYPE_LAST; i ++) {
        if (!test_bit(i, (void *)g_dev_info.edid_array))
            continue;
        seq_printf(sfile, "%d, ", i);

        if (i == (OUTPUT_TYPE_LAST - 1))
            seq_printf(sfile, "\n");
    }

    /* scalar */
    tmp = info->ops->reg_read((struct ffb_dev_info *)info, FEATURE);
    seq_printf(sfile, "\n");
    seq_printf(sfile, "LCD Scalar          : %s \n",
               ((tmp & (0x1 << 5)) > 0) ? "Enabled" : "Disabled");
    seq_printf(sfile, "Cursor postion scale: %s \n",
               (dev_info->cursor_pos_scale > 0) ? "Enabled" : "Disabled");

    return 0;
}

static int output_type_open(struct inode *inode, struct file *file)
{
    return single_open(file, output_type_show, NULL);
}

static ssize_t output_type_write(struct file *file, const char __user * buf, size_t size,
                                 loff_t * off)
{
    int len = size;
    unsigned char value[20];
    int tmp, l_input_res = VIN_NONE, ouput_type_tmp;
    struct ffb_dev_info *fdev_info = (struct ffb_dev_info *)&g_dev_info;

    if (copy_from_user(value, buf, len))
        return 0;

    down(&fdev_info->dev_lock);

    value[len] = '\0';
    sscanf(value, "%d", &ouput_type_tmp);

    tmp = l_input_res = fdev_info->input_res->input_res;
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

    if (pip_config_resolution((ushort) l_input_res, (ushort) ouput_type_tmp) < 0)
        printk("Fail! \n");
    else {
        if (l_input_res != tmp) {
            printk("Warning: input resoultion is changed to %s\n",
                   (l_input_res == VIN_NTSC) ? "NTSC" : "PAL");
            input_res = l_input_res;
        }
        output_type = ouput_type_tmp;
    }

    printk("\n");

    up(&fdev_info->dev_lock);

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

/* input_res
 */
static int input_res_show(struct seq_file *sfile, void *v)
{
    u32 i, tmp, j = 0;
    struct ffb_timing_info *timing;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;
    ffb_input_res_t *resolution;

    /* List all input_res name
     */
    seq_printf(sfile, "\n");
    for (i = 0; i < VIN_NONE; i++) {
        resolution = ffb_inres_get_info(i);
        if (resolution == NULL)
            continue;
        seq_printf(sfile, "id = %2d, %-20s ", resolution->input_res, resolution->name);

        if (j & 0x1)
            seq_printf(sfile, "\n");
        j++;
    }

    seq_printf(sfile, "\n\nCurrent input_res value = %d \n\n", input_res);

    timing = get_output_timing(dev_info->output_info, dev_info->video_output_type);

    seq_printf(sfile, "     Input Res.       Output Res.       Max Res.   \n");
    seq_printf(sfile, "------------------------------------------------   \n");

    if (dev_info->video_output_type >= VOT_CCIR656) {
        seq_printf(sfile, "xres:%-10d       %-11d       %-d\n", timing->data.ccir656.in.xres,
                   timing->data.ccir656.out.xres, dev_info->max_xres);
        seq_printf(sfile, "yres:%-10d       %-11d       %-d\n", timing->data.ccir656.in.yres,
                   timing->data.ccir656.out.yres, dev_info->max_yres);
    } else {
        seq_printf(sfile, "xres:%-10d       %-11d       %-d\n", timing->data.lcd.in.xres,
                   timing->data.lcd.out.xres, dev_info->max_xres);
        seq_printf(sfile, "yres:%-10d       %-11d       %-d\n", timing->data.lcd.in.yres,
                   timing->data.lcd.out.yres, dev_info->max_yres);
    }

    seq_printf(sfile, "(input_res = %s, output_type = %s)\n", dev_info->input_res->name,
               (char *)ffb_get_param((u_char) FFB_PARAM_OUTPUT, (char *)NULL,
                                     (int)dev_info->video_output_type));

    /* scalar */
    tmp = info->ops->reg_read((struct ffb_dev_info *)info, FEATURE);
    seq_printf(sfile, "\n");
    seq_printf(sfile, "LCD Scalar          : %s \n",
               ((tmp & (0x1 << 5)) > 0) ? "Enabled" : "Disabled");
    seq_printf(sfile, "Cursor postion scale: %s \n",
               ((tmp & (0x1 << 5)) > 0) ? "Enabled" : "Disabled");

    seq_printf(sfile, "\nInput resolution support list ==> \n");

    for (i = 0; i != (u32) VIN_NONE; i++) {
        /* check if the input resolution we can support */
        if (platform_check_input_res(info->lcd200_idx, (VIN_RES_T) i) < 0)
            continue;

        resolution = ffb_inres_get_info((VIN_RES_T) i);
        seq_printf(sfile, "%s \n", resolution->name);
    }
    seq_printf(sfile, "\n");

    return 0;
}

static int input_res_open(struct inode *inode, struct file *file)
{
    return single_open(file, input_res_show, NULL);
}

static ssize_t input_res_write(struct file *file, const char __user * buf, size_t size,
                               loff_t * off)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    struct ffb_dev_info *dev_info = (struct ffb_dev_info *)info;
    int len = size;
    unsigned char value[20];
    int input_res_tmp, l_output_type;

    if (copy_from_user(value, buf, len))
        return 0;

    down(&dev_info->dev_lock);

    value[len] = '\0';
    sscanf(value, "%d", &input_res_tmp);
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

    if (pip_config_resolution((ushort) input_res_tmp, (ushort) l_output_type) < 0)
        printk("Fail! \n");
    else {
        if (input_res != input_res_tmp) {
            //notify the input resolution
            if (lcdvg_cb_fn && lcdvg_cb_fn->res_change)
                lcdvg_cb_fn->res_change();
        }
        input_res = input_res_tmp;
        if (l_output_type != output_type) {
            output_type = l_output_type;
            printk("Warning: output_type is changed to NTSC or PAL! \n");
        }
    }

    printk("\n");

    up(&dev_info->dev_lock);

    return size;
}

static struct file_operations input_res_ops = {
    .owner = THIS_MODULE,
    .open = input_res_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = input_res_write,
};

/*
 * FB0 Virtual Screen
 */
static int fb0_vs_show(struct seq_file *sfile, void *v)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    struct ffb_info *fb = info->fb[0];

    if (fb) {
        seq_printf(sfile, "\n");
        seq_printf(sfile, "virutal screen: %s \n", fb->vs.enable ? "1:enabled" : "0:disabled");
        seq_printf(sfile, "width: %d, height: %d \n", fb->vs.width, fb->vs.height);
        seq_printf(sfile, "x offset: %d, y offset: %d \n\n", fb->vs.offset_x, fb->vs.offset_y);
        seq_printf(sfile, "echo <0/1> <x> <y> to proc this node. \n");
    }
    return 0;
}

static int fb0_vs_open(struct inode *inode, struct file *file)
{
    return single_open(file, fb0_vs_show, NULL);
}

static ssize_t fb0_vs_write(struct file *file, const char __user * buf, size_t size, loff_t * off)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    struct ffb_info *fb = info->fb[0];
    int ret, len = size, val1, val2, val3;
    int     org_val1, org_val2, org_val3;
    unsigned char value[60];

    if (copy_from_user(value, buf, len))
        return 0;

    if (!fb) {
         printk("FB1 is disabled. It can't be configured!\n");
        return size;
    }
    val1 = org_val1 = fb->vs.enable;
    val2 = org_val2 = fb->vs.offset_x;
    val3 = org_val3 = fb->vs.offset_y;

    value[len] = '\0';
    sscanf(value, "%u %u %u\n", &val1, &val2, &val3);
    fb->vs.enable = val1;
    fb->vs.offset_x = val2;
    fb->vs.offset_y = val3;
    ret = pip_update_vs(fb);
    if (ret) {
        printk("Update FAIL! \n");
        /* rollback the setting */
        fb->vs.enable = org_val1;
        fb->vs.offset_x = org_val2;
        fb->vs.offset_y = org_val3;
        pip_update_vs(fb);
    }
    return size;
}

static struct file_operations fb0_vs_ops = {
    .owner = THIS_MODULE,
    .open = fb0_vs_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = fb0_vs_write,
};


/*
 * FB1 Virtual Screen
 */
static int fb1_vs_show(struct seq_file *sfile, void *v)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    struct ffb_info *fb = info->fb[1];

    if (fb) {
        seq_printf(sfile, "\n");
        seq_printf(sfile, "virutal screen: %s \n", fb->vs.enable ? "enabled" : "disabled");
        seq_printf(sfile, "width: %d, height: %d \n", fb->vs.width, fb->vs.height);
        seq_printf(sfile, "x offset: %d, y offset: %d \n\n", fb->vs.offset_x, fb->vs.offset_y);
        seq_printf(sfile, "echo <0/1> <x offset> <y offset> to proc this node. \n");
    }

    return 0;
}

static int fb1_vs_open(struct inode *inode, struct file *file)
{
    return single_open(file, fb1_vs_show, NULL);
}

static ssize_t fb1_vs_write(struct file *file, const char __user * buf, size_t size, loff_t * off)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    struct ffb_info *fb = info->fb[1];
    int ret, len = size, val1, val2, val3;
    unsigned char value[60];

    if (copy_from_user(value, buf, len))
        return 0;

    if (!fb) {
         printk("FB1 is disabled. It can't be configured!\n");
        return size;
    }
    val1 = fb->vs.enable;
    val2 = fb->vs.offset_x;
    val3 = fb->vs.offset_y;
    value[len] = '\0';
    sscanf(value, "%u %u %u\n", &val1, &val2, &val3);
    fb->vs.enable = val1;
    fb->vs.offset_x = val2;
    fb->vs.offset_y = val3;
    ret = pip_update_vs(fb);
    if (ret)
        printk("Update FAIL! \n");
    return size;
}

static struct file_operations fb1_vs_ops = {
    .owner = THIS_MODULE,
    .open = fb1_vs_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = fb1_vs_write,
};


/*
 * FB2 Virtual Screen
 */
static int fb2_vs_show(struct seq_file *sfile, void *v)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    struct ffb_info *fb = info->fb[2];

    if (fb) {
        seq_printf(sfile, "\n");
        seq_printf(sfile, "virutal screen: %s \n", fb->vs.enable ? "enabled" : "disabled");
        seq_printf(sfile, "width: %d, height: %d \n", fb->vs.width, fb->vs.height);
        seq_printf(sfile, "x offset: %d, y offset: %d \n\n", fb->vs.offset_x, fb->vs.offset_y);
        seq_printf(sfile, "echo <0/1> <x offset> <y offset> to proc this node. \n");
    }

    return 0;
}

static int fb2_vs_open(struct inode *inode, struct file *file)
{
    return single_open(file, fb2_vs_show, NULL);
}

static ssize_t fb2_vs_write(struct file *file, const char __user * buf, size_t size, loff_t * off)
{
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;
    struct ffb_info *fb = info->fb[2];
    int ret, len = size, val1, val2, val3;
    unsigned char value[60];

    if (copy_from_user(value, buf, len))
        return 0;

    if (!fb) {
        printk("FB2 is disabled. It can't be configured!\n");
        return size;
    }
    val1 = fb->vs.enable;
    val2 = fb->vs.offset_x;
    val3 = fb->vs.offset_y;
    value[len] = '\0';
    sscanf(value, "%u %u %u\n", &val1, &val2, &val3);
    fb->vs.enable = val1;
    fb->vs.offset_x = val2;
    fb->vs.offset_y = val3;
    ret = pip_update_vs(fb);
    if (ret)
       printk("Update FAIL! \n");
    return size;
}

static struct file_operations fb2_vs_ops = {
    .owner = THIS_MODULE,
    .open = fb2_vs_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = fb2_vs_write,
};

static void pip_proc_remove(void)
{
    if (root != NULL) {
        if (fb0_input_mode != NULL)
            ffb_remove_proc_entry(root, fb0_input_mode);
        if (fb1_input_mode != NULL)
            ffb_remove_proc_entry(root, fb1_input_mode);
        if (fb2_input_mode != NULL)
            ffb_remove_proc_entry(root, fb2_input_mode);

        if (fb1_win != NULL)
            ffb_remove_proc_entry(root, fb1_win);
        if (fb2_win != NULL)
            ffb_remove_proc_entry(root, fb2_win);

        if (blend_param != NULL)
            ffb_remove_proc_entry(root, blend_param);
        if (pip_mode != NULL)
            ffb_remove_proc_entry(root, pip_mode);

        if (vg_debug != NULL)
            ffb_remove_proc_entry(root, vg_debug);

        if (property_debug != NULL)
            ffb_remove_proc_entry(root, property_debug);

        if (output_type_proc != NULL)
            ffb_remove_proc_entry(root, output_type_proc);

        if (input_res_proc != NULL)
            ffb_remove_proc_entry(root, input_res_proc);

        if (fb0_vs_proc != NULL)
            ffb_remove_proc_entry(root, fb0_vs_proc);

        if (fb1_vs_proc != NULL)
            ffb_remove_proc_entry(root, fb1_vs_proc);

        if (fb2_vs_proc != NULL)
            ffb_remove_proc_entry(root, fb2_vs_proc);

        ffb_remove_proc_entry((struct proc_dir_entry *)(g_dev_info.lcd200_info.lcd200_idx), root);
    }
}

static int pip_proc_init(void)
{
    int ret = 0;
    struct lcd200_dev_info *info = (struct lcd200_dev_info *)&g_dev_info;

    root =
        ffb_create_proc_entry("pip", S_IFDIR | S_IRUGO | S_IXUGO,
                              (struct proc_dir_entry *)(info->lcd200_idx));
    if (root == NULL) {
        err("Fail to create proc root!\n");
        ret = -EINVAL;
        goto fail;
    }

    if (info->fb[0]) {
        fb0_input_mode = ffb_create_proc_entry("fb0_input", S_IRUGO | S_IXUGO, root);
        if (fb0_input_mode == NULL) {
            err("Fail to create proc mode!\n");
            ret = -EINVAL;
            goto fail;
        }
        fb0_input_mode->proc_fops = &fb0_input_proc_ops;
    }

    if (info->fb[1]) {
        fb1_input_mode = ffb_create_proc_entry("fb1_input", S_IRUGO | S_IXUGO, root);
        if (fb1_input_mode == NULL) {
            err("Fail to create proc mode!\n");
            ret = -EINVAL;
            goto fail;
        }
        fb1_input_mode->proc_fops = &fb1_input_proc_ops;

        fb1_win = ffb_create_proc_entry("fb1_win", S_IRUGO | S_IXUGO, root);
        if (fb1_win == NULL) {
            err("Fail to create proc mode!\n");
            ret = -EINVAL;
            goto fail;
        }
        fb1_win->proc_fops = &fb1_win_proc_ops;
    }

    if (info->fb[2]) {
        fb2_input_mode = ffb_create_proc_entry("fb2_input", S_IRUGO | S_IXUGO, root);
        if (fb2_input_mode == NULL) {
            err("Fail to create proc mode!\n");
            ret = -EINVAL;
            goto fail;
        }
        fb2_input_mode->proc_fops = &fb2_input_proc_ops;

        fb2_win = ffb_create_proc_entry("fb2_win", S_IRUGO | S_IXUGO, root);
        if (fb1_win == NULL) {
            err("Fail to create proc mode!\n");
            ret = -EINVAL;
            goto fail;
        }
        fb2_win->proc_fops = &fb2_win_proc_ops;
    }

    blend_param = ffb_create_proc_entry("blend", S_IRUGO | S_IXUGO, root);
    if (blend_param == NULL) {
        err("Fail to create proc mode!\n");
        ret = -EINVAL;
        goto fail;
    }
    blend_param->proc_fops = &blend_proc_ops;

    pip_mode = ffb_create_proc_entry("pip_mode", S_IRUGO | S_IXUGO, root);
    if (pip_mode == NULL) {
        err("Fail to create proc mode!\n");
        ret = -EINVAL;
        goto fail;
    }
    pip_mode->proc_fops = &pip_mode_proc_ops;

    vg_debug = ffb_create_proc_entry("vg_dbg", S_IRUGO | S_IXUGO, root);
    if (vg_debug == NULL) {
        err("Fail to create vg debug!\n");
        ret = -EINVAL;
        goto fail;
    }
    vg_debug->proc_fops = &vg_dbg_ops;

    /* output_typeresoultion proc.
     */
    output_type_proc = ffb_create_proc_entry("output_type", S_IRUGO | S_IXUGO, root);
    if (output_type_proc == NULL) {
        err("Fail to create output_type_proc!\n");
        ret = -EINVAL;
        goto fail;
    }
    output_type_proc->proc_fops = &output_type_ops;

    /* input_res resoultion proc.
     */
    input_res_proc = ffb_create_proc_entry("input_res", S_IRUGO | S_IXUGO, root);
    if (input_res_proc == NULL) {
        err("Fail to create input_res_proc!\n");
        ret = -EINVAL;
        goto fail;
    }
    input_res_proc->proc_fops = &input_res_ops;

    /* virutal screen for fb0
     */
    fb0_vs_proc = ffb_create_proc_entry("fb0_vs", S_IRUGO | S_IXUGO, root);
    if (fb0_vs_proc == NULL) {
        err("Fail to create fb0_vs_proc!\n");
        ret = -EINVAL;
        goto fail;
    }
    fb0_vs_proc->proc_fops = &fb0_vs_ops;

    /* virutal screen for fb1
     */
    fb1_vs_proc = ffb_create_proc_entry("fb1_vs", S_IRUGO | S_IXUGO, root);
    if (fb1_vs_proc == NULL) {
        err("Fail to create fb1_vs_proc!\n");
        ret = -EINVAL;
        goto fail;
    }
    fb1_vs_proc->proc_fops = &fb1_vs_ops;

    /* virutal screen for fb2
    */
    fb2_vs_proc = ffb_create_proc_entry("fb2_vs", S_IRUGO | S_IXUGO, root);
    if (fb2_vs_proc == NULL) {
  err("Fail to create fb2_vs_proc!\n");
        ret = -EINVAL;
        goto fail;
    }
    fb2_vs_proc->proc_fops = &fb2_vs_ops;
    return ret;

fail:
    pip_proc_remove();
    return ret;
}
