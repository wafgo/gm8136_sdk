#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/ftpmu010.h>
#include <linux/delay.h>

static int scu_fd;

#define CLKOFF            0xB4
#define CLKOFF_THINK2D    (0x1<<13)

#define CLKSEL            0x28
#define CLKSEL_THINK2D    (0x1<<17)

#define CLKRST            0xA0
#define CLKRST_THINK2D    (0x1<<15)

#define PWMMOD            0x94
#define PWMPERIOD_MASK    0xFFFF0000 
#define PWMMOD_MASK       (0x1 << 2 | 0x1 << 1)
#define PWMACTIVE_MASK    0x01

/* *INDENT-OFF* */   
static pmuReg_t pmu_reg[] = {
/* {reg_off,    bits_mask,  lock_bits,  init_val,    init_mask } */
    {CLKOFF,      CLKOFF_THINK2D,  CLKOFF_THINK2D,   0x0 ,  0x0},    // on/off
    {CLKRST,      CLKRST_THINK2D,  CLKRST_THINK2D,   0x0 ,  0x0},    // active low
    {PWMMOD , 0xFFFFFFFF, 0xFFFFFFFF, 0x0, 0x0},    //pwm 250MHZ
};

static pmuRegInfo_t pmu_reg_info = {
    "THINK2D",
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_NONE,
    &pmu_reg[0],
};

void platform_power_save_mode(int is_enable);

int platform_init(unsigned short pwm_v)
{
    scu_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (scu_fd < 0)
        panic("Failed to register ft2dge pmu\n");
#if 0/*8139 think2d module is fail , so doesn't open it*/   
   /*think2d release reset pin*/
   //ftpmu010_write_reg(scu_fd, CLKRST, 0x0 << 15, 0x1 << 15);
    ftpmu010_write_reg(scu_fd, CLKRST, 0x1 << 15, 0x1 << 15);

   /*config PWM mode*/
    if(pwm_v != 0xFFFF){
        ftpmu010_write_reg(scu_fd, PWMMOD, (pwm_v << 16), PWMPERIOD_MASK);
        ftpmu010_write_reg(scu_fd, PWMMOD, (0x01 << 1), PWMMOD_MASK);
        ftpmu010_write_reg(scu_fd, PWMMOD, 0x1, PWMACTIVE_MASK);
    }
#endif    
    return 0;
}

void platform_exit(void)
{
#if 0 /* avoid someone is accessing the 2dengine */
    //turn off clock
    ftpmu010_write_reg(scu_fd, 0xB4, 0x1 << 20, 0x1 << 20);
#endif
    platform_power_save_mode(1);
    if (ftpmu010_deregister_reg(scu_fd))
        panic("Failed to deregister ft2dge\n");
}
void platform_power_save_mode(int is_enable)
{
#if 0/*8139 think2d module is fail , so doesn't open it*/  

    if(is_enable){
        /*turn OFF MCLK*/
        ftpmu010_write_reg(scu_fd, CLKOFF, 0x1 << 13, CLKOFF_THINK2D); 
    }
    else{        
        /*turn on MCLK*/       
        ftpmu010_write_reg(scu_fd, CLKOFF, 0x0 << 13, CLKOFF_THINK2D);
    }  
#endif    
}
