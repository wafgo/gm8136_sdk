#ifdef LTP_DEC

#ifndef __LT_PREDICT_H__
#define __LT_PREDICT_H__


#include "filtbank.h"

char is_ltp_ot(char object_type);

void lt_prediction(ic_stream *ics,
                   ltp_info *ltp,
                   real_t *spec,
                   short *lt_pred_stat,
                   fb_info *fb,
                   char win_shape,
                   char win_shape_prev,
                   char sr_index,
                   char object_type,
                   unsigned short frame_len);

void lt_update_state(short *lt_pred_stat,
                     real_t *time,
                     real_t *overlap,
                     unsigned short frame_len,
                     char object_type);

#endif

#endif
