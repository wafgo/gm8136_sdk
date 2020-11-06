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
#include <common.h>
#include <command.h>
#include <config.h>
#include <asm/io.h>
#include <i2c.h>
#include "hdmitx.h"

#define printk  printf

extern BYTE I2CADR ;
extern BYTE I2CDEV ;

#define CAT6611_I2CADR      0x98
#define CAT6611_I2C_DIV     100*1000 /* standard clock 100K */
#define iowrite32   writel
#define ioread32    readl

#define __u8    unsigned char
#define __u16   unsigned short

struct i2c_msg {
	unsigned short addr;	/* slave address			*/
	unsigned short flags;
#define I2C_M_TEN		0x0010	/* this is a ten bit chip address */
#define I2C_M_RD		0x0001	/* read data, from slave to master */
#define I2C_M_NOSTART		0x4000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_REV_DIR_ADDR	0x2000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK	0x1000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NO_RD_ACK		0x0800	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_RECV_LEN		0x0400	/* length will be first received byte */
	unsigned short len;		/* msg length				*/
	unsigned char *buf;		/* pointer to msg data			*/
};

struct ftiic010 {
	void *base;
	int  ack;
};

#define FTIIC010_OFFSET_CR	0x00
#define FTIIC010_OFFSET_SR	0x04
#define FTIIC010_OFFSET_DR	0x0c

/* status */
#define FTIIC010_SR_DT		(1 << 4)
#define FTIIC010_SR_DR		(1 << 5)

#define FTIIC010_DR_MASK	0xff

#define MAX_RETRY   1000

/*
 * Control Register
 */
#define FTIIC010_CR_I2C_EN	(1 << 1)
#define FTIIC010_CR_SCL_EN	(1 << 2)
#define FTIIC010_CR_START	(1 << 4)
#define FTIIC010_CR_STOP	(1 << 5)
#define FTIIC010_CR_NAK		(1 << 6)
#define FTIIC010_CR_TB_EN	(1 << 7)
#define FTIIC010_CR_BERRI_EN	(1 << 10)
#define FTIIC010_CR_ALI_EN	(1 << 13)	/* arbitration lost */


static void ftiic010_set_cr(struct ftiic010 *ftiic010, int start,
		int stop, int nak)
{
	unsigned int cr;

	cr = FTIIC010_CR_I2C_EN
	   | FTIIC010_CR_SCL_EN
	   | FTIIC010_CR_TB_EN
	   | FTIIC010_CR_BERRI_EN
	   | FTIIC010_CR_ALI_EN;

	if (start)
		cr |= FTIIC010_CR_START;

	if (stop)
		cr |= FTIIC010_CR_STOP;

	if (nak)
		cr |= FTIIC010_CR_NAK;

	iowrite32(cr, ftiic010->base + FTIIC010_OFFSET_CR);
}

static int ftiic010_tx_byte(struct ftiic010 *ftiic010, __u8 data, int start,
		int stop)
{
	iowrite32(data, ftiic010->base + FTIIC010_OFFSET_DR);

	ftiic010->ack = 0;
	ftiic010_set_cr(ftiic010, start, stop, 0);


    {
        int i = 0;

        for (i = 0; i < 100; i++)
        {
            unsigned int status;

            status = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
            if (status & FTIIC010_SR_DT)
                return 0;

            udelay(1);
        }

        //ftiic010_hw_init(ftiic010);
    }
	return -1;
}

static int ftiic010_rx_byte(struct ftiic010 *ftiic010, int stop, int nak)
{
	ftiic010->ack = 0;
	ftiic010_set_cr(ftiic010, 0, stop, nak);

    {
        int i = 0;
        for (i = 0; i < MAX_RETRY; i++)
        {
            unsigned int status;

            status = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
            if (status & FTIIC010_SR_DR)
                return ioread32(ftiic010->base + FTIIC010_OFFSET_DR) & FTIIC010_DR_MASK;

            udelay(1);
        }

        printf("I2C: Failed to receive!\n");

        //ftiic010_hw_init(ftiic010);
    }
	return -1;
}

static int ftiic010_tx_msg(struct ftiic010 *ftiic010,
		struct i2c_msg *msg, int last)
{
	__u8 data;
	int i, ret;

