/*+++ *******************************************************************\
*
*  Copyright and Disclaimer:
*
*     ---------------------------------------------------------------
*     This software is provided "AS IS" without warranty of any kind,
*     either expressed or implied, including but not limited to the
*     implied warranties of noninfringement, merchantability and/or
*     fitness for a particular purpose.
*     ---------------------------------------------------------------
*
*     Copyright (c) 2013 Conexant Systems, Inc.
*     All rights reserved.
*
\******************************************************************* ---*/
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

#include "lib_cx20811/Ibiza.h"
#include "lib_cx20811/Comm.h"
#include "platform.h"

#define MAX_CHIP_NUM		2
#define CX20811_I2C_ADDR   0x76

static int cx20811_audio_set_output_ch(int decoder_idx, int chan_num);

static unsigned char cx20811_i2c_addr[] = {CX20811_I2C_ADDR};

static CX_COMMUNICATION comm[MAX_CHIP_NUM];
static int initIbizaChip(void);

static struct i2c_client  *cx20811_i2c_client = NULL;

/* device init */
static int init = 1;
module_param(init, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init, "device init: 0:no-init  1:init");

/* device number */
static int dev_num = 1;
module_param(dev_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dev_num, "Device Number: 1~4");

/* device i2c bus */
static int ibus = 0;
module_param(ibus, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ibus, "Device I2C Bus: 0~4");

/* audio sample rate */
int ad_sample_rate = CX20811_SAMPLE_RATE_8000;
module_param(ad_sample_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ad_sample_rate, "cx20811 ad sample rate: 0:48k  1:44.k 2:32k 3:16k 4:8k");

