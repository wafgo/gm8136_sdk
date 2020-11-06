/*
 * (C) Copyright 2009 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <common.h>
#include <netdev.h>
#include <asm/io.h>

//#include <faraday/ftsmc020.h>
//#include <pci.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_FTGMAC100
extern int ftgmac100_initialize(bd_t * bd);
extern int ftgmac100_read_phy_register(const char *devname, unsigned char addr,
                                       unsigned char reg, unsigned short *value);
#else
extern int ftmac110_initialize(bd_t * bis);
#endif

static unsigned int pmu_get_chip(void);
/*
 * Miscellaneous platform dependent initialisations
 */

#define PMU_PMODE_OFFSET      0x0C
#define PMU_PLL1CR_OFFSET     0x30
#define PMU_PLL23CR_OFFSET    0x34

//#define MAC_FROM_PLL1

u32 ahbdma_cmd[5][2] = {
#ifdef CONFIG_SPI_NAND_GM
    {0, 0}, {0, 0},
    {
     (mDMACtrlTc1b(0) |         //TC_MSK
      mDMACtrlDMAFFTH3b(0) |    //DMA FIFO threshold value
      mDMACtrlChpri2b(0) |      //Channel priority
      mDMACtrlCache1b(0) |      //PROTO3
      mDMACtrlBuf1b(0) |        //PROTO2
      mDMACtrlPri1b(0) |        //PROTO1
      mDMACtrlSsz3b(DMA_BURST_8) |      //Source burst size selection
      mDMACtrlAbt1b(0) |        //ABT
      mDMACtrlSwid3b(DMA_WIDTH_4) |     //SRC_WIDTH
      mDMACtrlDwid3b(DMA_WIDTH_4) |     //DST_WIDTH
      mDMACtrlHW1b(1) |         //Hardware Handshake
      mDMACtrlSaddr2b(DMA_ADD_INC) |    //Memory is the source
      mDMACtrlDaddr2b(DMA_ADD_FIX) |    //NOR is the destination
      mDMACtrlSsel1b(1) |       //AHB Master 1 is the source
      mDMACtrlDsel1b(0) |       //AHB Master 0 is the destination
      mDMACtrlEn1b(1)),         //Channel Enable

     (mDMACfgDst5b(0x10 | REQ_SPI020RX) |       //bit13, 12-9; SSP0_TX:1
      mDMACfgSrc5b(0) |         //Memory
      mDMACfgAbtDis1b(0) |      //Channel abort interrupt mask. 0 for not mask
      mDMACfgErrDis1b(0) |      //Channel error interrupt mask. 0 for not mask
      mDMACfgTcDis1b(0))        //Channel terminal count interrupt mask. 0 for not mask
     },
    // DMA_SPINOR2MEM          ((4UL << 8))
    {
     (mDMACtrlTc1b(0) |         //TC_MSK
      mDMACtrlDMAFFTH3b(0) |    //DMA FIFO threshold value
      mDMACtrlChpri2b(0) |      //Channel priority
      mDMACtrlCache1b(0) |      //PROTO3
      mDMACtrlBuf1b(0) |        //PROTO2
      mDMACtrlPri1b(0) |        //PROTO1
      mDMACtrlSsz3b(DMA_BURST_8) |      //Source burst size selection
      mDMACtrlAbt1b(0) |        //ABT
      mDMACtrlSwid3b(DMA_WIDTH_4) |     //SRC_WIDTH
      mDMACtrlDwid3b(DMA_WIDTH_4) |     //DST_WIDTH
      mDMACtrlHW1b(1) |         //Hardware Handshake
      mDMACtrlSaddr2b(DMA_ADD_FIX) |    //Memory is the source
      mDMACtrlDaddr2b(DMA_ADD_INC) |    //NOR is the destination
      mDMACtrlSsel1b(0) |       //AHB Master 0 is the source
      mDMACtrlDsel1b(1) |       //AHB Master 1 is the destination
      mDMACtrlEn1b(1)),         //Channel Enable

     (mDMACfgDst5b(0) |         //Memory
      mDMACfgSrc5b(0x10 | REQ_SPI020TX) |       //bit13, 12-9; SSP0_RX:0
      mDMACfgAbtDis1b(0) |      //Channel abort interrupt mask. 0 for not mask
      mDMACfgErrDis1b(0) |      //Channel error interrupt mask. 0 for not mask
      mDMACfgTcDis1b(0))        //Channel terminal count interrupt mask. 0 for not mask
     },
#endif
#ifdef CONFIG_NAND024V2_GM      //CONFIG_CMD_NAND
    // DMA_MEM2NAND         ((1UL << 8))
    {
     (mDMACtrlTc1b(0) | mDMACtrlDMAFFTH3b(0) | mDMACtrlChpri2b(0) | mDMACtrlCache1b(0) | mDMACtrlBuf1b(0) | mDMACtrlPri1b(0) | mDMACtrlSsz3b(DMA_BURST_4) |     /* default value */
      mDMACtrlAbt1b(0) | mDMACtrlSwid3b(DMA_WIDTH_4) | mDMACtrlDwid3b(DMA_WIDTH_4) | mDMACtrlHW1b(1) | mDMACtrlSaddr2b(DMA_ADD_INC) |   //Memory is the source
      mDMACtrlDaddr2b(DMA_ADD_FIX) |    //NAND is the destination
      mDMACtrlSsel1b(1) |       //AHB Master 1 is the source
      mDMACtrlDsel1b(0) |       //AHB Master 0 is the destination
      mDMACtrlEn1b(1)),

     (mDMACfgDst5b(0x10 | REQ_NANDRX) | //bit13, 12-9, NANDC:9
      mDMACfgSrc5b(0) | mDMACfgAbtDis1b(0) | mDMACfgErrDis1b(0) | mDMACfgTcDis1b(0))
     },
    // DMA_NAND2MEM         ((2UL << 8))
    {
     (mDMACtrlTc1b(0) | mDMACtrlDMAFFTH3b(0) | mDMACtrlChpri2b(0) | mDMACtrlCache1b(0) | mDMACtrlBuf1b(0) | mDMACtrlPri1b(0) | mDMACtrlSsz3b(DMA_BURST_128) | mDMACtrlAbt1b(0) | mDMACtrlSwid3b(DMA_WIDTH_4) | mDMACtrlDwid3b(DMA_WIDTH_4) | mDMACtrlHW1b(1) | mDMACtrlSaddr2b(DMA_ADD_FIX) | mDMACtrlDaddr2b(DMA_ADD_INC) | mDMACtrlSsel1b(0) |        //AHB Master 0 is the source
      mDMACtrlDsel1b(1) |       //AHB Master 1 is the destination
      mDMACtrlEn1b(1)),

     (mDMACfgDst5b(0) |
      mDMACfgSrc5b(0x10 | REQ_NANDTX) | mDMACfgAbtDis1b(0) | mDMACfgErrDis1b(0) | mDMACfgTcDis1b(0))
     },
#else
    {0, 0}, {0, 0},
#endif
#ifdef CONFIG_CMD_SPI
    /*
     * NOR configuration
     */
    // DMA_MEM2SPINOR          ((3UL << 8))
    {
     (mDMACtrlTc1b(0) |         //TC_MSK
      mDMACtrlDMAFFTH3b(0) |    //DMA FIFO threshold value
      mDMACtrlChpri2b(0) |      //Channel priority
      mDMACtrlCache1b(0) |      //PROTO3
      mDMACtrlBuf1b(0) |        //PROTO2
      mDMACtrlPri1b(0) |        //PROTO1
      mDMACtrlSsz3b(DMA_BURST_8) |      //Source burst size selection
      mDMACtrlAbt1b(0) |        //ABT
      mDMACtrlSwid3b(DMA_WIDTH_4) |     //SRC_WIDTH
      mDMACtrlDwid3b(DMA_WIDTH_4) |     //DST_WIDTH
      mDMACtrlHW1b(1) |         //Hardware Handshake
      mDMACtrlSaddr2b(DMA_ADD_INC) |    //Memory is the source
      mDMACtrlDaddr2b(DMA_ADD_FIX) |    //NOR is the destination
      mDMACtrlSsel1b(1) |       //AHB Master 1 is the source
      mDMACtrlDsel1b(0) |       //AHB Master 0 is the destination
      mDMACtrlEn1b(1)),         //Channel Enable

     (mDMACfgDst5b(0x10 | REQ_SPI020RX) |       //bit13, 12-9; SSP0_TX:1
      mDMACfgSrc5b(0) |         //Memory
      mDMACfgAbtDis1b(0) |      //Channel abort interrupt mask. 0 for not mask
      mDMACfgErrDis1b(0) |      //Channel error interrupt mask. 0 for not mask
      mDMACfgTcDis1b(0))        //Channel terminal count interrupt mask. 0 for not mask
     },
    // DMA_SPINOR2MEM          ((4UL << 8))
    {
     (mDMACtrlTc1b(0) |         //TC_MSK
      mDMACtrlDMAFFTH3b(0) |    //DMA FIFO threshold value
      mDMACtrlChpri2b(0) |      //Channel priority
      mDMACtrlCache1b(0) |      //PROTO3
      mDMACtrlBuf1b(0) |        //PROTO2
      mDMACtrlPri1b(0) |        //PROTO1
      mDMACtrlSsz3b(DMA_BURST_8) |      //Source burst size selection
      mDMACtrlAbt1b(0) |        //ABT
      mDMACtrlSwid3b(DMA_WIDTH_4) |     //SRC_WIDTH
      mDMACtrlDwid3b(DMA_WIDTH_4) |     //DST_WIDTH
      mDMACtrlHW1b(1) |         //Hardware Handshake
      mDMACtrlSaddr2b(DMA_ADD_FIX) |    //Memory is the source
      mDMACtrlDaddr2b(DMA_ADD_INC) |    //NOR is the destination
      mDMACtrlSsel1b(0) |       //AHB Master 0 is the source
      mDMACtrlDsel1b(1) |       //AHB Master 1 is the destination
      mDMACtrlEn1b(1)),         //Channel Enable

     (mDMACfgDst5b(0) |         //Memory
      mDMACfgSrc5b(0x10 | REQ_SPI020TX) |       //bit13, 12-9; SSP0_RX:0
      mDMACfgAbtDis1b(0) |      //Channel abort interrupt mask. 0 for not mask
      mDMACfgErrDis1b(0) |      //Channel error interrupt mask. 0 for not mask
      mDMACfgTcDis1b(0))        //Channel terminal count interrupt mask. 0 for not mask
     },
#else
    {0, 0}, {0, 0},
#endif
    {0, 0}
};

