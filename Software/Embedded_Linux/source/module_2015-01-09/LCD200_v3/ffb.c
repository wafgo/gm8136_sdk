#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/device.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>

#include "ffb.h"
#include "dev.h"
#include "ffb_api_compat.h"
#include "debug.h"
#include "frammap_if.h"
#include <mach/platform/platform_io.h>

extern unsigned int Platform_Get_FB_AcceID(LCD_IDX_T lcd_idx , int fb_id);

#define FFB_SYNC_TIMEOUT      1300      //unit: msec
#define FFB_GUARD_NUM         2

static struct ffb_params mode_inputs[] = {
    {VIM_YUV422, "YUV422"}, {VIM_YUV420, "YUV420"},
    {VIM_ARGB, "ARGB"}, {VIM_RGB888, "RGB888"}, {VIM_RGB565, "RGB565"},
    {VIM_RGB555, "RGB555"}, {VIM_RGB444, "RGB444"}, {VIM_RGB8, "RGB8"},
    {VIM_RGB1555, "ARGB1555"}, {VIM_RGB1BPP, "RGB1BPP"}, {VIM_RGB2BPP, "RGB2BPP"}
};

static struct ffb_params type_outputs[] = {
    {VOT_LCD, "LCD"}, {VOT_640x480, "640x480"}, {VOT_800x600, "800x600"}, {VOT_1024x768, "1024x768"},
    {VOT_1280x800, "1280x800"}, {VOT_1280x960, "1280x960"}, {VOT_1280x1024, "1280x1024"},
    {VOT_1360x768, "1360x768"}, {VOT_1440x900, "1440x900"}, {VOT_1600x1200, "1600x1200"},
    {VOT_1680x1050, "1680x1050"}, {VOT_1920x1080, "1920x1080"}, {VOT_1280x720, "1280x720"},
    {VOT_PAL, "PAL"}, {VOT_NTSC, "NTSC"}, {VOT_480P, "480P"}, {VOT_576P, "576P"}, {VOT_720P, "720p"},
    {VOT_1440x1080I, "1440x1080i"}, {VOT_1920x1080P, "1920x1080p"}, {VOT_1024x768_30I, "1024x768x60I"},
    {VOT_1024x768_25I, "1024x768x25I"}, {VOT_1024x768P, "1024x768P"}, {VOT_1280x1024P, "1280x1024P"},
    {VOT_1440x900P, "1440x900P"}, {VOT_1680x1050P, "1680x1050P"}, {VOT_1440x960_30I, "1440x960x30I"},
    {VOT_1440x1152_25I, "1440x1152x25I"}, {VOT_1280x1024_30I, "1280x1024x30I"},
    {VOT_1280x1024_25I, "1280x1024x25I"}, {VOT_1920x1080I, "1920x1080I"},
};

/*
 * get frammap index
 */
int get_frammap_idx(struct ffb_info *fbi)
{
    struct ffb_dev_info *fd_info = fbi->dev_info;
    struct lcd200_dev_info *lcd200_info = (struct lcd200_dev_info *)fd_info;
    int index;

    switch (lcd200_info->lcd200_idx) {
    default:
    case LCD_ID:
        switch (fbi->index) {
        case 0:
            index = FRMIDX_FFB0_M;
            break;
        case 1:
            index = FRMIDX_FFB0_S1;
            break;
        case 2:
            index = FRMIDX_FFB0_S2;
            break;
        default:
            goto exit_1;
        }
        break;

    case LCD1_ID:
        switch (fbi->index) {
        case 0:
            index = FRMIDX_FFB1_M;
            break;
        case 1:
            index = FRMIDX_FFB1_S1;
            break;
        case 2:
            index = FRMIDX_FFB1_S2;
            break;
        default:
            goto exit_1;
        }
        break;
    case LCD2_ID:
        switch (fbi->index) {
        case 0:
            index = FRMIDX_FFB2_M;
            break;
        case 1:
            index = FRMIDX_FFB2_S1;
            break;
        case 2:
            index = FRMIDX_FFB2_S2;
            break;
        default:
            goto exit_1;
        }
        break;
    }

    return index;

  exit_1:
    printk("Fatar Error in %s \n", __FUNCTION__);
    return -EINVAL;
}

static int get_param(const struct ffb_params *p, const char *s, int val, int size)
{
    int i;

    for (i = 0; i < size; i++) {
        if (s != NULL) {
            if (!strnicmp(p[i].name, s, strlen(s)))
                return p[i].val;
        } else {
            if (p[i].val == val)
                return (int)p[i].name;
        }
    }
    return -1;
}

int ffb_get_param(u_char witch, const char *s, int val)
{
    int ret = -EINVAL;

    DBGENTER(1);
    switch (witch) {
    case FFB_PARAM_INPUT:
        ret = get_param(mode_inputs, s, val, ARRAY_SIZE(mode_inputs));
        break;
    case FFB_PARAM_OUTPUT:
        ret = get_param(type_outputs, s, val, ARRAY_SIZE(type_outputs));
        break;
    }
    DBGLEAVE(1);
    return ret;
}

