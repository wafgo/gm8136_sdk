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

#include <asm/arch-gm8210/gm8210.h>
#include <asm/arch-gm8210/ahbdma.h>
#include <asm/arch-gm8210/nandc_flash.h>
#include <asm/arch-gm8210/spi020.h>
#include <config_cmd_default.h>
/*
 * mach-type definition
 */
#define MACH_TYPE_FARADAY	758
#define CONFIG_MACH_TYPE	MACH_TYPE_FARADAY
#define CONFIG_FA626                    1               /* This is an FA626 Core: sync_cache()	*/
#define CONFIG_PLATFORM_GM8210          1
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
//#define DDR_SIZE_256MB
//#define DDR_SIZE_512MB
//#define DDR_SIZE_768MB
#define DDR_SIZE_1GB
//#define DDR_SIZE_2GB
#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_BOOTDELAY	3
#define CONFIG_FTGMAC100
#define CONFIG_GM_AHBDMA
//#define CONFIG_CMD_FPGA
#define CONFIG_CMD_I2C
#define CONFIG_VIDEO
//#define CONFIG_MMC
#define CONFIG_FOTG210
#define CONFIG_AUTO_UPDATE

//#define CONFIG_SYS_NO_FLASH
//#define CONFIG_SYS_FLASH_CFI
//#define CONFIG_CMD_SPI
#define CONFIG_CMD_NAND

//#define CONFIG_CHIP_MULTI

//#define CONFIG_GM_SINGLE    //use two chip EVB to simulation one chip EVB, slave_gm8210_quantity must always set 0
//#define CONFIG_CHIP_NUM   1

//#include <config_cmd_default.h>
#define CONFIG_CMD_CACHE
//#define CONFIG_CMD_DATE // need RTC
//#define CONFIG_CMD_PING

//#ifndef CONFIG_SKIP_LOWLEVEL_INIT
#define  CONFIG_USE_IRQ             1
//#endif

#ifdef CONFIG_USE_IRQ
#define  IRQ_PMU	                8
#define  CONFIG_STACKSIZE_IRQ       (4*1024)
#define  CONFIG_STACKSIZE_FIQ       (4*1024)
#endif

/***********************************************************
 * PCIE and Multi Chip Configuration
 ***********************************************************/
#define SLAVE_BOOT_SIGNATURE    0x474D8210

#ifdef CONFIG_MAP_PCIE_ADDR_A2C
#define SLAVE_8210_PCICFG_BAR0_BASE     0xE0000000 // map to local 0x98000000 on 2nd 8210
#else
#define SLAVE_8210_PCICFG_BAR0_BASE     0xB0000000 // map to local 0x98000000 on 2nd 8210
#endif
#define SLAVE_8210_SCU_BASE             (SLAVE_8210_PCICFG_BAR0_BASE + 0x01000000)
#define SLAVE_8210_PCIE_BASE            (SLAVE_8210_PCICFG_BAR0_BASE + 0x01E00000)
#define SLAVE_8210_CPU_IC0_BASE         (SLAVE_8210_PCICFG_BAR0_BASE + 0x02600000)
#define SLAVE_8210_CAP_IC3_BASE         (SLAVE_8210_PCICFG_BAR0_BASE + 0x02900000)
#define SLAVE_8210_ENC0_IC4_BASE        (SLAVE_8210_PCICFG_BAR0_BASE + 0x02A00000)
#define SLAVE_8210_ENC1_IC5_BASE        (SLAVE_8210_PCICFG_BAR0_BASE + 0x02B00000)
#define SLAVE_8210_3D_IC8_BASE          (SLAVE_8210_PCICFG_BAR0_BASE + 0x02E00000)
#define SLAVE_8210_PCIE_IC9_BASE        (SLAVE_8210_PCICFG_BAR0_BASE + 0x02F00000)
#define SLAVE_8210_MCP100_SRAM0_BASE    (SLAVE_8210_PCICFG_BAR0_BASE + 0x04000000)
#define SLAVE_8210_MCP100_SRAM1_BASE    (SLAVE_8210_PCICFG_BAR0_BASE + 0x04008000)
#define SLAVE_8210_MCP100_SRAM2_BASE    (SLAVE_8210_PCICFG_BAR0_BASE + 0x04010000)
#define SLAVE_8210_DDR_BASE             (SLAVE_8210_PCICFG_BAR0_BASE + 0x08000000)
#define SLAVE_8210_PCICFG_BAR1_BASE     0xF0000000 // map to local 0x00000000 on 2nd 8210
#define SLAVE_8210_AXIS_WIN0_BASE       SLAVE_8210_PCICFG_BAR0_BASE // just reverse map direction
#define SLAVE_8210_AXIS_WIN2_BASE       SLAVE_8210_DDR_BASE         // just reverse map direction
#define SLAVE_8210_AXIS_WIN3_BASE       SLAVE_8210_PCICFG_BAR1_BASE // just reverse map direction

