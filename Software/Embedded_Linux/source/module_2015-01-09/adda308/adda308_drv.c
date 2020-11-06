#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <mach/platform-GM8136/platform_io.h>
#include <mach/ftpmu010.h>
#include <linux/slab.h>
#include <linux/times.h>
#include <linux/synclink.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/delay.h>

#include "adda308.h"
#include <adda308_api.h>
#include <platform.h>

#define SUPPORT_POWER_PD
#define VERSION     141229 //2014_1229
adda308_priv_t priv;

extern int adda308_proc_init(void);
extern void adda308_proc_remove(void);

static u32 chip_id = 0;
static u32 bond_id = 0;

typedef enum {
    DMIC_PMU_64H,
    DMIC_PMU_50H
} adda308_DMIC_PMU_REG;

#define adda308_DMIC_DEF_PMU  DMIC_PMU_50H

static pmuReg_t DMIC_Reg[] = {
    /* reg_off, bit_masks, lock_bits, init_val, init_mask */
    {  0x64,    0xf<<12,   0x0,       0xa<<12,  0xf<<12}, // Select PMU for DMIC(Mode2) 0x64 bit 12,13,14,15
    {  0x50,    0xf<<6,    0x0,       0xf<<6,   0xf<<6}   // Select PMU for DMIC(Mode2) 0x50 bit 6,7,8,9,
};

static void adda308_PMU_Select_DMIC_Input(pmuReg_t *reg);

