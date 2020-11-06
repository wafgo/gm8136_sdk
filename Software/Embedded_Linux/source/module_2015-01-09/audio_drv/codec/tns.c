#include "common.h"
#include "structs.h"
#include "syntax.h"
#include "tns.h"

/* static function declarations */
#ifndef ARM_ASM
static void tns_decode_coef(char order, char coef_res_bits, char coef_compress, char *coef, real_t *a);
static void tns_ar_filter(real_t *spectrum, unsigned short size, signed char inc, real_t *lpc, char order);
#endif
#ifdef LTP_DEC
static void tns_ma_filter(real_t *spectrum, unsigned short size, signed char inc, real_t *lpc, char order);
#endif

#if 0
static real_t tns_coef_0_3[] =
{
    COEF_CONST(0.0), COEF_CONST(0.4338837391), COEF_CONST(0.7818314825), COEF_CONST(0.9749279122),
    COEF_CONST(-0.9848077530), COEF_CONST(-0.8660254038), COEF_CONST(-0.6427876097), COEF_CONST(-0.3420201433),
    COEF_CONST(-0.4338837391), COEF_CONST(-0.7818314825), COEF_CONST(-0.9749279122), COEF_CONST(-0.9749279122),
    COEF_CONST(-0.9848077530), COEF_CONST(-0.8660254038), COEF_CONST(-0.6427876097), COEF_CONST(-0.3420201433)
};
static real_t tns_coef_0_4[] =
{
    COEF_CONST(0.0), COEF_CONST(0.2079116908), COEF_CONST(0.4067366431), COEF_CONST(0.5877852523),
    COEF_CONST(0.7431448255), COEF_CONST(0.8660254038), COEF_CONST(0.9510565163), COEF_CONST(0.9945218954),
    COEF_CONST(-0.9957341763), COEF_CONST(-0.9618256432), COEF_CONST(-0.8951632914), COEF_CONST(-0.7980172273),
    COEF_CONST(-0.6736956436), COEF_CONST(-0.5264321629), COEF_CONST(-0.3612416662), COEF_CONST(-0.1837495178)
};
static real_t tns_coef_1_3[] =
{
    COEF_CONST(0.0), COEF_CONST(0.4338837391), COEF_CONST(-0.6427876097), COEF_CONST(-0.3420201433),
    COEF_CONST(0.9749279122), COEF_CONST(0.7818314825), COEF_CONST(-0.6427876097), COEF_CONST(-0.3420201433),
    COEF_CONST(-0.4338837391), COEF_CONST(-0.7818314825), COEF_CONST(-0.6427876097), COEF_CONST(-0.3420201433),
    COEF_CONST(-0.7818314825), COEF_CONST(-0.4338837391), COEF_CONST(-0.6427876097), COEF_CONST(-0.3420201433)
};
static real_t tns_coef_1_4[] =
{
    COEF_CONST(0.0), COEF_CONST(0.2079116908), COEF_CONST(0.4067366431), COEF_CONST(0.5877852523),
    COEF_CONST(-0.6736956436), COEF_CONST(-0.5264321629), COEF_CONST(-0.3612416662), COEF_CONST(-0.1837495178),
    COEF_CONST(0.9945218954), COEF_CONST(0.9510565163), COEF_CONST(0.8660254038), COEF_CONST(0.7431448255),
    COEF_CONST(-0.6736956436), COEF_CONST(-0.5264321629), COEF_CONST(-0.3612416662), COEF_CONST(-0.1837495178)
};
#else
//Shawn 2004.12.7
real_t tns_coef[2][2][16] =
{
{
	{
		COEF_CONST(0.0), COEF_CONST(0.4338837391), COEF_CONST(0.7818314825), COEF_CONST(0.9749279122),
		COEF_CONST(-0.9848077530), COEF_CONST(-0.8660254038), COEF_CONST(-0.6427876097), COEF_CONST(-0.3420201433),
		COEF_CONST(-0.4338837391), COEF_CONST(-0.7818314825), COEF_CONST(-0.9749279122), COEF_CONST(-0.9749279122),
		COEF_CONST(-0.9848077530), COEF_CONST(-0.8660254038), COEF_CONST(-0.6427876097), COEF_CONST(-0.3420201433)
	},
	{
		COEF_CONST(0.0), COEF_CONST(0.2079116908), COEF_CONST(0.4067366431), COEF_CONST(0.5877852523),
		COEF_CONST(0.7431448255), COEF_CONST(0.8660254038), COEF_CONST(0.9510565163), COEF_CONST(0.9945218954),
		COEF_CONST(-0.9957341763), COEF_CONST(-0.9618256432), COEF_CONST(-0.8951632914), COEF_CONST(-0.7980172273),
		COEF_CONST(-0.6736956436), COEF_CONST(-0.5264321629), COEF_CONST(-0.3612416662), COEF_CONST(-0.1837495178)
	}
},
{
	{
		COEF_CONST(0.0), COEF_CONST(0.4338837391), COEF_CONST(-0.6427876097), COEF_CONST(-0.3420201433),
		COEF_CONST(0.9749279122), COEF_CONST(0.7818314825), COEF_CONST(-0.6427876097), COEF_CONST(-0.3420201433),
		COEF_CONST(-0.4338837391), COEF_CONST(-0.7818314825), COEF_CONST(-0.6427876097), COEF_CONST(-0.3420201433),
		COEF_CONST(-0.7818314825), COEF_CONST(-0.4338837391), COEF_CONST(-0.6427876097), COEF_CONST(-0.3420201433)
	},
	{
		COEF_CONST(0.0), COEF_CONST(0.2079116908), COEF_CONST(0.4067366431), COEF_CONST(0.5877852523),
		COEF_CONST(-0.6736956436), COEF_CONST(-0.5264321629), COEF_CONST(-0.3612416662), COEF_CONST(-0.1837495178),
		COEF_CONST(0.9945218954), COEF_CONST(0.9510565163), COEF_CONST(0.8660254038), COEF_CONST(0.7431448255),
		COEF_CONST(-0.6736956436), COEF_CONST(-0.5264321629), COEF_CONST(-0.3612416662), COEF_CONST(-0.1837495178)
	}
}
};
#endif

