/*
 * (C) Copyright 2009 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <version.h>

#include <asm/macro.h>
#include <faraday/ftsdmc020.h>

.global reset_cpu
reset_cpu:
	ldr r0, WDT_BASE
	mov r1, #0
	str r1, [r0]

	mov r1, #1
	str r1, [r0,#4]

	ldr r1, WDT_RESET
	str r1, [r0,#8]

	mov r1, #3
	str r1, [r0,#0xc]

reset_failed:
	b reset_failed

WDT_BASE:
	.word CONFIG_WDT_BASE
WDT_RESET:
	.word 0x5ab9
	
#ifndef CONFIG_SKIP_LOWLEVEL_INIT

/* Set up the platform, once the cpu has been initialized */
.globl lowlevel_init
lowlevel_init:
	/* If U-Boot has been run after the ARM boot monitor
	 * then all the necessary actions have been done
	 * otherwise we are running from user flash mapped to 0x00000000
	 * --- DO NOT REMAP BEFORE THE CODE HAS BEEN RELOCATED --
	 * Changes to the (possibly soft) reset defaults of the processor
	 * itself should be performed in cpu/arm<>/start.S
	 * This function affects only the core module or board settings
	 */
	mov r13, lr

	/* set clock */
	ldr	r0, =CONFIG_PMU_BASE
	ldr	r1, [r0, #0x38]		/* AHB clock */
	ldr	r2, =0xFFFFF7B3		/* MEMC/DDR2/JPEG/DMA */
	and	r1, r1, r2
	str	r1, [r0, #0x38]

	ldr	r1, [r0, #0x3C]		/* APB clock */
	ldr	r2, =0xFFFFADDE		/* UART0/SSP0/GPIO0/INTC/TIMER */
	and	r1, r1, r2
	str	r1, [r0, #0x3C]

	/* Flash parameter */
	ldr r0, =CONFIG_FTSMC020_BASE
	ldr r1, =0x10000040		/* 8 bit, BNK size 16 MB, disable write protect */
	str r1, [r0]

	ldr r0, =0x92100004
	ldr r1, =0x010FF0F1
	str r1, [r0]

	/* CONFIG_PLL2_540MHZ */
	ldr     r1, [r0, #0x34]
	ldr     r2, =0x31B6
	orr     r1, r1, #0x01
	str			r1, [r0, #0x34]	

	/* delay to wait clock stable */
	ldr     r2, =0x5265C0			/* 10ms in CPU 540MHz */
	mov     r1, #0
pll2_wait:
	add     r1, r1, #1
	cmp     r1, r2
	bne     pll2_wait

#ifdef CONFIG_SERIAL //if no nsboot to init UART, run it
	/* enable UART for early debug trace */
	ldr	r1, =0x98300008
	ldr	r2, =0x35
	str	r2, [r1]

	ldr	r1, =0x9830000C
	ldr	r2, =0x083
	str	r2, [r1]

	ldr	r1, =CONFIG_UART_BASE
	ldr	r2, =0x3;
	str	r2, [r1]

	ldr	r1, =0x9830000C
	ldr	r2, =0x003
	str	r2, [r1]

	ldr	r1, =CONFIG_UART_BASE
	ldr	r2, =0x30		/* write '0' */
	str	r2, [r1]
#endif

	/* mask all IRQs by clearing all bits in the INTMRs */
	mov r1, #0x00
	ldr r0, =(CONFIG_INTC_BASE+0x04)
	str r1, [r0]
	ldr r0, =(CONFIG_INTC_BASE+0x24)
	str r1, [r0]
#endif /* end of #ifdef CONFIG_PLATFORM_GM8126 */

	mov lr, r13
	mov	pc, lr
