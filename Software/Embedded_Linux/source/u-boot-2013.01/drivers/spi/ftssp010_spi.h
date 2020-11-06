#ifndef _FTSSP010_SPI_H_
#define _FTSSP010_SPI_H_

/* SSP: Register Address Offset */
#define SSP_CONTROL0        0x00
#define SSP_CONTROL1        0x04
#define SSP_CONTROL2        0x08
#define SSP_STATUS      0x0C
#define SSP_INT_CONTROL     0X10
#define SSP_INT_STATUS      0x14
#define SSP_DATA        0x18
#define SSP_INFO        0x1C
#define SSP_ACLINK_SLOT_VALID   0x20
#define SSP_FEATURE   0x64

/* SSP: Control register 0  */

#define SSP_Format                  0x0000  //TI SSP
#define SPI_Format                    0x1000  //Motororal SPI
#define Microwire_Format            0x2000  //NS Microwire
#define I2S_Format                  0x3000  //Philips's I2S
#define AC97_Format                 0x4000  //Intel AC-Link

#define SSP_FSDIST                  0x300
#define SSP_LBM                     0x80  /* loopback mode */
#define SSP_LSB                     0x40  /* LSB first */
#define SSP_FSPO_LOW                0x20  /* Frame sync atcive low */
#define SSP_FSPO_HIGH               0x0   /* Frame sync atcive high */


#define SSP_DATAJUSTIFY             0x10  /* data padding in front of serial data */

#define SSP_OPM_MSST                0xC  /* Master stereo mode */
#define SSP_OPM_MSMO                0x8  /* Master mono mode */
#define SSP_OPM_SLST                0x4  /* Slave stereo mode */
#define SSP_OPM_SLMO                0x0  /* Slave mono mode */


#define SSP_SCLKPO_HIGH             0x2  /* SCLK Remain HIGH */
#define SSP_SCLKPO_LOW              0x0  /* SCLK Remain LOW */
#define SSP_SCLKPH_HALFCLK          0x1  /* Half CLK cycle */
#define SSP_SCLKPH_ONECLK           0x0  /* One CLK cycle */


/* SSP: Control Register 1 */

#define SSP_PDL                     0xFF000000  /* paddinf data length */
#define SSP_SDL                     0x1F0000    /* Serial data length(actual data length-1) */
#define SSP_CLKDIV                  0xFFFF      /*  clk divider */


/* SSP: Control Register 2 */

#define SSP_ACCRST                  0x20     /* AC-Link Cold Reset Enable */
#define SSP_ACWRST                  0x10     /* AC-Link Warm Reset Enable */
#define SSP_TXFCLR                  0x8      /* TX FIFO Clear */
#define SSP_RXFCLR                  0x4      /* RX FIFO Clear */
#define SSP_TXDOE                   0x2      /* TX Data Output Enable */
#define SSP_SSPEN                   0x1      /* SSP Enable */


/*
 * Status Register
 */
#define FTSSP010_STATUS_RFF     (1 << 0)        /* receive FIFO full */
#define FTSSP010_STATUS_TFNF        (1 << 1)        /* transmit FIFO not full */
#define FTSSP010_STATUS_BUSY        (1 << 2)        /* bus is busy */
#define FTSSP010_STATUS_GET_RFVE(reg)   (((reg) >> 4) & 0x1f)   /* receive  FIFO valid entries */
#define FTSSP010_STATUS_GET_TFVE(reg)   (((reg) >> 12) & 0x1f)  /* transmit FIFO valid entries */

/*
 * Feature Register
 */
#define FTSSP010_FEATURE_WIDTH(reg)     (((reg) >>  0) & 0xff)
#define FTSSP010_FEATURE_RXFIFO_DEPTH(reg)  (((reg) >>  8) & 0xff)
#define FTSSP010_FEATURE_TXFIFO_DEPTH(reg)  (((reg) >> 16) & 0xff)
#define FTSSP010_FEATURE_AC97           (1 << 24)
#define FTSSP010_FEATURE_I2S            (1 << 25)
#define FTSSP010_FEATURE_SPI_MWR        (1 << 26)
#define FTSSP010_FEATURE_SSP            (1 << 27)



/* SSP: Interrupr Control register */
#define SSP_TXDMAEN                 0x20     /* TX DMA Enable */
#define SSP_RXDMAEN                 0x10     /* RX DMA Enable */
#define SSP_TFIEN                   0x8      /* TX FIFO Int Enable */
#define SSP_RFIEN                   0x4      /* RX FIFO Int Enable */
#define SSP_TFURIEN                 0x2      /* TX FIFO Underrun int enable */
#define SSP_RFORIEN                 0x1      /* RX FIFO Overrun int enable */

// **************** SPI Flash Command **********
#define SPI_FLASH_CMD_WRSR          0x01
#define SPI_FLASH_CMD_PAGE_PRG      0x02
#define SPI_FLASH_CMD_RD_DATA       0x03
#define SPI_FLASH_CMD_WRITE_DISABLE 0x04
#define SPI_FLASH_CMD_RD_ST     	0x050000
#define SPI_FLASH_CMD_WRITE_ENABLE  0x06
#define SPI_FLASH_CMD_FAST_RD       0x0B
#define SPI_FLASH_CMD_SECTOR_ERASE  0x20
#define SPI_FLASH_CMD_BLK_ERASE     0x52
#define SPI_FLASH_CMD_PWR_DWN       0xB9
#define SPI_FLASH_CMD_RD_ID     	0x9F
#define SPI_FLASH_CMD_JEDEC_ID      0x9F9F9F9F

/* Register access macros */
#define spi_readl(reg)					\
	readl((volatile unsigned int) (CONFIG_SSP0_BASE + reg))
#define spi_writel(reg, value)				\
	writel(value,( volatile unsigned int) (CONFIG_SSP0_BASE + reg))

#define UINT32 unsigned int
#define UINT16 unsigned short 
#define UINT8  unsigned char

/*
 * NOTE: every platform_gm8xxx.c MUST implements this function
 * to make sure that platform specific PMU and PIN-MUX settings are done
 */
void ft_spi_cs_clk_init(void);

#endif//end of _FTSSP010_SPI_H_
