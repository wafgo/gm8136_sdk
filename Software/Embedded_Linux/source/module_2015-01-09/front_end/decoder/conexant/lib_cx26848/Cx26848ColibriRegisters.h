/*+++ *******************************************************************\
*
*  Cx26848ColibriRegisters.h
*
*  Copyright and Disclaimer:
*
*     ---------------------------------------------------------------
*     This software is provided "AS IS" without warranty of any kind,
*     either expressed or implied, including but not limited to the
*     implied warranties of noninfringement, merchantability and/or
*     fitness for a particular purpose.
*     ---------------------------------------------------------------
*
*     Copyright (c) 2013 Conexant Systems, Inc.
*     All rights reserved.
*
*  Cx26848 register definitions for the Colibri registers
*
\******************************************************************* ---*/

#ifndef __CX26848_COLIBRI_REGISTERS__
#define __CX26848_COLIBRI_REGISTERS__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


#define CX2684x_AFEA_SUP_BLK_TUNE1              0x400
#define CX2684x_AFEA_SUP_BLK_TUNE2              0x401
#define CX2684x_AFEA_SUP_BLK_TUNE3              0x402
#define CX2684x_AFEA_SUP_BLK_XTAL               0x403
#define CX2684x_AFEA_SUP_BLK_PLL                0x404
#define CX2684x_AFEA_SUP_BLK_PLL2               0x405
#define CX2684x_AFEA_SUP_BLK_PLL3               0x406
#define CX2684x_AFEA_SUP_BLK_REF                0x407
#define CX2684x_AFEA_SUP_BLK_PWRDN              0x408
#define CX2684x_AFEA_SUP_BLK_TESTPAD            0x409
#define CX2684x_AFEA_ADC_INT5_STAB_REF          0x40A
#define CX2684x_AFEA_ADC_COM_QUANT              0x40B
#define CX2684x_AFEA_ADC_COM_BIAS1              0x40C
#define CX2684x_AFEA_ADC_COM_BIAS2              0x40D
#define CX2684x_AFEA_ADC_COM_BIAS3              0x40E

#define CX2684x_AFEA_CH1_ADC_STATUS             0x420
#define CX2684x_AFEA_CH1_ADC_STATUS2            0x421
#define CX2684x_AFEA_CH1_ADC_CAL_ATEST          0x422
#define CX2684x_AFEA_CH1_ADC_PWRDN_CLAMP        0x423
#define CX2684x_AFEA_CH1_ADC_CTRL_DAC23         0x424
#define CX2684x_AFEA_CH1_ADC_CTRL_DAC1          0x425
#define CX2684x_AFEA_CH1_ADC_DCSERVO_DEM        0x426
#define CX2684x_AFEA_CH1_ADC_FB_FRCRST          0x427
#define CX2684x_AFEA_CH1_ADC_INPUT              0x428
#define CX2684x_AFEA_CH1_ADC_NTF_PRECLMP_EN     0x429
#define CX2684x_AFEA_CH1_ADC_QGAIN_RES_TRM      0x42A
#define CX2684x_AFEA_CH1_ADC_SOC_PRECLMP_TERM   0x42B

#define CX2684x_AFEA_CH2_ADC_STATUS             0x440
#define CX2684x_AFEA_CH2_ADC_STATUS2            0x441
#define CX2684x_AFEA_CH2_ADC_CAL_ATEST          0x442
#define CX2684x_AFEA_CH2_ADC_PWRDN_CLAMP        0x443
#define CX2684x_AFEA_CH2_ADC_CTRL_DAC23         0x444
#define CX2684x_AFEA_CH2_ADC_CTRL_DAC1          0x445
#define CX2684x_AFEA_CH2_ADC_DCSERVO_DEM        0x446
#define CX2684x_AFEA_CH2_ADC_FB_FRCRST          0x447
#define CX2684x_AFEA_CH2_ADC_INPUT              0x448
#define CX2684x_AFEA_CH2_ADC_NTF_PRECLMP_EN     0x449
#define CX2684x_AFEA_CH2_ADC_QGAIN_RES_TRM      0x44A
#define CX2684x_AFEA_CH2_ADC_SOC_PRECLMP_TERM   0x44B