#define DDR_LOAD_IMAGE_ADDR     0x02000000 // ep linux loaded on 1st 8210
#define DEVICE_LINUX_IMAGE_ADDR 0x02000040 // ddr offset on 2nd 8210, add uImage header

#define EP_CMDLINE_ADDR 0x01800000
#define CMDLINE_SIZE 0x400

/***********************************************************
 * USB Configuration
 ***********************************************************/
#ifdef CONFIG_FOTG210
#define CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_FOTG210
#define CONFIG_USB_STORAGE
#define CONFIG_USB_EHCI_BASE        CONFIG_FOTG210_0_BASE
#define CONFIG_USB_MAX_CONTROLLER_COUNT    2
#define CONFIG_USB_EHCI_BASE_LIST  \
	{ CONFIG_FOTG210_0_BASE, CONFIG_FOTG210_1_BASE }
#endif

/***********************************************************
 * FLASH and environment organization
 ***********************************************************/
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

/***********************************************************
 * CFI NOR FLASH argument
 ***********************************************************/
#ifdef CONFIG_SYS_FLASH_CFI
//#define CONFIG_FLASH_CFI_DRIVER   //use default driver

/* support JEDEC */
#define CONFIG_FLASH_CFI_LEGACY
#define CONFIG_SYS_FLASH_LEGACY_512Kx8

#define PHYS_FLASH			        0x00000000
#define PHYS_FLASH_2			    0x00400000
#define CONFIG_SYS_FLASH_BASE		PHYS_FLASH
#define CONFIG_SYS_FLASH_BANKS_LIST	{ PHYS_FLASH, PHYS_FLASH_2, }

#define CONFIG_SYS_MONITOR_BASE		PHYS_FLASH
#undef CONFIG_SYS_FLASH_EMPTY_INFO

#define CONFIG_BOOTCOMMAND          "md 0;" \
                                    "go 0x2000000"

#ifdef DDR_SIZE_2GB
#define CFG_FLASH_BASE                  0x81000000      /* remap with DDR */
#endif
#ifdef DDR_SIZE_1GB
#define CFG_FLASH_BASE                  0x40000000      /* remap with DDR */
#endif
#ifdef DDR_SIZE_768MB
#define CFG_FLASH_BASE                  0x30000000      /* remap with DDR */
#endif
#ifdef DDR_SIZE_512MB
#define CFG_FLASH_BASE                  0x20000000      /* remap with DDR */
#endif
#ifdef DDR_SIZE_256MB
#define CFG_FLASH_BASE                  0x10000000      /* remap with DDR */
#endif
#define PHYS_FLASH_SIZE                 0x1000000        /* 16 MB */

#define CFG_MAX_FLASH_BANKS             1                /* max num of flash banks */
#define CFG_FLASH_ERASE_TOUT            120000           /* (in ms) */
#define CFG_FLASH_WRITE_TOUT            500              /* (in ms) */

