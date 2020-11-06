#ifndef __FMCP_TYPE_H
#define __FMCP_TYPE_H

//#define ENABLE_CHECKSUM

#ifdef ENABLE_ROTATION
    #define FMCP_MAX_TYPE   5
#else
    #define FMCP_MAX_TYPE   4
#endif

typedef enum
{
#if 0
		TYPE_NONE = 0,
		TYPE_MP4_ENCODER = 1,
		TYPE_MP4_DECODER = 2,
		TYPE_JPG_ENCODER = 3,
		TYPE_JPG_DECODER = 4,
		TYPE_MAX = 5
#else
    TYPE_NONE = -1,
    TYPE_MPEG4_ENCODER = 0,
    TYPE_MPEG4_DECODER = 1,
    TYPE_JPEG_ENCODER = 2,
    TYPE_JPEG_DECODER = 3,
	#ifdef ENABLE_ROTATION
        TYPE_MCP100_ROTATION = 4,
        #ifdef ENABLE_CHECKSUM
            TYPE_CHECK_SUM = 5,
        #endif
    #elif defined(ENABLE_CHECKSUM)
        TYPE_CHECK_SUM = 4,
    #endif
    TYPE_MAX
#endif
}
ACTIVE_TYPE;
#endif