	data = (msg->addr & 0x7f) << 1 | 0; /* write */
	ret = ftiic010_tx_byte(ftiic010, data, 1, 0);
	if(unlikely(ret < 0)){
		return ret;
	}

	for (i = 0; i < msg->len; i++) {
		int stop = 0;

		if (last && (i + 1 == msg->len))
			stop = 1;

		ret = ftiic010_tx_byte(ftiic010, msg->buf[i], 0, stop);
		if(unlikely(ret < 0)){
			return ret;
		}
	}

	return 0;
}

static int ftiic010_rx_msg(struct ftiic010 *ftiic010,
		struct i2c_msg *msg, int last)
{
	__u8 data;
	int i, ret;

	data = (msg->addr & 0x7f) << 1 | 1; /* read */
	ret = ftiic010_tx_byte(ftiic010, data, 1, 0);
	if (unlikely(ret < 0)){
		return ret;
	}

	for (i = 0; i < msg->len; i++) {
		int nak = 0;
		int stop = 0;

		if (i + 1 == msg->len){
			if(last){
				stop = 1;
			}
			nak = 1;
		}

		ret = ftiic010_rx_byte(ftiic010, stop, nak);
		if (unlikely(ret < 0)){
			return ret;
		}

		msg->buf[i] = ret;
	}

	return 0;
}

static int ftiic010_do_msg(struct ftiic010 *ftiic010,
		struct i2c_msg *msg, int last)
{
	if (msg->flags & I2C_M_RD)
		return ftiic010_rx_msg(ftiic010, msg, last);
	else
		return ftiic010_tx_msg(ftiic010, msg, last);
}

int ftiic010_master_xfer(struct i2c_msg *msgs, int num)
{
    struct ftiic010 ftiic010_data;
	int i;

    memset(&ftiic010_data, 0, sizeof(struct ftiic010));
    ftiic010_data.base = (void *)CONFIG_I2C_BASE;

	for (i = 0; i < num; i++) {
		int ret, last = 0;

		if (i == num - 1)
			last = 1;

		ret = ftiic010_do_msg(&ftiic010_data, &msgs[i], last);
		if (ret < 0)
	     	return ret;
	}
	return num;
}

/* ER_SUCCESS = 0, ER_FAIL = 1 */
SYS_STATUS HDMITX_WriteI2C_Byte(SHORT RegAddr, BYTE Data)
{
    struct i2c_msg  msgs[1];
	unsigned char   buf[2];

	buf[0]          = RegAddr;
	buf[1]          = Data;
	msgs[0].addr    = CAT6611_I2CADR >> 1;
	msgs[0].flags   = 0; /* write */
	msgs[0].len     = 2;
	msgs[0].buf     = &buf[0];

    if (unlikely(ftiic010_master_xfer(msgs, 1) != 1)) {
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

	if (unlikely(ftiic010_master_xfer(msgs, 1) != 1)) {
		return ER_FAIL;
	}

    return (retVal == 0) ? ER_SUCCESS : ER_FAIL;
}

BYTE HDMITX_ReadI2C_Byte(SHORT RegAddr)
{
    struct i2c_msg  msgs[2];
	unsigned char   buf[2];

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

	if (unlikely(ftiic010_master_xfer(msgs, 2) != 2)) {
		return 0;
	}

    return buf[1];
}

SYS_STATUS HDMITX_ReadI2C_ByteN(SHORT RegAddr, BYTE *pData, int N)
{
    struct i2c_msg  msgs[2];
	int             i, retVal = 0;
	unsigned char   buf[1];

	buf[0]          = RegAddr;
	msgs[0].addr    = CAT6611_I2CADR >> 1;
	msgs[0].flags   = 0; /* write */
	msgs[0].len     = 1;
	msgs[0].buf     = &buf[0];

	msgs[1].addr    = CAT6611_I2CADR >> 1;
	msgs[1].flags   = I2C_M_RD;
	msgs[1].len     = N;
	msgs[1].buf     = pData;

	if (unlikely(ftiic010_master_xfer(msgs, 2) != 2)) {
		return ER_FAIL;
	}

    return (retVal == 0) ? ER_SUCCESS : ER_FAIL;
}
