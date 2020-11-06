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
	File:		threshold_tool.c

	Content:	Threshold compensation functions

*******************************************************************************/
//#include <string.h>
#include "basic_op.h"
#include "oper_32b.h"
#include "adj_thr_data.h"
#include "threshold_tool.h"
#include "quantizer_data.h"
//#include "line_pe.h"
//#include "pre_echo_control.h"


#define  minSnrLimit    0x6666 /* 1 dB */
#define  PEBITS_COEF	0x170a /* 0.18*(1 << 15)*/

#define  HOLE_THR_LONG	0x2873	/* 0.316*(1 << 15) */
#define  HOLE_THR_SHORT 0x4000  /* 0.5  *(1 << 15) */

#define  MS_THRSPREAD_COEF 0x7333  /* 0.9 * (1 << 15) */

#define	 MIN_SNR_COEF	   0x651f  /* 3.16* (1 << (15 - 2)) */

/* values for avoid hole flag */
enum _avoid_hole_state {
  NO_AH              =0,
  AH_INACTIVE        =1,
  AH_ACTIVE          =2
};

static void RedBandMinSnr(GM_PSY_OUT_CHANNEL     psyOutChannel[MAX_CHAN],
                        int16_t              logSfbEnergy[MAX_CHAN][MAX_GROUPED_SFB],
                        GM_MINSNR_ADAPT_ARGUM *msaParam,
                        const int16_t        nChannels);




void Calc_SfbPe_First(GM_PERCEP_ENTRO_PART_DATA *peData,
                  GM_PSY_OUT_CHANNEL  psyOutChannel[MAX_CHAN],
                  int16_t logSfbEnergy[MAX_CHAN][MAX_GROUPED_SFB],
                  int16_t sfbNRelevantLines[MAX_CHAN][MAX_GROUPED_SFB],
                  const int16_t nChannels,
                  const int16_t peOffset);





void calc_ConstPartSfPe(GM_PERCEP_ENTRO_PART_DATA *peData,
               GM_PSY_OUT_CHANNEL psyOutChannel[MAX_CHAN],
               const int16_t nChannels);




/********************************************************************************
*
* function name:Conv_bitToPe
* description: convert from bits to pe
*			   pe = 1.18*desiredBits
*
**********************************************************************************/
int16_t Conv_bitToPe(const int16_t bits) {
  return (bits + ((PEBITS_COEF * bits) >> 15));
}

/********************************************************************************
*
* function name:LoudnessThrRedExp
* description: loudness calculation (threshold to the power of redExp)
*			   thr(n)^0.25
*
**********************************************************************************/
static void LoudnessThrRedExp(int32_t thrExp[MAX_CHAN][MAX_GROUPED_SFB],
                          GM_PSY_OUT_CHANNEL psyOutChannel[MAX_CHAN],
                          const int16_t nChannels)
{
  int16_t ch, sfb, sfbGrp;
  int32_t *pthrExp = NULL, *psfbThre = NULL;

  for (ch=0; ch<nChannels; ch++) {
	GM_PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
	for(sfbGrp = 0; sfbGrp < psyOutChan->sfbCnt; sfbGrp+= psyOutChan->sfbPerGroup)
	  pthrExp = &(thrExp[ch][sfbGrp]);
	  psfbThre = psyOutChan->sfbThreshold + sfbGrp;
//	  for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
	  sfb = psyOutChan->maxSfbPerGroup;
	  do {
		*pthrExp = rsqrt(rsqrt(*psfbThre,INT_BITS),INT_BITS);
		pthrExp++; psfbThre++;
      } while (--sfb!=0);
  }

}


/********************************************************************************
*
* function name:DeterBandAvoiHole
* description: determine bands where avoid hole is not necessary resp. possible
*
**********************************************************************************/
/*
static void DeterBandAvoiHole(int16_t ahFlag[MAX_CHAN][MAX_GROUPED_SFB],
                              GM_PSY_OUT_CHANNEL  psyOutChannel[MAX_CHAN],
                              GM_PERCEP_OUT_PART* psyOutElement,
                              const int16_t nChannels,
                              GM_AVOIDHOLE_ARGUM *ahParam)
*/
//extern int frame_count;

static void DeterBandAvoiHole(int16_t ahFlag[MAX_CHAN][MAX_GROUPED_SFB],
                              GM_PSY_OUT_CHANNEL  psyOutChannel[MAX_CHAN],
                              GM_PERCEP_OUT_PART* psyOutElement,
                              int16_t nChannels,
                              GM_AVOIDHOLE_ARGUM *ahParam)

