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

#include <linux/kernel.h>
#include <linux/module.h>

#define _MAIN_
//#include <stdlib.h>
#include "hdmitx.h"

////////////////////////////////////////////////////////////////////////////////
// EDID
////////////////////////////////////////////////////////////////////////////////
void HDMITX_DevLoopProc(void) ;
void HDMITX_ChangeDisplayOption(HDMI_Video_Type VideoMode, HDMI_OutputColorMode OutputColorMode) ;

int main(void)
{

    HDMITX_ChangeDisplayOption(HDMI_480p60, HDMI_RGB444) ; // set initial video mode and initial output color mode

    InitCAT6611() ;

    while(1)
    {
        // if(Update Control)
        //     HDMITX_ChangeDisplayOption(HDMI_480p60, HDMI_RGB444) ;

        HDMITX_DevLoopProc() ;
    }

    return 0 ;
}

#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>

static struct task_struct *hdmi_thread=NULL;
HDMI_Video_Type g_video_type = HDMI_720p60;         //refer to OutputColorMode
HDMI_OutputColorMode OutputColorMode = HDMI_YUV422; //refer to g_video_type
int cat6611_init_done = 0, bChange = 0;
int hdmi_is_running = 0, isRGB = 0;

extern void mcuio_i2c_init(void);
extern mcuio_i2c_exit(void);

/* HDMI thread. The method follows ITE's advice
 */
static int hdmi_polling_thread(void *private)
{
    extern void CAT6611_Config_SyncMode(int isRGB);

    if (private)    {}

    /* init */
    HDMITX_ChangeDisplayOption(g_video_type, OutputColorMode) ; // set initial video mode and initial output color mode
    InitCAT6611();
    CAT6611_Config_SyncMode(isRGB);

    hdmi_is_running = 1;

    do
    {
        if (bChange)
        {
            printk("%s, OutputColorMode = %d, isRGB = %d \n", __func__, OutputColorMode, isRGB);

            HDMITX_ChangeDisplayOption(g_video_type, OutputColorMode) ; // set initial video mode and initial output color mode
            InitCAT6611();
            CAT6611_Config_SyncMode(isRGB);
            bChange = 0;
        }

        HDMITX_DevLoopProc();
        /* sleep 100ms, adviced by ITE corp. */
        msleep(100);

    } while (!kthread_should_stop());

    hdmi_is_running = 0;
    return 0;
}

#include "../codec.h"
static struct proc_dir_entry *cat6611_proc = NULL;

void cat6611_setting(struct ffb_dev_info *info)
{
    HDMI_Video_Type     input_mode = HDMI_Unkown;

    if (!hdmi_thread)
    {
        hdmi_thread = kthread_create(hdmi_polling_thread, NULL, "hdmi_poll");
        if (IS_ERR(hdmi_thread))
        {
            printk("LCD: Error in creating kernel thread! \n");
            return;
        }

        wake_up_process(hdmi_thread);
    }

    switch (info->video_output_type)
    {
      case VOT_720P:
        input_mode = HDMI_720p60;
        printk("CAT6611/6612 is in 720P. \n");
        break;
      case VOT_1920x1080:
        input_mode = HDMI_1080p60;
        printk("Input 1920x1080 RGB888 to cat6611/6612.\n");
        break;
      case VOT_1920x1080P:
        //input_mode = HDMI_1080i60;
        input_mode = HDMI_1080p60;
        printk("CAT6611/6612 is in 1080P. \n");
        break;
      case VOT_480P:
        input_mode = HDMI_480p60;
        printk("CAT6611/6612 is in 480P. \n");
        break;
      case VOT_576P:
        input_mode = HDMI_576p50;
        printk("CAT6611/6612 is in 576P. \n");
        break;
      case VOT_1024x768:
        input_mode = HDMI_1024x768p60;
        printk("Input 1024x768 RGB888 to cat6611/6612.\n");
        break;
      case VOT_1024x768P:
        input_mode = HDMI_1024x768p60;
        printk("Input 1024x768 BT1120 to cat6611/6612.\n");
        break;
      case VOT_1280x1024:
        input_mode = HDMI_1280x1024p60;
        printk("Input 1280x1024 RGB888 to cat6611/6612.\n");
        break;
      case VOT_1280x1024P:
        input_mode = HDMI_1280x1024p60;
        printk("Input 1280x1024 BT1120 to cat6611/6612.\n");
        break;
      case VOT_1680x1050P:
        input_mode = HDMI_1680x1050p60;
        printk("Input 1680x1050 BT1120 to cat6611/6612.\n");
        break;
      case VOT_1440x900P:
        input_mode = HDMI_1440x900p60;
        printk("Input 1440x900 BT1120 to cat6611/6612.\n");
        break;
      default:
        printk("Error input %d to cat6611/6612!!! \n", info->video_output_type);
        return;
    }

    if (g_video_type != input_mode)
    {
        isRGB = (info->video_output_type >= VOT_CCIR656) ? 0 : 1;
        OutputColorMode = isRGB ? HDMI_RGB444 : HDMI_YUV422;
        printk("LCD210: Input format to CAT6611/6612 is %s. \n", isRGB ? "RGB888" : "YUV422");
        g_video_type = input_mode;
        bChange = 1;
    }

    return;
}

