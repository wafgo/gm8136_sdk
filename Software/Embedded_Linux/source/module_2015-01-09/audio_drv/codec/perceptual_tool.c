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
	File:		Perceptual_tool.c

	Content:	Block switching functions

*******************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "oper_32b.h"
#include "perceptual_const.h"
#include "perceptual_tool.h"


#define ENERGY_SHIFT (8 - 1)

/**************** internal function prototypes ***********/
//static int16_t
//IIRFilter(const int16_t in, const int32_t coeff[], int32_t states[]);

static int32_t SearchMax(const int32_t *in, int16_t *index, int16_t n);


int32_t CalcBlkEnergy(GM_BLK_SWITCHING_CNT *blockSwitchingControl,
                 int16_t *timeSignal,
                 int16_t chIncrement,
                 int16_t windowLen);



/****************** Constants *****************************/


/*
  IIR high pass coeffs
*/
int32_t hiPassCoeff[BLOCK_SWITCHING_IIR_LEN] = {
  0xbec8b439, 0x609d4952  /* -0.5095f, 0.7548f */
};

static const int32_t accWindowNrgFac = 0x26666666;                   /* factor for accumulating filtered window energies 0.3 */
static const int32_t oneMinusAccWindowNrgFac = 0x5999999a;			/* 0.7 */
static const int32_t invAttackRatioHighBr = 0x0ccccccd;              /* inverted lower ratio limit for attacks 0.1*/
static const int32_t invAttackRatioLowBr =  0x072b020c;              /* 0.056 */
static const int32_t minAttackNrg = 0x00001e84;                      /* minimum energy for attacks 1e+6 */


/****************** Routines ****************************/


/*****************************************************************************
*
* function name: BlockSwitch_Init
* description:  init Block Switching parameter.
* returns:      TRUE if success
*
**********************************************************************************/
int16_t BlockSwitch_Init(GM_BLK_SWITCHING_CNT *blockSwitchingControl,
                          const int32_t bitRate, const int16_t nChannels)
{
  /* select attackRatio */

  if ((sub(nChannels,1)==0 && L_sub(bitRate, 24000) > 0) ||
      (sub(nChannels,1)>0 && bitRate > (nChannels * 16000))) {
    blockSwitchingControl->invAttackRatio = invAttackRatioHighBr;
  }
  else  {
    blockSwitchingControl->invAttackRatio = invAttackRatioLowBr;
  }

  return(TRUE);
}

static int16_t suggestedGroupingTable[TRANS_FAC][MAX_NO_OF_GROUPS] = {
  /* Attack in Window 0 */ {1,  3,  3,  1},
  /* Attack in Window 1 */ {1,  1,  3,  3},
  /* Attack in Window 2 */ {2,  1,  3,  2},
  /* Attack in Window 3 */ {3,  1,  3,  1},
  /* Attack in Window 4 */ {3,  1,  1,  3},
  /* Attack in Window 5 */ {3,  2,  1,  2},
  /* Attack in Window 6 */ {3,  3,  1,  1},
  /* Attack in Window 7 */ {3,  3,  1,  1}
};

