/*-----------------------------------------------------------------
 * Includes
 *-----------------------------------------------------------------
 */
#include <linux/version.h>      /* kernel versioning macros      */
#include <linux/module.h>       /* kernel modules                */
#include <linux/poll.h>         /* poll flags                    */
#include <linux/device.h>       /* sysfs/class interface         */
#include <linux/cdev.h>         /* character device interface    */
#include <linux/list.h>         /* linked-lists                  */
#include <linux/slab.h>         /* kmem_cache_t                  */
#include <linux/interrupt.h>    /* interrupts                    */
#include <linux/workqueue.h>    /* work-queues                   */
#include <linux/timer.h>        /* timers                        */
#include <linux/pci.h>          /* PCI + PCI DMA API             */
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/page.h>           /* get_order                     */
#include <asm/cacheflush.h>
#include <linux/platform_device.h>
#ifdef CONFIG_FTDMAC030
#include <mach/ftdmac030.h>
#endif
#ifdef CONFIG_FTDMAC020
#include <mach/ftdmac020.h>
#endif
#include <mach/fmem.h>
#include "zebra_sdram.h"        /* Host-to-DSP SDRAM layout      */
#include "zebra.h"
#include "mem_alloc.h"
#include "dma_llp.h"
#include <cpu_comm/cpu_comm.h>
#include <frammap/frammap_if.h>
#include <mach/dma_gm.h>
#include "platform.h"

//#define DEBUG

/*-----------------------------------------------------------------
 * local variable parameters
 *-----------------------------------------------------------------
 */
static zebra_driver_t   *zebra_driver = NULL;
static const char *zebra_name = "ZEBRA";
static unsigned int zebra_major = 0;
/* Minor number request tracking */
static unsigned int zebra_minor = 0;

/* for multiple boards */
static u32 pci_device_next_memory_paddr = 0;
static u32 pci_device_memory_poolsz = 0;

/*-----------------------------------------------------------------
 * extern function
 *-----------------------------------------------------------------
 */
extern int zebra_proc_init(zebra_board_t *brd, char *name);
extern void zebra_proc_remove(zebra_board_t *brd);

/*-----------------------------------------------------------------
 * local function declaration
 *-----------------------------------------------------------------
 */
static void zebra_write_timeout_handler(unsigned long data);
static void zebra_write_done(void *data);
static void zebra_debug_print(zebra_board_t *brd);

#ifdef USE_KTRHEAD
static int zebra_read_thread(void *data);
static int zebra_write_thread(void *data);
#endif

/*-----------------------------------------------------------------
 * macro definition
 *-----------------------------------------------------------------
 */
#define IS_PCI_DEVICE(x, y)   (IS_PCI(y) && !IS_PCI_HOST(x))
/* Error message macro */
#define LOG_ERROR(string, args...) \
	printk("%s(%s):" string, zebra_name, __func__, ##args)

/* Debug messages */
#ifdef DEBUG
#define LOG_DEBUG(string, args...) printk("%s:" string, zebra_name, ##args)
#else
#define LOG_DEBUG(string, args...)
#endif

#define ZEBRA_IOCTL_WRITE_MAX   CPU_COMM_READ_MAX
#define ZEBRA_IOCTL_READ_MAX    CPU_COMM_WRITE_MAX
#define ZEBRA_IOCTL_MEM_ATTR    CPU_COMM_MEM_ATTR

/*-----------------------------------------------------------------
 * File operations declarations
 *-----------------------------------------------------------------
 */
/* ZEBRA driver */
static int zebra_driver_open(struct inode *inode, struct file *file);
static int zebra_driver_release(struct inode *inode, struct file *file);
/* ZEBRA devices */
static ssize_t zebra_read(struct file *file, char *buf,	size_t count, loff_t *offset);
static ssize_t zebra_write(struct file *file, const char *buf, size_t count, loff_t *offset);
static unsigned int zebra_poll(struct file *file, poll_table *wait);
static long zebra_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int zebra_open(struct inode *inode, struct file *file);
static int zebra_release(struct inode *inode, struct file *file);
static void zebra_buffer_entry_destroy(buffer_entry_t *entry);

/* File operations structure  */
static struct file_operations zebra_driver_fops = {
	.owner	  = THIS_MODULE,
	.open	  = zebra_driver_open,
	.release  = zebra_driver_release,
};

static struct file_operations zebra_fops = {
	.owner	  = THIS_MODULE,
	.read	  = zebra_read,
	.write	  = zebra_write,
	.poll	  = zebra_poll,
	.unlocked_ioctl	  = zebra_ioctl,
	.open	  = zebra_open,
	.release  = zebra_release,
};

/*-----------------------------------------------------------------
 * function body
 *-----------------------------------------------------------------
 */
static bufmap_t bufmap[] = {
    /* device id,    write_ofs,           , read_ofs,            , buff sz       , max list length */
    {ZEBRA_DEVICE_0, ZEBRA_DEVICE_0_BUFOFS, ZEBRA_DEVICE_0_BUFOFS, ZEBRA_BUFSZ(0), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_1, ZEBRA_DEVICE_1_BUFOFS, ZEBRA_DEVICE_1_BUFOFS, ZEBRA_BUFSZ(1), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_2, ZEBRA_DEVICE_2_BUFOFS, ZEBRA_DEVICE_2_BUFOFS, ZEBRA_BUFSZ(2), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_3, ZEBRA_DEVICE_3_BUFOFS, ZEBRA_DEVICE_3_BUFOFS, ZEBRA_BUFSZ(3), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_4, ZEBRA_DEVICE_4_BUFOFS, ZEBRA_DEVICE_4_BUFOFS, ZEBRA_BUFSZ(4), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_5, ZEBRA_DEVICE_5_BUFOFS, ZEBRA_DEVICE_5_BUFOFS, ZEBRA_BUFSZ(5), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_6, ZEBRA_DEVICE_6_BUFOFS, ZEBRA_DEVICE_6_BUFOFS, ZEBRA_BUFSZ(6), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_7, ZEBRA_DEVICE_7_BUFOFS, ZEBRA_DEVICE_7_BUFOFS, ZEBRA_BUFSZ(7), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_8, ZEBRA_DEVICE_8_BUFOFS, ZEBRA_DEVICE_8_BUFOFS, ZEBRA_BUFSZ(8), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_9, ZEBRA_DEVICE_9_BUFOFS, ZEBRA_DEVICE_9_BUFOFS, ZEBRA_BUFSZ(9), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_10, ZEBRA_DEVICE_10_BUFOFS, ZEBRA_DEVICE_10_BUFOFS, ZEBRA_BUFSZ(10), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_11, ZEBRA_DEVICE_11_BUFOFS, ZEBRA_DEVICE_11_BUFOFS, ZEBRA_BUFSZ(11), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_12, ZEBRA_DEVICE_12_BUFOFS, ZEBRA_DEVICE_12_BUFOFS, ZEBRA_BUFSZ(12), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_13, ZEBRA_DEVICE_13_BUFOFS, ZEBRA_DEVICE_13_BUFOFS, ZEBRA_BUFSZ(13), ZEBRA_BUFFER_LIST_LENGTH},
    {ZEBRA_DEVICE_14, ZEBRA_DEVICE_14_BUFOFS, ZEBRA_DEVICE_14_BUFOFS, ZEBRA_BUFSZ(14), ZEBRA_BUFFER_LIST_LENGTH},
};

static inline void zebra_memory_copy(char *dest, char *src, int length, zebra_device_t *dev, int dir)
{
    int i, count;
    u32 *src_buf = (u32 *)src;
    u32 *dest_ptr = (u32 *)dest;

    if (dir) {}

    /* small size use CPU copy is enough.
     * if cpu copy, we don't need to sync cache. And the destination buffer is NCNB as well.
     */
    count = length >> 2;
    for (i = 0; i < count; i ++) {
        dest_ptr[i] = src_buf[i];
    }
    count = length % 4;
    if (count)
        memcpy(&dest_ptr[i], &src_buf[i], count);
}

/*-----------------------------------------------------------------
 * Driver-level file operations definitions
 *-----------------------------------------------------------------
 *
 * open() dispatches to the appropriate device fops.
 */
/* ZEBRA driver
 */
int zebra_driver_open(struct inode *inode,	struct file *file)
{
    zebra_board_t   *brd;
	zebra_device_t  *dev;
	int             ap_ch_id;
	unsigned int    major, minor;

	LOG_DEBUG("<driver open>\n");

	/* Find the major/minor */
	major = imajor(inode);
	minor = iminor(inode);

	/* Test for how it was opened */
	if (file->f_mode & FMODE_READ) {
		if (file->f_mode & FMODE_WRITE) {
			LOG_DEBUG("<driver-open> read/write, major: %d, minor:%d\n", major, minor);
		} else {
			LOG_DEBUG("<driver-open> read-only, major: %d, minor:%d\n", major, minor);
		}
	} else {
		if (file->f_mode & FMODE_WRITE) {
			LOG_DEBUG("<driver-open> write-only, major: %d, minor:%d\n", major, minor);
		} else {
			LOG_DEBUG("<driver-open> with unexpected f_mode setting!\n");
            return -EINVAL;
		}
	}

	/* Get the board reference */
	brd = container_of(inode->i_cdev, zebra_board_t, cdev);
	/* Check the major number */
	if (major != MAJOR(brd->devno)) {
		printk("ERROR: major number mismatch\n");
		return -EINVAL;
	}

    ap_ch_id = minor - MINOR(brd->devno) + ZEBRA_DEVICE_AP_START;
	dev = &brd->device[ap_ch_id];
	if (IS_PCI(brd))
	    sprintf(dev->name, "pcie_user_ap%d", ap_ch_id - ZEBRA_DEVICE_AP_START);
	else
	    sprintf(dev->name, "user_ap%d", ap_ch_id - ZEBRA_DEVICE_AP_START);
    printk("%s -- name: %s \n", __func__, dev->name);

	/* Pass off the device-specific info to the other fops */
	file->private_data = dev;
	file->f_op = &zebra_fops;

	return file->f_op->open(inode, file);
}

int zebra_driver_release(struct inode *inode, struct file *file)
{
    if (inode || file) {}

    LOG_DEBUG("<driver release>\n");
    return 0;
}

unsigned int zebra_poll(struct file *file, poll_table *wait)
{
    zebra_device_t *dev = file->private_data;
	buffer_list_t *list;
	unsigned int mask = 0;
	int length, space;

	LOG_DEBUG("<%s>\n", __func__);

    /* Read list */
	list = &dev->read_list;

    /* Add the wait-queue to the poll table */
	poll_wait(file, &list->wait, wait);

	/* Get the read buffer list length */
	down(&list->sem);
	length = list->list_length;
	up(&list->sem);
	if (length > 0) {
		LOG_DEBUG(" - dev%d, the device is readable (read list)\n", dev->device_id);
		mask |= POLLIN | POLLRDNORM;  /* readable  */
	}

	/* Its also readable if there is a read entry */
	down(&dev->read_mutex);
	if (dev->read_entry) {
		LOG_DEBUG(" - dev%d, the device is readable (read entry)\n", dev->device_id);
		mask |= POLLIN | POLLRDNORM;  /* readable  */
	}
	up(&dev->read_mutex);

	/* Write list */
	list = &dev->write_list;

	/* Add the wait-queue to the poll table */
	poll_wait(file, &list->wait, wait);

	/* Get the write buffer list space */
	down(&list->sem);
	space = list->list_length_max - list->list_length;
	up(&list->sem);
	if (space > 0) {
		LOG_DEBUG(" - dev%d, the device is writeable\n", dev->device_id);
		mask |= POLLOUT | POLLWRNORM; /* writable  */
	}

	/* no exceptions supported */
	return mask;
}

static long zebra_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	zebra_device_t *dev = (zebra_device_t *)file->private_data;
	unsigned long retVal = 0;
	u32 val;

	LOG_DEBUG("<ioctl> command 0x%.8X\n", cmd);

	switch (cmd) {
      case ZEBRA_IOCTL_WRITE_MAX:
		val = dev->write_list.buffer_length_max;
		LOG_DEBUG(" - WRITE_MAX 0x%.8X\n", val);
		if (put_user(val, (int *)arg))
			retVal = -EFAULT;
		break;

	  case ZEBRA_IOCTL_READ_MAX:
		val = dev->read_list.buffer_length_max;
		LOG_DEBUG(" - READ_MAX 0x%.8X\n", val);
		if (put_user(val, (int *)arg))
			retVal = -EFAULT;
		break;

      case ZEBRA_IOCTL_MEM_ATTR:
        if (copy_from_user(&val, (unsigned int *)arg, sizeof(unsigned int))) {
            retVal = -EFAULT;
            break;
        }
        LOG_DEBUG(" - MEM_ATTR 0x%x\n", val);
        /* TBD */
        printk("MEM_ATTR -- No ready yet! \n");
        break;

	  default:
		return -ENOTTY;
	}

	return retVal;
}

int zebra_open(struct inode *inode, struct file *file)
{
	zebra_board_t *brd;
	zebra_device_t *dev;
	unsigned int major, minor, index;

	LOG_DEBUG("<open>\n");

	/* Find the major/minor (p55 [1]) */
	major = imajor(inode);
	minor = iminor(inode);

	/* Get the board reference */
	brd = container_of(inode->i_cdev, zebra_board_t, cdev);

	/* Check the major number */
	if (major != MAJOR(brd->devno)) {
		LOG_ERROR(" - ERROR: major number mismatch\n");
		return -EINVAL;
	}

	index = minor - MINOR(brd->devno)+ ZEBRA_DEVICE_AP_START;

	dev = &brd->device[index];
	dev->attribute = 0;
	LOG_DEBUG(" - opening device index %d\n", index);

	/* Test for how it was opened */
	if (file->f_mode & FMODE_READ) {
		dev->open_mode |= ZEBRA_OPEN_READ;
		if (file->f_mode & FMODE_WRITE) {
			LOG_DEBUG(" - opened read/write\n");
			dev->open_mode |= ZEBRA_OPEN_WRITE;
		} else {
			LOG_DEBUG(" - opened read-only\n");
		}
	} else {
		if (file->f_mode & FMODE_WRITE) {
			LOG_DEBUG(" - opened write-only\n");
			dev->open_mode |= ZEBRA_OPEN_WRITE;
		} else {
			LOG_DEBUG(" - ERROR: unexpected f_mode setting\n");
			return -EINVAL;
		}
	}
	LOG_DEBUG(" - called with major %d, minor %d, by %s (pid = %u).\n",
		major, minor, current->comm, current->pid);

	/* Pass off the device-specific info to the other fops */
	file->private_data = dev;
	dev->open_read ++;

	return 0;
}

/* file operation release
 */
int zebra_release(struct inode *inode, struct file *file)
{
	zebra_device_t *dev = (zebra_device_t *)file->private_data;
    zebra_board_t  *brd = dev->board;

    return zebra_device_close(brd->board_id, dev->device_id);
}

int zebra_fsync(struct file *file, struct dentry *dentry, int datasync)
{
	zebra_device_t *dev = (zebra_device_t *)file->private_data;
	buffer_list_t *list = &dev->write_list;
	unsigned int count;
	int status;

	LOG_DEBUG("<fsync>\n");

	/* Check if the write list is empty and there is no
	 * write transaction in progress.
	 */
	down(&list->sem);
	count = list->list_length;
	up(&list->sem);

	if (!count && !down_trylock(&dev->write_busy)) {
		up(&dev->write_busy);
		LOG_DEBUG(" - dev%d, all writes have completed\n", dev->device_id);
		return 0;
	}

	/* The following wait will not block indefinitely;
	 * each write transaction starts a timeout timer, and
	 * if a write write timeout occurs, the write list
	 * is flushed.
	 */
	LOG_DEBUG(" - dev%d, wait for all writes to complete\n", dev->device_id);
	dev->fsync_requested = 1;
	status = wait_event_interruptible(
		dev->fsync_wait,
		(dev->fsync_requested == 0));
	LOG_DEBUG(" - dev%d, fsync woke up and status is %d\n", dev->device_id, status);
	return status;
}

/* user space write function
 * return value: number of bytes are transmitted.
 */
ssize_t zebra_write(struct file *file, const char *buf, size_t count, loff_t *offset)
{
    zebra_device_t *dev = (zebra_device_t *)file->private_data;
    zebra_board_t  *brd = dev->board;
    int     bWait, status;
    char    *buf_ptr;

    LOG_DEBUG("<write> (count %d, offset %d)\n", count, (int)*offset);

    /* Check the size */
	if (!count  || !dev || (count > dev->mtu))
		return 0;

	if (!(dev->open_mode & ZEBRA_OPEN_WRITE)) {
	    LOG_ERROR(" - device node is not opened! \n");
	    return 0;
	}

	bWait = (file->f_flags & O_NONBLOCK) ? 0 : -1;
	buf_ptr = (char *)(buf + *offset);

	status = zebra_msg_snd((int)brd->board_id,
	                           (int)dev->device_id,
	                           buf_ptr,
	                           count,
	                           bWait);
    if (status) {
        LOG_ERROR("%s, fail! \n", __func__);
        return 0;
    }

	return count;
}

/*
 * User space read function. count means the buffer size
 */
ssize_t zebra_read(struct file *file, char *buf, size_t count, loff_t *offset)
{
    zebra_device_t *dev = (zebra_device_t *)file->private_data;
    zebra_board_t  *brd = dev->board;
	int   status, size = count, b_wait;

	LOG_DEBUG("<read> (count = %d, offset %d), dev = 0x%x, name: %s\n", count, (int)*offset, (u32)dev, dev->name);

	/* Check the buffer count */
	if (count == 0 || count > dev->mtu)
		return 0;

	b_wait = (file->f_flags & O_NONBLOCK) ? 0 : -1;

	status = zebra_msg_rcv((int)brd->board_id,
	                       (int)dev->device_id,
	                       (char *)buf,
	                       &size,
	                       b_wait);

    if (status == 0)
        return size;

    return 0;
}

/*-----------------------------------------------------------------
 * Doorbell register access
 *-----------------------------------------------------------------
 */
void zebra_rxready_doorbell_handler(unsigned long data)
{
    zebra_board_t   *brd = (zebra_board_t *)data;

    platform_trigger_interrupt(brd, DOORBELL_RXREADY);
}

void zebra_txempty_doorbell_handler(unsigned long data)
{
    zebra_board_t   *brd = (zebra_board_t *)data;

    platform_trigger_interrupt(brd, DOORBELL_TXEMPTY);
}

/* Set local-to-peer RX_READY and TX_READY bits.
 * Flag rx_tx: RX_READY/TX_EMPTY
 */
void zebra_write_device_doorbell(zebra_device_t *dev, unsigned int dbell, unsigned int rx_tx)
{
    u32 value;
    zebra_board_t   *brd = dev->board;

    if (rx_tx == RX_READY) {
		LOG_DEBUG(" - dev%d, send RX_READY\n", dev->device_id);

        /* write device level RX_READY status */
        writel(dbell, dev->peer_rx_ready_status_kaddr);

        /* set board level RX_READY. For PCI, we skip the read back due to performance */
        value = IS_PCI(brd) ? 0 : readl(brd->peer_rx_ready_status_kaddr);
        if (value != DOORBELL_RXREADY) {
            writel(DOORBELL_RXREADY, brd->peer_rx_ready_status_kaddr);
            dev->counter.send_rxrdy_interrupts ++;
		    zebra_rxready_doorbell_handler((unsigned long)brd);
            brd->rx_ready_cnt ++;
        } else {
            LOG_DEBUG(" - dev%d, RX_REDAY is not sent again! \n", dev->device_id);
        }
        /* Write again. Acutally, multiple CPUs update the same address may casue race condition
         * with brd->peer_rx_ready_status_kaddr
         */
        writel(dbell, dev->peer_rx_ready_status_kaddr);
    } else {
        LOG_DEBUG(" - dev%d, send TX_EMPTY \n", dev->device_id);

        /* write device level TX_EMPTY status */
        writel(dbell, dev->peer_tx_empty_status_kaddr);

        /* write board level TX_EMPTY. For PCI, we skip the read back due to performance */
        value = IS_PCI(brd) ? 0 : readl(brd->peer_tx_empty_status_kaddr);
        if (value != DOORBELL_TXEMPTY) {
            writel(DOORBELL_TXEMPTY, brd->peer_tx_empty_status_kaddr);
            dev->counter.send_txempty_interrupts ++;
		    zebra_txempty_doorbell_handler((unsigned long)brd);
            brd->tx_empty_cnt ++;
        } else {
            LOG_DEBUG(" - dev%d, TX_EMPTY is not sent again! \n", dev->device_id);
        }
        /* Write again. Acutally, multiple CPUs update the same address may casue race condition
         * with brd->peer_tx_empty_status_kaddr
         */
        writel(dbell, dev->peer_tx_empty_status_kaddr);
    }
}

/*-----------------------------------------------------------------
 * buffer functions
 *-----------------------------------------------------------------
 */
/* Create a buffer entry and the buffer
 * length: real length without cache line alignment
 */
