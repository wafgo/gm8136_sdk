#ifndef __ZEBRA_H__
#define __ZEBRA_H__

#include <linux/device.h>       /* sysfs/class interface         */
#include <linux/cdev.h>         /* character device interface    */
#include <linux/list.h>         /* linked-lists                  */
#include <linux/workqueue.h>    /* work-queues                   */
#include <linux/timer.h>        /* timers                        */
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include "zebra_sdram.h"

#define DRV_VER   "3.3"
#define USE_KTRHEAD

/* used by pci host */
#define TO_PCI_ADDR(x, drv, brd) IS_PCI(brd) ? ((x) - (drv)->pci.axi_base_addr + (drv)->pci.pci_base_addr) : (x)
#define TO_AXI_ADDR(x, drv, brd) IS_PCI(brd) ? ((x) - (drv)->pci.pci_base_addr + (drv)->pci.axi_base_addr) : (x)

#define IS_PCI_HOST(x)  ((x)->pci_host)
#define IS_MBOX_HOST(x) ((x)->mbox_host)

/* the max buffer size transmission once */
#define MAX_BUFFSZ      			(4 << 20)   //xx M bytes
#define ZEBRA_BUFFER_LIST_LENGTH  	16   /* per device */
#define WAIT_TXEMPTY_TIMEOUT        30  //seconds
#define APP_READ_TIMEOUT            30  //seconds
#define ZEBRA_NUM_IN_PACK           36

/* macros for buffer index operation
 */
#define GET_BUFIDX(maxlen, x)	((x) % maxlen)
#define BUFIDX_DIST(head, tail)	((head) >= (tail) ? (head) - (tail) : ((unsigned short)~0 - (tail) + (head) + 1))
#define BUF_SPACE(maxlen, head, tail)	(maxlen - BUFIDX_DIST(head, tail))
#define BUF_COUNT(head, tail)	BUFIDX_DIST(head, tail)
#define GET_RXRDY_IDX(x)		((x) & 0xFFFF)
#define GET_TXEMPTY_IDX(x)		(((x) >> 16) & 0xFFFF)

typedef struct {
    union {
		struct {
			unsigned short  rx_rdy_idx;		//updated by sender
			unsigned short  tx_empty_idx;	//updated by receiver
		};
		unsigned int two_idx;
	} hdr;

    /* the number of entries is counted in entry_cnt */
    struct {
        dma_addr_t  paddr;
        int         buf_len;
    } mem_entry[1][ZEBRA_NUM_IN_PACK];
} share_memory_t;

/*
 * During the initialization time, the boards know nothing about the peer such as the
 * memory address, memory size ... for data communication. Thus we need a common space to
 * store these kinds of information to tell the peer about my environment. When the information
 * is ready, MAGIC_NUMBER which is a ready signal to tell the peer, ok I am ready.
 * Note, the mbox is for local system instead of PCI. For the PCI, we use BAR register to do the
 * communication.
 */

#define KERNEL_MEM(x)   ((unsigned int)(x) > TASK_SIZE)

/* set when ready */
#define MAGIC_NUMBER    0x35737888
/* init number */
#define INIT_NUMBER     0x05711438

enum {
    ZEBRA_OPEN_READ = 0x1,
    ZEBRA_OPEN_WRITE = 0x2
};

typedef enum {
    ZEBRA_DMA_NONE = 0,
    ZEBRA_DMA_AXI,
    ZEBRA_DMA_AHB
} zebra_dma_t;

/*
 * The message box passes the information to the zebra core.
 * This struture is a temp communication structure. Used to inform the peer about
 * mine share buffer information.
 */
typedef struct mbox_dev_s {
    char *name;
    int peer_irq;       //peer irq
    int local_irq;
    u32 mbox_ofs;     //communication mbox paddr offset
    cpu_core_id_t peer_cpu_id;
    cpu_core_id_t local_cpu_id;
    void *private_data;
    int  identifier;    /* only 16 bits are used */
} mbox_dev_t;

/*
 * the structure size must be less or equal to ONE_MBOX_SIZE
 */
typedef struct mbox {
#define NUM_CPU     4
    struct {
        cpu_core_id_t id;
        u32 paddr;  //store the data communciation buffer allocated from the host
        u32 size;
        u32 mempool_paddr;  //physical address of memory pool
        u32 mempool_size;   //memory pool size
        u32 magic_number;
    } data[NUM_CPU];        //three cpu core
} mbox_t;