/*****************************************************************************
*
* function name: DataBlockSwitching
* description:  detect this frame whether there is an attack
* returns:      TRUE if success
*
**********************************************************************************/
int16_t DataBlockSwitching(GM_BLK_SWITCHING_CNT *blockSwitchingControl,
                      int16_t *timeSignal,
					  int32_t sampleRate,
                      int16_t chIncrement)
{
  int32_t i, w;
  int32_t enM1, enMax;

  /* Reset grouping info */
  for (i=0; i<TRANS_FAC; i++) {
    blockSwitchingControl->groupLen[i] = 0;
  }


  /* Search for position and amplitude of attack in last frame (1 windows delay) */
  blockSwitchingControl->maxWindowNrg = SearchMax( &blockSwitchingControl->windowNrg[0][BLOCK_SWITCH_WINDOWS-1],
                                                          &blockSwitchingControl->attackIndex,
                                                          BLOCK_SWITCH_WINDOWS);

  blockSwitchingControl->attackIndex = blockSwitchingControl->lastAttackIndex;

  /* Set grouping info */
  blockSwitchingControl->noOfGroups = MAX_NO_OF_GROUPS;

  for (i=0; i<MAX_NO_OF_GROUPS; i++) {
    blockSwitchingControl->groupLen[i] = suggestedGroupingTable[blockSwitchingControl->attackIndex][i];
  }

  /* if the samplerate is less than 16000, it should be all the short block, avoid pre&post echo */
  if(sampleRate >= 16000) {
	  /* Save current window energy as last window energy */
	  for (w=0; w<BLOCK_SWITCH_WINDOWS; w++) {
		  blockSwitchingControl->windowNrg[0][w] = blockSwitchingControl->windowNrg[1][w];
		  blockSwitchingControl->windowNrgF[0][w] = blockSwitchingControl->windowNrgF[1][w];
	  }


	  /* Calculate unfiltered and filtered energies in subwindows and combine to segments */
	  CalcBlkEnergy(blockSwitchingControl, timeSignal, chIncrement, BLOCK_SWITCH_WINDOW_LEN);

	  /* reset attack */
	  blockSwitchingControl->attack = FALSE;

	  enMax = 0;
	  enM1 = blockSwitchingControl->windowNrgF[0][BLOCK_SWITCH_WINDOWS-1];

	  for (w=0; w<BLOCK_SWITCH_WINDOWS; w++) {
		  int32_t enM1_Tmp, accWindowNrg_Tmp, windowNrgF_Tmp;
		  int16_t enM1_Shf, accWindowNrg_Shf, windowNrgF_Shf;

		  accWindowNrg_Shf = norm_l(blockSwitchingControl->accWindowNrg);
		  enM1_Shf = norm_l(enM1);
		  windowNrgF_Shf = norm_l(blockSwitchingControl->windowNrgF[1][w]);

		  accWindowNrg_Tmp = blockSwitchingControl->accWindowNrg << accWindowNrg_Shf;
		  enM1_Tmp = enM1 << enM1_Shf;
		  windowNrgF_Tmp = blockSwitchingControl->windowNrgF[1][w] << windowNrgF_Shf;

		  /* a sliding average of the previous energies */
		  blockSwitchingControl->accWindowNrg = (fixmul(oneMinusAccWindowNrgFac, accWindowNrg_Tmp) >> accWindowNrg_Shf) +
			  (fixmul(accWindowNrgFac, enM1_Tmp) >> enM1_Shf);


		  /* if the energy with the ratio is bigger than the average, and the attack and short block  */
		  if ((fixmul(windowNrgF_Tmp, blockSwitchingControl->invAttackRatio) >> windowNrgF_Shf) >
			  blockSwitchingControl->accWindowNrg ) {
				  blockSwitchingControl->attack = TRUE;
				  blockSwitchingControl->lastAttackIndex = w;
		  }
		  enM1 = blockSwitchingControl->windowNrgF[1][w];
		  enMax = max(enMax, enM1);
	  }

	  if (enMax < minAttackNrg) {
		  blockSwitchingControl->attack = FALSE;
	  }
  }
  else
  {
	  blockSwitchingControl->attack = TRUE;
  }

  /* Check if attack spreads over frame border */
  if ((!blockSwitchingControl->attack) && (blockSwitchingControl->lastattack)) {

    if (blockSwitchingControl->attackIndex == TRANS_FAC-1) {
      blockSwitchingControl->attack = TRUE;
    }

    blockSwitchingControl->lastattack = FALSE;
  }
  else {
    blockSwitchingControl->lastattack = blockSwitchingControl->attack;
  }

  blockSwitchingControl->windowSequence =  blockSwitchingControl->nextwindowSequence;


  if (blockSwitchingControl->attack) {
    blockSwitchingControl->nextwindowSequence = SHORT_WINDOW;
  }
  else {
    blockSwitchingControl->nextwindowSequence = LONG_WINDOW;
  }

  /* update short block group */
  if (blockSwitchingControl->nextwindowSequence == SHORT_WINDOW) {

    if (blockSwitchingControl->windowSequence== LONG_WINDOW) {
      blockSwitchingControl->windowSequence = START_WINDOW;
    }

    if (blockSwitchingControl->windowSequence == STOP_WINDOW) {
      blockSwitchingControl->windowSequence = SHORT_WINDOW;
      blockSwitchingControl->noOfGroups = 3;
      blockSwitchingControl->groupLen[0] = 3;
      blockSwitchingControl->groupLen[1] = 3;
      blockSwitchingControl->groupLen[2] = 2;
    }
  }

  /* update block type */
  if (blockSwitchingControl->nextwindowSequence == LONG_WINDOW) {

    if (blockSwitchingControl->windowSequence == SHORT_WINDOW) {
      blockSwitchingControl->nextwindowSequence = STOP_WINDOW;
    }
  }

  return(TRUE);
}


