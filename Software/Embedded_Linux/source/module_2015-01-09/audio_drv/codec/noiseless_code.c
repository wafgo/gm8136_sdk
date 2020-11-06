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
	File:		noiseless_code.c

	Content:	Noiseless coder module functions

*******************************************************************************/

#include "aac_table.h"
#include "noiseless_code.h"
#include "huffman_1.h"
#include "perceptual_const.h"


/*****************************************************************************
*
* function name: Nl_BitCntHuffTable
* description:  count bits using all possible tables
*
*****************************************************************************/
static void
Nl_BitCntHuffTable(const int16_t *quantSpectrum,
               const int16_t maxSfb,
               const int16_t *sfbOffset,
               const uint16_t *sfbMax,
               int16_t bitLookUp[MAX_SFB_LONG][CODE_BOOK_ESC_NDX + 1],
               GM_SECTION_PARAM * sectionInfo)
{
  int32_t i;

  for (i=0; i<maxSfb; i++) {
    int16_t sfbWidth, maxVal;

    sectionInfo[i].sfbCnt = 1;
    sectionInfo[i].sfbStart = i;
    sectionInfo[i].sectionBits = INVALID_BITCOUNT;
    sectionInfo[i].codeBook = -1;
    sfbWidth = sfbOffset[i + 1] - sfbOffset[i];
    maxVal = sfbMax[i];
    Call_bitCntFunTab(quantSpectrum + sfbOffset[i], sfbWidth, maxVal, bitLookUp[i]);
  }
}


/*****************************************************************************
*
* function name: GoodCodBook
* description:  essential helper functions
*
*****************************************************************************/
static int16_t
GoodCodBook(const int16_t *bc, int16_t *book)
{
  int32_t minBits, j;
  minBits = INVALID_BITCOUNT;

  for (j=0; j<=CODE_BOOK_ESC_NDX; j++) {

    if (bc[j] < minBits) {
      minBits = bc[j];
      *book = j;
    }
  }
  return extract_l(minBits);
}

static int16_t
MinSecBigBits(const int16_t *bc1, const int16_t *bc2)
{
  int32_t minBits, j, sum;
  minBits = INVALID_BITCOUNT;

  for (j=0; j<=CODE_BOOK_ESC_NDX; j++) {
    sum = bc1[j] + bc2[j];
    if (sum < minBits) {
      minBits = sum;
    }
  }
  return extract_l(minBits);
}

static void
FindBitBigTable(int16_t *bc1, const int16_t *bc2)
{
  int32_t j;

  for (j=0; j<=CODE_BOOK_ESC_NDX; j++) {
    bc1[j] = min(bc1[j] + bc2[j], INVALID_BITCOUNT);
  }
}

/*
 * findMaxMerge : (Greedy Merge Algorithm)
*/
static int16_t
CalcSectBitMaxBig(const int16_t mergeGainLookUp[MAX_SFB_LONG],
             const GM_SECTION_PARAM *sectionInfo,
             const int16_t maxSfb, int16_t *maxNdx)
{
  int32_t i, maxMergeGain;
  maxMergeGain = 0;

  for (i=0; i+sectionInfo[i].sfbCnt < maxSfb; i += sectionInfo[i].sfbCnt) {

    if (mergeGainLookUp[i] > maxMergeGain) {
      maxMergeGain = mergeGainLookUp[i];
      *maxNdx = i;
    }
  }
  return extract_l(maxMergeGain);
}


/*
*
*/
static int16_t
FindBigGain(const GM_SECTION_PARAM *sectionInfo,
              int16_t bitLookUp[MAX_SFB_LONG][CODE_BOOK_ESC_NDX + 1],
              const int16_t *sideInfoTab,
              const int16_t ndx1,
              const int16_t ndx2)
{
  int32_t SplitBits;
  int32_t MergeBits;
  int32_t MergeGain;

  /*
    Bit amount for splitted sections
  */
  SplitBits = sectionInfo[ndx1].sectionBits + sectionInfo[ndx2].sectionBits;

  MergeBits = sideInfoTab[sectionInfo[ndx1].sfbCnt + sectionInfo[ndx2].sfbCnt] +
                  MinSecBigBits(bitLookUp[ndx1], bitLookUp[ndx2]);
  MergeGain = (SplitBits - MergeBits);

  return extract_l(MergeGain);
}

/*
  sectioning Stage 0:find minimum codbooks
*/

