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
	File:		quantizer_tool_2.c

	Content:	Scale factor estimation functions

*******************************************************************************/

#include "basic_op.h"
#include "oper_32b.h"
#include "quantizer_tool_2.h"
#include "quantizer_tool.h"
#include "huffman_1.h"
#include "aac_table.h"

static const int16_t MAX_SCF_DELTA = 60;

/*!
constants reference in comments

 C0 = 6.75f;
 C1 = -69.33295f;   -16/3*log(MAX_QUANT+0.5-logCon)/log(2)
 C2 = 4.0f;
 C3 = 2.66666666f;

  PE_C1 = 3.0f;        log(8.0)/log(2)
  PE_C2 = 1.3219281f;  log(2.5)/log(2)
  PE_C3 = 0.5593573f;  1-C2/C1

*/

#define FF_SQRT_BITS                    7
#define FF_SQRT_TABLE_SIZE              (1<<FF_SQRT_BITS - 1<<(FF_SQRT_BITS-2))
#define COEF08_31		0x66666666		/* 0.8*(1 << 31) */
#define PE_C1_8			24				/* PE_C1*8 */
#define PE_C2_16		21				/* PE_C2*8/PE_C3 */
#define PE_SCALE		0x059a			/* 0.7 * (1 << (15 - 1 - 3))*/

#define SCALE_ESTIMATE_COEF	0x5555		/* (8.8585/(4*log2(10))) * (1 << 15)*/

/*********************************************************************************
*
* function name: Calc_Sqrtdiv256
* description:  calculates sqrt(x)/256
*
**********************************************************************************/
__inline int32_t Calc_Sqrtdiv256(int32_t x)
{
	int32_t y;
	int32_t preshift, postshift;


	if (x==0) return 0;
	preshift  = norm_l(x) - (INT_BITS-1-FF_SQRT_BITS);
	postshift = preshift >> 1;
	preshift  = postshift << 1;
	postshift = postshift + 8;	  /* sqrt/256 */
	if(preshift >= 0)
		y = x << preshift;        /* now 1/4 <= y < 1 */
	else
		y = x >> (-preshift);
	y = formfac_sqrttable[y-32];

	if(postshift >= 0)
		y = y >> postshift;
	else
		y = y << (-postshift);

	return y;
}


/*********************************************************************************
*
* function name: EstFactorOneChann
* description:  calculate the form factor one channel
*				ffac(n) = sqrt(abs(X(k)) + sqrt(abs(X(k+1)) + ....
*
**********************************************************************************/
static void
EstFactorOneChann(int16_t *logSfbFormFactor,
                      int16_t *sfbNRelevantLines,
                      int16_t *logSfbEnergy,
                      GM_PSY_OUT_CHANNEL *psyOutChan)
{
	int32_t sfbw, sfbw1;
	int32_t i, j/*, t*/;
	int32_t sfbOffs, sfb;//, shift;

	sfbw = sfbw1 = 0;
	for (sfbOffs=0; sfbOffs<psyOutChan->sfbCnt; sfbOffs+=psyOutChan->sfbPerGroup){
		for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
			i = sfbOffs+sfb;
			if (psyOutChan->sfbEnergy[i] > psyOutChan->sfbThreshold[i]) {
				int32_t accu, avgFormFactor,iSfbWidth;
				int32_t *mdctSpec;
				sfbw = psyOutChan->sfbOffsets[i+1] - psyOutChan->sfbOffsets[i];
				iSfbWidth = invSBF[(sfbw >> 2) - 1];
				mdctSpec = psyOutChan->mdctSpectrum + psyOutChan->sfbOffsets[i];
				accu = 0;
				/* calc sum of sqrt(spec) */
				j = sfbw;
//				for (j=sfbw; j; j--) {
				do {
					accu += Calc_Sqrtdiv256(L_abs(*mdctSpec)); mdctSpec++;
				} while (--j!=0);
				logSfbFormFactor[i] = iLog4(accu);
				logSfbEnergy[i] = iLog4(psyOutChan->sfbEnergy[i]);
				avgFormFactor = fixmul(rsqrt(psyOutChan->sfbEnergy[i],INT_BITS), iSfbWidth);
//			    __asm {
//				  	SMULL	t, avgFormFactor, rsqrt(psyOutChan->sfbEnergy[i],INT_BITS), iSfbWidth
//				}
//				avgFormFactor <<= 1;

				avgFormFactor = rsqrt((int32_t)avgFormFactor,INT_BITS) >> 10;
				/* result is multiplied by 4 */
				if(avgFormFactor) {
					sfbNRelevantLines[i] = accu / avgFormFactor;
				} else {
					sfbNRelevantLines[i] = 0x7fff;
				}
			} else {
				/* set number of lines to zero */
				sfbNRelevantLines[i] = 0;
			}
		}
	}
}

