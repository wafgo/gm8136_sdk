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
#ifdef  CONFIG_SYS_FLASH_CFI    //NAND or SPI boot ROM code will do it
/*
 * Setup DDR controller registers
 *
 *      r0 - DDR controller base address
 *      r1 - DDR configure table
 */
set_ddrconf:
	ldr r4, [r1], #4

0:
	ldr r2, [r1], #4
	ldr r3, [r1], #4
	str r3, [r0, r2]
	subs r4, r4, #1
	bne 0b

	ldr r3, =0x1
	str r3, [r0, #0x04]
1:
	ldr r3, [r0, #0x04]         // wait for stable
	and r3, r3, #0x100
	cmp r3, #0x100
	bne 1b

	mov pc, lr

/*
 * Setup UART controller registers
 *
 *      r0 - UART controller base address
 *      r1 - UART configure table
 */
set_uartconf:
	ldr r4, [r1], #4

2:
	ldr r2, [r1], #4
	ldr r3, [r1], #4
	str r3, [r0, r2]
	subs r4, r4, #1
	bne 2b

	mov pc, lr

mask_PSslave_error:
    LDR     r1, =0x00000000
    LDR     r2, =0x9A000080
    STR     r1, [r2]
    
    LDR     r1, =0x00000001
    LDR     r2, =0x9A200008
    STR     r1, [r2]
    LDR     r2, =0x9A300008
    STR     r1, [r2]
    LDR     r2, =0x9A400008
    STR     r1, [r2]
    LDR     r2, =0x9A500008
    STR     r1, [r2]                
    LDR     r2, =0x9B000008
    STR     r1, [r2]
            
    LDR     r1, =0x00000008
    LDR     r2, =0x9A700130
    STR     r1, [r2]
    LDR     r2, =0x9A900130
    STR     r1, [r2]
    LDR     r2, =0x9AA00130
    STR     r1, [r2]
    LDR     r2, =0x9AB00130
    STR     r1, [r2]
    LDR     r2, =0x9AC00130
    STR     r1, [r2]
    LDR     r2, =0x9AD00130
    STR     r1, [r2]
    LDR     r2, =0x9AE00130
    STR     r1, [r2]
    LDR     r2, =0x9AF00130
    STR     r1, [r2] 

	mov pc, lr
	
init_memory:
	mov  r10, lr
	
	//b    ddr_800    //socket 800MHz
	b    ddr_1333    //socket 1333MHz
	//b    ddr_1600  //socket 1600MHz

	.type ddr0_config_800, #object
ddr0_config_800:
	.word 29
	.word 0x00, 0x07E45E02
	.word 0x08, 0x00421420
	.word 0x0C, 0x00000080
	.word 0x10, 0x00004444
	.word 0x14, 0x15090A07
	.word 0x18, 0x31551212
	.word 0x1C, 0x0300202F
	.word 0x20, 0x00006F01
	.word 0x24, 0x00000000
	.word 0x28, 0x00000925
	.word 0x2C, 0x00000000
	.word 0x30, 0x00000000
	.word 0x34, 0x06060606
	.word 0x38, 0x06060606
	.word 0x3C, 0x00620012
	.word 0x40, 0x00000000
	.word 0x48, 0x000000FF
	.word 0x4C, 0x00000000
	.word 0x60, 0x00F00F0E
	.word 0x68, 0x23231010
	.word 0x6C, 0x33333333
	.word 0x70, 0x000000FF
	.word 0x74, 0x00003333
	.word 0x78, 0x00000000
	.word 0x7C, 0xFFFFFFFF
	.word 0xA0, 0x87878787
	.word 0xA4, 0x87878787
	.word 0xA8, 0x00030000
	.word 0xAC, 0x00062A80
	.size ddr0_config_800, . - ddr0_config_800

	.type ddr1_config_800, #object
ddr1_config_800:
	.word 29
	.word 0x00, 0x07E45E02
	.word 0x08, 0x00421420
	.word 0x0C, 0x00000080
	.word 0x10, 0x10004444
	.word 0x14, 0x15090A07
	.word 0x18, 0x31551212
	.word 0x1C, 0x0300202F
	.word 0x20, 0x00006F01
	.word 0x24, 0x00000000
	.word 0x28, 0x00000925
	.word 0x2C, 0x00000000
	.word 0x30, 0x00000000
	.word 0x34, 0x06060606
	.word 0x38, 0x06060606
	.word 0x3C, 0x00620011
	.word 0x40, 0x00000000
	.word 0x48, 0x000000FF
	.word 0x4C, 0x00000000
	.word 0x60, 0x00F00F0E
	.word 0x68, 0x21211010
	.word 0x6C, 0x33333333
	.word 0x70, 0x000000FF
	.word 0x74, 0x00003333
	.word 0x78, 0x00000000
	.word 0x7C, 0xFFFFFFFF
	.word 0xA0, 0x87878787
	.word 0xA4, 0x87878787
	.word 0xA8, 0x00030000
	.word 0xAC, 0x00062A80
	.size ddr1_config_800, . - ddr1_config_800
	
ddr_800:
	/* Configure DDR0 */
	ldr r0, =0x99300000
	adr r1, ddr0_config_800
	bl  set_ddrconf

	/* Configure DDR1 */
	ldr r0, =0x99700000
	adr r1, ddr1_config_800
	bl  set_ddrconf
	b   ddr_exit
		
	.type ddr0_config_1333, #object
ddr0_config_1333:
	.word 29
	.word 0x00, 0x07E45E02
	.word 0x08, 0x00421A60
	.word 0x0C, 0x00000090
	.word 0x10, 0x00004444
	.word 0x14, 0x240E100B
	.word 0x18, 0x42951424
	.word 0x1C, 0x03002050
	.word 0x20, 0x00006F01
	.word 0x24, 0x00000000
	.word 0x28, 0x00000925
	.word 0x2C, 0x00000000
	.word 0x30, 0x00000000
	.word 0x34, 0x06060606
	.word 0x38, 0x06060606
	.word 0x3C, 0x00640012
	.word 0x40, 0x00000000
	.word 0x48, 0x000000FF
	.word 0x4C, 0x00000000
	.word 0x60, 0x00F00F0E
	.word 0x68, 0x2C2C2223
	.word 0x6C, 0x33333333
	.word 0x70, 0x000000FF
	.word 0x74, 0x00004444
	.word 0x78, 0x00000000
	.word 0x7C, 0xFFFFFFFF
	.word 0xA0, 0x87878787
	.word 0xA4, 0x87878787
	.word 0xA8, 0x00030000
	.word 0xAC, 0x00062A80	

	.size ddr0_config_1333, . - ddr0_config_1333

	.type ddr1_config_1333, #object
ddr1_config_1333:
	.word 29
	.word 0x00, 0x07E45E02
	.word 0x08, 0x00421A60
	.word 0x0C, 0x00000090
	.word 0x10, 0x10004444
	.word 0x14, 0x240E100B
	.word 0x18, 0x42951424
	.word 0x1C, 0x03002050
	.word 0x20, 0x00006F01
	.word 0x24, 0x00000000
	.word 0x28, 0x00000925
	.word 0x2C, 0x00000000
	.word 0x30, 0x00000000
	.word 0x34, 0x06060606
	.word 0x38, 0x06060606
	.word 0x3C, 0x00640012
	.word 0x40, 0x00000000
	.word 0x48, 0x000000FF
	.word 0x4C, 0x00000000
	.word 0x60, 0x00F00F0E
	.word 0x68, 0x2A2A2223
	.word 0x6C, 0x33333333
	.word 0x70, 0x000000FF
	.word 0x74, 0x00004444
	.word 0x78, 0x00000000
	.word 0x7C, 0xFFFFFFFF
	.word 0xA0, 0x87878787
	.word 0xA4, 0x87878787
	.word 0xA8, 0x00030000
	.word 0xAC, 0x00062A80

	.size ddr1_config_1333, . - ddr1_config_1333
		
	.type ddr0_config_1600, #object
ddr0_config_1600:
	.word 29
	.word 0x00, 0x07E45E02
	.word 0x08, 0x00421A70
	.word 0x0C, 0x00000090
	.word 0x10, 0x00004444
	.word 0x14, 0x2B0F130D
	.word 0x18, 0x52B51525
	.word 0x1C, 0x03002060
	.word 0x20, 0x00006F01
	.word 0x24, 0x00000000
	.word 0x28, 0x00000925
	.word 0x2C, 0x00000000
	.word 0x30, 0x00000000
	.word 0x34, 0x06060606
	.word 0x38, 0x06060606
	.word 0x3C, 0x00640012
	.word 0x40, 0x00000000
	.word 0x48, 0x000000FF
	.word 0x4C, 0x00000000
	.word 0x60, 0x00F00F0E
	.word 0x68, 0x43432525
	.word 0x6C, 0x33333333
	.word 0x70, 0x000000FF
	.word 0x74, 0x00004444
	.word 0x78, 0x00000000
	.word 0x7C, 0xFFFFFFFF
	.word 0xA0, 0x87878787
	.word 0xA4, 0x87878787
	.word 0xA8, 0x00030000
	.word 0xAC, 0x00062A80
		
	.size ddr0_config_1600, . - ddr0_config_1600

	.type ddr1_config_1600, #object
ddr1_config_1600:
	.word 29
	.word 0x00, 0x07E45E02
	.word 0x08, 0x00421A70
	.word 0x0C, 0x00000090
	.word 0x10, 0x10004444
	.word 0x14, 0x2B0F130D
	.word 0x18, 0x52B51525
	.word 0x1C, 0x03002060
	.word 0x20, 0x00006F01
	.word 0x24, 0x00000000
	.word 0x28, 0x00000925
	.word 0x2C, 0x00000000
	.word 0x30, 0x00000000
	.word 0x34, 0x06060606
	.word 0x38, 0x06060606
	.word 0x3C, 0x00640012
	.word 0x40, 0x00000000
	.word 0x48, 0x000000FF
	.word 0x4C, 0x00000000
	.word 0x60, 0x00F00F0E
	.word 0x68, 0x41412425
	.word 0x6C, 0x33333333
	.word 0x70, 0x000000FF
	.word 0x74, 0x00004444
	.word 0x78, 0x00000000
	.word 0x7C, 0xFFFFFFFF
	.word 0xA0, 0x87878787
	.word 0xA4, 0x87878787
	.word 0xA8, 0x00030000
	.word 0xAC, 0x00062A80

	.size ddr1_config_1600, . - ddr1_config_1600

ddr_1333:
	/* Configure DDR0 */
	ldr r0, =0x99300000
	adr r1, ddr0_config_1333
	bl  set_ddrconf

	/* Configure DDR1 */
	ldr r0, =0x99700000
	adr r1, ddr1_config_1333
	bl  set_ddrconf
	b   ddr_exit
	
ddr_1600:
	/* Configure DDR0 */
	ldr r0, =0x99300000
	adr r1, ddr0_config_1600
	bl  set_ddrconf

	/* Configure DDR1 */
	ldr r0, =0x99700000
	adr r1, ddr1_config_1600
	bl  set_ddrconf
	b   ddr_exit

ddr_exit:
	mov lr, r10
	mov pc, lr

	.type uart_config, #object
uart_config:

	.word 5
	.word 0x08, 0x35
	.word 0x0C, 0x83
#ifdef CONFIG_CMD_FPGA	
	.word 0x00, 0x14
#else
	.word 0x00, 0x0D	
#endif
	.word 0x0C, 0x03
	.word 0x00, 0x30	/* write '0' */
	
	.size uart_config, . - uart_config
#endif // end of CONFIG_SYS_FLASH_CFI
#endif // end of CONFIG_SKIP_LOWLEVEL_INIT

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

#ifndef CONFIG_SKIP_LOWLEVEL_INIT
#ifdef CONFIG_SYS_FLASH_CFI //UART nsboot code will do it

	/* Flash parameter */
	ldr r0, =0x92100000
	ldr r1, =0x10000040		/* 8 bit, BNK size 16 MB, disable write protect */
	str r1, [r0]

	ldr r0, =0x92100004
#ifdef CONFIG_CMD_FPGA
	ldr r1, =0x00151050
#else	
	ldr r1, =0x010FF0F1
#endif	
	str r1, [r0]

	/* set PCIE clock */
	ldr     r1, =0x990000A0
    ldr     r2, =0xF80FFFFF
    str     r2, [r1]

	ldr     r1, =0x99000088
    ldr     r2, =0x00080000
    str     r2, [r1]
        
	ldr     r1, =0x990000A0
    ldr     r2, =0xFBFFFFFF
    str     r2, [r1]
    
	/* set UART clock */
	ldr     r0, =CONFIG_PMU_BASE
	ldr     r1, =0x18004400			/* set UART/MAC clock source */
	str     r1, [r0, #0x28]
	
	ldr     r1, [r0, #0x70]			/* set UART PVALUE */
	ldr     r2, =0xFFFF03FF
	and     r1, r1, r2
	
    ldr     r2, =0x00001000         /* baudrate 115200 */
    	
	orr     r1, r1, r2
	str     r1, [r0, #0x70]

	/* enable UART for 8210_M early debug trace */
	ldr 	r0, =0x98300000
	adr 	r1, uart_config
	bl  	set_uartconf

	/* enable UART for 8210_S early debug trace */
	ldr 	r0, =0x98400000
	adr 	r1, uart_config
	bl  	set_uartconf

	/* enable UART for audio early debug trace */
	ldr 	r0, =0x82600000
	adr 	r1, uart_config
	bl  	set_uartconf
	
	/* AXIC PSlverr disable message */
	ldr     r0, =0x9A600130
	ldr     r1, =0x00000008
	str     r1, [r0]
	
	bl		init_memory
    	
	/* Set AHBC1 DDR map */
	ldr	r0, =0x92B00000
	ldr	r1, =0x00080000
	ldr	r2, =0x10080000
	str	r1, [r0, #0x00]
	str	r2, [r0, #0x04]
	
	/* relocate uboot to RAM for remap */
	ldr	r0, =0x00000000		/* r0 <- current position of code */
	ldr	r2, __stext
	//ldr	r3, __sbss
	//sub	r2, r3, r2		/* r2 <- size of armboot */
	ldr	r1, =0x20000000		/* r1 <- destination address */
	add	r2, r0, r2		/* r2 <- source end address */

	/*
	 * r0 = source address
	 * r1 = target address
	 * r2 = source end address
	 */
copy_remap:
	ldmia r0!, {r3-r10}
	stmia r1!, {r3-r10}
	cmp	r0, r2
	ble	copy_remap
		
	bl  mask_PSslave_error
	
	/* Remap slave3 & slave4 on AXIC bus */
	ldr	r0, =0x9A600000

    ldr	r1, =0x00080000
    str	r1, [r0,#0x24]     
    ldr	r1, =0x20080000
    str	r1, [r0,#0x28]    
    ldr	r1, =0x40080000
    str	r1, [r0,#0x2C]	
    
    
	ldr r2, =0x5000
	mov r3, #0		
	
	ldr	r1, =0x9
	str	r1, [r0, #0x130]

base_wait2:
	add r3, r3, #1
	cmp r3, r2
	bne base_wait2

	/* Fix DDR0/DDR1 base on AXIC bus after remap */
	ldr	r0, =0x9A600000
	ldr	r1, =0x20080000
	str	r1, [r0,#0x24]    
	ldr	r1, =0x00080000
	str	r1, [r0,#0x28]    
	ldr	r1, =0x10080000
	str	r1, [r0,#0x2C]

	ldr	r1, =0x00080000
	ldr	r2, =0x9A900024
	str	r1, [r2]
	ldr	r2, =0x9AA00024
	str	r1, [r2]
	ldr	r2, =0x9AB00024
	str	r1, [r2]
	ldr	r2, =0x9AC00024
	str	r1, [r2]
	ldr	r2, =0x9AE00024
	str	r1, [r2]
	
    ldr	r1, =0x10080000
	ldr	r2, =0x9A900028
	str	r1, [r2]
	ldr	r2, =0x9AA00028
	str	r1, [r2]
	ldr	r2, =0x9AB00028
	str	r1, [r2]
	ldr	r2, =0x9AC00028
	str	r1, [r2]
	ldr	r2, =0x9AE00028
	str	r1, [r2]    
		
	/* Fix DDR0/DDR1 base on AHBC0 bus after remap */
	ldr	r0, =0x92A00000
	ldr	r1, =0x00080000
	ldr	r2, =0x20080000
	ldr	r3, =0x10080000
	str	r1, [r0, #0x18]
	str	r2, [r0, #0x10]
	str	r3, [r0, #0x14]

	/* Fix DDR0/DDR1 base on AHBC1 bus after remap */
	ldr	r0, =0x92B00000
	ldr	r1, =0x00080000
	ldr	r2, =0x10080000	
	str	r1, [r0, #0x00]
	str	r2, [r0, #0x04]
		
#endif // end of CONFIG_SYS_FLASH_CFI
	
	/* mask all IRQs by clearing all bits in the INTMRs */
	mov r1, #0x00
	ldr r0, =(CONFIG_INTC_BASE+0x04)
	str r1, [r0]
	ldr r0, =(CONFIG_INTC_BASE+0x24)
	str r1, [r0]
	ldr r0, =(CONFIG_INTC_BASE+0x64)
	str r1, [r0]	

	mov lr, r13
	mov	pc, lr

__stext:
	.word NetRxPackets
//__sbss:
//	.word __bss_end__

#endif
