#ifndef _ROTATION_HOST_H_
    #define _ROTATION_HOST_H_


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
    	uint32_t in_buf_base;
    	uint32_t out_buf_base;
    	uint32_t width;
        uint32_t height;
        uint32_t clockwise;
        //uint32_t stop_line_cnt;
        //uint32_t stop_mb_cnt;
        //uint32_t error_code;
        //uint32_t dbg[32];
    } rt_hw;
    
    typedef struct {
    	volatile MP4_COMMAND command;
    	rt_hw rthw;
    } Share_Node_RT;

    #ifdef TWO_P_INT
	    #ifdef RT_INT_GLOBAL
			#define RT_INT_EXT
		#else
			#define RT_INT_EXT extern
		#endif
		RT_INT_EXT void rotation_main_start(Share_Node_RT *node);
    #endif

#endif