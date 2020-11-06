/*-----------------------------------------------------------------
 * Includes
 *-----------------------------------------------------------------
 */
#include <linux/version.h>      /* kernel versioning macros      */
#include <linux/module.h>       /* kernel modules                */
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm-generic/gpio.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <mach/ftpmu010.h>
#include <mach/fmem.h>
#include <mach/platform/platform_io.h>
#include <cpu_comm/cpu_comm.h>
#include "zebra_sdram.h"        /* Host-to-DSP SDRAM layout      */
#include "zebra.h"
#include "platform.h"

#define SHARE_GPIO_INT

/* remote SCU address */
#define PCIE_SCU_BASE   ((PMU_FTPMU010_PA_BASE - AXI_IP_BASE) + PCI_IP_BASE)

/*
 * pmu pin mux for GPIO (interrupt purpose)
 */
static pmuReg_t gpio_pmu_reg[] = {
#if (ZEBRA_GPIO_PIN_0 == GPIO_PIN_27)
    /* X_I2C3 pin mux. 01: gpio[28:27] */
    {0x50, 0x3 << 26, 0x3 << 26, 0x1 << 26, 0x3 << 26},
#else
    /* X_I2C4 pin mux. 01: gpio[51:52] */
    {0x50, 0x3 << 28, 0x3 << 28, 0x1 << 28, 0x3 << 28},
#endif
};
static pmuRegInfo_t pmu_reg_info = {
    "CPUCOMM",
    ARRAY_SIZE(gpio_pmu_reg),
    ATTR_TYPE_NONE,
    &gpio_pmu_reg[0]
};

int cpucomm_fd = -1;

/*
 * DMA channels the cpu can use
 */
#define DAMC030_FREECH_FA726    (0x3 << 4)  /* chan4, and 5, this is for FA726 */
#define DMAC030_FREECH_FA626    (0x3 << 6)  /* chan6 and 7, this is for FA626 */

#define GPIO_H2D    ZEBRA_GPIO_PIN_1    //GPIO_PIN_28
#define GPIO_D2H    ZEBRA_GPIO_PIN_0    //GPIO_PIN_27

/* local variables */
static unsigned int gpio_base = 0, pci_gpio_base = 0;
static unsigned int pcie_base = 0, pci_pcie_base = 0;
static unsigned int pci_scu_base = 0;

/* local functions */
static void init_gpio(zebra_board_t *brd);
static unsigned int gpio_read_int(void);
static void pci_wait_isr_clear(unsigned int peer_gpio_base, unsigned int value);
static void pci_gpio_clear_data(unsigned int peer_gpio_base, unsigned int value);
static void gpio_clear_int(unsigned int value);
static void gpio_set_data(unsigned int data);

static inline int is_gpio_irq(zebra_board_t *brd)
{
    int ret = 0;

    switch (brd->local_irq) {
      case ZEBRA_GPIO_IRQ_0:
      case ZEBRA_GPIO_IRQ_1:
        ret = 1;
        break;
      default:
        ret = 0;
        break;
    }

    return ret;
}

static void platform_trigger_pcie_intr(int sw_intr)
{
    u32 irq = sw_intr - CPU_INT_BASE;
    volatile u32 value;

    /* trigger the pci sw interrupt. When the peer receives the interrupt, it will clear local
     * sw interrupt by calling ftpmu010_clear_intr()
     */
    value = ioread32(pci_scu_base + 0xA8);
    value |= (0x1 << irq);
    iowrite32(value, pci_scu_base + 0xA8);
}

/* determine which DMA channels are used */
u32 platform_get_dma_chanmask(void)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
    u32             dma_mask;

    fmem_get_identifier(&pci_id, &cpu_id);
    switch (cpu_id) {
      case FMEM_CPU_FA726:
        dma_mask = DAMC030_FREECH_FA726;
        break;
      case FMEM_CPU_FA626:
        dma_mask = DMAC030_FREECH_FA626;
        break;
      default:
        panic("unknown cpu id: 0x%x \n", cpu_id);
        break;
    }

    return dma_mask;
}

/*
 * ISR handler processes the doorbell interupt.
 *
 * In dual chip system, only one GPIO pin is connected between two chips. Thus, we use share GPIO
 * mechanism to dispatch the interrupt.
 * In FA726 of pci host, it uses IRQF_SHARED to share the same GPIO pin.
 * In FA626 of pci device, FA626 will redirect the interrupt to FA726 by trigger sw interrupt.
 */
