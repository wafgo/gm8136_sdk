#ifndef FA410_H_INCLUDED
#define FA410_H_INCLUDED

#define EXRAM_BASE      0x0             
#define EXRAM_LENGTH    512*1024        //bytes

#define IRAM_BASE       0x8000000        //512*2*1024
#define IRAM_LENGTH     16*1024*4         //24-bit words

#define IRAM_COEFF_BASE         IRAM_BASE        
#define IRAM_COEFF_LENGTH       8*1024*4         //24-bit words
#define IRAM_DATA_BASE          (IRAM_BASE + IRAM_COEFF_LENGTH)        
#define IRAM_DATA_LENGTH        8*1024*4         //24-bit words

//#define CosSinTab1024Len	1024*4
//#define CosSinTab128Len		128*4
#define twidTab512Len		1008*4
#define	twidTab64Len		120*4
#define	LongWindowKBDLen	1024*4	//divide 32-bit to two 16-bit
#define	ShortWindowSineLen	256*4	//divide 32-bit to two 16-bit
#define	LongWindowKBDExdLen	1024*4	//divide 32-bit to two 16-bit
#define	ShortWindowSineExdLen	256*4	//divide 32-bit to two 16-bit
#define	CosSinTab1024ExdLen	1024*4
#define	CosSinTab128ExdLen	128*4
#define SQRT1_2Len		1*4		//
#define mTab_3_4Len		512*4		//5353 (mTab_4_3Offset)
#define mTab_4_3Len		512*4		//5865 (specExpMantTableComb_encOffset)
#define specExpMantTableComb_encLen 56*4        //5921 (pow2tominusNover16Offset)
#define pow2tominusNover16Len	17*4		//5938 (specExpTableComb_encOffset)
#define specExpTableComb_encLen 56*4 		//5994

			//sum:  5992 24-bit word	

//#define CosSinTab1024Offset	0	
//#define CosSinTab128Offset	0+CosSinTab1024Len
#define twidTab512Offset		0
#define	twidTab64Offset			0+twidTab512Len
#define	LongWindowKBDOffset 		0+twidTab512Len+twidTab64Len
#define	ShortWindowSineOffset 		0+twidTab512Len+twidTab64Len+LongWindowKBDLen
#define	LongWindowKBDExdOffset  	0+twidTab512Len+twidTab64Len+LongWindowKBDLen+ShortWindowSineLen
#define	ShortWindowSineExdOffset	0+twidTab512Len+twidTab64Len+LongWindowKBDLen+ShortWindowSineLen+LongWindowKBDExdLen
#define CosSinTabExd1024Offset		0+twidTab512Len+twidTab64Len+LongWindowKBDLen+ShortWindowSineLen+LongWindowKBDExdLen+ShortWindowSineExdLen	
#define CosSinTabExd128Offset		0+twidTab512Len+twidTab64Len+LongWindowKBDLen+ShortWindowSineLen+LongWindowKBDExdLen+ShortWindowSineExdLen+CosSinTab1024ExdLen
#define SQRT1_2Offset			0+twidTab512Len+twidTab64Len+LongWindowKBDLen+ShortWindowSineLen+LongWindowKBDExdLen+ShortWindowSineExdLen+CosSinTab1024ExdLen+CosSinTab128ExdLen
#define mTab_3_4Offset		        0+twidTab512Len+twidTab64Len+LongWindowKBDLen+ShortWindowSineLen+LongWindowKBDExdLen+ShortWindowSineExdLen+CosSinTab1024ExdLen+CosSinTab128ExdLen+SQRT1_2Len
#define mTab_4_3Offset		        0+twidTab512Len+twidTab64Len+LongWindowKBDLen+ShortWindowSineLen+LongWindowKBDExdLen+ShortWindowSineExdLen+CosSinTab1024ExdLen+CosSinTab128ExdLen+SQRT1_2Len+mTab_3_4Len
#define specExpMantTableComb_encOffset  0+twidTab512Len+twidTab64Len+LongWindowKBDLen+ShortWindowSineLen+LongWindowKBDExdLen+ShortWindowSineExdLen+CosSinTab1024ExdLen+CosSinTab128ExdLen+SQRT1_2Len+mTab_3_4Len+mTab_4_3Len
#define pow2tominusNover16Offset        0+twidTab512Len+twidTab64Len+LongWindowKBDLen+ShortWindowSineLen+LongWindowKBDExdLen+ShortWindowSineExdLen+CosSinTab1024ExdLen+CosSinTab128ExdLen+SQRT1_2Len+mTab_3_4Len+mTab_4_3Len+specExpMantTableComb_encLen 
#define specExpTableComb_encOffset      0+twidTab512Len+twidTab64Len+LongWindowKBDLen+ShortWindowSineLen+LongWindowKBDExdLen+ShortWindowSineExdLen+CosSinTab1024ExdLen+CosSinTab128ExdLen+SQRT1_2Len+mTab_3_4Len+mTab_4_3Len+specExpMantTableComb_encLen+pow2tominusNover16Len 


#define realOutLen		2048*4
#define temp_bufLen		8*4

#define realOutOffset		0
#define temp_bufOffset		0+realOutLen


#define SQRT1_2_24bit (0x5a82799a+0x80)>>8	/* sqrt(1/2) in Q23 */

int norm_l(int x);
int norm_s(int var1);
#endif



