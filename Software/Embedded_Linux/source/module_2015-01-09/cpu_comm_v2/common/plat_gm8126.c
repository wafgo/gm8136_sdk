/*-----------------------------------------------------------------
 * Includes
 *-----------------------------------------------------------------
 */
#include <linux/version.h>      /* kernel versioning macros      */
#include <linux/module.h>       /* kernel modules                */
#include <linux/pci.h>          /* PCI + PCI DMA API             */
#include <linux/device.h>       /* sysfs/class interface         */
#include <linux/delay.h>
#include "zebra_sdram.h"        /* Host-to-DSP SDRAM layout      */
#include "zebra.h"

#include <linux/timer.h>
static struct isr_timer_s {
	struct timer_list   isr_timer;	
	void				*brd;
} gisr_timer[5];

static int init_done = 0, timer_idx = 0;

static struct isr_timer_s *get_isr_timer(void *brd)
{
	int	i;

	for (i = 0; i < timer_idx; i ++) {
		if ((u32)gisr_timer[i].brd == (u32)brd)
			return &gisr_timer[i];
	}

	return NULL;
}

void isr_timer_handler(unsigned long data)
{
    zebra_board_t   *brd = (zebra_board_t *)data;
	struct isr_timer_s	*isr_timer = get_isr_timer((void *)brd);   
	
    zebra_process_doorbell(brd);    
        
    mod_timer(&isr_timer->isr_timer, jiffies + (2*HZ)/1000);
}

int platform_doorbell_init(zebra_board_t *brd)
{
    struct isr_timer_s	*isr_timer;

	if (!init_done) {
		init_done = 1;
		memset(gisr_timer, 0, sizeof(gisr_timer));
	}

	isr_timer = &gisr_timer[timer_idx ++];
	isr_timer->brd = brd;
	
    init_timer(&isr_timer->isr_timer);
    
	isr_timer->isr_timer.function = isr_timer_handler;
	isr_timer->isr_timer.data = (unsigned long)brd;
	mod_timer(&isr_timer->isr_timer, jiffies + (10*HZ)/1000);
    
    return 0;
}

void platform_trigger_interrupt(unsigned int doorbell)
{

}

void platform_doorbell_exit(zebra_board_t *brd)
{
    struct isr_timer_s	*isr_timer = get_isr_timer((void *)brd);   

    if (timer_pending(&isr_timer->isr_timer))
        del_timer(&isr_timer->isr_timer);
}