{
  int16_t ch, sfb, sfbGrp, shift;
  int32_t threshold;
  int32_t* psfbSpreadEn;
//---------- debug ------------------
  //int16_t channel_bk;
//---------- end --------------------
/*
  channel_bk = nChannels;
  if(channel_bk != 1)
  {
      while(1);
      return;
  }
*/
  for (ch=0; ch<nChannels; ch++) {
    GM_PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];

    if (psyOutChan->windowSequence != SHORT_WINDOW) {
      for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){
         psfbSpreadEn = psyOutChan->sfbSpreadedEnergy + sfbGrp;
		 for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
			*psfbSpreadEn = *psfbSpreadEn >> 1;  /* 0.5 */
			++psfbSpreadEn;
        }
      }
    }
    else {
      for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){
		psfbSpreadEn = psyOutChan->sfbSpreadedEnergy + sfbGrp;
        for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
          *psfbSpreadEn = (*psfbSpreadEn >> 1) + (*psfbSpreadEn >> 3);  /* 0.63 */
		  ++psfbSpreadEn;
        }
      }
    }
  }

  /* increase minSnr for local peaks, decrease it for valleys */
  if (ahParam->modifyMinSnr) {
    for(ch=0; ch<nChannels; ch++) {
      GM_PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];

      if (psyOutChan->windowSequence != SHORT_WINDOW)
        threshold = HOLE_THR_LONG;
      else
        threshold = HOLE_THR_SHORT;

      for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){
        int16_t *psfbMinSnr = psyOutChan->sfbMinSnr + sfbGrp;
		for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
          int32_t sfbEn, sfbEnm1, sfbEnp1, avgEn;

          if (sfb > 0)
            sfbEnm1 = psyOutChan->sfbEnergy[sfbGrp+sfb-1];
          else
            sfbEnm1 = psyOutChan->sfbEnergy[sfbGrp];

          if (sfb < (psyOutChan->maxSfbPerGroup-1))
            sfbEnp1 = psyOutChan->sfbEnergy[sfbGrp+sfb+1];
          else
            sfbEnp1 = psyOutChan->sfbEnergy[sfbGrp+sfb];
          avgEn = (sfbEnm1 + sfbEnp1) >> 1;
          sfbEn = psyOutChan->sfbEnergy[sfbGrp+sfb];

          if (sfbEn > avgEn && avgEn > 0) {
            int32_t tmpMinSnr;
            shift = norm_l(sfbEn);
			tmpMinSnr = Div_32(L_mpy_ls(avgEn, minSnrLimit) << shift, sfbEn << shift );
            tmpMinSnr = max(tmpMinSnr, HOLE_THR_LONG);
            tmpMinSnr = max(tmpMinSnr, threshold);
            *psfbMinSnr = min((int32_t)*psfbMinSnr, tmpMinSnr);
          }
          /* valley ? */

          if ((sfbEn < (avgEn >> 1)) && (sfbEn > 0)) {
            int32_t tmpMinSnr;
            int32_t minSnrEn = L_mpy_wx(avgEn, *psfbMinSnr);

            if(minSnrEn < sfbEn) {
			  shift = norm_l(sfbEn);
              tmpMinSnr = Div_32( minSnrEn << shift, sfbEn<<shift);
            }
            else {
              tmpMinSnr = MAX_16;
            }
            tmpMinSnr = min(minSnrLimit, tmpMinSnr);

            *psfbMinSnr =
              (min((tmpMinSnr >>  2), mult(*psfbMinSnr, MIN_SNR_COEF)) << 2);
          }
		  psfbMinSnr++;
        }
      }
    }
  }

  /* stereo: adapt the minimum requirements sfbMinSnr of mid and
     side channels */

  if (nChannels == 2) {
    GM_PSY_OUT_CHANNEL *psyOutChanM = &psyOutChannel[0];
    GM_PSY_OUT_CHANNEL *psyOutChanS = &psyOutChannel[1];
    for (sfb=0; sfb<psyOutChanM->sfbCnt; sfb++) {
      if (psyOutElement->toolsInfo.msMask[sfb]) {
        int32_t sfbEnM = psyOutChanM->sfbEnergy[sfb];
        int32_t sfbEnS = psyOutChanS->sfbEnergy[sfb];
        int32_t maxSfbEn = max(sfbEnM, sfbEnS);
        int32_t maxThr = L_mpy_wx(maxSfbEn, psyOutChanM->sfbMinSnr[sfb]) >> 1;

        if(maxThr >= sfbEnM) {
          psyOutChanM->sfbMinSnr[sfb] = MAX_16;
        }
        else {
          shift = norm_l(sfbEnM);
		  psyOutChanM->sfbMinSnr[sfb] = min(max((int32_t)psyOutChanM->sfbMinSnr[sfb],
			  round16(Div_32(maxThr<<shift, sfbEnM << shift))), minSnrLimit);
        }

        if(maxThr >= sfbEnS) {
          psyOutChanS->sfbMinSnr[sfb] = MAX_16;
        }
        else {
		  shift = norm_l(sfbEnS);
          psyOutChanS->sfbMinSnr[sfb] = min(max((int32_t)psyOutChanS->sfbMinSnr[sfb],
			  round16(Div_32(maxThr << shift, sfbEnS << shift))), minSnrLimit);
        }


        if (sfbEnM > psyOutChanM->sfbSpreadedEnergy[sfb])
          psyOutChanS->sfbSpreadedEnergy[sfb] = L_mpy_ls(sfbEnS, MS_THRSPREAD_COEF);

        if (sfbEnS > psyOutChanS->sfbSpreadedEnergy[sfb])
          psyOutChanM->sfbSpreadedEnergy[sfb] = L_mpy_ls(sfbEnM, MS_THRSPREAD_COEF);
      }
    }
  }


  /* init ahFlag (0: no ah necessary, 1: ah possible, 2: ah active */
  for(ch=0; ch<nChannels; ch++) {
    GM_PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
    for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){
      int16_t *pahFlag = ahFlag[ch] + sfbGrp;
	  for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

        if ((psyOutChan->sfbSpreadedEnergy[sfbGrp+sfb] > psyOutChan->sfbEnergy[sfbGrp+sfb]) ||
            (psyOutChan->sfbEnergy[sfbGrp+sfb] <= psyOutChan->sfbThreshold[sfbGrp+sfb]) ||
            (psyOutChan->sfbMinSnr[sfbGrp+sfb] == MAX_16)) {
          *pahFlag++ = NO_AH;
        }
        else {
          *pahFlag++ = AH_INACTIVE;
        }
      }
      for (sfb=psyOutChan->maxSfbPerGroup; sfb<psyOutChan->sfbPerGroup; sfb++) {
        *pahFlag++ = NO_AH;
      }
    }
  }
}

/********************************************************************************
*
* function name:SumPeAvoidHole
* description: sum the pe data only for bands where avoid hole is inactive
*
**********************************************************************************/
static void SumPeAvoidHole(int16_t          *pe,
                       int16_t          *constPart,
                       int16_t          *nActiveLines,
                       GM_PERCEP_ENTRO_PART_DATA         *peData,
                       int16_t           ahFlag[MAX_CHAN][MAX_GROUPED_SFB],
                       GM_PSY_OUT_CHANNEL  psyOutChannel[MAX_CHAN],
                       const int16_t     nChannels)
{
  int16_t ch, sfb, sfbGrp;
  int ipe, iconstPart, inActiveLines;

  ipe = 0;
  iconstPart = 0;
  inActiveLines = 0;
  for(ch=0; ch<nChannels; ch++) {
    GM_PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
    GM_PERCEP_ENTRO_CHANNEL_DATA *peChanData = &peData->peChannelData[ch];
    for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){
      for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

        if (ahFlag[ch][sfbGrp+sfb] < AH_ACTIVE) {
          ipe = ipe + peChanData->sfbPe[sfbGrp+sfb];
          iconstPart = iconstPart + peChanData->sfbConstPart[sfbGrp+sfb];
          inActiveLines = inActiveLines + peChanData->sfbNActiveLines[sfbGrp+sfb];
        }
      }
    }
  }

  *pe = saturate(ipe);
  *constPart = saturate(iconstPart);
  *nActiveLines = saturate(inActiveLines);
}

/********************************************************************************
*
* function name:ReduThrFormula
* description: apply reduction formula
*
**********************************************************************************/
static void ReduThrFormula(GM_PSY_OUT_CHANNEL  psyOutChannel[MAX_CHAN],
                             int16_t           ahFlag[MAX_CHAN][MAX_GROUPED_SFB],
                             int32_t           thrExp[MAX_CHAN][MAX_GROUPED_SFB],
                             const int16_t     nChannels,
                             const int32_t     redVal)
{
  int32_t sfbThrReduced = 0;
  int32_t *psfbEn, *psfbThr;
  int16_t ch, sfb, sfbGrp;

  for(ch=0; ch<nChannels; ch++) {
    GM_PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
    for(sfbGrp=0; sfbGrp<psyOutChan->sfbCnt; sfbGrp+=psyOutChan->sfbPerGroup) {
 	  psfbEn  = psyOutChan->sfbEnergy + sfbGrp;
      psfbThr = psyOutChan->sfbThreshold + sfbGrp;
	  for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

        if (*psfbEn > *psfbThr) {
          /* threshold reduction formula */
          int32_t t = 0;
          int32_t tmp = thrExp[ch][sfbGrp+sfb] + redVal;
#ifdef LINUX
					asm volatile("smull %[o1], %[o2], %[o2], %[o2]" : [o1] "+r" (t), [o2] "+r" (tmp));
				  tmp <<= 1;
					asm volatile("smull %[o1], %[o2], %[i1], %[i1]" : [o1] "+r" (t), [o2] "+r" (sfbThrReduced) : [i1] "r" (tmp));
		  		sfbThrReduced <<= 1;
#else
		  __asm {
		  	SMULL	t, tmp, tmp, tmp
		  }
		  tmp <<= 1;
		  __asm {
		  	SMULL	t, sfbThrReduced, tmp, tmp
		  }
		  sfbThrReduced <<= 1;
#endif
//          tmp = fixmul(tmp, tmp);
//          sfbThrReduced = fixmul(tmp, tmp);
          /* avoid holes */
          tmp = L_mpy_ls(*psfbEn, psyOutChan->sfbMinSnr[sfbGrp+sfb]);

          if ((sfbThrReduced > tmp) &&
              (ahFlag[ch][sfbGrp+sfb] != NO_AH)){
            sfbThrReduced = max(tmp, *psfbThr);
            ahFlag[ch][sfbGrp+sfb] = AH_ACTIVE;
          }
		  *psfbThr = sfbThrReduced;
        }

		psfbEn++;  psfbThr++;
      }
    }
  }
}