int da_sample_rate = CX20811_SAMPLE_RATE_8000;
module_param(da_sample_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(da_sample_rate, "cx20811 da sample rate: 0:48k  1:44.k 2:32k 3:16k 4:8k");

/* audio sample size */
int adda_sample_size = CX20811_SAMPLE_BIT_16;
module_param(adda_sample_size, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(adda_sample_size, "audio sample size: 0:8bit  1:16bit 2:24bit");

struct cx20811_dev_t {
    int                     index;                          ///< device index
    char                    name[16];                       ///< device name
    struct semaphore        lock;                           ///< device locker
    struct miscdevice       miscdev;                        ///< device node for ioctl access
};

static struct cx20811_dev_t  cx20811_dev[MAX_CHIP_NUM];
static audio_funs_t cx20811_audio_funs;

static int cx20811_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    cx20811_i2c_client = client;
    return 0;
}

static int cx20811_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id cx20811_i2c_id[] = {
    { "cx20811_i2c", 0 },
    { }
};

static struct i2c_driver cx20811_i2c_driver = {
    .driver = {
        .name  = "CX20811_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = cx20811_i2c_probe,
    .remove   = cx20811_i2c_remove,
    .id_table = cx20811_i2c_id
};

static int cx20811_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&cx20811_i2c_driver);
    if(ret < 0) {
        printk("CX20811 can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = cx20811_i2c_addr[0]>>1;
    strlcpy(info.type, "cx20811_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(ibus);   ///< I2C BUS#
    if (!adapter) {
        printk("CX20811 can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        printk("CX20811 can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&cx20811_i2c_driver);
   return -ENODEV;
}

static void cx20811_i2c_unregister(void)
{
    i2c_unregister_device(cx20811_i2c_client);
    i2c_del_driver(&cx20811_i2c_driver);
    cx20811_i2c_client = NULL;
}

int cx20811_i2c_write(u8 id, u16 reg, u32 data, int size)
{
    u8  buf[6];
    struct i2c_msg msgs;
    struct i2c_adapter *adapter;

    if((id >= dev_num) || (size <= 0) || (size > 4))
        return -1;

    if(!cx20811_i2c_client) {
        printk("CX20811 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(cx20811_i2c_client->dev.parent);

    buf[0]     = reg;
    buf[1]     = data;
    msgs.addr  = cx20811_i2c_addr[id]>>1;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    if(i2c_transfer(adapter, &msgs, 1) != 1) {
        printk("CX20811#%x i2c write failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(cx20811_i2c_write);

int cx20811_i2c_read(u8 id, u8 reg, void *data, int size)
{
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if((id >= dev_num) || (!data) || (size <= 0) || (size > 4))
        return -1;

    if(!cx20811_i2c_client) {
        printk("CX20811 i2c_client not register!!\n");
        return -1;
    }

    adapter = to_i2c_adapter(cx20811_i2c_client->dev.parent);

    msgs[0].addr  = cx20811_i2c_addr[id]>>1;
    msgs[0].flags = 0; /* write */
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = (cx20811_i2c_addr[id] + 1)>>1;
    msgs[1].flags = 1; /* read */
    msgs[1].len   = 1;
    msgs[1].buf   = (u8 *)data;

    if(i2c_transfer(adapter, msgs, 2) != 2) {
        printk("CX20811#%x i2c read failed!!\n", id);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(cx20811_i2c_read);


/*******************************************************************************************************
*
* writeCx20811Byte
* write a byte to a 8bit register address
*
********************************************************************************************************/
static int writeCx20811Byte(const CX_COMMUNICATION* p_comm,
              const unsigned long registerAddress,
              const unsigned char value)
{
    int ret_val = p_comm->write(p_comm->i2c_addr, registerAddress, value, sizeof(value));
    return ret_val;
}

/*******************************************************************************************************
*
* readCx20811Byte
* read a byte from 8bit register address
*
*******************************************************************************************************/
static int readCx20811Byte(const CX_COMMUNICATION *p_comm,
              const unsigned long registerAddress,
              unsigned char *p_value)
{
	int ret_val = p_comm->read(p_comm->i2c_addr, registerAddress, p_value, 1);
	return ret_val;

}

int cx20811_initializePLL(const CX_COMMUNICATION *p_comm)
{

	char value = 0x0;
	writeCx20811Byte(p_comm, 0x78, 0x2D );
	readCx20811Byte(p_comm, 0x78, &value);
	readCx20811Byte(p_comm, 0x78, &value);
	readCx20811Byte(p_comm, 0x78, &value);
	readCx20811Byte(p_comm, 0x78, &value);

	writeCx20811Byte(p_comm, 0x78, 0x6D );
	readCx20811Byte(p_comm, 0x78, &value);
	readCx20811Byte(p_comm, 0x78, &value);
	readCx20811Byte(p_comm, 0x78, &value);
	readCx20811Byte(p_comm, 0x78, &value);

	writeCx20811Byte(p_comm, 0x7A, 0x01 );
	writeCx20811Byte(p_comm, 0x80, 0x01 );
	writeCx20811Byte(p_comm, 0x08, 0x20 );
	writeCx20811Byte(p_comm, 0x09, 0x03 );
	writeCx20811Byte(p_comm, 0x0E, 0x0F );
	writeCx20811Byte(p_comm, 0x16, 0x00 );
	writeCx20811Byte(p_comm, 0xA0, 0x0E );
	writeCx20811Byte(p_comm, 0xA7, 0x0E );
	writeCx20811Byte(p_comm, 0xAE, 0x0E );
	writeCx20811Byte(p_comm, 0xB5, 0x0E );

	return 0;
}


int cx20811_initializeDACI2SMaster(const CX_COMMUNICATION *p_comm, int sample_bits, int channel_index)
{
	writeCx20811Byte(p_comm, 0x0C, 0x3B );
	writeCx20811Byte(p_comm, 0x82, 0x30 );
	writeCx20811Byte(p_comm, 0x83, 0x00 );

	switch (sample_bits)
	{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x30, 0x01 );
			writeCx20811Byte(p_comm, 0x32, 0x01 );
			writeCx20811Byte(p_comm, 0x34, 0x87 );
			break;

		case CX20811_SAMPLE_BIT_16:
			writeCx20811Byte(p_comm, 0x30, 0x0B );
			writeCx20811Byte(p_comm, 0x32, 0x03 );
			writeCx20811Byte(p_comm, 0x34, 0x8F );
			break;

		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x30, 0x15 );
			writeCx20811Byte(p_comm, 0x32, 0x07 );
			writeCx20811Byte(p_comm, 0x34, 0x9F );
			break;
	}


	writeCx20811Byte(p_comm, 0x37, 0x80 );
	writeCx20811Byte(p_comm, 0x38, 0x00 );
	writeCx20811Byte(p_comm, 0x39, 0xCA );

	if (channel_index == CX20811_LEFT_CHANNEL)
	{
		writeCx20811Byte(p_comm, 0x3F, 0x80 );
	}
	else if (channel_index == CX20811_RIGHT_CHANNEL)
	{
		if (sample_bits == CX20811_SAMPLE_BIT_8)
		{
			writeCx20811Byte(p_comm, 0x3F, 0x81 );
		}
		else if (sample_bits == CX20811_SAMPLE_BIT_16)
		{
			writeCx20811Byte(p_comm, 0x3F, 0x82 );
		}
		else if (sample_bits == CX20811_SAMPLE_BIT_24)
		{
			writeCx20811Byte(p_comm, 0x3F, 0x84 );
		}
	}

	return 0;
}

int cx20811_initializeDACI2SSlave(const CX_COMMUNICATION *p_comm, int sample_bits, int channel_index)
{
	writeCx20811Byte(p_comm, 0x0C, 0x2B);
	writeCx20811Byte(p_comm, 0x82, 0x3f);
	writeCx20811Byte(p_comm, 0x83, 0x00);

	switch (sample_bits)
	{
	case CX20811_SAMPLE_BIT_8:
		writeCx20811Byte(p_comm, 0x30, 0x01);
		writeCx20811Byte(p_comm, 0x32, 0x01);
		writeCx20811Byte(p_comm, 0x34, 0x87);
		break;
	case CX20811_SAMPLE_BIT_16:
		writeCx20811Byte(p_comm, 0x30, 0x0B);
		writeCx20811Byte(p_comm, 0x32, 0x07);
		writeCx20811Byte(p_comm, 0x34, 0x9F);
		break;
	case CX20811_SAMPLE_BIT_24:
		writeCx20811Byte(p_comm, 0x30, 0x15);
		writeCx20811Byte(p_comm, 0x32, 0x07);
		writeCx20811Byte(p_comm, 0x34, 0x9F);
		break;
	}

	writeCx20811Byte(p_comm, 0x37, 0x80);
	writeCx20811Byte(p_comm, 0x38, 0x00);
	writeCx20811Byte(p_comm, 0x39, 0xCA);

	if (channel_index == CX20811_LEFT_CHANNEL)
	{
		writeCx20811Byte(p_comm, 0x3F, 0x80);
	}
	else if (channel_index == CX20811_RIGHT_CHANNEL)
	{
		if (sample_bits == CX20811_SAMPLE_BIT_8)
		{
			writeCx20811Byte(p_comm, 0x3F, 0x81);
		}
		else if (sample_bits == CX20811_SAMPLE_BIT_16)
		{
			writeCx20811Byte(p_comm, 0x3F, 0x82);
		}
		else if (sample_bits == CX20811_SAMPLE_BIT_24)
		{
			writeCx20811Byte(p_comm, 0x3F, 0x84);
		}
	}
	return 0;
}


int cx20811_initializeDAC48K(const CX_COMMUNICATION *p_comm, int sample_bits)
{
	switch (sample_bits)
	{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x0B, 0x0F);
			writeCx20811Byte(p_comm, 0x0B, 0x8F);
			break;

		case CX20811_SAMPLE_BIT_16:
			writeCx20811Byte(p_comm, 0x0B, 0x07);
			writeCx20811Byte(p_comm, 0x0B, 0x87);
			break;

		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x0B, 0x03);
			writeCx20811Byte(p_comm, 0x0B, 0x83);
			break;
	}

	writeCx20811Byte(p_comm, 0x28, 0x95);
	writeCx20811Byte(p_comm, 0x29, 0x44);
	writeCx20811Byte(p_comm, 0x2A, 0x80);

    return 0;
}

int cx20811_initializeDAC32K(const CX_COMMUNICATION *p_comm, int sample_bits)
{
	switch (sample_bits)
{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x0B, 0x2F);
			writeCx20811Byte(p_comm, 0x0B, 0xAF);
			break;
		case CX20811_SAMPLE_BIT_16:
			writeCx20811Byte(p_comm, 0x0B, 0x05); //32
			writeCx20811Byte(p_comm, 0x0B, 0x85);
			break;
		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x0B, 0x0B);
			writeCx20811Byte(p_comm, 0x0B, 0x8B);
			break;
	}

	writeCx20811Byte(p_comm, 0x28, 0x95);
	writeCx20811Byte(p_comm, 0x29, 0x34);   //32
	writeCx20811Byte(p_comm, 0x2A, 0x80);

    return 0;
}

int cx20811_initializeDAC24K(const CX_COMMUNICATION *p_comm, int sample_bits)
{
	switch (sample_bits)
	{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x0B, 0x2F);
			writeCx20811Byte(p_comm, 0x0B, 0xAF);
			break;
		case CX20811_SAMPLE_BIT_16:
			writeCx20811Byte(p_comm, 0x0B, 0x07); //24
			writeCx20811Byte(p_comm, 0x0B, 0x87);
			break;
		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x0B, 0x0B);
			writeCx20811Byte(p_comm, 0x0B, 0x8B);
			break;
	}

	writeCx20811Byte(p_comm, 0x28, 0x95);
	writeCx20811Byte(p_comm, 0x29, 0x24);   //24
	writeCx20811Byte(p_comm, 0x2A, 0x80);

    return 0;
}

int cx20811_initializeDAC16K(const CX_COMMUNICATION *p_comm, int sample_bits)
{
	switch (sample_bits)
	{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x0B, 0x2F);
			writeCx20811Byte(p_comm, 0x0B, 0xAF);
			break;
		case CX20811_SAMPLE_BIT_16:
			writeCx20811Byte(p_comm, 0x0B, 0x0b);   //16
			writeCx20811Byte(p_comm, 0x0B, 0x8b);
			break;
		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x0B, 0x0B);
			writeCx20811Byte(p_comm, 0x0B, 0x8B);
			break;
	}

	writeCx20811Byte(p_comm, 0x28, 0x95);
	writeCx20811Byte(p_comm, 0x29, 0x14);   //16
	writeCx20811Byte(p_comm, 0x2A, 0x80);

    return 0;
}


int cx20811_initializeDAC8K(const CX_COMMUNICATION *p_comm, int sample_bits)
{

	switch (sample_bits)
	{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x0B, 0x5F);
			writeCx20811Byte(p_comm, 0x0B, 0xDF);
			break;

		case CX20811_SAMPLE_BIT_16:
			// bclk = 512Khz -> 12.288Mhz/24
			writeCx20811Byte(p_comm, 0x0B, 0x17);
			writeCx20811Byte(p_comm, 0x0B, 0x97);
			break;

		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x0B, 0x17);
			writeCx20811Byte(p_comm, 0x0B, 0x97);
			break;
	}

	writeCx20811Byte(p_comm, 0x28, 0x95);
	writeCx20811Byte(p_comm, 0x29, 0x04);
	writeCx20811Byte(p_comm, 0x2A, 0x80);

    return 0;
}