/*********************************************************************************
*
* function name: BetterScalFact
* description:  find better scalefactor with analysis by synthesis
*
**********************************************************************************/
static int16_t BetterScalFact(int32_t *spec,
                         int16_t  sfbWidth,
                         int32_t  thresh,
                         int16_t  scf,
                         int16_t  minScf,
                         int32_t *dist,
                         int16_t *minScfCalculated)
{
	int32_t cnt;
	int32_t sfbDist;
	int32_t scfBest;
	int32_t thresh125 = L_add(thresh, (thresh >> 2));

	scfBest = scf;

	/* calc real distortion */
	sfbDist = Quant_Line2Distor(spec, sfbWidth, scf);
	*minScfCalculated = scf;
	if(!sfbDist)
	  return scfBest;

	if (sfbDist > thresh125) {
		int32_t scfEstimated;
		int32_t sfbDistBest;
		scfEstimated = scf;
		sfbDistBest = sfbDist;

		cnt = 0;
		while (sfbDist > thresh125 && (cnt < 3)) {

			scf = scf + 1;
			sfbDist = Quant_Line2Distor(spec, sfbWidth, scf);

			if (sfbDist < sfbDistBest) {
				scfBest = scf;
				sfbDistBest = sfbDist;
			}
			cnt = cnt + 1;
		}
		cnt = 0;
		scf = scfEstimated;
		sfbDist = sfbDistBest;
		while ((sfbDist > thresh125) && (cnt < 1) && (scf > minScf)) {

			scf = scf - 1;
			sfbDist = Quant_Line2Distor(spec, sfbWidth, scf);

			if (sfbDist < sfbDistBest) {
				scfBest = scf;
				sfbDistBest = sfbDist;
			}
			*minScfCalculated = scf;
			cnt = cnt + 1;
		}
		*dist = sfbDistBest;
	}
	else {
		int32_t sfbDistBest;
		int32_t sfbDistAllowed;
		int32_t thresh08 = fixmul(COEF08_31, thresh);
		sfbDistBest = sfbDist;

		if (sfbDist < thresh08)
			sfbDistAllowed = sfbDist;
		else
			sfbDistAllowed = thresh08;
		for (cnt=0; cnt<3; cnt++) {
			scf = scf + 1;
			sfbDist = Quant_Line2Distor(spec, sfbWidth, scf);

			if (fixmul(COEF08_31,sfbDist) < sfbDistAllowed) {
				*minScfCalculated = scfBest + 1;
				scfBest = scf;
				sfbDistBest = sfbDist;
			}
		}
		*dist = sfbDistBest;
	}

	/* return best scalefactor */
	return scfBest;
}

/*********************************************************************************
*
* function name: CalcScf_SingleBitsHuffm
* description:  count single scf bits in huffum
*
**********************************************************************************/
static int16_t CalcScf_SingleBitsHuffm(int16_t scf, int16_t scfLeft, int16_t scfRight)
{
	int16_t scfBits;

	//scfBits = Get_HuffTableScf(scfLeft - scf) +
	//	Get_HuffTableScf(scf - scfRight);
//return(huff_ltabscf[delta+CODE_BOOK_SCF_LAV]);
    scfBits = huff_ltabscf[scfLeft - scf+CODE_BOOK_SCF_LAV] +
		huff_ltabscf[scf - scfRight+CODE_BOOK_SCF_LAV];

	return scfBits;
}

