#include <linux/platform_device.h>
#include <mach/platform-GM8139/platform_io.h>
#include <mach/ftpmu010.h>
#include <linux/synclink.h>
#include "ft3dnr.h"

static int ft3dnr_fd;    //pmu
static u64 all_dmamask = ~(u32)0;

static void ft3dnr_platform_release(struct device *dev)
{
    return;
}

// need to change in platform_io.h
// original 0~0x47fff up to 300k 0~0x4afff
/* FT3DNR resource 0*/
static struct resource ft3dnr_0_resource[] = {
	[0] = {
		.start = THDNR_FT3DNR_0_PA_BASE,
		.end   = THDNR_FT3DNR_0_PA_LIMIT,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = THDNR_FT3DNR_0_IRQ,
		.end   = THDNR_FT3DNR_0_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device ft3dnr_0_device = {
	.name		= "FT3DNR",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ft3dnr_0_resource),
	.resource	= ft3dnr_0_resource,
	.dev		= {
				.platform_data = 0,	//index for port info
				.dma_mask = &all_dmamask,
				.coherent_dma_mask = 0xffffffffUL,
				.release = ft3dnr_platform_release,
	},
};


static struct platform_device *platform_ft3dnr_devices[] = {
	&ft3dnr_0_device,
};

/*
 * PMU registers
 */
static pmuReg_t	pmu_reg[] = {
    /* ofs,     bit mask,               lock bits,          init value,  init mask */
    {0x88,      0xFFFF0007,             0,                  0x1,          0x1},         //pwm
    {0xb0,      0x1 << 11,              0x1 << 11,          0,            0x1 << 11}, //aximclkoff
    {0xbc,      0x1 << 13,              0x1 << 13,          0,            0x1 << 13}, //apbmclkoff
    {0xa0,      0x1 << 4,               0x1 << 4,           0x1 << 4,     0x1 << 4}, //aclk
};

static pmuRegInfo_t	pmu_reg_info = {
	"FT3DNR",
	ARRAY_SIZE(pmu_reg),
	ATTR_TYPE_AXI,
	&pmu_reg[0]
};

int platform_set_pwm(u32 pwm)
{
    u32 tmp = 0;
    int ret = 0;

    if (ft3dnr_fd < 0)
        return -1;

    // turn off clock
    ret = ftpmu010_write_reg(ft3dnr_fd, 0xbc, BIT13, BIT13);
    // set pwm
    tmp = ((pwm & 0xffff) << 16) | 0x3;
    ret = ftpmu010_write_reg(ft3dnr_fd, 0x88, tmp, 0xFFFF0007);

    while (1) {
        tmp = ftpmu010_read_reg(0x88);
        if ((tmp & BIT0) == 0)
            break;
    }

    // turn on clock
    ret = ftpmu010_write_reg(ft3dnr_fd, 0xbc, 0, BIT13);

    return ret;
}

int platform_clock_on(int on)
{
    if (on) {
        if (ftpmu010_write_reg(ft3dnr_fd, 0xbc, 0x0, 1 << 13))
            panic("%s, register 3dnr clock on fail\n", __func__);
    }
    else {
         if (ftpmu010_write_reg(ft3dnr_fd, 0xbc, 1 << 13, 1 << 13))
             panic("%s, register 3dnr clock off fail\n", __func__);
     }
     return 0;
}

int platform_init(void)
{
    ft3dnr_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (ft3dnr_fd < 0)
        panic("%s, ftpmu010_register_reg fail", __func__);

    /* Add platform devices */
	platform_add_devices(platform_ft3dnr_devices, ARRAY_SIZE(platform_ft3dnr_devices));

    return 0;
}

int platform_exit(void)
{
    int dev = 0;
    ftpmu010_deregister_reg(ft3dnr_fd);

    platform_device_unregister((struct platform_device *)platform_ft3dnr_devices[dev]);

    return 0;
}

