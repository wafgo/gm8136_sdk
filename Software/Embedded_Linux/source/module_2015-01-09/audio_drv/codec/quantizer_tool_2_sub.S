    		.global 	Scf_CountDiffBit
	
  .text
  .align 0
Scf_CountDiffBit:
        STMFD		r13!,{r4-r8,r14}
 
        LDR		r4,[r13,#0x18]
 
        MOV		r14,#0
 
        MOV		r12,r3
 
        B		Branch19 @ 0x330
Branch21:
        ADD		r12,r12,#1
 
Branch19:
        CMP		r12,r4
 
        BGE		Branch20 @ 0x348
        ADD		r5,r0,r12,LSL #1
 
        LDRH		r5,[r5,#0]
 
        CMP		r5,#0x8000
 
        BEQ		Branch21 @ 0x32c
Branch20:
        SUB		r3,r3,#1
 
        LDR		r7, =huff_ltabscf
 
        B		Branch22 @ 0x358
Branch24:
        SUB		r3,r3,#1
 
Branch22:
        CMP		r3,#0
 
        BLT		Branch23 @ 0x3b0
        ADD		r5,r0,r3,LSL #1
 
        LDRH		r5,[r5,#0]
 
        CMP		r5,#0x8000
 
        BEQ		Branch24 @ 0x354
        ADD		r5,r1,r3,LSL #1
 
        ADD		r6,r1,r12,LSL #1
 
        LDRSH		r6,[r6,#0]
 
        LDRSH		r5,[r5,#0]
 
        ADD		r3,r0,r3,LSL #1
 
        LDRSH		r3,[r3,#0]
 
        SUB		r5,r5,r6
 
        ADD		r6,r0,r12,LSL #1
 
        LDRSH		r6,[r6,#0]
 
        ADD		r5,r7,r5,LSL #1
 
        LDRH		r5,[r5,#0x78]
 
        SUB		r3,r3,r6
 
        ADD		r3,r7,r3,LSL #1
 
        LDRH		r3,[r3,#0x78]
 
        SUB		r3,r5,r3
 
        ADD		r14,r3,r14
 
Branch23:
        ADD		r3,r12,#1
 
        B		Branch25 @ 0x40c
Branch27:
        ADD		r5,r0,r3,LSL #1
 
        LDRH		r6,[r5,#0]
 
        CMP		r6,#0x8000
 
        BEQ		Branch26 @ 0x408
        ADD		r6,r1,r12,LSL #1
 
        ADD		r8,r1,r3,LSL #1
 
        LDRSH		r8,[r8,#0]
 
        LDRSH		r6,[r6,#0]
 
        ADD		r12,r0,r12,LSL #1
 
        LDRSH		r12,[r12,#0]
 
        LDRSH		r5,[r5,#0]
 
        SUB		r6,r6,r8
 
        ADD		r6,r7,r6,LSL #1
 
        SUB		r12,r12,r5
 
        ADD		r12,r7,r12,LSL #1
 
        LDRH		r6,[r6,#0x78]
 
        LDRH		r12,[r12,#0x78]
 
        SUB		r12,r6,r12
 
        ADD		r14,r12,r14
 
        MOV		r12,r3
 
Branch26:
        ADD		r3,r3,#1
 
Branch25:
        CMP		r3,r4
 
        BLT		Branch27 @ 0x3b8
        B		Branch28 @ 0x41c
Branch30:
        ADD		r4,r4,#1
 
Branch28:
        CMP		r4,r2
 
        BGE		Branch29 @ 0x434
        ADD		r3,r0,r4,LSL #1
 
        LDRH		r3,[r3,#0]
 
        CMP		r3,#0x8000
 
        BEQ		Branch30 @ 0x418
Branch29:
        CMP		r4,r2
 
        BGE		Branch31 @ 0x47c
        ADD		r2,r1,r12,LSL #1
 
        ADD		r1,r1,r4,LSL #1
 
        LDRSH		r1,[r1,#0]
 
        LDRSH		r2,[r2,#0]
 
        SUB		r1,r2,r1
 
        ADD		r2,r0,r12,LSL #1
 
        ADD		r0,r0,r4,LSL #1
 
        LDRSH		r0,[r0,#0]
 
        LDRSH		r2,[r2,#0]
 
        ADD		r1,r7,r1,LSL #1
 
        LDRH		r1,[r1,#0x78]
 
        SUB		r0,r2,r0
 
        ADD		r0,r7,r0,LSL #1
 
        LDRH		r0,[r0,#0x78]
 
        SUB		r0,r1,r0
 
        ADD		r14,r0,r14
 
Branch31:
        MOV		r0,r14
 
        BL		saturate @ 0x0
        MOV		r0,r0,LSL #16
 
        MOV		r0,r0,ASR #16
 
        LDMFD		r13!,{r4-r8,pc}

saturate:
        MOV		r1,r0
 
        MOV		r2,r1,ASR #15
 
        TEQ		r2,r1,ASR #31
 
        LDRNE		r0, =0x00007fff 
        EORNE		r0,r0,r1,ASR #31
 
        MOVEQ		r0,r1
 
        MOV		pc,r14
                
	.end	        