/* mode:0 for NAND, mode:1 for SPI */
int platform_setup(int mode)
{
    unsigned int reg = 0;

#ifndef CONFIG_CMD_FPGA
    if (readl(CONFIG_PMU_BASE) & 0xF000) {
        reg = readl(CONFIG_PMU_BASE + 0x4);
#ifdef CONFIG_NAND024V2_GM
        if (!((mode == 0) && ((reg & (0x1 << 22)) == 0))) {
            printf("not for NAND jump setting\n");
            return -1;
        }
#endif
#ifdef CONFIG_SPI_NAND_GM
        if (!((mode == 1) && ((reg & (0x3 << 22)) == (0x1 << 22)))) {
            printf("not for SPI NAND jump setting\n");
            return -1;
        }
#endif
#ifdef CONFIG_CMD_SPI
        if (!((mode == 1) && ((reg & (0x2 << 22)) == (0x2 << 22)))) {
            printf("not for SPI NOR jump setting\n");
            return -1;
        }
#endif
    } else {
        //A version
        reg = readl(CONFIG_PMU_BASE + 0x4);
#ifdef CONFIG_SPI_NAND_GM
        if (!((mode == 1) && ((reg & (1 << 23)) == 0))) {
            printf("not for SPI NAND jump setting\n");
            return -1;
        }
#endif
#ifdef CONFIG_CMD_SPI
        if (!((mode == 1) && ((reg & (1 << 23)) == 1))) {
            printf("not for SPI jump setting\n");
            return -1;
        }
#endif
    }
#endif

    if (mode == 0) {
        /* Turn on NAND clock */
        reg = readl(CONFIG_PMU_BASE + 0xB4);
        reg &= ~(0x1 << 16);
        writel(reg, CONFIG_PMU_BASE + 0xB4);

        /* Set PinMux */
        reg = readl(CONFIG_PMU_BASE + 0x54);
        reg &= ~(0x3F << 26);
        reg |= (0x2A << 26);
        writel(reg, CONFIG_PMU_BASE + 0x54);

        if (readl(CONFIG_PMU_BASE) & 0xF000) {
            reg = readl(CONFIG_PMU_BASE + 0x58);
            reg &= 0xF0F00030;
            reg |= 0x0A0AAA8A;
            writel(reg, CONFIG_PMU_BASE + 0x58);

            reg = readl(CONFIG_PMU_BASE + 0x64);
            reg &= 0xFFFFCFFF;
            reg |= 0x00003000;
            writel(reg, CONFIG_PMU_BASE + 0x64);
        } else {
            //A version
            reg = readl(CONFIG_PMU_BASE + 0x58);
            reg &= 0xF0000000;
            reg |= 0x0AAAAAAA;
            writel(reg, CONFIG_PMU_BASE + 0x58);
        }
    } else {
        /* Turn on SPI clock */
        reg = readl(CONFIG_PMU_BASE + 0xB4);
        reg &= ~(0x1 << 15);
        writel(reg, CONFIG_PMU_BASE + 0xB4);

        /* Set PinMux */
        reg = readl(CONFIG_PMU_BASE + 0x54);
        reg &= ~(0x3F << 26);
        reg |= (0x15 << 26);
        writel(reg, CONFIG_PMU_BASE + 0x54);

        reg = readl(CONFIG_PMU_BASE + 0x58);
        reg &= ~0x3;
        reg |= 0x1;
        writel(reg, CONFIG_PMU_BASE + 0x58);
    }
    return 0;
}

