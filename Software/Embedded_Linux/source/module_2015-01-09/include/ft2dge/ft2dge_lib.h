#ifndef __FT2DGE_IF_H__
#define __FT2DGE_IF_H__

#define FT2DGE_VERSION      "2.7"

typedef enum {
	FT2DGE_RGB_565 = 0,	//16bits, DSPF_RGB16
	FT2DGE_ARGB_1555 = 1,	//16bits, DSPF_ARGB1555
	FT2DGE_RGB_888 = 2,	    //32bits, DSPF_RGB32
	FT2DGE_ARGB_8888 = 3	//32bits, DSPF_ARGB
} ft2dge_bpp_t;

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

struct ft2dge_idx {
    unsigned int write_idx;
    unsigned int read_idx;
};

/* IOCTL command */
#define FT2DGE_IOC_MAGIC       'h'
#define FT2DGE_CMD_SYNC         _IOW(FT2DGE_IOC_MAGIC, 1, struct ft2dge_idx)
#define FT2DGE_CMD_VA2PA        _IOWR(FT2DGE_IOC_MAGIC, 2, int)

/* MACROs
 */
#define FT2DGE_XY(x, y) (((x) & 0xffff) << 16 | ((y) & 0xffff))
#define FT2DGE_WH(w, h) (((w) & 0xffff) << 16 | ((h) & 0xffff))
#define FT2DGE_PITCH(s, d)  (((s) & 0xffff) << 16 | ((d) & 0xffff))

/* line style */
#define STYLE_SOLID				0x00000000
#define STYLE_DASH              0x00FFFFC0      //  DASH         -------
#define STYLE_DOT               0x00E38E38      //  DOT          .......
#define STYLE_DASH_DOT          0x00FF81C0      //  DASHDOT      _._._._
#define STYLE_DASH_DOT_DOT      0x00FF8E38      //  DASHDOTDOT   _.._.._
#define INV_STYLE_DASH          0x0000003F
#define INV_STYLE_DOT           0x001C71C7
#define INV_STYLE_DASH_DOT      0x00007E3F
#define INV_STYLE_DASH_DOT_DOT  0x000071C7


/* frequently used ROP3 code */
#define ROP_BALCKNESS 	    0x00 	 	// 0x00 BLACKNESS
#define ROP_NOTSRCERASE     0x11 		// 0x11 NOTSRCERASE
#define ROP_NOTSRCCOPY 	    0x33  		// 0x33 NOTSRCCOPY
#define ROP_SRCERASE 	    0x44    	// 0x44 SRCERASE
#define ROP_DSTINVERT 	    0x55   		// 0x55 DSTINVERT
#define ROP_PATINVERT 	    0x5A   		// 0x5A PATINVERT
#define ROP_SRCINVERT 	    0x55 		// 0x66 SRCINVERT
#define ROP_SRCAND 		    0x88 		// 0x88 SRCAND
#define ROP_MERGEPAINT 	    0xBB 		// 0xBB MERGEPAINT
#define ROP_MERGECOPY 	    0xC0 		// 0xC0 MERGECOPY
#define ROP_SRCCOPY 	    0xCC 		// 0xCC SRCCOPY
#define ROP_SRCPAINT 	    0xEE 		// 0xEE SRCPAINT
#define ROP_PATCOPY 	    0xF0 		// 0xF0 PATCOPY
#define ROP_PATPAINT 	    0xFB 		// 0xFB PATPAINT
#define ROP_WHITENESS 	    0xFF 		// 0xFF WHITENESS

/*
 * Beside the general API, users can define the specific behavior by fill the command by themself
 */
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

#define LLST_MAX_CMD     384    /* it should be identical to HW_LLST_MAX_CMD */

/*
 * @This function is used to open a ft2dge channel for drawing.
 *
 * @function void *ft2dge_open(void);
 * @Parmameter reb_type indicates which RGB is used in this framebuffer
 * @return non zero means success, null for fail
 */
void *ft2dge_lib_open(ft2dge_bpp_t rgb_type);

/*
 * @This function is used to close a ft2dge channel.
 *
 * @function void ft2dge_close(void *cookie);
 * @Parmameter cookie indicates the cookie given by open stage
 * @return none
 */
void ft2dge_lib_close(void *descriptor);

/* fire all added commands
 * b_wait = 0: not wait for complete
 * b_wait = 1:  wait for complete
 * return value: 0 for success, -1 for fail
 */
int ft2dge_lib_fire(void *descriptor, int b_wait);

/* just add a draw line command but not fire yet */
int ft2dge_lib_line_add(void *descriptor, int src_x, int src_y, int dst_x, int dst_y, unsigned int color);
/* rectangle fill. source is from internal register or display memory */
int ft2dge_lib_rectangle_add(void *descriptor, int x, int y, int width, int height, unsigned int color);
/* source copy */
int ft2dge_lib_srccopy_add(void *descriptor, int src_x, int src_y, int dst_x, int dst_y, int width, int height, unsigned int src_paddr);

/* users can use this function to make the specific application.
 * Notice:
 *
 *  When this function is called, the semaphore should be called to protect the database
 */
void ft2dge_lib_set_param(void *descriptor, unsigned int ofs, unsigned int val);

/* get the physical address
 * return: 0xFFFFFFF for fail, others are success.
 */
unsigned int ft2dge_lib_get_paddr(void *descriptor, unsigned int vaddr);

/* get frame buffer (LCD) virtual address */
unsigned int ft2dge_lib_get_fbvaddr(void *descriptor);

#endif /* __FT2DGE_IF_H__ */