/********************************************************************************
*
* function name:DiffPeThrSfBand
* description: if pe difference deltaPe between desired pe and real pe is small enough,
*             the difference can be distributed among the scale factor bands.
*
**********************************************************************************/
static void DiffPeThrSfBand(GM_PSY_OUT_CHANNEL  psyOutChannel[MAX_CHAN],
                          int16_t           ahFlag[MAX_CHAN][MAX_GROUPED_SFB],
                          GM_PERCEP_ENTRO_PART_DATA          *peData,
                          int32_t           thrExp[MAX_CHAN][MAX_GROUPED_SFB],
                          const int32_t     redVal,
                          const int16_t     nChannels,
                          const int32_t     deltaPe)
{
  int16_t ch, sfb, sfbGrp,shift;
  GM_PSY_OUT_CHANNEL *psyOutChan;
  GM_PERCEP_ENTRO_CHANNEL_DATA *peChanData;
  int32_t deltaSfbPe;
  int32_t normFactor;
  int32_t *psfbPeFactors;
  int16_t *psfbNActiveLines, *pahFlag;
  int32_t sfbEn, sfbThr;
  int32_t sfbThrReduced;


  /* for each sfb calc relative factors for pe changes */
  normFactor = 1;
  for(ch=0; ch<nChannels; ch++) {
    psyOutChan = &psyOutChannel[ch];
    peChanData = &peData->peChannelData[ch];
    for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){
      psfbPeFactors = peData->sfbPeFactors[ch] + sfbGrp;
	  psfbNActiveLines = peChanData->sfbNActiveLines + sfbGrp;
	  pahFlag = ahFlag[ch] + sfbGrp;
	  for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
        int32_t redThrExp = thrExp[ch][sfbGrp+sfb] + redVal;

        if (((*pahFlag < AH_ACTIVE) || (deltaPe > 0)) && (redThrExp > 0) ) {

          *psfbPeFactors = (*psfbNActiveLines) * (0x7fffffff / redThrExp);
          normFactor = L_add(normFactor, *psfbPeFactors);
        }
        else {
          *psfbPeFactors = 0;
        }
		psfbPeFactors++;
		pahFlag++; psfbNActiveLines++;
      }
    }
  }


  /* calculate new thresholds */
  for(ch=0; ch<nChannels; ch++) {
    psyOutChan = &psyOutChannel[ch];
    peChanData = &peData->peChannelData[ch];
    for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){
      psfbPeFactors = peData->sfbPeFactors[ch] + sfbGrp;
	  psfbNActiveLines = peChanData->sfbNActiveLines + sfbGrp;
	  pahFlag = ahFlag[ch] + sfbGrp;
	  for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
        /* pe difference for this sfb */
        deltaSfbPe = *psfbPeFactors * deltaPe;

		/* thr3(n) = thr2(n)*2^deltaSfbPe/b(n) */
        if (*psfbNActiveLines > 0) {
          /* new threshold */
          int32_t thrFactor;
          sfbEn  = psyOutChan->sfbEnergy[sfbGrp+sfb];
          sfbThr = psyOutChan->sfbThreshold[sfbGrp+sfb];

           if(deltaSfbPe >= 0){
            /*
              reduce threshold
            */
            thrFactor = pow2_xy(L_negate(deltaSfbPe), (normFactor* (*psfbNActiveLines)));

            sfbThrReduced = L_mpy_ls(sfbThr, round16(thrFactor));
          }
          else {
            /*
              increase threshold
            */
            thrFactor = pow2_xy(deltaSfbPe, (normFactor * (*psfbNActiveLines)));


            if(thrFactor > sfbThr) {
              shift = norm_l(thrFactor);
			  sfbThrReduced = Div_32( sfbThr << shift, thrFactor<<shift );
            }
            else {
              sfbThrReduced = MAX_32;
            }

          }

          /* avoid hole */
          sfbEn = L_mpy_ls(sfbEn, psyOutChan->sfbMinSnr[sfbGrp+sfb]);

          if ((sfbThrReduced > sfbEn) &&
              (*pahFlag == AH_INACTIVE)) {
            sfbThrReduced = max(sfbEn, sfbThr);
            *pahFlag = AH_ACTIVE;
          }

          psyOutChan->sfbThreshold[sfbGrp+sfb] = sfbThrReduced;
        }

		pahFlag++; psfbNActiveLines++; psfbPeFactors++;
      }
    }
  }
}


/********************************************************************************
*
* function name:ReduPeSnr
* description: if the desired pe can not be reached, reduce pe by reducing minSnr
*
**********************************************************************************/
static void ReduPeSnr(GM_PSY_OUT_CHANNEL  psyOutChannel[MAX_CHAN],
                         GM_PERCEP_ENTRO_PART_DATA         *peData,
                         int16_t           ahFlag[MAX_CHAN][MAX_GROUPED_SFB],
                         const int16_t     nChannels,
                         const int16_t     desiredPe)
{
  int16_t ch, sfb, sfbSubWin;
  int16_t deltaPe;

  /* start at highest freq down to 0 */
  sfbSubWin = psyOutChannel[0].maxSfbPerGroup;
  while (peData->pe > desiredPe && sfbSubWin > 0) {

    sfbSubWin = sfbSubWin - 1;
    /* loop over all subwindows */
    for (sfb=sfbSubWin; sfb<psyOutChannel[0].sfbCnt;
        sfb+=psyOutChannel[0].sfbPerGroup) {
      /* loop over all channels */
		GM_PERCEP_ENTRO_CHANNEL_DATA* peChan = peData->peChannelData;
		GM_PSY_OUT_CHANNEL* psyOutCh = psyOutChannel;
		for (ch=0; ch<nChannels; ch++) {
        if (ahFlag[ch][sfb] != NO_AH &&
            psyOutCh->sfbMinSnr[sfb] < minSnrLimit) {
          psyOutCh->sfbMinSnr[sfb] = minSnrLimit;
          psyOutCh->sfbThreshold[sfb] =
            L_mpy_ls(psyOutCh->sfbEnergy[sfb], psyOutCh->sfbMinSnr[sfb]);

          /* calc new pe */
          deltaPe = ((peChan->sfbNLines4[sfb] + (peChan->sfbNLines4[sfb] >> 1)) >> 2) -
              peChan->sfbPe[sfb];
          peData->pe = peData->pe + deltaPe;
          peChan->pe = peChan->pe + deltaPe;
        }
		peChan += 1; psyOutCh += 1;
      }
      /* stop if enough has been saved */

      if (peData->pe <= desiredPe)
        break;
    }
  }
}

