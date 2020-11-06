#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#if defined(CONFIG_PLATFORM_GM8136)
#include <mach/platform-GM8136/platform_io.h>
#elif defined(CONFIG_PLATFORM_GM8139)
#include <mach/platform-GM8139/platform_io.h>
#endif
#include <mach/ftpmu010.h>
#include <linux/slab.h>
#include <linux/times.h>
#include <linux/synclink.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/delay.h>

#include "adda302.h"
#include <adda302_api.h>
#include <platform.h>

#define SUPPORT_POWER_PD
#define VERSION     140808
adda302_priv_t priv;

extern int adda302_proc_init(void);
extern void adda302_proc_remove(void);

static u32 chip_id = 0;
static u32 bond_id = 0;

typedef enum {
    DMIC_PMU_64H,
    DMIC_PMU_50H
} ADDA302_DMIC_PMU_REG;

#if defined(CONFIG_PLATFORM_GM8136)
#define ADDA302_DMIC_DEF_PMU  DMIC_PMU_50H
#else
#define ADDA302_DMIC_DEF_PMU  DMIC_PMU_64H
#endif

#define GM813X_STEREO_TYPE  \
        (chip_id == 0x8139) || \
        ((chip_id == 0x8138) && ((bond_id == 0xCF) || (bond_id == 0xAF))) ||    \
        ((chip_id == 0x8137) && (bond_id == 0x6F))
#define GM813X_MONO_TYPE  \
        ((chip_id == 0x8138) && (bond_id == 0xDF)) ||  \
        ((chip_id == 0x8137) && (bond_id == 0x7F))

static pmuReg_t DMIC_Reg[] = {
    /* reg_off, bit_masks, lock_bits, init_val, init_mask */
    {  0x64,    0xf<<12,   0x0,       0xa<<12,  0xf<<12}, // Select PMU for DMIC(Mode2) 0x64 bit 12,13,14,15
    {  0x50,    0xf<<6,    0x0,       0xf<<6,   0xf<<6}   // Select PMU for DMIC(Mode2) 0x50 bit 6,7,8,9,
};

static void ADDA302_PMU_Select_DMIC_Input(pmuReg_t *reg);

