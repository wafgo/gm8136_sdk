
/*
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
***********************************************************************/


/*******************************************************************************
	File:		Perceptual_cfg.c

	Content:	Psychoaccoustic configuration functions

*******************************************************************************/

#include "basic_op.h"
#include "oper_32b.h"
#include "perceptual_cfg.h"
#include "threshold_tool.h"
#include "aac_table.h"



#define BARC_SCALE	100 /* integer barc values are scaled with 100 */
#define LOG2_1000	301 /* log2*1000 */
#define PI2_1000	1571 /* pi/2*1000*/
#define ATAN_COEF1	3560 /* 1000/0.280872f*/
#define ATAN_COEF2	281 /* 1000*0.280872f*/

typedef struct{
  int32_t sampleRate;
  const uint8_t *paramLong;
  const uint8_t *paramShort;
}SFB_INFO_TAB;

static const int16_t ABS_LEV = 20;
static const int16_t BARC_THR_QUIET[] = {15, 10,  7,  2,  0,  0,  0,  0,  0,  0,
                                         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                                         3,  5, 10, 20, 30};



static const int16_t max_bark = 24; /* maximum bark-value */
static const int16_t maskLow  = 30; /* in 1dB/bark */
static const int16_t maskHigh = 15; /* in 1*dB/bark */
static const int16_t c_ratio  = 0x0029; /* pow(10.0f, -(29.0f/10.0f)) */

static const int16_t maskLowSprEnLong = 30;       /* in 1dB/bark */
static const int16_t maskHighSprEnLong = 20;      /* in 1dB/bark */
static const int16_t maskHighSprEnLongLowBr = 15; /* in 1dB/bark */
static const int16_t maskLowSprEnShort = 20;      /* in 1dB/bark */
static const int16_t maskHighSprEnShort = 15;     /* in 1dB/bark */
static const int16_t c_minRemainingThresholdFactor = 0x0148;    /* 0.01 *(1 << 15)*/
static const int32_t c_maxsnr = 0x66666666;		 /* upper limit is -1 dB */
static const int32_t c_minsnr = 0x00624dd3;		 /* lower limit is -25 dB */

static const int32_t c_maxClipEnergyLong = 0x77359400;  /* 2.0e9f*/
static const int32_t c_maxClipEnergyShort = 0x01dcd650; /* 2.0e9f/(AACENC_TRANS_FAC*AACENC_TRANS_FAC)*/


int32_t Get_SampleRateIndex(int32_t sampleRate)
{
    if (92017 <= sampleRate)
		return 0;
    if (75132 <= sampleRate)
		return 1;
    if (55426 <= sampleRate)
		return 2;
    if (46009 <= sampleRate)
		return 3;
    if (37566 <= sampleRate)
		return 4;
    if (27713 <= sampleRate)
		return 5;
    if (23004 <= sampleRate)
		return 6;
    if (18783 <= sampleRate)
		return 7;
    if (13856 <= sampleRate)
		return 8;
    if (11502 <= sampleRate)
		return 9;
    if (9391 <= sampleRate)
		return 10;

    return 11;
}


/*********************************************************************************
*
* function name: Cal_AtAppr
* description:  calculates 1000*atan(x/1000)
*               based on atan approx for x > 0
*				atan(x) = x/((float)1.0f+(float)0.280872f*x*x)  if x < 1
*						= pi/2 - x/((float)0.280872f +x*x)	    if x >= 1
* return:       1000*atan(x/1000)
*
**********************************************************************************/
static int16_t Cal_AtAppr(int32_t val)
{
  int32_t y;


  if(L_sub(val, 1000) < 0) {
    y = extract_l(((1000 * val) / (1000 + ((val * val) / ATAN_COEF1))));
  }
  else {
    y = PI2_1000 - ((1000 * val) / (ATAN_COEF2 + ((val * val) / 1000)));
  }

  return extract_l(y);
}


