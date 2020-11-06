//
// File Name   		:	MAIN.C
// Description 		:
// Ref. Docment		:
// Revision History 	:

// ----------------------------------------------------------------------
// Include files
// ----------------------------------------------------------------------
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <asm/uaccess.h>
#include <linux/i2c.h>
#include "lcd_def.h"
#include "codec.h"
#include "proc.h"
#include "mdin3xx.h"

// ----------------------------------------------------------------------
// Struct/Union Types and define
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Global Data section variables
// ----------------------------------------------------------------------
static struct task_struct *CI_thread = NULL;  //Check input thread
extern int   video_output_type;
extern int  video_input_vidsrc;
extern int  video_vidsrc_mode;
static struct proc_dir_entry *mdin340_proc = NULL;

// ----------------------------------------------------------------------
// External Variable && functions
// ----------------------------------------------------------------------
extern int mdin340_is_runing;
extern int mdin340_working_thread(void *private);
extern void ftiic010_set_clock_speed(void *, int hz);
// ----------------------------------------------------------------------
// Static Prototype Functions
// ----------------------------------------------------------------------
static int mdin340_proc_init(void);
static void mdin340_proc_remove(void);

// ----------------------------------------------------------------------
// Static functions
// ----------------------------------------------------------------------

static int mdin340_probe(void *private)
{
    if (CI_thread == NULL)
    {
        CI_thread = kthread_create(mdin340_working_thread, private, "mdin340");
        if (!IS_ERR(CI_thread))
        {
    		//wake_up_process(CI_thread);
            printk("MDIN340 Initialization is done. \n");
    	}
    	else
    	{
    		printk("MDIN340: Create check input thread failed\n");
    		return -1;
    	}
    }

    return 0;
}

void mdin340_setting(struct ffb_dev_info *info)
{
    static int wake_up = 0;

    switch (info->video_output_type)
{
      case VOT_1920x1080I:
        printk("Input 1920x1080i BT1120 to MDIN340\n");
        video_input_vidsrc = VIDSRC_1920x1080i60;
        video_output_type = VIDOUT_1920x1080p60;
        video_vidsrc_mode = MDIN_SRC_EMB422_8;
        break;
      case VOT_1920x1080P:
        printk("Input 1920x1080P BT1120 to MDIN340\n");
        video_input_vidsrc = VIDSRC_1920x1080p60;
        video_output_type = VIDOUT_1920x1080p60;
        video_vidsrc_mode = MDIN_SRC_EMB422_8;
        break;

      case VOT_480P:
        printk("Input 480P BT1120 to MDIN340\n");
        video_input_vidsrc = VIDSRC_720x480p60;
        video_output_type = VIDOUT_1024x768p60;
        video_vidsrc_mode = MDIN_SRC_EMB422_8;
        break;
      case VOT_576P:
        printk("Input 576P BT1120 to MDIN340\n");
        video_input_vidsrc = VIDSRC_720x576p50;
        video_output_type = VIDOUT_1024x768p60;
        video_vidsrc_mode = MDIN_SRC_EMB422_8;
        break;
      case VOT_NTSC:
        printk("Input 720x480i BT656 to MDIN340\n");
        video_input_vidsrc = VIDSRC_720x480i60;
        video_output_type = VIDOUT_1024x768p60;
        video_vidsrc_mode = MDIN_SRC_MUX656_8;
        break;
      case VOT_PAL:
        printk("Input 720x576i BT656 to MDIN340\n");
        video_input_vidsrc = VIDSRC_720x576i50;
        video_output_type = VIDOUT_1024x768p60;
        video_vidsrc_mode = MDIN_SRC_MUX656_8;
        break;
      default:
        printk("Error input %d to mdin340!!! \n", info->video_output_type);
        break;
}

    printk("%s, video_input_vidsrc = %d, video_output_type = %d, video_vidsrc_mode = %d \n", __func__,
                            video_input_vidsrc, video_output_type, video_vidsrc_mode);

    if (!wake_up) {
        wake_up_process(CI_thread);
        wake_up = 1;
    }

    return;
}

