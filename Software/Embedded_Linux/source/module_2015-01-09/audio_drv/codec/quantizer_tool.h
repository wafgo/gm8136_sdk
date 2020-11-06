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
	File:		quantizer_tool.h

	Content:	Quantization functions

*******************************************************************************/

#ifndef _quantizer_tool_h
#define _quantizer_tool_h
#include "typedefs.h"

/* quantizing */

#define MAX_QUANT 8191

void QuantScfBandsSpectr(int16_t sfbCnt,
                      int16_t maxSfbPerGroup,
                      int16_t sfbPerGroup,
                      int16_t *sfbOffset, int32_t *mdctSpectrum,
                      int16_t globalGain, int16_t *scalefactors,
                      int16_t *quantizedSpectrum);

int32_t Quant_Line2Distor(const int32_t *spec,
                   int16_t  sfbWidth,
                   int16_t  gain);
                   
#endif /* _quantizer_tool_h */