/********************************************************************************
*
* function name:MidSid_HoleChann
* description: if the desired pe can not be reached, some more scalefactor bands
*              have to be quantized to zero
*
**********************************************************************************/
static void MidSid_HoleChann(GM_PSY_OUT_CHANNEL  psyOutChannel[MAX_CHAN],
                           GM_PERCEP_OUT_PART *psyOutElement,
                           GM_PERCEP_ENTRO_PART_DATA         *peData,
                           int16_t           ahFlag[MAX_CHAN][MAX_GROUPED_SFB],
                           const GM_AVOIDHOLE_ARGUM  *ahParam,
                           const int16_t     nChannels,
                           const int16_t     desiredPe)
{
  int16_t ch, sfb;
  int16_t actPe, shift;

  actPe = peData->pe;

  /* for MS allow hole in the channel with less energy */

  if (nChannels==2 &&
      psyOutChannel[0].windowSequence==psyOutChannel[1].windowSequence) {
    GM_PSY_OUT_CHANNEL *psyOutChanL = &psyOutChannel[0];
    GM_PSY_OUT_CHANNEL *psyOutChanR = &psyOutChannel[1];
    for (sfb=0; sfb<psyOutChanL->sfbCnt; sfb++) {
      int32_t minEn;

      if (psyOutElement->toolsInfo.msMask[sfb]) {
        /* allow hole in side channel ? */
        minEn = L_mpy_ls(psyOutChanL->sfbEnergy[sfb], (minSnrLimit * psyOutChanL->sfbMinSnr[sfb]) >> 16);

        if (ahFlag[1][sfb] != NO_AH &&
            minEn > psyOutChanR->sfbEnergy[sfb]) {
          ahFlag[1][sfb] = NO_AH;
          psyOutChanR->sfbThreshold[sfb] = L_add(psyOutChanR->sfbEnergy[sfb], psyOutChanR->sfbEnergy[sfb]);
          actPe = actPe - peData->peChannelData[1].sfbPe[sfb];
        }
        /* allow hole in mid channel ? */
        else {
        minEn = L_mpy_ls(psyOutChanR->sfbEnergy[sfb], (minSnrLimit * psyOutChanR->sfbMinSnr[sfb]) >> 16);

          if (ahFlag[0][sfb]!= NO_AH &&
              minEn > psyOutChanL->sfbEnergy[sfb]) {
            ahFlag[0][sfb] = NO_AH;
            psyOutChanL->sfbThreshold[sfb] = L_add(psyOutChanL->sfbEnergy[sfb], psyOutChanL->sfbEnergy[sfb]);
            actPe = actPe - peData->peChannelData[0].sfbPe[sfb];
          }
        }

        if (actPe < desiredPe)
          break;
      }
    }
  }

  /* subsequently erase bands */
  if (actPe > desiredPe) {
    int16_t startSfb[2];
    int32_t avgEn, minEn;
    int16_t ahCnt;
    int16_t enIdx;
    int16_t enDiff;
    int32_t en[4];
    int16_t minSfb, maxSfb;
    Flag   done;

    /* do not go below startSfb */
    for (ch=0; ch<nChannels; ch++) {

      if (psyOutChannel[ch].windowSequence != SHORT_WINDOW)
        startSfb[ch] = ahParam->startSfbL;
      else
        startSfb[ch] = ahParam->startSfbS;
    }

    avgEn = 0;
    minEn = MAX_32;
    ahCnt = 0;
    for (ch=0; ch<nChannels; ch++) {
      GM_PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
      for (sfb=startSfb[ch]; sfb<psyOutChan->sfbCnt; sfb++) {

        if ((ahFlag[ch][sfb] != NO_AH) &&
            (psyOutChan->sfbEnergy[sfb] > psyOutChan->sfbThreshold[sfb])) {
          minEn = min(minEn, psyOutChan->sfbEnergy[sfb]);
          avgEn = L_add(avgEn, psyOutChan->sfbEnergy[sfb]);
          ahCnt++;
        }
      }
    }

    if(ahCnt) {
      int32_t iahCnt;
      shift = norm_l(ahCnt);
	  iahCnt = Div_32( 1 << shift, ahCnt << shift );
      avgEn = fixmul(avgEn, iahCnt);
    }

    enDiff = iLog4(avgEn) - iLog4(minEn);
    /* calc some energy borders between minEn and avgEn */
    for (enIdx=0; enIdx<4; enIdx++) {
      int32_t enFac;
      enFac = ((6-(enIdx << 1)) * enDiff);
      en[enIdx] = fixmul(avgEn, pow2_xy(L_negate(enFac),7*4));
    }

    /* start with lowest energy border at highest sfb */
    maxSfb = psyOutChannel[0].sfbCnt - 1;
    minSfb = startSfb[0];

    if (nChannels == 2) {
      maxSfb = max(maxSfb, (int16_t)(psyOutChannel[1].sfbCnt - 1));
      minSfb = min(minSfb, startSfb[1]);
    }

    sfb = maxSfb;
    enIdx = 0;
    done = 0;
    while (!done) {

      for (ch=0; ch<nChannels; ch++) {
        GM_PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];

        if (sfb>=startSfb[ch] && sfb<psyOutChan->sfbCnt) {
          /* sfb energy below border ? */

          if (ahFlag[ch][sfb] != NO_AH && psyOutChan->sfbEnergy[sfb] < en[enIdx]){
            /* allow hole */
            ahFlag[ch][sfb] = NO_AH;
            psyOutChan->sfbThreshold[sfb] = L_add(psyOutChan->sfbEnergy[sfb], psyOutChan->sfbEnergy[sfb]);
            actPe = actPe - peData->peChannelData[ch].sfbPe[sfb];
          }

          if (actPe < desiredPe) {
            done = 1;
            break;
          }
        }
      }
      sfb = sfb - 1;

      if (sfb < minSfb) {
        /* restart with next energy border */
        sfb = maxSfb;
        enIdx = enIdx + 1;

        if (enIdx - 4 >= 0)
          done = 1;
      }
    }
  }
}

