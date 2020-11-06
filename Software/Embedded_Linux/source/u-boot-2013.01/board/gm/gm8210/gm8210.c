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
#include <pci.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_FTGMAC100
extern int ftgmac100_initialize(bd_t *bd);
extern int ftgmac100_read_phy_register(const char *devname, unsigned char addr,
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
      mDMACtrlSsel1b(1) | mDMACtrlDsel1b(0) | mDMACtrlEn1b(1)),

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
      mDMACtrlDaddr2b(DMA_ADD_INC) | mDMACtrlSsel1b(0) | mDMACtrlDsel1b(1) | mDMACtrlEn1b(1)),

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
      mDMACtrlSsel1b(1) |       //AHB Master 0 is the source
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
      mDMACtrlDsel1b(1) |       //AHB Master 0 is the destination
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

#ifdef CONFIG_CMD_NAND
int platform_setup(void)
{
    unsigned int reg = 0;

    /* Check for NAND function */
	reg = readl(CONFIG_PMU_BASE + 0x5C);
	if((reg & (1 << 24)) == 0)
	    return -1;
    /* Turn on NAND clock */
    reg = readl(CONFIG_PMU_BASE + 0xB4);
    reg &= ~(0x1 << 9);
    writel(reg, CONFIG_PMU_BASE + 0xB4);
    /* PinMux with GPIO */
    reg = readl(CONFIG_PMU_BASE + 0x5C);
    reg |= (0x1 << 27);
    writel(reg, CONFIG_PMU_BASE + 0x5C);

    return 0;
}
#endif

#define GET_PLAT_FULL_ID()		(readl(CONFIG_PMU_BASE))

void intc_init(void)
{
    //only support FCS interrupt
	writel(0x100, CONFIG_INTC_BASE + 0x45C);
}

// used by 8210 PCIE RC port
#define LTSSM           0x44
#define IMASK           0x140
#define ISTATUS         0x144
#define CFG_CONTROL     0x160
#define CFG_WRITE_DATA  0x164
#define CFG_READ_DATA   0x168
#define IO_CONTROL      0x170
#define CFG_FBE         0x180

unsigned int cfg_pcie(unsigned pcie_base, unsigned bus_num, unsigned dev_num, unsigned fun_num, unsigned reg_ofst, unsigned msg_type, unsigned cfg_wdata, unsigned cfg_be)
{
    uint rdata;
    uint reg_num;
    uint retry = 100;

    reg_num = reg_ofst >> 2;

    writel(0x200, pcie_base + IMASK); // set bit[9] of PCIE controller IMASK register

    if (msg_type==1)
        writel(cfg_wdata, pcie_base + CFG_WRITE_DATA);

    writel(cfg_be, pcie_base + CFG_FBE);
    writel((bus_num<<24) | (dev_num<<19) | (fun_num<<16) | (reg_num<<6) | (msg_type<<2) | (1<<1), pcie_base + CFG_CONTROL); // set bit[15:6]=0:register number, bit[2]=0:configure read, bit[1]=1:start

    // Polling pcie_rootport_irq
    while (!(readl(pcie_base + ISTATUS) & 0x200) && --retry) {
        __udelay(10);
    }

    // clear Configuration End interrupt
    writel(1, pcie_base + CFG_CONTROL); // set bit[0]=1:stop
    writel(0x200, pcie_base + ISTATUS);
    writel(0, pcie_base + IMASK);

    if (msg_type == 0)
    {
        rdata = readl(pcie_base + CFG_READ_DATA);
        return rdata;
    }
    else
        return 1;
}

void pcie_read_cfg(unsigned pcie_base, unsigned bus_num, unsigned dev_num, unsigned fun_num, unsigned reg_ofst, unsigned *cfg_rdata, unsigned cfg_be)
{
    *cfg_rdata = cfg_pcie(pcie_base, bus_num, dev_num, fun_num, reg_ofst, 0, 0, cfg_be);
}

void pcie_write_cfg(unsigned pcie_base, unsigned bus_num, unsigned dev_num, unsigned fun_num, unsigned reg_ofst, unsigned cfg_wdata, unsigned cfg_be)
{
    cfg_pcie(pcie_base, bus_num, dev_num, fun_num, reg_ofst, 1, cfg_wdata, cfg_be);
}

