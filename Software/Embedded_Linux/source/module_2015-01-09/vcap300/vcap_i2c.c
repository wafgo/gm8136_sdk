/**
 * @file vcap_i2c.c
 * VCAP300 I2C Common Interface
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.3 $
 * $Date: 2012/12/11 09:10:05 $
 *
 * ChangeLog:
 *  $Log: vcap_i2c.c,v $
 *  Revision 1.3  2012/12/11 09:10:05  jerson_l
 *  1. modify code indentation
 *
 *  Revision 1.2  2012/10/24 06:47:51  jerson_l
 *
 *  switch file to unix format
 *
 *  Revision 1.1.1.1  2012/10/23 08:16:31  jerson_l
 *  import vcap300 driver
 *
 */

#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "vcap_dbg.h"

static struct i2c_client *vcap_i2c_client = NULL;

#define VCAP_XTOD(c)    ((c) <= '9' ? (c) - '0' : (c) <= 'F' ? (c) - 'A' + 10 : (c) - 'a' + 10)

static int vcap_substring(char **ptr, char *string, char *pattern)
{
    register int i;

    ptr[0] = (char *) strsep(&string, pattern);
    for (i = 0; ptr[i] != NULL; ++i) {
        ptr[i + 1] = strsep(&string, pattern);
    }
    return i;
}

static long vcap_atoi(register char *p)
{
    register long n;
    register int c, neg = 0;

    if (p == NULL)
        return 0;

    if (!isdigit(c = *p)) {
        while (isspace(c))
            c = *++p;
        switch (c) {
        case '-':
            neg++;
        case '+':   /* fall-through */
            c = *++p;
        }
        if (!isdigit(c))
            return (0);
    }
    if (c == '0' && *(p + 1) == 'x') {
        p += 2;
        c = *p;
        n = VCAP_XTOD(c);

        while ((c = *++p) && isxdigit(c)) {
            n *= 16;    /* two steps to avoid unnecessary overflow */
            n += VCAP_XTOD(c);  /* accum neg to avoid surprises at MAX */
        }
    } else {
        n = '0' - c;
        while ((c = *++p) && isdigit(c)) {
            n *= 10;    /* two steps to avoid unnecessary overflow */
            n += '0' - c;   /* accum neg to avoid surprises at MAX */
        }
    }
    return (neg ? -n : n);
}

static int vcap_i2c_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
    vcap_i2c_client = client;
    return 0;
}

static int vcap_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id vcap_i2c_id[] = {
    { "vcap_i2c", 0 },
    { }
};

static struct i2c_driver vcap_i2c_driver = {
    .driver = {
        .name  = "VCAP_I2C",
        .owner = THIS_MODULE,
    },
    .probe    = vcap_i2c_probe,
    .remove   = vcap_i2c_remove,
    .id_table = vcap_i2c_id
};

int vcap_i2c_register(void)
{
    int ret;
    struct i2c_board_info info;
    struct i2c_adapter    *adapter;
    struct i2c_client     *client;

    /* add i2c driver */
    ret = i2c_add_driver(&vcap_i2c_driver);
    if(ret < 0) {
        vcap_err("can't add i2c driver!\n");
        return ret;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = 0x7f;
    strlcpy(info.type, "vcap_i2c", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(0);   ///< I2C BUS#0
    if (!adapter) {
        vcap_err("can't get i2c adapter\n");
        goto err_driver;
    }

    /* add i2c device */
    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if(!client) {
        vcap_err("can't add i2c device!\n");
        goto err_driver;
    }

    return 0;

err_driver:
   i2c_del_driver(&vcap_i2c_driver);
   return -ENODEV;
}

void vcap_i2c_unregister(void)
{
    i2c_unregister_device(vcap_i2c_client);
    i2c_del_driver(&vcap_i2c_driver);
    vcap_i2c_client = NULL;
}

int vcap_i2c_transfer(struct i2c_msg *msgs, int num)
{
    struct i2c_adapter *adapter;

    if(!vcap_i2c_client) {
        vcap_err("i2c_client not register\n");
        return -1;
    }

    adapter = to_i2c_adapter(vcap_i2c_client->dev.parent);

    if (unlikely(i2c_transfer(adapter, msgs, num) != num)) {
        vcap_err("i2c transfer failed!\n");
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(vcap_i2c_transfer);

int vcap_i2c_array_write(ushort iaddr, char *sa_tokens[], int delay)
{
    int  i, j;
    int  argc;
    char *argv[30];
    char buffer[256];
    unsigned char tokenvalue[30];
    struct i2c_msg msgs[2];
    struct i2c_adapter *adapter;

    if(!vcap_i2c_client) {
        vcap_err("i2c_client not register\n");
        return -1;
    }

    adapter = to_i2c_adapter(vcap_i2c_client->dev.parent);

    for(i=0; sa_tokens[i]!=0; ++i) {
        strcpy(buffer, sa_tokens[i]);
        argc = vcap_substring(argv, buffer, " r\n\t");

        if(argc > 0) {  // has data
            for(j=0; j<argc; ++j)
                tokenvalue[j] = (unsigned char)vcap_atoi(argv[j]);

            msgs[0].addr  = iaddr >> 1;
            msgs[0].flags = 0;
            msgs[0].len   = argc;
            msgs[0].buf   = tokenvalue;

            if(i2c_transfer(adapter, &msgs[0], 1) != 1){
                vcap_err("i2c transfer failed!\n");
                return -1;
            }

            if(delay) {
                if(in_interrupt()){
                    udelay(delay);
                }
                else {
                    wait_queue_head_t t_wait_queue;
                    init_waitqueue_head(&t_wait_queue);
                    wait_event_timeout(t_wait_queue, 1 == 0, usecs_to_jiffies(delay));
                }
            }
        }
    }

    return 0;
}
EXPORT_SYMBOL(vcap_i2c_array_write);