#define SSP_MAX_USED    2
//input_mode: 0 -> analog MIC, 2 -> digital MIC
uint input_mode = 0;
module_param(input_mode, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(input_mode, "input mode can be analog MIC/line in/digital MIC");
//output_mode: 0 -> spk out, 1 -> line out
uint output_mode = 0;
module_param(output_mode, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(output_mode, "output mode can be speaker out/line out");
//enable_dmic: 0 -> disable, 1 -> enable,
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
//L channel input analog Gain, from -27 ~ +36 dB
int LIV = 15;
module_param(LIV, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(LIV, "L channel input analog gain");
//L channel input mute, 0 -> unmute, 1 -> mute
int LIM = 0;
module_param(LIM, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(LIM, "L channel input mute");
//L channel input digital Gain, -50 ~ 30 dB
int LADV = 0;
module_param(LADV, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(LADV, "L channel input digital gain");
//L channel output digital Gain, -40 ~ 0 dB
int LDAV = 0;
module_param(LDAV, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(LDAV, "L channel output digital gain");
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
// used to control ADDA codec's power
int power_control = 1;
module_param(power_control, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(power_control, "ADDA power control");

// ADDA clock source from PLL2
int clk_pll2 = 0;
int reback = 0;

extern int platform_init(void);
extern int platform_exit(void);

#if DEBUG_ADDA308
void dump_adda308_reg(void)
{
	u32 tmp = 0;
	u32 base = priv.adda308_reg;
	DEBUG_ADDA308_PRINT("\n0x28 = %x\n", ftpmu010_read_reg(0x28));
	DEBUG_ADDA308_PRINT("0x74 = %x\n", ftpmu010_read_reg(0x74));
	DEBUG_ADDA308_PRINT("0x7C = %x\n", ftpmu010_read_reg(0x7C));
	DEBUG_ADDA308_PRINT("0xb8 = %x\n", ftpmu010_read_reg(0xb8));
	DEBUG_ADDA308_PRINT("0xac = %x\n", ftpmu010_read_reg(0xac));
	/* adda wrapper */
	printk("ADDA Wrapper\n");
	tmp = *(volatile unsigned int *)(base + 0x0);
	DEBUG_ADDA308_PRINT("0x00 = %x\n", tmp);
	tmp = *(volatile unsigned int *)(base + 0x4);
	DEBUG_ADDA308_PRINT("0x04 = %x\n", tmp);
	tmp = *(volatile unsigned int *)(base + 0x8);
	DEBUG_ADDA308_PRINT("0x08 = %x\n", tmp);
	tmp = *(volatile unsigned int *)(base + 0xc);
	DEBUG_ADDA308_PRINT("0x0c = %x\n", tmp);
	tmp = *(volatile unsigned int *)(base + 0x10);
	DEBUG_ADDA308_PRINT("0x10 = %x\n", tmp);
}
#endif//end of DEBUG_ADDA300

/*
 * Create engine and FIFO
 */
int add_engine(unsigned long addr)
{
    priv.adda308_reg = addr;

    return 0;
}

/* Note: adda308_init_one() will be run before adda308_drv_init() */
static int adda308_init_one(struct platform_device *pdev)
{
	struct resource *mem;
	unsigned long   vaddr;
    printk("adda308_init_one [%d] version %d\n", pdev->id, VERSION);

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

static int adda308_remove_one(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver adda308_driver = {
    .probe	= adda308_init_one,
    .remove	=  __devexit_p(adda308_remove_one),
	.driver = {
	        .name = "adda308",
	        .owner = THIS_MODULE,
	    },
};

/*
 * ADDA main clock is 24MHz(line out) or 24.5454MHz(speaker out)
 * at GM8136
 * speaker out mode:
 * ssp source from PLL1/cntp3 (810/3/11MHz)
 * when ADDA is master, SSP is slave, SSP main clock is 24.5454MHz, meet sample rate:8/16/32/48k
 --------------------------------------------------------------------------------------------------
 * line out mode
 * ssp source from PLL2 (600/25MHz)
 * when ADDA is master, SSP is slave, SSP main clock is 24(600/25)MHz, meet sample rate:8/16/22.05/32/44.1/48k
 */
void adda308_ssp_mclk_init(void)
{
    int ret = 0;

    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 0)) { // adda da source select SSP0
        if (ISMASTER == 1) {
            if (output_mode == 0)
                ret = plat_set_ssp_clk(0, 24545454, clk_pll2);
            else
                ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
        }
        else
            ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
        if (ret < 0)
            printk("ADDA set ssp0 main clock fail\n");
    }

    if ((audio_out_enable[0] == 0) && (audio_out_enable[1] == 1)) {  // adda da source select SSP1
        // adda ad for record, da for playback
        // set SSP0
        if (ISMASTER == 1) {
            if (output_mode == 0)
                ret = plat_set_ssp_clk(0, 24545454, clk_pll2);
            else
                ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
        }
        else
            ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
        if (ret < 0)
            printk("ADDA set ssp0 main clock fail\n");
        // set SSP1
        if (ISMASTER == 1) {
            if (output_mode == 0)
                ret = plat_set_ssp_clk(1, 24545454, clk_pll2);
            else
                ret = plat_set_ssp_clk(1, 24000000, clk_pll2);
        }
        else
            ret = plat_set_ssp_clk(1, 24000000, clk_pll2);
        if (ret < 0)
            printk("ADDA set ssp1 main clock fail\n");
    }

    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 1)) { // adda da source select SSP0, SSP1 for decoder
        if (ISMASTER == 1) {
            if (output_mode == 0)
                ret = plat_set_ssp_clk(0, 24545454, clk_pll2);
            else
                ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
        }
        else
            ret = plat_set_ssp_clk(0, 24000000, clk_pll2);
        if (ret < 0)
            printk("ADDA set ssp0 main clock fail\n");
    }
}

void adda308_mclk_init(u32 hz)
{
	volatile u32 tmp = 0;
    volatile u32 mask = 0;
    volatile u32 pvalue = 0;
	unsigned long div = 0;
	unsigned long curr_div = 0;
	volatile u32 cntp3 = 0;

	pvalue = ftpmu010_read_reg(0x74);
	curr_div = (pvalue & 0x3f000000) >> 24;

    if (output_mode == 0)
        cntp3 = 2;

	//give adda308 24.5454 or 24MHz main clock
    tmp = 0;
    mask = (BIT24|BIT25|BIT26|BIT27|BIT28|BIT29);

    if (clk_pll2 == 0)      // clock source from pll1 cntp3
        div = PLL1_CLK_IN / (cntp3 + 1);
    if (clk_pll2 == 1)      // clock source from pll2
        div = PLL2_CLK_IN;
    if (clk_pll2 == 2)      // clock source from pll3
        div = PLL3_CLK_IN;

    div /= hz;
    div -= 1;

    // set clock divided value
    if (div != curr_div) {
        tmp |= (div << 24);
        DEBUG_ADDA308_PRINT("adda308 gets divisor %ld, PPL3_CLK = %d, mclk = %lu \n",
                div, PLL3_CLK_IN, PLL3_CLK_IN/(div+1));

        if (ftpmu010_write_reg(priv.adda308_fd, 0x74, tmp, mask) < 0) {
            panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x74);
        }
    }

    // set clock source, adda default clock source from pll3, set from pll2
    mask = 0;
    tmp = 0;

    mask = BIT15 | BIT16;
    if (clk_pll2 == 0)
        tmp = 1 << 15;
    if (clk_pll2 == 1)
        tmp = 2 << 15;
    if (clk_pll2 == 2)
        tmp = 0 << 15;

    if (ftpmu010_write_reg(priv.adda308_fd, 0x28, tmp, mask) < 0) {
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x28);
    }
}

void ADDA308_Set_DA_Source(int da_src)
{
    volatile u32 val = 0;
    volatile u32 mask = BIT27;

    //printk("%s, da_src(%d)\n", __func__, da_src);

    if (da_src >= DA_SRC_MAX) {
        printk("error DA source: %d\n", da_src);
        return;
    }

    val = (da_src == DA_SRC_SSP0)?(val & ~(1<<27)):(val | (1<<27));

    if (ftpmu010_write_reg(priv.adda308_fd, 0x7C, val, mask) < 0)
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x7C);
}

/*
 * set adda308 DA source select
 */
static void adda308_da_select(void)
{
	int ret = 0;
	volatile u32 tmp = 0;
    volatile u32 mask = 0;

    tmp = 0;
    mask = BIT27;

#if 0
    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 0))  // adda da source select SSP0
        tmp &= ~(1 << 27);
    if ((audio_out_enable[0] == 0) && (audio_out_enable[1] == 1))  // adda da source select SSP1
        tmp |= (1 << 27);
    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 1))  // adda da source select SSP0, SSP1 for decoder
        tmp &= ~(1 << 27);
