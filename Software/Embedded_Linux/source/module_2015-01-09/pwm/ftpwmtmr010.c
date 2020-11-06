#include <linux/version.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/cdev.h>
#include <linux/synclink.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <mach/ftpmu010.h>
#include <mach/platform/platform_io.h>

#include "ftpwmtmr010.h"
#include "ioctl_pwm.h"
#if defined(CONFIG_PLATFORM_GM8139)
#define PWM4_7_MASK (BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9)
#define PWM0_3_MASK (BIT12|BIT13|BIT14|BIT15|BIT16|BIT17|BIT18|BIT19)
#endif
#if defined(CONFIG_PLATFORM_GM8136)
#define PWM4_7_MASK (BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9)
#define PWM2_3_MASK (BIT22|BIT23|BIT24|BIT25)
#define PWM0_1_MASK (BIT12|BIT13|BIT14|BIT15)
#endif

static pmuReg_t pmu_reg[] = {
#if defined(CONFIG_PLATFORM_GM8139)
    //{reg_off, bits_mask, lock_bits, init_val, init_mask},
    {0xB8, BIT9, BIT9, 0, BIT9},                // PWM clock
    {0xB8, BIT10, 0, 0, BIT10},                 // WDT clock (PWM's external clk)
    {0x50, PWM4_7_MASK, 0, 0, PWM4_7_MASK},     // PWM 4~7 pinmux
    {0x64, PWM0_3_MASK, 0, 0, PWM0_3_MASK},     // PWM 0~3 pinmux
#endif
#if defined(CONFIG_PLATFORM_GM8136)
    //{reg_off, bits_mask, lock_bits, init_val, init_mask},
    {0xB8, BIT9, BIT9, 0, BIT9},                // PWM clock
    {0xB8, BIT10, 0, 0, BIT10},                 // WDT clock (PWM's external clk)
    //{0x50, PWM4_7_MASK, 0, 0, PWM4_7_MASK},     // PWM 4~7 pinmux
    //{0x54, PWM2_3_MASK, 0, 0, PWM2_3_MASK},     // PWM 2~3 pinmux
    {0x64, PWM0_1_MASK, 0, 0, PWM0_1_MASK},     // PWM 0~1 pinmux
#endif
};

static pmuRegInfo_t pmu_reg_info = {
    "ftpwmtmr010_clk",
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_APB,
    &pmu_reg[0]
};

static int pwm_pmu_fd = 0;

//============================================================================
// data structure
//============================================================================
typedef struct _pwm_dev {
    bool used;
    u32 clksrc;
    enum pwm_set_mode mode;
    u32 freq;
    u32 duty_steps;
    u32 duty_ratio;
    u32 pattern[4];
    spinlock_t lock;
    int intr_cnt;
    u16 repeat_cnt;
    u8 pattern_len;
} pwm_dev_t;

//============================================================================
// register definitions
//============================================================================
#define INT_CSTAT       0x00
#define START_CTRL      0x04
#define TMR_REV         0x90
#define INT_PRIO        0x160

#define PWM_CTRL(id)    ((0x10*id)+0x10)
#define PWM_CNTB(id)    ((0x10*id)+0x14)
#define PWM_CMPB(id)    ((0x10*id)+0x18)
#define PWM_CNTO(id)    ((0x10*id)+0x1c)
#define PWM_REP(id)     ((0x04*id)+0xa0)
#define PWM_PAT(id, n)	((0x10*id)+0xc0+(0x04*n))
#define PWM_PATLEN(id)	((0x04*id)+0x140)
#define PWM_DEVICE_NAME "ftpwmtmr010"
#define NUM_PWM 8

#define A369_FPGA
#undef A369_FPGA

