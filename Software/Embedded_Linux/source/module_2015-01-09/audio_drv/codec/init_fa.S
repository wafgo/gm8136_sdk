





.global init_FA410_reg

.text	 
.align 0 

init_FA410_reg:	

@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
@;;; Reset all Coprocessor registers
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

		.long	0XEE000300; 	
		.long	0XEE000301; 	
		.long	0XEE000302; 	
		.long	0XEE000303; 	
		.long	0XEE000304; 	
		.long	0XEE000305; 	
		.long	0XEE000306; 	
		.long	0XEE000307; 	
		.long	0XEE800301; 	
		.long	0XEE800302; 	
		.long	0XEE800303; 	
		.long	0XEE800305; 	
		.long	0XEE800306; 	
		.long	0XEE800307; 	
        MOV     r0, #0
		.long	0XEEF00210;  	
		.long	0XEEF00230;  	
		.long	0XEEF00250;  	
		.long	0XEEF00270;  	
		.long	0XEEE20200; 
		.long	0XEEE00200; 

@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
@;;;; Set MOD register = 0xF
@;;;; Destination : right justify [1]
@;;;; Source      : right justify [1]
@;;;; Extention   : signed        [1]
@;;;; MUL mode    : fractional    [1]
@;;;; Bit-reverse : no            [0]
@;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        MOV             r0,#0x6          ;@#0xF
		.long	0XEED00210; @1110-1110-1101-0000-0000-0010-0001-0000 	

init_FA410_exit:

		MOV		r0, #0
    MOV		pc,r14
   
    
  .END  
