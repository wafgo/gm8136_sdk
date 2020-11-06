/**
 * @file dh9901_drv.c
 * DaHua 4CH HDCVI Receiver Driver
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.4 $
 * $Date: 2014/10/22 08:45:31 $
 *
 * ChangeLog:
 *  $Log: dh9901_drv.c,v $
 *  Revision 1.4  2014/10/22 08:45:31  jerson_l
 *  1. modify DH9901_VOUT_FORMAT_BT656 to DH9901_VOUT_FORMAT_BT656_DUAL_HEADER
 *  2. add vin to vch api
 *
 *  Revision 1.3  2014/07/29 05:36:13  shawn_hu
 *  Add the module parameter of "ibus" to specify which I2C bus has the decoder attached.
 *
 *  Revision 1.2  2014/07/10 07:28:49  jerson_l
 *  1. support cpucomm for driver config sync in dual cpu platform
 *
 *  Revision 1.1.1.1  2014/07/03 01:57:57  jerson_l
 *  add hdcvi dh9901 driver
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#include "platform.h"
#include "plat_cpucomm.h"
#include "dh9901_lib.h"
#include "dahua/dh9901.h"              ///< from module/include/front_end/hdcvi

#define DH9901_CLKIN                   27000000
#define CH_IDENTIFY(id, cvi, vout)     (((id)&0xf)|(((cvi)&0xf)<<4)|(((vout)&0xf)<<8))
#define CH_IDENTIFY_ID(x)              ((x)&0xf)
#define CH_IDENTIFY_CVI(x)             (((x)>>4)&0xf)
#define CH_IDENTIFY_VOUT(x)            (((x)>>8)&0xf)

/* device number */
static int dev_num = 1;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1~4");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* device i2c address */
static ushort iaddr[DH9901_DEV_MAX] = {0x60, 0x62, 0x64, 0x66};
module_param_array(iaddr, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iaddr, "Device I2C Address");

/* clock port used */
static int clk_used = 0;
module_param(clk_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_used, "External Clock Port: BIT0:EXT_CLK0, BIT1:EXT_CLK1");

/* clock source */
static int clk_src = PLAT_EXTCLK_SRC_PLL3;
module_param(clk_src, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_src, "External Clock Source: 0:PLL1OUT1 1:PLL1OUT1/2 2:PLL4OUT2 3:PLL4OUT1/2 4:PLL3");

/* clock frequency */
static int clk_freq = DH9901_CLKIN;   ///< 27MHz
module_param(clk_freq, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(clk_freq, "External Clock Frequency(Hz)");

/* device init */
static int init = 1;
module_param(init, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init, "device init: 0:no-init  1:init");

/* device use gpio as rstb pin */
static int rstb_used = 1;
module_param(rstb_used, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rstb_used, "Use GPIO as RSTB Pin: 0:None, 1:X_CAP_RST");

/* device video port output format */
static int vout_format[DH9901_DEV_MAX] = {[0 ... (DH9901_DEV_MAX - 1)] = DH9901_VOUT_FORMAT_BT656_DUAL_HEADER};
module_param_array(vout_format, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(vout_format, "Video Output Format: 0:BT656_DUAL_HEADER, 1:BT1120");

/* device channel mapping */
static int ch_map = 0;
module_param(ch_map, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_map, "channel mapping");

/* audio sample rate */
static int sample_rate = DH9901_AUDIO_SAMPLERATE_8K;
module_param(sample_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sample_rate, "audio sample rate: 0:8k  1:16k");

/* audio sample size */
static int sample_size = DH9901_AUDIO_SAMPLESIZE_16BITS;
module_param(sample_size, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sample_size, "audio sample size: 0:8bit  1:16bit");

/* notify */
static int notify = 1;
module_param(notify, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(notify, "Device Notify => 0:Disable 1:Enable");

/******************************************************************************
 GM8287 EVB Channel and CVI Mapping Table (System Board)

 VCH#0 ------> CVI#0.0 ---------> VOUT#0.0 -------> X_CAP#0
 VCH#1 ------> CVI#0.1 ---------> VOUT#0.1 -------> X_CAP#1
 VCH#2 ------> CVI#0.2 ---------> VOUT#0.2 -------> X_CAP#2
 VCH#3 ------> CVI#0.3 ---------> VOUT#0.3 -------> X_CAP#3

 ------------------------------------------------------------------------------
 GM8287 EVB Channel and CVI Mapping Table (Socket Board)

 VCH#0 ------> CVI#0.0 ---------> VOUT#0.0 -------> X_CAP#1
 VCH#1 ------> CVI#0.1 ---------> VOUT#0.1 -------> X_CAP#0
 VCH#2 ------> CVI#0.2 ---------> VOUT#0.2 -------> X_CAP#3
 VCH#3 ------> CVI#0.3 ---------> VOUT#0.3 -------> X_CAP#2

*******************************************************************************/

struct dh9901_dev_t {
    int                     index;                          ///< device index
    char                    name[16];                       ///< device name
    struct semaphore        lock;                           ///< device locker
    struct miscdevice       miscdev;                        ///< device node for ioctl access

    int                     pre_plugin[DH9901_DEV_CH_MAX];  ///< device channel previous cable plugin status
    int                     cur_plugin[DH9901_DEV_CH_MAX];  ///< device channel current  cable plugin status

    struct dh9901_ptz_t     ptz[DH9901_DEV_CH_MAX];         ///< device channel PTZ config

    int                     (*vfmt_notify)(int id, int ch, struct dh9901_video_fmt_t *vfmt);
    int                     (*vlos_notify)(int id, int ch, int vlos);
};

static struct dh9901_dev_t    dh9901_dev[DH9901_DEV_MAX];
static struct proc_dir_entry *dh9901_proc_root[DH9901_DEV_MAX]        = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_status[DH9901_DEV_MAX]      = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_video_fmt[DH9901_DEV_MAX]   = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_video_color[DH9901_DEV_MAX] = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_video_pos[DH9901_DEV_MAX]   = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_cable_type[DH9901_DEV_MAX]  = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_clear_eq[DH9901_DEV_MAX]    = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_audio_vol[DH9901_DEV_MAX]   = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_ptz_root[DH9901_DEV_MAX]    = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_ptz_cfg[DH9901_DEV_MAX]     = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};
static struct proc_dir_entry *dh9901_proc_ptz_ctrl[DH9901_DEV_MAX]    = {[0 ... (DH9901_DEV_MAX - 1)] = NULL};

static audio_funs_t        dh9901_audio_funs;
static int                 dh9901_api_inited  = 0;
static struct i2c_client  *dh9901_i2c_client  = NULL;
static struct task_struct *dh9901_wd          = NULL;
static struct task_struct *dh9901_comm        = NULL;
static DEFINE_SEMAPHORE(dh9901_comm_lock);
static u32 DH9901_VCH_MAP[DH9901_DEV_MAX*DH9901_DEV_CH_MAX];

static struct dh9901_video_fmt_t dh9901_video_fmt_info[DH9901_VFMT_MAX] = {
    /* Idx                    Width   Height  Prog    FPS */
    {  DH9901_VFMT_720P25,    1280,   720,    1,      25  },  ///< 720P @25
    {  DH9901_VFMT_720P30,    1280,   720,    1,      30  },  ///< 720P @30
    {  DH9901_VFMT_720P50,    1280,   720,    1,      50  },  ///< 720P @50
    {  DH9901_VFMT_720P60,    1280,   720,    1,      60  },  ///< 720P @60
    {  DH9901_VFMT_1080P25,   1920,   1080,   1,      25  },  ///< 1080P@25
    {  DH9901_VFMT_1080P30,   1920,   1080,   1,      30  },  ///< 1080P@30
};

static int dh9901_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    dh9901_i2c_client = client;
    return 0;
}

static int dh9901_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id dh9901_i2c_id[] = {
    { "dh9901_i2c", 0 },
    { }
};

static struct i2c_driver dh9901_i2c_driver = {
    .driver = {
        .name  = "DH9901_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = dh9901_i2c_probe,
    .remove   = dh9901_i2c_remove,
    .id_table = dh9901_i2c_id
};

static int dh9901_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&dh9901_i2c_driver);
    if(ret < 0) {
        printk("DH9901 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = iaddr[0]>>1;
    strlcpy(info.type, "dh9901_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("DH9901 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("DH9901 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&dh9901_i2c_driver);
   return -ENODEV;
}

static void dh9901_i2c_unregister(void)
{
    i2c_unregister_device(dh9901_i2c_client);
    i2c_del_driver(&dh9901_i2c_driver);
    dh9901_i2c_client = NULL;
}

static int dh9901_create_ch_map(int map_id)
{
    int i;

    switch(map_id) {
        case 0: ///< for GM8210/GM8287 Socket Board
        default:
            for(i=0; i<dev_num; i++) {
                DH9901_VCH_MAP[(i*DH9901_DEV_CH_MAX)+0] = CH_IDENTIFY(i, 0, DH9901_DEV_VOUT_0);
                DH9901_VCH_MAP[(i*DH9901_DEV_CH_MAX)+1] = CH_IDENTIFY(i, 1, DH9901_DEV_VOUT_1);
                DH9901_VCH_MAP[(i*DH9901_DEV_CH_MAX)+2] = CH_IDENTIFY(i, 2, DH9901_DEV_VOUT_2);
                DH9901_VCH_MAP[(i*DH9901_DEV_CH_MAX)+3] = CH_IDENTIFY(i, 3, DH9901_DEV_VOUT_3);
            }
            break;
    }

    return 0;
}

int dh9901_get_vch_id(int id, DH9901_DEV_VOUT_T vout, int cvi_ch)
{
    int i;

    if(id >= dev_num || vout >= DH9901_DEV_VOUT_MAX || cvi_ch >= DH9901_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*DH9901_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(DH9901_VCH_MAP[i]) == id) &&
           (CH_IDENTIFY_VOUT(DH9901_VCH_MAP[i]) == vout) &&
           (CH_IDENTIFY_CVI(DH9901_VCH_MAP[i]) == cvi_ch)) {
            return i;
        }
    }

    return 0;
}
EXPORT_SYMBOL(dh9901_get_vch_id);

int dh9901_vin_to_vch(int id, int vin_idx)
{
    int i;

    if(id >= dev_num || vin_idx >= DH9901_DEV_CH_MAX)
        return 0;

    for(i=0; i<(dev_num*DH9901_DEV_CH_MAX); i++) {
        if((CH_IDENTIFY_ID(DH9901_VCH_MAP[i]) == id) && (CH_IDENTIFY_CVI(DH9901_VCH_MAP[i]) == vin_idx))
            return i;
    }

    return 0;
}
EXPORT_SYMBOL(dh9901_vin_to_vch);

int dh9901_get_vout_format(int id)
{
    if(id >= dev_num)
        return DH9901_VOUT_FORMAT_BT656_DUAL_HEADER;
    else
        return vout_format[id];
}
EXPORT_SYMBOL(dh9901_get_vout_format);

int dh9901_get_device_num(void)
{
    return dev_num;
}
EXPORT_SYMBOL(dh9901_get_device_num);

int dh9901_notify_vfmt_register(int id, int (*nt_func)(int, int, struct dh9901_video_fmt_t *))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&dh9901_dev[id].lock);

    dh9901_dev[id].vfmt_notify = nt_func;

    up(&dh9901_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(dh9901_notify_vfmt_register);

void dh9901_notify_vfmt_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&dh9901_dev[id].lock);

    dh9901_dev[id].vfmt_notify = NULL;

    up(&dh9901_dev[id].lock);
}
EXPORT_SYMBOL(dh9901_notify_vfmt_deregister);

