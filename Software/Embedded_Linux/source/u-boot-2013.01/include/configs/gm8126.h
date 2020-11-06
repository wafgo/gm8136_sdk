/*
 * (C) Copyright 2009 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
 *
 * Configuation settings for the Faraday A320 board.
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

#ifndef __CONFIG_H
#define __CONFIG_H

#include <asm/arch-gm8126/gm8126.h>
#include <asm/arch-gm8126/ahbdma.h>
#include <asm/arch-gm8126/nandc_flash.h>

/*
 * mach-type definition
 */
#define MACH_TYPE_FARADAY	758
#define CONFIG_MACH_TYPE	MACH_TYPE_FARADAY
#define CONFIG_FA626                    1               /* This is an FA626 Core: sync_cache()	*/
#define CONFIG_PLATFORM_GM8126          1
/*
 * Linux kernel tagged list
 */
#define CONFIG_INITRD_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_CMDLINE_EDITING
#define CONFIG_AUTO_COMPLETE

/***********************************************************
 * Command definition
 ***********************************************************/
#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_FTMAC110
#define CONFIG_GM_AHBDMA
#define CONFIG_BOOTDELAY	3 



//#define CONFIG_SYS_NO_FLASH
//#define CONFIG_SYS_FLASH_CFI
#define CONFIG_CMD_SPI
//#define CONFIG_CMD_NAND


#define DMA_BURST_128		0x6L
#define DMA_BURST_256		0x7L

/*
 * FLASH and environment organization
 */
#ifdef CONFIG_SYS_NO_FLASH
#undef CONFIG_CMD_IMLS
//#undef CONFIG_CMD_SAVEENV
#undef CONFIG_ENV_IS_IN_FLASH
#undef CONFIG_SYS_FLASH_CFI
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SIZE        		0x10000
#else
#define CONFIG_SYS_MAX_FLASH_BANKS	1	    /* number of banks */
#define CONFIG_SYS_MAX_FLASH_SECT	1024	/* sectors per device */
#endif

#ifndef CONFIG_SKIP_LOWLEVEL_INIT
#define  CONFIG_USE_IRQ     1
#endif

#ifdef CONFIG_USE_IRQ
#define  IRQ_PMU	                8
#define  CONFIG_STACKSIZE_IRQ       (4*1024)
#define  CONFIG_STACKSIZE_FIQ       (4*1024)
#endif
/***********************************************************
 * CFI NOR FLASH argument
 ***********************************************************/
#ifdef CONFIG_SYS_FLASH_CFI
#define CONFIG_FLASH_CFI_DRIVER

/* support JEDEC */
#define CONFIG_FLASH_CFI_LEGACY
#define CONFIG_SYS_FLASH_LEGACY_512Kx8

/* max number of memory banks */
#define CONFIG_SYS_MAX_FLASH_BANKS	2 
/* max number of sectors on one chip */
#define CONFIG_SYS_MAX_FLASH_SECT	512
#define PHYS_FLASH			0x00000000
#define PHYS_FLASH_2			0x00400000
#define CONFIG_SYS_FLASH_BASE		PHYS_FLASH
#define CONFIG_SYS_FLASH_BANKS_LIST	{ PHYS_FLASH, PHYS_FLASH_2, }

#define CONFIG_SYS_MONITOR_BASE		PHYS_FLASH
#undef CONFIG_SYS_FLASH_EMPTY_INFO

#define CONFIG_FTSMC020
#include <faraday/ftsmc020.h>

#define FTSMC020_BANK0_CONFIG	(FTSMC020_BANK_ENABLE             |	\
				 FTSMC020_BANK_BASE(PHYS_FLASH) |	\
				 FTSMC020_BANK_SIZE_1M            |	\
				 FTSMC020_BANK_MBW_8)

#define FTSMC020_BANK0_TIMING	(FTSMC020_TPR_RBE      |	\
				 FTSMC020_TPR_AST(3)   |	\
				 FTSMC020_TPR_CTW(3)   |	\
				 FTSMC020_TPR_ATI(0xf) |	\
				 FTSMC020_TPR_AT2(3)   |	\
				 FTSMC020_TPR_WTC(3)   |	\
				 FTSMC020_TPR_AHT(3)   |	\
				 FTSMC020_TPR_TRNA(0xf))

#define CONFIG_SYS_FTSMC020_CONFIGS	{			\
	{ FTSMC020_BANK0_CONFIG, FTSMC020_BANK0_TIMING, },	\
}
#endif
 
/***********************************************************
 * SPI FLASH argument
 ***********************************************************/
#ifdef CONFIG_CMD_SPI
#define CONFIG_HARD_SPI		        1
#define CONFIG_CMD_SF
#define CONFIG_SYS_NO_FLASH			1   // don't need run flash_init

#define CONFIG_SPI_FLASH            1
#define CONFIG_SPI_FLASH_EON        1
#define CONFIG_SPI_FLASH_MACRONIX   1
#define CONFIG_SPI_FLASH_SPANSION   1
//#define CONFIG_SPI_FLASH_SST        1
//#define CONFIG_SPI_FLASH_STMICRO    1
#define CONFIG_SPI_FLASH_WINBOND    1

#define CONFIG_FTSSP010_SPI         1

#define CONFIG_ENV_OFFSET        	0x20000  
#define CONFIG_ENV_SIZE        		0x10000    	/* 1 block */
#define CONFIG_ENV_SECT_SIZE        0x10000
#define CONFIG_ENV_SPI_BUS          0
#define CONFIG_ENV_SPI_CS           0
#define CONFIG_ENV_SPI_MAX_HZ       30000000    /*30Mhz */

