#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <mach/fmem.h>
#include <mach/ftpmu010.h>
#include <mach/platform/platform_io.h>
#include <mach/gm_jiffies.h>
#include "dmac030.h"
#include "scl_dma.h"
#include <log.h>    //include log system "printm","damnit"...

#define _TIME_DIFF(t2, t1)   ((long)(t2) - (long)(t1))
#define DMA_WRITE_BIT   (0x1 << 0)
#define DUMMY_READ_BIT  (0x1 << 1)

static int splitarea_height = 200;
static int align64_flag = 0;    /* default is disabled */

#define DMA_SPLIT_AREA      (480*splitarea_height*2)
#define DMA_CH              3   /* don't change this value */
//#define TEST_VERSION

static unsigned int     debug_value = 0;

#define IRQ_NUM         CPU_INT_9
#define LIST_LOCK(x)    spin_lock_irqsave(&g_dma_info.lock, x)
#define LIST_UNLOCK(x)  spin_unlock_irqrestore(&g_dma_info.lock, x)
#define IS_PARENT(x)    ((x)->parent == NULL)
#define DENGINE_BUSY     g_dma_info.engine_busy
#define ENGINE_NOT_BUSY (g_dma_info.engine_busy == 0)

struct dma_info_s {
    void __iomem *base;
    unsigned int irq;
	struct dma_pool *dma_desc_pool;
	spinlock_t lock;
	struct list_head active_list;   /* list for trigger or triggered already */
	struct list_head inactive_list; /* list for wait to trigger */
	int engine_busy;
    int job_id_fired;

	/* statistic counter */
	u32 trigger_cnt;
	u32 trigger_fail_cnt;
	u32 trigger_finish_cnt;
	u32 align_fault_cnt;
	u32 flush_cnt;
	u32 flush_fail_cnt;
	u32 put_cnt;
	u32 put_fail_cnt;
	u32 skip_cnt;
	u32 desc_alloc_cnt;
	u32 desc_alloc_fail;
	u32 split_cnt;
	u32 fatal_bug;
} g_dma_info;

typedef struct {
    struct dmac030_lld  lld;    /* hardware descriptor */
    struct list_head    child_list;
    struct list_head    sibling_list;   //list contains all root jobs
    dma_addr_t          desc_phys;
    int                 nr_child;
    int                 job_id;
    int                 child_seq_id;   //internal debug use
    void                *parent;
} scl_dma_desc_t;

static int read_idx = 0;
#define PAST_CNT     16
static int past_array[PAST_CNT];

/*
 * Local functions declaration
 */
static irqreturn_t dma030_scl_interrupt(int irq, void *dev_id);
static int proc_debug_read(char *page, char **start, off_t off, int count, int *eof, void *data);
static int proc_debug_write(struct file *file, const char *buffer, unsigned long count, void *data);
static int proc_split_height_read(char *page, char **start, off_t off, int count, int *eof, void *data);
static int proc_split_height_write(struct file *file, const char *buffer, unsigned long count, void *data);
static int proc_align64_read(char *page, char **start, off_t off, int count, int *eof, void *data);
static int proc_align64_write(struct file *file, const char *buffer, unsigned long count, void *data);
static u32 trigger_time, time_avg_val = 0;

static u64 dummy_addr[0x10];
/*
 * Local variables declaration
 */
static struct proc_dir_entry *debug_proc = NULL;
static struct proc_dir_entry *split_height_proc = NULL;
static struct proc_dir_entry *align64_proc = NULL;

/* find the root job */
static inline scl_dma_desc_t *scl_dma_get_parent_desc(int job_id)
{
    scl_dma_desc_t  *node, *desc = NULL;
    unsigned long flags;

    LIST_LOCK(flags);
    if (!list_empty(&g_dma_info.inactive_list)) {
        list_for_each_entry(node, &g_dma_info.inactive_list, sibling_list) {
            if (node->job_id != job_id)
                continue;
            desc = node;
            break;
        }
    }
    LIST_UNLOCK(flags);

    /* check again */
    if (desc && !IS_PARENT(desc))
        panic("%s, bug! node is not parent! \n", __func__);

    return desc;
}

/*
 * descpritor allocation function
 */