static irqreturn_t platform_gpio_doorbell_interrupt(int irq, void *devid)
{
    zebra_board_t *brd = (zebra_board_t *)devid;
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
    int             gpio_h2d, gpio_d2h;
    u32 value;

    /* process the indoor interrupt */
    if (!IS_PCI(brd))
        for (;;) printk("%s, wrong pci interrupt %d! \n", __func__, irq);

    /* redirect sw interrupt due to workaround */
    fmem_get_identifier(&pci_id, &cpu_id);
    if (pci_id == FMEM_PCI_HOST) {
#ifndef SHARE_GPIO_INT
        ftpmu010_trigger_intr(ZEBRA_RC_FA726_IRQ0);
#endif
    } else {
        if (cpu_id == FMEM_CPU_FA626) {
            ftpmu010_trigger_intr(ZEBRA_EP_FA726_IRQ0);
        } else {
            for (;;) printk("%s, bug! \n", __func__);
        }
    }

    gpio_h2d = (GPIO_H2D >= 32) ? GPIO_H2D - 32 : GPIO_H2D;
    gpio_d2h = (GPIO_D2H >= 32) ? GPIO_D2H - 32 : GPIO_D2H;

    value = gpio_read_int();

    if (value & (1 << gpio_h2d)) {  //device mode
        if (platform_get_pci_type() == PCIX_HOST)
            panic("PCI Device error to receive GPIO%d interrupt!\n", gpio_d2h);

        /* to have host clear its data (put the interrupt source down of the peer) */
        pci_gpio_clear_data(pci_gpio_base, 1 << gpio_h2d);
        /* confirm the interrupt of the peer is really down */
        pci_wait_isr_clear(pci_gpio_base, 1 << gpio_h2d);
        /* clear local interupt */
        gpio_clear_int(1 << gpio_h2d);
    } else if (value & (1 << gpio_d2h)) {   //host mode
        if (platform_get_pci_type() != PCIX_HOST)
            panic("PCI host error to receive GPIO%d interrupt!\n", gpio_d2h);

        /* to have device clear its data (put the interrupt source down of the peer) */
        pci_gpio_clear_data(pci_gpio_base, 1 << gpio_d2h);
        /* confirm the interrupt of the peer is really down */
        pci_wait_isr_clear(pci_gpio_base, 1 << gpio_d2h);
        /* clear local interupt */
        gpio_clear_int(1 << gpio_d2h);
    } else {
        //printk("cpucomm gpio dummy interrupt: %d \n", irq);
        return IRQ_HANDLED;
    }

#ifdef SHARE_GPIO_INT
    {
        zebra_driver_t  *drv = brd->driver;
        zebra_board_t   *brd_node;

        list_for_each_entry(brd_node, &drv->board_list, list) {
            if (!IS_PCI(brd_node))
                continue;
            zebra_process_doorbell(brd_node);
        }
    }
#else
    zebra_process_doorbell(brd);
#endif

    return IRQ_HANDLED;
}

/*
 * ISR handler processes the doorbell interupt
 */
static irqreturn_t platform_doorbell_interrupt(int irq, void *devid)
{
    zebra_board_t *brd = (zebra_board_t *)devid;

    /* process the indoor interrupt */
    if (irq != brd->local_irq)
        for (;;) printk("%s, wrong irq: %d, expected irq: %d \n", __func__, irq, brd->local_irq);

    /* clear scu interupt first, otherwise we may loss interrupt.
     */
    ftpmu010_clear_intr(irq);

    zebra_process_doorbell(brd);

    return IRQ_HANDLED;
}

/*
 * Doorbell interupt init.
 */
int platform_doorbell_init(zebra_board_t *brd)
{
    irq_handler_t doorbell_isr;

    if (brd->local_irq < 0)
        panic("%s, irq: %d \n", __func__, brd->local_irq);

    doorbell_isr = is_gpio_irq(brd) ? platform_gpio_doorbell_interrupt : platform_doorbell_interrupt;
    if (request_irq(brd->local_irq, doorbell_isr, 0, brd->name, brd) < 0)
        panic("%s, %s register irq%d fail! \n", __func__, brd->name, brd->local_irq);

    /* workaround here. Pevent someone from setting gpio again */
    if (1) {
        fmem_pci_id_t   pci_id;
        fmem_cpu_id_t   cpu_id;
        u32             ep_cnt;
        static int  pmu_init = 0;

        fmem_get_identifier(&pci_id, &cpu_id);

        /* the case that GPIO is inited in FA726 only, FA626 doesn't lock the pinmux. Someone else
         * may set the incorrect pinmux to affect gpio. Thus we add lock here
         */
        if ((pci_id == FMEM_PCI_HOST) && (cpu_id == FMEM_CPU_FA626)) {
            ep_cnt = ftpmu010_get_attr(ATTR_TYPE_EPCNT);
            if (ep_cnt && !pmu_init && (cpucomm_fd < 0)) {
                cpucomm_fd = ftpmu010_register_reg(&pmu_reg_info);
                if (cpucomm_fd < 0)
                    panic("cpu register pmu fail! \n");
                pmu_init = 1;
            }
        } /* if */
    } /* if (1) */

    return 0;
}

