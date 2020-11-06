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
	File:		out_bitstrm.h

	Content:	Bitstream encoder structure and functions

*******************************************************************************/

#ifndef _out_bitstrm_h
#define _out_bitstrm_h

#include "quantizer_data.h"
#include "tns_tool.h"
//#include "channel_map.h"
#include "perceptual_main.h"

struct GM_BITSTREAM_INFO
{
  int16_t profile;
  int16_t nChannels;
  int32_t bitrate;
  int32_t sampleRate; 
};




int16_t OutEncoder_BitData (int id,
											 HANDLE_BIT_BUF hBitstream,
                       GM_PART_INFO elInfo,
                       GM_QC_OUTSTRM *qcOut,
                       GM_PERCEP_OUT *psyOut,
                       int16_t *globUsedBits,
                       const uint8_t *ancBytes,
					   int16_t samplerate
                       );

#endif /* _out_bitstrm_h */
