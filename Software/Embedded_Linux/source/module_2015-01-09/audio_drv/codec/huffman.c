#include "common.h"
#include "structs.h"
//#include <stdlib.h>
#include "bits.h"
#include "huffman.h"
#include "codebook/hcb.h"

/* static function declarations */
static INLINE void huffman_sign_bits(bitfile *ld, short *sp, unsigned char len);
static INLINE short huffman_getescape(bitfile *ld, short sp);
static unsigned char huffman_2step_quad(unsigned char cb, bitfile *ld, short *sp);
static unsigned char huffman_2step_quad_sign(unsigned char cb, bitfile *ld, short *sp);
static unsigned char huffman_2step_pair(unsigned char cb, bitfile *ld, short *sp);
static unsigned char huffman_2step_pair_sign(unsigned char cb, bitfile *ld, short *sp);
static unsigned char huffman_binary_quad(unsigned char cb, bitfile *ld, short *sp);
static unsigned char huffman_binary_quad_sign(unsigned char cb, bitfile *ld, short *sp);
static unsigned char huffman_binary_pair(unsigned char cb, bitfile *ld, short *sp);
static unsigned char huffman_binary_pair_sign(unsigned char cb, bitfile *ld, short *sp);
static short huffman_codebook(unsigned char i);

#if 0
signed char huffman_scale_factor(bitfile *ld)
{
    unsigned short offset = 0;

    while (hcb_sf[offset][1])
    {
        char b = faad_get1bit(ld);
        offset += hcb_sf[offset][b];

        if (offset > 240)
        {
            /* printf("ERROR: offset into hcb_sf = %d >240!\n", offset); */
            return -1;
        }
    }

    return hcb_sf[offset][0];
}
#else
//Shawn 2004.10.6
int huffman_scale_factor(bitfile *ld)
{
    int offset = 0;

    while (hcb_sf[offset][1])
    {
        int b = (int)faad_get1bit(ld);

        offset += hcb_sf[offset][b];

        if (offset > 240)
        {
            /* printf("ERROR: offset into hcb_sf = %d >240!\n", offset); */
            return -1;
        }
    }

    return (int)hcb_sf[offset][0];
}
#endif


hcb *hcb_table[] =
{
    0, hcb1_1, hcb2_1, 0, hcb4_1, 0, hcb6_1, 0, hcb8_1, 0, hcb10_1, hcb11_1
};

hcb_2_quad *hcb_2_quad_table[] =
{
    0, hcb1_2, hcb2_2, 0, hcb4_2, 0, 0, 0, 0, 0, 0, 0
};

hcb_2_pair *hcb_2_pair_table[] =
{
    0, 0, 0, 0, 0, 0, hcb6_2, 0, hcb8_2, 0, hcb10_2, hcb11_2
};

hcb_bin_pair *hcb_bin_table[] =
{
    0, 0, 0, 0, 0, hcb5, 0, hcb7, 0, hcb9, 0, 0
};

char hcbN[] = { 0, 5, 5, 0, 5, 0, 5, 0, 5, 0, 6, 5 };