static inline scl_dma_desc_t *scl_dma_alloc_desc(int job_id)
{
    unsigned long flags;
    gfp_t gfp_flags;
    scl_dma_desc_t *desc, *parent;
    dma_addr_t phys;

    gfp_flags = (in_interrupt() || in_atomic()) ? GFP_ATOMIC : GFP_KERNEL;
    desc = (scl_dma_desc_t *)dma_pool_alloc(g_dma_info.dma_desc_pool, gfp_flags, &phys);
    if (!desc) {
        g_dma_info.desc_alloc_fail ++;
        printk("%s, desc allocation fail! \n", __func__);
        return NULL;
    }
    if (phys & 0x7)
        panic("%s, the llp addr 0x%x is not 8 alignment! \n", __func__, (u32)phys);

    memset((void *)desc, 0, sizeof(scl_dma_desc_t));
    INIT_LIST_HEAD(&desc->child_list);
    INIT_LIST_HEAD(&desc->sibling_list);
    desc->desc_phys = phys;
    desc->job_id = job_id;

    LIST_LOCK(flags);
    parent = scl_dma_get_parent_desc(job_id);
    if (parent) {
        if (list_empty(&parent->child_list)) {
            /* I am the first child */
            parent->lld.next_lld = (dma_addr_t)desc->desc_phys;
        } else {
            scl_dma_desc_t  *last_node;

            last_node = list_entry(parent->child_list.prev, scl_dma_desc_t, child_list);
            last_node->lld.next_lld = (dma_addr_t)desc->desc_phys;
        }
        /* add to parent's child list */
        list_add_tail(&desc->child_list, &parent->child_list);
        desc->child_seq_id = parent->nr_child + 1;  /* start from 1 */
        desc->parent = (void *)parent;
        parent->nr_child ++;
    } else {
        parent = desc;
        list_add_tail(&desc->sibling_list, &g_dma_info.inactive_list);
    }
    LIST_UNLOCK(flags);

    /* increase counter */
    g_dma_info.desc_alloc_cnt ++;

    return desc;
}

static inline void scl_dma_free_desc(scl_dma_desc_t *desc)
{
    dma_pool_free(g_dma_info.dma_desc_pool, desc, desc->desc_phys);
}

/*
 * Remove a group jobs with job_id
 */
static inline int scl_dma_remove_grpjob(struct list_head *gList, int job_id)
{
    scl_dma_desc_t  *node, *ne, *parent = NULL;
    unsigned long flags;
    int debug_cnt = 0;

    LIST_LOCK(flags);
    if (!list_empty(gList)) {
        list_for_each_entry(node, gList, sibling_list) {
            if (job_id != -1) {
                if (node->job_id != job_id)
                    continue;
            }
            parent = node;
            break;
        }
    }
    if (parent) {
        /* process child list first */
        if (!list_empty(&parent->child_list)) {
            list_for_each_entry_safe(node, ne, &parent->child_list, child_list) {
                list_del(&node->child_list);
                scl_dma_free_desc(node);
                debug_cnt ++;
            }
        }
        if (parent->nr_child != debug_cnt)
            panic("%s, bug! parent->nr_child: %d, debug_cnt: %d\n", __func__, parent->nr_child, debug_cnt);
        g_dma_info.desc_alloc_cnt -= debug_cnt;

        /* now process the parent itself */
        list_del(&parent->sibling_list);
        scl_dma_free_desc(parent);
        g_dma_info.desc_alloc_cnt --;
    }
    LIST_UNLOCK(flags);

    return parent ? 0 : -1;
}

static inline void dma_fire(scl_dma_desc_t *desc)
{
    u32 chan_base = 0x100 + 0x20 * DMA_CH;

    g_dma_info.job_id_fired = desc->job_id;

    iowrite32(DMA030_CONFIG_REG, g_dma_info.base + chan_base + 0x4);
    iowrite32(desc->desc_phys, g_dma_info.base + chan_base + 0x10); //LLP
    iowrite32(0, g_dma_info.base + chan_base + 0x14); //Transfer size
    iowrite32(desc->lld.ctrl, g_dma_info.base + chan_base);
    trigger_time = gm_jiffies;
}

/*
 * Init function
 */
