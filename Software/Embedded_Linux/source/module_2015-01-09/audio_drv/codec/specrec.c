/*
  Spectral reconstruction:
   - grouping/sectioning
   - inverse quantization
   - applying scalefactors
*/

#include "common.h"
#include "structs.h"
#include <linux/types.h>
#include <linux/string.h>

//#include <string.h>
//#include <stdlib.h>
#include "specrec.h"
#include "syntax.h"
#include "iq_table.h"
#include "ms.h"
#include "is.h"
#include "pns.h"
#include "tns.h"
#include "drc.h"
#include "lt_predict.h"

//Equalizer
volatile int eq_type;

const unsigned short MP3_EQ_table[5][10] = {
 {1789*4, 1789*4, 1789*4, 2900*4, 4703*4, 4096*4, 32768, 32768, 32768, 32768}, // Treble
 {32768, 32768, 32768, 5786*4, 4703*4, 2900*4, 2048*4, 1788*4, 1788*4, 1789*4}, // Bass
 {4096*4, 4096*4, 7118*4, 7118*4, 7118*4, 5786*4, 6200*4, 7118*4, 32768, 32768}, // Jazz
 {32768, 32768, 32768, 32768, 32768, 32768, 2357*4, 2357*4, 2357*4, 1789*4}, // Classic
 {7118*4, 6200*4, 2900*4, 2357*4, 2706*4, 6200*4, 7627*4, 32768, 32768, 32768} // Rock
};

unsigned short band_info_EQ_long[10] = {
 6, 6, 10, 16, 56, 116, 209, 186, 93, 326 //mid
 //4, 6, 6, 14, 18, 92, 140, 278, 92, 374 //end
};

unsigned short band_info_EQ_short[10] = {
 1, 1, 1, 2, 7, 14, 26, 23, 12, 41 //mid
 //0, 1, 1, 2, 2, 12, 18, 34, 12, 46 //end
};

real_t time_out_buf[2*1024];
real_t intermed_out_buf[2*1024];

#ifndef ARM_ASM
void EQ_function_long(real_t *spec_coef)
{
	int i, j;
	const unsigned short *MP3_EQ_table_p;
	unsigned short *band_info_EQ_p;

	MP3_EQ_table_p = &MP3_EQ_table[eq_type][0];
	band_info_EQ_p = &band_info_EQ_long[0];

	for (j = 10; j != 0; j --)
	{
		int reg_MP3_EQ_table;

		reg_MP3_EQ_table = *MP3_EQ_table_p++;

		for (i = *band_info_EQ_p++; i != 0; i --)
		{
#ifdef _WIN32
			*spec_coef++ = ((__int64)*spec_coef * reg_MP3_EQ_table) >> 15;
#else
				int lo, hi, tmp, reg_spec_coef;

				__asm
				{
					LDR	reg_spec_coef, [spec_coef]
					SMULL	lo, hi, reg_MP3_EQ_table, reg_spec_coef
					MOV	tmp, lo, LSR #15
					ORR	tmp, tmp, hi, LSL #17
					STR	tmp, [spec_coef], #4
				}
#endif
		}
	}
}


void EQ_function_short(real_t *spec_coef)
{
	int i, j, k;
	real_t *spec_coef_p;
	const unsigned short *MP3_EQ_table_p;
	unsigned short *band_info_EQ_p;

	for (k = 0; k < 8; k ++)
	{
		spec_coef_p = &spec_coef[128*k];
		MP3_EQ_table_p = &MP3_EQ_table[eq_type][0];
		band_info_EQ_p = &band_info_EQ_short[0];

		for (j = 0; j < 10; j ++)
		{
			int reg_MP3_EQ_table;

			reg_MP3_EQ_table = *MP3_EQ_table_p++;

			for (i = *band_info_EQ_p++; i != 0; i --)
			{
#ifdef _WIN32
				*spec_coef_p++ = ((__int64)*spec_coef_p * reg_MP3_EQ_table) >> 15;
#else
				int lo, hi, tmp, reg_spec_coef;

				__asm
				{
					LDR	reg_spec_coef, [spec_coef_p]
					SMULL	lo, hi, reg_MP3_EQ_table, reg_spec_coef
					MOV	tmp, lo, LSR #15
					ORR	tmp, tmp, hi, LSL #17
					STR	tmp, [spec_coef_p], #4
				}
#endif
			}
		}
	}
}
#else
void EQ_function_long(real_t *);
void EQ_function_short(real_t *);
#endif

/* static function declarations */
static void quant_to_spec(ic_stream *ics, real_t *spec_data, unsigned short frame_len);
#if 0
static char inverse_quantization(real_t *x_invquant, const int16_t *x_quant, const unsigned short frame_len);
#else
//Shawn 2004.8.23
static void inverse_quantization(real_t *x_invquant, const int16_t *x_quant);
#endif

#if 0
static const char num_swb_960_window[] =
{
    40, 40, 45, 49, 49, 49, 46, 46, 42, 42, 42, 40
};
#endif

static const char num_swb_1024_window[] =
{
    41, 41, 47, 49, 49, 51, 47, 47, 43, 43, 43, 40
};

static const char num_swb_128_window[] =
{
    12, 12, 12, 14, 14, 14, 15, 15, 15, 15, 15, 15
};

#if 0
static const unsigned short swb_offset_1024_96[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
    64, 72, 80, 88, 96, 108, 120, 132, 144, 156, 172, 188, 212, 240,
    276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960//, 1024 ??????????
};

static const unsigned short swb_offset_1024_64[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56,
    64, 72, 80, 88, 100, 112, 124, 140, 156, 172, 192, 216, 240, 268,
    304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784, 824,
    864, 904, 944, 984//, 1024 ??????????
};

static const unsigned short swb_offset_1024_48[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
    80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
    768, 800, 832, 864, 896, 928//, 1024 ??????????
};

static const unsigned short swb_offset_1024_32[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72,
    80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736,
    768, 800, 832, 864, 896, 928, 960, 992//, 1024 ??????????
};


static const unsigned short swb_offset_1024_24[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68,
    76, 84, 92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220,
    240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704,
    768, 832, 896, 960//, 1024 ??????????
};

static const unsigned short swb_offset_1024_16[] =
{
    0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124,
    136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320, 344,
    368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960//, 1024 ??????????
};

static const unsigned short swb_offset_1024_8[] =
{
    0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172,
    188, 204, 220, 236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448,
    476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944//, 1024 ??????????
};

static const unsigned short *swb_offset_1024_window[] =
{
    swb_offset_1024_96,      /* 96000 */
    swb_offset_1024_96,      /* 88200 */
    swb_offset_1024_64,      /* 64000 */
    swb_offset_1024_48,      /* 48000 */
    swb_offset_1024_48,      /* 44100 */
    swb_offset_1024_32,      /* 32000 */
    swb_offset_1024_24,      /* 24000 */
    swb_offset_1024_24,      /* 22050 */
    swb_offset_1024_16,      /* 16000 */
    swb_offset_1024_16,      /* 12000 */
    swb_offset_1024_16,      /* 11025 */
    swb_offset_1024_8        /* 8000  */
};
#else
//Shawn 2004.9.29  if 1024 is ignored, we can reduce the memory size
static const char swb_offset_1024_96[] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 27, 30, 33,
	36, 39, 43, 47, 53, 60, 69, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240
};

static const char swb_offset_1024_64[] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 25, 28, 31, 35,
	39, 43, 48, 54, 60, 67, 76, 86, 96, 106, 116, 126, 136, 146, 156, 166, 176, 186,
	196, 206, 216, 226, 236, 246
};

static const char swb_offset_1024_48[] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 20, 22, 24, 27, 30, 33, 36, 40,
	44, 49, 54, 60, 66, 73, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168,
	176, 184, 192, 200, 208, 216, 224, 232,
};

