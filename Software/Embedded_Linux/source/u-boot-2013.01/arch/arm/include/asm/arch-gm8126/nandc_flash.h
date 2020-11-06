#ifndef __NANDC_FLASH_H
#define __NANDC_FLASH_H

#define NAND_TO_BUF  0
#define BUF_TO_NAND  1

#define MODULE Module_0
#define APB_DMA 0
#define TRUE                    1
#define FALSE                   0

//#define CFG_NANDC_DEBUG1
//#define CFG_NANDC_DEBUG2
//#define CFG_NANDC_DEBUG3


/////////////////////////////////
#define SPARE_PAGE_MODE_SUPPORT		0   //don't change this

/* For Spare SRAM Organization */
#define SPARE_SIZE_OF_PAGE  64  /* not configurable */
#define MAX_SPARE_SIZE_OF_PAGE  SPARE_SIZE_OF_PAGE /* upper limit of nandc */
#define USR_SPARE_SIZE      3
#define NANDC_MAX_SECTOR    16  /* 8K Page size */

#define LSN_MAGIC	0x112233

#define FTNANDC023_DATA_PORT_BASE   NAND023_DATA_PORT
#define EC_SPARE_SIZE       		SPARE_SIZE_OF_PAGE

#define FTNANDC023_USE_DMA
#define FTNANDC023_Data_Port_Fixed

/* Command Queue First Word */
#define AUTO_CE_SEL		0	/* bit 25 */
#define AUTO_CE_FACTOR	0	/* bit 26 */
#define Sector_mode						0
#define Page_mode						1

/* Offset for the region */
#define FTNANDC023_DATA_PORT(offset)	readl(FTNANDC023_DATA_PORT_BASE + offset)

#define ECC_Data_Status					0x0000

#define ECC_Control						0x0008
#define ecc_existence_between_sector	0	//Keep spacing for ECC
#define ecc_nonexistence_between_sector	1	//No spacing for ECC
#define ecc_base_512byte				0
#define ecc_base_1kbyte					1
#define ecc_disable						0
#define	ecc_enable						1
#define ecc_unmask						0
#define ecc_mask						1

#define ECC_threshold_ecc_error_num		0x0010
#define	threshold_ecc_error_1bit		0
#define	threshold_ecc_error_2bit		1
#define	threshold_ecc_error_3bit		2
#define	threshold_ecc_error_4bit		3
#define	threshold_ecc_error_5bit		4
#define	threshold_ecc_error_6bit		5
#define	threshold_ecc_error_7bit		6
#define	threshold_ecc_error_8bit		7
#define	threshold_ecc_error_9bit		8
#define	threshold_ecc_error_10bit		9
#define	threshold_ecc_error_11bit		10
#define	threshold_ecc_error_12bit		11
#define	threshold_ecc_error_13bit		12
#define	threshold_ecc_error_14bit		13
#define	threshold_ecc_error_15bit		14
#define	threshold_ecc_error_16bit		15
#define	threshold_ecc_error_17bit		16
#define	threshold_ecc_error_18bit		17
#define	threshold_ecc_error_19bit		18
#define	threshold_ecc_error_20bit		19
#define	threshold_ecc_error_21bit		20
#define	threshold_ecc_error_22bit		21
#define	threshold_ecc_error_23bit		22
#define	threshold_ecc_error_24bit		23
#define threshold_ecc_spare_error_1bit	0
#define threshold_ecc_spare_error_2bit	1
#define threshold_ecc_spare_error_3bit	2
#define threshold_ecc_spare_error_4bit	3
#define threshold_ecc_spare_error_5bit	4
#define threshold_ecc_spare_error_6bit	5
#define threshold_ecc_spare_error_7bit	6
#define threshold_ecc_spare_error_8bit	7

