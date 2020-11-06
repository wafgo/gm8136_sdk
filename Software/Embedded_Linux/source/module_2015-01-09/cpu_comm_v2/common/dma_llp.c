#include <linux/version.h>      /* kernel versioning macros      */
#include <linux/module.h>       /* kernel modules                */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>         /* linked-lists                  */
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>
#include <linux/delay.h>
#include "zebra_sdram.h"
#include "zebra.h"
#include "dma_llp.h"
#include <cpu_comm/cpu_comm.h>
#include "platform.h"

#define CHGNTWIN	64
#define CONFIG_REG  ((0x1 << 28) /* high pri */ | ((CHGNTWIN & 0xFF) << 20))
#define DUMMY_LEN   1024
#define USE_WAIT_Q
#define DMA_IN_PROCESS  0
#define DMA_FINISH      1

typedef struct {
    u32 dmac_io_base;
    struct semaphore chan_sem;
    struct semaphore dma_sem;
    int active_dma_cnt; /* number of dma is active */
    struct {
        //struct dma_chan *dma_chan;    //dmac030
        int chan_id;    //dmac030 is off, so we cannot detect the free channels
        int busy;
        int dma_status;
        wait_queue_head_t dma_wait_queue;
        struct completion dma_complete;
        char name[20];
        int irq;
        u32 intr_cnt;
    } dma_info[NUM_DMA];
    void *dummy_dma;   /* for dummy dma */
} dma_info_t;

static dma_info_t     g_dma_info;

#define CHAN_SEMA_LOCK       down(&g_dma_info.chan_sem)
#define CHAN_SEMA_UNLOCK     up(&g_dma_info.chan_sem)
#define ENGINE_SEMA_LOCK     down(&g_dma_info.dma_sem)
#define ENGINE_SEMA_UNLOCK   up(&g_dma_info.dma_sem)

static irqreturn_t dma_llp_handle_irq(int irq, void *dev_id)
{
    int idx = (int)dev_id;

    ftpmu010_clear_intr(irq);

#ifdef USE_WAIT_Q
    g_dma_info.dma_info[idx].dma_status = DMA_FINISH;
    wake_up(&g_dma_info.dma_info[idx].dma_wait_queue);
#else
    complete(&g_dma_info.dma_info[idx].dma_complete);
#endif

    return IRQ_HANDLED;
}

void dma_llp_init(void)
{
    u32 dmach_mask;
    int i, ret, irq, idx;

    memset(&g_dma_info, 0, sizeof(g_dma_info));

    dmach_mask = platform_get_dma_chanmask();
    printk("cpucomm: use dmach mask: 0x%x \n", dmach_mask);

    for (i = 0; i < 8; i ++) {
        if (!((0x1 << i) & dmach_mask))
            continue;

        idx = g_dma_info.active_dma_cnt;
        init_completion(&g_dma_info.dma_info[idx].dma_complete);
        init_waitqueue_head(&g_dma_info.dma_info[idx].dma_wait_queue);

        g_dma_info.dma_info[idx].chan_id = i;
        g_dma_info.active_dma_cnt ++;
        if (g_dma_info.active_dma_cnt > NUM_DMA)
            panic("%s, too small array %d(%d)! \n", __func__, g_dma_info.active_dma_cnt, NUM_DMA);

        switch (i) {
          case 4:
            irq = g_dma_info.dma_info[idx].irq = CPU_INT_6; //82
            break;
          case 5:
            irq = g_dma_info.dma_info[idx].irq = CPU_INT_4; //80
            break;
          case 6:
            irq = g_dma_info.dma_info[idx].irq = CPU_INT_2; //78
            break;
          case 7:
            irq = g_dma_info.dma_info[idx].irq = CPU_INT_3; //79
            break;
          default:
            panic("error! \n");
            break;
        }

        sprintf(g_dma_info.dma_info[idx].name, "cpucomm_dmach%d", i);
        ret = request_irq(irq, dma_llp_handle_irq, 0, g_dma_info.dma_info[idx].name, (void *)idx);
        if (ret < 0)
            panic("%s, request irq%d fail! \n", __func__, irq);
    }

    if (!g_dma_info.active_dma_cnt)
        panic("%s, request dma channel fail! \n", __func__);

    //create counting semaphore
    sema_init(&g_dma_info.chan_sem, g_dma_info.active_dma_cnt);
    sema_init(&g_dma_info.dma_sem, 1);

    g_dma_info.dmac_io_base = (u32)ioremap_nocache(XDMAC_FTDMAC030_0_PA_BASE, XDMAC_FTDMAC030_0_PA_SIZE);
    if (!g_dma_info.dmac_io_base)
        panic("%s, no virtual memory! \n", __func__);

    g_dma_info.dummy_dma = kmalloc(DUMMY_LEN, GFP_KERNEL);
    if (g_dma_info.dummy_dma == NULL)
        panic("%s, No memory! \n", __func__);
    if (__pa(g_dma_info.dummy_dma) & 0x3)
        panic("%s, the paddr: 0x%x is not 4 alignment! \n", __func__, (u32)__pa(g_dma_info.dummy_dma));
}

