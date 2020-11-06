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
	File:		threshold_tool.h

	Content:	Threshold compensation function

*******************************************************************************/

#ifndef _threshold_tool_h
#define _threshold_tool_h

#include "adj_thr_data.h"
#include "quantizer_data.h"
//#include "interface.h"
#include "perceptual_main.h"
#include "perceptual_const.h"


/*******************************************************************************
	Content:	Perceptual entropie module functions
*******************************************************************************/
typedef struct {
   int16_t sfbLdEnergy[MAX_GROUPED_SFB];     /* 4*log(sfbEnergy)/log(2) */
   int16_t sfbNLines4[MAX_GROUPED_SFB];      /* 4*number of relevant lines in sfb */
   int16_t sfbPe[MAX_GROUPED_SFB];           /* pe for each sfb */
   int16_t sfbConstPart[MAX_GROUPED_SFB];    /* constant part for each sfb */
   int16_t sfbNActiveLines[MAX_GROUPED_SFB]; /* number of active lines in sfb */
   int16_t nActiveLines;                     /* sum of sfbNActiveLines */
   int16_t constPart;                        /* sum of sfbConstPart */
   int16_t pe;                               /* sum of sfbPe */   
} GM_PERCEP_ENTRO_CHANNEL_DATA; /* size int16_t: 303 */


typedef struct {
   GM_PERCEP_ENTRO_CHANNEL_DATA peChannelData[MAX_CHAN];   
   int16_t constPart;
   int16_t offset;
   int16_t pe; 
   int16_t nActiveLines;   
   int16_t ahFlag[MAX_CHAN][MAX_GROUPED_SFB];
   int32_t thrExp[MAX_CHAN][MAX_GROUPED_SFB];
   int32_t sfbPeFactors[MAX_CHAN][MAX_GROUPED_SFB];
} GM_PERCEP_ENTRO_PART_DATA; /* size int16_t: 303 + 4 + 120 + 240 = 667 */





int16_t Conv_bitToPe(const int16_t bits);

//int32_t AdjThrNew(GM_ADJ_THR_BLK** phAdjThr,
//                 int32_t nElements);

//void AdjThrDelete(GM_ADJ_THR_BLK *hAdjThr);

void ThrParam_Init(GM_ADJ_THR_BLK *hAdjThr,
                const int32_t peMean,
                int32_t chBitrate);

void Calc_Thr(GM_ADJ_THR_BLK *adjThrState,
                      GM_ATS_PART* AdjThrStateElement,
                      GM_PSY_OUT_CHANNEL psyOutChannel[MAX_CHAN],
                      GM_PERCEP_OUT_PART *psyOutElement,
                      int16_t *chBitDistribution,
                      int16_t logSfbEnergy[MAX_CHAN][MAX_GROUPED_SFB],
                      int16_t sfbNRelevantLines[MAX_CHAN][MAX_GROUPED_SFB],
                      GM_QC_OUTSTRM_PART* qcOE,
					  GM_ELEMENT_BITS* elBits,
					  const int16_t nChannels,
                      const int16_t maxBitFac);

void Save_DynBitThr(GM_ATS_PART *AdjThrStateElement,
                  const int16_t dynBitsUsed);


/*******************************************************************************
	Content:	Pre echo control functions
*******************************************************************************/
void InitPreEchoControl(int32_t *pbThresholdnm1,
                        int16_t  numPb,
                        int32_t *pbThresholdQuiet);


void PreEchoControl(int32_t *pbThresholdNm1,
                    int16_t  numPb,
                    int32_t  maxAllowedIncreaseFactor,
                    int16_t  minRemainingThresholdFactor,
                    int32_t *pbThreshold,
                    int16_t  mdctScale,
                    int16_t  mdctScalenm1);





#endif
