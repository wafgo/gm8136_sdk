/*
***********************************************************************************
** FARADAY(MTD)/GRAIN CONFIDENTIAL
** Copyright FARADAY(MTD)/GRAIN Corporation All Rights Reserved.
**
** The source code contained or described herein and all documents related to
** the source code (Material) are owned by FARADAY(MTD)/GRAIN Corporation Title to the 
** Material remains with FARADAY(MTD)/GRAIN Corporation. The Material contains trade 
** secrets and proprietary and confidential information of FARADAY(MTD)/GRAIN. The 
** Material is protected by worldwide copyright and trade secret laws and treaty 
** provisions. No part of the Material may be used, copied, reproduced, modified, 
** published, uploaded, posted, transmitted, distributed, or disclosed in any way 
** without FARADAY(MTD)/GRAIN prior express written permission.
** 
** No license under any patent, copyright, trade secret or other intellectual 
** property right is granted to or conferred upon you by disclosure or delivery
** of the Materials, either expressly, by implication, inducement, estoppel or 
** otherwise. Any license under such intellectual property rights must be express
** and approved by FARADAY(MTD)/GRAIN in writing.
**
** 
***********************************************************************/


/*******************************************************************************
	File:		perceptual_const.h

	Content:	Global psychoacoustic constants structures

*******************************************************************************/

#ifndef _perceptual_const_h
#define _perceptual_const_h

#include "config.h"

#define TRUE  1
#define FALSE 0

#define FRAME_LEN_LONG    AACENC_BLOCKSIZE
#define TRANS_FAC         8
#define FRAME_LEN_SHORT   (FRAME_LEN_LONG/TRANS_FAC) //1024/8=128



/* Block types */
enum
{
  LONG_WINDOW = 0,
  START_WINDOW,
  SHORT_WINDOW,
  STOP_WINDOW
};

/* Window shapes */
enum
{
  SINE_WINDOW = 0,
  KBD_WINDOW  = 1
};

/*
  MS stuff
*/
enum
{
  SI_MS_MASK_NONE = 0,
  SI_MS_MASK_SOME = 1,
  SI_MS_MASK_ALL  = 2
};

#define MAX_NO_OF_GROUPS 4
#define MAX_SFB_SHORT   15  /* 15 for a memory optimized implementation, maybe 16 for convenient debugging */
#define MAX_SFB_LONG    51  /* 51 for a memory optimized implementation, maybe 64 for convenient debugging */
#define MAX_SFB         (MAX_SFB_SHORT > MAX_SFB_LONG ? MAX_SFB_SHORT : MAX_SFB_LONG)   /* = MAX_SFB_LONG */
#define MAX_GROUPED_SFB (MAX_NO_OF_GROUPS*MAX_SFB_SHORT > MAX_SFB_LONG ? \
                         MAX_NO_OF_GROUPS*MAX_SFB_SHORT : MAX_SFB_LONG)

#define BLOCK_SWITCHING_OFFSET		   (1*1024+3*128+64+128)
#define BLOCK_SWITCHING_DATA_SIZE          FRAME_LEN_LONG

#define TRANSFORM_OFFSET_LONG    0
#define TRANSFORM_OFFSET_SHORT   448

#define LOG_NORM_PCM          -15

#define NUM_SAMPLE_RATES	12

#endif /* _perceptual_const_h */
