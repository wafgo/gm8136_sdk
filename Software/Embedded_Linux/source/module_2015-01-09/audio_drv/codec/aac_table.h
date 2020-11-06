/*
***********************************************************************************
** FARADAY(MTD)/GRAIN CONFIDENTIAL
** Copyright FARADAY(MTD)/GRAIN Corporation All Rights Reserved.
**
** The source code contained or described herein and all documents related to
** the source code (Material) are owned by FARADAY(MTD)/GRAIN Corporation Title to the 
** Material remains with FARADAY(MTD)/GRAIN Corporation. The Material contains trade 
** secrets and proprietary and confidential information of FARADAY(MTD)/GRAIN. The 
** Material is protected by worldwide copyright and trade secret laws and treaty 
** provisions. No part of the Material may be used, copied, reproduced, modified, 
** published, uploaded, posted, transmitted, distributed, or disclosed in any way 
** without FARADAY(MTD)/GRAIN prior express written permission.
** 
** No license under any patent, copyright, trade secret or other intellectual 
** property right is granted to or conferred upon you by disclosure or delivery
** of the Materials, either expressly, by implication, inducement, estoppel or 
** otherwise. Any license under such intellectual property rights must be express
** and approved by FARADAY(MTD)/GRAIN in writing.
**
** 
***********************************************************************/

/*******************************************************************************
	File:		aac_table.h

	Content:	constant tables

*******************************************************************************/

#ifndef _aac_table_h
#define _aac_table_h

#include "config.h"
#include "perceptual_const.h"
#include "tns_param.h"

/*
  mdct
*/
extern  int ShortWindowSine[FRAME_LEN_SHORT/2]; //64
extern  int LongWindowKBD[FRAME_LEN_LONG/2]; //512

//extern const unsigned char bitrevTab[17 + 129];//jk
//extern const int cossintab[128 + 1024]; //jk

extern  int twidTab64[4*6 + 16*6];
extern  int twidTab512[8*6 + 32*6 + 128*6];

extern  int cossintab_128[128];
extern  int cossintab_1024[1024];

extern  unsigned char bitrevTab_17[17];
extern  unsigned char bitrevTab_129[129];

/*
  form factor
*/
extern  int32_t formfac_sqrttable[96];

/*
  quantizer
*/
extern  int32_t mTab_3_4[512];
extern  int32_t mTab_4_3[512];
/*! $2^{-\frac{n}{16}}$ table */
extern  int16_t pow2tominusNover16[17] ;

extern int32_t specExpMantTableComb_enc[4][14];
extern  uint8_t specExpTableComb_enc[4][14];

extern  int16_t quantBorders[4][4];
//extern const int16_t quantRecon[3][4];
extern  int16_t quantRecon[4][3];

/*
  huffman
*/
extern  uint16_t huff_ltab1_2[3][3][3][3];
extern  uint16_t huff_ltab3_4[3][3][3][3];
extern  uint16_t huff_ltab5_6[9][9];
extern  uint16_t huff_ltab7_8[8][8];
extern  uint16_t huff_ltab9_10[13][13];
extern  uint16_t huff_ltab11[17][17];
extern  uint16_t huff_ltabscf[121];
extern  uint16_t huff_ctab1[3][3][3][3];
extern  uint16_t huff_ctab2[3][3][3][3];
extern  uint16_t huff_ctab3[3][3][3][3];
extern  uint16_t huff_ctab4[3][3][3][3];
extern  uint16_t huff_ctab5[9][9];
extern  uint16_t huff_ctab6[9][9];
extern  uint16_t huff_ctab7[8][8];
extern  uint16_t huff_ctab8[8][8];
extern  uint16_t huff_ctab9[13][13];
extern  uint16_t huff_ctab10[13][13];
extern  uint16_t huff_ctab11[17][17];
extern  uint32_t huff_ctabscf[121];



/*
  misc
*/
extern  int sampRateTab[NUM_SAMPLE_RATES];
extern  int BandwithCoefTab[8][NUM_SAMPLE_RATES];
extern  int rates[8];
extern  uint8_t sfBandTotalShort[NUM_SAMPLE_RATES];
extern  uint8_t sfBandTotalLong[NUM_SAMPLE_RATES];
extern  int sfBandTabShortOffset[NUM_SAMPLE_RATES];
extern  short sfBandTabShort[76];
extern  int sfBandTabLongOffset[NUM_SAMPLE_RATES];
extern  short sfBandTabLong[325];

extern  int32_t m_log2_table[INT_BITS];

/*
  TNS
*/
extern  int32_t tnsCoeff3[8];
extern  int32_t tnsCoeff3Borders[8];
extern  int32_t tnsCoeff4[16];
extern  int32_t tnsCoeff4Borders[16];
extern  int32_t invSBF[24];
extern  int16_t sideInfoTabLong[MAX_SFB_LONG + 1];
extern  int16_t sideInfoTabShort[MAX_SFB_SHORT + 1];
#endif