int dh9901_notify_vlos_register(int id, int (*nt_func)(int, int, int))
{
    if(id >= dev_num || !nt_func)
        return -1;

    down(&dh9901_dev[id].lock);

    dh9901_dev[id].vlos_notify = nt_func;

    up(&dh9901_dev[id].lock);

    return 0;
}
EXPORT_SYMBOL(dh9901_notify_vlos_register);

void dh9901_notify_vlos_deregister(int id)
{
    if(id >= dev_num)
        return;

    down(&dh9901_dev[id].lock);

    dh9901_dev[id].vlos_notify = NULL;

    up(&dh9901_dev[id].lock);
}
EXPORT_SYMBOL(dh9901_notify_vlos_deregister);

static int dh9901_writebyte(u8 addr, u8 reg, u8 data)
{
    u8 buf[2];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if(!dh9901_i2c_client) {
        printk("DH9901 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(dh9901_i2c_client->dev.parent);

    buf[0]     = reg;
    buf[1]     = data;
    msgs.addr  = addr>>1;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("DH9901(0x%02x) i2c write failed!!\n", addr);
        return -1;
    }

    return 0;
}

static u8 dh9901_readbyte(u8 addr, u8 reg)
{
    u8 reg_data = 0;
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(!dh9901_i2c_client) {
        printk("DH9901 i2c_client not register!!\n");
        return 0;
    }

    adapter = to_i2c_adapter(dh9901_i2c_client->dev.parent);

    msgs[0].addr  = addr>>1;
    msgs[0].flags = 0; /* write */
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = (addr + 1)>>1;
    msgs[1].flags = 1; /* read */
    msgs[1].len   = 1;
    msgs[1].buf   = &reg_data;

    if(i2c_transfer(adapter, msgs, 2) != 2)
        printk("DH9901(0x%02x) i2c read failed!!\n", addr);

    return reg_data;
}

/********************************************
  DH9901 COMM Message Buffer
  |-----------------| -
  |                 | |
  |     Header      | |
  |                 | |
  |-----------------| DH9901_COMM_MSG_BUF_MAX
  |                 | |
  |     Data        | |
  |                 | |
  |-----------------| -

 *********************************************/
static int dh9901_comm_cmd_send(int id, int ch, int cmd_id, void *cmd_data, u32 data_len, int wait_ack, int timeout)
{
    int ret = -1;
    struct plat_cpucomm_msg  tx_msg, rx_msg;
    struct dh9901_comm_header_t *p_header;
    unsigned char *p_data;
    unsigned char *msg_buf = NULL;

    if((id >= dev_num) || (ch >= DH9901_DEV_CH_MAX))
        return -1;

    if((data_len + sizeof(struct dh9901_comm_header_t)) > DH9901_COMM_MSG_BUF_MAX)
        return -1;

    down(&dh9901_comm_lock);

    /* allocate dh9901 comm message buffer */
    msg_buf = kzalloc(DH9901_COMM_MSG_BUF_MAX, GFP_KERNEL);
    if(!msg_buf)
        goto exit;

    p_header = (struct dh9901_comm_header_t *)msg_buf;
    p_data   = ((unsigned char *)p_header) + sizeof(struct dh9901_comm_header_t);

    /* process tx command */
    switch(cmd_id) {
        case DH9901_COMM_CMD_ACK_OK:
        case DH9901_COMM_CMD_ACK_FAIL:
        case DH9901_COMM_CMD_CLEAR_EQ:
            {
                p_header->magic    = DH9901_COMM_MSG_MAGIC;
                p_header->cmd_id   = cmd_id;
                p_header->data_len = 0;
                p_header->dev_id   = id;
                p_header->dev_ch   = ch;
                if(cmd_id <= DH9901_COMM_CMD_ACK_FAIL)
                    wait_ack = 0;
            }
            break;
        case DH9901_COMM_CMD_GET_VIDEO_STATUS:
        case DH9901_COMM_CMD_SET_COLOR:
        case DH9901_COMM_CMD_GET_COLOR:
        case DH9901_COMM_CMD_SEND_RS485:
        case DH9901_COMM_CMD_SET_VIDEO_POS:
        case DH9901_COMM_CMD_GET_VIDEO_POS:
        case DH9901_COMM_CMD_SET_AUDIO_VOL:
        case DH9901_COMM_CMD_SET_CABLE_TYPE:
        case DH9901_COMM_CMD_GET_PTZ_CFG:
            if(cmd_data) {
                p_header->magic    = DH9901_COMM_MSG_MAGIC;
                p_header->cmd_id   = cmd_id;
                p_header->data_len = data_len;
                p_header->dev_id   = id;
                p_header->dev_ch   = ch;
                memcpy(p_data, cmd_data, data_len);
            }
            else {
                ret = -1;
                goto exit;
            }
            break;
        default:
            ret = -1;
            goto exit;
    }

    /* prepare tx data */
    tx_msg.length   = sizeof(struct dh9901_comm_header_t) + p_header->data_len;
    tx_msg.msg_data = msg_buf;

    /* trigger data transmit */
    ret = plat_cpucomm_tx(&tx_msg, timeout);
    if(ret != 0)
        goto exit;

    /* wait to receive data or ack */
    if(wait_ack) {
        rx_msg.length   = DH9901_COMM_MSG_BUF_MAX;
        rx_msg.msg_data = msg_buf;
        ret = plat_cpucomm_rx(&rx_msg, timeout);
        if((ret != 0) || (rx_msg.length < sizeof(struct dh9901_comm_header_t)))
            goto exit;

        p_header = (struct dh9901_comm_header_t *)rx_msg.msg_data;
        p_data   = ((unsigned char *)p_header) + sizeof(struct dh9901_comm_header_t);

        if((p_header->magic == DH9901_COMM_MSG_MAGIC)) {
            switch(cmd_id) {
                /* ack with command data */
                case DH9901_COMM_CMD_GET_VIDEO_STATUS:
                case DH9901_COMM_CMD_GET_COLOR:
                case DH9901_COMM_CMD_GET_VIDEO_POS:
                case DH9901_COMM_CMD_GET_PTZ_CFG:
                    if((p_header->cmd_id != cmd_id)     ||
                       (p_header->data_len != data_len) ||
                       (p_header->dev_id != id)         ||
                       (p_header->dev_ch != ch)) {
                        ret = -1;
                        goto exit;
                    }
                    memcpy(cmd_data, p_data, p_header->data_len);
                    break;

                /* only ack */
                case DH9901_COMM_CMD_CLEAR_EQ:
                case DH9901_COMM_CMD_SET_COLOR:
                case DH9901_COMM_CMD_SEND_RS485:
                case DH9901_COMM_CMD_SET_VIDEO_POS:
                case DH9901_COMM_CMD_SET_AUDIO_VOL:
                case DH9901_COMM_CMD_SET_CABLE_TYPE:
                    if((p_header->cmd_id != DH9901_COMM_CMD_ACK_OK) ||
                       (p_header->dev_id != id)                     ||
                       (p_header->dev_ch != ch)) {
                        ret = -1;
                        goto exit;
                    }
                    break;
                default:
                    ret = -1;
                    goto exit;
            }
        }
        else {
            ret = -1;
            goto exit;
        }
    }

exit:
    if(msg_buf)
        kfree(msg_buf);

    up(&dh9901_comm_lock);

    return ret;
}

static int dh9901_proc_status_show(struct seq_file *sfile, void *v)
{
    int i, j, ret;
    int vlos;
    DH_VIDEO_STATUS_S cvi_sts;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    seq_printf(sfile, "[DH9901#%d]\n", pdev->index);
    seq_printf(sfile, "------------------\n");
    seq_printf(sfile, "CVI    VCH    LOS \n");
    seq_printf(sfile, "==================\n");
    for(i=0; i<DH9901_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*DH9901_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(DH9901_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_CVI(DH9901_VCH_MAP[j]) == i)) {
                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_GetVideoStatus(pdev->index, i, &cvi_sts);

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, i, DH9901_COMM_CMD_GET_VIDEO_STATUS, &cvi_sts, sizeof(cvi_sts), 1, 1000);
                }
                vlos = (ret == 0) ? cvi_sts.ucVideoLost : 0;
                seq_printf(sfile, "%-6d %-6d %-7s\n", i, j, ((vlos == 0) ? "NO" : "YES"));
            }
        }
    }



    return 0;
}