#define GET_PLAT_FULL_ID()		(readl(CONFIG_PMU_BASE))

#ifdef CONFIG_USE_IRQ
void intc_init(void)
{
    //only support FCS interrupt
    writel(0x100, CONFIG_INTC_BASE + 0x45C);
}
#endif

/* for 726 CPU, mode = 1 is write alloc enable  */
void set_write_alloc(u32 setting)
{
    __asm__ __volatile__(" mcr p15, 0, %0, c15, c0, 0\n"::"r"(setting));
}

u32 get_write_alloc(void)
{
    u32 data;

    __asm__ __volatile__(" mrc p15, 0, %0, c15, c0, 0 ":"=r"(data));
//__asm__ __volatile__ (" mrc p15, 0, %0, c0, c0, 0 " : "=r"(data));//read ID
    return data;
}

int board_init(void)
{
    unsigned int value = 0;

    gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_USE_IRQ
    intc_init();
#endif

#ifdef CONFIG_FTSMC020
    ftsmc020_init();            /* initialize Flash */
#endif

#ifdef CONFIG_CMD_NAND
#ifdef CONFIG_SPI_NAND_GM
    if (platform_setup(1) < 0)
        printf("SPI NAND init fail\n");
    else
        printf("SPI NAND mode\n");
#else
    if (platform_setup(0) < 0)
        printf("NAND init fail\n");
    else
        printf("NAND mode\n");        
#endif
#endif

#ifdef CONFIG_CMD_SPI
    if (platform_setup(1) < 0)
        printf("SPI init fail\n");
    else
        printf("SPI mode\n");         
#endif
    //enable write alloc
    value = get_write_alloc() | (1 << 20);
    set_write_alloc(value);

    return 0;
}

