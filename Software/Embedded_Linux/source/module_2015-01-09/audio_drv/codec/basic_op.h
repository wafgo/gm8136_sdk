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
	File:		basicop2.h

	Content:	Constants , Globals and Basic arithmetic operators.

*******************************************************************************/

#ifndef __BASIC_OP_H
#define __BASIC_OP_H

#include "typedef.h"

#define LINUX

#define MAX_32 (int32_t)0x7fffffffL
#define MIN_32 (int32_t)0x80000000L

#define MAX_16 (int16_t)0x7fff
#define MIN_16 (int16_t)0x8000
#define ABS(a)	((a) >= 0) ? (a) : (-(a))

/* Short abs,           1   */
//#define abs_s(x)       (((x) != MIN_16) ? (((x) >= 0) ? (x) : (-(x))) : MAX_16)
#define abs_s(a)       (((a) < 0) ? -(a) : (a))
#define my_abs(a) (((a) < 0) ? -(a) : (a))

/* 16 bit var1 -> MSB,     2 */
#define L_deposit_h(x) (((int32_t)(x)) << 16)


/* 16 bit var1 -> LSB,     2 */
#define L_deposit_l(x) ((int32_t)(x))


/* Long abs,              3  */
//#define L_abs(x) (((x) != MIN_32) ? (((x) >= 0) ? (x) : (-(x))) : MAX_32)
#define L_abs(a) (((a) < 0) ? -(a) : (a))

/* Short negate,        1   */
//#define negate(var1) (((var1) == MIN_16) ? MAX_16 : (-(var1)))
#define negate(var1) (-var1)

/* Long negate,     2 */
//#define L_negate(L_var1) (((L_var1) == (MIN_32)) ? (MAX_32) : (-(L_var1)))
#define L_negate(L_var1) (-L_var1)

//#define MULHIGH(A,B) (int)(((int64_t)(A)*(int64_t)(B)) >> 32)
//#define fixmul(a, b) (int)((((int64_t)(a)*(int64_t)(b)) >> 32) << 1)

static __inline int32_t saturate(int32_t L_var1);
//static __inline int32_t shl (int32_t var1, int32_t var2);
//static __inline int32_t shr (int32_t var1, int32_t var2);
static __inline int32_t L_mult(int32_t var1, int32_t var2);
static __inline int32_t L_msu (int32_t L_var3, int32_t var1, int32_t var2);
static __inline int32_t L_sub(int32_t L_var1, int32_t L_var2);
static __inline int32_t L_shl (int32_t L_var1, int32_t var2);
static __inline int32_t L_shr (int32_t L_var1, int32_t var2);
static __inline int32_t add (int32_t var1, int32_t var2);
static __inline int32_t sub(int32_t var1, int32_t var2);
static __inline int32_t div_s (int32_t var1, int32_t var2);
static __inline int32_t mult (int32_t var1, int32_t var2);
static __inline int32_t norm_s (int32_t var1);
static __inline int32_t norm_l (int32_t L_var1);
static __inline int32_t round16(int32_t L_var1);
static __inline int32_t L_mac (int32_t L_var3, int32_t var1, int32_t var2);
static __inline int32_t L_add (int32_t L_var1, int32_t L_var2);
static __inline int32_t extract_h (int32_t L_var1);
static __inline int32_t extract_l(int32_t L_var1);
static __inline int32_t mult_r(int32_t var1, int32_t var2);
//static __inline int32_t shr_r (int32_t var1, int32_t var2);
static __inline int32_t mac_r (int32_t L_var3, int32_t var1, int32_t var2);
static __inline int32_t msu_r (int32_t L_var3, int32_t var1, int32_t var2);
static __inline int32_t L_shr_r (int32_t L_var1, int32_t var2);
static __inline int32_t fixmul(int32_t a, int32_t b);


__inline int32_t fixmul(int32_t a, int32_t b)
{
	int acc0, acc1, tmp;
#ifdef LINUX
	asm volatile("smull %[result0], %[result1], %[in0], %[in1]" : [result0] "=r" (acc0), [result1] "=r" (acc1) : [in0] "r" (a), [in1] "r" (b));
	tmp = acc1<<1;
	return tmp;
#else
	__asm {
		SMULL	acc0, acc1, a, b
	}
	acc1 <<= 1;
	return acc1;
#endif
}


