/**
 * @file iris_pwm.c
 * DC IRIS driver controlled by PWM
 *
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2013/11/13 10:30:16 $
 *
 * ChangeLog:
 *  $Log: iris_pwm.c,v $
 *  Revision 1.2  2013/11/13 10:30:16  ricktsao
 *  no message
 *
 *  Revision 1.1  2013/11/13 06:39:06  ricktsao
 *  no message
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
#include "ftpwmtmr010.h"
#include "ioctl_pwm.h"

//============================================================================
// global
//============================================================================
#define IRIS_NAME   "PWM DC-IRIS"
static iris_dev_t *g_piris = NULL;

#define PWM_FREQ        1000000
#define PWM_CTRL_ID     0
#define PWM_DRV_ID      1

#define FP10(f)  ((u32)(f*(1<<10)+0.5))
#define DRIVE_MAX_OPEN_PWM_RATIO    FP10(0.0)
#define DRIVE_MAX_CLOSE_PWM_RATIO   FP10(1.0)
#define DRIVE_PWM_RATIO_RANGE       (DRIVE_MAX_CLOSE_PWM_RATIO - DRIVE_MAX_OPEN_PWM_RATIO)

//============================================================================
// internal functions
//============================================================================
/*
    ratio(max): max speed of close
    ratio( 0 ): max speed of open
*/
static int iris_set_close_to_open(u32 ratio, u32 scale)
{
    u32 voltage;

    voltage = ((ratio * DRIVE_PWM_RATIO_RANGE) / scale) + DRIVE_MAX_OPEN_PWM_RATIO;
    pwm_set_duty_ratio(PWM_DRV_ID, voltage);
    //printk("voltage=%d\n", voltage);
    pwm_update(PWM_DRV_ID);//Need Update
    return 0;
}

static int iris_init(void)
{
    // request PWM to control voltage
    if (!pwm_dev_request(PWM_DRV_ID)){
        err("fail to request pwm_%d\n", PWM_DRV_ID);
        return -EINVAL;;
    }

    if (!pwm_dev_request(PWM_CTRL_ID)){
        err("fail to request pwm_%d\n", PWM_CTRL_ID);
        return -EINVAL;;
    }

    // fix control signal and adjust drive signal to change aperture size
    // initlaize control signal
    pwm_tmr_stop(PWM_CTRL_ID);
    pwm_set_freq(PWM_CTRL_ID, PWM_FREQ);
    pwm_set_duty_steps(PWM_CTRL_ID, FP10(1.0));
    pwm_set_duty_ratio(PWM_CTRL_ID, FP10(0.5));
    pwm_update(PWM_CTRL_ID);//Need Update
    pwm_tmr_start(PWM_CTRL_ID);

    // initlaize drive signal
    pwm_tmr_stop(PWM_DRV_ID);
    pwm_set_freq(PWM_DRV_ID, PWM_FREQ);
    pwm_set_duty_steps(PWM_DRV_ID, FP10(1.0));
    pwm_update(PWM_DRV_ID);//Need Update
    pwm_tmr_start(PWM_DRV_ID);

    // set max range for iris aperture
    return iris_set_close_to_open(0, FP10(1.0));
}

//============================================================================
// external functions
//============================================================================
int iris_pwm_construct(void)
{
    snprintf(g_piris->name, DEV_MAX_NAME_SIZE, IRIS_NAME);
    g_piris->fn_init = iris_init;
    g_piris->fn_close_to_open = iris_set_close_to_open;

    return iris_init();
}

void iris_pwm_deconstruct(void)
{
    pwm_tmr_stop(PWM_DRV_ID);
    pwm_dev_release(PWM_DRV_ID);
    pwm_tmr_stop(PWM_CTRL_ID);
    pwm_dev_release(PWM_CTRL_ID);
}

//============================================================================
// module initialization / finalization
//============================================================================
static int __init iris_pwm_init(void)
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

    if ((ret = iris_pwm_construct()) < 0) {
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

static void __exit iris_pwm_exit(void)
{
    // remove iris
    isp_unregister_iris(g_piris);
    iris_pwm_deconstruct();
    if (g_piris)
        kfree(g_piris);
}

module_init(iris_pwm_init);
module_exit(iris_pwm_exit);
MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("PWM_IRIS");
MODULE_LICENSE("GPL");