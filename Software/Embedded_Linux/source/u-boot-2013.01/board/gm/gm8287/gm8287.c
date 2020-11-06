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
extern int ftgmac100_initialize(bd_t *bd);
extern int ftgmac100_read_phy_register(const char *devname, unsigned char addr,
				  unsigned char reg, unsigned short *value);
extern int ftgmac100_write_phy_register(const char *devname, unsigned char addr,
				  unsigned char reg, unsigned short *value);
#else
extern int ftmac110_initialize(bd_t *bis);
#endif

/*
 * Miscellaneous platform dependent initialisations
 */

#define PMU_PMODE_OFFSET      0x0C
#define PMU_PLL1CR_OFFSET     0x30
#define PMU_PLL23CR_OFFSET    0x34

u32 ahbdma_cmd[5][2] = {
#ifdef CONFIG_CMD_NAND
    // DMA_MEM2NAND         ((1UL << 8))
    {
     (mDMACtrlTc1b(0) | mDMACtrlDMAFFTH3b(0) | mDMACtrlChpri2b(0) | mDMACtrlCache1b(0) | mDMACtrlBuf1b(0) | mDMACtrlPri1b(0) | mDMACtrlSsz3b(DMA_BURST_4) |     /* default value */
      mDMACtrlAbt1b(0) | mDMACtrlSwid3b(DMA_WIDTH_4) | mDMACtrlDwid3b(DMA_WIDTH_4) | mDMACtrlHW1b(1) | mDMACtrlSaddr2b(DMA_ADD_INC) |   //Memory is the source
      mDMACtrlDaddr2b(DMA_ADD_FIX) |    //NAND is the destination
      mDMACtrlSsel1b(1) | //AHB Master 1 is the source
      mDMACtrlDsel1b(0) | //AHB Master 0 is the destination
      mDMACtrlEn1b(1)),

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
      mDMACtrlSsz3b(DMA_BURST_128) |
      mDMACtrlAbt1b(0) |
      mDMACtrlSwid3b(DMA_WIDTH_4) |
      mDMACtrlDwid3b(DMA_WIDTH_4) |
      mDMACtrlHW1b(1) |
      mDMACtrlSaddr2b(DMA_ADD_FIX) |
      mDMACtrlDaddr2b(DMA_ADD_INC) | 
      mDMACtrlSsel1b(0) | //AHB Master 0 is the source
      mDMACtrlDsel1b(1) | //AHB Master 1 is the destination
      mDMACtrlEn1b(1)),

     (mDMACfgDst5b(0) |
      mDMACfgSrc5b(0x10 | REQ_NANDTX) | mDMACfgAbtDis1b(0) | mDMACfgErrDis1b(0) | mDMACfgTcDis1b(0))
     },
#else
    {0, 0},{0, 0},
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

     (mDMACfgDst5b(0x10 | REQ_SPI020RX) |      //bit13, 12-9; SSP0_TX:1
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
      mDMACfgSrc5b(0x10 | REQ_SPI020TX) |      //bit13, 12-9; SSP0_RX:0
      mDMACfgAbtDis1b(0) |      //Channel abort interrupt mask. 0 for not mask
      mDMACfgErrDis1b(0) |      //Channel error interrupt mask. 0 for not mask
      mDMACfgTcDis1b(0))        //Channel terminal count interrupt mask. 0 for not mask
     },
#else
    {0, 0},{0, 0},
#endif
    {0, 0}
};

/* mode:0 for NAND, mode:1 for SPI */
int platform_setup(int mode)
{
#ifndef CONFIG_CMD_FPGA	
    unsigned int reg = 0;

    /* Check for NAND function */
	reg = readl(CONFIG_PMU_BASE + 0x4);
	if((mode == 0) && (reg & (1 << 23))){
	    printf("not for NAND jump setting\n");
	}else if((mode == 1) && ((reg & (1 << 23)) == 0)){
	    printf("not for SPI jump setting\n");
	}
#endif
    return 0;
}

static unsigned int pmu_get_chip(void)
{
	unsigned int ID = 0;
	unsigned int product;

    ID = readl(CONFIG_PMU_BASE + 0x100) & 0xF;

    if((ID >> 2) == 0x2)
        product = 0x8287;
	else{
        switch (ID & 0x3) {
          case 0:
            product = 0x8282;
            break;
          case 1:
            product = 0x8283;
            break;
          case 2:
            product = 0x8286;
            break;
          case 3:
            product = 0x8287;
            break;		        
          default:
            break;
        }
	}
	return product;
}

