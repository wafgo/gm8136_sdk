//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2010  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:  MDINDLY.H
// Description 		:  This file contains typedefine for the driver files	
// Ref. Docment		: 
// Revision History 	:

#ifndef		__MDINDLY_H__
#define		__MDINDLY_H__

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
extern VWORD usDelay, msDelay;

// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------

MDIN_ERROR_t MDINDLY_10uSec(WORD delay);
MDIN_ERROR_t MDINDLY_mSec(WORD delay);


#endif	/* __MDINDLY_H__ */
