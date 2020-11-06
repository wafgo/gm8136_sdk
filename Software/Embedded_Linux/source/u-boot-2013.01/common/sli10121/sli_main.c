#include <common.h>
#include <command.h>
#include <config.h>
#include <asm/io.h>
#include "lcd.h"
#include "HDMI_header.h"

int hdmi_vid = VID_16_1920x1080p;
extern HDMI_Setting_t HDMI_setting;

#define printk	printf
//-------------------------------------------------------------------------------------------------
//-----------------------------------   SLI DHMI SOURCE CODE    -----------------------------------
//-------------------------------------------------------------------------------------------------
/* state flag */
#define SLI10121_IS_PROB        0x01
#define SLI10121_IS_CHANGE      0x02
#define SLI10121_IS_RGB         0x04
#define SLI10121_IS_RUNNING     0x08
#define SLI10121_IS_INIT        0x10

#define NAME_LEN    30
static int g_state_flag = 0x00;

#define CLEAR_STATE_FLAG(_x_)   (g_state_flag &= (~_x_))
#define SET_STATE_FLAG(_x_)     (g_state_flag |= (_x_))
#define GET_STATE_FLAG(_x_)     (g_state_flag & (_x_))

typedef struct sli1013_type_mappingTag{
    unsigned int type;
    char name[NAME_LEN];
} sli1013_type_mapping;

unsigned char Monitor_CompSwitch (void)
{
    // Set to Global
    HDMI_setting.vid           = hdmi_vid;   //VID_16_1920x1080p;
    HDMI_setting.output_format = FORMAT_RGB;
    HDMI_setting.audio_freq    = AUD_48K;
    HDMI_setting.audio_ch      = AUD_2CH;
    HDMI_setting.deep_color    = DEEP_COLOR_8BIT;
    HDMI_setting.down_sampling = DS_none;
    HDMI_setting.audio_source  = AUD_I2S;

    printk("sli-hdmi is in 1920x1080 RGB");
    return 0;
}

extern void DelayMs(unsigned ms);
int sli10121_polling_thread(lcd_output_t output)
{
    extern int get_hdmi_is_ready(void);
    extern void HDMI_init(void);
    extern unsigned char get_hdmi_INT94_status(void);
    extern void HDMI_ISR_top (void);
    extern void HDMI_ISR_bottom (void);
    unsigned char last_int_val = 0, int_val;
    int count = -1;
    //int vid = VID_16_1920x1080p;

    if (GET_STATE_FLAG(SLI10121_IS_INIT))
        return -1;

    printk("start hdmi polling thread. \n");

    switch (output) {
      case OUTPUT_VGA_1920x1080:
        hdmi_vid = VID_16_1920x1080p;
        break;
      case OUTPUT_VGA_1280x720:
        hdmi_vid = VID_04_1280x720p;
        break;
      case OUTPUT_VGA_1024x768:
        hdmi_vid = VID_60_1024x768_RGB_EXT;
        break;
      default:
        return -1;
        break;
    }

    Monitor_CompSwitch();

    SET_STATE_FLAG(SLI10121_IS_RUNNING);
    HDMI_init(); /* init HDMI chip */
    SET_STATE_FLAG(SLI10121_IS_INIT);

    while(++ count < 300) {
        if (!(count & 0x1F))
            printk("Please plug-in the HDMI cable, otherwise the system may keep wating the connection.... \n");

        int_val = get_hdmi_INT94_status();

        if (int_val != last_int_val) {
            last_int_val = int_val;
            HDMI_ISR_top();
        }

        HDMI_ISR_bottom();

        if (get_hdmi_is_ready())
            break;

        DelayMs(100);
    };

    if (count >= 300)
        printk("HDMI connection is dropped! \n");

    return 0;
}

int hdmi_polling_thread(lcd_output_t output)
{
    return sli10121_polling_thread(output);
}