void mdin340_remove(void *private)
{
    if (CI_thread)
		kthread_stop(CI_thread);

    while (mdin340_is_runing)
        msleep(10);

    CI_thread = NULL;
}

struct mdin340_i2c {
	struct i2c_client *iic_client;
	struct i2c_adapter *iic_adapter;
} *p_mdin340_i2c = NULL;

static int __devinit mdin340_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

    if (!(p_mdin340_i2c = kzalloc(sizeof(struct mdin340_i2c), GFP_KERNEL))){
		printk("%s fail: kzalloc not OK.\n", __FUNCTION__);
		return -ENOMEM;
	}

	p_mdin340_i2c->iic_client = client;
	p_mdin340_i2c->iic_adapter = adapter;

	i2c_set_clientdata(client, p_mdin340_i2c);

	return 0;
}

static int __devexit mdin340_i2c_remove(struct i2c_client *client)
{
    if (client) {}

	kfree(p_mdin340_i2c);

	return 0;
}

static const struct i2c_device_id mdin340_id[] = {
        { "mdin340", 0 },
        { }
};

static struct i2c_driver mdin340_driver = {
        .driver = {
                .name   = "mdin340",
                .owner  = THIS_MODULE,
        },
        .probe          = mdin340_i2c_probe,
        .remove         = __devexit_p(mdin340_i2c_remove),
        .id_table       = mdin340_id
};

static struct i2c_board_info mdin340_device = {
        .type           = "mdin340",
        .addr           = (I2C_MDIN3xx_ADDR >> 1),
};


static int __init mdin340_init(void)
{
    codec_driver_t    driver;
    static int  i2c_init = 0;

    if (i2c_init)
        return 0;

    i2c_init = 1;

    if (i2c_new_device(i2c_get_adapter(0), &mdin340_device) == NULL)
    {
        printk("%s fail: i2c_new_device not OK.\n", __FUNCTION__);
        return -1;
    }

    if (i2c_add_driver(&mdin340_driver))
    {
        printk("%s fail: i2c_add_driver not OK.\n", __FUNCTION__);
        return -1;
    }

    /* SD 480i */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "mdin340-720x480x60");
    driver.codec_id = OUTPUT_TYPE_MDIN340_720x480_58;
    driver.probe = mdin340_probe;   //probe function
    driver.setting = mdin340_setting;
    driver.enc_proc_init = mdin340_proc_init;
    driver.enc_proc_remove = mdin340_proc_remove;
    driver.remove = mdin340_remove;
    if (codec_driver_register(&driver) < 0) {
        printk("register mdin340-720x480x60 to pip fail! \n");
        return -1;
    }

    /* SD 576i */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "mdin340-720x576x50");
    driver.codec_id = OUTPUT_TYPE_MDIN340_720x576_59;
    driver.probe = mdin340_probe;   //probe function
    driver.setting = mdin340_setting;
    driver.enc_proc_init = mdin340_proc_init;
    driver.enc_proc_remove = mdin340_proc_remove;
    driver.remove = mdin340_remove;
    if (codec_driver_register(&driver) < 0) {
        printk("register mdin340-720x576x50 to pip fail! \n");
        return -1;
    }

    /* SD 480p */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "mdin340-720x480x60P");
    driver.codec_id = OUTPUT_TYPE_MDIN340_480P_60;
    driver.probe = mdin340_probe;   //probe function
    driver.setting = mdin340_setting;
    driver.enc_proc_init = mdin340_proc_init;
    driver.enc_proc_remove = mdin340_proc_remove;
    driver.remove = mdin340_remove;
    if (codec_driver_register(&driver) < 0) {
        printk("register mdin340-720x480x60P to pip fail! \n");
        return -1;
    }

    /* SD 576p */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "mdin340-720x576x50P");
    driver.codec_id = OUTPUT_TYPE_MDIN340_576P_61;
    driver.probe = mdin340_probe;   //probe function
    driver.setting = mdin340_setting;
    driver.enc_proc_init = mdin340_proc_init;
    driver.enc_proc_remove = mdin340_proc_remove;
    driver.remove = mdin340_remove;
    if (codec_driver_register(&driver) < 0) {
        printk("register mdin340-720x576x50P to pip fail! \n");
        return -1;
    }

    /* 1920x1080P */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "mdin340-1920x1080x60P");
    driver.codec_id = OUTPUT_TYPE_MDIN340_1920x1080P_62;
    driver.probe = mdin340_probe;   //probe function
    driver.setting = mdin340_setting;
    driver.enc_proc_init = mdin340_proc_init;
    driver.enc_proc_remove = mdin340_proc_remove;
    driver.remove = mdin340_remove;
    if (codec_driver_register(&driver) < 0) {
        printk("register mdin340-1920x1080p to pip fail! \n");
        return -1;
    }

    /* 1920x1080i */
    memset(&driver, 0, sizeof(codec_driver_t));
    strcpy(driver.name, "mdin340-1920x1080i");
    driver.codec_id = OUTPUT_TYPE_MDIN340_1920x1080_63;
    driver.probe = mdin340_probe;   //probe function
    driver.setting = mdin340_setting;
    driver.enc_proc_init = mdin340_proc_init;
    driver.enc_proc_remove = mdin340_proc_remove;
    driver.remove = mdin340_remove;
    if (codec_driver_register(&driver) < 0) {
        printk("register mdin340-1920x1080i to pip fail! \n");
        return -1;
    }

    return 0;
}

