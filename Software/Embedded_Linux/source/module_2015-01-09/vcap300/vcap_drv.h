#ifndef _VCAP_DRV_H_
#define _VCAP_DRV_H_

#include <linux/semaphore.h>

#include "vcap_dev.h"

#define VCAP_MODULE_VERSION     "0.3.7"

struct vcap_drv_info_t {
    struct vcap_dev_info_t  dev_info;
    struct semaphore        lock;
};

struct vcap_dev_drv_info_t {
    struct vcap_drv_info_t  drv_info;
#define VCAP_DRV_NAME_SIZE  16
    char name[VCAP_DRV_NAME_SIZE];
};

#endif  /* _VCAP_DRV_H_ */