unsigned int GET_PLAT_FULL_ID(void)
{
    unsigned int value, version;
    
    value = readl(CONFIG_PMU_BASE + 0x100);
    version = (value >> 8) & 0x3;
    version = (pmu_get_chip() << 16) | version;
    
    if(((value >> 2) & 0x3) == 0x2)
        version |= 0x20;
    
    return version;
}

#ifdef CONFIG_USE_IRQ
void intc_init(void)
{
    //only support FCS interrupt
	writel(0x100, CONFIG_INTC_BASE + 0x45C);
}
#endif

int board_init(void)
{
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_USE_IRQ
	intc_init();
#endif

#ifdef CONFIG_FTSMC020
	ftsmc020_init();	/* initialize Flash */
#endif

#ifdef CONFIG_CMD_NAND
    printf("NAND mode\n");
    if(platform_setup(0) < 0)
    	printf("NAND init fail\n");
#endif

#ifdef CONFIG_CMD_SPI
    printf("SPI mode\n");
    if(platform_setup(1) < 0)
    	printf("SPI init fail\n");
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

uint u32PMU_ReadPLL1CLK(void)
{
  uint value = 0;

  value = readl(CONFIG_PMU_BASE+0x30);
  value = (value >> 3) & 0x7F;

  return (SYS_CLK * value);
}

uint u32PMU_ReadPLL4CLK(void)
{
  uint value = 0;

  value = readl(CONFIG_PMU_BASE+0x38);
  value = (value >> 4) & 0x7F;

  return (SYS_CLK * value);
}

uint u32PMU_ReadHCLK(void)
{ 
  uint value = 0;

  value = readl(CONFIG_PMU_BASE + 0x30);
  value = (value >> 17) & 0x3;

  switch(value){
  case 0:
  	value = (u32PMU_ReadPLL1CLK() * 2) / 5;
  	break;
  case 1:
  	value = u32PMU_ReadPLL1CLK() / 3;
  	break;
  case 2:
  	value = u32PMU_ReadPLL1CLK() / 4;
  	break;
  case 3:
  	value = u32PMU_ReadPLL1CLK() / 5;
  	break;
  default:
  	printf("AHB not support this mode\n");
  	break;
	}

	return value;
}

uint u32PMU_ReadCPUCLK(void)
{
    unsigned int value = 0;

    value = (u32PMU_ReadPLL1CLK() * 2) / 3;

  	return value;
}

uint u32PMU_ReadAXICLK(void)
{
  return u32PMU_ReadCPUCLK() / 2;
}

uint u32PMU_ReadAPB0CLK(void)
{
    return u32PMU_ReadAXICLK() / 8;
}

uint u32PMU_ReadAPB1CLK(void)
{
    return u32PMU_ReadAXICLK() / 2;
}

uint u32PMU_ReadAPB2CLK(void)
{
    return u32PMU_ReadHCLK() / 4;
}

//#ifndef CONFIG_SKIP_LOWLEVEL_INIT

#define VGA_CONFIG	0	// default vga setting = 1024x768

struct vga_setting {
	uint ns;
	uint ms;
	char resolution[10];
};

struct vga_setting vgs[] =
{
	{54,  1, "1024x768"},
	{63,  1, "1280x800"},
	{135, 2, "1280x960"},
	{54,  1, "1280x1024"},
	{72,  1, "1360x768"},
	{99,  2, "720P"},
	{99,  2, "1080I"},
	{63,  1, "800x600"},
	{0,   0, ""}
};

uint vga_res = VGA_CONFIG;

uint u32PMU_ReadDDRCLK(void)
{
    unsigned int value = 0;

    value = readl(CONFIG_PMU_BASE + 0x30);
    if(value & BIT16)
    	value = u32PMU_ReadPLL1CLK() / 4;
    else
  		value = u32PMU_ReadPLL1CLK() / 3;

  	return (value * 4);
}

uint u32PMU_ReadJpegCLK(void)
{
    uint clk;
    clk = readl(CONFIG_PMU_BASE + 0x28);
    if(clk & BIT16)
        clk = u32PMU_ReadPLL1CLK() / 4;
    else
		clk = u32PMU_ReadHCLK();

    return clk;
}

uint u32PMU_ReadH264encCLK(void)
{
    uint clk;
    clk = readl(CONFIG_PMU_BASE + 0x28);
    if(clk & BIT31)
		clk = (u32PMU_ReadPLL4CLK() / 5) * 2;
    else
        clk = u32PMU_ReadAXICLK();

    return clk;
}

uint u32PMU_ReadH264decCLK(void)
{
    uint clk;
    clk = readl(CONFIG_PMU_BASE + 0x28);
    if(clk & BIT30)
		clk = (u32PMU_ReadPLL4CLK() / 5) * 2;
    else
        clk = u32PMU_ReadAXICLK();

    return clk;
}

void display_sys_freq(void)
{
	printf("\n-------------------------------\n");
	printf("ID:%8x\n", GET_PLAT_FULL_ID());
	printf("AC:%4d  HC:%4d  P1:%4d  P4:%4d\n",
	u32PMU_ReadAXICLK()/1000000, u32PMU_ReadHCLK()/1000000,
	u32PMU_ReadPLL1CLK()/1000000, u32PMU_ReadPLL4CLK()/1000000);
	printf("C6:%4d  DR:%4d\n",
	u32PMU_ReadCPUCLK()/1000000, u32PMU_ReadDDRCLK()/1000000);
	printf("J:%4d   H1:%4d  H2:%4d\n",
	u32PMU_ReadJpegCLK()/1000000, u32PMU_ReadH264encCLK()/1000000, u32PMU_ReadH264decCLK()/1000000);
	//printf("   VGA : %s\n", vgs[vga_res].resolution);
	printf("-------------------------------\n");
}

//#endif

int board_eth_init(bd_t *bd)//justin
{
#ifndef CONFIG_FTGMAC100
    return 0;
#else
	int ret; /* 0:external PHY, 1:internal PHY */
	unsigned int value;
	unsigned short phy_data;
	
  //GMAC PHY CLK source
  writel(readl(CONFIG_PMU_BASE + 0x28) & (~BIT29), CONFIG_PMU_BASE + 0x28);
  //Add driving
  writel(readl(CONFIG_PMU_BASE + 0x40) | (BIT13), CONFIG_PMU_BASE + 0x40);
    
#ifdef CONFIG_RMII
  printf("GMAC set RMII mode\n");

  //RMII mode pinmux
	value = readl(CONFIG_PMU_BASE + 0x50) & (~(0xF << 10));
	value = value | (0x5 << 10);
	writel(value, CONFIG_PMU_BASE + 0x50);
	writel(0x2, CONFIG_PMU_BASE + 0xD8);
	    
  //GMAC clock divided value, RMII GTX_CK 50M, UART 25M
	value = readl(CONFIG_PMU_BASE + 0x70) & 0xFFFF03E0;
	value = value | (0x1 << 10) | 14;
	writel(value, CONFIG_PMU_BASE + 0x70);
#endif

#ifdef CONFIG_RGMII
  printf("GMAC set RGMII mode\n");

  //RGMII mode pinmux
	value = readl(CONFIG_PMU_BASE + 0x50) & (~(0xF << 10));
	writel(value, CONFIG_PMU_BASE + 0x50);
	writel(0x5, CONFIG_PMU_BASE + 0xD8);
    
  //GMAC clock divided value, RGMII GTX_CK 125M, UART 25M
	value = readl(CONFIG_PMU_BASE + 0x70) & 0xFFFF03E0;
	value = value | (0x4 << 10) | 0x5;
	//value = value | 0x1D;
	writel(value, CONFIG_PMU_BASE + 0x70);
#endif	

  //GMAC clock out on
  writel(readl(CONFIG_PMU_BASE + 0xB4) & (~BIT14), CONFIG_PMU_BASE + 0xB4);
  
  //GPIO0 pin 1 reset PHY
  writel(BIT1, CONFIG_GPIO0_BASE);//data out
  writel(BIT1, CONFIG_GPIO0_BASE + 0x8);//pin direction out
  mdelay(1);
  writel(0, CONFIG_GPIO0_BASE);//data out
  mdelay(1000);
  printf("reset PHY\n");
  writel(BIT1, CONFIG_GPIO0_BASE);//data out
  mdelay(30);

	phy_data = 0x0123;
	ret= ftgmac100_initialize(bd);
#if 0	//initial LED mode
	/*                                   addr             reg   data */
	ftgmac100_write_phy_register("eth0", CONFIG_PHY_ADDR, 0x1f, 0x07);
	mdelay(10);
	ftgmac100_write_phy_register("eth0", CONFIG_PHY_ADDR, 0x13, 0xC00C);
	mdelay(10);
	ftgmac100_write_phy_register("eth0", CONFIG_PHY_ADDR, 0x11, 0x82);
	mdelay(10);
	ftgmac100_write_phy_register("eth0", CONFIG_PHY_ADDR, 0x1f, 0x0);
	mdelay(10);
#endif

#ifdef FTMAC_DEBUG
	ftgmac100_read_phy_register("eth0", CONFIG_PHY_ADDR, 0x1, &phy_data);
	
	printf("PHY reg(0x01)=%x\n",value);
	if(value &0x0020)
		printf("Auto-negotiation process completed\n");
	else
		printf("Auto-negotiation process NOT completed\n");
#endif
	return ret;
#endif
}

#ifdef CONFIG_USE_IRQ

// only handle PMU irq
void do_irq (struct pt_regs *pt_regs)
{
	uint val;

	writel(1 << 17, CONFIG_PMU_BASE + 0x20);
	val = readl(CONFIG_PMU_BASE + 0x0C);
  val &= ~0x4;
  writel(val, CONFIG_PMU_BASE + 0x0C);

	writel(1 << IRQ_PMU, CONFIG_INTC_BASE + 0x08);
	*(volatile ulong *)(CONFIG_INTC_BASE + 0x04) &= ~(1<<IRQ_PMU);
}

void PMU_enter_fcs_mode(uint cpu_value, uint mul, uint div)
{
	uint setting, ddrclk;

	ddrclk = (u32PMU_ReadDDRCLK() * 2) / 1000000;

	/* pllns & pllms */
	setting = (cpu_value << 16) | (mul << 3);

	/* pll frang */
	if(ddrclk > 1000)
		setting |= (7 << 11);
	else if(ddrclk > 800)
		setting |= (6 << 11);
	else if(ddrclk > 700)
		setting |= (5 << 11);
	else if(ddrclk > 600)
		setting |= (4 << 11);
	else if(ddrclk > 500)
		setting |= (3 << 11);
	else if(ddrclk > 400)
		setting |= (2 << 11);
	else if(ddrclk > 300)
		setting |= (1 << 11);

	if(setting == (readl(CONFIG_PMU_BASE + 0x30) & 0xfffffff8))
		return;		// the same

	writel(setting, CONFIG_PMU_BASE + 0x30);

	*(volatile ulong *)(CONFIG_INTC_BASE + 0x0C) &= ~(1<<IRQ_PMU);
	*(volatile ulong *)(CONFIG_INTC_BASE + 0x10) &= ~(1<<IRQ_PMU);
	*(volatile ulong *)(CONFIG_INTC_BASE + 0x04) |= (1<<IRQ_PMU);

	printf("go...");

	setting = readl(CONFIG_PMU_BASE + 0x0C);
	writel(setting | (1 << 2), CONFIG_PMU_BASE + 0x0C);

	setting = 0;
	__asm__ __volatile__ (
                " mcr p15, 0, %0, c7, c0, 4 " :
                "=r"(setting)::
                "memory"
            );
	__asm__ __volatile__ (" nop ":::"memory"); //return here from IDLE mode interrupt
}

void FCS_go(unsigned int cpu_mode, unsigned int pll1_mul, unsigned int pll4_mul)
{
	unsigned int tmp;

  tmp = readl(CONFIG_PMU_BASE + 0x38) & 0xFFFFF80F;
  tmp |= (pll4_mul << 0x4);
  writel(tmp, CONFIG_PMU_BASE + 0x38);

	*(volatile ulong *)(CONFIG_INTC_BASE + 0x08) = (1<<IRQ_PMU);

	PMU_enter_fcs_mode(cpu_mode, pll1_mul, 1);
}
#endif

unsigned int commandline_mode(void)
{
	unsigned int ID = 0, product;

	ID = readl(CONFIG_PMU_BASE + 0x100) & 0xF;
	
	if((ID >> 2) == 0x2)
			product = 0x8287;
	else{
			switch (ID & 0x3) {
		      case 0:
		        product = 0x8282;
		        break;
		      case 1:
		        product = 0x8283;
		        break;
		      case 2:
		        product = 0x8286;
		        break;
		      case 3:
		        product = 0x8287;
		        break;		        
		      default:
		        break;
		    }
	}
	return product;
}

void init_sys_freq(void)
{
	//unsigned int data;

    // Open  AXI/AHB/APB clock
	//writel(0x0FC3807C, CONFIG_PMU_BASE + 0xB0);//AXI gating clock

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
}