#define ECC_correct_ecc_error_num		0x0018
#define	correct_ecc_error_1bit			0
#define	correct_ecc_error_2bit			1
#define	correct_ecc_error_3bit			2
#define	correct_ecc_error_4bit			3
#define	correct_ecc_error_5bit			4
#define	correct_ecc_error_6bit			5
#define	correct_ecc_error_7bit			6
#define	correct_ecc_error_8bit			7
#define	correct_ecc_error_9bit			8
#define	correct_ecc_error_10bit			9
#define	correct_ecc_error_11bit			10
#define	correct_ecc_error_12bit			11
#define	correct_ecc_error_13bit			12
#define	correct_ecc_error_14bit			13
#define	correct_ecc_error_15bit			14
#define	correct_ecc_error_16bit			15
#define	correct_ecc_error_17bit			16
#define	correct_ecc_error_18bit			17
#define	correct_ecc_error_19bit			18
#define	correct_ecc_error_20bit			19
#define	correct_ecc_error_21bit			20
#define	correct_ecc_error_22bit			21
#define	correct_ecc_error_23bit			22
#define	correct_ecc_error_24bit			23
#define correct_ecc_spare_error_1bit	0
#define correct_ecc_spare_error_2bit	1
#define correct_ecc_spare_error_3bit	2
#define correct_ecc_spare_error_4bit	3
#define correct_ecc_spare_error_5bit	4
#define correct_ecc_spare_error_6bit	5
#define correct_ecc_spare_error_7bit	6
#define correct_ecc_spare_error_8bit	7

#define ECC_Interrupt_Enable				0x0020
#define hit_spare_threshold_intr_disable	0
#define hit_spare_threshold_intr_enable		1
#define correct_spare_fail_intr_disable		0
#define correct_spare_fail_intr_enable		1
#define hit_threshold_intr_disable			0
#define hit_threshold_intr_enable			1
#define correct_fail_intr_disable			0
#define correct_fail_intr_enable			1

#define ECC_Interrupt_Status_For_Data	0x0024
#define ECC_Interrupt_Status_For_Spare  0x0026

#define ECC_Spare_Status				0x002C

#define Device_Busy_Ready_Status		0x0100
#define NANDC_General_Setting			0x0104
#define each_channel_have_1CE			0
#define	each_channel_have_2CE			1
#define	each_channel_have_4CE			2
#define	each_channel_have_8CE			3
#define check_crc_in_spare				0
#define noncheck_crc_in_spare			1
#define busy_ready_bit_location_bit0	0
#define busy_ready_bit_location_bit1	1
#define busy_ready_bit_location_bit2	2
#define busy_ready_bit_location_bit3	3
#define busy_ready_bit_location_bit4	4
#define busy_ready_bit_location_bit5	5
#define busy_ready_bit_location_bit6	6
#define busy_ready_bit_location_bit7	7
#define cmd_pass_fail_bit_location_bit0	0
#define cmd_pass_fail_bit_location_bit1	1
#define cmd_pass_fail_bit_location_bit2	2
#define cmd_pass_fail_bit_location_bit3	3
#define cmd_pass_fail_bit_location_bit4	4
#define cmd_pass_fail_bit_location_bit5	5
#define cmd_pass_fail_bit_location_bit6	6
#define cmd_pass_fail_bit_location_bit7	7
#define row_addr_update_disable			0
#define row_addr_update_enable			1
#define flash_write_protect_disable		0
#define flash_write_protect_enable		1
#define data_inverse_mode_disable		0
#define data_inverse_mode_enable		1
#define data_scrambler_disable			0
#define data_scrambler_enable			1

#define Memory_Attribute_Setting		0x0108
#define device_size_1page_per_ce		0
#define device_size_2page_per_ce		1
#define device_size_4page_per_ce		2
#define device_size_8page_per_ce		3
#define device_size_16page_per_ce		4
#define device_size_32page_per_ce		5
#define device_size_64page_per_ce		6
#define device_size_128page_per_ce		7
#define device_size_256page_per_ce		8
#define device_size_512page_per_ce		9
#define device_size_1024page_per_ce		10
#define device_size_2048page_per_ce		11
#define device_size_4096page_per_ce		12
#define device_size_8192page_per_ce		13
#define device_size_16384page_per_ce	14
#define device_size_32768page_per_ce	15
#define device_size_65536page_per_ce	16
#define device_size_131072page_per_ce	17
#define device_size_262144page_per_ce	18
#define device_size_524288page_per_ce	19
#define device_size_1048576page_per_ce	20

#define spare_area_8bytes_per_512byte	8
#define spare_area_16bytes_per_512byte	16
/*	#define spare_area_128byte				128//0x80
	#define spare_area_218byte				218//0xDA
	#define spare_area_224byte				224//0xE0*/
