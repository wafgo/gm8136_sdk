#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/ftpmu010.h>
#include <linux/delay.h>

static int scu_fd;

#define CLKOFF            0xB4
#define CLKOFF_THINK2D    (0x1<<20)

#define CLKSEL            0x28
#define CLKSEL_THINK2D    (0x1<<17)

#define CLKRST            0xA0
#define CLKRST_THINK2D    (0x1<<15)

/* *INDENT-OFF* */
static pmuReg_t pmu_reg[] = {
/* {reg_off,    bits_mask,  lock_bits,  init_val,    init_mask } */
    {CLKOFF,      CLKOFF_THINK2D,  CLKOFF_THINK2D,   0,  0},    // on/off
    {CLKSEL,      CLKSEL_THINK2D,  CLKSEL_THINK2D,   0,  0},    // 0:hclk/2 1:hclk
    {CLKRST,      CLKRST_THINK2D,  CLKRST_THINK2D,   0,  0},    // active low
};

static pmuRegInfo_t pmu_reg_info = {
    "THINK2D",
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_AHB,
    &pmu_reg[0],
};

int platform_init(unsigned short pwm_v)
{
    if(pwm_v){}

    scu_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (scu_fd < 0)
        panic("Failed to register ft2dge pmu\n");
#if 1
    
    //0:hclk/2 1:hclk use hclk/2
    ftpmu010_write_reg(scu_fd, 0x28, 0x0 << 17, 0x1 << 17);

    ftpmu010_write_reg(scu_fd, CLKRST, 0x1 << 15, 0x1 << 15);
    udelay(100);
    ftpmu010_write_reg(scu_fd, CLKRST, 0x0 << 15, 0x1 << 15);
    
    ftpmu010_write_reg(scu_fd, CLKOFF, 0x0 << 20, 0x1 << 20);

    printk("Think2dGE uses hclk/2. \n");
   
	/*think2d release reset pin*/
	//ftpmu010_write_reg(scu_fd, CLKRST, 0x0 << 15, 0x1 << 15);
	ftpmu010_write_reg(scu_fd, CLKRST, 0x1 << 15, 0x1 << 15);

#endif   
    return 0;
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
void platform_power_save_mode(int is_enable,unsigned short pwm_v)
{
    /*do nothing*/
}

