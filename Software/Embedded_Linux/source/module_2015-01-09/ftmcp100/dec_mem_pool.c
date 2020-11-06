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
#include "ioctl_mp4d.h"

extern int log_level;
extern char *config_path;
extern unsigned int mp4_tight_buf;
extern unsigned int mp4_overspec_handle;

#if 0
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

struct mem_buffer_info_t
{
    unsigned int addr_pa;
    unsigned int addr_va;
    unsigned int size;
};

#ifdef SUPPORT_8M
static const struct res_base_info_t res_base_info[MAX_RESOLUTION] = {
    { RES_8M,    "8M",    ALIGN16_UP(4096), ALIGN16_UP(2160)},  // 4096 x 2160  : 8847360
    { RES_5M,    "5M",    ALIGN16_UP(2560), ALIGN16_UP(1920)},  // 2560 x 1920  : 4659200
    { RES_4M,    "4M",    ALIGN16_UP(2304), ALIGN16_UP(1728)},  // 2304 x 1728  : 3981312
    { RES_3M,    "3M",    ALIGN16_UP(2048), ALIGN16_UP(1536)},  // 2048 x 1536  : 3145728
    { RES_2M,    "2M",    ALIGN16_UP(1920), ALIGN16_UP(1080)},  // 1920 x 1088  : 2088960
    { RES_1080P, "1080P", ALIGN16_UP(1920), ALIGN16_UP(1080)},  // 1920 x 1088  : 2088960
    { RES_1_3M,  "1.3M",  ALIGN16_UP(1280), ALIGN16_UP(1024)},  // 1280 x 1024  : 1310720
    { RES_1080I, "1080I", ALIGN16_UP(1920), ALIGN16_UP(540)},   // 1920 x 544   : 1044480
    { RES_1M,    "1M",    ALIGN16_UP(1280), ALIGN16_UP(800)},   // 1280 x 800   : 1024000
    { RES_720P,  "720P",  ALIGN16_UP(1280), ALIGN16_UP(720)},   // 1280 x 720   : 921600
    { RES_960H,  "960H",  ALIGN16_UP(960),  ALIGN16_UP(576)},   // 960  x 576   : 552960
    { RES_720I,  "720I",  ALIGN16_UP(1280), ALIGN16_UP(360)},   // 1280 x 368   : 471040
    { RES_D1,    "D1",    ALIGN16_UP(720),  ALIGN16_UP(576)},   // 720  x 576   : 414720
    { RES_VGA,   "VGA",   ALIGN16_UP(640),  ALIGN16_UP(480)},   // 640  x 480   : 307200
    { RES_HD1,   "HD1",   ALIGN16_UP(720),  ALIGN16_UP(360)},   // 720  x 368   : 264960
    { RES_2CIF,  "2CIF",  ALIGN16_UP(360),  ALIGN16_UP(596)},   // 368  x 596   : 219328
    { RES_CIF,   "CIF",   ALIGN16_UP(360),  ALIGN16_UP(288)},   // 368  x 288   : 105984
    { RES_QCIF,  "QCIF",  ALIGN16_UP(180),  ALIGN16_UP(144)}    // 176  x 144   : 25344
};
#else
static const struct res_base_info_t res_base_info[MAX_RESOLUTION] = {
    { RES_1080P, "1080P", ALIGN16_UP(1920), ALIGN16_UP(1080)}, { RES_1080I, "1080I", ALIGN16_UP(1920), ALIGN16_UP(540)},
    { RES_720P,  "720P",  ALIGN16_UP(1280), ALIGN16_UP(720)},  { RES_720I,  "720I",  ALIGN16_UP(1280), ALIGN16_UP(360)},
    { RES_960H,  "960H",  ALIGN16_UP(960),  ALIGN16_UP(576)},  { RES_D1,    "D1",    ALIGN16_UP(720),  ALIGN16_UP(576)},
    { RES_HD1,   "HD1",   ALIGN16_UP(720),  ALIGN16_UP(360)},  { RES_2CIF,  "2CIF",  ALIGN16_UP(360),  ALIGN16_UP(596)},
    { RES_CIF,   "CIF",   ALIGN16_UP(360),  ALIGN16_UP(288)},  { RES_QCIF,  "QCIF",  ALIGN16_UP(180),  ALIGN16_UP(144)}
};
#endif

//static int total_ref_buf_num = 0;
static struct ref_item_t *ref_item = NULL;
static struct layout_info_t layout_info[MAX_LAYOUT_NUM];
static struct ref_pool_t ref_pool[MAX_RESOLUTION];
static struct channel_ref_buffer_t *chn_pool = NULL;
static struct mem_buffer_info_t mp4d_ref_buffer = {0, 0, 0};
static int cur_layout = -1;
static unsigned int max_chn = MP4D_MAX_CHANNEL;
static unsigned int max_ref_num = 1;

