#ifndef FTMCP100_comm_H
#define FTMCP100_comm_H
//---------------------------------------------------------------
// common field for jpeg encode and decode
//
//---------------------------------------------------------------
typedef enum {
	WAIT_FOR_COMMAND = 0,
	DECODE_IFRAME=1,
	DECODE_PFRAME=2,
	DECODE_NFRAME=3,
	ENCODE_IFRAME=4,
	ENCODE_PFRAME=5,
	ENCODE_NFRAME=6,
	DECODE_JPEG=7,
	ENCODE_JPEG=8,
	INTERNAL_INT = 9
} CODEC_COMMAND;

#define IT_JPGE	(1L<<15)	// interrupt type: JPG Encoder
#define IT_JPGD	(1L<<14)	// interrupt type: JPG Decoder
#endif
