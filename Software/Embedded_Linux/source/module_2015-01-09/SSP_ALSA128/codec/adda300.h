/*
 * $Author: slchen $
 * $Date: 2011/08/25 07:12:55 $
 *
 * $Log: adda300.h,v $
 * Revision 1.4  2011/08/25 07:12:55  slchen
 * *** empty log message ***
 *
 * Revision 1.3  2010/08/10 09:52:50  mars_ch
 * *: add cat6612 audio codec and code clean for adda300
 *
 * Revision 1.2  2010/08/04 08:13:07  mars_ch
 * *: max MICIN path
 *
 * Revision 1.1  2010/07/16 09:06:08  mars_ch
 * +: add 8126 platform configuration and adda300 codec driver
 *
 */

#ifndef __ADDA300_H__
#define __ADDA300_H__

#define GM8312_SSP_TEST

#define DEBUG_ADDA300			0
#define ENABLE_ADDA300_DEBUG_PORT	0
#define SSP_SLAVE_MODE			1
#define ADDA300_LINE_IN			1

#define SSP3_CLK_SRC_12000K  1
/* for HDMI playback sample rate 48k/16 bits , ssp3 bclk must = 3072000, sdl&pdl must = 16
   so ssp3's mclk must = 24Mhz, adda300's MCLKMODE must = 256 --> adda300's MCLK = MCLKMODE(256)* 48000 = 12.288MHz
*/
#define SSP3_CLK_SRC_24000K  0

//debug helper
#if DEBUG_ADDA300
#define DEBUG_ADDA300_PRINT(FMT, ARGS...)    printk(FMT, ##ARGS)
#else
#define DEBUG_ADDA300_PRINT(FMT, ARGS...)
#endif

enum adda300_fs_frequency {
    ADDA300_FS_8K,
    ADDA300_FS_16K,
    ADDA300_FS_32K,
    ADDA300_FS_44_1K,
    ADDA300_FS_48K,
};

enum adda300_chip_select {
    ADDA300_FIRST_CHIP,
};

enum adda300_port_select {
    ADDA300_AD,
    ADDA300_DA,
};

enum adda300_i2s_select {
    ADDA300_FROM_PAD,
    ADDA300_FROM_SSP,
};

void adda300_I2S_mux(enum adda300_port_select ps, int mux);
void adda300_init(const enum adda300_port_select ps);
void adda300_set_fs(const enum adda300_chip_select cs, const enum adda300_fs_frequency fs);
void adda300_set_fs_ad_da(const enum adda300_port_select ps, const enum adda300_fs_frequency fs);
#endif //end of __ADDA300_H__