/*
__inline int32_t ASM_shr(int32_t L_var1, int32_t var2)
{
	int32_t result;
	__asm {
		CMP		var2, #15
		MOVLT	result, L_var1, ASR var2
		MOVGE 	result, L_var1, ASR #15
	}
	return result;
}

__inline int32_t ASM_shl(int32_t L_var1, int32_t var2)
{
	int32_t result;
	int32_t tmp;
	__asm {
		CMP		var2, #16
		MOVLT	result, L_var1, ASL var2
		MOVGE	result, L_var1, ASL #16
		MOV		tmp, result, ASR #15
		TEQ		tmp, result, ASR #31
		EORNE	result, 0x7fff, result, ASR #31
	}
	return result;
}
*/

/*___________________________________________________________________________
 |                                                                           |
 |   definitions for inline basic arithmetic operators                       |
 |___________________________________________________________________________|
*/
__inline int32_t saturate(int32_t L_var1)
{
	int32_t result;
	int32_t tmp;
#ifdef LINUX
	asm volatile (
		"MOV	%[tmp], %[L_var1],ASR#15\n"
		"TEQ	%[tmp], %[L_var1],ASR#31\n"
		"EORNE	%[result], %[mask],%[L_var1],ASR#31\n"
		"MOVEQ	%[result], %[L_var1]\n"
		:[result]"=&r"(result), [tmp]"=&r"(tmp)
		:[L_var1]"r"(L_var1), [mask]"r"(0x7fff)
	);
#else
	__asm {
		MOV		tmp, L_var1, ASR#15
		TEQ 	tmp, L_var1, ASR#31
		EORNE	result, 0x7fff, L_var1, ASR#31
		MOVEQ	result, L_var1
	}
#endif
	return result;
}

/* Short shift left,    1   */
/*
__inline int32_t shl (int32_t var1, int32_t var2)
{
	if(var2>=0)
	{
		return ASM_shl( var1, var2);
	}
	else
	{
		return ASM_shr( var1, -var2);
	}
}
*/
/* Short shift right,   1   */
/*
__inline int32_t shr (int32_t var1, int32_t var2)
{
	if(var2>=0)
	{
		return  ASM_shr( var1, var2);
	}
	else
	{
		return  ASM_shl( var1, -var2);
	}
}
*/

__inline int32_t L_mult(int32_t var1, int32_t var2)
{
	int result;
#ifdef LINUX
		asm (
		"SMULBB %[result], %[var1], %[var2] \n"
		"QADD %[result], %[result], %[result] \n"
		:[result]"=r"(result)
		:[var1]"r"(var1), [var2]"r"(var2)
		);
#else
	__asm
	{
		SMULBB 	result, var1, var2
		QADD		result, result, result
	}
#endif
	return result;
//    int32_t L_var_out;
//
//    L_var_out = (int32_t) var1 *(int32_t) var2;
//
//    if (L_var_out != (int32_t) 0x40000000L)
//    {
//        L_var_out <<= 1;
//    }
//    else
//    {
//        L_var_out = MAX_32;
//    }
//    return (L_var_out);
}

__inline int32_t L_msu (int32_t L_var3, int32_t var1, int32_t var2)
{
	int result;
#ifdef LINUX
	asm (
		"SMULBB %[result], %[var1], %[var2] \n"
		"QDSUB %[result], %[L_var3], %[result]\n"
		:[result]"=&r"(result)
		:[L_var3]"r"(L_var3), [var1]"r"(var1), [var2]"r"(var2)
		);
#else
	__asm {
		SMULBB	result, var1, var2
		QDSUB		result, L_var3, result
	}
#endif
	return result;

//    int32_t L_var_out;
//    int32_t L_product;
//
//    L_product = L_mult(var1, var2);
//    L_var_out = L_sub(L_var3, L_product);
//    return (L_var_out);
}

__inline int32_t L_sub(int32_t L_var1, int32_t L_var2)
{
    int32_t result;
#ifdef LINUX
		asm (
			"QSUB %[result], %[L_var1], %[L_var2]\n"
			:[result]"=r"(result)
			:[L_var1]"r"(L_var1), [L_var2]"r"(L_var2)
		);
#else
	__asm{
			QSUB	result, L_var1, L_var2
    }
#endif
    return result;
/*
    int32_t L_var_out;

    L_var_out = L_var1 - L_var2;

    if (((L_var1 ^ L_var2) & MIN_32) != 0)
    {
        if ((L_var_out ^ L_var1) & MIN_32)
        {
            L_var_out = (L_var1 < 0L) ? MIN_32 : MAX_32;
        }
    }

    return (L_var_out);
*/
}

