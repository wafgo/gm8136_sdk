/*
 * SPI flash internal definitions
 *
 * Copyright (C) 2008 Atmel Corporation
 */

/* Common parameters -- kind of high, but they should only occur when there
 * is a problem (and well your system already is broken), so err on the side
 * of caution in case we're dealing with slower SPI buses and/or processors.
 */
#define SPI_FLASH_PROG_TIMEOUT		(2 * CONFIG_SYS_HZ)
#define SPI_FLASH_PAGE_ERASE_TIMEOUT	(5 * CONFIG_SYS_HZ)
#define SPI_FLASH_SECTOR_ERASE_TIMEOUT	(10 * CONFIG_SYS_HZ)

/* Common commands */
#define CMD_READ_ARRAY_SLOW		0x03
#define CMD_READ_ARRAY_FAST		0x0b

#define CMD_WRITE_STATUS		0x01
#define CMD_PAGE_PROGRAM		0x02
#define CMD_WRITE_DISABLE		0x04
#define CMD_READ_STATUS			0x05
#define CMD_WRITE_ENABLE		0x06
#define CMD_ERASE_4K			0x20
#define CMD_ERASE_32K			0x52
#define CMD_ERASE_64K			0xd8
#define CMD_ERASE_CHIP			0xc7

/* Common status */
#define STATUS_WIP			0x01

/* Send a single-byte command to the device and read the response */
int spi_flash_cmd(struct spi020_flash *spi, u8 *cmd, void *response, size_t len);

/*
 * Send a multi-byte command to the device and read the response. Used
 * for flash array reads, etc.
 */
int spi_flash_cmd_read(struct spi020_flash *spi, const u8 *cmd,
		size_t cmd_len, void *data, size_t data_len);

int spi_flash_cmd_read_fast(struct spi020_flash *flash, u32 offset,
		size_t len, void *data);

/*
 * Send a multi-byte command to the device followed by (optional)
 * data. Used for programming the flash array, etc.
 */
int spi_flash_cmd_write(struct spi020_flash *spi, const u8 *cmd, size_t cmd_len,
		const void *data, size_t data_len);

/*
 * Write the requested data out breaking it up into multiple write
 * commands as needed per the write size.
 */
int spi_flash_cmd_write_multi(struct spi020_flash *flash, u32 offset,
		size_t len, const void *buf);

/*
 * Enable writing on the SPI flash.
 */
static inline int spi_flash_cmd_write_enable(struct spi020_flash *flash)
{
	return spi_flash_cmd(flash, (u8 *)CMD_WRITE_ENABLE, NULL, 0);
}

/*
 * Disable writing on the SPI flash.
 */
static inline int spi_flash_cmd_write_disable(struct spi020_flash *flash)
{
	return spi_flash_cmd(flash, (u8 *)CMD_WRITE_DISABLE, NULL, 0);
}

/* Program the status register. */
int spi_flash_cmd_write_status(struct spi020_flash *flash, u8 sr);

/*
 * Same as spi_flash_cmd_read() except it also claims/releases the SPI
 * bus. Used as common part of the ->read() operation.
 */
int spi_flash_read_common(struct spi020_flash *flash, const u8 *cmd,
		size_t cmd_len, void *data, size_t data_len);

/* Send a command to the device and wait for some bit to clear itself. */
int spi_flash_cmd_poll_bit(struct spi020_flash *flash, unsigned long timeout,
			   u8 cmd, u8 poll_bit);

/*
 * Send the read status command to the device and wait for the wip
 * (write-in-progress) bit to clear itself.
 */
int spi_flash_cmd_wait_ready(struct spi020_flash *flash, unsigned long timeout);

/* Erase sectors. */
int spi_flash_cmd_erase(struct spi020_flash *flash, u32 offset);