static int dh9901_proc_video_fmt_show(struct seq_file *sfile, void *v)
{
    int i, j, ret;
    DH_VIDEO_STATUS_S cvi_sts;
    struct dh9901_video_fmt_t *p_vfmt;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    seq_printf(sfile, "[DH9901#%d] Incoming Video Fromat\n", pdev->index);
    seq_printf(sfile, "====================================================\n");
    seq_printf(sfile, "CVI   VCH   Width   Height   Prog/Inter   Frame_Rate\n");
    seq_printf(sfile, "====================================================\n");

    for(i=0; i<DH9901_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*DH9901_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(DH9901_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_CVI(DH9901_VCH_MAP[j]) == i)) {
                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_GetVideoStatus(pdev->index, i, &cvi_sts);

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, i, DH9901_COMM_CMD_GET_VIDEO_STATUS, &cvi_sts, sizeof(cvi_sts), 1, 1000);
                }

                if((ret == 0) && ((cvi_sts.ucVideoFormat >= DH9901_VFMT_720P25) && (cvi_sts.ucVideoFormat < DH9901_VFMT_MAX))) {
                    p_vfmt = &dh9901_video_fmt_info[cvi_sts.ucVideoFormat];
                    seq_printf(sfile, "%-5d %-5d %-7d %-8d %-12s %-11d\n",
                               i,
                               j,
                               p_vfmt->width,
                               p_vfmt->height,
                               ((p_vfmt->prog == 1) ? "Progressive" : "Interlace"),
                               p_vfmt->frame_rate);
                }
                else {
                    seq_printf(sfile, "%-5d %-5d %-7s %-8s %-12s %-11s\n", i, j, "-", "-", "-", "-");
                }
            }
        }
    }

    return 0;
}

static int dh9901_proc_video_color_show(struct seq_file *sfile, void *v)
{
    int i, j, ret;
    DH_VIDEO_COLOR_S v_color;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    seq_printf(sfile, "[DH9901#%d] Video Color\n", pdev->index);
    seq_printf(sfile, "=======================================================\n");
    seq_printf(sfile, "CVI   VCH   BRI   CON   SAT   HUE   GAIN   W/B   SHARP \n");
    seq_printf(sfile, "=======================================================\n");

    for(i=0; i<DH9901_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*DH9901_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(DH9901_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_CVI(DH9901_VCH_MAP[j]) == i)) {
                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_GetColor(pdev->index, i, &v_color, DH_SET_MODE_USER);

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, i, DH9901_COMM_CMD_GET_COLOR, &v_color, sizeof(v_color), 1, 1000);
                }

                if(ret == 0) {
                    seq_printf(sfile, "%-5d %-5d %-5d %-5d %-5d %-5d %-6d %-5d %-5d\n",
                               i,
                               j,
                               v_color.ucBrightness,
                               v_color.ucContrast,
                               v_color.ucSaturation,
                               v_color.ucHue,
                               v_color.ucGain,
                               v_color.ucWhiteBalance,
                               v_color.ucSharpness);
                }
                else {
                    seq_printf(sfile, "%-5d %-5d %-5d %-5d %-5d %-5d %-6d %-5d %-5d\n", i, j, 0, 0, 0, 0, 0, 0, 0);
                }
            }
        }
    }
    seq_printf(sfile, "=======================================================\n");
    seq_printf(sfile, "[0]Brightness  : 0 ~ 100\n");
    seq_printf(sfile, "[1]Contrast    : 0 ~ 100\n");
    seq_printf(sfile, "[2]Saturation  : 0 ~ 100\n");
    seq_printf(sfile, "[3]Hue         : 0 ~ 100\n");
    seq_printf(sfile, "[4]Gain        : Not support\n");
    seq_printf(sfile, "[5]WhiteBalance: Not support\n");
    seq_printf(sfile, "[6]Sharpness   : 0 ~ 15\n");
    seq_printf(sfile, "=======================================================\n");
    seq_printf(sfile, "echo [CVI#] [PARAM#] [VALUE] for parameter setup\n\n");

    return 0;
}

static int dh9901_proc_video_pos_show(struct seq_file *sfile, void *v)
{
    int i, j, ret;
    DH_VIDEO_POSITION_S vpos;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    seq_printf(sfile, "[DH9901#%d] Video Position\n", pdev->index);
    seq_printf(sfile, "----------------------------------\n");
    seq_printf(sfile, "CVI    VCH    H_Offset    V_Offset\n");
    seq_printf(sfile, "==================================\n");
    for(i=0; i<DH9901_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*DH9901_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(DH9901_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_CVI(DH9901_VCH_MAP[j]) == i)) {
                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_GetVideoPosition(pdev->index, i, &vpos);

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, i, DH9901_COMM_CMD_GET_VIDEO_POS, &vpos, sizeof(vpos), 1, 1000);
                }

                if(ret == 0)
                    seq_printf(sfile, "%-6d %-6d %-11d %-11d\n", i, j, vpos.sHorizonOffset, vpos.sVerticalOffset);
                else
                    seq_printf(sfile, "%-6d %-6d %-11d %-11d\n", i, j, 0, 0);
            }
        }
    }
    seq_printf(sfile, "==================================\n");
    seq_printf(sfile, "echo [CVI#] [H_Offset] [V_Offset] for video position setup\n\n");

    return 0;
}

static int dh9901_proc_cable_type_show(struct seq_file *sfile, void *v)
{
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901#%d] Cable Type\n", pdev->index);
    seq_printf(sfile, "========================================\n");
    seq_printf(sfile, "[0]COAXIAL\n");
    seq_printf(sfile, "[1]UTP_10OHM\n");
    seq_printf(sfile, "[2]UTP_17OHM\n");
    seq_printf(sfile, "[3]UTP_25OHM\n");
    seq_printf(sfile, "[4]UTP_35OHM\n");
    seq_printf(sfile, "========================================\n");
    seq_printf(sfile, "echo [CVI#] [TYPE#] for cable type setup\n\n");

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_clear_eq_show(struct seq_file *sfile, void *v)
{
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901#%d] Clear EQ\n", pdev->index);
    seq_printf(sfile, "========================================\n");
    seq_printf(sfile, "echo [CVI#] for trigger channel EQ clear\n\n");

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_audio_vol_show(struct seq_file *sfile, void *v)
{
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901#%d] Audio Volume\n", pdev->index);
    seq_printf(sfile, "========================================\n");
    seq_printf(sfile, "echo [CVI#] [Volume(0~100)] for setup audio volume\n\n");

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_ptz_cfg_show(struct seq_file *sfile, void *v)
{
    int i, j, ret = 0;
    struct dh9901_ptz_t ptz_cfg;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;
    char *ptz_prot_str[] = {"DH_SD1"};
    char *ptz_baud_str[] = {"1200", "2400", "4800", "9600", "19200", "38400"};

    seq_printf(sfile, "[DH9901#%d] PTZ Configuration\n", pdev->index);
    seq_printf(sfile, "---------------------------------------------\n");
    seq_printf(sfile, "CVI   VCH   PROTOCOL   BAUD_RATE   PARITY_CHK\n");
    seq_printf(sfile, "=============================================\n");
    for(i=0; i<DH9901_DEV_CH_MAX; i++) {
        for(j=0; j<(dev_num*DH9901_DEV_CH_MAX); j++) {
            if((CH_IDENTIFY_ID(DH9901_VCH_MAP[j]) == pdev->index) && (CH_IDENTIFY_CVI(DH9901_VCH_MAP[j]) == i)) {
                if(init) {
                    down(&pdev->lock);

                    memcpy(&ptz_cfg, &pdev->ptz[i], sizeof(struct dh9901_ptz_t));

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, i, DH9901_COMM_CMD_GET_PTZ_CFG, &ptz_cfg, sizeof(struct dh9901_ptz_t), 1, 1000);
                }
                if(ret == 0) {
                    seq_printf(sfile, "%-5d %-5d %-10s %-11s %-10s\n", i, j,
                               (ptz_cfg.protocol < DH9901_PTZ_PTOTOCOL_MAX) ? ptz_prot_str[ptz_cfg.protocol] : "Unknown",
                               (ptz_cfg.baud_rate < DH9901_PTZ_BAUD_MAX) ? ptz_baud_str[ptz_cfg.baud_rate] : "Unknown",
                               (ptz_cfg.parity_chk == 0) ? "No" : "Yes");
                }
                else {
                    seq_printf(sfile, "%-5d %-5d %-10s %-11s %-10s\n", i, j, "Unknown", "Unknown", "No");
                }
            }
        }
    }

    return 0;
}

static int dh9901_proc_ptz_ctrl_show(struct seq_file *sfile, void *v)
{
    int i;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    down(&pdev->lock);

    seq_printf(sfile, "[DH9901#%d]\n", pdev->index);

    for(i=0; i<DH9901_PTZ_PTOTOCOL_MAX; i++) {
        switch(i) {
            case DH9901_PTZ_PTOTOCOL_DHSD1:
                seq_printf(sfile, "<< DH_SD1 Command List >>\n");
                seq_printf(sfile, "=========================\n");
                seq_printf(sfile, "[0]Menu_CLOSE\n");
                seq_printf(sfile, "[1]Menu_OPEN\n");
                seq_printf(sfile, "[2]Menu_BACK\n");
                seq_printf(sfile, "[2]Menu_NEXT\n");
                seq_printf(sfile, "[4]Menu_UP\n");
                seq_printf(sfile, "[5]Menu_DOWN\n");
                seq_printf(sfile, "[6]Menu_LEFT\n");
                seq_printf(sfile, "[7]Menu_RIGHT\n");
                seq_printf(sfile, "[8]Menu_ENTER\n");
                seq_printf(sfile, "=========================\n");
                seq_printf(sfile, "echo [CVI#] [CAMERA#(0~255)] [CMD#] for remoet camrea control\n\n");
                break;
            default:
                break;
        }
    }

    up(&pdev->lock);

    return 0;
}

static int dh9901_proc_status_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_status_show, PDE(inode)->data);
}

