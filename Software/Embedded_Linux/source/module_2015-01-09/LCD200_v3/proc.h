#ifndef _FFB_PROC_H_
#define _FFB_PROC_H_

#include <linux/proc_fs.h>
#include "lcd_def.h"

extern struct proc_dir_entry *flcd_common_proc;

extern struct proc_dir_entry *ffb_create_proc_entry(const char *name,
                                                    mode_t mode, struct proc_dir_entry *parent);
extern void ffb_remove_proc_entry(struct proc_dir_entry *parent, struct proc_dir_entry *de);
extern int ffb_proc_init(const char *name);
extern void ffb_proc_release(char *name);
extern int ffb_proc_get_usecnt(void);

typedef struct {    
    struct {
        int xres;
        int yres;
        int buff_len;   //debug only
        int format; //VIM_RGB565, ...
        unsigned int phys_addr;
        int bits_per_pixel;
        int active;
    } gui[4];   //four pip
    int PIP_En; //same as pip_dev_info
} gui_info_t;

/* for lcd vg use */
typedef struct proc_lcdinfo_s {
    LCD_IDX_T lcd_idx;
    unsigned int engine_minor;
    unsigned int vg_type;   //DRIVER_TYPE
    int hw_frame_rate;
    int input_res;
    char input_res_desc[50];
    int in_x;
    int in_y;
    int output_type;
    char output_type_desc[50];
    int out_x;
    int out_y;
    void *fb_vaddr; /* virtual address */
    dma_addr_t fb_paddr;    /* physaddr */
    /* 
     * for the same gui content use 
     */
    gui_info_t  gui_info;
    int  lcd_disable;
} proc_lcdinfo_t;

/**
 *@brief collect every LCD's summary, such as resolution, frame rate ...
 */
int proc_register_lcdinfo(LCD_IDX_T idx, int (*callback)(proc_lcdinfo_t *summary));

int ffb_proc_get_lcdinfo(LCD_IDX_T lcd_idx, proc_lcdinfo_t *lcdinfo);

#endif /* _FFB_PROC_H_ */
