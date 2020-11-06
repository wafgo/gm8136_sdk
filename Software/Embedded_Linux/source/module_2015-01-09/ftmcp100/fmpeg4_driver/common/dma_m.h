#ifndef __DMA_M_H
	#define __DMA_M_H
	#include "mp4.h"

#ifdef LINUX //kernel driver mode
	// Drain Write Buffer
	#define mFa526DrainWrBuf()
	// Clean & Invalidate all D-cache 
	#define mFa526CleanInvalidateDCacheAll()
#else
	#ifndef TWO_P_INT
	// Drain Write Buffer
	#define mFa526DrainWrBuf()		\
		__asm {					\
			MCR p15, 0, 0, c7, c10, 4	\
		}
	// Clean & Invalidate all D-cache 
	#define mFa526CleanInvalidateDCacheAll()	\
		__asm {							\
			MCR p15, 0, 0, c7, c14, 0			\
		}
	#else	
	// Drain Write Buffer
	#define mFa526DrainWrBuf()
	// Clean & Invalidate all D-cache 
	#define mFa526CleanInvalidateDCacheAll()	
	#endif
#endif

	// Macros
	#define mDma_FinishCheck()		((MDMA1->Status & BIT0) != 0)

	// System memory start address
	#define mDmaSysInc3b(v)		((uint32_t)(v))	// memory Inc. Index
											// 0: No change
											// 1: +16 bytes
											// 2: +32 bytes
											// 3: +48 bytes
											// 4: +64 bytes
											// 5: +128 bytes
											// 6: +256 bytes
											// 7: +512 bytes
	#define mDmaSysMemAddr29b(v)	((uint32_t)(v))	// system memory start addr. (8bytes)
	// Local memory start address
	#define mDmaLocInc2b(v)		((uint32_t)(v))	// memory Inc. Index
											// 0: No change
											// 1: +16 bytes
											// 2: +32 bytes
											// 3: +64 bytes
	#ifdef GM8120
		#define mDmaLocMemAddr14b(v)	(((uint32_t)(v)>>2<<2) & 0xFFFC)	// Local memory start addr. (4bytes)
	#else		
		#define mDmaLocMemAddr14b(v)	(((uint32_t)(v)>>3<<2) & 0xFFFC)	// Local memory start addr. (8bytes)
	#endif
	#define mDmaLoc2dWidth4b(v)	(((uint32_t)(v)) << 16)	// Local memory 2D width (words)
	#define mDmaLoc2dOff8b(v)		((((uint32_t)(v) & 0xFF)) << 20)	// Local memory 2D offset (words)
	#define mDmaLoc3dWidth4b(v)	(((uint32_t)(v)) << 28)			// Local memory 3D width (words)

	// dma width
	#define mDmaSysWidth6b(v)		((uint32_t)(v))				// System block width (1unit: 8 bytes)
	#define mDmaSysOff14b(v)		(((uint32_t)(v) & 0x3FFF) << 6)	// System memory data line offset
	#define mDmaLocWidth4b(v)		(((uint32_t)(v)) << 20)			// Local block width (words)
	#define mDmaLocOff8b(v)		(((uint32_t)(v) & 0xFF) << 24)	// Local memory data line offset
	// dma control
	#define mDmaLen12b(v)			((uint32_t)(v))				// Transfer Length (words)
	#define mDmaID4b(v)			(((uint32_t)(v)) << 12)		// ID
	#define mDmaSType2b(v)			(((uint32_t)(v)) << 16)		// System Memory data type
														// 00: Sequential data
														// 01: 2D
														// 1X: Reserved
	#define mDmaLType2b(value)		(((uint32_t)value) << 18)		// Local Memory data type
														// 00: Sequential data
														// 01: 2D
														// 1X: Reserved
	#define mDmaDir1b(value)		(((uint32_t)value) << 20)		// Transfer Direction
														// 0: Sys -> Local
														// 1: Local -> Sys
	#define mDmaChainEn1b(value)	(((uint32_t)value) << 21)		// Enable Chain transfer
	#define mDmaEn1b(value)		(((uint32_t)value) << 23)		// Start transfer
	#define mDmaIntDoneEn1b(value)	(((uint32_t)value) << 24)		// Transfer done interrupt enable
	#define mDmaIntErrEn1b(value)	(((uint32_t)value) << 25)		// Bus Error interrupt enable
	#define mDmaIntChainMask1b(value)	(((uint32_t)value) << 26)		// Transfer done interrupt mask for
															// chain trasnfer operation
	#define mDmaReset1b(value)		(((uint32_t)value) << 27)		// reset & auto clear
	#define mDmaLoc3dOff8b(value)	(((uint32_t)value) << 28)		// Local memory 3D offset (words)
	// dma chain
	#define mDmaChnCmdSour(value)	(value)
	#define DMA_LC_SYS			0
	#define DMA_LC_LOC			2

	#define DMA_DATA_SEQUENTAIL	0
	#define DMA_DATA_2D			1
	#define DMA_DATA_3D			2
	#define DMA_DATA_4D			3

	#define DMA_DIR_2LOCAL		0
	#define DMA_DIR_2SYS			1

	#ifdef GM8120
		#define DMA_INCL_0			0
		#define DMA_INCL_16			1
		#define DMA_INCL_32			2
		#define DMA_INCL_64			3
	#else
		#define DMA_INCL_0			0
		#define DMA_INCL_32			1
		#define DMA_INCL_64			2
		#define DMA_INCL_128			3
	#endif

	#define DMA_INCS_0			0
	#define DMA_INCS_16			1
	#define DMA_INCS_32			2
	#define DMA_INCS_48			3
	#define DMA_INCS_64			4
	#define DMA_INCS_128			5
	#define DMA_INCS_256			6
	#define DMA_INCS_512			7

	#define DMA_EXEC		0
	#define DMA_SKIP_INC	1
	#define DMA_SKIP		2
	#define DMA_EXEC_SYNC	3

	#define DMA_SYNC_VLD	0
	#define DMA_SYNC_ME	1
	#define DMA_SYNC_MC	2

	#define mDmaSMaddr(chn)		((chn) << 2 + 0)
	#define mDmaLMaddr(chn)		((chn) << 2 + 1)
	#define mDmaWidth(chn)			((chn) << 2 + 2)
	#define mDmaControl(chn)		((chn) << 2 + 3)
	#if 0
	// mpeg4 coprocessor register operation
	#define mMP4RegWr(Cxreg, value)			\
			__asm {						\
				MCR		p4, 0, value, Cxreg	\
			}

	#define mMP4RegRd(Cxreg, variable)		\
			__asm {						\
				MRC		p4, 0, variable, Cxreg\
			}
	#endif
#endif
