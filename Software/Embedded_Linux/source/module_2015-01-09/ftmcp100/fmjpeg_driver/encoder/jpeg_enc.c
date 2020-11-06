#ifdef LINUX
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/hdreg.h>	/* HDIO_GETGEO */
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/pci.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#endif

#define JPEG_INTERNALS
#include "jenclib.h"
#include "jpeg_enc.h"
#include "../../fmcp.h"
#ifdef DMAWRP
#include "mp4_wrp_register.h"
#endif

#ifdef EVALUATION_PERFORMANCE
#include "../common/performance.h"
static struct timeval tv_init, tv_curr;
extern FRAME_TIME encframe;
extern TOTAL_TIME enctotal;
extern uint64 time_delta(struct timeval *start, struct timeval *stop);
extern void enc_performance_count(void);
extern void enc_performance_report(void);
extern void enc_performance_reset(void);
extern unsigned int get_counter(void);
#endif

#define CKR_RATIO 0
struct Compress_recon rinfo; 
//unsigned int *DMA_COMMAND_local;//0500 -- 0700


unsigned int BASE_ADDRESS;

void FJpegEncReset(void)
{
  	unsigned reg;
  	SET_RST_REG(BASE_ADDRESS, 1<<27)
  	do {
  		READ_RST_REG(BASE_ADDRESS, reg) 
  	} while( reg & (1<<27));
	return;  	
}

static int jc_updateroi(j_compress_ptr cinfo, FJPEG_ENC_FRAME * ptFrame)
{
	int temp;
	int allign_byte_h = cinfo->max_hs_fact*DCTSIZE;
	int allign_byte_v = cinfo->max_vs_fact*DCTSIZE;


	if (ptFrame->roi_x < 0)
		ptFrame->roi_x = cinfo->roi_x;
	if (ptFrame->roi_y < 0)
		ptFrame->roi_y = cinfo->roi_y;
	if (ptFrame->roi_w < 0)
		ptFrame->roi_w = cinfo->roi_w;
	if (ptFrame->roi_h < 0)
		ptFrame->roi_h = cinfo->roi_h;

	if (0 == ptFrame->roi_w)
		ptFrame->roi_w = cinfo->pub.image_width;
	if (0 == ptFrame->roi_h)
		ptFrame->roi_h = cinfo->image_height;
// 0: MP4 2D, 420p,
														// 1: sequencial 1D,
														// 2: H264 2D, 420p
														// 3: sequencial 1D 420, one case of u82D=1, (only support when DMAWRP is configured)
														// 4: sequencial 1D 422, (only support when DMAWRP is configured)
	switch (cinfo->pub.u82D) {
        case 1:
		case 3: // Raster scan 420
			allign_byte_h = 8;
			allign_byte_v = 2;
			break;
		case 4: // Raster scan 422
			allign_byte_h = 2;
			allign_byte_v = 1;
			break;
		default: // H2642D, MPEG42D
			break;
	}
	// roi_x
	temp = jround_up(ptFrame->roi_x, allign_byte_h);
	if (temp != ptFrame->roi_x) {
		printk ("[mje]error: roi_x %d should be align at %d\n", ptFrame->roi_x, allign_byte_h);
		return -1;
	}
	// roi_left_y
	temp = jround_up(ptFrame->roi_y, allign_byte_v);
	if (temp != ptFrame->roi_y) {
		printk ("[mje]error: roi_y %d should be align at %d\n", ptFrame->roi_y, allign_byte_v);
		return -1;
	}
	// roi_w
	temp = jround_up(ptFrame->roi_w, cinfo->max_hs_fact * DCTSIZE);
	if (temp != ptFrame->roi_w) {
		printk ("[mje]error: roi_w %d should be align at %d\n",
				ptFrame->roi_w, cinfo->max_hs_fact * DCTSIZE);
		return -1;
	}
	// roi_h
	temp = jround_up(ptFrame->roi_h, cinfo->max_vs_fact * DCTSIZE);
	if (temp != ptFrame->roi_h) {
		printk ("[mje]error: roi_h %d should be align at %d\n",
				ptFrame->roi_h, cinfo->max_vs_fact * DCTSIZE);
		return -1;
	}
	if ((ptFrame->roi_x + ptFrame->roi_w) > cinfo->pub.image_width) {
		printk ("Error roi_x (%d) + roi_w (%d) > image_width (%d)\n",
						ptFrame->roi_x, ptFrame->roi_w, cinfo->pub.image_width);
		return -1;
	}
	if ((ptFrame->roi_y + ptFrame->roi_h) > cinfo->image_height) {
		printk ("Error roi_y (%d) + roi_h (%d) > image_height (%d)\n",
						ptFrame->roi_y, ptFrame->roi_h, cinfo->image_height);
		return -1;
	}

	cinfo->roi_x = ptFrame->roi_x;
	cinfo->roi_y = ptFrame->roi_y;
	cinfo->roi_w = ptFrame->roi_w;
	cinfo->roi_h = ptFrame->roi_h;
	cinfo->pub.roi_left_mb_x = cinfo->roi_x / (cinfo->max_hs_fact * DCTSIZE);
	cinfo->pub.roi_left_mb_y = cinfo->roi_y / (cinfo->max_vs_fact * DCTSIZE);
	cinfo->pub.roi_right_mb_x = (cinfo->roi_x+cinfo->roi_w) / (cinfo->max_hs_fact*DCTSIZE);
	cinfo->pub.roi_right_mb_y = (cinfo->roi_y+cinfo->roi_h) / (cinfo->max_vs_fact * DCTSIZE);
	return 0;
}

