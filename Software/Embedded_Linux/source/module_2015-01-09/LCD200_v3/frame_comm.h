#ifndef __FRAMEADDR_COMM_H__
#define __FRAMEADDR_COMM_H__

#include "frame_comm.h"

/* get current read_idx and write_idx
 */
void lcd_frame_get_rwidx(unsigned int *read_idx, unsigned int *write_idx);

/* Simply to add a physical frame address
 * Return: 0 for success, -1 for fail
 */
int lcd_frameaddr_add(unsigned int frameaddr);

/* Simply to get a physical frame address
 * Return: 0 for success, -1 for fail
 */
unsigned int lcd_frameaddr_get(void);

/*
 * Simply to clear all physical frame address
 */
void lcd_frameaddr_clearall(void);

/* init function */
void lcd_frameaddr_init(void);

#endif /* __FRAMEADDR_COMM_H__ */