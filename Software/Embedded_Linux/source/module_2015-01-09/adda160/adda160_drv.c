#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <asm/div64.h>
#include <asm/io.h>
#include <mach/platform/board.h>
#include <mach/ftpmu010_pcie.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include "adda160.h"

static int pmu_fd = -1;

static int vol = 0x0;
static int mute = 0x0;
static int iv = 0x0;
static struct proc_dir_entry *entity_proc = NULL;
static struct proc_dir_entry *vol_d = NULL;
static struct proc_dir_entry *mute_d = NULL;
static struct proc_dir_entry *iv_d = NULL;

static pmuPcieReg_t reg_array_adda160_pcie[] = {
    /* reg_off,  bit_masks,     lock_bits,      init_val,   init_mask */
    { 0x30,     (0x3 << 14),    (0x3 << 14),    0x0,        (0x3 << 14)},   // audio ad/da mclk on, SSP0/1 clk on
    { 0x48,     0x3,            0x3,            0x3,        0x3        },   // audio ad/da mclk, SSP0/1 clk : 12MHz
    { 0x64,     0xFFFFFFFF,     0xFFFFFFFF,     0,          0          },
    { 0x68,     0xFFFFFFFF,     0xFFFFFFFF,     0,          0          },
    { 0x6c,     0xFFFFFFFF,     0xFFFFFFFF,     0,          0          },
};

static pmuPcieRegInfo_t adda160_pcie_clk_info = {
    "adda160_pcie_clk",
    ARRAY_SIZE(reg_array_adda160_pcie),
    ATTR_TYPE_PCIE_NONE,
    reg_array_adda160_pcie,
};

#if  DEBUG_ADDA160
void dump_adda160_reg(void)
{
    DEBUG_ADDA160_PRINT("\n0x30 = %x\n", readl(PCIPMU_FTPMU010_VA_BASE + 0x30));
    DEBUG_ADDA160_PRINT("0x48 = %x\n", readl(PCIPMU_FTPMU010_VA_BASE + 0x48));
    DEBUG_ADDA160_PRINT("0x64 = %x\n", readl(PCIPMU_FTPMU010_VA_BASE + 0x64));
    DEBUG_ADDA160_PRINT("0x68 = %x\n", readl(PCIPMU_FTPMU010_VA_BASE + 0x68));
    DEBUG_ADDA160_PRINT("0x6c = %x\n", readl(PCIPMU_FTPMU010_VA_BASE + 0x6c));

}
#endif //end of DEBUG_ADDA160

static void adda160_pmu_register(void)
{
    pmu_fd = ftpmu010_pcie_register_reg(&adda160_pcie_clk_info);
    if (pmu_fd < 0) {
        panic("In %s: register pmu setting failed\n", __func__);
    }
}

static void adda160_i2s_init(void)
{
    volatile u32 tmp = 0;
    volatile u32 mask = 0;

    /* reset */
    tmp = 1 << 19;
    mask = 1 << 19;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x6c, tmp, mask) < 0)
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x6c);
    mdelay(10);
    tmp = 0;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x6c, tmp, mask) < 0)
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x6c);

    /* I2S mode */
    tmp = ftpmu010_pcie_read_reg(0x64);
    tmp &= ~(1 << 12);              // ADC unmute
    tmp |= (1 << 17);               // set to I2S mode
    //tmp |= (0x06 << 28);            // default MCLK AD mode set to serve 48K sampling rate
    //tmp |= (0x06 << 24);            // default MCLK DA mode set to serve 48K sampling rate
    tmp |= (0x0f << 28);            // default MCLK AD mode set to serve 48K sampling rate
    tmp |= (0x0f << 24);            // default MCLK DA mode set to serve 48K sampling rate
    tmp |= 0x3F ;                   // set DAC digital volume control
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x64, tmp, mask) < 0)
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x64);

    /* set DAC power on & DAC unmute */
    tmp = ftpmu010_pcie_read_reg(0x68);
    tmp &= ~0x03;
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x68, tmp, mask) < 0) {
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x68);
    }
    /* set master/slave mode */
    tmp = ftpmu010_pcie_read_reg(0x6C);
