/*-----------------------------------------------------------------
 * Includes
 *-----------------------------------------------------------------
 */
#include <linux/version.h>      /* kernel versioning macros      */
#include <linux/module.h>       /* kernel modules                */
#include <linux/poll.h>         /* poll flags                    */
#include <linux/device.h>       /* sysfs/class interface         */
#include <linux/cdev.h>         /* character device interface    */
#include <linux/list.h>         /* linked-lists                  */
#include <linux/slab.h>         /* kmem_cache_t                  */
#include <linux/interrupt.h>    /* interrupts                    */
#include <linux/workqueue.h>    /* work-queues                   */
#include <linux/timer.h>        /* timers                        */
#include <linux/pci.h>          /* PCI + PCI DMA API             */
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/scatterlist.h>    /* scatter-gather DMA list       */
#include <asm/page.h>           /* get_order                     */
#include <linux/platform_device.h>
#include <mach/ftpmu010.h>
#include <mach/gm_jiffies.h>
#include "zebra_sdram.h"        /* Host-to-DSP SDRAM layout      */
#include "zebra.h"
#include <cpu_comm/cpu_comm.h>
#include <mach/fmem.h>

/*-----------------------------------------------------------------
 * module parameters
 *-----------------------------------------------------------------
 */
static int mem = 8 << 20;

module_param(mem, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mem, "memory size for communication");

/*
 * global information define all peer instances.
 */

/* FA726 of HOST */
static mbox_dev_t mbox_dev_fa726[] = {
    {
        "FA626",                /* peer name */
        ZEBRA_FA626_IRQ0,       /* peer_irq */
        ZEBRA_FA726_IRQ,        /* local irq */
        ZEBRA_MBOX_HOST_OFS0,   /* mbox paddr offset */
        FA626_CORE_ID,          /* peer cpu id */
        FA726_CORE_ID,          /* local cpu id */
        NULL,                   /* private data */
        CHAN_726_626_BASE       /* channel base */
    },
#ifdef INCLUDE_PCI
    /* PCI */
    {
        "DEV0_FA726",           /* peer name */
        ZEBRA_EP_FA726_IRQ0,    /* peer_irq */
        ZEBRA_RC_FA726_IRQ0,    /* local irq */
        ZEBRA_MBOX_HOST_OFS3,   /* mbox paddr offset */
        PCIDEV_FA726_CORE_ID,   /* peer cpu id */
        FA726_CORE_ID,          /* local cpu id */
        NULL,                   /* private data */
        CHAN_PCI0_726_726_BASE  /* channel base */
    },
    {
        "DEV0_FA626",           /* peer name */
#ifdef USE_GPIO
        ZEBRA_GPIO_IRQ_1,       /* peer_irq */
        ZEBRA_GPIO_IRQ_0,       /* local irq */
#else
        ZEBRA_EP_FA626_IRQ0,    /* peer_irq */
        ZEBRA_RC_FA726_IRQ1,    /* local irq */
#endif
        ZEBRA_MBOX_HOST_OFS4,   /* mbox paddr offset */
        PCIDEV_FA626_CORE_ID,   /* peer cpu id */
        FA726_CORE_ID,          /* local cpu id */
        NULL,                   /* private data */
        CHAN_PCI0_726_626_BASE  /* channel base */
    },
#endif
};

/* FA626 of HOST */
static mbox_dev_t mbox_dev_fa626[] = {
    {
        "FA726",                /* peer name */
        ZEBRA_FA726_IRQ,        /* peer_irq */
        ZEBRA_FA626_IRQ0,       /* local irq */
        ZEBRA_MBOX_HOST_OFS0,   /* mbox paddr offset */
        FA726_CORE_ID,          /* peer cpu id */
        FA626_CORE_ID,          /* local cpu id */
        NULL,                   /* private data */
        CHAN_726_626_BASE       /* channel base */
    },
#ifdef INCLUDE_FC7500
    {
        "FC7500",               /* peer name */
        ZEBRA_FC7500_IRQ,       /* peer_irq */
        ZEBRA_FA626_IRQ1,       /* local irq */
        ZEBRA_MBOX_HOST_OFS1,   /* mbox paddr offset */
        FC7500_CORE_ID,         /* peer cpu id */
        FA626_CORE_ID,          /* local cpu id */
        NULL,                   /* private data */
        CHAN_626_7500_BASE      /* channel base */
    },
#endif
};

