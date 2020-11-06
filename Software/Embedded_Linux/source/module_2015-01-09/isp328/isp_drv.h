/**
 * @file isp_drv.h
 * ISP driver layer header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
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
