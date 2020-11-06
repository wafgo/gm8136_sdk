#include <common.h>
#include <spi.h>
#include <malloc.h>

#include <asm/io.h>

#include "ftssp010_spi.h"

#define GPIO_DIRC       0x08
#define GPIO_DATASET	0x10
#define GPIO_DATACLR	0x14

static int swap_enable = 1;

enum SPI_CS_NUM {
	SPI_CS_FIRST = 14,
	SPI_CS_SEC   = 15,
	SPI_CS_THIRD = 19,
	SPI_CS_FOUR  = 20,
	SPI_CS_FIVE  = 21,
	SPI_CS_SIX   = 22
};  

void spi_swap_enable(void)
{
    swap_enable = 1;
}

void spi_swap_disable(void)
{
    swap_enable = 0;
}

void spi_init()
{
}

static inline void gpio_out_init(UINT32 pin)
{
	UINT32 reg = 0;

	reg = readl(CONFIG_GPIO0_BASE + GPIO_DIRC);   
	reg |= (1 << pin);
	writel(reg, CONFIG_GPIO0_BASE + GPIO_DIRC);
}

static inline void gpio_out_1(UINT32 pin)
{
	UINT32 reg = 0;

	reg = readl(CONFIG_GPIO0_BASE + GPIO_DATASET);   
	reg |= (0x1 << pin);
	writel(reg, CONFIG_GPIO0_BASE + GPIO_DATASET);
}

static inline void gpio_out_0(UINT32 pin)
{
	UINT32 reg = 0;

	reg = readl(CONFIG_GPIO0_BASE + GPIO_DATACLR);
	reg |= (0x1 << pin);
	writel(reg, CONFIG_GPIO0_BASE + GPIO_DATACLR);
}

static void spi_cs_gpio_out(int state, UINT32 cs)
{
	UINT8 gpio_pin = SPI_CS_FIRST;

	if (cs == 0) {
		gpio_pin = SPI_CS_FIRST;
	} else if (cs == 1) {
		gpio_pin = SPI_CS_SEC;
	} else if (cs == 2) {
		gpio_pin = SPI_CS_THIRD;
	} else if (cs == 3) {
		gpio_pin = SPI_CS_FOUR;
	} else if (cs == 4) {
		gpio_pin = SPI_CS_FIVE;
	} else if (cs == 5) {
		gpio_pin = SPI_CS_SIX;
	}

	if(state){
		gpio_out_1(gpio_pin);
	}else{
		gpio_out_0(gpio_pin);
	}
}

static void spi_cs_gpio_init(void)
{
	/* set FS0 ~ FS5 as output*/
	gpio_out_init(SPI_CS_FIRST);
	gpio_out_init(SPI_CS_SEC);
	gpio_out_init(SPI_CS_THIRD);
	gpio_out_init(SPI_CS_FOUR);
	gpio_out_init(SPI_CS_FIVE);
	gpio_out_init(SPI_CS_SIX);

	/* set init high */
	gpio_out_1(SPI_CS_FIRST);
	gpio_out_1(SPI_CS_SEC);
	gpio_out_1(SPI_CS_THIRD);
	gpio_out_1(SPI_CS_FOUR);
	gpio_out_1(SPI_CS_FIVE);
	gpio_out_1(SPI_CS_SIX);
}

#ifdef CONFIG_FTSSP010_SPI
void ft_spi_cs_clk_init(void)
{
	UINT32 reg = 0;

	/* 0x78, SSP0CLK_PVALUE. Default value is 0x10011. 540/(2+1) = 180M.
	 * SSP has divisor as well. bclk = SSPCLK/(2(div+1))
	 */
	reg = readl(CONFIG_PMU_BASE + 0x78);
	reg &= ~0x3F;
	reg |= 0x3;
	writel(reg, CONFIG_PMU_BASE + 0x78);

	/* turn on SSP clock and GPIO clock */
	reg = readl(CONFIG_PMU_BASE + 0x3C);
	reg &= ~(0x1 << 5 | 0x1 << 9);
	writel(reg, CONFIG_PMU_BASE + 0x3C);

	/* PinMux with GPIO, set SSP0_RXD, SSP0_SCLK, SSP0_TXD*/
	reg = readl(CONFIG_PMU_BASE + 0x50);
	reg &= ~(0x3 << 30 | 0x3 << 28 | 0x3 << 26);
	reg |= (0x1 << 30 | 0x1 << 28 | 0x1 << 26);
	writel(reg, CONFIG_PMU_BASE + 0x50);

	/* set SSP0_FS0 as GPIO(pin 14) to be the SPI chip select*/
	reg = readl(CONFIG_PMU_BASE + 0x54);
	reg &= ~(0x3 << 4 | 0x7 << 0);
	reg |= (0x0 << 4); /* choose GPIO14 as FS0 */
	reg &= ~(0x3 << 6);/* choose GPIO15 as FS1 */
	writel(reg, CONFIG_PMU_BASE + 0x54);

	/* set FS2 ~ FS5 */
	reg = readl(CONFIG_PMU_BASE + 0x60);
	reg &= ~(0xFFF);//choose GPIO function for FS2 ~ FS5
	writel(reg, CONFIG_PMU_BASE + 0x60);
}
#endif

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
			unsigned int max_hz, unsigned int mode)
{
	unsigned int data = 0;
	struct spi_slave *fs = NULL;
	
	ft_spi_cs_clk_init();

	spi_cs_gpio_init();
	data = spi_readl(SSP_CONTROL0);
	data &= ~0x702C;
	data |= SSP_OPM_MSST | SPI_Format | SSP_FSPO_LOW;
	spi_writel(SSP_CONTROL0, data);

	data = spi_readl(SSP_CONTROL1);
	data &= ~SSP_CLKDIV;
	data |= 0x003;  //Set 5 is fastest in FPGA test
	spi_writel(SSP_CONTROL1, data);

	data = spi_readl(SSP_CONTROL2);
	data |= (SSP_SSPEN | SSP_TXDOE | SSP_TXFCLR | SSP_RXFCLR);
	spi_writel(SSP_CONTROL2, data);
	
	fs = malloc(sizeof(struct spi_slave));
	if (!fs){
		return NULL;
	}

	fs->bus = bus;
	fs->cs = cs;

	return fs;
}