#if SSP_SLAVE_MODE
    tmp |= (0x01 << 18);            // set I2S Master Mode
#else
    tmp &= ~(0x01 << 18);           // set I2S Slave Mode
#endif
    tmp |= (0x01 << 13);            // enable MIC boost
    tmp |= (0x01 << 14);            // IRSEL to Normal setting
    tmp &= ~(0x01 << 10);           // set not mute

    tmp &= ~(0x3F << 4);            // set ADC gain 0 dB, 0x3F is 36dB
    tmp |= (0x1B << 4);

    tmp &= ~(0x01 << 16);           // Line OUT mute control --> normal
    tmp &= ~(0x01 << 17);           // LOUT power-down control --> normal
    tmp &= ~(0x01 << 12);           // ADC power-down control --> normal
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x6C, tmp, mask) < 0) {
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x6c);
    }
}

/* audio dac da_i2s_sel. 0: from pad, 1: from ssp0/1 */
void adda160_I2S_mux(enum adda160_port_select ps, int mux)
{
    volatile u32 tmp = 0;
    volatile u32 mask = 0;

    tmp = ftpmu010_pcie_read_reg(0x6C);
    if(ps == ADDA160_DA){
        if(mux)
            tmp |= 0x01;
        else
            tmp &= ~0x01;
    }else {
        if(mux)
            tmp |= 0x02;
        else
            tmp &= ~0x02;
    }
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x6C, tmp, mask) < 0) {
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x78);
    }
    return;
}

void adda160_set_fs(const enum adda160_chip_select cs, const enum adda160_fs_frequency fs)
{
    volatile u32 tmp = 0;
    volatile u32 mask = 0;

    tmp = ftpmu010_pcie_read_reg(0x64);

    if (unlikely(cs != ADDA160_FIRST_CHIP)) {
        DEBUG_ADDA160_PRINT("%s fails: gm8126 should only accept ADDA160_FIRST_CHIP\n",
                            __FUNCTION__);
        return;
    }

    tmp &= ~(0x0F << 24);   // DA
    tmp &= ~(0x0F << 28);   // AD
    switch (fs) {
    case ADDA160_FS_8K:
    {
        tmp |= (0x04 << 24);
        tmp |= (0x04 << 28);
        break;
    }
    case ADDA160_FS_16K:
    {
        tmp |= (0x01 << 24);
        tmp |= (0x01 << 28);
        break;
    }
    case ADDA160_FS_32K:
    {
        //keep the four bits zero
        break;
    }
    case ADDA160_FS_44_1K:
    {
        tmp |= (0x0D << 24);
        tmp |= (0x0D << 28);
        break;
    }
    case ADDA160_FS_48K:
    {
#if SSP3_CLK_SRC_12000K     //ADDA160 MCLKMODE = MCLK(12000000) / SAMPLE RATE(48000) = 250
        tmp |= (0x06 << 24);
        tmp |= (0x06 << 28);
/* for HDMI playback sample rate 48k/16 bits , ssp3 bclk must = 3072000, sdl&pdl must = 16
   so ssp3's mclk must = 24.576Mhz, adda160's MCLKMODE must = 256 --> adda160's MCLK = MCLKMODE(256)* 48000 = 12.288MHz
*/
#else   // SSP3_CLK_SRC_24000K
        tmp |= (0x0f << 24);
        tmp |= (0x0f << 28);
#endif
        break;
    }
    default:
        DEBUG_ADDA160_PRINT("%s fails: fs wrong value\n", __FUNCTION__);
        return;
    }

    //set codec's MCLK mode setting
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x64, tmp, mask) < 0) {
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x78);
    }
    //unmute
    tmp = 0x0;
    mask = (1|(1<<1));
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x68, tmp, mask) < 0) {
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x78);
    }
#if DEBUG_ADDA160
    DEBUG_ADDA160_PRINT("In %s", __FUNCTION__);
    dump_adda160_reg();
