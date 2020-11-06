#ifndef _IMA_ADPCM_ENC_API_
#define _IMA_ADPCM_ENC_API_

#include "audio_main.h"

#include "adpcm.h"

int Init_IMA_ADPCM_Enc(ADPCM_CODEC_PARAMETERS *ima_paras, IMA_state_t *state[][2], int MultiChannel);
//int IMA_ADPCM_Enc(IMA_state_t *state[], short encInputBuf[][2048], unsigned char encOutputBuf[][2048], int encLen[], AUDIO_CODEC_SETTING *codec_setting);
int IMA_ADPCM_Enc(IMA_state_t *state[][2], short encInputBuf[][2048], unsigned char encOutputBuf[][2048], int encLen[],  AUDIO_CODEC_SETTING *codec_setting);
//int IMA_ADPCM_Enc_Destory(IMA_state_t *state[], short multiChannel);
int IMA_ADPCM_Enc_Destory(IMA_state_t *state[][2], short multiChannel);
//void Update_IMA_ADPCM_File_Header(header_t *adpcm_header[], unsigned int outputLen[], short MultiChannel);

#endif
