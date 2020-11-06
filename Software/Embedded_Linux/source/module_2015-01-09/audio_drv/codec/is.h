#ifndef __IS_H__
#define __IS_H__

#include "syntax.h"

void is_decode(ic_stream *ics, ic_stream *icsr, real_t *l_spec, real_t *r_spec, unsigned short frame_len);

#if 0
static INLINE signed char is_intensity(ic_stream *ics, char group, char sfb)
{
    switch (ics->sfb_cb[group][sfb])
    {
    case INTENSITY_HCB:
        return 1;
    case INTENSITY_HCB2:
        return -1;
    default:
        return 0;
    }
}
#else
//Shawn 2004.9.24
static __inline int is_intensity(ic_stream *ics, char group, char sfb)
{
	char *sfb_cb_p = &ics->sect_start_end_cb_offset[3*8*15*8 + 2*8*15*8];

    switch (sfb_cb_p[group*8*15+sfb])
    {
		case INTENSITY_HCB:
			return 1;
		case INTENSITY_HCB2:
			return -1;
		default:
			return 0;
    }
}
#endif

#if 0
static INLINE signed char invert_intensity(ic_stream *ics, char group, char sfb)
{
    if (ics->ms_mask_present == 1)
        return (1-2*ics->ms_used[group][sfb]);
    return 1;
}
#else
//Shawn 2004.9.24
static __inline int invert_intensity(ic_stream *ics, char group, char sfb)
{
    if (ics->ms_mask_present == 1)
        return (1 - (ics->ms_used[group*MAX_SFB+sfb] << 1));
    return 1;
}
#endif
#endif
