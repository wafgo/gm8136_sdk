#include "bitstream.h"

unsigned char BIT_MASK[8] = {0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

static __inline int next_bit(Bitstream *stream)
{
    int value = ((*stream->cur_byte) >> (stream->bitOffset--)) & 0x01;
    if (stream->bitOffset < 0) {
        stream->bitOffset = 7;
        stream->cur_byte++;
    }
    return value;
}

int init_bitstream(Bitstream *stream, unsigned char *buffer, int length)
{
    stream->streamBuffer = buffer;
    stream->totalLength = length;
    stream->bitOffset = 7;
    //stream->byteOffset = 0;
    //stream->curByte = *stream->streamBuffer;
    stream->cur_byte = stream->streamBuffer;
    return 0;
}

int byte_align(Bitstream *stream)
{
    if (stream->bitOffset != 7) {
        stream->bitOffset = 7;
        stream->cur_byte++;
    }
    return 0;
}

int skip_byte(Bitstream *stream, int skipByteNum)
{
    stream->cur_byte += skipByteNum;
    return 0;
}

int skip_bits(Bitstream *stream, int skipBitNum)
{
    while (skipBitNum--) {
        stream->bitOffset--;
        if (stream->bitOffset < 0) {
            stream->bitOffset = 7;
            stream->cur_byte++;
        }
    }
    return 0;
}

unsigned int read_bits(Bitstream *stream, int nbits)
{
    int bitcounter = nbits;
    int inf = 0;

    while (bitcounter--) {
        inf = (inf << 1) | next_bit(stream);
    }
    return inf;
}

unsigned int show_bits(Bitstream *stream, int nbits)
{
    int bitoffset  = stream->bitOffset;
    unsigned char  *curbyte  = stream->cur_byte;
    int inf        = 0;

    while (nbits--) {
        inf <<=1;    
        inf |= ((*curbyte)>> (bitoffset--)) & 0x01;

        if (bitoffset == -1 ) { //Move onto next byte to get all of numbits
            curbyte++;
            bitoffset = 7;
        }
    }
    return inf;
}

unsigned int read_ue(Bitstream *stream)
{
    int bitcounter = 1;
    int len = 0;
    int ctr_bit;
    int inf = 0;
    int value;

    ctr_bit = next_bit(stream);

    while (ctr_bit == 0) {  // find leading 1 bit
        len++;
        bitcounter++;
        ctr_bit = next_bit(stream);
    }

    while (len--) {
        inf = (inf << 1) | next_bit(stream);
        bitcounter++;
    }

    value = (int) (((unsigned int) 1 << (bitcounter >> 1)) + (unsigned int) (inf) - 1);

    return value; 
}

int read_se(Bitstream *stream)
{
    int bitcounter = 1;
    int len = 0;
    int ctr_bit;// = (stream->curByte >> stream->bitOffset) & 0x01;  // control bit for current bit posision
    int inf = 0;
    int value;
    unsigned int n;

    ctr_bit = next_bit(stream);

    while (ctr_bit == 0) {  // find leading 1 bit
        len++;
        bitcounter++;
        ctr_bit = next_bit(stream);
    }

    while (len--) {
        inf = (inf << 1) | next_bit(stream);
        bitcounter++;
    }

    n = ((unsigned int) 1 << (bitcounter >> 1)) + (unsigned int) inf - 1;
    value = (n + 1) >> 1;
    if ((n & 0x01) == 0)
        value = -value;
    return value;
}
