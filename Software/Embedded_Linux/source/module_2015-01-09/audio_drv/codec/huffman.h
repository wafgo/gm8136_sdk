#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__


#if 0
char huffman_spectral_data(char cb, bitfile *ld, short *sp);
signed char huffman_scale_factor(bitfile *ld);
#else
//Shawn 2004.10.6
int huffman_spectral_data(unsigned char cb, bitfile *ld, short *sp);
int huffman_scale_factor(bitfile *ld);
#endif

#endif
