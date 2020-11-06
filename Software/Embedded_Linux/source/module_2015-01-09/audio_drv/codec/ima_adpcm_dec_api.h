#ifndef _IMA_ADPCM_DEC_API_
#define _IMA_ADPCM_DEC_API_
#include "adpcm.h"

//void IMA_ADPCM_Dec_Destory(header_t *adpcm_header);
//int Init_IMA_ADPCM_Dec(header_t **adpcm_header_dec, unsigned char **file_ptr);
int Init_IMA_ADPCM_Dec(ADPCM_CODEC_PARAMETERS *ima_paras);
void IMA_ADPCM_dec(ADPCM_CODEC_PARAMETERS *ima_paras, unsigned char decInputBuf[], short decOutputBuf[]);
//int Count_Block_Num(header_t *adpcm_header_dec);
#endif