#if defined(A369_FPGA)
/* External Freq for FPGA */
#define FREQ_SETTING    1000000
#include <mach/irqs-a369.h>
#define FTPWM_IRQ       IRQ_FTAHBB020_3_EXINT(1)
#define FTPWM_BASE      0xc0100000
#define FTPWM_SIZE      0x10000
#else
#define FTPWM_IRQ       PWMTMR_FTPWM010_0_IRQ0
#define FTPWM_BASE      PWMTMR_FTPWM010_0_PA_BASE
#define FTPWM_SIZE      PWMTMR_FTPWM010_0_PA_SIZE
#endif

//============================================================================
// Global variables
//============================================================================
static int open_cnt = 0;
static DEFINE_SEMAPHORE(drv_sem);
static void __iomem * pwm_vbase = 0;
static pwm_dev_t g_pwm_info[NUM_PWM];
static struct cdev pwm_cdev;
static struct class *pwm_class = NULL;
static dev_t dev_num;

static bool pwm_reg_read(u32 offset, u32 * pvalue)
{
    if (!pwm_vbase) {
        err("PWM_VBASE not map\n");
        return false;
    }
    *pvalue = ioread32(pwm_vbase + offset);
    return true;
}

static bool pwm_reg_write(u32 offset, u32 value)
{
    if (!pwm_vbase) {
        err("PWM_VBASE not map\n");
        return false;
    }
    iowrite32(value, pwm_vbase + offset);
    return true;
}

//----------------------------------------------------------------------------
static u32 pwm_get_clock(int id)
{
    u32 tmp = 0;

    pwm_reg_read(PWM_CTRL(id), &tmp);
    if (unlikely(tmp & BIT0)) {
        return PWM_EXTCLK;
    } else {
#if defined(A369_FPGA)
        struct clk *clk;
        clk = clk_get(NULL, "extahb");
        return clk_get_rate(clk);
#endif
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
        return ftpmu010_get_attr(ATTR_TYPE_APB0);
#endif
    }
}

static void pwm_clk_enable(void)
{
#if defined(A369_FPGA)
    struct clk *clk;
    /* Setting the Ext-AHB in A369 */
    clk = clk_get(NULL, "extahb");
    clk_disable(clk);
    clk_set_rate(clk, FREQ_SETTING);
    clk_enable(clk);
    clk_put(clk);
#endif
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    pwm_pmu_fd = ftpmu010_register_reg(&pmu_reg_info);
    if (pwm_pmu_fd < 0)
        panic("PWM turn on clock fail");
#endif
}

static void pwm_clk_disable(void)
{
#if defined(A369_FPGA)
    struct clk *clk;
    /* Setting the Ext-AHB in A369 */
    clk = clk_get(NULL, "extahb");
    info("Ext AHB clock = %lu", clk_get_rate(clk));
#endif
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    ftpmu010_write_reg(pwm_pmu_fd, 0xB8, BIT9, BIT9);
#endif
}

static void pwm_set_pinmux(int id)
{
#if defined(A369_FPGA)
#endif
#if defined(CONFIG_PLATFORM_GM8139)
    u32 reg_offset, mask, mode;

    if (id < 4) {
        reg_offset = 0x64;
        mask = (BIT12 | BIT13) << (id * 2);
        mode = BIT12 << (id * 2);
    } else {
        reg_offset = 0x50;
        mask = (BIT2 | BIT3) << ((id - 4) * 2);
        mode = BIT3 << ((id - 4) * 2);
    }
    if (ftpmu010_add_lockbits(pwm_pmu_fd, reg_offset, mask) < 0)
        panic("%s: PMU add lockbits 0x%X failed\n", __func__, mask);
    ftpmu010_write_reg(pwm_pmu_fd, reg_offset, mode, mask);
#endif
#if defined(CONFIG_PLATFORM_GM8136)
    u32 reg_offset, mask, mode;

    if (id < 2) {
        reg_offset = 0x64;
        mask = (BIT12 | BIT13) << (id * 2);
        mode = BIT12 << (id * 2);
    } else if (id >= 2 && id <= 3) {
        reg_offset = 0x54;
        mask = (BIT22 | BIT23) << ((id - 2) * 2);
        mode = BIT23 << ((id - 2) * 2);
    } else {
        reg_offset = 0x50;
        mask = (BIT2 | BIT3) << ((id - 4) * 2);
        mode = BIT3 << ((id - 4) * 2);
    }
    if (ftpmu010_add_lockbits(pwm_pmu_fd, reg_offset, mask) < 0)
        panic("%s: PMU add lockbits 0x%X failed\n", __func__, mask);
    ftpmu010_write_reg(pwm_pmu_fd, reg_offset, mode, mask);
#endif
}

