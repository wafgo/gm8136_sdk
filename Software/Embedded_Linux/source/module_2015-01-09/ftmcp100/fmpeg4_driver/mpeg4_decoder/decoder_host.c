#define LOCAL_MEM_GLOBAL
#ifdef LINUX
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#endif
#include "Mp4Vdec.h"
#include "dma_b.h"
#include "../common/define.h"
#include "../common/portab.h"
#include "../common/dma.h"
#include "../common/mp4.h"
#include "../common/vpe_m.h"
#include "../common/dma_m.h"
#include "../common/image.h"
#include "me.h"
#include "bitstream_d.h"
#include "image_d.h"
#include "local_mem_d.h"
#include "mem_transfer1.h"
#include "ioctl_mp4d.h"
#include "fmcp_type.h"
#ifdef TWO_P_EXT
#include "decoder_ext.h"
#else
#include "decoder.h"
#endif

#ifdef EVALUATION_PERFORMANCE
#include "../common/performance.h"
static struct timeval tv_init, tv_curr;
extern FRAME_TIME decframe;
extern TOTAL_TIME dectotal;
extern uint64 time_delta(struct timeval *start, struct timeval *stop);
extern void dec_performance_count(void);
extern void dec_performance_report(void);
extern void dec_performance_reset(void);
extern unsigned int get_counter(void);
#endif

 extern void mcp100_switch(void * codec, ACTIVE_TYPE type);


 static __inline uint32_t log2bin(uint32_t value)
{
	/* Changed by Chenm001 */
	int n = 0;

	while (value) {
		value >>= 1;
		n++;
	}
	return n;
}
static __inline void mpeg4d_hw_init(DECODER_ext *dec)
{
	int32_t base = dec->u32VirtualBaseAddr;
	unsigned char output_fmt = dec->output_fmt;

	volatile MP4_t * ptMP4 = (MP4_t *)(base  + MP4_OFF);
	#ifdef TWO_P_EXT
		extern void decoder_em_init(uint32_t base);
		// reset command for em
		decoder_em_init (base);
	#endif
	
	ptMP4->CKR = 0;
	ptMP4->SCODE = VOP_START_CODE;
	ptMP4->TOADR = TABLE_OUTPUT_OFF;
	ptMP4->ACDCPAR = PREDICTOR0_OFF;
	ptMP4->PMVADDR = PMV_BUFFER_OFF;

    if (output_fmt < OUTPUT_FMT_YUV) {
  		#ifdef ENABLE_DEBLOCKING
			ptMP4->DTOFMT = (ptMP4->DTOFMT & ~0x03) | (unsigned int) output_fmt;
  		#else
			ptMP4->DTOFMT = (unsigned int) output_fmt;
  		#endif
    }
	me_dec_commandq_init(base);
	//mcp100_switch ((void *)dec, TYPE_MPEG4_DECODER);
}

