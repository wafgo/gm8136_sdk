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
	File:		oper_32b.c

	Content:	  This file contains operations in double precision.

*******************************************************************************/

#include "typedef.h"
#include "basic_op.h"
#include "oper_32b.h"

/*****************************************************************************
 *                                                                           *
 *  Function L_Extract()                                                     *
 *                                                                           *
 *  Extract from a 32 bit integer two 16 bit DPF.                            *
 *                                                                           *
 *  Arguments:                                                               *
 *                                                                           *
 *   L_32      : 32 bit integer.                                             *
 *               0x8000 0000 <= L_32 <= 0x7fff ffff.                         *
 *   hi        : b16 to b31 of L_32                                          *
 *   lo        : (L_32 - hi<<16)>>1                                          *
 *****************************************************************************
*/

void L_Extract (int32_t L_32, int16_t *hi, int16_t *lo)
{
    *hi = (L_32>>16);
	*lo = (L_32 - (*hi<<16))>>1;
    return;

//    *hi = extract_h (L_32);
//    *lo = extract_l (L_msu (L_shr (L_32, 1), *hi, 16384));
//    return;
}

/*****************************************************************************
 *                                                                           *
 *  Function L_Comp()                                                        *
 *                                                                           *
 *  Compose from two 16 bit DPF a 32 bit integer.                            *
 *                                                                           *
 *     L_32 = hi<<16 + lo<<1                                                 *
 *                                                                           *
 *  Arguments:                                                               *
 *                                                                           *
 *   hi        msb                                                           *
 *   lo        lsf (with sign)                                               *
 *                                                                           *
 *   Return Value :                                                          *
 *                                                                           *
 *             32 bit long signed integer (int32_t) whose value falls in the  *
 *             range : 0x8000 0000 <= L_32 <= 0x7fff fff0.                   *
 *                                                                           *
 *****************************************************************************
*/

int32_t L_Comp (int16_t hi, int16_t lo)
{
//    int32_t L_32;
//
//    L_32 = L_deposit_h (hi);
//    return (L_mac (L_32, lo, 1));       /* = hi<<16 + lo<<1 */
    return ((hi<<16) + (lo<<1));       /* = hi<<16 + lo<<1 */
}

/*****************************************************************************
 * Function Mpy_32()                                                         *
 *                                                                           *
 *   Multiply two 32 bit integers (DPF). The result is divided by 2**31      *
 *                                                                           *
 *   L_32 = (hi1*hi2)<<1 + ( (hi1*lo2)>>15 + (lo1*hi2)>>15 )<<1              *
 *                                                                           *
 *   This operation can also be viewed as the multiplication of two Q31      *
 *   number and the result is also in Q31.                                   *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 *  hi1         hi part of first number                                      *
 *  lo1         lo part of first number                                      *
 *  hi2         hi part of second number                                     *
 *  lo2         lo part of second number                                     *
 *                                                                           *
 *****************************************************************************
*/

int32_t Mpy_32 (int16_t hi1, int16_t lo1, int16_t hi2, int16_t lo2)
{
//    int32_t L_32;
//
//    L_32 = L_mult (hi1, hi2);
//    L_32 = L_mac (L_32, mult (hi1, lo2), 1);
//    L_32 = L_mac (L_32, mult (lo1, hi2), 1);
//
//    return (L_32);
#ifdef LINUX
		asm volatile("mul %[result0], %[in0], %[result0]" : [result0] "+r" (lo2) : [in0] "r" (hi1));
		asm volatile("mul %[result0], %[in0], %[result0]" : [result0] "+r" (lo1) : [in0] "r" (hi2));
		asm volatile("mov %[result0], %[result0], asr #15" : [result0] "+r" (lo2));
		asm volatile("add %[result0], %[in0], %[result0], asr #15" : [result0] "+r" (lo1) : [in0] "r" (lo2));
		asm volatile("mov %[result0], %[result0], lsl #1" : [result0] "+r" (lo1));
		asm volatile("mul %[result0], %[in0], %[result0]" : [result0] "+r" (hi1) : [in0] "r" (hi2));
		asm volatile("add %[result0], %[in0], %[result0], lsl #1" : [result0] "+r" (hi1) : [in0] "r" (lo1));
#else
    __asm {
		MUL lo2,hi1,lo2
		MUL lo1,hi2,lo1
		MOV lo2,lo2,ASR #15
		ADD lo1,lo2,lo1,ASR #15
		MOV lo1,lo1,lsl #1
		MUL hi1,hi2,hi1
		ADD hi1,lo1,hi1,lsl #1
	}
#endif
    return (hi1);
}