static void pwm_hw_init(int id)
{
    u32 tmp;

    /*
     * 1. set source clock: BIT0
     * 2. timerx output inverter: BIT2
     * 3. pwm_set_mode: BIT3~4
     * 4. enable PWM function: BIT8
     */
    tmp = g_pwm_info[id].clksrc | BIT2 | (g_pwm_info[id].mode << 3) | BIT8;
    pwm_reg_write(PWM_CTRL(id), tmp);
}

void pwm_update(int id)
{
    u32 tmp;

    pwm_reg_read(PWM_CTRL(id), &tmp);
    tmp |= BIT1;
    pwm_reg_write(PWM_CTRL(id), tmp);
}
EXPORT_SYMBOL(pwm_update);

static void pwm_set_tmrcnt(int id, u32 freq, u32 steps, u32 duty)
{
    u32 clk, cntb, cmpb;

    // get clock
    clk = pwm_get_clock(id);

    // set timer counting value : frequency
    cntb = clk / freq;
    cntb -= 1;
    pwm_reg_write(PWM_CNTB(id), cntb);

    // set timer counter compare : duty ratio
    cmpb = cntb * duty / steps;
    if (cmpb == 0)
        cmpb = 1;
    pwm_reg_write(PWM_CMPB(id), cmpb);

    // update
    //pwm_update(id);

    //notice("cntb = %d, cmpb = %d\n", cntb, cmpb);
}

//============================================================================
// functions
//============================================================================
bool pwm_dev_request(int id)
{
    unsigned long flags;
    bool ret = true;
    u32 tmp = 0;

    if (id >= NUM_PWM) {
        err("%s: There's No PWM %d\n", __func__, id);
        return false;
    }

    spin_lock_irqsave(&g_pwm_info[id].lock, flags);
    pwm_reg_read(START_CTRL, &tmp);
    if (g_pwm_info[id].used && (tmp & (1 << id))) {
        ret = false;
        warn("%s: PWM %d has been used\n", __func__, id);
    } else {
        g_pwm_info[id].used = true;
    }
    spin_unlock_irqrestore(&g_pwm_info[id].lock, flags);

    if (!ret)
        return ret;

    // set pinmux
    pwm_set_pinmux(id);

    // initialize PWM
    pwm_hw_init(id);
    pwm_set_tmrcnt(id, g_pwm_info[id].freq, g_pwm_info[id].duty_steps, g_pwm_info[id].duty_ratio);
    return true;
}

EXPORT_SYMBOL(pwm_dev_request);

//----------------------------------------------------------------------------
void pwm_tmr_start(int id)
{
    u32 tmp = 0;

    pwm_reg_read(START_CTRL, &tmp);
    tmp |= (1 << id);
    pwm_reg_write(START_CTRL, tmp);
}

EXPORT_SYMBOL(pwm_tmr_start);

//----------------------------------------------------------------------------
void pwm_tmr_stop(int id)
{
    u32 tmp = 0;

    pwm_reg_read(START_CTRL, &tmp);
    tmp &= ~(1 << id);
    pwm_reg_write(START_CTRL, tmp);
}

EXPORT_SYMBOL(pwm_tmr_stop);

