
	.global apply_scalefactors

/* Security check: asm protection by the revrese of Faraday's company name */
 
CID1:    
	.byte 'Y'
        .byte 'A'
        .byte 'D'
        .byte 'A'
        .byte 'R'
        .byte 'A'
        .byte 'F'
        
.align 4	

  .text
  .align 0

apply_scalefactors:
        STMFD		r13!,{r0-r11,r14} 
        SUB		r13,r13,#0x20 
        LDR		r1,[r13,#0x24] 
        MOV		r0,#0 
        MOV		r12,r3,LSR #3 
        STR		r12,[r13,#0xc] 
        STR		r0,[r13,#0x10] 
        LDR		r0,[r1,#0x78] 
        ADD		r11,r1,#0x7c 
        ADD		r1,r1,#5 
        STMIA		r13,{r1,r11} 
        ADD		r10,r0,#0xb00 
        ADD		r10,r10,#0x42 
        LDR		r1,[r13,#0x24] 
        STR		r10,[r13,#8] 
        LDRB		r0,[r1,#2] 
        STR		r0,[r13,#0x14] 
        CMP		r0,#0 
        ADDEQ		r13,r13,#0x30 
        LDMEQFD		r13!,{r4-r11,pc} 
        LDR		r1,[r13,#0x24] 
        LDRB		r5,[r1,#0] 
        STR		r5,[r13,#0x1c] 
Branch_APPLY_6:
        LDR		r5,[r13,#0x1c] 
        MOV		r3,#0 
        LDR		r10,[r13,#8] 
        LDR		r11,[r13,#4] 
        CMP		r5,#0 
        BEQ		Branch_APPLY_0 @ 0x404
        LDR		r1,[r13,#0x24] 
        LDRB		r0,[r1,#4] 
        STR		r0,[r13,#0x18] 
Branch_APPLY_5:
        LDR		r0,[r13,#0x18] 
        LDR		r2,=pow2_table 
        CMP		r0,#2 
        LDRSH		r0,[r11],#2 
        MVNEQ		r1,#0x1c 
        MVNNE		r1,#0x1f 
        ADD		r1,r1,r0,ASR #2 
        AND		r0,r0,#3 
        LDR		r2,[r2,r0,LSL #2] 
        LDR		r0,[r13,#0x10] 
        LDR		r12,[r13,#0xc] 
        MLA		r0,r12,r0,r3 
        LDR		r12,[r13,#0x28] 
        LDRH		r6,[r10],#2 
        ADD		r0,r12,r0,LSL #2 
        CMP		r3,r6 
        BGE		Branch_APPLY_1 @ 0x3fc
        RSB		r9,r1,#0 
Branch_APPLY_4:
        LDR		r4,[r0,#0xc] 
        LDMIA		r0,{r7,r12,r14} 
        CMP		r1,#0 
        BGE		Branch_APPLY_2 @ 0x3a0
        MOV		r7,r7,ASR r9 
        MOV		r12,r12,ASR r9 
        MOV		r14,r14,ASR r9 
        MOV		r4,r4,ASR r9 
        B		Branch_APPLY_3 @ 0x3b0
Branch_APPLY_2:
        MOV		r7,r7,LSL r1 
        MOV		r12,r12,LSL r1 
        MOV		r14,r14,LSL r1 
        MOV		r4,r4,LSL r1 
Branch_APPLY_3:
        SMULL		r7,r8,r2,r7 
        MOV		r7,r7,LSR #28 
        ORR		r7,r7,r8,LSL #4 
        STR		r7,[r0],#4 
        SMULL		r12,r7,r2,r12 
        MOV		r12,r12,LSR #28 
        ORR		r12,r12,r7,LSL #4 
        STR		r12,[r0],#4 
        SMULL		r12,r14,r2,r14 
        MOV		r12,r12,LSR #28 
        ORR		r12,r12,r14,LSL #4 
        STR		r12,[r0],#4 
        SMULL		r12,r14,r2,r4 
        MOV		r12,r12,LSR #28 
        ORR		r12,r12,r14,LSL #4 
        ADD		r3,r3,#4 
        CMP		r3,r6 
        STR		r12,[r0],#4 
        BLT		Branch_APPLY_4 @ 0x37c
Branch_APPLY_1:
        SUBS		r5,r5,#1 
        BNE		Branch_APPLY_5 @ 0x334
Branch_APPLY_0:
        LDR		r10,[r13,#8] 
        LDR		r1,[r13,#0] 
        ADD		r10,r10,#0xf0 
        STR		r10,[r13,#8] 
        LDRB		r0,[r1],#1 
        STR		r1,[r13,#0] 
        LDR		r1,[r13,#0x10] 
        LDR		r11,[r13,#4] 
        ADD		r0,r0,r1 
        STR		r0,[r13,#0x10] 
        LDR		r0,[r13,#0x14] 
        ADD		r11,r11,#0x66 
        SUBS		r0,r0,#1 
        STR		r0,[r13,#0x14] 
        STR		r11,[r13,#4] 
        BNE		Branch_APPLY_6 @ 0x310
        ADD		r13,r13,#0x30 
        LDMFD		r13!,{r4-r11,pc} 

    .end