/*********************************************************************************
*
* function name: Count_SinglePe
* description:  ldRatio = log2(en(n)) - 0,375*scfGain(n)
*				nbits = 0.7*nLines*ldRation for ldRation >= c1
*				nbits = 0.7*nLines*(c2 + c3*ldRatio) for ldRation < c1
*
**********************************************************************************/
static int16_t Count_SinglePe(int16_t scf, int16_t sfbConstPePart, int16_t nLines)
{
	int32_t specPe;
	int32_t ldRatio;
	int32_t scf3;

	ldRatio = sfbConstPePart << 3; /*  (sfbConstPePart -0.375*scf)*8 */
	scf3 = scf + scf + scf;
	ldRatio = ldRatio - scf3;

	if (ldRatio < PE_C1_8) {
		/* 21 : 2*8*PE_C2, 2*PE_C3 ~ 1*/
		ldRatio = (ldRatio + PE_C2_16) >> 1;
	}
	specPe = nLines * ldRatio;
	specPe = (specPe * PE_SCALE) >> 14;

	return saturate(specPe);
}


/*********************************************************************************
*
* function name: Scf_CountDiffBit
* description:  count different scf bits used
*
**********************************************************************************/
static int16_t Scf_CountDiffBit(int16_t *scfOld, int16_t *scfNew,
                               int16_t sfbCnt, int16_t startSfb, int16_t stopSfb)
{
	int32_t scfBitsDiff;
	int32_t sfb, sfbLast;
	int32_t sfbPrev, sfbNext;

	scfBitsDiff = 0;
	sfb = 0;

	/* search for first relevant sfb */
	sfbLast = startSfb;
	while (sfbLast < stopSfb && scfOld[sfbLast] == GMAAC_SHRT_MIN) {

		sfbLast = sfbLast + 1;
	}
	/* search for previous relevant sfb and count diff */
	sfbPrev = startSfb - 1;
	while ((sfbPrev>=0) && scfOld[sfbPrev] == GMAAC_SHRT_MIN) {

		sfbPrev = sfbPrev - 1;
	}

	if (sfbPrev>=0) {
		//scfBitsDiff += Get_HuffTableScf(scfNew[sfbPrev] - scfNew[sfbLast]) -
		//	Get_HuffTableScf(scfOld[sfbPrev] - scfOld[sfbLast]);
		//return(huff_ltabscf[delta+CODE_BOOK_SCF_LAV]);
		scfBitsDiff += huff_ltabscf[scfNew[sfbPrev] - scfNew[sfbLast]+CODE_BOOK_SCF_LAV] -
			huff_ltabscf[scfOld[sfbPrev] - scfOld[sfbLast]+CODE_BOOK_SCF_LAV];
	}
	/* now loop through all sfbs and count diffs of relevant sfbs */
	for (sfb=sfbLast+1; sfb<stopSfb; sfb++) {

		if (scfOld[sfb] != GMAAC_SHRT_MIN) {
			//scfBitsDiff += Get_HuffTableScf(scfNew[sfbLast] - scfNew[sfb]) -
			//	Get_HuffTableScf(scfOld[sfbLast] - scfOld[sfb]);
            scfBitsDiff += huff_ltabscf[scfNew[sfbLast] - scfNew[sfb]+CODE_BOOK_SCF_LAV] -
				huff_ltabscf[scfOld[sfbLast] - scfOld[sfb]+CODE_BOOK_SCF_LAV];
			sfbLast = sfb;
		}
	}
	/* search for next relevant sfb and count diff */
	sfbNext = stopSfb;
	while (sfbNext < sfbCnt && scfOld[sfbNext] == GMAAC_SHRT_MIN) {

		sfbNext = sfbNext + 1;
	}

	if (sfbNext < sfbCnt)
		//scfBitsDiff += Get_HuffTableScf(scfNew[sfbLast] - scfNew[sfbNext]) -
		//Get_HuffTableScf(scfOld[sfbLast] - scfOld[sfbNext]);
		scfBitsDiff += huff_ltabscf[scfNew[sfbLast] - scfNew[sfbNext]+CODE_BOOK_SCF_LAV] -
		               huff_ltabscf[scfOld[sfbLast] - scfOld[sfbNext]+CODE_BOOK_SCF_LAV];

	return saturate(scfBitsDiff);
}

