
/*
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
#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <net.h>
#include <miiphy.h>
#include "ftgmac100.h"

#ifdef CONFIG_FTGMAC100

static const char version[] = "Faraday FTGMAC100 Driver\n";

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

//#define FTGMAC100_DEBUG 1 // Must be defined in makefile

#if (FTGMAC100_DEBUG > 2 )
#define PRINTK3(args...) DO_PRINT(args)
#else
#define PRINTK3(args...)
#endif

#if FTGMAC100_DEBUG > 1
#define PRINTK2(args...) DO_PRINT(args)
#else
#define PRINTK2(args...)
#endif

#ifdef FTGMAC100_DEBUG
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
#define CARDNAME "FTGMAC100"

#ifdef FTGMAC100_TIMER
static struct timer_list ftgmac100_timer;
#endif

#define ETH_ZLEN 60

#ifdef  CONFIG_SMC_USE_32_BIT
#define USE_32_BIT
#else
#undef USE_32_BIT
#endif

//#define IP101G_LED_MODE_2
#define ICPLUS_175D_PHYID 0x02430d80
#define MDC_CYCTHR 0xF0
/*-----------------------------------------------------------------
 .
 .  The driver can be entered at any of the following entry points.
 .
 .------------------------------------------------------------------  */

extern int eth_init(bd_t * bd);
extern void eth_halt(void);
extern int eth_rx(void);

static int ip175d_config_init(struct eth_device *dev);

int initialized = 0;

/*
 . This is called by  register_netdev().  It is responsible for
 . checking the portlist for the FTMAC100 series chipset.  If it finds
 . one, then it will initialize the device, find the hardware information,
 . and sets up the appropriate device parameters.
 . NOTE: Interrupts are *OFF* when this procedure is called.
 .
 . NB:This shouldn't be static since it is referred to externally.
*/
int ftgmac100_init(struct eth_device *dev, bd_t * bd);

/*
 . This is called by  unregister_netdev().  It is responsible for
 . cleaning up before the driver is finally unregistered and discarded.
*/
void ftgmac100_destructor(struct eth_device *dev);

/*
 . The kernel calls this function when someone wants to use the eth_device,
 . typically 'ifconfig ethX up'.
*/
static int ftgmac100_open(struct eth_device *dev);

/*
 . This is called by the kernel in response to 'ifconfig ethX down'.  It
 . is responsible for cleaning up everything that the open routine
 . does, and maybe putting the card into a powerdown state.
*/
static void ftgmac100_close(struct eth_device *dev);

/*
 . This is a separate procedure to handle the receipt of a packet, to
 . leave the interrupt code looking slightly cleaner
*/
static int ftgmac100_rcv(struct eth_device *dev);

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
#if FTGMAC100_DEBUG > 2
static void print_packet(byte *, int);
#endif

/* this does a soft reset on the device */
static void ftgmac100_reset(struct eth_device *dev);

/* Enable Interrupts, Receive, and Transmit */
static void ftgmac100_enable(struct eth_device *dev);

/* this puts the device in an inactive state */
static void ftgmac100_shutdown(unsigned int ioaddr);

/* Routines to Read and Write the PHY Registers across the
   MII Management Interface
*/

void put_mac(int base, char *mac_addr)
{
    int val;

    val = ((u32) mac_addr[0]) << 8 | (u32) mac_addr[1];
    writel(val, base);
    val = ((((u32) mac_addr[2]) << 24) & 0xff000000) |
        ((((u32) mac_addr[3]) << 16) & 0xff0000) |
        ((((u32) mac_addr[4]) << 8) & 0xff00) | ((((u32) mac_addr[5]) << 0) & 0xff);
    writel(val, base + 4);

}

void get_mac(int base, char *mac_addr)
{
    int val;
    val = readl(base);
    mac_addr[0] = (val >> 8) & 0xff;
    mac_addr[1] = val & 0xff;
    val = readl(base);
    mac_addr[2] = (val >> 24) & 0xff;
    mac_addr[3] = (val >> 16) & 0xff;
    mac_addr[4] = (val >> 8) & 0xff;
    mac_addr[5] = val & 0xff;
}

// --------------------------------------------------------------------
//      Print the Ethernet address
// --------------------------------------------------------------------
void print_mac(char *mac_addr)
{
    int i;

    DO_PRINT("ADDR: ");
    for (i = 0; i < 5; i++) {
        DO_PRINT("%2.2x:", mac_addr[i]);
    }
    DO_PRINT("%2.2x \n", mac_addr[5]);
}

