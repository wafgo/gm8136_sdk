/*
   (c) Copyright 2001-2009  The world wide DirectFB Open Source Community (directfb.org)
   (c) Copyright 2000-2004  Convergence (integrated media) GmbH

   All rights reserved.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Changelog:
   Wed Mar 17 2010, version 0.1: First version
   2014/08/28 klcheng , allways set write and read unlock
*/

#include <stdio.h>
#include <memory.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fbdev/fbdev.h>

#include <directfb.h>
#include <core/graphics_driver.h>

#include "think2d_gfx.h"

/// Enables debug messages for graphics operations
//#define DEBUG_OPERATIONS

#ifdef DEBUG_OPERATIONS
#define DEBUG_OP(args...)	printf(args)
#else
#define DEBUG_OP(args...)
#endif

/// This enables or disables command list support in driver.
#define USE_CLIST

/// Enables debug messages on procedure entry.
//#define DEBUG_PROCENTRY

#ifdef DEBUG_PROCENTRY
#define DEBUG_PROC_ENTRY	do { printf("* %s\n", __FUNCTION__); } while (0)
#else
#define DEBUG_PROC_ENTRY
#endif

DFB_GRAPHICS_DRIVER( think2d )

/// Macro to spin on the hardware status. If a lockup is detected, it prints out an error message, dumps device registers and resets the device.
#define WAIT_THINK2D(sdrv)											\
do 														\
{														\
	spin_count = 0;												\
	while(T2D_GETREG(sdrv, T2D_STATUS) != 0)								\
		if (spin_count++ > 100000)									\
		{												\
			D_ERROR("Think2D: Device lockup! (status:0x%08x)\n", T2D_GETREG(sdrv, T2D_STATUS));	\
			dump_registers(sdrv);									\
			Think2D_EngineReset(tdrv, tdev);							\
		}												\
}														\
while (0)

//#define WAIT_THINK2D(sdrv) while(T2D_GETREG(sdrv, T2D_STATUS)!=0);
//#define WAIT_THINK2D(sdrv) while(1) { spin_count++; printf("%x:%u\n", T2D_STATUS, T2D_GETREG(sdrv, T2D_STATUS)); if (!T2D_GETREG(sdrv, T2D_STATUS)) break; }
//#define WAIT_THINK2D(sdrv) usleep(50)

// Color conversion macros
#define GET5551_R(x)            ((((x) & 0xF800) >> 11) << 3 | ((x) & 0xF800) >> 13)
#define GET5551_G(x)            ((((x) & 0x07C0) >> 6) << 3 | ((x) & 0x07C0) >> 8)
#define GET5551_B(x)            ((((x) & 0x003e) >> 1) << 3 | ((x) & 0x003e) >> 3)
#define GET5551_A(x)            (((x) & 0x1) * 255)
#define COLOUR5551(r, g, b, a)  (((r) & 0x000000f8) << 8 | ((g) & 0x000000f8) << 3 | ((b) & 0x000000f8) >> 2 | ((a) & 0x00000080) >> 7)
#define COLOUR8888(r, g, b, a)  (((r) & 0x000000ff) << 24 | ((g) & 0x000000ff) << 16 | ((b) & 0x000000ff) << 8 | ((a) & 0x000000ff))
#define OPAQUE(color)           ((color) | 0x000000FF)
#define TRANSP(alpha, color)    ((color) & 0xFFFFFF00 | (alpha))

/// Reads a device register.
#define T2D_GETREG(sdrv, reg) (*(volatile u32 *)((unsigned long)sdrv->mmio_base + reg))

/// Writes a device register.
#define T2D_SETREG(sdrv, reg, val) do { *(volatile u32 *)((unsigned long)sdrv->mmio_base + reg) = (val); } while (0)
//#define T2D_SETREG(sdrv, reg, val) do { *(volatile u32 *)((unsigned long)sdrv->mmio_base + reg) = (val); printf("%%%%%%%%%%%%%% '%02lx'=0x%08x\n", reg, val); } while (0)

/// Packs a pair of 16-bit coordinates in a 32-bit word as used by hardware.
#define T2D_XYTOREG(x, y) ((y) << 16 | ((x) & 0xffff))

#define T2D_SUPPORTED_DRAWINGFUNCTIONS (DFXL_FILLRECTANGLE | DFXL_DRAWRECTANGLE | DFXL_DRAWLINE)
#define T2D_SUPPORTED_BLITTINGFUNCTIONS (DFXL_BLIT | DFXL_STRETCHBLIT)
#define T2D_SUPPORTED_DRAWINGFLAGS (DSDRAW_BLEND | DSDRAW_DST_COLORKEY)
#define T2D_SUPPORTED_BLITTINGFLAGS (DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_BLEND_COLORALPHA | DSBLIT_SRC_PREMULTCOLOR | DSBLIT_ROTATE90 | DSBLIT_ROTATE180 | DSBLIT_ROTATE270 | DSBLIT_SRC_COLORKEY | DSBLIT_DST_COLORKEY | DSBLIT_COLORIZE | DSBLIT_FLIP_HORIZONTAL | DSBLIT_FLIP_VERTICAL)

static void Think2D_EngineReset( void *drv, void *dev );

unsigned int stat_blit = 0;			///< Number of blits (statistics)
unsigned int stat_fillrect = 0;			///< Number of filled rectangles (statistics)
unsigned int stat_lines = 0;			///< Number of lines (statistics)
unsigned long spin_count;			///< Used by WAIT_THINK2D() macro to detect lockups

/**
 * \brief Think2D device data.
 *
 * Members are set in Think2D_SetState() and callees and used in other functions.
 *
 * @see Think2D_SetState()
 */
typedef struct {
	unsigned int src_format;		///< Current source surface format
	unsigned int dst_format;		///< Current destination surface format
	unsigned long v_srcAddr;		///< Current source surface physical address
	unsigned int v_srcStride;		///< Current source surface stride
	DFBSurfaceBlendFunction v_srcBlend;	///< Current source surface blend function
	DFBSurfaceBlendFunction v_dstBlend;	///< Current destination surface blend function

	DFBSurfaceDrawingFlags drawingflags;	///< Current drawing flags
	DFBSurfaceBlittingFlags blittingflags;	///< Current blitting flags
	DFBAccelerationMask lastop;		///< Last operation type. Possible values are DFXL_ALL_DRAW, DFXL_ALL_BLIT.
} Think2D_DeviceData;

/**
 * \brief Think2D driver data.
 */
typedef struct {
	int gfx_fd;				///< Think2D opened device file descriptor
	struct think2d_shared *gfx_shared;	///< Pointer to data block shared among hardware, kernel driver and DirectFB driver
	volatile u32 *mmio_base;		///< Base address for Think2D register file as mapped to driver's address space
} Think2D_DriverData;

/**
 * Converts from DirectFB to Think2D surface pixel format constants
 * and also performs validation for unsupported source or
 * destination formats.
 *
 * As of DirectFB 1.4.3, YUV pixel format defines are somewhat confusing, so
 * the following may need to change in the future.
 *
 * @param format A DirectFB surface pixel format constant, defined in include/directfb.h
 * @param flag Indicates whether it is a source of destination surface. Possible values are SMF_SOURCE, SMF_DESTINATION.
 */
static inline int
t2d_video_mode( DFBSurfacePixelFormat format,
                uint                  flag )
{
    int ret = -1;
	
	switch (format)
	{
		case DSPF_RGB16:	
		{
			ret = T2D_RGBA5650;
			break;
		}
		case DSPF_RGB32:	
		{
			ret = T2D_XRGB8888;
			break;
		}
		case DSPF_ARGB:
		{
			ret = T2D_ARGB8888;
			break;
		}
		case DSPF_RGBA4444:
		{
			ret = T2D_RGBA4444;
			break;
		}
		case DSPF_RGBA5551:	
		{
			ret = T2D_RGBA5551;
			break;
		}
		case DSPF_A8:
		{
			ret = T2D_RGBA0008;
			break;
		}
		case DSPF_YUY2:	
		{
			ret =( flag == SMF_SOURCE ? T2D_UYVY : -1 ); // FIXME: DirectFB documentation is unclear about YUV surface pixel formats
			break;
		}
		case DSPF_A1:		
		{
			ret = ( flag == SMF_SOURCE ? T2D_BW1 : -1);
			break;
		}
		default:		
		{
			ret = -1;
			break;
		}
	}
	DEBUG_OP("t2d_video_mode %d\n", ret);
	return ret;
}

#ifdef USE_CLIST
static DFBResult Think2D_EngineNext( Think2D_DriverData *tdrv );
u32 **p_buffer; ///< Pointer to command list buffer under preparation.

