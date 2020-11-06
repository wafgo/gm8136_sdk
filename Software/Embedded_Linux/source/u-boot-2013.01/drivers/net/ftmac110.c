/* ftgmac.c: Faraday 10/100/1000 Ethernet device driver for Linux. */
/*
	Written 2005 by Fred.Chien.

	Copyright (C) 2005 faraday Technology Inc.

	This software may be used and distributed
	according to the terms of the GNU General Public License,
	incorporated herein by reference.

	The author may be reached at fred@faraday-tech.com
	or No.5,Li-Shin Rd.III Hsinchu City,Taiwan,300,ROC.

	

	Versions:
	1.0 Support read write pointer in register instead of polling the own bit in memory ---fred.chien

	

*/

static const char version[] =
	"Faraday FTMAC110 Driver, (uboot 3.4) 07/03/07 - by Faraday\n";
#include <common.h>
#include <malloc.h>
#include <net.h>
#include "ftmac110.h"

#ifdef CONFIG_FTMAC110
#define inl(addr) 			(*((volatile u32 *)(addr)))
#define inw(addr)			(*((volatile u16 *)(addr)))
#define outl(value, addr)  	(*((volatile u32 *)(addr)) = value)
#define outb(value, addr)	(*((volatile u8 *)(addr)) = value)
int tx_rx_cnt = 0;

/*------------------------------------------------------------------------
 .
 . Configuration options, for the experienced user to change.
 .
 -------------------------------------------------------------------------*/

/*
 . DEBUGGING LEVELS
 .
 . 0 for normal operation
 . 1 for slightly more details
 . >2 for various levels of increasingly useless information
 .    2 for interrupt tracking, status flags
 .    3 for packet info
 .    4 for complete packet dumps
*/

#define DO_PRINT(args...) printk(args)

//#define FTMAC110_DEBUG  5  // Must be defined in makefile

#if FTMAC110_DEBUG > 2 
#define PRINTK3(args...) DO_PRINT(args)
#else
#define PRINTK3(args...)
#endif

#if FTMAC110_DEBUG > 1
#define PRINTK2(args...) DO_PRINT(args)
#else
#define PRINTK2(args...)
#endif

#ifdef FTMAC110_DEBUG
#define PRINTK(args...) DO_PRINT(args)
#else
#define PRINTK(args...)
#endif


/*------------------------------------------------------------------------
 .
 . The internal workings of the driver.  If you are changing anything
 . here with the SMC stuff, you should have the datasheet and know
 . what you are doing.
 .
 -------------------------------------------------------------------------*/
#define CARDNAME "FTMAC110"

#ifdef FTMAC110_TIMER
	static struct timer_list ftmac110_timer;
#endif

#define ETH_ZLEN 60

/*-----------------------------------------------------------------
 .
 .  The driver can be entered at any of the following entry points.
 .
 .------------------------------------------------------------------  */


/*
 . This is called by  register_netdev().  It is responsible for
 . checking the portlist for the FTGMAC200 series chipset.  If it finds
 . one, then it will initialize the device, find the hardware information,
 . and sets up the appropriate device parameters.
 . NOTE: Interrupts are *OFF* when this procedure is called.
 .
 . NB:This shouldn't be static since it is referred to externally.
*/
int ftmac110_init(struct eth_device *dev,bd_t *bd);

/*
 . This is called by  unregister_netdev().  It is responsible for
 . cleaning up before the driver is finally unregistered and discarded.
*/
void ftmac110_destructor(struct eth_device *dev);

/*
 . The kernel calls this function when someone wants to use the net_device,
 . typically 'ifconfig ethX up'.
*/
static int ftmac110_open(struct eth_device *dev);


/*
 . This is a separate procedure to handle the receipt of a packet, to
 . leave the interrupt code looking slightly cleaner
*/
inline static int ftmac110_rcv( struct eth_device *dev );