/*****************************************************************************
*
* function name: SearchMax
* description:  search for the biggest value in an array
* returns:      the max value
*
**********************************************************************************/
static int32_t SearchMax(const int32_t in[], int16_t *index, int16_t n)
{
  int32_t max;
  int32_t i, idx;

  /* Search maximum value in array and return index and value */
  max = 0;
  idx = 0;

  for (i = 0; i < n; i++) {

    if (in[i+1]  > max) {
      max = in[i+1];
      idx = i;
    }
  }
  *index = idx;

  return(max);
}

/*****************************************************************************
*
* function name: CalcBlkEnergy
* description:  calculate the energy before iir-filter and after irr-filter
* returns:      TRUE if success
*
**********************************************************************************/
#ifndef ARMV5E
int32_t CalcBlkEnergy(GM_BLK_SWITCHING_CNT *blockSwitchingControl,
                        int16_t *timeSignal,
                        int16_t chIncrement,
                        int16_t windowLen)
{
  int32_t w, i, tidx/*, ch, wOffset*/;
  int32_t accuUE, accuFE;
  int32_t tempUnfiltered;
  int32_t tempFiltered;
  int32_t states0, states1;
  int32_t Coeff0, Coeff1;


  states0 = blockSwitchingControl->iirStates[0];
  states1 = blockSwitchingControl->iirStates[1];
  Coeff0 = hiPassCoeff[0];
  Coeff1 = hiPassCoeff[1];
  tidx = 0;
  for (w=0; w < BLOCK_SWITCH_WINDOWS; w++) {

    accuUE = 0;
    accuFE = 0;

//    for(i=0; i<windowLen; i++) {
    i = windowLen;
    do {
	  int32_t accu1, accu2, accu3;
	  int32_t out;
	  tempUnfiltered = timeSignal[tidx];
      tidx = tidx + chIncrement;

	  accu1 = L_mpy_ls(Coeff1, tempUnfiltered);
	  accu2 = fixmul( Coeff0, states1 );
	  accu3 = accu1 - states0;
	  out = accu3 - accu2;

	  states0 = accu1;
	  states1 = out;

      tempFiltered = extract_h(out);
      accuUE += (tempUnfiltered * tempUnfiltered) >> ENERGY_SHIFT;
      accuFE += (tempFiltered * tempFiltered) >> ENERGY_SHIFT;
    } while (--i!=0);

    blockSwitchingControl->windowNrg[1][w] = accuUE;
    blockSwitchingControl->windowNrgF[1][w] = accuFE;

  }

  blockSwitchingControl->iirStates[0] = states0;
  blockSwitchingControl->iirStates[1] = states1;

  return(TRUE);
}
#endif