/*
 * Send the interrupt to the peer.
 * If the target is PCIe, use GPIO as the interrupt event.
 */
void platform_trigger_interrupt(zebra_board_t *brd, unsigned int doorbell)
{
    int gpio_h2d, gpio_d2h;

    gpio_h2d = (GPIO_H2D >= 32) ? GPIO_H2D - 32 : GPIO_H2D;
    gpio_d2h = (GPIO_D2H >= 32) ? GPIO_D2H - 32 : GPIO_D2H;

    if (doorbell)   {}

    if (brd->peer_irq < 0)
        panic("%s, peer_irq: %d \n", __func__, brd->peer_irq);

    if (brd->doorbell_delayms)
        msleep(brd->doorbell_delayms);

    if (IS_PCI(brd)) {
#ifdef USE_GPIO
        /* currently, gpio is used in pci notification */
        if (platform_get_pci_type() == PCIX_HOST)
            gpio_set_data(1 << gpio_h2d);
        else
            gpio_set_data(1 << gpio_d2h);
#else
        /* sw interupt */
        platform_trigger_pcie_intr(brd->peer_irq);
#endif
    } else {
        /* sw interupt */
        ftpmu010_trigger_intr(brd->peer_irq);
    }

    /* avoid compile warning */
    if (0) platform_trigger_pcie_intr(0);
}

/* Free irq.
 */
void platform_doorbell_exit(zebra_board_t *brd)
{
    free_irq(brd->local_irq, brd);

    if (1) {
        /* workaround in platform_doorbell_init() */
        if (cpucomm_fd >= 0) {
            ftpmu010_deregister_reg(cpucomm_fd);
            cpucomm_fd = -1;
        }
    }
}

void platform_pci_init(zebra_board_t *brd)
{
    static int pci_init = 0;

    if (pci_init == 1)
        return;

    pci_init = 1;

    if (cpucomm_fd < 0) {
        cpucomm_fd = ftpmu010_register_reg(&pmu_reg_info);
        if (cpucomm_fd < 0)
            panic("cpu register pmu fail! \n");
    }

    /* remote scu base */
    pci_scu_base = (u32)ioremap_nocache(PCIE_SCU_BASE, PAGE_SIZE);
    if (!pci_scu_base) panic("cpucomm pci_scu_base ioremap fail! \n");

    /* local PCIE vbase */
    pcie_base = (u32)ioremap_nocache(PCIE_PLDA_PA_BASE, PAGE_SIZE);
    if (!pcie_base) panic("cpucomm pcie_base ioremap fail! \n");

    /* remote PCIE vbase */
    pci_pcie_base = (u32)ioremap_nocache((PCIE_PLDA_PA_BASE - AXI_IP_BASE + PCI_IP_BASE), PAGE_SIZE);
    if (!pci_pcie_base) panic("cpucomm pci_pcie_base ioremap fail! \n");

    init_gpio(brd);
}

void platform_pci_exit(zebra_board_t *brd)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
    static int pci_exit = 0;

    fmem_get_identifier(&pci_id, &cpu_id);

    /* Only support one PCI device. For multiple PCI devices, we must change the code
     */
    if (IS_PCI(brd) && !pci_exit) {
        pci_exit = 1;

        if (cpu_id == FMEM_CPU_FA726) {
            gpio_free(GPIO_H2D);
            gpio_free(GPIO_D2H);
        }

        if (pci_scu_base) __iounmap((void *)pci_scu_base);
        pci_scu_base = 0;
        if (pcie_base) __iounmap((void *)pcie_base);
        pcie_base = 0;
        if (pci_pcie_base) __iounmap((void *)pci_pcie_base);
        pci_pcie_base = 0;

        /* de-register pmu */
        if (cpucomm_fd >= 0) {
            ftpmu010_deregister_reg(cpucomm_fd);
            cpucomm_fd = -1;
        }
    }
}