static void set_MDC_CLK(struct eth_device *dev)
{
#ifndef CONFIG_PLATFORM_GM8210
    int tmp;
    
    tmp = readl(dev->iobase + PHYCR_REG);    
    tmp &= 0xFFFFFF00;
    tmp |= (MDC_CYCTHR & 0xFF);
    writel(tmp, dev->iobase + PHYCR_REG);
#endif
}
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
static void ftgmac100_reset(struct eth_device *dev)
{
    //struct ftgmac100_local *lp    = (struct ftgmac100_local *)dev->priv;
    unsigned int tmp;
    unsigned int ioaddr = dev->iobase;

    PRINTK2("%s:ftgmac100_reset\n", dev->name);

    writel(SW_RST_bit, ioaddr + MACCR_REG);

#ifdef not_complete_yet
    /* Setup for fast accesses if requested */
    /* If the card/system can't handle it then there will */
    /* be no recovery except for a hard reset or power cycle */
    if (dev->dma) {
        outw(readw(ioaddr + CONFIG_REG) | CONFIG_NO_WAIT, ioaddr + CONFIG_REG);
    }
#endif /* end_of_not */

    /* this should pause enough for the chip to be happy */
    for (; (readl(ioaddr + MACCR_REG) & SW_RST_bit) != 0;) {
        mdelay(10);
        PRINTK3("RESET: reset not complete yet\n");
    }
    tmp = readl(ioaddr + MACCR_REG);
    tmp |= 0x80000;
    writel(tmp, ioaddr + MACCR_REG);
    writel(0, ioaddr + IER_REG);        /* Disable all interrupts */
    
    set_MDC_CLK(dev);
}

/*
 . Function: ftgmac100_enable
 . Purpose: let the chip talk to the outside work
 . Method:
 .	1.  Enable the transmitter
 .	2.  Enable the receiver
 .	3.  Enable interrupts
*/
static void ftgmac100_enable(struct eth_device *dev)
{
    unsigned int ioaddr = dev->iobase;
    int i;
    struct ftgmac100_local *lp = (struct ftgmac100_local *)dev->priv;
    unsigned int tmp_rsize;     //Richard
    unsigned int rfifo_rsize;   //Richard
    unsigned int tfifo_rsize;   //Richard
    unsigned int rxbuf_size;

    PRINTK2("%s:ftgmac100_enable\n", dev->name);

#ifdef NC_Body
    rxbuf_size = RX_BUF_SIZE & 0x3fff;
    writel(rxbuf_size, ioaddr + RBSR_REG);      //for NC Body
#endif

    for (i = 0; i < RXDES_NUM; ++i) {
        lp->rx_descs[i].RXPKT_RDY = RX_OWNBY_FTGMAC100; // owned by FTMAC100
    }
    lp->rx_idx = 0;

    for (i = 0; i < TXDES_NUM; ++i) {
        lp->tx_descs[i].TXDMA_OWN = TX_OWNBY_SOFTWARE;  // owned by software
    }
    lp->tx_idx = 0;

    /* set the MAC address */
    put_mac(ioaddr + MAC_MADR_REG, (char *)dev->enetaddr);

    writel(lp->rx_descs_dma, ioaddr + RXR_BADR_REG);
    writel(lp->tx_descs_dma, ioaddr + TXR_BADR_REG);
    writel(0x00001010, ioaddr + ITC_REG);       // 此值為 document 所建議
    ///writel( 0x0, ioaddr + ITC_REG);
    ///writel( (1UL<<TXPOLL_CNT)|(1UL<<RXPOLL_CNT), ioaddr + APTC_REG);
    writel((0UL << TXPOLL_CNT) | (0x1 << RXPOLL_CNT), ioaddr + APTC_REG);
    //writel( 0x1df, ioaddr + DBLAC_REG );                                          // 此值為 document 所建議
    writel(0x22f97, ioaddr + DBLAC_REG);        //Richard burst

    writel(readl(FCR_REG) | 0x1, ioaddr + FCR_REG);     // enable flow control
    writel(readl(BPR_REG) | 0x1, ioaddr + BPR_REG);     // enable back pressure register

    // +++++ Richard +++++ //
    tmp_rsize = readl(ioaddr + FEAR_REG);
    rfifo_rsize = tmp_rsize & 0x00000007;
    tfifo_rsize = (tmp_rsize >> 3) & 0x00000007;

    tmp_rsize = readl(ioaddr + TPAFCR_REG);
    tmp_rsize &= 0x3f000000;
    tmp_rsize |= (tfifo_rsize << 27);
    tmp_rsize |= (rfifo_rsize << 24);

    writel(tmp_rsize, ioaddr + TPAFCR_REG);
    // ----- Richard ----- //

    /* now, enable interrupts */
    writel(PHYSTS_CHG_bit | AHB_ERR_bit |
///                     RPKT_LOST_bit   |
///                     RPKT2F_bit      |
///                     TPKT_LOST_bit   |
///                     TPKT2E_bit              |
///                     NPTXBUF_UNAVA_bit               |
///                     TPKT2F_bit      |
///                     RXBUF_UNAVA_bit         |
           RPKT2B_bit, ioaddr + IER_REG);

    /// enable trans/recv,...
    writel(lp->maccr_val, ioaddr + MACCR_REG);

#ifdef FTGMAC100_TIMER
    /// waiting to do: 兩個以上的網路卡
    init_timer(&ftgmac100_timer);
    ftgmac100_timer.function = ftgmac100_timer_func;
    ftgmac100_timer.data = (unsigned long)dev;
    mod_timer(&ftgmac100_timer, jiffies + FTGMAC100_STROBE_TIME);
#endif
}