/**
 * Starts hardware command list processing.
 *
 * Only compiled if building with command list support.
 *
 * @param tdrv Driver data
 */
static inline void
flush_prepared( Think2D_DriverData *tdrv )
{
	/* if there are unprocessed commands and the hardware is not running, start it! */
	if (tdrv->gfx_shared->sw_ready)
	{
		/* multiple checks are not strictly necessary, but aim in reducing the overhead */
		if (!tdrv->gfx_shared->hw_running && !T2D_GETREG(tdrv, T2D_STATUS))
			ioctl(tdrv->gfx_fd, T2D_START_BLOCK, 0);
	}
}

/**
 * Initiates a command list buffer operation.
 *
 * Makes sure that the requested number of words is available
 * in the command list buffer. May call flush_prepared() and
 * possibly wait until sufficient space is found.
 *
 * Only compiled if building with command list support.
 *
 * @param tdrv    Driver data
 * @param entries Number of words to request space for
 */
static inline void
start_buffer( Think2D_DriverData *tdrv,
              unsigned int        entries )
{
	/* if there is no space available... */
	while (tdrv->gfx_shared->sw_ready + entries > T2D_CWORDS)
	{
		/* start the hardware in case it is idle; this will also switch buffers ending in a lot of free space */
		flush_prepared(tdrv);

		/* if the hardware started and we have available space, leave */
		if (tdrv->gfx_shared->sw_ready + entries <= T2D_CWORDS)
			break;

		/* otherwise, wait for a block to finish before trying again */
		Think2D_EngineNext(tdrv);
	}

	/* set up a portion of the buffer as being prepared */
	tdrv->gfx_shared->sw_preparing = entries;
	tdrv->gfx_shared->sw_now = &tdrv->gfx_shared->cbuffer[tdrv->gfx_shared->sw_buffer][tdrv->gfx_shared->sw_ready];
}

/**
 * Finishes a command list buffer operation.
 *
 * Only compiled if building with command list support.
 *
 * @param tdrv Driver data
 */
static inline void
submit_buffer( Think2D_DriverData *tdrv )
{
	/* finish buffer preparation by considering new entries as ready */
	tdrv->gfx_shared->sw_ready += tdrv->gfx_shared->sw_preparing;
}
#endif

/**
 * Sets blit source.
 *
 * Needs to save source address and pitch because Blit or StretchBlit
 * may specify source coordinates and we lack a source
 * XY register.
 *
 * @param tdrv Driver data
 * @param tdev Device data
 * @param state Card state
 */
static inline void
t2d_set_src( Think2D_DriverData *tdrv,
             Think2D_DeviceData *tdev,
	     CardState          *state )
{
	DEBUG_PROC_ENTRY;

	tdev->v_srcAddr = state->src.phys;
	tdev->v_srcStride = state->src.pitch;

#ifdef USE_CLIST
	start_buffer(tdrv, 2);

	(*p_buffer)[0] = T2D_BLIT_SRCSTRIDE;
	(*p_buffer)[1] = state->src.pitch;

	submit_buffer(tdrv);
#else
	T2D_SETREG(tdrv, T2D_BLIT_SRCSTRIDE, state->src.pitch);
#endif
}

/**
 * Sets colorization color.
 *
 * Sets the color to be used for colorizing _or_ color modulation
 * with alpha value. The latter is essentially identical to colorizing
 * but using the alpha value instead of an RGB color.
 *
 * @param tdrv Driver data
 * @param tdev Device data
 * @param state Card state
 */
static inline void
t2d_set_color( Think2D_DriverData *tdrv,
               Think2D_DeviceData *tdev,
	       CardState          *state )
{
	DEBUG_PROC_ENTRY;

#ifdef USE_CLIST
	start_buffer(tdrv, 2);

	(*p_buffer)[0] = T2D_BLIT_FG_COLOR;

	if (tdev->blittingflags & DSBLIT_SRC_PREMULTCOLOR)
		(*p_buffer)[1] = COLOUR8888(state->color.a, state->color.a, state->color.a, state->color.a);
	else
		(*p_buffer)[1] = COLOUR8888(state->color.r, state->color.g, state->color.b, state->color.a);

	submit_buffer(tdrv);
#else
	if (tdev->blittingflags & DSBLIT_SRC_PREMULTCOLOR)
		T2D_SETREG(tdrv, T2D_BLIT_FG_COLOR, COLOUR8888(state->color.a, state->color.a, state->color.a, state->color.a));
	else
		T2D_SETREG(tdrv, T2D_BLIT_FG_COLOR, COLOUR8888(state->color.r, state->color.g, state->color.b, state->color.a));
#endif
}

/**
 * Sets source color key register.
 *
 * @param tdrv Driver data
 * @param tdev Device data
 * @param state Card state
 */
static inline void
t2d_set_src_colorkey( Think2D_DriverData *tdrv,
                      Think2D_DeviceData *tdev,
		      CardState          *state )
{
	DEBUG_PROC_ENTRY;

#ifdef USE_CLIST
	start_buffer(tdrv, 2);

	(*p_buffer)[0] = T2D_BLIT_SRCCOLORKEY;
	(*p_buffer)[1] = state->src_colorkey;

	submit_buffer(tdrv);
#else
	T2D_SETREG(tdrv, T2D_BLIT_SRCCOLORKEY, state->src_colorkey);
#endif
}

/**
 * Sets destination color key register.
 *
 * @param tdrv Driver data
 * @param tdev Device data
 * @param state Card state
 */
static inline void
t2d_set_dst_colorkey( Think2D_DriverData *tdrv,
                      Think2D_DeviceData *tdev,
		      CardState          *state )
{
	DEBUG_PROC_ENTRY;

#ifdef USE_CLIST
	start_buffer(tdrv, 2);

	(*p_buffer)[0] = T2D_DST_COLORKEY;
	(*p_buffer)[1] = state->dst_colorkey;

	submit_buffer(tdrv);
#else
	T2D_SETREG(tdrv, T2D_DST_COLORKEY, state->dst_colorkey);
#endif
}

/**
 * Sets destination surface registers.
 *
 * @param tdrv Driver data
 * @param tdev Device data
 * @param state Card state
 */
static inline void
t2d_set_dest( Think2D_DriverData *tdrv,
              Think2D_DeviceData *tdev,
	      CardState          *state )
{
	DEBUG_PROC_ENTRY;

#ifdef USE_CLIST
	start_buffer(tdrv, 8);

	(*p_buffer)[0] = T2D_TARGET_BAR;
	(*p_buffer)[1] = state->dst.phys;
	(*p_buffer)[2] = T2D_TARGET_STRIDE;
	(*p_buffer)[3] = state->dst.pitch;
	(*p_buffer)[4] = T2D_TARGET_RESOLXY;
	//(*p_buffer)[5] = T2D_XYTOREG(800, 480);
	(*p_buffer)[5] = T2D_XYTOREG(1920, 1080);
	(*p_buffer)[6] = T2D_TARGET_MODE;
	(*p_buffer)[7] = (1 << 31) | (0 << 29) | tdev->dst_format;
	//(*p_buffer)[7] = (T2D_modesize(tdev->dst_format) == 4 ? 0 : ((1 << 31) | (1 << 23))) | (0 << 29) | tdev->dst_format;  // IS:bit 29 disables 16->32 packing, bits 31/23 set on 32-bit mode

	submit_buffer(tdrv);
#else
	T2D_SETREG(tdrv, T2D_TARGET_BAR, state->dst.phys);
	T2D_SETREG(tdrv, T2D_TARGET_STRIDE, state->dst.pitch);
	T2D_SETREG(tdrv, T2D_TARGET_RESOLXY, T2D_XYTOREG(800, 480));
	T2D_SETREG(tdrv, T2D_TARGET_MODE, (0 << 29) | tdev->dst_format);
#endif
}

/**
 * Programs the blender from a drawing perspective.
 *
 * This function needs to be called whenever the type of operation
 * (draw/blit) changes, because DirectFB assumes separate drawing
 * and blitting blend states, but we don't. Of course this also
 * gets called when drawing flags or blending modes change.
 *
 * @param tdrv Driver data
 * @param tdev Device data
 * @param state Card state
 */