/* init gm8210 pcie RC and gm8312 pcie EP */
void pcie_init(void)
{
    uint tmp, bus, dev, fun, rdata, wdata, be;
    uint us_delay = 50000; // us, at lease 33000 for 8312 reset, default 50000
    uint pcie_base = CONFIG_PCIE_BASE;

    //printf("\n");
    // select 12MHz source for 8312
    tmp = readl(CONFIG_PMU_BASE + 0x60);
    tmp &= ~(3 << 12);
    tmp |= (2 << 12);
    writel(tmp, CONFIG_PMU_BASE + 0x60);

    __udelay(us_delay);

    // disable GM8310 PLL5 clock
    //printf(">>>>>> disable GM8310 PLL5 clock......\n");
    tmp = readl(CONFIG_PMU_BASE + 0x34);
    tmp &= ~BIT16;
    writel(tmp, CONFIG_PMU_BASE + 0x34);

    // assert GM8310 PCIE controller aresetn, presetn, npor.
    // assert GM8310 PCIE PHY AUX_RESETn, RESETn.
    //printf(">>>>>> reset GM8310 PCIe controller/PHY......\n");
    tmp = readl(CONFIG_PMU_BASE + 0xA0);
    tmp &= ~(0x37 << 20);
    writel(tmp, CONFIG_PMU_BASE + 0xA0);

    // De-assert GM8310 PCIE controller aresetn, presetn, npor.
    //printf(">>>>>> release reset GM8310 PCIe controller......\n");
    tmp = readl(CONFIG_PMU_BASE + 0xA0);
    tmp |= (0x7 << 20);
    writel(tmp, CONFIG_PMU_BASE + 0xA0);

    // enable GM8310 PLL5 clock
    //printf(">>>>>> enable GM8310 PLL5 clock......\n");
    tmp = readl(CONFIG_PMU_BASE + 0x34);
    tmp |= BIT16;
    writel(tmp, CONFIG_PMU_BASE + 0x34);

    __udelay(us_delay);

    // enable PCIe PHY REG (XCfg_TxRegOn)
    //printf(">>>>>> enable XCfg_TxRegOn......\n");
    tmp = readl(CONFIG_PMU_BASE + 0x88);
    tmp |= BIT19; // set Xcfg_TxRegOn=1
    writel(tmp, CONFIG_PMU_BASE + 0x88);

    __udelay(us_delay);

    // De-assert GM8310 PCIE PHY AUX_RESETn, RESETn.
    //printf(">>>>>> release reset GM8310 PCIe PHY......\n");
    tmp = readl(CONFIG_PMU_BASE + 0xA0);
    tmp |= (0x30 << 20);
    writel(tmp, CONFIG_PMU_BASE + 0xA0);

    __udelay(us_delay);

    // assert GM8312(Analog Die) resetn. (default reset active, BIT23=0)
    tmp = readl(CONFIG_PMU_BASE + 0xA0);
    tmp &= ~BIT23;
    writel(tmp, CONFIG_PMU_BASE + 0xA0);

    __udelay(us_delay);

    // De-assert GM8312(Analog Die) resetn. (default reset active, BIT23=0)
    tmp = readl(CONFIG_PMU_BASE + 0xA0);
    tmp |= BIT23;
    writel(tmp, CONFIG_PMU_BASE + 0xA0);

    // waiting for gm8312 digital part be ready, must be greater than 33ms at 12MHz
    __udelay(us_delay);

    // Waiting for GM8310 PCIe controller enter LTSSM L0 state.
    //printf(">>>>>> waiting for GM8310 PCIe controller enter LTSSM L0 state......");
    while (((readl(pcie_base + LTSSM) >> 24) & 0xF) != 0xF);
    //printf("done\n");

    __udelay(us_delay);

    /* disable k_fix[10,11] to config AXIM_WINDOW(0,1) */
    tmp = readl(pcie_base + 0x1004);
    tmp &= ~(BIT10 | BIT11);
    writel(tmp, pcie_base + 0x1004);

    writel(0x00000000, pcie_base + 0x100); // set base address AXI
    writel(0x80000001, pcie_base + 0x104); // set size of window and enable bit
    writel(0x00000001, pcie_base + 0x108); // set LSB of base address PCIe and select BAR0
    writel(0x00000000, pcie_base + 0x10C); // set MSB of base address PCIe

    writel(0x80000000, pcie_base + 0x110); // set base address AXI
    writel(0x80000001, pcie_base + 0x114); // set size of window and enable bit
    writel(0x00000002, pcie_base + 0x118); // set LSB of base address PCIe and select BAR1
    writel(0x00000000, pcie_base + 0x11C); // set MSB of base address PCIe

    /* check 8210 pcie rc port */
    bus = 0; // 0: RC, 1: EP
    dev = 0;
    fun = 0;
    be = 0xF;
    do {
        pcie_read_cfg(pcie_base, bus, dev, fun, PCI_VENDOR_ID, &rdata, be);
        //printf(">>>>>> get 8210 pcie rc DID&VID = 0x%x\n", rdata);
    } while (rdata != 0x8210188B);

    pcie_read_cfg(pcie_base, bus, dev, fun, PCI_COMMAND, &rdata, be);
    rdata &= ~0xFFFF;
    wdata = rdata | 0x0147;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_COMMAND, wdata, be);
#if 1
    pcie_read_cfg(pcie_base, bus, dev, fun, PCI_CACHE_LINE_SIZE, &rdata, be);
    rdata &= ~0xFF;
    wdata = rdata | 0x08;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_CACHE_LINE_SIZE, wdata, be);
#endif
    wdata = 0x00000000;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_BASE_ADDRESS_0, wdata, be);
    wdata = 0x80000000;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_BASE_ADDRESS_1, wdata, be);
    pcie_read_cfg(pcie_base, bus, dev, fun, PCI_PRIMARY_BUS, &rdata, be);
    rdata &= ~0xFFFFFFFF;
    wdata = rdata | (1 << 8) | (1 << 16);
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_PRIMARY_BUS, wdata, be);
#if 0
    pcie_read_cfg(pcie_base, bus, dev, fun, PCI_IO_BASE, &rdata, be);
    rdata &= ~0xFFFF;
    wdata= rdata | 0x01F1;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_IO_BASE, wdata, be);
    wdata = 0xBFF0B000;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_MEMORY_BASE, wdata, be);
    wdata = 0x0001FFF1;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_PREF_MEMORY_BASE, wdata, be);
#endif
    /* disable MSI generation */
    pcie_read_cfg(pcie_base, bus, dev, fun, 0x50, &rdata, be); // read MSI Capability
    wdata = rdata | (1 << 16);
    pcie_write_cfg(pcie_base, bus, dev, fun, 0x50, wdata, be);

    /* config 8312 pcie ep configuration space*/
    bus = 1;
    dev = 0;
    do {
        pcie_read_cfg(pcie_base, bus, dev, fun, PCI_VENDOR_ID, &rdata, be);
        //printf(">>>>>> get 8312 pcie ep DID&VID = 0x%x\n", rdata);
    } while (rdata != 0x83121556);

    pcie_read_cfg(pcie_base, bus, dev, fun, PCI_COMMAND, &rdata, be);
    rdata &= ~0xFFFF;
    wdata = rdata | 0x0147;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_COMMAND, wdata, be);