static int jc_checkdim (j_compress_ptr cinfo)
{
	int ci;
	jpeg_component_info *compptr;
	JDIMENSION height_in_blocks;
	JDIMENSION width_in_blocks;


	/* Compute maximum sampling factors; check factor validity */
	cinfo->max_hs_fact = 1;
	cinfo->max_vs_fact = 1;
	for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components; ci++, compptr++) {
		cinfo->max_hs_fact = MAX(cinfo->max_hs_fact, compptr->hs_fact);
		cinfo->max_vs_fact = MAX(cinfo->max_vs_fact, compptr->vs_fact);
	}

	/* Compute dimensions of components */
	for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
			ci++, compptr++) {
		/* Fill in the correct component_index value; don't rely on application */
		compptr->component_index = ci;
		/* Size in DCT blocks */
		width_in_blocks = (JDIMENSION) jdiv_round_up(
				(long) cinfo->pub.image_width * (long) compptr->hs_fact,
				(long) (cinfo->max_hs_fact * DCTSIZE));
		height_in_blocks = (JDIMENSION) jdiv_round_up(
				(long) cinfo->image_height * (long) compptr->vs_fact,
				(long) (cinfo->max_vs_fact * DCTSIZE));
		// check
		if ((width_in_blocks*cinfo->max_hs_fact * DCTSIZE) !=
				(cinfo->pub.image_width * compptr->hs_fact)) {
			printk ("JE: Error sampling image_w=%d, h_factor=%d, mcu_w = %d\n",
				cinfo->pub.image_width, compptr->hs_fact, cinfo->max_hs_fact * DCTSIZE);
			return -1;
		}
		if ((height_in_blocks*cinfo->max_vs_fact * DCTSIZE) !=
				(cinfo->image_height * compptr->vs_fact)) {
			printk ("JE: Error sampling image_h=%d, v_factor=%d, mcu_h = %d\n",
				cinfo->image_height, compptr->vs_fact, cinfo->max_vs_fact * DCTSIZE);
			return -1;
		}
	}
	return 0;
}



