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
	File:		Perceptual_cfg.h

	Content:	Psychoaccoustic configuration structure and functions

*******************************************************************************/

#ifndef _perceptual_cfg_h_
#define _perceptual_cfg_h_

#include "typedefs.h"
#include "perceptual_const.h"
#include "tns_tool.h"

typedef struct{

  int16_t sfbActive;   /* number of sf bands containing energy after lowpass */
  int16_t sfbCnt;  
  const int16_t *sfbOffset;

  int32_t sfbThresholdQuiet[MAX_SFB_LONG];

  int16_t maxAllowedIncreaseFactor;   /* preecho control */
  int16_t minRemainingThresholdFactor;

  int16_t lowpassLine;
  int16_t sampRateIdx;
  int32_t clipEnergy;                 /* for level dependend tmn */

  int16_t ratio;
  int16_t sfbMaskHighFactor[MAX_SFB_LONG];
  int16_t sfbMaskLowFactor[MAX_SFB_LONG];
  
  int16_t sfbMaskHighFactorSprEn[MAX_SFB_LONG];
  int16_t sfbMaskLowFactorSprEn[MAX_SFB_LONG];

  int16_t sfbMinSnr[MAX_SFB_LONG];       /* minimum snr (formerly known as bmax) */

  GM_TNS_CFG_INFO tnsConf;

}GM_PSY_CFG_LONGBLK; /*int16_t size: 8 + 52 + 102 + 51 + 51 + 51 + 51 + 47 = 515 */


typedef struct{
  int16_t sfbActive;   /* number of sf bands containing energy after lowpass */
  int16_t sfbCnt;  
  const int16_t *sfbOffset;

  int32_t sfbThresholdQuiet[MAX_SFB_SHORT];

  int16_t minRemainingThresholdFactor;
  int16_t maxAllowedIncreaseFactor;   /* preecho control */  
  
  int32_t clipEnergy;                 /* for level dependend tmn */
  int16_t lowpassLine;
  int16_t sampRateIdx;  

  int16_t ratio;
  int16_t sfbMaskLowFactor[MAX_SFB_SHORT];
  int16_t sfbMaskHighFactor[MAX_SFB_SHORT];

  int16_t sfbMaskLowFactorSprEn[MAX_SFB_SHORT];
  int16_t sfbMaskHighFactorSprEn[MAX_SFB_SHORT];

  int16_t sfbMinSnr[MAX_SFB_SHORT];       /* minimum snr (formerly known as bmax) */

  GM_TNS_CFG_INFO tnsConf;

}GM_PSY_CFG_SHORTBLK; /*int16_t size: 8 + 16 + 16 + 16 + 16 + 16 + 16 + 16 + 47 = 167 */


/* Returns the sample rate index */
int32_t Get_SampleRateIndex(int32_t sampleRate);


int16_t Perce_InitCfgLngBlk(int32_t bitrate,
                                int32_t samplerate,
                                int16_t bandwidth,
                                GM_PSY_CFG_LONGBLK *psyConf);

int16_t Psy_InitCfgShortBlk(int32_t bitrate,
                                 int32_t samplerate,
                                 int16_t bandwidth,
                                 GM_PSY_CFG_SHORTBLK *psyConf);

#endif /* _perceptual_cfg_h_ */



