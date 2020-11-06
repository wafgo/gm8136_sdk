/**
 * @file es8328_lib.c
 * EVEREST ES8328 2-CH Audio Codecs Library
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <mach/ftpmu010.h>

#include "everest/es8328.h"        ///< from module/include/front_end/decoder

extern int audio_chnum;

static int es8328_audio_init(int id, int ch_num)
{
    int ret;

    ret = es8328_i2c_write(id, 0x08, 0x85); if(ret < 0) goto exit;      ///< master mode/mclkdiv2=0/bclkdiv=6(SCLK ratio)
    //ret = es8328_i2c_write(id, 0x08, 0x91); if(ret < 0) goto exit;      ///< master mode/mclkdiv2=0/bclkdiv=6(SCLK ratio)
                                                                        /// record/playback 8k --> SCLK ratio = MCLK/6
    ret = es8328_i2c_write(id, 0x2b, 0x80); if(ret < 0) goto exit;      /// ADC & DAC use same bit clock, use DAC LRCK
    ret = es8328_i2c_write(id, 0x00, 0x32); if(ret < 0) goto exit;      ///< sameFs/DACMCLK is master clock source/disable Ref
    ret = es8328_i2c_write(id, 0x01, 0x72); if(ret < 0) goto exit;      ///< Audio input gain init - Gain 1.0
    ret = es8328_i2c_write(id, 0x03, 0x00); if(ret < 0) goto exit;      /// ADC power on
    ret = es8328_i2c_write(id, 0x04, 0x3c); if(ret < 0) goto exit;      /// DAC power on/LROUT1/2 enable

    // ADC control
    ret = es8328_i2c_write(id, 0x09, 0x00); if(ret < 0) goto exit;      /// ADC left channel gain = 0dB
    ret = es8328_i2c_write(id, 0x0A, 0x00); if(ret < 0) goto exit;      /// L/RINPUT1
    //ret = es8328_i2c_write(id, 0x0A, 0x50); if(ret < 0) goto exit;      /// L/RINPUT2
    ret = es8328_i2c_write(id, 0x0C, 0x0c); if(ret < 0) goto exit;      /// I2S 16bits, data = left/right ADC
    //ret = es8328_i2c_write(id, 0x0C, 0x4c); if(ret < 0) goto exit;      /// I2S 16bits, data = left/left ADC

    ret = es8328_i2c_write(id, 0x0D, 0x0a); if(ret < 0) goto exit;      /// ADCFsRatio = 1536 : 8KHz = MCLK/1536
    ret = es8328_i2c_write(id, 0x0F, 0x70); if(ret < 0) goto exit;      /// Softramp enable and Ramp rate =0.5db/8LRCKs ??
    ret = es8328_i2c_write(id, 0x10, 0x00); if(ret < 0) goto exit;      /// left volume 0db
    ret = es8328_i2c_write(id, 0x11, 0x00); if(ret < 0) goto exit;      /// right volume 0db

    // DAC control
    ret = es8328_i2c_write(id, 0x17, 0x18); if(ret < 0) goto exit;      /// DAC I2S 16 bits
    ret = es8328_i2c_write(id, 0x18, 0x0a); if(ret < 0) goto exit;      /// DACFsRatio = 1536 : 8KHz = MCLK/1536
    ret = es8328_i2c_write(id, 0x19, 0x70); if(ret < 0) goto exit;      /// SOFT RAMP RATE=32LRCKS/STEP,Enable ZERO-CROSS CHECK,DAC un-mute
    ret = es8328_i2c_write(id, 0x1a, 0x18); if(ret < 0) goto exit;      /// DAC Left volume 0db
    ret = es8328_i2c_write(id, 0x1b, 0x18); if(ret < 0) goto exit;      /// DAC right volume 0db

    //ret = es8328_i2c_write(id, 0x26, 0x00); if(ret < 0) goto exit;      /// select LIN1/RIN1 for output mix
    ret = es8328_i2c_write(id, 0x26, 0x12); if(ret < 0) goto exit;      /// reserved
    //ret = es8328_i2c_write(id, 0x27, 0xd0); if(ret < 0) goto exit;      /// left DAC & LIN to left mixer
    ret = es8328_i2c_write(id, 0x27, 0x90); if(ret < 0) goto exit;      /// left DAC to left mixer
    ret = es8328_i2c_write(id, 0x28, 0x38); if(ret < 0) goto exit;      /// disable right DAC/RIN to left mixer
    ret = es8328_i2c_write(id, 0x29, 0x38); if(ret < 0) goto exit;      /// disable left DAC/LIN to right mixer
    //ret = es8328_i2c_write(id, 0x2A, 0xd0); if(ret < 0) goto exit;      /// right DAC & RIN to right mixer
    ret = es8328_i2c_write(id, 0x2A, 0x90); if(ret < 0) goto exit;      /// right DAC to right mixer

    ret = es8328_i2c_write(id, 0x2E, 0x1b); if(ret < 0) goto exit;      /// LOUT1 volume = -3db
    ret = es8328_i2c_write(id, 0x2F, 0x1b); if(ret < 0) goto exit;      /// ROUT1 volume = -3db
    ret = es8328_i2c_write(id, 0x30, 0x1b); if(ret < 0) goto exit;      /// LOUT2 volume = -3db
    ret = es8328_i2c_write(id, 0x31, 0x1b); if(ret < 0) goto exit;      /// ROUT2 volume = -3db

    mdelay(200);

    ret = es8328_i2c_write(id, 0x02, 0x00); if(ret < 0) goto exit;      /// power on DEM STM

    printk("ES8328#%d init done\n", id);


exit:
    return ret;
}

/* setting sample rate and bit clock
 * SCLK ratio is bit clock divider
 * bit clock = mclk/sclk ratio
 * Everest FAE suggest SCLK ratio better to be 6/12/24
 * for sample rate 8k, mclk = 12.288, sclk = 6, bit clock = 12.288Mhz/6 = 2.048Mhz
 * for sample rate 16k, mclk = 12.288, sclk = 6, bit clock = 12.288Mhz/6 = 2.048Mhz
 * sample rate = 8k
 * ADC frame sync = MCLK/ADCFsRatio = 12.288MHz/1536 = 8k
 * DAC frame sync = MCLK/DACFsRatio = 12.288MHz/1536 = 8k
 * sample rate = 16k
 * ADC frame sync = MCLK/ADCFsRatio = 12.288MHz/768 = 16k
 * DAC frame sync = MCLK/DACFsRatio = 12.288MHz/768 = 16k
*/
static int es8328_audio_set_sample_rate(int id, ES8328_SAMPLERATE_T samplerate)
{
    int ret = 0;

    if (samplerate == AUDIO_SAMPLERATE_8K) {
        ret = es8328_i2c_write(id, 0x08, 0x85); if(ret < 0) goto exit;      ///< master mode/mclkdiv2=0/bclkdiv=6(SCLK ratio)
        ret = es8328_i2c_write(id, 0x0D, 0x0a); if(ret < 0) goto exit;      /// ADCFsRatio = 1536 : 8KHz = MCLK/1536
        ret = es8328_i2c_write(id, 0x18, 0x0a); if(ret < 0) goto exit;      /// DACFsRatio = 1536 : 8KHz = MCLK/1536
    }

    if (samplerate == AUDIO_SAMPLERATE_16K) {
        ret = es8328_i2c_write(id, 0x08, 0x85); if(ret < 0) goto exit;      ///< master mode/mclkdiv2=0/bclkdiv=6(SCLK ratio)
        ret = es8328_i2c_write(id, 0x0D, 0x06); if(ret < 0) goto exit;      /// ADCFsRatio = 768 : 8KHz = MCLK/768
        ret = es8328_i2c_write(id, 0x18, 0x06); if(ret < 0) goto exit;      /// DACFsRatio = 768 : 8KHz = MCLK/768
    }

exit:
    return ret;
}

