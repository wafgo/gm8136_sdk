#include <linux/kernel.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>
#include "platform.h"

int es_fd = 0;
static pmuReg_t security_pmu[] = {
    {0x38, (0x1 << 9), (0x1 << 9), 0, 0},       //turn on/off clock
    {0x6C, (0x1 << 9), (0x1 << 9), 0, 0}        //reset
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
    ftpmu010_write_reg(es_fd, 0x38, 0, 1 << 9); // turn on AES clock
    ftpmu010_write_reg(es_fd, 0x6C, 1 << 9, 1 << 9);    // release AES 

    return 0;
}

/*
 * Platform exit function. This platform dependent.
 */
int platform_pmu_exit(void)
{
    ftpmu010_write_reg(es_fd, 0x38, 1 << 9, 1 << 9);    // turn off AES clock
    ftpmu010_deregister_reg(es_fd);
    
    return 0;
}
