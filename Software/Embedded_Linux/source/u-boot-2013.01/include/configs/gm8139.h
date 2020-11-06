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

#include <asm/arch-gm8139/gm8139.h>
#include <asm/arch-gm8139/ahbdma.h>
#include <asm/arch-gm8139/nandc_flash.h>
#include <asm/arch-gm8139/spi020.h>
#include <config_cmd_default.h>
/*
 * mach-type definition
 */
#define MACH_TYPE_FARADAY	758
#define CONFIG_MACH_TYPE	MACH_TYPE_FARADAY
#define CONFIG_FA626                    1               /* This is an FA626 Core: sync_cache()	*/
#define CONFIG_PLATFORM_GM8139          1
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
//#define DDR_SIZE_512MB
//#define DDR_SIZE_256MB
//#define DDR_SIZE_128MB
#define DDR_SIZE_64MB
#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_BOOTDELAY	3 
#define CONFIG_FTGMAC100
#define CONFIG_GM_AHBDMA
//#define CONFIG_CMD_FPGA
//#define CONFIG_CMD_I2C
//#define CONFIG_VIDEO
//#define CONFIG_MMC
#define CONFIG_FOTG210
#define CONFIG_AUTO_UPDATE

#define CONFIG_RMII					1
//#define CONFIG_RGMII			    1

//#define CONFIG_SYS_NO_FLASH
#define CONFIG_CMD_SPI
//#define CONFIG_SPI_NAND_GM
//#define CONFIG_CMD_NAND

//#include <config_cmd_default.h>
#define CONFIG_CMD_CACHE
//#define CONFIG_CMD_DATE // need RTC
#define CONFIG_CMD_PING

//#ifndef CONFIG_SKIP_LOWLEVEL_INIT
//#define  CONFIG_USE_IRQ             1
//#endif

#ifdef CONFIG_USE_IRQ
#define  IRQ_PMU	                8
#define  CONFIG_STACKSIZE_IRQ       (4*1024)
#define  CONFIG_STACKSIZE_FIQ       (4*1024)
#endif

/***********************************************************
 * USB Configuration
 ***********************************************************/
#ifdef CONFIG_FOTG210
#define CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_FOTG210
#define CONFIG_USB_STORAGE
#define CONFIG_USB_EHCI_BASE        CONFIG_FOTG210_0_BASE
#define CONFIG_USB_MAX_CONTROLLER_COUNT    1
#define CONFIG_USB_EHCI_BASE_LIST  \
	{ CONFIG_FOTG210_0_BASE }
#define CONFIG_CMD_FAT
#define CONFIG_FS_FAT
#define CONFIG_SUPPORT_VFAT
#define CONFIG_DOS_PARTITION
#endif

/***********************************************************
 * No FLASH argument
 ***********************************************************/
#ifdef CONFIG_SYS_NO_FLASH
#undef CONFIG_CMD_IMLS
//#undef CONFIG_CMD_SAVEENV
#undef CONFIG_ENV_IS_IN_FLASH
#undef CONFIG_SYS_FLASH_CFI
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SIZE        		0x10000
#endif
/***********************************************************
 * SPI FLASH argument
 ***********************************************************/
#ifdef CONFIG_CMD_SPI
#define CONFIG_HARD_SPI		        1
#define CONFIG_CMD_SF
#define CONFIG_SYS_NO_FLASH			1   // don't need run flash_init
#undef  CONFIG_CMD_IMLS

#define CONFIG_SPI_FLASH            1
#define CONFIG_SPI_FLASH_EON        1
#define CONFIG_SPI_FLASH_MACRONIX   1
#define CONFIG_SPI_FLASH_SPANSION   1
#define CONFIG_SPI_FLASH_ESMT				1
//#define CONFIG_SPI_FLASH_SST        1
//#define CONFIG_SPI_FLASH_STMICRO    1
#define CONFIG_SPI_FLASH_WINBOND    1
#define CONFIG_SPI_FLASH_GD         1