#if 1
    pcie_read_cfg(pcie_base, bus, dev, fun, PCI_CACHE_LINE_SIZE, &rdata, be);
    rdata &= ~0xFF;
    wdata = rdata | 0x08;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_CACHE_LINE_SIZE, wdata, be);
#endif
    /* default base of IP on 8312 */
    wdata = 0xA0000000;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_BASE_ADDRESS_0, wdata, be);
    /* pre-define for new address remap on 8312, change base 0xA0000000 to 0xC0000000 */
    wdata = 0xC0000000;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_BASE_ADDRESS_1, wdata, be);
    /* disable MSI generation */
    pcie_read_cfg(pcie_base, bus, dev, fun, 0x50, &rdata, be); // read MSI Capability
    wdata = rdata | (1 << 16);
    pcie_write_cfg(pcie_base, bus, dev, fun, 0x50, wdata, be);

    /* wait for 8312 pcie ready */
    //printf(">>>>>> wait 8312 data ready through PCIe......");
    while (1) {
        if (((readl(CONFIG_8312_PMU_EXBASE + 0x00) >> 16) & 0xFFFF) == 0x8312)
            break;
    }
    //printf("done\n");

#ifdef CONFIG_MAP_PCIE_ADDR_A2C
    /* enable 8312 axic and pcie0 apb clock*/
    tmp = readl(CONFIG_8312_PMU_EXBASE + 0x34);
    tmp &= ~(BIT1 | BIT23);
    writel(tmp, CONFIG_8312_PMU_EXBASE + 0x34);

    /* extra operation to config new IP base on 8312 */
    /* disable k_fix[11,12,13] to config AXIM_WINDOW(1,2,3) */
    tmp = readl(CONFIG_8312_PCIE_0_EXBASE + 0x80004);
    tmp &= ~(BIT11 | BIT12 | BIT13);
    writel(tmp, CONFIG_8312_PCIE_0_EXBASE + 0x80004);

    writel(0xC0000000, CONFIG_8312_PCIE_0_EXBASE + 0x800A4); // set BAR1 size to 1GB

    writel(0xA0000000, CONFIG_8312_PCIE_0_EXBASE + 0x110); // set base address AXI
    writel(0xE0000001, CONFIG_8312_PCIE_0_EXBASE + 0x114); // set size of window and enable bit
    writel(0x00000002, CONFIG_8312_PCIE_0_EXBASE + 0x118); // set LSB of base address PCIe and select BAR1
    writel(0x00000000, CONFIG_8312_PCIE_0_EXBASE + 0x11C); // set MSB of base address PCIe

    writel(0xE0000000, CONFIG_8312_PCIE_0_EXBASE + 0x120); // set base address AXI
    writel(0xE0000001, CONFIG_8312_PCIE_0_EXBASE + 0x124); // set size of window and enable bit
    writel(0x20000002, CONFIG_8312_PCIE_0_EXBASE + 0x128); // set LSB of base address PCIe and select BAR0
    writel(0x00000000, CONFIG_8312_PCIE_0_EXBASE + 0x12C); // set MSB of base address PCIe

    /* set 8312_AXIC slave5 (PCIe1) base */
    writel(0xE0090000, CONFIG_8312_AXI_EXBASE + 0x2C);
    /* set AXIC slave2 base on 8210 */
    writel(0xC00A0000, CONFIG_CPU_IC0_BASE + 0x20);
    /* set AXIC_9 slave1 port8 base on 8210*/
    writel(0xC00A0000, CONFIG_PCIE_IC9_BASE + 0x1C);
    /* set AXIC_3 slave1 base on 8210*/
    writel(0xC00A0000, CONFIG_CAP_IC3_BASE + 0x00);
    /* set AXIC_4 slave1 base on 8210*/
    writel(0xC00A0000, CONFIG_ENC0_IC4_BASE + 0x00);
    /* set AXIC_5 slave1 base on 8210*/
    writel(0xC00A0000, CONFIG_ENC1_IC5_BASE + 0x00);
    /* set AXIC_8 slave2 base on 8210*/
    writel(0xC00A0000, CONFIG_3D_IC8_BASE + 0x20);

    /* after now, the domain base of 8312 is mapped to 0xC0000000 on local 8210 */
#endif
}

void gm8312_clk_init(void)
{
    uint tmp;

    /* enable 8312 default clock */
    tmp = readl(CONFIG_8312_PMU_BASE + 0x30);
    tmp &= 0xFFFFDAFF;
    writel(tmp, CONFIG_8312_PMU_BASE + 0x30);

    tmp = readl(CONFIG_8312_PMU_BASE + 0x34);
    tmp &= 0xFE7E00F9;
    writel(tmp, CONFIG_8312_PMU_BASE + 0x34);

#ifdef CONFIG_CHIP_MULTI
    /* set 8312 RC VC0 credit */
    writel(0x003F000F, CONFIG_8312_PCIE_1_BASE + 0x800C4);
#endif
}