static int dh9901_proc_video_fmt_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_video_fmt_show, PDE(inode)->data);
}

static int dh9901_proc_video_color_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_video_color_show, PDE(inode)->data);
}

static int dh9901_proc_video_pos_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_video_pos_show, PDE(inode)->data);
}

static int dh9901_proc_cable_type_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_cable_type_show, PDE(inode)->data);
}

static int dh9901_proc_clear_eq_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_clear_eq_show, PDE(inode)->data);
}

static int dh9901_proc_audio_vol_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_audio_vol_show, PDE(inode)->data);
}

static int dh9901_proc_ptz_cfg_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_ptz_cfg_show, PDE(inode)->data);
}

static int dh9901_proc_ptz_ctrl_open(struct inode *inode, struct file *file)
{
    return single_open(file, dh9901_proc_ptz_ctrl_show, PDE(inode)->data);
}

static ssize_t dh9901_proc_video_color_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int ret, cvi_ch, param_id, param_value;
    DH_VIDEO_COLOR_S video_color;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %d\n", &cvi_ch, &param_id, &param_value);

    if((cvi_ch >= DH9901_DEV_CH_MAX) || (param_id >= 7))
        goto exit;

    if(init) {
        down(&pdev->lock);

        ret = DH9901_API_GetColor(pdev->index, cvi_ch, &video_color, DH_SET_MODE_USER);

        up(&pdev->lock);
    }
    else {
        ret = dh9901_comm_cmd_send(pdev->index, cvi_ch, DH9901_COMM_CMD_GET_COLOR, &video_color, sizeof(video_color), 1, 1000);
    }
    if(ret != 0)
        goto exit;

    switch(param_id) {
        case 0:
            video_color.ucBrightness = (DH_U8)param_value;
            break;
        case 1:
            video_color.ucContrast = (DH_U8)param_value;
            break;
        case 2:
            video_color.ucSaturation = (DH_U8)param_value;
            break;
        case 3:
            video_color.ucHue = (DH_U8)param_value;
            break;
        case 4:
            video_color.ucGain = (DH_U8)param_value;
            break;
        case 5:
            video_color.ucWhiteBalance = (DH_U8)param_value;
            break;
        case 6:
            video_color.ucSharpness = (DH_U8)param_value;
            break;
        default:
            break;
    }

    if(init) {
        down(&pdev->lock);

        DH9901_API_SetColor(pdev->index, cvi_ch, &video_color, DH_SET_MODE_USER);

        up(&pdev->lock);
    }
    else {
        dh9901_comm_cmd_send(pdev->index, cvi_ch, DH9901_COMM_CMD_SET_COLOR, &video_color, sizeof(video_color), 1, 1000);
    }

exit:
    return count;
}

static ssize_t dh9901_proc_video_pos_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int cvi_ch, h_offset, v_offset;
    DH_VIDEO_POSITION_S video_pos;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %d\n", &cvi_ch, &h_offset, &v_offset);

    if(cvi_ch >= DH9901_DEV_CH_MAX)
        goto exit;

    video_pos.sHorizonOffset  = h_offset;
    video_pos.sVerticalOffset = v_offset;

    if(init) {
        down(&pdev->lock);

        DH9901_API_SetVideoPosition(pdev->index, cvi_ch, &video_pos);

        up(&pdev->lock);
    }
    else {
        dh9901_comm_cmd_send(pdev->index, cvi_ch, DH9901_COMM_CMD_SET_VIDEO_POS, &video_pos, sizeof(video_pos), 1, 1000);
    }

exit:
    return count;
}

static ssize_t dh9901_proc_cable_type_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int cvi_ch, cable_type;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &cvi_ch, &cable_type);

    if(cvi_ch >= DH9901_DEV_CH_MAX)
        goto exit;

    if(init) {
        down(&pdev->lock);

        DH9901_API_SetCableType(pdev->index, cvi_ch, cable_type);

        up(&pdev->lock);
    }
    else {
        dh9901_comm_cmd_send(pdev->index, cvi_ch, DH9901_COMM_CMD_SET_CABLE_TYPE, &cable_type, sizeof(DH9901_CABLE_TYPE_T), 1, 1000);
    }

exit:
    return count;
}

static ssize_t dh9901_proc_clear_eq_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int cvi_ch;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d\n", &cvi_ch);

    if(cvi_ch >= DH9901_DEV_CH_MAX)
        goto exit;

    if(init) {
        down(&pdev->lock);

        DH9901_API_ClearEq(pdev->index, cvi_ch);

        up(&pdev->lock);
    }
    else {
        dh9901_comm_cmd_send(pdev->index, cvi_ch, DH9901_COMM_CMD_CLEAR_EQ, NULL, 0, 1, 2000);
    }

exit:
    return count;
}

static ssize_t dh9901_proc_audio_vol_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int cvi_ch, volume;
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d\n", &cvi_ch, &volume);

    if(cvi_ch >= DH9901_DEV_CH_MAX)
        goto exit;

    if(init) {
        down(&pdev->lock);

        DH9901_API_SetAudioInVolume(pdev->index, cvi_ch, (DH_U8)volume);

        up(&pdev->lock);
    }
    else {
        dh9901_comm_cmd_send(pdev->index, cvi_ch, DH9901_COMM_CMD_SET_AUDIO_VOL, (DH_U8 *)&volume, sizeof(DH_U8), 1, 1000);
    }

exit:
    return count;
}

static ssize_t dh9901_proc_ptz_ctrl_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int cvi_ch, cam_id, cmd, ret, protocol;
    char rs485_cmd[64];
    char value_str[32] = {'\0'};
    struct seq_file *sfile = (struct seq_file *)file->private_data;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)sfile->private;

    if(copy_from_user(value_str, buffer, count))
        return -EFAULT;

    value_str[count] = '\0';
    sscanf(value_str, "%d %d %d\n", &cvi_ch, &cam_id, &cmd);

    if(cvi_ch >= DH9901_DEV_CH_MAX)
        goto exit;

    /* get ptz protocol */
    if(init) {
        protocol = pdev->ptz[cvi_ch].protocol;
    }
    else {
        struct dh9901_ptz_t ptz_cfg;

        ret = dh9901_comm_cmd_send(pdev->index, cvi_ch, DH9901_COMM_CMD_GET_PTZ_CFG, &ptz_cfg, sizeof(ptz_cfg), 1, 1000);
        if(ret != 0)
            goto exit;

        protocol = ptz_cfg.protocol;
    }

    switch(protocol) {
        case DH9901_PTZ_PTOTOCOL_DHSD1:
            /*
             * Prepare DH_SD1 PTZ RS485 Command
             * [SYNC CAM_ID CMD1 DATA1 DATA2 DATA3 CRC]
             */
            rs485_cmd[0] = 0xa5;
            rs485_cmd[1] = (u8)cam_id;
            rs485_cmd[2] = 0x89;
            rs485_cmd[3] = (u8)cmd;
            rs485_cmd[4] = 0x00;
            rs485_cmd[5] = 0x00;
            rs485_cmd[6] = (rs485_cmd[0] + rs485_cmd[1] + rs485_cmd[2] + rs485_cmd[3] + rs485_cmd[4] + rs485_cmd[5])%0x100;

            if(init) {
                down(&pdev->lock);

                ret = DH9901_API_Contrl485Enable(pdev->index, cvi_ch, 1);

                if(ret == 0)
                    ret = DH9901_API_Send485Buffer(pdev->index, cvi_ch, rs485_cmd, 7);

                DH9901_API_Contrl485Enable(pdev->index, cvi_ch, 0);

                up(&pdev->lock);
            }
            else {
                struct dh9901_comm_send_rs485_t comm_rs485;

                comm_rs485.buf_len = 7;
                memcpy(comm_rs485.cmd_buf, rs485_cmd, comm_rs485.buf_len);

                dh9901_comm_cmd_send(pdev->index, cvi_ch, DH9901_COMM_CMD_SEND_RS485, &comm_rs485, sizeof(comm_rs485), 1, 1000);
            }
            break;
        default:
            break;
    }

exit:
    return count;
}

static struct file_operations dh9901_proc_status_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_status_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_video_fmt_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_video_fmt_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_video_color_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_video_color_open,
    .write  = dh9901_proc_video_color_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_video_pos_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_video_pos_open,
    .write  = dh9901_proc_video_pos_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_cable_type_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_cable_type_open,
    .write  = dh9901_proc_cable_type_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_clear_eq_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_clear_eq_open,
    .write  = dh9901_proc_clear_eq_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_audio_vol_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_audio_vol_open,
    .write  = dh9901_proc_audio_vol_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_ptz_cfg_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_ptz_cfg_open,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static struct file_operations dh9901_proc_ptz_ctrl_ops = {
    .owner  = THIS_MODULE,
    .open   = dh9901_proc_ptz_ctrl_open,
    .write  = dh9901_proc_ptz_ctrl_write,
    .read   = seq_read,
    .llseek = seq_lseek,
    .release= single_release,
};

