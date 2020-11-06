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
	File:		quantizer_data.h

	Content:	Quantizing & coding structures

*******************************************************************************/

#ifndef _quantizer_data_h
#define _quantizer_data_h

#include "perceptual_const.h"
#include "noiseless_code.h"
#include "adj_thr_data.h"


#define MAX_MODES 10

typedef enum {
  MODE_INVALID = 0,
  MODE_1,        /* mono      */
  MODE_1_1,      /* dual mono */
  MODE_2         /* stereo    */
} ENCODER_MODE;

typedef enum {
  ID_SCE=0,     /* Single Channel Element   */
  ID_CPE=1,     /* Channel Pair Element     */
  ID_CCE=2,     /* Coupling Channel Element */
  ID_LFE=3,     /* LFE Channel Element      */
  ID_DSE=4,     /* current one DSE element for ancillary is supported */
  ID_PCE=5,
  ID_FIL=6,
  ID_END=7
}ELEMENT_TYPE;

typedef struct {
  ELEMENT_TYPE elType;
  int16_t instanceTag;
  int16_t nChannelsInEl;
  int16_t ChannelIndex[MAX_CHAN];
} GM_PART_INFO; 

typedef struct {
  int32_t paddingRest;
} GM_PAD_RESET;


/* Quantizing & coding stage */

struct GM_QC_INFO{
  GM_PART_INFO *elInfo;
  int16_t maxBits;     /* maximum number of bits in reservoir  */
  int16_t averageBits; /* average number of bits we should use */
  int16_t bitRes;
  int16_t meanPe;
  int32_t chBitrate;
  int16_t maxBitFac;
  int32_t bitrate;

  GM_PAD_RESET padding;
};

typedef struct
{
  int16_t          *quantSpec;       /* [FRAME_LEN_LONG];                            */
  uint16_t         *maxValueInSfb;   /* [MAX_GROUPED_SFB];                           */
  int16_t          *scf;             /* [MAX_GROUPED_SFB];                           */
  int16_t          globalGain;
  int16_t          mdctScale;
  int16_t          groupingMask;
  GM_SECTION_DATA    sectionData;
  int16_t          windowShape;
} GM_QC_CHANNDATA_OUT;

typedef struct
{
  int16_t		  adtsUsed;
  int16_t          staticBitsUsed; /* for verification purposes */
  int16_t          dynBitsUsed;    /* for verification purposes */
  int16_t          pe;
  int16_t          ancBitsUsed;
  int16_t          fillBits;
} GM_QC_OUTSTRM_PART;

typedef struct
{
  GM_QC_CHANNDATA_OUT  qcChannel[MAX_CHAN];
  GM_QC_OUTSTRM_PART  qcElement;
  int16_t          totStaticBitsUsed; /* for verification purposes */
  int16_t          totDynBitsUsed;    /* for verification purposes */
  int16_t          totAncBitsUsed;    /* for verification purposes */
  int16_t          totFillBits;
  int16_t          alignBits;
  int16_t          bitResTot;
  int16_t          averageBitsTot;
} GM_QC_OUTSTRM;

typedef struct {
  int32_t chBitrate;
  int16_t averageBits;               /* brutto -> look ancillary.h */
  int16_t maxBits;
  int16_t bitResLevel;
  int16_t maxBitResBits;
  int16_t relativeBits;            /* Bits relative to total Bits scaled down by 2 */
} GM_ELEMENT_BITS;

typedef struct
{
  /* this is basically struct GM_QC_INFO */
  int16_t averageBitsTot;
  int16_t maxBitsTot;
  int16_t globStatBits;
  int16_t nChannels;
  int16_t bitResTot;

  int16_t maxBitFac;

  GM_PAD_RESET   padding;

  GM_ELEMENT_BITS  elementBits;
  GM_ADJ_THR_BLK adjThr;

  int16_t logSfbFormFactor[MAX_CHAN][MAX_GROUPED_SFB];
  int16_t sfbNRelevantLines[MAX_CHAN][MAX_GROUPED_SFB];
  int16_t logSfbEnergy[MAX_CHAN][MAX_GROUPED_SFB];
} GM_QC_BLK;

#endif /* _quantizer_data_h */
