#ifndef __LOCAL_MEM_H
	#define __LOCAL_MEM_H
	#include "../common/mp4.h"
	#ifdef GM8120
		#define LOCAL_BASE			0x00000
	#else
		#define LOCAL_BASE			0x10000
	#endif
	#define MP4_OFF			(LOCAL_BASE + 0x10000)
	#define DMA_OFF			(MP4_OFF + 0x400)
	#define SHAREMEM_OFF	(0x8000)

/****************  bank 0	*****************************************/
	// D00 ~ E00, Length = 256
	#define RUN_LEVEL_OFF		(LOCAL_BASE + 0xD00)
	// E00 ~ 1000, Leng = (2 * 16) * 16(mbs)		// must 0x100 bundary
	#define PREDICTOR0_OFF		(LOCAL_BASE + 0xE00)
	#define PREDICTOR4_OFF		(LOCAL_BASE + 0xE80)
	#define PREDICTOR8_OFF	 	(LOCAL_BASE + 0xF00)
	// 1000 ~ 1400, Leng = (2W) * 128(mbs/row)			// must 0x400 bundary
	// 128(mbs/row) => max width 128mbs/row x 16 pixel/mb = 2048 pixel
	#define PMV_BUFFER_OFF		(LOCAL_BASE + 0x1000)

/****************  bank 4	*****************************************/
	// 7C00~7D00, 4 * 64
	#define INTRA_QUANT_TABLE_ADDR	(LOCAL_BASE + 0x7C00)
	// 7D00~7F00, 4 * 64
	#define INTER_QUANT_TABLE_ADDR	(LOCAL_BASE + 0x7D00)

#endif
