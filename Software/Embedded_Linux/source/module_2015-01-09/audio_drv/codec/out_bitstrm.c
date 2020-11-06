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
	File:		out_bitstrm.c

	Content:	Bitstream encoder functions

*******************************************************************************/

#include "out_bitstrm.h"
#include "huffman_1.h"
#include "noiseless_code.h"
#include "quantizer_data.h"
//#include "interface.h"
#include "perceptual_main.h"


static const  int16_t globalGainOffset = 100;
static const  int16_t icsReservedBit   = 0;


/*****************************************************************************
*
* function name: SpectrPartBits
* description:  encode spectral data
* returns:      spectral bits used
*
*****************************************************************************/
static int32_t SpectrPartBits(int16_t             *sfbOffset,
                                 GM_SECTION_DATA       *sectionData,
                                 int16_t             *quantSpectrum,
                                 HANDLE_BIT_BUF      hBitStream)
{
  int16_t i,sfb;
  int16_t dbgVal;
  GM_SECTION_PARAM* psectioninfo;
  //dbgVal = Get_BitsCnt(hBitStream);
  dbgVal = hBitStream->cntBits;
 

  for(i=0; i<sectionData->noOfSections; i++) {
    psectioninfo = &(sectionData->sectionInfo[i]);
	/*
       huffencode spectral data for this section
    */
    for(sfb=psectioninfo->sfbStart;
        sfb<psectioninfo->sfbStart+psectioninfo->sfbCnt;
        sfb++) {
      Write_HuffBits(quantSpectrum+sfbOffset[sfb],
                 sfbOffset[sfb+1] - sfbOffset[sfb],
                 psectioninfo->codeBook,
                 hBitStream);
    }
  }

  //return(Get_BitsCnt(hBitStream)-dbgVal);
  return((hBitStream->cntBits)-dbgVal);
}

/*****************************************************************************
*
* function name:GlobalGainPartBits
* description: encodes Global Gain (common scale factor)
* returns:     none
*
*****************************************************************************/
static void GlobalGainPartBits(int16_t globalGain,
                             int16_t logNorm,
                             int16_t scalefac,
                             HANDLE_BIT_BUF hBitStream)
{
  Write_Bits2Buf(hBitStream, ((globalGain - scalefac) + globalGainOffset-(logNorm << 2)), 8);
}


/*****************************************************************************
*
* function name:GlobalGainPartBits
* description: encodes Ics Info
* returns:     none
*
*****************************************************************************/

static void IcsPartBits(int16_t blockType,
                          int16_t windowShape,
                          int16_t groupingMask,
                          GM_SECTION_DATA *sectionData,
                          HANDLE_BIT_BUF  hBitStream)
{
  Write_Bits2Buf(hBitStream,icsReservedBit,1);
  Write_Bits2Buf(hBitStream,blockType,2);
  Write_Bits2Buf(hBitStream,windowShape,1);


  switch(blockType){
    case LONG_WINDOW:
    case START_WINDOW:
    case STOP_WINDOW:
      Write_Bits2Buf(hBitStream,sectionData->maxSfbPerGroup,6);

      /* No predictor data present */
      Write_Bits2Buf(hBitStream, 0, 1);
      break;

    case SHORT_WINDOW:
      Write_Bits2Buf(hBitStream,sectionData->maxSfbPerGroup,4);

      /*
      Write grouping bits
      */
      Write_Bits2Buf(hBitStream,groupingMask,TRANS_FAC-1);
      break;
  }
}

