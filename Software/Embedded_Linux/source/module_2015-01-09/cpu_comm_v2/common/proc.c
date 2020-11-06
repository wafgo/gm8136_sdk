#include <linux/version.h>      /* kernel versioning macros      */
#include <linux/module.h>       /* kernel modules                */
#include <linux/list.h>         /* linked-lists                  */
#include <linux/moduleparam.h>
#include <linux/miscdevice.h>
#include <linux/semaphore.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <mach/ftpmu010.h>
#include "zebra_sdram.h"        /* Host-to-DSP SDRAM layout      */
#include "zebra.h"
#include "cpu_comm/cpu_comm.h"
#include "mem_alloc.h"
#include "dma_llp.h"

#define NUM_OF_BRD  4
//----------------------------------------------------------------------------------------
// Proc functions
//----------------------------------------------------------------------------------------
static struct proc_dir_entry *zebra_proc_root[NUM_OF_BRD] = {NULL, NULL, NULL, NULL};
static struct proc_dir_entry *counter_proc[NUM_OF_BRD] = {NULL, NULL, NULL, NULL};
static struct proc_dir_entry *write_list_proc[NUM_OF_BRD] = {NULL, NULL, NULL, NULL};
static struct proc_dir_entry *read_list_proc[NUM_OF_BRD] = {NULL, NULL, NULL, NULL};
static struct proc_dir_entry *interrupt_proc[NUM_OF_BRD] = {NULL, NULL, NULL, NULL};
static struct proc_dir_entry *memory_proc[NUM_OF_BRD] = {NULL, NULL, NULL, NULL};
static struct proc_dir_entry *debug_proc[NUM_OF_BRD] = {NULL, NULL, NULL, NULL};
static struct proc_dir_entry *cputick_proc[NUM_OF_BRD] = {NULL, NULL, NULL, NULL};
static struct proc_dir_entry *doorbell_delay_proc[NUM_OF_BRD] = {NULL, NULL, NULL, NULL};

struct array_idx_s {
    int board_id;
} array_idx[NUM_OF_BRD];

int get_array_idx(int board_id)
{
    static int database_init = 0;
    int i, free_idx = -1;

    if (!database_init) {
        database_init = 1;
        memset((char *)&array_idx[0], 0, sizeof(struct array_idx_s) * NUM_OF_BRD);
    }
    for (i = 0; i < NUM_OF_BRD; i ++) {
        if (array_idx[i].board_id == board_id)
            return i;
        if ((free_idx == -1) && !array_idx[i].board_id)
            free_idx = i;
    }

    if (free_idx < NUM_OF_BRD) {
        array_idx[free_idx].board_id = board_id;
        return free_idx;
    }

    panic("%s, cpucomm array is too small! \n", __func__);
    return -1;
}

/* Read statistic counter for each device
 */
int proc_read_counter(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    zebra_board_t  *brd = (zebra_board_t *)data;
    zebra_device_t *dev;
    int i, len = 0;

    len += sprintf(page + len, "id  name           dropped  write_wait  user_read  user_write  write_ack  error  workaround\n");
    len += sprintf(page + len, "--  -------------  -------  ----------  ---------  ----------  ---------  -----  ----------\n");

    for (i = ZEBRA_DEVICE_0; i < ZEBRA_DEVICE_MAX; i ++) {
        dev = &brd->device[i];
        if (!dev->open_mode)
            continue;

        len += sprintf(page + len, "%-2d  %-13s  0x%-5x  0x%-8x  0x%-7x  0x%-8x  0x%-7x  0x%-3x  0x%-10x\n",
            dev->device_id, dev->name, dev->counter.dropped,
            dev->counter.user_blocking, dev->counter.user_read, dev->counter.user_write,
            dev->counter.write_ack, dev->counter.error, dev->counter.workaround);
    }

    return len;
}

