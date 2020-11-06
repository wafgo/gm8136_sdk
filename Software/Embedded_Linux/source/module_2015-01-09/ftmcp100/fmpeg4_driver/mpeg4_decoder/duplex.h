#ifndef _DUPLEX_H_
	#define _DUPLEX_H_

	typedef enum
	{
	#if 0
		TYPE_NONE = 0,
		TYPE_MP4_ENCODER = 1,
		TYPE_MP4_DECODER = 2,
		TYPE_JPG_ENCODER = 3,
		TYPE_JPG_DECODER = 4
    #else
        TYPE_NONE = -1,
		TYPE_MPEG4_ENCODER = 0,
		TYPE_MPEG4_DECODER = 1,
		TYPE_JPEG_ENCODER = 2,
		TYPE_JPEG_DECODER = 3,
		TYPE_MCP100_ROTATION = 4,
    #endif
	}
	ACTIVE_TYPE;


	#ifdef DUPLEX_GLOBALS
		#define DUPLEX_EXT
	#else
		#define DUPLEX_EXT extern
	#endif

	DUPLEX_EXT void duplex_switch(void * codec, ACTIVE_TYPE cur_type, ACTIVE_TYPE last_type);
	DUPLEX_EXT void duplex_end(void * codec, ACTIVE_TYPE type);

#endif							/* _DUPLEX_H_ */