/********************************************************************************
*
* function name:Calc_ThrToPe
* description: two guesses for the reduction value and one final correction of the
*              thresholds
*
**********************************************************************************/
static void Calc_ThrToPe(GM_PSY_OUT_CHANNEL     psyOutChannel[MAX_CHAN],
                                GM_PERCEP_OUT_PART    *psyOutElement,
                                int16_t              logSfbEnergy[MAX_CHAN][MAX_GROUPED_SFB],
                                GM_PERCEP_ENTRO_PART_DATA            *peData,
                                const int16_t        nChannels,
                                const int16_t        desiredPe,
                                GM_AVOIDHOLE_ARGUM           *ahParam,
                                GM_MINSNR_ADAPT_ARGUM *msaParam)
{
  int16_t noRedPe, redPe, redPeNoAH;
  int16_t constPart, constPartNoAH;
  int16_t nActiveLines, nActiveLinesNoAH;
  int16_t desiredPeNoAH;
  int32_t redVal, avgThrExp;
  int32_t iter;

  LoudnessThrRedExp(peData->thrExp, psyOutChannel, nChannels);

  RedBandMinSnr(psyOutChannel, logSfbEnergy, msaParam, nChannels);

  DeterBandAvoiHole(peData->ahFlag, psyOutChannel, psyOutElement, nChannels, ahParam);

  noRedPe = peData->pe;
  constPart = peData->constPart;
  nActiveLines = peData->nActiveLines;

  /* first guess of reduction value t^0.25 = 2^((a-pen)/4*b) */
  avgThrExp = pow2_xy((constPart - noRedPe), (nActiveLines << 2));

  /* r1 = 2^((a-per)/4*b) - t^0.25 */
  redVal = pow2_xy((constPart - desiredPe), (nActiveLines << 2)) - avgThrExp;

  /* reduce thresholds */
  ReduThrFormula(psyOutChannel, peData->ahFlag, peData->thrExp, nChannels, redVal);

  /* pe after first guess */
  calc_ConstPartSfPe(peData, psyOutChannel, nChannels);
  redPe = peData->pe;


  iter = 0;
  do {
    /* pe for bands where avoid hole is inactive */
    SumPeAvoidHole(&redPeNoAH, &constPartNoAH, &nActiveLinesNoAH,
               peData, peData->ahFlag, psyOutChannel, nChannels);

    desiredPeNoAH = desiredPe -(redPe - redPeNoAH);

    if (desiredPeNoAH < 0) {
      desiredPeNoAH = 0;
    }

    /* second guess */

    if (nActiveLinesNoAH > 0) {

		avgThrExp = pow2_xy((constPartNoAH - redPeNoAH), (nActiveLinesNoAH << 2));

		redVal = (redVal + pow2_xy((constPartNoAH - desiredPeNoAH), (nActiveLinesNoAH << 2))) - avgThrExp;

		/* reduce thresholds */
		ReduThrFormula(psyOutChannel, peData->ahFlag, peData->thrExp, nChannels, redVal);
    }

    calc_ConstPartSfPe(peData, psyOutChannel, nChannels);
    redPe = peData->pe;

    iter = iter+1;

  } while ((20 * abs_s(redPe - desiredPe) > desiredPe) && (iter < 2));


  if ((100 * redPe < 115 * desiredPe)) {
    DiffPeThrSfBand(psyOutChannel, peData->ahFlag, peData, peData->thrExp, redVal,
                  nChannels, desiredPe - redPe);
  }
  else {
    int16_t desiredPe105 = (105 * desiredPe) / 100;
    ReduPeSnr(psyOutChannel, peData, peData->ahFlag,
                 nChannels, desiredPe105);
    MidSid_HoleChann(psyOutChannel, psyOutElement, peData, peData->ahFlag,
                   ahParam, nChannels, desiredPe105);
  }
}


/*****************************************************************************
*
* function name: BitSavePercen
* description:  Calculates percentage of bit save, see figure below
* returns:
* input:        parameters and bitres-fullness
* output:       percentage of bit save
*
*****************************************************************************/
static int16_t BitSavePercen(int16_t fillLevel,
                          const int16_t clipLow,
                          const int16_t clipHigh,
                          const int16_t minBitSave,
                          const int16_t maxBitSave)
{
  int16_t bitsave = 0;

  fillLevel = max(fillLevel, clipLow);
  fillLevel = min(fillLevel, clipHigh);

  if(clipHigh-clipLow)
  bitsave = (maxBitSave - (((maxBitSave-minBitSave)*(fillLevel-clipLow))/
                              (clipHigh-clipLow)));

  return (bitsave);
}



/*****************************************************************************
*
* function name: calcBitSpend
* description:  Calculates percentage of bit spend, see figure below
* returns:
* input:        parameters and bitres-fullness
* output:       percentage of bit spend
*
*****************************************************************************/
static int16_t calcBitSpend(int16_t fillLevel,
                           const int16_t clipLow,
                           const int16_t clipHigh,
                           const int16_t minBitSpend,
                           const int16_t maxBitSpend)
{
  int16_t bitspend = 1;

  fillLevel = max(fillLevel, clipLow);
  fillLevel = min(fillLevel, clipHigh);

  if(clipHigh-clipLow)
  bitspend = (minBitSpend + ((maxBitSpend - minBitSpend)*(fillLevel - clipLow) /
                                (clipHigh-clipLow)));

  return (bitspend);
}


/*****************************************************************************
*
* function name: Calc_PeMinPeMax()
* description:  adjusts peMin and peMax parameters over time
* returns:
* input:        current pe, peMin, peMax
* output:       adjusted peMin/peMax
*
*****************************************************************************/
static void Calc_PeMinPeMax(const int16_t currPe,
                           int16_t      *peMin,
                           int16_t      *peMax)
{
  int16_t minFacHi, maxFacHi, minFacLo, maxFacLo;
  int16_t diff;
  int16_t minDiff = (int16_t)(currPe / 6);//extract_l(currPe / 6);
  minFacHi = 30;
  maxFacHi = 100;
  minFacLo = 14;
  maxFacLo = 7;

  diff = currPe - *peMax ;

  if (diff > 0) {
    *peMin = *peMin + ((diff * minFacHi) / 100);
    *peMax = *peMax + ((diff * maxFacHi) / 100);
  } else {
    diff = *peMin - currPe;

    if (diff > 0) {
      *peMin = *peMin - ((diff * minFacLo) / 100);
      *peMax = *peMax - ((diff * maxFacLo) / 100);
    } else {
      *peMin = *peMin + ((currPe - *peMin) * minFacHi / 100);
      *peMax = *peMax - ((*peMax - currPe) * maxFacLo / 100);
    }
  }


  if ((*peMax - *peMin) < minDiff) {
    int16_t partLo, partHi;

    partLo = max(0, (currPe - *peMin));
    partHi = max(0, (*peMax - currPe));

    *peMax = currPe + ((partHi * minDiff) / (partLo + partHi));
    *peMin = currPe - ((partLo * minDiff) / (partLo + partHi));
	*peMin = max(0, (int32_t)*peMin);
  }
}


/*****************************************************************************
*
* function name: Calc_FactorSpendBitreservoir
* description:  calculates factor of spending bits for one frame
*                1.0 : take all frame dynpart bits
*                >1.0 : take all frame dynpart bits + bitres
*                <1.0 : put bits in bitreservoir
*  returns:      BitFac*100
*  input:        bitres-fullness, pe, blockType, parameter-settings
*  output:
*
*****************************************************************************/
static int16_t Calc_FactorSpendBitreservoir( const int16_t   bitresBits,
                                const int16_t   maxBitresBits,
                                const int16_t   pe,
                                const int16_t   windowSequence,
                                const int16_t   avgBits,
                                const int16_t   maxBitFac,
                                GM_ADJ_THR_BLK *AdjThr,
                                GM_ATS_PART   *adjThrChan)
{
  GM_BITRESER_ARGUM *bresParam;
  int16_t pex;
  int16_t fillLevel;
  int16_t bitSave, bitSpend, bitresFac;


  fillLevel = (int16_t)((100* bitresBits) / maxBitresBits);//extract_l

  if (windowSequence != SHORT_WINDOW)
    bresParam = &(AdjThr->bresParamLong);
  else
    bresParam = &(AdjThr->bresParamShort);

  pex = max(pe, adjThrChan->peMin);
  pex = min(pex,adjThrChan->peMax);

  bitSave = BitSavePercen(fillLevel,
                        bresParam->clipSaveLow, bresParam->clipSaveHigh,
                        bresParam->minBitSave, bresParam->maxBitSave);


  bitSpend = calcBitSpend(fillLevel,
                          bresParam->clipSpendLow, bresParam->clipSpendHigh,
                          bresParam->minBitSpend, bresParam->maxBitSpend);

  if(adjThrChan->peMax != adjThrChan->peMin)
	bitresFac = (100 - bitSave) + extract_l(((bitSpend + bitSave) * (pex - adjThrChan->peMin)) /
                    (adjThrChan->peMax - adjThrChan->peMin));
  else
	bitresFac = 0x7fff;


  bitresFac = min((int32_t)bitresFac,
                    (100-30 + extract_l((100 * bitresBits) / avgBits)));


  bitresFac = min(bitresFac, maxBitFac);

  Calc_PeMinPeMax(pe, &adjThrChan->peMin, &adjThrChan->peMax);

  return bitresFac;
}