int cx20811_initializeADCSlave(const CX_COMMUNICATION *p_comm, int sample_bits)
{
    writeCx20811Byte(p_comm, 0x0C, 0x2a);   // rx slave, tx slave
	writeCx20811Byte(p_comm, 0x82, 0x3f);
    writeCx20811Byte(p_comm, 0x83, 0x0f);

	switch (sample_bits)
	{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x30, 0x01);
			writeCx20811Byte(p_comm, 0x31, 0x03);
			writeCx20811Byte(p_comm, 0x33, 0x8F);
			writeCx20811Byte(p_comm, 0x3A, 0x00);
			writeCx20811Byte(p_comm, 0x3B, 0x01);
			writeCx20811Byte(p_comm, 0x3C, 0x02);
			writeCx20811Byte(p_comm, 0x3D, 0x03);
			writeCx20811Byte(p_comm, 0x3E, 0x7F);
			break;

		case CX20811_SAMPLE_BIT_16:
			writeCx20811Byte(p_comm, 0x30, 0x0B);
			writeCx20811Byte(p_comm, 0x31, 0x07);
			writeCx20811Byte(p_comm, 0x33, 0x9F);
			// 0123
			writeCx20811Byte(p_comm, 0x3A, 0x02);
			writeCx20811Byte(p_comm, 0x3B, 0x00);
			writeCx20811Byte(p_comm, 0x3C, 0x06);
			writeCx20811Byte(p_comm, 0x3D, 0x04);
			writeCx20811Byte(p_comm, 0x3E, 0x7F);
			break;

		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x30, 0x15);
			writeCx20811Byte(p_comm, 0x31, 0x0F);
			writeCx20811Byte(p_comm, 0x33, 0xBF);
			writeCx20811Byte(p_comm, 0x3A, 0x00);
			writeCx20811Byte(p_comm, 0x3B, 0x03);
			writeCx20811Byte(p_comm, 0x3C, 0x08);
			writeCx20811Byte(p_comm, 0x3D, 0x0B);
			writeCx20811Byte(p_comm, 0x3E, 0x3F);
			break;
	}

	writeCx20811Byte(p_comm, 0x35, 0x80);
	writeCx20811Byte(p_comm, 0x36, 0x00);
	writeCx20811Byte(p_comm, 0x39, 0xCA);

	return 0;
}

