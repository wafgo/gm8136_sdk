
/*
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef FTGMAC100_H
#define FTGMAC100_H
#define NC_Body
typedef unsigned char			byte;
typedef unsigned short			word;
typedef unsigned long int 		dword;


// --------------------------------------------------------------------
//			structure for armboot
// --------------------------------------------------------------------

#define ARMBOOT
#ifdef ARMBOOT

	#define MAX_ADDR_LEN			6
	#define IFNAMSIZ				16
	#define printk					printf
	#define mdelay					udelay
	#define spin_lock_irq			
	#define kmalloc(a, b)			malloc(a)
	//#define BUG()					printf("faultal error\n"); for (;;)
	#define virt_to_phys(a)			(unsigned int)(a)

	

	struct net_device
	{
		void *priv;
		unsigned long base_addr;
		int irq;
		unsigned char dev_addr[MAX_ADDR_LEN]; /* hw address   */
		char name[IFNAMSIZ];
	
	};

	struct net_device_stats
	{
		unsigned long multicast;
	};
	
	typedef int	spinlock_t;
	
	
#endif


// --------------------------------------------------------------------
//		FTMAC100 hardware related defenition
// --------------------------------------------------------------------
#define PHYCR_REG						0x60				// PHY control register
#define PHYDATA_REG					0x64				// PHY Write Data register
#define PHY_READ_bit				(1UL<<26)				
#define PHY_WRITE_bit				(1UL<<27)	
#define PHY_RE_AUTO_bit			(1UL<<9)
#define PHY_SPEED_1G				0x2
#define PHY_SPEED_mask			0xC000


#define ISR_REG				0x00				// interrups status register
#define IER_REG				0x04				// interrupt maks register
#define MAC_MADR_REG		0x08				// MAC address (Most significant)
#define MAC_LADR_REG		0x0c				// MAC address (Least significant)

#define MAHT0_REG			0x10				// Multicast Address Hash Table 0 register
#define MAHT1_REG			0x14				// Multicast Address Hash Table 1 register
#define TXPD_REG			0x18				// Transmit Poll Demand register
#define RXPD_REG			0x1c				// Receive Poll Demand register
#define TXR_BADR_REG		0x20				// Transmit Ring Base Address register
#define RXR_BADR_REG		0x24				// Receive Ring Base Address register

#define HPTXPD_REG			0x28	//Richard
#define HPTXR_BADR_REG		0x2c	//Richard


#define ITC_REG				0x30				// interrupt timer control register
#define APTC_REG			0x34				// Automatic Polling Timer control register
#define DBLAC_REG			0x38				// DMA Burst Length and Arbitration control register

#define DMAFIFOS_REG		0x3c	//Richard
#define FEAR_REG			0x44	//Richard
#define TPAFCR_REG			0x48	//Richard
#define RBSR_REG			0x4c	//Richard, for NC Body

#define MACCR_REG			0x50				// MAC control register
#define MACSR_REG			0x54				// MAC status register
#define PHYCR_REG			0x60				// PHY control register
#define PHYWDATA_REG		0x64				// PHY Write Data register
#define FCR_REG				0x68				// Flow Control register
#define BPR_REG				0x6c				// back pressure register
#define WOLCR_REG			0x70				// Wake-On-Lan control register
#define WOLSR_REG			0x74				// Wake-On-Lan status register
#define WFCRC_REG			0x78				// Wake-up Frame CRC register
#define WFBM1_REG			0x80				// wake-up frame byte mask 1st double word register
#define WFBM2_REG			0x84				// wake-up frame byte mask 2nd double word register
#define WFBM3_REG			0x88				// wake-up frame byte mask 3rd double word register
#define WFBM4_REG			0x8c				// wake-up frame byte mask 4th double word register

#define NPTXR_PTR_REG		0x90	//Richard
#define HPTXR_PTR_REG		0x94	//Richard
#define RXR_PTR_REG			0x98	//Richard


// --------------------------------------------------------------------
//		ISR_REG 及 IMR_REG 相關內容
// --------------------------------------------------------------------
#define HPTXBUF_UNAVA_bit	(1UL<<10)
#define PHYSTS_CHG_bit		(1UL<<9)
#define AHB_ERR_bit			(1UL<<8)
#define TPKT_LOST_bit		(1UL<<7)
#define NPTXBUF_UNAVA_bit	(1UL<<6)
#define TPKT2F_bit			(1UL<<5)
#define TPKT2E_bit			(1UL<<4)
#define RPKT_LOST_bit		(1UL<<3)
#define RXBUF_UNAVA_bit		(1UL<<2)
#define RPKT2F_bit			(1UL<<1)
#define RPKT2B_bit			(1UL<<0)