static inline void
t2d_set_draw_blend( Think2D_DriverData *tdrv,
                    Think2D_DeviceData *tdev,
		    CardState          *state )
{
	int dst_colorkey = 0;

	DEBUG_PROC_ENTRY;

	if (tdev->drawingflags & DSDRAW_DST_COLORKEY)
		dst_colorkey = 1;

#ifdef USE_CLIST
	start_buffer(tdrv, 2);

	(*p_buffer)[0] = T2D_TARGET_BLEND;

	if (tdev->drawingflags & DSDRAW_BLEND)
		(*p_buffer)[1] = (dst_colorkey << 31) | (tdev->v_dstBlend << 16) | (tdev->v_srcBlend);
	else
		(*p_buffer)[1] = (dst_colorkey << 31) | T2D_SRCCOPY;

	submit_buffer(tdrv);
#else
	if (tdev->drawingflags & DSDRAW_BLEND)
		T2D_SETREG(tdrv, T2D_TARGET_BLEND, (dst_colorkey << 31) | (tdev->v_dstBlend << 16) | (tdev->v_srcBlend));
	else
		T2D_SETREG(tdrv, T2D_TARGET_BLEND, (dst_colorkey << 31) | T2D_SRCCOPY);
#endif
}

/**
 * Programs the blender from a blitting perspective.
 *
 * This function needs to be called whenever the type of operation
 * (draw/blit) changes, because DirectFB assumes separate drawing
 * and blitting blend states, but we don't. Of course this also
 * gets called when blitting flags or blending modes change. Note
 * that hardware restrictions as described in Think2D_CheckState()
 * have been taken into account.
 *
 * @see Think2D_SetState()
 * @param tdrv Driver data
 * @param tdev Device data
 * @param state Card state
 */
static inline void
t2d_set_blit_blend( Think2D_DriverData *tdrv,
                    Think2D_DeviceData *tdev,
		    CardState          *state )
{
	int dst_colorkey = 0;
	int colorize = 0;

	DEBUG_PROC_ENTRY;

	/* will we do DST_COLORKEY? */
	if (tdev->blittingflags & DSBLIT_DST_COLORKEY)
		dst_colorkey = 1;
	/* will we be using the FG_COLOR register for COLORIZE _or_ SRC_PREMULTIPLY? */
	if (tdev->blittingflags & (DSBLIT_COLORIZE | DSBLIT_SRC_PREMULTCOLOR))
		colorize = 1;

#ifdef USE_CLIST
	start_buffer(tdrv, 2);

	(*p_buffer)[0] = T2D_TARGET_BLEND;

	if (tdev->blittingflags & (DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL))
		(*p_buffer)[1] = (dst_colorkey << 31) | (tdev->v_dstBlend << 16) | (tdev->v_srcBlend);
	else
		(*p_buffer)[1] = (dst_colorkey << 31) | (colorize << 30) | T2D_SRCCOPY;

	submit_buffer(tdrv);
#else
	if (tdev->blittingflags & (DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL))
		T2D_SETREG(tdrv, T2D_TARGET_BLEND, (dst_colorkey << 31) | (tdev->v_dstBlend << 16) | (tdev->v_srcBlend));
	else
		T2D_SETREG(tdrv, T2D_TARGET_BLEND, (dst_colorkey << 31) | (colorize << 30) | T2D_SRCCOPY);
#endif
}

/**
 * Sets the color for drawing operations.
 *
 * @param tdrv Driver data
 * @param tdev Device data
 * @param state Card state
 */
static inline void
t2d_set_drawing_color( Think2D_DriverData *tdrv,
                       Think2D_DeviceData *tdev,
		       CardState          *state )
{
	DEBUG_PROC_ENTRY;

#ifdef USE_CLIST
	start_buffer(tdrv, 2);

	(*p_buffer)[0] = T2D_DRAW_COLOR;
	//if (t2d_video_mode(DFBSurfacePixelFormat format,uint flag)/*always use 8888*/
	(*p_buffer)[1] = COLOUR8888(state->color.r, state->color.g, state->color.b, state->color.a);

	submit_buffer(tdrv);
#else
	T2D_SETREG(tdrv, T2D_DRAW_COLOR, COLOUR8888(state->color.r, state->color.g, state->color.b, state->color.a));
#endif
}

/**
 * Sets the clipping registers.
 *
 * @param tdrv Driver data
 * @param tdev Device data
 * @param clip Clip region
 */
static inline void
t2d_set_clip( Think2D_DriverData *tdrv,
              Think2D_DeviceData *tdev,
	      DFBRegion          *clip )
{
	DEBUG_PROC_ENTRY;

#ifdef USE_CLIST
	start_buffer(tdrv, 4);

	(*p_buffer)[0] = T2D_CLIPMIN;
	(*p_buffer)[1] = T2D_XYTOREG(clip->x1, clip->y1);
	(*p_buffer)[2] = T2D_CLIPMAX;
	(*p_buffer)[3] = T2D_XYTOREG(clip->x2, clip->y2);

	submit_buffer(tdrv);
#else
     	T2D_SETREG(tdrv, T2D_CLIPMIN, T2D_XYTOREG(clip->x1, clip->y1));
	T2D_SETREG(tdrv, T2D_CLIPMAX, T2D_XYTOREG(clip->x2, clip->y2));
#endif
}

/**
 * Dumps all DirectFB flags in state and acceleration mask.
 *
 * @param state Card state
 * @param accel Acceleration mask
 */
static void
dump_flags( CardState           *state,
            DFBAccelerationMask  accel )
{
	printf("accel: DFXL_DRAWRECTANGLE:%d DFXL_DRAWLINE:%d DFXL_FILLRECTANGLE:%d DFXL_FILLTRIANGLE:%d DFXL_BLIT:%d DFXL_STRETCHBLIT:%d DFXL_TEXTRIANGLES:%d DFXL_DRAWSTRING:%d\n", accel & DFXL_DRAWRECTANGLE, accel & DFXL_DRAWLINE, accel & DFXL_FILLRECTANGLE, accel & DFXL_FILLTRIANGLE, accel & DFXL_BLIT, accel & DFXL_STRETCHBLIT, accel & DFXL_TEXTRIANGLES, accel & DFXL_DRAWSTRING);
	printf("mod_hw: SMF_DESTINATION:%d SMF_CLIP:%d SMF_SRC_BLEND:%d SMF_DST_BLEND:%d SMF_COLOR:%d SMF_SOURCE:%d SMF_DRAWING_FLAGS:%d SMF_BLITTING_FLAGS:%d SMF_RENDER_OPTIONS:%d\n", state->mod_hw & SMF_DESTINATION, state->mod_hw & SMF_CLIP, state->mod_hw & SMF_SRC_BLEND, state->mod_hw & SMF_DST_BLEND, state->mod_hw & SMF_COLOR, state->mod_hw & SMF_SOURCE, state->mod_hw & SMF_DRAWING_FLAGS, state->mod_hw & SMF_BLITTING_FLAGS, state->mod_hw & SMF_RENDER_OPTIONS);
	printf("drawing flags: DSDRAW_BLEND:%d DSDRAW_DST_COLORKEY:%d DSDRAW_SRC_PREMULTIPLY:%d DSDRAW_DST_PREMULTIPLY:%d DSDRAW_DEMULTIPLY:%d DSDRAW_XOR:%d\n", state->drawingflags & DSDRAW_BLEND, state->drawingflags & DSDRAW_DST_COLORKEY, state->drawingflags & DSDRAW_SRC_PREMULTIPLY, state->drawingflags & DSDRAW_DST_PREMULTIPLY, state->drawingflags & DSDRAW_DEMULTIPLY, state->drawingflags & DSDRAW_XOR);
	printf("blitting flags: DSBLIT_BLEND_ALPHACHANNEL:%d DSBLIT_BLEND_COLORALPHA:%d DSBLIT_COLORIZE:%d DSBLIT_SRC_COLORKEY:%d DSBLIT_DST_COLORKEY:%d DSBLIT_SRC_PREMULTIPLY:%d DSBLIT_DST_PREMULTIPLY:%d DSBLIT_DEMULTIPLY:%d\n", state->blittingflags & DSBLIT_BLEND_ALPHACHANNEL, state->blittingflags & DSBLIT_BLEND_COLORALPHA, state->blittingflags & DSBLIT_COLORIZE, state->blittingflags & DSBLIT_SRC_COLORKEY, state->blittingflags & DSBLIT_DST_COLORKEY, state->blittingflags & DSBLIT_SRC_PREMULTIPLY, state->blittingflags & DSBLIT_DST_PREMULTIPLY, state->blittingflags & DSBLIT_DEMULTIPLY);
	printf("                DSBLIT_DEINTERLACE:%d DSBLIT_SRC_PREMULTCOLOR:%d DSBLIT_XOR:%d DSBLIT_INDEX_TRANSLATION:%d DSBLIT_ROTATE90:%d DSBLIT_ROTATE180:%d DSBLIT_ROTATE270:%d DSBLIT_COLORKEY_PROTECT:%d\n                DSBLIT_SRC_MASK_ALPHA:%d DSBLIT_SRC_MASK_COLOR:%d\n", state->blittingflags & DSBLIT_DEINTERLACE, state->blittingflags & DSBLIT_SRC_PREMULTCOLOR, state->blittingflags & DSBLIT_XOR, state->blittingflags & DSBLIT_INDEX_TRANSLATION, state->blittingflags & DSBLIT_ROTATE90, state->blittingflags & DSBLIT_ROTATE180, state->blittingflags & DSBLIT_ROTATE270, state->blittingflags & DSBLIT_COLORKEY_PROTECT, state->blittingflags & DSBLIT_SRC_MASK_ALPHA, state->blittingflags & DSBLIT_SRC_MASK_COLOR);
}