int cx20811_initializeADCMaster(const CX_COMMUNICATION *p_comm, int sample_bits)
{
	writeCx20811Byte(p_comm, 0x0C, 0x2B);   // rx master, tx slave
	writeCx20811Byte(p_comm, 0x82, 0x3f);
	writeCx20811Byte(p_comm, 0x83, 0x00);

	switch (sample_bits)
	{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x30, 0x01);
			writeCx20811Byte(p_comm, 0x31, 0x03);
			writeCx20811Byte(p_comm, 0x33, 0x8F);
			writeCx20811Byte(p_comm, 0x3A, 0x00);
			writeCx20811Byte(p_comm, 0x3B, 0x01);
			writeCx20811Byte(p_comm, 0x3C, 0x02);
			writeCx20811Byte(p_comm, 0x3D, 0x03);
			writeCx20811Byte(p_comm, 0x3E, 0x7F);
			break;

		case CX20811_SAMPLE_BIT_16:
			writeCx20811Byte(p_comm, 0x30, 0x0B);
			writeCx20811Byte(p_comm, 0x31, 0x07);
			writeCx20811Byte(p_comm, 0x33, 0x9F);
            // channel sequence 0123
			writeCx20811Byte(p_comm, 0x3A, 0x02);
			writeCx20811Byte(p_comm, 0x3B, 0x00);
			writeCx20811Byte(p_comm, 0x3C, 0x06);
			writeCx20811Byte(p_comm, 0x3D, 0x04);
			writeCx20811Byte(p_comm, 0x3E, 0x7F);
			break;

		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x30, 0x15);
			writeCx20811Byte(p_comm, 0x31, 0x0F);
			writeCx20811Byte(p_comm, 0x33, 0xBF);
			writeCx20811Byte(p_comm, 0x3A, 0x00);
			writeCx20811Byte(p_comm, 0x3B, 0x03);
			writeCx20811Byte(p_comm, 0x3C, 0x08);
			writeCx20811Byte(p_comm, 0x3D, 0x0B);
			writeCx20811Byte(p_comm, 0x3E, 0x3F);
			break;
	}

	writeCx20811Byte(p_comm, 0x35, 0x80);
	writeCx20811Byte(p_comm, 0x36, 0x00);
	writeCx20811Byte(p_comm, 0x39, 0xCA);

	return 0;

}


