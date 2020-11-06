#ifndef __LOCAL_MEM_E_H
	#define __LOCAL_MEM_E_H
#ifdef LINUX
	#include "../common/local_mem.h"
    #include "../common/mp4.h"
#else
	#include "local_mem.h"
	#include "mp4.h"
#endif

/****************  bank 0	*****************************************/
	// ME engine will copy the current image to the local buffer space 
	// 000 ~ 180
	#define CUR_YUV				(LOCAL_BASE + 0x000)		// word boundary


	// different with decoder
	// 400 ~ 800, Leng = [(2 * 64) * 6 (blocks) + 256 (acdc)]	// must 0x400 bundary
	#define QCOEFF_OFF			(LOCAL_BASE + 0x400)
	// 600 ~ 900, Leng = (2 * 64) * 6 (blocks)		// must 0x100 bundary
//	#define QCOEFF_OFF			(LOCAL_BASE + 0x600)
	// 900 ~ D00,		DMA cmd chain,		define in local_mem_d.h, used by decoder
	// D00 ~ E00,		VLC/VLD auto buffer,	define in local_mem.h
	// E00 ~ 1000, 	ACDC predictor buffer,	define in local_mem.h
	// 1000 ~ 1400,	PMV buffer,			define in local_mem.h
	// 1400 ~ 1600,	ME cmd chain,			define in local_mem_d.h, used by decoder

	// 1600 ~ 1800, Leng = 4 * 128 (commands)
	// (till now [2005/10/16], we use 65 commands)
	#define ME_CMD_Q_OFF		(LOCAL_BASE + 0x1600)
	// 1800 ~ 1900, Leng = (4 * 4) * 16 (structures)
	// (till now [2005/10/16], we use 11 commands)
	#define DMA_ENC_CMD_OFF_0		(LOCAL_BASE + 0x1800)
	// 1900 ~ 1A00, Leng = (4 * 4) * 16 (structures)
	// (till now [2005/10/16], we use 11 commands)
	#define DMA_ENC_CMD_OFF_1		(LOCAL_BASE + 0x1900)
	#define DMA_CMD_STRIDE  		((DMA_ENC_CMD_OFF_1 - DMA_ENC_CMD_OFF_0) / 4)
/****************  bank 1	(2K bytes)*****************************************/
	#define CUR_Y0    (LOCAL_BASE + 0x4000)
	#define CUR_U0    (LOCAL_BASE + 0x4100)
	#define CUR_V0    (LOCAL_BASE + 0x4140)
	#define CUR_Y1    (LOCAL_BASE + 0x4180)
	#define CUR_U1    (LOCAL_BASE + 0x4280)
	#define CUR_V1    (LOCAL_BASE + 0x42c0)

	#define INTER_Y0  (LOCAL_BASE + 0x4300)
	#define INTER_U0  (LOCAL_BASE + 0x4400)
	#define INTER_V0  (LOCAL_BASE + 0x4440)
	#define INTER_Y1  (LOCAL_BASE + 0x4480)
	#define INTER_U1  (LOCAL_BASE + 0x4580)
	#define INTER_V1  (LOCAL_BASE + 0x45c0)
	#define INTER_Y2  (LOCAL_BASE + 0x4600)
	#define INTER_U2  (LOCAL_BASE + 0x4700)
	#define INTER_V2  (LOCAL_BASE + 0x4740)
/****************  bank 2/3	*****************************************/
// reference image's local buffer
	#define REF_Y	(LOCAL_BASE + 0x8000)
	#define REF_U	(LOCAL_BASE + 0x8c00)
	#define REF_V	(LOCAL_BASE + 0x8f00)
	#define RADDR ((((uint32_t) REF_Y + 64*16) >> 2) & 0xfff)
	#define RADDR23 ((((uint32_t) REF_Y + 64*(16+8)) >> 2) & 0xfff) //for block 2,3

/****************  bank 4	*****************************************/
	// 7C00 ~ 7D00,	intra quant table,		define in local_mem.h
	// 7D00 ~ 7F00,	inter quant table,		define in local_mem.h


#endif