int zebra_buffer_entry_create(unsigned int length, buffer_entry_t **pentry, void *private)
{
	zebra_board_t  *board = (zebra_board_t *)private;
	zebra_driver_t *drv = board->driver;
	buffer_entry_t *entry;
	int alloc_len = CACHE_ALIGN(length);

	//LOG_DEBUG(" - kmem_cache_alloc (entry)\n");
	entry = kmem_cache_alloc(drv->buffer_cache, GFP_KERNEL);
	if (entry == NULL) {
		LOG_ERROR(" - buffer element cache allocation failed\n");
		return -ENOMEM;
	}

	atomic_inc(&drv->buffer_alloc);
	memset(entry, 0, sizeof(struct buffer_entry));
    entry->private = private;   /* board */
    INIT_LIST_HEAD(&entry->list);
    INIT_LIST_HEAD(&entry->pack_list);

	/* A read transaction copies pre-allocated buffers
	 * into the entry
	 */
	if (length == 0) {
		*pentry = entry;
		return 0;
	}

	/* allocate the body */
	if (IS_PCI_DEVICE(drv, board)) {
	    entry->databuf = (unsigned char *)pci_mem_alloc(alloc_len, CACHE_LINE_SZ, &entry->phy_addr,
	                    MEM_TYPE_NCNB);
	} else {
	    entry->databuf = (unsigned char *)mem_alloc(alloc_len, CACHE_LINE_SZ, &entry->phy_addr,
	                    MEM_TYPE_NCNB);
    }
	if (entry->databuf) {
	    if (entry->phy_addr & CACHE_LINE_MASK)
	        panic("%s, invalid phy_addr: 0x%x \n", __func__, entry->phy_addr);

	    atomic_inc(&drv->databuf_alloc);
	    entry->direct_databuf = __va(entry->phy_addr);
#ifdef WIN32
	    memset(entry->databuf, 0, alloc_len);
#endif
	}

	if (!entry->databuf) {
		LOG_ERROR(" - buffer entry page allocation failed, size:%d \n", alloc_len);
		//LOG_DEBUG(" - kmem_cache_free (entry)\n");
		kmem_cache_free(drv->buffer_cache, entry);
		atomic_dec(&drv->buffer_alloc);
		*pentry = NULL;
		return -ENOMEM;
	}

	entry->length = length;
	entry->length_keep = length;

	/* Return the entry */
	*pentry = entry;
	return 0;
}

/* Delete a buffer entry and buffer (if required) */
void zebra_buffer_entry_destroy(buffer_entry_t *entry)
{
    zebra_board_t  *board = (zebra_board_t *)entry->private;
	zebra_driver_t *drv = board->driver;

	if (entry->databuf) {
	    //LOG_DEBUG(" - free data buffer of entry \n");
	    if (IS_PCI_DEVICE(drv, board))
	        pci_mem_free(entry->databuf);
	    else
	        mem_free(entry->databuf);

	    entry->databuf = NULL;
		atomic_dec(&drv->databuf_alloc);
	}

	//LOG_DEBUG(" - kmem_cache_free (entry)\n");
	kmem_cache_free(drv->buffer_cache, entry);
	atomic_dec(&drv->buffer_alloc);

	entry = NULL;
}

/*-----------------------------------------------------------------
 * Work scheduling
 *-----------------------------------------------------------------
 */
void zebra_schedule_work(zebra_driver_t *drv, struct work_struct *work, zebra_device_t *dev)
{
    if ((u32)drv->work_queue > 0xF0000000) {
        printk("%s, drv->work_queue: 0x%p, drv: 0x%p \n", __func__, drv->work_queue, drv);
        dump_stack();
    }

    if ((u32)work > 0xF0000000) {
        printk("%s, work: 0x%p, \n", __func__, work);
        dump_stack();
    }

#ifdef USE_KTRHEAD
    if ((&dev->write_request == work) || (&dev->pack_write_request == work)) {
        dev->write_thread_event = 1;
        wake_up(&dev->write_thread_wait);
    } else if ((&dev->read_request == work) || (&dev->pack_read_request == work)) {
        dev->read_thread_event = 1;
        wake_up(&dev->read_thread_wait);
    } else {
        panic("%s, can't find match work! \n", __func__);
    }
#else
    queue_work(drv->work_queue, work);
#endif
}

/*-----------------------------------------------------------------
 * write work function
 *-----------------------------------------------------------------
 *
 * This function is scheduled by two cases:
 * 1) by upper application if write_busy semaphore is available.
 * 2) by interrupt. When sending data to the peer and getting tx_empty acknowledge, this function
 *      will be scheduled.
 * Three indices will be updated for the buffer index indexing the share memory:
 * 1)both rx_rdy_idex and tx_empty_idx of transmit_list
 * 2)rx_rdy_idx in the share memory.
 */
void zebra_write_workfn(struct work_struct *work)
{
	zebra_device_t *dev = container_of(work, zebra_device_t, write_request);
	zebra_board_t  *brd = dev->board;
	zebra_driver_t *drv = brd->driver;
	buffer_list_t *write_list = &dev->write_list;
	transmit_list_t *transmit_list = &dev->wr_transmit_list;
	int complete_length = 0, write_count, count, list_length;
	unsigned short buf_idx;
	buffer_entry_t *entry;
	share_memory_t  *sharemem;
    int fire_timer = 0, pack_idx;

	/* error checking:
	 * dev->write_busy semaphore is released when write_list is empty.
	 * when dev->write_busy is true, upper layer would not schedule this work queue.
	 */
	if (!down_trylock(&dev->write_busy)) {
#if 0 /* while dummy interrupt comes, it takes the write_busy semaphore and may cause ap NOT to
       * schedule work
       */
		up(&dev->write_busy);
		LOG_DEBUG(" - dev%d, <w_work> dev->write_busy is released, dummy interrupt! \n",
		                                                                dev->device_id);
		return;
#endif
	}

    /* Now check the write list for buffers */
	down(&write_list->sem);

    /* process the data had been acknowledged */
    zebra_write_done(dev);

    LOG_DEBUG("<%s, dev%d>\n", __func__, dev->device_id);

    /* reset to zero */
    complete_length = transmit_list->complete_length;
    transmit_list->complete_length = 0;

    /* release the space */
    write_list->list_length -= complete_length;
    if (write_list->list_length < 0)
        panic("%s, bug happens, write_list length = %d \n", __func__, write_list->list_length);

    /* Note:
	 * write_list->list_length includes transmit_list->list_length which is not acknowledged. So
	 * the real not transmitted packets in write_list->list_length are equal to
	 * write_list->list_length - transmit_list->list_length
	 */
	if (write_list->list_length < transmit_list->list_length)
		panic("%s, bug, write_list->list_length: %d, transmit_list->list_length: %d! \n",
		                __func__, write_list->list_length, transmit_list->list_length);

	list_length = write_list->list_length - transmit_list->list_length;

	LOG_DEBUG(" - dev%d, <w_work> write_list len: %d, transmit_len:%d, diff: %d \n",
			    dev->device_id, write_list->list_length, transmit_list->list_length, list_length);

    /* all data packets are sent out and acknowledged already. */
    if (write_list->list_length == 0) {
        if (unlikely(transmit_list->list_length))
            panic("%s, bug, transmit_list->list_length:%d is not zero! \n", __func__,
                    transmit_list->list_length);

        /* all packets are acknowledged, so this work will never be scheduled again */
        LOG_DEBUG(" - dev%d, <w_work> no buffers in the write-list and exit\n", dev->device_id);

		/* Wake-up processes blocked in fsync() */
		if (dev->fsync_requested) {
			dev->fsync_requested = 0;
			LOG_DEBUG(" - dev%d, wake-up fsync\n", dev->device_id);
			if (dev->device_id >= ZEBRA_DEVICE_AP_START)
			    wake_up_interruptible(&dev->fsync_wait);
			else
			    wake_up(&dev->fsync_wait);
		}

		/* Please be aware that:
		 * dev->write_busy is not released until write_list->list_length is empty. When the upper
		 * layer found write_busy is released, it will schedule this work queue.
		 */
		up(&dev->write_busy);
	    /* release the semaphore */
        up(&write_list->sem);
        goto exit;
    }

    /*
     * write_list is not empty.
     */

	/* start to do the transaction */
    sharemem = (share_memory_t *)dev->write_kaddr;

	/* calculate how many available space to write packets */
	write_count = BUF_SPACE(write_list->list_length_max, transmit_list->rx_rdy_idx,
	                                                    transmit_list->tx_empty_idx);
	/* take the min value.
	 * write_count may be zero because all data was transmitted already, and we are waiting the ack.
	 */
	write_count = min(write_count, list_length);
	LOG_DEBUG(" - dev%d, <w_work> will transmitted:%d to the peer \n", dev->device_id,
	                                                                            write_count);

	LOG_DEBUG(" - dev%d, <w_work> BEFORE transmit(rx_rdy:%d, tx_emtpy:%d) \n",
	                dev->device_id, transmit_list->rx_rdy_idx, transmit_list->tx_empty_idx);
    LOG_DEBUG(" - dev%d, <w_work> BEFORE sharemem(rx_rdy:%d, tx_empty:%d) \n",
	            dev->device_id, sharemem->hdr.rx_rdy_idx, sharemem->hdr.tx_empty_idx);

	count = 0;
	while (write_count --) {
	    /* Remove a write-list buffer and add it to transmitting list.
	     * Note:
	     * write_list->list_length is not decreased until tx_empty is received
	     */
	    if (list_empty(&write_list->list))
	        panic("%s, write_list is empty! \n", __func__);

	    entry = list_entry(write_list->list.prev, buffer_entry_t, list);
	    list_del(write_list->list.prev);
	    if (entry->length > write_list->buffer_length_max) {
	        panic("%s(master), write length: %d exceeds SDRAM size: %d! \n", __func__,
	            entry->length, write_list->buffer_length_max);
	    }

        /* aware that: copy the physical address to the share memory instead of body.
         * Note: the type of buf_idx must be same as rx_rdy_idx. It should be unsigned short.
         */
        buf_idx = GET_BUFIDX(write_list->list_length_max, transmit_list->rx_rdy_idx);
        if (IS_PCI(brd)) {
            /* give the peer about the src address */
            sharemem->mem_entry[buf_idx][0].paddr = IS_PCI_HOST(drv) ?
                    TO_PCI_ADDR((dma_addr_t)entry->phy_addr, drv, brd) : (dma_addr_t)entry->phy_addr;
        } else {
            sharemem->mem_entry[buf_idx][0].paddr = (dma_addr_t)entry->phy_addr;
        }

		sharemem->mem_entry[buf_idx][0].buf_len = entry->length; /* real data length */

		for (pack_idx = 1; pack_idx < ZEBRA_NUM_IN_PACK; pack_idx ++) {
            sharemem->mem_entry[buf_idx][pack_idx].paddr = 0xFFFFFFFF;
            sharemem->mem_entry[buf_idx][pack_idx].buf_len = 0; //tell the termination
        }

#if 0
		LOG_DEBUG(" - dev%d, copy PADDR into share memory with index: %d\n", dev->device_id, buf_idx);
#endif
        /* keep the entry on hand until the peer get the data */
        INIT_LIST_HEAD(&entry->list);

        list_add(&entry->list, &transmit_list->list);
	    transmit_list->list_length ++;
	    transmit_list->rx_rdy_idx ++;

        count ++;
    };

    /* release the semaphore.
     * The upper application can write data into write_list now.
     */
    up(&write_list->sem);

    /* something is added to transmitting list */
    if (count != 0) {
		/* update to share memory */
        sharemem->hdr.rx_rdy_idx = transmit_list->rx_rdy_idx;
        LOG_DEBUG(" - dev%d, <w_work> copy %d entries into share memory \n", dev->device_id, count);
    	LOG_DEBUG(" - dev%d, <w_work> AFTER write_list: %d, transmit_list: %d \n", dev->device_id,
    	            write_list->list_length, transmit_list->list_length);
    	LOG_DEBUG(" - dev%d, <w_work> AFTER transmitting(rx_rdy: %d, tx_emtpy: %d) \n",
    	            dev->device_id, transmit_list->rx_rdy_idx, transmit_list->tx_empty_idx);

    	LOG_DEBUG(" - dev%d, <w_work> AFTER sharemem(rx_rdy: %d, tx_empty: %d) \n",
    	            dev->device_id, sharemem->hdr.rx_rdy_idx, sharemem->hdr.tx_empty_idx);

    	if (!timer_pending(&dev->write_timeout)) {
    		LOG_DEBUG(" - dev%d, setup TX_EMPTY acknowledge timeout function\n", dev->device_id);
    		dev->write_timeout.function = zebra_write_timeout_handler;
    		dev->write_timeout.data = (unsigned long)dev;
    		mod_timer(&dev->write_timeout, jiffies + WAIT_TXEMPTY_TIMEOUT * HZ); /* 20 seconds */
    		fire_timer = 1;
    	} else {
    	    LOG_ERROR(" - dev%d, timer should not running! \n", dev->device_id);
    	    panic("%s, dev%d, timer bug happens. \n", __func__, dev->device_id);
    	}

        /* inform the peer about the data coming */
        zebra_write_device_doorbell(dev, dev->rx_ready, RX_READY);
    } else {
        LOG_DEBUG(" - dev%d, <w_work> AFTER transmitting(rx_rdy: %d, tx_emtpy: %d) \n",
	                dev->device_id, transmit_list->rx_rdy_idx, transmit_list->tx_empty_idx);
        LOG_DEBUG(" - dev%d, <w_work> AFTER sharemem(rx_rdy: %d, tx_empty: %d) \n",
	                dev->device_id, sharemem->hdr.rx_rdy_idx, sharemem->hdr.tx_empty_idx);
    }

exit:
    /* Wake up any writers, as there is space there now */
    if (complete_length) {
    	dev->write_event = 1;
    	if (dev->device_id >= ZEBRA_DEVICE_AP_START)
	        wake_up_interruptible(&write_list->wait);
	    else
    	    wake_up(&write_list->wait);
    }

    /* if the transmitting list is not empty, we need to setup the timeout timer */
    if (transmit_list->list_length && !fire_timer) {
        LOG_DEBUG(" - dev%d, setup TX_EMPTY acknowledge timeout function\n", dev->device_id);
		dev->write_timeout.function = zebra_write_timeout_handler;
		dev->write_timeout.data = (unsigned long)dev;
		mod_timer(&dev->write_timeout, jiffies + WAIT_TXEMPTY_TIMEOUT * HZ); /* 20 seconds */
    }

    dev->pktwr_out_jiffies = (u32)jiffies;
    LOG_DEBUG("<%s, dev%d> exit \n", __func__, dev->device_id);
}

void zebra_pack_write_workfn(struct work_struct *work)
{
	zebra_device_t *dev = container_of(work, zebra_device_t, pack_write_request);
	zebra_board_t  *brd = dev->board;
	zebra_driver_t *drv = brd->driver;
	buffer_list_t *write_list = &dev->write_list;
	transmit_list_t *transmit_list = &dev->wr_transmit_list;
	int complete_length = 0, write_count, count, list_length;
	unsigned short buf_idx;
	buffer_entry_t *entry, *pack_entry;
	share_memory_t  *sharemem;
	int fire_timer = 0, pack_idx;

	/* error checking:
	 * dev->write_busy semaphore is released when write_list is empty.
	 * when dev->write_busy is true, upper layer would not schedule this work queue.
	 */
	if (!down_trylock(&dev->write_busy)) {
#if 0 /* while dummy interrupt comes, it takes the write_busy semaphore and may cause ap NOT to
       * schedule work
       */
		up(&dev->write_busy);
		LOG_DEBUG(" - dev%d, <w_work> dev->write_busy is released, dummy interrupt! \n", dev->device_id);
		return;
#endif
	}

    /* Now check the write list for buffers */
	down(&write_list->sem);

    /* process the data had been acknowledged */
    zebra_write_done(dev);

    LOG_DEBUG("<%s, dev%d>\n", __func__, dev->device_id);

    /* reset to zero */
    complete_length = transmit_list->complete_length;
    transmit_list->complete_length = 0;

    /* release the space */
    write_list->list_length -= complete_length;
    if (write_list->list_length < 0)
        panic("%s, bug happens, write_list length = %d \n", __func__, write_list->list_length);

    /* Note:
	 * write_list->list_length includes transmit_list->list_length which is not acknowledged. So
	 * the real not transmitted packets in write_list->list_length are equal to
	 * write_list->list_length - transmit_list->list_length
	 */
	if (write_list->list_length < transmit_list->list_length)
		panic("%s, bug, write_list->list_length: %d, transmit_list->list_length: %d! \n",
		                __func__, write_list->list_length, transmit_list->list_length);

	list_length = write_list->list_length - transmit_list->list_length;

	LOG_DEBUG(" - dev%d, <w_work> write_list len: %d, transmit_len:%d, diff: %d \n",
			    dev->device_id, write_list->list_length, transmit_list->list_length, list_length);

    /* all data packets are sent out and acknowledged already. */
    if (write_list->list_length == 0) {
        if (unlikely(transmit_list->list_length))
            panic("%s, bug, transmit_list->list_length:%d is not zero! \n", __func__,
                    transmit_list->list_length);

        /* all packets are acknowledged, so this work will never be scheduled again */
        LOG_DEBUG(" - dev%d, <w_work> no buffers in the write-list and exit\n", dev->device_id);

		/* Wake-up processes blocked in fsync() */
		if (dev->fsync_requested) {
			dev->fsync_requested = 0;
			LOG_DEBUG(" - dev%d, wake-up fsync\n", dev->device_id);
			if (dev->device_id >= ZEBRA_DEVICE_AP_START)
			    wake_up_interruptible(&dev->fsync_wait);
			else
			    wake_up(&dev->fsync_wait);
		}

		/* Please be aware that:
		 * dev->write_busy is not released until write_list->list_length is empty. When the upper
		 * layer found write_busy is released, it will schedule this work queue.
		 */
		up(&dev->write_busy);
	    /* release the semaphore */
        up(&write_list->sem);
        goto exit;
    }

    /*
     * write_list is not empty.
     */

	/* start to do the transaction */
    sharemem = (share_memory_t *)dev->write_kaddr;

	/* calculate how many available space to write packets */
	write_count = BUF_SPACE(write_list->list_length_max, transmit_list->rx_rdy_idx,
	                                                    transmit_list->tx_empty_idx);
	/* take the min value.
	 * write_count may be zero because all data was transmitted already, and we are waiting the ack.
	 */
	write_count = min(write_count, list_length);
	LOG_DEBUG(" - dev%d, <w_work> will transmitted:%d to the peer \n", dev->device_id,
	                                                                            write_count);

	LOG_DEBUG(" - dev%d, <w_work> BEFORE transmit(rx_rdy:%d, tx_emtpy:%d) \n",
	                dev->device_id, transmit_list->rx_rdy_idx, transmit_list->tx_empty_idx);
    LOG_DEBUG(" - dev%d, <w_work> BEFORE sharemem(rx_rdy:%d, tx_empty:%d) \n",
	            dev->device_id, sharemem->hdr.rx_rdy_idx, sharemem->hdr.tx_empty_idx);

	count = 0;
	while (write_count --) {
	    /* Remove a write-list buffer and add it to transmitting list.
	     * Note:
	     * write_list->list_length is not decreased until tx_empty is received.
	     */
	    if (list_empty(&write_list->list))
	        panic("%s, write_list is empty! \n", __func__);

	    entry = list_entry(write_list->list.prev, buffer_entry_t, list);
	    list_del(write_list->list.prev);
	    if (entry->length > write_list->buffer_length_max) {
	        panic("%s(master), write length: %d exceeds SDRAM size: %d! \n", __func__,
	            entry->length, write_list->buffer_length_max);
	    }

        /* aware that: copy the physical address to the share memory instead of body.
         * Note: the type of buf_idx must be same as rx_rdy_idx. It should be unsigned short.
         */
        buf_idx = GET_BUFIDX(write_list->list_length_max, transmit_list->rx_rdy_idx);

		/* first entry */
		if (IS_PCI(brd)) {
		    /* give the peer about the src address */
            sharemem->mem_entry[buf_idx][0].paddr = IS_PCI_HOST(drv) ?
                    TO_PCI_ADDR((dma_addr_t)entry->phy_addr, drv, brd) : (dma_addr_t)entry->phy_addr;
        } else {
            sharemem->mem_entry[buf_idx][0].paddr = (dma_addr_t)entry->phy_addr;
        }
		sharemem->mem_entry[buf_idx][0].buf_len = entry->length;
		/* pack entry if exist */
        pack_idx = 1;
        list_for_each_entry(pack_entry, &entry->pack_list, pack_list) {
            if (IS_PCI(brd)) {
                /* give the peer about the src address */
                sharemem->mem_entry[buf_idx][pack_idx].paddr = IS_PCI_HOST(drv) ?
                                TO_PCI_ADDR((dma_addr_t)pack_entry->phy_addr, drv, brd) :
                                (dma_addr_t)pack_entry->phy_addr;
            } else {
                sharemem->mem_entry[buf_idx][pack_idx].paddr = (dma_addr_t)pack_entry->phy_addr;
            }
            sharemem->mem_entry[buf_idx][pack_idx].buf_len = (dma_addr_t)pack_entry->length;
            pack_idx ++;
        }

        for (;pack_idx < ZEBRA_NUM_IN_PACK; pack_idx ++) {
            sharemem->mem_entry[buf_idx][pack_idx].paddr = 0xFFFFFFFF;
            sharemem->mem_entry[buf_idx][pack_idx].buf_len = 0; //tell the termination
        }

        /* keep the entry on hand until the peer get the data */
        INIT_LIST_HEAD(&entry->list);

        /* save irq for transmitting list */
	    //spin_lock_irqsave(&transmit_list->lock, flags);
        list_add(&entry->list, &transmit_list->list);
	    transmit_list->list_length ++;
	    transmit_list->rx_rdy_idx ++;

        count ++;
    };

    /* release the semaphore.
     * The upper application can write data into write_list now.
     */
    up(&write_list->sem);

    /* something is added to transmitting list */
    if (count != 0) {
		/* update to share memory */
        sharemem->hdr.rx_rdy_idx = transmit_list->rx_rdy_idx;

        LOG_DEBUG(" - dev%d, <w_work> copy %d entries into share memory \n", dev->device_id, count);
    	LOG_DEBUG(" - dev%d, <w_work> AFTER write_list: %d, transmit_list: %d \n", dev->device_id,
    	            write_list->list_length, transmit_list->list_length);
    	LOG_DEBUG(" - dev%d, <w_work> AFTER transmitting(rx_rdy: %d, tx_emtpy: %d) \n",
    	            dev->device_id, transmit_list->rx_rdy_idx, transmit_list->tx_empty_idx);

    	LOG_DEBUG(" - dev%d, <w_work> AFTER sharemem(rx_rdy: %d, tx_empty: %d) \n",
    	            dev->device_id, sharemem->hdr.rx_rdy_idx, sharemem->hdr.tx_empty_idx);

    	if (!timer_pending(&dev->write_timeout)) {
    		LOG_DEBUG(" - dev%d, setup TX_EMPTY acknowledge timeout function\n", dev->device_id);
    		dev->write_timeout.function = zebra_write_timeout_handler;
    		dev->write_timeout.data = (unsigned long)dev;
    		mod_timer(&dev->write_timeout, jiffies + WAIT_TXEMPTY_TIMEOUT * HZ); /* 20 seconds */
    		fire_timer = 1;
    	} else {
    	    LOG_ERROR(" - dev%d, timer should not running! \n", dev->device_id);
    	    panic("%s, dev%d, timer bug happens. \n", __func__, dev->device_id);
    	}

        /* inform the peer about the data coming */
        zebra_write_device_doorbell(dev, dev->rx_ready, RX_READY);
    } else {
        LOG_DEBUG(" - dev%d, <w_work> AFTER transmitting(rx_rdy: %d, tx_emtpy: %d) \n",
	                dev->device_id, transmit_list->rx_rdy_idx, transmit_list->tx_empty_idx);
        LOG_DEBUG(" - dev%d, <w_work> AFTER sharemem(rx_rdy: %d, tx_empty: %d) \n",
	                dev->device_id, sharemem->hdr.rx_rdy_idx, sharemem->hdr.tx_empty_idx);
    }

exit:
    /* Wake up any writers, as there is space there now */
    if (complete_length) {
    	dev->write_event = 1;
    	if (dev->device_id >= ZEBRA_DEVICE_AP_START)
    	    wake_up_interruptible(&write_list->wait);
    	else
    	    wake_up(&write_list->wait);
    }

    /* if the transmitting list is not empty, we need to setup the timeout timer */
    if (transmit_list->list_length && !fire_timer) {
        LOG_DEBUG(" - dev%d, setup TX_EMPTY acknowledge timeout function\n", dev->device_id);
		dev->write_timeout.function = zebra_write_timeout_handler;
		dev->write_timeout.data = (unsigned long)dev;
		mod_timer(&dev->write_timeout, jiffies + WAIT_TXEMPTY_TIMEOUT * HZ); /* 20 seconds */
    }

    dev->pktwr_out_jiffies = (u32)jiffies;
    LOG_DEBUG("<%s, dev%d> exit \n", __func__, dev->device_id);
}