static void dh9901_proc_remove(int id)
{
    if(id >= DH9901_DEV_MAX)
        return;

    if(dh9901_proc_root[id]) {
        if(dh9901_proc_ptz_root[id]) {
            if(dh9901_proc_ptz_cfg[id]) {
                remove_proc_entry(dh9901_proc_ptz_cfg[id]->name, dh9901_proc_ptz_root[id]);
                dh9901_proc_ptz_cfg[id] = NULL;
            }

            if(dh9901_proc_ptz_ctrl[id]) {
                remove_proc_entry(dh9901_proc_ptz_ctrl[id]->name, dh9901_proc_ptz_root[id]);
                dh9901_proc_ptz_ctrl[id] = NULL;
            }

            remove_proc_entry(dh9901_proc_ptz_root[id]->name, dh9901_proc_root[id]);
            dh9901_proc_ptz_root[id] = NULL;
        }

        if(dh9901_proc_status[id]) {
            remove_proc_entry(dh9901_proc_status[id]->name, dh9901_proc_root[id]);
            dh9901_proc_status[id] = NULL;
        }

        if(dh9901_proc_video_fmt[id]) {
            remove_proc_entry(dh9901_proc_video_fmt[id]->name, dh9901_proc_root[id]);
            dh9901_proc_video_fmt[id] = NULL;
        }

        if(dh9901_proc_video_color[id]) {
            remove_proc_entry(dh9901_proc_video_color[id]->name, dh9901_proc_root[id]);
            dh9901_proc_video_color[id] = NULL;
        }

        if(dh9901_proc_video_pos[id]) {
            remove_proc_entry(dh9901_proc_video_pos[id]->name, dh9901_proc_root[id]);
            dh9901_proc_video_pos[id] = NULL;
        }

        if(dh9901_proc_cable_type[id]) {
            remove_proc_entry(dh9901_proc_cable_type[id]->name, dh9901_proc_root[id]);
            dh9901_proc_cable_type[id] = NULL;
        }

        if(dh9901_proc_clear_eq[id]) {
            remove_proc_entry(dh9901_proc_clear_eq[id]->name, dh9901_proc_root[id]);
            dh9901_proc_clear_eq[id] = NULL;
        }

        if(dh9901_proc_audio_vol[id]) {
            remove_proc_entry(dh9901_proc_audio_vol[id]->name, dh9901_proc_root[id]);
            dh9901_proc_audio_vol[id] = NULL;
        }

        remove_proc_entry(dh9901_proc_root[id]->name, NULL);
        dh9901_proc_root[id] = NULL;
    }
}