static int mp4d_output_img_fmt = -1;
int decoder_create_fake(FMP4_DEC_PARAM_MY* ptParam, void * ptDecHandle)
{
	DECODER_ext *dec = (DECODER_ext *)ptDecHandle;

	if (dec == NULL) {
        printk("decoder_create_fake: dec is NULL\n");
		return -1;
	}
	switch (ptParam->pub.output_image_format) {
		case OUTPUT_FMT_YUV:
		case OUTPUT_FMT_CbYCrY:
			break;
		default:
			printk ("MPEG4 decoder output format unsupport (%d)\n", ptParam->pub.output_image_format);
			return -1;
	}
	if (mp4d_output_img_fmt == -1)
		mp4d_output_img_fmt = ptParam->pub.output_image_format;
	if (mp4d_output_img_fmt != ptParam->pub.output_image_format) {
		printk ("MPEG4 decoder output format conflict (%d != %d)\n",
			mp4d_output_img_fmt, ptParam->pub.output_image_format);
		return -1;
	}
	if ((dec->u32MaxWidth < ptParam->pub.u32MaxWidth) ||
	   (dec->u32MaxHeight < ptParam->pub.u32MaxHeight)) {
		printk ("MPEG4 decoder max w/h over the original one\n");
		return -1;
	}

	dec->u32VirtualBaseAddr = ptParam->u32VirtualBaseAddr;
	dec->u32BS_buf_sz = ptParam->u32BSBufSize;
	dec->output_fmt = ptParam->pub.output_image_format;

	dec->output_stride = ptParam->pub.u32FrameWidth;
	dec->output_height= ptParam->pub.u32FrameHeight;
	dec->u32CacheAlign = ptParam->u32CacheAlign;
   
	dec->pfnDmaMalloc = ptParam->pfnDmaMalloc;
	dec->pfnDmaFree = ptParam->pfnDmaFree;
	dec->pfnMalloc = ptParam->pfnMalloc;
	dec->pfnFree = ptParam->pfnFree;
	dec->pu8BS_start_virt = ptParam->pu8BitStream;
	dec->pu8BS_start_phy = ptParam->pu8BitStream_phy;
	dec->pu8BS_ptr_virt = dec->pu8BS_start_virt;
	dec->pu8BS_ptr_phy = dec->pu8BS_start_phy;
	dec->u32BS_buf_sz_remain = 0;
	dec->bBS_end_of_data = FALSE;


	dec->data_partitioned=0;
	dec->bH263 = 0;
	dec->reversible_vlc = 0;
	dec->interlacing = 0;
	dec->time_inc_bits = 0;
	//dec->low_delay = 0;

	dec->custom_intra_matrix =
	dec->custom_inter_matrix = 0;

	dec->output_base_ref_phy = NULL;

	dec->frames = -1;
	dec->time = dec->time_base = dec->last_time_base = 0;
	// for auto set used
	dec->width = 0;
	dec->height = 0;
	#ifdef ENABLE_DEINTERLACE
		dec->u32EnableDeinterlace = ptParam->pub.u32EnableDeinterlace;
	#endif

#ifdef MPEG4_COMMON_PRED_BUFFER
    dec->pu16ACDC_ptr_phy = ptParam->pu16ACDC_phy;
    dec->mbs = (MACROBLOCK *)ptParam->pu32Mbs_phy;
#endif
#ifdef REF_POOL_MANAGEMENT
    #ifndef REF_BUFFER_FLOW
    dec->cur.y_phy = ptParam->pu8ReConFrameCur_phy;
    dec->cur.u_phy = (uint8_t *)(((uint32_t)dec->cur.y_phy) + dec->u32MaxWidth * dec->u32MaxHeight);
    dec->cur.v_phy = (uint8_t *)(((uint32_t)dec->cur.u_phy) + dec->u32MaxWidth * dec->u32MaxHeight / 4);
    //dec->cur.y = ptParam->pu8ReConFrameCur;
    dec->refn.y_phy = ptParam->pu8ReConFrameRef_phy;
    dec->refn.u_phy = (uint8_t *)(((uint32_t)dec->refn.y_phy) + dec->u32MaxWidth * dec->u32MaxHeight);
    dec->refn.v_phy = (uint8_t *)(((uint32_t)dec->refn.u_phy) + dec->u32MaxWidth * dec->u32MaxHeight / 4);
    //dec->refn.y = ptParam->pu8ReConFrameRef;
    #endif
#endif

	mpeg4d_hw_init(dec);
	return 0;
}
int decoder_create(FMP4_DEC_PARAM_MY* ptParam, void ** pptDecHandle)
{
	DECODER_ext *dec = NULL;
	uint32_t max_mb_width;
	uint32_t max_mb_height;

	if ((ptParam->pfnMalloc == NULL) || (ptParam->pfnFree == NULL)) {
        printk("GM_ERR_NULL_FUNCTION2 ERR\n");
		goto decoder_create_err;
	}

	dec = ptParam->pfnMalloc(sizeof(DECODER), ptParam->u32CacheAlign, ptParam->u32CacheAlign);
	if (dec == NULL) {
        printk("GM_ERR_MEMORY2 ERR\n");
		goto decoder_create_err;
	}
    memset(dec,0,sizeof(DECODER));
	dec->u32MaxWidth = ptParam->pub.u32MaxWidth;
	dec->u32MaxHeight = ptParam->pub.u32MaxHeight;
	decoder_create_fake(ptParam, dec);	// never fail at this time
	max_mb_width = (dec->u32MaxWidth + 15) / 16;
	max_mb_height = (dec->u32MaxHeight + 15) / 16;
	if (dec->pfnDmaMalloc) {
		// initial AC/DC prediction buffer
		// 1 MB includes 4 blocks(Y2, Y3, U, V), 16 bytes for each block
    #ifndef MPEG4_COMMON_PRED_BUFFER
		dec->pu16ACDC_ptr_virt = dec->pfnDmaMalloc(
								max_mb_width * 4 * 16,
								dec->u32CacheAlign /*8*/,
								dec->u32CacheAlign,
								(void **)&dec->pu16ACDC_ptr_phy);
		if (dec->pu16ACDC_ptr_virt == NULL) {
            printk("Fail to allocate pu16ACDC_ptr_virt 0x%x\n", max_mb_width *4 *16);
			goto decoder_create_err;
		}
    #endif
		#ifdef ENABLE_DEBLOCKING		
		// deblocking temp buffer
		// 1 MB includes 4 blocks(Y2, Y3, U/2, V/2), 64 & 32 bytes for each block
		//			==> 192 bytes
		// to fit DMA Auto_increase , 192--> 256
		dec->pu8DeBlock_virt = dec->pfnDmaMalloc(
								 max_mb_width * 256,
								 dec->u32CacheAlign /*8*/,
								 dec->u32CacheAlign,
								 (void **)&dec->pu8DeBlock_phy);
        if (dec->pu8DeBlock_virt == NULL) {
            printk("Fail to allocate pu8DeBlock_virt 0x%x!\n",max_mb_width * 256);
			goto decoder_create_err;
		}
		#endif
    #ifndef REF_POOL_MANAGEMENT
		if (image_create(&dec->cur, max_mb_width, max_mb_height, dec)) {
	        printk("image_create cur ERR\n");
			goto decoder_create_err;
		}
		if (image_create(&dec->refn, max_mb_width, max_mb_height, dec)) {
	        printk("image_create ref ERR\n");
			goto decoder_create_err;
		}
    #endif
	}
	else {
		dec->pu8BS_start_virt = ptParam->pu8BitStream;
		dec->pu8BS_start_phy = ptParam->pu8BitStream_phy;
		#ifdef ENABLE_DEBLOCKING
			dec->pu8DeBlock_phy = ptParam->pu8DeBlock_phy;
			dec->pu8DeBlock_virt = ptParam->pu8DeBlock;
		#endif
		dec->pu16ACDC_ptr_phy = ptParam->pu16ACDC_phy;
		dec->pu16ACDC_ptr_virt = ptParam->pu16ACDC;
		//current reconstruct frame
		#ifndef REF_BUFFER_FLOW
		dec->cur.y_phy = 
			ptParam->pu8ReConFrameCur_phy;
		dec->cur.u_phy =
			ptParam->pu8ReConFrameCur_phy + max_mb_width * max_mb_height * SIZE_Y;
		dec->cur.v_phy =
			ptParam->pu8ReConFrameCur_phy + max_mb_width * max_mb_height * (SIZE_Y + SIZE_U);
		dec->cur.y = 
			ptParam->pu8ReConFrameCur;
		dec->cur.u =
			ptParam->pu8ReConFrameCur + max_mb_width * max_mb_height * SIZE_Y;
		dec->cur.v =
			ptParam->pu8ReConFrameCur + max_mb_width * max_mb_height * (SIZE_Y + SIZE_U);
		// reference reconstruct frame
		dec->refn.y_phy = 
			ptParam->pu8ReConFrameRef_phy;
		dec->refn.u_phy =
			ptParam->pu8ReConFrameRef_phy + max_mb_width * max_mb_height * SIZE_Y;
		dec->refn.v_phy =
			ptParam->pu8ReConFrameRef_phy + max_mb_width * max_mb_height * (SIZE_Y + SIZE_U);
		dec->refn.y = 
			ptParam->pu8ReConFrameRef;
		dec->refn.u =
			ptParam->pu8ReConFrameRef + max_mb_width * max_mb_height * SIZE_Y;
		dec->refn.v =
			ptParam->pu8ReConFrameRef + max_mb_width * max_mb_height * (SIZE_Y + SIZE_U);
        #endif
	}
#ifndef MPEG4_COMMON_PRED_BUFFER
	#if defined(TWO_P_EXT)
		if (dec->pfnDmaMalloc) {
			// allocate AC/DC prediction buffer
			// 1 MB includes 4 blocks(Y2, Y3, U, V), 16 bytes for each block
			dec->mbs_virt = dec->pfnDmaMalloc(
						//	max_mb_width * 64,    //fixed by ivan for mb size error
						sizeof(MACROBLOCK) * max_mb_width * max_mb_height ,
						dec->u32CacheAlign /*8*/,
						dec->u32CacheAlign,
						(void **)&dec->mbs);
			if (dec->mbs_virt == NULL) {
	            printk("Fail to allocate mbs_virt 0x%x!\n",sizeof(MACROBLOCK)*max_mb_width*max_mb_height);
				goto decoder_create_err;
			}
		}
		else {
			dec->mbs_virt = ptParam->pu32Mbs;
			dec->mbs = (MACROBLOCK *)ptParam->pu32Mbs_phy;
		}
	#else
		dec->mbs = dec->pfnMalloc(sizeof(MACROBLOCK) * max_mb_width * max_mb_height,
											dec->u32CacheAlign, dec->u32CacheAlign);
		if (dec->mbs == NULL) {
		    printk("mbs create ERR\n");
			goto decoder_create_err;
		}
	#endif
#endif
	*pptDecHandle = dec;
	return 0;

decoder_create_err:
	if (dec) {
		if (dec->pfnDmaMalloc) {
			#ifdef ENABLE_DEBLOCKING
				if (dec->pu8DeBlock_virt)
					dec->pfnDmaFree(dec->pu8DeBlock_virt, dec->pu8DeBlock_phy);
			#endif
			if (dec->pu8BS_start_virt)
				dec->pfnDmaFree(dec->pu8BS_start_virt, dec->pu8BS_ptr_phy);
        #ifndef MPEG4_COMMON_PRED_BUFFER
			if (dec->pu16ACDC_ptr_virt)
				dec->pfnDmaFree(dec->pu16ACDC_ptr_virt, dec->pu16ACDC_ptr_phy);
        #endif
        #ifndef REF_POOL_MANAGEMENT
			image_destroy(&dec->cur, max_mb_width, dec);
			image_destroy(&dec->refn, max_mb_width, dec);
        #endif
		}
		dec->pfnFree(dec);
	}
	*pptDecHandle = NULL;
	return -1;
}