/*
 . This is called by the kernel in response to 'ifconfig ethX down'.  It
 . is responsible for cleaning up everything that the open routine
 . does, and maybe putting the card into a powerdown state.
*/
static void ftmac110_close(struct eth_device *dev);

/*
 ------------------------------------------------------------
 .
 . Internal routines
 .
 ------------------------------------------------------------
*/


/*
 . A rather simple routine to print out a packet for debugging purposes.
*/
#if FTMAC110_DEBUG > 2
static void print_packet( byte *, int );
#endif


/* this does a soft reset on the device */
static void ftmac110_reset( struct eth_device* dev );

/* Enable Interrupts, Receive, and Transmit */
static void ftmac110_enable( struct eth_device *dev );

/* this puts the device in an inactive state */
static void ftmac110_shutdown( unsigned int ioaddr );

int initialized = 0;


/* Routines to Read and Write the PHY Registers across the
   MII Management Interface
*/
void put_mac(int base, char *mac_addr)
{
	int val;
	
	val = ((u32)mac_addr[0])<<8 | (u32)mac_addr[1];
	outl(val, base);
	val = ((((u32)mac_addr[2])<<24)&0xff000000) |
		  ((((u32)mac_addr[3])<<16)&0xff0000) |
		  ((((u32)mac_addr[4])<<8)&0xff00)  |
		  ((((u32)mac_addr[5])<<0)&0xff);
	outl(val, base+4);
}

void get_mac(int base, char *mac_addr)
{
	int val;
	val = inl(base);
	mac_addr[0] = (val>>8)&0xff;
	mac_addr[1] = val&0xff;
	val = inl(base+4);
	mac_addr[2] = (val>>24)&0xff;
	mac_addr[3] = (val>>16)&0xff;
	mac_addr[4] = (val>>8)&0xff;
	mac_addr[5] = val&0xff;
}

// --------------------------------------------------------------------
// 	Print the Ethernet address
// --------------------------------------------------------------------
void print_mac(char *mac_addr)
{
	int i;
	 
	DO_PRINT("ADDR: ");
	for (i = 0; i < 5; i++)
	{
		DO_PRINT("%2.2x:", mac_addr[i] );
	}
	DO_PRINT("%2.2x \n", mac_addr[5] );
}

/*
 * Reads a register from the MII Management serial interface
 */
u16 ftmac110_read_phy_register(unsigned char phyaddr, unsigned char phyreg)
{
    u32 cvalue;
    u32 tmp;
	cvalue = inl(CONFIG_FTMAC110_BASE + PHYCR_REG);
    cvalue &= 0x3000FFFF;
    cvalue |= (phyaddr << 16);      //PHY address
    cvalue |= (phyreg << 21);
    cvalue |= 0x04000000;           //MIIRD
    
    outl(cvalue, CONFIG_FTMAC110_BASE + PHYCR_REG);
    do {
	mdelay(10);
	tmp = inl(CONFIG_FTMAC110_BASE + PHYCR_REG);
    }while(tmp & 0x04000000);

    return (u16)(tmp&0xffff);
}

#if 0
/*
 * Writes a register to the MII Management serial interface
 */
static void ftmac110_write_phy_register(unsigned char phyaddr, unsigned char phyreg, u16 phydata)
{
    u32 cvalue;
    u32 tmp;
	cvalue = inl(CONFIG_FTMAC110_BASE + PHYCR_REG);
    cvalue &= 0x3000FFFF;
    cvalue |= (phyaddr << 16);      //PHY address
    cvalue |= (phyreg << 21);
    cvalue |= 0x08000000;           //MIIWR
    outl(phydata, CONFIG_FTMAC110_BASE + PHYWDATA_REG);
    outl(cvalue, CONFIG_FTMAC110_BASE + PHYCR_REG);
    do {
	mdelay(10);
	tmp = inl(CONFIG_FTMAC110_BASE + PHYCR_REG);
    }while(tmp & 0x08000000);
}
#endif

