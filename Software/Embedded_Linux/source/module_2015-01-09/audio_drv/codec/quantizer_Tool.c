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
	File:		quantizer_Tool.c

	Content:	quantization functions

*******************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "oper_32b.h"
#include "quantizer_tool.h"
#include "aac_table.h"

#define MANT_DIGITS 9
#define MANT_SIZE   (1<<MANT_DIGITS)

static const int32_t XROUND = 0x33e425af; /* final rounding constant (-0.0946f+ 0.5f) */


/*****************************************************************************
*
* function name:pow34
* description: calculate $x^{\frac{3}{4}}, for 0.5 < x < 1.0$.
*
*****************************************************************************/
__inline int32_t pow34(int32_t x)
{
  /* index table using MANT_DIGITS bits, but mask out the sign bit and the MSB
     which is always one */
  return mTab_3_4[(x >> (INT_BITS-2-MANT_DIGITS)) & (MANT_SIZE-1)];
}


/*****************************************************************************
*
* function name:SingLineQuant
* description: quantizes spectrum
*              quaSpectrum = mdctSpectrum^3/4*2^(-(3/16)*gain)
*
*****************************************************************************/
static int16_t SingLineQuant(const int16_t gain, const int32_t absSpectrum)
{
  int32_t e, minusFinalExp, finalShift;
  int32_t x;
  int16_t qua = 0;


  if (absSpectrum) {
    e = norm_l(absSpectrum);
    x = pow34(absSpectrum << e);

    /* calculate the final fractional exponent times 16 (was 3*(4*e + gain) + (INT_BITS-1)*16) */
    minusFinalExp = (e << 2) + gain;
    minusFinalExp = (minusFinalExp << 1) + minusFinalExp;
    minusFinalExp = minusFinalExp + ((INT_BITS-1) << 4);

    /* separate the exponent into a shift, and a multiply */
    finalShift = minusFinalExp >> 4;

    if (finalShift < INT_BITS) {
      x = L_mpy_wx(x, pow2tominusNover16[minusFinalExp & 15]);

      x += XROUND >> (INT_BITS - finalShift);

      /* shift and quantize */
#ifdef LINUX
			asm volatile("subs %[o1], %[o1], #1" : [o1] "+r" (finalShift));
			asm volatile("rsblt %[o1], %[o1], #0" : [o1] "+r" (finalShift));
			asm volatile("movlt %[o1], %[o1], lsl %[i1]" : [o1] "+r" (x) : [i1] "r" (finalShift));
			asm volatile("movge %[o1], %[o1], asr %[i1]" : [o1] "+r" (x) : [i1] "r" (finalShift));
#else
      __asm {
      	SUBS		finalShift, finalShift, #1
      	RSBLT		finalShift, finalShift, #0
      	MOVLT		x, x, lsl finalShift
      	MOVGE		x, x, asr finalShift
      }
#endif
//	  finalShift--;
//
//	  if(finalShift >= 0)
//		  x >>= finalShift;
//	  else
//		  x <<= (-finalShift);

	  qua = saturate(x);
    }
  }

  return qua;
}