#define CONFIG_FTSPI020_SPI         1
#define CONFIG_SPI_DMA
//#define CONFIG_SPI_QUAD

#define SPI020_CHANNEL            	0	/* Use CH0 in DMAC */
#define REQ_SPI020TX				8
#define REQ_SPI020RX				8
#define DMA_MEM2SPI020	            2
#define DMA_SPI0202MEM	            3

#define CONFIG_CMD_MTDPARTS
#define CONFIG_MTD_PARTITIONS
#define CONFIG_MTD_DEVICE
#define MTDIDS_DEFAULT		"nor0=nor-flash"
#define MTDPARTS_DEFAULT	"mtdparts=nor-flash:0x50000@0x10000(uboot)," \
	"0x3A0000@0x60000(linux),2M@4M(fs),1M@6M(user0),-(reserved)"

#define CONFIG_ENV_IS_IN_SPI_FLASH  1
#define CONFIG_ENV_OFFSET        	0x50000  
#define CONFIG_ENV_SIZE        		0x10000    	/* 1 block */
#define CONFIG_ENV_SECT_SIZE        0x10000

#define CONFIG_ENV_SPI_BUS          0
#define CONFIG_ENV_SPI_CS           0
#define CONFIG_ENV_SPI_MAX_HZ       30000000    /*30Mhz */

/* cmd1 for 8138, cmd2 for 8139 */
#define CONFIG_EXTRA_ENV_SETTINGS					 \
	"lm=sf read 0x02000000 z\0" \
	"cmd1=mem=256M gmmem=190M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock2 rootfstype=squashfs\0" \
	"cmd2=mem=512M gmmem=432M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock2 rootfstype=squashfs\0" \
	"cmd3=mem=128M gmmem=90M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock2 rootfstype=squashfs\0"

#define CONFIG_BOOTCOMMAND          "sf probe 0:0;run lm;bootm 0x2000000"		
#define CONFIG_VIDEO_FB_BASE	    0xE000000	/* frame buffer address */
#endif

/***********************************************************
 * NAND FLASH argument
 ***********************************************************/
#if defined(CONFIG_CMD_NAND) || defined(CONFIG_SPI_NAND_GM)
//====================== normal NAND ======================
#ifdef CONFIG_CMD_NAND
#undef CONFIG_CMD_FLASH
#define CONFIG_SYS_NO_FLASH			1   // don't need run flash_init
#define CONFIG_NAND024V2_GM     1
#define NAND_CHANNEL            0	/* Use CH0 in DMAC */
#define REQ_NANDTX							2   
#define REQ_NANDRX							2
#define DMA_MEM2NAND	      		0
#define DMA_NAND2MEM	     			1
#endif
//====================== SPI NAND ======================
#ifdef CONFIG_SPI_NAND_GM
#define CONFIG_CMD_NAND
#undef CONFIG_CMD_FLASH
#define CONFIG_SYS_NO_FLASH			1   // don't need run flash_init
#define CONFIG_SPI_DMA

#define SPI020_CHANNEL      		0	/* Use CH0 in DMAC */
#define REQ_SPI020TX						8
#define REQ_SPI020RX						8
#define DMA_MEM2SPI020	    		2
#define DMA_SPI0202MEM	    		3
#endif

#define CONFIG_CMD_MTDPARTS
#define CONFIG_MTD_PARTITIONS
#define CONFIG_MTD_DEVICE
#define MTDIDS_DEFAULT		"nand0=nand-flash"
#define MTDPARTS_DEFAULT	"mtdparts=nand-flash:0x100000@0x140000(uboot)," \
	"5M@16M(linux),5M@32M(fs),5M@40M(user0),-(reserved)"

//#define CONFIG_SPI_QUAD
//#define CONFIG_GPIO_WP
#undef  CONFIG_CMD_IMLS

