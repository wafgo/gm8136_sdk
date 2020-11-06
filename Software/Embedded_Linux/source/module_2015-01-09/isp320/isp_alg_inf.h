/**
 * @file isp_alg_inf.h
 * ISP algorithm module interface header file
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.3 $
 * $Date: 2014/05/05 08:53:55 $
 *
 * ChangeLog:
 *  $Log: isp_alg_inf.h,v $
 *  Revision 1.3  2014/05/05 08:53:55  ricktsao
 *  Add IOCTL to get driver version
 *
 *  Revision 1.2  2014/03/26 06:24:06  ricktsao
 *  1. divide 3A algorithms from core driver.
 *
 *  Revision 1.1  2013/09/26 02:06:48  ricktsao
 *  no message
 *
 */

#ifndef __ISP_ALG_INF_H__
#define __ISP_ALG_INF_H__

#include "isp_common.h"

//=============================================================================
// interface version
//=============================================================================
#define ISP_ALG_INF_VER   ((1<<16)|(0<<8)|(0<<4)|0)  // 1.0.00

//=============================================================================
// struct & flag definition
//=============================================================================
#define ALG_MAX_NAME_SIZE   32

typedef struct alg_module {
    char name[ALG_MAX_NAME_SIZE];
    void *private;
    void *pdev_info; // pointer to isp_dev_info_t
    bool enable;

    int  (*fn_init)(void *dev);
    void (*fn_reset)(void);
    void (*fn_release)(void);
    void (*fn_enable)(void);
    void (*fn_disable)(void);
    int  (*fn_resize)(int width, int height);
    int  (*fn_ioctl)(int cmd, void *arg);
    int  (*fn_process)(void);
} alg_module_t;

//=============================================================================
// extern functions
//=============================================================================
extern u32 isp_get_alg_inf_ver(void);
extern int isp_register_algorithm(alg_module_t *palg_mod, char *name);
extern int isp_unregister_algorithm(alg_module_t *palg_mod);
extern alg_module_t* isp_get_algorithm(char *name);

#endif /* __ISP_ALG_INF_H__ */