/*-----------------------------------------------------------------
 * Read && callbacks
 *-----------------------------------------------------------------
 */
/* Read transaction request
 *
 * This work function is scheduled by the IRQ handler when an RX_READY
 * interrupt is received for a device, indicating that the DEVICE
 * has written data to SDRAM for the local to read.
 *
 * The length of the read transaction is written by the DEVICE to a
 * device specific location in SDRAM (given by two indices: rx_rdy_idx and tx_empty_idx).
 * Please see the definition in zebra.h of share_memory_t structure.
 *
 * This function is called due to an RX_READY interrupt. The
 * peer -> local protocol requires that the DSP not send another
 * RX_READY interrupt until the TX_EMPTY acknowledge has been
 * sent. The dev->read_busy semaphore acts as a two state state-machine,
 * with states IDLE (unlocked) and BUSY (locked). The transition from
 * IDLE to BUSY occurs with the receipt of an RX_READY and returns
 * to IDLE when TX_EMPTY is sent (which may be held off until a read
 * occurs if the read buffer list fills). If an RX_READY is
 * received while in the BUSY state, it is a protocol error.
 */

/* READ timeout timer handler
 *
 * If the applications doesn't recevice the data within 10second,
 * this handler will pop out warning message on console to warn the
 * users.
 */
void zebra_read_timeout_handler(unsigned long data)
{
	zebra_device_t *dev = (zebra_device_t *)data;
    buffer_list_t *read_list = &dev->read_list;

	LOG_ERROR("<read_timeout, dev%d>\n", dev->device_id);
	LOG_ERROR(" - @@@ BAD! dev%d of board: %s, the application doesn't read data over %ds!\n",
	    dev->device_id, dev->board->name, APP_READ_TIMEOUT);

    //print debug message
	zebra_debug_print(dev->board);

	/* schedule the timeout again */
	down(&read_list->sem);
    if ((read_list->list_length != 0) && !timer_pending(&dev->read_timeout)) {
        LOG_DEBUG(" - dev%d, setup READ timeout function again\n", dev->device_id);
    	dev->read_timeout.function = zebra_read_timeout_handler;
    	dev->read_timeout.data = (unsigned long)dev;
    	mod_timer(&dev->read_timeout, jiffies + APP_READ_TIMEOUT * HZ); /* 20 seconds */
    }
	up(&read_list->sem);
}

/*
 * When receiving an acknowledge from the peer, this is the first entry point.
 * When this work receives the data, it just link to read_list without allocating a buffer.
 * So the tx_empty is sent by upper layer, instead of this work function.
 */
void zebra_read_workfn(struct work_struct *work)
{
    zebra_device_t *dev = container_of(work, zebra_device_t, read_request);
	zebra_board_t *brd = dev->board;
	zebra_driver_t *drv = brd->driver;
	transmit_list_t *transmit_list = &dev->rd_transmit_list;
	buffer_list_t *read_list = &dev->read_list;
	share_memory_t  *sharemem;
	buffer_entry_t *entry;
	int status, read_burst, space;
	unsigned short buf_idx;
	unsigned int   two_idx;

	LOG_DEBUG("<%s, dev%d>\n", __func__, dev->device_id);

	/* The first time the read request is scheduled, the lock
	 * will be acquired. If another read request is scheduled,
	 * then the attempt to lock will fail (return non-zero).
	 *
	 * Note: Before TX_EMPTY is been sent, dev->ready_busy is not cleared.
	 * When the peer receives this interrupt, then it can start to schedule the next
	 * transmission.
	 *
	 * transmit_list->tx_empty_idx is updated by upper layer. And transmit_list->rx_rdy_idx
	 * is only updated here.
	 */
    if (down_trylock(&dev->read_busy)) {
	    /* lock semaphore fails.
		 * re-entry protection. That is it shouldn't two works be scheduled.
		 */
		LOG_ERROR(" - ERROR: dev%d, <r_work> read_busy is true, drop data!\n", dev->device_id);
		panic("%s, dev%d, bug in dev->read_busy semaphore! \n", __func__, dev->device_id);
		brd->internal_error ++;
		return;
	}

    dev->pktrd_in_jiffies = (u32)jiffies;

    down(&read_list->sem);

    LOG_DEBUG(" - dev%d, <r_work> BEFORE => transmit_list(rx_rdy: %d, tx_empty: %d) \n", dev->device_id,
                    transmit_list->rx_rdy_idx, transmit_list->tx_empty_idx);

    /* get from the share memory */
    sharemem = (share_memory_t *)dev->read_kaddr;
    two_idx = sharemem->hdr.two_idx;

    read_burst = BUF_COUNT(GET_RXRDY_IDX(two_idx), transmit_list->rx_rdy_idx);
    if (read_burst == 0) {
        /* The case:
         * zebra_write_workfn of the peer is executed twice before this work being executed.
         * And it ringed the RX_RDY interrupts even it doesn't have nothing to transmit(its
         * transmitting list is not empty but no new data in write_list.
         */
        LOG_DEBUG(" - dev%d, <r_work> the read burst is 0. \n", dev->device_id);
        up(&read_list->sem);
        goto exit;
    }
    if (read_burst < 0)
        panic("%s, bug happen, read burst < 0 \n", __func__);

    space = read_list->list_length_max - read_list->list_length;
    if (read_burst > space)
        panic("%s, <r_work> the read burst:%d is smaller than space:%d. \n", __func__,
                                                                    read_burst, space);
#if 0
    /* Drop data if there is no reader */
	if (!dev->open_read) {
		LOG_ERROR(" - dev%d, <r_work> no reader, dropping all data\n", dev->device_id);
		while (read_burst -- ){
		    dev->counter.dropped ++;
            transmit_list->rx_rdy_idx ++;
            transmit_list->tx_empty_idx ++; //indicate this data has been read out
        }
        /* update share memory */
        sharemem->hdr.tx_empty_idx = transmit_list->tx_empty_idx;
        LOG_DEBUG(" - dev%d, <r_work> sharemem(rx_rdy: %d, tx_empty: %d) \n", dev->device_id,
                            sharemem->hdr.rx_rdy_idx, sharemem->hdr.tx_empty_idx);
        up(&read_list->sem);

		/* Acknowledge it */
		zebra_write_device_doorbell(dev, dev->tx_empty, TX_EMPTY);
		goto exit;
	}
#endif

    LOG_DEBUG(" - dev%d, <r_work> the read burst is %d. \n", dev->device_id, read_burst);

    while (read_burst --) {
        buf_idx = GET_BUFIDX(read_list->list_length_max, transmit_list->rx_rdy_idx);

        /* create buffer entry */
	    status = zebra_buffer_entry_create(0, &entry, brd);
	    if (status < 0) {
    		LOG_ERROR(" - (read) buffer entry creation failed\n");
    		up(&read_list->sem);

    		goto exit;
	    }
        /* update to real length */
        entry->length = sharemem->mem_entry[buf_idx][0].buf_len;
        entry->length_keep = entry->length;
        if (IS_PCI(brd)) {
            entry->phy_addr = IS_PCI_HOST(drv) ?
                    TO_AXI_ADDR(sharemem->mem_entry[buf_idx][0].paddr, drv, brd) :
                    sharemem->mem_entry[buf_idx][0].paddr;
        } else {
            entry->phy_addr = sharemem->mem_entry[buf_idx][0].paddr;
        }

        /* Add the entry to the read-list
         */
    	list_add(&entry->list, &read_list->list);
    	read_list->list_length ++;

	    /* move to next slot */
	    transmit_list->rx_rdy_idx ++;
    }

    /* wake up the reader */
    dev->read_event = 1;
    if (dev->device_id >= ZEBRA_DEVICE_AP_START)
        wake_up_interruptible(&read_list->wait);
    else
        wake_up(&read_list->wait);

    if (!timer_pending(&dev->read_timeout)) {
        LOG_DEBUG(" - dev%d, setup READ timeout function\n", dev->device_id);
		dev->read_timeout.function = zebra_read_timeout_handler;
		dev->read_timeout.data = (unsigned long)dev;
		mod_timer(&dev->read_timeout, jiffies + APP_READ_TIMEOUT * HZ); /* 20 seconds */
    }

    up(&read_list->sem);

    LOG_DEBUG(" - dev%d, <r_work> AFTER => transmit_list(rx_rdy: %d, tx_empty: %d) \n", dev->device_id,
                    transmit_list->rx_rdy_idx, transmit_list->tx_empty_idx);
    LOG_DEBUG(" - dev%d, <r_work> AFTER => sharemem(rx_rdy: %d, tx_empty: %d) \n", dev->device_id,
                    GET_RXRDY_IDX(two_idx), GET_TXEMPTY_IDX(two_idx));

exit:
	/* upper layer will send tx_empty if all entries are read out.
	 */
    LOG_DEBUG("<%s, dev%d> exit\n", __func__, dev->device_id);

    /* Allow read requests to be scheduled */
    up(&dev->read_busy);

    return;
}

void zebra_pack_read_workfn(struct work_struct *work)
{
    zebra_device_t *dev = container_of(work, zebra_device_t, pack_read_request);
	zebra_board_t *brd = dev->board;
	zebra_driver_t *drv = brd->driver;
	transmit_list_t *transmit_list = &dev->rd_transmit_list;
	buffer_list_t *read_list = &dev->read_list;
	share_memory_t  *sharemem;
	buffer_entry_t *entry = NULL, *pack_entry;
	int status, read_burst, space;
	unsigned short buf_idx;
	unsigned int   two_idx, pack_idx;

	LOG_DEBUG("<%s, dev%d>\n", __func__, dev->device_id);

	/* The first time the read request is scheduled, the lock
	 * will be acquired. If another read request is scheduled,
	 * then the attempt to lock will fail (return non-zero).
	 *
	 * Note: Before TX_EMPTY is been sent, dev->ready_busy is not cleared.
	 * When the peer receives this interrupt, then it can start to schedule the next
	 * transmission.
	 *
	 * transmit_list->tx_empty_idx is updated by upper layer. And transmit_list->rx_rdy_idx
	 * is only updated here.
	 */
    if (down_trylock(&dev->read_busy)) {
	    /* lock semaphore fails.
		 * re-entry protection. That is it shouldn't two works be scheduled.
		 */
		LOG_ERROR(" - ERROR: %dev, <r_work> read_busy is true, drop data!\n", dev->device_id);
		panic("%s, bug in dev->read_busy semaphore! \n", __func__);
		brd->internal_error ++;
		return;
	}

    dev->pktrd_in_jiffies = (u32)jiffies;

    down(&read_list->sem);

    LOG_DEBUG(" - dev%d, <r_work> BEFORE => transmit_list(rx_rdy: %d, tx_empty: %d) \n", dev->device_id,
                    transmit_list->rx_rdy_idx, transmit_list->tx_empty_idx);

    /* get from the share memory */
    sharemem = (share_memory_t *)dev->read_kaddr;
    two_idx = sharemem->hdr.two_idx;

    read_burst = BUF_COUNT(GET_RXRDY_IDX(two_idx), transmit_list->rx_rdy_idx);
    if (read_burst == 0) {
        /* The case:
         * zebra_write_workfn of the peer is executed twice before this work being executed.
         * And it ringed the RX_RDY interrupts even it doesn't have nothing to transmit(its
         * transmitting list is not empty but no new data in write_list.
         */
        LOG_DEBUG(" - dev%d, <r_work> the read burst is 0. \n", dev->device_id);
        up(&read_list->sem);
        goto exit;
    }
    if (read_burst < 0)
        panic("%s, bug happen, read_burst < 0 \n", __func__);

    space = read_list->list_length_max - read_list->list_length;
    if (read_burst > space)
        panic("%s, <r_work> the read burst:%d is smaller than space:%d. \n", __func__,
                                                                        read_burst, space);
#if 0
    /* Drop data if there is no reader */
	if (!dev->open_read) {
		LOG_ERROR(" - dev%d, <r_work> no reader, dropping all data\n", dev->device_id);
		while (read_burst -- ){
		    dev->counter.dropped ++;
            transmit_list->rx_rdy_idx ++;
            transmit_list->tx_empty_idx ++; //indicate this data has been read out
        }
        /* update share memory */
        sharemem->hdr.tx_empty_idx = transmit_list->tx_empty_idx;
        LOG_DEBUG(" - dev%d, <r_work> sharemem(rx_rdy: %d, tx_empty: %d) \n", dev->device_id,
                                    sharemem->hdr.rx_rdy_idx, sharemem->hdr.tx_empty_idx);
        up(&read_list->sem);

		/* Acknowledge it */
		zebra_write_device_doorbell(dev, dev->tx_empty, TX_EMPTY);
		goto exit;
	}
#endif

    LOG_DEBUG(" - dev%d, <r_work>, the read burst is %d. \n", dev->device_id, read_burst);

    while (read_burst --) {
        buf_idx = GET_BUFIDX(read_list->list_length_max, transmit_list->rx_rdy_idx);

        for (pack_idx = 0; pack_idx < ZEBRA_NUM_IN_PACK; pack_idx ++) {
            if ((sharemem->mem_entry[buf_idx][pack_idx].buf_len == 0) ||
                                    (sharemem->mem_entry[buf_idx][pack_idx].paddr == 0xFFFFFFFF))
                break;

            /* create buffer entry */
    	    status = zebra_buffer_entry_create(0, &pack_entry, brd);
    	    if (status < 0) {
        		LOG_ERROR(" - (read) buffer entry creation failed\n");
        		panic("%s, (read) buffer entry creation failed\n", __func__);
        		up(&read_list->sem);

        		goto exit;
    	    }

            /* copy physical address of remote buffer to the entry->databuf and pass to upper layer.
             * Note: Here we will have endian problem. But bus and cpu are little endian. So we don't
             *      need to consider endian problem.
             */
            if (IS_PCI(brd)) {
                pack_entry->phy_addr = IS_PCI_HOST(drv) ?
                        TO_AXI_ADDR(sharemem->mem_entry[buf_idx][pack_idx].paddr, drv, brd) :
                        sharemem->mem_entry[buf_idx][pack_idx].paddr;
            } else {
    	        pack_entry->phy_addr = sharemem->mem_entry[buf_idx][pack_idx].paddr;
    	    }
    	    /* update to real length */
    	    pack_entry->length = sharemem->mem_entry[buf_idx][pack_idx].buf_len;
            pack_entry->length_keep = sharemem->mem_entry[buf_idx][pack_idx].buf_len;

            /* read from sharemem again */
            if ((sharemem->mem_entry[buf_idx][pack_idx].buf_len == 0) ||
                                    (sharemem->mem_entry[buf_idx][pack_idx].paddr == 0xFFFFFFFF)) {
                printk("%s, DDR data sync problem! \n", __func__);
                zebra_buffer_entry_destroy(pack_entry);
                break;
            }

    	    if (pack_idx == 0)
                entry = pack_entry;
            else
    	        list_add_tail(&pack_entry->pack_list, &entry->pack_list);
        }
        /* Add the entry to the read-list
         */
    	list_add(&entry->list, &read_list->list);
    	read_list->list_length ++;
	    /* move to next slot */
	    transmit_list->rx_rdy_idx ++;
    }

	dev->read_event = 1;
	if (dev->device_id >= ZEBRA_DEVICE_AP_START)
	    wake_up_interruptible(&read_list->wait);
	else
	    wake_up(&read_list->wait);

    LOG_DEBUG(" - dev%d, <r_work> AFTER => transmit_list(rx_rdy: %d, tx_empty: %d) \n", dev->device_id,
                    transmit_list->rx_rdy_idx, transmit_list->tx_empty_idx);
    LOG_DEBUG(" - dev%d, <r_work> AFTER => sharemem(rx_rdy: %d, tx_empty: %d) \n", dev->device_id,
                    GET_RXRDY_IDX(two_idx), GET_TXEMPTY_IDX(two_idx));

    if (!timer_pending(&dev->read_timeout)) {
        LOG_DEBUG(" - dev%d, setup READ timeout function\n", dev->device_id);
		dev->read_timeout.function = zebra_read_timeout_handler;
		dev->read_timeout.data = (unsigned long)dev;
		mod_timer(&dev->read_timeout, jiffies + APP_READ_TIMEOUT * HZ); /* 20 seconds */
    }
	up(&read_list->sem);
exit:
	/* upper layer will send tx_empty if all entries are read out.
	 */
    LOG_DEBUG("<%s, dev%d> exit\n", __func__, dev->device_id);

    /* Allow read requests to be scheduled */
    up(&dev->read_busy);

    return;
}

/*
 * This function processes the transmitting list. It will calculate how many data had been
 * processed by the peer. Those processed data will be released in this function.
 * return:
 *      number of complete count
 */
