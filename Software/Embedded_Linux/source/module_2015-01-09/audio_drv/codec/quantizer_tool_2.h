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
	File:		quantizer_tool_2.h

	Content:	Scale factor estimation functions

*******************************************************************************/

#ifndef _quantizer_tool_2_h
#define _quantizer_tool_2_h
/*
   Scale factor estimation
 */
#include "perceptual_const.h"
#include "perceptual_main.h"
#include "quantizer_data.h"

void
EstimFactAllChan(int16_t          logSfbFormFactor[MAX_CHAN][MAX_GROUPED_SFB],
               int16_t          sfbNRelevantLines[MAX_CHAN][MAX_GROUPED_SFB],
               int16_t          logSfbEnergy[MAX_CHAN][MAX_GROUPED_SFB],
               GM_PSY_OUT_CHANNEL psyOutChannel[MAX_CHAN],
               const int16_t    nChannels);

void
CalcScaleFactAllChan(GM_PSY_OUT_CHANNEL psyOutChannel[MAX_CHAN],
                     GM_QC_CHANNDATA_OUT  qcOutChannel[MAX_CHAN],
                     int16_t          logSfbEnergy[MAX_CHAN][MAX_GROUPED_SFB],
                     int16_t          logSfbFormFactor[MAX_CHAN][MAX_GROUPED_SFB],
                     int16_t          sfbNRelevantLines[MAX_CHAN][MAX_GROUPED_SFB],
                     const int16_t    nChannels);
#endif