/* FA726 of DEVICE0 */
static mbox_dev_t mbox_dev_fa726_dev0[] = {
#ifdef INCLUDE_PCI
    {
        "HOST_FA726",           /* peer name */
        ZEBRA_RC_FA726_IRQ0,    /* peer_irq */
        ZEBRA_EP_FA726_IRQ0,    /* local irq */
        ZEBRA_MBOX_HOST_OFS3,   /* mbox paddr offset */
        FA726_CORE_ID,          /* peer cpu id */
        PCIDEV_FA726_CORE_ID,   /* local cpu id */
        NULL,                   /* private data */
        CHAN_PCI0_726_726_BASE  /* channel base */
    },
#endif
    {
        "FA626",                /* peer name */
        ZEBRA_FA626_IRQ0,       /* peer_irq */
        ZEBRA_FA726_IRQ,        /* local irq */
        ZEBRA_MBOX_DEV0_OFS0,   /* mbox paddr offset */
        FA626_CORE_ID,          /* peer cpu id */
        FA726_CORE_ID,          /* local cpu id */
        NULL,                   /* private data */
        CHAN_726_626_BASE       /* channel base */
    },
};

/* FA626 of DEVICE0 */
static mbox_dev_t mbox_dev_fa626_dev0[] = {
#ifdef INCLUDE_PCI
    {
        "HOST_FA726",           /* peer name */
#ifdef USE_GPIO
        ZEBRA_GPIO_IRQ_0,       /* peer_irq */
        ZEBRA_GPIO_IRQ_1,       /* local irq */
#else
        ZEBRA_RC_FA726_IRQ1,    /* peer_irq */
        ZEBRA_EP_FA626_IRQ0,    /* local irq */
#endif
        ZEBRA_MBOX_HOST_OFS4,   /* mbox paddr offset */
        FA726_CORE_ID,          /* peer cpu id */
        PCIDEV_FA626_CORE_ID,   /* local cpu id */
        NULL,                   /* private data */
        CHAN_PCI0_726_626_BASE  /* channel base */
    },
#endif
    {
        "FA726",                /* peer name */
        ZEBRA_FA726_IRQ,        /* peer_irq */
        ZEBRA_FA626_IRQ0,       /* local irq */
        ZEBRA_MBOX_DEV0_OFS0,   /* mbox paddr offset */
        FA726_CORE_ID,          /* peer cpu id */
        FA626_CORE_ID,          /* local cpu id */
        NULL,                   /* private data */
        CHAN_726_626_BASE       /* channel base */
    },
#ifdef INCLUDE_FC7500
    {
        "FC7500",               /* peer name */
        ZEBRA_FC7500_IRQ,       /* peer_irq */
        ZEBRA_FA626_IRQ1,       /* local irq */
        ZEBRA_MBOX_DEV0_OFS1,   /* mbox paddr offset */
        FC7500_CORE_ID,         /* peer cpu id */
        FA626_CORE_ID,          /* local cpu id */
        NULL,                   /* private data */
        CHAN_626_7500_BASE      /* channel base */
    },
#endif
};

struct mbox_array_s {
    mbox_dev_t  *mbox_dev;
    int         mbox_cnt;
} mbox_array[] = {
    {mbox_dev_fa726,        ARRAY_SIZE(mbox_dev_fa726)},
    {mbox_dev_fa626,        ARRAY_SIZE(mbox_dev_fa626)},
    {mbox_dev_fa726_dev0,   ARRAY_SIZE(mbox_dev_fa726_dev0)},
    {mbox_dev_fa626_dev0,   ARRAY_SIZE(mbox_dev_fa626_dev0)},
};

static struct mbox_array_s  *mbox_ptr = NULL;

/* Local function used to calculate how many PCI peers or Local peers
 */
static int get_peer_cnt(mbox_dev_t *mdev_ptr, int mdev_cnt, int is_pci)
{
    int i, cnt = 0;

    for (i = 0; i < mdev_cnt; i ++) {
        if (is_pci) {
            /* pci instance */
            if (PCIX_ID(mdev_ptr->identifier))
                cnt ++;
        } else {
            /* not pci instance */
            if (!PCIX_ID(mdev_ptr->identifier))
                cnt ++;
        }
        mdev_ptr ++;
    }
    return cnt;
}

