/*
 * zebra_sdram.h
 *
 * ZEBRA board SDRAM layout used by the device driver devices
 * as shared-memory for host-to-device communications.
 *
 *
 * Within each memory region, the ZEBRA driver typically defines
 * a write buffer and a read buffer. The layout of the buffer
 * regions are a 32-bit little-endian size, followed by the
 * buffer data.
 */
#ifndef __ZEBRA_SDRAM_H__
#define __ZEBRA_SDRAM_H__

#ifndef WIN32
#include <mach/platform/platform_io.h>
#include <mach/platform/board.h>
#endif

#define INCLUDE_FC7500
#define INCLUDE_PCI
#define CACHE_COPY
#define USE_GPIO    /* RC726 <-> EP726 */

#ifdef WIN32
#define PCI_DDR_DEVICE_BASE     0x00000000  //mapped to DDR memory
#define PCI_DDR_HOST_BASE       0x00000000
#else
#define PCI_DDR_DEVICE_BASE     0xF0000000  //mapped to DDR memory, orginal is 0xB8000000
#define PCI_DDR_HOST_BASE       0xF0000000  //orginal is 0xB8000000
#endif

#define IS_PCI_ADDR(x)          ((x) >= 0xA0000000)
#define PCI_IP_BASE             0xE0000000  //mapped to 0x98000000
#define AXI_IP_BASE             0x98000000

#define PCI_BAR_SIZE            (256 << 20) //orginal is 128M
#define PCI_BAR_MASK            (~(PCI_BAR_SIZE - 1))

//for PCIe case, so the value must be 128
#define CACHE_LINE_SZ           128 //must multiple of cpu cache line 32
#define CACHE_LINE_MASK         (CACHE_LINE_SZ - 1)
#define CACHE_ALIGN(x)          ((((x) + CACHE_LINE_SZ - 1) >> 7) << 7)   //128 alignemt in order to pci-e

#define CACHE_ALIGN_32(x)       ((((x) + 32 - 1) >> 5) << 5)
#define CACHE_LINE32_MASK       0x1F

#ifdef CONFIG_PLATFORM_GM8210
/* a free register in SCU. NOTE: It MUST NOT DDR address if pci case. */
#define ZEBRA_MBOX_PADDR        (PMU_FTPMU010_PA_BASE + 0x6C)
#define ZEBRA_PCI_MBOX_PADDR    (ZEBRA_MBOX_PADDR - AXI_IP_BASE + PCI_IP_BASE)  //PCI
#elif defined(WIN32)
extern unsigned int ZEBRA_MBOX_PADDR;
extern unsigned int ZEBRA_PCI_MBOX_PADDR;
#else
    #error "ERROR...."
#endif


//define the mbox offset
#define ONE_MBOX_SIZE           96 /* bytes, It is for sizeof(mbox_t) */

/*
 * Don't change the below order
 */
//HOST use
#define ZEBRA_MBOX_HOST_OFS0    0                                       //ofs 726 <-> 626
#define ZEBRA_MBOX_HOST_OFS1    (ZEBRA_MBOX_HOST_OFS0 + ONE_MBOX_SIZE)  //ofs 626 <-> 7500
#define ZEBRA_MBOX_HOST_OFS2    (ZEBRA_MBOX_HOST_OFS1 + ONE_MBOX_SIZE)  //ofs 726 <-> 7500
#define ZEBRA_MBOX_HOST_OFS3    (ZEBRA_MBOX_HOST_OFS2 + ONE_MBOX_SIZE)  //ofs PCIhost 726 <-> PCIdev 726
#define ZEBRA_MBOX_HOST_OFS4    (ZEBRA_MBOX_HOST_OFS3 + ONE_MBOX_SIZE)  //ofs PCIhost 726 <-> PCIdev 626
#define ZEBRA_MBOX_HOST_OFS5    (ZEBRA_MBOX_HOST_OFS4 + ONE_MBOX_SIZE)  //ofs PCIdev  726 <-> PCIdev 626
#define ZEBRA_MBOX_HOST_END     (ZEBRA_MBOX_HOST_OFS5 + ONE_MBOX_SIZE)
//DEVICE0 use
#define ZEBRA_MBOX_DEV0_OFS0    0                                       //ofs 726 <-> 626
#define ZEBRA_MBOX_DEV0_OFS1    (ZEBRA_MBOX_DEV0_OFS0 + ONE_MBOX_SIZE)  //ofs 626 <-> 7500
#define ZEBRA_MBOX_DEV0_OFS2    (ZEBRA_MBOX_DEV0_OFS1 + ONE_MBOX_SIZE)  //ofs 726 <-> 7500
#define ZEBRA_MBOX_DEV0_OFS3    (ZEBRA_MBOX_DEV0_OFS2 + ONE_MBOX_SIZE)  //ofs PCIhost 726 <-> PCIdev 726
#define ZEBRA_MBOX_DEV0_OFS4    (ZEBRA_MBOX_DEV0_OFS3 + ONE_MBOX_SIZE)  //ofs PCIhost 726 <-> PCIdev 626
#define ZEBRA_MBOX_DEV0_OFS5    (ZEBRA_MBOX_DEV0_OFS4 + ONE_MBOX_SIZE)  //ofs PCIdev  726 <-> PCIdev 626
#define ZEBRA_MBOX_DEV0_END     (ZEBRA_MBOX_DEV0_OFS5 + ONE_MBOX_SIZE)

