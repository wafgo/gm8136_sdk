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
	File:		tns_tool.h

	Content:	TNS structures

*******************************************************************************/

#ifndef _tns_tool_h
#define _tns_tool_h

#include "typedef.h"
#include "perceptual_const.h"



#define TNS_MAX_ORDER 12
#define TNS_MAX_ORDER_SHORT 5

#define FILTER_DIRECTION    0

typedef struct{ /*stuff that is tabulated dependent on bitrate etc. */
  int16_t     threshOn;                /* min. prediction gain for using tns TABUL * 100*/
  int32_t     lpcStartFreq;            /* lowest freq for lpc TABUL*/
  int32_t     lpcStopFreq;             /* TABUL */
  int32_t     tnsTimeResolution;
}GM_TNS_CFG_TABULATED;


typedef struct {   /*assigned at InitTime*/
  int16_t tnsActive;
  int16_t tnsMaxSfb;

  int16_t maxOrder;                /* max. order of tns filter */
  int16_t tnsStartFreq;            /* lowest freq. for tns filtering */
  int16_t coefRes;

  GM_TNS_CFG_TABULATED confTab;

  int32_t acfWindow[TNS_MAX_ORDER+1];

  int16_t tnsStartBand;
  int16_t tnsStartLine;

  int16_t tnsStopBand;
  int16_t tnsStopLine;

  int16_t lpcStartBand;
  int16_t lpcStartLine;

  int16_t lpcStopBand;
  int16_t lpcStopLine;

  int16_t tnsRatioPatchLowestCb;
  int16_t tnsModifyBeginCb;

  int16_t threshold; /* min. prediction gain for using tns TABUL * 100 */

}GM_TNS_CFG_INFO;


typedef struct {
  int16_t tnsActive;
  int32_t parcor[TNS_MAX_ORDER];
  int16_t predictionGain;
} GM_TNS_SUBBLK_INFO; /* int16_t size: 26 */

typedef struct{
  GM_TNS_SUBBLK_INFO subBlockInfo[TRANS_FAC];
} GM_TNS_SHORTBLK_INFO;

typedef struct{
  GM_TNS_SUBBLK_INFO subBlockInfo;
} GM_TNS_LONGBLK_INFO;

typedef struct{
  GM_TNS_SHORTBLK_INFO tnsShort;	
  GM_TNS_LONGBLK_INFO tnsLong;  
}GM_TNS_DATABLK_INFO;

typedef struct{
  GM_TNS_DATABLK_INFO dataRaw;
  int16_t numOfSubblocks;  
}GM_TNS_PART_DATA; /* int16_t size: 1 + 8*26 + 26 = 235 */

typedef struct{
  int16_t order[TRANS_FAC];
  int16_t coef[TRANS_FAC*TNS_MAX_ORDER_SHORT];
  int16_t tnsActive[TRANS_FAC];
  int16_t coefRes[TRANS_FAC];
  int16_t length[TRANS_FAC];  
}GM_TNS_BLK_INFO; /* int16_t size: 72 */




#endif /* _tns_tool_h */
