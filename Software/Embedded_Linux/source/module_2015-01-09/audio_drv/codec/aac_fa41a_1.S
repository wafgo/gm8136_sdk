



.global  Tns_AutoCorre      
.global  CalcBlkEnergy
.global  SfbMdct_Energy
.global  MDCT_CalcScalefactor 

.text	 
.align 0


Tns_AutoCorre:

        STMFD    sp!,{r4-r10,lr}
        MOV      r12,#7                       
        MOV      r7,r2            
        MOV      r2,#0           
        MOV      r4,r0            
        ADR       r8,L1324        
        ADR       r9,L1344        
		.word	0XEEE11220;@		
		.word	0XEEE2E200;@	
		.word	0XEE800330;@		
L1324:
        LDRSH     r6,[r4],#2       
        MOV       r6,r6,ASR #2	   
L1344:
		.word	0XEE00D0E0;@		
		.word	0XEE800310;@	
		.word	0XEEF00208;@	
		.word	0XEEF00229;@	
				nop
				MOV			 r8,r8,LSR r12       
				ORR      r2, r8, r9
        STR      r2,[r1,#0]                                                                                                                          
        CMP      r2,#0            
        BEQ      L1488                                                                                                                                                                                                
        MOV      r5,#1          
        CMP      r3,#1
        BLE      L1488        
L1396:
        SUB       r7,r7,#1         
        MOV       r2,#0            
        MOV       r8,r0            
        ADD       r4,r0,r5,LSL #1 
        ADR       r8,L1416        
        ADR       r9,L1464        
		.word	0XEEE11220;	
		.word	0XEEE2E200;	
		.word	0XEE800330;	
L1416:
        LDRSH     r9,[r8],#2       
        LDRSH     r10,[r4],#2       
        MOV       r9,r9,ASR #2     
        MOV       r10,r10,ASR #2   
L1464:
		.word	0XEE0130E0; 	
		.word	0XEE800310;
		.word	0XEEF00208;
		.word	0XEEF00229;
				nop
				MOV			 r8,r8,LSR r12 
				ORR      r2, r8, r9 ,LSL #17 
        STR      r2,[r1,#0]              
        STR      r2,[r1,r5,LSL #2] 
        ADD      r5,r5,#1
        CMP      r5,r3
        BLT      L1396        
L1488:
        LDMFD    sp!,{r4-r10,pc}
     
    

CalcBlkEnergy:
        STMFD    sp!,{r0-r11,lr}
        LDR      r1,L1.988
        LDR      r3,[r0,#0xac]
        LDR      r6,[r0,#0xa8]
        LDMIA    r1,{r10,r11}  
        MOV      r8,#0        
L144:
        LDR      r7,[sp,#0xc]   
        MOV      r12,#0        
        MOV      lr,#0 
        ADR       r4,L156          
        ADR       r9,L_end_L156 
		.word	0XEEE09220; 	
		.word	0XEEE2E200;
		.word	0XEE800330; 
        MOV       r1,#0          
        MOV       r9,#0             
L156:
        LDR      r4,[sp,#4]
        ADD      r4,r4,r1,LSL #1 
        LDRSH    r5,[r4,#0]
        ADD      r1,r1,r2        
        MOV      r4,r5,LSL #16
        SMULL    r9,r4,r11,r4
        MOV      r9,r9,LSR #31
        ADD      r4,r9,r4,LSL #1
        SUB      r6,r4,r6         
        SMULL    r3,r9,r10,r3     
        SUB      r9,r6,r9,LSL #1  
        MOV      r6,r4            
        MOV      r4,r9,ASR #16    
        MOV      r3,r9
		.word	0XEE00B0E0;	@	
L_end_L156:
		.word	0XEE1090E0;@	
		.word	0XEE800310;@	
		.word	0XEEF00205;@	
		.word	0XEEF00224;@	
        nop
				MOV			 r5,r4,LSR #7       							 
				ORR      r5, r5, r4 ,LSL #17 
		.word	0XEEF00245;	@	
		.word	0XEEF00266;	@	
        nop
				MOV			 r5,r4,LSR #7       					 
				ORR      r5, r5, r6 ,LSL #17 	

		.word	0XEE800330;	@
        ADD      r4,r0,r8,LSL #2        
        STR      r12,[r4,#0x48]!              
        ADD      r8,r8,#1        
        CMP      r8,#8           
        STR      lr,[r4,#0x40]
        BLT      L144            
L_end_L144:
        STR      r6,[r0,#0xa8]!   
        STR      r3,[r0,#4]       
        ADD      sp,sp,#0x10
        MOV      r0,#1
        LDMFD    sp!,{r4-r11,pc}             
     









SfbMdct_Energy:
        STMFD    sp!,{r4-r11,lr}
        LDR      r11,[sp,#0x24]
        MOV      lr,#0           
        MOV      r7,#0x80000000  
        MOV     r6,#0           
        CMP     r2,#0           
        BLE     L13796_end

L13712:
        ADD      r5,r1,r6,LSL #1
        LDRSH    r4,[r5,#0]      
        LDRSH    r9,[r5,#2]   
        SUB      r9,r9,r4         
        ADR      r10,L13736            
        ADR      r12,L13764_end 
		.word	0XEEE15280;@	
		.word	0XEEE32200;@	          

        MOV      r12,#0        
L13736:
        LDR      r5,[r0,r4,LSL #2]
        ADD      r4,r4,#1           
        SMULL    r10,r8,r5,r5       
        ADDS     r12,r12,r8         
L13764_end:
        EORVS    r12,r7,r12,ASR #31
        ADDS     r12,r12,r12        
        EORVS    r12,r7,r12,ASR #31 
        ADDS     lr,lr,r12          
        STR      r12,[r3,r6,LSL #2]
        ADD      r6,r6,#1           
@;L13796_end
        EORVS    lr,r7,lr,ASR #31
        CMP      r6,r2
        BLT      L13712
L13796_end:
        STR      lr,[r11,#0]
        LDMFD    sp!,{r4-r11,pc}       
      




MDCT_CalcScalefactor:
        STMFD    sp!,{r3,lr}

		.word	0XEEE22200;@	
        ADR      r3,L119081_s            
        ADR      r12,L119081_e     
		.word	0XEEE07280;@	

        MOV      r3,#0       
L119081_s:
        LDRSH    r12,[r0,#0]       
        ADD      r0,r0,r2,LSL #1   
        CMP      r12,#0            
        RSBLT    r12,r12,#0
L119081_e:
        ORR      r3,r3,r12   
        CMP      r3,#0
        MOVEQ    r0,#0xf
        BEQ      L11956111
        MOV      r0,r3
        BL       norm_s
L11956111:
        MOV      r0,r0,LSL #16
        MOV      r0,r0,ASR #16

        LDMFD    sp!,{r3,pc}           
        

L1.988:
        .word      hiPassCoeff        

hiPassCoeff:
        .word      0xbec8b439
        .word      0x609d4952



norm_s:
        CMP      r0,#0
        MOVEQ    r1,#0
        BEQ      L2.68888
        CMN      r0,#1
        MOVEQ    r1,#0xf
        BEQ      L2.68888
        CMP      r0,#0
        MVNLT    r0,r0
        MOVLT    r0,r0,LSL #16
        MOVLT    r0,r0,ASR #16
        CMP      r0,#0x4000
        MOV      r1,#0
        BGE      L2.68888
L2.5222:
        MOV      r0,r0,LSL #1
        CMP      r0,#0x4000
        ADD      r1,r1,#1
        BLT      L2.5222
L2.68888:
        MOV      r0,r1
        MOV      pc,lr
       
 
      
.end