int cx20811_initializeADC48K(const CX_COMMUNICATION *p_comm, int sample_bits)
{
	switch (sample_bits)
	{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x0A, 0x07);
			writeCx20811Byte(p_comm, 0x0A, 0x87);
			break;
		case CX20811_SAMPLE_BIT_16:
			// bclk = 3072khz -> 12.288mhz / 4
			writeCx20811Byte(p_comm, 0x0A, 0x03);
			writeCx20811Byte(p_comm, 0x0A, 0x83);
			break;
		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x0A, 0x01);
			writeCx20811Byte(p_comm, 0x0A, 0x81);
			break;
	}

	writeCx20811Byte(p_comm, 0x10, 0x00);
	writeCx20811Byte(p_comm, 0x11, 0x00);
	writeCx20811Byte(p_comm, 0x10, 0x1F);
	writeCx20811Byte(p_comm, 0x11, 0x4F);
	writeCx20811Byte(p_comm, 0x10, 0x5F);
	writeCx20811Byte(p_comm, 0xA0, 0x06);
	writeCx20811Byte(p_comm, 0xA7, 0x06);
	writeCx20811Byte(p_comm, 0xAE, 0x06);
	writeCx20811Byte(p_comm, 0xB5, 0x06);

	return 0;
}

int cx20811_initializeADC32K(const CX_COMMUNICATION *p_comm, int sample_bits)
{

	switch (sample_bits)
	{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x0A, 0x2F);
			writeCx20811Byte(p_comm, 0x0A, 0xAF);
			break;
		case CX20811_SAMPLE_BIT_16:
			// bclk = 2048khz = 12.288Mhz / 6
			writeCx20811Byte(p_comm, 0x0A, 0x05);   //  32k
			writeCx20811Byte(p_comm, 0x0A, 0x85);
			break;

		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x0A, 0x0B);
			writeCx20811Byte(p_comm, 0x0A, 0x8B);
			break;
	}

	writeCx20811Byte(p_comm, 0x10, 0x00);
	writeCx20811Byte(p_comm, 0x11, 0x00);
	writeCx20811Byte(p_comm, 0x10, 0x1F);
	writeCx20811Byte(p_comm, 0x11, 0x3F);   //32
	writeCx20811Byte(p_comm, 0x10, 0x5F);
	writeCx20811Byte(p_comm, 0xA0, 0x06);
	writeCx20811Byte(p_comm, 0xA7, 0x06);
	writeCx20811Byte(p_comm, 0xAE, 0x06);
	writeCx20811Byte(p_comm, 0xB5, 0x06);

	return 0;
}

