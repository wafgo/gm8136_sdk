#include <linux/kernel.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>
#include "platform.h"

int es_fd = 0;
static pmuReg_t security_pmu[] = {
    {0xB4, (0x1 << 15), (0x1 << 15), 0, 0},     //turn on/off clock
    {0xA0, (0x1 << 17), (0x1 << 17), 0, 0}      //reset
};

pmuRegInfo_t security_pmu_info = {
    MODULE_NAME,
    ARRAY_SIZE(security_pmu),
    ATTR_TYPE_PLL3,
    security_pmu
};

/*
 * Platform init function. This platform dependent.
 */
int platform_pmu_init(void)
{
    es_fd = ftpmu010_register_reg(&security_pmu_info);
    if (es_fd < 0) {
        printk("Error in regster PMU \n");
        return -1;
    }
    ftpmu010_write_reg(es_fd, 0xB4, 0, 1 << 15);        // turn on AES clock
    ftpmu010_write_reg(es_fd, 0xA0, 1 << 17, 1 << 17);  // release AES 

    return 0;
}

/*
 * Platform exit function. This platform dependent.
 */
int platform_pmu_exit(void)
{
    ftpmu010_write_reg(es_fd, 0xB4, 1 << 15, 1 << 15);  // turn off AES clock
    ftpmu010_deregister_reg(es_fd);

    return 0;
}