extern spinlock_t mcp100_pool_lock;

static int get_one_buffer_size(int width, int height, int buf_type)
{
    int size = 0;
    switch (buf_type) {
    case IDX_MB_BUF:
        size = (width * height / 256 + 1) * sizeof(MPEG4_MACROBLOCK_E);
        break;
    case IDX_REF_BUF:
        size = ((height + 32)*width + 256*2) * 3 / 2;
        break;
    case IDX_DEC_REF_BUF:
        size = width * height * 3 / 2;
        break;
    default:
        break;
    }
    return size;
}

static int get_pool_buf_num(struct ref_pool_t *pool)
{
#ifdef REF_BUFFER_FLOW
    if (pool->max_chn > 1)
        return pool->max_chn + MAX_ENGINE_NUM;
    else
        return pool->max_chn + 1;
#else
    return pool->max_chn * (max_ref_num + 1);
#endif
}

static void mem_error(void)
{
    dec_dump_ref_pool(NULL);
    dec_dump_chn_pool(NULL);
    damnit(MCP100_MODULE_NAME);
}

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

static char *str_tok = NULL;
static char *find_blank_token(char *str)
{
    if (NULL == str) {
        while (*str_tok != '\0' && *str_tok != '\n') {
            if (*str_tok == ' ') 
                break;
            str_tok++;
        }
    }
    else
        str_tok = str;
    if (NULL == str_tok)
        return str_tok;
    while (*str_tok != '\0' && *str_tok != '\n') {
        if (*str_tok == ' ') {
            str_tok++;
            continue;
        }
        return str_tok;
    }
    return NULL;
}

static int str_cmp(char *str0, char *str1, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (str0[i] != str1[i])
            return -1;
    }
    return 0;
}

static int parse_buffer_type(char *str, int *res_order)
{
    int i, idx = 0;
    char *ptr;
    ptr = find_blank_token(str);
    while (ptr) {
        for (i = 0; i < MAX_RESOLUTION; i++) {
            if (str_cmp(ptr, (char *)res_base_info[i].name, (int)strlen(res_base_info[i].name)) == 0) {
                res_order[idx] = i;
                idx++;
                break;
            }
        }
        ptr = find_blank_token(NULL);
    }
    return idx;
}

static int parse_buffer_num(char *str, struct layout_info_t *layout, int *res_order)
{
    char *ptr;
    int idx, res_idx;

    sscanf(str, "CH_%d", &layout->max_channel);
    idx = 0;
    ptr = find_blank_token(str);
    ptr = find_blank_token(NULL);
    while (ptr) {
        res_idx = res_order[idx];
        if (res_idx < 0)
            break;
        sscanf(ptr, "%d/%d", &layout->ref_pool[res_idx].max_chn, &layout->ref_pool[res_idx].max_fps);
        ptr = find_blank_token(NULL);
        idx++;
    }
    return idx;
}

#ifdef USE_GMLIB_CFG
static int get_res_idx(char *res_name)
{
    int i;
    for (i = 0; i < MAX_RESOLUTION; i++) {
        if (0 == str_cmp(res_name, (char *)res_base_info[i].name, strlen(res_base_info[i].name)))
            return i;
    }
    return -1;
}

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

int parse_dec_layout_gmlib_cfg(void)
{
    char line[MAX_LINE_CHAR];
    char filename[0x80];
    mm_segment_t fs;
    struct file *fd = NULL;
    unsigned long long offset = 0;
    int idx, l = 0;
    int i, line_size, chn_cnt;
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
        if (strstr(line, "[PLAYBACK]")) {
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

                chn_cnt = 0;
                while (1) {
                    i = get_config_res_name(res_name, line, i, line_size);
                    if (i >= line_size)
                        break;
                    sscanf(&line[i], "/%d/%d/%d", &chn_info.max_chn, &chn_info.max_fps, &chn_info.ddr_idx);
                    idx = get_res_idx(res_name);
                    if (idx >= 0 && (0 == chn_info.ddr_idx || 1 == chn_info.ddr_idx)) {
                        layout_info[l].ref_pool[idx].max_chn = chn_info.max_chn;
                        layout_info[l].ref_pool[idx].max_fps = chn_info.max_fps;
                        layout_info[l].ref_pool[idx].ddr_idx = chn_info.ddr_idx;
                        chn_cnt += chn_info.max_chn;
                    }
                    else {
                        mcp100_err("wrong input: res name %s, ddr_idx %d\n", res_name, chn_info.ddr_idx);
                    }
                    while (',' != line[i] && i < line_size)  i++;
                    if (i == line_size)
                        break;
                }
                layout_info[l].max_channel = chn_cnt;
                l++;
                
            }
            break;
        }
    }
    filp_close(fd, NULL);
    set_fs(fs);
    if (0 == l) {
        mcp100_err("%s does not declare buffer layout of decoder\n", GMLIB_FILENAME);
    }
    return l;
}
#endif

