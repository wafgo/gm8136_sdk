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

#include <faraday/ftsmc020.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_FTGMAC100
extern int ftgmac100_initialize(bd_t *bd);
#else
extern int ftmac110_initialize(bd_t *bis);
#endif
/*
 * Miscellaneous platform dependent initialisations
 */

#define PMU_PMODE_OFFSET      0x0C
#define PMU_PLL1CR_OFFSET     0x30
#define PMU_PLL23CR_OFFSET    0x34

unsigned int ahbdma_cmd[5][2] = {  
    //DMA_MEM2MEM           ((0UL << 8) | 8)
    {
     (mDMACtrlTc1b(0) |
      mDMACtrlDMAFFTH3b(0) |
      mDMACtrlChpri2b(0) |
      mDMACtrlCache1b(0) |
      mDMACtrlBuf1b(0) |
      mDMACtrlPri1b(0) |
      mDMACtrlSsz3b(DMA_BURST_4) |
      mDMACtrlAbt1b(0) |
      mDMACtrlSwid3b(DMA_WIDTH_4) |
      mDMACtrlDwid3b(DMA_WIDTH_4) |
      mDMACtrlHW1b(0) |
      mDMACtrlSaddr2b(DMA_ADD_INC) |
      mDMACtrlDaddr2b(DMA_ADD_INC) | mDMACtrlSsel1b(0) | mDMACtrlDsel1b(0) | mDMACtrlEn1b(1)),

     (mDMACfgDst5b(0) |
      mDMACfgSrc5b(0) | mDMACfgAbtDis1b(0) | mDMACfgErrDis1b(0) | mDMACfgTcDis1b(0))
     },
#ifdef CONFIG_NAND       
    // DMA_MEM2NAND         ((1UL << 8))
    {
     (mDMACtrlTc1b(0) | mDMACtrlDMAFFTH3b(0) | mDMACtrlChpri2b(0) | mDMACtrlCache1b(0) | mDMACtrlBuf1b(0) | mDMACtrlPri1b(0) | mDMACtrlSsz3b(DMA_BURST_4) |     /* default value */
      mDMACtrlAbt1b(0) | mDMACtrlSwid3b(DMA_WIDTH_4) | mDMACtrlDwid3b(DMA_WIDTH_4) | mDMACtrlHW1b(1) | mDMACtrlSaddr2b(DMA_ADD_INC) |   //Memory is the source
      mDMACtrlDaddr2b(DMA_ADD_FIX) |    //NAND is the destination
      mDMACtrlSsel1b(0) | mDMACtrlDsel1b(0) | mDMACtrlEn1b(1)),

     (mDMACfgDst5b(0x10 | REQ_NANDRX) |      //bit13, 12-9, NANDC:9
      mDMACfgSrc5b(0) | mDMACfgAbtDis1b(0) | mDMACfgErrDis1b(0) | mDMACfgTcDis1b(0))
     },
    // DMA_NAND2MEM         ((2UL << 8))
    {
     (mDMACtrlTc1b(0) |
      mDMACtrlDMAFFTH3b(0) |
      mDMACtrlChpri2b(0) |
      mDMACtrlCache1b(0) |
      mDMACtrlBuf1b(0) |
      mDMACtrlPri1b(0) |
      mDMACtrlSsz3b(DMA_BURST_4) |
      mDMACtrlAbt1b(0) |
      mDMACtrlSwid3b(DMA_WIDTH_4) |
      mDMACtrlDwid3b(DMA_WIDTH_4) |
      mDMACtrlHW1b(1) |
      mDMACtrlSaddr2b(DMA_ADD_FIX) |
      mDMACtrlDaddr2b(DMA_ADD_INC) | mDMACtrlSsel1b(0) | mDMACtrlDsel1b(0) | mDMACtrlEn1b(1)),

     (mDMACfgDst5b(0) |
      mDMACfgSrc5b(0x10 | REQ_NANDTX) | mDMACfgAbtDis1b(0) | mDMACfgErrDis1b(0) | mDMACfgTcDis1b(0))
     },
#else
    {0, 0},{0, 0},
#endif
#ifdef CONFIG_SPI_FLASH
    /*
     * NOR configuration
     */
    // DMA_MEM2NOR          ((3UL << 8))
    {
     (mDMACtrlTc1b(0) |         //TC_MSK
      mDMACtrlDMAFFTH3b(0) |    //DMA FIFO threshold value
      mDMACtrlChpri2b(0) |      //Channel priority
      mDMACtrlCache1b(0) |      //PROTO3
      mDMACtrlBuf1b(0) |        //PROTO2    
      mDMACtrlPri1b(0) |        //PROTO1
      mDMACtrlSsz3b(DMA_BURST_1) |      //Source burst size selection
      mDMACtrlAbt1b(0) |        //ABT    
      mDMACtrlSwid3b(DMA_WIDTH_4) |     //SRC_WIDTH
      mDMACtrlDwid3b(DMA_WIDTH_4) |     //DST_WIDTH
      mDMACtrlHW1b(1) |         //Hardware Handshake
      mDMACtrlSaddr2b(DMA_ADD_INC) |    //Memory is the source
      mDMACtrlDaddr2b(DMA_ADD_FIX) |    //NOR is the destination
      mDMACtrlSsel1b(0) |       //AHB Master 0 is the source
      mDMACtrlDsel1b(0) |       //AHB Master 0 is the destination
      mDMACtrlEn1b(1)),         //Channel Enable

     (mDMACfgDst5b(0x11) |      //bit13, 12-9; SSP0_TX:1
      mDMACfgSrc5b(0) |         //Memory
      mDMACfgAbtDis1b(0) |      //Channel abort interrupt mask. 0 for not mask
      mDMACfgErrDis1b(0) |      //Channel error interrupt mask. 0 for not mask
      mDMACfgTcDis1b(0))        //Channel terminal count interrupt mask. 0 for not mask
     },
    // DMA_NOR2MEM          ((4UL << 8))
    {
     (mDMACtrlTc1b(0) |         //TC_MSK
      mDMACtrlDMAFFTH3b(0) |    //DMA FIFO threshold value
      mDMACtrlChpri2b(0) |      //Channel priority
      mDMACtrlCache1b(0) |      //PROTO3
      mDMACtrlBuf1b(0) |        //PROTO2
      mDMACtrlPri1b(0) |        //PROTO1
      mDMACtrlSsz3b(DMA_BURST_1) |      //Source burst size selection
      mDMACtrlAbt1b(0) |        //ABT 
      mDMACtrlSwid3b(DMA_WIDTH_4) |     //SRC_WIDTH
      mDMACtrlDwid3b(DMA_WIDTH_4) |     //DST_WIDTH
      mDMACtrlHW1b(1) |         //Hardware Handshake
      mDMACtrlSaddr2b(DMA_ADD_FIX) |    //Memory is the source
      mDMACtrlDaddr2b(DMA_ADD_INC) |    //NOR is the destination
      mDMACtrlSsel1b(0) |       //AHB Master 0 is the source
      mDMACtrlDsel1b(0) |       //AHB Master 0 is the destination
      mDMACtrlEn1b(1)),         //Channel Enable

     (mDMACfgDst5b(0) |         //Memory
      mDMACfgSrc5b(0x10) |      //bit13, 12-9; SSP0_RX:0
      mDMACfgAbtDis1b(0) |      //Channel abort interrupt mask. 0 for not mask
      mDMACfgErrDis1b(0) |      //Channel error interrupt mask. 0 for not mask
      mDMACfgTcDis1b(0))        //Channel terminal count interrupt mask. 0 for not mask
     }
#else
    {0, 0},{0, 0}
#endif      
};

