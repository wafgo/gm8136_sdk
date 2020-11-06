/*
 * Revision History
 * 2013/11/27 prevent CVBS jitter for read saradc REG 2
 * 2013/12/13 support runing mode for 8139
 */
#include "platform.h"                                                                                                                                                     
#include "sar_adc_drv.h"                                                                                                                                                  
#include "sar_adc_dev.h"                                                                                                                                                  
#include <mach/ftpmu010.h>                                                                                                                                                
#include <linux/delay.h>                                                                                                                                                  
#include <linux/kthread.h>

#define PRINT_E(args...) do { printk(args); }while(0)                                                                                                                     
#define PRINT_I(args...) do { printk(args); }while(0)                                                                                                                     
                                                                                                                                                                          
#define PLL_CLK          0xB8                                                                                                                                             
#define PLL_CLK_BIT     (0x1<<8 )  /* ADC*/                                                                                                                                        
                                                                                                                                                                          
#define GATE_CLK	     0xAC                                                                                                                                     
#define GATE_CLK_BIT	(0x1 << 3)/*  bit3 is SAR ADC */                                                                                          

#define CLOCK_DIVID_BIT  0x003F0000
static int scu_fd;

#define POLLING_DELAY  200
#define ADDA_PLUG_AUTO 0x00
#define ADDA_PLUG_IN   0x01
#define ADDA_PLUG_OUT  0x02

#define XGAIN_MASK     (0x7 | 0x03 << 20)
#define ENABLE_XGAIM_0 (0x1|0x1<<19|0x1<<20)
#define ENABLE_XGAIM_1 (0x2|0x1<<19|0x2<<20)
#define ENABLE_XGAIM_2 (0x4|0x0<<19|0x3<<20)
#define CVBS_PLUG_REPEAT_TIME      2

struct timer_list xain_change_timer;                                                                                                                                                                          
/* *INDENT-OFF* */                                                                                                                                                        
static pmuReg_t pmu_reg[] = {                                                                                                                                             
/*  {reg_off,   bits_mask,     lock_bits,    init_val,    init_mask } */                                                                                                  
    {PLL_CLK,   PLL_CLK_BIT,   PLL_CLK_BIT,      0x0,      PLL_CLK_BIT }, // ADC clock selection                                                                          
    {GATE_CLK,  GATE_CLK_BIT,  GATE_CLK_BIT, 	 0x0<<3,   0x1<< 3}, // Enable ADC 
    {0x74,  CLOCK_DIVID_BIT,  CLOCK_DIVID_BIT, 	 (0x27 << 16), CLOCK_DIVID_BIT },//30MHZ /40 = 750khz
};                                                                                                                                                                        
                                                                                                                                                                          
static pmuRegInfo_t pmu_reg_info = {                                                                                                                                      
    DEV_NAME,                                                                                                                                                             
    ARRAY_SIZE(pmu_reg),                                                                                                                                                  
    ATTR_TYPE_NONE,                                                                                                                                                       
    pmu_reg,                                                                                                                                                              
};                                                                                                                                                                        

static struct task_struct *saradc_thread = NULL;
static volatile int saradc_thread_runing = 0;
struct semaphore    polling_sem; /*spec for 8139 usage*/

static volatile unsigned int g_tv100_base = 0;
static volatile unsigned int g_adda_base = 0;

static int  polling_process_is_init = 0;
static int  g_saradc_cvbs_info = ADDA_PLUG_OUT;/*0 :init ,1 : plug in ,2:plug out*/
static int  g_saradc_debug = 0;/*1 : xgain1 ,2:xgain2 */
static int  g_saradc_forcecvbs = ADDA_PLUG_AUTO;/*0 :init ,1 : plug in ,2:plug out*/
static unsigned int  g_cvbs_on_tsr = OUTPUT_VOL_THR_ON;
static unsigned int  g_cvbs_off_tsr = OUTPUT_VOL_THR_OFF;


/************************************/
void Platform_Set_ADDA_TVBS(int enable);

/**********************************/
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

    g_adda_base = (unsigned int) ioremap_nocache ((uint32_t)ADDA_PA_START,(ADDA_PA_END -  ADDA_PA_START +1));
    g_tv100_base = (unsigned int)ioremap_nocache(TVE_FTTVE100_PA_BASE, TVE_FTTVE100_PA_SIZE);
    printk(" * ADDA paddr=%x vaddr=%x tve paddr=%x vaddr=%x\n",ADDA_PA_START,g_adda_base,TVE_FTTVE100_PA_BASE,g_tv100_base);
    return 0;                                                                                                                                                             
}                                                                                                                                                                         
                                                                                                                                                                          