/*****************************************************************************
*
* function name: ThrParam_Init
* description:  init thresholds parameter
*
*****************************************************************************/
void ThrParam_Init(GM_ADJ_THR_BLK 	*hAdjThr,
                const int32_t   meanPe,
                int32_t         chBitrate)
{
  GM_ATS_PART* atsElem = &hAdjThr->adjThrStateElem;
  GM_MINSNR_ADAPT_ARGUM *msaParam = &atsElem->minSnrAdaptParam;

  /* common for all elements: */
  /* parameters for bitres control */
  hAdjThr->bresParamLong.clipSaveLow   =  20;
  hAdjThr->bresParamLong.clipSaveHigh  =  95;
  hAdjThr->bresParamLong.minBitSave    =  -5;
  hAdjThr->bresParamLong.maxBitSave    =  30;
  hAdjThr->bresParamLong.clipSpendLow  =  20;
  hAdjThr->bresParamLong.clipSpendHigh =  95;
  hAdjThr->bresParamLong.minBitSpend   = -10;
  hAdjThr->bresParamLong.maxBitSpend   =  40;

  hAdjThr->bresParamShort.clipSaveLow   =  20;
  hAdjThr->bresParamShort.clipSaveHigh  =  75;
  hAdjThr->bresParamShort.minBitSave    =   0;
  hAdjThr->bresParamShort.maxBitSave    =  20;
  hAdjThr->bresParamShort.clipSpendLow  =  20;
  hAdjThr->bresParamShort.clipSpendHigh =  75;
  hAdjThr->bresParamShort.minBitSpend   = -5;
  hAdjThr->bresParamShort.maxBitSpend   =  50;

  /* specific for each element: */

  /* parameters for bitres control */
  atsElem->peMin = extract_l(((80*meanPe) / 100));
  atsElem->peMax = extract_l(((120*meanPe) / 100));

  /* additional pe offset to correct pe2bits for low bitrates */
  atsElem->peOffset = 0;
  if (chBitrate < 32000) {
    atsElem->peOffset = max(50, (100 - extract_l((100 * chBitrate) / 32000)));
  }

  /* avoid hole parameters */
  if (chBitrate > 20000) {
    atsElem->ahParam.modifyMinSnr = TRUE;
    atsElem->ahParam.startSfbL = 15;
    atsElem->ahParam.startSfbS = 3;
  }
  else {
    atsElem->ahParam.modifyMinSnr = FALSE;
    atsElem->ahParam.startSfbL = 0;
    atsElem->ahParam.startSfbS = 0;
  }

  /* minSnr adaptation */
  /* maximum reduction of minSnr goes down to minSnr^maxRed */
  msaParam->maxRed = 0x20000000;     /* *0.25f*/
  /* start adaptation of minSnr for avgEn/sfbEn > startRatio */
  msaParam->startRatio = 0x0ccccccd; /* 10 */
  /* maximum minSnr reduction to minSnr^maxRed is reached for
     avgEn/sfbEn >= maxRatio */
  msaParam->maxRatio =  0x0020c49c; /* 1000 */
  /* helper variables to interpolate minSnr reduction for
     avgEn/sfbEn between startRatio and maxRatio */

  msaParam->redRatioFac = 0xfb333333; /* -0.75/20 */

  msaParam->redOffs = 0x30000000;  /* msaParam->redRatioFac * 10*log10(msaParam->startRatio) */


  /* pe correction */
  atsElem->peLast = 0;
  atsElem->dynBitsLast = 0;
  atsElem->peCorrectionFactor = 100; /* 1.0 */

}

/*****************************************************************************
*
* function name: Perceptual_Entropy_Fact
* description:  calculates the desired perceptual entropy factor
*				It is between 0.85 and 1.15
*
*****************************************************************************/
static void Perceptual_Entropy_Fact(int16_t *correctionFac,
                             const int16_t peAct,
                             const int16_t peLast,
                             const int16_t bitsLast)
{
  int32_t peAct100 = 100 * peAct;
  int32_t peLast100 = 100 * peLast;
  int16_t peBitsLast = Conv_bitToPe(bitsLast);

  if ((bitsLast > 0) &&
      (peAct100 < (150 * peLast)) &&  (peAct100 > (70 * peLast)) &&
      ((120 * peBitsLast) > peLast100 ) && (( 65 * peBitsLast) < peLast100))
    {
      int16_t newFac = (100 * peLast) / peBitsLast;
      /* dead zone */

      if (newFac < 100) {
        newFac = min(((110 * newFac) / 100), 100);
        newFac = max((int32_t)newFac, 85);
      }
      else {
        newFac = max(((90 * newFac) / 100), 100);
        newFac = min((int32_t)newFac, 115);
      }

      if ((newFac > 100 && *correctionFac < 100) ||
          (newFac < 100 && *correctionFac > 100)) {
        *correctionFac = 100;
      }
      /* faster adaptation towards 1.0, slower in the other direction */

      if ((*correctionFac < 100 && newFac < *correctionFac) ||
          (*correctionFac > 100 && newFac > *correctionFac))
        *correctionFac = (85 * *correctionFac + 15 * newFac) / 100;
      else
        *correctionFac = (70 * *correctionFac + 30 * newFac) / 100;
      *correctionFac = min((int32_t)*correctionFac, 115);
      *correctionFac = max((int32_t)*correctionFac, 85);
    }
  else {
    *correctionFac = 100;
  }
}

