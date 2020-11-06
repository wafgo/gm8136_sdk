/**
 * @file isp_proc.h
 * ISP driver proc header file
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2013/09/26 02:06:48 $
 *
 * ChangeLog:
 *  $Log: isp_proc.h,v $
 *  Revision 1.1  2013/09/26 02:06:48  ricktsao
 *  no message
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
