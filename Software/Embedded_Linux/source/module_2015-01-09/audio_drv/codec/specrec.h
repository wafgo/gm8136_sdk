#ifndef __SPECREC_H__
#define __SPECREC_H__

#include "syntax.h"

char window_grouping_info(GMAACDecHandle hDecoder, ic_stream *ics);
void apply_scalefactors(GMAACDecHandle hDecoder, ic_stream *ics, real_t *x_invquant, unsigned short frame_len);
#if 0
char reconstruct_channel_pair(GMAACDecHandle hDecoder, ic_stream *ics1, ic_stream *ics2,
                                 element *cpe, short *spec_data1, short *spec_data2);
#else
//Shawn 2004.10.12
int reconstruct_channel_pair(GMAACDecHandle hDecoder, ic_stream *ics1, ic_stream *ics2,
                                 element *cpe, short *spec_data1, short *spec_data2);
#endif
char reconstruct_single_channel(GMAACDecHandle hDecoder, ic_stream *ics, element *sce, short *spec_data);


#endif