/********************************************************************************
*
* function name: Calc_Thr
* description:  Adjust thresholds to the desired bitrate
*
**********************************************************************************/
GM_PERCEP_ENTRO_PART_DATA peData;
void Calc_Thr(GM_ADJ_THR_BLK   *adjThrState,
                      GM_ATS_PART     *AdjThrStateElement,
                      GM_PSY_OUT_CHANNEL  psyOutChannel[MAX_CHAN],
                      GM_PERCEP_OUT_PART *psyOutElement,
                      int16_t          *chBitDistribution,
                      int16_t           logSfbEnergy[MAX_CHAN][MAX_GROUPED_SFB],
                      int16_t           sfbNRelevantLines[MAX_CHAN][MAX_GROUPED_SFB],
                      GM_QC_OUTSTRM_PART  *qcOE,
					  GM_ELEMENT_BITS	  *elBits,
					  const int16_t     nChannels,
                      const int16_t     maxBitFac)
{
#if 0	//GM8287 audio driver limited the local variable size to 1024 bytes
  GM_PERCEP_ENTRO_PART_DATA peData = { 0 };
#endif
  int16_t noRedPe, grantedPe, grantedPeCorr;
  int16_t curWindowSequence;
  int16_t bitFactor;
  int16_t avgBits = (elBits->averageBits - (qcOE->staticBitsUsed + qcOE->ancBitsUsed));
  int16_t bitresBits = elBits->bitResLevel;
  int16_t maxBitresBits = elBits->maxBits;
  int16_t sideInfoBits = (qcOE->staticBitsUsed + qcOE->ancBitsUsed);
  int16_t ch;

  memset(&peData, 0, sizeof(GM_PERCEP_ENTRO_PART_DATA));
  Calc_SfbPe_First(&peData, psyOutChannel, logSfbEnergy, sfbNRelevantLines, nChannels, AdjThrStateElement->peOffset);

  /* pe without reduction */
  calc_ConstPartSfPe(&peData, psyOutChannel, nChannels);
  noRedPe = peData.pe;


  curWindowSequence = LONG_WINDOW;

  if (nChannels == 2) {

    if ((psyOutChannel[0].windowSequence == SHORT_WINDOW) ||
        (psyOutChannel[1].windowSequence == SHORT_WINDOW)) {
      curWindowSequence = SHORT_WINDOW;
    }
  }
  else {
    curWindowSequence = psyOutChannel[0].windowSequence;
  }


  /* bit factor */
  bitFactor = Calc_FactorSpendBitreservoir(bitresBits, maxBitresBits, noRedPe+5*sideInfoBits,
                               curWindowSequence, avgBits, maxBitFac,
                               adjThrState,
                               AdjThrStateElement);



  /* desired pe */
  grantedPe = ((bitFactor * Conv_bitToPe(avgBits)) / 100);


  /* correction of pe value */
  Perceptual_Entropy_Fact(&(AdjThrStateElement->peCorrectionFactor),
                   min(grantedPe, noRedPe),
                   AdjThrStateElement->peLast,
                   AdjThrStateElement->dynBitsLast);
  grantedPeCorr = (grantedPe * AdjThrStateElement->peCorrectionFactor) / 100;


  if (grantedPeCorr < noRedPe && noRedPe > peData.offset) {
    /* calc threshold necessary for desired pe */
    Calc_ThrToPe(psyOutChannel,
                        psyOutElement,
                        logSfbEnergy,
                        &peData,
                        nChannels,
                        grantedPeCorr,
                        &AdjThrStateElement->ahParam,
                        &AdjThrStateElement->minSnrAdaptParam);
  }

  /* calculate relative distribution */
  for (ch=0; ch<nChannels; ch++) {
    int16_t peOffsDiff = peData.pe - peData.offset;
    chBitDistribution[ch] = 200;

    if (peOffsDiff > 0) {
      int32_t temp = 1000 - (nChannels * 200);
      chBitDistribution[ch] = chBitDistribution[ch] +
		  (temp * peData.peChannelData[ch].pe) / peOffsDiff;
    }
  }

  /* store pe */
  qcOE->pe = noRedPe;

  /* update last pe */
  AdjThrStateElement->peLast = grantedPe;

}

/********************************************************************************
*
* function name: Save_DynBitThr
* description:  save dynBitsUsed for correction of Conv_bitToPe relation
*
**********************************************************************************/
void Save_DynBitThr(GM_ATS_PART *AdjThrStateElement,
                  const int16_t dynBitsUsed)
{
  AdjThrStateElement->dynBitsLast = dynBitsUsed;
}


/********************************************************************************
*
* function name:RedBandMinSnr
* description: reduce minSnr requirements for bands with relative low energies
*
**********************************************************************************/
static void RedBandMinSnr(GM_PSY_OUT_CHANNEL     psyOutChannel[MAX_CHAN],
                        int16_t              logSfbEnergy[MAX_CHAN][MAX_GROUPED_SFB],
                        GM_MINSNR_ADAPT_ARGUM *msaParam,
                        const int16_t        nChannels)
{
  int16_t ch, sfb, sfbOffs;//, shift;
  int32_t nSfb, avgEn;
  int16_t log_avgEn = 0;
  int32_t startRatio_x_avgEn = 0;

  for (ch=0; ch<nChannels; ch++) {
    GM_PSY_OUT_CHANNEL* psyOutChan = &psyOutChannel[ch];

    /* calc average energy per scalefactor band */
    avgEn = 0;
    nSfb = 0;
    for (sfbOffs=0; sfbOffs<psyOutChan->sfbCnt; sfbOffs+=psyOutChan->sfbPerGroup) {
      for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
        avgEn = L_add(avgEn, psyOutChan->sfbEnergy[sfbOffs+sfb]);
        nSfb = nSfb + 1;
      }
    }

    if (nSfb > 0) {
      int t;
	  avgEn = avgEn / nSfb;

      log_avgEn = iLog4(avgEn);
//      startRatio_x_avgEn = fixmul(msaParam->startRatio, avgEn);
#ifdef LINUX
		{
			int tmpp = msaParam->startRatio;
			asm volatile("smull %[o1], %[o2], %[i1], %[i2]" : [o1] "=r" (t), [o2] "=r" (startRatio_x_avgEn) : [i1] "r" (tmpp), [i2] "r" (avgEn));
		}
#else
	  __asm {
		  	SMULL	t, startRatio_x_avgEn, msaParam->startRatio, avgEn
	  }
#endif
	  startRatio_x_avgEn <<= 1;
    }


    /* reduce minSnr requirement by minSnr^minSnrRed dependent on avgEn/sfbEn */
    for (sfbOffs=0; sfbOffs<psyOutChan->sfbCnt; sfbOffs+=psyOutChan->sfbPerGroup) {
      for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
        if (psyOutChan->sfbEnergy[sfbOffs+sfb] < startRatio_x_avgEn) {
          int16_t dbRatio, minSnrRed;
          int32_t snrRed;
          int16_t newMinSnr;

          dbRatio = log_avgEn - logSfbEnergy[ch][sfbOffs+sfb];
          dbRatio = dbRatio + (dbRatio << 1);

          minSnrRed = 110 - ((dbRatio + (dbRatio << 1)) >> 2);
          minSnrRed = max((int32_t)minSnrRed, 20); /* 110: (0.375(redOffs)+1)*80,
                                               3: 0.00375(redRatioFac)*80
                                               20: 0.25(maxRed) * 80 */

          snrRed = minSnrRed * iLog4((psyOutChan->sfbMinSnr[sfbOffs+sfb] << 16));
          /*
             snrRedI si now scaled by 80 (minSnrRed) and 4 (ffr_iLog4)
          */

          newMinSnr = round16(pow2_xy(snrRed,80*4));

          psyOutChan->sfbMinSnr[sfbOffs+sfb] = min((int32_t)newMinSnr, minSnrLimit);
        }
      }
    }
  }

}



static const int16_t  C1_I = 12;    /* log(8.0)/log(2) *4         */
static const int32_t  C2_I = 10830; /* log(2.5)/log(2) * 1024 * 4 * 2 */
static const int16_t  C3_I = 573;   /* (1-C2/C1) *1024            */


/*****************************************************************************
*
* function name: prepareSfbPe
* description:  constants that do not change during successive pe calculations
*
**********************************************************************************/
void Calc_SfbPe_First(GM_PERCEP_ENTRO_PART_DATA *peData,
                  GM_PSY_OUT_CHANNEL  psyOutChannel[MAX_CHAN],
                  int16_t logSfbEnergy[MAX_CHAN][MAX_GROUPED_SFB],
                  int16_t sfbNRelevantLines[MAX_CHAN][MAX_GROUPED_SFB],
                  const int16_t nChannels,
                  const int16_t peOffset)
{
  int32_t sfbGrp, sfb;
  int32_t ch;

  for(ch=0; ch<nChannels; ch++) {
    GM_PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
    GM_PERCEP_ENTRO_CHANNEL_DATA *peChanData=&peData->peChannelData[ch];
    for(sfbGrp=0;sfbGrp<psyOutChan->sfbCnt; sfbGrp+=psyOutChan->sfbPerGroup){
      for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
	    peChanData->sfbNLines4[sfbGrp+sfb] = sfbNRelevantLines[ch][sfbGrp+sfb];
        sfbNRelevantLines[ch][sfbGrp+sfb] = sfbNRelevantLines[ch][sfbGrp+sfb] >> 2;
	    peChanData->sfbLdEnergy[sfbGrp+sfb] = logSfbEnergy[ch][sfbGrp+sfb];
      }
    }
  }
  peData->offset = peOffset;
}


