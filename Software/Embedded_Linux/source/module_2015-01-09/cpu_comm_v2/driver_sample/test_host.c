#include <linux/version.h>      /* kernel versioning macros      */
#include <linux/module.h>       /* kernel modules                */
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>         /* kmem_cache_t                  */
#include <linux/workqueue.h>    /* work-queues                   */
#include <linux/timer.h>        /* timers                        */
#include <mach/gm_jiffies.h>
#include <cpu_comm/cpu_comm.h>

static int round = 1000000;
module_param(round, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(round, "test round");

static struct task_struct *test_thread_tx[3] = {0, 0, 0};  //Check input thread
static int test_is_running[3] = {0, 0, 0};
static int test_exit[3] = {0, 0, 0};

#define USE_PACK_RXTX

#define TOTAL_LOOP  100000
#define TEST_SIZE   (PAGE_SIZE - 4)

int test_working_thread_host_pack_tx0(void *private)
{
    int idx = (int)private;
    u32 loop, i, j, start;
    cpu_comm_pack_msg_t  snd_msg;

    test_is_running[idx] = 1;
    memset(&snd_msg, 0, sizeof(snd_msg));

    switch(idx) {
      case 0:
        cpu_comm_pack_open(CHAN_0_USR0, "Sender0");
        snd_msg.target = CHAN_0_USR0;
        break;
      case 1:
        cpu_comm_pack_open(CHAN_0_USR1, "Sender1");
        snd_msg.target = CHAN_0_USR1;
        break;
      case 2:
        cpu_comm_pack_open(CHAN_0_USR2, "Sender2");
        snd_msg.target = CHAN_0_USR2;
        break;
      default:
        panic("error! \n");
        break;
    }

    printk("Master testing with TEST_SIZE = %d \n", (u32)TEST_SIZE);

    for (i = 0; i < CPU_COMM_NUM_IN_PACK; i ++) {
        snd_msg.msg_data[i] = (unsigned char *)kmalloc(TEST_SIZE, GFP_KERNEL);
        if (snd_msg.msg_data[i] == NULL)
            panic("no memory! \n");
    }

    start = 100 + idx;

    for (loop = 0; loop < TOTAL_LOOP; loop ++) {
        /* prepare the pack data
         r*/
        for (i = 0; i < CPU_COMM_NUM_IN_PACK; i ++) {
            u32 *ptr = (u32 *)snd_msg.msg_data[i];

            if (i == 0) {
                //fill the content
                ptr[0] = start;
                ptr[1] = loop;  //index
                ptr[2] = TOTAL_LOOP;

                snd_msg.length[0] = 12;
            } else {
                snd_msg.length[i] = TEST_SIZE;
                //fill the content with random data
                for (j = 0; j < (TEST_SIZE >> 2); j ++)
                    ptr[j] = start ++;
            }
        }
        cpu_comm_pack_msg_snd(&snd_msg, -1);
    }

    printk("Master%d sends all data out with loop = %d \n", idx, TOTAL_LOOP);

    switch (idx) {
      case 0:
        cpu_comm_pack_close(CHAN_0_USR0);
        break;
      case 1:
        cpu_comm_pack_close(CHAN_0_USR1);
        break;
      case 2:
        cpu_comm_pack_close(CHAN_0_USR2);
        break;
      default:
        panic("error! \n");
        break;
    }


    /* free memory */
    for (i = 0; i < CPU_COMM_NUM_IN_PACK; i ++) {
        kfree(snd_msg.msg_data[i]);
        snd_msg.msg_data[i] = NULL;
    }
    test_is_running[idx] = 0;
    test_thread_tx[idx] = NULL;

    return 0;
}

int test_working_thread_host_pack_tx1(void *private)
{
    return test_working_thread_host_pack_tx0(private);
}

int test_working_thread_host_pack_tx2(void *private)
{
    return test_working_thread_host_pack_tx0(private);
}

/* **************************************************************
 * single
 * **************************************************************
 */
int test_working_thread_host_tx(void *private)
{
    cpu_comm_msg_t  snd_msg, rcv_msg;
    unsigned int    test_round = 0;
    unsigned int    ack_data = 0;
    unsigned char   *tbuf;
    u32 send_jiffies, diff;

    if (private)    {}

    tbuf = kzalloc(TEST_SIZE, GFP_KERNEL);

    test_is_running[0] = 1;

    /* open a new channel to transmit/receive the data */
    cpu_comm_open(CHAN_0_USR0, "Master");

    strcpy(&tbuf[0], "Hello Jerry! You are a good man.");

    printk("Test round is %d \n", round);

    msleep(900);

    /* Tell the peer about the test round */
    memset(&snd_msg, 0, sizeof(cpu_comm_msg_t));
    snd_msg.target = CHAN_0_USR0;
    snd_msg.msg_data = (unsigned char *)&round;
    snd_msg.length = 4;
    cpu_comm_msg_snd(&snd_msg, -1);

    /* start to measure */
    send_jiffies = gm_jiffies;

    snd_msg.target = CHAN_0_USR0;
    snd_msg.msg_data = tbuf;
    snd_msg.length = strlen(tbuf);

    while (round --) {
        /* blocking if the internal queue is full. */
        if (cpu_comm_msg_snd(&snd_msg, -1) < 0)
            panic("Master sends data fail! \n");
        test_round ++;
    }
    printk("Master sends messages, round %d, starts to wait ack\n", test_round);

    /*rcv info */
    memset(&rcv_msg, 0, sizeof(cpu_comm_msg_t));
    rcv_msg.target = CHAN_0_USR0;
    rcv_msg.msg_data = (unsigned char *)&ack_data;
    rcv_msg.length = 4;
    cpu_comm_msg_rcv(&rcv_msg, -1);
    diff = gm_jiffies - send_jiffies;

    printk("Master receives response ok, consumes gm_jiffies = %d \n", diff);

    while (!test_exit[0])
        msleep(100);

    cpu_comm_close(CHAN_0_USR0);
    kfree(tbuf);

    test_is_running[0] = 0;

    return 0;
}

static int __init test_master_init(void)
{
#ifdef USE_PACK_RXTX
    int i;

    for (i = 0; i < 3; i ++) {
        switch (i) {
          case 0:
            test_thread_tx[i] = kthread_create(test_working_thread_host_pack_tx0, (void *)i, "master_tx0");
            break;
          case 1:
            test_thread_tx[i] = kthread_create(test_working_thread_host_pack_tx1, (void *)i, "master_tx1");
            break;
          case 2:
            test_thread_tx[i] = kthread_create(test_working_thread_host_pack_tx2, (void *)i, "master_tx2");
            break;
          default:
            panic("invalid number! \n");
            break;
        }
        if (!IS_ERR(test_thread_tx[i]))
		    wake_up_process(test_thread_tx[i]);
    }
#else
    test_thread_tx[0] = kthread_create(test_working_thread_host_tx, NULL, "test_master_tx");

    if (!IS_ERR(test_thread_tx[0])) {
		wake_up_process(test_thread_tx[0]);
	}
#endif

	return 0;
}

static void __exit test_master_cleanup(void)
{
    int i;

    for (i = 0; i < 3; i ++) {
        test_exit[i] = 1;

        if (test_thread_tx[i])
    		kthread_stop(test_thread_tx[i]);

        while (test_is_running[i])
            msleep(10);

        test_thread_tx[i] = NULL;
    }
}

module_init(test_master_init);
module_exit(test_master_cleanup);

MODULE_LICENSE("GPL");
