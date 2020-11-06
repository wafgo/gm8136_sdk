///*****************************************
//  Copyright (C) 2009-2014
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   >mcu.h<
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2009/08/24
//   @fileversion: CAT6611_SAMPLEINTERFACE_1.12
//******************************************/

#ifndef _MCU_H_
#define _MCU_H_

#ifdef _MCU_
#define _1PORT_
//#define _2PORT_
//#define _3PORT_
// #define _4PORT_
//#define _8PORT_

//#define _ATC_
//#define _DEBUG_
//#define _FORCEHDCPON_
//#define _FORCEHDCPOFF_
#define _REALHDCP_
#define _ENMULTICH_
#define _ENLED_
#define _HPDMOS_
#define _ENPARSE_
//#define _RXPOLLING_
//#define _TESTMODE_

#ifdef _ENPARSE_
    #define _EDIDI2C_

    #ifdef _1PORT_
        #define _EDIDCOPY_
    #else
        #define _EDIDPARSE_
    #endif

    #ifdef _EDIDPARSE_
//  	#define _ENCEC_
		#define _MAXEDID_
    #endif
#endif

#ifdef _ATC_
    #define _FORCEHDCPOFF_
#endif

#ifndef _ENLED_
    #define _DBGLED_
#endif

#ifdef _TESTMODE_
 	#define _EDIDI2C_
#endif

///////////////////////////////////////////////////////////////////////////////
// Include file
///////////////////////////////////////////////////////////////////////////////
//#include <reg51.h>
//#include <math.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <linux/string.h>
#include "typedef.h"
///////////////////////////////////////////////////////////////////////////////
// Type Definition
///////////////////////////////////////////////////////////////////////////////
//typedef bit BOOL, bool ;
//typedef unsigned char BYTE, byte;
typedef unsigned int UINT, uint;
///////////////////////////////////////////////////////////////////////////////
// Constant Definition
///////////////////////////////////////////////////////////////////////////////
#define TX0DEV			0
#define TX1DEV			1
#define TX2DEV			2
#define TX3DEV			3
#define TX4DEV			4
#define TX5DEV			5
#define TX6DEV			6
#define TX7DEV			7
#define RXDEV			8

#ifdef _1PORT_
#define TXDEVCOUNT 1
#endif // _1PORT_

#ifdef _2PORT_
#define TXDEVCOUNT 2
#endif // _2PORT_

#ifdef _3PORT_
#define TXDEVCOUNT 3
#endif // _3PORT_

#ifdef _4PORT_
#define TXDEVCOUNT 4
#endif // _4PORT_

#ifdef _5PORT_
#define TXDEVCOUNT 5
#endif // _5PORT_

#ifdef _6PORT_
#define TXDEVCOUNT 6
#endif // _6PORT_

#ifdef _7PORT_
#define TXDEVCOUNT 7
#endif // _7PORT_

#ifdef _8PORT_
#define TXDEVCOUNT 8
#endif // _8PORT_

#ifdef _1PORT_
    #define TX0ADR	0x98
#endif

#ifdef _2PORT_
    #ifdef _DEBUG_
	    #define TX0ADR	0x98
        #define TX1ADR	0x9A
    #else
	    #define TX0ADR	0x98
        #define TX1ADR	0x98
    #endif
#endif

#ifdef _3PORT_
    #ifdef _DEBUG_
        #define TX0ADR	0x98
	    #define TX1ADR	0x9A
    	#define TX2ADR	0x98
	#else
    	#define TX0ADR	0x98
		#define TX1ADR	0x98
    	#define TX2ADR	0x98
	#endif
#endif

#ifdef _4PORT_
    #ifdef _DEBUG_
        #define TX0ADR	0x98
	    #define TX1ADR	0x9A
    	#define TX2ADR	0x98
    	#define TX3ADR	0x9A
	#else
    	#define TX0ADR	0x98
		#define TX1ADR	0x98
    	#define TX2ADR	0x98
    	#define TX3ADR	0x98
	#endif
#endif

#ifdef _8PORT_
	#define TX0ADR		0x98
    #define TX1ADR		0x9A
    #define TX2ADR		0x98
    #define TX3ADR		0x9A
    #define TX4ADR		0x98
    #define TX5ADR		0x9A
    #define TX6ADR		0x98
    #define TX7ADR		0x9A
#endif

#define RXADR			0x90

#define EDID_ADR		0xA0	// alex 070321

#define DELAY_TIME		1		// unit = 1 us;
#define IDLE_TIME		100		// unit = 1 ms;

#define HIGH			1
#define LOW				0

#ifdef _HPDMOS_
    #define HPDON		0
	#define HPDOFF		1
#else
    #define HPDON		1
	#define HPDOFF		0
#endif
///////////////////////////////////////////////////////////////////////////////
// 8051 Definition
///////////////////////////////////////////////////////////////////////////////
sbit RX_HPD				= P3^3;
sbit HW_RSTN			= P3^4;

#ifdef _ENCEC_
    #define CECDEV	0
#endif

#ifdef _1PORT_
    #define DevNum	1
	#define LOOPMS	20
    #ifdef _DEBUG_
    	sbit TX0_SDA_PORT	= P1^1;
    	sbit RX_SDA_PORT	= P1^1;
    	sbit SCL_PORT		= P1^5;
	#else
	    sbit TX0_SDA_PORT	= P1^0;
    	sbit RX_SDA_PORT	= P1^4;
    	sbit SCL_PORT		= P1^5;
	#endif
