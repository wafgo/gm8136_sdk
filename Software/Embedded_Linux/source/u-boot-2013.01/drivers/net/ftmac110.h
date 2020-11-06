
#ifndef FTMAC110_H
#define FTMAC110_H

typedef unsigned char			byte;
typedef unsigned short			word;
typedef unsigned long int 		dword;

// --------------------------------------------------------------------
//			structure for armboot
// --------------------------------------------------------------------

#define UBOOT

#ifdef UBOOT

	#define MAX_ADDR_LEN			6
	#define IFNAMSIZ				16
	#define printk					printf
//	#define mdelay(n)       udelay((n)*1000)
	#define mdelay(n)       do{int i;for(i=0;i<1000;i++)udelay((n));}while(0)
	#define spin_lock_irq			

	#define kmalloc(a, b)			malloc(a)
	#define NET_BUG()				printf("faultal error\n"); for (;;)
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

#define ISR_REG				0x00				// interrups status register
#define IMR_REG				0x04				// interrupt maks register
#define MAC_MADR_REG		0x08				// MAC address (Most significant)
#define MAC_LADR_REG		0x0c				// MAC address (Least significant)

#define MAHT0_REG			0x10				// Multicast Address Hash Table 0 register
#define MAHT1_REG			0x14				// Multicast Address Hash Table 1 register
#define TXPD_REG			0x18				// Transmit Poll Demand register
#define RXPD_REG			0x1c				// Receive Poll Demand register
#define TXR_BADR_REG		0x20				// Transmit Ring Base Address register
#define RXR_BADR_REG		0x24				// Receive Ring Base Address register
#define ITC_REG				0x28				// interrupt timer control register
#define APTC_REG			0x2c				// Automatic Polling Timer control register
#define DBLAC_REG			0x30				// DMA Burst Length and Arbitration control register



#define MACCR_REG			0x88				// MAC control register
#define MACSR_REG			0x8c				// MAC status register
#define PHYCR_REG			0x90				// PHY control register
#define PHYWDATA_REG		0x94				// PHY Write Data register
#define FCR_REG				0x98				// Flow Control register
#define BPR_REG				0x9c				// back pressure register
#define WOLCR_REG			0xa0				// Wake-On-Lan control register
#define WOLSR_REG			0xa4				// Wake-On-Lan status register
#define WFCRC_REG			0xa8				// Wake-up Frame CRC register
#define WFBM1_REG			0xb0				// wake-up frame byte mask 1st double word register
#define WFBM2_REG			0xb4				// wake-up frame byte mask 2nd double word register
#define WFBM3_REG			0xb8				// wake-up frame byte mask 3rd double word register
#define WFBM4_REG			0xbc				// wake-up frame byte mask 4th double word register
#define TM_REG				0xcc				// test mode register



// --------------------------------------------------------------------
//		ISR_REG 及 IMR_REG 相關內容
// --------------------------------------------------------------------
#define PHYSTS_CHG_bit		(1UL<<9)
#define AHB_ERR_bit			(1UL<<8)
#define RPKT_LOST_bit		(1UL<<7)
#define RPKT_SAV_bit		(1UL<<6)
#define XPKT_LOST_bit		(1UL<<5)
#define XPKT_OK_bit			(1UL<<4)
#define NOTXBUF_bit			(1UL<<3)
#define XPKT_FINISH_bit		(1UL<<2)
#define NORXBUF_bit			(1UL<<1)
#define RPKT_FINISH_bit		(1UL<<0)


// --------------------------------------------------------------------
//	APTC_REG 
// --------------------------------------------------------------------
typedef struct
{
	u32 RXPOLL_CNT:4;
	u32 RXPOLL_TIME_SEL:1;
	u32 Reserved1:3;
	u32 TXPOLL_CNT:4;
	u32 TXPOLL_TIME_SEL:1;
	u32 Reserved2:19;
}FTMAC110_APTCR_Status;


// --------------------------------------------------------------------
//		MACCR_REG 
// --------------------------------------------------------------------
#define SPEED_100	(1UL<<18)					// Speed mode  1:100Mbps 0: 10Mbps
#define RX_BROADPKT_bit		(1UL<<17)				// Receiving broadcast packet
#define RX_MULTIPKT_bit		(1UL<<16)				// receiving multicast packet
#define FULLDUP_bit			(1UL<<15)				// full duplex
#define CRC_APD_bit			(1UL<<14)				// append crc to transmit packet
#define MDC_SEL_bit			(1UL<<13)				// set MDC as TX_CK/10
#define RCV_ALL_bit			(1UL<<12)				// not check incoming packet's destination address
#define RX_FTL_bit			(1UL<<11)				// Store incoming packet even its length is great than 1518 byte
#define RX_RUNT_bit			(1UL<<10)				// Store incoming packet even its length is les than 64 byte
#define HT_MULTI_EN_bit		(1UL<<9)
#define RCV_EN_bit			(1UL<<8)				// receiver enable
#define XMT_EN_bit			(1UL<<5)				// transmitter enable
#define CRC_DIS_bit			(1UL<<4)
#define LOOP_EN_bit			(1UL<<3)				// Internal loop-back
#define SW_RST_bit			(1UL<<2)				// software reset/
#define RDMA_EN_bit			(1UL<<1)				// enable DMA receiving channel
#define XDMA_EN_bit			(1UL<<0)				// enable DMA transmitting channel


// --------------------------------------------------------------------
//		Receive Ring descriptor structure
// --------------------------------------------------------------------
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
	u32 RXDMA_OWN:1;			// 1 ==> owned by FTMAC110, 0 ==> owned by software
	
	// RXDES1
	u32 RXBUF_Size:11;			
	u32 Reserved:20;
	u32 EDOTR:1;
	
	// RXDES2
	u32 RXBUF_BADR;
				
	u32 VIR_RXBUF_BADR;			// not defined, 我們拿來放 receive buffer 的 virtual address

}RX_DESC;


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

	// RXDES2
	u32 TXBUF_BADR;

	u32 VIR_TXBUF_BADR;			// Reserve, 我們拿來放 receive buffer 的 virtual address

}TX_DESC;


// waiting to do:
#define	TXPOLL_CNT			8
#define RXPOLL_CNT			0

#define OWNBY_SOFTWARE		0
#define OWNBY_FTMAC110		1

// --------------------------------------------------------------------
//		driver related definition
// --------------------------------------------------------------------
#define RXDES_NUM			64      // we defined 64 descriptors to put the eceive package
#define RX_BUF_SIZE			1536	// each receive buffer is 1536 bytes
#define TXDES_NUM			16
#define TX_BUF_SIZE			1536

/* store this information for the driver.. */
struct ftmac110_local {

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
	
	int     maccr_val;
};




#define FTMAC110_STROBE_TIME			(20*HZ)



 
#endif  /* FTMAC110_H */



