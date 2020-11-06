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
	File:			gmaac_api.h
	Description:	AAC codec APIs & data types

*/

#ifndef __GMAACENCAPI_H__
#define __GMAACENCAPI_H__


/*!
 * the structure for AAC encoder input parameter
 */
#include "audio_main.h"
 
#define ENC_MAX_BITRATE 192000
#define ENC_MIN_BITRATE 14500

//#define MaxMultiChannel 32

/*!
 * the structure for AAC encoder input parameter
 */
typedef  struct {
  int	  sampleRate;          /*! audio file sample rate */
  int	  bitRate;             /*! encoder bit rate in bits/sec */
  short   nChannels;		   /*! number of channels on input (1,2) */
  short   adtsUsed;			   /*! whether write adts header */
} GM_AAC_INSPEC;

typedef  struct {
  //int	  	sampleRate;          /*! audio file sample rate */
  //int	  	bitRate;             /*! encoder bit rate in bits/sec */
  //short   nChannels;		   /*! number of channels on input (1,2) */
  //short   adtsUsed;			   /*! whether write adts header */
  short		id;								/* ID = 0 for MPEG-4, ID = 1 for MPEG-2	*/
  char		version[16];
  int     MultiChannel;    /*number of MultiChannel      */
  void*   private_region[MaxMultiChannel];
} GMAAC_INFORMATION;

int AAC_Create(int id, GMAAC_INFORMATION *gm_aac, AUDIO_CODEC_SETTING *codec_setting, int MultiChannel);     
int AAC_Enc(short convertBuf[][2048], unsigned char outbuf[][2048], int *len, GMAAC_INFORMATION *gmaac_api);
int	AAC_Destory(GMAAC_INFORMATION *gmaac_api);

#endif // __GMAAC_H__