static inline void zebra_write_done(void *data)
{
    zebra_device_t *dev = (zebra_device_t *)data;
    transmit_list_t *transmit_list = &dev->wr_transmit_list;
    buffer_entry_t  *entry, *pack_entry, *ne;
    unsigned int	two_idx, count;
    share_memory_t  *sharemem;

    LOG_DEBUG("<zebra_write_done, dev:%d> \n", dev->device_id);

    /* Free the transmitting-list entry.
     * If transmit_list->list is empty, it means nothing was transmitted.
     * The case is possible because the peer keeps updating its tx_empty_idx of sharemem when it
     *  sending the ex_empty interrupt.
     */
    if (list_empty(&transmit_list->list)) {
        LOG_DEBUG("<zebra_write_done, dev:%d, transmit_list empty> exit \n", dev->device_id);
        if (timer_pending(&dev->write_timeout))
            panic("%s, timer bug in the code! \n", __func__);
        return;
    }

    /* Cancel the timeout. If the transmitting list is not empty, we must have started the timer.
     */
	if (timer_pending(&dev->write_timeout)) {
		LOG_DEBUG(" - dev%d, cancel the timer\n", dev->device_id);
		del_timer(&dev->write_timeout);
	} else {
	    LOG_ERROR(" - dev%d, timer is not running. It should be bug! \n", dev->device_id);
	    panic("%s, timer bug! \n", __func__);
	}

	/* start to do the transaction */
    sharemem = (share_memory_t *)dev->write_kaddr;
    two_idx = sharemem->hdr.two_idx;

    /* calculate how manys entries are acknowledged
     * Consideration: The timing issue:
     * 		During the interrupt processing, the peer may still update its tx_empty_idx in the
     *      share memory. So it may have the chance that count is zero.
     */
    count = BUF_COUNT(GET_TXEMPTY_IDX(two_idx), transmit_list->tx_empty_idx);
    if (count > dev->write_list.list_length_max)
    	panic("%s, wrong value: %d \n", __func__, count);

	LOG_DEBUG(" - dev:%d, return complete count: %d \n", dev->device_id, count);

	if (count == 0) {
	    /* Tricky: it may enter here, because
	     * zebra_write_workfn was scheduled during the work function is executing. Thus, this
	     *    time of woken up is because of os scheduling instead of interrupt coming. This may
	     *    cause this work to be executed twice before zebra_read_workfn be executed of the peer.
	     *    So transmitting list not empty and return count = 0 may happen.
	     */
	    LOG_DEBUG("<zebra_write_done, dev:%d, count=0> exit \n", dev->device_id);
		return;
	}

	/* increase the counter */
    dev->counter.write_ack += count;

    /* update to new */
	transmit_list->tx_empty_idx = GET_TXEMPTY_IDX(two_idx);
    /* free the transmitted entry one by one */
    do {
        if (list_empty(&transmit_list->list))
	        panic("%s, transmit_list is empty! \n", __func__);

        entry = list_entry(transmit_list->list.prev, buffer_entry_t, list);
	    list_del(transmit_list->list.prev);

	    /* remove the pack entries */
        list_for_each_entry_safe(pack_entry, ne, &entry->pack_list, pack_list) {
		    list_del(&pack_entry->list);
		    zebra_buffer_entry_destroy(pack_entry);
		}

	    zebra_buffer_entry_destroy(entry);
        transmit_list->list_length --;
        transmit_list->complete_length ++;
    } while (-- count);

    LOG_DEBUG(" - dev%d, complete:%d, transmit_len:%d, sharemem(rx_rdy:%d, tx_empty:%d) \n",
                dev->device_id, transmit_list->complete_length, transmit_list->list_length,
                GET_RXRDY_IDX(two_idx), GET_TXEMPTY_IDX(two_idx));

    /* sanity check */
    if (unlikely(transmit_list->list_length < 0))
        panic("%s(dev: %d), bug, transmit_list is < 0, %d! \n", __func__, dev->device_id,
                                        transmit_list->list_length);

    LOG_DEBUG("<zebra_write_done, exit. dev:%d> \n", dev->device_id);

    return;
}

/*
 * timeout function while sending the data to medium.
 */
void zebra_write_error(struct work_struct *work)
{
	zebra_device_t *dev = container_of(work, zebra_device_t, write_error);
	zebra_board_t  *brd = dev->board;
	buffer_list_t *write_list = &dev->write_list;
	transmit_list_t *transmit_list = &dev->wr_transmit_list;
    share_memory_t  *sharemem;

	LOG_DEBUG("<write_error, dev%d>\n", dev->device_id);

	/* Protocol check. If write_list is empty(dev->write_busy is 0), the work should not be
	 * executed.
	 */
	if (!down_trylock(&dev->write_busy)) {
		up(&dev->write_busy);
		/* The flag should have been set */
		LOG_ERROR(" - ERROR: dev%d, spurious write transaction error "\
			"call (spurious timer interrupt?)\n", dev->device_id);
		return;
	}

    LOG_ERROR(" - ERROR: dev%d doesn't receive the acknowledge from remote board: %s ...\n",
                dev->device_id, dev->board->name);

    down(&write_list->sem);
    LOG_ERROR(" - dev%d, write_list is %d, transmit_list length is %d \n", dev->device_id,
                write_list->list_length, transmit_list->list_length);
    LOG_ERROR(" - dev%d, two transmit index: %d %d \n", dev->device_id, transmit_list->rx_rdy_idx,
                transmit_list->tx_empty_idx);
    LOG_ERROR(" - dev%d, peer rcv rx_ready_status = %d(0x%x), tx_empty_status: %d(0x%x)\n", dev->device_id,
                readl(dev->peer_rx_ready_status_kaddr), readl(brd->peer_rx_ready_status_kaddr),
                readl(dev->peer_tx_empty_status_kaddr), readl(brd->peer_tx_empty_status_kaddr));

	/* error checking */
	if (unlikely(!write_list->list_length))
	    panic("%s, bug, write_list length: %d \n", __func__, write_list->list_length);

    if (unlikely(!transmit_list->list_length))
	    panic("%s, bug, transmitt length: %d \n", __func__, transmit_list->list_length);

	sharemem = (share_memory_t *)dev->write_kaddr;
	/* the case: we lost inerrupt */
	if (sharemem->hdr.rx_rdy_idx == sharemem->hdr.tx_empty_idx) {
	    printk("%s, interrupt loss, two share index: %d %d is identical!!!\n",
	                        __func__, sharemem->hdr.rx_rdy_idx, sharemem->hdr.tx_empty_idx);
    }

    /* setup the timer again */
    dev->write_timeout.function = zebra_write_timeout_handler;
	dev->write_timeout.data = (unsigned long)dev;
	mod_timer(&dev->write_timeout, jiffies + WAIT_TXEMPTY_TIMEOUT * HZ); /* 20 seconds */

    dev->counter.error ++;

	up(&write_list->sem);

    //zebra_write_device_doorbell(dev, dev->rx_ready, RX_READY);
}

/* Write timeout timer handler
 *
 * If TX_EMPTY is not acknowledged, then log an error,
 * perform the same sequence as the interrupt handler does
 * for a correctly received TX_EMPTY.
 */
void zebra_write_timeout_handler(unsigned long data)
{
	zebra_device_t *dev = (zebra_device_t *)data;

	LOG_ERROR("<write_timeout, dev%d, board:%s>\n", dev->device_id, dev->board->name);
	zebra_write_error(&dev->write_error);
	//zebra_schedule_work(dev->board->driver, &dev->write_error);
	zebra_debug_print(dev->board);
}

static inline void zebra_read_done(void *data)
{
    zebra_device_t *dev = (zebra_device_t *)data;
    buffer_list_t *read_list = &dev->read_list;
    transmit_list_t *transmit_list = &dev->rd_transmit_list;
    share_memory_t  *sharemem;

    LOG_DEBUG("<zebra_read_done, dev%d> \n", dev->device_id);

    down(&read_list->sem);

    transmit_list->tx_empty_idx ++;
    /* update the share memory as well */
    sharemem = (share_memory_t *)dev->read_kaddr;
    sharemem->hdr.tx_empty_idx = transmit_list->tx_empty_idx;
    dev->counter.user_read ++;

    LOG_DEBUG(" - dev%d, <rdone> transmitting(rx_rdy: %d, tx_empty: %d) \n", dev->device_id,
                transmit_list->rx_rdy_idx, transmit_list->tx_empty_idx);

    if (need_send_txempty(read_list->list_length)) {
        /* send tx empty */
        zebra_write_device_doorbell(dev, dev->tx_empty, TX_EMPTY);
    }

    if (timer_pending(&dev->read_timeout)) {
		if (!read_list->list_length) {
		    del_timer(&dev->read_timeout);
		    LOG_DEBUG(" - dev%d, cancel the READ timer\n", dev->device_id);
		}
        else {
            mod_timer(&dev->read_timeout, jiffies + APP_READ_TIMEOUT * HZ); /* 20 seconds */
            LOG_DEBUG(" - dev%d, refersh the READ timer\n", dev->device_id);
        }
	}

    up(&read_list->sem);

    LOG_DEBUG("<zebra_read_done, dev%d> exit \n", dev->device_id);
}

/*-----------------------------------------------------------------
 * interface functions
 *-----------------------------------------------------------------
 */
/*
 * timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 */
int zebra_msg_rcv(int board_id, int device_id, unsigned char *buf, int *size, int timeout_ms)
{
    zebra_device_t *dev;
    zebra_board_t  *brd;
	buffer_list_t *read_list;
	buffer_entry_t *entry;
	unsigned int bFound = 0;
	int status, offset;

	list_for_each_entry(brd, &zebra_driver->board_list, list) {
        if (brd->board_id == board_id) {
            bFound = 1;
            break;
        }
    }
    if (unlikely(!bFound)) {
        LOG_ERROR("cannot find the board_id: 0x%x \n", board_id);
        return -ENODEV;
    }

    dev = &brd->device[device_id];
    if (!(dev->open_mode & ZEBRA_OPEN_READ)) {
        printk("%s, device %d is not opened! \n", __func__, dev->device_id);
        return -1;
    }

    if (dev->attribute & ATTR_PACK) {
        LOG_ERROR("dev%d is opened as pack attribute! \n", dev->device_id);
        return -2;
    }

    while (unlikely(!brd->devices_initialized))
        msleep(1);

	read_list = &dev->read_list;

    dev->pktrd_out_jiffies = (u32)jiffies;

	/* Read from the current buffer, or get one from the read_list */
	down(&dev->read_mutex);
	entry = dev->read_entry;
	if (entry == NULL) {
	    /* Get a new buffer from the buffer list */
	    down(&read_list->sem);
    	/* check if read list is empty? */
    	while (read_list->list_length == 0) {
    		up(&read_list->sem);

    		/* Nothing to read */
    		if (timeout_ms == 0) {
    			LOG_DEBUG(" - dev%d, non-blocking read, and no data\n", dev->device_id);
    			up(&dev->read_mutex);
    			return -EAGAIN;
    		}

            if (dev->open_read == 0) {
                printk("%s, device %d is closed! \n", __func__, dev->device_id);
				up(&dev->read_mutex);
                return -2;
            }

    		LOG_DEBUG(" - dev%d, waiting on the device's read wait-queue.\n", dev->device_id);
    		if (dev->device_id >= ZEBRA_DEVICE_AP_START) {
        		if (timeout_ms > 0) {
        		    status = wait_event_interruptible_timeout(
            			read_list->wait,
            			dev->read_event,
            			msecs_to_jiffies(timeout_ms));
                    if (status == 0) {
                        LOG_DEBUG(" - dev%d, wait data timeout!\n", dev->device_id);
                        up(&dev->read_mutex);
                        return -EAGAIN;
                    }
        		} else {
        		    /* timeout_ms = -1, blocking forever until data coming */
                    wait_event_interruptible(read_list->wait, dev->read_event);
        		}

                /* Woken by new data or a signal */
        		if (signal_pending(current)) {
        			LOG_DEBUG(" - dev%d, read woken by a signal\n", dev->device_id);
        			up(&dev->read_mutex);
        			return -ERESTARTSYS;
        		}
    		} else {
    		    if (timeout_ms > 0) {
        		    status = wait_event_timeout(
            			read_list->wait,
            			dev->read_event,
            			msecs_to_jiffies(timeout_ms));
                    if (status == 0) {
                        LOG_DEBUG(" - dev%d, wait data timeout!\n", dev->device_id);
                        up(&dev->read_mutex);
                        return -EAGAIN;
                    }
        		} else {
        		    /* timeout_ms = -1, blocking forever until data coming */
                    wait_event(read_list->wait, dev->read_event);
        		}
    		}

    		/* Must have got here due to a list update,
    		 * but double check for a buffer in the list
    		 */
    		down(&read_list->sem);
    		if (dev->read_event)
    			dev->read_event = 0;
    	}

    	dev->read_event = 0;

        if (list_empty(&read_list->list))
	        panic("%s, read_list is empty! \n", __func__);

    	/* Remove the tail entry from the read_list */
    	entry = list_entry(read_list->list.prev, buffer_entry_t, list);
    	list_del(read_list->list.prev);
    	read_list->list_length --;
    	up(&read_list->sem); /* release read list sema */
		dev->read_entry = entry;
	}

	/* Read count check */
	if (*size > (int)entry->length) {
		//LOG_DEBUG(" - dev%d, read count %d reduced to %d\n", dev->device_id, *size, entry->length);
		*size = entry->length;
	}

	/* do the copy. */
	offset = entry->offset;

    if (offset == 0) {
        u32 phy_end = brd->pool_mem.paddr + brd->pool_mem.size;

        if ((entry->phy_addr >= brd->pool_mem.paddr) && (entry->phy_addr < phy_end)) {
#ifdef CACHE_COPY
            /* Notice:
             * If the target is PCIe, the cache data cannot pass through due to
             * length 128 alignment. That is cache burst length is only 32.
             * This limitation is only for PCI DEVICE because its memory is in
             * remote PCI HOST's DDR.
             */
            if (IS_PCI_DEVICE(brd->driver, brd) && IS_PCI(brd)) {
                /* must be non-cache */
                entry->va_addr = (void *)(brd->pool_mem.vaddr + (entry->phy_addr - brd->pool_mem.paddr));
            } else {
                /* cache */
                entry->va_addr = (void *)(brd->pool_mem_cache + (entry->phy_addr - brd->pool_mem.paddr));
                if ((u32)entry->va_addr & CACHE_LINE_MASK) {
                        panic("%s, from board: %s, entry->va_addr: 0x%x, paddr: 0x%x not %d alignment!\n", __func__,
                                brd->name, (u32)entry->va_addr, entry->phy_addr, CACHE_LINE_SZ);
                }
                //invalidate first
                fmem_dcache_sync((void *)entry->va_addr, CACHE_ALIGN(entry->length), DMA_FROM_DEVICE);
            }
#else
            entry->va_addr = (void *)(brd->pool_mem.vaddr + (entry->phy_addr - brd->pool_mem.paddr));
#endif
        } else {
            dev->counter.va_exception ++;
            entry->va_addr = (void *)ioremap_nocache(entry->phy_addr, entry->length);
    	    if (entry->va_addr == NULL)
                panic("%s, No virutal memory for size %d \n", __func__, entry->length);
            entry->f_ioreamp = 1;
        }
    }
    if (KERNEL_MEM(buf))
        zebra_memory_copy(buf, entry->va_addr + entry->offset, *size, dev, PCI_DMA_FROMDEVICE);
    else
        status = copy_to_user(buf, entry->va_addr + entry->offset, *size);

    /* move the next pointer */
    entry->offset += *size;
    entry->length -= *size;

    if (entry->length == 0) {
        //u32 va_end = brd->pool_mem.vaddr + brd->pool_mem.size;

        /* free vaddr */
	    if (entry->f_ioreamp)
            __iounmap(entry->va_addr);
        entry->f_ioreamp = 0;

        /* release the buffer entry */
	    zebra_buffer_entry_destroy(entry);
	    dev->read_entry = NULL;

        zebra_read_done(dev);
    }

	up(&dev->read_mutex);

	return 0;
}

/*
 * timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 */