static const char swb_offset_1024_32[] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 20, 22, 24, 27, 30, 33, 36, 40,
	44, 49, 54, 60, 66, 73, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168,
	176, 184, 192, 200, 208, 216, 224, 232, 240, 248
};


static const char swb_offset_1024_24[] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 34,
	37, 40, 43, 47, 51, 55, 60, 65, 71, 77, 84, 91, 99, 108, 117, 127, 138, 150, 163,
	176, 192, 208, 224, 240
};

static const char swb_offset_1024_16[] =
{
	0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49, 53,
	57, 61, 65, 70, 75, 80, 86, 92, 99, 106, 114, 123, 133, 143, 154, 166, 179, 193, 208, 224, 240
};

static const char swb_offset_1024_8[] =
{
	0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 43, 47, 51, 55, 59, 63, 67,
	72, 77, 82, 87, 93, 99, 105, 112, 119, 127, 136, 145, 155, 166, 178, 191, 205, 220, 236
};

static const char *swb_offset_1024_window[] =
{
    swb_offset_1024_96,      /* 96000 */
    swb_offset_1024_96,      /* 88200 */
    swb_offset_1024_64,      /* 64000 */
    swb_offset_1024_48,      /* 48000 */
    swb_offset_1024_48,      /* 44100 */
    swb_offset_1024_32,      /* 32000 */
    swb_offset_1024_24,      /* 24000 */
    swb_offset_1024_24,      /* 22050 */
    swb_offset_1024_16,      /* 16000 */
    swb_offset_1024_16,      /* 12000 */
    swb_offset_1024_16,      /* 11025 */
    swb_offset_1024_8        /* 8000  */
};
#endif


/*static const char swb_offset_128_96[] =
{
    0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
};*///the same as 64 Shawn 2004.9.29

static const char swb_offset_128_64[] =
{
    0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92//, 128 ???????????
};

static const char swb_offset_128_48[] =
{
    0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112//, 128 ???????????
};

static const char swb_offset_128_24[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108//, 128 ???????????
};


static const char swb_offset_128_16[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108//, 128 ???????????
};

static const char swb_offset_128_8[] =
{
    0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108//, 128 ???????????
};



static const  char *swb_offset_128_window[] =
{
    swb_offset_128_64,       /* 96000 */
    swb_offset_128_64,       /* 88200 */
    swb_offset_128_64,       /* 64000 */
    swb_offset_128_48,       /* 48000 */
    swb_offset_128_48,       /* 44100 */
    swb_offset_128_48,       /* 32000 */
    swb_offset_128_24,       /* 24000 */
    swb_offset_128_24,       /* 22050 */
    swb_offset_128_16,       /* 16000 */
    swb_offset_128_16,       /* 12000 */
    swb_offset_128_16,       /* 11025 */
    swb_offset_128_8         /* 8000  */
};

#define bit_set(A, B) ((A) & (1<<(B)))

/* 4.5.2.3.4 */
/*
  - determine the number of windows in a window_sequence named num_windows
  - determine the number of window_groups named num_window_groups
  - determine the number of windows in each group named window_group_length[g]
  - determine the total number of scalefactor window bands named num_swb for
    the actual window type
  - determine swb_offset[swb], the offset of the first coefficient in
    scalefactor window band named swb of the window actually used
  - determine sect_sfb_offset[g][section],the offset of the first coefficient
    in section named section. This offset depends on window_sequence and
    scale_factor_grouping and is needed to decode the spectral_data().
*/
#if 0
char window_grouping_info(GMAACDecHandle hDecoder, ic_stream *ics)
{
    char i, g;

    char sf_index = hDecoder->sf_index;

    switch (ics->window_sequence) {
    case ONLY_LONG_SEQUENCE:
    case LONG_START_SEQUENCE:
    case LONG_STOP_SEQUENCE:
        ics->num_windows = 1;
        ics->num_window_groups = 1;
        ics->window_group_length[ics->num_window_groups-1] = 1;

            if (hDecoder->frameLength == 1024)
                ics->num_swb = num_swb_1024_window[sf_index];
            else /* if (hDecoder->frameLength == 960) */
                ics->num_swb = num_swb_960_window[sf_index];

        /* preparation of sect_sfb_offset for long blocks */
        /* also copy the last value! */
            for (i = 0; i < ics->num_swb; i++)
            {
                ics->sect_sfb_offset[0][i] = swb_offset_1024_window[sf_index][i];
                ics->swb_offset[i] = swb_offset_1024_window[sf_index][i];
            }
            ics->sect_sfb_offset[0][ics->num_swb] = hDecoder->frameLength;
            ics->swb_offset[ics->num_swb] = hDecoder->frameLength;

        return 0;
    case EIGHT_SHORT_SEQUENCE:
        ics->num_windows = 8;
        ics->num_window_groups = 1;
        ics->window_group_length[ics->num_window_groups-1] = 1;
        ics->num_swb = num_swb_128_window[sf_index];

        for (i = 0; i < ics->num_swb; i++)
            ics->swb_offset[i] = swb_offset_128_window[sf_index][i];
        ics->swb_offset[ics->num_swb] = hDecoder->frameLength/8;

        for (i = 0; i < ics->num_windows-1; i++) {
            if (bit_set(ics->scale_factor_grouping, 6-i) == 0)
            {
                ics->num_window_groups += 1;
                ics->window_group_length[ics->num_window_groups-1] = 1;
            } else {
                ics->window_group_length[ics->num_window_groups-1] += 1;
            }
        }

        /* preparation of sect_sfb_offset for short blocks */
        for (g = 0; g < ics->num_window_groups; g++)
        {
            unsigned short width;
            char sect_sfb = 0;
            unsigned short offset = 0;

            for (i = 0; i < ics->num_swb; i++)
            {
                if (i+1 == ics->num_swb)
                {
                    width = (hDecoder->frameLength/8) - swb_offset_128_window[sf_index][i];
                } else {
                    width = swb_offset_128_window[sf_index][i+1] -
                        swb_offset_128_window[sf_index][i];
                }
                width *= ics->window_group_length[g];
                ics->sect_sfb_offset[g][sect_sfb++] = offset;
                offset += width;
            }
            ics->sect_sfb_offset[g][sect_sfb] = offset;
        }
        return 0;
    default:
        return 1;
    }
}
#else
//Shawn 2004.9.29
char window_grouping_info(GMAACDecHandle hDecoder, ic_stream *ics)
{
    int i, g;
    int sf_index;
	int reg_idx;
	int reg_num_group;
	int reg_sf_g;
	const char *swb_offset_1024_window_p;
	const char *swb_offset_128_window_p;
	unsigned short *sect_sfb_offset_p;
	unsigned short *swb_offset_p;

	swb_offset_p = &ics->swb_offset[0];
	sect_sfb_offset_p = (unsigned short *)&ics->sect_start_end_cb_offset[3*8*15*8];
	sf_index = hDecoder->sf_index;

    switch (ics->window_sequence)
	{
    case ONLY_LONG_SEQUENCE:
    case LONG_START_SEQUENCE:
    case LONG_STOP_SEQUENCE:

		swb_offset_1024_window_p = (const char *)swb_offset_1024_window[sf_index];

		reg_idx = num_swb_1024_window[sf_index]; //only for hDecoder->frameLength = 1024

		ics->num_windows = 1;
        ics->num_window_groups = 1;
        ics->window_group_length[0] = 1;
		ics->num_swb = reg_idx;

        /* preparation of sect_sfb_offset for long blocks */
        /* also copy the last value! */
		for (; reg_idx != 0; reg_idx --)
        {
			int reg1;

			reg1 = *swb_offset_1024_window_p++;

			reg1 <<= 2;

            *sect_sfb_offset_p++ = reg1;
            *swb_offset_p++ = reg1;
        }

        *sect_sfb_offset_p = 1024;
        *swb_offset_p = 1024;

        return 0;

    case EIGHT_SHORT_SEQUENCE:

		swb_offset_128_window_p = (const char *)swb_offset_128_window[sf_index];

		reg_idx = num_swb_128_window[sf_index];

        ics->num_windows = 8;
        reg_num_group = 1;
        ics->window_group_length[0] = 1;
		ics->num_swb = reg_idx;

		for (i = reg_idx; i != 0; i --)
            *swb_offset_p++ = *swb_offset_128_window_p++;

        *swb_offset_p = 128;

		reg_sf_g = ics->scale_factor_grouping;

        for (i = 6; i >= 0; i --)
		{
            if (bit_set(reg_sf_g, i) == 0)
            {
				ics->window_group_length[reg_num_group] = 1;
                reg_num_group ++;
            }
			else
			{
                ics->window_group_length[reg_num_group-1] += 1;
            }
        }

		ics->num_window_groups = reg_num_group;

        /* preparation of sect_sfb_offset for short blocks */
        for (g = 0; g < reg_num_group; g ++)
        {
            int width;
            int sect_sfb = 0;
            int offset = 0;
			unsigned short *sect_start_end_cb_offset_tmp_p = (unsigned short *)&ics->sect_start_end_cb_offset[3*8*15*8];

            for (i = 0; i < reg_idx; i++)
            {
				int reg_tmp;

				reg_tmp = swb_offset_128_window[sf_index][i];

                if (i+1 == reg_idx)
                {
                    width = 128 - reg_tmp;
                }
				else
				{
                    width = swb_offset_128_window[sf_index][i+1] - reg_tmp;
                }
                width *= ics->window_group_length[g];
				sect_start_end_cb_offset_tmp_p[g*8*15+(sect_sfb++)] = offset;
                offset += width;
            }
			sect_start_end_cb_offset_tmp_p[g*8*15+sect_sfb] = offset;
        }
        return 0;
    default:
        return 1;
    }
}
#endif

