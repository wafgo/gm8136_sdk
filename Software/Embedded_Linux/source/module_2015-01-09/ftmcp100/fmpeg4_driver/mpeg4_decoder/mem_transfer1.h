#ifndef _MEM_TRANSFER1_H
	#define _MEM_TRANSFER1_H
#ifdef LINUX
	#include "decoder.h"
	#include "../common/mp4.h"
#else
	#include "decoder.h"
#endif

	#ifdef MEM_TRANSFER1_GLOBALS
		#define MEM_TRANSFER1_EXT
	#else
		#define MEM_TRANSFER1_EXT extern
	#endif

	MEM_TRANSFER1_EXT void transfer8x8_copy_ben(DECODER * const dec,
				int curoffset,
				uint8_t * cur,
				uint8_t * ref,
				int refx,
				int refy);
#endif
