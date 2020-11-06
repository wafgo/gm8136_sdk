#define ME_GLOBALS
#ifdef LINUX
#include "../common/portab.h"
#include "me.h"
#include "dma_b.h"
#include "../common/dma_m.h"
#include "decoder.h"
#include "local_mem_d.h"
#include "../common/define.h"
#include "../common/vpe_m.h"
#include "../common/mp4.h"
#else
#include "portab.h"
#include "me.h"
#include "dma_b.h"
#include "dma_m.h"
#include "decoder.h"
#include "local_mem_d.h"
#include "define.h"
#include "vpe_m.h"
#include "mp4.h"
#endif

void 
me_dec_commandq_init(uint32_t base)
{
	int i;
	uint32_t * me_cmd_q = (uint32_t *)(base + ME_CMD_Q_OFF);
	// init cmd_queue 0 ~ 12 to prevent PMV unknow
	for (i = 0; i < 13; i ++)
		me_cmd_q[i] = 0;

	//////////////////////////////////////////////////////////
	// init cmd_queue 13 - 20 for pmv
	// block 0
	me_cmd_q[PMV_CMD_ST + 0] = mMECMD_TYPE3b(MEC_PMVX)
								| mMECPMV_4MVBN2b(0);
	me_cmd_q[PMV_CMD_ST + 1] = mMECMD_TYPE3b(MEC_PMVY)
								| mMECPMV_4MVBN2b(0);
	// block 1
	me_cmd_q[PMV_CMD_ST + 2] = mMECMD_TYPE3b(MEC_PMVX)
								| mMECPMV_4MVBN2b(1);
	me_cmd_q[PMV_CMD_ST + 3] = mMECMD_TYPE3b(MEC_PMVY)
								| mMECPMV_4MVBN2b(1);
	// block 2
	me_cmd_q[PMV_CMD_ST + 4] = mMECMD_TYPE3b(MEC_PMVX)
								| mMECPMV_4MVBN2b(2);
	me_cmd_q[PMV_CMD_ST + 5] = mMECMD_TYPE3b(MEC_PMVY)
								| mMECPMV_4MVBN2b(2);
	// block 3
	me_cmd_q[PMV_CMD_ST + 6] = mMECMD_TYPE3b(MEC_PMVX)
								| mMECPMV_4MVBN2b(3);
	me_cmd_q[PMV_CMD_ST + 7] = mMECMD_TYPE3b(MEC_PMVY)
								| mMECPMV_4MVBN2b(3);
	//////////////////////////////////////////////////////////
	// init cmd_queue 21 - 32 for pmv
	for (i = 0; i < 6; i ++) {
		me_cmd_q[PXI_CMD0_ST + i] = u32ConstMePxi0[i];
		me_cmd_q[PXI_CMD1_ST + i] = u32ConstMePxi1[i];
	}

}
