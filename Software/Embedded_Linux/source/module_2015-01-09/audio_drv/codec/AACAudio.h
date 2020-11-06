
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


#ifndef __Audio_H__
#define __Audio_H__

#include "typedefs.h"

typedef struct
{
    int32_t		SampleBits;  /*!< Bits per sample */
	int32_t		SampleRate;  /*!< Sample rate */
	int32_t		Channels;    /*!< Channel count */	
} GM_AUDIO_SPEC;

typedef struct
{
	GM_AUDIO_SPEC		Format;			/*!< Sample rate */
	uint32_t			InputUsed;		/*!< Channel count */ //useLeng
	uint32_t			Resever;		/*!< Resevered */
} GM_AUDIO_OUTSPEC;
/*
typedef struct GM_AUDIO_TOPAPI
{
	uint32_t (* Init) (AAC_HANDLE * phCodec);									// initial AAC encoder	
	uint32_t (* SetParament) (AAC_HANDLE hCodec, int32_t uParamID, AAC_PTR pData);
	uint32_t (* GetParament) (AAC_HANDLE hCodec, int32_t uParamID, AAC_PTR pData);
	uint32_t (* SetInputData) (AAC_HANDLE hCodec, GM_AUDIO_BUF * pInput);	   // set input data
	uint32_t (* GetOutputData) (AAC_HANDLE hCodec, GM_AUDIO_BUF * pOutBuffer, GM_AUDIO_OUTSPEC * pOutInfo);
	uint32_t (* Destory) (AAC_HANDLE hCodec);
} GM_AUDIO_TOPAPI;
*/

#endif // __Audio_H__
