/**
 * @file i2c_common.c
 * Common I2C source code for sensor
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2013/09/26 02:06:49 $
 *
 * ChangeLog:
 *  $Log: i2c_common.c,v $
 *  Revision 1.1  2013/09/26 02:06:49  ricktsao
 *  no message
 *
 */

#include <linux/i2c.h>

#ifndef I2C_NAME
#define I2C_NAME    "sensor"
#endif

#ifndef I2C_ADDR
#define I2C_ADDR    0
#endif

typedef struct sen_i2c_info {
    struct i2c_client  *iic_client;
    struct i2c_adapter *iic_adapter;
} sen_i2c_info_t;

static sen_i2c_info_t *sensor_i2c_info = NULL;

static const struct i2c_device_id sensor_i2c_id[] = {
    { I2C_NAME, 0 },
    { }
};

static int __devinit sensor_i2c_probe(struct i2c_client *client,
                                      const struct i2c_device_id *id)
{
    if (!(sensor_i2c_info = kzalloc(sizeof(sen_i2c_info_t), GFP_KERNEL))) {
        printk("%s fail: kzalloc not OK.\n", __FUNCTION__);
        return -ENOMEM;
    }

    sensor_i2c_info->iic_client  = client;
    sensor_i2c_info->iic_adapter = client->adapter;

    i2c_set_clientdata(client, sensor_i2c_info);

    return 0;
}

static int __devexit sensor_i2c_remove(struct i2c_client *client)
{
    kfree(sensor_i2c_info);
    return 0;
}

static struct i2c_driver sensor_i2c_driver = {
    .driver = {
        .name  = I2C_NAME,
        .owner = THIS_MODULE,
    },
    .probe    = sensor_i2c_probe,
    .remove   = __devexit_p(sensor_i2c_remove),
    .id_table = sensor_i2c_id
};

static struct i2c_board_info sensor_i2c_device = {
    .type = I2C_NAME,
    .addr = I2C_ADDR
};

static int sensor_init_i2c_driver(void)
{
    int ret = 0;

    // add i2c_device to i2c bus 0
    if (i2c_new_device(i2c_get_adapter(0), &sensor_i2c_device) == NULL) {
        printk("%s fail: i2c_new_device not OK.\n", __FUNCTION__);
        return -ENXIO;
    }

    // bind i2c client driver to i2c bus
    if ((ret = i2c_add_driver(&sensor_i2c_driver)) != 0) {
        printk("%s fail: i2c_add_driver not OK.\n", __FUNCTION__);
    }

    return ret;
}

static void sensor_remove_i2c_driver(void)
{
    i2c_unregister_device(sensor_i2c_info->iic_client);
    i2c_del_driver(&sensor_i2c_driver);
}

static int sensor_i2c_transfer(struct i2c_msg *msgs, int num)
{
    if (unlikely(sensor_i2c_info->iic_adapter == NULL)) {
        printk("%s fail: sensor_i2c_info->ii2c_adapter not OK\n", __FUNCTION__);
        return -1;
    }

    if (unlikely(i2c_transfer(sensor_i2c_info->iic_adapter, msgs, num) != num)) {
        printk("%s fail: i2c_transfer not OK \n", __FUNCTION__);
        return -1;
    }

    return 0;
}
