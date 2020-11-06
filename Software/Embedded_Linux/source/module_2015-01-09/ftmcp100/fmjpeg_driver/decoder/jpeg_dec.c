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

#include "jpeg_dec.h"
#include "jdeclib.h"
#include "../../fmcp.h"
#include "../common/jinclude.h"


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

#if defined(SUPPORT_VG_422T)||defined(SUPPORT_VG_422P)
extern unsigned int jpg_max_width;
extern unsigned int jpg_max_height;
#endif

#ifdef LINUX
extern unsigned int LOAD_INTERCODE;
#endif
unsigned int ADDRESS_BASE;
unsigned int Stride_MCU;
static unsigned int JDS_format[] =
{
	0x00221111,		// 420 mode
	0x00412121,		// 422 mode 0
	0x00211111,		// 422 mode 1
	0x00313131,		// 444 mode 0
	0x00212121,		// 444 mode 1
	0x00111111	,		// 444 mode 2
#ifdef SEQ
	0xFFFFFFFF,			// 422 mode 2, not support
	0xFFFFFFFF,			// 422 mode 3, not support
	0xFFFFFFFF,			// 444 mode 3, not support
#else
	0x00222121,		// 422 mode 2
	0x00121111,		// 422 mode 3
	0x00121212,		// 444 mode 3
#endif
	0x00000011			// 400 mode
};
void FJpegDecReCreate(FJPEG_DEC_PARAM_MY * ptParam, void * dec)
{
	j_decompress_ptr dinfo = (j_decompress_ptr )dec;

	dinfo->pfnFree = ptParam->pfnFree;
	dinfo->pfnMalloc = ptParam->pfnMalloc;
	dinfo->u32CacheAlign = ptParam->u32CacheAlign;
	dinfo->output_width_set = ptParam->pub.frame_width;
	dinfo->output_height_set = ptParam->pub.frame_height;
	dinfo->buf_width_set = 0;		// set to default 0
	dinfo->buf_height_set = 0;		// set to default 0

	dinfo->pub.format = ptParam->pub.output_format; 
	ADDRESS_BASE = (unsigned int)ptParam->pu32BaseAddr;  
	dinfo->VirtBaseAddr = (unsigned int)ptParam->pu32BaseAddr;  
	dinfo->bs_phy = ptParam->bs_buffer_phy;
	dinfo->pub.bs_phy_end = ptParam->bs_buffer_phy + ptParam->bs_buffer_len;
	dinfo->bs_virt = ptParam->bs_buffer_virt;	
	dinfo->bs_len = ptParam->bs_buffer_len;
	{
		//dinfo->pub.pLDMA = (unsigned int *) (dinfo->VirtBaseAddr+DMA_COMMAND_LOCAL_ADDR);
		//ben mark: SET_QAR(ADDRESS_BASE, 0)
		//ben mark: SET_VLDCTL(ADDRESS_BASE, 1<<5) //stop autobuffer in case previous firmware program did not terminate normally
		//ben mark: SET_PYDCR(ADDRESS_BASE, 0)  // set JPEG Previous Y DC Value Register to zero
		//ben mark: SET_PUVDCR(ADDRESS_BASE, 0) // set JPEG Previous UV DC Value Register to zero
		SET_CKR(dinfo->VirtBaseAddr, CKR_RATIO) // set Coprocessor Clock Control Register
		//ben mark: SET_VADR(ADDRESS_BASE, VLD_AUTOBUFFER_ADDR)  // set VLD Data Address Register (in Local Memory)
		//ben mark: SET_BADR(ADDRESS_BASE, 0) // reset the Bitstream Access Data Regsiter
		//ben mark: SET_BALR(ADDRESS_BASE, 0) // reset the Bitstream Access Length Register 
		//ben mark: SET_ABADR(ADDRESS_BASE, (unsigned int)inbitstr) // set Auto-Buffer System Data Address for VLD read operation
		//ben mark: SET_VLDCTL(ADDRESS_BASE, 0x58)  //Autobuffer Start (1011000) , just enable the VLD autobuffer engine and enable endian swapping (bit 6)
		//ben mark: SET_MCCADDR(ADDRESS_BASE, 0)  // we set MCCADDR to zero on purpose 
		#if 0
		unsigned int vldsts_reg;
		//ben mark: 
		do {
			READ_VLDSTS(ADDRESS_BASE, vldsts_reg)
		} while(!(vldsts_reg&0x0800));
		#endif
	}
}