/*****************************************************************************
*
* function name:SpectLinesQuant
* description: quantizes spectrum lines
*              quaSpectrum = mdctSpectrum^3/4*2^(-(3/16)*gain)
*  input: global gain, number of lines to process, spectral data
*  output: quantized spectrum
*
*****************************************************************************/
static void SpectLinesQuant(const int16_t gain,
                          const int16_t noOfLines,
                          const int32_t *mdctSpectrum,
                          int16_t *quaSpectrum)
{
  int32_t line;
  int32_t m = gain&3;
  int32_t g = (gain >> 2) + 4;
  int32_t mdctSpeL;
  int16_t pquat0, pquat1, pquat2, pquat3;
  const int16_t *pquat;
    /* gain&3 */

  pquat = quantBorders[m];
  pquat0 = pquat[0];
  pquat1 = pquat[1];
  pquat2 = pquat[2];
  pquat3 = pquat[3];
  g += 16;
  if(g >= 0)
  {
	for (line=0; line<noOfLines; line++) {
	  int32_t qua;
	  qua = 0;
	  mdctSpeL = mdctSpectrum[line];
	  if (mdctSpeL) {
		int32_t sa;
		int32_t saShft;
        sa = my_abs(mdctSpeL);
        //saShft = L_shr(sa, 16 + g);
	    saShft = sa >> g;
        if (saShft > pquat0) {
          if (saShft < pquat1) {
            qua = mdctSpeL>0 ? 1 : -1;
		  }
          else {
            if (saShft < pquat2) {
              qua = mdctSpeL>0 ? 2 : -2;
			}
            else {
              if (saShft < pquat3) {
                qua = mdctSpeL>0 ? 3 : -3;
			  }
              else {
                qua = SingLineQuant(gain, sa);
                /* adjust the sign. Since 0 < qua < 1, this cannot overflow. */
                if (mdctSpeL < 0)
                  qua = -qua;
			  }
			}
		  }
		}
	  }
      quaSpectrum[line] = qua ;
	}
  } else {
	for (line=0; line<noOfLines; line++) {
	  int32_t qua;
	  qua = 0;
	  mdctSpeL = mdctSpectrum[line];
	  if (mdctSpeL) {
		int32_t sa;
		int32_t saShft;
        sa = my_abs(mdctSpeL);
        saShft = sa << g;
        if (saShft > pquat0) {
          if (saShft < pquat1) {
            qua = mdctSpeL>0 ? 1 : -1;
		  }
          else {
            if (saShft < pquat2) {
              qua = mdctSpeL>0 ? 2 : -2;
			} else {
              if (saShft < pquat3) {
                qua = mdctSpeL>0 ? 3 : -3;
			  } else {
                qua = SingLineQuant(gain, sa);
                /* adjust the sign. Since 0 < qua < 1, this cannot overflow. */
                if (mdctSpeL < 0)
                  qua = -qua;
			  }
			}
		  }
		}
	  }
      quaSpectrum[line] = qua ;
	}
  }

}


/*****************************************************************************
*
* function name:iQuantL1
* description: iquantizes spectrum lines without sign
*              mdctSpectrum = iquaSpectrum^4/3 *2^(0.25*gain)
* input: global gain, number of lines to process,quantized spectrum
* output: spectral data
*
*****************************************************************************/
static void iQuantL1(const int16_t gain,
                           const int16_t noOfLines,
                           const int16_t *quantSpectrum,
                           int32_t *mdctSpectrum)
{
  int32_t   iquantizermod;
  int32_t   iquantizershift;
  int32_t   line;

  iquantizermod = gain & 3;
  iquantizershift = gain >> 2;

  for (line=0; line< noOfLines; line++) {

    if( quantSpectrum[line] != 0 ) {
      int32_t accu;
      int32_t ex;
	  int32_t tabIndex;
      int32_t specExp;
      int32_t s,t;
      int32_t tmp;

      accu = quantSpectrum[line];

      ex = norm_l(accu);
      accu = accu << ex;
      specExp = INT_BITS-1 - ex;

      tabIndex = (accu >> (INT_BITS-2-MANT_DIGITS)) & (~MANT_SIZE);

      /* calculate "mantissa" ^4/3 */
      s = mTab_4_3[tabIndex];

      /* get approperiate exponent multiplier for specExp^3/4 combined with scfMod */
      t = specExpMantTableComb_enc[iquantizermod][specExp];

      /* multiply "mantissa" ^4/3 with exponent multiplier */
#ifdef LINUX
			asm volatile("smull %[o1], %[o2], %[i1], %[i2]" : [o1] "+r" (tmp), [o2] "+r" (accu) : [i1] "r" (s), [i2] "r" (t));
//      accu = MULHIGH(s, t);

      /* get approperiate exponent shifter */
      specExp = specExpTableComb_enc[iquantizermod][specExp];

//      specExp += iquantizershift + 1;
      specExp += iquantizershift;

////      __asm {
////      	ADDS		specExp, specExp, #1
////      	RSBLT		specExp, specExp, #0
////      	MOVLT		accu, accu, asr specExp
////      	MOVGE		accu, accu, lsl specExp
////      }
			asm volatile("adds %[o1], %[o1], #1" : [o1] "+r" (specExp));
			asm volatile("rsblt %[o1], %[o1], #0" : [o1] "+r" (specExp));
			asm volatile("movlt %[o1], %[o1], asr %[i1]" : [o1] "+r" (accu) : [i1] "r" (specExp));
			asm volatile("movge %[o1], %[o1], lsl %[i1]" : [o1] "+r" (accu) : [i1] "r" (specExp));
#else
      __asm {
      	SMULL	tmp, accu, s, t
      }
//      accu = MULHIGH(s, t);

      /* get approperiate exponent shifter */
      specExp = specExpTableComb_enc[iquantizermod][specExp];

//      specExp += iquantizershift + 1;
      specExp += iquantizershift;

      __asm {
      	ADDS		specExp, specExp, #1
      	RSBLT		specExp, specExp, #0
      	MOVLT		accu, accu, asr specExp
      	MOVGE		accu, accu, lsl specExp
      }
#endif
      mdctSpectrum[line] = accu;
//	  if(specExp >= 0)
//		  mdctSpectrum[line] = accu << specExp;
//	  else
//		  mdctSpectrum[line] = accu >> (-specExp);
    }
    else {
      mdctSpectrum[line] = 0;
    }
  }
}