void platform_private_init(void)
{
    uint tmp;

    /* disable AXI slave port interleaving control function */
    tmp = readl(CONFIG_PCIE_IC9_BASE + 0x12C);
    tmp |= BIT1;
    writel(tmp, CONFIG_PCIE_IC9_BASE + 0x12C);

    tmp = readl(CONFIG_8312_AXI_BASE + 0x12C);
    tmp |= (BIT1 | BIT5);
    writel(tmp, CONFIG_8312_AXI_BASE + 0x12C);

    /* set AXIC_8 slave1 port1~6 base on 8210
     * default value is overlap to ddr1 base
     */
    writel(0x80400000, CONFIG_3D_IC8_BASE + 0x00);
    writel(0x80500000, CONFIG_3D_IC8_BASE + 0x04);
    writel(0x80600000, CONFIG_3D_IC8_BASE + 0x08);
    writel(0x80700000, CONFIG_3D_IC8_BASE + 0x0C);
    writel(0x80800000, CONFIG_3D_IC8_BASE + 0x10);
    writel(0x80900000, CONFIG_3D_IC8_BASE + 0x14);

#if 0 //#ifdef CONFIG_CHIP_MULTI
    /* enable gm8312 pcie1 phy internal clock */
    tmp = readl(CONFIG_8312_PMU_BASE + 0x54);
    tmp |= BIT1;
    writel(tmp, CONFIG_8312_PMU_BASE + 0x54);
#endif
}


#define SLAVE_DDR_INIT_DONE     0x434F4D45

uint slave_gm8210_quantity = 0; // record the quantity of slave gm8210 device

/* init gm8312 pcie RC and slave gm8210 pcie EP */
void gm8312_pcie_init(void)
{
    uint tmp, bus, dev, fun, rdata, wdata, be;
    uint pcie_base = CONFIG_8312_PCIE_1_BASE;
    int retry = 100;

    /* config 8312 pcie rc BAR0/BAR1 size */
    writel(0x80000000, pcie_base + 0x800A0); // set BAR0 size to 2GB
    writel(0x80000000, pcie_base + 0x800A4); // set BAR1 size to 2GB

    /* disable k_fix[10,11] to config AXIM_WINDOW(0,1) */
    tmp = readl(pcie_base + 0x80004);
    tmp &= ~(BIT10 | BIT11);
    writel(tmp, pcie_base + 0x80004);

    writel(0x00000000, pcie_base + 0x100); // set base address AXI
    writel(0x80000001, pcie_base + 0x104); // set size of window and enable bit
    writel(0x00000001, pcie_base + 0x108); // set LSB of base address PCIe and select BAR0
    writel(0x00000000, pcie_base + 0x10C); // set MSB of base address PCIe

    writel(0x80000000, pcie_base + 0x110); // set base address AXI
    writel(0x80000001, pcie_base + 0x114); // set size of window and enable bit
    writel(0x00000002, pcie_base + 0x118); // set LSB of base address PCIe and select BAR1
    writel(0x00000000, pcie_base + 0x11C); // set MSB of base address PCIe

    /* check 8312 pcie rc port */
    bus = 0; // 0: RC, 1: EP
    dev = 0;
    fun = 0;
    be = 0xF;
    do {
        pcie_read_cfg(pcie_base, bus, dev, fun, PCI_VENDOR_ID, &rdata, be);
        //printf(">>>>>> get 8312 pcie rc DID&VID = 0x%x\n", rdata);
    } while (rdata != 0x83121556);

    pcie_read_cfg(pcie_base, bus, dev, fun, PCI_COMMAND, &rdata, be);
    rdata &= ~0xFFFF;
    wdata = rdata | 0x0147;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_COMMAND, wdata, be);
#if 1
    pcie_read_cfg(pcie_base, bus, dev, fun, PCI_CACHE_LINE_SIZE, &rdata, be);
    rdata &= ~0xFF;
    wdata = rdata | 0x08;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_CACHE_LINE_SIZE, wdata, be);
#endif
    wdata = 0x00000000;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_BASE_ADDRESS_0, wdata, be);
    wdata = 0x80000000;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_BASE_ADDRESS_1, wdata, be);
    pcie_read_cfg(pcie_base, bus, dev, fun, PCI_PRIMARY_BUS, &rdata, be);
    rdata &= ~0xFFFFFFFF;
    wdata = rdata | (1 << 8) | (1 << 16);
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_PRIMARY_BUS, wdata, be);
#if 0
    pcie_read_cfg(pcie_base, bus, dev, fun, PCI_IO_BASE, &rdata, be);
    rdata &= ~0xFFFF;
    wdata= rdata | 0x01F1;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_IO_BASE, wdata, be);
    wdata = 0xBFF0B000;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_MEMORY_BASE, wdata, be);
    wdata = 0x0001FFF1;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_PREF_MEMORY_BASE, wdata, be);
#endif
    /* disable MSI generation */
    pcie_read_cfg(pcie_base, bus, dev, fun, 0x50, &rdata, be); // read MSI Capability
    wdata = rdata | (1 << 16);
    pcie_write_cfg(pcie_base, bus, dev, fun, 0x50, wdata, be);

    __udelay(50000); /* wait for slave 8210 power on ready if existing */

    /* config slave 8210 pcie ep configuration space*/
    bus = 1;
    dev = 0;
    do {
        pcie_read_cfg(pcie_base, bus, dev, fun, PCI_VENDOR_ID, &rdata, be);
        //printf(">>>>>> get slave 8210 pcie ep DID&VID = 0x%x\n", rdata);
    } while ((rdata != 0x8210188B) && (--retry != 0));

    if (retry == 0) {
        //printf(">>>>>> Slave 8210 not exists\n");
        return;
    }

    slave_gm8210_quantity++;

    pcie_read_cfg(pcie_base, bus, dev, fun, PCI_COMMAND, &rdata, be);
    rdata &= ~0xFFFF;
    wdata = rdata | 0x0147;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_COMMAND, wdata, be);
#if 1
    pcie_read_cfg(pcie_base, bus, dev, fun, PCI_CACHE_LINE_SIZE, &rdata, be);
    rdata &= ~0xFF;
    wdata = rdata | 0x08;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_CACHE_LINE_SIZE, wdata, be);