int scu_remove(struct scu_t *p_scu)                                                                                                                                       
{                                                                                                                                                                         
    int ret;                                                                                                                                                              
                                                                                                                                                                          
    ret = deregister_scu();  

     __iounmap((void *)g_adda_base);
    g_adda_base = 0;
    __iounmap((void *)g_tv100_base);
    g_tv100_base = 0;
    
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
    PRINT_I(" * %s set as 0x%02X 0x%08X OK\n", __func__, PLL_CLK, pmu_mfps);
    return ret;                                                                                                                                                           
}
static void _platform_process_xgain1_value(struct dev_data* p_dev_data)
{
    unsigned int vbase = (unsigned int)p_dev_data->io_vadr;
    struct dev_specific_data_t* p_data  = (struct dev_specific_data_t*)&p_dev_data->dev_specific_data;
    unsigned int adc_val;
    int          len;
    sar_adc_pub_data new_data;
    
    adc_val=SAR_ADC_RAW_REG_RD(vbase,FTSAR_ADC_DAT1);
    if(g_saradc_debug)
        printk("xgain1_value = %d\n",adc_val);
    
    if(adc_val == 0)
        return;

    new_data.adc_val = adc_val;
    new_data.status  = KEY_XAIN_1;

    Platform_Lock_Polling_Mutex();
    
    len = kfifo_in(
		&p_data->fifo, 
		&new_data,
		sizeof(sar_adc_pub_data)
		);
    
    Platform_UnLock_Polling_Mutex();
    
    if(len < sizeof(sar_adc_pub_data)){
        if(g_saradc_debug)
            printk("put data into fifo queue fail \n");
    }
     
}
static void change_detect_xain(struct dev_data* data)                                                                                                                        
{                                                                                                                                                                         
    struct dev_data *p_dev_data = (struct dev_data*)data;                                                                                                             
    unsigned int base = (unsigned int)p_dev_data->io_vadr;                                                                                                            
	                                                                                                                                                                  
    static int xain_num=0;                                                                                                                                                
    unsigned int val,ran_flag = 0;
    
    val = SAR_ADC_RAW_REG_RD( base, FTSAR_ADC_CR0);
    
    do{
        switch(xain_num){
            case 0:
                if(p_dev_data->running_mode & SAR_ADC_RUNING_MODE_GAIN0){
                    val &= ~XGAIN_MASK; 
                    val |= ENABLE_XGAIM_0;                                                                                                                                                
                    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_CR0, val);                                                                                                                    
                    xain_num = 1;
                    ran_flag = 1;                   
                }
                else{
                    xain_num = 1;
                    ran_flag = 0;         
                }
                break;
            case 1:
                if(p_dev_data->running_mode & SAR_ADC_RUNING_MODE_GAIN1){
                    val &= ~XGAIN_MASK; 
                    val |= ENABLE_XGAIM_1;                                                                                                                                                
                    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_CR0, val);
                    mdelay(1);
                    _platform_process_xgain1_value(p_dev_data);
                    xain_num = 2;
                    ran_flag = 1;                  
                }
                else{
                    xain_num = 2;
                    ran_flag = 0;                  
                }
                break;
            case 2:
                if(p_dev_data->running_mode & SAR_ADC_RUNING_MODE_GAIN2){
                    val &= ~XGAIN_MASK; 
                    val |= ENABLE_XGAIM_2;                                                                                                                                                
                    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_CR0, val);
                    mdelay(1);
                    Platform_Process_Volt_Threshold(p_dev_data);
                    xain_num = 0;
                    ran_flag = 1;                   
                }
                else{
                    xain_num = 0;
                    ran_flag = 0;                    
                }
                break;
            default:
                printk("%s unexpected case\n",__func__);
                break;
        }
    }while(!ran_flag);
                                                                                                                        
} 

static int saradc_polling_thread(void *private)
{
    struct dev_data *p_dev_data = (struct dev_data*)private;
    saradc_thread_runing = 1;

    /*First time set adda and tvbs down*/
    Platform_Set_ADDA_TVBS(0);
    
    do {
        
        change_detect_xain(p_dev_data);
        msleep(POLLING_DELAY);
        
    }while(!kthread_should_stop());
    
    saradc_thread_runing = 0;
    return 0;
}

