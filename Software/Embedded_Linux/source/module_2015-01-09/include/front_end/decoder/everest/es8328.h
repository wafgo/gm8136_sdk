#ifndef _FE_DEC_ES8328_H_
#define _FE_DEC_ES8328_H_

#include <linux/ioctl.h>

/*************************************************************************************
 *  ES8328 Device Definition
 *************************************************************************************/
#define ES8328_DEV_MAX                 2

typedef enum {
    AUDIO_SAMPLERATE_8K = 0,
    AUDIO_SAMPLERATE_16K,
}ES8328_SAMPLERATE_T;

typedef enum {
    AUDIO_BITS_16B = 0,
    AUDIO_BITS_8B,
}ES8328_SAMPLESIZE_T;

typedef struct {
    ES8328_SAMPLERATE_T sample_rate;
    ES8328_SAMPLESIZE_T sample_size;
    int ch_num;
} ES8328_AUDIO_INIT_T;

/*************************************************************************************
 *  ES8328 IOCTL
 *************************************************************************************/
struct es8328_ioc_data {
    int ch;         ///< access channel number ==> vin index
    int data;       ///< read/write data value
};

#define ES8328_IOC_MAGIC       'n'

#define ES8328_GET_NOVID       _IOR(ES8328_IOC_MAGIC,   1, struct es8328_ioc_data)

#define ES8328_GET_MODE   		_IOR(ES8328_IOC_MAGIC,   2, int)

#define ES8328_GET_CONTRAST    _IOR(ES8328_IOC_MAGIC,   3, struct es8328_ioc_data)
#define ES8328_SET_CONTRAST    _IOWR(ES8328_IOC_MAGIC,  4, struct es8328_ioc_data)

#define ES8328_GET_BRIGHTNESS  _IOR(ES8328_IOC_MAGIC,   5, struct es8328_ioc_data)
#define ES8328_SET_BRIGHTNESS  _IOWR(ES8328_IOC_MAGIC,  6, struct es8328_ioc_data)

#define ES8328_GET_SATURATION  _IOR(ES8328_IOC_MAGIC,   7, struct es8328_ioc_data)
#define ES8328_SET_SATURATION  _IOWR(ES8328_IOC_MAGIC,  8, struct es8328_ioc_data)

#define ES8328_GET_HUE         _IOR(ES8328_IOC_MAGIC,   9, struct es8328_ioc_data)
#define ES8328_SET_HUE         _IOWR(ES8328_IOC_MAGIC, 10, struct es8328_ioc_data)

#define ES8328_GET_VOL         _IOR(ES8328_IOC_MAGIC,  11, int)
#define ES8328_SET_VOL         _IOWR(ES8328_IOC_MAGIC, 12, int)

#define ES8328_GET_OUT_CH      _IOR(ES8328_IOC_MAGIC,  13, int)
#define ES8328_SET_OUT_CH      _IOWR(ES8328_IOC_MAGIC, 14, int)

/*************************************************************************************
 *  ES8328 API Export Function Prototype
 *************************************************************************************/
#ifdef __KERNEL__
int es8328_get_device_num(void);

u8  es8328_i2c_read(u8 id, u8 reg);
int es8328_i2c_write(u8 id, u8 reg, u8 data);
int es8328_i2c_array_write(u8 id, u8 addr, u8 *parray, u32 array_cnt);

int es8328_audio_set_mode(int id, ES8328_SAMPLESIZE_T samplesize, ES8328_SAMPLERATE_T samplerate);
int es8328_audio_set_mute(int id, int on);
int es8328_audio_get_mute(int id);
int es8328_audio_set_volume(int id, int volume);
int es8328_audio_get_output_ch(int id);
int es8328_audio_set_output_ch(int id, int ch);
#endif

#endif  /* _FE_DEC_ES8328_H_ */