void decoder_destroy(void * ptDecHandle)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;
#ifndef REF_POOL_MANAGEMENT
	uint32_t max_mb_width = (dec ->u32MaxWidth + 15) / 16;
#endif
	if (dec == NULL)
		return;

	if (dec->pfnDmaMalloc) {
    #ifndef MPEG4_COMMON_PRED_BUFFER
	    if((dec->pu16ACDC_ptr_virt!=0)&&(dec->pu16ACDC_ptr_phy!=0))
		    dec->pfnDmaFree(dec->pu16ACDC_ptr_virt, dec->pu16ACDC_ptr_phy);
    #endif
		#ifdef ENABLE_DEBLOCKING
			if((dec->pu8DeBlock_virt!=0)&&(dec->pu8DeBlock_phy!=0))
				dec->pfnDmaFree(dec->pu8DeBlock_virt, dec->pu8DeBlock_phy);
		#endif
    #ifndef REF_POOL_MANAGEMENT
		image_destroy(&dec->cur, max_mb_width, dec);
		image_destroy(&dec->refn, max_mb_width, dec);
    #endif
	}
#ifndef MPEG4_COMMON_PRED_BUFFER    
	#if defined(TWO_P_EXT)
		if(dec->mbs_virt)
			dec->pfnDmaFree(dec->mbs_virt,dec->mbs);
	#else
		if(dec->mbs)
			dec->pfnFree(dec->mbs);
	#endif
