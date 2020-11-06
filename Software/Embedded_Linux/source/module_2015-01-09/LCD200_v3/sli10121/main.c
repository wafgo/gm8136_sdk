#define _MAIN_C_

////////////////////////////////////////////Standard Include///////////////////////////////////////////////
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <mach/platform/platform_io.h>
#include <mach/ftpmu010.h>

////////////////////////////////////////////Project Include////////////////////////////////////////////////
#include "codec.h"
#include "typedef.h"
#include "USB_API.h"
#include "HDMI_header.h"
#include "gm_i2c.h"
#include "lcd_def.h"

#define SLI_USE_INTR_MODE
#undef SLI_USE_INTR_MODE

#ifdef SLI_USE_INTR_MODE
    #ifdef CONFIG_PLATFORM_GM8210
    #define SLI10121_IRQ_NO     HDMI_FTHDMI_0_IRQ
    #else
        #error "Please define sli10121 irq number!"
    #endif
#endif

static u32 edid_array[(OUTPUT_TYPE_LAST + 7) / 8];

///////////////////////////////////////////////////Macro///////////////////////////////////////////////////

/* state flag */
#define SLI10121_IS_PROB        0x01
#define SLI10121_IS_CHANGE      0x02
#define SLI10121_IS_RGB         0x04
#define SLI10121_IS_RUNNING     0x08
#define SLI10121_IS_INIT        0x10


#define CLEAR_STATE_FLAG(_x_)     (g_state_flag &= (~_x_))
#define SET_STATE_FLAG(_x_)       (g_state_flag |= (_x_))
#define GET_STATE_FLAG(_x_)       (g_state_flag & (_x_))

//////////////////////////////////////////Global Type Definitions//////////////////////////////////////////
typedef struct sli1013_type_mappingTag{
    unsigned int type;
    char name[NAME_LEN];
}sli1013_type_mapping;

///////////////////////////////////////Global Function Definitions////////////////////////////////////////
unsigned char Monitor_CompSwitch (void);
int sli10121_proc_init(void);
int sli10121_probe(void *private);
void sli10121_setting(struct ffb_dev_info *info);
void sli10121_remove(void *private);
void sli10121_proc_remove(void);

///////////////////////////////////////Private Function Definitions////////////////////////////////////////
static int sli10121_proc_read_reg(char *page, char **start, off_t off, int count,
			  int *eof, void *data);
static ssize_t sli10121_proc_write_reg(struct file *file, const char __user *buf, size_t size, loff_t *off);
static int __init sli10121_init(void);
int (*edid_callback)(u32 *edid_array, int error_checking, int add_remove) = NULL;

///////////////////////////////////////Private Variable Definitions/////////////////////////////////////////
u32 g_dump_edid = 0;
u32 g_force_outmode = 0;
u32 g_force_yccmode = 1;

static int g_state_flag = 0x00;
static int g_sli10121_dev_id = 0x88;         //for share irq identifier
static unsigned int g_input_type=0; //VID_02_720x480p
static unsigned int g_out_color_mode=FORMAT_YCC422;
static struct task_struct *gp_hdmi_thread = NULL;
static u32 hdmi_onoff = LCD_SCREEN_ON;


sli1013_type_mapping out_type_table[] =
{
    {OUTPUT_TYPE_SLI10121_480P_42,      "sli10121-480P_BT1120"},
    {OUTPUT_TYPE_SLI10121_576P_43,      "sli10121-576P_BT1120"},
    {OUTPUT_TYPE_SLI10121_1024x768_46,  "sli10121-1024x768_RGB"},
    {OUTPUT_TYPE_SLI10121_1280x1024_50, "sli10121-1280x1024_RGB"},
    {OUTPUT_TYPE_SLI10121_1920x1080_51, "sli10121-1920x1080_RGB"},
    {OUTPUT_TYPE_SLI10121_1280x720_53,  "sli10121-1280x720_RGB"},
};

///////////////////////////////////////Global Variable Definitions/////////////////////////////////////////
extern BYTE RE_SETTING;
extern HDMI_Setting_t HDMI_setting;
extern BYTE PowerMode;

