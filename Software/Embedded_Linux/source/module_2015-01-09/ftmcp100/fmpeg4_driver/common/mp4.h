#ifndef __MP4_H
	#define __MP4_H

///////////////////////////////////////////////////////////////////////////////////
//  The various kind of flags and configurations set by user.
//  User should set the following flags for different hardware platforms and
//  software features.
///////////////////////////////////////////////////////////////////////////////////
//	#define GM8120
//	#define GM8180
//	#define GM8180_FUTURE_VERSION	// this is for the future hardware version after GM8180, because 
                                    // the deblock_done sync signal will be added in future hardware
	
	// This flag was used for encoder and to generate the VisualObjectSequence syntax element in
	// header in order to comply with the ISO/IEC 14496-2 standard. This flag can be enabled depending
	// on customer's request.
//	#define GENERATE_VISUAL_OBJECT_SEQUENCE_SYNTAX
    // This flag was used for decoder and used to enable the MPEG4 decoder deblocking function 
    // which only exists in GM8180 and GM8180_FUTURE_VERSION hardware core.
//    #define ENABLE_DEBLOCKING  // enable this flag only for GM8180 and GM8180_FUTURE_VERSION
    
    // a flag to fix the resync bug, remove this flag after the bug was fixed
    #define FIX_RESYNC_BUG
    
   	// This flag was used just for experiment. It tries to optimize the GM8180 by utilizing the 
   	// ME command query function that FIE8160 provides.
	// But the GM8180 optimization did not work very well. Just keep this flag here for reference
	// and further improvement. Just don't enable this flag since it was used for experiment.
//	#define GM8180_OPTIMIZATION

    // This flag was used for our special de-interlace purpose. That's because our original MPEG4
    // encoder did not support interlaced coding, and in order to support interlaced coding, so we
    // directly incorporate two fields into one frame (even field in upper half of image and odd field
    // in lower half of image) during encoding. And after we receive this frame,we will use known
    // deinterlace algorithm to perforam deinterlace function on PC. And accordingly,when this
    // bitstream was saved on PC, our decoder should be able to play such bitstream. So we modify
    // the MPEG4 decoder and make it capable of playing such interlaced frame.
    // We define this flag to tell the difference.
    // Don't enable the 'ENABLE_DEBLOCKING' when this flag is turned on.
    // And this flag is used only for GM8120
    // And two limitations are required for this deinterlace feature :
    //   - The number of macroblock rows must be even number.
    //   - The image width and height must be equal to the display width and height.
    //#define ENABLE_DEINTERLACE

    