pcix_id_t platform_get_pci_type(void)
{
    static u32 value = 0xFF;

    if (value == 0xFF)
        value =(ftpmu010_read_reg(0x04) >> 19) & 0x3;

    /* 01: Endpoint, 10: RootPoint */
    return (value == 0x2) ? PCIX_HOST : PCIX_DEV0;
}

/* Set inbound window
 *
 * If I am EP, I only configure my axi-base for host to access my DDR.
 */
void platform_set_inbound_win(zebra_board_t *brd, u32 axi_base)
{
//    int i = 0;

    if (!IS_PCI(brd))
        return;

    if (platform_get_pci_type() == PCIX_HOST)
        return;

    if (axi_base)   {}

    /* EP configure the axi-base itself, 0x99E00000 */
    //iowrite32(axi_base, pcie_base + 0x120);

}

/* Set outbound window
 * If I am RC, I must configure the remote PCI outbound window
 * If I am EP, I only configure my axi-base for host to access the ddr.
 */
void platform_set_outbound_win(zebra_board_t *brd, u32 peer_ddr_base)
{
    if (platform_get_pci_type() != PCIX_HOST)
        return;

    /* RC configure the remote PCI win, 0x99E00000 */
    iowrite32(peer_ddr_base, pci_pcie_base + 0xF8); //org is 0xE8
}

/* -----------------------------------------------------------------------------------------
 * The following code is GPIO utility. Actually Linux kernel provides the GPIO library to
 * the users already. But the GPIO library is only for local GPIO control excluding the remote
 * PCI GPIO control. For the interface consistent, we use the own GPIO utility to implement
 * GPIO library.
 * -----------------------------------------------------------------------------------------
 */

/*
 * @brief GPIO data direction function
 *
 * @function void gpio_set_data_direct(u_int group, u_int ctrl_pin, u_int data)
 * @param data_mask the mask to notify cared data pin
 * @param data the data to be set
*/
void gpio_set_data_direct(unsigned int data_mask, unsigned int data)
{
	unsigned int tmp1 = 0;

    /* PinDir, 0 for input, 1 for output */
	tmp1 = *(volatile unsigned int *)(gpio_base + 0x8);

	*(volatile unsigned int *)(gpio_base + 0x8) = data | (tmp1 & (~data_mask));
}

/*
 * @brief GPIO data set (GpioDataSet)
 *
 * @function void gpio_set_data(u_int data)
 * @param data the data to be set
*/
void gpio_set_data(unsigned int data)
{
    /* GpioDataSet */
	*(volatile unsigned int *)(gpio_base + 0x10) = data;
}

/*
 * @brief GPIO data clear
 *
 * @function void gpio_clear_data(u_int data)
 * @param data the data to be clear
*/
void gpio_clear_data(unsigned int data)
{
    /* GpioDataClear */
	*(volatile unsigned int *)(gpio_base + 0x14) = data;
}

/*
 * @brief other PCI's GPIO clear data
 *
 * @function void pci_gpio_clear_data(unsigned int gpio_base,unsigned int value)
 * @param value the value to clear interrupt to other side of PCI
*/
void pci_gpio_clear_data(unsigned int peer_gpio_base, unsigned int value)
{
    /* GpioDataClear */
	*(volatile unsigned int *)(peer_gpio_base + 0x14) = value;
}

/*
 * @brief read GPIO int status
 *
 * @function void gpio_read_int(unsigned int data)
 * @return interrupt status
*/
unsigned int gpio_read_int(void)
{
    /* IntrMaskedState. GPIO interrupt masked status register
     * 0: Interrupt is not detected or masked.
     * 1: Interrupt is detected and not masked
     */
	return *(volatile unsigned int *)(gpio_base + 0x28);
}

/*
 * @brief GPIO clear int
 *
 * @function void gpio_clear_int(unsigned int value)
 * @param value the value to clear interrupt
*/
void gpio_clear_int(unsigned int value)
{
    /* GPIO interrupt clear, 0: No effect, 1: Clear interrupt */
	*(volatile unsigned int *)(gpio_base + 0x30) = value;
}