static inline void clean_screen(struct ffb_info *fbi)
{
    struct ffb_dev_info *fdev_info = fbi->dev_info;
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)fdev_info;
    u32 yuv_pattern = 0x10801080;
    static u_char video_input_mode[3] = { 0xFF, 0xFF, 0xFF };   /* PIP 0/1/2 */
    static VIN_RES_T input_res[3] = { VIN_NONE, VIN_NONE, VIN_NONE };   /* PIP 0/1/2 */

    if (fbi->not_clean_screen) {
        fbi->not_clean_screen = 0;
        return;
    }

    /* avoid clear screen serveral times. Some vendors put their log in frameBuffer, so we can't clear it
     */
    if (input_res[fbi->index] == fdev_info->input_res->input_res) {
        /* If the input resolution doesn't change, we need to check the input format. If it is changed, we need to
         * clean the screen as well.
         */
        if (video_input_mode[fbi->index] == fbi->video_input_mode)
            return;
        video_input_mode[fbi->index] = fbi->video_input_mode;
    } else {
        /* When the input resolution is changed, we need to clean the buffer */
        input_res[fbi->index] = fdev_info->input_res->input_res;
    }

    DBGIENTER(1, fbi->index);

    switch (fbi->video_input_mode) {
    case VIM_YUV422:
        {
            unsigned int *pbuf;
            int i;

            pbuf = (unsigned int *)(fbi->ppfb_cpu[0]);
            //for (i = 0; i < (fbi->fb.var.xres*fbi->fb.var.yres)>>1; i++)
            for (i = 0; i < (fbi->fb.fix.smem_len >> 2); i++)
                pbuf[i] = yuv_pattern;
            break;
        }
    case VIM_YUV420:
        {
            unsigned char *pbuf;

            pbuf = fbi->ppfb_cpu[0] + fdev_info->planar_off.y;
            memset(pbuf, 16, fdev_info->planar_off.u - fdev_info->planar_off.y);
            pbuf = pbuf + fdev_info->planar_off.u;
            memset(pbuf, 128, fdev_info->planar_off.v - fdev_info->planar_off.u);
            pbuf = pbuf + fdev_info->planar_off.v;
            memset(pbuf, 128, fdev_info->planar_off.v - fdev_info->planar_off.u);
            break;
        }
    case VIM_ARGB:
    case VIM_RGB888:
    case VIM_RGB565:
    case VIM_RGB555:
    case VIM_RGB1555:
    case VIM_RGB444:
    case VIM_RGB8:
    case VIM_RGB1BPP:
    case VIM_RGB2BPP:
        {
            unsigned char *pbuf;
            int i;

            if (fdev_info->fb0_fb1_share == 0) {
                pbuf = fbi->ppfb_cpu[0];
                memset(pbuf, 0, fbi->fb.var.xres * fbi->fb.var.yres * fbi->fb.var.bits_per_pixel/8);
                DBGPRINT(4, "(%s):base=0x%x, xres=%d,yres=%d,bits=%d\n",
                         (char *)ffb_get_param(FFB_PARAM_INPUT, NULL, fbi->video_input_mode),
                         fbi->ppfb_dma[0], fbi->fb.var.xres, fbi->fb.var.yres,
                         fbi->fb.var.bits_per_pixel);
            }

            /* GRAPHIC_COMPRESS, for GUI plane only */
            if (dev_info->support_ge && fbi->index == 1) {
                for (i = 0; i < 2; i ++) {
                    pbuf = fbi->gui_buf[i];
                    if (pbuf) {
                        memset(pbuf, 0, (fbi->fb.fix.smem_len * WRITE_LIMIT_RATIO) / 100);
                        *(u32 *)pbuf = 0x1;             //init value
                        *(u32 *)((u32)pbuf + 4) = 0x0;  //init value
                    }
                }
            }

            break;
        }
    default:
        err("The input mode %d does not support.", fbi->video_input_mode);
        break;
    }

    DBGILEAVE(1, fbi->index);
}

static inline u_int chan_to_field(u_int chan, struct fb_bitfield *bf)
{
    chan &= 0xffff;
    chan >>= 16 - bf->length;
    return chan << bf->offset;
}

/*
 */
static int setpalettereg(u_int regno, u_int red, u_int green,
                         u_int blue, u_int trans, struct fb_info *info)
{
    struct ffb_info *fbi = (struct ffb_info *)info;
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)fbi->dev_info;
    u_int val, ret = 1, offset;
    u32 tmp;

    DBGIENTER(1, fbi->index);

    if (regno < fbi->palette_size) {
        /* this value based on custom's parameter.
         */
        val = blue & 0x1F;       /* blue */
        val |= (green & 0x3F) << 5;      /* green */
        val |= (red & 0x1F) << 11;       /* red */

        fbi->palette_cpu[regno] = val;  //16bits
        ret = 0;

        /* write to hardware */
        if (regno % 2) {
            tmp = fbi->palette_cpu[regno - 1] & 0xFFFF;
            tmp |= (val & 0xFFFF) << 16;
        } else {
            tmp = fbi->palette_cpu[regno + 1] & 0xFFFF;
            tmp |= (val & 0xFFFF);
        }

        offset = regno / 2;

        dev_info->ops->reg_write((struct ffb_dev_info *)dev_info, LCD_PALETTE_RAM + (offset * 4),
                                 tmp, 0xFFFFFFFF);
    }

    DBGILEAVE(1, fbi->index);
    return ret;
}