//----------------------------------------------------------------------------
void pwm_dev_release(int id)
{
    unsigned long flags;
    u32 mask, reg_offset;

    // stop PWM timer
    pwm_tmr_stop(id);

    // release PWM device
    spin_lock_irqsave(&g_pwm_info[id].lock, flags);
    g_pwm_info[id].used = false;
#if defined(CONFIG_PLATFORM_GM8139)
    if (id < 4) {
        reg_offset = 0x64;
        mask = (BIT12 | BIT13) << (id * 2);
    } else {
        reg_offset = 0x50;
        mask = (BIT2 | BIT3) << ((id - 4) * 2);
    }
#endif
#if defined(CONFIG_PLATFORM_GM8136)
    if (id < 2) {
        reg_offset = 0x64;
        mask = (BIT12 | BIT13) << (id * 2);
    }
    else if (id >= 2 && id <= 3) {
        reg_offset = 0x54;
        mask = (BIT22 | BIT23) << ((id - 2) * 2);
    }
    else {
        reg_offset = 0x50;
        mask = (BIT2 | BIT3) << ((id - 4) * 2);
    }
#endif
    if (ftpmu010_bits_is_locked(pwm_pmu_fd, reg_offset, mask) == 0)
        ftpmu010_del_lockbits(pwm_pmu_fd, reg_offset, mask);
    spin_unlock_irqrestore(&g_pwm_info[id].lock, flags);
}

EXPORT_SYMBOL(pwm_dev_release);

//----------------------------------------------------------------------------
void pwm_clksrc_switch(int id, int clksrc)
{
    u32 tmp = 0;

    if (clksrc == g_pwm_info[id].clksrc)
        return;

    pwm_reg_read(PWM_CTRL(id), &tmp);
    tmp &= ~BIT0;
    tmp |= clksrc;
    pwm_reg_write(PWM_CTRL(id), tmp);
    g_pwm_info[id].clksrc = clksrc;
//      pwm_set_tmrcnt(id, g_pwm_info[id].freq, g_pwm_info[id].duty_steps,
//                     g_pwm_info[id].duty_ratio);
}

EXPORT_SYMBOL(pwm_clksrc_switch);

//----------------------------------------------------------------------------
void pwm_set_freq(int id, u32 freq)
{
    if (freq == g_pwm_info[id].freq)
        return;

    g_pwm_info[id].freq = freq;
    pwm_set_tmrcnt(id, g_pwm_info[id].freq, g_pwm_info[id].duty_steps, g_pwm_info[id].duty_ratio);
}

EXPORT_SYMBOL(pwm_set_freq);

//----------------------------------------------------------------------------
void pwm_set_duty_steps(int id, u32 duty_steps)
{
    if (duty_steps == g_pwm_info[id].duty_steps)
        return;

    g_pwm_info[id].duty_steps = duty_steps;
    pwm_set_tmrcnt(id, g_pwm_info[id].freq, g_pwm_info[id].duty_steps, g_pwm_info[id].duty_ratio);
}

EXPORT_SYMBOL(pwm_set_duty_steps);

//----------------------------------------------------------------------------
void pwm_set_duty_ratio(int id, u32 duty_ratio)
{
    if (duty_ratio == g_pwm_info[id].duty_ratio)
        return;

    g_pwm_info[id].duty_ratio = duty_ratio;
    pwm_set_tmrcnt(id, g_pwm_info[id].freq, g_pwm_info[id].duty_steps, g_pwm_info[id].duty_ratio);
}

EXPORT_SYMBOL(pwm_set_duty_ratio);

//----------------------------------------------------------------------------
void pwm_mode_change(int id, enum pwm_set_mode mode)
{
    u32 tmp;

    pwm_reg_read(PWM_CTRL(id), &tmp);

    tmp &= ~(BIT3 | BIT4);
    tmp |= (mode << 3);

    pwm_reg_write(PWM_CTRL(id), tmp);
    pwm_reg_read(PWM_CTRL(id), &tmp);
    printk("Set PWM Mode as 0x%X\n", tmp);
}

EXPORT_SYMBOL(pwm_mode_change);