#if 0
/* TNS decoding for one channel and frame */
void tns_decode_frame(ic_stream *ics, tns_info *tns, char sr_index, char object_type, real_t *spec, unsigned short frame_len)
{
    char w, f, tns_order;
    signed char inc;
    short size;
    unsigned short bottom, top, start, end;
    unsigned short nshort = frame_len/8;
    real_t lpc[TNS_MAX_ORDER+1];

    if (!ics->tns_data_present)
        return;

    for (w = 0; w < ics->num_windows; w++)
    {
        bottom = ics->num_swb;

        for (f = 0; f < tns->n_filt[w]; f++)
        {
			int reg_window_sequence;

            top = bottom;
            bottom = max(top - tns->length[w][f], 0);
            tns_order = min(tns->order[w][f], TNS_MAX_ORDER);

            if (!tns_order)
                continue;

            tns_decode_coef(tns_order, tns->coef_res[w]+3,
                tns->coef_compress[w][f], tns->coef[w][f], lpc);

			reg_window_sequence = (int)ics->window_sequence;

            start = min(bottom, max_tns_sfb(sr_index, object_type, (reg_window_sequence == EIGHT_SHORT_SEQUENCE)));
            start = min(start, ics->max_sfb);
            start = ics->swb_offset[start];

            end = min(top, max_tns_sfb(sr_index, object_type, (reg_window_sequence == EIGHT_SHORT_SEQUENCE)));
            end = min(end, ics->max_sfb);
            end = ics->swb_offset[end];

            size = end - start;
            if (size <= 0)
                continue;

            if (tns->direction[w][f])
            {
                inc = -1;
                start = end - 1;
            } else {
                inc = 1;
            }

            tns_ar_filter(&spec[(w*nshort)+start], size, inc, lpc, tns_order);
        }
    }
}
#else
//Shawn 2004.9.29
#ifndef ARM_ASM
/* TNS decoding for one channel and frame */
extern const char tns_sbf_max[][2];

static INLINE char max_tns_sfb_INLINE(const char sr_index, const char object_type, const char is_short)
{
    /* entry for each sampling rate
     * 1    Main/LC long window
     * 2    Main/LC short window
     * 3    SSR long window
     * 4    SSR short window
     */

    if (is_short) return tns_sbf_max[sr_index][1];
	else return tns_sbf_max[sr_index][0];


}

void tns_decode_frame(ic_stream *ics, tns_info *tns, char sr_index, char object_type, real_t *spec, unsigned short frame_len)
{
    int w, f, tns_order;
    int inc;
    int size;
    int bottom, top, start, end;
    //int nshort = 128;//frame_len/8;
    real_t lpc[TNS_MAX_ORDER+1];

    if (!ics->tns_data_present)
        return;

    for (w = 0; w < ics->num_windows; w++)
    {
        bottom = ics->num_swb;

        for (f = 0; f < tns->n_filt[w]; f++)
        {
            top = bottom;
            bottom = max(top - tns->length[w][f], 0);
            tns_order = min(tns->order[w][f], TNS_MAX_ORDER);

            if (!tns_order)
                continue;

            tns_decode_coef(tns_order, tns->coef_res[w]+3, tns->coef_compress[w][f], tns->coef[w][f], lpc);

            start = min(bottom, max_tns_sfb_INLINE(sr_index, object_type, (ics->window_sequence == EIGHT_SHORT_SEQUENCE)));
            start = min(start, ics->max_sfb);
            start = ics->swb_offset[start];

            end = min(top, max_tns_sfb_INLINE(sr_index, object_type, (ics->window_sequence == EIGHT_SHORT_SEQUENCE)));
            end = min(end, ics->max_sfb);
            end = ics->swb_offset[end];

            size = end - start;

            if (size <= 0)
                continue;

            if (tns->direction[w][f])
            {
                inc = -1;
                start = end - 1;
            }
			else
			{
                inc = 1;
            }

            tns_ar_filter(&spec[(w*128)+start], size, inc, lpc, tns_order);
        }
    }
}
#endif
#endif

