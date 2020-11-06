#include <linux/module.h>
#include <mach/ftpmu010.h>
#include <linux/list.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include <mach/fmem.h>
#include <mach/platform/platform_io.h>
#include <frammap/frammap_if.h>
#include "scaler_trans.h"

struct element {
    directio_info_t  data;
    unsigned int reserved[10];
};
#define DIO_SIZE        PAGE_SIZE*4
#define DHCPMEM_OFFSET  (HDMI_FTHDCPMEM_PA_SIZE - 16)
#define MAGIC_NUMBER    0xAABBCCDD
#define PCI_DDR_BASE    0xF0000000
#define TO_PCIE_ADDR(x) ((x) + PCI_DDR_BASE)

typedef struct {
    unsigned int    rx_idx;
    unsigned int    tx_idx;
    struct element  entry[ENTRY_COUNT];
} share_mem_t;
#if defined(CONFIG_PLATFORM_GM8210)
static frammap_buf_t    frm_buf;
static share_mem_t *snd_sharemem = NULL;
static share_mem_t *rcv_sharemem = NULL;
static int init_done = 0;

static void *dhcpmem = NULL;
#endif
void scaler_trans_init(void)
{
#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
    u32 value, phys_addr, pcie_phys_addr, round = 10000;

    fmem_get_identifier(&pci_id, &cpu_id);

    printk("%s, version: 0x%x \n", __func__, DIO_VERSION);

    /* Memory Layout: Whole share memory is DIO_SIZE. The memory is provided by FA726 of PCI RC due to BAR mapping issue.
     * RC uses top half of the PAGE.
     * EP uses bottom half of the PAGE.
     */
    if ((pci_id == FMEM_PCI_HOST)  && (cpu_id == FMEM_CPU_FA726)) {
        memset(&frm_buf, 0, sizeof(frm_buf));
        frm_buf.size = DIO_SIZE;
        frm_buf.align = 128;
        frm_buf.name = "2ddma";
        frm_buf.alloc_type = ALLOC_NONCACHABLE;
        if (frm_get_buf_ddr(DDR_ID_SYSTEM, &frm_buf) < 0)
            panic("%s, frammap fail! \n", __func__);

        memset(frm_buf.va_addr, 0, DIO_SIZE);
        if ((sizeof(share_mem_t) + 8) > (DIO_SIZE >> 1))
            panic("%s, memory size is too small! \n", __func__);

        dhcpmem = ioremap_nocache(HDMI_FTHDCPMEM_PA_BASE, HDMI_FTHDCPMEM_PA_SIZE);
        if (dhcpmem == NULL)
            panic("%s, ioremap fail2! \n", __func__);

        iowrite32(MAGIC_NUMBER, dhcpmem + DHCPMEM_OFFSET);
        iowrite32(frm_buf.phy_addr, dhcpmem + DHCPMEM_OFFSET + 4);
        return;
    }

    if (cpu_id != FMEM_CPU_FA626)
        return;

    dhcpmem = ioremap_nocache(HDMI_FTHDCPMEM_PA_BASE, HDMI_FTHDCPMEM_PA_SIZE);
    if (dhcpmem == NULL)
        panic("%s, ioremap fail2! \n", __func__);

    printk("%s, Wait RC FA726 provides the directIO memory...\n", __func__);
    do {
        printk(".");
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(msecs_to_jiffies(500));
        value = ioread32(dhcpmem + DHCPMEM_OFFSET);
        round --;
    } while ((value != MAGIC_NUMBER) && round);
    if (round == 0)
        panic("%s, wait FA726 timeout!!! \n", __func__);

    /* read the directIO physical address */
    phys_addr = ioread32(dhcpmem + DHCPMEM_OFFSET + 4);

    if (pci_id == FMEM_PCI_HOST)  {
        snd_sharemem = (share_mem_t *)ioremap_nocache(phys_addr, DIO_SIZE); /* top half */
        if (snd_sharemem == NULL)
            panic("%s, ioremap fail! \n", __func__);

        rcv_sharemem = (share_mem_t *)((u32)snd_sharemem + (DIO_SIZE >> 1));
        printk("%s, done, the directIO physical address: 0x%x \n", __func__, phys_addr);
    } else {
        pcie_phys_addr = TO_PCIE_ADDR(phys_addr);
        rcv_sharemem = (share_mem_t *)ioremap_nocache(pcie_phys_addr, DIO_SIZE);
        if (rcv_sharemem == NULL)
            panic("%s, ioremap fail! \n", __func__);
        snd_sharemem = (share_mem_t *)((u32)rcv_sharemem + (DIO_SIZE >> 1));
        printk("%s, done, the directIO physical address: 0x%x, pcie: 0x%x \n", __func__, phys_addr, pcie_phys_addr);
    }

    init_done = 1;
#endif
}
EXPORT_SYMBOL(scaler_trans_init);

