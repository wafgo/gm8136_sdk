#include <linux/module.h>
#include <linux/seq_file.h>
#include "proc.h"
#include "debug.h"
#include "lcd_def.h"
#include "platform.h"
#include "codec.h"
#include "frame_comm.h"

#define NUM_PROC    LCD_IP_NUM
#define FLCD_COMMON "flcd200_cmn"

static struct proc_dir_entry *ffb_proc_root[3] = {NULL, NULL, NULL};
char *proc_name[3] = { "flcd200", "flcd200_1", "flcd200_2"}; /* name is driver name */
struct proc_dir_entry *flcd_common_proc = NULL;
static struct proc_dir_entry *lcd_summary_proc = NULL;
static struct proc_dir_entry *ffb_gui_proc = NULL;

static int get_proc_idx(char *name)
{
    int i, bFound = -1;

    for (i = 0; i < NUM_PROC; i++) {
        if ((strcmp(name, proc_name[i])) == 0) {
            bFound = i;
            break;
        }
    }

    return bFound;
}

struct proc_dir_entry *ffb_create_proc_entry(const char *name,
                                             mode_t mode, struct proc_dir_entry *parent)
{
    struct proc_dir_entry *p;
    struct proc_dir_entry *root;
    u32 idx = 0;

    if ((u32) parent == LCD_ID) {
        idx = 0;
        parent = NULL;
    } else if ((u32) parent == LCD1_ID) {
        idx = 1;
        parent = NULL;
    } else if ((u32) parent == LCD2_ID) {
        idx = 2;
        parent = NULL;
    }

    root = (parent == NULL) ? ffb_proc_root[idx] : parent;

    p = create_proc_entry(name, mode, root);

    return p;
}

EXPORT_SYMBOL(ffb_create_proc_entry);

void ffb_remove_proc_entry(struct proc_dir_entry *parent, struct proc_dir_entry *de)
{
    struct proc_dir_entry *root;
    int idx = 0;

    /* this case: caller pass its idx via parent parameter */
    if (((int)parent) == LCD_ID) {
        idx = 0;
        parent = NULL;
    } else if (((int)parent) == LCD1_ID) {
        idx = 1;
        parent = NULL;
    } else if ((u32) parent == LCD2_ID) {
        idx = 2;
        parent = NULL;
    }

    root = (parent == NULL) ? ffb_proc_root[idx] : parent;

    if (de)
        remove_proc_entry(de->name, root);
}

EXPORT_SYMBOL(ffb_remove_proc_entry);

int ffb_proc_init(const char *name)
{
    int idx, ret = 0;
    struct proc_dir_entry *p;

    p = create_proc_entry(name, S_IFDIR | S_IRUGO | S_IXUGO, NULL);     /* /proc */

    if (p == NULL) {
        ret = -ENOMEM;
        goto end;
    }

    idx = get_proc_idx((char *)name);
    if (idx < 0)
        goto fail;

    if (ffb_proc_root[idx] != NULL)
        printk("Warning: ffb_proc_init: %s is occupied by others! \n", name);

    ffb_proc_root[idx] = p;

    ret = dbg_proc_init(p);

    if (ret < 0)
        goto fail;

  end:
    return ret;
  fail:
    remove_proc_entry(name, NULL);
    return ret;
}

EXPORT_SYMBOL(ffb_proc_init);

void ffb_proc_release(char *name)
{
    int idx;

    idx = get_proc_idx(name);
    if (idx < 0) {
        printk("(ffb_proc_release): Error device name %s... \n", name);
        return;
    }

    if (ffb_proc_root[idx] != NULL) {
        dbg_proc_remove(ffb_proc_root[idx]);
        remove_proc_entry(ffb_proc_root[idx]->name, NULL);
    }

    ffb_proc_root[idx] = NULL;
}

int ffb_proc_get_usecnt(void)
{
    int i, cnt = 0;

    for (i = 0; i < NUM_PROC; i++) {
        if (ffb_proc_root[i] != NULL)
            cnt++;
    }

    return cnt;
}

/*
 * lcd information summary
 */
static int (*summary_func[LCD_MAX_ID])(proc_lcdinfo_t *lcd_info);

/**
 *@brief collect every LCD's summary, such as resolution, frame rate ...
 */
int proc_register_lcdinfo(LCD_IDX_T idx, int (*callback)(proc_lcdinfo_t *summary))
{
    if (idx >= LCD_IP_NUM) {
        panic("%d is out of range! \n", idx);
        return -1;
    }

    summary_func[idx] = (void *)callback;

    return 0;
}


/*
 * ffb gui info
 */
