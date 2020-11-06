#define VPE_GLOBALS

#ifdef LINUX
#include "../common/portab.h"
#include "vpe.h"
#include "../common/vpe_m.h"
#include "../common/define.h"
#include "../common/dma_m.h"
#include "../common/mp4.h"
#include "decoder.h"
#include "Mp4Vdec.h"
#include "local_mem_d.h"
#else
#include "portab.h"
#include "vpe.h"
#include "vpe_m.h"
#include "define.h"
#include "dma_m.h"
#include "mp4.h"
#include "decoder.h"
#include "mp4vdec.h"
#include "local_mem_d.h"
#endif

void 
vVPE_OpenDecoderFile(void)
{
	// open mp4.decoder
	mVpe_Indicator(0x60000000);
}
void 
vVPE_CloseDecoderFile(void)
{
	// close mp4.decoder
	mVpe_Indicator(0x60000001);
}
void 
vVPE_SaveYuvSeq(void *pvDec)
{
	DECODER *dec;

    dec = (DECODER *)pvDec;
    
	//change to mp4.decoder
	mVpe_Indicator(0x60000002);
	// log raw data from refn, instead of cur.
	// y
	mVpe_Indicator(0x40000000 | (dec->width * dec->height));
	mVpe_Indicator(0x50000000 | (uint32_t)dec->refn.y_phy);
	// u
	mVpe_Indicator(0x40000000 | (dec->width * dec->height / 4));
	mVpe_Indicator(0x50000000 | (uint32_t)dec->refn.u_phy);
	// v
	mVpe_Indicator(0x40000000 | (dec->width * dec->height / 4));
	mVpe_Indicator(0x50000000 | (uint32_t)dec->refn.v_phy);
}

void 
vVPE_OpenEncoderFile(void)
{
	// open mp4.encoder
	mVpe_Indicator(0x60000004);
}
void 
vVPE_CloseEncoderFile(void)
{
	// close mp4.yuv
	mVpe_Indicator(0x60000005);
}

void 
vVPE_SaveBS(uint8_t * buf, int32_t len)
{
	//change to mp4.decoder
	mVpe_Indicator(0x60000006);
	mVpe_Indicator(0x40000000 | len);
	mVpe_Indicator(0x50000000 | (int32_t)buf);
}