static void jc_me_init(unsigned int base)
{
	unsigned int *ME_cmd_que = (unsigned int *) (ME_CMD_START_ADDR+ base);
  	// due to the limitation of HW, we must use two SAD command to get deviation
  	ME_cmd_que[0] = 0;
  	ME_cmd_que[1] = 0;
  	ME_cmd_que[2] = 0;
  	ME_cmd_que[3] = 0;
  	ME_cmd_que[4] = 0;
  	ME_cmd_que[5] = 0;
  	ME_cmd_que[6] = 0;
  	ME_cmd_que[7] = 0;
  	ME_cmd_que[8] = 0;
  	ME_cmd_que[9] = 0;
  	ME_cmd_que[10] = 0;
  	ME_cmd_que[11] = 0;
  	ME_cmd_que[12] = 0;
  	ME_cmd_que[13] = 0;
  	ME_cmd_que[14] = 0;
  	ME_cmd_que[15] = 0;
  	ME_cmd_que[16] = (0<<29) | (1<<25) | (0<<23);
  	ME_cmd_que[17] = (1<<29) | (1<<25) | (0<<23);
  	ME_cmd_que[18] = (2<<29) | (0<<28) | (0<<19) | (0<<12) | ((unsigned int) 0x8000>>2);
  	ME_cmd_que[19] = (2<<29) | (0<<28) | (1<<19) | (1<<12) | ((unsigned int) 0x8000>>2);
  	ME_cmd_que[20] = (6<<29) | (0<<28) | (0<<27) | ((unsigned int) 0x8000>>2);
  	ME_cmd_que[21] = (6<<29) | (1<<28) | (1<<27) | ((unsigned int) 0x8000>>2);
  
  	SET_CMDADDR(base, ME_CMD_START_ADDR)
  	SET_MEIADDR(base, ME_INTERPOLATION_ADDR)
}

extern void mcp100_switch(void * codec, ACTIVE_TYPE type);
static int FJpegEncinit(struct jpeg_compress_struct *cinfo, FJPEG_ENC_FRAME * ptFrame)
{
	unsigned int qarval;
	int i; 
	volatile MDMA *pmdma = MDMA1(cinfo->VirtBaseAddr);

	cinfo->quality = ptFrame->u32ImageQuality;
	jpeg_set_quality(cinfo);
	if (jc_updateroi( cinfo, ptFrame) < 0)
		return -1;
	if (cinfo->pub.u32MotionDetection)
		jc_me_init(cinfo->VirtBaseAddr);
	for(i = 0; i < 3; i++)
		cinfo->pub.YUVAddr[i] = (unsigned int)ptFrame->pu8YUVAddr[i];
	cinfo->pub.bs_phy = (unsigned int) ptFrame->pu8BitstreamAddr;

	SET_VLDCTL(cinfo->VirtBaseAddr, 1<<5) //stop autobuffer in case previous firmware program did not terminate normally
	SET_VADR(cinfo->VirtBaseAddr, (unsigned int )VLCOUT_START_ADDR)
	SET_ABADR(cinfo->VirtBaseAddr, cinfo->pub.bs_phy | 1)
	SET_PYDCR(cinfo->VirtBaseAddr, 0)
	SET_PUVDCR(cinfo->VirtBaseAddr, 0)

	qarval = (QCOEFF_BANK & 0xffff)<<16 | (QCOEFF_BANK & 0xffff);
	SET_QAR(cinfo->VirtBaseAddr, qarval)
	SET_DMA_GCR(cinfo->VirtBaseAddr, 0)
	SET_DMA_GSR(cinfo->VirtBaseAddr, 0)
	SET_VLDCTL(cinfo->VirtBaseAddr, 0x50);		//ABF start & change endian
	// reset bs length
	#if defined(SEQ)
		mSEQ_BS_LEN(BASE_ADDRESS) = 0;
	#endif
	#if !(defined(GM8120) || defined(GM8180) || defined(GM8185))
	pmdma->BsBufSz = ptFrame->bitstream_size; //mcp100_bs_length;
	#endif

	pmdma->GRPC = 0;
	pmdma->GRPS = 0;
	pmdma->Status = 1;
	mcp100_switch(NULL, TYPE_JPEG_ENCODER);
	return 0;
}