/* defines whether a huffman codebook is unsigned or not */
/* Table 4.6.2 */
#if 0
char unsigned_cb[] = { 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  /* codebook 16 to 31 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
#endif

int hcb_2_quad_table_size[] = { 0, 114, 86, 0, 185, 0, 0, 0, 0, 0, 0, 0 };
int hcb_2_pair_table_size[] = { 0, 0, 0, 0, 0, 0, 126, 0, 83, 0, 210, 373 };
int hcb_bin_table_size[] = { 0, 0, 0, 161, 0, 161, 0, 127, 0, 337, 0, 0 };

#if 0
static INLINE void huffman_sign_bits(bitfile *ld, short *sp, char len)
{
    char i;

    for (i = 0; i < len; i++)
    {
        if(sp[i])
        {
            if(faad_get1bit(ld) & 1)
            {
                sp[i] = -sp[i];
            }
        }
    }
}
#else
//Shawn 2004.10.7
static INLINE void huffman_sign_bits(bitfile *ld, short *sp, unsigned char len)
{
    int i;
	int reg_sp;

    for (i = len; i != 0; i --)
    {
		reg_sp = *sp++;

        if(reg_sp)
        {
            if(faad_get1bit(ld))
            {
                sp[-1] = -reg_sp;
            }
        }
    }
}
#endif

#if 0
static INLINE short huffman_getescape(bitfile *ld, short sp)
{
    char neg, i;
    int j;
	int off;

    if (sp < 0)
    {
        if (sp != -16)
            return sp;
        neg = 1;
    } else {
        if (sp != 16)
            return sp;
        neg = 0;
    }

    for (i = 4; ; i++)
    {
        if (faad_get1bit(ld) == 0)
        {
            break;
        }
    }

    off = faad_getbits(ld, i);

    j = off | (1<<i);
    if (neg)
        j = -j;

    return j;
}
#else
//Shawn 2004.10.7
static INLINE short huffman_getescape(bitfile *ld, short sp)
{
    int neg, i;
    int j;
	int off;
	int reg_sp;

    if (sp < 0)
    {
        reg_sp = -sp;
        neg = 1;
    }
	else
	{
		reg_sp = sp;
        neg = 0;
    }

	if (reg_sp != 16)
            return sp;

	i = 4;
    //for (i = 4; ; i++)
	while(1)
    {
        if (faad_get1bit(ld) == 0)
        {
            break;
        }
		i ++;
    }

    off = faad_getbits(ld, i);

    j = off | (1 << i);

    if (neg)
        j = -j;

    return j;
}
#endif

#if 0
static char huffman_2step_quad(char cb, bitfile *ld, short *sp)
{
    unsigned int cw;
    unsigned short offset = 0;
    char extra_bits;

    cw = faad_showbits(ld, hcbN[cb]);
    offset = hcb_table[cb][cw].offset;
    extra_bits = hcb_table[cb][cw].extra_bits;

    if (extra_bits)
    {
        /* we know for sure it's more than hcbN[cb] bits long */
        faad_flushbits(ld, hcbN[cb]);
        offset += (unsigned short)faad_showbits(ld, extra_bits);
        faad_flushbits(ld, hcb_2_quad_table[cb][offset].bits - hcbN[cb]);
    } else {
        faad_flushbits(ld, hcb_2_quad_table[cb][offset].bits);
    }

    if (offset > hcb_2_quad_table_size[cb])
    {
        /* printf("ERROR: offset into hcb_2_quad_table = %d >%d!\n", offset,
           hcb_2_quad_table_size[cb]); */
        return 10;
    }

    sp[0] = hcb_2_quad_table[cb][offset].x;
    sp[1] = hcb_2_quad_table[cb][offset].y;
    sp[2] = hcb_2_quad_table[cb][offset].v;
    sp[3] = hcb_2_quad_table[cb][offset].w;

    return 0;
}
#else
//Shawn 2004.8.24
static unsigned char huffman_2step_quad(unsigned char cb, bitfile *ld, short *sp)
{
    unsigned int cw;
    int offset = 0;
    unsigned char extra_bits;
	hcb_2_quad *hcb_2_quad_table_p;
	int regx, regy, regv, regw;



    cw = faad_showbits(ld, hcbN[cb]);
    offset = hcb_table[cb][cw].offset;
    extra_bits = hcb_table[cb][cw].extra_bits;

    if (extra_bits)
    {
        /* we know for sure it's more than hcbN[cb] bits long */
        faad_flushbits(ld, hcbN[cb]);
        offset += (unsigned short)faad_showbits(ld, extra_bits);
		hcb_2_quad_table_p = (hcb_2_quad *)&hcb_2_quad_table[cb][offset];
        faad_flushbits(ld, hcb_2_quad_table_p->bits - hcbN[cb]);
    }
	else
	{
		hcb_2_quad_table_p = (hcb_2_quad *)&hcb_2_quad_table[cb][offset];
        faad_flushbits(ld, hcb_2_quad_table_p->bits);
    }

    if (offset > hcb_2_quad_table_size[cb])
    {
        /* printf("ERROR: offset into hcb_2_quad_table = %d >%d!\n", offset,
           hcb_2_quad_table_size[cb]); */
        return 10;
    }

	regx = hcb_2_quad_table_p->x;
	regy = hcb_2_quad_table_p->y;
	regv = hcb_2_quad_table_p->v;
	regw = hcb_2_quad_table_p->w;

    sp[0] = regx;
    sp[1] = regy;
    sp[2] = regv;
    sp[3] = regw;

    return 0;
}
#endif

static unsigned char huffman_2step_quad_sign(unsigned char cb, bitfile *ld, short *sp)
{
    unsigned char err = huffman_2step_quad(cb, ld, sp);
    huffman_sign_bits(ld, sp, QUAD_LEN);

    return err;
}

#if 0
static char huffman_2step_pair(char cb, bitfile *ld, short *sp)
{
    unsigned int cw;
    unsigned short offset = 0;
    char extra_bits;

    cw = faad_showbits(ld, hcbN[cb]);
    offset = hcb_table[cb][cw].offset;
    extra_bits = hcb_table[cb][cw].extra_bits;

    if (extra_bits)
    {
        /* we know for sure it's more than hcbN[cb] bits long */
        faad_flushbits(ld, hcbN[cb]);
        offset += (unsigned short)faad_showbits(ld, extra_bits);
        faad_flushbits(ld, hcb_2_pair_table[cb][offset].bits - hcbN[cb]);
    } else {
        faad_flushbits(ld, hcb_2_pair_table[cb][offset].bits);
    }

    if (offset > hcb_2_pair_table_size[cb])
    {
        /* printf("ERROR: offset into hcb_2_pair_table = %d >%d!\n", offset,
           hcb_2_pair_table_size[cb]); */
        return 10;
    }

    sp[0] = hcb_2_pair_table[cb][offset].x;
    sp[1] = hcb_2_pair_table[cb][offset].y;

    return 0;
}
#else
static unsigned char huffman_2step_pair(unsigned char cb, bitfile *ld, short *sp)
{
    unsigned int cw;
    unsigned short offset = 0;
    unsigned char extra_bits;

	int regx, regy;

    cw = faad_showbits(ld, hcbN[cb]);
    offset = hcb_table[cb][cw].offset;
    extra_bits = hcb_table[cb][cw].extra_bits;

    if (extra_bits)
    {
        /* we know for sure it's more than hcbN[cb] bits long */
        faad_flushbits(ld, hcbN[cb]);
        offset += (unsigned short)faad_showbits(ld, extra_bits);
        faad_flushbits(ld, hcb_2_pair_table[cb][offset].bits - hcbN[cb]);
    } else {
        faad_flushbits(ld, hcb_2_pair_table[cb][offset].bits);
    }

    if (offset > hcb_2_pair_table_size[cb])
    {
        /* printf("ERROR: offset into hcb_2_pair_table = %d >%d!\n", offset,
           hcb_2_pair_table_size[cb]); */
        return 10;
    }

	regx = hcb_2_pair_table[cb][offset].x;
	regy = hcb_2_pair_table[cb][offset].y;

    sp[0] = regx;
    sp[1] = regy;

    return 0;
}
#endif

static unsigned char huffman_2step_pair_sign(unsigned char cb, bitfile *ld, short *sp)
{
    unsigned char err = huffman_2step_pair(cb, ld, sp);
    huffman_sign_bits(ld, sp, PAIR_LEN);

    return err;
}

#if 0
static char huffman_binary_quad(char cb, bitfile *ld, short *sp)
{
    unsigned short offset = 0;

    while (!hcb3[offset].is_leaf)
    {
        char b = faad_get1bit(ld);
        offset += hcb3[offset].data[b];
    }

    if (offset > hcb_bin_table_size[cb])
    {
        /* printf("ERROR: offset into hcb_bin_table = %d >%d!\n", offset,
           hcb_bin_table_size[cb]); */
        return 10;
    }

    sp[0] = hcb3[offset].data[0];
    sp[1] = hcb3[offset].data[1];
    sp[2] = hcb3[offset].data[2];
    sp[3] = hcb3[offset].data[3];

    return 0;
}
#else
//Shawn 2004.10.7
static unsigned char huffman_binary_quad(unsigned char cb, bitfile *ld, short *sp)
{
    int offset = 0;
	int reg0, reg1, reg2, reg3;

    while (!hcb3[offset].is_leaf)
    {
        int b = faad_get1bit(ld);
        offset += hcb3[offset].data[b];
    }

    if (offset > hcb_bin_table_size[cb])
    {
        /* printf("ERROR: offset into hcb_bin_table = %d >%d!\n", offset,
           hcb_bin_table_size[cb]); */
        return 10;
    }

	reg0 = hcb3[offset].data[0];
	reg1 = hcb3[offset].data[1];
	reg2 = hcb3[offset].data[2];
	reg3 = hcb3[offset].data[3];

    sp[0] = reg0;
    sp[1] = reg1;
    sp[2] = reg2;
    sp[3] = reg3;

    return 0;
}
#endif

static unsigned char huffman_binary_quad_sign(unsigned char cb, bitfile *ld, short *sp)
{
    unsigned char err = huffman_binary_quad(cb, ld, sp);
    huffman_sign_bits(ld, sp, QUAD_LEN);

    return err;
}

