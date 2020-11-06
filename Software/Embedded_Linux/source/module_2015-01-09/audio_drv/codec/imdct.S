    
    .global faad_imdct

 /* Security check: asm protection by Faraday's commercial number */

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

faad_imdct:
        STMFD		r13!,{r3-r11,r14} 
        LDR		r10,[r0,#8] 
        LDRH		r4,[r0,#0] 
        MOV		r5,r0 
        MOV		r9,r1 
        MOV		r0,r4,LSR #1 
        ADD		r0,r1,r0,LSL #2 
        SUB		r11,r0,#4 
        MOVS		r0,r4,LSR #2 
        MOV		r8,r2 
        MOV		r6,r2 
        BEQ		Branch_IMDCT_A @ 0x20c
Branch_IMDCT_B:
        LDR		r14,[r9],#8 
        LDMIA		r10!,{r3,r12} 
        LDR		r2,[r11],#-8 
        SMULL		r1,r7,r3,r14 
        SMLAL		r1,r7,r12,r2 
        RSB		r1,r14,#0 
        SMULL		r2,r14,r3,r2 
        SMLAL		r2,r14,r12,r1 
        MOV		r2,r14,LSL #1 
        STR		r2,[r8],#4 
        MOV		r1,r7,LSL #1 
        STR		r1,[r8],#4 
        SUBS		r0,r0,#1 
        BNE		Branch_IMDCT_B @ 0x1d4
Branch_IMDCT_A:
        MOV		r1,r6 
        LDR		r0,[r5,#4] 
        BL		cfftb 
        LDR		r1,[r5,#4]! 
        LDR		r0,[r5,#4] 
        LDR		r9,[r1,#4] 
        MOV		r2,r4,ASR #2 
        MOVS		r3,r2 
        MOV		r1,r9 
        BEQ		Branch_IMDCT_C @ 0x270
Branch_IMDCT_D:
        LDR		r14,[r0],#4 
        LDR		r7,[r1,#4] 
        LDR		r5,[r0],#4 
        LDR		r12,[r1,#0] 
        SMULL		r10,r8,r14,r7 
        SMLAL		r10,r8,r5,r12 
        SMULL		r12,r10,r14,r12 
        RSB		r7,r7,#0 
        SMLAL		r12,r10,r5,r7 
        MOV		r14,r10,LSL #1 
        STR		r14,[r1],#4 
        MOV		r12,r8,LSL #1 
        STR		r12,[r1],#4 
        SUBS		r2,r2,#1 
        BNE		Branch_IMDCT_D @ 0x234
Branch_IMDCT_C:
        MOVS		r12,r4,ASR #3 
        ADD		r0,r9,r12,LSL #3 
        ADD		r2,r9,r3,LSL #3 
        SUB		r2,r2,#0x10 
        SUB		r1,r0,#0x10 
        LDMEQFD		r13!,{r3-r11,pc} 
        MOV		r14,r4,ASR #1 
        ADD		r4,r14,r4,ASR #2 
Branch_IMDCT_E:
        LDR		r8,[r1,#8] 
        LDR		r10,[r1,#0] 
        LDR		r5,[r0,#4] 
        LDR		r7,[r0,#0xc] 
        RSB		r8,r8,#0 
        STMIA		r6,{r5,r8} 
        STR		r7,[r6,#8] 
        RSB		r10,r10,#0 
        STR		r10,[r6,#0xc] 
        LDR		r7,[r1,#0xc] 
        LDR		r8,[r1,#4] 
        LDR		r10,[r0],#8 
        LDR		r5,[r0],#8 
        STR		r10,[r6,r14,LSL #2] 
        ADD		r10,r6,r14,LSL #2 
        RSB		r7,r7,#0 
        STR		r7,[r10,#4]! 
        RSB		r8,r8,#0 
        STMIB		r10,{r5,r8} 
        LDR		r7,[r2,#0xc] 
        LDR		r8,[r2,#4] 
        LDR		r10,[r9,#0] 
        LDR		r5,[r9,#8] 
        STR		r10,[r6,r3,LSL #2] 
        ADD		r10,r6,r3,LSL #2 
        RSB		r7,r7,#0 
        STR		r7,[r10,#4]! 
        RSB		r8,r8,#0 
        STMIB		r10,{r5,r8} 
        LDR		r8,[r9,#0xc] 
        LDR		r10,[r9,#4] 
        LDR		r5,[r2,#8] 
        LDR		r7,[r2],#-0x10 
        RSB		r10,r10,#0 
        STR		r10,[r6,r4,LSL #2] 
        ADD		r10,r6,r4,LSL #2 
        RSB		r8,r8,#0 
        STMIB		r10!,{r5,r8} 
        STR		r7,[r10,#4] 
        ADD		r6,r6,#0x10 
        ADD		r9,r9,#0x10 
        SUB		r1,r1,#0x10 
        SUBS		r12,r12,#2 
        BNE		Branch_IMDCT_E @ 0x290
        LDMFD		r13!,{r3-r11,pc} 

    .end

