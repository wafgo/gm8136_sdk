
.global  div_s 

.text	 
.align 0

div_s:
        MOV      r2,r0          
        MOV      r0,#0x8000        
        SUB      r0,r0,#1          
        CMP      r2,r1            
        MOVEQ    pc,lr             
        MOV      r3,r1            

        ADR       r0,L23_divs      
        ADR       r1,L23_divs_end  
		.word	0XEEE00220;@	
        MOV       r0 ,#0xf  
		.word	0XEEE20200;@	
        MOV      r0,#0          

L23_divs:
        MOV      r2,r2,LSL #1     
        CMP      r2,r3             
        MOV      r0,r0,LSL #1      
        ADDGE    r0,r0,#1          
L23_divs_end:
        SUBGE    r2,r2,r3
        MOV      pc,lr

.end