static int dec_parse_spec_cfg(void)
{
    char line[MAX_LINE_CHAR];
    char filename[0x80];
    int res_idx_order[MAX_RESOLUTION];
    mm_segment_t fs;
    struct file *fd = NULL;
    unsigned long long offset = 0;
    int i, l = 0;
    int res_num = 0;
    int ret;

#ifdef USE_GMLIB_CFG
    if ((l = parse_dec_layout_gmlib_cfg() > 0)) {
        return l;
    }
    l = 0;
#endif
    fs = get_fs();
    set_fs(get_ds());
    sprintf(filename, "%s/%s", config_path, SPEC_FILENAME);
    fd = filp_open(filename, O_RDONLY, S_IRWXU);
    if (IS_ERR(fd)) {
        mcp100_err("open %s error\n", filename);
        damnit(MCP100_MODULE_NAME);
        return -1;
    }
    for (i = 0; i < MAX_RESOLUTION; i++)
        res_idx_order[i] = -1;

    while ((ret = readline(fd, sizeof(line), line, &offset))) {
        if (strstr(line, "[PLAYBACK]")) {
            if (0 == readline(fd, sizeof(line), line, &offset))    // resolution type
                break;
            // read resolution idx
            res_num = parse_buffer_type(line, res_idx_order);
            l = 0;
            while (readline(fd, sizeof(line), line, &offset)) {
                if (strstr(line, "CH_")) {
                    parse_buffer_num(line, &layout_info[l], res_idx_order);
                    l++;
                }
                else
                    break;
            }
            break;
        }
    }
    filp_close(fd, NULL);
    set_fs(fs);

    return l;
}

int dec_dump_ref_pool(char *str)
{
    int res;
    int cnt = 0;
    int len = 0;
    struct ref_item_t *buf;
    unsigned long flags;
    
    dmp_ref("Decoder Reference Pool");
    spin_lock_irqsave(&mcp100_pool_lock, flags);
    dmp_ref("Avail:");
    dmp_ref(" id    addr_virt      addr_phy       size");
    dmp_ref("====  ============  ============  ==========");
    for (res = 0; res < MAX_RESOLUTION; res++) {
        if (ref_pool[res].buf_num > 0) {
            buf = ref_pool[res].avail;
            cnt = 0;
            while (buf) {
                dmp_ref("%3d    0x%08x    0x%08x    %8d", buf->buf_idx, buf->addr_va, buf->addr_pa, buf->size);
                buf = buf->next;
            }
        }
    }
    dmp_ref("Allocated:");
    dmp_ref(" id    addr_virt      addr_phy       size");
    dmp_ref("====  ============  ============  ==========");
    for (res = 0; res < MAX_RESOLUTION; res++) {
        if (ref_pool[res].buf_num > 0) {
            buf = ref_pool[res].alloc;
            cnt = 0;
            while (buf) {
                dmp_ref("%3d    0x%08x    0x%08x    %8d", buf->buf_idx, buf->addr_va, buf->addr_pa, buf->size);
                buf = buf->next;
            }
        }
    }
    spin_unlock_irqrestore(&mcp100_pool_lock, flags);
    return len;
}

int dec_dump_chn_pool(char *str)
{
    int i, chn;
    struct ref_item_t *ref;
    int cnt;
    int len = 0;
    unsigned long flags;
    char suit_res_name[6], res_name[6];

    dmp_ref("Decoder channel pool");
    dmp_ref(" chn     res     s.res     addr_virt      addr_phy       size");
    dmp_ref("=====  =======  =======   ============  ============  ==========");
    spin_lock_irqsave(&mcp100_pool_lock, flags);
    for (chn = 0; chn < max_chn; chn++) {
        cnt = 0;
        if (chn_pool[chn].res_type >= 0 && chn_pool[chn].res_type < MAX_RESOLUTION)
            strcpy(res_name, res_base_info[chn_pool[chn].res_type].name);
        else
            strcpy(res_name, "NULL");
        if (chn_pool[chn].suitable_res_type >= 0 && chn_pool[chn].suitable_res_type < MAX_RESOLUTION)
            strcpy(suit_res_name, res_base_info[chn_pool[chn].suitable_res_type].name);
        else
            strcpy(suit_res_name, "NULL");
        for (i = 0; i < MAX_NUM_REF_FRAMES + 1; i++) {
            if (chn_pool[chn].alloc_buf[i]) {
                ref = chn_pool[chn].alloc_buf[i];
                dmp_ref(" %3d    %5s    %5s     0x%08x    0x%08x    %8d", chn, 
                    res_name, suit_res_name, ref->addr_va, ref->addr_pa, ref->size);
                cnt++;
            }
        }
        if (0 == cnt && chn_pool[chn].suitable_res_type >= 0)
            dmp_ref(" %3d    %5s    %5s     ----------    ----------      -----", chn, res_name, suit_res_name);
    }
    spin_unlock_irqrestore(&mcp100_pool_lock, flags);
    return len;
}

