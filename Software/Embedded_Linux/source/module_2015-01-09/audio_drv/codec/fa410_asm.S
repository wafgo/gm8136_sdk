.global norm_l
.global norm_s
.text	 
.align 0


norm_l:
	;@STMFD   r13!,{r4-r11,r14}
		.word	0XEEE40200;@	FCLZ R0,R0 	;
	SUB     r0, r0, #1;
	MOV   pc, r14
	
norm_s:
		.word	0XEEE40200;@	FCLZ R0,R0 	;
	SUB     r0, r0, #17;
	MOV   pc, r14	
.end
