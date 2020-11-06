#ifndef __MS_CORE_H__
#define __MS_CORE_H__
#ifndef WIN32
    #include <linux/types.h>
    #include <linux/version.h>
    #include <linux/wait.h>
    #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
        #include <linux/kernel.h>
    #else
        #include <linux/printk.h>
    #endif
#else // define WIN32
    #include "linux_fnc.h"
#endif

#if (LINUX_VERSION_CODE == KERNEL_VERSION(2,6,14))
#include <frammap/frammap.h>
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28))
#include <frammap/frammap_if.h>
#endif

#define MS_MAX_DDR                          DDR_ID_MAX //set max ddr number from frammap
#define MS_MAX_NAME_LENGTH                  64
/* address alignment of all buf must be at 1K boundary */
#define MS_BUF_ALIGNMENT                    1024
#define MS_BUF_ALIGNMENT_MASK               (~(MS_BUF_ALIGNMENT - 1))
typedef unsigned long long user_token_t;
#define MS_USER_TOKEN(job_id,ver) \
    (user_token_t)((((unsigned long long)job_id)<<32)|((unsigned long long)ver))
#define MS_TOKEN_JOB_ID(user_token)  (unsigned int)(((user_token)>>32)&0xFFFFFFFF)
#define MS_TOKEN_BID(user_token) (unsigned int)((user_token)&0xFFFFFFFF)
#define MS_TOKEN_IS_EMPTY(user_token) ((user_token)==0)

enum MS_FREE_TAG {
    MS_FROM_GS = (1 << 0), 
    MS_FROM_EM = (1 << 1)
};

/** 
 * buffer manager family API 
 */
int ms_mgr_create(void);
void ms_mgr_destroy(void);
/** 
 * buffer sea family API 
 */
int ms_sea_create(int ddr_no, u32 size);
int ms_sea_destroy(int ddr_no);
u32 ms_sea_size(int ddr_no);
/** 
 * video graph buffer family API 
 */
struct ms_pool;
typedef struct ms_pool ms_pool_t;

enum MS_BUF_TYPE {
    MS_FIXED_BUF = 0x78, MS_VARLEN_BUF = 0x87,
};

enum MS_ALLOC_CXT {
    MS_ALLOC_FIXED = 0x00000001, MS_ALLOC_VARLEN = 0x00000002,
};

typedef struct {
    const char *name;
    int ddr_no;
    enum MS_BUF_TYPE type;
    u32 size;
}
ms_pool_attribute_t;

#define FLAG_ALGO_MODE_PASS         (1 << 0)
#define FLAG_INIT_PATTERN_BLACK     (1 << 1)
#define FLAG_FORBIDED_FREE_POOL     (1 << 2)
#define FLAG_CANFROM_FREE_POOL      (1 << 3)
#define FLAG_PASS_CHECK_BID         (1 << 4)

int ms_pool_create(ms_pool_t **pool, const ms_pool_attribute_t *attr, int flag);
int ms_pool_destroy(ms_pool_t **pool);

typedef struct {
    u32 size;
    u32 nr;
} ms_fixed_buf_des_t;

typedef struct {
    ms_fixed_buf_des_t *fbds;
} ms_fixed_pool_layout_t;

typedef struct {
    /* total size of slot */
    u32 size;
    /* max size to alloc onc varlen buffer. Surely, this should be less than size */
    u32 window_size;
    /* valid slot_id must be all different, can ranging from 0 ~ (4G-1)*/
    u32 slot_id;
} ms_varlen_buf_des_t;

typedef struct {
    ms_varlen_buf_des_t *vbds;
} ms_varlen_pool_layout_t;

typedef struct {
    int ver;
    char  user[40];
    unsigned int user_begin;   //AU?Ouser begin??
    unsigned int  size;
    unsigned int  time;
} ms_layout_fragment_t;

