#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/ftpmu010.h>
#include <linux/delay.h>

static int scu_fd;

/* *INDENT-OFF* */
static pmuReg_t pmu_reg[] = {
/* {reg_off,    bits_mask,  lock_bits,  init_val,    init_mask } */
    {0xB4,      0x1 << 20,  0x1 << 20,   0x0 << 20,  0x1 << 20},    // on/off
    {0x28,      0x1 << 17,  0x1 << 17,   0x1 << 17,  0x1 << 17},    // 0:hclk/2 1:hclk
    {0xA0,      0x1 << 15,  0x1 << 15,   0x0,        0x0},          // active low
};

static pmuRegInfo_t pmu_reg_info = {
    "FT2DGE",
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_NONE,
    &pmu_reg[0],
};


void platform_init(void)
{
    scu_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (scu_fd < 0)
        panic("Failed to register ft2dge pmu\n");
    
    //0:hclk/2 1:hclk
    if (ftpmu010_get_attr(ATTR_TYPE_PMUVER) != PMUVER_A) {
        ftpmu010_write_reg(scu_fd, 0x28, 0x0 << 17, 0x1 << 17);
        printk("Ft2dge uses hclk/2. \n");
    } else {
        printk("Ft2dge uses hclk. \n");
    }
}

void platform_exit(void)
{
#if 0 /* avoid someone is accessing the 2dengine */
    //turn off clock
    ftpmu010_write_reg(scu_fd, 0xB4, 0x1 << 20, 0x1 << 20);
#endif

    if (ftpmu010_deregister_reg(scu_fd))
        panic("Failed to deregister ft2dge\n");
}