void spi_free_slave(struct spi_slave *slave)
{
	free(slave);
}

int spi_claim_bus(struct spi_slave *slave)
{
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
}

static int ftssp010_txfifo_not_full(void)
{
    return spi_readl(SSP_STATUS) & FTSSP010_STATUS_TFNF;
}

static int ftssp010_rxfifo_valid_entries(void)
{
    return FTSSP010_STATUS_GET_RFVE(spi_readl(SSP_STATUS));
}

static int ftssp010_rxfifo_depth(void)
{
    return  FTSSP010_FEATURE_RXFIFO_DEPTH(spi_readl(SSP_FEATURE)) + 1;
}

static void ftssp010_write_word(const void *data, int wsize)
{
    unsigned int    tmp = 0;

    if (data) {
        switch (wsize) {
        case 1:
            tmp = *(const u8 *)data;
            break;

        case 2:
            tmp = *(const u16 *)data;
            break;

        default:
            tmp = *(const u32 *)data;
            break;
        }
    }

    spi_writel(SSP_DATA, tmp);
}

static void ftssp010_read_word(void *buf, int wsize)
{
    unsigned int    data = spi_readl(SSP_DATA);

    //if (buf) {//buf addr = 0 will be stop
        switch (wsize) {
        case 1:
            *(u8 *) buf = data&0xff;
            break;

        case 2:
            *(u16 *) buf = data;
            break;
        default:
            *(u32 *) buf = data;
            break;
        }
    //}
}



//port from linux
int _ftssp010_spi_work_transfer(const void *tx_buf,void *rx_buf,
		int len, u32 wsize, unsigned int flags)
{
	while (len > 0) {
		int count = 0;
		int i;
		/* tx FIFO not full */
		while (ftssp010_txfifo_not_full()) {
			if (len <= 0)
				break;

			ftssp010_write_word(tx_buf, wsize);

			if (tx_buf)
				tx_buf += wsize;

			count++;
			len -= wsize;

			/* avoid Rx FIFO overrun */
			if (count >= ftssp010_rxfifo_depth())
				break;
		}

		/* receive the same number as transfered */

		for (i = 0; i < count; i++) {
			while (1){
				if (ftssp010_rxfifo_valid_entries())
					break;
			}

			ftssp010_read_word(rx_buf, wsize);

			//if (rx_buf)//buf addr = 0 will be stop
				rx_buf += wsize;
		}
		
		if (len%0x20000 == 0 || ((len < 0x200) && (len%10 == 0))) {
			if (flags & SPI_XFER_END) {
				puts("#");
			}

		}
	}
	
	return len;
}

u32 wsize = 0;

void set_wsize(u8 size)
{
    wsize = size;
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	u32		len = bitlen/8;
	u32		data = 0;
	u8		WordAlign = 0;

	/* 1. Setup data length */
	data = spi_readl(SSP_CONTROL1);
	data &= ~ SSP_SDL;
	data |= (wsize-1) << 16;
	spi_writel(SSP_CONTROL1, data);

	if (flags & SPI_XFER_BEGIN){
		spi_cs_gpio_out(0, slave->cs);
	}
    
	_ftssp010_spi_work_transfer(dout, din, len, wsize/8, flags);

	if (flags & SPI_XFER_END){
		spi_cs_gpio_out(1, slave->cs);
	}
	
    return 0;
}
