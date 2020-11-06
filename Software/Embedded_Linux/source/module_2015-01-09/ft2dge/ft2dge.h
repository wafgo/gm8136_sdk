#ifndef __FT2DGE_H__
#define __FT2DGE_H__

#define FT2DGE_PITCHTOREG(s, d) ((s) << 16 | ((d) & 0xffff))
#define FT2DGE_XYTOREG(x, y) ((x) << 16 | ((y) & 0xffff))
#define FT2DGE_WHTOREG(w, h) ((w) << 16 | ((h) & 0xffff))
#define MAKE_RGB565(r, g, b)  ((((r) & 0x1F) << 11) | (((g) & 0x3F) << 5) | ((b) & 0x1F))

#define HW_LLST_MAX_CMD     384

#define STATUS_DRV_BUSY         0x1
#define STATUS_DRV_INTR_FIRE    0x2


/* macros for buffer index operation
 */
#define FT2DGE_GET_BUFIDX(x)	        ((x) % HW_LLST_MAX_CMD)
#define FT2DGE_BUFIDX_DIST(head, tail)	((head) >= (tail) ? (head) - (tail) : ((unsigned short)~0 - (tail) + (head) + 1))
#define FT2DGE_BUF_SPACE(head, tail)	(HW_LLST_MAX_CMD - FT2DGE_BUFIDX_DIST(head, tail))
#define FT2DGE_BUF_COUNT(head, tail)    FT2DGE_BUFIDX_DIST(head, tail)

/*
 * The memory shared between kernel and user space
 */
typedef struct {
    struct {
        unsigned int ofs;
        unsigned int val;
    } cmd[HW_LLST_MAX_CMD];
    unsigned int read_idx;  //updated by kernel space
    unsigned int write_idx; //updated by write space
    unsigned int fire_count;    //how many parameters are processed
    unsigned int status;    //driver status
    unsigned int usr_fire;
    unsigned int drv_fire;
} ft2dge_sharemem_t;

#endif /* __FT2DGE_H__*/