/*
 * @brief GPIO interrupt attribute
 *
 * @function void gpio_set_int
 * @param data_mask the mask to notify cared data pin
 * @param int_mask_enable the enable of mask about interrupt
 * @param int_trigger_method 0:Edge 1:Level
 * @param rise_method 0:rising 1:falling/0:hi_level 1:low_level
*/
void gpio_set_int(unsigned int data_mask,unsigned int int_mask_enable,
    unsigned int int_trigger_method,unsigned int rise_method)
{
    unsigned int tmp1;

    tmp1 = *(volatile unsigned int *)(gpio_base + 0x34);
    *(volatile unsigned int *)(gpio_base + 0x34) = int_trigger_method | (tmp1 & (~data_mask));
    tmp1 = *(volatile unsigned int *)(gpio_base + 0x3c);
    *(volatile unsigned int *)(gpio_base + 0x3c)= rise_method|(tmp1 & (~data_mask));
    tmp1 = *(volatile unsigned int *)(gpio_base + 0x20);
    *(volatile unsigned int *)(gpio_base + 0x20)= data_mask | (tmp1 & (~data_mask));
    tmp1 = *(volatile unsigned int *)(gpio_base + 0x2c);
    *(volatile unsigned int *)(gpio_base + 0x2c)= int_mask_enable | (tmp1 & (~data_mask));
}

/*
 * @brief GPIO initalization function
 *
 * @function void init_gpio(zebra_board_t *brd)
*/
void init_gpio(zebra_board_t *brd)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;
    volatile u32    gpio_phybase;
    int             gpio_h2d, gpio_d2h, ret;
    static int      init_done = 0;

    if (init_done)
        return;

    init_done = 1;

    gpio_h2d = (GPIO_H2D >= 32) ? GPIO_H2D - 32 : GPIO_H2D;
    gpio_d2h = (GPIO_D2H >= 32) ? GPIO_D2H - 32 : GPIO_D2H;

    /* request gpio pin */
    ret = gpio_request(GPIO_H2D, "h2d");
    if (ret) panic("cpucomm requests gpio %d fail! \n", GPIO_H2D);
    ret = gpio_request(GPIO_D2H, "d2h");
    if (ret) panic("cpucomm requests gpio %d fail! \n", GPIO_H2D);

    gpio_phybase = (GPIO_H2D >= 32) ? GPIO_FTGPIO010_1_PA_BASE : GPIO_FTGPIO010_PA_BASE;
    gpio_base = (u32)ioremap_nocache(gpio_phybase, PAGE_SIZE);
    if (!gpio_base) panic("cpucomm gpio_base ioremap fail! \n");

    gpio_phybase = (GPIO_D2H >= 32) ? GPIO_FTGPIO010_1_PA_BASE : GPIO_FTGPIO010_PA_BASE;
    pci_gpio_base = (u32)ioremap_nocache((gpio_phybase - AXI_IP_BASE + PCI_IP_BASE), PAGE_SIZE);
    if (!pci_gpio_base) panic("cpucomm gpio_base ioremap fail! \n");

    /* init the gpio as the interrupt source, the following code prevents two CPUs from double init gpio.
     */
    fmem_get_identifier(&pci_id, &cpu_id);
    if (pci_id == FMEM_PCI_HOST) {
        if (cpu_id != FMEM_CPU_FA726)
            return;
    } else {
        if (cpu_id != FMEM_CPU_FA626)
            return;
    }

    if (platform_get_pci_type() == PCIX_HOST) {
        /* host mode
         */
        gpio_set_data_direct(1 << gpio_d2h, 0); //set input
        gpio_set_data_direct(1 << gpio_h2d, 1 << gpio_h2d);     //set output
        gpio_clear_data(1 << gpio_h2d);
        gpio_set_int(1 << gpio_d2h, 0, 1 << gpio_d2h, 0);
        gpio_clear_int(1 << gpio_d2h);
    } else {
        /* device mode
         */
        gpio_set_data_direct(1 << gpio_h2d, 0); //set input
        gpio_set_data_direct(1 << gpio_d2h, 1 << gpio_d2h);     //set output
        gpio_clear_data(1 << gpio_d2h);
        gpio_set_int(1 << gpio_h2d, 0, 1 << gpio_h2d, 0);
        gpio_clear_int(1 << gpio_h2d);
    }
    mdelay(200);                //wait GPIO actived
}

/*
 * @brief wait isr bit is cleared from pci
 *
 * @function void wait_isr_clear(unsigned int value)
 * @param value the value of checking data
*/
void pci_wait_isr_clear(unsigned int peer_gpio_base, unsigned int value)
{
    volatile unsigned int data;
    int i = 0;

    /* when GpioDataClear is set, we must wait the GpioDataOut is really clear */
    while (i < 100) {
        data = *(volatile unsigned int *)peer_gpio_base;
        udelay(10);
        if ((data & value) == 0)
            break;
        pci_gpio_clear_data(peer_gpio_base, value);
        i ++;
    }

    if (i >= 100)
        panic("%s, clear pci gpio fail! \n", __func__);
}
