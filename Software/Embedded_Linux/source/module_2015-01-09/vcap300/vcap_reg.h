#ifndef _VCAP_REG_H_
#define _VCAP_REG_H_

/*************************************************************************************
 *  Channel Parameter Memory Offset
 *************************************************************************************/
#define VCAP_CH_REG_SIZE                    0x1D8
#define VCAP_CH_REG_OFFSET(ch)              (VCAP_CH_REG_SIZE*(ch))

#define VCAP_CH_SUBCH_OFFSET(ch, offset)    (VCAP_CH_REG_OFFSET(ch) + 0x0000 + (offset))
#define VCAP_CH_FCS_OFFSET(ch, offset)      (VCAP_CH_REG_OFFSET(ch) + 0x0028 + (offset))
#define VCAP_CH_MASK_OFFSET(ch, offset)     (VCAP_CH_REG_OFFSET(ch) + 0x0030 + (offset))
#define VCAP_CH_DNSD_OFFSET(ch, offset)     (VCAP_CH_REG_OFFSET(ch) + 0x0070 + (offset))
#define VCAP_CH_OSD_OFFSET(ch, offset)      (VCAP_CH_REG_OFFSET(ch) + 0x00B8 + (offset))
#define VCAP_CH_TC_OFFSET(ch, offset)       (VCAP_CH_REG_OFFSET(ch) + 0x0160 + (offset))
#define VCAP_CH_MD_OFFSET(ch, offset)       (VCAP_CH_REG_OFFSET(ch) + 0x0180 + (offset))
#define VCAP_CH_DMA_OFFSET(ch, offset)      (VCAP_CH_REG_OFFSET(ch) + 0x0198 + (offset))
#define VCAP_CH_LL_OFFSET(ch, offset)       (VCAP_CH_REG_OFFSET(ch) + 0x01C0 + (offset))

/*************************************************************************************
 *  Cascade Channel Parameter Memory Offset
 *************************************************************************************/
#define VCAP_CAS_SUBCH_OFFSET(offset)        (0x3E00 + (offset))
#define VCAP_CAS_FCS_OFFSET(offset)          (0x3E28 + (offset))
#define VCAP_CAS_MASK_OFFSET(offset)         (0x3E30 + (offset))
#define VCAP_CAS_DNSD_OFFSET(offset)         (0x3E70 + (offset))
#define VCAP_CAS_OSD_OFFSET(offset)          (0x3EB8 + (offset))
#define VCAP_CAS_TC_OFFSET(offset)           (0x3F60 + (offset))
#define VCAP_CAS_MD_OFFSET(offset)           (0x3F80 + (offset))
#define VCAP_CAS_DMA_OFFSET(offset)          (0x3F98 + (offset))
#define VCAP_CAS_LL_OFFSET(offset)           (0x3FC0 + (offset))

 /*************************************************************************************
 *  Split Channel Parameter Memory Offset, split_ch: 0~31
 *************************************************************************************/
#define VCAP_SPLIT_REG_SIZE                  0x18
#define VCAP_SPLIT_CH_OFFSET(ch, offset)     (0x3B00 + (VCAP_SPLIT_REG_SIZE*(ch)) + (offset))

/*************************************************************************************
 *  Register Offset
 *************************************************************************************/
#define VCAP_VI_REG_SIZE                     0x70
#define VCAP_SC_REG_SIZE                     0x108
#define VCAP_PALETTE_REG_SIZE                0x40
#define VCAP_GLOBAL_REG_SIZE                 0x80
#define VCAP_STATUS_REG_SIZE                 0x100

#define VCAP_VI_OFFSET(offset)               (0x5000 + (offset))
#define VCAP_SC_OFFSET(offset)               (0x5100 + (offset))
#define VCAP_PALETTE_OFFSET(offset)          (0x5300 + (offset))
#define VCAP_GLOBAL_OFFSET(offset)           (0x5400 + (offset))
#define VCAP_STATUS_OFFSET(offset)           (0x5500 + (offset))
#define VCAP_CAS_VI_OFFSET(offset)           (0x5040 + (offset))
#define VCAP_CAS_SC_OFFSET(offset)           (0x5200 + (offset))

/*************************************************************************************
 *  Internal Memory Offset
 *************************************************************************************/
#define VCAP_OSD_MEM_SIZE                    PLAT_OSD_SRAM_SIZE
#define VCAP_MARK_MEM_SIZE                   PLAT_MARK_SRAM_SIZE
#define VCAP_OSD_DISP_MEM_SIZE               PLAT_DISP_SRAM_SIZE
#define VCAP_LLI_MEM_SIZE                    PLAT_LLI_SRAM_SIZE

#define VCAP_OSD_MEM_OFFSET(offset)          (0x10000 + (offset))
#define VCAP_MARK_MEM_OFFSET(offset)         (0x20000 + (offset))
#define VCAP_OSD_DISP_MEM_OFFSET(offset)     (0x30000 + (offset))
#define VCAP_LLI_MEM_OFFSET(offset)          (0x40000 + (offset))

#endif  /* _VCAP_REG_H_ */
