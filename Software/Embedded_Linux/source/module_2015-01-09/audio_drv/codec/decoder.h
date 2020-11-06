/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: decoder.h,v 1.2 2013/10/20 15:31:25 easonli Exp $
**/

#ifndef __DECODER_H__
#define __DECODER_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
  #pragma pack(push, 8)
  #ifndef FAADAPI
    #define FAADAPI __cdecl
  #endif
#else
  #ifndef FAADAPI
    #define FAADAPI
  #endif
#endif

#include "bits.h"
#include "syntax.h"
#include "drc.h"
#include "specrec.h"
#include "filtbank.h"
#include <linux/export.h>


/* library output formats */
#define FAAD_FMT_16BIT  1
#define FAAD_FMT_24BIT  2
#define FAAD_FMT_32BIT  3
#define FAAD_FMT_FLOAT  4
#define FAAD_FMT_DOUBLE 5

#define LC_DEC_CAP            (1<<0)
#define LTP_DEC_CAP           (1<<2)
#define FIXED_POINT_CAP       (1<<5)

#define FRONT_CHANNEL_CENTER (1)
#define FRONT_CHANNEL_LEFT   (2)
#define FRONT_CHANNEL_RIGHT  (3)
#define SIDE_CHANNEL_LEFT    (4)
#define SIDE_CHANNEL_RIGHT   (5)
#define BACK_CHANNEL_LEFT    (6)
#define BACK_CHANNEL_RIGHT   (7)
#define BACK_CHANNEL_CENTER  (8)
#define LFE_CHANNEL          (9)
#define UNKNOWN_CHANNEL      (0)

signed char* FAADAPI faacDecGetErrorMessage(char errcode);

unsigned int FAADAPI faacDecGetCapabilities(void);

GMAACDecHandle FAADAPI faacDecOpen(void);

faacDecConfigurationPtr FAADAPI faacDecGetCurrentConfiguration(GMAACDecHandle hDecoder);

char FAADAPI faacDecSetConfiguration(GMAACDecHandle hDecoder,
                                    faacDecConfigurationPtr config);

/* Init the library based on info from the AAC file (ADTS/ADIF) */
int FAADAPI faacDecInit(GMAACDecHandle hDecoder,
                            char *buffer,
                            unsigned int buffer_size,
                            unsigned int *samplerate,
                            char *channels);

/* Init the library using a DecoderSpecificInfo */
//int FAADAPI faacDecInit2(GMAACDecHandle hDecoder, char *pBuffer,
//                         unsigned int SizeOfDecoderSpecificInfo,
//                         unsigned int *samplerate, char *channels);

void FAADAPI faacDecClose(GMAACDecHandle hDecoder);

void FAADAPI faacDecPostSeekReset(GMAACDecHandle hDecoder, int frame);

//void* FAADAPI GMAACDecode(GMAACDecHandle hDecoder,
//                            GMAACDecInfo *hInfo,
//                            char *buffer,
//                            unsigned int buffer_size);

#ifdef _WIN32
  #pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif
#endif