/*
 . Function: ftmac100_shutdown
 . Purpose:  closes down the chip.
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
static void ftgmac100_shutdown(unsigned int ioaddr)
{
    /// unmask interrupt mask register
    writel(0, ioaddr + IER_REG);

    /// disable trans/recv,...
    writel(0, ioaddr + MACCR_REG);
}

//static int time_old,time_new;
static int ftgmac100_send_packet(struct eth_device *dev, void *packet, int length)
{
    volatile struct ftgmac100_local *lp = (struct ftgmac100_local *)dev->priv;
    volatile unsigned int ioaddr = dev->iobase, i = 0;
    volatile TX_DESC *cur_desc;

    //printf("<d%x>",get_timer(0));
    PRINTK3("%s:ftgmac100_wait_to_send_packet\n", dev->name);
    cur_desc = &lp->tx_descs[lp->tx_idx];
    for (; cur_desc->TXDMA_OWN != TX_OWNBY_SOFTWARE;)   /// 沒有空的 transmit descriptor 可以使用
    {
        DO_PRINT("Transmitting busy\n");
        udelay(10);
    }
    length = ETH_ZLEN < length ? length : ETH_ZLEN;
    length = length > TX_BUF_SIZE ? TX_BUF_SIZE : length;

#if FTGMAC100_DEBUG > 2

    DO_PRINT("Transmitting Packet\n");
    print_packet(packet, length);       //Justin
#endif
    //memcpy((char *)cur_desc->VIR_TXBUF_BADR, packet, length);             /// waiting to do: 將 data 切成許多 segment
    memcpy((char *)virt_to_phys(cur_desc->TXBUF_BADR), packet, length); //Richard
    //time_new=get_timer(0);
    //if((time_new-time_old)<5000)
    //printf("<%x>",readl(ioaddr + 0xa0));
#if 0
    {
        volatile unsigned int j = 0;
        for (i = 0; i < 8; i++) {
            printf("\n");
            printf("%02x:", (i * 32));
            for (j = 0; j < 8; j++)
                printf(" %08x", readl(ioaddr + (j * 4) + (i * 32)));

        }
        printf("===\n");
    }
#endif
    {
        cur_desc->TXBUF_Size = length;
        cur_desc->LTS = 1;
        cur_desc->FTS = 1;
        cur_desc->TX2FIC = 0;
        cur_desc->TXIC = 0;
        cur_desc->TXDMA_OWN = TX_OWNBY_FTGMAC100;
        writel(0xffffffff, ioaddr + TXPD_REG);
//printf("=>");
        lp->tx_idx = (lp->tx_idx + 1) % TXDES_NUM;
    }
//else
//      printf("too late\n");
//time_old=get_timer(0);
//printf("<%x>",lp->tx_idx);
    while (cur_desc->TXDMA_OWN == 1) {
        udelay(10);
        i++;
        if (i > 100) {
            printf("timeout\n");
            break;
        }
        if (cur_desc->TXDMA_OWN == 0)
            break;
        //printf("<%x>",get_timer(0));
    }
    //printf("<%x>",readl(ioaddr + 0xa0));
    return 0;
}

/*-------------------------------------------------------------------------
 |
 | smc_destructor( struct eth_device * dev )
 |   Input parameters:
 |	dev, pointer to the device structure
 |
 |   Output:
 |	None.
 |
 ---------------------------------------------------------------------------
*/
void ftgmac100_destructor(struct eth_device *dev)
{
    PRINTK3("%s:ftgmac100_destructor\n", dev->name);
}

