/*
 *      entry.c
 *
 *      Entry point for nsboot
 *
 *      Written by:     Justin Shih <wdshih@faraday-tech.com>
 *
 *              Copyright (c) 2014 GM, ALL Rights Reserve
 */
#include "board.h"
#include "nand.h"
#include "spi020.h"
#include "kprint.h"
#include "ahb_dma.h"

#include "ddr_1333.h"
#include "ddr_1240.h"
#include "ddr_1180.h" 
#include "ddr_1140.h"
#include "ddr_1080.h"
#include "ddr_1000.h"
#include "ddr_950.h"
#include "ddr_850.h"
#include "ddr_800.h"
#include "ddr_790.h"
#include "ddr_700.h"
#include "ddr_666.h"
#include "ddr_590.h"

//#define DO_DEBUG

#define VERSION			    "v0.1"
#define LOAD_ADDR		    0x0
//#define UART_FROM_PLL1

#define SYS_CLK             30

FLASH_TYPE_T flash_type = FLASH_SPI_NAND;

extern void serial_init(void);

void hang(void)
{
    puts("### ERROR ### Please RESET the board ###\n");
    for (;;) ;
}

void __div0(void)
{
    hang();
}

#ifdef DO_DEBUG

static void hexdump(unsigned char *data, int len)
{
    int i, j;

    for (i = 0; i < len; i += 16) {
        KPrint("\n%04xh:", i);
        for (j = 0; j < 16; j++)
            KPrint(" %02X", data[i + j]);
        KPrint("   ");
        for (j = 0; j < 16; j++) {
            int c = data[i + j];
            if (c < 0x20)
                KPrint(".");
            else
                KPrint("%c", c);
        }
    }
    KPrint("\n");
}

static void hdrdump(IMGHDR * hdr)
{
    KPrint("<Image Header>\n");
    KPrint("  magic : 0x%x\n", hdr->magic);
    KPrint("  chksum: 0x%x\n", hdr->chksum);
    KPrint("  size  : 0x%x\n", hdr->size);
    KPrint("  time  : 0x%x\n", hdr->time);
    KPrint("  name  : %s\n", hdr->name);
}

void syshdrdump(SYSHDR * hdr)
{
    KPrint("<System Header>\n");
    KPrint("  signature: %s\n", hdr->signature);
    KPrint("  bootm_addr: 0x%x\n", hdr->bootm_addr);
    KPrint("  stsize: %d\n", hdr->stsize);
    KPrint("  st_0: 0x%x\n", hdr->st.setting[0]);
    KPrint("  st_1: 0x%x\n", hdr->st.setting[1]);
    KPrint("  st_2: 0x%x\n", hdr->st.setting[2]);
}

#endif

void check_flash_type(void)
{
#if 0
    flash_type = FLASH_SPI_NAND;
#else
    /* bit7 => 1:NAND, 0:SPI
     */
    //A version
    if (read_reg(REG_PMU_BASE + 0x04) & (1 << 7))
        flash_type = FLASH_SPI_NAND;
    else
        flash_type = FLASH_NOR;

#endif
}

int u32PMU_ReadPLL1CLK(void)
{
    unsigned int value, n, m;

    value = read_reg(REG_PMU_BASE + 0x30);
    n = (value >> 4) & 0x7F;
    m = (value >> 11) & 0x1F;
    value = (SYS_CLK * n) / m;
 
    return value;
}

int u32PMU_ReadPLL2CLK(void)
{
    unsigned int value, n, m;

    value = read_reg(REG_PMU_BASE + 0x34);
    n = (value >> 4) & 0x7F;
    m = (value >> 11) & 0x1F;
    value = (SYS_CLK * n) / m;

    return value;
}

void check_SIP(void)
{
	unsigned int ID = 0;

    ID = (read_reg(REG_PMU_BASE) >> 2) & 0x1F;

	switch (ID) {
	  case 0x4:case 0x10:
        write_reg(REG_PMU_BASE + 0x64, 0x1 << 30);
	    break;
	  case 0x14:
        write_reg(REG_PMU_BASE + 0x64, 0x3 << 30);
	    break;
	  default:
	    break;
	}
}

