#define ME_E_GLOBALS
#ifdef LINUX
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/version.h>
#include "../common/portab.h"
#include "me_e.h"
#include "../common/mp4.h"
#include "local_mem_e.h"
#include "motion.h"
#else
#include "portab.h"
#include "me_e.h"
#include "mp4.h"
#include "local_mem_e.h"
#include "motion.h"
#endif

/*

	4mv me_cmd:
		pmv				sad			dia		ref
====================================================
	16:	pmvx 1mv block0
	17:	pmvy 1mv block0
	18:					sad mpmv
	19:					sad
	20:					sad
	21:					sad
	22:					sad last preMV3
	23:								dia
	24:										ref 4mv
	25:  pmvx 4mv block0
	26:  pmvy 4mv block0
	27:					sad mpmv
	28:					sad
	29:					sad
	30:					sad
	31:					sad last preMV0
	32:								dia
	33:										ref 4mv

	34:	pmvx 4mv block1
	35:	pmvy 4mv block1
	36:					sad mpmv
	37:					sad idx0
	38:					sad
	39:					sad
	40:					sad last preMV1
	41:								dia
	42:										ref 4mv

	43:	pmvx 4mv block2
	44:	pmvy 4mv block2

	45:					sad mpmv
	46:					sad
	47:					sad idx0
	48:					sad idx1
	49:					sad last preMV2
	50:								dia
	51:										ref 4mv

	52:	pmvx 4mv block3
	53:	pmvy 4mv block3



	1mv me_cmd:
		pmv				sad			dia		ref
====================================================
	16:	pmvx 1mv block0
	17:	pmvy 1mv block0
	18:					sad mpmv
	19:					sad
	20:					sad
	21:					sad
	22:					sad last preMV3
	23:								dia
	24:										ref 1mv
*/

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
void 
me_enc_common_init(uint32_t base, int disableIntra)
{
	volatile uint32_t * me_cmd = (volatile uint32_t *)(base + ME_CMD_Q_OFF);
    uint32_t maxloop = (disableIntra?1:Diamond_search_limit);   // Tuba 20111209_1

	// to set horizontal prediction vector,BSIZE=1 (for 1MV),BK_BUM=0 (this bit should be 0 when BSIZE==1)
	me_cmd[16] =
			mMECMD_TYPE3b(MEC_PMVX) |
			MECPMV_1MV |
			mMECPMV_4MVBN2b(0);
	// to set vertical prediction vector,BSIZE=1 (for 1MV),BK_BUM=0 (this bit should be 0 when BSIZE==1)
	me_cmd[17] =
			mMECMD_TYPE3b(MEC_PMVY) |
			MECPMV_1MV |
			mMECPMV_4MVBN2b(0);
	me_cmd[18] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_MPMV |
			mMECSAD_RADD12b(RADDR);

	// *** followings for 4MV only,******
	// to set diamond command , dsize=0 (choose small diamond) , 
	// maxloop= (Diamond_search_limit << 16) , MinSADth= ThEES_4 (Threshold for minimim SAD for diamond stop)
	// according to the C model program, we always search the diamond from small diamond in 1MV macroblock search
	me_cmd[32] =
			mMECMD_TYPE3b(MEC_DIA) |
			/*mMECDIA_MAX_LOOP8b(Diamond_search_limit)|*/   // Tuba 20111209_1
            mMECDIA_MAX_LOOP8b(maxloop)|
			mMECDIA_MINSADTH16b(ThEES_4);


	// to set Half-Pel-Refine command , 4MV=1 (4MV mode is used), 
	// SAD8th = 0 (1MV/4MV threshold) , MinSADth= ThEES1 (Threshold for minimim SAD for skipping Half-Pel-Refine search)
	me_cmd[33] =
			mMECMD_TYPE3b(MEC_REF) |
			MECREF_4MV |
			mMECREF_SAD8TH12b(0) |
			mMECREF_MINSADTH16b(ThEES1);

	// to set horizontal prediction vector,BSIZE=0 (for 4MV),BK_BUM=1 (block number 1)
	me_cmd[34] =
			mMECMD_TYPE3b(MEC_PMVX) |
			mMECPMV_4MVBN2b(1);

	// to set vertical prediction vector,BSIZE=0 (for 4MV),BK_BUM=1 (block number 1)
	me_cmd[35] =
			mMECMD_TYPE3b(MEC_PMVY) |
			mMECPMV_4MVBN2b(1);
	me_cmd[36] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_MPMV |
			mMECSAD_RADD12b(RADDR);

	// to set SAD command (top neighbor),last=0 (not last SAD command), MPMV=0 (not use median predictor)
	// SRC=1 (the source is from MV buffer)
	// MVX=0 (get motion vector and reference start address from index 0 of MV buffer)
	// MVY and RADD is not used since SRC==1 
	me_cmd[37] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_SRC |
			mMECSAD_MVX7b(0);


	// to set diamond command , dsize=0 (choose small diamond) , 
	// maxloop= (Diamond_search_limit << 16) , MinSADth= ThEES_4 (Threshold for minimim SAD for diamond stop)
	me_cmd[41] =
			mMECMD_TYPE3b(MEC_DIA) |
			/*mMECDIA_MAX_LOOP8b(Diamond_search_limit)|*/   // Tuba 20111209_1
			mMECDIA_MAX_LOOP8b(maxloop)|			
			mMECDIA_MINSADTH16b(ThEES_4);

	// to set Half-Pel-Refine command , 4MV=1 (4MV mode is used), 
	// SAD8th = 0 (1MV/4MV threshold) , MinSADth= ThEES1 (Threshold for minimim SAD for skipping Half-Pel-Refine search)
	me_cmd[42] =
			mMECMD_TYPE3b(MEC_REF) |
			MECREF_4MV |
			mMECREF_SAD8TH12b(0) |
			mMECREF_MINSADTH16b(ThEES1);
	//-------------------------------------

	// to set horizontal prediction vector,BSIZE=0 (for 4MV),BK_BUM=2 (block number 2)
	me_cmd[43] =
			mMECMD_TYPE3b(MEC_PMVX) |
			mMECPMV_4MVBN2b(2);

	// to set vertical prediction vector,BSIZE=0 (for 4MV),BK_BUM=2 (block number 2)
	me_cmd[44] =
			mMECMD_TYPE3b(MEC_PMVY) |
			mMECPMV_4MVBN2b(2);

	// to set SAD command (median predictor) ,last=0 (not last SAD command), MPMV=1 (use median predictor) --- we select the median predictor as a candidate
	// SRC=0 (not meaningful since MPMV=1) ,MVX and MVY (not meaningful since MPMV=1)
	// RADD=Raddr23 (reference block start address)
	me_cmd[45] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_MPMV |
			mMECSAD_RADD12b(RADDR23);

	// to set SAD command (top neighbor),last=0 (not last SAD command), MPMV=0 (not use median predictor)
	// SRC=1 (the source is from MV buffer)
	// MVX=0 (get motion vector and reference start address from index 0 of MV buffer)
	// MVY and RADD is not used since SRC==1
	me_cmd[47] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_SRC |
			mMECSAD_MVX7b(0);

	// to set SAD command (top-right neighbor),last=0 (not last SAD command), MPMV=0 (not use median predictor)
	// SRC=1 (the source is from MV buffer)
	// MVX=0 (get motion vector and reference start address from index 1 of MV buffer)
	// MVY and RADD is not used since SRC==1
	me_cmd[48] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_SRC |
			mMECSAD_MVX7b(1);

	// to set diamond command , dsize=0 (choose small diamond) , 
	// maxloop= (Diamond_search_limit << 16) , MinSADth= ThEES_4 (Threshold for minimim SAD for diamond stop)
	me_cmd[50] =
			mMECMD_TYPE3b(MEC_DIA) |
			/*mMECDIA_MAX_LOOP8b(Diamond_search_limit)|*/   // Tuba 20111209_1
			mMECDIA_MAX_LOOP8b(maxloop)|			
			mMECDIA_MINSADTH16b(ThEES_4);

	// to set Half-Pel-Refine command , 4MV=1 (4MV mode is used), 
	// SAD8th = 0 (1MV/4MV threshold) , MinSADth= ThEES1 (Threshold for minimim SAD for skipping Half-Pel-Refine search)
	me_cmd[51] =
			mMECMD_TYPE3b(MEC_REF) |
			MECREF_4MV |
			mMECREF_SAD8TH12b(0) |
			mMECREF_MINSADTH16b(ThEES1);

	// to set horizontal prediction vector,BSIZE=0 (for 4MV),BK_BUM=3 (block number 3)
	me_cmd[52] =
			mMECMD_TYPE3b(MEC_PMVX) |
			mMECPMV_4MVBN2b(3);

	// to set vertical prediction vector,BSIZE=0 (for 4MV),BK_BUM=3 (block number 3)
	me_cmd[53] =
			mMECMD_TYPE3b(MEC_PMVY) |
			mMECPMV_4MVBN2b(3);
			

    // to set SAD command (median predictor) ,last=0 (not last SAD command), MPMV=1 (use median predictor) --- we select the median predictor as a candidate
	// SRC=0 (not meaningful since MPMV=1) ,MVX and MVY (not meaningful since MPMV=1)
	// RADD=Raddr (reference block start address)
	me_cmd[54] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_MPMV |
			mMECSAD_RADD12b(RADDR23);

	// to set SAD command (left neighbor),last=0 (not last SAD command), MPMV=0 (not use median predictor)
	// SRC=1 (the source is from MV buffer)
	// MVX=2 (get motion vector and reference start address from index 2 of MV buffer)
	// MVY and RADD is not used since SRC==1
	me_cmd[55] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_SRC |
			mMECSAD_MVX7b(2);

	// to set SAD command (top neighbor),last=0 (not last SAD command), MPMV=0 (not use median predictor)
	// SRC=1 (the source is from MV buffer)
	// MVX=0 (get motion vector and reference start address from index 0 of MV buffer)
	// MVY and RADD is not used since SRC==1
	me_cmd[56] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_SRC |
			mMECSAD_MVX7b(0);

	// to set SAD command (top-right neighbor),last=0 (not last SAD command), MPMV=0 (not use median predictor)
	// SRC=1 (the source is from MV buffer)
	// MVX=1 (get motion vector and reference start address from index 1 of MV buffer)
	// MVY and RADD is not used since SRC==1
	me_cmd[57] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_SRC |
			mMECSAD_MVX7b(1);

	// to set diamond command , dsize=0 (choose small diamond) , 
	// maxloop= (Diamond_search_limit << 16) , MinSADth= ThEES_4 (Threshold for minimim SAD for diamond stop)
	me_cmd[59] =
			mMECMD_TYPE3b(MEC_DIA) |
			/*mMECDIA_MAX_LOOP8b(Diamond_search_limit)|*/   // Tuba 20111209_1
			mMECDIA_MAX_LOOP8b(maxloop)|
			mMECDIA_MINSADTH16b(ThEES_4);

	// to set Half-Pel-Refine command , 4MV=1 (4MV mode is used), 
	// SAD8th = 0 (1MV/4MV threshold) , MinSADth= ThEES1 (Threshold for minimim SAD for skipping Half-Pel-Refine search)
	me_cmd[60] =
			mMECMD_TYPE3b(MEC_REF) |
			MECREF_4MV |
			mMECREF_SAD8TH12b(0) |
			mMECREF_MINSADTH16b(ThEES1);
	//----------------------------------------------------------------------------

	// to set Intra/Inter Mode Decision command , MODEN=1 (enable mode decision command), 
	// intraSADth= MV16_INTER_BIAS (threshold for intra/inter mode)
	// Tuba 20111209_1 start
	if (disableIntra) {
    	me_cmd[61] =
    			mMECMD_TYPE3b(MEC_MOD) |
    			mMECMOD_INTRASADTH16b(MV16_INTER_BIAS);
    }
    else {
        me_cmd[61] =
			mMECMD_TYPE3b(MEC_MOD) |
			MECMOD_EN |
			mMECMOD_INTRASADTH16b(MV16_INTER_BIAS);
    }
    // Tuba 20111209_1

	// to set pixel interpolation command , last=0 (not last pixel interpolation command), 
	// Chroma= 0 (select Y component)
	// RADD =0 (not used since Chroma==0)
	me_cmd[62] =
			mMECMD_TYPE3b(MEC_PXI) |
			mMECPXI_BKNUM2b(0) |
			mMECPXI_RADD12b(0);

	// to set pixel interpolation command , last=0 (not last pixel interpolation command), 
	// Chroma= 1 (select chrominance (Cb & Cr) component)
	// RADD =(((uint32_t) (REF_U + 32*8)) >> 2) (reference block start address)
	me_cmd[63] =
			mMECMD_TYPE3b(MEC_PXI) |
			MECPXI_UV |
			mMECPXI_RADD12b(((uint32_t) (REF_U + 32*8)) >> 2);

	// to set pixel interpolation command , last=1 (it's last pixel interpolation command), 
	// Chroma= 1 (select chrominance (Cb & Cr) component)
	// RADD =(((uint32_t) (REF_U + 32*8)) >> 2) (reference block start address)
	me_cmd[64] =
			mMECMD_TYPE3b(MEC_PXI) |
			MECPXI_E_LAST |
			MECPXI_UV |
			mMECPXI_RADD12b(((uint32_t) (REF_V + 32*8)) >> 2);

}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#ifdef GM8180_OPTIMIZATION

