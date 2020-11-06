#ifndef _AUDIO_MAIN_
#define _AUDIO_MAIN_

#include <linux/export.h>
#define MaxMultiChannel 32//#include "gmaac_enc_api.h"

#define AAC_ENCODE		0
#define IMA_ADPCM_ENCODE	1
#define AULAW_ENCODE	2

#define AAC_DECODE		0
#define IMA_ADPCM_DECODE	1
#define AULAW_DECODE	2

#define AAC_FRAME_SZ    	1024
#define IMA_BLK_SZ_MONO		505
#define IMA_BLK_SZ_DUAL		498
#define AULAW_BLK_SZ     320

typedef struct {
  short   nChannels;		/*! number of channels on input (1,2) */
  int	  sampleRate;          /*! audio file sample rate */
  int	  bitRate;             /*! encoder bit rate in bits/sec */
} AAC_ENCODE_PARAMETERS;

typedef struct {
  short   nChannels;
  short   nBlockAlign;
  short   Sample_Num;		
} ADPCM_CODEC_PARAMETERS;

typedef struct {
  unsigned short 	aulaw_enc_option;
  unsigned short 	aulaw_dec_option;
  unsigned short 	block_size;
  short	  Decoded_Sample_Num;
} G711_CODEC_PARAMETERS;

typedef struct {
  short   encode_error;
  short   decode_error;
  short   encode_audio_type;	//0: aac 1: adpcm
  short   decode_audio_type;	//0: aac 1: adpcm
  short   decode_mode;          //0: auto, 1: aac, 2:adpcm 3:aulaw
  short   EncMultiChannel;

  AAC_ENCODE_PARAMETERS   aac_enc_paras[MaxMultiChannel];
  ADPCM_CODEC_PARAMETERS ima_paras;
  G711_CODEC_PARAMETERS      aulaw_codec_pars;
//  ADPCM_ENCODE_PARAMETERS ima_enc_paras;
} AUDIO_CODEC_SETTING;

#endif
