#ifndef _THINK2D_IF_H_
#define _THINK2D_IF_H_

#define THINK2D_MMIOALLOC	0x1000
#define T2D_MAGIC		0x84465203 	///< Think2D accelerator magic number, as returned by hardware and used by drivers


#define T2D_SCALER_BASE_SHIFT	14		///< Scaler fixed point arithmetic precision

/* Driver constants */
#if 1
#define T2D_CWORDS		4080		///< Length of each command list buffer in words
#else
//#define T2D_CWORDS		0x100		///< Length of each command list buffer in words
#define T2D_CWORDS		512		///< Length of each command list buffer in words
#endif
#define T2D_WAIT_IDLE		_IO('T', 1)
#define T2D_START_BLOCK		_IO('T', 3)
#define T2D_RESET		_IO('T', 5) /* Nish20120620 add for debug reset*/
#define T2D_PRINT_PHY    _IOR('T', 7, unsigned int) //nish_test

/* Think2D register file */
#define T2D_TARGET_MODE		    0x00
#define T2D_TARGET_BLEND	    0x04
#define T2D_TARGET_BAR		    0x08
#define T2D_TARGET_STRIDE	    0x0c
#define T2D_TARGET_RESOLXY	    0x10
#define T2D_DST_COLORKEY	    0x14
#define T2D_CLIPMIN		        0x18
#define T2D_CLIPMAX		        0x1c
#define T2D_DRAW_CMD		    0x20
#define T2D_DRAW_STARTXY	    0x24
#define T2D_DRAW_ENDXY		    0x28
#define T2D_DRAW_COLOR		    0x2c
#define T2D_BLIT_CMD		    0x30
#define T2D_BLIT_SRCADDR	    0x34
#define T2D_BLIT_SRCRESOL	    0x38 
#define T2D_BLIT_SRCSTRIDE	    0x3c 
#define T2D_BLIT_SRCCOLORKEY	0x40 
#define T2D_BLIT_FG_COLOR	    0x44 
#define T2D_BLIT_DSTADDR	    0x48 
#define T2D_BLIT_DSTYXSIZE	    0x4c 
#define T2D_BLIT_SCALE_YFN	    0x50
#define T2D_BLIT_SCALE_XFN	    0x54 
#define T2D_SHADERBASE		    0x58 
#define T2D_SHADERCTRL		    0x5c 
#define T2D_IDREG		        0xec 
#define T2D_CMDADDR		        0xf0 
#define T2D_CMDSIZE		        0xf4 
#define T2D_INTERRUPT		    0xf8 
#define T2D_STATUS		        0xfc
#define T2D_REGFILE_SIZE	    0x100

/* If command lists are enabled, must be used with every drawing/blitting operation */
#define T2D_CMDLIST_WAIT	0x80000000

/* Think2D modes */
typedef enum{
    T2D_RGBX8888, //0x00
    T2D_RGBA8888, //0x01
    T2D_XRGB8888, //0x02
    T2D_ARGB8888, //0x03
    T2D_RGBA5650, //0x04
    T2D_RGBA5551, //0x05
    T2D_RGBA4444, //0x06
    T2D_RGBA0800, //0x07
    T2D_RGBA0008, //0x08
    T2D_L8,       //0x09
    T2D_RGBA3320, //0x0A
    T2D_LUT8,     //0x0B
    T2D_BW1,      //0x0C
    T2D_UYVY,     //0x0D
    T2D_DUMMY_MODE
}THINK2D_MODE_T;

/* Think2D scaling constants */
typedef enum{
    T2D_NOSCALE, //0x00
    T2D_UPSCALE, //0x01
    T2D_DNSCALE, //0x02
    T2D_DUMMY_SCALING
}THINK2D_SCALING_T;

/* Think2D rotation constants */
typedef enum{
    T2D_DEG000, //0x00
    T2D_DEG090, //0x01
    T2D_DEG180, //0x02
    T2D_DEG270, //0x03
    T2D_MIR_X,  //0x04
    T2D_MIR_Y,   //0x05
    T2D_DUMMY_ROTATION
}THINK2D_ROTATION_T;

/* Think2D predefined blending modes */
#define T2D_SRCCOPY		((DSBF_ZERO << 16) | DSBF_ONE)
#define T2D_SRCALPHA	((DSBF_INVSRCALPHA << 16) | DSBF_SRCALPHA)

/**
 * Returns the bytes per pixel for a mode.
 *
 * @param mode Surface pixel format
 */
inline unsigned int T2D_modesize(unsigned int mode)
{
	switch (mode)
	{
		case T2D_RGBA0800:
		case T2D_RGBA0008:
		case T2D_L8:
		case T2D_RGBA3320:
			return 1;

		case T2D_RGBA5650:
		case T2D_RGBA5551:
		case T2D_RGBA4444:
			return 2;

		case T2D_RGBX8888:
		case T2D_RGBA8888:
		case T2D_XRGB8888:
		case T2D_ARGB8888:
		default:
			return 4;
	}
}

/*
 * brief Data shared among hardware, kernel driver and Think2D driver.
 */
struct think2d_shared
{
	unsigned int hw_running;	///< Set while hardware is running
	unsigned int sw_buffer; 	///< Indicates which buffer is used by the driver to prepare the command list
	unsigned int sw_ready;		///< Number of ready commands in current software buffer

	unsigned int sw_preparing;	///< Number of words under preparation

	unsigned int num_starts;	///< Number of hardware starts (statistics)
	unsigned int num_interrupts;	///< Number of processed Think2D interrupts (statistics)
	unsigned int num_waits; 	///< Number of ioctl waits (statistics)
	unsigned int *sw_now;			///< First word in buffer under preparation
	unsigned int cbuffer[2][T2D_CWORDS]; ///< Command list double buffer
};


#endif
