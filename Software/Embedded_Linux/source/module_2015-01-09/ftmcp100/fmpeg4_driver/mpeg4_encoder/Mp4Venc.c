/**
 *  @file Mp4Venc.c
 *  @brief The implementation file for Gm MPEG4 encoder API.
 *
 */
#ifdef LINUX
#include "../common/portab.h"
#include "ftmcp100.h"
#include "Mp4Venc.h"
#include "encoder.h"
#include "motion.h"
#else
#include "portab.h"
#include "ftmcp100.h"
#include "mp4venc.h"
#include "encoder.h"
#include "motion.h"
#endif

void * FMpeg4EncCreate(FMP4_ENC_PARAM_MY * ptParam)
{
	return encoder_create(ptParam);
}
int FMpeg4EncReCreate(void *enc_handle, FMP4_ENC_PARAM_MY * ptParam)
{
	return encoder_create_fake((Encoder_x *)enc_handle, ptParam);
}
int FMpeg4Enc_IFP(void *enc_handle, FMP4_ENC_PARAM * encp)
{
	return encoder_ifp((Encoder_x *)enc_handle, encp);
}

#ifndef SUPPORT_VG
int FMpeg4EncOneFrame(void *enc_handle, FMP4_ENC_FRAME * pframe, int vol)
{
	return encoder_encode((Encoder_x *)enc_handle, pframe, vol);
}
#endif

extern void ee_show(Encoder_x * pEnc_x);
void FMpeg4EE_show(void *enc_handle)
{
	return ee_show((Encoder_x *)enc_handle);
}

extern int ee_Tri(Encoder_x * pEnc_x, FMP4_ENC_FRAME * pFrame, int force_vol);
int FMpeg4EE_Tri(void *enc_handle, FMP4_ENC_FRAME * pframe)
{
	return ee_Tri((Encoder_x *)enc_handle, pframe, 1);
}

extern int ee_End(Encoder_x * pEnc_x);
int FMpeg4EE_End(void *enc_handle)
{
	return ee_End((Encoder_x *)enc_handle);
}

MACROBLOCK_INFO * FMpeg4EncGetMBInfo(void *enc_handle)
{
	Encoder_x * pEnc_x = (Encoder_x *)enc_handle;
	#if defined(TWO_P_EXT)
		return (MACROBLOCK_INFO *) (pEnc_x->Enc.current1->mbs_virt);
	#else
		return (MACROBLOCK_INFO *) (pEnc_x->Enc.current1->mbs);
	#endif
}
void FMpeg4EncDestroy(void *enc_handle)
{
	encoder_destroy((Encoder_x *) enc_handle);
}

int FMpeg4Encactivity(void *enc_handle, int region)
{
    Encoder_x   *pEnc_x = (Encoder_x *)enc_handle;
    Encoder     *pEnc = &(pEnc_x->Enc);
    int         *act;
    
    act = &(pEnc->current1->activity0); 
    return (act[region]);
}

int FMpeg4EncFrameCnt(void *enc_handle)
{
    Encoder_x   *pEnc_x = (Encoder_x *)enc_handle;
    Encoder     *pEnc = &(pEnc_x->Enc);

    return pEnc->frame_counter;
}

// Tuba 20111209_1 start
int FMpeg4EncHandleIntraOver(void *enc_handle, int intra)
{
    Encoder_x   *pEnc_x = (Encoder_x *)enc_handle;
    Encoder     *pEnc = &(pEnc_x->Enc);

    if (intra)
        pEnc->sucIntraOverNum = 0;
    else {  // P frame
        if (pEnc->disableModeDecision)  // disbale intra in P frame
            pEnc->disableModeDecision--;
        else {  // count the succesive P frame which intra mb number over threshold
            if (pEnc->intraMBNum > pEnc->mb_width*pEnc->mb_height*INTRA_MB_THD) {
                pEnc->sucIntraOverNum++;
                if (pEnc->sucIntraOverNum >= SUCCESSIVE_INTRA_OVER) {
                    // for intra & disable intra in P frame
                    pEnc->forceOverIntra = 1;
                    pEnc->disableModeDecision = SUCCESSIVE_DISABLE_INTRA;
                    pEnc->sucIntraOverNum = 0;
                }
            }
            else {
                pEnc->sucIntraOverNum = 0;
            }
        }
    }
    return pEnc->intraMBNum;
}
// Tuba 20111209_1 end

