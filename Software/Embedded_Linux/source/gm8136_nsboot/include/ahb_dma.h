#ifndef __AHB_DMA_H
	#define __AHB_DMA_H
	#include "board.h"

	typedef struct {
		INT32U ctrl;
		INT32U cfg;
		INT32U *src;
		INT32U *dst;
		INT32U llp;
		INT32U size;
		INT32U reserved[2];
	} CHANNELx;

	typedef struct {
		INT32U ints;			// 0x00
		INT32U int_tc;
		INT32U int_tc_clr;
		INT32U int_err;
		INT32U int_err_clr;		// 0x10
		INT32U tc;
		INT32U err;
		INT32U ch_en;
		INT32U ch_busy;			// 0x20
		INT32U csr;
		INT32U sync;			//0x28
		INT32U reserved[53];
		CHANNELx ch[8];		//0x100
	} ipDMA;

	typedef struct {
		INT32U src_addr;
		INT32U dst_addr;
		INT32U llp;
		INT32U llp_ctrl;
		INT32U size;
	}DMA_LLPD;


	#define REQ_NANDTX    2   /* Use CH2 in DMAC */
	#define REQ_NANDRX    2
    #define REQ_SPI020TX  8   /* Use CH8 in DMAC for NOR TX */
    #define REQ_SPI020RX  8   /* Use CH8 in DMAC for NOR RX */
    
	// bit[7:0]:	dedicated channel no if 0-7
	//			any channel no if other
	// bit[15: 8]: request no.
	/* The following defintions are not hardware relative. The following high byte is just table index to ahbdma_cmd[].
	 */
	#define DMA_MEM2MEM		((0UL << 8) | 0)            /* index 0 */

	#define NAND_CHANNEL            		0	/* Use CH0 in DMAC */
	/* define at ahb_dma.c ahbdma_cmd[4][2] */
	#define ARRAY_MEM2NAND							0
	#define ARRAY_NAND2MEM							1
	
	#define DMA_MEM2NAND	((ARRAY_MEM2NAND << 8) | NAND_CHANNEL)   /* index 1 */
	#define DMA_NAND2MEM	((ARRAY_NAND2MEM << 8) | NAND_CHANNEL)   /* index 2 */

	#define SPI020_CHANNEL            	0	/* Use CH0 in DMAC */
	/* define at ahb_dma.c ahbdma_cmd[4][2] */
	#define ARRAY_MEM2SPI020						2
	#define ARRAY_SPI0202MEM						3
	
	#define DMA_MEM2SPI020	((ARRAY_MEM2SPI020 << 8) | SPI020_CHANNEL)   /* index 1 */
	#define DMA_SPI0202MEM	((ARRAY_SPI0202MEM << 8) | SPI020_CHANNEL)   /* index 2 */
    

	#define DMA_MAX			8
    
	typedef void (* DMA_FINISH_CALLBACK)(volatile CHANNELx * pch, INT32U dma_st);

	#define AHB_DMA_EXT extern

	AHB_DMA_EXT INT32S Dma_go_trigger(INT32U * src, INT32U *dst, INT32U byte, DMA_LLPD *pllpd,INT32U req, int real);
	AHB_DMA_EXT INT32S Dma_wait_timeout(INT32S hw_ch, INT32U timeout);
	AHB_DMA_EXT void Dma_init(INT32U sync);


    //DMA Source size
    #define DMA_BURST_1			0x0L
    #define DMA_BURST_4			0x1L
    #define DMA_BURST_8			0x2L
    #define DMA_BURST_16		0x3L
    #define DMA_BURST_32		0x4L
    #define DMA_BURST_64		0x5L
    #define DMA_BURST_128		0x6L
    #define DMA_BURST_256		0x7L
    
    /* req: DMA_MEM2MEM / DMA_MEM2NAND ....
     * burst: DMA_BURST_1 ~ DMA_BURST_256
     * return:
     *  >= 0 for success
     */
    AHB_DMA_EXT int Dma_Set_BurstSz(INT32U req, int burst);

#endif /* __AHB_DMA_H  */

