//*****************************************************************************
//  Copyright  Faraday Technology Corp 2002-2003.  All rights reserved.       *
//----------------------------------------------------------------------------*
// Name: spi_flash.h                                                          *
// Description: SPI flash access functions                                    *
//*****************************************************************************
#ifndef _SPI_FALSH_H_
#define _SPI_FALSH_H_

#include <linux/types.h>
//#include <spi_flash.h>
    
// Command
#define COMMAND_READ_ID							0x9f
#define COMMAND_WRITE_ENABLE        0x06
#define COMMAND_WRITE_DISABLE       0x04
#define COMMAND_ERASE_SECTOR        0x20
#define COMMAND_ERASE_128K_BLOCK    0xD8
#define COMMAND_ERASE_CHIP          0xC7
#define COMMAND_READ_STATUS1        0x05
#define COMMAND_READ_STATUS2        0x35
#define COMMAND_WRITE_STATUS        0x01
#define COMMAND_WRITE_PAGE          0x02
#define COMMAND_WINBOND_QUAD_WRITE_PAGE     0x32
#define COMMAND_QUAD_WRITE_PAGE     0x38
#define COMMAND_READ_DATA           0x03
#define COMMAND_FAST_READ           0x0B
#define COMMAND_FAST_READ_DUAL	    0x3B
#define COMMAND_FAST_READ_DUAL_IO   0xBB
#define COMMAND_FAST_READ_QUAD	    0x6B
#define COMMAND_FAST_READ_QUAD_IO   0xEB
#define COMMAND_WORD_READ_QUAD_IO   0xE7
#define COMMAND_READ_UNIQUE_ID	    0x4B
#define COMMAND_EN4B                0xB7        //enter 4 byte mode
#define COMMAND_EX4B                0xE9        //exit 4 byte mode
#define COMMAND_RESET               0xFF

/* Additional command for SPI NAND */
#define COMMAND_GET_FEATURES        0x0F
#define COMMAND_SET_FEATURES        0x1F
#define COMMAND_PAGE_READ           0x13
#define COMMAND_PROGRAM_LOAD        0x02
#define COMMAND_PROGRAM_EXECUTE     0x10
#define COMMAND_BLOCK_ERASE         0xD8

/* Status register 1 bits */ 
#define FLASH_STS_BUSY              (0x1 << 0)
#define FLASH_STS_WE_LATCH          (0x1 << 1)
#define FLASH_STS_REG_PROTECT0	    (0x1 << 7)
    
/* Status register 2 bits */ 
#define FLASH_STS_REG_PROTECT1	    (0x1 << 0)
#define FLASH_STS_QUAD_ENABLE       (0x1 << 6)
#define FLASH_WINBOND_STS_QUAD_ENABLE   (0x1 << 1)
    
/* PRIVATE use */ 
#define SPI_XFER_CMD_STATE              0x00000002
//#define SPI_XFER_DATA_STATE               0x00000004
#define SPI_XFER_CHECK_CMD_COMPLETE     0x00000008
#define SPI_XFER_DATA_IN_STATE          0x00000010
#define SPI_XFER_DATA_OUT_STATE         0x00000020
    struct winbond_spi_flash_params {
    uint16_t id;
    
        /* Log2 of page size in power-of-two mode */ 
    uint8_t byte_mode;          /* 3: 3 byte mode, 4:4 byte mode(over 128Mb flash) */
    uint8_t l2_page_size;
    uint16_t pages_per_sector;
    uint16_t sectors_per_block;
    uint16_t nr_blocks;
    const char *name;
};
struct winbond_spi_flash {
    //struct spi_flash flash;
    const struct winbond_spi_flash_params *params;
};

int spi_xfer(struct spi020_flash *spi, unsigned int len, const void *dout, void *din,
                unsigned long flags);
int spi_dataflash_read_fast(struct spi020_flash *flash, u32 offset, size_t len, void *buf);
int spi_dataflash_write_fast(struct spi020_flash *flash, u32 offset, size_t len, void *buf);
int spi_dataflash_erase_fast(struct spi020_flash *flash, u32 offset, size_t len);

#endif //_SPI_FALSH_H_
    
//-----------------------------------------------------------------------------
// EOF spi_flash.h