static int ffb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
                         u_int trans, struct fb_info *info)
{
    struct ffb_info *fbi = (struct ffb_info *)info;
    unsigned int val;
    int ret = 1;

    DBGIENTER(1, fbi->index);
    if (fbi->video_input_mode < VIM_RGB) {
        err("%s mode does not support this feature!",
            (char *)ffb_get_param(FFB_PARAM_INPUT, NULL, fbi->video_input_mode));
        goto end;
    }

        /**
	 * If inverse mode was selected, invert all the colours
	 * rather than the register number.  The register number
	 * is what you poke into the framebuffer to produce the
	 * colour you requested.
	 */
    if (fbi->cmap_inverse) {
        red = 0xffff - red;
        green = 0xffff - green;
        blue = 0xffff - blue;
    }

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
            u32 *pal = fbi->fb.pseudo_palette;

            val = chan_to_field(red, &fbi->fb.var.red);
            val |= chan_to_field(green, &fbi->fb.var.green);
            val |= chan_to_field(blue, &fbi->fb.var.blue);

            pal[regno] = val;
            ret = 0;
        }
        break;
    case FB_VISUAL_DIRECTCOLOR:
    case FB_VISUAL_STATIC_PSEUDOCOLOR:
    case FB_VISUAL_PSEUDOCOLOR:
        ret = setpalettereg(regno, red, green, blue, trans, info);
        break;
    }
  end:
    DBGILEAVE(1, fbi->index);
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
#if 0
    struct ffb_info *fbi = (struct ffb_info *)info;
    int i;

    DBGIENTER(1, fbi->index);

    DBGPRINT(3, "blank=%d\n", blank);

    switch (blank) {
    case FB_BLANK_POWERDOWN:
    case FB_BLANK_VSYNC_SUSPEND:
    case FB_BLANK_HSYNC_SUSPEND:
    case FB_BLANK_NORMAL:
        if (fbi->fb.fix.visual == FB_VISUAL_PSEUDOCOLOR ||
            fbi->fb.fix.visual == FB_VISUAL_STATIC_PSEUDOCOLOR)
            for (i = 0; i < fbi->palette_size; i++)
                ffb_setpalettereg(i, 0, 0, 0, 0, info);
        ffb_schedule_work(fbi, C_DISABLE);
        break;

    case FB_BLANK_UNBLANK:
        if (fbi->fb.fix.visual == FB_VISUAL_PSEUDOCOLOR ||
            fbi->fb.fix.visual == FB_VISUAL_STATIC_PSEUDOCOLOR)
            fb_set_cmap(&fbi->fb.cmap, info);
        ffb_schedule_work(fbi, C_ENABLE);
        break;
    }
    DBGILEAVE(1, fbi->index);
#endif

    return 0;
}

/**
 *@fn ffb_check_var():
 *    Round up in the following order: bits_per_pixel, xres,
 *    yres, xres_virtual, yres_virtual, xoffset, yoffset, grayscale,
 *    bitfields, horizontal timing, vertical timing.
 */
static int ffb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    struct ffb_info *fbi = (struct ffb_info *)info;
    struct ffb_dev_info *dev_info = fbi->dev_info;
    int rgbidx;

    DBGIENTER(1, fbi->index);
    if (var->xres < MIN_XRES)
        var->xres = MIN_XRES;
    if (var->yres < MIN_YRES)
        var->yres = MIN_YRES;
    if (var->xres > dev_info->max_xres)
        var->xres = dev_info->max_xres;
    if (var->yres > dev_info->max_yres)
        var->yres = dev_info->max_yres;
    var->xres_virtual = max(var->xres_virtual, var->xres);
    var->yres_virtual = max(var->yres_virtual, var->yres);

    DBGPRINT(2, "var->bits_per_pixel=%d\n", var->bits_per_pixel);
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

                /**
		 * Copy the RGB parameters for this display
		 * from the machine specific parameters.
		 */
        var->red = fbi->rgb[rgbidx]->red;
        var->green = fbi->rgb[rgbidx]->green;
        var->blue = fbi->rgb[rgbidx]->blue;
        var->transp = fbi->rgb[rgbidx]->transp;

        DBGPRINT(2, "RGBT length = %d:%d:%d:%d\n",
                 var->red.length, var->green.length, var->blue.length, var->transp.length);
    }
    DBGILEAVE(1, fbi->index);

    return 0;
}

/**
 *@fn ffb_set_par():
 *	Set the user defined part of the display for the specified console
 */
static int ffb_set_par(struct fb_info *info)
{
    struct ffb_info *fbi = (struct ffb_info *)info;
    //struct ffb_dev_info *fdev_info = fbi->dev_info;
    struct fb_var_screeninfo *var = &info->var;
    unsigned long palette_mem_size;

    DBGIENTER(1, fbi->index);
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
            if (!fbi->cmap_static)
                fbi->fb.fix.visual = FB_VISUAL_PSEUDOCOLOR;
            else {
                            /**
			     * Some people have weird ideas about wanting static
			     * pseudocolor maps.  I suspect their user space
			     * applications are broken.
			     **/
                fbi->fb.fix.visual = FB_VISUAL_STATIC_PSEUDOCOLOR;
            }
            break;
        }

        fbi->palette_size = 256;

        palette_mem_size = fbi->palette_size * sizeof(u16);

        DBGPRINT(2, "palette_mem_size = 0x%08lx\n", (u_long) palette_mem_size);

        fbi->palette_cpu = (u16 *) (fbi->map_cpu + PAGE_SIZE - palette_mem_size);
        memset(fbi->palette_cpu, 0, palette_mem_size);
        fbi->palette_dma = fbi->map_dma + PAGE_SIZE - palette_mem_size;
    }

    fbi->fb.fix.line_length = var->xres_virtual * var->bits_per_pixel / 8;

    clean_screen(fbi);

    DBGILEAVE(1, fbi->index);
    return 0;
}