int scl_dma_init(void)
{
    int ret;
    int i = 0;

    memset(&g_dma_info, 0, sizeof(struct dma_info_s));
    INIT_LIST_HEAD(&g_dma_info.active_list);
    INIT_LIST_HEAD(&g_dma_info.inactive_list);

    g_dma_info.base = (void *)ioremap(XDMAC_FTDMAC030_PA_BASE, PAGE_SIZE);
    if (g_dma_info.base == NULL)
        panic("%s, ioremap fail! \n", __func__);
    g_dma_info.irq = IRQ_NUM;
    ret = request_irq(g_dma_info.irq, dma030_scl_interrupt, 0, "scl_2ddma", NULL);
    if (ret < 0)
        panic("%s, fail to request sw irq%d \n", __func__, g_dma_info.irq);

    /* create a pool of consistent memory blocks for hardware descriptors */
    g_dma_info.dma_desc_pool = dma_pool_create("scl2d_desc_pool",
                			NULL, sizeof(scl_dma_desc_t),
                			0x20 /* double word alignment */, 0);
	if (!g_dma_info.dma_desc_pool)
		panic("%s, No memory for descriptor pool!\n", __func__);

    debug_proc = create_proc_entry("scl_dma", S_IRUGO | S_IXUGO, NULL);
	if (debug_proc == NULL) {
		panic("Fail to create scl_dma_proc!\n");
		return -1;
	}
	debug_proc->read_proc = (read_proc_t *)proc_debug_read;
    debug_proc->write_proc = proc_debug_write;

    /* split height */
    split_height_proc = create_proc_entry("scl_dma_split_height", S_IRUGO | S_IXUGO, NULL);
	if (split_height_proc == NULL) {
		panic("Fail to create split_height_proc!\n");
		return -1;
	}
	split_height_proc->read_proc = (read_proc_t *)proc_split_height_read;
    split_height_proc->write_proc = proc_split_height_write;

    /* alignment 64 */
    align64_proc = create_proc_entry("scl_dma_align64", S_IRUGO | S_IXUGO, NULL);
	if (align64_proc == NULL) {
		panic("Fail to create align64_proc!\n");
		return -1;
	}
	align64_proc->read_proc = (read_proc_t *)proc_align64_read;
    align64_proc->write_proc = proc_align64_write;

    for (i = 0; i < PAST_CNT; i++)
        past_array[i] = 0xffff;

    printk("%s: 2ddma is enabled, align64: %d, harry ver-7...\n", __func__, align64_flag);

    return 0;
}

int scl_dma_check_job_count(int job_id)
{
    scl_dma_desc_t *node;
    unsigned long flags;
    int ret = 1, count = 0;

    LIST_LOCK(flags);

    list_for_each_entry(node, &g_dma_info.active_list, sibling_list) {
        count++;
        if (node->job_id == job_id) {
            ret = 0;
            break;
        }
    }
    LIST_UNLOCK(flags);
    return count;
}

/*
 * check job done or not
 * return 0 means job still at active list
 * return 1 means job done
 */
int scl_dma_check_job_done(int job_id)
{
    scl_dma_desc_t *node;
    unsigned long flags;
    int ret = 1;

    LIST_LOCK(flags);

    list_for_each_entry(node, &g_dma_info.active_list, sibling_list) {
        if (node->job_id == job_id) {
            ret = 0;
            break;
        }
    }
    LIST_UNLOCK(flags);

    return ret;
}

/*
 * Delete a group id
 * return 0 for success, others for fail
 */
int scl_dma_delete(int job_id)
{
    int ret;

    ret = scl_dma_remove_grpjob(&g_dma_info.inactive_list, job_id);

    if (ret)
        g_dma_info.flush_fail_cnt ++;
    else
        g_dma_info.flush_cnt ++;

    return ret;
}

void scl_dma_exit(void)
{
    int wait_cnt = 0;

    while (g_dma_info.desc_alloc_cnt && (wait_cnt < 200)) {
        wait_cnt ++;
        msleep(10);
    }

    if (wait_cnt >= 200)
        panic("%s, dma job not finish! \n", __func__);

    iounmap(g_dma_info.base);
    free_irq(g_dma_info.irq, NULL);
    remove_proc_entry("scl_dma", NULL);
    remove_proc_entry("scl_dma_split_height", NULL);
    remove_proc_entry("scl_dma_align64", NULL);
}