/*****************************************************************************
 * Function Mpy_32_16()                                                      *
 *                                                                           *
 *   Multiply a 16 bit integer by a 32 bit (DPF). The result is divided      *
 *   by 2**15                                                                *
 *                                                                           *
 *                                                                           *
 *   L_32 = (hi1*lo2)<<1 + ((lo1*lo2)>>15)<<1                                *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 *  hi          hi part of 32 bit number.                                    *
 *  lo          lo part of 32 bit number.                                    *
 *  n           16 bit number.                                               *
 *                                                                           *
 *****************************************************************************
*/

int32_t Mpy_32_16 (int16_t hi, int16_t lo, int16_t n)
{
//    int32_t L_32;
//
//    L_32 = L_mult (hi, n);
//    L_32 = L_mac (L_32, mult (lo, n), 1);
//
//    return (L_32);
#ifdef LINUX
		asm volatile("mul %[result0], %[in0], %[result0]" : [result0] "+r" (lo) : [in0] "r" (n));
		asm volatile("mul %[result0], %[in0], %[result0]" : [result0] "+r" (hi) : [in0] "r" (n));
		asm volatile("mov %[result0], %[result0], asr #15" : [result0] "+r" (lo));
		asm volatile("mov %[result0], %[result0], lsl #1" : [result0] "+r" (hi));
		asm volatile("add %[result0], %[result0], %[in0], lsl #1" : [result0] "+r" (hi) : [in0] "r" (lo));
#else
	__asm {
		MUL lo,n,lo
		MUL hi,n,hi
		MOV lo,lo,ASR #15
		MOV hi,hi,LSL #1
		ADD hi,hi,lo,LSL #1
	}
#endif
    return (hi);

}

/*****************************************************************************
 *                                                                           *
 *   Function Name : Div_32                                                  *
 *                                                                           *
 *   Purpose :                                                               *
 *             Fractional integer division of two 32 bit numbers.            *
 *             L_num / L_denom.                                              *
 *             L_num and L_denom must be positive and L_num < L_denom.       *
 *             L_denom = denom_hi<<16 + denom_lo<<1                          *
 *             denom_hi is a normalize number.                               *
 *                                                                           *
 *   Inputs :                                                                *
 *                                                                           *
 *    L_num                                                                  *
 *             32 bit long signed integer (int32_t) whose value falls in the  *
 *             range : 0x0000 0000 < L_num < L_denom                         *
 *                                                                           *
 *    L_denom = denom_hi<<16 + denom_lo<<1      (DPF)                        *
 *                                                                           *
 *       denom_hi                                                            *
 *             16 bit positive normalized integer whose value falls in the   *
 *             range : 0x4000 < hi < 0x7fff                                  *
 *       denom_lo                                                            *
 *             16 bit positive integer whose value falls in the              *
 *             range : 0 < lo < 0x7fff                                       *
 *                                                                           *
 *   Return Value :                                                          *
 *                                                                           *
 *    L_div                                                                  *
 *             32 bit long signed integer (int32_t) whose value falls in the  *
 *             range : 0x0000 0000 <= L_div <= 0x7fff ffff.                  *
 *                                                                           *
 *  Algorithm:                                                               *
 *                                                                           *
 *  - find = 1/L_denom.                                                      *
 *      First approximation: approx = 1 / denom_hi                           *
 *      1/L_denom = approx * (2.0 - L_denom * approx )                       *
 *                                                                           *
 *  -  result = L_num * (1/L_denom)                                          *
 *****************************************************************************
*/

int32_t Div_32 (int32_t L_num, int32_t denom)
{
    int16_t approx;
    int32_t L_32;
    int32_t acc0 = 0;
    /* First approximation: 1 / L_denom = 1/denom_hi */

    approx = div_s ((int16_t) 0x3fff, denom >> 16);

    /* 1/L_denom = approx * (2.0 - L_denom * approx) */

    L_32 = L_mpy_ls (denom, approx);

    L_32 = L_sub ((int32_t) 0x7fffffffL, L_32);

	L_32 = L_mpy_ls (L_32, approx);
    /* L_num * (1/L_denom) */
#ifdef LINUX
		asm volatile("smull %[o1], %[o2], %[o2], %[i1]" : [o1] "+r" (acc0), [o2] "+r" (L_32) : [i1] "r" (L_num));
#else
	__asm {
		SMULL	acc0, L_32, L_32, L_num
	}
#endif
//	L_32 = MULHIGH(L_32, L_num);
    L_32 = L_shl (L_32, 3);

    return (L_32);
}