#define SSP_MAX_USED    2
//input_mode: 0 -> analog MIC, 1 -> line in, 2 -> digital MIC
uint input_mode = 0;
module_param(input_mode, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(input_mode, "input mode can be analog MIC/line in/digital MIC");
//enable_dmic: 0 -> disable, 1 -> enable,
//NOTE: this module parameter is the only one which has no /proc/ interface,
//user MUST decide if enable it when inserting module
uint enable_dmic = 0;
module_param(enable_dmic, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(enable_dmic, "enable digital MIC");
//enable BTL or not
uint enable_BTL = 1;
module_param(enable_BTL, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(enable_BTL, "select 1: speaker mode or 0: headphone mode");
//single-ended / differential mode, 1 --> single ended mode, 0 --> differential mode
uint single_end = 1;
module_param(single_end, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(single_end, "select 1: single ended or 0: differential mode");
//ALC enable
uint enable_ALC = 0;
module_param(enable_ALC, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(enable_ALC, "enable ALC");
//ALCNGTH, from -78, -72, -66, -60, -54, -48, -42, -36dB
int ALCNGTH = -36;
module_param(ALCNGTH, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ALCNGTH, "ALC noise gate threshold");
//ALC MIN, from -27, -24, -21, -18, -15, -12, -9, -6dB
int ALCMIN = -27;
module_param(ALCMIN, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ALCMIN, "ALC minimum gain");
//ALC MAX, from -6, 0, 6, 12, 18, 24, 30, 36dB
int ALCMAX = -6;
module_param(ALCMAX, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ALCMAX, "ALC maximum gain");
//R channel input analog Gain, from -27 ~ +36 dB
int RIV = 15;
module_param(RIV, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(RIV, "R channel input analog gain");
//L channel input analog Gain, from -27 ~ +36 dB
int LIV = 15;
module_param(LIV, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(LIV, "L channel input analog gain");
//R channel input mute, 0 -> unmute, 1 -> mute
int RIM = 0;
module_param(RIM, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(RIM, "R channel input mute");
//L channel input mute, 0 -> unmute, 1 -> mute
int LIM = 0;
module_param(LIM, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(LIM, "L channel input mute");
//R channel input digital Gain, -50 ~ +30 dB
int RADV = 0;
module_param(RADV, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(RADV, "R channel input digital gain");
//L channel input digital Gain, -50 ~ 30 dB
int LADV = 0;
module_param(LADV, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(LADV, "L channel input digital gain");
//R channel output digital Gain, -40 ~ 0 dB
int RDAV = 0;
module_param(RDAV, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(RDAV, "R channel output digital gain");
//L channel output digital Gain, -40 ~ 0 dB
int LDAV = 0;
module_param(LDAV, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(LDAV, "L channel output digital gain");
//R channel output mute, 0 -> unmute, 1 -> mute
int RHM = 0;
module_param(RHM, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(RHM, "R channel output mute");
//L channel output mute, 0 -> unmute, 1 -> mute
int LHM = 0;
module_param(LHM, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(LHM, "L channel output mute");
//speaker output Gain, -40 ~ +6 dB
int SPV = 0;
module_param(SPV, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(SPV, "speaker volume");
//High pass filter
int HPF = 0;
module_param(HPF, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(HPF, "high pass filter");
//DAMIXER: simulate mono audio source to output stereo audio
int DAMIXER = 0;
module_param(DAMIXER, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(DAMIXER, "DAMIXER enable or not");
// ADDA is master or is slave
int ISMASTER = 1;
module_param(ISMASTER, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ISMASTER, "select 1: ADDA as master, 0: ADDA as slave");
// audio output enable
static int audio_out_enable[SSP_MAX_USED] = {1,0};
module_param_array(audio_out_enable, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audio_out_enable, "is output enable for each audio interface");
//use SSP0 as rx/tx or use SSP0 as rx, SSP1 as tx --> keep it, but don't use
uint same_i2s_tx_rx = 1;
module_param(same_i2s_tx_rx, uint, S_IRUGO|S_IWUSR);
// test mode only for testing adda performance, set micboost as 0db
uint test_mode = 0;
module_param(test_mode, uint, S_IRUGO|S_IWUSR);
// ADDA clock source from PLL2
#if defined(CONFIG_PLATFORM_GM8139)
uint clk_pll2 = 0;
#endif
#if defined(CONFIG_PLATFORM_GM8136)
uint clk_pll2 = 1;
#endif
module_param(clk_pll2, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(clk_pll2, "adda clock source from pll2");
#if defined(CONFIG_PLATFORM_GM8136)
// used to control ADDA codec's power
int power_control = 1;
module_param(power_control, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(power_control, "ADDA power control");
#endif
int reback = 0;

extern int platform_init(void);
extern int platform_exit(void);

#if DEBUG_ADDA302
void dump_adda302_reg(void)
{
	u32 tmp = 0;
	u32 base = priv.adda302_reg;
	DEBUG_ADDA302_PRINT("\n0x28 = %x\n", ftpmu010_read_reg(0x28));
	DEBUG_ADDA302_PRINT("0x74 = %x\n", ftpmu010_read_reg(0x74));
	DEBUG_ADDA302_PRINT("0x7C = %x\n", ftpmu010_read_reg(0x7C));
	DEBUG_ADDA302_PRINT("0xb8 = %x\n", ftpmu010_read_reg(0xb8));
	DEBUG_ADDA302_PRINT("0xac = %x\n", ftpmu010_read_reg(0xac));
	/* adda wrapper */
	printk("ADDA Wrapper\n");
	tmp = *(volatile unsigned int *)(base + 0x0);
	DEBUG_ADDA302_PRINT("0x00 = %x\n", tmp);
	tmp = *(volatile unsigned int *)(base + 0x4);
	DEBUG_ADDA302_PRINT("0x04 = %x\n", tmp);
	tmp = *(volatile unsigned int *)(base + 0x8);
	DEBUG_ADDA302_PRINT("0x08 = %x\n", tmp);
	tmp = *(volatile unsigned int *)(base + 0xc);
	DEBUG_ADDA302_PRINT("0x0c = %x\n", tmp);
	tmp = *(volatile unsigned int *)(base + 0x10);
	DEBUG_ADDA302_PRINT("0x10 = %x\n", tmp);
}
#endif//end of DEBUG_ADDA300

/*
 * Create engine and FIFO
 */
int add_engine(unsigned long addr)
{
    priv.adda302_reg = addr;

    return 0;
}

/* Note: adda302_init_one() will be run before adda302_drv_init() */
static int adda302_init_one(struct platform_device *pdev)
{
	struct resource *mem;
	unsigned long   vaddr;
    printk("adda302_init_one [%d] version %d\n", pdev->id, VERSION);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		printk("no mem resource?\n");
		return -ENODEV;
	}

    vaddr = (unsigned long)ioremap_nocache(mem->start, PAGE_ALIGN(mem->end - mem->start));
    if (unlikely(!vaddr))
        panic("%s, no virtual address! \n", __func__);

	add_engine(vaddr);

    return 0;
}

static int adda302_remove_one(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver adda302_driver = {
    .probe	= adda302_init_one,
    .remove	=  __devexit_p(adda302_remove_one),
	.driver = {
	        .name = "ADDA302",
	        .owner = THIS_MODULE,
	    },
};

/*
 * ADDA main clock is 12MHz,
 * at GM8138/GM8139
 * ssp source from PLL1 (540MHz)
 * when ADDA is master, SSP is slave, SSP main clock is 12MHz, meet sample rate:8/16/22.05k
 * when ADDA is master, SSP is slave, SSP main clock is 18MHz, meet sample rate:8/16/22.05/44.1/48k
 --------------------------------------------------------------------------------------------------
 * when ADDA is slave, SSP is master, SSP main clock is 12MHz, meet sample rate:8/22.05/44.1k(bclk not exactly meet)
 * when ADDA is slave, SSP is master, SSP main clock is 24.54(540/22)MHz, meet sample rate:8/16/22.05/44.1/48k(bclk not exactly meet)
 =======================================================================================================================
 * at GM8136
 * ssp source from PLL2 (600MHz)
 * when ADDA is master, SSP is slave, SSP main clock is 12MHz, meet sample rate:8/16/22.05k
 * when ADDA is master, SSP is slave, SSP main clock is 24MHz, meet sample rate:8/16/22.05/44.1/48k
 --------------------------------------------------------------------------------------------------
 * when ADDA is slave, SSP is master, SSP main clock is 12MHz, meet sample rate:8/22.05/44.1k(bclk not exactly meet)
 * when ADDA is slave, SSP is master, SSP main clock is 24(600/25)MHz, meet sample rate:8/16/22.05/44.1/48k(bclk not exactly meet)
 */
static void adda302_ssp_mclk_init(void)
{
    int ret = 0;

    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 0)) { // adda da source select SSP0
        if (chip_id == 0x8137 || chip_id == 0x8138 || chip_id == 0x8139) {
            if (ISMASTER == 1)
                ret = plat_set_ssp_clk(0, 18000000, clk_pll2);
            else
                ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
        }
        if (chip_id == 0x8135 || chip_id == 0x8136) {
            if (ISMASTER == 1)
                ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
            else
                ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
        }
        if (ret < 0)
            printk("ADDA set ssp0 main clock fail\n");
    }

    if ((audio_out_enable[0] == 0) && (audio_out_enable[1] == 1)) {  // adda da source select SSP1
        // adda ad for record, da for playback
        if (chip_id == 0x8137 || chip_id == 0x8138 || chip_id == 0x8139) {
            if (ISMASTER == 1)
                ret = plat_set_ssp_clk(0, 18000000, clk_pll2);
            else
                ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
        }
        if (chip_id == 0x8135 || chip_id == 0x8136) {
            if (ISMASTER == 1)
                ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
            else
                ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
        }
        if (ret < 0)
            printk("ADDA set ssp0 main clock fail\n");

        if (chip_id == 0x8137 || chip_id == 0x8138 || chip_id == 0x8139) {
            if (ISMASTER == 1)
                ret = plat_set_ssp_clk(1, 18000000, clk_pll2);
            else
                ret = plat_set_ssp_clk(1, 24000000, clk_pll2);
        }
        if (chip_id == 0x8135 || chip_id == 0x8136) {
            if (ISMASTER == 1)
                ret = plat_set_ssp_clk(1, 24000000, clk_pll2);
            else
                ret = plat_set_ssp_clk(1, 24000000, clk_pll2);
        }
        if (ret < 0)
            printk("ADDA set ssp1 main clock fail\n");
    }

    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 1)) { // adda da source select SSP0, SSP1 for decoder
        if (chip_id == 0x8137 || chip_id == 0x8138 || chip_id == 0x8139) {
            if (ISMASTER == 1)
                ret = plat_set_ssp_clk(0, 18000000, clk_pll2);
            else
                ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
        }
        if (chip_id == 0x8135 || chip_id == 0x8136) {
            if (ISMASTER == 1)
                ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
            else
                ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
        }
        if (ret < 0)
            printk("ADDA set ssp0 main clock fail\n");
    }
}

static void adda302_mclk_init(u32 hz)
{
	volatile u32 tmp = 0;
    volatile u32 mask = 0;
    volatile u32 pvalue = 0;
	unsigned long div = 0;
	unsigned long curr_div = 0;

	pvalue = ftpmu010_read_reg(0x74);
	curr_div = (pvalue & 0x3f000000) >> 24;

	//give ADDA302 12MHz main clock
    tmp = 0;
    mask = (BIT24|BIT25|BIT26|BIT27|BIT28|BIT29);
    if (chip_id == 0x8137 || chip_id == 0x8138 || chip_id == 0x8139) {
        if (clk_pll2 == 0)      // clock source from pll3
            div = PLL3_CLK_IN;
        if (clk_pll2 == 1)      // clock source from pll2/2
            div = PLL2_CLK_IN / 2;
    }

    if (chip_id == 0x8135 || chip_id == 0x8136) {
        if (clk_pll2 == 0)      // clock source from pll1 cntp3
            div = PLL1_CLK_IN;
        if (clk_pll2 == 1)      // clock source from pll2
            div = PLL2_CLK_IN;
        if (clk_pll2 == 2)      // clock source from pll3
            div = PLL3_CLK_IN;
    }

    div /= hz;
    div -= 1;
    // set clock divided value
    if (div != curr_div) {
        tmp |= (div << 24);
        DEBUG_ADDA302_PRINT("ADDA302 gets divisor %ld, PPL3_CLK = %d, mclk = %lu \n",
                div, PLL3_CLK_IN, PLL3_CLK_IN/(div+1));

        if (ftpmu010_write_reg(priv.adda302_fd, 0x74, tmp, mask) < 0) {
            panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x74);
        }
    }

    // set clock source, adda default clock source from pll3, set from pll2
    mask = 0;
    tmp = 0;
    if (chip_id == 0x8137 || chip_id == 0x8138 || chip_id == 0x8139) {
        if (clk_pll2 == 1) {
            tmp = 1 << 16;
            mask = BIT16;
        }
    }

    if (chip_id == 0x8135 || chip_id == 0x8136) {
        mask = BIT15 | BIT16;
        if (clk_pll2 == 0)
            tmp = 1 << 15;
        if (clk_pll2 == 1)
            tmp = 2 << 15;
        if (clk_pll2 == 2)
            tmp = 0 << 15;
    }

    if (ftpmu010_write_reg(priv.adda302_fd, 0x28, tmp, mask) < 0) {
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x28);
    }

}
#if 0
/* get current mclk from pvalue */
int adda302_get_mclk(void)
{
	volatile u32 pvalue = 0;
    volatile u32 mask = 0;
	int mclk = 0;

	//get audio clock pvalue
    pvalue = ftpmu010_read_reg(0x74);
    mask = (BIT24|BIT25|BIT26|BIT27|BIT28|BIT29);
    pvalue &= mask;
    pvalue >>= 24;

    mclk = PLL3_CLK_IN / (pvalue + 1);
    if (mclk >= 12000000 && mclk < 13000000)
        mclk = 12000000;    // 12Mhz

    if (mclk >= 16000000 && mclk < 17000000)
        mclk = 16000000;    // 16Mhz

    if (mclk >= 24000000 && mclk < 25000000)
        mclk = 24000000;    // 24Mhz

    //printk("%s mclk = %d, pvalue = %d\n", __func__, mclk, pvalue);

    return mclk;
}
#endif

void ADDA302_Set_DA_Source(int da_src)
{
    volatile u32 val = 0;
    volatile u32 mask = BIT27;

    //printk("%s, da_src(%d)\n", __func__, da_src);

    if (da_src >= DA_SRC_MAX) {
        printk("error DA source: %d\n", da_src);
        return;
    }

    val = (da_src == DA_SRC_SSP0)?(val & ~(1<<27)):(val | (1<<27));

    if (ftpmu010_write_reg(priv.adda302_fd, 0x7C, val, mask) < 0)
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x7C);
}

/*
 * set adda302 DA source select
 */
static void adda302_da_select(void)
{
	int ret = 0;
	volatile u32 tmp = 0;
    volatile u32 mask = 0;

    tmp = 0;
    mask = BIT27;

    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 0))  // adda da source select SSP0
        tmp &= ~(1 << 27);
    if ((audio_out_enable[0] == 0) && (audio_out_enable[1] == 1))  // adda da source select SSP1
        tmp |= (1 << 27);
    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 1))  // adda da source select SSP0, SSP1 for decoder
        tmp &= ~(1 << 27);

    if (ftpmu010_write_reg(priv.adda302_fd, 0x7C, tmp, mask) < 0)
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x7C);

    // When ADDA(DA) select source from SSP1, SSP1 also have to select source from ADDA(DA)
    if ((audio_out_enable[0] == 0) && (audio_out_enable[1] == 1)) {  // SSP1 source select ADDA(DA)
        ret = plat_set_ssp1_src(PLAT_SRC_ADDA);
        if (ret < 0)
            printk("%s set ssp1 source fail\n", __func__);
    }
    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 1)) {  // SSP1 source select DECODER(DA)
        ret = plat_set_ssp1_src(PLAT_SRC_ADDA);
        if (ret < 0)
            printk("%s set ssp1 source fail\n", __func__);
    }

}

#if defined(CONFIG_PLATFORM_GM8136)
void adda302_i2s_reset(void)
{
	volatile u32 tmp = 0;
	u32 base = 0;
    u32 ALCNGTH_reg_val = 0x0;      //0x0 is the default -36 dB reg val
    u32 ALCMIN_reg_val = 0x0;       //0x0 is the default -27 dB reg val
    u32 ALCMAX_reg_val = 0x0;       //0x0 is the default -6 dB reg val
    u32 IV_reg_val = 0x1B;         //0x1B is the default 0 dB reg val, 0x3F is 36dB, 0x25 is 10dB
    u32 IM_reg_val = 0;            //0 -> unmute, 1 -> mute
    u32 ADV_reg_val = 0x61;        //0x61 is the default 0 dB reg val
    u32 DAV_reg_val = 0x61;        //0x3F is the default 0 dB reg val
    u32 HM_reg_val = 0;            //0 -> unmute, 1 -> mute
    u32 SPV_reg_val = 0x0;         //0x39 is the default 0 dB reg val, 0x2A is -15dB

    base = priv.adda302_reg;

    if ((ALCNGTH > -36) || (ALCNGTH < -78)) {
        printk("%s: ALCNGTH(%d) MUST between -78 ~ -36dB, use default(-78dB)\n", __func__, ALCNGTH);
        ALCNGTH = -78;
    }

    if ((ALCMIN > -6) || (ALCMIN < -27)) {
        printk("%s: ALCMIN(%d) MUST between -27 ~ -6dB, use default(-6dB)\n", __func__, ALCMIN);
        ALCMIN = -6;
    }

    if ((ALCMAX > 36) || (ALCMAX < -6)) {
        printk("%s: ALCMAX(%d) MUST between -6 ~ +36dB, use default(-6dB)\n", __func__, ALCMAX);
        ALCMAX = -6;
    }

    if ((LIV > 36) || (LIV < -27)) {
        printk("%s: LIV(%ddB) MUST between -27dB ~ 36dB, use default(-2dB)\n", __func__, LIV);
        LIV = -2;
    }

    if ((LADV < -50) || (LADV > 30)) {
        printk("%s: LADV(%ddB) MUST between -50dB ~ 30dB, use default(0dB)\n", __func__, LADV);
        LADV = 0;
    }

    if ((LDAV < -40) || (LDAV > 0)) {
        printk("%s: DAV(%ddB) MUST between -40dB ~ 0dB, use default(-1dB)\n", __func__, LDAV);
        LDAV = -1;
    }

    if ((SPV < -40) || (SPV > 6)) {
        printk("%s: SPV(%ddB) MUST between -40dB ~ 6dB, use default(-1dB)\n", __func__, SPV);
        SPV = -1;
    }

    if (LIM != 0) { LIM = 1; }
    if (LHM != 0) { LHM = 1; }

    ALCNGTH_reg_val += (ALCNGTH - (-78))/6; //refer to spec
    ALCMIN_reg_val += (ALCMIN - (-27))/3; //refer to spec
    ALCMAX_reg_val += (ALCMAX - (-6))/6; //refer to spec

    IV_reg_val += LIV;
    ADV_reg_val += LADV;
    DAV_reg_val += LDAV;
    SPV_reg_val += SPV;
    IM_reg_val = LIM;
    HM_reg_val = LHM;

    // ===== start ADDA 00h =====
	tmp = *(volatile unsigned int *)(base + 0x0);

    if (ISMASTER == 1)  // ADDA as master
        tmp |= (0x1 << 1);
    else                // ADDA as slave
        tmp &= ~(0x1 << 1);

    if (reback == 1)    // enable reback
        tmp |= (0x1 << 3);
    else
        tmp &= ~(0x1 << 3);

    tmp &= ~(0x1 << 18);            // enable ADC frontend power
    tmp &= ~(0x1 << 20);            // enable ADC power
    tmp &= ~(0x1 << 22);            // enable DAC power
#if 0
    tmp |= (0x1 << 28);            // set MIC power down(MIC bias power down)
#else
    tmp &= ~(0x1 << 28);
#endif
    tmp |= (0x1 << 30);         // set mic mode as single-end input

    tmp &= ~(0x1 << 16);
    if (input_mode == 2)            // enable digital MIC
        tmp |= (0x1 << 16);

    *(volatile unsigned int *)(base + 0x0) = tmp;
    // ===== end ADDA 00h =====

    if (chip_id == 0x8135 || chip_id == 0x8136) {
        if (input_mode == 1)
            printk("[ADDA] Error !! 8135/8136 do not support line in mode\n");
    }

    // ===== start ADDA 04h =====
	tmp = *(volatile unsigned int *)(base + 0x4);
 
    tmp &= ~0x3;                                
    tmp &= ~(0x3 << 2);            // disable boost, 0 dB added
    if (test_mode == 0)
        tmp |= (0x1 << 2);         // enable boost, 20 dB added

    // set input unmute
    tmp &= ~(0x1 << 4);
    tmp |= (IM_reg_val << 4);

    // clear IV setting
	tmp &= ~(0x3F << 6);

    // set IV based on user's setting
    tmp |= (IV_reg_val << 6);

    // clear ADV setting
    tmp &= ~(0x7F << 18);

    // set RADV, LADV based on user's setting
    tmp |= (ADV_reg_val << 18);

    // apply 04h setting
    *(volatile unsigned int *)(base + 0x4) = tmp;
    // ===== end ADDA 04h =====

    // ===== start ADDA 08h =====
	tmp = *(volatile unsigned int *)(base + 0x8);
	tmp &= ~(0x3);                  // ADC gain 1.5db
	tmp |= 0x1;

    tmp &= ~(0x1 << 2);             // disable ADC DEM
    tmp |= (0x1 << 3);              // enable ADC DWA

    if (HPF==1)
        tmp|=(0x1<<4);

	if (enable_ALC)                 //enable ALC or not
        tmp |= (1 << 5);
    else
        tmp &= ~(1 << 5);
    // ALC noise gate threshold
    tmp &= ~(0x7 << 7);
    tmp |= (ALCNGTH_reg_val << 7);
    // ALC MAX / ALC MIN
    tmp &= ~(0x7 << 10);
    tmp &= ~(0x7 << 13);
    tmp |= (ALCMAX_reg_val << 10);
    tmp |= (ALCMIN_reg_val << 13);
    *(volatile unsigned int *)(base + 0x8) = tmp;
    // ===== end ADDA 08h =====

    // ===== start ADDA 0Ch =====
	tmp = *(volatile unsigned int *)(base + 0xc);

    if (chip_id == 0x8135 || chip_id == 0x8136) {
        tmp &= ~(0x7F);                 // set DAV
        tmp |= DAV_reg_val;
    }

    tmp &= ~(0x3 << 12);
    tmp |= (0x1 << 12);            // SDMGAIN (-2db)
    tmp &= ~(0x1 << 16);
    tmp |= (HM_reg_val << 16);

    if (chip_id == 0x8135 || chip_id == 0x8136) {
        tmp &= ~(0x1 << 7);                 // line out disable mute
        tmp &= ~(0x1 << 8);                 // enable LOUT
        tmp &= ~(0x3 << 25);
        tmp |= (SPV_reg_val << 25);
    }

    *(volatile unsigned int *)(base + 0xc) = tmp;
    // ===== end ADDA 0Ch =====

    // ===== start ADDA 10h =====
	tmp = *(volatile unsigned int *)(base + 0x10);

    // clear HYS
    tmp &= ~(0x1 << 0); // audio_hys1
    tmp &= ~(0x1 << 1); // audio_hys0

    // clear DT
    tmp &= ~(0x3 << 2);
    tmp |= (0x01 << 2);

    // set cm_en to default
    tmp |= (0x1 << 4);

    tmp &= ~(0x1 << 5);           // HPFAPP
    tmp &= ~(0x1 << 6);           // HPFCUT2
    tmp &= ~(0x1 << 7);           // HPFCUT1
    tmp &= ~(0x1 << 8);           // HPFCUT0
    tmp &= ~(0x1 << 9);           // HPFGATEST1
    tmp &= ~(0x1 << 10);          // HPFGATEST0
    
    // IRSEL
	tmp &= ~(0x07 << 16);           // reset IRSEL setting
	tmp |= (0x04 << 16);            // set IRSEL 4 as normal, 2 as power saving mode

    // SCFPD
    tmp &= ~(0x01 << 19);          // enable SCF in LOUT

    // IRSEL SPK
    tmp &= ~(0x7 << 24);
    tmp |= (0x04 << 24);

    tmp &= ~(0x03 << 28);
    if (power_control == 0) // power down
        tmp |= (0x01 << 28);
    else // power up
        tmp |= (0x01 << 29);        

    *(volatile unsigned int *)(base + 0x10) = tmp;
    // ===== end ADDA 10h =====

    //enable SPEAKER power
    tmp = *(volatile unsigned int *)(base + 0x0);
    tmp &= ~(0x01 << 27);
    *(volatile unsigned int *)(base + 0x0) = tmp;
}

#else // for products beyond GM8136

void adda302_i2s_reset(void)
{
    volatile u32 tmp = 0;
    u32 base = 0;
    u32 ALCNGTH_reg_val = 0x0;      //0x0 is the default -36 dB reg val
    u32 ALCMIN_reg_val = 0x0;       //0x0 is the default -27 dB reg val
    u32 ALCMAX_reg_val = 0x0;       //0x0 is the default -6 dB reg val
    u32 RIV_reg_val = 0x1B;         //0x1B is the default 0 dB reg val, 0x3F is 36dB, 0x25 is 10dB
    u32 LIV_reg_val = 0x1B;         //0x1B is the default 0 dB reg val, 0x3F is 36dB, 0x25 is 10dB
    u32 RIM_reg_val = 0;            //0 -> unmute, 1 -> mute
    u32 LIM_reg_val = 0;            //0 -> unmute, 1 -> mute
    u32 RADV_reg_val = 0x61;        //0x61 is the default 0 dB reg val
    u32 LADV_reg_val = 0x61;        //0x61 is the default 0 dB reg val
    u32 RDAV_reg_val = 0x3F;        //0x3F is the default 0 dB reg val
    u32 LDAV_reg_val = 0x3F;        //0x3F is the default 0 dB reg val
    u32 RHM_reg_val = 0;            //0 -> unmute, 1 -> mute
    u32 LHM_reg_val = 0;            //0 -> unmute, 1 -> mute
    u32 SPV_reg_val = 0x39;         //0x39 is the default 0 dB reg val, 0x2A is -15dB

    base = priv.adda302_reg;

    if ((ALCNGTH > -36) || (ALCNGTH < -78)) {
        printk("%s: ALCNGTH(%d) MUST between -78 ~ -36dB, use default(-78dB)\n", __func__, ALCNGTH);
        ALCNGTH = -78;
    }

    if ((ALCMIN > -6) || (ALCMIN < -27)) {
        printk("%s: ALCMIN(%d) MUST between -27 ~ -6dB, use default(-6dB)\n", __func__, ALCMIN);
        ALCMIN = -6;
    }

    if ((ALCMAX > 36) || (ALCMAX < -6)) {
        printk("%s: ALCMAX(%d) MUST between -6 ~ +36dB, use default(-6dB)\n", __func__, ALCMAX);
        ALCMAX = -6;
    }

    if ((RIV > 36) || (RIV < -27)) {
        printk("%s: RIV(%ddB) MUST between -27dB ~ 36dB, use default(-2dB)\n", __func__, RIV);
        RIV = -2;
    }

    if ((LIV > 36) || (LIV < -27)) {
        printk("%s: LIV(%ddB) MUST between -27dB ~ 36dB, use default(-2dB)\n", __func__, LIV);
        LIV = -2;
    }

    if ((RADV < -50) || (RADV > 30)) {
        printk("%s: RADV(%ddB) MUST between -50dB ~ 30dB, use default(0dB)\n", __func__, RADV);
        RADV = 0;
    }

    if ((LADV < -50) || (LADV > 30)) {
        printk("%s: LADV(%ddB) MUST between -50dB ~ 30dB, use default(0dB)\n", __func__, LADV);
        LADV = 0;
    }

    if ((RDAV < -40) || (RDAV > 0)) {
        printk("%s: RDAV(%ddB) MUST between -40dB ~ 0dB, use default(-1dB)\n", __func__, RDAV);
        RDAV = -1;
    }

    if ((LDAV < -40) || (LDAV > 0)) {
        printk("%s: LDAV(%ddB) MUST between -40dB ~ 0dB, use default(-1dB)\n", __func__, LDAV);
        LDAV = -1;
    }

    if ((SPV < -40) || (SPV > 6)) {
        printk("%s: SPV(%ddB) MUST between -40dB ~ 6dB, use default(-1dB)\n", __func__, SPV);
        SPV = -1;
    }

    if (RIM != 0) { RIM = 1; }
    if (LIM != 0) { LIM = 1; }
    if (RHM != 0) { RHM = 1; }
    if (LHM != 0) { LHM = 1; }

    ALCNGTH_reg_val += (ALCNGTH - (-78))/6; //refer to spec
    ALCMIN_reg_val += (ALCMIN - (-27))/3; //refer to spec
    ALCMAX_reg_val += (ALCMAX - (-6))/6; //refer to spec

    RIV_reg_val += RIV;
    LIV_reg_val += LIV;
    RADV_reg_val += RADV;
    LADV_reg_val += LADV;
    RDAV_reg_val += RDAV;
    LDAV_reg_val += LDAV;
    SPV_reg_val += SPV;
    RIM_reg_val = RIM;
    LIM_reg_val = LIM;
    RHM_reg_val = RHM;
    LHM_reg_val = LHM;
    // start ADDA 0x00
    tmp = *(volatile unsigned int *)(base + 0x0);

    if (ISMASTER == 1)  // ADDA as master
        tmp |= (0x1 << 1);
    else                // ADDA as slave
        tmp &= ~(0x1 << 1);

    if (reback == 1)    // enable reback
        tmp |= (0x1 << 3);
    else
        tmp &= ~(0x1 << 3);

#if 1
    if (GM813X_STEREO_TYPE) {
        tmp &= ~(0x3 << 18);            // enable right/left ADC frontend power
        tmp &= ~(0x3 << 20);            // enable right/left ADC power
        tmp &= ~(0x3 << 22);            // enable right/left DAC power
    } else if (GM813X_MONO_TYPE) {
        tmp &= ~(0x1 << 19);            // enable left ADC frontend power
        tmp |= (0x1 << 18);             // power down right ADC frontend power
        tmp &= ~(0x1 << 21);            // enable left ADC power
        tmp |= (0x1 << 20);             // power down right ADC power
        tmp &= ~(0x1 << 22);            // enable right DAC power
        tmp |= (0x1 << 23);             // power down left DAC power
    }
#else
    if (chip_id == 0x8138 || chip_id == 0x8137) {  // 8138
        tmp &= ~(0x1 << 19);            // enable left ADC frontend power
        tmp |= (0x1 << 18);             // power down right ADC frontend power
        tmp &= ~(0x1 << 21);            // enable left ADC power
        tmp |= (0x1 << 20);             // power down right ADC power
        tmp &= ~(0x1 << 22);            // enable right DAC power
        tmp |= (0x1 << 23);             // power down left DAC power
    }
    if (chip_id == 0x8139) {  // 8139
        tmp &= ~(0x3 << 18);            // enable right/left ADC frontend power
        tmp &= ~(0x3 << 20);            // enable right/left ADC power
        tmp &= ~(0x3 << 22);            // enable right/left DAC power
    }
    if (chip_id == 0x8135 || chip_id == 0x8136) {
        tmp &= ~(0x1 << 18);            // enable ADC frontend power
        tmp &= ~(0x1 << 20);            // enable ADC power
        tmp &= ~(0x1 << 22);            // enable DAC power
    }
    // 8138 & 8138S has same chip_id, use bond_id to check.
    if (bond_id == 0xCF) {  // 8138S
        tmp &= ~(0x3 << 18);            // enable right/left ADC frontend power
        tmp &= ~(0x3 << 20);            // enable right/left ADC power
        tmp &= ~(0x3 << 22);            // enable right/left DAC power
    }
#endif

    tmp |= (0x01 << 28);            // set MIC power down(MIC bias power down)
    tmp &= ~(0x3 << 30);
    if (single_end)
        tmp |= (0x3 << 30);         // right/left mic mode as single-end input

    if (input_mode == 2)            // enable digital MIC
        tmp |= (0x1 << 16);

    *(volatile unsigned int *)(base + 0x0) = tmp;
    // end ADDA 0x00

    if (chip_id == 0x8135 || chip_id == 0x8136) {
        if (input_mode == 1)
            printk("[ADDA] Error !! 8135/8136 do not support line in mode\n");
    }
    // start ADDA 0x04
    tmp = *(volatile unsigned int *)(base + 0x4);
    if ((input_mode == 0) || (input_mode == 2)) {   // analog MIC or digital MIC
        tmp &= ~0x3;                                // select right/left MIC in
        tmp &= ~(0x3 << 2);                         // disable L/R boost, 0 dB added
        if (test_mode == 0)
            tmp |= (0x3 << 2);                      // enable L/R boost, 20 dB added
    } else if (input_mode == 1) {                   // line in
        if (chip_id == 0x8137 || chip_id == 0x8138 || chip_id == 0x8139) {
            tmp |= 0x3;                                 // select Line in
            tmp &= ~(0x3 << 2);                         // disable L/R boost, 0 dB added
        }
        if (chip_id == 0x8135 || chip_id == 0x8136) {   //8135/8136 no line in mode, set to MIC mode
            tmp &= ~0x3;                                // select right/left MIC in
            tmp &= ~(0x3 << 2);                         // disable L/R boost, 0 dB added
            if (test_mode == 0)
                tmp |= (0x3 << 2);                      // enable L/R boost, 20 dB added
        }
    }
    //set L/R input unmute
    tmp &= ~(0x3 << 4);
    tmp |= ((RIM_reg_val << 4) | (LIM_reg_val << 5));
    //clear RIV, LIV setting
    tmp &= ~(0x3F << 6);
    tmp &= ~(0x3F << 12);
    //set RIV, LIV based on user's setting
    tmp |= (RIV_reg_val << 6);
    tmp |= (LIV_reg_val << 12);
    //clear RADV, LADV setting
    tmp &= ~(0x7F << 18);
    tmp &= ~(0x7F << 25);
    //set RADV, LADV based on user's setting
    tmp |= (RADV_reg_val << 18);
    tmp |= (LADV_reg_val << 25);
    *(volatile unsigned int *)(base + 0x4) = tmp;
    // end ADDA 0x04
    // start ADDA 0x08
    tmp = *(volatile unsigned int *)(base + 0x8);
    tmp &= ~(0x3);                  // ADC gain 1.5db
    tmp |= 0x1;

    tmp &= ~(0x1 << 2);             // disable ADC DEM
    tmp |= (0x1 << 3);              // enable ADC DWA

    if (HPF==1)
        tmp|=(0x1<<4);

    if (enable_ALC)                 //enable ALC or not
        tmp |= (1 << 5);
    else
        tmp &= ~(1 << 5);
    // ALC noise gate threshold
    tmp &= ~(0x7 << 7);
    tmp |= (ALCNGTH_reg_val << 7);
    // ALC MAX / ALC MIN
    tmp &= ~(0x7 << 10);
    tmp &= ~(0x7 << 13);
    tmp |= (ALCMAX_reg_val << 10);
    tmp |= (ALCMIN_reg_val << 13);
    *(volatile unsigned int *)(base + 0x8) = tmp;
    // end ADDA 0x08
    // start ADDA 0x0c
    tmp = *(volatile unsigned int *)(base + 0xc);

    if (chip_id == 0x8137 || chip_id == 0x8138 || chip_id == 0x8139) {
        tmp &= ~(0x3F);                 // set RDAV, LDAV based on user's setting
        tmp &= ~(0x3F << 6);
        tmp |= RDAV_reg_val;
        tmp |= (LDAV_reg_val << 6);
    }
    if (chip_id == 0x8135 || chip_id == 0x8136) {
        tmp &= ~(0x7F);                 // set DAV
        tmp |= RDAV_reg_val;
    }
    tmp &= ~(0x3 << 12);
    tmp |= (0x01 << 12);            // SDMGAIN (-2db)
    tmp &= ~(0x3 << 16);
    tmp |= (RHM_reg_val << 16);
    tmp |= (LHM_reg_val << 17);
    if (chip_id == 0x8137 || chip_id == 0x8138 || chip_id == 0x8139) {
        if (DAMIXER == 1)
            tmp |= (0x01 << 22);            // enable DAMIXER
        else
            tmp &= ~(0x01 << 22);           // diable DAMIXER
        tmp &= ~(0x3 << 23);                // DAMIXER inverter as normal mode

        //clear SPV setting
        tmp &= ~(0x3F << 25);
        tmp |= (SPV_reg_val << 25);
    }
    if (chip_id == 0x8135 || chip_id == 0x8136) {
        tmp &= ~(0x1 << 7);                 // line out disable mute
        tmp &= ~(0x1 << 8);                 // enable LOUT
        //tmp &= ~(0x3 << 25);
        //tmp |= (SPV_reg_val << 25);
    }

    *(volatile unsigned int *)(base + 0xc) = tmp;
    // end ADDA 0x0c
    // start ADDA 0x10
    tmp = *(volatile unsigned int *)(base + 0x10);
    tmp &= ~(0x07 << 16);           // reset IRSEL setting
    tmp |= (0x04 << 16);            // set IRSEL 4 as normal, 2 as power saving mode
    // R/L SPKSEL
    if (chip_id == 0x8137 || chip_id == 0x8138 || chip_id == 0x8139) {
        tmp &= ~(0x03 << 12);           // reset BTL setting
        if (enable_BTL)
            tmp |= (0x02 << 12);        // enable BTL : speaker mode
        else
            tmp |= (0x03 << 12);        // disable BTL : headphone mode
    }
    if (chip_id == 0x8135 || chip_id == 0x8136) {
        tmp &= ~(0x01 << 5);           // HPFAPP
        tmp &= ~(0x01 << 6);           // HPFCUT2
        tmp &= ~(0x01 << 7);           // HPFCUT1
        tmp &= ~(0x01 << 8);           // HPFCUT0
        tmp &= ~(0x01 << 19);          // enable SCF in LOUT
    }

    tmp &= ~(0x03 << 28);           // set isolation on & digital power on
    *(volatile unsigned int *)(base + 0x10) = tmp;
    // end ADDA 0x010

    //enable SPEAKER power
    tmp = *(volatile unsigned int *)(base + 0x0);
    tmp &= ~(0x01 << 27);
    *(volatile unsigned int *)(base + 0x0) = tmp;
}
#endif

static void adda302_i2s_apply(void)
{
    u32 tmp = 0;
    u32 base = 0;

	base = priv.adda302_reg;

    //apply ADDA setting, reset it!!
	tmp = *(volatile unsigned int *)(base + 0x0);
	tmp |= 0x01;
    *(volatile unsigned int *)(base + 0x0) = tmp;
	mdelay(10);

    tmp &= ~0x1;
    *(volatile unsigned int *)(base + 0x0) = tmp;
}
#if 0
void adda302_speaker_powerdown(int on)
{
    u32 tmp = 0;
    u32 base = 0;

	base = priv.adda302_reg;

#ifndef SUPPORT_POWER_PD
    return; /* do nothing */
#endif

    tmp = *(volatile unsigned int *)(base + 0x0);

    if (on == 0) {        /* power down mode */
        tmp |= (0x1 << 27);
        printk("ADDA300: Turn Off the Speaker. \n");
    }
    else {
        tmp &= ~(0x1 << 27);
        printk("ADDA300: Turn On the Speaker. \n");
    }

    *(volatile unsigned int *)(base + 0x0) = tmp;

}
#endif
/* when tx & rx at same i2s, we have to set ad/da together */
void adda302_set_fs(const enum adda302_fs_frequency fs)
{
	u32 tmp = 0;
    u32 base = priv.adda302_reg;

	tmp = *(volatile unsigned int *)(base + 0x0);

	tmp &= ~(0xFF << 4);

	switch(fs){
		case ADDA302_FS_8K:
#if 0
            if (input_mode == 2) {   //dmic
                tmp |= (0x03 << 4);
    			tmp |= (0x03 << 8);
            } else {
                /* original code */
    			tmp |= (0x04 << 4);
    			tmp |= (0x04 << 8);
    		}
#else
            tmp |= (0x04 << 4);
            tmp |= (0x04 << 8);
#endif
			break;
		case ADDA302_FS_16K:
#if 0
            if (input_mode == 2) {   //dmic
                tmp |= (0x02 << 4);
    			tmp |= (0x02 << 8);
            } else {
                /* orginal code */
    			tmp |= (0x04 << 4);
    			tmp |= (0x04 << 8);
    		}
#else
            tmp |= (0x01 << 4);
            tmp |= (0x01 << 8);
#endif
			break;
		case ADDA302_FS_22_05K:
			tmp |= (0x08 << 4);
			tmp |= (0x08 << 8);
			break;
		case ADDA302_FS_32K:
            adda302_mclk_init(16000000);
            tmp |= (0x05 << 4);
			tmp |= (0x05 << 8);
			break;
		case ADDA302_FS_44_1K:
			tmp |= (0x0d << 4);
			tmp |= (0x0d << 8);
			break;
		case ADDA302_FS_48K:
			tmp |= (0x06 << 4);
			tmp |= (0x06 << 8);
			break;
		default:
			DEBUG_ADDA302_PRINT("%s fails: fs wrong value\n", __FUNCTION__);
			return;
	}

    *(volatile unsigned int *)(base + 0x0) = tmp;

	adda302_i2s_apply();

#if DEBUG_ADDA302
	DEBUG_ADDA302_PRINT("In %s", __FUNCTION__);
	dump_adda302_reg();
#endif
}
// loopback function is DAC to ADC
void adda302_set_loopback(int on)
{
    u32 tmp = 0;
    u32 base = priv.adda302_reg;

    tmp = *(volatile unsigned int *)(base + 0x0);

    if (on == 1)
        tmp |= (0x1 << 3);            // enable loopback

    if (on == 0)
        tmp &= ~(0x1 << 3);             // disable loopback

    *(volatile unsigned int *)(base + 0x0) = tmp;
}
EXPORT_SYMBOL(adda302_set_loopback);

/* set AD power down */
void adda302_set_ad_powerdown(int off)
{
    u32 tmp = 0;
    u32 base = priv.adda302_reg;

    tmp = *(volatile unsigned int *)(base + 0x0);
    /* power on ad */
    if (off == 0) {
#if 1
        if (GM813X_STEREO_TYPE) {
            tmp &= ~(0x3 << 18);            // enable right/left ADC frontend power
            tmp &= ~(0x3 << 20);            // enable right/left ADC power
        } else if (GM813X_MONO_TYPE) {
            tmp &= ~(0x1 << 19);            // enable left ADC frontend power
            tmp &= ~(0x1 << 21);            // enable left ADC power
        }
#else
        if (chip_id == 0x8138 || chip_id == 0x8137) {
            tmp &= ~(0x1 << 19);            // enable left ADC frontend power
            tmp &= ~(0x1 << 21);            // enable left ADC power
        }
        if (chip_id == 0x8139) {
            tmp &= ~(0x3 << 18);            // enable right/left ADC frontend power
            tmp &= ~(0x3 << 20);            // enable right/left ADC power
        }
        if (chip_id == 0x8135 || chip_id == 0x8136) {
            tmp &= ~(0x1 << 18);            // enable ADC frontend power
            tmp &= ~(0x1 << 20);            // enable ADC power
        }
#endif
    }
    /* power down ad */
    if (off == 1) {
#if 1
        if (GM813X_STEREO_TYPE) {
            tmp |= (0x3 << 18);             // power down right/left ADC frontend power
            tmp |= (0x3 << 20);             // power down right/left ADC power
        } else if (GM813X_MONO_TYPE) {
            tmp |= (0x1 << 19);             // power down left ADC frontend power
            tmp |= (0x1 << 21);             // power down left ADC power
        }
#else
        if (chip_id == 0x8138 || chip_id == 0x8137) {
            tmp |= (0x1 << 19);             // power down left ADC frontend power
            tmp |= (0x1 << 21);             // power down left ADC power
        }
        if (chip_id == 0x8139) {
            tmp |= (0x3 << 18);             // power down right/left ADC frontend power
            tmp |= (0x3 << 20);             // power down right/left ADC power
        }
        if (chip_id == 0x8135 || chip_id == 0x8136) {
            tmp |= (0x1 << 18);            // power down ADC frontend power
            tmp |= (0x1 << 20);            // power down ADC power
        }
#endif        
    }

    *(volatile unsigned int *)(base + 0x0) = tmp;
}

/* set DA power down */
void adda302_set_da_powerdown(int off)
{
    u32 tmp = 0;
    u32 base = priv.adda302_reg;

    tmp = *(volatile unsigned int *)(base + 0x0);
    /* power on da */
    if (off == 0) {
#if 1
        if (GM813X_STEREO_TYPE) {
            tmp &= ~(0x3 << 22);             // enable right/left DAC power
        } else if (GM813X_MONO_TYPE) {
            tmp &= ~(0x1 << 22);             // enable right DAC power
        }
#else
        if (chip_id == 0x8138 || chip_id == 0x8137)
            tmp &= ~(0x1 << 22);             // enable right DAC power

        if (chip_id == 0x8139)
            tmp &= ~(0x3 << 22);             // enable right/left DAC power

        if (chip_id == 0x8135 || chip_id == 0x8136)
            tmp &= ~(0x1 << 22);             // enable DAC power
#endif
    }
    /* power down da */
    if (off == 1) {
#if 1
        if (GM813X_STEREO_TYPE) {
            tmp |= (0x3 << 22);             // power down right/left DAC power
        } else if (GM813X_MONO_TYPE) {
            tmp |= (0x1 << 22);             // power down right DAC power
        }
#else
        if (chip_id == 0x8138 || chip_id == 0x8137)
            tmp |= (0x1 << 22);             // power down right DAC power

        if (chip_id == 0x8139)
            tmp |= (0x3 << 22);             // power down right/left DAC power

        if (chip_id == 0x8135 || chip_id == 0x8136)
            tmp |= ~(0x1 << 22);            // power down DAC power
#endif
    }

    *(volatile unsigned int *)(base + 0x0) = tmp;
}

/* when tx & rx not at same i2s, we have to set ad/da frame sync separately,
   because audio mclk rx/tx should be 12MHz,
   here we set audio ad mclk mode ratio base on mclk 12MHz,
*/
void adda302_set_ad_fs(const enum adda302_fs_frequency fs)
{
	u32 tmp = 0;
	u32 base = priv.adda302_reg;

	tmp = *(volatile unsigned int *)(base + 0x0);

	tmp &= ~(0xF << 4);

	switch(fs){
		case ADDA302_FS_8K:
#if 0
            if (input_mode == 2) {   //dmic
                tmp |= (0x03 << 4);
            } else {
                /* orginal code */
    			tmp |= (0x04 << 4);
    		}
#else
            tmp |= (0x04 << 4);
#endif
			break;
		case ADDA302_FS_16K:
#if 0
            if (input_mode == 2) {   //dmic
                tmp |= (0x02 << 4);
            } else {
                /* orginal code */
    			tmp |= (0x04 << 4);
    		}
#else
            tmp |= (0x01 << 4);
#endif
			break;
		case ADDA302_FS_22_05K:
			tmp |= (0x08 << 4);
			break;
		case ADDA302_FS_32K:
            adda302_mclk_init(16000000);
            tmp |= (0x5 << 4);
			break;
		case ADDA302_FS_44_1K:
    		tmp |= (0x0d << 4);
			break;
		case ADDA302_FS_48K:
            tmp |= (0x06 << 4);
			break;
		default:
			DEBUG_ADDA302_PRINT("%s fails: fs wrong value\n", __FUNCTION__);
			return;
	}

	*(volatile unsigned int *)(base + 0x0) = tmp;

	adda302_i2s_apply();

#if DEBUG_ADDA302
	DEBUG_ADDA302_PRINT("In %s", __FUNCTION__);
	dump_adda302_reg();
#endif
}

/* when tx & rx not at same i2s, we have to set ad/da frame sync separately,
   because audio mclk rx/tx should be 12MHz,
   here we set audio da mclk mode ratio base on mclk 12MHz
*/
void adda302_set_da_fs(const enum adda302_fs_frequency fs)
{
	u32 tmp = 0;
	u32 base = priv.adda302_reg;

	tmp = *(volatile unsigned int *)(base + 0x0);

	tmp &= ~(0xF << 8);

	switch(fs){
		case ADDA302_FS_8K:
#if 0
            if (input_mode == 2) {   //dmic
    			tmp |= (0x03 << 8);
            } else {
                /* orginal code */
    			if (mclk == 12000000)
    			    tmp |= (0x04 << 8);
    			if (mclk == 24000000)
    			    printk("not support 8k sample rate under mclk 24Mhz\n");
    		}
#else
            tmp |= (0x04 << 8);
#endif
			break;
		case ADDA302_FS_16K:
#if 0
            if (input_mode == 2) {   //dmic
    			tmp |= (0x02 << 8);
            } else {
                /* orginal code */
    			if (mclk == 12000000)
    			    tmp |= (0x01 << 8);
    			if (mclk == 24000000)
    			    tmp |= (0x04 << 8);
    		}
#else
            tmp |= (0x01 << 8);
#endif
			break;
		case ADDA302_FS_22_05K:
			tmp |= (0x08 << 8);
			break;
		case ADDA302_FS_32K:
            adda302_mclk_init(16000000);
			tmp |= (0x05 << 8);
			break;
		case ADDA302_FS_44_1K:
			tmp |= (0x0d << 8);
			break;
		case ADDA302_FS_48K:
			tmp |= (0x06 << 8);
			break;
		default:
			DEBUG_ADDA302_PRINT("%s fails: fs wrong value\n", __FUNCTION__);
			return;
	}

    *(volatile unsigned int *)(base + 0x0) = tmp;

	adda302_i2s_apply();

#if DEBUG_ADDA302
	DEBUG_ADDA302_PRINT("In %s", __FUNCTION__);
	dump_adda302_reg();
#endif
}

static void adda302_i2s_ctrl_init(void)
{
	u32 tmp = 0;
	u32 base = 0;

	base = priv.adda302_reg;

	tmp = *(volatile unsigned int *)(base + 0x0);
    tmp |= (0x04 << 8);     // default MCLK DA mode set to serve 8K sampling rate, you should change this when changing sampling rate
	tmp |= (0x01 << 14);    // I2S mode
	tmp |= (0x04 << 4);     // default MCLK AD mode set to serve 8K sampling rate, you should change this when changing sampling rate
    if (ISMASTER == 1)
	    tmp |= (0x01 << 1);     // set ADDA Master Mode
    else
	    tmp &= ~(0x01 << 1);    // set ADAA Slave Mode

    *(volatile unsigned int *)(base + 0x0) = tmp;
}

static void adda302_i2s_init(void)
{
    adda302_i2s_ctrl_init();
    adda302_i2s_reset();
    adda302_i2s_apply();
    //adda302_i2s_powerdown(0);
}

static void adda302_get_chip_id(void)
{
    u32 temp;

    temp = ftpmu010_read_reg(0x00);

    bond_id = (temp & 0xf) | ((temp >> 4) & 0xf0);
    chip_id = (ftpmu010_get_attr(ATTR_TYPE_CHIPVER) >> 16) & 0xFFFF;

    if (chip_id != 0x8139 && chip_id != 0x8138 && chip_id != 0x8137 && chip_id != 0x8135 && chip_id != 0x8136)
        panic("ADDA302 can not recognize this chip_id %x\n", chip_id);

    //printk("bond_id(%x), chip_id(%x)\n", bond_id, chip_id);
}

static int __init adda302_drv_init(void)
{
    memset(&priv, 0x0, sizeof(adda302_priv_t));

    /* init the clock and add the device .... */
    platform_init();

    /* Register the driver */
	if (platform_driver_register(&adda302_driver)) {
		printk("Failed to register ADDA driver\n");
		return -ENODEV;
	}

    adda302_get_chip_id();
    // set ADDA Main clock : 12MHz
    adda302_mclk_init(12000000);
    // set SSP Main clock : 24MHz
    adda302_ssp_mclk_init();
    // select da source from ADDA or decoder
    adda302_da_select();

    // Change PMU setting
    if (input_mode==2)
        ADDA302_PMU_Select_DMIC_Input(&DMIC_Reg[ADDA302_DMIC_DEF_PMU]);

    adda302_i2s_init();

    adda302_proc_init();

    return 0;
}

static void __exit adda302_drv_clearnup(void)
{
    adda302_proc_remove();

    platform_driver_unregister(&adda302_driver);


    platform_exit();

    __iounmap((void *)priv.adda302_reg);
}

ADDA302_SSP_USAGE
ADDA302_Check_SSP_Current_Usage(void)
{
    //return (same_i2s_tx_rx==1)?(ADDA302_SSP0RX_SSP0TX):(ADDA203_SSP0RX_SSP1TX);
    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 0))  // adda da source select SSP0
        return ADDA302_SSP0RX_SSP0TX;
    if ((audio_out_enable[0] == 0) && (audio_out_enable[1] == 1))  // adda da source select SSP1
        return ADDA203_SSP0RX_SSP1TX;
    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 1))  // adda da source select SSP0, SSP1 for decoder
        return ADDA302_SSP0RX_SSP0TX;

    return 0;
}
EXPORT_SYMBOL(ADDA302_Check_SSP_Current_Usage);

static void
ADDA302_PMU_Select_DMIC_Input(pmuReg_t *reg)
{
    if (!reg) {
        panic("%s(%d) NULL reg!\n",__FUNCTION__,__LINE__);
        return;
    }

    if (ftpmu010_write_reg(priv.adda302_fd, reg->reg_off, reg->init_val, reg->bits_mask) < 0) {
        panic("%s(%d) setting offset(0x%x) failed.\n",__FUNCTION__,__LINE__,reg->reg_off);
        return;
    }

    printk("%s ok!\n",__FUNCTION__);
}

EXPORT_SYMBOL(adda302_set_ad_fs);
EXPORT_SYMBOL(adda302_set_da_fs);
EXPORT_SYMBOL(adda302_set_fs);
EXPORT_SYMBOL(adda302_set_ad_powerdown);
EXPORT_SYMBOL(adda302_set_da_powerdown);
EXPORT_SYMBOL(ADDA302_Set_DA_Source);

module_init(adda302_drv_init);
module_exit(adda302_drv_clearnup);
MODULE_LICENSE("GPL");
