/**
 * @file lens_9510a2.h
 * Largan lens 9510A2 driver header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
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