#ifdef DMAWRP
static void jc_dmawrp (j_compress_ptr cinfo, int input_type)
{
	jpeg_compress_struct_my * my = &cinfo->pub;
	volatile WRP_reg * wrpreg = (volatile WRP_reg *)cinfo->u32BaseAddr_wrapper;
	if ((input_type == 3) || (input_type == 4)) {
		wrpreg->aim_hfp = cinfo->roi_x >> 4;
		wrpreg->aim_hbp = (my->image_width >> 4) - my->roi_right_mb_x;
		wrpreg->Aim_width = cinfo->roi_w >> 4;
		wrpreg->Aim_height = cinfo->roi_h >> 4;
		if (input_type == 4) {	// Raster scan 422
			wrpreg->Vfmt = (0 << 8) |			// (ID_CHN_IMG << 8) |
										(1L << 6) |	// always 1
										(1L << 5) |	// encode mode
										5;			// packed 422 mode
			wrpreg->Vsmsa = (my->YUVAddr[0] + ((cinfo->roi_x + cinfo->roi_y * my->image_width) << 1)) >> 2;
		}
		else {							// 420
			wrpreg->Vfmt = (0 << 8) |		// (ID_CHN_IMG << 8) |
										(1L << 6) |	// always 1
										(1L << 5) |	// encode mode
										0;					// planar 420 mode
			wrpreg->Ysmsa = (my->YUVAddr[0]  + ((cinfo->roi_x 		) + (cinfo->roi_y * my->image_width		  ))) >> 2;
			wrpreg->Usmsa = (my->YUVAddr[1]  + ((cinfo->roi_x >> 1) + (cinfo->roi_y * my->image_width >> 2))) >> 2;
			wrpreg->Vsmsa = (my->YUVAddr[2]  + ((cinfo->roi_x >> 1) + (cinfo->roi_y * my->image_width >> 2))) >> 2;	 
		}
		wrpreg->pf_go = 0x01;// DMA wrapper go
	}
	else				 // MP4_2D/H264_2D
		wrpreg->Vfmt = (1L << 7);				// bypass mode
}
#endif

#ifdef TWO_P_EXT
	#ifdef SEQ
		extern void compress_YUVsTri (j_compress_ptr cinfo);
		extern unsigned int compress_YUVsEnd (j_compress_ptr cinfo);
	#else
		extern void compress_YUVeTri (j_compress_ptr cinfo);
		extern unsigned int compress_YUVeEnd (j_compress_ptr cinfo);
	#endif
