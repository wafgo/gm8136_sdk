#ifndef __IOCTL_ROTATION_H_
	#define __IOCTL_ROTATION_H_

	/*
	Version 0.0.1: initial version
	*/
    #define ROTATION_VER        0x00000100
	#define ROTATION_VER_MAJOR  ((ROTATION_VER>>28)&0x0F)     // huge change
	#define ROTATION_VER_MINOR  ((ROTATION_VER>>20)&0xFF)     // interface change
	#define ROTATION_VER_MINOR2 ((ROTATION_VER>>8)&0x0FFF)    // functional modified or buf fixed
    #define ROTATION_VER_BRANCH (ROTATION_VER&0xFF)           // branch for customer request

    #define RT_MODULE_NAME      "RT"
    #define RT_MAX_CHANNEL      70
    #define FROTATION_MINOR     60
    
    //#define HOST_MODE

    typedef struct {
        unsigned int u32BaseAddress;
		unsigned int u32InputBufferPhy;
		unsigned int u32OutputBufferPhy;
		unsigned int u32Width;
		unsigned int u32Height;
        unsigned int u32Clockwise;
	} FMCP_RT_PARAM;

#endif
