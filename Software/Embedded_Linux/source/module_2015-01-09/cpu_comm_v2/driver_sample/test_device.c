#include <linux/version.h>      /* kernel versioning macros      */
#include <linux/module.h>       /* kernel modules                */
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>         /* kmem_cache_t                  */
#include <linux/workqueue.h>    /* work-queues                   */
#include <linux/timer.h>        /* timers                        */
#include <cpu_comm/cpu_comm.h>

static struct task_struct *test_thread_rx[3] = {0, 0, 0};  //Check input thread

static int test_is_running[3] = {0, 0, 0};
static int test_exit[3] = {0, 0, 0};

#define USE_PACK_RXTX

#define TOTAL_LOOP  100000
#define TEST_SIZE   (PAGE_SIZE - 4)

int test_working_thread_device_pack_rx0(void *private)
{
    int     idx = (int)private;
    cpu_comm_pack_msg_t  rcv_msg;
    u32 i, j, loop, start = (u32) -100;

    test_is_running[idx] = 1;

    memset(&rcv_msg, 0, sizeof(rcv_msg));

    /* open a new channel to transmit/receive the data */
    switch(idx) {
      case 0:
        cpu_comm_pack_open(CHAN_0_USR0, "Receiver0");
        rcv_msg.target = CHAN_0_USR0;
        break;
      case 1:
        cpu_comm_pack_open(CHAN_0_USR1, "Receiver1");
        rcv_msg.target = CHAN_0_USR1;
        break;
      case 2:
        cpu_comm_pack_open(CHAN_0_USR2, "Receiver2");
        rcv_msg.target = CHAN_0_USR2;
        break;
      default:
        panic("error! \n");
        break;
    }

    printk("Slave testing with TEST_SIZE = %d \n", (u32)TEST_SIZE);
    
    for (i = 0; i < CPU_COMM_NUM_IN_PACK; i ++) {
        rcv_msg.msg_data[i] = (unsigned char *)kmalloc(TEST_SIZE, GFP_KERNEL);
        if (rcv_msg.msg_data[i] == NULL)
            panic("no memory! \n");

        printk("idx: %d, pack_idx:%d, memory = 0x%x \n", idx, i, (u32)rcv_msg.msg_data[i]);
    }

    for (loop = 0; loop < TOTAL_LOOP; loop ++) {
        /*
         * receive first entry only
         */
        if (1) {
            u32 *ptr = (u32 *)rcv_msg.msg_data[0];

            rcv_msg.length[0] = TEST_SIZE;
            for (j = 1; j < CPU_COMM_NUM_IN_PACK; j ++)
                rcv_msg.length[j] = 0;

            if (cpu_comm_pack_msg_rcv(&rcv_msg, -1) < 0)
                panic("%s, receive error0! \n", __func__);

            if (rcv_msg.length[0] < 12)
                panic("The return length is %d \n", rcv_msg.length[0]);

            /* get the start number for loop zero */
            if (loop == 0) {
                start = ptr[0];
            }
            else {
                if (start != ptr[0])
                    panic("idx%d, start value fail! Expected value is %d but %d in loop%d!\n", 
                                idx, start, (u32)ptr[0], loop);
            }

            if (loop != ptr[1])
                panic("idx%d, loop value fail! Expected value is %d but %d!\n", 
                                idx, loop, (u32)ptr[1]);

            if (TOTAL_LOOP != ptr[2])
                panic("idx%d, total loop value fail! Expected value is %d but %d!\n", idx, TOTAL_LOOP, 
                                (u32)ptr[2]);
        }

        /*
         * Now we start to process the followed data
         */
        for (i = 1; i < CPU_COMM_NUM_IN_PACK; i ++) {
            rcv_msg.length[i] = TEST_SIZE;
            //test
            memset(rcv_msg.msg_data[i], 0x01, rcv_msg.length[i]);
        }

        if (cpu_comm_pack_msg_rcv(&rcv_msg, -1) < 0)
            panic("%s, receive error1! \n", __func__);

        /* check the content */
        for (i = 1; i < CPU_COMM_NUM_IN_PACK; i ++) {
            u32 *ptr = (u32 *)rcv_msg.msg_data[i];

            for (j = 0; j < (rcv_msg.length[i] >> 2); j ++) {
                if (ptr[j] != start ++)
                    panic("idx%d, data verify fail! Expected value = %d, but get %d, virtual = 0x%x, ofs = %d \n", 
                            idx, start, (u32)ptr[j], (u32)ptr, j);
            }  /* j */
        } /* i */

        printk("idx: %d, loop%d, start = %d \n", idx, loop, start);
    } /* loop */

    printk("Slave%d recevie %d data entries and verify sucess. \n", idx, loop);

    /* free memory */
    for (i = 0; i < CPU_COMM_NUM_IN_PACK; i ++) {
        kfree(rcv_msg.msg_data[i]);
        rcv_msg.msg_data[i] = NULL;
    }

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

    test_thread_rx[idx] = NULL;
    test_is_running[idx] = 0;

    return 0;
}

