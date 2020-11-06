
                                                                                                                      
.global  Tns_CalcLPCWeightedSpectrum       
.global  saturate_1
.global  norm_l_123
 
.text	   
.align 0 

Tns_CalcLPCWeightedSpectrum:
        STMFD    sp!,{r0-r11,lr}
        SUB      sp,sp,#0xcc
        ADD      r9,sp,#0x104
        LDMIA    r9,{r5,r6,r9}
        LDR      r4,[sp,#0x110]         
        LDR      r7,[sp,#0x100] 
        MOV      r11,r1     
        MOV      r10,#0     
        CMP      r6,r9                   
        BGE      L1.1656
L1.1568:
        LDR      r0,[sp,#0xd4]
        LDR      r0,[r0,r6,LSL #2] 
        SUB      r1,r0,#2                
        CMP      r1,#0                   
        MVNLE    r0,#0x80000000
        BLE      L1.1640
        MOV      r1,#0x20                
        BL       rsqrt                   
        MOV      r8,r0
        CMP      r0,#0x10000             
        MVNLE    r0,#0x80000000          
        BLE      L1.1640
        MOV      r0,r8                   
        BL       norm_l_123                  
        MOV      r1,r8,LSL r0            
        MOV      r2,#0x10000             
        MOV      r0,r2,LSL r0            
        BL       Div_32                  
L1.1640:
        STR      r0,[sp,r6,LSL #2]       
        ADD      r6,r6,#1                
        CMP      r6,r9                   
        BLT      L1.1568
L1.1656:
        MOV      r1,r7              
        CMP      r7,r5       
        
        BGE      L1.1736                

        SUB      r12,r5,r7                      
        ADR      r0,L11680_start          
        ADR      r1,L11680_end            
		.word	0XEEE00220;@	
		.word	0XEEE38200;@	  

        LDR      r0,[sp,#0x108]           
        MOV      r3,sp
        LDR      r2,[sp,r0,LSL #2]        
        MOV      r1,r7                    
L11680_start:  @;
        LDR      r12,[sp,#0xd8]           
        ADD      r12,r12,r0,LSL #1
        LDRSH    r12,[r12,#2]
        SUBS     r12,r12,r1                
        BNE      L11680_end_1              
        ADD      r0,r0,#1                  
        ADD      r12,r0,#1                 
        SUB      r12,r12,r9                
        CMP      r12,#0                    
        LDRLE    r2,[r3,r0,LSL #2]         
L11680_end_1:
        STR      r2,[r4,r1,LSL #2]
L11680_end:
        ADD      r1,r1,#1  
L1.1736:                                 
        SUB      r0,r5,#2                  
        CMP      r0,r7                     
        BLT      L1.1784 
        SUB      r12,r0,r7                 
		.word	0XEEE38200;@		     
        ADR      r2,L11748_start                
        ADR      r12,L11748_end            
		.word	0XEEE05280;@		
  
L11748_start:
        ADD      r2,r4,r0,LSL #2           
        LDR      r2,[r2,#4]
        LDR      r1,[r4,r0,LSL #2]
        ADD      r1,r1,r2
        MOV      r1,r1,ASR #1
        STR      r1,[r4,r0,LSL #2]
L11748_end:
        SUB      r0,r0,#1 
L1.1784:
        ADD      r0,r7,#1                  
        CMP      r0,r5                     
        BGE      L1.1832
        SUB      r12,r5,r0                
		.word	0XEEE38200;@		    
        ADR      r1,L11796_start                
        ADR      r12,L11796_end          
		.word	0XEEE03280;@	 	
L11796_start:
        ADD      r2,r4,r0,LSL #2           
        LDR      r2,[r2,#-4]
        LDR      r1,[r4,r0,LSL #2]
        ADD      r1,r1,r2
        MOV      r1,r1,ASR #1
        STR      r1,[r4,r0,LSL #2]
L11796_end:
        ADD      r0,r0,#1  
L1.1832:         
        CMP      r7,r5                     
        BGE      L1.1884

        SUB      r12,r5,r7                      
        ADR      r0,L11844_start                 
        ADR      r1,L11844_end            
		.word	0XEEE00220;@		
		.word	0XEEE38200;@	
        MOV      r1,r7                   
L11844_start:
        LDR      r2,[r4,r1,LSL #2]
        LDR      r0,[sp,#0xcc]             
        LDR      r0,[r0,r1,LSL #2]
        SMULLS   r3,r0,r2,r0              
        STR      r0,[r4,r1,LSL #2]
        RSBLT    r0,r0,#0                  
        ADD      r1,r1,#1   
L11844_end:
        ORR      r10,r10,r0         
L1.1884:
        MOV      r0,r10                    
        BL       norm_l_123
        RSBS     r0,r0,#0x10               
        BMI      L1.1948                
        CMP      r7,r5                     
        ADDGE    sp,sp,#0xdc
        LDMGEFD  sp!,{r4-r11,pc}
        SUB      r12,r5,r7                      
        ADR      r2,L11912_start                
        ADR      r1,L11912_end            
		.word	0XEEE04220;@		
		.word	0XEEE38200;@	

L11912_start:
        LDR      r1,[r4,r7,LSL #2]         
        ADD      r2,r11,r7,LSL #1
        ADD      r7,r7,#1                 
        MOV      r1,r1,ASR r0              
L11912_end:
        STRH     r1,[r2,#0]
        ADD      sp,sp,#0xdc
        LDMFD    sp!,{r4-r11,pc}
L1.1948:      
        CMP      r7,r5                    
        ADDGE    sp,sp,#0xdc
        RSB      r6,r0,#0                  
        LDMGEFD  sp!,{r4-r11,pc} 
        SUB      r12,r5,r7                        
        ADR      r0,L11964_start                  
        ADR      r1,L11964_end           
		.word	0XEEE00220;@		
		.word	0XEEE38200;@	

L11964_start:
        LDR      r0,[r4,r7,LSL #2]         
        MOV      r0,r0,LSL r6
        @BL       saturate_1
        
        ADD      r1,r11,r7,LSL #1
        ADD      r7,r7,#1  
L11964_end:
        STRH     r0,[r1,#0] 
        ADD      sp,sp,#0xdc
        LDMFD    sp!,{r4-r11,pc}           
    
    
                                             
                                                                                 
 norm_l_123:                                                                                                                                                 
                                                                         
         ORR      r0,r0,r0,LSR #1              
         ORR      r0,r0,r0,LSR #2              
         ORR      r0,r0,r0,LSR #4              
         ORR      r0,r0,r0,LSR #8              
         BIC      r0,r0,r0,LSR #16             
         RSB      r0,r0,r0,LSL #9              
         RSB      r0,r0,r0,LSL #11             
         RSB      r1,r0,r0,LSL #14             
         LDR      r0,=hash_table_123                      
         LDRB     r0,[r0,r1,LSR #26]           
         SUB      r0,r0,#1                     
         MOV      pc,r14                       
   
       
   
 hash_table_123:
       .byte      0x20,0x14,0x13,0xff
       .byte      0xff,0x12,0xff,0x07
       .byte      0x0a,0x11,0xff,0xff
       .byte      0x0e,0xff,0x06,0xff
       .byte      0xff,0x09,0xff,0x10
       .byte      0xff,0xff,0x01,0x1a
       .byte      0xff,0x0d,0xff,0xff
       .byte      0x18,0x05,0xff,0xff
       .byte      0xff,0x15,0xff,0x08
       .byte      0x0b,0xff,0x0f,0xff
       .byte      0xff,0xff,0xff,0x02
       .byte      0x1b,0x00,0x19,0xff
       .byte      0x16,0xff,0x0c,0xff
       .byte      0xff,0x03,0x1c,0xff
       .byte      0x17,0xff,0x04,0x1d
       .byte      0xff,0xff,0x1e,0x1f   
   
    
   
                                                                                                          
        
saturate_1:                                                                                                                                                      
                                                                       
       MOV      r1,r0                         
       MOV      r2,r1,ASR #15                 
       TEQ      r2,r1,ASR #31                 
       MOVNE    r0,#0x8000                    
       SUBNE    r0,r0,#1                      
       EORNE    r0,r0,r1,ASR #31              
       MOVEQ    r0,r1                         
       MOV      pc,r14 
       
       
       
       
       
.end       
                              