#ifndef INTERNAL_PHY
static int ftmac110_phy_reset(void)
{
    return 0;
}
#endif
/*
 . Function: ftgmac100_reset( struct eth_device* dev )
 . Purpose:
 .  	This sets the SMC91111 chip to its normal state, hopefully from whatever
 . 	mess that any other DOS driver has put it in.
 .
 . Maybe I should reset more registers to defaults in here?  SOFTRST  should
 . do that for me.
 .
 . Method:
 .	1.  send a SOFT RESET
 .	2.  wait for it to finish
 .	3.  enable autorelease mode
 .	4.  reset the memory management unit
 .	5.  clear all interrupts
 .
*/
static void ftmac110_reset( struct eth_device* dev )
{
      //struct ftmac110_local *lp 	= (struct ftmac100_local *)dev->priv;
	unsigned int	ioaddr = dev->iobase;
#ifdef INTERNAL_PHY
#else
	mdelay(5);
	value = ftmac110_read_phy_register(0, 1);
	while((value & 0x20)==0x00)
	{ //Auto-negotiation process not completed => SW Reset
		printk("Reset PHY\n");
		ftmac110_phy_reset(); //DAVACOM ethernet PHY
		value = ftmac110_read_phy_register(0, 1);
		todo_phy_reset=1;
		if ((value & 0x20)==0x00)
		mdelay(2000);
	};

	value=0;
	value=ftmac110_read_phy_register(0, 1);
#ifdef FTMAC_DEBUG
	printf("0x01=%x\n",value);
	if(value &0x0020)
		printf("Auto-negotiation process completed\n");
	else
		printf("Auto-negotiation process NOT completed\n");
#endif
		
	
	PRINTK2("%s:ftmac110_reset\n", dev->name);
	if (todo_phy_reset)
		mdelay(4000);
#endif
	outl( SW_RST_bit, ioaddr + MACCR_REG );

#ifdef not_complete_yet
	/* Setup for fast accesses if requested */
	/* If the card/system can't handle it then there will */
	/* be no recovery except for a hard reset or power cycle */
	if (dev->dma)
	{
		outw( inw( ioaddr + CONFIG_REG ) | CONFIG_NO_WAIT,	ioaddr + CONFIG_REG );
	}
#endif /* end_of_not */

	/* this should pause enough for the chip to be happy */
	for (; (inl( ioaddr + MACCR_REG ) & SW_RST_bit) != 0; )
	{
		mdelay(10);
		PRINTK3("RESET: reset not complete yet\n" );
	}

	outl( 0, ioaddr + IMR_REG );			/* Disable all interrupts */
}


