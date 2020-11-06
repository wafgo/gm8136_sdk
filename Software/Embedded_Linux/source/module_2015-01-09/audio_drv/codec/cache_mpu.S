  .global cache_mpu
 
 .text	  
.align 0 

cache_mpu:     
     
        
        
        MRC		p15, 0, r0, c1, c0, 0
        ORR		r0, r0, #0x1000
        ORR		r0, r0, #0xC
        ORR		r0, r0, #0x1
        MCR		p15, 0, r0, c1, c0, 0	@;turn on U/D cache and MPU        
        
        MOV		pc,r14    
		   
    .end           
    