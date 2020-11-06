#ifndef __DMA_E_H
	#define __DMA_E_H
	#include "encoder.h"

/*
	DMA_COMMAND

	local		(struct)		(words)		(group ID)
	0=============
	 | move_ref(3)		|	3*4*4		1
	 | 				|
	 | 				|
	 =============
	 | move_img(3)	|	3*4*4		2
	 | 				|
	 | 				|
	 =============
	 | move_rec(3)	|	3*4*4		3, 4
	 | 				|
	 | 				|
	 =============
	 | move_ACDC(2)	|	2*4*4		5
	 |				|
	 =============


current chain:
	dummy start.
*/

	// reference reconstructed frame y u v,
	// 0, 1, 2
	#define CHN_REF_RE_Y			0
	#define CHN_REF_RE_U			4
	#define CHN_REF_RE_V			8
	// current image y u v,
	// 3, 4, 5
	#define CHN_IMG_Y			12
	#define CHN_IMG_U			16
	#define CHN_IMG_V			20
	// reference reconstructed frame y u v,
	// 6, 7, 8
	#define CHN_CUR_RE_Y		24
	#define CHN_CUR_RE_U		28
	#define CHN_CUR_RE_V		32
	
#ifdef TWO_P_INT
	// 09
	#define CHN_STORE_MBS		36
	// 10
	#define CHN_ACDC_MBS		40	
	// 11
	#define CHN_LOAD_REF_MBS	44	
	// 12
	#define CHN_LOAD_PREDITOR	48
	// 13
	#define CHN_STORE_PREDITOR	52
#else
	// 9
	#define CHN_LOAD_PREDITOR	36
	// 10
	#define CHN_STORE_PREDITOR	40
#endif

	#define ID_CHN_AWYS		0	// always do
	#define ID_CHN_REF_REC		1
	#define ID_CHN_IMG		2
	#define ID_CHN_CUR_REC_Y	3
	#define ID_CHN_CUR_REC_UV	4
	#define ID_CHN_ACDC		5	
	#define ID_CHN_STORE_MBS	6
	#define ID_CHN_ACDC_MBS	7
	#define ID_CHN_LOAD_REF_MBS	8

	#ifdef DMA_E_GLOBALS
		#define DMA_E_EXT
	#else
		#define DMA_E_EXT extern
	#endif

	DMA_E_EXT void dma_enc_commandq_init(Encoder * pEnc);
	DMA_E_EXT void dma_enc_commandq_init_each(Encoder * pEnc);
	DMA_E_EXT void dma_enc_cmd_init_each(Encoder * pEnc);
#endif /* __DMA_E_H  */