int dram_init(void)
{
    unsigned long sdram_base = PHYS_SDRAM;
    unsigned long expected_size = PHYS_SDRAM_SIZE;
    unsigned long actual_size;

    actual_size = get_ram_size((void *)sdram_base, expected_size);

    gd->ram_size = actual_size;

    if (expected_size != actual_size)
        printf("Warning: Only %lu of %lu MiB SDRAM is working\n",
               actual_size >> 20, expected_size >> 20);

    return 0;
}

uint u32PMU_ReadPLL1CLK(void)
{
#ifdef CONFIG_CMD_FPGA
    return 20000000;
#else
    uint value, n, m;

    value = readl(CONFIG_PMU_BASE + 0x30);
    n = (value >> 4) & 0x7F;
    m = (value >> 11) & 0x1F;
    value = (SYS_CLK * n) / m;

    return value;
#endif
}

uint u32PMU_ReadPLL2CLK(void)
{
#ifdef CONFIG_CMD_FPGA
    return 20000000;
#else
    u32 value, n, m;

    value = readl(CONFIG_PMU_BASE + 0x34);
    n = (value >> 4) & 0x7F;
    m = (value >> 11) & 0x1F;
    value = (SYS_CLK * n) / m;

    return value;
#endif
}

uint u32PMU_ReadPLL3CLK(void)
{
#ifdef CONFIG_CMD_FPGA
    return 20000000;
#else
    u32 value, n, m;

    value = readl(CONFIG_PMU_BASE + 0x34);
    n = (value >> 20) & 0x7F;
    m = (value >> 27) & 0x1F;
	value = (SYS_CLK * n) / m;

    return value;
#endif
}

