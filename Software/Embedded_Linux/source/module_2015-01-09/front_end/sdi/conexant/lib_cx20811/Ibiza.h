/*+++ *******************************************************************\
*
*  Copyright and Disclaimer:
*
*     ---------------------------------------------------------------
*     This software is provided "AS IS" without warranty of any kind,
*     either expressed or implied, including but not limited to the
*     implied warranties of noninfringement, merchantability and/or
*     fitness for a particular purpose.
*     ---------------------------------------------------------------
*
*     Copyright (c) 2013 Conexant Systems, Inc.
*     All rights reserved.
*
\******************************************************************* ---*/
#ifndef _IBIZA_H_
#define _IBIZA_H_






#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */



typedef enum _cx20811_audio_samplerate
{
    CX20811_SAMPLE_RATE_8000,
    CX20811_SAMPLE_RATE_16000,
    CX20811_SAMPLE_RATE_32000,
    CX20811_SAMPLE_RATE_44100,
    CX20811_SAMPLE_RATE_48000,
    CX20811_SAMPLE_RATE_BUTT
} cx20811_audio_samplerate;



typedef enum _cx20811_audio_samplebits
{
    CX20811_SAMPLE_BIT_8,
    CX20811_SAMPLE_BIT_16,
    CX20811_SAMPLE_BIT_24
} cx20811_audio_samplebits;

typedef enum _cx20811_audio_channel_index
{
	CX20811_LEFT_CHANNEL,
	CX20811_RIGHT_CHANNEL
}cx20811_audio_channel_index;


extern int cx20811_chip_init(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // _IBIZA_H_