static void __exit mdin340_exit(void)
{
    codec_driver_t    driver;

    driver.codec_id = OUTPUT_TYPE_MDIN340_720x480_58;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_MDIN340_720x576_59;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_MDIN340_480P_60;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_MDIN340_576P_61;
    codec_driver_deregister(&driver);

    driver.codec_id = OUTPUT_TYPE_MDIN340_1920x1080P_62;
    codec_driver_deregister(&driver);

    driver.codec_id =  OUTPUT_TYPE_MDIN340_1920x1080_63;
    codec_driver_deregister(&driver);
}

/*
 * Proc interface
 */
static int proc_read_mdin340(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int     len = 0;

    return len;
}

int mdin340_proc_init(void)
{
    int ret = 0;

    if (mdin340_proc != NULL)
        return 0;

    mdin340_proc = ffb_create_proc_entry("mdin340", S_IRUGO | S_IXUGO, flcd_common_proc);
	if (mdin340_proc == NULL) {
		printk("Fail to create proc for mdin340!\n");
		ret = -EINVAL;
		goto end;
	}

	mdin340_proc->read_proc = (read_proc_t *)proc_read_mdin340;
	mdin340_proc->write_proc = (write_proc_t *)NULL;

end:
    return ret;
}

void mdin340_proc_remove(void)
{
    if (mdin340_proc != NULL)
        ffb_remove_proc_entry(flcd_common_proc, mdin340_proc);

    mdin340_proc = NULL;
}