#else
    tmp &= ~(1 << 27);
#endif

    if (ftpmu010_write_reg(priv.adda308_fd, 0x7C, tmp, mask) < 0)
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

void adda308_i2s_reset(void)
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

    base = priv.adda308_reg;

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

    if ((LDAV < -50) || (LDAV > 30)) {
        printk("%s: DAV(%ddB) MUST between -50dB ~ 30dB, use default(-1dB)\n", __func__, LDAV);
        LDAV = -1;
    }

    if ((SPV < 0) || (SPV > 3)) {
        printk("%s: SPV(%ddB) MUST between 0 ~ 3, use default(0dB)\n", __func__, SPV);
        SPV = 0;
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

    // prevent the background noise when playing silence, 2014/12/29
    tmp &= ~(0x1 << 2);

    if (reback == 1)    // enable reback
        tmp |= (0x1 << 3);
    else
        tmp &= ~(0x1 << 3);

    tmp &= ~(0x3 << 14);
    tmp |= (0x1 << 14);             // audio interface is i2s serial data mode

    tmp &= ~(0x1 << 18);            // enable ADC frontend power
    tmp &= ~(0x1 << 20);            // enable ADC power
    tmp &= ~(0x1 << 22);            // enable DAC power
    tmp &= ~(0x1 << 28);            // enable MIC bias power
    tmp |= (0x1 << 30);             // set mic mode as single-end input

    tmp &= ~(0x1 << 16);
    if (input_mode == 2)            // enable digital MIC
        tmp |= (0x1 << 16);

    *(volatile unsigned int *)(base + 0x0) = tmp;

    // ===== end ADDA 00h =====

    if (input_mode == 1)
        printk("[ADDA] Error !! 8135/8136 do not support line in mode\n");

    // ===== start ADDA 04h =====
	tmp = *(volatile unsigned int *)(base + 0x4);

    tmp &= ~0x3;
    tmp &= ~(0x3 << 2);            // disable boost, 0 dB added
    if (test_mode == 0 && input_mode == 0)      // when input is MIC mode
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


    if (HPF == 1)                   // enable HPE or not
        tmp |= (0x1<<4);
    else
        tmp &= ~(0x1<<4);

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

    tmp &= ~(0x7F);                 // set DAV
    tmp |= DAV_reg_val;

    tmp &= ~(0x1 << 7);             // line out un-mute

    if (output_mode == 0)   //speaker out
        tmp |= (0x1 << 8);          // line out power down
    else
        tmp &= ~(0x1 << 8);         // enable line out power 

    tmp &= ~(0x3 << 9);             // speaker gain
    tmp |= (0x1 << 9);

    tmp &= ~(0x3 << 12);
    tmp |= (0x1 << 12);            // SDMGAIN (-2db)

    tmp |= (0x1 << 14);             // enable DAC modulator

    tmp &= ~(0x1 << 16);
    tmp |= (HM_reg_val << 16);

    tmp &= ~(0x3 << 25);            // speaker out power compensation
    tmp |= (SPV_reg_val << 25);

    *(volatile unsigned int *)(base + 0xc) = tmp;
    // ===== end ADDA 0Ch =====

    // ===== start ADDA 10h =====
	tmp = *(volatile unsigned int *)(base + 0x10);

    // clear HYS
    tmp &= ~(0x3);
    if (output_mode == 0)
        tmp |= 0x1;                 // audio_hys1

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
    if (output_mode == 0)
        tmp |= (0x01 << 19);          // power down SCF in LOUT
    else
        tmp &= ~(0x01 << 19);          // enable SCF in LOUT

    // IRSEL SPK
    tmp &= ~(0x7 << 24);
    tmp |= (0x04 << 24);

    tmp &= ~(0x01 << 28);
    if (power_control == 0) // power down
        tmp |= (0x01 << 28);

    *(volatile unsigned int *)(base + 0x10) = tmp;
    // ===== end ADDA 10h =====

    //enable SPEAKER power
    tmp = *(volatile unsigned int *)(base + 0x0);
    if (output_mode == 0)
        tmp &= ~(0x01 << 27);
    else
        tmp |= (0x01 << 27);
    *(volatile unsigned int *)(base + 0x0) = tmp;
}