static struct proc_dir_entry *sli10121_proc = NULL;
static struct proc_dir_entry *hdmi_onoff_proc = NULL;
static struct proc_dir_entry *edid_dump_proc = NULL;
static struct proc_dir_entry *force_outmode_proc = NULL;
static struct proc_dir_entry *force_yccmode_proc = NULL;


/* parameter provided by SILICON */
unsigned int EX0 = 0;
unsigned int ET2 = 0;
unsigned int TR2 = 0;
unsigned int TMR2CN = 0;
unsigned int P3 = 0;
unsigned int P4 = 0;
unsigned int P1 = 0;
unsigned int CKCON=0;
unsigned int TMR2RL=0;
unsigned int TMR2=0;
unsigned char COMP_MODE = 1;
static int enter_isr = 0;

extern EDID_Support_setting_t edid_support_res_array[];
/*
 * Module Parameter
 */
static bool hdmi_test = 0;
module_param(hdmi_test, bool, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(hdmi_test, "HDMI Test Mode 1:enable 0:disbale");

static unsigned int sleep_ms = 100;
module_param(sleep_ms, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(sleep_ms, "HDMI sleep ms parameter");


#ifdef CONFIG_PLATFORM_GM8210
#include <cpu_comm/cpu_comm.h>

#define CPU_COMM_HDMI           CHAN_0_USR4
#define HDMI_CMD_SET_ONOFF      0x100
#define HDMI_CMD_GET_ONOFF      0x104
#define HDMI_CMD_REPLY_ONOFF    0x108

static struct task_struct *hdmi_onoff_thread = NULL;
int hdmi_thread_running = 0;
static int hdmi_onoff_function(void *private)
{
    cpu_comm_msg_t  snd_msg, rcv_msg;
    unsigned int    command[4];

    hdmi_thread_running = 1;

    if (cpu_comm_open(CPU_COMM_HDMI, "hdmi"))
        panic("%s, open cpucomm channel %d fail! \n", __func__, CPU_COMM_HDMI);

    while(!kthread_should_stop()) {
        memset(&rcv_msg, 0, sizeof(cpu_comm_msg_t));
        rcv_msg.target = CPU_COMM_HDMI;
        rcv_msg.length = sizeof(command);
        rcv_msg.msg_data = (unsigned char *)&command[0];

        if (cpu_comm_msg_rcv(&rcv_msg, 1000 /* ms */))
            continue;

        switch (command[0]) {
          case HDMI_CMD_SET_ONOFF:
            if (command[0] == LCD_SCREEN_ON) {
                if (hdmi_onoff == LCD_SCREEN_ON)
                    break;
            }
            if (command[0] == LCD_SCREEN_OFF) {
                if (hdmi_onoff == LCD_SCREEN_OFF)
                    break;
            }
            HDMI_System_PD(PowerMode_A);
            hdmi_onoff = command[2];
            break;

          case HDMI_CMD_GET_ONOFF:
            memset(&snd_msg, 0, sizeof(cpu_comm_msg_t));
            snd_msg.target = CPU_COMM_HDMI;
            snd_msg.length = sizeof(command);
            snd_msg.msg_data = (unsigned char *)&command[0];
            command[0] = HDMI_CMD_REPLY_ONOFF;
            command[1] = 4; //length
            command[2] = hdmi_onoff;
            cpu_comm_msg_snd(&snd_msg, -1);
            break;

          default:
            printk("%s, unknown command: 0x%x \n", __func__, command[0]);
            break;
        }
    }

    cpu_comm_close(CPU_COMM_HDMI);
    hdmi_thread_running = 0;
    return 0;
}
#endif /* CONFIG_PLATFORM_GM8210 */

static int __init sli10121_init(void)
{
    codec_driver_t    driver;
    unsigned int i, chip_id;

    chip_id = (ftpmu010_get_attr(ATTR_TYPE_CHIPVER) >> 16) & 0xFFFF;
    if (chip_id == 0x8282)
        return 0;   /* without HDMI */

    memset((void *)&edid_array[0], 0, sizeof(edid_array));

    sli10121_i2c_init();

    for (i=0 ; i < ARRAY_SIZE(out_type_table); i++) {
        memset(&driver, 0, sizeof(codec_driver_t));
        strcpy(driver.name, out_type_table[i].name);
        driver.codec_id = out_type_table[i].type;
        driver.probe = sli10121_probe;   //probe function
        driver.setting = sli10121_setting;
        driver.enc_proc_init = sli10121_proc_init;
        driver.enc_proc_remove = sli10121_proc_remove;
        driver.remove = sli10121_remove;
        if (codec_driver_register(&driver) < 0){
            printk("register %d to pip fail! \n",driver.codec_id);
            return -1;
        }
    }
    printk("sli10121 sleep %d ms\n",sleep_ms);
#ifdef CONFIG_PLATFORM_GM8210
    hdmi_onoff_thread = kthread_create(hdmi_onoff_function, 0, "hdmi_onoff");
    if (IS_ERR(hdmi_onoff_thread)) {
        printk("HDMI: Error in creating kernel thread! \n");
        hdmi_onoff_thread = NULL;
    } else {
        wake_up_process(hdmi_onoff_thread);
    }
#endif
    return 0;
}

irqreturn_t sli10121_irq_handler(int irq, void *dev_id)
{
    if (enter_isr == 1)
        return IRQ_HANDLED;

    enter_isr = 1;
    HDMI_ISR_top();

    return IRQ_HANDLED;
}

//-----------------------------------------------------------------------------
// Monitor_CompSwitch()
//-----------------------------------------------------------------------------
//
// Return Value :
//
// Parameters   :
//
// Monitors switch for compliance mode. When setting changed, set the new
// video and audio mode according to the switch.
//
unsigned char Monitor_CompSwitch (void)
{
    int i;
    int found = 0;
    // Set to Global
    HDMI_setting.vid           = g_input_type;
    HDMI_setting.audio_freq    = AUD_48K;
    HDMI_setting.audio_ch      = AUD_2CH;
    HDMI_setting.deep_color    = DEEP_COLOR_8BIT;
    HDMI_setting.down_sampling = DS_none;
    HDMI_setting.audio_source  = AUD_I2S;

    for (i = 0; edid_support_res_array[i].vid != -1; i ++) {
        if (!edid_support_res_array[i].active)
            continue;

        switch (edid_support_res_array[i].vid) {
          case VID_02_720x480p:
            set_bit(OUTPUT_TYPE_SLI10121_480P_42, (void *)edid_array);
            found = 1;
            break;
          case VID_04_1280x720p:
            set_bit(OUTPUT_TYPE_SLI10121_1280x720_53, (void *)edid_array);
            found = 1;
            break;
          case VID_16_1920x1080p:
            set_bit(OUTPUT_TYPE_SLI10121_1920x1080_51, (void *)edid_array);
            found = 1;
            break;
          case VID_60_1024x768_RGB_EXT:
            set_bit(OUTPUT_TYPE_SLI10121_1024x768_46, (void *)edid_array);
            found = 1;
            break;
          case VID_61_1280x1024_RGB_EXT:
            set_bit(OUTPUT_TYPE_SLI10121_1280x1024_50, (void *)edid_array);
            found = 1;
            break;
          default:
            break;
        }
    }

    if(!found){
        printk("EDID isn't found , set to default 1280x720p\n");
        set_bit(OUTPUT_TYPE_SLI10121_1280x720_53, (void *)edid_array);
    }

    if (edid_callback)
        edid_callback(edid_array, OUTPUT_TYPE_LAST, 1);

    return 0;
}


/* HDMI thread. The method follows Silicon's advice
 */
static int sli10121_polling_thread(void *private)
{
    extern int get_hdmi_is_ready(bool tmode);
#ifndef SLI_USE_INTR_MODE
    extern BYTE get_hdmi_INT94_status(void);
    BYTE last_int_val = 0, int_val = -1;
#endif

    printk("%s INIT = %d \n", __func__, GET_STATE_FLAG(SLI10121_IS_INIT));

    if (!GET_STATE_FLAG(SLI10121_IS_INIT)) {
        int status;

        if (status) {}

        sli10121_timer_init(); /* init workque for timer */

        SET_STATE_FLAG(SLI10121_IS_RUNNING);

        HDMI_init(); /* init HDMI chip */
        SET_STATE_FLAG(SLI10121_IS_INIT);

#ifdef SLI_USE_INTR_MODE
        /* we MUST use edge trigger */
        status = request_irq(SLI10121_IRQ_NO, sli10121_irq_handler,
                            0, "sli10121", (void *)g_sli10121_dev_id);
        if (status < 0) {
            printk("[ERROR]%s Error to request irq %d!\n", __func__, SLI10121_IRQ_NO);
            return -ENODEV;
        }
#endif
    }

    printk("hdmi_test mode=%d\n",hdmi_test);
    do {
        if (hdmi_onoff == LCD_SCREEN_OFF) {
            /* keep in loop */
            DelayMs(2000);
            int_val = -1;
            last_int_val = 0;
            SET_STATE_FLAG(SLI10121_IS_CHANGE);
            continue;
        }

#ifndef SLI_USE_INTR_MODE
        if (int_val != last_int_val) {
            last_int_val = int_val;
            HDMI_ISR_top();
        }
#endif
        if (PowerMode == PowerMode_E) {
            if (GET_STATE_FLAG(SLI10121_IS_CHANGE)) {
                RE_SETTING = 1;
                CLEAR_STATE_FLAG(SLI10121_IS_CHANGE);
            }
        }

        HDMI_ISR_bottom();
        enter_isr = 0;

        /* If the system is not ready, we use busy polling */
        if (!get_hdmi_is_ready(hdmi_test)) {
            DelayMs(sleep_ms);
        } else {
            extern BYTE HDMI_hotplug_check2 (void);
            int i;

            if(hdmi_test)
                DelayMs(10);
            else
                DelayMs(2000);

            if (!HDMI_hotplug_check2()) {
                printk("HDMI_hotplug_check2 detect\n");
                int_val = -1;
                last_int_val = 0;
                SET_STATE_FLAG(SLI10121_IS_CHANGE);
                HDMI_System_PD (PowerMode_A);
                if (edid_callback)
                    edid_callback(edid_array, OUTPUT_TYPE_LAST, 0);

                /* clear database */
                memset((void *)edid_array, 0, sizeof(edid_array));
                for (i = 0; edid_support_res_array[i].vid != -1; i ++){
                    edid_support_res_array[i].active = 0;
                    edid_support_res_array[i].is_hdmi = 0;
                }
            }
        }
    } while(!kthread_should_stop());

    HDMI_System_PD (PowerMode_A);

#ifdef SLI_USE_INTR_MODE
    free_irq(SLI10121_IRQ_NO, (void *)g_sli10121_dev_id);
#endif

    printk("END %s\n", __func__);

    CLEAR_STATE_FLAG(SLI10121_IS_RUNNING);
    return 0;
}


/*
 * Probe function,. Only called once when inserting this module
 */
int sli10121_probe(void *private)
{
    if (GET_STATE_FLAG(SLI10121_IS_PROB))
        return 0;

    SET_STATE_FLAG(SLI10121_IS_PROB);
    SET_STATE_FLAG(SLI10121_IS_CHANGE); //in order to have InitSLI10121()

    return 0;
}

/*
 * remove function. Only called when unload this module
 */
void sli10121_remove(void *private)
{
    sli10121_timer_destroy();

    if (gp_hdmi_thread)
        kthread_stop(gp_hdmi_thread);

    while (GET_STATE_FLAG(SLI10121_IS_RUNNING)){
        DelayMs(10);
    }

    gp_hdmi_thread = NULL;

    CLEAR_STATE_FLAG(SLI10121_IS_PROB);
    CLEAR_STATE_FLAG(SLI10121_IS_INIT);

    if (edid_callback) {
        edid_callback(edid_array, OUTPUT_TYPE_LAST, 0);
        edid_callback = NULL;
    }
}


void sli10121_setting(struct ffb_dev_info *info)
{
    unsigned input_mode = 0;

    if (!gp_hdmi_thread) {
        gp_hdmi_thread = kthread_create(sli10121_polling_thread, NULL, "hdmi_sli10121_poll");
        if (IS_ERR(gp_hdmi_thread)){
            printk("LCD: Error in creating kernel thread! \n");
            return;
        }

        wake_up_process(gp_hdmi_thread);
    }

    switch (info->video_output_type){
        case VOT_480P:
            input_mode = VID_02_720x480p;   //pre-programmed VID, Progressive, 27Mhz, 60Hz
            HDMI_setting.output_format = FORMAT_YCC422;
            printk("sli-hdmi is in 480P BT1120");
            break;
        case VOT_576P:
            input_mode = VID_02_720x480p;   //pre-programmed VID, Progressive, 27Mhz, 50Hz
            HDMI_setting.output_format = FORMAT_YCC422;
            printk("sli-hdmi is in 576P BT1120");
            break;
        case VOT_1024x768:
            input_mode = VID_60_1024x768_RGB_EXT;
            HDMI_setting.output_format = FORMAT_RGB;
            printk("sli-hdmi is in 1024x768 RGB");
            break;
        case VOT_1280x1024:
            input_mode = VID_61_1280x1024_RGB_EXT;
            HDMI_setting.output_format = FORMAT_RGB;
            printk("sli-hdmi is in 1280x1024 RGB");
            break;
        case VOT_1920x1080:
            input_mode = VID_16_1920x1080p;
            HDMI_setting.output_format = FORMAT_RGB;
            printk("sli-hdmi is in 1920x1080 RGB");
            break;
        case VOT_1280x720:
            input_mode = VID_04_1280x720p;
            HDMI_setting.output_format = FORMAT_RGB;
            printk("sli-hdmi is in 1280x720 RGB");
            break;
        default:
            while (1)
                printk("[ERROR]Error input %d to sli-hdmi!!! \n", info->video_output_type);
            return;
    }

    /* while getting the edit table, it will call this function to notice the pip */
    edid_callback = info->update_edid;

    if (g_input_type != input_mode) {
        if(info->video_output_type >= VOT_CCIR656)
            CLEAR_STATE_FLAG(SLI10121_IS_RGB);
        else
            SET_STATE_FLAG(SLI10121_IS_RGB);

        g_out_color_mode = GET_STATE_FLAG(SLI10121_IS_RGB) ? FORMAT_RGB : FORMAT_YCC422;
        printk("LCD210: Input format to sli-hdmi is %s. \n", GET_STATE_FLAG(SLI10121_IS_RGB) ? "RGB888" : "YUV422");
        g_input_type = input_mode;
        SET_STATE_FLAG(SLI10121_IS_CHANGE);
    }

    return;
}

unsigned int sli10121_get_lcd_output_mode(void)
{
    return g_out_color_mode;
}

void sli10121_proc_remove(void)
{
//    printk("sli10121_proc_remove\n");

    if (sli10121_proc != NULL)
        ffb_remove_proc_entry(flcd_common_proc, sli10121_proc);
    sli10121_proc = NULL;

    if (hdmi_onoff_proc != NULL)
        ffb_remove_proc_entry(flcd_common_proc, hdmi_onoff_proc);
    hdmi_onoff_proc = NULL;

    if (edid_dump_proc != NULL)
        ffb_remove_proc_entry(flcd_common_proc, edid_dump_proc);
    edid_dump_proc = NULL;

    if (force_outmode_proc != NULL)
            ffb_remove_proc_entry(flcd_common_proc, force_outmode_proc);
    force_outmode_proc = NULL;

    if (force_yccmode_proc != NULL)
            ffb_remove_proc_entry(flcd_common_proc, force_yccmode_proc);
    force_yccmode_proc = NULL;
}

/*
 * dump the sli10121 register
 */
static int sli10121_proc_read_reg(char *page, char **start, off_t off, int count,
			  int *eof, void *data)
{
	int    len = 0, i;

	len += sprintf(page+len, "sli-hdmi register: \n");

    for (i = 0; i <= 0xff; i ++) {
        if (i % 10 == 0)
            len += sprintf(page+len, "\n");
        len += sprintf(page+len,"(0x%02x,0x%02x) ", i, I2C_ByteRead(i));
    }

	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;
	return len;
}
static ssize_t sli10121_proc_write_reg(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	unsigned char value[60];
	int addr,data;

	if(copy_from_user(value, buf, len))
		return 0;

	value[len] = '\0';
    sscanf(value, "%x %x\n",&addr,&data);
    I2C_ByteWrite(addr, data);
    data = I2C_ByteRead(addr);
    printk("(0x%x, 0x%x)\n",addr,data);

	return size;
}
/*
 * read edid dump on/off
 */
static int edid_dump_proc_read(char *page, char **start, off_t off, int count,
			  int *eof, void *data)
{
	int    len = 0;

	len += sprintf(page+len, "0 for OFF, 1 for ON \n");
    len += sprintf(page+len, "edid dump value = %d \n", g_dump_edid);

	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;
	return len;
}
static ssize_t edid_dump_proc_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	unsigned char value[10];
	int data;

	if(copy_from_user(value, buf, len))
		return 0;

	value[len] = '\0';
    sscanf(value, "%d\n", &data);
    if (data > 1) {
        printk("out of range! Please input 0 or 1 \n");
        return size;
    }

    if (g_dump_edid != data) {
        g_dump_edid = data;      
    }
    
	return size;
}

u32  Sli10121_Get_Output_Mode(void)
{
    return g_force_outmode;
}

u32  sli10121_Get_YCC_Mode(void)
{
    return g_force_yccmode;
}

/*
 * read edid dump on/off
 */


static int force_outm_proc_read(char *page, char **start, off_t off, int count,
			  int *eof, void *data)
{
	int    len = 0;

	len += sprintf(page+len, "0 is AUTO, 1 is RGB, 2 is YUV422 , 3 IS YUV444 \n");
    len += sprintf(page+len, "FORCE OUT MODE = %d \n", g_force_outmode);

	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;
	return len;
}
static ssize_t force_outm_proc_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	unsigned char value[10];
	int data;

	if(copy_from_user(value, buf, len))
		return 0;

	value[len] = '\0';
    sscanf(value, "%d\n", &data);
    if (data < 0 || data > 3) {
        printk("out of range! Please input 0~3 \n");
        return size;
    }

    if (g_force_outmode != data) {
        g_force_outmode = data;      
    }
    
	return size;
}