static void
Codbooks_1(GM_SECTION_PARAM * sectionInfo,
         int16_t bitLookUp[MAX_SFB_LONG][CODE_BOOK_ESC_NDX + 1],
         const int16_t maxSfb)
{
  int32_t i;

  for (i=0; i<maxSfb; i++) {
    /* Side-Info bits will be calculated in Stage 1!  */

    if (sectionInfo[i].sectionBits == INVALID_BITCOUNT) {
      sectionInfo[i].sectionBits = GoodCodBook(bitLookUp[i], &(sectionInfo[i].codeBook));
    }
  }
}

/*
  sectioning Stage 1:merge all connected regions with the same code book and
  calculate side info
*/

static void
Codbooks_2(GM_SECTION_PARAM * sectionInfo,
         int16_t bitLookUp[MAX_SFB_LONG][CODE_BOOK_ESC_NDX + 1],
         const int16_t maxSfb,
         const int16_t *sideInfoTab)
{
  GM_SECTION_PARAM * sectionInfo_s;
  GM_SECTION_PARAM * sectionInfo_e;
  int32_t mergeStart, mergeEnd;
  mergeStart = 0;

  do {

    sectionInfo_s = sectionInfo + mergeStart;
	for (mergeEnd=mergeStart+1; mergeEnd<maxSfb; mergeEnd++) {
      sectionInfo_e = sectionInfo + mergeEnd;
      if (sectionInfo_s->codeBook != sectionInfo_e->codeBook)
        break;
      sectionInfo_s->sfbCnt += 1;
      sectionInfo_s->sectionBits += sectionInfo_e->sectionBits;
     /* mergeBitLookUp*/
      FindBitBigTable(bitLookUp[mergeStart], bitLookUp[mergeEnd]);
    }

    sectionInfo_s->sectionBits += sideInfoTab[sectionInfo_s->sfbCnt];
    sectionInfo[mergeEnd - 1].sfbStart = sectionInfo_s->sfbStart;      /* speed up prev search */

    mergeStart = mergeEnd;


  } while (mergeStart - maxSfb < 0);
}

/*
  sectioning Stage 2:greedy merge algorithm, merge connected sections with
  maximum bit gain until no more gain is possible
*/
static void
Codbooks_3(GM_SECTION_PARAM *sectionInfo,
         int16_t mergeGainLookUp[MAX_SFB_LONG],
         int16_t bitLookUp[MAX_SFB_LONG][CODE_BOOK_ESC_NDX + 1],
         const int16_t maxSfb,
         const int16_t *sideInfoTab)
{
  int16_t i;

  for (i=0; i+sectionInfo[i].sfbCnt<maxSfb; i+=sectionInfo[i].sfbCnt) {
  	/*Calc. section Merge Gain */
    mergeGainLookUp[i] = FindBigGain(sectionInfo,
                                       bitLookUp,
                                       sideInfoTab,
                                       i,
                                       (i + sectionInfo[i].sfbCnt));
  }

  while (TRUE) {
    int16_t maxMergeGain = 0, maxNdx = 0, maxNdxNext = 0, maxNdxLast = 0;

   /*findMaxMerge : */
    maxMergeGain = CalcSectBitMaxBig(mergeGainLookUp, sectionInfo, maxSfb, &maxNdx);


    if (maxMergeGain <= 0)
      break;


    maxNdxNext = maxNdx + sectionInfo[maxNdx].sfbCnt;

    sectionInfo[maxNdx].sfbCnt = sectionInfo[maxNdx].sfbCnt + sectionInfo[maxNdxNext].sfbCnt;
    sectionInfo[maxNdx].sectionBits = sectionInfo[maxNdx].sectionBits +
                                          (sectionInfo[maxNdxNext].sectionBits - maxMergeGain);


    FindBitBigTable(bitLookUp[maxNdx], bitLookUp[maxNdxNext]);


    if (maxNdx != 0) {
      maxNdxLast = sectionInfo[maxNdx - 1].sfbStart;
      mergeGainLookUp[maxNdxLast] = FindBigGain(sectionInfo,
                                                  bitLookUp,
                                                  sideInfoTab,
                                                  maxNdxLast,
                                                  maxNdx);
    }
    maxNdxNext = maxNdx + sectionInfo[maxNdx].sfbCnt;

    sectionInfo[maxNdxNext - 1].sfbStart = sectionInfo[maxNdx].sfbStart;


    if (maxNdxNext - maxSfb < 0) {
      mergeGainLookUp[maxNdx] = FindBigGain(sectionInfo,
                                              bitLookUp,
                                              sideInfoTab,
                                              maxNdx,
                                              maxNdxNext);
    }
  }
}



