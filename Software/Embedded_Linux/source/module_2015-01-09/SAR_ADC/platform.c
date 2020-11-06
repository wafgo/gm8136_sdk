
#include "platform.h"
#include "sar_adc_drv.h"
#include "sar_adc_dev.h"
#include <mach/ftpmu010.h>

#ifdef CONFIG_PLATFORM_GM8287
#include <linux/delay.h>
#include <linux/timer.h>
#define POLLING_DELAY 200
struct timer_list xain_change_timer;
#endif


#define PRINT_E(args...) do { printk(args); }while(0)
#define PRINT_I(args...) do { printk(args); }while(0)

#ifdef CONFIG_PLATFORM_GM8287
#define PLL_CLK          0xB8   /* ADC0 clock */
#define PLL_CLK_BIT     (0x1<<22)

#define GATE_CLK		 0x70  /* ADC0 clk select and pvalue */ 
#define GATE_CLK_BIT	(0xff << 24)
#endif

#ifdef CONFIG_PLATFORM_GM8139
#define PLL_CLK          0xB8
#define PLL_CLK_BIT     (0x1<<8)

#define GATE_CLK		 0xAC
#define GATE_CLK_BIT	(0x1 << 2 | 0x1 << 3)/* bit 2 is ADDA bit3 is SAR ADC */
#endif
static int scu_fd;

#ifdef CONFIG_PLATFORM_GM8287
/* *INDENT-OFF* */
static pmuReg_t pmu_reg[] = {
/*  {reg_off,   bits_mask,     lock_bits,    init_val,    init_mask } */ 
    {PLL_CLK,   PLL_CLK_BIT,   PLL_CLK_BIT,      0x0<<22,         0x1 << 22 }, // ADC PLL clock selection
    {GATE_CLK,  GATE_CLK_BIT,  GATE_CLK_BIT, 	 (0x0f<<24|0x2<<30),0xff<<24}, /* pvalue is 16(0x0f+1) 12M/16=768K , clk sel is 0x2 for dividing  by cntyp */ 
};
#endif

#ifdef CONFIG_PLATFORM_GM8139
/* *INDENT-OFF* */
static pmuReg_t pmu_reg[] = {
/*  {reg_off,   bits_mask,     lock_bits,    init_val,    init_mask } */ 
    {PLL_CLK,   PLL_CLK_BIT,   PLL_CLK_BIT,      0x0<<8,      0x1 << 8 }, // ADC clock selection
    {GATE_CLK,  GATE_CLK_BIT,  GATE_CLK_BIT, 	 (0x0<<2|0x0<<3), (0x1<<2|0x1<< 3) }, // Enable ADC, ADDA clock
};
#endif

static pmuRegInfo_t pmu_reg_info = {
    DEV_NAME,
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_NONE,
    pmu_reg,
};
/* *INDENT-ON* */

int register_scu(void)
{
    scu_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (scu_fd < 0) {
        printk("Failed to register %s scu\n", DEV_NAME);
        return -1;
    }
    return 0;

}


int deregister_scu(void)
{
    if (scu_fd >= 0) {
        if (ftpmu010_deregister_reg(scu_fd)) {
            printk("Failed to deregister %s scu\n", DEV_NAME);
            scu_fd = -1;
            return -1;
        }
    }
    return 0;
}

int scu_probe(struct scu_t *p_scu)
{
    if (unlikely(register_scu() < 0)) 
    {
        printk("register_scu errr\n");
        return -1;
    }
    return 0;
}

int scu_remove(struct scu_t *p_scu)
{
    int ret;

    ret = deregister_scu();
    return ret;
}

int set_pinmux(void)
{
 	unsigned int pmu_mfps = ftpmu010_read_reg(PLL_CLK);
	int ret = 0;

	pmu_mfps &= ~(PLL_CLK_BIT);

	if (unlikely((ret = ftpmu010_write_reg(scu_fd, PLL_CLK , 0, PLL_CLK_BIT))!= 0))
	{
		PRINT_E("%s set as 0x%08X\n", __func__, pmu_mfps);
	}
	pmu_mfps = ftpmu010_read_reg(PLL_CLK);
	PRINT_I("%s set as 0x%02X 0x%08X OK\n", __func__, PLL_CLK, pmu_mfps);

    return ret;
}