/* the definition is coressponding to zebra.h */
enum {
    ZEBRA_DEVICE_0 = 0,
    ZEBRA_DEVICE_1,
    ZEBRA_DEVICE_2,
    ZEBRA_DEVICE_3,
    ZEBRA_DEVICE_4,
    ZEBRA_DEVICE_5,
    ZEBRA_DEVICE_6,
    ZEBRA_DEVICE_7,
    ZEBRA_DEVICE_8,
    ZEBRA_DEVICE_9,
    ZEBRA_DEVICE_10,
    ZEBRA_DEVICE_11,
    ZEBRA_DEVICE_12,
    ZEBRA_DEVICE_13,
    ZEBRA_DEVICE_14,
    ZEBRA_DEVICE_MAX
};

/* AP use */
#define ZEBRA_DEVICE_AP_START       ZEBRA_DEVICE_12

typedef struct {
    int device;
    int write_ofs;
    int read_ofs;
    int bufsz;      		/* max buffer size during transmission */
    int max_list_length; 	/* read and write list length */
} bufmap_t;

#define ZEBRA_BUFSZ(x)            	ZEBRA_DEVICE##x##_BUFSZ
#define NUM_OF_DEVICES_PER_BOARD   	(ZEBRA_DEVICE_MAX - ZEBRA_DEVICE_AP_START)

/* Doorbell register bits */
#define RX_READY    1
#define TX_EMPTY    2

/* Door direction */
#define DOORBELL_RXREADY    0x00001111  /* RX ready */
#define DOORBELL_TXEMPTY    0x00002222  /* TX Empty */

/* attributes */
#define ATTR_PACK           0x1

typedef struct
{
    unsigned int dropped;     //how many frames are dropped due to 1) no memory, 2) channel not opened.
    unsigned int user_blocking;    //if the space is full, the caller will be blocked until one space is released.
    unsigned int user_read;   //how many frames are read from user space.
    unsigned int user_write;  //how many frames are wrote from user space.
    unsigned int error;       //how many coding errors
    unsigned int write_ack;   //how many acks are received for write
    unsigned int va_exception;
    unsigned int send_rxrdy_interrupts;    //send by myself
    unsigned int send_txempty_interrupts;  //send by myself
    unsigned int rcv_rxrdy_interrupts;    //recevie from the remote
    unsigned int rcv_txempty_interrupts;  //recevie from the remote
    unsigned int workaround;
} counter_t;

/*-----------------------------------------------------------------
 * Device structures
 *-----------------------------------------------------------------
 */

/* Read/write buffer list entry
 *
 * Read and write copy data to or from applications in terms
 * of a linked list of buffers. When a application write occurs,
 * the length of the write is checked and truncated to the
 * maximum length allowed by the device, and the application
 * data is copied into the buffer. When the device writes to
 * the host, a similar action is performed, and its data
 * ends up on the read list. When application reads the data,
 * the driver removes a buffer from the list, and copies data
 * out until all data is read (multiple applications reads are
 * allowed).
 *
 * Buffer list entries and buffer pages are allocated in write().
 * The list entries are allocated from the buffer cache, while
 * the buffer pages are allocated using alloc_pages(). The
 * driver write() and read() routines temporarily map pages in
 * the buffer, a page at a time using kmap().
 * The data is written starting at buffer = kmap(entry->page),
 * of length entry->length, where the length is less than or
 * equal to the maximum length for the device, and entry->offset
 * is set to zero (that field is used by read).
 * (The field names offset and length were chosen to match
 * the same names in a struct scatterlist).
 *
 * read() starts reading from the first page in the buffer.
 * If the user-space read request is less than entry->length,
 * then entry->offset and entry->length are updated, and the
 * next read continues with the same buffer element. For
 * buffers that are multiple pages long, care is taken to
 * map each page to kernel addresses before accessing it.
 *
 * Once the buffer is empty, it is deallocated (the pages
 * are freed, and the list entry is returned to the buffer
 * list entry cache).
 **/
typedef struct buffer_entry {
	struct list_head list;  //linked to buffer_list
	struct list_head pack_list;
	unsigned int    offset;  /* Start of the buffer */
	unsigned int    length;  /* Number of characters */
	unsigned int    length_keep;  /* Number of characters */
	unsigned char   *databuf;
	unsigned char   *direct_databuf;
	int             f_ioreamp;  //1 if ioreamap
	dma_addr_t      phy_addr;
	void            *va_addr;
	void            *private;   //board
} buffer_entry_t;

/* Buffer list (used for read and write buffers) */
typedef struct buffer_list {
	struct list_head list;
	struct semaphore sem;
	wait_queue_head_t wait;
	int             list_length;
	unsigned int    list_length_max;
	unsigned int    buffer_length_max;
} buffer_list_t;

/* If the buffer remove from write_list, it will be moved to transmit_list until the peer
 * finish reading.
 */