/*
  For ONLY_LONG_SEQUENCE windows (num_window_groups = 1,
  window_group_length[0] = 1) the spectral data is in ascending spectral
  order.
  For the EIGHT_SHORT_SEQUENCE window, the spectral order depends on the
  grouping in the following manner:
  - Groups are ordered sequentially
  - Within a group, a scalefactor band consists of the spectral data of all
    grouped SHORT_WINDOWs for the associated scalefactor window band. To
    clarify via example, the length of a group is in the range of one to eight
    SHORT_WINDOWs.
  - If there are eight groups each with length one (num_window_groups = 8,
    window_group_length[0..7] = 1), the result is a sequence of eight spectra,
    each in ascending spectral order.
  - If there is only one group with length eight (num_window_groups = 1,
    window_group_length[0] = 8), the result is that spectral data of all eight
    SHORT_WINDOWs is interleaved by scalefactor window bands.
  - Within a scalefactor window band, the coefficients are in ascending
    spectral order.
*/
#if 0
static void quant_to_spec(ic_stream *ics, real_t *spec_data, unsigned short frame_len)
{
    char g, sfb, win;
    unsigned short width, bin, k, gindex;

    real_t tmp_spec[1024] = {0};

    k = 0;
    gindex = 0;

    for (g = 0; g < ics->num_window_groups; g++)
    {
        unsigned short j = 0;
        unsigned short gincrease = 0;
        unsigned short win_inc = ics->swb_offset[ics->num_swb];

        for (sfb = 0; sfb < ics->num_swb; sfb++)
        {
            width = ics->swb_offset[sfb+1] - ics->swb_offset[sfb];

            for (win = 0; win < ics->window_group_length[g]; win++)
            {
                for (bin = 0; bin < width; bin += 4)
                {
                    tmp_spec[gindex+(win*win_inc)+j+bin+0] = spec_data[k+0];
                    tmp_spec[gindex+(win*win_inc)+j+bin+1] = spec_data[k+1];
                    tmp_spec[gindex+(win*win_inc)+j+bin+2] = spec_data[k+2];
                    tmp_spec[gindex+(win*win_inc)+j+bin+3] = spec_data[k+3];
                    gincrease += 4;
                    k += 4;
                }
            }
            j += width;
        }
        gindex += gincrease;
    }

    memcpy(spec_data, tmp_spec, frame_len*sizeof(real_t));
}
#else
//Shawn 2004.8.28
extern volatile char sect_start_end_cb_offset_buf[2*(3*8*15*8 + 2*8*15*8)+2688];
//extern real_t cfft_work_buf[1024];

static void quant_to_spec(ic_stream *ics, real_t *spec_data, unsigned short frame_len)
{
    int g, sfb, win;
    int width, bin, gindex;
	int reg_window_group_length;
    real_t *tmp_spec;//[1024];//No need to init
	real_t *spec_data_p;
	real_t *tmp_spec_p;
	char *window_group_length_p;

	tmp_spec = (real_t *)&sect_start_end_cb_offset_buf[2*(3*8*15*8 + 2*8*15*8) - 1408];//&cfft_work_buf[0];

    spec_data_p = spec_data;
    gindex = 0;
	window_group_length_p = (char *)&ics->window_group_length[0];

    for (g = ics->num_window_groups; g != 0; g --)
    {
        int j = 0;
        int gincrease = 0;
        int win_inc = (int)ics->swb_offset[ics->num_swb];
		unsigned short *swb_offset_p;

		reg_window_group_length = *window_group_length_p++;
		swb_offset_p = &ics->swb_offset[0];

        for (sfb = ics->num_swb; sfb != 0; sfb --)
        {
            width = swb_offset_p[1] - swb_offset_p[0];swb_offset_p++;

            for (win = 0; win < reg_window_group_length; win++)
            {
				tmp_spec_p = (real_t *)&tmp_spec[gindex+(win*win_inc)+j];

                for (bin = width; bin != 0; bin -= 4)
                {
                    *tmp_spec_p++ = *spec_data_p++;
                    *tmp_spec_p++ = *spec_data_p++;
                    *tmp_spec_p++ = *spec_data_p++;
                    *tmp_spec_p++ = *spec_data_p++;
                }
				gincrease += width;
            }
            j += width;
        }
        gindex += gincrease;
    }

	memcpy(spec_data, tmp_spec, frame_len << 2);
}
#endif

#if 0
static INLINE real_t iquant(int16_t q, const real_t *tab, char *error)
{
#ifdef FIXED_POINT
    static const real_t errcorr[] = {
        REAL_CONST(0), REAL_CONST(1.0/8.0), REAL_CONST(2.0/8.0), REAL_CONST(3.0/8.0),
        REAL_CONST(4.0/8.0),  REAL_CONST(5.0/8.0), REAL_CONST(6.0/8.0), REAL_CONST(7.0/8.0),
        REAL_CONST(0)
    };
    real_t x1, x2;
    int16_t sgn = 1;

    if (q < 0)
    {
        q = -q;
        sgn = -1;
    }

    if (q < IQ_TABLE_SIZE)
        return sgn * tab[q];

    /* linear interpolation */
    x1 = tab[q>>3];
    x2 = tab[(q>>3) + 1];
    return sgn * 16 * (MUL_R(errcorr[q&7],(x2-x1)) + x1);
#else
    if (q < 0)
    {
        /* tab contains a value for all possible q [0,8192] */
        if (-q < IQ_TABLE_SIZE)
            return -tab[-q];

        *error = 17;
        return 0;
    } else {
        /* tab contains a value for all possible q [0,8192] */
        if (q < IQ_TABLE_SIZE)
            return tab[q];

        *error = 17;
        return 0;
    }
#endif
}
#else
//Shawn 2004.8.23
static INLINE real_t iquant(int16_t q)
{
    real_t x1, x2;
	real_t fixed_term, result;

    /*if (q < 0)
    {
        q = -q;

		if (q < IQ_TABLE_SIZE)
			result = -tab[q];
		else
		{
			x1 = tab[q>>3];
			x2 = tab[(q>>3) + 1];

			fixed_term = ((x2 - x1) << 1) * (q & 7);

			result = -((x1 << 4) + fixed_term);
		}
    }
	else
	{
		if (q < IQ_TABLE_SIZE)
			result = tab[q];
		else
		{
			x1 = tab[q>>3];
			x2 = tab[(q>>3) + 1];

			fixed_term = ((x2 - x1) << 1) * (q & 7);

			result = ((x1 << 4) + fixed_term);
		}
	}*/

	if ((unsigned)(q) <= (IQ_TABLE_SIZE-1))
	{
		result = iq_table[q];
	}
	else
	{
		if (q < 0)
		{
			q = -q;

			if ((unsigned)(q) <= (IQ_TABLE_SIZE-1))
			{
				result = -iq_table[q];
			}
			else
			{
				x1 = iq_table[q>>3];
				x2 = iq_table[(q>>3) + 1];

				fixed_term = ((x2 - x1) << 1) * (q & 7);

				result = -((x1 << 4) + fixed_term);
			}

		}
		else
		{
			x1 = iq_table[q>>3];
			x2 = iq_table[(q>>3) + 1];

			fixed_term = ((x2 - x1) << 1) * (q & 7);

			result = ((x1 << 4) + fixed_term);
		}
	}//must check again????!!!!!!!!! Shawn 2004.10.11


	return result;
}
#endif