typedef void (*ms_buf_addr_update)(void *priv, void *new_addr);
int ms_fixed_pool_layout_change(ms_pool_t *pool, ms_fixed_pool_layout_t *layout);
void *ms_fixed_buf_normal_alloc_timeout(ms_pool_t *pool, u32 size, ms_buf_addr_update cb,
    void *cb_priv, u8 *need_realloc, u32 timeout_msec, const user_token_t user, char *name, int can_from_free);
void *ms_fixed_buf_alloc_timeout(ms_pool_t *pool, u32 size, ms_buf_addr_update cb,
    void *cb_priv, u8 *need_realloc, u32 timeout_msec, const user_token_t user, char *name);
void *ms_fixed_buf_alloc(ms_pool_t *pool, u32 size, ms_buf_addr_update cb,
    void *cb_priv, u8 *need_realloc, const user_token_t user, char *name, int can_from_free);
int ms_layout_is_ready(ms_pool_t *pool);
int ms_AllLayout_is_ready(void);
void ms_notify_fromGs(void);

u32 ms_fix_buf_get_usenr(ms_pool_t *pool, u32 size);
ms_fixed_pool_layout_t *ms_fix_buf_container_of_pool(u32 addr);
/* return the user's private data, user can not store 0 to it, 
0 is used by ms internally. */
void *ms_fixed_buf_private(void *buf);
int ms_fixed_buf_start_to_wait(ms_pool_t *pool, u32 size, wait_queue_head_t *wq, int *wake_condition);
int ms_varlen_pool_layout_change(ms_pool_t *pool, const ms_varlen_pool_layout_t *layout);

void *ms_varlen_buf_alloc_timeout(ms_pool_t *pool, u32 slot_id, u32 size, ms_buf_addr_update cb,
    void *cb_priv, u8 *need_realloc, u32 timeout_msec, const user_token_t user, int can_from_free);

void *ms_varlen_buf_alloc(ms_pool_t *pool, u32 slot_id, u32 size,
    ms_buf_addr_update cb, void *cb_priv, u8 *need_realloc, const user_token_t user, int can_from_free);

int ms_varlen_get_usedbuf_counter(ms_pool_t *pool, u32 slot_id);
int ms_varlen_buf_complete(ms_pool_t *pool, u32 slot_id, void *buf, u32 used_size);
int ms_reserve_by_user(void *buf);
int ms_buf_is_used(void *buf);
int ms_buf_free(void *buf, enum MS_FREE_TAG free_tag);
int ms_get_fixed_pool_usage(ms_pool_t *pool, ms_layout_fragment_t *pool_usage, int nr);
int ms_check_job_buf_id(ms_pool_t *ms_pool, unsigned int max_bid,  unsigned int min_bid);
int ms_get_fixedpoolinfo_by_name(char *ms_pool_name, unsigned int *pool_pa_start,
                            unsigned int *pool_va_start, unsigned int *pool_size);
/* for debug use */
void ms_sea_info(int ddr_no);

/**
 * for those users who need very fast structure allocation. 
 * NOTE: size can't be larger than PAGE_SIZE (usually 4KB)
 */
void *ms_small_fast_alloc_timeout(u32 size, u32 timeout_msec);
void *ms_small_fast_alloc(u32 size);
int ms_small_fast_free(void *buf);

/* for debug use */
void ms_small_fast_info(void);
/* va to pa, return corrosponding pa which is related to va */
void *ms_phy_address(void *va);
u32 ms_which_buffer_sea(u32 va);
u32 ms_offset_in_buffer_sea(u32 va);
u32 ms_buffer_sea_pa_base(u8 ddr_no);
u32 ms_buffer_sea_size(u8 ddr_no);
void ms_set_extra_pool(ms_pool_t *src_pool, ms_pool_t *extra_pool, unsigned int buf_size, unsigned int buf_nr);
ms_pool_t * ms_get_pool_by_name(char *pool_name);
#endif // end of __MS_CORE_H__