static inline int dsanity_check(scl_dma_t *scl_dma)
{
#ifndef TEST_VERSION
    static fmem_pci_id_t pci_id = -1;
    static fmem_cpu_id_t cpu_id = -1;

    if (pci_id == -1)
        fmem_get_identifier(&pci_id, &cpu_id);

    /* Only exeute in EP */
    if (pci_id == FMEM_PCI_HOST) {
        if (printk_ratelimit())
            printk(KERN_DEBUG"%s, error role. Don't execute 2D in RC!\n", __func__);
        g_dma_info.put_fail_cnt ++;
        return -1;
    }
#endif

    if (scl_dma->job_id < 0) {
        if (printk_ratelimit())
            printk(KERN_DEBUG"%s, error parameter. invalid job_id: %d \n", __func__, scl_dma->job_id);
        g_dma_info.put_fail_cnt ++;
        return -1;
    }

    if (!scl_dma->width || !scl_dma->height) {
        if (printk_ratelimit())
            printk(KERN_DEBUG"%s, error parameter. width: %d, heigth: %d \n", __func__, scl_dma->width,
                                scl_dma->height);
        g_dma_info.put_fail_cnt ++;
        return -1;
    }

    if ((scl_dma->x_offset + scl_dma->width) > scl_dma->bg_width) {
        if (printk_ratelimit())
            printk(KERN_DEBUG"%s, error parameter. x_offset: %d, width: %d, bg_width: %d \n", __func__,
                                scl_dma->x_offset,
            scl_dma->width, scl_dma->bg_width);
        g_dma_info.put_fail_cnt ++;
        return -1;
    }

    if ((scl_dma->y_offset + scl_dma->height) > scl_dma->bg_height) {
        if (printk_ratelimit())
            printk(KERN_DEBUG"%s, error parameter. y_offset: %d, height: %d, bg_height: %d \n", __func__,
                                scl_dma->y_offset,
            scl_dma->height, scl_dma->bg_height);
        g_dma_info.put_fail_cnt ++;
        return -1;
    }

    if (!scl_dma->src_addr || !scl_dma->dst_addr) {
        if (printk_ratelimit())
            printk(KERN_DEBUG"%s, error parameter. src_addr: 0x%x, dst_addr: 0x%x \n", __func__,
                                scl_dma->src_addr, scl_dma->dst_addr);
        g_dma_info.put_fail_cnt ++;
        return -1;
    }

    return 0;
}

static inline void scl_dma_add_dummycmd(int job_id, u32 next_dst)
{
    scl_dma_desc_t  *desc;

    desc = scl_dma_alloc_desc(job_id);
    if (desc == NULL) {
        panic("%s, no memory! \n", __func__);
        return;
    }
    desc->lld.src = next_dst;   //0xc1b00000;
    desc->lld.dst = __pa(dummy_addr);
    desc->lld.ctrl = 0x16c18106;//0x14a98100 | 0x6;
    desc->lld.cnt = 0x1;
    desc->lld.stride = 0x0;

    if (debug_value & DUMMY_READ_BIT) {
        printk("2DDMA DummyRead: Dummy-SRC:0x%x, Dummy-DST:0x%x, CNT:0x%x \n", desc->lld.src, desc->lld.dst, desc->lld.cnt);
    }
}

/* The function to add job
 */