#if 0
static char inverse_quantization(real_t *x_invquant, const int16_t *x_quant, const unsigned short frame_len)
{
    int16_t i;
    char error = 0; /* Init error flag */
    const real_t *tab = iq_table;

    for (i = 0; i < frame_len; i+=4)
    {
        x_invquant[i] = iquant(x_quant[i], tab, &error);
        x_invquant[i+1] = iquant(x_quant[i+1], tab, &error);
        x_invquant[i+2] = iquant(x_quant[i+2], tab, &error);
        x_invquant[i+3] = iquant(x_quant[i+3], tab, &error);
    }

    return error;
}
#else
//Shawn 2004.8.23
static void inverse_quantization(real_t *x_invquant_p, const int16_t *x_quant_p)
{
    int i;

    for (i = 1024; i != 0; i -= 4)
    {
        *x_invquant_p++ = iquant(*x_quant_p++);
        *x_invquant_p++ = iquant(*x_quant_p++);
        *x_invquant_p++ = iquant(*x_quant_p++);
        *x_invquant_p++ = iquant(*x_quant_p++);
    }
}
#endif

real_t pow2_table[] =
{
#if 0
    COEF_CONST(0.59460355750136053335874998528024), /* 2^-0.75 */
    COEF_CONST(0.70710678118654752440084436210485), /* 2^-0.5 */
    COEF_CONST(0.84089641525371454303112547623321), /* 2^-0.25 */
#endif
    COEF_CONST(1.0),
    COEF_CONST(1.1892071150027210667174999705605), /* 2^0.25 */
    COEF_CONST(1.4142135623730950488016887242097), /* 2^0.5 */
    COEF_CONST(1.6817928305074290860622509524664) /* 2^0.75 */
};

#if 0
void apply_scalefactors(GMAACDecHandle hDecoder, ic_stream *ics,
                        real_t *x_invquant, unsigned short frame_len)
{
    char g, sfb;
    unsigned short top;
    int32_t exp, frac;
    char groups = 0;
    unsigned short nshort = frame_len/8;

    for (g = 0; g < ics->num_window_groups; g++)
    {
        unsigned short k = 0;

        /* using this nshort*groups doesn't hurt long blocks, because
           long blocks only have 1 group, so that means 'groups' is
           always 0 for long blocks
        */
        for (sfb = 0; sfb < ics->max_sfb; sfb++)
        {
            top = ics->sect_sfb_offset[g][sfb+1];

            /* this could be scalefactor for IS or PNS, those can be negative or bigger then 255 */
            /* just ignore them */
            if (ics->scale_factors[g][sfb] < 0 || ics->scale_factors[g][sfb] > 255)
            {
                exp = 0;
                frac = 0;
            } else {
                /* ics->scale_factors[g][sfb] must be between 0 and 255 */
                exp = (ics->scale_factors[g][sfb] /* - 100 */) >> 2;
                frac = (ics->scale_factors[g][sfb] /* - 100 */) & 3;
            }

#ifdef FIXED_POINT
            exp -= 25;
            /* IMDCT pre-scaling */
            if (hDecoder->object_type == LD)
            {
                exp -= 6 /*9*/;
            } else {
                if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
                    exp -= 4 /*7*/;
                else
                    exp -= 7 /*10*/;
            }
#endif

            /* minimum size of a sf band is 4 and always a multiple of 4 */
            for ( ; k < top; k += 4)
            {
#ifdef FIXED_POINT
                if (exp < 0)
                {
                    x_invquant[k+(groups*nshort)] >>= -exp;
                    x_invquant[k+(groups*nshort)+1] >>= -exp;
                    x_invquant[k+(groups*nshort)+2] >>= -exp;
                    x_invquant[k+(groups*nshort)+3] >>= -exp;
                } else {
                    x_invquant[k+(groups*nshort)] <<= exp;
                    x_invquant[k+(groups*nshort)+1] <<= exp;
                    x_invquant[k+(groups*nshort)+2] <<= exp;
                    x_invquant[k+(groups*nshort)+3] <<= exp;
                }
#else
                x_invquant[k+(groups*nshort)]   = x_invquant[k+(groups*nshort)]   * pow2sf_tab[exp/*+25*/];
                x_invquant[k+(groups*nshort)+1] = x_invquant[k+(groups*nshort)+1] * pow2sf_tab[exp/*+25*/];
                x_invquant[k+(groups*nshort)+2] = x_invquant[k+(groups*nshort)+2] * pow2sf_tab[exp/*+25*/];
                x_invquant[k+(groups*nshort)+3] = x_invquant[k+(groups*nshort)+3] * pow2sf_tab[exp/*+25*/];
#endif

                x_invquant[k+(groups*nshort)]   = MUL_C(x_invquant[k+(groups*nshort)],pow2_table[frac /* + 3*/]);
                x_invquant[k+(groups*nshort)+1] = MUL_C(x_invquant[k+(groups*nshort)+1],pow2_table[frac /* + 3*/]);
                x_invquant[k+(groups*nshort)+2] = MUL_C(x_invquant[k+(groups*nshort)+2],pow2_table[frac /* + 3*/]);
                x_invquant[k+(groups*nshort)+3] = MUL_C(x_invquant[k+(groups*nshort)+3],pow2_table[frac /* + 3*/]);
            }
        }
        groups += ics->window_group_length[g];
    }
}
#else
//Shawn 2004.8.27
#ifndef ARM_ASM
void apply_scalefactors(GMAACDecHandle hDecoder, ic_stream *ics,
                        real_t *x_invquant, unsigned short frame_len)
{
    int g, sfb;
    int top;
    int exp, frac;
    int groups = 0;
    int nshort = frame_len >> 3;
	real_t reg_pow2;
	real_t *x_invquant_p;
	unsigned short *sect_sfb_offset_p;
	unsigned short *sect_sfb_offset_pp;
	int16_t *scale_factors_p, *scale_factors_pp;
	char *window_group_length_p;

	sect_sfb_offset_p = (unsigned short *)&ics->sect_start_end_cb_offset[3*8*15*8+1*2];

	scale_factors_pp = &ics->scale_factors[0];//(int16_t *)&ics->sect_start_end_cb_offset[3*8*15*8 + 2*8*15*8];//
	window_group_length_p = &ics->window_group_length[0];

    for (g = ics->num_window_groups; g != 0; g --, scale_factors_pp += 51)
    {
        int k = 0;

		scale_factors_p = scale_factors_pp;
		sect_sfb_offset_pp = sect_sfb_offset_p;

        /* using this nshort*groups doesn't hurt long blocks, because
           long blocks only have 1 group, so that means 'groups' is
           always 0 for long blocks
        */
        for (sfb = ics->max_sfb; sfb != 0; sfb --)
        {
			int reg_sf;

			//exp -= 25; sub with the next action
            /* IMDCT pre-scaling */
            /*if (hDecoder->object_type == LD)
            {
                exp -= 6 //9;
            } else *///no LD
			{
                if (ics->window_sequence != EIGHT_SHORT_SEQUENCE)
                    exp = -32 /*10*/;
                else
                    exp = -29 /*7*/;
            }

			reg_sf = *scale_factors_p++;

            /* this could be scalefactor for IS or PNS, those can be negative or bigger then 255 */
            /* just ignore them */
#if 0
			//if (reg_sf >= 0 && reg_sf <= 255)
			if ((unsigned)(reg_sf) <= 255)
            {
                /* ics->scale_factors[g][sfb] must be between 0 and 255 */
                exp += (reg_sf /* - 100 */) >> 2;
                frac = (reg_sf /* - 100 */) & 3;
            }
			else
			{
                frac = 0;
            }
#else
			//if (reg_sf >= 0 && reg_sf <= 255)
			if ((unsigned)(reg_sf) <= 255)
            {
                /* ics->scale_factors[g][sfb] must be between 0 and 255 */

            }
			else
			{
                frac = 0;
            }

			exp += (reg_sf /* - 100 */) >> 2;
            frac = (reg_sf /* - 100 */) & 3;
#endif

			reg_pow2 = pow2_table[frac /* + 3*/];


			x_invquant_p = (real_t *)&x_invquant[k+(groups*nshort)];

			top = *sect_sfb_offset_pp++;

            /* minimum size of a sf band is 4 and always a multiple of 4 */
            for ( ; k < top; k += 4)
            {
				real_t reg_x0, reg_x1, reg_x2, reg_x3;
				real_t lo, tmp0, hi0, tmp1, hi1, tmp2, hi2, tmp3, hi3;

                reg_x0 = x_invquant_p[0];
                reg_x1 = x_invquant_p[1];
                reg_x2 = x_invquant_p[2];
                reg_x3 = x_invquant_p[3];

				if (exp < 0)
                {
					int reg_exp = -exp;

					reg_x0 >>= reg_exp;
					reg_x1 >>= reg_exp;
					reg_x2 >>= reg_exp;
					reg_x3 >>= reg_exp;
                }
				else
				{
					reg_x0 <<= exp;
					reg_x1 <<= exp;
					reg_x2 <<= exp;
					reg_x3 <<= exp;
                }
#ifdef _WIN32
                *x_invquant_p++ = MUL_C(reg_x0, reg_pow2);
                *x_invquant_p++ = MUL_C(reg_x1, reg_pow2);
                *x_invquant_p++ = MUL_C(reg_x2, reg_pow2);
                *x_invquant_p++ = MUL_C(reg_x3, reg_pow2);
#else //ARM
//Shawn 2004.8.27
				__asm
				{
					SMULL	lo, hi0, reg_pow2, reg_x0
					MOV		tmp0, lo, lsr COEF_BITS
					ORR		reg_x0, tmp0, hi0, LSL #(32 - COEF_BITS)
					STR		reg_x0, [x_invquant_p], #4

					SMULL	lo, hi1, reg_pow2, reg_x1
					MOV		tmp1, lo, lsr COEF_BITS
					ORR		reg_x1, tmp1, hi1, LSL #(32 - COEF_BITS)
					STR		reg_x1, [x_invquant_p], #4

					SMULL	lo, hi2, reg_pow2, reg_x2
					MOV		tmp2, lo, lsr COEF_BITS
					ORR		reg_x2, tmp2, hi2, LSL #(32 - COEF_BITS)
					STR		reg_x2, [x_invquant_p], #4

					SMULL	lo, hi3, reg_pow2, reg_x3
					MOV		tmp3, lo, lsr COEF_BITS
					ORR		reg_x3, tmp3, hi3, LSL #(32 - COEF_BITS)
					STR		reg_x3, [x_invquant_p], #4
				}
#endif
            }
        }
		sect_sfb_offset_p += (8*15);
        groups += *window_group_length_p++;
    }
}
#endif
#endif