#endif
    wdata = SLAVE_8210_PCICFG_BAR0_BASE;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_BASE_ADDRESS_0, wdata, be);
    wdata = SLAVE_8210_PCICFG_BAR1_BASE;
    pcie_write_cfg(pcie_base, bus, dev, fun, PCI_BASE_ADDRESS_1, wdata, be);
    /* disable MSI generation */
    pcie_read_cfg(pcie_base, bus, dev, fun, 0x50, &rdata, be); // read MSI Capability
    wdata = rdata | (1 << 16);
    pcie_write_cfg(pcie_base, bus, dev, fun, 0x50, wdata, be);

    __udelay(100000);

    /* wait for salve 8210 pcie ep ready */
    //printf(">>>>>> wait slave 8210 data ready through PCIe......");
    while (1) {
        if (((readl(SLAVE_8210_SCU_BASE + 0x00) >> 16) & 0xFFFF) == 0x8210)
            break;
    }
    //printf("done\n");

    /* for BAR0, map offset 0x08000000 to local address 0x00000000, total len 128MB  */
    writel(0x00000000, SLAVE_8210_PCIE_BASE + 0x120); // set base address AXI
    writel(0xF8000001, SLAVE_8210_PCIE_BASE + 0x124); // set size of window and enable bit
    writel(0x08000001, SLAVE_8210_PCIE_BASE + 0x128); // set LSB of base address PCIe and select BAR
    writel(0x00000000, SLAVE_8210_PCIE_BASE + 0x12C); // set MSB of base address PCIe

    writel(0xF0000000, SLAVE_8210_PCIE_BASE + 0x1060); // set BAR1 size to 256MB

    /* for BAR1, map offset 0x00000000 to local address 0x00000000, total len 256MB  */
    writel(0x00000000, SLAVE_8210_PCIE_BASE + 0x130); // set base address AXI
    writel(0xF0000001, SLAVE_8210_PCIE_BASE + 0x134); // set size of window and enable bit
    writel(0x00000002, SLAVE_8210_PCIE_BASE + 0x138); // set LSB of base address PCIe and select BAR
    writel(0x00000000, SLAVE_8210_PCIE_BASE + 0x13C); // set MSB of base address PCIe

    /* disable k_fix[6,7,8,9] to config AXIS_WINDOW(0,1,2,3) */
    tmp = readl(SLAVE_8210_PCIE_BASE + 0x1004);
    tmp &= ~(BIT6 | BIT7 | BIT8 | BIT9);
    writel(tmp, SLAVE_8210_PCIE_BASE + 0x1004);

    /* on 2nd 8210, map AXI base 0xB/E0000000 to PCIe address 0x98000000, total len 64MB  */
    writel(SLAVE_8210_AXIS_WIN0_BASE, SLAVE_8210_PCIE_BASE + 0xC0); // set base address AXI Slave
    writel(0xFC000001, SLAVE_8210_PCIE_BASE + 0xC4); // set size of window and enable bit
    writel(0x98000000, SLAVE_8210_PCIE_BASE + 0xC8); // set LSB of base address PCIe
    writel(0x00000000, SLAVE_8210_PCIE_BASE + 0xCC); // set MSB of base address PCIe

    /* on 2nd 8210, map AXI base 0xC0000000 to PCIe address 0xA0000000, total len 512MB  */
    writel(0xC0000000, SLAVE_8210_PCIE_BASE + 0xD0); // set base address AXI Slave
    writel(0xE0000001, SLAVE_8210_PCIE_BASE + 0xD4); // set size of window and enable bit
    writel(0xA0000000, SLAVE_8210_PCIE_BASE + 0xD8); // set LSB of base address PCIe
    writel(0x00000000, SLAVE_8210_PCIE_BASE + 0xDC); // set MSB of base address PCIe

    /* on 2nd 8210, map AXI base 0xB/E8000000 to PCIe address 0x00000000, total len 128MB  */
    writel(SLAVE_8210_AXIS_WIN2_BASE, SLAVE_8210_PCIE_BASE + 0xE0); // set base address AXI Slave
    writel(0xF8000001, SLAVE_8210_PCIE_BASE + 0xE4); // set size of window and enable bit
    writel(0x00000000, SLAVE_8210_PCIE_BASE + 0xE8); // set LSB of base address PCIe
    writel(0x00000000, SLAVE_8210_PCIE_BASE + 0xEC); // set MSB of base address PCIe

    /* on 2nd 8210, map AXI base 0xF0000000 to PCIe address 0x00000000, total len 256MB  */
    writel(SLAVE_8210_AXIS_WIN3_BASE, SLAVE_8210_PCIE_BASE + 0xF0); // set base address AXI Slave
    writel(0xF0000001, SLAVE_8210_PCIE_BASE + 0xF4); // set size of window and enable bit
    writel(0x00000000, SLAVE_8210_PCIE_BASE + 0xF8); // set LSB of base address PCIe
    writel(0x00000000, SLAVE_8210_PCIE_BASE + 0xFC); // set MSB of base address PCIe

#ifdef CONFIG_MAP_PCIE_ADDR_A2C
    /* set AXIC slave2 base on 2nd 8210 */
    writel(0xC00A0000, SLAVE_8210_CPU_IC0_BASE + 0x20);
    /* set AXIC_9 slave1 port8 base on 2nd 8210*/
    writel(0xC00A0000, SLAVE_8210_PCIE_IC9_BASE + 0x1C);
    /* set AXIC_3 slave1 base on 2nd 8210*/
    writel(0xC00A0000, SLAVE_8210_CAP_IC3_BASE + 0x00);
    /* set AXIC_4 slave1 base on 2nd 8210*/
    writel(0xC00A0000, SLAVE_8210_ENC0_IC4_BASE + 0x00);
    /* set AXIC_5 slave1 base on 2nd 8210*/
    writel(0xC00A0000, SLAVE_8210_ENC1_IC5_BASE + 0x00);
    /* set AXIC_8 slave2 base on 2nd 8210*/
    writel(0xC00A0000, SLAVE_8210_3D_IC8_BASE + 0x20);
