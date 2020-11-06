#ifndef _MEM_POOL_H_
#define _MEM_POOL_H_

//#include "enc_driver/define.h"
#include "fmpeg4_driver/common/portab.h"
#include "fmcp_type.h"
#ifdef SUPPORT_VG
    #include "common.h"
#else
    #define TYPE_H264_ENC   0x31
    #define TYPE_MPEG4_ENC  0x32
    #define TYPE_DECODER    0x20
#endif

#define USE_GMLIB_CFG

#define SUPPORT_8M
#define MAX_NUM_REF_FRAMES  1
#define MAX_ENGINE_NUM      1
#define ALIGN16_UP(x)   (((x + 15) >> 4) << 4)
#define MAX_LINE_CHAR   256
#define MAX_LAYOUT_NUM  5
#ifdef SUPPORT_8M
    #define MAX_RESOLUTION  18
#else
    #define MAX_RESOLUTION  10
#endif
//#define SPEC_FILENAME   "/mnt/mtd/spec.cfg"
//#define PARAM_FILENAME  "/mnt/mtd/param.cfg"
#define SPEC_FILENAME   "spec.cfg"
#define PARAM_FILENAME  "param.cfg"
#define GMLIB_FILENAME  "gmlib.cfg"

#define MEM_ERROR           -2
#define NO_SUITABLE_BUF     -3
#define OVER_SPEC           -4
#define NOT_OVER_SPEC_ERROR -5

typedef enum {
    IDX_BS_BUF = 1,
    IDX_SYS_BUF,
    IDX_MB_BUF,
    IDX_L1COL_BUF,
    IDX_DIDN_BUF,
    IDX_SRC_BUF,
    IDX_REF_BUF,
    IDX_PRED_BUF,
    IDX_DEC_REF_BUF
} BufferTypeIdx;

#ifdef SUPPORT_8M
typedef enum {
    RES_8M = 0,     // 4096 x 2160  : 8847360
    RES_5M,         // 2560 x 1920  : 4659200
    RES_4M,         // 2304 x 1728  : 3981312
    RES_3M,         // 2048 x 1536  : 3145728
    RES_2M,         // 1920 x 1088  : 2088960
    RES_1080P,      // 1920 x 1088  : 2088960
    RES_1_3M,       // 1280 x 1024  : 1310720
    RES_1080I,      // 1920 x 544   : 1044480
    RES_1M,         // 1280 x 800   : 1024000
    RES_720P,       // 1280 x 720   : 921600
    RES_960H,       // 960 x 576    : 552960
    RES_720I,       // 1280 x 368   : 471040
    RES_D1,         // 720 x 576    : 414720
    RES_VGA,        // 640 x 480    : 307200
    RES_HD1,        // 720 x 368    : 264960
    RES_2CIF,       // 368 x 596    : 219328
    RES_CIF,        // 368 x 288    : 105984
    RES_QCIF,       // 176 x 144    : 25344
} ResoluationIdx;
#else
typedef enum {
    RES_1080P = 0,  // 1920 x 1088
    RES_1080I,      // 1920 x 544
    RES_720P,       // 1280 x 720
    RES_720I,       // 1280 x 360
    RES_960H,       // 960 x 480
    RES_D1,         // 720 x 576
    RES_HD1,        // 720 x 368
    RES_2CIF,       // 368 x 596
    RES_CIF,        // 352 x 288
    RES_QCIF,       // 176 x 144
} ResoluationIdx;
#endif

struct res_base_info_t {
    int index;
    char name[6];
    int width;
    int height;
};

struct res_info_t {
    int index;
    int max_chn;
    int max_fps;
};

struct ref_item_t {
    unsigned int addr_va;
    unsigned int addr_pa;
    unsigned int size;
    unsigned int mb_addr_va;
    unsigned int mb_addr_pa;
    unsigned int mb_size;
    ResoluationIdx res_type;    // resolution type
    int buf_idx;                // buffer index (unique)
    struct ref_item_t *prev;
    struct ref_item_t *next;
};

struct ref_pool_t {
    ResoluationIdx res_type;
    int max_chn;
    int max_fps;
    int size;
    int buf_num;
    int avail_num;
    int used_num;
    int reg_num;
    struct ref_item_t *avail;
    struct ref_item_t *alloc;
    int ddr_idx;
};

struct layout_info_t {
    struct ref_pool_t ref_pool[MAX_RESOLUTION];
    int max_channel;
    int index;
    int buf_num;
};

struct channel_ref_buffer_t {
    int chn;
    int used_num;
    int max_num;
    int res_type;
    struct ref_item_t *alloc_buf[MAX_NUM_REF_FRAMES + 1];
    int head;
    int tail;
    int suitable_res_type;
};

typedef struct
{
	int16_t quant;	
	int16_t mode;
	union VECTOR1 mvs[4];
	int32_t sad16;        // SAD value for inter-VECTOR
} MPEG4_MACROBLOCK_E;

struct ref_buffer_info_t
{
    unsigned int ref_phy;
    unsigned int ref_virt;
    unsigned int mb_phy;
    unsigned int mb_virt;
};

/************ local used ************/
// encoder
extern int enc_init_mem_pool(int max_width, int max_height);
extern int enc_clean_mem_pool(void);
extern int enc_dump_ref_pool(char *str);
extern int enc_dump_chn_pool(char *str);
extern int enc_dump_pool_num(char *str);
extern int enc_register_ref_pool(unsigned int chn, int width, int height);
extern int enc_deregister_ref_pool(unsigned int chn);
extern int enc_allocate_pool_buffer2(struct ref_buffer_info_t *buf, unsigned int res_idx, int chn);
extern int enc_release_pool_buffer2(struct ref_buffer_info_t *buf, int chn);

// decoder
extern int dec_init_mem_pool(int max_width, int max_height);
extern int dec_clean_mem_pool(void);
extern int dec_dump_ref_pool(char *str);
extern int dec_dump_chn_pool(char *str);
extern int dec_dump_pool_num(char *str);
extern int dec_register_ref_pool(unsigned int chn, int width, int height);
extern int dec_deregister_ref_pool(unsigned int chn);
extern int dec_allocate_pool_buffer2(struct ref_buffer_info_t *buf, unsigned int res_idx, int chn);
extern int dec_release_pool_buffer2(struct ref_buffer_info_t *buf, int chn);

/************ export symbol ************/
extern int init_mem_pool(int max_width, int max_height);
extern int clean_mem_pool(void);
extern int register_ref_pool(unsigned int chn, int width, int height, int codec_type);
extern int deregister_ref_pool(unsigned int chn, int codec_type);

extern int allocate_pool_buffer2(struct ref_buffer_info_t *buf, unsigned int res_idx, int chn, int codec_type);
extern int release_pool_buffer2(struct ref_buffer_info_t *buf, int chn, int codec_type);

extern int check_reg_buf_suitable(unsigned char *checker);
extern int mark_suitable_buf(int chn, unsigned char *checker, int width, int height);
//extern int parse_param_cfg(int *ddr_idx, int *compress_rate);
extern int parse_param_cfg(int *ddr_idx, int *compress_rate, int codec_type);
extern int dump_ref_pool(char *str);
extern int dump_chn_pool(char *str);
extern void dump_pool_num(int ddr_idx);

#endif
