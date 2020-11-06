    
    
    .global ifilter_bank

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

ifilter_bank:

        STMFD		r13!,{r3-r11,r14}
 
        MOV		r7,r0
 
        MOV		r0,r1
 
        LDR		r11,[r7,r2,LSL #2]
 
        ADD		r5,r13,#0x28
 
        ADD		r2,r7,r2,LSL #2
 
        LDR		r9,[r2,#8]
 
        ADD		r2,r7,r3,LSL #2
 
        LDMIA		r5,{r1,r4,r5}
 
        LDR		r10,[r7,r3,LSL #2]
 
        LDR		r8,[r2,#8]
 
        CMP		r0,#0
 
        LDR		r6,=sect_start_end_cb_offset_buf 
        BEQ		Branch_filterbank_0 @ 0x108
        CMP		r0,#1
 
        BEQ		Branch_filterbank_1 @ 0x1c8
        CMP		r0,#2
 
        BEQ		Branch_filterbank_2 @ 0x24c
        CMP		r0,#3
 
        LDMNEFD		r13!,{r3-r11,pc}
 
        MOV		r2,r6
 
        LDR		r0,[r7,#0x14]
 
        BL		faad_imdct
 
        MOV		r0,#0x1c0
 
        B		Branch_filterbank_17 @ 0x3f8
Branch_filterbank_0:
        MOV		r2,r6
 
        LDR		r0,[r7,#0x14]
 
        BL		faad_imdct
 
        MOV		r0,#0x400
 
Branch_filterbank_4:
        LDR		r1,[r6],#4
 
        LDR		r2,[r10],#4
 
        LDR		r3,[r5],#4
 
        SMULL		r12,r1,r2,r1
 
        ADD		r1,r3,r1
 
        STR		r1,[r4],#4
 
        LDR		r1,[r6],#4
 
        LDR		r2,[r10],#4
 
        LDR		r3,[r5],#4
 
        SMULL		r12,r1,r2,r1
 
        ADD		r1,r3,r1
 
        STR		r1,[r4],#4
 
        LDR		r1,[r6],#4
 
        LDR		r2,[r10],#4
 
        LDR		r3,[r5],#4
 
        SMULL		r12,r1,r2,r1
 
        ADD		r1,r3,r1
 
        STR		r1,[r4],#4
 
        LDR		r1,[r6],#4
 
        LDR		r2,[r10],#4
 
        LDR		r3,[r5],#4
 
        SMULL		r12,r1,r2,r1
 
        ADD		r1,r3,r1
 
        STR		r1,[r4],#4
 
        SUBS		r0,r0,#4
 
        BNE		Branch_filterbank_4 @ 0x118
        ADD		r1,r11,#0xc00
 
        ADD		r1,r1,#0x3fc
 
        SUB		r2,r5,#0x1000
 
        MOV		r0,#0x400
 
Branch_filterbank_5:
        LDR		r9,[r6],#4
 
        LDMDA		r1!,{r4,r8}
 
        LDMIA		r6!,{r3,r12,r14}
 
        SMULL		r10,r9,r8,r9
 
        LDR		r5,[r1],#-4
 
        SMULL		r8,r3,r4,r3
 
        LDR		r7,[r1],#-4
 
        SMULL		r4,r12,r5,r12
 
        SMULL		r4,r14,r7,r14
 
        STR		r9,[r2],#4
 
        STMIA		r2!,{r3,r12,r14}
 
        SUBS		r0,r0,#4
 
        BNE		Branch_filterbank_5 @ 0x190
        LDMFD		r13!,{r3-r11,pc}
 
Branch_filterbank_1:
        MOV		r2,r6
 
        LDR		r0,[r7,#0x14]
 
        BL		faad_imdct
 
        MOV		r0,#0x400
 
Branch_filterbank_6:
        LDR		r3,[r6],#4
 
        LDR		r2,[r10],#4
 
        LDR		r1,[r5],#4
 
        SMULL		r12,r3,r2,r3
 
        ADD		r1,r1,r3
 
        STR		r1,[r4],#4
 
        SUBS		r0,r0,#1
 
        BNE		Branch_filterbank_6 @ 0x1d8
        SUB		r0,r5,#0x1000
 
        MOV		r1,#0x1c0
 
Branch_filterbank_7:
        LDR		r2,[r6],#4
 
        SUBS		r1,r1,#1
 
        MOV		r2,r2,ASR #9
 
        STR		r2,[r0],#4
 
        BNE		Branch_filterbank_7 @ 0x200
        ADD		r2,r9,#0x1fc
 
        MOV		r1,#0x80
 
Branch_filterbank_8:
        LDR		r12,[r6],#4
 
        LDR		r3,[r2],#-4
 
        SUBS		r1,r1,#1
 
        SMULL		r14,r12,r3,r12
 
        STR		r12,[r0],#4
 
        BNE		Branch_filterbank_8 @ 0x21c
        MOV		r1,#0x1c0
 
        MOV		r2,#0
 
Branch_filterbank_9:
        SUBS		r1,r1,#1
 
        STR		r2,[r0],#4
 
        BNE		Branch_filterbank_9 @ 0x23c
        LDMFD		r13!,{r3-r11,pc}
 
Branch_filterbank_2:
        MOV		r11,r6
 
        MOV		r10,#8
 
        STR		r1,[r13,#0]
 
Branch_filterbank_10:
        LDR		r0,[r7,#0x10]
 
        LDR		r1,[r13,#0]
 
        MOV		r2,r11
 
        BL		faad_imdct
 
        LDR		r1,[r13,#0]
 
        ADD		r11,r11,#0x400
 
        ADD		r1,r1,#0x200
 
        STR		r1,[r13,#0]
 
        SUBS		r10,r10,#1
 
        BNE		Branch_filterbank_10 @ 0x258
        MOV		r1,#0x1c0
 
Branch_filterbank_11:
        LDMIA		r5!,{r0,r2}
 
        STMIA		r4!,{r0,r2}
 
        SUBS		r1,r1,#2
 
        BNE		Branch_filterbank_11 @ 0x284
        MOV		r2,r5
 
        MOV		r0,r6
 
Branch_filterbank_13:
        ADD		r3,r9,r1,LSL #2
 
        RSB		r12,r1,#0x7f
 
        ADD		r12,r9,r12,LSL #2
 
        LDR		r3,[r3,#0]
 
        LDR		r14,[r12,#0]
 
        CMP		r1,#0x40
 
        BGE		Branch_filterbank_12 @ 0x2d4
        LDR		r10,[r0,#0xe00]
 
        LDR		r11,[r2,#0x800]
 
        ADD		r12,r0,#0x1000
 
        LDR		r12,[r12,#0]
 
        SMLAL		r7,r11,r14,r10
 
        SMLAL		r7,r11,r3,r12
 
        STR		r11,[r4,#0x800]
 
Branch_filterbank_12:
        LDR		r12,[r0,#0xa00]
 
        LDR		r11,[r2,#0x600]
 
        LDR		r10,[r0,#0xc00]
 
        SMLAL		r7,r11,r14,r12
 
        MOV		r12,r7
 
        SMLAL		r12,r11,r3,r10
 
        STR		r11,[r4,#0x600]
 
        LDR		r10,[r0,#0x600]
 
        LDR		r7,[r2,#0x400]
 
        LDR		r11,[r0,#0x800]
 
        SMLAL		r12,r7,r14,r10
 
        SMLAL		r12,r7,r3,r11
 
        STR		r7,[r4,#0x400]
 
        LDR		r10,[r0,#0x200]
 
        LDR		r7,[r2,#0x200]
 
        LDR		r11,[r0,#0x400]
 
        SMLAL		r12,r7,r14,r10
 
        SMLAL		r12,r7,r3,r11
 
        STR		r7,[r4,#0x200]
 
        LDR		r14,[r0],#4
 
        LDR		r10,[r8],#4
 
        LDR		r3,[r2],#4
 
        MOV		r7,r12
 
        SMLAL		r7,r3,r10,r14
 
        ADD		r1,r1,#1
 
        CMP		r1,#0x80
 
        STR		r3,[r4],#4
 
        BLT		Branch_filterbank_13 @ 0x29c
        MOV		r0,r5
 
        ADD		r14,r9,#0x1fc
 
        MOV		r3,#0x80
 
Branch_filterbank_15:
        LDR		r1,[r14],#-4
 
        LDR		r2,[r9],#4
 
        CMP		r3,#0x40
 
        BGT		Branch_filterbank_14 @ 0x378
        LDR		r4,[r6,#0xe00]
 
        ADD		r12,r6,#0x1000
 
        LDR		r12,[r12,#0]
 
        SMULL		r4,r7,r1,r4
 
        SMLAL		r4,r7,r2,r12
 
        STR		r7,[r0,#-0x800]
 
Branch_filterbank_14:
        ADD		r12,r6,#0x1000
 
        LDR		r4,[r12,#0x200]
 
        LDR		r12,[r12,#0x400]
 
        SMULL		r4,r7,r1,r4
 
        SMLAL		r4,r7,r2,r12
 
        STR		r7,[r0,#-0x600]
 
        ADD		r12,r6,#0x1000
 
        LDR		r4,[r12,#0x600]
 
        LDR		r12,[r12,#0x800]
 
        SMULL		r4,r7,r1,r4
 
        SMLAL		r4,r7,r2,r12
 
        STR		r7,[r0,#-0x400]
 
        ADD		r12,r6,#0x1000
 
        LDR		r4,[r12,#0xa00]
 
        LDR		r12,[r12,#0xc00]
 
        SMULL		r4,r7,r1,r4
 
        SMLAL		r4,r7,r2,r12
 
        STR		r7,[r0,#-0x200]
 
        ADD		r12,r6,#0x1000
 
        LDR		r2,[r12,#0xe00]
 
        ADD		r6,r6,#4
 
        SMULL		r12,r2,r1,r2
 
        STR		r2,[r0],#4
 
        SUBS		r3,r3,#1
 
        BNE		Branch_filterbank_15 @ 0x350
        ADD		r1,r5,#0x200
 
        MOV		r0,#0x1c0
 
        MOV		r2,#0
 
Branch_filterbank_16:
        SUBS		r0,r0,#1
 
        STR		r2,[r1],#4
 
        BNE		Branch_filterbank_16 @ 0x3e8
        LDMFD		r13!,{r3-r11,pc}
 
Branch_filterbank_17:
        LDMIA		r5!,{r1,r2}
 
        STMIA		r4!,{r1,r2}
 
        SUBS		r0,r0,#2
 
        BNE		Branch_filterbank_17 @ 0x3f8
        ADD		r1,r6,#0x700
 
        MOV		r0,#0x80
 
Branch_filterbank_18:
        LDR		r12,[r1],#4
 
        LDR		r3,[r8],#4
 
        LDR		r2,[r5],#4
 
        SMULL		r14,r12,r3,r12
 
        ADD		r2,r2,r12
 
        STR		r2,[r4],#4
 
        SUBS		r0,r0,#1
 
        BNE		Branch_filterbank_18 @ 0x410
        MOV		r0,#0x1c0
 
Branch_filterbank_19:
        LDR		r2,[r1],#4
 
        LDR		r3,[r5],#4
 
        SUBS		r0,r0,#1
 
        ADD		r2,r3,r2,ASR #9
 
        STR		r2,[r4],#4
 
        BNE		Branch_filterbank_19 @ 0x434
        ADD		r0,r11,#0xc00
 
        ADD		r0,r0,#0x3fc
 
        SUB		r3,r5,#0x1000
 
        MOV		r2,#0x400
 
Branch_filterbank_20:
        LDR		r12,[r1],#4
 
        LDR		r14,[r0],#-4
 
        SUBS		r2,r2,#1
 
        SMULL		r4,r14,r12,r14
 
        STR		r14,[r3],#4
 
        BNE		Branch_filterbank_20 @ 0x45c
        LDMFD		r13!,{r3-r11,pc}
 

Var0:        .word		0  @ AXDDCD0 0x00000488


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

    .end