/*****************************************************************************
*
* function name: IIRFilter
* description:  calculate the iir-filter for an array
* returns:      the result after iir-filter
*
**********************************************************************************/
/*
static int16_t IIRFilter(const int16_t in, const int32_t coeff[], int32_t states[])
{
  int32_t accu1, accu2, accu3;
  int32_t out;

  accu1 = L_mpy_ls(coeff[1], in);
  accu3 = accu1 - states[0];
  accu2 = fixmul( coeff[0], states[1] );
  out = accu3 - accu2;

  states[0] = accu1;
  states[1] = out;

  return round16(out);
}
*/

static int16_t synchronizedBlockTypeTable[4][4] = {
  /*                 LONG_WINDOW   START_WINDOW  SHORT_WINDOW  STOP_WINDOW */
  /* LONG_WINDOW  */{LONG_WINDOW,  START_WINDOW, SHORT_WINDOW, STOP_WINDOW},
  /* START_WINDOW */{START_WINDOW, START_WINDOW, SHORT_WINDOW, SHORT_WINDOW},
  /* SHORT_WINDOW */{SHORT_WINDOW, SHORT_WINDOW, SHORT_WINDOW, SHORT_WINDOW},
  /* STOP_WINDOW  */{STOP_WINDOW,  SHORT_WINDOW, SHORT_WINDOW, STOP_WINDOW}
};


/*****************************************************************************
*
* function name: UpdateBlock
* description:  update block type and group value
* returns:      TRUE if success
*
**********************************************************************************/
int16_t UpdateBlock(GM_BLK_SWITCHING_CNT *blockSwitchingControlLeft,
                          GM_BLK_SWITCHING_CNT *blockSwitchingControlRight,
                          const int16_t nChannels)
{
  int16_t i;
  int16_t patchType = LONG_WINDOW;


  if (nChannels == 1) { /* Mono */
    if (blockSwitchingControlLeft->windowSequence != SHORT_WINDOW) {
      blockSwitchingControlLeft->noOfGroups = 1;
      blockSwitchingControlLeft->groupLen[0] = 1;

      for (i=1; i<TRANS_FAC; i++) {
        blockSwitchingControlLeft->groupLen[i] = 0;
      }
    }
  }
  else { /* Stereo common Window */
    patchType = synchronizedBlockTypeTable[patchType][blockSwitchingControlLeft->windowSequence];
    patchType = synchronizedBlockTypeTable[patchType][blockSwitchingControlRight->windowSequence];

    /* Set synchronized Blocktype */
    blockSwitchingControlLeft->windowSequence = patchType;
    blockSwitchingControlRight->windowSequence = patchType;

    /* Synchronize grouping info */
    if(patchType != SHORT_WINDOW) { /* Long Blocks */
      /* Set grouping info */
      blockSwitchingControlLeft->noOfGroups = 1;
      blockSwitchingControlRight->noOfGroups = 1;
      blockSwitchingControlLeft->groupLen[0] = 1;
      blockSwitchingControlRight->groupLen[0] = 1;

      for (i=1; i<TRANS_FAC; i++) {
        blockSwitchingControlLeft->groupLen[i] = 0;
        blockSwitchingControlRight->groupLen[i] = 0;
      }
    }
    else {

      if (blockSwitchingControlLeft->maxWindowNrg > blockSwitchingControlRight->maxWindowNrg) {
        /* Left Channel wins */
        blockSwitchingControlRight->noOfGroups = blockSwitchingControlLeft->noOfGroups;
        for (i=0; i<TRANS_FAC; i++) {
          blockSwitchingControlRight->groupLen[i] = blockSwitchingControlLeft->groupLen[i];
        }
      }
      else {
        /* Right Channel wins */
        blockSwitchingControlLeft->noOfGroups = blockSwitchingControlRight->noOfGroups;
        for (i=0; i<TRANS_FAC; i++) {
          blockSwitchingControlLeft->groupLen[i] = blockSwitchingControlRight->groupLen[i];
        }
      }
    }
  } /*endif Mono or Stereo */

  return(TRUE);
}