uint u32PMU_ReadHCLK(void)
{
#ifdef CONFIG_CMD_FPGA
    return 20000000;
#else
    uint value = 0;

    value = readl(CONFIG_PMU_BASE + 0x30);
    value = (value >> 18) & 0x1;

    if (value)
        value = u32PMU_ReadPLL2CLK() / 4;
    else
        value = u32PMU_ReadPLL1CLK() / 4;

    return value;
#endif
}

uint u32PMU_ReadCPUCLK(void)
{
    uint value = 0;

    value = readl(CONFIG_PMU_BASE + 0x30);
    value = (value >> 22) & 0x1;

    if(pmu_get_chip() == 0x8137)
        value = u32PMU_ReadPLL2CLK() / 2;
    else if (value)
        value = u32PMU_ReadPLL2CLK() / 2;
    else
        value = u32PMU_ReadPLL2CLK();

    return value;
}

uint u32PMU_ReadAXICLK(void)
{
#ifdef CONFIG_CMD_FPGA
    return 50000000;
#else
    uint value = 0;

    value = readl(CONFIG_PMU_BASE + 0x30);
    value = (value >> 20) & 0x1;

    if (value)
        value = u32PMU_ReadPLL1CLK() / 4;
    else
        value = u32PMU_ReadPLL1CLK() / 3;

    return value;
#endif
}

uint u32PMU_ReadPCLK(void)
{
#ifdef CONFIG_CMD_FPGA
    return 20000000;
#else
    return u32PMU_ReadHCLK() / 2;
#endif
}

//#ifndef CONFIG_SKIP_LOWLEVEL_INIT

#define VGA_CONFIG	0       // default vga setting = 1024x768

struct vga_setting {
    uint ns;
    uint ms;
    char resolution[10];
};

struct vga_setting vgs[] = {
    {54, 1, "1024x768"},
    {63, 1, "1280x800"},
    {135, 2, "1280x960"},
    {54, 1, "1280x1024"},
    {72, 1, "1360x768"},
    {99, 2, "720P"},
    {99, 2, "1080I"},
    {63, 1, "800x600"},
    {0, 0, ""}
};

uint vga_res = VGA_CONFIG;

uint u32PMU_ReadDDRCLK(void)
{
    unsigned int value = 0;

    value = readl(CONFIG_PMU_BASE + 0x30);
    if (value & BIT16)
        value = u32PMU_ReadPLL1CLK() / 4;
    else
        value = u32PMU_ReadPLL1CLK() / 3;

    return (value * 4);
}

uint u32PMU_ReadJpegCLK(void)
{
    return u32PMU_ReadHCLK();
}

uint u32PMU_ReadH264encCLK(void)
{
    if(pmu_get_chip() == 0x8137)
        return u32PMU_ReadAXICLK() / 2;
    else
        return u32PMU_ReadAXICLK();
}

void display_sys_freq(void)
{
    printf("\n-------------------------------\n");
    printf("ID:%8x\n", GET_PLAT_FULL_ID());
    printf("AC:%4d  HC:%4d  P1:%4d  P2:%4d  P3:%4d\n",
           u32PMU_ReadAXICLK() / 1000000, u32PMU_ReadHCLK() / 1000000,
           u32PMU_ReadPLL1CLK() / 1000000, u32PMU_ReadPLL2CLK() / 1000000, u32PMU_ReadPLL3CLK() / 1000000);
    printf("C7:%4d  DR:%4d\n", u32PMU_ReadCPUCLK() / 1000000, u32PMU_ReadDDRCLK() / 1000000);
    printf("J:%4d   H1:%4d\n", u32PMU_ReadJpegCLK() / 1000000, u32PMU_ReadH264encCLK() / 1000000);
    //printf("   VGA : %s\n", vgs[vga_res].resolution);
    printf("-------------------------------\n");
}

