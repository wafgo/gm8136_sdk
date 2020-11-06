#ifndef _FTMCP_H_
#define _FTMCP_H_

#define CKR_RATIO 0
//#define VPE 		0x90180000
//#define RTL_DEBUG_OUT(v) *(unsigned int*)VPE=v;

#if defined(GM8120)
#define REG_OFFSET	0x10000
#define BANK_OFFSET	0x00000
#else
#define REG_OFFSET	0x20000
#define BANK_OFFSET	0x10000
#endif


#define MDMA1_OFFSET ( REG_OFFSET + 0x0400)
	
#define MECTL		(REG_OFFSET + 0x0000)
#define MCUBR	    	(REG_OFFSET + 0x0008)
#define MIN_SADMV	(REG_OFFSET + 0x000c)
#define CMDADDR		(REG_OFFSET + 0x0010)
#define MECADDR		(REG_OFFSET + 0x0014)
#define HOFFSET     	(REG_OFFSET + 0x0018)
#define MCCTL       		(REG_OFFSET + 0x001c)
#define MCCADDR	    	(REG_OFFSET + 0x0020)
#define MEIADDR	    	(REG_OFFSET + 0x0024)
#define CPSTS	    	(REG_OFFSET + 0x0028)
#define MCUTIR      	(REG_OFFSET + 0x002c)
#define PYDCR       		(REG_OFFSET + 0x0030)
#define PUVDCR	    	(REG_OFFSET + 0x0034)
#define QAR         		(REG_OFFSET + 0x0038)
#define CKR         		(REG_OFFSET + 0x003c)
#define ACDCPAR		(REG_OFFSET + 0x0040)
#define VADR  		(REG_OFFSET + 0x0044)
#define CURDEV      	(REG_OFFSET + 0x0048)
#define BADR 			(REG_OFFSET + 0x004c)
#define BALR        		(REG_OFFSET + 0x0050)
#define VLDCTL	    	(REG_OFFSET + 0x005c)
#define MBIDX		(REG_OFFSET + 0x0054 )
#define MCIADDR     	(REG_OFFSET + 0x0058)
#define VOP0			(REG_OFFSET + 0x0060)
#define VOP1        		(REG_OFFSET + 0x0064)
#define MVD0        	(REG_OFFSET + 0x0068)
#define SCODE     		(REG_OFFSET + 0x0068)
#define VLASTWORD   	(REG_OFFSET + 0x006c)
#define TOADR       	(REG_OFFSET + 0x0070)
#define VLDSTS      	(REG_OFFSET + 0x0074)
#define ABADR       	(REG_OFFSET + 0x0078)
#define INCTL       	(REG_OFFSET + 0x007c)
#define DTOFMT		(REG_OFFSET + 0x0088)
#define EXT_MASK	(REG_OFFSET + 0x0094)	// ben
#define INT_FLAG		(REG_OFFSET + 0x0098)
#define INT_STS		(REG_OFFSET + 0x009C) // ben, before mask
#define SEQ_CTL		(REG_OFFSET + 0x00A0)	// ben
#define SEQ_FRM1	(REG_OFFSET + 0x00A4)	// ben
#define SEQ_FRM2	(REG_OFFSET + 0x00A8)	// ben
#define SEQ_FRM3	(REG_OFFSET + 0x00AC)	// ben
#define RST_REG		(REG_OFFSET + 0x040C)
#define DMA_GCR		(REG_OFFSET + 0x041C)
#define DMA_GSR		(REG_OFFSET + 0x0420)

//#define JPG_BS_LEN	(REG_OFFSET + 0x0430)	// ben
//#define SEQ_ROW_OFF_Y		(REG_OFFSET + 0x0434)// ben
//#define SEQ_ROW_OFF_U		(REG_OFFSET + 0x0438)// ben
//#define SEQ_ROW_OFF_V		(REG_OFFSET + 0x043C)// ben

#define MDMA1(base)			 	((volatile MDMA *)(MDMA1_OFFSET + base))
#define SET_MCUBR(base, v)		*(volatile unsigned long*)(MCUBR + base)=v;
#define SET_MECTL(base, v)		*(volatile unsigned long*)(MECTL + base)=v;
#define SET_MCCTL(base, v)		*(volatile unsigned long*)(MCCTL + base)=v;
#define SET_VOP0(base, v)		*(volatile unsigned long*)(VOP0 + base)=v;
#define SET_VOP1(base, v)		*(volatile unsigned long*)(VOP1 + base)=v;
#define SET_MCCADDR(base, v)	*(volatile unsigned long*)(MCCADDR + base)=v;	// DT dst. addr. must stay bank1
#define SET_MCIADDR(base, v)	*(volatile unsigned long*)(MCIADDR + base)=v;		// only valid at [11: 2]
																																// ref. MCCADDR[14] to point bank0/1
