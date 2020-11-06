#ifndef __ME_H
	#define __ME_H
#ifdef LINUX
	#include "../common/define.h"
	#include "local_mem_d.h"
	#include "decoder.h"
	#include "../common/me_m.h"
#else
	#include "define.h"
	#include "local_mem_d.h"
	#include "decoder.h"
	#include "me_m.h"
#endif

	/////////////////////////////////////////////////
	// ME command queue offset
	#define MV1_BUFF0			0		// 4W
	#define MV2_BUFF1			4		// 4W
	#define MV_BUFF			8		// 4W
	#define MVUV_BUFF			12		// 1W
	#define PMV_CMD_ST		13		// 8W
	#define PXI_CMD0_ST		21		// 6W
	#define PXI_CMD1_ST		27		// 6W


	/*

		1MV:

				REF1_Y_OFF-------
REF_Y_OFF_0 ------				|
				|				|
				V				V
				###########--###########---
				#Y	|	|	#	#Y	|	|	#	|
				#-----------#---#-----------#---
				#	|	|	#	#	|	|	#	|
				#-----------#---#-----------#---
				#	|	|	#	#	|	|	#	|
				###########--###########---
				|	|	|	|	|	|	|	|	|
				##########################
				#U	|	#V	|	#U	|	#V	|	#
				#-------#-------#------#--------#
				#	|	#	|	#	|	#	|	#
				##########################

		4MV:

				REF_Y0_OFF_1 -----
REF_Y0_OFF_0 -----				|
				|				|
				V				V
				##########################
				#Y0	|	#Y1	|	#Y0	|	#Y1	|	#
				#-------#-------#-------#------#
				#	|	#	|	#	|	#	|	#
				##########################
				#Y2	|	#Y3	|	#Y2	|	#Y3	|	#
				#-------#-------#------#-------#
				#	|	#	|	#	|	#	|	#
				###########################
				#U	|	#V	|	#U	|	#V	|	#
				#-------#-------#------#--------#
				#	|	#	|	#	|	#	|	#
				###########################
	*/
	//Y0
	#define CONST_MEPXI0Y0	(mMECMD_TYPE3b(MEC_PXI)| mMECPXI_BKNUM2b(0)| mMECPXI_RADD12b(REF_Y0_OFF_0 >> 2))
	
	// Y1
	#define CONST_MEPXI0Y1	(mMECMD_TYPE3b(MEC_PXI)| mMECPXI_BKNUM2b(1)| mMECPXI_RADD12b(REF_Y1_OFF_0 >> 2))
	
	// Y2
	#define CONST_MEPXI0Y2	(mMECMD_TYPE3b(MEC_PXI)| mMECPXI_BKNUM2b(2)| mMECPXI_RADD12b(REF_Y2_OFF_0 >> 2))
	
	// Y3
	#define CONST_MEPXI0Y3	(mMECMD_TYPE3b(MEC_PXI)| mMECPXI_BKNUM2b(3)| mMECPXI_RADD12b(REF_Y3_OFF_0 >> 2))
	
	// U
	#define CONST_MEPXI0U	(mMECMD_TYPE3b(MEC_PXI)| mMECPXI_BKNUM2b(0)| MECPXI_UV| mMECPXI_RADD12b(REF_U_OFF_0 >> 2))
	
	// V
	#define CONST_MEPXI0V	(mMECMD_TYPE3b(MEC_PXI)| MECPXI_E_LAST| mMECPXI_BKNUM2b(0)| MECPXI_UV| mMECPXI_RADD12b(REF_V_OFF_0 >> 2))

	//Y0
	#define CONST_MEPXI1Y0	(mMECMD_TYPE3b(MEC_PXI)| mMECPXI_BKNUM2b(0)| MECPXI_GRP1| mMECPXI_RADD12b(REF_Y0_OFF_1 >> 2))
	
	// Y1
	#define CONST_MEPXI1Y1	(mMECMD_TYPE3b(MEC_PXI)| mMECPXI_BKNUM2b(1)| MECPXI_GRP1| mMECPXI_RADD12b(REF_Y1_OFF_1 >> 2))
	
	// Y2
	#define CONST_MEPXI1Y2	(mMECMD_TYPE3b(MEC_PXI)| mMECPXI_BKNUM2b(2)| MECPXI_GRP1| mMECPXI_RADD12b(REF_Y2_OFF_1 >> 2))
	
	// Y3
	#define CONST_MEPXI1Y3	(mMECMD_TYPE3b(MEC_PXI)| mMECPXI_BKNUM2b(3)| MECPXI_GRP1| mMECPXI_RADD12b(REF_Y3_OFF_1 >> 2))
	
	// U
	#define CONST_MEPXI1U	(mMECMD_TYPE3b(MEC_PXI)| MECPXI_UV| mMECPXI_BKNUM2b(0)| MECPXI_GRP1| mMECPXI_RADD12b(REF_U_OFF_1 >> 2))
	
	// V
	#define CONST_MEPXI1V	(mMECMD_TYPE3b(MEC_PXI)| MECPXI_E_LAST| MECPXI_UV| mMECPXI_BKNUM2b(0)| MECPXI_GRP1| mMECPXI_RADD12b(REF_V_OFF_1 >> 2))


	#ifdef ME_GLOBALS
		#define ME_EXT
		const uint32_t u32ConstMePxi0[6] = {
			CONST_MEPXI0Y0,
			CONST_MEPXI0Y1,
			CONST_MEPXI0Y2,
			CONST_MEPXI0Y3,
			CONST_MEPXI0U,
			CONST_MEPXI0V
		};
		const uint32_t u32ConstMePxi1[6] = {
			CONST_MEPXI1Y0,
			CONST_MEPXI1Y1,
			CONST_MEPXI1Y2,
			CONST_MEPXI1Y3,
			CONST_MEPXI1U,
			CONST_MEPXI1V
		};
	#else
		#define ME_EXT extern
		extern uint32_t u32ConstMePxi0[];
		extern uint32_t u32ConstMePxi1[];
	#endif

	ME_EXT void me_dec_commandq_init(uint32_t base);
#endif /* __ME_H  */