int es8328_audio_set_mute(int id, int on)
{
    int ret;

    if (on) {
        ret = es8328_i2c_write(id, 0x2e, 0x0); if(ret < 0) goto exit;     ///< LOUT1 volume = 0
        ret = es8328_i2c_write(id, 0x2f, 0x0); if(ret < 0) goto exit;     ///< ROUT1 volume = 0
        ret = es8328_i2c_write(id, 0x30, 0x0); if(ret < 0) goto exit;     ///< LOUT2 volume = 0
        ret = es8328_i2c_write(id, 0x31, 0x0); if(ret < 0) goto exit;     ///< ROUT2 volume = 0
    }
    else {
        ret = es8328_i2c_write(id, 0x2e, 0x1b); if(ret < 0) goto exit;     ///< LOUT1 volume = -3db
        ret = es8328_i2c_write(id, 0x2f, 0x1b); if(ret < 0) goto exit;     ///< ROUT1 volume = -3db
        ret = es8328_i2c_write(id, 0x30, 0x1b); if(ret < 0) goto exit;     ///< LOUT2 volume = -3db
        ret = es8328_i2c_write(id, 0x31, 0x1b); if(ret < 0) goto exit;     ///< ROUT2 volume = -3db
    }

exit:
    return ret;
}
EXPORT_SYMBOL(es8328_audio_set_mute);

int es8328_audio_get_mute(int id)
{
    int volume = 0;

    volume = es8328_i2c_read(id, 0x2e);

    if (volume != 0)
        volume = ((volume - 2) >> 1);

    return volume;
}
EXPORT_SYMBOL(es8328_audio_get_mute);