#if 0
static char allocate_single_channel(GMAACDecHandle hDecoder, char channel,
                                       char output_channels)
{
    char mul = 1;

#ifdef LTP_DEC
    if (is_ltp_ot(hDecoder->object_type))
    {
        /* allocate the state only when needed */
        if (hDecoder->lt_pred_stat[channel] == NULL)
        {
            hDecoder->lt_pred_stat[channel] = (int16_t*)faad_malloc(hDecoder->frameLength*4 * sizeof(int16_t));
            memset(hDecoder->lt_pred_stat[channel], 0, hDecoder->frameLength*4 * sizeof(int16_t));
        }
    }
#endif

    if (hDecoder->time_out[channel] == NULL)
    {
        mul = 1;

        hDecoder->time_out[channel] = (real_t*)faad_malloc(mul*hDecoder->frameLength*sizeof(real_t));
        memset(hDecoder->time_out[channel], 0, mul*hDecoder->frameLength*sizeof(real_t));
    }

    if (hDecoder->fb_intermed[channel] == NULL)
    {
        hDecoder->fb_intermed[channel] = (real_t*)faad_malloc(hDecoder->frameLength*sizeof(real_t));
        memset(hDecoder->fb_intermed[channel], 0, hDecoder->frameLength*sizeof(real_t));
    }

    return 0;
}
#else
//Shawn 2004.12.9

static char allocate_single_channel(GMAACDecHandle hDecoder, unsigned char channel,
                                       unsigned char output_channels)
{
    //int mul = 1;

#ifdef LTP_DEC
    if (is_ltp_ot(hDecoder->object_type))
    {
        /* allocate the state only when needed */
        if (hDecoder->lt_pred_stat[channel] == NULL)
        {
            hDecoder->lt_pred_stat[channel] = (int16_t*)faad_malloc(hDecoder->frameLength*4 * sizeof(int16_t));
            memset(hDecoder->lt_pred_stat[channel], 0, hDecoder->frameLength*4 * sizeof(int16_t));
        }
    }
#endif

    if (hDecoder->time_out[channel] == NULL)
    {
        //mul = 1;

        hDecoder->time_out[channel] = (real_t*)&time_out_buf[0*1024];//(real_t*)faad_malloc(mul*hDecoder->frameLength*sizeof(real_t));
        //memset(hDecoder->time_out[channel], 0, mul*hDecoder->frameLength*sizeof(real_t));
    }

    if (hDecoder->fb_intermed[channel] == NULL)
    {
        hDecoder->fb_intermed[channel] = (real_t*)&intermed_out_buf[0*1024];//(real_t*)faad_malloc(hDecoder->frameLength*sizeof(real_t));
        //memset(hDecoder->fb_intermed[channel], 0, hDecoder->frameLength*sizeof(real_t));
    }

    return 0;
}
#endif