/**
 * Dumps all device registers.
 *
 * @param tdrv Driver data
 */
static void
dump_registers( Think2D_DriverData *tdrv )
{
    printf("\ndump_registers [s]\n");
	printf("T2D_TARGET_MODE:%08x T2D_TARGET_BLEND:%08x T2D_TARGET_BAR:%08x T2D_TARGET_STRIDE:%08x T2D_TARGET_RESOLXY:%08x\nT2D_DST_COLORKEY:%08x\nT2D_CLIPMIN:%08x T2D_CLIPMAX:%08x\n", T2D_GETREG(tdrv, T2D_TARGET_MODE), T2D_GETREG(tdrv, T2D_TARGET_BLEND), T2D_GETREG(tdrv, T2D_TARGET_BAR), T2D_GETREG(tdrv, T2D_TARGET_STRIDE), T2D_GETREG(tdrv, T2D_TARGET_RESOLXY), T2D_GETREG(tdrv, T2D_DST_COLORKEY), T2D_GETREG(tdrv, T2D_CLIPMIN), T2D_GETREG(tdrv, T2D_CLIPMAX));
	printf("T2D_DRAW_CMD:%08x T2D_DRAW_STARTXY:%08x T2D_DRAW_ENDXY:%08x T2D_DRAW_COLOR:%08x\n", T2D_GETREG(tdrv, T2D_DRAW_CMD), T2D_GETREG(tdrv, T2D_DRAW_STARTXY), T2D_GETREG(tdrv, T2D_DRAW_ENDXY), T2D_GETREG(tdrv, T2D_DRAW_COLOR));
	printf("T2D_BLIT_CMD:%08x T2D_BLIT_SRCADDR:%08x T2D_BLIT_SRCRESOL:%08x T2D_BLIT_SRCSTRIDE:%08x T2D_BLIT_SRCCOLORKEY:%08x T2D_BLIT_FG_COLOR:%08x T2D_BLIT_DSTADDR:%08x T2D_BLIT_DSTYXSIZE:%08x\nT2D_BLIT_SCALE_YFN:%08x T2D_BLIT_SCALE_XFN:%08x\n", T2D_GETREG(tdrv, T2D_BLIT_CMD), T2D_GETREG(tdrv, T2D_BLIT_SRCADDR), T2D_GETREG(tdrv, T2D_BLIT_SRCRESOL), T2D_GETREG(tdrv, T2D_BLIT_SRCSTRIDE), T2D_GETREG(tdrv, T2D_BLIT_SRCCOLORKEY), T2D_GETREG(tdrv, T2D_BLIT_FG_COLOR), T2D_GETREG(tdrv, T2D_BLIT_DSTADDR), T2D_GETREG(tdrv, T2D_BLIT_DSTYXSIZE), T2D_GETREG(tdrv, T2D_BLIT_SCALE_YFN), T2D_GETREG(tdrv, T2D_BLIT_SCALE_XFN));
	printf("T2D_SHADERBASE:%08x T2D_SHADERCTRL:%08x\n", T2D_GETREG(tdrv, T2D_SHADERBASE), T2D_GETREG(tdrv, T2D_SHADERCTRL));
	printf("T2D_IDREG:%08x T2D_CMDADDR:%08x T2D_CMDSIZE:%08x\n", T2D_GETREG(tdrv, T2D_SHADERBASE), T2D_GETREG(tdrv, T2D_SHADERCTRL), T2D_GETREG(tdrv, T2D_IDREG), T2D_GETREG(tdrv, T2D_CMDADDR), T2D_GETREG(tdrv, T2D_CMDSIZE));
	printf("T2D_INTERRUPT:%08x T2D_STATUS:%08x\n", T2D_GETREG(tdrv, T2D_SHADERBASE), T2D_GETREG(tdrv, T2D_SHADERCTRL), T2D_GETREG(tdrv, T2D_IDREG), T2D_GETREG(tdrv, T2D_CMDADDR), T2D_GETREG(tdrv, T2D_CMDSIZE), T2D_GETREG(tdrv, T2D_INTERRUPT), T2D_GETREG(tdrv, T2D_STATUS));
    printf("dump_registers [e]\n");
}

/**
 * DirectFB callback that decides whether a drawing/blitting operation is supported in hardware.
 *
 * @param drv Driver data
 * @param dev Device data
 * @param state Card state
 * @param accel Acceleration mask
 */
static void
Think2D_CheckState( void                *drv,
                    void                *dev,
                    CardState           *state,
		    DFBAccelerationMask  accel )
{
	Think2D_DriverData *tdrv = (Think2D_DriverData*) drv;
	Think2D_DeviceData *tdev = (Think2D_DeviceData*) dev;
	CoreSurface *source = state->source;
	CoreSurface *destination = state->destination;

	static unsigned int rej = 0;

	DEBUG_PROC_ENTRY;
	DEBUG_OP("Checking for %d...\n", accel);

	/* reject if we don't support the destination format */
	if (t2d_video_mode(destination->config.format, SMF_DESTINATION) == -1)
	{
		//printf("[%u] Rejecting: bad destination video mode: %u\n", rej++, destination->config.format);
		return;
	}

	if (DFB_DRAWING_FUNCTION(accel))
	{
		/* reject if:
		 *  - we don't support the drawing function
		 *  - we don't support the drawing flag */
		if (accel & ~T2D_SUPPORTED_DRAWINGFUNCTIONS || state->drawingflags & ~T2D_SUPPORTED_DRAWINGFLAGS)
		{
			printf("[%u] Rejecting, cannot draw...\n", rej++);
			dump_flags(state, accel);
			return;
		}

		state->accel |= T2D_SUPPORTED_DRAWINGFUNCTIONS;
	}
	else
	{
		/* reject if:
		 *  - we don't support the blitting function
		 *  - we don't support the blitting flag at all
		 *  - we are asked to do both COLORIZE and SRC_PREMULTCOLOR
		 *  - we are asked to do both COLORALPHA and ALPHACHANNEL
		 *  - we are asked to do COLORIZE _or_ SRC_PREMULTCOLOR along with blending but from a not-purely-alpha source surface
		 *  - we don't support the source format */
		if (accel & ~T2D_SUPPORTED_BLITTINGFUNCTIONS ||
			state->blittingflags & ~T2D_SUPPORTED_BLITTINGFLAGS ||
			(state->blittingflags & DSBLIT_COLORIZE && state->blittingflags & DSBLIT_SRC_PREMULTCOLOR) ||
			(state->blittingflags & DSBLIT_BLEND_ALPHACHANNEL && state->blittingflags & DSBLIT_BLEND_COLORALPHA) ||
			(state->blittingflags & (DSBLIT_COLORIZE | DSBLIT_SRC_PREMULTCOLOR) && state->blittingflags & (DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_BLEND_COLORALPHA) && source->config.format != DSPF_A8))
		{
			printf("[%u] Rejecting, cannot blit...\n", rej++);
			dump_flags(state, accel);
			return;
		}

		if (t2d_video_mode(source->config.format, SMF_SOURCE) == -1)
		{
			//printf("[%u] Rejecting: bad source video mode: %u\n", rej++, source->config.format);
			return;
		}

		state->accel |= T2D_SUPPORTED_BLITTINGFUNCTIONS;
	}

	DEBUG_OP("    ...we'll do it.\n");
}

/**
 * DirectFB callback that prepares the hardware for a drawing/blitting operation.
 *
 * @param drv Driver data
 * @param dev Device data
 * @param funcs Graphics device functions (may be altered)
 * @param state Card state
 * @param accel Acceleration mask
 */
