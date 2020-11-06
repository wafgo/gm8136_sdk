#ifndef __FSCALER300_UTIL_H__
#define __FSCALER300_UTIL_H__

#define BITMASK_BASE(idx)   (idx >> 5)
#define BITMASK_IDX(idx)    (idx & 0x1F)

#define REFCNT_INC(x)       atomic_inc(&(x)->ref_cnt)
#define REFCNT_DEC(x)       do {if (atomic_read(&(x)->ref_cnt)) atomic_dec(&(x)->ref_cnt);} while(0)

#define JOBCNT_ADD(x, y)    atomic_add(y, &(x)->job_cnt)
#define JOBCNT_SUB(x, y)    do {if (atomic_read(&(x)->job_cnt)) atomic_sub(y, &(x)->job_cnt);} while(0)

#define BLKCNT_INC(x)       atomic_inc(&(x)->blk_cnt)
//#define BLKCNT_ADD(x, y)    atomic_add(y, &(x)->blk_cnt)
#define BLKCNT_SUB(x, y)    do {if (atomic_read(&(x)->blk_cnt)) atomic_sub(y, &(x)->blk_cnt);} while(0)

#define LL_ADDR(x)          (x * 0x100)

#define MASK_REF_INC(x, y, z)  atomic_inc(&(x)->mask_ref_cnt[y][z])
#define MASK_REF_DEC(x, y, z)  atomic_dec(&(x)->mask_ref_cnt[y][z])

#define OSD_REF_INC(x, y, z)   atomic_inc(&(x)->osd_ref_cnt[y][z])
#define OSD_REF_DEC(x, y, z)   atomic_dec(&(x)->osd_ref_cnt[y][z])

#define MEMCNT_ADD(x, y)    atomic_add(y, &(x)->mem_cnt)
#define MEMCNT_SUB(x, y)    do {if (atomic_read(&(x)->mem_cnt)) atomic_sub(y, &(x)->mem_cnt);} while(0)

#endif