/*****************************************************************************
*
* function name: SectionPartBits
* description:  encode section data (common Huffman codebooks for adjacent
*               SFB's)
* returns:      none
*
*****************************************************************************/
static int32_t SectionPartBits(GM_SECTION_DATA *sectionData,
                                HANDLE_BIT_BUF hBitStream)
{
  int16_t sectEscapeVal=0,sectLenBits=0;
  int16_t sectLen;
  int16_t i;
  //int16_t dbgVal=Get_BitsCnt(hBitStream);
  int16_t dbgVal=(hBitStream->cntBits);//hBitBuf->cntBits



  switch(sectionData->blockType)
  {
    case LONG_WINDOW:
    case START_WINDOW:
    case STOP_WINDOW:
      sectEscapeVal = SECT_ESC_VAL_LONG;
      sectLenBits   = SECT_BITS_LONG;
      break;

    case SHORT_WINDOW:
      sectEscapeVal = SECT_ESC_VAL_SHORT;
      sectLenBits   = SECT_BITS_SHORT;
      break;
  }

  for(i=0;i<sectionData->noOfSections;i++) {
    Write_Bits2Buf(hBitStream,sectionData->sectionInfo[i].codeBook,4);
    sectLen = sectionData->sectionInfo[i].sfbCnt;

    while(sectLen >= sectEscapeVal) {

      Write_Bits2Buf(hBitStream,sectEscapeVal,sectLenBits);
      sectLen = sectLen - sectEscapeVal;
    }
    Write_Bits2Buf(hBitStream,sectLen,sectLenBits);
  }
 // return(Get_BitsCnt(hBitStream)-dbgVal);
 return((hBitStream->cntBits)-dbgVal);
}

/*****************************************************************************
*
* function name: ScaleFactorPartBits
* description:  encode DPCM coded scale factors
* returns:      none
*
*****************************************************************************/
static int32_t ScaleFactorPartBits(uint16_t      *maxValueInSfb,
                                    GM_SECTION_DATA   *sectionData,
                                    int16_t        *scalefac,
                                    HANDLE_BIT_BUF hBitStream)
{
  int16_t i,j,lastValScf,deltaScf;
  //int16_t dbgVal = Get_BitsCnt(hBitStream); //hBitBuf->cntBits
  int16_t dbgVal = (hBitStream->cntBits);
  GM_SECTION_PARAM* psectioninfo;

  lastValScf=scalefac[sectionData->firstScf];

  for(i=0;i<sectionData->noOfSections;i++){
    psectioninfo = &(sectionData->sectionInfo[i]);
    if (psectioninfo->codeBook != CODE_BOOK_ZERO_NO){
      for (j=psectioninfo->sfbStart;
           j<psectioninfo->sfbStart+psectioninfo->sfbCnt; j++){

        if(maxValueInSfb[j] == 0) {
          deltaScf = 0;
        }
        else {
          deltaScf = lastValScf - scalefac[j];
          lastValScf = scalefac[j];
        }
        
        if(Write_HuffTabScfBitLen(deltaScf,hBitStream)){
          return(1);
        }
      }
    }

  }
  //return(Get_BitsCnt(hBitStream)-dbgVal);
  return((hBitStream->cntBits)-dbgVal);
}

/*****************************************************************************
*
* function name:MSPartBits
* description: encodes MS-Stereo Info
* returns:     none
*
*****************************************************************************/
static void MSPartBits(int16_t          sfbCnt,
                         int16_t          grpSfb,
                         int16_t          maxSfb,
                         int16_t          msDigest,
                         int16_t         *jsFlags,
                         HANDLE_BIT_BUF  hBitStream)
{
  int16_t sfb, sfbOff;

  switch(msDigest)
  {
    case MS_NONE:
      Write_Bits2Buf(hBitStream,SI_MS_MASK_NONE,2);
      break;

    case MS_ALL:
      Write_Bits2Buf(hBitStream,SI_MS_MASK_ALL,2);
      break;

    case MS_SOME:
      Write_Bits2Buf(hBitStream,SI_MS_MASK_SOME,2);
      for(sfbOff = 0; sfbOff < sfbCnt; sfbOff+=grpSfb) {
        for(sfb=0; sfb<maxSfb; sfb++) {

          if(jsFlags[sfbOff+sfb] & MS_ON) {
            Write_Bits2Buf(hBitStream,1,1);
          }
          else{
            Write_Bits2Buf(hBitStream,0,1);
          }
        }
      }
      break;
  }

}