void FJpegDecParaA2(void *dec_handle, FJPEG_DEC_PARAM_A2* ptPA2)
{
	j_decompress_ptr dinfo=(j_decompress_ptr) dec_handle;

	dinfo->buf_width_set = ptPA2->buf_width;
	dinfo->buf_height_set = ptPA2->buf_height;
}

void *FJpegDecCreate(FJPEG_DEC_PARAM_MY * ptParam)
{
	j_decompress_ptr dinfo;

	dinfo = (j_decompress_ptr)ptParam->pfnMalloc(sizeof(struct jpeg_decompress_struct),
  								ptParam->u32CacheAlign, ptParam->u32CacheAlign);
	if(!dinfo) {
		F_DEBUG("dinfo allocate fail\n");
		goto dinfo_alloc_err;
	}
  	memset( dinfo, 0, sizeof(struct jpeg_decompress_struct));
	// entropy alloc
	dinfo->entropy= (huff_entropy_ptr) ptParam->pfnMalloc (sizeof(struct huff_entropy_decoder),
  									ptParam->u32CacheAlign, ptParam->u32CacheAlign);
	if( !dinfo->entropy) {
		F_DEBUG("entropy allocate fail\n");
		goto dinfo_alloc_err;
	}
  	memset( dinfo->entropy, 0, sizeof(struct huff_entropy_decoder));
	// component information alloc
	dinfo->comp_info = (jpeg_component_info *) ptParam->pfnMalloc (
														4 * sizeof(jpeg_component_info),
									ptParam->u32CacheAlign, ptParam->u32CacheAlign);
	if (dinfo->comp_info == NULL) {
		printk("comp_info allocate fail\n");
		goto dinfo_alloc_err;
	}

	FJpegDecReCreate(ptParam,(void *)dinfo);
	return (void *)dinfo;
 
dinfo_alloc_err:
	if (dinfo) {
		if (dinfo->comp_info) ptParam->pfnFree (dinfo->comp_info);
		if (dinfo->entropy) ptParam->pfnFree (dinfo->entropy);
		ptParam->pfnFree (dinfo);
	}
	return 0;	
}