/*
*/
static int16_t Count_PeDiff(int16_t *scfOld,
                             int16_t *scfNew,
                             int16_t *sfbConstPePart,
                             int16_t *logSfbEnergy,
                             int16_t *logSfbFormFactor,
                             int16_t *sfbNRelevantLines,
                             int16_t startSfb,
                             int16_t stopSfb)
{
	int32_t specPeDiff;
	int32_t sfb;

	specPeDiff = 0;

	/* loop through all sfbs and count pe difference */
	for (sfb=startSfb; sfb<stopSfb; sfb++) {


		if (scfOld[sfb] != GMAAC_SHRT_MIN) {
			int32_t ldRatioOld, ldRatioNew;
			int32_t scf3;


			if (sfbConstPePart[sfb] == MIN_16) {
				sfbConstPePart[sfb] = ((logSfbEnergy[sfb] -
					logSfbFormFactor[sfb]) + 11-8*4+3) >> 2;
			}


			ldRatioOld = sfbConstPePart[sfb] << 3;
			scf3 = scfOld[sfb] + scfOld[sfb] + scfOld[sfb];
			ldRatioOld = ldRatioOld - scf3;
			ldRatioNew = sfbConstPePart[sfb] << 3;
			scf3 = scfNew[sfb] + scfNew[sfb] + scfNew[sfb];
			ldRatioNew = ldRatioNew - scf3;

			if (ldRatioOld < PE_C1_8) {
				/* 21 : 2*8*PE_C2, 2*PE_C3 ~ 1*/
				ldRatioOld = (ldRatioOld + PE_C2_16) >> 1;
			}

			if (ldRatioNew < PE_C1_8) {
				/* 21 : 2*8*PE_C2, 2*PE_C3 ~ 1*/
				ldRatioNew = (ldRatioNew + PE_C2_16) >> 1;
			}

			specPeDiff +=  sfbNRelevantLines[sfb] * (ldRatioNew - ldRatioOld);
		}
	}

	specPeDiff = (specPeDiff * PE_SCALE) >> 14;

	return saturate(specPeDiff);
}