int dec_dump_pool_num(char *str)
{
    int res;
    int size, cnt;
    int total_size = 0;
    int len = 0;
    struct ref_item_t *buf;

    dmp_ref("decoder allocate reference buffer 0x%x/0x%x, size %d", 
        mp4d_ref_buffer.addr_va, mp4d_ref_buffer.addr_pa, mp4d_ref_buffer.size);
    
    for (res = 0; res < MAX_RESOLUTION; res++) {
        if (ref_pool[res].buf_num > 0) {
            size = 0;
            cnt = 0;
            buf = ref_pool[res].avail;
            while (buf) {
                size += buf->size;
                cnt++;
                buf = buf->next;                
            }
            buf = ref_pool[res].alloc;
            while (buf) {
                size += buf->size;
                cnt++;
                buf = buf->next;                
            }
            total_size += size;
            dmp_ref("\t%s: num %d, size %d", res_base_info[res].name, cnt, size);
        }
    }
    dmp_ref("\t\tmpeg4 decoder total size %d byte (%d.%dM)\n", total_size, total_size / 1048576, (total_size%1048576)*1000/1048576);
    return len;
}

int dec_dump_layout_info(char *str)
{
    int l, i, len = 0;
    int str_len = 0;
    char line[0x100];
    for (l = 0; l < MAX_LAYOUT_NUM; l++) {
        if (layout_info[l].max_channel > 0) {
            dmp_ref("========== decoder layout %d ==========", l);
            str_len = 0;
            for (i = 0; i < MAX_RESOLUTION; i++)
                str_len += sprintf(line + str_len, "%6s", res_base_info[i].name);
            dmp_ref("%s", line);
            str_len = 0;
            for (i = 0; i < MAX_RESOLUTION; i++)
                str_len += sprintf(line + str_len, "%6d", layout_info[l].ref_pool[i].max_chn);
            dmp_ref("%s", line);
        }
    }
    return len;
}

static int insert_buf_to_pool(struct ref_item_t **head, struct ref_item_t *buf)
{
    if (NULL == (*head)) {
        (*head) = buf;
    }
    else {
        (*head)->prev = buf;
        buf->next = (*head);
        buf->prev = NULL;
        (*head) = buf;
    }
    return 0;
}

static int remove_buf_from_pool(struct ref_item_t **head, struct ref_item_t *buf)
{
    struct ref_item_t *next, *prev;
    next = buf->next;
    prev = buf->prev;
    if (next) {
        next->prev = prev;
        // be head
        if (NULL == prev)
            (*head) = next;
    }
    if (prev)
        prev->next = next;
    if (NULL == next && NULL == prev)
        (*head) = NULL;
    buf->next = buf->prev = NULL;
    return 0;
}

static int put_buf_to_chn_pool(struct channel_ref_buffer_t *pool, struct ref_item_t *ref)
{
    int i, idx = pool->head;
    // select one to buf: FIFO
    //for (i = 0; i < MAX_NUM_REF_FRAMES + 1; i++) {
    for (i = 0; i < pool->max_num; i++) {
        if (NULL == pool->alloc_buf[idx]) {
            pool->alloc_buf[idx] = ref;
            pool->head = (pool->head + 1)%pool->max_num;
            pool->used_num++;
            return 0;
        }
        idx = (idx + 1)%pool->max_num;
    }
    
    // not find empty buffer
    mcp100_err("put buf to pool error\n");
    mem_error();

    return -1;
}

static struct ref_item_t *remove_buf_from_chn_pool(struct channel_ref_buffer_t *pool, unsigned int addr_va)
{
    int i, idx = pool->tail;
    struct ref_item_t *ref;
    for (i = 0; i < pool->max_num; i++) {
        if (pool->alloc_buf[idx] && pool->alloc_buf[idx]->addr_va == addr_va) {
            ref = pool->alloc_buf[idx];
            pool->alloc_buf[idx] = NULL;
            pool->used_num--;
            pool->tail = (pool->tail + 1)%pool->max_num;
            return ref;
        }
        idx = (idx + 1)%pool->max_num;
    }
    
    // not find this buffer
    mem_error();
    return NULL;
}