static int ffb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
    struct ffb_info *fbi = (struct ffb_info *)info;
    unsigned long off = vma->vm_pgoff << PAGE_SHIFT;
    int ret = -EINVAL;
    unsigned long total_size = vma->vm_end - vma->vm_start;
    unsigned long start = vma->vm_start;
    int i;
    unsigned long pfn;
    unsigned long size;

    DBGIENTER(1, fbi->index);
    if (off < fbi->fb.fix.smem_len) {
        size = fbi->fb.fix.smem_len - off;
        vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
        vma->vm_flags |= VM_RESERVED;
        if (total_size < fbi->fb.fix.smem_len) {
            size = total_size;
        }
        for (i = 0; i < fbi->fb_num; i++) {
            off += fbi->ppfb_dma[i];
            pfn = off >> PAGE_SHIFT;
            ret = remap_pfn_range(vma, start, pfn, size, vma->vm_page_prot);
            DBGPRINT(3, "p=%#x, v=%#x, size=%#x\n", off, start, size);
            start += size;
            total_size -= size;
            off = 0;
            if (!total_size)
                break;
            if (total_size < fbi->fb.fix.smem_len) {
                size = total_size;
            } else {
                size = fbi->fb.fix.smem_len;
            }
        }
    } else {
        err("buffer mapping error !!\n");
    }

    DBGILEAVE(1, fbi->index);
    return ret;
}

/* get how many fb_num queued in the FIFO */
static inline int fifo_len(struct ffb_info *fbi)
{
    /* return 0 means empty */
    if (fbi->fci.put_idx >= fbi->fci.get_idx)
        return (fbi->fci.put_idx - fbi->fci.get_idx);
    else
        return (fbi->fb_num - fbi->fci.get_idx + fbi->fci.put_idx);
}

/* AP puts the index to the fifo */
static int put_fifo(struct ffb_info *fbi, int idx)
{
    int timeout = FFB_SYNC_TIMEOUT / 100;

    while ((fbi->fb_num - fifo_len(fbi)) <= FFB_GUARD_NUM) {
        set_current_state(TASK_UNINTERRUPTIBLE);
        schedule_timeout(HZ / 100);
        if (--timeout <= 0)
            break;
    }
    if (timeout <= 0)
        return -EBUSY;

    //printk(KERN_DEBUG "PUT p=%d g=%d idx=%d len=%d\n",g_fci->put_idx,g_fci->get_idx,idx,len);
    fbi->fci.displayfifo[fbi->fci.put_idx] = idx;

    /* move to next position */
    fbi->fci.put_idx++;

    if (fbi->fci.put_idx == fbi->fb_num)
        fbi->fci.put_idx = 0;

    return 0;
}

//*.S need put_idx,get_idx,displayfif
/* ISR gets fb_num for displaying
 * This function is only invoked from ISR
 */
static inline int get_fifo(struct ffb_info *fbi)
{
    int ret, len;

    /* old method */
    if (fbi->fci.use_queue == 0)
        return fbi->fci.ppfb_num;

    /* get_idx keeps running to catch up put_idx, so if len = 0 means AP didn't feed data in time */
    len = fifo_len(fbi);        // hom many frame in Q

    if (unlikely(len == 0))     //empty
    {
        fbi->fci.miss_num++;
        return -1;
    }

    ret = fbi->fci.displayfifo[fbi->fci.get_idx];
    //printk(KERN_DEBUG "GET p=%d g=%d ret=%d len=%d\n",put_idx,get_idx,ret,len);

    /* move to next get_idx */
    fbi->fci.get_idx++;

    if (fbi->fci.get_idx == fbi->fb_num)
        fbi->fci.get_idx = 0;

    return ret;
}

/* this function called from ioctl when AP wants to get a free fbnum
 */
int get_free_fbnum(struct ffb_info *fbi)
{
    int fbnum, timeout = FFB_SYNC_TIMEOUT / 100;

    if (timeout) {
    }                           // avoid warning

    /* empty slot must be larger than 2, one is displaying, one was updated(or is going to update) to hardware. */
    while ((fbi->fb_num - fifo_len(fbi)) <= FFB_GUARD_NUM) {
        set_current_state(TASK_UNINTERRUPTIBLE);
        schedule_timeout(HZ / 100);
        if (--timeout <= 0)
            break;
    }
    if (timeout <= 0) {
        DBGPRINT(3, "get free num timeout! \n");
        return -EBUSY;
    }

    fbnum = fbi->fci.set_idx + 1;       /* move to next idx */

    if (fbnum == fbi->fb_num)
        fbnum = 0;

    DBGPRINT(3, "get free num = %d \n", fbnum);

    return fbnum;
}

/**
 * Get the next buffer index that want to switch.
 * This function will be called in IRQ mode.
 * Return Value:
 * < 0 means no buffer can be switched.
 * >= 0 means valid fbnum
 */
static int ffb_switch_buffer(struct ffb_info *fbi)
{
    int fbnum;

    spin_lock(&fbi->display_lock);
    fbnum = get_fifo(fbi);
    spin_unlock(&fbi->display_lock);

    return fbnum;
}

/**
 * This interlace also support buffer queue to manage buffer.
 */