///////////////////////////////////////////////////////////////////////////////////
    
    #ifdef ENABLE_DEBLOCKING
      #ifdef GM8180_FUTURE_VERSION
        //  In GM8180 future version, a new sync signal (deblock_done) will be added
        //  to DMA Group Sync Register and allow DMA to synchronize with deblock_done signal.
        //  This deblock_done sync signal is shared with original VLC_done sync signal
        //  in DMA Group Sync Register. The deblock_done sync signal is enabled in decoding
        //  mode and VLC_done sync signal is enabled in encoding mode.
        #define ENABLE_DEBLOCKING_SYNC_DEBLOCKING_DONE 
      #else // #ifdef GM8180_FUTURE_VERSION
        //  workaround for the bug of buffer contention between DMA Engine (while storing deblocking data from the buffer) 
        //  and Deblocking Engine (while doing deblocking in the same buffer). Because GM8180
        //  did not have any deblock_done sync signal in DMA Group Sync Register, so we 
        //  arrange DMA commands of storing and loading deblocking data as the last 
        //  DMA entries in order to avoid the buffer contention caused by DMA and Deblocking 
        //  engine simultaneously and to reduce the probabilities of buffer contention.
        #define ENABLE_DEBLOCKING_DELAY_STORE_DEBLOCK
      #endif // #ifdef GM8180_FUTURE_VERSION
    #endif // #ifdef ENABLE_DEBLOCKING
    

	#ifndef TWO_P_INT
		#define Mp4Base(dec)		(dec->u32VirtualBaseAddr)
		#define Mp4EncBase(enc)		(enc->u32VirtualBaseAddr)
		#define DECODER			DECODER_ext
	#else
		#define Mp4Base(dec)		0
		#define Mp4EncBase(enc)		0
		#define DECODER			DECODER_int
	#endif

	typedef struct MP4Tag
	{										// ahb offset,	CRn,	CRm
		uint32_t MECTL;			// @00,		0,	0
		uint32_t MEPMV;			// @04,		0,	1	// no this register
		uint32_t MECR;				// @08,		0,	2
		uint32_t MIN_SADMV;	// @0C,		0,	3
		uint32_t CMDADDR;		// @10,		0,	4	0x200 boundary
		uint32_t MECADDR;		// @14,		0,	5	0x10 boundary
		uint32_t HOFFSET;		// @18,		0,	6	0x10 boundary
		uint32_t MCCTL;			// @1C,		0,	7
		// MCCADDR ==> encoder: 4B boundary, decoder: 0x80 boundary
		uint32_t MCCADDR;		// @20,		0,	8
		uint32_t MEIADDR;		// @24,		0,	9	0x08 boundary
		uint32_t CPSTS;			// @28,		0,	10
		uint32_t QCR0;				// @2C,		0,	11
		uint32_t QCR1;				// @30,		0,	12
		uint32_t QCR2;				// @34,		0,	13
		uint32_t QAR;				// @38,		0,	14	0x100 boundary
		uint32_t CKR;				// @3C,		0,	15

		uint32_t ACDCPAR;		// @40,		1,	0	0x200 boundary
		uint32_t VADR;				// @44,		1,	1
		uint32_t CURDEV;			// @48,		1,	2
		uint32_t BITDATA;			// @4C,		1,	3
		uint32_t BITLEN;			// @50,		1,	4	0x04 boundary
		uint32_t MVXYD;			// @54,		1,	5
		uint32_t MCIADDR;		// @58,		1,	6	0x08 boundary
		uint32_t VLDCTL;			// @5C,		1,	7
		uint32_t VOP0;				// @60,		1,	8
		uint32_t VOP1;				// @64,		1,	9
		uint32_t SCODE;			// @68,		1,	10
		uint32_t RSMRK;			// @6C,		1,	11
		uint32_t TOADR;			// @70,		1,	12	16B boundary
		uint32_t VLDSTS;			// @74,		1,	13
		uint32_t ASADR;			// @78,		1,	14
		uint32_t INTEN;			// @7C,		1,	15
		uint32_t VOPDM;			// @80,		2,	0
		uint32_t PMVADDR;		// @84,		2,	1	1KB boundary
		uint32_t DTOFMT;			// @88,		2,	2	Image output format select
	} MP4_t;

	/////////////////////////////////////////////////
	// ME Control Reigster (MECTL)
	#define MECTL_DEBLOCK_PP1	BIT8
	#define MECTL_PXI_MBCNT_DIS	BIT6
	#define MECTL_SKIP_PXI		BIT5
	#define MECTL_VOPSTART		BIT4
	#define MECTL_VPKTSTART		BIT3
	#define mMECTL_RND1b(v)		((v) << 2)
	#define MECTL_PXI_1MV		BIT1
	#define MECTL_MEGO			BIT0

	/////////////////////////////////////////////////
	// MC Control Reigster (MCCTL)
	#define MCCTL_ENC_MODE		BIT24
	#define MCCTL_DT_BOT		BIT23
	#define MCCTL_DEBK_TOP		BIT22
	#define MCCTL_DEBK_LEFT		BIT21
	#define MCCTL_FRAME_DEBK	BIT20
	#define MCCTL_DEBK_GO	BIT19
	#define MCCTL_FRAME_DT	BIT18

	#define MCCTL_DTGO		BIT17
	#define MCCTL_MVZ		BIT16
	#define MCCTL_INTRA_F	BIT15
	#define MCCTL_SVH		BIT14
	#define MCCTL_ACDC_D	BIT13
	#define MCCTL_ACDC_T		BIT12
	#define MCCTL_ACDC_L		BIT11
	#define MCCTL_DMCGO		BIT10
	#define MCCTL_JPGM		BIT9
	#define mMCCTL_MP4M3b(v)	((v) << 6)
	#define MCCTL_DECGO		BIT5
	#define MCCTL_REMAP		BIT4
	#define MCCTL_MP4Q		BIT3
	#define MCCTL_DIS_ACP		BIT2
	#define MCCTL_INTRA		BIT1
	#define MCCTL_MCGO		BIT0

	/////////////////////////////////////////////////
	// VLD Control Register (VLDCTL)
	#define VLDCTL_ABF_LITTLE_E	BIT6
	#define VLDCTL_ABFSTOP		BIT5
#ifdef BS_IS_BIG_ENDIAN
	#define VLDCTL_ABFSTART	(BIT4 | BIT6)
#else
	#define VLDCTL_ABFSTART	BIT4
#endif
	#define mVLDCTL_CMD4b(v)	(v)

	/////////////////////////////////////////////////
	//		Interrupt control register (INTEN)
	// --------------------------------------------------------------------
	#define INTEN_CPU_STOP			BIT17
	#define INTEN_CPU_START			BIT18
	#define INTEN_CPU_RESET			BIT19
	#define INTEN_INT_TRIGGER		BIT20		// interrupt external CPU
	#define INTEN_CPU_HALT			BIT23

	#ifdef MP4_GLOBALS
		#define MP4_EXT
	#else
		#define MP4_EXT extern
	#endif

    #define ENABLE_VARIOUS_RESOLUTION

#endif /* __MP4_H  */