int zebra_pack_msg_rcv(int board_id, int device_id, unsigned char **buf, int *size, int timeout_ms)
{
    zebra_driver_t *drv;
    zebra_device_t *dev;
    zebra_board_t  *brd;
	buffer_list_t *read_list;
	buffer_entry_t *entry, *pack_entry;
	unsigned int bFound = 0;
	int status, offset, pack_idx, well_read = 1, ret = -1;
    u32 llp_va = 0, llp_pa = 0, dst_paddr, left_len;
    unsigned int error_code = 0;
    int app_size[ZEBRA_NUM_IN_PACK], sec_read = 0;

	list_for_each_entry(brd, &zebra_driver->board_list, list) {
        if (brd->board_id == board_id) {
            bFound = 1;
            break;
        }
    }
    if (unlikely(!bFound)) {
        LOG_ERROR("cannot find the board_id: 0x%x \n", board_id);
        return -ENODEV;
    }

    drv = brd->driver;
    dev = &brd->device[device_id];
    if (!(dev->open_mode & ZEBRA_OPEN_READ)) {
        printk("%s, device %d is not opened! \n", __func__, dev->device_id);
        return -1;
    }

    if (!(dev->attribute & ATTR_PACK)) {
        LOG_ERROR("dev%d is not opened as pack attribute! \n", dev->device_id);
        return -2;
    }

    while (unlikely(!brd->devices_initialized))
        msleep(1);

	read_list = &dev->read_list;

    dev->pktrd_out_jiffies = (u32)jiffies;

	/* Read from the current buffer, or get one from the read_list */
	down(&dev->read_mutex);

	memset(app_size, 0, sizeof(app_size));

	entry = dev->read_entry;
	if (entry == NULL) {
	    /* Get a new buffer from the buffer list */
	    down(&read_list->sem);
    	/* check if read list is empty? */
    	while (read_list->list_length == 0) {
    		up(&read_list->sem);

    		/* Nothing to read */
    		if (timeout_ms == 0) {
    			LOG_DEBUG(" - dev%d, non-blocking read, and no data\n", dev->device_id);
    			up(&dev->read_mutex);
    			return -EAGAIN;
    		}

            if (dev->open_read == 0) {
                printk("%s, device %d is closed! \n", __func__, dev->device_id);
				up(&dev->read_mutex);
                return -2;
            }

    		LOG_DEBUG(" - dev%d, waiting on the device's read wait-queue.\n", dev->device_id);
    		if (timeout_ms > 0) {
        		status = wait_event_timeout(
        			read_list->wait,
        			dev->read_event,
        			msecs_to_jiffies(timeout_ms));
                if (status == 0) {
                    LOG_DEBUG(" - dev%d, wait data timeout!\n", dev->device_id);
                    up(&dev->read_mutex);
                    return -EAGAIN;
                }
            } else {
                /* timeout_ms = -1, blocking forever until data coming */
                wait_event(read_list->wait,	dev->read_event);
            }
    		/* Must have got here due to a list update,
    		 * but double check for a buffer in the list
    		 */
    		down(&read_list->sem);
    		if (dev->read_event)
    			dev->read_event = 0;
    	}

    	dev->read_event = 0;

        if (list_empty(&read_list->list))
	        panic("%s, read_list is empty! \n", __func__);

    	/* Remove the tail entry from the read_list */
    	entry = list_entry(read_list->list.prev, buffer_entry_t, list);
    	list_del(read_list->list.prev);
    	read_list->list_length --;
    	up(&read_list->sem); /* release read list sema */
		dev->read_entry = entry;
	} else {
	    sec_read = 1;
	    error_code |= 0x1;
	}

    pack_idx = 0;
    do {
        pack_entry = (pack_idx == 0) ? entry : list_entry(pack_entry->pack_list.next, buffer_entry_t, pack_list);
        /* backup the application size */
        app_size[pack_idx] = size[pack_idx];

        if (!pack_entry->length && size[pack_idx])
            error_code |= 0x2;

        if (!size[pack_idx] && pack_entry->length)
            error_code |= 0x4;

        /* Read count check */
        if (size[pack_idx] > (int)pack_entry->length)
            size[pack_idx] = (int)pack_entry->length;

        if (size[pack_idx] == 0)
            goto loop;

        /* do the copy. */
	    offset = pack_entry->offset;
	    if (offset == 0) {
	        /*
	         * Init cache processing is done here
	         */
	        u32 phy_end = brd->pool_mem.paddr + brd->pool_mem.size;

	        if ((pack_entry->phy_addr >= brd->pool_mem.paddr) && (pack_entry->phy_addr < phy_end)) {
#ifdef CACHE_COPY
                if (drv->dma_type == ZEBRA_DMA_NONE) {
                    pack_entry->va_addr = (void *)(brd->pool_mem_cache + (pack_entry->phy_addr - brd->pool_mem.paddr));
                    if ((u32)pack_entry->va_addr & CACHE_LINE_MASK) {
                        panic("%s, from board: %s, pack_entry->va_addr: 0x%x, paddr: 0x%x not %d alignment!\n", __func__,
                                brd->name, (u32)pack_entry->va_addr, pack_entry->phy_addr, CACHE_LINE_SZ);
                    }
                    /* Notice:
                     * invalidate whole packet buffer before DMA. This prevent the dirty data in buffer
                     * be written to DMA buffer. It will cause cpu data to overwrite DMA data
                     */
                    fmem_dcache_sync((void *)pack_entry->va_addr, CACHE_ALIGN(pack_entry->length), DMA_FROM_DEVICE);
                } else {
                    pack_entry->va_addr = (void *)(brd->pool_mem.vaddr + (pack_entry->phy_addr - brd->pool_mem.paddr));
                }
#else
                pack_entry->va_addr = (void *)(brd->pool_mem.vaddr + (pack_entry->phy_addr - brd->pool_mem.paddr));
#endif
            } else {
                panic("%s, No virutal memory for size %d \n", __func__, pack_entry->length);

                dev->counter.va_exception ++;
                pack_entry->va_addr = (void *)ioremap_nocache(pack_entry->phy_addr, pack_entry->length);
        	    if (pack_entry->va_addr == NULL)
                    panic("%s, No virutal memory for size %d \n", __func__, pack_entry->length);
                pack_entry->f_ioreamp = 1;
            }
        } else {
            /* offset != 0
             */
            /* limitation */
    	    if (drv->dma_type != ZEBRA_DMA_NONE) {
    	        dump_stack();
    	        panic("%s, dma not support partial read! \n", __func__);
    	    }
        }

        if (KERNEL_MEM(buf[pack_idx])) {
            if (drv->dma_type == ZEBRA_DMA_NONE) {
                if ((u32)buf[pack_idx] > 0xF0000000)
                    panic("User Application kernel address: 0x%p \n", buf[pack_idx]);

                /* CPU copy. Note, the source is NCNB.
                 */
                zebra_memory_copy(buf[pack_idx], (char *)pack_entry->va_addr + pack_entry->offset, size[pack_idx], dev, PCI_DMA_FROMDEVICE);
            } else {
                /* DMA copy
                 */
                u32 size_align, align;

                if (!llp_va)
                    llp_va = (u32)mem_llp_alloc(&llp_pa);

                dst_paddr = fmem_lookup_pa((u32)buf[pack_idx]);
                if (!(dst_paddr & 0x7)) {
                    align = 8;
                    /* PCI-e: the burst length should be multiple of 128 bytes */
                    size_align = IS_PCI(brd) ? ((size[pack_idx] >> 7) << 7) : ((size[pack_idx] >> 3) << 3);
                } else if (!(dst_paddr & 0x3)) {
                    if (IS_PCI(brd)) {
                        dump_stack();
                        panic("%s, the dest paddr: 0x%x, vaddr: 0x%x is not aligned 8! \n", __func__,
                            dst_paddr, (u32)buf[pack_idx]);
                    }
                    align = 4;
                    /* PCI-e: the burst length should be multiple of 128 bytes */
                    size_align = IS_PCI(brd) ? ((size[pack_idx] >> 7) << 7) : ((size[pack_idx] >> 2) << 2);
                }
                else {
                    dump_stack();
                    panic("%s, the dest paddr: 0x%x is not aligned 4 or 8! \n", __func__, dst_paddr);
                }

                dma_llp_add(llp_va, llp_pa, pack_entry->phy_addr, dst_paddr, size_align, align);

                left_len = size[pack_idx] - size_align;
                if (left_len) {
                    memcpy((char *)buf[pack_idx] + size_align, (char *)pack_entry->va_addr + size_align, left_len);
                    /* Notice:
                     * clean kernel buffer because of write alloc function may be enabled in cpu and cause
                     * one cache line to be read in cpu cache, so we must do clean cache first. Otherwise,
                     * the cache sync will overwrite the data in DDR.
                     * scenario:
                     *  1) memcpy data to DDR
                     *  2) cache miss and line refill
                     *  3) update data in cache line
                     *  4) flush cache line into DDR.
                     *  5) inv whole buffer
                     */
                    //fmem_dcache_sync((char *)buf[pack_idx] + size_align, left_len, DMA_TO_DEVICE);
                }

                /* Notice:
                 * sync whole packet buffer before DMA. This prevent the dirty data in buffer from
                 * being written to DMA buffer. It will cause cpu data to overwrite DMA data
                 */

                /* kernel buffer we must use clean + inv for safe */
                fmem_dcache_sync((char *)buf[pack_idx], size[pack_idx], DMA_BIDIRECTIONAL);
            }
        } else {
            if ((u32)buf[pack_idx] > 0xF0000000)
                panic("User Application user address: 0x%p \n", buf[pack_idx]);

            /* User Space copy */
            status = copy_to_user((char *)buf[pack_idx], (char *)pack_entry->va_addr + pack_entry->offset, size[pack_idx]);
        }

        ret = 0;
        /* move the next pointer */
        pack_entry->offset += size[pack_idx];
        //pack_entry->length -= size[pack_idx];
        pack_entry->length = 0;
loop:
        if (pack_entry->length)
            well_read = 0;

        /* next entry in a pack */
        pack_idx ++;
    } while (pack_entry->pack_list.next != &entry->pack_list);  //end

    if ((drv->dma_type != ZEBRA_DMA_NONE) && llp_va && llp_pa) {
        dma_llp_fire(llp_va, llp_pa, IS_PCI(brd));
        /* free memory */
        mem_llp_free((void *)llp_va);
    }

    /* clear others to zero */
    for (; pack_idx < ZEBRA_NUM_IN_PACK; pack_idx ++)
        size[pack_idx] = 0;

    if (well_read == 1) {
        buffer_entry_t  *ne;
        //u32 va_end = brd->pool_mem.vaddr + brd->pool_mem.size;

        /* free pack entries first */
        list_for_each_entry_safe(pack_entry, ne, &entry->pack_list, pack_list) {
		    list_del(&pack_entry->list);
		    /* free vaddr */
    	    if (pack_entry->f_ioreamp) {
                __iounmap(pack_entry->va_addr);
                pack_entry->f_ioreamp = 0;
                pack_entry->va_addr = NULL;
            }
		    zebra_buffer_entry_destroy(pack_entry);
		}

        /* free vaddr */
	    if (entry->f_ioreamp)
            __iounmap(entry->va_addr);
        entry->va_addr = NULL;
        entry->f_ioreamp = 0;

        /* release the first buffer entry */
	    zebra_buffer_entry_destroy(entry);
	    dev->read_entry = NULL;
        zebra_read_done(dev);
    }

    if ((ret < 0) && error_code) {
        printk("%s, device_name: %s, board_name: %s, return error_code: 0x%x\n",
                __func__, dev->name, brd->name, error_code);

        pack_idx = 0;
        do {
            pack_entry = (pack_idx == 0) ? entry : list_entry(pack_entry->pack_list.next, buffer_entry_t, pack_list);
            printk("pack_idx: %d, size: %d(%d), pack_len: %d, pack_len_keep: %d, paddr:0x%x \n", pack_idx,
                size[pack_idx], app_size[pack_idx], pack_entry->length, pack_entry->length_keep, pack_entry->phy_addr);
            pack_idx ++;
        } while (pack_entry->pack_list.next != &entry->pack_list);  //end

        ret = -7;
    }

    up(&dev->read_mutex);

	return ret;
}

/* This function is used to reset the on-hand entry if the user doesn't want to process this
 * entry anymore.
 * Note: the packets queuing in the readlist are not affected.
 */
void zebra_pack_clean_readentry(int board_id, int device_id)
{
    zebra_driver_t *drv;
    zebra_device_t *dev;
    zebra_board_t  *brd;
	buffer_list_t *read_list;
	buffer_entry_t *entry, *pack_entry, *ne;
	int bFound = 0;

	list_for_each_entry(brd, &zebra_driver->board_list, list) {
        if (brd->board_id == board_id) {
            bFound = 1;
            break;
        }
    }
    if (unlikely(!bFound)) {
        LOG_ERROR("%s, cannot find the board_id: 0x%x \n", __func__, board_id);
        return;
    }

    drv = brd->driver;
    dev = &brd->device[device_id];
    if (!(dev->open_mode & ZEBRA_OPEN_READ)) {
        printk("%s, device %d is not opened! \n", __func__, dev->device_id);
        return;
    }

    if (!(dev->attribute & ATTR_PACK)) {
        LOG_ERROR("dev%d is not opened as pack attribute! \n", dev->device_id);
        return;
    }

    while (unlikely(!brd->devices_initialized))
        msleep(1);

	read_list = &dev->read_list;

	/* Read from the current buffer, or get one from the read_list */
	down(&dev->read_mutex);
	entry = dev->read_entry;
	if (entry == NULL) {
	    up(&dev->read_mutex);
	    return;   //do nothing
	}

    /* free pack entries first */
    list_for_each_entry_safe(pack_entry, ne, &entry->pack_list, pack_list) {
	    list_del(&pack_entry->list);
	    /* free vaddr */
	    if (pack_entry->f_ioreamp) {
            __iounmap(pack_entry->va_addr);
            pack_entry->f_ioreamp = 0;
            pack_entry->va_addr = NULL;
        }
	    zebra_buffer_entry_destroy(pack_entry);
	}

    /* free vaddr */
    if (entry->f_ioreamp)
        __iounmap(entry->va_addr);
    entry->va_addr = NULL;
    entry->f_ioreamp = 0;

    /* release the first buffer entry */
    zebra_buffer_entry_destroy(entry);
    dev->read_entry = NULL;
    zebra_read_done(dev);

    up(&dev->read_mutex);
    return;
}

/* return value: 0 for success, otherwise for fail
 * Note: even many entries are queued in write_list, but only one entry can be moved
 *      to transactiin list when TX_EMPTY interrupt is received.
 */
int zebra_msg_snd(int board_id, int device_id, unsigned char *buf, int count, int timeout_ms)
{
    int bFound = 0, status;
    zebra_board_t *brd;
    zebra_device_t *dev;
    buffer_list_t *write_list;
    buffer_entry_t *entry;
    unsigned int space;

    if (device_id >= ZEBRA_DEVICE_MAX) {
        LOG_ERROR("cannot find the device id: %d \n", device_id);
        return -ENODEV;
    }

    list_for_each_entry(brd, &zebra_driver->board_list, list) {
        if (brd->board_id == board_id) {
            bFound = 1;
            break;
        }
    }

    if (unlikely(!bFound)) {
        LOG_ERROR("cannot find the board_id: 0x%x \n", board_id);
        return -ENODEV;
    }
    dev = &brd->device[device_id];

    if (!count || (count > dev->mtu)) {
        LOG_ERROR("message size %d exceeds %d! \n", count, dev->mtu);
        return -EINVAL;
    }

    if (!dev->open_read) {
        LOG_ERROR("dev%d is not opened! \n", dev->device_id);
        return -EINVAL;
    }

    if (dev->attribute & ATTR_PACK) {
        LOG_ERROR("dev%d is opened as pack attribute! \n", dev->device_id);
        return -2;
    }

    while (unlikely(!brd->devices_initialized))
        msleep(1);

    /* Block other writes (i.e., mutual exclusion) */
	if (down_interruptible(&dev->write_mutex)) {
	    LOG_DEBUG(" - dev%d, down semaphore fail due to singal! \n", dev->device_id);
		return -ERESTARTSYS;
	}

    dev->pktwr_in_jiffies = (u32)jiffies;

	write_list = &dev->write_list;
	/* Grab the write list semaphore and check for space */
	down(&write_list->sem);
	space = write_list->list_length_max - write_list->list_length;
	up(&write_list->sem);
	if (!space && !timeout_ms) {
	    LOG_DEBUG(" - dev%d, non-blocking write; buffer list is full\n", dev->device_id);
		up(&dev->write_mutex);
		return -EAGAIN; /* try again */
	}

    status = zebra_buffer_entry_create(count, &entry, (void *)brd);
    if (status) {
        LOG_ERROR(" - (write) buffer entry creation failed\n");
		up(&dev->write_mutex);
		dev->counter.dropped ++;
		panic("%s, no buffer entry! \n", __func__);
		return status;
    }

#ifdef CACHE_COPY
    /* Notice:
     * If the target is PCIe, the cache data cannot pass through due to
     * length 128 alignment. That is cache burst length is only 32.
     * This limitation is only for PCI DEVICE because its memory is in
     * remote PCI HOST's DDR.
     */
    if (IS_PCI_DEVICE(brd->driver, brd) && IS_PCI(brd)) {
        /* non-cache case */
        if (KERNEL_MEM(buf)) {
    	    zebra_memory_copy((char *)entry->databuf, (char *)buf, count, dev, PCI_DMA_TODEVICE);
    	    LOG_DEBUG(" - dev%d, copy %d bytes from upper layer.\n", dev->device_id, count);
    	}
        else {
            status = copy_from_user(entry->databuf, buf, count);
            LOG_DEBUG(" - dev%d, copy %d bytes from application.\n", dev->device_id, count);
        }
    } else {
        /* cache case */
        if (KERNEL_MEM(buf)) {
    	    zebra_memory_copy((char *)entry->direct_databuf, (char *)buf, count, dev, PCI_DMA_TODEVICE);
    	    LOG_DEBUG(" - dev%d, copy %d bytes from upper layer.\n", dev->device_id, count);
    	}
        else {
            status = copy_from_user(entry->direct_databuf, buf, count);
            LOG_DEBUG(" - dev%d, copy %d bytes from application.\n", dev->device_id, count);
        }
        /* clean first */
        fmem_dcache_sync((void *)entry->direct_databuf, CACHE_ALIGN_32(count), DMA_TO_DEVICE);
    }
#else
	if (KERNEL_MEM(buf)) {
	    zebra_memory_copy((char *)entry->databuf, (char *)buf, count, dev, PCI_DMA_TODEVICE);
	    LOG_DEBUG(" - dev%d, copy %d bytes from upper layer.\n", dev->device_id, count);
	}
    else {
        status = copy_from_user(entry->databuf, buf, count);
        LOG_DEBUG(" - dev%d, copy %d bytes from application.\n", dev->device_id, count);
    }
#endif

    /*
     * Add the buffer to the head of the write buffer list
     */
	down(&write_list->sem); /* write list */
	space = write_list->list_length_max - write_list->list_length;

    while (write_list->list_length == (int)write_list->list_length_max) {
		up(&write_list->sem);

        if (!dev->open_read) {
            dev->write_event = 0;
            LOG_ERROR("dev%d is not opened! \n", dev->device_id);
            zebra_buffer_entry_destroy(entry);
			up(&dev->write_mutex);
            return -2;
        }

		dev->counter.user_blocking ++;

		/* Blocking write; Wait for space */
		LOG_DEBUG(" - dev%d, wait on the write wait-queue.\n", dev->device_id);
		if (dev->device_id >= ZEBRA_DEVICE_AP_START) {
		    if (timeout_ms > 0) {
        		status = wait_event_interruptible_timeout(
        			write_list->wait,
        			dev->write_event,
        			msecs_to_jiffies(timeout_ms));
                if (status == 0) {
                    LOG_DEBUG(" - dev%d, write timeout!\n", dev->device_id);
        			zebra_buffer_entry_destroy(entry);
        			up(&dev->write_mutex);
        			return -EAGAIN;
                }
            } else {
                /* timeout_ms = -1 */
                wait_event_interruptible(write_list->wait, dev->write_event);
            }
            /* Woken by read or a signal */
    		if (signal_pending(current)) {
    			LOG_DEBUG(" - dev%d, write woken by a signal\n", dev->device_id);
    			zebra_buffer_entry_destroy(entry);
    			up(&dev->write_mutex);
    			return -ERESTARTSYS;
    		}
		} else {
    		if (timeout_ms > 0) {
        		status = wait_event_timeout(
        			write_list->wait,
        			dev->write_event,
        			msecs_to_jiffies(timeout_ms));
                if (status == 0) {
                    LOG_DEBUG(" - dev%d, write timeout!\n", dev->device_id);
        			zebra_buffer_entry_destroy(entry);
        			up(&dev->write_mutex);
        			return -EAGAIN;
                }
            } else {
                /* timeout_ms = -1 */
                wait_event(write_list->wait, dev->write_event);
            }
        }
        /* the case: write timeout */
        if (dev->write_event == 2) {
            dev->write_event = 0;
            LOG_DEBUG(" - dev%d, write woken by write-timeout error\n", dev->device_id);
			zebra_buffer_entry_destroy(entry);
			up(&dev->write_mutex);
			return -1;
        }

        /* Must have got here due to space, but double check */
		down(&write_list->sem);
		space = write_list->list_length_max - write_list->list_length;
		if (dev->write_event)
			dev->write_event = 0;
	}

	dev->write_event = 0;
	/* Ok, the buffer list is ours */
	list_add(&entry->list, &write_list->list);
	write_list->list_length ++;
    dev->counter.user_write ++;

	LOG_DEBUG(" - dev%d, zebra_msg_snd: write-list length %d\n", dev->device_id, write_list->list_length);

	up(&write_list->sem); //write list

	/* Schedule the work function */
	if (!down_trylock(&dev->write_busy)) {
	    /* success to get semaphore. Note: It means the write_busy semaphore is taken first */
		LOG_DEBUG(" - dev%d, schedule write-request\n", dev->device_id);
		/* call zebra_write_request() */
		zebra_schedule_work(brd->driver, &dev->write_request, dev);
	}

	/* Allow other writers */
	up(&dev->write_mutex);

	/* Return the number of bytes sucessfully written */
	return 0;
}

/* return value: 0 for success, otherwise for fail
 * Note: even many entries are queued in write_list, but only one entry can be moved
 *      to transactiin list when TX_EMPTY interrupt is received.
 */
int zebra_pack_msg_snd(int board_id, int device_id, unsigned char **buf, int *count, int timeout_ms)
{
    int bFound = 0, status;
    zebra_driver_t *drv;
    zebra_board_t *brd;
    zebra_device_t *dev;
    buffer_list_t *write_list;
    buffer_entry_t *entry = NULL, *pack_entry;
    unsigned int space, pack_idx = 0;
    u32 llp_va = 0, llp_pa = 0, src_paddr, left_len;


    if (device_id >= ZEBRA_DEVICE_MAX) {
        LOG_ERROR("cannot find the device id: %d \n", device_id);
        return -ENODEV;
    }

    list_for_each_entry(brd, &zebra_driver->board_list, list) {
        if (brd->board_id == board_id) {
            bFound = 1;
            break;
        }
    }

    if (unlikely(!bFound)) {
        LOG_ERROR("cannot find the board_id: 0x%x \n", board_id);
        return -ENODEV;
    }

    while (unlikely(!brd->devices_initialized))
        msleep(1);

    drv = brd->driver;
    dev = &brd->device[device_id];

	if (!dev->open_read) {
        LOG_ERROR("dev%d is not opened! \n", dev->device_id);
        return -2;
    }

    if (!(dev->attribute & ATTR_PACK)) {
        LOG_ERROR("dev%d is not opened as pack attribute! \n", dev->device_id);
        return -2;
    }

    /* Block other writes (i.e., mutual exclusion) */
	if (down_interruptible(&dev->write_mutex)) {
	    LOG_DEBUG(" - dev%d, down semaphore fail due to singal! \n", dev->device_id);
		return -ERESTARTSYS;
	}

	//LOG_DEBUG(" - dev%d, zebra_msg_snd, count = %d bytes \n", dev->device_id, count);

    dev->pktwr_in_jiffies = (u32)jiffies;

	write_list = &dev->write_list;
	/* Grab the write list semaphore and check for space */
	down(&write_list->sem);
	space = write_list->list_length_max - write_list->list_length;
	up(&write_list->sem);
	if (!space && !timeout_ms) {
	    LOG_DEBUG(" - dev%d, non-blocking write; buffer list is full\n", dev->device_id);
		up(&dev->write_mutex);
		return -EAGAIN; /* try again */
	}

	for (pack_idx = 0; pack_idx < ZEBRA_NUM_IN_PACK; pack_idx ++) {
	    if (count[0] == 0) {
	        dump_stack();
	        panic("%s, error zero length! \n", __func__);
        }
	    if (!count[pack_idx])
	        break;

    	status = zebra_buffer_entry_create(count[pack_idx], &pack_entry, (void *)brd);
        if (status) {
            LOG_ERROR(" - (write) buffer entry creation failed\n");
    		up(&dev->write_mutex);
    		dev->counter.dropped ++;
    		panic("%s(master), no buffer entry! \n", __func__);
    		return status;
        }

    	if (KERNEL_MEM(buf[pack_idx])) {
    	    if (drv->dma_type == ZEBRA_DMA_NONE) {
    	        /* CPU copy
    	         */
#ifdef CACHE_COPY
                zebra_memory_copy((char *)pack_entry->direct_databuf, (char *)buf[pack_idx], count[pack_idx], dev, PCI_DMA_TODEVICE);
                /* clean first */
                fmem_dcache_sync((void *)pack_entry->direct_databuf, CACHE_ALIGN_32(count[pack_idx]), DMA_TO_DEVICE);
#else
    	        zebra_memory_copy((char *)pack_entry->databuf, (char *)buf[pack_idx], count[pack_idx], dev, PCI_DMA_TODEVICE);
#endif
    	    } else {
    	        /* DMA copy
    	         */
                u32 size_align, align;

                if (!llp_va)
                    llp_va = (u32)mem_llp_alloc(&llp_pa);

                src_paddr = fmem_lookup_pa((u32)buf[pack_idx]);
                if (!(src_paddr & 0x7)) {
                    align = 8;
                    size_align = IS_PCI(brd) ? ((count[pack_idx] >> 7) << 7) : ((count[pack_idx] >> 3) << 3);
                } else if (!(src_paddr & 0x3)) {
                    if (IS_PCI(brd)) {
                        dump_stack();
                        panic("%s, the source paddr: 0x%x, vaddr: 0x%x is not aligned 8! \n", __func__,
                            src_paddr, (u32)buf[pack_idx]);
                    }
                    align = 4;
                    size_align = IS_PCI(brd) ? ((count[pack_idx] >> 7) << 7) : ((count[pack_idx] >> 2) << 2);
                }
                else {
                    dump_stack();
                    panic("%s, the source paddr: 0x%x is not aligned 4 or 8! \n", __func__, src_paddr);
                }

                left_len = count[pack_idx] - size_align;
                if (left_len)
                    memcpy((char *)pack_entry->databuf + size_align, (char *)buf[pack_idx] + size_align, left_len);

                //clear cache
                fmem_dcache_sync(buf[pack_idx], count[pack_idx], DMA_TO_DEVICE);
                dma_llp_add(llp_va, llp_pa, src_paddr, pack_entry->phy_addr, size_align, align);
            }
    	    LOG_DEBUG(" - dev%d, copy %d bytes from upper layer.\n", dev->device_id, count[pack_idx]);
    	} else {
    	    /* User Space
    	     */
#ifdef CACHE_COPY
            status = copy_from_user((char *)pack_entry->direct_databuf, (char *)buf[pack_idx], count[pack_idx]);
            /* clean first */
            fmem_dcache_sync((void *)pack_entry->direct_databuf, CACHE_ALIGN_32(count[pack_idx]), DMA_TO_DEVICE);
#else
            status = copy_from_user((char *)pack_entry->databuf, (char *)buf[pack_idx], count[pack_idx]);
#endif
            LOG_DEBUG(" - dev%d, copy %d bytes from application.\n", dev->device_id, count[pack_idx]);
        }

	    if (pack_idx == 0)
	        entry = pack_entry;
	    else
	        list_add_tail(&pack_entry->pack_list, &entry->pack_list);
    }

    if ((drv->dma_type != ZEBRA_DMA_NONE) && llp_va && llp_pa) {
        dma_llp_fire(llp_va, llp_pa, IS_PCI(brd));
        /* free memory */
        mem_llp_free((void *)llp_va);
    }

    /*
     * Add the buffer to the head of the write buffer list
     */
	down(&write_list->sem); /* write list */
	space = write_list->list_length_max - write_list->list_length;

    while (write_list->list_length == (int)write_list->list_length_max) {
		up(&write_list->sem);

		dev->counter.user_blocking ++;

		/* Blocking write; Wait for space */
		LOG_DEBUG(" - dev%d, wait on the write wait-queue.\n", dev->device_id);
		if (timeout_ms > 0) {
    		status = wait_event_timeout(
    			write_list->wait,
    			dev->write_event,
    			msecs_to_jiffies(timeout_ms));
            if (status == 0) {
                LOG_DEBUG(" - dev%d, write timeout!\n", dev->device_id);
    			zebra_buffer_entry_destroy(entry);
    			up(&dev->write_mutex);
    			return -EAGAIN;
            }
        } else {
            /* timeout_ms = -1 */
            wait_event(write_list->wait, dev->write_event);
        }

		if (!dev->open_read) {
		    buffer_entry_t *pack_entry, *ne;

			dev->write_event = 0;
			LOG_ERROR("dev%d is not opened! \n", dev->device_id);

			list_for_each_entry_safe(pack_entry, ne, &entry->pack_list, pack_list) {
			    list_del(&pack_entry->list);
			    zebra_buffer_entry_destroy(pack_entry);
			}
			zebra_buffer_entry_destroy(entry);  /* delete head */
			up(&dev->write_mutex);
			return -1;
		}

        /* the case: write timeout */
        if (dev->write_event == 2) {
            buffer_entry_t *pack_entry, *ne;

            dev->write_event = 0;
            LOG_DEBUG(" - dev%d, write woken by write-timeout error\n", dev->device_id);
            list_for_each_entry_safe(pack_entry, ne, &entry->pack_list, pack_list) {
			    list_del(&pack_entry->list);
			    zebra_buffer_entry_destroy(pack_entry);
			}
			zebra_buffer_entry_destroy(entry); /* delete head */
			up(&dev->write_mutex);
			return -1;
        }

        /* Must have got here due to space, but double check */
		down(&write_list->sem);
		space = write_list->list_length_max - write_list->list_length;
		if (dev->write_event)
			dev->write_event = 0;
	}
	dev->write_event = 0;
	/* Ok, the buffer list is ours */
	list_add(&entry->list, &write_list->list);
	write_list->list_length ++;
    dev->counter.user_write ++;

	LOG_DEBUG(" - dev%d, zebra_msg_snd: write-list length %d\n", dev->device_id, write_list->list_length);

	up(&write_list->sem); //write list

	/* Schedule the work function */
	if (!down_trylock(&dev->write_busy)) {
	    /* success to get semaphore. Note: It means the write_busy semaphore is taken first */
		LOG_DEBUG(" - dev%d, schedule pack write-request\n", dev->device_id);
		/* call zebra_write_request() */
		zebra_schedule_work(brd->driver, &dev->pack_write_request, dev);
	}

	/* Allow other writers */
	up(&dev->write_mutex);

	/* Return the number of bytes sucessfully written */
	return 0;
}

