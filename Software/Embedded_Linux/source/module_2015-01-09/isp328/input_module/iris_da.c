/**
 * @file iris_da.c
 * DC IRIS driver controlled by DA
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/io.h>
#include "isp_input_inf.h"
#include "isp_api.h"
#include <linux/i2c.h>


//============================================================================
// global
//============================================================================
#define IRIS_NAME   "DA DC-IRIS"
static iris_dev_t *g_piris = NULL;

#define MCP4725_DEAFULT_DEVICE0_ADDR    0xC0    // DA0 ( Control signal )
#define MCP4725_DEAFULT_DEVICE1_ADDR    0xC2    // DA1 ( Drive signal )
#define DRIVE_MAX_OPEN_VOLTAGE          0xfff
#define DRIVE_MAX_CLOSE_VOLTAGE         0x000

struct i2c_adapter *adap = NULL;
extern struct i2c_adapter* i2c_get_adapter(int id);
extern int i2c_transfer(struct i2c_adapter * adap, struct i2c_msg *msgs, int num);

//============================================================================
// internal functions
//============================================================================
/*
    ratio(max): max speed of close
    ratio( 0 ): max speed of open
*/
static int iris_set_close_to_open(u32 ratio, u32 scale)
{
    struct i2c_msg msgs;
    char tokenvalue[2];
    u32 voltage;
    int voltage_close, voltage_open; /* must be int */

    voltage_close = DRIVE_MAX_CLOSE_VOLTAGE;
    voltage_open = DRIVE_MAX_OPEN_VOLTAGE;
    voltage = voltage_open - ABS(voltage_close-voltage_open)*ratio/scale;

    /* drive signal */
    tokenvalue[0] = (voltage >> 8) & 0xf;
    tokenvalue[1] = voltage & 0xff;
    msgs.addr = MCP4725_DEAFULT_DEVICE1_ADDR >> 1;
    msgs.flags = 0;
    msgs.len = 2;
    msgs.buf = tokenvalue;
    //printk("<IRIS_DA> set to %d\n", ratio);
    if (unlikely(i2c_transfer(adap, &msgs, 1) != 1)) {
        printk("%s fail: i2c_transfer not OK\n", __FUNCTION__);
        return -EINVAL;
    }

    return 0;
}


static int iris_init(void)
{
    struct i2c_msg msgs;
    char tokenvalue[2];

    // initial I2C
    adap = i2c_get_adapter(0);

    // control signal
    tokenvalue[0] = 0x08;
    tokenvalue[1] = 0x00;
    msgs.addr = MCP4725_DEAFULT_DEVICE0_ADDR >> 1;
    msgs.flags = 0;
    msgs.len = 2;
    msgs.buf = tokenvalue;

    if (unlikely(i2c_transfer(adap, &msgs, 1) != 1)) {
        printk("%s fail: i2c_transfer not OK\n", __FUNCTION__);
        return -EINVAL;;
    }

    // set max range for iris aperture
    return iris_set_close_to_open(0, FP10(1.0));

}

//============================================================================
// external functions
//============================================================================
int iris_da_construct(void)
{
    snprintf(g_piris->name, DEV_MAX_NAME_SIZE, IRIS_NAME);
    g_piris->fn_init = iris_init;
    g_piris->fn_close_to_open = iris_set_close_to_open;

    return iris_init();
}

void iris_da_deconstruct(void)
{
    return;
}

//============================================================================
// module initialization / finalization
//============================================================================
static int __init iris_da_init(void)
{
    int ret = 0;

    // check ISP driver version
    if (isp_get_input_inf_ver() != ISP_INPUT_INF_VER) {
        isp_err("input module version(%x) is not equal to fisp320.ko(%x)!\n",
             ISP_INPUT_INF_VER, isp_get_input_inf_ver());
        ret = -EINVAL;
        goto init_err1;
    }

    g_piris = kzalloc(sizeof(iris_dev_t), GFP_KERNEL);
    if (!g_piris) {
        isp_err("failed to allocate iris_dev_t\n");
        ret = -ENODEV;
        goto init_err1;
    }

    if ((ret = iris_da_construct()) < 0) {
        isp_err("failed to construct iris_dev_t\n");
        goto init_err2;
    }

    // register iris device to ISP driver
    if ((ret = isp_register_iris(g_piris)) < 0) {
        isp_err("failed to register iris!\n");
        goto init_err2;
    }
    return ret;

init_err2:
    kfree(g_piris);

init_err1:
    return ret;
}

static void __exit iris_da_exit(void)
{
    // remove iris
    isp_unregister_iris(g_piris);
    iris_da_deconstruct();
    if (g_piris)
        kfree(g_piris);
}

module_init(iris_da_init);
module_exit(iris_da_exit);
MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("DA_IRIS");
MODULE_LICENSE("GPL");
