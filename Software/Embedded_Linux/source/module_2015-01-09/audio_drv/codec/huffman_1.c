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
	File:		Huffman_1.c

	Content:	Huffman Bitcounter & coder functions

*******************************************************************************/

#include "huffman_1.h"
#include "aac_table.h"

#define HI_LTAB(a) (a>>8)
#define LO_LTAB(a) (a & 0xff)

#define EXPAND(a)  ((((int32_t)(a&0xff00)) << 8)|(int32_t)(a&0xff))


/*****************************************************************************
*
* function name: Write_LookUpTabHuf_0
* description:  counts tables 1-11
* returns:
* input:        quantized spectrum
* output:       bitCount for tables 1-11
*
*****************************************************************************/

static void Write_LookUpTabHuf_0(const int16_t *values,
                                         const int16_t  width,
                                         int16_t       *bitCount)
{
  int32_t t0,t1,t2,t3,i;
  int32_t bc1_2,bc3_4,bc5_6,bc7_8,bc9_10;
  int16_t bc11,sc;
  int16_t *values_local;
  values_local = (int16_t *)values;
  bc1_2=0;
  bc3_4=0;
  bc5_6=0;
  bc7_8=0;
  bc9_10=0;
  bc11=0;
  sc=0;
  i = width/4;
//  for(i=0;i<width;i+=4){
  do {

    t0= *values_local++;
    t1= *values_local++;
    t2= *values_local++;
    t3= *values_local++;

    /* 1,2 */

    bc1_2 = bc1_2 + EXPAND(huff_ltab1_2[t0+1][t1+1][t2+1][t3+1]);

    /* 5,6 */
    bc5_6 = bc5_6 + EXPAND(huff_ltab5_6[t0+4][t1+4]);
    bc5_6 = bc5_6 + EXPAND(huff_ltab5_6[t2+4][t3+4]);

    t0=ABS(t0);
    t1=ABS(t1);
    t2=ABS(t2);
    t3=ABS(t3);


    bc3_4 = bc3_4 + EXPAND(huff_ltab3_4[t0][t1][t2][t3]);

    bc7_8 = bc7_8 + EXPAND(huff_ltab7_8[t0][t1]);
    bc7_8 = bc7_8 + EXPAND(huff_ltab7_8[t2][t3]);

    bc9_10 = bc9_10 + EXPAND(huff_ltab9_10[t0][t1]);
    bc9_10 = bc9_10 + EXPAND(huff_ltab9_10[t2][t3]);

    bc11 = bc11 + huff_ltab11[t0][t1];
    bc11 = bc11 + huff_ltab11[t2][t3];


    sc = sc + (t0>0) + (t1>0) + (t2>0) + (t3>0);
  } while (--i!=0);

  bitCount[1]=extract_h(bc1_2);
  bitCount[2]=extract_l(bc1_2);
  bitCount[3]=extract_h(bc3_4) + sc;
  bitCount[4]=extract_l(bc3_4) + sc;
  bitCount[5]=extract_h(bc5_6);
  bitCount[6]=extract_l(bc5_6);
  bitCount[7]=extract_h(bc7_8) + sc;
  bitCount[8]=extract_l(bc7_8) + sc;
  bitCount[9]=extract_h(bc9_10) + sc;
  bitCount[10]=extract_l(bc9_10) + sc;
  bitCount[11]=bc11 + sc;
}


/*****************************************************************************
*
* function name: Write_LookUpTabHuf_1
* description:  counts tables 3-11
* returns:
* input:        quantized spectrum
* output:       bitCount for tables 3-11
*
*****************************************************************************/