/*
 * Open and Initialize the board
 *
 * Set up everything, reset the card, etc ..
 *
 */
static int ftgmac100_open(struct eth_device *dev)
{
    PRINTK2("%s:ftgmac100_open\n", dev->name);

    /* reset the hardware */
    ftgmac100_reset(dev);
    ftgmac100_enable(dev);

    return 0;
}

#ifdef USE_32_BIT
void insl32(r, b, l)
{
    int __i;
    dword *__b2;

    __b2 = (dword *) b;
    for (__i = 0; __i < l; __i++) {
        *(__b2 + __i) = *(dword *) (r + 0x10000300);
    }
}
#endif

/*-------------------------------------------------------------
 .
 . ftmac100_rcv -  receive a packet from the card
 .
 . There is ( at least ) a packet waiting to be read from
 . chip-memory.
 .
 . o Read the status
 . o If an error, record it
 . o otherwise, read in the packet
 --------------------------------------------------------------
*/
static int ftgmac100_rcv(struct eth_device *dev)
{
    struct ftgmac100_local *lp = (struct ftgmac100_local *)dev->priv;
    int packet_length;
    volatile RX_DESC *cur_desc;
    int cpy_length;
    int start_idx;
    int seg_length;
    int rcv_cnt;
    //printf("<R>");
    //PRINTK3("%s:ftgmac100_rcv\n", dev->name);
    for (rcv_cnt = 0; rcv_cnt < 1; ++rcv_cnt) {
        packet_length = 0;
        start_idx = lp->rx_idx;

        for (; (cur_desc = &lp->rx_descs[lp->rx_idx])->RXPKT_RDY == RX_OWNBY_SOFTWARE;) {
            lp->rx_idx = (lp->rx_idx + 1) % RXDES_NUM;
            if (cur_desc->FRS) {
                if (cur_desc->RX_ERR || cur_desc->CRC_ERR || cur_desc->FTL || cur_desc->RUNT
                    || cur_desc->RX_ODD_NB) {
                    cur_desc->RXPKT_RDY = RX_OWNBY_FTGMAC100;   /// 此 frame 已處理完畢, 還給 hardware
                    return 0;
                }
                //packet_length = cur_desc->ReceiveFrameLength; //Richard
            }

            packet_length += cur_desc->VDBC;    //Richard

            if (cur_desc->LRS)  // packet's last frame
            {
                break;
            }
        }
        if (packet_length > 0)  // 收到一個 packet
        {
            byte *data;
//printf("R%x,%x>",get_timer(0),packet_length);
            data = (byte *) NetRxPackets[0];
            cpy_length = 0;
            for (; start_idx != lp->rx_idx; start_idx = (start_idx + 1) % RXDES_NUM) {
                seg_length = min(packet_length - cpy_length, RX_BUF_SIZE);

                //memcpy(data+cpy_length, (char *)lp->rx_descs[start_idx].VIR_RXBUF_BADR, seg_length);
                memcpy(data + cpy_length, (char *)virt_to_phys(lp->rx_descs[start_idx].RXBUF_BADR), seg_length);        //Richard
//justin
//print_packet(data+cpy_length, seg_length );
                cpy_length += seg_length;
                lp->rx_descs[start_idx].RXPKT_RDY = RX_OWNBY_FTGMAC100; /// 此 frame 已處理完畢, 還給 hardware
            }
            NetReceive(NetRxPackets[0], packet_length);
#if	FTGMAC100_DEBUG > 4
            DO_PRINT("Receiving Packet\n");
            print_packet(data, packet_length);
#endif
            return packet_length;
        }
    }
    return 0;
}

/*----------------------------------------------------
 . ftmac100_close
 .
 . this makes the board clean up everything that it can
 . and not talk to the outside world.   Caused by
 . an 'ifconfig ethX down'
 .
 -----------------------------------------------------*/
