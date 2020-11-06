#include <linux/platform_device.h>
#include <mach/platform-GM8136/platform_io.h>
#include <mach/ftpmu010.h>
#include "fscaler300.h"

static int scaler300_fd;    //pmu
static u64 all_dmamask = ~(u32)0;

static void fscaler300_platform_release(struct device *dev)
{
    return;
}

// need to change in platform_io.h
// original 0~0x47fff up to 300k 0~0x4afff
/* SCALER300 resource 0*/
static struct resource scaler300_0_resource[] = {
	[0] = {
		.start = SCAL_FTSCAL300_0_PA_BASE,
		.end   = SCAL_FTSCAL300_0_PA_LIMIT,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = SCAL_FTSCAL300_0_IRQ,
		.end   = SCAL_FTSCAL300_0_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device scaler300_0_device = {
	.name		= "FSCALER300",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(scaler300_0_resource),
	.resource	= scaler300_0_resource,
	.dev		= {
				.platform_data = 0,	//index for port info
				.dma_mask = &all_dmamask,
				.coherent_dma_mask = 0xffffffffUL,
				.release = fscaler300_platform_release,
	},
};

static struct platform_device *platform_scaler300_devices[] = {
	&scaler300_0_device,
};

/*
 * PMU registers
 */
static pmuReg_t	pmu_reg[] = {
    /* ofs,     bit mask,    lock bits,   init value,  init mask */
    {0xb0,     0x1 << 14,   0x1 << 14,      0,         0x1 << 14}, //aximclkoff
};

static pmuRegInfo_t	pmu_reg_info = {
	"FSCALER300",
	ARRAY_SIZE(pmu_reg),
	ATTR_TYPE_AXI,
	&pmu_reg[0]
};

int platform_init(void)
{
    scaler300_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (scaler300_fd < 0)
        panic("%s, ftpmu010_register_reg fail", __func__);

    /* Add platform devices */
	platform_add_devices(platform_scaler300_devices, ARRAY_SIZE(platform_scaler300_devices));

    return 0;
}

int platform_exit(void)
{
    int dev;
    ftpmu010_deregister_reg(scaler300_fd);

    for (dev = 0; dev < priv.eng_num; dev ++)
        platform_device_unregister((struct platform_device *)platform_scaler300_devices[dev]);

    return 0;
}