/*
 * @This function registers a client to CPU_COMM module. When the frames come, it will be delivered
 *      to the channel of this reigstered client.
 *
 * @function int cpu_comm_open(chan_id_t ch_id, char *name);
 * @param chan_id_t indicates the channel id prdefined in chan_id_t
 * @param name indicates your name.
 * @return 0 on success, <0 on error
 */
int cpu_comm_open(chan_id_t chan_id, char *name)
{
    int status = 0, brd_id, brd_chan_id;
    u32 zebra_attr = 0;

    brd_chan_id = CHAN_ID(chan_id); //channel id in this board
    brd_id = BOARD_ID(chan_id);     //board idx

    if (brd_chan_id >= ZEBRA_DEVICE_AP_START) {
        printk("%s, chan id:%d exceeds %d! \n", __func__, brd_chan_id, ZEBRA_DEVICE_AP_START);
        return -1;
    }

    if (strlen(name) >= NAME_LEN) {
        printk("%s, name:%s is too long! \n", __func__, name);
        return -1;
    }

    status = zebra_device_open(brd_id, brd_chan_id, name, zebra_attr);

    return status;
}
EXPORT_SYMBOL(cpu_comm_open);

/*
 * @This function de-registers a client from CPU_COMM module. When this function is called,
 *  all frames queued in this channel are removed.
 *
 * @function int cpu_comm_close(chan_id_t ch_id, char *name);
 * @param chan_id_t indicates the channel id prdefined in chan_id_t
 * @return 0 on success, <0 on error
 */
int cpu_comm_close(chan_id_t chan_id)
{
    int status = 0, brd_id, brd_chan_id;

    brd_chan_id = CHAN_ID(chan_id); //channel id in this board
    brd_id = BOARD_ID(chan_id);     //board idx


    if (brd_chan_id >= ZEBRA_DEVICE_AP_START) {
        printk("%s, chan id:%d exceeds %d! \n", __func__, brd_chan_id, ZEBRA_DEVICE_AP_START);
        return -1;
    }

    status = zebra_device_close(brd_id, brd_chan_id);

    return status;
}
EXPORT_SYMBOL(cpu_comm_close);

/*
 * @This function sends the msg into the channel.
 *
 * @function int cpu_comm_msg_snd(cpu_comm_msg_t *msg, int timeout_ms);
 * @param msg indicates the data the module wants to send. Note: msg->channel is
 *      necessary. It indicates the destination.
 * @param timeout_ms indicates whether the caller will be blocked if the ring buffer is full or the
 *      buffer of the peer is full.
 *      timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 * @return 0 on success, <0 on error
 */
int cpu_comm_msg_snd(cpu_comm_msg_t *msg, int timeout_ms)
{
    int brd_id, brd_chan_id;
    int status;

    brd_chan_id = CHAN_ID(msg->target); //channel id in this board
    brd_id = BOARD_ID(msg->target);     //board idx

    if (brd_chan_id >= ZEBRA_DEVICE_AP_START) {
        printk("%s, chan id:%d exceeds %d! \n", __func__, brd_chan_id, ZEBRA_DEVICE_AP_START);
        return -1;
    }

    status = zebra_msg_snd(brd_id, brd_chan_id, msg->msg_data, msg->length, timeout_ms);

    return status;
}
EXPORT_SYMBOL(cpu_comm_msg_snd);

/*
 * @This function receives the msg from the channel.
 *
 * @function int cpu_comm_msg_rcv(cpu_comm_msg_t *msg, int timeout_ms);
 * @param msg indicates the data the module wants to receive. Note: msg->channel is
 *      necessary. It indicates the destination.
 * @param timeout_ms indicates whether the caller will be blocked if the channel is empty.
 *      timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 * @return 0 on success, <0 on error
 */
int cpu_comm_msg_rcv(cpu_comm_msg_t *msg, int timeout_ms)
{
    int brd_id, brd_chan_id;
    int status;

    brd_chan_id = CHAN_ID(msg->target); //channel id in this board
    brd_id = BOARD_ID(msg->target);     //board idx

    if (brd_chan_id >= ZEBRA_DEVICE_AP_START) {
        printk("%s, chan id:%d(0x%x) exceeds %d! \n", __func__, brd_chan_id, msg->target,
                                ZEBRA_DEVICE_AP_START);
        return -1;
    }

    status = zebra_msg_rcv(brd_id, brd_chan_id, msg->msg_data, &msg->length, timeout_ms);

    return status;
}
EXPORT_SYMBOL(cpu_comm_msg_rcv);