static void Write_LookUpTabHuf_1(const int16_t *values,
                                     const int16_t  width,
                                     int16_t       *bitCount)
{
  int32_t t0,t1,t2,t3, i;
  int32_t bc3_4,bc5_6,bc7_8,bc9_10;
  int16_t bc11,sc;
  int16_t *values_local;
  values_local = (int16_t *)values;

  bc3_4=0;
  bc5_6=0;
  bc7_8=0;
  bc9_10=0;
  bc11=0;
  sc=0;

  i = width/4;
//  for(i=0;i<width;i+=4){
  do {

    t0= *values_local++;
    t1= *values_local++;
    t2= *values_local++;
    t3= *values_local++;

    /*
      5,6
    */
    bc5_6 = bc5_6 + EXPAND(huff_ltab5_6[t0+4][t1+4]);
    bc5_6 = bc5_6 + EXPAND(huff_ltab5_6[t2+4][t3+4]);

    t0=ABS(t0);
    t1=ABS(t1);
    t2=ABS(t2);
    t3=ABS(t3);


    bc3_4 = bc3_4 + EXPAND(huff_ltab3_4[t0][t1][t2][t3]);

    bc7_8 = bc7_8 + EXPAND(huff_ltab7_8[t0][t1]);
    bc7_8 = bc7_8 + EXPAND(huff_ltab7_8[t2][t3]);

    bc9_10 = bc9_10 + EXPAND(huff_ltab9_10[t0][t1]);
    bc9_10 = bc9_10 + EXPAND(huff_ltab9_10[t2][t3]);

    bc11 = bc11 + huff_ltab11[t0][t1];
    bc11 = bc11 + huff_ltab11[t2][t3];


    sc = sc + (t0>0) + (t1>0) + (t2>0) + (t3>0);
  } while (--i!=0);

  bitCount[1]=INVALID_BITCOUNT;
  bitCount[2]=INVALID_BITCOUNT;
  bitCount[3]=extract_h(bc3_4) + sc;
  bitCount[4]=extract_l(bc3_4) + sc;
  bitCount[5]=extract_h(bc5_6);
  bitCount[6]=extract_l(bc5_6);
  bitCount[7]=extract_h(bc7_8) + sc;
  bitCount[8]=extract_l(bc7_8) + sc;
  bitCount[9]=extract_h(bc9_10) + sc;
  bitCount[10]=extract_l(bc9_10) + sc;
  bitCount[11]=bc11 + sc;

}



/*****************************************************************************
*
* function name: Write_LookUpTabHuf_2
* description:  counts tables 5-11
* returns:
* input:        quantized spectrum
* output:       bitCount for tables 5-11
*
*****************************************************************************/
static void Write_LookUpTabHuf_2(const int16_t *values,
                                 const int16_t  width,
                                 int16_t       *bitCount)
{

  int32_t t0,t1,i;
  int32_t bc5_6,bc7_8,bc9_10;
  int16_t bc11,sc;

  bc5_6=0;
  bc7_8=0;
  bc9_10=0;
  bc11=0;
  sc=0;

  for(i=0;i<width;i+=2){

    t0 = values[i+0];
    t1 = values[i+1];

    bc5_6 = bc5_6 + EXPAND(huff_ltab5_6[t0+4][t1+4]);

    t0=ABS(t0);
    t1=ABS(t1);

    bc7_8 = bc7_8 + EXPAND(huff_ltab7_8[t0][t1]);
    bc9_10 = bc9_10 + EXPAND(huff_ltab9_10[t0][t1]);
    bc11 = bc11 + huff_ltab11[t0][t1];


    sc = sc + (t0>0) + (t1>0);
  }
  bitCount[1]=INVALID_BITCOUNT;
  bitCount[2]=INVALID_BITCOUNT;
  bitCount[3]=INVALID_BITCOUNT;
  bitCount[4]=INVALID_BITCOUNT;
  bitCount[5]=extract_h(bc5_6);
  bitCount[6]=extract_l(bc5_6);
  bitCount[7]=extract_h(bc7_8) + sc;
  bitCount[8]=extract_l(bc7_8) + sc;
  bitCount[9]=extract_h(bc9_10) + sc;
  bitCount[10]=extract_l(bc9_10) + sc;
  bitCount[11]=bc11 + sc;

}


/*****************************************************************************
*
* function name: Write_LookUpTabHuf_3
* description:  counts tables 7-11
* returns:
* input:        quantized spectrum
* output:       bitCount for tables 7-11
*
*****************************************************************************/

