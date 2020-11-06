#ifndef _CHECKSUM_INT_H_
    #define _CHECKSUM_INT_H_

    #include "portab.h"
    #include "share_mem.h"
    
    typedef struct {
        uint32_t base_address;
    	uint32_t buf_base;
    	uint32_t len;
        uint32_t type;
        uint32_t result;
        uint32_t log_idx;
        uint32_t data[0x30];
        uint32_t last_addr;
    } cs_hw;
    
    typedef struct {
    	volatile MP4_COMMAND command;
    	cs_hw cshw;
    } Share_Node_CS;

    #ifdef CS_INT_GLOBAL
		#define CS_INT_EXT
	#else
		#define CS_INT_EXT extern
	#endif

	CS_INT_EXT void cs_main_start(Share_Node_CS *node);

#endif