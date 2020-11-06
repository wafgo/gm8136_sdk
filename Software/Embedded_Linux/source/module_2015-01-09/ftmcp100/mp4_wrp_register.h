#ifndef _MP4_WRP_REGISTER_H_
	#define _MP4__WRP_REGISTER_H_
	typedef struct WRPREGTag {
		unsigned int aim_hfp;		// 0x00	 8	R/W	Active Image H front porch
												// unit: 16 pixel 
		unsigned int aim_hbp;		// 0x04	8	R/W	Active Image H back porch
												// unit: 16 pixel 
		unsigned int Aim_width;		// 0x08	12	R/W	Active Image Width
												// unit: 16 pixel 
		unsigned int Aim_height;	// 0x0C	12	R/W	Active Image Height
												// unit: 16 pixel 
		unsigned int Ysmsa;	// 0x10	28	R/W	Y System memory starting address when 420 mode
										// unit: 4 bytes
		unsigned int Usmsa;	// 0x14	28	R/W	U System memory starting address when 420 mode
										// unit: 4 bytes
		unsigned int Vsmsa;	// 0x18	28	R/W	V System memory starting address when 420 mode
										// 							CbYCrY System memory starting address when 422 mode
										// unit: 4 bytes
		unsigned int Vfmt;		// 0x1C	6	R/W	"decode/encode, YUV/CbYCr
										// bit[11:8] : DMA ID for input image, must fit to encoder driver
										// bit7: 1: Bypass, 0: not bypass (default value =1)
										// bit6: always 1, &  "PARM6" register of mcp210 always 1
										// bit5: 0: decode, 1: encode, ==> only support encode now
										// bit4: planar/packed (Decoding only) ==> don't care now
										// bit3: Rectangle/Square ==> don't care now
										// bit[2:0] : Video format
										//  0: Planar 420  
										//  1: Planar 422 
										//  2: 222 
										//  3=211 
										//  4=111 
										//  5=Packed 422 (CbYCrY)"
										// ==> only support 0/5 now
		unsigned int pf_go;	//0x20
										// bit0: 1: go	R/W
										// bit31: 1, reset R/W
	} WRP_reg;
#endif