#endif
#define SHOW(str) printk(#str" = %d\n", str);
#define SHOW1t(str) printk("\t"#str" = %d\n", str);
#define SHOWx(str) printk(#str" = 0x%08x\n", (int)(str));
void FJpegEE_show (void *enc_handle)
{
	struct jpeg_compress_struct *cinfo = (struct jpeg_compress_struct *)enc_handle;
	jpeg_compress_struct_my * my = &cinfo->pub;
	jpeg_component_info * comp_info = cinfo->comp_info;
	int i;
	SHOW (my->image_width);
	SHOW (my->restart_interval);
	SHOW (my->comps_in_scan);
	SHOW (my->MCUs_per_row);
	SHOW (my->u82D);
	SHOW (my->roi_left_mb_x);
	SHOW (my->roi_left_mb_y);
	SHOW (my->roi_right_mb_x);
	SHOW (my->roi_right_mb_y);
	SHOW (my->sample);

	SHOW (cinfo->image_height);
	SHOW (cinfo->num_components);
	for (i = 0; i < cinfo->num_components; i ++) {
		if (comp_info) {
			SHOW1t (i);
			SHOW1t (comp_info->component_id);
			SHOW1t (comp_info->component_index);
			SHOW1t (comp_info->hs_fact);
			SHOW1t (comp_info->vs_fact);
			SHOW1t (comp_info->quant_tbl_no);
			SHOW1t (comp_info->dc_tbl_no);
			SHOW1t (comp_info->ac_tbl_no);
		}
		comp_info ++;
	}
	SHOW (cinfo->max_hs_fact);
	SHOW (cinfo->max_vs_fact);
	SHOW (cinfo->dev_flag);
	SHOW (cinfo->u32JPEGMode);
	SHOW (cinfo->qtbl_idx);
	SHOW (cinfo->htbl_idx);
	SHOW (cinfo->adc_idx);
	SHOW (cinfo->roi_enable);
	SHOW (cinfo->roi_x);
	SHOW (cinfo->roi_y);
	SHOW (cinfo->roi_w);
	SHOW (cinfo->roi_h);
	SHOW (cinfo->quality);
	SHOW (cinfo->qtbl_no);
	SHOW (cinfo->hufftbl_no);
	SHOWx (cinfo->luma_qtbl);
	SHOWx (cinfo->chroma_qtbl);
}
int FJpegEE_Tri(void *enc_handle, FJPEG_ENC_FRAME * ptFrame)
{
	struct jpeg_compress_struct *cinfo = (struct jpeg_compress_struct *)enc_handle;
   
	#ifdef EVALUATION_PERFORMANCE
		 do_gettimeofday(&tv_curr);  
		 if (  time_delta( &tv_init, &tv_curr) > TIME_INTERVAL ) {
			 enc_performance_report();
			 enc_performance_reset();
			 tv_init.tv_sec = tv_curr.tv_sec;
			 tv_init.tv_usec = tv_curr.tv_usec;
		 }
		 enctotal.count++; 
		 // encode start timestamp
		 encframe.ap_stop = get_counter();
		 enc_performance_count();
		 encframe.ap_start =	encframe.ap_stop;
		 encframe.start= encframe.ap_stop;
	#endif	

	if (FJpegEncinit(cinfo, ptFrame) < 0)
		return -1;
	if ( ptFrame->u8JPGPIC )
		jpeg_start_compress(cinfo, TRUE,TRUE);
	else
		jpeg_start_compress(cinfo, TRUE,FALSE);

 
	#ifdef DMAWRP
		jc_dmawrp (cinfo, cinfo->pub.u82D);
	#endif

	cinfo->dev_flag = 1;
	#if defined(SEQ)
	compress_YUVsTri(cinfo);
	#elif defined(TWO_P_EXT)
	compress_YUVeTri(cinfo);
	#endif
	return 0;
}

#ifndef SUPPORT_VG
//#if !defined(SUPPORT_VG)|defined(USE_WORKQUEUE)
extern unsigned int compress_YUV (jpeg_compress_struct_my *my, jpeg_component_info * jci[MAX_COMPS_IN_SCAN]);
#endif
int FJpegEE_End(void *enc_handle)
{
	struct jpeg_compress_struct *cinfo = (struct jpeg_compress_struct *)enc_handle;
	#ifdef EVALUATION_PERFORMANCE
		encframe.stop = get_counter();  
	#endif

	#if defined(SEQ)
		return compress_YUVsEnd(cinfo);
	#elif defined(TWO_P_EXT)
		return compress_YUVeEnd(cinfo);
	#else
		return compress_YUV(&cinfo->pub, cinfo->cur_comp_info);
	#endif
}

#ifndef SUPPORT_VG
#ifdef TWO_P_EXT
	extern void compress_YUV_block (j_compress_ptr cinfo);
#endif
unsigned int FJpegEncEncode(void *enc_handle, FJPEG_ENC_FRAME * ptFrame)
{
	if (FJpegEE_Tri (enc_handle, ptFrame) < 0)
		return 0;
	#ifdef TWO_P_EXT
		compress_YUV_block((struct jpeg_compress_struct *)enc_handle);
	#endif
	return FJpegEE_End(enc_handle);
}
#else

#endif	// SUPPORT_VG
unsigned int FJpegEncSetQTbls(void * enc_handle, unsigned int *luma, unsigned int * chroma)
{
    struct jpeg_compress_struct *cinfo = (struct jpeg_compress_struct *)enc_handle;
    int index;
    unsigned int *pluma;
    unsigned int *pchroma;

    pchroma = cinfo->chroma_qtbl;
    pluma = cinfo->luma_qtbl;
    
    for ( index=0; index < DCTSIZE2; index++)
        *pchroma++ = chroma[index];
    for ( index=0; index < DCTSIZE2; index++)
        *pluma++ = luma[index];
    cinfo->qtbl_no = 3;
    return 0;
}

