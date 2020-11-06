#define __KEYSCAN_DEV_C__


//platform dependent(START)
//#include <asm/arch/platform/spec.h>
//platform dependent(END)
#include <linux/kernel.h>
#include <asm/page.h>
#include <asm/io.h>
#include "ks_dev.h"

unsigned long jiffies_diff(unsigned long a, unsigned long b)
{
    if(a>b)
        return (a-b);
    else
        return (0xFFFFFFFF-b+a);     //the only case is overflow
}

void Keyscan_SetControl(unsigned int base, unsigned int ctrl_cmd)
{
    KEYSCAN_RAW_REG_WR(base, KEYSCAN_Control, ctrl_cmd);
}

void Keyscan_SetOutPinSelection(unsigned int base, unsigned int gpio_bmp)
{
    KEYSCAN_RAW_REG_WR(base, KEYSCAN_OutPinSel, gpio_bmp);
}

void Keyscan_SetInPinSelection(unsigned int base, unsigned int gpio_bmp)
{
    KEYSCAN_RAW_REG_WR(base, KEYSCAN_InPinSel, gpio_bmp);
}

void Keyscan_SetPinPullUpSelection(unsigned int base, unsigned int gpio_bmp)
{
    KEYSCAN_RAW_REG_WR(base, KEYSCAN_PinPullUpSel, gpio_bmp);
}

void Keyscan_SetClockDiv(unsigned int base, unsigned int val)
{
    KEYSCAN_RAW_REG_WR(base, KEYSCAN_ClockDiv, val);
}

void Keyscan_SetStatus(unsigned int base, unsigned int st)
{
    KEYSCAN_RAW_REG_WR(base, KEYSCAN_Status, st);
}

unsigned int Keyscan_GetStatus(unsigned int base)
{
    return KEYSCAN_RAW_REG_RD(base, KEYSCAN_Status);
}

unsigned int Keyscan_GetData(unsigned int base)
{
    return KEYSCAN_RAW_REG_RD(base, KEYSCAN_Data);
}

unsigned int Keyscan_GetDataIndex(unsigned int base)
{
    return KEYSCAN_RAW_REG_RD(base, KEYSCAN_DataIndex);
}



void restart_hardware(struct work_struct* data)
{
    struct delayed_work* work = 
    		container_of(data, struct delayed_work, work);
	
    struct dev_specific_data_t* p_data = 
		container_of(work, struct dev_specific_data_t, work_st);
	
    struct dev_data* p_dev_data = 
		container_of(p_data, struct dev_data, dev_specific_data);
	unsigned int base = (unsigned int)p_dev_data->io_vadr;
	Keyscan_SetControl(base, KS_CTRL_START_SCAN);
}