//#endif

int board_eth_init(bd_t * bd)
{
#ifndef CONFIG_FTGMAC100
    return 0;
#else
    int ret;                    /* 0:external PHY, 1:internal PHY */
    unsigned int value;
    unsigned short phy_data;
    unsigned int div;

    //add driving
    value = readl(CONFIG_PMU_BASE + 0x44);
    value |= 0x3;
    writel(value, CONFIG_PMU_BASE + 0x44);

#ifndef CONFIG_CMD_FPGA
#ifdef CONFIG_RMII
    printf("GMAC set RMII mode\n");

    //GMAC PHY CLK source
#ifndef MAC_FROM_PLL1    
    value = readl(CONFIG_PMU_BASE + 0x28);
    value &= ~BIT4;
    value |= BIT5;
    writel(value, CONFIG_PMU_BASE + 0x28);     //clock from PLL2 200M
#endif
    //set pin mux
    writel(0x22222222, CONFIG_PMU_BASE + 0x5C);
    writel(0x11112022, CONFIG_PMU_BASE + 0x60);

    value = readl(CONFIG_PMU_BASE + 0x7C);
    value &= ~BIT5;
    value |= BIT6;
    writel(value, CONFIG_PMU_BASE + 0x7C);

#ifdef MAC_FROM_PLL1
    value = (u32PMU_ReadPLL1CLK() / 1000000) / 2;
#else
    value = (u32PMU_ReadPLL2CLK() / 1000000) / 4;
#endif        
    if(value % 50)
        printf("========================> can't div 50MHz from PLL2 = %dMHz\n", value);

    //clock divided value    
    div = (value / 50) - 1;
    div = (div << 8);   // GMAC 50M, PHY 50M, 200/4 = 50, 50 / 1 = 50
    value = readl(CONFIG_PMU_BASE + 0x70) & 0xFFFFF00F;
    writel(value | div, CONFIG_PMU_BASE + 0x70);
#endif
#ifdef CONFIG_RGMII
    printf("GMAC set RGMII mode\n");

    //GMAC PHY CLK source
    writel(readl(CONFIG_PMU_BASE + 0x28) & ~(BIT4 | BIT5), CONFIG_PMU_BASE + 0x28);     //clock from PLL1 500M
    value = (u32PMU_ReadPLL1CLK() / 1000000) / 2;
    if(value % 125)
        printf("========================> can't div 125MHz from PLL1 = %dMHz\n", value);
    
    div = (value / 125) - 1;
    
    //set pin mux
    writel(0x11111111, CONFIG_PMU_BASE + 0x5C);
    writel(0x11111111, CONFIG_PMU_BASE + 0x60);

    //set phase
    value = readl(CONFIG_PMU_BASE + 0x7C);
    writel(value | BIT5 | BIT6, CONFIG_PMU_BASE + 0x7C);

    //clock divided value
    value = readl(CONFIG_PMU_BASE + 0x70) & 0xFFFFF00F;
    div = (div << 8) | (4 << 4);   // GMAC 125M, PHY 25M, 500/4 = 125, 125 / 5 = 25
    writel(value | div, CONFIG_PMU_BASE + 0x70);
#endif
#endif

    //GMAC PHY clock out on
    writel(readl(CONFIG_PMU_BASE + 0xAC) & (~BIT5), CONFIG_PMU_BASE + 0xAC);
    //GMAC clock out on
    writel(readl(CONFIG_PMU_BASE + 0xB4) & (~BIT11), CONFIG_PMU_BASE + 0xB4);

#ifndef CONFIG_CMD_FPGA

    writel(readl(CONFIG_PMU_BASE + 0xB8) & (~BIT17), CONFIG_PMU_BASE + 0xB8);   //APB gating clock, enable GPIO1
    //GPIO1 pin 23 reset PHY
    writel(BIT23, CONFIG_GPIO1_BASE);   //data out
    writel(BIT23, CONFIG_GPIO1_BASE + 0x8);     //pin direction out
    mdelay(1);
    writel(0, CONFIG_GPIO1_BASE);       //data out
    mdelay(10);
    printf("reset PHY\n");
    writel(BIT23, CONFIG_GPIO1_BASE);   //data out
    mdelay(30);
    writel(readl(CONFIG_PMU_BASE + 0xB8) | BIT17, CONFIG_PMU_BASE + 0xB8);      //APB gating clock, disable GPIO1
#endif

    phy_data = 0x0123;
    ret = ftgmac100_initialize(bd);

#ifdef CONFIG_CMD_FPGA
    ftgmac100_read_phy_register("eth0", 0, 2, &phy_data);
    printf("PHY reg(0x01)=%x\n", value);
#else
#ifdef FTMAC_DEBUG
    ftgmac100_read_phy_register("eth0", 0, 0x10, &phy_data);    //0x10 is PHY addr

    printf("PHY reg(0x01)=%x\n", value);
    if (value & 0x0020)
        printf("Auto-negotiation process completed\n");
    else
        printf("Auto-negotiation process NOT completed\n");
#endif
#endif
    return ret;
#endif
}

