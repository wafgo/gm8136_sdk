 
 
 
 
  
.global  init_FA410

.text	
.align 0  	

init_FA410:


        MOV             r0,#0x6		;#0xf
		.word	0XEED00210;@	FMMR MOD,R0 	



		.word	0XEE000300;@	FMIC I0,#0 	
		.word	0XEE000301;@	FMIC I1,#0 	
		.word	0XEE000302;@	FMIC I2,#0 	
		.word	0XEE000303;@	FMIC I3,#0 	
		.word	0XEE000304;@	FMIC I4,#0 	
		.word	0XEE000305;@	FMIC I5,#0 	
		.word	0XEE000306;@	FMIC I6,#0 	
		.word	0XEE000307;@	FMIC I7,#0 	
		.word	0XEE800301;@	FMMC M1,#0 	
		.word	0XEE800302;@	FMMC M2,#0 	
		.word	0XEE800303;@	FMMC M3,#0 	
		.word	0XEE800305;@	FMMC M5,#0 	
		.word	0XEE800306;@	FMMC M6,#0 	
		.word	0XEE800307;@	FMMC M7,#0 	
        MOV     r0, #0
		.word	0XEEF00210;@	FMAR ACC0.l,R0 	
		.word	0XEEF00230;@	FMAR ACC0.h,R0 	
		.word	0XEEF00250;@	FMAR ACC1.l,R0 	
		.word	0XEEF00270;@	FMAR ACC1.h,R0 	
		.word	0XEEE20200;@	FLCNT R0 	
		.word	0XEEE00200;@	FLOOP R0,R0

init_FA410_exit:
        
        MOV		pc,r14

  
  .end 