/* whole message box size for communication */
#define TOTAL_MBOX_SIZE         PAGE_SIZE

/*
 * CPU core id
 */
typedef enum {
    FA726_CORE_ID = 0x07260726,
    FA626_CORE_ID = 0x06260626,
    FC7500_CORE_ID = 0x75007500,
    PCIDEV_FA726_CORE_ID = 0x17261726,
    PCIDEV_FA626_CORE_ID = 0x16261626,
} cpu_core_id_t;

/* DEFINE IRQ */
#define ZEBRA_FA726_IRQ         CPU_INT_0   //76
#define ZEBRA_FA626_IRQ0        CPU_INT_1   //77
#define ZEBRA_FA626_IRQ1        CPU_INT_14  //90
#define ZEBRA_FC7500_IRQ        CPU_INT_15  //91
/* PCI, RC <--> EP */
#define ZEBRA_RC_FA726_IRQ0     CPU_INT_5   //81, TO EP726
#define ZEBRA_RC_FA726_IRQ1     CPU_INT_7   //83, TO EP626
#define ZEBRA_EP_FA726_IRQ0     CPU_INT_8   //84, TO RC726
#define ZEBRA_EP_FA626_IRQ0     CPU_INT_9   //85, TO RC726

//PCI
//X_I2C3
#define GPIO_PIN_27             27  //INT56, PCI HOST
#define GPIO_PIN_28             28  //INT57, PCI DEV0
#define ZEBRA_GPIO_PIN_27_IRQ   56  //PCI HOST
#define ZEBRA_GPIO_PIN_28_IRQ   57  //PCI DEV0

//X_I2C4
#define GPIO_PIN_51             51  //INT58, PCI HOST
#define GPIO_PIN_52             52  //INT59, PCI DEV0
#define ZEBRA_GPIO_PIN_51_IRQ   58  //PCI HOST
#define ZEBRA_GPIO_PIN_52_IRQ   59  //PCI DEV0

//define GPIO pin according your board
#define ZEBRA_GPIO_PIN_0        GPIO_PIN_27
#define ZEBRA_GPIO_PIN_1        GPIO_PIN_28
#define ZEBRA_GPIO_IRQ_0        ZEBRA_GPIO_PIN_27_IRQ
#define ZEBRA_GPIO_IRQ_1        ZEBRA_GPIO_PIN_28_IRQ
/*
 * Define the buffer size for unique direction, it only pass the pointer instead of body
 */
#define ZEBRA_DEVICE0_BUFSZ         4612 /* ZEBRA_BUFFER_LIST_LENGTH * ZEBRA_NUM_IN_PACK + hdr, see share_memory_t */
#define ZEBRA_DEVICE1_BUFSZ         4612
#define ZEBRA_DEVICE2_BUFSZ         4612
#define ZEBRA_DEVICE3_BUFSZ         4612
#define ZEBRA_DEVICE4_BUFSZ         4612
#define ZEBRA_DEVICE5_BUFSZ         4612
#define ZEBRA_DEVICE6_BUFSZ         4612
#define ZEBRA_DEVICE7_BUFSZ         4612
#define ZEBRA_DEVICE8_BUFSZ         4612
#define ZEBRA_DEVICE9_BUFSZ         4612
#define ZEBRA_DEVICE10_BUFSZ        4612
#define ZEBRA_DEVICE11_BUFSZ        4612
#define ZEBRA_DEVICE12_BUFSZ        4612
#define ZEBRA_DEVICE13_BUFSZ        4612
#define ZEBRA_DEVICE14_BUFSZ        4612