#endif
	dec->pfnFree(dec);
}


void decoder_reset(void * ptDecHandle)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;
  	volatile MDMA * ptDma = (MDMA *)(Mp4Base (dec) + DMA_OFF);

	ptDma->Control  |= 1<<27;

	while( ptDma->Control & (1<<27) );
}
static int dd_check_dim(DECODER_ext * dec)
{
	if ((dec->width > dec->u32MaxWidth) || (dec->height > dec->u32MaxHeight)) {
		printk("Fail width and height:(%d,%d) Real:(%d,%d)\n",dec->width,dec->height,dec->u32MaxWidth,dec->u32MaxHeight);
		return -1;
	}
	if (dec->output_stride == 0) dec->output_stride = dec->width;
	if (dec->output_height == 0) dec->output_height = dec->height;
#if 0
	if (dec->crop_w == 0) dec->crop_w = dec->width - dec->crop_x;
	if (dec->crop_h == 0) dec->crop_h = dec->height - dec->crop_y;
#else
    dec->crop_w = dec->width - dec->crop_x;
    dec->crop_h = dec->height - dec->crop_y;
#endif

	if ((dec->crop_x + dec->crop_w) > dec->width) {
		printk("Fail \"crop_x, crop_w, width\" ((%d + %d) > %d)\n", dec->crop_x,dec->crop_w, dec->width);
		return -1;
	}
	if ((dec->crop_y + dec->crop_h) > dec->height) {
		printk("Fail \"crop_y, crop_h, height\" ((%d + %d) > %d)\n", dec->crop_x,dec->crop_h, dec->height);
		return -1;
	}
	if (dec->crop_w > dec->output_stride) {
		printk("Fail \"crop_w, output_stride\" (%d > %d)\n", dec->crop_w, dec->output_stride);
		return -1;
	}
	else if (dec->crop_w <= 0) {
		printk("Fail \"crop_w\" (%d)\n", dec->crop_w);
		return -1;
	}
	if (dec->crop_h > dec->output_height) {
		printk("Fail \"crop_h, output_height\" (%d > %d)\n", dec->crop_h, dec->output_height);
		return -1;
	}
	else if (dec->crop_h <= 0) {
		printk("Fail \"crop_h\" (%d)\n", dec->crop_h);
		return -1;
	}
	return 0;
}
static void dd_oneframe_init(DECODER_ext * dec,
		uint32_t quant,
		uint32_t fcode_forward,
		uint32_t intra_dc_threshold_bit)
{
	MP4_t * ptMP4 = (MP4_t *)(Mp4Base (dec) + MP4_OFF);
	int pp, ii;

	if (dec->vop_type && (dec->bH263 == 0))
		pp = NUMBITS_VP_RESYNC_MARKER + fcode_forward-1;
	else
		pp = NUMBITS_VP_RESYNC_MARKER;
	ii = 1<< (32-pp);
	ptMP4->RSMRK = ii;
	// Disable auto-buffering if closed abnormally last time
//		for (; (ptMP4->VLDSTS & BIT10)==0; ) ;		/// wait for autobuffer clean
	ptMP4->VLDCTL = VLDCTL_ABFSTOP|mVLDCTL_CMD4b(0);
	ptMP4->ASADR = (uint32_t)dec->bs.tail_phy;		//bs_phy_address;
	ptMP4->VADR = RUN_LEVEL_OFF | dec->bs.pos;
	ptMP4->CMDADDR = ME_CMD_Q_OFF;
	ptMP4->VOPDM = (dec->mb_width << 20) | (dec->mb_height << 4);
	ptMP4->VOP0 = 
			(fcode_forward << 20) |
			(intra_dc_threshold_bit << 16) |
			(quant << 8) |
			(dec->vop_type);
	#ifndef GM8120
	 	ptMP4->MCCTL = dec->bH263 << 14;
	#endif
	if (dec->bH263)
		ptMP4->VOP1 = 
			(3 << 28) |						// start code length
			((pp - 16) << 24) |				// length of resync-marker
			((dec->time_inc_bits + 2) << 16) |	// length of vop_time_increment code
			(7 << 12) |						// length of macroblock_number code
			(dec->reversible_vlc << 1) |		// RVLC
			(dec->data_partitioned << 0);
	else
		ptMP4->VOP1 = 
			(3 << 28) |						// start code length
			((pp - 16) << 24) |				// length of resync-marker
			((dec->time_inc_bits + 2) << 16) |	// length of vop_time_increment code
											// length of macroblock_number code
				(log2bin(dec->mb_width *  dec->mb_height - 1) << 12) |
				(dec->reversible_vlc << 1) |		// RVLC
				(dec->data_partitioned << 0);
}
#define SHOW(str) printk(#str" = %d\n", str);
#define SHOW1t(str) printk("\t"#str" = %d\n", str);
#define SHOWx(str) printk(#str" = 0x%08x\n", (int)(str));
void dd_show(void * dec_handle)
{
	DECODER_ext * dec = (DECODER_ext *)dec_handle;

	SHOW (dec->bMp4_quant);
	SHOW (dec->bH263);
	SHOW (dec->data_partitioned);
	SHOW (dec->output_fmt);
	SHOW (dec->output_stride);
	SHOW (dec->output_height);
	SHOW (dec->width);
	SHOW (dec->height);
	SHOW (dec->mb_width);
	SHOW (dec->mb_height);
	SHOW (dec->crop_x);
	SHOW (dec->crop_y);
	SHOW (dec->crop_w);
	SHOW (dec->crop_h);
}
int dd_tri(void * ptDecHandle, FMP4_DEC_RESULT * ptResult)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;
	uint32_t rounding = 0;
	uint32_t quant = 0;
	uint32_t fcode_forward = 0;
	uint32_t fcode_backward = 0;
	uint32_t intra_dc_threshold_bit = 0;
	uint32_t vop_type;

	#ifdef EVALUATION_PERFORMANCE
		do_gettimeofday(&tv_curr);	
		if (  time_delta( &tv_init, &tv_curr) > TIME_INTERVAL ) {
			dec_performance_report();
			dec_performance_reset();
			tv_init.tv_sec = tv_curr.tv_sec;
			tv_init.tv_usec = tv_curr.tv_usec;
		}
		dectotal.count++;	
		// decode start timestamp
		decframe.ap_stop = get_counter();
		dec_performance_count();
		decframe.ap_start = decframe.ap_stop;
		decframe.start= decframe.ap_stop;
	#endif	
	
	memset(&dec->bs, 0, sizeof(Bitstream) );
	BitstreamInit(&dec->bs, dec->pu8BS_ptr_virt, dec->pu8BS_ptr_phy, dec->u32BS_buf_sz_remain);
	dec->u32BS_used_byte = 0;
	dec->frames++;

	vop_type = BitstreamReadHeaders(&dec->bs, dec, &rounding, &quant, &fcode_forward,
										&fcode_backward, &intra_dc_threshold_bit);
	if (vop_type < 0)
		return -1;
	dec->vop_type = vop_type;
	if (dd_check_dim (dec) < 0)
		return -1;
