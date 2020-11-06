#ifndef _IRDET_PLATFORM_H
#define _IRDET_PLATFORM_H
#include "irda_drv.h"

#define DEV_IRQ_START 	IR_DET_FTIRDET_IRQ
#define DEV_IRQ_END 	IR_DET_FTIRDET_IRQ
#define DEV_PA_START 	IR_DET_FTIRDET_PA_BASE
#define DEV_PA_END 		IR_DET_FTIRDET_PA_LIMIT

extern int irda_SetPinMux(int gpio_num);

extern int scu_probe(struct scu_t *p_scu);
extern int scu_remove(struct scu_t *p_scu);

#endif