extern const unsigned int luma_qtbl_1[DCTSIZE2];
extern const unsigned int chroma_qtbl_1[DCTSIZE2];
extern const unsigned int luma_qtbl_2[DCTSIZE2];
extern const unsigned int chroma_qtbl_2[DCTSIZE2];
void
jpeg_get_quant_table ( const unsigned int *src_table,
		      unsigned int *des_table,
		      int scaler)
{
  int i,j;
  long temp;

  for(i=0;i<8;i++){
	  for(j=0;j<8;j++){
		temp = ((long) *src_table++ * scaler + 50L) / 100L;
		/* limit the values to the valid range */
		if (temp <= 0L) temp = 1L;
		if (temp > 255L) temp = 255L;		/* limit to baseline range if requested */
		*des_table++ = (unsigned int) temp;
	  }
  }
}


unsigned int 
FJpegEncGetQTbls(
    void *enc_handle,
    FJPEG_ENC_QTBLS *qtbl)
{
	struct jpeg_compress_struct *cinfo = (struct jpeg_compress_struct *)enc_handle;
    int quality;
    const unsigned int *pluma;
    const unsigned int *pchroma;

   	/* Safety limit on quality factor.  Convert 0 to 1 to avoid zero divide. */
	quality = cinfo->quality;
  	if (cinfo->quality <= 0) quality = 1;
  	if (cinfo->quality > 100) quality = 100;
     
  	/* The basic table is used as-is (scaling 100) for a quality of 50.
   	 * Qualities 50..100 are converted to scaling percentage 200 - 2*Q;
   	 * note that at Q=100 the scaling is 0, which will cause jpeg_add_quant_table
   	 * to make all the table entries 1 (hence, minimum quantization loss).
   	 * Qualities 1..50 are converted to scaling percentage 5000/Q.
   	 */
  	if (cinfo->quality < 50)
    	quality = 5000 / quality;
  	else
    	quality = 200 - quality*2;

	switch (cinfo->qtbl_no){
        case 1:
	     pluma = luma_qtbl_1;
	     pchroma = chroma_qtbl_1; 
        break;
        case 2:
	     pluma = luma_qtbl_2;
	     pchroma= chroma_qtbl_2; 
        break;
        case 3: //user defined table
        default:
         pluma= cinfo->luma_qtbl;
	     pchroma = cinfo->chroma_qtbl; 
        break;
    }
	jpeg_get_quant_table(pchroma, qtbl->chroma_qtbl, quality);
	jpeg_get_quant_table(pluma, qtbl->luma_qtbl, quality);
	
	return 1;
}


void FJpegEncDestroy(void *enc_handle)
{

 	struct jpeg_compress_struct *cinfo = (struct jpeg_compress_struct *)enc_handle;
	int tblno;

	
	if(NULL == cinfo)
		return;
	if(cinfo->dev_buffer_virt)
		cinfo->pfnDmaFree(cinfo->dev_buffer_virt, cinfo->pub.dev_buffer_phy);
	if ( cinfo->chroma_qtbl )
        cinfo->pfnFree(cinfo->chroma_qtbl);
    if ( cinfo->luma_qtbl )
        cinfo->pfnFree(cinfo->luma_qtbl);
    
	for(tblno=0; tblno<NUM_HUFF_TBLS; tblno++) {
		if (cinfo->ac_huff_tbl_ptrs[tblno] )
			cinfo->pfnFree(cinfo->ac_huff_tbl_ptrs[tblno]);
		if (cinfo->dc_huff_tbl_ptrs[tblno] )
			cinfo->pfnFree(cinfo->dc_huff_tbl_ptrs[tblno]);
  	}	

	for(tblno=0; tblno<cinfo->adc_idx; tblno++) {
		if (cinfo->adctbl[tblno] )
			cinfo->pfnFree(cinfo->adctbl[tblno]);
  	}	
	if(cinfo->comp_info)
		cinfo->pfnFree(cinfo->comp_info);
	cinfo->pfnFree(cinfo);
}

