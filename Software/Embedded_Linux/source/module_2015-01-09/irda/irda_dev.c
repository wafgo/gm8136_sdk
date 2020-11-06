#define __IRDET_DEV_C__


//platform dependent(START)
//#include <asm/arch/platform/spec.h>
//platform dependent(END)
#include <linux/kernel.h>
#include <asm/page.h>
#include <asm/io.h>
#include "irda_dev.h"

#define IRDET_RAW_REG_RD(base, offset)         (ioread32((base)+(offset)))
#define IRDET_RAW_REG_WR(base, offset, val)    (iowrite32(val, (base)+(offset)))
#define CHANNEL_SELECT 16 

static unsigned short irda_hi_param = 0,irda_loh_param=0,irda_lol_param=0;

void dump_register(unsigned int base)
{
	int i = 0;
	for (i = 0; i < 5; i++)
	{
		printk("reg %d 0x%08X\n", i, ioread32(base));
		base += 4;
	}
}
void Irdet_SetControl(unsigned int base, unsigned int ctrl_cmd)
{
    IRDET_RAW_REG_WR(base, IRDET_Control, ctrl_cmd);
}

void Irdet_SetParam0(unsigned int base, unsigned int Param)
{
    IRDET_RAW_REG_WR(base, IRDET_Param0, Param);
}

void Irdet_SetParam1(unsigned int base, unsigned int Param)
{
    IRDET_RAW_REG_WR(base, IRDET_Param1, Param);
}

unsigned int Irdet_GetParam0(unsigned int base)
{
    return IRDET_RAW_REG_RD(base, IRDET_Param0);
}

unsigned int Irdet_GetParam1(unsigned int base)
{
    return IRDET_RAW_REG_RD(base, IRDET_Param1);
}

void Irdet_SetParam2(unsigned int base, unsigned int Param)
{
    IRDET_RAW_REG_WR(base, IRDET_Param2, Param);
}

void Irdet_SetStatus(unsigned int base, unsigned int st)
{
    IRDET_RAW_REG_WR(base, IRDET_Status, st);
}

unsigned int Irdet_GetStatus(unsigned int base)
{
    return IRDET_RAW_REG_RD(base, IRDET_Status);
}

unsigned int Irdet_GetData(unsigned int base)
{
    return IRDET_RAW_REG_RD(base, IRDET_Data);
}