#define CONFIG_CMD_JFFS2
#define CONFIG_SYS_MAX_NAND_DEVICE	1
//#define CONFIG_SYS_NAND_MAX_CHIPS	1
#define CONFIG_SYS_NAND_BASE        CONFIG_FTNANDC023_BASE//NAND023_DATA_PORT

#define SS_SIGNATURE                "GM8129"

/* cmd1 for 8138, cmd2 for 8139 */
#define CONFIG_EXTRA_ENV_SETTINGS					 \
	"lm=nand read 0x02000000 z\0" \
	"cmd1=mem=256M gmmem=190M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock2 rootfstype=squashfs\0" \
	"cmd2=mem=512M gmmem=432M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock2 rootfstype=squashfs\0" \
	"cmd3=mem=128M gmmem=90M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock2 rootfstype=squashfs\0"

#define CONFIG_BOOTCOMMAND          "run lm;bootm 0x2000000"

//#define CONFIG_MTD_DEBUG            // u-boot NAND default debug
//#define CONFIG_MTD_DEBUG_VERBOSE    3

#define ADDR_COLUMN                 1
#define ADDR_PAGE                   2
#define ADDR_COLUMN_PAGE            3

#define NAND_ChipID_UNKNOWN         0x00
#define NAND_MAX_FLOORS             1
#define NAND_MAX_CHIPS              1

#define CONFIG_ENV_IS_IN_NAND       1
//#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_OFFSET           0x220000   
#define CONFIG_ENV_SIZE             0x20000			/* must reserve 0x20000 for erase */

#define CONFIG_VIDEO_FB_BASE	    0xE000000	/* frame buffer address */
#endif

/***********************************************************
 * I2C support
 ***********************************************************/
#ifdef CONFIG_CMD_I2C
#define CONFIG_SYS_LONGHELP
#define CONFIG_HARD_I2C		1	/* I2C with hardware support */

#define CONFIG_I2C_GM		1
#define CFG_I2C_SPEED		100000 /* 100 kHz */
#define CFG_I2C_SLAVE		0xFE		 /* not be used, compiler issue */ 

#define I2C_GSR_Value		0x2
#define I2C_TSR_Value		0x27
#endif

/***********************************************************
 * VIDEO Logo support
 ***********************************************************/
#ifdef CONFIG_VIDEO
#define CONFIG_VIDEO_LOGO
#define CONFIG_VIDEO_FB_BASE    0xE000000	/* frame buffer address */
#endif

/***********************************************************
 * Platform setting
 ***********************************************************/
#define CONFIG_SYS_HZ		1000	/* timer ticks per second */
#define SYS_CLK		        30000000
#define GPLAT_8139_ID 	    0x81391000
#define PMU_PMODE_OFFSET    0x0C
#define PMU_PLL1CR_OFFSET   0x30
#define PMU_PLL23CR_OFFSET  0x34


/***********************************************************
 * Serial console configuration
 ***********************************************************/
/* FTUART is a high speed NS 16C550A compatible UART */
#ifdef CONFIG_CMD_FPGA
#define CONFIG_BAUDRATE			    38400
#else
#define CONFIG_BAUDRATE			    115200
#endif
#define CONFIG_CONS_INDEX		    1
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_COM1		CONFIG_UART_BASE
#define CONFIG_SYS_NS16550_REG_SIZE	-4
#define CONFIG_SYS_NS16550_CLK		25000000

/***********************************************************
 * Ethernet, Autobooting
 ***********************************************************/
#define CONFIG_NET_NUM                  1

#define CONFIG_PHY_ADDR					0
//#define CONFIG_PHY_ADDR				0x10
#ifndef CONFIG_RMII
#define CONFIG_PHY_GIGE					1
#define CFG_PHY_ID						0x0D90	/* GIGE only check this IP1001 ID */
//#define CFG_PHY_ID					0xC915	/* GIGE only check this rtl8211 ID */
#else
#define CFG_PHY_ID						0x0C54	/* GIGE only check this IP101G ID */
#endif