#if 0
static char allocate_channel_pair(GMAACDecHandle hDecoder,
                                     char channel, char paired_channel)
{
    char mul = 1;

#ifdef LTP_DEC
    if (is_ltp_ot(hDecoder->object_type))
    {
        /* allocate the state only when needed */
        if (hDecoder->lt_pred_stat[channel] == NULL)
        {
            hDecoder->lt_pred_stat[channel] = (int16_t*)faad_malloc(hDecoder->frameLength*4 * sizeof(int16_t));
            memset(hDecoder->lt_pred_stat[channel], 0, hDecoder->frameLength*4 * sizeof(int16_t));
        }
        if (hDecoder->lt_pred_stat[paired_channel] == NULL)
        {
            hDecoder->lt_pred_stat[paired_channel] = (int16_t*)faad_malloc(hDecoder->frameLength*4 * sizeof(int16_t));
            memset(hDecoder->lt_pred_stat[paired_channel], 0, hDecoder->frameLength*4 * sizeof(int16_t));
        }
    }
#endif

    if (hDecoder->time_out[channel] == NULL)
    {
        mul = 1;

        hDecoder->time_out[channel] = (real_t*)faad_malloc(mul*hDecoder->frameLength*sizeof(real_t));
        memset(hDecoder->time_out[channel], 0, mul*hDecoder->frameLength*sizeof(real_t));
    }
    if (hDecoder->time_out[paired_channel] == NULL)
    {
        hDecoder->time_out[paired_channel] = (real_t*)faad_malloc(mul*hDecoder->frameLength*sizeof(real_t));
        memset(hDecoder->time_out[paired_channel], 0, mul*hDecoder->frameLength*sizeof(real_t));
    }

    if (hDecoder->fb_intermed[channel] == NULL)
    {
        hDecoder->fb_intermed[channel] = (real_t*)faad_malloc(hDecoder->frameLength*sizeof(real_t));
        memset(hDecoder->fb_intermed[channel], 0, hDecoder->frameLength*sizeof(real_t));
    }
    if (hDecoder->fb_intermed[paired_channel] == NULL)
    {
        hDecoder->fb_intermed[paired_channel] = (real_t*)faad_malloc(hDecoder->frameLength*sizeof(real_t));
        memset(hDecoder->fb_intermed[paired_channel], 0, hDecoder->frameLength*sizeof(real_t));
    }

    return 0;
}
#else
//Shawn 2004.10.12
static char allocate_channel_pair(GMAACDecHandle hDecoder,
                                     unsigned char channel, unsigned char paired_channel)
{
    //char mul = 1;

#ifdef LTP_DEC
    if (is_ltp_ot(hDecoder->object_type))
    {
        /* allocate the state only when needed */
        if (hDecoder->lt_pred_stat[channel] == NULL)
        {
            hDecoder->lt_pred_stat[channel] = (int16_t*)faad_malloc(hDecoder->frameLength*4 * sizeof(int16_t));
            memset(hDecoder->lt_pred_stat[channel], 0, hDecoder->frameLength*4 * sizeof(int16_t));
        }
        if (hDecoder->lt_pred_stat[paired_channel] == NULL)
        {
            hDecoder->lt_pred_stat[paired_channel] = (int16_t*)faad_malloc(hDecoder->frameLength*4 * sizeof(int16_t));
            memset(hDecoder->lt_pred_stat[paired_channel], 0, hDecoder->frameLength*4 * sizeof(int16_t));
        }
    }
#endif

    if (hDecoder->time_out[channel] == NULL)
    {
        //mul = 1;

		// use globle buffer
        hDecoder->time_out[channel] = (real_t*)&time_out_buf[0*1024];//(real_t*)faad_malloc(hDecoder->frameLength*sizeof(real_t));
        //memset(hDecoder->time_out[channel], 0, mul*hDecoder->frameLength*sizeof(real_t));// no needed to reset???
    }
    if (hDecoder->time_out[paired_channel] == NULL)
    {
		// use globle buffer
        hDecoder->time_out[paired_channel] = (real_t*)&time_out_buf[1*1024];//(real_t*)faad_malloc(hDecoder->frameLength*sizeof(real_t));
        //memset(hDecoder->time_out[paired_channel], 0, mul*hDecoder->frameLength*sizeof(real_t));// no needed to reset???
    }

    if (hDecoder->fb_intermed[channel] == NULL)
    {
		// use globle buffer
        hDecoder->fb_intermed[channel] = (real_t*)&intermed_out_buf[0*1024];//(real_t*)faad_malloc(hDecoder->frameLength*sizeof(real_t));
        //memset(hDecoder->fb_intermed[channel], 0, hDecoder->frameLength*sizeof(real_t));// no needed to reset???
    }
    if (hDecoder->fb_intermed[paired_channel] == NULL)
    {
		// use globle buffer
        hDecoder->fb_intermed[paired_channel] = (real_t*)&intermed_out_buf[1*1024];//(real_t*)faad_malloc(hDecoder->frameLength*sizeof(real_t));
        //memset(hDecoder->fb_intermed[paired_channel], 0, hDecoder->frameLength*sizeof(real_t));// no needed to reset???
    }

    return 0;
}
#endif

