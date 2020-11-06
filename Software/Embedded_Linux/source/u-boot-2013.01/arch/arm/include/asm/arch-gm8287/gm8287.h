/*
 * (C) Copyright 2013 Faraday Technology
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

#ifndef __GM8210_H
#define __GM8210_H

/*
 * Hardware register bases
 */
#define CONFIG_AHBC01_BASE      0x92B00000
//#define CONFIG_AHBC04_BASE      0x94A00000
//#define CONFIG_AHBC05_BASE      0x90500000

//#define CONFIG_FTSMC020_BASE    0x92100000
#define CONFIG_FTGMAC100_BASE   0x92500000
#define CONFIG_FTGMAC100_1_BASE 0x92500000//only one IP
#define CONFIG_DMAC_BASE        0x92600000
#define CONFIG_FTNANDC023_BASE  0x80000000
#define NAND023_DATA_PORT       0x80100000
#define FTNANDC023_BASE         CONFIG_FTNANDC023_BASE

#define CONFIG_SSP0_BASE        0x82000000
#define CONFIG_SPI020_BASE		0x80300000

#define CONFIG_UART_BASE        0x98300000

#define CONFIG_PMU_BASE         0x99000000
#define CONFIG_TIMER_BASE       0x99100000
#define CONFIG_WDT_BASE         0x99200000
#define CONFIG_INTC_BASE        0x96000000
#define CONFIG_GPIO_BASE        0x99400000
#define CONFIG_GPIO0_BASE       CONFIG_GPIO_BASE
#define CONFIG_GPIO1_BASE       0x98E00000
#define CONFIG_I2C_BASE		    0x99600000

#define CONFIG_TMR_BASE         0x99100000
#define CONFIG_LCDC_BASE      	0x9A800000
#define CONFIG_LCDC2_BASE	0x9B800000
#define CONFIG_FTTVE100_BASE	0x93400000

//#define CONFIG_FTHDMI_0_BASE    0xA1900000
#define CONFIG_MCP100_BASE      0x92000000
#define CONFIG_FOTG210_0_BASE   0x93000000
#define CONFIG_FOTG210_1_BASE   0x93100000

#define BIT0            0x00000001
#define BIT1            0x00000002
#define BIT2            0x00000004
#define BIT3            0x00000008
#define BIT4            0x00000010
#define BIT5            0x00000020
#define BIT6            0x00000040
#define BIT7            0x00000080
#define BIT8            0x00000100
#define BIT9            0x00000200
#define BIT10           0x00000400
#define BIT11           0x00000800
#define BIT12           0x00001000
#define BIT13           0x00002000
#define BIT14           0x00004000
#define BIT15           0x00008000
#define BIT16           0x00010000
#define BIT17           0x00020000
#define BIT18           0x00040000
#define BIT19           0x00080000
#define BIT20           0x00100000
#define BIT21           0x00200000
#define BIT22           0x00400000
#define BIT23           0x00800000
#define BIT24           0x01000000
#define BIT25           0x02000000
#define BIT26           0x04000000
#define BIT27           0x08000000
#define BIT28           0x10000000
#define BIT29           0x20000000
#define BIT30           0x40000000
#define BIT31           0x80000000

#endif	/* __GM8126_H */