#endif
}

void adda160_set_fs_ad_da(const enum adda160_port_select ps, const enum adda160_fs_frequency fs)
{
    volatile u32 tmp = 0;
    volatile u32 mask = 0;

    tmp = ftpmu010_pcie_read_reg(0x64);

    if(ps == ADDA160_DA)
        tmp &= ~(0xF << 24);   // DA
    else
        tmp &= ~(0xF << 28);   // AD

    switch (fs) {
    case ADDA160_FS_8K:
    {
        if(ps == ADDA160_DA)
            tmp |= (0x4 << 24);
        else
            tmp |= (0x4 << 28);
        break;
    }
    case ADDA160_FS_16K:
    {
        if(ps == ADDA160_DA)
            tmp |= (0x1 << 24);
        else
            tmp |= (0x1 << 28);
        break;
    }
    case ADDA160_FS_32K:
    {
        //keep the four bits zero
        break;
    }
    case ADDA160_FS_44_1K:
    {
        if(ps == ADDA160_DA)
            tmp |= (0xD << 24);
        else
            tmp |= (0xD << 28);
        break;
    }
    case ADDA160_FS_48K:
    {
        if(ps == ADDA160_DA)
#if SSP3_CLK_SRC_12000K     //ADDA160 MCLKMODE = MCLK(12000000) / SAMPLE RATE(48000) = 250
            tmp |= (0x6 << 24);
#else
            tmp |= (0xf << 24);
#endif
        else
#if SSP3_CLK_SRC_12000K     //ADDA160 MCLKMODE = MCLK(12000000) / SAMPLE RATE(48000) = 250
            tmp |= (0x6 << 28);
#else
            tmp |= (0xf << 28);
#endif
        break;
    }
    default:
        DEBUG_ADDA160_PRINT("%s fails: fs wrong value\n", __FUNCTION__);
        return;
    }

    //set codec's MCLK mode setting
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x64, tmp, mask) < 0) {
            panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x64);
    }

    if(ps == ADDA160_DA)
    {
        tmp = 0x0;
        mask = (1|(1<<1));
        if (ftpmu010_pcie_write_reg(pmu_fd, 0x68, tmp, mask) < 0) {
            panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x68);
        }
    }
#if DEBUG_ADDA160
    DEBUG_ADDA160_PRINT("In %s", __FUNCTION__);
    dump_adda160_reg();
#endif
    return;
}

void adda160_set_iv(void)
{
    volatile u32 tmp = 0;
    volatile u32 mask = 0;

    tmp = ftpmu010_pcie_read_reg(0x6c);

    tmp &= ~(0x3F << 4);

    tmp |= ((0x1B + iv) << 4);

    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x6c, tmp, mask) < 0)
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x6c);
}

void adda160_set_mute(void)
{
    volatile u32 tmp = 0;
    volatile u32 mask = 0;

    tmp = ftpmu010_pcie_read_reg(0x6c);

    if (mute == 1)
        tmp |= (0x1 << 16);
    if (mute == 0)
        tmp &= ~(0x1 << 16);

    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x6c, tmp, mask) < 0)
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x6c);
}

void adda160_set_vol(void)
{
    volatile u32 tmp = 0;
    volatile u32 mask = 0;
    u32 vol_reg_val = 0x3F;        //0x3F is the default 0 dB reg val

    if ((vol < -63) || (vol > 0)) {
        printk("%s: vol(%ddB) MUST between -63dB ~ 0dB, use default(-1dB)\n", __func__, vol);
        vol = -1;
    }

    vol_reg_val += vol;

    tmp = ftpmu010_pcie_read_reg(0x64);

    tmp &= ~(0x3F);
    tmp |= vol_reg_val;                   // set DAC digital volume control
    mask = 0xFFFFFFFF;
    if (ftpmu010_pcie_write_reg(pmu_fd, 0x64, tmp, mask) < 0)
        panic("In %s: setting offset(0x%x) failed.\n", __func__, 0x64);

}