/*********************************************************************************
*
* function name: Scf_SearchSingleBands
* description:  searched for single scalefactor bands, where the number of bits gained
*				by using a smaller scfgain(n) is greater than the estimated increased
*				bit demand
*
**********************************************************************************/
static void Scf_SearchSingleBands(GM_PSY_OUT_CHANNEL *psyOutChan,
                                int16_t *scf,
                                int16_t *minScf,
                                int32_t *sfbDist,
                                int16_t *sfbConstPePart,
                                int16_t *logSfbEnergy,
                                int16_t *logSfbFormFactor,
                                int16_t *sfbNRelevantLines,
                                int16_t *minScfCalculated,
                                Flag    restartOnSuccess)
{
	int16_t sfbLast, sfbAct, sfbNext, scfAct, scfMin;
	int16_t *scfLast, *scfNext;
	int32_t sfbPeOld, sfbPeNew;
	int32_t sfbDistNew;
	int32_t j;
	Flag   success;
	int16_t deltaPe, deltaPeNew, deltaPeTmp;
	int16_t *prevScfLast = psyOutChan->prevScfLast;
	int16_t *prevScfNext = psyOutChan->prevScfNext;
	int16_t *deltaPeLast = psyOutChan->deltaPeLast;
	Flag   updateMinScfCalculated;

	success = 0;
	deltaPe = 0;

	for(j=0;j<psyOutChan->sfbCnt;j++){
		prevScfLast[j] = MAX_16;
		prevScfNext[j] = MAX_16;
		deltaPeLast[j] = MAX_16;
	}

	sfbLast = -1;
	sfbAct = -1;
	sfbNext = -1;
	scfLast = 0;
	scfNext = 0;
	scfMin = MAX_16;
	do {
		/* search for new relevant sfb */
		sfbNext = sfbNext + 1;
		while (sfbNext < psyOutChan->sfbCnt && scf[sfbNext] == MIN_16) {

			sfbNext = sfbNext + 1;
		}

		if ((sfbLast>=0) && (sfbAct>=0) && sfbNext < psyOutChan->sfbCnt) {
			/* relevant scfs to the left and to the right */
			scfAct  = scf[sfbAct];
			scfLast = scf + sfbLast;
			scfNext = scf + sfbNext;
			scfMin  = min(*scfLast, *scfNext);
		}
		else {

			if (sfbLast == -1 && (sfbAct>=0) && sfbNext < psyOutChan->sfbCnt) {
				/* first relevant scf */
				scfAct  = scf[sfbAct];
				scfLast = &scfAct;
				scfNext = scf + sfbNext;
				scfMin  = *scfNext;
			}
			else {

				if ((sfbLast>=0) && (sfbAct>=0) && sfbNext == psyOutChan->sfbCnt) {
					/* last relevant scf */
					scfAct  = scf[sfbAct];
					scfLast = scf + sfbLast;
					scfNext = &scfAct;
					scfMin  = *scfLast;
				}
			}
		}

		if (sfbAct>=0)
			scfMin = max(scfMin, minScf[sfbAct]);

		if ((sfbAct >= 0) &&
			(sfbLast>=0 || sfbNext < psyOutChan->sfbCnt) &&
			scfAct > scfMin &&
			(*scfLast != prevScfLast[sfbAct] ||
			*scfNext != prevScfNext[sfbAct] ||
			deltaPe < deltaPeLast[sfbAct])) {
			success = 0;

			/* estimate required bits for actual scf */
			if (sfbConstPePart[sfbAct] == MIN_16) {
				sfbConstPePart[sfbAct] = logSfbEnergy[sfbAct] -
					logSfbFormFactor[sfbAct] + 11-8*4; /* 4*log2(6.75) - 32 */

				if (sfbConstPePart[sfbAct] < 0)
					sfbConstPePart[sfbAct] = sfbConstPePart[sfbAct] + 3;
				sfbConstPePart[sfbAct] = sfbConstPePart[sfbAct] >> 2;
			}

			sfbPeOld = Count_SinglePe(scfAct, sfbConstPePart[sfbAct], sfbNRelevantLines[sfbAct]) +
				CalcScf_SingleBitsHuffm(scfAct, *scfLast, *scfNext);
			deltaPeNew = deltaPe;
			updateMinScfCalculated = 1;
			do {
				scfAct = scfAct - 1;
				/* check only if the same check was not done before */

				if (scfAct < minScfCalculated[sfbAct]) {
					sfbPeNew = Count_SinglePe(scfAct, sfbConstPePart[sfbAct], sfbNRelevantLines[sfbAct]) +
						CalcScf_SingleBitsHuffm(scfAct, *scfLast, *scfNext);
					/* use new scf if no increase in pe and
					quantization error is smaller */
					deltaPeTmp = deltaPe + sfbPeNew - sfbPeOld;

					if (deltaPeTmp < 10) {
						sfbDistNew = Quant_Line2Distor(psyOutChan->mdctSpectrum+
							psyOutChan->sfbOffsets[sfbAct],
							(psyOutChan->sfbOffsets[sfbAct+1] - psyOutChan->sfbOffsets[sfbAct]),
							scfAct);
						if (sfbDistNew < sfbDist[sfbAct]) {
							/* success, replace scf by new one */
							scf[sfbAct] = scfAct;
							sfbDist[sfbAct] = sfbDistNew;
							deltaPeNew = deltaPeTmp;
							success = 1;
						}
						/* mark as already checked */

						if (updateMinScfCalculated) {
							minScfCalculated[sfbAct] = scfAct;
						}
					}
					else {
						updateMinScfCalculated = 0;
					}
				}

			} while (scfAct > scfMin);
			deltaPe = deltaPeNew;
			/* save parameters to avoid multiple computations of the same sfb */
			prevScfLast[sfbAct] = *scfLast;
			prevScfNext[sfbAct] = *scfNext;
			deltaPeLast[sfbAct] = deltaPe;
		}

		if (success && restartOnSuccess) {
			/* start again at first sfb */
			sfbLast = -1;
			sfbAct  = -1;
			sfbNext = -1;
			scfLast = 0;
			scfNext = 0;
			scfMin  = MAX_16;
			success = 0;
		}
		else {
			/* shift sfbs for next band */
			sfbLast = sfbAct;
			sfbAct  = sfbNext;
		}

  } while (sfbNext < psyOutChan->sfbCnt);
}


