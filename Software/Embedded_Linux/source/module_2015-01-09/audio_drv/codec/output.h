#ifndef __OUTPUT_H__
#define __OUTPUT_H__


void* output_to_PCM(GMAACDecHandle hDecoder,
                    real_t **input,
                    void *samplebuffer,
                    unsigned char channels,
                    unsigned short frame_len,
                    unsigned char format);

#endif
