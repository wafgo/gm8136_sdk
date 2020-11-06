#ifndef _GM_PWM_H_
#define _GM_PWM_H_

#define PWM_EXTCLK          30000000   // 27Mhz

#define PWM_CLKSRC_PCLK     0
#define PWM_CLKSRC_EXTCLK   1

#define PWM_DEF_CLKSRC      PWM_CLKSRC_EXTCLK
#define PWM_DEF_FREQ        20000
#define PWM_DEF_DUTY_STEPS  100
#define PWM_DEF_DUTY_RATIO  50

#define err(format, arg...)    printk(KERN_ERR "%s: " format ,  __func__, ## arg)
#define warn(format, arg...)   printk(KERN_WARNING "%s: " format,  __func__, ## arg)
#define info(format, arg...)   printk(KERN_INFO format, ## arg)
#define notice(format, arg...) printk(KERN_NOTICE format, ## arg)
//============================================================================
// PWM API
//============================================================================
bool pwm_dev_request(int id);
void pwm_tmr_start(int id);
void pwm_tmr_stop(int id);
void pwm_dev_release(int id);
void pwm_clksrc_switch(int id, int clksrc);
void pwm_set_freq(int id, u32 freq);
void pwm_set_duty_steps(int id, u32 duty_steps);
void pwm_set_duty_ratio(int id, u32 duty_ratio);
void pwm_set_interrupt_mode(int id);
void pwm_update(int id);

#endif /*_GM_PWM_H_*/