#endif

    /* set AXIC_8 slave1 port1~6 base on 2nd 8210
     * default value is overlap to ddr1 base
     */
    writel(0x80400000, SLAVE_8210_3D_IC8_BASE + 0x00);
    writel(0x80500000, SLAVE_8210_3D_IC8_BASE + 0x04);
    writel(0x80600000, SLAVE_8210_3D_IC8_BASE + 0x08);
    writel(0x80700000, SLAVE_8210_3D_IC8_BASE + 0x0C);
    writel(0x80800000, SLAVE_8210_3D_IC8_BASE + 0x10);
    writel(0x80900000, SLAVE_8210_3D_IC8_BASE + 0x14);
}

uint read_slave_8210_ddr_mask(void)
{
    return readl(SLAVE_8210_PCIE_BASE + 0x124);
}

uint read_slave_8210_ddr_map(void)
{
    return readl(SLAVE_8210_PCIE_BASE + 0x120);
}

void write_slave_8210_ddr_map(uint local_ddr_offset)
{
    uint win_mask;

    win_mask = readl(SLAVE_8210_PCIE_BASE + 0x124) & 0xFFFFF000;
    local_ddr_offset &= win_mask;
    writel(local_ddr_offset, SLAVE_8210_PCIE_BASE + 0x120); // set base address AXI of AXIM_WINDOW(2)
}

#define WINDOW_SIZE 0x08000000

void cp_to_slave(uint addr_src, uint addr_dst, uint size)
{
    printf("move ddr map to 0x%08x, offset 0x%x, size 0x%x\n", (addr_dst / WINDOW_SIZE) * WINDOW_SIZE, (addr_dst % WINDOW_SIZE), size);
    write_slave_8210_ddr_map((addr_dst / WINDOW_SIZE) * WINDOW_SIZE);
    if(size)
        memcpy((void *) (SLAVE_8210_DDR_BASE + (addr_dst % WINDOW_SIZE)), (void *) addr_src, size);
}

int do_loadsl (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    uint addr, size;

    if (argc != 3)
        return CMD_RET_USAGE;

    addr = (uint) simple_strtoul(argv[1], NULL, 16);
    size = (uint) simple_strtoul(argv[2], NULL, 16);

    cp_to_slave(addr, addr, size);

	return 0;
}

void trigger_ep(void)
{
    /* write starting address of linux and notify slave 8210 device to go */
    writel(DEVICE_LINUX_IMAGE_ADDR, SLAVE_8210_MCP100_SRAM2_BASE + 0x04);
    writel(SLAVE_BOOT_SIGNATURE, SLAVE_8210_MCP100_SRAM2_BASE);
}

void slave_gm8210_boot(void)
{
    char cmd_load_image[128];

    printf("\n");
    printf("Found slave GM8210 device, prepare booting sequence...\n");

#ifdef CONFIG_CMD_NAND
    sprintf(cmd_load_image, "nand read 0x%x 0x20000 0x6000", DDR_LOAD_IMAGE_ADDR);
#endif
#ifdef CONFIG_CMD_SPI
    sprintf(cmd_load_image, "sf probe 0:0;sf read 0x%x 0x1000 0x6000", DDR_LOAD_IMAGE_ADDR);
#endif
    //printf(">>> %s\n", cmd_load_image);

    if (run_command(cmd_load_image, 0) != 0) {
        printf(">>>>>> load nsboot image fail\n");
        return;
    }
    printf("\n");

    /* load nsboot to slave gm8210 mcp100 sram */
    memcpy((void *) SLAVE_8210_MCP100_SRAM0_BASE, (void *) DDR_LOAD_IMAGE_ADDR, 0x6000);
    writel(0x92000000, SLAVE_8210_MCP100_SRAM1_BASE + 0x04); /* set PC on slave 8210 device */
    writel(SLAVE_BOOT_SIGNATURE, SLAVE_8210_MCP100_SRAM1_BASE); /* notify slave 8210 device to go */

    /* wait for slave 8210 device ddr init complete */
    while (readl(SLAVE_8210_MCP100_SRAM2_BASE + 0x10) != SLAVE_DDR_INIT_DONE);

#if 0
#ifdef CONFIG_CMD_NAND
    sprintf(cmd_load_image, "nand read 0x%x x", DDR_LOAD_IMAGE_ADDR);
#endif
#ifdef CONFIG_CMD_SPI
    sprintf(cmd_load_image, "sf probe 0:0;sf read 0x%x x", DDR_LOAD_IMAGE_ADDR);
#endif
    //printf(">>> %s\n", cmd_load_image);

    if (run_command(cmd_load_image, 0) != 0) {
        printf(">>> command execute fail\n");
        return;
    }

    /* load linux to slave gm8210 ddr */
    memcpy((void *) (SLAVE_8210_DDR_BASE + DEVICE_LINUX_IMAGE_ADDR), (void *) DDR_LOAD_IMAGE_ADDR, 0x500000);
    //writel(DEVICE_LINUX_IMAGE_ADDR, SLAVE_8210_MCP100_SRAM2_BASE + 0x04); /* set PC on slave 8210 device */
    writel(SLAVE_BOOT_SIGNATURE, SLAVE_8210_MCP100_SRAM2_BASE); /* notify slave 8210 device to go */
#endif
}

/* for 726 CPU, mode = 1 is write alloc enable  */
void set_write_alloc(u32 setting)
{
	__asm__ __volatile__ (" mcr p15, 0, %0, c15, c0, 0\n" : : "r"(setting));
}