static void ftgmac100_close(struct eth_device *dev)
{
    //netif_stop_queue(dev);
    //dev->start = 0;

    PRINTK2("%s:ftgmac100_close\n", dev->name);

    /* clear everything */
    ftgmac100_shutdown(dev->iobase);

}

//---PHY CONTROL AND CONFIGURATION-----------------------------------------

#if FTGMAC100_DEBUG > 2
static void print_packet(byte * buf, int length)
{
#if 1
#if FTGMAC100_DEBUG > 3
    int i;
    int remainder;
    int lines;
#endif

    DO_PRINT("Packet of length %d \n", length);

#if FTGMAC100_DEBUG > 3
    lines = length / 16;
    remainder = length % 16;

    for (i = 0; i < lines; i++) {
        int cur;

        for (cur = 0; cur < 8; cur++) {
            byte a, b;

            a = *(buf++);
            b = *(buf++);
            DO_PRINT("%02x%02x ", a, b);
        }
        DO_PRINT("\n");
    }
    for (i = 0; i < remainder / 2; i++) {
        byte a, b;

        a = *(buf++);
        b = *(buf++);
        DO_PRINT("%02x%02x ", a, b);
    }
    DO_PRINT("\n");
#endif
#endif
}
#endif

void ftgmac100_ringbuf_alloc(struct ftgmac100_local *lp)
{
    int i;

    lp->rx_descs = kmalloc(sizeof(RX_DESC) * (RXDES_NUM + 1), GFP_DMA | GFP_KERNEL);
    if (lp->rx_descs == NULL) {
        DO_PRINT("Receive Ring Buffer allocation error\n");
        BUG();
    }
    lp->rx_descs = (RX_DESC *) ((int)(((char *)lp->rx_descs) + sizeof(RX_DESC) - 1) & 0xfffffff0);
    lp->rx_descs_dma = virt_to_phys(lp->rx_descs);
    memset((void *)lp->rx_descs, 0, sizeof(RX_DESC) * RXDES_NUM);

    lp->rx_buf = kmalloc(RX_BUF_SIZE * RXDES_NUM, GFP_DMA | GFP_KERNEL);
    lp->rx_buf_dma = virt_to_phys(lp->rx_buf);

    if (lp->rx_buf == NULL || (((u32) lp->rx_buf & 0x7) != 0) || (((u32) lp->rx_buf_dma & 0x7) != 0))   //Richard
    {
        DO_PRINT("Receive Ring Buffer allocation error, lp->rx_buf = %x\n", (u32) lp->rx_buf);
        BUG();
    }

    for (i = 0; i < RXDES_NUM; ++i) {
#ifndef NC_Body
        lp->rx_descs[i].RXBUF_Size = RX_BUF_SIZE;
#endif
        lp->rx_descs[i].EDORR = 0;      // not last descriptor
        lp->rx_descs[i].RXBUF_BADR = lp->rx_buf_dma + RX_BUF_SIZE * i;
        //lp->rx_descs[i].VIR_RXBUF_BADR = virt_to_phys( lp->rx_descs[i].RXBUF_BADR );//Richard
    }
    lp->rx_descs[RXDES_NUM - 1].EDORR = 1;      // is last descriptor

    lp->tx_descs = kmalloc(sizeof(TX_DESC) * (TXDES_NUM + 1), GFP_DMA | GFP_KERNEL);
    if (lp->tx_descs == NULL) {
        DO_PRINT("Transmit Ring Buffer allocation error\n");
        BUG();
    }
    lp->tx_descs = (TX_DESC *) ((int)(((char *)lp->tx_descs) + sizeof(TX_DESC) - 1) & 0xfffffff0);
    lp->tx_descs_dma = virt_to_phys(lp->tx_descs);
    memset((void *)lp->tx_descs, 0, sizeof(TX_DESC) * TXDES_NUM);

    lp->tx_buf = kmalloc(TX_BUF_SIZE * TXDES_NUM, GFP_DMA | GFP_KERNEL);
    if (lp->tx_buf == NULL || (((u32) lp->tx_buf % 4) != 0)) {
        DO_PRINT("Transmit Ring Buffer allocation error\n");
        BUG();
    }
    lp->tx_buf_dma = virt_to_phys(lp->tx_buf);

    for (i = 0; i < TXDES_NUM; ++i) {
        lp->tx_descs[i].EDOTR = 0;      // not last descriptor
        lp->tx_descs[i].TXBUF_BADR = lp->tx_buf_dma + TX_BUF_SIZE * i;
        //lp->tx_descs[i].VIR_TXBUF_BADR = virt_to_phys( lp->tx_descs[i].TXBUF_BADR );//Richard
    }
    lp->tx_descs[TXDES_NUM - 1].EDOTR = 1;      // is last descriptor
    PRINTK("lp->rx_descs = %x, lp->rx_rx_descs_dma = %x\n", (u32) lp->rx_descs, lp->rx_descs_dma);
    PRINTK("lp->rx_buf = %x, lp->rx_buf_dma = %x\n", (u32) lp->rx_buf, lp->rx_buf_dma);
    PRINTK("lp->tx_descs = %x, lp->tx_rx_descs_dma = %x\n", (u32) lp->tx_descs, lp->tx_descs_dma);
    PRINTK("lp->tx_buf = %x, lp->tx_buf_dma = %x\n", (u32) lp->tx_buf, lp->tx_buf_dma);
}