/*****************************************************************************
*
* function name: calc_ConstPartSfPe
* description:  constPart is sfbPe without the threshold part n*ld(thr) or n*C3*ld(thr)
*
**********************************************************************************/
void calc_ConstPartSfPe(GM_PERCEP_ENTRO_PART_DATA *peData,
               GM_PSY_OUT_CHANNEL psyOutChannel[MAX_CHAN],
               const int16_t nChannels)
{
  int32_t ch;
  int32_t sfbGrp, sfb;
  int32_t nLines4;
  int32_t ldThr, ldRatio;
  int32_t pe, constPart, nActiveLines;
  //int32_t tmp;

  peData->pe = peData->offset;
  peData->constPart = 0;
  peData->nActiveLines = 0;
  for(ch=0; ch<nChannels; ch++) {
    GM_PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
    GM_PERCEP_ENTRO_CHANNEL_DATA *peChanData = &peData->peChannelData[ch];
    const int32_t *sfbEnergy = psyOutChan->sfbEnergy;
    const int32_t *sfbThreshold = psyOutChan->sfbThreshold;

    pe = 0;
    constPart = 0;
    nActiveLines = 0;

    for(sfbGrp=0; sfbGrp<psyOutChan->sfbCnt; sfbGrp+=psyOutChan->sfbPerGroup) {
      for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
        int32_t nrg = sfbEnergy[sfbGrp+sfb];
        int32_t thres = sfbThreshold[sfbGrp+sfb];
        int32_t sfbLDEn = peChanData->sfbLdEnergy[sfbGrp+sfb];

        if (nrg > thres) {
          ldThr = iLog4(thres);


          ldRatio = sfbLDEn - ldThr;

          nLines4 = peChanData->sfbNLines4[sfbGrp+sfb];

          /* sfbPe = nl*log2(en/thr)*/
		  if (ldRatio >= C1_I) {
            peChanData->sfbPe[sfbGrp+sfb] = (nLines4*ldRatio + 8) >> 4;
            peChanData->sfbConstPart[sfbGrp+sfb] = ((nLines4*sfbLDEn)) >> 4;
          }
          else {
		  /* sfbPe = nl*(c2 + c3*log2(en/thr))*/
            //peChanData->sfbPe[sfbGrp+sfb] = extract_l((L_mpy_wx(
            //        (C2_I + C3_I * ldRatio * 2) << 4, nLines4) + 4) >> 3);

		  peChanData->sfbPe[sfbGrp+sfb] = (int16_t)((L_mpy_wx(
                    (C2_I + C3_I * ldRatio * 2) << 4, nLines4) + 4) >> 3);

			//peChanData->sfbConstPart[sfbGrp+sfb] = extract_l(( L_mpy_wx(
            //        (C2_I + C3_I * sfbLDEn * 2) << 4, nLines4) + 4) >> 3);
            peChanData->sfbConstPart[sfbGrp+sfb] = (int16_t)(( L_mpy_wx(
                    (C2_I + C3_I * sfbLDEn * 2) << 4, nLines4) + 4) >> 3);
            nLines4 = (nLines4 * C3_I + (1024<<1)) >> 10;
          }
          peChanData->sfbNActiveLines[sfbGrp+sfb] = nLines4 >> 2;
        }
        else {
          peChanData->sfbPe[sfbGrp+sfb] = 0;
          peChanData->sfbConstPart[sfbGrp+sfb] = 0;
          peChanData->sfbNActiveLines[sfbGrp+sfb] = 0;
        }
        pe = pe + peChanData->sfbPe[sfbGrp+sfb];
        constPart = constPart + peChanData->sfbConstPart[sfbGrp+sfb];
        nActiveLines = nActiveLines + peChanData->sfbNActiveLines[sfbGrp+sfb];
      }
    }

	peChanData->pe = saturate(pe);
  peChanData->constPart = saturate(constPart);
  peChanData->nActiveLines = saturate(nActiveLines);


	pe += peData->pe;


	peData->pe = saturate(pe);
  constPart += peData->constPart;


	peData->constPart = saturate(constPart);
  nActiveLines += peData->nActiveLines;
	peData->nActiveLines = saturate(nActiveLines);
  }
}


/*******************************************************************************
	Content:	Pre echo control functions

*******************************************************************************/




/*****************************************************************************
*
* function name:InitPreEchoControl
* description: init pre echo control parameter
*
*****************************************************************************/
void InitPreEchoControl(int32_t *pbThresholdNm1,
                        int16_t  numPb,
                        int32_t *pbThresholdQuiet)
{
  int16_t pb;

  for(pb=0; pb<numPb; pb++) {
    pbThresholdNm1[pb] = pbThresholdQuiet[pb];
  }
}

/*****************************************************************************
*
* function name:PreEchoControl
* description: update shreshold to avoid pre echo
*			   thr(n) = max(rpmin*thrq(n), min(thrq(n), rpelev*thrq1(n)))
*
*
*****************************************************************************/
void PreEchoControl(int32_t *pbThresholdNm1,
                    int16_t  numPb,
                    int32_t  maxAllowedIncreaseFactor,
                    int16_t  minRemainingThresholdFactor,
                    int32_t *pbThreshold,
                    int16_t  mdctScale,
                    int16_t  mdctScalenm1)
{
  int32_t i;
  int32_t tmpThreshold1, tmpThreshold2;
  int32_t scaling;

  /* maxAllowedIncreaseFactor is hard coded to 2 */
  (void)maxAllowedIncreaseFactor;

  scaling = ((mdctScale - mdctScalenm1) << 1);

  if ( scaling > 0 ) {
    for(i = 0; i < numPb; i++) {
      tmpThreshold1 = pbThresholdNm1[i] >> (scaling-1);
      tmpThreshold2 = L_mpy_ls(pbThreshold[i], minRemainingThresholdFactor);

      /* copy thresholds to internal memory */
      pbThresholdNm1[i] = pbThreshold[i];


      if(pbThreshold[i] > tmpThreshold1) {
        pbThreshold[i] = tmpThreshold1;
      }

      if(tmpThreshold2 > pbThreshold[i]) {
        pbThreshold[i] = tmpThreshold2;
      }

    }
  }
  else {
    scaling = -scaling;
    for(i = 0; i < numPb; i++) {

      tmpThreshold1 = pbThresholdNm1[i] << 1;
      tmpThreshold2 = L_mpy_ls(pbThreshold[i], minRemainingThresholdFactor);

      /* copy thresholds to internal memory */
      pbThresholdNm1[i] = pbThreshold[i];


      if(((pbThreshold[i] >> scaling) > tmpThreshold1)) {
        pbThreshold[i] = tmpThreshold1 << scaling;
      }

      if(tmpThreshold2 > pbThreshold[i]) {
        pbThreshold[i] = tmpThreshold2;
      }

    }
  }
}