int es8328_audio_set_volume(int id, int volume)
{
    int ret;

    if (volume == 0) {
        ret = es8328_i2c_write(id, 0x2e, 0x0); if(ret < 0) goto exit;     ///< LOUT1 volume
        ret = es8328_i2c_write(id, 0x2f, 0x0); if(ret < 0) goto exit;     ///< ROUT1 volume
        ret = es8328_i2c_write(id, 0x30, 0x0); if(ret < 0) goto exit;     ///< LOUT2 volume
        ret = es8328_i2c_write(id, 0x31, 0x0); if(ret < 0) goto exit;     ///< ROUT2 volume
    }
    else {
        volume = 0x2 + (volume << 1);
        ret = es8328_i2c_write(id, 0x2e, volume); if(ret < 0) goto exit;     ///< LOUT1 volume
        ret = es8328_i2c_write(id, 0x2f, volume); if(ret < 0) goto exit;     ///< ROUT1 volume
        ret = es8328_i2c_write(id, 0x30, volume); if(ret < 0) goto exit;     ///< LOUT2 volume
        ret = es8328_i2c_write(id, 0x31, volume); if(ret < 0) goto exit;     ///< ROUT2 volume
    }

exit:
    return ret;
}
EXPORT_SYMBOL(es8328_audio_set_volume);

int es8328_audio_get_output_ch(int id)
{
    int data;
    int ch = 0;

    data = es8328_i2c_read(id, 0x26);

    if (data == 0x0)
        ch = 0;
    if (data == 0x9)
        ch = 1;
    if (data == 0x12)
        ch = 16;
    if (data != 0x0 && data != 0x9 && data!= 0x12)
        ch = 25;

    return ch;
}
EXPORT_SYMBOL(es8328_audio_get_output_ch);

int es8328_audio_set_output_ch(int id, int ch)
{
    int ret;

    if (ch != 0 && ch != 1 && ch != 0x10) {
        printk("[Error] ES8328 only support 2 channel\n");
        ret = -1;
    }

    if (ch == 0) {
        ret = es8328_i2c_write(id, 0x26, 0x00); if(ret < 0) goto exit;              ///< LMIXSEL/RMIXSEL = LIN1/RIN1
        ret = es8328_i2c_write(id, 0x27, 0x50); if(ret < 0) goto exit;              ///< LIN signal to left mixer
        ret = es8328_i2c_write(id, 0x2a, 0x50); if(ret < 0) goto exit;              ///< RIN signal to right mixer
    }

    if (ch == 1) {
        ret = es8328_i2c_write(id, 0x26, 0x09); if(ret < 0) goto exit;              ///< LMIXSEL/RMIXSEL = LIN2/RIN2
        ret = es8328_i2c_write(id, 0x27, 0x50); if(ret < 0) goto exit;              ///< LIN signal to left mixer
        ret = es8328_i2c_write(id, 0x2a, 0x50); if(ret < 0) goto exit;              ///< RIN signal to right mixer
    }

    if (ch == 0x10) {
        ret = es8328_i2c_write(id, 0x26, 0x12); if(ret < 0) goto exit;              ///< LMIXSEL/RMIXSEL = reserved
        ret = es8328_i2c_write(id, 0x27, 0x90); if(ret < 0) goto exit;              ///< LIN signal to left mixer
        ret = es8328_i2c_write(id, 0x2a, 0x90); if(ret < 0) goto exit;              ///< RIN signal to right mixer
    }

exit:
    return ret;
}
EXPORT_SYMBOL(es8328_audio_set_output_ch);

int es8328_audio_set_play_ch(int id, unsigned char ch)
{
    int ret;

    ret = es8328_i2c_write(id, 0xff, 0x01); if(ret < 0) goto exit;          ///< select bank 1
    ret = es8328_i2c_write(id, 0x14, (ch & 0x0F)); if(ret < 0) goto exit;

exit:
    return ret;
}
EXPORT_SYMBOL(es8328_audio_set_play_ch);

int es8328_audio_set_mode(int id, ES8328_SAMPLESIZE_T samplesize, ES8328_SAMPLERATE_T samplerate)
{
    int ch_num = 0;
    int ret;

    ch_num = audio_chnum;
    if(ch_num < 0)
        return -1;

    /* es8328 audio initialize */
    ret = es8328_audio_init(id, ch_num); if(ret < 0) goto exit;

    /* set audio sample rate */
    ret = es8328_audio_set_sample_rate(id, samplerate); if(ret < 0) goto exit;

exit:
    return ret;
}
EXPORT_SYMBOL(es8328_audio_set_mode);