static int proc_clear_counter(struct file *file, const char *buffer,unsigned long count, void *data)
{
    zebra_board_t  *brd = (zebra_board_t *)data;
    zebra_device_t *dev;
    int start, end, len = count;
    unsigned char value[10];
    int channel, i;

    if (copy_from_user(value, buffer, len))
        return 0;

    value[len] = '\0';
    sscanf(value, "%d \n", &channel);

    if (channel < 0) {
        start = ZEBRA_DEVICE_0;
        end = ZEBRA_DEVICE_MAX;
    } else {
        start = channel;
        end = channel + 1;
        if (end >= ZEBRA_DEVICE_MAX)
            end = ZEBRA_DEVICE_MAX;
    }

    for (i = start; i < end; i ++) {
        dev = &brd->device[i];
        if (!dev->open_mode)
            continue;

        memset(&dev->counter, 0, sizeof(dev->counter));
    }
    return count;
}

/* Read write_list information
 */
int proc_read_write_list(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    zebra_board_t  *brd = (zebra_board_t *)data;
    zebra_device_t *dev;
    buffer_list_t  *write_list;
    transmit_list_t *wr_transmit_list;
    share_memory_t *sharemem;
    int i, len = 0;
    char *str;

    len += sprintf(page + len, "                   write  xmit   sharemem  sharemem  xmit    xmit           \n");    //how many rx_rdys are sent
    len += sprintf(page + len, "id  name           list   list   rx_rdy    tx_empty  rx_rdy  tx_empty  timer  read_in   read_out  write_in  write_out\n");
    len += sprintf(page + len, "--  -------------  -----  -----  --------  --------  ------  --------  -----  --------  --------  --------  ---------\n");

    for (i = ZEBRA_DEVICE_0; i < ZEBRA_DEVICE_MAX; i ++) {
        dev = &brd->device[i];
        if (!dev->open_mode)
            continue;

        write_list = &dev->write_list;
        wr_transmit_list = &dev->wr_transmit_list;
        sharemem = (share_memory_t *)dev->write_kaddr;
        str = timer_pending(&dev->write_timeout) ? "yes" : "no";

        len += sprintf(page + len, "%-2d  %-13s  %-5d  %-5d  %-8d  %-8d  %-6d  %-8d  %-5s  %-8x  %-8x  %-8x  %-9x\n",
            dev->device_id, dev->name, write_list->list_length, wr_transmit_list->list_length,
            sharemem->hdr.rx_rdy_idx, sharemem->hdr.tx_empty_idx, wr_transmit_list->rx_rdy_idx,
            wr_transmit_list->tx_empty_idx, str, dev->pktrd_in_jiffies, dev->pktrd_out_jiffies,
            dev->pktwr_in_jiffies, dev->pktwr_out_jiffies);
    }

    return len;
}

/* Read read_list information
 */
int proc_read_read_list(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    zebra_board_t  *brd = (zebra_board_t *)data;
    zebra_device_t *dev;
    buffer_list_t  *read_list;
    transmit_list_t *rd_transmit_list;
    share_memory_t *sharemem;
    int i, len = 0, on_hand;

    len += sprintf(page + len, "                   read   sharemem  sharemem  xmit    xmit      on       \n");
    len += sprintf(page + len, "id  name           list   rx_rdy    tx_empty  rx_rdy  tx_empty  hand  read_in   read_out  write_in  write_out\n");
    len += sprintf(page + len, "--  -------------  -----  --------  --------  ------  --------  ----  --------  --------  --------  ---------\n");

    for (i = ZEBRA_DEVICE_0; i < ZEBRA_DEVICE_MAX; i ++) {
        dev = &brd->device[i];
        if (!dev->open_mode)
            continue;

        read_list = &dev->read_list;
        on_hand = dev->read_entry ? 1 : 0;

        rd_transmit_list = &dev->rd_transmit_list;
        sharemem = (share_memory_t *)dev->read_kaddr;

        len += sprintf(page + len, "%-2d  %-13s  %-5d  %-8d  %-8d  %-6d  %-8d  %-4d  %-8x  %-8x  %-8x  %-9x\n",
            dev->device_id, dev->name, read_list->list_length, sharemem->hdr.rx_rdy_idx,
            sharemem->hdr.tx_empty_idx, rd_transmit_list->rx_rdy_idx, rd_transmit_list->tx_empty_idx,
            on_hand, dev->pktrd_in_jiffies, dev->pktrd_out_jiffies, dev->pktwr_in_jiffies, dev->pktwr_out_jiffies);
    }

    return len;
}