/*
 . Function: ftgmac100_enable
 . Purpose: let the chip talk to the outside work
 . Method:
 .	1.  Enable the transmitter
 .	2.  Enable the receiver
 .	3.  Enable interrupts
*/
static void ftmac110_enable( struct eth_device *dev )
{

       unsigned int ioaddr 	= dev->iobase;
	int i;
	struct ftmac110_local *lp 	= (struct ftmac110_local *)dev->priv;

	PRINTK2("%s:ftmac110_enable\n", dev->name);

	for (i=0; i<RXDES_NUM; ++i)
	{
		lp->rx_descs[i].RXDMA_OWN = OWNBY_FTMAC110;				// owned by FTMAC100
	}
	lp->rx_idx = 0;

	for (i=0; i<TXDES_NUM; ++i)
	{
		lp->tx_descs[i].TXDMA_OWN = OWNBY_SOFTWARE;			// owned by software
	}
	lp->tx_idx = 0;

	/* set the MAC address */
	put_mac(ioaddr + MAC_MADR_REG, (char *)dev->enetaddr);

	outl( lp->rx_descs_dma, ioaddr + RXR_BADR_REG);
	outl( lp->tx_descs_dma, ioaddr + TXR_BADR_REG);
	outl( 0x00001010, ioaddr + ITC_REG);					// 此值為 document 所建議
	///outl( 0x0, ioaddr + ITC_REG);
	///outl( (1UL<<TXPOLL_CNT)|(1UL<<RXPOLL_CNT), ioaddr + APTC_REG);
	outl( (0UL<<TXPOLL_CNT)|(0x1<<RXPOLL_CNT), ioaddr + APTC_REG);
	outl( 0x390, ioaddr + DBLAC_REG );	
//outl( 0x1df, ioaddr + DBLAC_REG );						// 此值為 document 所建議
	outl( inl(FCR_REG)|0x1, ioaddr + FCR_REG );				// enable flow control
	outl( inl(BPR_REG)|0x1, ioaddr + BPR_REG );				// enable back pressure register

	/* now, enable interrupts */
	outl(
			PHYSTS_CHG_bit	|
			AHB_ERR_bit		|
///			RPKT_LOST_bit	|
///			RPKT_SAV_bit	|
///			XPKT_LOST_bit	|
///			XPKT_OK_bit		|
///			NOTXBUF_bit		|
///			XPKT_FINISH_bit	|
///			NORXBUF_bit		|
			RPKT_FINISH_bit
        	,ioaddr + IMR_REG
        );
	/// enable trans/recv,...
	outl(lp->maccr_val, ioaddr + MACCR_REG );

#ifdef FTMAC110_TIMER
	/// waiting to do: 兩個以上的網路卡
	init_timer(&ftmac110_timer);
	ftmac110_timer.function = ftmac110_timer_func;
	ftmac110_timer.data = (unsigned long)dev;
	mod_timer(&ftmac110_timer, jiffies + FTMAC110_STROBE_TIME);
#endif
}

/*
 . Function: ftmac110_shutdown
 . Purpose:  closes down the SMC91xxx chip.
 . Method:
 .	1. zero the interrupt mask
 .	2. clear the enable receive flag
 .	3. clear the enable xmit flags
 .
 . TODO:
 .   (1) maybe utilize power down mode.
 .	Why not yet?  Because while the chip will go into power down mode,
 .	the manual says that it will wake up in response to any I/O requests
 .	in the register space.   Empirical results do not show this working.
*/
static void ftmac110_shutdown( unsigned int ioaddr )
{
		/// 設定 interrupt mask register
	outl( 0, ioaddr + IMR_REG );

	/// enable trans/recv,...
	outl( 0, ioaddr + MACCR_REG );
}

static int ftmac110_send_packet( struct eth_device *dev,void *packet, int length )
{


	struct ftmac110_local *lp 	= (struct ftmac110_local *)dev->priv;
	unsigned int ioaddr 	= dev->iobase;
	volatile TX_DESC *cur_desc;


	PRINTK3("%s:ftmac110_wait_to_send_packet\n", dev->name);
	cur_desc = &lp->tx_descs[lp->tx_idx];
	for (; cur_desc->TXDMA_OWN != OWNBY_SOFTWARE; )		/// 沒有空的 transmit descriptor 可以使用
	{
		DO_PRINT("Transmitting busy\n");
		udelay(10);
   	}
	length = ETH_ZLEN < length ? length : ETH_ZLEN;
	length = length > TX_BUF_SIZE ? TX_BUF_SIZE : length;

#if FTMAC110_DEBUG > 2
	DO_PRINT("Transmitting Packet\n");
	print_packet( packet, length );
#endif
//Far @ add
	memcpy((char *)cur_desc->VIR_TXBUF_BADR, packet, length);		/// waiting to do: 將 data 切成許多 segment
	cur_desc->TXBUF_Size = length;
	cur_desc->LTS = 1;
	cur_desc->FTS = 1;
	cur_desc->TX2FIC = 0;
	cur_desc->TXIC = 0;
	cur_desc->TXDMA_OWN = OWNBY_FTMAC110;
	outl( 0xffffffff, ioaddr + TXPD_REG);
	lp->tx_idx = (lp->tx_idx + 1) % TXDES_NUM;

	return length;
}


