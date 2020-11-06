#ifndef _IOCTL_IVS_H_

	#define _IOCTL_IVS_H_
     
    #define IVS_VER           	0x00200100
    //                            HIIFFFBB
    //                            H 		huge change
    //                             II 		interface change
    //                               FFF 	functional modified or bug fixed
    //                                  BB 	branch for customer request
	
    #define IVS_HSI_CONVERSION     	0x4170
    #define IVS_DE_INTERLEAVE_YC    0x4172
    #define IVS_INTEGRAL_IMAGE    	0x4177
    #define IVS_SAD                 0x4178
	#define IVS_INIT                0x4179    
	
    #define IVS_DEV  				"/dev/ivs"
       
    typedef struct ivs_param_t{
	
		int img_width  ;
		int img_height ;
	
		int operation ;
		int blk_size  ;
		int swap	   ;	
		int endian    ;

	} ivs_param_t;

	typedef struct ivs_info_t{
	
		int fd;
		unsigned char *mmap_addr;
		
	} ivs_info_t;
#endif