static int mac_write_hwaddr(struct eth_device *dev)
{
    PRINTK("init MAC-ADDR %02x:%02x:%02x:%02x:%02x:%02x\n",
           dev->enetaddr[0], dev->enetaddr[1], dev->enetaddr[2],
           dev->enetaddr[3], dev->enetaddr[4], dev->enetaddr[5]);

    return 0;
}

int ftgmac100_initialize(bd_t * bis)
{
    int i;    
    struct eth_device *dev;

    for (i = 0; i < CONFIG_NET_NUM; i++) {

        dev = (struct eth_device *)malloc(sizeof(struct eth_device));

        sprintf(dev->name, "eth%d", i);

        if (i)
            dev->iobase = CONFIG_FTGMAC100_1_BASE;
        else    
            dev->iobase = CONFIG_FTGMAC100_BASE;

        dev->init = ftgmac100_init;
        dev->halt = ftgmac100_close;
        dev->send = ftgmac100_send_packet;
        dev->recv = ftgmac100_rcv;
        dev->write_hwaddr = mac_write_hwaddr;

        eth_register(dev);

        set_MDC_CLK(dev);
    }
    return i;
}

//static word ftgmac100_read_phy_register(unsigned int ioaddr, byte phyaddr, byte phyreg)
int ftgmac100_read_phy_register(const char *devname, unsigned char addr,
                                unsigned char reg, unsigned short *value)
{
    struct eth_device *dev;
    unsigned int tmp;
    int count = 0, timeout = 0x5000;

    dev = eth_get_dev_by_name(devname);
    tmp = readl(dev->iobase + PHYCR_REG);

#ifdef CONFIG_PLATFORM_GM8210
    tmp &= 0x3000003F;
#else
    tmp &= 0x300000FF;
#endif    
    tmp |= (addr << 16);
    tmp |= (reg << (16 + 5));
    tmp |= PHY_READ_bit;

    writel(tmp, dev->iobase + PHYCR_REG);

    for (count = 0; count < timeout; count++)
        if (!(readl(dev->iobase + PHYCR_REG) & (1 << 26)))
            break;
    if (count >= timeout) {
        printk("read PHY timeout\n");
        return -1;
    }

    *value = (unsigned short)(readl(dev->iobase + PHYDATA_REG) >> 16);
    PRINTK3("reg=%d,data=0x%x\n", reg, *value);
    return 0;
}

//static void ftgmac100_write_phy_register(unsigned int ioaddr, byte phyaddr, byte phyreg, word phydata)
int ftgmac100_write_phy_register(const char *devname, unsigned char addr,
                                        unsigned char reg, unsigned short value)
{
    struct eth_device *dev;
    unsigned int tmp;
    int count = 0, timeout = 0x5000;

    dev = eth_get_dev_by_name(devname);
    tmp = readl(dev->iobase + PHYCR_REG);

#ifdef CONFIG_PLATFORM_GM8210
    tmp &= 0x3000003F;
#else
    tmp &= 0x300000FF;
#endif    
    tmp |= (addr << 16);
    tmp |= (reg << (16 + 5));
    tmp |= PHY_WRITE_bit;

    writel(value, dev->iobase + PHYDATA_REG);

    writel(tmp, dev->iobase + PHYCR_REG);

    for (count = 0; count < timeout; count++)
        if (!(readl(dev->iobase + PHYCR_REG) & (1 << 27)))
            break;
    if (count >= timeout) {
        printk("write PHY timeout\n");
        return -1;
    }

    return 0;
}

