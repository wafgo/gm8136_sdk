#ifndef __IMDCT_H__
#define __IMDCT_H__

mdct_info *faad_mdct_init(unsigned short N);
void faad_mdct_end(mdct_info *mdct);
void faad_imdct(mdct_info *mdct, real_t *X_in, real_t *X_out);
void faad_mdct(mdct_info *mdct, real_t *X_in, real_t *X_out);


#endif
