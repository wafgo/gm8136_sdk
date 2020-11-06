/*
 * Algorithmically based on Fortran-77 FFTPACK
 * by Paul N. Swarztrauber(Version 4, 1985).
 *
 * Does even sized fft only
 */

/* isign is +1 for backward and -1 for forward transforms */
#include <linux/slab.h>
#include "common.h"
#include "structs.h"

//#include <stdlib.h>

#include "cfft.h"
#include "cfft_tab.h"

#ifndef ARM_ASM
/* static function declarations */
#if 0
static void passf2pos(const unsigned short ido, const unsigned short l1, const complex_t *cc,
                      complex_t *ch, const complex_t *wa);
static void passf4pos(const unsigned short ido, const unsigned short l1, const complex_t *cc, complex_t *ch,
                      const complex_t *wa1, const complex_t *wa2, const complex_t *wa3);
#else
//Shawn 2004.8.23
static void passf2pos(int ido, int l1, real_t *cc, real_t *ch, real_t *wa);
static void passf4pos(int ido, int l1, complex_t *cc, complex_t *ch, complex_t *wa1);
#endif
static void passf2neg(const unsigned short ido, const unsigned short l1, const complex_t *cc,
                      complex_t *ch, const complex_t *wa);
static void passf3(const unsigned short ido, const unsigned short l1, const complex_t *cc,
                   complex_t *ch, const complex_t *wa1, const complex_t *wa2, const signed char isign);

static void passf4neg(const unsigned short ido, const unsigned short l1, const complex_t *cc, complex_t *ch,
                      const complex_t *wa1, const complex_t *wa2, const complex_t *wa3);
static void passf5(const unsigned short ido, const unsigned short l1, const complex_t *cc, complex_t *ch,
                   const complex_t *wa1, const complex_t *wa2, const complex_t *wa3,
                   const complex_t *wa4, const signed char isign);
INLINE void cfftf1(unsigned short n, complex_t *c, complex_t *ch,
                   const unsigned short *ifac, const complex_t *wa, const signed char isign);
static void cffti1(unsigned short n, complex_t *wa, unsigned short *ifac);


/*----------------------------------------------------------------------
   passf2, passf3, passf4, passf5. Complex FFT passes fwd and bwd.
  ----------------------------------------------------------------------*/
#if 0
static void passf2pos(const unsigned short ido, const unsigned short l1, const complex_t *cc,
                      complex_t *ch, const complex_t *wa)
{
    unsigned short i, k, ah, ac;

    if (ido == 1)
    {
        for (k = 0; k < l1; k++)
        {
            ah = 2*k;
            ac = 4*k;

            RE(ch[ah])    = RE(cc[ac]) + RE(cc[ac+1]);
            RE(ch[ah+l1]) = RE(cc[ac]) - RE(cc[ac+1]);
            IM(ch[ah])    = IM(cc[ac]) + IM(cc[ac+1]);
            IM(ch[ah+l1]) = IM(cc[ac]) - IM(cc[ac+1]);
        }
    } else {
        for (k = 0; k < l1; k++)
        {
            ah = k*ido;
            ac = 2*k*ido;

            for (i = 0; i < ido; i++)
            {
                complex_t t2;

                RE(ch[ah+i]) = RE(cc[ac+i]) + RE(cc[ac+i+ido]);
                RE(t2)       = RE(cc[ac+i]) - RE(cc[ac+i+ido]);

                IM(ch[ah+i]) = IM(cc[ac+i]) + IM(cc[ac+i+ido]);
                IM(t2)       = IM(cc[ac+i]) - IM(cc[ac+i+ido]);

#if 1
                ComplexMult(&IM(ch[ah+i+l1*ido]), &RE(ch[ah+i+l1*ido]),
                    IM(t2), RE(t2), RE(wa[i]), IM(wa[i]));
#else
                ComplexMult(&RE(ch[ah+i+l1*ido]), &IM(ch[ah+i+l1*ido]),
                    RE(t2), IM(t2), RE(wa[i]), IM(wa[i]));
#endif
            }
        }
    }
}
#else
//Shawn 2004.8.23
static void passf2pos(int ido, int l1, real_t *cc, real_t *ch, real_t *wa)
{
    int i, k;

    if (ido == 1)
    {
		int ah, ac;

		complex_t *cc_pp = (complex_t *)cc;
		complex_t *ch_pp = (complex_t *)ch;

        for (k = 0; k < l1; k++)
        {
            ah = 2*k;
            ac = 4*k;

            RE(ch_pp[ah])    = RE(cc_pp[ac]) + RE(cc_pp[ac+1]);
            RE(ch_pp[ah+l1]) = RE(cc_pp[ac]) - RE(cc_pp[ac+1]);
            IM(ch_pp[ah])    = IM(cc_pp[ac]) + IM(cc_pp[ac+1]);
            IM(ch_pp[ah+l1]) = IM(cc_pp[ac]) - IM(cc_pp[ac+1]);
        }
    }
	else
	{
		real_t *wa_p;
		real_t *cc_p;
		real_t *ch_p;

        for (k = l1; k != 0; k --)
        {
			real_t tmp, tmpy1, tmpy2;
			real_t reg_wa0, reg_wa1;
			real_t cc00, cc01, cc10, cc11;

			wa_p = wa;//???? maybe error
			cc_p = cc;//???? maybe error
			ch_p = ch;//???? maybe error

            for (i = ido; i != 0; i --)
            {
                real_t t20, t21, neg_t21;


				cc10 = ((real_t *)(cc_p+(ido << 1)))[0];
				cc00 = ((real_t *)(cc_p))[0];
				cc_p = cc_p + 1;

				cc11 = ((real_t *)(cc_p+(ido << 1)))[0];
				cc01 = ((real_t *)(cc_p))[0];
				cc_p = cc_p + 1;

				ch_p[0] = cc00 +cc10;
                ch_p[1] = cc01 + cc11;

				t21       = cc01 - cc11;
				t20       = cc00 - cc10;

#ifdef _WIN32
				reg_wa0 = wa_p[0];
				wa_p = wa_p + 1;
				reg_wa1 = wa_p[0];
				wa_p = wa_p + 1;

                ComplexMult(&IM(((real_t *)(ch_p+(l1*ido << 1)))), &RE(((real_t *)(ch_p+(l1*ido << 1)))),
                    t21, t20, reg_wa0, reg_wa1);
#else //ARM
//Shawn 2004.8.23
				__asm
				{
					LDR		reg_wa0, [wa_p], #4
					LDR		reg_wa1, [wa_p], #4
					SMULL	tmp, tmpy1, t21, reg_wa0
					SMLAL	tmp, tmpy1, t20, reg_wa1
					RSB		neg_t21, t21, #0
					SMULL	tmp, tmpy2, t20, reg_wa0
					SMLAL	tmp, tmpy2, neg_t21, reg_wa1
				}

				IM(((real_t *)(ch_p+(l1*ido << 1)))) = tmpy1 << (FRAC_SIZE-FRAC_BITS);
				RE(((real_t *)(ch_p+(l1*ido << 1)))) = tmpy2 << (FRAC_SIZE-FRAC_BITS);
				//can be improved but need pattern to test Shawn 2004.8.26
#endif
				ch_p = ch_p + 2;
            }
        }
    }
}
#endif

//Shawn 2004.9.23
static INLINE void passf2pos_long(real_t *cc, real_t *ch, real_t *wa)
{
    int i;

	real_t tmp, tmpy1, tmpy2;
	real_t reg_wa0, reg_wa1;
	real_t cc00, cc01, cc10, cc11;

	for (i = 256; i != 0; i --)
	{
#ifdef _WIN32
		real_t t20, t21;

		cc10 = *(cc + 512);
		cc00 = *cc++;

		cc11 = *(cc + 512);
		cc01 = *cc++;

		*ch++ = cc00 + cc10;
		*ch++ = cc01 + cc11;

		t21 = cc01 - cc11;
		t20 = cc00 - cc10;

		reg_wa0 = *wa++;
		reg_wa1 = *wa++;

		ComplexMult((ch+511), (ch+510), t21, t20, reg_wa0, reg_wa1);
#else //ARM
//Shawn 2004.8.23
		real_t t20, t21, neg_t21;
		real_t reg_r0, reg_r1;

		cc10 = *(cc + 512);
		cc00 = *cc++;

		cc11 = *(cc + 512);
		cc01 = *cc++;

		reg_r0 = cc00 + cc10;
		reg_r1 = cc01 + cc11;

		*ch++ = reg_r0;
		*ch++ = reg_r1;

		t21 = cc01 - cc11;
		t20 = cc00 - cc10;

		__asm
		{
			LDR		reg_wa0, [wa], #4
			LDR		reg_wa1, [wa], #4
			SMULL	tmp, tmpy1, reg_wa0, t21
			SMLAL	tmp, tmpy1, reg_wa1, t20
			RSB		neg_t21, t21, #0
			SMULL	tmp, tmpy2, reg_wa0, t20
			SMLAL	tmp, tmpy2, reg_wa1, neg_t21
			MOV		*(ch+511), tmpy1 << (FRAC_SIZE-FRAC_BITS)
			MOV		*(ch+510), tmpy2 << (FRAC_SIZE-FRAC_BITS)
		}

#endif
    }
}

