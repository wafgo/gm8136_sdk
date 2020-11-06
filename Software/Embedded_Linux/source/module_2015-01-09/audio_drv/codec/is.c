#include "common.h"
#include "structs.h"
#include "syntax.h"
#include "is.h"

/* multiply with coef shift */
static INLINE real_t MUL_C_IS(real_t A, real_t B)
{
	return (real_t)(((long long)A*B)>>COEF_BITS);
}

real_t pow05_table[] = 
{
    COEF_CONST(1.68179283050743), /* 0.5^(-3/4) */
    COEF_CONST(1.41421356237310), /* 0.5^(-2/4) */
    COEF_CONST(1.18920711500272), /* 0.5^(-1/4) */
    COEF_CONST(1.0),              /* 0.5^( 0/4) */
    COEF_CONST(0.84089641525371), /* 0.5^(+1/4) */
    COEF_CONST(0.70710678118655), /* 0.5^(+2/4) */
    COEF_CONST(0.59460355750136)  /* 0.5^(+3/4) */
};


#if 0
void is_decode(ic_stream *ics, ic_stream *icsr, real_t *l_spec, real_t *r_spec, unsigned short frame_len)
{
    char g, sfb, b;
    unsigned short i;
#ifndef FIXED_POINT
    real_t scale;
#else
    int exp, frac;
#endif

    unsigned short nshort = frame_len/8;
    char group = 0;

    for (g = 0; g < icsr->num_window_groups; g++)
    {
        /* Do intensity stereo decoding */
        for (b = 0; b < icsr->window_group_length[g]; b++)
        {
            for (sfb = 0; sfb < icsr->max_sfb; sfb++)
            {
                if (is_intensity(icsr, g, sfb))
                {
                    /* For scalefactor bands coded in intensity stereo the
                       corresponding predictors in the right channel are
                       switched to "off".
                     */
                    ics->pred.prediction_used[sfb] = 0;
                    icsr->pred.prediction_used[sfb] = 0;

#ifndef FIXED_POINT
                    scale = (real_t)pow(0.5, (0.25*icsr->scale_factors[g][sfb]));
#else
                    exp = icsr->scale_factors[g][sfb] >> 2;
                    frac = icsr->scale_factors[g][sfb] & 3;
#endif

                    /* Scale from left to right channel,
                       do not touch left channel */
                    for (i = icsr->swb_offset[sfb]; i < icsr->swb_offset[sfb+1]; i++)
                    {
#ifndef FIXED_POINT
                        r_spec[(group*nshort)+i] = MUL_R(l_spec[(group*nshort)+i], scale);
#else
                        if (exp < 0)
                            r_spec[(group*nshort)+i] = l_spec[(group*nshort)+i] << -exp;
                        else
                            r_spec[(group*nshort)+i] = l_spec[(group*nshort)+i] >> exp;
                        r_spec[(group*nshort)+i] = MUL_C(r_spec[(group*nshort)+i], pow05_table[frac + 3]);
#endif
                        if (is_intensity(icsr, g, sfb) != invert_intensity(ics, g, sfb))
                            r_spec[(group*nshort)+i] = -r_spec[(group*nshort)+i];
                    }
                }
            }
            group++;
        }
    }
}
#else
//Shawn 2004.9.24
#ifndef ARM_ASM
void is_decode(ic_stream *ics, ic_stream *icsr, real_t *l_spec, real_t *r_spec, unsigned short frame_len)
{
    int g, sfb, b;
    int i;
    int exp, frac;
    //int nshort = frame_len >> 3;
    int group = 0;

    for (g = 0; g < icsr->num_window_groups; g++)
    {
        /* Do intensity stereo decoding */
        for (b = 0; b < icsr->window_group_length[g]; b++)
        {
            for (sfb = 0; sfb < icsr->max_sfb; sfb++)
            {
                if (is_intensity(icsr, g, sfb))
                {
					int reg_low, reg_up, reg_tab;
					real_t *r_spec_p, *l_spec_p;
					short reg_scalefctor;
                    /* For scalefactor bands coded in intensity stereo the
                       corresponding predictors in the right channel are
                       switched to "off".
                     */
                    ics->pred.prediction_used[sfb] = 0;
                    icsr->pred.prediction_used[sfb] = 0;

					reg_scalefctor = icsr->scale_factors[g*51+sfb];

                    exp = reg_scalefctor >> 2;
                    frac = reg_scalefctor & 3;

					reg_tab = pow05_table[frac + 3];

					reg_low = icsr->swb_offset[sfb];
					reg_up = icsr->swb_offset[sfb+1];

					r_spec_p = &r_spec[(group<<7)+reg_low];
					l_spec_p = &l_spec[(group<<7)+reg_low];


                    /* Scale from left to right channel,
                       do not touch left channel */
                    for (i = reg_up - reg_low; i != 0; i --)
                    {
						real_t reg_r, reg_l;

						reg_l = *l_spec_p++;

                        if (exp < 0)
                            reg_r = reg_l << -exp;
                        else
                            reg_r = reg_l >> exp;

#ifdef _WIN32
                        *r_spec_p++ = MUL_C_IS(reg_r, reg_tab);
#else
//Shawn 2004.12.13
						{
							real_t lo, tmp, hi, r;
	
							__asm
							{
								SMULL	lo, hi, reg_tab, reg_r
								MOV		tmp, lo, LSR COEF_BITS
								ORR		r, tmp, hi, LSL (32 - COEF_BITS)
							}

							*r_spec_p++ = r;
						}
#endif

                        if (is_intensity(icsr, g, sfb) != invert_intensity(ics, g, sfb))
                            r_spec_p[-1] = -r_spec_p[-1];
                    }
                }
            }
            group++;
        }
    }
}
#endif
#endif
