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
** $Id: mp4meta.c,v 1.1 2013/09/04 06:26:55 easonli Exp $
**/

#ifdef USE_TAGGING

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mp4ffint.h"

static int mp4ff_tag_add_field(mp4ff_metadata_t *tags, const char *item, const char *value)
{
    void *backup = (void *)tags->tags;

    if (!item || (item && !*item) || !value) return 0;

    tags->tags = (mp4ff_tag_t*)realloc(tags->tags, (tags->count+1) * sizeof(mp4ff_tag_t));
    if (!tags->tags)
    {
        if (backup) free(backup);
        return 0;
    } else {
        tags->tags[tags->count].item = strdup(item);
        tags->tags[tags->count].value = strdup(value);

        if (!tags->tags[tags->count].item || !tags->tags[tags->count].value)
        {
            if (!tags->tags[tags->count].item) free (tags->tags[tags->count].item);
            if (!tags->tags[tags->count].value) free (tags->tags[tags->count].value);
            tags->tags[tags->count].item = NULL;
            tags->tags[tags->count].value = NULL;
            return 0;
        }

        tags->count++;
        return 1;
    }
}

static int mp4ff_tag_set_field(mp4ff_metadata_t *tags, const char *item, const char *value)
{
    unsigned int i;

    if (!item || (item && !*item) || !value) return 0;

    for (i = 0; i < tags->count; i++)
    {
        if (!stricmp(tags->tags[i].item, item))
        {
			free(tags->tags[i].value);
			tags->tags[i].value = strdup(value);
            return 1;
        }
    }

    return mp4ff_tag_add_field(tags, item, value);
}

int mp4ff_tag_delete(mp4ff_metadata_t *tags)
{
    unsigned int i;

    for (i = 0; i < tags->count; i++)
    {
        if (tags->tags[i].item) free(tags->tags[i].item);
        if (tags->tags[i].value) free(tags->tags[i].value);
    }

    if (tags->tags) free(tags->tags);

    tags->tags = NULL;
    tags->count = 0;

    return 0;
}

static const char* ID3v1GenreList[] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
    "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",
    "Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
    "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
    "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
    "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
    "Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass",
    "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
    "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk",
    "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
    "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
    "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
    "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
    "Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing",
    "Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
    "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
    "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson",
    "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus",
    "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
    "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
    "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
    "Goa", "Drum & Bass", "Club House", "Hardcore", "Terror",
    "Indie", "BritPop", "NegerPunk", "Polsk Punk", "Beat",
    "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", "Contemporary C",
    "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
    "SynthPop",
};

unsigned int mp4ff_meta_genre_to_index(const char * genrestr)
{
	unsigned n;
	for(n=0;n<sizeof(ID3v1GenreList)/sizeof(ID3v1GenreList[0]);n++)
	{
		if (!stricmp(genrestr,ID3v1GenreList[n])) return n+1;
	}
	return 0;
}

const char * mp4ff_meta_index_to_genre(unsigned int idx)
{
	if (idx>0 && idx<=sizeof(ID3v1GenreList)/sizeof(ID3v1GenreList[0]))
	{
		return ID3v1GenreList[idx-1];
	}
	else
	{
		return 0;
	}
}


static int TrackToString(char** str, const unsigned short track, const unsigned short totalTracks)
{
	char temp[32];
    sprintf(temp, "%.5u of %.5u", track, totalTracks);
	*str = strdup(temp);
    return 0;
}

static int mp4ff_set_metadata_name(mp4ff_t *f, const unsigned char atom_type, char **name)
{
    static char *tag_names[] = {
        "unknown", "title", "artist", "writer", "album",
        "date", "tool", "comment", "genre", "track",
        "disc", "compilation", "genre", "tempo", "cover"
    };
    unsigned char tag_idx = 0;

    switch (atom_type)
    {
    case ATOM_TITLE: tag_idx = 1; break;
    case ATOM_ARTIST: tag_idx = 2; break;
    case ATOM_WRITER: tag_idx = 3; break;
    case ATOM_ALBUM: tag_idx = 4; break;
    case ATOM_DATE: tag_idx = 5; break;
    case ATOM_TOOL: tag_idx = 6; break;
    case ATOM_COMMENT: tag_idx = 7; break;
    case ATOM_GENRE1: tag_idx = 8; break;
    case ATOM_TRACK: tag_idx = 9; break;
    case ATOM_DISC: tag_idx = 10; break;
    case ATOM_COMPILATION: tag_idx = 11; break;
    case ATOM_GENRE2: tag_idx = 12; break;
    case ATOM_TEMPO: tag_idx = 13; break;
    case ATOM_COVER: tag_idx = 14; break;
    default: tag_idx = 0; break;
    }

	*name = strdup(tag_names[tag_idx]);

    return 0;
}