static void Write_LookUpTabHuf_3(const int16_t *values,
                             const int16_t  width,
                             int16_t       *bitCount)
{
  int32_t t0,t1, i;
  int32_t bc7_8,bc9_10;
  int16_t bc11,sc;

  bc7_8=0;
  bc9_10=0;
  bc11=0;
  sc=0;

  for(i=0;i<width;i+=2){

    t0=ABS(values[i+0]);
    t1=ABS(values[i+1]);

    bc7_8 = bc7_8 + EXPAND(huff_ltab7_8[t0][t1]);
    bc9_10 = bc9_10 + EXPAND(huff_ltab9_10[t0][t1]);
    bc11 = bc11 + huff_ltab11[t0][t1];


    sc = sc + (t0>0) + (t1>0);
  }
  bitCount[1]=INVALID_BITCOUNT;
  bitCount[2]=INVALID_BITCOUNT;
  bitCount[3]=INVALID_BITCOUNT;
  bitCount[4]=INVALID_BITCOUNT;
  bitCount[5]=INVALID_BITCOUNT;
  bitCount[6]=INVALID_BITCOUNT;
  bitCount[7]=extract_h(bc7_8) + sc;
  bitCount[8]=extract_l(bc7_8) + sc;
  bitCount[9]=extract_h(bc9_10) + sc;
  bitCount[10]=extract_l(bc9_10) + sc;
  bitCount[11]=bc11 + sc;

}

/*****************************************************************************
*
* function name: Write_LookUpTabHuf_4
* description:  counts tables 9-11
* returns:
* input:        quantized spectrum
* output:       bitCount for tables 9-11
*
*****************************************************************************/
static void Write_LookUpTabHuf_4(const int16_t *values,
                         const int16_t  width,
                         int16_t       *bitCount)
{

  int32_t t0,t1,i;
  int32_t bc9_10;
  int16_t bc11,sc;

  bc9_10=0;
  bc11=0;
  sc=0;

  for(i=0;i<width;i+=2){

    t0=ABS(values[i+0]);
    t1=ABS(values[i+1]);


    bc9_10 += EXPAND(huff_ltab9_10[t0][t1]);
    bc11 = bc11 + huff_ltab11[t0][t1];


    sc = sc + (t0>0) + (t1>0);
  }
  bitCount[1]=INVALID_BITCOUNT;
  bitCount[2]=INVALID_BITCOUNT;
  bitCount[3]=INVALID_BITCOUNT;
  bitCount[4]=INVALID_BITCOUNT;
  bitCount[5]=INVALID_BITCOUNT;
  bitCount[6]=INVALID_BITCOUNT;
  bitCount[7]=INVALID_BITCOUNT;
  bitCount[8]=INVALID_BITCOUNT;
  bitCount[9]=extract_h(bc9_10) + sc;
  bitCount[10]=extract_l(bc9_10) + sc;
  bitCount[11]=bc11 + sc;

}

/*****************************************************************************
*
* function name: Write_LookUpTabHuf_5
* description:  counts table 11
* returns:
* input:        quantized spectrum
* output:       bitCount for table 11
*
*****************************************************************************/
 static void Write_LookUpTabHuf_5(const int16_t *values,
                    const int16_t  width,
                    int16_t        *bitCount)
{
  int32_t t0,t1,i;
  int16_t bc11,sc;

  bc11=0;
  sc=0;
  for(i=0;i<width;i+=2){
    t0=ABS(values[i+0]);
    t1=ABS(values[i+1]);
    bc11 = bc11 + huff_ltab11[t0][t1];


    sc = sc + (t0>0) + (t1>0);
  }

  bitCount[1]=INVALID_BITCOUNT;
  bitCount[2]=INVALID_BITCOUNT;
  bitCount[3]=INVALID_BITCOUNT;
  bitCount[4]=INVALID_BITCOUNT;
  bitCount[5]=INVALID_BITCOUNT;
  bitCount[6]=INVALID_BITCOUNT;
  bitCount[7]=INVALID_BITCOUNT;
  bitCount[8]=INVALID_BITCOUNT;
  bitCount[9]=INVALID_BITCOUNT;
  bitCount[10]=INVALID_BITCOUNT;
  bitCount[11]=bc11 + sc;
}

/*****************************************************************************
*
* function name: Write_LookUpTabHuf_Esc
* description:  counts table 11 (with Esc)
* returns:
* input:        quantized spectrum
* output:       bitCount for tables 11 (with Esc)(Huffman )
*
*****************************************************************************/

