#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>
#include "ftdi220.h"
#include <mach/fmem.h>

static int di220_fd;    //pmu
static u64 all_dmamask = ~(u32)0;

void platform_mclk_onoff(int dev, int on_off);
static void ftdi220_platform_release(struct device *dev)
{
    return;
}

/* DI220 resource */
static struct resource di220_0_resource[] = {
	[0] = {
		.start = DI3D_FTDI3D_0_PA_BASE,
		.end   = DI3D_FTDI3D_0_PA_LIMIT,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DI3D_FTDI3D_0_IRQ,
		.end   = DI3D_FTDI3D_0_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device di220_0_device = {
	.name		= "FTDI220",
	.id		    = 0,
	.num_resources	= ARRAY_SIZE(di220_0_resource),
	.resource	= di220_0_resource,
	.dev		= {
				.platform_data = 0,	//index for port info
				.dma_mask = &all_dmamask,
				.coherent_dma_mask = 0xffffffffUL,
				.release = ftdi220_platform_release,
	},
};

/* DI220 resource */
static struct resource di220_1_resource[] = {
	[0] = {
		.start = DI3D_FTDI3D_1_PA_BASE,
		.end   = DI3D_FTDI3D_1_PA_LIMIT,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DI3D_FTDI3D_1_IRQ,
		.end   = DI3D_FTDI3D_1_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device di220_1_device = {
	.name		= "FTDI220",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(di220_1_resource),
	.resource	= di220_1_resource,
	.dev		= {
				.platform_data = 0,	//index for port info
				.dma_mask = &all_dmamask,
				.coherent_dma_mask = 0xffffffffUL,
				.release = ftdi220_platform_release,
	},
};

static struct platform_device *platform_3di_devices[] = {
	&di220_0_device,
	&di220_1_device,
};

/*
 * PMU registers
 */
static pmuReg_t	pmu_reg[] = {
    /* ofs, bit mask,   lock bits,   init value,  init mask */
    /* 0x28: bit19 : di3dclk_sel 0-pll1out1_div4, 1-pll4out4_div1 */
    {0x28, (0x1 << 19), (0x1 << 19), (0x1 << 19), (0x1 << 19)}, //Frequency divider Setting Register
    {0xb0, (0x1 << 15) | (0x1 << 22), (0x1 << 15) | (0x1 << 22), 0, (0x1 << 15) | (0x1 << 22)}, //aximclkoff
    {0xbc, 0x1 | (0x1 << 4), 0x1 | (0x1 << 4), 0, 0x1 | (0x1 << 4)}, //apbmclkoff1
    {0xa0, (0x3 << 5), (0x3 << 5), 0, 0},   //IP Reset
};

static pmuRegInfo_t	pmu_reg_info = {
	"FTDI220",
	ARRAY_SIZE(pmu_reg),
	ATTR_TYPE_NONE,
	&pmu_reg[0]
};

int platform_init(void)
{
    int dev;
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    //compile check
    if (fmem_get_identifier(&pci_id, &cpu_id) < 0)
        panic("Error compile for di220! \n");

    di220_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (di220_fd < 0)
        panic("%s, ftpmu010_register_reg fail", __func__);

    /* reset ip */
    for (dev = 0; dev < ARRAY_SIZE(platform_3di_devices); dev ++) {
        platform_mclk_onoff(dev, 1);
        if (dev == 0) {
            /* release reset */
            ftpmu010_write_reg(di220_fd, 0xa0, 0x1 << 5, 0x1 << 5);
        } else {
            /* release reset */
            ftpmu010_write_reg(di220_fd, 0xa0, 0x1 << 6, 0x1 << 6);
        }

        platform_mclk_onoff(dev, 0);
    }

    /* Add platform devices */
	platform_add_devices(platform_3di_devices, ARRAY_SIZE(platform_3di_devices));

    return 0;
}

int platform_exit(void)
{
    int dev;
    ftpmu010_deregister_reg(di220_fd);

    for (dev = 0; dev < priv.eng_num; dev ++)
        platform_device_unregister((struct platform_device *)platform_3di_devices[dev]);

    return 0;
}

/* 1 for on, 0 for off */
void platform_mclk_onoff(int dev, int on_off)
{
    if (dev || on_off) {}
}

MODULE_LICENSE("GPL");

