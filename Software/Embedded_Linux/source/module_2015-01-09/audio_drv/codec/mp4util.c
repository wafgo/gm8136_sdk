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
** $Id: mp4util.c,v 1.1 2013/09/04 06:26:55 easonli Exp $
**/

#include "mp4ffint.h"
#include <stdlib.h>

int mp4ff_read_data(mp4ff_t *f, signed char *data, unsigned int size)
{
    int result = 1;

    result = f->stream->read(f->stream->user_data, data, size);

    f->current_position += size;

    return result;
}

int mp4ff_truncate(mp4ff_t * f)
{
	return f->stream->truncate(f->stream->user_data);
}

int mp4ff_write_data(mp4ff_t *f, signed char *data, unsigned int size)
{
    int result = 1;

    result = f->stream->write(f->stream->user_data, data, size);

    f->current_position += size;

    return result;
}

int mp4ff_write_int32(mp4ff_t *f,const unsigned int data)
{
	unsigned int result;
    unsigned int a, b, c, d;
    signed char temp[4];
    
    *(unsigned int*)temp = data;
    a = (unsigned char)temp[0];
    b = (unsigned char)temp[1];
    c = (unsigned char)temp[2];
    d = (unsigned char)temp[3];

    result = (a<<24) | (b<<16) | (c<<8) | d;

    return mp4ff_write_data(f,(signed char*)&result,sizeof(result));
}

int mp4ff_set_position(mp4ff_t *f, const int64_t position)
{
    f->stream->seek(f->stream->user_data, position);
    f->current_position = position;

    return 0;
}

int64_t mp4ff_position(const mp4ff_t *f)
{
    return f->current_position;
}

unsigned long long mp4ff_read_int64(mp4ff_t *f)
{
    unsigned char data[8];
    unsigned long long result = 0;
    signed char i;

    mp4ff_read_data(f, (signed char*)data, 8);

    for (i = 0; i < 8; i++)
    {
        result |= ((unsigned long long)data[i]) << ((7 - i) * 8);
    }

    return result;
}

unsigned int mp4ff_read_int32(mp4ff_t *f)
{
    unsigned int result;
    unsigned int a, b, c, d;
    signed char data[4];
    
    mp4ff_read_data(f, data, 4);
    a = (unsigned char)data[0];
    b = (unsigned char)data[1];
    c = (unsigned char)data[2];
    d = (unsigned char)data[3];

    result = (a<<24) | (b<<16) | (c<<8) | d;
    return (unsigned int)result;
}

unsigned int mp4ff_read_int24(mp4ff_t *f)
{
    unsigned int result;
    unsigned int a, b, c;
    signed char data[4];
    
    mp4ff_read_data(f, data, 3);
    a = (unsigned char)data[0];
    b = (unsigned char)data[1];
    c = (unsigned char)data[2];

    result = (a<<16) | (b<<8) | c;
    return (unsigned int)result;
}

unsigned short mp4ff_read_int16(mp4ff_t *f)
{
    unsigned int result;
    unsigned int a, b;
    signed char data[2];
    
    mp4ff_read_data(f, data, 2);
    a = (unsigned char)data[0];
    b = (unsigned char)data[1];

    result = (a<<8) | b;
    return (unsigned short)result;
}

char * mp4ff_read_string(mp4ff_t * f,unsigned int length)
{
	char * str = (char*)malloc(length + 1);
	if (str!=0)
	{
		if ((unsigned int)mp4ff_read_data(f,(signed char *)str,length)!=length)
		{
			free(str);
			str = 0;
		}
		else
		{
			str[length] = 0;
		}
	}
	return str;	
}

unsigned char mp4ff_read_char(mp4ff_t *f)
{
    unsigned char output;
    mp4ff_read_data(f, (signed char *)&output, 1);
    return output;
}

unsigned int mp4ff_read_mp4_descr_length(mp4ff_t *f)
{
    unsigned char b;
    unsigned char numBytes = 0;
    unsigned int length = 0;

    do
    {
        b = mp4ff_read_char(f);
        numBytes++;
        length = (length << 7) | (b & 0x7F);
    } while ((b & 0x80) && numBytes < 4);

    return length;
}