#define CONFIG_ENV_IS_IN_FLASH          1
#define CONFIG_ENV_SIZE                 0x20000
#define CONFIG_ENV_SECT_SIZE            0x20000
#define CONFIG_ENV_ADDR                 (CFG_FLASH_BASE + 0x000c0000)

#define CONFIG_FTSMC020

#define CONFIG_VIDEO_FB_BASE	    0xE000000	/* frame buffer address */
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

#define CONFIG_ENV_IS_IN_SPI_FLASH  1
#define CONFIG_ENV_OFFSET        	0x50000
#define CONFIG_ENV_SIZE        		0x10000    	/* 1 block */
#define CONFIG_ENV_SECT_SIZE        0x10000

#define CONFIG_ENV_SPI_BUS          0
#define CONFIG_ENV_SPI_CS           0
#define CONFIG_ENV_SPI_MAX_HZ       30000000    /*30Mhz */

/* lm: load master image, ls: load slave image, la: load audio image */
/* setm for master, sets for slave, setepm for EP master, seteps for EP slave */
//-------------------------------------DDR_SIZE_512MB-------------------------------------------/
#ifdef DDR_SIZE_512MB
#define CONFIG_EXTRA_ENV_SETTINGS					 \
	"lm=sf read 0x02000000 y\0" \
	"ls=sf read 0x18100000 y\0" \
	"la=sf read 0x00000000 z\0" \
	"lf=sf read 0x1F600000 x\0" \
	"set1=run lm;run ls;run la;run lf;\0" \
	"bootargs=mem=112M@0x01000000 gmmem=30M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock4 rootfstype=squashfs\0" \
	"cmds=initrd=0x1f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=128M@0x08000000 gmmem=90M console=ttyS0,115200 mem=256M@0x10000000\0"

#define CONFIG_BOOTCOMMAND          "sf probe 0:0;run set1;bootm 0x2000000"

#endif

//-------------------------------------DDR_SIZE_768MB-------------------------------------------/
#ifdef DDR_SIZE_768MB
#ifdef CONFIG_CHIP_MULTI
#define CONFIG_EXTRA_ENV_SETTINGS					 \
	"lm=sf read 0x02000000 y\0" \
	"ls=sf read 0x18100000 y\0" \
	"la=sf read 0x00000000 z\0" \
	"lmf=sf read 0x0F600000 x\0" \
	"lsf=sf read 0x2F600000 x\0" \
	"ldm=sf read 0x02000000 w\0" \
	"lds=sf read 0x18100000 w\0" \
	"ldmf=sf read 0x0F600000 w\0" \
	"ldsf=sf read 0x2F600000 w\0" \
	"lda=sf read 0x00000000 w\0" \
	"set1=run lm;run ls;run lmf;run lsf;run la;\0" \
	"set2=run ldm;run lds;run ldmf;run ldsf;run lda;\0" \
	"bootargs=mem=240M@0x01000000 gmmem=50M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock4 rootfstype=squashfs\0" \
	"cmds=initrd=0x2f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=256M@0x10000000 gmmem=190M console=ttyS0,115200 mem=256M@0x20000000\0" \
	"cmdepm=initrd=0x0f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=240M@0x01000000 gmmem=100M console=ttyS0,115200 user_debug=31\0" \
	"cmdeps=initrd=0x2f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=256M@0x10000000 gmmem=190M console=ttyS0,115200 mem=256M@0x20000000\0"

#define CONFIG_BOOTCOMMAND          "sf probe 0:0;run set1;run set2;bootm 0x2000000"

#else
#define CONFIG_EXTRA_ENV_SETTINGS					 \
	"lm=sf read 0x02000000 y\0" \
	"ls=sf read 0x18100000 y\0" \
	"la=sf read 0x00000000 z\0" \
	"lf=sf read 0x2F600000 x\0" \
	"set1=run lm;run ls;run la;run lf;\0" \
	"bootargs=mem=240M@0x01000000 gmmem=30M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock4 rootfstype=squashfs\0" \
	"cmds=initrd=0x2f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=256M@0x10000000 gmmem=190M console=ttyS0,115200 mem=256M@0x20000000\0"

