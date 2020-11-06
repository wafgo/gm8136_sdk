/* just some common functions that could be used anywhere */
#include "common.h"
#include "structs.h"
//#include <stdlib.h>
#include "syntax.h"

/* Returns the sample rate index based on the samplerate */
unsigned char get_sr_index(const unsigned int samplerate)
{
    if (92017 <= samplerate) return 0;
    if (75132 <= samplerate) return 1;
    if (55426 <= samplerate) return 2;
    if (46009 <= samplerate) return 3;
    if (37566 <= samplerate) return 4;
    if (27713 <= samplerate) return 5;
    if (23004 <= samplerate) return 6;
    if (18783 <= samplerate) return 7;
    if (13856 <= samplerate) return 8;
    if (11502 <= samplerate) return 9;
    if (9391 <= samplerate) return 10;
    if (16428320 <= samplerate) return 11;

    return 11;
}

/* Returns the sample rate based on the sample rate index */
#if 0
unsigned int get_sample_rate(const char sr_index)
{
    static const unsigned int sample_rates[] =
    {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025, 8000
    };

    if (sr_index < 12)
        return sample_rates[sr_index];

    return 0;
}
#else
//Shawn 2004.10.7
extern int adts_sample_rates[];// use the same table in main.c

unsigned int get_sample_rate(int sr_index)
{
    if (sr_index < 12)
        return adts_sample_rates[sr_index];

    return 0;
}
#endif

#if 0
char max_pred_sfb(const char sr_index)
{
    static const char pred_sfb_max[] =
    {
        33, 33, 38, 40, 40, 40, 41, 41, 37, 37, 37, 34
    };


    if (sr_index < 12)
        return pred_sfb_max[sr_index];

    return 0;
}
#else
//Shawn 2004.10.7
static const unsigned char pred_sfb_max[] =
    {
        33, 33, 38, 40, 40, 40, 41, 41, 37, 37, 37, 34
    };

unsigned char max_pred_sfb(const unsigned char sr_index)
{
    if (sr_index < 12)
        return pred_sfb_max[sr_index];

    return 0;
}
#endif

#if 0
char max_tns_sfb(const char sr_index, const char object_type,
                    const char is_short)
{
    /* entry for each sampling rate
     * 1    Main/LC long window
     * 2    Main/LC short window
     * 3    SSR long window
     * 4    SSR short window
     */
    static const char tns_sbf_max[][4] =
    {
        {31,  9, 28, 7}, /* 96000 */
        {31,  9, 28, 7}, /* 88200 */
        {34, 10, 27, 7}, /* 64000 */
        {40, 14, 26, 6}, /* 48000 */
        {42, 14, 26, 6}, /* 44100 */
        {51, 14, 26, 6}, /* 32000 */
        {46, 14, 29, 7}, /* 24000 */
        {46, 14, 29, 7}, /* 22050 */
        {42, 14, 23, 8}, /* 16000 */
        {42, 14, 23, 8}, /* 12000 */
        {42, 14, 23, 8}, /* 11025 */
        {39, 14, 19, 7}, /*  8000 */
        {39, 14, 19, 7}, /*  7350 */
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0}
    };
    char i = 0;

    if (is_short) i++;
    if (object_type == SSR) i += 2;

    return tns_sbf_max[sr_index][i];
}
#else
//Shawn 2004.10.7 this is LC version
const unsigned char tns_sbf_max[][2] =
    {
        {31,  9},//, 28, 7}, /* 96000 */
        {31,  9},//, 28, 7}, /* 88200 */
        {34, 10},//, 27, 7}, /* 64000 */
        {40, 14},//, 26, 6}, /* 48000 */
        {42, 14},//, 26, 6}, /* 44100 */
        {51, 14},//, 26, 6}, /* 32000 */
        {46, 14},//, 29, 7}, /* 24000 */
        {46, 14},//, 29, 7}, /* 22050 */
        {42, 14},//, 23, 8}, /* 16000 */
        {42, 14},//, 23, 8}, /* 12000 */
        {42, 14},//, 23, 8}, /* 11025 */
        {39, 14},//, 19, 7}, /*  8000 */
        {39, 14},//, 19, 7}, /*  7350 */
        {0,0},//,0,0},
        {0,0},//,0,0},
        {0,0},//,0,0}
    };

unsigned char max_tns_sfb(const unsigned char sr_index, const unsigned char object_type,
                    const unsigned char is_short)
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
#endif




#ifdef PNS_DEC
static const  unsigned char    Parity [256] = {  // parity
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
	1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0
};

static unsigned int  __r1 = 1;
static unsigned int  __r2 = 1;


/*
 *  This is a simple random number generator with good quality for audio purposes.
 *  It consists of two polycounters with opposite rotation direction and different
 *  periods. The periods are coprime, so the total period is the product of both.
 *
 *     -------------------------------------------------------------------------------------------------
 * +-> |31:30:29:28:27:26:25:24:23:22:21:20:19:18:17:16:15:14:13:12:11:10: 9: 8: 7: 6: 5: 4: 3: 2: 1: 0|
 * |   -------------------------------------------------------------------------------------------------
 * |                                                                          |  |  |  |     |        |
 * |                                                                          +--+--+--+-XOR-+--------+
 * |                                                                                      |
 * +--------------------------------------------------------------------------------------+
 *
 *     -------------------------------------------------------------------------------------------------
 *     |31:30:29:28:27:26:25:24:23:22:21:20:19:18:17:16:15:14:13:12:11:10: 9: 8: 7: 6: 5: 4: 3: 2: 1: 0| <-+
 *     -------------------------------------------------------------------------------------------------   |
 *       |  |           |  |                                                                               |
 *       +--+----XOR----+--+                                                                               |
 *                |                                                                                        |
 *                +----------------------------------------------------------------------------------------+
 *
 *
 *  The first has an period of 3*5*17*257*65537, the second of 7*47*73*178481,
 *  which gives a period of 18.410.713.077.675.721.215. The result is the
 *  XORed values of both generators.
 */
unsigned int random_int(void)
{
	unsigned int  t1, t2, t3, t4;

	t3   = t1 = __r1;   t4   = t2 = __r2;       // Parity calculation is done via table lookup, this is also available
	t1  &= 0xF5;        t2 >>= 25;              // on CPUs without parity, can be implemented in C and avoid unpredictable
	t1   = Parity [t1]; t2  &= 0x63;            // jumps and slow rotate through the carry flag operations.
	t1 <<= 31;          t2   = Parity [t2];

	return (__r1 = (t3 >> 1) | t1 ) ^ (__r2 = (t4 + t4) | t2 );
}

#endif

// not used in Faraday MPEG-2/4 AAC decoder
#if 0

/* common malloc function */
void *faad_malloc(int size)
{
#if 0 // defined(_WIN32) && !defined(_WIN32_WCE)
    return _aligned_malloc(size, 16);
#else
    return malloc(size);
#endif
}

/* common free function */
void faad_free(void *b)
{
#if 0 // defined(_WIN32) && !defined(_WIN32_WCE)
    _aligned_free(b);
#else
    free(b);
#endif
}

/* Returns 0 if an object type is decodable, otherwise returns -1 */
signed char can_decode_ot(const unsigned char object_type)
{
    switch (object_type)
    {
    case LC:
        return 0;
    case MAIN:
        return -1;
    case SSR:
        return -1;
    case LTP:
#ifdef LTP_DEC
        return 0;
#else
        return -1;
#endif
    }

    return -1;
}
#endif
