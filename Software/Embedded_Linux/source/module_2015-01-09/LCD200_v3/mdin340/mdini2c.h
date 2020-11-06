//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2010  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:  MDINI2C.H
// Description 		:  This file contains typedefine for the driver files	
// Ref. Docment		: 
// Revision History 	:

#ifndef		__MDINI2C_H__
#define		__MDINI2C_H__

// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------
#ifndef		__MDINTYPE_H__
#include	 "mdintype.h"
#endif
#ifndef		__MDINBUS_H__
#include	 "mdinbus.h"
#endif

// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Exported Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------

MDIN_ERROR_t MDINI2C_SetPageID(WORD nID);
MDIN_ERROR_t MDINI2C_MultiWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
MDIN_ERROR_t MDINI2C_RegWrite(BYTE nID, DWORD rAddr, WORD wData);
MDIN_ERROR_t MDINI2C_MultiRead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
MDIN_ERROR_t MDINI2C_RegRead(BYTE nID, DWORD rAddr, PWORD rData);
MDIN_ERROR_t MDINI2C_RegField(BYTE nID, DWORD rAddr, WORD bPos, WORD bCnt, WORD bData);


#endif	/* __MDINI2C_H__ */