/*
 * read edid dump on/off
 */
static int force_yccm_proc_read(char *page, char **start, off_t off, int count,
			  int *eof, void *data)
{
	int    len = 0;

	len += sprintf(page+len, "0 is RGBTOYCC_SDTV_LIMITED_RANGE\n");
    len += sprintf(page+len, "1 is RGBTOYCC_SDTV_FULL_RANGE\n");
    len += sprintf(page+len, "2 is RGBTOYCC_HDTV_60HZ_RANGE\n");
    len += sprintf(page+len, "3 is RGBTOYCC_HDTV_50HZ_RANGE\n");
    len += sprintf(page+len, "FORCE YCC MODE = %d \n", g_force_yccmode);

	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;
	return len;
}
static ssize_t force_yccm_proc_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	unsigned char value[10];
	int data;

	if(copy_from_user(value, buf, len))
		return 0;

	value[len] = '\0';
    sscanf(value, "%d\n", &data);
    if (data < 0 || data > 3) {
        printk("out of range! Please input 0~3 \n");
        return size;
    }

    if (g_force_yccmode != data) {
        g_force_yccmode = data;      
    }
    
	return size;
}

/*
 * read hdmi state on/off
 */
static int hdmi_onoff_proc_read(char *page, char **start, off_t off, int count,
			  int *eof, void *data)
{
	int    len = 0;

	len += sprintf(page+len, "0 for OFF, 1 for ON \n");
    len += sprintf(page+len, "current value = %d \n", hdmi_onoff);

	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;
	return len;
}
static ssize_t hdmi_onoff_proc_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	unsigned char value[10];
	int data;

	if(copy_from_user(value, buf, len))
		return 0;

	value[len] = '\0';
    sscanf(value, "%d\n", &data);
    if (data > 1) {
        printk("out of range! Please input 0 or 1 \n");
        return size;
    }

    if (hdmi_onoff != data) {
        hdmi_onoff = data;
        HDMI_System_PD(PowerMode_A);
    }
	return size;
}