/*****************************************************************************
*
* function name: QuantScfBandsSpectr
* description: quantizes the entire spectrum
* returns:
* input: number of scalefactor bands to be quantized, ...
* output: quantized spectrum
*
*****************************************************************************/
void QuantScfBandsSpectr(int16_t sfbCnt,
                      int16_t maxSfbPerGroup,
                      int16_t sfbPerGroup,
                      int16_t *sfbOffset,
                      int32_t *mdctSpectrum,
                      int16_t globalGain,
                      int16_t *scalefactors,
                      int16_t *quantizedSpectrum)
{
  int32_t sfbOffs, sfb;

  for(sfbOffs=0;sfbOffs<sfbCnt;sfbOffs+=sfbPerGroup) {
    int32_t sfbNext ;
    for (sfb = 0; sfb < maxSfbPerGroup; sfb = sfbNext) {
      int16_t scalefactor = scalefactors[sfbOffs+sfb];
      /* coalesce sfbs with the same scalefactor */
      for (sfbNext = sfb+1;
           sfbNext < maxSfbPerGroup && scalefactor == scalefactors[sfbOffs+sfbNext];
           sfbNext++) ;

      SpectLinesQuant(globalGain - scalefactor,
                    sfbOffset[sfbOffs+sfbNext] - sfbOffset[sfbOffs+sfb],
                    mdctSpectrum + sfbOffset[sfbOffs+sfb],
                    quantizedSpectrum + sfbOffset[sfbOffs+sfb]);
    }
  }
}


