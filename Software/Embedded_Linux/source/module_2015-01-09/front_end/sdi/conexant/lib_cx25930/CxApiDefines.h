/*+++ *******************************************************************\
*
*  Copyright and Disclaimer:
*
*     ---------------------------------------------------------------
*     This software is provided "AS IS" without warranty of any kind,
*     either expressed or implied, including but not limited to the
*     implied warranties of noninfringement, merchantability and/or
*     fitness for a particular purpose.
*     ---------------------------------------------------------------
*
*     Copyright (c) 2013 Conexant Systems, Inc.
*     All rights reserved.
*
\******************************************************************* ---*/

#ifndef __CX_API_DEFINES_H__
#define __CX_API_DEFINES_H__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

// defines for video standards
#define NTSC_VIDEO_STD          0
#define PAL_VIDEO_STD           1
#define NTSC_EFFIO_STD          2
#define PAL_EFFIO_STD           3


#define SetBit(Bit)  (1 << Bit)

static inline unsigned char getBit(unsigned long sample, unsigned char index)
{
    return (unsigned char) ((sample >> index) & 1);
}

static inline unsigned long clearBitAtPos(unsigned long value, unsigned char bit)
{
    return value & ~(1 << bit);
}

static inline unsigned long setBitAtPos(unsigned long sample, unsigned char bit)
{
    sample |= (1 << bit);
    return sample;

}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CX_API_DEFINES_H__ */