static int set_current_layout(int layout_idx)
{
    int i;
    //unsigned long flags;
    if (layout_info[layout_idx].max_channel == 0) {
        mcp100_err("not such layout %d\n", layout_idx);
        return -1;
    }
    //spin_lock_irqsave(&mcp100_pool_lock, flags);
    if (cur_layout < 0 || cur_layout != layout_idx) {
        for (i = 0; i < MAX_RESOLUTION; i++) {
            // check all buffer are avail
            if (cur_layout >= 0) {
                if (ref_pool[i].alloc) {
                    mcp100_err("layout change but there are some buffers not released!\n");
                    mem_error();
                }
            }
            memcpy(&ref_pool[i], &layout_info[layout_idx].ref_pool[i], sizeof(struct ref_pool_t));
        }
    }
    cur_layout = layout_idx;
    //spin_unlock_irqrestore(&mcp100_pool_lock, flags);
    return 0;
}

static int filter_over_res_layout(int max_width, int max_height, int overspec_handle)
{
    int l, i;
    int counter = 0;
    int max_res_type;

    for (max_res_type = MAX_RESOLUTION - 1; max_res_type >= 0; max_res_type--) {
        if (res_base_info[max_res_type].width >= max_width && res_base_info[max_res_type].height >= max_height)
            break;
    }
    // mp4_overspec_handle: 0: no activity (filter), 1: keep all channel
    for (l = 0; l < MAX_LAYOUT_NUM; l++) {
        if (layout_info[l].max_channel <= 0)
            continue;
        // merge 2M & 1080P
        layout_info[l].ref_pool[RES_1080P].max_chn += layout_info[l].ref_pool[RES_2M].max_chn;
        layout_info[l].ref_pool[RES_2M].max_chn = 0;
        
        for (i = 0; i < MAX_RESOLUTION; i++) {
            if (layout_info[l].ref_pool[i].max_chn > 0) {
                if (res_base_info[i].width*res_base_info[i].height > max_width * max_height) {
                //if (res_base_info[i].width >= 2048)
                    if (overspec_handle)
                        counter += layout_info[l].ref_pool[i].max_chn;
                    layout_info[l].ref_pool[i].max_chn = 0;
                }
            }
        }
        if (overspec_handle && counter > 0) {
            layout_info[l].ref_pool[max_res_type].max_chn += counter;
        }
    }
    return 0;
}

/*
static int add_snapshot_channel(int ss_chn)
{
    int l, res_type;

    for (res_type = MAX_RESOLUTION - 1; res_type >= 0; res_type--) {
        if (res_base_info[res_type].width >= h264e_max_width && res_base_info[res_type].height >= h264e_max_height)
            break;
    }
    if (res_base_info[res_type].width < h264e_max_width && res_base_info[res_type].height < h264e_max_height) {
        mcp100_err("resolution is over max resolution, %d x %d > %d x %d\n", 
            h264e_max_width, h264e_max_height, res_base_info[res_type].width, res_base_info[res_type].height);
        return -1;
    }
    for (l = 0; l < MAX_LAYOUT_NUM; l++) {
        if (layout_info[l].max_channel > 0)
            layout_info[l].ref_pool[res_type].max_chn += ss_chn;
    }
    
    return 0;
}
*/

static int allocate_pool_buffer_from_frammap(struct mem_buffer_info_t *buf, int alloc_size, int ddr_idx)
{
    struct frammap_buf_info buf_info;

    memset(&buf_info, 0, sizeof(struct frammap_buf_info));
    buf_info.size = alloc_size;
    buf_info.align = 32;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
    buf_info.name = "mpeg4";
#endif
    buf_info.alloc_type = ALLOC_NONCACHABLE;
    if (1 == ddr_idx) {
        if (frm_get_buf_ddr(DDR_ID1, &buf_info) < 0) {
            mcp100_err("allocate reference buffer pool from DDR1 error\n");
            return -1;
        }
    }
    else {
        if (frm_get_buf_ddr(DDR_ID_SYSTEM, &buf_info) < 0) {
            mcp100_err("allocate reference buffer pool from DDR0 error\n");
            return -1;
        }
    }
    if (0 == buf_info.va_addr) {
        mcp100_err("allocate reference buffer pool error\n");
        return -1;
    }
    buf->addr_va = (unsigned int)buf_info.va_addr;
    buf->addr_pa = (unsigned int)buf_info.phy_addr;
    buf->size = buf_info.size;
    return 0;
}

static int fill_ref_item(struct mem_buffer_info_t *ref_buf, struct mem_buffer_info_t *mb_buf, 
    struct layout_info_t *layout, struct ref_item_t *buf_item)
{
    int l, res, i;
    int idx = 0;
    unsigned int ref_base_va, ref_base_pa;
    unsigned int mb_base_va, mb_base_pa;
    struct ref_pool_t *pool;
    unsigned int ref_size, mb_size = 0;
    int buf_num;

