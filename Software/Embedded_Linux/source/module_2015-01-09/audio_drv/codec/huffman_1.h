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
	File:		huffman_1.h

	Content:	Huffman Bitcounter & coder structure and functions

*******************************************************************************/

#ifndef _huffman_1_h
#define _huffman_1_h

#include "buf_manag.h"
#include "basic_op.h"
#define INVALID_BITCOUNT (MAX_16/4)

/*
  code book number table
*/

enum codeBookNo{
  CODE_BOOK_ZERO_NO=               0,
  CODE_BOOK_1_NO=                  1,
  CODE_BOOK_2_NO=                  2,
  CODE_BOOK_3_NO=                  3,
  CODE_BOOK_4_NO=                  4,
  CODE_BOOK_5_NO=                  5,
  CODE_BOOK_6_NO=                  6,
  CODE_BOOK_7_NO=                  7,
  CODE_BOOK_8_NO=                  8,
  CODE_BOOK_9_NO=                  9,
  CODE_BOOK_10_NO=                10,
  CODE_BOOK_ESC_NO=               11,
  CODE_BOOK_RES_NO=               12,
  CODE_BOOK_PNS_NO=               13
};

/*
  code book index table
*/

enum codeBookNdx{
  CODE_BOOK_ZERO_NDX=0,
  CODE_BOOK_1_NDX,
  CODE_BOOK_2_NDX,
  CODE_BOOK_3_NDX,
  CODE_BOOK_4_NDX,
  CODE_BOOK_5_NDX,
  CODE_BOOK_6_NDX,
  CODE_BOOK_7_NDX,
  CODE_BOOK_8_NDX,
  CODE_BOOK_9_NDX,
  CODE_BOOK_10_NDX,
  CODE_BOOK_ESC_NDX,
  CODE_BOOK_RES_NDX,
  CODE_BOOK_PNS_NDX,
  NUMBER_OF_CODE_BOOKS
};

/*
  code book lav table
*/

enum codeBookLav{
  CODE_BOOK_ZERO_LAV=0,
  CODE_BOOK_1_LAV=1,
  CODE_BOOK_2_LAV=1,
  CODE_BOOK_3_LAV=2,
  CODE_BOOK_4_LAV=2,
  CODE_BOOK_5_LAV=4,
  CODE_BOOK_6_LAV=4,
  CODE_BOOK_7_LAV=7,
  CODE_BOOK_8_LAV=7,
  CODE_BOOK_9_LAV=12,
  CODE_BOOK_10_LAV=12,
  CODE_BOOK_ESC_LAV=16,
  CODE_BOOK_SCF_LAV=60,
  CODE_BOOK_PNS_LAV=60
};

int16_t Call_bitCntFunTab(const int16_t *aQuantSpectrum,
                const int16_t  noOfSpecLines,
                int16_t        maxVal,
                int16_t       *bitCountLut);

int16_t Write_HuffBits(int16_t *values, int16_t width, int16_t codeBook, HANDLE_BIT_BUF hBitstream);

int16_t Get_HuffTableScf(int16_t delta);
int16_t Write_HuffTabScfBitLen(int16_t scalefactor, HANDLE_BIT_BUF hBitstream);




#endif
