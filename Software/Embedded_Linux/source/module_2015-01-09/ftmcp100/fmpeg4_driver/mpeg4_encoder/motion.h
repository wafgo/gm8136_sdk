#ifndef _MOTION_H_
#define _MOTION_H_

#ifdef LINUX
#include "../common/portab.h"
#include "ftmcp100.h"
#include "global_e.h"
#include "encoder.h"
#else
#include "portab.h"
#include "ftmcp100.h"
#include "global_e.h"
#include "encoder.h"
#endif

/* hard coded motion search parameters for motion_est */
/* INTER bias for INTER/INTRA decision; mpeg4 spec suggests 2*nb */
#define MV16_INTER_BIAS	512

/* vector map (vlc delta size) smoother parameters */
#define NEIGH_TEND_16X16	2


#define Diamond_search_limit 				10

#define L1 									2				//	Motion activity threshold
#define MAX3(x,y,z)      	  				MAX(MAX(x,y),z)
#define ThEES				  				512
#define ThEES_4				  				512/4
#define ThEES1				  				0


/* default methods of Search, will be changed to function variable */

#define INTRA_MB_THD                        1/3
#define SUCCESSIVE_INTRA_OVER               1
#define SUCCESSIVE_DISABLE_INTRA            1


/*
 * Calculate the min/max range (in halfpixels)
 * relative to the _MACROBLOCK_ position
 */
/*
 * getref: calculate reference image pointer 
 * the decision to use interpolation h/v/hv or the normal image is
 * based on dx & dy.
 */

/* This is somehow a copy of get_ref, but with MV instead of X,Y */



void MotionEstimation_4MV(MACROBLOCK_E *const pMB,
				 MACROBLOCK_E *const pMB_mc,
		  		 uint32_t x, uint32_t y,
		 		 Encoder *pEnc,
		 		 int32_t counter,
		 		 MACROBLOCK_pass * ptMBpass);

void MotionEstimation_blocklast_4MV(MACROBLOCK_E *const pMB_mc,
				 Encoder *pEnc,
				 MACROBLOCK_pass * ptMBpass);


void MotionEstimation_block0_4MV(MACROBLOCK_E *const pMB,
				 Encoder *pEnc,
				 MACROBLOCK_pass * ptMBpass);

void MotionEstimation_1MV(MACROBLOCK_E *const pMB,
				 MACROBLOCK_E *const pMB_mc,
		  		 uint32_t x, uint32_t y,
		 		 Encoder *pEnc,
		 		 int32_t counter,
		 		 MACROBLOCK_pass * ptMBpass);


void MotionEstimation_blocklast_1MV(MACROBLOCK_E *const pMB_mc,
				 Encoder *pEnc,
				 MACROBLOCK_pass * ptMBpass);


void MotionEstimation_block0_1MV(MACROBLOCK_E *const pMB,
				 Encoder *pEnc,
				 MACROBLOCK_pass * ptMBpass);

extern __inline void
get_pmvdata0_4MV(const MACROBLOCK_E * const mbs,
         const int mb_width,
         const int x,
         const int counter,
		 VECTOR * const pmv,
		 Encoder * const pEnc);

//extern __inline void
//get_pmvdata1_4MV(const int x, int iWcount, int *a, int *b, int *c, int *d, Encoder * pEnc);


extern __inline void
get_pmvdata0_1MV(const MACROBLOCK_E * const mbs,
	const int mb_width,
	const int x,
	const int counter,
	VECTOR * const pmv,
	Encoder * const pEnc);


extern __inline void
get_pmvdata1_1MV(const int x, int iWcount, int *a, int *b, int *Z);


#endif							/* _MOTION_H_ */