void Platform_Set_Init_Reg(struct dev_data *p_dev_data)                                                                                                                   
{ 
    unsigned int val;                                                                                                                                                 
    unsigned int base = (unsigned int)p_dev_data->io_vadr;
    unsigned char lmin=0,lmax=0;

    lmin = p_dev_data->dev_specific_data.xgain0_min & 0xff;
	lmax = p_dev_data->dev_specific_data.xgain0_max & 0xff; 
    
    val = (lmax << 16)| lmin;/* set high threshold as 0x00010001 */                                                                                                                 
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_THD0, val);                                                                                                                       
    val = 0x00100000;/* set high threshold as 0x00010001 */                                                                                                                     
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_THD1, val);                                                                                                                       

    val = 0x0;/* xlain 2 low threshold */                                                                                                        
    val |= 0x0 << 16;/* xlain 2 high threshold */                                                                                                
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_THD2, val);  

    val =  0x01; /* xlain0 auto request period */                                                                                                                     
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_AUTO_REQ0, val); 
    
    val =  0x01; /* xlain1 auto request period */                                                                                                                     
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_AUTO_REQ1, val); 

    val =  0x01; /* xlain2 auto request period */                                                                                                                     
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_AUTO_REQ2, val); 

    val = SAR_ADC_RAW_REG_RD(g_adda_base, ADDA_DAC_REG);                                                                                                                
    val |= 0x10;                                                                                                                                                      
    SAR_ADC_RAW_REG_WR( g_adda_base, ADDA_DAC_REG, val); 

    val= SAR_ADC_RAW_REG_RD(base, FTSAR_ADC_CR0);
    //val = 0x0B018701;    
    val = 0x0B19FF01; /*xgain 0 enable interrupt , xgain_1 xgain_2 polling mode*/
    SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_CR0, val); 

    if(lmin!=0 || lmax!=0){
        val = 0x3; /*bit 0 low below interrupt,bit 1 hight above*/
        SAR_ADC_RAW_REG_WR( base, FTSAR_ADC_INT, val);
    }
    udelay(50);
}
                                                                                                                                                                          
int Platform_Init_Pollign_Timer(struct dev_data *p_dev_data)                                                                                                             
{    
    init_MUTEX(&polling_sem);
    
    saradc_thread = kthread_create(saradc_polling_thread, (void*)p_dev_data, "saradc thread");
    if (IS_ERR(saradc_thread)){
        printk("SARADC: Error in creating kernel thread! \n");
        return -1;
    }

    wake_up_process(saradc_thread); 
    polling_process_is_init = 1;
    return 0;
}                                                                                                                                                                         
                                                                                                                                                                          
void Platform_Del_Pollign_Timer(void)                                                                                                                                     
{
    unsigned int val;
    
    if (saradc_thread)
        kthread_stop(saradc_thread);
    
    while (saradc_thread_runing == 1)
        msleep(1);
    
    /* turn on ADDA wrapper
     */
    /* 1. video_dac_stby low */
    val = SAR_ADC_RAW_REG_RD(g_adda_base, ADDA_DAC_REG);                                                                                                                
    val &= ~(0x1 << 3);
    /* 2. PD, PSW_PD, ISO_ENABLE */
    val &= ~(0x7); 
    SAR_ADC_RAW_REG_WR(g_adda_base, ADDA_DAC_REG, val); 

    /* turn on TVE100
     */
    val = SAR_ADC_RAW_REG_RD(g_tv100_base, TVE_POWER_DOWN_REG);
    val &= ~(0x7);
    SAR_ADC_RAW_REG_WR(g_tv100_base,TVE_POWER_DOWN_REG,val);   
}                                                                                                                                                                         
                                                                                                                                                                          
