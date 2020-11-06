





.global init_FA410_test
                 
.text	           
.align 0 

init_FA410_test:

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


test:
        MOV     r0, #0
        MOV     r1, #2
        MOV     r2, #3
        MOV     r0, #10
		.long	0XEE800318;@ 
		.long	0XEE000311;@ 
		.long	0XEE10032D;@ 
		.long	0XEE800311;@ 
		.long	0XEE80030D;@ 
		.long	0XEE8230E0;@ 
		.long	0XEE01D049;@ 
		.long	0XEE01C04D;@       
		.long	0XEE000469;@ 
        MOV     r0, #0
        MOV     r0, #0
        MOV     r0, #0
		.long	0XEE01C1D5; @1110-1110-0000-0001-1100-0001-1101-0101 	
        MOV     r0, #0
        MOV     r0, #0
        MOV     r0, #0
        MOV     r0, #0
		.long	0XEE01D155; @1110-1110-0000-0001-1101-0001-0101-0101 	
@; FLR2	r4,r5,[I0,I5],M1,M5
        MOV     r0, #0 
        MOV     r1, #7 
		.long	0XEEE42200; @1110-1110-1110-0100-0010-0010-0000-0000 	 	
		 MOV     r0, #7
		  MOV     r1, #7
		.long	0XEEE42210; @1110-1110-1110-0100-0010-0010-0001-0000 		
@;MOV     r0, #1        
@;FCHEN   R1,R0
        MOV     r0, #1  
        MOV     r1, #3  
		.long	0XEE820289; @1110-1110-1000-0010-0000-0010-1000-1001 	
init_FA410_test_exit:

        MOV		pc,r14


  .END
  
