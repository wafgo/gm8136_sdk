#ifndef SHARE_MEM_H
	#define SHARE_MEM_H
	#include "portab.h"


	// --------------------------------------------------------------------
	//		Share_Node->command
	// --------------------------------------------------------------------
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
		CHECKSUM = 9,
        ROTATION = 10
	} MP4_COMMAND;


	#define SHARE_NODE_SIZE			0x480//0x130
	#define SHARE_NODE_NO			2

	typedef struct {
		volatile MP4_COMMAND command;
		char datas[SHARE_NODE_SIZE - 4];
	} Share_Node;
#endif
