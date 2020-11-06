    		EXPORT 	SingLineQuant0
    		AREA QUANSUB0, CODE, READONLY
SingLineQuant0
      STMFD    r13!,{r4-r11,r14}
      cmp  r1, #0
      moveq r0, #0
      beq  exit_SingLineQuant0
		DCD	0XEEE42280;	FCLZ R4,R1 	;e = norm_l(absSpectrum);
      sub  r4, r4, #1
      MOV  r2, r2, LSR #2		;mTab_3_4_ptr
      MOV  r3, r3, LSR #2      		;pow2tominusNover16_ptr
		DCD	0XEEB04214;	FMIR I4,R2 	
		DCD	0XEEB06215;	FMIR I5,R3 	
		DCD	0XEE800330;	FMMC MOD,#6 	
      mov  r8, r1, lsl r4 		;x = absSpectrum << e;
      mov  r8, r8, lsl #2		;x >> (INT_BITS-2-MANT_DIGITS)
      mov  r8, r8, lsr #23		;x >> (INT_BITS-2-MANT_DIGITS)
		DCD	0XEED10215;	FMMR M5,R8 	;x
		DCD	0XEE01D0EC;	FUPI I4,M5 	
		DCD	0XEE01D004;	FLR R8,[I4],M4 	;x

      add  r5, r0, r4, lsl #2		;minusFinalExp = (e << 2) + gain
      add  r5, r5, r5, lsl #1		;minusFinalExp = (minusFinalExp << 1) + minusFinalExp
      add  r5, r5, #0x1f0		;minusFinalExp = minusFinalExp + ((INT_BITS-1) << 4)
      mov  r6, r5, lsr #4		;finalShift = minusFinalExp >> 4
      cmp  r6, #0x20			;if (finalShift < INT_BITS)
      bge  exit_SingLineQuant0
      and  r5, r5, #0xf
		DCD	0XEED0A215;	FMMR M5,R5 	
		DCD	0XEE01D0ED;	FUPI I5,M5 	
		DCD	0XEE01C0A5;	FLR R5,[I5],M4 	;pow2tominusNover16_ptr[minusFinalExp & 15] 
		DCD	0XEE86B0E0;	FSMP ACC0,R5,R8 	;xxxx xxxx 0000 0000 0000 0000 0000 0000	      
		DCD	0XEEF00228;	FMRA R8,ACC0.h 	;
      mov  r8, r8,lsl #8;

      LDR  r2, = 0x33e425af;
      RSB  r3, r6, #32
      subs r6, r6, #1
      add  r8, r8, r2, lsr r3		;x += XROUND >> (INT_BITS - finalShift);    
      rsblt  r6, r6, #0
      movlt  r8, r8, lsl r6
      movge  r8, r8, asr r6       

      LDR    r1, = 0x7fff
      MOV    r2, r8, ASR#15
      TEQ    r2, r8, ASR#31
      EORNE  r0, r1, r8, ASR#31
      MOVEQ  r0, r8 
exit_SingLineQuant0
      LDMFD    r13!,{r4-r11,pc}


    		EXPORT 	iQuantL1_s
    		AREA QUANSUB1, CODE, READONLY
iQuantL1_s
      STMFD    r13!,{r4-r11,r14}      

      ldr  r5,[r1]		;accu = *quantSpectrum;
      cmp  r5, #0;
      beq  exit_iQuantL1_s

		DCD	0XEE0A734C;	FMIC I4,#14e9 	;mTab_4_3
		DCD	0XEE0B734D;	FMIC I5,#16e9 	;specExpMantTableComb_enc
		DCD	0XEE0B9396;	FMIC I6,#1732 	;specExpTableComb_enc
		DCD	0XEE800330;	FMMC MOD,#6 	

      mov  r10, r5, lsl #8
		DCD	0XEEE55260;	FCLZ R11,R10 	;ex = norm_l(accu<<8);
      ldr  r1, =0x1ff      
      sub  r11, r11, #1
      mov  r5, r5, lsl r11	;accu(r5) = accu << ex;
      rsb  r6, r11, #23;	;specExp(r6) = 24 -1 - ex;

;mov  r5, r5, lsl #2       ;-2
      mov  r10, r5, lsr #13  	;15(24-MANT_DIGITS(9)),tabIndex = (accu >> (24-2-MANT_DIGITS)) & (~MANT_SIZE);
      and  r10, r10, r1

		DCD	0XEED14215;	FMMR M5,R10 	;tabIndex
		DCD	0XEE01D0EC;	FUPI I4,M5 	
		DCD	0XEE01C024;	FLR R1,[I4],M4 	;s(r10) = mTab_4_3_ptr[tabIndex]


      and r8, r0, #3 		;(r8)iquantizermod = gain & 3;
      mov r9, r0, asr #2	;(r9)iquantizershift = gain >> 2;     

      mov r11, r8, lsl #4
      sub r11, r11, r8
      sub r11, r11, r8		;iquantizermod*14  	
      add r11, r11, r6		;iquantizermod*14+specExp

		DCD	0XEED16215;	FMMR M5,R11 	;tabIndex
		DCD	0XEE01D0ED;	FUPI I5,M5 	
		DCD	0XEE01C005;	FLR R0,[I5],M4 	;t(r0) = *(specExpMantTableComb_enc_ptr+iquantizermod*14+specExp);


		DCD	0XEE8210E0;	FSMP ACC0,R0,R1 	
		DCD	0XEEF00220;	FMRA R0,ACC0.h 	
		DCD	0XEE800310;	FMMC MOD,#2 	
		DCD	0XEEF00201;	FMRA R1,ACC0.l 	
		DCD	0XEE800330;	FMMC MOD,#6 	
      mov  r0, r0, lsl #8
      mov  r1, r1, lsr #16
      orr  r0, r0, r1  		;accu(r0) 


		DCD	0XEE01D0EE;	FUPI I6,M5 	
		DCD	0XEE01C0C6;	FLR R6,[I6],M4 	;specExp(r6)

      add  r6, r6, r9		;specExp(r6) += iquantizershift

      ADDS   r6, r6, #1
      RSBLT  r6, r6, #0
      MOVLT  r5, r0, asr r6
      MOVGE  r5, r0, lsl r6
exit_iQuantL1_s
      str r5, [r2]

      LDMFD    r13!,{r4-r11,pc}      

