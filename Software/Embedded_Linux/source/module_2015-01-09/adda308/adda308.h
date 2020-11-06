#ifndef __ADDA308_H__
#define __ADDA308_H__

#include <adda308_api.h>

#define ADDA308_PROC_NAME "adda308"

//debug helper
#if DEBUG_ADDA308
#define DEBUG_ADDA308_PRINT(FMT, ARGS...)    printk(FMT, ##ARGS)
#else
#define DEBUG_ADDA308_PRINT(FMT, ARGS...)
#endif

typedef struct adda308_priv {
    int                 adda308_fd;
    u32                 adda308_reg;
} adda308_priv_t;

/*
 * extern global variables
 */
extern adda308_priv_t   priv;

#endif //end of __ADDA308_H__