    for (l = 0; l < MAX_LAYOUT_NUM; l++) {
        if (layout[l].max_channel <= 0)
            continue;
        ref_base_va = ref_buf->addr_va;
        ref_base_pa = ref_buf->addr_pa;
        if (mb_buf) {
            mb_base_va = mb_buf->addr_va;
            mb_base_pa = mb_buf->addr_pa;
        }
        else {
            mb_base_va = 
            mb_base_pa = 0;
        }
        for (res = 0; res < MAX_RESOLUTION; res++) {
            pool = &layout[l].ref_pool[res];
            pool->res_type = res;
            pool->buf_num = 
            pool->avail_num = 
            pool->used_num = 0;
            pool->avail = 
            pool->alloc = NULL;

            if (0 == pool->max_chn)
                continue;

            if (mb_base_va) {
                ref_size = get_one_buffer_size(res_base_info[res].width, res_base_info[res].height, IDX_REF_BUF);
                mb_size = get_one_buffer_size(res_base_info[res].width, res_base_info[res].height, IDX_MB_BUF);
            }
            else {
                ref_size = get_one_buffer_size(res_base_info[res].width, res_base_info[res].height, IDX_DEC_REF_BUF);
                mb_size = 0;
            }
            //buf_num = pool->max_chn * (max_ref_num + 1);
            buf_num = get_pool_buf_num(pool);
            for (i = 0; i < buf_num; i++) {
                buf_item[idx].addr_va = ref_base_va;
                buf_item[idx].addr_pa = ref_base_pa;
                buf_item[idx].size = ref_size;
                ref_base_va += ref_size;
                ref_base_pa += ref_size;

                if (mb_base_va) {
                    buf_item[idx].mb_addr_va = mb_base_va;
                    buf_item[idx].mb_addr_pa = mb_base_pa;
                    buf_item[idx].mb_size = mb_size;
                    mb_base_va += mb_size;
                    mb_base_pa += mb_size;
                }

                buf_item[idx].res_type = res;
                buf_item[idx].buf_idx = idx;
                insert_buf_to_pool(&pool->avail, &buf_item[idx]);
                pool->buf_num++;
                pool->avail_num++;

                idx++;

                if (ref_base_va > ref_buf->addr_va + ref_buf->size) {
                    mcp100_err("ref buffer management out of range\n");
                    return -1;
                }
                if (mb_base_va && mb_base_va > mb_buf->addr_va + mb_buf->size) {
                    mcp100_err("mb buffer management out of range\n");
                    return -1;
                }
            }        
        }
    }
    return 0;
}

int dec_clean_mem_pool(void)
{
    if (chn_pool)
        kfree(chn_pool);
    chn_pool = NULL;
    if (ref_item)
        kfree(ref_item);
    ref_item = NULL;
    if (mp4d_ref_buffer.addr_va)
        frm_free_buf_ddr((void *)mp4d_ref_buffer.addr_va);
    return 0;
}