/*****************************************************************************
*
* function name: TnsPartBits
* description:  encode TNS data (filter order, coeffs, ..)
* returns:      none
*
*****************************************************************************/
static void TnsPartBits(GM_TNS_BLK_INFO tnsInfo,
                          int16_t blockType,
                          HANDLE_BIT_BUF hBitStream) {
  int16_t i,k;
  Flag tnsPresent;
  int16_t numOfWindows;
  int16_t coefBits;
  Flag isShort;


  if (blockType==2) {
    isShort = 1;
    numOfWindows = TRANS_FAC;
  }
  else {
    isShort = 0;
    numOfWindows = 1;
  }

  tnsPresent=0;
  for (i=0; i<numOfWindows; i++) {

    if (tnsInfo.tnsActive[i]) {
      tnsPresent=1;
    }
  }

  if (tnsPresent==0) {
    Write_Bits2Buf(hBitStream,0,1);
  }
  else{ /* there is data to be written*/
    Write_Bits2Buf(hBitStream,1,1); /*data_present */
    for (i=0; i<numOfWindows; i++) {

      Write_Bits2Buf(hBitStream,tnsInfo.tnsActive[i],(isShort?1:2));

      if (tnsInfo.tnsActive[i]) {

        Write_Bits2Buf(hBitStream,((tnsInfo.coefRes[i] - 4)==0?1:0),1);

        Write_Bits2Buf(hBitStream,tnsInfo.length[i],(isShort?4:6));

        Write_Bits2Buf(hBitStream,tnsInfo.order[i],(isShort?3:5));

        if (tnsInfo.order[i]){
          Write_Bits2Buf(hBitStream, FILTER_DIRECTION, 1);

          if(tnsInfo.coefRes[i] == 4) {
            coefBits = 3;
            for(k=0; k<tnsInfo.order[i]; k++) {

              if (tnsInfo.coef[i*TNS_MAX_ORDER_SHORT+k] > 3 ||
                  tnsInfo.coef[i*TNS_MAX_ORDER_SHORT+k] < -4) {
                coefBits = 4;
                break;
              }
            }
          }
          else {
            coefBits = 2;
            for(k=0; k<tnsInfo.order[i]; k++) {

              if (tnsInfo.coef[i*TNS_MAX_ORDER_SHORT+k] > 1 ||
                  tnsInfo.coef[i*TNS_MAX_ORDER_SHORT+k] < -2) {
                coefBits = 3;
                break;
              }
            }
          }
          Write_Bits2Buf(hBitStream, tnsInfo.coefRes[i] - coefBits, 1); /*coef_compres*/
          for (k=0; k<tnsInfo.order[i]; k++ ) {
            static const int16_t rmask[] = {0,1,3,7,15};

            Write_Bits2Buf(hBitStream,tnsInfo.coef[i*TNS_MAX_ORDER_SHORT+k] & rmask[coefBits],coefBits);
          }
        }
      }
    }
  }

}

/*****************************************************************************
*
* function name: GainControlPartBits
* description:  unsupported
* returns:      none
*
*****************************************************************************/
/*
static void GainControlPartBits(HANDLE_BIT_BUF hBitStream)
{
  Write_Bits2Buf(hBitStream,0,1);
}
*/
/*****************************************************************************
*
* function name: encodePulseData
* description:  not supported yet (dummy)
* returns:      none
*
*****************************************************************************/
/*
static void encodePulseData(HANDLE_BIT_BUF hBitStream)
{
  Write_Bits2Buf(hBitStream,0,1);
}
*/

