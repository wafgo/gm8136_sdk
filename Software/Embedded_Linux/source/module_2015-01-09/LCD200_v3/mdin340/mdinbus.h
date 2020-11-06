//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2010  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:  MDINBUS.H
// Description 		:  This file contains typedefine for the driver files	
// Ref. Docment		: 
// Revision History 	:

#ifndef		__MDINBUS_H__
#define		__MDINBUS_H__

// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------
#ifndef		__MDINTYPE_H__
#include	 "mdintype.h"
#endif
#ifndef		__MDINI2C_H__
#include	 "mdini2c.h"
#endif

#if	defined(SYSTEM_USE_MDIN380)&&defined(SYSTEM_USE_BUS_HIF)
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
#if   CPU_ACCESS_BUS_NBYTE == 1 	// bus width = 16bit
#define		MDIN380_BASE_ADDR		((VPWORD)0xa0000000)	// cpu dependent
#else								// bus width = 8bit
#define		MDIN380_BASE_ADDR		((VPBYTE)0xa0000000)
#endif

// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------

MDIN_ERROR_t MDINBUS_SetPageID(WORD nID);
MDIN_ERROR_t MDINBUS_MultiWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
MDIN_ERROR_t MDINBUS_RegWrite(BYTE nID, DWORD rAddr, WORD wData);
MDIN_ERROR_t MDINBUS_MultiRead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
MDIN_ERROR_t MDINBUS_RegRead(BYTE nID, DWORD rAddr, PWORD rData);
MDIN_ERROR_t MDINBUS_RegField(BYTE nID, DWORD rAddr, WORD bPos, WORD bCnt, WORD bData);
MDIN_ERROR_t MDINBUS_SetDMA(DWORD rAddr, WORD bytes, BOOL dir);

MDIN_ERROR_t MDINBUS_DMAWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
MDIN_ERROR_t MDINBUS_DMARead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
#endif	/* SYSTEM_USE_BUS_HIF */


#endif	/* __MDINBUS_H__ */
