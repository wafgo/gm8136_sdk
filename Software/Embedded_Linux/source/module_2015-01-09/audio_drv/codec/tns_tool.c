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
	File:		tns_tool.c

	Content:	Definition TNS tools functions

*******************************************************************************/

#include "basic_op.h"
#include "oper_32b.h"
#include "aac_table.h"
#include "perceptual_const.h"
#include "tns_tool.h"
#include "tns_param.h"
#include "perceptual_cfg.h"
#include "tns_func.h"

#define TNS_MODIFY_BEGIN         2600  /* Hz */
#define RATIO_PATCH_LOWER_BORDER 380   /* Hz */
#define TNS_GAIN_THRESH			 141   /* 1.41*100 */
#define NORM_COEF				 0x028f5c28
#define assert(expr) \
	if (!(expr)) { \
	printk( "Assertion failed! %s,%s,%s,line=%d\n",\
	#expr,__FILE__,__func__,__LINE__); \
	BUG(); \
	}

static const int32_t TNS_PARCOR_THRESH = 0x0ccccccd; /* 0.1*(1 << 31) */
/* Limit bands to > 2.0 kHz */
static unsigned short tnsMinBandNumberLong[12] =
{ 11, 12, 15, 16, 17, 20, 25, 26, 24, 28, 30, 31 };
static unsigned short tnsMinBandNumberShort[12] =
{ 2, 2, 2, 3, 3, 4, 6, 6, 8, 10, 10, 12 };

/**************************************/
/* Main/Low Profile TNS Parameters    */
/**************************************/
static unsigned short tnsMaxBandsLongMainLow[12] =
{ 31, 31, 34, 40, 42, 51, 46, 46, 42, 42, 42, 39 };

static unsigned short tnsMaxBandsShortMainLow[12] =
{ 9, 9, 10, 14, 14, 14, 14, 14, 14, 14, 14, 14 };


static void Tns_CalcLPCWeightedSpectrum(const int32_t spectrum[],
                                 int16_t weightedSpectrum[],
                                 int32_t* sfbEnergy,
                                 const int16_t* sfbOffset, int16_t lpcStartLine,
                                 int16_t lpcStopLine, int16_t lpcStartBand,int16_t lpcStopBand,
                                 int32_t *pWork32);



void Tns_AutoCorre(const int16_t input[], int32_t corr[],
                            int16_t samples, int16_t corrCoeff);
static int16_t Tns_AutocorrelationToParcorCoef(int32_t workBuffer[], int32_t reflCoeff[], int16_t numOfCoeff);

static int16_t Tns_CalcLPCFilter(const int16_t* signal, const int32_t window[], int16_t numOfLines,
                                              int16_t tnsOrder, int32_t parcor[]);


static void Tns_ParcorCoef2Index(const int32_t parcor[], int16_t index[], int16_t order,
                         int16_t bitsPerCoeff);

static void Tns_Index2ParcorCoef(const int16_t index[], int32_t parcor[], int16_t order,
                         int16_t bitsPerCoeff);



static void Tns_AnalysisFilter(const int32_t signal[], int16_t numOfLines,
                                  const int32_t parCoeff[], int16_t order,
                                  int32_t output[]);


/**
*
* function name: Tns_FreqToBandWithRound
* description:  Retrieve index of nearest band border
* returnt:		index
*
*/
static int16_t Tns_FreqToBandWithRound(int32_t freq,                   /*!< frequency in Hertz */
                                     int32_t fs,                     /*!< Sampling frequency in Hertz */
                                     int16_t numOfBands,             /*!< total number of bands */
                                     const int16_t *bandStartOffset) /*!< table of band borders */
{
  int32_t lineNumber, band;
  int32_t temp, shift;

  /*  assert(freq >= 0);  */
  shift = norm_l(fs);
  lineNumber = (extract_l(fixmul((bandStartOffset[numOfBands] << 2),Div_32(freq << shift,fs << shift))) + 1) >> 1;

  /* freq > fs/2 */
  temp = lineNumber - bandStartOffset[numOfBands] ;
  if (temp >= 0)
    return numOfBands;

  /* find band the line number lies in */
  for (band=0; band<numOfBands; band++) {
    temp = bandStartOffset[band + 1] - lineNumber;
    if (temp > 0) break;
  }

  temp = (lineNumber - bandStartOffset[band]);
  temp = (temp - (bandStartOffset[band + 1] - lineNumber));
  if ( temp > 0 )
  {
    band = band + 1;
  }

  return extract_l(band);
}


/**
*
* function name: Init_TnsCfgLongBlk
* description:  Fill GM_TNS_CFG_INFO structure with sensible content for long blocks
* returns:		0 if success
*
*/
int16_t Init_TnsCfgLongBlk(int32_t bitRate,          /*!< bitrate */
                                int32_t sampleRate,          /*!< Sampling frequency */
                                int16_t channels,            /*!< number of channels */
                                GM_TNS_CFG_INFO *tC,             /*!< TNS Config struct (modified) */
                                GM_PSY_CFG_LONGBLK *pC, /*!< psy config struct */
                                int16_t active)              /*!< tns active flag */
{

  int32_t bitratePerChannel;
  tC->maxOrder     = TNS_MAX_ORDER;
  tC->tnsStartFreq = 1275;
  tC->coefRes      = 4;

  /* to avoid integer division */
  if ( sub(channels,2) == 0 ) {
    bitratePerChannel = bitRate >> 1;
  }
  else {
    bitratePerChannel = bitRate;
  }

  tC->tnsMaxSfb = tnsMaxBandsLongMainLow[pC->sampRateIdx];

  tC->tnsActive = active;

  /* now calc band and line borders */
  tC->tnsStopBand = min(pC->sfbCnt, tC->tnsMaxSfb);
  tC->tnsStopLine = pC->sfbOffset[tC->tnsStopBand];

  tC->tnsStartBand = Tns_FreqToBandWithRound(tC->tnsStartFreq, sampleRate,
                                            pC->sfbCnt, (const int16_t*)pC->sfbOffset);

  tC->tnsModifyBeginCb = Tns_FreqToBandWithRound(TNS_MODIFY_BEGIN,
                                                sampleRate,
                                                pC->sfbCnt,
                                                (const int16_t*)pC->sfbOffset);

  tC->tnsRatioPatchLowestCb = Tns_FreqToBandWithRound(RATIO_PATCH_LOWER_BORDER,
                                                     sampleRate,
                                                     pC->sfbCnt,
                                                     (const int16_t*)pC->sfbOffset);


  tC->tnsStartLine = pC->sfbOffset[tC->tnsStartBand];

  tC->lpcStopBand = tnsMaxBandsLongMainLow[pC->sampRateIdx];
  tC->lpcStopBand = min(tC->lpcStopBand, pC->sfbActive);

  tC->lpcStopLine = pC->sfbOffset[tC->lpcStopBand];

  tC->lpcStartBand = tnsMinBandNumberLong[pC->sampRateIdx];

  tC->lpcStartLine = pC->sfbOffset[tC->lpcStartBand];

  tC->threshold = TNS_GAIN_THRESH;


  return(0);
}

/**
*
* function name: Init_TnsCfgShortBlk
* description:  Fill GM_TNS_CFG_INFO structure with sensible content for short blocks
* returns:		0 if success
*
*/
int16_t Init_TnsCfgShortBlk(int32_t bitRate,              /*!< bitrate */
                                 int32_t sampleRate,           /*!< Sampling frequency */
                                 int16_t channels,             /*!< number of channels */
                                 GM_TNS_CFG_INFO *tC,              /*!< TNS Config struct (modified) */
                                 GM_PSY_CFG_SHORTBLK *pC, /*!< psy config struct */
                                 int16_t active)               /*!< tns active flag */
{
  int32_t bitratePerChannel;
  tC->maxOrder     = TNS_MAX_ORDER_SHORT;
  tC->tnsStartFreq = 2750;
  tC->coefRes      = 3;

  /* to avoid integer division */
  if ( sub(channels,2) == 0 ) {
    bitratePerChannel = L_shr(bitRate,1);
  }
  else {
    bitratePerChannel = bitRate;
  }

  tC->tnsMaxSfb = tnsMaxBandsShortMainLow[pC->sampRateIdx];

  tC->tnsActive = active;

  /* now calc band and line borders */
  tC->tnsStopBand = min(pC->sfbCnt, tC->tnsMaxSfb);
  tC->tnsStopLine = pC->sfbOffset[tC->tnsStopBand];

  tC->tnsStartBand=Tns_FreqToBandWithRound(tC->tnsStartFreq, sampleRate,
                                          pC->sfbCnt, (const int16_t*)pC->sfbOffset);

  tC->tnsModifyBeginCb = Tns_FreqToBandWithRound(TNS_MODIFY_BEGIN,
                                                sampleRate,
                                                pC->sfbCnt,
                                                (const int16_t*)pC->sfbOffset);

  tC->tnsRatioPatchLowestCb = Tns_FreqToBandWithRound(RATIO_PATCH_LOWER_BORDER,
                                                     sampleRate,
                                                     pC->sfbCnt,
                                                     (const int16_t*)pC->sfbOffset);


  tC->tnsStartLine = pC->sfbOffset[tC->tnsStartBand];

  tC->lpcStopBand = tnsMaxBandsShortMainLow[pC->sampRateIdx];

  tC->lpcStopBand = min(tC->lpcStopBand, pC->sfbActive);

  tC->lpcStopLine = pC->sfbOffset[tC->lpcStopBand];

  tC->lpcStartBand = tnsMinBandNumberShort[pC->sampRateIdx];

  tC->lpcStartLine = pC->sfbOffset[tC->lpcStartBand];

  tC->threshold = TNS_GAIN_THRESH;

  return(0);
}

/**
*
* function name: Tns_CalcFiltDecide
* description:  Calculate TNS filter and decide on TNS usage
* returns:		0 if success
*
*/
int32_t Tns_CalcFiltDecide(GM_TNS_PART_DATA* tnsData,        /*!< tns data structure (modified) */
                 GM_TNS_CFG_INFO tC,            /*!< tns config structure */
                 int32_t* pScratchTns,      /*!< pointer to scratch space */
                 const int16_t sfbOffset[], /*!< scalefactor size and table */
                 int32_t* spectrum,         /*!< spectral data */
                 int16_t subBlockNumber,    /*!< subblock num */
                 int16_t blockType,         /*!< blocktype (long or short) */
                 int32_t * sfbEnergy)       /*!< sfb-wise energy */
{

  int32_t  predictionGain;
  int32_t  temp;
  int32_t* pWork32 = &pScratchTns[subBlockNumber >> 8];
  int16_t* pWeightedSpectrum = (int16_t *)&pScratchTns[subBlockNumber >> 8];


  if (tC.tnsActive) {
    Tns_CalcLPCWeightedSpectrum(spectrum,
                         pWeightedSpectrum,
                         sfbEnergy,
                         sfbOffset,
                         tC.lpcStartLine,
                         tC.lpcStopLine,
                         tC.lpcStartBand,
                         tC.lpcStopBand,
                         pWork32);

    temp = blockType - SHORT_WINDOW;
    if ( temp != 0 ) {
        predictionGain = Tns_CalcLPCFilter( &pWeightedSpectrum[tC.lpcStartLine],
                                        tC.acfWindow,
                                        tC.lpcStopLine - tC.lpcStartLine,
                                        tC.maxOrder,
                                        tnsData->dataRaw.tnsLong.subBlockInfo.parcor);


        temp = predictionGain - tC.threshold;
        if ( temp > 0 ) {
          tnsData->dataRaw.tnsLong.subBlockInfo.tnsActive = 1;
        }
        else {
          tnsData->dataRaw.tnsLong.subBlockInfo.tnsActive = 0;
        }

        tnsData->dataRaw.tnsLong.subBlockInfo.predictionGain = predictionGain;
    }
    else{

        predictionGain = Tns_CalcLPCFilter( &pWeightedSpectrum[tC.lpcStartLine],
                                        tC.acfWindow,
                                        tC.lpcStopLine - tC.lpcStartLine,
                                        tC.maxOrder,
                                        tnsData->dataRaw.tnsShort.subBlockInfo[subBlockNumber].parcor);

        temp = predictionGain - tC.threshold;
        if ( temp > 0 ) {
          tnsData->dataRaw.tnsShort.subBlockInfo[subBlockNumber].tnsActive = 1;
        }
        else {
          tnsData->dataRaw.tnsShort.subBlockInfo[subBlockNumber].tnsActive = 0;
        }

        tnsData->dataRaw.tnsShort.subBlockInfo[subBlockNumber].predictionGain = predictionGain;
    }

  }
  else{

    temp = blockType - SHORT_WINDOW;
    if ( temp != 0 ) {
        tnsData->dataRaw.tnsLong.subBlockInfo.tnsActive = 0;
        tnsData->dataRaw.tnsLong.subBlockInfo.predictionGain = 0;
    }
    else {
        tnsData->dataRaw.tnsShort.subBlockInfo[subBlockNumber].tnsActive = 0;
        tnsData->dataRaw.tnsShort.subBlockInfo[subBlockNumber].predictionGain = 0;
    }
  }

  return(0);
}


/*****************************************************************************
*
* function name: Tns_SyncParam
* description: update tns parameter
*
*****************************************************************************/
void Tns_SyncParam(GM_TNS_PART_DATA *tnsDataDest,
             const GM_TNS_PART_DATA *tnsDataSrc,
             const GM_TNS_CFG_INFO tC,
             const int16_t subBlockNumber,
             const int16_t blockType)
{
   GM_TNS_SUBBLK_INFO *sbInfoDest;
   const GM_TNS_SUBBLK_INFO *sbInfoSrc;
   int32_t i, temp;

   temp =  blockType - SHORT_WINDOW;
   if ( temp != 0 ) {
      sbInfoDest = &tnsDataDest->dataRaw.tnsLong.subBlockInfo;
      sbInfoSrc  = &tnsDataSrc->dataRaw.tnsLong.subBlockInfo;
   }
   else {
      sbInfoDest = &tnsDataDest->dataRaw.tnsShort.subBlockInfo[subBlockNumber];
      sbInfoSrc  = &tnsDataSrc->dataRaw.tnsShort.subBlockInfo[subBlockNumber];
   }

   if (100*abs_s(sbInfoDest->predictionGain - sbInfoSrc->predictionGain) <
       (3 * sbInfoDest->predictionGain)) {
      sbInfoDest->tnsActive = sbInfoSrc->tnsActive;
      for ( i=0; i< tC.maxOrder; i++) {
        sbInfoDest->parcor[i] = sbInfoSrc->parcor[i];
      }
   }
}

/*****************************************************************************
*
* function name: Tns_Filtering
* description: do TNS filtering
* returns:     0 if success
*
*****************************************************************************/
int16_t Tns_Filtering(GM_TNS_BLK_INFO* tnsInfo,     /*!< tns info structure (modified) */
                 GM_TNS_PART_DATA* tnsData,     /*!< tns data structure (modified) */
                 int16_t numOfSfb,       /*!< number of scale factor bands */
                 GM_TNS_CFG_INFO tC,         /*!< tns config structure */
                 int16_t lowPassLine,    /*!< lowpass line */
                 int32_t* spectrum,      /*!< spectral data (modified) */
                 int16_t subBlockNumber, /*!< subblock num */
                 int16_t blockType)      /*!< blocktype (long or short) */
{
  int32_t i;
  int32_t temp_s;
  int32_t temp;
  GM_TNS_SUBBLK_INFO *psubBlockInfo;

  temp_s = blockType - SHORT_WINDOW;
  if ( temp_s != 0) {
    psubBlockInfo = &tnsData->dataRaw.tnsLong.subBlockInfo;
	if (psubBlockInfo->tnsActive == 0) {
      tnsInfo->tnsActive[subBlockNumber] = 0;
      return(0);
    }
    else {

      Tns_ParcorCoef2Index(psubBlockInfo->parcor,
                   tnsInfo->coef,
                   tC.maxOrder,
                   tC.coefRes);

      Tns_Index2ParcorCoef(tnsInfo->coef,
                   psubBlockInfo->parcor,
                   tC.maxOrder,
                   tC.coefRes);

      for (i=tC.maxOrder - 1; i>=0; i--)  {
        temp = psubBlockInfo->parcor[i] - TNS_PARCOR_THRESH;
        if ( temp > 0 )
          break;
        temp = psubBlockInfo->parcor[i] + TNS_PARCOR_THRESH;
        if ( temp < 0 )
          break;
      }
      tnsInfo->order[subBlockNumber] = i + 1;


      tnsInfo->tnsActive[subBlockNumber] = 1;
      for (i=subBlockNumber+1; i<TRANS_FAC; i++) {
        tnsInfo->tnsActive[i] = 0;
      }
      tnsInfo->coefRes[subBlockNumber] = tC.coefRes;
      tnsInfo->length[subBlockNumber] = numOfSfb - tC.tnsStartBand;


      Tns_AnalysisFilter(&(spectrum[tC.tnsStartLine]),
                            (min(tC.tnsStopLine,lowPassLine) - tC.tnsStartLine),
                            psubBlockInfo->parcor,
                            tnsInfo->order[subBlockNumber],
                            &(spectrum[tC.tnsStartLine]));

    }
  }     /* if (blockType!=SHORT_WINDOW) */
  else /*short block*/ {
    psubBlockInfo = &tnsData->dataRaw.tnsShort.subBlockInfo[subBlockNumber];
	if (psubBlockInfo->tnsActive == 0) {
      tnsInfo->tnsActive[subBlockNumber] = 0;
      return(0);
    }
    else {

      Tns_ParcorCoef2Index(psubBlockInfo->parcor,
                   &tnsInfo->coef[subBlockNumber*TNS_MAX_ORDER_SHORT],
                   tC.maxOrder,
                   tC.coefRes);

      Tns_Index2ParcorCoef(&tnsInfo->coef[subBlockNumber*TNS_MAX_ORDER_SHORT],
                   psubBlockInfo->parcor,
                   tC.maxOrder,
                   tC.coefRes);
      for (i=(tC.maxOrder - 1); i>=0; i--)  {
        temp = psubBlockInfo->parcor[i] - TNS_PARCOR_THRESH;
         if ( temp > 0 )
          break;

        temp = psubBlockInfo->parcor[i] + TNS_PARCOR_THRESH;
        if ( temp < 0 )
          break;
      }
      tnsInfo->order[subBlockNumber] = i + 1;

      tnsInfo->tnsActive[subBlockNumber] = 1;
      tnsInfo->coefRes[subBlockNumber] = tC.coefRes;
      tnsInfo->length[subBlockNumber] = numOfSfb - tC.tnsStartBand;


      Tns_AnalysisFilter(&(spectrum[tC.tnsStartLine]), (tC.tnsStopLine - tC.tnsStartLine),
                 psubBlockInfo->parcor,
                 tnsInfo->order[subBlockNumber],
                 &(spectrum[tC.tnsStartLine]));

    }
  }

  return(0);
}


/*****************************************************************************
*
* function name: m_pow2_cordic
* description: Iterative power function
*
*	Calculates pow(2.0,x-1.0*(scale+1)) with INT_BITS bit precision
*	using modified cordic algorithm
* returns:     the result of pow2
*
*****************************************************************************/
#if 0
static int32_t m_pow2_cordic(int32_t x, int16_t scale)
{
  int32_t k;

  int32_t accu_y = 0x40000000;
  accu_y = L_shr(accu_y,scale);

  for(k=1; k<INT_BITS; k++) {
    const int32_t z = m_log2_table[k];

    while(L_sub(x,z) >= 0) {

      x = L_sub(x, z);
      accu_y = L_add(accu_y, (accu_y >> k));
    }
  }
  return(accu_y);
}
#endif


/*****************************************************************************
*
* function name: Tns_CalcLPCWeightedSpectrum
* description: Calculate weighted spectrum for LPC calculation
*
*****************************************************************************/
static void Tns_CalcLPCWeightedSpectrum(const int32_t  spectrum[],         /*!< input spectrum */
                                 int16_t        weightedSpectrum[],
                                 int32_t       *sfbEnergy,          /*!< sfb energies */
                                 const int16_t *sfbOffset,
                                 int16_t        lpcStartLine,
                                 int16_t        lpcStopLine,
                                 int16_t        lpcStartBand,
                                 int16_t        lpcStopBand,
                                 int32_t       *pWork32)
{
    #define INT_BITS_SCAL 1<<(INT_BITS/2)

    int32_t i, sfb, shift;
    int32_t maxShift;
    int32_t tmp_s, tmp2_s;
    int32_t tmp, tmp2;
    int32_t maxWS;
    int32_t tnsSfbMean[MAX_SFB];    /* length [lpcStopBand-lpcStartBand] should be sufficient here */
    //int32_t tmp1;

    maxWS = 0;

    /* calc 1.0*2^-INT_BITS/2/sqrt(en) */
    for( sfb = lpcStartBand; sfb < lpcStopBand; sfb++) {

      tmp2 = sfbEnergy[sfb] - 2;
      if( tmp2 > 0) {
        tmp = rsqrt(sfbEnergy[sfb], INT_BITS);
		if(tmp > INT_BITS_SCAL)
		{
			shift =  norm_l(tmp);
			tmp = Div_32( INT_BITS_SCAL << shift, tmp << shift );
		}
		else
		{
			tmp = 0x7fffffff;
		}
      }
      else {
        tmp = 0x7fffffff;
      }
      tnsSfbMean[sfb] = tmp;
    }

    /* spread normalized values from sfbs to lines */
    sfb = lpcStartBand;
    tmp = tnsSfbMean[sfb];
    for ( i=lpcStartLine; i<lpcStopLine; i++){
      tmp_s = sfbOffset[sfb + 1] - i;
      if ( tmp_s == 0 ) {
        sfb = sfb + 1;
        tmp2_s = sfb + 1 - lpcStopBand;
        if (tmp2_s <= 0) {
          tmp = tnsSfbMean[sfb];
        }
      }
      pWork32[i] = tmp;
    }
    /*filter down*/
    for (i=(lpcStopLine - 2); i>=lpcStartLine; i--){
        pWork32[i] = (pWork32[i] + pWork32[i + 1]) >> 1;
    }
    /* filter up */
    for (i=(lpcStartLine + 1); i<lpcStopLine; i++){
       pWork32[i] = (pWork32[i] + pWork32[i - 1]) >> 1;
    }

    /* weight and normalize */
    for (i=lpcStartLine; i<lpcStopLine; i++){
#ifdef LINUX
    	int tmp1 = 0, aa, bb;
			aa = pWork32[i];
			bb = spectrum[i];
			asm volatile("smulls %[o1], %[o2], %[i1], %[i2]" : [o1] "=r" (tmp1), [o2] "=r" (tmp) : [i1] "r" (aa), [i2] "r" (bb));
      pWork32[i] = tmp;
			asm volatile("rsblt %[o1], %[o1], #0" : [o1] "+r" (tmp));
      maxWS |= (tmp);
#else
      __asm {
      	SMULLS	tmp1, tmp, pWork32[i], spectrum[i]
      }
      pWork32[i] = tmp;
      __asm {
      	RSBLT  tmp, tmp, 0
      }
      maxWS |= (tmp);
#endif
    }
    maxShift = norm_l(maxWS);

	maxShift = 16 - maxShift;
    if(maxShift >= 0)
	{
		for (i=lpcStartLine; i<lpcStopLine; i++){
			weightedSpectrum[i] = pWork32[i] >> maxShift;
		}
    }
	else
	{
		maxShift = -maxShift;
		for (i=lpcStartLine; i<lpcStopLine; i++){
			weightedSpectrum[i] = saturate(pWork32[i] << maxShift);
		}
	}
}




/*****************************************************************************
*
* function name: Tns_CalcLPCFilter
* description:  LPC calculation for one TNS filter
* returns:      prediction gain
* input:        signal spectrum, acf window, no. of spectral lines,
*                max. TNS order, ptr. to reflection ocefficients
* output:       reflection coefficients
*(half) window size must be larger than tnsOrder !!*
******************************************************************************/

static int16_t Tns_CalcLPCFilter(const int16_t *signal,
                            const int32_t window[],
                            int16_t numOfLines,
                            int16_t tnsOrder,
                            int32_t parcor[])
{
  int32_t parcorWorkBuffer[2*TNS_MAX_ORDER+1];
  int32_t predictionGain;
  int32_t i;
  int32_t tnsOrderPlus1 = tnsOrder + 1;

  assert(tnsOrder <= TNS_MAX_ORDER);      /* remove asserts later? (btg) */

  for(i=0;i<tnsOrder;i++) {
    parcor[i] = 0;
  }

  Tns_AutoCorre(signal, parcorWorkBuffer, numOfLines, tnsOrderPlus1);

  /* early return if signal is very low: signal prediction off, with zero parcor coeffs */
  if (parcorWorkBuffer[0] == 0)
    return 0;

  predictionGain = Tns_AutocorrelationToParcorCoef(parcorWorkBuffer, parcor, tnsOrder);

  return(predictionGain);
}

/*****************************************************************************
*
* function name: Tns_AutoCorre
* description:  calc. Tns_AutoCorre (acf)
* returns:      -
* input:        input values, no. of input values, no. of acf values
* output:       acf values
*
*****************************************************************************/
#ifndef ARMV5E
void Tns_AutoCorre(const int16_t		 input[],
                            int32_t       corr[],
                            int16_t       samples,
                            int16_t       corrCoeff) {
  int32_t i, j, isamples;
  int32_t accu;
  int32_t scf, tmp, tmp1;
  int16_t *in_p, *in_p1;

  scf = 10 - 1;

  isamples = samples;
  /* calc first corrCoef:  R[0] = sum { t[i] * t[i] } ; i = 0..N-1 */
  //accu = L_add(accu, ((input[j] * input[j]) >> scf));
  accu = 0;
  in_p = (int16_t *)input;
  j = isamples>>1; //jk
  tmp = 0x80000000;
  do {
#ifdef LINUX
  	int a, b;
		a = *in_p;
		b = *in_p++;
		asm volatile("mul %[o1], %[i1], %[i2]" : [o1] "=r" (tmp1) : [i1] "r" (a), [i2] "r" (b));
		asm volatile("mov %[o1], %[o1], asr %[i1]" : [o1] "+r" (tmp1) : [i1] "r" (scf));
		asm volatile("qadd %[o1], %[o1], %[i1]" : [o1] "+r" (accu) : [i1] "r" (tmp));
#else
   	__asm{
   		MUL		tmp1, *in_p, *in_p++
		//ADDS	accu, accu, tmp1, asr scf
		//EORVS	accu, tmp, accu, ASR#31
        QADD   accu, accu, tmp1
		MUL		tmp1, *in_p, *in_p++        //jk
		//ADDS	accu, accu, tmp1, asr scf	//jk
		//EORVS	accu, tmp, accu, ASR#31     //jk
		QADD   accu, accu, tmp1
	}
#endif
  } while (--j!=0);
  corr[0] = accu;

  /* early termination if all corr coeffs are likely going to be zero */
  if(corr[0] == 0) return ;

  /* calc all other corrCoef:  R[j] = sum { t[i] * t[i+j] } ; i = 0..(N-j-1), j=1..p */
  for(i=1; i<corrCoeff; i++) {
    isamples = isamples - 1;
    accu = 0;
    in_p = (int16_t *)input;
    in_p1 = (int16_t *)&input[i];
//    for(j=0; j<isamples; j++) {
	j = isamples>>1;//jk
	do {
#ifdef LINUX
			int a, b;
			a = *in_p++;
			b = *in_p1++;
			asm volatile("mul %[o1], %[i1], %[i2]" : [o1] "+r" (tmp1) : [i1] "r" (a), [i2] "r" (b));
			asm volatile("mov %[o1], %[o1], asr %[i1]" : [o1] "+r" (tmp1) : [i1] "r" (scf));
			asm volatile("qadd %[o1], %[o1], %[i1]" : [o1] "+r" (accu) : [i1] "r" (tmp));
#else
	   	__asm{
	   		MUL		tmp1, *in_p++, *in_p1++
			//ADDS	accu, accu, tmp1, asr scf
			//EORVS	accu, tmp, accu, ASR#31
			QADD    accu, accu, tmp1
//			ADDS	accu, accu, ((*in_p++ * *in_p1++) >> scf)
//			EORVS	accu, tmp, accu, ASR#31
            MUL		tmp1, *in_p++, *in_p1++  //jk
			//ADDS	accu, accu, tmp1, asr scf //jk
			//EORVS	accu, tmp, accu, ASR#31 //jk
			QADD  accu, accu, tmp1
		}
#endif
    } while (--j!=0);
    corr[i] = accu;
  }

}
#endif

/*****************************************************************************
*
* function name: Tns_AutocorrelationToParcorCoef
* description:  conversion autocorrelation to reflection coefficients
* returns:      prediction gain
* input:        <order+1> input values, no. of output values (=order),
*               ptr. to workbuffer (required size: 2*order)
* output:       <order> reflection coefficients
*
*****************************************************************************/
static int16_t Tns_AutocorrelationToParcorCoef(int32_t workBuffer[], int32_t reflCoeff[], int16_t numOfCoeff) {

  int32_t i, j, shift;
  int32_t *pWorkBuffer; /* temp pointer */
  int32_t predictionGain = 0;
  int32_t num, denom = 0;
  int32_t temp, workBuffer0;
  int32_t tmp;


  num = workBuffer[0];
  temp = workBuffer[numOfCoeff];

  for(i=0; i<numOfCoeff-1; i++) {
    workBuffer[i + numOfCoeff] = workBuffer[i + 1];
  }
  workBuffer[i + numOfCoeff] = temp;
  tmp = 0x80000000;
  for(i=0; i<numOfCoeff; i++) {
    int32_t refc;


    if (workBuffer[0] < L_abs(workBuffer[i + numOfCoeff])) {
      return 0 ;
    }
	shift = norm_l(workBuffer[0]);
	workBuffer0 = Div_32(1 << shift, workBuffer[0] << shift);
    /* calculate refc = -workBuffer[numOfCoeff+i] / workBuffer[0]; -1 <= refc < 1 */
	refc = L_negate(fixmul(workBuffer[numOfCoeff + i], workBuffer0));

    reflCoeff[i] = refc;

    pWorkBuffer = &(workBuffer[numOfCoeff]);

    for(j=i; j<numOfCoeff; j++) {
#ifdef LINUX
      int32_t accu1 = 0, accu2 = 0, aa, bb;
			aa = pWorkBuffer[j];
			bb= fixmul(refc, workBuffer[j - i]);
			asm volatile("qadd %[o1], %[i1], %[i2]" : [o1] "=r" (accu1) : [i1] "r" (aa), [i2] "r" (bb));
			aa = workBuffer[j-i];
			bb= fixmul(refc, pWorkBuffer[j]);
			asm volatile("qadd %[o1], %[i1], %[i2]" : [o1] "=r" (accu2) : [i1] "r" (aa), [i2] "r" (bb));
      pWorkBuffer[j] = accu1;
      workBuffer[j - i] = accu2;
#else
      int32_t accu1, accu2;
//      accu1 = L_add(pWorkBuffer[j], fixmul(refc, workBuffer[j - i]));
//      accu2 = L_add(workBuffer[j - i], fixmul(refc, pWorkBuffer[j]));
      __asm {
		//ADDS	accu1, pWorkBuffer[j], fixmul(refc, workBuffer[j - i])
		//EORVS	accu1, tmp, accu1, ASR#31
        QADD   accu1, pWorkBuffer[j], fixmul(refc, workBuffer[j - i])
		//ADDS	accu2, workBuffer[j - i], fixmul(refc, pWorkBuffer[j])
		//EORVS	accu2, tmp, accu2, ASR#31
		QADD   accu2, workBuffer[j - i], fixmul(refc, pWorkBuffer[j])
	  }
      pWorkBuffer[j] = accu1;
      workBuffer[j - i] = accu2;
#endif
    }
  }
#ifdef LINUX
	{
		int aa, bb;
		aa = workBuffer[0];
		bb = NORM_COEF;
		asm volatile("smull %[o1], %[o2], %[i1], %[i2]" : [o1] "=r" (tmp), [o2] "=r" (denom) : [i1] "r" (aa), [i2] "r" (bb));
	}
#else
  __asm {
  	SMULL	tmp, denom, workBuffer[0], NORM_COEF
  }
#endif
//  denom = MULHIGH(workBuffer[0], NORM_COEF);

  if (denom != 0) {
    int32_t temp;
	shift = norm_l(denom);
	temp = Div_32(1 << shift, denom << shift);
    predictionGain = fixmul(num, temp);
  }

  return extract_l(predictionGain);
}



static int16_t Tns_Search31(int32_t parcor)
{
  int32_t index = 0;
  int32_t i;
  int32_t temp;

  for (i=0;i<8;i++) {
    temp = L_sub( parcor, tnsCoeff3Borders[i]);
    if (temp > 0)
      index=i;
  }
  return extract_l(index - 4);
}

static int16_t Tns_Search32(int32_t parcor)
{
  int32_t index = 0;
  int32_t i;
  int32_t temp;


  for (i=0;i<16;i++) {
    temp = L_sub(parcor, tnsCoeff4Borders[i]);
    if (temp > 0)
      index=i;
  }
  return extract_l(index - 8);
}



/*****************************************************************************
*
* functionname: Tns_ParcorCoef2Index
* description:  quantization index for reflection coefficients
*
*****************************************************************************/
static void Tns_ParcorCoef2Index(const int32_t parcor[],   /*!< parcor coefficients */
                         int16_t index[],          /*!< quantized coeff indices */
                         int16_t order,            /*!< filter order */
                         int16_t bitsPerCoeff) {   /*!< quantizer resolution */
  int32_t i;
  int32_t temp;

  for(i=0; i<order; i++) {
    temp = bitsPerCoeff - 3;
    if (temp == 0) {
      index[i] = Tns_Search31(parcor[i]);
    }
    else {
      index[i] = Tns_Search32(parcor[i]);
    }
  }
}

/*****************************************************************************
*
* functionname: Tns_Index2ParcorCoef
* description:  Inverse quantization for reflection coefficients
*
*****************************************************************************/
static void Tns_Index2ParcorCoef(const int16_t index[],  /*!< quantized values */
                         int32_t parcor[],       /*!< ptr. to reflection coefficients (output) */
                         int16_t order,          /*!< no. of coefficients */
                         int16_t bitsPerCoeff)   /*!< quantizer resolution */
{
  int32_t i;
  int32_t temp;

  for (i=0; i<order; i++) {
    temp = bitsPerCoeff - 4;
    if ( temp == 0 ) {
        parcor[i] = tnsCoeff4[index[i] + 8];
    }
    else {
        parcor[i] = tnsCoeff3[index[i] + 4];
    }
  }
}

/*****************************************************************************
*
* functionname: Tns_FIR
* description:  in place lattice filtering of spectral data
* returns:		pointer to modified data
*
*****************************************************************************/
static int32_t Tns_FIR(int16_t order,           /*!< filter order */
                         int32_t x,               /*!< spectral data */
                         int32_t *state_par,      /*!< filter states */
                         const int32_t *coef_par) /*!< filter coefficients */
{
   int32_t i;
   int32_t accu,tmp,tmpSave;

   x = x >> 1;
   tmpSave = x;

   for (i=0; i<(order - 1); i++) {

     tmp = L_add(fixmul(coef_par[i], x), state_par[i]);
     x   = L_add(fixmul(coef_par[i], state_par[i]), x);

     state_par[i] = tmpSave;
     tmpSave = tmp;
  }

  /* last stage: only need half operations */
  accu = fixmul(state_par[order - 1], coef_par[(order - 1)]);
  state_par[(order - 1)] = tmpSave;

  x = L_add(accu, x);
  x = L_add(x, x);

  return x;
}

/*****************************************************************************
*
* functionname: Tns_AnalysisFilter
* description:  filters spectral lines with TNS filter
*
*****************************************************************************/
static void Tns_AnalysisFilter(const  int32_t signal[],  /*!< input spectrum */
                                  int16_t numOfLines,       /*!< no. of lines */
                                  const  int32_t parCoeff[],/*!< PARC coefficients */
                                  int16_t order,            /*!< filter order */
                                  int32_t output[])         /*!< filtered signal values */
{

  int32_t state_par[TNS_MAX_ORDER];
  int32_t j;

  for ( j=0; j<TNS_MAX_ORDER; j++ ) {
    state_par[j] = 0;
  }

  for(j=0; j<numOfLines; j++) {
    output[j] = Tns_FIR(order,signal[j],state_par,parCoeff);
  }
}

/*****************************************************************************
*
* functionname: Tns_ApplyMultTableToRatios
* description:  Change thresholds according to tns
*
*****************************************************************************/
void Tns_ApplyMultTableToRatios(int16_t startCb,
                               int16_t stopCb,
                               GM_TNS_SUBBLK_INFO subInfo, /*!< TNS subblock info */
                               int32_t *thresholds)        /*!< thresholds (modified) */
{
  int32_t i;
  if (subInfo.tnsActive) {
    for(i=startCb; i<stopCb; i++) {
      /* thresholds[i] * 0.25 */
      thresholds[i] = (thresholds[i] >> 2);
    }
  }
}