u32 get_write_alloc(void)
{
	u32 data;

	__asm__ __volatile__ (" mrc p15, 0, %0, c15, c0, 0 " : "=r"(data));
//__asm__ __volatile__ (" mrc p15, 0, %0, c0, c0, 0 " : "=r"(data));//read ID
    return data;
}

int board_init(void)
{
    unsigned int value, i, *ptr;

    /* begin, do not rearrange the following sequence */
    printf("\nInit 8210 PCIe RC...");
    pcie_init(); /* init gm8210 pcie rc and gm8312 pcie ep */
    printf("done\n\n");
    gm8312_clk_init();
    platform_private_init();
    /* end */
#ifdef CONFIG_GM_SINGLE
    slave_gm8210_quantity = 0;
#else
#ifdef CONFIG_CHIP_MULTI
    printf("Init 8312 PCIe RC...");
    gm8312_pcie_init(); /* init gm8312 pcie rc and slave gm8210 pcie ep */
    printf("done\n\n");
#endif
#endif

	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	intc_init();

#ifdef CONFIG_FTSMC020
	ftsmc020_init();	/* initialize Flash */
#endif

#ifdef CONFIG_CMD_NAND
    printf("NAND mode\n");
    if(platform_setup() < 0)
    	printf("nand init fail\n");
#else
    printf("SPI mode\n");
#endif
		//enable write alloc
		value = get_write_alloc() | (1 << 20);
		set_write_alloc(value);

    //turn on mcp100 gate clock
    value = readl(CONFIG_PMU_BASE + 0xB4);
    value &= ~(0x1 << 12);
    writel(value, CONFIG_PMU_BASE + 0xB4);

    /* this memory will be used by CPUCOMM. When software reset happens, the memory
     * is not reset. Thus we need to clear it first.
     */
    ptr = (unsigned int *) CONFIG_MCP100_BASE;
    for (i = 0; i < (0x200 / 4); i ++)
        ptr[i] = 0;

    /* this memory will be used for multiple CPU accessing the same controller. */
    ptr = (unsigned int *) CONFIG_HDCP_BASE;
    for (i = 0; i < (0x200 / 4); i ++)
        ptr[i] = 0;

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
	unsigned int tmp;

    tmp = (readl(CONFIG_PMU_BASE + 0x38) >> 4 ) & 0x7F;

    return (tmp * SYS_CLK);
}

uint u32PMU_ReadHCLK(void)
{
#ifndef CONFIG_CMD_FPGA
  uint value = 0;

  value = readl(CONFIG_PMU_BASE+0x30);
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
    if((GET_PLAT_FULL_ID() >> 8) & 0xF)
        value = u32PMU_ReadPLL4CLK() / 3;
    else
  	    value = u32PMU_ReadPLL1CLK() / 5;
  	break;
  default:
  	printf("AHB not support this mode\n");
  	break;
	}

	return value;
#else
		return 20000000;
#endif
}

uint u32PMU_ReadAXICLK(void)
{
  uint value = 0;

  value = readl(CONFIG_PMU_BASE+0x30);

  if(value & BIT19)
  	value = u32PMU_ReadPLL1CLK() / 3;
  else
  	value = (u32PMU_ReadPLL1CLK() * 2) / 5;

  return value;
}