//----------------------------------------------------------------------------
void pwm_interrupt_enable(int id)
{
    u32 tmp;

    pwm_reg_read(PWM_CTRL(id), &tmp);
    tmp |= BIT5;                /* Turn On Interrupt Enable */
    pwm_reg_write(PWM_CTRL(id), tmp);
}

EXPORT_SYMBOL(pwm_interrupt_enable);

//----------------------------------------------------------------------------
void pwm_interrupt_disable(int id)
{
    u32 tmp;

    pwm_reg_read(PWM_CTRL(id), &tmp);
    tmp &= ~BIT5;               /* Turn On Interrupt Enable */
    pwm_reg_write(PWM_CTRL(id), tmp);
}

EXPORT_SYMBOL(pwm_interrupt_disable);

//----------------------------------------------------------------------------
#include <linux/delay.h>
static irqreturn_t pwm_isr(int irq, void *dev_id)
{
    u32 tmp = ioread32(pwm_vbase + INT_PRIO);
    unsigned long flags;
    u8 id = 0;

    id = (tmp & 0x0f) - 1;

    spin_lock_irqsave(&g_pwm_info[id].lock, flags);

    pwm_reg_write(INT_CSTAT, 1 << id);  /* write 1 clear */

    g_pwm_info[id].intr_cnt--;

    if (g_pwm_info[id].intr_cnt > 0) {
        if (g_pwm_info[id].mode == PWM_REPEAT || g_pwm_info[id].mode == PWM_PATTERN) {
            pwm_reg_write(PWM_REP(id), g_pwm_info[id].repeat_cnt);
        }
        pwm_update(id);
    }

    spin_unlock_irqrestore(&g_pwm_info[id].lock, flags);
    return IRQ_HANDLED;
}

//============================================================================
// file operation functions
//============================================================================
static int pwm_open(struct inode *inode, struct file *filp)
{
    int ret = 0;

    down(&drv_sem);

    if (open_cnt == 0) {
        open_cnt++;
    } else
        ret = -EINVAL;

    up(&drv_sem);

    return ret;
}

//----------------------------------------------------------------------------
static int pwm_release(struct inode *inode, struct file *filp)
{
    int i, ret = 0;

    down(&drv_sem);

    for (i = 0; i < NUM_PWM; i++) {
        if (g_pwm_info[i].used == true) {
            pwm_dev_release(i);
        }
    }
    open_cnt = 0;

    up(&drv_sem);

    return ret;
}