static int dh9901_proc_init(int id)
{
    int ret = 0;

    if(id >= DH9901_DEV_MAX)
        return -1;

    /* root */
    dh9901_proc_root[id] = create_proc_entry(dh9901_dev[id].name, S_IFDIR|S_IRUGO|S_IXUGO, NULL);
    if(!dh9901_proc_root[id]) {
        printk("create proc node '%s' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_root[id]->owner = THIS_MODULE;
#endif

    /* status */
    dh9901_proc_status[id] = create_proc_entry("status", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_status[id]) {
        printk("create proc node '%s/status' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_status[id]->proc_fops = &dh9901_proc_status_ops;
    dh9901_proc_status[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_status[id]->owner     = THIS_MODULE;
#endif

    /* frame format */
    dh9901_proc_video_fmt[id] = create_proc_entry("video_format", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_video_fmt[id]) {
        printk("create proc node '%s/video_format' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_video_fmt[id]->proc_fops = &dh9901_proc_video_fmt_ops;
    dh9901_proc_video_fmt[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_video_fmt[id]->owner     = THIS_MODULE;
#endif

    /* video color */
    dh9901_proc_video_color[id] = create_proc_entry("video_color", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_video_color[id]) {
        printk("create proc node '%s/video_color' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_video_color[id]->proc_fops = &dh9901_proc_video_color_ops;
    dh9901_proc_video_color[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_video_color[id]->owner     = THIS_MODULE;
#endif

    /* video position */
    dh9901_proc_video_pos[id] = create_proc_entry("video_pos", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_video_pos[id]) {
        printk("create proc node '%s/video_pos' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_video_pos[id]->proc_fops = &dh9901_proc_video_pos_ops;
    dh9901_proc_video_pos[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_video_pos[id]->owner     = THIS_MODULE;
#endif

    /* cable type */
    dh9901_proc_cable_type[id] = create_proc_entry("cable_type", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_cable_type[id]) {
        printk("create proc node '%s/cable_type' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_cable_type[id]->proc_fops = &dh9901_proc_cable_type_ops;
    dh9901_proc_cable_type[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_cable_type[id]->owner     = THIS_MODULE;
#endif

    /* clear EQ */
    dh9901_proc_clear_eq[id] = create_proc_entry("clear_eq", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_clear_eq[id]) {
        printk("create proc node '%s/clear_eq' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_clear_eq[id]->proc_fops = &dh9901_proc_clear_eq_ops;
    dh9901_proc_clear_eq[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_clear_eq[id]->owner     = THIS_MODULE;
#endif

    /* audio volume */
    dh9901_proc_audio_vol[id] = create_proc_entry("audio_volume", S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_audio_vol[id]) {
        printk("create proc node '%s/audio_volume' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_audio_vol[id]->proc_fops = &dh9901_proc_audio_vol_ops;
    dh9901_proc_audio_vol[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_audio_vol[id]->owner     = THIS_MODULE;
#endif

    /* ptz root */
    dh9901_proc_ptz_root[id] = create_proc_entry("ptz", S_IFDIR|S_IRUGO|S_IXUGO, dh9901_proc_root[id]);
    if(!dh9901_proc_ptz_root[id]) {
        printk("create proc node '%s/ptz' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto end;
    }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_ptz_root[id]->owner = THIS_MODULE;
#endif

    /* ptz config */
    dh9901_proc_ptz_cfg[id] = create_proc_entry("cfg", S_IRUGO|S_IXUGO, dh9901_proc_ptz_root[id]);
    if(!dh9901_proc_ptz_cfg[id]) {
        printk("create proc node '%s/ptz/cfg' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_ptz_cfg[id]->proc_fops = &dh9901_proc_ptz_cfg_ops;
    dh9901_proc_ptz_cfg[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_ptz_cfg[id]->owner     = THIS_MODULE;
#endif

    /* ptz control */
    dh9901_proc_ptz_ctrl[id] = create_proc_entry("control", S_IRUGO|S_IXUGO, dh9901_proc_ptz_root[id]);
    if(!dh9901_proc_ptz_ctrl[id]) {
        printk("create proc node '%s/ptz/control' failed!\n", dh9901_dev[id].name);
        ret = -EINVAL;
        goto err;
    }
    dh9901_proc_ptz_ctrl[id]->proc_fops = &dh9901_proc_ptz_ctrl_ops;
    dh9901_proc_ptz_ctrl[id]->data      = (void *)&dh9901_dev[id];
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30))
    dh9901_proc_ptz_ctrl[id]->owner     = THIS_MODULE;
#endif

end:
    return ret;

err:
    dh9901_proc_remove(id);
    return ret;
}

static int dh9901_miscdev_open(struct inode *inode, struct file *file)
{
    int i, ret = 0;
    struct dh9901_dev_t *pdev = NULL;

    /* lookup device */
    for(i=0; i<dev_num; i++) {
        if(dh9901_dev[i].miscdev.minor == iminor(inode)) {
            pdev = &dh9901_dev[i];
            break;
        }
    }

    if(!pdev) {
        ret = -EINVAL;
        goto exit;
    }

    file->private_data = (void *)pdev;

exit:
    return ret;
}

static int dh9901_miscdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long dh9901_miscdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct dh9901_dev_t *pdev = (struct dh9901_dev_t *)file->private_data;

    if(_IOC_TYPE(cmd) != DH9901_IOC_MAGIC) {
        ret = -ENOIOCTLCMD;
        goto exit;
    }

    switch(cmd) {
        case DH9901_GET_NOVID:
            {
                struct dh9901_ioc_data_t ioc_data;
                DH_VIDEO_STATUS_S cvi_sts;

                if(copy_from_user(&ioc_data, (void __user *)arg, sizeof(ioc_data))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_data.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_GetVideoStatus(pdev->index, ioc_data.cvi_ch, &cvi_sts);

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, ioc_data.cvi_ch, DH9901_COMM_CMD_GET_VIDEO_STATUS, &cvi_sts, sizeof(cvi_sts), 1, 1000);
                }

                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }

                ioc_data.data = cvi_sts.ucVideoLost;

                ret = (copy_to_user((void __user *)arg, &ioc_data, sizeof(ioc_data))) ? (-EFAULT) : 0;
            }
            break;

        case DH9901_GET_VIDEO_FMT:
            {
                DH_VIDEO_STATUS_S cvi_sts;
                struct dh9901_ioc_vfmt_t ioc_vfmt;

                if(copy_from_user(&ioc_vfmt, (void __user *)arg, sizeof(ioc_vfmt))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vfmt.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_GetVideoStatus(pdev->index, ioc_vfmt.cvi_ch, &cvi_sts);

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, ioc_vfmt.cvi_ch, DH9901_COMM_CMD_GET_VIDEO_STATUS, &cvi_sts, sizeof(cvi_sts), 1, 1000);
                }

                if((ret != 0) || (cvi_sts.ucVideoFormat >= DH9901_VFMT_MAX)) {
                    ret = -EFAULT;
                    goto exit;
                }

                ioc_vfmt.width      = dh9901_video_fmt_info[cvi_sts.ucVideoFormat].width;
                ioc_vfmt.height     = dh9901_video_fmt_info[cvi_sts.ucVideoFormat].height;
                ioc_vfmt.prog       = dh9901_video_fmt_info[cvi_sts.ucVideoFormat].prog;
                ioc_vfmt.frame_rate = dh9901_video_fmt_info[cvi_sts.ucVideoFormat].frame_rate;

                ret = (copy_to_user((void __user *)arg, &ioc_vfmt, sizeof(ioc_vfmt))) ? (-EFAULT) : 0;
            }
            break;

        case DH9901_GET_VIDEO_COLOR:
            {
                struct dh9901_ioc_vcolor_t ioc_vcolor;
                DH_VIDEO_COLOR_S vcol;

                if(copy_from_user(&ioc_vcolor, (void __user *)arg, sizeof(ioc_vcolor))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vcolor.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_GetColor(pdev->index, ioc_vcolor.cvi_ch, &vcol, DH_SET_MODE_USER);

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, ioc_vcolor.cvi_ch, DH9901_COMM_CMD_GET_COLOR, &vcol, sizeof(vcol), 1, 1000);
                }

                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }

                ioc_vcolor.brightness    = vcol.ucBrightness;
                ioc_vcolor.contrast      = vcol.ucContrast;
                ioc_vcolor.saturation    = vcol.ucSaturation;
                ioc_vcolor.hue           = vcol.ucHue;
                ioc_vcolor.gain          = vcol.ucGain;
                ioc_vcolor.white_balance = vcol.ucWhiteBalance;
                ioc_vcolor.sharpness     = vcol.ucSharpness;

                ret = (copy_to_user((void __user *)arg, &ioc_vcolor, sizeof(ioc_vcolor))) ? (-EFAULT) : 0;
            }
            break;

        case DH9901_SET_VIDEO_COLOR:
            {
                struct dh9901_ioc_vcolor_t ioc_vcolor;
                DH_VIDEO_COLOR_S vcol;

                if(copy_from_user(&ioc_vcolor, (void __user *)arg, sizeof(ioc_vcolor))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vcolor.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                vcol.ucBrightness   = ioc_vcolor.brightness;
                vcol.ucContrast     = ioc_vcolor.contrast;
                vcol.ucSaturation   = ioc_vcolor.saturation;
                vcol.ucHue          = ioc_vcolor.hue;
                vcol.ucGain         = ioc_vcolor.gain;
                vcol.ucWhiteBalance = ioc_vcolor.white_balance;
                vcol.ucSharpness    = ioc_vcolor.sharpness;

                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_SetColor(pdev->index, ioc_vcolor.cvi_ch, &vcol, DH_SET_MODE_USER);

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, ioc_vcolor.cvi_ch, DH9901_COMM_CMD_SET_COLOR, &vcol, sizeof(vcol), 1, 1000);
                }

                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }
            }
            break;

        case DH9901_GET_VIDEO_POS:
            {
                DH_VIDEO_POSITION_S vpos;
                struct dh9901_ioc_vpos_t ioc_vpos;

                if(copy_from_user(&ioc_vpos, (void __user *)arg, sizeof(ioc_vpos))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vpos.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_GetVideoPosition(pdev->index, ioc_vpos.cvi_ch, &vpos);

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, ioc_vpos.cvi_ch, DH9901_COMM_CMD_GET_VIDEO_POS, &vpos, sizeof(vpos), 1, 1000);
                }

                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }

                ioc_vpos.h_offset = vpos.sHorizonOffset;
                ioc_vpos.v_offset = vpos.sVerticalOffset;

                ret = (copy_to_user((void __user *)arg, &ioc_vpos, sizeof(ioc_vpos))) ? (-EFAULT) : 0;
            }
            break;

        case DH9901_SET_VIDEO_POS:
            {
                DH_VIDEO_POSITION_S vpos;
                struct dh9901_ioc_vpos_t ioc_vpos;

                if(copy_from_user(&ioc_vpos, (void __user *)arg, sizeof(ioc_vpos))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vpos.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                vpos.sHorizonOffset  = ioc_vpos.h_offset;
                vpos.sVerticalOffset = ioc_vpos.v_offset;

                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_SetVideoPosition(pdev->index, ioc_vpos.cvi_ch, &vpos);

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, ioc_vpos.cvi_ch, DH9901_COMM_CMD_SET_VIDEO_POS, &vpos, sizeof(vpos), 1, 1000);
                }

                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }
            }
            break;

        case DH9901_SET_CABLE_TYPE:
            {
                struct dh9901_ioc_cable_t ioc_cable;

                if(copy_from_user(&ioc_cable, (void __user *)arg, sizeof(ioc_cable))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_cable.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_SetCableType(pdev->index, ioc_cable.cvi_ch, ioc_cable.cab_type);

                    up(&pdev->lock);
                }
                else {
                    dh9901_comm_cmd_send(pdev->index, ioc_cable.cvi_ch, DH9901_COMM_CMD_SET_CABLE_TYPE, &ioc_cable.cab_type, sizeof(DH9901_CABLE_TYPE_T), 1, 1000);
                }

                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }
            }
            break;

        case DH9901_RS485_TX:
            {
                struct dh9901_ioc_rs485_tx_t ioc_rs485;

                if(copy_from_user(&ioc_rs485, (void __user *)arg, sizeof(ioc_rs485))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_rs485.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_Contrl485Enable(pdev->index, ioc_rs485.cvi_ch, 1);

                    if(ret == 0)
                        ret = DH9901_API_Send485Buffer(pdev->index, ioc_rs485.cvi_ch, ioc_rs485.cmd_buf, ioc_rs485.buf_len);

                    DH9901_API_Contrl485Enable(pdev->index, ioc_rs485.cvi_ch, 0);

                    up(&pdev->lock);
                }
                else {
                    struct dh9901_comm_send_rs485_t comm_rs485;

                    comm_rs485.buf_len = ioc_rs485.buf_len;
                    memcpy(comm_rs485.cmd_buf, ioc_rs485.cmd_buf, comm_rs485.buf_len);

                    ret = dh9901_comm_cmd_send(pdev->index, ioc_rs485.cvi_ch, DH9901_COMM_CMD_SEND_RS485, &comm_rs485, sizeof(comm_rs485), 1, 1000);
                }

                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }
            }
            break;

        case DH9901_CLEAR_EQ:
            {
                int cvi_ch;

                if(copy_from_user(&cvi_ch, (void __user *)arg, sizeof(cvi_ch))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_ClearEq(pdev->index, (DH_U8)cvi_ch);

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, cvi_ch, DH9901_COMM_CMD_CLEAR_EQ, NULL, 0, 1, 2000);
                }

                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }
            }
            break;

        case DH9901_SET_AUDIO_VOL:
            {
                struct dh9901_ioc_audio_vol_t ioc_vol;

                if(copy_from_user(&ioc_vol, (void __user *)arg, sizeof(ioc_vol))) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(ioc_vol.cvi_ch >= DH9901_DEV_CH_MAX) {
                    ret = -EFAULT;
                    goto exit;
                }

                if(init) {
                    down(&pdev->lock);

                    ret = DH9901_API_SetAudioInVolume(pdev->index, ioc_vol.cvi_ch, ioc_vol.volume);

                    up(&pdev->lock);
                }
                else {
                    ret = dh9901_comm_cmd_send(pdev->index, ioc_vol.cvi_ch, DH9901_COMM_CMD_SET_AUDIO_VOL, &ioc_vol.volume, sizeof(ioc_vol.volume), 1, 1000);
                }

                if(ret != 0) {
                    ret = -EFAULT;
                    goto exit;
                }
            }
            break;

        default:
            ret = -ENOIOCTLCMD;
            break;
    }

exit:
    return ret;
}

static struct file_operations dh9901_miscdev_fops = {
    .owner          = THIS_MODULE,
    .open           = dh9901_miscdev_open,
    .release        = dh9901_miscdev_release,
    .unlocked_ioctl = dh9901_miscdev_ioctl,
};

static int dh9901_miscdev_init(int id)
{
    int ret;

    if(id >= DH9901_DEV_MAX)
        return -1;

    /* clear */
    memset(&dh9901_dev[id].miscdev, 0, sizeof(struct miscdevice));

    /* create device node */
    dh9901_dev[id].miscdev.name  = dh9901_dev[id].name;
    dh9901_dev[id].miscdev.minor = MISC_DYNAMIC_MINOR;
    dh9901_dev[id].miscdev.fops  = (struct file_operations *)&dh9901_miscdev_fops;
    ret = misc_register(&dh9901_dev[id].miscdev);
    if(ret) {
        printk("create %s misc device node failed!\n", dh9901_dev[id].name);
        dh9901_dev[id].miscdev.name = 0;
        goto exit;
    }

exit:
    return ret;
}

static void dh9901_hw_reset(void)
{
    if(!rstb_used || !init)
        return;

    /* Active Low */
    plat_gpio_set_value(rstb_used, 0);

    udelay(100);

    /* Active High and release reset */
    plat_gpio_set_value(rstb_used, 1);

    udelay(100);
}

static int dh9901_device_register(void)
{
    int i, j, ret;
    DH_DH9901_INIT_ATTR_S dev_attr;
    DH_VIDEO_COLOR_S      video_color;

    if(dh9901_api_inited)
        return -1;

    /* clear attr */
    memset(&dev_attr, 0, sizeof(DH_DH9901_INIT_ATTR_S));

    /* device number on board */
    dev_attr.ucAdCount = dev_num;

    for(i=0; i<dev_num; i++) {
        /* device i2c address */
        dev_attr.stDh9901Attr[i].ucChipAddr = iaddr[i];

        /* video attr */
        dev_attr.stDh9901Attr[i].stVideoAttr.enSetVideoMode   = DH_SET_MODE_USER;
        dev_attr.stDh9901Attr[i].stVideoAttr.ucChnSequence[0] = 0;              ///< VIN#0 -> VPORT#0
        dev_attr.stDh9901Attr[i].stVideoAttr.ucChnSequence[1] = 1;              ///< VIN#1 -> VPORT#1
        dev_attr.stDh9901Attr[i].stVideoAttr.ucChnSequence[2] = 2;              ///< VIN#2 -> VPORT#2
        dev_attr.stDh9901Attr[i].stVideoAttr.ucChnSequence[3] = 3;              ///< VIN#3 -> VPORT#3
        dev_attr.stDh9901Attr[i].stVideoAttr.ucVideoOutMux    = vout_format[i]; ///< 0:BT656, 1:BT1120

        /* audio attr */
        dev_attr.stDh9901Attr[i].stAuioAttr.bAudioEnable = 1;
        dev_attr.stDh9901Attr[i].stAuioAttr.stAudioConnectMode.ucCascade      = 0;
        dev_attr.stDh9901Attr[i].stAuioAttr.stAudioConnectMode.ucCascadeNum   = 0;
        dev_attr.stDh9901Attr[i].stAuioAttr.stAudioConnectMode.ucCascadeStage = 0;
        dev_attr.stDh9901Attr[i].stAuioAttr.enSetAudioMode    = DH_SET_MODE_USER;
        dev_attr.stDh9901Attr[i].stAuioAttr.enAudioSampleRate = sample_rate;
        dev_attr.stDh9901Attr[i].stAuioAttr.enAudioEncClkMode = DH_AUDIO_ENCCLK_MODE_MASTER;
        dev_attr.stDh9901Attr[i].stAuioAttr.enAudioSyncMode   = DH_AUDIO_SYNC_MODE_I2S;
        dev_attr.stDh9901Attr[i].stAuioAttr.enAudioEncClkSel  = DH_AUDIO_ENCCLK_MODE_27M;

        /* rs485 attr */
        dev_attr.stDh9901Attr[i].stPtz485Attr.enSetVideoMode = DH_SET_MODE_USER;
        dev_attr.stDh9901Attr[i].stPtz485Attr.ucProtocolLen  = 7;
    }

    /* auto detect thread attr */
    dev_attr.stDetectAttr.enDetectMode   = DH_SET_MODE_USER;
    dev_attr.stDetectAttr.ucDetectPeriod = 50; ///< ms

    /* register i2c read/write function */
    dev_attr.Dh9901_WriteByte = dh9901_writebyte;
    dev_attr.Dh9901_ReadByte  = dh9901_readbyte;

    ret = DH9901_API_Init(&dev_attr);
    if(ret != 0)
        goto exit;

    /* setup default color setting */
    video_color.ucBrightness   = 50;
    video_color.ucContrast     = 50;
    video_color.ucSaturation   = 50;
    video_color.ucHue          = 50;
    video_color.ucGain         = 0;
    video_color.ucWhiteBalance = 0;
    video_color.ucSharpness    = 1;
    for(i=0; i<dev_num; i++) {
        for(j=0; j<DH9901_DEV_CH_MAX; j++) {
            ret = DH9901_API_SetColor(i, j, &video_color, DH_SET_MODE_USER);
            if(ret != 0)
                goto exit;
        }
    }

    /* PTZ config init */
    for(i=0; i<dev_num; i++) {
        for(j=0; j<DH9901_DEV_CH_MAX; j++) {
            dh9901_dev[i].ptz[j].protocol     = DH9901_PTZ_PTOTOCOL_DHSD1;
            dh9901_dev[i].ptz[j].baud_rate    = DH9901_PTZ_BAUD_9600;
            dh9901_dev[i].ptz[j].parity_chk   = 1;
        }
    }

    /* set flag */
    dh9901_api_inited = 1;

exit:
    return ret;
}

static void dh9901_device_deregister(void)
{
    if(dh9901_api_inited) {
        DH9901_API_DeInit();
        dh9901_api_inited = 0;
    }
}

static int dh9901_sound_switch(int ssp_idx, int chan_num, bool is_on)
{
    /* ToDo */

    return 0;
}

static int dh9901_watchdog_thread(void *data)
{
    int ret, i, ch;
    DH_VIDEO_STATUS_S cvi_sts;
    struct dh9901_dev_t *pdev;
    struct dh9901_video_fmt_t *p_vfmt;

    do {
            /* check dh9901 channel status */
            for(i=0; i<dev_num; i++) {
                pdev = &dh9901_dev[i];

                down(&pdev->lock);

                for(ch=0; ch<DH9901_DEV_CH_MAX; ch++) {
                    ret = DH9901_API_GetVideoStatus(i, ch, &cvi_sts);
                    if(ret != 0)
                        continue;

                    /* get video format information */
                    if(cvi_sts.ucVideoFormat >= DH9901_VFMT_MAX)
                        p_vfmt = &dh9901_video_fmt_info[DH9901_VFMT_720P25];
                    else
                        p_vfmt = &dh9901_video_fmt_info[cvi_sts.ucVideoFormat];

                    pdev->cur_plugin[ch] = (cvi_sts.ucVideoLost == 0) ? 1 : 0;
                    if((pdev->cur_plugin[ch] == 1) && (pdev->pre_plugin[ch] == 0)) {        ///< cable plugged-in
                        goto sts_update;
                    }
                    else if((pdev->cur_plugin[ch] == 1) && (pdev->pre_plugin[ch] == 1)) {   ///< cable is connected
                        /* notify current video present */
                        if(notify && pdev->vlos_notify)
                            pdev->vlos_notify(i, ch, 0);

                        /* notify current video format */
                        if(notify && pdev->vfmt_notify)
                            pdev->vfmt_notify(i, ch, p_vfmt);
                    }
                    else {  ///< cable is plugged-out
                        /* notify current video loss */
                        if(notify && pdev->vlos_notify)
                            pdev->vlos_notify(i, ch, 1);

                        /* notify current video format, video loss will switch to 720P25 */
                        if(notify && pdev->vfmt_notify)
                            pdev->vfmt_notify(i, ch, p_vfmt);
                    }
sts_update:
                    pdev->pre_plugin[ch] = pdev->cur_plugin[ch];
                }

                up(&pdev->lock);
            }

            /* sleep 0.5 second */
            schedule_timeout_interruptible(msecs_to_jiffies(500));

    } while(!kthread_should_stop());

    return 0;
}

static int dh9901_comm_thread(void *data)
{
    int ret, id, ch;
    struct dh9901_dev_t *pdev;
    struct plat_cpucomm_msg  rx_msg;
    struct dh9901_comm_header_t *p_header;
    unsigned char *p_data;
    unsigned char *msg_buf = NULL;

    /* allocate dh9901 comm rx message buffer */
    msg_buf = kzalloc(DH9901_COMM_MSG_BUF_MAX, GFP_KERNEL);
    if(!msg_buf) {
        printk("DH9901 COMM allocate memory failed!!\n");
        goto exit;
    }

    do {
            /* wait to receive data */
            rx_msg.length   = DH9901_COMM_MSG_BUF_MAX;
            rx_msg.msg_data = msg_buf;
            ret = plat_cpucomm_rx(&rx_msg, 100);    ///< 100ms timeout
            if(ret != 0)
                continue;

            /* process receive message data */
            if(rx_msg.length >= sizeof(struct dh9901_comm_header_t)) {
                p_header = (struct dh9901_comm_header_t *)rx_msg.msg_data;
                p_data   = ((unsigned char *)p_header) + sizeof(struct dh9901_comm_header_t);

                /* check header magic number */
                if(p_header->magic == DH9901_COMM_MSG_MAGIC) {
                    /* check device id and channel */
                    id = p_header->dev_id;
                    ch = p_header->dev_ch;
                    if(((id >= dev_num) || (ch >= DH9901_DEV_CH_MAX))) {
                        dh9901_comm_cmd_send(0, 0, DH9901_COMM_CMD_ACK_FAIL, NULL, 0, 0, 0);
                        continue;
                    }

                    pdev = &dh9901_dev[id];

                    /* process command */
                    switch(p_header->cmd_id) {
                        case DH9901_COMM_CMD_ACK_OK:
                        case DH9901_COMM_CMD_ACK_FAIL:
                            break;

                        case DH9901_COMM_CMD_GET_VIDEO_STATUS:
                            {
                                DH_VIDEO_STATUS_S *pcvi_sts = (DH_VIDEO_STATUS_S *)p_data;

                                down(&pdev->lock);

                                ret = DH9901_API_GetVideoStatus(id, ch, pcvi_sts);

                                up(&pdev->lock);

                                if(ret == 0)
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_GET_VIDEO_STATUS, pcvi_sts, sizeof(DH_VIDEO_STATUS_S), 0, 0);
                                else
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_FAIL, NULL, 0, 0, 0);
                            }
                            break;

                        case DH9901_COMM_CMD_SET_COLOR:
                            {
                                DH_VIDEO_COLOR_S *pvideo_color = (DH_VIDEO_COLOR_S *)p_data;

                                down(&pdev->lock);

                                ret = DH9901_API_SetColor(id, ch, pvideo_color, DH_SET_MODE_USER);

                                up(&pdev->lock);

                                if(ret == 0)
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_OK,   NULL, 0, 0, 0);
                                else
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_FAIL, NULL, 0, 0, 0);
                            }
                            break;

                        case DH9901_COMM_CMD_GET_COLOR:
                            {
                                DH_VIDEO_COLOR_S *pvideo_color = (DH_VIDEO_COLOR_S *)p_data;

                                down(&pdev->lock);

                                ret = DH9901_API_GetColor(id, ch, pvideo_color, DH_SET_MODE_USER);

                                up(&pdev->lock);

                                if(ret == 0)
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_GET_COLOR, pvideo_color, sizeof(DH_VIDEO_COLOR_S), 0, 0);
                                else
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_FAIL, NULL, 0, 0, 0);
                            }
                            break;

                        case DH9901_COMM_CMD_CLEAR_EQ:
                            {
                                down(&pdev->lock);

                                ret = DH9901_API_ClearEq(id, ch);

                                up(&pdev->lock);

                                if(ret == 0)
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_OK,   NULL, 0, 0, 0);
                                else
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_FAIL, NULL, 0, 0, 0);
                            }
                            break;

                        case DH9901_COMM_CMD_SEND_RS485:
                            {
                                struct dh9901_comm_send_rs485_t *pcomm_rs485 = (struct dh9901_comm_send_rs485_t *)p_data;

                                down(&pdev->lock);

                                ret = DH9901_API_Contrl485Enable(id, ch, 1);

                                if(ret == 0)
                                    ret = DH9901_API_Send485Buffer(id, ch, pcomm_rs485->cmd_buf, pcomm_rs485->buf_len);

                                DH9901_API_Contrl485Enable(id, ch, 0);

                                up(&pdev->lock);

                                if(ret == 0)
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_OK,   NULL, 0, 0, 0);
                                else
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_FAIL, NULL, 0, 0, 0);
                            }
                            break;

                        case DH9901_COMM_CMD_SET_VIDEO_POS:
                            {
                                DH_VIDEO_POSITION_S *pvideo_pos = (DH_VIDEO_POSITION_S *)p_data;

                                down(&pdev->lock);

                                ret = DH9901_API_SetVideoPosition(id, ch, pvideo_pos);

                                up(&pdev->lock);

                                if(ret == 0)
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_OK,   NULL, 0, 0, 0);
                                else
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_FAIL, NULL, 0, 0, 0);
                            }
                            break;

                        case DH9901_COMM_CMD_GET_VIDEO_POS:
                            {
                                DH_VIDEO_POSITION_S *pvideo_pos = (DH_VIDEO_POSITION_S *)p_data;

                                down(&pdev->lock);

                                ret = DH9901_API_GetVideoPosition(id, ch, pvideo_pos);

                                up(&pdev->lock);

                                if(ret == 0)
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_GET_VIDEO_POS, pvideo_pos, sizeof(DH_VIDEO_POSITION_S), 0, 0);
                                else
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_FAIL, NULL, 0, 0, 0);
                            }
                            break;

                        case DH9901_COMM_CMD_SET_CABLE_TYPE:
                            {
                                DH9901_CABLE_TYPE_T *pcab_type = (DH9901_CABLE_TYPE_T *)p_data;

                                down(&pdev->lock);

                                ret = DH9901_API_SetCableType(id, ch, *pcab_type);

                                up(&pdev->lock);

                                if(ret == 0)
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_OK,   NULL, 0, 0, 0);
                                else
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_FAIL, NULL, 0, 0, 0);
                            }
                            break;

                        case DH9901_COMM_CMD_SET_AUDIO_VOL:
                            {
                                u8 *p_vol = (u8 *)p_data;

                                down(&pdev->lock);

                                ret = DH9901_API_SetAudioInVolume(id, ch, *p_vol);

                                up(&pdev->lock);

                                if(ret == 0)
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_OK,   NULL, 0, 0, 0);
                                else
                                    dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_ACK_FAIL, NULL, 0, 0, 0);
                            }
                            break;

                        case DH9901_COMM_CMD_GET_PTZ_CFG:
                            {
                                struct dh9901_ptz_t ptz_cfg;

                                down(&pdev->lock);

                                memcpy(&ptz_cfg, &pdev->ptz[ch], sizeof(struct dh9901_ptz_t));

                                up(&pdev->lock);

                                dh9901_comm_cmd_send(id, ch, DH9901_COMM_CMD_GET_PTZ_CFG, &ptz_cfg, sizeof(ptz_cfg), 0, 0);
                            }
                            break;

                        default:
                            printk("DH9901 COMM unkonw command id(%d)\n", p_header->cmd_id);
                            break;
                    }
                }
                else
                    printk("DH9901 COMM message magic not matched(0x%08x)\n", p_header->magic);
            }
            else
                printk("DH9901 COMM message receive length incorrect!!(%d)\n", rx_msg.length);
    } while(!kthread_should_stop());

