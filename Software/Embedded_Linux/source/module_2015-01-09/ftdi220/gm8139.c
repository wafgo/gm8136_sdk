#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>
#include "ftdi220.h"
#include <mach/fmem.h>

static int di220_fd;    //pmu
static u64 all_dmamask = ~(u32)0;

extern ushort pwm;

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

static struct platform_device *platform_3di_devices[] = {
	&di220_0_device,
};

/*
 * PMU registers
 */
static pmuReg_t	pmu_reg[] = {
    /* ofs, bit mask,   lock bits,   init value,  init mask */
    {0xA0, (0x1 << 5), (0x1 << 5), (0x0 << 5), (0x1 << 5)}, //IPs reset control by register : active low
    {0xb0, (0x1 << 10), (0x1 << 10), (0x0 << 10), (0x1 << 10)},   //aximclkoff, default is off
    {0xbc, (0x1 << 14), (0x1 << 14), 0, (0x1 << 14)},   //apbmclkoff1
    {0x9C, 0xFFFFFFFF, 0xFFFFFFFF, 0x1, 0xFFFFFFFF},    //pwm
};

static pmuRegInfo_t	pmu_reg_info = {
	"FTDI220",
	ARRAY_SIZE(pmu_reg),
	ATTR_TYPE_NONE,
	&pmu_reg[0]
};

int platform_init(void)
{
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    //compile check
    if (fmem_get_identifier(&pci_id, &cpu_id) < 0)
        panic("Error compile for di220! \n");

    di220_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (di220_fd < 0)
        panic("%s, ftpmu010_register_reg fail", __func__);

    /* release the reset */
    ftpmu010_write_reg(di220_fd, 0xA0, 0x1 << 5, 0x1 << 5);

#if 1 /* default should be off */
    ftpmu010_write_reg(di220_fd, 0x9C, 0x7, 0x7);
#else
    if (pwm != 0) {
        u32 value = ((pwm & 0xFFFF) << 16) | (0x1 << 1) | 0x1;
        ftpmu010_write_reg(di220_fd, 0x9C, value, 0xFFFFFFFF);
    }
#endif
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
    if (dev) {}

    if (on_off == 1) {
        /* on */
        if (pwm) {
            u32 value = ((pwm & 0xFFFF) << 16) | (0x1 << 1) | 0x1;
            ftpmu010_write_reg(di220_fd, 0x9C, value, 0xFFFFFFFF);
        } else {
            //mode: 2'b00- always on; 2'b01- PWM mode; 2'b1X- always off
            ftpmu010_write_reg(di220_fd, 0x9c, 0x1, 0x7);
        }
    } else {
        /* off */
        ftpmu010_write_reg(di220_fd, 0x9c, 0x7, 0x7);
    }
}

MODULE_LICENSE("GPL");

