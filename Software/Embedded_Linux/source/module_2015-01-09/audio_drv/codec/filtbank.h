#ifndef __FILTBANK_H__
#define __FILTBANK_H__

fb_info *filter_bank_init(unsigned short frame_len);
void filter_bank_end(fb_info *fb);

#ifdef LTP_DEC
void filter_bank_ltp(fb_info *fb,
                     char window_sequence,
                     char window_shape,
                     char window_shape_prev,
                     real_t *in_data,
                     real_t *out_mdct,
                     char object_type,
                     unsigned short frame_len);
#endif

void ifilter_bank(fb_info *fb, char window_sequence, char window_shape,
                  char window_shape_prev, real_t *freq_in,
                  real_t *time_out, real_t *overlap,
                  char object_type, unsigned short frame_len);

#endif
