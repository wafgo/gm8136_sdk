///*****************************************
//  Copyright (C) 2009-2014
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   >main.c<
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2009/08/24
//   @fileversion: CAT6611_SAMPLEINTERFACE_1.12
//******************************************/

///////////////////////////////////////////////////////////////////////////////
// This is the sample program for CAT6611 driver usage.
///////////////////////////////////////////////////////////////////////////////

#define _MAIN_

#include <common.h>
#include <command.h>
#include <config.h>
#include <asm/io.h>
#include "hdmitx.h"
#include "lcd.h"

#define printk  printf

////////////////////////////////////////////////////////////////////////////////
// EDID
////////////////////////////////////////////////////////////////////////////////
void HDMITX_DevLoopProc(void) ;
void HDMITX_ChangeDisplayOption(HDMI_Video_Type VideoMode, HDMI_OutputColorMode OutputColorMode) ;

HDMI_Video_Type g_video_type = HDMI_720p60;         //refer to OutputColorMode
HDMI_OutputColorMode OutputColorMode = HDMI_YUV422; //refer to g_video_type
int cat6611_init_done = 0, bChange = 0, isRGB = 0;
extern int hdmi_connected;

/* HDMI thread. The method follows ITE's advice
 */
static int hdmi_polling_thread2(void *private)
{
    extern void CAT6611_Config_SyncMode(int isRGB);
    int loop = 0;
    BYTE    sysstat;

    if (private)    {}

    /* init */
    //HDMITX_ChangeDisplayOption(g_video_type, OutputColorMode) ; // set initial video mode and initial output color mode
    //InitCAT6611();
    //CAT6611_Config_SyncMode(isRGB);

    do
    {
        if (bChange)
        {
            printk("%s, g_video_type = %d, OutputColorMode = %d, isRGB = %d \n", __func__, g_video_type, OutputColorMode, isRGB);

            HDMITX_ChangeDisplayOption(g_video_type, OutputColorMode) ; // set initial video mode and initial output color mode
            InitCAT6611();
            CAT6611_Config_SyncMode(isRGB);
            bChange = 0;
        }

        sysstat = HDMITX_ReadI2C_Byte(0xE) ;    /* status */
        if (!(sysstat & (0x1 << 6))) {   /* hpd detect */
            printf("Please insert the HDMI cable! \n");
            mdelay(1000);
            continue;
        }

        HDMITX_DevLoopProc();
        /* sleep 100ms, adviced by ITE corp. */
        mdelay(100);
        if (hdmi_connected)
            loop ++;
    } while (loop < 20);

    //DumpCat6611Reg();

    return 0;
}

int hdmi_polling_thread(lcd_output_t output)
{
    int input_mode;

    switch (output) {
      case OUTPUT_CCIR656_NTSC:
        input_mode = HDMI_480p60;
        isRGB = 0;
        printk("CAT6611/6612 is in 480P. \n");
        break;
      case OUTPUT_CCIR656_PAL:
        input_mode = HDMI_576p50;
        isRGB = 0;
        printk("CAT6611/6612 is in 576P. \n");
        break;
      case OUTPUT_BT1120_720P:
        input_mode = HDMI_720p60;
        isRGB = 0;
        printk("CAT6611/6612 is in 720P. \n");
        break;
      case OUTPUT_BT1120_1080P:
        input_mode = HDMI_1080p60;
        isRGB = 0;
        printk("CAT6611/6612 is in 1080P. \n");
        break;
      case OUTPUT_VGA_1920x1080:
        input_mode = HDMI_1080p60;
        isRGB = 1;
        printk("Input 1920x1080 RGB888 to cat6611/6612.\n");
        break;
      case OUTPUT_VGA_1280x720:
        isRGB = 1;
        return -1;
        break;
      case OUTPUT_VGA_1024x768:
        isRGB = 1;
        input_mode = HDMI_1024x768p60;
        printk("Input 1024x768 RGB888 to cat6611/6612.\n");
        break;
      default:
        return -1;
        break;
    }

    if (g_video_type != input_mode)
    {
        OutputColorMode = isRGB ? HDMI_RGB444 : HDMI_YUV422;
        printk("LCD210: Input format to CAT6611/6612 is %s. \n", isRGB ? "RGB888" : "YUV422");
        g_video_type = input_mode;
        bChange = 1;
    }
    bChange = 1;
    hdmi_polling_thread2(NULL);
    return 0;
}