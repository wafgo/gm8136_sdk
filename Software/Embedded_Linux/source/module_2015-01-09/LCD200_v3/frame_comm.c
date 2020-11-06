#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>

#define MAX_FRAME_CNT       1   //not suitable for 1
#define FRAME_IDX_MASK      (MAX_FRAME_CNT - 1)
#define FRAME_COUNT(head, tail)	((head) >= (tail) ? (head) - (tail) : ((unsigned int)~0 - (tail) + (head) + 1))
#define FRAME_SPACE(head, tail)	(MAX_FRAME_CNT - FRAME_COUNT(head, tail))

static struct frame_info_s {
    unsigned int read_idx;
    unsigned int write_idx;
    spinlock_t   lock;
    unsigned int phys_addr[MAX_FRAME_CNT];
} frame_info;

void lcd_frame_get_rwidx(unsigned int *read_idx, unsigned int *write_idx)
{
    unsigned long flags;

    spin_lock_irqsave(&frame_info.lock, flags);
    *read_idx = frame_info.read_idx;
    *write_idx = frame_info.write_idx;
    spin_unlock_irqrestore(&frame_info.lock, flags);
}
EXPORT_SYMBOL(lcd_frame_get_rwidx);

/* Simply to add a D1 physical frame address
 * Return: 0 for success, -1 for fail
 */
int lcd_frameaddr_add(unsigned int frameaddr)
{
    unsigned long flags;
    int write_idx, ret = -1;

    spin_lock_irqsave(&frame_info.lock, flags);
    
    if (MAX_FRAME_CNT > 1) {
        if (FRAME_SPACE(frame_info.write_idx, frame_info.read_idx) != 0) {
            write_idx = frame_info.write_idx & FRAME_IDX_MASK;
            frame_info.phys_addr[write_idx] = frameaddr;
            frame_info.write_idx ++;
            ret = 0;
        }
    } else {
        frame_info.phys_addr[0] = frameaddr;
    }
    
    spin_unlock_irqrestore(&frame_info.lock, flags);

    return ret;
}
EXPORT_SYMBOL(lcd_frameaddr_add);

/* Simply to get a physical frame address
 * Return: 0 for success, -1 for fail
 */
unsigned int lcd_frameaddr_get(void)
{
    unsigned long flags;
    int read_idx, ret_addr = 0;

    spin_lock_irqsave(&frame_info.lock, flags);
    if (MAX_FRAME_CNT > 1) {
        if (FRAME_COUNT(frame_info.write_idx, frame_info.read_idx) != 0) {
            read_idx = frame_info.read_idx & FRAME_IDX_MASK;
            ret_addr = frame_info.phys_addr[read_idx];
            frame_info.read_idx ++;
        }
    } else {
        ret_addr = frame_info.phys_addr[0];
    }
    spin_unlock_irqrestore(&frame_info.lock, flags);

    return ret_addr;
}
EXPORT_SYMBOL(lcd_frameaddr_get);

/*
 * Simply to clear all physical frame address
 */
void lcd_frameaddr_clearall(void)
{
    unsigned long flags;

    spin_lock_irqsave(&frame_info.lock, flags);
    /* reset the index */
    frame_info.write_idx = frame_info.read_idx = 0;
    spin_unlock_irqrestore(&frame_info.lock, flags);
}
EXPORT_SYMBOL(lcd_frameaddr_clearall);

void lcd_frameaddr_init(void)
{
    memset(&frame_info, 0, sizeof(struct frame_info_s));

    spin_lock_init(&frame_info.lock);
}