// --------------------------------------------------------------------
//	APTC_REG 相關的內容
// --------------------------------------------------------------------
typedef struct
{
	u32 RXPOLL_CNT:4;
	u32 RXPOLL_TIME_SEL:1;
	u32 Reserved1:3;
	u32 TXPOLL_CNT:4;
	u32 TXPOLL_TIME_SEL:1;
	u32 Reserved2:19;
}FTGMAC100_APTCR_Status;


// --------------------------------------------------------------------
//		MACCR_REG 相關內容
// --------------------------------------------------------------------
#define SW_RST_bit			(1UL<<31)				// software reset/
#define DIRPATH_bit			(1UL<<21)	//Richard
#define RX_IPCS_FAIL_bit	(1UL<<20)	//Richard
#define SPEED_100_bit		(1UL<<19)	//Richard
#define DISCARD_CRCERR	(1UL<<18)	//Richard
#define RX_BROADPKT_bit		(1UL<<17)				// Receiving broadcast packet
#define RX_MULTIPKT_bit		(1UL<<16)				// receiving multicast packet
#define RX_HT_EN_bit		(1UL<<15)
#define RX_ALLADR_bit		(1UL<<14)				// not check incoming packet's destination address
#define JUMBO_LF_bit		(1UL<<13)	//Richard
#define RX_RUNT_bit			(1UL<<12)				// Store incoming packet even its length is les than 64 byte
#define CRC_CHK_bit			(1UL<<11)	//Richard
#define CRC_APD_bit			(1UL<<10)				// append crc to transmit packet
#define GMAC_MODE_bit		(1UL<<9)	//Richard
#define FULLDUP_bit			(1UL<<8)				// full duplex
#define ENRX_IN_HALFTX_bit	(1UL<<7)	//Richard
#define LOOP_EN_bit			(1UL<<6)				// Internal loop-back
#define HPTXR_EN_bit		(1UL<<5)	//Richard
#define REMOVE_VLAN_bit		(1UL<<4)	//Richard
//#define MDC_SEL_bit		(1UL<<13)				// set MDC as TX_CK/10
//#define RX_FTL_bit		(1UL<<11)				// Store incoming packet even its length is great than 1518 byte
#define RXMAC_EN_bit		(1UL<<3)				// receiver enable
#define TXMAC_EN_bit		(1UL<<2)				// transmitter enable
#define RXDMA_EN_bit		(1UL<<1)				// enable DMA receiving channel
#define TXDMA_EN_bit		(1UL<<0)				// enable DMA transmitting channel


// --------------------------------------------------------------------
//		Receive Ring descriptor structure
// --------------------------------------------------------------------
/*
typedef struct
{
	// RXDES0
	u32 ReceiveFrameLength:11;//0~10
	u32 Reserved1:5;          //11~15
	u32 MULTICAST:1;          //16
	u32 BROARDCAST:1;         //17
	u32 RX_ERR:1;             //18
	u32 CRC_ERR:1;            //19
	u32 FTL:1;
	u32 RUNT:1;
	u32 RX_ODD_NB:1;
	u32 Reserved2:5;
	u32 LRS:1;
	u32 FRS:1;
	u32 Reserved3:1;
	u32 RXDMA_OWN:1;			// 1 ==> owned by FTMAC100, 0 ==> owned by software
	
	// RXDES1
	u32 RXBUF_Size:11;			
	u32 Reserved:20;
	u32 EDOTR:1;
	
	// RXDES2
	u32 RXBUF_BADR;
				
	u32 VIR_RXBUF_BADR;			// not defined, 我們拿來放 receive buffer 的 virtual address

}RX_DESC;
*/