static int proc_read_vol(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current vol = %d\n", vol);

    return 0;
}

static ssize_t proc_write_vol(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &vol);

    adda160_set_vol();

    return count;
}

static int proc_read_mute(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "current mute = %d\n", mute);

    return 0;
}

static ssize_t proc_write_mute(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &mute);

    adda160_set_mute();

    return count;
}

static int proc_read_iv(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "IV should be between -27~36dB\n");
    seq_printf(sfile, "current ADC gain IV = %d\n", iv);

    return 0;
}

static ssize_t proc_write_iv(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int len = count;
    unsigned char value[20] = {'\0'};

    if (copy_from_user(value, buffer, len)) {
        return 0;
    }

    value[len] = '\0';
    sscanf(value, "%d\n", &iv);

    adda160_set_iv();

    return count;
}

static int proc_open_vol(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_vol, PDE(inode)->data);
}

static int proc_open_mute(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_mute, PDE(inode)->data);
}

static int proc_open_iv(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read_iv, PDE(inode)->data);
}

static struct file_operations vol_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_vol,
    .write  = proc_write_vol,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations mute_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_mute,
    .write  = proc_write_mute,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations iv_proc_ops = {
    .owner  = THIS_MODULE,
    .open   = proc_open_iv,
    .write  = proc_write_iv,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

#define ADDA160_PROC_NAME "adda160"
int adda160_proc_init(void)
{
    int ret = -1;

    /* create proc */
    entity_proc = create_proc_entry(ADDA160_PROC_NAME, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (entity_proc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        ret = -EFAULT;
        goto err0;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    entity_proc->owner = THIS_MODULE;
#endif

    vol_d = create_proc_entry("volume", S_IRUGO | S_IXUGO, entity_proc);
    if (vol_d == NULL) {
        printk("%s fails: create volume proc not OK", __func__);
        ret = -EINVAL;
        goto err1;
    }
    vol_d->proc_fops  = &vol_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    vol_d->owner = THIS_MODULE;
#endif

    mute_d = create_proc_entry("mute", S_IRUGO | S_IXUGO, entity_proc);
    if (mute_d == NULL) {
        printk("%s fails: create mute proc not OK", __func__);
        ret = -EINVAL;
        goto err2;
    }
    mute_d->proc_fops  = &mute_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    mute_d->owner = THIS_MODULE;
#endif

    iv_d = create_proc_entry("iv", S_IRUGO | S_IXUGO, entity_proc);
    if (iv_d == NULL) {
        printk("%s fails: create iv proc not OK", __func__);
        ret = -EINVAL;
        goto err3;
    }
    iv_d->proc_fops  = &iv_proc_ops;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    iv_d->owner = THIS_MODULE;
#endif

    return 0;

err3:
    remove_proc_entry(iv_d->name, entity_proc);
err2:
    remove_proc_entry(mute_d->name, entity_proc);
err1:
    remove_proc_entry(vol_d->name, entity_proc);
err0:
    remove_proc_entry(entity_proc->name, entity_proc->parent);

    return ret;
}

void adda160_proc_remove(void)
{
    remove_proc_entry(iv_d->name, entity_proc);
    remove_proc_entry(mute_d->name, entity_proc);
    remove_proc_entry(vol_d->name, entity_proc);

    if (entity_proc)
        remove_proc_entry(entity_proc->name, entity_proc->parent);
}

static int __init adda160_drv_init(void)
{
    adda160_pmu_register();

    adda160_i2s_init();

    adda160_proc_init();

#if DEBUG_ADDA160
    DEBUG_ADDA160_PRINT("In %s", __FUNCTION__);
    dump_adda160_reg();
#endif

    return 0;
}

static void __exit adda160_drv_clearnup(void)
{
    adda160_proc_remove();

    if (pmu_fd)
        ftpmu010_pcie_deregister_reg(pmu_fd);
}

module_init(adda160_drv_init);
module_exit(adda160_drv_clearnup);
MODULE_LICENSE("GPL");