#ifdef CONFIG_USE_IRQ

// only handle PMU irq
void do_irq(struct pt_regs *pt_regs)
{
    uint val;

    writel(1 << 17, CONFIG_PMU_BASE + 0x20);
    val = readl(CONFIG_PMU_BASE + 0x0C);
    val &= ~0x4;
    writel(val, CONFIG_PMU_BASE + 0x0C);

    writel(1 << IRQ_PMU, CONFIG_INTC_BASE + 0x08);
    *(volatile ulong *)(CONFIG_INTC_BASE + 0x04) &= ~(1 << IRQ_PMU);
}

void PMU_enter_fcs_mode(uint cpu_value, uint mul, uint div)
{
    uint setting, ddrclk;

    ddrclk = (u32PMU_ReadDDRCLK() * 2) / 1000000;

    setting = readl(CONFIG_PMU_BASE + 0x80) & 0xFFFFFCFF;
    writel(setting | (0x2 << 8), CONFIG_PMU_BASE + 0x80);
    
    /* pllns & pllms */
    setting = (0x2 << 26) | (0x2 << 24) | (cpu_value << 16) | (mul << 3);

    /* pll frang */
    if (ddrclk > 1000)
        setting |= (7 << 11);
    else if (ddrclk > 800)
        setting |= (6 << 11);
    else if (ddrclk > 700)
        setting |= (5 << 11);
    else if (ddrclk > 600)
        setting |= (4 << 11);
    else if (ddrclk > 500)
        setting |= (3 << 11);
    else if (ddrclk > 400)
        setting |= (2 << 11);
    else if (ddrclk > 300)
        setting |= (1 << 11);

    if (setting == (readl(CONFIG_PMU_BASE + 0x30) & 0xfffffff8))
        return;                 // the same

    writel(setting, CONFIG_PMU_BASE + 0x30);

    *(volatile ulong *)(CONFIG_INTC_BASE + 0x0C) &= ~(1 << IRQ_PMU);
    *(volatile ulong *)(CONFIG_INTC_BASE + 0x10) &= ~(1 << IRQ_PMU);
    *(volatile ulong *)(CONFIG_INTC_BASE + 0x04) |= (1 << IRQ_PMU);

    printf("go...");

    setting = readl(CONFIG_PMU_BASE + 0x0C);
    writel(setting | (1 << 2), CONFIG_PMU_BASE + 0x0C);

    setting = 0;
    __asm__ __volatile__(" mcr p15, 0, %0, c7, c0, 4 ":"=r"(setting)::"memory");
    __asm__ __volatile__(" nop ":::"memory");   //return here from IDLE mode interrupt
}

void FCS_go(unsigned int cpu_mode, unsigned int pll1_mul, unsigned int pll4_mul)
{
    unsigned int tmp;

    tmp = readl(CONFIG_PMU_BASE + 0x38) & 0xFFFFF80F;
    tmp |= (pll4_mul << 0x4);
    writel(tmp, CONFIG_PMU_BASE + 0x38);

    *(volatile ulong *)(CONFIG_INTC_BASE + 0x08) = (1 << IRQ_PMU);

    PMU_enter_fcs_mode(cpu_mode, pll1_mul, 1);
}
#endif

