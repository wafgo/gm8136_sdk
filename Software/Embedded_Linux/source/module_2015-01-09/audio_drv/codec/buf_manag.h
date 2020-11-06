
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
	File:		buf_manag.h

	Content:	Bit Buffer Management structure and functions

*******************************************************************************/

#ifndef _buf_manag_h
#define _buf_manag_h

#include "typedef.h"


/*!
   The pointer 'pReadNext' points to the next available word, where bits can be read from. The pointer
   'pWriteNext' points to the next available word, where bits can be written to. The pointer pBitBufBase
   points to the start of the bitstream buffer and the pointer pBitBufEnd points to the end of the bitstream
   buffer. The two pointers are used as lower-bound respectively upper-bound address for the modulo addressing
   mode.

   The element cntBits contains the currently available bits in the bit buffer. It will be incremented when
   bits are written to the bitstream buffer and decremented when bits are read from the bitstream buffer.
*/
struct GM_STREAM_BITBUFMANAG
{
  uint8_t *pBitBufBase;          /*!< pointer points to first position in bitstream buffer */
  uint8_t *pBitBufEnd;           /*!< pointer points to last position in bitstream buffer */

  uint8_t *pWriteNext;           /*!< pointer points to next available word in bitstream buffer to write */

  uint32_t cache;  

  int16_t  wBitPos;              /*!< 31<=wBitPos<=0*/
  int16_t  cntBits;              /*!< number of available bits in the bitstream buffer
                                     write bits to bitstream buffer  => increment cntBits
                                     read bits from bitstream buffer => decrement cntBits */
                                     
  int16_t  size;                 /*!< size of bitbuffer in bits */
  int16_t  isValid;              /*!< indicates whether the instance has been initialized */
									 
}; /* size int16_t: 8 */

/*! Define pointer to bit buffer structure */
typedef struct GM_STREAM_BITBUFMANAG *HANDLE_BIT_BUF;



HANDLE_BIT_BUF BuildNew_BitBuffer(HANDLE_BIT_BUF hBitBuf,
                               uint8_t *pBitBufBase,
                               int16_t  bitBufSize);


void Del_BitBuf(HANDLE_BIT_BUF *hBitBuf);


//int16_t Get_BitsCnt(HANDLE_BIT_BUF hBitBuf);


int16_t Write_Bits2Buf(HANDLE_BIT_BUF hBitBuf,
                 uint32_t writeValue,
                 int16_t noBitsToWrite);
/*
void Set0Start_BitBufManag(HANDLE_BIT_BUF hBitBuf,
                 uint8_t *pBitBufBase,
                 int16_t  bitBufSize);
*/

//#define GetNrBitsAvailable(hBitBuf) ( (hBitBuf)->cntBits)
//#define GetNrBitsRead(hBitBuf)       ((hBitBuf)->size-(hBitBuf)->cntBits)

#endif /* _buf_manag_h */
