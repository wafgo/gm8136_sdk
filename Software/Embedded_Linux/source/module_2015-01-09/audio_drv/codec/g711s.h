#ifndef _G711_S_
#define	_G711_S_
#include "audio_main.h"
#include "filter.h"
typedef  struct   {
    char  *input_buf  ;
    //unsigned int in_file_size;
    char  *output_buf  ;
    //unsigned int out_file_size;
    unsigned short block_size;
    unsigned short option;
   } INPARAM ;

#define LINEAR2ALAW     1
#define ALAW2LINEAR     2
#define LINEAR2ULAW     3
#define ULAW2LINEAR     4
#define ALAW2ULAW       5
#define ULAW2ALAW       6

int G711_linear2alaw( int pcmSample ) ;
int G711_alaw2linear( int aLawSample );

int G711_linear2ulaw( int pcmSample ) ;
int G711_ulaw2linear( int uLawSample ) ;

int G711_alaw2ulaw( int aLawSample ) ;
int G711_ulaw2alaw( int uLawSample ) ;
 
int G711_linear2linear( int pcmSample ) ;

//void perform_conversion( char inputs[ ],  unsigned short block_size, char *output_buf, int ( *ConvRoutine )( int ) );
int   G711_Enc(INPARAM *inparam[], short encInputBuf[][2048], unsigned char encOutputBuf[][2048], int encLen[], short Mulichannel_no );
int   G711_Dec(INPARAM *inparam, unsigned char *decInputBuf, short *decOutputBuf);
void perform_conversion( char inputs[ ], unsigned short numberInSamples, unsigned int inBytes, unsigned int outBytes, char outputs[], int ( *ConvRoutine )( int ) );
void g711_convertion(INPARAM *inparam, unsigned int option );
int   g711_init_enc(AUDIO_CODEC_SETTING *codec_setting, INPARAM *inparam[]);
int   g711_init_dec(AUDIO_CODEC_SETTING *codec_setting, INPARAM **inparam);
void g711enc_destory(INPARAM *inparam[], short MultiChannel);
void g711dec_destory(INPARAM *inparam);

#endif	 /* _G711_S_ */
