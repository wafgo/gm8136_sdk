/**
 * @file lens_9510a2.c
 * Largan lens 9510A2 driver
 *
 * Copyright (C) 2010 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2014/05/07 09:36:34 $
 *
 * ChangeLog:
 *  $Log: lens_9510a2.c,v $
 *  Revision 1.1  2014/05/07 09:36:34  allenhsu
 *  *** empty log message ***
 *
 *  Revision 1.1  2012/02/15 06:40:27  ricktsao
 *  no message
 *
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/mm.h>
#include <mach/ftpmu010.h>

#define PFX "lens_9510a2"
#include "lens_9510a2.h"
#include "ioctl_isp320.h"

//============================================================================
// global
//============================================================================
#define LENS_NAME           "9510A2"

#define GPIO_ID(group, pin)     ((32*group)+pin)
#define PIN_NUM(id)             (id%32)
#define AF_DC_PIN           GPIO_ID(1, 28)
#define POWER_PIN           GPIO_ID(1, 29)
#define PWM_ID              0

#define PWM_FREQ            60000
#define PWM_DUTY_STEPS      300

#define FOCUS_DIR_INIT      0
#define FOCUS_DIR_INF       1
#define FOCUS_DIR_NEAR      (-1)

#define FOCUS_POS_STEPS     300
#define FOCUS_POS_MAX       200
#define FOCUS_POS_MIN       80
#define FOCUS_POS_DEF       100

typedef struct lens_info {
    int is_init;
    int current_fdir;
    int focus_pos;
} lens_info_t;

static lens_dev_t *g_plens = NULL;
//============================================================================
// internal functions
//============================================================================
static inline void vcm_power_ctrl(int on)
{
    if (on)
        gpio_set_value(POWER_PIN, 1);
    else
        gpio_set_value(POWER_PIN, 0);
}

static inline int vcm_get_pwm_duty_ratio(int pos)
{
    return ((pos*PWM_DUTY_STEPS) / FOCUS_POS_STEPS);
}

static int vcm_inf_focus(int steps)
{
    lens_dev_t *pdev = g_plens; 
    lens_info_t *pinfo = (lens_info_t *)pdev->private;

    int move_steps = steps;

    if ((pinfo->focus_pos + steps) > FOCUS_POS_MAX) {
        move_steps = FOCUS_POS_MAX - pinfo->focus_pos;
    }

    pinfo->current_fdir = FOCUS_DIR_INF;
    if (move_steps == 0)
        return 0;

    pinfo->focus_pos += move_steps;
    pwm_set_duty_ratio(PWM_ID, vcm_get_pwm_duty_ratio(pinfo->focus_pos));
    pwm_update(PWM_ID);//Need Update
    return move_steps;
}

static int vcm_near_focus(int steps)
{
    lens_dev_t *pdev = g_plens; 
    lens_info_t *pinfo = (lens_info_t *)pdev->private;
    int move_steps = steps;

    if ((pinfo->focus_pos - steps) < FOCUS_POS_MIN) {
        move_steps = pinfo->focus_pos - FOCUS_POS_MIN;
    }

    pinfo->current_fdir = FOCUS_DIR_NEAR;
    if (move_steps == 0)
        return 0;

    pinfo->focus_pos -= move_steps;
    pwm_set_duty_ratio(PWM_ID, vcm_get_pwm_duty_ratio(pinfo->focus_pos));
    pwm_update(PWM_ID);//Need Update    
    return move_steps;
}

static int focus_move_to_pi(void)
{
    lens_dev_t *pdev = g_plens; 
    lens_info_t *pinfo = (lens_info_t *)pdev->private;

    if (pinfo->current_fdir == FOCUS_DIR_INIT)
        return 0;

    pinfo->focus_pos = FOCUS_POS_MIN;
    pinfo->current_fdir = FOCUS_DIR_INIT;
    pwm_set_duty_ratio(PWM_ID, vcm_get_pwm_duty_ratio(FOCUS_POS_MIN));

    return 0;
}

static s32 focus_get_pos(void)
{
    lens_dev_t *pdev = g_plens; 
    lens_info_t *pinfo = (lens_info_t *)pdev->private;
    return pinfo->focus_pos;
}

static s32 focus_set_pos(s32 pos)
{
    lens_dev_t *pdev = g_plens; 
    lens_info_t *pinfo = (lens_info_t *)pdev->private;
    s32 steps;

    if (pos == pinfo->focus_pos)
        return 0;

    if (pos > pinfo->focus_pos)
        steps = vcm_inf_focus(pos - pinfo->focus_pos);
    else
        steps = vcm_near_focus(pinfo->focus_pos - pos);

    return steps;
}

static s32 focus_move(s32 steps)
{
    s32 move_steps = 0;

    if (steps > 0) {
        move_steps = vcm_inf_focus(steps);
    } else {
        steps *= -1;
        move_steps = vcm_near_focus(steps);
    }

    return move_steps;
}

static int lens_init(void)
{
    lens_dev_t *pdev = g_plens;
    lens_info_t *pinfo = (lens_info_t *)pdev->private;

    if (pinfo->is_init)
        return 0;

    pinfo->current_fdir = FOCUS_DIR_INIT;
    pinfo->focus_pos = FOCUS_POS_MIN;

    // initlaize PWM
    if (!pwm_dev_request(PWM_ID)){
        err("Fail to request PWM%d\n", PWM_ID);
        return -ENXIO;
    }
    pwm_tmr_stop(PWM_ID);
    pwm_set_freq(PWM_ID, PWM_FREQ);
    pwm_set_duty_steps(PWM_ID, PWM_DUTY_STEPS);
    pwm_set_duty_ratio(PWM_ID, vcm_get_pwm_duty_ratio(FOCUS_POS_MIN));
    pwm_update(PWM_ID);//Need Update
    pwm_tmr_start(PWM_ID);
    printk("VCM inti success\n");
    // enable VCM
    vcm_power_ctrl(1);

    focus_move_to_pi();
    focus_set_pos(FOCUS_POS_DEF);
    pinfo->is_init = 1;

    return 0;
}

//============================================================================
// external functions
//============================================================================
static pmuReg_t vcm_pmu[] = {
    /*
     * Multi-Function Port Setting Register 5 [offset=0x64]
     * ----------------------------------------------------
     *  [15:14] 0:GPIO_1[29] 1:PWM1
     */
    {
     .reg_off   = 0x64,
     .bits_mask = 0x0000C000,
     .lock_bits = 0x0000C000,   ///< default don't lock these bit
     .init_val  = 0,
     .init_mask = 0,            ///< don't init
    }
};
static pmuRegInfo_t vcm_pmu_info = {
    "VCM",
    ARRAY_SIZE(vcm_pmu),
    ATTR_TYPE_AHB,
    vcm_pmu
};