/*
 * dump the cat6611/6612 register
 */
static int proc_read_cat6611_reg(char *page, char **start, off_t off, int count,
			  int *eof, void *data)
{
	int    len = 0;
	extern void DumpCat6611Reg(void);

	len += sprintf(page+len, "CAT6611/6612 register: \n");

    DumpCat6611Reg();

	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;
	return len;
}

/* GM porting interface */
int cat6611_proc_init(void)
{
    int ret = 0;

    if (cat6611_proc != NULL)
        return 0;

    cat6611_proc = ffb_create_proc_entry("cat6611", S_IRUGO | S_IXUGO, flcd_common_proc);
	if (cat6611_proc == NULL) {
		printk("Fail to create proc for CAT6611/6612!\n");
		ret = -EINVAL;
		goto end;
	}

	cat6611_proc->read_proc = (read_proc_t*)proc_read_cat6611_reg;
	cat6611_proc->write_proc = NULL;

end:
    return ret;
}

void cat6611_proc_remove(void)
{
    if (cat6611_proc != NULL)
        ffb_remove_proc_entry(flcd_common_proc, cat6611_proc);

    cat6611_proc = NULL;
}

/*
 * Probe function,. Only called once when inserting this module
 */
int cat6611_probe(void *private)
{
    if (private)    {}

    if (cat6611_init_done)
        return 0;

    cat6611_init_done = 1;

    return 0;
}

/*
 * remove function. Only called when unload this module
 */
void cat6611_remove(void *private)
{
    if (private)    {}

    if (hdmi_thread)
        kthread_stop(hdmi_thread);

    while (hdmi_is_running) {
        msleep(10);
    }
    hdmi_thread = NULL;
    cat6611_init_done = 0;


}