int FJpegEncCreate_fake(FJPEG_ENC_PARAM_MY * my, void * pthandler)
{
	struct jpeg_compress_struct *cinfo = (struct jpeg_compress_struct *)pthandler;

	if ( !cinfo) {
		printk("%s: null pointer\n", __FUNCTION__);
		return -1;
	}

	cinfo->pfnDmaMalloc = my->pfnDmaMalloc;
	cinfo->pfnDmaFree = my->pfnDmaFree;
	cinfo->pfnMalloc = my->pfnMalloc;
	cinfo->pfnFree = my->pfnFree;
	cinfo->u32CacheAlign = my->u32CacheAlign;
	cinfo->pub.u32MotionDetection = 0;
	cinfo->pub.u82D = my->pub.u82D;
	cinfo->pub.sample = my->pub.sample;
#ifdef DMAWRP
	if (cinfo->pub.u82D > 4) {
		printk("%s: no such u82D (%d) type\n", __FUNCTION__, cinfo->pub.u82D);
		return -1;
	}
	else if ((cinfo->pub.u82D > 2) && (cinfo->pub.sample != JCS_yuv420) ) {
		printk("%s: sample must be JCS_yuv420 when u82D equal to 1, but %d\n", __FUNCTION__, cinfo->pub.sample);
		return -1;
	}
#else
	if (cinfo->pub.u82D > 2) {
		printk("%s: no such u82D (%d) type\n", __FUNCTION__, cinfo->pub.u82D);
		return -1;
	}
#endif
	else if (cinfo->pub.u82D != 1) {
		if ((cinfo->pub.sample != JCS_yuv420) && (cinfo->pub.sample != JCS_yuv400)){
			printk("%s: u82D must be 1 when not-JCS_yuv420/400 (%d)\n", __FUNCTION__, cinfo->pub.sample);
			return -1;
		}
	}
	if (cinfo->pub.sample > JCS_yuv400) {
		printk("%s: NOT support such sampling %x\n", __FUNCTION__, cinfo->pub.sample);
		return -1;
	}
	cinfo->qtbl_idx =0;
	cinfo->dev_flag = 0;
	// default tables index
	cinfo->qtbl_no = 1;	
	cinfo->hufftbl_no = 1;
/*
	cinfo->chroma_qtbl = 0; // Tuba 20120321: fix bug: user define quantization table is allocated and it can not assign to NULL 
	cinfo->luma_qtbl = 0;
*/
	cinfo->pub.restart_interval = my->pub.u32RestartInterval;
	// ROI information
	if (my->pub.roi_x >= 0)
		cinfo->roi_x = my->pub.roi_x;
	if (my->pub.roi_y >= 0)
		cinfo->roi_y = my->pub.roi_y;
	cinfo->roi_w = my->pub.roi_w;
	cinfo->roi_h = my->pub.roi_h;

	cinfo->pub.image_width = my->pub.u32ImageWidth;
	cinfo->image_height = my->pub.u32ImageHeight;
    // Tuba 20130311 satrt: add crop size to encode varibale size
    if (my->pub.crop_width >= 16) {
        printk("jpeg crop width(%d) must be smaller than 16, now set to zero\n", my->pub.crop_width);
        cinfo->crop_width = 0;
    }
    else
        cinfo->crop_width = my->pub.crop_width;
    if (my->pub.crop_height >= 16) {
        printk("jpeg crop height(%d) must be smaller thane 16, now set to zero\n", my->pub.crop_height);
        cinfo->crop_height = 0;
    }
    else
        cinfo->crop_height = my->pub.crop_height;
    
    // Tuba 20130311 end
	BASE_ADDRESS=(unsigned int)my->pu32BaseAddr;
	cinfo->VirtBaseAddr=(unsigned int)my->pu32BaseAddr;
	#ifdef DMAWRP
		cinfo->u32BaseAddr_wrapper = (unsigned int)mcp100_dev->va_base_wrp;
	#endif
	jpeg_set_defaults(cinfo);
	if (jc_checkdim (cinfo) < 0)
		return -1;

	//ben mark: SET_VLDCTL(cinfo->VirtBaseAddr, 1<<5) //stop autobuffer in case previous firmware program did not terminate normally
	//ben mark: SET_PYDCR(cinfo->VirtBaseAddr, 0)
	//ben mark: SET_PUVDCR(cinfo->VirtBaseAddr, 0)
	SET_CKR(cinfo->VirtBaseAddr, CKR_RATIO)
	//ben mark: SET_BADR(cinfo->VirtBaseAddr, 0)
	//ben mark: SET_BALR(cinfo->VirtBaseAddr, 0)
	// set for motion detection
	SET_HOFFSET(cinfo->VirtBaseAddr, 0)
	return 0;
}