/* get the max message size to read/write of the specific channel. > 0 for success, others for fail.
 */
int zebra_get_msgsz(int board_id, int device_id)
{
    int bFound = 0;
    zebra_board_t *brd;
    zebra_device_t *dev;
    buffer_list_t  *write_list;

    if (device_id >= ZEBRA_DEVICE_MAX) {
        LOG_ERROR("cannot find the device id: %d \n", device_id);
        return -1;
    }

    list_for_each_entry(brd, &zebra_driver->board_list, list) {
        if (brd->board_id == board_id) {
            bFound = 1;
            break;
        }
    }
    if (unlikely(!bFound)) {
        LOG_ERROR("cannot find the board_id: 0x%x \n", board_id);
        return -1;
    }

    dev = &brd->device[device_id];
    write_list = &dev->write_list;

    return write_list->buffer_length_max;
}

int zebra_device_register(zebra_board_t *brd)
{
#ifndef WIN32
    struct device  *class_dev;
	zebra_driver_t *drv = brd->driver;
	int i, status;
	dev_t devno;

	/* Register the character device */
	LOG_DEBUG(" - register a character device for major.minor.count %d.%d.%d\n",
		MAJOR(brd->devno), MINOR(brd->devno), NUM_OF_DEVICES_PER_BOARD);

	cdev_init(&brd->cdev, &zebra_driver_fops);
	brd->cdev.owner = THIS_MODULE;
	status = cdev_add(&brd->cdev, brd->devno, NUM_OF_DEVICES_PER_BOARD);
	if (status < 0) {
		panic("error, device registration failed with error %d\n", status);
		return status;
	}

	/* Create the /dev entries: /dev/zebra_<bus:dev.fn>_<minor> */
	for (i = 0; i < NUM_OF_DEVICES_PER_BOARD ; i++) {
		devno = MKDEV(MAJOR(brd->devno), MINOR(brd->devno) + i);
		class_dev = device_create(drv->class,
                            NULL,
                            devno,
                            NULL,
        				    "cpucomm_%s_chan%d",
        				    brd->name, i);
        if (IS_ERR(class_dev))
            panic("<%s> device_create fail! \n", __func__);
    }
#endif
    return 0;
}

void zebra_device_unregister(zebra_board_t *brd)
{
#ifndef WIN32
	zebra_driver_t *drv = brd->driver;
	int i;
	dev_t devno;

	LOG_DEBUG("<unregister the devices>\n");

	/* Remove the /dev entries */
	if (brd->devices_initialized) {
		LOG_DEBUG("remove the device nodes for major.minor.count %d.%d.%d\n",
			MAJOR(brd->devno), MINOR(brd->devno), NUM_OF_DEVICES_PER_BOARD);

		for (i = 0; i < NUM_OF_DEVICES_PER_BOARD; i++) {
			devno = MKDEV(MAJOR(brd->devno), MINOR(brd->devno) + i);
			device_destroy(drv->class, devno);
		}

		LOG_DEBUG("unregister the character device\n");
		cdev_del(&brd->cdev);
	}
#endif
	return;
}

/* Minor number acquisition */
int zebra_minor_range_claim(zebra_board_t *brd)
{
#ifndef WIN32
	int status;
	int major, minor;
	dev_t devno;

	major = zebra_major;
	minor = zebra_minor;

    /* 1. first to allocate a major number.
     * 2. then use register method to register the minor with the same major.
     * Zebra shares the same major among all devices as possible.
     * Only minor is different. cat /proc/devices can see the number.
     */
	if (major != 0) {
		/* Request a specific major */
		devno = MKDEV(major, minor);
		status = register_chrdev_region(devno, NUM_OF_DEVICES_PER_BOARD, (char *)brd->name);
	} else {
		/* Request dynamic allocation of a major */
		status = alloc_chrdev_region(&devno, minor, NUM_OF_DEVICES_PER_BOARD, (char *)brd->name);
		major = MAJOR(devno);
		minor = MINOR(devno);

		/* A dynamic major will be assigned on the first call
		 * to probe. After that, the same specific major will
		 * be requested.
		 */
		zebra_major = major;
	}
	if (status < 0) {
		LOG_ERROR(" - major/minor request failed with error %d\n", status);
		return status;
	}

	/* Store the start major/minor so it can be released later */
	LOG_DEBUG(" - obtained major.minor.count %d.%d.%d\n",
		major, minor, NUM_OF_DEVICES_PER_BOARD);
	brd->devno = devno;
	zebra_minor += NUM_OF_DEVICES_PER_BOARD;

	LOG_DEBUG(" - check again major.minor.count %d.%d.%d\n",
		MAJOR(devno), MINOR(devno), NUM_OF_DEVICES_PER_BOARD);
#endif
	return 0;
}

void zebra_minor_range_release(zebra_board_t *brd)
{
    LOG_DEBUG("<release minor number>\n");

#ifndef WIN32
	if (MAJOR(brd->devno) != 0) {
		LOG_DEBUG("release major.minor.count %d.%d.%d\n",
			MAJOR(brd->devno), MINOR(brd->devno),
			NUM_OF_DEVICES_PER_BOARD);

		unregister_chrdev_region(brd->devno, NUM_OF_DEVICES_PER_BOARD);
	}
#endif
}

/* this function setup the mbox.
 * Note: only the cpu core host in PCI host can create the mbox. others will keep polling the mbox
 *       until it is established by the host.
 */
void zebra_setup_mbox(zebra_driver_t *drv, zebra_board_t *brd)
{
	u32 *mbox_vaddr;

#ifdef WIN32
	if (drv->mbox_host) {
    	ZEBRA_MBOX_PADDR = (u32)malloc(4);
    	ZEBRA_PCI_MBOX_PADDR = ZEBRA_MBOX_PADDR;
    	memset((char *)ZEBRA_MBOX_PADDR, 0, 4);
	} else {
		while (ZEBRA_MBOX_PADDR == 0)
			msleep(100);
		ZEBRA_PCI_MBOX_PADDR = ZEBRA_MBOX_PADDR;
	}
#endif

    if (IS_PCI_HOST(drv)) {
        /* assign mbox paddr to the specific address, other devices(local devices and pci devices)
         * will see this information.
         */
        mbox_vaddr = (u32 *)ioremap_nocache(ZEBRA_MBOX_PADDR, 4);
        if (mbox_vaddr == NULL) panic("%s, 1\n", __func__);

        if (drv->mbox_host) {
            if (!drv->mbox_kaddr || !drv->mbox_paddr)
                panic("%s, bug! \n", __func__);
#ifndef WIN32
            /* to set the remote outbound window to have the PCI peer access my local memory
             * including the mbox
             */
            if (IS_PCI(brd)) {
                u32 peer_ddr_base = drv->local_pool_paddr & PCI_BAR_MASK;

                platform_set_outbound_win(brd, peer_ddr_base);
            }
#endif
            /* give the mbox memory and everyone will see this address of mbox */
            *mbox_vaddr = drv->mbox_paddr;
        } else {
            int i = 0;

            if (!drv->mbox_paddr && !drv->mbox_kaddr) {
                printk("Wait Host core to provide the mbox .");
                while ((ioread32(mbox_vaddr) == 0) || (ioread32(mbox_vaddr) == 0xFFFFFFFF)) {
                    msleep(500);
                    i ++;
                    if (i & 1)
                        printk(".");
                }
                printk("done \n");
                drv->mbox_paddr = *mbox_vaddr;
                drv->mbox_kaddr = (unsigned int *)ioremap_nocache(drv->mbox_paddr, TOTAL_MBOX_SIZE);
                if (drv->mbox_kaddr == NULL)
                    panic("%s, 2\n", __func__);
            }
        }
        __iounmap((void *)mbox_vaddr);
        printk("--- mbox paddr: 0x%x, vaddr: 0x%x --- \n", drv->mbox_paddr, (u32)drv->mbox_kaddr);

        return;
    }

    /*
     * PCI DEVICE
     */

    /* pci board */
    if (IS_PCI(brd)) {
        u32 host_axi_ddr_base;
        int i = 0;

        mbox_vaddr = (u32 *)ioremap_nocache(ZEBRA_PCI_MBOX_PADDR, 4);
        if (mbox_vaddr == NULL) panic("%s, 3\n", __func__);

        printk("Wait PCI Host to provide the mbox .");
        while ((ioread32(mbox_vaddr) == 0) || (ioread32(mbox_vaddr) == 0xFFFFFFFF)) {
            msleep(500);
            i ++;
            if (i & 1)
                printk(".");
        }
        host_axi_ddr_base = *mbox_vaddr & PCI_BAR_MASK;
        printk("done, host mbox paddr base: 0x%x(host_axi_base:0x%x) \n", *mbox_vaddr, host_axi_ddr_base);
        if (!drv->pci_mbox_paddr) {
            //translate into host pci address
            drv->pci_mbox_paddr = (*mbox_vaddr - host_axi_ddr_base) +  PCI_DDR_HOST_BASE;
            drv->pci_mbox_kaddr = (unsigned int *)ioremap_nocache(drv->pci_mbox_paddr, TOTAL_MBOX_SIZE);
            if (drv->pci_mbox_kaddr == NULL)
                panic("%s, 3\n", __func__);
        }
        __iounmap((void *)mbox_vaddr);
        printk("--- pci mbox paddr: 0x%x, vaddr: 0x%x --- \n", drv->pci_mbox_paddr, (u32)drv->pci_mbox_kaddr);
    } else {
        /* Local */
        mbox_vaddr = (u32 *)ioremap_nocache(ZEBRA_MBOX_PADDR, 4);
        if (mbox_vaddr == NULL) panic("%s, 5\n", __func__);

        if (drv->mbox_host) {
            if (!drv->mbox_kaddr || !drv->mbox_paddr)
                panic("%s, bug! \n", __func__);
            /* give the mbox memory and everyone will see this address of mbox */
            *mbox_vaddr = drv->mbox_paddr;
        } else {
            int i = 0;

            if (!drv->mbox_paddr && !drv->mbox_kaddr) {
                printk("Wait Host to provide the local mbox .");
                while ((ioread32(mbox_vaddr) == 0) || (ioread32(mbox_vaddr) == 0xFFFFFFFF)) {
                    msleep(500);
                    i ++;
                    if (i & 1)
                        printk(".");
                }
                printk("done \n");
                drv->mbox_paddr = *mbox_vaddr;
                drv->mbox_kaddr = (unsigned int *)ioremap_nocache(drv->mbox_paddr, TOTAL_MBOX_SIZE);
                if (drv->mbox_kaddr == NULL)
                    panic("%s, 2\n", __func__);
            } /* if */
        } /* drv->mbox_host */
        printk("--- mbox paddr: 0x%x, vaddr: 0x%x --- \n", drv->mbox_paddr, (u32)drv->mbox_kaddr);
    } /* IS_PCI(brd) */
}

/*
 * Init function
 */
int __init zebra_init(int mbox_host, zebra_dma_t dma_type, int ep_cnt, int non_ep_cnt, u32 pool_sz)
{
    zebra_driver_t *drv;
    int  size;

    LOG_DEBUG("<init> called.\n");

    /* mbox size check */
    if (TOTAL_MBOX_SIZE < ONE_MBOX_SIZE * 4) {
        panic("%s, the total mbox size: %d is lesser than %d! \n", __func__, (u32)TOTAL_MBOX_SIZE,
                        (u32)ONE_MBOX_SIZE * 4);
    }

    if (ONE_MBOX_SIZE < sizeof(mbox_t)) {
        panic("%s, each ONE_MBOX_SIZE: %d is lesser than %d! \n", __func__,
                ONE_MBOX_SIZE, sizeof(mbox_t));
    }

    /* each device consumes 4 bytes for the rx_ready/tx_empty status */
    if ((ZEBRA_STATUS_BUFFER_SIZE >> 2) < ZEBRA_DEVICE_MAX)
        panic("%s, the status buffer is not enough! \n", __func__);

    size = PAGE_ALIGN(sizeof(zebra_driver_t));
    zebra_driver = drv = kmalloc(size, GFP_ATOMIC);
    if (unlikely(!drv))
        panic("%s, no memory! \n", __func__);

    printk("----------> %s, zebra_drv address: 0x%x, pa: 0x%x, size: %d <----------\n",
                                    __func__, (u32)drv, (u32)__pa(drv), size);

    memset(drv, 0, size);
    INIT_LIST_HEAD(&drv->board_list);

    /* memory init */
    if (pool_sz == 0)
        pool_sz = 10 * PAGE_SIZE;    //for dma pool use, just random large memory

    if (mem_alloc_init(pool_sz, "vm_device"))
        panic("%s, no memory with size: %d \n", __func__, pool_sz);

    //just init
    pci_mem_alloc_init(0, 0, "none");

    if (mem_get_poolinfo(&drv->local_pool_paddr, &drv->local_pool_sz))
        panic("%s, error! \n", __func__);

    /* Create the buffer entry cache */
    LOG_DEBUG(" - create the buffer cache\n");

    size = sizeof(struct buffer_entry);
	drv->buffer_cache = kmem_cache_create(
		"zebra_entry",
		size,         /* size of each allocation */
		0,            /* offset into page */
		0,            /* no allocation flags */
		NULL          /* constructor */
	);
	if (drv->buffer_cache == NULL) {
		LOG_ERROR(" - buffer cache creation failed\n");
		panic("%s, create buffer cache fail! \n", __func__);
	}

	/* Create the work-queue */
	LOG_DEBUG(" - create the work-queue\n");
	drv->work_queue = create_singlethread_workqueue(
			(char *)zebra_name);
	if (drv->work_queue == 0) {
		LOG_ERROR(" - work-queue creation failed\n");
		panic("%s, create work-queue fail! \n", __func__);
	}

    drv->pci_host = (platform_get_pci_type() == PCIX_HOST) ? 1 : 0;
    drv->dma_type = dma_type;
    drv->mbox_host = mbox_host;
    //prepare PCI data
    drv->pci.axi_base_addr = drv->local_pool_paddr & PCI_BAR_MASK;  //alignment to bar size
    drv->pci.pci_base_addr = IS_PCI_HOST(drv) ? PCI_DDR_HOST_BASE : PCI_DDR_DEVICE_BASE;

    /* sanity check */
    if ((dma_type == ZEBRA_DMA_NONE) && !drv->pci_host) {
        /* Notice:
         * Because the PCI device memory comes from HOST side not from Linux.
         * In order to speed up the performance we use cache copy instead.
         * The cache vaddr is from __va() directly, it will cause problem in
         * cpu copy due to __va() not exist. Thus we only can use DMAC.
         */
        printk("only FA726 of PCI_HOST can use CPU COPY, others must use DMAC! \n");
        panic("%s, dma type:%d is wrong, please correct it! \n", __func__, dma_type);
    }

    if (IS_PCI_HOST(drv) && IS_MBOX_HOST(drv)) {
        u32 end_addr, free_space;

        if (!(ep_cnt + non_ep_cnt)) {
            printk("%s, ep_cnt + non_ep_cnt = 0!\n", __func__);
            return -1;
        }

        /*
         * Host provides the memory to PCI EPs
         */
        /* calculate each instance size */
        pci_device_memory_poolsz = drv->local_pool_sz / ((non_ep_cnt + ep_cnt) + ep_cnt);
        pci_device_next_memory_paddr = drv->local_pool_paddr +
                            (non_ep_cnt + ep_cnt) * pci_device_memory_poolsz;
        /* adjust the pool size */
        mem_alloc_pool_resize(pci_device_memory_poolsz * (non_ep_cnt + ep_cnt));

        /* create mbox */
        if (drv->mbox_kaddr)
            panic("%s, duplicate mbox allocation! \n", __func__);

        drv->mbox_kaddr = (u32 *)mem_alloc(TOTAL_MBOX_SIZE, 4, &drv->mbox_paddr, MEM_TYPE_NCNB);
        if (drv->mbox_kaddr == NULL)
            panic("%s, bug! \n", __func__);

        memset((char *)drv->mbox_kaddr, 0, TOTAL_MBOX_SIZE);  //clear to zero

        /* across the PCI_BAR_SIZE region? */
        end_addr = drv->pci.axi_base_addr + PCI_BAR_SIZE;
        if ((drv->local_pool_paddr + drv->local_pool_sz) > end_addr) {
            panic("%s, the end address is 0x%x across the region 0x%x ~ 0x%x\n",
                __func__, drv->local_pool_paddr + drv->local_pool_sz,
                drv->pci.axi_base_addr, end_addr);
        } else {
            free_space = end_addr - (drv->local_pool_paddr + drv->local_pool_sz);
        }

        printk("cpucomm: PCI host, pool paddr:0x%x, axi base:0x%x, pci DDR base:0x%x, <guard:0x%x>\n",
            drv->local_pool_paddr, drv->pci.axi_base_addr, drv->pci.pci_base_addr, free_space);
    }

    /* host core of PCI DEVICE
     */
    if (!IS_PCI_HOST(drv) && IS_MBOX_HOST(drv)) {
        drv->mbox_kaddr = (u32 *)mem_alloc(TOTAL_MBOX_SIZE, 4, &drv->mbox_paddr, MEM_TYPE_NCNB);
        if (drv->mbox_kaddr == NULL)
            panic("%s, bug or no memory! \n", __func__);

        memset((char *)drv->mbox_kaddr, 0, TOTAL_MBOX_SIZE);  //clear to zero
    }

#ifndef WIN32
	/* Dynamic /dev creation
	 *
	 * This creates entries in /dev and /sys/class/zebra
	 */
	LOG_DEBUG(" - create the sysfs %s class\n", zebra_name);

	drv->class = class_create(THIS_MODULE, (char *)zebra_name);
	if (drv->class == NULL) {
		LOG_ERROR(" - class creation failed\n");
		panic("%s, create class fail! \n", __func__);
	}
#endif /* WIN32 */

    if (drv->dma_type != ZEBRA_DMA_NONE) {
        mem_llp_alloc_init(4 * PAGE_SIZE, 2 * ZEBRA_NUM_IN_PACK * sizeof(dmac_llp_t));  //2: include dummy dma
        dma_llp_init();
    }

    LOG_DEBUG("<init> done.\n");

    return 0;
}

