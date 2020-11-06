/*+++ *******************************************************************\
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
\******************************************************************* ---*/

#include "Cx26848ColibriRegisters.h"
#include "Cx26848Colibri.h"

/* APIs */
/*******************************************************************************************************
 * CX2684x_initColibri()
 *
 *******************************************************************************************************/
int CX2684x_initColibri(const CX_COMMUNICATION *p_comm, const unsigned char freq_clk)
{
    int result = TRUE;

    result &= writeByte(p_comm, CX2684x_AFEA_SUP_BLK_TUNE1, 0xEC);  /* Force tuning, cap_sel_in = 0x14 */
    result &= writeByte(p_comm, CX2684x_AFEA_SUP_BLK_TUNE2, 0x20);  /* Set TUNE1,TUNE2 ref_count = 0x1F8 */

    // Setup clocks and tune analog filter
    result &= writeByte(p_comm, CX2684x_AFEA_SUP_BLK_XTAL, 0x0F);
    result &= writeByte(p_comm, CX2684x_AFEA_SUP_BLK_PLL, 0x3B);
    result &= writeByte(p_comm, CX2684x_AFEA_SUP_BLK_PLL2, freq_clk);   /* 0x0F for 720MHz, 0x0D for 624MHz */
    result &= writeByte(p_comm, CX2684x_AFEA_SUP_BLK_PLL3, 0x67);
    result &= writeByte(p_comm, CX2684x_AFEA_SUP_BLK_REF, 0x1b);   /* default value (bit[4:3]ref_adc=0x2), Set ref_adc = 0x3 to get maximum input amplitude */

    result &= writeByte(p_comm, CX2684x_AFEA_SUP_BLK_PWRDN, 0x18);
    result &= writeByte(p_comm, CX2684x_AFEA_SUP_BLK_TUNE3, 0x40);

    result &= writeByte(p_comm, CX2684x_AFEB_SUP_BLK_TUNE1, 0xEC); /* Force tuning, cap_sel_in = 0x14 */
    result &= writeByte(p_comm, CX2684x_AFEB_SUP_BLK_TUNE2, 0x20); /* Set TUNE1,TUNE2 Ref_count=0x1F8 */

    result &= writeByte(p_comm, CX2684x_AFEB_SUP_BLK_XTAL, 0x0F);
    result &= writeByte(p_comm, CX2684x_AFEB_SUP_BLK_PLL, 0x3B);
    result &= writeByte(p_comm, CX2684x_AFEB_SUP_BLK_PLL2, freq_clk);   /* 0x0F for 720MHz, 0x0D for 624MHz */
    result &= writeByte(p_comm, CX2684x_AFEB_SUP_BLK_PLL3, 0x67);
    result &= writeByte(p_comm, CX2684x_AFEB_SUP_BLK_REF, 0x1b);    /* ref_adc = 3 to get maximum input amplitude*/

    result &= writeByte(p_comm, CX2684x_AFEB_SUP_BLK_PWRDN, 0x18);
    result &= writeByte(p_comm, CX2684x_AFEB_SUP_BLK_TUNE3, 0x40);

    p_comm->sleep(3);

    result &= writeByte(p_comm, CX2684x_AFEA_SUP_BLK_TUNE3, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEA_ADC_INT5_STAB_REF, 0x52); /* Set int5_boost = 0 */

    result &= writeByte(p_comm, CX2684x_AFEB_SUP_BLK_TUNE3, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEB_ADC_INT5_STAB_REF, 0x52); /* Set int5_boost = 0 */


    /* power up all channels, clear pd_buffer */
    result &= writeByte(p_comm, CX2684x_AFEA_CH1_ADC_PWRDN_CLAMP,  0x00);
    result &= writeByte(p_comm, CX2684x_AFEA_CH2_ADC_PWRDN_CLAMP,  0x00);
    result &= writeByte(p_comm, CX2684x_AFEA_CH3_ADC_PWRDN_CLAMP,  0x00);
    result &= writeByte(p_comm, CX2684x_AFEA_CH4_ADC_PWRDN_CLAMP,  0x00);

    result &= writeByte(p_comm, CX2684x_AFEB_CH1_ADC_PWRDN_CLAMP,  0x00);
    result &= writeByte(p_comm, CX2684x_AFEB_CH2_ADC_PWRDN_CLAMP,  0x00);
    result &= writeByte(p_comm, CX2684x_AFEB_CH3_ADC_PWRDN_CLAMP,  0x00);
    result &= writeByte(p_comm, CX2684x_AFEB_CH4_ADC_PWRDN_CLAMP,  0x00);

    /* Enable channel calibration */
    result &= writeByte(p_comm, CX2684x_AFEA_ADC_COM_QUANT, 0x02);
    result &= writeByte(p_comm, CX2684x_AFEB_ADC_COM_QUANT, 0x02);

    /* force modulator reset */
    result &= writeByte(p_comm, CX2684x_AFEA_CH1_ADC_FB_FRCRST,  0x14);
    result &= writeByte(p_comm, CX2684x_AFEA_CH2_ADC_FB_FRCRST,  0x14);
    result &= writeByte(p_comm, CX2684x_AFEA_CH3_ADC_FB_FRCRST,  0x14);
    result &= writeByte(p_comm, CX2684x_AFEA_CH4_ADC_FB_FRCRST,  0x14);

    result &= writeByte(p_comm, CX2684x_AFEB_CH1_ADC_FB_FRCRST,  0x14);
    result &= writeByte(p_comm, CX2684x_AFEB_CH2_ADC_FB_FRCRST,  0x14);
    result &= writeByte(p_comm, CX2684x_AFEB_CH3_ADC_FB_FRCRST,  0x14);
    result &= writeByte(p_comm, CX2684x_AFEB_CH4_ADC_FB_FRCRST,  0x14);

    p_comm->sleep(3);

    /* start quantizer calibration */
    result &= writeByte(p_comm, CX2684x_AFEA_CH1_ADC_CAL_ATEST, 0x10);
    result &= writeByte(p_comm, CX2684x_AFEA_CH2_ADC_CAL_ATEST, 0x10);
    result &= writeByte(p_comm, CX2684x_AFEA_CH3_ADC_CAL_ATEST, 0x10);
    result &= writeByte(p_comm, CX2684x_AFEA_CH4_ADC_CAL_ATEST, 0x10);

    result &= writeByte(p_comm, CX2684x_AFEB_CH1_ADC_CAL_ATEST, 0x10);
    result &= writeByte(p_comm, CX2684x_AFEB_CH2_ADC_CAL_ATEST, 0x10);
    result &= writeByte(p_comm, CX2684x_AFEB_CH3_ADC_CAL_ATEST, 0x10);
    result &= writeByte(p_comm, CX2684x_AFEB_CH4_ADC_CAL_ATEST, 0x10);

    p_comm->sleep(3);

    /* exit modulator (fb) reset */
    result &= writeByte(p_comm, CX2684x_AFEA_CH1_ADC_FB_FRCRST, 0x04);
    result &= writeByte(p_comm, CX2684x_AFEA_CH2_ADC_FB_FRCRST, 0x04);
    result &= writeByte(p_comm, CX2684x_AFEA_CH3_ADC_FB_FRCRST, 0x04);
    result &= writeByte(p_comm, CX2684x_AFEA_CH4_ADC_FB_FRCRST, 0x04);

    result &= writeByte(p_comm, CX2684x_AFEB_CH1_ADC_FB_FRCRST, 0x04);
    result &= writeByte(p_comm, CX2684x_AFEB_CH2_ADC_FB_FRCRST, 0x04);
    result &= writeByte(p_comm, CX2684x_AFEB_CH3_ADC_FB_FRCRST, 0x04);
    result &= writeByte(p_comm, CX2684x_AFEB_CH4_ADC_FB_FRCRST, 0x04);

    /* Setup each channel as single-ended input */
    result &= writeByte(p_comm, CX2684x_AFEA_CH1_ADC_NTF_PRECLMP_EN, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEA_CH2_ADC_NTF_PRECLMP_EN, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEA_CH3_ADC_NTF_PRECLMP_EN, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEA_CH4_ADC_NTF_PRECLMP_EN, 0x00);

    result &= writeByte(p_comm, CX2684x_AFEB_CH1_ADC_NTF_PRECLMP_EN, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEB_CH2_ADC_NTF_PRECLMP_EN, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEB_CH3_ADC_NTF_PRECLMP_EN, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEB_CH4_ADC_NTF_PRECLMP_EN, 0x00);

    result &= writeByte(p_comm, CX2684x_AFEA_CH1_ADC_INPUT, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEA_CH2_ADC_INPUT, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEA_CH3_ADC_INPUT, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEA_CH4_ADC_INPUT, 0x00);

    result &= writeByte(p_comm, CX2684x_AFEB_CH1_ADC_INPUT, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEB_CH2_ADC_INPUT, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEB_CH3_ADC_INPUT, 0x00);
    result &= writeByte(p_comm, CX2684x_AFEB_CH4_ADC_INPUT, 0x00);

    result &= writeByte(p_comm, CX2684x_AFEA_CH1_ADC_DCSERVO_DEM, 0x03);
    result &= writeByte(p_comm, CX2684x_AFEA_CH2_ADC_DCSERVO_DEM, 0x03);
    result &= writeByte(p_comm, CX2684x_AFEA_CH3_ADC_DCSERVO_DEM, 0x03);
    result &= writeByte(p_comm, CX2684x_AFEA_CH4_ADC_DCSERVO_DEM, 0x03);

    result &= writeByte(p_comm, CX2684x_AFEB_CH1_ADC_DCSERVO_DEM, 0x03);
    result &= writeByte(p_comm, CX2684x_AFEB_CH2_ADC_DCSERVO_DEM, 0x03);
    result &= writeByte(p_comm, CX2684x_AFEB_CH3_ADC_DCSERVO_DEM, 0x03);
    result &= writeByte(p_comm, CX2684x_AFEB_CH4_ADC_DCSERVO_DEM, 0x03);

    /* dac23_gain = 0x3 to get maximum input amplitude */
    result &= writeByte(p_comm, CX2684x_AFEA_CH1_ADC_CTRL_DAC1 , 0x06);
    result &= writeByte(p_comm, CX2684x_AFEA_CH2_ADC_CTRL_DAC1 , 0x06);
    result &= writeByte(p_comm, CX2684x_AFEA_CH3_ADC_CTRL_DAC1 , 0x06);
    result &= writeByte(p_comm, CX2684x_AFEA_CH4_ADC_CTRL_DAC1 , 0x06);

    result &= writeByte(p_comm, CX2684x_AFEB_CH1_ADC_CTRL_DAC1 , 0x06);
    result &= writeByte(p_comm, CX2684x_AFEB_CH2_ADC_CTRL_DAC1 , 0x06);
    result &= writeByte(p_comm, CX2684x_AFEB_CH3_ADC_CTRL_DAC1 , 0x06);
    result &= writeByte(p_comm, CX2684x_AFEB_CH4_ADC_CTRL_DAC1 , 0x06);

    result &= writeByte(p_comm, CX2684x_AFEA_CH1_ADC_CTRL_DAC23 , 0x24);
    result &= writeByte(p_comm, CX2684x_AFEA_CH2_ADC_CTRL_DAC23 , 0x24);
    result &= writeByte(p_comm, CX2684x_AFEA_CH3_ADC_CTRL_DAC23 , 0x24);
    result &= writeByte(p_comm, CX2684x_AFEA_CH4_ADC_CTRL_DAC23 , 0x24);
    result &= writeByte(p_comm, CX2684x_AFEB_CH1_ADC_CTRL_DAC23 , 0x24);
    result &= writeByte(p_comm, CX2684x_AFEB_CH2_ADC_CTRL_DAC23 , 0x24);
    result &= writeByte(p_comm, CX2684x_AFEB_CH3_ADC_CTRL_DAC23 , 0x24);
    result &= writeByte(p_comm, CX2684x_AFEB_CH4_ADC_CTRL_DAC23 , 0x24);

    /* use diode instead of resistor, so set term_en to 0, res_en to 0 */
    result &= writeByte(p_comm, CX2684x_AFEA_CH1_ADC_QGAIN_RES_TRM , 0x02);
    result &= writeByte(p_comm, CX2684x_AFEA_CH2_ADC_QGAIN_RES_TRM , 0x02);
    result &= writeByte(p_comm, CX2684x_AFEA_CH3_ADC_QGAIN_RES_TRM , 0x02);
    result &= writeByte(p_comm, CX2684x_AFEA_CH4_ADC_QGAIN_RES_TRM , 0x02);

    result &= writeByte(p_comm, CX2684x_AFEB_CH1_ADC_QGAIN_RES_TRM , 0x02);
    result &= writeByte(p_comm, CX2684x_AFEB_CH2_ADC_QGAIN_RES_TRM , 0x02);
    result &= writeByte(p_comm, CX2684x_AFEB_CH3_ADC_QGAIN_RES_TRM , 0x02);
    result &= writeByte(p_comm, CX2684x_AFEB_CH4_ADC_QGAIN_RES_TRM , 0x02);

    return result;
}
