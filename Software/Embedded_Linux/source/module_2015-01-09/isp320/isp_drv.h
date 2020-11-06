/**
 * @file isp_drv.h
 * ISP driver layer header file
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.11 $
 * $Date: 2014/11/27 09:00:21 $
 *
 * ChangeLog:
 *  $Log: isp_drv.h,v $
 *  Revision 1.11  2014/11/27 09:00:21  ricktsao
 *  add ISP_IOC_GET_MRNR_EN
 *         ISP_IOC_GET_MRNR_PARAM
 *         ISP_IOC_GET_TMNR_EN
 *         ISP_IOC_GET_TMNR_EDG_EN
 *         ISP_IOC_GET_TMNR_LEARN_EN
 *         ISP_IOC_GET_TMNR_PARAM
 *         ISP_IOC_GET_TMNR_EDG_TH
 *         ISP_IOC_GET_SP_MASK / ISP_IOC_SET_SP_MASK
 *         ISP_IOC_GET_MRNR_OPERATOR / ISP_IOC_SET_MRNR_OPERATOR
 *
 *  Revision 1.10  2014/08/01 11:31:04  ricktsao
 *  add image pipe control IOCTLs
 *
 *  Revision 1.9  2014/07/04 10:13:15  ricktsao
 *  support auto adjust black offset
 *
 *  Revision 1.8  2014/06/11 07:12:11  ricktsao
 *  add err_panic module parameter
 *
 *  Revision 1.7  2014/06/03 02:53:31  jason_ha
 *  *** empty log message ***
 *
 *  Revision 1.6  2014/05/26 03:31:35  ricktsao
 *  no message
 *
 *  Revision 1.5  2014/05/20 09:48:46  jason_ha
 *  Update brickcom V1.0.1
 *
 *  Revision 1.4  2014/05/20 02:13:24  ricktsao
 *  move SPI control APIs from core driver to input module
 *
 *  Revision 1.3  2013/11/05 09:21:21  ricktsao
 *  no message
 *
 *  Revision 1.2  2013/10/22 09:16:04  ricktsao
 *  add ISP export API
 *
 *  Revision 1.1  2013/09/26 02:06:48  ricktsao
 *  no message
 *
 */

#ifndef __ISP_DRV_H__
#define __ISP_DRV_H__

#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include "isp_dev.h"

//=============================================================================
// driver version
//=============================================================================
#define ISP_DRIVER_VER   ((1 << 16) | (4 << 8) | (0))
#define ISPVER_MAJOR     ((ISP_DRIVER_VER >> 16)& 0xffff)
#define ISPVER_MINOR     ((ISP_DRIVER_VER >> 8) & 0xff)
#define ISPVER_MINOR2     (ISP_DRIVER_VER & 0x00ff)

//=============================================================================
// struct & flag definition
//=============================================================================
typedef struct isp_drv_info {
    isp_dev_info_t dev_info;
    struct miscdevice miscdev;

    // proc entries
    struct proc_dir_entry *proc_root;
    struct proc_dir_entry *proc_info;
    struct proc_dir_entry *proc_dump_reg;
    struct proc_dir_entry *proc_dump_cfg;
    struct proc_dir_entry *proc_command;
    struct proc_dir_entry *proc_help;
    struct proc_dir_entry *proc_dbg_mode;
} isp_drv_info_t;

//=============================================================================
// extern functions
//=============================================================================
extern isp_dev_info_t *isp_drv_get_dev_info(void);

#endif /* __ISP_DRV_H__ */
