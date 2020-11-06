#ifndef _VCAP_I2C_H_
#define _VCAP_I2C_H_

#include <linux/i2c.h>

int vcap_i2c_transfer(struct i2c_msg *msgs, int num);
int vcap_i2c_array_write(ushort iaddr, char *sa_tokens[], int delay);

#endif
