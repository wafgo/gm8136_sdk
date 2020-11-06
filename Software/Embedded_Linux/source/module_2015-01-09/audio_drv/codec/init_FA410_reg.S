





.global   init_FA410_1

.text	
.align 0 

init_FA410_1:

@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
@;;;; Set MOD register = 0xF
@;;;; Destination : right justify [1]
@;;;; Source      : right justify [1]
@;;;; Extention   : signed        [1]
@;;;; MUL mode    : fractional    [1]
@;;;; Bit-reverse : no            [0]
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        MOV             r0,#0xF
		.long	0XEED00210;@ 1110-1110-1101-0000-0000-0010-0001-0000 

@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
@;;; Reset all Coprocessor registers
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
@
		.long	0XEE000300;@ 
		.long	0XEE000301;@ 
		.long	0XEE000302;@ 
		.long	0XEE000303;@ 
		.long	0XEE000304;@ 
		.long	0XEE000305;@ 
		.long	0XEE000306;@ 
		.long	0XEE000307;@ 
		.long	0XEE800301;@ 
		.long	0XEE800302;@ 
		.long	0XEE800303;@ 
		.long	0XEE800305;@ 
		.long	0XEE800306;@ 
		.long	0XEE800307;@ 
        MOV     r0, #0
		.long	0XEEF00210;@ 
		.long	0XEEF00230;@ 
		.long	0XEEF00250;@ 
		.long	0XEEF00270;@ 
		.long	0XEEE20200;@ 
		.long	0XEEE00200;@ 

test:
        @;MOV     r0, #0
        @;MOV     r1, #2
        @;MOV     r2, #3
        @;MOV     r0, #10
		@;.long	0XEE800330;@ 
		@;.long	0XEE000309;@ 
		@;.long	0XEE00037D;@ 
		@;.long	0XEE800319;@ 
		@;.long	0XEE800315;@ 
		@;.long	0XEE8230E0;@ 
		@;.long	0XEE01D049;@ 
		@;.long	0XEE000469;@ 
		@;.long	0XEE01C1D5;@ 
		@;.long	0XEE01D155;@ 
@; FLR2	r4,r5,[I0,I5],M1,M5
init_FA410_1_exit:

        MOV		pc,r14
        

  .END 
  