#endif

#ifdef _2PORT_
    #define DevNum	2
	#define LOOPMS	20
    #ifdef _DEBUG_
    	sbit TX0_SDA_PORT	= P1^1;
    	sbit TX1_SDA_PORT	= P1^1;
    	sbit RX_SDA_PORT	= P1^1;
    	sbit SCL_PORT		= P1^5;
	#else
	    sbit TX0_SDA_PORT	= P1^0;
    	sbit TX1_SDA_PORT	= P1^1;
    	sbit RX_SDA_PORT	= P1^4;
    	sbit SCL_PORT		= P1^5;
	#endif
#endif

#ifdef _3PORT_
    #define DevNum	3
	#define LOOPMS	30
    #ifdef _DEBUG_
    	sbit TX0_SDA_PORT	= P1^1;
    	sbit TX1_SDA_PORT	= P1^1;
    	sbit TX2_SDA_PORT	= P1^1;
     	sbit RX_SDA_PORT	= P1^1;
    	sbit SCL_PORT		= P1^5;
	#else
	    sbit TX0_SDA_PORT	= P1^0;
    	sbit TX1_SDA_PORT	= P1^1;
    	sbit TX2_SDA_PORT	= P1^2;
    	sbit RX_SDA_PORT	= P1^4;
    	sbit SCL_PORT		= P1^5;
	#endif
#endif

#ifdef _4PORT_
    #define DevNum	4
	#define LOOPMS	40
    #ifdef _DEBUG_
    	sbit TX0_SDA_PORT	= P1^1;
    	sbit TX1_SDA_PORT	= P1^1;
    	sbit TX2_SDA_PORT	= P1^1;
    	sbit TX3_SDA_PORT	= P1^1;
    	sbit RX_SDA_PORT	= P1^1;
    	sbit SCL_PORT		= P1^5;
	#else
	    sbit TX0_SDA_PORT	= P1^0;
    	sbit TX1_SDA_PORT	= P1^1;
    	sbit TX2_SDA_PORT	= P1^2;
    	sbit TX3_SDA_PORT	= P1^3;
    	sbit RX_SDA_PORT	= P1^4;
    	sbit SCL_PORT		= P1^5;
	#endif
#endif

#ifdef _8PORT_
    #define DevNum	8
	#define LOOPMS	80
    #ifdef _DEBUG_
    	sbit TX0_SDA_PORT	= P1^1;
    	sbit TX1_SDA_PORT	= P1^1;
    	sbit TX2_SDA_PORT	= P1^1;
    	sbit TX3_SDA_PORT	= P1^1;
    	sbit TX4_SDA_PORT	= P1^1;
    	sbit TX5_SDA_PORT	= P1^1;
    	sbit TX6_SDA_PORT	= P1^1;
    	sbit TX7_SDA_PORT	= P1^1;
    	sbit RX_SDA_PORT	= P1^1;
    	sbit SCL_PORT		= P1^5;
	#else
        sbit TX0_SDA_PORT	= P1^0;
        sbit TX1_SDA_PORT	= P1^0;
	    sbit TX2_SDA_PORT	= P1^1;
	    sbit TX3_SDA_PORT	= P1^1;
        sbit TX4_SDA_PORT	= P1^2;
        sbit TX5_SDA_PORT	= P1^2;
	    sbit TX6_SDA_PORT	= P1^3;
	    sbit TX7_SDA_PORT	= P1^3;
    	sbit RX_SDA_PORT	= P1^4;
    	sbit SCL_PORT		= P1^5;
	#endif
#endif

//sbit RX_LED		= P4^0;

#ifdef _1PORT_
    sbit TX0_LED	= P0^0;
#endif

#ifdef _2PORT_
    sbit TX0_LED	= P0^0;
    sbit TX1_LED	= P0^1;
#endif

#ifdef _3PORT_
    sbit TX0_LED	= P0^1;
    sbit TX1_LED	= P0^0;
    sbit TX2_LED	= P0^2;
#endif

#ifdef _4PORT_
    sbit TX0_LED	= P0^1;
    sbit TX1_LED	= P0^0;
    sbit TX2_LED	= P0^2;
    sbit TX3_LED	= P0^3;
#endif

#ifdef _8PORT_
    sbit TX0_LED	= P0^4;
    sbit TX1_LED	= P0^5;
    sbit TX2_LED	= P0^6;
    sbit TX3_LED	= P0^7;
    sbit TX4_LED	= P0^3;
    sbit TX5_LED	= P0^2;
    sbit TX6_LED	= P0^1;
    sbit TX7_LED	= P0^0;
#endif

// alex 070320
// for edid EEPROM i2c bus
sbit EDID_SCL 		= P1^6;
sbit EDID_SDA 		= P1^7;
sbit EDID_WP 		= P3^5;
void SelectHDMIDev(BYTE dev) ;
BYTE HDMITX_ReadI2C_Byte(BYTE RegAddr);
SYS_STATUS HDMITX_WriteI2C_Byte(BYTE RegAddr, BYTE d);
SYS_STATUS HDMITX_ReadI2C_ByteN(BYTE RegAddr, BYTE *pData, int N);
SYS_STATUS HDMITX_WriteI2C_ByteN(SHORT RegAddr, BYTE *pData, int N);

#ifdef _DEBUG_
#define ErrorF printf
#else
#define ErrorF
#endif

void DelayMS(USHORT ms) ;

#endif // _MCU_
#endif	// _MCU_H_