/*-------------------------------------------------------------------------
 |
 | ftgmac100_destructor( struct eth_device * dev )
 |   Input parameters:
 |	dev, pointer to the device structure
 |
 |   Output:
 |	None.
 |
 ---------------------------------------------------------------------------
*/
void ftmac110_destructor(struct eth_device *dev)
{
	PRINTK3("%s:ftmac110_destructor\n", dev->name);
}


/*
 * Open and Initialize the board
 *
 * Set up everything, reset the card, etc ..
 *
 */
static int ftmac110_open(struct eth_device *dev)
{
	

	PRINTK2("%s:ftmac110_open on ioaddr is %d\n", dev->name, dev->iobase);

	/* reset the hardware */
	ftmac110_reset( dev );
	ftmac110_enable( dev );

	/* set the MAC address */
	put_mac(dev->iobase + MAC_MADR_REG, (char *)dev->enetaddr);

	return 0;

	
}


#ifdef USE_32_BIT
void
insl32(r,b,l) 	
{	
   int __i ;  
   dword *__b2;  

	__b2 = (dword *) b;  
	for (__i = 0; __i < l; __i++) 
   {  
		  *(__b2 + __i) = *(dword *)(r+0x10000300);  
	}  
}
#endif
 

/*-------------------------------------------------------------
 .
 . ftgmac100_rcv -  receive a packet from the card
 .
 . There is ( at least ) a packet waiting to be read from
 . chip-memory.
 .
 . o Read the status
 . o If an error, record it
 . o otherwise, read in the packet
 --------------------------------------------------------------
*/


static int ftmac110_rcv(struct eth_device *dev)
{



struct ftmac110_local *lp = (struct ftmac110_local *)dev->priv;
	int 	packet_length;
	volatile RX_DESC *cur_desc;
	int 	cpy_length;
	int		start_idx;
	int		seg_length;
	int 	rcv_cnt;

	///PRINTK3("%s:ftmac110_rcv\n", dev->name);
	for (rcv_cnt=0; rcv_cnt<1; ++rcv_cnt)
	{
		packet_length = 0;
		start_idx= lp->rx_idx;

		for (; (cur_desc = &lp->rx_descs[lp->rx_idx])->RXDMA_OWN==0; )
		{
			lp->rx_idx = (lp->rx_idx+1)%RXDES_NUM;
			if (cur_desc->FRS)
			{
				if (cur_desc->RX_ERR || cur_desc->CRC_ERR || cur_desc->FTL || cur_desc->RUNT || cur_desc->RX_ODD_NB)
				{
					cur_desc->RXDMA_OWN = 1;
					return 0;
				}
				packet_length = cur_desc->ReceiveFrameLength;				// normal frame
			}
			if ( cur_desc->LRS )		// packet's last frame
			{
				break;
			}
		}
		
		if (packet_length>0)			
		{

			byte* data = (byte *) NetRxPackets[0];
			byte* pkts = (byte *) 0;
			cpy_length = 0;
			for (; start_idx!=lp->rx_idx; start_idx=(start_idx+1)%RXDES_NUM)
			{
				seg_length = min(packet_length - cpy_length, RX_BUF_SIZE);
// Alex @ modify - 2 bytes aligned only at first desc 
                            pkts = (byte *) lp->rx_descs[start_idx].VIR_RXBUF_BADR;

				if (!cpy_length)
				{
				    pkts += 2;
				    if (seg_length > (RX_BUF_SIZE-2)) seg_length = RX_BUF_SIZE - 2;
				}
				memcpy(data+cpy_length, pkts, seg_length);
// end
				// Far @ modify - Rx_buffer 2-byte aligm
				//memcpy(data+cpy_length, (char *)lp->rx_descs[start_idx].VIR_RXBUF_BADR+2, seg_length);
				
				cpy_length += seg_length;
				lp->rx_descs[start_idx].RXDMA_OWN = 1;			/// 此 frame 已處理完畢, 還給 hardware
			}
			NetReceive(NetRxPackets[0], packet_length);
#if	FTMAC110_DEBUG > 4
			DO_PRINT("Receiving Packet\n");
			print_packet( data, packet_length );
#endif
			return packet_length;
		}
	}
	return 0;


	
}

