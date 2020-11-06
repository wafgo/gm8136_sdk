#ifndef __ADDA308_API_H__
#define __ADDA308_API_H__

#define DEBUG_ADDA308			0
#define ENABLE_ADDA308_DEBUG_PORT	0
#define ADDA308_LINE_IN			0
#define SAME_I2S_RXTX           0

enum adda308_fs_frequency {
    ADDA302_FS_8K,
    ADDA302_FS_16K,
    ADDA302_FS_22_05K,
    ADDA302_FS_32K,
    ADDA302_FS_44_1K,
    ADDA302_FS_48K,
};

enum adda308_chip_select {
    ADDA308_FIRST_CHIP,
};

enum adda308_port_select {
    ADDA308_AD,
    ADDA308_DA,
};

enum adda308_i2s_select {
    ADDA308_FROM_PAD,
    ADDA308_FROM_SSP,
};

typedef enum {
    ADDA308_SSP0RX_SSP0TX,
    ADDA308_SSP0RX_SSP1TX
} ADDA308_SSP_USAGE;

typedef enum {
    DA_SRC_SSP0 = 0,
    DA_SRC_SSP1,
    DA_SRC_MAX
} ADDA308_DA_SRC_T;

void adda308_init(const enum adda308_port_select ps);
void adda302_set_fs(const enum adda308_fs_frequency fs);
void adda302_set_ad_fs(const enum adda308_fs_frequency fs);
void adda302_set_da_fs(const enum adda308_fs_frequency fs);
ADDA308_SSP_USAGE ADDA308_Check_SSP_Current_Usage(void);
int adda308_get_output_mode(void);
void ADDA308_Set_DA_Source(int da_src);

#endif //end of __ADDA308_API_H__