/* Read rx_rdy/tx_empty interrupts counter
 */
int proc_read_interrupt(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    zebra_board_t  *brd = (zebra_board_t *)data;
    zebra_device_t *dev;
    int i, len = 0;

    len += sprintf(page + len, "                   snd rx_rdy    snd tx_empty  rcv rx_rdy    rcv tx_empty        \n");
    len += sprintf(page + len, "id  name           interrupts    interrupts    interrupts    interrupts   warning\n");
    len += sprintf(page + len, "--  -------------  ------------  ------------  ------------  ------------ -------\n");

    for (i = ZEBRA_DEVICE_0; i < ZEBRA_DEVICE_MAX; i ++) {
        dev = &brd->device[i];
        if (!dev->open_mode)
            continue;

        len += sprintf(page + len, "%-2d  %-13s  %-12d  %-12d  %-12d  %-12d %-7d\n",
            dev->device_id, dev->name, dev->counter.send_rxrdy_interrupts, dev->counter.send_txempty_interrupts,
            dev->counter.rcv_rxrdy_interrupts, dev->counter.rcv_txempty_interrupts, brd->internal_error);
    }

    return len;
}

/* Read memory counter
 */
int proc_read_memory(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    zebra_board_t  *brd = (zebra_board_t *)data;
    zebra_driver_t *drv = brd->driver;
    mem_counter_t  mem_counter;
    u32 paddr, size;
    int len = 0;

    /* board level */
    mem_get_counter(&mem_counter);
    mem_get_poolinfo(&paddr, &size);
    len += sprintf(page + len, "Local Pool Memory Statistic\n");
    len += sprintf(page + len, "memory base  memory size  mem_count  max_slice  clean_memory  memory_wait  llp_count\n");
    len += sprintf(page + len, "-----------  -----------  ---------  ---------  ------------  -----------  ---------\n");
    len += sprintf(page + len, "0x%-9x  %-11d  %-9d  %-9d  %-12d  %-11d  %-9d \n", paddr, mem_counter.memory_sz,
        mem_counter.alloc_cnt, mem_counter.max_slice, mem_counter.clean_space, mem_counter.wait_cnt,
        mem_counter.dma_llp_cnt);

    /* if the pci memory is not zero which means the memory comes from PCI Host */
    if (drv->pci_pool_paddr) {
        len += sprintf(page + len, "\n");
        pci_mem_get_counter(&mem_counter);
        pci_mem_get_poolinfo(&paddr, &size);
        len += sprintf(page + len, "PCI Pool Memory Statistic\n");
        len += sprintf(page + len, "memory base  memory size  mem_count  max_slice  clean_memory  memory_wait  llp_count\n");
        len += sprintf(page + len, "-----------  -----------  ---------  ---------  ------------  -----------  ---------\n");
        len += sprintf(page + len, "0x%-9x  %-11d  %-9d  %-9d  %-12d  %-11d  %-9d \n", paddr, mem_counter.memory_sz,
            mem_counter.alloc_cnt, mem_counter.max_slice, mem_counter.clean_space, mem_counter.wait_cnt,
            mem_counter.dma_llp_cnt);
    }

    return len;
}

/* Read debug counter
 */
int proc_read_debug(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    zebra_board_t  *brd = (zebra_board_t *)data;
    zebra_driver_t *drv = brd->driver;
    dma_status_t dma_status;
    int i, len = 0;

    dma_get_status(&dma_status);

    for (i = 0; i < dma_status.active_cnt; i ++) {
        len += sprintf(page + len, "dma chan:    %d \n", dma_status.chan_id[i]);
        len += sprintf(page + len, "sw irq:      %d \n", dma_status.sw_irq[i]);
        len += sprintf(page + len, "rcv sw intr: %d \n", dma_status.intr_cnt[i]);
        len += sprintf(page + len, "chan busy:   %d \n", dma_status.busy[i]);
        len += sprintf(page + len, "-------------------------\n");
    }

    len += sprintf(page + len, "buffer_alloc count: %d \n", atomic_read(&drv->buffer_alloc));
    len += sprintf(page + len, "databuf_alloc count: %d \n", atomic_read(&drv->databuf_alloc));

    return len;
}

