//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2010  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:	MDINDLY.C
// Description 		:
// Ref. Docment		: 
// Revision History 	:

// ----------------------------------------------------------------------
// Include files
// ----------------------------------------------------------------------
#include	"mdin3xx.h"
#include	"..\common\ticortex.h"	// cpu dependent for making delay

// ----------------------------------------------------------------------
// Struct/Union Types and define
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Global Data section variables
// ----------------------------------------------------------------------
VWORD usDelay, msDelay;

// ----------------------------------------------------------------------
// External Variable 
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Prototype Functions
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static functions
// ----------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for delay (usec and msec)
// You must make functions which is defined below.
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINDLY_10uSec(WORD delay)
{
	TimerLoadSet(TIMER0_BASE, TIMER_B, SysCtlClockGet()/100000);	// set period to 10us

	usDelay = delay; TimerEnable (TIMER0_BASE, TIMER_B);
	while (usDelay); TimerDisable(TIMER0_BASE, TIMER_B);
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINDLY_mSec(WORD delay)
{
	TimerLoadSet(TIMER0_BASE, TIMER_B, SysCtlClockGet()/1000);		// set period to 1ms

	msDelay = delay; TimerEnable (TIMER0_BASE, TIMER_B);
	while (msDelay); TimerDisable(TIMER0_BASE, TIMER_B);
	return MDIN_NO_ERROR;
}

/*  FILE_END_HERE */
