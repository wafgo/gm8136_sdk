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
	File:		mdct.h

	Content:	MDCT Transform functions

*******************************************************************************/

#ifndef  _enc_mdct_h
#define _enc_mdct_h

#include "typedef.h"

void MDCT_Trans(int16_t *mdctDelayBuffer,
                    int16_t *timeSignal,
                    int16_t chIncrement,     /*! channel increment */
                    int32_t *realOut,
                    int16_t *mdctScale,
                    int16_t windowSequence
                    );

#endif
