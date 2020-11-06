///*****************************************
//  Copyright (C) 2009-2014
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   >hdmitx.h<
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2009/08/24
//   @fileversion: CAT6611_SAMPLEINTERFACE_1.12
//******************************************/

#ifndef _HDMITX_H_
#define _HDMITX_H_

#define HARRY_CHANGE    /* Harry */

#define SUPPORT_DEGEN
#define SUPPORT_SYNCEMB

#define SUPPORT_EDID
#define SUPPORT_HDCP
#define SUPPORT_INPUTRGB
#define SUPPORT_INPUTYUV444
#define SUPPORT_INPUTYUV422


#if defined(SUPPORT_INPUTYUV444) || defined(SUPPORT_INPUTYUV422)
#define SUPPORT_INPUTYUV
#endif

#include "mcu.h"
#include "typedef.h"
#include "cat6611_sys.h"
#include "cat6611_drv.h"
#include "edid.h"

#ifdef HARRY_CHANGE
extern int support_syncemb;
extern int support_degen;
#define DelayMS mdelay
#endif

//////////////////////////////////////////////////////////////////////
// Function Prototype
//////////////////////////////////////////////////////////////////////


// dump
#ifdef _DEBUG_
void DumpCat6611Reg() ;
#endif
// I2C

//////////////////////////////////////////////////////////////////////////////
// main.c
//////////////////////////////////////////////////////////////////////////////

#endif // _HDMITX_H_