int cx20811_initializeADC24K(const CX_COMMUNICATION *p_comm, int sample_bits)
{

	switch (sample_bits)
	{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x0A, 0x2F);
			writeCx20811Byte(p_comm, 0x0A, 0xAF);
			break;
		case CX20811_SAMPLE_BIT_16:
			// bclk = 1536khz = 12.288Mhz / 8
			writeCx20811Byte(p_comm, 0x0A, 0x07);   //  24k
			writeCx20811Byte(p_comm, 0x0A, 0x87);
			break;

		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x0A, 0x0B);
			writeCx20811Byte(p_comm, 0x0A, 0x8B);
			break;
	}

	writeCx20811Byte(p_comm, 0x10, 0x00);
	writeCx20811Byte(p_comm, 0x11, 0x00);
	writeCx20811Byte(p_comm, 0x10, 0x1F);
	writeCx20811Byte(p_comm, 0x11, 0x2F);   //24
	writeCx20811Byte(p_comm, 0x10, 0x5F);
	writeCx20811Byte(p_comm, 0xA0, 0x06);
	writeCx20811Byte(p_comm, 0xA7, 0x06);
	writeCx20811Byte(p_comm, 0xAE, 0x06);
	writeCx20811Byte(p_comm, 0xB5, 0x06);

	return 0;
}

int cx20811_initializeADC16K(const CX_COMMUNICATION *p_comm, int sample_bits)
{

	switch (sample_bits)
	{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x0A, 0x2F);
			writeCx20811Byte(p_comm, 0x0A, 0xAF);
			break;
		case CX20811_SAMPLE_BIT_16:
			// bclk = 1024khz = 12.288Mhz / 12
			writeCx20811Byte(p_comm, 0x0A, 0x0b); //16k
			writeCx20811Byte(p_comm, 0x0A, 0x8b);
			break;

		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x0A, 0x0B);
			writeCx20811Byte(p_comm, 0x0A, 0x8B);
			break;
	}

	writeCx20811Byte(p_comm, 0x10, 0x00);
	writeCx20811Byte(p_comm, 0x11, 0x00);
	writeCx20811Byte(p_comm, 0x10, 0x1F);
	writeCx20811Byte(p_comm, 0x11, 0x1F); //16
	writeCx20811Byte(p_comm, 0x10, 0x5F);
	writeCx20811Byte(p_comm, 0xA0, 0x06);
	writeCx20811Byte(p_comm, 0xA7, 0x06);
	writeCx20811Byte(p_comm, 0xAE, 0x06);
	writeCx20811Byte(p_comm, 0xB5, 0x06);

	return 0;
}

int cx20811_initializeADC8K(const CX_COMMUNICATION *p_comm, int sample_bits)
{
	switch (sample_bits)
	{
		case CX20811_SAMPLE_BIT_8:
			writeCx20811Byte(p_comm, 0x0A, 0x2F);
			writeCx20811Byte(p_comm, 0x0A, 0xAF);
			break;


		case CX20811_SAMPLE_BIT_16:
			// bclk = 512Khz -> 12.288Mhz/24
			writeCx20811Byte(p_comm, 0x0A, 0x17);
			writeCx20811Byte(p_comm, 0x0A, 0x97);
			break;

		case CX20811_SAMPLE_BIT_24:
			writeCx20811Byte(p_comm, 0x0A, 0x0B);
			writeCx20811Byte(p_comm, 0x0A, 0x8B);
			break;
	}

	writeCx20811Byte(p_comm, 0x10, 0x00);
	writeCx20811Byte(p_comm, 0x11, 0x00);
	writeCx20811Byte(p_comm, 0x10, 0x1F);
	writeCx20811Byte(p_comm, 0x11, 0x0F);
	writeCx20811Byte(p_comm, 0x10, 0x5F);
	writeCx20811Byte(p_comm, 0xA0, 0x06);
	writeCx20811Byte(p_comm, 0xA7, 0x06);
	writeCx20811Byte(p_comm, 0xAE, 0x06);
	writeCx20811Byte(p_comm, 0xB5, 0x06);

	return 0;
}

