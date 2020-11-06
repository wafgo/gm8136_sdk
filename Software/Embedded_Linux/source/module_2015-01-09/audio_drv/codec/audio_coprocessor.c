
#include "fa410.h"
#include "aac_table.h"

extern int init_FA410(void);
//extern void init_PMN(void);

//------ fa410 table pointer ------------------------------------------------
//int *CosSinTab1024_24bit_ptr = 	(int *)(IRAM_COEFF_BASE+CosSinTab1024Offset);
//int *CosSinTab128_24bit_ptr =  	(int *)(IRAM_COEFF_BASE+CosSinTab128Offset);
int *twidTab512_24bit_ptr =    	(int *)(IRAM_COEFF_BASE+twidTab512Offset);
int *twidTab64_24bit_ptr =     	(int *)(IRAM_COEFF_BASE+twidTab64Offset);
int *LongWindowKBD_24bit_ptr = 	(int *)(IRAM_COEFF_BASE+LongWindowKBDOffset);
int *ShortWindowSine_24bit_ptr =(int *)(IRAM_COEFF_BASE+ShortWindowSineOffset);
int *LongWindowKBDExd_24bit_ptr = 	(int *)(IRAM_COEFF_BASE+LongWindowKBDExdOffset);
int *ShortWindowSineExd_24bit_ptr =(int *)(IRAM_COEFF_BASE+ShortWindowSineExdOffset);
int *CosSinTabExd1024_24bit_ptr=(int *)(IRAM_COEFF_BASE+CosSinTabExd1024Offset);
int *CosSinTabExd128_24bit_ptr= (int *)(IRAM_COEFF_BASE+CosSinTabExd128Offset);
int *SQRT1_2_24bit_ptr =        (int *)(IRAM_COEFF_BASE+SQRT1_2Offset);
int *mTab_3_4_ptr =        	(int *)(IRAM_COEFF_BASE+mTab_3_4Offset);
int *mTab_4_3_ptr =        	(int *)(IRAM_COEFF_BASE+mTab_4_3Offset);
int *specExpMantTableComb_enc_ptr = (int *)(IRAM_COEFF_BASE+specExpMantTableComb_encOffset);
int *pow2tominusNover16_ptr =   (int *)(IRAM_COEFF_BASE+pow2tominusNover16Offset);
int *specExpTableComb_enc_ptr = (int *)(IRAM_COEFF_BASE+specExpTableComb_encOffset);
//int *Mid_LongWin_24bit_ptr  = 	(int *)(LongWindowKBD_24bit_ptr+512);  	
//int *Mid_ShortWin_24bit_ptr = 	(int *)(ShortWindowSine_24bit_ptr+128);
//------ end ----------------------------------------------------------------