static void platform_setup(void)
{
    unsigned int reg, value;

#ifdef PLL3_594//default clock 540, Nx30/M
    /* PLL3 set 594MHz */
    reg = read_reg(REG_PMU_BASE + 0x34) & 0x0003FFFF;

    reg = reg | (5 << 27) | (99 << 20);     //PLL3 ms[31:27], PLL3 ns[26:20]
    //reg = reg | (3 << 11) | (80 << 4);      //PLL2 ms[15:11], PLL2 ns[10:4]
    write_reg(REG_PMU_BASE + 0x34, reg);
#endif
    /* UART pin mux */
    reg = read_reg(REG_PMU_BASE + 0x58) & 0x0FFFFFFF;
    write_reg(REG_PMU_BASE + 0x58, reg | (0x5 << 28));

#ifdef CONFIG_FPGA
    /* Open AXI/AHB/APB clock */
    write_reg(REG_PMU_BASE + 0xB0, 0x0);
    write_reg(REG_PMU_BASE + 0xB4, 0x0);
    write_reg(REG_PMU_BASE + 0xB8, 0x0);
    write_reg(REG_PMU_BASE + 0xBC, 0x0);
#else
    /* axic2 PWM enable */
    write_reg(REG_PMU_BASE + 0x38, 0x1);
    
    /* UART clock div */
    reg = read_reg(REG_PMU_BASE + 0x70) & 0xFFC0FFFF;
#ifdef UART_FROM_PLL1
    write_reg(REG_PMU_BASE + 0x70, reg | (0x9 << 16));

    reg = read_reg(REG_PMU_BASE + 0x30) & 0xFFFFF80F;
    reg = reg | (40 << 4);
    write_reg(REG_PMU_BASE + 0x30, reg);
    
    /* UART clock source */
    reg = read_reg(REG_PMU_BASE + 0x28) | (0x1 << 3);
    write_reg(REG_PMU_BASE + 0x28, reg);      
#else
    /* UART from PLL2, calc 25MHz to uart */
    value = u32PMU_ReadPLL2CLK() / 2;
    value = (value / 25) - 1;
    write_reg(REG_PMU_BASE + 0x70, reg | (value << 16));
#endif        
#endif
    serial_init();
    check_SIP();
}

void set_fcs(unsigned int cpu_value, int mul, int div)
{
    unsigned int setting;

    /* set PLL3 CC */
    //setting = read_reg(REG_PMU_BASE + 0x80) & 0xFFFFFCFF;
    //write_reg(REG_PMU_BASE + 0x80, (setting | (0x2 << 8)));
    
    setting = read_reg(REG_PMU_BASE + 0x30) & 0xF;
    /* Set CPU to deal with FCS irq */
    write_reg(0x96000000 + 0x45C, 0x100);
    write_reg(0x96000000 + 0x4, 0x100);

    /* set PLL1 */
    setting = setting | (cpu_value << 16) | (div << 11) | (mul << 4) | (0x3 << 2);
    write_reg(REG_PMU_BASE + 0x30, setting);

    KPrint("go...");

    /* set PLL2 & PLL3 frang */
#ifdef MODE_CPU    
    setting = read_reg(REG_PMU_BASE + 0x34) & 0xFFF3F803;
    write_reg(REG_PMU_BASE + 0x34, setting | (0x3 << 18) | (40 << 4) | (0x3 << 2));
#else
    setting = read_reg(REG_PMU_BASE + 0x34) & 0xFFF3FFF3;
    write_reg(REG_PMU_BASE + 0x34, setting | (0x3 << 18) | (0x3 << 2));
#endif
        
    setting = read_reg(REG_PMU_BASE + 0x0C);
    write_reg(REG_PMU_BASE + 0x0C, setting | (1 << 21));        //select CPU off
    write_reg(REG_PMU_BASE + 0x0C, setting | (1 << 21) | (1 << 2));

    setting = 0;

    __asm__ __volatile__(" mcr p15, 0, %0, c7, c0, 4 ": "=r"(setting):: "memory");
    __asm__ __volatile__(" nop ":::"memory");
    __asm__ __volatile__(" nop ":::"memory");
    __asm__ __volatile__(" nop ":::"memory");   //return here from IDLE mode interrupt 
}

void InvalidateCache(void)
{
    unsigned int setting = 0;

    __asm__ __volatile__(" mcr p15, 0, %0, c7, c5, 0 ": "=r"(setting):: "memory");

    __asm__ __volatile__(" mcr p15, 0, %0, c7, c6, 0 ": "=r"(setting):: "memory");
}

