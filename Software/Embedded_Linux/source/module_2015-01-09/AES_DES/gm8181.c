#include <linux/kernel.h>
#include <linux/version.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include "platform.h"
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,14))
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>
#else
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,14))
int es_fd = 0;
static pmuReg_t security_pmu[] = {
    {0x38, (0x1 << 9), (0x1 << 9), 0, 0},       //turn on/off clock
    {0x6C, (0x1 << 5), (0x1 << 5), 0, 0}        //reset
};

pmuRegInfo_t security_pmu_info = {
    MODULE_NAME,
    ARRAY_SIZE(security_pmu),
    ATTR_TYPE_PLL3,
    security_pmu
};
#else
#define BIT(x) (1<<x)
#endif

/*
 * Platform init function. This platform dependent.
 */
int platform_pmu_init(void)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,14))
    es_fd = ftpmu010_register_reg(&security_pmu_info);
    if (es_fd < 0) {
        printk("Error in regster PMU \n");
        return -1;
    }
    ftpmu010_write_reg(es_fd, 0x38, 0, 1 << 9); // turn on AES clock
    ftpmu010_write_reg(es_fd, 0x6C, 1 << 5, 1 << 5);    // release AES 
#else
#if defined(CONFIG_PLATFORM_GM8185_v2) || defined(CONFIG_PLATFORM_GM8180)
	*(volatile unsigned int *)(PMU_FTPMU010_VA_BASE + 0x38) = (*(volatile unsigned int *)(PMU_FTPMU010_VA_BASE + 0x38)) & 0xFFFFDFFF;// turn on AES clock
#elif defined(CONFIG_PLATFORM_GM8181)
	*(volatile unsigned int *)(PMU_FTPMU010_VA_BASE + 0x38) = (*(volatile unsigned int *)(PMU_FTPMU010_VA_BASE + 0x38)) & 0xFFFFFDFF;// turn on AES clock
	*(volatile unsigned int *)(PMU_FTPMU010_VA_BASE + 0x6C) |= BIT(5);// turn on AES clock
#elif defined(CONFIG_PLATFORM_GM8126)
	*(volatile unsigned int *)(PMU_FTPMU010_VA_BASE + 0x38) = (*(volatile unsigned int *)(PMU_FTPMU010_VA_BASE + 0x38)) & 0xFFFFFDFF;// turn on AES clock
	*(volatile unsigned int *)(PMU_FTPMU010_VA_BASE + 0x6C) |= BIT(9);// turn on AES clock
#elif
	printk("Not define this platform\n");	
#endif
#endif

    return 0;
}

/*
 * Platform exit function. This platform dependent.
 */
int platform_pmu_exit(void)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,14))
    ftpmu010_write_reg(es_fd, 0x38, 1 << 9, 1 << 9);    // turn off AES clock
    ftpmu010_deregister_reg(es_fd);
#else
#if defined(CONFIG_PLATFORM_GM8185_v2) || defined(CONFIG_PLATFORM_GM8180)
	*(volatile unsigned int *)(PMU_FTPMU010_VA_BASE + 0x38) |= 0x2000;// turn off AES clock
#endif 
#if defined(CONFIG_PLATFORM_GM8181)
	*(volatile unsigned int *)(PMU_FTPMU010_VA_BASE + 0x38) |= BIT(9);// turn off AES clock
#endif 	
#endif

    return 0;
}