int dec_init_mem_pool(int max_width, int max_height)
{
    int l, i;
    int ddr_idx;
    int largest_layout_idx = 0;
    unsigned int total_ref_size = 0;
    unsigned int max_ref_size = 0;
    unsigned int total_ref_buf_num = 0;

    if (max_ref_num > MAX_NUM_REF_FRAMES) {
        mcp100_err("max referecne number(%d) can not exceed %d\n", max_ref_num, MAX_NUM_REF_FRAMES);
        return -1;
    }
    
    //spin_lock_init(&mcp100_pool_lock);

    chn_pool = kzalloc(sizeof(struct channel_ref_buffer_t) * max_chn, GFP_KERNEL);
    if (NULL == chn_pool) {
        mcp100_err("Fail to allocate chn_pool!\n");
        goto init_mem_pool_fail;
    }

    // 0. reset global parameter
    memset(layout_info, 0, sizeof(struct layout_info_t) * MAX_LAYOUT_NUM);
    //memset(ref_pool, 0, sizeof(struct ref_pool_t) * MAX_RESOLUTION);
    //memset(chn_pool, 0, sizeof(struct channel_ref_buffer_t) * max_chn);

    // 1. load spec configure
    if (0 == dec_parse_spec_cfg()) {
        mcp100_err("parsing buffer configure error\n");
        goto init_mem_pool_fail;
    }
    //dec_dump_layout_info(NULL);

    // 2. load param (ddr index)
    if (parse_param_cfg(&ddr_idx, NULL, TYPE_MPEG4_DECODER) < 0) {
        mcp100_err("parsing ddr index error\n");
        goto init_mem_pool_fail;
    }

    //filter_over_res_layout(max_width, max_height);
    filter_over_res_layout(((max_width+15)/16)*16, ((max_height+15)/16)*16, mp4_overspec_handle);

    // 2-1. add extra buffer of max reoslution
    if (0 == mp4_tight_buf) {
        for (l = 0; l < MAX_LAYOUT_NUM; l++) {
            if (layout_info[l].max_channel <= 0)
                continue;
            for (i = 0; i < MAX_RESOLUTION; i++) {
                if (layout_info[l].ref_pool[i].max_chn > 1) {
                    layout_info[l].ref_pool[i].max_chn += MAX_ENGINE_NUM;
                    break;
                }
            }
        }
    }
    //dec_dump_layout_info(NULL);

    // 3. compute number of reference buffer
    // encoder
    total_ref_buf_num = 0;
    for (l = 0; l < MAX_LAYOUT_NUM; l++) {
        int buf_num;
        if (layout_info[l].max_channel <= 0)
            continue;
        layout_info[l].buf_num = 0;
        layout_info[l].index = l;
        total_ref_size = 0;
        
        // can not reset ref pool because max_chn contains info
        for (i = 0; i < MAX_RESOLUTION; i++) {
            if (layout_info[l].ref_pool[i].max_chn) {
                //buf_num = layout_info[l].ref_pool[i].max_chn * (max_ref_num + 1);
                buf_num = get_pool_buf_num(&layout_info[l].ref_pool[i]);                
                total_ref_size += get_one_buffer_size(res_base_info[i].width, res_base_info[i].height, IDX_DEC_REF_BUF) * buf_num;
                total_ref_buf_num += buf_num;
                layout_info[l].buf_num += buf_num;
            }
        }
        if (total_ref_size > max_ref_size) {
            max_ref_size = total_ref_size;
            largest_layout_idx = l;
        }
    }
    ref_item = kzalloc(sizeof(struct ref_item_t) * total_ref_buf_num, GFP_KERNEL);
    if (NULL == ref_item) {
        mcp100_err("allocate reference item error\n");
        goto init_mem_pool_fail;
    }        
    memset(ref_item, 0 , sizeof(struct ref_item_t) * total_ref_buf_num);
    memset(ref_pool, 0 , sizeof(ref_pool));
    memset(chn_pool, 0 , sizeof(chn_pool));

    // 4. allocate buffer
    if (max_ref_size > 0) {
        if (allocate_pool_buffer_from_frammap(&mp4d_ref_buffer, max_ref_size, ddr_idx) < 0)
            goto init_mem_pool_fail;
    }
    // 5. buffer assignment
   
    if (fill_ref_item(&mp4d_ref_buffer, NULL, layout_info, ref_item) < 0)
        goto init_mem_pool_fail;
    set_current_layout(largest_layout_idx);
    //dec_dump_layout_info(NULL);
    //dec_dump_pool_num(ddr_idx);
    //dec_dump_ref_pool(NULL, 0);

    // 6. channel pool init
    for (i = 0; i < max_chn; i++) {
        // encoder
        chn_pool[i].chn = i;
        chn_pool[i].res_type = -1;
        chn_pool[i].max_num = max_ref_num + 1;
        chn_pool[i].suitable_res_type = -1;
    }

    return 0;

init_mem_pool_fail:
    dec_clean_mem_pool();
    damnit(MCP100_MODULE_NAME);
    return -1;
}

/* 
 * accumulate from smaller buffer to larger buffer & check if suitable buffer over spec
*/
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
            //dec_dump_ref_pool(NULL);
            //dec_dump_chn_pool(NULL);
            return -1;
        }
    }
    return 0;
}

static int get_suitable_resolution(int width, int height)
{
    int i;
    for (i = MAX_RESOLUTION - 1; i >= 0; i--) {
        if (ref_pool[i].max_chn > 0 && res_base_info[i].width >= width && res_base_info[i].height >= height)
            return i;
    }
    return -1;
}
/*
 * Return -1: can not find suitable buffer pool
 */