/*!

  \brief  calculates the log dualis times 4 of argument
          iLog4(x) = (int32_t)(4 * log(value)/log(2.0))

  \return ilog4 value

*/
int16_t iLog4(int32_t value)
{
  int16_t iLog4;

  if(value != 0){
    int32_t tmp;
    int16_t tmp16;
    iLog4 = norm_l(value);
    tmp = (value << iLog4);
    tmp16 = round16(tmp);
//    tmp = L_mult(tmp16, tmp16);
#ifdef LINUX
		asm volatile("mul %[o1], %[i1], %[i1]" : [o1] "+r" (tmp) : [i1] "r" (tmp16));
		asm volatile("teq %[o1], #0x40000000" : : [o1] "r" (tmp));
		asm volatile("movne %[o1], %[o1], lsl #1" : [o1] "+r" (tmp));
		asm volatile("mvneq %[o1], #0x80000000" : [o1] "=r" (tmp));
#else
	__asm
	{
		MUL 	tmp, tmp16, tmp16
		TEQ 	tmp, #0x40000000
		MOVNE 	tmp, tmp, LSL #1
		MVNEQ 	tmp, #0x80000000
	}
#endif
    tmp16 = round16(tmp);
//    tmp = L_mult(tmp16, tmp16);
#ifdef LINUX
		asm volatile("mul %[o1], %[i1], %[i1]" : [o1] "+r" (tmp) : [i1] "r" (tmp16));
		asm volatile("teq %[o1], #0x40000000" : : [o1] "r" (tmp));
		asm volatile("movne %[o1], %[o1], lsl #1" : [o1] "+r" (tmp));
		asm volatile("mvneq %[o1], #0x80000000" : [o1] "=r" (tmp));
#else
	__asm
	{
		MUL 	tmp, tmp16, tmp16
		TEQ 	tmp, #0x40000000
		MOVNE 	tmp, tmp, LSL #1
		MVNEQ 	tmp, #0x80000000
	}
#endif
    tmp16 = round16(tmp);

    iLog4 = (-(iLog4 << 2) - norm_s(tmp16)) - 1;
  }
  else {
    iLog4 = -128; /* -(INT_BITS*4); */
  }

  return iLog4;
}

#define step(shift) \
    if ((0x40000000l >> shift) + root <= value)       \
    {                                                 \
        value -= (0x40000000l >> shift) + root;       \
        root = (root >> 1) | (0x40000000l >> shift);  \
    } else {                                          \
        root = root >> 1;                             \
    }

int32_t rsqrt(int32_t value,     /*!< Operand to square root (0.0 ... 1) */
             int32_t accuracy)  /*!< Number of valid bits that will be calculated */
{
    int32_t root = 0;
	int32_t scale;

	if(value < 0)
		return 0;

	scale = norm_l(value);
	if(scale & 1) scale--;

	value <<= scale;

	step( 0); step( 2); step( 4); step( 6);
    step( 8); step(10); step(12); step(14);
    step(16); step(18); step(20); step(22);
    step(24); step(26); step(28); step(30);

    scale >>= 1;
	if (root < value)
        ++root;

	root >>= scale;
    return root* 46334;
}

