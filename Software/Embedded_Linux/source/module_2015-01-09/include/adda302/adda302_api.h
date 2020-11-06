#ifndef __ADDA302_API_H__
#define __ADDA302_API_H__

#define DEBUG_ADDA302			0
#define ENABLE_ADDA302_DEBUG_PORT	0
#define SSP_SLAVE_MODE			0
#define ADDA302_LINE_IN			0
#define SAME_I2S_RXTX           0

enum adda302_fs_frequency {
    ADDA302_FS_8K,
    ADDA302_FS_16K,
    ADDA302_FS_22_05K,
    ADDA302_FS_32K,
    ADDA302_FS_44_1K,
    ADDA302_FS_48K,
};

enum adda302_chip_select {
    ADDA302_FIRST_CHIP,
};

enum adda302_port_select {
    ADDA302_AD,
    ADDA302_DA,
};

enum adda302_i2s_select {
    ADDA302_FROM_PAD,
    ADDA302_FROM_SSP,
};

typedef enum {
    ADDA302_SSP0RX_SSP0TX,
    ADDA203_SSP0RX_SSP1TX
} ADDA302_SSP_USAGE;

typedef enum {
    DA_SRC_SSP0 = 0,
    DA_SRC_SSP1,
    DA_SRC_MAX
} ADDA302_DA_SRC_T;

void adda302_init(const enum adda302_port_select ps);
void adda302_set_fs(const enum adda302_fs_frequency fs);
void adda302_set_ad_fs(const enum adda302_fs_frequency fs);
void adda302_set_da_fs(const enum adda302_fs_frequency fs);
ADDA302_SSP_USAGE ADDA302_Check_SSP_Current_Usage(void);
void ADDA302_Set_DA_Source(int da_src);

#endif //end of __ADDA302_API_H__
