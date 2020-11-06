
  .text
  .align 0
    
    .global	passf4pos_new
    .global	cfftb
    
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
    
    
    
    
cfftb:

        STMFD		r13!,{r4-r11,r14}
 
        MOV		r10,r1
 
        LDRH		r1,[r0,#0]
 
        LDR		r4,[r0,#4]
 
        SUB		r13,r13,#0xc
 
        MOV		r5,#0
 
        CMP		r1,#0x200
 
        BNE		Branch_cfftb_A @ 0xa8
        LDR		r7,=cfft_tab_512 
        MOV		r1,r10
 
        MOV		r0,r4
 
        MOV		r2,#0x100
 
Branch_cfftb_B:
        LDMIA		r1!,{r3,r14}
 
        LDR		r12,[r1,#0x7f8]
 
        LDR		r5,[r1,#0x7fc]
 
        ADD		r8,r3,r12
 
        ADD		r6,r14,r5
 
        SUB		r5,r14,r5
 
        STR		r8,[r0],#4
 
        STR		r6,[r0],#4
 
        SUB		r14,r3,r12
 
        LDMIA		r7!,{r3,r12}
 
        SMULL		r8,r6,r3,r5
 
        SMLAL		r8,r6,r12,r14
 
        SMULL		r14,r8,r3,r14
 
        RSB		r5,r5,#0
 
        MOV		r3,r6,LSL #1
 
        STR		r3,[r0,#0x7fc]
 
        SMLAL		r14,r8,r12,r5
 
        MOV		r3,r8,LSL #1
 
        STR		r3,[r0,#0x7f8]
 
        SUBS		r2,r2,#1
 
        BNE		Branch_cfftb_B @ 0x30
        LDR		r2,=cfft_tab_512_64 
        MOV		r3,#4
 
        STMIA		r13,{r2,r3}
 
        MOV		r3,r10
 
        MOV		r2,r4
 
        MOV		r1,#2
 
        MOV		r0,#0x40
 
        BL		passf4pos_new
 
        MOV		r5,#3
 
Branch_cfftb_A:
        LDR		r2,=cfft_tab_512_16 
        MOV		r3,#0x10
 
        STMIA		r13,{r2,r3}
 
        MOV		r0,#1
 
        MOV		r1,r0,LSL r5
 
        MOV		r0,#0x10
 
        MOV		r3,r4
 
        MOV		r2,r10
 
        BL		passf4pos_new
 
        LDR		r2,=cfft_tab_512_4 
        MOV		r3,#0x40
 
        STMIA		r13,{r2,r3}
 
        MOV		r0,#4
 
        MOV		r1,r0,LSL r5
 
        MOV		r3,r10
 
        MOV		r2,r4
 
        BL		passf4pos_new
 
        MOV		r0,#0x10
 
        MOV		r0,r0,LSL r5
 
        MOVS		r2,r0
 
        ADDEQ		r13,r13,#0xc
 
        MOV		r1,r4
 
        LDMEQFD		r13!,{r4-r11,pc}
 
        ADD		r11,r0,r0,LSL #1
 
Branch_cfftb_C:
        LDMIA		r10!,{r3,r14}
 
        LDMIA		r10!,{r6,r8}
 
        LDR		r5,[r10],#4
 
        LDMIA		r10!,{r4,r7,r9}
 
        ADD		r12,r3,r5
 
        SUB		r3,r3,r5
 
        ADD		r5,r14,r4
 
        SUB		r4,r14,r4
 
        ADD		r14,r6,r7
 
        SUB		r7,r6,r7
 
        ADD		r6,r9,r8
 
        SUB		r8,r9,r8
 
        ADD		r9,r12,r14
 
        SUB		r14,r12,r14
 
        ADD		r12,r5,r6
 
        SUB		r6,r5,r6
 
        ADD		r5,r3,r8
 
        SUB		r8,r3,r8
 
        STR		r8,[r1,r11,LSL #3]
 
        STR		r14,[r1,r0,LSL #4]
 
        STR		r5,[r1,r0,LSL #3]
 
        ADD		r3,r4,r7
 
        SUB		r4,r4,r7
 
        STR		r9,[r1],#4
 
        STR		r4,[r1,r11,LSL #3]
 
        STR		r6,[r1,r0,LSL #4]
 
        STR		r3,[r1,r0,LSL #3]
 
        STR		r12,[r1],#4
 
        SUBS		r2,r2,#1
 
        BNE		Branch_cfftb_C @ 0x108
        ADD		r13,r13,#0xc
 
        LDMFD		r13!,{r4-r11,pc}     
    
passf4pos_new:

        STMFD		r13!,{r0-r11,r14}
 
        SUB		r13,r13,#0x20
 
        STR		r1,[r13,#0xc]
 
        CMP		r1,#0
 
        ADDEQ		r13,r13,#0x30
 
        LDMEQFD		r13!,{r4-r11,pc}
 
        MOV		r2,r1,LSL #1
 
        MUL		r6,r0,r2
 
        MOV		r2,r1,LSL #2
 
        MUL		r5,r0,r2
 
        STR		r5,[r13,#0x18]
 
        MUL		r1,r0,r1
 
        ADD		r5,r1,r1,LSL #1
 
        ADD		r3,r0,r0,LSL #1
 
        STR		r3,[r13,#0x10]
 
        STR		r5,[r13,#0x14]
 
        STR		r6,[r13,#0x1c]
 
Branch_pass4pos_C:
        LDR		r3,[r13,#0x54]
 
        LDR		r12,=cfft_tab_512 
        STR		r3,[r13,#8]
 
        LDR		r2,[r13,#0x28]
 
        MOVS		r4,r0
 
        LDR		r1,[r13,#0x2c]
 
        MOV		r14,r12
 
        BEQ		Branch_pass4pos_A @ 0x198
Branch_pass4pos_B:
        LDR		r3,[r13,#0x10]
 
        LDR		r5,[r2,r0,LSL #4]
 
        LDR		r8,[r2,r3,LSL #3]
 
        LDR		r3,[r2,r0,LSL #3]
 
        LDR		r7,[r2],#4
 
        ADD		r6,r3,r8
 
        SUB		r8,r3,r8
 
        ADD		r3,r7,r5
 
        SUB		r11,r3,r6
 
        SUB		r5,r7,r5
 
        ADD		r7,r3,r6
 
        STR		r11,[r13,#0]
 
        STR		r7,[r1,#0]
 
        LDR		r3,[r13,#0x10]
 
        LDR		r10,[r2,r0,LSL #4]
 
        LDR		r6,[r2,r0,LSL #3]
 
        LDR		r9,[r2,r3,LSL #3]
 
        LDR		r3,[r2],#4
 
        SUBS		r4,r4,#1
 
        ADD		r7,r3,r10
 
        SUB		r3,r3,r10
 
        ADD		r10,r9,r6
 
        SUB		r6,r9,r6
 
        ADD		r11,r5,r6
 
        SUB		r5,r5,r6
 
        ADD		r9,r7,r10
 
        ADD		r6,r3,r8
 
        SUB		r8,r3,r8
 
        STR		r9,[r1,#4]
 
        LDR		r3,[r13,#8]
 
        STR		r11,[r13,#4]
 
        SUB		r7,r7,r10
 
        LDMIA		r3!,{r9,r10}
 
        STR		r3,[r13,#8]
 
        SMULL		r3,r11,r9,r8
 
        SMLAL		r3,r11,r10,r5
 
        RSB		r3,r8,#0
 
        SMULL		r5,r8,r9,r5
 
        SMLAL		r5,r8,r10,r3
 
        LDR		r5,[r13,#0x14]
 
        MOV		r3,r8,LSL #1
 
        STR		r3,[r1,r5,LSL #3]
 
        LDMIA		r14,{r3,r5}
 
        MOV		r9,r11,LSL #1
 
        LDR		r11,[r13,#0]
 
        SMULL		r10,r8,r3,r7
 
        SMLAL		r10,r8,r5,r11
 
        SMULL		r10,r11,r3,r11
 
        RSB		r7,r7,#0
 
        SMLAL		r10,r11,r5,r7
 
        LDR		r5,[r13,#0x18]
 
        MOV		r3,r11,LSL #1
 
        STR		r3,[r1,r5,LSL #2]
 
        LDMIA		r12,{r3,r5}
 
        SMULL		r10,r7,r3,r6
 
        LDR		r11,[r13,#4]
 
        RSB		r6,r6,#0
 
        SMLAL		r10,r7,r5,r11
 
        SMULL		r10,r11,r3,r11
 
        SMLAL		r10,r11,r5,r6
 
        LDR		r6,[r13,#0x1c]
 
        LDR		r3,[r13,#0x58]
 
        MOV		r5,r11,LSL #1
 
        STR		r5,[r1,r6,LSL #2]
 
        LDR		r5,[r13,#0x14]
 
        ADD		r1,r1,#4
 
        STR		r9,[r1,r5,LSL #3]
 
        LDR		r5,[r13,#0x18]
 
        MOV		r8,r8,LSL #1
 
        STR		r8,[r1,r5,LSL #2]
 
        LDR		r6,[r13,#0x1c]
 
        ADD		r14,r14,r3,LSL #3
 
        ADD		r12,r12,r3,LSL #2
 
        MOV		r3,r7,LSL #1
 
        STR		r3,[r1,r6,LSL #2]
 
        ADD		r1,r1,#4
 
        BNE		Branch_pass4pos_B @ 0x64
Branch_pass4pos_A:
        LDR		r1,[r13,#0x2c]
 
        LDR		r2,[r13,#0x28]
 
        ADD		r1,r1,r0,LSL #3
 
        STR		r1,[r13,#0x2c]
 
        LDR		r1,[r13,#0xc]
 
        ADD		r2,r2,r0,LSL #5
 
        SUBS		r1,r1,#1
 
        STR		r1,[r13,#0xc]
 
        STR		r2,[r13,#0x28]
 
        BNE		Branch_pass4pos_C @ 0x44
        ADD		r13,r13,#0x30
 
        LDMFD		r13!,{r4-r11,pc} 

    .end

