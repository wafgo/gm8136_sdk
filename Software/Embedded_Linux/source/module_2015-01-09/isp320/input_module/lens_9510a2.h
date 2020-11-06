/**
 * @file lens_9510a2.h
 * header file of lens_9510a2.c
 *
 * Copyright (C) 2010 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2014/05/07 09:36:34 $
 *
 * ChangeLog:
 *  $Log: lens_9510a2.h,v $
 *  Revision 1.1  2014/05/07 09:36:34  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.2  2012/05/07 06:16:20  ricktsao
 *  1. include "ftpwmtmr010.h"
 *
 *  Revision 1.1  2012/02/15 06:40:27  ricktsao
 *  no message
 *
 *
 */

#ifndef __LENS_9510A2_H__
#define __LENS_9510A2_H__

#include "isp_input_inf.h"
#include "ftpwmtmr010.h"

//============================================================================
// external functions
//============================================================================
extern int  lens_dev_construct(lens_dev_t *pdev);
extern void lens_dev_deconstruct(lens_dev_t *pdev);

#endif /*__LENS_9510A2_H__*/