static __inline int32_t ASM_L_shr(int32_t L_var1, int32_t var2)
{
	return L_var1 >> var2;
}

static __inline int32_t ASM_L_shl(int32_t L_var1, int32_t var2)
{
	int32_t result;
#ifdef LINUX
	asm (
		"MOV	%[result], %[L_var1], ASL %[var2] \n"
		"TEQ	%[L_var1], %[result], ASR %[var2]\n"
		"EORNE  %[result], %[mask], %[L_var1], ASR #31\n"
		:[result]"=&r"(result)
		:[L_var1]"r"(L_var1), [var2]"r"(var2), [mask]"r"(0x7fffffff)
		);
#else
	__asm {
		MOV		result, L_var1, ASL var2
		TEQ		L_var1, result, ASR var2
		EORNE	result, 0x7fffffff, L_var1, ASR #31
	}
#endif
	return result;
}

__inline int32_t L_shl(int32_t L_var1, int32_t var2)
{
    if(var2>=0)
    {
        return  ASM_L_shl( L_var1, var2);
    }
    else
    {
        return  ASM_L_shr( L_var1, -var2);
    }
}

__inline int32_t L_shr (int32_t L_var1, int32_t var2)
{
	if(var2>=0)
	{
		return ASM_L_shr( L_var1, var2);
	}
	else
	{
		return ASM_L_shl( L_var1, -var2);
	}
}

/* Short add,           1   */
__inline int32_t add (int32_t var1, int32_t var2)
{
	int32_t result;
	int32_t tmp;
#ifdef LINUX
	asm (
		"ADD  %[result], %[var1], %[var2] \n"
		"MOV  %[tmp], %[result], ASR #15 \n"
		"TEQ  %[tmp], %[result], ASR #31 \n"
		"EORNE %[result], %[mask], %[result], ASR #31"
		:[result]"=&r"(result), [tmp]"=&r"(tmp)
		:[var1]"r"(var1), [var2]"r"(var2), [mask]"r"(0x7fff)
		);
#else
	__asm {
		ADD	result, var1, var2
		MOV	tmp, result, ASR #15
		TEQ	tmp, result, ASR #31
		EORNE	result, 0x7fff, result, ASR #31
	}
#endif
	return result;
}

/* Short sub,           1   */
__inline int32_t sub(int32_t var1, int32_t var2)
{
	int32_t result;
	int32_t tmp;
#ifdef LINUX
	asm (
		"SUB   %[result], %[var1], %[var2] \n"
		"MOV   %[tmp], %[var1], ASR #15 \n"
		"TEQ   %[tmp], %[var1], ASR #31 \n"
		"EORNE %[result], %[mask], %[result], ASR #31 \n"
		:[result]"=&r"(result), [tmp]"=&r"(tmp)
		:[var1]"r"(var1), [var2]"r"(var2), [mask]"r"(0x7fff)
		);
#else
	__asm {
		SUB	result, var1, var2
		MOV	tmp, result, ASR #15
		TEQ	tmp, result, ASR #31
		EORNE	result, 0x7fff, result, ASR #31
	}
#endif
	return result;

}

/* Short division,       18  */
__inline int32_t div_s (int32_t var1, int32_t var2)
{
    int32_t var_out = 0;
    int32_t iteration;
    int32_t L_num;
    int32_t L_denom;

    var_out = MAX_16;
    if (var1!= var2)//var1!= var2
    {
    	var_out = 0;
    	L_num = (int32_t) var1;

    	L_denom = (int32_t) var2;

		//return (L_num<<15)/var2;

    	for (iteration = 0; iteration < 15; iteration++)
    	{
    		var_out <<= 1;
    		L_num <<= 1;

    		if (L_num >= L_denom)
    		{
    			L_num -= L_denom;
    			var_out++;
    		}
    	}
    }
    return (var_out);
}

