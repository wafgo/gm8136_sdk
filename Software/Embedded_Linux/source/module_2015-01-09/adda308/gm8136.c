#include <linux/platform_device.h>
#include <mach/platform-GM8136/platform_io.h>
#include <mach/ftpmu010.h>
#include "adda308.h"

static int adda_fd;
static u64 all_dmamask = ~(u32)0;

static void adda308_platform_release(struct device *dev)
{
    return;
}

// need to change in platform_io.h
// original 0~0x47fff up to 300k 0~0x4afff
static struct resource adda308_0_resource[] = {
	[0] = {
		.start = ADDA_WRAP_PA_BASE,
		.end   = ADDA_WRAP_PA_LIMIT,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device adda308_0_device = {
	.name		= "adda308",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(adda308_0_resource),
	.resource	= adda308_0_resource,
	.dev		= {
				.platform_data = 0,	//index for port info
				.dma_mask = &all_dmamask,
				.coherent_dma_mask = 0xffffffffUL,
				.release = adda308_platform_release,
	},
};


static struct platform_device *platform_adda308_devices[] = {
	&adda308_0_device,
};

/*
 * PMU registers
 */
static pmuReg_t pmu_reg[] = {
    /* reg_off,  bit_masks,     lock_bits,      init_val,   init_mask */
    {0x28,     0x3<<15|0x7<<24,   0x0,      0x1<<15|0x2<<24,  0x3<<15|0x7<<24},   // adda clk pll1 cntp3:810/3=270
    {0x74,     0x3f << 24,     0x0,            0xa << 24, 0x3f << 24 },   // adda clock divided : 270/(10+1) = 24.5454MHz
    //{0x28,     0x3 << 15,      0x0,            0x2 << 15,  0x3 << 15 },   // adda clk 0:PLL3(594MHz) 1: PLL1(400MHz) 2:PLL2(600MHz)
    //{0x74,     0x3f << 24,     0x0,            0x18 << 24, 0x3f << 24 },   // adda clock divided : 600/(24+1) = 24MHz
    {0x7c,     0x1 << 27,      0x0,            0,          0x1 << 27  },   // adda da source sel : 0:ssp0 1:ssp1
    {0xac,     0x1 << 2,       0x0,            0,          0x1 << 2   },   // adda gated clock
    {0xb8,     0x1 << 6,       0x0,            0,          0x1 << 6   },   // adda wrapper gated clock
    {0x64,     0xf<<12,        0x0,            0x0<<12,    0xf<<12    },   // Set default PMU as GPIO, for DMIC(Mode2) 0x64 bit 12,13,14,15
    {0x50,     0xf<<6,         0x0,            0x0<<6,     0xf<<6     },   // Set default PMU as ICE, for DMIC(Mode2) 0x50 bit 6,7,8,9,
};

static pmuRegInfo_t	pmu_reg_info = {
	"adda308",
	ARRAY_SIZE(pmu_reg),
	ATTR_TYPE_NONE,
	&pmu_reg[0]
};

int platform_init(void)
{
    printk("%s\n", __func__);
    adda_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (adda_fd < 0)
        panic("%s, ftpmu010_register_reg fail", __func__);
    priv.adda308_fd = adda_fd;
    /* Add platform devices */
	platform_add_devices(platform_adda308_devices, ARRAY_SIZE(platform_adda308_devices));

    return 0;
}

int platform_exit(void)
{
    int dev = 0;

    ftpmu010_deregister_reg(priv.adda308_fd);

    platform_device_unregister((struct platform_device *)platform_adda308_devices[dev]);

    return 0;
}