#define CX2684x_AFEA_CH3_ADC_STATUS             0x460
#define CX2684x_AFEA_CH3_ADC_STATUS2            0x461
#define CX2684x_AFEA_CH3_ADC_CAL_ATEST          0x462
#define CX2684x_AFEA_CH3_ADC_PWRDN_CLAMP        0x463
#define CX2684x_AFEA_CH3_ADC_CTRL_DAC23         0x464
#define CX2684x_AFEA_CH3_ADC_CTRL_DAC1          0x465
#define CX2684x_AFEA_CH3_ADC_DCSERVO_DEM        0x466
#define CX2684x_AFEA_CH3_ADC_FB_FRCRST          0x467
#define CX2684x_AFEA_CH3_ADC_INPUT              0x468
#define CX2684x_AFEA_CH3_ADC_NTF_PRECLMP_EN     0x469
#define CX2684x_AFEA_CH3_ADC_QGAIN_RES_TRM      0x46A
#define CX2684x_AFEA_CH3_ADC_SOC_PRECLMP_TERM   0x46B

#define CX2684x_AFEA_CH4_ADC_STATUS             0x480
#define CX2684x_AFEA_CH4_ADC_STATUS2            0x481
#define CX2684x_AFEA_CH4_ADC_CAL_ATEST          0x482
#define CX2684x_AFEA_CH4_ADC_PWRDN_CLAMP        0x483
#define CX2684x_AFEA_CH4_ADC_CTRL_DAC23         0x484
#define CX2684x_AFEA_CH4_ADC_CTRL_DAC1          0x485
#define CX2684x_AFEA_CH4_ADC_DCSERVO_DEM        0x486
#define CX2684x_AFEA_CH4_ADC_FB_FRCRST          0x487
#define CX2684x_AFEA_CH4_ADC_INPUT              0x488
#define CX2684x_AFEA_CH4_ADC_NTF_PRECLMP_EN     0x489
#define CX2684x_AFEA_CH4_ADC_QGAIN_RES_TRM      0x48A
#define CX2684x_AFEA_CH4_ADC_SOC_PRECLMP_TERM   0x48B
    
#define CX2684x_AFEB_SUP_BLK_TUNE1              0x500
#define CX2684x_AFEB_SUP_BLK_TUNE2              0x501
#define CX2684x_AFEB_SUP_BLK_TUNE3              0x502
#define CX2684x_AFEB_SUP_BLK_XTAL               0x503
#define CX2684x_AFEB_SUP_BLK_PLL                0x504
#define CX2684x_AFEB_SUP_BLK_PLL2               0x505
#define CX2684x_AFEB_SUP_BLK_PLL3               0x506
#define CX2684x_AFEB_SUP_BLK_REF                0x507
#define CX2684x_AFEB_SUP_BLK_PWRDN              0x508
#define CX2684x_AFEB_SUP_BLK_TESTPAD            0x509
#define CX2684x_AFEB_ADC_INT5_STAB_REF          0x50A
#define CX2684x_AFEB_ADC_COM_QUANT              0x50B
#define CX2684x_AFEB_ADC_COM_BIAS1              0x50C
#define CX2684x_AFEB_ADC_COM_BIAS2              0x50D
#define CX2684x_AFEB_ADC_COM_BIAS3              0x50E
    
