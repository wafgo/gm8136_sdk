/**
 * @file isp_proc.h
 * ISP driver proc header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#ifndef __ISP_PROC_H__
#define __ISP_PROC_H__

#include "isp_drv.h"

//=============================================================================
// extern functions
//=============================================================================
extern int  isp_drv_proc_init(isp_drv_info_t *pdrv_info);
extern void isp_drv_proc_remove(isp_drv_info_t *pdrv_info);

#endif /* __ISP_PROC_H__ */
