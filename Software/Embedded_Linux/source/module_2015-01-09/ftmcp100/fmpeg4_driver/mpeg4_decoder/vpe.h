#ifndef __VPE_H
	#define __VPE_H
	#include "Mp4Vdec.h"

	#define VPE 		0x90180000

	#ifdef VPE_GLOBALS
		#define VPE_EXT
	#else
		#define VPE_EXT extern
	#endif

	VPE_EXT void vVPE_OpenDecoderFile(void);
	VPE_EXT void vVPE_CloseDecoderFile(void);
	VPE_EXT void vVPE_OpenEncoderFile(void);
	VPE_EXT void vVPE_CloseEncoderFile(void);
	VPE_EXT void vVPE_SaveYuvSeq(void *pvDec);
	VPE_EXT void vVPE_SaveRgb(FMP4_DEC_RESULT * ptDecResult);
	VPE_EXT void vVPE_SaveBS(uint8_t * buf, int32_t len);

#endif
