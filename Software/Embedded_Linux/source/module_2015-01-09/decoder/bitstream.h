#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

typedef struct bitstream_t
{
    unsigned char *streamBuffer;
    int totalLength;
    int bitOffset;
    //int byteOffset;
    //unsigned char curByte;
    unsigned char *cur_byte;
} Bitstream;

extern int init_bitstream(Bitstream *stream, unsigned char *buffer, int length);
extern int byte_align(Bitstream *stream);
extern int skip_bits(Bitstream *stream, int skipBitNum);
extern int skip_byte(Bitstream *stream, int skipByteNum);
extern unsigned int read_bits(Bitstream *stream, int nbits);
extern unsigned int show_bits(Bitstream *stream, int nbits);
extern unsigned int read_ue(Bitstream *stream);
extern int read_se(Bitstream *stream);

#endif