/* Read cpu tick
 */
int proc_read_cputick(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    u32 tick_in_hz, tick;
    pcix_id_t   pci_id;
    cpu_id_t    cpu_id;

    cpu_comm_get_identifier(&pci_id, &cpu_id);

    switch (cpu_id) {
      case CPU_FA626:
        if (cpu_comm_get_tick(pci_id, CPU_FA726, &tick_in_hz, &tick))
            break;
        len += sprintf(page + len, "*** Format: local CONFIG_HZ, remote CONFIG_HZ, remote jiffies.  *** \n");
        len += sprintf(page + len, "%d  %d  0x%x \n", CONFIG_HZ, tick_in_hz, tick);
        break;

      case CPU_FA726:
        if (cpu_comm_get_tick(pci_id, CPU_FA626, &tick_in_hz, &tick))
            break;
        len += sprintf(page + len, "*** Format: local CONFIG_HZ, remote CONFIG_HZ, remote jiffies.  *** \n");
        len += sprintf(page + len, "%d  %d  0x%x \n", CONFIG_HZ, tick_in_hz, tick);
        break;
      default:
        break;
    }
    return len;
}

/* DoorBell Delay in ms
 */
int proc_read_doorbellms(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    zebra_board_t  *brd = (zebra_board_t *)data;

    len += sprintf(page + len, "Board: %s, doorbell_delayms = %d(ms) \n", brd->name, brd->doorbell_delayms);

    return len;
}

static int proc_write_doorbellms(struct file *file, const char *buffer, unsigned long count, void *data)
{
    zebra_board_t  *brd = (zebra_board_t *)data;

    sscanf(buffer,"%d", &brd->doorbell_delayms);

    printk("Board %s, doorbell_delayms = %d(ms) \n", brd->name, brd->doorbell_delayms);

    return count;
}