void Platform_Process_Volt_Threshold(struct dev_data *p_dev_data)                                                                                    
{
    unsigned int vbase = (unsigned int)p_dev_data->io_vadr; 
    unsigned int adc_val ,val; 
    static int  out_detect_count = 0  ,int_detect_count = 0;
        
    adc_val=SAR_ADC_RAW_REG_RD(vbase,FTSAR_ADC_DAT2);

    if(g_saradc_debug)
        printk("Platform_Process_Volt_Threshold=%d-%d-%d\n",adc_val,g_saradc_cvbs_info,g_saradc_forcecvbs);

    if(adc_val == 0 ){
        return;
    }

    if(g_saradc_forcecvbs != ADDA_PLUG_AUTO){
        return;
    }

    if(g_saradc_cvbs_info == ADDA_PLUG_IN && adc_val > g_cvbs_on_tsr){                                                                                                                                
        out_detect_count++;
        if(out_detect_count  < CVBS_PLUG_REPEAT_TIME){
         
            return;
        }
        out_detect_count = 0;
        int_detect_count = 0;
        printk(KERN_DEBUG "plug-out=%x\n",adc_val);
        g_saradc_cvbs_info = ADDA_PLUG_OUT;
        //pre_adda_status = ADDA_PLUG_OUT;
        /* turn off ADDA wrapper
         */
        /* 1. set ISO enable */
        val = SAR_ADC_RAW_REG_RD(g_adda_base, ADDA_DAC_REG);                                                                                                                
        val |= (0x1);
        SAR_ADC_RAW_REG_WR( g_adda_base, ADDA_DAC_REG, val); 
        /* 2. PD, PSW_PD */
        val |= (0x6); 
        SAR_ADC_RAW_REG_WR( g_adda_base, ADDA_DAC_REG, val);

        /* turn off TVE100
         */
        val = SAR_ADC_RAW_REG_RD(g_tv100_base, TVE_POWER_DOWN_REG);
        val |= (0x7);
        SAR_ADC_RAW_REG_WR(g_tv100_base,TVE_POWER_DOWN_REG,val); 
   
    }
    else if(g_saradc_cvbs_info == ADDA_PLUG_OUT && adc_val < g_cvbs_off_tsr){ 
        int_detect_count++;
        if(int_detect_count  < CVBS_PLUG_REPEAT_TIME){
            return;
        }
        int_detect_count = 0;
        out_detect_count = 0;
        printk(KERN_DEBUG "plug-in=%x\n",adc_val);
        //pre_adda_status = ADDA_PLUG_IN;
        g_saradc_cvbs_info = ADDA_PLUG_IN;
        /* turn on ADDA wrapper
         */
        /* 1. video_dac_stby low */
        val = SAR_ADC_RAW_REG_RD(g_adda_base, ADDA_DAC_REG);                                                                                                                
        val &= ~(0x1 << 3);
        SAR_ADC_RAW_REG_WR( g_adda_base, ADDA_DAC_REG, val);
        /* 2. PD, PSW_PD, ISO_ENABLE */
        val &= ~(0x7); 
        SAR_ADC_RAW_REG_WR( g_adda_base, ADDA_DAC_REG, val); 

        /* turn on TVE100
         */
        val = SAR_ADC_RAW_REG_RD(g_tv100_base, TVE_POWER_DOWN_REG);
        val &= ~(0x7);
        SAR_ADC_RAW_REG_WR(g_tv100_base,TVE_POWER_DOWN_REG,val);  
    }
    else if(g_saradc_cvbs_info == ADDA_PLUG_IN && adc_val <= g_cvbs_on_tsr){
        int_detect_count = 0;
        out_detect_count = 0;
    }
    else if(g_saradc_cvbs_info == ADDA_PLUG_OUT && adc_val >= g_cvbs_off_tsr){
        int_detect_count = 0;
        out_detect_count = 0;
    }
    else {
        if(g_saradc_debug)
            printk("Uknow state=%d-%d\n",adc_val,g_saradc_cvbs_info);
    }

} 
int Platform_SAR_ADC_GetData(struct dev_data *p_dev_data , sar_adc_pub_data* new_data)
{
    unsigned int base = (unsigned int)p_dev_data->io_vadr;
    unsigned int value;

    value=SAR_ADC_RAW_REG_RD(base,FTSAR_ADC_DAT0);

    if(g_saradc_debug)
        printk("xgain0_value = %d\n",value);
      
    if(value > 0x0) {
        new_data->adc_val = value;
        new_data->status = KEY_XAIN_0;
        return 0;
    }
#if 0 

    value = SAR_ADC_RAW_REG_RD(base,FTSAR_ADC_DAT1);
    if(value > 0x0) {
        new_data->adc_val = value;
        new_data->status = KEY_XAIN_1;
        return 0;
    }
   
    value=SAR_ADC_RAW_REG_RD(base,FTSAR_ADC_DAT2); 
    if(value > 0x0) {
        new_data->adc_val = value;
        new_data->status = KEY_XAIN_2;
        return 0;
    }
#endif    
    new_data->adc_val = 0;
    new_data->status = 0;
    return -1;
}

int Platform_Set_CVBS_THR_Value(unsigned int on_vlaue,unsigned int off_value)
{
    if(on_vlaue >= OUTPUT_VOL_THR_TOP || on_vlaue <= OUTPUT_VOL_THR_DOWN
       || off_value >= OUTPUT_VOL_THR_TOP || off_value <= OUTPUT_VOL_THR_DOWN)
       return -1;
    
    g_cvbs_on_tsr =  on_vlaue;
    g_cvbs_off_tsr = off_value;
    return 0;
}

