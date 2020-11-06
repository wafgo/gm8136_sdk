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