/*
 * Buffer Offset for each device
 */
#define ZEBRA_DEVICE_0_BUFOFS       0
#define ZEBRA_DEVICE_1_BUFOFS       (ZEBRA_DEVICE_0_BUFOFS + ZEBRA_DEVICE0_BUFSZ)
#define ZEBRA_DEVICE_2_BUFOFS       (ZEBRA_DEVICE_1_BUFOFS + ZEBRA_DEVICE1_BUFSZ)
#define ZEBRA_DEVICE_3_BUFOFS       (ZEBRA_DEVICE_2_BUFOFS + ZEBRA_DEVICE2_BUFSZ)
#define ZEBRA_DEVICE_4_BUFOFS       (ZEBRA_DEVICE_3_BUFOFS + ZEBRA_DEVICE3_BUFSZ)
#define ZEBRA_DEVICE_5_BUFOFS       (ZEBRA_DEVICE_4_BUFOFS + ZEBRA_DEVICE4_BUFSZ)
#define ZEBRA_DEVICE_6_BUFOFS       (ZEBRA_DEVICE_5_BUFOFS + ZEBRA_DEVICE5_BUFSZ)
#define ZEBRA_DEVICE_7_BUFOFS       (ZEBRA_DEVICE_6_BUFOFS + ZEBRA_DEVICE6_BUFSZ)
#define ZEBRA_DEVICE_8_BUFOFS       (ZEBRA_DEVICE_7_BUFOFS + ZEBRA_DEVICE7_BUFSZ)
#define ZEBRA_DEVICE_9_BUFOFS       (ZEBRA_DEVICE_8_BUFOFS + ZEBRA_DEVICE8_BUFSZ)
#define ZEBRA_DEVICE_10_BUFOFS      (ZEBRA_DEVICE_9_BUFOFS + ZEBRA_DEVICE9_BUFSZ)
#define ZEBRA_DEVICE_11_BUFOFS      (ZEBRA_DEVICE_10_BUFOFS + ZEBRA_DEVICE10_BUFSZ)
#define ZEBRA_DEVICE_12_BUFOFS      (ZEBRA_DEVICE_11_BUFOFS + ZEBRA_DEVICE11_BUFSZ)
#define ZEBRA_DEVICE_13_BUFOFS      (ZEBRA_DEVICE_12_BUFOFS + ZEBRA_DEVICE12_BUFSZ)
#define ZEBRA_DEVICE_14_BUFOFS      (ZEBRA_DEVICE_13_BUFOFS + ZEBRA_DEVICE13_BUFSZ)
#define ZEBRA_DEVICE_BUFOFS_END     (ZEBRA_DEVICE_14_BUFOFS + ZEBRA_DEVICE14_BUFSZ)

/*
 * define the status
 */
#define ZEBRA_STATUS_START                  PAGE_ALIGN(ZEBRA_DEVICE_BUFOFS_END) //for mapping to NCNB
#define ZEBRA_STATUS_BUFFER_SIZE            128 /* for all devices, it can up to 128/4=32 devices */

/* STATUS */
#define ZEBRA_RX_READY_STATUS_ADDR          ZEBRA_STATUS_START
#define ZEBRA_TX_EMPTY_STATUS_ADDR          (ZEBRA_RX_READY_STATUS_ADDR + ZEBRA_STATUS_BUFFER_SIZE)

/* RXREADY / TXEMPTY */
#define ZEBRA_GATHER_RX_READY_STATUS_ADDR   (ZEBRA_TX_EMPTY_STATUS_ADDR + ZEBRA_STATUS_BUFFER_SIZE)
#define ZEBRA_GATHER_TX_EMPTY_STATUS_ADDR   (ZEBRA_GATHER_RX_READY_STATUS_ADDR + 4)
#define ZEBRA_STATUS_END                    (ZEBRA_GATHER_TX_EMPTY_STATUS_ADDR + 4)
/* Total status size */
#define ZEBRA_TOTAL_STATUSSZ                (ZEBRA_STATUS_END - ZEBRA_STATUS_START)

/* The total memory size */
#define ZEBRA_MEM_END                       ZEBRA_STATUS_END
#define ZEBRA_TOTAL_MEMSZ                   (PAGE_ALIGN(ZEBRA_MEM_END - ZEBRA_DEVICE_0_BUFOFS))

#endif /* __ZEBRA_SDRAM_H__ */


