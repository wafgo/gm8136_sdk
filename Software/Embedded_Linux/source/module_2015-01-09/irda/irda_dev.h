
#ifndef __IRDA_DEV_H__
#define __IRDA_DEV_H__

#include "irda_drv.h"


#define BIT0  (0x00000001)
#define BIT1  (0x00000002)
#define BIT2  (0x00000004)
#define BIT3  (0x00000008)
#define BIT4  (0x00000010)
#define BIT5  (0x00000020)
#define BIT6  (0x00000040)
#define BIT7  (0x00000080)
#define BIT8  (0x00000100)
#define BIT9  (0x00000200)
#define BIT10 (0x00000400)
#define BIT11 (0x00000800)
#define BIT12 (0x00001000)
#define BIT13 (0x00002000)
#define BIT14 (0x00004000)
#define BIT15 (0x00008000)
#define BIT16 (0x00010000)
#define BIT17 (0x00020000)
#define BIT18 (0x00040000)
#define BIT19 (0x00080000)
#define BIT20 (0x00100000)
#define BIT21 (0x00200000)
#define BIT22 (0x00400000)
#define BIT23 (0x00800000)
#define BIT24 (0x01000000)
#define BIT25 (0x02000000)
#define BIT26 (0x04000000)
#define BIT27 (0x08000000)
#define BIT28 (0x10000000)
#define BIT29 (0x20000000)
#define BIT30 (0x40000000)
#define BIT31 (0x80000000)



/* register offset */
#define IRDET_Control           0x0
#define IRDET_Param0            0x4
#define IRDET_Param1            0x8
#define IRDET_Param2            0xC
#define IRDET_Status            0x10
#define IRDET_Data              0x14

/* function bit */
#define IRDET_CTRL_ENABLE           BIT0
#define IRDET_CTRL_NSCD             BIT1
#define IRDET_CTRL_IIL              BIT2
#define IRDET_CTRL_BI_PHASE         BIT3
#define IRDET_CTRL_MSB_F            BIT4

#define IRDET_PARAM0_TIMEOUT_TH     (0xFFFF8000)
#define IRDET_PARAM0_HIGH_TH0       (0x00007FFF)

#define IRDET_PARAM1_LOW_TH1        (0xFFFF0000)
#define IRDET_PARAM1_LOW_TH0        (0x0000FFFF)

#define IRDET_PARAM2_CHANNEL_SEL    (0x001F0000)
#define IRDET_PARAM2_FIFO_SIZE      (0x00003F00)
#define IRDET_PARAM2_CLK_DIV        (0x000000FF)

#define IRDET_STS_IR_INT            BIT0
#define IRDET_STS_IR_REPEAT         BIT1
#define IRDET_STS_IR_DATA_AMOUNT    (0xFC)

/* Default protocol */
#define IRDET_PARAM0_NEC            (0x04000030)
#define IRDET_PARAM1_NEC            (0x00700190)
#define IRDET_PARAM2_NEC            (0x00001F77)
#define IRDET_STS_NEC               (0x3)
#define IRDET_CTRL_NEC              (0x05)

#define IRDET_PARAM0_RCA            (0x04000032)
#define IRDET_PARAM1_RCA            (0x00700190)
#define IRDET_PARAM2_RCA            (0x00001F77)
#define IRDET_STS_RCA               (0x3)
#define IRDET_CTRL_RCA              (0x15)

#define IRDET_PARAM0_SHARP          (0x0FC0001E)
#define IRDET_PARAM1_SHARP          (0x00760FC0)
#define IRDET_PARAM2_SHARP          (0x00081E77)
#define IRDET_STS_SHARP             (0x3)
#define IRDET_CTRL_SHARP            (0x03)

#define IRDET_PARAM0_PHILIPS_RC5    (0x01100000)
#define IRDET_PARAM1_PHILIPS_RC5    (0x00870000)
#define IRDET_PARAM2_PHILIPS_RC5    (0x00101F77)
#define IRDET_STS_PHILIPS_RC5       (0x3)
#define IRDET_CTRL_PHILIPS_RC5      (0x07)

#define IRDET_PARAM0_NOKIA_NRC17    (0x01300017)
#define IRDET_PARAM1_NOKIA_NRC17    (0x004B00F8)
#define IRDET_PARAM2_NOKIA_NRC17    (0x001F3F77)
#define IRDET_STS_NOKIA_NRC17       (0x3)
#define IRDET_CTRL_NOKIA_NRC17      (0x09)

#define IRDET_NEC           0
#define IRDET_RCA           1
#define IRDET_SHARP         2
#define IRDET_PHILIPS_RC5   3
#define IRDET_NOKIA_NRC17   4

#if(HZ == 1000)
#define DEFAULT_DELAY_DURATION       100
#elif (HZ == 100)
#define DEFAULT_DELAY_DURATION       10
#else
#define DEFAULT_DELAY_DURATION       100
#endif

#define IRDET_HI_PARAM_MSK           0x7FFF //14bit
#define IRDET_LO_PARAM_HMSK          0xFFFF0000 //16bit
#define IRDET_LO_PARAM_LMSK          0x0000FFFF //16bit


extern void restart_hardware(struct work_struct* data);
extern void Irdet_SetControl(unsigned int base, unsigned int ctrl_cmd);
extern void Irdet_SetParam0(unsigned int base, unsigned int Param);
extern void Irdet_SetParam1(unsigned int base, unsigned int Param);
extern void Irdet_SetParam2(unsigned int base, unsigned int Param);
extern void Irdet_SetStatus(unsigned int base, unsigned int st);
extern unsigned int Irdet_GetStatus(unsigned int base);
extern unsigned int Irdet_GetData(unsigned int base);
extern int Irdet_HwSetup(unsigned int base, int protocol, int gpio_num);
extern unsigned int Irdet_GetParam0(unsigned int base);
extern unsigned int Irdet_GetParam1(unsigned int base);
extern int Irdet_Set_HL_Param(unsigned short hi_pam,unsigned short loh_pam,unsigned short lol_pam);


#endif /*__IRDA_DEV_H__*/