#define CONFIG_BOOTCOMMAND          "sf probe 0:0;run set1;bootm 0x2000000"

#endif//CONFIG_CHIP_MULTI
#endif

//-------------------------------------DDR_SIZE_1GB, DDR_SIZE_2GB-----------------------------/
#if defined(DDR_SIZE_1GB) || defined(DDR_SIZE_2GB)
#ifdef CONFIG_CHIP_MULTI
#define CONFIG_EXTRA_ENV_SETTINGS					 \
	"lm=sf read 0x02000000 y\0" \
	"ls=sf read 0x18100000 y\0" \
	"la=sf read 0x00000000 z\0" \
	"lmf=sf read 0x0F600000 x\0" \
	"lsf=sf read 0x3F600000 x\0" \
	"ldm=sf read 0x02000000 w\0" \
	"lds=sf read 0x18100000 w\0" \
	"ldmf=sf read 0x0F600000 w\0" \
	"ldsf=sf read 0x3F600000 w\0" \
	"lda=sf read 0x00000000 w\0" \
	"set1=run lm;run ls;run lmf;run lsf;run la;\0" \
	"set2=run ldm;run lds;run ldmf;run ldsf;run lda;\0" \
	"bootargs=mem=240M@0x01000000 gmmem=50M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock4 rootfstype=squashfs\0" \
	"cmds=initrd=0x3f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=256M@0x10000000 gmmem=190M console=ttyS0,115200 mem=512M@0x20000000\0" \
	"cmdepm=initrd=0x0f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=240M@0x01000000 gmmem=100M console=ttyS0,115200 user_debug=31\0" \
	"cmdeps=initrd=0x3f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=256M@0x10000000 gmmem=190M console=ttyS0,115200 mem=512M@0x20000000\0"

#define CONFIG_BOOTCOMMAND          "sf probe 0:0;run set1;run set2;bootm 0x2000000"

#else
#define CONFIG_EXTRA_ENV_SETTINGS					 \
	"lm=sf read 0x02000000 y\0" \
	"ls=sf read 0x18100000 y\0" \
	"la=sf read 0x00000000 z\0" \
	"lf=sf read 0x3F600000 x\0" \
	"set1=run lm;run ls;run la;run lf;\0" \
	"bootargs=mem=240M@0x01000000 gmmem=30M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock4 rootfstype=squashfs\0" \
	"cmds=initrd=0x3f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=256M@0x10000000 gmmem=190M console=ttyS0,115200 mem=512M@0x20000000\0"

#define CONFIG_BOOTCOMMAND          "sf probe 0:0;run set1;bootm 0x2000000"

#endif//CONFIG_CHIP_MULTI
#endif
//--------------------------------------------------------------------------------------------/
#define CONFIG_VIDEO_FB_BASE	    0xE000000	/* frame buffer address */
#endif

/***********************************************************
 * NAND FLASH argument
 ***********************************************************/
#ifdef CONFIG_CMD_NAND
#undef CONFIG_CMD_FLASH
#define CONFIG_SYS_NO_FLASH			1   // don't need run flash_init
#define CONFIG_NAND_GM              1
//#define CONFIG_GPIO_WP
#undef  CONFIG_CMD_IMLS

#define NAND_CHANNEL                0	/* Use CH0 in DMAC */
#define REQ_NANDTX					2
#define REQ_NANDRX					2
#define DMA_MEM2NAND	            0
#define DMA_NAND2MEM	            1

#define CONFIG_CMD_JFFS2
#define CONFIG_SYS_MAX_NAND_DEVICE	1
//#define CONFIG_SYS_NAND_MAX_CHIPS	1
#define CONFIG_SYS_NAND_BASE        CONFIG_FTNANDC023_BASE//NAND023_DATA_PORT

