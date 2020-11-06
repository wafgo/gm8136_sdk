    .global is_decode

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

is_decode:
        STMFD		r13!,{r0-r11,r14} 
        SUB		r13,r13,#0x1c 
        MOV		r2,#0 
        STR		r2,[r13,#0] 
        STR		r2,[r13,#8] 
        LDRB		r2,[r1,#2] 
        CMP		r2,#0 
        ADDLE		r13,r13,#0x2c 
        LDMLEFD		r13!,{r4-r11,pc} 
        MOV		r3,#1 
Branch_is_8:
        LDR		r2,[r13,#8] 
        MOV		r12,#0 
        ADD		r2,r1,r2 
        STR		r2,[r13,#0x18] 
        STR		r12,[r13,#4] 
        LDRB		r2,[r2,#5] 
        CMP		r2,#0 
        BLE		Branch_is_0 @ 0x1c4
        LDR		r2,[r13,#8] 
        ADD		r2,r2,r2,LSL #4 
        ADD		r2,r2,r2,LSL #1 
        STR		r2,[r13,#0x14] 
        LDR		r2,[r13,#8] 
        AND		r2,r2,#0xff 
        ADD		r7,r2,r2,LSL #4 
        ADD		r7,r7,r7,LSL #1 
        RSB		r14,r2,r2,LSL #4 
        STR		r14,[r13,#0x10] 
        STR		r7,[r13,#0xc] 
Branch_is_7:
        LDRB		r2,[r1,#0] 
        MOV		r12,#0 
        CMP		r2,#0 
        BLE		Branch_is_1 @ 0x19c
Branch_is_6:
        LDR		r2,[r1,#0x78] 
        LDR		r14,[r13,#0x10] 
        AND		r6,r12,#0xff 
        ADD		r9,r6,r14,LSL #3 
        ADD		r2,r2,#0x12c0 
        LDRB		r2,[r2,r9] 
        CMP		r2,#0xe 
        CMPNE		r2,#0xf 
        BNE		Branch_is_3 @ 0x18c
        MOV		r11,#0 
        ADD		r2,r0,r12 
        STRB		r11,[r2,#0x9f0] 
        ADD		r2,r1,r12 
        STRB		r11,[r2,#0x9f0] 
        LDR		r2,[r13,#0x14] 
        LDR		r14,=pow05_table 
        ADD		r2,r2,r12 
        ADD		r2,r1,r2,LSL #1 
        LDRSH		r2,[r2,#0x7c] 
        MOV		r4,r2,ASR #2 
        AND		r2,r2,#3 
        ADD		r2,r14,r2,LSL #2 
        LDR		r8,[r2,#0xc] 
        ADD		r2,r1,r12,LSL #1 
        LDRH		r14,[r2,#0x10] 
        LDRH		r7,[r2,#0x12] 
        LDR		r2,[r13,#0] 
        LDR		r10,[r13,#0x24] 
        ADD		r5,r14,r2,LSL #7 
        LDR		r2,[r13,#0x28] 
        SUBS		r14,r7,r14 
        ADD		r2,r2,r5,LSL #2 
        ADD		r5,r10,r5,LSL #2 
        BEQ		Branch_is_3 @ 0x18c
        LDR		r7,[r13,#0xc] 
        RSB		r10,r4,#0 
        ADD		r6,r7,r6 
        ADD		r11,r6,r0 
Branch_is_5:
        LDR		r6,[r5],#4 
        CMP		r4,#0 
        MOVGE		r6,r6,ASR r4 
        MOVLT		r6,r6,LSL r10 
        SMULL		r6,r7,r8,r6 
        MOV		r6,r6,LSR #28 
        ORR		r6,r6,r7,LSL #4 
        STR		r6,[r2],#4 
        LDR		r6,[r1,#0x78] 
        ADD		r6,r6,#0x12c0 
        LDRB		r6,[r6,r9] 
        CMP		r6,#0xe 
        MVNEQ		r6,#0 
        BEQ		Branch_is_4 @ 0x160
        CMP		r6,#0xf 
        MOVEQ		r6,r3 
        MOVNE		r6,#0 
Branch_is_4:
        LDRB		r7,[r0,#0x3b5] 
        CMP		r7,#1 
        LDREQB		r7,[r11,#0x3b6] 
        MOVNE		r7,#1 
        SUBEQ		r7,r3,r7,LSL #1 
        CMP		r6,r7 
        LDRNE		r6,[r2,#-4] 
        RSBNE		r6,r6,#0 
        STRNE		r6,[r2,#-4] 
        SUBS		r14,r14,#1 
        BNE		Branch_is_5 @ 0x11c
Branch_is_3:
        LDRB		r2,[r1,#0] 
        ADD		r12,r12,#1 
        CMP		r2,r12 
        BGT		Branch_is_6 @ 0x84
Branch_is_1:
        LDR		r2,[r13,#0] 
        LDR		r12,[r13,#4] 
        ADD		r2,r2,#1 
        STR		r2,[r13,#0] 
        LDR		r2,[r13,#0x18] 
        ADD		r12,r12,#1 
        STR		r12,[r13,#4] 
        LDRB		r2,[r2,#5] 
        CMP		r2,r12 
        BGT		Branch_is_7 @ 0x74
Branch_is_0:
        LDR		r2,[r13,#8] 
        ADD		r2,r2,#1 
        STR		r2,[r13,#8] 
        LDRB		r2,[r1,#2] 
        LDR		r12,[r13,#8] 
        CMP		r2,r12 
        BGT		Branch_is_8 @ 0x28
        ADD		r13,r13,#0x2c 
        LDMFD		r13!,{r4-r11,pc} 

    .end