static int __init cat6611_init(void)
{
    codec_driver_t    driver;

    mcuio_i2c_init();

    /* 1440x900 BT1120 */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "cat6612-1440x900P");
    driver.codec_id = OUTPUT_TYPE_CAT6611_1440x900P_30;
    driver.probe = cat6611_probe;   //probe function
    driver.setting = cat6611_setting;
    driver.enc_proc_init = cat6611_proc_init;
    driver.enc_proc_remove = cat6611_proc_remove;
    driver.remove = cat6611_remove;
    if (codec_driver_register(&driver) < 0)
    {
        printk("register cat6612-1440x900P to pip fail! \n");
        return -1;
    }

    /* 1024x768 BT1120 */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "cat6612-1024x768P");
    driver.codec_id = OUTPUT_TYPE_CAT6611_1024x768P_25;
    driver.probe = cat6611_probe;   //probe function
    driver.setting = cat6611_setting;
    driver.enc_proc_init = cat6611_proc_init;
    driver.enc_proc_remove = cat6611_proc_remove;
    driver.remove = cat6611_remove;
    if (codec_driver_register(&driver) < 0)
    {
        printk("register cat6612-1024x768P to pip fail! \n");
        return -1;
    }

    /* 1680x1050p BT1120 */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "cat6612-1680x1050P");
    driver.codec_id = OUTPUT_TYPE_CAT6611_1680x1050P_27;
    driver.setting = cat6611_setting;
    driver.enc_proc_init = cat6611_proc_init;
    driver.enc_proc_remove = cat6611_proc_remove;
    driver.remove = cat6611_remove;
    if (codec_driver_register(&driver) < 0)
    {
        printk("register cat6612-1680x1050P to pip fail! \n");
        return -1;
    }

    /* 1680x1050p BT1120 */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "cat6612-1280x1024");
    driver.codec_id = OUTPUT_TYPE_CAT6611_1280x1024_28;
    driver.setting = cat6611_setting;
    driver.enc_proc_init = cat6611_proc_init;
    driver.enc_proc_remove = cat6611_proc_remove;
    driver.remove = cat6611_remove;
    if (codec_driver_register(&driver) < 0)
    {
        printk("register cat6612-1280x1024 to pip fail! \n");
        return -1;
    }

    /* 1280x1024p BT1120 */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "cat6612-1280x1024P");
    driver.codec_id = OUTPUT_TYPE_CAT6611_1280x1024P_26;
    driver.setting = cat6611_setting;
    driver.enc_proc_init = cat6611_proc_init;
    driver.enc_proc_remove = cat6611_proc_remove;
    driver.remove = cat6611_remove;
    if (codec_driver_register(&driver) < 0)
    {
        printk("register cat6612-1280x1024P to pip fail! \n");
        return -1;
    }

    /* HDMI FullHD*/
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "cat6612-FullHD");
    driver.codec_id = OUTPUT_TYPE_CAT6611_1080P_23;
    driver.setting = cat6611_setting;
    driver.enc_proc_init = cat6611_proc_init;
    driver.enc_proc_remove = cat6611_proc_remove;
    driver.remove = cat6611_remove;
    if (codec_driver_register(&driver) < 0)
    {
        printk("register cat6612-FullHD to pip fail! \n");
        return -1;
    }

    /* 720P */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "cat6612-720P");
    driver.codec_id = OUTPUT_TYPE_CAT6611_720P_22;
    driver.setting = cat6611_setting;
    driver.enc_proc_init = cat6611_proc_init;
    driver.enc_proc_remove = cat6611_proc_remove;
    driver.remove = cat6611_remove;
    if (codec_driver_register(&driver) < 0)
    {
        printk("register cat6612-720P to pip fail! \n");
        return -1;
    }

    /* 480P */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "cat6612-480P");
    driver.codec_id = OUTPUT_TYPE_CAT6611_480P_20;
    driver.setting = cat6611_setting;
    driver.enc_proc_init = cat6611_proc_init;
    driver.enc_proc_remove = cat6611_proc_remove;
    driver.remove = cat6611_remove;
    if (codec_driver_register(&driver) < 0)
    {
        printk("register cat6612-480P to pip fail! \n");
        return -1;
    }

    /* 576P */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "cat6612-576P");
    driver.codec_id = OUTPUT_TYPE_CAT6611_576P_21;
    driver.setting = cat6611_setting;
    driver.enc_proc_init = cat6611_proc_init;
    driver.enc_proc_remove = cat6611_proc_remove;
    driver.remove = cat6611_remove;
    if (codec_driver_register(&driver) < 0)
    {
        printk("register cat6612-576P to pip fail! \n");
        return -1;
    }

    /* 1024x768 RGB888 */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "cat6612-1024x768");
    driver.codec_id = OUTPUT_TYPE_CAT6611_1024x768_24;
    driver.setting = cat6611_setting;
    driver.enc_proc_init = cat6611_proc_init;
    driver.enc_proc_remove = cat6611_proc_remove;
    driver.remove = cat6611_remove;
    if (codec_driver_register(&driver) < 0)
    {
        printk("register cat6612-1024x768 to pip fail! \n");
        return -1;
    }

    /* 1920x1080 RGB888 */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "cat6612-1920x1080");
    driver.codec_id = OUTPUT_TYPE_CAT6611_1920x1080_29;
    driver.setting = cat6611_setting;
    driver.enc_proc_init = cat6611_proc_init;
    driver.enc_proc_remove = cat6611_proc_remove;
    driver.remove = cat6611_remove;
    if (codec_driver_register(&driver) < 0)
    {
        printk("register cat6612-1920x1080 to pip fail! \n");
        return -1;
    }


    return 0;
}

static void __exit cat6611_exit(void)
{
    codec_driver_t    driver;

    driver.codec_id = OUTPUT_TYPE_CAT6611_1440x900P_30;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_CAT6611_1024x768P_25;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_CAT6611_1280x1024P_26;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_CAT6611_1680x1050P_27;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_CAT6611_1280x1024_28;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_CAT6611_1080P_23;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_CAT6611_720P_22;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_CAT6611_480P_20;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_CAT6611_576P_21;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_CAT6611_1024x768_24;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_CAT6611_1920x1080_29;
    codec_driver_deregister(&driver);

    cat6611_remove(NULL);

    mcuio_i2c_exit();
}

module_init(cat6611_init);
module_exit(cat6611_exit);

MODULE_AUTHOR("ITE");
MODULE_DESCRIPTION("CAT6611 driver");
MODULE_LICENSE("GPL");