void * FJpegEncCreate(FJPEG_ENC_PARAM_MY * my)
{
	struct jpeg_compress_struct *cinfo;

	cinfo = (struct jpeg_compress_struct *) my->pfnMalloc( sizeof(struct jpeg_compress_struct ),
									my->u32CacheAlign, my->u32CacheAlign);
	if ( !cinfo) {
		printk ("jpeg_compress_struct allocate error\n");
		return NULL;
	}
	memset(cinfo, 0 , sizeof(struct jpeg_compress_struct ));
	cinfo->comp_info = (jpeg_component_info *)	my->pfnMalloc(MAX_COMPONENTS * sizeof(jpeg_component_info),
  									cinfo->u32CacheAlign, cinfo->u32CacheAlign); 	
	if ( !cinfo->comp_info) {
		printk ("can't allocate jpeg_component_info\n");
		goto md_dev_buffer_err;
	}
	cinfo->chroma_qtbl = (unsigned int *) my->pfnMalloc( sizeof(unsigned int)*DCTSIZE2,
									cinfo->u32CacheAlign, cinfo->u32CacheAlign);
	if(! cinfo->chroma_qtbl) {
		printk ("allocate user define chroma_qtbl fail\n");
		goto md_dev_buffer_err;
	}
	cinfo->luma_qtbl = (unsigned int *) my->pfnMalloc( sizeof(unsigned int)*DCTSIZE2,
									cinfo->u32CacheAlign, cinfo->u32CacheAlign);
	if(! cinfo->luma_qtbl) {
		printk ("allocate user define luma_qtbl fail\n");
		goto md_dev_buffer_err;
	}
	if (FJpegEncCreate_fake(my, (void *)cinfo) < 0)
		goto md_dev_buffer_err;
#if 0
	if (cinfo->pub.u32MotionDetection) {
		unsigned int mb_width, mb_height;
		mb_width = cinfo->pub.image_width /(cinfo->comp_info->hs_fact*8);
		mb_height= cinfo->image_height /(cinfo->comp_info->vs_fact*8);	
  		cinfo->dev_buffer_virt =(void *) cinfo->pfnDmaMalloc(
					mb_width*mb_height*sizeof(cinfo->dev_buffer_virt), 
					cinfo->u32CacheAlign, cinfo->u32CacheAlign,
					(void *)(&cinfo->pub.dev_buffer_phy) );
		if (cinfo->dev_buffer_virt == NULL) {
			F_DEBUG("can't alloc deviation buffer width %d height %d comp %d\n", 
					cinfo->pub.image_width, cinfo->image_height,
					cinfo->num_components);
			goto md_dev_buffer_err;
		}
	}
#endif
	return (void *)cinfo;

md_dev_buffer_err:
	FJpegEncDestroy((void *)cinfo);
	return NULL;
}

unsigned int *FMjpegEncGetMBInfo(void *enc_handle)
{
	struct jpeg_compress_struct *cinfo = (struct jpeg_compress_struct *) enc_handle;
	
	return (unsigned int *) (cinfo->dev_buffer_virt);
}
