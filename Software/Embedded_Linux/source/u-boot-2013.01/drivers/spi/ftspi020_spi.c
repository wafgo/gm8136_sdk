#include <common.h>
#include <spi.h>
#include <malloc.h>
#include <asm/io.h>

#include "ftspi020_spi.h"

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
			unsigned int max_hz, unsigned int mode)
{
	struct spi_slave *fs = NULL;

	if(bus != 0){
		printf("only support bus 0\n");
		return NULL;
	}	
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

	/* 1. Setup data length */
	data = spi_readl(SSP_CONTROL1);
	data &= ~ SSP_SDL;
	data |= (wsize-1) << 16;
	spi_writel(SSP_CONTROL1, data);

	_ftssp010_spi_work_transfer(dout, din, len, wsize/8, flags);

    return 0;
}