static int ffb_gui_show(struct seq_file *sfile, void *v)
{
    int idx, ret, plane;
    proc_lcdinfo_t  info;
    gui_info_t      *gui_info;

    for (idx = LCD_ID; idx < LCD_MAX_ID; idx ++) {
        if (summary_func[idx] == NULL)
            continue;

        ret = (*summary_func[idx])(&info);
        if (ret)    /* error */
            continue;

        gui_info = &info.gui_info;
        seq_printf(sfile,"lcd idx: %d \n", idx);
        for (plane = 0; plane < PIP_NUM; plane ++) {
            if (gui_info->gui[plane].active == 0)
                continue;

            seq_printf(sfile,"  plane idx: %d \n", plane);
            seq_printf(sfile,"  xres: %d \n", gui_info->gui[plane].xres);
            seq_printf(sfile,"  yres: %d \n", gui_info->gui[plane].yres);
            seq_printf(sfile,"  buff_len: %d \n", gui_info->gui[plane].buff_len);
            seq_printf(sfile,"  format: %d \n", gui_info->gui[plane].format);
            seq_printf(sfile,"  phys_addr: 0x%x \n", gui_info->gui[plane].phys_addr);
            seq_printf(sfile,"  bits_per_pixel: %d \n", gui_info->gui[plane].bits_per_pixel);
            seq_printf(sfile,"\n");
        }
    }

    return 0;
}

static int ffb_gui_open(struct inode *inode, struct file *file)
{
	return single_open(file, ffb_gui_show, NULL);
}

static struct file_operations ffb_gui_ops = {
	.owner = THIS_MODULE,
	.open = ffb_gui_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/* called from pip(vg) layer */
int ffb_proc_get_lcdinfo(LCD_IDX_T lcd_idx, proc_lcdinfo_t *lcdinfo)
{
    int ret;

    if (lcd_idx >= LCD_MAX_ID)
        return -1;

    if (summary_func[lcd_idx] == NULL)
        return -1;

    ret = (*summary_func[lcd_idx])(lcdinfo);
    if (ret)
        return -1;

    return 0;
}

static int proc_lcd_summary_read(char *page, char **start, off_t off, int count, int *eof,
                                   void *data)
{
    unsigned int len = 0, idx;
    char *string[]= {"active", "inactive"};
    proc_lcdinfo_t  info;

    len += sprintf(page+len, "#lcd summary information(new): \n");
    len += sprintf(page+len, "#lcd_id, type, engine_minor, framerate, input info, output info, fb_addr, status \n");

    for (idx = LCD_ID; idx < LCD_MAX_ID; idx ++) {
        if (ffb_proc_get_lcdinfo(idx, &info))
            continue;

        len += sprintf(page+len, "%d %d 0x%x %d {%d %s %d %d} {%d %s %d %d} 0x%x %s\n", info.lcd_idx, info.vg_type,
            info.engine_minor, info.hw_frame_rate, info.input_res, info.input_res_desc,
            info.in_x, info.in_y, info.output_type, info.output_type_desc, info.out_x, info.out_y, (u32)info.fb_vaddr,
            (info.lcd_disable & 0x1) ? string[1] : string[0]);
    }
    *eof = 1;
    return len;
}

int __init dummy_proc_init(void)
{
    extern void codec_middleware_init(void);
    int idx;

    flcd_common_proc = ffb_create_proc_entry(FLCD_COMMON, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (flcd_common_proc == NULL)
        return -1;

    lcd_frameaddr_init();
    codec_middleware_init();
    platform_pmu_init();

    for (idx = 0; idx < LCD_MAX_ID; idx ++) {
        summary_func[idx] = (void *)NULL;
    }

    /* create a proc node in common dir
     */
    lcd_summary_proc = ffb_create_proc_entry("vg_info", S_IRUGO | S_IXUGO, flcd_common_proc);
	if (lcd_summary_proc == NULL) {
		panic("Fail to create input_res_proc!\n");
		return -1;
	}
	lcd_summary_proc->read_proc = (read_proc_t *)proc_lcd_summary_read;

    /* ffb GUI information */
    ffb_gui_proc = ffb_create_proc_entry("gui_info", S_IRUGO | S_IXUGO, flcd_common_proc);
	if (ffb_gui_proc == NULL) {
		panic("Fail to create ffb_gui_proc!\n");
		return -1;
	}
	ffb_gui_proc->proc_fops = &ffb_gui_ops;

    return 0;

}

static void __exit dummy_proc_cleanup(void)
{
    platform_pmu_exit();
    codec_middle_exit();

    remove_proc_entry("vg_info", flcd_common_proc);
    remove_proc_entry("gui_info", flcd_common_proc);
    remove_proc_entry(FLCD_COMMON, NULL);
    return;
}

EXPORT_SYMBOL(ffb_proc_get_usecnt);
EXPORT_SYMBOL(ffb_proc_release);
EXPORT_SYMBOL(flcd_common_proc);
EXPORT_SYMBOL(proc_register_lcdinfo);
EXPORT_SYMBOL(ffb_proc_get_lcdinfo);

module_init(dummy_proc_init);
module_exit(dummy_proc_cleanup);

MODULE_DESCRIPTION("GM LCD200-Common driver");
MODULE_AUTHOR("GM Technology Corp.");
MODULE_LICENSE("GPL");