#define block_size_16kbyte				16
#define block_size_64kbyte				64	//0
#define block_size_128kbyte				128	//1, 0 for Toshiba
#define block_size_256kbyte				256	//2, 1 for Toshiba
#define block_size_512kbyte				512	//3, 2 for Toshiba
#define block_size_1Mbyte               1024// , 3 for Toshiba

#define page_size_512byte				1	//0
#define page_size_2kbyte				2	//1, 0 for Toshiba
#define page_size_4kbyte				4	//2, 1 for Toshiba
#define page_size_8kbyte				8	//3, 2 for Toshiba


#define Row_address_1cycle				0
#define Row_address_2cycle				1
#define Row_address_3cycle				2
#define Row_address_4cycle				3
#define Column_address_1cycle			0
#define Column_address_2cycle			1

#define Spare_Region_Access_Mode		0x010C
#define spare_access_by_sector			Sector_mode
#define spare_access_by_page			Page_mode

#define Spare_Region_Protect_Mode 		0x010D
#define spare_protect_disable			0
#define spare_protect_enable			1

#define Flash_AC_Timing0(ch)			(0x0110 + (ch << 3))
#define Flash_AC_Timing1(ch)			(0x0114 + (ch << 3))
#define Flash_AC_Timing2(ch)			(0x0190 + (ch << 3))
#define Flash_AC_Timing3(ch)			(0x0194 + (ch << 3))

#define NANDC_Interrupt_Enable			0x0150
#define crc_fail_intr_disable			0
#define	crc_fail_intr_enable			1
#define flash_status_fail_intr_disable	0
#define flash_status_fail_intr_enable	1
#define NANDC_Interrupt_Status			0x0154
#define Current_Access_Row_Addr(ch)		(0x0158 + (ch << 2 ))
#define	Read_Status						0x0178
#define Addr_Toggle_Bit_Location		0x0180
#define NANDC_Software_Reset			0x0184
#define NANDC_Auto_Compare_Pattern		0x0188
#define Command_Queue_Status			0x0200
#define Command_Queue_Flush			    0x0204
#define Command_Complete_Count			0x0208
#define Command_Complete_Count_Reset	0x020C

/* Command Queue for all channel */
#define General_Command_Queue1			0x0280
#define General_Command_Queue2			0x0284
#define General_Command_Queue3			0x0288
#define General_Command_Queue4			0x028C
#define General_Command_Queue5			0x0290
#define General_Command_Queue6			0x0294

/* Command Queue for specified channel */
#define Command_Queue1(ch)				(0x0300 + (ch << 5 ))
#define CE_INC_1						1
#define CE_INC_2						2
#define CE_INC_3						3
#define Command_Queue2(ch)				(0x0304 + (ch << 5 ))
#define CE_DEC_1						1
#define CE_DEC_2						2
#define CE_DEC_3						3
#define Command_Queue3(ch)				(0x0308 + (ch << 5 ))
#define Command_Queue4(ch)				(0x030C + (ch << 5 ))
#define Command_Queue5(ch)				(0x0310 + (ch << 5 ))
#define Command_Queue6(ch)				(0x0314 + (ch << 5 ))
	/* Mulitply factor of automatic CE selection*/
#define Auto_ce_select_1_multiply		0
#define Auto_ce_select_2_multiply		1
	/* Auto ce selection*/
#define Auto_ce_select_disable			0
#define Auto_ce_select_enable			1
	/* Support Flash Type */
#define Legacy_flash					0
#define Toggle_flash					2
#define ONFI_flash						3	
    /* Command Starting CE */
#define Start_From_CE0					0
#define Start_From_CE1					1
#define Start_From_CE2					2
#define Start_From_CE3					3
#define Start_From_CE4					4
#define Start_From_CE5					5
#define Start_From_CE6					6
#define Start_From_CE7					7
    /* Byte Mode */
#define Byte_Mode_Enable				1
#define Byte_Mode_Disable				0
    /* Ignore BMC region status of full/empty */
#define Ignore_BMC_Region_Status		1
#define No_Ignore_BMC_Region_Status		0
    /* BMC region selection */