/*
 * @This function registers a client to CPU_COMM module. When the frames come, it will be delivered
 *      to the channel of this reigstered client.
 *
 * @function int cpu_comm_open(chan_id_t ch_id, char *name);
 * @param chan_id_t indicates the channel id prdefined in chan_id_t
 * @param name indicates your name. *
 * @return 0 on success, <0 on error
 */
int cpu_comm_pack_open(chan_id_t chan_id, char *name)
{
    int status = 0, brd_id, brd_chan_id;
    u32 zebra_attr = ATTR_PACK;

    brd_chan_id = CHAN_ID(chan_id); //channel id in this board
    brd_id = BOARD_ID(chan_id);     //board idx

    if (brd_chan_id >= ZEBRA_DEVICE_AP_START) {
        printk("%s, chan id:%d exceeds %d! \n", __func__, brd_chan_id, ZEBRA_DEVICE_AP_START);
        return -1;
    }

    if (strlen(name) >= NAME_LEN) {
        printk("%s, name:%s is too long! \n", __func__, name);
        return -1;
    }

    status = zebra_device_open(brd_id, brd_chan_id, name, zebra_attr);

    return status;
}
EXPORT_SYMBOL(cpu_comm_pack_open);

/*
 * @This function de-registers a client from CPU_COMM module. When this function is called,
 *  all frames queued in this channel are removed.
 *
 * @function int cpu_comm_pack_close(chan_id_t ch_id, char *name);
 * @param chan_id_t indicates the channel id prdefined in chan_id_t
 * @return 0 on success, <0 on error
 */
int cpu_comm_pack_close(chan_id_t chan_id)
{
    return cpu_comm_close(chan_id);
}
EXPORT_SYMBOL(cpu_comm_pack_close);

/*
 * @This function sends the packed msg into the channel.
 *
 * @function int cpu_comm_pack_msg_snd(cpu_comm_pack_msg_t *msg, int timeout_ms);
 * @param msg indicates the data the module wants to send. Every parameter is necessary.
 * @param timeout_ms indicates whether the caller will be blocked if the ring buffer is full or the
 *      buffer of the peer is full.
 *      timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 * @return 0 on success, <0 on error
 */
int cpu_comm_pack_msg_snd(cpu_comm_pack_msg_t *msg, int timeout_ms)
{
    int brd_id, brd_chan_id;
    int status;

    brd_chan_id = CHAN_ID(msg->target); //channel id in this board
    brd_id = BOARD_ID(msg->target);     //board idx

    if (brd_chan_id >= ZEBRA_DEVICE_AP_START) {
        printk("%s, chan id:%d exceeds %d! \n", __func__, brd_chan_id, ZEBRA_DEVICE_AP_START);
        return -1;
    }

    status = zebra_pack_msg_snd(brd_id, brd_chan_id, (unsigned char **)&msg->msg_data, (int *)&msg->length, timeout_ms);

    return status;
}
EXPORT_SYMBOL(cpu_comm_pack_msg_snd);

/*
 * @This function receives the pack msg from the channel.
 *
 * @function int cpu_comm_pack_msg_rcv(cpu_comm_pack_msg_t *msg, int timeout_ms);
 * @param msg indicates the data the module wants to receive. Note: msg->channel is
 *      necessary. It indicates the destination.
 * @param timeout_ms indicates whether the caller will be blocked if the channel is empty.
 *      timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 * @return 0 on success, <0 on error
 */
int cpu_comm_pack_msg_rcv(cpu_comm_pack_msg_t *msg, int timeout_ms)
{
    int brd_id, brd_chan_id;
    int status;

    brd_chan_id = CHAN_ID(msg->target); //channel id in this board
    brd_id = BOARD_ID(msg->target);     //board idx

    if (brd_chan_id >= ZEBRA_DEVICE_AP_START) {
        printk("%s, chan id:%d(0x%x) exceeds %d! \n", __func__, brd_chan_id, msg->target,
                                ZEBRA_DEVICE_AP_START);
        return -1;
    }

    status = zebra_pack_msg_rcv(brd_id, brd_chan_id, (unsigned char **)&msg->msg_data, (int *)&msg->length, timeout_ms);

    return status;
}
EXPORT_SYMBOL(cpu_comm_pack_msg_rcv);

