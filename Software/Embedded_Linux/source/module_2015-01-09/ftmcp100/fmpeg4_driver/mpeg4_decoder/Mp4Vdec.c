
#define MP4VDEC_GLOBALS

#ifdef LINUX
#include <linux/blkdev.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
#include <linux/dma-mapping.h>
#endif
#endif
#include "../common/portab.h"
#include "Mp4Vdec.h"
#include "decoder.h"
#include "../common/mp4.h"
#include "bitstream_d.h"
#include "local_mem_d.h"
#include "me.h"

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
int Mp4VDec_Init(FMP4_DEC_PARAM_MY * ptParam, void ** pptDecHandle)
{
	return decoder_create(ptParam, pptDecHandle);
}
int Mp4VDec_ReInit(FMP4_DEC_PARAM_MY * ptParam, void * ptDecHandle)
{
	return decoder_create_fake(ptParam, ptDecHandle);
}
uint32_t Mp4VDec_QueryEmptyBuffer (void * ptDecHandle)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;
	return dec->u32BS_buf_sz - dec->u32BS_buf_sz_remain;
}
uint32_t Mp4VDec_QueryFilledBuffer (void * ptDecHandle)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;
	return dec->u32BS_buf_sz_remain;
}
#ifndef SUPPORT_VG
int Mp4VDec_FillBuffer(void * ptDecHandle, uint8_t * ptBuf, uint32_t u32BufSize)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;

	if (u32BufSize > (dec->u32BS_buf_sz - dec->u32BS_buf_sz_remain)) {
		#ifdef LINUX
			printk("Mp4VDec_FillBuffer ERROR: u32BufSize %d BS_buf %d\n", 
				u32BufSize, dec->u32BS_buf_sz - dec->u32BS_buf_sz_remain);
		#endif	
		return -1;
	}

	// ivan 2007.0528 -->>
	#ifdef LINUX
	if(dec->u32BS_buf_sz_remain==1) {
		dec->u32BS_buf_sz_remain=0;
		//printk("u32BufSize %x u32BS_buf_sz_remain %x\n", u32BufSize, dec->u32BS_buf_sz_remain);	  	  
	} 
	else if(dec->u32BS_buf_sz_remain > 1) {
		printk("Fail to decode frame! u32BufSize %x u32BS_buf_sz_remain %x\n", u32BufSize, dec->u32BS_buf_sz_remain);	  
	}
	#endif
	// ivan 2007.0528 --<<	 
	if ((dec->u32BS_buf_sz_remain) && (dec->pu8BS_start_virt != dec->pu8BS_ptr_virt))
		memcpy(dec->pu8BS_start_virt, dec->pu8BS_ptr_virt, dec->u32BS_buf_sz_remain);
	
	if (u32BufSize) {
		#ifdef LINUX
			if (copy_from_user((void *)(dec->pu8BS_start_virt+dec->u32BS_buf_sz_remain), (void *)ptBuf, u32BufSize)) {
				printk("Error copy_to_user()\n");
				return -1;
			}
		#else
			memcpy(dec->pu8BS_start_virt + dec->u32BS_buf_sz_remain, ptBuf, u32BufSize);
		#endif
	}

	dec->pu8BS_ptr_virt = dec->pu8BS_start_virt;
	dec->pu8BS_ptr_phy = dec->pu8BS_start_phy;
	dec->u32BS_buf_sz_remain += u32BufSize;
    
    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33))
        __cpuc_flush_dcache_area((void *)dec->pu8BS_start_virt, dec->u32BS_buf_sz_remain);
	#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24))
		dma_consistent_sync((unsigned long)dec->pu8BS_start_virt,
				(void __iomem *)dec->pu8BS_start_phy, dec->u32BS_buf_sz_remain, DMA_TO_DEVICE);
	#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
		consistent_sync(dec->pu8BS_start_virt, dec->u32BS_buf_sz_remain, DMA_TO_DEVICE);
	#endif

	return 0;
}
#endif
void Mp4VDec_AssignBS(void * ptDecHandle, uint8_t * ptBuf_virt, uint8_t * BS_phy, uint32_t u32BufSize)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;

	dec->pu8BS_ptr_virt = dec->pu8BS_start_virt = ptBuf_virt;
	dec->pu8BS_ptr_phy = dec->pu8BS_start_phy = BS_phy;
	dec->u32BS_buf_sz_remain = u32BufSize;
}
void Mp4VDec_InvalidBS (void * ptDecHandle)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;

	dec->pu8BS_ptr_virt = dec->pu8BS_start_virt;
	dec->pu8BS_ptr_phy = dec->pu8BS_start_phy;
	dec->u32BS_buf_sz_remain = 0;
}
void Mp4VDec_EndOfData (void * ptDecHandle)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;

	dec->bBS_end_of_data = TRUE;
}
void Mp4VDec_SetOutputAddr (void * ptDecHandle,
		uint8_t * pu8output_phy, uint8_t * pu8output_u_phy, uint8_t * pu8output_v_phy)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;

	dec->output_base_phy = pu8output_phy;
	dec->output_base_u_phy = pu8output_u_phy;
	dec->output_base_v_phy = pu8output_v_phy;
}
void Mp4VDec_SetCrop(void * ptDecHandle, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;

	dec->crop_x = ((x+15) >> 4) << 4;
	dec->crop_y = ((y+15) >> 4) << 4;
	dec->crop_w = ((w+15) >> 4) << 4;
	dec->crop_h = ((h+15) >> 4) << 4;
}
#ifndef SUPPORT_VG
int Mp4VDec_OneFrame(void * ptDecHandle, FMP4_DEC_RESULT * ptResult)
{
	 return decoder_decode(ptDecHandle, ptResult);
}
#endif
void Mp4VDec_Release(void * ptDecHandle)
{
	decoder_destroy(ptDecHandle);
}
// following functions is made to facilitate testing or simulation
/*
_VPE:  speed up for simulation
_TEST: make verification task more easily
*/
void Mp4VDec_AssignBS_VPE(void * ptDecHandle, uint8_t * ptBuf, uint32_t u32BufSize)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;

	dec->pu8BS_ptr_virt = dec->pu8BS_start_virt =
	dec->pu8BS_ptr_phy = dec->pu8BS_start_virt = ptBuf;
	dec->u32BS_buf_sz_remain = u32BufSize;
}
void Mp4VDec_Add_BS_VPE(void * ptDecHandle, uint32_t u32BufSize)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;

	dec->u32BS_buf_sz_remain += u32BufSize;
}
uint8_t * Mp4VDec_Got_BS_ptrv_VPE(void * ptDecHandle)
{
	return (((DECODER_ext *)ptDecHandle)->pu8BS_ptr_virt);
}
void Mp4VDec_AssignRefBase_VPE(void * ptDecHandle,
				uint8_t * y_phy, uint8_t * u_phy, uint8_t * v_phy)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;

	dec->refn.y_phy = y_phy;
	dec->refn.u_phy = u_phy;
	dec->refn.v_phy = v_phy;
}
uint8_t * Mp4VDec_GetRefY_TEST(void * ptDecHandle)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;

	return dec->refn.y_phy;
}
uint8_t * Mp4VDec_GetRefU_TEST(void * ptDecHandle)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;

	return dec->refn.u_phy;
}
uint8_t * Mp4VDec_GetRefV_TEST(void * ptDecHandle)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;

	return dec->refn.v_phy;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
