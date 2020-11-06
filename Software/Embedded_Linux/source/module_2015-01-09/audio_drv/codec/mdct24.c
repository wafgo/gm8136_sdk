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
	File:		MDCT.c

	Content:	MDCT Transform functionss

*******************************************************************************/

#include "basic_op.h"
#include "perceptual_const.h"
//#include "mdct.h"
#include "aac_table.h"
#include "fa410.h"

#define FA410

int *temp_buf_ptr = (int *)(IRAM_DATA_BASE+temp_bufOffset);

//extern int *CosSinTab1024_24bit_ptr;
//extern int *CosSinTab128_24bit_ptr;
extern int *twidTab512_24bit_ptr;
extern int *twidTab64_24bit_ptr;
extern int *LongWindowKBD_24bit_ptr;
extern int *ShortWindowSine_24bit_ptr;
extern int *CosSinTabExd1024_24bit_ptr;
extern int *CosSinTabExd128_24bit_ptr;
extern int *SQRT1_2_24bit_ptr;
extern void do_long_window(int16_t *mdctDelayBuffer, int32_t *realOut, int32_t *LongWindowKBDExd_24bit_ptr, int32_t minSf);
extern void do_long_window_minus(int16_t *mdctDelayBuffer, int32_t *realOut, int32_t *LongWindowKBDExd_24bit_ptr, int32_t minSf);
extern void do_start_short_minus(int16_t *mdctDelayBuffer, int32_t *realOut, int32_t *ShortWindowSineExd_24bit_ptr, int32_t minSf);
extern void do_stop_short(int16_t *mdctDelayBuffer, int32_t *realOut, int32_t *ShortWindowSineExd_24bit_ptr, int32_t minSf);
extern void do_short_win(int16_t *mdctDelayBuffer_p1, int32_t *realOut, int32_t *ShortWindowSineExd_24bit_ptr, int32_t minSf);
extern void MDCT_OneExd_24bit(int *buf, int num, int *CosSinTabExd128_24bit_ptr);
extern void Radix4_1_24bit(int *buf, int num);
extern void MDCT_SecondExd_24bit(int *buf, int num, int *CosSinTabExd128_24bit_ptr);
extern void Radix8FFTExd_24bit(int *buf, int num, int *temp_buf_ptr, int *SQRT1_2_24bit_ptr);
extern void Radix4FFTExd_Inner_24bit(int **xptr, int j, int fftstep, int **csptr);
//------ test code ----------------
int pre_blockType24 = LONG_WINDOW;
//------ end ----------------------

#define LS_TRANS ((FRAME_LEN_LONG-FRAME_LEN_SHORT)/2) /* 448 */
#define SQRT1_2 0x5a82799a	/* sqrt(1/2) in Q31 */

#define swap2(p0,p1) \
	t = p0; t1 = *(&(p0)+1);	\
	p0 = p1; *(&(p0)+1) = *(&(p1)+1);	\
	p1 = t; *(&(p1)+1) = t1

//#define not_fa4_asm
extern int *LongWindowKBDExd_24bit_ptr;
extern int *ShortWindowSineExd_24bit_ptr;

//extern void Radix4_1(int *buf, int num);
//extern void MDCT_ShifDelayBuf(int16_t *mdctDelayBuffer, /*! start of mdct delay buffer */
//								 int16_t *timeSignal,      /*! pointer to new time signal samples, interleaved */
//								 int16_t chIncrement       /*! number of channels */
//								 );
extern int16_t MDCT_CalcScalefactor(const int16_t *vector, /*!< Pointer to input vector */
								 int16_t len,           /*!< Length of input vector */
								 int16_t stride);
/*********************************************************************************
*
* function name: ShuffleSwap
* description:  ShuffleSwap points prepared function for fft
*
**********************************************************************************/
void ShuffleSwapExd(int *buf, int num, const unsigned char* bitTab)
{
    int *part0, *part1;
	int i, j;
	int t, t1;

	part0 = buf;
    part1 = buf + num;

	while ((i = *bitTab++) != 0) {
        j = *bitTab++;

        swap2(part0[4*i+0], part0[4*j+0]);
        swap2(part0[4*i+2], part1[4*j+0]);
        swap2(part1[4*i+0], part0[4*j+2]);
        swap2(part1[4*i+2], part1[4*j+2]);
    }

    do {
        swap2(part0[4*i+2], part1[4*i+0]);
    } while ((i = *bitTab++) != 0);
}