#ifdef TWO_P_INT
__inline void
get_pmv_4MV(
    MACROBLOCK_E_b *mbb_me,
	//const int x,
	//const int mbpos,
	union VECTOR1 * const pmv,
	Encoder * const pEnc,
	MBS_NEIGHBOR_INFO *mbs_neighbor)
#else
__inline void
get_pmv_4MV(
    MACROBLOCK_E_b *mbb_me,
	//const int x,
	//const int mbpos,
	union VECTOR1 * const pmv,
	Encoder * const pEnc)
#endif
{
	MACROBLOCK_E * mbs = pEnc->current1->mbs;
	int mb_width = pEnc->mb_width;
	int x = mbb_me->x;
	int mbpos = mbb_me->mbpos;
	int tpos, rpos;
	unsigned char me_pipe[2];
	
	me_pipe[0] = (mbb_me->mecmd_pipe & BIT0)      ^ ((mbb_me->mecmd_pipe & BIT16)>>16);
	me_pipe[1] = ((mbb_me->mecmd_pipe & BIT1)>>1) ^ ((mbb_me->mecmd_pipe & BIT17)>>17);
	
	//rpos = mbpos - mb_width + 1;
	
	if(me_pipe[1]) {
	  pmv[0].u32num = 0;
	  pmv[1].u32num = 0;
	  
	  if (x > 0) {
      //if ((x != (mb_width - 1)) && (mbpos >= 1)) {
	    #ifdef TWO_P_INT
	      pmv[0].u32num = mbs_neighbor->mbs_cur_row[(mbpos - 1)%ENC_MBS_LOCAL_P].mvs[1].u32num;
	      pmv[1].u32num = mbs_neighbor->mbs_cur_row[(mbpos - 1)%ENC_MBS_LOCAL_P].mvs[3].u32num;
	    #else
		  pmv[0].u32num = mbs[mbpos - 1].mvs[1].u32num;
		  pmv[1].u32num = mbs[mbpos - 1].mvs[3].u32num;
		#endif
	}
	else	if (pEnc->bResyncMarker) {
		volatile uint32_t * me_cmd = (volatile uint32_t *) (Mp4EncBase(pEnc) + ME_CMD_Q_OFF); //volatile uint32_t * me_cmd = (volatile uint32_t *) (pEnc->u32VirtualBaseAddr + ME_CMD_Q_OFF);
		me_cmd[1] = 0;
	}	
	} // if(me_pipe[1])
	
	if(me_pipe[0]) {      
	  pmv[2].u32num = 0;
	  pmv[3].u32num = 0;
	  pmv[4].u32num = 0;
	    
	if (pEnc->bResyncMarker == 0){
		tpos = mbpos - mb_width;
		if (tpos >= 0) {
		    #ifdef TWO_P_INT
		      pmv[2].u32num = mbs_neighbor->mbs_upper_row[tpos%ENC_MBS_ACDC].mvs[2].u32num;
			  pmv[3].u32num = mbs_neighbor->mbs_upper_row[tpos%ENC_MBS_ACDC].mvs[3].u32num;
		    #else
			  pmv[2].u32num = mbs[tpos].mvs[2].u32num;
			  pmv[3].u32num = mbs[tpos].mvs[3].u32num;
			#endif
		} else
			return;
		rpos = tpos + 1;
		if ((x + 1) != mb_width)
		  #ifdef TWO_P_INT
		    pmv[4].u32num = mbs_neighbor->mbs_upper_row[rpos%ENC_MBS_ACDC].mvs[2].u32num;
		  #else
			pmv[4].u32num = mbs[rpos].mvs[2].u32num;
		  #endif
	}
	}//if(me_pipe[0])
    
}

