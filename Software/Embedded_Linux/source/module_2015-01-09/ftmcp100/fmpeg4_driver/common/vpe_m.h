#ifndef __VPE_M_H
	#define __VPE_M_H
	#include "mp4.h"
	#define VPE 		0x90180000

	#define WAIT_VLD_START		0x10000000
	#define WAIT_VLD_END		0x10000001
	#define WAIT_DMC_START	0x10000010
	#define WAIT_DMC_END		0x10000011
	#define WAIT_DMA_START	0x10000020
	#define WAIT_DMA_END		0x10000021
	#define WAIT_ME_START		0x10000030
	#define WAIT_ME_END		0x10000031
	#define WAIT_DT_START		0x10000040
	#define WAIT_DT_END		0x10000041
	#define WAIT_START			0x10000050
	#define WAIT_END			0x10000051
     
	#if defined(FPGA)
		#define vpe_prob_vld_start()
		#define vpe_prob_vld_end()
		#define vpe_prob_dmc_start()
		#define vpe_prob_dmc_end()
		#define vpe_prob_dma_start()
		#define vpe_prob_dma_end()
		#define vpe_prob_me_start()
		#define vpe_prob_me_end()
		#define vpe_prob_dt_start()
		#define vpe_prob_dt_end()
	#else
		#define vpe_prob_vld_start()	*(volatile uint32_t *)VPE = WAIT_VLD_START
		#define vpe_prob_vld_end()	*(volatile uint32_t *)VPE = WAIT_VLD_END
		#define vpe_prob_dmc_start()	*(volatile uint32_t *)VPE = WAIT_DMC_START
		#define vpe_prob_dmc_end()	*(volatile uint32_t *)VPE = WAIT_DMC_END
		#define vpe_prob_dma_start()	*(volatile uint32_t *)VPE = WAIT_DMA_START
		#define vpe_prob_dma_end()	*(volatile uint32_t *)VPE = WAIT_DMA_END
		#define vpe_prob_me_start()	*(volatile uint32_t *)VPE = WAIT_ME_START
		#define vpe_prob_me_end()	*(volatile uint32_t *)VPE = WAIT_ME_END
		#define vpe_prob_dt_start()	*(volatile uint32_t *)VPE = WAIT_DT_START
		#define vpe_prob_dt_end()	*(volatile uint32_t *)VPE = WAIT_DT_END
		#define vpe_wait_start()		*(volatile uint32_t *)VPE = WAIT_START
		#define vpe_wait_end()		*(volatile uint32_t *)VPE = WAIT_END
	#endif
	
	#ifdef FPGA
		#ifdef LINUX
			#define mVpe_PASS()	printk("VPE pass\n");
			#define mVpe_FAIL(idx, num) printk("VPE %d 0x%x fail\n", idx, num);
		#else
			#define mVpe_PASS()			\
			{							\
				printf("VPE pass\n");			\
				while (1);					\
			}
			#define mVpe_FAIL(idx, num) 		\
			{							\
				printf("VPE %d 0x%x fail\n",idx, num);			\
				while(1);					\
			}
		#endif
		#define mVpe_Indicator(num)
	#else
		#define mVpe_PASS()			\
		{							\
			*(volatile uint32_t *)VPE = 0x01234568;\
			*(volatile uint32_t *)VPE = 0x01234567;\
		}

		#define mVpe_FAIL(idx, num)			\
		{							\
			*(volatile uint32_t *)VPE = 0x01234569;\
			*(volatile uint32_t *)VPE = 0x01234567;\
		}

		#if defined(TWO_P_INT)
			#define mVpe_Indicator(num)		((MP4_t *)(MP4_OFF))->QCR2 = num;
		#else
			#define mVpe_Indicator(num)		*(volatile uint32_t *)VPE = num;
		#endif
	#endif


#endif /* __VPE_M_H  */
