    .global CalcScaleFactAllChan
    .global Scf_CountDiffBit
    .global EstFactorOneChann
    .global EstimFactAllChan
    .global BetterScalFact
    .global Scf_DiffReduction
    .global Count_PeDiff
    .global Count_SinglePe
    .global Scf_SearchSingleBands
    .global saturate
    .global CalcScaleFactOneChann
    
    
    
    

  .text
  .align 0

EstFactorOneChann:
        STMFD		r13!,{r0-r11,r14}
 
        SUB		r13,r13,#4
 
        MOV		r11,#0
 
        MOV		r9,#0
 
        MOV		r8,r1
 
        MOV		r5,r3
 
        B		Branch0 @ 0x180
Branch7:
        MOV		r7,#0
 
        B		Branch1 @ 0x16c
Branch6:
        LDR		r0,[r5,#0x88]
 
        ADD		r4,r9,r7
 
        LDR		r1,[r5,#0x8c]
 
        LDR		r0,[r0,r4,LSL #2]
 
        LDR		r1,[r1,r4,LSL #2]
 
        CMP		r0,r1
 
        ADDLE		r0,r8,r4,LSL #1
 
        STRLEH		r11,[r0,#0]
 
        BLE		Branch2 @ 0x168
        ADD		r0,r5,r4,LSL #1
 
        LDRSH		r1,[r0,#0xe]
 
        LDRSH		r0,[r0,#0xc]
 
        LDR		r2,Var0 
        LDR		r14,Var1 
        SUB		r12,r1,r0
 
        MOV		r1,r12,ASR #2
 
        ADD		r1,r2,r1,LSL #2
 
        LDR		r10,[r1,#-4]
 
        LDR		r1,[r5,#0x90]
 
        MOV		r6,#0
 
        ADD		r3,r1,r0,LSL #2
 
Branch4:
        LDR		r0,[r3,#0]
 
        CMP		r0,#0
 
        RSBLT		r0,r0,#0
 
        CMP		r0,#0
 
        MOVEQ		r0,r11
 
        BEQ		Branch3 @ 0xf0
        ORR		r1,r0,r0,LSR #1
 
        ORR		r1,r1,r1,LSR #2
 
        ORR		r1,r1,r1,LSR #4
 
        ORR		r1,r1,r1,LSR #8
 
        BIC		r1,r1,r1,LSR #16
 
        RSB		r1,r1,r1,LSL #9
 
        RSB		r1,r1,r1,LSL #11
 
        LDR		r2,Var2 
        RSB		r1,r1,r1,LSL #14
 
        LDRB		r1,[r2,r1,LSR #26]
 
        SUB		r1,r1,#0x19
 
        MOV		r2,r1,ASR #1
 
        MOV		r1,r2,LSL #1
 
        CMP		r1,#0
 
        RSBLT		r1,r1,#0
 
        MOVLT		r0,r0,ASR r1
 
        MOVGE		r0,r0,LSL r1
 
        ADD		r0,r14,r0,LSL #2
 
        ADD		r2,r2,#8
 
        CMP		r2,#0
 
        LDR		r0,[r0,#-0x80]
 
        RSBLT		r1,r2,#0
 
        MOVLT		r0,r0,LSL r1
 
        MOVGE		r0,r0,ASR r2
 
Branch3:
        ADD		r6,r0,r6
 
        ADD		r3,r3,#4
 
        SUBS		r12,r12,#1
 
        BNE		Branch4 @ 0x78
        MOV		r0,r6
 
        BL		iLog4
 
        LDR		r1,[r13,#4]
 
        ADD		r1,r1,r4,LSL #1
 
        STRH		r0,[r1,#0]
 
        LDR		r0,[r5,#0x88]
 
        LDR		r0,[r0,r4,LSL #2]
 
        BL		iLog4
 
        LDR		r1,[r13,#0xc]
 
        ADD		r1,r1,r4,LSL #1
 
        STRH		r0,[r1,#0]
 
        LDR		r0,[r5,#0x88]
 
        MOV		r1,#0x20
 
        LDR		r0,[r0,r4,LSL #2]
 
        BL		rsqrt
 
        SMULL		r2,r1,r0,r10
 
        MOV		r0,r1,LSL #1
 
        MOV		r1,#0x20
 
        BL		rsqrt
 
        MOVS		r1,r0,ASR #10
 
        MVNEQ		r0,#0x8000
 
        BEQ		Branch5 @ 0x160
        MOV		r0,r6
 
        BL		__divsi3
 
Branch5:
        ADD		r1,r8,r4,LSL #1
 
        STRH		r0,[r1,#0]
 
Branch2:
        ADD		r7,r7,#1
 
Branch1:
        LDRSH		r0,[r5,#0xa]
 
        CMP		r0,r7
 
        BGT		Branch6 @ 0x24
        LDRSH		r0,[r5,#6]
 
        ADD		r9,r0,r9
 
Branch0:
        LDRSH		r0,[r5,#4]
 
        CMP		r0,r9
 
        BGT		Branch7 @ 0x1c
        ADD		r13,r13,#0x14
 
        LDMFD		r13!,{r4-r11,pc}
 
BetterScalFact:
        STMFD		r13!,{r3-r11,r14}
 
        MOV		r8,r1
 
        MOV		r1,r2,ASR #2
 
        ADDS		r10,r2,r1
 
        MOV		r7,r0
 
        MOV		r0,#0x80000000
 
        MOV		r4,r3
 
        EORVS		r10,r0,r10,ASR #31
 
        MOV		r6,r2
 
        MOV		r2,r4
 
        MOV		r0,r7
 
        MOV		r5,r4
 
        MOV		r1,r8
 
        LDR		r11,[r13,#0x30]
 
        BL		Quant_Line2Distor
 
        STRH		r4,[r11,#0]
 
        CMP		r0,#0
 
        BEQ		Branch17 @ 0x30c
        CMP		r0,r10
 
        BLE		Branch9 @ 0x2a0
        MOV		r6,r0
 
        MOV		r9,#0
 
        STR		r4,[r13,#0]
 
        B		Branch10 @ 0x220
Branch12:
        ADD		r0,r4,#1
 
        MOV		r4,r0,LSL #16
 
        MOV		r4,r4,ASR #16
 
        MOV		r2,r4
 
        MOV		r0,r7
 
        MOV		r1,r8
 
        BL		Quant_Line2Distor
 
        CMP		r0,r6
 
        MOVLT		r5,r4
 
        MOVLT		r6,r0
 
        ADD		r9,r9,#1
 
Branch10:
        CMP		r0,r10
 
        BLE		Branch11 @ 0x230
        CMP		r9,#3
 
        BLT		Branch12 @ 0x1f4
Branch11:
        LDR		r0,[r13,#0]
 
        MOV		r4,#0
 
        MOV		r9,r0,LSL #16
 
        MOV		r9,r9,ASR #16
 
        MOV		r0,r6
 
        B		Branch13 @ 0x278
Branch16:
        SUB		r0,r9,#1
 
        MOV		r9,r0,LSL #16
 
        MOV		r9,r9,ASR #16
 
        MOV		r2,r9
 
        MOV		r0,r7
 
        MOV		r1,r8
 
        BL		Quant_Line2Distor
 
        CMP		r0,r6
 
        MOVLT		r5,r9
 
        MOVLT		r6,r0
 
        STRH		r9,[r11,#0]
 
        ADD		r4,r4,#1
 
Branch13:
        CMP		r0,r10
 
        BLE		Branch15 @ 0x294
        CMP		r4,#1
 
        BGE		Branch15 @ 0x294
        LDR		r0,[r13,#0x28]
 
        CMP		r9,r0
 
        BGT		Branch16 @ 0x248
Branch15:
        LDR		r0,[r13,#0x2c]
 
        STR		r6,[r0,#0]
 
        B		Branch17 @ 0x30c
Branch9:
        LDR		r3,Var4 
        MOV		r9,r0
 
        SMULL		r2,r1,r3,r6
 
        MOV		r10,r1,LSL #1
 
        CMP		r0,r10
 
        MOVLT		r10,r0
 
        MOV		r6,#0
 
Branch18:
        ADD		r0,r4,#1
 
        MOV		r4,r0,LSL #16
 
        MOV		r4,r4,ASR #16
 
        MOV		r2,r4
 
        MOV		r0,r7
 
        MOV		r1,r8
 
        BL		Quant_Line2Distor
 
        LDR		r3,Var4 
        ADD		r6,r6,#1
 
        SMULL		r2,r1,r3,r0
 
        MOV		r1,r1,LSL #1
 
        CMP		r1,r10
 
        ADDLT		r1,r5,#1
 
        STRLTH		r1,[r11,#0]
 
        MOVLT		r5,r4
 
        MOVLT		r9,r0
 
        CMP		r6,#3
 
        BLT		Branch18 @ 0x2bc
        LDR		r0,[r13,#0x2c]
 
        STR		r9,[r0,#0]
 
Branch17:
        MOV		r0,r5,LSL #16
 
        MOV		r0,r0,ASR #16
 
        LDMFD		r13!,{r3-r11,pc}
 
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
 
        LDR		r7,Var8 
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
 
Scf_SearchSingleBands:
        STMFD		r13!,{r0-r11,r14}
 
        MOV		r9,r1
 
        ADD		r1,r0,#0x2f8
 
        SUB		r13,r13,#0x3c
 
        STR		r1,[r13,#0xc]
 
        ADD		r1,r0,#0x370
 
        MOV		r11,r0
 
        ADD		r0,r0,#0x3e8
 
        STMIB		r13,{r0,r1}
 
        MOV		r0,#0
 
        LDR		r8,Var13 
        STR		r0,[r13,#0x1c]
 
        STR		r0,[r13,#0x18]
 
Branch32:
        LDRSH		r1,[r11,#4]
 
        CMP		r1,r0
 
        LDRGT		r1,[r13,#0xc]
 
        ADDGT		r1,r1,r0,LSL #1
 
        STRGTH		r8,[r1,#0]
 
        LDRGT		r1,[r13,#8]
 
        ADDGT		r1,r1,r0,LSL #1
 
        STRGTH		r8,[r1,#0]
 
        LDRGT		r1,[r13,#4]
 
        ADDGT		r1,r1,r0,LSL #1
 
        STRGTH		r8,[r1,#0]
 
        ADDGT		r0,r0,#1
 
        BGT		Branch32 @ 0x4c4
        MVN		r0,#0
 
        MVN		r4,#0
 
        MVN		r5,#0
 
        MOV		r6,#0
 
        MOV		r7,#0
 
Branch56:
        ADD		r1,r5,#1
 
        MOV		r5,r1,LSL #16
 
        MOV		r5,r5,ASR #16
 
        LDRSH		r1,[r11,#4]
 
        B		Branch33 @ 0x52c
Branch35:
        ADD		r2,r5,#1
 
        MOV		r5,r2,LSL #16
 
        MOV		r5,r5,ASR #16
 
Branch33:
        CMP		r1,r5
 
        BLE		Branch34 @ 0x544
        ADD		r2,r9,r5,LSL #1
 
        LDRH		r2,[r2,#0]
 
        CMP		r2,#0x8000
 
        BEQ		Branch35 @ 0x520
Branch34:
        CMP		r0,#0
 
        CMPGE		r4,#0
 
        CMPGE		r1,r5
 
        BLE		Branch36 @ 0x584
        ADD		r1,r9,r4,LSL #1
 
        LDRH		r1,[r1,#0]
 
        ADD		r6,r9,r0,LSL #1
 
        ADD		r7,r9,r5,LSL #1
 
        STRH		r1,[r13,#0x24]
 
        LDRSH		r1,[r6,#0]
 
        LDRSH		r2,[r7,#0]
 
        CMP		r1,r2
 
        MOVGE		r1,r2
 
        MOV		r8,r1,LSL #16
 
        MOV		r8,r8,ASR #16
 
        B		Branch43 @ 0x5ec
Branch36:
        CMN		r0,#1
 
        BNE		Branch38 @ 0x5b4
        CMP		r4,#0
 
        BLT		Branch49 @ 0x890
        CMP		r1,r5
 
        ADDGT		r1,r9,r4,LSL #1
 
        LDRGTH		r1,[r1,#0]
 
        ADDGT		r7,r9,r5,LSL #1
 
        ADDGT		r6,r13,#0x24
 
        STRGTH		r1,[r13,#0x24]
 
        LDRGTSH		r8,[r7,#0]
 
        B		Branch43 @ 0x5ec
Branch38:
        CMP		r0,#0
 
        BLT		Branch41 @ 0x5e4
        CMP		r4,#0
 
        BLT		Branch49 @ 0x890
        CMP		r1,r5
 
        ADDEQ		r1,r9,r4,LSL #1
 
        LDREQH		r1,[r1,#0]
 
        ADDEQ		r6,r9,r0,LSL #1
 
        ADDEQ		r7,r13,#0x24
 
        STREQH		r1,[r13,#0x24]
 
        LDREQSH		r8,[r6,#0]
 
        B		Branch43 @ 0x5ec
Branch41:
        CMP		r4,#0
 
        BLT		Branch49 @ 0x890
Branch43:
        LDR		r1,[r13,#0x44]
 
        ADD		r1,r1,r4,LSL #1
 
        LDRSH		r1,[r1,#0]
 
        CMP		r1,r8
 
        MOVLT		r1,r8
 
        MOV		r8,r1,LSL #16
 
        MOV		r8,r8,ASR #16
 
        CMP		r0,#0
 
        BGE		Branch45 @ 0x61c
        LDRSH		r0,[r11,#4]
 
        CMP		r0,r5
 
        BLE		Branch49 @ 0x890
Branch45:
        LDRSH		r0,[r13,#0x24]
 
        CMP		r0,r8
 
        BLE		Branch49 @ 0x890
        LDRH		r0,[r6,#0]
 
        LDR		r1,[r13,#0xc]
 
        ADD		r1,r1,r4,LSL #1
 
        STR		r1,[r13,#0x38]
 
        LDRH		r1,[r1,#0]
 
        CMP		r0,r1
 
        LDREQH		r0,[r7,#0]
 
        LDREQ		r1,[r13,#8]
 
        ADDEQ		r1,r1,r4,LSL #1
 
        LDREQH		r1,[r1,#0]
 
        CMPEQ		r0,r1
 
        BNE		Branch48 @ 0x670
        LDR		r0,[r13,#4]
 
        ADD		r0,r0,r4,LSL #1
 
        LDRSH		r0,[r0,#0]
 
        LDR		r1,[r13,#0x18]
 
        CMP		r0,r1
 
        BLE		Branch49 @ 0x890
Branch48:
        MOV		r0,#0
 
        STR		r0,[r13,#0x1c]
 
        LDR		r0,[r13,#0x70]
 
        ADD		r10,r0,r4,LSL #1
 
        LDRH		r0,[r10,#0]
 
        CMP		r0,#0x8000
 
        BNE		Branch50 @ 0x6cc
        LDR		r0,[r13,#0x74]
 
        ADD		r0,r0,r4,LSL #1
 
        LDRH		r0,[r0,#0]
 
        LDR		r1,[r13,#0x78]
 
        ADD		r1,r1,r4,LSL #1
 
        LDRH		r1,[r1,#0]
 
        SUB		r0,r0,r1
 
        SUB		r0,r0,#0x15
 
        STRH		r0,[r10,#0]
 
        LDRSH		r0,[r10,#0]
 
        CMP		r0,#0
 
        ADDLT		r0,r0,#3
 
        STRLTH		r0,[r10,#0]
 
        LDRSH		r0,[r10,#0]
 
        MOV		r0,r0,ASR #2
 
        STRH		r0,[r10,#0]
 
Branch50:
        LDR		r0,[r13,#0x7c]
 
        ADD		r0,r0,r4,LSL #1
 
        STR		r0,[r13,#0x34]
 
        LDRSH		r2,[r0,#0]
 
        LDRSH		r1,[r10,#0]
 
        LDRSH		r0,[r13,#0x24]
 
        BL		Count_SinglePe @ 0xfd0
        LDRSH		r1,[r6,#0]
 
        LDRSH		r2,[r13,#0x24]
 
        LDR		r3,Var8 
        SUB		r1,r1,r2
 
        ADD		r1,r3,r1,LSL #1
 
        LDRH		r1,[r1,#0x78]
 
        LDRSH		r2,[r13,#0x24]
 
        ADD		r0,r0,r1
 
        LDRSH		r1,[r7,#0]
 
        SUB		r1,r2,r1
 
        ADD		r1,r3,r1,LSL #1
 
        LDRH		r1,[r1,#0x78]
 
        ADD		r2,r9,r4,LSL #1
 
        STR		r2,[r13,#0x30]
 
        ADD		r1,r0,r1
 
        LDR		r0,[r13,#0x18]
 
        STR		r1,[r13,#0x20]
 
        STR		r0,[r13,#0x14]
 
        MOV		r0,#1
 
        STR		r0,[r13,#0]
 
        LDR		r0,[r13,#0x80]
 
        ADD		r2,r13,#0x28
 
        ADD		r1,r0,r4,LSL #1
 
        ADD		r0,r11,r4,LSL #1
 
        STMIA		r2,{r0,r1}
 
Branch54:
        LDRH		r0,[r13,#0x24]
 
        SUB		r0,r0,#1
 
        STRH		r0,[r13,#0x24]
 
        LDR		r1,[r13,#0x2c]
 
        LDRSH		r0,[r1,#0]
 
        LDRSH		r1,[r13,#0x24]
 
        CMP		r0,r1
 
        BLE		Branch52 @ 0x850
        LDR		r0,[r13,#0x34]
 
        LDRSH		r1,[r10,#0]
 
        LDRSH		r2,[r0,#0]
 
        LDRSH		r0,[r13,#0x24]
 
        BL		Count_SinglePe @ 0xfd0
        LDRSH		r1,[r6,#0]
 
        LDRSH		r2,[r13,#0x24]
 
        LDR		r3,Var8 
        SUB		r1,r1,r2
 
        ADD		r1,r3,r1,LSL #1
 
        LDRH		r1,[r1,#0x78]
 
        LDRSH		r2,[r13,#0x24]
 
        ADD		r0,r0,r1
 
        LDRSH		r1,[r7,#0]
 
        SUB		r1,r2,r1
 
        ADD		r1,r3,r1,LSL #1
 
        LDRH		r1,[r1,#0x78]
 
        ADD		r0,r0,r1
 
        LDR		r1,[r13,#0x18]
 
        ADD		r0,r1,r0
 
        LDR		r1,[r13,#0x20]
 
        SUB		r0,r0,r1
 
        MOV		r0,r0,LSL #16
 
        MOV		r0,r0,ASR #16
 
        STR		r0,[r13,#0x10]
 
        CMP		r0,#0xa
 
        MOVGE		r0,#0
 
        STRGE		r0,[r13,#0]
 
        BGE		Branch52 @ 0x850
        LDR		r0,[r13,#0x28]
 
        LDR		r2,[r11,#0x90]
 
        LDRH		r1,[r0,#0xe]
 
        LDRSH		r0,[r0,#0xc]
 
        SUB		r1,r1,r0
 
        MOV		r1,r1,LSL #16
 
        ADD		r0,r2,r0,LSL #2
 
        LDRSH		r2,[r13,#0x24]
 
        MOV		r1,r1,ASR #16
 
        BL		Quant_Line2Distor
 
        LDR		r1,[r13,#0x48]
 
        LDR		r1,[r1,r4,LSL #2]
 
        CMP		r1,r0
 
        BLE		Branch53 @ 0x83c
        LDRH		r1,[r13,#0x24]
 
        LDR		r2,[r13,#0x30]
 
        STRH		r1,[r2,#0]
 
        LDR		r1,[r13,#0x48]
 
        STR		r0,[r1,r4,LSL #2]
 
        LDR		r0,[r13,#0x10]
 
        STR		r0,[r13,#0x14]
 
        MOV		r0,#1
 
        STR		r0,[r13,#0x1c]
 
Branch53:
        LDR		r0,[r13,#0]
 
        CMP		r0,#0
 
        LDRNEH		r0,[r13,#0x24]
 
        LDRNE		r1,[r13,#0x2c]
 
        STRNEH		r0,[r1,#0]
 
Branch52:
        LDRSH		r0,[r13,#0x24]
 
        CMP		r0,r8
 
        BGT		Branch54 @ 0x74c
        LDR		r0,[r13,#0x14]
 
        STR		r0,[r13,#0x18]
 
        LDRH		r0,[r6,#0]
 
        LDR		r1,[r13,#0x38]
 
        STRH		r0,[r1,#0]
 
        LDRH		r0,[r7,#0]
 
        LDR		r1,[r13,#8]
 
        ADD		r1,r1,r4,LSL #1
 
        STRH		r0,[r1,#0]
 
        LDR		r1,[r13,#4]
 
        LDR		r0,[r13,#0x18]
 
        ADD		r1,r1,r4,LSL #1
 
        STRH		r0,[r1,#0]
 
Branch49:
        LDR		r0,[r13,#0x1c]
 
        CMP		r0,#0
 
        LDRNE		r0,[r13,#0x84]
 
        CMPNE		r0,#0
 
        MOVEQ		r0,r4
 
        MOVEQ		r4,r5
 
        BEQ		Branch55 @ 0x8cc
        MVN		r0,#0
 
        MOV		r1,#0
 
        STR		r1,[r13,#0x1c]
 
        MOV		r8,r0,LSR #17
 
        MVN		r4,#0
 
        MVN		r5,#0
 
        MOV		r6,#0
 
        MOV		r7,#0
 
Branch55:
        LDRSH		r1,[r11,#4]
 
        CMP		r1,r5
 
        BGT		Branch56 @ 0x50c
        ADD		r13,r13,#0x4c
 
        LDMFD		r13!,{r4-r11,pc}
 
Var0:        .word		invSBF            @0x00019ee4 
 
Var1:        .word		formfac_sqrttable @0x0001d33c

 
Var2:        .word		quan_hash_table   @0x0001FB70

 
Var4:        .word		0x66666666
 
Var8:        .word		huff_ltabscf      @0x0001aca4
 
Var13:       .word		0x00007fff

 quan_hash_table:
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
 
Scf_DiffReduction:
        STMFD		r13!,{r0-r11,r14}
 
        SUB		r13,r13,#0x2c
 
        LDR		r2,[r13,#0x2c]
 
        MOV		r6,r1
 
        ADD		r1,r2,#0x190
 
        MOV		r0,#0
 
        STR		r0,[r13,#0x18]
 
        STR		r1,[r13,#0x10]
 
        LDRSH		r8,[r2,#4]
 
        ADD		r9,r2,#0x2f8
 
        LDR		r2,Var13 
        SUB		r7,r0,#0x8000
 
        SUB		r1,r8,#1
 
        STR		r2,[r13,#0x1c]
 
Branch58:
        ADD		r0,r6,r1,LSL #1
 
        LDRH		r2,[r0,#0]
 
        CMP		r2,#0x8000
 
        BEQ		Branch57 @ 0x964
        LDRSH		r0,[r0,#0]
 
        LDR		r2,[r13,#0x1c]
 
        CMP		r0,r2
 
        MOVLE		r2,r0
 
        LDRGT		r2,[r13,#0x1c]
 
        CMP		r0,r7
 
        MOVLT		r0,r7
 
        MOV		r7,r0
 
        STR		r2,[r13,#0x1c]
 
Branch57:
        SUBS		r1,r1,#1
 
        BPL		Branch58 @ 0x930
        CMN		r7,#0x8000
 
        BEQ		Branch59 @ 0xbe4
        MOV		r2,r8,LSL #16
 
        MOV		r2,r2,ASR #16
 
        STR		r2,[r13,#0x28]
 
Branch83:
        SUB		r7,r7,#1
 
        MOV		r0,#0
 
Branch60:
        CMP		r0,r8
 
        ADDLT		r1,r6,r0,LSL #1
 
        LDRLTH		r1,[r1,#0]
 
        ADDLT		r2,r9,r0,LSL #1
 
        ADDLT		r0,r0,#1
 
        STRLTH		r1,[r2,#0]
 
        BLT		Branch60 @ 0x988
        MOV		r2,r7,LSL #16
 
        MOV		r2,r2,ASR #16
 
        STR		r2,[r13,#0x24]
 
        MOV		r5,#0
 
        B		Branch82 @ 0x9bc
Branch63:
        ADD		r5,r5,#1
 
Branch82:
        CMP		r5,r8
 
        BGE		Branch62 @ 0x9dc
        ADD		r0,r6,r5,LSL #1
 
        LDRH		r1,[r0,#0]
 
        CMP		r1,#0x8000
 
        LDRNESH		r0,[r0,#0]
 
        CMPNE		r0,r7
 
        BLE		Branch63 @ 0x9b8
Branch62:
        STR		r5,[r13,#0x20]
 
Branch66:
        ADD		r5,r5,#1
 
        CMP		r5,r8
 
        BGE		Branch64 @ 0xa08
        ADD		r0,r6,r5,LSL #1
 
        LDRH		r1,[r0,#0]
 
        CMP		r1,#0x8000
 
        BEQ		Branch66 @ 0x9e0
        LDRSH		r0,[r0,#0]
 
        CMP		r0,r7
 
        BGT		Branch66 @ 0x9e0
Branch64:
        LDR		r0,[r13,#0x20]
 
        CMP		r0,r8
 
        BGE		Branch79 @ 0xbd0
        MOV		r1,#1
 
        LDR		r0,[r13,#0x20]
 
        B		Branch68 @ 0xa44
Branch70:
        ADD		r2,r6,r0,LSL #1
 
        LDRH		r2,[r2,#0]
 
        CMP		r2,#0x8000
 
        LDRNE		r2,[r13,#0x34]
 
        ADDNE		r2,r2,r0,LSL #1
 
        LDRNESH		r2,[r2,#0]
 
        CMPNE		r2,r7
 
        BGT		Branch79 @ 0xbd0
        ADD		r0,r0,#1
 
Branch68:
        CMP		r0,r5
 
        BLT		Branch70 @ 0xa20
        CMP		r1,#0
 
        BEQ		Branch79 @ 0xbd0
        LDR		r0,[r13,#0x20]
 
        B		Branch72 @ 0xa74
Branch73:
        ADD		r1,r9,r0,LSL #1
 
        LDRH		r2,[r1,#0]
 
        ADD		r0,r0,#1
 
        CMP		r2,#0x8000
 
        LDRNE		r2,[r13,#0x24]
 
        STRNEH		r2,[r1,#0]
 
Branch72:
        CMP		r0,r5
 
        BLT		Branch73 @ 0xa5c
        MOV		r3,r5,LSL #16
 
        MOV		r3,r3,ASR #16
 
        STR		r3,[r13,#0]
 
        LDR		r0,[r13,#0x20]
 
        MOV		r10,r3
 
        MOV		r2,r0,LSL #16
 
        MOV		r2,r2,ASR #16
 
        MOV		r11,r2
 
        MOV		r3,r2
 
        LDR		r2,[r13,#0x28]
 
        MOV		r0,r6
 
        MOV		r1,r9
 
        BL		Scf_CountDiffBit @ 0x318
        MOV		r4,r0
 
        ADD		r1,r13,#0x68
 
        LDMIA		r1,{r0,r1}
 
        STMIA		r13,{r0,r1,r11}
 
        STR		r10,[r13,#0xc]
 
        ADD		r3,r13,#0x60
 
        LDMIA		r3,{r2,r3}
 
        MOV		r1,r9
 
        MOV		r0,r6
 
        BL		Count_PeDiff @ 0x1008
        LDR		r1,[r13,#0x18]
 
        ADD		r1,r1,r4
 
        ADD		r0,r1,r0
 
        STR		r0,[r13,#0x14]
 
        CMP		r0,#0xa
 
        BGE		Branch79 @ 0xbd0
        MOV		r10,#0
 
        MOV		r11,#0
 
        LDR		r4,[r13,#0x20]
 
        B		Branch75 @ 0xb80
Branch78:
        ADD		r0,r9,r4,LSL #1
 
        LDRH		r0,[r0,#0]
 
        CMP		r0,#0x8000
 
        BEQ		Branch76 @ 0xb7c
        LDR		r0,[r13,#0x38]
 
        LDR		r1,[r0,r4,LSL #2]
 
        LDR		r2,[r13,#0x2c]
 
        MOV		r0,#0x80000000
 
        ADDS		r10,r10,r1
 
        EORVS		r10,r0,r10,ASR #31
 
        ADD		r0,r2,r4,LSL #1
 
        LDRH		r1,[r0,#0xe]
 
        LDRSH		r0,[r0,#0xc]
 
        LDR		r2,[r2,#0x90]
 
        SUB		r1,r1,r0
 
        MOV		r1,r1,LSL #16
 
        ADD		r0,r2,r0,LSL #2
 
        LDR		r2,[r13,#0x24]
 
        MOV		r1,r1,ASR #16
 
        BL		Quant_Line2Distor
 
        LDR		r1,[r13,#0x10]
 
        STR		r0,[r1,r4,LSL #2]
 
        LDR		r2,[r13,#0x2c]
 
        LDR		r1,[r2,#0x8c]
 
        LDR		r1,[r1,r4,LSL #2]
 
        CMP		r1,r0
 
        MOVLT		r11,r10,LSL #1
 
        BLT		Branch77 @ 0xb88
        MOV		r1,#0x80000000
 
        ADDS		r11,r11,r0
 
        EORVS		r11,r1,r11,ASR #31
 
Branch76:
        ADD		r4,r4,#1
 
Branch75:
        CMP		r4,r5
 
        BLT		Branch78 @ 0xb00
Branch77:
        CMP		r11,r10
 
        BGE		Branch79 @ 0xbd0
        LDR		r0,[r13,#0x14]
 
        LDR		r4,[r13,#0x20]
 
        STR		r0,[r13,#0x18]
 
        B		Branch80 @ 0xbc8
Branch81:
        ADD		r0,r6,r4,LSL #1
 
        LDRH		r1,[r0,#0]
 
        CMP		r1,#0x8000
 
        LDRNE		r2,[r13,#0x24]
 
        STRNEH		r2,[r0,#0]
 
        LDRNE		r1,[r13,#0x10]
 
        LDRNE		r0,[r1,r4,LSL #2]
 
        LDRNE		r1,[r13,#0x38]
 
        STRNE		r0,[r1,r4,LSL #2]
 
        ADD		r4,r4,#1
 
Branch80:
        CMP		r4,r5
 
        BLT		Branch81 @ 0xba0
Branch79:
        CMP		r5,r8
 
        BLE		Branch82 @ 0x9bc
        LDR		r2,[r13,#0x1c]
 
        CMP		r7,r2
 
        BGT		Branch83 @ 0x980
Branch59:
        ADD		r13,r13,#0x3c
 
        LDMFD		r13!,{r4-r11,pc}
 
CalcScaleFactOneChann:
        STMFD		r13!,{r0-r11,r14}
 
        MOV		r4,r0
 
        SUB		r13,r13,#0xa4
 
        ADD		r0,r0,#0xa0
 
        STR		r0,[r13,#0x98]
 
        ADD		r0,r4,#0x400
 
        MOV		r7,r1
 
        ADD		r1,r4,#0x460
 
        ADD		r0,r0,#0xd8
 
        ADD		r6,r13,#0x90
 
        STMIA		r6,{r0,r1}
 
        MOV		r5,#0
 
        B		Branch84 @ 0xd30
Branch88:
        LDR		r0,[r4,#0x8c]
 
        ADD		r1,r4,r5,LSL #1
 
        LDRSH		r8,[r1,#0xc]
 
        LDRSH		r1,[r1,#0xe]
 
        LDR		r10,[r0,r5,LSL #2]
 
        LDR		r0,[r4,#0x88]
 
        SUB		r11,r1,r8
 
        LDR		r0,[r0,r5,LSL #2]
 
        LDR		r1,[r4,#0x90]
 
        MOV		r6,#0
 
        ADD		r2,r1,r8,LSL #2
 
        MOV		r1,r11
 
        B		Branch85 @ 0xc6c
Branch86:
        LDR		r3,[r2,#0]
 
        ADD		r2,r2,#4
 
        CMP		r3,#0
 
        RSBLT		r3,r3,#0
 
        ORR		r6,r6,r3
 
        SUB		r1,r1,#1
 
Branch85:
        CMP		r1,#0
 
        BNE		Branch86 @ 0xc54
        LDR		r2,Var10 
        ADD		r1,r7,r5,LSL #1
 
        STR		r1,[r13,#0xa0]
 
        STRH		r2,[r1,#0]
 
        LDR		r1,[r13,#0x94]
 
        CMP		r6,#0
 
        ADD		r1,r1,r5,LSL #1
 
        STR		r1,[r13,#0x9c]
 
        STRH		r2,[r1,#0]
 
        CMPGT		r0,r10
 
        BLE		Branch87 @ 0xd2c
        LDR		r0,[r13,#0xd8]
 
        ADD		r0,r0,r5,LSL #1
 
        LDRSH		r9,[r0,#0]
 
        MOV		r0,r10
 
        BL		iLog4
 
        LDR		r1,Var11 
        SUB		r0,r0,r9
 
        MUL		r0,r1,r0
 
        LDR		r1,Var12 
        ADD		r0,r0,r1
 
        MOV		r9,r0,ASR #15
 
        MOV		r0,r6
 
        BL		iLog4
 
        SUB		r0,r0,#0x44
 
        MOV		r1,r0,LSL #16
 
        LDR		r0,[r13,#0x9c]
 
        MOV		r1,r1,ASR #16
 
        STRH		r1,[r0,#0]
 
        LDR		r0,[r13,#0x90]
 
        CMP		r1,r9
 
        ADD		r3,r0,r5,LSL #1
 
        LDR		r0,[r13,#0x98]
 
        MOVGT		r9,r1
 
        ADD		r2,r0,r5,LSL #2
 
        STMIA		r13,{r1-r3}
 
        LDR		r0,[r4,#0x90]
 
        MOV		r3,r9,LSL #16
 
        MOV		r1,r11,LSL #16
 
        MOV		r1,r1,ASR #16
 
        MOV		r3,r3,ASR #16
 
        MOV		r2,r10
 
        ADD		r0,r0,r8,LSL #2
 
        BL		BetterScalFact @ 0x194
        LDR		r1,[r13,#0xa0]
 
        STRH		r0,[r1,#0]
 
Branch87:
        ADD		r5,r5,#1
 
Branch84:
        LDRSH		r0,[r4,#4]
 
        CMP		r0,r5
 
        BGT		Branch88 @ 0xc20
        MOV		r0,#0
 
        SUB		r5,r0,#0x8000
 
Branch89:
        LDRSH		r1,[r4,#4]
 
        CMP		r1,r0
 
        ADDGT		r1,r13,#0x18
 
        ADDGT		r1,r1,r0,LSL #1
 
        STRGTH		r5,[r1,#0]
 
        ADDGT		r0,r0,#1
 
        BGT		Branch89 @ 0xd44
        ADD		r1,r13,#0xd8
 
        LDMIA		r1,{r0,r1}
 
        LDR		r2,[r13,#0x90]
 
        MOV		r3,#1
 
        ADD		r6,r13,#8
 
        STMIA		r6,{r0-r3}
 
        LDR		r3,[r13,#0xb0]
 
        ADD		r2,r13,#0x18
 
        STMIA		r13,{r2,r3}
 
        ADD		r3,r13,#0x94
 
        LDMIA		r3,{r2,r3}
 
        MOV		r1,r7
 
        MOV		r0,r4
 
        BL		Scf_SearchSingleBands @ 0x490
        LDR		r1,[r13,#0xb0]
 
        ADD		r3,r13,#0xd8
 
        LDMIA		r3,{r2,r3}
 
        ADD		r0,r13,#0x18
 
        STMIA		r13,{r0-r3}
 
        ADD		r3,r13,#0x94
 
        LDMIA		r3,{r2,r3}
 
        MOV		r1,r7
 
        MOV		r0,r4
 
        BL		Scf_DiffReduction @ 0x8f8
        LDR		r2,Var13 
        MOV		r0,#0
 
        LDRSH		r12,[r4,#4]
 
        B		Branch90 @ 0xdf4
Branch91:
        ADD		r3,r7,r0,LSL #1
 
        LDRSH		r1,[r3,#0]
 
        LDRH		r3,[r3,#0]
 
        ADD		r0,r0,#1
 
        CMP		r1,r5
 
        MOVGT		r5,r1
 
        CMP		r3,#0x8000
 
        CMPNE		r1,r2
 
        MOVLT		r2,r1
 
Branch90:
        CMP		r12,r0
 
        BGT		Branch91 @ 0xdd0
        ADD		r1,r2,#0x3c
 
        MOV		r12,r1,LSL #16
 
        MOV		r12,r12,ASR #16
 
        MOV		r0,#0
 
        B		Branch92 @ 0xe2c
Branch93:
        ADD		r2,r7,r0,LSL #1
 
        LDRH		r3,[r2,#0]
 
        ADD		r0,r0,#1
 
        CMP		r3,#0x8000
 
        LDRNESH		r3,[r2,#0]
 
        CMPNE		r3,r1
 
        STRGTH		r12,[r2,#0]
 
Branch92:
        LDRSH		r2,[r4,#4]
 
        CMP		r2,r0
 
        BGT		Branch93 @ 0xe10
        CMP		r1,r5
 
        MOVLT		r5,r1
 
        CMN		r5,#0x8000
 
        LDR		r0,[r13,#0xac]
 
        MOV		r2,#0
 
        STRLEH		r2,[r0,#0]
 
        MOVLE		r1,#0
 
        BLE		Branch94 @ 0xef4
        STRH		r5,[r0,#0]
 
        MOV		r14,#0
 
        MOV		r1,#0
 
        B		Branch95 @ 0xeb4
Branch98:
        ADD		r0,r7,r1,LSL #1
 
        LDRH		r3,[r0,#0]
 
        CMP		r3,#0x8000
 
        LDRNEH		r3,[r0,#0]
 
        SUBNE		r3,r5,r3
 
        MOVNE		r14,r3,LSL #16
 
        MOVNE		r14,r14,ASR #16
 
        STRNEH		r14,[r0,#0]
 
        BNE		Branch96 @ 0xeb0
        STRH		r14,[r0,#0]
 
        ADD		r12,r4,r1,LSL #1
 
        LDRSH		r0,[r12,#0xc]
 
Branch97:
        LDRSH		r3,[r12,#0xe]
 
        CMP		r3,r0
 
        LDRGT		r3,[r4,#0x90]
 
        STRGT		r2,[r3,r0,LSL #2]
 
        ADDGT		r0,r0,#1
 
        BGT		Branch97 @ 0xe98
Branch96:
        ADD		r1,r1,#1
 
Branch95:
        LDRSH		r0,[r4,#4]
 
        CMP		r0,r1
 
        BGT		Branch98 @ 0xe68
Branch101:
        ADD		r13,r13,#0xb4
 
        LDMFD		r13!,{r4-r11,pc}
 
Branch100:
        ADD		r0,r7,r1,LSL #1
 
        STRH		r2,[r0,#0]
 
        ADD		r12,r4,r1,LSL #1
 
        LDRSH		r0,[r12,#0xc]
 
Branch99:
        LDRSH		r3,[r12,#0xe]
 
        CMP		r3,r0
 
        LDRGT		r3,[r4,#0x90]
 
        STRGT		r2,[r3,r0,LSL #2]
 
        ADDGT		r0,r0,#1
 
        BGT		Branch99 @ 0xed8
        ADD		r1,r1,#1
 
Branch94:
        LDRSH		r0,[r4,#4]
 
        CMP		r0,r1
 
        BGT		Branch100 @ 0xec8
        B		Branch101 @ 0xec0
EstimFactAllChan:
        STMFD		r13!,{r4-r10,r14}
 
        LDR		r5,[r13,#0x20]
 
        MOV		r10,#0x2f
 
        MOV		r9,r3
 
        MOV		r8,r2
 
        MOV		r7,r1
 
        MOV		r6,r0
 
        MOV		r4,#0
 
        B		Branch102 @ 0xf50
Branch103:
        MUL		r0,r10,r4
 
        ADD		r3,r9,r0,LSL #5
 
        RSB		r0,r4,r4,LSL #4
 
        ADD		r2,r8,r0,LSL #3
 
        ADD		r1,r7,r0,LSL #3
 
        ADD		r0,r6,r0,LSL #3
 
        BL		EstFactorOneChann @ 0x0
        ADD		r0,r4,#1
 
        MOV		r4,r0,LSL #16
 
        MOV		r4,r4,ASR #16
 
Branch102:
        CMP		r4,r5
 
        BLT		Branch103 @ 0xf28
        LDMFD		r13!,{r4-r10,pc}
 
CalcScaleFactAllChan:
        STMFD		r13!,{r1-r11,r14}
 
        ADD		r6,r13,#0x30
 
        LDMIA		r6,{r5,r6}
 
        MOV		r11,#0xe7
 
        MOV		r10,r3
 
        MOV		r9,r2
 
        MOV		r8,r1
 
        MOV		r7,r0
 
        MOV		r4,#0
 
        B		Branch104 @ 0xfc4
Branch105:
        RSB		r0,r4,r4,LSL #4
 
        ADD		r3,r5,r0,LSL #3
 
        ADD		r2,r10,r0,LSL #3
 
        STMIA		r13,{r2,r3}
 
        ADD		r3,r9,r0,LSL #3
 
        MUL		r0,r11,r4
 
        ADD		r0,r8,r0,LSL #3
 
        ADD		r2,r0,#0xc
 
        LDR		r1,[r0,#8]
 
        MOV		r12,#0x2f
 
        MUL		r0,r12,r4
 
        ADD		r0,r7,r0,LSL #5
 
        BL		CalcScaleFactOneChann @ 0xbec
        ADD		r0,r4,#1
 
        MOV		r4,r0,LSL #16
 
        MOV		r4,r4,ASR #16
 
Branch104:
        CMP		r4,r6
 
        BLT		Branch105 @ 0xf84
        LDMFD		r13!,{r1-r11,pc}
 
Count_SinglePe:
        ADD		r0,r0,r0,LSL #1
 
        RSB		r0,r0,r1,LSL #3
 
        CMP		r0,#0x18
 
        ADDLT		r0,r0,#0x15
 
        MOVLT		r0,r0,ASR #1
 
        MUL		r1,r0,r2
 
        LDR		r0,Var15 
        STMFD		r13!,{r3,r14}
 
        MUL		r1,r0,r1
 
        MOV		r0,r1,ASR #14
 
        BL		saturate @ 0x0
        MOV		r0,r0,LSL #16
 
        MOV		r0,r0,ASR #16
 
        LDMFD		r13!,{r3,pc}
 
Count_PeDiff:
        STMFD		r13!,{r4-r10,r14}
 
        ADD		r12,r13,#0x20
 
        LDMIA		r12,{r7,r8,r12}
 
        LDR		r9,[r13,#0x2c]
 
        MOV		r14,#0
 
        B		Branch106 @ 0x10b0
Branch109:
        ADD		r5,r0,r12,LSL #1
 
        LDRH		r4,[r5,#0]
 
        CMP		r4,#0x8000
 
        BEQ		Branch107 @ 0x10ac
        ADD		r4,r2,r12,LSL #1
 
        LDRH		r6,[r4,#0]
 
        CMP		r6,#0x8000
 
        BNE		Branch108 @ 0x1060
        ADD		r6,r3,r12,LSL #1
 
        ADD		r10,r7,r12,LSL #1
 
        LDRSH		r10,[r10,#0]
 
        LDRSH		r6,[r6,#0]
 
        SUB		r6,r6,r10
 
        SUB		r6,r6,#0x12
 
        MOV		r6,r6,ASR #2
 
        STRH		r6,[r4,#0]
 
Branch108:
        LDRSH		r4,[r4,#0]
 
        LDRSH		r5,[r5,#0]
 
        ADD		r6,r1,r12,LSL #1
 
        MOV		r4,r4,LSL #3
 
        ADD		r5,r5,r5,LSL #1
 
        SUB		r5,r4,r5
 
        LDRSH		r6,[r6,#0]
 
        CMP		r5,#0x18
 
        ADDLT		r5,r5,#0x15
 
        ADD		r6,r6,r6,LSL #1
 
        SUB		r4,r4,r6
 
        MOVLT		r5,r5,ASR #1
 
        CMP		r4,#0x18
 
        ADD		r6,r8,r12,LSL #1
 
        LDRSH		r6,[r6,#0]
 
        ADDLT		r4,r4,#0x15
 
        MOVLT		r4,r4,ASR #1
 
        SUB		r4,r4,r5
 
        MLA		r14,r4,r6,r14
 
Branch107:
        ADD		r12,r12,#1
 
Branch106:
        CMP		r12,r9
 
        BLT		Branch109 @ 0x1020
        LDR		r0,Var15 
        MUL		r1,r0,r14
 
        MOV		r0,r1,ASR #14
 
        BL		saturate @ 0x0
        MOV		r0,r0,LSL #16
 
        MOV		r0,r0,ASR #16
 
        LDMFD		r13!,{r4-r10,pc}
 
Var10:        .word		0xffff8000
 
Var11:        .word		0x00005555
 
Var12:        .word		0xfff9555c
 
Var15:        .word		0x0000059a
 
saturate:
        MOV		r1,r0
 
        MOV		r2,r1,ASR #15
 
        TEQ		r2,r1,ASR #31
 
        LDRNE		r0,Var16 
        EORNE		r0,r0,r1,ASR #31
 
        MOVEQ		r0,r1
 
        MOV		pc,r14
 
@Branch7
Var16:        .word		0x00007fff
 

    .END