/*********************************************************************************
*
* function name: Scf_DiffReduction
* description:  scalefactor difference reduction
*               assimilateMultipleScf
**********************************************************************************/
static void Scf_DiffReduction(GM_PSY_OUT_CHANNEL *psyOutChan,
                                  int16_t *scf,
                                  int16_t *minScf,
                                  int32_t *sfbDist,
                                  int16_t *sfbConstPePart,
                                  int16_t *logSfbEnergy,
                                  int16_t *logSfbFormFactor,
                                  int16_t *sfbNRelevantLines)
{
	int32_t sfb, startSfb, stopSfb, scfMin, scfMax, scfAct;
	Flag   possibleRegionFound;
	int32_t deltaScfBits;
	int32_t deltaSpecPe;
	int32_t deltaPe, deltaPeNew;
	int32_t sfbCnt;
	int32_t *sfbDistNew = psyOutChan->sfbDistNew;
	int16_t *scfTmp = psyOutChan->prevScfLast;

	deltaPe = 0;
	sfbCnt = psyOutChan->sfbCnt;

	/* calc min and max scalfactors */
	scfMin = MAX_16;
	scfMax = MIN_16;
	sfb = sfbCnt-1;
	do {
		if (scf[sfb] != MIN_16) {
			scfMin = min(scfMin, (int32_t)scf[sfb]);
			scfMax = max(scfMax, (int32_t)scf[sfb]);
		}
	} while(--sfb>=0);
//	for (sfb=0; sfb<sfbCnt; sfb++) {
//		if (scf[sfb] != MIN_16) {
//			scfMin = min(scfMin, scf[sfb]);
//			scfMax = max(scfMax, scf[sfb]);
//		}
//	}

	if (scfMax !=  MIN_16) {

		scfAct = scfMax;

		do {
			scfAct = scfAct - 1;
			for (sfb=0; sfb<sfbCnt; sfb++) {
				scfTmp[sfb] = scf[sfb];
			}
			stopSfb = 0;
			do {
				sfb = stopSfb;

				while (sfb < sfbCnt && (scf[sfb] == MIN_16 || scf[sfb] <= scfAct)) {
					sfb = sfb + 1;
				}
				startSfb = sfb;
				sfb = sfb + 1;

				while (sfb < sfbCnt && (scf[sfb] == MIN_16 || scf[sfb] > scfAct)) {
					sfb = sfb + 1;
				}
				stopSfb = sfb;

				possibleRegionFound = 0;

				if (startSfb < sfbCnt) {
					possibleRegionFound = 1;
					for (sfb=startSfb; sfb<stopSfb; sfb++) {

						if (scf[sfb]!=MIN_16) {

							if (scfAct < minScf[sfb]) {
								possibleRegionFound = 0;
								break;
							}
						}
					}
				}


				if (possibleRegionFound) { /* region found */
					/* replace scfs in region by scfAct */
					for (sfb=startSfb; sfb<stopSfb; sfb++) {

						if (scfTmp[sfb]!=MIN_16)
							scfTmp[sfb] = scfAct;
					}

					/* estimate change in bit demand for new scfs */
					deltaScfBits = Scf_CountDiffBit(scf,scfTmp,sfbCnt,startSfb,stopSfb);
					deltaSpecPe = Count_PeDiff(scf, scfTmp, sfbConstPePart,
						logSfbEnergy, logSfbFormFactor, sfbNRelevantLines,
						startSfb, stopSfb);
					deltaPeNew = deltaPe + deltaScfBits + deltaSpecPe;


					if (deltaPeNew < 10) {
						int32_t distOldSum, distNewSum;

						/* quantize and calc sum of new distortion */
						distOldSum = 0;
						distNewSum = 0;
						for (sfb=startSfb; sfb<stopSfb; sfb++) {

							if (scfTmp[sfb] != MIN_16) {
								distOldSum = L_add(distOldSum, sfbDist[sfb]);

								sfbDistNew[sfb] = Quant_Line2Distor(psyOutChan->mdctSpectrum +
									psyOutChan->sfbOffsets[sfb],
									(psyOutChan->sfbOffsets[sfb+1] - psyOutChan->sfbOffsets[sfb]),
									scfAct);


								if (sfbDistNew[sfb] > psyOutChan->sfbThreshold[sfb]) {
									distNewSum = distOldSum << 1;
									break;
								}
								distNewSum = L_add(distNewSum, sfbDistNew[sfb]);
							}
						}

						if (distNewSum < distOldSum) {
							deltaPe = deltaPeNew;
							for (sfb=startSfb; sfb<stopSfb; sfb++) {

								if (scf[sfb]!=MIN_16) {
									scf[sfb] = scfAct;
									sfbDist[sfb] = sfbDistNew[sfb];
								}
							}
						}
					}
				}
			} while (stopSfb <= sfbCnt);
		} while (scfAct > scfMin);
	}
}

