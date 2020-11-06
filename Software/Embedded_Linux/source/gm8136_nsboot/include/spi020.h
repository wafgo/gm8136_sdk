#ifndef _SPI020_H_
#define _SPI020_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spi_flash.h"

#define FTSPI020_DMA_TX_CH  0
#define FTSPI020_DMA_RX_CH  1

//=============================================================================
// System Header, size = 256 bytes
//=============================================================================

#define NORMAL_MODE				0
#define HW_HANDSHAKE_MODE		1

#define FTSPI020_32BIT(offset)		*((volatile unsigned int *)(REG_SPI020_BASE + offset))
#define FTSPI020_16BIT(offset)		*((volatile UINT16 *)(REG_SPI020_BASE + offset))
#define FTSPI020_8BIT(offset)		*((volatile unsigned char  *)(REG_SPI020_BASE + offset))
#define READ_REG32(_register_)  *((volatile unsigned int *)(_register_))
#define WRITE_REG32(_register_, _value_)  (*((volatile unsigned int *)(_register_)) = (unsigned int)(_value_))

#define SPI_FLASH_ADDR		0x0000
#define SPI_CMD_FEATURE1	0x0004
    #define no_addr_state		    0   // bit[2:0]
    #define addr_1byte          1
    #define addr_2byte          2
    #define addr_3byte			    3
    #define addr_4byte			    4
    #define no_dummy_cycle_2nd		0   // bit[23:16]
    #define dummy_1cycle_2nd		1
    #define dummy_2cycle_2nd		2
    #define dummy_3cycle_2nd		3
    #define no_instr_state			0   // bit[25:24]
    #define instr_1byte			    1
    #define instr_2byte			    2
    	// bit[27:26]
    #define no_enable_dummy_cycle	0

#define SPI_DATA_CNT		0x0008
#define SPI_CMD_FEATURE2	0x000C
    // bit0
    #define cmd_complete_intr_enable	1
    // bit1
    #define spi_read			        0
    #define spi_write			        1
	// bit2
    #define read_status_disable		    0
    #define read_status_enable		    1
	// bit3 
    #define read_status_by_hw		    0
    #define read_status_by_sw		    1
	// bit4
    #define dtr_disable			        0
    #define dtr_enable			        1
	// bit[7:5]
    #define spi_operate_serial_mode		0
    #define spi_operate_dual_mode		1
    #define spi_operate_quad_mode		2
    #define spi_operate_dualio_mode		3
    #define spi_operate_quadio_mode		4
	// bit[9:8]
    #define	start_ce0			        0
	// bit[23:16] op_code info

	// bit[31:24] instr index
	
#define CONTROL_REG			0x0010
    // bit[1:0] divider
    #define divider_2			0
    #define divider_4			1
    #define divider_6			2
    #define divider_8			3
    // bit 4 mode
    #define mode0				0
    #define mode3				1
	// bit 8 abort
	// bit 9 wp_en
	// bit[18:16] busy_loc
    #define BUSY_BIT0			0

#define AC_TIMING			0x0014
// bit[3:0] CS timing
// bit[7:4] Trace delay

#define STATUS_REG			0x0018
// bit0 Tx Empty
// bit1 Rx Full
    #define tx_ready			0
    #define rx_ready			1
// bit2 Cmd Queue Full

#define INTR_CONTROL_REG	0x0020

#define INTR_STATUS_REG		0x0024
#define cmd_complete        (0x1 << 0)

#define SPI_READ_STATUS		0x0028
#define TX_DATA_CNT			0x0030
#define RX_DATA_CNT			0x0034
#define REVISION			0x0050
#define FEATURE				0x0054
#define	support_dtr_mode    (0x1 << 24)
#define SPI020_DATA_PORT	0x0100

// =====================================   Common commands   =====================================
#define CMD_READ_ID  	    0x9F
#define CMD_RESET			0xFF

// Command
#define COMMAND_WRITE_ENABLE		0x06
#define COMMAND_WRITE_DISABLE		0x04
#define COMMAND_ERASE_SECTOR		0x20
#define COMMAND_ERASE_32K_BLOCK	    0x52
#define COMMAND_ERASE_64K_BLOCK	    0xD8
#define COMMAND_ERASE_CHIP		    0xC7
#define COMMAND_READ_STATUS1		0x05
#define COMMAND_READ_STATUS2		0x35
#define COMMAND_WRITE_STATUS		0x01
#define COMMAND_WRITE_PAGE		    0x02
#define COMMAND_WINBOND_QUAD_WRITE_PAGE	    0x32
#define COMMAND_QUAD_WRITE_PAGE	    0x38
#define COMMAND_READ_DATA		    0x03
#define COMMAND_FAST_READ		    0x0B
#define COMMAND_FAST_READ_DUAL	    0x3B
#define COMMAND_FAST_READ_DUAL_IO	0xBB
#define COMMAND_FAST_READ_QUAD	    0x6B
#define COMMAND_FAST_READ_QUAD_IO	0xEB
#define COMMAND_WORD_READ_QUAD_IO	0xE7
#define COMMAND_READ_UNIQUE_ID	    0x4B
#define COMMAND_EN4B                0xB7    //enter 4 byte mode
#define COMMAND_EX4B                0xE9    //exit 4 byte mode