static int ffb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    int fbnum, ret = 0;
    unsigned long retVal;
    struct ffb_info *fbi = (struct ffb_info *)info;

    DBGIENTER(1, fbi->index);

    switch (cmd) {
    case FLCD_GET_DATA_SEP:
    case FFB_GET_DATA_SEP:
        {
            struct flcd_data f_data = { 0 };
            int i;

            f_data.buf_len = fbi->fb.fix.smem_len;
            f_data.uv_offset = fbi->fb.var.xres * fbi->fb.var.yres;
            f_data.frame_no = fbi->fb_num;
            for (i = 0; i < fbi->fb_num; i++) {
                f_data.mp4_map_dma[i] = fbi->ppfb_dma[i];
            }

            DBGPRINT(3, "FFB_GET_DATA_SEP:\n");
            if (copy_to_user((void __user *)arg, &f_data, sizeof(struct flcd_data)))
                ret = -EFAULT;
        }
        break;

    case FLCD_SET_FB_NUM:      //0,1.. n
    case FFB_SET_FB_NUM:
    case FLCD_SET_SPECIAL_FB:  //0,1.. n
    case FFB_SET_SPECIAL_FB:
        if (fbi->fci.use_queue == 1) {
            fbi->fci.use_queue = 0;
            fbi->fci.get_idx = fbi->fci.put_idx = 0;
            fbi->fci.set_idx = fbi->fb_num - 1; /* park in the last fb_num */

            printk("Change to manage frame buffer without queue! \n");
        }

        if (copy_from_user(&fbnum, (unsigned int *)arg, sizeof(unsigned int))) {
            ret = -EFAULT;
            break;
        }

        /* boundary check */
        if (fbnum >= fbi->fb_num) {
            ret = -EINVAL;
            err("Boundary error of fb_num  = %d !", fbnum);
            break;
        }
        fbi->fb_use_ioctl = 1;
        if (fbnum == -1) {      //parking
            fbi->fb_use_ioctl = 0;
            fbnum = 0;
        }

        fbi->fci.ppfb_num = fbnum;

        DBGPRINT(3, "FLCD_SET_FB_NUM: Display fb index=%d, dma_addr=0x%x\n", fbnum,
                 fbi->ppfb_dma[fbnum]);
        break;

    case FFB_GETBUF:           //get
        if (fbi->index == 3)    // VBI
        {
            err("VBI doesn't support buffer control! \n");
            ret = EINVAL;
            break;
        }

        if (fbi->fb_num <= 3) { /* configured fbnum is too small */
            err("Number of frame buffer is too small. Can't use queue management IOCTL.. \n");
            ret = EINVAL;
            break;
        }
        fbi->fci.use_queue = 1;
        fbnum = get_free_fbnum(fbi);
        if (fbnum < 0) {
            ret = -EBUSY;
            err("get free buffer timeout!");
        } else {
            retVal = copy_to_user((void __user *)arg, &fbnum, sizeof(int));
            DBGPRINT(3, "FFB_GETBUF: get fbnum = %d \n", fbnum);
        }
        fbi->fb_use_ioctl = 1;
        break;

    case FFB_PUTBUF:           //put
        if (fbi->index == 3)    // VBI
        {
            err("VBI doesn't support buffer control! \n");
            ret = EINVAL;
            break;
        }

        fbi->fb_use_ioctl = 1;
        fbi->fci.use_queue = 1;
        retVal = copy_from_user(&fbnum, (unsigned int *)arg, sizeof(unsigned int));
        /* boundary check */
        if (fbnum == 0xff) {
            /* parking */
            fbi->fci.use_queue = 0;
            fbi->fci.get_idx = fbi->fci.put_idx = 0;
            fbi->fci.set_idx = fbi->fb_num - 1; /* park in the last fb_num */
            fbi->fb_use_ioctl = 0;
            break;
        }

        if (fbnum >= fbi->fb_num) {
            ret = -EINVAL;
            err("Boundary error of fb_num  = %d !", fbnum);
            break;
        }
        fbi->fci.set_idx = fbi->fci.ppfb_num = fbnum;
        put_fifo(fbi, fbnum);
        DBGPRINT(3, "FFB_PUTBUF: put fb_num = %d \n", fbnum);
        break;

    case FFB_SETBUF_FMT:
        /* display with interlace or progressive */
        // 0xffffiiii
        // ffff: buffer format (1:progress 2:interlace)
        // iiii: buffer index
        {
            unsigned int fmt;

            retVal = copy_from_user(&fmt, (unsigned int *)arg, sizeof(unsigned int));
            fbnum = fmt & 0xffff;
            fmt = (fmt >> 16) & 0xffff;
            fbi->ppfb_dma[fbnum] = (fbi->ppfb_dma[fbnum] & 0xfffffffc) | fmt;   /* last two bits */
        }
        break;

    case FFB_GETBUFSIZE:
        {
            int len;

            /* get queued size for the frames */
            fbi->fci.use_queue = 1;
            len = fifo_len(fbi);
            retVal = copy_to_user((void __user *)arg, &len, sizeof(int));
            DBGPRINT(3, "FFB_GETBUFSIZE: frames queued size = %d \n", len);
        }
        break;
    case FBIOPUTCMAP:
        break;

    case FFB_SET_PALETTE:
        {
            struct palette  data;
            u_int red, green, blue, regno;

            retVal = copy_from_user(&data, (unsigned int *)arg, sizeof(struct palette));
            regno = data.entry_idx;
            red = (data.color >> 11) & 0x1F;
            green = (data.color >> 5) & 0x3F;
            blue = data.color & 0x1F;
            if (setpalettereg(regno, red, green, blue, 0, &fbi->fb))
                ret = -1;
        }
        break;
    default:
        err("cmd(0x%x) no define!\n", cmd);
        ret = -EFAULT;
        break;
    }

    DBGILEAVE(1, fbi->index);
    return ret;
}