/*----------------------------------------------------
 . ftgmac100_close
 .
 . this makes the board clean up everything that it can
 . and not talk to the outside world.   Caused by
 . an 'ifconfig ethX down'
 .
 -----------------------------------------------------*/
static void ftmac110_close(struct eth_device *dev)
{
	



	PRINTK2("%s:ftmac110_close\n", dev->name);


	/* clear everything */
	ftmac110_shutdown( dev->iobase );
}




#if FTMAC110_DEBUG > 2
static void print_packet( byte * buf, int length )
{

#if FTMAC110_DEBUG > 2
	int i;
	int remainder;
	int lines;
#endif

#if FTMAC110_DEBUG > 2
	lines = length / 16;
	remainder = length % 16;

	for ( i = 0; i < lines ; i ++ ) {
		int cur;

		for ( cur = 0; cur < 8; cur ++ ) {
			byte a, b;

			a = *(buf ++ );
			b = *(buf ++ );
			DO_PRINT("%02x%02x ", a, b );
		}
		DO_PRINT("\n");
	}
	for ( i = 0; i < remainder/2 ; i++ ) {
		byte a, b;

		a = *(buf ++ );
		b = *(buf ++ );
		DO_PRINT("%02x%02x ", a, b );
	}
	DO_PRINT("\n");
#endif

}
#endif

void ftmac110_ringbuf_alloc(struct ftmac110_local *lp)
{
	int i;

	lp->rx_descs = kmalloc( sizeof(RX_DESC)*(RXDES_NUM+1), GFP_DMA|GFP_KERNEL );
	if (lp->rx_descs == NULL)
	{
		DO_PRINT("Receive Ring Buffer allocation error\n");
		NET_BUG();
	}
	lp->rx_descs =  (RX_DESC *)((int)(((char *)lp->rx_descs)+sizeof(RX_DESC)-1)&0xfffffff0);
	lp->rx_descs_dma = virt_to_phys(lp->rx_descs);
	memset((unsigned *)lp->rx_descs, 0, sizeof(RX_DESC)*RXDES_NUM);


	lp->rx_buf = kmalloc( RX_BUF_SIZE*RXDES_NUM, GFP_DMA|GFP_KERNEL );
	if (lp->rx_buf == NULL || (( (u32)lp->rx_buf % 4)!=0))
	{
		DO_PRINT("Receive Ring Buffer allocation error, lp->rx_buf = %x\n",(unsigned int) lp->rx_buf);
		NET_BUG();
	}
	lp->rx_buf_dma = virt_to_phys(lp->rx_buf);


	for (i=0; i<RXDES_NUM; ++i)
	{
		lp->rx_descs[i].RXBUF_Size = RX_BUF_SIZE;
		lp->rx_descs[i].EDOTR = 0;							// not last descriptor
		lp->rx_descs[i].RXBUF_BADR = lp->rx_buf_dma+RX_BUF_SIZE*i;
		lp->rx_descs[i].VIR_RXBUF_BADR = virt_to_phys( lp->rx_descs[i].RXBUF_BADR );
	}
	lp->rx_descs[RXDES_NUM-1].EDOTR = 1;					// is last descriptor


	lp->tx_descs = kmalloc( sizeof(TX_DESC)*(TXDES_NUM+1), GFP_DMA|GFP_KERNEL );
	if (lp->tx_descs == NULL)
	{
		DO_PRINT("Transmit Ring Buffer allocation error\n");
		NET_BUG();
	}
	lp->tx_descs =  (TX_DESC *)((int)(((char *)lp->tx_descs)+sizeof(TX_DESC)-1)&0xfffffff0);
	lp->tx_descs_dma = virt_to_phys(lp->tx_descs);
	memset((unsigned *)lp->tx_descs, 0, sizeof(TX_DESC)*TXDES_NUM);

	lp->tx_buf = kmalloc( TX_BUF_SIZE*TXDES_NUM, GFP_DMA|GFP_KERNEL );
	if (lp->tx_buf == NULL || (( (u32)lp->tx_buf % 4)!=0))
	{
		DO_PRINT("Transmit Ring Buffer allocation error\n");
		NET_BUG();
	}
	lp->tx_buf_dma = virt_to_phys(lp->tx_buf);

	for (i=0; i<TXDES_NUM; ++i)
	{
		lp->tx_descs[i].EDOTR = 0;							// not last descriptor
		lp->tx_descs[i].TXBUF_BADR = lp->tx_buf_dma+TX_BUF_SIZE*i;
		lp->tx_descs[i].VIR_TXBUF_BADR = virt_to_phys( lp->tx_descs[i].TXBUF_BADR );
	}
	lp->tx_descs[TXDES_NUM-1].EDOTR = 1;					// is last descriptor
	PRINTK3("lp->rx_descs = %x, lp->rx_rx_descs_dma = %x\n", lp->rx_descs, lp->rx_descs_dma);
	PRINTK3("lp->rx_buf = %x, lp->rx_buf_dma = %x\n", lp->rx_buf, lp->rx_buf_dma);
	PRINTK3("lp->tx_descs = %x, lp->tx_rx_descs_dma = %x\n", lp->tx_descs, lp->tx_descs_dma);
	PRINTK3("lp->tx_buf = %x, lp->tx_buf_dma = %x\n", lp->tx_buf, lp->tx_buf_dma);


}