exit:
    /* free buffer */
    if(msg_buf)
        kfree(msg_buf);

    return 0;
}

static int __init dh9901_init(void)
{
    int i, ret;
    int comm_ready = 0;

    /* platfrom check */
    plat_identifier_check();

    /* check device number */
    if(dev_num > DH9901_DEV_MAX) {
        printk("DH9901 dev_num=%d invalid!!(Max=%d)\n", dev_num, DH9901_DEV_MAX);
        return -1;
    }

    /* check video output format */
    for(i=0; i<dev_num; i++) {
        if((vout_format[i] < 0) || (vout_format[i] >= DH9901_VOUT_FORMAT_MAX)) {
            printk("DH9901#%d vout_format=%d not support!!\n", i, vout_format[i]);
            return -1;
        }
    }

    /* register i2c client for contol dh9901 */
    ret = dh9901_i2c_register();
    if(ret < 0)
        return -1;

    /* register gpio pin for device rstb pin */
    if(rstb_used && init) {
        ret = plat_register_gpio_pin(rstb_used, PLAT_GPIO_DIRECTION_OUT, 0);
        if(ret < 0)
            goto err;
    }

    /* setup platfrom external clock */
    if((init == 1) && (clk_used >= 0)) {
        if(clk_used) {
            ret = plat_extclk_set_src(clk_src);
            if(ret < 0)
                goto err;

            if(clk_used & 0x1) {
                ret = plat_extclk_set_freq(0, clk_freq);
                if(ret < 0)
                    goto err;
            }

            if(clk_used & 0x2) {
                ret = plat_extclk_set_freq(1, clk_freq);
                if(ret < 0)
                    goto err;
            }

            ret = plat_extclk_onoff(1);
            if(ret < 0)
                goto err;

        }
        else {
            /* use external oscillator. disable SoC external clock output */
            ret = plat_extclk_onoff(0);
            if(ret < 0)
                goto err;
        }
    }

    /* open cpu_comm channel for dh9901 driver data communication of dual cpu */
    ret = plat_cpucomm_open();
    if(ret == 0)
        comm_ready = 1;

    /* create channel mapping table for different board */
    ret = dh9901_create_ch_map(ch_map);
    if(ret < 0)
        goto err;

    /* hardware reset for all device */
    dh9901_hw_reset();

    /* DH9901 init */
    for(i=0; i<dev_num; i++) {
        dh9901_dev[i].index = i;

        sprintf(dh9901_dev[i].name, "dh9901.%d", i);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&dh9901_dev[i].lock, 1);
#else
        init_MUTEX(&dh9901_dev[i].lock);
#endif
        ret = dh9901_proc_init(i);
        if(ret < 0)
            goto err;

        ret = dh9901_miscdev_init(i);
        if(ret < 0)
            goto err;
    }

    /* dh9901 device register */
    if(init) {
        ret = dh9901_device_register();
        if(ret != 0)
            goto err;
    }

    if(init) {
        /* init dh9901 watchdog thread for check video status */
        dh9901_wd = kthread_create(dh9901_watchdog_thread, NULL, "dh9901_wd");
        if(!IS_ERR(dh9901_wd))
            wake_up_process(dh9901_wd);
        else {
            printk("create dh9901 watchdog thread failed!!\n");
            dh9901_wd = 0;
            ret = -EFAULT;
            goto err;
        }

        /* init dh9901 comm thread for receive data from corresponding cpu */
        if(comm_ready) {
            dh9901_comm = kthread_create(dh9901_comm_thread, NULL, "dh9901_comm");
            if(!IS_ERR(dh9901_comm))
                wake_up_process(dh9901_comm);
            else {
                printk("create dh9901 comm thread failed!!\n");
                dh9901_comm = 0;
                ret = -EFAULT;
                goto err;
            }
        }
    }

    /* register audio function for platform used */
    dh9901_audio_funs.sound_switch = dh9901_sound_switch;
    dh9901_audio_funs.codec_name   = dh9901_dev[0].name;
    plat_audio_register_function(&dh9901_audio_funs);

    return 0;