static struct ffb_ops ffb_drv_ops = {
    .check_var = ffb_check_var,
    .set_par = ffb_set_par,
    .setcolreg = ffb_setcolreg,
    .blank = ffb_blank,
    .ioctl = ffb_ioctl,
    .switch_buffer = ffb_switch_buffer,
};

static inline void unmap_video_memory(struct ffb_info *fbi)
{
    struct ffb_dev_info *fd_info = fbi->dev_info;
    u32 i;

    for (i = 1; i < fbi->fb_num; i++) {
        if (fbi->ppfb_cpu[i]) {
            if (fd_info->fb0_fb1_share && (fbi->index == 1)) {
                fbi->ppfb_cpu[i] = NULL;
                continue;
            }

            if (frm_release_buf_info((void *) fbi->ppfb_cpu[i]) < 0)
                err("FFB: Release memory fail1! \n");

            fbi->ppfb_cpu[i] = NULL;
        }
    }

    if (fbi->map_cpu) {
        if (fd_info->fb0_fb1_share) {
            /* only fb1 can't release memory */
            if ((fbi->index != 1) && frm_release_buf_info((void *)fbi->map_cpu) < 0)
                err("FFB: Release memory fail2! \n");
        } else {
            if (frm_release_buf_info((void *)fbi->map_cpu) < 0)
                err("FFB: Release memory fail3! \n");
        }

        fbi->map_cpu = NULL;
    }

    /* GRAPHIC_COMPRESS */
    if (fbi->gui_buf[i] != NULL) {
        /* only graphic plane 1 supports the compression */
        int i;

        for (i = 0; i < 2; i++) {
            frm_release_buf_info((void *)fbi->gui_buf[i]);
        }
    }

}

/**
 *@fn map_video_memory():
 *      Allocates the DRAM memory for the frame buffer.  This buffer is
 *	remapped into a non-cached, non-buffered, memory region to
 *      allow palette and pixel writes to occur without flushing the
 *      cache.  Once this area is remapped, all virtual memory
 *      access to the video memory should occur at the new region.
 */
static int map_video_memory(struct ffb_info *fbi)
{
    struct ffb_dev_info *fd_info = fbi->dev_info;
    struct lcd200_dev_info *dev_info = (struct lcd200_dev_info *)fd_info;
    int i, index = 0;
    struct frammap_buf_info binfo;
    char name[20];

    DBGIENTER(1, fbi->index);

    /* clear memory first */
    for (i = 0; i < fbi->fb_num; i++) {
        fbi->ppfb_cpu[i] = NULL;
        fbi->ppfb_dma[i] = 0;
    }

    index = get_frammap_idx(fbi);

    /**
	 * We reserve one page for the palette, plus the size
	 * of the framebuffer.
	 */
    fbi->map_size = fbi->fb.fix.smem_len + PAGE_SIZE;
    fbi->map_size = PAGE_ALIGN(fbi->map_size);

    /* GRAPHIC_COMPRESS */
    if (fbi->index == 1) {
        if (dev_info->support_ge) {
            struct frammap_buf_info ge_info;

            /* only graphic plane 1 supports the compression */
            int i;

            memset(&ge_info, 0, sizeof(ge_info));
            ge_info.size = (fbi->fb.fix.smem_len * WRITE_LIMIT_RATIO) / 100; //0.5
            ge_info.alloc_type = ALLOC_NONCACHABLE;
            ge_info.name = "lcd_ge";
            for (i = 0; i < 2; i++) {
                if (frm_get_buf_info(FRM_ID(index, 0), &ge_info) < 0)
                    panic("Memory MMAP buffer allocation error!\n");

                fbi->gui_buf[i] = (u_char *) ge_info.va_addr;
                fbi->gui_dma[i] = ge_info.phy_addr;
            }
        }
    }

    memset(&name[0], 0, sizeof(name));
    sprintf(name, "lcd%d_fb%d", dev_info->lcd200_idx, fbi->index);

    memset(&binfo, 0, sizeof(binfo));
    binfo.size = fbi->map_size;
    binfo.alloc_type = ALLOC_NONCACHABLE;
    binfo.name = name;
    if (fd_info->fb0_fb1_share && (fbi->index == 1)) {
        /* fb1 shares with fb0 memory */
        struct ffb_info *fb0_fbi = (struct ffb_info *)dev_info->fb[0];

        binfo.phy_addr = fb0_fbi->map_dma;
        binfo.va_addr = (void *)fb0_fbi->map_cpu;
        binfo.size = fb0_fbi->map_size;
    } else {
        if (frm_get_buf_info(FRM_ID(index, 0), &binfo) < 0) {
            err("Memory MMAP buffer allocation error!\n");
            return -EFAULT;
        }
    }

    fbi->map_dma = binfo.phy_addr;
    fbi->map_cpu = (u_char *) binfo.va_addr;

    if (binfo.size < fbi->map_size) {
        err("frammap returns smaller memory! Expected size = %d, but got size = %d\n",
            fbi->map_size, binfo.size);
        return -ENOMEM;
    }

    if (fbi->map_cpu) {
        u32 i;

        fbi->ppfb_cpu[0] = fbi->map_cpu + PAGE_SIZE;
        fbi->ppfb_dma[0] = fbi->map_dma + PAGE_SIZE;

        for (i = 1; i < fbi->fb_num; i++) {
            memset(&name[0], 0, sizeof(name));
            memset(&binfo, 0, sizeof(binfo));
            binfo.size = fbi->fb.fix.smem_len;
            binfo.alloc_type = ALLOC_NONCACHABLE;
            sprintf(name, "lcd%d_fb%d", dev_info->lcd200_idx, fbi->index);
            binfo.name = name;

            if (fd_info->fb0_fb1_share && (fbi->index == 1)) {
                /* fb1 shares with fb0 memory */
                struct ffb_info *fb0_fbi = (struct ffb_info *)dev_info->fb[0];

                binfo.phy_addr = (u32)fb0_fbi->ppfb_dma[i];
                binfo.va_addr = (void *)fb0_fbi->ppfb_cpu[i];
                binfo.size = fb0_fbi->fb.fix.smem_len;
            } else {
                if (frm_get_buf_info(FRM_ID(index, 0), &binfo) < 0) {
                    err("Memory MMAP buffer allocation error!\n");
                    return -EFAULT;
                }
            }

            fbi->ppfb_dma[i] = (dma_addr_t) binfo.phy_addr;
            fbi->ppfb_cpu[i] = (u_char *) binfo.va_addr;

            if (binfo.size < fbi->fb.fix.smem_len) {
                err("frammap returns smaller memory! Expected size = %d, return size = %d\n",
                    fbi->fb.fix.smem_len, binfo.size);
                return -ENOMEM;
            }

            if (fbi->ppfb_cpu[i] == NULL) {
                unmap_video_memory(fbi);
                return -ENOMEM;
            }
        }

        fbi->fb.screen_base = fbi->map_cpu + PAGE_SIZE;
        fbi->fb.fix.smem_start = fbi->map_dma + PAGE_SIZE;
    }

    DBGILEAVE(1, fbi->index);

    return fbi->map_cpu ? 0 : -ENOMEM;
}