/*****************************************************************************
*
* function name: OutPersonalChannelBits
* description:  management of write process of individual channel stream
* returns:      none
*
*****************************************************************************/
static void
OutPersonalChannelBits(Flag   commonWindow,
                             int16_t mdctScale,
                             int16_t windowShape,
                             int16_t groupingMask,
                             int16_t *sfbOffset,
                             int16_t scf[],
                             uint16_t *maxValueInSfb,
                             int16_t globalGain,
                             int16_t quantSpec[],
                             GM_SECTION_DATA *sectionData,
                             HANDLE_BIT_BUF hBitStream,
                             GM_TNS_BLK_INFO tnsInfo)
{
  int16_t logNorm;

  logNorm = LOG_NORM_PCM - (mdctScale + 1);

  GlobalGainPartBits(globalGain, logNorm,scf[sectionData->firstScf], hBitStream);


  if(!commonWindow) {
    IcsPartBits(sectionData->blockType, windowShape, groupingMask, sectionData, hBitStream);
  }

  SectionPartBits(sectionData, hBitStream);

  ScaleFactorPartBits(maxValueInSfb,
                        sectionData,
                        scf,
                        hBitStream);

  //encodePulseData(hBitStream);
  Write_Bits2Buf(hBitStream,0,1);

  TnsPartBits(tnsInfo, sectionData->blockType, hBitStream);

  //GainControlPartBits(hBitStream);
  Write_Bits2Buf(hBitStream,0,1); 

  SpectrPartBits(sfbOffset,
                     sectionData,
                     quantSpec,
                     hBitStream);

}

/*****************************************************************************
*
* function name: OutSingleChannelBits
* description:  write single channel element to bitstream
* returns:      none
*
*****************************************************************************/
static int16_t OutSingleChannelBits(int16_t instanceTag,
                                        int16_t *sfbOffset,
                                        GM_QC_CHANNDATA_OUT* qcOutChannel,
                                        HANDLE_BIT_BUF hBitStream,
                                        GM_TNS_BLK_INFO tnsInfo)
{
  Write_Bits2Buf(hBitStream,ID_SCE,3);
  Write_Bits2Buf(hBitStream,instanceTag,4);
  OutPersonalChannelBits(0,
                               qcOutChannel->mdctScale,
                               qcOutChannel->windowShape,
                               qcOutChannel->groupingMask,
                               sfbOffset,
                               qcOutChannel->scf,
                               qcOutChannel->maxValueInSfb,
                               qcOutChannel->globalGain,
                               qcOutChannel->quantSpec,
                               &(qcOutChannel->sectionData),
                               hBitStream,
                               tnsInfo
                               );
  return(0);
}



/*****************************************************************************
*
* function name: OutPairChannelBits
* description:
* returns:      none
*
*****************************************************************************/
static int16_t OutPairChannelBits(int16_t instanceTag,
                                      int16_t msDigest,
                                      int16_t msFlags[MAX_GROUPED_SFB],
                                      int16_t *sfbOffset[2],
                                      GM_QC_CHANNDATA_OUT qcOutChannel[2],
                                      HANDLE_BIT_BUF hBitStream,
                                      GM_TNS_BLK_INFO tnsInfo[2])
{
  Write_Bits2Buf(hBitStream,ID_CPE,3);
  Write_Bits2Buf(hBitStream,instanceTag,4);
  Write_Bits2Buf(hBitStream,1,1); /* common window */

  IcsPartBits(qcOutChannel[0].sectionData.blockType,
                qcOutChannel[0].windowShape,
                qcOutChannel[0].groupingMask,
                &(qcOutChannel[0].sectionData),
                hBitStream);

  MSPartBits(qcOutChannel[0].sectionData.sfbCnt,
               qcOutChannel[0].sectionData.sfbPerGroup,
               qcOutChannel[0].sectionData.maxSfbPerGroup,
               msDigest,
               msFlags,
               hBitStream);

  OutPersonalChannelBits(1,
                               qcOutChannel[0].mdctScale,
                               qcOutChannel[0].windowShape,
                               qcOutChannel[0].groupingMask,
                               sfbOffset[0],
                               qcOutChannel[0].scf,
                               qcOutChannel[0].maxValueInSfb,
                               qcOutChannel[0].globalGain,
                               qcOutChannel[0].quantSpec,
                               &(qcOutChannel[0].sectionData),
                               hBitStream,
                               tnsInfo[0]);

  OutPersonalChannelBits(1,
                               qcOutChannel[1].mdctScale,
                               qcOutChannel[1].windowShape,
                               qcOutChannel[1].groupingMask,
                               sfbOffset[1],
                               qcOutChannel[1].scf,
                               qcOutChannel[1].maxValueInSfb,
                               qcOutChannel[1].globalGain,
                               qcOutChannel[1].quantSpec,
                               &(qcOutChannel[1].sectionData),
                               hBitStream,
                               tnsInfo[1]);

  return(0);
}



