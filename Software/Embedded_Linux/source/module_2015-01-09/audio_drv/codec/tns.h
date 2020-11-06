#ifndef __TNS_H__
#define __TNS_H__

#define TNS_MAX_ORDER 20

    
void tns_decode_frame(ic_stream *ics, tns_info *tns, char sr_index, char object_type, real_t *spec, unsigned short frame_len);

#ifdef LTP_DEC
void tns_encode_frame(ic_stream *ics, tns_info *tns, char sr_index, char object_type, real_t *spec, unsigned short frame_len);
#endif

#endif
