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
** $Id: mp4ff.h,v 1.1 2013/09/04 06:27:26 easonli Exp $
**/

#ifndef MP4FF_H
#define MP4FF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mp4ff_int_types.h"

/* file callback structure */
typedef struct
{
    unsigned int (*read)(void *user_data, void *buffer, unsigned int length);
    unsigned int (*write)(void *udata, void *buffer, unsigned int length);
    unsigned int (*seek)(void *user_data, unsigned long long position);
	unsigned int (*truncate)(void *user_data);
    void *user_data;
} mp4ff_callback_t;

/* mp4 main file structure */
typedef void* mp4ff_t;


/* API */

mp4ff_t *mp4ff_open_read(mp4ff_callback_t *f);
void mp4ff_close(mp4ff_t *f);
int mp4ff_get_sample_duration(const mp4ff_t *f, const int track, const int sample);
int mp4ff_get_sample_duration_use_offsets(const mp4ff_t *f, const int track, const int sample);
int64_t mp4ff_get_sample_position(const mp4ff_t *f, const int track, const int sample);
int mp4ff_get_sample_offset(const mp4ff_t *f, const int track, const int sample);
int mp4ff_find_sample(const mp4ff_t *f, const int track, const int64_t offset,int * toskip);
int mp4ff_find_sample_use_offsets(const mp4ff_t *f, const int track, const int64_t offset,int * toskip);

int mp4ff_read_sample(mp4ff_t *f, const int track, const int sample,
                          unsigned char **audio_buffer,  unsigned int *bytes);

int mp4ff_read_sample_v2(mp4ff_t *f, const int track, const int sample,unsigned char *buffer);//returns 0 on error, number of bytes read on success, use mp4ff_read_sample_getsize() to check buffer size needed
int mp4ff_read_sample_getsize(mp4ff_t *f, const int track, const int sample);//returns 0 on error, buffer size needed for mp4ff_read_sample_v2() on success



int mp4ff_get_decoder_config(const mp4ff_t *f, const int track,
                             unsigned char** ppBuf, unsigned int* pBufSize);
int mp4ff_get_track_type(const mp4ff_t *f, const int track);
int mp4ff_total_tracks(const mp4ff_t *f);
int mp4ff_num_samples(const mp4ff_t *f, const int track);
int mp4ff_time_scale(const mp4ff_t *f, const int track);

unsigned int mp4ff_get_avg_bitrate(const mp4ff_t *f, const int track);
unsigned int mp4ff_get_max_bitrate(const mp4ff_t *f, const int track);
int64_t mp4ff_get_track_duration(const mp4ff_t *f, const int track); //returns (-1) if unknown
int64_t mp4ff_get_track_duration_use_offsets(const mp4ff_t *f, const int track); //returns (-1) if unknown
unsigned int mp4ff_get_sample_rate(const mp4ff_t *f, const int track);
unsigned int mp4ff_get_channel_count(const mp4ff_t * f,const int track);
unsigned int mp4ff_get_audio_type(const mp4ff_t * f,const int track);


/* metadata */
int mp4ff_meta_get_num_items(const mp4ff_t *f);
int mp4ff_meta_get_by_index(const mp4ff_t *f, unsigned int index,
                            char **item, char **value);
int mp4ff_meta_get_title(const mp4ff_t *f, char **value);
int mp4ff_meta_get_artist(const mp4ff_t *f, char **value);
int mp4ff_meta_get_writer(const mp4ff_t *f, char **value);
int mp4ff_meta_get_album(const mp4ff_t *f, char **value);
int mp4ff_meta_get_date(const mp4ff_t *f, char **value);
int mp4ff_meta_get_tool(const mp4ff_t *f, char **value);
int mp4ff_meta_get_comment(const mp4ff_t *f, char **value);
int mp4ff_meta_get_genre(const mp4ff_t *f, char **value);
int mp4ff_meta_get_track(const mp4ff_t *f, char **value);
int mp4ff_meta_get_disc(const mp4ff_t *f, char **value);
int mp4ff_meta_get_compilation(const mp4ff_t *f, char **value);
int mp4ff_meta_get_tempo(const mp4ff_t *f, char **value);
int mp4ff_meta_get_coverart(const mp4ff_t *f, char **value);
#ifdef USE_TAGGING

/* metadata tag structure */
typedef struct
{
    char *item;
    char *value;
} mp4ff_tag_t;

/* metadata list structure */
typedef struct
{
    mp4ff_tag_t *tags;
    unsigned int count;
} mp4ff_metadata_t;

int mp4ff_meta_update(mp4ff_callback_t *f,const mp4ff_metadata_t * data);

#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif