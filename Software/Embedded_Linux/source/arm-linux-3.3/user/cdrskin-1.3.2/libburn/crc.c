/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

/* Copyright (c) 2012 Thomas Schmitt <scdbackup@gmx.net>
   Provided under GPL version 2 or later.

   Containing disabled code pieces from other GPL programs.
   They are just quotes for reference.

   The activated code uses plain polynomial division and other primitve
   algorithms to build tables of pre-computed CRC values. It then computes
   the CRCs by algorithms which are derived from mathematical considerations
   and from analysing the mathematical meaning of the disabled code pieces.

   The comments here are quite detailed in order to prove my own understanding
   of the topic.
*/


#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "crc.h"


/* Exploration ts B00214 :
   ECMA-130, 22.3.6 "CRC field"
   "This field contains the inverted parity bits. The CRC code word must be
    divisible by the check polynomial. [...]
    The generating polynomial shall be
       G(x) = x^16 + x^12 + x^5 + 1
   "
   Also known as CRC-16-CCITT, CRC-CCITT 

   Used in libburn for raw write modes in sector.c.
   There is also disabled code in read.c which would use it.

   ts B11222:
   The same algorithm is prescribed for CD-TEXT in MMC-3 Annex J.
   "CRC Field consists of 2 bytes. Initiator system may use these bytes
    to check errors in the Pack. The polynomial is x^16 + x^12 + x^5 + 1.
    All bits shall be inverted."
   
   libburn/cdtext.c uses a simple bit shifting function : crc_11021()


   ts B20211:
   Discussion why both are equivalent in respect to their result:

   Both map the bits of the given bytes to a polynomial over the finite field
   of two elements "GF(2)". If bytes 0 .. M are given, then bit n of byte m
   is mapped to the coefficient of x exponent (n + ((M - m) * 8) + 16).
   I.e. they translate the bits into a polynomial with the highest bit
   becomming the coefficient of the highest power of x. Then this polynomial
   is multiplied by (x exp 16).

   The set of all such polynomials forms a commutative ring. Its addition
   corresponds to bitwise exclusive or. Addition and subtraction are identical.
   Multiplication with polynomials of only one single non-zero coefficient
   corresponds to leftward bit shifting by the exponent of that coefficient.
   The same rules apply as with elementary school arithmetics on integer
   numbers, but with surprising results due to the finite nature of the
   coefficient number space.
   Note that multiplication is _not_ an iteration of addition here.

   Function crc_11021() performs a division with residue by the euclidian
   algorithm. I.e. it splits polynomial d into quotient q(d) and residue r(d)
   in respect to the polynomial p = x exp 16 + x exp 12 + x exp 5 + x exp 0
      d = p * q(d) + r(d)
   where r(d) is of a polynomial degree lower than p, i.e. only x exp 15
   or lower have non-zero coefficients.
   The checksum crc(D) is derived by reverse mapping (r(d) * (x exp 16)).
   I.e. by mapping the coefficient of (x exp n) to bit n of the 16 bit word
   crc(D).
   The function result is the bit-wise complement of crc(D).

   Function crc_ccitt uses a table ccitt_table of r(d) values for the
   polynomials d which represent the single byte values 0x00 to 0xff.
   It computes r(d) by computing the residues of an iteratively expanded
   polynomial. The expansion of the processed byte string A by the next byte B
   from the input byte string happens by shifting the string 8 bits to the
   left, and by oring B onto bits 0 to 7.
   In the space of polynomials, the already processed polynomial "a" (image of
   byte string A) gets expanded by polynomial b (the image of byte B) like this
      a * X + b
   where X is (x exp 8), i.e. the single coefficient polynomial of degree 8.

   The following argumentation uses algebra with commutative, associative
   and distributive laws.
   Valid especially with polynomials is this rule:
     (1):  r(a + b) = r(a) + r(b)
   because r(a) and r(b) are of degree lower than degree(p) and
   degree(a + b) <= max(degree(a), degree(b))
   Further valid are:
     (2):  r(a) = r(r(a))
     (3):  r(p * a) = 0

   The residue of this expanded polynomial can be expressed by means of the
   residue r(a) which is known from the previous iteration step, and the
   residue r(b) which may be looked up in ccitt_table.
      r(a * X + b)
    = r(p * q(a) * X + r(a) * X + p * q(b) + r(b))

   Applying rule (1):
    = r(p * q(a) * X) + r(r(a) * X) + r(p * q(b)) + r(r(b))

   Rule (3) and rule (2):
    = r(r(a) * X) + r(b)

   Be h(a) and l(a) chosen so that:  r(a) = h(a) * X + l(a),
   and l(a) has zero coefficients above (x exp 7), and h(a) * X has zero
   coefficients below (x exp 8). (They correspond to the high and low byte
   of the 16 bit word crc(A).)
   So the previous statement can be written as:
    = r(h(a) * X * X)  + r(l(a) * X) + r(b)

   Since the degree of l(a) is lower than 8, the degree of l(a) * X is lower
   than 16. Thus it cannot be divisible by p which has degree 16.
   So: r(l(a) * X) =  l(a) * X
   This yields
    = l(a) * X + r(h(a) * X * X + b)

   h(a) * X * X is the polynomial representation of the high byte of 16 bit
   word crc(A).
   So in the world of bit patterns the iteration step is:

      crc(byte string A expanded by byte B)
    = (low_byte(crc(A)) << 8) ^ crc(high_byte(crc(A)) ^ B)

   And this is what function crc_ccitt() does, modulo swapping the exor
   operants and the final bit inversion which is prescribed by ECMA-130
   and MMC-3 Annex J.

   The start value of the table driven byte shifting algorithm may be
   different from the start value of an equivalent bit shifting algorithm.
   This is because the final flushing by zero bits is already pre-computed
   in the table. So the start value of the table driven algorithm must be
   the CRC of the 0-polynomial under the start value of the bit shifting
   algorithm.
   This fact is not of much importance here, because the start value of
   the bit shifter is 0x0000 which leads to CRC 0x0000 and thus to start
   value 0x0000 with the table driven byte shifter.
*/


/* Plain implementation of polynomial division on a Galois field, where
   addition and subtraction both are binary exor. Euclidian algorithm.
   Divisor is x^16 + x^12 + x^5 + 1 = 0x11021.

   This is about ten times slower than the table driven algorithm.
*/
static int crc_11021(unsigned char *data, int count, int flag)
{
        int acc = 0, i;

        for (i = 0; i < count * 8 + 16; i++) {
                acc = (acc << 1);
                if (i < count * 8)
                        acc |= ((data[i / 8] >> (7 - (i % 8))) & 1);
                if (acc & 0x10000)
                        acc ^= 0x11021; 
        }
        return acc;
}


/* This is my own table driven implementation for which i claim copyright.

   Copyright (c) 2012 Thomas Schmitt <scdbackup@gmx.net>
*/
unsigned short crc_ccitt(unsigned char *data, int count)
{
	static unsigned short crc_tab[256], tab_initialized = 0;
	unsigned short acc = 0;
	unsigned char b[1];
	int i;

	if (!tab_initialized) {
		/* Create table of byte residues */
		for (i = 0; i < 256; i++) {
			b[0] = i;
			crc_tab[i] = crc_11021(b, 1, 0);
		}
		tab_initialized = 1;
	}
	/* There seems to be a speed advantage on amd64 if (acc << 8) is the
	   second operant of exor, and *(data++) seems faster than data[i].
	*/
	for (i = 0; i < count; i++)
		acc = crc_tab[(acc >> 8) ^ *(data++)] ^ (acc << 8);

	/* ECMA-130 22.3.6 and MMC-3 Annex J (CD-TEXT) want the result with
	   inverted bits
	*/
	return ~acc;
}


/* 
   This was the function inherited with libburn-0.2.

   static unsigned short ccitt_table[256] = {
   	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
   	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
   	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
   	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
   	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
   	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
   	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
   	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
   	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
   	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
   	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
   	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
   	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
   	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
   	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
   	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
   	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
   	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
   	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
   	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
   	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
   	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
   	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
   	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
   	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
   	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
   	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
   	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
   	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
   	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
   	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
   	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
   };
   
   unsigned short crc_ccitt(unsigned char *q, int len)
   {
   	unsigned short crc = 0;
   
   	while (len-- > 0)
   		crc = ccitt_table[(crc >> 8 ^ *q++) & 0xff] ^ (crc << 8);
   	return ~crc;
   }

*/


/* Exploration ts B00214 :
   ECMA-130, 14.3 "EDC field"
   "The EDC field shall consist of 4 bytes recorded in positions 2064 to 2067.
    The error detection code shall be a 32-bit CRC applied on bytes 0 to 2063.
    The least significant bit of a data byte is used first. The EDC codeword
    must be divisible by the check polynomial:
       P(x) = (x^16 + x^15 + x^2 + 1) . (x^16 + x^2 + x + 1)   
    The least significant parity bit (x^0) is stored in the most significant
    bit position of byte 2067.
   "

   Used for raw writing in sector.c

   
   ts B20211:
   Discussion why function crc_32() implements above prescription of ECMA-130.
   See end of this file for the ofunction inherited with libburn-0.2.

   The mentioned polynomial product 
     (x^16 + x^15 + x^2 + 1) . (x^16 + x^2 + x + 1)
   yields this sum of x exponents
       32 31    18    16
                18 17           4     2
                   17 16           3     1
                      16 15           2     0
       ======================================
       32 31          16 15     4  3     1  0
   (The number of x^18 and x^17 is divisible by two and thus 0 in GF(2).)
   This yields as 33 bit number:
      0x18001801b

   If above prescription gets implemented straight forward by function
   crc_18001801b(), then its results match the ones of crc_32() with all test
   strings which i could invent.

   The function consists of a conventional polynomial division with reverse
   input order of bits per byte.

   Further it swaps the bits in the resulting 32 bit word. That is because
   sector.c:sector_headers writes the 4 bytes of crc_32() as little endian.
   The ECMA-130 prescription rather demands big endianness and bit swapping
   towards the normal bit order in bytes:
    "The EDC field shall consist of 4 bytes recorded in positions 2064 to 2067.
     [...]
     The least significant parity bit (x^0) is stored in the most
     significant bit position of byte 2067."

   -----------------------------------------------------------------------
*/


/* Overall bit mirroring of a 32 bit word */ 
unsigned int rfl32(unsigned int acc)
{
	unsigned int inv_acc;
	int i;

	inv_acc = 0; 
	for (i = 0; i < 32; i++)
		if (acc & (1 << i))
			inv_acc |= 1 << (31 - i);
	return inv_acc;
}


/* Plain implementation of polynomial division on a Galois field, where
   addition and subtraction both are binary exor. Euclidian algorithm.
   Divisor is (x^16 + x^15 + x^2 + 1) * (x^16 + x^2 + x + 1).

   This is about ten times slower than the table driven algorithm.

   @param flag bit0= do not mirror bits in input bytes and result word
                     (Useful for building the byte indexed CRC table)
*/
static unsigned int crc_18001801b(unsigned char *data, int count, int flag)
{
	unsigned int acc = 0, top;
	long int i;
	unsigned int inv_acc;

	for (i = 0; i < count * 8 + 32; i++) {
		top = acc & 0x80000000;
		acc = (acc << 1);
		if (i < count * 8) {
			if (flag & 1)
				/* Normal bit sequence of input bytes */
				acc |= ((data[i / 8] >> (7 - (i % 8))) & 1);
			else
				/* Bit sequence of input bytes mirrored */
				acc |= ((data[i / 8] >> (i % 8)) & 1);
		}
		if (top)
			acc ^= 0x8001801b;
	}

	if (flag & 1)
		return (unsigned int) (acc & 0xffffffff);

	/* The bits of the whole 32 bit result are mirrored for ECMA-130
	   output compliance and for sector.c habit to store CRC little endian
	   although ECMA-130 prescribes it big endian.
	*/
	inv_acc = rfl32((unsigned int) acc);
	return inv_acc;
}


/*
   -----------------------------------------------------------------------

   Above discussion why crc_ccitt() and crc_11021() yield identical results
   can be changed from 16 bit to 32 bit by chosing h(a) and l(a) so that:
     r(a) = h(a) * X * X * X + l(a)
   h(a) corresponds to the highest byte of crc(A), whereas l(a) corresponds
   to the lower three bytes of crc(A).

   This yields
      r(a * X + b)
    = l(a) * X + r(h(a) * X * X * X * X + b)

   h(a) * X * X * X * X is the polynomial representation of the high byte of
   32 bit word crc(A).
   So in the world of bit patterns we have:

      crc(byte string A expanded by byte B)
    = (lowest_three_bytes(crc(A)) << 8) ^ crc(high_byte(crc(A)) ^ B)


   Regrettably this does not yet account for the byte-internal mirroring of
   bits during the conversion from bit pattern to polynomial, and during
   conversion from polynomial residue to bit pattern.
   
   Be rfl8(D) the result of byte-internal mirroring of bit pattern D,
   and mirr8(d) its corresponding polynom.

   Be now h(a) and l(a) chosen so that: r(mirr8(a)) = h(a) * X * X * X + l(a)
   This corresponds to highest byte and lower three bytes of crc(A).

      r(mirr8(a) * X + mirr8(b))
    = r(h(a) * X * X * X * X) + r(l(a) * X) + r(mirr8(b))
    = l(a)) * X + r(h(a) * X * X * X * X + mirr8(b))

   The corresponding bit pattern operation is

      crc(mirrored byte string A expanded by mirrored byte B)
    = (lowest_three_bytes(crc(A)) << 8) ^ crc(high_byte(crc(A)) ^ rfl8(B))

   This demands a final result mirroring to meet the ECMA-130 prescription.

   rfl8() can be implemented as lookup table.

   The start value of the bit shifting iteration is 0x00000000, which leads
   to the same start value for the table driven byte shifting.

   The following function crc32_by_tab() yields the same results as functions
   crc_18001801b() and crc_32():

   -----------------------------------------------------------------------
*/


/* Byte-internal bit mirroring function.
*/
unsigned int rfl8(unsigned int acc)
{
	unsigned int inv_acc;
	int i, j;

	inv_acc = 0;
	for (j = 0; j < 4; j++)
		for (i = 0; i < 8; i++)
			if (acc & (1 << (i + 8 * j)))
				inv_acc |= 1 << ((7 - i) + 8 * j);
	return inv_acc;
}


#ifdef Libburn_with_crc_illustratioN
/* Not needed for libburn. The new implementation of function crc_32() is the
   one that is used.
*/ 

unsigned int crc32_by_tab(unsigned char *data, int count, int flag)
{
	static unsigned int crc_tab[256], tab_initialized = 0;
	static unsigned char mirr_tab[256];
	unsigned int acc, inv_acc;
	unsigned char b[1];
	int i;

	if (!tab_initialized) {
		for (i = 0; i < 256; i++) {
			b[0] = i;
			/* Create table of non-mirrored 0x18001801b residues */
			crc_tab[i] = crc_18001801b(b, 1, 1);
			/* Create table of mirrored byte values */
			mirr_tab[i] = rfl8(i);
		}
		tab_initialized = 1;
	}

	acc = 0;
	for (i = 0; i < count; i++)
		acc = (acc << 8) ^ crc_tab[(acc >> 24) ^ mirr_tab[data[i]]];

	/* The bits of the whole 32 bit result are mirrored for ECMA-130
	   output compliance and for sector.c habit to store CRC little endian
	   although ECMA-130 prescribes it big endian.
	*/
	inv_acc = rfl32((unsigned int) acc);
	return inv_acc;
}

#endif /* Libburn_with_crc_illustratioN */


/*
   -----------------------------------------------------------------------

   Above function yields sufficient performance, nevertheless the old function
   crc_32() (see below) is faster by avoiding the additional mirror table
   lookup.
   A test with 10 times 650 MB on 3000 MHz amd64:
     crc_18001801b  :  187 s
     crc32_by_tab   :   27 s
     crc_32         :   16 s

   So how does crc_32() avoid the application of bit mirroring to B ?.

   Inherited crc_32() performs
		crc = crc32_table[(crc ^ *data++) & 0xffL] ^ (crc >> 8);

   Above function crc32_by_tab() would be
		crc = crc_tab[(crc >> 24) ^ mirr_tab[*data++]] ^ (crc << 8);

   The shortcut does not change the polynomial representation of the algorithm
   or the mapping from and to bit patterns. It only mirrors the bit direction
   in the bytes and in the 32-bit words which are involved in the bit pattern
   computation. This affects input (which is desired), intermediate state
   (which is as good as unmirrored), and final output (which would be slightly
   undesirable if libburn could not use the mirrored result anyway).
   
   Instead of the high byte (crc >> 24), the abbreviated algorithm uses
   the low byte of the mirrored intermediate checksum (crc & 0xffL).
   Instead of shifting the other three intermediate bytes to the left
   (crc << 8), the abbreviated algorithm shifts them to the right (crc >> 8).
   In both cases they overwrite the single byte that was used for computing
   the table index.

   The byte indexed table of CRC values needs to hold mirrored 32 bit values.
   The byte index [(crc ^ *data++) & 0xffL] would need to be mirrored, which
   would eat up the gain of not mirroring the input bytes. But this mirroring
   can be pre-computed into the table by exchanging each value with the value
   of its mirrored index.
   
   So this relation exists between the CRC table crc_tab[] of crc32_by_tab()
   and the table crc32_table[] of the abbreviated algorithm crc_32():

     crc_tab[i] == rfl32(crc32_table[rfl8(i)])

   for i={0..255}. 

   I compared the generated table in crc32_by_tab() by this test
                for (i = 0; i < 256; i++) {
                        if (rfl32(crc_tab[rfl8(i)]) != crc32_table[i] ||
                            crc_tab[i] != rfl32(crc32_table[rfl8(i)])) {
                                printf("DEVIATION : i = %d\n", i);
                                exit(1);
                        }
                }
   No screaming abort happened.

   -----------------------------------------------------------------------
*/

