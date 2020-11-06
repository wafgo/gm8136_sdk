

/*******************************************************************************
	File:		oper_32b.h

	Content:	Double precision operations

*******************************************************************************/

#ifndef __OPER_32b_H
#define __OPER_32b_H

#include "typedef.h"

#define POW2_TABLE_BITS 8
#define POW2_TABLE_SIZE (1<<POW2_TABLE_BITS)

void L_Extract (int32_t L_32, int16_t *hi, int16_t *lo);
int32_t L_Comp (int16_t hi, int16_t lo);
int32_t Mpy_32 (int16_t hi1, int16_t lo1, int16_t hi2, int16_t lo2);
int32_t Mpy_32_16 (int16_t hi, int16_t lo, int16_t n);
int32_t Div_32 (int32_t L_num, int32_t denom);
int16_t iLog4(int32_t value);
int32_t rsqrt(int32_t value,  int32_t accuracy);
int32_t pow2_xy(int32_t x, int32_t y);
#ifdef LINUX
static __inline int L_mpy_ls(int L_var2, short var1)
{
    unsigned short swLow1;
    short swHigh1;
    int l_var_out;

    swLow1 = (unsigned short)(L_var2);
    swHigh1 = (short)(L_var2 >> 16);

    l_var_out = (long)swLow1 * (long)var1 >> 15;

    l_var_out += swHigh1 * var1 << 1;

    return(l_var_out);
}
#else
__inline int32_t L_mpy_ls(int32_t L_var2, int16_t var1)
{
	int32_t acc0, acc1;

    __asm {
    	SMULL	acc0, acc1, L_var2, var1<<16
    }
    acc0 = ((uint32_t)acc0>>31) + (acc1<<1);

    return(acc0);
}
#endif

#ifdef LINUX
static __inline int32_t L_mpy_wx(int32_t L_var2, int16_t var1)
{
	int result = 0;
	asm ("SMULWB  %[result], %[L_var2], %[var1] \n"	:[result] "+r" (result)	: [L_var2] "r" (L_var2), [var1] "r" (var1));
	return result;
}
#else
__inline int32_t L_mpy_wx(int32_t L_var2, int16_t var1)
{
#if ARMV5TE_L_MPY_LS
	int32_t result;
	__asm {
		SMULWB	result, L_var2, var1
	}
//	asm volatile(
//		"SMULWB  %[result], %[L_var2], %[var1] \n"
//		:[result]"+r"(result)
//		:[L_var2]"r"(L_var2), [var1]"r"(var1)
//		);
	return result;
#else
	int32_t a, l_var_out;
	__asm {
		SMULL	a, l_var_out, L_var2, var1 << 16
	}
//    unsigned short swLow1;
//    int16_t swHigh1;
//    int32_t l_var_out;

//    swLow1 = (unsigned short)(L_var2);
//    swHigh1 = (int16_t)(L_var2 >> 16);

//    l_var_out = (long)swLow1 * (long)var1 >> 16;
//    l_var_out += swHigh1 * var1;

    return(l_var_out);
#endif
}
#endif

#endif