static int get_cpu_idx(u32 cpu_id)
{
    int idx;

    switch (cpu_id) {
      case FA726_CORE_ID:
        idx = 0;
        break;
      case FA626_CORE_ID:
        idx = 1;
        break;
      case FC7500_CORE_ID:
        idx = 2;
        break;
      case PCIDEV_FA726_CORE_ID:
        idx = 3;
        break;
      case PCIDEV_FA626_CORE_ID:
        idx = 4;
        break;
      default:
        panic("%s, unknown cpu id: 0x%x \n", __func__, cpu_id);
        break;
    }

    return idx;
}

/*
 * host cpu core within PCI Host will provide the memory pool to the PCI device
 */
void zebra_pcidev_set_mempool(zebra_board_t *brd)
{
    zebra_driver_t *drv = brd->driver;
    mbox_t  *mbox;
    int device_idx = -1;

    if (!IS_PCI(brd) || !IS_PCI_HOST(drv) || !IS_MBOX_HOST(drv))
        return;

    /* sanity check */
    if (!pci_device_next_memory_paddr || !pci_device_memory_poolsz)
        panic("bug in %s, addr: 0x%x, size: 0x%x \n", __func__,
		        pci_device_next_memory_paddr, pci_device_memory_poolsz);
    /*
     * Note: the memory in the mbox is PCI address.
     */
    mbox = (mbox_t *)brd->mbox_addr.vaddr;

    device_idx = get_cpu_idx(brd->peer_cpu_id);
    //inform the peer about its address in host memory. It is PCI address from device's view
    mbox->data[device_idx].mempool_paddr = TO_PCI_ADDR(pci_device_next_memory_paddr, drv, brd);
    mbox->data[device_idx].mempool_size = pci_device_memory_poolsz;

    printk("cpucomm: host gives PCI memory paddr: 0x%x(loc:0x%x), size: 0x%x to board: %s \n",
        mbox->data[device_idx].mempool_paddr, pci_device_next_memory_paddr,
        mbox->data[device_idx].mempool_size, brd->name);

    /* update pci_device_next_avail_pool_paddr to next */
    pci_device_next_memory_paddr += pci_device_memory_poolsz;
}

/*
 * Tell the peer about my share memory phyaddr
 */
void zebra_set_Iam_ready(zebra_board_t *brd)
{
    mbox_t  *mbox;
    u32     idx;
    zebra_driver_t *drv = brd->driver;

    /* tell the peer about my pool address and share memory address
     * Note: here we will translate the memory into PCI which the peer's view. Because cpucomm
     *       just pass the source pointer to the sharemem to the peer and one will use this PCI
     *       address as the source to move the data to its destination.
     */
#ifndef WIN32
    /* set inbound axi_base address first and then tell all my information in the mbox */
    if (IS_PCI(brd)) {
        platform_set_inbound_win(brd, drv->pci.axi_base_addr);
    }
#endif
    mbox = (mbox_t *)brd->mbox_addr.vaddr;

    idx = get_cpu_idx(brd->local_cpu_id);

    if (IS_PCI_DEVICE(drv, brd)) {
        /* I am the PCI device, the memory comes from PCI host.
         * Note: It is pci address already due to the memory from PCI Host.
         */
        mbox->data[idx].paddr = brd->local_map_mem.paddr;
        mbox->data[idx].size = brd->local_map_mem.size;
        /* error checking */
        if (mbox->data[idx].mempool_paddr != drv->pci_pool_paddr)
            panic("%s, addr bug! 0x%x, 0x%x \n", __func__, mbox->data[idx].mempool_paddr, drv->pci_pool_paddr);
        if (mbox->data[idx].mempool_size != drv->pci_pool_sz)
            panic("%s, size bug! 0x%x, 0x%x \n", __func__, mbox->data[idx].mempool_size, drv->pci_pool_sz);
    } else {
        mbox->data[idx].paddr = TO_PCI_ADDR(brd->local_map_mem.paddr, drv, brd);
        mbox->data[idx].size = brd->local_map_mem.size;
        mbox->data[idx].mempool_paddr = TO_PCI_ADDR(drv->local_pool_paddr, drv, brd);
        mbox->data[idx].mempool_size = drv->local_pool_sz;
    }
    mbox->data[idx].magic_number = MAGIC_NUMBER;
    mbox->data[idx].id = brd->local_cpu_id;
}

/* Wait the peer to tell its share memory phyaddr
 */
void zebra_wait_peer_ready(zebra_board_t *brd)
{
    zebra_driver_t *drv = brd->driver;
    volatile mbox_t  *mbox;
    u32 idx, round = 10000;

    LOG_DEBUG(" - wait %s core ready.\n", brd->name);

    mbox = (mbox_t *)brd->mbox_addr.vaddr;

    idx = get_cpu_idx(brd->peer_cpu_id);
    printk("Wait %s core ready.", brd->name);
    do {
        printk(".");
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(msecs_to_jiffies(500));
        round --;
        if (round == 0)
            panic("Wait %s core timeout! \n", brd->name);
    } while ((mbox->data[idx].magic_number != MAGIC_NUMBER) || (mbox->data[idx].id != brd->peer_cpu_id));
    printk("done \n");

    /* get the peer's memory information
     */
    if (IS_PCI(brd)) {
        /* The address which the host given is pci address, so we need to translate it into AXI address again */
        brd->peer_map_mem.paddr = IS_PCI_HOST(drv) ?
                TO_AXI_ADDR(mbox->data[idx].paddr, drv, brd) : mbox->data[idx].paddr;
        brd->peer_map_mem.size = mbox->data[idx].size;
        brd->pool_mem.paddr = IS_PCI_HOST(drv) ?
                TO_AXI_ADDR(mbox->data[idx].mempool_paddr, drv, brd) : mbox->data[idx].mempool_paddr;
        brd->pool_mem.size = mbox->data[idx].mempool_size;
    } else {
        /* general case */
        brd->peer_map_mem.paddr = mbox->data[idx].paddr;
        brd->peer_map_mem.size = mbox->data[idx].size;
        //the information of pool of the peer
        brd->pool_mem.paddr = mbox->data[idx].mempool_paddr;
        brd->pool_mem.size = mbox->data[idx].mempool_size;
    }

    //if (brd->pool_mem.paddr && brd->pool_mem.size) {
    if (brd->pool_mem.size) {   //FC7500 physical starts from 0
        brd->pool_mem.vaddr = (u32)ioremap_nocache(brd->pool_mem.paddr, brd->pool_mem.size);
        if (unlikely(!brd->pool_mem.vaddr))
            panic("%s no virtual memory for %s core with pool size:0x%x \n", __func__, brd->name,
                                            brd->pool_mem.size);

        brd->peer_map_mem.vaddr = brd->pool_mem.vaddr + brd->peer_map_mem.paddr - brd->pool_mem.paddr;

#ifdef CACHE_COPY
        /* mapping whole paddr */
        brd->pool_mem_cache = (u32)ioremap_cached(brd->pool_mem.paddr, brd->pool_mem.size);
        if (!brd->pool_mem_cache)
            panic("cpucomm: %s, no virtual address for size: %d \n", __func__, brd->pool_mem.size);
#endif /* CACHE_COPY */

        printk("cpucomm: board %s, PoolMem:(paddr:0x%x, size:0x%x, vaddr:0x%x, cbaddr:0x%x). ShareMem:(paddr:0x%x, size:0x%x, vaddr:0x%x)\n",
                brd->name, brd->pool_mem.paddr, brd->pool_mem.size, brd->pool_mem.vaddr, brd->pool_mem_cache,
                brd->peer_map_mem.paddr, brd->peer_map_mem.size, brd->peer_map_mem.vaddr);
    } else {
        panic("cpucomm: board %s, didn't provide pool address: 0x%x or size: %d \n",
                        brd->name, brd->pool_mem.paddr, brd->pool_mem.size);
    }

    return;
}

/* host provides the pci memory pool already, now it is time to get */
static void zebra_pcidev_get_mempool(zebra_board_t *brd)
{
    mbox_t  *mbox;
    u32     idx;
    zebra_driver_t *drv = brd->driver;

    if (!IS_PCI_DEVICE(drv, brd))
        return;

    mbox = (mbox_t *)brd->mbox_addr.vaddr;

    idx = get_cpu_idx(brd->local_cpu_id);
    drv->pci_pool_paddr = mbox->data[idx].mempool_paddr;
    drv->pci_pool_sz = mbox->data[idx].mempool_size;
    if (!drv->pci_pool_paddr)
        panic("%s, drv->pci_pool_paddr is zero! \n", __func__);

    printk("cpucomm: PCI Device get memory paddr: 0x%x, size: 0x%x from host: %s \n",
                            drv->pci_pool_paddr, drv->pci_pool_sz, brd->name);

}

/* allocate the share memory for data communication */
int zebra_resource_claim(zebra_board_t *brd)
{
    int size;

    LOG_DEBUG(" - %s\n", __func__);

    /* allocate the mapped memory for data communication.
     */
    size = PAGE_ALIGN(ZEBRA_TOTAL_MEMSZ);

    if (IS_PCI_DEVICE(brd->driver, brd))
        brd->local_map_mem.vaddr = (u32)pci_mem_alloc(size, 4, &brd->local_map_mem.paddr, MEM_TYPE_NCNB);
    else
        brd->local_map_mem.vaddr = (u32)mem_alloc(size, 4, &brd->local_map_mem.paddr, MEM_TYPE_NCNB);

    if (unlikely(!brd->local_map_mem.vaddr))
        panic("%s, error to allocate virtual memory!\n", __func__);
    brd->local_map_mem.size = size;

    //clear to zero
    memset((void *)brd->local_map_mem.vaddr, 0, size);

    return 0;
}

void zebra_claim_device_buffer(int dev_id, zebra_device_t *dev, zebra_board_t *brd)
{
    buffer_list_t *write_list, *read_list;

    write_list = &dev->write_list;
    read_list = &dev->read_list;

    if (dev_id >= ZEBRA_DEVICE_MAX || dev_id >= ARRAY_SIZE(bufmap) || bufmap[dev_id].device != dev_id)
        panic("%s, dev_id %d is out of range! \n", __func__, dev_id);

    dev->write_paddr = brd->peer_map_mem.paddr + bufmap[dev_id].write_ofs;
    dev->write_kaddr = (char *)(brd->peer_map_mem.vaddr + bufmap[dev_id].write_ofs);
    dev->read_paddr = brd->local_map_mem.paddr + bufmap[dev_id].read_ofs;
    dev->read_kaddr = (char *)(brd->local_map_mem.vaddr + bufmap[dev_id].read_ofs);

    bufmap[dev_id].bufsz = MAX_BUFFSZ; //4M

    dev->mtu = bufmap[dev_id].bufsz;

    /* Max buffer size can be read/write to SDRAM. */
    write_list->buffer_length_max = bufmap[dev_id].bufsz;
    read_list->buffer_length_max = bufmap[dev_id].bufsz;

    /* assign the door bell */
    dev->rx_ready = RX_READY;
    dev->tx_empty = TX_EMPTY;

 	/* clear the header of the real hardware buffer */
 	memset((char *)dev->read_kaddr, 0x0, sizeof(share_memory_t));
 	memset((char *)dev->write_kaddr, 0x0, sizeof(share_memory_t));
}

/* Initialize the zebra devices on a board */
int zebra_devices_init(zebra_board_t *brd)
{
    zebra_device_t      *dev;
    buffer_list_t       *write_list, *read_list;
    transmit_list_t     *transmit_list;
    share_memory_t      sharemem;
    int                 sharemem_sz, i;

    LOG_DEBUG(" - %s\n", __func__);

    /* This board supports multiple channels */
	for (i = ZEBRA_DEVICE_0; i < ZEBRA_DEVICE_MAX; i++) {
	    dev = &brd->device[i];
	    /* Board reference */
		dev->board = brd;
		/* Device Index Reference */
		dev->device_id = i;
		/* Initialize the fsync wait-queue */
		init_waitqueue_head(&dev->fsync_wait);

		/* Initialize the write list
		 */
		write_list = &dev->write_list;
		INIT_LIST_HEAD(&write_list->list);
		sema_init(&write_list->sem, 1);
		init_waitqueue_head(&write_list->wait);
		write_list->list_length_max = bufmap[i].max_list_length;

        /* buffer size check */
        sharemem_sz = sizeof(share_memory_t) +
                                    (bufmap[i].max_list_length - 1) * sizeof(sharemem.mem_entry[0][0]);
        if (sharemem_sz > bufmap[i].bufsz)
            panic("%s, share memory is too small(%d, %d)! \n", __func__,
                                            sharemem_sz, bufmap[i].bufsz);

		/* Initialize the read list
		 */
		read_list = &dev->read_list;
		INIT_LIST_HEAD(&read_list->list);
		sema_init(&read_list->sem, 1);
		init_waitqueue_head(&read_list->wait);
		read_list->list_length_max = bufmap[i].max_list_length;

        /* Initialized the transmitting list
         */
        transmit_list = &dev->wr_transmit_list;
        INIT_LIST_HEAD(&transmit_list->list);

        transmit_list = &dev->rd_transmit_list;
        INIT_LIST_HEAD(&transmit_list->list);

		/* Initialize the work structs */
        INIT_WORK(&dev->write_request, zebra_write_workfn);
        INIT_WORK(&dev->pack_write_request, zebra_pack_write_workfn);
        INIT_WORK(&dev->read_request, zebra_read_workfn);
        INIT_WORK(&dev->pack_read_request, zebra_pack_read_workfn);
        INIT_WORK(&dev->write_error, zebra_write_error);
#ifdef USE_KTRHEAD
        dev->read_thread = kthread_create(zebra_read_thread, dev, "zebra_read_thread");
        if (IS_ERR(dev->read_thread))
            panic("%s, create read thread%d fail1 ! \n", __func__, dev->device_id);
        dev->write_thread = kthread_create(zebra_write_thread, dev, "zebra_write_thread");
        if (IS_ERR(dev->write_thread))
            panic("%s, create write thread%d fail1 ! \n", __func__, dev->device_id);
        init_waitqueue_head(&dev->read_thread_wait);
        init_waitqueue_head(&dev->write_thread_wait);
        wake_up_process(dev->read_thread);
        wake_up_process(dev->write_thread);
#endif /* USE_KTRHEAD */
        sema_init(&dev->write_mutex, 1);
		sema_init(&dev->write_busy, 1);
		sema_init(&dev->read_mutex, 1);
		sema_init(&dev->read_busy, 1);

		/* Initialize the write-timeout timer */
		init_timer(&dev->write_timeout);

        /* Initialize the read-timeout timer */
        init_timer(&dev->read_timeout);

        /* setup the master and slave rx_ready and tx_empty status addresses, each field is 32bits
         * Note, the status memory is kept in BOTH side.
		 *
		 * From host view,
         *  when the local sends RX_READY, it set the flag in peer_rx_ready_status_kaddr.
         *  when the local sends TX_EMTPY, it set the flag in peer_tx_empty_status_kaddr.
         *  when the local receives RX_READY, it checks the flag from local_rx_ready_status_kaddr.
         *  when the local receives TX_EMPTY, it checks the flag from local_tx_empty_status_kaddr.
         */
	    dev->local_rx_ready_status_paddr = brd->local_map_mem.paddr + ZEBRA_RX_READY_STATUS_ADDR + (i<<2);
		dev->local_rx_ready_status_kaddr = (char *)(brd->local_map_mem.vaddr + ZEBRA_RX_READY_STATUS_ADDR + (i << 2));
		dev->peer_rx_ready_status_paddr = brd->peer_map_mem.paddr + ZEBRA_RX_READY_STATUS_ADDR + (i << 2);
		dev->peer_rx_ready_status_kaddr = (char *)(brd->peer_map_mem.vaddr + ZEBRA_RX_READY_STATUS_ADDR + (i << 2));
		dev->local_tx_empty_status_paddr = brd->local_map_mem.paddr + ZEBRA_TX_EMPTY_STATUS_ADDR + (i << 2);
		dev->local_tx_empty_status_kaddr = (char *)(brd->local_map_mem.vaddr + ZEBRA_TX_EMPTY_STATUS_ADDR + (i << 2));
		dev->peer_tx_empty_status_paddr = brd->peer_map_mem.paddr + ZEBRA_TX_EMPTY_STATUS_ADDR + (i << 2);
		dev->peer_tx_empty_status_kaddr = (char *)(brd->peer_map_mem.vaddr + ZEBRA_TX_EMPTY_STATUS_ADDR + (i << 2));

		/* reset the related rx_ready and tx_empty status */
		writel(0, dev->local_rx_ready_status_kaddr);
		writel(0, dev->peer_rx_ready_status_kaddr);
		writel(0, dev->local_tx_empty_status_kaddr);
		writel(0, dev->peer_tx_empty_status_kaddr);

        dev->read_event = 0;
        dev->attribute = ATTR_PACK; //default value

		zebra_claim_device_buffer(i, dev, brd);
	}

	return 0;
}

/* DOORBELL_RXREADY: rcv RX_READY from the peer
 * DOORBELL_TXEMPTY: rcv TX_EMPTY from the peer
 */
void zebra_process_doorbell(zebra_board_t *brd)
{
    unsigned int status, i;
    zebra_device_t *dev;

    /*
     * process TX_EMPTY
     */
    status = readl(brd->local_tx_empty_status_kaddr);
    if (status == DOORBELL_TXEMPTY) {
        int bFound = 0;

        LOG_DEBUG(" - get DOORBELL_TXEMPTY interrupt\n");

        /* clear the gather flag */
        writel(0, brd->local_tx_empty_status_kaddr);

        for (i = 0; i < ZEBRA_DEVICE_MAX; i++) {
			dev = &brd->device[i];
			status = readl(dev->local_tx_empty_status_kaddr);
			if (status != dev->tx_empty)
			    continue;

            /* clear the flag */
            writel(0, dev->local_tx_empty_status_kaddr);

			/* receive the TX_EMPTY, we can say the SDRAM is clear already.
             */
            if ((u32)&dev->pack_write_request > 0xF0000000 || (u32)&dev->write_request > 0xF0000000) {
                printk("%s, #444444 1111111 brd: %s(0x%p), dev: %d(0x%p), ####################\n",
                                                    __func__, brd->name,  brd, dev->device_id, dev);
                dump_stack();
                printk("%s, #444444 done \n", __func__);
            }

            if (dev->attribute & ATTR_PACK)
                zebra_schedule_work(brd->driver, &dev->pack_write_request, dev);
            else
                zebra_schedule_work(brd->driver, &dev->write_request, dev);

            /* interrupt count for this device */
	        dev->counter.rcv_txempty_interrupts ++;
	        bFound = 1;
		} /* for */
		if (!bFound) {
		    //printk("%s, interrupt is not sync for H2D_TXEMPTY! \n", __func__);
		    brd->internal_error ++;
		}
    } /* if */

    /*
     * process RX_READY
     */
    status = readl(brd->local_rx_ready_status_kaddr);
    if (status == DOORBELL_RXREADY) {
        int bFound = 0;

        LOG_DEBUG(" - get DOORBELL_RXREADY interrupt\n");

        /* clear the gather flag */
        writel(0, brd->local_rx_ready_status_kaddr);

        for (i = 0; i < ZEBRA_DEVICE_MAX; i ++) {
            dev = &brd->device[i];
            status = readl(dev->local_rx_ready_status_kaddr);
            if (status != dev->rx_ready)
                continue;
            /* clear the flag */
            writel(0, dev->local_rx_ready_status_kaddr);
            /* peer -> local data to read */
    	    LOG_DEBUG(" - dev%d, schedule read-request\n", dev->device_id);

            if ((u32)&dev->pack_read_request > 0xF0000000 || (u32)&dev->read_request > 0xF0000000) {
                printk("%s, #444444 2222222 brd: %s(0x%p), dev: %d(0x%p), ####################\n",
                                                    __func__, brd->name,  brd, dev->device_id, dev);
                dump_stack();
                printk("%s, #444444 done \n", __func__);
            }

            /* schedule a read request workq */
            if (dev->attribute & ATTR_PACK)
                zebra_schedule_work(brd->driver, &dev->pack_read_request, dev);
            else
                zebra_schedule_work(brd->driver, &dev->read_request, dev);

            /* interrupt count for this device */
	        dev->counter.rcv_rxrdy_interrupts ++;
	        bFound = 1;
        } /* for */
        if (!bFound) {
		    //printk("%s, interrupt is not sync for RXREADY! \n", __func__);
		    brd->internal_error ++;
		}
    } /* if */
}