void FJpegDecReset(void)
{
  	unsigned reg;
	int cnt;

	cnt=0;
  	SET_RST_REG(ADDRESS_BASE, 0x8000000)
  	do {
  		READ_RST_REG(ADDRESS_BASE, reg) 
		cnt++;
		if ( cnt > JPEG_SYNC_TIMEOUT )
			break;
  	} while( reg & (0x8000000));
  	if ( cnt > JPEG_SYNC_TIMEOUT )
		F_DEBUG("Reset Sync Timeout\n");
	return;
}
 #ifdef JPG_VLD_ERR_WORKARROUND
	#define PADD_BUF_LEN 8
	unsigned char padd_buf[PADD_BUF_LEN] = {0x6F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#else
	#define PADD_BUF_LEN 0
#endif
#ifdef SUPPORT_VG
void FJpegDec_assign_bs(void *dec_handle, char * bs_va, char * bs_pa, int bs_cur_sz)
{
	j_decompress_ptr dinfo=(j_decompress_ptr) dec_handle;

	#if !(defined(GM8120) || defined(GM8180) || defined(GM8185))
		volatile MDMA *pmdma = MDMA1(dinfo->VirtBaseAddr);
	#endif
	dinfo->bs_phy = (unsigned int)bs_pa;
	dinfo->bs_virt = bs_va;
	dinfo->pub.bs_phy_end = (unsigned int)bs_pa + bs_cur_sz;

	#ifdef JPG_VLD_ERR_WORKARROUND
		memcpy(dinfo->bs_virt + bs_cur_sz, padd_buf , PADD_BUF_LEN);
	#endif
	SET_ABADR(dinfo->VirtBaseAddr, dinfo->bs_phy) // set Auto-Buffer System Data Address for VLD read operation
	#if !(defined(GM8120) || defined(GM8180) || defined(GM8185))
		pmdma->BsBufSz = bs_cur_sz + PADD_BUF_LEN+0x1000;
		//printk ("FJpegDec_assign_bs leng = %d\n", bs_cur_sz);
	#endif
}
#else

extern void jd_copy_from_user(void* dst, void * src, int byte_sz);
extern void jd_sync2device(void * v_ptr, void *p_ptr, int byte_sz);
void FJpegDec_fill_bs(void *dec_handle, void * src_init, int bs_sz_init)
{
	j_decompress_ptr dinfo=(j_decompress_ptr) dec_handle;
	#if !(defined(GM8120) || defined(GM8180) || defined(GM8185))
		volatile MDMA *pmdma = MDMA1(dinfo->VirtBaseAddr);
	#endif
	int sz = dinfo->bs_len;
	int sz2hw = dinfo->bs_len;
	int add_sync = 0;
	// init user bitstream
	if (src_init) {
		dinfo->user_bs_virt = src_init;
		dinfo->user_bs_len = bs_sz_init;
	}
	// fill bs.
	if (dinfo->user_bs_len < sz) {
		if ((dinfo->user_bs_len+PADD_BUF_LEN) > sz) {
			sz2hw -= 64;			// reduce one unit(64 bytes) to hardware
			sz -= 64;
		}
		else {
			sz = dinfo->user_bs_len;
			#ifdef JPG_VLD_ERR_WORKARROUND
			memcpy(dinfo->bs_virt + sz, padd_buf , PADD_BUF_LEN);
			add_sync = PADD_BUF_LEN;
			#endif
		}
	}
	if (sz)
		jd_copy_from_user(dinfo->bs_virt, dinfo->user_bs_virt, sz);
	if (sz+add_sync)
		jd_sync2device((void *)dinfo->bs_virt, (void *)dinfo->bs_phy, sz + add_sync);
	//printk("fill bs 0x%x, len 0x%x, hw 0x%x\n", (int)dinfo->user_bs_virt, sz, sz2hw);
	dinfo->user_bs_len -= sz;
	dinfo->user_bs_virt += sz;
	SET_ABADR(dinfo->VirtBaseAddr, dinfo->bs_phy) // set Auto-Buffer System Data Address for VLD read operation
	#if !(defined(GM8120) || defined(GM8180) || defined(GM8185))
		pmdma->BsBufSz = sz2hw;	
	#endif
}

#endif

#define size(array) (sizeof(array)/sizeof(array[0]))
#if TUBA_LIMIT_TABLE_SIZE   // tuba 20110721_0: add limit to huffman tbale
extern int dj_setup_per_scan (j_decompress_ptr dinfo);
#else
extern void dj_setup_per_scan (j_decompress_ptr dinfo);
#endif
extern void default_decompress_parms (j_decompress_ptr dinfo);
int FJpegDecReadHeader(void *dec_handle, FJPEG_DEC_FRAME * ptFrame, unsigned int pass)
{
	int ci;
	jpeg_component_info *compptr;
	j_decompress_ptr dinfo=(j_decompress_ptr) dec_handle;
	unsigned int samp_encod=0;
	int ret;
	int i;
	//unsigned int * inbitstr;
	unsigned int vldsts_reg;
	volatile MDMA *pmdma = MDMA1(ADDRESS_BASE);

	// 0: CbYCrY default
	// 1: RGB 16 bpp [0 B G R]
	// 2: RGB 24 bpp [0 B G R] 
	// 3: RGB 16 bpp [B G R]
	SET_DTOFMT(ADDRESS_BASE, 0) // set 0 for CbYCrY DOTFM
	// we group the DMA commands type
	// group ID 0 : normal operation
	// group ID 1 : disable this DMA command
	// group ID 2 : sync to MC done
	pmdma->GRPC =(unsigned int) 0x00000038;
	pmdma->GRPS =(unsigned int) 0x00000020;

	SET_QAR(ADDRESS_BASE, 0)
	SET_PYDCR(ADDRESS_BASE, 0)  // set JPEG Previous Y DC Value Register to zero
	SET_PUVDCR(ADDRESS_BASE, 0) // set JPEG Previous UV DC Value Register to zero
	SET_BADR(ADDRESS_BASE, 0) // reset the Bitstream Access Data Regsiter
	SET_BALR(ADDRESS_BASE, 0) // reset the Bitstream Access Length Register 
	SET_MCCADDR(ADDRESS_BASE, 0)  // we set MCCADDR to zero on purpose 
	SET_CKR(ADDRESS_BASE, CKR_RATIO) // set Coprocessor Clock Control Register

	//ben mark: SET_QAR(ADDRESS_BASE, 0)
	//ben mark: SET_BADR(ADDRESS_BASE, 0) // reset the Bitstream Access Data Regsiter
	//ben mark: SET_BALR(ADDRESS_BASE, 0) // reset the Bitstream Access Length Register
	SET_VLDCTL(ADDRESS_BASE, 1<<5) //stop autobuffer in case previous firmware program did not terminate normally
	SET_VADR(ADDRESS_BASE, VLD_AUTOBUFFER_ADDR)  // set VLD Data Address Register (in Local Memory)
	//ben mark: SET_MCCADDR(ADDRESS_BASE, 0)  // we set MCCADDR to zero on purpose
	//ben mark: SET_CKR(ADDRESS_BASE, CKR_RATIO) // set Coprocessor Clock Control Register
	#if defined(SEQ)
		#define MCCTL_JPG ((1L<<9) | (1L<<1))
		SET_MCCTL(ADDRESS_BASE, MCCTL_JPG)
	#endif
	SET_VLDCTL(ADDRESS_BASE, 0x58)  //Autobuffer Start (1011000) , just enable the VLD autobuffer engine and enable endian swapping (bit 6)

	do {
		READ_VLDSTS(ADDRESS_BASE, vldsts_reg)
	} while(!(vldsts_reg&0x0800));

	dinfo->HeaderPass = pass;
	dinfo->saw_SOI = FALSE;
	dinfo->saw_SOF = FALSE;
	ret = jpeg_read_header(dinfo);
	switch (ret) {
		case JPEG_REACHED_SOS:
    		break;
  		case JPEG_REACHED_EOI:
  		case JPEG_SUSPENDED:
			ptFrame->err = JDE_BSERR;
			return -1;
  		case JPEG_SYSERR:
			ptFrame->err = JDE_SYSERR;
			return -1;
  	}

	for (ci = 0, compptr = dinfo->comp_info; ci < dinfo->num_components;ci++, compptr++)
		samp_encod = (samp_encod << 8) | (compptr->hs_fact << 4) | compptr->vs_fact;

	Stride_MCU = STRIDE_MCU;
	for (i = 0; i < size(JDS_format); i ++) {
		if (samp_encod == JDS_format[i]) {
			dinfo->pub.sample = i;
			break;
		}
	}
	if (i == size(JDS_format)) {
		printk ("Not support this JPEG sample %x\n", samp_encod);
		ptFrame->err = JDE_USPERR;
		return -1;
	}

	ptFrame->NumofComponents=dinfo->num_components;
	ptFrame->img_width=dinfo->image_width;
	ptFrame->img_height=dinfo->image_height;
	ptFrame->sample = dinfo->pub.sample;
	if (dinfo->pub.comps_in_scan < dinfo->num_components) {
		ptFrame->sample += 16;
#ifdef SEQ
		printk ("Not support non-interleave\n");
		ptFrame->err = JDE_USPERR;
		return -1;
#endif
	}
	ptFrame->err = JDE_OK;
//printk ("no %d, %d\n", dinfo->num_components, dinfo->pub.comps_in_scan);
	return 0;
}

void stop_jd(void *dec_handle)
{
	jpeg_finish_decompress ((j_decompress_ptr) dec_handle, 0);
}
#define align(val, alig) ((val+alig-1)/alig*alig)
extern int dj_initial_setup (j_decompress_ptr dinfo);
extern void mcp100_switch(void * codec, ACTIVE_TYPE type);
#ifdef SUPPORT_VG
extern int decompress_YUVsTri (j_decompress_ptr dinfo);
extern int decompress_YUVeTri (j_decompress_ptr dinfo);
#define SHOW(str) printk(#str" = %d\n", str);
#define SHOW1t(str) printk("\t"#str" = %d\n", str);
#define SHOWx(str) printk(#str" = 0x%08x\n", (int)(str));
void FJpegDD_show(void *dec_handle)
{
	j_decompress_ptr dinfo = (j_decompress_ptr) dec_handle;
	jpeg_decompress_struct_my *my = &dinfo->pub;
	int i;
	jpeg_component_info * comp_info = dinfo->comp_info;

	SHOW (my->crop_x);
	SHOW (my->crop_y);
	SHOW (my->crop_xend);
	SHOW (my->crop_yend);
	SHOW (my->sample);
	SHOW (my->format);
	SHOW (my->restart_interval);
	SHOW (my->comps_in_scan);
	SHOW (my->total_iMCU_rows);
	SHOW (my->MCUs_per_row);
	SHOW (my->max_hs_fact);
	SHOW (my->max_vs_fact);
	SHOW (my->output_width);
	SHOW (my->monitor);
	SHOW (my->monitor1);
	SHOW (dinfo->crop_mcu_x);
	SHOW (dinfo->crop_mcu_y);
	SHOW (dinfo->crop_mcu_xend);
	SHOW (dinfo->crop_mcu_yend);
	SHOW (dinfo->image_width);
	SHOW (dinfo->image_height);
	SHOW (dinfo->num_components);
	SHOW (dinfo->output_height);
	SHOW (dinfo->output_width_set);
	SHOW (dinfo->output_height_set);
	for (i = 0; i < dinfo->num_components; i ++) {
		if (comp_info) {
			SHOW1t (i);
			SHOW1t (comp_info->component_id);
			SHOW1t (comp_info->component_index);
			SHOW1t (comp_info->hs_fact);
			SHOW1t (comp_info->vs_fact);
			SHOW1t (comp_info->quant_tbl_no);
			SHOW1t (comp_info->dc_tbl_no);
			SHOW1t (comp_info->ac_tbl_no);
			SHOW1t (comp_info->width_in_blocks);
			SHOW1t (comp_info->height_in_blocks);
			SHOW1t (comp_info->out_width);
			SHOW1t (comp_info->crop_x);
			SHOW1t (comp_info->crop_y);
			SHOW1t (comp_info->crop_xend);
			SHOW1t (comp_info->crop_yend);
		}
		comp_info ++;
	}
	SHOW (dinfo->blocks_in_MCU);
	for (i = 0; i < D_MAX_BLOCKS_IN_MCU; i ++) {
		if (dinfo->MCU_membership[i])
			SHOWx (dinfo->MCU_membership[i]);
	}
	SHOW (dinfo->crop_x_set);
	SHOW (dinfo->crop_y_set);
	SHOW (dinfo->crop_w_set);
	SHOW (dinfo->crop_h_set);
	SHOW (dinfo->bs_len);
	SHOW (dinfo->user_bs_len);
	SHOW (dinfo->ci);

}

int FJpegDD_Tri(void *dec_handle, FJPEG_DEC_FRAME *ptFrame)
{
	j_decompress_ptr dinfo=(j_decompress_ptr) dec_handle;
	jpeg_decompress_struct_my *my= & dinfo->pub;
	int ci;
	int ret = -1;

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
	if (ptFrame) {
		if (dj_initial_setup(dinfo) < 0) {
        #ifdef SEQ
			return JDE_USPERR;
        #else
			return JDE_SETERR;
        #endif
		}
		default_decompress_parms(dinfo);
		for(ci=0; ci < 3; ci ++)
			my->outdata[ci] = ptFrame->pu8YUVAddr[ci];
		dinfo->ci = 0;
	}
	mcp100_switch(NULL, TYPE_JPEG_DECODER);

#if TUBA_LIMIT_TABLE_SIZE   // tuba 20110721_0: add limit to huffman tbale
	if (dj_setup_per_scan (dinfo) < 0)
        return JDE_SETERR;
#else
    dj_setup_per_scan (dinfo)
#endif
	ftmcp100_store_multilevel_huffman_table (dinfo);
	if (jd_dma_cmd_init(dinfo) < 0) {
		ret = JDE_SETERR;
		goto FJpegDD_Tri_leave;
	}
#if defined(SEQ)
	if (decompress_YUVsTri(dinfo) < 0) {
		ret = JDE_SYSERR;
		goto FJpegDD_Tri_leave;
	}
#elif defined(TWO_P_EXT)
	if (decompress_YUVeTri (dinfo) < 0){
		ret = JDE_SYSERR;
		goto FJpegDD_Tri_leave;
	}
#else
	decompress_YUV (&dinfo->pub, dinfo->cur_comp_info);
#endif
	return JDE_OK;

FJpegDD_Tri_leave:
	if (jpeg_finish_decompress(dinfo, (ret == 0)) < 0) {
		ret = JDE_BSERR;
	}
	return ret;
}
void FJpegDD_GetInfo(void *dec_handle, FJPEG_DEC_FRAME *ptFrame)
{
  	j_decompress_ptr dinfo=(j_decompress_ptr) dec_handle;
	ptFrame->NumofComponents=dinfo->num_components;
	ptFrame->img_width=dinfo->image_width;
	ptFrame->img_height=dinfo->image_height;
	ptFrame->sample = dinfo->pub.sample;
}
#ifdef SUPPORT_VG_422T
extern int Yuv2Cbycry (jpeg_decompress_struct_my * my, unsigned int in_y, unsigned int in_u, unsigned int in_v, unsigned int out_yuv);
int FJpegD_420to422(void *dec_handle, unsigned int in_y, unsigned int in_u, unsigned int in_v, unsigned int out_yuv )
{
	j_decompress_ptr dinfo=(j_decompress_ptr) dec_handle;
    if (dinfo->pub.total_iMCU_rows*8 > jpg_max_height || (dinfo->pub.crop_xend - dinfo->pub.crop_x)*16 > jpg_max_width) {
        printk("420 to 422 out of max resolution (%dx%d > %dx%d)\n", dinfo->pub.total_iMCU_rows*8, (dinfo->pub.crop_xend - dinfo->pub.crop_x)*16, jpg_max_width, jpg_max_height);
        return -1;
    }
	Yuv2Cbycry(&dinfo->pub, in_y, in_u, in_v, out_yuv);
    return 0;
}
#endif
#ifdef SUPPORT_VG_422P
extern int YuvPackedCbycry (jpeg_decompress_struct_my * my, unsigned int in_y, unsigned int in_u, unsigned int in_v, unsigned int out_yuv);
int FJpegD_422Packed(void * dec_handle,unsigned int in_y,unsigned int in_u,unsigned int in_v,unsigned int out_yuv)
{
    j_decompress_ptr dinfo=(j_decompress_ptr) dec_handle;
    if (dinfo->pub.total_iMCU_rows*8 > jpg_max_height || (dinfo->pub.crop_xend - dinfo->pub.crop_x)*16 > jpg_max_width) {
        printk("out of max resolution (%dx%d > %dx%d)\n", dinfo->pub.total_iMCU_rows*8, (dinfo->pub.crop_xend - dinfo->pub.crop_x)*16, jpg_max_width, jpg_max_height);
        return -1;
    }
    YuvPackedCbycry(&dinfo->pub, in_y, in_u, in_v, out_yuv);
    return 0;
}
#endif

extern int decompress_YUVsEnd (j_decompress_ptr dinfo);
extern int decompress_YUVeEnd (j_decompress_ptr dinfo);
int FJpegDD_End(void *dec_handle)
{
  	j_decompress_ptr dinfo=(j_decompress_ptr) dec_handle;
	jpeg_decompress_struct_my *my= & dinfo->pub;
	int ret = -1;

#if defined(SEQ)
	if (decompress_YUVsEnd(dinfo) < 0) {
		ret = JDE_SYSERR;
		goto FJpegDD_End_leave;
	}
#elif defined(TWO_P_EXT)
	if (decompress_YUVeEnd (dinfo) < 0){
		ret = JDE_SYSERR;
		goto FJpegDD_End_leave;
	}
#endif

	dinfo->ci += my->comps_in_scan;
	if (dinfo->ci < dinfo->num_components) {
		int hdr_ret;
#if defined(SEQ)
		ret = JDE_USPERR;
		goto FJpegDD_End_leave;
#endif
		//printk ("ci no %d, %d\n", ci, dinfo->num_components);
		// start another scan
		hdr_ret = jpeg_read_header(dinfo);
		switch (hdr_ret) {
			case JPEG_REACHED_SOS:
	    		break;
			case JPEG_REACHED_EOI:
	  		case JPEG_SUSPENDED:
				ret = JDE_BSERR;
				goto FJpegDD_End_leave;
  			case JPEG_SYSERR:
				ret = JDE_SYSERR;
				goto FJpegDD_End_leave;
	  	}
		ret = 1;
	}
	else
		ret = JDE_OK;	// done this picture

FJpegDD_End_leave:
	//printk ("ret = %d\n", ret);
	if (jpeg_finish_decompress(dinfo, (ret == 0)) < 0) {
		ret = JDE_BSERR;
	}
	return ret;
}
#else   // SUPPORT_VG
extern int decompress_YUVs (j_decompress_ptr dinfo);
extern int decompress_YUVe (j_decompress_ptr dinfo);
extern int decompress_YUV (jpeg_decompress_struct_my *my, jpeg_component_info * cci_simple[4]);
static unsigned int __inline BitstreamRead32Bits(unsigned int base)
{  
	unsigned int v;
	READ_BADR(base, v) 
	return v;
}


int FJpegDecDecode(void *dec_handle, FJPEG_DEC_FRAME *ptFrame)
{
  	j_decompress_ptr dinfo=(j_decompress_ptr) dec_handle;
	jpeg_decompress_struct_my *my= & dinfo->pub;
  	int ci;
	int ret = -1;

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

	if (dj_initial_setup(dinfo) < 0) {
#ifdef SEQ
		ptFrame->err = JDE_USPERR;
#else
		ptFrame->err = JDE_SETERR;
#endif
		return -1;
	}
	mcp100_switch(NULL, TYPE_JPEG_DECODER);
	default_decompress_parms(dinfo);

	for(ci=0; ci < 3; ci ++)
		my->outdata[ci] = ptFrame->pu8YUVAddr[ci];

	ci = 0;
	while (1) {
		dj_setup_per_scan (dinfo);
		ftmcp100_store_multilevel_huffman_table (dinfo);
  		if (jd_dma_cmd_init(dinfo) < 0) {
			ptFrame->err = JDE_SETERR;
			goto FJpegDec_leave;
  		}
		// we need to flush the D cache or the result will be wrong 
  		//FA526_CleanInvalidateDCacheAll();

		#if defined(SEQ)
			if (decompress_YUVs (dinfo) < 0) {
				ptFrame->err = JDE_SYSERR;
				goto FJpegDec_leave;
			}
		#elif defined(TWO_P_EXT)
			if (decompress_YUVe (dinfo) < 0){
				ptFrame->err = JDE_SYSERR;
				goto FJpegDec_leave;
			}
		#else
			decompress_YUV (&dinfo->pub, dinfo->cur_comp_info);
		#endif

		ci += my->comps_in_scan;
		if (ci < dinfo->num_components) {
			int hdr_ret;
        #if defined(SEQ)
			ptFrame->err = JDE_USPERR;
			goto FJpegDec_leave;
        #endif
			//printk ("ci no %d, %d\n", ci, dinfo->num_components);
			// start another scan
			hdr_ret = jpeg_read_header(dinfo);
			switch (hdr_ret) {
				case JPEG_REACHED_SOS:
		    		break;
  				case JPEG_REACHED_EOI:
		  		case JPEG_SUSPENDED:
					ptFrame->err = JDE_BSERR;
					goto FJpegDec_leave;
  				case JPEG_SYSERR:
					ptFrame->err = JDE_SYSERR;
					goto FJpegDec_leave;
		  	}
		}
		else
			break;
	}
	ret = 0;
FJpegDec_leave:
	if (jpeg_finish_decompress(dinfo, (ret == 0)) < 0) {
		ptFrame->err = JDE_BSERR;
		ret = -1;
	}
    // decode end timestamp
	#ifdef EVALUATION_PERFORMANCE
    	decframe.stop = get_counter();  
    #endif
	return ret;
}
#endif
void FJpegDecDestroy(void *dec_handle)
{ 
	j_decompress_ptr dinfo=(j_decompress_ptr) dec_handle;  

	if (dinfo) {
		jpeg_destroy_decompress(dinfo);
		if (dinfo->comp_info)
			dinfo->pfnFree(dinfo->comp_info);
		if (dinfo->entropy)
			dinfo->pfnFree(dinfo->entropy);
		dinfo->pfnFree( dinfo) ;
	}
}
void FJpegDecSetDisp(void *dec_handle, FJPEG_DEC_DISPLAY * ptDisp)
{
	j_decompress_ptr dinfo = (j_decompress_ptr) dec_handle;

	if (ptDisp->crop_x != 0xFFFFFFFF)
		dinfo->crop_x_set = ptDisp->crop_x;
	if (ptDisp->crop_y != 0xFFFFFFFF)
		dinfo->crop_y_set = ptDisp->crop_y;
	if (ptDisp->crop_w != 0xFFFFFFFF)
		dinfo->crop_w_set = ptDisp->crop_w;
	if (ptDisp->crop_h != 0xFFFFFFFF)
		dinfo->crop_h_set = ptDisp->crop_h;
	if (ptDisp->output_format != 0xFFFFFFFF)
		dinfo->pub.format= ptDisp->output_format;
}