#ifdef Big_Endian
typedef struct
{
	// RXDES0
	u32 RXPKT_RDY:1;		// 1 ==> owned by FTMAC100, 0 ==> owned by software
	u32 Reserved3:1;
	u32 FRS:1;
	u32 LRS:1;
	u32 Reserved2:2;
	u32 PAUSE_FRAME:1;
	u32 PAUSE_OPCODE:1;
	u32 FIFO_FULL:1;
	u32 RX_ODD_NB:1;
	u32 RUNT:1;
	u32 FTL:1;
	u32 CRC_ERR:1;
	u32 RX_ERR:1;
	u32 BROARDCAST:1;
	u32 MULTICAST:1;
	u32 EDORR:1;
	u32 Reserved1:1;   
	u32 VDBC:14;
	
	
	// RXDES1
	u32 Reserved5:4;
	u32 IPCS_FAIL:1;
	u32 UDPCS_FAIL:1;
	u32 TCPCS_FAIL:1;
	u32 VLAN_AVA:1;	
	u32 DF:1;
	u32 LLC_PKT:1;
	u32 PROTL_TYPE:2;
	u32 Reserved4:4;
	u32 VLAN_TAGC:16;
	
	
	// RXDES2
	u32 Reserved6:32;
	
	// RXDES3
	u32 RXBUF_BADR;
				
}RX_DESC;

#else // little endian

#ifdef NC_Body
typedef struct
{
	// RXDES0
	u32 VDBC:14;//0~10
	u32 Reserved1:1;          //11~15
	u32 EDORR:1;
	u32 MULTICAST:1;          //16
	u32 BROARDCAST:1;         //17
	u32 RX_ERR:1;             //18
	u32 CRC_ERR:1;            //19
	u32 FTL:1;
	u32 RUNT:1;
	u32 RX_ODD_NB:1;
	u32 FIFO_FULL:1;
	u32 PAUSE_OPCODE:1;
	u32 PAUSE_FRAME:1;
	u32 Reserved2:2;
	u32 LRS:1;
	u32 FRS:1;
	u32 Reserved3:1;
	u32 RXPKT_RDY:1;		// 1 ==> owned by FTMAC100, 0 ==> owned by software
	
	// RXDES1
	u32 VLAN_TAGC:16;
	u32 Reserved4:4;
	u32 PROTL_TYPE:2;
	u32 LLC_PKT:1;
	u32 DF:1;
	u32 VLAN_AVA:1;	
	u32 TCPCS_FAIL:1;
	u32 UDPCS_FAIL:1;
	u32 IPCS_FAIL:1;
	u32 Reserved5:4;
	
	// RXDES2
	u32 Reserved6:32;
	
	// RXDES3
	u32 RXBUF_BADR;
				
}RX_DESC;

#else //NC WholeChip

typedef struct
{
	// RXDES0
	u32 VDBC:14;//0~10
	u32 Reserved1:2;          //11~15
	u32 MULTICAST:1;          //16
	u32 BROARDCAST:1;         //17
	u32 RX_ERR:1;             //18
	u32 CRC_ERR:1;            //19
	u32 FTL:1;
	u32 RUNT:1;
	u32 RX_ODD_NB:1;
	u32 FIFO_FULL:1;
	u32 PAUSE_OPCODE:1;
	u32 PAUSE_FRAME:1;
	u32 Reserved2:2;
	u32 LRS:1;
	u32 FRS:1;
	u32 Reserved3:1;
	u32 RXPKT_RDY:1;
	
	// RXDES1
	u32 VLAN_TAGC:16;
	u32 VLAN_AVA:1;	
	u32 TCPCS_FAIL:1;
	u32 UDPCS_FAIL:1;
	u32 IPCS_FAIL:1;
	u32 PROTL_TYPE:2;
	u32 LLC_PKT:1;
	u32 Reserved4:9;
	
	// RXDES2
	u32 RXBUF_Size:14;			
	u32 Reserved5:17;
	u32 EDORR:1;
	
	// RXDES3
	u32 RXBUF_BADR;
				
}RX_DESC;


#endif // end of #ifdef NC_Body

#endif


/*
typedef struct
{
	// TXDES0
	u32 TXPKT_LATECOL:1;
	u32 TXPKT_EXSCOL:1;
	u32 Reserved1:29;
	u32 TXDMA_OWN:1;
	
	// TXDES1
	u32 TXBUF_Size:11;
	u32 Reserved2:16;
	u32 LTS:1;
	u32 FTS:1;
	u32 TX2FIC:1;
	u32 TXIC:1;
	u32 EDOTR:1;

	// TXDES2
	u32 TXBUF_BADR;

	u32 VIR_TXBUF_BADR;			// Reserve, 我們拿來放 receive buffer 的 virtual address

}TX_DESC;
*/



#ifdef Big_Endian

