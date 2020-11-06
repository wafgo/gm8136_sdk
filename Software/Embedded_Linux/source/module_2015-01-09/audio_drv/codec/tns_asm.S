    .global tns_decode_frame
    
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

tns_decode_frame:
        STMFD		r13!,{r0-r11,r14}
 
        LDRB		r1,[r0,#0x550]
 
        SUB		r13,r13,#0xd0
 
        CMP		r1,#0
 
        LDRNEB		r1,[r0,#3]
 
        MOVNE		r6,#0
 
        CMPNE		r1,#0
 
        ADDLE		r13,r13,#0xe0
 
        LDMLEFD		r13!,{r4-r11,pc}
 
        LDR		r2,[r13,#0xd8]
 
        LDR		r1,=tns_sbf_max 
        ADD		r14,r13,#8
 
        ADD		r2,r1,r2,LSL #1
 
        STR		r2,[r13,#0xc8]
 
        ADD		r2,r13,#0x60
 
        STR		r2,[r13,#0xc4]
 
        STR		r14,[r13,#0xcc]
 
Branch_tns_19:
        LDRB		r9,[r0,#1]
 
        LDR		r1,[r13,#0xd4]
 
        MOV		r7,#0
 
        LDRB		r1,[r1,r6]
 
        CMP		r1,#0
 
        BLE		Branch_tns_0 @ 0x2dc
        LDR		r1,[r13,#0xd4]
 
        ADD		r3,r1,r6,LSL #7
 
        ADD		r1,r1,r6
 
        STR		r1,[r13,#0xbc]
 
        LDR		r1,[r13,#0xd4]
 
        STR		r3,[r13,#0xc0]
 
        ADD		r1,r1,r6,LSL #2
 
        STR		r1,[r13,#0xb8]
 
Branch_tns_18:
        LDR		r1,[r13,#0xb8]
 
        MOV		r10,r9
 
        ADD		r11,r1,r7
 
        LDRB		r1,[r11,#0x10]
 
        SUB		r9,r9,r1
 
        LDRB		r1,[r11,#0x30]
 
        CMP		r9,#0
 
        MOVLE		r9,#0
 
        CMP		r1,#0x14
 
        MOVCS		r1,#0x14
 
        CMP		r1,#0
 
        BEQ		Branch_tns_12 @ 0x2c8
        AND		r1,r1,#0xff
 
        STR		r1,[r13,#0xb4]
 
        STR		r1,[r13,#0x58]
 
        LDR		r1,[r13,#0xbc]
 
        LDRB		r1,[r1,#8]
 
        LDRB		r2,[r11,#0x70]
 
        LDR		r3,[r13,#0xc0]
 
        ADD		r1,r1,#3
 
        ADD		r3,r3,r7,LSL #5
 
        ADD		r8,r3,#0x90
 
        LDR		r3,=tns_coef 
        AND		r1,r1,#0xff
 
        ADD		r2,r3,r2,LSL #7
 
        ADD		r1,r2,r1,LSL #6
 
        SUB		r2,r1,#0xc0
 
        MOV		r1,#0x10000000
 
        STR		r1,[r13,#0x5c]
 
        LDR		r1,[r13,#0x58]
 
        STR		r2,[r13,#0]
 
        MOV		r3,#1
 
        CMP		r1,#1
 
        BLT		Branch_tns_2 @ 0x184
Branch_tns_7:
        LDRB		r1,[r8],#1
 
        LDR		r2,[r13,#0]
 
        CMP		r3,#1
 
        LDR		r12,[r2,r1,LSL #2]
 
        MOV		r1,#1
 
        BLE		Branch_tns_3 @ 0x150
Branch_tns_4:
        ADD		r14,r13,#0x5c
 
        SUB		r2,r3,r1
 
        LDR		r2,[r14,r2,LSL #2]
 
        LDR		r5,[r14,r1,LSL #2]
 
        SMULL		r14,r4,r12,r2
 
        MOV		r2,r14,LSR #28
 
        ORR		r2,r2,r4,LSL #4
 
        ADD		r2,r2,r5
 
        ADD		r14,r13,#4
 
        STR		r2,[r14,r1,LSL #2]
 
        ADD		r1,r1,#1
 
        CMP		r1,r3
 
        BLT		Branch_tns_4 @ 0x11c
Branch_tns_3:
        SUBS		r1,r3,#1
 
        LDR		r2,[r13,#0xc4]
 
        LDR		r14,[r13,#0xcc]
 
        BEQ		Branch_tns_5 @ 0x170
Branch_tns_6:
        LDR		r4,[r14],#4
 
        SUBS		r1,r1,#1
 
        STR		r4,[r2],#4
 
        BNE		Branch_tns_6 @ 0x160
Branch_tns_5:
        STR		r12,[r2,#0]
 
        LDR		r1,[r13,#0x58]
 
        ADD		r3,r3,#1
 
        CMP		r3,r1
 
        BLE		Branch_tns_7 @ 0x104
Branch_tns_2:
        LDRB		r12,[r0,#4]
 
        CMP		r12,#2
 
        MOVNE		r1,#0
 
        MOVEQ		r1,#1
 
        CMP		r1,#0
 
        LDREQ		r2,[r13,#0xd8]
 
        LDREQ		r3,=tns_sbf_max 
        LDRNE		r2,[r13,#0xc8]
 
        LDREQB		r2,[r3,r2,LSL #1]
 
        LDRNEB		r2,[r2,#1]
 
        CMP		r2,r9
 
        MOVGT		r14,r9
 
        BGT		Branch_tns_8 @ 0x1bc
        MOV		r14,r2
 
Branch_tns_8:
        LDRB		r3,[r0,#0]
 
        CMP		r3,r14
 
        MOVLE		r14,r3
 
        CMP		r2,r10
 
        MOVGT		r2,r10
 
        CMP		r3,r2
 
        MOVLE		r2,r3
 
        ADD		r14,r0,r14,LSL #1
 
        LDRH		r14,[r14,#0x10]
 
        ADD		r1,r0,r2,LSL #1
 
        LDRH		r1,[r1,#0x10]
 
        SUB		r3,r1,r14
 
        CMP		r3,#0
 
        BLE		Branch_tns_12 @ 0x2c8
        LDRB		r2,[r11,#0x50]
 
        LDR		r12,[r13,#0x104]
 
        LDR		r11,[r13,#0xb4]
 
        CMP		r2,#0
 
        MVNNE		r2,#0
 
        ADDNE		r14,r1,r2
 
        MOVEQ		r2,#1
 
        MOV		r2,r2,LSL #24
 
        ADD		r1,r14,r6,LSL #7
 
        ADD		r1,r12,r1,LSL #2
 
        MOV		r2,r2,ASR #24
 
        MOV		r4,r3,LSL #16
 
        STR		r2,[r13,#0x58]
 
        ADD		r12,r13,#8
 
        MOV		r3,r12
 
        MOVS		r2,r11
 
        MOV		r4,r4,LSR #16
 
        BEQ		Branch_tns_10 @ 0x24c
        MOV		r14,#0
 
Branch_tns_11:
        SUBS		r2,r2,#1
 
        STR		r14,[r3],#4
 
        BNE		Branch_tns_11 @ 0x240
Branch_tns_10:
        CMP		r4,#0
 
        BEQ		Branch_tns_12 @ 0x2c8
        SUB		r2,r11,#1
 
        STR		r2,[r13,#0xb0]
 
Branch_tns_17:
        LDR		r3,[r1,#0]
 
        LDR		r14,[r13,#0xc4]
 
        ADD		r12,r13,#8
 
        MOVS		r2,r11
 
        BEQ		Branch_tns_13 @ 0x290
Branch_tns_14:
        LDR		r8,[r12],#4
 
        LDR		r5,[r14],#4
 
        SUBS		r2,r2,#1
 
        SMULL		r8,r10,r5,r8
 
        MOV		r5,r8,LSR #28
 
        ORR		r5,r5,r10,LSL #4
 
        SUB		r3,r3,r5
 
        BNE		Branch_tns_14 @ 0x270
Branch_tns_13:
        LDR		r2,[r13,#0xb0]
 
        SUB		r12,r12,#4
 
        CMP		r2,#0
 
        BEQ		Branch_tns_15 @ 0x2b0
Branch_tns_16:
        LDR		r14,[r12,#-4]
 
        SUBS		r2,r2,#1
 
        STR		r14,[r12],#-4
 
        BNE		Branch_tns_16 @ 0x2a0
Branch_tns_15:
        STR		r3,[r12,#0]
 
        STR		r3,[r1,#0]
 
        LDR		r2,[r13,#0x58]
 
        SUBS		r4,r4,#1
 
        ADD		r1,r1,r2,LSL #2
 
        BNE		Branch_tns_17 @ 0x25c
Branch_tns_12:
        LDR		r1,[r13,#0xd4]
 
        ADD		r7,r7,#1
 
        LDRB		r1,[r1,r6]
 
        CMP		r1,r7
 
        BGT		Branch_tns_18 @ 0x7c
Branch_tns_0:
        LDRB		r1,[r0,#3]
 
        ADD		r6,r6,#1
 
        CMP		r1,r6
 
        BGT		Branch_tns_19 @ 0x44
        ADD		r13,r13,#0xe0
 
        LDMFD		r13!,{r4-r11,pc}
 
  

    .end