static void
Think2D_SetState( void                *drv,
                  void                *dev,
                  GraphicsDeviceFuncs *funcs,
                  CardState           *state,
		  DFBAccelerationMask  accel )
{
	Think2D_DriverData *tdrv      = (Think2D_DriverData*) drv;
	Think2D_DeviceData *tdev      = (Think2D_DeviceData*) dev;

	CoreSurface *destination      = state->destination;
	CoreSurface *source           = state->source;
	StateModificationFlags mod_hw = state->mod_hw;

	DEBUG_PROC_ENTRY;

#ifndef USE_CLIST
	WAIT_THINK2D(tdrv);
#endif

	/* update first the drawing and blitting flags */
	if (mod_hw & SMF_DRAWING_FLAGS)
		tdev->drawingflags = state->drawingflags;
	if (mod_hw & SMF_BLITTING_FLAGS)
		tdev->blittingflags = state->blittingflags;

	/* update other fields common between drawing and blitting */
	if (mod_hw & SMF_DESTINATION)
	{
		tdev->dst_format = t2d_video_mode(destination->config.format, SMF_DESTINATION);
		t2d_set_dest(tdrv, tdev, state);
	}
	if (mod_hw & SMF_CLIP)
		t2d_set_clip(tdrv, tdev, &state->clip);
	if (mod_hw & SMF_SRC_COLORKEY)
		t2d_set_src_colorkey(tdrv, tdev, state);
	if (mod_hw & SMF_DST_COLORKEY)
		t2d_set_dst_colorkey(tdrv, tdev, state);
	if (mod_hw & (SMF_SRC_BLEND | SMF_DST_BLEND))
		tdev->v_srcBlend = state->src_blend, tdev->v_dstBlend = state->dst_blend;

	switch (accel)
	{
	case DFXL_DRAWRECTANGLE:
	case DFXL_DRAWLINE:
	case DFXL_FILLRECTANGLE:
		/* for drawing operations, set drawing color and blend; the lastop check is necessary, see t2d_set_draw_blend() */
		if (mod_hw & SMF_COLOR)
			t2d_set_drawing_color(tdrv, tdev, state);
		if (mod_hw & (SMF_SRC_BLEND | SMF_DST_BLEND) || mod_hw & SMF_DRAWING_FLAGS || tdev->lastop != DFXL_ALL_DRAW)
			t2d_set_draw_blend(tdrv, tdev, state);

		state->set |= DFXL_FILLRECTANGLE | DFXL_DRAWRECTANGLE | DFXL_DRAWLINE;
		tdev->lastop = DFXL_ALL_DRAW;
		break;

	case DFXL_BLIT:
	case DFXL_STRETCHBLIT:
		/* for blitting operations also set source; the lastop check is necessary, see t2d_set_blit_blend() */
		//if (mod_hw & SMF_SOURCE || state->source->config.format == DSPF_A8) //FIXME: due to a DirectFB bug the latter check is necessary as of version 1.4.3
		{
			tdev->src_format = t2d_video_mode(state->source->config.format, SMF_SOURCE);
			t2d_set_src(tdrv, tdev, state);
		}
		if (mod_hw & SMF_COLOR)
			t2d_set_color(tdrv, tdev, state);
		if (mod_hw & (SMF_SRC_BLEND | SMF_DST_BLEND) || mod_hw & SMF_BLITTING_FLAGS || tdev->lastop != DFXL_ALL_BLIT)
			t2d_set_blit_blend(tdrv, tdev, state);

		state->set |= DFXL_BLIT | DFXL_STRETCHBLIT;
		tdev->lastop = DFXL_ALL_BLIT;
		break;
	default:
		D_BUG("Unexpected drawing/blitting function");
	}

	state->mod_hw = 0;
}

#ifdef USE_CLIST
/**
 * DirectFB callback that emits buffered commands.
 *
 * Some drivers (e.g. Renesas sh772x driver) store prepared commands
 * in a local buffer and use EmitCommands() to copy that buffer to a
 * global, shared buffer. In our case, prepared commands are appended
 * directly to the shared buffer, so we only need to start the
 * hardware here.
 *
 * Only compiled if building with command list support.
 *
 * @param drv Driver data
 * @param dev Device data
 */
static void
Think2D_EmitCommands( void *drv,
                      void *dev )
{
	Think2D_DriverData *tdrv = (Think2D_DriverData*) drv;

	DEBUG_PROC_ENTRY;

	flush_prepared(tdrv);
}
#endif

/**
 * DirectFB callback that draws a line.
 *
 * @param drv Driver data
 * @param dev Device data
 * @param line Line region
 */
static bool
Think2D_DrawLine( void      *drv,
                  void      *dev,
		  DFBRegion *line )
{
	Think2D_DriverData *tdrv = (Think2D_DriverData*) drv;
	Think2D_DeviceData *tdev = (Think2D_DeviceData*) dev;

	DEBUG_PROC_ENTRY;
	DEBUG_OP("DrawLine (%s) (%d, %d)-(%d, %d)\n", tdev->drawingflags & DSDRAW_BLEND ? "A" : "", line->x1, line->y1, line->x2, line->y2);

	stat_lines++;

#ifdef USE_CLIST
	start_buffer(tdrv, 6);

	(*p_buffer)[0] = T2D_DRAW_STARTXY;
	(*p_buffer)[1] = T2D_XYTOREG(line->x1, line->y1);
	(*p_buffer)[2] = T2D_DRAW_ENDXY;
	(*p_buffer)[3] = T2D_XYTOREG(line->x2, line->y2);
	(*p_buffer)[4] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
	(*p_buffer)[5] = 0x01;

	submit_buffer(tdrv);
#else
	WAIT_THINK2D(tdrv);
	T2D_SETREG(tdrv, T2D_DRAW_STARTXY, T2D_XYTOREG(line->x1, line->y1));
	T2D_SETREG(tdrv, T2D_DRAW_ENDXY, T2D_XYTOREG(line->x2, line->y2));
	T2D_SETREG(tdrv, T2D_DRAW_CMD, 0x01);
#endif

	return true;
}

/**
 * DirectFB callback that draws a filled rectangle.
 *
 * @param drv Driver data
 * @param dev Device data
 * @param rect Rectangle coordinates
 */
static bool
Think2D_FillRectangle( void         *drv,
                       void         *dev,
                       DFBRectangle *rect )
{
	Think2D_DriverData *tdrv = (Think2D_DriverData*) drv;
	Think2D_DeviceData *tdev = (Think2D_DeviceData*) dev;

	DEBUG_PROC_ENTRY;
	DEBUG_OP("FillRectangle (%s) (%d, %d), %dx%d\n", tdev->drawingflags & DSDRAW_BLEND ? "A" : "", rect->x, rect->y, rect->w, rect->h);

	stat_fillrect++;

#ifdef USE_CLIST
	start_buffer(tdrv, 6);

	(*p_buffer)[0] = T2D_DRAW_STARTXY;
	(*p_buffer)[1] = T2D_XYTOREG(rect->x, rect->y);
	(*p_buffer)[2] = T2D_DRAW_ENDXY;
	(*p_buffer)[3] = T2D_XYTOREG(rect->x + rect->w - 1, rect->y + rect->h - 1);
	(*p_buffer)[4] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
	(*p_buffer)[5] = 0x02;

	submit_buffer(tdrv);
#else
	WAIT_THINK2D(tdrv);
	T2D_SETREG(tdrv, T2D_DRAW_STARTXY, T2D_XYTOREG(rect->x, rect->y));
	T2D_SETREG(tdrv, T2D_DRAW_ENDXY, T2D_XYTOREG(rect->x + rect->w - 1, rect->y + rect->h - 1));
	T2D_SETREG(tdrv, T2D_DRAW_CMD, 0x02);
#endif

	return true;
}

/**
 * DirectFB callback that blits an image.
 *
 * We use information saved on t2d_set_src() to also set the
 * source address register.
 *
 * @see t2d_set_src()
 * @param drv Driver data
 * @param dev Device data
 * @param rect Source rectangle
 * @param dx Destination X
 * @param dy Destination Y
 */
