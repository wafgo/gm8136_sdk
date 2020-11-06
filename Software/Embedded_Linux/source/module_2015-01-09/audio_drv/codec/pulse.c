#include "common.h"
#include "structs.h"
#include "syntax.h"
#include "pulse.h"

#if 0
char pulse_decode(ic_stream *ics, short *spec_data, unsigned short framelen)
{
    char i;
    unsigned short k;
    pulse_info *pul = &(ics->pul);

    k = ics->swb_offset[pul->pulse_start_sfb];

    for(i = 0; i <= pul->number_pulse; i++) {
        k += pul->pulse_offset[i];

        if (k >= framelen)
            return 15; /* should not be possible */

        if (spec_data[k] > 0)
            spec_data[k] += pul->pulse_amp[i];
        else
            spec_data[k] -= pul->pulse_amp[i];
    }

    return 0;
}
#else
//Shawn 2004.12.9
char pulse_decode(ic_stream *ics, short *spec_data, unsigned short framelen)
{
    int i, k;
	char *pulse_offset_p, *pulse_amp_p;
    pulse_info *pul = &(ics->pul);

    k = ics->swb_offset[pul->pulse_start_sfb];

	pulse_offset_p = &pul->pulse_offset[0];
	pulse_amp_p = &pul->pulse_amp[0];

    for(i = pul->number_pulse+1; i != 0; i --) 
	{
		int reg_pulse_amp, reg_spec_data;

        k += *pulse_offset_p++;		

        if (k >= framelen)
            return 15; /* should not be possible */

		reg_pulse_amp = *pulse_amp_p++;

		reg_spec_data = spec_data[k];

        if (reg_spec_data > 0)
            spec_data[k] = reg_spec_data + reg_pulse_amp;
        else
            spec_data[k] = reg_spec_data -  reg_pulse_amp;
    }

    return 0;
}
#endif