static void adda308_i2s_apply(void)
{
    u32 tmp = 0;
    u32 base = 0;

	base = priv.adda308_reg;

    //apply ADDA setting, reset it!!
	tmp = *(volatile unsigned int *)(base + 0x0);
	tmp |= 0x01;
    *(volatile unsigned int *)(base + 0x0) = tmp;

    /* Note: ADDA308's IP designer has adiviced that the delay time
             should be 100ms to avoid some issue, but there will be 
             a short noise during 100 ms delay. Currently change back
             10 ms. 2014/10/09
    */
	mdelay(10);

    tmp &= ~0x1;
    *(volatile unsigned int *)(base + 0x0) = tmp;
}

/* when tx & rx at same i2s, we have to set ad/da together */
void adda302_set_fs(const enum adda308_fs_frequency fs)
{
	u32 tmp = 0;
    u32 base = priv.adda308_reg;

	tmp = *(volatile unsigned int *)(base + 0x0);

	tmp &= ~(0xFF << 4);

	switch(fs){
		case ADDA302_FS_8K:
            if (output_mode == 0) {
                tmp |= (0x03 << 4); // 1536
                tmp |= (0x03 << 8); // 1536
            }
            else {
                tmp |= (0x04 << 4); // 1500
                tmp |= (0x04 << 8); // 1500
            }
			break;
		case ADDA302_FS_16K:
            if (output_mode == 0) {
                tmp |= (0x02 << 4); //768
                tmp |= (0x02 << 8); //768
            }
            else {
                tmp |= (0x01 << 4); //750
                tmp |= (0x01 << 8); //750
            }
			break;
		case ADDA302_FS_22_05K:
			tmp |= (0x08 << 4); //544
			tmp |= (0x08 << 8); //544
			break;
		case ADDA302_FS_32K:
            tmp |= (0x07 << 4); //384
			tmp |= (0x07 << 8); //384
			break;
		case ADDA302_FS_44_1K:
			tmp |= (0x0d << 4); //272
			tmp |= (0x0d << 8); //272
			break;
		case ADDA302_FS_48K:
			if (output_mode == 0) {
    			tmp |= (0x0f << 4); //256
    			tmp |= (0x0f << 8); //256
    		}
    		else {
    		    tmp |= (0x06 << 4); //250
    			tmp |= (0x06 << 8); //250
    		}
			break;
		default:
			DEBUG_ADDA308_PRINT("%s fails: fs wrong value\n", __FUNCTION__);
			return;
	}

    *(volatile unsigned int *)(base + 0x0) = tmp;

    //adda308_i2s_apply();

#if DEBUG_ADDA308
	DEBUG_ADDA308_PRINT("In %s", __FUNCTION__);
	dump_adda308_reg();
#endif
}
// loopback function is DAC to ADC
void adda302_set_loopback(int on)
{
    u32 tmp = 0;
    u32 base = priv.adda308_reg;

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
    u32 base = priv.adda308_reg;

    tmp = *(volatile unsigned int *)(base + 0x0);
    /* power on ad */
    if (off == 0) {
        tmp &= ~(0x1 << 18);            // enable ADC frontend power
        tmp &= ~(0x1 << 20);            // enable ADC power
    }
    /* power down ad */
    if (off == 1) {
        tmp |= (0x1 << 18);            // power down ADC frontend power
        tmp |= (0x1 << 20);            // power down ADC power
    }

    *(volatile unsigned int *)(base + 0x0) = tmp;
}