#ifdef MEM_TEST
int memtest(void)
{
    unsigned int *bg;
    int i = 0, j = 0, fail = 0;
    unsigned int pattern[2] = {0x5a5a5a5a, 0xa5a5a5a5};
    int TEST_LENGTH = 0x60000;
    unsigned int addr = 0;

    KPrint("Test addr: 0x%x, len = 0x%x\n", addr, TEST_LENGTH);    
    bg = (unsigned int *)addr;
    for (j = 0; j < (TEST_LENGTH / 4); j += 2) {
        *bg++ = pattern[i];
        *bg++ = pattern[i + 1];
    }
    
    bg = (unsigned int *)addr;
    for (j = 0; j < (TEST_LENGTH / 4); j += 2) {
        if (*bg != pattern[i] || (*(bg + 1) != pattern[i + 1]))
            fail++;
    
        bg += 2;
    }
    if (fail) {
        KPrint("error number %d: ", fail);
    } else {
        KPrint("Test pass, pattern 0x%x, 0x%x\n", pattern[i], pattern[i + 1]);
    }
    if(fail > 0)
    	while(1);

    return 0;
}
#endif

#ifndef USE_SCP
void set_ddr(void)
{
    volatile int i;    
    //KPrint("set ddr arg\n");

    for(i = 0; i < sizeof(DDR_arg) / 8; i++) {
        write_reg(REG_DDR0_BASE + DDR_arg[i][0], DDR_arg[i][1]);
    }
    
    write_reg(REG_DDR0_BASE + 0x04, 0x00000001);
    while(!(read_reg(REG_DDR0_BASE + 0x04) & 0x100));
    
    for(i = 0; i < 60000; i++);
}
#endif

void FCS_irq_data(void)
{
    //enable CPU IRQ
    __asm__ __volatile__("\n"
			"	MRS r0, CPSR\n"
			"	BIC r0, r0, #0x80\n"
			"	MSR CPSR_c, r0\n");

    //ldr pc, 0x40 assembly code
    write_reg(0x18, 0xE59FF020);
    //0x40 data is 0x92000040 assembly code
    write_reg(0x40, 0x92000040);
}