/* Short mult,          1   */
__inline int32_t mult (int32_t var1, int32_t var2)
{
#ifdef LINUX
	int result, tmp;
	asm (
		"SMULBB %[tmp], %[var1], %[var2] \n"
		"MOV	%[result], %[tmp], ASR #15\n"
		"MOV	%[tmp], %[result], ASR #15\n"
		"TEQ	%[tmp], %[result], ASR #31\n"
		"EORNE  %[result], %[mask], %[result], ASR #31 \n"
		:[result]"=&r"(result), [tmp]"=&r"(tmp)
		:[var1]"r"(var1), [var2]"r"(var2), [mask]"r"(0x7fff)
		);
	return result;
#else
	__asm {
		MUL 	var1, var2, var1
		TEQ 	var1, #0x40000000
		MOVNE 	var1, var1, ASR #15
		MOVEQ 	var1, #0x7FFF
	}
	return (var1);
#endif

//    int32_t var_out;
//    int32_t L_product;
//
//    L_product = (int32_t) var1 *(int32_t) var2;
//    L_product = (L_product & (int32_t) 0xffff8000L) >> 15;
//    if (L_product & (int32_t) 0x00010000L)
//        L_product = L_product | (int32_t) 0xffff0000L;
//    var_out = saturate(L_product);
//
//    return (var_out);
}


/* Short norm,           15  */
__inline int32_t norm_s (int32_t var1)
{
    int32_t var_out;

    if (var1 == 0)
    {
        var_out = 0;
    }
    else
    {
        if (var1 == -1)
        {
            var_out = 15;
        }
        else
        {
            if (var1 < 0)
            {
                var1 = (int16_t)~var1;
            }
            for (var_out = 0; var1 < 0x4000; var_out++)
            {
                var1 <<= 1;
            }
        }
    }
    return var_out;
}

/* Long norm,            30  */
/*
__inline int32_t norm_l (int32_t x)
{
	uint32_t shift;
	__asm {
		mov	shift, #-1
		cmp x, #1<<16
		movcc x, x, lsl#16
		addcc shift, shift, #16
		tst	x, #0xff000000
		moveq x, x, lsl #8
		addeq shift, shift, #8
		tst x, #0xf0000000
		moveq x, x, lsl #4
		addeq shift, shift, #4
		tst x, #0xc0000000
		moveq x, x, lsl #2
		addeq shift, shift, #2
		tst x, #0x80000000
		addeq shift, shift, #1
		moveqs x, x, lsl #1
		moveq shift, #32
	}
	return shift;
}
*/
#ifdef LINUX
__inline int32_t norm_l (int32_t L_var1)
{
	int result;
	asm volatile(
		"CMP    %[L_var1], #0\n"
		"CLZNE  %[result], %[L_var1]\n"
		"SUBNE  %[result], %[result], #1\n"
		"MOVEQ  %[result], #0\n"
		:[result]"=r"(result)
		:[L_var1]"r"(L_var1)
		);
	return result;
}
#else
static uint8_t hash_table[64] = {
0x20, 0x14, 0x13, 0xff, 0xff, 0x12, 0xff, 0x07,
0x0a, 0x11, 0xff, 0xff, 0x0e, 0xff, 0x06, 0xff,
0xff, 0x09, 0xff, 0x10, 0xff, 0xff, 0x01, 0x1a,
0xff, 0x0d, 0xff, 0xff, 0x18, 0x05, 0xff, 0xff,
0xff, 0x15, 0xff, 0x08, 0x0b, 0xff, 0x0f, 0xff,
0xff, 0xff, 0xff, 0x02, 0x1b, 0x00, 0x19, 0xff,
0x16, 0xff, 0x0c, 0xff, 0xff, 0x03, 0x1c, 0xff,
0x17, 0xff, 0x04, 0x1d, 0xff, 0xff, 0x1e, 0x1f};