/* set DA power down */
void adda302_set_da_powerdown(int off)
{
    u32 tmp = 0;
    u32 base = priv.adda308_reg;

    tmp = *(volatile unsigned int *)(base + 0x0);
    /* power on da */
    if (off == 0) {
        tmp &= ~(0x1 << 22);             // enable DAC power
    }
    /* power down da */
    if (off == 1) {
        tmp |= ~(0x1 << 22);            // power down DAC power
    }

    *(volatile unsigned int *)(base + 0x0) = tmp;
}

/* when tx & rx not at same i2s, we have to set ad/da frame sync separately,
   because audio mclk rx/tx should be 12MHz,
   here we set audio ad mclk mode ratio base on mclk 12MHz,
*/
void adda302_set_ad_fs(const enum adda308_fs_frequency fs)
{
	u32 tmp = 0;
	u32 base = priv.adda308_reg;

	tmp = *(volatile unsigned int *)(base + 0x0);

	tmp &= ~(0xF << 4);

	switch(fs){
		case ADDA302_FS_8K:
            if (output_mode == 0)
                tmp |= (0x03 << 4); //1536
            else
                tmp |= (0x04 << 4); //1500
			break;
		case ADDA302_FS_16K:
            if (output_mode == 0)
                tmp |= (0x02 << 4); //768
            else
                tmp |= (0x01 << 4); //750
			break;
		case ADDA302_FS_22_05K:
			tmp |= (0x08 << 4); //544
			break;
		case ADDA302_FS_32K:
            tmp |= (0x7 << 4);  //384
			break;
		case ADDA302_FS_44_1K:
    		tmp |= (0x0d << 4); //272
			break;
		case ADDA302_FS_48K:
            if (output_mode == 0)
                tmp |= (0x0f << 4); //256
            else
                tmp |= (0x06 << 4); //250
			break;
		default:
			DEBUG_ADDA308_PRINT("%s fails: fs wrong value\n", __FUNCTION__);
			return;
	}

	*(volatile unsigned int *)(base + 0x0) = tmp;

	adda308_i2s_apply();

#if DEBUG_ADDA308
	DEBUG_ADDA308_PRINT("In %s", __FUNCTION__);
	dump_adda308_reg();
#endif
}