/***********************************************************************************/
/* Forward declarations                                                            */
/***********************************************************************************/


int set_ad_sample_rate(const CX_COMMUNICATION *p_comm)
{
    switch (ad_sample_rate) {
        case CX20811_SAMPLE_RATE_8000:
            cx20811_initializeADC8K(p_comm, adda_sample_size);
            break;
        case CX20811_SAMPLE_RATE_16000:
            cx20811_initializeADC16K(p_comm, adda_sample_size);
            break;
        case CX20811_SAMPLE_RATE_32000:
            printk("not yet support 32k\n");
            break;
        case CX20811_SAMPLE_RATE_44100:
            printk("not yet support 44.1k\n");
            break;
        case CX20811_SAMPLE_RATE_48000:
            cx20811_initializeADC48K(p_comm, adda_sample_size);
            break;
    }

    return 0;
}

int set_da_sample_rate(const CX_COMMUNICATION *p_comm)
{
    switch (da_sample_rate) {
        case CX20811_SAMPLE_RATE_8000:
            cx20811_initializeDAC8K(p_comm, adda_sample_size);
            break;
        case CX20811_SAMPLE_RATE_16000:
            cx20811_initializeDAC16K(p_comm, adda_sample_size);
            break;
        case CX20811_SAMPLE_RATE_32000:
            printk("not yet support 32k\n");
            break;
        case CX20811_SAMPLE_RATE_44100:
            printk("not yet support 44.1k\n");
            break;
        case CX20811_SAMPLE_RATE_48000:
            cx20811_initializeDAC48K(p_comm, adda_sample_size);
            break;
    }

    return 0;
}

/******************************************************************************
 *
 * cx20811_initialize()
 *
 ******************************************************************************/
int
cx20811_initialize(const CX_COMMUNICATION *p_comm)
{
	char value;
	cx20811_initializePLL(p_comm);
    // cx20811 as master, i2s as slave, tx/rx with same i2s
    cx20811_initializeDACI2SSlave(p_comm, CX20811_SAMPLE_BIT_16, CX20811_LEFT_CHANNEL);
    cx20811_initializeDAC8K(p_comm, CX20811_SAMPLE_BIT_16);
    cx20811_initializeADCMaster(p_comm, CX20811_SAMPLE_BIT_16);
    cx20811_initializeADC8K(p_comm, CX20811_SAMPLE_BIT_16);
#if 0
    // cx20811 as slave, i2s as master, tx/rx with same i2s
    cx20811_initializeDACI2SSlave(p_comm, CX20811_SAMPLE_BIT_16, CX20811_LEFT_CHANNEL);
    cx20811_initializeDAC8K(p_comm, CX20811_SAMPLE_BIT_16);
    cx20811_initializeADCSlave(p_comm, CX20811_SAMPLE_BIT_16);
    cx20811_initializeADC8K(p_comm, CX20811_SAMPLE_BIT_16);
#endif
	readCx20811Byte(p_comm, 0x28, &value);
	printk("cx20811_initialize: %x\n", value);
    return 0;
}

static unsigned int cx20811_write_i2c(unsigned id, unsigned register_addr, int value, int size)
{
    int ret = 0;
    ret = cx20811_i2c_write(id, register_addr, value, size);

    return ret;
}

static unsigned int cx20811_read_i2c(unsigned char id, unsigned short register_addr, void *buffer, int num_bytes)
{
    int value = 0;
    int ret = 0;
    unsigned char register_return_vaule = 0;

    ret = cx20811_i2c_read(id, register_addr, &value, num_bytes);
    if (ret < 0)
        return -1;
    register_return_vaule = value;

    memcpy(buffer, &register_return_vaule, 1);

    return 0;
}

static void cx20811_sleep(unsigned long msec)
{
    udelay(msec*4000);
}

