//#include <stdlib.h>
//#include <string.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "common.h"
#include "structs.h"
#include "bits.h"


unsigned int bitmask[] = {
    0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF,
    0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
    0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF, 0x1FFFFF, 0x3FFFFF,
    0x7FFFFF, 0xFFFFFF, 0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF,
    0xFFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF
    /* added bitmask 32, correct?!?!?! */
    , 0xFFFFFFFF
};

/* initialize buffer, call once before first getbits or showbits */
#if 0
void faad_initbits(bitfile *ld, const void *_buffer, const unsigned int buffer_size)
{
    unsigned int tmp;

    if (ld == NULL)
        return;

    memset(ld, 0, sizeof(bitfile));

    if (buffer_size == 0 || _buffer == NULL)
    {
        ld->error = 1;
        ld->no_more_reading = 1;
        return;
    }

    ld->buffer = faad_malloc((buffer_size+12)*sizeof(char));
    memset(ld->buffer, 0, (buffer_size+12)*sizeof(char));
    memcpy(ld->buffer, _buffer, buffer_size*sizeof(char));

    ld->buffer_size = buffer_size;

    tmp = getdword((unsigned int*)ld->buffer);
    ld->bufa = tmp;

    tmp = getdword((unsigned int*)ld->buffer + 1);
    ld->bufb = tmp;

    ld->start = (unsigned int*)ld->buffer;
    ld->tail = ((unsigned int*)ld->buffer + 2);

    ld->bits_left = 32;

    ld->bytes_used = 0;
    ld->no_more_reading = 0;
    ld->error = 0;
}
#else
//Shawn 2004.10.7
void faad_initbits(bitfile *ld, const void *_buffer, const unsigned int buffer_size)
{
    unsigned int tmp;

    if (ld == NULL)
        return;

    if (buffer_size == 0 || _buffer == NULL)
    {
        ld->error = 1;
        ld->no_more_reading = 1;
        return;
    }

    ld->buffer = kmalloc((buffer_size+12), GFP_KERNEL);//faad_malloc((buffer_size+12));
    memset((char *)(ld->buffer+buffer_size), 0, 12);//memset(ld->buffer, 0, (buffer_size+12));//need to set to 0????????????? Shawn 2004.10.7
    memcpy(ld->buffer, _buffer, buffer_size);

    ld->buffer_size = buffer_size;

    tmp = getdword((unsigned int*)ld->buffer);
    ld->bufa = tmp;

    tmp = getdword((unsigned int*)ld->buffer + 1);
    ld->bufb = tmp;

    ld->start = (unsigned int*)ld->buffer;
    ld->tail = ((unsigned int*)ld->buffer + 2);

    ld->bits_left = 32;

    ld->bytes_used = 0;
    ld->no_more_reading = 0;
    ld->error = 0;
}
#endif

#if 0
void faad_endbits(bitfile *ld)
{
    if (ld)
    {
        if (ld->buffer)
        {
            faad_free(ld->buffer);
            ld->buffer = NULL;
        }
    }
}
#else
//Shawn 2004.12.9
void faad_endbits(bitfile *ld)
{
    if (ld)
    {
        if (ld->buffer)
        {
            kfree(ld->buffer);//faad_free(ld->buffer);
            ld->buffer = NULL;
        }
    }
}
#endif

#if 0
unsigned int faad_get_processed_bits(bitfile *ld)
{
    return (unsigned int)(8 * (4*(ld->tail - ld->start) - 4) - (ld->bits_left));
}
#else
//Shawn 2004.8.27
unsigned int faad_get_processed_bits(bitfile *ld)
{
	unsigned int reg_tmp, *reg_tail, *reg_start, reg_bits_left;

	reg_tail = ld->tail;
	reg_start = ld->start;
	reg_bits_left = ld->bits_left;
	reg_tmp = reg_tail - reg_start;
	reg_tmp = (-4<<3) + (reg_tmp << 5);
	reg_tmp = reg_tmp - reg_bits_left;

    return (unsigned int)(reg_tmp);
}
#endif