static void Radix4FFTExd_24bit(int *buf, int num, int bgn, int *twidTab)
{
	//int r0, r1, r2, r3;
	//int r4, r5, r6, r7;
	//int t0, t1;
	//int sinx, cosx;
	int i, j, fftstep;
	int *xptr, *csptr;
	//int acc0, acc1, acc2;

	for (num >>= 2; num != 0; num >>= 2) //bgn:=4;8 ; 4;16
	{
		fftstep = 2*bgn;
		xptr = buf;
		i = num;
//    	for (i = num; i != 0; i--)
		do {                              //16;64
			csptr = twidTab;
			j = bgn;
//			for (j = bgn; j != 0; j--)
		        #if 0
			do {
				if((i==1)&&(j==1))
				     while(0);
				r0 = xptr[0];
				r1 = xptr[1];
				xptr += fftstep;

				t0 = xptr[0];
				t1 = xptr[1];
				cosx = csptr[0];
				sinx = csptr[1];
				r2 = MUL24_HIGH(cosx, t0) + MUL24_HIGH(sinx, t1);		/* cos*br + sin*bi */
				r3 = MUL24_HIGH(cosx, t1) - MUL24_HIGH(sinx, t0);		/* cos*bi - sin*br */
				xptr += fftstep;

				t0 = r0 >> 2;
				t1 = r1 >> 2;
				r0 = t0 - r2;
				r1 = t1 - r3;
				r2 = t0 + r2;
				r3 = t1 + r3;

				t0 = xptr[0];
				t1 = xptr[1];
				cosx = csptr[2];
				sinx = csptr[3];
				r4 = MUL24_HIGH(cosx, t0) + MUL24_HIGH(sinx, t1);		/* cos*cr + sin*ci */
				r5 = MUL24_HIGH(cosx, t1) - MUL24_HIGH(sinx, t0);		/* cos*ci - sin*cr */
				xptr += fftstep;

				t0 = xptr[0];
				t1 = xptr[1];
				cosx = csptr[4];
				sinx = csptr[5];
				r6 = MUL24_HIGH(cosx, t0) + MUL24_HIGH(sinx, t1);		/* cos*cr + sin*ci */
				r7 = MUL24_HIGH(cosx, t1) - MUL24_HIGH(sinx, t0);		/* cos*ci - sin*cr */
				csptr += 6;

				t0 = r4;
				t1 = r5;
				r4 = t0 + r6;
				r5 = r7 - t1;
				r6 = t0 - r6;
				r7 = r7 + t1;

				xptr[0] = r0 + r5;
				xptr[1] = r1 + r6;
				xptr -= fftstep;

				xptr[0] = r2 - r4;
				xptr[1] = r3 - r7;
				xptr -= fftstep;

				xptr[0] = r0 - r5;
				xptr[1] = r1 - r6;
				xptr -= fftstep;

				xptr[0] = r2 + r4;
				xptr[1] = r3 + r7;
				xptr += 2;

			} while (--j!=0);
		        #else
		        Radix4FFTExd_Inner_24bit(&xptr, j, fftstep, &csptr);
		        #endif
			xptr += 3*fftstep;
		} while (--i!=0);
		twidTab += 3*fftstep;
		bgn <<= 2;
	}
}


