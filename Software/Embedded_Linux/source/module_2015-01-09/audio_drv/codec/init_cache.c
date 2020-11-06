#include "fa410.h"

#define LINUX

void init_cache( void )
{

#ifdef LINUX
   int r0=3;

    // :"cc"
    //"MCR		p15, 0, %[r0], c5, c0, 1 \n"
    //"MCR		p15, 0, %[r0], c5, c0, 0 \n"
    asm volatile(
        "MCR		p15, 0, %[r0], c7, c10, 0 \n"
         :
         :[r0]"r"(0x03)
        );

    // :"cc"
    asm volatile(
        "MCR		p15, 0, %[r0], c6, c0, 1 \n"
        "MCR		p15, 0, %[r0], c6, c0, 0 \n"
         :
         :[r0]"r"(0x3F)
        );

   // :	"cc"
   asm volatile(
        "MCR		p15, 0, %[r0], c2, c0, 1 \n"
        "MCR		p15, 0, %[r0], c2, c0, 0 \n"
        "MCR		p15, 0, %[r0], c3, c0, 0 \n"
        "MRC		p15, 0, %[r0], c1, c0, 0 "
        :
        :	[r0]"r"(0x01)
      );
   asm volatile(
        "ORR		%[r0], %[r0], %[in01] \n"
        "ORR		%[r0], %[r0], #0xC \n"
        "ORR		%[r0], %[r0], #0x1 "
        :	[r0]"+r"(r0)
        : [in01]"r"(0x1000)
      );

   // :	"cc"
   asm volatile(
        "MCR		p15, 0, %[r0], c1, c0, 0 "
        :
        :	[r0]"r"(r0)
      );

#else
	__asm
	{
        MOV		r0, #0x3
        MCR		p15, 0, r0, c5, c0, 1;	//set privilege of instruction region
        MCR		p15, 0, r0, c5, c0, 0;	//set privilege of data region
        MCR		p15, 0, r0, c7, c10, 0;	//clean data cache

        MOV		r0, #0x3F
        MCR		p15, 0, r0, c6, c0, 1;	//4G instruction region 0 based on 0x0
        MCR		p15, 0, r0, c6, c0, 0;	//4G data region 0 based on 0x0

        MOV		r0, #0x1
        MCR		p15, 0, r0, c2, c0, 1;	//set cacheable attribute of instruction regions
        MCR		p15, 0, r0, c2, c0, 0;	//set cacheable attribute of data regions
        MCR		p15, 0, r0, c3, c0, 0;	//set bufferable attribute of data regions

        MRC		p15, 0, r0, c1, c0, 0;
        ORR		r0, r0, #0x1000;
        ORR		r0, r0, #0xC;
        ORR		r0, r0, #0x1;
        MCR		p15, 0, r0, c1, c0, 0;	//turn on U/D cache and MPU
	}
#endif
}