int dec_register_ref_pool(unsigned int chn, int width, int height)
{
    int i, j;
    unsigned long flags;

    if (chn >= max_chn) {
        mcp100_err("channel id(%d) over max channel\n", chn);
        damnit(MCP100_MODULE_NAME);
        return MEM_ERROR;
    }
    chn_pool[chn].suitable_res_type = get_suitable_resolution(width, height);
    // not suitable 
    if (chn_pool[chn].suitable_res_type < 0) {
        mcp100_err("{chn%d} no suitable buffer for %d x %d\n", chn, width, height);
        mem_error();
        return NO_SUITABLE_BUF;
    }

    if (check_over_spec() < 0) {
        chn_pool[chn].suitable_res_type = -1;
        mcp100_wrn("{chn%d} allocated buffer over spec, %d x %d\n", chn, width, height);
        return OVER_SPEC;
    }
    spin_lock_irqsave(&mcp100_pool_lock, flags);

    for (i = MAX_RESOLUTION - 1; i >= 0; i--) {
        if (ref_pool[i].max_chn && res_base_info[i].width >= width && res_base_info[i].height >= height) {
            // if register number hit max channel => register larger pool
            if (ref_pool[i].reg_num + 1 > ref_pool[i].max_chn) {
                //mcp100_wrn("{chn%d} pool %s is overflow, use larger pool\n", chn, res_base_info[i].name);
                continue;
            }
            // previous channel is not de-register
            if (chn_pool[chn].res_type >= 0) {
                // check all ref buffers are released
                for (j = 0; j <= MAX_NUM_REF_FRAMES; j++) {
                    if (chn_pool[chn].alloc_buf[j]) {
                        mcp100_err("chn%d ref buffer %d is not free\n", chn, chn_pool[chn].alloc_buf[j]->buf_idx);
                        mem_error();
                        spin_unlock_irqrestore(&mcp100_pool_lock, flags);
                        return MEM_ERROR;
                    }
                }
                ref_pool[chn_pool[chn].res_type].reg_num--;                
            }

            chn_pool[chn].res_type = i;
            ref_pool[i].reg_num++;

            mcp100_info("{chn%d} resolution = %d x %d, register %s (suit %s)\n", chn, width, height, 
                res_base_info[i].name, res_base_info[chn_pool[chn].suitable_res_type].name);
            spin_unlock_irqrestore(&mcp100_pool_lock, flags);
            return i;
        }
    }
    mcp100_wrn("{chn%d} can not find suitable pool for resolution %d x %d\n", chn, width, height);
    chn_pool[chn].suitable_res_type = -1;
    spin_unlock_irqrestore(&mcp100_pool_lock, flags);
    return NOT_OVER_SPEC_ERROR;
}

int dec_deregister_ref_pool(unsigned int chn)
{
    unsigned long flags;
    int j;
    if (chn >= max_chn) {
        mcp100_err("deregister: channel id(%d) over max channel\n", chn);
        damnit(MCP100_MODULE_NAME);
        return -1;
    }
    spin_lock_irqsave(&mcp100_pool_lock, flags);

    if (chn_pool[chn].res_type >= 0) {
        // check all ref buffers are released
        if (chn_pool[chn].used_num) {
            mcp100_err("chn%d is not free all reference\n", chn);
            mem_error();
            spin_unlock_irqrestore(&mcp100_pool_lock, flags);
            return -1;
        }
        for (j = 0; j <= MAX_NUM_REF_FRAMES; j++) {
            if (chn_pool[chn].alloc_buf[j]) {
                mcp100_err("chn%d ref buffer %d is not free\n", chn, chn_pool[chn].alloc_buf[j]->buf_idx);
                mem_error();
                spin_unlock_irqrestore(&mcp100_pool_lock, flags);
                return -1;
            }
        }
        mcp100_info("{chn%d} deregister %s\n", chn, res_base_info[chn_pool[chn].res_type].name);
        ref_pool[chn_pool[chn].res_type].reg_num--;
        chn_pool[chn].res_type = -1;
        chn_pool[chn].suitable_res_type = -1;
    }
    spin_unlock_irqrestore(&mcp100_pool_lock, flags);
    //dec_dump_ref_pool(NULL, 0);
    //dec_dump_chn_pool(NULL, 0);
    return 0;
}

/*
 * check buffer pool suitable: register buffer pool & suitable buffer pool
*/
int dec_check_reg_buf_suitable(unsigned char *checker)
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

int dec_mark_suitable_buf(int chn, unsigned char *checker, int width, int height)
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

int dec_allocate_pool_buffer2(struct ref_buffer_info_t *buf, unsigned int res_idx, int chn)
{
    struct ref_item_t *ref;
    unsigned long flags;
    
    if (res_idx >= MAX_RESOLUTION) {
        mcp100_err("{chn%d} allocate unknown resolution idx %d\n", chn, res_idx);
        mem_error();
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

    buf->ref_virt = ref->addr_va;
    buf->ref_phy = ref->addr_pa;
    //buf->mb_virt = ref->mb_addr_va;
    //buf->mb_phy = ref->mb_addr_pa;

    spin_unlock_irqrestore(&mcp100_pool_lock, flags);

    //dec_dump_ref_pool(NULL, 0);
    //dec_dump_chn_pool(NULL, 0);

    return 0;       
}

int dec_release_pool_buffer2(struct ref_buffer_info_t *buf, int chn)
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
    res_idx = chn_pool[chn].res_type;
    ref = remove_buf_from_chn_pool(&chn_pool[chn], buf->ref_virt);
    remove_buf_from_pool(&ref_pool[res_idx].alloc, ref);
    ref_pool[res_idx].used_num--;
    insert_buf_to_pool(&ref_pool[res_idx].avail, ref);
    ref_pool[res_idx].avail_num++;
    spin_unlock_irqrestore(&mcp100_pool_lock, flags);
    
    //dec_dump_ref_pool(NULL, 0);
    //dec_dump_chn_pool(NULL, 0);

    return 0;
}