int Irdet_HwSetup(unsigned int base, int protocol, int gpio_num)
{
    unsigned int param0 = 0,param1 = 0;
    switch(protocol)
    {
      case IRDET_RCA:
        if(!irda_hi_param)
            param0 = IRDET_PARAM0_RCA;
        else
            param0 = ((IRDET_PARAM0_RCA & ~(IRDET_HI_PARAM_MSK)) | (irda_hi_param&IRDET_HI_PARAM_MSK));

        Irdet_SetParam0(base, param0);

        param1 = IRDET_PARAM1_RCA;

        if(irda_loh_param){
            param1 &= ~(IRDET_LO_PARAM_HMSK);
            param1 |=  ((irda_loh_param <<16) & IRDET_LO_PARAM_HMSK); 
        }

        if(irda_lol_param){
            param1 &= ~(IRDET_LO_PARAM_LMSK);
            param1 |=  (irda_lol_param & IRDET_LO_PARAM_LMSK); 
        }
        Irdet_SetParam1(base, param1);
        Irdet_SetParam2(base, IRDET_PARAM2_RCA | (gpio_num << CHANNEL_SELECT));
        Irdet_SetStatus(base, IRDET_STS_RCA);
        Irdet_SetControl(base, IRDET_CTRL_RCA);
        break;
      case IRDET_SHARP:
        if(!irda_hi_param)
            param0 = IRDET_PARAM0_SHARP;
        else
            param0 = ((IRDET_PARAM0_SHARP & ~(IRDET_HI_PARAM_MSK)) | (irda_hi_param&IRDET_HI_PARAM_MSK));

        Irdet_SetParam0(base, param0);

        param1 = IRDET_PARAM1_SHARP;

        if(irda_loh_param){
            param1 &= ~(IRDET_LO_PARAM_HMSK);
            param1 |=  ((irda_loh_param <<16) & IRDET_LO_PARAM_HMSK); 
        }

        if(irda_lol_param){
            param1 &= ~(IRDET_LO_PARAM_LMSK);
            param1 |=  (irda_lol_param & IRDET_LO_PARAM_LMSK); 
        }
        Irdet_SetParam1(base, param1);
        Irdet_SetParam2(base, IRDET_PARAM2_SHARP | (gpio_num << CHANNEL_SELECT));
        Irdet_SetStatus(base, IRDET_STS_SHARP);
        Irdet_SetControl(base, IRDET_CTRL_SHARP); 
        break;
      case IRDET_PHILIPS_RC5:
        if(!irda_hi_param)
            param0 = IRDET_PARAM0_PHILIPS_RC5;
        else
            param0 = ((IRDET_PARAM0_PHILIPS_RC5 & ~(IRDET_HI_PARAM_MSK)) | (irda_hi_param&IRDET_HI_PARAM_MSK));

        Irdet_SetParam0(base, param0);

        param1 = IRDET_PARAM1_PHILIPS_RC5;

        if(irda_loh_param){
            param1 &= ~(IRDET_LO_PARAM_HMSK);
            param1 |=  ((irda_loh_param <<16) & IRDET_LO_PARAM_HMSK); 
        }

        if(irda_lol_param){
            param1 &= ~(IRDET_LO_PARAM_LMSK);
            param1 |=  (irda_lol_param & IRDET_LO_PARAM_LMSK); 
        }

        Irdet_SetParam1(base, param1);
        Irdet_SetParam2(base, IRDET_PARAM2_PHILIPS_RC5 | (gpio_num << CHANNEL_SELECT));
        Irdet_SetStatus(base, IRDET_STS_PHILIPS_RC5);
        Irdet_SetControl(base, IRDET_CTRL_PHILIPS_RC5);
        break;
      case IRDET_NOKIA_NRC17:
        if(!irda_hi_param)
            param0 = IRDET_PARAM0_NOKIA_NRC17;
        else
            param0 = ((IRDET_PARAM0_NOKIA_NRC17 & ~(IRDET_HI_PARAM_MSK)) | (irda_hi_param&IRDET_HI_PARAM_MSK));

        Irdet_SetParam0(base, param0);

        param1 = IRDET_PARAM1_NOKIA_NRC17;

        if(irda_loh_param){
            param1 &= ~(IRDET_LO_PARAM_HMSK);
            param1 |=  ((irda_loh_param <<16) & IRDET_LO_PARAM_HMSK); 
        }

        if(irda_lol_param){
            param1 &= ~(IRDET_LO_PARAM_LMSK);
            param1 |=  (irda_lol_param & IRDET_LO_PARAM_LMSK); 
        }
        Irdet_SetParam1(base, param1);
        Irdet_SetParam2(base, IRDET_PARAM2_NOKIA_NRC17 | (gpio_num << CHANNEL_SELECT));
        Irdet_SetStatus(base, IRDET_STS_NOKIA_NRC17);
        Irdet_SetControl(base, IRDET_CTRL_NOKIA_NRC17);
        break;

      case IRDET_NEC:
      default:
        if(!irda_hi_param)
            param0 = IRDET_PARAM0_NEC;
        else
            param0 = ((IRDET_PARAM0_NEC & ~(IRDET_HI_PARAM_MSK)) | (irda_hi_param&IRDET_HI_PARAM_MSK));

        Irdet_SetParam0(base, param0);

        param1 = IRDET_PARAM1_NEC;

        if(irda_loh_param){
            param1 &= ~(IRDET_LO_PARAM_HMSK);
            param1 |=  ((irda_loh_param <<16) & IRDET_LO_PARAM_HMSK); 
        }

        if(irda_lol_param){
            param1 &= ~(IRDET_LO_PARAM_LMSK);
            param1 |=  (irda_lol_param & IRDET_LO_PARAM_LMSK); 
        }
        Irdet_SetParam1(base, param1);
        Irdet_SetParam2(base, IRDET_PARAM2_NEC | (gpio_num << CHANNEL_SELECT));
        Irdet_SetStatus(base, IRDET_STS_NEC);
        Irdet_SetControl(base, IRDET_CTRL_NEC);
        break;
    }

	//dump_register(base);
    return 0;
}


//void restart_hardware(struct work_struct *data)
void restart_hardware(struct work_struct* data)
{
    struct delayed_work* work = 
    		container_of(data, struct delayed_work, work);
	
    struct dev_specific_data_t* p_data = 
		container_of(work, struct dev_specific_data_t, work_st);
	
    struct dev_data* p_dev_data = 
		container_of(p_data, struct dev_data, dev_specific_data);

    Irdet_HwSetup((u32)p_dev_data->io_vadr, p_data->protocol, p_data->gpio_num);
}

int Irdet_Set_HL_Param(unsigned short hi_pam,unsigned short loh_pam,unsigned short lol_pam)
{
    irda_hi_param =  hi_pam;
    irda_loh_param = loh_pam;
    irda_lol_param = lol_pam;
    return 0;
}