#define BMC_Region0						0
#define BMC_Region1						1
#define BMC_Region2						2
#define BMC_Region3						3
#define BMC_Region4						4
#define BMC_Region5						5
#define BMC_Region6						6
#define BMC_Region7						7
    /* Spare area size ( for sector, byte, and highspeed mode) */
#define Spare_size_1Byte				0
#define Spare_size_2Byte				1
#define Spare_size_3Byte				2
#define Spare_size_4Byte				3
#define Spare_size_5Byte				4
#define Spare_size_6Byte				5
#define Spare_size_7Byte				6
#define Spare_size_8Byte				7
#define Spare_size_9Byte				8
#define Spare_size_10Byte				9
#define Spare_size_11Byte				10
#define Spare_size_12Byte				11
#define Spare_size_13Byte				12
#define Spare_size_14Byte				13
#define Spare_size_15Byte				14
#define Spare_size_16Byte				15
	/* Spare area size (just for page mode) */
#define Spare_size_for_pagemode_4Byte	0
#define Spare_size_for_pagemode_8Byte	1
#define Spare_size_for_pagemode_16Byte	3
#define Spare_size_for_pagemode_32Byte	7
#define Spare_size_for_pagemode_64Byte	15
    /* Command Index */
#define Command(index)					index
    /* Program Flow Control */
#define Programming_Control_Flow		1
#define Fixed_Control_Flow				0
    /* Spare SRAM access region */
#define Spare_sram_region0				0
#define Spare_sram_region1				1
#define Spare_sram_region2				2
#define Spare_srma_region3				3
    /* Command Handshake mode */
#define Command_Handshake_Enable		1
#define Command_Handshake_Disable		0
    /* Command Incrememtal Scale */
#define Command_Inc_By_Twoblock			2
#define Command_Inc_By_Block			1
#define Command_Inc_By_Page				0
    /* Command Wait */
#define Command_Wait_Disable			0
#define Command_Wait_Enable				1
    /* Command Complete Interrupt */
#define Command_Complete_Intrrupt_Enable	1
#define Command_Complete_Intrrupt_Disable	0

#define BMC_Region_Status				0x0400
#define BMC_Region_Pointer_Adjust(n)	(0x404 + (n << 2))
#define DMA_Write_Data_Fill_Pop			0x0424
#define Region_Software_Reset			0x0428
#define Force_Region_Fill_Read_Data		0x042C

#define AHB_Slave_Memory_Space			0x0508
	/* AHB slave memory space range */
#define AHB_Memory_512Byte				(1 << 0)
#define AHB_Memory_1KByte				(1 << 1)
#define AHB_Memory_2KByte				(1 << 2)
#define AHB_Memory_4KByte				(1 << 3)
#define AHB_Memory_8KByte				(1 << 4)
#define AHB_Memory_16KByte				(1 << 5)
#define AHB_Memory_32KByte				(1 << 6)
#define AHB_Memory_64KByte				(1 << 7)
	/* AHB enable prefech*/
#define AHB_Prefetch_Slave_0			(1 << 12)
#define AHB_Prefetch_Slave_1			(1 << 13)
#define AHB_Prefetch_Slave_2			(1 << 14)
#define AHB_Prefetch_Slave_3			(1 << 15)
	
#define Global_Software_Reset			0x050C
#define AHB_Data_Slave_Reset			0x0510
#define DQS_In_Delay					0x0518
#define Programmable_Flow_Control		0x0600
#define Spare_Sram_Access				0x1000
#define Data_Sram_Access				0x10000
/* Large page */
#define READ_ID						0x4F
#define RESET						0x55
	
