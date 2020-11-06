#ifndef _HEADER_H_
#define _HEADER_H_

#include "bitstream.h"

// JPEG
typedef enum
{
	SOF0  = 0xc0,
	SOF1  = 0xc1,
	SOF2  = 0xc2,
	SOF3  = 0xc3,

	SOF5  = 0xc5,
	SOF6  = 0xc6,
	SOF7  = 0xc7,

	JPG   = 0xc8,
	SOF9  = 0xc9,
	SOF10 = 0xca,
	SOF11 = 0xcb,

	SOF13 = 0xcd,
	SOF14 = 0xce,
	SOF15 = 0xcf,

	DHT   = 0xc4,

	DAC   = 0xcc,

	RST0  = 0xd0,
	RST1  = 0xd1,
	RST2  = 0xd2,
	RST3  = 0xd3,
	RST4  = 0xd4,
	RST5  = 0xd5,
	RST6  = 0xd6,
	RST7  = 0xd7,

	SOI   = 0xd8,
	EOI   = 0xd9,
	SOS   = 0xda,
	DQT   = 0xdb,
	DNL   = 0xdc,
	DRI   = 0xdd,
	DHP   = 0xde,
	EXP   = 0xdf,

	APP0  = 0xe0,
	APP1  = 0xe1,
	APP2  = 0xe2,
	APP3  = 0xe3,
	APP4  = 0xe4,
	APP5  = 0xe5,
	APP6  = 0xe6,
	APP7  = 0xe7,
	APP8  = 0xe8,
	APP9  = 0xe9,
	APP10 = 0xea,
	APP11 = 0xeb,
	APP12 = 0xec,
	APP13 = 0xed,
	APP14 = 0xee,
	APP15 = 0xef,

	JPG0  = 0xf0,
	JPG1  = 0xf1,
	JPG2  = 0xf2,
	JPG3  = 0xf3,
	JPG4  = 0xf4,
	JPG5  = 0xf5,
	JPG6  = 0xf6,
	JPG7  = 0xf7,
	JPG8  = 0xf8,
	JPG9  = 0xf9,
	JPG10  = 0xfa,
	JPG11  = 0xfb,
	JPG12  = 0xfc,
	JPG13  = 0xfd,

	COM   = 0xfe,

	TEM   = 0x01
} JPEG_MARKER;

// H264 
typedef enum{
	NALU_TYPE_SLICE    = 1,
	NALU_TYPE_DPA      = 2,
	NALU_TYPE_DPB      = 3,
	NALU_TYPE_DPC      = 4,
	NALU_TYPE_IDR      = 5,
	NALU_TYPE_SEI      = 6,
	NALU_TYPE_SPS      = 7,
	NALU_TYPE_PPS      = 8,
	NALU_TYPE_AUD      = 9,
	NALU_TYPE_EOSEQ    = 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL     = 12,
	NALU_TYPE_SPSE     = 13
} NaluType;

#define BASELINE         66      //!< YUV 4:2:0/8  "Baseline"
#define MAIN             77      //!< YUV 4:2:0/8  "Main"
#define EXTENDED         88      //!< YUV 4:2:0/8  "Extended"
#define FREXT_HP        100      //!< YUV 4:2:0/8 "High"
#define FREXT_Hi10P     110      //!< YUV 4:2:0/10 "High 10"
#define FREXT_Hi422     122      //!< YUV 4:2:2/10 "High 4:2:2"
#define FREXT_Hi444     144      //!< YUV 4:4:4/14 "High 4:4:4"

// MPEG4
#define VIDOBJ_START_CODE		0x00000100
#define VIDOBJLAY_START_CODE	0x00000120
#define VISOBJSEQ_START_CODE	0x000001b0
#define VISOBJSEQ_STOP_CODE		0x000001b1
#define USERDATA_START_CODE		0x000001b2
#define GRPOFVOP_START_CODE		0x000001b3
#define VISOBJ_START_CODE		0x000001b5
#define VOP_START_CODE			0x000001b6

#define VIDOBJ_START_CODE_MASK      0x0000001f
#define VIDOBJLAY_START_CODE_MASK   0x0000000f
#define VIDO_SHORT_HEADER_MASK      0x000003ff
#define VIDO_SHORT_HEADER           0x8000
#define VIDO_SHORT_HEADER_END       0x3f

#define SIMPLE_PF_LEVEL0        0x01
#define SIMPLE_PF_LEVEL1        0x02
#define SIMPLE_PF_LEVEL2        0x03
#define ADV_SIMPLE_PF_LEVEL0    0xF0
#define ADV_SIMPLE_PF_LEVEL1    0xF1
#define ADV_SIMPLE_PF_LEVEL2    0xF2
#define ADV_SIMPLE_PF_LEVEL3    0xF3
#define ADV_SIMPLE_PF_LEVEL4    0xF4
#define ADV_SIMPLE_PF_LEVEL5    0xF5

#define VISOBJ_TYPE_VIDEO       1

#define VIDOBJLAY_AR_EXTPAR     15

#define VIDOBJLAY_SHAPE_RECTANGULAR     0
#define VIDOBJLAY_SHAPE_BINARY          1
#define VIDOBJLAY_SHAPE_BINARY_ONLY     2
#define VIDOBJLAY_SHAPE_GRAYSCALE       3

#define iMin(a, b)  ((a)<(b)?(a):(b))
#define iMax(a, b)  ((a)>(b)?(a):(b))

#define MAX_PARSING_BYTE    30

typedef struct header_info_s
{
    int width;
    int height;
    int resolution_exist;
} HeaderInfo;

extern int parsing_header(Bitstream *stream, HeaderInfo *header);

#endif
