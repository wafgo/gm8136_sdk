#ifndef _SPI020_H_
#define _SPI020_H_

#define NORMAL_MODE                     0
#define HW_HANDSHAKE_MODE               1
    
#define FTSPI020_32BIT(offset)		*((volatile unsigned int *)(CONFIG_SPI020_BASE + offset))
#define FTSPI020_16BIT(offset)		*((volatile unsigned short *)(CONFIG_SPI020_BASE + offset))
#define FTSPI020_8BIT(offset)		*((volatile unsigned char  *)(CONFIG_SPI020_BASE + offset))
#define READ_REG32(_register_)  *((volatile unsigned int *)(_register_))
#define WRITE_REG32(_register_, _value_)  (*((volatile unsigned int *)(_register_)) = (unsigned int)(_value_))
    
#define SPI_FLASH_ADDR          0x0000
#define SPI_CMD_FEATURE1        0x0004
#define no_addr_state           0   // bit[2:0]
#define addr_3byte              3
#define addr_4byte              4
#define no_dummy_cycle_2nd      0       // bit[23:16]
#define dummy_1cycle_2nd        1
#define dummy_2cycle_2nd        2
#define dummy_3cycle_2nd        3
#define no_instr_state          0       // bit[25:24]
#define instr_1byte             1
#define instr_2byte             2
    // bit[27:26]
#define no_enable_dummy_cycle	0
    
#define SPI_DATA_CNT            0x0008
#define SPI_CMD_FEATURE2        0x000C
    // bit0
#define cmd_complete_intr_enable        1
    // bit1
#define spi_read                        0
#define spi_write                       1
    // bit2
#define read_status_disable             0
#define read_status_enable              1
    // bit3 
#define read_status_by_hw               0
#define read_status_by_sw               1
    // bit4
#define dtr_disable                     0
#define dtr_enable                      1
    // bit[7:5]
#define spi_operate_serial_mode         0
#define spi_operate_dual_mode           1
#define spi_operate_quad_mode           2
#define spi_operate_dualio_mode         3
#define spi_operate_quadio_mode         4
    // bit[9:8]
#define	start_ce0           0
    // bit[23:16] op_code info
    
    // bit[31:24] instr index
    
#define CONTROL_REG         0x0010
    // bit[1:0] divider
#define divider_2           0
#define divider_4           1
#define divider_6           2
#define divider_8           3
    // bit 4 mode
#define mode0               0
#define mode3               1
    // bit 8 abort
    // bit 9 wp_en
    // bit[18:16] busy_loc
#define BUSY_BIT0           0
    
#define AC_TIMING           0x0014
// bit[3:0] CS timing
// bit[7:4] Trace delay
    
#define STATUS_REG          0x0018
#define tx_ready            0
#define rx_ready            1
// bit2 Cmd Queue Full
    
#define INTR_CONTROL_REG    0x0020
    
#define INTR_STATUS_REG     0x0024
#define cmd_complete        (0x1 << 0)
    
#define SPI_READ_STATUS     0x0028
#define TX_DATA_CNT         0x0030
#define RX_DATA_CNT         0x0034
#define REVISION            0x0050
#define FEATURE             0x0054
#define	support_dtr_mode    (0x1 << 24)
#define	rxfifo_depth        0x20 /* hardware define */        
#define	txfifo_depth        0x20 /* hardware define */       
#define SPI020_DATA_PORT    0x0100
    
// ===================================== Variable and Struct =====================================
    
/* default flash parameters */ 
#define FLASH_SECTOR_SZ     4096
#define FLASH_PAGE_SZ       256
#define FLASH_SZ            (16 << 20)  //16M bytes


//=============================================================================
// System Header, size = 256 bytes
//=============================================================================
#ifndef __ASSEMBLY__
typedef struct
{
    char signature[8];          /* Signature is "GM8xxx" */
    unsigned int bootm_addr;    /* Image offset to load by spiboot */
    unsigned int bootm_size;
    unsigned int bootm_reserved;
    
    struct {
        unsigned int addr;          /* image address */
        unsigned int size;          /* image size */
        unsigned char name[8];      /* image name */
        unsigned int reserved[1];
    } image[10];
    
    struct 
    {
        unsigned int pagesz_log2;   // page size with bytes in log2
        unsigned int secsz_log2;    // sector size with bytes in log2
        unsigned int chipsz_log2;   // chip size with bytes in log2
        unsigned int clk_div;       // 2/4/6/8
        unsigned int reserved[4];   // unused
    } norfixup;	
    
    unsigned char   last_256[4];    // byte254:0x55, byte255:0xAA
} NOR_SYSHDR;

/* Image header which is same as nand's */ 
typedef struct nor_img_header  {
    unsigned int magic;        /* Image header magic number (0x805A474D) */
    unsigned int chksum;      /* Image CRC checksum */
    unsigned int size;        /* Image size */
    unsigned int unused;
    unsigned char name[80];   /* Image name */
    unsigned char reserved[160];      /* Reserved for future */
} NOR_IMGHDR;

struct spi020_flash {
    unsigned int ce;
    unsigned int erase_sector_size;
    unsigned short page_size;
    unsigned int nr_pages;
    unsigned int size;                  //chip size
    unsigned int mode_3_4;               //addres length = 3 or 4 bytes
    unsigned short chip_type;
};

typedef struct ftspi020_cmd {
    
        // offset 0x00
    unsigned int spi_addr;
    
        // offset 0x04
    unsigned char addr_len;
    unsigned char dum_2nd_cyc;
    unsigned char ins_len;                /* instruction length */
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

/* Update the flash config
 */ 
int FTSPI020_Update_Param(int divisor, int pagesz_log2, int secsz_log2, int chipsz_log2, int mode_3_4);

/* load the code and execute it. */ 
void fLib_NOR_Copy_RomCode(unsigned int addr, unsigned int size);

/*
 * The following functions are used internal only.
 */ 
void ftspi020_read_status(unsigned char * status);
unsigned char ftspi020_issue_cmd(struct ftspi020_cmd *command);
void ftspi020_data_access(unsigned char direction, unsigned char * addr, unsigned int len);
void ftspi020_wait_cmd_complete(void);
//int FTSPI020_Init(void);
void ftspi020_setup_cmdq(ftspi020_cmd_t * spi_cmd, int ins_code, int ins_len, int write_en,
                          int spi_mode, int data_cnt);

#endif	/* __ASSEMBLY__ */
#endif /* _SPI020_H_ */
