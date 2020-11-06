#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/hdreg.h>	/* HDIO_GETGEO */
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <mach/ftpmu010.h>

#include "debug.h"
#include "fmcp.h"

static int mcp100_clock_on = 0;

#ifdef SUPPORT_PWM
    extern unsigned int pwm;
#endif

static pmuReg_t regMCP100Array[] = {
    /* reg_off  bit_masks  lock_bits     init_val    init_mask */
#ifdef SUPPORT_PWM
    {0x98,      0xFFFF0007,  0xFFFF0007,  0x0,         0x0        },
#endif
    {0xA0,      (0x1 << 16), (0x1 << 16),  (0x0 << 16), (0x0 << 16)},
    {0xB4,      (0x1 << 12), (0x1 << 12),  (0x0 << 12), (0x0 << 12)},
};

static pmuRegInfo_t mcp100_clk_info = {
    "MCP100_CLK",
    ARRAY_SIZE(regMCP100Array),
    ATTR_TYPE_PLL1,
    regMCP100Array
};

static int ftpmu010_mcp100_fd = -1;

int pf_codec_init(void)
{
    ftpmu010_mcp100_fd = ftpmu010_register_reg(&mcp100_clk_info);
    
    if (unlikely(ftpmu010_mcp100_fd < 0)){
        printk("MCP100 registers to PMU fail! \n");
        return -1;
    }
    /*
    else
        printk("MCP100 pmu register\n");
    */
    return 0;
}

int pf_codec_exit(void)
{
    ftpmu010_deregister_reg(ftpmu010_mcp100_fd);
    ftpmu010_mcp100_fd = -1;
    //printk("MCP100 pmu deregister\n");
    return 0;
}

int pf_codec_on(void)
{
    if (mcp100_clock_on)
        return 0;
    if (ftpmu010_mcp100_fd < 0) {
        if (pf_codec_init() < 0)
            return -1;
    }
    mcp100_clock_on = 1;
#ifdef SUPPORT_PWM
    if (pwm & 0xFFFF)
        ftpmu010_write_reg(ftpmu010_mcp100_fd, 0x98, ((pwm&0xFFFF)<<16)|0x03, 0xFFFF0007);
    else
        ftpmu010_write_reg(ftpmu010_mcp100_fd, 0x98, 0x01,      0xFFFF0007);
#endif
    ftpmu010_write_reg(ftpmu010_mcp100_fd, 0xA0, 0x01<<16, 0x01<<16);   // reset
    ftpmu010_write_reg(ftpmu010_mcp100_fd, 0xB4, 0, 0x01<<12);          // ahb gatting
    return 1;
}

void pf_codec_off(void)
{
    if (0 == mcp100_clock_on)
        return;
    if (ftpmu010_mcp100_fd < 0)
        return;

    mcp100_clock_on = 0;
#ifdef SUPPORT_PWM
    ftpmu010_write_reg(ftpmu010_mcp100_fd, 0x98, 0x05,      0xFFFF0007);
#endif
    ftpmu010_write_reg(ftpmu010_mcp100_fd, 0xB4, 0x01<<12, 0x01<<12);
    ftpmu010_write_reg(ftpmu010_mcp100_fd, 0xA0, 0, 0x01<<16);
}

#ifdef ENABLE_SWITCH_CLOCK
#ifdef SUPPORT_PWM
    #define SWITCH_PWM_CLOCK
#endif
int pf_clock_switch_on(void)
{
    int ret = 0;
#ifndef SWITCH_PWM_CLOCK
    ret = pf_codec_on();
    mcp100_info("clock on\n");
#else
    #ifdef SUPPORT_PWM
        if (mcp100_clock_on)
            return 0;
        if (pwm & 0xFFFF)
            ftpmu010_write_reg(ftpmu010_mcp100_fd, 0x98, ((pwm&0xFFFF)<<16)|0x03, 0xFFFF0007);
        else
            ftpmu010_write_reg(ftpmu010_mcp100_fd, 0x98, 0x01,      0xFFFF0007);
        mcp100_clock_on = 1;
        mcp100_info("pwm clock on\n");
    #endif
#endif
    return ret;
}
void pf_clock_switch_off(void)
{
#ifndef SWITCH_PWM_CLOCK
    pf_codec_off();
    mcp100_info("clock off\n");
#else
    #ifdef SUPPORT_PWM
        if (0 == mcp100_clock_on)
            return;
        ftpmu010_write_reg(ftpmu010_mcp100_fd, 0x98, 0x05,      0xFFFF0007);
        mcp100_clock_on = 0;
        mcp100_info("pwm clock off\n");
    #endif
#endif
}
#endif