/* This function is used to reset the on-hand entry if the user doesn't want to process this
 * entry anymore.
 * Note: the packets queuing in the readlist are not affected.
 */
void cpu_comm_pack_clean_readentry(chan_id_t chan_id)
{
    int brd_id, brd_chan_id;

    brd_chan_id = CHAN_ID(chan_id); //channel id in this board
    brd_id = BOARD_ID(chan_id);     //board idx

    zebra_pack_clean_readentry(brd_id, brd_chan_id);

    return;
}
EXPORT_SYMBOL(cpu_comm_pack_clean_readentry);

/*
 * @This function get the max buffer size without truncating message.
 *
 * @function int cpu_comm_get_msgsz(chan_id_t chan_id);
 * @param: chan_id indicates the specific channel
 *
 * @return the max message size if > 0, <= 0 for fail.
 */
int cpu_comm_get_msgsz(chan_id_t chan_id)
{
    int brd_id, brd_chan_id;
    int msglen = -1;

    brd_chan_id = CHAN_ID(chan_id); //channel id in this board
    brd_id = BOARD_ID(chan_id);     //board idx

    msglen = zebra_get_msgsz(brd_id, brd_chan_id);

    return msglen;
}
EXPORT_SYMBOL(cpu_comm_get_msgsz);

/*
 * @This function returns who I am.
 *
 * @function int cpu_comm_get_identifier(int *chip_id, int *subsys_id)
 * @param: pcix_id indicates the pcix id in the whole system
 * @param: cpu_id_t indicates the numbered cpu id within one chip
 * @return 0 for success others for fail.
 */
int cpu_comm_get_identifier(pcix_id_t *pcix_id, cpu_id_t *cpu_id)
{
    fmem_pci_id_t pci;
    fmem_cpu_id_t cpu;

    fmem_get_identifier(&pci, &cpu);

    switch (pci) {
      case FMEM_PCI_HOST:
        *pcix_id = PCIX_HOST;
        break;
      case FMEM_PCI_DEV0:
        *pcix_id = PCIX_DEV0;
        break;
      default:
        panic("unknown value: %d \n", pci);
        break;
    }

    switch (cpu) {
      case FMEM_CPU_FA726:
        *cpu_id = CPU_FA726;
        break;
      case FMEM_CPU_FA626:
        *cpu_id = CPU_FA626;
        break;
      default:
        panic("unknown value: %d \n", cpu);
        break;
    }

    return 0;
}
EXPORT_SYMBOL(cpu_comm_get_identifier);


/*
 * @This function returns the ticks
 *
 * @function int cpu_comm_get_tick(pcix_id_t pci_id, cpu_id_t cpu_id, u32 *tick_in_hz, u32 *tick)
 * @param: pci_id indicates the pcix id in the whole system. Current it will be ingored...
 * @param: cpu_id_t indicates the numbered cpu id within one chip
 * @param: tick_in_hz indicates number of ticks in a second (hz)
 * @param: tick indicates current jiffies
 * @return 0 for success others for fail.
 */
int cpu_comm_get_tick(pcix_id_t pci_id, cpu_id_t cpu_id, u32 *tick_in_hz, u32 *tick)
{
    fmem_pci_id_t   fmem_pci_id;
    fmem_cpu_id_t   fmem_cpu_id;

    fmem_get_identifier(&fmem_pci_id, &fmem_cpu_id);

    /* Only FA626 will update the jiffies in the SCU */
    if (fmem_cpu_id == FMEM_CPU_FA726) {
        switch (cpu_id) {
          case CPU_FA726:
            *tick = (u32)gm_jiffies;
            *tick_in_hz = CONFIG_HZ;
            break;
          case CPU_FA626:
            *tick = ftpmu010_read_reg(0x64);
            *tick_in_hz = ftpmu010_read_reg(0x54);
            break;
          default:
            return -1;
            break;
        }
    } else {
        /* I am FA626 */
        switch (cpu_id) {
          case CPU_FA626:
            *tick = (u32)gm_jiffies;
            *tick_in_hz = CONFIG_HZ;
            break;
          default:
            return -1;
            break;
        }
    }

    return 0;
}
EXPORT_SYMBOL(cpu_comm_get_tick);

