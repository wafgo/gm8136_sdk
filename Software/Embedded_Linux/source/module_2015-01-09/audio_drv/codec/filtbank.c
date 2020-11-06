/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: filtbank.c,v 1.2 2013/10/29 05:36:56 easonli Exp $
**/
#include <linux/slab.h>
#include "common.h"
#include "structs.h"
#include "filtbank.h"
#include "decoder.h"
#include "syntax.h"
#include "kbd_win.h"
#include "sine_win.h"
#include "imdct.h"
#define assert(expr) \
	if (!(expr)) { \
	printk( "Assertion failed! %s,%s,%s,line=%d\n",\
	#expr,__FILE__,__func__,__LINE__); \
	BUG(); \
	}


fb_info *filter_bank_init(unsigned short frame_len)
{
    unsigned short nshort = frame_len/8;

    fb_info *fb = (fb_info*)kmalloc(sizeof(fb_info), GFP_KERNEL);//(fb_info*)faad_malloc(sizeof(fb_info));
    memset(fb, 0, sizeof(fb_info));

    /* normal */
    fb->mdct256 = faad_mdct_init(2*nshort);
    fb->mdct2048 = faad_mdct_init(2*frame_len);

        fb->long_window[0]  = sine_long_1024;
        fb->short_window[0] = sine_short_128;
        fb->long_window[1]  = kbd_long_1024;
        fb->short_window[1] = kbd_short_128;

    return fb;
}

#if 0
void filter_bank_end(fb_info *fb)
{
    if (fb != NULL)
    {
#ifdef PROFILE
        printf("FB:                 %I64d cycles\n", fb->cycles);
#endif

        faad_mdct_end(fb->mdct256);
        faad_mdct_end(fb->mdct2048);


        faad_free(fb);
    }
}
#else
//Shawn 2004.12.9
void filter_bank_end(fb_info *fb)
{
    if (fb != NULL)
    {
#ifdef PROFILE
        printf("FB:                 %I64d cycles\n", fb->cycles);
#endif

        faad_mdct_end(fb->mdct256);
        faad_mdct_end(fb->mdct2048);


        kfree(fb);//faad_free(fb);
    }
}
#endif

/*static INLINE void imdct_long(fb_info *fb, real_t *in_data, real_t *out_data, unsigned short len)
{
    faad_imdct(fb->mdct2048, in_data, out_data);
}*/

#ifdef LTP_DEC
static INLINE void mdct(fb_info *fb, real_t *in_data, real_t *out_data, unsigned short len)
{
    mdct_info *mdct = NULL;

    switch (len)
    {
    case 2048:
    case 1920:
        mdct = fb->mdct2048;
        break;
    case 256:
    case 240:
        mdct = fb->mdct256;
        break;
    }

    faad_mdct(mdct, in_data, out_data);
}
#endif

#if 0
void ifilter_bank(fb_info *fb, char window_sequence, char window_shape,
                  char window_shape_prev, real_t *freq_in,
                  real_t *time_out, real_t *overlap,
                  char object_type, unsigned short frame_len)
{
    short i;
    real_t transf_buf[2*1024] = {0};

    const real_t *window_long = NULL;
    const real_t *window_long_prev = NULL;
    const real_t *window_short = NULL;
    const real_t *window_short_prev = NULL;

    unsigned short nlong = frame_len;
    unsigned short nshort = frame_len/8;
    unsigned short trans = nshort/2;

    unsigned short nflat_ls = (nlong-nshort)/2;

#ifdef PROFILE
    int64_t count = faad_get_ts();
#endif

        window_long       = fb->long_window[window_shape];
        window_long_prev  = fb->long_window[window_shape_prev];
        window_short      = fb->short_window[window_shape];
        window_short_prev = fb->short_window[window_shape_prev];



    switch (window_sequence)
    {
    case ONLY_LONG_SEQUENCE:
        imdct_long(fb, freq_in, transf_buf, 2*nlong);
        for (i = 0; i < nlong; i+=4)
        {
            time_out[i]   = overlap[i]   + MUL_F(transf_buf[i],window_long_prev[i]);
            time_out[i+1] = overlap[i+1] + MUL_F(transf_buf[i+1],window_long_prev[i+1]);
            time_out[i+2] = overlap[i+2] + MUL_F(transf_buf[i+2],window_long_prev[i+2]);
            time_out[i+3] = overlap[i+3] + MUL_F(transf_buf[i+3],window_long_prev[i+3]);
        }
        for (i = 0; i < nlong; i+=4)
        {
            overlap[i]   = MUL_F(transf_buf[nlong+i],window_long[nlong-1-i]);
            overlap[i+1] = MUL_F(transf_buf[nlong+i+1],window_long[nlong-2-i]);
            overlap[i+2] = MUL_F(transf_buf[nlong+i+2],window_long[nlong-3-i]);
            overlap[i+3] = MUL_F(transf_buf[nlong+i+3],window_long[nlong-4-i]);
        }
        break;

    case LONG_START_SEQUENCE:
        imdct_long(fb, freq_in, transf_buf, 2*nlong);
        for (i = 0; i < nlong; i+=4)
        {
            time_out[i]   = overlap[i]   + MUL_F(transf_buf[i],window_long_prev[i]);
            time_out[i+1] = overlap[i+1] + MUL_F(transf_buf[i+1],window_long_prev[i+1]);
            time_out[i+2] = overlap[i+2] + MUL_F(transf_buf[i+2],window_long_prev[i+2]);
            time_out[i+3] = overlap[i+3] + MUL_F(transf_buf[i+3],window_long_prev[i+3]);
        }
        for (i = 0; i < nflat_ls; i++)
            overlap[i] = transf_buf[nlong+i];
        for (i = 0; i < nshort; i++)
            overlap[nflat_ls+i] = MUL_F(transf_buf[nlong+nflat_ls+i],window_short[nshort-i-1]);
        for (i = 0; i < nflat_ls; i++)
            overlap[nflat_ls+nshort+i] = 0;
        break;

    case EIGHT_SHORT_SEQUENCE:
        faad_imdct(fb->mdct256, freq_in+0*nshort, transf_buf+2*nshort*0);
        faad_imdct(fb->mdct256, freq_in+1*nshort, transf_buf+2*nshort*1);
        faad_imdct(fb->mdct256, freq_in+2*nshort, transf_buf+2*nshort*2);
        faad_imdct(fb->mdct256, freq_in+3*nshort, transf_buf+2*nshort*3);
        faad_imdct(fb->mdct256, freq_in+4*nshort, transf_buf+2*nshort*4);
        faad_imdct(fb->mdct256, freq_in+5*nshort, transf_buf+2*nshort*5);
        faad_imdct(fb->mdct256, freq_in+6*nshort, transf_buf+2*nshort*6);
        faad_imdct(fb->mdct256, freq_in+7*nshort, transf_buf+2*nshort*7);
        for (i = 0; i < nflat_ls; i++)
            time_out[i] = overlap[i];
        for(i = 0; i < nshort; i++)
        {
            time_out[nflat_ls+         i] = overlap[nflat_ls+         i] + MUL_F(transf_buf[nshort*0+i],window_short_prev[i]);
            time_out[nflat_ls+1*nshort+i] = overlap[nflat_ls+nshort*1+i] + MUL_F(transf_buf[nshort*1+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*2+i],window_short[i]);
            time_out[nflat_ls+2*nshort+i] = overlap[nflat_ls+nshort*2+i] + MUL_F(transf_buf[nshort*3+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*4+i],window_short[i]);
            time_out[nflat_ls+3*nshort+i] = overlap[nflat_ls+nshort*3+i] + MUL_F(transf_buf[nshort*5+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*6+i],window_short[i]);
            if (i < trans)
                time_out[nflat_ls+4*nshort+i] = overlap[nflat_ls+nshort*4+i] + MUL_F(transf_buf[nshort*7+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*8+i],window_short[i]);
        }
        for(i = 0; i < nshort; i++)
        {
            if (i >= trans)
                overlap[nflat_ls+4*nshort+i-nlong] = MUL_F(transf_buf[nshort*7+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*8+i],window_short[i]);
            overlap[nflat_ls+5*nshort+i-nlong] = MUL_F(transf_buf[nshort*9+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*10+i],window_short[i]);
            overlap[nflat_ls+6*nshort+i-nlong] = MUL_F(transf_buf[nshort*11+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*12+i],window_short[i]);
            overlap[nflat_ls+7*nshort+i-nlong] = MUL_F(transf_buf[nshort*13+i],window_short[nshort-1-i]) + MUL_F(transf_buf[nshort*14+i],window_short[i]);
            overlap[nflat_ls+8*nshort+i-nlong] = MUL_F(transf_buf[nshort*15+i],window_short[nshort-1-i]);
        }
        for (i = 0; i < nflat_ls; i++)
            overlap[nflat_ls+nshort+i] = 0;
        break;

    case LONG_STOP_SEQUENCE:
        imdct_long(fb, freq_in, transf_buf, 2*nlong);
        for (i = 0; i < nflat_ls; i++)
            time_out[i] = overlap[i];
        for (i = 0; i < nshort; i++)
            time_out[nflat_ls+i] = overlap[nflat_ls+i] + MUL_F(transf_buf[nflat_ls+i],window_short_prev[i]);
        for (i = 0; i < nflat_ls; i++)
            time_out[nflat_ls+nshort+i] = overlap[nflat_ls+nshort+i] + transf_buf[nflat_ls+nshort+i];
        for (i = 0; i < nlong; i++)
            overlap[i] = MUL_F(transf_buf[nlong+i],window_long[nlong-1-i]);
		break;
    }

#ifdef PROFILE
    count = faad_get_ts() - count;
    fb->cycles += count;
#endif
}
#else
//Shawn 2004.8.23
#ifndef ARM_ASM
extern volatile char sect_start_end_cb_offset_buf[2*(3*8*15*8 + 2*8*15*8)+2688];

#ifdef ARM_ASM
static INLINE real_t _MulHigh(real_t A, real_t B)
{
	int lo, hi;

    __asm
	{
			SMULL	lo, hi, A, B
	}
	return hi;
}
#endif

void ifilter_bank(fb_info *fb, char window_sequence, char window_shape,
                  char window_shape_prev, real_t *freq_in,
                  real_t *time_out, real_t *overlap,
                  char object_type, unsigned short frame_len)
{
    int i;


    const real_t *window_long;
    const real_t *window_long_prev;
    const real_t *window_short;
    const real_t *window_short_prev;

	real_t tr0, tr1, tr2, tr3;
	real_t wlp0, wlp1, wlp2, wlp3;
	real_t r0, r1, r2, r3;
	real_t ol0, ol1, ol2, ol3;
	real_t to0, to1, to2, to3;
	real_t lo0, lo1, lo2, lo3, hi0, hi1, hi2, hi3;

	real_t reg_tr, reg_win, reg_r;
	real_t lo, hi, r;

	real_t *tb_p;
	real_t *wl_p;
	real_t *ol_p;
	real_t *fi_p;
	real_t *ws_p;
	const real_t *win_short_p0, *win_short_p1;

	real_t *transf_buf;//[2*1024];

	transf_buf = (real_t *)&sect_start_end_cb_offset_buf[0];

        window_long       = fb->long_window[window_shape];
        window_long_prev  = fb->long_window[window_shape_prev];
        window_short      = fb->short_window[window_shape];
        window_short_prev = fb->short_window[window_shape_prev];



    switch (window_sequence)
    {
    case ONLY_LONG_SEQUENCE:

		faad_imdct(fb->mdct2048, freq_in, transf_buf);

		tb_p = (real_t *)&transf_buf[0];
		ol_p = (real_t *)&overlap[0];

#ifdef _WIN32
		for (i = 1024; i != 0; i -= 4)
        {
			tr0 = *tb_p++;
			wlp0 = *window_long_prev++;
			tr1 = *tb_p++;
			wlp1 = *window_long_prev++;
			ol0 = *ol_p++;
			ol1 = *ol_p++;

			r0 = _MulHigh(tr0,wlp0);
			r1 = _MulHigh(tr1,wlp1);

			to0 = ol0 + r0;
			to1 = ol1 + r1;

			tr2 = *tb_p++;
			tr3 = *tb_p++;
			wlp2 = *window_long_prev++;
			wlp3 = *window_long_prev++;


			r2 = _MulHigh(tr2,wlp2);
			r3 = _MulHigh(tr3,wlp3);

			ol2 = *ol_p++;
			ol3 = *ol_p++;
			to2 = ol2 + r2;
			to3 = ol3 + r3;

            *time_out++ = to0;
            *time_out++ = to1;
			*time_out++ = to2;
			*time_out++ = to3;
		}

		wl_p = (real_t *)&window_long[1023];
		ol_p = ol_p - 1024;

		for (i = 1024; i != 0; i -= 4)
        {
			tr0 = *tb_p++;
			wlp0 = *wl_p--;
			tr1 = *tb_p++;
			wlp1 = *wl_p--;
            ol0 = _MulHigh(tr0,wlp0);
            ol1 = _MulHigh(tr1,wlp1);

			tr2 = *tb_p++;
			wlp2 = *wl_p--;
			tr3 = *tb_p++;
			wlp3 = *wl_p--;
            ol2 = _MulHigh(tr2,wlp2);
            ol3 = _MulHigh(tr3,wlp3);

			*ol_p++ = ol0;
			*ol_p++ = ol1;
			*ol_p++ = ol2;
			*ol_p++ = ol3;
        }
#else //ARM
//Shawn 2004.8.23
		__asm
		{
			MOV		i, #1024

LOOP_ONLY_LONG_A:

			LDR		tr0, [tb_p], #4
			LDR		wlp0, [window_long_prev], #4
			LDR		ol0, [ol_p], #4
			SMULL	lo0, hi0, wlp0, tr0
			ADD		to0, ol0, hi0
			STR		to0, [time_out], #4

			LDR		tr1, [tb_p], #4
			LDR		wlp1, [window_long_prev], #4
			LDR		ol1, [ol_p], #4
			SMULL	lo1, hi1, wlp1, tr1
			ADD		to1, ol1, hi1
			STR		to1, [time_out], #4

			LDR		tr2, [tb_p], #4
			LDR		wlp2, [window_long_prev], #4
			LDR		ol2, [ol_p], #4
			SMULL	lo2, hi2, wlp2, tr2
			ADD		to2, ol2, hi2
			STR		to2, [time_out], #4

			LDR		tr3, [tb_p], #4
			LDR		wlp3, [window_long_prev], #4
			LDR		ol3, [ol_p], #4
			SMULL	lo3, hi3, wlp3, tr3
			ADD		to3, ol3, hi3
			STR		to3, [time_out], #4

			SUBS	i, i, #4
			BNE		LOOP_ONLY_LONG_A

			MOV		wl_p, &window_long[1023]
			SUB		ol_p, ol_p, #1024*4

			MOV		i, #1024

LOOP_ONLY_LONG_B:

			LDR		tr0, [tb_p], #4
			LDR		tr1, [tb_p], #4
			LDR		tr2, [tb_p], #4
			LDR		tr3, [tb_p], #4
			LDR		wlp0, [wl_p], #-4
			LDR		wlp1, [wl_p], #-4
			LDR		wlp2, [wl_p], #-4
			LDR		wlp3, [wl_p], #-4

			SMULL	lo0, hi0, wlp0, tr0
			SMULL	lo1, hi1, wlp1, tr1
			SMULL	lo2, hi2, wlp2, tr2
			SMULL	lo3, hi3, wlp3, tr3

			STR		hi0, [ol_p], #4
			STR		hi1, [ol_p], #4
			STR		hi2, [ol_p], #4
			STR		hi3, [ol_p], #4

			SUBS	i, i, #4
			BNE		LOOP_ONLY_LONG_B
		}
#endif
        break;

    case LONG_START_SEQUENCE:

		faad_imdct(fb->mdct2048, freq_in, transf_buf);

		tb_p = &transf_buf[0];

#ifdef _WIN32
		for (i = 1024; i != 0; i --)
        {
			real_t reg_r0, reg_r1, reg_r2, reg_r3;
			reg_tr = *tb_p++;
			reg_win = *window_long_prev++;
			reg_r = _MulHigh(reg_tr, reg_win);
			*time_out++ = *overlap++ + reg_r;
		}

		overlap -= 1024;

		for (i = 448; i != 0; i --)
			*overlap++ = *tb_p++ >> 9;

		ws_p = (real_t *)&window_short[127];

		for (i = 128; i != 0; i --)
		{
			reg_tr = *tb_p++;
			reg_win = *ws_p--;
			ol0 = _MulHigh(reg_tr, reg_win);
			*overlap++ = ol0;
		}
#else
//Shawn 2004.10.13
        {
			real_t reg_r0, reg_r1, reg_r2, reg_r3;
			real_t lo, reg_ol, reg_to;
			real_t reg_tb;

			__asm
			{
				MOV		i, #1024

LOOP_LONG_START_A:

				LDR		reg_tr, [tb_p], #4
				LDR		reg_win, [window_long_prev], #4
				LDR		reg_ol, [overlap], #4
				SMULL	lo, reg_r, reg_win, reg_tr
				ADD		reg_to, reg_ol, reg_r
				STR		reg_to, [time_out], #4

				SUBS	i, i, #1
				BNE		LOOP_LONG_START_A

				SUB		overlap, overlap, #1024*4

				MOV		i, #448

LOOP_LONG_START_B:

				LDR		reg_tb, [tb_p], #4
				MOV		reg_ol, reg_tb >> 9
				STR		reg_ol, [overlap], #4

				SUBS	i, i, #1
				BNE		LOOP_LONG_START_B

				MOV		ws_p, &window_short[127]

				MOV		i, #128

LOOP_LONG_START_C:

				LDR		reg_tr, [tb_p], #4
				LDR		reg_win, [ws_p], #-4
				SMULL	lo, ol0, reg_win, reg_tr
				STR		ol0, [overlap], #4

				SUBS	i, i, #1
				BNE		LOOP_LONG_START_C
			}
		}
#endif

		//memset(overlap, 0, 448 << 2);

		for (i = 448; i != 0; i --)
			*overlap++ = 0;

        break;

    case EIGHT_SHORT_SEQUENCE:

		fi_p = (real_t *)&freq_in[0];
		tb_p = (real_t *)&transf_buf[0];

		for (i = 8; i != 0; i --)
		{
			faad_imdct(fb->mdct256, fi_p, tb_p);
			fi_p = fi_p + 128;
			tb_p = tb_p + 256;
		}

		//memcpy(time_out, overlap, 448 << 2);
		//time_out += 448;
		//overlap += 448;

		/*for (i = 448; i != 0; i --)
			*time_out++ = *overlap++;*/

		for (i = 448; i != 0; i -= 2)
		{
			int reg_o0, reg_o1;

			reg_o0 = *overlap++;
			reg_o1 = *overlap++;
			*time_out++ = reg_o0;
			*time_out++ = reg_o1;
		}

		ol_p = (real_t *)&overlap[0];
		tb_p = (real_t *)&transf_buf[0];

        for(i = 0; i < 128; i++)
        {
			real_t reg_winshort0, reg_winshort1;
			real_t *reg_winshort0_p, *reg_winshort1_p;
			real_t reg_tmp0, reg_tmp1, reg_tmp2, tr4, tr5, tr6, reg_wl, reg_tmp3;

#ifdef _WIN32
			reg_winshort1 = window_short[127-i];
			reg_winshort0 = window_short[i];

			if (i < 64)
			{
					reg_tmp0 = _MulHigh(tb_p[896], reg_winshort1);
					reg_tmp1 = _MulHigh(tb_p[1024], reg_winshort0);
					ol0 = ol_p[512];
					to0 = ol0 + reg_tmp0 + reg_tmp1;
					time_out[512] = to0;
			}

			time_out[384] = ol_p[384] + _MulHigh(tb_p[640], reg_winshort1) + _MulHigh(tb_p[768], reg_winshort0);
			time_out[256] = ol_p[256] + _MulHigh(tb_p[384], reg_winshort1) + _MulHigh(tb_p[512], reg_winshort0);
			time_out[128] = ol_p[128] + _MulHigh(tb_p[128], reg_winshort1) + _MulHigh(tb_p[256], reg_winshort0);
			*time_out++ = *ol_p++ + _MulHigh(*tb_p++, *window_short_prev++);
#else
			__asm
			{
				MOV		reg_winshort0_p, &window_short[i]
				MOV		reg_winshort1_p, &window_short[127-i]
				LDR		reg_winshort0, [reg_winshort0_p]
				LDR		reg_winshort1, [reg_winshort1_p]

				CMP		i, #64
				BGE		BRANCH_SHORT_A

				LDR		tr0, [tb_p, #896*4]
				LDR		tr1, [tb_p, #1024*4]
				LDR		ol0, [ol_p, #512*4]
				SMLAL	lo, ol0, reg_winshort1, tr0
				SMLAL	lo, ol0, reg_winshort0, tr1
				STR		ol0, [time_out, #512*4]
BRANCH_SHORT_A :

				LDR		tr0, [tb_p, #640*4]
				LDR		tr1, [tb_p, #768*4]
				LDR		ol0, [ol_p, #384*4]
				SMLAL	lo, ol0, reg_winshort1, tr0
				SMLAL	lo, ol0, reg_winshort0, tr1
				STR		ol0, [time_out, #384*4]

				LDR		tr2, [tb_p, #384*4]
				LDR		tr3, [tb_p, #512*4]
				LDR		ol1, [ol_p, #256*4]
				SMLAL	lo, ol1, reg_winshort1, tr2
				SMLAL	lo, ol1, reg_winshort0, tr3
				STR		ol1, [time_out, #256*4]

				LDR		tr4, [tb_p, #128*4]
				LDR		tr5, [tb_p, #256*4]
				LDR		ol2, [ol_p, #128*4]
				SMLAL	lo, ol2, reg_winshort1, tr4
				SMLAL	lo, ol2, reg_winshort0, tr5
				STR		ol2, [time_out, #128*4]

				LDR		tr6, [tb_p], #4
				LDR		reg_wl, [window_short_prev], #4
				LDR		ol3, [ol_p], #4
				SMLAL	lo, ol3, reg_wl, tr6
				STR		ol3, [time_out], #4
			}
#endif

        }

		ol_p = &overlap[0];
		tb_p = &transf_buf[0];
		win_short_p0 = &window_short[0];
		win_short_p1 = &window_short[127];


        for(i = 128; i != 0; i --)
        {
			real_t reg_winshort0, reg_winshort1;
			real_t reg_tmp0, reg_tmp1, reg_tmp2, tr4, tr5, tr6, reg_wl, reg_tmp3;

			reg_winshort1 = *win_short_p1--;
			reg_winshort0 = *win_short_p0++;

#ifdef _WIN32

            if (i <= 64)
                ol_p[512-1024] = _MulHigh(tb_p[896], reg_winshort1) + _MulHigh(tb_p[1024], reg_winshort0);
            ol_p[640-1024] = _MulHigh(tb_p[1152], reg_winshort1) + _MulHigh(tb_p[1280], reg_winshort0);
            ol_p[768-1024] = _MulHigh(tb_p[1408], reg_winshort1) + _MulHigh(tb_p[1536], reg_winshort0);
            ol_p[896-1024] = _MulHigh(tb_p[1664], reg_winshort1) + _MulHigh(tb_p[1792], reg_winshort0);
            ol_p[1024-1024] = _MulHigh(tb_p[1920], reg_winshort1);ol_p++;tb_p++;
#else
			__asm
			{
				CMP		i, #64
				BGT		BRANCH_SHORT_B

				LDR		tr0, [tb_p, #896*4]
				LDR		tr1, [tb_p, #1024*4]
				SMULL	lo, reg_tmp0, reg_winshort1, tr0
				SMLAL	lo, reg_tmp0, reg_winshort0, tr1
				STR		reg_tmp0, [ol_p, #(512-1024)*4]
BRANCH_SHORT_B :

				LDR		tr0, [tb_p, #1152*4]
				LDR		tr1, [tb_p, #1280*4]
				SMULL	lo, reg_tmp0, reg_winshort1, tr0
				SMLAL	lo, reg_tmp0, reg_winshort0, tr1
				STR		reg_tmp0, [ol_p, #(640-1024)*4]

				LDR		tr2, [tb_p, #1408*4]
				LDR		tr3, [tb_p, #1536*4]
				SMULL	lo, reg_tmp1, reg_winshort1, tr2
				SMLAL	lo, reg_tmp1, reg_winshort0, tr3
				STR		reg_tmp1, [ol_p, #(768-1024)*4]

				LDR		tr4, [tb_p, #1664*4]
				LDR		tr5, [tb_p, #1792*4]
				SMULL	lo, reg_tmp2, reg_winshort1, tr4
				SMLAL	lo, reg_tmp2, reg_winshort0, tr5
				STR		reg_tmp2, [ol_p, #(896-1024)*4]

				LDR		tr6, [tb_p, #1920*4]
				SMULL	lo, reg_tmp3, reg_winshort1, tr6
				STR		reg_tmp3, [ol_p, #(1024-1024)*4]
			}
			ol_p++;tb_p++;
#endif
		}

		//memset(&overlap[128], 0, 448 << 2);

		ol_p = &overlap[128];

		for (i = 448; i != 0; i --)
			*ol_p++ = 0;

        break;

    case LONG_STOP_SEQUENCE:

		faad_imdct(fb->mdct2048, freq_in, transf_buf);

		//memcpy(time_out, overlap, 448 << 2);
		//time_out += 448;
		//overlap += 448;

		/*for (i = 448; i != 0; i --)
			*time_out++ = *overlap++;*/

		for (i = 448; i != 0; i -= 2)
		{
			int reg_o0, reg_o1;

			reg_o0 = *overlap++;
			reg_o1 = *overlap++;
			*time_out++ = reg_o0;
			*time_out++ = reg_o1;
		}

		transf_buf += 448;

#ifdef _WIN32
		for (i = 128; i != 0; i --)
		{
			reg_tr = *transf_buf++;
			reg_win = *window_short_prev++;
			reg_r = _MulHigh(reg_tr, reg_win);
			*time_out++ = *overlap++ + reg_r;
		}

		for (i = 448; i != 0; i --)
		{
            *time_out++ = (*overlap++ + (*transf_buf++>>9));
		}

		wl_p = (real_t *)&window_long[1023];
		tb_p = (real_t *)&transf_buf[1024 - 448 - 448 - 128];
		ol_p = (real_t *)&overlap[- 448 - 448 - 128];

		for (i = 1024; i != 0; i --)
		{
            *ol_p++ = _MulHigh(*tb_p++, *wl_p--);
		}
#else
//Shawn 2004.10.13
		{
			real_t lo, reg_ol, reg_to;
			real_t reg0, reg1, reg_r;

			__asm
			{
				MOV		i, #128

LOOP_LONG_STOP_A:

				LDR		reg_tr, [transf_buf], #4
				LDR		reg_win, [window_short_prev], #4
				LDR		reg_ol, [overlap], #4
				SMULL	lo, reg_r, reg_win, reg_tr
				ADD		reg_to, reg_ol, reg_r
				STR		reg_to, [time_out], #4

				SUBS	i, i, #1
				BNE		LOOP_LONG_STOP_A

				MOV		i, #448

LOOP_LONG_STOP_B:

				LDR		reg_tr, [transf_buf], #4
				LDR		reg_ol, [overlap], #4
				ADD		reg_to, reg_ol, reg_tr >> 9
				STR		reg_to, [time_out], #4

				SUBS	i, i, #1
				BNE		LOOP_LONG_STOP_B

				MOV		wl_p, &window_long[1023]
				MOV		tb_p, &transf_buf[1024 - 448 - 448 - 128]
				MOV		ol_p, &overlap[- 448 - 448 - 128]

				MOV		i, #1024

LOOP_LONG_STOP_C:

				LDR		reg0, [tb_p], #4
				LDR		reg1, [wl_p], #-4
				SMULL	lo, reg_r, reg0, reg1
				STR		reg_r, [ol_p], #4

				SUBS	i, i, #1
				BNE		LOOP_LONG_STOP_C
			}
		}
#endif
		break;
    }
}
#endif
#endif

#ifdef LTP_DEC
/* only works for LTP -> no overlapping, no short blocks */
void filter_bank_ltp(fb_info *fb, char window_sequence, char window_shape,
                     char window_shape_prev, real_t *in_data, real_t *out_mdct,
                     char object_type, unsigned short frame_len)
{
    short i;
    real_t windowed_buf[2*1024] = {0};

    const real_t *window_long = NULL;
    const real_t *window_long_prev = NULL;
    const real_t *window_short = NULL;
    const real_t *window_short_prev = NULL;

    unsigned short nlong = frame_len;
    unsigned short nshort = frame_len/8;
    unsigned short nflat_ls = (nlong-nshort)/2;

    assert(window_sequence != EIGHT_SHORT_SEQUENCE);

        window_long       = fb->long_window[window_shape];
        window_long_prev  = fb->long_window[window_shape_prev];
        window_short      = fb->short_window[window_shape];
        window_short_prev = fb->short_window[window_shape_prev];


    switch(window_sequence)
    {
    case ONLY_LONG_SEQUENCE:
        for (i = nlong-1; i >= 0; i--)
        {
            windowed_buf[i] = MUL_F(in_data[i], window_long_prev[i]);
            windowed_buf[i+nlong] = MUL_F(in_data[i+nlong], window_long[nlong-1-i]);
        }
        mdct(fb, windowed_buf, out_mdct, 2*nlong);
        break;

    case LONG_START_SEQUENCE:
        for (i = 0; i < nlong; i++)
            windowed_buf[i] = MUL_F(in_data[i], window_long_prev[i]);
        for (i = 0; i < nflat_ls; i++)
            windowed_buf[i+nlong] = in_data[i+nlong];
        for (i = 0; i < nshort; i++)
            windowed_buf[i+nlong+nflat_ls] = MUL_F(in_data[i+nlong+nflat_ls], window_short[nshort-1-i]);
        for (i = 0; i < nflat_ls; i++)
            windowed_buf[i+nlong+nflat_ls+nshort] = 0;
        mdct(fb, windowed_buf, out_mdct, 2*nlong);
        break;

    case LONG_STOP_SEQUENCE:
        for (i = 0; i < nflat_ls; i++)
            windowed_buf[i] = 0;
        for (i = 0; i < nshort; i++)
            windowed_buf[i+nflat_ls] = MUL_F(in_data[i+nflat_ls], window_short_prev[i]);
        for (i = 0; i < nflat_ls; i++)
            windowed_buf[i+nflat_ls+nshort] = in_data[i+nflat_ls+nshort];
        for (i = 0; i < nlong; i++)
            windowed_buf[i+nlong] = MUL_F(in_data[i+nlong], window_long[nlong-1-i]);
        mdct(fb, windowed_buf, out_mdct, 2*nlong);
        break;
    }
}
#endif
