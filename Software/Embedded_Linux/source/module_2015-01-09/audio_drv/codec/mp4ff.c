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
** $Id: mp4ff.c,v 1.1 2013/09/04 06:26:55 easonli Exp $
**/

#include <stdlib.h>
#include <string.h>
#include "mp4ffint.h"

mp4ff_t *mp4ff_open_read(mp4ff_callback_t *f)
{
    mp4ff_t *ff = malloc(sizeof(mp4ff_t));

    memset(ff, 0, sizeof(mp4ff_t));

    ff->stream = f;

    parse_atoms(ff);

    return ff;
}

void mp4ff_close(mp4ff_t *ff)
{
    int i;

    for (i = 0; i < ff->total_tracks; i++)
    {
        if (ff->track[i])
        {
            if (ff->track[i]->stsz_table)
                free(ff->track[i]->stsz_table);
            if (ff->track[i]->stts_sample_count)
                free(ff->track[i]->stts_sample_count);
            if (ff->track[i]->stts_sample_delta)
                free(ff->track[i]->stts_sample_delta);
            if (ff->track[i]->stsc_first_chunk)
                free(ff->track[i]->stsc_first_chunk);
            if (ff->track[i]->stsc_samples_per_chunk)
                free(ff->track[i]->stsc_samples_per_chunk);
            if (ff->track[i]->stsc_sample_desc_index)
                free(ff->track[i]->stsc_sample_desc_index);
            if (ff->track[i]->stco_chunk_offset)
                free(ff->track[i]->stco_chunk_offset);
            if (ff->track[i]->decoderConfig)
                free(ff->track[i]->decoderConfig);
			if (ff->track[i]->ctts_sample_count)
				free(ff->track[i]->ctts_sample_count);
			if (ff->track[i]->ctts_sample_offset)
				free(ff->track[i]->ctts_sample_offset);
            free(ff->track[i]);
        }
    }

#ifdef USE_TAGGING
    mp4ff_tag_delete(&(ff->tags));
#endif

    if (ff) free(ff);
}

void mp4ff_track_add(mp4ff_t *f)
{
    f->total_tracks++;

    f->track[f->total_tracks - 1] = malloc(sizeof(mp4ff_track_t));

    memset(f->track[f->total_tracks - 1], 0, sizeof(mp4ff_track_t));
}

/* parse atoms that are sub atoms of other atoms */
int parse_sub_atoms(mp4ff_t *f, const unsigned long long total_size)
{
    unsigned long long size;
    unsigned char atom_type = 0;
    unsigned long long counted_size = 0;
    unsigned char header_size = 0;

    while (counted_size < total_size)
    {
        size = mp4ff_atom_read_header(f, &atom_type, &header_size);
        counted_size += size;

        /* check for end of file */
        if (size == 0)
            break;

        /* we're starting to read a new track, update index,
         * so that all data and tables get written in the right place
         */
        if (atom_type == ATOM_TRAK)
        {
            mp4ff_track_add(f);
        }

        /* parse subatoms */
        if (atom_type < SUBATOMIC)
        {
            parse_sub_atoms(f, size-header_size);
        } else {
            mp4ff_atom_read(f, (unsigned int)size, atom_type);
        }
    }

    return 0;
}

/* parse root atoms */
int parse_atoms(mp4ff_t *f)
{
    unsigned long long size;
    unsigned char atom_type = 0;
    unsigned char header_size = 0;

    f->file_size = 0;

    while ((size = mp4ff_atom_read_header(f, &atom_type, &header_size)) != 0)
    {
        f->file_size += size;
        f->last_atom = atom_type;

        if (atom_type == ATOM_MDAT && f->moov_read)
        {
            /* moov atom is before mdat, we can stop reading when mdat is encountered */
            /* file position will stay at beginning of mdat data */
//            break;
        }

        if (atom_type == ATOM_MOOV && size > header_size)
        {
            f->moov_read = 1;
            f->moov_offset = mp4ff_position(f)-header_size;
            f->moov_size = size;
        }

        /* parse subatoms */
        if (atom_type < SUBATOMIC)
        {
            parse_sub_atoms(f, size-header_size);
        } else {
            /* skip this atom */
            mp4ff_set_position(f, mp4ff_position(f)+size-header_size);
        }
    }

    return 0;
}

int mp4ff_get_decoder_config(const mp4ff_t *f, const int track,
                                 unsigned char** ppBuf, unsigned int* pBufSize)
{
    if (track >= f->total_tracks)
    {
        *ppBuf = NULL;
        *pBufSize = 0;
        return 1;
    }

    if (f->track[track]->decoderConfig == NULL || f->track[track]->decoderConfigLen == 0)
    {
        *ppBuf = NULL;
        *pBufSize = 0;
    } else {
        *ppBuf = malloc(f->track[track]->decoderConfigLen);
        if (*ppBuf == NULL)
        {
            *pBufSize = 0;
            return 1;
        }
        memcpy(*ppBuf, f->track[track]->decoderConfig, f->track[track]->decoderConfigLen);
        *pBufSize = f->track[track]->decoderConfigLen;
    }

    return 0;
}

int mp4ff_get_track_type(const mp4ff_t *f, const int track)
{
	return f->track[track]->type;
}

int mp4ff_total_tracks(const mp4ff_t *f)
{
    return f->total_tracks;
}

int mp4ff_time_scale(const mp4ff_t *f, const int track)
{
    return f->track[track]->timeScale;
}