//Group 1
	#define PAGE_READ							0x1B
	#define PAGE_WRITE_WITH_SPARE				0x25
	#define PAGE_READ_WITH_SPARE				0x44
	#define SPARE_WRITE							0x30
	#define SPARE_READ							0x3A
	#define ERASE								0x58
	#define INTERNAL_COPYBACK					0x5F
	#define BLANKING_CHECK						0x6E
	#define BYTE_MODE_WRITE						0x7F
	#define BYTE_MODE_READ						0x88
	#define MULTI_READ_STATUS					0x91
	#define READ_STATUS							0x93
	#define INTERNAL_COPYBACK_WITH_CORRECTION	0x324

	//Group 2
	#define PAGE_WRITE_2P				0x96	//Samsung
	#define ERASE_2P					0xBC
	#define PAGE_READ_2P				0xC5

	//Group 3       
	#define PAGE_READ_2I				0x1A4
	#define	PAGE_WRITE_2I				0x1BE
	#define COPYBACK_2P_SAMSUNG			0x101
	#define ERASE_2I					0x24E
	#define CACHE_READ					0x2D2	// Toshiba
	#define CACHE_WRITE					0x2E7	// Toshiba
	#define ERASE_4I					0x232

	//Group 4
	#define PAGE_WRITE_2P_2I			0xD9
	#define COPYBACK_CACHE_TOSHIBA		0x2FC
	#define PAGE_READ_4I				0x1D4
	#define PAGE_WRITE_4I				0x206
	#define COPYBACK_2P_INTEL			0x114
	#define ERASE_2P_2I					0x129	//Samsung
	#define HW_COPYBACK_CROSS_CHIP		0x2B9
	#define HW_COPYBACK_NOCROSS_CHIP	0x2A6
	#define HW_COPY_2I					0x25C
	#define COPYBACK_2I					0x286
	#define PAGE_WRITE_2P_MICRON		0xA9
	#define CACHE_COPYBACK				0x2FC
	
#define AHB_Memory_8KByte			    (1 << 4)


#define FTNANDC023_32BIT(offset)		*((volatile u32 *)(FTNANDC023_BASE + offset))
#define FTNANDC023_16BIT(offset)		*((volatile u16 *)(FTNANDC023_BASE + offset))
#define FTNANDC023_8BIT(offset)			*((volatile u8 *)(FTNANDC023_BASE + offset))

#define KB_TO_B(x)  ((x) << 10)
#define B_TO_KB(x)  ((x) >> 10)

#ifndef __ASSEMBLY__

typedef enum
{
    ECC_BASE_512    = 0,
    ECC_BASE_1024   = 1
} ECC_BASE_T; 

typedef struct {
	unsigned int ST;				// 0x00
	unsigned int CTRL;				// 0x04
	unsigned int LOWADDR;				// 0x08
	unsigned int HIGHADDR;			// 0x0C
	unsigned int CFG1;				// 0x10
	unsigned int CFG2;				// 0x14
	unsigned int CFG3;				// 0x18
	unsigned int CFG4;				// 0x1C
	unsigned int DATA;				// 0x20
	unsigned int DATA_LENGTH;			// 0x24
	unsigned int DMA_FIFO;			// 0x28
	unsigned int AC_TIMING1;			// 0x2C
	unsigned int AC_TIMING2;			// 0x30
	unsigned int IOBUS_CTRL;			// 0x34
	unsigned int SF_RST;				// 0x38
	unsigned int INTCLR;				// 0x3C
	unsigned int INTENABLEMASK;			// 0x40

	unsigned int REVERSED44;			// 0x44
	unsigned int REVERSED48;			// 0x48
	unsigned int REVERSED4C;			// 0x4C

	unsigned int MAINAREAECC1;			// 0x50
	unsigned int MAINAREAECC2;			// 0x54
	unsigned int MAINAREAECC3;			// 0x58
	unsigned int MAINAREAECC4;			// 0x5C
	unsigned int LSNECC1;				// 0x60
	unsigned int LSNECC2;				// 0x64
	unsigned int LSNECC3;				// 0x68
	unsigned int LSNECC4;				// 0x6C

	unsigned int REVERSED70;			// 0x70
	unsigned int REVERSED74;			// 0x74
	unsigned int REVERSION;			// 0x78
	unsigned int CONFIG;				// 0x7C
} FTNAND020_T;

#define ptflash	((volatile FTNAND020_T *)FTNANDC023_BASE)

typedef struct {
  int sectorsize;
	int pagesize;
	int blocksize;
	int chipsize;
	int sparesize;
	int ecc_base;       //0:512, 1:1024	
	int ecc_capability; //0 ~ 23 which indicates 1 ~ 24 bits.
	int status_bit;
	int cmmand_bit;
	int row_cyc;
	int col_cyc;
} NANDCHIP_T;
	