#ifdef CONFIG_PLATFORM_GM8287
static void change_detect_xain(unsigned long data)
{
	struct dev_data *p_dev_data = (struct dev_data*)data;
	unsigned int base = (unsigned int)p_dev_data->io_vadr;
	
    static int xain_num=0;
    int val;

    if(!xain_num){
        val = 0x000B2021;
        SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_CR0, val);
        xain_num = 1;
    }else{
        val = 0x000B2022;
        SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_CR0, val);
        xain_num = 0;
    }
    xain_change_timer.expires = jiffies + POLLING_DELAY;
    add_timer(&xain_change_timer);

}
#endif

void Platform_Set_Init_Reg(struct dev_data *p_dev_data)
{
	unsigned int val;
	unsigned int base = (unsigned int)p_dev_data->io_vadr;
	
#if defined(CONFIG_PLATFORM_GM8139)	
	unsigned int adda_base = (unsigned int)p_dev_data->adda_io_vadr;
#endif

#if defined(CONFIG_PLATFORM_GM8287)

	val = 0x00100000;/* set high threshold as 0x10 */
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_THD0, val);
    val = 0x000E0000;/* set high threshold as 0x0E */
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_THD1, val);
    val = 0x000B2021;
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_CR0, val);
    udelay(50);
	
#elif defined(CONFIG_PLATFORM_GM8139)
	val = SAR_ADC_RAW_REG_RD(base, FTSAR_ADC_CR0);
	val |= 0x0A04E014; /* bit 2 13 14 18 active xlain2 */
	SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_CR0, val);

	val = OUTPUT_VOLTAGE_THRESHOLD;/* xlain 2 low threshold */ 
	val |= OUTPUT_VOLTAGE_THRESHOLD << 16;/* xlain 2 high threshold */ 
	SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_THD2, val);

	val =  0x01; /* xlain2 auto request period */
	SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_AUTO_REQ2, val);

	val = SAR_ADC_RAW_REG_RD(adda_base, ADDA_DAC_REG);
	val |= 0x10;
	SAR_ADC_RAW_REG_WR( adda_base, ADDA_DAC_REG, val);


#endif

}

void Platform_Init_Pollign_Timer(struct dev_data *p_dev_data)
{
#if defined(CONFIG_PLATFORM_GM8287)	
/* Because xian 0 and xain 1 can't be selected at the same time, so we need to polling it */
   init_timer(&xain_change_timer);
   xain_change_timer.function = change_detect_xain;
   xain_change_timer.data = ((unsigned long) p_dev_data);
   xain_change_timer.expires = jiffies + POLLING_DELAY;
   add_timer(&xain_change_timer);
#endif
}

void Platform_Del_Pollign_Timer(void)
{
#if defined(CONFIG_PLATFORM_GM8287)	
	del_timer(&xain_change_timer);
#endif
}

void Platform_Process_Volt_Threshold(struct dev_data *p_dev_data,unsigned int tho_val)
{
#if defined(CONFIG_PLATFORM_GM8139)	
    unsigned int vbase = (unsigned int)p_dev_data->io_vadr;

    if(tho_val > OUTPUT_VOLTAGE_THRESHOLD)
       SAR_ADC_RAW_REG_WR(vbase, FTSAR_ADC_THD2, 0x00ff0000 | OUTPUT_VOLTAGE_THRESHOLD); /* [7:0] SAR ADC xlain2 low threshold setting, disable high threshold detection*/
    else
       SAR_ADC_RAW_REG_WR(vbase, FTSAR_ADC_THD2, OUTPUT_VOLTAGE_THRESHOLD << 16); /* [23:16]SAR ADC xlain2 high threshold setting */

#endif
}