static void Write_LookUpTabHuf_Esc(const int16_t *values,
                     const int16_t  width,
                     int16_t       *bitCount)
{
  int32_t t0,t1,t00,t01,i;
  int16_t bc11,ec,sc;

  bc11=0;
  sc=0;
  ec=0;
  for(i=0;i<width;i+=2){
    t0=ABS(values[i+0]);
    t1=ABS(values[i+1]);


    sc = sc + (t0>0) + (t1>0);

    t00 = min(t0,16);
    t01 = min(t1,16);
    bc11 = bc11 + huff_ltab11[t00][t01];


    if(t0 >= 16){
      ec = ec + 5;
      while(sub(t0=(t0 >> 1), 16) >= 0) {
        ec = ec + 2;
      }
    }


    if(t1 >= 16){
      ec = ec + 5;
      while(sub(t1=(t1 >> 1), 16) >= 0) {
        ec = ec + 2;
      }
    }
  }
  bitCount[1]=INVALID_BITCOUNT;
  bitCount[2]=INVALID_BITCOUNT;
  bitCount[3]=INVALID_BITCOUNT;
  bitCount[4]=INVALID_BITCOUNT;
  bitCount[5]=INVALID_BITCOUNT;
  bitCount[6]=INVALID_BITCOUNT;
  bitCount[7]=INVALID_BITCOUNT;
  bitCount[8]=INVALID_BITCOUNT;
  bitCount[9]=INVALID_BITCOUNT;
  bitCount[10]=INVALID_BITCOUNT;
  bitCount[11]=bc11 + sc + ec;
}


typedef void (*COUNT_FUNCTION)(const int16_t *values,
                               const int16_t  width,
                               int16_t       *bitCount);

//bitLookUp :
static COUNT_FUNCTION countFuncTable[CODE_BOOK_ESC_LAV+1] =
  {

    Write_LookUpTabHuf_0,     /* 0  */
    Write_LookUpTabHuf_0,     /* 1  */
    Write_LookUpTabHuf_1,     /* 2  */
    Write_LookUpTabHuf_2,     /* 3  */
    Write_LookUpTabHuf_2,     /* 4  */
    Write_LookUpTabHuf_3,     /* 5  */
    Write_LookUpTabHuf_3,     /* 6  */
    Write_LookUpTabHuf_3,     /* 7  */
    Write_LookUpTabHuf_4,     /* 8  */
    Write_LookUpTabHuf_4,     /* 9  */
    Write_LookUpTabHuf_4,     /* 10 */
    Write_LookUpTabHuf_4,     /* 11 */
    Write_LookUpTabHuf_4,     /* 12 */
    Write_LookUpTabHuf_5,     /* 13 */
    Write_LookUpTabHuf_5,     /* 14 */
    Write_LookUpTabHuf_5,     /* 15 */
    Write_LookUpTabHuf_Esc    /* 16 */
  };