/*********************************************************************************
*
* function name: CalcScaleFactOneChann
* description:  estimate scale factors for one channel
*
**********************************************************************************/
static void
CalcScaleFactOneChann(GM_PSY_OUT_CHANNEL *psyOutChan,
                            int16_t          *scf,
                            int16_t          *globalGain,
                            int16_t          *logSfbEnergy,
                            int16_t          *logSfbFormFactor,
                            int16_t          *sfbNRelevantLines)
{
	int32_t i, j;
	int32_t thresh, energy;
	int32_t energyPart, thresholdPart;
	int32_t scfInt, minScf, maxScf, maxAllowedScf, lastSf;
	int32_t maxSpec;
	int32_t *sfbDist = psyOutChan->sfbDist;
	int16_t *minSfMaxQuant = psyOutChan->minSfMaxQuant;
	int16_t *minScfCalculated = psyOutChan->minScfCalculated;


	for (i=0; i<psyOutChan->sfbCnt; i++) {
		int32_t sbfwith, sbfStart;
		int32_t *mdctSpec;
		thresh = psyOutChan->sfbThreshold[i];
		energy = psyOutChan->sfbEnergy[i];

		sbfStart = psyOutChan->sfbOffsets[i];
		sbfwith = psyOutChan->sfbOffsets[i+1] - sbfStart;
		mdctSpec = psyOutChan->mdctSpectrum+sbfStart;

		maxSpec = 0;
		/* maximum of spectrum */
		for (j=sbfwith; j; j-- ) {
			int32_t absSpec = L_abs(*mdctSpec); mdctSpec++;
			maxSpec |= absSpec;
		}

		/* scfs without energy or with thresh>energy are marked with MIN_16 */
		scf[i] = MIN_16;
		minSfMaxQuant[i] = MIN_16;

		if ((maxSpec > 0) && (energy > thresh)) {

			energyPart = logSfbFormFactor[i];
			thresholdPart = iLog4(thresh);
			/* -20 = 4*log2(6.75) - 32 */
			scfInt = ((thresholdPart - energyPart - 20) * SCALE_ESTIMATE_COEF) >> 15;

			minSfMaxQuant[i] = iLog4(maxSpec) - 68; /* 68  -16/3*log(MAX_QUANT+0.5-logCon)/log(2) + 1 */


			if (minSfMaxQuant[i] > scfInt) {
				scfInt = minSfMaxQuant[i];
			}

			/* find better scalefactor with analysis by synthesis */
			scfInt = BetterScalFact(psyOutChan->mdctSpectrum+sbfStart,
				sbfwith,
				thresh, scfInt, minSfMaxQuant[i],
				&sfbDist[i], &minScfCalculated[i]);

			scf[i] = scfInt;
		}
	}


	/* scalefactor differece reduction  */
	{
		int16_t sfbConstPePart[MAX_GROUPED_SFB];
		for(i=0;i<psyOutChan->sfbCnt;i++) {
			sfbConstPePart[i] = MIN_16;
		}

		Scf_SearchSingleBands(psyOutChan, scf,
			minSfMaxQuant, sfbDist, sfbConstPePart, logSfbEnergy,
			logSfbFormFactor, sfbNRelevantLines, minScfCalculated, 1);

		Scf_DiffReduction(psyOutChan, scf,
			minSfMaxQuant, sfbDist, sfbConstPePart, logSfbEnergy,
			logSfbFormFactor, sfbNRelevantLines);
	}

	/* get max scalefac for global gain */
	maxScf = MIN_16;
	minScf = MAX_16;
	for (i=0; i<psyOutChan->sfbCnt; i++) {

		if (maxScf < scf[i]) {
			maxScf = scf[i];
		}

		if ((scf[i] != MIN_16) && (minScf > scf[i])) {
			minScf = scf[i];
		}
	}
	/* limit scf delta */
	maxAllowedScf = minScf + MAX_SCF_DELTA;
	for(i=0; i<psyOutChan->sfbCnt; i++) {

		if ((scf[i] != MIN_16) && (maxAllowedScf < scf[i])) {
			scf[i] = maxAllowedScf;
		}
	}
	/* new maxScf if any scf has been limited */

	if (maxAllowedScf < maxScf) {
		maxScf = maxAllowedScf;
	}

	/* calc loop scalefactors */

	if (maxScf > MIN_16) {
		*globalGain = maxScf;
		lastSf = 0;

		for(i=0; i<psyOutChan->sfbCnt; i++) {

			if (scf[i] == MIN_16) {
				scf[i] = lastSf;
				/* set band explicitely to zero */
				for (j=psyOutChan->sfbOffsets[i]; j<psyOutChan->sfbOffsets[i+1]; j++) {
					psyOutChan->mdctSpectrum[j] = 0;
				}
			}
			else {
				scf[i] = maxScf - scf[i];
				lastSf = scf[i];
			}
		}
	}
	else{
		*globalGain = 0;
		/* set spectrum explicitely to zero */
		for(i=0; i<psyOutChan->sfbCnt; i++) {
			scf[i] = 0;
			for (j=psyOutChan->sfbOffsets[i]; j<psyOutChan->sfbOffsets[i+1]; j++) {
				psyOutChan->mdctSpectrum[j] = 0;
			}
		}
	}
}

