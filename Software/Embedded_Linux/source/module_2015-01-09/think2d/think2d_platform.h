#ifndef _THINK2D_PLATFORM_H_
#define _THINK2D_PLATFORM_H_


#include <linux/kernel.h>
#include <linux/version.h>



#if defined(CONFIG_PLATFORM_GM8287)
//#include <mach/spec.h>
#define THINK2D_PA_START  GRA_FT2DGRA_PA_BASE
#define THINK2D_PA_END    GRA_FT2DGRA_PA_LIMIT   /* GRA_FT2DGRA_PA_LIMIT */
#define THINK2D_IRQ_START GRA_FT2DGRA_0_IRQ
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
//#include <mach/spec.h>
#define THINK2D_PA_START  GRA_FT2DGRA_PA_BASE
#define THINK2D_PA_END    GRA_FT2DGRA_PA_LIMIT   /* GRA_FT2DGRA_PA_LIMIT */
#define THINK2D_IRQ_START GRA_FT2DGRA_0_IRQ
#else
#define THINK2D_PA_START	0x92200000 //NISH20120523
#define THINK2D_PA_END		(THINK2D_PA_START+0x100-1)  
#define THIKK2D_IRQ			29		///< Think2D irq number
#endif

struct think2d_scu_t {
    int fd;
    int (*register_scu)(int* scu_fd);
    int (*deregister_scu)(int* scu_fd);
    int (*enable_scu)(int scu_fd);
    int (*select_scu)(int scu_fd, int half);
    int (*disable_scu)(int scu_fd);
    int (*reset_scu)(int scu_fd);
} ;

extern int think2d_scu_probe(struct think2d_scu_t *p_think2d_scu);
extern int think2d_scu_remove(struct think2d_scu_t *p_think2d_scu);


#endif /* _THINK2D_PLATFORM_H */