static int __init cpucomm_init(void)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
    zebra_dma_t     dma_type = ZEBRA_DMA_NONE;
    int i, ep_cnt = 0, non_ep_cnt = 0, mbox_host = 0, cache_yes = 0;
    mbox_dev_t  *data_ptr;

    //compile check
    fmem_get_identifier(&pci_id, &cpu_id);

    if (ZEBRA_NUM_IN_PACK != CPU_COMM_NUM_IN_PACK)
        panic("%s, inconsistent value in pack %d %d! \n", __func__, ZEBRA_NUM_IN_PACK, CPU_COMM_NUM_IN_PACK);

    if (pci_id == FMEM_PCI_HOST) {
        if (cpu_id == FMEM_CPU_FA726) {
            mbox_ptr = &mbox_array[0];
            /* only the one with FA726 cpu can provide the mbox memory */
            mbox_host = 1;
            dma_type = ZEBRA_DMA_NONE;
            /* how many EP in the system */
            ep_cnt = ftpmu010_get_attr(ATTR_TYPE_EPCNT);
            if (ep_cnt)
                ep_cnt = get_peer_cnt(mbox_ptr->mbox_dev, mbox_ptr->mbox_cnt, 1);
            non_ep_cnt = get_peer_cnt(mbox_ptr->mbox_dev, mbox_ptr->mbox_cnt, 0);

            mem = mem * (ep_cnt + (ep_cnt + non_ep_cnt)); /* pci memory + local memory */
        } else if (cpu_id == FMEM_CPU_FA626) {
            mbox_ptr = &mbox_array[1];
            dma_type = ZEBRA_DMA_AXI;
            non_ep_cnt = get_peer_cnt(mbox_ptr->mbox_dev, mbox_ptr->mbox_cnt, 0);
            mem = mem * non_ep_cnt;
        } else {
            panic("%s, host unknown cpu! \n", __func__);
        }
    } else {
        if (cpu_id == FMEM_CPU_FA726) {
            mbox_host = 1;
            mbox_ptr = &mbox_array[2];
            dma_type = ZEBRA_DMA_AXI;
            non_ep_cnt = get_peer_cnt(mbox_ptr->mbox_dev, mbox_ptr->mbox_cnt, 0);
            mem = mem * non_ep_cnt;
        } else if (cpu_id == FMEM_CPU_FA626) {
            mbox_ptr = &mbox_array[3];
            dma_type = ZEBRA_DMA_AXI;
            non_ep_cnt = get_peer_cnt(mbox_ptr->mbox_dev, mbox_ptr->mbox_cnt, 0);
            mem = mem * non_ep_cnt;
        } else {
            panic("%s, ep unknown cpu! \n", __func__);
        }
    }

    printk("ep_cnt = %d, non_ep_cnt = %d\n", ep_cnt, non_ep_cnt);
    zebra_init(mbox_host, dma_type, ep_cnt, non_ep_cnt, mem);

    /* register all information to zebra core */
    for (i = 0; i < mbox_ptr->mbox_cnt; i ++) {
        data_ptr = mbox_ptr->mbox_dev + i;

	/* skip if not in PCI system */
	if ((pci_id == FMEM_PCI_HOST) && !ep_cnt && PCIX_ID(data_ptr->identifier))
	    continue;

        if (zebra_probe(data_ptr))
            panic("%s, register %s fail! \n", __func__, data_ptr->name);
	}

#ifdef CACHE_COPY
    cache_yes = 1;
#endif
    switch (cpu_id) {
      case FMEM_CPU_FA726:
        printk("ok, cpu_comm_%s driver is executed in FA726 core with dma_type: %d(%d).\n",
                    DRV_VER, dma_type, cache_yes);
        break;
      case FMEM_CPU_FA626:
        printk("ok, cpu_comm_%s driver is executed in FA626 core with dma_type: %d(%d).\n",
                    DRV_VER, dma_type, cache_yes);
        break;
      default:
        panic("%s, cpu is unknown. \n", __func__);
       break;
    }

    return 0;
}

static void __exit cpucomm_cleanup(void)
{
    int  i;
    mbox_dev_t  *data_ptr;

    for (i = 0; i < mbox_ptr->mbox_cnt; i ++) {
        data_ptr = mbox_ptr->mbox_dev + i;

        if (zebra_remove(data_ptr))
            panic("%s, register %s fail! \n", __func__, data_ptr->name);
    }

    zebra_exit();
}

module_init(cpucomm_init);
module_exit(cpucomm_cleanup);
MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Two CPU core communication");
MODULE_LICENSE("GPL");