/*********************************************************************************
*
* function name: EstimFactAllChan
* description:  estimate Form factors for all channel
*
**********************************************************************************/
void
EstimFactAllChan(int16_t logSfbFormFactor[MAX_CHAN][MAX_GROUPED_SFB],
               int16_t sfbNRelevantLines[MAX_CHAN][MAX_GROUPED_SFB],
               int16_t logSfbEnergy[MAX_CHAN][MAX_GROUPED_SFB],
               GM_PSY_OUT_CHANNEL psyOutChannel[MAX_CHAN],
               const int16_t nChannels)
{
	int16_t j;

	for (j=0; j<nChannels; j++) {
		EstFactorOneChann(logSfbFormFactor[j], sfbNRelevantLines[j], logSfbEnergy[j], &psyOutChannel[j]);
	}
}

/*********************************************************************************
*
* function name: CalcScaleFactAllChan
* description:  estimate scale factors for all channel
*
**********************************************************************************/
void
CalcScaleFactAllChan(GM_PSY_OUT_CHANNEL psyOutChannel[MAX_CHAN],
                     GM_QC_CHANNDATA_OUT  qcOutChannel[MAX_CHAN],
                     int16_t          logSfbEnergy[MAX_CHAN][MAX_GROUPED_SFB],
                     int16_t          logSfbFormFactor[MAX_CHAN][MAX_GROUPED_SFB],
                     int16_t          sfbNRelevantLines[MAX_CHAN][MAX_GROUPED_SFB],
                     const int16_t    nChannels)
{
	int16_t j;

	for (j=0; j<nChannels; j++) {
		CalcScaleFactOneChann(&psyOutChannel[j],
			qcOutChannel[j].scf,
			&(qcOutChannel[j].globalGain),
			logSfbEnergy[j],
			logSfbFormFactor[j],
			sfbNRelevantLines[j]);
	}
}