//--------------------------------------------------------------------------------------------------
// Drive Function for delay (usec and msec)
// You must make functions which is defined below.
//--------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINDLY_10uSec(WORD delay)
{
    udelay(delay);

    return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINDLY_mSec(WORD delay)
{
    msleep(delay);

    return MDIN_NO_ERROR;
}

// ----------------------------------------------------------------------
// Struct/Union Types and define
// ----------------------------------------------------------------------
#define		I2C_OK				0
#define		I2C_NOT_FREE		1
#define		I2C_HOST_NACK		2
#define		I2C_TIME_OUT		3

// ----------------------------------------------------------------------
// Static Global Data section variables
// ----------------------------------------------------------------------
static WORD PageID = 0;

// ----------------------------------------------------------------------
// External Variable
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Prototype Functions
// ----------------------------------------------------------------------

// You must make functions which is defined below.
static BYTE MDINI2C_Write(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes);
static BYTE MDINI2C_Read(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes);

// ----------------------------------------------------------------------
// Static functions
// ----------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------
static BYTE MDINI2C_SetPage(BYTE nID, WORD page)
{
#if	defined(SYSTEM_USE_MDIN380)&&defined(SYSTEM_USE_BUS_HIF)
	MDINBUS_SetPageID(page);	// set pageID to BUS-IF
#endif

	if (page==PageID) return I2C_OK;	PageID = page;
	return MDINI2C_Write(nID, 0x400, (PBYTE)&page, 2);	// write page
}

//--------------------------------------------------------------------------------------------------------------------------
static BYTE MHOST_I2CWrite(WORD rAddr, PBYTE pBuff, WORD bytes)
{
	BYTE err = I2C_OK;

	err = MDINI2C_SetPage(MDIN_HOST_ID, 0x0000);	// write host page
	if (err) return err;

	err = MDINI2C_Write(MDIN_HOST_ID, rAddr, pBuff, bytes);
	return err;
}

//--------------------------------------------------------------------------------------------------------------------------
static BYTE MHOST_I2CRead(WORD rAddr, PBYTE pBuff, WORD bytes)
{
	BYTE err = I2C_OK;

	err = MDINI2C_SetPage(MDIN_HOST_ID, 0x0000);	// write host page
	if (err) return err;

	err = MDINI2C_Read(MDIN_HOST_ID, rAddr, pBuff, bytes);
	return err;
}

//--------------------------------------------------------------------------------------------------------------------------
static BYTE LOCAL_I2CWrite(WORD rAddr, PBYTE pBuff, WORD bytes)
{
	BYTE err = I2C_OK;

	err = MDINI2C_SetPage(MDIN_LOCAL_ID, 0x0101);	// write local page
	if (err) return err;

	err = MDINI2C_Write(MDIN_LOCAL_ID, rAddr, pBuff, bytes);
	return err;
}

//--------------------------------------------------------------------------------------------------------------------------
static BYTE LOCAL_I2CRead(WORD rAddr, PBYTE pBuff, WORD bytes)
{
	WORD RegOEN, err = I2C_OK;

	if		(rAddr>=0x030&&rAddr<0x036)	RegOEN = 0x04;	// mfc-size
	else if (rAddr>=0x043&&rAddr<0x045)	RegOEN = 0x09;	// out-ptrn
	else if (rAddr>=0x062&&rAddr<0x083)	RegOEN = 0x09;	// enhance
	else if (rAddr>=0x088&&rAddr<0x092)	RegOEN = 0x09;	// out-sync
	else if (rAddr>=0x094&&rAddr<0x097)	RegOEN = 0x09;	// out-sync
	else if (rAddr>=0x09a&&rAddr<0x09c)	RegOEN = 0x09;	// bg-color
	else if (rAddr>=0x0a0&&rAddr<0x0d0)	RegOEN = 0x09;	// out-ctrl
	else if (              rAddr<0x100)	RegOEN = 0x01;	// in-sync
	else if (rAddr>=0x100&&rAddr<0x140)	RegOEN = 0x05;	// main-fc
	else if (rAddr>=0x140&&rAddr<0x1a0)	RegOEN = 0x07;	// aux
	else if (rAddr>=0x1a0&&rAddr<0x1c0)	RegOEN = 0x03;	// arbiter
	else if (rAddr>=0x1c0&&rAddr<0x1e0)	RegOEN = 0x02;	// fc-mc
	else if (rAddr>=0x1e0&&rAddr<0x1f8)	RegOEN = 0x08;	// encoder
	else if (rAddr>=0x1f8&&rAddr<0x200)	RegOEN = 0x0a;	// audio
	else if (rAddr>=0x200&&rAddr<0x280)	RegOEN = 0x04;	// ipc
	else if (rAddr>=0x2a0&&rAddr<0x300)	RegOEN = 0x07;	// aux-osd
	else if (rAddr>=0x300&&rAddr<0x380)	RegOEN = 0x06;	// osd
	else if (rAddr>=0x3c0&&rAddr<0x3f8)	RegOEN = 0x09;	// enhance
	else								RegOEN = 0x00;	// host state

	err = LOCAL_I2CWrite(0x3ff, (PBYTE)&RegOEN, 2);	// write reg_oen
	if (err) return err;

	err = MDINI2C_Read(MDIN_LOCAL_ID, rAddr, pBuff, bytes);
	return err;
}

#if defined(SYSTEM_USE_MDIN340)||defined(SYSTEM_USE_MDIN380)
//--------------------------------------------------------------------------------------------------------------------------
static BYTE MHDMI_I2CWrite(WORD rAddr, PBYTE pBuff, WORD bytes)
{
	BYTE err = I2C_OK;

	err = MDINI2C_SetPage(MDIN_HDMI_ID, 0x0202);	// write hdmi page
	if (err) return err;

	err = MDINI2C_Write(MDIN_HDMI_ID, rAddr/2, (PBYTE)pBuff, bytes);
	return err;
}

//--------------------------------------------------------------------------------------------------------------------------
static BYTE MHDMI_GetWriteDone(void)
{
	WORD rVal = 0, count = 100, err = I2C_OK;

	while (count&&(rVal==0)) {
		err = MDINI2C_Read(MDIN_HDMI_ID, 0x027, (PBYTE)&rVal, 2);
		if (err) return err;	rVal &= 0x04;	count--;
	}
	return (count)? I2C_OK : I2C_TIME_OUT;
}

//--------------------------------------------------------------------------------------------------------------------------
static BYTE MHDMI_HOSTRead(WORD rAddr, PBYTE pBuff)
{
	WORD rData, err = I2C_OK;

	err = MDINI2C_SetPage(MDIN_HOST_ID, 0x0000);	// write host page
	if (err) return err;

	err = MDINI2C_Write(MDIN_HOST_ID, 0x025, (PBYTE)&rAddr, 2);
	if (err) return err;	rData = 0x0003;
	err = MDINI2C_Write(MDIN_HOST_ID, 0x027, (PBYTE)&rData, 2);
	if (err) return err;	rData = 0x0002;
	err = MDINI2C_Write(MDIN_HOST_ID, 0x027, (PBYTE)&rData, 2);
	if (err) return err;

	// check done flag
	err = MHDMI_GetWriteDone(); if (err) {mdinERR = 4; return err;}

	err = MDINI2C_Read(MDIN_HOST_ID, 0x026, (PBYTE)pBuff, 2);
	return err;
}

//--------------------------------------------------------------------------------------------------------------------------
static BYTE MHDMI_I2CRead(WORD rAddr, PBYTE pBuff, WORD bytes)
{
	BYTE err = I2C_OK;

	// DDC_STATUS, DDC_FIFOCNT
	if (rAddr==0x0f2||rAddr==0x0f5) return MHDMI_HOSTRead(rAddr, pBuff);

	err = MDINI2C_SetPage(MDIN_HDMI_ID, 0x0202);	// write hdmi page
	if (err) return err;

	err = MDINI2C_Read(MDIN_HDMI_ID, rAddr/2, (PBYTE)pBuff, bytes);
	return err;
}
#endif	/* defined(SYSTEM_USE_MDIN340)||defined(SYSTEM_USE_MDIN380) */

//--------------------------------------------------------------------------------------------------------------------------
static BYTE SDRAM_I2CWrite(DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	WORD row, len, idx, unit, err = I2C_OK;

	err = MDINI2C_RegRead(MDIN_HOST_ID, 0x005, &unit);	if (err) return err;
	unit = (unit&0x0100)? 4096 : 2048;

	while (bytes>0) {
		row = ADDR2ROW(rAddr, unit);	// get row
		idx = ADDR2COL(rAddr, unit);	// get col
		len = MIN((unit/2)-(rAddr%(unit/2)), bytes);

		err = MDINI2C_RegWrite(MDIN_HOST_ID, 0x003, row); if (err) return err;	// host access
		err = MDINI2C_SetPage(MDIN_HOST_ID, 0x0303); if (err) return err;	// write sdram page
		err = MDINI2C_Write(MDIN_SDRAM_ID, idx/2, (PBYTE)pBuff, len); if (err) return err;
		bytes-=len; rAddr+=len; pBuff+=len;
	}
	return err;
}

//--------------------------------------------------------------------------------------------------------------------------
static BYTE SDRAM_I2CRead(DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	WORD row, len, idx, unit, err = I2C_OK;

	err = MDINI2C_RegRead(MDIN_HOST_ID, 0x005, &unit);	if (err) return err;
	unit = (unit&0x0100)? 4096 : 2048;

	while (bytes>0) {
		row = ADDR2ROW(rAddr, unit);	// get row
		idx = ADDR2COL(rAddr, unit);	// get col
		len = MIN((unit/2)-(rAddr%(unit/2)), bytes);

		err = MDINI2C_RegWrite(MDIN_HOST_ID, 0x003, row); if (err) return err;	// host access
		err = MDINI2C_SetPage(MDIN_HOST_ID, 0x0303); if (err) return err;	// write sdram page
		err = MDINI2C_Read(MDIN_SDRAM_ID, idx/2, (PBYTE)pBuff, len); if (err) return err;
		bytes-=len; rAddr+=len; pBuff+=len;
	}
	return err;
}

//--------------------------------------------------------------------------------------------------------------------------
static BYTE I2C_WriteByID(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	BYTE err = I2C_OK;

	switch (nID&0xfe) {
		case MDIN_HOST_ID:	err = MHOST_I2CWrite(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_LOCAL_ID:	err = LOCAL_I2CWrite(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_SDRAM_ID:	err = SDRAM_I2CWrite(rAddr, (PBYTE)pBuff, bytes); break;

	#if defined(SYSTEM_USE_MDIN340)||defined(SYSTEM_USE_MDIN380)
		case MDIN_HDMI_ID:	err = MHDMI_I2CWrite(rAddr, (PBYTE)pBuff, bytes); break;
	#endif

	}
	return err;
}

//--------------------------------------------------------------------------------------------------------------------------
static BYTE I2C_ReadByID(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	BYTE err = I2C_OK;

	switch (nID&0xfe) {
		case MDIN_HOST_ID:	err = MHOST_I2CRead(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_LOCAL_ID:	err = LOCAL_I2CRead(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_SDRAM_ID:	err = SDRAM_I2CRead(rAddr, (PBYTE)pBuff, bytes); break;

	#if defined(SYSTEM_USE_MDIN340)||defined(SYSTEM_USE_MDIN380)
		case MDIN_HDMI_ID:	err = MHDMI_I2CRead(rAddr, (PBYTE)pBuff, bytes); break;
	#endif

	}
	return err;
}

//--------------------------------------------------------------------------------------------------------------------------
#if	defined(SYSTEM_USE_MDIN380)&&defined(SYSTEM_USE_BUS_HIF)
MDIN_ERROR_t MDINI2C_SetPageID(WORD nID)
{
	PageID = nID;
	return MDIN_NO_ERROR;
}
#endif

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINI2C_MultiWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	return (I2C_WriteByID(nID, rAddr, (PBYTE)pBuff, bytes))? MDIN_I2C_ERROR : MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINI2C_RegWrite(BYTE nID, DWORD rAddr, WORD wData)
{
	return (MDINI2C_MultiWrite(nID, rAddr, (PBYTE)&wData, 2))? MDIN_I2C_ERROR : MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINI2C_MultiRead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	return (I2C_ReadByID(nID, rAddr, (PBYTE)pBuff, bytes))? MDIN_I2C_ERROR : MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINI2C_RegRead(BYTE nID, DWORD rAddr, PWORD rData)
{
	return (MDINI2C_MultiRead(nID, rAddr, (PBYTE)rData, 2))? MDIN_I2C_ERROR : MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINI2C_RegField(BYTE nID, DWORD rAddr, WORD bPos, WORD bCnt, WORD bData)
{
	WORD temp;

	if (bPos>15||bCnt==0||bCnt>16||(bPos+bCnt)>16) return MDIN_INVALID_PARAM;
	if (MDINI2C_RegRead(nID, rAddr, &temp)) return MDIN_I2C_ERROR;
	bCnt = ~(0xffff<<bCnt);
	temp &= ~(bCnt<<bPos);
	temp |= ((bData&bCnt)<<bPos);
	return (MDINI2C_RegWrite(nID, rAddr, temp))? MDIN_I2C_ERROR : MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for I2C read & I2C write
// You must make functions which is defined below.
//--------------------------------------------------------------------------------------------------------------------------
#include <linux/i2c.h>
#define MDIN_ISC_DIV        60 //60k
#define I2C_DATA_LEN        257

unsigned char buffer[I2C_DATA_LEN];

extern int GM_i2c_xfer(struct i2c_msg *msgs, int num, int clockdiv);

static BYTE MDINI2C_Write(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes)
{
    BYTE sAddr = I2C_MDIN3xx_ADDR;

    int i;
    struct i2c_msg  msgs[2];
	unsigned char buf[2];
	unsigned short *temp, value;

	if (bytes >= 2) {
    	/* swap */
    	for (i = 0; i < bytes; i += 2)
    	{
    		temp = (unsigned short *)&pBuff[i];
    		value = BYTESWAP(*temp);
    		memcpy(&buffer[i], &value, 2);
    	}
    } else {
        buffer[0] = *pBuff;
    }

    buf[0]          = rAddr >> 8;
	buf[1]          = (rAddr & 0x00FF);
	msgs[0].addr    = sAddr >> 1;
	msgs[0].flags   = 0; /* 0 for write */
	msgs[0].len     = 2;
	msgs[0].buf     = buf;

    /* Repeated START */
	msgs[1].addr    = sAddr >> 1;
	msgs[1].flags   = 0; /* 0 for write */
	msgs[1].len     = bytes + 1;
	msgs[1].buf     = buffer;

	//ftiic010_set_clock_speed(i2c_get_adapdata(p_mdin340_i2c->iic_adapter), MDIN_ISC_DIV);
	if (unlikely(i2c_transfer(p_mdin340_i2c->iic_adapter, msgs, 2) != 2)) {
		return I2C_TIME_OUT;
	}

    msleep(1);

    return I2C_OK;
}

static BYTE MDINI2C_Read(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes)
{
    BYTE sAddr = I2C_MDIN3xx_ADDR;
    int i;
    struct i2c_msg  msgs[2];
	unsigned char   buf[2];
	unsigned short  *temp;

    if (unlikely(p_mdin340_i2c->iic_adapter == NULL)) {
		printk("p_mdin340_i2c->iic_adapter is NULL \n");
		return I2C_TIME_OUT;
	}

    memset(buf, 0, sizeof(buf));
    buf[0]          = rAddr >> 8;
	buf[1]          = (rAddr & 0x00FF);
    msgs[0].addr    = sAddr >> 1;
	msgs[0].flags   = 0; /* 0 for write */
	msgs[0].len     = 2;
	msgs[0].buf     = buf;
    /* Repeated START */
	msgs[1].addr    = (sAddr + 1) >> 1;
	msgs[1].flags   = 1; /* 1 for Read */
	msgs[1].len     = bytes;
	msgs[1].buf     = pBuff;

	//ftiic010_set_clock_speed(i2c_get_adapdata(p_mdin340_i2c->iic_adapter), MDIN_ISC_DIV);
	if (unlikely(i2c_transfer(p_mdin340_i2c->iic_adapter, msgs, 2) != 2)) {
		return I2C_TIME_OUT;
	}

    if (bytes >= 2) {
    	/* swap */
    	for (i=0; i<bytes; i+=2)
    	{
    		temp = (unsigned short *)&pBuff[i];
    		*temp = BYTESWAP(*temp);
    	}
	}
	msleep(1);
    return I2C_OK;
}

module_init(mdin340_init);
module_exit(mdin340_exit);

MODULE_AUTHOR("MicroImage");
MODULE_DESCRIPTION("mdin340 driver");
MODULE_LICENSE("GPL");
