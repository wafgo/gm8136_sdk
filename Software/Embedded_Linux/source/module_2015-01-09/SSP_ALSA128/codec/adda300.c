#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <asm/div64.h>
#include <asm/io.h>
//#include <mach/platform/pmu.h>
#include <mach/platform/board.h>
#include <mach/ftpmu010_pcie.h>
#include "adda300.h"

static int pmu_fd = -1;

static pmuPcieReg_t reg_array_adda300_pcie[] = {
    /* reg_off,  bit_masks,     lock_bits,      init_val,   init_mask */
    { 0x30,     (0x3 << 14),    (0x3 << 14),    0x0,        (0x3 << 14)},   // audio ad/da mclk on, SSP0/1 clk on
    { 0x48,     0x3,            0x3,            0x3,        0x3        },   // audio ad/da mclk, SSP0/1 clk : 12MHz
    { 0x64,     0xFFFFFFFF,     0xFFFFFFFF,     0,          0          },
    { 0x68,     0xFFFFFFFF,     0xFFFFFFFF,     0,          0          },
    { 0x6c,     0xFFFFFFFF,     0xFFFFFFFF,     0,          0          },
};

static pmuPcieRegInfo_t adda300_pcie_clk_info = {
    "adda300_pcie_clk",
    ARRAY_SIZE(reg_array_adda300_pcie),
    ATTR_TYPE_PCIE_NONE,
    reg_array_adda300_pcie,
};

#if  DEBUG_ADDA300
void dump_adda300_reg(void)
{
    DEBUG_ADDA300_PRINT("\n0x30 = %x\n", readl(PCIPMU_FTPMU010_VA_BASE + 0x30));
    DEBUG_ADDA300_PRINT("0x48 = %x\n", readl(PCIPMU_FTPMU010_VA_BASE + 0x48));
    DEBUG_ADDA300_PRINT("0x64 = %x\n", readl(PCIPMU_FTPMU010_VA_BASE + 0x64));
    DEBUG_ADDA300_PRINT("0x68 = %x\n", readl(PCIPMU_FTPMU010_VA_BASE + 0x68));
    DEBUG_ADDA300_PRINT("0x6c = %x\n", readl(PCIPMU_FTPMU010_VA_BASE + 0x6c));

}
#endif //end of DEBUG_ADDA300

static void adda300_pmu_register(void)
{
    pmu_fd = ftpmu010_pcie_register_reg(&adda300_pcie_clk_info);
    if (pmu_fd < 0) {
        panic("In %s: register pmu setting failed\n", __func__);
    }
}

static void adda300_i2s_init(void)
{
    volatile u32 tmp = 0;
    volatile u32 mask = 0;

    /* reset */
    tmp = 1 << 19;
    mask = 1 << 19;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x6c, tmp, mask) < 0)
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x6c);
    mdelay(10);
    tmp = 0;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x6c, tmp, mask) < 0)
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x6c);

    /* I2S mode */
    tmp = ftpmu010_pcie_read_reg(0x64);
    tmp &= ~(1 << 12);              // ADC unmute
    tmp |= (1 << 17);               // set to I2S mode
    tmp |= (0x06 << 28);            // default MCLK AD mode set to serve 48K sampling rate
    tmp |= (0x06 << 24);            // default MCLK DA mode set to serve 48K sampling rate
    tmp |= 0x3F ;                   // set DAC digital volume control
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x64, tmp, mask) < 0)
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x64);

    /* set DAC power on & DAC unmute */
    tmp = ftpmu010_pcie_read_reg(0x68);
    tmp &= ~0x03;
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x68, tmp, mask) < 0) {
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x68);
    }
    /* set master/slave mode */
    tmp = ftpmu010_pcie_read_reg(0x6C);
#if SSP_SLAVE_MODE
    tmp |= (0x01 << 18);            // set I2S Master Mode
#else
    tmp &= ~(0x01 << 18);           // set I2S Slave Mode
#endif
    tmp |= (0x01 << 13);            // enable MIC boost
    tmp |= (0x01 << 14);            // IRSEL to Normal setting
    tmp &= ~(0x01 << 10);           // set not mute

    tmp &= ~(0x3F << 4);            // set ADC gain 36 dB
    tmp |= (0x3F << 4);

    tmp &= ~(0x01 << 16);           // Line OUT mute control --> normal
    tmp &= ~(0x01 << 17);           // LOUT power-down control --> normal
    tmp &= ~(0x01 << 12);           // ADC power-down control --> normal
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x6C, tmp, mask) < 0) {
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x6c);
    }
}

/* audio dac da_i2s_sel. 0: from pad, 1: from ssp0/1 */
void adda300_I2S_mux(enum adda300_port_select ps, int mux)
{
    volatile u32 tmp = 0;
    volatile u32 mask = 0;

    tmp = ftpmu010_pcie_read_reg(0x6C);
    if(ps == ADDA300_DA){
        if(mux)
            tmp |= 0x01;
        else
            tmp &= ~0x01;
    }else {
        if(mux)
            tmp |= 0x02;
        else
            tmp &= ~0x02;
    }
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x6C, tmp, mask) < 0) {
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x78);
    }
    return;
}