#else // #ifdef GM8180_OPTIMIZATION

#ifdef TWO_P_INT
__inline void
get_pmv_4MV(
	const int x,
	const int mbpos,
	union VECTOR1 * const pmv,
	Encoder * const pEnc,
	MBS_NEIGHBOR_INFO *mbs_neighbor)
#else
__inline void
get_pmv_4MV(
	const int x,
	const int mbpos,
	union VECTOR1 * const pmv,
	Encoder * const pEnc)
#endif
{
	MACROBLOCK_E * mbs = pEnc->current1->mbs;
	int mb_width = pEnc->mb_width;
	int tpos, rpos;

	rpos = mbpos - mb_width + 1;
	pmv[0].u32num = 0;
	pmv[1].u32num = 0;
	pmv[2].u32num = 0;
	pmv[3].u32num = 0;
	pmv[4].u32num = 0;

      if (x > 0) {
      //if ((x != (mb_width - 1)) && (mbpos >= 1)) {
	    #ifdef TWO_P_INT
	      pmv[0].u32num = mbs_neighbor->mbs_cur_row[(mbpos - 1)%ENC_MBS_LOCAL_P].mvs[1].u32num;
	      pmv[1].u32num = mbs_neighbor->mbs_cur_row[(mbpos - 1)%ENC_MBS_LOCAL_P].mvs[3].u32num;
	    #else
		  pmv[0].u32num = mbs[mbpos - 1].mvs[1].u32num;
		  pmv[1].u32num = mbs[mbpos - 1].mvs[3].u32num;
		#endif
	}
	else	if (pEnc->bResyncMarker) {
		volatile uint32_t * me_cmd = (volatile uint32_t *) (Mp4EncBase(pEnc) + ME_CMD_Q_OFF); //volatile uint32_t * me_cmd = (volatile uint32_t *) (pEnc->u32VirtualBaseAddr + ME_CMD_Q_OFF);
		me_cmd[1] = 0;
	}

	if (pEnc->bResyncMarker == 0){
		tpos = mbpos - mb_width;
		if (tpos >= 0) {
		    #ifdef TWO_P_INT
		      pmv[2].u32num = mbs_neighbor->mbs_upper_row[tpos%ENC_MBS_ACDC].mvs[2].u32num;
			  pmv[3].u32num = mbs_neighbor->mbs_upper_row[tpos%ENC_MBS_ACDC].mvs[3].u32num;
		    #else
			  pmv[2].u32num = mbs[tpos].mvs[2].u32num;
			  pmv[3].u32num = mbs[tpos].mvs[3].u32num;
			#endif
		} else
			return;
		rpos = tpos + 1;
		if ((x + 1) != mb_width)
		  #ifdef TWO_P_INT
		    pmv[4].u32num = mbs_neighbor->mbs_upper_row[rpos%ENC_MBS_ACDC].mvs[2].u32num;
		  #else
			pmv[4].u32num = mbs[rpos].mvs[2].u32num;
		  #endif
	}
}