int zebra_proc_init(zebra_board_t *brd, char *parent_name)
{
    struct proc_dir_entry *p;
    char    name[40];
    int     len, ret = 0, idx;

    /* counter
     */
    idx = get_array_idx(brd->board_id);

    if (zebra_proc_root[idx])
        panic("%s, %s(0x%x) \n", __func__, brd->name, brd->board_id);

    memset(&name[0], 0, sizeof(name));
    strcpy(name, parent_name);
    len = strlen(name);
    sprintf(&name[len], brd->name);
    /* create_proc_entry will have owned name memory */
    p = create_proc_entry(name, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (p == NULL) {
        ret = -ENOMEM;
        goto exit;
    }
    zebra_proc_root[idx] = p;

    /*
     * counter for device
     */
    counter_proc[idx] = create_proc_entry("counter", S_IRUGO | S_IXUGO, zebra_proc_root[idx]);
    if (counter_proc[idx] == NULL)
        panic("Fail to create counter_proc!\n");

    counter_proc[idx]->read_proc = (read_proc_t *) proc_read_counter;
    counter_proc[idx]->write_proc = (write_proc_t *) proc_clear_counter;
    counter_proc[idx]->data = (void *)brd;

    /*
     * write list for device
     */
    write_list_proc[idx] = create_proc_entry("write_list", S_IRUGO, zebra_proc_root[idx]);
    if (write_list_proc[idx] == NULL)
        panic("Fail to create write_list_proc!\n");

    write_list_proc[idx]->read_proc = (read_proc_t *) proc_read_write_list;
    write_list_proc[idx]->write_proc = NULL;
    write_list_proc[idx]->data = (void *)brd;

    /*
     * read list for device
     */
    read_list_proc[idx] = create_proc_entry("read_list", S_IRUGO, zebra_proc_root[idx]);
    if (read_list_proc[idx] == NULL)
        panic("Fail to create read_list_proc!\n");

    read_list_proc[idx]->read_proc = (read_proc_t *) proc_read_read_list;
    read_list_proc[idx]->write_proc = NULL;
    read_list_proc[idx]->data = (void *)brd;

    /*
     * interrupt for device
     */
    interrupt_proc[idx] = create_proc_entry("interrupt", S_IRUGO, zebra_proc_root[idx]);
    if (interrupt_proc[idx] == NULL)
        panic("Fail to create interrupt_proc!\n");

    interrupt_proc[idx]->read_proc = (read_proc_t *) proc_read_interrupt;
    interrupt_proc[idx]->write_proc = NULL;
    interrupt_proc[idx]->data = (void *)brd;

    /*
     * Memory
     */
    memory_proc[idx] = create_proc_entry("memory", S_IRUGO, zebra_proc_root[idx]);
    if (memory_proc[idx] == NULL)
        panic("Fail to create memory_proc!\n");

    memory_proc[idx]->read_proc = (read_proc_t *) proc_read_memory;
    memory_proc[idx]->write_proc = NULL;
    memory_proc[idx]->data = (void *)brd;

    /*
     * debug only
     */
    debug_proc[idx] = create_proc_entry("debug", S_IRUGO, zebra_proc_root[idx]);
    if (debug_proc[idx] == NULL)
        panic("Fail to create debug_proc!\n");

    debug_proc[idx]->read_proc = (read_proc_t *) proc_read_debug;
    debug_proc[idx]->write_proc = NULL;
    debug_proc[idx]->data = (void *)brd;

    /*
     * cpu tick
     */
    cputick_proc[idx] = create_proc_entry("cpu_tick", S_IRUGO, zebra_proc_root[idx]);
    if (cputick_proc[idx] == NULL)
        panic("Fail to create memory_proc!\n");
    cputick_proc[idx]->read_proc = (read_proc_t *) proc_read_cputick;
    cputick_proc[idx]->write_proc = NULL;
    cputick_proc[idx]->data = (void *)brd;

    /*
     * doorbell delay ms
     */
    doorbell_delay_proc[idx] = create_proc_entry("doorbell_delayms", S_IRUGO, zebra_proc_root[idx]);
    if (doorbell_delay_proc[idx] == NULL)
        panic("Fail to create doorbell_delay_proc!\n");
    doorbell_delay_proc[idx]->read_proc = (read_proc_t *) proc_read_doorbellms;
    doorbell_delay_proc[idx]->write_proc = (write_proc_t *)proc_write_doorbellms;
    doorbell_delay_proc[idx]->data = (void *)brd;

exit:
    return ret;
}

/* Remove function
 */
void zebra_proc_remove(zebra_board_t *brd)
{
    zebra_board_t *node;
    zebra_driver_t *driver = brd->driver;
    int idx, count = 0;

    idx = get_array_idx(brd->board_id);

    if (doorbell_delay_proc[idx] != NULL)
        remove_proc_entry(doorbell_delay_proc[idx]->name, zebra_proc_root[idx]);

    if (counter_proc[idx] != NULL)
        remove_proc_entry(counter_proc[idx]->name, zebra_proc_root[idx]);

    if (write_list_proc[idx] != NULL)
        remove_proc_entry(write_list_proc[idx]->name, zebra_proc_root[idx]);

    if (read_list_proc[idx] != NULL)
        remove_proc_entry(read_list_proc[idx]->name, zebra_proc_root[idx]);

    if (interrupt_proc[idx] != NULL)
        remove_proc_entry(interrupt_proc[idx]->name, zebra_proc_root[idx]);

    if (memory_proc[idx] != NULL)
        remove_proc_entry(memory_proc[idx]->name, zebra_proc_root[idx]);

    if (debug_proc[idx] != NULL)
        remove_proc_entry(debug_proc[idx]->name, zebra_proc_root[idx]);

    if (cputick_proc[idx] != NULL)
        remove_proc_entry(cputick_proc[idx]->name, zebra_proc_root[idx]);

    list_for_each_entry(node, &driver->board_list, list)
        count ++;

    /* last one, root */
    if (!count && zebra_proc_root[idx] != NULL) {
        remove_proc_entry(zebra_proc_root[idx]->name, NULL);
        zebra_proc_root[idx] = NULL;
    }
}

