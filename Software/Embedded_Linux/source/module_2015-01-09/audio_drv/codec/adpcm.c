
/*
** Intel/DVI ADPCM coder/decoder.
**
** The algorithm for this coder was taken from the IMA Compatability Project
** proceedings, Vol 2, Number 2; May 1992.
**
** Version 1.0, 7-Jul-92.
*/
#include <linux/stddef.h>
#include "adpcm.h"

/* Intel ADPCM step variation table */
static int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static int stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

short adpcm_encode(IMA_state_t *state, short pcm){
	int step_index, prev_val, step;
	short nibble;

	int sign; /* sign bit of the nibble (MSB) */
	int delta, predicted_delta;
	//int i;


	prev_val = state->prev_val;
	step_index = state->index;

	sign=0;

	delta = pcm - prev_val;

	if (delta < 0) {
		sign = 1;
		delta = -delta;
    	}

    	step = stepsizeTable[step_index];

    	/* nibble = 4 * delta / step_table[step_index]; */
    	nibble = (delta << 2) / step;

    	if (nibble > 7)
        	nibble = 7;

    	step_index += indexTable[nibble];
    	if (step_index < 0)
        	step_index = 0;
    	if (step_index > 88)
        	step_index = 88;

    	/* what the decoder will find */
    	predicted_delta = ((2 * nibble + 1) * step) >> 3;

    	if (sign)
        	prev_val -= predicted_delta;
    	else
        	prev_val += predicted_delta;

	if (prev_val > 32767)
    		prev_val = 32767;
	else if (prev_val < -32768)
		prev_val = -32768;

	nibble += sign << 3; /* sign * 8 */

	/* save back */
	state->index = step_index;
	state->prev_val = prev_val;

	return nibble;
}

short adpcm_decode(IMA_state_t *state, char nibble){
	int step_index;
	int predicedValue;
	int diff, step;
    int idx = nibble & 0x0f;

	step = stepsizeTable[state->index];
	step_index = state->index + indexTable[idx];

	if(step_index<0)
		step_index = 0;
	else if(step_index>88)
		step_index = 88;

	diff = step>>3;
	if(nibble&4)
		diff += step;
	if(nibble&2)
		diff += step>>1;
	if(nibble&1)
		diff += step>>2;

	predicedValue = state->prev_val;
	if(nibble&8)
		predicedValue -= diff;
	else
		predicedValue += diff;
	if(predicedValue<-0x8000)
		predicedValue = -0x8000;
	else if(predicedValue>0x7fff)
		predicedValue = 0x7fff;

	state->prev_val = predicedValue;
	state->index = step_index;

	return predicedValue;
#if 0
//	short *pcmp;
	//char *outp;

	int predictor;
	int step_index;
	int sign, delta, diff, step;
//	short i;

	step = stepsizeTable[state->index];
	step_index = state->index + indexTable[nibble];
	if (step_index < 0)
		step_index = 0;
	else if (step_index > 88)
		step_index = 88;

	sign = nibble & 8;
	delta = nibble & 7;
	/* perform direct multiplication instead of series of jumps proposed by
	 * the reference ADPCM implementation since modern CPUs can do the mults
	 * quickly enough */
	diff = ((2 * delta + 1) * step) >> 3;
	predictor = state->prev_val;
	if (sign)
		predictor -= diff;
	else
		predictor += diff;

	if (predictor > 32767)
		predictor = 32767;
	else if (predictor < -32768)
		predictor = -32768;

	state->prev_val = predictor;
	state->index = step_index;

	return predictor;
#endif
}

void fact_ParmSetNULL(fact_chunk_t *hfact)
{

  /*
     nothing to do
  */
  hfact=NULL;
}
void data_ParmSetNULL(data_chunk_t *hdata)
{

  /*
     nothing to do
  */
  hdata=NULL;
}
void wav_ParmSetNULL(wav_file_t *hwav)
{

  /*
     nothing to do
  */
  hwav=NULL;
}
void format_ParmSetNULL(format_chunk_t *hformat)
{

  /*
     nothing to do
  */
  hformat=NULL;
}