static const int32_t pow2Table[POW2_TABLE_SIZE] = {
0x7fffffff, 0x7fa765ad, 0x7f4f08ae, 0x7ef6e8da,
0x7e9f0606, 0x7e476009, 0x7deff6b6, 0x7d98c9e6,
0x7d41d96e, 0x7ceb2523, 0x7c94acde, 0x7c3e7073,
0x7be86fb9, 0x7b92aa88, 0x7b3d20b6, 0x7ae7d21a,
0x7a92be8b, 0x7a3de5df, 0x79e947ef, 0x7994e492,
0x7940bb9e, 0x78ecccec, 0x78991854, 0x78459dac,
0x77f25cce, 0x779f5591, 0x774c87cc, 0x76f9f359,
0x76a7980f, 0x765575c8, 0x76038c5b, 0x75b1dba2,
0x75606374, 0x750f23ab, 0x74be1c20, 0x746d4cac,
0x741cb528, 0x73cc556d, 0x737c2d55, 0x732c3cba,
0x72dc8374, 0x728d015d, 0x723db650, 0x71eea226,
0x719fc4b9, 0x71511de4, 0x7102ad80, 0x70b47368,
0x70666f76, 0x7018a185, 0x6fcb096f, 0x6f7da710,
0x6f307a41, 0x6ee382de, 0x6e96c0c3, 0x6e4a33c9,
0x6dfddbcc, 0x6db1b8a8, 0x6d65ca38, 0x6d1a1057,
0x6cce8ae1, 0x6c8339b2, 0x6c381ca6, 0x6bed3398,
0x6ba27e66, 0x6b57fce9, 0x6b0daeff, 0x6ac39485,
0x6a79ad56, 0x6a2ff94f, 0x69e6784d, 0x699d2a2c,
0x69540ec9, 0x690b2601, 0x68c26fb1, 0x6879ebb6,
0x683199ed, 0x67e97a34, 0x67a18c68, 0x6759d065,
0x6712460b, 0x66caed35, 0x6683c5c3, 0x663ccf92,
0x65f60a80, 0x65af766a, 0x6569132f, 0x6522e0ad,
0x64dcdec3, 0x64970d4f, 0x64516c2e, 0x640bfb41,
0x63c6ba64, 0x6381a978, 0x633cc85b, 0x62f816eb,
0x62b39509, 0x626f4292, 0x622b1f66, 0x61e72b65,
0x61a3666d, 0x615fd05f, 0x611c6919, 0x60d9307b,
0x60962665, 0x60534ab7, 0x60109d51, 0x5fce1e12,
0x5f8bccdb, 0x5f49a98c, 0x5f07b405, 0x5ec5ec26,
0x5e8451d0, 0x5e42e4e3, 0x5e01a540, 0x5dc092c7,
0x5d7fad59, 0x5d3ef4d7, 0x5cfe6923, 0x5cbe0a1c,
0x5c7dd7a4, 0x5c3dd19c, 0x5bfdf7e5, 0x5bbe4a61,
0x5b7ec8f2, 0x5b3f7377, 0x5b0049d4, 0x5ac14bea,
0x5a82799a, 0x5a43d2c6, 0x5a055751, 0x59c7071c,
0x5988e209, 0x594ae7fb, 0x590d18d3, 0x58cf7474,
0x5891fac1, 0x5854ab9b, 0x581786e6, 0x57da8c83,
0x579dbc57, 0x57611642, 0x57249a29, 0x56e847ef,
0x56ac1f75, 0x567020a0, 0x56344b52, 0x55f89f70,
0x55bd1cdb, 0x5581c378, 0x55469329, 0x550b8bd4,
0x54d0ad5b, 0x5495f7a1, 0x545b6a8b, 0x542105fd,
0x53e6c9db, 0x53acb607, 0x5372ca68, 0x533906e0,
0x52ff6b55, 0x52c5f7aa, 0x528cabc3, 0x52538786,
0x521a8ad7, 0x51e1b59a, 0x51a907b4, 0x5170810b,
0x51382182, 0x50ffe8fe, 0x50c7d765, 0x508fec9c,
0x50582888, 0x50208b0e, 0x4fe91413, 0x4fb1c37c,
0x4f7a9930, 0x4f439514, 0x4f0cb70c, 0x4ed5ff00,
0x4e9f6cd4, 0x4e69006e, 0x4e32b9b4, 0x4dfc988c,
0x4dc69cdd, 0x4d90c68b, 0x4d5b157e, 0x4d25899c,
0x4cf022ca, 0x4cbae0ef, 0x4c85c3f1, 0x4c50cbb8,
0x4c1bf829, 0x4be7492b, 0x4bb2bea5, 0x4b7e587d,
0x4b4a169c, 0x4b15f8e6, 0x4ae1ff43, 0x4aae299b,
0x4a7a77d5, 0x4a46e9d6, 0x4a137f88, 0x49e038d0,
0x49ad1598, 0x497a15c4, 0x4947393f, 0x49147fee,
0x48e1e9ba, 0x48af768a, 0x487d2646, 0x484af8d6,
0x4818ee22, 0x47e70611, 0x47b5408c, 0x47839d7b,
0x47521cc6, 0x4720be55, 0x46ef8210, 0x46be67e0,
0x468d6fae, 0x465c9961, 0x462be4e2, 0x45fb521a,
0x45cae0f2, 0x459a9152, 0x456a6323, 0x453a564d,
0x450a6abb, 0x44daa054, 0x44aaf702, 0x447b6ead,
0x444c0740, 0x441cc0a3, 0x43ed9ac0, 0x43be9580,
0x438fb0cb, 0x4360ec8d, 0x433248ae, 0x4303c517,
0x42d561b4, 0x42a71e6c, 0x4278fb2b, 0x424af7da,
0x421d1462, 0x41ef50ae, 0x41c1aca8, 0x41942839,
0x4166c34c, 0x41397dcc, 0x410c57a2, 0x40df50b8,
0x40b268fa, 0x4085a051, 0x4058f6a8, 0x402c6be9
};

/*!

  \brief calculates 2 ^ (x/y) for x<=0, y > 0, x <= 32768 * y

  avoids integer division

  \return
*/
int32_t pow2_xy(int32_t x, int32_t y)
{
  int32_t iPart;
  int32_t fPart;
  int32_t res;
  int32_t  tmp2;//tmp,
 // int32_t shift, shift2;

  tmp2 = -x;
  iPart = tmp2 / y;
  fPart = tmp2 - iPart*y;
  iPart = min(iPart,INT_BITS-1);

  res = pow2Table[(POW2_TABLE_SIZE*fPart)/y] >> iPart;

  return(res);
}
