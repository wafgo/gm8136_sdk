#include "common.h"
#include "structs.h"
#include "output.h"
#include "decoder.h"

#if 1
void* output_to_PCM(GMAACDecHandle hDecoder,
                    real_t **input, void *sample_buffer, unsigned char channels,
                    unsigned short frame_len, unsigned char format)
{
    unsigned char ch;
    unsigned short i;
    short *short_sample_buffer = (short*)sample_buffer;
    int *int_sample_buffer = (int*)sample_buffer;
	real_t *input_p;

    /* Copy output to a standard PCM buffer */
    for (ch = 0; ch < channels; ch++)
    {
		input_p = (real_t *)input[ch];
		short_sample_buffer = (short*)sample_buffer;
		short_sample_buffer = short_sample_buffer + ch;

        switch (format)
        {
        case FAAD_FMT_16BIT:

            for(i = frame_len; i != 0; i -= 2)
            {
                int tmp0, tmp1;

				tmp0 = (int )*input_p++;
				tmp1 = (int )*input_p++;

                if (tmp0 >= 0)
                {
                    tmp0 += (1 << (REAL_BITS-2-8));					

                    if (tmp0 >= (32768 << (REAL_BITS-1-8)) - 1)
                    {
                        tmp0 = (32768 << (REAL_BITS-1-8)) - 1;
                    }
                } 
				else 
				{
                    tmp0 += -(1 << (REAL_BITS-2-8));

                    if (tmp0 <= -(32768 << (REAL_BITS-1-8)))
                    {
                        tmp0 = -(32768 << (REAL_BITS-1-8));
                    }
                }

				if (tmp1 >= 0)
                {
                    tmp1 += (1 << (REAL_BITS-2-8));					

                    if (tmp1 >= (32768 << (REAL_BITS-1-8)) - 1)
                    {
                        tmp1 = (32768 << (REAL_BITS-1-8)) - 1;
                    }
                } 
				else 
				{
                    tmp1 += -(1 << (REAL_BITS-2-8));

                    if (tmp1 <= -(32768 << (REAL_BITS-1-8)))
                    {
                        tmp1 = -(32768 << (REAL_BITS-1-8));
                    }
                }
                
				tmp0 >>= (REAL_BITS-1-8);
				tmp1 >>= (REAL_BITS-1-8);

                *short_sample_buffer = (short)tmp0;short_sample_buffer+=channels;
				*short_sample_buffer = (short)tmp1;short_sample_buffer+=channels;
            }
            break;
        case FAAD_FMT_24BIT:
            for(i = 0; i < frame_len; i ++)
            {
                int tmp = input[ch][i];
                //int tmp = get_sample(input, ch, i, hDecoder->downMatrix, hDecoder->internal_channel);

					tmp <<= 3;

                    if (tmp >= ((32768 << 8)-1))
                    {
                        tmp = ((32768 << 8)-1);
                    }

                    if (tmp <= -(32768 << 8))
                    {
                        tmp = -(32768 << 8);
                    }

                int_sample_buffer[(i*channels)+ch] = (int)tmp;
            }
            break;
        }
    }

    return sample_buffer;
}
#else
//Shawn 2004.8.24
void* output_to_PCM(GMAACDecHandle hDecoder,
                    real_t **input, void *sample_buffer, char channels,
                    unsigned short frame_len, char format)
{
    char ch;
    unsigned short i;
    short *short_sample_buffer = (short*)sample_buffer;
	real_t *input_p;

    /* Copy output to a standard PCM buffer */
    for (ch = 0; ch < channels; ch++)
    {
		input_p = (real_t *)input[ch];
		short_sample_buffer = (short*)sample_buffer;
		short_sample_buffer = short_sample_buffer + ch;



            for(i = frame_len; i != 0; i -= 2)
            {
                int tmp0, tmp1;

				tmp0 = (int )*input_p++;
				tmp1 = (int )*input_p++;

                if (tmp0 >= 0)
                {
                    tmp0 += (1 << (REAL_BITS-2-8));					

                    if (tmp0 >= (32768 << (REAL_BITS-1-8)) - 1)
                    {
                        tmp0 = (32768 << (REAL_BITS-1-8)) - 1;
                    }
                } 
				else 
				{
                    tmp0 += -(1 << (REAL_BITS-2-8));

                    if (tmp0 <= -(32768 << (REAL_BITS-1-8)))
                    {
                        tmp0 = -(32768 << (REAL_BITS-1-8));
                    }
                }

				if (tmp1 >= 0)
                {
                    tmp1 += (1 << (REAL_BITS-2-8));					

                    if (tmp1 >= (32768 << (REAL_BITS-1-8)) - 1)
                    {
                        tmp1 = (32768 << (REAL_BITS-1-8)) - 1;
                    }
                } 
				else 
				{
                    tmp1 += -(1 << (REAL_BITS-2-8));

                    if (tmp1 <= -(32768 << (REAL_BITS-1-8)))
                    {
                        tmp1 = -(32768 << (REAL_BITS-1-8));
                    }
                }
                
				tmp0 >>= (REAL_BITS-1-8);
				tmp1 >>= (REAL_BITS-1-8);

                *short_sample_buffer = (short)tmp0;short_sample_buffer+=channels;
				*short_sample_buffer = (short)tmp1;short_sample_buffer+=channels;
            }
            
    }

    return sample_buffer;
}
#endif

// not used in Faraday MPEG-2/4 AAC Decoder
#if 0
#define DM_MUL FRAC_CONST(0.3203772410170407) // 1/(1+sqrt(2) + 1/sqrt(2))
#define RSQRT2 FRAC_CONST(0.7071067811865475244) // 1/sqrt(2)

static INLINE real_t get_sample(real_t **input, char channel, unsigned short sample,
                                char down_matrix, char *internal_channel)
{
    if (!down_matrix)
        return input[internal_channel[channel]][sample];

    if (channel == 0)
    {
        real_t C   = MUL_F(input[internal_channel[0]][sample], RSQRT2);
        real_t L_S = MUL_F(input[internal_channel[3]][sample], RSQRT2);
        real_t cum = input[internal_channel[1]][sample] + C + L_S;
        return MUL_F(cum, DM_MUL);
    } else {
        real_t C   = MUL_F(input[internal_channel[0]][sample], RSQRT2);
        real_t R_S = MUL_F(input[internal_channel[4]][sample], RSQRT2);
        real_t cum = input[internal_channel[2]][sample] + C + R_S;
        return MUL_F(cum, DM_MUL);
    }
}
#endif