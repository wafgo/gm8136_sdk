#ifndef __ME_E_H
	#define __ME_E_H
#ifdef LINUX
    #include "../common/portab.h"
	#include "../common/me_m.h"
	#include "global_e.h"
	#include "encoder.h"
#else
	#include "portab.h"
	#include "me_m.h"
	#include "global_e.h"
	#include "encoder.h"
#endif

/*

offset:	pmv				sad				dia	ref		mod		pxi
=======================================================
	16:	PMVX 1MV block0
	17:	PMVY 1MV block0
	18:					SAD MPMV
	19:					SAD MV[1]
	20:					SAD
	21:					SAD
	22:					SAD LAST preMV[3]
	23:									DIA
	24:										REF 4MV
	25:	PMVX block0
	26:	PMVY block0
	27:					SAD MPMV
	28:					SAD
	29:					SAD
	30:					SAD
	31:					SAD LAST preMV[0]
	32:									DIA
	33:										REF 4MV
	34:	PMVX block1
 	35:	PMVY block1
	36:					SAD MPMV
	37:					SAD idx0
	38:					SAD
	39:					SAD
	40:					SAD LAST preMV[1]
	41:									DIA
	42:										Ref 4MV
	43:	PMVX block2
	44:	PMVY block2
	45:					SAD MPMV 23
	46:					SAD 23
	47:					SAD idx0
	48:					SAD idx1
	49:					SAD LAST preMV[2] 23
	50:									DIA
	51:										REF 4MV
	52:	PMVX block3
	53:	PMVY block3
	54:					SAD MPMV 23
	55:					SAD idx2
	56:					SAD idx0
	57:					SAD idx1
	58:					SAD LAST preMV[3] 23
	59:									DIA
	60:										REF 4MV
	61:												MOD
	62:													PXI
	63:													PXI UV
	64:													PXI UV LAST

*/
	#define ME_CMD_1MV	16
	#define ME_CMD_4MV0	(ME_CMD_1MV + 9)
	#define ME_CMD_4MV1	(ME_CMD_4MV0 + 9)
	#define ME_CMD_4MV2	(ME_CMD_4MV1 + 9)
	#define ME_CMD_4MV3	(ME_CMD_4MV2 + 9)
	#define ME_CMD_MOD	(ME_CMD_4MV3 + 9)

	#ifdef ME_E_GLOBALS
		#define ME_E_EXT
	#else
		#define ME_E_EXT extern
	#endif

    // Tuba 20111209_1
	//ME_E_EXT void me_enc_common_init(uint32_t base);
	ME_E_EXT void me_enc_common_init(uint32_t base, int disableIntra);
	#ifdef TWO_P_INT
	typedef struct 
	{
  	  MACROBLOCK_E *mbs_cur_row;
  	  MACROBLOCK_E *mbs_upper_row;
  	  MACROBLOCK_E *mbs_prev_ref;
     } MBS_NEIGHBOR_INFO;
	ME_E_EXT void me_cmd_set(MACROBLOCK_E_b *mbb_me, MACROBLOCK_E * mb_mc, Encoder * pEnc,MBS_NEIGHBOR_INFO *mbs_neighbor);
	#else	
	ME_E_EXT void me_cmd_set(MACROBLOCK_E_b *mbb_me, MACROBLOCK_E * mb_mc, Encoder * pEnc);
	#endif

#endif /* __ME_E_H  */