#if 0
char reconstruct_single_channel(GMAACDecHandle hDecoder, ic_stream *ics,
                                   element *sce, int16_t *spec_data)
{
    char retval, output_channels;
    real_t spec_coef[1024];

#ifdef PROFILE
    int64_t count = faad_get_ts();
#endif

    output_channels = 1;

    if (hDecoder->element_output_channels[hDecoder->fr_ch_ele] == 0)
    {
        /* element_output_channels not set yet */
        hDecoder->element_output_channels[hDecoder->fr_ch_ele] = output_channels;
    } else if (hDecoder->element_output_channels[hDecoder->fr_ch_ele] != output_channels) {
        /* element inconsistency */
        return 21;
    }


    if (hDecoder->element_alloced[hDecoder->fr_ch_ele] == 0)
    {
        retval = allocate_single_channel(hDecoder, sce->channel, output_channels);
        if (retval > 0)
            return retval;

        hDecoder->element_alloced[hDecoder->fr_ch_ele] = 1;
    }


    /* inverse quantization */
#if 0
    retval = inverse_quantization(spec_coef, spec_data, hDecoder->frameLength);
    if (retval > 0)
        return retval;
#else
//Shawn 2004.8.23
	inverse_quantization(spec_coef, spec_data);
#endif

    /* apply scalefactors */
    apply_scalefactors(hDecoder, ics, spec_coef, hDecoder->frameLength);

    /* deinterleave short block grouping */
    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
        quant_to_spec(ics, spec_coef, hDecoder->frameLength);

#ifdef PROFILE
    count = faad_get_ts() - count;
    hDecoder->requant_cycles += count;
#endif


    /* pns decoding */
#ifdef PNS_DEC
    pns_decode(ics, NULL, spec_coef, NULL, hDecoder->frameLength, 0, hDecoder->object_type);
#endif


#ifdef LTP_DEC
    if (is_ltp_ot(hDecoder->object_type))
    {
        /* long term prediction */
        lt_prediction(ics, &(ics->ltp), spec_coef, hDecoder->lt_pred_stat[sce->channel], hDecoder->fb,
            ics->window_shape, hDecoder->window_shape_prev[sce->channel],
            hDecoder->sf_index, hDecoder->object_type, hDecoder->frameLength);
    }
#endif

    /* tns decoding */
    tns_decode_frame(ics, &(ics->tns), hDecoder->sf_index, hDecoder->object_type,
        spec_coef, hDecoder->frameLength);

    /* drc decoding */
#ifdef DRC_DEC
    if (hDecoder->drc->present)
    {
        if (!hDecoder->drc->exclude_mask[sce->channel] || !hDecoder->drc->excluded_chns_present)
            drc_decode(hDecoder->drc, spec_coef);
    }
#endif


    /* filter bank */
        ifilter_bank(hDecoder->fb, ics->window_sequence, ics->window_shape,
            hDecoder->window_shape_prev[sce->channel], spec_coef,
            hDecoder->time_out[sce->channel], hDecoder->fb_intermed[sce->channel],
            hDecoder->object_type, hDecoder->frameLength);

    /* save window shape for next frame */
    hDecoder->window_shape_prev[sce->channel] = ics->window_shape;

#ifdef LTP_DEC
    if (is_ltp_ot(hDecoder->object_type))
    {
        lt_update_state(hDecoder->lt_pred_stat[sce->channel], hDecoder->time_out[sce->channel],
            hDecoder->fb_intermed[sce->channel], hDecoder->frameLength, hDecoder->object_type);
    }
#endif

    return 0;
}
#else
//Shawn 2004.12.9
char reconstruct_single_channel(GMAACDecHandle hDecoder, ic_stream *ics,
                                   element *sce, int16_t *spec_data)
{
    int retval, output_channels;
    #if 0 //GM8287 Audio driver limited the local variable size to 1024 bytes 
    real_t spec_coef[1024];
    #else
    real_t *spec_coef = (real_t *)&time_out_buf[0];
    #endif

    output_channels = 1;

    if (hDecoder->element_output_channels[hDecoder->fr_ch_ele] == 0)
    {
        /* element_output_channels not set yet */
        hDecoder->element_output_channels[hDecoder->fr_ch_ele] = output_channels;
    }
	else if (hDecoder->element_output_channels[hDecoder->fr_ch_ele] != output_channels)
	{
        /* element inconsistency */
        return 21;
    }

    if (hDecoder->element_alloced[hDecoder->fr_ch_ele] == 0)
    {
        retval = allocate_single_channel(hDecoder, sce->channel, output_channels);
        if (retval > 0)
            return retval;

        hDecoder->element_alloced[hDecoder->fr_ch_ele] = 1;
    }

    /* inverse quantization */
	inverse_quantization(spec_coef, spec_data);

    /* apply scalefactors */
    apply_scalefactors(hDecoder, ics, spec_coef, hDecoder->frameLength);

    /* deinterleave short block grouping */
    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
        quant_to_spec(ics, spec_coef, hDecoder->frameLength);

    /* pns decoding */
#ifdef PNS_DEC
    pns_decode(ics, NULL, spec_coef, NULL, hDecoder->frameLength, 0, hDecoder->object_type);
#endif


#ifdef LTP_DEC
    if (is_ltp_ot(hDecoder->object_type))
    {
        /* long term prediction */
        lt_prediction(ics, &(ics->ltp), spec_coef, hDecoder->lt_pred_stat[sce->channel], hDecoder->fb,
            ics->window_shape, hDecoder->window_shape_prev[sce->channel],
            hDecoder->sf_index, hDecoder->object_type, hDecoder->frameLength);
    }
#endif

    /* tns decoding */
    tns_decode_frame(ics, &(ics->tns), hDecoder->sf_index, hDecoder->object_type,
        spec_coef, hDecoder->frameLength);

    /* drc decoding */
#ifdef DRC_DEC
    if (hDecoder->drc->present)
    {
        if (!hDecoder->drc->exclude_mask[sce->channel] || !hDecoder->drc->excluded_chns_present)
            drc_decode(hDecoder->drc, spec_coef);
    }
#endif

	//Equalizer
	if (eq_type != -1)
	{

		if (ics->window_sequence != EIGHT_SHORT_SEQUENCE)
		{

			EQ_function_long(spec_coef);
		}
		else
		{
			EQ_function_short(spec_coef);
		}
	}

    /* filter bank */
        ifilter_bank(hDecoder->fb, ics->window_sequence, ics->window_shape,
            hDecoder->window_shape_prev[sce->channel], spec_coef,
            hDecoder->time_out[sce->channel], hDecoder->fb_intermed[sce->channel],
            hDecoder->object_type, hDecoder->frameLength);

    /* save window shape for next frame */
    hDecoder->window_shape_prev[sce->channel] = ics->window_shape;

#ifdef LTP_DEC
    if (is_ltp_ot(hDecoder->object_type))
    {
        lt_update_state(hDecoder->lt_pred_stat[sce->channel], hDecoder->time_out[sce->channel],
            hDecoder->fb_intermed[sce->channel], hDecoder->frameLength, hDecoder->object_type);
    }
#endif

    return 0;
}
#endif