/**********************************************************************************
*
* function name: MDCT_BlkLong
* description:  the long block mdct, include long_start block, end_long block
*
**********************************************************************************/
void MDCT_BlkLongExd_24bit(int *buf)
{
	//MDCT_One(buf, 1024, cossintab + 128);

	MDCT_OneExd_24bit(buf, 1024, CosSinTabExd1024_24bit_ptr);

	//ShuffleSwap(buf, 512, bitrevTab + 17);
	ShuffleSwapExd(buf, 512, bitrevTab_129);
	Radix8FFTExd_24bit(buf, 512 >> 3, temp_buf_ptr, SQRT1_2_24bit_ptr);
	Radix4FFTExd_24bit(buf, 512 >> 3, 8, twidTab512_24bit_ptr);

	//MDCT_Second(buf, 1024, cossintab + 128);
	MDCT_SecondExd_24bit(buf, 1024, CosSinTabExd1024_24bit_ptr);
}

/**********************************************************************************
*
* function name: MDCT_BlkShort
* description:  the short block mdct
*
**********************************************************************************/
void MDCT_BlkShortExd_24bit(int *buf)
{
	//MDCT_One(buf, 128, cossintab);
	MDCT_OneExd_24bit(buf, 128, CosSinTabExd128_24bit_ptr);

	//ShuffleSwap(buf, 64, bitrevTab);
	ShuffleSwapExd(buf, 64, bitrevTab_17);
	Radix4_1_24bit(buf, 64 >> 2);
	Radix4FFTExd_24bit(buf, 64 >> 2, 4, twidTab64_24bit_ptr);

	//MDCT_Second(buf, 128, cossintab);
	MDCT_SecondExd_24bit(buf, 128, CosSinTabExd128_24bit_ptr);
}

/*****************************************************************************
*
* function name: MDCT_ShifDelayBuf
* description:    the mdct delay buffer has a size of 1600,
*  so the calculation of LONG,STOP must be  spilt in two
*  passes with 1024 samples and a mid shift,
*  the SHORT transforms can be completed in the delay buffer,
*  and afterwards a shift
*
* 16bits
**********************************************************************************/
void MDCT_ShifDelayBuf(int16_t *mdctDelayBuffer, /*! start of mdct delay buffer */
								 int16_t *timeSignal,      /*! pointer to new time signal samples, interleaved */
								 int16_t chIncrement       /*! number of channels */
								 )
{
	int32_t i;
	int16_t *srBuf = mdctDelayBuffer;
	int16_t *dsBuf = mdctDelayBuffer+FRAME_LEN_LONG;

	for(i = 0; i < BLOCK_SWITCHING_OFFSET-FRAME_LEN_LONG; i+= 8)
	{
		*srBuf++ = *dsBuf++;	 *srBuf++ = *dsBuf++;
		*srBuf++ = *dsBuf++;	 *srBuf++ = *dsBuf++;
		*srBuf++ = *dsBuf++;	 *srBuf++ = *dsBuf++;
		*srBuf++ = *dsBuf++;	 *srBuf++ = *dsBuf++;
	}

	srBuf = mdctDelayBuffer + BLOCK_SWITCHING_OFFSET-FRAME_LEN_LONG;
	dsBuf = timeSignal;

	for(i=0; i<FRAME_LEN_LONG; i+=8)
	{
		*srBuf++ = *dsBuf; dsBuf += chIncrement;
		*srBuf++ = *dsBuf; dsBuf += chIncrement;
		*srBuf++ = *dsBuf; dsBuf += chIncrement;
		*srBuf++ = *dsBuf; dsBuf += chIncrement;
		*srBuf++ = *dsBuf; dsBuf += chIncrement;
		*srBuf++ = *dsBuf; dsBuf += chIncrement;
		*srBuf++ = *dsBuf; dsBuf += chIncrement;
		*srBuf++ = *dsBuf; dsBuf += chIncrement;
	}
}