int ftmac110_initialize(bd_t *bis)
{
	
		
	int card_number = 0;
	struct eth_device *dev;

	dev = (struct eth_device *)malloc(sizeof *dev);
	sprintf (dev->name, "FTMAC110#%d", card_number);
	dev->iobase = CONFIG_FTMAC110_BASE;
	dev->init = ftmac110_init;
	dev->halt = ftmac110_close;
	dev->send = ftmac110_send_packet;
	dev->recv = ftmac110_rcv;

       initialized = 0;
	   
	eth_register (dev);
	card_number++;

    
	return card_number;


	
}


static char ftmac110_mac_addr[] = {0x00, 0x41, 0x71, 0x81, 0x26, 0x22}; 


int ftmac110_init(struct eth_device *dev, bd_t *bis) 
{
	struct ftmac110_local *lp;
	int i;

	//ahb_init();
	if (initialized == 0)
	{
		initialized = 1;

		dev->iobase = CONFIG_FTMAC110_BASE;
		
		/* Initialize the private structure. */
		dev->priv = (void *)malloc(sizeof(struct ftmac110_local));
		if (dev->priv == NULL)
		{
			DO_PRINT("out of memory\n");
			return 0;
		}


		/// --------------------------------------------------------------------
		///		 ftmac110_local
		/// --------------------------------------------------------------------
		memset(dev->priv, 0, sizeof(struct ftmac110_local));
		strcpy(dev->name, "eth0");
		lp = (struct ftmac110_local *)dev->priv;
		lp->maccr_val = CRC_APD_bit | RCV_EN_bit | XMT_EN_bit | RDMA_EN_bit | XDMA_EN_bit | SPEED_100 | FULLDUP_bit;
		ftmac110_ringbuf_alloc(lp);
		for (i=0; i<6; ++i)
    	      {
    		  dev->enetaddr[i] = ftmac110_mac_addr[i];
    	      }
	}
	ftmac110_open(dev);

	return 0;

	
}


#endif //CONFIG_FTMAC110