/* Decoder transmitted coefficients for one TNS filter */
#if 0
static void tns_decode_coef(char order, char coef_res_bits, char coef_compress, char *coef, real_t *a)
{
    char i, m;
    real_t tmp2[TNS_MAX_ORDER+1], b[TNS_MAX_ORDER+1];

    /* Conversion to signed integer */
    for (i = 0; i < order; i++)
    {
        if (coef_compress == 0)
        {
            if (coef_res_bits == 3)
            {
                tmp2[i] = tns_coef_0_3[coef[i]];
            } else {
                tmp2[i] = tns_coef_0_4[coef[i]];
            }
        } else {
            if (coef_res_bits == 3)
            {
                tmp2[i] = tns_coef_1_3[coef[i]];
            } else {
                tmp2[i] = tns_coef_1_4[coef[i]];
            }
        }
    }

    /* Conversion to LPC coefficients */
    a[0] = COEF_CONST(1.0);
    for (m = 1; m <= order; m++)
    {
        for (i = 1; i < m; i++) /* loop only while i<m */
            b[i] = a[i] + MUL_C(tmp2[m-1], a[m-i]);

        for (i = 1; i < m; i++) /* loop only while i<m */
            a[i] = b[i];

        a[m] = tmp2[m-1]; /* changed */
    }
}
#else
#ifndef ARM_ASM
static void tns_decode_coef(char order, char coef_res_bits, char coef_compress, char *coef_p, real_t *a)
{
    int i, m;
    real_t b[TNS_MAX_ORDER+1];
	real_t *tns_coef_p;
	real_t *a_p, *b_p;

	tns_coef_p = &tns_coef[coef_compress][coef_res_bits-3][0];

    /* Conversion to LPC coefficients */
    a[0] = COEF_CONST(1.0);

    for (m = 1; m <= order; m++)
    {
		int N_coef;
		real_t reg_tmp2;

		N_coef = *coef_p++;

        reg_tmp2 = tns_coef_p[N_coef];

        for (i = 1; i < m; i++) // loop only while i<m
		{

#ifdef _WIN32
			b[i] = a[i] + MUL_C(reg_tmp2, a[m-i]);
#else
			int lo, hi, tmp, r;
			int reg_a0, reg_a1;

			reg_a0 = a[i];
			reg_a1 = a[m-i];

			__asm
			{
				SMULL	lo, hi, reg_tmp2, reg_a1
				MOV		tmp, lo, LSR COEF_BITS
				ORR		r, tmp, hi, LSL (32 - COEF_BITS)
				ADD		r, r, reg_a0
			}

			b[i] = r;
#endif
		}

		b_p = &b[1];
		a_p = &a[1];

        for (i = m - 1; i != 0; i --) // loop only while i<m
            *a_p++ = *b_p++;
		//memcpy(&a[1], &b[1], (m-1) << 2);

        *a_p = reg_tmp2; /* changed */
    }
}
#endif
#endif