//----------------------------------------------------------------------------
static long pwm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret = 0;

    if (_IOC_TYPE(cmd) != PWM_IOC_MAGIC)
        return -ENOTTY;
    if (_IOC_NR(cmd) > PWM_IOC_MAXNR)
        return -ENOTTY;
    if (_IOC_DIR(cmd) & _IOC_READ)
        ret = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        ret = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (ret)
        return -EFAULT;

    switch (cmd) {
    case PWM_IOCTL_REQUEST:
        {
            int devNum;
            if (copy_from_user(&devNum, (void __user *)arg, sizeof(devNum))) {
                ret = -EFAULT;
                break;
            }
            if (!pwm_dev_request(devNum)) {
                err("request pwm %d fail\n", devNum);
                ret = -EINVAL;
            }
            break;
        }
    case PWM_IOCTL_START:
        {
            int devNum;
            if (copy_from_user(&devNum, (void __user *)arg, sizeof(devNum))) {
                ret = -EFAULT;
                break;
            }
            if (devNum >= NUM_PWM) {
                err("your pwm %d is illegal\n", devNum);
                ret = -EINVAL;
                break;
            }
            if (g_pwm_info[devNum].used == true) {
                pwm_tmr_start(devNum);
            } else {
                err("your pwm %d isn't requested yet.\n", devNum);
                ret = -EINVAL;
            }
            break;
        }
    case PWM_IOCTL_STOP:
        {
            int devNum;
            if (copy_from_user(&devNum, (void __user *)arg, sizeof(devNum))) {
                ret = -EFAULT;
                break;
            }
            if (devNum >= NUM_PWM) {
                err("your pwm %d is illegal\n", devNum);
                ret = -EINVAL;
                break;
            }
            pwm_tmr_stop(devNum);
            break;
        }
    case PWM_IOCTL_GET_INFO:
        {
            pwm_info_t info;
            if (copy_from_user(&info, (void __user *)arg, sizeof(pwm_info_t))) {
                ret = -EFAULT;
                break;
            }
            if (info.id >= NUM_PWM) {
                err("your pwm %d is illegal\n", info.id);
                ret = -EINVAL;
                break;
            }
            info.clksrc = g_pwm_info[info.id].clksrc;
            info.freq = g_pwm_info[info.id].freq;
            info.duty_steps = g_pwm_info[info.id].duty_steps;
            info.duty_ratio = g_pwm_info[info.id].duty_ratio;
            info.mode = g_pwm_info[info.id].mode;
            info.intr_cnt = g_pwm_info[info.id].intr_cnt;
            info.repeat_cnt = g_pwm_info[info.id].repeat_cnt;
            info.pattern_len = g_pwm_info[info.id].pattern_len;
            memcpy(info.pattern, g_pwm_info[info.id].pattern, 4 * sizeof(u32));
            ret = copy_to_user((void __user *)arg, &info, sizeof(info)) ? -EFAULT : 0;
            break;
        }

    case PWM_IOCTL_SET_CLKSRC:
        {
            pwm_info_t info;
            if (copy_from_user(&info, (void __user *)arg, sizeof(pwm_info_t))) {
                ret = -EFAULT;
                break;
            }
            if (info.id >= NUM_PWM) {
                err("your pwm %d is illegal\n", info.id);
                ret = -EINVAL;
                break;
            }
            pwm_clksrc_switch(info.id, info.clksrc);
            break;
        }

    case PWM_IOCTL_SET_FREQ:
        {
            pwm_info_t info;
            if (copy_from_user(&info, (void __user *)arg, sizeof(pwm_info_t))) {
                ret = -EFAULT;
                break;
            }
            if (info.id >= NUM_PWM) {
                err("Your PWM %d is illegal\n", info.id);
                ret = -EINVAL;
                break;
            }
            pwm_set_freq(info.id, info.freq);
            break;
        }

    case PWM_IOCTL_SET_DUTY_STEPS:
        {
            pwm_info_t info;
            if (copy_from_user(&info, (void __user *)arg, sizeof(pwm_info_t))) {
                ret = -EFAULT;
                break;
            }
            if (info.id >= NUM_PWM) {
                err("Your PWM %d is illegal\n", info.id);
                ret = -EINVAL;
                break;
            }
            pwm_set_duty_steps(info.id, info.duty_steps);
            break;
        }

    case PWM_IOCTL_SET_DUTY_RATIO:
        {
            pwm_info_t info;
            if (copy_from_user(&info, (void __user *)arg, sizeof(pwm_info_t))) {
                ret = -EFAULT;
                break;
            }
            if (info.id >= NUM_PWM) {
                err("Your PWM %d is illegal\n", info.id);
                ret = -EINVAL;
                break;
            }
            pwm_set_duty_ratio(info.id, info.duty_ratio);
            break;
        }

    case PWM_IOCTL_SET_MODE:
        {
            pwm_info_t info;
            int i;
            if (copy_from_user(&info, (void __user *)arg, sizeof(pwm_info_t))) {
                ret = -EFAULT;
                break;
            }
            if (info.id >= NUM_PWM) {
                err("Your PWM %d is illegal\n", info.id);
                ret = -EINVAL;
                break;
            }
            switch (info.mode) {
            case PWM_ONESHOT:
                info("Set PWM%d as OneShot Mode", info.id);
                break;
            case PWM_INTERVAL:
                info("Set PWM%d as Interval Mode", info.id);
                break;
            case PWM_REPEAT:
                g_pwm_info[info.id].repeat_cnt = info.repeat_cnt;
                pwm_reg_write(PWM_REP(info.id), info.repeat_cnt);
                info("Set PWM%d as Repeat Mode", info.id);
                break;
            case PWM_PATTERN:
                /* Set Pattern */
                memcpy(g_pwm_info[info.id].pattern, info.pattern, 4 * sizeof(u32));
                for (i = 0; i < 4; i++)
                    pwm_reg_write(PWM_PAT(info.id, i), info.pattern[i]);

                /* Set Pattern length */
                g_pwm_info[info.id].pattern_len = info.pattern_len;
                pwm_reg_write(PWM_PATLEN(info.id), info.pattern_len);

                /* Set Repeat count */
                g_pwm_info[info.id].repeat_cnt = info.repeat_cnt;
                pwm_reg_write(PWM_REP(info.id), info.repeat_cnt);
                info("Set PWM%d as Pattern Mode", info.id);
                break;
            default:
                err("No such PWM mode");
                info.mode = g_pwm_info[info.id].mode;
                ret = -EINVAL;
                break;
            }
            g_pwm_info[info.id].mode = info.mode;
            g_pwm_info[info.id].intr_cnt = info.intr_cnt;
            pwm_mode_change(info.id, info.mode);
            break;
        }

    case PWM_IOCTL_ENABLE_INTERRUPT:
        {
            int devNum;
            if (copy_from_user(&devNum, (void __user *)arg, sizeof(devNum))) {
                ret = -EFAULT;
                break;
            }
            if (devNum >= NUM_PWM) {
                err("Your PWM %d is illegal\n", devNum);
                ret = -EINVAL;
                break;
            }
            pwm_interrupt_enable(devNum);
            break;
        }

    case PWM_IOCTL_DISABLE_INTERRUPT:
        {
            int devNum;
            if (copy_from_user(&devNum, (void __user *)arg, sizeof(devNum))) {
                ret = -EFAULT;
                break;
            }
            if (devNum >= NUM_PWM) {
                err("Your PWM %d is illegal\n", devNum);
                ret = -EINVAL;
                break;
            }
            pwm_interrupt_disable(devNum);
            break;
        }

    case PWM_IOCTL_ALL_START:
        {
            u32 tmp = 0;
            int i;

            for (i = 0; i < NUM_PWM; i++) {
                if (g_pwm_info[i].used == true) {
                    tmp |= (1 << i);
                }
            }
            pwm_reg_write(START_CTRL, tmp);
            pwm_reg_read(START_CTRL, &tmp);
            printk("STAR_CTRL reg = 0x%x\n", tmp);
        }
        break;

    case PWM_IOCTL_ALL_STOP:
        pwm_reg_write(START_CTRL, 0x0);
        break;

    case PWM_IOCTL_UPDATE:
        {
            int devNum;
            if (copy_from_user(&devNum, (void __user *)arg, sizeof(devNum))) {
                ret = -EFAULT;
                break;
            }
            if (devNum >= NUM_PWM) {
                err("Your PWM %d is illegal\n", devNum);
                ret = -EINVAL;
                break;
            }
            pwm_update(devNum);
            break;
        }

    case PWM_IOCTL_ALL_UPDATE:
        {
            int i;

            for (i = 0; i < NUM_PWM; i++) {
                if (g_pwm_info[i].used == true) {
                    pwm_update(i);
                }
            }
        }
        break;

    case PWM_IOCTL_RELEASE:
        {
            int devNum;
            if (copy_from_user(&devNum, (void __user *)arg, sizeof(devNum))) {
                ret = -EFAULT;
                break;
            }
            if (devNum >= NUM_PWM) {
                err("Your PWM %d is illegal\n", devNum);
                ret = -EINVAL;
                break;
            }
            pwm_dev_release(devNum);
            break;
        }

    default:
        ret = -ENOIOCTLCMD;
        err("Does not support this command.(0x%x)", cmd);
        break;
    }

    return ret;
}