/*****************************************************************************
*
* function name: Calc_BarkOneLineVal
* description:  Calculates barc value for one frequency line
* returns:      barc value of line * BARC_SCALE
* input:        number of lines in transform, index of line to check, Fs
* output:
*
*****************************************************************************/
static int16_t Calc_BarkOneLineVal(int16_t noOfLines, int16_t fftLine, int32_t samplingFreq)
{
  int32_t center_freq, temp, bvalFFTLine;

  /* center frequency of fft line */
  center_freq = (fftLine * samplingFreq) / (noOfLines << 1);
  temp =  Cal_AtAppr((center_freq << 2) / (3*10));
  bvalFFTLine =
    (26600 * Cal_AtAppr((center_freq*76) / 100) + 7*temp*temp) / (2*1000*1000 / BARC_SCALE);

  return saturate(bvalFFTLine);
}

/*****************************************************************************
*
* function name: SetStart_ThrQuiet
* description:  init thredhold in quiet
*
*****************************************************************************/
static void SetStart_ThrQuiet(int16_t  numPb,
                         const int16_t *pbOffset,
                         int16_t *pbBarcVal,
                         int32_t *pbThresholdQuiet) {
  int16_t i;
  int16_t barcThrQuiet;

  for(i=0; i<numPb; i++) {
    int16_t bv1, bv2;


    if (i>0)
      bv1 = (pbBarcVal[i] + pbBarcVal[i-1]) >> 1;
    else
      bv1 = pbBarcVal[i] >> 1;


    if (i < (numPb - 1))
      bv2 = (pbBarcVal[i] + pbBarcVal[i+1]) >> 1;
    else {
      bv2 = pbBarcVal[i];
    }

    bv1 = min((int16_t)(bv1 / BARC_SCALE), max_bark);
    bv2 = min((int16_t)(bv2 / BARC_SCALE), max_bark);

    barcThrQuiet = min(BARC_THR_QUIET[bv1], BARC_THR_QUIET[bv2]);


    /*
      we calculate
      pow(10.0f,(float)(barcThrQuiet - ABS_LEV)*0.1)*(float)ABS_LOW*(pbOffset[i+1] - pbOffset[i]);
    */

    pbThresholdQuiet[i] = pow2_xy((((barcThrQuiet - ABS_LEV) * 100) +
                          LOG2_1000*(14+2*LOG_NORM_PCM)), LOG2_1000) * (pbOffset[i+1] - pbOffset[i]);
  }
}


/*****************************************************************************
*
* function name: Init_EnergySpreading
* description:  init energy spreading parameter
*
*****************************************************************************/
static void Init_EnergySpreading(int16_t  numPb,
                          int16_t *pbBarcValue,
                          int16_t *pbMaskLoFactor,
                          int16_t *pbMaskHiFactor,
                          int16_t *pbMaskLoFactorSprEn,
                          int16_t *pbMaskHiFactorSprEn,
                          const int32_t bitrate,
                          const int16_t blockType)
{
  int16_t i;
  int16_t maskLowSprEn, maskHighSprEn;

	//	check here

  if (sub(blockType, SHORT_WINDOW) != 0) {
    maskLowSprEn = maskLowSprEnLong;

    if (bitrate > 22000)
      maskHighSprEn = maskHighSprEnLong;
    else
      maskHighSprEn = maskHighSprEnLongLowBr;
  }
  else {
    maskLowSprEn = maskLowSprEnShort;
    maskHighSprEn = maskHighSprEnShort;
  }

  for(i=0; i<numPb; i++) {

    if (i > 0) {
      int32_t dbVal;
      int16_t dbark = pbBarcValue[i] - pbBarcValue[i-1];

      /*
        we calulate pow(10.0f, -0.1*dbVal/BARC_SCALE)
      */
      dbVal = (maskHigh * dbark);
      pbMaskHiFactor[i] = round16(pow2_xy(L_negate(dbVal), (int32_t)LOG2_1000));             /* 0.301 log10(2) */

      dbVal = (maskLow * dbark);
      pbMaskLoFactor[i-1] = round16(pow2_xy(L_negate(dbVal),(int32_t)LOG2_1000));


      dbVal = (maskHighSprEn * dbark);
      pbMaskHiFactorSprEn[i] =  round16(pow2_xy(L_negate(dbVal),(int32_t)LOG2_1000));
      dbVal = (maskLowSprEn * dbark);
      pbMaskLoFactorSprEn[i-1] = round16(pow2_xy(L_negate(dbVal),(int32_t)LOG2_1000));
    }
    else {
      pbMaskHiFactor[i] = 0;
      pbMaskLoFactor[numPb-1] = 0;

      pbMaskHiFactorSprEn[i] = 0;
      pbMaskLoFactorSprEn[numPb-1] = 0;
    }
  }

}