#define SET_CMDADDR(base, v)	*(volatile unsigned long*)(CMDADDR + base)=v;	//
#define SET_HOFFSET(base, v)	*(volatile unsigned long*)(HOFFSET + base)=v;		//
#define SET_MECADDR(base, v)	*(volatile unsigned long*)(MECADDR + base)=v;	// DT source addr. must stay bank1
#define SET_MEIADDR(base, v)	*(volatile unsigned long*)(MEIADDR + base)=v;		//
#define SET_CPSTS(base, v)		*(volatile unsigned long*)(CPSTS + base)=v;
#define SET_MCUTIR(base, v)		*(volatile unsigned long*)(MCUTIR + base)=v;
#define SET_PYDCR(base, v)		*(volatile unsigned long*)(PYDCR + base)=v;
#define SET_PUVDCR(base, v)		*(volatile unsigned long*)(PUVDCR + base)=v;
#define SET_QAR(base, v)		*(volatile unsigned long*)(QAR + base)=v;
#define SET_ACDCPAR(base, v)    *(volatile unsigned long*)(ACDCPAR + base)=v;
#define SET_CKR(base, v)		*(volatile unsigned long*)(CKR + base)=v;
#define SET_VADR(base, v)		*(volatile unsigned long*)(VADR + base)=v;
#define SET_BADR(base, v)		*(volatile unsigned long*)(BADR + base)=v;
#define SET_BALR(base, v)		*(volatile unsigned long*)(BALR + base)=v;
#define SET_VLDCTL(base, v)		*(volatile unsigned long*)(VLDCTL + base)=v;
#define SET_ABADR(base, v)		*(volatile unsigned long*)(ABADR + base)=v;    
#define SET_INCTL(base, v)		*(volatile unsigned long*)(INCTL + base)=v;
#define SET_SCODE(base, v)  	*(volatile unsigned long*)(SCODE + base)=v;
#define SET_DTOFMT(base,v)		*(volatile unsigned long*)(DTOFMT+ base)=v;
#define SET_INT_FLAG(base,v)	*(volatile unsigned long*)(INT_FLAG+ base)=v;


#define SET_RST_REG(base, v)	*(volatile unsigned long*)(RST_REG+ base)=v;
#define SET_DMA_GCR(base, v)	*(volatile unsigned long*)(DMA_GCR+ base)=v;
#define SET_DMA_GSR(base, v)	*(volatile unsigned long*)(DMA_GSR+ base)=v;

#define mSEQ_INTSTS(base)	    (*(volatile unsigned int*)(INT_STS+ base))	// ben
#define mSEQ_INTFLG(base)	    (*(volatile unsigned int*)(INT_FLAG+ base))	// ben
#define mSEQ_EXTMSK(base)	    (*(volatile unsigned int*)(EXT_MASK+ base))	// ben
#define mSEQ_CTL(base)	    (*(volatile unsigned int*)(SEQ_CTL+ base))	// ben
#define mSEQ_DIMEN(base)	(*(volatile unsigned int*)(SEQ_FRM1+ base))	// ben
#define mSEQ_RS_ITV(base)	(*(volatile unsigned int*)(SEQ_FRM2+ base))	// ben
#define mSEQ_BS_LEN(base)	(*(volatile unsigned int*)(SEQ_FRM3+ base))	// ben
//#define mSEQ_BSL(base)		(*(volatile unsigned int*)(JPG_BS_LEN+ base))	// ben
//#define mSEQ_OFFY(base)	(*(volatile unsigned int*)(SEQ_ROW_OFF_Y+ base))	// ben
//#define mSEQ_OFFU(base)	(*(volatile unsigned int*)(SEQ_ROW_OFF_U+ base))	// ben
//#define mSEQ_OFFV(base)	(*(volatile unsigned int*)(SEQ_ROW_OFF_V+ base))	// ben
//#define mSEQ_OFF(base, off)	(*(volatile unsigned int*)(SEQ_ROW_OFF_Y+off+ base))	// ben
#define mDTOFMT(base)		(*(volatile unsigned long*)(DTOFMT+ base))


