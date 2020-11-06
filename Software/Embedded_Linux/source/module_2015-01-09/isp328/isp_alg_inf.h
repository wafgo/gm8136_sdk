/**
 * @file isp_alg_inf.h
 * ISP algorithm module interface header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
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
