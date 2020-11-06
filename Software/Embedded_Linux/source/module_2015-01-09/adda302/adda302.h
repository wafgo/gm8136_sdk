#ifndef __ADDA302_H__
#define __ADDA302_H__

#include <adda302_api.h>

#define ADDA302_PROC_NAME "adda302"

//debug helper
#if DEBUG_ADDA302
#define DEBUG_ADDA302_PRINT(FMT, ARGS...)    printk(FMT, ##ARGS)
#else
#define DEBUG_ADDA302_PRINT(FMT, ARGS...)
#endif

typedef struct adda302_priv {
    int                 adda302_fd;
    u32                 adda302_reg;
} adda302_priv_t;

/*
 * extern global variables
 */
extern adda302_priv_t   priv;

#endif //end of __ADDA302_H__
