#ifndef __DRC_H__
#define __DRC_H__


#define DRC_REF_LEVEL 20*4 /* -20 dB */


drc_info *drc_init(real_t cut, real_t boost);
void drc_end(drc_info *drc);
void drc_decode(drc_info *drc, real_t *spec);


#endif
