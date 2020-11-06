#ifndef __BITS_H__
#define __BITS_H__

#define BYTE_NUMBIT 8
#define bit2byte(a) ((a+7)/BYTE_NUMBIT)

#if 0
typedef struct _bitfile
{
    /* bit input */
    unsigned int bufa;
    unsigned int bufb;
    unsigned int bits_left;
    unsigned int buffer_size; /* size of the buffer in bytes */
    unsigned int bytes_used;
    char no_more_reading;
    char error;
    unsigned int *tail;
    unsigned int *start;
    void *buffer;
} bitfile;
#else
//Shawn 2004.10.7
typedef struct _bitfile
{
    /* bit input */
    unsigned int bufa;
    unsigned int bufb;
    unsigned int bits_left;
    unsigned int buffer_size; /* size of the buffer in bytes */
    unsigned int bytes_used;
    char no_more_reading;
    char error;
    unsigned int *tail;
    unsigned int *start;
    char *buffer;// is this char??????
} bitfile;
#endif


#define BSWAP(a)  ((a) = ( ((a)&0xff)<<24) | (((a)&0xff00)<<8) | (((a)>>8)&0xff00) | (((a)>>24)&0xff))

extern unsigned int bitmask[];

void faad_initbits(bitfile *ld, const void *buffer, const unsigned int buffer_size);
void faad_endbits(bitfile *ld);
void faad_initbits_rev(bitfile *ld, void *buffer,
                       unsigned int bits_in_buffer);
char faad_byte_align(bitfile *ld);
unsigned int faad_get_processed_bits(bitfile *ld);
void faad_flushbits_ex(bitfile *ld, unsigned int bits);
void faad_rewindbits(bitfile *ld);
char *faad_getbitbuffer(bitfile *ld, unsigned int bits);

/* circumvent memory alignment errors on ARM */
#if 0
static INLINE unsigned int getdword(void *mem)
{
#ifdef ARM
    unsigned int tmp;
#ifndef ARCH_IS_BIG_ENDIAN
    ((char*)&tmp)[0] = ((char*)mem)[3];
    ((char*)&tmp)[1] = ((char*)mem)[2];
    ((char*)&tmp)[2] = ((char*)mem)[1];
    ((char*)&tmp)[3] = ((char*)mem)[0];
#else
    ((char*)&tmp)[0] = ((char*)mem)[0];
    ((char*)&tmp)[1] = ((char*)mem)[1];
    ((char*)&tmp)[2] = ((char*)mem)[2];
    ((char*)&tmp)[3] = ((char*)mem)[3];
#endif

    return tmp;
#else
    unsigned int tmp;
    tmp = *(unsigned int*)mem;
#ifndef ARCH_IS_BIG_ENDIAN
    BSWAP(tmp);
#endif
    return tmp;
#endif
}
#else
//Shawn 2004.8.24
static INLINE unsigned int getdword(void *mem)
{
    unsigned int tmp;

    ((char*)&tmp)[0] = ((char*)mem)[3];
    ((char*)&tmp)[1] = ((char*)mem)[2];
    ((char*)&tmp)[2] = ((char*)mem)[1];
    ((char*)&tmp)[3] = ((char*)mem)[0];

    return tmp;
}
#endif

#if 0
static INLINE unsigned int faad_showbits(bitfile *ld, unsigned int bits)
{
    if (bits <= ld->bits_left)
    {
        return (ld->bufa >> (ld->bits_left - bits)) & bitmask[bits];
    }

    bits -= ld->bits_left;
    return ((ld->bufa & bitmask[ld->bits_left]) << bits) | (ld->bufb >> (32 - bits));
}
#else
//Shawn 2004.8.24
static INLINE unsigned int faad_showbits(bitfile *ld, unsigned int bits)
{
	unsigned int reg_r;
	unsigned int reg_ld_bl;
	unsigned int reg_bufa = ld->bufa;

	reg_ld_bl = ld->bits_left;

    if (bits <= reg_ld_bl)
    {
		reg_r = (reg_bufa >> (reg_ld_bl - bits)) & bitmask[bits];

        return reg_r;
    }

    bits = bits - reg_ld_bl;

	reg_r = ((reg_bufa & bitmask[reg_ld_bl]) << bits);
	reg_r = reg_r | (ld->bufb >> (32 - bits));

    return reg_r;
}
#endif

#if 0
static INLINE void faad_flushbits(bitfile *ld, unsigned int bits)
{
    /* do nothing if error */
    if (ld->error != 0)
        return;

    if (bits < ld->bits_left)
    {
        ld->bits_left -= bits;
    } else {
        faad_flushbits_ex(ld, bits);
    }
}
#else
//Shawn 2004.8.24
static INLINE void faad_flushbits(bitfile *ld, unsigned int bits)
{
    /* do nothing if error */
    if (ld->error != 0)
        return;

    if (bits < ld->bits_left)
    {
        ld->bits_left -= bits;
    } 
	else 
	{
        faad_flushbits_ex(ld, bits);
    }
}
#endif

/* return next n bits (right adjusted) */
#if 0
static INLINE unsigned int faad_getbits(bitfile *ld, unsigned int n)
{
    unsigned int ret;

    if (ld->no_more_reading || n == 0)
        return 0;

    ret = faad_showbits(ld, n);
    faad_flushbits(ld, n);

    return ret;
}
#else
//Shawn 2004.10.7
static INLINE unsigned int faad_getbits(bitfile *ld, unsigned int n)
{
    unsigned int ret;

    if (ld->no_more_reading)
        return 0;

    ret = faad_showbits(ld, n);
    faad_flushbits(ld, n);

    return ret;
}
#endif

#if 0
static INLINE char faad_get1bit(bitfile *ld)
{
    char r;

    if (ld->bits_left > 0)
    {
        ld->bits_left--;
        r = (char)((ld->bufa >> ld->bits_left) & 1);
        return r;
    }

    /* bits_left == 0 */
#if 0
    r = (char)(ld->bufb >> 31);
    faad_flushbits_ex(ld, 1);
#else
    r = (char)faad_getbits(ld, 1);
#endif
    return r;
}
#else
//Shawn 2004.10.7
static INLINE char faad_get1bit(bitfile *ld)
{
    int r;

    if (ld->bits_left > 0)
    {
        ld->bits_left--;
        r = ((ld->bufa >> ld->bits_left) & 1);
        return r;
    }

    /* bits_left == 0 */
    r = faad_getbits(ld, 1);

    return r;
}
#endif

#endif
