
////////////////////////////////////////////Standard Include///////////////////////////////////////////////
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/gfp.h>
#include <linux/slab.h>

////////////////////////////////////////////Project Include////////////////////////////////////////////////
#include "typedef.h"

///////////////////////////////////////////////////Macro///////////////////////////////////////////////////
#define SLI10121_I2C_DIV    100 * 1000
#define SLI10121_I2C_ADDR   0x66        /* Device address for slave target */

//////////////////////////////////////////Global Type Definitions//////////////////////////////////////////
///////////////////////////////////////Global Function Definitions////////////////////////////////////////

///////////////////////////////////////Private Function Definitions////////////////////////////////////////

///////////////////////////////////////Private Variable Definitions/////////////////////////////////////////
static struct delayed_work *g_work_job = NULL;
static struct workqueue_struct *g_work_queue = NULL;
static int g_en_timer = 0;

///////////////////////////////////////Global Variable Definitions/////////////////////////////////////////
unsigned char WD_TIMER_CNT = 0;

struct sli10121_i2c {
    struct i2c_client *iic_client;
    struct i2c_adapter *iic_adapter;
} *p_sli10121_i2c = NULL;

extern void ftiic010_set_clock_speed(void *, int hz);

static int __devinit sli10121_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

    if (!(p_sli10121_i2c = kzalloc(sizeof(struct sli10121_i2c), GFP_KERNEL))) {
        printk("%s fail: kzalloc not OK.\n", __FUNCTION__);
        return -ENOMEM;
    }

    p_sli10121_i2c->iic_client = client;
    p_sli10121_i2c->iic_adapter = adapter;

    i2c_set_clientdata(client, p_sli10121_i2c);

    return 0;
}

static int __devexit sli10121_remove(struct i2c_client *client)
{
    if (client) {
    }

    kfree(p_sli10121_i2c);

    return 0;
}

static const struct i2c_device_id sli10121_id[] = {
    {"sli10121", 0},
    {}
};

static struct i2c_driver sli10121_driver = {
    .driver = {
               .name = "sli10121",
               .owner = THIS_MODULE,
               },
    .probe = sli10121_probe,
    .remove = __devexit_p(sli10121_remove),
    .id_table = sli10121_id
};

static struct i2c_board_info sli10121_device = {
    .type = "sli10121",
    .addr = (SLI10121_I2C_ADDR >> 1),
};

void sli10121_i2c_init(void)
{
    if (i2c_new_device(i2c_get_adapter(1), &sli10121_device) == NULL) {
        printk("%s fail: i2c_new_device not OK.\n", __FUNCTION__);
        return;
    }

    if (i2c_add_driver(&sli10121_driver)) {
        printk("%s fail: i2c_add_driver not OK.\n", __FUNCTION__);
        return;
    }

    return;
}

void sli10121_i2c_exit(void)
{
    i2c_unregister_device(p_sli10121_i2c->iic_client);
    i2c_del_driver(&sli10121_driver);
}

void DelayMs(unsigned ms)
{
    msleep(ms);
}

void Delay(void)
{
    DelayMs(1);
}

SYS_STATUS I2C_ByteWrite(unsigned char addr, unsigned char dat)
{
    struct i2c_msg msgs[1];
    unsigned char buf[4];

    if (unlikely(p_sli10121_i2c->iic_adapter == NULL)) {
        printk("p_sli10121_i2c->iic_adapter is NULL \n");
        return -1;
    }

    buf[0] = 0;
    buf[1] = addr;
    buf[2] = dat;
    msgs[0].addr = SLI10121_I2C_ADDR >> 1;
    msgs[0].flags = 0;          /* write */
    msgs[0].len = 3;
    msgs[0].buf = &buf[0];

    //ftiic010_set_clock_speed(i2c_get_adapdata(p_sli10121_i2c->iic_adapter), SLI10121_I2C_DIV);
    if (unlikely(i2c_transfer(p_sli10121_i2c->iic_adapter, msgs, 1) != 1)) {
        printk("I2C_ByteWrite ERROUT \n");
        return -1;
    }

    return ER_SUCCESS;
}

void TI_ByteWrite(unsigned char addr, unsigned char dat)
{
    printk("TI_ByteWrite addr=0x%x data=0x%x\n", addr, dat);
    if (addr || dat) {
    }

}

unsigned char I2C_ByteRead(unsigned char RegAddr)
{
    struct i2c_msg msgs[2];
    unsigned char buf[3];

    if (unlikely(p_sli10121_i2c->iic_adapter == NULL)) {
        printk("p_sli10121_i2c->iic_adapter is NULL \n");
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    buf[0] = 0;
    buf[1] = RegAddr;
    msgs[0].addr = SLI10121_I2C_ADDR >> 1;
    msgs[0].flags = 0;          /* write */
    msgs[0].len = 2;
    msgs[0].buf = &buf[0];

    /* setup read command */
    msgs[1].addr = SLI10121_I2C_ADDR >> 1;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = 1;
    msgs[1].buf = &buf[2];

    //ftiic010_set_clock_speed(i2c_get_adapdata(p_sli10121_i2c->iic_adapter), SLI10121_I2C_DIV);
    if (unlikely(i2c_transfer(p_sli10121_i2c->iic_adapter, msgs, 2) != 2)) {
        printk("I2C_ByteRead ERR OUT \n");
        return 0;
    }

    return buf[2];
}

void I2C_ReadArray(unsigned char *dest_addr, unsigned char src_addr, unsigned char len)
{
    /* do nothing */
    unsigned char i = 0;
    //I2C_ByteWrite(0,(0xC6));/*remove it , maintain by HW*/
	
    for(i = 0 ; i < len ; i++) {      
		*(dest_addr+i) = I2C_ByteRead(src_addr);
    }
	
}
#ifdef USE_TIMER
static void job_callback_process(struct work_struct *work)
{
    if (work) {
    }

    if (g_en_timer) {
        WD_TIMER_CNT = WD_TIMER_CNT + 1;
        queue_delayed_work(g_work_queue, g_work_job, (265 * HZ) / 1000);        // 265ms wakeup
    }
}
#endif

void sli10121_timer_init(void)
{
#ifdef USE_TIMER
    g_work_queue = create_workqueue("hdmi_sli10121_workque");
    if (g_work_queue == NULL)
        panic("%s, create workQ fail! \n", __func__);

    g_work_job = kmalloc(sizeof(struct work_struct), GFP_KERNEL);
    if (g_work_job == NULL)
        panic("[ERROR]create g_work_job memory fail!\n");

    INIT_DELAYED_WORK(g_work_job, job_callback_process);
#endif
     if (g_work_queue || g_work_job ) {} //avoid warning
}

void start_timer(void)
{
    g_en_timer = 1;
#ifdef USE_TIMER
    queue_delayed_work(g_work_queue, g_work_job, 0);
#endif    
}

void stop_timer(void)
{
    g_en_timer = 0;
}

void sli10121_timer_destroy(void)
{
#ifdef USE_TIMER
    stop_timer();

    if (g_work_job) {
        cancel_delayed_work(g_work_job);
        flush_scheduled_work();
    }

    if (g_work_job) {
        kfree(g_work_job);
        g_work_job = NULL;
    }

    if (g_work_queue) {
        destroy_workqueue(g_work_queue);
        g_work_queue = NULL;
    }
#endif    
}