int scl_dma_add(scl_dma_t *scl_dma)
{
    int offset, xcnt, shift;
    int xcnt_left;
    scl_dma_desc_t  *desc;
    u32 total_area, split_area, split_height, next_src, next_dst;
#define LEFTAREA_WIDTH  8   /* bytes */

    /* parameters sanity check */
    if (dsanity_check(scl_dma) < 0)
        return -1;

    desc = scl_dma_alloc_desc(scl_dma->job_id);
    if (desc == NULL)
        return -1;

    g_dma_info.put_cnt ++;

    offset = (scl_dma->bg_width << 1) * scl_dma->y_offset + (scl_dma->x_offset << 1); /* pixel to byte */
    desc->lld.src = scl_dma->src_addr + offset; /* byte */
    desc->lld.dst = scl_dma->dst_addr + offset; /* byte */

    xcnt = scl_dma->width << 1; /* pixel to bytes */

    if (debug_value & DMA_WRITE_BIT)
        printk("\n\n<-------- 2D DMA start to add commands --------> \n");
#if 1
    /* PCIe limitation with 64 bytes alignment + burst8 cannot across 128 boudnary */
    if (!align64_flag && (desc->lld.dst != AXI_ALIGN_64(desc->lld.dst))) {
        int loop = 0;

        g_dma_info.align_fault_cnt ++;

        xcnt_left = AXI_ALIGN_64(desc->lld.dst) - AXI_ALIGN_8(desc->lld.dst);
        do {
            desc->lld.src = AXI_ALIGN_8(desc->lld.src) + loop * LEFTAREA_WIDTH;
            desc->lld.dst = AXI_ALIGN_8(desc->lld.dst) + loop * LEFTAREA_WIDTH;

            if (!(xcnt_left & 0x7)) {
                desc->lld.ctrl |= DMAC030_CTRL_SRC_WIDTH_64; /* bits */
                desc->lld.ctrl |= DMAC030_CTRL_DST_WIDTH_64;
                shift = 3;
            } else if (!(xcnt_left & 0x3)) {
                desc->lld.ctrl |= DMAC030_CTRL_SRC_WIDTH_32; /* bits */
                desc->lld.ctrl |= DMAC030_CTRL_DST_WIDTH_32;
                shift = 2;
            } else if (!(xcnt_left & 0x1)) {
                desc->lld.ctrl |= DMAC030_CTRL_SRC_WIDTH_16; /* bits */
                desc->lld.ctrl |= DMAC030_CTRL_DST_WIDTH_16;
                shift = 1;
            } else {
                desc->lld.ctrl |= DMAC030_CTRL_SRC_WIDTH_8; /* bits */
                desc->lld.ctrl |= DMAC030_CTRL_DST_WIDTH_8;
                shift = 0;
            }

            /* NoInterrupt, ChEn, ExpEn, 2DEn, WSync */
            desc->lld.ctrl |= ((0x1 << 28) | (0x1 << 16) | (0x1 << 15) | (0x1 << 14) | (0x1 << 8) | 0x6);
            desc->lld.cnt =  (scl_dma->height << 16) | (LEFTAREA_WIDTH >> shift);
            desc->lld.stride = ((scl_dma->bg_width << 1) << 16) | (scl_dma->bg_width << 1);
            if (debug_value & DMA_WRITE_BIT) {
                printk("2DDMA Write: SRC:0x%x, DST:0x%x, X:%d, Y:%d, 2D-SRC:0x%x, 2D-DST:0x%x, CNT:0x%x (Not-Align-Area) \n",
                        scl_dma->src_addr + offset, scl_dma->dst_addr + offset, scl_dma->x_offset, scl_dma->y_offset,
                        desc->lld.src, desc->lld.dst, desc->lld.cnt);
            }

            scl_dma_add_dummycmd(scl_dma->job_id, desc->lld.dst);

            /* for the right area */
            desc = scl_dma_alloc_desc(scl_dma->job_id);
            if (desc == NULL)
                return -1;

            xcnt_left = xcnt_left - LEFTAREA_WIDTH;
            desc->lld.src = scl_dma->src_addr + offset; /* byte */
            desc->lld.dst = scl_dma->dst_addr + offset; /* byte */
            loop ++;
        } while (xcnt_left > 0);
    }
#endif

    if (desc->lld.dst != AXI_ALIGN_64(desc->lld.dst)) {
        int deviation = AXI_ALIGN_64(desc->lld.dst) - desc->lld.dst;

        desc->lld.src += deviation;
        desc->lld.dst += deviation;
        if (xcnt <= deviation) {
            if (printk_ratelimit())
                printk(KERN_DEBUG"%s, error! xcnt: %d, deviation: %d, ignore! \n", __func__, xcnt, deviation);
            xcnt = 0;
            g_dma_info.skip_cnt ++;
        } else {
            xcnt -= deviation;
        }
    }

    total_area = xcnt * scl_dma->height;
    split_area = total_area > DMA_SPLIT_AREA ? DMA_SPLIT_AREA : total_area;
    split_height = xcnt ? split_area / xcnt : 0;
    if (split_height == 0) {
        printk("%s, the split area is too small, area size: %d, width(bytes): %d \n", __func__,
                                                                            split_area, xcnt);
        return -1;
    }

    split_area = split_height * xcnt;

    do {
        if (!(xcnt & 0x7)) {
            desc->lld.ctrl |= DMAC030_CTRL_SRC_WIDTH_64; /* bits */
            desc->lld.ctrl |= DMAC030_CTRL_DST_WIDTH_64;
            shift = 3;
        } else if (!(xcnt & 0x3)) {
            desc->lld.ctrl |= DMAC030_CTRL_SRC_WIDTH_32; /* bits */
            desc->lld.ctrl |= DMAC030_CTRL_DST_WIDTH_32;
            shift = 2;
        } else if (!(xcnt & 0x1)) {
            desc->lld.ctrl |= DMAC030_CTRL_SRC_WIDTH_16; /* bits */
            desc->lld.ctrl |= DMAC030_CTRL_DST_WIDTH_16;
            shift = 1;
        } else {
            desc->lld.ctrl |= DMAC030_CTRL_SRC_WIDTH_8; /* bits */
            desc->lld.ctrl |= DMAC030_CTRL_DST_WIDTH_8;
            shift = 0;
        }

        /* NoInterrupt, ChEn, ExpEn, 2DEn, WSync */
        desc->lld.ctrl |= ((0x1 << 28) | (0x1 << 16) | (0x1 << 15) | (0x1 << 14) | (0x1 << 8) | 0x6);
        desc->lld.cnt = xcnt ? ((split_height << 16) | (xcnt >> shift)) : 0;
        desc->lld.stride = ((scl_dma->bg_width << 1) << 16) | (scl_dma->bg_width << 1);

        if (debug_value & DMA_WRITE_BIT) {
            printk("2DDMA Write: SRC:0x%x, DST:0x%x, 2D-SRC:0x%x, 2D-DST:0x%x, CNT:0x%x \n",
                    scl_dma->src_addr + offset, scl_dma->dst_addr + offset, desc->lld.src, desc->lld.dst, desc->lld.cnt);
        }

        next_src = desc->lld.src + (scl_dma->bg_width << 1) * split_height;
        next_dst = desc->lld.dst + (scl_dma->bg_width << 1) * split_height;

        total_area -= split_area;
        if (total_area) {
            /* add a dummy workaround command first */
            scl_dma_add_dummycmd(scl_dma->job_id, desc->lld.dst);

            /* prepare the next LLP command */
            split_area = total_area > split_area ? split_area : total_area;
            split_height = split_area / xcnt;
            desc = scl_dma_alloc_desc(scl_dma->job_id);
            if (desc == NULL)
                panic("%s, no memory for desc! \n", __func__);
            desc->lld.src = next_src;
            desc->lld.dst = next_dst;
            //increase statistic
            g_dma_info.split_cnt ++;
        } else {
            /* last command we still add dummy read */
            scl_dma_add_dummycmd(scl_dma->job_id, desc->lld.dst);
        }
    } while (total_area);

    return 0;
}

