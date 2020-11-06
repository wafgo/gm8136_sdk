#ifndef _CHECKSUM_EX_H_
    #define _CHECKSUM_EX_H_


	#ifndef TWO_P_INT
	    //#define HOST_MODE
	#endif

    #ifdef TWO_P_INT
        #include "share_mem.h"
		#include "portab.h"
    #else
        #include "../fmpeg4_driver/common/share_mem.h"
    #endif
    
    typedef struct {
        uint32_t base_address;
    	uint32_t buf_base;
    	uint32_t len;
        uint32_t type;
        uint32_t result;
        //uint32_t data[0x30];
        //uint32_t raw[0x30];
    } cs_hw;
    
    typedef struct {
    	volatile MP4_COMMAND command;
    	cs_hw cshw;
    } Share_Node_CS;

    #ifdef TWO_P_INT
	    #ifdef CS_INT_GLOBAL
			#define CS_INT_EXT
		#else
			#define CS_INT_EXT extern
		#endif
		CS_INT_EXT void cs_main_start(Share_Node_CS *node);
    #endif

#endif