/*****************************************************************************
*
* function name: Write_HuffBits
* description:  write huffum bits
*
*****************************************************************************/
int16_t Write_HuffBits(int16_t *values, int16_t width, int16_t codeBook, HANDLE_BIT_BUF hBitstream)
{

  int32_t i, t0, t1, t2, t3, t00, t01;
  uint16_t codeWord, codeLength;
  int16_t sign, signLength;


  switch (codeBook) {
    case CODE_BOOK_ZERO_NO:
      break;

    case CODE_BOOK_1_NO:
      for(i=0; i<width; i+=4) {
        t0         = values[i+0];
        t1         = values[i+1];
        t2         = values[i+2];
        t3         = values[i+3];
        codeWord   = huff_ctab1[t0+1][t1+1][t2+1][t3+1];
        codeLength = HI_LTAB(huff_ltab1_2[t0+1][t1+1][t2+1][t3+1]);
        Write_Bits2Buf(hBitstream, codeWord, codeLength);
      }
      break;

    case CODE_BOOK_2_NO:
      for(i=0; i<width; i+=4) {
        t0         = values[i+0];
        t1         = values[i+1];
        t2         = values[i+2];
        t3         = values[i+3];
        codeWord   = huff_ctab2[t0+1][t1+1][t2+1][t3+1];
        codeLength = LO_LTAB(huff_ltab1_2[t0+1][t1+1][t2+1][t3+1]);
        Write_Bits2Buf(hBitstream,codeWord,codeLength);
      }
      break;

    case CODE_BOOK_3_NO:
      for(i=0; i<width; i+=4) {
        sign=0;
        signLength=0;
        t0 = values[i+0];

        if(t0 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t0 < 0){
            sign|=1;
            t0=-t0;
          }
        }
        t1 = values[i+1];

        if(t1 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t1 < 0){
            sign|=1;
            t1=-t1;
          }
        }
        t2 = values[i+2];

        if(t2 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t2 < 0){
            sign|=1;
            t2=-t2;
          }
        }
        t3 = values[i+3];
        if(t3 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t3 < 0){
            sign|=1;
            t3=-t3;
          }
        }

        codeWord   = huff_ctab3[t0][t1][t2][t3];
        codeLength = HI_LTAB(huff_ltab3_4[t0][t1][t2][t3]);
        Write_Bits2Buf(hBitstream,codeWord,codeLength);
        Write_Bits2Buf(hBitstream,sign,signLength);
      }
      break;

    case CODE_BOOK_4_NO:
      for(i=0; i<width; i+=4) {
        sign=0;
        signLength=0;
        t0 = values[i+0];

        if(t0 != 0){
          signLength = signLength + 1;
          sign = sign << 1;
          if(t0 < 0){
            sign|=1;
            t0=-t0;
          }
        }
        t1 = values[i+1];

        if(t1 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t1 < 0){
            sign|=1;
            t1=-t1;
          }
        }
        t2 = values[i+2];

        if(t2 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t2 < 0){
            sign|=1;
            t2=-t2;
          }
        }
        t3 = values[i+3];

        if(t3 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t3 < 0){
            sign|=1;
            t3=-t3;
          }
        }
        codeWord   = huff_ctab4[t0][t1][t2][t3];
        codeLength = LO_LTAB(huff_ltab3_4[t0][t1][t2][t3]);
        Write_Bits2Buf(hBitstream,codeWord,codeLength);
        Write_Bits2Buf(hBitstream,sign,signLength);
      }
      break;

    case CODE_BOOK_5_NO:
      for(i=0; i<width; i+=2) {
        t0         = values[i+0];
        t1         = values[i+1];
        codeWord   = huff_ctab5[t0+4][t1+4];
        codeLength = HI_LTAB(huff_ltab5_6[t0+4][t1+4]);
        Write_Bits2Buf(hBitstream,codeWord,codeLength);
      }
      break;

    case CODE_BOOK_6_NO:
      for(i=0; i<width; i+=2) {
        t0         = values[i+0];
        t1         = values[i+1];
        codeWord   = huff_ctab6[t0+4][t1+4];
        codeLength = LO_LTAB(huff_ltab5_6[t0+4][t1+4]);
        Write_Bits2Buf(hBitstream,codeWord,codeLength);
      }
      break;

    case CODE_BOOK_7_NO:
      for(i=0; i<width; i+=2){
        sign=0;
        signLength=0;
        t0 = values[i+0];

        if(t0 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t0 < 0){
            sign|=1;
            t0=-t0;
          }
        }

        t1 = values[i+1];

        if(t1 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t1 < 0){
            sign|=1;
            t1=-t1;
          }
        }
        codeWord   = huff_ctab7[t0][t1];
        codeLength = HI_LTAB(huff_ltab7_8[t0][t1]);
        Write_Bits2Buf(hBitstream,codeWord,codeLength);
        Write_Bits2Buf(hBitstream,sign,signLength);
      }
      break;

    case CODE_BOOK_8_NO:
      for(i=0; i<width; i+=2) {
        sign=0;
        signLength=0;
        t0 = values[i+0];

        if(t0 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t0 < 0){
            sign|=1;
            t0=-t0;
          }
        }

        t1 = values[i+1];

        if(t1 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t1 < 0){
            sign|=1;
            t1=-t1;
          }
        }
        codeWord   = huff_ctab8[t0][t1];
        codeLength = LO_LTAB(huff_ltab7_8[t0][t1]);
        Write_Bits2Buf(hBitstream,codeWord,codeLength);
        Write_Bits2Buf(hBitstream,sign,signLength);
      }
      break;

    case CODE_BOOK_9_NO:
      for(i=0; i<width; i+=2) {
        sign=0;
        signLength=0;
        t0 = values[i+0];

        if(t0 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t0 < 0){
            sign|=1;
            t0=-t0;
          }
        }

        t1 = values[i+1];

        if(t1 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t1 < 0){
            sign|=1;
            t1=-t1;
          }
        }
        codeWord   = huff_ctab9[t0][t1];
        codeLength = HI_LTAB(huff_ltab9_10[t0][t1]);
        Write_Bits2Buf(hBitstream,codeWord,codeLength);
        Write_Bits2Buf(hBitstream,sign,signLength);
      }
      break;

    case CODE_BOOK_10_NO:
      for(i=0; i<width; i+=2) {
        sign=0;
        signLength=0;
        t0 = values[i+0];

        if(t0 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t0 < 0){
            sign|=1;
            t0=-t0;
          }
        }

        t1 = values[i+1];

        if(t1 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t1 < 0){
            sign|=1;
            t1=-t1;
          }
        }
        codeWord   = huff_ctab10[t0][t1];
        codeLength = LO_LTAB(huff_ltab9_10[t0][t1]);
        Write_Bits2Buf(hBitstream,codeWord,codeLength);
        Write_Bits2Buf(hBitstream,sign,signLength);
      }
      break;

    case CODE_BOOK_ESC_NO:
      for(i=0; i<width; i+=2) {
        sign=0;
        signLength=0;
        t0 = values[i+0];

        if(t0 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t0 < 0){
            sign|=1;
            t0=-t0;
          }
        }

        t1 = values[i+1];

        if(t1 != 0){
          signLength = signLength + 1;
          sign = sign << 1;

          if(t1 < 0){
            sign|=1;
            t1=-t1;
          }
        }
        t00 = min(t0,16);
        t01 = min(t1,16);

        codeWord   = huff_ctab11[t00][t01];
        codeLength = huff_ltab11[t00][t01];
        Write_Bits2Buf(hBitstream,codeWord,codeLength);
        Write_Bits2Buf(hBitstream,sign,signLength);

        if(t0 >= 16){
          int16_t n, p;
          n=0;
          p=t0;
          while(sub(p=(p >> 1), 16) >= 0){

            Write_Bits2Buf(hBitstream,1,1);
            n = n + 1;
          }
          Write_Bits2Buf(hBitstream,0,1);
          n = n + 4;
          Write_Bits2Buf(hBitstream,(t0 - (1 << n)),n);
        }

        if(t1 >= 16){
          int16_t n, p;
          n=0;
          p=t1;
          while(sub(p=(p >> 1), 16) >= 0){

            Write_Bits2Buf(hBitstream,1,1);
            n = n + 1;
          }
          Write_Bits2Buf(hBitstream,0,1);
          n = n + 4;
          Write_Bits2Buf(hBitstream,(t1 - (1 << n)),n);
        }
      }
      break;

    default:
      break;
  }
  return(0);
}