/*****************************************************************************
*
* function name: OutSubBlkPartBits
* description:  write fill elements to bitstream
* returns:      none
*
*****************************************************************************/
static void OutSubBlkPartBits( const uint8_t *ancBytes,
                              int16_t totFillBits,
                              HANDLE_BIT_BUF hBitStream)
{
  int16_t i;
  int16_t cnt,esc_count;

  /*
    Write fill Element(s):
    amount of a fill element can be 7+X*8 Bits, X element of [0..270]
  */

  while(totFillBits >= (3+4)) {
    cnt = min(((totFillBits - (3+4)) >> 3), ((1<<4)-1));

    Write_Bits2Buf(hBitStream,ID_FIL,3);
    Write_Bits2Buf(hBitStream,cnt,4);

    totFillBits = totFillBits - (3+4);


    if ((cnt == (1<<4)-1)) {

      esc_count = min( ((totFillBits >> 3) - ((1<<4)-1)), (1<<8)-1);
      Write_Bits2Buf(hBitStream,esc_count,8);
      totFillBits = (totFillBits - 8);
      cnt = cnt + (esc_count - 1);
    }

    for(i=0;i<cnt;i++) {

      if(ancBytes)
        Write_Bits2Buf(hBitStream, *ancBytes++,8);
      else
        Write_Bits2Buf(hBitStream,0,8);
      totFillBits = totFillBits - 8;
    }
  }
}

