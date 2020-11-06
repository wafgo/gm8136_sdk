#include <linux/module.h>
#include <linux/version.h>      /* kernel versioning macros      */
#include <linux/module.h>       /* kernel modules                */
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>         /* kmem_cache_t                  */
#include <linux/workqueue.h>    /* work-queues                   */
#include <linux/timer.h>        /* timers                        */
#include <mach/gm_jiffies.h>
#include <cpu_comm/cpu_comm.h>


/*-----------------------------------------------------------------
 * module parameters
 *-----------------------------------------------------------------
 */
static unsigned int unit_sz = 2 << 20, mem_sz = 400 << 20;

int rd = 0, wt = 0;
module_param(rd, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rd, "read");

module_param(wt, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(wt, "write");

static struct task_struct *rx_thread = NULL;
static struct task_struct *tx_thread = NULL;
static char *rx_buf = NULL, *tx_buf = NULL;

chan_id_t   chan_id = CHAN_0_USR0;

int rx_thread_fn(void *private)
{
    cpu_comm_pack_msg_t rcv_msg;
    u32 left_sz, rcv_size;
    u32 start_jiffies = 0;

    cpu_comm_pack_open(chan_id, "rx_thread");

    memset(&rcv_msg, 0, sizeof(cpu_comm_msg_t));

    left_sz = mem_sz;
    do {
        rcv_size = left_sz > unit_sz ? unit_sz : left_sz;

        rcv_msg.target = chan_id;
        rcv_msg.msg_data[0] = rx_buf;
        rcv_msg.length[0] = rcv_size;

        if (cpu_comm_pack_msg_rcv(&rcv_msg, -1) < 0)
            panic("%s, error ........ \n", __func__);
        if (!start_jiffies)
            start_jiffies = gm_jiffies;

        left_sz -= rcv_size;
    } while (left_sz);

    cpu_comm_pack_close(chan_id);

    printk("The total consume jiffies = %d \n", (u32)(gm_jiffies - start_jiffies));

    rx_thread = NULL;
    return 0;
}

int tx_thread_fn(void *private)
{
    cpu_comm_pack_msg_t snd_msg;
    u32 left_sz, snd_size;
    u32 start_jiffies;

    cpu_comm_pack_open(chan_id, "tx_thread");

    memset(&snd_msg, 0, sizeof(cpu_comm_msg_t));
    start_jiffies = gm_jiffies;

    left_sz = mem_sz;

    do {
        snd_size = left_sz > unit_sz ? unit_sz : left_sz;

        snd_msg.target = chan_id;
        snd_msg.msg_data[0] = tx_buf;
        snd_msg.length[0] = snd_size;

        if (cpu_comm_pack_msg_snd(&snd_msg, -1) < 0)
            panic("%s, error ........ \n", __func__);
        left_sz -= snd_size;
    } while (left_sz);

    /* ensure all packets in q to be flushed out */
    cpu_comm_pack_close(chan_id);

    printk("The total consume jiffies = %d \n", (u32)(gm_jiffies - start_jiffies));

    tx_thread = NULL;
    return 0;
}

static int __init cpucomm_test_init(void)
{
    if (!rd && !wt) {
        printk("please specify rt = 1 or wt = 1! \n");
        return -1;
    }

    if (rd && wt) {
        printk("please specify either rt = 1 or wt = 1, not both! \n");
        return -1;
    }

    if (rd) {
        rx_buf = kmalloc((1 << 20), GFP_KERNEL);
        if (rx_buf == NULL)
            return -1;

        rx_thread = kthread_create(rx_thread_fn, NULL, "rx_thread");
        if (!IS_ERR(rx_thread))
            wake_up_process(rx_thread);
    }

    if (wt) {
        tx_buf = kmalloc((1 << 20), GFP_KERNEL);
        if (tx_buf == NULL)
            return -1;

        tx_thread = kthread_create(tx_thread_fn, NULL, "tx_thread");
        if (!IS_ERR(tx_thread))
            wake_up_process(tx_thread);
    }

    printk("start to test......... \n");

	return 0;
}

static void __exit cpucomm_test_cleanup(void)
{
    if (rx_buf)
        kfree(rx_buf);
    if (tx_buf)
        kfree(tx_buf);

    if (rx_thread)
        kthread_stop(rx_thread);

    if (tx_thread)
        kthread_stop(tx_thread);
}

module_init(cpucomm_test_init);
module_exit(cpucomm_test_cleanup);

MODULE_LICENSE("GPL");