#define CONFIG_BOOTCOMMAND          "sf probe 0:0;sf read 0x2000000 0x16100 0x200000;"\
                                    "go 0x2000000"	
					   
/* environments */
#define CONFIG_ENV_IS_IN_SPI_FLASH  1
//#define CONFIG_ENV_ADDR			    0x60000
//#define CONFIG_ENV_SIZE			    0x20000
#endif

/***********************************************************
 * NAND FLASH argument
 ***********************************************************/
#ifdef CONFIG_CMD_NAND
#define CONFIG_SYS_NO_FLASH			1   // don't need run flash_init
#define CONFIG_NAND_GM              1

#define NAND_CHANNEL                0	/* Use CH0 in DMAC */
#define REQ_NANDTX  9   
#define REQ_NANDRX  9

#define DMA_MEM2MEM		((0UL << 8) | 0)            /* index 0 */
#define DMA_MEM2NAND	1   /* index 1 */
#define DMA_NAND2MEM	2   /* index 2 */

#define CONFIG_CMD_JFFS2
#define CONFIG_SYS_MAX_NAND_DEVICE	1
//#define CONFIG_SYS_NAND_MAX_CHIPS	1
#define CONFIG_SYS_NAND_BASE        CONFIG_FTNANDC023_BASE//NAND023_DATA_PORT

#define IMAGE_MAGIC                 0x805A474D
#define SS_SIGNATURE                "GM8126"

#define CONFIG_BOOTCOMMAND          "nand read 0x4000000 0x500000 0x1000000;" \
                                    "go 0x4000800"

//#define CONFIG_MTD_DEBUG            // u-boot NAND default debug
//#define CONFIG_MTD_DEBUG_VERBOSE    3

#define ADDR_COLUMN                 1
#define ADDR_PAGE                   2
#define ADDR_COLUMN_PAGE            3

#define NAND_ChipID_UNKNOWN         0x00
#define NAND_MAX_FLOORS             1
#define NAND_MAX_CHIPS              1

//#define CONFIG_ENV_IS_IN_NAND       1
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_OFFSET           0x440000   
#define CONFIG_ENV_SIZE             0x20000			/* must reserve 0x20000 for erase */
#endif

/*
 * Power Management Unit
 */
//#define CONFIG_FTPMU010_POWER

/*
 * Platform setting
 */
#define SYS_CLK		30000000
#define GET_PLAT_FULL_ID()		(inw(CONFIG_PMU_BASE))
#define GPLAT_8126_1080P_ID 	0x81262100
#define GPLAT_8126_1080P_1_ID 0x81262200
#define PMU_PMODE_OFFSET      0x0C
#define PMU_PLL1CR_OFFSET     0x30
#define PMU_PLL23CR_OFFSET    0x34

/*
 * Timer
 */
#define CONFIG_SYS_HZ		1000	/* timer ticks per second */

/*
 * Real Time Clock
 */
//#define CONFIG_RTC_FTRTC010
//#define CONFIG_SERIAL

/*
 * Serial console configuration
 */

/* FTUART is a high speed NS 16C550A compatible UART */
#define CONFIG_BAUDRATE			38400
#define CONFIG_CONS_INDEX		1
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_COM1		CONFIG_UART_BASE
#define CONFIG_SYS_NS16550_REG_SIZE	-4
#define CONFIG_SYS_NS16550_CLK		18432000

/*
 * Ethernet, Autobooting
 */
#define INTERNAL_PHY
 
#define CONFIG_ETHADDR                  00:42:70:00:30:22  /* used by common/env_common.c */
#define CONFIG_NETMASK                  255.0.0.0
#define CONFIG_IPADDR                   10.0.1.52
#define CONFIG_SERVERIP                 10.0.1.51
#define CONFIG_GATEWAYIP                10.0.1.51

/*
 * Command line configuration.
 */
#include <config_cmd_default.h>

#define CONFIG_CMD_CACHE
//#define CONFIG_CMD_DATE // need RTC
#define CONFIG_CMD_PING

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP			/* undef to save memory */
#define CONFIG_SYS_PROMPT	"A320 # "	/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE	256		/* Console I/O Buffer Size */

/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE	\
	(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)

/* max number of command args */
#define CONFIG_SYS_MAXARGS	16

/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 128 * 1024)


/*
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	1		/* we have 1 bank of DRAM */
#define PHYS_SDRAM		0x00000000	/* SDRAM Bank #1 */
#define PHYS_SDRAM_SIZE	0x08000000	/* 128 MB */

#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM


/*
 * Load address and memory test area should agree with
 * board/faraday/a320/config.mk. Be careful not to overwrite U-boot itself.
 */
#define CONFIG_SYS_LOAD_ADDR		(PHYS_SDRAM + 0x2000000)

#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_LOAD_ADDR + 0x1000 - \
					GENERATED_GBL_DATA_SIZE)

/* memtest works on 63 MB in DRAM */
#define CONFIG_SYS_MEMTEST_START	PHYS_SDRAM
#define CONFIG_SYS_MEMTEST_END		(PHYS_SDRAM + PHYS_SDRAM_SIZE - 0x100000)

#define CONFIG_SYS_TEXT_BASE		0
#ifdef	DEBUG
#define	CONFIG_SYS_MONITOR_LEN		(256 << 10)	/* Reserve 256 kB for Monitor	*/
#else
#define	CONFIG_SYS_MONITOR_LEN		(128 << 10)	/* Reserve 128 kB for Monitor	*/
#endif


#endif	/* __CONFIG_H */