//volatile int tt=0;
void entry(void)
{
    unsigned int image_offset;
    struct spi_flash *spi_chip = NULL;
    unsigned int reg;
    void (*image_entry) (void) = (void *)LOAD_ADDR;

    platform_setup();

#ifndef USE_SCP
    set_ddr();
#endif    
    FCS_irq_data();

#ifndef CONFIG_FPGA
#ifdef SOCKET_EVB
    KPrint("SOCKET ");
#endif
#ifdef SYSTEM_EVB
    KPrint("SYSTEM ");
#endif

#ifdef MEM_TEST
    KPrint("memtest before FCS\n");
    memtest();
    write_reg(0x18, 0xE59FF020);
    write_reg(0x40, 0x92000040);
#endif
/* ================= GM8136 ================ */
#ifdef GM8136
    KPrint("GM8136 ");

#ifdef NORMAL
    set_fcs(0x6216, 81, 3);
    //set_fcs(0x6256, 86, 3);//audio
    KPrint("normal\n");     
#endif 
#ifdef MODE_0
    set_fcs(0x5014, 50, 3);
    KPrint("mode 0\n");    
#endif    
#ifdef MODE_1
    set_fcs(0x4202, 60, 3); //CPU600
    //set_fcs(0x4000, 40, 3); //CPU400
    KPrint("mode 1\n");    
#endif
#ifdef MODE_2
    set_fcs(0x6256, 100, 3);
    KPrint("mode 2\n");    
#endif
#ifdef MODE_3
    set_fcs(0x6216, 118, 4);
    KPrint("mode 3\n");    
#endif
#ifdef MODE_4
    set_fcs(0x4202, 95, 4);
    KPrint("mode 4\n");    
#endif
#ifdef MODE_5
    set_fcs(0x6216, 86, 3);
    KPrint("mode 5\n");    
#endif
#ifdef MODE_6
    set_fcs(0x6216, 127, 5);
    KPrint("mode 6\n");    
#endif
#ifdef USE_JUMP
    KPrint("debug\n");    
#endif

#endif  //GM8136
/* ================= GM8136S ================ */
#ifdef GM8136S
    KPrint("GM8136S ");

#ifdef NORMAL
    set_fcs(0x6216, 81, 3);
    //set_fcs(0x6216, 90, 3);//1200
    //set_fcs(0x6054, 59, 3);//1180
    KPrint("normal\n");     
#endif 
#ifdef MODE_0
    set_fcs(0x5216, 127, 5);
    KPrint("mode 0\n");    
#endif    
#ifdef MODE_1
    set_fcs(0x4202, 60, 3); //CPU600
    //set_fcs(0x4000, 40, 3); //CPU400
    KPrint("mode 1\n");    
#endif
#ifdef MODE_2
    set_fcs(0x3216, 59, 3);
    KPrint("mode 2\n");    
#endif
#ifdef MODE_3
    set_fcs(0x6216, 86, 3);
    KPrint("mode 3\n");    
#endif
#ifdef MODE_4
    set_fcs(0x6216, 118, 4);
    KPrint("mode 4\n");    
#endif
#ifdef MODE_5
    set_fcs(0x5216, 93, 3);
    KPrint("mode 5\n");    
#endif
#ifdef MODE_6
    set_fcs(0x7156, 100, 3);
    KPrint("mode 6\n");    
#endif
#ifdef MODE_7
    set_fcs(0x5216, 95, 4);
    KPrint("mode 7\n");    
#endif
#ifdef MODE_8
    set_fcs(0x1002, 59, 4);
    //set_fcs(0x1016, 59, 4);
    KPrint("mode 8\n");
#endif
#ifdef USE_JUMP
    KPrint("debug\n");    
#endif
#endif  //GM8136S
/* ================= GM8135S ================ */
#ifdef GM8135S
    KPrint("GM8135S ");

#ifdef NORMAL
    set_fcs(0x6216, 81, 3);
    KPrint("normal\n");
#endif 
#ifdef MODE_0
    set_fcs(0x5014, 50, 3);
    KPrint("mode 0\n");
#endif    
#ifdef MODE_1
    set_fcs(0x3216, 59, 3);
    //set_fcs(0x4000, 40, 3); //CPU400
    KPrint("mode 1\n");
#endif
#ifdef MODE_CPU
    set_fcs(0x4201, 81, 3); //CPU810/800
    KPrint("mode cpu\n");
#endif
#ifdef MODE_4
    set_fcs(0x5216, 95, 4);
    KPrint("mode 4\n");
#endif
#ifdef MODE_5
    set_fcs(0x1016, 59, 4);
    //set_fcs(0x1012, 59, 4);
    KPrint("mode 5\n");
#endif
#ifdef USE_JUMP
    KPrint("debug\n");
#endif
#endif  //GM8135S
/* ======================================== */
#endif  //CONFIG_FPGA

#ifdef MEM_TEST
    KPrint("memtest after FCS\n");
    memtest();    
#endif

    InvalidateCache();
    check_flash_type();

    /* SPI clock on */
    reg = read_reg(REG_PMU_BASE + 0xB4);
    reg &= ~(0x1 << 15);
    write_reg(REG_PMU_BASE + 0xB4, reg);

#ifdef CONFIG_SPI_DMA    
    /* DMA init */
    Dma_init((1 << REQ_SPI020RX) | (1 << REQ_SPI020TX));
#endif
    /* SPI init */
    spi_chip = FTSPI020_Init();
    if (!spi_chip) {
        KPrint("SPI init fail! \n");
        return;
    }

    if (flash_type == FLASH_SPI_NAND) {
        KPrint("MP SPI-NAND Bootstrap " VERSION "\n");

        image_offset = fLib_SPI_NAND_Copy_Image((unsigned int)LOAD_ADDR);
    }
    if (flash_type == FLASH_NOR) {
        //KPrint("MP SPI-NOR Bootstrap " VERSION "\n");

        image_offset = fLib_NOR_Copy_Image((unsigned int)LOAD_ADDR);
    }

    if (image_offset < 0) {
        KPrint("image_offset < 0\n");
        return;
    }
//tt++;
//while(tt > 0);            
    //KPrint("Boot image offset: 0x%x. Booting Image .....\n", image_offset);
    image_entry();

    return;
}
