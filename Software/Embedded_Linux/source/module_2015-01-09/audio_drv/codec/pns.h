#ifndef __PNS_H__
#define __PNS_H__

#include "common.h"

#include "syntax.h"

#define NOISE_OFFSET 90

void pns_decode(ic_stream *ics_left, ic_stream *ics_right,
                real_t *spec_left, real_t *spec_right, unsigned short frame_len,
                char channel_pair, char object_type);


#if 0
static INLINE char is_noise(ic_stream *ics, char group, char sfb)
{
    if (ics->sfb_cb[group][sfb] == NOISE_HCB)
        return 1;
    return 0;
}
#else
//Shawn 2004.9.24
static __inline int is_noise(ic_stream *ics, char group, char sfb)
{
	char *sfb_cb_p = &ics->sect_start_end_cb_offset[3*8*15*8 + 2*8*15*8];

    if (sfb_cb_p[group*8*15 + sfb] == NOISE_HCB)
        return 1;
    return 0;
}
#endif

#endif
