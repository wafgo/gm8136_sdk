#ifndef __FT2D_DRIVER_FT2D_H__
#define __FT2D_DRIVER_FT2DH__

#define LLST_MAX_CMD     384    /* it should be identical to HW_LLST_MAX_CMD */

/* Information maintained by every user process
 */
typedef struct {
    struct {
        unsigned int ofs;
        unsigned int val;
    } cmd[LLST_MAX_CMD];
    int cmd_cnt;    //how many commands are active
    /* TODO semaphore */
} ft2dge_llst_t;

typedef enum {
    FT2DGE_CMD_BITBLT = 0,
    FT2DGE_CMD_COLORKEY = 1,
    FT2DGE_CMD_ENHANCE_COLOR_EXPANSION = 2,
    FT2DGE_CMD_LINE_DRAWING = 3,
    FT2DGE_CMD_ALPHA_BLENDING = 4,
} ft2dge_draw_cmd_t;

typedef union{
	unsigned int value;
	struct{
		unsigned int reserve_bit4				        :4;  //[00:03]
		unsigned int mono_pattern_or_match_not_write	:1;  //[04:04]
		unsigned int style_en					        :1;  //[05:05]
		unsigned int AlphaFormat  	    		        :2;  //[06:07]
		unsigned int no_source_image			        :1;  //[08:08]
		unsigned int Dst_mode					        :1;  //[09:09]
		unsigned int clip_outside				        :1;  //[10:10] // 1: outside of the rectangle is drawn
		unsigned int clip_en					        :1;  //[11:11]
		unsigned int color_tpr_ctrl_by_dst  	        :1;  //[12:12] // for color_tbitblt only
		unsigned int msline_n					        :1;  //[13:13]
		unsigned int tpr_en					            :1;  //[14:14]
		unsigned int inten_n					        :1;  //[15:15]
		unsigned int command					        :4;  //[16:19]
		unsigned int bpp						        :2;  //[20:21]
		unsigned int dither					            :2;  //[22:23]
		unsigned int rop						        :8;  //[24:31]
	} bits;
} ft2dge_cmd_t;

#define LLIST_BASE   0x80
typedef enum {
    FT2DGE_PATSA = 0x00 + LLIST_BASE,    /* Pattern pixel data start address */
    FT2DGE_SRCSA = 0x04 + LLIST_BASE,    /* Source pixel data start address */
    FT2DGE_DSTSA = 0x08 + LLIST_BASE,    /* Destination pixel data start address */
    FT2DGE_STBSA = 0x0C + LLIST_BASE,    /* Standby destination pixel data start address */
    FT2DGE_SPDP  = 0x10 + LLIST_BASE,    /* Source Pitch and Destination Pitch */
    FT2DGE_SRCWH = 0x14 + LLIST_BASE,    /* Source Rectangle Width and Height */
    FT2DGE_DSTWH = 0x18 + LLIST_BASE,    /* Destination rectangle width and height */
    FT2DGE_DSTWM = 0x1C + LLIST_BASE,    /* Destination Write Mask */
    FT2DGE_SRCXY = 0x20 + LLIST_BASE,    /* Source object upper-left X/Y coordinates */
    FT2DGE_PNTAXY= 0x20 + LLIST_BASE,    /* Endpoint-A X/Y coordinates for line drawing */
    FT2DGE_DSTXY = 0x24 + LLIST_BASE,    /* Destination object upper-left X/Y coordinates */
    FT2DGE_PNTBXY= 0x24 + LLIST_BASE,    /* Endpoint-B X/Y coordinates for line drawing */
    FT2DGE_CLPTL = 0x28 + LLIST_BASE,    /* Clipping window Top/Left Boundary */
    FT2DGE_CLPBR = 0x2C + LLIST_BASE,    /* Clipping Window Bottom/Right Boundary */
    FT2DGE_PATG1 = 0x30 + LLIST_BASE,    /* Monochrome Pattern Group 1 */
    FT2DGE_PATG2 = 0x34 + LLIST_BASE,    /* Monochrome Pattern Group 2 */
    FT2DGE_PBGCOLR = 0x38 + LLIST_BASE,  /* Pattern Background Color */
    FT2DGE_FPGCOLR = 0x3C + LLIST_BASE,  /* Pattern Forground Color */
    FT2DGE_SBGCOLR = 0x40 + LLIST_BASE,  /* Source Background Color */
    FT2DGE_SFGCOLR = 0x44 + LLIST_BASE,  /* Source Foreground Color */
    FT2DGE_TPRCOLR = 0x48 + LLIST_BASE,  /* Transparency Color */
    FT2DGE_TPRMASK = 0x4C + LLIST_BASE,  /* Transparency Color Mask */
    FT2DGE_LSTYLE  = 0x50 + LLIST_BASE,   /* Line Syle Pattern and AlphaFormat/SCA */
    FT2DGE_CMD     = 0x54 + LLIST_BASE,   /* Command */
    FT2DGE_TRIGGER = 0x58 + LLIST_BASE    /* Trigger */
} ft2dge_llst_cmd_t;

struct ft2dge_idx {
    unsigned int write_idx;
    unsigned int read_idx;
};
#define FT2DGE_SUPPORTED_ACCEL_DRAWING_FUNC    (FT2D_DRAWLINE|FT2D_DRAWRECT)
#define FT2DGE_SUPPORTED_ACCEL_FT2D_FILLINGRECT (FT2D_FILLINGRECT)
#define FT2DGE_SUPPORTED_ACCEL_BLITTING_FUNC    (FT2D_BLITTING)
#define FT2DGE_SUPPORTED_FILLINGRECT_FLAG (FT2D_COLOR_SET | FT2D_DSTINATION_SET )
#define FT2DGE_SUPPORTED_DRAWINGFLAGS  (FT2D_COLOR_SET | FT2D_DSTINATION_SET |FT2D_LINE_STYLE_SET)
#define FT2DGE_SUPPORTED_BLITTINGFLAGS ( FT2D_DSTINATION_SET | FT2D_SOURCE_SET | FT2D_ROP_METHOD_SET| FT2D_FORCE_FG_ALPHA | FT2D_SRC_COLORKEY_SET)

#define FT2DGE_RGB_565            0
#define FT2DGE_ARGB_1555          1
#define FT2DGE_RGB_888            2
#define FT2DGE_ARGB_8888          3

/* MACROs
 */
#define FT2DGE_XY(x, y) (((x) & 0xffff) << 16 | ((y) & 0xffff))
#define FT2DGE_WH(w, h) (((w) & 0xffff) << 16 | ((h) & 0xffff))
#define FT2DGE_PITCH(s, d)  (((s) & 0xffff) << 16 | ((d) & 0xffff))
#define f2dge_readl(a)	    (*(volatile unsigned int *)(a))
#define f2dge_writel(v,a)	(*(volatile unsigned int *)(a) = (v))

/* IOCTL command */
#define FT2DGE_IOC_MAGIC       'h'
#define FT2DGE_CMD_SYNC         _IOW(FT2DGE_IOC_MAGIC, 1, struct ft2dge_idx)
#define FT2DGE_CMD_VA2PA        _IOWR(FT2DGE_IOC_MAGIC, 2, int)

#define STATUS_DRV_BUSY         0x1
#define STATUS_DRV_INTR_FIRE    0x2


/* macros for buffer index operation
 */
#define HW_LLST_MAX_CMD     384
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

#endif