#if 0
static char huffman_binary_pair(char cb, bitfile *ld, short *sp)
{
    unsigned short offset = 0;

    while (!hcb_bin_table[cb][offset].is_leaf)
    {
        char b = faad_get1bit(ld);
        offset += hcb_bin_table[cb][offset].data[b];
    }

    if (offset > hcb_bin_table_size[cb])
    {
        /* printf("ERROR: offset into hcb_bin_table = %d >%d!\n", offset,
           hcb_bin_table_size[cb]); */
        return 10;
    }

    sp[0] = hcb_bin_table[cb][offset].data[0];
    sp[1] = hcb_bin_table[cb][offset].data[1];

    return 0;
}
#else
//Shawn 2004.10.7
static unsigned char huffman_binary_pair(unsigned char cb, bitfile *ld, short *sp)
{
    int offset = 0;
	int reg0, reg1;

    while (!hcb_bin_table[cb][offset].is_leaf)
    {
        int b = faad_get1bit(ld);
        offset += hcb_bin_table[cb][offset].data[b];
    }

    if (offset > hcb_bin_table_size[cb])
    {
        /* printf("ERROR: offset into hcb_bin_table = %d >%d!\n", offset,
           hcb_bin_table_size[cb]); */
        return 10;
    }

	reg0 = hcb_bin_table[cb][offset].data[0];
	reg1 = hcb_bin_table[cb][offset].data[1];

    sp[0] = reg0;
    sp[1] = reg1;

    return 0;
}
#endif

static unsigned char huffman_binary_pair_sign(unsigned char cb, bitfile *ld, short *sp)
{
    unsigned char err = huffman_binary_pair(cb, ld, sp);
    huffman_sign_bits(ld, sp, PAIR_LEN);

    return err;
}

static short huffman_codebook(unsigned char i)
{
    static const unsigned int data = 16428320;
    if (i == 0) return (short)(data >> 16) & 0xFFFF;
    else        return (short)data & 0xFFFF;
}

#if 0
char huffman_spectral_data(char cb, bitfile *ld, short *sp)
{
    switch (cb)
    {
    case 1: /* 2-step method for data quadruples */
    case 2:
        return huffman_2step_quad(cb, ld, sp);
    case 3: /* binary search for data quadruples */
        return huffman_binary_quad_sign(cb, ld, sp);
    case 4: /* 2-step method for data quadruples */
        return huffman_2step_quad_sign(cb, ld, sp);
    case 5: /* binary search for data pairs */
        return huffman_binary_pair(cb, ld, sp);
    case 6: /* 2-step method for data pairs */
        return huffman_2step_pair(cb, ld, sp);
    case 7: /* binary search for data pairs */
    case 9:
        return huffman_binary_pair_sign(cb, ld, sp);
    case 8: /* 2-step method for data pairs */
    case 10:
        return huffman_2step_pair_sign(cb, ld, sp);
    case 12: {
        char err = huffman_2step_pair(11, ld, sp);
        sp[0] = huffman_codebook(0); sp[1] = huffman_codebook(1);
        return err; }
    case 11:
    {
        char err = huffman_2step_pair_sign(11, ld, sp);
        sp[0] = huffman_getescape(ld, sp[0]);
        sp[1] = huffman_getescape(ld, sp[1]);
        return err;
    }
    default:
        /* Non existent codebook number, something went wrong */
        return 11;
    }

    return 0;
}
#else
//Shawn 2004.10.6
int huffman_spectral_data(unsigned char cb, bitfile *ld, short *sp)
{
    switch (cb)
    {
    case 1: /* 2-step method for data quadruples */
    case 2:
        return huffman_2step_quad(cb, ld, sp);
    case 3: /* binary search for data quadruples */
        return huffman_binary_quad_sign(cb, ld, sp);
    case 4: /* 2-step method for data quadruples */
        return huffman_2step_quad_sign(cb, ld, sp);
    case 5: /* binary search for data pairs */
        return huffman_binary_pair(cb, ld, sp);
    case 6: /* 2-step method for data pairs */
        return huffman_2step_pair(cb, ld, sp);
    case 7: /* binary search for data pairs */
    case 9:
        return huffman_binary_pair_sign(cb, ld, sp);
    case 8: /* 2-step method for data pairs */
    case 10:
        return huffman_2step_pair_sign(cb, ld, sp);
    case 12: {
        int err = huffman_2step_pair(11, ld, sp);
        sp[0] = huffman_codebook(0); sp[1] = huffman_codebook(1);
        return err; }
    case 11:
    {
        int err = huffman_2step_pair_sign(11, ld, sp);
        sp[0] = huffman_getescape(ld, sp[0]);
        sp[1] = huffman_getescape(ld, sp[1]);
        return err;
    }
    default:
        /* Non existent codebook number, something went wrong */
        return 11;
    }

    return 0;
}
#endif