#define READ_VADR(base, v)      v=*(volatile unsigned long*)(VADR + base);
#define READ_BADR(base, v)      v=*(volatile unsigned long*)(BADR + base);
#define READ_CURDEV(base, v)    v=*(volatile unsigned long*)(CURDEV + base);
#define READ_MIN_SADMV(base, v) v=*(volatile unsigned long*)(MIN_SADMV + base);
#define READ_CPSTS(base, v)     v=*(volatile unsigned long*)(CPSTS + base);
#define READ_BALR(base, v)      v=*(volatile unsigned long*)(BALR + base);		// write: bitlen, read: word in local
#define READ_VLASTWORD(base, v) v=*(volatile unsigned long*)(VLASTWORD + base);
#define READ_VLDSTS(base, v)    v=*(volatile unsigned long*)(VLDSTS + base);
#define READ_ABADR(base, v)     v=*(volatile unsigned long*)(ABADR + base);
#define READ_INCTL(base, v)     v=*(volatile unsigned long*)(INCTL + base);
#define READ_VLDCTL(base,v)		v=*(volatile unsigned long*)(VLDCTL + base);
#define READ_QAR(base,v)		v=*(volatile unsigned long*)(QAR + base);
#define READ_MCCTL(base, v)		v=*(volatile unsigned long*)(MCCTL + base);
#define READ_VOP0(base, v)  	v=*(volatile unsigned long*)(VOP0 + base);
//#define READ_INCTL(base, v)		v=*(volatile unsigned long*)(INCTL + base);
#define READ_INT_FLAG(base, v)	v=*(volatile unsigned long*)(INT_FLAG + base);
#define READ_RST_REG(base, v)	v=*(volatile unsigned long*)(RST_REG + base);

#define DMA_INCS_0		0
#define DMA_INCS_16		1
#define DMA_INCS_32		2
#define DMA_INCS_48		3
#define DMA_INCS_64		4
#define DMA_INCS_128		5
#define DMA_INCS_256		6
#define DMA_INCS_512		7

#define DONT_CARE	0
#define PIXEL_Y	16
#define PIXEL_U	8
#define PIXEL_V	8
#define SIZE_Y		(PIXEL_Y * PIXEL_Y)
#define SIZE_U		(PIXEL_U * PIXEL_U)
#define SIZE_V		(PIXEL_V * PIXEL_V)
// for CbYCrY buffer
#define RGB_PIXEL_SIZE		2

// with RISC share memory
#define SHARE_MEM_ADDR			(0x8000)
// in sequencer mode 
// bank 0: 0x0 ~ 0x1000
// bank 1: 0x4000 ~ 0x4500
// Encode memory association
#define QCOEFF_BANK				(BANK_OFFSET + 0x0000)	// 0000 -- 0x0500 (10 bolcks)
#define ME_CMD_START_ADDR		(BANK_OFFSET + 0x1000)
#define VLCOUT_START_ADDR		(BANK_OFFSET + 0x0C00)
//#define DMA_CMD_START_ADDR	(BANK_OFFSET + 0x0EC0) 
//#define DMA_CMD_START_ADDR	(BANK_OFFSET + 0x0500)	// must be 0x0500 for sequencer
#define JE_DMA_CMD_OFF0			(BANK_OFFSET + 0x0500)	// must be 0x0500 for sequencer
#define ME_INTERPOLATION_ADDR	(BANK_OFFSET + 0x4500)
// decoder memory association
// in sequencer mode 
// bank 0: 0x0 ~ 0x1000
// bank 1: 0x4000 ~ 0x4500
//CUR_B0_ping must be 0x4000 if sequencer
//CUR_B0_pong must be 0x4280 if sequencer
#define CUR_B0_0		(BANK_OFFSET + 0x4000 )	//4000 -- 4280
#define CUR1_B0_0		(BANK_OFFSET + 0x4280 )	//4280 -- 4500
// must be 0x500 if sequencer
#define JD_DMA_CMD_OFF0  	(BANK_OFFSET + 0x500)		// 3 cmd-chain * 2 pp = 12 w *2 = 24w = 24*4B = 96B
																							// 0x500 ~ 0x580 (0x640)
#ifndef SEQ
#define JD_DMA_CMD_OFF1  	(BANK_OFFSET + 0x500+0x40)		// 3 cmd-chain * 2 pp = 12 w *2 = 24w = 24*4B = 96B
																										// 0x500 ~ 0x580 (0x640)
#define JD_DMA_STRIDE	((JD_DMA_CMD_OFF1 - JD_DMA_CMD_OFF0)/4)
// RGB buffer must stay at bank1
#define RGB_OFF0	(BANK_OFFSET + 0x4500)	// 0x4500 -- 0x4700=256 pixel * 2 B = 0x200
#define RGB_OFF1	(BANK_OFFSET + 0x4700)	// 0x4700 -- 0x4900
#endif

