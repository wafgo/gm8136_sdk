#ifndef _IRDET_PLATFORM_H
#define _IRDET_PLATFORM_H
#include "ks_drv.h"
//#include <linux/kernel.h>
//#include <linux/version.h>

#if defined(CONFIG_PLATFORM_GM8210)
#define DEV_IRQ_START 	KPD_FTKPD_IRQ
#define DEV_IRQ_END 	KPD_FTKPD_IRQ
#define DEV_PA_START 	KPD_FTKPD_PA_BASE
#define DEV_PA_END 		KPD_FTKPD_PA_LIMIT

#elif defined(CONFIG_PLATFORM_GM8287)

#define DEV_IRQ_START   KPD_FTKPD_IRQ
#define DEV_IRQ_END     KPD_FTKPD_IRQ
#define DEV_PA_START    KPD_FTKPD_PA_BASE
#define DEV_PA_END      KPD_FTKPD_PA_LIMIT

#endif


//extern int clock_on(void);
//extern int clock_on(void);

extern int scu_probe(struct scu_t *p_scu);
extern int scu_remove(struct scu_t *p_scu);
extern int set_pinmux(void);

#endif