#define SS_SIGNATURE                "GM8210"

/* lm: load master image, ls: load slave image, la: load audio image */
/* setm for master, sets for slave, setepm for EP master, seteps for EP slave */
//-------------------------------------DDR_SIZE_512MB-------------------------------------------/
#ifdef DDR_SIZE_512MB
#define CONFIG_EXTRA_ENV_SETTINGS					 \
	"lm=nand read 0x02000000 y\0" \
	"ls=nand read 0x18100000 y\0" \
	"la=nand read 0x00000000 z\0" \
	"lf=nand read 0x1F600000 x\0" \
	"set1=run lm;run ls;run la;run lf;\0" \
	"bootargs=mem=112M@0x01000000 gmmem=80M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock4 rootfstype=squashfs\0" \
	"cmds=initrd=0x1f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=128M@0x08000000 gmmem=90M console=ttyS0,115200 mem=256M@0x10000000\0"

#define CONFIG_BOOTCOMMAND          "run set1;bootm 0x2000000"

#endif

//-------------------------------------DDR_SIZE_768MB-------------------------------------------/
#ifdef DDR_SIZE_768MB
#ifdef CONFIG_CHIP_MULTI
#define CONFIG_EXTRA_ENV_SETTINGS					 \
	"lm=nand read 0x02000000 y\0" \
	"ls=nand read 0x18100000 y\0" \
	"la=nand read 0x00000000 z\0" \
	"lmf=nand read 0x0F600000 x\0" \
	"lsf=nand read 0x2F600000 x\0" \
	"ldm=nand read 0x02000000 w\0" \
	"lds=nand read 0x18100000 w\0" \
	"ldmf=nand read 0x0F600000 w\0" \
	"ldsf=nand read 0x2F600000 w\0" \
	"lda=nand read 0x00000000 w\0" \
	"set1=run lm;run ls;run lmf;run lsf;run la;\0" \
	"set2=run ldm;run lds;run ldmf;run ldsf;run lda;\0" \
	"bootargs=mem=240M@0x01000000 gmmem=50M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock4 rootfstype=squashfs\0" \
	"cmds=initrd=0x2f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=256M@0x10000000 gmmem=190M console=ttyS0,115200 mem=256M@0x20000000\0" \
	"cmdepm=initrd=0x0f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=240M@0x01000000 gmmem=100M console=ttyS0,115200 user_debug=31\0" \
	"cmdeps=initrd=0x2f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=256M@0x10000000 gmmem=190M console=ttyS0,115200 mem=256M@0x20000000\0"

#define CONFIG_BOOTCOMMAND          "run set1;run set2;bootm 0x2000000"

#else
#define CONFIG_EXTRA_ENV_SETTINGS					 \
	"lm=nand read 0x02000000 y\0" \
	"ls=nand read 0x18100000 y\0" \
	"la=nand read 0x00000000 z\0" \
	"lf=nand read 0x2F600000 x\0" \
	"set1=run lm;run ls;run la;run lf;\0" \
	"bootargs=mem=240M@0x01000000 gmmem=50M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock4 rootfstype=squashfs\0" \
	"cmds=initrd=0x2f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=256M@0x10000000 gmmem=190M console=ttyS0,115200 mem=256M@0x20000000\0"

#define CONFIG_BOOTCOMMAND          "run set1;bootm 0x2000000"

#endif//CONFIG_CHIP_MULTI
#endif

