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
	File:		buf_manag.c

	Content:	Bit Buffer Management functions

*******************************************************************************/

#include "buf_manag.h"

/*****************************************************************************
*
* function name: updateBitBufWordPtr
* description:  update Bit Buffer pointer
*
*****************************************************************************/
/*
static void updateBitBufWordPtr(HANDLE_BIT_BUF hBitBuf,
                                uint8_t **pBitBufWord,
                                int16_t   cnt)
{
  *pBitBufWord += cnt;


  if(*pBitBufWord > hBitBuf->pBitBufEnd) {
    *pBitBufWord -= (hBitBuf->pBitBufEnd - hBitBuf->pBitBufBase + 1);
  }

  if(*pBitBufWord < hBitBuf->pBitBufBase) {
    *pBitBufWord += (hBitBuf->pBitBufEnd - hBitBuf->pBitBufBase + 1);
  }
}
*/

/*****************************************************************************
*
* function name: BuildNew_BitBuffer
* description:  create and init Bit Buffer Management
*
*****************************************************************************/
HANDLE_BIT_BUF BuildNew_BitBuffer(HANDLE_BIT_BUF hBitBuf,
                               uint8_t *pBitBufBase,
                               int16_t  bitBufSize)
{
  hBitBuf->pBitBufBase = pBitBufBase;
  hBitBuf->pBitBufEnd  = pBitBufBase + bitBufSize - 1;

  hBitBuf->pWriteNext  = pBitBufBase;

  hBitBuf->cache       = 0;

  hBitBuf->wBitPos     = 0;
  hBitBuf->cntBits     = 0;

  hBitBuf->size        = (bitBufSize << 3);
  hBitBuf->isValid     = 1;

  return hBitBuf;
}

/*****************************************************************************
*
* function name: Del_BitBuf
* description:  uninit Bit Buffer Management
*
*****************************************************************************/

void Del_BitBuf(HANDLE_BIT_BUF *hBitBuf)
{
  if(*hBitBuf) {
	(*hBitBuf)->isValid = 0;
  }
  *hBitBuf = NULL;
}


/*****************************************************************************
*
* function name: Set0Start_BitBufManag
* description:  reset Bit Buffer Management
*
*****************************************************************************/
/*
void Set0Start_BitBufManag(HANDLE_BIT_BUF hBitBuf,
                 uint8_t *pBitBufBase,
                 int16_t  bitBufSize)
{
  hBitBuf->pBitBufBase = pBitBufBase;
  hBitBuf->pBitBufEnd  = pBitBufBase + bitBufSize - 1;


  hBitBuf->pWriteNext  = pBitBufBase;

  hBitBuf->wBitPos     = 0;
  hBitBuf->cntBits     = 0;

  hBitBuf->cache	   = 0;
}
*/

/*****************************************************************************
*
* function name: CopyBitBuf
* description:  copy Bit Buffer Management
*
*****************************************************************************/
/*
void CopyBitBuf(HANDLE_BIT_BUF hBitBufSrc,
                HANDLE_BIT_BUF hBitBufDst)
{
  *hBitBufDst = *hBitBufSrc;
}
*/
/*****************************************************************************
*
* function name: Get_BitsCnt
* description:  get available bits
*
*****************************************************************************/
/*
int16_t Get_BitsCnt(HANDLE_BIT_BUF hBitBuf)
{
  return hBitBuf->cntBits;
}
*/
/*****************************************************************************
*
* function name: Write_Bits2Buf
* description:  write bits to the buffer
* Note: buf,value,bits ; write byte unit ,if write len< byte -> save to cache
*****************************************************************************/
int16_t Write_Bits2Buf(HANDLE_BIT_BUF hBitBuf,
                 uint32_t writeValue,
                 int16_t noBitsToWrite)
{
  int16_t wBitPos;

//  assert(noBitsToWrite <= (int16_t)sizeof(int32_t)*8);

  if(noBitsToWrite == 0) {
	  return noBitsToWrite;
  }
  hBitBuf->cntBits += noBitsToWrite;

  wBitPos = hBitBuf->wBitPos;
  wBitPos += noBitsToWrite;
  writeValue &= ~((~0) << noBitsToWrite); // Mask out everything except the lowest noBitsToWrite bits
  writeValue <<= 32 - wBitPos;
  writeValue |= hBitBuf->cache;

  while (wBitPos >= 8)
  {
	  uint8_t tmp;
	  tmp = (uint8_t)((writeValue >> 24) & 0xFF);

	  *hBitBuf->pWriteNext++ = tmp;
	  writeValue <<= 8;
	  wBitPos -= 8;
  }

  hBitBuf->wBitPos = wBitPos;
  hBitBuf->cache = writeValue;

  return noBitsToWrite;
}
