/*
 * head.S - Startup Code for GM8136
 *
 */
#include <board.h>

#define STACK_SIZE		0x2000

	.globl _start
	.section ".text"

_start:
	b	reset				// 0x00
	ldr	pc, _undefined_instruction	// 0x04
	ldr	pc, _software_interrupt		// 0x08
	ldr	pc, _prefetch_abort		// 0x0C
	ldr	pc, _data_abort			// 0x10
	nop					// unused
	ldr	pc, _irq			// 0x18
	ldr	pc, _fiq			// 0x1C

_undefined_instruction:
	.word undefined_instruction
_software_interrupt:
	.word software_interrupt
_prefetch_abort:
	.word prefetch_abort
_data_abort:
	.word data_abort
_irq:
	.word irq
_fiq:
	.word fiq

	.balignl 16, 0xdeadbeef

irq_return:

	MRS	r4, cpsr
	ORR	r5, r4, #0x13	//Switch back to SVC mode
	MSR	cpsr_c, r5
	
    ldr	r0, =REG_PMU_BASE   //clear FCS int
    ldr	r1, =0x00020000
    str	r1, [r0,#0x20] 
  	
	add	lr, lr, #0x4
	mov	pc, lr

/*
 * The actual reset code
 */
reset:
	//bl  init_uart
	bl  init_clock

    //ldr r2, =0x030
    //bl  print_debug
    //bl  mask_PSslave_error

#ifndef USE_SCP
	/* Init AHBC0 ~ AHBC6 bus */
	bl	init_bus
#endif	

	/* Enable ICache & invalid I/D cache */
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #0x1000
	mcr	p15,0,r0,c7,c5,0
	mcr	p15,0,r0,c1,c0,0

	/* Check if code is already in place or not. */
	ldr	r0, =SRAM0_BASE
	ldr	r1, =_stext
	cmp	r0, r1
	beq	0f

	/* Copy code from SRAM0/SRAM2 to SDRAM (_stext) */
	ldr	r2, =SRAM0_SIZE
	bl	copy
0:
	/* Init stack */
	//ldr	sp, =.stack
	ldr     sp, =(SRAM1_BASE + SRAM1_SIZE)

	/* Init DATA/BSS area */
    bl      init_databss

	/* Enetr C entry */
	ldr	r0, =entry
	mov	lr, pc
	mov	pc, r0

loop:
	b       loop

/*
 * copy data from source address to destination address
 *
 *	r0 - source address
 *	r1 - destination address
 *	r2 - length (bytes)
 */
copy:
	add	r2, r1, r2
loop1:
	cmp	r1, r2
	ldrcc	r3, [r0], #4
	strcc	r3, [r1], #4
	bcc	loop1

	mov	pc, lr

init_databss:
	ldr	r0, =_etext
	ldr	r1, =_sdata
	ldr	r2, =_sbss
	ldr	r3, =_ebss

	/* Check if data is already in place or not. */
	cmp	r0, r1
	beq	1f

	/* Copy data for initialized variables. */
0:	ldmia	r0!, {r4-r11}
	stmia	r1!, {r4-r11}
	cmp	r1, r2
	blt	0b

	/* Zero unitialized variables. */
1:	mov	r4, #0
	mov	r5, #0
	mov	r6, #0
	mov	r7, #0
	mov	r8, #0
	mov	r9, #0
	mov	r10, #0
	mov	r11, #0
2:	stmia	r2!, {r4-r11}
	cmp	r2, r3
	blt	2b

	mov	pc, lr

init_bus:
	
	ldr	r0, =REG_AXIC_BASE
#if 0   /* 8136 default 512MB size */
#if defined(GM8136) || defined(GM8136S)		
  ldr	r1, =0x00080000
#else
	ldr	r1, =0x00090000
#endif    
  str	r1, [r0,#0x24] 
#if defined(GM8136) || defined(GM8136S)	        
  ldr	r1, =0x20080000
#else
	ldr	r1, =0x40090000
#endif    
  str	r1, [r0,#0x28]
#endif
	/* Remap slave3 & slave4 on AXIC bus */    
	ldr	r1, =0x9
	str	r1, [r0, #AXIC_REMAP]
	nop
	nop

	ldr	r1, =0x00090000         /* 512MB size */
	ldr	r2, =REG_AHBC_1_BASE
	str	r1, [r2]
		
	mov	pc, lr

#if 0
init_uart://not call this API
	ldr r0, =REG_PMU_BASE
	ldr r1, =0x18004408			/* set UART/MAC clock source */
	str r1, [r0, #0x28]
	
	ldr r1, [r0, #0x70]			/* set UART PVALUE */
	ldr r2, =0xFFC0FFFF
	and r1, r1, r2
	
  ldr r2, =0x00070000         /* baudrate 115200 */
    	
	orr r1, r1, r2
	str r1, [r0, #0x70]
		
	mov pc, lr
#endif	

init_clock:
	ldr r0, =REG_PMU_BASE
	ldr r1, =0xFFFFFE00
	str r1, [r0, #0xB0]	
	ldr r1, =0xFFFBE6F0
	str r1, [r0, #0xB4]
	ldr r1, =0xFFEFF7FE
	str r1, [r0, #0xB8]
	
	ldr r1, =0x40
	str r1, [r0, #0x88]
	str r1, [r0, #0x8C]
	str r1, [r0, #0x90]
	str r1, [r0, #0x94]
	str r1, [r0, #0x98]
	str r1, [r0, #0x9C]

	mov pc, lr

//print_debug:
//    LDR     r1, =0x98300000
//    STR     r2, [r1]
//	mov pc, lr

mask_PSslave_error: //X2H_reg
    LDR r1, =0x00000001
    LDR r2, =0x9A400008
    STR r1, [r2]            
    LDR r2, =0x9AF00008
    STR r1, [r2]
        
    LDR r1, =0x00000008
    LDR r2, =0x9A600130//IC_0
    STR r1, [r2]
    LDR r2, =0x9A800130//IC_1
    STR r1, [r2]
    LDR r2, =0x9AD00130//IC_2
    STR r1, [r2]

	mov pc, lr
/////////////////////////////////////////////////////////////////
/*
 * exception handlers
 */

undefined_instruction:
	b	undefined_instruction

software_interrupt:
	b	software_interrupt

prefetch_abort:
	b	prefetch_abort

data_abort:
	b	data_abort

irq:
	b	irq

fiq:
	b	fiq

.comm	.stack_base, STACK_SIZE, 4
.comm	.stack, 0, 4
