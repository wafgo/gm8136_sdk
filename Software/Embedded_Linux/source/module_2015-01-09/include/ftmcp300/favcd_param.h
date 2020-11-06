
#ifndef _FAVCD_PARAM_H_
#define _FAVCD_PARAM_H_



/* mbinfo functions */

typedef enum {
    FAVCD_MBINFO_FMT_RAW = 0,
    FAVCD_MBINFO_FMT_16X16_BLK,
    FAVCD_MBINFO_FMT_MAX
} FAVCD_MBINFO_FMT_T;

extern int favcd_get_mbinfo(u32 fd, FAVCD_MBINFO_FMT_T fmt, u32 *buf, u32 size);


#endif /* _FAVCD_PARAM_H_ */
