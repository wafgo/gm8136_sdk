
#define LINUX

void init_PMN(void)
{
  unsigned int t2 = 0, PMNC, ZERO;
  PMNC = 0x107d;
  ZERO = 0;

 #ifdef LINUX
 asm volatile(
		"LDR	%[t2], [%[t1]], #4	\n"
		"MCR	p15, 0, %[t2], c15, c12, 0 \n"
		"MCR  p15, 0, %[ZERO], c15, c12, 1  "
		 :
		 :[t1]"r"(0x40000),[t2]"r"(t2),[ZERO]"r"(ZERO)
		 :"cc"
     );
 #else
  __asm{

    MOV	t1, #0x40000
    //STR PMNC, [t1], #0
		LDR	t2, [t1], #4				 // Load PMN condition
		MCR	p15, 0, t2, c15, c12, 0	     // Enable PMN
		MCR p15, 0, ZERO, c15, c12, 1
  }
 #endif

}



void read_PMR( unsigned int* pCCNT, unsigned int* pPMN0, unsigned int* pPMN1 )
{
	unsigned int CCNT = 0, PMN0 = 0, PMN1 = 0;

#ifdef LINUX
 asm volatile(
    "MRC	p15, 0, %[CCNT], c15, c12, 1	\n"
		"MRC	p15, 0, %[PMN0], c15, c12, 2	\n"
		"MRC	p15, 0, %[PMN1], c15, c12, 3	"
		:
		:[CCNT]"r"(CCNT),[PMN0]"r"(PMN0),[PMN1]"r"(PMN1)
		:"cc"
 );
#else
	__asm
	{
	  //MOV DIS_PMN, #0
		//MCR	p15, 0, DIS_PMN, c15, c12, 0	// Disable PMN
		MRC	p15, 0, CCNT, c15, c12, 1		// Read CCNT
		MRC	p15, 0, PMN0, c15, c12, 2		// Read PMN0
		MRC	p15, 0, PMN1, c15, c12, 3		// Read PMN1

		//STR	CCNT, [ADDR], #4
		//STR	PMN0, [ADDR], #4
		//STR	PMN1, [ADDR], #4
	}
#endif

	*pCCNT = CCNT;
	*pPMN0 = PMN0;
 	*pPMN1 = PMN1;
}