int test_working_thread_device_pack_rx1(void *private)
{
    return test_working_thread_device_pack_rx0(private);
}

int test_working_thread_device_pack_rx2(void *private)
{
    return test_working_thread_device_pack_rx0(private);
}

/* **************************************************************
 * single
 * **************************************************************
 */
int test_working_thread_device_rx(void *private)
{
    cpu_comm_msg_t  snd_msg, rcv_msg;
    unsigned char   *rbuf;
    u32 round, rcv_round = 0;

    if (private)    {}

    test_is_running[0] = 1;

    rbuf = kzalloc(TEST_SIZE, GFP_KERNEL);

    /* open a new channel to transmit/receive the data */
    cpu_comm_open(CHAN_0_USR0, "Slave");

    /*rcv info */
    memset(&rcv_msg, 0, sizeof(cpu_comm_msg_t));
    rcv_msg.target = CHAN_0_USR0;
    rcv_msg.msg_data = (unsigned char *)&round;
    rcv_msg.length = 4;
    cpu_comm_msg_rcv(&rcv_msg, -1);
    printk("Slave will receive %d round data. \n", round);

    rcv_msg.target = CHAN_0_USR0;
    rcv_msg.msg_data = rbuf;
    rcv_msg.length = TEST_SIZE;

    while (round --) {
        if (cpu_comm_msg_rcv(&rcv_msg, -1) < 0)
            panic("%s, error ........ \n", __func__);
        rcv_round ++;
    }

    memset(&snd_msg, 0, sizeof(cpu_comm_msg_t));
    snd_msg.target = CHAN_0_USR0;
    snd_msg.msg_data = (unsigned char *)&rcv_round;
    snd_msg.length = 4;
    cpu_comm_msg_snd(&snd_msg, -1);
    printk("Slave receives %d round, snd ack back. \n", rcv_round);

    while (!test_exit[0])
        msleep(100);

    cpu_comm_close(CHAN_0_USR0);
    kfree(rbuf);

    test_is_running[0] = 0;

    return 0;
}

static int __init test_device_init(void)
{
#ifdef USE_PACK_RXTX
    int i;

    for (i = 0; i < 3; i ++) {
        switch (i) {
          case 0:
            test_thread_rx[i] = kthread_create(test_working_thread_device_pack_rx0, (void *)i, "slave_rx0");
            break;
          case 1:
            test_thread_rx[i] = kthread_create(test_working_thread_device_pack_rx1, (void *)i, "slave_rx1");
            break;
          case 2:
            test_thread_rx[i] = kthread_create(test_working_thread_device_pack_rx2, (void *)i, "slave_rx2");
            break;
          default:
            panic("error \n");
            break;
        }

        if (!IS_ERR(test_thread_rx[i]))
		    wake_up_process(test_thread_rx[i]);
   }
#else
    test_thread_rx[0] = kthread_create(test_working_thread_device_rx, NULL, "test_device_rx");

    if (!IS_ERR(test_thread_rx[0])) {
		wake_up_process(test_thread_rx[0]);
	}
#endif

	return 0;
}

static void __exit test_device_cleanup(void)
{
    int i;

    for (i = 0; i < 3; i ++) {
        test_exit[i] = 1;

        if (test_thread_rx[i])
    		kthread_stop(test_thread_rx[i]);

        while (test_is_running[i])
            msleep(10);

        test_thread_rx[i] = NULL;
    }

}

module_init(test_device_init);
module_exit(test_device_cleanup);

MODULE_LICENSE("GPL");