struct file_operations pwm_fops = {
    .owner = THIS_MODULE,
    .open = pwm_open,
    .release = pwm_release,
    .unlocked_ioctl = pwm_ioctl,
};

//============================================================================
// module initialization / finalization
//============================================================================
static void ftpwmtmr010_infoinit(void)
{
    int i;

    memset(g_pwm_info, 0, sizeof(g_pwm_info));

    for (i = 0; i < NUM_PWM; i++) {
        g_pwm_info[i].used = false;
        g_pwm_info[i].clksrc = PWM_DEF_CLKSRC;
        g_pwm_info[i].mode = PWM_INTERVAL;
        g_pwm_info[i].freq = PWM_DEF_FREQ;
        g_pwm_info[i].duty_steps = PWM_DEF_DUTY_STEPS;
        g_pwm_info[i].duty_ratio = PWM_DEF_DUTY_RATIO;
        spin_lock_init(&g_pwm_info[i].lock);
    }
}

static int ftpwmtmr010_chdrv_register(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, PWM_DEVICE_NAME);
    if (unlikely(ret < 0)) {
        err("%s:alloc_chrdev_region failed\n", __func__);
        goto err1;
    }
    cdev_init(&pwm_cdev, &pwm_fops);
    pwm_cdev.owner = THIS_MODULE;
    pwm_cdev.ops = &pwm_fops;
    ret = cdev_add(&pwm_cdev, dev_num, 1);
    if (unlikely(ret < 0)) {
        err("%s:cdev_add failed\n", __func__);
        goto err3;
    }
    pwm_class = class_create(THIS_MODULE, "pwm");
    if (IS_ERR(pwm_class)) {
        err("%s:class_create failed\n", __func__);
        goto err2;
    }
    device_create(pwm_class, NULL, pwm_cdev.dev, NULL, PWM_DEVICE_NAME);
    notice("Register PWM character driver success!\n");
  err1:
    return ret;
  err2:
    cdev_del(&pwm_cdev);
  err3:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void ftpwmtmr010_chdrv_unregister(void)
{
    unregister_chrdev_region(dev_num, 1);
    cdev_del(&pwm_cdev);
    device_destroy(pwm_class, dev_num);
    class_destroy(pwm_class);
}