__inline int32_t norm_l (int32_t x)
{
	int32_t shift;
	uint8_t *table;
	__asm {
		orr	shift, x, x, lsr #1
		orr shift, shift, shift, lsr #2
		orr shift, shift, shift, lsr #4
		orr shift, shift, shift, lsr #8
		bic shift, shift, shift, lsr #16
		rsb shift, shift, shift, lsl #9
		rsb shift, shift, shift, lsl #11
		rsb shift, shift, shift, lsl #14
	}
	table = hash_table;
	__asm {
		ldrb shift, [table, shift, lsr#26]
	}
	return (shift - 1);
}
#endif
/*
__inline int32_t norm_l (int32_t L_var1)
{
  int16_t a16;
  int16_t r = 0 ;
  int32_t shift;


  if ( L_var1 < 0 ) {
    L_var1 = ~L_var1;
  }

  if (0 == (L_var1 & 0x7fff8000)) {
    a16 = extract_l(L_var1);
    r += 16;

    if (0 == (a16 & 0x7f80)) {
      r += 8;

      if (0 == (a16 & 0x0078)) {
        r += 4;

        if (0 == (a16 & 0x0006)) {
          r += 2;

          if (0 == (a16 & 0x0001)) {
            r += 1;
          }
        }
        else {

          if (0 == (a16 & 0x0004)) {
            r += 1;
          }
        }
      }
      else {

        if (0 == (a16 & 0x0060)) {
          r += 2;

          if (0 == (a16 & 0x0010)) {
            r += 1;
          }
        }
        else {

          if (0 == (a16 & 0x0040)) {
            r += 1;
          }
        }
      }
    }
    else {

      if (0 == (a16 & 0x7800)) {
        r += 4;

        if (0 == (a16 & 0x0600)) {
          r += 2;

          if (0 == (a16 & 0x0100)) {
            r += 1;
          }
        }
        else {

          if (0 == (a16 & 0x0400)) {
            r += 1;
          }
        }
      }
      else {

        if (0 == (a16 & 0x6000)) {
          r += 2;

          if (0 == (a16 & 0x1000)) {
            r += 1;
          }
        }
        else {

          if (0 == (a16 & 0x4000)) {
            r += 1;
          }
        }
      }
    }
  }
  else {
    a16 = extract_h(L_var1);

    if (0 == (a16 & 0x7f80)) {
      r += 8;

      if (0 == (a16 & 0x0078)) {
        r += 4 ;

        if (0 == (a16 & 0x0006)) {
          r += 2;

          if (0 == (a16 & 0x0001)) {
            r += 1;
          }
        }
        else {

          if (0 == (a16 & 0x0004)) {
            r += 1;
          }
        }
      }
      else {

        if (0 == (a16 & 0x0060)) {
          r += 2;

          if (0 == (a16 & 0x0010)) {
            r += 1;
          }
        }
        else {

          if (0 == (a16 & 0x0040)) {
            r += 1;
          }
        }
      }
    }
    else {

      if (0 == (a16 & 0x7800)) {
        r += 4;

        if (0 == (a16 & 0x0600)) {
          r += 2;

          if (0 == (a16 & 0x0100)) {
            r += 1;
          }
        }
        else {

          if (0 == (a16 & 0x0400)) {
            r += 1;
          }
        }
      }
      else {

        if (0 == (a16 & 0x6000)) {
          r += 2;

          if (0 == (a16 & 0x1000)) {
            r += 1;
          }
        }
        else {

          if (0 == (a16 & 0x4000)) {
            return 1;
          }
        }
      }
    }
  }

	__asm {
		mov	shift, #0
		cmp L_var1, #1<<16
		movcc L_var1, L_var1, lsl#16
		addcc shift, shift, #16
		tst	L_var1, #0xff000000
		moveq L_var1, L_var1, lsl #8
		addeq shift, shift, #8
		tst L_var1, #0xf0000000
		moveq L_var1, L_var1, lsl #4
		addeq shift, shift, #4
		tst L_var1, #0xc0000000
		moveq L_var1, L_var1, lsl #2
		addeq shift, shift, #2
		tst L_var1, #0x80000000
		addeq shift, shift, #1
		moveqs L_var1, L_var1, lsl #1
		moveq shift, #32
	}
	printf("%d %d\n", shift-1, r);

  return r ;
}
*/


/* Round,               1   */
__inline int32_t round16(int32_t L_var1)
{
    int32_t var_out;
    int32_t L_rounded;

    L_rounded = L_add (L_var1, (int32_t) 0x00008000L);
    var_out = extract_h (L_rounded);
    return (var_out);
}

#ifdef LINUX
__inline int32_t L_mac (int32_t L_var3, int32_t var1, int32_t var2)
{
	int result;
	asm (
		"SMULBB %[result], %[var1], %[var2]\n"
		"QDADD  %[result], %[L_var3], %[result]\n"
		:[result]"=&r"(result)
		: [L_var3]"r"(L_var3), [var1]"r"(var1), [var2]"r"(var2)
		);
	return result;
}
#else
/* Mac,  1  */
__inline int32_t L_mac (int32_t L_var3, int32_t var1, int32_t var2)
{
    int32_t L_var;
	int32_t tmp;
	__asm
	{
		MUL 	L_var, var2, var1
		TEQ 	L_var, #0x40000000
		MOVNE 	L_var, L_var, LSL #1
		MVNEQ 	L_var, #0x80000000
	    // MUL result is in L_var
		MOV  	tmp, #0x80000000
		//ADDS 	L_var3, L_var, L_var3
		//EORVS 	L_var3, tmp, L_var3, ASR#31
		QADD   L_var3, L_var, L_var3
    } //asm
    return (L_var3);

//    int32_t L_var_out;
//    int32_t L_product;
//
//    L_product = L_mult(var1, var2);
//    L_var_out = L_add (L_var3, L_product);
//    return (L_var_out);
}
#endif

#ifdef LINUX
__inline int32_t L_add (int32_t L_var1, int32_t L_var2)
{
	int32_t out;
	asm volatile("qadd %[result0], %[in0], %[in1]" : [result0] "=r" (out) : [in0] "r" (L_var1), [in1] "r" (L_var2));
	return (out);
}
#else
__inline int32_t L_add (int32_t L_var1, int32_t L_var2)
{
	int32_t out;
	int32_t tmp;
	__asm{
		MOV		tmp, #0x80000000
		//ADDS	out, L_var1, L_var2
		//EORVS	out, tmp, out, ASR#31
		QADD   out, L_var1, L_var2
	}
	return (out);
}
#endif

#ifdef LINUX
__inline int32_t mult_r (int32_t var1, int32_t var2)
        {
  int16_t product;
	int32_t a, b = 0x4000, c = 0x7fff;
	asm volatile("mla %[result0], %[in0], %[in1], %[in2]" : [result0] "=r" (product) : [in0] "r" (var1), [in1] "r" (var2), [in2] "r" (b));
	asm volatile("mov %[result0], %[result0], asr #15" : [result0] "+r" (product));
	asm volatile("mov %[result0], %[in0], asr #15" : [result0] "=r" (a) : [in0] "r" (product));
	asm volatile("teq %[result0], %[in0], asr #31" : [result0] "=r" (a) : [in0] "r" (product));
	asm volatile("eorne %[result0], %[in0], %[result0], asr #31" : [result0] "+r" (product) : [in0] "r" (c));
	return product;
}
#else
__inline int32_t mult_r (int32_t var1, int32_t var2)
{
    int16_t product;
	int32_t a;//,tmp;
	__asm {
	    MLA 	product, var1, var2, 0x4000
	    MOV 	product, product, ASR#15
		MOV 	a, product, ASR#15
		TEQ 	a, product, ASR#31
		EORNE 	product, 0x7fff, product, ASR#31
	}
	return product;
}
#endif

/*
__inline int32_t shr_r (int32_t var1, int32_t var2)
{
    int32_t var_out;

    if (var2 > 15)
    {
        var_out = 0;
    }
    else
    {
        var_out = shr(var1, var2);

        if (var2 > 0)
        {
            if ((var1 & ((int16_t) 1 << (var2 - 1))) != 0)
            {
                var_out++;
            }
        }
    }

    return (var_out);
}
*/

__inline int32_t mac_r (int32_t L_var3, int32_t var1, int32_t var2)
{
    int32_t var_out;

    L_var3 = L_mac (L_var3, var1, var2);
    var_out = (int16_t)((L_var3 + 0x8000L) >> 16);

    return (var_out);
}

__inline int32_t msu_r (int32_t L_var3, int32_t var1, int32_t var2)
{
    int32_t var_out;

    L_var3 = L_msu (L_var3, var1, var2);
    var_out = (int16_t)((L_var3 + 0x8000L) >> 16);

    return (var_out);
}

__inline int32_t L_shr_r (int32_t L_var1, int32_t var2)
{
    int32_t L_var_out;

    if (var2 > 31)
    {
        L_var_out = 0;
    }
    else
    {
        L_var_out = L_shr(L_var1, var2);

        if (var2 > 0)
        {
            if ((L_var1 & ((int32_t) 1 << (var2 - 1))) != 0)
            {
                L_var_out++;
            }
        }
    }

    return (L_var_out);
}

__inline int32_t extract_h (int32_t L_var1)
{
    int32_t var_out;

    var_out = (L_var1 >> 16);

    return (var_out);
}

__inline int32_t extract_l(int32_t L_var1)
{
	return (int16_t) L_var1;
}

#endif
