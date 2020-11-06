#ifndef _ENCODER_EXT_H_
	#define _ENCODER_EXT_H_

#ifdef LINUX
	#include "global_e.h"
	#include "../common/share_mem.h"
	#include "encoder.h"
#else
	#include "global_e.h"
	#include "share_mem.h"
	#include "encoder.h"
#endif

	#define HAS_INTERRUPT 0x12345678
	#define NO_INTERRUPT  0x87654321

	#define ENABLE_INTERRUPT    0x12345678
	#define DISABLE_INTERRUPT   0x87654321
    
	typedef struct {
		volatile MP4_COMMAND command;
		Encoder_x enc;
		FRAMEINFO current1;
		FRAMEINFO reference;
		volatile int int_enabled;   //Yes:0x12345678, NO:0x87654321
		volatile int int_flag;  //Yes:0x12345678, NO:0x87654321
	} Share_Node_Enc;

	extern int encoder_iframe_tri(Encoder_x * pEnc_x);
	extern int encoder_pframe_tri(Encoder_x * pEnc_x);
	extern int encoder_nframe_tri(Encoder_x * pEnc_x);
	extern int encoder_frame_sync(Encoder_x * pEnc_x);
	extern int encoder_frame_block(Encoder_x * pEnc_x);

#endif
