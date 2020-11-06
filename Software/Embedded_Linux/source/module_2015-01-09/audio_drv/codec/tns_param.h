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
	File:		tns_param.h

	Content:	TNS parameters

*******************************************************************************/

/*
   TNS parameters
 */
#ifndef _TNS_PARAM_H
#define _TNS_PARAM_H

#include "tns_tool.h"

typedef struct{
  int32_t samplingRate;
  int16_t maxBandLong;
  int16_t maxBandShort;
}TNS_MAX_TAB_ENTRY;

typedef struct{
    int32_t bitRateFrom;
    int32_t bitRateTo;
    const GM_TNS_CFG_TABULATED *paramMono_Long;  /* contains TNS parameters */
    const GM_TNS_CFG_TABULATED *paramMono_Short;
    const GM_TNS_CFG_TABULATED *paramStereo_Long;
    const GM_TNS_CFG_TABULATED *paramStereo_Short;
}TNS_INFO_TAB;


void GetTnsParam(GM_TNS_CFG_TABULATED *tnsConfigTab,
                 int32_t bitRate, int16_t channels, int16_t blockType);

void GetTnsMaxBands(int32_t samplingRate, int16_t blockType, int16_t* tnsMaxSfb);

#endif /* _TNS_PARAM_H */
