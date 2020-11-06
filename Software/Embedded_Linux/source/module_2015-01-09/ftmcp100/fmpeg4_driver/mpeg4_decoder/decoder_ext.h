#ifndef _DECODER_EXT_H_
	#define _DECODER_EXT_H_

	#include "global_d.h"
	#ifdef LINUX
		#include "../common/share_mem.h"
	#else
		#include "share_mem.h"
	#endif
	#include "decoder.h"

    #define HAS_INTERRUPT 0x12345678
    #define NO_INTERRUPT  0x87654321

    #define ENABLE_INTERRUPT    0x12345678
    #define DISABLE_INTERRUPT   0x87654321

	typedef struct {
		volatile MP4_COMMAND command;
		DECODER_int dec;
		Bitstream bs;
		int rounding;
		volatile int int_enabled;   //Yes:0x12345678, NO:0x87654321
		volatile int int_flag;      //Yes:0x12345678, NO:0x87654321
	} Share_Node_Dec;

	extern int decoder_iframe_tri(DECODER_ext * dec, Bitstream * bs);
	extern int decoder_pframe_tri(DECODER_ext * dec, Bitstream * bs, int rounding);
	extern int decoder_nframe_tri(DECODER_ext * dec);
	extern int decoder_frame_sync(DECODER_ext * dec, Bitstream * bs);
	extern int decoder_frame_block(DECODER_ext * dec);
#endif