static void passf2neg(const unsigned short ido, const unsigned short l1, const complex_t *cc,
                      complex_t *ch, const complex_t *wa)
{
    unsigned short i, k, ah, ac;

    if (ido == 1)
    {
        for (k = 0; k < l1; k++)
        {
            ah = 2*k;
            ac = 4*k;

            RE(ch[ah])    = RE(cc[ac]) + RE(cc[ac+1]);
            RE(ch[ah+l1]) = RE(cc[ac]) - RE(cc[ac+1]);
            IM(ch[ah])    = IM(cc[ac]) + IM(cc[ac+1]);
            IM(ch[ah+l1]) = IM(cc[ac]) - IM(cc[ac+1]);
        }
    } else {
        for (k = 0; k < l1; k++)
        {
            ah = k*ido;
            ac = 2*k*ido;

            for (i = 0; i < ido; i++)
            {
                complex_t t2;

                RE(ch[ah+i]) = RE(cc[ac+i]) + RE(cc[ac+i+ido]);
                RE(t2)       = RE(cc[ac+i]) - RE(cc[ac+i+ido]);

                IM(ch[ah+i]) = IM(cc[ac+i]) + IM(cc[ac+i+ido]);
                IM(t2)       = IM(cc[ac+i]) - IM(cc[ac+i+ido]);

#if 1
                ComplexMult(&RE(ch[ah+i+l1*ido]), &IM(ch[ah+i+l1*ido]),
                    RE(t2), IM(t2), RE(wa[i]), IM(wa[i]));
#else
                ComplexMult(&IM(ch[ah+i+l1*ido]), &RE(ch[ah+i+l1*ido]),
                    IM(t2), RE(t2), RE(wa[i]), IM(wa[i]));
#endif
            }
        }
    }
}


static void passf3(const unsigned short ido, const unsigned short l1, const complex_t *cc,
                   complex_t *ch, const complex_t *wa1, const complex_t *wa2,
                   const signed char isign)
{
    static real_t taur = FRAC_CONST(-0.5);
    static real_t taui = FRAC_CONST(0.866025403784439);
    unsigned short i, k, ac, ah;
    complex_t c2, c3, d2, d3, t2;

    if (ido == 1)
    {
        if (isign == 1)
        {
            for (k = 0; k < l1; k++)
            {
                ac = 3*k+1;
                ah = k;

                RE(t2) = RE(cc[ac]) + RE(cc[ac+1]);
                IM(t2) = IM(cc[ac]) + IM(cc[ac+1]);
                RE(c2) = RE(cc[ac-1]) + MUL_F(RE(t2),taur);
                IM(c2) = IM(cc[ac-1]) + MUL_F(IM(t2),taur);

                RE(ch[ah]) = RE(cc[ac-1]) + RE(t2);
                IM(ch[ah]) = IM(cc[ac-1]) + IM(t2);

                RE(c3) = MUL_F((RE(cc[ac]) - RE(cc[ac+1])), taui);
                IM(c3) = MUL_F((IM(cc[ac]) - IM(cc[ac+1])), taui);

                RE(ch[ah+l1]) = RE(c2) - IM(c3);
                IM(ch[ah+l1]) = IM(c2) + RE(c3);
                RE(ch[ah+2*l1]) = RE(c2) + IM(c3);
                IM(ch[ah+2*l1]) = IM(c2) - RE(c3);
            }
        } else {
            for (k = 0; k < l1; k++)
            {
                ac = 3*k+1;
                ah = k;

                RE(t2) = RE(cc[ac]) + RE(cc[ac+1]);
                IM(t2) = IM(cc[ac]) + IM(cc[ac+1]);
                RE(c2) = RE(cc[ac-1]) + MUL_F(RE(t2),taur);
                IM(c2) = IM(cc[ac-1]) + MUL_F(IM(t2),taur);

                RE(ch[ah]) = RE(cc[ac-1]) + RE(t2);
                IM(ch[ah]) = IM(cc[ac-1]) + IM(t2);

                RE(c3) = MUL_F((RE(cc[ac]) - RE(cc[ac+1])), taui);
                IM(c3) = MUL_F((IM(cc[ac]) - IM(cc[ac+1])), taui);

                RE(ch[ah+l1]) = RE(c2) + IM(c3);
                IM(ch[ah+l1]) = IM(c2) - RE(c3);
                RE(ch[ah+2*l1]) = RE(c2) - IM(c3);
                IM(ch[ah+2*l1]) = IM(c2) + RE(c3);
            }
        }
    } else {
        if (isign == 1)
        {
            for (k = 0; k < l1; k++)
            {
                for (i = 0; i < ido; i++)
                {
                    ac = i + (3*k+1)*ido;
                    ah = i + k * ido;

                    RE(t2) = RE(cc[ac]) + RE(cc[ac+ido]);
                    RE(c2) = RE(cc[ac-ido]) + MUL_F(RE(t2),taur);
                    IM(t2) = IM(cc[ac]) + IM(cc[ac+ido]);
                    IM(c2) = IM(cc[ac-ido]) + MUL_F(IM(t2),taur);

                    RE(ch[ah]) = RE(cc[ac-ido]) + RE(t2);
                    IM(ch[ah]) = IM(cc[ac-ido]) + IM(t2);

                    RE(c3) = MUL_F((RE(cc[ac]) - RE(cc[ac+ido])), taui);
                    IM(c3) = MUL_F((IM(cc[ac]) - IM(cc[ac+ido])), taui);

                    RE(d2) = RE(c2) - IM(c3);
                    IM(d3) = IM(c2) - RE(c3);
                    RE(d3) = RE(c2) + IM(c3);
                    IM(d2) = IM(c2) + RE(c3);

#if 1
                    ComplexMult(&IM(ch[ah+l1*ido]), &RE(ch[ah+l1*ido]),
                        IM(d2), RE(d2), RE(wa1[i]), IM(wa1[i]));
                    ComplexMult(&IM(ch[ah+2*l1*ido]), &RE(ch[ah+2*l1*ido]),
                        IM(d3), RE(d3), RE(wa2[i]), IM(wa2[i]));
#else
                    ComplexMult(&RE(ch[ah+l1*ido]), &IM(ch[ah+l1*ido]),
                        RE(d2), IM(d2), RE(wa1[i]), IM(wa1[i]));
                    ComplexMult(&RE(ch[ah+2*l1*ido]), &IM(ch[ah+2*l1*ido]),
                        RE(d3), IM(d3), RE(wa2[i]), IM(wa2[i]));
#endif
                }
            }
        } else {
            for (k = 0; k < l1; k++)
            {
                for (i = 0; i < ido; i++)
                {
                    ac = i + (3*k+1)*ido;
                    ah = i + k * ido;

                    RE(t2) = RE(cc[ac]) + RE(cc[ac+ido]);
                    RE(c2) = RE(cc[ac-ido]) + MUL_F(RE(t2),taur);
                    IM(t2) = IM(cc[ac]) + IM(cc[ac+ido]);
                    IM(c2) = IM(cc[ac-ido]) + MUL_F(IM(t2),taur);

                    RE(ch[ah]) = RE(cc[ac-ido]) + RE(t2);
                    IM(ch[ah]) = IM(cc[ac-ido]) + IM(t2);

                    RE(c3) = MUL_F((RE(cc[ac]) - RE(cc[ac+ido])), taui);
                    IM(c3) = MUL_F((IM(cc[ac]) - IM(cc[ac+ido])), taui);

                    RE(d2) = RE(c2) + IM(c3);
                    IM(d3) = IM(c2) + RE(c3);
                    RE(d3) = RE(c2) - IM(c3);
                    IM(d2) = IM(c2) - RE(c3);

#if 1
                    ComplexMult(&RE(ch[ah+l1*ido]), &IM(ch[ah+l1*ido]),
                        RE(d2), IM(d2), RE(wa1[i]), IM(wa1[i]));
                    ComplexMult(&RE(ch[ah+2*l1*ido]), &IM(ch[ah+2*l1*ido]),
                        RE(d3), IM(d3), RE(wa2[i]), IM(wa2[i]));
#else
                    ComplexMult(&IM(ch[ah+l1*ido]), &RE(ch[ah+l1*ido]),
                        IM(d2), RE(d2), RE(wa1[i]), IM(wa1[i]));
                    ComplexMult(&IM(ch[ah+2*l1*ido]), &RE(ch[ah+2*l1*ido]),
                        IM(d3), RE(d3), RE(wa2[i]), IM(wa2[i]));
#endif
                }
            }
        }
    }
}

