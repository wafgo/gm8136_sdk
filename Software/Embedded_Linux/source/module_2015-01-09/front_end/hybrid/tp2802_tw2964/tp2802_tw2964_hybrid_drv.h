#ifndef _TP2802_TW2964_HYBRID_DRV_H_
#define _TP2802_TW2964_HYBRID_DRV_H_

#define CLKIN                          27000000
#define CH_IDENTIFY(id, tvi, vout)     (((id)&0xf)|(((tvi)&0xf)<<4)|(((vout)&0xf)<<8))
#define CH_IDENTIFY_ID(x)              ((x)&0xf)
#define CH_IDENTIFY_VIN(x)             (((x)>>4)&0xf)
#define CH_IDENTIFY_VOUT(x)            (((x)>>8)&0xf)

int tp2802_tw2964_i2c_write(u8, u8, u8); // (u8 addr, u8 reg, u8 data)
u8 tp2802_tw2964_i2c_read(u8, u8); // (u8 addr, u8 reg)
int tp2802_tw2964_i2c_read_ext(u8, u8); // (u8 addr, u8 reg)

#endif  /* _TP2802_TW2964_DRV_H_ */