int scl_dma_trigger(int job_id)
{
    unsigned long flags;
    scl_dma_desc_t *parent, *last_node, *node;
    int i = 0, ret = 0;

    LIST_LOCK(flags);

    list_for_each_entry(node, &g_dma_info.active_list, sibling_list) {
        if (node->job_id == job_id) {
            ret = 1;
            break;
        }
    }
    /* if not in active_list, try to know if the job was done already. */
    if (!ret) {
        for (i = 0; i < PAST_CNT; i++) {
            if (past_array[i] != job_id)
                continue;
            ret = 1;
            break;
        }
    }

    if (ret == 1) {
        LIST_UNLOCK(flags);
        return 0;
    }

    LIST_UNLOCK(flags);

    parent = scl_dma_get_parent_desc(job_id);
    if (!parent) {
        g_dma_info.trigger_fail_cnt ++;
        return -1;
    }

    LIST_LOCK(flags);
    /* turn on the last node interrupt */
    last_node = list_entry(parent->child_list.prev, scl_dma_desc_t, child_list);
    last_node->lld.ctrl &= ~(0x1 << 28);   /* tcmask is off which will issue interrupt */
    /* delete from inactive list and add to active list */
    list_move_tail(&parent->sibling_list, &g_dma_info.active_list);
    if (ENGINE_NOT_BUSY) {
        DENGINE_BUSY = 1;
        dma_fire(parent);
    }
    LIST_UNLOCK(flags);

    g_dma_info.trigger_cnt ++;
    return 0;
}

