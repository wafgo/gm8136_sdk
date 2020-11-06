/**
 * @file cfg_cfg.h
 * ISP cfg parameter header file
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2013/09/26 02:06:48 $
 *
 * ChangeLog:
 *  $Log: isp_cfg.h,v $
 *  Revision 1.1  2013/09/26 02:06:48  ricktsao
 *  no message
 *
 */

#ifndef __ISP_CFG_H__
#define __ISP_CFG_H__

#include "isp_common.h"

//=============================================================================
// extern functions
//=============================================================================
extern int isp_cfg_load_file(char *fname, cfg_info_t *pcfg);

#endif /* __ISP_CFG_H__ */