#if 0
char reconstruct_channel_pair(GMAACDecHandle hDecoder, ic_stream *ics1, ic_stream *ics2,
                                 element *cpe, int16_t *spec_data1, int16_t *spec_data2)
{
    char retval;
    real_t spec_coef1[1024];
    real_t spec_coef2[1024];

#ifdef PROFILE
    int64_t count = faad_get_ts();
#endif
    if (hDecoder->element_alloced[hDecoder->fr_ch_ele] == 0)
    {
        retval = allocate_channel_pair(hDecoder, cpe->channel, cpe->paired_channel);
        if (retval > 0)
            return retval;

        hDecoder->element_alloced[hDecoder->fr_ch_ele] = 1;
    }

    /* inverse quantization */
#if 0
    retval = inverse_quantization(spec_coef1, spec_data1, hDecoder->frameLength);
    if (retval > 0)
        return retval;
	retval = inverse_quantization(spec_coef2, spec_data2, hDecoder->frameLength);
    if (retval > 0)
        return retval;
#else
//Shawn 2004.8.23
    inverse_quantization(spec_coef1, spec_data1, hDecoder->frameLength);
	inverse_quantization(spec_coef2, spec_data2, hDecoder->frameLength);
#endif



    /* apply scalefactors */
    apply_scalefactors(hDecoder, ics1, spec_coef1, hDecoder->frameLength);
    apply_scalefactors(hDecoder, ics2, spec_coef2, hDecoder->frameLength);

    /* deinterleave short block grouping */
    if (ics1->window_sequence == EIGHT_SHORT_SEQUENCE)
        quant_to_spec(ics1, spec_coef1, hDecoder->frameLength);
    if (ics2->window_sequence == EIGHT_SHORT_SEQUENCE)
        quant_to_spec(ics2, spec_coef2, hDecoder->frameLength);

#ifdef PROFILE
    count = faad_get_ts() - count;
    hDecoder->requant_cycles += count;
#endif


    /* pns decoding */
    /*if (ics1->ms_mask_present)
    {
        pns_decode(ics1, ics2, spec_coef1, spec_coef2, hDecoder->frameLength, 1, hDecoder->object_type);
    } else {
        pns_decode(ics1, NULL, spec_coef1, NULL, hDecoder->frameLength, 0, hDecoder->object_type);
        pns_decode(ics2, NULL, spec_coef2, NULL, hDecoder->frameLength, 0, hDecoder->object_type);
    }*///no PNS version

    /* mid/side decoding */
    ms_decode(ics1, ics2, spec_coef1, spec_coef2, hDecoder->frameLength);

    /* intensity stereo decoding */
    is_decode(ics1, ics2, spec_coef1, spec_coef2, hDecoder->frameLength);

#ifdef LTP_DEC
    if (is_ltp_ot(hDecoder->object_type))
    {
        ltp_info *ltp1 = &(ics1->ltp);
        ltp_info *ltp2 = (cpe->common_window) ? &(ics2->ltp2) : &(ics2->ltp);

        /* long term prediction */
        lt_prediction(ics1, ltp1, spec_coef1, hDecoder->lt_pred_stat[cpe->channel], hDecoder->fb,
            ics1->window_shape, hDecoder->window_shape_prev[cpe->channel],
            hDecoder->sf_index, hDecoder->object_type, hDecoder->frameLength);
        lt_prediction(ics2, ltp2, spec_coef2, hDecoder->lt_pred_stat[cpe->paired_channel], hDecoder->fb,
            ics2->window_shape, hDecoder->window_shape_prev[cpe->paired_channel],
            hDecoder->sf_index, hDecoder->object_type, hDecoder->frameLength);
    }
#endif

    /* tns decoding */
    tns_decode_frame(ics1, &(ics1->tns), hDecoder->sf_index, hDecoder->object_type,
        spec_coef1, hDecoder->frameLength);
    tns_decode_frame(ics2, &(ics2->tns), hDecoder->sf_index, hDecoder->object_type,
        spec_coef2, hDecoder->frameLength);

    /* drc decoding */
    if (hDecoder->drc->present)
    {
        if (!hDecoder->drc->exclude_mask[cpe->channel] || !hDecoder->drc->excluded_chns_present)
            drc_decode(hDecoder->drc, spec_coef1);
        if (!hDecoder->drc->exclude_mask[cpe->paired_channel] || !hDecoder->drc->excluded_chns_present)
            drc_decode(hDecoder->drc, spec_coef2);
    }

    /* filter bank */
        ifilter_bank(hDecoder->fb, ics1->window_sequence, ics1->window_shape,
            hDecoder->window_shape_prev[cpe->channel], spec_coef1,
            hDecoder->time_out[cpe->channel], hDecoder->fb_intermed[cpe->channel],
            hDecoder->object_type, hDecoder->frameLength);
        ifilter_bank(hDecoder->fb, ics2->window_sequence, ics2->window_shape,
            hDecoder->window_shape_prev[cpe->paired_channel], spec_coef2,
            hDecoder->time_out[cpe->paired_channel], hDecoder->fb_intermed[cpe->paired_channel],
            hDecoder->object_type, hDecoder->frameLength);

    /* save window shape for next frame */
    hDecoder->window_shape_prev[cpe->channel] = ics1->window_shape;
    hDecoder->window_shape_prev[cpe->paired_channel] = ics2->window_shape;

#ifdef LTP_DEC
    if (is_ltp_ot(hDecoder->object_type))
    {
        lt_update_state(hDecoder->lt_pred_stat[cpe->channel], hDecoder->time_out[cpe->channel],
            hDecoder->fb_intermed[cpe->channel], hDecoder->frameLength, hDecoder->object_type);
        lt_update_state(hDecoder->lt_pred_stat[cpe->paired_channel], hDecoder->time_out[cpe->paired_channel],
            hDecoder->fb_intermed[cpe->paired_channel], hDecoder->frameLength, hDecoder->object_type);
    }
#endif

    return 0;
}
#else
//Shawn 2004.10.12
int reconstruct_channel_pair(GMAACDecHandle hDecoder, ic_stream *ics1, ic_stream *ics2,
                                 element *cpe, int16_t *spec_data1, int16_t *spec_data2)
{
    int retval;
    real_t *spec_coef1;
    real_t *spec_coef2;

	spec_coef1 = (real_t *)&time_out_buf[0];	// use time out buffer as spectrum buffer
	spec_coef2 = (real_t *)&time_out_buf[1024]; // use time out buffer as spectrum buffer

    /* allocate time out and overlap buffer */
	if (hDecoder->element_alloced[hDecoder->fr_ch_ele] == 0)
    {
        retval = allocate_channel_pair(hDecoder, cpe->channel, cpe->paired_channel);

        if (retval > 0)
            return retval;

        hDecoder->element_alloced[hDecoder->fr_ch_ele] = 1;
    }

    /* inverse quantization */
    inverse_quantization(spec_coef1, spec_data1);
	inverse_quantization(spec_coef2, spec_data2);

    /* apply scalefactors */
    apply_scalefactors(hDecoder, ics1, spec_coef1, hDecoder->frameLength);
    apply_scalefactors(hDecoder, ics2, spec_coef2, hDecoder->frameLength);

    /* deinterleave short block grouping */
    if (ics1->window_sequence == EIGHT_SHORT_SEQUENCE)
        quant_to_spec(ics1, spec_coef1, hDecoder->frameLength);
    if (ics2->window_sequence == EIGHT_SHORT_SEQUENCE)
        quant_to_spec(ics2, spec_coef2, hDecoder->frameLength);

    /* pns decoding */
#ifdef PNS_DEC
    if (ics1->ms_mask_present)
    {
        pns_decode(ics1, ics2, spec_coef1, spec_coef2, hDecoder->frameLength, 1, hDecoder->object_type);
    } else {
        pns_decode(ics1, NULL, spec_coef1, NULL, hDecoder->frameLength, 0, hDecoder->object_type);
        pns_decode(ics2, NULL, spec_coef2, NULL, hDecoder->frameLength, 0, hDecoder->object_type);
    }//no PNS version
#endif

    /* mid/side decoding */
    ms_decode(ics1, ics2, spec_coef1, spec_coef2, hDecoder->frameLength);

    /* intensity stereo decoding */
    is_decode(ics1, ics2, spec_coef1, spec_coef2, hDecoder->frameLength);

#ifdef LTP_DEC
    if (is_ltp_ot(hDecoder->object_type))
    {
        ltp_info *ltp1 = &(ics1->ltp);
        ltp_info *ltp2 = (cpe->common_window) ? &(ics2->ltp2) : &(ics2->ltp);

        /* long term prediction */
        lt_prediction(ics1, ltp1, spec_coef1, hDecoder->lt_pred_stat[cpe->channel], hDecoder->fb,
            ics1->window_shape, hDecoder->window_shape_prev[cpe->channel],
            hDecoder->sf_index, hDecoder->object_type, hDecoder->frameLength);
        lt_prediction(ics2, ltp2, spec_coef2, hDecoder->lt_pred_stat[cpe->paired_channel], hDecoder->fb,
            ics2->window_shape, hDecoder->window_shape_prev[cpe->paired_channel],
            hDecoder->sf_index, hDecoder->object_type, hDecoder->frameLength);
    }
#endif

    /* tns decoding */
    tns_decode_frame(ics1, &(ics1->tns), hDecoder->sf_index, hDecoder->object_type,
        spec_coef1, hDecoder->frameLength);
    tns_decode_frame(ics2, &(ics2->tns), hDecoder->sf_index, hDecoder->object_type,
        spec_coef2, hDecoder->frameLength);

    /* drc decoding */
#ifdef DRC_DEC
    if (hDecoder->drc->present)
    {
        if (!hDecoder->drc->exclude_mask[cpe->channel] || !hDecoder->drc->excluded_chns_present)
            drc_decode(hDecoder->drc, spec_coef1);
        if (!hDecoder->drc->exclude_mask[cpe->paired_channel] || !hDecoder->drc->excluded_chns_present)
            drc_decode(hDecoder->drc, spec_coef2);
    }
#endif

	//Equalizer
	if (eq_type != -1)
	{

		if (ics1->window_sequence != EIGHT_SHORT_SEQUENCE)
		{

			EQ_function_long(spec_coef1);
		}
		else
		{
			EQ_function_short(spec_coef1);
		}

		if (ics2->window_sequence != EIGHT_SHORT_SEQUENCE)
		{
			EQ_function_long(spec_coef2);
		}
		else
		{
			EQ_function_short(spec_coef2);
		}
	}

    /* filter bank */
        ifilter_bank(hDecoder->fb, ics1->window_sequence, ics1->window_shape,
            hDecoder->window_shape_prev[cpe->channel], spec_coef1,
            hDecoder->time_out[cpe->channel], hDecoder->fb_intermed[cpe->channel],
            hDecoder->object_type, hDecoder->frameLength);
        ifilter_bank(hDecoder->fb, ics2->window_sequence, ics2->window_shape,
            hDecoder->window_shape_prev[cpe->paired_channel], spec_coef2,
            hDecoder->time_out[cpe->paired_channel], hDecoder->fb_intermed[cpe->paired_channel],
            hDecoder->object_type, hDecoder->frameLength);

    /* save window shape for next frame */
    hDecoder->window_shape_prev[cpe->channel] = ics1->window_shape;
    hDecoder->window_shape_prev[cpe->paired_channel] = ics2->window_shape;

#ifdef LTP_DEC
    if (is_ltp_ot(hDecoder->object_type))
    {
        lt_update_state(hDecoder->lt_pred_stat[cpe->channel], hDecoder->time_out[cpe->channel],
            hDecoder->fb_intermed[cpe->channel], hDecoder->frameLength, hDecoder->object_type);
        lt_update_state(hDecoder->lt_pred_stat[cpe->paired_channel], hDecoder->time_out[cpe->paired_channel],
            hDecoder->fb_intermed[cpe->paired_channel], hDecoder->frameLength, hDecoder->object_type);
    }
#endif

    return 0;
}
#endif
