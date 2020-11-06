#ifndef __ME_M_H
	#define __ME_M_H
#ifdef LINUX
	#include "../common/define.h"
#else
    #include "define.h"
#endif

	/////////////////////////////////////////////////
	// ME command queue bit field
	#define mMECMD_TYPE3b(v)	((uint32_t)(v) << 29)
	// command (3 bits)
	#define MEC_PMVX			0	// Set horizontal prediction vector command
	#define MEC_PMVY			1	// Set vertical prediction vector command
	#define MEC_SAD			2	// Performs SAD calculation based on a given
								// motion vector
	#define MEC_DIA			3	// Performs diamond search repeatedly to find
								// the best motion vector
	#define MEC_REF			4	// Performs half-pixel refine based on previous
								// found best motion vector by DIA
	#define MEC_MOD			5	// Intra/inter mode decision command
	#define MEC_PXI			6	// Performs pixel interpolation of the reference
								// block & output the interpolated result block
								// to local memory

	// MV / MVD  buffer
	#define mMV_X8b(v)			((uint32_t)(v) << 20)
	#define mMV_Y8b(v)			((uint32_t)(v) << 12)
	#define mMV_RADD12b(v)		((uint32_t)(v) & 0xfff)

	// MEC_PMVX / MEC_PMVY
	#define MECPMV_1MV			BIT25
									// 1: 1MV mode, 0: 4MV mode
									// In decoding mode: not used and should be 0
	#define mMECPMV_4MVBN2b(v)	((uint32_t)(v) << 23)	// block number


	// MEC_DIA
	#define MECDIA_BIG_D				BIT24	//0:small diamond, 1: big diamond
	#define mMECDIA_MAX_LOOP8b(v)	((uint32_t)(v) << 16)
	#define mMECDIA_MINSADTH16b(v)	((uint32_t)(v) << 0)


	// MEC_REF
	#define MECREF_4MV				BIT28	//0:1mv mode, 1: 4mv mode
	#define mMECREF_SAD8TH12b(v)		((uint32_t)(v) << 16)
	#define mMECREF_MINSADTH16b(v)	((uint32_t)(v) << 0)

	// MEC_SAD
	#define MECSAD_LAST				BIT28
	#define MECSAD_MPMV				BIT27
	#define MECSAD_SRC				BIT26
	#define mMECSAD_MVX7b(v)		((uint32_t)(v) << 19)
	#define mMECSAD_MVY7b(v)		((uint32_t)(v) << 12)
	#define mMECSAD_MVXY14b(v)		((uint32_t)(v) << 12)
	#define mMECSAD_RADD12b(v)		((uint32_t)(v) << 0)

	// MEC_MOD
	#define MECMOD_EN				BIT28
	#define mMECMOD_INTRASADTH16b(v)		((uint32_t)(v) << 0)

	// MEC_PXI
	#define MECPXI_E_LAST			BIT28
	#define MECPXI_UV				(BIT27 | BIT25)
	#define mMECPXI_BKNUM2b(v)		((uint32_t)(v) << 23)
	#define MECPXI_GRP1				BIT22
	#define mMECPXI_RADD12b(v)		((uint32_t)(v) << 0)


#endif /* __ME_M_H  */