static int mp4ff_parse_tag(mp4ff_t *f, const unsigned char parent_atom_type, const int size)
{
    unsigned char atom_type;
    unsigned char header_size = 0;
    unsigned long long subsize, sumsize = 0;
    char * name = NULL;
	char * data = NULL;
	unsigned int done = 0;


    while (sumsize < size)
    {
		unsigned long long destpos;
        subsize = mp4ff_atom_read_header(f, &atom_type, &header_size);
		destpos = mp4ff_position(f)+subsize-header_size;
		if (!done)
		{
			if (atom_type == ATOM_DATA)
			{
				mp4ff_read_char(f); /* version */
				mp4ff_read_int24(f); /* flags */
				mp4ff_read_int32(f); /* reserved */

				/* some need special attention */
				if (parent_atom_type == ATOM_GENRE2 || parent_atom_type == ATOM_TEMPO)
				{
					if (subsize - header_size >= 8 + 2)
					{
						unsigned short val = mp4ff_read_int16(f);

						if (parent_atom_type == ATOM_TEMPO)
						{
							char temp[16];
							sprintf(temp, "%.5u BPM", val);
							mp4ff_tag_add_field(&(f->tags), "tempo", temp);
						}
						else
						{
							const char * temp = mp4ff_meta_index_to_genre(val);
							if (temp)
							{
								mp4ff_tag_add_field(&(f->tags), "genre", temp);
							}
						}
						done = 1;
					}
				} else if (parent_atom_type == ATOM_TRACK || parent_atom_type == ATOM_DISC) {
					if (!done && subsize - header_size >= 8 + 8)
					{
						unsigned short index,total;
						char temp[32];
						mp4ff_read_int16(f);
						index = mp4ff_read_int16(f);
						total = mp4ff_read_int16(f);
						mp4ff_read_int16(f);

						sprintf(temp,"%d",index);
						mp4ff_tag_add_field(&(f->tags), parent_atom_type == ATOM_TRACK ? "track" : "disc", temp);
						if (total>0)
						{
							sprintf(temp,"%d",total);
							mp4ff_tag_add_field(&(f->tags), parent_atom_type == ATOM_TRACK ? "totaltracks" : "totaldiscs", temp);
						}
						done = 1;
					}
				} else
				{
					if (data) {free(data);data = NULL;}
					data = mp4ff_read_string(f,(unsigned int)(subsize-(header_size+8)));
				}
			} else if (atom_type == ATOM_NAME) {
				if (!done)
				{
					mp4ff_read_char(f); /* version */
					mp4ff_read_int24(f); /* flags */
					if (name) free(name);
					name = mp4ff_read_string(f,(unsigned int)(subsize-(header_size+4)));
				}
			}
			mp4ff_set_position(f, destpos);
			sumsize += subsize;
		}
    }

	if (data)
	{
		if (!done)
		{
			if (name == NULL) mp4ff_set_metadata_name(f, parent_atom_type, &name);
			if (name) mp4ff_tag_add_field(&(f->tags), name, data);
		}

		free(data);
	}
	if (name) free(name);
    return 1;
}

int mp4ff_parse_metadata(mp4ff_t *f, const int size)
{
    unsigned long long subsize, sumsize = 0;
    unsigned char atom_type;
    unsigned char header_size = 0;

    while (sumsize < size)
    {
        subsize = mp4ff_atom_read_header(f, &atom_type, &header_size);
        mp4ff_parse_tag(f, atom_type, (unsigned int)(subsize-header_size));
        sumsize += subsize;
    }

    return 0;
}

/* find a metadata item by name */
/* returns 0 if item found, 1 if no such item */
static int mp4ff_meta_find_by_name(const mp4ff_t *f, const char *item, char **value)
{
    unsigned int i;

    for (i = 0; i < f->tags.count; i++)
    {
        if (!stricmp(f->tags.tags[i].item, item))
        {
			*value = strdup(f->tags.tags[i].value);
            return 1;
        }
    }

    *value = NULL;

    /* not found */
    return 0;
}

int mp4ff_meta_get_num_items(const mp4ff_t *f)
{
    return f->tags.count;
}

int mp4ff_meta_get_by_index(const mp4ff_t *f, unsigned int index,
                                char **item, char **value)
{
    if (index >= f->tags.count)
    {
        *item = NULL;
        *value = NULL;
        return 0;
    } else {
		*item = strdup(f->tags.tags[index].item);
		*value = strdup(f->tags.tags[index].value);
		return 1;
    }
}

int mp4ff_meta_get_title(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "title", value);
}

int mp4ff_meta_get_artist(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "artist", value);
}

int mp4ff_meta_get_writer(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "writer", value);
}

int mp4ff_meta_get_album(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "album", value);
}

int mp4ff_meta_get_date(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "date", value);
}

int mp4ff_meta_get_tool(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "tool", value);
}

int mp4ff_meta_get_comment(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "comment", value);
}

int mp4ff_meta_get_genre(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "genre", value);
}

int mp4ff_meta_get_track(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "track", value);
}

int mp4ff_meta_get_disc(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "disc", value);
}

int mp4ff_meta_get_compilation(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "compilation", value);
}

int mp4ff_meta_get_tempo(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "tempo", value);
}

int mp4ff_meta_get_coverart(const mp4ff_t *f, char **value)
{
    return mp4ff_meta_find_by_name(f, "cover", value);
}

#endif