typedef struct
{
	// TXDES0
	u32 TXDMA_OWN:1;
	u32 Reserved4:1;
	u32 FTS:1;
	u32 LTS:1;
	u32 Reserved3:8;
	u32 CRC_ERR:1;
	u32 Reserved2:3;
	u32 EDOTR:1;
	u32 Reserved1:1;
	u32 TXBUF_Size:14;
	
	// TXDES1
	u32 TXIC:1;	
	u32 TX2FIC:1;
	u32 Reserved6:7;
	u32 LLC_PKT:1;
	u32 Reserved5:2;
	u32 IPCS_EN:1;
	u32 UDPCS_EN:1;
	u32 TCPCS_EN:1;
	u32 INS_VLAN:1;
	u32 VLAN_TAGC:16;
	
	// TXDES2
	u32 Reserved7:32;

	// TXDES3
	u32 TXBUF_BADR;


}TX_DESC;

#else //little endian

#ifdef NC_Body

typedef struct
{
	// TXDES0
	u32 TXBUF_Size:14;
	u32 Reserved1:1;
	u32 EDOTR:1;
	u32 Reserved2:3;
	u32 CRC_ERR:1;
	u32 Reserved3:8;
	u32 LTS:1;
	u32 FTS:1;
	u32 Reserved4:1;
	u32 TXDMA_OWN:1;
	
	// TXDES1
	u32 VLAN_TAGC:16;
	u32 INS_VLAN:1;
	u32 TCPCS_EN:1;
	u32 UDPCS_EN:1;
	u32 IPCS_EN:1;
	u32 Reserved5:2;
	u32 LLC_PKT:1;
	u32 Reserved6:7;
	u32 TX2FIC:1;
	u32 TXIC:1;	
	
	// TXDES2
	u32 Reserved7:32;

	// TXDES3
	u32 TXBUF_BADR;


}TX_DESC;

#else //NC WholeChip

typedef struct
{
	// TXDES0
	//u32 TXPKT_LATECOL:1;
	//u32 TXPKT_EXSCOL:1;
	u32 TXBUF_Size:14;
	u32 Reserved1:14;
	u32 LTS:1;
	u32 FTS:1;
	u32 Reserved2:1;
	u32 TXDMA_OWN:1;
	
	// TXDES1
	u32 VLAN_TAGC:16;
	u32 INS_VLAN:1;
	u32 TCPCS_EN:1;
	u32 UDPCS_EN:1;
	u32 IPCS_EN:1;
	u32 LLC_PKT:1;
	u32 Reserved3:11;
	
	// TXDES2
	u32 Reserved4:29;
	u32 TX2FIC:1;
	u32 TXIC:1;
	u32 EDOTR:1;

	// TXDES3
	u32 TXBUF_BADR;


}TX_DESC;

#endif //#ifdef NC_Body

#endif


// waiting to do:
#define	TXPOLL_CNT			8
#define RXPOLL_CNT			0

//#define OWNBY_SOFTWARE		0
//#define OWNBY_FTMAC100		1

#define TX_OWNBY_SOFTWARE		0
#define TX_OWNBY_FTGMAC100		1


#define RX_OWNBY_SOFTWARE		1
#define RX_OWNBY_FTGMAC100		0

// --------------------------------------------------------------------
//		driver related definition
// --------------------------------------------------------------------
#define RXDES_NUM			32						// we defined 32 個 descriptor 用來接收 receive package
#define RX_BUF_SIZE			2048						// 每個 receive buffer 為 512 byte
#define TXDES_NUM			32
#define TX_BUF_SIZE			2048


/* store this information for the driver.. */
struct ftgmac100_local {

 	// these are things that the kernel wants me to keep, so users
	// can find out semi-useless statistics of how well the card is
	// performing
	struct net_device_stats stats;

	// Set to true during the auto-negotiation sequence
	int	autoneg_active;

	// Address of our PHY port
	u32		phyaddr;

	// Type of PHY
	u32		phytype;

	// Last contents of PHY Register 18
	u32		lastPhy18;
	
	spinlock_t lock;
	volatile RX_DESC *rx_descs;					// receive ring base address
	u32		rx_descs_dma;				// receive ring physical base address
	char	*rx_buf;					// receive buffer cpu address
	int		rx_buf_dma;					// receive buffer physical address
	int		rx_idx;						// 目前 receive descriptor
	
	volatile TX_DESC *tx_descs;
	u32		tx_descs_dma;
	char	*tx_buf;
	int		tx_buf_dma;
	int		tx_idx;
	
	u32     maccr_val;
};




#define FTGMAC100_STROBE_TIME			(20*HZ)



 
#endif  /* FTMAC100_H */