//-------------------------------------DDR_SIZE_1GB, DDR_SIZE_2GB-----------------------------/
#if defined(DDR_SIZE_1GB) || defined(DDR_SIZE_2GB)
#ifdef CONFIG_CHIP_MULTI
#define CONFIG_EXTRA_ENV_SETTINGS					 \
	"lm=nand read 0x02000000 y\0" \
	"ls=nand read 0x18100000 y\0" \
	"la=nand read 0x00000000 z\0" \
	"lmf=nand read 0x0F600000 x\0" \
	"lsf=nand read 0x3F600000 x\0" \
	"ldm=nand read 0x02000000 w\0" \
	"lds=nand read 0x18100000 w\0" \
	"ldmf=nand read 0x0F600000 w\0" \
	"ldsf=nand read 0x3F600000 w\0" \
	"lda=nand read 0x00000000 w\0" \
	"set1=run lm;run ls;run lmf;run lsf;run la;\0" \
	"set2=run ldm;run lds;run ldmf;run ldsf;run lda;\0" \
	"bootargs=mem=240M@0x01000000 gmmem=50M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock4 rootfstype=squashfs\0" \
	"cmds=initrd=0x3f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=256M@0x10000000 gmmem=190M console=ttyS0,115200 mem=512M@0x20000000\0" \
	"cmdepm=initrd=0x0f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=240M@0x01000000 gmmem=100M console=ttyS0,115200 user_debug=31\0" \
	"cmdeps=initrd=0x3f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=256M@0x10000000 gmmem=190M console=ttyS0,115200 mem=512M@0x20000000\0"

#define CONFIG_BOOTCOMMAND          "run set1;run set2;bootm 0x2000000"

#else
#define CONFIG_EXTRA_ENV_SETTINGS					 \
	"lm=nand read 0x02000000 y\0" \
	"ls=nand read 0x18100000 y\0" \
	"la=nand read 0x00000000 z\0" \
	"lf=nand read 0x3F600000 x\0" \
	"set1=run lm;run ls;run la;run lf;\0" \
	"bootargs=mem=240M@0x01000000 gmmem=50M console=ttyS0,115200 user_debug=31 init=/squashfs_init root=/dev/mtdblock4 rootfstype=squashfs\0" \
	"cmds=initrd=0x3f600000,0x300000 ramdisk_size=10240 root=/dev/ram rw init=/init mem=256M@0x10000000 gmmem=190M console=ttyS0,115200 mem=512M@0x20000000\0"

#define CONFIG_BOOTCOMMAND          "run set1;bootm 0x2000000"

#endif//CONFIG_CHIP_MULTI
#endif
//--------------------------------------------------------------------------------------------/

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
#define CONFIG_SLI10121
#endif

/***********************************************************
 * Platform setting
 ***********************************************************/
#define CONFIG_SYS_HZ		1000	/* timer ticks per second */

#ifdef CONFIG_CMD_FPGA
#define SYS_CLK		        12000000
#else
#define SYS_CLK		        12000000
#endif

#define GPLAT_8210_ID 	    0x82102100
#define PMU_PMODE_OFFSET    0x0C
#define PMU_PLL1CR_OFFSET   0x30
#define PMU_PLL23CR_OFFSET  0x34

/***********************************************************
 * Serial console configuration
 ***********************************************************/
/* FTUART is a high speed NS 16C550A compatible UART */
#define CONFIG_BAUDRATE			    115200//38400
#define CONFIG_CONS_INDEX		    1
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_COM1		CONFIG_UART_BASE
#define CONFIG_SYS_NS16550_REG_SIZE	-4
#define CONFIG_SYS_NS16550_CLK		25000000

/***********************************************************
 * Ethernet, Autobooting
 ***********************************************************/
#define CONFIG_NET_MULTI
#define CONFIG_NET_NUM                  2
#define CONFIG_ETHPRIME	                "eth0"
#if 1
#define CONFIG_PHY_ADDR					0
#define CFG_PHY_GIGE_ID					0x0D90	/* GIGE only check this IP1001 ID */
#else
#define CONFIG_PHY_ADDR					0x10
#define CFG_PHY_GIGE_ID					0xC915	/* GIGE only check this rtl8211 ID */
#endif
#define CONFIG_PHY_GIGE					1
#define CFG_DISCOVER_PHY