int Platform_Get_CVBS_THR_Value(unsigned int *on_vlaue,unsigned int *off_value)
{
    if(on_vlaue == NULL || off_value == NULL)
        return -1;
   
    *on_vlaue = g_cvbs_on_tsr;
    *off_value = g_cvbs_off_tsr;
    
    return 0;
}

void Platform_Set_ADDA_TVBS(int enable)
{
    unsigned int val = 0;
    if(enable)
    {
        /* turn on ADDA wrapper
         */
        /* 1. video_dac_stby low */
        val = SAR_ADC_RAW_REG_RD(g_adda_base, ADDA_DAC_REG);                                                                                                                
        val &= ~(0x1 << 3);
        SAR_ADC_RAW_REG_WR( g_adda_base, ADDA_DAC_REG, val);
        /* 2. PD, PSW_PD, ISO_ENABLE */
        val &= ~(0x7); 
        SAR_ADC_RAW_REG_WR( g_adda_base, ADDA_DAC_REG, val); 

        /* turn on TVE100
         */
        val = SAR_ADC_RAW_REG_RD(g_tv100_base, TVE_POWER_DOWN_REG);
        val &= ~(0x7);
        SAR_ADC_RAW_REG_WR(g_tv100_base,TVE_POWER_DOWN_REG,val);      
    }else{
        /* turn off ADDA wrapper
         */
        /* 1. set ISO enable */
        val = SAR_ADC_RAW_REG_RD(g_adda_base, ADDA_DAC_REG);                                                                                                                
        val |= (0x1);
        SAR_ADC_RAW_REG_WR( g_adda_base, ADDA_DAC_REG, val); 
        /* 2. PD, PSW_PD */
        val |= (0x6); 
        SAR_ADC_RAW_REG_WR( g_adda_base, ADDA_DAC_REG, val);

        /* turn off TVE100
         */
        val = SAR_ADC_RAW_REG_RD(g_tv100_base, TVE_POWER_DOWN_REG);
        val |= (0x7);
        SAR_ADC_RAW_REG_WR(g_tv100_base,TVE_POWER_DOWN_REG,val); 
    }
}

int Platform_Force_Set_ADDA_TVBS(int force)
{
    if(force == ADDA_PLUG_IN && g_saradc_forcecvbs != ADDA_PLUG_IN){
        Platform_Set_ADDA_TVBS(1);
        g_saradc_forcecvbs = ADDA_PLUG_IN;
        g_saradc_cvbs_info = ADDA_PLUG_IN;
    }
    else if(force == ADDA_PLUG_OUT && g_saradc_forcecvbs != ADDA_PLUG_OUT){
        Platform_Set_ADDA_TVBS(0);
        g_saradc_forcecvbs = ADDA_PLUG_OUT;
        g_saradc_cvbs_info = ADDA_PLUG_OUT;
    }
    else if(force == ADDA_PLUG_AUTO && g_saradc_forcecvbs != ADDA_PLUG_AUTO){
        g_saradc_forcecvbs = ADDA_PLUG_AUTO;
    }
    return 0;
}

int Platform_Force_Get_ADDA_TVBS(void)
{
    return  g_saradc_forcecvbs;   
}

int Platform_Get_CVBS_Info(void)
{
    return g_saradc_cvbs_info;
}

int Platform_Get_SARADC_Debug(void)
{
    return g_saradc_debug;
}

void Platform_Set_SARADC_Debug(int value)
{
    g_saradc_debug = value;
}

int Platform_Get_Polling_Timer_State(void)
{
    if(polling_process_is_init)
        return 1;
    return 0;
}

void Platform_Lock_Polling_Mutex(void)
{
    down(&polling_sem);    
}

void Platform_UnLock_Polling_Mutex(void)
{
    up(&polling_sem);    
}

int Platform_Check_Support_XGain(int num)
{
    if(num >=0 && num <= 2)
        return 0;
    return -1;    
}
unsigned int Platform_Direct_Get_XGain_Value(unsigned int base , int num)
{
    unsigned int value = 0;
    
    if(num == 0){
        value=SAR_ADC_RAW_REG_RD(base,FTSAR_ADC_DAT0);       
    }

    if(num == 1){
        value=SAR_ADC_RAW_REG_RD(base,FTSAR_ADC_DAT1);
    }

    if(num == 2){
        value=SAR_ADC_RAW_REG_RD(base,FTSAR_ADC_DAT2);
       
    }
    return value;
}