unsigned int mp4ff_get_avg_bitrate(const mp4ff_t *f, const int track)
{
	return f->track[track]->avgBitrate;
}

unsigned int mp4ff_get_max_bitrate(const mp4ff_t *f, const int track)
{
	return f->track[track]->maxBitrate;
}

int64_t mp4ff_get_track_duration(const mp4ff_t *f, const int track)
{
	return f->track[track]->duration;
}

int64_t mp4ff_get_track_duration_use_offsets(const mp4ff_t *f, const int track)
{
	int64_t duration = mp4ff_get_track_duration(f,track);
	if (duration!=-1)
	{
		int64_t offset = mp4ff_get_sample_offset(f,track,0);
		if (offset > duration) duration = 0;
		else duration -= offset;
	}
	return duration;
}


int mp4ff_num_samples(const mp4ff_t *f, const int track)
{
    int i;
    int total = 0;

    for (i = 0; i < f->track[track]->stts_entry_count; i++)
    {
        total += f->track[track]->stts_sample_count[i];
    }
    return total;
}




unsigned int mp4ff_get_sample_rate(const mp4ff_t *f, const int track)
{
	return f->track[track]->sampleRate;
}

unsigned int mp4ff_get_channel_count(const mp4ff_t * f,const int track)
{
	return f->track[track]->channelCount;
}

unsigned int mp4ff_get_audio_type(const mp4ff_t * f,const int track)
{
	return f->track[track]->audioType;
}

int mp4ff_get_sample_duration_use_offsets(const mp4ff_t *f, const int track, const int sample)
{
	int d,o;
	d = mp4ff_get_sample_duration(f,track,sample);
	if (d!=-1)
	{
		o = mp4ff_get_sample_offset(f,track,sample);
		if (o>d) d = 0;
		else d -= o;
	}
	return d;
}

int mp4ff_get_sample_duration(const mp4ff_t *f, const int track, const int sample)
{
    int i, co = 0;

    for (i = 0; i < f->track[track]->stts_entry_count; i++)
    {
		int delta = f->track[track]->stts_sample_count[i];
		if (sample < co + delta)
			return f->track[track]->stts_sample_delta[i];
		co += delta;
    }
    return (int)(-1);
}

int64_t mp4ff_get_sample_position(const mp4ff_t *f, const int track, const int sample)
{
    int i, co = 0;
	int64_t acc = 0;

    for (i = 0; i < f->track[track]->stts_entry_count; i++)
    {
		int delta = f->track[track]->stts_sample_count[i];
		if (sample < co + delta)
		{
			acc += f->track[track]->stts_sample_delta[i] * (sample - co);
			return acc;
		}
		else
		{
			acc += f->track[track]->stts_sample_delta[i] * delta;
		}
		co += delta;
    }
    return (int64_t)(-1);
}

int mp4ff_get_sample_offset(const mp4ff_t *f, const int track, const int sample)
{
    int i, co = 0;

    for (i = 0; i < f->track[track]->ctts_entry_count; i++)
    {
		int delta = f->track[track]->ctts_sample_count[i];
		if (sample < co + delta)
			return f->track[track]->ctts_sample_offset[i];
		co += delta;
    }
    return 0;
}

int mp4ff_find_sample(const mp4ff_t *f, const int track, const int64_t offset,int * toskip)
{
	int i, co = 0;
	int64_t offset_total = 0;
	mp4ff_track_t * p_track = f->track[track];

	for (i = 0; i < p_track->stts_entry_count; i++)
	{
		int sample_count = p_track->stts_sample_count[i];
		int sample_delta = p_track->stts_sample_delta[i];
		int64_t offset_delta = (int64_t)sample_delta * (int64_t)sample_count;
		if (offset < offset_total + offset_delta)
		{
			int64_t offset_fromstts = offset - offset_total;
			if (toskip) *toskip = (int)(offset_fromstts % sample_delta);
			return co + (int)(offset_fromstts / sample_delta);
		}
		else
		{
			offset_total += offset_delta;
		}
		co += sample_count;
	}
	return (int)(-1);
}

int mp4ff_find_sample_use_offsets(const mp4ff_t *f, const int track, const int64_t offset,int * toskip)
{
	return mp4ff_find_sample(f,track,offset + mp4ff_get_sample_offset(f,track,0),toskip);
}

int mp4ff_read_sample(mp4ff_t *f, const int track, const int sample,
                          unsigned char **audio_buffer,  unsigned int *bytes)
{
    int result = 0;

    *bytes = mp4ff_audio_frame_size(f, track, sample);

	if (*bytes==0) return 0;

    *audio_buffer = (unsigned char*)malloc(*bytes);

    mp4ff_set_sample_position(f, track, sample);

    result = mp4ff_read_data(f, (signed char *)*audio_buffer, *bytes);

    if (!result)
	{
		free(*audio_buffer);
		*audio_buffer = 0;
        return 0;
	}

    return *bytes;
}


int mp4ff_read_sample_v2(mp4ff_t *f, const int track, const int sample,unsigned char *buffer)
{
    int result = 0;
	int size = mp4ff_audio_frame_size(f,track,sample);
	if (size<=0) return 0;
	mp4ff_set_sample_position(f, track, sample);
	result = mp4ff_read_data(f,(signed char *)buffer,size);

    return result;
}

int mp4ff_read_sample_getsize(mp4ff_t *f, const int track, const int sample)
{
	int temp = mp4ff_audio_frame_size(f, track, sample);
	if (temp<0) temp = 0;
	return temp;
}