#ifdef CONFIG_CMD_NAND
int platform_setup(void)
{
    unsigned int reg = 0;

    /* Turn on NAND clock */
    reg = readl(CONFIG_PMU_BASE + 0x38);
    reg &= ~(0x1 << 5);
    writel(reg, CONFIG_PMU_BASE + 0x38);
    /* PinMux with GPIO */
    reg = readl(CONFIG_PMU_BASE + 0x5C);
    reg &= ~(0x3 << 26 | 0x3 << 24 | 0x3 << 22 | 0x3 << 20 | 0x3 << 18 | 0x3 << 16 |
             0x3 << 14 | 0x3 << 12 | 0x3 << 10 | 0x3 << 8 | 0x3 << 6 | 0x3 << 4 | 0x3 << 2 | 0x3 <<
             0);
    reg |=
        (0x1 << 26 | 0x1 << 24 | 0x1 << 22 | 0x1 << 20 | 0x1 << 18 | 0x1 << 16 | 0x1 << 14 | 0x1 <<
         12 | 0x1 << 10 | 0x1 << 8 | 0x1 << 6 | 0x1 << 4 | 0x1 << 2 | 0x1 << 0);
    writel(reg, CONFIG_PMU_BASE + 0x5C);
  
    return 0;    
}
#endif