/*******************************************************************************
*
* functionname: Nl_ScalefactorsCntBit
* returns     : ---
* description : count bits used by scalefactors.
*
********************************************************************************/
static void Nl_ScalefactorsCntBit(const int16_t *scalefacGain,
                     const uint16_t *maxValueInSfb,
                     GM_SECTION_DATA * sectionData)

{
  GM_SECTION_PARAM *psectionInfo;
  GM_SECTION_PARAM *psectionInfom;

  /* counter */
  int32_t i = 0; /* section counter */
  int32_t j = 0; /* sfb counter */
  int32_t k = 0; /* current section auxiliary counter */
  int32_t m = 0; /* other section auxiliary counter */
  int32_t n = 0; /* other sfb auxiliary counter */

  /* further variables */
  int32_t lastValScf     = 0;
  int32_t deltaScf       = 0;
  Flag found            = 0;
  int32_t scfSkipCounter = 0;


  sectionData->scalefacBits = 0;


  if (scalefacGain == NULL) {
    return;
  }

  lastValScf = 0;
  sectionData->firstScf = 0;

  psectionInfo = sectionData->sectionInfo;
  for (i=0; i<sectionData->noOfSections; i++) {

    if (psectionInfo->codeBook != CODE_BOOK_ZERO_NO) {
      sectionData->firstScf = psectionInfo->sfbStart;
      lastValScf = scalefacGain[sectionData->firstScf];
      break;
    }
	psectionInfo += 1;
  }

  psectionInfo = sectionData->sectionInfo;
  for (i=0; i<sectionData->noOfSections; i++, psectionInfo += 1) {

    if (psectionInfo->codeBook != CODE_BOOK_ZERO_NO
        && psectionInfo->codeBook != CODE_BOOK_PNS_NO) {
      for (j = psectionInfo->sfbStart;
           j < (psectionInfo->sfbStart + psectionInfo->sfbCnt); j++) {
        /* check if we can repeat the last value to save bits */

        if (maxValueInSfb[j] == 0) {
          found = 0;

          if (scfSkipCounter == 0) {
            /* end of section */

            if (j - ((psectionInfo->sfbStart + psectionInfo->sfbCnt) - 1) == 0) {
              found = 0;
            }
            else {
              for (k = j + 1; k < psectionInfo->sfbStart + psectionInfo->sfbCnt; k++) {

                if (maxValueInSfb[k] != 0) {
                  int tmp = L_abs(scalefacGain[k] - lastValScf);
				  found = 1;

                  if ( tmp < CODE_BOOK_SCF_LAV) {
                    /* save bits */
                    deltaScf = 0;
                  }
                  else {
                    /* do not save bits */
                    deltaScf = lastValScf - scalefacGain[j];
                    lastValScf = scalefacGain[j];
                    scfSkipCounter = 0;
                  }
                  break;
                }
                /* count scalefactor skip */
                scfSkipCounter = scfSkipCounter + 1;
              }
            }

			psectionInfom = psectionInfo + 1;
            /* search for the next maxValueInSfb[] != 0 in all other sections */
            for (m = i + 1; (m < sectionData->noOfSections) && (found == 0); m++) {

              if ((psectionInfom->codeBook != CODE_BOOK_ZERO_NO) &&
                  (psectionInfom->codeBook != CODE_BOOK_PNS_NO)) {
                for (n = psectionInfom->sfbStart;
                     n < (psectionInfom->sfbStart + psectionInfom->sfbCnt); n++) {

                  if (maxValueInSfb[n] != 0) {
                    found = 1;

                    if ( (abs_s(scalefacGain[n] - lastValScf) < CODE_BOOK_SCF_LAV)) {
                      deltaScf = 0;
                    }
                    else {
                      deltaScf = (lastValScf - scalefacGain[j]);
                      lastValScf = scalefacGain[j];
                      scfSkipCounter = 0;
                    }
                    break;
                  }
                  /* count scalefactor skip */
                  scfSkipCounter = scfSkipCounter + 1;
                }
              }

			  psectionInfom += 1;
            }

            if (found == 0) {
              deltaScf = 0;
              scfSkipCounter = 0;
            }
          }
          else {
            deltaScf = 0;
            scfSkipCounter = scfSkipCounter - 1;
          }
        }
        else {
          deltaScf = lastValScf - scalefacGain[j];
          lastValScf = scalefacGain[j];
        }
        //sectionData->scalefacBits += Get_HuffTableScf(deltaScf);
		//return(huff_ltabscf[delta+CODE_BOOK_SCF_LAV]);
		sectionData->scalefacBits += huff_ltabscf[deltaScf+CODE_BOOK_SCF_LAV];
      }
    }
  }
}