#if 0
static void passf4pos(const unsigned short ido, const unsigned short l1, const complex_t *cc,
                      complex_t *ch, const complex_t *wa1, const complex_t *wa2,
                      const complex_t *wa3)
{
    unsigned short i, k, ac, ah;

    if (ido == 1)
    {
        for (k = 0; k < l1; k++)
        {
            complex_t t1, t2, t3, t4;

            ac = 4*k;
            ah = k;

            RE(t2) = RE(cc[ac])   + RE(cc[ac+2]);
            RE(t1) = RE(cc[ac])   - RE(cc[ac+2]);
            IM(t2) = IM(cc[ac])   + IM(cc[ac+2]);
            IM(t1) = IM(cc[ac])   - IM(cc[ac+2]);
            RE(t3) = RE(cc[ac+1]) + RE(cc[ac+3]);
            IM(t4) = RE(cc[ac+1]) - RE(cc[ac+3]);
            IM(t3) = IM(cc[ac+3]) + IM(cc[ac+1]);
            RE(t4) = IM(cc[ac+3]) - IM(cc[ac+1]);

            RE(ch[ah])      = RE(t2) + RE(t3);
            RE(ch[ah+2*l1]) = RE(t2) - RE(t3);

            IM(ch[ah])      = IM(t2) + IM(t3);
            IM(ch[ah+2*l1]) = IM(t2) - IM(t3);

            RE(ch[ah+l1])   = RE(t1) + RE(t4);
            RE(ch[ah+3*l1]) = RE(t1) - RE(t4);

            IM(ch[ah+l1])   = IM(t1) + IM(t4);
            IM(ch[ah+3*l1]) = IM(t1) - IM(t4);
        }
    } else {
        for (k = 0; k < l1; k++)
        {
            ac = 4*k*ido;
            ah = k*ido;

            for (i = 0; i < ido; i++)
            {
                complex_t c2, c3, c4, t1, t2, t3, t4;

                RE(t2) = RE(cc[ac+i]) + RE(cc[ac+i+2*ido]);
                RE(t1) = RE(cc[ac+i]) - RE(cc[ac+i+2*ido]);
                IM(t2) = IM(cc[ac+i]) + IM(cc[ac+i+2*ido]);
                IM(t1) = IM(cc[ac+i]) - IM(cc[ac+i+2*ido]);
                RE(t3) = RE(cc[ac+i+ido]) + RE(cc[ac+i+3*ido]);
                IM(t4) = RE(cc[ac+i+ido]) - RE(cc[ac+i+3*ido]);
                IM(t3) = IM(cc[ac+i+3*ido]) + IM(cc[ac+i+ido]);
                RE(t4) = IM(cc[ac+i+3*ido]) - IM(cc[ac+i+ido]);

                RE(c2) = RE(t1) + RE(t4);
                RE(c4) = RE(t1) - RE(t4);

                IM(c2) = IM(t1) + IM(t4);
                IM(c4) = IM(t1) - IM(t4);

                RE(ch[ah+i]) = RE(t2) + RE(t3);
                RE(c3)       = RE(t2) - RE(t3);

                IM(ch[ah+i]) = IM(t2) + IM(t3);
                IM(c3)       = IM(t2) - IM(t3);

#if 1
                ComplexMult(&IM(ch[ah+i+l1*ido]), &RE(ch[ah+i+l1*ido]),
                    IM(c2), RE(c2), RE(wa1[i]), IM(wa1[i]));
                ComplexMult(&IM(ch[ah+i+2*l1*ido]), &RE(ch[ah+i+2*l1*ido]),
                    IM(c3), RE(c3), RE(wa2[i]), IM(wa2[i]));
                ComplexMult(&IM(ch[ah+i+3*l1*ido]), &RE(ch[ah+i+3*l1*ido]),
                    IM(c4), RE(c4), RE(wa3[i]), IM(wa3[i]));
#else
                ComplexMult(&RE(ch[ah+i+l1*ido]), &IM(ch[ah+i+l1*ido]),
                    RE(c2), IM(c2), RE(wa1[i]), IM(wa1[i]));
                ComplexMult(&RE(ch[ah+i+2*l1*ido]), &IM(ch[ah+i+2*l1*ido]),
                    RE(c3), IM(c3), RE(wa2[i]), IM(wa2[i]));
                ComplexMult(&RE(ch[ah+i+3*l1*ido]), &IM(ch[ah+i+3*l1*ido]),
                    RE(c4), IM(c4), RE(wa3[i]), IM(wa3[i]));
#endif
            }
        }
    }
}
#else
#if 0
//Shawn 2004.8.26
static void passf4pos(int ido, int l1, complex_t *cc,
                      complex_t *ch, complex_t *wa1, complex_t *wa2,
                      complex_t *wa3)
{
    int i, k, ac, ah;
	real_t *cc_p;

    if (ido == 1)
    {
		cc_p = (real_t *)cc;

        for (k = 0; k < l1; k++)
        {
            real_t t10, t20, t30, t40, t11, t21, t31, t41;
			real_t cc00, cc01, cc10, cc11, cc20, cc21, cc30, cc31;
			real_t ch00, ch01, ch10, ch11, ch20, ch21, ch30, ch31;

            ah = k;

#ifdef _WIN32
			cc00 = cc_p[0];
			cc_p = cc_p + 1;
			cc01 = cc_p[0];
			cc_p = cc_p + 1;
			cc10 = cc_p[0];
			cc_p = cc_p + 1;
			cc11 = cc_p[0];
			cc_p = cc_p + 1;
			cc20 = cc_p[0];
			cc_p = cc_p + 1;
			cc21 = cc_p[0];
			cc_p = cc_p + 1;
			cc30 = cc_p[0];
			cc_p = cc_p + 1;
			cc31 = cc_p[0];
			cc_p = cc_p + 1;


#else //ARM
//Shawn 2004.8.27
			__asm
			{
				LDR		cc00, [cc_p], #4
				LDR		cc01, [cc_p], #4
				LDR		cc10, [cc_p], #4
				LDR		cc11, [cc_p], #4
				LDR		cc20, [cc_p], #4
				LDR		cc21, [cc_p], #4
				LDR		cc30, [cc_p], #4
				LDR		cc31, [cc_p], #4
			}

#endif

			t20 = cc00 + cc20;
            t10 = cc00 - cc20;
            t21 = cc01 + cc21;
            t11 = cc01 - cc21;
            t30 = cc10 + cc30;
            t41 = cc10 - cc30;
            t31 = cc31 + cc11;
            t40 = cc31 - cc11;


			ch00      = t20 + t30;
            ch20 = t20 - t30;

            ch01     = t21 + t31;
            ch21 = t21 - t31;

            ch10  = t10 + t40;
            ch30 = t10 - t40;

            ch11   = t11 + t41;
            ch31 = t11 - t41;

            RE(ch[ah])      = ch00;
            RE(ch[ah+2*l1]) = ch20;

            IM(ch[ah])      = ch01;
            IM(ch[ah+2*l1]) = ch21;

            RE(ch[ah+l1])   = ch10;
            RE(ch[ah+3*l1]) = ch30;

            IM(ch[ah+l1])   = ch11;
            IM(ch[ah+3*l1]) = ch31;
        }
    }
	else
	{
		real_t tmp, tmpy1, tmpy2, neg_x1;
		real_t *wa1_p;
		real_t *wa2_p;
		real_t *wa3_p;

        for (k = 0; k < l1; k++)
        {
            //ac = 4*k*ido;
            ah = k*ido;

			wa1_p = (real_t *)wa1;
			wa2_p = (real_t *)wa2;
			wa3_p = (real_t *)wa3;
			cc_p = (real_t *)cc;

            for (i = 0; i < ido; i++)
            {
                real_t c20, c30, c40, t10, t20, t30, t40, c21, c31, c41, t11, t21, t31, t41;
				real_t wa10, wa11, wa20, wa21, wa30, wa31;
				real_t cc00, cc01, cc10, cc11, cc20, cc21, cc30, cc31;

				cc00 = cc_p[0];
				cc10 = cc_p[2*ido];
				cc20 = cc_p[4*ido];
				cc30 = cc_p[6*ido];
				cc_p = cc_p + 1;
				cc01 = cc_p[0];
				cc11 = cc_p[2*ido];
				cc21 = cc_p[4*ido];
				cc31 = cc_p[6*ido];
				cc_p = cc_p + 1;

                t20 = cc00 + cc20;
                t10 = cc00 - cc20;
                t21 = cc01 + cc21;
                t11 = cc01 - cc21;
                t30 = cc10 + cc30;
                t41 = cc10 - cc30;
                t31 = cc31 + cc11;
                t40 = cc31 - cc11;

                c20 = t10 + t40;
                c40 = t10 - t40;

                c21 = t11 + t41;
                c41 = t11 - t41;


				RE(ch[ah+i]) = t20 + t30;
				c30       = t20 - t30;
				IM(ch[ah+i]) = t21 + t31;
                c31       = t21 - t31;






#ifdef _WIN32
				wa10 = wa1_p[0];
				wa1_p = wa1_p + 1;
				wa11 =  wa1_p[0];
				wa1_p = wa1_p + 1;

				wa20 = wa2_p[0];
				wa2_p = wa2_p + 1;
				wa21 =  wa2_p[0];
				wa2_p = wa2_p + 1;

				wa30 = wa3_p[0];
				wa3_p = wa3_p + 1;
				wa31 =  wa3_p[0];
				wa3_p = wa3_p + 1;

                ComplexMult(&IM(ch[ah+i+l1*ido]), &RE(ch[ah+i+l1*ido]),
                    c21, c20, wa10, wa11);
				ComplexMult(&IM(ch[ah+i+2*l1*ido]), &RE(ch[ah+i+2*l1*ido]),
                    c31, c30, wa20, wa21);
				ComplexMult(&IM(ch[ah+i+3*l1*ido]), &RE(ch[ah+i+3*l1*ido]),
                    c41, c40, wa30, wa31);
#else //ARM
//Shawn 2004.8.26
				__asm
				{
					LDR		wa10, [wa1_p], #4
					LDR		wa11, [wa1_p], #4
					SMULL	tmp, tmpy1, c21, wa10
					SMLAL	tmp, tmpy1, c20, wa11
					RSB		neg_x1, c21, #0
					SMULL	tmp, tmpy2, c20, wa10
					SMLAL	tmp, tmpy2, neg_x1, wa11
				}

				IM(ch[ah+i+l1*ido]) = tmpy1 << (FRAC_SIZE-FRAC_BITS);
				RE(ch[ah+i+l1*ido]) = tmpy2 << (FRAC_SIZE-FRAC_BITS);

				__asm
				{
					LDR		wa20, [wa2_p], #4
					LDR		wa21, [wa2_p], #4
					SMULL	tmp, tmpy1, c31, wa20
					SMLAL	tmp, tmpy1, c30, wa21
					RSB		neg_x1, c31, #0
					SMULL	tmp, tmpy2, c30, wa20
					SMLAL	tmp, tmpy2, neg_x1, wa21
				}

				IM(ch[ah+i+2*l1*ido]) = tmpy1 << (FRAC_SIZE-FRAC_BITS);
				RE(ch[ah+i+2*l1*ido]) = tmpy2 << (FRAC_SIZE-FRAC_BITS);

				__asm
				{
					LDR		wa30, [wa3_p], #4
					LDR		wa31, [wa3_p], #4
					SMULL	tmp, tmpy1, c41, wa30
					SMLAL	tmp, tmpy1, c40, wa31
					RSB		neg_x1, c41, #0
					SMULL	tmp, tmpy2, c40, wa30
					SMLAL	tmp, tmpy2, neg_x1, wa31
				}

				IM(ch[ah+i+3*l1*ido]) = tmpy1 << (FRAC_SIZE-FRAC_BITS);
				RE(ch[ah+i+3*l1*ido]) = tmpy2 << (FRAC_SIZE-FRAC_BITS);
#endif
            }
			cc = cc + 4*ido;
        }
    }
}
#else
//Shawn 2004.9.23
static void passf4pos(int ido, int l1, complex_t *cc,  complex_t *ch, complex_t *wa1)
{
    int i, k;
	real_t *cc_p;
	complex_t *ch_cp;

	{
		real_t tmp, tmpy1, tmpy2, tmpy3, tmpy4, tmpy5, tmpy6, neg_x1;
		real_t *wa1_p;
		real_t *wa2_p;
		real_t *wa3_p;

        for (k = l1; k != 0; k --)
        {
			wa1_p = (real_t *)wa1;

			cc_p = (real_t *)cc;
			ch_cp = ch;

            for (i = ido; i != 0; i --)
            {
                real_t c20, c30, c40, t10, t20, t30, t40, c21, c31, c41, t11, t21, t31, t41;
				real_t wa10, wa11, wa20, wa21, wa30, wa31;
				real_t cc00, cc01, cc10, cc11, cc20, cc21, cc30, cc31;


				cc30 = cc_p[6*ido];
				cc20 = cc_p[4*ido];
				cc10 = cc_p[2*ido];
				cc00 = *cc_p++;

				cc31 = cc_p[6*ido];
				cc21 = cc_p[4*ido];
				cc11 = cc_p[2*ido];
				cc01 = *cc_p++;

                t20 = cc00 + cc20;
                t10 = cc00 - cc20;
                t21 = cc01 + cc21;
                t11 = cc01 - cc21;
                t30 = cc10 + cc30;
                t41 = cc10 - cc30;
                t31 = cc31 + cc11;
                t40 = cc31 - cc11;

				RE(ch_cp[0]) = t20 + t30;
				IM(ch_cp[0]) = t21 + t31;

                c20 = t10 + t40;
                c40 = t10 - t40;

                c21 = t11 + t41;
                c41 = t11 - t41;

				c30       = t20 - t30;
                c31       = t21 - t31;


#ifdef _WIN32
				wa30 = wa1_p[ido << 2];
				wa20 = wa1_p[ido << 1];
				wa10 = *wa1_p++;

				wa31 = wa1_p[ido << 2];
				wa21 = wa1_p[ido << 1];
				wa11 = *wa1_p++;

                ComplexMult(&IM(ch_cp[l1*ido]), &RE(ch_cp[l1*ido]),
                    c21, c20, wa10, wa11);
				ComplexMult(&IM(ch_cp[2*l1*ido]), &RE(ch_cp[2*l1*ido]),
                    c31, c30, wa20, wa21);
				ComplexMult(&IM(ch_cp[3*l1*ido]), &RE(ch_cp[3*l1*ido]),
                    c41, c40, wa30, wa31);

				ch_cp += 1;
#else //ARM
//Shawn 2004.8.26
				wa3_p = (real_t *)(wa1_p + (ido << 2));

				__asm
				{
					LDR		wa30, [wa3_p]
					LDR		wa31, [wa3_p, #4]
					SMULL	tmp, tmpy1, c41, wa30
					SMLAL	tmp, tmpy1, c40, wa31
					RSB		neg_x1, c41, #0
					SMULL	tmp, tmpy2, c40, wa30
					SMLAL	tmp, tmpy2, neg_x1, wa31
				}

				IM(ch_cp[3*l1*ido]) = tmpy1 << (FRAC_SIZE-FRAC_BITS);
				RE(ch_cp[3*l1*ido]) = tmpy2 << (FRAC_SIZE-FRAC_BITS);

				wa2_p = (real_t *)(wa1_p + (ido << 1));

				__asm
				{
					LDR		wa20, [wa2_p]
					LDR		wa21, [wa2_p, #4]
					SMULL	tmp, tmpy3, c31, wa20
					SMLAL	tmp, tmpy3, c30, wa21
					RSB		neg_x1, c31, #0
					SMULL	tmp, tmpy4, c30, wa20
					SMLAL	tmp, tmpy4, neg_x1, wa21
				}

				IM(ch_cp[2*l1*ido]) = tmpy3 << (FRAC_SIZE-FRAC_BITS);
				RE(ch_cp[2*l1*ido]) = tmpy4 << (FRAC_SIZE-FRAC_BITS);

				__asm
				{
					LDR		wa10, [wa1_p], #4
					LDR		wa11, [wa1_p], #4
					SMULL	tmp, tmpy5, c21, wa10
					SMLAL	tmp, tmpy5, c20, wa11
					RSB		neg_x1, c21, #0
					SMULL	tmp, tmpy6, c20, wa10
					SMLAL	tmp, tmpy6, neg_x1, wa11
				}

				IM(ch_cp[l1*ido]) = tmpy5 << (FRAC_SIZE-FRAC_BITS);
				RE(ch_cp[l1*ido]) = tmpy6 << (FRAC_SIZE-FRAC_BITS);

				ch_cp ++;


#endif
            }
			cc = cc + (ido << 2);
			ch += ido;
        }
    }
}
#ifndef ARM_ASM
void passf4pos_new(int ido, int l1, complex_t *cc,  complex_t *ch, complex_t *wa1, int mode)
{
    int i, k;
	real_t *cc_p;
	real_t *ch_cp;

	{
		real_t tmp, tmpy1, tmpy2, tmpy3, tmpy4, tmpy5, tmpy6, neg_x1;
		real_t *wa1_p;
		real_t *wa2_p;
		real_t *wa3_p;

        for (k = l1; k != 0; k --)
        {
			wa3_p = (real_t *)wa1;
			wa1_p = (real_t *)cfft_tab_512;
			wa2_p = (real_t *)cfft_tab_512;

			cc_p = (real_t *)cc;
			ch_cp = (real_t *)ch;

            for (i = ido; i != 0; i --)
            {
                real_t c20, c30, c40, t10, t20, t30, t40, c21, c31, c41, t11, t21, t31, t41;
				real_t wa10, wa11, wa20, wa21, wa30, wa31;
				real_t cc00, cc01, cc10, cc11, cc20, cc21, cc30, cc31;
				real_t reg_tmp0, reg_tmp1;


				cc30 = cc_p[6*ido];
				cc20 = cc_p[4*ido];
				cc10 = cc_p[2*ido];
				cc00 = *cc_p++;

				t30 = cc10 + cc30;
                t41 = cc10 - cc30;
				t20 = cc00 + cc20;
                t10 = cc00 - cc20;

				reg_tmp0 = t20 + t30;
				c30		 = t20 - t30;
				ch_cp[0] = reg_tmp0;

				cc31 = cc_p[6*ido];
				cc21 = cc_p[4*ido];
				cc11 = cc_p[2*ido];
				cc01 = *cc_p++;

                t21 = cc01 + cc21;
                t11 = cc01 - cc21;
                t31 = cc31 + cc11;
                t40 = cc31 - cc11;

				reg_tmp1 = t21 + t31;
				c31       = t21 - t31;
				ch_cp[1] = reg_tmp1;

                c20 = t10 + t40;
                c40 = t10 - t40;
                c21 = t11 + t41;
                c41 = t11 - t41;


#ifdef _WIN32
				wa30 = *wa3_p++;
				wa31 = *wa3_p++;
				wa10 = wa1_p[0];
				wa11 = wa1_p[1];wa1_p+=mode;
				wa20 = wa2_p[0];
				wa21 = wa2_p[1];wa2_p+=mode*2;

                ComplexMult(&ch_cp[2*l1*ido+1], &ch_cp[2*l1*ido],
                    c21, c20, wa10, wa11);
				ComplexMult(&ch_cp[4*l1*ido+1], &ch_cp[4*l1*ido],
                    c31, c30, wa20, wa21);
				ComplexMult(&ch_cp[6*l1*ido+1], &ch_cp[6*l1*ido],
                    c41, c40, wa30, wa31);

				ch_cp += 2;
#else //ARM
//Shawn 2004.8.26
				__asm
				{
					LDR		wa30, [wa3_p], #4
					LDR		wa31, [wa3_p], #4
					SMULL	tmp, tmpy1, wa30, c41
					SMLAL	tmp, tmpy1, wa31, c40
					RSB		neg_x1, c41, #0
					SMULL	tmp, tmpy2, wa30, c40
					SMLAL	tmp, tmpy2, wa31, neg_x1
				}

				tmpy1 = tmpy1 << (FRAC_SIZE-FRAC_BITS);
				ch_cp[6*l1*ido] = tmpy2 << (FRAC_SIZE-FRAC_BITS);

				__asm
				{
					LDR		wa20, [wa2_p]
					LDR		wa21, [wa2_p, #4]
					SMULL	tmp, tmpy3, wa20, c31
					SMLAL	tmp, tmpy3, wa21, c30
					RSB		neg_x1, c31, #0
					SMULL	tmp, tmpy4, wa20, c30
					SMLAL	tmp, tmpy4, wa21, neg_x1
				}



				tmpy3 = tmpy3 << (FRAC_SIZE-FRAC_BITS);
				ch_cp[4*l1*ido] = tmpy4 << (FRAC_SIZE-FRAC_BITS);

				__asm
				{
					LDR		wa10, [wa1_p]
					LDR		wa11, [wa1_p, #4]
					SMULL	tmp, tmpy5, wa10, c21
					SMLAL	tmp, tmpy5, wa11, c20
					RSB		neg_x1, c21, #0
					SMULL	tmp, tmpy6, wa10, c20
					SMLAL	tmp, tmpy6, wa11, neg_x1
				}

				wa2_p+=mode*2;
				wa1_p+=mode;

				tmpy5 = tmpy5 << (FRAC_SIZE-FRAC_BITS);
				ch_cp[2*l1*ido] = tmpy6 << (FRAC_SIZE-FRAC_BITS);ch_cp ++;

				ch_cp[6*l1*ido] = tmpy1;
				ch_cp[4*l1*ido] = tmpy3;
				ch_cp[2*l1*ido] = tmpy5;


				ch_cp ++;


#endif
            }
			cc = cc + (ido << 2);
			ch += ido;
        }
    }
}
#endif
#if 0
static void passf4pos_1(int l1, complex_t *cc, complex_t *ch)
{
    int k;
	real_t *cc_p;
	real_t *ch_p;

	cc_p = (real_t *)cc;
	ch_p = (real_t *)ch;

    for (k = l1; k != 0; k --)
    {
		real_t t10, t20, t30, t40, t11, t21, t31, t41;
		real_t cc00, cc01, cc10, cc11, cc20, cc21, cc30, cc31;
		real_t ch00, ch01, ch10, ch11, ch20, ch21, ch30, ch31;

		cc00 = *cc_p++;
		cc01 = *cc_p++;
		cc10 = *cc_p++;
		cc11 = *cc_p++;
		cc20 = *cc_p++;
		cc21 = *cc_p++;
		cc30 = *cc_p++;
		cc31 = *cc_p++;

		t20 = cc00 + cc20;
        t10 = cc00 - cc20;
		t21 = cc01 + cc21;
		t11 = cc01 - cc21;
		t30 = cc10 + cc30;
		t41 = cc10 - cc30;
		t31 = cc31 + cc11;
		t40 = cc31 - cc11;

		ch00 = t20 + t30;
        ch20 = t20 - t30;

		ch01 = t21 + t31;
		ch21 = t21 - t31;

		ch10 = t10 + t40;
		ch30 = t10 - t40;

		ch11 = t11 + t41;
		ch31 = t11 - t41;

		ch_p[6*l1] = ch30;
		ch_p[4*l1] = ch20;
		ch_p[2*l1] = ch10;
		*ch_p++    = ch00;
		ch_p[6*l1] = ch31;
		ch_p[4*l1] = ch21;
		ch_p[2*l1] = ch11;
		*ch_p++    = ch01;
    }
}
#else
//Shawn 2004.11.26
static void passf4pos_1(int l1, real_t *cc_p, real_t *ch_p)
{
    int k;

    for (k = l1; k != 0; k --)
    {
		real_t t10, t20, t30, t40, t11, t21, t31, t41;
		real_t cc00, cc01, cc10, cc11, cc20, cc21, cc30, cc31;
		real_t ch00, ch01, ch10, ch11, ch20, ch21, ch30, ch31;

		cc00 = *cc_p++;
		cc01 = *cc_p++;
		cc10 = *cc_p++;
		cc11 = *cc_p++;
		cc20 = *cc_p++;
		cc21 = *cc_p++;
		cc30 = *cc_p++;
		cc31 = *cc_p++;

		t20 = cc00 + cc20;
        t10 = cc00 - cc20;
		t21 = cc01 + cc21;
		t11 = cc01 - cc21;
		t30 = cc10 + cc30;
		t41 = cc10 - cc30;
		t31 = cc31 + cc11;
		t40 = cc31 - cc11;

		ch00 = t20 + t30;
        ch20 = t20 - t30;

		ch01 = t21 + t31;
		ch21 = t21 - t31;

		ch10 = t10 + t40;
		ch30 = t10 - t40;

		ch11 = t11 + t41;
		ch31 = t11 - t41;

		ch_p[6*l1] = ch30;
		ch_p[4*l1] = ch20;
		ch_p[2*l1] = ch10;
		*ch_p++    = ch00;
		ch_p[6*l1] = ch31;
		ch_p[4*l1] = ch21;
		ch_p[2*l1] = ch11;
		*ch_p++    = ch01;
    }
}
#endif
#endif
#endif

static void passf4neg(const unsigned short ido, const unsigned short l1, const complex_t *cc,
                      complex_t *ch, const complex_t *wa1, const complex_t *wa2,
                      const complex_t *wa3)
{
    unsigned short i, k, ac, ah;

    if (ido == 1)
    {
        for (k = 0; k < l1; k++)
        {
            complex_t t1, t2, t3, t4;

            ac = 4*k;
            ah = k;

            RE(t2) = RE(cc[ac])   + RE(cc[ac+2]);
            RE(t1) = RE(cc[ac])   - RE(cc[ac+2]);
            IM(t2) = IM(cc[ac])   + IM(cc[ac+2]);
            IM(t1) = IM(cc[ac])   - IM(cc[ac+2]);
            RE(t3) = RE(cc[ac+1]) + RE(cc[ac+3]);
            IM(t4) = RE(cc[ac+1]) - RE(cc[ac+3]);
            IM(t3) = IM(cc[ac+3]) + IM(cc[ac+1]);
            RE(t4) = IM(cc[ac+3]) - IM(cc[ac+1]);

            RE(ch[ah])      = RE(t2) + RE(t3);
            RE(ch[ah+2*l1]) = RE(t2) - RE(t3);

            IM(ch[ah])      = IM(t2) + IM(t3);
            IM(ch[ah+2*l1]) = IM(t2) - IM(t3);

            RE(ch[ah+l1])   = RE(t1) - RE(t4);
            RE(ch[ah+3*l1]) = RE(t1) + RE(t4);

            IM(ch[ah+l1])   = IM(t1) - IM(t4);
            IM(ch[ah+3*l1]) = IM(t1) + IM(t4);
        }
    } else {
        for (k = 0; k < l1; k++)
        {
            ac = 4*k*ido;
            ah = k*ido;

            for (i = 0; i < ido; i++)
            {
                complex_t c2, c3, c4, t1, t2, t3, t4;

                RE(t2) = RE(cc[ac+i]) + RE(cc[ac+i+2*ido]);
                RE(t1) = RE(cc[ac+i]) - RE(cc[ac+i+2*ido]);
                IM(t2) = IM(cc[ac+i]) + IM(cc[ac+i+2*ido]);
                IM(t1) = IM(cc[ac+i]) - IM(cc[ac+i+2*ido]);
                RE(t3) = RE(cc[ac+i+ido]) + RE(cc[ac+i+3*ido]);
                IM(t4) = RE(cc[ac+i+ido]) - RE(cc[ac+i+3*ido]);
                IM(t3) = IM(cc[ac+i+3*ido]) + IM(cc[ac+i+ido]);
                RE(t4) = IM(cc[ac+i+3*ido]) - IM(cc[ac+i+ido]);

                RE(c2) = RE(t1) - RE(t4);
                RE(c4) = RE(t1) + RE(t4);

                IM(c2) = IM(t1) - IM(t4);
                IM(c4) = IM(t1) + IM(t4);

                RE(ch[ah+i]) = RE(t2) + RE(t3);
                RE(c3)       = RE(t2) - RE(t3);

                IM(ch[ah+i]) = IM(t2) + IM(t3);
                IM(c3)       = IM(t2) - IM(t3);

#if 1
                ComplexMult(&RE(ch[ah+i+l1*ido]), &IM(ch[ah+i+l1*ido]),
                    RE(c2), IM(c2), RE(wa1[i]), IM(wa1[i]));
                ComplexMult(&RE(ch[ah+i+2*l1*ido]), &IM(ch[ah+i+2*l1*ido]),
                    RE(c3), IM(c3), RE(wa2[i]), IM(wa2[i]));
                ComplexMult(&RE(ch[ah+i+3*l1*ido]), &IM(ch[ah+i+3*l1*ido]),
                    RE(c4), IM(c4), RE(wa3[i]), IM(wa3[i]));
#else
                ComplexMult(&IM(ch[ah+i+l1*ido]), &RE(ch[ah+i+l1*ido]),
                    IM(c2), RE(c2), RE(wa1[i]), IM(wa1[i]));
                ComplexMult(&IM(ch[ah+i+2*l1*ido]), &RE(ch[ah+i+2*l1*ido]),
                    IM(c3), RE(c3), RE(wa2[i]), IM(wa2[i]));
                ComplexMult(&IM(ch[ah+i+3*l1*ido]), &RE(ch[ah+i+3*l1*ido]),
                    IM(c4), RE(c4), RE(wa3[i]), IM(wa3[i]));
#endif
            }
        }
    }
}

static void passf5(const unsigned short ido, const unsigned short l1, const complex_t *cc,
                   complex_t *ch, const complex_t *wa1, const complex_t *wa2, const complex_t *wa3,
                   const complex_t *wa4, const signed char isign)
{
    static real_t tr11 = FRAC_CONST(0.309016994374947);
    static real_t ti11 = FRAC_CONST(0.951056516295154);
    static real_t tr12 = FRAC_CONST(-0.809016994374947);
    static real_t ti12 = FRAC_CONST(0.587785252292473);
    unsigned short i, k, ac, ah;
    complex_t c2, c3, c4, c5, d3, d4, d5, d2, t2, t3, t4, t5;

    if (ido == 1)
    {
        if (isign == 1)
        {
            for (k = 0; k < l1; k++)
            {
                ac = 5*k + 1;
                ah = k;

                RE(t2) = RE(cc[ac]) + RE(cc[ac+3]);
                IM(t2) = IM(cc[ac]) + IM(cc[ac+3]);
                RE(t3) = RE(cc[ac+1]) + RE(cc[ac+2]);
                IM(t3) = IM(cc[ac+1]) + IM(cc[ac+2]);
                RE(t4) = RE(cc[ac+1]) - RE(cc[ac+2]);
                IM(t4) = IM(cc[ac+1]) - IM(cc[ac+2]);
                RE(t5) = RE(cc[ac]) - RE(cc[ac+3]);
                IM(t5) = IM(cc[ac]) - IM(cc[ac+3]);

                RE(ch[ah]) = RE(cc[ac-1]) + RE(t2) + RE(t3);
                IM(ch[ah]) = IM(cc[ac-1]) + IM(t2) + IM(t3);

                RE(c2) = RE(cc[ac-1]) + MUL_F(RE(t2),tr11) + MUL_F(RE(t3),tr12);
                IM(c2) = IM(cc[ac-1]) + MUL_F(IM(t2),tr11) + MUL_F(IM(t3),tr12);
                RE(c3) = RE(cc[ac-1]) + MUL_F(RE(t2),tr12) + MUL_F(RE(t3),tr11);
                IM(c3) = IM(cc[ac-1]) + MUL_F(IM(t2),tr12) + MUL_F(IM(t3),tr11);

                ComplexMult(&RE(c5), &RE(c4),
                    ti11, ti12, RE(t5), RE(t4));
                ComplexMult(&IM(c5), &IM(c4),
                    ti11, ti12, IM(t5), IM(t4));

                RE(ch[ah+l1]) = RE(c2) - IM(c5);
                IM(ch[ah+l1]) = IM(c2) + RE(c5);
                RE(ch[ah+2*l1]) = RE(c3) - IM(c4);
                IM(ch[ah+2*l1]) = IM(c3) + RE(c4);
                RE(ch[ah+3*l1]) = RE(c3) + IM(c4);
                IM(ch[ah+3*l1]) = IM(c3) - RE(c4);
                RE(ch[ah+4*l1]) = RE(c2) + IM(c5);
                IM(ch[ah+4*l1]) = IM(c2) - RE(c5);
            }
        } else {
            for (k = 0; k < l1; k++)
            {
                ac = 5*k + 1;
                ah = k;

                RE(t2) = RE(cc[ac]) + RE(cc[ac+3]);
                IM(t2) = IM(cc[ac]) + IM(cc[ac+3]);
                RE(t3) = RE(cc[ac+1]) + RE(cc[ac+2]);
                IM(t3) = IM(cc[ac+1]) + IM(cc[ac+2]);
                RE(t4) = RE(cc[ac+1]) - RE(cc[ac+2]);
                IM(t4) = IM(cc[ac+1]) - IM(cc[ac+2]);
                RE(t5) = RE(cc[ac]) - RE(cc[ac+3]);
                IM(t5) = IM(cc[ac]) - IM(cc[ac+3]);

                RE(ch[ah]) = RE(cc[ac-1]) + RE(t2) + RE(t3);
                IM(ch[ah]) = IM(cc[ac-1]) + IM(t2) + IM(t3);

                RE(c2) = RE(cc[ac-1]) + MUL_F(RE(t2),tr11) + MUL_F(RE(t3),tr12);
                IM(c2) = IM(cc[ac-1]) + MUL_F(IM(t2),tr11) + MUL_F(IM(t3),tr12);
                RE(c3) = RE(cc[ac-1]) + MUL_F(RE(t2),tr12) + MUL_F(RE(t3),tr11);
                IM(c3) = IM(cc[ac-1]) + MUL_F(IM(t2),tr12) + MUL_F(IM(t3),tr11);

                ComplexMult(&RE(c4), &RE(c5),
                    ti12, ti11, RE(t5), RE(t4));
                ComplexMult(&IM(c4), &IM(c5),
                    ti12, ti12, IM(t5), IM(t4));

                RE(ch[ah+l1]) = RE(c2) + IM(c5);
                IM(ch[ah+l1]) = IM(c2) - RE(c5);
                RE(ch[ah+2*l1]) = RE(c3) + IM(c4);
                IM(ch[ah+2*l1]) = IM(c3) - RE(c4);
                RE(ch[ah+3*l1]) = RE(c3) - IM(c4);
                IM(ch[ah+3*l1]) = IM(c3) + RE(c4);
                RE(ch[ah+4*l1]) = RE(c2) - IM(c5);
                IM(ch[ah+4*l1]) = IM(c2) + RE(c5);
            }
        }
    } else {
        if (isign == 1)
        {
            for (k = 0; k < l1; k++)
            {
                for (i = 0; i < ido; i++)
                {
                    ac = i + (k*5 + 1) * ido;
                    ah = i + k * ido;

                    RE(t2) = RE(cc[ac]) + RE(cc[ac+3*ido]);
                    IM(t2) = IM(cc[ac]) + IM(cc[ac+3*ido]);
                    RE(t3) = RE(cc[ac+ido]) + RE(cc[ac+2*ido]);
                    IM(t3) = IM(cc[ac+ido]) + IM(cc[ac+2*ido]);
                    RE(t4) = RE(cc[ac+ido]) - RE(cc[ac+2*ido]);
                    IM(t4) = IM(cc[ac+ido]) - IM(cc[ac+2*ido]);
                    RE(t5) = RE(cc[ac]) - RE(cc[ac+3*ido]);
                    IM(t5) = IM(cc[ac]) - IM(cc[ac+3*ido]);

                    RE(ch[ah]) = RE(cc[ac-ido]) + RE(t2) + RE(t3);
                    IM(ch[ah]) = IM(cc[ac-ido]) + IM(t2) + IM(t3);

                    RE(c2) = RE(cc[ac-ido]) + MUL_F(RE(t2),tr11) + MUL_F(RE(t3),tr12);
                    IM(c2) = IM(cc[ac-ido]) + MUL_F(IM(t2),tr11) + MUL_F(IM(t3),tr12);
                    RE(c3) = RE(cc[ac-ido]) + MUL_F(RE(t2),tr12) + MUL_F(RE(t3),tr11);
                    IM(c3) = IM(cc[ac-ido]) + MUL_F(IM(t2),tr12) + MUL_F(IM(t3),tr11);

                    ComplexMult(&RE(c5), &RE(c4),
                        ti11, ti12, RE(t5), RE(t4));
                    ComplexMult(&IM(c5), &IM(c4),
                        ti11, ti12, IM(t5), IM(t4));

                    IM(d2) = IM(c2) + RE(c5);
                    IM(d3) = IM(c3) + RE(c4);
                    RE(d4) = RE(c3) + IM(c4);
                    RE(d5) = RE(c2) + IM(c5);
                    RE(d2) = RE(c2) - IM(c5);
                    IM(d5) = IM(c2) - RE(c5);
                    RE(d3) = RE(c3) - IM(c4);
                    IM(d4) = IM(c3) - RE(c4);

#if 1
                    ComplexMult(&IM(ch[ah+l1*ido]), &RE(ch[ah+l1*ido]),
                        IM(d2), RE(d2), RE(wa1[i]), IM(wa1[i]));
                    ComplexMult(&IM(ch[ah+2*l1*ido]), &RE(ch[ah+2*l1*ido]),
                        IM(d3), RE(d3), RE(wa2[i]), IM(wa2[i]));
                    ComplexMult(&IM(ch[ah+3*l1*ido]), &RE(ch[ah+3*l1*ido]),
                        IM(d4), RE(d4), RE(wa3[i]), IM(wa3[i]));
                    ComplexMult(&IM(ch[ah+4*l1*ido]), &RE(ch[ah+4*l1*ido]),
                        IM(d5), RE(d5), RE(wa4[i]), IM(wa4[i]));
#else
                    ComplexMult(&RE(ch[ah+l1*ido]), &IM(ch[ah+l1*ido]),
                        RE(d2), IM(d2), RE(wa1[i]), IM(wa1[i]));
                    ComplexMult(&RE(ch[ah+2*l1*ido]), &IM(ch[ah+2*l1*ido]),
                        RE(d3), IM(d3), RE(wa2[i]), IM(wa2[i]));
                    ComplexMult(&RE(ch[ah+3*l1*ido]), &IM(ch[ah+3*l1*ido]),
                        RE(d4), IM(d4), RE(wa3[i]), IM(wa3[i]));
                    ComplexMult(&RE(ch[ah+4*l1*ido]), &IM(ch[ah+4*l1*ido]),
                        RE(d5), IM(d5), RE(wa4[i]), IM(wa4[i]));
#endif
                }
            }
        } else {
            for (k = 0; k < l1; k++)
            {
                for (i = 0; i < ido; i++)
                {
                    ac = i + (k*5 + 1) * ido;
                    ah = i + k * ido;

                    RE(t2) = RE(cc[ac]) + RE(cc[ac+3*ido]);
                    IM(t2) = IM(cc[ac]) + IM(cc[ac+3*ido]);
                    RE(t3) = RE(cc[ac+ido]) + RE(cc[ac+2*ido]);
                    IM(t3) = IM(cc[ac+ido]) + IM(cc[ac+2*ido]);
                    RE(t4) = RE(cc[ac+ido]) - RE(cc[ac+2*ido]);
                    IM(t4) = IM(cc[ac+ido]) - IM(cc[ac+2*ido]);
                    RE(t5) = RE(cc[ac]) - RE(cc[ac+3*ido]);
                    IM(t5) = IM(cc[ac]) - IM(cc[ac+3*ido]);

                    RE(ch[ah]) = RE(cc[ac-ido]) + RE(t2) + RE(t3);
                    IM(ch[ah]) = IM(cc[ac-ido]) + IM(t2) + IM(t3);

                    RE(c2) = RE(cc[ac-ido]) + MUL_F(RE(t2),tr11) + MUL_F(RE(t3),tr12);
                    IM(c2) = IM(cc[ac-ido]) + MUL_F(IM(t2),tr11) + MUL_F(IM(t3),tr12);
                    RE(c3) = RE(cc[ac-ido]) + MUL_F(RE(t2),tr12) + MUL_F(RE(t3),tr11);
                    IM(c3) = IM(cc[ac-ido]) + MUL_F(IM(t2),tr12) + MUL_F(IM(t3),tr11);

                    ComplexMult(&RE(c4), &RE(c5),
                        ti12, ti11, RE(t5), RE(t4));
                    ComplexMult(&IM(c4), &IM(c5),
                        ti12, ti12, IM(t5), IM(t4));

                    IM(d2) = IM(c2) - RE(c5);
                    IM(d3) = IM(c3) - RE(c4);
                    RE(d4) = RE(c3) - IM(c4);
                    RE(d5) = RE(c2) - IM(c5);
                    RE(d2) = RE(c2) + IM(c5);
                    IM(d5) = IM(c2) + RE(c5);
                    RE(d3) = RE(c3) + IM(c4);
                    IM(d4) = IM(c3) + RE(c4);

#if 1
                    ComplexMult(&RE(ch[ah+l1*ido]), &IM(ch[ah+l1*ido]),
                        RE(d2), IM(d2), RE(wa1[i]), IM(wa1[i]));
                    ComplexMult(&RE(ch[ah+2*l1*ido]), &IM(ch[ah+2*l1*ido]),
                        RE(d3), IM(d3), RE(wa2[i]), IM(wa2[i]));
                    ComplexMult(&RE(ch[ah+3*l1*ido]), &IM(ch[ah+3*l1*ido]),
                        RE(d4), IM(d4), RE(wa3[i]), IM(wa3[i]));
                    ComplexMult(&RE(ch[ah+4*l1*ido]), &IM(ch[ah+4*l1*ido]),
                        RE(d5), IM(d5), RE(wa4[i]), IM(wa4[i]));
#else
                    ComplexMult(&IM(ch[ah+l1*ido]), &RE(ch[ah+l1*ido]),
                        IM(d2), RE(d2), RE(wa1[i]), IM(wa1[i]));
                    ComplexMult(&IM(ch[ah+2*l1*ido]), &RE(ch[ah+2*l1*ido]),
                        IM(d3), RE(d3), RE(wa2[i]), IM(wa2[i]));
                    ComplexMult(&IM(ch[ah+3*l1*ido]), &RE(ch[ah+3*l1*ido]),
                        IM(d4), RE(d4), RE(wa3[i]), IM(wa3[i]));
                    ComplexMult(&IM(ch[ah+4*l1*ido]), &RE(ch[ah+4*l1*ido]),
                        IM(d5), RE(d5), RE(wa4[i]), IM(wa4[i]));
#endif
                }
            }
        }
    }
}


/*----------------------------------------------------------------------
   cfftf1, cfftf, cfftb, cffti1, cffti. Complex FFTs.
  ----------------------------------------------------------------------*/
#if 0
static INLINE void cfftf1pos(unsigned short n, complex_t *c, complex_t *ch,
                             const unsigned short *ifac, const complex_t *wa,
                             const signed char isign)
{
    unsigned short i;
    unsigned short k1, l1, l2;
    unsigned short na, nf, ip, iw, ix2, ix3, ix4, ido, idl1;

    nf = ifac[1];
    na = 0;
    l1 = 1;
    iw = 0;

    for (k1 = 2; k1 <= nf+1; k1++)
    {
        ip = ifac[k1];
        l2 = ip*l1;
        ido = n / l2;
        idl1 = ido*l1;

        switch (ip)
        {
        case 4:
            ix2 = iw + ido;
            ix3 = ix2 + ido;

            if (na == 0)
                passf4pos((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)c, ch, &wa[iw], &wa[ix2], &wa[ix3]);
            else
                passf4pos((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)ch, c, &wa[iw], &wa[ix2], &wa[ix3]);

            na = 1 - na;
            break;
        case 2:
            if (na == 0)
                passf2pos((unsigned short)ido, (unsigned short)l1, (real_t *)c, (real_t *)ch, (real_t *)&wa[iw]);
            else
                passf2pos((unsigned short)ido, (unsigned short)l1, (real_t *)ch, (real_t *)c, (real_t *)&wa[iw]);

            na = 1 - na;
            break;
        case 3:
            ix2 = iw + ido;

            if (na == 0)
                passf3((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)c, ch, &wa[iw], &wa[ix2], isign);
            else
                passf3((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)ch, c, &wa[iw], &wa[ix2], isign);

            na = 1 - na;
            break;
        case 5:
            ix2 = iw + ido;
            ix3 = ix2 + ido;
            ix4 = ix3 + ido;

            if (na == 0)
                passf5((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)c, ch, &wa[iw], &wa[ix2], &wa[ix3], &wa[ix4], isign);
            else
                passf5((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)ch, c, &wa[iw], &wa[ix2], &wa[ix3], &wa[ix4], isign);

            na = 1 - na;
            break;
        }

        l1 = l2;
        iw += (ip-1) * ido;
    }

    if (na == 0)
        return;

    for (i = 0; i < n; i++)
    {
        RE(c[i]) = RE(ch[i]);
        IM(c[i]) = IM(ch[i]);
    }
}
#else
#if 0
//Shawn 2004.8.26
static INLINE void cfftf1pos(unsigned short n, complex_t *c, complex_t *ch,
                             const unsigned short *ifac, const complex_t *wa,
                             const signed char isign)
{
    int i;
    int k1, l1, l2;
    int na, nf, ip, iw, ix2, ix3, ix4, ido, idl1;
	real_t *c_p;
	real_t *ch_p;

    nf = ifac[1];
    na = 0;
    l1 = 1;
    iw = 0;

    for (k1 = 2; k1 <= nf+1; k1++)
    {
        ip = ifac[k1];
        l2 = ip*l1;
        ido = n / l2;
        idl1 = ido*l1;

        switch (ip)
        {
        case 4:
            ix2 = iw + ido;
            ix3 = ix2 + ido;

            if (na == 0)
                passf4pos(ido, l1, (complex_t *)c, (complex_t *)ch, (complex_t *)&wa[iw], (complex_t *)&wa[ix2], (complex_t *)&wa[ix3]);
            else
                passf4pos(ido, l1, (complex_t *)ch, (complex_t *)c, (complex_t *)&wa[iw], (complex_t *)&wa[ix2], (complex_t *)&wa[ix3]);

            na = 1 - na;
            break;
        case 2:
            if (na == 0)
                passf2pos(ido, l1, (real_t *)c, (real_t *)ch, (real_t *)&wa[iw]);
            else
                passf2pos(ido, l1, (real_t *)ch, (real_t *)c, (real_t *)&wa[iw]);

            na = 1 - na;
            break;
        case 3:
            ix2 = iw + ido;

            if (na == 0)
                passf3((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)c, ch, &wa[iw], &wa[ix2], isign);
            else
                passf3((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)ch, c, &wa[iw], &wa[ix2], isign);

            na = 1 - na;
            break;
        case 5:
            ix2 = iw + ido;
            ix3 = ix2 + ido;
            ix4 = ix3 + ido;

            if (na == 0)
                passf5((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)c, ch, &wa[iw], &wa[ix2], &wa[ix3], &wa[ix4], isign);
            else
                passf5((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)ch, c, &wa[iw], &wa[ix2], &wa[ix3], &wa[ix4], isign);

            na = 1 - na;
            break;
        }

        l1 = l2;
        iw += (ip-1) * ido;
    }

    if (na == 0)
        return;

#ifdef _WIN32
	c_p = (real_t *)c;
	ch_p = (real_t *)ch;

    for (i = 0; i < n; i++)
    {
		c_p[0] = ch_p[0];
		c_p = c_p + 1;
		ch_p = ch_p + 1;
		c_p[0] = ch_p[0];
		c_p = c_p + 1;
		ch_p = ch_p + 1;
    }
#else //ARM
//Shawn 2004.8.26
	c_p = (real_t *)c;
	ch_p = (real_t *)ch;

    for (i = n; i != 0; i --)
    {
		real_t reg_ch0, reg_ch1;

		__asm
		{
			LDR		reg_ch0, [ch_p], #4
			LDR		reg_ch1, [ch_p], #4
			STR		reg_ch0, [c_p], #4
			STR		reg_ch1, [c_p], #4
		}
    }
#endif
}
#else //only for long & short windows !!!!!!!!!!!!!?????????????
//Shawn 2004.9.23
static INLINE void cfftf1pos_long(complex_t *c, complex_t *ch, const complex_t *wa)
{
	passf2pos_long((real_t *)c, (real_t *)ch, (real_t *)&wa[0]);
	passf4pos(64, 2, (complex_t *)ch, (complex_t *)c, (complex_t *)&wa[256]);
	passf4pos(16, 8, (complex_t *)c, (complex_t *)ch, (complex_t *)&wa[448]);
	passf4pos(4, 32, (complex_t *)ch, (complex_t *)c, (complex_t *)&wa[496]);
	passf4pos_1(128, (real_t *)c, (real_t *)ch);

	//memcpy(c, ch, n << 3);// removed by Shawn 2004.9.23  we use ch as the output

}

static INLINE void cfftf1pos_short(complex_t *c, complex_t *ch, const complex_t *wa)
{
    passf4pos(16, 1, (complex_t *)c, (complex_t *)ch, (complex_t *)&wa[448]);
	passf4pos(4, 4, (complex_t *)ch, (complex_t *)c, (complex_t *)&wa[496]);
	passf4pos_1(16, (real_t *)c, (real_t *)ch);

	//memcpy(c, ch, n << 3);// removed by Shawn 2004.9.23  we use ch as the output
}

static INLINE void cfftf1pos_LS(unsigned short n, complex_t *c, complex_t *ch)
{
	int d = 0;

	if (n == 512)
	{
		passf2pos_long((real_t *)c, (real_t *)ch, (real_t *)&cfft_tab_512[0]);
		passf4pos_new(64, 2, (complex_t *)ch, (complex_t *)c, (complex_t *)&cfft_tab_512_64[0], 4);
		d = 3;
	}
	passf4pos_new(16, 1<<d, (complex_t *)c, (complex_t *)ch, (complex_t *)&cfft_tab_512_16[0], 16);
	passf4pos_new(4, 4<<d, (complex_t *)ch, (complex_t *)c, (complex_t *)&cfft_tab_512_4[0], 64);
	passf4pos_1(16<<d, (real_t *)c, (real_t *)ch);

	//memcpy(c, ch, n << 3);// removed by Shawn 2004.9.23  we use ch as the output

}

#endif
#endif

#ifdef LTP_DEC
static INLINE void cfftf1neg(unsigned short n, complex_t *c, complex_t *ch,
                             const unsigned short *ifac, const complex_t *wa,
                             const signed char isign)
{
    unsigned short i;
    unsigned short k1, l1, l2;
    unsigned short na, nf, ip, iw, ix2, ix3, ix4, ido, idl1;

    nf = ifac[1];
    na = 0;
    l1 = 1;
    iw = 0;

    for (k1 = 2; k1 <= nf+1; k1++)
    {
        ip = ifac[k1];
        l2 = ip*l1;
        ido = n / l2;
        idl1 = ido*l1;

        switch (ip)
        {
        case 4:
            ix2 = iw + ido;
            ix3 = ix2 + ido;

            if (na == 0)
                passf4neg((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)c, ch, &wa[iw], &wa[ix2], &wa[ix3]);
            else
                passf4neg((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)ch, c, &wa[iw], &wa[ix2], &wa[ix3]);

            na = 1 - na;
            break;
        case 2:
            if (na == 0)
                passf2neg((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)c, ch, &wa[iw]);
            else
                passf2neg((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)ch, c, &wa[iw]);

            na = 1 - na;
            break;
        case 3:
            ix2 = iw + ido;

            if (na == 0)
                passf3((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)c, ch, &wa[iw], &wa[ix2], isign);
            else
                passf3((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)ch, c, &wa[iw], &wa[ix2], isign);

            na = 1 - na;
            break;
        case 5:
            ix2 = iw + ido;
            ix3 = ix2 + ido;
            ix4 = ix3 + ido;

            if (na == 0)
                passf5((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)c, ch, &wa[iw], &wa[ix2], &wa[ix3], &wa[ix4], isign);
            else
                passf5((const unsigned short)ido, (const unsigned short)l1, (const complex_t*)ch, c, &wa[iw], &wa[ix2], &wa[ix3], &wa[ix4], isign);

            na = 1 - na;
            break;
        }

        l1 = l2;
        iw += (ip-1) * ido;
    }

    if (na == 0)
        return;

    for (i = 0; i < n; i++)
    {
        RE(c[i]) = RE(ch[i]);
        IM(c[i]) = IM(ch[i]);
    }
}

void cfftf(cfft_info *cfft, complex_t *c)
{
    cfftf1neg(cfft->n, c, cfft->work, (const unsigned short*)cfft->ifac, (const complex_t*)cfft->tab, -1);
}
#endif

#if 0
void cfftb(cfft_info *cfft, complex_t *c)
{
    if (cfft->n == 512)
		cfftf1pos_long(c, cfft->work, (const complex_t*)cfft->tab);
	else
		cfftf1pos_short(c, cfft->work, (const complex_t*)cfft->tab);
}
#else
//Shawn 2004.9.24
#ifndef ARM_ASM
void cfftb(cfft_info *cfft, complex_t *c)
{
		cfftf1pos_LS(cfft->n, c, cfft->work);
}
#endif
#endif

static void cffti1(unsigned short n, complex_t *wa, unsigned short *ifac)
{
    static unsigned short ntryh[4] = {3, 4, 2, 5};
#ifndef FIXED_POINT
    real_t arg, argh, argld, fi;
    unsigned short ido, ipm;
    unsigned short i1, k1, l1, l2;
    unsigned short ld, ii, ip;
#endif
    unsigned short ntry = 0, i, j;
    unsigned short ib;
    unsigned short nf, nl, nq, nr;

    nl = n;
    nf = 0;
    j = 0;

startloop:
    j++;

    if (j <= 4)
        ntry = ntryh[j-1];
    else
        ntry += 2;

    do
    {
        nq = nl / ntry;
        nr = nl - ntry*nq;

        if (nr != 0)
            goto startloop;

        nf++;
        ifac[nf+1] = ntry;
        nl = nq;

        if (ntry == 2 && nf != 1)
        {
            for (i = 2; i <= nf; i++)
            {
                ib = nf - i + 2;
                ifac[ib+1] = ifac[ib];
            }
            ifac[2] = 2;
        }
    } while (nl != 1);

    ifac[0] = n;
    ifac[1] = nf;

#ifndef FIXED_POINT
    argh = (real_t)2.0*(real_t)M_PI / (real_t)n;
    i = 0;
    l1 = 1;

    for (k1 = 1; k1 <= nf; k1++)
    {
        ip = ifac[k1+1];
        ld = 0;
        l2 = l1*ip;
        ido = n / l2;
        ipm = ip - 1;

        for (j = 0; j < ipm; j++)
        {
            i1 = i;
            RE(wa[i]) = 1.0;
            IM(wa[i]) = 0.0;
            ld += l1;
            fi = 0;
            argld = ld*argh;

            for (ii = 0; ii < ido; ii++)
            {
                i++;
                fi++;
                arg = fi * argld;
                RE(wa[i]) = (real_t)cos(arg);
#if 1
                IM(wa[i]) = (real_t)sin(arg);
#else
                IM(wa[i]) = (real_t)-sin(arg);
#endif
            }

            if (ip > 5)
            {
                RE(wa[i1]) = RE(wa[i]);
                IM(wa[i1]) = IM(wa[i]);
            }
        }
        l1 = l2;
    }
#endif
}

#endif

#if 0
cfft_info *cffti(unsigned short n)
{
    cfft_info *cfft = (cfft_info*)faad_malloc(sizeof(cfft_info));

    cfft->n = n;
    cfft->work = (complex_t*)faad_malloc(n*sizeof(complex_t));

#ifndef FIXED_POINT
    cfft->tab = (complex_t*)faad_malloc(n*sizeof(complex_t));

    cffti1(n, cfft->tab, cfft->ifac);
#else
    cffti1(n, NULL, cfft->ifac);

    /*switch (n)
    {
    case 64: cfft->tab = cfft_tab_64; break;
    case 512: cfft->tab = cfft_tab_512; break;
    }*/

	cfft->tab = cfft_tab_512; //512 and 64 can share the same table Shanw 2004.9.24
#endif

    return cfft;
}
#else
//Shawn 2004.10.12
//real_t cfft_work_buf[1024];

cfft_info *cffti(unsigned short n)
{
    cfft_info *cfft = (cfft_info*)kmalloc(sizeof(cfft_info), GFP_KERNEL);//(cfft_info*)faad_malloc(sizeof(cfft_info));


    cfft->n = n;
    cfft->work = (complex_t *)&sect_start_end_cb_offset_buf[2*(3*8*15*8 + 2*8*15*8) - 1408];//cfft_work_buf;//faad_malloc(n*sizeof(complex_t));

    //cffti1(n, NULL, cfft->ifac);

    /*switch (n)
    {
    case 64: cfft->tab = cfft_tab_64; break;
    case 512: cfft->tab = cfft_tab_512; break;
    }*/

	//cfft->tab = cfft_tab_512; //512 and 64 can share the same table Shanw 2004.9.24

    return cfft;
}
#endif

#if 0
void cfftu(cfft_info *cfft)
{
    if (cfft->work) faad_free(cfft->work);
#ifndef FIXED_POINT
    if (cfft->tab) faad_free(cfft->tab);
#endif

    if (cfft) faad_free(cfft);
}
#else
//Shawn 2004.10.12
void cfftu(cfft_info *cfft)
{
    //if (cfft->work) faad_free(cfft->work);// use globle buffer


    if (cfft) kfree(cfft);//faad_free(cfft);
}
#endif