#ifdef REF_BUFFER_FLOW
    // set recon buffer
    dec->cur.y_phy = (uint8_t *)ptResult->pReconBuffer->ref_phy;
    dec->cur.u_phy = (uint8_t *)(((uint32_t)dec->cur.y_phy) + dec->u32MaxWidth * dec->u32MaxHeight);
    dec->cur.v_phy = (uint8_t *)(((uint32_t)dec->cur.u_phy) + dec->u32MaxWidth * dec->u32MaxHeight / 4);
#endif
	dec->mb_width = (dec->width + PIXEL_Y-1) / PIXEL_Y;
	dec->mb_height = (dec->height + PIXEL_Y-1) / PIXEL_Y;
	mcp100_switch ((void *)dec, TYPE_MPEG4_DECODER);

	BitstreamUpdatePos(&dec->bs, NULL, DONT_CARE);
	dd_oneframe_init(dec, quant, fcode_forward, intra_dc_threshold_bit);

	switch (vop_type) {
		#ifdef TWO_P_EXT
		case I_VOP:
			return decoder_iframe_tri((DECODER_ext *)dec, &dec->bs);
		case P_VOP:
			return decoder_pframe_tri((DECODER_ext *)dec, &dec->bs, rounding);
		case N_VOP:		// vop not coded
			return decoder_nframe_tri((DECODER_ext *)dec);
		#else
		case I_VOP:
			return decoder_iframe((DECODER_ext *)dec, &dec->bs);
		case P_VOP:
			return decoder_pframe((DECODER_ext *)dec, &dec->bs, rounding);
		case N_VOP:		// vop not coded
			return decoder_nframe((DECODER_ext *)dec);
		#endif
		default:
		    printk("Fail VOP type %d\n",vop_type);
			return -1;
	}
}
int dd_sync(void * ptDecHandle, FMP4_DEC_RESULT * ptResult)
{
	DECODER_ext * dec = (DECODER_ext *)ptDecHandle;
	MP4_t * ptMP4 = (MP4_t *)(Mp4Base (dec) + MP4_OFF);
	uint32_t bs_len;
	int ret = 0;

	#ifdef TWO_P_EXT
		if (decoder_frame_sync (dec, &dec->bs) < 0)
			return -1;
	#endif

	if (dec->vop_type == I_VOP || dec->vop_type == P_VOP) {
		BitstreamUpdatePos_phy (&dec->bs,
			(uint32_t *)(ptMP4->ASADR - 256 + (ptMP4->BITLEN & 0x3f) - 0xc),
			ptMP4->VADR & 0x1f);
		image_swap(&dec->cur, &dec->refn);
	}

	dec->output_base_ref_phy = dec->output_base_phy;
	if (dec->output_fmt == OUTPUT_FMT_YUV) {
		dec->output_base_u_ref_phy = dec->output_base_u_phy;
		dec->output_base_v_ref_phy = dec->output_base_v_phy;
	}
	bs_len = (BitstreamPos(&dec->bs) + 7 )/8;
		
	#ifndef LINUX
		dec->u32BS_buf_sz_remain -= bs_len;
	#else
		if(dec->u32BS_buf_sz_remain < bs_len) {
			printk("ERROR: Bit Stream Size Error!decode size %d > Input size %d !!\n",
				bs_len, dec->u32BS_buf_sz_remain );
			printk("VLDSTS err code 0x%x\n", ptMP4->VLDSTS & 0xF000 );
			ret = -1;
		}
        // Tuba 20141225: because vg add 5 to length
		//else if ((dec->u32BS_buf_sz_remain -bs_len) > 4) {  // Tuba 20110810_0: change threshold from 1 to 4 because AVI need 4-byte align
		else if ((dec->u32BS_buf_sz_remain -bs_len) > 5) {
			// in AVI file, one frame one decode
			printk("VLDSTS err code 0x%x\n", ptMP4->VLDSTS & 0xF000 );	
			printk("ERROR: Bit Stream Size Error!decode size %d Input size %d !!\n",
					bs_len, dec->u32BS_buf_sz_remain );
			ret = -1;
		}
		bs_len = dec->u32BS_buf_sz_remain;
		dec->u32BS_buf_sz_remain = 0;
	#endif
         
	dec->pu8BS_ptr_virt += bs_len;
	dec->pu8BS_ptr_phy += bs_len;
	
	ptResult->u32VopWidth = dec->width;
	ptResult->u32VopHeight = dec->height;
	ptResult->u32UsedBytes = dec->u32BS_used_byte + bs_len;
	ptResult->pu8FrameBaseAddr_phy = dec->output_base_phy;
	if ( dec->output_fmt == OUTPUT_FMT_YUV) {
		ptResult->pu8FrameBaseAddr_U_phy = dec->output_base_u_phy;
		ptResult->pu8FrameBaseAddr_V_phy = dec->output_base_v_phy;
	}

	#ifdef EVALUATION_PERFORMANCE
	    decframe.stop = get_counter();  
    #endif
		
	return ret;
}
#ifndef SUPPORT_VG
int decoder_decode(void * ptDecHandle, FMP4_DEC_RESULT * ptResult)
{
	if (dd_tri (ptDecHandle, ptResult) < 0)
		return -1;
	#ifdef TWO_P_EXT
		if (decoder_frame_block((DECODER_ext *)ptDecHandle) < 0)
			return -1;;
	#endif
	return dd_sync (ptDecHandle, ptResult);
}
#endif