/*****************************************************************************
*
* function name: Start_BarkVal
* description:  init bark value
*
*****************************************************************************/
static void Start_BarkVal(int16_t  numPb,
                           const int16_t *pbOffset,
                           int16_t  numLines,
                           int32_t  samplingFrequency,
                           int16_t *pbBval)
{
  int16_t i;
  int16_t pbBval0, pbBval1;

  pbBval0 = 0;

  for(i=0; i<numPb; i++){
    pbBval1 = Calc_BarkOneLineVal(numLines, pbOffset[i+1], samplingFrequency);
    pbBval[i] = (pbBval0 + pbBval1) >> 1;
    pbBval0 = pbBval1;
  }
}


/*****************************************************************************
*
* function name: Calc_MinSnrParm
* description:  calculate min snr parameter
*				minSnr(n) = 1/(2^sfbPemin(n)/w(n) - 1.5)
*
*****************************************************************************/
static void Calc_MinSnrParm(const int32_t  bitrate,
                       const int32_t  samplerate,
                       const int16_t  numLines,
                       const int16_t *sfbOffset,
                       const int16_t *pbBarcVal,
                       const int16_t  sfbActive,
                       int16_t       *sfbMinSnr)
{
  int16_t sfb;
  int16_t barcWidth;
  int16_t pePerWindow;
  int32_t pePart;
  int32_t snr;
  int16_t pbVal0, pbVal1, shift;

  /* relative number of active barks */


  pePerWindow = Conv_bitToPe(extract_l((bitrate * numLines) / samplerate));

  pbVal0 = 0;

  for (sfb=0; sfb<sfbActive; sfb++) {

    pbVal1 = (pbBarcVal[sfb] << 1) - pbVal0;
    barcWidth = pbVal1 - pbVal0;
    pbVal0 = pbVal1;

    /* allow at least 2.4% of pe for each active barc */
	pePart = ((pePerWindow * 24) * (max_bark * barcWidth)) /
        (pbBarcVal[sfbActive-1] * (sfbOffset[sfb+1] - sfbOffset[sfb]));


    pePart = min(pePart, 8400);
    pePart = max(pePart, 1400);

    /* minSnr(n) = 1/(2^sfbPemin(n)/w(n) - 1.5)*/
	/* we add an offset of 2^16 to the pow functions */
	/* 0xc000 = 1.5*(1 << 15)*/

    snr = pow2_xy((pePart - 16*1000),1000) - 0x0000c000;

    if(snr > 0x00008000)
	{
		shift = norm_l(snr);
		snr = Div_32(0x00008000 << shift, snr << shift);
	}
	else
	{
		snr = 0x7fffffff;
	}

    /* upper limit is -1 dB */
    snr = min(snr, c_maxsnr);
    /* lower limit is -25 dB */
    snr = max(snr, c_minsnr);
    sfbMinSnr[sfb] = round16(snr);
  }

}

