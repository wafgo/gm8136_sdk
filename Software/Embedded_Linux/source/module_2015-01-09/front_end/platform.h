#ifndef _FE_PLATFORM_H_
#define _FE_PLATFORM_H_

/*************************************************************************************
 *  Platform Version Definition
 *************************************************************************************/
#define PLAT_COMMON_VERSION    140714

/*************************************************************************************
 *  Platform Global Definition
 *************************************************************************************/
typedef enum {
    PLAT_EXTCLK_SRC_PLL1OUT1 = 0,
    PLAT_EXTCLK_SRC_PLL1OUT1_DIV2,
    PLAT_EXTCLK_SRC_PLL4OUT2,
    PLAT_EXTCLK_SRC_PLL4OUT1_DIV2,
    PLAT_EXTCLK_SRC_PLL3,
    PLAT_EXTCLK_SRC_OSC_30MHZ,
    PLAT_EXTCLK_SRC_PLL2,
} PLAT_EXTCLK_SRC_T;

typedef enum {
    PLAT_EXTCLK_SSCG_DISABLE = 0,
    PLAT_EXTCLK_SSCG_MR0,
    PLAT_EXTCLK_SSCG_MR1,
    PLAT_EXTCLK_SSCG_MR2,
    PLAT_EXTCLK_SSCG_MR3,
    PLAT_EXTCLK_SSCG_MAX,
} PLAT_EXTCLK_SSCG_T;

typedef enum {
    PLAT_EXTCLK_DRIVING_4mA = 0,        ///< 2mA for GM813X
    PLAT_EXTCLK_DRIVING_8mA,            ///< 4mA for GM813X
    PLAT_EXTCLK_DRIVING_12mA,           ///< 6mA for GM813X
    PLAT_EXTCLK_DRIVING_16mA,           ///< 8mA for GM813X
    PLAT_EXTCLK_DRIVING_MAX,
} PLAT_EXTCLK_DRIVING_T;

typedef enum {
    PLAT_GPIO_ID_NONE = 0,
    PLAT_GPIO_ID_X_CAP_RST,
    PLAT_GPIO_ID_4,             ///< frontend hybrid driver design on GM8210 
    PLAT_GPIO_ID_5,             ///< frontend hybrid driver design on GM8210
    PLAT_GPIO_ID_6,             ///< frontend hybrid driver design on GM8210
    PLAT_GPIO_ID_7,             ///< frontend hybrid driver design on GM8210
    PLAT_GPIO_ID_8,             ///< frontend hybrid driver design on GM8286
    PLAT_GPIO_ID_9,             ///< frontend hybrid driver design on GM8286
    PLAT_GPIO_ID_15,            ///< frontend hybrid driver design on GM8286
    PLAT_GPIO_ID_16,            ///< frontend hybrid driver design on GM8286
    PLAT_GPIO_ID_25,
    PLAT_GPIO_ID_26,
    PLAT_GPIO_ID_27,
    PLAT_GPIO_ID_28,
    PLAT_GPIO_ID_47,
    PLAT_GPIO_ID_48,
    PLAT_GPIO_ID_49,
    PLAT_GPIO_ID_50,
    PLAT_GPIO_ID_51,
    PLAT_GPIO_ID_52,
    PLAT_GPIO_ID_59,
    PLAT_GPIO_ID_MAX
} PLAT_GPIO_ID_T;

typedef enum {
    PLAT_GPIO_DIRECTION_IN = 0,
    PLAT_GPIO_DIRECTION_OUT
} PLAT_GPIO_DIRECTION_T;

typedef enum {
    PLAT_SRC_ADDA = 0,
    PLAT_SRC_DECODER,
} PLAT_AUDIO_SRC_T;

typedef struct _audio_funs {
    char *codec_name;
    char *digital_codec_name;
    int (*sound_switch)(int ssp_idx, int chan_num, bool is_on);
} audio_funs_t;

int  plat_extclk_onoff(int on);
int  plat_extclk_set_freq(int clk_id, u32 freq);
int  plat_extclk_set_src(PLAT_EXTCLK_SRC_T src);
int  plat_extclk_set_driving(PLAT_EXTCLK_DRIVING_T driving);
int  plat_audio_sound_switch(int ssp_idx, int chan_num, bool is_on);
const char *plat_audio_get_codec_name(void);
const char *plat_audio_get_digital_codec_name(void);
int  plat_audio_register_function(audio_funs_t *func);
void plat_audio_deregister_function(void);
int  plat_gpio_set_value(PLAT_GPIO_ID_T gpio_id, int value);
int  plat_gpio_get_value(PLAT_GPIO_ID_T gpio_id);
int  plat_register_gpio_pin(PLAT_GPIO_ID_T gpio_id, PLAT_GPIO_DIRECTION_T direction, int out_value);
void plat_deregister_gpio_pin(PLAT_GPIO_ID_T gpio_id);
void plat_identifier_check(void);
int  plat_extclk_set_sscg(PLAT_EXTCLK_SSCG_T sscg_mr);
int  plat_set_ssp_clk(int cardno, int mclk, int clk_pll2);
int  plat_set_ssp1_src(int index);

#endif  /* _FE_PLATFORM_H_ */