static int initIbizaChip(void)
{
    int ret = 0;
    char value = 0;
	int i;
	CX_COMMUNICATION *p_chip;
    // check has cx20811 or not
    p_chip = &comm[0];
    ret = readCx20811Byte(p_chip, 0x78, &value);
    if (ret < 0) {
        printk("No cx20811!!\n");
        return -1;
    }

    for(i = 0; i < dev_num; i++)
    {
        p_chip = &comm[i];
        cx20811_initialize(p_chip);
    }

    return 1;
}

int cx20811_chip_init(void)
{
    int i = 0;
    int ret = TRUE;

    /* first check the parameters*/
    if (dev_num <= 0 || dev_num > 2)
    {
        printk("module param chip invalid value:%d\n", dev_num);
        return -1;
    }

    if(!init)
        goto exit;

    /* initize each cx20811*/
    for (i = 0; i < dev_num; i++)
    {
        //comm[i].i2c_addr = cx20811_i2c_addr[i];
        comm[i].i2c_addr = (u8)i;
        comm[i].p_handle = &comm[i];
        comm[i].write = cx20811_write_i2c;
        comm[i].read = cx20811_read_i2c;
        comm[i].sleep = cx20811_sleep;
    }

    ret = initIbizaChip();

exit:
    printk("CX20811 Init...%s!\n", ((ret == TRUE) ? "OK" : "Fail"));

    return ((ret == TRUE) ? 0 : -1);
}

static int cx20811_audio_set_output_ch(int decoder_idx, int chan_num)
{
    char value;
    CX_COMMUNICATION *p_comm;

    p_comm = &comm[decoder_idx];

    if (chan_num == 0xff) {
        writeCx20811Byte(p_comm, 0x31, 0x07);
        writeCx20811Byte(p_comm, 0x32, 0x07);
    }
    else {
        readCx20811Byte(p_comm, 0x31, &value);
        value &= 0x80;
        value |= (1 << 7);  //enable loopback
        value |= chan_num << 5;
        writeCx20811Byte(p_comm, 0x31, value);
        readCx20811Byte(p_comm, 0x32, &value);
        value &= 0x80;
        value |= (1 << 7);
        writeCx20811Byte(p_comm, 0x32, value);
    }
    return 0;
}

static int cx20811_sound_switch(int ssp_idx, int chan_num, bool is_on)
{
    const u32 single_decoder_chan_cnt  = 4;
    const u32 max_audio_chan_cnt   = 12;
    u32 decoder_idx = 0;

    if (chan_num >= max_audio_chan_cnt) {
        printk(KERN_WARNING "%s: channel number %d over max\n", __func__, chan_num);
        return -EACCES;
    }

    if (chan_num >= single_decoder_chan_cnt)
        chan_num -= (ssp_idx * single_decoder_chan_cnt);

    is_on ? cx20811_audio_set_output_ch(decoder_idx, chan_num) : cx20811_audio_set_output_ch(decoder_idx, 0xff);

    return 0;
}

static int __init cx20811_init(void)
{
    int i, ret;
    /* check device number */
    if(dev_num > MAX_CHIP_NUM) {
        printk("CX20811 dev_num=%d invalid!!(Max=%d)\n", dev_num, MAX_CHIP_NUM);
        return -1;
    }

    /* register i2c client for contol cx25930 */
    ret = cx20811_i2c_register();
    if(ret < 0)
        return -1;

    /* CX20811 init */
    for(i=0; i<dev_num; i++) {
        cx20811_dev[i].index = i;

        sprintf(cx20811_dev[i].name, "cx20811.%d", i);

    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        sema_init(&cx20811_dev[i].lock, 1);
    #else
        init_MUTEX(&cx20811_dev[i].lock);
    #endif
    }

    ret = cx20811_chip_init();
    //if(ret < 0)
      //  goto err;

    /* register audio function for platform used */
    cx20811_audio_funs.sound_switch = cx20811_sound_switch;
    cx20811_audio_funs.codec_name = cx20811_dev[0].name;
    plat_audio_register_function(&cx20811_audio_funs);

    return 0;

//err:
  //  cx20811_i2c_unregister();

    return ret;

}

static void __exit cx20811_exit(void)
{
    cx20811_i2c_unregister();
    plat_audio_deregister_function();
}

module_init(cx20811_init);
module_exit(cx20811_exit);

MODULE_DESCRIPTION("Grain Media CX20811 4CH AUDIO SDI Receiver Driver");
MODULE_AUTHOR("Grain Media Technology Corp.");
MODULE_LICENSE("GPL");