/* GM porting interface */
int sli10121_proc_init(void)
{
    int ret = 0;

//    printk("sli10121_proc_init sli10121_proc=0x%x\n",sli10121_proc);

    if (sli10121_proc != NULL)
        return 0;

    sli10121_proc = ffb_create_proc_entry("sli10121", S_IRUGO | S_IXUGO, flcd_common_proc);
	if (sli10121_proc == NULL) {
		printk("Fail to create proc for sli-hdmi!\n");
		ret = -EINVAL;
		goto end;
	}

	sli10121_proc->read_proc = (read_proc_t*)sli10121_proc_read_reg;
	sli10121_proc->write_proc = (write_proc_t*)sli10121_proc_write_reg;

    hdmi_onoff_proc = ffb_create_proc_entry("hdmi_onoff", S_IRUGO | S_IXUGO, flcd_common_proc);
	if (hdmi_onoff_proc == NULL) {
		printk("Fail to create proc for hdmi_onoff_proc!\n");
		ret = -EINVAL;
		goto end;
	}

	hdmi_onoff_proc->read_proc = (read_proc_t*)hdmi_onoff_proc_read;
	hdmi_onoff_proc->write_proc = (write_proc_t*)hdmi_onoff_proc_write;


    edid_dump_proc = ffb_create_proc_entry("edid_dump", S_IRUGO | S_IXUGO, flcd_common_proc);
	if (edid_dump_proc == NULL) {
		printk("Fail to create proc for edid_dump_proc!\n");
		ret = -EINVAL;
		goto end;
	}

	edid_dump_proc->read_proc = (read_proc_t*)edid_dump_proc_read;
	edid_dump_proc->write_proc = (write_proc_t*)edid_dump_proc_write;

    
    force_outmode_proc = ffb_create_proc_entry("force_out_mode", S_IRUGO | S_IXUGO, flcd_common_proc);
    if (force_outmode_proc == NULL) {
            printk("Fail to create proc for force_outmode_proc!\n");
            ret = -EINVAL;
            goto end;
    }
    
    force_outmode_proc->read_proc = (read_proc_t*)force_outm_proc_read;
    force_outmode_proc->write_proc = (write_proc_t*)force_outm_proc_write;

    force_yccmode_proc = ffb_create_proc_entry("force_ycc_mode", S_IRUGO | S_IXUGO, flcd_common_proc);
    if (force_yccmode_proc == NULL) {
            printk("Fail to create proc for force_yccmode_proc!\n");
            ret = -EINVAL;
            goto end;
    }
    
    force_yccmode_proc->read_proc = (read_proc_t*)force_yccm_proc_read;
    force_yccmode_proc->write_proc = (write_proc_t*)force_yccm_proc_write;    

end:
    return ret;
}