typedef struct {
	unsigned int u16blocks_in_disk;			// Block number in Card
	unsigned int u8blocks_in_disk_exp2;		// Block number in Card
	unsigned int u8pages_in_block;			// Pages/blk
	unsigned int u8pages_in_block_exp2;		// Pages/blk

	unsigned int u16bytes_in_page;			// Bytes/page
	unsigned int u8bytes_in_page_exp2;		// Bytes/page
} NANDStruct;

/* Command Queue Sixth Word */
struct command_queue_feature {
	unsigned int Complete_interrupt_enable:1;	/* 0 */
	unsigned int Command_wait:1;				/* 1 */
	unsigned int Command_incremental_scale:2;	/* 3:2 */
	unsigned int Command_handshake_mode:1;	/* 4 */
	unsigned int Spare_sram_region:2;			/* 6:5 */
	unsigned int Program_flow:1;				/* 7 */
	unsigned int Command_index:10;			/* 17:8 */
	unsigned int Spare_size:4;				/* 21:18 */
	unsigned int BMC_region:3;				/* 24:22 */
	unsigned int BMC_ignore:1;				/* 25 */
	unsigned int Byte_mode:1;					/* 26 */
	unsigned int Command_start_ce:3;			/* 29:27 */
	unsigned int Flash_type:2;				/* 31:30 */
};

struct flash_info 
{
    /* pass by pc */        
    unsigned short u16_block_size_kb;          // unit is kbytes
    unsigned short u16_page_size_kb;	        // page_size_512byte/page_size_2kbyte/page_size_4kbyte/page_size_8kbyte
    unsigned short u16_spare_size_in_page;     // Spare area size for a whole page(bytes)	
	unsigned char  u8_status_bit;              // which bit used to indicate the status in the nand flash
	unsigned char  u8_cmmand_bit;	            // which bit used to indicate the cmd fail/success in the nand flash
	unsigned char  u8_ecc_capability;         	// ecc capability, 0/1/2/3...
	ECC_BASE_T ecc_base;               	// ecc based on 512 bytes/1k bytes
	
	/* the followings are calculated by self */
	unsigned int   u32_spare_start_in_byte_pos;
	unsigned short u16_sector_size;            // 512/1024	bytes
	unsigned short u16_page_in_block;
	unsigned char  u16_sector_in_page;         	//how many sectors in a page
	unsigned char  u8_row_cyc;                 //0 for 1 cycle....
	unsigned char  u8_col_cyc;                 //0 for 1 cycle....	
};

//=============================================================================
// NAND System Header, size = 512 bytes
//=============================================================================
typedef struct sys_header 
{
	char         signature[8];              /* Signature is "GM8126" */
	unsigned int bootm_addr;                /* default Image offset to load by nandboot */
	unsigned int burnin_addr;               /* burn-in image address */
	unsigned int uboot_addr;                /* uboot address */
	unsigned int linux_addr;                /* linux image address */
	unsigned int reserved1[7];              /* unused */
	struct 
	{
		unsigned int nand_numblks;          //number of blocks in chip
		unsigned int nand_numpgs_blk;       //how many pages in a block
		unsigned int nand_pagesz;           //real size in bytes			
		unsigned int nand_sparesz_inpage;   //64bytes for 2k, ...... needed for NANDC023
		unsigned int nand_numce;            //how many CE in chip
		unsigned int nand_status_bit;
		unsigned int nand_cmmand_bit;
		unsigned int nand_ecc_capability;
		unsigned int nand_ecc_base;         //0/1 indicates 512/1024 bytes	
		unsigned int nand_actimg1;
		unsigned int nand_actimg2;
		unsigned int nand_row_cycle;        //1 for 1 cycle ...
		unsigned int nand_col_cycle;        //1 for 1 cycle ...
		unsigned int reserved[1];	
	} nandfixup;
	unsigned int    reserved2[100];         // unused
	unsigned char   last_511[4];            // byte510:0x55, byte511:0xAA
} NAND_SYSHDR;

/* All parameters are necessary for the designated part.
 */
