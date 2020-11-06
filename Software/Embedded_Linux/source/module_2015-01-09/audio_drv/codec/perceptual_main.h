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
	File:		Perceptual_main.h

	Content:	Psychoacoustic major function block

*******************************************************************************/

#ifndef _Perceptual_main_h
#define _Perceptual_main_h

#include "perceptual_cfg.h"
#include "quantizer_data.h"
#include "mem_manag.h"

#include "config.h"
#include "perceptual_const.h"
#include "perceptual_data.h"
#include "typedefs.h"

/*
  psy kernel
*/
typedef struct  {
  GM_PSY_CFG_LONGBLK  psyConfLong;           /* int16_t size: 515 */
  GM_PSY_CFG_SHORTBLK psyConfShort;          /* int16_t size: 167 */
  GM_TNS_PART_DATA                tnsData[MAX_CHAN]; /* int16_t size: MAX_CHAN*235 */
  GM_PSY_PART_DATA                psyData[MAX_CHAN]; /* int16_t size: MAX_CHAN*1669*/
  int32_t*                 pScratchTns;
  int16_t				  sampleRateIdx;
}GM_PSY_BLK; /* int16_t size: 2587 / 4491 */




/*******************************************************************************
	Content:	psychoaccoustic/quantizer structures and interface
*******************************************************************************/


enum
{
  MS_NONE = 0,
  MS_SOME = 1,
  MS_ALL  = 2
};

enum
{
  MS_ON = 1
};

struct TOOLSINFO {
  int16_t msDigest;
  int16_t msMask[MAX_GROUPED_SFB];
};


typedef struct {
  int16_t  windowSequence;
  int16_t  windowShape;
  int16_t  sfbCnt;
  int16_t  sfbPerGroup;
  int16_t  groupingMask;
  int16_t  maxSfbPerGroup;
  int16_t  sfbOffsets[MAX_GROUPED_SFB+1];
  int16_t  mdctScale;
  int32_t *sfbEnergy;
  int32_t *sfbThreshold;
  int32_t *mdctSpectrum;
  int32_t *sfbSpreadedEnergy;
  int32_t  sfbEnSumLR;
  int32_t  sfbEnSumMS;
  int32_t sfbDist[MAX_GROUPED_SFB];
  int32_t sfbDistNew[MAX_GROUPED_SFB];
  int16_t  sfbMinSnr[MAX_GROUPED_SFB];
  int16_t prevScfLast[MAX_GROUPED_SFB];
  int16_t prevScfNext[MAX_GROUPED_SFB];
  int16_t deltaPeLast[MAX_GROUPED_SFB];
  int16_t minSfMaxQuant[MAX_GROUPED_SFB];
  int16_t minScfCalculated[MAX_GROUPED_SFB];
  GM_TNS_BLK_INFO tnsInfo;
} GM_PSY_OUT_CHANNEL; /* int16_t size: 14 + 60(MAX_GROUPED_SFB) + 112(GM_TNS_BLK_INFO) = 186 */

typedef struct {
  struct TOOLSINFO toolsInfo;
  int16_t groupedSfbOffset[MAX_CHAN][MAX_GROUPED_SFB+1];  /* plus one for last dummy offset ! */
  int16_t groupedSfbMinSnr[MAX_CHAN][MAX_GROUPED_SFB];
} GM_PERCEP_OUT_PART;

typedef struct {
  /* information specific to each channel */
  GM_PSY_OUT_CHANNEL  psyOutChannel[MAX_CHAN];
  /* information shared by both channels  */
  GM_PERCEP_OUT_PART  psyOutElement;
}GM_PERCEP_OUT;







int16_t Perceptual_AllocMem( GM_PSY_BLK  *hPsy, int32_t nChan);
int16_t Perceptual_DelMem( GM_PSY_BLK  *hPsy);

int16_t Perceptual_OutStrmSet0( GM_PERCEP_OUT *hPsyOut);
int16_t Perceptual_OutStrmSetNULL( GM_PERCEP_OUT *hPsyOut);

int16_t Perceptual_MainInit( GM_PSY_BLK *hPsy,
                    int32_t sampleRate,
                    int32_t bitRate,
                    int16_t channels,
                    int16_t tnsMask,
                    int16_t bandwidth);

/*Psychoacoustic_Main*/
int16_t Perceptual_Main(int16_t                   nChannels,   /*!< total number of channels */
               GM_PART_INFO             *elemInfo,
               int16_t                   *timeSignal, /*!< interleaved time signal */
               GM_PSY_PART_DATA                 psyData[MAX_CHAN],
               GM_TNS_PART_DATA                 tnsData[MAX_CHAN],
               GM_PSY_CFG_LONGBLK*  psyConfLong,
               GM_PSY_CFG_SHORTBLK* psyConfShort,
               GM_PSY_OUT_CHANNEL          psyOutChannel[MAX_CHAN],
               GM_PERCEP_OUT_PART          *psyOutElement,
               int32_t                   *pScratchTns,
			   int32_t					sampleRate);





#endif /* _Perceptual_main_h */