/*
*
*  description: count bits used by the noiseless coder
*               Nl: noiseless
*/
static void
Nl_CounBit(GM_SECTION_DATA *sectionData,
                 int16_t mergeGainLookUp[MAX_SFB_LONG],
                 int16_t bitLookUp[MAX_SFB_LONG][CODE_BOOK_ESC_NDX + 1],
                 const int16_t *quantSpectrum,
                 const uint16_t *maxValueInSfb,
                 const int16_t *sfbOffset,
                 const int32_t blockType)
{
  int32_t grpNdx, i;
  const int16_t *sideInfoTab = NULL;
  GM_SECTION_PARAM *sectionInfo;

  /*
    use appropriate side info table
  */
  switch (blockType)
  {
    case LONG_WINDOW:
    case START_WINDOW:
    case STOP_WINDOW:
      sideInfoTab = sideInfoTabLong;
      break;
    case SHORT_WINDOW:
      sideInfoTab = sideInfoTabShort;
      break;
  }


  sectionData->noOfSections = 0;
  sectionData->huffmanBits = 0;
  sectionData->sideInfoBits = 0;


  if (sectionData->maxSfbPerGroup == 0)
    return;

  /*
    loop trough groups
  */
  for (grpNdx=0; grpNdx<sectionData->sfbCnt; grpNdx+=sectionData->sfbPerGroup) {

    sectionInfo = sectionData->sectionInfo + sectionData->noOfSections;

    Nl_BitCntHuffTable(quantSpectrum,
                   sectionData->maxSfbPerGroup,
                   sfbOffset + grpNdx,
                   maxValueInSfb + grpNdx,
                   bitLookUp,
                   sectionInfo);

    /*
       0.Stage
    */
    Codbooks_1(sectionInfo, bitLookUp, sectionData->maxSfbPerGroup);

    /*
       1.Stage
    */
    Codbooks_2(sectionInfo, bitLookUp, sectionData->maxSfbPerGroup, sideInfoTab);


    /*
       2.Stage
    */
    Codbooks_3(sectionInfo,
             mergeGainLookUp,
             bitLookUp,
             sectionData->maxSfbPerGroup,
             sideInfoTab);


    /*
       compress output, calculate total huff and side bits
    */
    for (i=0; i<sectionData->maxSfbPerGroup; i+=sectionInfo[i].sfbCnt) {
      GoodCodBook(bitLookUp[i], &(sectionInfo[i].codeBook));
      sectionInfo[i].sfbStart = sectionInfo[i].sfbStart + grpNdx;

      sectionData->huffmanBits = (sectionData->huffmanBits +
                                     (sectionInfo[i].sectionBits - sideInfoTab[sectionInfo[i].sfbCnt]));
      sectionData->sideInfoBits = (sectionData->sideInfoBits + sideInfoTab[sectionInfo[i].sfbCnt]);
      sectionData->sectionInfo[sectionData->noOfSections] = sectionInfo[i];
      sectionData->noOfSections = sectionData->noOfSections + 1;
    }
  }
}






typedef int16_t (*lookUpTable)[CODE_BOOK_ESC_NDX + 1];



int16_t NoiselessCoder(const int16_t  *quantSpectrum,
            const uint16_t *maxValueInSfb,
            const int16_t  *scalefac,
            const int16_t   blockType,
            const int16_t   sfbCnt,
            const int16_t   maxSfbPerGroup,
            const int16_t   sfbPerGroup,
            const int16_t  *sfbOffset,
            GM_SECTION_DATA  *sectionData)
{
  sectionData->blockType      = blockType;
  sectionData->sfbCnt         = sfbCnt;
  sectionData->sfbPerGroup    = sfbPerGroup;
  if(sfbPerGroup)
	sectionData->noOfGroups   = sfbCnt/sfbPerGroup;
  else
	sectionData->noOfGroups   = 0x7fff;
  sectionData->maxSfbPerGroup = maxSfbPerGroup;

  Nl_CounBit(sectionData,
                   sectionData->mergeGainLookUp,
                   (lookUpTable)sectionData->bitLookUp,
                   quantSpectrum,
                   maxValueInSfb,
                   sfbOffset,
                   blockType);

  Nl_ScalefactorsCntBit(scalefac,
           maxValueInSfb,
           sectionData);


  return (sectionData->huffmanBits + sectionData->sideInfoBits +
	      sectionData->scalefacBits);
}
