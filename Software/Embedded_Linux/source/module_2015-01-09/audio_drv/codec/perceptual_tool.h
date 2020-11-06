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
	File:		Perceptual_tool.h

	Content:	Block switching structure and functions

*******************************************************************************/

#ifndef _perceptual_tool_h
#define _perceptual_tool_h

#include "typedef.h"


/****************** Defines ******************************/
#define BLOCK_SWITCHING_IIR_LEN 2                                           /* Length of HighPass-FIR-Filter for Attack-Detection */
#define BLOCK_SWITCH_WINDOWS TRANS_FAC             //8                      /* number of windows for energy calculation */
#define BLOCK_SWITCH_WINDOW_LEN FRAME_LEN_SHORT   //128                     /* minimal granularity of energy calculation */



/****************** Structures ***************************/
typedef struct{
  int32_t invAttackRatio;  
  int16_t nextwindowSequence;
  int16_t windowSequence; 
  Flag attack;
  Flag lastattack;  
  int16_t lastAttackIndex;
  int16_t attackIndex; 
  int16_t noOfGroups;
  int16_t groupLen[TRANS_FAC];
  int32_t windowNrg[2][BLOCK_SWITCH_WINDOWS];     /* time signal energy in Subwindows (last and current) */
  int32_t windowNrgF[2][BLOCK_SWITCH_WINDOWS];    /* filtered time signal energy in segments (last and current) */
  int32_t iirStates[BLOCK_SWITCHING_IIR_LEN];     /* filter delay-line */
  int32_t accWindowNrg;                           /* recursively accumulated windowNrgF */
  int32_t maxWindowNrg;                           /* max energy in subwindows */  
}GM_BLK_SWITCHING_CNT;





int16_t BlockSwitch_Init(GM_BLK_SWITCHING_CNT *blockSwitchingControl,
                          const int32_t bitRate, const int16_t nChannels);

int16_t DataBlockSwitching(GM_BLK_SWITCHING_CNT *blockSwitchingControl,
                      int16_t *timeSignal,
					  int32_t  sampleRate,
                      int16_t chIncrement);

int16_t UpdateBlock(GM_BLK_SWITCHING_CNT *blockSwitchingControlLeft,
                          GM_BLK_SWITCHING_CNT *blockSwitchingControlRight,
                          const int16_t noOfChannels);



#endif  /* #ifndef _perceptual_tool_h */
