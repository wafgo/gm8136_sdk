#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/miscdevice.h>
#include <linux/kallsyms.h>
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "log.h"
#include "frammap_if.h"
#include "mem_pool.h"
#include "debug.h"
#include "ioctl_mp4e.h"

#if 1
#define dmp_ref(fmt, args...) { \
    if (str)    \
        len += sprintf(str+len, fmt "\n", ## args);  \
    else \
        printk(fmt "\n", ## args); }
#else
#define dmp_ref(fmt, args...) { \
    if (str)    \
        len += sprintf(str+len, fmt "\n", ## args);  \
    else \
        printm(MCP100_MODULE_NAME, fmt "\n", ## args); }
#endif

spinlock_t mcp100_pool_lock;

extern int log_level;
extern unsigned int mp4_dec_enable;
extern char *config_path;

static int getline(char *line, int size, struct file *fd, unsigned long long *offset)
{
    char ch;
    int lineSize = 0, ret;

    memset(line, 0, size);
    while ((ret = (int)vfs_read(fd, &ch, 1, offset)) == 1) {
        if (ch == 0xD)
            continue;
        if (lineSize >= MAX_LINE_CHAR) {
            printk("Line buf is not enough %d! (%s)\n", MAX_LINE_CHAR, line);
            break;
        }
        line[lineSize++] = ch;
        if (ch == 0xA)
            break;
    }
    return lineSize;
}

static int readline(struct file *fd, int size, char *line, unsigned long long *offset)
{
    int ret = 0;
    do {
        ret = getline(line, size, fd, offset);
    } while (((line[0] == 0xa)) && (ret != 0));
    return ret;
}

#ifdef USE_GMLIB_CFG
static int get_config_res_name(char *name, char *line, int offset, int size)
{
    int i = 0;
    while (',' == line[offset] || ' ' == line[offset]) {
    	if (offset >= size)
    		return -1;
    	offset++;
    }
    while ('/' != line[offset]) {
    	name[i] = line[offset];
    	i++;
    	offset++;
    }
    name[i] = '\0';
    return offset;
}

int parse_gmlib_cfg(int *ddr_idx, int codec_type)
{
    char line[MAX_LINE_CHAR];
    char filename[0x80];
    mm_segment_t fs;
    struct file *fd = NULL;
    unsigned long long offset = 0;
    int i, line_size;
    int ret = -1;
    char res_name[10];
    struct ref_pool_t chn_info;

    fs = get_fs();
    set_fs(get_ds());
    sprintf(filename, "%s/%s", config_path, GMLIB_FILENAME);
    fd = filp_open(filename, O_RDONLY, S_IRWXU);
    if (IS_ERR(fd)) {
        //favce_err("open %s error\n", filename);
        //damnit(MODULE_NAME);
        mcp100_wrn("open %s error\n", filename);
        return -1;
    }
    while (readline(fd, sizeof(line), line, &offset)) {
        if (TYPE_MPEG4_ENCODER == codec_type) {
            if (NULL == strstr(line, "[ENCODE_STREAMS]"))
                continue;
        }
        if (TYPE_MPEG4_DECODER == codec_type) {
            if (NULL == strstr(line, "[PLAYBACK]"))
                continue;
        }            

        while (readline(fd, sizeof(line), line, &offset)) {
            if ('\n' == line[0] || '\0' == line[0] || '[' == line[0]) {
                break;
            }
            line_size = strlen(line);
            if (';' == line[0])
                continue;
            i = 0;
            while ('=' != line[i])  i++;
            i++;
            while (' ' == line[i])  i++;

            i = get_config_res_name(res_name, line, i, line_size);
            if (i >= line_size)
                break;
            sscanf(&line[i], "/%d/%d/%d", &chn_info.max_chn, &chn_info.max_fps, ddr_idx);
            ret = 1;
            break;
        }
        if (ret)
            break;
    }
    filp_close(fd, NULL);
    set_fs(fs);
    return ret;
}
#endif


int parse_param_cfg(int *ddr_idx, int *compress_rate, int codec_type)
{
    char line[MAX_LINE_CHAR];
    char filename[0x80];
    mm_segment_t fs;
    struct file *fd = NULL;
    unsigned long long offset = 0;
    int ret = 0;
    int checker = 0;

#ifdef USE_GMLIB_CFG
    if (parse_gmlib_cfg(ddr_idx, codec_type) > 0)
        return 1;
#endif
    if (ddr_idx)
        checker |= 0x01;
    if (compress_rate)
        checker |= 0x02;

    fs = get_fs();
    set_fs(get_ds());
    sprintf(filename, "%s/%s", config_path, PARAM_FILENAME);
    fd = filp_open(filename, O_RDONLY, S_IRWXU);
    if (IS_ERR(fd)) {
        mcp100_err("open %s error\n", filename);
        damnit(MCP100_MODULE_NAME);
        return -1;
    }
    while ((ret = readline(fd, sizeof(line), line, &offset))) {
        if (ddr_idx && strstr(line, "enc_refer_ddr")) {
            sscanf(line, "enc_refer_ddr=%d", ddr_idx);
            ret |= 0x01;
            if (ret == checker)
                break;
        }
        if (compress_rate && strstr(line, "min_compressed_ratio")) {
            sscanf(line, "min_compressed_ratio=%d", compress_rate);
            ret |= 0x02;
            if (ret == checker)
                break;
        }
    }
    filp_close(fd, NULL);
    set_fs(fs);

    return 0;
}

int dump_ref_pool(char *str)
{
    int len = 0;
    len += enc_dump_ref_pool(str+len);
    if (mp4_dec_enable)
        len += dec_dump_ref_pool(str+len);
    return len;
}

int dump_chn_pool(char *str)
{
    int len = 0;
    len += enc_dump_chn_pool(str+len);
    if (mp4_dec_enable)
        len += dec_dump_chn_pool(str+len);
    return len;
}

void dump_pool_num(int ddr_idx)
{
    enc_dump_pool_num(NULL);
    if (mp4_dec_enable)
        dec_dump_pool_num(NULL);
}

int init_mem_pool(int max_width, int max_height)
{
    spin_lock_init(&mcp100_pool_lock);
    if (enc_init_mem_pool(max_width, max_height) < 0)
        return -1;
    if (mp4_dec_enable) {
        if (dec_init_mem_pool(max_width, max_height) < 0) {
            enc_clean_mem_pool();
            return -1;
        }
    }

    return 0;
}

int clean_mem_pool(void)
{
    enc_clean_mem_pool();
    if (mp4_dec_enable)
        dec_clean_mem_pool();
    return 0;
}

/* 
 * accumulate from smaller buffer to larger buffer & check if suitable buffer over spec
*/
/*
static int check_over_spec(void)
{
    int i, count = 0;
    int chn_res_cnt = 0;
    int res_cnt[MAX_RESOLUTION];

    memset(res_cnt, 0, sizeof(res_cnt));
    for (i = 0; i < max_chn; i++) {
        if (chn_pool[i].suitable_res_type >= 0)
            res_cnt[chn_pool[i].suitable_res_type]++;
    }
    for (i = 0; i < MAX_RESOLUTION; i++) {
        chn_res_cnt += res_cnt[i];
        count += ref_pool[i].max_chn;
        if (chn_res_cnt > count) {
            //printk("over spec\n");
            //mcp100_wrn("allocated buffer over spec\n");
            //dump_ref_pool(NULL);
            //dump_chn_pool(NULL);
            return -1;
        }
    }
    return 0;
}
*/
/*
 * Return -1: can not find suitable buffer pool
 */
int register_ref_pool(unsigned int chn, int width, int height, int codec_type)
{
    if (TYPE_MPEG4_ENC == codec_type)
        return enc_register_ref_pool(chn, width, height);
    else if (mp4_dec_enable && TYPE_DECODER == codec_type)
        return dec_register_ref_pool(chn, width, height);
    
    return -1;
    
}

int deregister_ref_pool(unsigned int chn, int codec_type)
{
    if (TYPE_MPEG4_ENC == codec_type)
        return enc_deregister_ref_pool(chn);
    else if (mp4_dec_enable && TYPE_DECODER == codec_type)
        return dec_deregister_ref_pool(chn);

    return 0;
}

/*
 * check buffer pool suitable: register buffer pool & suitable buffer pool
*/
/*
static int get_suitable_resolution(int width, int height)
{
    int i;
    for (i = MAX_RESOLUTION - 1; i >= 0; i--) {
        if (ref_pool[i].max_chn > 0 && res_base_info[i].width >= width && res_base_info[i].height >= height)
            return i;
    }
    return -1;
}

int check_reg_buf_suitable(unsigned char *checker)
{
    int i, ret = 0;
    memset(checker, 0, sizeof(unsigned char)*max_chn);
    mcp100_info("check resolution & suitable resolution\n");
    for (i = 0; i < max_chn; i++) {
        if (chn_pool[i].res_type >= 0 && chn_pool[i].res_type != chn_pool[i].suitable_res_type) {
            checker[i] = 1;
            ret = 1;
        }
    }
    return ret;
}

int mark_suitable_buf(int chn, unsigned char *checker, int width, int height)
{
    int i, ret = 0;
    int suit_res_type;
    if (chn_pool[chn].suitable_res_type < 0)
        return -1;
    memset(checker, 0, sizeof(unsigned char)*max_chn);
    suit_res_type = get_suitable_resolution(width, height);
//printm("FE", "chn%d suit %s\n", chn, res_base_info[suit_res_type].name);
//dump_chn_pool(NULL);
    for (i = 0; i < max_chn; i++) {
        if (chn_pool[i].res_type >= 0 && chn_pool[i].res_type <= suit_res_type) {
            checker[i] = 1;
            ret = 1;
        }
    }
    return ret;
}

int allocate_pool_buffer(unsigned int *addr_va, unsigned int *addr_pa, unsigned int res_idx, int chn)
{
    struct ref_item_t *ref;
    unsigned long flags;
    if (res_idx >= MAX_RESOLUTION) {
        mcp100_err("{chn%d} allocate unknown resolution idx %d\n", chn, res_idx);
        mem_error();
        //damnit(MCP100_MODULE_NAME);
        return -1;
    }
    
    // spin lock
    spin_lock_irqsave(&mcp100_pool_lock, flags);
    
    if (NULL == ref_pool[res_idx].avail) {
        // error
        mcp100_err("%s: no ref buffer\n", res_base_info[res_idx].name);
        mem_error();
        return -1;
    }
    if (chn_pool[chn].used_num >= chn_pool[chn].max_num) {
        mcp100_err("{chn%d} allocate buffer %d over max reference number %d\n", chn, chn_pool[chn].used_num + 1, chn_pool[chn].max_num);
        mem_error();
        return -1;
    }
    
    ref = ref_pool[res_idx].avail;
    remove_buf_from_pool(&ref_pool[res_idx].avail, ref);
    ref_pool[res_idx].avail_num--;
    insert_buf_to_pool(&ref_pool[res_idx].alloc, ref);
    ref_pool[res_idx].used_num++;
    put_buf_to_chn_pool(&chn_pool[chn], ref);
    
    *addr_va = ref->addr_va;
    *addr_pa = ref->addr_pa;
//printk("chn%d allocate buffer %x\n", chn, *addr_va);
//dump_ref_pool(NULL);
    
    spin_unlock_irqrestore(&mcp100_pool_lock, flags);
    
    return 0;
}

int release_pool_buffer(unsigned int addr_va, int chn)
{
    struct ref_item_t *ref;
    int res_idx;
    unsigned long flags;

    if (chn >= max_chn) {
        mcp100_err("channel id(%d) over max channel\n", chn);
        damnit(MCP100_MODULE_NAME);
        return -1;
    }
    if (chn_pool[chn].res_type < 0 || chn_pool[chn].res_type >= MAX_RESOLUTION) {
        mcp100_err("{chn%d} unknown resolution %d\n", chn, chn_pool[chn].res_type);
        mem_error();
        return -1;
    }
    spin_lock_irqsave(&mcp100_pool_lock, flags);
//printk("chn%d release buffer %x\n", chn, addr_va);
    res_idx = chn_pool[chn].res_type;
    ref = remove_buf_from_chn_pool(&chn_pool[chn], addr_va);
    remove_buf_from_pool(&ref_pool[res_idx].alloc, ref);
    ref_pool[res_idx].used_num--;
    insert_buf_to_pool(&ref_pool[res_idx].avail, ref);
    ref_pool[res_idx].avail_num++;
//dump_ref_pool(NULL);

    spin_unlock_irqrestore(&mcp100_pool_lock, flags);
    
    return 0;
}
*/
int allocate_pool_buffer2(struct ref_buffer_info_t *buf, unsigned int res_idx, int chn, int codec_type)
{
    if (TYPE_MPEG4_ENC == codec_type)
        return enc_allocate_pool_buffer2(buf, res_idx, chn);
    else if (mp4_dec_enable && TYPE_DECODER == codec_type)
        return dec_allocate_pool_buffer2(buf, res_idx, chn);
    return -1;
}

int release_pool_buffer2(struct ref_buffer_info_t *buf, int chn, int codec_type)
{
    if (TYPE_MPEG4_ENC == codec_type)
        return enc_release_pool_buffer2(buf, chn);
    else if (mp4_dec_enable && TYPE_DECODER == codec_type)
        return dec_release_pool_buffer2(buf, chn);
    return -1;
}