/*****************************************************************************
*
* function name:Quant_Line2Distor
* description: quantizes and requantizes lines to calculate distortion
* input:  number of lines to be quantized, ...
* output: distortion
*
*****************************************************************************/
int32_t Quant_Line2Distor(const int32_t *spec,
                   int16_t  sfbWidth,
                   int16_t  gain)
{
  int32_t line;
  int32_t dist;
  int32_t m = gain&3;
  int32_t g = (gain >> 2) + 4;
  int32_t g2 = (g << 1) + 1;
  int32_t spec_v, *spec_p;
  const int16_t *pquat, *repquat;
  int16_t pquat0, pquat1, pquat2, pquat3;
//  uint32_t tmp = 0x80000000;
    /* gain&3 */

  pquat = quantBorders[m];
  repquat = quantRecon[m];
  pquat0 = pquat[0];
  pquat1 = pquat[1];
  pquat2 = pquat[2];
  pquat3 = pquat[3];
  dist = 0;
  g += 16;
  if(g2 < 0 && g >= 0)
  {
	  g2 = -g2;
	  line = sfbWidth;
	  spec_p = (int32_t *)spec;
//	  for(line=0; line<sfbWidth; line++) {
	  do {
	  	  spec_v = *spec_p++;
		  if (spec_v) {
			  int32_t diff;
			  int32_t distSingle = 0;
			  int32_t sa;
			  int32_t saShft;
			  sa = my_abs(spec_v);
			  //saShft = round16(L_shr(sa, g));
			  //saShft = L_shr(sa, 16+g);
			  saShft = sa >> g;

			  if (saShft < pquat0) {
				  distSingle = (saShft * saShft) >> g2;
			  } else {
				  if (saShft < pquat1) {
					  diff = saShft - repquat[0];
					  distSingle = (diff * diff) >> g2;
				  } else {
					  if (saShft < pquat2) {
						  diff = saShft - repquat[1];
						  distSingle = (diff * diff) >> g2;
					  } else {
						  if (saShft < pquat3) {
							  diff = saShft - repquat[2];
							  distSingle = (diff * diff) >> g2;
						  } else {
							  int16_t qua = SingLineQuant(gain, sa);
							  int32_t iqval, diff32, t = 0;
							  /* now that we have quantized x, re-quantize it. */
							  iQuantL1(gain, 1, &qua, &iqval);
							  diff32 = sa - iqval;
#ifdef LINUX
								asm volatile("smull %[o1], %[o2], %[i1], %[i1]" : [o1] "+r" (t), [o2] "+r" (distSingle) : [i1] "r" (diff32));
#else
							  __asm {
							  	SMULL	t, distSingle, diff32, diff32
							  }
#endif
							  distSingle <<= 1;
//							  distSingle = fixmul(diff32, diff32);
						  }
					  }
				  }
			  }
//			  dist = L_add(dist, distSingle);
			  dist += distSingle;
		  }
	  } while (--line!=0);
  } else {
	  line = sfbWidth;
	  spec_p = (int32_t *) spec;

//	  for(line=0; line<sfbWidth; line++) {
	  do {
	  	  spec_v = *spec_p++;
		  if (spec_v) {
			  int32_t diff;
			  int32_t distSingle = 0;
			  int32_t sa;
			  int32_t saShft;
			  sa = my_abs(spec_v);
			  //saShft = round16(L_shr(sa, g));
			  saShft = L_shr(sa, g);

			  if (saShft < pquat0) {
				  distSingle = L_shl((saShft * saShft), g2);
			  }
			  else {

				  if (saShft < pquat1) {
					  diff = saShft - repquat[0];
					  distSingle = L_shl((diff * diff), g2);
				  }
				  else {

					  if (saShft < pquat2) {
						  diff = saShft - repquat[1];
						  distSingle = L_shl((diff * diff), g2);
					  }
					  else {

						  if (saShft < pquat3) {
							  diff = saShft - repquat[2];
							  distSingle = L_shl((diff * diff), g2);
						  }
						  else {
							  int16_t qua = SingLineQuant(gain, sa);
							  int32_t iqval, diff32, t = 0;
							  /* now that we have quantized x, re-quantize it. */
							  iQuantL1(gain, 1, &qua, &iqval);
							  diff32 = sa - iqval;
#ifdef LINUX
								asm volatile("smull %[o1], %[o2], %[i1], %[i1]" : [o1] "+r" (t), [o2] "+r" (distSingle) : [i1] "r" (diff32));
#else
							  __asm {
							  	SMULL	t, distSingle, diff32, diff32
							  }
#endif
							  distSingle <<= 1;
//							  distSingle = fixmul(diff32, diff32);
						  }
					  }
				  }
			  }
//			  dist = L_add(dist, distSingle);
			  dist += distSingle;
		  }
	  } while (--line!=0);
  }

  return dist;
}
