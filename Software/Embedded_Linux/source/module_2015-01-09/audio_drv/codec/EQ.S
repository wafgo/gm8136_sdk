	.global EQ_function_long
    .global EQ_function_short
    
CID:    .byte '8'
	.byte '4'
        .byte '1'
        .byte '4'
        .byte '9'
        .byte '0'
        .byte '6'
        .byte '0'
        
.align 4     
    

  .text
  .align 0
    
EQ_function_long:

        LDR		r1,=eq_type 
        STMFD		r13!,{r4,r5,r14}
 
        LDR		r1,[r1,#0]
 
        LDR		r2,=MP3_EQ_table 
        ADD		r1,r1,r1,LSL #2
 
        LDR		r5,=band_info_EQ_long 
        ADD		r4,r2,r1,LSL #2
 
        MOV		r12,#0xa
 
Branch2:
        LDRH		r14,[r4],#2
 
        LDRH		r1,[r5],#2
 
        CMP		r1,#0
 
        BEQ		Branch0 @ 0x4c
Branch1:
        LDR		r2,[r0,#0]
 
        SUBS		r1,r1,#1
 
        SMULL		r2,r3,r14,r2
 
        MOV		r2,r2,LSR #15
 
        ORR		r2,r2,r3,LSL #17
 
        STR		r2,[r0],#4
 
        BNE		Branch1 @ 0x30
Branch0:
        SUBS		r12,r12,#1
 
        BNE		Branch2 @ 0x20
        LDMFD		r13!,{r4,r5,pc}
 
EQ_function_short:
        STMFD		r13!,{r4-r10,r14}
 
        LDR		r10,=eq_type 
        LDR		r9,=MP3_EQ_table 
        LDR		r8,=band_info_EQ_short 
        MOV		r7,#0
 
Branch6:
        LDR		r1,[r10,#0]
 
        ADD		r2,r0,r7,LSL #9
 
        ADD		r1,r1,r1,LSL #2
 
        ADD		r6,r9,r1,LSL #2
 
        MOV		r5,r8
 
        MOV		r14,#0
 
Branch5:
        LDRH		r4,[r6],#2
 
        LDRH		r1,[r5],#2
 
        CMP		r1,#0
 
        BEQ		Branch3 @ 0xb0
Branch4:
        LDR		r3,[r2,#0]
 
        SUBS		r1,r1,#1
 
        SMULL		r3,r12,r4,r3
 
        MOV		r3,r3,LSR #15
 
        ORR		r3,r3,r12,LSL #17
 
        STR		r3,[r2],#4
 
        BNE		Branch4 @ 0x94
Branch3:
        ADD		r14,r14,#1
 
        CMP		r14,#0xa
 
        BLT		Branch5 @ 0x84
        ADD		r7,r7,#1
 
        CMP		r7,#8
 
        BLT		Branch6 @ 0x6c
        LDMFD		r13!,{r4-r10,pc} 

    .end

