///*****************************************
//  Copyright (C) 2009-2014
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   >mcuio.c<
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2009/08/24
//   @fileversion: CAT6611_SAMPLEINTERFACE_1.12
//******************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include "hdmitx.h"

extern BYTE I2CADR ;
extern BYTE I2CDEV ;

#define CAT6611_I2CADR      0x98
#define CAT6611_I2C_DIV     100*1000 /* standard clock 100K */

struct cat6611_i2c {
	struct i2c_client *iic_client;
	struct i2c_adapter *iic_adapter;
} *p_cat6611_i2c = NULL;

extern void ftiic010_set_clock_speed(void *, int hz);

static int __devinit cat6611_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

    if (!(p_cat6611_i2c = kzalloc(sizeof(struct cat6611_i2c), GFP_KERNEL))){
		printk("%s fail: kzalloc not OK.\n", __FUNCTION__);
		return -ENOMEM;
	}

	p_cat6611_i2c->iic_client = client;
	p_cat6611_i2c->iic_adapter = adapter;

	i2c_set_clientdata(client, p_cat6611_i2c);

	return 0;
}

static int __devexit cat6611_remove(struct i2c_client *client)
{
    if (client) {}

	kfree(p_cat6611_i2c);

	return 0;
}

static const struct i2c_device_id cat6611_id[] = {
        { "cat6611", 0 },
        { }
};

static struct i2c_driver cat6611_driver = {
        .driver = {
                .name   = "cat6611",
                .owner  = THIS_MODULE,
        },
        .probe          = cat6611_probe,
        .remove         = __devexit_p(cat6611_remove),
        .id_table       = cat6611_id
};

static struct i2c_board_info cat6611_device = {
        .type           = "cat6611",
        .addr           = (CAT6611_I2CADR >> 1),
};

void mcuio_i2c_init(void)
{
    static int  i2c_init = 0;

    if (i2c_init)
        return;

    i2c_init = 1;

    if (i2c_new_device(i2c_get_adapter(0), &cat6611_device) == NULL)
    {
        printk("%s fail: i2c_new_device not OK.\n", __FUNCTION__);
        return;
    }

    if (i2c_add_driver(&cat6611_driver))
    {
        printk("%s fail: i2c_add_driver not OK.\n", __FUNCTION__);
        return;
    }

    return;
}

/* a pair to mcuio_i2c_init() */
void mcuio_i2c_exit(void)
{
    i2c_unregister_device(p_cat6611_i2c->iic_client);
    i2c_del_driver(&cat6611_driver);
}

/* Delay ms
 */
void DelayMS(USHORT ms)
{
    msleep(ms);
}

/* ER_SUCCESS = 0, ER_FAIL = 1 */
SYS_STATUS HDMITX_WriteI2C_Byte(SHORT RegAddr, BYTE Data)
{
    struct i2c_msg  msgs[1];
	unsigned char   buf[2];

	if (unlikely(p_cat6611_i2c->iic_adapter == NULL)) {
		printk("p_cat6611_i2c->iic_adapter is NULL \n");
		return -1;
	}

	buf[0]          = RegAddr;
	buf[1]          = Data;
	msgs[0].addr    = CAT6611_I2CADR >> 1;
	msgs[0].flags   = 0; /* write */
	msgs[0].len     = 2;
	msgs[0].buf     = &buf[0];

    //ftiic010_set_clock_speed(i2c_get_adapdata(p_cat6611_i2c->iic_adapter), CAT6611_I2C_DIV);
    if (unlikely(i2c_transfer(p_cat6611_i2c->iic_adapter, msgs, 1) != 1)) {
		return ER_FAIL;
	}

    return ER_SUCCESS;
}

SYS_STATUS HDMITX_WriteI2C_ByteN(SHORT RegAddr, BYTE *pData, int N)
{
    struct i2c_msg  msgs[1];
	int             i, retVal = 0;
    unsigned char   gBuff[30];

	if (N+1 > 30)
	{
	    while (1)
	        printk("No memory in %s \n", __FUNCTION__);
	}

	gBuff[0]        = RegAddr;
	for (i = 0; i < N; i ++)
	    gBuff[i+1] = pData[i];

	msgs[0].addr    = CAT6611_I2CADR >> 1;
	msgs[0].flags   = 0; /* write */
	msgs[0].len     = N+1;
	msgs[0].buf     = &gBuff[0];

	//ftiic010_set_clock_speed(i2c_get_adapdata(p_cat6611_i2c->iic_adapter), CAT6611_I2C_DIV);
	if (unlikely(i2c_transfer(p_cat6611_i2c->iic_adapter, msgs, 1) != 1)) {
		return ER_FAIL;
	}

    return (retVal == 0) ? ER_SUCCESS : ER_FAIL;
}

BYTE HDMITX_ReadI2C_Byte(SHORT RegAddr)
{
    struct i2c_msg  msgs[2];
	unsigned char   buf[2];

	if (unlikely(p_cat6611_i2c->iic_adapter == NULL)) {
		printk("p_cat6611_i2c->iic_adapter is NULL \n");
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	buf[0]          = RegAddr;
	msgs[0].addr    = CAT6611_I2CADR >> 1;
	msgs[0].flags   = 0; /* write */
	msgs[0].len     = 1;
	msgs[0].buf     = &buf[0];

	/* setup read command */
	msgs[1].addr    = CAT6611_I2CADR >> 1;
	msgs[1].flags   = I2C_M_RD;
	msgs[1].len     = 1;
	msgs[1].buf     = &buf[1];

    //ftiic010_set_clock_speed(i2c_get_adapdata(p_cat6611_i2c->iic_adapter), CAT6611_I2C_DIV);
	if (unlikely(i2c_transfer(p_cat6611_i2c->iic_adapter, msgs, 2) != 2)) {
		return 0;
	}

    return buf[1];
}

SYS_STATUS HDMITX_ReadI2C_ByteN(SHORT RegAddr, BYTE *pData, int N)
{
    struct i2c_msg  msgs[2];
	int             i, retVal = 0;
	unsigned char   buf[1];

	if (unlikely(p_cat6611_i2c->iic_adapter == NULL)) {
		printk("p_cat6611_i2c->iic_adapter is NULL \n");
		return -1;
	}

	buf[0]          = RegAddr;
	msgs[0].addr    = CAT6611_I2CADR >> 1;
	msgs[0].flags   = 0; /* write */
	msgs[0].len     = 1;
	msgs[0].buf     = &buf[0];

	msgs[1].addr    = CAT6611_I2CADR >> 1;
	msgs[1].flags   = I2C_M_RD;
	msgs[1].len     = N;
	msgs[1].buf     = pData;

	//ftiic010_set_clock_speed(i2c_get_adapdata(p_cat6611_i2c->iic_adapter), CAT6611_I2C_DIV);
	if (unlikely(i2c_transfer(p_cat6611_i2c->iic_adapter, msgs, 2) != 2)) {
		return ER_FAIL;
	}

    return (retVal == 0) ? ER_SUCCESS : ER_FAIL;
}