typedef struct transmit_list {
    struct list_head list;
    int             list_length;
    unsigned int    complete_length;    /* increase when when tx-empty is recevied */
    /* same as hdr of share_memory_t */
    unsigned short  rx_rdy_idx;
    unsigned short  tx_empty_idx;
} transmit_list_t;

#define NAME_LEN    20

/* define the channel */
typedef struct zebra_device {
    char name[NAME_LEN + 1];

    /* Write buffer list
	 * - write() adds to the head of the list
	 * - the work function removes from the tail
	 */
	buffer_list_t write_list;

	/* Wait-queue for user-space fsync() / zebra_fsync() */
	wait_queue_head_t   fsync_wait;
	u32                 fsync_requested;

	/* Read buffer list
	 * - the work function adds to the head of the list
	 * - read() removes from the tail (list->prev)
	 */
	buffer_list_t read_list;

    /* transmitting list */
    transmit_list_t   rd_transmit_list;
    transmit_list_t   wr_transmit_list;

	/* Opened for read flag */
	unsigned int open_read;
	unsigned int open_mode; /* ZEBRA_OPEN_READ/ZEBRA_OPEN_WRITE */
	/* Buffer list element used by read */
	buffer_entry_t *read_entry;

    struct work_struct write_request;
    struct work_struct pack_write_request;
    struct work_struct write_error;

	struct semaphore write_mutex;
	struct semaphore write_busy;

	struct work_struct read_request;
	struct work_struct pack_read_request;

#ifdef USE_KTRHEAD
    struct task_struct *read_thread;
    struct task_struct *write_thread;
    wait_queue_head_t read_thread_wait;
    wait_queue_head_t write_thread_wait;
    u32     read_thread_event;
    u32     write_thread_event;
#endif /* USE_KTRHEAD */

	u32     write_event;

	/* Read transaction status */
	struct semaphore read_mutex;
	struct semaphore read_busy;
	u32 read_event;

	/* Write timeout (RX_READY to TX_EMPTY timeout) */
	struct timer_list write_timeout;    /* the timer to wait tx_empty */
    struct timer_list read_timeout;     /* the timer for the application to read data */

	/* Device SDRAM regions */
	u32     write_paddr;       /* physical address */
	char    *write_kaddr;      /* kernel address */
	u32     read_paddr;
	char    *read_kaddr;

    /* RX ready status */
    u32     local_rx_ready_status_paddr;    /* physical address */
    char    *local_rx_ready_status_kaddr;   /* kernel address */
    u32     peer_rx_ready_status_paddr;     /* physical address */
    char    *peer_rx_ready_status_kaddr;    /* kernel address */

    /* TX empty status */
	u32     local_tx_empty_status_paddr;    /* physical address */
	char    *local_tx_empty_status_kaddr;   /* kernel address */
	u32     peer_tx_empty_status_paddr;     /* physical address */
	char    *peer_tx_empty_status_kaddr;    /* kernel address */

	/* The index of device */
	u32     device_id;  //starts from ZEBRA_DEVICE_0
    unsigned int attribute; //attribute ATTR_PACK ....

	/* Device doorbell bits */
	u32 rx_ready;
	u32 tx_empty;
	counter_t   counter;
	int         mtu;
    /* Board reference */
	struct zebra_board   *board;

	/* timestamp for debug purpose. From cpucomm view */
	u32 pktwr_in_jiffies;   //ap send to cpucomm
	u32 pktwr_out_jiffies;  //cpucomm send out to peer
	u32 pktrd_in_jiffies;   //peer sends to cpucomm
	u32 pktrd_out_jiffies;  //ap rcv from cpucomm
} zebra_device_t;

struct zebra_driver;
typedef struct {
    u32     paddr;
    u32     vaddr;
    u32     size;
} zebra_addr_t;

/*
 * Board structure, each board indicates a sub-system.
 * The board consists of many devices(channels).
 */