/* when tx & rx not at same i2s, we have to set ad/da frame sync separately,
   because audio mclk rx/tx should be 12MHz,
   here we set audio da mclk mode ratio base on mclk 12MHz
*/
void adda302_set_da_fs(const enum adda308_fs_frequency fs)
{
	u32 tmp = 0;
	u32 base = priv.adda308_reg;

	tmp = *(volatile unsigned int *)(base + 0x0);

	tmp &= ~(0xF << 8);

	switch(fs){
		case ADDA302_FS_8K:
            if (output_mode == 0)
                tmp |= (0x03 << 8); //1536
            else
                tmp |= (0x04 << 8); //1500
			break;
		case ADDA302_FS_16K:
            if (output_mode == 0)
                tmp |= (0x02 << 8); //768
            else
                tmp |= (0x01 << 8); //750
			break;
		case ADDA302_FS_22_05K:
			tmp |= (0x08 << 8); //544
			break;
		case ADDA302_FS_32K:
			tmp |= (0x07 << 8); //384
			break;
		case ADDA302_FS_44_1K:
			tmp |= (0x0d << 8); //272
			break;
		case ADDA302_FS_48K:
			if (output_mode == 0)
			    tmp |= (0x0f << 8); //256
			else
			    tmp |= (0x06 << 8); //250
			break;
		default:
			DEBUG_ADDA308_PRINT("%s fails: fs wrong value\n", __FUNCTION__);
			return;
	}

    *(volatile unsigned int *)(base + 0x0) = tmp;

	adda308_i2s_apply();

#if DEBUG_ADDA308
	DEBUG_ADDA308_PRINT("In %s", __FUNCTION__);
	dump_adda308_reg();
#endif
}
/* adda308 initial sequence
 * 1. enable iso_enable & psw_pd
   2. reset
   3. set adda mclk
   4. set default ad/da mclk mode
   5. set adda as slave
   6. reset
*/
static void adda308_i2s_ctrl_init(void)
{
	u32 tmp = 0;
	u32 base = 0;

	base = priv.adda308_reg;
    //1. set psw_pd = 1, iso_enable = 0
    tmp = *(volatile unsigned int *)(base + 0x10);
    tmp |= (0x1 << 29);
    tmp &= ~(1 << 28);
    *(volatile unsigned int *)(base + 0x10) = tmp;
    //2. reset
    adda308_i2s_apply();

    //3. set adda mclk
    if (output_mode == 0)
        adda308_mclk_init(24545454);
    else
        adda308_mclk_init(24000000);
    //4. set default ad/da mclk mode
	tmp = *(volatile unsigned int *)(base + 0x0);
    tmp |= (0x03 << 8);     // default MCLK DA mode set to serve 8K sampling rate, you should change this when changing sampling rate
	tmp |= (0x01 << 14);    // I2S mode
	tmp |= (0x03 << 4);     // default MCLK AD mode set to serve 8K sampling rate, you should change this when changing sampling rate
    //5. set adda as slave mode
	tmp &= ~(0x01 << 1);    // set ADAA Slave Mode

    *(volatile unsigned int *)(base + 0x0) = tmp;
    //6. reset
    adda308_i2s_apply();
}