uint u32PMU_ReadPCLK(void)
{
#ifndef CONFIG_CMD_FPGA
    return (u32PMU_ReadAXICLK() >> 3);//APB0 = AXI / 8
#else
    return u32PMU_ReadHCLK();
#endif
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


uint u32PMU_ReadPLL2CLK(void)
{
	printf("GM8210 not use PLL2\n");
	return 0;
}

uint u32PMU_ReadPLL3CLK(void)
{
    uint mul;

    mul = ((readl(CONFIG_PMU_BASE + 0x34)) >>  4) & 0xff;

    return (SYS_CLK * mul / 2);
}

uint u32PMU_ReadDDRCLK(void)
{
    unsigned int value = 0;

    value = readl(CONFIG_PMU_BASE + 0x30);
    if(value & BIT16)
    	value = u32PMU_ReadPLL1CLK() / 3;
    else
  		value = (u32PMU_ReadPLL1CLK() * 2) / 5;

  	return (value * 4);
}

uint u32PMU_ReadMasterCPUCLK(void)
{
  unsigned int value = 0;

  value = readl(CONFIG_PMU_BASE+0x30);
  if(value & BIT22)
    if(value & BIT24){
        value = (readl(CONFIG_PMU_BASE+0x38) >> 4) & 0x7F;
        value = (SYS_CLK * value);
    }
    else
  	    value = u32PMU_ReadPLL1CLK() / 3;
  else
  	value = u32PMU_ReadPLL1CLK();

	return value;
}

uint u32PMU_ReadSlaveCPUCLK(void)
{
   	return (u32PMU_ReadPLL1CLK() * 2) / 3;
}

uint u32PMU_ReadJpegCLK(void)
{
    uint clk;
    clk = readl(CONFIG_PMU_BASE+0x28);
    if(clk & BIT16)
        clk = u32PMU_ReadPLL1CLK() / 4;
    else
		clk = u32PMU_ReadHCLK();

    return clk;
}

uint u32PMU_ReadH264encCLK(void)
{
    uint clk;
    clk = readl(CONFIG_PMU_BASE+0x28);
    if(clk & BIT31)
        clk = (u32PMU_ReadPLL4CLK() * 2) / 5;
    else
        clk = u32PMU_ReadAXICLK();

    return clk;
}

uint u32PMU_ReadH264decCLK(void)
{
    uint clk;
    clk = readl(CONFIG_PMU_BASE+0x28);
    if(clk & BIT30)
        clk = (u32PMU_ReadPLL4CLK() * 2) / 5;
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
	printf("C7:%4d  C6:%4d  DR:%4d\n",
		u32PMU_ReadMasterCPUCLK()/1000000, u32PMU_ReadSlaveCPUCLK()/1000000, u32PMU_ReadDDRCLK()/1000000);
	printf("J:%4d   H1:%4d  H2:%4d\n",
		u32PMU_ReadJpegCLK()/1000000, u32PMU_ReadH264encCLK()/1000000, u32PMU_ReadH264decCLK()/1000000);
	//printf("   VGA : %s\n", vgs[vga_res].resolution);
	printf("-------------------------------\n");
}

uint cal_pvalue(uint wclk)
{
	uint ret = 0, pll3clk;

	pll3clk = u32PMU_ReadPLL3CLK();

	ret = pll3clk/wclk;
	if ((pll3clk/ret) > wclk)
		ret++;
	ret -= 1;

	return ret;
}
//#endif

int board_eth_init(bd_t *bd)
{
	int ret; /* 0:external PHY, 1:internal PHY */
	unsigned int value;
	unsigned short phy_data;

    //GPIO32 reset PHY
    writel(readl(CONFIG_PMU_BASE + 0x5C) | BIT25, CONFIG_PMU_BASE + 0x5C);//pinmux
    writel(BIT0, CONFIG_GPIO1_BASE);//data out
    writel(BIT0, CONFIG_GPIO1_BASE + 0x8);//pin direction out
    mdelay(1);
    writel(0, CONFIG_GPIO1_BASE);//data out
    mdelay(10);
    writel(BIT0, CONFIG_GPIO1_BASE);//data out
    mdelay(30);

	value = readl(CONFIG_PMU_BASE + 0x70) & 0xFFFF0000;//X_GMII_GTX_CK 125M
	writel(value | (0x4 << 10), CONFIG_PMU_BASE + 0x70);// X_GMAC_25M

	//GMAC 0
	writel(readl(CONFIG_PMU_BASE + 0x4C) | BIT26, CONFIG_PMU_BASE + 0x4C);//set GMAC0_GTX_CK

	writel(readl(CONFIG_PMU_BASE + 0x28) & (~BIT29), CONFIG_PMU_BASE + 0x28);// pcie clock in
	writel(readl(CONFIG_PMU_BASE + 0xB4) & (~BIT14), CONFIG_PMU_BASE + 0xB4);// clock on

	//GMAC 1
	writel(readl(CONFIG_PMU_BASE + 0x28) & (~BIT28), CONFIG_PMU_BASE + 0x28);// pcie clock in
	writel(readl(CONFIG_PMU_BASE + 0xB4) & (~BIT13), CONFIG_PMU_BASE + 0xB4);// clock on

	phy_data = 0x0123;
	ret= ftgmac100_initialize(bd);
	ftgmac100_read_phy_register("eth0", 0, 2, &phy_data);

#ifdef FTMAC_DEBUG
	printf("PHY reg(0x01)=%x\n",value);
	if(value &0x0020)
		printf("Auto-negotiation process completed\n");
	else
		printf("Auto-negotiation process NOT completed\n");
#endif
	return ret;
}

#ifdef CONFIG_USE_IRQ

// only handle PMU irq
void do_irq (struct pt_regs *pt_regs)
{
	uint val;

	writel(1<<17, CONFIG_PMU_BASE + 0x20);
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
	unsigned int pll1_out, cpu_clk, aclk, hclk, ddrclk;
	unsigned int tmp;

    tmp = readl(CONFIG_PMU_BASE + 0x38) & 0xFFFFF80F;
    tmp |= ((pll4_mul & 0x7F) << 0x4);
    writel(tmp, CONFIG_PMU_BASE + 0x38);

	printf("Will set the following freq...\n");

	pll1_out = SYS_CLK * (pll1_mul & 0x7F);

	tmp = (cpu_mode >> 1) & 0x3;
	if(tmp == 0)
		hclk = pll1_out * 2 / 5;
	else if(tmp == 1)
		hclk = pll1_out / 3;
	else if(tmp == 2)
		hclk = pll1_out / 4;
	else
		hclk = pll1_out / 5;

	tmp = (cpu_mode >> 3) & 0x1;
	if(tmp == 0)
		aclk = pll1_out * 2 / 5;
	else
		aclk = pll1_out / 3;

	tmp = (cpu_mode >> 6) & 0x1;
	if(tmp == 0)
		cpu_clk = pll1_out;
	else
		cpu_clk = pll1_out / 3;

	ddrclk = aclk * 4;

	*(volatile ulong *)(CONFIG_INTC_BASE + 0x08) = (1<<IRQ_PMU);

	PMU_enter_fcs_mode(cpu_mode, pll1_mul, 1);
}


void init_sys_freq(void)
{
#ifndef CONFIG_CMD_FPGA
	unsigned int data;

    // Open  AXI/AHB/APB clock
	writel(0x0FC3807C, CONFIG_PMU_BASE + 0xB0);//AXI gating clock
	writel(0x60108180, CONFIG_PMU_BASE + 0xB4);//AHB gating clock and mcp100 clock
	writel(0x7C00181C, CONFIG_PMU_BASE + 0xB8);//APB gating clock
	writel(0xFFFEE631, CONFIG_PMU_BASE + 0xBC);//APB gating clock

    data = readl(CONFIG_PMU_BASE + 0x0C);
    writel(data | BIT6, CONFIG_PMU_BASE + 0x0C);//626 clock on

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
	/* Re-calculate CLOCK PVALUE of IPs */
	//calculate_codec_clock();
	/* ultra config H.264 enc to SYNC mode */
	//outw(CONFIG_PMU_BASE+0x28, readl(CONFIG_PMU_BASE+0x28)|BIT11); // H264ENC(MCP210) CLK = HCLK

	//FCS_go(0x28AD, 99, 1);
#endif
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