struct collect_data_buf {
    /* data */
	unsigned char *u8_data_buf;	
	unsigned int u32_data_length_in_bytes;
	unsigned short u16_data_length_in_sector;   //also for spare sector count
    /* Spare */
	unsigned char *u8_spare_data_buf;
	unsigned short u16_spare_length;    //total spare length for a page?
	unsigned char u8_spare_size_for_sector;    
	
	/* New */
	unsigned char u8_mode;					// 0: sector mode, 1: page mode
	unsigned char u8_bytemode;				// 0: Not bytemode, 1: Bytemode
};

typedef unsigned int BOOLEAN;
typedef unsigned char INT8U;    /* 1 byte */
typedef unsigned short INT16U;  /* 2 bytes */
typedef unsigned int INT32U;    /* 4 bytes */
typedef signed char INT8S;              /* 1 byte */
typedef signed short INT16S;            /* 2 bytes */
typedef signed int INT32S;              /* 4 bytes */
typedef unsigned int    UINT32;
typedef int     INT32;
typedef unsigned short  UINT16;
typedef short   INT16;
typedef unsigned char   UINT8;
typedef char    INT8;
typedef unsigned char   BOOL;

#ifdef CFG_NANDC_DEBUG1
# define NANDC_DEBUG1(fmt, args...) printf(fmt, ##args)
#else
# define NANDC_DEBUG1(fmt, args...)
#endif

#ifdef CFG_NANDC_DEBUG2
# define NANDC_DEBUG2(fmt, args...) printf(fmt, ##args)
#else
# define NANDC_DEBUG2(fmt, args...)
#endif

#ifdef CFG_NANDC020_DEBUG3
# define NANDC_DEBUG3(fmt, args...) printf(fmt, ##args)
#else
# define NANDC_DEBUG3(fmt, args...)
#endif

BOOL Check_command_status(unsigned char u8_channel, unsigned char detect);
void nand_load_default(NANDCHIP_T * chip);
void nand023_drv_load_config(unsigned int u16_block_pg, unsigned int u16_page_size,
                             unsigned int u16_spare_size_in_page, unsigned int u8_status_bit,
                             unsigned int u8_cmmand_bit, unsigned int ecc_capability, unsigned int ecc_base,
                             unsigned int row_cycle, unsigned int col_cycle);
BOOL nand023_drv_read_page_with_spare(unsigned int u32_row_addr, unsigned char u8_sector_offset,
                                      unsigned short u16_sector_cnt,
                                      struct collect_data_buf *data_buf, int check_status);
BOOL Read_flash_ID_setting(unsigned char u8_channel);
BOOL Setting_feature_and_fire(unsigned char u8_channel, unsigned short u16_command_index,
                              unsigned char u8_starting_ce, unsigned int u32_sixth_word);
BOOL Page_read_with_spare_setting(unsigned char u8_channel, unsigned int u32_row_addr,
                                  unsigned char u8_sector_offset, struct collect_data_buf *data_buf);
unsigned char Command_queue_status_full(unsigned char u8_channel);
BOOL FTNANDC023_clear_specified_spare_sram(UINT8 u8_channel, struct collect_data_buf *data_buf);
BOOL Byte_mode_read_setting(unsigned char u8_channel, unsigned int u32_row_addr, unsigned short u16_col_addr,
                            struct collect_data_buf *data_buf);
void Dma_init(unsigned int sync);
void nand023_drv_init(void);
BOOL Data_read(unsigned char u8_channel, struct collect_data_buf *data_buf,
               struct flash_info *flash_readable_info, unsigned char u8_data_presence,
               unsigned char u8_spare_presence);

BOOL Row_addr_cycle(unsigned char u8_cycle);
BOOL Manual_command_queue_setting(unsigned char u8_channel, unsigned int *u32_manual_command_queue);

void nand023_read_page(unsigned int u32_row_addr);
BOOL nand023_write_page(unsigned int u32_row_addr);
BOOL Data_write(unsigned char u8_channel, struct collect_data_buf *data_buf,
                struct flash_info *flash_readable_info, unsigned char u8_data_presence,
                unsigned char u8_spare_presence);
BOOL spare_dirty(UINT32 len, unsigned char* buf);
void write_protect(int mode);
void init_cmd_feature_default(struct command_queue_feature *cmd_feature);

#endif	/* __ASSEMBLY__ */
#endif	/* __NANDC_FLASH_H */