#define CONFIG_CMD_MII					1
#define CONFIG_DRIVER_ETHER
#define CONFIG_NET_RETRY_COUNT          100

#define CONFIG_OVERWRITE_ETHADDR_ONCE
#define CONFIG_ETHADDR                  00:42:70:00:30:22  /* used by common/env_common.c */
#define CONFIG_ETH1ADDR                 00:42:70:00:30:24
#define CONFIG_NETMASK                  255.0.0.0
#define CONFIG_IPADDR                   10.0.1.52
#define CONFIG_SERVERIP                 10.0.1.51
#define CONFIG_GATEWAYIP                10.0.1.51

/***********************************************************
 * SD (MMC) controller
 ***********************************************************/
#ifdef CONFIG_MMC
#define CONFIG_SDHCI
#define CONFIG_FTSDC021
#define CONFIG_CMD_MMC
#define CONFIG_GENERIC_MMC
#endif

/***********************************************************
 * File system related support
 ***********************************************************/
#define CONFIG_CMD_FAT
#define CONFIG_FS_FAT
#define CONFIG_SUPPORT_VFAT
#define CONFIG_DOS_PARTITION

/***********************************************************
 * Default image file name for firmware upgrade
 ***********************************************************/
#ifdef CONFIG_AUTO_UPDATE
#define AU_ENABLE "no"  /* execute firmware auto update as system up, "yes":enable, "no":disable */

#define AU_IMG_FILE_AIO "flash.img" /* store the content of whole flash */

#ifdef CONFIG_CMD_NAND
#define AU_IMG_FILE_0   "u-boot_nand.bin"
#endif
#ifdef CONFIG_CMD_SPI
#define AU_IMG_FILE_0   "u-boot_spi.bin"
#endif
#define AU_IMG_FILE_1   "fc7500.bin"
#define AU_IMG_FILE_2   "uImage_8210"
#define AU_IMG_FILE_3   "rootfs_fa6.ramdisk.lzma"
#define AU_IMG_FILE_4   "rootfs_fa7.squashfs.img"
#define AU_IMG_FILE_5   "mtd.img"
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
    "auimg3=" AU_IMG_FILE_3 "\0" \
    "auimg4=" AU_IMG_FILE_4 "\0" \
    "auimg5=" AU_IMG_FILE_5 "\0"
#endif

/***********************************************************
 * Miscellaneous configurable options
 ***********************************************************/
#define CONFIG_SYS_LONGHELP			/* undef to save memory */
#define CONFIG_SYS_PROMPT	"GM # "	/* Monitor Command Prompt */
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
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 1024 * 1024)


/*
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	1		/* we have 1 bank of DRAM */
#define PHYS_SDRAM		    0x01000000	/* SDRAM Master start Addr */

#ifdef DDR_SIZE_512MB
#define PHYS_SDRAM_SLAVE	0x08000000	/* SDRAM Slave start Addr */
#else
#define PHYS_SDRAM_SLAVE	0x10000000	/* SDRAM Slave start Addr */
#endif

#define PHYS_EP_SDRAM_SLAVE_CMD_ADDR	0x00100000	/* SDRAM EP slave command line Addr */
#ifdef DDR_SIZE_2GB
#define PHYS_SDRAM_SIZE	0x7F000000	/* 2048 MB */
#endif
#ifdef DDR_SIZE_1GB
#define PHYS_SDRAM_SIZE	0x3F000000	/* 1024 MB */
#endif
#ifdef DDR_SIZE_512MB
#define PHYS_SDRAM_SIZE	0x1F000000	/* 512 MB */
#endif
#ifdef DDR_SIZE_256MB
#define PHYS_SDRAM_SIZE	0x0F000000	/* 256 MB */
#endif
#ifdef DDR_SIZE_768MB
#define PHYS_SDRAM_SIZE	0x2F000000	/* 768 MB */
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