/* Additional command for SPI NAND */
#define COMMAND_GET_FEATURES        0x0F
#define COMMAND_SET_FEATURES        0x1F
#define COMMAND_PAGE_READ           0x13
#define COMMAND_PROGRAM_LOAD        0x02
#define COMMAND_PROGRAM_EXECUTE     0x10
#define COMMAND_BLOCK_ERASE         0xD8

#define SPI_XFER_CMD_STATE		    0x00000002
#define SPI_XFER_DATA_STATE		    0x00000004
#define SPI_XFER_CHECK_CMD_COMPLETE	0x00000008

/* Status register 1 bits */
#define FLASH_STS_BUSY		        (0x1 << 0)
// ===================================== Variable and Struct =====================================

/* default flash parameters */
#define FLASH_SECTOR_SZ     4096
#define FLASH_PAGE_SZ       256

#define FLASH_NAND_BLOCK_PG 64
#define FLASH_NAND_PAGE_SZ  2048
#define FLASH_NAND_SPARE_SZ 64

typedef enum {
	READ_DATA = 0,
	FAST_READ,
	FAST_READ_QUAD_IO
} read_type_t;

typedef enum {
	SECTOR_ERASE = 0,   //4K
	BLOCK_32K_ERASE,
	BLOCK_64K_ERASE
} erase_type_t;

// Cmd Type for choosing
typedef enum {
	PAGE_PROGRAM = 0,
	QUAD_PROGRAM
} write_type_t;

struct spi_flash {
  unsigned int ce;
	unsigned int erase_sector_size;
	unsigned int page_size;
	unsigned int nr_pages;
	unsigned int size;        //chip size
	unsigned int mode_3_4;    //addres length = 3 or 4 bytes
	int (*spi_xfer)(struct spi_flash * flash, unsigned int len, const void *dout, void *din, unsigned long flags, int sts_hw);
	int (*read) (struct spi_flash * flash, read_type_t type, unsigned int offset, unsigned int len, void *buf);
	int (*write) (struct spi_flash * flash, write_type_t type, unsigned int offset, unsigned int len, void *buf);
	int (*erase) (struct spi_flash * flash, erase_type_t type, unsigned int offset, unsigned int len);
	int (*erase_all) (struct spi_flash * flash);
  struct {
  	UINT32 chip_block;
    UINT32 block_pg;
    UINT32 page_size;
    UINT32 spare_size;
    UINT32 block_size;
  } nand_parm;	
};

typedef struct ftspi020_cmd {
	// offset 0x00
	unsigned int spi_addr;
	// offset 0x04
	unsigned char addr_len;
	unsigned char dum_2nd_cyc;
	unsigned char ins_len;  /* instruction length */
	unsigned char conti_read_mode_en;
	// offset 0x08
	unsigned int data_cnt;
	// offset 0x0C
	unsigned char intr_en;
	unsigned char write_en;
	unsigned char read_status_en;
	unsigned char read_status;
	unsigned char dtr_mode;
	unsigned char spi_mode;
	unsigned char start_ce;
	unsigned char conti_read_mode_code;
	unsigned char ins_code;
} ftspi020_cmd_t;
   
/* Init function, 0 for success, otherwise for fail
 */
struct spi_flash *FTSPI020_Init(void);

/* Flash Read function, 
 * start_pg: start page
 * len: how many bytes
 * 0 for success, otherwise for fail
 */
int FTSPI020_PgRd(unsigned int start_pg, unsigned char *rx_buf, unsigned int len);

/* Flash Write function, 
 * start_pg: start page
 * 0 for success, otherwise for fail
 */
int FTSPI020_PgWr(unsigned int start_pg, unsigned char *tx_buf);

/* Flash sector erase, 
 * sector: sector no.
 * sector_cnt: how many sectors
 * 0 for success, otherwise for fail 
 */
int FTSPI020_Erase_Sector(unsigned int sector, unsigned int sector_cnt);

/* Flash chip erase
 * 0 for success, otherwise for fail 
 */
int FTSPI020_Erase_Chip(void);

/* Update the flash config
 */
int FTSPI020_Update_Param(unsigned int divisor, unsigned int pagesz_log2, unsigned int secsz_log2, unsigned int chipsz_log2);

/* load the code and execute it. */
unsigned int fLib_NOR_Copy_Image(unsigned int addr);
unsigned int fLib_SPI_NAND_Copy_Image(unsigned int addr);

/* Get the current config of FLASH.
 */
struct spi_flash *FTSPI020_Get_Param(void);

/* Get FLASH status-1 and -2 register
 */ 
unsigned int FTSPI020_Get_Status(void);
int flash_set_quad_enable(struct spi_flash *flash, int en);
/*
 * API backward compatiable 
 */
#define spi_init        FTSPI020_Init
#define spi_load_para   FTSPI020_Update_Param
#define spi_pgrd        FTSPI020_PgRd
#define spi_pgwr        FTSPI020_PgWr
#define spi_get_satus   FTSPI020_Get_Status
#define spi_erase_sect  FTSPI020_Erase_Sector
#define spi_erase_chip  FTSPI020_Erase_Chip
/*
 * The following functions are used internal only.
 */
void ftspi020_read_status(unsigned char * status);
unsigned char ftspi020_issue_cmd(struct ftspi020_cmd *command);
void ftspi020_data_access(unsigned char *dout, unsigned char *din, unsigned int len);
void ftspi020_wait_cmd_complete(void);

#endif /* _SPI020_H_ */