void init_fa410_coef_tbl(void)
{
	int i, j;
	int *word_ptr, *temp_ptr;
	int *wordexd_ptr, *tempexd_ptr;

	//---- init atble on XY memory ------------------------------------------------------
	//int *CosSinTab1024_24bit_ptr = 	(int *)(IRAM_COEFF_BASE+CosSinTab1024Offset);
	//int *CosSinTab128_24bit_ptr =  	(int *)(IRAM_COEFF_BASE+CosSinTab128Offset);
	//int *twidTab512_24bit_ptr =    	(int *)(IRAM_COEFF_BASE+twidTab512Offset);
	//int *twidTab64_24bit_ptr =     	(int *)(IRAM_COEFF_BASE+twidTab64Offset);
	//int *LongWindowKBD_24bit_ptr = 	(int *)(IRAM_COEFF_BASE+LongWindowKBDOffset);
	//int *ShortWindowSine_24bit_ptr =(int *)(IRAM_COEFF_BASE+ShortWindowSineOffset);
	//int *Mid_LongWin_24bit_ptr  = 	LongWindowKBD_24bit_ptr+512;  	
	//int *Mid_ShortWin_24bit_ptr = 	ShortWindowSine_24bit_ptr+128;
	//int *word_ptr, *temp_ptr;
	//------------------------------------------------------------------------------------
	// twidTab512[1008], twidTab64[120] transfer to 24-bit
	// cossintab_1024[1024], cossintab_128[128] transfer to 24-bit
	//int *CosSinTabExd1024_24bit_ptr=(int *)(IRAM_COEFF_BASE+CosSinTabExd1024Offset)
	//int *CosSinTabExd128_24bit_ptr= (int *)(IRAM_COEFF_BASE+CosSinTabExd128Offset)
	
	word_ptr = CosSinTabExd1024_24bit_ptr;
	for(i=0;i<1024; i++)
	{
		*word_ptr = cossintab_1024[i]>>8;	//Q10.22
		word_ptr++;
	}
		
	word_ptr = CosSinTabExd128_24bit_ptr;	
	for(i=0;i<128; i++)
	{
		*word_ptr = cossintab_128[i]>>8;	//Q10.22
		word_ptr++;
	}
	
	word_ptr = twidTab512_24bit_ptr;		
	for(i=0;i<1008; i++)
		*word_ptr++ = twidTab512[i]>>8;
		
	word_ptr = twidTab64_24bit_ptr;	
	for(i=0;i<120; i++)
		*word_ptr++ = twidTab64[i]>>8;

	wordexd_ptr = LongWindowKBDExd_24bit_ptr;
	tempexd_ptr = LongWindowKBDExd_24bit_ptr + 512;
	word_ptr =    LongWindowKBD_24bit_ptr;		
	temp_ptr =    LongWindowKBD_24bit_ptr+512;					
	for(i=0;i<512; i++)
	{
		*word_ptr = LongWindowKBD[i]>>16;
		*temp_ptr = LongWindowKBD[i]&0x0000ffff;
		*wordexd_ptr = (*word_ptr)<<8;
		*tempexd_ptr = (*temp_ptr)<<8;
		word_ptr++;
		temp_ptr++;
		wordexd_ptr++;
		tempexd_ptr++;	
	}

	wordexd_ptr = ShortWindowSineExd_24bit_ptr;
	tempexd_ptr = ShortWindowSineExd_24bit_ptr + 128;	
	word_ptr =    ShortWindowSine_24bit_ptr;
	temp_ptr =    ShortWindowSine_24bit_ptr+128;					
	for(i=0;i<128; i++)
	{
		*word_ptr = ShortWindowSine[i]>>16;
		*temp_ptr = ShortWindowSine[i]&0x0000ffff;
		*wordexd_ptr = (*word_ptr)<<8;
		*tempexd_ptr = (*temp_ptr)<<8;
		word_ptr++;
		temp_ptr++;
		wordexd_ptr++;
		tempexd_ptr++;			
	}
	
	*SQRT1_2_24bit_ptr = SQRT1_2_24bit;			
	
	word_ptr = mTab_3_4_ptr;
	for(i=0;i<512; i++)
		*word_ptr++ = mTab_3_4[i]>>8;
		
	word_ptr = mTab_4_3_ptr;
	for(i=0;i<512; i++)
		*word_ptr++ = mTab_4_3[i]>>8;						

	word_ptr = specExpMantTableComb_enc_ptr;
	for(i=0;i<4; i++)
	    for(j=0;j<14; j++)	
		*word_ptr++ = specExpMantTableComb_enc[i][j]>>8;
		
	word_ptr = pow2tominusNover16_ptr;
	for(i=0;i<17; i++)
	    *word_ptr++ = (int)(pow2tominusNover16[i]<<8);
	    
	word_ptr = specExpTableComb_enc_ptr;
	for(i=0;i<4; i++)
	    for(j=0;j<14; j++)	
		*word_ptr++ = (int)specExpTableComb_enc[i][j];	    

		
}

void init_audio_coprocessor(void)
{
	init_FA410();
	//init_PMN();	?
	init_fa410_coef_tbl();	
}