/*
   Get_HuffTableScf :bitCountScalefactorDelta
 */
/*
int16_t Get_HuffTableScf(int16_t delta)
{
  return(huff_ltabscf[delta+CODE_BOOK_SCF_LAV]);
}
*/
/*
*/
int16_t Write_HuffTabScfBitLen(int16_t delta, HANDLE_BIT_BUF hBitstream)
{
  int32_t codeWord;
  int16_t codeLength;


  if(delta > CODE_BOOK_SCF_LAV || delta < -CODE_BOOK_SCF_LAV)
    return(1);

  codeWord   = huff_ctabscf[delta + CODE_BOOK_SCF_LAV];
  codeLength = huff_ltabscf[delta + CODE_BOOK_SCF_LAV];
  Write_Bits2Buf(hBitstream,codeWord,codeLength);
  return(0);
}

/*****************************************************************************
*
* function name: Call_bitCntFunTab
* description:  count bits
*
*****************************************************************************/
int16_t Call_bitCntFunTab(const int16_t *values,
                const int16_t  width,
                int16_t        maxVal,
                int16_t       *bitCount)
{
  /*
    check if we can use codebook 0
  */

  if(maxVal == 0)
    bitCount[0] = 0;
  else
    bitCount[0] = INVALID_BITCOUNT;

  maxVal = min(maxVal, (int16_t)CODE_BOOK_ESC_LAV);
  countFuncTable[maxVal](values,width,bitCount);

  return(0);
}