#define CX2684x_AFEB_CH1_ADC_STATUS             0x520
#define CX2684x_AFEB_CH1_ADC_STATUS2            0x521
#define CX2684x_AFEB_CH1_ADC_CAL_ATEST          0x522
#define CX2684x_AFEB_CH1_ADC_PWRDN_CLAMP        0x523
#define CX2684x_AFEB_CH1_ADC_CTRL_DAC23         0x524
#define CX2684x_AFEB_CH1_ADC_CTRL_DAC1          0x525
#define CX2684x_AFEB_CH1_ADC_DCSERVO_DEM        0x526
#define CX2684x_AFEB_CH1_ADC_FB_FRCRST          0x527
#define CX2684x_AFEB_CH1_ADC_INPUT              0x528
#define CX2684x_AFEB_CH1_ADC_NTF_PRECLMP_EN     0x529
#define CX2684x_AFEB_CH1_ADC_QGAIN_RES_TRM      0x52A
#define CX2684x_AFEB_CH1_ADC_SOC_PRECLMP_TERM   0x52B
    
#define CX2684x_AFEB_CH2_ADC_STATUS             0x540
#define CX2684x_AFEB_CH2_ADC_STATUS2            0x541
#define CX2684x_AFEB_CH2_ADC_CAL_ATEST          0x542
#define CX2684x_AFEB_CH2_ADC_PWRDN_CLAMP        0x543
#define CX2684x_AFEB_CH2_ADC_CTRL_DAC23         0x544
#define CX2684x_AFEB_CH2_ADC_CTRL_DAC1          0x545
#define CX2684x_AFEB_CH2_ADC_DCSERVO_DEM        0x546
#define CX2684x_AFEB_CH2_ADC_FB_FRCRST          0x547
#define CX2684x_AFEB_CH2_ADC_INPUT              0x548
#define CX2684x_AFEB_CH2_ADC_NTF_PRECLMP_EN     0x549
#define CX2684x_AFEB_CH2_ADC_QGAIN_RES_TRM      0x54A
#define CX2684x_AFEB_CH2_ADC_SOC_PRECLMP_TERM   0x54B
    
#define CX2684x_AFEB_CH3_ADC_STATUS             0x560
#define CX2684x_AFEB_CH3_ADC_STATUS2            0x561
#define CX2684x_AFEB_CH3_ADC_CAL_ATEST          0x562
#define CX2684x_AFEB_CH3_ADC_PWRDN_CLAMP        0x563
#define CX2684x_AFEB_CH3_ADC_CTRL_DAC23         0x564
#define CX2684x_AFEB_CH3_ADC_CTRL_DAC1          0x565
#define CX2684x_AFEB_CH3_ADC_DCSERVO_DEM        0x566
#define CX2684x_AFEB_CH3_ADC_FB_FRCRST          0x567
#define CX2684x_AFEB_CH3_ADC_INPUT              0x568
#define CX2684x_AFEB_CH3_ADC_NTF_PRECLMP_EN     0x569
#define CX2684x_AFEB_CH3_ADC_QGAIN_RES_TRM      0x56A
#define CX2684x_AFEB_CH3_ADC_SOC_PRECLMP_TERM   0x56B

#define CX2684x_AFEB_CH4_ADC_STATUS             0x580
#define CX2684x_AFEB_CH4_ADC_STATUS2            0x581
#define CX2684x_AFEB_CH4_ADC_CAL_ATEST          0x582
#define CX2684x_AFEB_CH4_ADC_PWRDN_CLAMP        0x583
#define CX2684x_AFEB_CH4_ADC_CTRL_DAC23         0x584
#define CX2684x_AFEB_CH4_ADC_CTRL_DAC1          0x585
#define CX2684x_AFEB_CH4_ADC_DCSERVO_DEM        0x586
#define CX2684x_AFEB_CH4_ADC_FB_FRCRST          0x587
#define CX2684x_AFEB_CH4_ADC_INPUT              0x588
#define CX2684x_AFEB_CH4_ADC_NTF_PRECLMP_EN     0x589
#define CX2684x_AFEB_CH4_ADC_QGAIN_RES_TRM      0x58A
#define CX2684x_AFEB_CH4_ADC_SOC_PRECLMP_TERM   0x58B

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CX26848_COLIBRI_REGISTERS__ */