/*****************************************************************************
*
* function name: Perce_InitCfgLngBlk
* description:  init long block psychoacoustic configuration
*
*****************************************************************************/
int16_t Perce_InitCfgLngBlk(int32_t bitrate,
                                int32_t samplerate,
                                int16_t bandwidth,
                                GM_PSY_CFG_LONGBLK *psyConf)
{
  int32_t samplerateindex;
  int16_t sfbBarcVal[MAX_SFB_LONG];
  int16_t sfb;

  /*
    init sfb table
  */
  samplerateindex = Get_SampleRateIndex(samplerate);
  psyConf->sfbCnt = sfBandTotalLong[samplerateindex];
  psyConf->sfbOffset = sfBandTabLong + sfBandTabLongOffset[samplerateindex];
  psyConf->sampRateIdx = samplerateindex;

  /*
    calculate barc values for each pb
  */
  Start_BarkVal(psyConf->sfbCnt,
                 psyConf->sfbOffset,
                 psyConf->sfbOffset[psyConf->sfbCnt],
                 samplerate,
                 sfbBarcVal);

  /*
    init thresholds in quiet
  */
  SetStart_ThrQuiet(psyConf->sfbCnt,
               psyConf->sfbOffset,
               sfbBarcVal,
               psyConf->sfbThresholdQuiet);

  /*
    calculate spreading function
  */
  Init_EnergySpreading(psyConf->sfbCnt,
                sfbBarcVal,
                psyConf->sfbMaskLowFactor,
                psyConf->sfbMaskHighFactor,
                psyConf->sfbMaskLowFactorSprEn,
                psyConf->sfbMaskHighFactorSprEn,
                bitrate,
                LONG_WINDOW);

  /*
    init ratio
  */
  psyConf->ratio = c_ratio;

  psyConf->maxAllowedIncreaseFactor = 2;
  psyConf->minRemainingThresholdFactor = c_minRemainingThresholdFactor;    /* 0.01 *(1 << 15)*/

  psyConf->clipEnergy = c_maxClipEnergyLong;
  psyConf->lowpassLine = extract_l((bandwidth<<1) * FRAME_LEN_LONG / samplerate);

  for (sfb = 0; sfb < psyConf->sfbCnt; sfb++) {
    if (sub(psyConf->sfbOffset[sfb], psyConf->lowpassLine) >= 0)
      break;
  }
  psyConf->sfbActive = sfb;

  /*
    calculate minSnr
  */
  Calc_MinSnrParm(bitrate,
             samplerate,
             psyConf->sfbOffset[psyConf->sfbCnt],
             psyConf->sfbOffset,
             sfbBarcVal,
             psyConf->sfbActive,
             psyConf->sfbMinSnr);


  return(0);
}

/*****************************************************************************
*
* function name: Psy_InitCfgShortBlk
* description:  init short block psychoacoustic configuration
*
*****************************************************************************/
int16_t Psy_InitCfgShortBlk(int32_t bitrate,
                                 int32_t samplerate,
                                 int16_t bandwidth,
                                 GM_PSY_CFG_SHORTBLK *psyConf)
{
  int32_t samplerateindex;
  int16_t sfbBarcVal[MAX_SFB_SHORT];
  int16_t sfb;
  /*
    init sfb table
  */
  samplerateindex = Get_SampleRateIndex(samplerate);
  psyConf->sfbCnt = sfBandTotalShort[samplerateindex];
  psyConf->sfbOffset = sfBandTabShort + sfBandTabShortOffset[samplerateindex];
  psyConf->sampRateIdx = samplerateindex;
  /*
    calculate barc values for each pb
  */
  Start_BarkVal(psyConf->sfbCnt,
                 psyConf->sfbOffset,
                 psyConf->sfbOffset[psyConf->sfbCnt],
                 samplerate,
                 sfbBarcVal);

  /*
    init thresholds in quiet
  */
  SetStart_ThrQuiet(psyConf->sfbCnt,
               psyConf->sfbOffset,
               sfbBarcVal,
               psyConf->sfbThresholdQuiet);

  /*
    calculate spreading function
  */
  Init_EnergySpreading(psyConf->sfbCnt,
                sfbBarcVal,
                psyConf->sfbMaskLowFactor,
                psyConf->sfbMaskHighFactor,
                psyConf->sfbMaskLowFactorSprEn,
                psyConf->sfbMaskHighFactorSprEn,
                bitrate,
                SHORT_WINDOW);

  /*
    init ratio
  */
  psyConf->ratio = c_ratio;

  psyConf->maxAllowedIncreaseFactor = 2;
  psyConf->minRemainingThresholdFactor = c_minRemainingThresholdFactor;

  psyConf->clipEnergy = c_maxClipEnergyShort;

  psyConf->lowpassLine = extract_l(((bandwidth << 1) * FRAME_LEN_SHORT) / samplerate);

  for (sfb = 0; sfb < psyConf->sfbCnt; sfb++) {

    if (psyConf->sfbOffset[sfb] >= psyConf->lowpassLine)
      break;
  }
  psyConf->sfbActive = sfb;

  /*
    calculate minSNR
  */
  Calc_MinSnrParm(bitrate,
             samplerate,
             psyConf->sfbOffset[psyConf->sfbCnt],
             psyConf->sfbOffset,
             sfbBarcVal,
             psyConf->sfbActive,
             psyConf->sfbMinSnr);

  return(0);
}
