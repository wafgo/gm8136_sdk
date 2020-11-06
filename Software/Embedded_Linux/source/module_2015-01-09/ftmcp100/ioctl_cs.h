#ifndef __IOCTL_CHECKSUM_H_
	#define __IOCTL_CHECKSUM_H_

	/*
	Version 0.0.1: initial version
	Version 0.0.2: change defition of timer
	Version 1.0.3: when rotation enable, checksum must be disable
	*/
    #define CHECKSUM_VER        0x10000300
	#define CHECKSUM_VER_MAJOR  ((CHECKSUM_VER>>28)&0x0F)     // huge change
	#define CHECKSUM_VER_MINOR  ((CHECKSUM_VER>>20)&0xFF)     // interface change
	#define CHECKSUM_VER_MINOR2 ((CHECKSUM_VER>>8)&0x0FFF)    // functional modified or buf fixed
    #define CHECKSUM_VER_BRANCH (CHECKSUM_VER&0xFF)           // branch for customer request

    #define MCS_MODULE_NAME     "CS"

    typedef struct {
        unsigned int ip_base;
        //unsigned int id;
		unsigned int buffer_pa;
        unsigned int buffer_len;
        unsigned int result;
        unsigned int type;
	} FMCP_CS_PARAM;

#endif