#endif //#ifdef GM8180_OPTIMIZATION

#define LEFT_B1	0
#define LEFT_B3	1
#define TOP_B2	2
#define TOP_B3	3
#define RT_B2		4
#ifdef LINUX
#include "../common/vpe_m.h"
#else
#include "vpe_m.h"
#endif

#ifdef GM8180_OPTIMIZATION

#ifdef TWO_P_INT
void
me_cmd_set(MACROBLOCK_E_b *mbb_me, MACROBLOCK_E * mb_mc, Encoder * pEnc,MBS_NEIGHBOR_INFO *mbs_neighbor)
#else
void
me_cmd_set(MACROBLOCK_E_b *mbb_me, MACROBLOCK_E * mb_mc, Encoder * pEnc)
#endif
{
    volatile uint32_t * me_cmd = (volatile uint32_t *) (Mp4EncBase(pEnc) + ME_CMD_Q_OFF); //volatile uint32_t * me_cmd = (volatile uint32_t *) (pEnc->u32VirtualBaseAddr + ME_CMD_Q_OFF);
    
    #ifdef TWO_P_INT
      MACROBLOCK_E * prevMB = &(mbs_neighbor->mbs_prev_ref[mbb_me->mbpos%ENC_MBS_LOCAL_REF_P]);
    #else
	  MACROBLOCK_E * prevMB = &pEnc->reference->mbs[mbb_me->mbpos];
	#endif
	int32_t MOTION_ACTIVITY;
	int32_t d_type;
	int32_t s32temp;
	register uint32_t u32temp;
	// pmv mapping
	// 0: b1 of left mb
	// 1: b3 of left mb
	// 2: b2 of top mb
	// 3: b3 of top mb
	// 4: b2 of top-right mb

	//static union VECTOR1 pmv[5];
	//union VECTOR1 pmv[5];	
	union VECTOR1 *pmv=&(mbb_me->pmv[0]);
		
	int32_t first_row = (mbb_me->y == 0);
	volatile MP4_t * ptMP4 = pEnc->ptMP4; // debug usage, remove it later
	unsigned char me_pipe[2];
	me_pipe[0] = (mbb_me->mecmd_pipe & BIT0)      ^ ((mbb_me->mecmd_pipe & BIT16)>>16);
	me_pipe[1] = ((mbb_me->mecmd_pipe & BIT1)>>1) ^ ((mbb_me->mecmd_pipe & BIT17)>>17);
	//me_pipe[2] = ((mbb_me->mecmd_pipe & BIT2)>>2) ^ ((mbb_me->mecmd_pipe & BIT18)>>18);
	//me_pipe[3] = ((mbb_me->mecmd_pipe & BIT3)>>3) ^ ((mbb_me->mecmd_pipe & BIT19)>>19);	
	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x100)
	/* debug usage
	mVpe_Indicator(0x94100000 | (mbb_me->mecmd_pipe));
	mVpe_Indicator(0x94200000 | (me_pipe[0]));
	mVpe_Indicator(0x94300000 | (me_pipe[1]));
	mVpe_Indicator(0x94400000 | (me_pipe[2]));
	mVpe_Indicator(0x94500000 | (me_pipe[3]));
	*/
	mPOLL_MECMD_SET_MARKER_2(ptMP4,(0x10000000 | (mbb_me->mecmd_pipe)))
	mPOLL_MECMD_SET_MARKER_2(ptMP4,(0x20000000 | (me_pipe[0])))
	mPOLL_MECMD_SET_MARKER_2(ptMP4,(0x30000000 | (me_pipe[1])))
	//mPOLL_MECMD_SET_MARKER(ptMP4,(0x40000000 | (me_pipe[2])))
	//mPOLL_MECMD_SET_MARKER(ptMP4,(0x50000000 | (me_pipe[3])))

    
    //if(me_pipe[0]) {
    #ifdef TWO_P_INT
      get_pmv_4MV (mbb_me, pmv, pEnc, mbs_neighbor);
    #else
	  get_pmv_4MV (mbb_me, pmv, pEnc);
	#endif
	//}
	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x110)
	
	if(me_pipe[1]) {	
	  MOTION_ACTIVITY = MAX3 (
						FUN_ABS(pmv[LEFT_B1].vec.s16x) + FUN_ABS(pmv[LEFT_B1].vec.s16y),
						FUN_ABS(pmv[TOP_B2].vec.s16x) + FUN_ABS(pmv[TOP_B2].vec.s16y),
						FUN_ABS(pmv[RT_B2].vec.s16x) + FUN_ABS(pmv[RT_B2].vec.s16y));
	}
	
	/* debug usage
	mVpe_Indicator(0x93100000 | (pmv[LEFT_B1].vec.s16x & 0xFF));
	mVpe_Indicator(0x93200000 | (pmv[LEFT_B1].vec.s16y & 0xFF));
	mVpe_Indicator(0x93300000 | (pmv[LEFT_B3].vec.s16x & 0xFF));
	mVpe_Indicator(0x93400000 | (pmv[LEFT_B3].vec.s16y & 0xFF));
	mVpe_Indicator(0x93500000 | (pmv[TOP_B2].vec.s16x & 0xFF));
	mVpe_Indicator(0x93600000 | (pmv[TOP_B2].vec.s16y & 0xFF));
	mVpe_Indicator(0x93700000 | (pmv[TOP_B3].vec.s16x & 0xFF));
	mVpe_Indicator(0x93800000 | (pmv[TOP_B3].vec.s16y & 0xFF));
	mVpe_Indicator(0x93900000 | (pmv[RT_B2].vec.s16x & 0xFF));
	mVpe_Indicator(0x93a00000 | (pmv[RT_B2].vec.s16y & 0xFF));
	*/

    mPOLL_MECMD_SET_MARKER(ptMP4,0x120)
    
	// me_cmd[19]
	// me_cmd[28]
	if(me_pipe[1]) {
	mPOLL_MECMD_SET_MARKER(ptMP4,0x121)	
	u32temp =
		mMECMD_TYPE3b(MEC_SAD) |
		mMECSAD_MVX7b(pmv[LEFT_B1].vec.s16x & 0x7f) |
		mMECSAD_MVY7b(pmv[LEFT_B1].vec.s16y & 0x7f) |
		mMECSAD_RADD12b(RADDR +
			((pmv[LEFT_B1].vec.s16y < 0) && (mbb_me->y == 0)?
			0: (pmv[LEFT_B1].vec.s16y >> 1)*16));
	me_cmd[19] = u32temp;
	if (pEnc->bEnable_4mv)
		me_cmd[28] = u32temp;		
	}
	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x130)	

	// me_cmd[20]
	// me_cmd[29]
	if(me_pipe[0]) {
	  mPOLL_MECMD_SET_MARKER(ptMP4,0x131)	
	u32temp =
		mMECMD_TYPE3b(MEC_SAD) |
		mMECSAD_MVX7b(pmv[TOP_B2].vec.s16x & 0x7f) |
		mMECSAD_MVY7b(pmv[TOP_B2].vec.s16y & 0x7f);
	if (pEnc->bEnable_4mv) {
		me_cmd[20] =
		me_cmd[29] = u32temp |
			mMECSAD_RADD12b(RADDR +
				((pmv[TOP_B2].vec.s16y < 0) && (mbb_me->y == 0) ? 0: (pmv[TOP_B2].vec.s16y >> 1)*16));
	}
	else {
	    me_cmd[20] =u32temp |
			mMECSAD_RADD12b(RADDR +
				((first_row && pmv[TOP_B2].vec.s16y < 0) ? 0: (pmv[TOP_B2].vec.s16y >> 1)*16));
	}
	}
	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x140)	

	// me_cmd[21]
	// me_cmd[30]
	// me_cmd[39]
	if(me_pipe[0]) {
	  mPOLL_MECMD_SET_MARKER(ptMP4,0x141)	
	u32temp =
		mMECMD_TYPE3b(MEC_SAD) |
		mMECSAD_MVX7b(pmv[RT_B2].vec.s16x & 0x7f) |
		mMECSAD_MVY7b(pmv[RT_B2].vec.s16y & 0x7f);
	if (pEnc->bEnable_4mv) {
		me_cmd[21] =
		me_cmd[30] =
		me_cmd[39] =
		    mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(pmv[RT_B2].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(pmv[RT_B2].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((pmv[RT_B2].vec.s16y < 0) && first_row ? 0: (pmv[RT_B2].vec.s16y >> 1)*16));
	}
	else {
	    me_cmd[21] =
			u32temp |
			mMECSAD_RADD12b(RADDR +
				((first_row && (pmv[RT_B2].vec.s16y < 0)) ? 0: (pmv[RT_B2].vec.s16y >> 1)*16));
	}
	}
	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x150)	
	
	// me_cmd[22]
	if(me_pipe[0]) {
	  mPOLL_MECMD_SET_MARKER(ptMP4,0x151)
	u32temp = 
		mMECMD_TYPE3b(MEC_SAD) |
		MECSAD_LAST |
		mMECSAD_MVX7b(prevMB->mvs[3].vec.s16x & 0x7f) |
		mMECSAD_MVY7b(prevMB->mvs[3].vec.s16y & 0x7f);
	if (pEnc->bEnable_4mv)
		me_cmd[22] =
			u32temp |
			mMECSAD_RADD12b(RADDR +
				((prevMB->mvs[3].vec.s16y < 0) && first_row ? 0 : (prevMB->mvs[3].vec.s16y >> 1)*16));
	else
	    me_cmd[22] =
			u32temp |
			mMECSAD_RADD12b(RADDR +
				((first_row && (prevMB->mvs[3].vec.s16y < 0))? 0 : (prevMB->mvs[3].vec.s16y >> 1)*16));
	
	}
	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x160)	
	
	if(me_pipe[1]) {
	  mPOLL_MECMD_SET_MARKER(ptMP4,0x161)	
	// me_cmd[23]
	d_type = (MOTION_ACTIVITY>L1) ? MECDIA_BIG_D: 0;
    // Tuba 20111209_1
    if (pEnc->disableModeDecision) {
    	me_cmd[23] =
    		mMECMD_TYPE3b(MEC_DIA) |
    		d_type |
    		mMECDIA_MAX_LOOP8b(1) |
    		mMECDIA_MINSADTH16b(ThEES);
    	}
    }
    else {
        me_cmd[23] =
    		mMECMD_TYPE3b(MEC_DIA) |
    		d_type |
    		mMECDIA_MAX_LOOP8b(Diamond_search_limit) |
    		mMECDIA_MINSADTH16b(ThEES);
    	}
    }
	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x170)
	
	if (pEnc->bEnable_4mv) {
	    mPOLL_MECMD_SET_MARKER(ptMP4,0x171)
		// me_cmd[31]		
		me_cmd[31] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_LAST |
			mMECSAD_MVX7b(prevMB->mvs[0].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(prevMB->mvs[0].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((prevMB->mvs[0].vec.s16y < 0) && first_row ? 0 : (prevMB->mvs[0].vec.s16y >> 1)*16));
		
		// me_cmd[38]
		me_cmd[38] =
			mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(pmv[TOP_B3].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(pmv[TOP_B3].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((pmv[TOP_B3].vec.s16y < 0) && first_row ? 0: (pmv[TOP_B3].vec.s16y >> 1)*16));
				
		mPOLL_MECMD_SET_MARKER(ptMP4,0x172)
		
		// me_cmd[40]
		me_cmd[40] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_LAST |
			mMECSAD_MVX7b(prevMB->mvs[1].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(prevMB->mvs[1].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((prevMB->mvs[1].vec.s16y < 0) && first_row ? 0 : (prevMB->mvs[1].vec.s16y >> 1)*16));

        mPOLL_MECMD_SET_MARKER(ptMP4,0x173)
        
        // to determine whether it is last row or not
		if ((mbb_me->y == (pEnc->mb_height - 1)))
			d_type = 1;
		else
			d_type = 0;
		
	    mPOLL_MECMD_SET_MARKER(ptMP4,0x174)
	    
		// me_cmd[46]
		s32temp = pmv[LEFT_B3].vec.s16y;
		if ((s32temp < -16) && first_row)
			s32temp = ((-16>>1) * 16);
		else {
			if ((s32temp > 15) && d_type)
				s32temp = ((15>>1) * 16);
			else
				s32temp = (s32temp >> 1)*16;
		}
		me_cmd[46] =
			mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(pmv[LEFT_B3].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(pmv[LEFT_B3].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR23 + s32temp);
		// me_cmd[49]
		s32temp = prevMB->mvs[2].vec.s16y;
		if ((s32temp < -16) && first_row)
			s32temp = ((-16>>1) * 16);
		else {
			if ((s32temp > 15) && d_type)
				s32temp = ((15>>1) * 16);
			else
				s32temp = (s32temp >> 1)*16;
		}

        me_cmd[49] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_LAST |
			mMECSAD_MVX7b(prevMB->mvs[2].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(prevMB->mvs[2].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR23 + s32temp);
		

	    mPOLL_MECMD_SET_MARKER(ptMP4,0x175)
	    
		// me_cmd[58]
		s32temp = prevMB->mvs[3].vec.s16y;
		if ((s32temp < -16) && first_row)
			s32temp = ((-16>>1) * 16);
		else {
			if ((s32temp > 15) && d_type)
				s32temp = ((15>>1) * 16);
			else
				s32temp = (s32temp >> 1)*16;
		}

        me_cmd[58] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_LAST |
			mMECSAD_MVX7b(prevMB->mvs[3].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(prevMB->mvs[3].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR23 + s32temp);
			
		mPOLL_MECMD_SET_MARKER(ptMP4,0x176)
	}
	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x180)
	
	// to indicate the stage is done
	/*
	for(u32temp=0;u32temp<4;u32temp++) {	
	  if(me_pipe[u32temp])
	    mbb_me->mecmd_pipe |= (1<<(u32temp+16));
	}*/
	
	// to indicate the stage is done
	//mbb_me->mecmd_pipe |= ((me_pipe[0] ? BIT16:0)|(me_pipe[1] ? BIT17:0)|(me_pipe[2] ? BIT18:0)|(me_pipe[3] ? BIT19:0));
	mbb_me->mecmd_pipe |= ((me_pipe[0] ? BIT16:0)|(me_pipe[1] ? BIT17:0));
	
	mPOLL_MECMD_SET_MARKER(ptMP4,(0x60000000 | (mbb_me->mecmd_pipe)))	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x190)
}


#else // else of #ifdef GM8180_OPTIMIZATION

#ifdef TWO_P_INT
void
me_cmd_set(MACROBLOCK_E_b *mbb_me, MACROBLOCK_E * mb_mc, Encoder * pEnc,MBS_NEIGHBOR_INFO *mbs_neighbor)
#else
void
me_cmd_set(MACROBLOCK_E_b *mbb_me, MACROBLOCK_E * mb_mc, Encoder * pEnc)
#endif
{
    volatile uint32_t * me_cmd = (volatile uint32_t *) (Mp4EncBase(pEnc) + ME_CMD_Q_OFF); //volatile uint32_t * me_cmd = (volatile uint32_t *) (pEnc->u32VirtualBaseAddr + ME_CMD_Q_OFF);
    
    #ifdef TWO_P_INT
      MACROBLOCK_E * prevMB = &(mbs_neighbor->mbs_prev_ref[mbb_me->mbpos%ENC_MBS_LOCAL_REF_P]);
    #else
	  MACROBLOCK_E * prevMB = &pEnc->reference->mbs[mbb_me->mbpos];
	#endif
	int32_t MOTION_ACTIVITY;
	int32_t d_type;
	int32_t s32temp;
	register uint32_t u32temp;
	// pmv mapping
	// 0: b1 of left mb
	// 1: b3 of left mb
	// 2: b2 of top mb
	// 3: b3 of top mb
	// 4: b2 of top-right mb
	union VECTOR1 pmv[5];
	int32_t first_row = (mbb_me->y == 0);
	
//	volatile MP4_t * ptMP4 = pEnc->ptMP4; // debug usage, remove it later
    mPOLL_MECMD_SET_MARKER(ptMP4,0x100)

    #ifdef TWO_P_INT
      get_pmv_4MV (mbb_me->x, mbb_me->mbpos, pmv, pEnc, mbs_neighbor);
    #else
	  get_pmv_4MV (mbb_me->x, mbb_me->mbpos, pmv, pEnc);
	#endif
	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x110)
	
	MOTION_ACTIVITY = MAX3 (
						FUN_ABS(pmv[LEFT_B1].vec.s16x) + FUN_ABS(pmv[LEFT_B1].vec.s16y),
						FUN_ABS(pmv[TOP_B2].vec.s16x) + FUN_ABS(pmv[TOP_B2].vec.s16y),
						FUN_ABS(pmv[RT_B2].vec.s16x) + FUN_ABS(pmv[RT_B2].vec.s16y));
	/* debug print purpose
	mVpe_Indicator(0x93100000 | (pmv[LEFT_B1].vec.s16x & 0xFF));
	mVpe_Indicator(0x93200000 | (pmv[LEFT_B1].vec.s16y & 0xFF));
	mVpe_Indicator(0x93300000 | (pmv[LEFT_B3].vec.s16x & 0xFF));
	mVpe_Indicator(0x93400000 | (pmv[LEFT_B3].vec.s16y & 0xFF));
	mVpe_Indicator(0x93500000 | (pmv[TOP_B2].vec.s16x & 0xFF));
	mVpe_Indicator(0x93600000 | (pmv[TOP_B2].vec.s16y & 0xFF));
	mVpe_Indicator(0x93700000 | (pmv[TOP_B3].vec.s16x & 0xFF));
	mVpe_Indicator(0x93800000 | (pmv[TOP_B3].vec.s16y & 0xFF));
	mVpe_Indicator(0x93900000 | (pmv[RT_B2].vec.s16x & 0xFF));
	mVpe_Indicator(0x93a00000 | (pmv[RT_B2].vec.s16y & 0xFF));
	*/
	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x120)

	// me_cmd[19]
	// me_cmd[28]
	u32temp =
		mMECMD_TYPE3b(MEC_SAD) |
		mMECSAD_MVX7b(pmv[LEFT_B1].vec.s16x & 0x7f) |
		mMECSAD_MVY7b(pmv[LEFT_B1].vec.s16y & 0x7f) |
		mMECSAD_RADD12b(RADDR +
			((pmv[LEFT_B1].vec.s16y < 0) && (mbb_me->y == 0)?
			0: (pmv[LEFT_B1].vec.s16y >> 1)*16));
	me_cmd[19] = u32temp;
	if (pEnc->bEnable_4mv)
		me_cmd[28] = u32temp;

    mPOLL_MECMD_SET_MARKER(ptMP4,0x130)
    
	// me_cmd[20]
	// me_cmd[29]
	u32temp =
		mMECMD_TYPE3b(MEC_SAD) |
		mMECSAD_MVX7b(pmv[TOP_B2].vec.s16x & 0x7f) |
		mMECSAD_MVY7b(pmv[TOP_B2].vec.s16y & 0x7f);
	if (pEnc->bEnable_4mv) {
		me_cmd[20] =
		me_cmd[29] = u32temp |
			mMECSAD_RADD12b(RADDR +
				((pmv[TOP_B2].vec.s16y < 0) && (mbb_me->y == 0) ? 0: (pmv[TOP_B2].vec.s16y >> 1)*16));
	}
	else {
	    me_cmd[20] =u32temp |
			mMECSAD_RADD12b(RADDR +
				((first_row && pmv[TOP_B2].vec.s16y < 0) ? 0: (pmv[TOP_B2].vec.s16y >> 1)*16));
	}
    
    mPOLL_MECMD_SET_MARKER(ptMP4,0x140)	
    
	// me_cmd[21]
	// me_cmd[30]
	// me_cmd[39]
	u32temp =
		mMECMD_TYPE3b(MEC_SAD) |
		mMECSAD_MVX7b(pmv[RT_B2].vec.s16x & 0x7f) |
		mMECSAD_MVY7b(pmv[RT_B2].vec.s16y & 0x7f);
	if (pEnc->bEnable_4mv) {
		me_cmd[21] =
		me_cmd[30] =
		me_cmd[39] =
			mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(pmv[RT_B2].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(pmv[RT_B2].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((pmv[RT_B2].vec.s16y < 0) && first_row ? 0: (pmv[RT_B2].vec.s16y >> 1)*16));
	}
	else {
	    me_cmd[21] =
			u32temp |
			mMECSAD_RADD12b(RADDR +
				((first_row && (pmv[RT_B2].vec.s16y < 0)) ? 0: (pmv[RT_B2].vec.s16y >> 1)*16));
	}
	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x150)	
	
	// me_cmd[22]
	u32temp = 
		mMECMD_TYPE3b(MEC_SAD) |
		MECSAD_LAST |
		mMECSAD_MVX7b(prevMB->mvs[3].vec.s16x & 0x7f) |
		mMECSAD_MVY7b(prevMB->mvs[3].vec.s16y & 0x7f);
	if (pEnc->bEnable_4mv)
		me_cmd[22] =
			u32temp |
			mMECSAD_RADD12b(RADDR +
				((prevMB->mvs[3].vec.s16y < 0) && first_row ? 0 : (prevMB->mvs[3].vec.s16y >> 1)*16));
	else
	    me_cmd[22] =
			u32temp |
			mMECSAD_RADD12b(RADDR +
				((first_row && (prevMB->mvs[3].vec.s16y < 0))? 0 : (prevMB->mvs[3].vec.s16y >> 1)*16));
				
	mPOLL_MECMD_SET_MARKER(ptMP4,0x160)	
	
	// me_cmd[23]
	d_type = (MOTION_ACTIVITY>L1) ? MECDIA_BIG_D: 0;
    // Tuba 20111209_1
    if (pEnc->disableModeDecision) {
    	me_cmd[23] =
    		mMECMD_TYPE3b(MEC_DIA) |
    		d_type |
    		mMECDIA_MAX_LOOP8b(1) |
    		mMECDIA_MINSADTH16b(ThEES);
    }
    else {
        me_cmd[23] =
    		mMECMD_TYPE3b(MEC_DIA) |
    		d_type |
    		mMECDIA_MAX_LOOP8b(Diamond_search_limit) |
    		mMECDIA_MINSADTH16b(ThEES);
    }
		
	mPOLL_MECMD_SET_MARKER(ptMP4,0x170)
	
	if (pEnc->bEnable_4mv) {
	    mPOLL_MECMD_SET_MARKER(ptMP4,0x171)
		// me_cmd[31]
		me_cmd[31] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_LAST |
			mMECSAD_MVX7b(prevMB->mvs[0].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(prevMB->mvs[0].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((prevMB->mvs[0].vec.s16y < 0) && first_row ? 0 : (prevMB->mvs[0].vec.s16y >> 1)*16));
		// me_cmd[38]
		me_cmd[38] =
			mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(pmv[TOP_B3].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(pmv[TOP_B3].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((pmv[TOP_B3].vec.s16y < 0) && first_row ? 0: (pmv[TOP_B3].vec.s16y >> 1)*16));
				
		mPOLL_MECMD_SET_MARKER(ptMP4,0x172)
		
		// me_cmd[40]
		me_cmd[40] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_LAST |
			mMECSAD_MVX7b(prevMB->mvs[1].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(prevMB->mvs[1].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((prevMB->mvs[1].vec.s16y < 0) && first_row ? 0 : (prevMB->mvs[1].vec.s16y >> 1)*16));
				
		mPOLL_MECMD_SET_MARKER(ptMP4,0x173)

        // to determine whether it is last row or not
		if ((mbb_me->y == (pEnc->mb_height - 1)))
			d_type = 1;
		else
			d_type = 0;
			
		mPOLL_MECMD_SET_MARKER(ptMP4,0x174)
			
		// me_cmd[46]
		s32temp = pmv[LEFT_B3].vec.s16y;
		if ((s32temp < -16) && first_row)
			s32temp = ((-16>>1) * 16);
		else {
			if ((s32temp > 15) && d_type)
				s32temp = ((15>>1) * 16);
			else
				s32temp = (s32temp >> 1)*16;
		}
		me_cmd[46] =
			mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(pmv[LEFT_B3].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(pmv[LEFT_B3].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR23 + s32temp);
		// me_cmd[49]
		s32temp = prevMB->mvs[2].vec.s16y;
		if ((s32temp < -16) && first_row)
			s32temp = ((-16>>1) * 16);
		else {
			if ((s32temp > 15) && d_type)
				s32temp = ((15>>1) * 16);
			else
				s32temp = (s32temp >> 1)*16;
		}

        me_cmd[49] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_LAST |
			mMECSAD_MVX7b(prevMB->mvs[2].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(prevMB->mvs[2].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR23 + s32temp);
			
		mPOLL_MECMD_SET_MARKER(ptMP4,0x175)
		
		// me_cmd[58]
		s32temp = prevMB->mvs[3].vec.s16y;
		if ((s32temp < -16) && first_row)
			s32temp = ((-16>>1) * 16);
		else {
			if ((s32temp > 15) && d_type)
				s32temp = ((15>>1) * 16);
			else
				s32temp = (s32temp >> 1)*16;
		}

        me_cmd[58] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_LAST |
			mMECSAD_MVX7b(prevMB->mvs[3].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(prevMB->mvs[3].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR23 + s32temp);
			
		mPOLL_MECMD_SET_MARKER(ptMP4,0x176)
	}
	
	mPOLL_MECMD_SET_MARKER(ptMP4,0x180)
	
#if 0
	// me_cmd[19]
	me_cmd[19] =
		mMECMD_TYPE3b(MEC_SAD) |
		mMECSAD_MVX7b(pmv[LEFT_B1].vec.s16x & 0x7f) |
		mMECSAD_MVY7b(pmv[LEFT_B1].vec.s16y & 0x7f) |
		mMECSAD_RADD12b(RADDR +
			((pmv[LEFT_B1].vec.s16y < 0) && (mbb_me->y == 0)? 0: (pmv[LEFT_B1].vec.s16y >> 1)*16));
	if (pEnc->bEnable_4mv) {
		// me_cmd[28]
		me_cmd[28] =
			mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(pmv[LEFT_B1].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(pmv[LEFT_B1].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((pmv[LEFT_B1].vec.s16y < 0) && (mbb_me->y == 0) ? 0: (pmv[LEFT_B1].vec.s16y >> 1)*16));
	}
	// me_cmd[20]
	me_cmd[20] =
		mMECMD_TYPE3b(MEC_SAD) |
		mMECSAD_MVX7b(pmv[TOP_B2].vec.s16x & 0x7f) |
		mMECSAD_MVY7b(pmv[TOP_B2].vec.s16y & 0x7f) |
		mMECSAD_RADD12b(RADDR +
			((pmv[TOP_B2].vec.s16y < 0) && first_row ? 0: (pmv[TOP_B2].vec.s16y >> 1)*16));
	if (pEnc->bEnable_4mv) {
		// me_cmd[29]
		me_cmd[29] =
			mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(pmv[TOP_B2].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(pmv[TOP_B2].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((pmv[TOP_B2].vec.s16y < 0) && first_row ? 0: (pmv[TOP_B2].vec.s16y >> 1)*16));
	}
	// me_cmd[21]
	me_cmd[21] =
		mMECMD_TYPE3b(MEC_SAD) |
		mMECSAD_MVX7b(pmv[RT_B2].vec.s16x & 0x7f) |
		mMECSAD_MVY7b(pmv[RT_B2].vec.s16y & 0x7f) |
		mMECSAD_RADD12b(RADDR +
			((pmv[RT_B2].vec.s16y < 0) && first_row ? 0: (pmv[RT_B2].vec.s16y >> 1)*16));
	if (pEnc->bEnable_4mv) {
		// me_cmd[30]
		// me_cmd[39]
		me_cmd[30] =
		me_cmd[39] =
			mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(pmv[RT_B2].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(pmv[RT_B2].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((pmv[RT_B2].vec.s16y < 0) && first_row ? 0: (pmv[RT_B2].vec.s16y >> 1)*16));
	}
	// me_cmd[22]
	me_cmd[22] =
		mMECMD_TYPE3b(MEC_SAD) |
		MECSAD_LAST |
		mMECSAD_MVX7b(prevMB->mvs[3].vec.s16x & 0x7f) |
		mMECSAD_MVY7b(prevMB->mvs[3].vec.s16y & 0x7f) |
		mMECSAD_RADD12b(RADDR +
			((prevMB->mvs[3].vec.s16y < 0) && first_row ? 0 : (prevMB->mvs[3].vec.s16y >> 1)*16));
	// me_cmd[23]
	d_type = (MOTION_ACTIVITY>L1) ? MECDIA_BIG_D: 0;
	me_cmd[23] =
		mMECMD_TYPE3b(MEC_DIA) |
		d_type |
		mMECDIA_MAX_LOOP8b(Diamond_search_limit) |
		mMECDIA_MINSADTH16b(ThEES);
	if (pEnc->bEnable_4mv) {
		// me_cmd[31]
		me_cmd[31] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_LAST |
			mMECSAD_MVX7b(prevMB->mvs[0].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(prevMB->mvs[0].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((prevMB->mvs[0].vec.s16y < 0) && first_row ? 0 : (prevMB->mvs[0].vec.s16y >> 1)*16));
		// me_cmd[38]
		me_cmd[38] =
			mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(pmv[TOP_B3].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(pmv[TOP_B3].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((pmv[TOP_B3].vec.s16y < 0) && first_row ? 0: (pmv[TOP_B3].vec.s16y >> 1)*16));
		// me_cmd[40]
		me_cmd[40] =
			mMECMD_TYPE3b(MEC_SAD) |
			MECSAD_LAST |
			mMECSAD_MVX7b(prevMB->mvs[1].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(prevMB->mvs[1].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR +
				((prevMB->mvs[1].vec.s16y < 0) && first_row ? 0 : (prevMB->mvs[1].vec.s16y >> 1)*16));

		if ((mbb_me->y == (pEnc->mb_height - 1)) ||
			((mbb_me->y==(pEnc->mb_height - 2)) && (mbb_me->x == (pEnc->mb_width-1))))
			d_type = 1;
		else
			d_type = 0;
		// me_cmd[46]
		s32temp = pmv[LEFT_B3].vec.s16y;
		if ((s32temp < -16) && first_row)
			s32temp = ((-16>>1) * 16);
		else {
			if ((s32temp > 15) && d_type)
				s32temp = ((15>>1) * 16);
			else
				s32temp = (s32temp >> 1)*16;
		}
		me_cmd[46] =
			mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(pmv[LEFT_B3].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(pmv[LEFT_B3].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR23 + s32temp);
		// me_cmd[49]
		s32temp = prevMB->mvs[2].vec.s16y;
		if ((s32temp < -16) && first_row)
			s32temp = ((-16>>1) * 16);
		else {
			if ((s32temp > 15) && d_type)
				s32temp = ((15>>1) * 16);
			else
				s32temp = (s32temp >> 1)*16;
		}
		me_cmd[49] =
			mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(prevMB->mvs[2].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(prevMB->mvs[2].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR23 + s32temp);
		// me_cmd[58]
		s32temp = prevMB->mvs[3].vec.s16y;
		if ((s32temp < -16) && first_row)
			s32temp = ((-16>>1) * 16);
		else {
			if ((s32temp > 15) && d_type)
				s32temp = ((15>>1) * 16);
			else
				s32temp = (s32temp >> 1)*16;
		}
		me_cmd[58] =
			mMECMD_TYPE3b(MEC_SAD) |
			mMECSAD_MVX7b(prevMB->mvs[3].vec.s16x & 0x7f) |
			mMECSAD_MVY7b(prevMB->mvs[3].vec.s16y & 0x7f) |
			mMECSAD_RADD12b(RADDR23 + s32temp);
	}
#endif

}

#endif // end of #ifdef GM8180_OPTIMIZATION
