#if defined(CONFIG_PLATFORM_GM8210) || defined(CONFIG_PLATFORM_GM8287) 

#include "platform.h"
#include "ks_drv.h"
#include "ks_dev.h"
#include <mach/ftpmu010.h>

#define PRINT_E(args...) do { printk(args); }while(0)
#define PRINT_I(args...) do { printk(args); }while(0)


#define CLKOFF           0xB8
#define CLKOFF_IRDET    (0x1<<12)

#define GPIO_REG		 0x50

#if defined(CONFIG_PLATFORM_GM8210)

#define TRANS_KEYSCAN_TO_GPIO_MAP(n) (n)
#define GPIO_MODE_SET   (0) 
#define GPIO_MSK		(0x1<<31 | 0x1<<30)

#elif defined(CONFIG_PLATFORM_GM8287)

#define GPIO_MODE_SET   (0x001)
#define GPIO_MSK		(0x1<<2 | 0x01<<1 |0x1<<0)
int keyscan_map_to_gpio[] = {
/*0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10,11,12,13,14,15*/
  0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10,11,12,13,14,15,
/*16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31*/  
  16,36,37,38,39,40,45,46,49,33,35,27,28,29,30,31}; 

#define KEYSACN_MAP_TO_GPIO_NUM    ARRAY_SIZE(keyscan_map_to_gpio)
#define TRANS_KEYSCAN_TO_GPIO_MAP(n)  keyscan_map_to_gpio[n]

#endif

static int scu_fd;

/* *INDENT-OFF* */
static pmuReg_t pmu_reg[] = {
/*  {reg_off,   bits_mask,     lock_bits,    init_val,    init_mask } */ 
    {CLKOFF,    CLKOFF_IRDET,  CLKOFF_IRDET, 0x0,         0x0}, // on/off
    {GPIO_REG,  GPIO_MSK,  	   GPIO_MSK, 	 0x0,         0x0}, // on/off
};


static pmuRegInfo_t pmu_reg_info = {
    DEV_NAME,
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_NONE,
    &pmu_reg[0],
};
/* *INDENT-ON* */

int register_scu(void)
{
    scu_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (scu_fd < 0) {
        printk("Failed to register %s scu\n", DEV_NAME);
        goto exit;
    }

    return 0;

  exit:
    return -1;
}


int deregister_scu(void)
{
    if (scu_fd >= 0) {
        if (ftpmu010_deregister_reg(scu_fd)) {
            printk("Failed to deregister %s scu\n", DEV_NAME);
            goto exit;
        }
        scu_fd = -1;
    }
    return 0;

  exit:
    return -1;
}

int clock_on(void)
{
    if (unlikely(scu_fd < 0)) {
        goto exit;
    }

    //if (ftpmu010_write_reg(scu_fd, CLKOFF, 1, CLKOFF_IRDET)) {
    if (ftpmu010_write_reg(scu_fd, CLKOFF, 0 , CLKOFF_IRDET)) {
        PRINT_E("Failed to enable %s scu\n", DEV_NAME);
        goto exit;
    }

    return 0;

  exit:
    return -1;
}

int clock_off(void)
{
    if (unlikely(scu_fd < 0)) 
    {
        goto exit;
    }

    //if (unlikely(ftpmu010_write_reg(scu_fd, CLKOFF, CLKOFF_IRDET, CLKOFF_IRDET))) 
    if (unlikely(ftpmu010_write_reg(scu_fd, CLKOFF, 0, CLKOFF_IRDET))) 
    {
        PRINT_E("Failed to disable %s scu\n", DEV_NAME);
        goto exit;
    }

    return 0;

  exit:
    return -1;
}


#if 1
int scu_probe(struct scu_t *p_scu)
{
    int ret = 0;
    
    
    if (unlikely((ret = register_scu()) < 0)) 
    {
        printk("register_scu errr\n");
        goto err1;
    }
    
    if (unlikely((ret = clock_on()) < 0)) 
    {
        printk("enable_scu errr\n");
        goto err2;
    }

    return 0;

    err2:
        deregister_scu();
    err1:
        return ret;
}

int scu_remove(struct scu_t *p_scu)
{
    clock_off();
    deregister_scu();

    return 0;
}

int set_pinmux(void)
{
 	unsigned int pmu_mfps = ftpmu010_read_reg(GPIO_REG);
	int ret = 0;

	pmu_mfps &= ~(GPIO_MSK);

	/* set 0 to select GPIO 9*/
	if (unlikely((ret = ftpmu010_write_reg(scu_fd, GPIO_REG,GPIO_MODE_SET, GPIO_MSK))!= 0))
	{
		PRINT_E("%s set as 0x%08X\n", __func__, pmu_mfps);
	}
#if 0
	else
	{
		PRINT_I("%s set as 0x%08X OK\n", __func__, pmu_mfps);
	}
#endif
	pmu_mfps = ftpmu010_read_reg(GPIO_REG);
	PRINT_I("%s set as 0x%02X 0x%08X OK\n", __func__, GPIO_REG, pmu_mfps);

    return ret;
}

int Trans_Keyscan_To_Gpio(int key_num)
{
	return TRANS_KEYSCAN_TO_GPIO_MAP(key_num);
}
#endif
#endif


