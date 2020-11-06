/*
 * Fast (I)MDCT Implementation using (I)FFT ((Inverse) Fast Fourier Transform)
 * and consists of three steps: pre-(I)FFT complex multiplication, complex
 * (I)FFT, post-(I)FFT complex multiplication,
 *
 * As described in:
 *  P. Duhamel, Y. Mahieux, and J.P. Petit, "A Fast Algorithm for the
 *  Implementation of Filter Banks Based on 'Time Domain Aliasing
 *  Cancellation?" IEEE Proc. on ICASSP?1, 1991, pp. 2209-2212.
 *
 *
 * As of April 6th 2002 completely rewritten.
 * This (I)MDCT can now be used for any data size n, where n is divisible by 8.
 *
 */

#include <linux/slab.h>
#include "common.h"
#include "structs.h"
#include "cfft.h"
#include "mdct.h"
#define assert(expr) \
	if (!(expr)) { \
	printk( "Assertion failed! %s,%s,%s,line=%d\n",\
	#expr,__FILE__,__func__,__LINE__); \
	BUG(); \
	}

/* const_tab[]:
    0: sqrt(2 / N)
    1: cos(2 * PI / N)
    2: sin(2 * PI / N)
    3: cos(2 * PI * (1/8) / N)
    4: sin(2 * PI * (1/8) / N)
 */
real_t const_tab[][5] =
{
    {    /* 2048 */
        COEF_CONST(1),
        FRAC_CONST(0.99999529380957619),
        FRAC_CONST(0.0030679567629659761),
        FRAC_CONST(0.99999992646571789),
        FRAC_CONST(0.00038349518757139556)
    }, { /* 1920 */
        COEF_CONST(/* sqrt(1024/960) */ 1.0327955589886444),
        FRAC_CONST(0.99999464540169647),
        FRAC_CONST(0.0032724865065266251),
        FRAC_CONST(0.99999991633432805),
        FRAC_CONST(0.00040906153202803459)
    }, { /* 1024 */
        COEF_CONST(1),
        FRAC_CONST(0.99998117528260111),
        FRAC_CONST(0.0061358846491544753),
        FRAC_CONST(0.99999970586288223),
        FRAC_CONST(0.00076699031874270449)
    }, { /* 960 */
        COEF_CONST(/* sqrt(512/480) */ 1.0327955589886444),
        FRAC_CONST(0.99997858166412923),
        FRAC_CONST(0.0065449379673518581),
        FRAC_CONST(0.99999966533732598),
        FRAC_CONST(0.00081812299560725323)
    }, { /* 256 */
        COEF_CONST(1),
        FRAC_CONST(0.99969881869620425),
        FRAC_CONST(0.024541228522912288),
        FRAC_CONST(0.99999529380957619),
        FRAC_CONST(0.0030679567629659761)
    }, {  /* 240 */
        COEF_CONST(/* sqrt(256/240) */ 1.0327955589886444),
        FRAC_CONST(0.99965732497555726),
        FRAC_CONST(0.026176948307873149),
        FRAC_CONST(0.99999464540169647),
        FRAC_CONST(0.0032724865065266251)
    }
};

static char map_N_to_idx(unsigned short N)
{
    /* gives an index into const_tab above */
    /* for normal AAC deocding (eg. no scalable profile) only */
    /* index 0 and 4 will be used */
    switch(N)
    {
    case 2048: return 0;
    case 1920: return 1;
    case 1024: return 2;
    case 960:  return 3;
    case 256:  return 4;
    case 240:  return 5;
    }
    return 0;
}

#if 0
mdct_info *faad_mdct_init(unsigned short N)
{
    unsigned short k;
    unsigned short N_idx;
    real_t cangle, sangle, c, s, cold;

    mdct_info *mdct = (mdct_info*)faad_malloc(sizeof(mdct_info));

    assert(N % 8 == 0);

    mdct->N = N;
    mdct->sincos = (complex_t*)faad_malloc(N/4*sizeof(complex_t));

    N_idx = map_N_to_idx(N);

    cangle = const_tab[N_idx][1];
    sangle = const_tab[N_idx][2];
    c = const_tab[N_idx][3];
    s = const_tab[N_idx][4];


    /* (co)sine table build using recurrence relations */
    /* this can also be done using static table lookup or */
    /* some form of interpolation */
    for (k = 0; k < N/4; k++)
    {
        RE(mdct->sincos[k]) = c; //MUL_C_C(c,scale);
        IM(mdct->sincos[k]) = s; //MUL_C_C(s,scale);

        cold = c;
        c = MUL_F(c,cangle) - MUL_F(s,sangle);
        s = MUL_F(s,cangle) + MUL_F(cold,sangle);
    }

    /* initialise fft */
    mdct->cfft = cffti(N/4);

    return mdct;
}
#else
//Shawn 2004.12.9
/* multiply with fractional shift */
static INLINE real_t MUL_F_MDCT(real_t A, real_t B)
{
    return ((real_t)(((long long)A*B)>>32)) << (FRAC_SIZE-FRAC_BITS);
}

mdct_info *faad_mdct_init(unsigned short N)
{
    unsigned short k;
    unsigned short N_idx;
    real_t cangle, sangle, c, s, cold;

    mdct_info *mdct = (mdct_info*)kmalloc(sizeof(mdct_info), GFP_KERNEL);//(mdct_info*)faad_malloc(sizeof(mdct_info));

    assert(N % 8 == 0);

    mdct->N = N;
    mdct->sincos = (complex_t*)kmalloc(N/4*sizeof(complex_t), GFP_KERNEL);//(complex_t*)faad_malloc(N/4*sizeof(complex_t));

    N_idx = map_N_to_idx(N);

    cangle = const_tab[N_idx][1];
    sangle = const_tab[N_idx][2];
    c = const_tab[N_idx][3];
    s = const_tab[N_idx][4];


    /* (co)sine table build using recurrence relations */
    /* this can also be done using static table lookup or */
    /* some form of interpolation */
    for (k = 0; k < N/4; k++)
    {
        RE(mdct->sincos[k]) = c; //MUL_C_C(c,scale);
        IM(mdct->sincos[k]) = s; //MUL_C_C(s,scale);

        cold = c;
        c = MUL_F_MDCT(c,cangle) - MUL_F_MDCT(s,sangle);
        s = MUL_F_MDCT(s,cangle) + MUL_F_MDCT(cold,sangle);
    }

    /* initialise fft */
    mdct->cfft = cffti(N/4);

    return mdct;
}
#endif

#if 0
void faad_mdct_end(mdct_info *mdct)
{
    if (mdct != NULL)
    {
        cfftu(mdct->cfft);

        if (mdct->sincos) faad_free(mdct->sincos);

        faad_free(mdct);
    }
}
#else
//Shawn 2004.12.9
void faad_mdct_end(mdct_info *mdct)
{
    if (mdct != NULL)
    {
        cfftu(mdct->cfft);

        if (mdct->sincos) kfree(mdct->sincos);//faad_free(mdct->sincos);

        kfree(mdct);//faad_free(mdct);
    }
}
#endif

#if 0
void faad_imdct(mdct_info *mdct, real_t *X_in, real_t *X_out)
{
    unsigned short k;

    complex_t x;
    complex_t Z1[512];
    complex_t *sincos = mdct->sincos;

    unsigned short N  = mdct->N;
    unsigned short N2 = N >> 1;
    unsigned short N4 = N >> 2;
    unsigned short N8 = N >> 3;

#ifdef PROFILE
    int64_t count1, count2 = faad_get_ts();
#endif

    /* pre-IFFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        ComplexMult(&IM(Z1[k]), &RE(Z1[k]),
            X_in[2*k], X_in[N2 - 1 - 2*k], RE(sincos[k]), IM(sincos[k]));
    }

#ifdef PROFILE
    count1 = faad_get_ts();
#endif

    /* complex IFFT, any non-scaling FFT can be used here */
    cfftb(mdct->cfft, Z1);

#ifdef PROFILE
    count1 = faad_get_ts() - count1;
#endif

    /* post-IFFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        RE(x) = RE(Z1[k]);
        IM(x) = IM(Z1[k]);
        ComplexMult(&IM(Z1[k]), &RE(Z1[k]),
            IM(x), RE(x), RE(sincos[k]), IM(sincos[k]));
    }

    /* reordering */
    for (k = 0; k < N8; k+=2)
    {
        X_out[              2*k] =  IM(Z1[N8 +     k]);
        X_out[          2 + 2*k] =  IM(Z1[N8 + 1 + k]);

        X_out[          1 + 2*k] = -RE(Z1[N8 - 1 - k]);
        X_out[          3 + 2*k] = -RE(Z1[N8 - 2 - k]);

        X_out[N4 +          2*k] =  RE(Z1[         k]);
        X_out[N4 +    + 2 + 2*k] =  RE(Z1[     1 + k]);

        X_out[N4 +      1 + 2*k] = -IM(Z1[N4 - 1 - k]);
        X_out[N4 +      3 + 2*k] = -IM(Z1[N4 - 2 - k]);

        X_out[N2 +          2*k] =  RE(Z1[N8 +     k]);
        X_out[N2 +    + 2 + 2*k] =  RE(Z1[N8 + 1 + k]);

        X_out[N2 +      1 + 2*k] = -IM(Z1[N8 - 1 - k]);
        X_out[N2 +      3 + 2*k] = -IM(Z1[N8 - 2 - k]);

        X_out[N2 + N4 +     2*k] = -IM(Z1[         k]);
        X_out[N2 + N4 + 2 + 2*k] = -IM(Z1[     1 + k]);

        X_out[N2 + N4 + 1 + 2*k] =  RE(Z1[N4 - 1 - k]);
        X_out[N2 + N4 + 3 + 2*k] =  RE(Z1[N4 - 2 - k]);
    }

#ifdef PROFILE
    count2 = faad_get_ts() - count2;
    mdct->fft_cycles += count1;
    mdct->cycles += (count2 - count1);
#endif
}
#else
#ifndef ARM_ASM
//Shawn 2004.8.23
#define N2 (N >> 1)
#define N4 (N >> 2)
#define N8 (N >> 3)

void faad_imdct(mdct_info *mdct, real_t *X_in, real_t *X_out)
{
    int k;
	complex_t *Z1;
    real_t *sincos_p = (real_t *)mdct->sincos;
	real_t *X_in_p0, *X_in_p1;
	real_t *Z1_p;
	real_t *Z1_p0, *Z1_p1, *Z1_p2, *Z1_p3;
	real_t *X_out_p0;
	real_t *Z1_tmp;
	//complex_t *sincos = mdct->sincos;

    int N  = mdct->N;
    //int N2 = N >> 1;
    //int N4 = N >> 2;
    //int N8 = N >> 3;

	Z1_tmp = (real_t *)X_out;

	X_in_p0 = X_in;
	X_in_p1 = &X_in[N2 - 1];
	Z1_p = (real_t *)&Z1_tmp[0];

    /* pre-IFFT complex multiplication */
    for (k = N4; k != 0; k --)
    {
		real_t x1, x2, c1, c2;

#ifdef _WIN32
		x1 = *X_in_p0;
		X_in_p0 = X_in_p0 + 2;
		x2 = *X_in_p1;
		X_in_p1 = X_in_p1 - 2;

		c1 = sincos_p[0];
		sincos_p = sincos_p + 1;
		c2 = sincos_p[0];
		sincos_p = sincos_p + 1;

        ComplexMult(Z1_p + 1, Z1_p, x1, x2, c1, c2);
		Z1_p = Z1_p + 2;
#else //ARM
//Shawn 2004.8.26
		real_t tmp, tmpy1, tmpy2;

		__asm
		{
			LDR		x1, [X_in_p0], #8
			LDR		x2, [X_in_p1], #-8
			LDR		c1, [sincos_p], #4
			LDR		c2, [sincos_p], #4

			smull	tmp, tmpy1, c1, x1
			smlal	tmp, tmpy1, c2, x2
			rsb		x1, x1, #0
			smull	tmp, tmpy2, c1, x2
			smlal	tmp, tmpy2, c2, x1
			MOV		tmpy1, tmpy1 << (FRAC_SIZE-FRAC_BITS)
			MOV		tmpy2, tmpy2 << (FRAC_SIZE-FRAC_BITS)
			STR		tmpy2, [Z1_p], #4
			STR		tmpy1, [Z1_p], #4
		}
#endif
    }

    /* complex IFFT, any non-scaling FFT can be used here */
    cfftb(mdct->cfft, (complex_t *)Z1_tmp);

    /* post-IFFT complex multiplication */
	sincos_p = (real_t *)mdct->sincos;

	Z1 = mdct->cfft->work;

    Z1_p = (real_t *)&Z1[0];

	for (k = N4; k != 0; k --)
    {
		real_t x1, x2, c1, c2;

#ifdef _WIN32
		c1 = sincos_p[0];
		sincos_p = sincos_p + 1;
		c2 = sincos_p[0];
		sincos_p = sincos_p + 1;

		x2 = Z1_p[0];
        x1 = Z1_p[1];

        ComplexMult(Z1_p + 1, Z1_p, x1, x2, c1, c2);
		Z1_p = Z1_p + 2;
#else //ARM
//Shawn 2004.8.26
		real_t tmp, tmpy1, tmpy2;

		__asm
		{
			LDR		c1, [sincos_p], #4
			LDR		c2, [sincos_p], #4
			LDR		x2, [Z1_p]
			LDR		x1, [Z1_p, #4]

			smull	tmp, tmpy1, c1, x1
			smlal	tmp, tmpy1, c2, x2
			rsb	x1, x1, #0
			smull	tmp, tmpy2, c1, x2
			smlal	tmp, tmpy2, c2, x1
			MOV		tmpy1, tmpy1 << (FRAC_SIZE-FRAC_BITS)
			MOV		tmpy2, tmpy2 << (FRAC_SIZE-FRAC_BITS)
			STR		tmpy2, [Z1_p], #4
			STR		tmpy1, [Z1_p], #4
		}
#endif
    }

	Z1_p0 = (real_t *)&Z1[N8];
	Z1_p1 = (real_t *)&Z1[N8 - 2];
	Z1_p2 = (real_t *)&Z1[0];
	Z1_p3 = (real_t *)&Z1[N4 - 2];
	X_out_p0 = (real_t *)X_out;

    /* reordering */
    for (k = N8; k != 0; k -= 2)
    {
		real_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15;

        r2 = Z1_p1[2];
        r3 = Z1_p1[0];
		r0 = Z1_p0[1];
        r1 = Z1_p0[3];
		r2 = -r2;
		r3 = -r3;

		X_out_p0[0] = r0;
		X_out_p0[2] = r1;
		X_out_p0[1] = r2;
        X_out_p0[3] = r3;

		r10 = Z1_p1[3];
        r11 = Z1_p1[1];
		r8 = Z1_p0[0];
        r9 = Z1_p0[2];
		r10 = -r10;
		r11 = -r11;

		Z1_p0 = Z1_p0 + 4;
		Z1_p1 = Z1_p1 - 4;

		X_out_p0[N2] = r8;
        X_out_p0[N2 + 2] = r9;
		X_out_p0[N2 + 1] = r10;
        X_out_p0[N2 + 3] = r11;

        r6 = Z1_p3[3];
        r7 = Z1_p3[1];
		r4 = Z1_p2[0];
        r5 = Z1_p2[2];
		r6 = -r6;
		r7 = -r7;

		X_out_p0[N4] = r4;
		X_out_p0[N4 + 1] = r6;
        X_out_p0[N4 + 2] = r5;
        X_out_p0[N4 + 3] = r7;

        r12 = Z1_p2[1];
        r13 = Z1_p2[3];
		r14 = Z1_p3[2];
        r15 = Z1_p3[0];
		r12 = -r12;
		r13 = -r13;

		Z1_p2 = Z1_p2 + 4;
		Z1_p3 = Z1_p3 - 4;

		X_out_p0[N2 + N4] = r12;
        X_out_p0[N2 + N4 + 2] = r13;
		X_out_p0[N2 + N4 + 1] = r14;
        X_out_p0[N2 + N4 + 3] = r15;

		X_out_p0 = X_out_p0 + 4;
    }
}
#endif
#endif

#ifdef LTP_DEC
void faad_mdct(mdct_info *mdct, real_t *X_in, real_t *X_out)
{
    unsigned short k;

    complex_t x;
    complex_t Z1[512];
    complex_t *sincos = mdct->sincos;

    unsigned short N  = mdct->N;
    unsigned short N2 = N >> 1;
    unsigned short N4 = N >> 2;
    unsigned short N8 = N >> 3;

#ifndef FIXED_POINT
	real_t scale = REAL_CONST(N);
#else
	real_t scale = REAL_CONST(4.0/N);
#endif

    /* pre-FFT complex multiplication */
    for (k = 0; k < N8; k++)
    {
        unsigned short n = k << 1;
        RE(x) = X_in[N - N4 - 1 - n] + X_in[N - N4 +     n];
        IM(x) = X_in[    N4 +     n] - X_in[    N4 - 1 - n];

        ComplexMult(&RE(Z1[k]), &IM(Z1[k]),
            RE(x), IM(x), RE(sincos[k]), IM(sincos[k]));

        RE(Z1[k]) = MUL_R(RE(Z1[k]), scale);
        IM(Z1[k]) = MUL_R(IM(Z1[k]), scale);

        RE(x) =  X_in[N2 - 1 - n] - X_in[        n];
        IM(x) =  X_in[N2 +     n] + X_in[N - 1 - n];

        ComplexMult(&RE(Z1[k + N8]), &IM(Z1[k + N8]),
            RE(x), IM(x), RE(sincos[k + N8]), IM(sincos[k + N8]));

        RE(Z1[k + N8]) = MUL_R(RE(Z1[k + N8]), scale);
        IM(Z1[k + N8]) = MUL_R(IM(Z1[k + N8]), scale);
    }

    /* complex FFT, any non-scaling FFT can be used here  */
    cfftf(mdct->cfft, Z1);

    /* post-FFT complex multiplication */
    for (k = 0; k < N4; k++)
    {
        unsigned short n = k << 1;
        ComplexMult(&RE(x), &IM(x),
            RE(Z1[k]), IM(Z1[k]), RE(sincos[k]), IM(sincos[k]));

        X_out[         n] = -RE(x);
        X_out[N2 - 1 - n] =  IM(x);
        X_out[N2 +     n] = -IM(x);
        X_out[N  - 1 - n] =  RE(x);
    }
}
#endif