err:
    if(dh9901_wd)
        kthread_stop(dh9901_wd);

    if(dh9901_comm)
        kthread_stop(dh9901_comm);

    dh9901_device_deregister();

    plat_cpucomm_close();

    dh9901_i2c_unregister();
    for(i=0; i<DH9901_DEV_MAX; i++) {
        if(dh9901_dev[i].miscdev.name)
            misc_deregister(&dh9901_dev[i].miscdev);

        dh9901_proc_remove(i);
    }

    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    return ret;
}

static void __exit dh9901_exit(void)
{
    int i;

    /* stop dh9901 watchdog thread */
    if(dh9901_wd)
        kthread_stop(dh9901_wd);

    if(dh9901_comm)
        kthread_stop(dh9901_comm);

    dh9901_device_deregister();

    plat_cpucomm_close();

    dh9901_i2c_unregister();

    for(i=0; i<DH9901_DEV_MAX; i++) {
        /* remove device node */
        if(dh9901_dev[i].miscdev.name)
            misc_deregister(&dh9901_dev[i].miscdev);

        /* remove proc node */
        dh9901_proc_remove(i);
    }

    /* release gpio pin */
    if(rstb_used && init)
        plat_deregister_gpio_pin(rstb_used);

    plat_audio_deregister_function();
}

module_init(dh9901_init);
module_exit(dh9901_exit);

MODULE_DESCRIPTION("Grain Media DH9901 4CH HDCVI Receiver Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