static irqreturn_t dma030_scl_interrupt(int irq, void *dev_id)
{
    scl_dma_desc_t  *desc;
    u32 diff;

    if (dev_id) {}

    /* clear sw interrupt source */
    ftpmu010_clear_intr(irq);

    diff = _TIME_DIFF(gm_jiffies, trigger_time);
    if (time_avg_val == 0)
        time_avg_val = diff;
    else
        time_avg_val = (time_avg_val * 9 + diff) / 10;

    g_dma_info.trigger_finish_cnt ++;

    if (ENGINE_NOT_BUSY) {
        g_dma_info.fatal_bug ++;
        DENGINE_BUSY = 1;
        panic("%s, fatal bug1! \n", __func__);
    }

    /* get the first node from active list, it is done. */
    if (scl_dma_remove_grpjob(&g_dma_info.active_list, g_dma_info.job_id_fired)) {
        g_dma_info.fatal_bug ++;
        panic("%s, fatal bug2! \n", __func__);
    }

    past_array[read_idx] = g_dma_info.job_id_fired;
    read_idx ++;
    read_idx &= (PAST_CNT - 1);

    /* get the next one */
    if (!list_empty(&g_dma_info.active_list)) {
        desc = list_entry(g_dma_info.active_list.next, scl_dma_desc_t, sibling_list);
        dma_fire(desc);
    } else {
        DENGINE_BUSY = 0;
    }

    return IRQ_HANDLED;
}

/* dump the debug information */
void scl_dma_dump(char *module_name)
{
    int i;
    scl_dma_desc_t *node;
    unsigned long flags;

    if (module_name != NULL) {
        printm(module_name, "scl_dma: DENGINE_BUSY: %d, trigger time: 0x%x, now: 0x%x \n", DENGINE_BUSY, trigger_time, gm_jiffies);
        printm(module_name, "scl_dma: current trigger jid: %d \n", g_dma_info.job_id_fired);

        LIST_LOCK(flags);
        list_for_each_entry(node, &g_dma_info.active_list, sibling_list)
            printm(module_name, "scl_dma: active_list, node->job_id: %d \n", node->job_id);

        if (!list_empty(&g_dma_info.inactive_list)) {
            list_for_each_entry(node, &g_dma_info.inactive_list, sibling_list)
                printm(module_name, "scl_dma: inactive_list, node->job_id: %d \n", node->job_id);
        }
        printm(module_name, "scl_dma: pass array: \n");
        for (i = 0; i < PAST_CNT; i ++)
            printm(module_name, "%d   \n", past_array[i]);
        printm(module_name, "\n");
        LIST_UNLOCK(flags);
    } else {
        printk("scl_dma: DENGINE_BUSY: %d, trigger time: 0x%x, now: 0x%x \n", DENGINE_BUSY, (u32)trigger_time, (u32)gm_jiffies);
        printk("scl_dma: current trigger jid: %d \n", g_dma_info.job_id_fired);

        LIST_LOCK(flags);
        list_for_each_entry(node, &g_dma_info.active_list, sibling_list)
            printk("scl_dma: active_list, node->job_id: %d \n", node->job_id);

        if (!list_empty(&g_dma_info.inactive_list)) {
            list_for_each_entry(node, &g_dma_info.inactive_list, sibling_list)
                printk("scl_dma: inactive_list, node->job_id: %d \n", node->job_id);
        }
        printk("scl_dma: pass array: \n");
        for (i = 0; i < PAST_CNT; i ++)
            printk("%d   \n", past_array[i]);
        printk("\n");
        LIST_UNLOCK(flags);
    }
}

