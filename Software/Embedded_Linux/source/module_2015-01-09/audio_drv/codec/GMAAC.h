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



/*
	File:			GMAAC.h
	Description:	AAC codec APIs & data types

*/

#ifndef __GMAAC_H__
#define __GMAAC_H__


#include "AACAudio.h"
#include "typedef.h"

/* AAC Param ID */
#define GMID_AAC_Mdoule				0x42211000
#define GMID_AAC_ENCPARAM			GMID_AAC_Mdoule | 0x0040  /*!< get/set AAC encoder parameter, the parameter is a pointer to GM_AAC_INSPEC */

//int32_t GetAACEncAPI (GM_AUDIO_TOPAPI * pEncHandle);

uint32_t GMAACEncInit(AAC_HANDLE * phCodec);
uint32_t GMAACEncSetInputData(AAC_HANDLE hCodec, GM_AUDIO_BUF * pInput);
uint32_t GMAACEncGetOutputData(AAC_HANDLE hCodec, GM_AUDIO_BUF * pOutput, GM_AUDIO_OUTSPEC * pOutInfo);
uint32_t GMAACEncDestory(AAC_HANDLE hCodec);

uint32_t GMAACEncGetParam(AAC_HANDLE hCodec, int32_t uParamID, AAC_PTR pData);

#endif // __GMAAC_H__