typedef struct zebra_board {
    int                 board_id;       /* consist of pci_id + board_id */
    cpu_core_id_t       peer_cpu_id;    /* peer cpu core id */
    cpu_core_id_t       local_cpu_id;   /* local cpu core id */
    char                name[NAME_LEN+1];
    /* Character device registration
	 *  - the open() method maps the cdev in this structure
	 *    back to the containing zebra_board_t
	 */
#ifndef WIN32
    struct cdev         cdev;
    /* Major/minor numbers */
	dev_t               devno;
	struct device       *dev;
#endif
	/* the memory for indentifier recognize */
	zebra_addr_t        mbox_addr;          /* for temp communication only */
	zebra_addr_t        local_map_mem;
	zebra_addr_t        peer_map_mem;
    zebra_addr_t        pool_mem;           /* peer memory pool information */
    u32                 pool_mem_cache;     /* new, for cache buffer purpose */
    zebra_device_t      device[ZEBRA_DEVICE_MAX];
    int                 local_irq;          /* irq number to receive interupt */
    int                 peer_irq;           /* irq number of the peer */
    int                 devices_initialized;

    struct list_head    list;               /* board list */
    struct zebra_driver *driver;
    struct timer_list   fire_rx_ready;      /* timer to aggregate the ringbells */
    struct timer_list   fire_tx_empty;      /* timer to aggregate the ringbells */

    u32     local_rx_ready_status_paddr;    /* physical address */
	char    *local_rx_ready_status_kaddr;   /* kernel address */
	u32     peer_rx_ready_status_paddr;     /* physical address */
	char    *peer_rx_ready_status_kaddr;    /* kernel address */
	u32     local_tx_empty_status_paddr;    /* physical address */
	char    *local_tx_empty_status_kaddr;   /* kernel address */
	u32     peer_tx_empty_status_paddr;     /* physical address */
	char    *peer_tx_empty_status_kaddr;    /* kernel address */
    int     doorbell_delayms;               /* default is 0 */
    int     nice_val;                       /* nice value, only valid when USE_KTHREAD is set */

    /* count error in the code */
	unsigned int    internal_error;
	/* count the number of ringbell is triggered */
	unsigned int    rx_ready_cnt;
	unsigned int    tx_empty_cnt;
} zebra_board_t;

/* Driver structure (common resources) */
typedef struct zebra_driver {
	/* Buffer list entry cache */
	struct kmem_cache   *buffer_cache; /* descriptor only */

	/* Work queue */
	struct workqueue_struct *work_queue;
#ifndef WIN32
    struct class        *class;
#endif
	/* counter */
    atomic_t            buffer_alloc;
    atomic_t            databuf_alloc;
    /* message box address of local */
    unsigned int        *mbox_kaddr;    //ioremap
    dma_addr_t          mbox_paddr;
    /* message box address of PCI */
    unsigned int        *pci_mbox_kaddr;    //ioremap
    dma_addr_t          pci_mbox_paddr;

    int                 mbox_host;
    zebra_dma_t         dma_type;

    /* pool information */
    u32 local_pool_paddr;
    u32 local_pool_sz;

    /* not zero if the pool given by PCI HOST */
    u32 pci_pool_paddr;
    u32 pci_pool_sz;

    //for PCI
    struct {
        u32 pci_base_addr;
        u32 axi_base_addr;
    } pci;

    int pci_host;   //1 for pci host, 0 for pci device
    /* board list in driver */
    struct list_head    board_list;
} zebra_driver_t;

int zebra_init(int mbox_host, zebra_dma_t dma_type, int ep_cnt, int non_ep_cnt, u32 pool_sz);
void zebra_exit(void);

int zebra_probe(mbox_dev_t *mbox_dev);
int zebra_remove(mbox_dev_t *mbox_dev);

int zebra_device_open(int board_id, int device_id, char *name, u32 attribute);
int zebra_device_close(int board_id, int device_id);

/* return value: 0 for success, otherwise for fail.
 * timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 */
int zebra_msg_snd(int board_id, int device_id, unsigned char *buf, int size, int timeout_ms);

/* return value: 0 for success, otherwise for fail.
 * timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 */
int zebra_msg_rcv(int board_id, int device_id, unsigned char *buf, int *size, int timeout_ms);
/* return value: 0 for success, otherwise for fail.
 * timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 */
int zebra_pack_msg_snd(int board_id, int device_id, unsigned char **buf, int *size, int timeout_ms);
/* return value: 0 for success, otherwise for fail.
 * timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 */
int zebra_pack_msg_rcv(int board_id, int device_id, unsigned char **buf, int *size, int timeout_ms);
/* interrupt handler */
void zebra_process_doorbell(zebra_board_t *brd);
/* get the max message size to read/write of the specific channel. > 0 for success, others for fail. */
int zebra_get_msgsz(int board_id, int device_id);

/* This function is used to reset the on-hand entry if the user doesn't want to process this
 * entry anymore.
 * Note: the packets queuing in the readlist are not affected.
 */
void zebra_pack_clean_readentry(int board_id, int device_id);

/* if the threshold meet, send tx_empty */
static inline int need_send_txempty(int space)
{
    int send_txempty;

    switch (space) {
      case 0:
      case 1:
      case 4:
      case 8:
        send_txempty = 1;
        break;
      default:
        send_txempty = 0;
        break;
    }
    return send_txempty;
}

#endif /* __ZEBRA_H__ */


