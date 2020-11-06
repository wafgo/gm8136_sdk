#ifndef __BOARD_H__
#define __BOARD_H__

//#define NULL    0
#define BIT0    0x00000001L
#define BIT1    0x00000002L
#define BIT2    0x00000004L
#define BIT3    0x00000008L
#define BIT4    0x00000010L
#define BIT5    0x00000020L
#define BIT6    0x00000040L
#define BIT7    0x00000080L
#define BIT8	0x00000100L
#define BIT9	0x00000200L
#define BIT10	0x00000400L
#define BIT11	0x00000800L
#define BIT12	0x00001000L
#define BIT13	0x00002000L
#define BIT14	0x00004000L
#define BIT15	0x00008000L	
#define BIT16	0x00010000L
#define BIT17	0x00020000L
#define BIT18	0x00040000L
#define BIT19	0x00080000L
#define BIT20	0x00100000L
#define BIT21	0x00200000L
#define BIT22	0x00400000L
#define BIT23	0x00800000L	


		
#define SRAM0_BASE              0x92000000
#define SRAM0_SIZE              0x6000  /* 24K */
#define SRAM1_BASE              0x92008000
#define SRAM1_SIZE              0x2000

#define REG_AHBC_1_BASE         0x92B00000
#define REG_AHBC_BASE(N)        (0x92900000 + 0x200000 * (N))
#define AHBC_SLAVE(N)           (0x00 + 0x04 * (N))
#define AHBC_REMAP              0x88
#define AXIC_REMAP              0x130

//#define REG_SMC_BASE            0x92100000
//#define SMC_CFG(N)              (0x00 + 0x08 * (N))
//#define SMC_TIMING(N)           (0x04 + 0x08 * (N))

#define REG_DMA_BASE            0x92600000
//#define REG_NAND023_BASE        0x93700000
//#define NAND023_DATA_PORT       0x93800000  
#define REG_UART0_BASE          0x90700000

#define REG_PMU_BASE            0x90C00000
#define PMU_AHBCLK              0xB4

#define REG_AXIC_BASE           0x9A600000

#define REG_DDR0_BASE           0x9A100000

#define BASE_GPIO0              0x91000000
#define REG_SPI020_BASE         0x92300000

#ifndef __ASSEMBLY__

typedef enum 
{
    FLASH_NOR = 0,
    FLASH_NAND,
    FLASH_SPI_NAND
} FLASH_TYPE_T;

/* type definition */
typedef unsigned int UINT32;
typedef unsigned int INT32U;
typedef int INT32;
typedef int INT32S;
typedef unsigned short UINT16;
typedef unsigned short INT16U;
typedef short INT16;
typedef unsigned char UINT8;
typedef unsigned char INT8U;
typedef char INT8;
typedef char INT8S;
typedef unsigned char BOOL;

unsigned int read_reg(unsigned int reg);
void write_reg(unsigned int reg, unsigned int value);
void sal_memset(void *pbuf, unsigned char val, int len);
int sal_memcmp(void *sbuf, void *dbuf, int len);
int sal_memcpy(void *sbuf, void *dbuf, int len);

#endif /* __ASSEMBLY__ */


#endif
