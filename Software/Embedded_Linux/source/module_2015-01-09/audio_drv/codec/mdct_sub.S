.global do_long_window
.global do_long_window_minus
.global do_start_short_minus
.global do_stop_short
.global do_short_win
.global MDCT_OneExd_24bit
.global Radix4_1_24bit
.global Radix4FFTExd_Inner_24bit
.global MDCT_SecondExd_24bit
.global Radix8FFTExd_24bit

.text	 
.align 0

do_long_window:
	STMFD    r13!,{r4-r11,r14}
;@SUB      r13,r13,#0x14
	MOV      r10,r3			;@r10:minSf
;@LDR      r2,0x12e4		;@
;@LDR      r4,[r2,#0]		;@r4: LongWindowKBD_24bit_ptr
	MOV	 r4, r2	
	ADD      r5,r4,#0x800		;@r5: LongWindowKBD_24bit_ptr+512
	MOV      r6,r0			;@r6: mdctDelayBuffer (int16)
	ADD      r7,r0,#0x700		;@ 
	ADD      r7,r7,#0xfe		;@r7: mdctDelayBuffer + FRAME_LEN_LONG - 1 
	ADD      r8,r1,#0x800		;@r8: outData0
	MOV      r9,#0x200		;@r9: loop counter


		.word	0XEE800310;@	FMMC MOD,#2 	
	MOV  r4, r4, LSR #2
	MOV  r5, r5, LSR #2
	MOV  r8, r8, LSR #2
		.word	0XEEB08214;@	FMIR I4,R4 	;@LongWindowKBD_24bit_ptr
		.word	0XEEB0A215;@	FMIR I5,R5 	;@LongWindowKBD_24bit_ptr+512
		.word	0XEEB10210;@	FMIR I0,R8 	

        ADR      r0,DO_LONG_WIN_START          
        ADR      r1,DO_LONG_WIN_END
		.word	0XEEE00220;@	FLOOP R0,R1 	
		.word	0XEEE32200;@	FLCNT R9 	
		.word	0XEE800330;@	FMMC MOD,#6 	
		.word	0XEE800309;@	FMMC M1,#1 	
		.word	0XEE01C024;@	FLR R1,[I4],M4 	
DO_LONG_WIN_START:
	LDRSH    r0,[r6],#2		;@pcm16 (loop start)

	MOV      r0,r0,lsl r10		;@pcm<<minSf

;@FLR      R1, [I4],M4
		.word	0XEE820025;@	FSMPL ACC0,R0,R1,R1,[I5],M4 	

	LDRSH    r0,[r7],#-2

	MOV      r0,r0,LSL r10

;@FLR      R1,[I5],M4
		.word	0XEE420024;@	FSMAML ACC0,R0,R1,R1,[I4],M4 	

;@FMRA	 R0, ACC0.H
DO_LONG_WIN_END:
;@STR      r0,[r8],#4
		.word	0XEE03C228;@	FSA ACC0.h,[I0],M1 	

	LDMFD    r13!,{r4-r11,pc}



do_long_window_minus:
	STMFD    r13!,{r4-r11,r14}
;@SUB      r13,r13,#0x14
	MOV      r10,r3			;@r10:minSf
;@LDR      r2,0x12e4		;@
;@LDR      r4,[r2,#0]		;@r4: LongWindowKBD_24bit_ptr
	MOV	 r4, r2	
	ADD      r5,r4,#0x800		;@r5: LongWindowKBD_24bit_ptr+512
	MOV      r6,r0			;@r6: mdctDelayBuffer (int16)
	ADD      r7,r0,#0x700		;@ 
	ADD      r7,r7,#0xfe		;@r7: mdctDelayBuffer + FRAME_LEN_LONG - 1 
	ADD      r8,r1,#0x800		;@r8: outData0
	SUB      r8, r8, #4
	MOV      r9,#0x200		;@r9: loop counter


		.word	0XEE800310;@	FMMC MOD,#2 	
	MOV  r4, r4, LSR #2
	MOV  r5, r5, LSR #2
	MOV  r8, r8, LSR #2
		.word	0XEEB08214;@	FMIR I4,R4 	;@LongWindowKBD_24bit_ptr
		.word	0XEEB0A215;@	FMIR I5,R5 	;@LongWindowKBD_24bit_ptr+512
		.word	0XEEB10210;@	FMIR I0,R8 	

        ADR      r0,DO_LONG_WIN_START_1          
        ADR      r1,DO_LONG_WIN_END_1
		.word	0XEEE00220;@	FLOOP R0,R1 	
		.word	0XEEE32200;@	FLCNT R9 	
		.word	0XEE800330;@	FMMC MOD,#6 	
		.word	0XEEFFF3FB;@	FMMC M3,#ffffffff 	
		.word	0XEE01C025;@	FLR R1,[I5],M4 	
DO_LONG_WIN_START_1:
	LDRSH    r0,[r6],#2		;@pcm16 (loop start)
;@ADD      r2,r13,#0xc
	MOV      r0,r0,lsl r10		;@pcm<<minSf
;@LDR      r0,[r4,#0]		;@win[]
;@SMULL    r12,r1,r0,r1		;@--
;@MOV      r0,r12,LSR #24		;@ |mul and truncate 24-bit
;@ORR      r0,r0,r1,LSL #8	;@--
;@FLR      R1, [I5],M4
		.word	0XEEC20024;@	FSMML ACC0,R0,R1,R1,[I4],M4 	


;@STR      r1,[r13,#8]
;@MOV      r1,r1,ASR #24
;@STR      r12,[r13,#4]
;@STMIA    r2,{r0,r1}
;@MOV      r11,r0
	LDRSH    r0,[r7],#-2
;@ADD      r2,r13,#0xc
	MOV      r0,r0,LSL r10
;@LDR      r0,[r5,#0]		;@*dctIn1
;@SMULL    r12,r1,r0,r1		;@-- 
;@MOV      r0,r12,LSR #24      	;@ |mul and truncate 24-bit
;@ORR      r0,r0,r1,LSL #8     	;@-- 
;@FLR      R1,[I4],M4
		.word	0XEE420025;@	FSMAML ACC0,R0,R1,R1,[I5],M4 	
;@STR      r1,[r13,#8]
;@MOV      r1,r1,ASR #24
;@STR      r12,[r13,#4]
;@STMIA    r2,{r0,r1}
;@SUB      r0,r11,r0
;@FMRA	 R0, ACC0.H
DO_LONG_WIN_END_1:
;@STR      r0,[r8],#-4
		.word	0XEE03C238;@	FSA ACC0.h,[I0],M3 	
;@ADD      r4,r4,#4
;@ADD      r5,r5,#4

;@BNE      {pc} - 0x70  ;@ 0xb88 ;@loop end

;@ADD      r13,r13,#0x14
	LDMFD    r13!,{r4-r11,pc}


do_start_short_minus:
	STMFD    r13!,{r4-r11,r14}
;@SUB      r13,r13,#0x14
	MOV      r10,r3			;@r10:minSf
;@LDR      r2,0x12e4		;@
;@LDR      r4,[r2,#0]		;@r4: ShortWindowSineExd_24bit_ptr
	MOV	 r4, r2	
	ADD      r5,r4,#0x200		;@r5: ShortWindowSineExd_24bit_ptr+128
	ADD      r6, r0, #0x380;@      	;@r6: dctIn0 = mdctDelayBuffer(r0) + LS_TRANS(448*2);@ (int16)
;@MOV      r6,r0			;@
	ADD      r7,r0,#0x400		;@ 
	ADD      r7,r7,#0x7e		;@r7: dctIn1 = mdctDelayBuffer + (FRAME_LEN_LONG(1024) - 1 - LS_TRANS(448))*2;@
	ADD      r8,r1,#0xfc		;@r8: outData0, offset:(512- 448 - 1)*4=0xfc
;@SUB      r8, r8, #4
	MOV      r9,#0x40		;@r9: loop counter


		.word	0XEE800310;@	FMMC MOD,#2 	
	MOV  r4, r4, LSR #2
	MOV  r5, r5, LSR #2
		.word	0XEEB08214;@	FMIR I4,R4 	;@LongWindowKBD_24bit_ptr
		.word	0XEEB0A215;@	FMIR I5,R5 	;@LongWindowKBD_24bit_ptr+512

        ADR      r0,DO_LONG_WIN_START_2          
        ADR      r1,DO_LONG_WIN_END_2
		.word	0XEEE00220;@	FLOOP R0,R1 	
		.word	0XEEE32200;@	FLCNT R9 	
		.word	0XEE800330;@	FMMC MOD,#6 	
;@FLR      R1, [I4],M4          	
DO_LONG_WIN_START_2:
	LDRSH    r0,[r6],#2		;@pcm16 (loop start)
;@ADD      r2,r13,#0xc
	MOV      r0,r0,lsl r10		;@pcm<<minSf
;@LDR      r0,[r4,#0]		;@win[]
;@SMULL    r12,r1,r0,r1		;@--
;@MOV      r0,r12,LSR #24		;@ |mul and truncate 24-bit
;@ORR      r0,r0,r1,LSL #8	;@--
		.word	0XEE01C025;@	FLR R1,[I5],M4 	
		.word	0XEEC210E0;@	FSMM ACC0,R0,R1 	


;@STR      r1,[r13,#8]
;@MOV      r1,r1,ASR #24
;@STR      r12,[r13,#4]
;@STMIA    r2,{r0,r1}
;@MOV      r11,r0
	LDRSH    r0,[r7],#-2
;@ADD      r2,r13,#0xc
	MOV      r0,r0,LSL r10
;@LDR      r0,[r5,#0]		;@*dctIn1
;@SMULL    r12,r1,r0,r1		;@-- 
;@MOV      r0,r12,LSR #24      	;@ |mul and truncate 24-bit
;@ORR      r0,r0,r1,LSL #8     	;@-- 
		.word	0XEE01C024;@	FLR R1,[I4],M4 	
		.word	0XEE4210E0;@	FSMAM ACC0,R0,R1 	
;@STR      r1,[r13,#8]
;@MOV      r1,r1,ASR #24
;@STR      r12,[r13,#4]
;@STMIA    r2,{r0,r1}
;@SUB      r0,r11,r0
		.word	0XEEF00220;@	FMRA R0,ACC0.h 	
DO_LONG_WIN_END_2:
	STR      r0,[r8],#-4
;@ADD      r4,r4,#4
;@ADD      r5,r5,#4

;@BNE      {pc} - 0x70  ;@ 0xb88 ;@loop end

;@ADD      r13,r13,#0x14
	LDMFD    r13!,{r4-r11,pc}



do_stop_short:
	STMFD    r13!,{r4-r11,r14}
;@SUB      r13,r13,#0x14
	MOV      r10,r3			;@r10:minSf
;@LDR      r2,0x12e4		;@
;@LDR      r4,[r2,#0]		;@r4: ShortWindowSineExd_24bit_ptr
	MOV	 r4, r2	
	ADD      r5,r4,#0x200		;@r5: ShortWindowSineExd_24bit_ptr+128
	ADD      r6, r0, #0x380;@      	;@r6: dctIn0 = mdctDelayBuffer(r0) + LS_TRANS(448*2);@ (int16)
;@MOV      r6,r0			;@
	ADD      r7,r0,#0x400		;@ 
	ADD      r7,r7,#0x7e		;@r7: dctIn1 = mdctDelayBuffer + (FRAME_LEN_LONG(1024) - 1 - LS_TRANS(448))*2;@
	ADD      r8,r1,#0xf00		;@r8: outData0, offset:(512 + 448)*4=0xf00
;@SUB      r8, r8, #4
	MOV      r9,#0x40		;@r9: loop counter


		.word	0XEE800310;@	FMMC MOD,#2 	
	MOV  r4, r4, LSR #2
	MOV  r5, r5, LSR #2
	MOV  r8, r8, LSR #2

		.word	0XEEB08214;@	FMIR I4,R4 	;@LongWindowKBD_24bit_ptr
		.word	0XEEB0A215;@	FMIR I5,R5 	;@LongWindowKBD_24bit_ptr+512
		.word	0XEEB10210;@	FMIR I0,R8 	

        ADR      r0,DO_SHORT_WIN_START_2          
        ADR      r1,DO_SHORT_WIN_END_2
		.word	0XEEE00220;@	FLOOP R0,R1 	
		.word	0XEEE32200;@	FLCNT R9 	
		.word	0XEE800330;@	FMMC MOD,#6 	
		.word	0XEE800309;@	FMMC M1,#1 	
		.word	0XEE01C024;@	FLR R1,[I4],M4 	
DO_SHORT_WIN_START_2:
	LDRSH    r0,[r6],#2		;@pcm16 (loop start)
;@ADD      r2,r13,#0xc
	MOV      r0,r0,lsl r10		;@pcm<<minSf
;@LDR      r0,[r4,#0]		;@win[]
;@SMULL    r12,r1,r0,r1		;@--
;@MOV      r0,r12,LSR #24		;@ |mul and truncate 24-bit
;@ORR      r0,r0,r1,LSL #8	;@--
;@FLR      R1, [I4],M4
		.word	0XEE820025;@	FSMPL ACC0,R0,R1,R1,[I5],M4 	


;@STR      r1,[r13,#8]
;@MOV      r1,r1,ASR #24
;@STR      r12,[r13,#4]
;@STMIA    r2,{r0,r1}
;@MOV      r11,r0
	LDRSH    r0,[r7],#-2
;@ADD      r2,r13,#0xc
	MOV      r0,r0,LSL r10
;@LDR      r0,[r5,#0]		;@*dctIn1
;@SMULL    r12,r1,r0,r1		;@-- 
;@MOV      r0,r12,LSR #24      	;@ |mul and truncate 24-bit
;@ORR      r0,r0,r1,LSL #8     	;@-- 
;@FLR      R1,[I5],M4
		.word	0XEE420024;@	FSMAML ACC0,R0,R1,R1,[I4],M4 	
;@STR      r1,[r13,#8]
;@MOV      r1,r1,ASR #24
;@STR      r12,[r13,#4]
;@STMIA    r2,{r0,r1}
;@SUB      r0,r11,r0
;@FMRA	 R0, ACC0.H
DO_SHORT_WIN_END_2:
;@STR      r0,[r8],#4
		.word	0XEE03C228;@	FSA ACC0.h,[I0],M1 	
;@ADD      r4,r4,#4
;@ADD      r5,r5,#4

;@BNE      {pc} - 0x70  ;@ 0xb88 ;@loop end

;@ADD      r13,r13,#0x14
	LDMFD    r13!,{r4-r11,pc}

	
do_short_win:
        STMFD    r13!,{r4-r11,r14}
;@SUB      r13,r13,#0x24
        MOV      r6,r3				;@r6: minSf
        MOV      r7,r0				;@r7: dctIn0
        ADD      r8,r0,#0xfe			;@r8: dctIn1
        ADD      r9,r0,#0x100			;@r9: dctIn00
        ADD      r10,r0,#0x100			
        ADD      r10,r10,#0xfe			;@r10:dctIn11
        ADD      r0,r1,#0x100			;@r0: outData0
;@STR      r0,[r13,#0x18]
        ADD      r1,r1,#0xfc			;@r1: outData1
;@STR      r1,[r13,#0x14]
        MOV      r4,r2				;@r4: word_ptr  (ShortWindowSineExd_24bit_ptr)
        ADD      r5,r2,#0x200			;@r5: mid_winPtr(ShortWindowSineExd_24bit_ptr+128)
        MOV      r11,#0x40			;@r11:loop counter

		.word	0XEE800309;@	FMMC M1,#1 	
		.word	0XEEFFF3FB;@	FMMC M3,#ffffffff 	

		.word	0XEE800310;@	FMMC MOD,#2 	
	MOV  r4, r4, LSR #2
	MOV  r5, r5, LSR #2
		.word	0XEEB08214;@	FMIR I4,R4 	
		.word	0XEEB0A215;@	FMIR I5,R5 	
	MOV  r0, r0, LSR #2
	MOV  r1, r1, LSR #2
		.word	0XEEB00210;@	FMIR I0,R0 	
		.word	0XEEB02211;@	FMIR I1,R1 	

        ADR      r2,DO_SHORT_BLOCK_BEG          
        ADR      r3,DO_SHORT_BLOCK_END
		.word	0XEEE04260;@	FLOOP R2,R3 	
	ADD      r11, r11, #-1
		.word	0XEEE36200;@	FLCNT R11 	
		.word	0XEE800330;@	FMMC MOD,#6 	

;@---- TEST -------------------------------
;@FSMPL   ACC0,R2,R3,R12, [I4],M4
;@---- END --------------------------------       
;@ no use register: r2, r3, r4, r5, r11, r12
        LDRSH    r2,[r7],#2			;@dctIn0
        MOV      r2,r2,LSL r6
		.word	0XEE01D064;@	FLR R11,[I4],M4 	;@*word_ptr
		.word	0XEE8D70E0;@	FSMP ACC0,R2,R11 	
        LDRSH    r3,[r8],#-2			;@dctIn1
        MOV      r3,r3,LSL r6
		.word	0XEE01D085;@	FLR R12,[I5],M4 	;@*mid_winPtr
		.word	0XEE4D90E0;@	FSMAM ACC0,R3,R12 	
;@FSA	 ACC0.H, [I0],M1
        LDRSH    r2,[r9],#2			;@dctIn00
        MOV      r2,r2,LSL r6

		.word	0XEE4B9228;@	FSMMSA ACC0,R2,R12,ACC0.h,[I0],M1 	;@(*dctIn00)*(*mid_winPtr)


        LDRSH    r3,[r10],#-2			;@dctIn11
        MOV      r3,r3,LSL r6
		.word	0XEE4F70E0;@	FSMAM ACC0,R3,R11 	;@(*dctIn11)*(*word_ptr)        
DO_SHORT_BLOCK_BEG:
        LDRSH    r2,[r7],#2			;@dctIn0
        MOV      r2,r2,LSL r6
		.word	0XEE01D064;@	FLR R11,[I4],M4 	;@*word_ptr

		.word	0XEE0D7239;@	FSMPSA ACC0,R2,R11,ACC0.h,[I1],M3 	

        LDRSH    r3,[r8],#-2			;@dctIn1
        MOV      r3,r3,LSL r6
		.word	0XEE01D085;@	FLR R12,[I5],M4 	;@*mid_winPtr
		.word	0XEE4D90E0;@	FSMAM ACC0,R3,R12 	

        LDRSH    r2,[r9],#2			;@dctIn00
        MOV      r2,r2,LSL r6

		.word	0XEE4B9228;@	FSMMSA ACC0,R2,R12,ACC0.h,[I0],M1 	;@(*dctIn00)*(*mid_winPtr)

        LDRSH    r3,[r10],#-2			;@dctIn11
        MOV      r3,r3,LSL r6
DO_SHORT_BLOCK_END:
		.word	0XEE4F70E0;@	FSMAM ACC0,R3,R11 	;@(*dctIn11)*(*word_ptr)

		.word	0XEE03C239;@	FSA ACC0.h,[I1],M3 	

        LDMFD    r13!,{r4-r11,pc}




;@*****************************************************************
MDCT_OneExd_24bit:
        STMFD    r13!,{r4-r11,r14}

		.word	0XEE800316;@	FMMC M6,#2 	
		.word	0XEE800309;@	FMMC M1,#1 	
		.word	0XEEFFF3FB;@	FMMC M3,#ffffffff 	
		.word	0XEE800305;@	FMMC M5,#0 	

		.word	0XEE800310;@	FMMC MOD,#2 	;@        
        MOV     r0, r0, lsr #2		;@buf0		
        MOV     r2, r2, lsr #2		;@csptr
		.word	0XEEB00210;@	FMIR I0,R0 	;@I0: buf0
		.word	0XEEB00211;@	FMIR I1,R0 	;@I1: buf0
		.word	0XEEB00215;@	FMIR I5,R0 	;@I5: buf0
		.word	0XEEB04214;@	FMIR I4,R2 	;@I4: csptr
        ADD     r2, r2, #1;@
		.word	0XEEB04216;@	FMIR I6,R2 	;@I4: csptr+1

   	ADD     r3, r0, r1;@
        SUB     r3, r3, #1;@		;@buf0 + num - 1
		.word	0XEEB06212;@	FMIR I2,R3 	;@			;@I2: buf1 
		.word	0XEEB06213;@	FMIR I3,R3 	;@			;@I3: buf1 

		.word	0XEE800338;@	FMMC MOD,#7 	;@

        ADR	r4, LOOP_SHIFT_START
        ADR     r5, LOOP_SHIFT_END
		.word	0XEEE082A0;@	FLOOP R4,R5 	
		.word	0XEEE22200;@	FLCNT R1 	;@num

LOOP_SHIFT_START:
		.word	0XEE01C08D;@	FLR R4,[I5],M5 	
	MOV     r4, r4, asr #2;@
LOOP_SHIFT_END:
		.word	0XEEF88205;@	FSR R4,[I5],M4 	;@FSR: MOD bit 0 must set to 1, source register (right justified), get LSB 24-bit 
;@FSR: MOD bit 0 must set to 0, source register (left justified),  get MSB 24-bit
;@FSA: always "source register (right justified), get LSB 24-bit", don't be affected by MOD 
        ADR	r4, LOOP_MDCT_One_START
        ADR     r5, LOOP_MDCT_One_END
		.word	0XEEE082A0;@	FLOOP R4,R5 	
        MOV 	r1, r1, lsr #2;@
		.word	0XEEE22200;@	FLCNT R1 	;@num>>2        

		.word	0XEE01C118;@	FLR2 R0,R1,[I0,I4],M1,M6 	
LOOP_MDCT_One_START:
;@MOV     r8, #0;@ 
;@------------------
;@I0, I1
;@tr1 = *(buf0 + 0)			//Q10.22
;@ti2 = *(buf0 + 1)
;@------------------
;@I2, I3
;@ti1 = *(buf1 + 0)
;@tr2 = *(buf1 - 1)
;@------------------
;@I4: cossintab
;@------------------

;@FLR2	R0,R1 [I0,I4],M1,M6			;@R0:tr1, 
;@R1:cosa =   *(buf0 + 0)
;@I0--> ti2,  *(buf0 + 1) 
;@ cosa: R1 
;@ tr1:  R0  							
;@*buf0++ = MUL24_HIGH(cosa, tr1) + MUL24_HIGH(sina, ti1);@ 

		.word	0XEE82017A;@	FSMPL2 ACC0,R1,R0,R2,R3,[I2,I6],M3,M6 	;@R2:ti1 
;@R3:sina =   *(buf1 + 0)
;@I2--> tr2,  *(buf1 - 1)
;@ ti1:  R2 
;@ sina: R3 
;@MOV     r8, #0;@

		.word	0XEE0250E0;@	FSMAP ACC0,R3,R2 	
;@FSA	ACC0.H,[I1],M1
;@MOV     r8, #0;@

;@*buf0++ = MUL24_HIGH(cosa, ti1) - MUL24_HIGH(sina, tr1);@
		.word	0XEE023229;@	FSMPSA ACC0,R1,R2,ACC0.h,[I1],M1 	
;@MOV     r8, #0;@
		.word	0XEE460118;@	FSMAML2 ACC0,R3,R0,R0,R1,[I0,I4],M1,M6 	
;@MOV     r8, #0;@
		.word	0XEE03C229;@	FSA ACC0.h,[I1],M1 	
;@MOV     r8, #0;@
;@*buf1-- = MUL24_HIGH(cosb, ti2) - MUL24_HIGH(sinb, tr2);@
;@ti2:  R0
;@cosb: R1
		.word	0XEE82017A;@	FSMPL2 ACC0,R1,R0,R2,R3,[I2,I6],M3,M6 	
;@MOV     r8, #0;@
;@tr2:  R2
;@sinb: R3
		.word	0XEE4250E0;@	FSMAM ACC0,R3,R2 	
;@MOV     r8, #0;@
;@FSA	ACC0.H,[I3],M3

		.word	0XEE02323B;@	FSMPSA ACC0,R1,R2,ACC0.h,[I3],M3 	
;@MOV     r8, #0;@
		.word	0XEE060118;@	FSMAPL2 ACC0,R3,R0,R0,R1,[I0,I4],M1,M6 	
;@MOV     r8, #0;@
LOOP_MDCT_One_END:
		.word	0XEE03C23B;@	FSA ACC0.h,[I3],M3 	
		.word	0XEE800330;@	FMMC MOD,#6 	;@
        LDMFD    r13!,{r4-r11,pc}


;@*****************************************************************
Radix4_1_24bit:
        STMFD    r13!,{r4-r9,r14}
        ADR	r4, LOOP_Radix4_1_START
        ADR     r5, LOOP_Radix4_1_END
		.word	0XEEE082A0;@	FLOOP R4,R5 	
		.word	0XEEE22200;@	FLCNT R1 	;@num

LOOP_Radix4_1_START:
        LDR      r5,[r0,#8]
        LDR      r12,[r0,#0]
        ADD      r2,r12,r5
        LDR      r4,[r0,#0xc]
        LDR      r14,[r0,#4]
        ADD      r3,r14,r4
        SUB      r12,r12,r5
        SUB      r14,r14,r4
        LDR      r6,[r0,#0x10]
        LDR      r7,[r0,#0x18]
        ADD      r4,r6,r7
        LDR      r8,[r0,#0x14]
        LDR      r9,[r0,#0x1c]
        ADD      r5,r8,r9
        SUB      r6,r6,r7
        SUB      r7,r8,r9
        ADD      r8,r2,r4
        STR      r8,[r0,#0]
        ADD      r8,r3,r5
        STR      r8,[r0,#4]
        SUB      r2,r2,r4
        STR      r2,[r0,#0x10]
        SUB      r2,r3,r5
        STR      r2,[r0,#0x14]
        ADD      r2,r12,r7
        STR      r2,[r0,#8]
        SUB      r2,r14,r6
        STR      r2,[r0,#0xc]
        SUB      r2,r12,r7
        STR      r2,[r0,#0x18]
        ADD      r2,r14,r6
        STR      r2,[r0,#0x1c]
LOOP_Radix4_1_END:
        ADD      r0,r0,#0x20
        LDMFD    r13!,{r4-r9,pc}


Radix4FFTExd_Inner_24bit:

	STMFD    r13!,{r0-r11,r14}
	SUB      r13,r13,#0x2c

	LDR      r1,[r13,#0x30]
        ADR	r4, LOOP_Radix4FFTExd_Inner_START
        ADR     r5, LOOP_Radix4FFTExd_Inner_END
		.word	0XEEE082A0;@	FLOOP R4,R5 	
		.word	0XEEE22200;@	FLCNT R1 	;@num

;@MOV      r5,r2			;@
	ADD     r3, r2, #1		;@fftstep+1
	RSB	r3, r3, #0
		.word	0XEED06212;@	FMMR M2,R3 	;@-(fftstep+1)
	SUB     r3, r2, #1
	MOV     r12, r3
		.word	0XEED06213;@	FMMR M3,R3 	;@fftstep-1

		.word	0XEE800309;@	FMMC M1,#1 	;@  
	LDR     r0,[r13,#0x2c]		;@&xptr
	LDR     r0,[r0,#0]		;@xptr = 0x8008000
	LDR     r1,[r13,#0x38]		;@&csptr
	LDR     r4,[r1,#0]		;@csptr =0x80021c0
;@LDR     r1,[r0,#0]		;@

		.word	0XEE800310;@	FMMC MOD,#2 	

	MOV	r0, r0, lsr #2
		.word	0XEEB00210;@	FMIR I0,R0 	;@
;@FMIR    I1, R0;@
	MOV     r4, r4, lsr #2
		.word	0XEEB08214;@	FMIR I4,R4 	;@
;@FMIR    I5, R4;@

		.word	0XEE800338;@	FMMC MOD,#7 	

LOOP_Radix4FFTExd_Inner_START:
	;@ADD     R12, R12, #0			;@debug code	
		.word	0XEE01C008;@	FLR R0,[I0],M1 	;@r0
		.word	0XEE01C038;@	FLR R1,[I0],M3 	;@r1

		.word	0XEE01C048;@	FLR R2,[I0],M1 	;@*t0
;@FLR     R4,[I0],M3		;@t1

		.word	0XEE01C064;@	FLR R3,[I4],M4 	;@*cosx
;@	R5,[I4],M4		;@sinx
	;@ADD     R12, R12, #0			;@debug code
;@r2 = MUL24_HIGH(cosx, t0) + MUL24_HIGH(sinx, t1);@	
		.word	0XEE8241B0;@	FSMPL2 ACC0,R2,R3,R4,R5,[I0,I4],M3,M4 	;@MUL24_HIGH(cosx, t0)
		.word	0XEE0290E0;@	FSMAP ACC0,R4,R5 	;@MUL24_HIGH(sinx, t1)
	;@ADD     R12, R12, #0			;@debug code
;@r3 = MUL24_HIGH(cosx, t1) - MUL24_HIGH(sinx, t0);@ 
		.word	0XEE9270E0;@	FSMP ACC1,R3,R4 	
		.word	0XEE565110;@	FSMAML2 ACC1,R2,R5,R8,R9,[I0,I4],M1,M4 	;@load (t0,cosx)
	;@ADD     R12, R12, #0			;@debug code
		.word	0XEEF00226;@	FMRA R6,ACC0.h 	;@r2: R6
		.word	0XEEF00267;@	FMRA R7,ACC1.h 	;@r3: R7
	;@ADD     R12, R12, #0			;@debug code
	MOV     R4, R0, ASR #2		;@t0: R4
	MOV     R5, R1, ASR #2		;@t1: R5
	;@ADD     R12, R12, #0			;@debug code
;@r0 = t0 - r2;@
	SUB     R0, R4, R6
;@r1 = t1 - r3;@
	SUB     R1, R5, R7
;@r2 = t0 + r2;@
	ADD     R2, R4, R6
;@r3 = t1 + r3;@ 
	ADD	R3, R5, R7

;@t0:  R8	t1:R10
;@cosx:R9      sinx:R11
;@r4 = MUL24_HIGH(cosx, t0) + MUL24_HIGH(sinx, t1);@	
		.word	0XEE811170;@	FSMPL2 ACC0,R8,R9,R10,R11,[I0,I4],M3,M4 	
		.word	0XEE0150E0;@	FSMAP ACC0,R10,R11 	

	;@ADD     R12, R12, #0			;@debug code

;@r5 = MUL24_HIGH(cosx, t1) - MUL24_HIGH(sinx, t0);@
		.word	0XEE9130E0;@	FSMP ACC1,R9,R10 	
		.word	0XEE5501D0;@	FSMAML2 ACC1,R8,R11,R6,R7,[I0,I4],M1,M4 	;@load (t0,cosx)
		.word	0XEEF00228;@	FMRA R8,ACC0.h 	;@"R4"
		.word	0XEEF00269;@	FMRA R9,ACC1.h 	;@"R5"	

	;@ADD     R12, R12, #0			;@debug code

		.word	0XEEFFF3FB;@	FMMC M3,#ffffffff 	

;@t0:  R6   t1: R10	
;@cosx:R7 sinx: R11
;@r6 = MUL24_HIGH(cosx, t0) + MUL24_HIGH(sinx, t1);@
		.word	0XEE82D170;@	FSMPL2 ACC0,R6,R7,R10,R11,[I0,I4],M3,M4 	;@I0-->xptr[0]
		.word	0XEE0150E0;@	FSMAP ACC0,R10,R11 	

	;@ADD     R12, R12, #0			;@debug code

;@r7 = MUL24_HIGH(cosx, t1) - MUL24_HIGH(sinx, t0);@
		.word	0XEE96F0E0;@	FSMP ACC1,R7,R10 	
		.word	0XEE5AD0E0;@	FSMAM ACC1,R6,R11 	

	;@ADD     R12, R12, #0			;@debug code

		.word	0XEEF00226;@	FMRA R6,ACC0.h 	;@
		.word	0XEEF00267;@	FMRA R7,ACC1.h 	;@

		.word	0XEED18213;@	FMMR M3,R12 	

;@r4 = t0 + r6;@
	ADD R4, R8, R6
;@r5 = r7 - t1;@
	SUB R5,R7,R9
;@r6 = t0 - r6;@
	SUB  R6, R8, R6;@
;@r7 = r7 + t1;@
	ADD R7, R7, R9

;@xptr[0] = r0 + r5;@
	ADD R11, R0, R5
		.word	0XEEF96208;@	FSR R11,[I0],M1 	
;@xptr[1] = r1 + r6;@
	ADD R11, R1, R6
		.word	0XEEF96210;@	FSR R11,[I0],M2 	
;@xptr -= fftstep;@

;@xptr[0] = r2 - r4;@
	SUB R11, R2, R4
		.word	0XEEF96208;@	FSR R11,[I0],M1 	
;@xptr[1] = r3 - r7;@
	SUB R11, R3, R7
		.word	0XEEF96210;@	FSR R11,[I0],M2 	
;@xptr -= fftstep;@

;@xptr[0] = r0 - r5;@
	SUB R11, R0, R5
		.word	0XEEF96208;@	FSR R11,[I0],M1 	
;@xptr[1] = r1 - r6;@
	SUB R11, R1, R6
		.word	0XEEF96210;@	FSR R11,[I0],M2 	
;@xptr -= fftstep;@

;@xptr[0] = r2 + r4;@
	ADD R11, R2, R4
		.word	0XEEF96208;@	FSR R11,[I0],M1 	
;@xptr[1] = r3 + r7;@
	ADD R11, R3, R7

;@xptr += 2;@ 
LOOP_Radix4FFTExd_Inner_END:
        .word	0XEEF96208;@	FSR R11,[I0],M1
	;@ADD     R12, R12, #0			;@debug code 



		.word	0XEEB00200;@	FMRI R0,I0 	
	ADD     R0, R0, #0X2000000
	MOV 	R0, R0, LSL #2

	LDR     r1,[r13,#0x2c]
	STR     r0,[r1,#0]

		.word	0XEEB00284;@	FMRI R4,I4 	
	ADD	R4, R4, #0X2000000
	MOV 	R4, R4, LSL #2

		.word	0XEE800330;@	FMMC MOD,#6 	

	LDR     r1,[r13,#0x38]
	STR     r4,[r1,#0]
	ADD     r13,r13,#0x3c
	LDMFD   r13!,{r4-r11,pc} 



;@*****************************************************************

MDCT_SecondExd_24bit:
        STMFD    r13!,{r4-r11,r14}

		.word	0XEE800326;@	FMMC M6,#4 	
		.word	0XEE800309;@	FMMC M1,#1 	
		.word	0XEE800312;@	FMMC M2,#2 	
		.word	0XEEFFF3F3;@	FMMC M3,#fffffffe 	
;@FMMC    M5, #0 

		.word	0XEE800310;@	FMMC MOD,#2 	;@        
        MOV     r0, r0, lsr #2		;@buf0		
        MOV     r2, r2, lsr #2		;@csptr

		.word	0XEEB00210;@	FMIR I0,R0 	;@I0: buf0
    	MOV	r3, r0
    	ADD     r3,r3,#1
		.word	0XEEB06211;@	FMIR I1,R3 	;@I1: buf0+1

   	ADD     r3, r0, r1;@
        SUB     r3, r3, #1;@		;@buf0 + num - 1

		.word	0XEEB06212;@	FMIR I2,R3 	;@			;@I2: buf1 
        SUB     r3, r3, #1;@
		.word	0XEEB06213;@	FMIR I3,R3 	;@			;@I3: buf1 - 1         

		.word	0XEEB04214;@	FMIR I4,R2 	;@I4: csptr
        ADD     r2, r2, #1;@
		.word	0XEEB04215;@	FMIR I5,R2 	;@I5: csptr+1
        ADD     r2, r2, #1;@
		.word	0XEEB04216;@	FMIR I6,R2 	;@I6: csptr+2
        ADD     r2, r2, #1;@
		.word	0XEEB04217;@	FMIR I7,R2 	;@I7: csptr+3



		.word	0XEE800338;@	FMMC MOD,#7 	;@

;@        ADR	r4, LOOP_SHIFT_START
;@        ADR     r5, LOOP_SHIFT_END
;@        FLOOP	r4, r5
;@        FLCNT 	r1			;@num

;@LOOP_SHIFT_START        
;@	FLR	r4, [I5],M5
;@	MOV     r4, r4, asr #2;@
;@LOOP_SHIFT_END
;@        FSR	r4, [I5],M4		;@FSR: MOD bit 0 must set to 1, source register (right justified), get LSB 24-bit 
;@FSR: MOD bit 0 must set to 0, source register (left justified),  get MSB 24-bit
;@FSA: always "source register (right justified), get LSB 24-bit", don't be affected by MOD 
        ADR	r4, LOOP_MDCT_Second_START
        ADR     r5, LOOP_MDCT_Second_END
		.word	0XEEE082A0;@	FLOOP R4,R5 	
        MOV 	r1, r1, lsr #2;@
		.word	0XEEE22200;@	FLCNT R1 	;@num>>2        

		.word	0XEE01C108;@	FLR2 R0,R1,[I0,I4],M0,M6 	
LOOP_MDCT_Second_START:
;@MOV     r8, #0;@ 

;@I4: cosa	I0: tr1 = *(buf0 + 0);@
;@I5: sina	I1: ti1 = *(buf0 + 1);@	
;@I6: cosb	I2: ti2 = *(buf1 + 0);@
;@I7: sinb	I3: tr2 = *(buf1 - 1);@	

;@*buf0++ = MUL24_HIGH(cosa, tr1) + MUL24_HIGH(sina, ti1);@
;@*buf0++ = MUL24_HIGH(sinb, tr2) - MUL24_HIGH(cosb, ti2);@	





;@FLR2	R0,R1 [I0,I4],M1,M6			;@R0:tr1 = *(buf0 + 0)
;@R1:cosa-->cosb 

;@ cosa: R1 
;@ tr1:  R0  							
;@*buf0++ = MUL24_HIGH(cosa, tr1) + MUL24_HIGH(sina, ti1);@ 

		.word	0XEE820149;@	FSMPL2 ACC0,R0,R1,R2,R3,[I1,I5],M0,M6 	;@MUL24_HIGH(cosa, tr1)
;@R2:ti1 = *(buf0 + 1)
;@R3:sina

;@ ti1:  R2 
;@ sina: R3 
;@MOV     r8, #0;@

		.word	0XEE02418A;@	FSMAPL2 ACC0,R3,R2,R4,R5,[I2,I6],M0,M6 	;@MUL24_HIGH(sina, ti1)
;@R4: ti2 = *(buf1 + 0)
;@R5: cosb
;@FSA	ACC0.H,[I1],M1
;@MOV     r8, #0;@

;@*buf0++ = MUL24_HIGH(sinb, tr2) - MUL24_HIGH(cosb, ti2);@
		.word	0XEED281CB;@	FSMML2 ACC1,R4,R5,R6,R7,[I3,I7],M0,M6 	;@- MUL24_HIGH(cosb, ti2)
;@R6: tr2: *(buf1 - 1)
;@R7: sinb
;@MOV     r8, #0;@
		.word	0XEE12D0E0;@	FSMAP ACC1,R6,R7 	;@MUL24_HIGH(sinb, tr2)
;@MOV     r8, #0;@
;@FSA	ACC0.H,[I1],M1
;@FSA	ACC1.H,[I1],M1

;@R0:tr1 
;@	R1:cosa
;@R2:ti1
;@	R3:sina
;@R4:ti2
;@	R5:cosb
;@R6:tr2
;@	R7:sinb

;@MOV     r8, #0;@
;@*buf1-- = MUL24_HIGH(sina(R3), tr1(R0)) - MUL24_HIGH(cosa(R1), ti1(R2));@
;@ti2:  R0
;@cosb: R1
		.word	0XEE061230;@	FSMPSA ACC0,R0,R3,ACC0.h,[I0],M2 	
;@MOV     r8, #0;@
;@tr2:  R2
;@sinb: R3
		.word	0XEE422271;@	FSMAMSA ACC0,R1,R2,ACC1.h,[I1],M2 	
;@MOV     r8, #0;@
;@FSA	ACC0.H,[I3],M3

;@*buf1-- = MUL24_HIGH(cosb(R5), tr2(R6)) + MUL24_HIGH(sinb(R7), ti2(R4));@
		.word	0XEE02B23A;@	FSMPSA ACC0,R5,R6,ACC0.h,[I2],M3 	
;@MOV     r8, #0;@
		.word	0XEE068108;@	FSMAPL2 ACC0,R4,R7,R0,R1,[I0,I4],M0,M6 	
;@MOV     r8, #0;@
LOOP_MDCT_Second_END:
		.word	0XEE03C23B;@	FSA ACC0.h,[I3],M3 	
		.word	0XEE800330;@	FMMC MOD,#6 	;@
        LDMFD    r13!,{r4-r11,pc}



;@*****************************************************************
Radix8FFTExd_24bit:
        STMFD    r13!,{r4-r11,r14}

;@temp_buf  = {r4,i4,r5,i5 r6,i6,r7,i7}
;@M1 = 1
;@M2 = 2
;@M5 = 0

;@I2(store (REAL)) 	I4(get) = temp_buf
;@I3(store (IMAGE))	   	= temp_buf + 1

;@I5: SQRT1_2_24bit						
;@R14--> buf
	MOV	R14, R0

;@FMMC	M1, #1
		.word	0XEE800312;@	FMMC M2,#2 	
		.word	0XEEFFF3D3;@	FMMC M3,#fffffffa 	
		.word	0XEE800305;@	FMMC M5,#0 	
		.word	0XEEFFF3CE;@	FMMC M6,#fffffff9 	

		.word	0XEE800310;@	FMMC MOD,#2 	
	MOV	R2, R2, LSR #2
	MOV     R3, R3, LSR #2
		.word	0XEEB06215;@	FMIR I5,R3 	
		.word	0XEEB04212;@	FMIR I2,R2 	
		.word	0XEEB04214;@	FMIR I4,R2 	
	ADD     R3, R2, #1
		.word	0XEEB06213;@	FMIR I3,R3 	

		.word	0XEE800338;@	FMMC MOD,#7 	

        ADR	r4, LOOP_Radix8FFTExd_24bit_START
        ADR     r5, LOOP_Radix8FFTExd_24bit_END
		.word	0XEEE082A0;@	FLOOP R4,R5 	
		.word	0XEEE22200;@	FLCNT R1 	;@num	

LOOP_Radix8FFTExd_24bit_START:
;@r0 = buf[0] + buf[2];@
	LDR 	R0,[R14,#0]		;@R0: [buf0]
	LDR 	R2,[R14,#8]		;@R2: [buf2]
	ADD 	R8, R0, R2		;@r0

;@r2 = buf[4] + buf[6];@
	LDR 	R4,[R14, #16]		;@R4: [buf4]						
	LDR 	R6,[R14, #24]		;@R6: [buf6]  
	ADD     R9, R4, R6		;@r2

;@r4 = (r0 + r2) >> 1;@
	ADD	R10, R8, R9
	MOV	R10, R10, ASR #1
		.word	0XEEF94212;@	FSR R10,[I2],M2 	;@0->2
;@r5 = (r0 - r2) >> 1;@
	SUB	R10, R8, R9 
	MOV	R10, R10, ASR #1	
		.word	0XEEF94212;@	FSR R10,[I2],M2 	;@2->4				

;@i0 = buf[1] + buf[3];@
	LDR 	R1,[R14, #4]		;@R1: [buf1]
	LDR 	R3,[R14, #12]		;@R3: [buf3]  						
	ADD 	R8, R1, R3

;@i2 = buf[5] + buf[7];@
	LDR 	R5,[R14, #20]		;@R5: [buf5]						
	LDR 	R7,[R14, #28]		;@R7: [buf7]  		
	ADD 	R9, R5, R7
;@i4 = (i0 + i2) >> 1;@
	ADD	R10, R8, R9
	MOV	R10, R10, ASR #1
		.word	0XEEF94213;@	FSR R10,[I3],M2 	;@1->3		
;@i5 = (i0 - i2) >> 1;@
	SUB	R10, R8, R9
	MOV	R10, R10, ASR #1
		.word	0XEEF94213;@	FSR R10,[I3],M2 	;@3->5						

;@r1 = buf[0] - buf[2];@
	SUB	R8, R0, R2
;@i3 = buf[5] - buf[7];@
	SUB	R9, R5, R7
;@r6 = (r1 - i3) >> 1;@
	SUB	R10, R8, R9
	MOV	R10, R10, ASR #1
		.word	0XEEF94212;@	FSR R10,[I2],M2 	;@4->6						
;@r7 = (r1 + i3) >> 1;@
	ADD	R10, R8, R9
	MOV	R10, R10, ASR #1
		.word	0XEEF9421A;@	FSR R10,[I2],M3 	;@6->0

;@i1 = buf[1] - buf[3];@
	SUB	R8, R1, R3
;@r3 = buf[4] - buf[6];@
	SUB	R9, R4, R6

;@i6 = (i1 + r3) >> 1;@
	ADD	R10, R8, R9
	MOV	R10, R10, ASR #1
		.word	0XEEF94213;@	FSR R10,[I3],M2 	;@5->7	
;@i7 = (i1 - r3) >> 1;@
	SUB	R10, R8, R9
	MOV	R10, R10, ASR #1
		.word	0XEEF9421B;@	FSR R10,[I3],M3 	;@7->1	
;@--------------------------------------------------------------------------------------						

;@r0 = buf[ 8] + buf[10]
	LDR 	R4,[R14, #32]		;@R4: [buf8]
	LDR 	R6,[R14, #40]		;@R6: [buf10]
;@r1(ACC0.H) = buf[ 8] - buf[10]
	SUB	R1, R4, R6						
;@FSR	R1, [I0],M1			;@r1(A)
		.word	0XEEF02230;@	FMAR ACC0.h,R1 	
	ADD 	R1, R4, R6			;@r0(R1)		

;@r2 = buf[12] + buf[14];@
	LDR 	R8,[R14, #48]		;@R8: [buf12]
	LDR 	R10,[R14, #56]		;@R10:[buf14]
	ADD 	R3, R8, R10			;@r2(R3)		

;@t0(R0) = (r0(R1) + r2(R3)) >> 1;@
	ADD	R0, R1, R3
	MOV	R0, R0, ASR #1		

;@t2(R2) = (r0(R1) - r2(R3)) >> 1;@
	SUB	R2, R1, R3
	MOV	R2, R2, ASR #1

;@i0(R4) = buf[ 9] + buf[11];@
	LDR 	R5,[R14, #36]		;@R5: [buf9]
	LDR 	R7,[R14, #44]		;@R7: [buf11]  						
	ADD 	R4, R5, R7		
;@i2(R6) = buf[13] + buf[15];@
	LDR 	R9,[R14, #52]		;@R9: [buf13]
	LDR 	R11,[R14, #60]		;@R11: [buf15]  						
	ADD 	R6, R9, R11		
;@t1(R1) = (i0 + i2) >> 1;@
	ADD	R1, R4, R6
	MOV	R1, R1, ASR #1			
;@t3(R3) = (i0 - i2) >> 1;@		
	SUB	R3, R4, R6
	MOV	R3, R3, ASR #1
;@r1(R4)	
		.word	0XEEF00224;@	FMRA R4,ACC0.h 	

;@i1(R5) = buf[ 9](R5) - buf[11](R7);@
	SUB	R5, R5, R7						
;@FSR	R4, [I0],M1

;@i3(R6) = buf[13](R9) - buf[15](R11);@
	SUB 	R6, R9, R11
;@FSR	R4, [I0],M1

;@r3(R7) = buf[12](R8) - buf[14](R10);@
	SUB 	R7, R8, R10
;@FSR	R4, [I0],M1

;@temp_buf  = {r4,i4,r5,i5 r6,i6,r7,i7}
;@I4(real)  = temp_buf;@
;@t0,t1,t2,t3: R0, R1, R2, R3
;@R9=temp_buf[]
;@R8=result
;@buf[ 0] = r4(R4) + t0(R0);@
		.word	0XEE01D024;@	FLR R9,[I4],M4 	;@0->1
	ADD	R8, R9, R0;@
	STR	R8,[R14,#0];@
;@buf[ 8] = r4 - t0;@	
	SUB	R8, R9, R0;@
	STR	R8,[R14,#32];@								
;@buf[ 1] = i4(R5) + t1(R1);@
		.word	0XEE01D024;@	FLR R9,[I4],M4 	;@1->2
	ADD	R8, R9, R1;@
	STR	R8,[R14,#4];@
;@buf[ 9] = i4 - t1;@
	SUB	R8, R9, R1;@
	STR	R8,[R14,#36];@						
;@buf[ 4] = r5(R6) + t3(R3);@
		.word	0XEE01D024;@	FLR R9,[I4],M4 	;@2->3
	ADD	R8, R9, R3
	STR	R8,[R14,#16]
;@buf[12] = r5 - t3;@
	SUB	R8, R9, R3
	STR	R8,[R14,#48]
;@buf[13] = i5 + t2;@
		.word	0XEE01D024;@	FLR R9,[I4],M4 	;@3->4
	ADD	R8, R9, R2
	STR	R8,[R14,#52]		
;@buf[ 5] = i5(R7) - t2(R2);@
	SUB	R8, R9, R2
	STR	R8,[R14,#20]		


;@r0(R8) = r1(R4) - i3(R6);@
	SUB	R8, R4, R6
;@r2(R9) = r1 + i3;@
	ADD     R9, R4, R6								
;@i0(R10) = i1(R5) + r3(R7);@
	ADD	R10, R5, R7
;@i2(R11) = i1 - r3;@
	SUB	R11, R5, R7

		.word	0XEE01C0ED;@	FLR R7,[I5],M5 	;@SQRT1_2_24bit
;@t0 = MUL24_HIGH(SQRT1_2_24bit, r0(R8) - i0(R10));@	
		.word	0XEE82E084;@	FSMPL ACC0,R7,R8,R4,[I4],M4 	;@r6	4->5
		.word	0XEE46E0A4;@	FSMAML ACC0,R7,R10,R5,[I4],M4 	;@i6	5->6
		.word	0XEEF00220;@	FMRA R0,ACC0.h 	
;@t1 = MUL24_HIGH(SQRT1_2_24bit, r0(R8) + i0(R10));@
		.word	0XEE82E0C4;@	FSMPL ACC0,R7,R8,R6,[I4],M4 	;@r7	6->7
		.word	0XEE06F014;@	FSMAPL ACC0,R7,R10,R8,[I4],M6 	;@i7	7->0
		.word	0XEEF00221;@	FMRA R1,ACC0.h 	
;@t2 = MUL24_HIGH(SQRT1_2_24bit, r2(R9) - i2(R11));@ //Q7.17 foramt
		.word	0XEE84F0E0;@	FSMP ACC0,R7,R9 	
		.word	0XEE48F0E0;@	FSMAM ACC0,R7,R11 	
		.word	0XEEF00222;@	FMRA R2,ACC0.h 	
;@t3 = MUL24_HIGH(SQRT1_2_24bit, r2(R9) + i2(R11));@  
		.word	0XEE84F0E0;@	FSMP ACC0,R7,R9 	
		.word	0XEE08F0E0;@	FSMAP ACC0,R7,R11 	
		.word	0XEEF00223;@	FMRA R3,ACC0.h 	
;@buf[ 6] = r6(R4) - t0(R0);@
;@buf[14] = r6(R4) + t0(R0);@
	SUB	R7,R4,R0
	STR	R7,[R14,#24]
	ADD	R7,R4,R0
	STR	R7,[R14,#56]		

;@buf[ 7] = i6(R5) - t1(R1);@
;@buf[15] = i6(R5) + t1(R1);@
	SUB	R7,R5,R1
	STR	R7,[R14,#28]
	ADD	R7,R5,R1
	STR	R7,[R14,#60]
;@buf[10] = r7(R6) - t3(R3);@
;@buf[ 2] = r7(R6) + t3(R3);@
	SUB	R7,R6,R3
	STR	R7,[R14,#40]
	ADD	R7,R6,R3
	STR	R7,[R14,#8]		
;@buf[ 3] = i7(R8) - t2(R2);@
;@buf[11] = i7(R8) + t2(R2);@
	SUB	R7,R8,R2
	STR	R7,[R14,#12]
	ADD	R7,R8,R2
	STR	R7,[R14,#44]
LOOP_Radix8FFTExd_24bit_END:
	ADD	R14, R14, #0X40		
	LDMFD    r13!,{r4-r11,pc}					        	     

.end