#if 0
char faad_byte_align(bitfile *ld)
{
    char remainder = (char)((32 - ld->bits_left) % 8);

    if (remainder)
    {
        faad_flushbits(ld, 8 - remainder);
        return (8 - remainder);
    }
    return 0;
}
#else
//Shawn 2004.8.27
char faad_byte_align(bitfile *ld)
{
    int remainder = (int)((32 - ld->bits_left) & 7);

    if (remainder)
    {
        faad_flushbits(ld, 8 - remainder);
        return (8 - remainder);
    }
    return 0;
}
#endif

#if 0
void faad_flushbits_ex(bitfile *ld, unsigned int bits)
{
    unsigned int tmp;

    ld->bufa = ld->bufb;
    if (ld->no_more_reading == 0)
    {
        tmp = getdword(ld->tail);
        ld->tail++;
    } else {
        tmp = 0;
    }
    ld->bufb = tmp;
    ld->bits_left += (32 - bits);
    ld->bytes_used += 4;
    if (ld->bytes_used == ld->buffer_size)
        ld->no_more_reading = 1;
    if (ld->bytes_used > ld->buffer_size)
        ld->error = 1;
}
#else
//Shawn 2004.8.24
void faad_flushbits_ex(bitfile *ld, unsigned int bits)
{
    unsigned int tmp;
	int reg_bufb, reg_bits_left, reg_bytes_used, reg_buffer_size;

	reg_bits_left = ld->bits_left;
	reg_bytes_used = ld->bytes_used;


    if (ld->no_more_reading == 0)
    {
        tmp = getdword(ld->tail);
        ld->tail++;
    }
	else
	{
        tmp = 0;
    }

	reg_bufb = ld->bufb;

	reg_bits_left += (32 - bits);
	reg_bytes_used += 4;

	ld->bufa = reg_bufb;
    ld->bufb = tmp;
    ld->bits_left = reg_bits_left;
    ld->bytes_used = reg_bytes_used;

	reg_buffer_size = ld->buffer_size;

    if (reg_bytes_used == reg_buffer_size)
        ld->no_more_reading = 1;
    if (reg_bytes_used > reg_buffer_size)
        ld->error = 1;
}
#endif







//not used by FMAD-MP4
#if 0
char *faad_getbitbuffer(bitfile *ld, unsigned int bits)
{
    unsigned short i;
    char temp;
    unsigned short bytes = (unsigned short)bits / 8;
    char remainder = (char)bits % 8;

    char *buffer = (char*)faad_malloc((bytes+1)*sizeof(char));

    for (i = 0; i < bytes; i++)
    {
        buffer[i] = (char)faad_getbits(ld, 8);
    }

    if (remainder)
    {
        temp = (char)faad_getbits(ld, remainder) << (8-remainder);

        buffer[bytes] = temp;
    }

    return buffer;
}

/* reversed bit reading routines, used for RVLC and HCR */
void faad_initbits_rev(bitfile *ld, void *buffer,
                       unsigned int bits_in_buffer)
{
    unsigned int tmp;
    int index;

    ld->buffer_size = bit2byte(bits_in_buffer);

    index = (bits_in_buffer+31)/32 - 1;

    ld->start = (unsigned int*)buffer + index - 2;

    tmp = getdword((unsigned int*)buffer + index);
    ld->bufa = tmp;

    tmp = getdword((unsigned int*)buffer + index - 1);
    ld->bufb = tmp;

    ld->tail = (unsigned int*)buffer + index;

    ld->bits_left = bits_in_buffer % 32;
    if (ld->bits_left == 0)
        ld->bits_left = 32;

    ld->bytes_used = 0;
    ld->no_more_reading = 0;
    ld->error = 0;
}

/* rewind to beginning */
void faad_rewindbits(bitfile *ld)
{
    unsigned int tmp;

    tmp = ld->start[0];
#ifndef ARCH_IS_BIG_ENDIAN
    BSWAP(tmp);
#endif
    ld->bufa = tmp;

    tmp = ld->start[1];
#ifndef ARCH_IS_BIG_ENDIAN
    BSWAP(tmp);
#endif
    ld->bufb = tmp;
    ld->bits_left = 32;
    ld->tail = &ld->start[2];
    ld->bytes_used = 0;
    ld->no_more_reading = 0;
}
#endif