typedef struct
{
	int ud_dma;				// update dma
	int32_t mb_width;
	int32_t output_stride;
	uint32_t * intra_matrix_fix1;
	uint32_t * inter_matrix_fix1;
} duplex_struct_mp4d;

static duplex_struct_mp4d tDuplex_mp4d = {1, -1, -1, NULL, NULL};

void switch2mp4d(void * codec, ACTIVE_TYPE curr)
{
	int ud_dma_each = 0;
	int ud_q = 0;
	DECODER_ext * dec = (DECODER_ext *)codec;
	if (curr != TYPE_MPEG4_DECODER) {
		switch ( curr) {
			case TYPE_JPEG_ENCODER:
			case TYPE_JPEG_DECODER:
				tDuplex_mp4d.ud_dma = 1;		// force update dma
				tDuplex_mp4d.mb_width = -1;	// force update dma_each
				// no break here
			default:
			//case TYPE_MP4_ENCODER:
				tDuplex_mp4d.intra_matrix_fix1 = NULL;	// force update quant.
				break;
		}
	}
	else {
		if ((dec->mb_width != tDuplex_mp4d.mb_width)||(dec->output_stride != tDuplex_mp4d.output_stride))
				ud_dma_each = 1;
		if ((dec->bMp4_quant) &&
			((tDuplex_mp4d.intra_matrix_fix1 != dec->u32intra_matrix_fix1) ||
			(tDuplex_mp4d.inter_matrix_fix1 != dec->u32inter_matrix_fix1)))
			ud_q = 1;
		if (tDuplex_mp4d.ud_dma) {
			dma_dec_commandq_init(dec);
			tDuplex_mp4d.ud_dma = 0;
		}
		if (ud_dma_each) {
			dma_dec_commandq_init_each(dec);
			tDuplex_mp4d.mb_width = dec->mb_width;
			tDuplex_mp4d.output_stride = dec->output_stride;
		}
		if (ud_q) {	// MPEG4_QUANT
			int i;
			uint32_t * ptr_d, * ptr_s;
			ptr_d = (uint32_t *)(dec->u32VirtualBaseAddr + INTRA_QUANT_TABLE_ADDR);
			ptr_s = &dec->u32intra_matrix_fix1[0];
			for (i = 0; i < 64; i ++)
				*(ptr_d ++) = *(ptr_s ++);
			ptr_d = (uint32_t *)(dec->u32VirtualBaseAddr + INTER_QUANT_TABLE_ADDR);
			ptr_s = &dec->u32inter_matrix_fix1[0];
			for (i = 0; i < 64; i ++)
				*(ptr_d ++) = *(ptr_s ++);
			tDuplex_mp4d.intra_matrix_fix1 = dec->u32intra_matrix_fix1;
			tDuplex_mp4d.inter_matrix_fix1 = dec->u32inter_matrix_fix1;
		}
	}
}