static int vcm_pmu_fd = -1;
void lens_9510a2_deconstruct(void);
static int lens_9510a2_construct(void)
{
    lens_dev_t *pdev = g_plens;
    lens_info_t *pinfo = NULL;
    int ret = 0;

    // register PMU
    if (vcm_pmu_fd < 0) {
        vcm_pmu_fd = ftpmu010_register_reg(&vcm_pmu_info);
        if (vcm_pmu_fd < 0) {
            err("PMU register fail");
            return -ENXIO;
        }
    }

    // initial GPIO_1[29] as VCM power control
    if (gpio_request(POWER_PIN, "VCM_PWR") < 0) {
        ret = -ENXIO;
        goto err;
    }
    gpio_direction_output(POWER_PIN, 1);
    gpio_set_value(POWER_PIN, 0);

    // set pinmux to GPIO_1[29]
    ftpmu010_write_reg(vcm_pmu_fd, 0x64, 0x0000, 0xC000);


    // allocate memory for lens information
    pinfo = kzalloc(sizeof(lens_info_t), GFP_KERNEL);
    if (!pinfo) {
        err("Allocate lens_info_t fail!");
        ret = -ENOMEM;
        goto err;
    }
    pdev->private = pinfo;

    snprintf(pdev->name, DEV_MAX_NAME_SIZE, LENS_NAME);

    pdev->capability = LENS_SUPPORT_FOCUS;
    pdev->fn_init = lens_init;
    pdev->fn_focus_move_to_pi = focus_move_to_pi;
    pdev->fn_focus_get_pos = focus_get_pos;
    pdev->fn_focus_set_pos = focus_set_pos;
    pdev->fn_focus_move = focus_move;
    pdev->focus_low_bound = FOCUS_POS_MIN;
    pdev->focus_hi_bound = FOCUS_POS_MAX;
    
    pdev->fn_zoom_move_to_pi = NULL;
    pdev->fn_zoom_get_pos = NULL;
    pdev->fn_zoom_set_pos = NULL;
    pdev->fn_zoom_move = NULL;

    ret = lens_init();
    if (!ret)
        return ret;

err:
    lens_9510a2_deconstruct();
    return ret;
}

void lens_9510a2_deconstruct(void)
{
    lens_dev_t *pdev = g_plens;
    lens_info_t *pinfo = (lens_info_t *)pdev->private;

    gpio_free(POWER_PIN);

    if (pinfo) {
        pwm_tmr_stop(PWM_ID);
        pwm_dev_release(PWM_ID);
        kfree(pinfo);
    }

    if (vcm_pmu_fd > 0)
        ftpmu010_deregister_reg(vcm_pmu_fd);
}


//============================================================================
// module initialization / finalization
//============================================================================
static int __init lens_9510a2_init(void)
{
    int ret = 0;

    // check ISP driver version
    if (isp_get_input_inf_ver() != ISP_INPUT_INF_VER) {
        ret = -EFAULT;
        err("Input module version(%x) is not compatibility with fisp320.ko(%x).!",
             ISP_INPUT_INF_VER, isp_get_input_inf_ver());
        goto end;
    }

    // allocate lens device information
    g_plens = kzalloc(sizeof(lens_dev_t), GFP_KERNEL);
    if (!g_plens) {
        ret = -ENODEV;
        err("[ERROR]fail to alloc dev_lens!\n");
        goto end;
    }

    // construct lens device information
    ret = lens_9510a2_construct();
    if (ret < 0) {
        ret = -ENODEV;
        err("[ERROR]fail to construct dev_lens!\n");
        goto err_free_lens;
    }

    isp_register_lens(g_plens);
    
    goto end;

err_free_lens:
    kfree(g_plens);
end:
    return ret;
}

static void __exit lens_9510a2_exit(void)
{
    // remove lens
    lens_9510a2_deconstruct();
    kfree(g_plens);
}

module_init(lens_9510a2_init);
module_exit(lens_9510a2_exit);
MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("LENS_9510A2");
MODULE_LICENSE("GPL");

