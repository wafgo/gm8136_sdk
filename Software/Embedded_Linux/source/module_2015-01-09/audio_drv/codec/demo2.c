#include <stdio.h>
#include <math.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/fie702x.h>

#define LINUX

static __inline int fixmul(int a, int b);

__inline int fixmul(int a, int b)
{
	int acc0, acc1, tmp;
	
	printf(" fixmul : a=%d   b=%d  \n",a,b);
	
#ifdef LINUX
	asm volatile("smull %[result0], %[result1], %[in0], %[in1]" : [result0] "=r" (acc0), [result1] "=r" (acc1) : [in0] "r" (a), [in1] "r" (b));
	//tmp = acc1<<1;
	tmp = acc0;
	return tmp;
#else
	__asm {
		SMULL	acc0, acc1, a, b
	}
	acc1 <<= 1;
	return acc1;
#endif
}

int main(void)
{
    unsigned int regval;
    float f1, f2;
    int select;
    f1 = 3.14;
    f2 = f1 * 0.1324345;
    int a=10,b=30,c; 
    
    printf("=================================\n");
    printf(" PLLOUT = %d\n", HAL_PMU_GET_PLLOUT());
    printf(" PLLOUT_DIV2 = %d\n", HAL_PMU_GET_PLLOUT2());
    printf(" HCLK = %d\n", HAL_PMU_GET_HCLK());
    printf(" PCLK = %d\n", HAL_PMU_GET_PCLK());
    printf("=================================\n");
    
    HAL_READ_UINT32(FIE702x_GPIOA_BASE + _GPIO_DIRC, regval);
    printf("FIE702x_GPIOA_BASE + _GPIO_DIRC = 0x%x\n", regval);
    
    HAL_READ_UINT32(FIE702x_GPIOA_BASE + _GPIO_DATAOUT, regval);
    printf("FIE702x_GPIOA_BASE + _GPIO_DATAOUT = 0x%x\n", regval);
    
    printf("f1 = %f, f2 = %f\n", f1, f2);
	  printf("Hello World!\n\n");
			
	//while (1) {
		  printf(" a=%d   b=%d  \n",a,b);
	    printf(" input >> :\n");
	    scanf("%x", &select);
	    printf(" select = %d\n", select);
	    c=fixmul(a,b);
	    printf(" c=%d \n",c);
	//}
	
	return 0;
}