#ifdef IP101G_LED_MODE_2
static void ip101g_config_init(struct eth_device *dev)
{
    unsigned short phy_data;
    
    miiphy_write(dev->name, CONFIG_PHY_ADDR, 20, 0x3);
    miiphy_read(dev->name, CONFIG_PHY_ADDR, 16, &phy_data);
    miiphy_write(dev->name, CONFIG_PHY_ADDR, 20, 0x0);
    
    miiphy_write(dev->name, CONFIG_PHY_ADDR, 20, 0x3);
    phy_data &= ~(0x3 << 14);
    phy_data |= (0x2 << 14);
    miiphy_write(dev->name, CONFIG_PHY_ADDR, 20, phy_data);
    miiphy_write(dev->name, CONFIG_PHY_ADDR, 20, 0x0);    
}
#endif

int ftgmac100_init(struct eth_device *dev, bd_t * bis)
{
    struct ftgmac100_local *lp;
    unsigned int tmp, tmp_1;
    volatile unsigned int i;
    unsigned short phy_data;

    if (initialized == 0) {
        initialized = 1;

        /* Initialize the private structure. */
        dev->priv = (void *)malloc(sizeof(struct ftgmac100_local));
        if (dev->priv == NULL) {
            DO_PRINT("out of memory\n");
            return 0;
        }

#if 0
        tmp = ftgmac100_read_phy_register(dev->iobase, CONFIG_PHY_ADDR, 0x1);
        printf("read_phy reg 0x1=0x%x\n", tmp);
#endif

        /// --------------------------------------------------------------------
        ///             ftmac100_local
        /// --------------------------------------------------------------------
        memset(dev->priv, 0, sizeof(struct ftgmac100_local));
        //copy the mac address
        //memcpy(dev->enetaddr,ftgmac100_mac_addr,6);
        lp = (struct ftgmac100_local *)dev->priv;
        lp->maccr_val =
            FULLDUP_bit | CRC_APD_bit | RXMAC_EN_bit | TXMAC_EN_bit | RXDMA_EN_bit | TXDMA_EN_bit |
            RX_BROADPKT_bit;

#ifdef CONFIG_RGMII
        printf("RGMII mode\n");

#endif
#ifdef CONFIG_RMII
        printf("RMII mode\n");
#endif

        miiphy_register(dev->name, ftgmac100_read_phy_register, ftgmac100_write_phy_register);
        miiphy_read(dev->name, CONFIG_PHY_ADDR, MII_PHYSID1, &phy_data);   //dummy read for 8287 and 8139

#if 0                           //print PHY status
        for (tmp_1 = 0; tmp_1 <= 31; tmp_1++) {
            tmp = ftgmac100_read_phy_register(dev->iobase, CONFIG_PHY_ADDR, tmp_1);
            printf("read_phy reg %d=0x%x\n", tmp_1, tmp);
        }
#endif
#if 0                           //internal loopback test
        tmp = ftgmac100_read_phy_register(dev->iobase, CONFIG_PHY_ADDR, 0x0);
        tmp &= ~(1 << 6);
        tmp &= ~(1 << 12);
        tmp &= ~(1 << 13);
        //tmp |= (1<<14);
        ftgmac100_write_phy_register(dev->iobase, CONFIG_PHY_ADDR, 0x0, tmp);
        printf("set reg 0=%x\n", tmp);
        tmp = ftgmac100_read_phy_register(dev->iobase, CONFIG_PHY_ADDR, 0x0);
        //printf("reg reg 0=%x\n",tmp);
#endif
        miiphy_read(dev->name, CONFIG_PHY_ADDR, MII_PHYSID1, &phy_data);
        tmp = (phy_data << 16);
        miiphy_read(dev->name, CONFIG_PHY_ADDR, MII_PHYSID2, &phy_data);
        tmp |= phy_data;

        if (tmp == ICPLUS_175D_PHYID) {
            printf("Phy ID = 0x%08x\n", tmp);

            /*Force to 100M FULL mode */
            tmp_1 = readl(dev->iobase + MACCR_REG);
            tmp_1 |= FULLDUP_bit;
            lp->maccr_val |= FULLDUP_bit;
            writel(tmp_1, dev->iobase + MACCR_REG);
            printf("FULL\n");

            phy_data = _100BASET;
            if (ip175d_config_init(dev) != 0)
                printf("config ip175d fail\n");
        } else {
            for (i = 0; i < 10; i++) {
                miiphy_read(dev->name, CONFIG_PHY_ADDR, MII_PHYSID2, &phy_data);
                if (phy_data != 0xFFFF)
                    break;
            }
            printf("Phy ID = 0x%x\n", phy_data);

            for (i = 0; i < 200; i++) {
                miiphy_read(dev->name, CONFIG_PHY_ADDR, MII_BMSR, &phy_data);

                if (phy_data & BMSR_LSTATUS)    //BMSR_ANEGCOMPLETE)
                    break;
                udelay(10000);
                i++;
            }
            if (i >= 200)
                printf("Link can't be completed, detect speed fail, please check Network cable\n");
            else
                for(i = 0; i < 60000; i++); //wait TXEN ready

            /* adapt to the half/full speed settings */
            tmp_1 = readl(dev->iobase + MACCR_REG);
            if (miiphy_duplex(dev->name, CONFIG_PHY_ADDR) == HALF) {
                tmp_1 &= ~FULLDUP_bit;
                lp->maccr_val &= ~FULLDUP_bit;
                writel(tmp_1, dev->iobase + MACCR_REG);
                printf("HALF\n");
            } else {
                tmp_1 |= FULLDUP_bit;
                lp->maccr_val |= FULLDUP_bit;
                writel(tmp_1, dev->iobase + MACCR_REG);
                printf("FULL\n");
            }
            phy_data = miiphy_speed(dev->name, CONFIG_PHY_ADDR);
        }

        /* adapt to the 10/100/1000 speed settings */
        if (phy_data == _1000BASET) {
            printk("PHY_SPEED_1G\n");
            lp->maccr_val |= (GMAC_MODE_bit | SPEED_100_bit);

            //change RX clock delay value
            ftgmac100_read_phy_register(dev->name, CONFIG_PHY_ADDR, 0x10,
                                        (unsigned short *)&phy_data);
            ftgmac100_write_phy_register(dev->name, CONFIG_PHY_ADDR, 0x10, phy_data & 0xFFFFFFFE);
            //printf("read reg 16=%x\n",tmp);
        } else if (phy_data == _100BASET) {
            printk("PHY_SPEED_100M\n");
            lp->maccr_val &= ~GMAC_MODE_bit;
            lp->maccr_val |= SPEED_100_bit;
        } else {
            printk("PHY_SPEED_10M\n");
            lp->maccr_val &= ~GMAC_MODE_bit;
            lp->maccr_val &= ~SPEED_100_bit;
        }
        ftgmac100_ringbuf_alloc(lp);
    }                           //only need to set one times

    ftgmac100_open(dev);
    miiphy_read(dev->name, CONFIG_PHY_ADDR, MII_PHYSID1, &phy_data);   //dummy read for 8287 and 8139
    
#ifdef IP101G_LED_MODE_2
    ip101g_config_init(dev);
#endif    

    return 0;
}

static int ip175d_config_init(struct eth_device *dev)
{
    unsigned short phy_data;
    int err;
    /* master reset */
    err = miiphy_write(dev->name, 20, 2, 0x175D);
    if (err != 0)
        return err;

    mdelay(2);                  /* data sheet specifies reset period is 2 msec */
    /*make sure it is well after reset */
    err = miiphy_read(dev->name, 20, 2, &phy_data);
    if (err != 0)
        return err;

    /*Force MAC5 to 100M FULL mode */
    err = miiphy_read(dev->name, 20, 4, &phy_data);
    if (err != 0)
        return err;
    phy_data |= (0x1 << 13 | 0x1 << 15);
    err = miiphy_write(dev->name, 20, 4, phy_data);
    if (err != 0)
        return err;

    /*config pvlan phy 0~3 to MAC 5 ,phy 4 to MAC 5 */
    err = miiphy_write(dev->name, 23, 0, 0x2f2f);
    if (err != 0)
        return err;
    err = miiphy_write(dev->name, 23, 1, 0x2f2f);
    if (err != 0)
        return err;
    err = miiphy_write(dev->name, 23, 2, 0x3f30);
    if (err != 0)
        return err;

    return 0;
}

#endif
