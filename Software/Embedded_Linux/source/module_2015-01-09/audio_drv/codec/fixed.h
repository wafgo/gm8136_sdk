#ifndef __FIXED_H__
#define __FIXED_H__

#ifdef __cplusplus
extern "C" {
#endif

#define COEF_BITS 28
#define COEF_PRECISION (1 << COEF_BITS)
#define REAL_BITS 14 // MAXIMUM OF 14 FOR FIXED POINT SBR
#define REAL_PRECISION (1 << REAL_BITS)

/* FRAC is the fractional only part of the fixed point number [0.0..1.0) */
#define FRAC_SIZE 32 /* frac is a 32 bit integer */
#define FRAC_BITS 31
#define FRAC_PRECISION ((unsigned int)(1 << FRAC_BITS))
#define FRAC_MAX 0x7FFFFFFF

typedef int real_t;

#define REAL_CONST(A) (((A) >= 0) ? ((real_t)((A)*(REAL_PRECISION)+0.5)) : ((real_t)((A)*(REAL_PRECISION)-0.5)))
#define COEF_CONST(A) (((A) >= 0) ? ((real_t)((A)*(COEF_PRECISION)+0.5)) : ((real_t)((A)*(COEF_PRECISION)-0.5)))
#define FRAC_CONST(A) (((A) == 1.00) ? ((real_t)FRAC_MAX) : (((A) >= 0) ? ((real_t)((A)*(FRAC_PRECISION)+0.5)) : ((real_t)((A)*(FRAC_PRECISION)-0.5))))

#ifndef ARM_ASM
#define __int64		long long
/* multiply with real shift */
static INLINE real_t MUL_R(real_t A, real_t B)
{
    /*_asm {
        mov eax,A
        imul B
        shrd eax,edx,REAL_BITS
    }*/

	return (real_t)(((__int64)A*B)>>REAL_BITS);
}

/* multiply with coef shift */
static INLINE real_t MUL_C(real_t A, real_t B)
{
    /*_asm {
        mov eax,A
        imul B
        shrd eax,edx,COEF_BITS
    }*/

	return (real_t)(((__int64)A*B)>>COEF_BITS);
}

static INLINE real_t _MulHigh(real_t A, real_t B)
{
    /*_asm {
        mov eax,A
        imul B
        mov eax,edx
    }*/

	return (real_t)(((__int64)A*B)>>32);
}

/* multiply with fractional shift */
static INLINE real_t MUL_F(real_t A, real_t B)
{
    return _MulHigh(A,B) << (FRAC_SIZE-FRAC_BITS);
}

/* Complex multiplication */
static INLINE void ComplexMult(real_t *y1, real_t *y2,
    real_t x1, real_t x2, real_t c1, real_t c2)
{
    *y1 = (_MulHigh(x1, c1) + _MulHigh(x2, c2))<<(FRAC_SIZE-FRAC_BITS);
    *y2 = (_MulHigh(x2, c1) - _MulHigh(x1, c2))<<(FRAC_SIZE-FRAC_BITS);
}

#else
//ARM_ASM
//Shawn 2004.8.20
#if 0
// ARM_ASM not used !!! Shawn 2004.12.9
static INLINE real_t MUL_C(real_t A, real_t B)
{
    real_t lo, tmp, hi, r;

	__asm
	{
		smull	lo, hi, A, B
		mov	tmp, lo, lsr COEF_BITS
		orr	r, tmp, hi, lsl (32 - COEF_BITS)
	}

    return r;
}

static INLINE real_t MUL_R(real_t A, real_t B)
{
	real_t lo, tmp, hi, r;

	__asm
	{
		smull	lo, hi, A, B
		mov	tmp, lo, lsr REAL_BITS
		orr	r, tmp, hi, lsl (32 - REAL_BITS)
	}

    return r;
}


static INLINE real_t MUL_F(real_t A, real_t B)
{
	real_t lo, hi, r;

	__asm
	{
		smull lo, hi, A, B
		mov	r, hi, lsl (FRAC_SIZE-FRAC_BITS)
	}

    return r;
}

/* Complex multiplication */
static INLINE void ComplexMult(real_t *y1, real_t *y2, real_t x1, real_t x2, real_t c1, real_t c2)
{
	real_t tmp, tmpy1, tmpy2;

	__asm
	{
		smull	tmp, tmpy1, c1, x1
		smlal	tmp, tmpy1, c2, x2
		rsb	x1, x1, #0
		smull	tmp, tmpy2, c1, x2
		smlal	tmp, tmpy2, c2, x1
	}

	*y1 = tmpy1 << (FRAC_SIZE-FRAC_BITS);
    *y2 = tmpy2 << (FRAC_SIZE-FRAC_BITS);
}
#endif



#endif



#ifdef __cplusplus
}
#endif
#endif