#define CFG_DISCOVER_PHY

#define CONFIG_CMD_MII					1
#define CONFIG_DRIVER_ETHER
#define CONFIG_NET_RETRY_COUNT          100

#define CONFIG_OVERWRITE_ETHADDR_ONCE
#define CONFIG_ETHADDR                  00:42:70:00:30:22  /* used by common/env_common.c */
#define CONFIG_NETMASK                  255.0.0.0
#define CONFIG_IPADDR                   10.0.1.52
#define CONFIG_SERVERIP                 10.0.1.51
#define CONFIG_GATEWAYIP                10.0.1.51

/***********************************************************
 * SD (MMC) controller
 ***********************************************************/
#ifdef CONFIG_MMC
#define CONFIG_CMD_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_DOS_PARTITION
#define CONFIG_FTSDC010
#define CONFIG_FTSDC010_NUMBER		1
#define CONFIG_CMD_FAT
#endif

/***********************************************************
 * Default image file name for firmware upgrade
 ***********************************************************/
#ifdef CONFIG_AUTO_UPDATE
#define AU_ENABLE "no"  /* execute firmware auto update as system up, "yes":enable, "no":disable */

#define AU_IMG_FILE_AIO "flash.img" /* store the content of whole flash */

#ifdef CONFIG_CMD_NAND
#ifdef CONFIG_SPI_NAND_GM
#define AU_IMG_FILE_0   "u-boot_spi_nand_rmii.bin"
#else
#define AU_IMG_FILE_0   "u-boot_nand_rmii.bin"
#endif
#endif
#ifdef CONFIG_CMD_SPI
#define AU_IMG_FILE_0   "u-boot_spi_rmii.bin"
#endif
#define AU_IMG_FILE_1   "uImage_813x"
#define AU_IMG_FILE_2   "rootfs-cpio_813x.squashfs.img"
#define AU_IMG_FILE_3   "mtd.img"
#define AU_IMG_FILE_4   ""
#define AU_IMG_FILE_5   ""
#define AU_IMG_FILE_6   ""
#define AU_IMG_FILE_7   ""
#define AU_IMG_FILE_8   ""
#define AU_IMG_FILE_9   ""

/* Attention : filename must NOT be NULL to append to CONFIG_AU_ENV_SETTING */
#define CONFIG_AU_ENV_SETTING \
    "autoupdate=" AU_ENABLE "\0" \
    "auimgaio=" AU_IMG_FILE_AIO "\0" \
    "auimg0=" AU_IMG_FILE_0 "\0" \
    "auimg1=" AU_IMG_FILE_1 "\0" \
    "auimg2=" AU_IMG_FILE_2 "\0" \
    "auimg3=" AU_IMG_FILE_3 "\0"
#endif

/***********************************************************
 * Miscellaneous configurable options
 ***********************************************************/
#define CONFIG_SYS_LONGHELP			/* undef to save memory */
#define CONFIG_SYS_PROMPT	"GM # "	/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE	1024//256		/* Console I/O Buffer Size */

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
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 1024 * 1024)


/*
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	1		/* we have 1 bank of DRAM */
#define PHYS_SDRAM		0x00000000	/* SDRAM Bank #1 */
#ifdef DDR_SIZE_64MB
#define PHYS_SDRAM_SIZE	0x04000000	/* 64 MB */
#endif
#ifdef DDR_SIZE_128MB
#define PHYS_SDRAM_SIZE	0x08000000	/* 128 MB */
#endif
#ifdef DDR_SIZE_256MB
#define PHYS_SDRAM_SIZE	0x10000000	/* 256 MB */
#endif
#ifdef DDR_SIZE_512MB
#define PHYS_SDRAM_SIZE	0x20000000	/* 512 MB */
#endif

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