/* probe function comes from platform driver register */
int zebra_probe(mbox_dev_t *mbox_dev)
{
    zebra_driver_t  *drv = zebra_driver;
    zebra_board_t   *brd;
    int status, size;

	LOG_DEBUG(" - %s\n", __func__);

    /* board_id is duplicated? */
    list_for_each_entry(brd, &zebra_driver->board_list, list) {
        if (brd->board_id == mbox_dev->identifier) {
            LOG_ERROR(" - The board_id: 0x%x is duplicated! \n", brd->board_id);
            return -1;
        }
    }
    size = PAGE_ALIGN(sizeof(zebra_board_t));
    /* Create a new board */
	brd = kmalloc(size, GFP_KERNEL);
	if (!brd)
	    panic("%s, no memory for board! \n", __func__);

    printk("----------> %s, zebra_brd: %s address: 0x%x, pa: 0x%x, size:%d <----------\n", __func__,
                    mbox_dev->name, (u32)brd, (u32)__pa(brd), size);

    memset(brd, 0, PAGE_ALIGN(sizeof(zebra_board_t)));
    brd->board_id = mbox_dev->identifier;   //unique id
    brd->peer_cpu_id = mbox_dev->peer_cpu_id;
    brd->local_cpu_id = mbox_dev->local_cpu_id;
	brd->driver = zebra_driver;
	brd->local_irq = mbox_dev->local_irq;
	brd->peer_irq = mbox_dev->peer_irq;
	mbox_dev->private_data = (void *)brd;
	strcpy(brd->name, mbox_dev->name);

#ifndef WIN32
    if (IS_PCI(brd))
        platform_pci_init(brd);
#endif

    /* setup the mbox. mbox only can be created by cpu core host in PCI Host */
    zebra_setup_mbox(drv, brd);

    if (IS_PCI_HOST(drv)) {
        brd->mbox_addr.paddr = drv->mbox_paddr + mbox_dev->mbox_ofs;
    	brd->mbox_addr.vaddr = (u32)drv->mbox_kaddr + mbox_dev->mbox_ofs;
    } else {
        if (IS_PCI(brd)) {
        	brd->mbox_addr.paddr = drv->pci_mbox_paddr + mbox_dev->mbox_ofs;
        	brd->mbox_addr.vaddr = (u32)drv->pci_mbox_kaddr + mbox_dev->mbox_ofs;
        } else {
            brd->mbox_addr.paddr = drv->mbox_paddr + mbox_dev->mbox_ofs;
        	brd->mbox_addr.vaddr = (u32)drv->mbox_kaddr + mbox_dev->mbox_ofs;
        }
    }
	brd->mbox_addr.size = ONE_MBOX_SIZE;

    if (IS_PCI_DEVICE(drv, brd)) {
        /*
         * PCI device. Wait host ready and give pool memory to the PCI device
         */
        /* 1. wait PCI host give the pool address */
        zebra_wait_peer_ready(brd);
        /* 1.1. get the pool base now, it is pci memory base */
        zebra_pcidev_get_mempool(brd);
        /* 2. set it the memory pool for management */
        pci_mem_alloc_init(drv->pci_pool_paddr, drv->pci_pool_sz, "pci_mem");
        /* 3. set local_map_mem, it will call pci_mem_alloc() */
        status = zebra_resource_claim(brd);
        if (status) {
            LOG_ERROR(" - shared memory resource acquisition failed\n");
            return -1;
        }
        /* 4. inform the host I am ready */
        zebra_set_Iam_ready(brd);
    } else {
        status = zebra_resource_claim(brd);
        if (status) {
            LOG_ERROR(" - shared memory resource acquisition failed\n");
            return -1;
        }

        /* PCI host or cpu cores within a chip
         */
        /* pci host give the pool address to the pci device first */
        zebra_pcidev_set_mempool(brd);
        /* set the allocated share memory to the mbox to inform the peer I am ready */
        zebra_set_Iam_ready(brd);
        /* wait peer ready */
        zebra_wait_peer_ready(brd);
    }

    /* Initialize the ZEBRA devices
	 * Note:
	 *   The share memory should be allocated first in order to create the channels.
	 */
	LOG_DEBUG(" - initialize the ZEBRA devices\n");
	status = zebra_devices_init(brd);
	if (status < 0) {
		LOG_ERROR(" - device initialization failed\n");
		return status;
	}

	init_timer(&brd->fire_rx_ready);
	init_timer(&brd->fire_tx_empty);

	/* initialize the board level rx_ready and tx_empty status address */
	brd->local_rx_ready_status_paddr = brd->local_map_mem.paddr + ZEBRA_GATHER_RX_READY_STATUS_ADDR;
	brd->local_rx_ready_status_kaddr = (char *)(brd->local_map_mem.vaddr + ZEBRA_GATHER_RX_READY_STATUS_ADDR);

	brd->peer_rx_ready_status_paddr = brd->peer_map_mem.paddr + ZEBRA_GATHER_RX_READY_STATUS_ADDR;
	brd->peer_rx_ready_status_kaddr = (char *)(brd->peer_map_mem.vaddr + ZEBRA_GATHER_RX_READY_STATUS_ADDR);

	brd->local_tx_empty_status_paddr = brd->local_map_mem.paddr + ZEBRA_GATHER_TX_EMPTY_STATUS_ADDR;
    brd->local_tx_empty_status_kaddr = (char *)(brd->local_map_mem.vaddr + ZEBRA_GATHER_TX_EMPTY_STATUS_ADDR);

	brd->peer_tx_empty_status_paddr = brd->peer_map_mem.paddr + ZEBRA_GATHER_TX_EMPTY_STATUS_ADDR;
	brd->peer_tx_empty_status_kaddr = (char *)(brd->peer_map_mem.vaddr + ZEBRA_GATHER_TX_EMPTY_STATUS_ADDR);

	/* reset the related rx_ready and tx_empty status */
	writel(0, brd->local_rx_ready_status_kaddr);
	writel(0, brd->peer_rx_ready_status_kaddr);
	writel(0, brd->local_tx_empty_status_kaddr);
	writel(0, brd->peer_tx_empty_status_kaddr);

	/*
	 * Initialize the transaction list done
	 */

	/* Claim a block of minor numbers */
	zebra_minor_range_claim(brd);

	/* Register a character device */
	zebra_device_register(brd);

	/* init the list of node */
    INIT_LIST_HEAD(&brd->list);

    /* add to board_list */
    list_add_tail(&brd->list, &zebra_driver->board_list);

     /* init the doorbell */
    LOG_DEBUG(" - board id: 0x%x, door bell init \n", brd->board_id);
    platform_doorbell_init(brd);

    /* create proc */
#ifndef WIN32
    zebra_proc_init(brd, "cpu_comm_");
#endif

    /* the peer needs time to fully initialization */
    msleep(900);

    brd->devices_initialized = 1;

    return 0;
}

/* Make sure the device lists are empty */
void zebra_devices_destroy(zebra_board_t *brd)
{
    zebra_device_t *dev;
	buffer_list_t *list;
	buffer_entry_t *entry, *ne;
	transmit_list_t *transmit_list;
	int i;

	LOG_DEBUG(" - %s\n", __func__);

	for (i = 0; i < ZEBRA_DEVICE_MAX; i++) {
		dev = &brd->device[i];

		/* Cancel the write timeout timer */
		if (timer_pending(&dev->write_timeout)) {
			del_timer(&dev->write_timeout);
		}

		/* Empty the write list */
		list = &dev->write_list;
		down(&list->sem);

        while(list->list_length) {
			entry = list_entry(list->list.prev, buffer_entry_t, list);
			list_del(list->list.prev);
			list->list_length--;
			zebra_buffer_entry_destroy(entry);
		}
		up(&list->sem);

        transmit_list = &dev->wr_transmit_list;
        list_for_each_entry_safe(entry, ne, &transmit_list->list, list) {
            transmit_list->list_length --;
            zebra_buffer_entry_destroy(entry);
        }

		/* Empty the read list */
		list = &dev->read_list;
		down(&list->sem);
		while(list->list_length > 0) {
			entry = list_entry(list->list.prev, buffer_entry_t, list);
			list_del(list->list.prev);
			list->list_length--;
			zebra_buffer_entry_destroy(entry);
		}
		up(&list->sem);

		/* Delete a read in progress */
		down(&dev->read_mutex);
		entry = dev->read_entry;
		if (entry) {
			zebra_buffer_entry_destroy(entry);
		}
		dev->read_entry = 0;
		up(&dev->read_mutex);
	}

	LOG_DEBUG(" - %s exit\n", __func__);
	return;
}

/* remove function comes from platform driver unregister
 * this function only remove the board level relative. It may be called
 * several times.
 */
int zebra_remove(mbox_dev_t *mbox_dev)
{
    zebra_driver_t *drv;
    zebra_board_t   *brd;

    LOG_DEBUG(" - %s\n", __func__);

    brd = (zebra_board_t *)mbox_dev->private_data;
    drv = brd->driver;

    platform_pci_exit(brd);
    platform_doorbell_exit(brd);

    zebra_device_unregister(brd);

	/* Release the minor number */
	zebra_minor_range_release(brd);

    /* Clear the device lists */
	if (brd->devices_initialized) {
		zebra_devices_destroy(brd);
		brd->devices_initialized = 0;
	}

    /* Cancel the write rx_ready timer */
	if (timer_pending(&brd->fire_rx_ready)) {
		del_timer(&brd->fire_rx_ready);
	}

	/* Cancel the write tx_empty timer */
	if (timer_pending(&brd->fire_tx_empty)) {
		del_timer(&brd->fire_tx_empty);
	}

    if (brd->local_map_mem.vaddr) {
        if (IS_PCI_DEVICE(drv, brd))
            pci_mem_free((void *)brd->local_map_mem.vaddr);
        else
            mem_free((void *)brd->local_map_mem.vaddr);

        brd->local_map_mem.vaddr = 0;
    }

    if (brd->peer_map_mem.vaddr) {
        __iounmap((void *)brd->peer_map_mem.vaddr);
        brd->peer_map_mem.vaddr = 0;
    }

    if (brd->pool_mem.vaddr) {
        __iounmap((void *)brd->pool_mem.vaddr);
        brd->pool_mem.vaddr = 0;
    }

    /* delete this board from driver board_list */
    list_del(&brd->list);

    LOG_DEBUG("call proc remove \n");

#ifndef WIN32
    zebra_proc_remove(brd);
#endif

    kfree(brd);

    if (drv->dma_type != ZEBRA_DMA_NONE) {
        dma_llp_exit();
        mem_llp_alloc_exit();
    }
    mem_alloc_exit();
    pci_mem_alloc_exit();

    return 0;
}

void zebra_exit(void)
{
	zebra_driver_t *drv = zebra_driver;

	LOG_DEBUG("<exit> called.\n");

	/* Destroy the buffer cache */
	if (drv->buffer_cache) {
		LOG_DEBUG(" - freeing the buffer cache\n");
		kmem_cache_destroy(drv->buffer_cache);
		drv->buffer_cache = NULL;
	}

	/* Flush and destroy the work queue */
	if (drv->work_queue) {
		LOG_DEBUG(" - flush the work-queue\n");
		flush_workqueue(drv->work_queue);

		LOG_DEBUG(" - delete the work-queue\n");
		destroy_workqueue(drv->work_queue);
	}

#ifndef WIN32
	/* Remove /dev entries */
	if (drv->class != NULL) {
		LOG_DEBUG(" - destroy the zebra class\n");
		class_destroy(drv->class);
	}
#endif

    if (drv->mbox_host) {
        if (drv->mbox_kaddr)
            mem_free(drv->mbox_kaddr);
    } else {
        if (drv->mbox_kaddr)
            __iounmap((void *)drv->mbox_kaddr);
        if (drv->pci_mbox_kaddr)
            __iounmap((void *)drv->pci_mbox_kaddr);
    }
	/* Always log unload */
	LOG_DEBUG("module unloaded\n");
}

/* open a local channel */
int zebra_device_open(int board_id, int device_id, char *name, u32 attribute)
{
    zebra_board_t   *brd = NULL;
    zebra_device_t  *dev = NULL;
    int ret = -1;

    if (device_id >= ZEBRA_DEVICE_AP_START)
        return -1;

    list_for_each_entry(brd, &zebra_driver->board_list, list) {
        /* find the specific board */
        if (brd->board_id != board_id)
            continue;

        ret = 0;
        dev = &brd->device[device_id];

        /* use read mutex to protect the dev */
        down(&dev->read_mutex);
        if (dev->open_read) {
            if (strcmp(dev->name, name)) {
                printk("%s, this channel%d was opened by %s already.\n", __func__, device_id, dev->name);
                up(&dev->read_mutex);
                return -1;
            }
            dev->open_read ++;
            up(&dev->read_mutex);
            return 0;
        }
        dev->open_read ++;  /* in order to receive the data from lower function */
        dev->open_mode |= (ZEBRA_OPEN_READ | ZEBRA_OPEN_WRITE);
        //dev->read_event = 0;
        dev->attribute = attribute;
        strcpy(dev->name, name);
        /* clear status */
        //writel(0, dev->host_rx_ready_status_kaddr);
	    //writel(0, dev->host_tx_empty_status_kaddr);

        up(&dev->read_mutex);

        printk("%s -- %s opens a channel for read/write. \n", __func__, name);
        break;
    }

    return ret;
}

/* close a local channel */
int zebra_device_close(int board_id, int device_id)
{
    zebra_board_t   *brd = NULL;
    zebra_device_t  *dev = NULL;
    buffer_list_t   *read_list, *write_list;
    int ret = -1;

    list_for_each_entry(brd, &zebra_driver->board_list, list) {
        /* find the specific board */
        if (brd->board_id != board_id)
            continue;

        ret = 0;
        dev = &brd->device[device_id];
        down(&dev->read_mutex);

        if (!dev->open_mode || !dev->open_read) {
            up(&dev->read_mutex);
            printk("%s, this channel %d didn't be opened or closed already! \n", __func__, device_id);
            return -1;
        }
        dev->open_read --;
        if (dev->open_read) {
            up(&dev->read_mutex);
            return 0;
        }
        up(&dev->read_mutex);
retry:
        /* use read mutex to protect the dev */
        down(&dev->read_mutex);

        /* we can't close this channel until both write_list and read_list is empty
         */
        /* wait read_list to empty */
        read_list = &dev->read_list;
        down(&read_list->sem);
        if (read_list->list_length) {
            up(&dev->read_mutex);
            up(&read_list->sem);
            msleep(500);
            goto retry;
        }
        /* wake up the reader if it is blocking */
        dev->read_event = 1;
        if (dev->device_id >= ZEBRA_DEVICE_AP_START)
            wake_up_interruptible(&read_list->wait);
        else
            wake_up(&read_list->wait);

        up(&read_list->sem);

        /* wait write_list to empty */
        write_list = &dev->write_list;
        down(&write_list->sem);
        if (write_list->list_length) {
            up(&dev->read_mutex);
            up(&write_list->sem);
            msleep(500);
            goto retry;
        }
        /* wake up the writer if it is blocking */
        dev->write_event = 1;
        if (dev->device_id >= ZEBRA_DEVICE_AP_START)
            wake_up_interruptible(&write_list->wait);
        else
    	    wake_up(&write_list->wait);

    	dev->attribute = 0;
        up(&write_list->sem);
        up(&dev->read_mutex);

    	/* Note: the transaction list will be released while unloading the driver.
    	 */
        break;
    }

	if (ret < 0)
		return ret;

    /* clear status */
    //writel(0, dev->local_rx_ready_status_kaddr);
    //writel(0, dev->local_tx_empty_status_kaddr);
    dev->open_mode = 0;
    dev->name[0] = 0;
    return 0;
}

void zebra_debug_print(zebra_board_t *brd)
{
    zebra_device_t  *dev = NULL;
    buffer_list_t   *read_list, *write_list;
    transmit_list_t *wr_transmit_list, *rd_transmit_list;
    share_memory_t *sharemem;
    int device_id, on_hand;
    char *str;

    printk("*************** board name: %s, READ LIST **************** \n", brd->name);
    printk("                   read   sharemem  sharemem  xmit    xmit      on                                           \n");
    printk("id  name           list   rx_rdy    tx_empty  rx_rdy  tx_empty  hand  read_in   read_out  write_in  write_out\n");
    printk("--  -------------  -----  --------  --------  ------  --------  ----  --------  --------  --------  ---------\n");

    for (device_id = ZEBRA_DEVICE_0; device_id < ZEBRA_DEVICE_MAX; device_id ++) {
        dev = &brd->device[device_id];
        if (!dev->open_mode)
            continue;

        //read ------------------------------------------------------------------------------
        read_list = &dev->read_list;
        on_hand = dev->read_entry ? 1 : 0;

        rd_transmit_list = &dev->rd_transmit_list;
        sharemem = (share_memory_t *)dev->read_kaddr;

        printk("%-2d  %-13s  %-5d  %-8d  %-8d  %-6d  %-8d  %-4d  %-8x  %-8x  %-8x  %-9x\n",
            dev->device_id, dev->name, read_list->list_length, sharemem->hdr.rx_rdy_idx,
            sharemem->hdr.tx_empty_idx, rd_transmit_list->rx_rdy_idx, rd_transmit_list->tx_empty_idx,
            on_hand, dev->pktrd_in_jiffies, dev->pktrd_out_jiffies, dev->pktwr_in_jiffies, dev->pktwr_out_jiffies);
    }

    printk("\n*************** board name: %s, WRITE LIST **************** \n", brd->name);
    printk("                   write  xmit   sharemem  sharemem  xmit    xmit           \n");
    printk("id  name           list   list   rx_rdy    tx_empty  rx_rdy  tx_empty  timer  read_in   read_out  write_in  write_out\n");
    printk("--  -------------  -----  -----  --------  --------  ------  --------  -----  --------  --------  --------  ---------\n");

    //write ------------------------------------------------------------------------------
    for (device_id = ZEBRA_DEVICE_0; device_id < ZEBRA_DEVICE_MAX; device_id ++) {
        dev = &brd->device[device_id];
        if (!dev->open_mode)
            continue;

        write_list = &dev->write_list;
        wr_transmit_list = &dev->wr_transmit_list;
        sharemem = (share_memory_t *)dev->write_kaddr;
        str = timer_pending(&dev->write_timeout) ? "yes" : "no";

        printk("%-2d  %-13s  %-5d  %-5d  %-8d  %-8d  %-6d  %-8d  %-5s  %-8x  %-8x  %-8x  %-9x\n",
            dev->device_id, dev->name, write_list->list_length, wr_transmit_list->list_length,
            sharemem->hdr.rx_rdy_idx, sharemem->hdr.tx_empty_idx, wr_transmit_list->rx_rdy_idx,
            wr_transmit_list->tx_empty_idx, str, dev->pktrd_in_jiffies, dev->pktrd_out_jiffies, dev->pktwr_in_jiffies, dev->pktwr_out_jiffies);
    }
}

#ifdef USE_KTRHEAD
/* read thread for read/write data from/to share memory
 */
static int zebra_read_thread(void *data)
{
    zebra_device_t  *dev = (zebra_device_t *)data;
    int status, nice_val = 0;

    do {
        status = wait_event_timeout(dev->read_thread_wait, dev->read_thread_event, msecs_to_jiffies(10*1000));
        if (status == 0)
            continue;   /* timeout */

        dev->read_thread_event = 0;
        /* nice value */
        if (nice_val != dev->board->nice_val) {
            nice_val = dev->board->nice_val;
            set_user_nice(current, nice_val);
        }

        if (dev->attribute & ATTR_PACK)
            zebra_pack_read_workfn(&dev->pack_read_request);
        else
            zebra_read_workfn(&dev->read_request);
    } while (1);

    return 0;
}

/* read thread for read/write data from/to share memory
 */
static int zebra_write_thread(void *data)
{
    zebra_device_t  *dev = (zebra_device_t *)data;
    int status, nice_val = 0;

    do {
        status = wait_event_timeout(dev->write_thread_wait, dev->write_thread_event, msecs_to_jiffies(10*1000));
        if (status == 0)
            continue;   /* timeout */

        dev->write_thread_event = 0;
        /* nice value */
        if (nice_val != dev->board->nice_val) {
            nice_val = dev->board->nice_val;
            set_user_nice(current, nice_val);
        }

        if (dev->attribute & ATTR_PACK)
            zebra_pack_write_workfn(&dev->pack_write_request);
        else
            zebra_write_workfn(&dev->write_request);
    } while (1);
    return 0;
}
#endif /* USE_KTRHEAD */

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Two CPU communication");
MODULE_LICENSE("GPL");

