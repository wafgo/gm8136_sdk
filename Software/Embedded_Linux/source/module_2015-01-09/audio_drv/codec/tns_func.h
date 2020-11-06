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
	File:		tns_func.h

	Content:	TNS functions

*******************************************************************************/

/*
   Temporal noise shaping
 */
#ifndef _TNS_FUNC_H
#define _TNS_FUNC_H
#include "typedef.h"
#include "perceptual_cfg.h"



int16_t Init_TnsCfgLongBlk(int32_t bitrate,
                                int32_t samplerate,
                                int16_t channels,
                                GM_TNS_CFG_INFO *tnsConfig,
                                GM_PSY_CFG_LONGBLK *psyConfig,
                                int16_t active);

int16_t Init_TnsCfgShortBlk(int32_t bitrate,
                                 int32_t samplerate,
                                 int16_t channels,
                                 GM_TNS_CFG_INFO *tnsConfig,
                                 GM_PSY_CFG_SHORTBLK *psyConfig,
                                 int16_t active);

int32_t Tns_CalcFiltDecide(GM_TNS_PART_DATA* tnsData,
                 GM_TNS_CFG_INFO tC,
                 int32_t* pScratchTns,
                 const int16_t sfbOffset[],
                 int32_t* spectrum,
                 int16_t subBlockNumber,
                 int16_t blockType,
                 int32_t * sfbEnergy);

void Tns_SyncParam(GM_TNS_PART_DATA *tnsDataDest,
             const GM_TNS_PART_DATA *tnsDataSrc,
             const GM_TNS_CFG_INFO tC,
             const int16_t subBlockNumber,
             const int16_t blockType);

int16_t Tns_Filtering(GM_TNS_BLK_INFO* tnsInfo,
                 GM_TNS_PART_DATA* tnsData,
                 int16_t numOfSfb,
                 GM_TNS_CFG_INFO tC,
                 int16_t lowPassLine,
                 int32_t* spectrum,
                 int16_t subBlockNumber,
                 int16_t blockType);

void Tns_ApplyMultTableToRatios(int16_t startCb,
                               int16_t stopCb,
                               GM_TNS_SUBBLK_INFO subInfo,
                               int32_t *thresholds);




#endif /* _TNS_FUNC_H */
