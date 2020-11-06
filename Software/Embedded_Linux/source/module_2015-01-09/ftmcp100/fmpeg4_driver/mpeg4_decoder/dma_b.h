#ifndef __DMA_B_H
	#define __DMA_B_H
#ifdef LINUX
	#include "../common/mp4.h"
	#include "decoder.h"
	#include "../common/dma.h"
#else
	#include "mp4.h"
	#include "decoder.h"
	#include "dma.h"
#endif

/*
	DMA_COMMAND

	local		(struct)		(words)		(group ID)
	0=============
	 | move_4mv(4)	|	4*4*4		1
	 | 				|
	 | 				|
 	 |				|
	 =============
	 | move_1mv(2)	|	2*4*4		2
 	 |				|
	 =============
	 | move_ref_uv(2)	|	2*4*4		6
 	 |				|
	 =============
	 | move_img(3)	|	3*4*4		3
	 | 				|
	 | 				|
	 =============  
	 | move_mbs(1)	|	1*4*4		6
	 =============  
	 | move_rgb(1)	|	1*4*4		4
	 =============
	 | move_ACDC(2)	|	2*4*4		5
	 |				|
	 =============


current chain:
	dummy start.
*/
	// dma_id: 0	always do
	// dma_id: 1
	// 0 1 2 3
	#define CHN_REF_4MV_Y0		0
	#define CHN_REF_4MV_Y1		4
	#define CHN_REF_4MV_Y2		8
	#define CHN_REF_4MV_Y3		12
	// dma_id: 2
	// 4
	#define CHN_REF_1MV_Y		(CHN_REF_4MV_Y3 + 4)
	// dma_id: 6
	// 5 6
	#define CHN_REF_U			(CHN_REF_4MV_Y3 + 8)
	#define CHN_REF_V			(CHN_REF_4MV_Y3 + 12)
	// dma_id: 3
	// 7 8 9
	#define CHN_IMG_Y			(CHN_REF_V + 4)
	#define CHN_IMG_U			(CHN_REF_V + 8)
	#define CHN_IMG_V			(CHN_REF_V + 12)
	#if (defined(TWO_P_EXT) || defined(TWO_P_INT))
	// dma_id: 7
	// 10
	#define CHN_STORE_MBS		(CHN_IMG_V + 4)
	// dma_id: 8
	// 11
	#define CHN_ACDC_MBS		(CHN_STORE_MBS + 4)
	#define CHN_YUV_PREVOUS		(CHN_ACDC_MBS)
	#else
	#define CHN_YUV_PREVOUS		(CHN_IMG_V)
	#endif
	
  #ifdef ENABLE_DEBLOCKING
  
   	
	// YUVmode
		// 10 11 12 ~ 18
		#define CHN_YUV_TOP_Y		(CHN_YUV_PREVOUS + 4)
		#define CHN_YUV_TOP_U		(CHN_YUV_PREVOUS + 8)
		#define CHN_YUV_TOP_V		(CHN_YUV_PREVOUS + 12)
		#define CHN_YUV_MID_Y		(CHN_YUV_PREVOUS + 16)
		#define CHN_YUV_MID_U		(CHN_YUV_PREVOUS + 20)
		#define CHN_YUV_MID_V		(CHN_YUV_PREVOUS + 24)
		#define CHN_YUV_BOT_Y		(CHN_YUV_PREVOUS + 28)
		#define CHN_YUV_BOT_U		(CHN_YUV_PREVOUS + 32)
		#define CHN_YUV_BOT_V		(CHN_YUV_PREVOUS + 36)
		
	// notYUVmode
		// 10 11 12
		#define CHN_RGB_TOP			(CHN_YUV_PREVOUS + 4)
		#define CHN_RGB_MID			(CHN_YUV_PREVOUS + 8)
		#define CHN_RGB_BOT			(CHN_YUV_PREVOUS + 12)
	   	#define CHN_YUV_END        	(CHN_YUV_BOT_V)
		
		// dma_id: 7
		// 19
		#define CHN_STORE_YUV_DEBK		(CHN_YUV_BOT_V + 4)
		// 20
       	#define CHN_LOAD_YUV_DEBK		(CHN_STORE_YUV_DEBK + 4)                
        #define CHN_ACDC_YUV_PREOUS    	(CHN_LOAD_YUV_DEBK)
        		
		#define CHN_RGB_END        		(CHN_RGB_BOT)	
		
		// 13
		#define CHN_STORE_RGB_DEBK		(CHN_RGB_BOT + 4)
		// 14
        #define CHN_LOAD_RGB_DEBK		(CHN_STORE_RGB_DEBK + 4)                
        #define CHN_ACDC_RGB_PREOUS     (CHN_LOAD_RGB_DEBK)
              
  #else // #ifdef ENABLE_DEBLOCKING
	
	
	// YUVmode
		// dma_id: 4
		// 12 13 14
		#define CHN_YUV_Y			(CHN_YUV_PREVOUS + 4)
		#define CHN_YUV_U			(CHN_YUV_PREVOUS + 8)
		#define CHN_YUV_V			(CHN_YUV_PREVOUS + 12)
	// notYUVmode
		// dma_id: 4
		// 12
		#define CHN_RGB				(CHN_YUV_PREVOUS + 4)
		#define CHN_YUV_END        	(CHN_YUV_V)
		#define CHN_ACDC_YUV_PREOUS	(CHN_YUV_V)*/
        #define CHN_RGB_END        	(CHN_RGB)
		#define CHN_ACDC_RGB_PREOUS	(CHN_RGB)*/
	
  #endif // #ifdef ENABLE_DEBLOCKING
  
  
  #ifdef ENABLE_DEBLOCKING
  
    #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
      #define CHN_LOAD_YUV_PREDITOR		(CHN_YUV_END + 4)
      #define CHN_LOAD_RGB_PREDITOR		(CHN_RGB_END + 4)	  
      #define CHN_STORE_YUV_PREDITOR	(CHN_LOAD_YUV_PREDITOR + 4)
      #define CHN_STORE_RGB_PREDITOR	(CHN_LOAD_RGB_PREDITOR + 4)
      #define CHN_STORE_YUV_DEBK		(CHN_STORE_YUV_PREDITOR + 4)
      #define CHN_STORE_RGB_DEBK		(CHN_STORE_RGB_PREDITOR + 4)	  
      #define CHN_LOAD_YUV_DEBK			(CHN_STORE_YUV_DEBK + 4)
      #define CHN_LOAD_RGB_DEBK			(CHN_STORE_RGB_DEBK + 4)	  
      #define CHN_DMA_YUV_DUMMY         (CHN_LOAD_YUV_DEBK + 4)
      #define CHN_DMA_RGB_DUMMY         (CHN_LOAD_RGB_DEBK + 4)	  
      
      #define CHN_NVOP_YUV_BEGIN        (CHN_DMA_YUV_DUMMY)
      #define CHN_NVOP_RGB_BEGIN        (CHN_DMA_RGB_DUMMY)	  
	  
    #else // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
      #define CHN_STORE_YUV_DEBK		(CHN_YUV_END + 4)
      #define CHN_STORE_RGB_DEBK		(CHN_RGB_END + 4)	  
      #define CHN_LOAD_YUV_DEBK	        (CHN_STORE_YUV_DEBK + 4)
      #define CHN_LOAD_RGB_DEBK	        (CHN_STORE_RGB_DEBK + 4)	  
      #define CHN_LOAD_YUV_PREDITOR     (CHN_LOAD_YUV_DEBK + 4)
      #define CHN_LOAD_RGB_PREDITOR     (CHN_LOAD_RGB_DEBK + 4)	  
      #define CHN_STORE_YUV_PREDITOR	(CHN_LOAD_YUV_PREDITOR + 4)
      #define CHN_STORE_RGB_PREDITOR	(CHN_LOAD_RGB_PREDITOR + 4)	  
	           
      #define CHN_NVOP_YUV_BEGIN        (CHN_STORE_YUV_PREDITOR)
      #define CHN_NVOP_RGB_BEGIN        (CHN_STORE_RGB_PREDITOR)	  
    #endif // #ifdef ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
    
  #else // #ifdef ENABLE_DEBLOCKING
	// dma_id: 5
	// 15 or 13
	//#define CHN_LOAD_PREDITOR		(CHN_ACDC_PREOUS + 4)
	#define CHN_LOAD_YUV_PREDITOR	(CHN_YUV_END + 4)
	#define CHN_LOAD_RGB_PREDITOR	(CHN_RGB_END + 4)
	// dma_id: always
	// 16 or 14
	#define CHN_STORE_YUV_PREDITOR	(CHN_LOAD_YUV_PREDITOR + 4)
	#define CHN_STORE_RGB_PREDITOR	(CHN_LOAD_RGB_PREDITOR + 4)
	
	#define CHN_NVOP_YUV_BEGIN	         (CHN_STORE_YUV_PREDITOR)
	#define CHN_NVOP_RGB_BEGIN	         (CHN_STORE_RGB_PREDITOR)
  #endif // #ifdef ENABLE_DEBLOCKING
	
	
	// 17 18 or 15 16
	//#define CHN_NVOP_IN		(CHN_STORE_PREDITOR + 4)
	#define CHN_NVOP_YUV_IN			(CHN_NVOP_YUV_BEGIN + 4)
	#define CHN_NVOP_RGB_IN			(CHN_NVOP_RGB_BEGIN + 4)
	#define CHN_NVOP_YUV_OUT		(CHN_NVOP_YUV_IN + 4)
	#define CHN_NVOP_RGB_OUT		(CHN_NVOP_RGB_IN + 4)

  #ifdef ENABLE_DEBLOCKING
	#define ID_CHN_AWYS		0	// always do
	#define ID_CHN_4MV		1
	#define ID_CHN_1MV		2
	#define ID_CHN_REF_UV	3
	#define ID_CHN_IMG		4
	#define ID_CHN_YUV_TOP	5
	#define ID_CHN_YUV_MID	6
	#define ID_CHN_YUV_BOT	7
	#define ID_CHN_RGB_TOP	5
	#define ID_CHN_RGB_MID	6
	#define ID_CHN_RGB_BOT	7
	#define ID_CHN_ACDC		8	
	#define ID_CHN_ST_DEBK	9
	#define ID_CHN_LD_DEBK	10
	#define ID_CHN_STORE_MBS	11
	#define ID_CHN_ACDC_MBS	12	
  #else // #ifdef ENABLE_DEBLOCKING
	#define ID_CHN_AWYS		0	// always do
	#define ID_CHN_4MV		1
	#define ID_CHN_1MV		2
	#define ID_CHN_REF_UV	6
	#define ID_CHN_IMG		3
	#define ID_CHN_YUV		4
	#define ID_CHN_RGB		4
	#define ID_CHN_ACDC		5
	#define ID_CHN_STORE_MBS	7
	#define ID_CHN_ACDC_MBS	8
  #endif



	#ifdef DMA_B_GLOBALS
		#define DMA_B_EXT
	#else
		#define DMA_B_EXT extern
	#endif

	DMA_B_EXT void dma_dec_commandq_init(DECODER_ext * dec);
	DMA_B_EXT void dma_dec_commandq_init_each(DECODER_ext * dec);
	DMA_B_EXT void dma_mvRef1MV(DECODER * dec,
					const MACROBLOCK *mb, const MACROBLOCK_b *mbb);
	DMA_B_EXT void  dma_mvRef4MV(DECODER * dec,
					const MACROBLOCK *mb, const MACROBLOCK_b *mbb);
	DMA_B_EXT void  dma_mvRefNotCoded(DECODER * dec, const MACROBLOCK_b *mbb);
#endif /* __DMA_B_H  */
