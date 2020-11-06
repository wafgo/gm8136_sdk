#ifndef _PIP_H_
#define _PIP_H_

#include "lcd_def.h"

typedef enum {
    RET_GENERIC_ERR = 0,
    RET_NOT_VGPLANE,
    RET_NO_NEW_FRAME,
    RET_SUCCESS
} RET_VAL_T;

struct pip_vg_cb {
    LCD_IDX_T lcd_idx;
     RET_VAL_T(*process_frame) (struct lcd200_dev_info * dev_info, int plane_idx);
    void (*flush_all_jobs) (void);
    void (*driver_clearnup) (void);     //cleanup function 
    void (*res_change)(void);           //resolution change
};

/* called by vg layer to hook its callback 
 */
extern void pip_register_callback(struct pip_vg_cb *callback);
extern void pip_deregister_callback(struct pip_vg_cb *callback);
extern unsigned int pip_get_passive_phyaddr(int idx);

#endif /* _PIP_H_ */
