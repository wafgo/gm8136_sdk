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
	File:		adj_thr_data.h

	Content:	Threshold compensation parameter

*******************************************************************************/

#ifndef __ADJ_THR_DATA_H
#define __ADJ_THR_DATA_H

#include "typedef.h"
#include "perceptual_const.h"
//#include "line_pe.h"

typedef struct {
   int16_t clipSpendLow, clipSpendHigh;
   int16_t minBitSpend, maxBitSpend;
   int16_t clipSaveLow, clipSaveHigh;
   int16_t minBitSave, maxBitSave;   
} GM_BITRESER_ARGUM;

typedef struct {
   uint8_t modifyMinSnr;
   int16_t startSfbL, startSfbS;
} GM_AVOIDHOLE_ARGUM;

typedef struct {
  int32_t redRatioFac;	
  int32_t maxRed;
  int32_t startRatio, maxRatio;  
  int32_t redOffs;
} GM_MINSNR_ADAPT_ARGUM;

typedef struct {
  /* parameters for bitreservoir control */
  int16_t peMin, peMax;
  /* constant offset to pe               */
  int16_t  peOffset;
  /* avoid hole parameters               */
  GM_AVOIDHOLE_ARGUM ahParam;
  /* paramters for adaptation of minSnr */
  GM_MINSNR_ADAPT_ARGUM minSnrAdaptParam;
  /* values for correction of pe */  
  int16_t dynBitsLast;
  int16_t peCorrectionFactor;
  int16_t peLast; 
} GM_ATS_PART;

typedef struct {	
  GM_BITRESER_ARGUM   bresParamLong, bresParamShort; /* int16_t size: 2*8 */
  GM_ATS_PART  adjThrStateElem;               /* int16_t size: 19 */
} GM_ADJ_THR_BLK;

#endif
