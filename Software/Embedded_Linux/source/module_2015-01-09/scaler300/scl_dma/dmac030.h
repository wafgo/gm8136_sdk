#ifndef __DMA030_H__
#define __DMA030_H__

#define DMAC030_OFFSET_CTRL_CH(x)	(0x100 + (x) * 0x20)
#define DMAC030_OFFSET_CFG_CH(x)	(0x104 + (x) * 0x20)
#define DMAC030_OFFSET_SRC_CH(x)	(0x108 + (x) * 0x20)
#define DMAC030_OFFSET_DST_CH(x)	(0x10c + (x) * 0x20)
#define DMAC030_OFFSET_LLP_CH(x)	(0x110 + (x) * 0x20)
#define DMAC030_OFFSET_CYC_CH(x)	(0x114 + (x) * 0x20)
#define DMAC030_OFFSET_STRIDE_CH(x)	(0x118 + (x) * 0x20)

#define DMAC030_CTRL_DST_WIDTH_8	(0x0 << 22)
#define DMAC030_CTRL_DST_WIDTH_16	(0x1 << 22)
#define DMAC030_CTRL_DST_WIDTH_32	(0x2 << 22)
#define DMAC030_CTRL_DST_WIDTH_64	(0x3 << 22)
#define DMAC030_CTRL_SRC_WIDTH_8	(0x0 << 25)
#define DMAC030_CTRL_SRC_WIDTH_16	(0x1 << 25)
#define DMAC030_CTRL_SRC_WIDTH_32	(0x2 << 25)
#define DMAC030_CTRL_SRC_WIDTH_64	(0x3 << 25)
#define DMAC030_CTRL_MASK_TC		(1 << 28)

#define GRANT_WINDOW	64
#define DMA030_CONFIG_REG       ((GRANT_WINDOW & 0xFF) << 20)
#define DMA030_CONFIG_HI_REG    ((0x1 << 28) /* high pri */ | ((GRANT_WINDOW & 0xFF) << 20))
#define AXI_ALIGN_64(x)             ((((x) + 63) >> 6) << 6)
#define AXI_ALIGN_8(x)              ((((x) + 7) >> 3) << 3)

/**
 * struct dmac030_lld - hardware link list descriptor.
 * @src: source physical address
 * @dst: destination physical addr
 * @next: phsical address to the next link list descriptor
 * @ctrl: control field
 * @cycle: transfer size
 * @stride: stride for 2D mode
 *
 * should be 32 or 64 bits aligned depends on AXI configuration
 */
struct dmac030_lld {
	dma_addr_t src;         /* Source Address */
	dma_addr_t dst;         /* Destination Address */
	dma_addr_t next_lld;    /* Link List Pointer */
	unsigned int ctrl;      /* Control,  Expanded Control */
	unsigned int cnt;       /* YCNT, Total size(XCNT) */
	unsigned int stride;    /* DSTRIDE, SSTRIDE */
};


#endif /* __DMA030_H__ */