int board_init(void)
{
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;
#ifdef CONFIG_FTSMC020
	ftsmc020_init();	/* initialize Flash */
#endif	
#ifdef CONFIG_CMD_NAND
    if(platform_setup() < 0)
    	printf("nand init fail\n");
#endif
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

unsigned int u32PMU_ReadPLL1CLK(void)
{
    unsigned int mul, div;

    mul = ((readl(CONFIG_PMU_BASE+PMU_PLL1CR_OFFSET)) >>  4) & 0x1FF;    /* N */
    div = ((readl(CONFIG_PMU_BASE+PMU_PLL1CR_OFFSET)) >> 13) & 0x1F;     /* M */

    return (SYS_CLK * mul / div);
}

unsigned int u32PMU_ReadPLL2CLK(void)
{
		unsigned int mul, div;
	
    mul = ((readl(CONFIG_PMU_BASE+PMU_PLL23CR_OFFSET)) >> 3) & 0x1FF;    /* N */
    div = ((readl(CONFIG_PMU_BASE+PMU_PLL23CR_OFFSET)) >> 12) & 0x1F;    /* M */

    return (SYS_CLK / div * mul);	
}

unsigned int u32PMU_ReadHCLK(void)
{
    unsigned int pll1_out,pll2_out;
    unsigned int cpu3x;

    cpu3x = ((readl(CONFIG_PMU_BASE+PMU_PLL1CR_OFFSET)) >> 25) & 0x3;
    pll1_out = u32PMU_ReadPLL1CLK();
    pll2_out = u32PMU_ReadPLL2CLK();
    if(cpu3x == 0)
    	return pll1_out;
    else if(cpu3x == 1)
    	return pll1_out / 2;
    else if(cpu3x == 2)
    	return pll1_out / 3;
    else
        return pll2_out / 2;	
}

unsigned int u32PMU_ReadPCLK(void)
{
    return (u32PMU_ReadHCLK() >> 1);
}

void board_pmu_eth_init(int internal)
{
	u32 reg_v, reg_pll1;
    u32 mul, div, clk;

	if(internal) {
		/* turn on Power */
		*(volatile ulong *)(CONFIG_PMU_BASE + 0x6c) |= ((BIT10) | (BIT18)); //bit10(mac_clk_en)) and bit18(reg_enable)
		/* turn on CLK */
		*(volatile ulong *)(CONFIG_PMU_BASE + 0x38) &= ~0x00000500; // (bit10 and bit8)

		mdelay(60);
		//*(volatile ulong *)(CONFIG_PMU_BASE + 0x84) = 0x0012c170; // The mac phy hardware reset should be longer than 500 ns
		//*(volatile ulong *)(CONFIG_PMU_BASE + 0x84) = 0x0012c140; // The mac phy hardware reset should be longer than 500 ns
		*(volatile ulong *)(CONFIG_PMU_BASE + 0x84) = 0x0012c100; // The mac phy hardware reset should be longer than 500 ns
		
		/* check MAC CLK */ 
		reg_v = *(volatile ulong *)(CONFIG_PMU_BASE + 0x74) & 0x03ffffff;
		clk = u32PMU_ReadPLL1CLK() / 1000000;
		
		//MII 25MHz
		reg_v |= (((clk / 25) - 1) << 26); 
		*(volatile ulong *)(CONFIG_PMU_BASE + 0x74) = reg_v;

		reg_v = *(volatile ulong *)(CONFIG_PMU_BASE + 0x5c) & 0x0fffffff;
		reg_v |= 0x50000000; //  X_EDP_LINKLED pin is mux-ed out,  X_EDP_SPDLED pin is mux-ed out
		*(volatile ulong *)(CONFIG_PMU_BASE + 0x5c) = reg_v;
		mdelay(10);
		
		//*(volatile ulong *)(CONFIG_PMU_BASE + 0x84) = 0x0012c171; // reset Faraday PHY, for 10M/half
		//*(volatile ulong *)(CONFIG_PMU_BASE + 0x84) = 0x0012c141; // reset Faraday PHY, for 100M/half
		*(volatile ulong *)(CONFIG_PMU_BASE + 0x84) = 0x0012c101; // reset Faraday PHY, for N-Way
	} else {
		/* bit10(mac_clk_en), bit19(RMII), bit11(mac_phy_bypas) */
		*(volatile ulong *)(CONFIG_PMU_BASE + 0x6c) |= ((BIT10)|(BIT11)|(BIT19)); 
		/* bit18(reg_enable=0) */
		*(volatile ulong *)(CONFIG_PMU_BASE + 0x6c) &= ~(BIT18); 			
		/* turn on CLK */
		*(volatile ulong *)(CONFIG_PMU_BASE + 0x38) &= ~0x00000500; // (bit10 and bit8)
		mdelay(60);
		/* check MAC CLK */ 
		reg_v = *(volatile ulong *)(CONFIG_PMU_BASE + 0x74) & 0x03ffffff;
		reg_pll1 = *(volatile ulong *)(CONFIG_PMU_BASE + 0x30);
	    mul = (reg_pll1 >>  3) & 0x1FF;    /* N */
	    div = (reg_pll1 >> 12) & 0x1F;     /* M */
		clk = 30 * mul / div ;
		reg_v |= (((clk / 25) - 1) << 26);
		*(volatile ulong *)(CONFIG_PMU_BASE + 0x74) = reg_v;
		*(volatile ulong *)(CONFIG_PMU_BASE + 0x28) &= ~0x0000003f;	// debug mode turnoff
		/* for pin muxed out */
		*(volatile ulong *)(CONFIG_PMU_BASE + 0x64) = 0x15555555;
	}
}

int board_eth_init(bd_t *bd)
{
	int ret,internalPhy=1; /* 0:external PHY, 1:internal PHY */

	board_pmu_eth_init(internalPhy); /*internalPhy ==> 0:external PHY, 1:internal PHY */

#ifdef CONFIG_FTGMAC100
	ret= ftgmac100_initialize(bd);
#else
	ret= ftmac110_initialize(bd);

#ifdef FTMAC_DEBUG
	value=0;
	if(internalPhy) {
		value=ftmac110_read_phy_register(1, 1);	// PHY_ADD 1
	} else {
		value=ftmac110_read_phy_register(0, 1); // PHY_ADD 0
	}
	printf("PHY reg(0x01)=%x\n",value);
	if(value &0x0020)
		printf("Auto-negotiation process completed\n");
	else
		printf("Auto-negotiation process NOT completed\n");
#endif
#endif
	return ret;
}

#ifdef CONFIG_USE_IRQ
#define GET_PLAT_FULL_ID()		(readl(CONFIG_PMU_BASE))

uint u32PMU_ReadCPUCLK(void)
{
    uint pll1_out,pll2_out;
    uint cpu3x;

    cpu3x = ((readl(CONFIG_PMU_BASE+PMU_PLL1CR_OFFSET)) >> 27) & 0x3;
    pll1_out = u32PMU_ReadPLL1CLK();
    pll2_out = u32PMU_ReadPLL2CLK();

    if(cpu3x == 0)
    	return pll1_out;
    else if(cpu3x == 1)
        return pll1_out / 2;
    else if(cpu3x == 2)
    	return pll1_out / 1.5;
    else
        return pll2_out;	
}

uint u32PMU_ReadMpeg4CLK(void)
{
    uint div,clk;

    clk = readl(CONFIG_PMU_BASE+0x28);
    if(clk & BIT7)
			return u32PMU_ReadHCLK();

    div = (readl(CONFIG_PMU_BASE+0x7c) >> 21) & 0x1f;
    return (u32PMU_ReadPLL1CLK() / (div + 1));
}

uint u32PMU_ReadH264CLK(void)
{
    uint div,clk,ret_clk;

    clk = readl(CONFIG_PMU_BASE+0x28) >> 18;
    switch (clk & 0x3) {
        case 0: 
            div = ((*(unsigned int *)(CONFIG_PMU_BASE+0x7c)) >> 16) & 0x1f;
            ret_clk = (u32PMU_ReadPLL1CLK() / (div + 1));
            break;
        case 1: 
        ret_clk = u32PMU_ReadHCLK();
            break;
        case 2: 
            ret_clk = (u32PMU_ReadPLL1CLK() * 2 / 5);
            break;
        case 3: 
        default: 
            ret_clk = 0;
            printf("Read H264 Clock error!\n");     
            break;
    }
    return (ret_clk);    
}

uint u32PMU_ReadDDRCLK(void)
{
    uint pll1_out, pll2_out;
    uint cpu3x;

    cpu3x = ((readl(CONFIG_PMU_BASE+PMU_PLL1CR_OFFSET)) >> 23) & 0x3;
    pll1_out = u32PMU_ReadPLL1CLK();
    pll2_out = u32PMU_ReadPLL2CLK();
    
    if(cpu3x == 0)
    	return pll1_out;
    else if(cpu3x == 1)
        return pll1_out / 2;
    else if(cpu3x == 2)
    	return pll2_out;
    else
        return pll2_out / 2;
}

void display_sys_freq(void)
{
	printf("\n---------------------------------------------------------\n");
	printf("   PLL1:%4d MHz       PLL2:%4d MHz       DDR:%4d MHz\n",
	       u32PMU_ReadPLL1CLK() / 1000000, u32PMU_ReadPLL2CLK() / 1000000,
	       u32PMU_ReadDDRCLK() / 1000000);
	printf("   CPU :%4d MHz       HCLK:%4d MHz       \n",
	       u32PMU_ReadCPUCLK() / 1000000, u32PMU_ReadHCLK() / 1000000);
	printf("   H.264:%4d MHz      MPEG4:%4d MHz\n",
	     u32PMU_ReadH264CLK() / 1000000, u32PMU_ReadMpeg4CLK() / 1000000);
	printf("---------------------------------------------------------\n");
}


void mcp_clk_mode(uint mpeg4_setting, uint h264_e_setting)
{
	uint setting = 0, value;

	value = readl(CONFIG_PMU_BASE + 0x28);

	setting = mpeg4_setting << 7;
	setting |= h264_e_setting << 18;
    writel((value & 0xFFF3FF7F) | setting, CONFIG_PMU_BASE + 0x28);
}

void set_PLL2(uint pll2_mul, uint pll2_div)
{
	uint pll2_out, tmp;
	
	tmp = readl(CONFIG_PMU_BASE+PMU_PLL23CR_OFFSET);
	tmp &= 0xFFFE0000;
	pll2_out = SYS_CLK / pll2_div * pll2_mul / 1000000;
 	if(pll2_out > 500) 
  		tmp |= (BIT1 | BIT2);
  	else if(pll2_out > 250)
   		tmp |= BIT2;
 	else if(pll2_out > 125)
  		tmp |= BIT1;
  
    tmp |= ((pll2_div << 12) | (pll2_mul << 3)); 
    writel(tmp | BIT0, CONFIG_PMU_BASE + PMU_PLL23CR_OFFSET);
}

void ChangeCacheWAMode(int mode)
{
    uint val; 

    //Enable ALO
    __asm__ __volatile__("mrc p15, 0, %0, c1, c0, 1" : "=r" (val) : : "memory");
    val |= 0x8;
    __asm__ __volatile__("mcr p15, 0, %0, c1, c0, 1" : "=r" (val) : : "memory");
    __asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0" : "=r" (val) : : "memory");

    val &= ~0x14000;

    switch(mode){

        case 1: //RAO=0, STMWA=1
            val |= 0x10000;
            break;
        case 2: //RAO=0, STMWA=0
            break;
        case 0: //RAO=1, STMWA=X
        default:
            val |= 0x4000;
             break;
    } 

    __asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0" : "=r" (val) : : "memory");
    //Disable ALO
    __asm__ __volatile__("mrc p15, 0, %0, c1, c0, 1" : "=r" (val) : : "memory");
    val &= ~0x8;
    __asm__ __volatile__("mcr p15, 0, %0, c1, c0, 1" : "=r" (val) : : "memory");
}

// only handle PMU irq
void do_irq (struct pt_regs *pt_regs)
{
	uint val;

	*(volatile ulong *)(CONFIG_PMU_BASE + 0x20) = (1<<17);
	val = *(volatile ulong *)(CONFIG_PMU_BASE + 0x0C);
    val &= ~0x4;
    *(volatile ulong *)(CONFIG_PMU_BASE + 0x0C) = val;

	*(volatile ulong *)(CONFIG_INTC_BASE + 0x08) = (1<<IRQ_PMU);
	*(volatile ulong *)(CONFIG_INTC_BASE + 0x04) &= ~(1<<IRQ_PMU);
}

void FCS_go(unsigned int cpu3x, unsigned int mul, unsigned int div)
{
  uint setting, pllout;

  pllout = SYS_CLK / (div & 0x1f) * (mul & 0x1FF);
  pllout = pllout/1000000;

  /* pllns & pllms */
  setting = (cpu3x << 22) | (div << 12) | (mul << 3);

  /* pll frang */
  if (u32PMU_ReadDDRCLK() < 600)
      setting |= BIT31;
  
  if (pllout >= 50 && pllout < 125)
      setting |= (0x0 << 20);
  else if (pllout >= 125 && pllout < 250)
      setting |= (0x01 << 20);
  else if (pllout >= 250 && pllout < 500)
      setting |= (0x02 << 20);
  else
      setting |= (0x03 << 20);

  if(setting == (readl(CONFIG_PMU_BASE + PMU_PLL1CR_OFFSET) & 0xfffffff8))
  	return;		// the same
  
  writel(setting, CONFIG_PMU_BASE + PMU_PLL1CR_OFFSET);

    
	*(volatile ulong *)(CONFIG_INTC_BASE + 0x0C) &= ~(1<<IRQ_PMU);
	*(volatile ulong *)(CONFIG_INTC_BASE + 0x10) &= ~(1<<IRQ_PMU);
	*(volatile ulong *)(CONFIG_INTC_BASE + 0x04) |= (1<<IRQ_PMU);

	printf("go...");

	setting = readl(CONFIG_PMU_BASE + PMU_PMODE_OFFSET);
	writel(setting | (1<<2), CONFIG_PMU_BASE + PMU_PMODE_OFFSET);

	setting = 0;

  __asm__ __volatile__("mcr p15, 0, %0, c7, c0, 4" : "=r" (setting) : : "memory");
	__asm__ __volatile__ (" nop ":::"memory"); //return here from IDLE mode interrupt    
}


void init_sys_freq(void)
{
	uint reg_v;

	/* Configure AHBC01 for LCD priority high */
	writel(0x00000002, CONFIG_AHBC01_BASE + 0x80);

	/* Configure AHBC04/AHBC05 for Capture0-1 priority high */
	writel(0x00000006, CONFIG_AHBC04_BASE + 0x80);
	writel(0x00000006, CONFIG_AHBC05_BASE + 0x80);

	writel(readl(CONFIG_PMU_BASE + 0x08) & 0x000fffff, CONFIG_PMU_BASE + 0x08);	// PLL0 CP1 CP2 set to 00
	writel(readl(CONFIG_PMU_BASE + 0xc) | BIT12, CONFIG_PMU_BASE + 0xc);	// isp_Power_off,
	writel(0xf87ff2e3, CONFIG_PMU_BASE + 0x38); 
	writel(0xfbffaffe, CONFIG_PMU_BASE + 0x3c);	// cmos clock on
	writel(readl(CONFIG_PMU_BASE + 0x6c) | 0x00020000, CONFIG_PMU_BASE + 0x6c);	// cmos_clk_en = 1
	writel(readl(CONFIG_PMU_BASE + 0x7c) | 0xf, CONFIG_PMU_BASE + 0x7c);	// tve_Power_off,
	writel(readl(CONFIG_PMU_BASE + 0x90) | 0x00780000, CONFIG_PMU_BASE + 0x90);	// ADC_Power_off, 
	writel(readl(CONFIG_PMU_BASE + 0x9c) | 0x03007000, CONFIG_PMU_BASE + 0x9c);	// Audio_Power_off,
		
	/* mcp100clk_sel, Select the PLL1 output. */
	writel(readl(CONFIG_PMU_BASE + 0x28) & (~BIT7), CONFIG_PMU_BASE + 0x28);
	
	writel(0x200200, CONFIG_PMU_BASE + 0x44);	// tune clock
	ChangeCacheWAMode(0);    
	    
	/* Sync LCD and EXT_CLK#3 Clock for use EXT_CLK#3 as CT656_CLK source */
	/* Set Ext_CLK#3 to 27MHz */
	reg_v = readl(CONFIG_PMU_BASE + 0x70);
	reg_v &= ~(0x3f<<18);
	reg_v |= (0x09<<18);
	writel(reg_v, CONFIG_PMU_BASE + 0x70);
	
	/* Set LCD_CLK to 27MHz */
	reg_v = readl(CONFIG_PMU_BASE + 0x74);
	reg_v &= ~(0x3f<<20);
	reg_v |= (0x13<<20);
	writel(reg_v, CONFIG_PMU_BASE + 0x74);    
	        
	if (((GET_PLAT_FULL_ID() & 0xFFFFFFF0) == GPLAT_8126_1080P_ID)
		|| ((GET_PLAT_FULL_ID() & 0xFFFFFFF0) == GPLAT_8126_1080P_1_ID)){
		mcp_clk_mode(0, 2);  // mpeg4=HCLK, h264=Div_2.5 
		set_PLL2(99, 5);
		//FCS_go(444, 90, 3);// fclk/hclk=297MHz, h264 clk=360MHz, pll1/ddr=900MHz, must use DDR chip > 900MHz
		FCS_go(444, 80, 3);	 // fclk/hclk=297MHz, h264 clk=320MHz, pll1/ddr=800MHz	
	}
	else{
		mcp_clk_mode(0, 1);
		FCS_go(444, 80, 3);	 // fclk/hclk=270MHz, h264 clk=320MHz, pll1/ddr=800MHz
	}
}
#endif

#ifdef CONFIG_SYS_FLASH_CFI
ulong board_flash_get_legacy(ulong base, int banknum, flash_info_t *info)
{
	if (banknum == 0) {	/* non-CFI boot flash */
		info->portwidth = FLASH_CFI_8BIT;
		info->chipwidth = FLASH_CFI_BY8;
		info->interface = FLASH_CFI_X8;
		return 1;
	} else
		return 0;
}
#endif