static void adda308_i2s_init(void)
{
    u32 tmp = 0;
	u32 base = 0;

	base = priv.adda308_reg;

    adda308_i2s_ctrl_init();
    // set default value
    adda308_i2s_reset();
    //7. set master
    tmp = *(volatile unsigned int *)(base + 0x0);
    if (ISMASTER == 1)
	    tmp |= (0x01 << 1);     // set ADDA Master Mode
    *(volatile unsigned int *)(base + 0x0) = tmp;
    // reset
    adda308_i2s_apply();
    //adda308_i2s_apply();
    //adda308_i2s_powerdown(0);
}

static void adda308_get_chip_id(void)
{
    u32 temp;

    temp = ftpmu010_read_reg(0x00);

    bond_id = (temp & 0xf) | ((temp >> 4) & 0xf0);
    chip_id = (ftpmu010_get_attr(ATTR_TYPE_CHIPVER) >> 16) & 0xFFFF;

    if (chip_id != 0x8135 && chip_id != 0x8136)
        panic("adda308 can not recognize this chip_id %x\n", chip_id);
}

int adda308_get_output_mode(void)
{
    return output_mode;
}

static int __init adda308_drv_init(void)
{
    memset(&priv, 0x0, sizeof(adda308_priv_t));

    /* init the clock and add the device .... */
    platform_init();

    /* Register the driver */
	if (platform_driver_register(&adda308_driver)) {
		printk("Failed to register ADDA driver\n");
		return -ENODEV;
	}

    adda308_get_chip_id();

    if (output_mode == 0)
        clk_pll2 = 0;       // speaker out, source from PLL1/cntp3 : 24.5454MHz
    else
        clk_pll2 = 1;       // line out, source from PLL2 : 24MHz

    // set SSP Main clock : 24MHz
    adda308_ssp_mclk_init();
    // select da source from ADDA or decoder
    adda308_da_select();

    // Change PMU setting
    if (input_mode == 2)
        adda308_PMU_Select_DMIC_Input(&DMIC_Reg[adda308_DMIC_DEF_PMU]);
    // init adda308
    adda308_i2s_init();

    adda308_proc_init();

    return 0;
}

static void __exit adda308_drv_clearnup(void)
{
    adda308_proc_remove();

    platform_driver_unregister(&adda308_driver);


    platform_exit();

    __iounmap((void *)priv.adda308_reg);
}

ADDA308_SSP_USAGE
ADDA308_Check_SSP_Current_Usage(void)
{
    //return (same_i2s_tx_rx==1)?(adda308_SSP0RX_SSP0TX):(ADDA203_SSP0RX_SSP1TX);
    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 0))  // adda da source select SSP0
        return ADDA308_SSP0RX_SSP0TX;
    if ((audio_out_enable[0] == 0) && (audio_out_enable[1] == 1))  // adda da source select SSP1
        return ADDA308_SSP0RX_SSP1TX;
    if ((audio_out_enable[0] == 1) && (audio_out_enable[1] == 1))  // adda da source select SSP0, SSP1 for decoder
        return ADDA308_SSP0RX_SSP0TX;

    return 0;
}
EXPORT_SYMBOL(ADDA308_Check_SSP_Current_Usage);

static void
adda308_PMU_Select_DMIC_Input(pmuReg_t *reg)
{
    if (!reg) {
        panic("%s(%d) NULL reg!\n",__FUNCTION__,__LINE__);
        return;
    }

    if (ftpmu010_write_reg(priv.adda308_fd, reg->reg_off, reg->init_val, reg->bits_mask) < 0) {
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
EXPORT_SYMBOL(adda308_get_output_mode);
EXPORT_SYMBOL(ADDA308_Set_DA_Source);

module_init(adda308_drv_init);
module_exit(adda308_drv_clearnup);
MODULE_LICENSE("GPL");