/*****************************************************************************
*
* function name: OutEncoder_BitData
* description:  main function of write bitsteam process
* returns:      0 if success
*
*****************************************************************************/
int16_t OutEncoder_BitData (int id,
											 HANDLE_BIT_BUF hBitStream,
                       GM_PART_INFO elInfo,
                       GM_QC_OUTSTRM *qcOut,
                       GM_PERCEP_OUT *psyOut,
                       int16_t *globUsedBits,
                       const uint8_t *ancBytes,
					   int16_t sampindex
                       ) /* returns error code */
{
  int16_t bitMarkUp;
  int16_t elementUsedBits;
  int16_t frameBits=0;

  /*   struct bitbuffer bsWriteCopy; */
  //bitMarkUp = Get_BitsCnt(hBitStream);//hBitStream->cntBits
  bitMarkUp = (hBitStream->cntBits);
  if(qcOut->qcElement.adtsUsed)  /*  write adts header*/
  {
	  Write_Bits2Buf(hBitStream, 0xFFF, 12); /* 12 bit Syncword */
	  Write_Bits2Buf(hBitStream, id, 1); /* ID == 0 for MPEG4 AAC, 1 for MPEG2 AAC */
	  Write_Bits2Buf(hBitStream, 0, 2); /* layer == 0 */
	  Write_Bits2Buf(hBitStream, 1, 1); /* protection absent */
	  Write_Bits2Buf(hBitStream, 1, 2); /* profile */
	  Write_Bits2Buf(hBitStream, sampindex, 4); /* sampling rate */
	  Write_Bits2Buf(hBitStream, 0, 1); /* private bit */
	  Write_Bits2Buf(hBitStream, elInfo.nChannelsInEl, 3); /* ch. config (must be > 0) */
								   /* simply using numChannels only works for
									6 channels or less, else a channel
									configuration should be written */
	  Write_Bits2Buf(hBitStream, 0, 1); /* original/copy */
	  Write_Bits2Buf(hBitStream, 0, 1); /* home */

	  /* Variable ADTS header */
	  Write_Bits2Buf(hBitStream, 0, 1); /* copyr. id. bit */
	  Write_Bits2Buf(hBitStream, 0, 1); /* copyr. id. start */
	  Write_Bits2Buf(hBitStream, *globUsedBits >> 3, 13);
	  Write_Bits2Buf(hBitStream, 0x7FF, 11); /* buffer fullness (0x7FF for VBR) */
	  Write_Bits2Buf(hBitStream, 0, 2); /* raw data blocks (0+1=1) */
  }

  *globUsedBits=0;

  {

    int16_t *sfbOffset[2];
    GM_TNS_BLK_INFO tnsInfo[2];
    elementUsedBits = 0;

    switch (elInfo.elType) {

      case ID_SCE:      /* single channel */
        sfbOffset[0] = psyOut->psyOutChannel[elInfo.ChannelIndex[0]].sfbOffsets;
        tnsInfo[0] = psyOut->psyOutChannel[elInfo.ChannelIndex[0]].tnsInfo;

        OutSingleChannelBits(elInfo.instanceTag,
                                  sfbOffset[0],
                                  &qcOut->qcChannel[elInfo.ChannelIndex[0]],
                                  hBitStream,
                                  tnsInfo[0]);
        break;

      case ID_CPE:     /* channel pair */
        {
          int16_t msDigest;
          int16_t *msFlags = psyOut->psyOutElement.toolsInfo.msMask;
          msDigest = psyOut->psyOutElement.toolsInfo.msDigest;
          sfbOffset[0] =
            psyOut->psyOutChannel[elInfo.ChannelIndex[0]].sfbOffsets;
          sfbOffset[1] =
            psyOut->psyOutChannel[elInfo.ChannelIndex[1]].sfbOffsets;

          tnsInfo[0]=
            psyOut->psyOutChannel[elInfo.ChannelIndex[0]].tnsInfo;
          tnsInfo[1]=
            psyOut->psyOutChannel[elInfo.ChannelIndex[1]].tnsInfo;
          OutPairChannelBits(elInfo.instanceTag,
                                  msDigest,
                                  msFlags,
                                  sfbOffset,
                                  &qcOut->qcChannel[elInfo.ChannelIndex[0]],
                                  hBitStream,
                                  tnsInfo);
        }
        break;

      default:
        return(1);

      }   /* switch */

    elementUsedBits = elementUsedBits - bitMarkUp;
    //bitMarkUp = Get_BitsCnt(hBitStream);
    bitMarkUp = (hBitStream->cntBits);
    frameBits = frameBits + elementUsedBits + bitMarkUp;

  }

  OutSubBlkPartBits(NULL,
                   qcOut->totFillBits,
                   hBitStream);

  Write_Bits2Buf(hBitStream,ID_END,3);

  /* byte alignement */
  Write_Bits2Buf(hBitStream,0, (8 - (hBitStream->cntBits & 7)) & 7);

  *globUsedBits = *globUsedBits- bitMarkUp;
  //bitMarkUp = Get_BitsCnt(hBitStream);
  bitMarkUp = (hBitStream->cntBits);
  *globUsedBits = *globUsedBits + bitMarkUp;
  frameBits = frameBits + *globUsedBits;


  if (frameBits !=  (qcOut->totStaticBitsUsed+qcOut->totDynBitsUsed + qcOut->totAncBitsUsed +
                     qcOut->totFillBits + qcOut->alignBits)) {
    return(-1);
  }
  return(0);
}