/* Fake monspecs to fill in fbinfo structure */
static struct fb_monspecs monspecs = {
    .hfmin = 30000,
    .hfmax = 70000,
    .vfmin = 50,
    .vfmax = 65,
};

int ffb_soft_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
    unsigned int scan_align = info->pixmap.scan_align - 1;
    unsigned int buf_align = info->pixmap.buf_align - 1;
    unsigned int i, size, dsize, s_pitch, d_pitch;
    struct ffb_info *fbi = (struct ffb_info *)info;
    struct fb_image *image;
    u8 *dst, *src;

    DBGIENTER(1, fbi->index);

    if (info->state != FBINFO_STATE_RUNNING)
        return 0;

    s_pitch = (cursor->image.width + 7) >> 3;
    dsize = s_pitch * cursor->image.height;

    src = kmalloc(dsize + sizeof(struct fb_image), GFP_ATOMIC);
    if (!src)
        return -ENOMEM;

    image = (struct fb_image *)(src + dsize);
    *image = cursor->image;
    d_pitch = (s_pitch + scan_align) & ~scan_align;

    size = d_pitch * image->height + buf_align;
    size &= ~buf_align;
    dst = fb_get_buffer_offset(info, &info->pixmap, size);

    if (cursor->enable) {
        switch (cursor->rop) {
        case ROP_XOR:
            for (i = 0; i < dsize; i++)
                src[i] = image->data[i] ^ cursor->mask[i];
            break;
        case ROP_COPY:
        default:
            for (i = 0; i < dsize; i++)
                src[i] = image->data[i] & cursor->mask[i];
            break;
        }
    } else
        memcpy(src, image->data, dsize);

    fb_pad_aligned_buffer(dst, d_pitch, src, s_pitch, image->height);
    image->data = dst;
    info->fbops->fb_imageblit(info, image);
    kfree(src);

    DBGILEAVE(1, fbi->index);
    return 0;
}

/**
 *      ffb_fb_pan_display - Pans the display.
 *      @var: frame buffer variable screen structure
 *      @info: frame buffer structure that represents a single frame buffer
 *      Only support frame0 & frame1
 *
 *      Returns negative errno on error, or zero on success.
 */
static int ffb_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    struct ffb_info *fbi = (struct ffb_info *)info;
    struct fb_fix_screeninfo *fix = &info->fix;

    if (var->xoffset != 0) {	/* not support ... */
        printk("%s, var->xoffset: %d is not supported! \n", __func__, var->xoffset);
		return -EINVAL;
	}

    if (!fix->ypanstep) {
        printk("%s, zero fix->ypanstep \n", __func__);
        return -EINVAL;
    }

    if (var->yoffset % fix->ypanstep) {
        printk("%s, only support yoffset %d, the pass value is %d \n", __func__, fix->ypanstep, var->yoffset);
        return -EINVAL;
    }

    if (var->yoffset + info->var.yres > info->var.yres_virtual) {
        printk("%s, out of range! yres: %d, yoffset: %d, yres_virtual: %d \n", __func__,
                                            info->var.yres, var->yoffset, info->var.yres_virtual);
        return -EINVAL;
    }

    fbi->ypan_offset = var->yoffset;

    return 0;
}

/**
 * Initiation frame buffer information that kernel's layer needs.
 */