static u64 dummy_addr[1];
void dma_llp_add_dummycmd(u32 head_va, u32 head_pa)
{
    dmac_llp_t  *head, *llp;

    head = (dmac_llp_t *)head_va;
    llp = (dmac_llp_t *)(head_va + head->llp_cnt * sizeof(dmac_llp_t));

    llp->llp.src_addr = 0xc1b00000;
    llp->llp.dst_addr = __pa(dummy_addr);
    llp->llp.llst_ptr = head_pa + (head->llp_cnt + 1) * sizeof(dmac_llp_t);
    llp->llp.control = 0x16c18106;    //fix address, and 1D
    llp->llp.tcnt = 1;
    head->llp_cnt ++;
}

void dma_llp_add(u32 head_va, u32 head_pa, u32 src_paddr, u32 dst_paddr, u32 length, u32 src_width)
{
    dmac_llp_t  *head, *llp;

    if (length == 0)
        return;

    head = (dmac_llp_t *)head_va;
    llp = (dmac_llp_t *)(head_va + head->llp_cnt * sizeof(dmac_llp_t));

    llp->llp.src_addr = src_paddr;
    llp->llp.dst_addr = dst_paddr;
    llp->llp.llst_ptr = head_pa + (head->llp_cnt + 1) * sizeof(dmac_llp_t);
    if ((src_width != 4) && (src_width != 8))
        panic("%s, source width is %d which is illegal! \n", __func__, src_width);

    if (IS_PCI_ADDR(src_paddr) && (src_paddr & CACHE_LINE_MASK)) {
        dump_stack();
        panic("%s, src_paddr = 0x%x is not 128 alignment! \n", __func__, src_paddr);
    }
    if (IS_PCI_ADDR(dst_paddr) && (dst_paddr & CACHE_LINE_MASK)) {
        dump_stack();
        panic("%s, dst_paddr = 0x%x is not 128 alignment! \n", __func__, dst_paddr);
    }
    if (IS_PCI_ADDR(src_paddr) || IS_PCI_ADDR(dst_paddr))
        length = CACHE_ALIGN(length);

    if (src_width == 4) {
        llp->llp.control = (0x1 << 28) /* no intr */ | (0x2 << 25) /* 32bits */ | (0x2 << 22) /* 32bits */ | (0x1 << 16) | (0x1 << 15) | (0x1 << 8) | 0x6;
        llp->llp.tcnt = (length >> 2);
    } else if (src_width == 8) {
        llp->llp.control = (0x1 << 28) /* no intr */ | (0x3 << 25) /* 64bits */ | (0x3 << 22) /* 64bits */ | (0x1 << 16)  | (0x1 << 15) | (0x1 << 8) | 0x6;
        llp->llp.tcnt = (length >> 3);
    }

    head->llp_cnt ++;

    if (IS_PCI_ADDR(dst_paddr))
        dma_llp_add_dummycmd(head_va, head_pa);
}

static inline int get_free_dmach(void)
{
    int i;

    /* get a free channel to do dma */
    ENGINE_SEMA_LOCK;
    for (i = 0; i < g_dma_info.active_dma_cnt; i ++) {
        if (g_dma_info.dma_info[i].busy == 0) {
            g_dma_info.dma_info[i].busy = 1;
            break;
        }
    }

    if (i == g_dma_info.active_dma_cnt)
        panic("%s, bug happen, active_dma_cnt = %d \n", __func__, g_dma_info.active_dma_cnt);
    /* release */
    ENGINE_SEMA_UNLOCK;

    return i;
}

static inline void set_dmach_free(int idx)
{
    ENGINE_SEMA_LOCK;
    if (g_dma_info.dma_info[idx].busy == 0)
        panic("%s, bug! \n", __func__);

    g_dma_info.dma_info[idx].busy = 0;
    ENGINE_SEMA_UNLOCK;
}