static bool
Think2D_Blit( void         *drv,
              void         *dev,
	      DFBRectangle *rect,
	      int           dx,
	      int           dy )
{
	Think2D_DriverData *tdrv = (Think2D_DriverData*) drv;
	Think2D_DeviceData *tdev = (Think2D_DeviceData*) dev;

	DEBUG_PROC_ENTRY;
	DEBUG_OP("Blit (%s%s%s%s%s) (%d, %d), %dx%d\n", tdev->blittingflags & (DSBLIT_ROTATE90 | DSBLIT_ROTATE180 | DSBLIT_ROTATE270 | DSBLIT_FLIP_HORIZONTAL | DSBLIT_FLIP_VERTICAL) ? "R" : "", tdev->blittingflags & DSBLIT_SRC_COLORKEY ? "K" : "", tdev->blittingflags & DSBLIT_DST_COLORKEY ? "D" : "", tdev->blittingflags & (DSBLIT_COLORIZE | DSBLIT_SRC_PREMULTCOLOR) ? "C" : "", tdev->blittingflags & (DSBLIT_BLEND_COLORALPHA | DSBLIT_BLEND_ALPHACHANNEL) ? "A" : "", dx, dy, rect->w, rect->h);

	int rotation = 0;
	int src_colorkey = 0;
	int color_alpha = 0;
	unsigned long srcAddr = tdev->v_srcAddr + rect->x * T2D_modesize(tdev->src_format) + rect->y * tdev->v_srcStride;

	stat_blit++;

	/* will we do rotation? */
	if (tdev->blittingflags & DSBLIT_ROTATE90)
		rotation = T2D_DEG090, dy += rect->h - 1;
	else if (tdev->blittingflags & DSBLIT_ROTATE180)
		rotation = T2D_DEG180, dx += rect->w - 1, dy += rect->h - 1;
	else if (tdev->blittingflags & DSBLIT_ROTATE270)
		rotation = T2D_DEG270, dx += rect->h - 1;
	else if (tdev->blittingflags & DSBLIT_FLIP_HORIZONTAL)
		rotation = T2D_MIR_X, dx += rect->w - 1;
	else if (tdev->blittingflags & DSBLIT_FLIP_VERTICAL)
		rotation = T2D_MIR_Y, dy += rect->h - 1;

	/* will we do SRC_COLORKEY? */
	if (tdev->blittingflags & DSBLIT_SRC_COLORKEY)
		src_colorkey = 1;

	/* will we do COLORALPHA? */
	if (tdev->blittingflags & DSBLIT_BLEND_COLORALPHA)
		color_alpha = 1;

#ifdef USE_CLIST
	start_buffer(tdrv, 8);

	(*p_buffer)[0] = T2D_BLIT_SRCADDR;
	(*p_buffer)[1] = srcAddr;
	(*p_buffer)[2] = T2D_BLIT_SRCRESOL;
	(*p_buffer)[3] = T2D_XYTOREG(rect->w, rect->h);
	(*p_buffer)[4] = T2D_BLIT_DSTADDR;
	(*p_buffer)[5] = T2D_XYTOREG(dx, dy);
	(*p_buffer)[6] = T2D_CMDLIST_WAIT | T2D_BLIT_CMD;
	(*p_buffer)[7] = (color_alpha << 28) | (src_colorkey << 27) | (rotation & 0x07) << 20 | tdev->src_format;

	submit_buffer(tdrv);
#else
	WAIT_THINK2D(tdrv);
	T2D_SETREG(tdrv, T2D_BLIT_SRCADDR, srcAddr);
	T2D_SETREG(tdrv, T2D_BLIT_SRCRESOL, T2D_XYTOREG(rect->w, rect->h));
	T2D_SETREG(tdrv, T2D_BLIT_DSTADDR, T2D_XYTOREG(dx, dy));
	T2D_SETREG(tdrv, T2D_BLIT_CMD, (color_alpha << 28) | (src_colorkey << 27) | (rotation & 0x07) << 20 | tdev->src_format);
#endif

	return true;
}

/**
 * DirectFB callback that does a stretched blit.
 *
 * We use information saved on t2d_set_src() to also set the
 * source address register. See t2d_set_src(). We also
 * compute the scaling coefficients in the fixed point
 * format expected by the hardware.
 *
 * @param drv Driver data
 * @param dev Device data
 * @param srect Source rectangle
 * @param drect Destination rectangle
 */
static bool
Think2D_StretchBlit( void         *drv,
                     void         *dev,
		     DFBRectangle *srect,
		     DFBRectangle *drect )
{
	Think2D_DriverData *tdrv = (Think2D_DriverData *)drv;
	Think2D_DeviceData *tdev = (Think2D_DeviceData *)dev;

	DEBUG_PROC_ENTRY;
	DEBUG_OP("StretchBlit (%d, %d), %dx%d\n", drect->x, drect->y, drect->w, drect->h);

	int rotation = 0;
	int src_colorkey = 0;
	int color_alpha = 0;
	int scale_x = T2D_NOSCALE, scale_xfn = (1 << T2D_SCALER_BASE_SHIFT);
	int scale_y = T2D_NOSCALE, scale_yfn = (1 << T2D_SCALER_BASE_SHIFT);
	int dx = drect->x, dy = drect->y;
	unsigned long srcAddr = tdev->v_srcAddr + srect->x * T2D_modesize(tdev->src_format) + srect->y * tdev->v_srcStride;

	stat_blit++;

	/* will we do rotation? */
	if (tdev->blittingflags & DSBLIT_ROTATE90)
		rotation = T2D_DEG090, dy += srect->h - 1;
	else if (tdev->blittingflags & DSBLIT_ROTATE180)
		rotation = T2D_DEG180, dx += srect->w - 1, dy += srect->h - 1;
	else if (tdev->blittingflags & DSBLIT_ROTATE270)
		rotation = T2D_DEG270, dx += srect->h - 1;
	else if (tdev->blittingflags & DSBLIT_FLIP_HORIZONTAL)
		rotation = T2D_MIR_X, dx += srect->w - 1;
	else if (tdev->blittingflags & DSBLIT_FLIP_VERTICAL)
		rotation = T2D_MIR_Y, dy += srect->h - 1;

	/* will we do SRC_COLORKEY? */
	if (tdev->blittingflags & DSBLIT_SRC_COLORKEY)
		src_colorkey = 1;

	/* will we do COLORALPHA? */
	if (tdev->blittingflags & DSBLIT_BLEND_COLORALPHA)
		color_alpha = 1;

	/* compute the scaling coefficients for X */
	if (drect->w > srect->w)
		scale_x = T2D_UPSCALE, scale_xfn = (srect->w << T2D_SCALER_BASE_SHIFT) / drect->w;
	else if (drect->w < srect->w)
		scale_x = T2D_DNSCALE, scale_xfn = (drect->w << T2D_SCALER_BASE_SHIFT) / srect->w;

	/* compute the scaling coefficients for Y */
	if (drect->h > srect->h)
		scale_y = T2D_UPSCALE, scale_yfn = (srect->h << T2D_SCALER_BASE_SHIFT) / drect->h;
	else if (drect->h < srect->h)
		scale_y = T2D_DNSCALE, scale_yfn = (drect->h << T2D_SCALER_BASE_SHIFT) / srect->h;

#ifdef USE_CLIST
	start_buffer(tdrv, 14);

	(*p_buffer)[ 0] = T2D_BLIT_SRCADDR;
	(*p_buffer)[ 1] = srcAddr;
	(*p_buffer)[ 2] = T2D_BLIT_SRCRESOL;
	(*p_buffer)[ 3] = T2D_XYTOREG(srect->w, srect->h);
	(*p_buffer)[ 4] = T2D_BLIT_DSTADDR;
	(*p_buffer)[ 5] = T2D_XYTOREG(dx, dy);
	(*p_buffer)[ 6] = T2D_BLIT_DSTYXSIZE;
	(*p_buffer)[ 7] = T2D_XYTOREG(drect->w, drect->h);
	(*p_buffer)[ 8] = T2D_BLIT_SCALE_XFN;
	(*p_buffer)[ 9] = scale_xfn;
	(*p_buffer)[10] = T2D_BLIT_SCALE_YFN;
	(*p_buffer)[11] = scale_yfn;
	(*p_buffer)[12] = T2D_CMDLIST_WAIT | T2D_BLIT_CMD;
	(*p_buffer)[13] = (color_alpha << 28) | (src_colorkey << 27) | (scale_y << 25) | (scale_x << 23) | (rotation & 0x07) << 20 | tdev->src_format;

	submit_buffer(tdrv);
#else
	WAIT_THINK2D(tdrv);
	T2D_SETREG(tdrv, T2D_BLIT_SRCADDR, srcAddr);
	T2D_SETREG(tdrv, T2D_BLIT_SRCRESOL, T2D_XYTOREG(srect->w, srect->h));
	T2D_SETREG(tdrv, T2D_BLIT_DSTADDR, T2D_XYTOREG(dx, dy));
	T2D_SETREG(tdrv, T2D_BLIT_DSTYXSIZE, T2D_XYTOREG(drect->w, drect->h));
	T2D_SETREG(tdrv, T2D_BLIT_SCALE_XFN, scale_xfn);
	T2D_SETREG(tdrv, T2D_BLIT_SCALE_YFN, scale_yfn);
	T2D_SETREG(tdrv, T2D_BLIT_CMD, (color_alpha << 28) | (src_colorkey << 27) | (scale_y << 25) | (scale_x << 23) | (rotation & 0x07) << 20 | tdev->src_format);
#endif
	return true;
}

/**
 * DirectFB callback that draws a rectangle.
 *
 * Drawing the rectangle is implemented as drawing 4 lines
 * in driver level.
 *
 * @param drv Driver data
 * @param dev Device data
 * @param rect Rectangle coordinates
 */
