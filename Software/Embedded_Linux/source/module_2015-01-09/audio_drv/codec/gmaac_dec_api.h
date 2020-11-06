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
#include "faad.h"
#ifndef __GMAACDECAPI_H__
#define __GMAACDECAPI_H__
#include <linux/export.h>

//typedef void *GMAACDecHandle;
#define MAX_CHAN 6 // make this higher to support files withmore channels
#define DECODE_BUF_SIZE MAX_CHAN*FAAD_MIN_STREAMSIZE /* 6*768 = 4608 */

/* FAAD file buffering routines */
typedef struct {
    long bytes_into_buffer;
    long bytes_consumed;
    unsigned char *buffer;
} aac_buffer;

typedef struct GMAACDecInfo
{
    unsigned long bytesconsumed;
    unsigned long samples;
    unsigned char channels;
    unsigned char error;
    unsigned long samplerate;

    /* SBR: 0: off, 1: on; upsample, 2: on; downsampled, 3: off; upsampled */
    unsigned char sbr;

    /* MPEG-4 ObjectType */
    unsigned char object_type;

    /* AAC header type; MP4 will be signalled as RAW also */
    unsigned char header_type;

    /* multichannel configuration */
    unsigned char num_front_channels;
    unsigned char num_side_channels;
    unsigned char num_back_channels;
    unsigned char num_lfe_channels;
    unsigned char channel_position[64];
	float song_length;
} GMAACDecInfo;


void* GMAACDecode(GMAACDecHandle hDecoder,
                            GMAACDecInfo *hInfo,
                            unsigned char *buffer,
                            unsigned long buffer_size);

void GM_AAC_Dec_Destory(GMAACDecHandle hDecoder,
                        aac_buffer *b,
                        GMAACDecInfo **frameInfo);

int  GM_AAC_Dec_Init(GMAACDecHandle *hDecoder,
                     aac_buffer *b,
                     GMAACDecInfo **frameInfo);


#endif // __GMAAC_H__
