#ifndef _VCAP_LLI_H_
#define _VCAP_LLI_H_

/*************************************************************************************
 * VCAP LLI CMD Descriptor
 *************************************************************************************/
typedef enum {
    VCAP_LLI_DESC_NULL = 0,
    VCAP_LLI_DESC_STATUS,
    VCAP_LLI_DESC_UPDATE,
    VCAP_LLI_DESC_RESERVED3,
    VCAP_LLI_DESC_NEXT_LLI,
    VCAP_LLI_DESC_NEXT_CMD,
    VCAP_LLI_DESC_RESERVED6,
    VCAP_LLI_DESC_INFO
} VCAP_LLI_DESC_T;

/*************************************************************************************
 * VCAP LLI Null Command Structure
 *************************************************************************************/
struct vcap_lli_cmd_null_t {
    u32  reg_value;
    u32  reg_offset:19;
    u32  byte_en:4;
    u32  ch_id:6;
    u32  desc:3;
};

/*************************************************************************************
 * VCAP LLI Status Command Structure
 *************************************************************************************/
struct vcap_lli_cmd_status_t {
    u32  ready:1;
    u32  frame_mode:1;       ///< 0: field mode, 1: frame mode
    u32  field_type:1;       ///< 0: top field,  1: bottom filed
    u32  field_pair_1st:1;   ///< 0: 2nd field,  1: 1st field
    u32  split_ch:1;
    u32  reserved0:11;
    u32  frm_cnt:16;

    u32  skip:1;
    u32  dma_wc_done:1;
    u32  split_ch_done:1;
    u32  sd_fail:1;
    u32  vi_fifo_full:1;
    u32  vi_pixel_lack:1;
    u32  vi_line_lack:1;
    u32  reg_55a8_sts:1;
    u32  osd_dbg_info:4;
    u32  sd_dbg_info:7;

    u32  reserved1:1;
    u32  split:1;
    u32  sub_id:2;
    u32  ch_id:6;
    u32  desc:3;
};

#define VCAP_LLI_STATUS_CH_ID(x)         (((x)>>23)&0x3f)
#define VCAP_LLI_STATUS_SUB_ID(x)        (((x)>>21)&0x3)
#define VCAP_LLI_STATUS_FRAME_CNT(x)     (((x)>>16)&0xffff)

#define VCAP_LLI_STATUS_FRAME_MODE       1
#define VCAP_LLI_STATUS_FIELD_MODE       0
#define VCAP_LLI_STATUS_TOP_FIELD        0
#define VCAP_LLI_STATUS_BOT_FIELD        1

/*************************************************************************************
 * VCAP LLI Update Command Structure
 *************************************************************************************/
struct vcap_lli_cmd_update_t {
    u32  reg_value;
    u32  reg_offset:19;
    u32  byte_en:4;
    u32  ch_id:6;
    u32  desc:3;
};

/*************************************************************************************
 * VCAP LLI Next Link-List Command Pointer Structure
 *************************************************************************************/
struct vcap_lli_cmd_next_lli_t {
    u32  reserved0;
    u32  next:19;
    u32  reserved1:1;
    u32  split:1;
    u32  sub_id:2;
    u32  ch_id:6;
    u32  desc:3;
};

/*************************************************************************************
 * VCAP LLI Next Command Pointer Structure
 *************************************************************************************/
struct vcap_lli_cmd_next_cmd_t {
    u32  reserved0;
    u32  next:19;
    u32  reserved1:1;
    u32  split:1;
    u32  sub_id:2;
    u32  ch_id:6;
    u32  desc:3;
};

/*************************************************************************************
 * VCAP300 LLI Info Command Structure
 *************************************************************************************/
struct vcap_lli_cmd_info_t {
    u32  info0;
    u32  info1:19;
    u32  reserved1:1;
    u32  split:1;
    u32  sub_id:2;
    u32  ch_id:6;
    u32  desc:3;
};

#endif /* _VCAP_LLI_H_ */