static int  init_fbinfo(struct ffb_info *fbi)
{
    int ret = 0;
    struct fb_ops *fbops = NULL;
    struct ffb_dev_info *fd_info = fbi->dev_info;
    struct lcd200_dev_info *lcd200_info = (struct lcd200_dev_info *)fd_info;

    DBGIENTER(1, fbi->index);
    fbops = kzalloc(sizeof(struct fb_ops), GFP_KERNEL);
    if (fbops == NULL) {
        ret = -ENOMEM;
        err("Alloc buffer failed.");
        goto end;
    }

    fbops->fb_check_var = ffb_check_var,
    fbops->fb_set_par = ffb_set_par,
    fbops->fb_setcolreg = ffb_setcolreg,
    fbops->fb_fillrect = cfb_fillrect,
    fbops->fb_copyarea = cfb_copyarea,
    fbops->fb_imageblit = cfb_imageblit,
    fbops->fb_blank = ffb_blank,
    fbops->fb_cursor = ffb_soft_cursor,
    fbops->fb_mmap = ffb_mmap, fbops->fb_ioctl = ffb_ioctl, fbi->fb.fbops = fbops;
    fbops->fb_pan_display = (fbi->fb_pan_display == 1) ? ffb_fb_pan_display : NULL;

    fbi->fb.fix.type = FB_TYPE_PACKED_PIXELS;
    fbi->fb.fix.type_aux = 0;
    fbi->fb.fix.xpanstep = 0;
    fbi->fb.fix.ypanstep = (fbi->fb_pan_display == 1) ? 1 : 0;

    fbi->fb.fix.ywrapstep = 0;
    fbi->fb.fix.accel = Platform_Get_FB_AcceID(lcd200_info->lcd200_idx,fbi->index);
    fbi->fb.var.nonstd = 0;
    fbi->fb.var.activate = FB_ACTIVATE_NOW;
    fbi->fb.var.height = -1;
    fbi->fb.var.width = -1;
    fbi->fb.var.accel_flags = 0;
    fbi->fb.var.vmode = FB_VMODE_NONINTERLACED;
    fbi->fb.flags = FBINFO_DEFAULT;
    fbi->fb.monspecs = monspecs;
    fbi->fb.pseudo_palette = (fbi + 1);
  end:
    DBGILEAVE(1, fbi->index);

    return ret;
}

int  ffb_construct(struct ffb_info *fbi)
{
    int ret = 0;

    DBGIENTER(1, fbi->index);
    /* Initiation frame buffer information that kernel's layer needs. */
    ret = init_fbinfo(fbi);
    if (ret) {
        goto cleanup;
    }
    fbi->ops = &ffb_drv_ops;

    /* Initialize video memory */
    ret = map_video_memory(fbi);
    if (ret < 0) {
        goto cleanup;
    }

    /* allocate display queue */
    fbi->fci.displayfifo = kzalloc(fbi->fb_num * sizeof(int), GFP_KERNEL);
    if (fbi->fci.displayfifo == NULL) {
        err("Alloc displayfifo failed for fb%d", fbi->index);
        ret = -ENOMEM;
        goto err_free;
    }

    /* for display queue use */
    spin_lock_init(&fbi->display_lock);

    init_waitqueue_head(&fbi->fb_wlist);

    DBGILEAVE(1, fbi->index);
    return 0;

  err_free:
    unmap_video_memory(fbi);
  cleanup:
    DBGILEAVE(1, fbi->index);
    return ret;
}

/* Called when the device is being detached from the driver */
void ffb_deconstruct(struct ffb_info *fbi)
{
    DBGIENTER(1, fbi->index);
    unmap_video_memory(fbi);
    DBGILEAVE(1, fbi->index);
}

/*
 * This function is used to get the frame buffer base in which the hardware is using.
 * Paramter:
 *  lcd_idx: 0(first LCD) or 1(2nd LCD)
 * Return:
 *  physical address or 0 for fail
 */
unsigned int ffb_get_fb_pbase(int lcd_idx, int fb_plane)
{
    unsigned int base, phy_addr;

    if ((lcd_idx != LCD_ID) && (lcd_idx != LCD1_ID) && (lcd_idx != LCD2_ID))
        return 0;

    switch (lcd_idx) {
      case LCD_ID:
        base = (unsigned int)ioremap_nocache(LCD_FTLCDC200_0_PA_BASE, PAGE_SIZE);
        break;

#ifdef LCD_DEV1
      case LCD1_ID:
        base = (unsigned int)ioremap_nocache(LCD_FTLCDC200_1_PA_BASE, PAGE_SIZE);
        break;
#endif

#ifdef LCD_DEV2
      case LCD2_ID:
        base = (unsigned int)ioremap_nocache(LCD_FTLCDC200_2_PA_BASE, PAGE_SIZE);
        break;
#endif
      default:
        panic("%s, not support lcd_idx: %d \n", __func__, lcd_idx);
        break;
    }

    switch (fb_plane) {
    case 0:
        phy_addr = *(unsigned int *)(base + 0x18);
        break;
    case 1:
        phy_addr = *(unsigned int *)(base + 0x24);
        break;
    case 2:
        phy_addr = *(unsigned int *)(base + 0x30);
        break;
    default:
        phy_addr = 0;
        break;
    }

    return phy_addr;
}

EXPORT_SYMBOL(ffb_get_param);
EXPORT_SYMBOL(ffb_get_fb_pbase);
EXPORT_SYMBOL(ffb_construct);
EXPORT_SYMBOL(ffb_deconstruct);

MODULE_LICENSE("GPL");