// ping pong buffer between VLD-DZ Engine and DQ-MC Engine
// (9 bolcks + 9 blocks) = 9 blocks*2 = 9 * 64pixel * 2= 576pixel*2 =576*2byte*2= 1152 byte*2
// since DZAR and QAR require 256B aligned
// must be 0x0000 if fsequencer
#define BUFFER_0  	( BANK_OFFSET + 0x000)		// 0x000 ~ 0x480
// must be 0x0800 if fsequencer
#define BUFFER_1 	( BANK_OFFSET + 0x800)		// 0x800 ~ 0xC80
#define VLD_AUTOBUFFER_ADDR	(BANK_OFFSET + 0xD00)		// 0xD00 ~ 0xF00

//#ifdef SUPPORT_VG_422T
#if defined(SUPPORT_VG_422T)||defined(SUPPORT_VG_422P)
#define DMACMD_422_0		(BUFFER_0)						// 0x000 ~ 0x030
#define DMACMD_422_OUT	(DMACMD_422_0 + 0x30)	// 0x030 ~ 0x040
#define DMACMD_422_1		(DMACMD_422_0 + 0x80)	// 0x080 ~ 0x0B0

#define IN_420_0			CUR_B0_0						// 0x4000 ~ 0x4180
#define IN_420_1			(CUR_B0_0 + 0x180)		// 0x4180 ~ 0x4300
#define OUT_422	 		(CUR_B0_0 + 0x300)		// 0x4300 ~ 0x4500
#endif

#define CUR_B0	(CUR_B0_0)							//4000 -- 4280
#define CUR_B1	(CUR_B0 + 0x40 )
#define CUR_B2	(CUR_B1 + 0x40 )
#define CUR_B3	(CUR_B2 + 0x40 )
#define CUR_B4	(CUR_B3 + 0x40 )
#define CUR_B5	(CUR_B4 + 0x40 )
#define CUR_B6	(CUR_B5 + 0x40 )
#define CUR_B7	(CUR_B6 + 0x40 )
#define CUR_B8	(CUR_B7 + 0x40 )
#define CUR_B9	(CUR_B8 + 0x40 )
#define CUR1_B0	(CUR1_B0_0)							//4000 -- 4280
#define STRIDE_MCU  (CUR1_B0 - CUR_B0)

#define HUFTBL0_AC_DEC(base) (BANK_OFFSET + 0x8000 + base)
#define HUFTBL1_AC_DEC(base) (BANK_OFFSET + 0x8800 + base)
#define HUFTBL0_DC_DEC(base) (BANK_OFFSET + 0x9000 + base)
#define HUFTBL1_DC_DEC(base) (BANK_OFFSET + 0x9100 + base)
#define QTBL0(base) (BANK_OFFSET + 0x7C00 + base)			//7C00 -- 7CFF
#define QTBL1(base) (BANK_OFFSET + 0x7D00 + base)			//7D00 -- 7DFF		
#define QTBL2(base) (BANK_OFFSET + 0x7E00 + base)			//7E00 -- 7EFF
#define QTBL3(base) (BANK_OFFSET + 0x7F00 + base)			//7F00 -- 7FFF
#define HUFTBL0_AC(base) (BANK_OFFSET + 0x8000 + base)		//8000 -- 8400
#define HUFTBL1_AC(base) (BANK_OFFSET + 0x8400 + base)		//8400 -- 8800
#define HUFTBL0_DC(base) (BANK_OFFSET + 0x8800 + base)		//8800 -- 8c00
#define HUFTBL1_DC(base) (BANK_OFFSET + 0x8c00 + base)		//8c00 -- 9000


#ifdef LINUX
#define FA526_DrainWriteBuffer()
#define FA526_CleanInvalidateDCacheAll()
#else
static __inline void FA526_DrainWriteBuffer(void)
{
  unsigned int tmp=0;
  __asm  { MCR  p15,0,tmp,c7,c10,4 }
}

static __inline void FA526_CleanInvalidateDCacheAll()
{
  unsigned int tmp=0;
  __asm  { MCR  p15,0,tmp,c7,c14,0 }
}
#endif

#if 1
#ifdef LINUX
#define F_DEBUG(fmt, args...) printk("FMJPEG: " fmt, ## args)
#define D_DEBUG(fmt, args...) printk("FMJPEG: " fmt, ## args)
#define K_DEBUG(fmt, args...) printk("FMJPEG: " fmt, ## args)
#else
#define F_DEBUG printf
#define D_DEBUG printf
#define K_DEBUG printf
#endif
#else
#define F_DEBUG(a...)
#define D_DEBUG(a...)
#define K_DEBUG(a...)
#endif


#endif