static int proc_debug_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    unsigned int len = 0;

    len += sprintf(page + len, "engine busy: %d \n", DENGINE_BUSY);
    len += sprintf(page + len, "put_cnt: %d \n", g_dma_info.put_cnt);
    len += sprintf(page + len, "put_fail_cnt: %d \n", g_dma_info.put_fail_cnt);
    len += sprintf(page + len, "trigger_cnt: %d \n", g_dma_info.trigger_cnt);
    len += sprintf(page + len, "trigger_fail_cnt: %d \n", g_dma_info.trigger_fail_cnt);
    len += sprintf(page + len, "trigger_finish_cnt: %d \n", g_dma_info.trigger_finish_cnt);
    len += sprintf(page + len, "8bytes align_fault_cnt: %d \n", g_dma_info.align_fault_cnt);
    len += sprintf(page + len, "flush_cnt: %d \n", g_dma_info.flush_cnt);
    len += sprintf(page + len, "flush_fail_cnt: %d \n", g_dma_info.flush_fail_cnt);
    len += sprintf(page + len, "skip_cnt: %d \n", g_dma_info.skip_cnt);
    len += sprintf(page + len, "desc_alloc_cnt: %d \n", g_dma_info.desc_alloc_cnt);
    len += sprintf(page + len, "desc_alloc_fail: %d \n", g_dma_info.desc_alloc_fail);
    len += sprintf(page + len, "split_cnt: %d \n", g_dma_info.split_cnt);
    len += sprintf(page + len, "fatal_bug: %d \n", g_dma_info.fatal_bug);
    len += sprintf(page + len, "time_avg_val: %d \n", time_avg_val);
    len += sprintf(page + len, "debug value: 0x%x \n", debug_value);
    len += sprintf(page + len, "    BIT0: dump DMA write address, BIT1: dump dummy read \n");
    *eof = 1;
    return len;
}

static int proc_debug_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = count;
    unsigned char value[20];

    if (copy_from_user(value, buffer, len))
        return 0;
    value[len] = '\0';

    sscanf(value, "%u\n", &debug_value);

    printk("debug_value = 0x%x \n", debug_value);

    return count;
}

/* split height
 */
static int proc_split_height_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    unsigned int len = 0;

    len += sprintf(page + len, "scl_dma split height: %d \n", splitarea_height);

    *eof = 1;
    return len;
}

static int proc_split_height_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = count;
    unsigned char value[20];

    if (copy_from_user(value, buffer, len))
        return 0;
    value[len] = '\0';

    sscanf(value, "%u\n", &splitarea_height);

    printk("scl_dma split height: %d \n", splitarea_height);

    return count;
}

/*
 * Alignment 64 bytes
 */
static int proc_align64_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    unsigned int len = 0;

    len += sprintf(page + len, "scl_dma force 64bytes alignment: %d \n", align64_flag);

    *eof = 1;
    return len;
}

static int proc_align64_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = count;
    unsigned char value[20];

    if (copy_from_user(value, buffer, len))
        return 0;
    value[len] = '\0';

    sscanf(value, "%u\n", &align64_flag);

    printk("scl_dma force 64bytes alignment: %d \n", align64_flag);

    return count;
}

#if 0
#include <mach/fmem.h>
//extern unsigned int ffb_get_fb_pbase(int lcd_idx, int fb_plane);
static int __init test_init(void)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
    scl_dma_t   info;
    u32 paddr, *ptr;
    void *vaddr;
    int i, j;

    fmem_get_identifier(&pci_id, &cpu_id);

    if (pci_id == FMEM_PCI_HOST) {
        u32 pci_addr;
        paddr = 0x1543a000;
        fmem_set_ep_outbound_win(paddr, 16 << 20);

        pci_addr = fmem_get_pcie_addr(0x1543a000);
        printk("remote address = 0x%x \n", pci_addr);

        return 0;
    }

    paddr = 0x1543a000;

    scl_dma_init();

    memset(&info, 0, sizeof(scl_dma_t));

    /* prepare the pattern */
    vaddr = (void *)ioremap(paddr, 1920*1080*2);
    memset((void *)vaddr, 0xf8, 1920*1080*2);

    ptr = (u32 *)vaddr;
    /* pattern */
    for (j = 0; j < 540; j ++) {    //height
        ptr = (u32 *)(vaddr + 1920*2 * j);
        for (i = 0; i < 200; i ++) {    //width
            *ptr = 0xf800f800;
            ptr ++;
        }
    }

    /* start to test */
    info.job_id = 1;
    info.src_addr = paddr;
    info.dst_addr = 0xed43a000 + 540 * 1920 * 2;
    info.x_offset = 0;
    info.y_offset = 0;
    info.width = 1920;
    info.height = 540;
    info.bg_width = 1920;
    info.bg_height = 1080;
    scl_dma_add(&info);
    scl_dma_trigger(info.job_id);

    printk("remote address = 0x%x \n", info.dst_addr);
    return 0;
}

static void __exit test_exit(void)
{
    scl_dma_exit();
}

module_init(test_init);
module_exit(test_exit);
#endif
MODULE_AUTHOR("Grain Media Corp.");
MODULE_LICENSE("GPL");