void dma_llp_fire(unsigned int head_va, unsigned int head_pa, int is_pci)
{
    u32 dmac_base = (u32)g_dma_info.dmac_io_base;
    int idx, chan_base;
    dmac_llp_t *head = (dmac_llp_t *)head_va;
    dmac_llp_t *llp;
    int status;

    if (head->llp_cnt == 0)
        return;

    if (is_pci && (head->llp_cnt == 1) && (head->llp.tcnt < 64)) {
        /* the length is too small, it may cause the DMA data not arrived yet but cpu
         * reads it out already. Thus we add a dummy dma
         */
        dma_llp_add(head_va, head_pa, (u32)__pa(g_dma_info.dummy_dma),
                            (u32)__pa(g_dma_info.dummy_dma + DUMMY_LEN / 2), DUMMY_LEN / 2, 4);
    }

    llp = (dmac_llp_t *)(head_va + (head->llp_cnt - 1) * sizeof(dmac_llp_t));
    llp->llp.llst_ptr = 0x0;
    llp->llp.control &= ~(0x1 << 28);   //tcmask is off which will issue interrupt

    CHAN_SEMA_LOCK;

    idx = get_free_dmach();

    chan_base = 0x100 + 0x20 * g_dma_info.dma_info[idx].chan_id;

    iowrite32(CONFIG_REG, (void *)(dmac_base + chan_base + 0x4));
    iowrite32(head_pa, (void *)(dmac_base + chan_base + 0x10));
    iowrite32(0, (void *)(dmac_base + chan_base + 0x14));

    g_dma_info.dma_info[idx].dma_status = DMA_IN_PROCESS;

    //ch enable, ExpEn, WSync
    iowrite32((0x1 << 28) | (0x1 << 16) | (0x1 << 15) | (0x1 << 8), (void *)(dmac_base + chan_base));

#ifdef USE_WAIT_Q
    status = wait_event_timeout(g_dma_info.dma_info[idx].dma_wait_queue,
                                g_dma_info.dma_info[idx].dma_status == DMA_FINISH, 10 * HZ);
    if (status == 0) {
        void __iomem *dma_io_base = (void *)g_dma_info.dmac_io_base;

        printk("%s, DMA_CH(%d) Timeout in DMAC030\n", __func__, g_dma_info.dma_info[idx].chan_id);
        printk("DMA global register dump: \n");
        printk("0x0: 0x%x, 0x4: 0x%x, 0x8: 0x%x, 0xC: 0x%x, 0x14: 0x%x, 0x18: 0x%x \n",
            ioread32(dma_io_base), ioread32(dma_io_base + 0x4), ioread32(dma_io_base + 0x8),
            ioread32(dma_io_base + 0xC), ioread32(dma_io_base + 0x14), ioread32(dma_io_base + 0x18));
        printk("DMA channel register dump: \n");
        printk("0x0: 0x%x, 0x4: 0x%x, 0x8: 0x%x, 0xC: 0x%x, 0x10: 0x%x, 0x14: 0x%x \n",
            ioread32(dma_io_base + chan_base), ioread32(dma_io_base + chan_base + 0x4),
            ioread32(dma_io_base + chan_base + 0x8), ioread32(dma_io_base + chan_base + 0xC),
            ioread32(dma_io_base + chan_base + 0x10), ioread32(dma_io_base + chan_base + 0x14));

        printk("cpucomm dma timeout! sw_irq:%d, int_cnt = %d\n",
                g_dma_info.dma_info[idx].irq, g_dma_info.dma_info[idx].intr_cnt);

        while (1)
            msleep(500);
    }
#else
    /* wait interrupt comes */
    wait_for_completion(&g_dma_info.dma_info[idx].dma_complete);
#endif

    g_dma_info.dma_info[idx].intr_cnt ++;
    set_dmach_free(idx);

    CHAN_SEMA_UNLOCK;
}

void dma_get_status(dma_status_t *status)
{
    int i;

    status->active_cnt = g_dma_info.active_dma_cnt;

    for (i = 0; i < status->active_cnt; i ++) {
        status->chan_id[i] = g_dma_info.dma_info[i].chan_id;
        status->sw_irq[i] = g_dma_info.dma_info[i].irq;
        status->intr_cnt[i] = g_dma_info.dma_info[i].intr_cnt;
        status->busy[i] = g_dma_info.dma_info[i].busy;
    }
}

void dma_llp_exit(void)
{
    int i;

    __iounmap((void *)g_dma_info.dmac_io_base);

    for (i = 0; i < g_dma_info.active_dma_cnt; i ++) {
        free_irq(g_dma_info.dma_info[i].irq, (void *)i);
    }

    kfree(g_dma_info.dummy_dma);
}