/* This is my own mirrored table implementation for which i claim copyright.
   With gcc -O2 it shows the same efficiency as the inherited implementation
   below. With -O3, -O1, or -O0 it is only slightly slower.

   Copyright (c) 2012 Thomas Schmitt <scdbackup@gmx.net>
*/
unsigned int crc_32(unsigned char *data, int count)
{
	static unsigned int crc_tab[256], tab_initialized = 0;
	unsigned int acc = 0;
	unsigned char b[1];
	int i;

	if (!tab_initialized) {
		/* Create table of mirrored 0x18001801b residues in
		   bit-mirrored index positions.
		*/
		for (i = 0; i < 256; i++) {
			b[0] = i;
			crc_tab[rfl8(i)] = rfl32(crc_18001801b(b, 1, 1));
		}
		tab_initialized = 1;
	}
	for (i = 0; i < count; i++)
		acc = (acc >> 8) ^ crc_tab[(acc & 0xff) ^ data[i]];

	/* The bits of the whole 32 bit result stay mirrored for ECMA-130
	   output 8-bit mirroring and for sector.c habit to store the CRC
	   little endian although ECMA-130 prescribes it big endian.
	*/
	return acc;
}


/*
   -----------------------------------------------------------------------

   This was the function inherited with libburn-0.2 which implements the
   abbreviated algorithm. Its obscure existence led me to above insights.
   My compliments to the (unknown) people who invented this.

   unsigned long crc32_table[256] = {
	0x00000000L, 0x90910101L, 0x91210201L, 0x01B00300L,
	0x92410401L, 0x02D00500L, 0x03600600L, 0x93F10701L,
	0x94810801L, 0x04100900L, 0x05A00A00L, 0x95310B01L,
	0x06C00C00L, 0x96510D01L, 0x97E10E01L, 0x07700F00L,
	0x99011001L, 0x09901100L, 0x08201200L, 0x98B11301L,
	0x0B401400L, 0x9BD11501L, 0x9A611601L, 0x0AF01700L,
	0x0D801800L, 0x9D111901L, 0x9CA11A01L, 0x0C301B00L,
	0x9FC11C01L, 0x0F501D00L, 0x0EE01E00L, 0x9E711F01L,
	0x82012001L, 0x12902100L, 0x13202200L, 0x83B12301L,
	0x10402400L, 0x80D12501L, 0x81612601L, 0x11F02700L,
	0x16802800L, 0x86112901L, 0x87A12A01L, 0x17302B00L,
	0x84C12C01L, 0x14502D00L, 0x15E02E00L, 0x85712F01L,
	0x1B003000L, 0x8B913101L, 0x8A213201L, 0x1AB03300L,
	0x89413401L, 0x19D03500L, 0x18603600L, 0x88F13701L,
	0x8F813801L, 0x1F103900L, 0x1EA03A00L, 0x8E313B01L,
	0x1DC03C00L, 0x8D513D01L, 0x8CE13E01L, 0x1C703F00L,
	0xB4014001L, 0x24904100L, 0x25204200L, 0xB5B14301L,
	0x26404400L, 0xB6D14501L, 0xB7614601L, 0x27F04700L,
	0x20804800L, 0xB0114901L, 0xB1A14A01L, 0x21304B00L,
	0xB2C14C01L, 0x22504D00L, 0x23E04E00L, 0xB3714F01L,
	0x2D005000L, 0xBD915101L, 0xBC215201L, 0x2CB05300L,
	0xBF415401L, 0x2FD05500L, 0x2E605600L, 0xBEF15701L,
	0xB9815801L, 0x29105900L, 0x28A05A00L, 0xB8315B01L,
	0x2BC05C00L, 0xBB515D01L, 0xBAE15E01L, 0x2A705F00L,
	0x36006000L, 0xA6916101L, 0xA7216201L, 0x37B06300L,
	0xA4416401L, 0x34D06500L, 0x35606600L, 0xA5F16701L,
	0xA2816801L, 0x32106900L, 0x33A06A00L, 0xA3316B01L,
	0x30C06C00L, 0xA0516D01L, 0xA1E16E01L, 0x31706F00L,
	0xAF017001L, 0x3F907100L, 0x3E207200L, 0xAEB17301L,
	0x3D407400L, 0xADD17501L, 0xAC617601L, 0x3CF07700L,
	0x3B807800L, 0xAB117901L, 0xAAA17A01L, 0x3A307B00L,
	0xA9C17C01L, 0x39507D00L, 0x38E07E00L, 0xA8717F01L,
	0xD8018001L, 0x48908100L, 0x49208200L, 0xD9B18301L,
	0x4A408400L, 0xDAD18501L, 0xDB618601L, 0x4BF08700L,
	0x4C808800L, 0xDC118901L, 0xDDA18A01L, 0x4D308B00L,
	0xDEC18C01L, 0x4E508D00L, 0x4FE08E00L, 0xDF718F01L,
	0x41009000L, 0xD1919101L, 0xD0219201L, 0x40B09300L,
	0xD3419401L, 0x43D09500L, 0x42609600L, 0xD2F19701L,
	0xD5819801L, 0x45109900L, 0x44A09A00L, 0xD4319B01L,
	0x47C09C00L, 0xD7519D01L, 0xD6E19E01L, 0x46709F00L,
	0x5A00A000L, 0xCA91A101L, 0xCB21A201L, 0x5BB0A300L,
	0xC841A401L, 0x58D0A500L, 0x5960A600L, 0xC9F1A701L,
	0xCE81A801L, 0x5E10A900L, 0x5FA0AA00L, 0xCF31AB01L,
	0x5CC0AC00L, 0xCC51AD01L, 0xCDE1AE01L, 0x5D70AF00L,
	0xC301B001L, 0x5390B100L, 0x5220B200L, 0xC2B1B301L,
	0x5140B400L, 0xC1D1B501L, 0xC061B601L, 0x50F0B700L,
	0x5780B800L, 0xC711B901L, 0xC6A1BA01L, 0x5630BB00L,
	0xC5C1BC01L, 0x5550BD00L, 0x54E0BE00L, 0xC471BF01L,
	0x6C00C000L, 0xFC91C101L, 0xFD21C201L, 0x6DB0C300L,
	0xFE41C401L, 0x6ED0C500L, 0x6F60C600L, 0xFFF1C701L,
	0xF881C801L, 0x6810C900L, 0x69A0CA00L, 0xF931CB01L,
	0x6AC0CC00L, 0xFA51CD01L, 0xFBE1CE01L, 0x6B70CF00L,
	0xF501D001L, 0x6590D100L, 0x6420D200L, 0xF4B1D301L,
	0x6740D400L, 0xF7D1D501L, 0xF661D601L, 0x66F0D700L,
	0x6180D800L, 0xF111D901L, 0xF0A1DA01L, 0x6030DB00L,
	0xF3C1DC01L, 0x6350DD00L, 0x62E0DE00L, 0xF271DF01L,
	0xEE01E001L, 0x7E90E100L, 0x7F20E200L, 0xEFB1E301L,
	0x7C40E400L, 0xECD1E501L, 0xED61E601L, 0x7DF0E700L,
	0x7A80E800L, 0xEA11E901L, 0xEBA1EA01L, 0x7B30EB00L,
	0xE8C1EC01L, 0x7850ED00L, 0x79E0EE00L, 0xE971EF01L,
	0x7700F000L, 0xE791F101L, 0xE621F201L, 0x76B0F300L,
	0xE541F401L, 0x75D0F500L, 0x7460F600L, 0xE4F1F701L,
	0xE381F801L, 0x7310F900L, 0x72A0FA00L, 0xE231FB01L,
	0x71C0FC00L, 0xE151FD01L, 0xE0E1FE01L, 0x7070FF00L
   };

   unsigned int crc_32(unsigned char *data, int len)
   {
	unsigned int crc = 0;

	while (len-- > 0)
		crc = crc32_table[(crc ^ *data++) & 0xffL] ^ (crc >> 8);
	return crc;
   }
*/ 