void adda300_init(const enum adda300_port_select ps)
{
    if(ps > ADDA300_DA)
    {
        DEBUG_ADDA300_PRINT("%s fails: gm8126 should only accept ADDA300_AD or ADDA300_DA\n",
                            __FUNCTION__);
        return;
    }

    adda300_pmu_register();

    adda300_i2s_init();

#if DEBUG_ADDA300
    DEBUG_ADDA300_PRINT("In %s", __FUNCTION__);
    dump_adda300_reg();
#endif
}

void adda300_set_fs(const enum adda300_chip_select cs, const enum adda300_fs_frequency fs)
{
    volatile u32 tmp = 0;
    volatile u32 mask = 0;

    tmp = ftpmu010_pcie_read_reg(0x64);

    if (unlikely(cs != ADDA300_FIRST_CHIP)) {
        DEBUG_ADDA300_PRINT("%s fails: gm8126 should only accept ADDA300_FIRST_CHIP\n",
                            __FUNCTION__);
        return;
    }

    tmp &= ~(0x0F << 24);   // DA
    tmp &= ~(0x0F << 28);   // AD
    switch (fs) {
    case ADDA300_FS_8K:
    {
        tmp |= (0x04 << 24);
        tmp |= (0x04 << 28);
        break;
    }
    case ADDA300_FS_16K:
    {
        tmp |= (0x01 << 24);
        tmp |= (0x01 << 28);
        break;
    }
    case ADDA300_FS_32K:
    {
        //keep the four bits zero
        break;
    }
    case ADDA300_FS_44_1K:
    {
        tmp |= (0x0D << 24);
        tmp |= (0x0D << 28);
        break;
    }
    case ADDA300_FS_48K:
    {
#if SSP3_CLK_SRC_12000K     //ADDA300 MCLKMODE = MCLK(12000000) / SAMPLE RATE(48000) = 250
        tmp |= (0x06 << 24);
        tmp |= (0x06 << 28);
/* for HDMI playback sample rate 48k/16 bits , ssp3 bclk must = 3072000, sdl&pdl must = 16
   so ssp3's mclk must = 24.576Mhz, adda300's MCLKMODE must = 256 --> adda300's MCLK = MCLKMODE(256)* 48000 = 12.288MHz
*/
#else   // SSP3_CLK_SRC_24000K
        tmp |= (0x0f << 24);
        tmp |= (0x0f << 28);
#endif
        break;
    }
    default:
        DEBUG_ADDA300_PRINT("%s fails: fs wrong value\n", __FUNCTION__);
        return;
    }

    //set codec's MCLK mode setting
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x64, tmp, mask) < 0) {
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x78);
    }
    //unmute
    tmp = 0x0;
    mask = (1|(1<<1));
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x68, tmp, mask) < 0) {
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x78);
    }
#if DEBUG_ADDA300
    DEBUG_ADDA300_PRINT("In %s", __FUNCTION__);
    dump_adda300_reg();
#endif
}

void adda300_set_fs_ad_da(const enum adda300_port_select ps, const enum adda300_fs_frequency fs)
{
    volatile u32 tmp = 0;
    volatile u32 mask = 0;

    tmp = ftpmu010_pcie_read_reg(0x64);


    if(ps == ADDA300_DA)
        tmp &= ~(0xF << 24);   // DA
    else
        tmp &= ~(0xF << 28);   // AD

    switch (fs) {
    case ADDA300_FS_8K:
    {
        if(ps == ADDA300_DA)
            tmp |= (0x4 << 24);
        else
            tmp |= (0x4 << 28);
        break;
    }
    case ADDA300_FS_16K:
    {
        if(ps == ADDA300_DA)
            tmp |= (0x1 << 24);
        else
            tmp |= (0x1 << 28);
        break;
    }
    case ADDA300_FS_32K:
    {
        //keep the four bits zero
        break;
    }
    case ADDA300_FS_44_1K:
    {
        if(ps == ADDA300_DA)
            tmp |= (0xD << 24);
        else
            tmp |= (0xD << 28);
        break;
    }
    case ADDA300_FS_48K:
    {
        if(ps == ADDA300_DA)
#if SSP3_CLK_SRC_12000K     //ADDA300 MCLKMODE = MCLK(12000000) / SAMPLE RATE(48000) = 250
            tmp |= (0x6 << 24);
#else
            tmp |= (0xf << 24);
#endif
        else
#if SSP3_CLK_SRC_12000K     //ADDA300 MCLKMODE = MCLK(12000000) / SAMPLE RATE(48000) = 250
            tmp |= (0x6 << 28);
#else
            tmp |= (0xf << 28);
#endif
        break;
    }
    default:
        DEBUG_ADDA300_PRINT("%s fails: fs wrong value\n", __FUNCTION__);
        return;
    }

    //set codec's MCLK mode setting
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x64, tmp, mask) < 0) {
            panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x64);
    }

    if(ps == ADDA300_DA)
    {
        tmp = 0x0;
        mask = (1|(1<<1));
        if (ftpmu010_pcie_write_reg(pmu_fd, 0x68, tmp, mask) < 0) {
            panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x68);
        }
    }
#if DEBUG_ADDA300
    DEBUG_ADDA300_PRINT("In %s", __FUNCTION__);
    dump_adda300_reg();
#endif
    return;
}