static void __exit sli10121_exit(void)
{
    int i;
    unsigned int chip_id;
    codec_driver_t    driver;

    if (g_sli10121_dev_id)  {}

    chip_id = (ftpmu010_get_attr(ATTR_TYPE_CHIPVER) >> 16) & 0xFFFF;
    if ((chip_id == 0x8282) || (chip_id == 0x8286))
        return;   /* without HDMI */

    for(i = 0 ; i < ARRAY_SIZE(out_type_table) ;i++) {
        driver.codec_id = out_type_table[i].type;
        codec_driver_deregister(&driver);
    }

    sli10121_i2c_exit();

#ifdef CONFIG_PLATFORM_GM8210
    if (hdmi_onoff_thread)
        kthread_stop(hdmi_onoff_thread);
    hdmi_onoff_thread = NULL;
    while (hdmi_thread_running)
        msleep(100);
#endif
}

void sli10121_reset_audio(void)
{
    int addr = 0x45, data;

    data = I2C_ByteRead(addr);
    data |= (0x1 << 2);
    I2C_ByteWrite(addr, data);

    //delay for a while
    msleep(1);

    data &= ~(0x1 << 2);
    I2C_ByteWrite(addr, data);
}
EXPORT_SYMBOL(sli10121_reset_audio);

module_init(sli10121_init);
module_exit(sli10121_exit);

MODULE_AUTHOR("Grain Media Corp.");
MODULE_LICENSE("GPL");

#undef _MAIN_C_

