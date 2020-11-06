    .global L_shl
    .global SpectLinesQuant
    .global Quant_Line2Distor
    .global QuantScfBandsSpectr
    
    

  .text
  .align 0

SpectLinesQuant:
        STMFD		r13!,{r0-r11,r14} 
        AND		r1,r0,#3 
        LDR		r2,Var7 
        MOV		r6,r0 
        ADD		r1,r2,r1,LSL #3 
        MOV		r0,#4 
        ADD		r0,r0,r6,ASR #2 
        ADD		r7,r0,#0x10 
        LDRSH		r8,[r1,#0] 
        LDRSH		r9,[r1,#2] 
        LDRSH		r10,[r1,#4] 
        LDRSH		r11,[r1,#6] 
        LDR		r1,Var14 
        CMP		r7,#0 
        LDR		r0,Var13 
        SUB		r13,r13,#4 
        MOV		r5,#0 
        BLT		Branch0 @ 0x1a0
        B		Branch1 @ 0xec
Branch10:
        LDR		r1,[r13,#0xc] 
        MOV		r0,#0 
        LDR		r4,[r1,r5,LSL #2] 
        CMP		r4,#0 
        BEQ		Branch9 @ 0xdc
        MOVGE		r1,r4 
        RSBLT		r1,r4,#0 
        MOV		r2,r1,ASR r7 
        CMP		r2,r8 
        BLE		Branch9 @ 0xdc
        CMP		r2,r9 
        BGE		Branch4 @ 0x8c
        CMP		r4,#0 
        MVNLE		r0,#0 
        MOVGT		r0,#1 
        B		Branch9 @ 0xdc
Branch4:
        CMP		r2,r10 
        BGE		Branch6 @ 0xa4
        CMP		r4,#0 
        MVNLE		r0,#1 
        MOVGT		r0,#2 
        B		Branch9 @ 0xdc
Branch6:
        CMP		r2,r11 
        BGE		Branch8 @ 0xbc
        CMP		r4,#0 
        MVNLE		r0,#2 
        MOVGT		r0,#3 
        B		Branch9 @ 0xdc
Branch8:
        LDR		r0,Var13 
        LDR		r3,[r0,#0] 
        LDR		r0,Var14 
        LDR		r2,[r0,#0] 
        MOV		r0,r6 
        BL		SingLineQuant0 
        CMP		r4,#0 
        RSBLT		r0,r0,#0 
Branch9:
        LDR		r1,[r13,#0x10] 
        ADD		r1,r1,r5,LSL #1 
        STRH		r0,[r1,#0] 
        ADD		r5,r5,#1 
Branch1:
        LDR		r0,[r13,#8] 
        CMP		r5,r0 
        BLT		Branch10 @ 0x4c
Branch20:
        ADD		r13,r13,#0x14 
        LDMFD		r13!,{r4-r11,pc} 
Branch19:
        LDR		r1,[r13,#0xc] 
        MOV		r0,#0 
        LDR		r4,[r1,r5,LSL #2] 
        CMP		r4,#0 
        BEQ		Branch18 @ 0x190
        MOVGE		r1,r4 
        RSBLT		r1,r4,#0 
        MOV		r2,r1,LSL r7 
        CMP		r2,r8 
        BLE		Branch18 @ 0x190
        CMP		r2,r9 
        BGE		Branch13 @ 0x140
        CMP		r4,#0 
        MVNLE		r0,#0 
        MOVGT		r0,#1 
        B		Branch18 @ 0x190
Branch13:
        CMP		r2,r10 
        BGE		Branch15 @ 0x158
        CMP		r4,#0 
        MVNLE		r0,#1 
        MOVGT		r0,#2 
        B		Branch18 @ 0x190
Branch15:
        CMP		r2,r11 
        BGE		Branch17 @ 0x170
        CMP		r4,#0 
        MVNLE		r0,#2 
        MOVGT		r0,#3 
        B		Branch18 @ 0x190
Branch17:
        LDR		r0,Var13 
        LDR		r3,[r0,#0] 
        LDR		r0,Var14 
        LDR		r2,[r0,#0] 
        MOV		r0,r6 
        BL		SingLineQuant0 
        CMP		r4,#0 
        RSBLT		r0,r0,#0 
Branch18:
        LDR		r1,[r13,#0x10] 
        ADD		r1,r1,r5,LSL #1 
        STRH		r0,[r1,#0] 
        ADD		r5,r5,#1 
Branch0:
        LDR		r0,[r13,#8] 
        CMP		r5,r0 
        BLT		Branch19 @ 0x100
        B		Branch20 @ 0xf8
QuantScfBandsSpectr:
        STMFD		r13!,{r0-r11,r14} 
        SUB		r13,r13,#4 
        ADD		r11,r13,#0x40 
        LDMIA		r11,{r6,r11} 
        ADD		r10,r13,#0x38 
        LDMIA		r10,{r9,r10} 
        MOV		r8,r3 
        MOV		r7,r1 
        MOV		r5,#0 
        B		Branch21 @ 0x254
Branch26:
        MOV		r4,#0 
        B		Branch22 @ 0x244
Branch25:
        ADD		r2,r5,r4 
        ADD		r0,r6,r2,LSL #1 
        LDRSH		r0,[r0,#0] 
Branch24:
        ADD		r4,r4,#1 
        CMP		r4,r7 
        BGE		Branch23 @ 0x20c
        ADD		r1,r5,r4 
        ADD		r1,r6,r1,LSL #1 
        LDRSH		r1,[r1,#0] 
        CMP		r1,r0 
        BEQ		Branch24 @ 0x1ec
Branch23:
        ADD		r1,r5,r4 
        ADD		r1,r8,r1,LSL #1 
        LDRH		r1,[r1,#0] 
        ADD		r2,r8,r2,LSL #1 
        LDRSH		r2,[r2,#0] 
        SUB		r0,r10,r0 
        MOV		r0,r0,LSL #16 
        SUB		r1,r1,r2 
        MOV		r1,r1,LSL #16 
        ADD		r3,r11,r2,LSL #1 
        ADD		r2,r9,r2,LSL #2 
        MOV		r1,r1,ASR #16 
        MOV		r0,r0,ASR #16 
        BL		SpectLinesQuant @ 0x0
Branch22:
        CMP		r4,r7 
        BLT		Branch25 @ 0x1e0
        LDR		r0,[r13,#0xc] 
        ADD		r5,r5,r0 
Branch21:
        LDR		r0,[r13,#4] 
        CMP		r5,r0 
        BLT		Branch26 @ 0x1d8
        ADD		r13,r13,#0x14 
        LDMFD		r13!,{r4-r11,pc} 
Quant_Line2Distor:
        STMFD		r13!,{r4-r11,r14} 
        MOV		r7,r2 
        AND		r12,r2,#3 
        MOV		r2,#4 
        ADD		r2,r2,r7,ASR #2 
        MOV		r3,#1 
        ADD		r6,r3,r2,LSL #1 
        LDR		r3,Var7 
        LDR		r14,Var8 
        ADD		r3,r3,r12,LSL #3 
        ADD		r12,r12,r12,LSL #1 
        ADD		r5,r14,r12,LSL #1 
        LDRSH		r12,[r3,#0] 
        SUB		r13,r13,#0x1c 
        ADD		r9,r2,#0x10 
        STR		r12,[r13,#0x14] 
        LDRSH		r12,[r3,#2] 
        LDR		r2,Var13 
        CMP		r6,#0 
        STR		r12,[r13,#0x10] 
        LDRSH		r12,[r3,#4] 
        MOV		r8,#0 
        STR		r12,[r13,#0xc] 
        LDRSH		r3,[r3,#6] 
        STR		r3,[r13,#8] 
        LDR		r3,Var14 
        BGE		Branch28 @ 0x39c
        CMP		r9,#0 
        BLT		Branch28 @ 0x39c
        RSB		r4,r6,#0 
        MOV		r10,r1 
        MOV		r11,r0 
Branch36:
        LDR		r6,[r11],#4 
        CMP		r6,#0 
        BEQ		Branch29 @ 0x390
        RSBLT		r6,r6,#0 
        LDR		r1,[r13,#0x14] 
        MOV		r0,r6,ASR r9 
        CMP		r0,r1 
        BGE		Branch30 @ 0x314
Branch35:
        MUL		r1,r0,r0 
        MOV		r0,r1,ASR r4 
        B		Branch31 @ 0x38c
Branch30:
        LDR		r1,[r13,#0x10] 
        CMP		r0,r1 
        LDRLTSH		r1,[r5,#0] 
        BLT		Branch33 @ 0x344
        LDR		r1,[r13,#0xc] 
        CMP		r0,r1 
        LDRLTSH		r1,[r5,#2] 
        BLT		Branch33 @ 0x344
        LDR		r1,[r13,#8] 
        CMP		r0,r1 
        BGE		Branch34 @ 0x34c
        LDRSH		r1,[r5,#4] 
Branch33:
        SUB		r0,r0,r1 
        B		Branch35 @ 0x308
Branch34:
        LDR		r2,Var13 
        LDR		r0,Var14 
        LDR		r3,[r2,#0] 
        LDR		r2,[r0,#0] 
        MOV		r0,r7 
        MOV		r1,r6 
        BL		SingLineQuant0 
        STR		r0,[r13,#4] 
        MOV		r0,r7 
        MOV		r2,r13 
        ADD		r1,r13,#4 
        BL		iQuantL1_s 
        LDR		r0,[r13,#0] 
        SUB		r0,r6,r0 
        SMULL		r2,r1,r0,r0 
        MOV		r0,r1,LSL #1 
Branch31:
        ADD		r8,r8,r0 
Branch29:
        SUBS		r10,r10,#1 
        BNE		Branch36 @ 0x2e8
        B		Branch37 @ 0x480
Branch28:
        MOV		r10,r1 
        RSB		r1,r9,#0 
        STR		r1,[r13,#0x18] 
        MOV		r11,r0 
Branch45:
        LDR		r4,[r11],#4 
        CMP		r4,#0 
        BEQ		Branch38 @ 0x478
        RSBLT		r4,r4,#0 
        CMP		r9,#0 
        MOVGE		r0,r4,ASR r9 
        BGE		Branch39 @ 0x3dc
        LDR		r1,[r13,#0x18] 
        MOV		r0,r4,LSL r1 
        TEQ		r4,r0,ASR r1 
        MVNNE		r0,#0x80000000 
        EORNE		r0,r0,r4,ASR #31 
Branch39:
        LDR		r1,[r13,#0x14] 
        CMP		r0,r1 
        MULLT		r1,r0,r0 
        MOVLT		r0,r1 
        BLT		Branch40 @ 0x428
        LDR		r1,[r13,#0x10] 
        CMP		r0,r1 
        LDRLTSH		r1,[r5,#0] 
        BLT		Branch42 @ 0x420
        LDR		r1,[r13,#0xc] 
        CMP		r0,r1 
        LDRLTSH		r1,[r5,#2] 
        BLT		Branch42 @ 0x420
        LDR		r1,[r13,#8] 
        CMP		r0,r1 
        BGE		Branch43 @ 0x434
        LDRSH		r1,[r5,#4] 
Branch42:
        SUB		r1,r0,r1 
        MUL		r0,r1,r1 
Branch40:
        MOV		r1,r6 
        BL		L_shl @ 0x0
        B		Branch44 @ 0x474
Branch43:
        LDR		r2,Var13 
        LDR		r0,Var14 
        LDR		r3,[r2,#0] 
        LDR		r2,[r0,#0] 
        MOV		r0,r7 
        MOV		r1,r4 
        BL		SingLineQuant0 
        STR		r0,[r13,#4] 
        MOV		r0,r7 
        MOV		r2,r13 
        ADD		r1,r13,#4 
        BL		iQuantL1_s 
        LDR		r0,[r13,#0] 
        SUB		r0,r4,r0 
        SMULL		r2,r1,r0,r0 
        MOV		r0,r1,LSL #1 
Branch44:
        ADD		r8,r8,r0 
Branch38:
        SUBS		r10,r10,#1 
        BNE		Branch45 @ 0x3ac
Branch37:
        MOV		r0,r8 
        ADD		r13,r13,#0x1c 
        LDMFD		r13!,{r4-r11,pc} 
Var7:        .word		quantBorders @0 
Var14:        .word		mTab_3_4_ptr @0 
Var13:        .word		pow2tominusNover16_ptr @0 
Var8:        .word		quantRecon @0 
L_shl:
        CMP		r1,#0 
        RSBLT		r1,r1,#0 
        MOVLT		r0,r0,ASR r1 
        MOVLT		pc,r14 
        MOV		r2,r0,LSL r1 
        TEQ		r0,r2,ASR r1 
        MVNNE		r1,#0x80000000 
        EORNE		r2,r1,r0,ASR #31 
        MOV		r0,r2 
        MOV		pc,r14 

    .end