static int __init ftpwmtmr010_init(void)
{
    // turn on PWM clock
    pwm_clk_enable();

    // map register address
    pwm_vbase = ioremap_nocache(FTPWM_BASE, FTPWM_SIZE);
    info("ftpwmtmr010(0x%x) mapping address = 0x%x\n", FTPWM_BASE, (u32)pwm_vbase);

    // initialize PWM device information
    ftpwmtmr010_infoinit();

    // Setting the IRQ type
    irq_set_irq_type(FTPWM_IRQ, IRQ_TYPE_LEVEL_HIGH);

    // set ISR
    if (request_irq(FTPWM_IRQ, pwm_isr, IRQF_SHARED, "PWM", &g_pwm_info) < 0) {
        err("%s: request_irq %d failed\n", __func__, FTPWM_IRQ);
    }
    // register char device and generate device node
    return ftpwmtmr010_chdrv_register();
}

//----------------------------------------------------------------------------
static void __exit ftpwmtmr010_exit(void)
{
    // free irq
    free_irq(FTPWM_IRQ, &g_pwm_info);

    // unmap register address
    iounmap(pwm_vbase);

    // unregister char driver
    ftpwmtmr010_chdrv_unregister();

    // turn off PWM clock
    pwm_clk_disable();

#if defined(A369_FPGA)
#endif
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    // unregister pmu
    ftpmu010_deregister_reg(pwm_pmu_fd);
#endif
}

//----------------------------------------------------------------------------
module_init(ftpwmtmr010_init);
module_exit(ftpwmtmr010_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("GM PWM Driver");
MODULE_LICENSE("GPL");