extern int init_FA410_reg(void);
void MDCT_Exd_Trans_24bitmode(int16_t *mdctDelayBuffer,
                    int16_t *timeSignal,
                    int16_t chIncrement,
                    int32_t *realOut,
                    int16_t *mdctScale,
                    int16_t blockType
                    )
{
	int32_t i, w;
	//int32_t timeSignalSample;
	//int32_t ws1,ws2;
	int16_t *dctIn1;//*dctIn0,
	int32_t *outData0;//, *outData1;
	//int32_t *winPtr;

	int32_t delayBufferSf,timeSignalSf,minSf;
	//int32_t headRoom=0;
	int16_t *mdctDelayBuffer_p1;

  	//int ii;
  	//int *word_ptr;
  	//int *mid_winPtr;

  	init_FA410_reg();

  	//------ test code ----------------
/*
  	if (pre_blockType24 == LONG_WINDOW)
  		blockType = START_WINDOW;
  	else if (pre_blockType24 == START_WINDOW)
  		blockType = SHORT_WINDOW;
  	else if (pre_blockType24 == SHORT_WINDOW)
  		blockType = STOP_WINDOW;
  	else if (pre_blockType24 == STOP_WINDOW)
  		blockType = LONG_WINDOW;
  	pre_blockType24 = blockType;
*/
  	//----------------------------------

	switch(blockType){

	case LONG_WINDOW:
		/*
		we access BLOCK_SWITCHING_OFFSET (1600 ) delay buffer samples + 448 new timeSignal samples
		and get the biggest scale factor for next calculate more precise
		*/
		                //16 bits
		delayBufferSf = MDCT_CalcScalefactor(mdctDelayBuffer,BLOCK_SWITCHING_OFFSET,1);
		timeSignalSf  = MDCT_CalcScalefactor(timeSignal,2*FRAME_LEN_LONG-BLOCK_SWITCHING_OFFSET,chIncrement);
		minSf = min(delayBufferSf,timeSignalSf);
		minSf = min(minSf,14);
	#if 0
		dctIn0 = mdctDelayBuffer;
		dctIn1 = mdctDelayBuffer + FRAME_LEN_LONG - 1; //16bits
		outData0 = realOut + FRAME_LEN_LONG/2;  //16bits
	#endif
		/* add windows and pre add for mdct to last buffer*/

#if 0
		word_ptr =   LongWindowKBD_24bit_ptr; //32bit table
		mid_winPtr = LongWindowKBD_24bit_ptr+512;
//		for(i=0;i<FRAME_LEN_LONG/2;i++){
		i = FRAME_LEN_LONG/2;
		do {
			timeSignalSample = (*dctIn0++) << minSf;
			ws1 = (timeSignalSample * (*word_ptr))>>10;
			timeSignalSample = (*dctIn1--) << minSf;
			ws2 = (timeSignalSample * (*mid_winPtr))>>10;
			word_ptr ++;
			mid_winPtr++;
			/* shift 2 to avoid overflow next *///Q30
			*outData0++ = ws1 - ws2;
		} while (--i!=0);

#else
	#if 0
		word_ptr =   LongWindowKBDExd_24bit_ptr; //32bit table
		mid_winPtr = LongWindowKBDExd_24bit_ptr+512;
	#endif
//		for(i=0;i<FRAME_LEN_LONG/2;i++){
        #if 0
		i = FRAME_LEN_LONG/2;
		do {
			timeSignalSample = (*dctIn0++) << (minSf+8);
			ws1 = MUL24_HIGH(timeSignalSample,*word_ptr);
			timeSignalSample = (*dctIn1--) << (minSf+8);
			ws2 = MUL24_HIGH(timeSignalSample,*mid_winPtr);	//Q2.22
			word_ptr ++;
			mid_winPtr++;
			/* shift 2 to avoid overflow next *///Q30
			*outData0++ = ws1 - ws2;
		} while (--i!=0);
	#endif

	do_long_window(mdctDelayBuffer, realOut, LongWindowKBDExd_24bit_ptr, minSf+8);

#endif

        //16 bits
		MDCT_ShifDelayBuf(mdctDelayBuffer,timeSignal,chIncrement);

		/* add windows and pre add for mdct to new buffer*/
  #if 0
		dctIn0 = mdctDelayBuffer;
		dctIn1 = mdctDelayBuffer + FRAME_LEN_LONG - 1;
		outData0 = realOut + FRAME_LEN_LONG/2 - 1;
  #endif
#if 0
		word_ptr =   LongWindowKBD_24bit_ptr; //32bit table
		mid_winPtr = LongWindowKBD_24bit_ptr+512;
		i = FRAME_LEN_LONG/2;
//		for(i=0;i<FRAME_LEN_LONG/2;i++){
		do {
			timeSignalSample = (*dctIn0++) << minSf;
			ws1 = (timeSignalSample * (*mid_winPtr))>>10;
			timeSignalSample = (*dctIn1--) << minSf;
			ws2 = (timeSignalSample * (*word_ptr))>>10;
			word_ptr++;
			mid_winPtr++;
			/* shift 2 to avoid overflow next */
			*outData0-- = -(ws1 + ws2);

		} while (--i!=0);

#else
  #if 0
		word_ptr =   LongWindowKBDExd_24bit_ptr; //32bit table
		mid_winPtr = LongWindowKBDExd_24bit_ptr+512;
//		for(i=0;i<FRAME_LEN_LONG/2;i++){
		i = FRAME_LEN_LONG/2;
		do {
			timeSignalSample = (*dctIn0++) << (minSf+8);
			ws1 = MUL24_HIGH(timeSignalSample,*mid_winPtr);
			timeSignalSample = (*dctIn1--) << (minSf+8);
			ws2 = MUL24_HIGH(timeSignalSample,*word_ptr);
			word_ptr ++;
			mid_winPtr++;
			/* shift 2 to avoid overflow next *///Q30
			*outData0-- = -(ws1 + ws2);

		} while (--i!=0);
  #endif
                do_long_window_minus(mdctDelayBuffer, realOut, LongWindowKBDExd_24bit_ptr, minSf+8);
#endif
	        MDCT_BlkLongExd_24bit(realOut);

		/* update scale factor */
		minSf = 14 - minSf;
		*mdctScale=minSf;
		break;

	case START_WINDOW:
		/*
		we access BLOCK_SWITCHING_OFFSET (1600 ) delay buffer samples + no timeSignal samples
		and get the biggest scale factor for next calculate more precise
		*/

		minSf = MDCT_CalcScalefactor(mdctDelayBuffer,BLOCK_SWITCHING_OFFSET,1);

		minSf = min(minSf,14);
  #if 0
		dctIn0 = mdctDelayBuffer;
		dctIn1 = mdctDelayBuffer + FRAME_LEN_LONG - 1;
		outData0 = realOut + FRAME_LEN_LONG/2;
  #endif
#if 0

		word_ptr =  LongWindowKBD_24bit_ptr;
		mid_winPtr =LongWindowKBD_24bit_ptr+512;
		/* add windows and pre add for mdct to last buffer*/
//		for(i=0;i<FRAME_LEN_LONG/2;i++){
		i = FRAME_LEN_LONG/2;
		do {
			timeSignalSample = (*dctIn0++) << minSf;
			ws1 = (timeSignalSample * (*word_ptr))>>10;
			timeSignalSample = (*dctIn1--) << minSf;
			ws2 = (timeSignalSample * (*mid_winPtr))>>10;
			*outData0++ = (ws1 - ws2);  /* shift 2 to avoid overflow next */
			word_ptr++;
			mid_winPtr++;
		} while (--i!=0);
#else
  #if 0
		word_ptr =  LongWindowKBDExd_24bit_ptr;
		mid_winPtr =LongWindowKBDExd_24bit_ptr+512;
		/* add windows and pre add for mdct to last buffer*/
//		for(i=0;i<FRAME_LEN_LONG/2;i++){
		i = FRAME_LEN_LONG/2;
		do {
			timeSignalSample = (*dctIn0++) << (minSf+8);
			ws1 = MUL24_HIGH(timeSignalSample,*word_ptr);
			timeSignalSample = (*dctIn1--) << (minSf+8);
			ws2 = MUL24_HIGH(timeSignalSample,*mid_winPtr);
			*outData0++ = (ws1 - ws2);  /* shift 2 to avoid overflow next */
			word_ptr++;
			mid_winPtr++;
		} while (--i!=0);
  #endif
  		do_long_window(mdctDelayBuffer, realOut, LongWindowKBDExd_24bit_ptr, minSf+8);
#endif

		MDCT_ShifDelayBuf(mdctDelayBuffer,timeSignal,chIncrement);

		outData0 = realOut + FRAME_LEN_LONG/2 - 1;
		for(i=0;i<LS_TRANS;i++){
			*outData0-- = -mdctDelayBuffer[i] << (15 - 2 + minSf - 6);
		}

		/* add windows and pre add for mdct to new buffer*/
 #if 0
		dctIn0 = mdctDelayBuffer + LS_TRANS;
		dctIn1 = mdctDelayBuffer + FRAME_LEN_LONG - 1 - LS_TRANS;
		outData0 = realOut + FRAME_LEN_LONG/2 - 1 -LS_TRANS;	//(512- 448 - 1)*4 =
 #endif
#if 0
		word_ptr = 	ShortWindowSine_24bit_ptr;
		mid_winPtr =	ShortWindowSine_24bit_ptr+128;
//		for(i=0;i<FRAME_LEN_SHORT/2;i++){
		i = FRAME_LEN_SHORT/2;
		do {
			timeSignalSample= (*dctIn0++) << minSf;
			ws1 = (timeSignalSample * (*mid_winPtr))>>10;
			timeSignalSample= (*dctIn1--) << minSf;
			ws2 = (timeSignalSample * (*word_ptr))>>10;
			mid_winPtr++;
			word_ptr++;
			*outData0-- =  -(ws1 + ws2);  /* shift 2 to avoid overflow next */
		} while (--i!=0);
#else
	#if 0
		word_ptr = 	ShortWindowSineExd_24bit_ptr;
		mid_winPtr =	ShortWindowSineExd_24bit_ptr+128;
//		for(i=0;i<FRAME_LEN_SHORT/2;i++){
		i = FRAME_LEN_SHORT/2;
		do {
			timeSignalSample= (*dctIn0++) << (minSf+8);
			ws1 = MUL24_HIGH(timeSignalSample,*mid_winPtr);
			timeSignalSample= (*dctIn1--) << (minSf+8);
			ws2 = MUL24_HIGH(timeSignalSample,*word_ptr);
			mid_winPtr++;
			word_ptr++;
			*outData0-- =  -(ws1 + ws2);  /* shift 2 to avoid overflow next */
		} while (--i!=0);
	#endif
		do_start_short_minus(mdctDelayBuffer, realOut, ShortWindowSineExd_24bit_ptr, minSf+8);
#endif
	 	MDCT_BlkLongExd_24bit(realOut);

		/* update scale factor */
		minSf = 14 - minSf;
		*mdctScale= minSf;
		break;

	case STOP_WINDOW:
		/*
		we access BLOCK_SWITCHING_OFFSET-LS_TRANS (1600-448 ) delay buffer samples + 448 new timeSignal samples
		and get the biggest scale factor for next calculate more precise
		*/
		delayBufferSf = MDCT_CalcScalefactor(mdctDelayBuffer+LS_TRANS,BLOCK_SWITCHING_OFFSET-LS_TRANS,1);
		timeSignalSf  = MDCT_CalcScalefactor(timeSignal,2*FRAME_LEN_LONG-BLOCK_SWITCHING_OFFSET,chIncrement);
		minSf = min(delayBufferSf,timeSignalSf);
		minSf = min(minSf,13);

		outData0 = realOut + FRAME_LEN_LONG/2;
		dctIn1 = mdctDelayBuffer + FRAME_LEN_LONG - 1;
//		for(i=0;i<LS_TRANS;i++){
		i = LS_TRANS;
		do {
			*outData0++ = -(*dctIn1--) << (15 - 2 + minSf - 6);
		} while (--i!=0);

		/* add windows and pre add for mdct to last buffer*/
  #if 0
		dctIn0 = mdctDelayBuffer + LS_TRANS;
		dctIn1 = mdctDelayBuffer + FRAME_LEN_LONG - 1 - LS_TRANS;
		outData0 = realOut + FRAME_LEN_LONG/2 + LS_TRANS;
  #endif
#if 0

		word_ptr = 	ShortWindowSine_24bit_ptr;
		mid_winPtr =	ShortWindowSine_24bit_ptr+128;
//		for(i=0;i<FRAME_LEN_SHORT/2;i++){
		i = FRAME_LEN_SHORT/2;
		do {
			timeSignalSample = (*dctIn0++) << minSf;
			ws1 = (timeSignalSample * (*word_ptr))>>10;
			timeSignalSample= (*dctIn1--) << minSf;
			ws2 = (timeSignalSample * (*mid_winPtr))>>10;
			word_ptr++;
			mid_winPtr++;
			*outData0++ = ws1 - ws2;  /* shift 2 to avoid overflow next */
		} while (--i!=0);
#else
  #if 0
		word_ptr = 	ShortWindowSineExd_24bit_ptr;
		mid_winPtr =	ShortWindowSineExd_24bit_ptr+128;
//		for(i=0;i<FRAME_LEN_SHORT/2;i++){
		i = FRAME_LEN_SHORT/2;
		do {
			timeSignalSample = (*dctIn0++) << (minSf+8);
			ws1 = MUL24_HIGH(timeSignalSample,*word_ptr);
			timeSignalSample=  (*dctIn1--) << (minSf+8);
			ws2 = MUL24_HIGH(timeSignalSample,*mid_winPtr);
			word_ptr++;
			mid_winPtr++;
			*outData0++ = ws1 - ws2;  /* shift 2 to avoid overflow next */
		} while (--i!=0);
  #endif
  		do_stop_short(mdctDelayBuffer, realOut, ShortWindowSineExd_24bit_ptr, minSf+8);
#endif
		MDCT_ShifDelayBuf(mdctDelayBuffer,timeSignal,chIncrement);

		/* add windows and pre add for mdct to new buffer*/
  #if 0
		dctIn0 = mdctDelayBuffer;
		dctIn1 = mdctDelayBuffer + FRAME_LEN_LONG - 1;
		outData0 = realOut + FRAME_LEN_LONG/2 - 1;
  #endif
#if 0

		word_ptr =  LongWindowKBD_24bit_ptr;
		mid_winPtr =LongWindowKBD_24bit_ptr+512;
//		for(i=0;i<FRAME_LEN_LONG/2;i++){
		i = FRAME_LEN_LONG/2;
		do {
			timeSignalSample= (*dctIn0++) << minSf;
			ws1 = (timeSignalSample *(*mid_winPtr))>>10;
			timeSignalSample= (*dctIn1--) << minSf;
			ws2 = (timeSignalSample * (*word_ptr))>>10;
			*outData0-- =  -(ws1 + ws2);  /* shift 2 to avoid overflow next */
			word_ptr++;
			mid_winPtr++;
		} while (--i!=0);
#else
  #if 0
		word_ptr =  LongWindowKBDExd_24bit_ptr;
		mid_winPtr =LongWindowKBDExd_24bit_ptr+512;
//		for(i=0;i<FRAME_LEN_LONG/2;i++){
		i = FRAME_LEN_LONG/2;
		do {
			timeSignalSample= (*dctIn0++) << (minSf+8);
			ws1 = MUL24_HIGH(timeSignalSample,*mid_winPtr);
			timeSignalSample= (*dctIn1--) << (minSf+8);
			ws2 = MUL24_HIGH(timeSignalSample,*word_ptr);
			*outData0-- =  -(ws1 + ws2);  /* shift 2 to avoid overflow next */
			word_ptr++;
			mid_winPtr++;
		} while (--i!=0);
  #endif
		do_long_window_minus(mdctDelayBuffer, realOut, LongWindowKBDExd_24bit_ptr, minSf+8);
#endif
		MDCT_BlkLongExd_24bit(realOut);

		minSf = 14 - minSf;
		*mdctScale= minSf; /* update scale factor */
		break;

	case SHORT_WINDOW:
		/*
		we access BLOCK_SWITCHING_OFFSET (1600 ) delay buffer samples + no new timeSignal samples
		and get the biggest scale factor for next calculate more precise
		*/
		minSf = MDCT_CalcScalefactor(mdctDelayBuffer+TRANSFORM_OFFSET_SHORT,9*FRAME_LEN_SHORT,1);

		minSf = min(minSf,10);

		mdctDelayBuffer_p1 = mdctDelayBuffer + TRANSFORM_OFFSET_SHORT - FRAME_LEN_SHORT;
//		for(w=0;w<TRANS_FAC;w++){
		w = TRANS_FAC;
		do {
			mdctDelayBuffer_p1 += FRAME_LEN_SHORT;
  #if 0
			dctIn0 = mdctDelayBuffer_p1;
			dctIn1 = mdctDelayBuffer_p1 + FRAME_LEN_SHORT-1;
//			dctIn0 = mdctDelayBuffer+w*FRAME_LEN_SHORT+TRANSFORM_OFFSET_SHORT;
//			dctIn1 = mdctDelayBuffer+w*FRAME_LEN_SHORT+TRANSFORM_OFFSET_SHORT + FRAME_LEN_SHORT-1;
			outData0 = realOut + FRAME_LEN_SHORT/2;
			outData1 = realOut + FRAME_LEN_SHORT/2 - 1;
  #endif
#if 0

			word_ptr = 	ShortWindowSine_24bit_ptr;
			mid_winPtr =	ShortWindowSine_24bit_ptr+128;

//			for(i=0;i<FRAME_LEN_SHORT/2;i++){
			i = FRAME_LEN_SHORT/2;
			do {
				timeSignalSample= *dctIn0 << minSf;
				ws1 = (timeSignalSample * (*word_ptr))>>10;
				timeSignalSample= *dctIn1 << minSf;
				ws2 = (timeSignalSample * (*mid_winPtr))>>10;
				*outData0++ = ws1 - ws2;  /* shift 2 to avoid overflow next */

				timeSignalSample= *(dctIn0 + FRAME_LEN_SHORT) << minSf;
				ws1 = (timeSignalSample * (*mid_winPtr))>>10;
				timeSignalSample= *(dctIn1 + FRAME_LEN_SHORT) << minSf;
				ws2 = (timeSignalSample * (*word_ptr))>>10;
				*outData1-- =  -(ws1 + ws2);  /* shift 2 to avoid overflow next */

				word_ptr++;
				mid_winPtr++;
				dctIn0++;
				dctIn1--;
			} while (--i!=0);
#else
  #if 0
			word_ptr = 	ShortWindowSineExd_24bit_ptr;
			mid_winPtr =	ShortWindowSineExd_24bit_ptr+128;

//			for(i=0;i<FRAME_LEN_SHORT/2;i++){
			i = FRAME_LEN_SHORT/2;
			do {
				timeSignalSample= *dctIn0 << (minSf+8);
				ws1 = MUL24_HIGH(timeSignalSample,*word_ptr);
				timeSignalSample= *dctIn1 << (minSf+8);
				ws2 = MUL24_HIGH(timeSignalSample,*mid_winPtr);
				*outData0++ = ws1 - ws2;  /* shift 2 to avoid overflow next */

				timeSignalSample= *(dctIn0 + FRAME_LEN_SHORT) << (minSf+8);
				ws1 = MUL24_HIGH(timeSignalSample,*mid_winPtr);
				timeSignalSample= *(dctIn1 + FRAME_LEN_SHORT) << (minSf+8);
				ws2 = MUL24_HIGH(timeSignalSample,*word_ptr);
				*outData1-- =  -(ws1 + ws2);  /* shift 2 to avoid overflow next */

				word_ptr++;
				mid_winPtr++;
				dctIn0++;
				dctIn1--;
			} while (--i!=0);
  #endif
  			do_short_win(mdctDelayBuffer_p1, realOut, ShortWindowSineExd_24bit_ptr, minSf+8);
#endif
			MDCT_BlkShortExd_24bit(realOut);

			realOut += FRAME_LEN_SHORT;
		} while (--w!=0);

		minSf = 11 - minSf;
		*mdctScale = minSf; /* update scale factor */

		MDCT_ShifDelayBuf(mdctDelayBuffer,timeSignal,chIncrement);
		break;
  }

}