#if 0
static void tns_ar_filter(real_t *spectrum, unsigned short size, signed char inc, real_t *lpc, char order)
{
    /*
     - Simple all-pole filter of order "order" defined by
       y(n) = x(n) - lpc[1]*y(n-1) - ... - lpc[order]*y(n-order)
     - The state variables of the filter are initialized to zero every time
     - The output data is written over the input data ("in-place operation")
     - An input vector of "size" samples is processed and the index increment
       to the next data sample is given by "inc"
    */

    char j;
    unsigned short i;
    real_t y, state[TNS_MAX_ORDER];

    for (i = 0; i < order; i++)
        state[i] = 0;

    for (i = 0; i < size; i++)
    {
        y = *spectrum;

        for (j = 0; j < order; j++)
            y -= MUL_C(state[j], lpc[j+1]);

        for (j = order-1; j > 0; j--)
            state[j] = state[j-1];

        state[0] = y;
        *spectrum = y;
        spectrum += inc;
    }
}
#else
//Shawn 2004.12.7
#ifndef ARM_ASM
static void tns_ar_filter(real_t *spectrum, unsigned short size, signed char inc, real_t *lpc, char order)
{
    /*
     - Simple all-pole filter of order "order" defined by
       y(n) = x(n) - lpc[1]*y(n-1) - ... - lpc[order]*y(n-order)
     - The state variables of the filter are initialized to zero every time
     - The output data is written over the input data ("in-place operation")
     - An input vector of "size" samples is processed and the index increment
       to the next data sample is given by "inc"
    */

    int j, i;
    real_t y, state[TNS_MAX_ORDER];
	real_t *state_p, *lpc_p;

	state_p = &state[0];

    for (i = order; i != 0; i --)
        *state_p++ = 0;
	//memset(&state[0], 0, order << 2);

    for (i = size; i != 0; i --)
    {
        y = *spectrum;

		state_p = &state[0];
		lpc_p = &lpc[1];

        for (j = order; j != 0; j --)
		{
			int reg_state, reg_lpc;
#ifdef _WIN32
			reg_state = *state_p++;
			reg_lpc = *lpc_p++;

			y -= MUL_C(reg_state, reg_lpc);
#else
			int lo, hi, tmp, r;

			__asm
			{
				LDR		reg_state, [state_p], #4
				LDR		reg_lpc, [lpc_p], #4
				SMULL	lo, hi, reg_lpc, reg_state
				MOV		tmp, lo, LSR COEF_BITS
				ORR		r, tmp, hi, LSL (32 - COEF_BITS)
				SUB		y, y, r
			}
#endif
		}

		state_p --;

        for (j = order-1; j != 0; j--)
		{
			real_t reg_state;

			reg_state = state_p[-1];

            *state_p-- = reg_state;
		}

        *state_p = y;
        *spectrum = y;
        spectrum += inc;
    }
}
#endif
#endif



// not used in Faraday MP-2/4 AAC Decoder

#ifdef LTP_DEC
/* TNS encoding for one channel and frame */
void tns_encode_frame(ic_stream *ics, tns_info *tns, char sr_index, char object_type, real_t *spec, unsigned short frame_len)
{
    char w, f, tns_order;
    signed char inc;
    short size;
    unsigned short bottom, top, start, end;
    unsigned short nshort = frame_len/8;
    real_t lpc[TNS_MAX_ORDER+1];

    if (!ics->tns_data_present)
        return;

    for (w = 0; w < ics->num_windows; w++)
    {
        bottom = ics->num_swb;

        for (f = 0; f < tns->n_filt[w]; f++)
        {
            top = bottom;
            bottom = max(top - tns->length[w][f], 0);
            tns_order = min(tns->order[w][f], TNS_MAX_ORDER);
            if (!tns_order)
                continue;

            tns_decode_coef(tns_order, tns->coef_res[w]+3,
                tns->coef_compress[w][f], tns->coef[w][f], lpc);

            start = min(bottom, max_tns_sfb(sr_index, object_type, (ics->window_sequence == EIGHT_SHORT_SEQUENCE)));
            start = min(start, ics->max_sfb);
            start = ics->swb_offset[start];

            end = min(top, max_tns_sfb(sr_index, object_type, (ics->window_sequence == EIGHT_SHORT_SEQUENCE)));
            end = min(end, ics->max_sfb);
            end = ics->swb_offset[end];

            size = end - start;
            if (size <= 0)
                continue;

            if (tns->direction[w][f])
            {
                inc = -1;
                start = end - 1;
            } else {
                inc = 1;
            }

            tns_ma_filter(&spec[(w*nshort)+start], size, inc, lpc, tns_order);
        }
    }
}

static void tns_ma_filter(real_t *spectrum, unsigned short size, signed char inc, real_t *lpc, char order)
{
    /*
     - Simple all-zero filter of order "order" defined by
       y(n) =  x(n) + a(2)*x(n-1) + ... + a(order+1)*x(n-order)
     - The state variables of the filter are initialized to zero every time
     - The output data is written over the input data ("in-place operation")
     - An input vector of "size" samples is processed and the index increment
       to the next data sample is given by "inc"
    */

    char j;
    unsigned short i;
    real_t y, state[TNS_MAX_ORDER];

    for (i = 0; i < order; i++)
        state[i] = REAL_CONST(0.0);

    for (i = 0; i < size; i++)
    {
        y = *spectrum;

        for (j = 0; j < order; j++)
            y += MUL_C(state[j], lpc[j+1]);

        for (j = order-1; j > 0; j--)
            state[j] = state[j-1];

        state[0] = *spectrum;
        *spectrum = y;
        spectrum += inc;
    }
}

#endif