static bool
Think2D_DrawRectangle( void         *drv,
                       void         *dev,
		       DFBRectangle *rect )
{
	Think2D_DriverData *tdrv = (Think2D_DriverData*) drv;
	Think2D_DeviceData *tdev = (Think2D_DeviceData*) dev;

	DEBUG_PROC_ENTRY;
	DEBUG_OP("DrawRectangle (%s) (%d, %d), %dx%d\n", tdev->drawingflags & DSDRAW_BLEND ? "A" : "", rect->x, rect->y, rect->w, rect->h);

	int x1 = rect->x;
	int y1 = rect->y;
	int x2 = rect->x + rect->w - 1;
	int y2 = rect->y + rect->h - 1;

	stat_lines += 4;

#ifdef USE_CLIST
	start_buffer(tdrv, 24);

	(*p_buffer)[ 0] = T2D_DRAW_STARTXY;
	(*p_buffer)[ 1] = T2D_XYTOREG(x1, y1);
	(*p_buffer)[ 2] = T2D_DRAW_ENDXY;
	(*p_buffer)[ 3] = T2D_XYTOREG(x1, y2);
	(*p_buffer)[ 4] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
	(*p_buffer)[ 5] = 0x01;
	(*p_buffer)[ 6] = T2D_DRAW_STARTXY;
	(*p_buffer)[ 7] = T2D_XYTOREG(x1, y2);
	(*p_buffer)[ 8] = T2D_DRAW_ENDXY;
	(*p_buffer)[ 9] = T2D_XYTOREG(x2, y2);
	(*p_buffer)[10] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
	(*p_buffer)[11] = 0x01;
	(*p_buffer)[12] = T2D_DRAW_STARTXY;
	(*p_buffer)[13] = T2D_XYTOREG(x1, y1);
	(*p_buffer)[14] = T2D_DRAW_ENDXY;
	(*p_buffer)[15] = T2D_XYTOREG(x2, y1);
	(*p_buffer)[16] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
	(*p_buffer)[17] = 0x01;
	(*p_buffer)[18] = T2D_DRAW_STARTXY;
	(*p_buffer)[19] = T2D_XYTOREG(x2, y1);
	(*p_buffer)[20] = T2D_DRAW_ENDXY;
	(*p_buffer)[21] = T2D_XYTOREG(x2, y2);
	(*p_buffer)[22] = T2D_CMDLIST_WAIT | T2D_DRAW_CMD;
	(*p_buffer)[23] = 0x01;

	submit_buffer(tdrv);
#else
	WAIT_THINK2D(tdrv);
	T2D_SETREG(tdrv, T2D_DRAW_STARTXY, T2D_XYTOREG(x1, y1));
	T2D_SETREG(tdrv, T2D_DRAW_ENDXY, T2D_XYTOREG(x1, y2));
	T2D_SETREG(tdrv, T2D_DRAW_CMD, 0x01);

	WAIT_THINK2D(tdrv);
	T2D_SETREG(tdrv, T2D_DRAW_STARTXY, T2D_XYTOREG(x1, y2));
	T2D_SETREG(tdrv, T2D_DRAW_ENDXY, T2D_XYTOREG(x2, y2));
	T2D_SETREG(tdrv, T2D_DRAW_CMD, 0x01);

	WAIT_THINK2D(tdrv);
	T2D_SETREG(tdrv, T2D_DRAW_STARTXY, T2D_XYTOREG(x1, y1));
	T2D_SETREG(tdrv, T2D_DRAW_ENDXY, T2D_XYTOREG(x2, y1));
	T2D_SETREG(tdrv, T2D_DRAW_CMD, 0x01);

	WAIT_THINK2D(tdrv);
	T2D_SETREG(tdrv, T2D_DRAW_STARTXY, T2D_XYTOREG(x2, y1));
	T2D_SETREG(tdrv, T2D_DRAW_ENDXY, T2D_XYTOREG(x2, y2));
	T2D_SETREG(tdrv, T2D_DRAW_CMD, 0x01);
#endif

	return true;
}

/**
 * DirectFB callback that resets the hardware.
 *
 * We try to reset the hardware a number of times. We also invalidate
 * DirectFB's saved state because we have also reset all registers,
 * including the destination registers.
 *
 * @param drv Driver data
 * @param dev Device data
 */
static void
Think2D_EngineReset( void *drv,
                     void *dev )
{
	Think2D_DriverData *tdrv = (Think2D_DriverData*) drv;
	Think2D_DeviceData *tdev = (Think2D_DeviceData*) dev;

	DEBUG_PROC_ENTRY;

	int timeout = 5;

	do
	{
		/* try to reset the device */
		T2D_SETREG(tdrv, T2D_STATUS, (1 << 31));
	}
	while (--timeout && T2D_GETREG(tdrv, T2D_STATUS));

	if (!timeout)
		/* print an error message if unsuccessful */
		D_ERROR("Think2D: Failed to reset the device!\n");
	else
		/* invalidate DirectFB's saved state if successful; this is because all device registers were wiped out */
		dfb_gfxcard_invalidate_state();
}

#ifdef USE_CLIST
/**
 * Waits until hardware finishes processing current command list.
 *
 * Only compiled if building with command list support.
 *
 * @param tdrv Driver data
 */
static DFBResult
Think2D_EngineNext( Think2D_DriverData *tdrv )
{
	DFBResult ret = DFB_OK;

	DEBUG_PROC_ENTRY;

	/* check if hardware is busy and possibly wait */
	while (T2D_GETREG(tdrv, T2D_STATUS) && ioctl(tdrv->gfx_fd, T2D_WAIT_IDLE) < 0)
	{
		if (errno == EINTR)
			continue;

		ret = errno2result(errno);

		D_ERROR("Think2D: EngineNext lockup! (status:0x%08x, hw_running:%u, sw_ready:%u)\n", T2D_GETREG(tdrv, T2D_STATUS), tdrv->gfx_shared->hw_running, tdrv->gfx_shared->sw_ready);
		dump_registers(tdrv);

		/* we should reset the device, it seems to be stuck */
		Think2D_EngineReset(tdrv, NULL);

		break;
	}

	/* the kernel component may not have had the chance to reset hw_running, so we do it here */
	tdrv->gfx_shared->hw_running = T2D_GETREG(tdrv, T2D_STATUS) != 0;

	if (ret == DFB_OK)
	{
		D_ASSERT(!tdrv->gfx_shared->hw_running);
		D_ASSERT(!tdrv->gfx_shared->sw_ready);
	}

	return ret;
}
#endif

/**
 * DirectFB callback that blocks until hardware processing has finished.
 *
 * If building with command list support, Think2D_EngineSync() functions
 * just as Think2D_EngineNext() that also restarts the hardware as long
 * as there are unprocessed operations. Otherwise, we simply spin on the
 * hardware status.
 *
 * @param drv Driver data
 * @param dev Device data
 */
static DFBResult
Think2D_EngineSync( void *drv,
                    void *dev )
{
	Think2D_DriverData *tdrv = (Think2D_DriverData*) drv;
	Think2D_DeviceData *tdev = (Think2D_DeviceData*) dev;
	DFBResult ret = DFB_OK;

	DEBUG_PROC_ENTRY;
#ifdef USE_CLIST
restart:
	/* check if hardware is busy and possibly wait */
	while (T2D_GETREG(tdrv, T2D_STATUS) && ioctl(tdrv->gfx_fd, T2D_WAIT_IDLE) < 0)
	{
		if (errno == EINTR)
			continue;

		ret = errno2result(errno);

		D_ERROR("Think2D: EngineSync lockup! (status:0x%08x, hw_running:%u, sw_ready:%u)\n", T2D_GETREG(tdrv, T2D_STATUS), tdrv->gfx_shared->hw_running, tdrv->gfx_shared->sw_ready);
		dump_registers(tdrv);

		/* we should reset the device, it seems to be stuck */
		Think2D_EngineReset(tdrv, tdev);

		break;
	}

	/* the kernel component may not have had the chance to reset hw_running, so we do it here */
	tdrv->gfx_shared->hw_running = T2D_GETREG(tdrv, T2D_STATUS) != 0;

	if (tdrv->gfx_shared->sw_ready)
	{
		/* in case there are unprocessed commands, start the hardware and wait again */
		flush_prepared(tdrv);
		goto restart;
	}

	if (ret == DFB_OK)
	{
		D_ASSERT(!tdrv->gfx_shared->hw_running);
		D_ASSERT(!tdrv->gfx_shared->sw_ready);
	}
#else
	WAIT_THINK2D(tdrv);
#endif
	return ret;
}

/**
 * DirectFB driver framework function that probes a device for use with a driver.
 *
 * @param device Graphics device
 */