static unsigned int pmu_get_chip(void)
{
	static unsigned int ID = 0;
	unsigned int product = 0;

    ID = (readl(CONFIG_PMU_BASE) >> 8) & 0xF;

	switch (ID) {
	  case 0xB:case 0xE:
	    product = 0x8139;
	    break;
	  case 0xA:case 0xD:case 0xF:
	    product = 0x8138;
	    break;
	  case 0xC:
	    product = 0x813C;
	    break;	    	            
	  case 0x5:case 0x6:case 0x7:
	    product = 0x8137;
	    break;
	  default:
	  	printf("Not define this ID\n");
	    break;
	}

	return product;
}

unsigned int commandline_mode(void)
{
    unsigned int ID = 0, product = 1;
    
    ID = pmu_get_chip();

    if ((ID == 0x8137) || (ID == 0x8138))
        product = 1;
    else if (ID == 0x8139)
        product = 2;
    else if (ID == 0x813C)//8138S
        product = 3;

exit_cmd:
    return product;
}

void init_sys_freq(void)
{
#ifndef CONFIG_CMD_FPGA
    unsigned int reg;

    // Open AXI/AHB/APB clock
    reg = readl(CONFIG_PMU_BASE + 0xB0);
    writel(reg & (~BIT8), CONFIG_PMU_BASE + 0xB0);      //AXI gating clock, enable intc030
    reg = readl(CONFIG_PMU_BASE + 0xB4);
    reg |= BIT12;
    writel(reg & (~BIT8), CONFIG_PMU_BASE + 0xB4);      //AHB gating clock, enable DMA
    reg = readl(CONFIG_PMU_BASE + 0xB8);
    writel(reg & (~(BIT0 | BIT10 | BIT11)), CONFIG_PMU_BASE + 0xB8);    //APB gating clock, enable UART0 / WDT / TIMER0
    //UART0 pin mux
    reg = readl(CONFIG_PMU_BASE + 0x58);
    reg &= 0x0FFFFFFF;
    reg |= (0x5 << 28);
    writel(reg, CONFIG_PMU_BASE + 0x58);

#if 0
    reg = readl(CONFIG_PMU_BASE + 0x4);

    if ((reg & (1 << 23)) != (1 << 23)) {       /* BIT23 0: NAND, 1: SPI */
        /* NAND clock on */
        reg = readl(CONFIG_PMU_BASE + 0xB4);
        reg &= ~(0x1 << 16);
        writel(reg, CONFIG_PMU_BASE + 0xB4);

        /* PinMux with NAND */
        reg = readl(CONFIG_PMU_BASE + 0x54);
        reg &= ~(0x3F << 26);
        reg |= (0x2A << 26);
        writel(reg, CONFIG_PMU_BASE + 0x54);

        reg = readl(CONFIG_PMU_BASE + 0x58);
        reg &= 0xF0000000;
        reg |= 0x0AAAAAAA;
        writel(reg, CONFIG_PMU_BASE + 0x58);

    } else {
        /* SPI clock on */
        reg = readl(CONFIG_PMU_BASE + 0xB4);
        reg &= ~(0x1 << 15);
        writel(reg, CONFIG_PMU_BASE + 0xB4);

        /* PinMux with SPI */
        reg = readl(CONFIG_PMU_BASE + 0x54);
        reg &= ~(0x3F << 26);
        reg |= (0x15 << 26);
        writel(reg, CONFIG_PMU_BASE + 0x54);

        reg = readl(CONFIG_PMU_BASE + 0x58);
        reg &= ~0x3;
        reg |= 0x1;
        writel(reg, CONFIG_PMU_BASE + 0x58);
    }
#endif
#ifdef CONFIG_VIDEO
    /* Enable PLL3 */
    //data = readl(CONFIG_PMU_BASE+0x34);

    // adjust PLL3 freq out to 648MHz, default is 486MHz
    //data &= ~0x0000FFFF;
    //data |= (vgs[vga_res].ms<<12)|(vgs[vga_res].ns<<4)|(1<<2); // ms/ns/frang

    //outw(CONFIG_PMU_BASE+0x34, data|BIT0);
    /* Wait for 500us */
    //udelay(5000);
#endif
#endif
}
