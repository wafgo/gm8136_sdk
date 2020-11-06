#ifndef __ADDA160_H__
#define __ADDA160_H__

#define GM8312_SSP_TEST

#define DEBUG_ADDA160			0
#define ENABLE_ADDA160_DEBUG_PORT	0
#define SSP_SLAVE_MODE			0

#define SSP3_CLK_SRC_12000K  0
/* for HDMI playback sample rate 48k/16 bits , ssp3 bclk must = 3072000, sdl&pdl must = 16
   so ssp3's mclk must = 24Mhz, adda160's MCLKMODE must = 256 --> adda160's MCLK = MCLKMODE(256)* 48000 = 12.288MHz
*/
#define SSP3_CLK_SRC_24000K  1

//debug helper
#if DEBUG_ADDA160
#define DEBUG_ADDA160_PRINT(FMT, ARGS...)    printk(FMT, ##ARGS)
#else
#define DEBUG_ADDA160_PRINT(FMT, ARGS...)
#endif

enum adda160_fs_frequency {
    ADDA160_FS_8K,
    ADDA160_FS_16K,
    ADDA160_FS_32K,
    ADDA160_FS_44_1K,
    ADDA160_FS_48K,
};

enum adda160_chip_select {
    ADDA160_FIRST_CHIP,
};

enum adda160_port_select {
    ADDA160_AD,
    ADDA160_DA,
};

enum adda160_i2s_select {
    ADDA160_FROM_PAD,
    ADDA160_FROM_SSP,
};

void adda160_I2S_mux(enum adda160_port_select ps, int mux);
void adda160_init(const enum adda160_port_select ps);
void adda160_set_fs(const enum adda160_chip_select cs, const enum adda160_fs_frequency fs);
void adda160_set_fs_ad_da(const enum adda160_port_select ps, const enum adda160_fs_frequency fs);
#endif //end of __ADDA160_H__
