#include "common.h"
#include "structs.h"
#include "syntax.h"
#include "ms.h"
#include "is.h"
#include "pns.h"

#if 0
void ms_decode(ic_stream *ics, ic_stream *icsr, real_t *l_spec, real_t *r_spec, unsigned short frame_len)
{
    char g, b, sfb;
    char group = 0;
    unsigned short nshort = frame_len/8;

    unsigned short i, k;
    real_t tmp;

    if (ics->ms_mask_present >= 1)
    {
        for (g = 0; g < ics->num_window_groups; g++) 
        {
            for (b = 0; b < ics->window_group_length[g]; b++)
            {
                for (sfb = 0; sfb < ics->max_sfb; sfb++)
                {
                    /* If intensity stereo coding or noise substitution is on
                       for a particular scalefactor band, no M/S stereo decoding
                       is carried out.
                     */
                    if ((ics->ms_used[g][sfb] || ics->ms_mask_present == 2) &&
                        !is_intensity(icsr, g, sfb) && !is_noise(ics, g, sfb))
                    {
                        for (i = ics->swb_offset[sfb]; i < ics->swb_offset[sfb+1]; i++)
                        {
                            k = (group*nshort) + i;
                            tmp = l_spec[k] - r_spec[k];
                            l_spec[k] = l_spec[k] + r_spec[k];
                            r_spec[k] = tmp;
                        }
                    }
                }
                group++;
            }
        }
    }
}
#else
//Shawn 2004.8.23
void ms_decode(ic_stream *ics, ic_stream *icsr, real_t *l_spec_p, real_t *r_spec_p, unsigned short frame_len)
{
    int g, b, sfb;
    int nshort = frame_len >> 3;
    int i;
	

    if (ics->ms_mask_present >= 1)
    {
        for (g = 0; g < ics->num_window_groups; g++) 
        {
            for (b = ics->window_group_length[g]; b != 0; b --)
            {
                for (sfb = 0; sfb < ics->max_sfb; sfb++)
                {
                    /* If intensity stereo coding or noise substitution is on
                       for a particular scalefactor band, no M/S stereo decoding
                       is carried out.
                     */
                    /*if ((ics->ms_used[g*MAX_SFB+sfb] || ics->ms_mask_present == 2) &&
                        !is_intensity(icsr, g, sfb) && !is_noise(ics, g, sfb))*/
					if (!is_intensity(icsr, g, sfb))
						if (!is_noise(ics, g, sfb))
							if ((ics->ms_used[g*MAX_SFB+sfb] || ics->ms_mask_present == 2))
                    {
						real_t *l_spec_pp;
						real_t *r_spec_pp;
						int reg_lo, reg_up, N;

						reg_lo = ics->swb_offset[sfb];
						reg_up = ics->swb_offset[sfb+1];

						l_spec_pp = &l_spec_p[reg_lo];
						r_spec_pp = &r_spec_p[reg_lo];

						N = reg_up - reg_lo;

                        for (i = N; i != 0; i --)
                        {
							real_t reg_l, reg_r;
							real_t reg_r_l, reg_r_r;

							reg_l = *l_spec_pp;
							reg_r = *r_spec_pp;

                            reg_r_l = reg_l + reg_r;
                            reg_r_r = reg_l - reg_r;

							*l_spec_pp++ = reg_r_l;
							*r_spec_pp++ = reg_r_r;
                        }
                    }
                }
				l_spec_p = l_spec_p + nshort;
				r_spec_p = r_spec_p + nshort;
            }
        }
    }
}
#endif
