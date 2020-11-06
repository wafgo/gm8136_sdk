
#ifndef __KEYSCAN_DEV_H__
#define __KEYSCAN_DEV_H__

#include "ks_drv.h"

#ifdef __KEYSCAN_DEV_C__
  #define KEYSCAN_DEV_EXT
#else
  #define KEYSCAN_DEV_EXT extern
#endif


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
#define KEYSCAN_Control         0x0
#define KEYSCAN_OutPinSel       0x4
#define KEYSCAN_InPinSel        0x8
#define KEYSCAN_PinPullUpSel    0xC
#define KEYSCAN_ClockDiv        0x10
#define KEYSCAN_Status          0x14
#define KEYSCAN_Data            0x18
#define KEYSCAN_DataIndex       0x1C

/* function bit */
#define KS_CTRL_ENABLE          BIT0
#define KS_CTRL_START           BIT1
#define KS_CTRL_STOP            BIT2
#define KS_CTRL_START_SCAN      (KS_CTRL_ENABLE|KS_CTRL_START)
#define KS_CTRL_STOP_SCAN       (KS_CTRL_STOP)

#define KS_STS_INT              BIT0

#define KEYSCAN_RAW_REG_RD(base, offset)         (ioread32((base)+(offset)))
#define KEYSCAN_RAW_REG_WR(base, offset, val)    (iowrite32(val, (base)+(offset)))

#if defined(CONFIG_PLATFORM_GM8210)
#define KEY_O    BIT0|BIT1|BIT2 /* O1 O2 O3 */
#define KEY_I	 BIT5|BIT6|BIT7 /* I1 I2 I3 */
#elif defined(CONFIG_PLATFORM_GM8287)
#define KEY_O    BIT20|BIT24|BIT17 /* O1 O2 O3 */
#define KEY_I    BIT18|BIT21|BIT19 /* I1 I2 I3 */
#endif

#define KEY_PULLUP  KEY_O|KEY_I
KEYSCAN_DEV_EXT void Keyscan_SetControl(unsigned int base, unsigned int ctrl_cmd);
KEYSCAN_DEV_EXT void Keyscan_SetOutPinSelection(unsigned int base, unsigned int gpio_bmp);
KEYSCAN_DEV_EXT void Keyscan_SetInPinSelection(unsigned int base, unsigned int gpio_bmp);
KEYSCAN_DEV_EXT void Keyscan_SetPinPullUpSelection(unsigned int base, unsigned int gpio_bmp);
KEYSCAN_DEV_EXT void Keyscan_SetClockDiv(unsigned int base, unsigned int val);
KEYSCAN_DEV_EXT void Keyscan_SetStatus(unsigned int base, unsigned int st);
KEYSCAN_DEV_EXT unsigned int Keyscan_GetStatus(unsigned int base);
KEYSCAN_DEV_EXT unsigned int Keyscan_GetData(unsigned int base);
KEYSCAN_DEV_EXT unsigned int Keyscan_GetDataIndex(unsigned int base);
extern void restart_hardware(struct work_struct* data);
KEYSCAN_DEV_EXT unsigned long jiffies_diff(unsigned long a, unsigned long b);


#endif /*__KEYSCAN_DEV_H__*/