void scaler_trans_exit(void)
{
#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);
    if ((pci_id == FMEM_PCI_HOST)  && (cpu_id == FMEM_CPU_FA726)) {
        frm_free_buf_ddr(frm_buf.va_addr);
        return;
    }

    if (pci_id == FMEM_PCI_HOST) {
        __iounmap(snd_sharemem);
        snd_sharemem = NULL;
    } else {
        __iounmap(rcv_sharemem);
        rcv_sharemem = NULL;
    }
#endif
}
EXPORT_SYMBOL(scaler_trans_exit);

/* send data */
int scaler_directio_snd(directio_info_t *data, int len)
{
#if defined(CONFIG_PLATFORM_GM8210)
    struct element  *entry;
    int write_idx;

    if (!init_done) {
        printk("%s, init_done not yet! \n", __func__);
        return -1;
    }

    if (len == 0 || !data) {
        printk("%s, zero length! \n", __func__);
        return -2;
    }

    if (len > sizeof(directio_info_t)) {
        printk("%s, len %d is bigger than %d! \n", __func__, len, sizeof(directio_info_t));
        return -3;
    }

    /* we assume the data will not be blocking */
    if (DIO_BUF_SPACE(snd_sharemem->tx_idx, snd_sharemem->rx_idx) == 0) {
        printk("%s, no space! tx_idx: %d, rx_idx: %d\n", __func__, snd_sharemem->tx_idx, snd_sharemem->rx_idx);
        return -4;
    }

    write_idx = DIO_BUFIDX(snd_sharemem->tx_idx);
    entry = &snd_sharemem->entry[write_idx];
    memcpy((void *)&entry->data.data[0], (void *)data, len);

    /* ok, the data is ready. Update the index */
    snd_sharemem->tx_idx ++;
#endif
   return 0;
}
EXPORT_SYMBOL(scaler_directio_snd);

/* rcv data */
int scaler_directio_rcv(directio_info_t *data, int len)
{
#if defined(CONFIG_PLATFORM_GM8210)
    struct element  *entry;
    int read_idx;

    if (!init_done) {
        printk("%s, init_done not yet! \n", __func__);
        return -1;
    }

    if (len == 0 || !data) {
        printk("%s, zero length! \n", __func__);
        return -2;
    }

    if (len > sizeof(directio_info_t)) {
        printk("%s, len %d is bigger than %d! \n", __func__, len, sizeof(directio_info_t));
        return -3;
    }

    if (DIO_BUF_COUNT(rcv_sharemem->tx_idx, rcv_sharemem->rx_idx) == 0)
        return -4;

    read_idx = DIO_BUFIDX(rcv_sharemem->rx_idx);
    entry = &rcv_sharemem->entry[read_idx];
    memcpy((void *)data, (void *)&entry->data.data[0], len);

    /* ok, the data is copied. Update the index */
    rcv_sharemem->rx_idx ++;
#endif
    return 0;
}
EXPORT_SYMBOL(scaler_directio_rcv);