static int
driver_probe( CoreGraphicsDevice *device )
{
	DEBUG_PROC_ENTRY;
	return dfb_gfxcard_get_accelerator( device ) == TSI_ACCELERATOR;
}

/**
 * DirectFB driver framework function that provides info about a driver.
 *
 * @param device Graphics device
 * @param info Driver info
 */
static void
driver_get_info( CoreGraphicsDevice *device,
                 GraphicsDriverInfo *info )
{
	DEBUG_PROC_ENTRY;

	snprintf( info->name,    DFB_GRAPHICS_DRIVER_INFO_NAME_LENGTH,    "TSi Think2D" );
	snprintf( info->vendor,  DFB_GRAPHICS_DRIVER_INFO_VENDOR_LENGTH,  "Think Silicon Ltd." );
	snprintf( info->url,     DFB_GRAPHICS_DRIVER_INFO_URL_LENGTH,     "http://www.think-silicon.com" );
	snprintf( info->license, DFB_GRAPHICS_DRIVER_INFO_LICENSE_LENGTH, "LGPL" );

	info->version.major = 0;
	info->version.minor = 1;

	info->driver_data_size = sizeof(Think2D_DriverData);
	info->device_data_size = sizeof(Think2D_DeviceData);
}

/**
 * DirectFB driver framework function that initializes a driver.
 *
 * @param device Graphics device
 * @param funcs Device functions
 * @param driver_data Driver data
 * @param device_data Device data
 * @param core DirectFB core object
 */
static DFBResult
driver_init_driver( CoreGraphicsDevice  *device,
                    GraphicsDeviceFuncs *funcs,
                    void                *driver_data,
                    void                *device_data,
                    CoreDFB             *core )
{
	DFBResult ret;

	DEBUG_PROC_ENTRY;

	Think2D_DriverData *tdrv = (Think2D_DriverData*) driver_data;

	/* first of all open the Think2D device; for this to work:
	 *  - the think2d module should have been build in the kernel or loaded
	 *  - a device node must have been created as /dev/t2d; for device major and minor numbers see think2d.c */
	tdrv->gfx_fd = direct_try_open("/dev/think2d", NULL, O_RDWR, true);
	if (tdrv->gfx_fd < 0) {
		D_PERROR("Think2D: Cannot open '/dev/think2d'!\n");
		return DFB_INIT;
	}

	/* ask from the kernel to map the shared data block to our address space; it consists of two parts:
	 *  - the shared data structure that enables communication among the hardware, the kernel driver and the DirectFB driver, used for command lists
	 *  - the Think2D register file
	 *
	 * memory length should be specified exactly or the kernel driver will reject the request */
	tdrv->gfx_shared = mmap(NULL, direct_page_align(sizeof(struct think2d_shared)) + direct_page_align(T2D_REGFILE_SIZE), PROT_READ | PROT_WRITE, MAP_SHARED, tdrv->gfx_fd, 0);
	if (tdrv->gfx_shared == MAP_FAILED)
	{
		D_PERROR("Think2D: Failed to mmap area!\n");
		close(tdrv->gfx_fd);
		return DFB_INIT;
	}

#ifdef USE_CLIST
	/* resetting the shared data structure should precede all operations */
	memset((void *)tdrv->gfx_shared, 0, sizeof(struct think2d_shared));

	p_buffer = &tdrv->gfx_shared->sw_now;
#endif

	/* the base address of the register file is a known offset into the area the kernel mapped for us... */
	tdrv->mmio_base = (volatile u32 *)((unsigned long)tdrv->gfx_shared + direct_page_align(sizeof(struct think2d_shared)));

	/* confirm that we have a Think2D accelerator in place */
	if (T2D_GETREG(tdrv, T2D_IDREG) != T2D_MAGIC)
	{
		D_PERROR("Think2D: Accelerator not found!\n");
		munmap(tdrv->gfx_shared, direct_page_align(sizeof(struct think2d_shared)) + direct_page_align(T2D_REGFILE_SIZE));
		close(tdrv->gfx_fd);
		return DFB_INIT;
	}

	/* assign system functions */
	funcs->AfterSetVar       = NULL;
	funcs->EngineReset       = Think2D_EngineReset;
	funcs->EngineSync        = Think2D_EngineSync;
	funcs->InvalidateState   = NULL;
	funcs->FlushTextureCache = NULL;
	funcs->FlushReadCache    = NULL;
	funcs->SurfaceEnter      = NULL;
	funcs->SurfaceLeave      = NULL;
	funcs->GetSerial         = NULL;
	funcs->WaitSerial        = NULL;
#ifdef USE_CLIST
	funcs->EmitCommands      = Think2D_EmitCommands;
#else
	funcs->EmitCommands      = NULL;
#endif
	funcs->CheckState        = Think2D_CheckState;
	funcs->SetState          = Think2D_SetState;

	/* assign drawing functions */
	funcs->FillRectangle     = Think2D_FillRectangle;
	funcs->DrawRectangle     = Think2D_DrawRectangle;
	funcs->DrawLine          = Think2D_DrawLine;
	funcs->FillTriangle      = NULL;

	/* assign blitting functions */
	funcs->Blit              = Think2D_Blit;
	funcs->StretchBlit       = Think2D_StretchBlit;
	funcs->TextureTriangles  = NULL;

	/* assign other functions */
	funcs->StartDrawing      = NULL;
	funcs->StopDrawing       = NULL;

	return DFB_OK;
}

/**
 * DirectFB driver framework function that initializes a device for use with a driver.
 *
 * @param device Graphics device
 * @param device_info Graphics device info
 * @param drv Driver data
 * @param dev Device data
 */
static DFBResult
driver_init_device( CoreGraphicsDevice *device,
                    GraphicsDeviceInfo *device_info,
                    void               *drv,
                    void               *dev )
{
	Think2D_DriverData *tdrv = (Think2D_DriverData*) drv;
	Think2D_DeviceData *tdev = (Think2D_DeviceData*) dev;

	DEBUG_PROC_ENTRY;

	Think2D_EngineReset(tdrv, tdev);

	snprintf( device_info->name,   DFB_GRAPHICS_DEVICE_INFO_NAME_LENGTH,   "Think2D" );
	snprintf( device_info->vendor, DFB_GRAPHICS_DEVICE_INFO_VENDOR_LENGTH, "Think Silicon" );

	device_info->caps.flags    = CCF_CLIPPING;
	device_info->caps.accel    = T2D_SUPPORTED_DRAWINGFUNCTIONS | T2D_SUPPORTED_BLITTINGFUNCTIONS;
	device_info->caps.drawing  = T2D_SUPPORTED_DRAWINGFLAGS;
	device_info->caps.blitting = T2D_SUPPORTED_BLITTINGFLAGS;

	device_info->limits.surface_byteoffset_alignment = 4;
	device_info->limits.surface_bytepitch_alignment = 4;

	return DFB_OK;
}

/**
 * DirectFB driver framework function that closes a device.
 *
 * @param device Graphics device
 * @param driver_data Driver data
 * @param device_data Device data
 */
static void
driver_close_device( CoreGraphicsDevice *device,
		     void               *driver_data,
		     void               *device_data )
{
	Think2D_DriverData *tdrv = (Think2D_DriverData*) driver_data;

	DEBUG_PROC_ENTRY;

	D_DEBUG("Think2D: driver_close_device (Statistics)\n");
	D_DEBUG("-----------------------------------------\n");
	D_DEBUG("Fill Rects: %u\n", stat_fillrect);
	D_DEBUG("Draw Lines: %u\n", stat_lines);
	D_DEBUG("Blits     : %u\n", stat_blit);
	D_DEBUG("Starts    : %u\n", tdrv->gfx_shared->num_starts);
	D_DEBUG("Interrupts: %u\n", tdrv->gfx_shared->num_interrupts);
	D_DEBUG("Waits     : %u\n", tdrv->gfx_shared->num_waits);
	D_DEBUG("Nice to see you!\n");
}

/**
 * DirectFB driver framework function that closes a driver.
 *
 * @param device Graphics device
 * @param driver_data Driver data
 */
static void
driver_close_driver( CoreGraphicsDevice *device,
                     void               *driver_data )
{
	Think2D_DriverData *tdrv = (Think2D_DriverData*) driver_data;

	DEBUG_PROC_ENTRY;

	/* undo all operations performed by driver_init_driver() */
	dfb_gfxcard_unmap_mmio(device, tdrv->mmio_base, -1);
	munmap(tdrv->gfx_shared, direct_page_align(sizeof(struct think2d_shared)) + direct_page_align(T2D_REGFILE_SIZE));
	close(tdrv->gfx_fd);
}
