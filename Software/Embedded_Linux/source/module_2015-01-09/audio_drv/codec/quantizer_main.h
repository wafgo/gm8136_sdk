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
	File:		quantizer_main.h

	Content:	Quantizing & coding functions

*******************************************************************************/

#ifndef _quantizer_main_h
#define _quantizer_main_h

#include "quantizer_data.h"
#include "perceptual_main.h" 
#include "mem_manag.h"

/* Quantizing & coding stage */

int16_t QC_ParamMem_Init(GM_QC_OUTSTRM *hQC, int16_t nChannels);

void QC_ParamMem_Del(GM_QC_OUTSTRM *hQC);

int16_t QC_Mem0(GM_QC_BLK *hQC);

int16_t QuantCode_Init(GM_QC_BLK *hQC,
              struct GM_QC_INFO *init);

void QC_ParmSetNULL(GM_QC_BLK *hQC);


int16_t QC_Main(GM_QC_BLK *hQC,
              GM_ELEMENT_BITS* elBits,
              GM_ATS_PART* adjThrStateElement,
              GM_PSY_OUT_CHANNEL psyOutChannel[MAX_CHAN], /* may be modified in-place */
              GM_PERCEP_OUT_PART* psyOutElement,
              GM_QC_CHANNDATA_OUT  qcOutChannel[MAX_CHAN],   /* out                      */
              GM_QC_OUTSTRM_PART* qcOutElement,
              int16_t nChannels,
			  int16_t ancillaryDataBytes);     /* returns error code       */

void QC_WriteBitrese(GM_QC_BLK* qcKernel,
                  GM_QC_OUTSTRM* qcOut);

int16_t QC_WriteOutParam(GM_QC_BLK *hQC,
                              GM_QC_OUTSTRM* qcOut);

int16_t UpdateBitrate(GM_QC_BLK *hQC,
                     int32_t bitRate,
                     int32_t sampleRate);


int16_t Init_ChannelPartInfo (int16_t nChannels, GM_PART_INFO* elInfo);



#endif /* _quantizer_main_h */
