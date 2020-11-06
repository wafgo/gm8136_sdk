#ifdef KENNY_CHANGE
#include "typedef.h"
#include "gm_i2c.h"
#include "codec.h"
#else
#include <c8051f340.h>
#include <stddef.h>
#include "I2C_header.h"
#endif
#include "USB_API.h"
#include "HDMI_header.h"
#include "HDMI_TxRegs.h"
#include <linux/delay.h>

//#define MonM

//  IP Conversion option. Set by the board switch setting in the main()
BYTE RE_SETTING = 0;   //
//  HDCP option. Set by the board switch setting in the main()
#ifdef KENNY_CHANGE
BYTE HDCP_ENABLED = 0; //  (0 - disable HDCP, 1 - enable HDCP)
#else
BYTE HDCP_ENABLED = 1; //  (0 - disable HDCP, 1 - enable HDCP)
#endif
BYTE HDCP_BLACK = 0;   //  (0 - no black screen, 1 - black screen when HDCP off) COMP_MODE only!
#define BLACK_SCREEN_ENABLED    P3 = (P3 | 0x80)
#define BLACK_SCREEN_DISABLED   P3 = (P3 & 0x7F)
#define EDID_RETRY_COUNTER      3

#ifdef SLI11131
BYTE IP_ON;             // IP Conversion(0 - disable IP Conversion, 1 - enable IP Conversion)
#endif      // SLI11131

/*EDID parse definition*/

#define EDID_LENGTH                             0x80

#define EDID_HEADER                             0x00
#define EDID_HEADER_END                         0x07

#define EDID_STRUCT_VERSION			0x12
#define EDID_STRUCT_REVISION			0x13

#define ESTABLISHED_TIMING_1                    0x23
#define ESTABLISHED_TIMING_2                    0x24
#define MANUFACTURERS_TIMINGS                   0x25

#define ESTABLISHED_STANDARD_TIMING             0x26
#define ESTABLISHED_STANDARD_TIMING_NUM         8
#define ESTABLISHED_STANDARD_TIMING_SIZE        2

#define DETAILED_TIMING_DESCRIPTIONS_START      0x36
#define DETAILED_TIMING_DESCRIPTION_SIZE        18
#define DETAILED_TIMING_DESCRIPTIONS_NUM        4



#define DETAILED_TIMING_DESCRIPTION_1           0x36
#define DETAILED_TIMING_DESCRIPTION_2           0x48
#define DETAILED_TIMING_DESCRIPTION_3           0x5a
#define DETAILED_TIMING_DESCRIPTION_4           0x6c

/*EDID EXT definition*/
#define EDID_EXT_BEGINE                         0x80

#define EDID_EXT_HDMI_IDENTIFIER                0x000C03
#define EDID_EXT_AUDIO_BLOCK	                0x01
#define EDID_EXT_VIDEO_BLOCK                    0x02
#define EDID_EXT_VENDOR_BLOCK                   0x03
#define EDID_EXT_SPEAKER_BLOCK	                0x04
#define EDID_EXT_EDID_BASIC_AUDIO	            (1 << 6)

#define EDID_SUCCESS                            0
#define EDID_FAIL                               -1

#define EDID_SUPPORT_TIMING_NUM                 ARRAY_SIZE(edid_support_res_array)

EDID_Support_setting_t edid_support_res_array[] = {
	{VID_01_640x480p          ,  640 , 480, 0 ,60 ,0,"EDID 640x480 60HZ"},/*1*/
	{VID_02_720x480p          ,  720 , 480, 0 ,60 ,0,"EDID 720x480 60HZ"},/*2*/
	{VID_04_1280x720p         , 1280 , 720, 0 ,60 ,0,"EDID 1280x720 60HZ"},/*3*/
	{VID_08_720x240p          ,  720 , 240, 0 ,60 ,0,"EDID 720x240 60HZ"},/*4*/
	{VID_12_2880x240p         , 2880 , 240, 0 ,60 ,0,"EDID 2880x240 60HZ"},/*5*/
	{VID_14_1440x480p         , 1440 , 480, 0 ,60 ,0,"EDID 1440x480 60HZ"},/*6*/
	{VID_16_1920x1080p        , 1920 ,1080, 0 ,60 ,0,"EDID 1920x1080 60HZ"},/*7*/
	{VID_23_720x288p          ,  720 , 288, 0 ,60 ,0,"EDID 720x288 60HZ"},/*8*/
	{VID_27_2880x288p         , 2880 , 288, 0 ,60 ,0,"EDID 2880x288 60HZ"},/*9*/
	{VID_29_1440x576p         , 1440 , 576, 0 ,60 ,0,"EDID 1440x576 60HZ"},/*10*/
	{VID_35_2880x480p         , 2880 , 480, 0 ,60 ,0,"EDID 2880x480 60HZ"},/*11*/
	{VID_37_2880x576p         , 2880 , 576, 0 ,60 ,0,"EDID 2880x576 60HZ"},/*12*/
	{VID_42_720x576p          ,  720 , 576, 0 ,60 ,0,"EDID 720x576 60HZ"},/*13*/
	{VID_60_1024x768_RGB_EXT  , 1024 , 768, 0 ,60 ,0,"EDID 1024x768 60HZ"},/*14*/
	{VID_61_1280x1024_RGB_EXT , 1280 ,1024, 0 ,60 ,0,"EDID 1280x1024 60HZ"},/*15*/
	{-1                       , 0    ,0   , 0 , 0 ,0,""},    /* terminated */
};

EDID_Support_setting_t edid_cea81_mode_array[] = {
	{0, 640, 480, 0 ,60, 0,"640x480p@59.94/60Hz"},/*1*/
	{0, 720, 480, 0 ,60, 0,"720x480p@59.94/60Hz"},/*2*/
	{0, 720, 480, 0 ,60, 0,"720x480p@59.94/60Hz-W"},/*3*/
	{0,1280, 720, 0 ,60, 0,"1280x720p@59.94/60Hz"},/*4*/
	{0,1920,1080, 0 ,60, 1,"1920x1080i@59.94/60Hz"},/*5*/
	{0,1440, 480, 0 ,60, 1,"720(1440)x480i@59.94/60Hz"},/*6*/
	{0,1440, 480, 0 ,60, 1,"720(1440)x480i@59.94/60Hz-W"},/*7*/
	{0,1440, 240, 0 ,60, 0,"720(1440)x240p@59.94/60Hz"},/*8*/
	{0,1440, 240, 0 ,60, 0,"720(1440)x240p@59.94/60Hz-W"},/*9*/
	{0,2880, 480, 0 ,60, 1,"(2880)x480i@59.94/60Hz"},/*10*/
	{0,2880, 480, 0 ,60, 1,"(2880)x480i@59.94/60Hz-W"},/*11*/
	{0,2880, 480, 0 ,60, 1,"(2880)x240p@59.94/60Hz"},/*12*/
	{0,2880, 480, 0 ,60, 1,"(2880)x240p@59.94/60Hz-W"},/*13*/
	{0,1440, 480, 0 ,60, 0,"1440x480p@59.94/60Hz"},/*14*/
	{0,1440, 480, 0 ,60, 0,"1440x480p@59.94/60Hz-W"},/*15*/
	{0,1920,1080, 0 ,60, 0,"1920x1080p@59.94/60Hz"},/*16*/
	{0, 720, 576, 0 ,50, 0,"720x576p@50Hz"},/*17*/
	{0, 720, 576, 0 ,50, 0,"720x576p@50Hz-W"},/*18*/
	{0,1280, 720, 0 ,50, 0,"1280x720p@50Hz"},/*19*/
	{0,1920,1080, 0 ,50, 1,"1920x1080i@50Hz"},/*20*/
	{0,1440, 576, 0 ,50, 1,"720(1440)x576i@50Hz"},/*21*/
	{0,1440, 576, 0 ,50, 1,"720(1440)x576i@50Hz-W"},/*22*/
	{0,1440, 288, 0 ,50, 0,"720(1440)x288p@50Hz"},/*23*/
	{0,1440, 288, 0 ,50, 0,"720(1440)x288p@50Hz-W"},/*24*/
	{0,2880, 576, 0 ,50, 1,"(2880)x576i@50Hz"},/*25*/
	{0,2880, 576, 0 ,50, 1,"(2880)x576i@50Hz-W"},/*26*/
	{0,2880, 288, 0 ,50, 0,"(2880)x288p@50Hz"},/*27*/
	{0,2880, 288, 0 ,50, 0,"(2880)x288p@50Hz-W"},/*28*/
	{0,1440, 576, 0 ,50, 0,"1440x576p@50Hz"},/*29*/
	{0,1440, 576, 0 ,50, 0,"1440x576p@50Hz-W"},/*30*/
	{0,1920,1080, 0 ,50, 0,"1920x1080p@50Hz"},/*31*/
	{0,1920,1080, 0 ,24, 0,"1920x1080p@23.98/24Hz"},/*32*/
	{0,1920,1080, 0 ,25, 0,"1920x1080p@25Hz"},/*33*/
	{0,1920,1080, 0 ,30, 0,"1920x1080p@29.97/30Hz"},/*34*/
	{0,2880, 480, 0 ,60, 0,"(2880)x480p@59.94/60Hz"},/*35*/
	{0,2880, 480, 0 ,60, 0,"(2880)x480p@59.94/60Hz-W"},/*36*/
	{0,2880, 576, 0 ,50, 0,"(2880)x576p@50Hz"},/*37*/
	{0,2880, 576, 0 ,50, 0,"(2880)x576p @ 50Hz-W"},/*38*/
	{0,1920,1080, 0 ,50, 1,"1920x1080i(1250 Total)@50Hz"},/*39*/
	{0,1920,1080, 0 ,100,1,"1920x1080i@100Hz"},/*40*/
	{0,1280, 720, 0 ,100,0,"1280x720p@100Hz"},/*41*/
	{0, 720, 576, 0 ,100,0,"720x576p@100Hz"},/*42*/
	{0, 720, 576, 0 ,100,0,"720x576p@100Hz-W"},/*43*/
	{0,1440, 576, 0 ,100,1,"720(1440)x576i@100Hz"},/*44*/
	{0,1440, 576, 0 ,100,1,"720(1440)x576i@100Hz-W"},/*45*/
	{0,1920,1080, 0 ,120,1,"1920x1080i@119.88/120Hz"},/*46*/
	{0,1280, 720, 0 ,120,0,"1280x720p@119.88/120Hz"},/*47*/
	{0, 720, 480, 0 ,120,0,"720x480p@119.88/120Hz"},/*48*/
	{0, 720, 480, 0 ,120,0,"720x480p@119.88/120Hz-W"},/*49*/
	{0,1440, 480, 0 ,120,1,"720(1440)x480i@119.88/120Hz"},/*50*/
	{0,1440, 480, 0 ,120,1,"720(1440)x480i@119.88/120Hz-W"},/*51*/
	{0, 720, 576, 0 ,200,0,"720x576p@200Hz"},/*52*/
	{0, 720, 576, 0 ,200,0,"720x576p@200Hz-W"},/*53*/
	{0,1440, 576, 0 ,200,1,"720(1440)x576i@200Hz"},/*54*/
	{0,1440, 576, 0 ,200,1,"720(1440)x576i@200Hz-W"},/*55*/
	{0, 720, 480, 0 ,240,0,"720x480p@239.76/240Hz"},/*56*/
	{0, 720, 480, 0 ,240,0,"720x480p@239.76/240Hz-W"},/*57*/
	{0,1440, 480, 0 ,240,1,"720(1440)x480i@239.76/240Hz"},/*58*/
	{0,1440, 480, 0 ,240,1,"720(1440)x480i@239.76/240Hz-W"},/*59*/
	{0,1280, 720, 0 , 24,0,"1280x720p@23.98/24Hz"},/*60*/
	{0,1280, 720, 0 , 25,0,"1280x720p@25Hz"},/*61*/
	{0,1280, 720, 0 , 30,0,"1280x720p@29.97/30Hz"},/*62*/
	{0,1920,1080, 0 ,120,0,"1920x1080p@119.88/120Hz"},/*63*/
};


#define EDID_SUPPORT_CEA81_MODE_NUM  (sizeof(edid_cea81_mode_array) / sizeof(edid_cea81_mode_array[0]))

const BYTE edid_v1x_header[] = { 0x00, 0xff, 0xff, 0xff,
                                0xff, 0xff, 0xff, 0x00 };

int edid_parse_established_timer(int do_ext,unsigned int block_num);

int edid_check_block0_header(void);

enum {
RGBTOYCC_SDTV_LIMIT_RANGE  = 0,
RGBTOYCC_SDTV_FULL_RANGE     ,
RGBTOYCC_HDTV_60HZ_RANGE     ,
RGBTOYCC_HDTV_50HZ_RANGE     ,
RGBTOYCC_MAX_ENRTRY
};

enum {
FROCE_OUT_MODE_AUTO  = 0,
FROCE_OUT_MODE_RGB      ,
FROCE_OUT_MODE_YUV422     ,
FROCE_OUT_MODE_YUV444 
};

#define RGBTOTCC_COEFFICENCE_NUM     24
#define RGBTOYCC_TOTAL_NUM           (RGBTOYCC_MAX_ENRTRY*RGBTOTCC_COEFFICENCE_NUM)

unsigned char g_rgbtoycc_parameter [RGBTOYCC_TOTAL_NUM] = {
0x11,0xAD,0x02,0x00,0x10,0x53,0x00,0x80,0x02,0x59,0x01,0x32,0x00,0x75,0x00,0x00,0x11,0x53,0x10,0xAD,0x02,0x00,0x00,0x80,
0x11,0x79,0x01,0xC2,0x10,0x49,0x00,0x80,0x02,0x04,0x01,0x07,0x00,0x64,0x00,0x10,0x11,0x2A,0x10,0x98,0x01,0xC2,0x00,0x80,
0x11,0xD1,0x02,0x00,0x10,0x2F,0x00,0x80,0x02,0xDC,0x00,0xDA,0x00,0x4A,0x00,0x00,0x11,0x8B,0x10,0x75,0x02,0x00,0x00,0x80,
0x11,0xAD,0x02,0x00,0x10,0x53,0x00,0x80,0x02,0x59,0x01,0x32,0x00,0x75,0x00,0x00,0x11,0x53,0x10,0xAD,0x02,0x00,0x00,0x80
};

#define COLOR_TRANSFORM_AUTO       0
#define COLOR_TRANSFORM_MANUAL     1

//-----------------------------------------------------------------------------
// Global Variable
//-----------------------------------------------------------------------------
BYTE HDMI_interrupt = 0;                // Set by HDMI ISR
BYTE CtrlPkt_Enable = 0x00;             // Control Packet. Set after EDID read.
BYTE CtrlPkt_Auto = 0;
BYTE CtrlPkt_Manual = 0;
BYTE HDCP_ON = 0;           // HDCP

HDMI_Setting_t HDMI_setting;            // HDMI Chip Setting structure

#ifdef KENNY_CHANGE
sli10121_ext_frame_timing g_ext_frame_timing[]=
{
    {VID_60_1024x768_RGB_EXT,   0 /* neg */, 0 ,136, 160, 1024, 24, 6, 29,  768, 3}, /* 65MHZ */
    {VID_61_1280x1024_RGB_EXT,  1 /* pos */, 1, 112, 248, 1280, 48, 3, 38, 1024, 1}, /* 108MHZ */
};

BYTE xdata  EDID_Data[128 * 8] = {0};
static volatile int in_bottom_isr = 0, external_reg_update = 0;
#else
BYTE xdata  EDID_Data[128 * 8] _at_ 0;  // Can save 8 blocks of EDID
#endif

BYTE xdata  BKSV_Data[5];
BYTE PowerMode = PowerMode_A;

// LED on test board
sbit Led1 = P2^0;
sbit Led2 = P2^1;                       // LED='1' means ON
sbit Led3 = P2^2;
sbit Led4 = P2^3;
sbit Led5 = P2^4;
sbit Led6 = P2^5;
sbit Led7 = P2^6;
extern BYTE COMP_MODE;                      // Compliance Mode setting (Board DIP switch)

// Debug: Log State History
BYTE xdata history[15];
BYTE hist_i = 0;

#ifdef MonM
// USB Monitor
BYTE USB_MCU_MON;
extern BYTE USB_Packet[16];                   // Last packet received from host
#endif

// Watch Dog Timer for state of HDMI to fix
extern BYTE WD_TIMER_CNT;

static char *hdmi_state_str[] = {
    "HDMI_STATE_ERROR",
    "HDMI_STATE_IDLE",
    "HDMI_STATE_HOTPLUG WAIT",
    "HDMI_STATE_EDID_START",
    "HDMI_STATE_EDID_READY",
    "HDMI_STATE_EDID_READ",
    "HDMI_STATE_EDID_PROCESS",
    "HDMI_STATE_TX_SETTING",
    "HDMI_STATE_TX_START",
    "HDMI_STATE_TX_RUNNING",
    "HDMI_STATE_HDCP_START",
    "HDMI_STATE_HDCP_READY",
    "HDMI_STATE_HDCP_READ",
    "HDMI_STATE_HDCP_AUTH",
    "HDMI_STATE_END"
};

#define HDMI_PRINT_MODE(x)  { \
            if ((x) & 0x10) printk("%s, PowerMode_A \n", __func__); \
            if ((x) & 0x20) printk("%s, PowerMode_B \n", __func__); \
            if ((x) & 0x40) printk("%s, PowerMode_D \n", __func__); \
            if ((x) & 0x80) printk("%s, PowerMode_E \n", __func__); }


#define HDMI_DEBUG_PRINT  printk("%s, state is: %s \n", __func__, hdmi_state_str[HDMI_STATE])

//-----------------------------------------------------------------------------
// function prototype definitions
//-----------------------------------------------------------------------------
//static void packet_strncpy (BYTE, char *, int);
static u32 rdy_cnt = 0;
/*get lcd output mode*/
extern unsigned int sli10121_get_lcd_output_mode(void);
extern u32 g_dump_edid;

/* 1 for ready, 0 for not ready */
int get_hdmi_is_ready(bool tmode)
{
    if(tmode){
        if ((PowerMode == PowerMode_E) && (rdy_cnt >= 1))
            return 1;
    }
    else{
        if ((PowerMode == PowerMode_E) && (rdy_cnt >= 20))
            return 1;
    }

    return 0;
}

BYTE get_hdmi_INT94_status(void)
{
    BYTE INT_92h, INT_94h;

    INT_92h = I2C_ByteRead(X92_INT_MASK1);
    INT_94h = I2C_ByteRead (X94_INT1_ST);

    return (INT_92h & INT_94h);
}

//-----------------------------------------------------------------------------
// HDMI_init
//-----------------------------------------------------------------------------
//
// Initialize global settings for SLI10130
//
void HDMI_init (void)
{
    printk("%s, 1 \n", __func__);

#if 1 /* Harry */
    HDMI_System_PD (PowerMode_B);
    I2C_ByteWrite(0x00, 0x2C);
    mdelay(10);
    I2C_ByteWrite(0x00, 0x28);
    mdelay(10);
    HDMI_System_PD(PowerMode_A);
    mdelay(10);
#endif

    // Set to power mode B, in order to read/write to registers
    HDMI_System_PD (PowerMode_B);
    Delay();

    I2C_ByteWrite (X17_DC_REG,   0x20); /*  dc_soh_clr */

    // Control Packet init
    I2C_ByteWrite (X5F_PACKET_INDEX, ACP_PACKET); // Index.1 ACP
    I2C_ByteWrite (X60_PACKET_HB0, 0x04); // HB0
    I2C_ByteWrite (X61_PACKET_HB1, 0x00); // HB1
    I2C_ByteWrite (X62_PACKET_HB2, 0x00); // HB2
    I2C_ByteWrite (X63_PACKET_PB0, 0x00); // PB0
    I2C_ByteWrite (X64_PACKET_PB1, 0x00); // PB1
    I2C_ByteWrite (X65_PACKET_PB2, 0x00); // PB2
    I2C_ByteWrite (X66_PACKET_PB3, 0x00); // PB3
    I2C_ByteWrite (X67_PACKET_PB4, 0x00); // PB4
    I2C_ByteWrite (X68_PACKET_PB5, 0x00); // PB5
    I2C_ByteWrite (X69_PACKET_PB6, 0x00); // PB6
    I2C_ByteWrite (X6A_PACKET_PB7, 0x00); // PB7
    I2C_ByteWrite (X6B_PACKET_PB8, 0x00); // PB8
    I2C_ByteWrite (X6C_PACKET_PB9, 0x00); // PB9
    I2C_ByteWrite (X6D_PACKET_PB10, 0x00); // PB10
    I2C_ByteWrite (X6E_PACKET_PB11, 0x00); // PB11
    I2C_ByteWrite (X6F_PACKET_PB12, 0x00); // PB12
    I2C_ByteWrite (X70_PACKET_PB13, 0x00); // PB13
    I2C_ByteWrite (X71_PACKET_PB14, 0x00); // PB14
    I2C_ByteWrite (X72_PACKET_PB15, 0x00); // PB15
    I2C_ByteWrite (X73_PACKET_PB16, 0x00); // PB16
    I2C_ByteWrite (X74_PACKET_PB17, 0x00); // PB17
    I2C_ByteWrite (X75_PACKET_PB18, 0x00); // PB18
    I2C_ByteWrite (X76_PACKET_PB19, 0x00); // PB19
    I2C_ByteWrite (X77_PACKET_PB20, 0x00); // PB20
    I2C_ByteWrite (X78_PACKET_PB21, 0x00); // PB21
    I2C_ByteWrite (X79_PACKET_PB22, 0x00); // PB22
    I2C_ByteWrite (X7A_PACKET_PB23, 0x00); // PB23
    I2C_ByteWrite (X7B_PACKET_PB24, 0x00); // PB24
    I2C_ByteWrite (X7C_PACKET_PB25, 0x00); // PB25
    I2C_ByteWrite (X7D_PACKET_PB26, 0x00); // PB26
    I2C_ByteWrite (X7E_PACKET_PB27, 0x00); // PB27

    I2C_ByteWrite (X5F_PACKET_INDEX, ISRC1_PACKET); // Index.2 ISRC1
    I2C_ByteWrite (X60_PACKET_HB0, 0x05); // HB0
    I2C_ByteWrite (X61_PACKET_HB1, 0xC2); // HB1
    I2C_ByteWrite (X62_PACKET_HB2, 0x00); // HB2
    I2C_ByteWrite (X63_PACKET_PB0, 0x00); // PB0
    I2C_ByteWrite (X64_PACKET_PB1, 0x00); // PB1
    I2C_ByteWrite (X65_PACKET_PB2, 0x00); // PB2
    I2C_ByteWrite (X66_PACKET_PB3, 0x00); // PB3
    I2C_ByteWrite (X67_PACKET_PB4, 0x00); // PB4
    I2C_ByteWrite (X68_PACKET_PB5, 0x00); // PB5
    I2C_ByteWrite (X69_PACKET_PB6, 0x00); // PB6
    I2C_ByteWrite (X6A_PACKET_PB7, 0x00); // PB7
    I2C_ByteWrite (X6B_PACKET_PB8, 0x00); // PB8
    I2C_ByteWrite (X6C_PACKET_PB9, 0x00); // PB9
    I2C_ByteWrite (X6D_PACKET_PB10, 0x00); // PB10
    I2C_ByteWrite (X6E_PACKET_PB11, 0x00); // PB11
    I2C_ByteWrite (X6F_PACKET_PB12, 0x00); // PB12
    I2C_ByteWrite (X70_PACKET_PB13, 0x00); // PB13
    I2C_ByteWrite (X71_PACKET_PB14, 0x00); // PB14
    I2C_ByteWrite (X72_PACKET_PB15, 0x00); // PB15
    I2C_ByteWrite (X73_PACKET_PB16, 0x00); // PB16
    I2C_ByteWrite (X74_PACKET_PB17, 0x00); // PB17
    I2C_ByteWrite (X75_PACKET_PB18, 0x00); // PB18
    I2C_ByteWrite (X76_PACKET_PB19, 0x00); // PB19
    I2C_ByteWrite (X77_PACKET_PB20, 0x00); // PB20
    I2C_ByteWrite (X78_PACKET_PB21, 0x00); // PB21
    I2C_ByteWrite (X79_PACKET_PB22, 0x00); // PB22
    I2C_ByteWrite (X7A_PACKET_PB23, 0x00); // PB23
    I2C_ByteWrite (X7B_PACKET_PB24, 0x00); // PB24
    I2C_ByteWrite (X7C_PACKET_PB25, 0x00); // PB25
    I2C_ByteWrite (X7D_PACKET_PB26, 0x00); // PB26
    I2C_ByteWrite (X7E_PACKET_PB27, 0x00); // PB27

    I2C_ByteWrite (X5F_PACKET_INDEX, ISRC2_PACKET); // Index.3 ISRC2
    I2C_ByteWrite (X60_PACKET_HB0, 0x06); // HB0
    I2C_ByteWrite (X61_PACKET_HB1, 0x00); // HB1
    I2C_ByteWrite (X62_PACKET_HB2, 0x00); // HB2
    I2C_ByteWrite (X63_PACKET_PB0, 0x00); // PB0
    I2C_ByteWrite (X64_PACKET_PB1, 0x00); // PB1
    I2C_ByteWrite (X65_PACKET_PB2, 0x00); // PB2
    I2C_ByteWrite (X66_PACKET_PB3, 0x00); // PB3
    I2C_ByteWrite (X67_PACKET_PB4, 0x00); // PB4
    I2C_ByteWrite (X68_PACKET_PB5, 0x00); // PB5
    I2C_ByteWrite (X69_PACKET_PB6, 0x00); // PB6
    I2C_ByteWrite (X6A_PACKET_PB7, 0x00); // PB7
    I2C_ByteWrite (X6B_PACKET_PB8, 0x00); // PB8
    I2C_ByteWrite (X6C_PACKET_PB9, 0x00); // PB9
    I2C_ByteWrite (X6D_PACKET_PB10, 0x00); // PB10
    I2C_ByteWrite (X6E_PACKET_PB11, 0x00); // PB11
    I2C_ByteWrite (X6F_PACKET_PB12, 0x00); // PB12
    I2C_ByteWrite (X70_PACKET_PB13, 0x00); // PB13
    I2C_ByteWrite (X71_PACKET_PB14, 0x00); // PB14
    I2C_ByteWrite (X72_PACKET_PB15, 0x00); // PB15
    I2C_ByteWrite (X73_PACKET_PB16, 0x00); // PB16
    I2C_ByteWrite (X74_PACKET_PB17, 0x00); // PB17
    I2C_ByteWrite (X75_PACKET_PB18, 0x00); // PB18
    I2C_ByteWrite (X76_PACKET_PB19, 0x00); // PB19
    I2C_ByteWrite (X77_PACKET_PB20, 0x00); // PB20
    I2C_ByteWrite (X78_PACKET_PB21, 0x00); // PB21
    I2C_ByteWrite (X79_PACKET_PB22, 0x00); // PB22
    I2C_ByteWrite (X7A_PACKET_PB23, 0x00); // PB23
    I2C_ByteWrite (X7B_PACKET_PB24, 0x00); // PB24
    I2C_ByteWrite (X7C_PACKET_PB25, 0x00); // PB25
    I2C_ByteWrite (X7D_PACKET_PB26, 0x00); // PB26
    I2C_ByteWrite (X7E_PACKET_PB27, 0x00); // PB27

    I2C_ByteWrite (X5F_PACKET_INDEX, GAMUT_PACKET); // Index.4 Gamut
    I2C_ByteWrite (X60_PACKET_HB0, 0x0A); // HB0
    I2C_ByteWrite (X61_PACKET_HB1, 0x00); // HB1
    I2C_ByteWrite (X62_PACKET_HB2, 0xB0); // HB2
    I2C_ByteWrite (X63_PACKET_PB0, 0x00); // PB0
    I2C_ByteWrite (X64_PACKET_PB1, 0x00); // PB1
    I2C_ByteWrite (X65_PACKET_PB2, 0x00); // PB2
    I2C_ByteWrite (X66_PACKET_PB3, 0x00); // PB3
    I2C_ByteWrite (X67_PACKET_PB4, 0x00); // PB4
    I2C_ByteWrite (X68_PACKET_PB5, 0x00); // PB5
    I2C_ByteWrite (X69_PACKET_PB6, 0x00); // PB6
    I2C_ByteWrite (X6A_PACKET_PB7, 0x00); // PB7
    I2C_ByteWrite (X6B_PACKET_PB8, 0x00); // PB8
    I2C_ByteWrite (X6C_PACKET_PB9, 0x00); // PB9
    I2C_ByteWrite (X6D_PACKET_PB10, 0x00); // PB10
    I2C_ByteWrite (X6E_PACKET_PB11, 0x00); // PB11
    I2C_ByteWrite (X6F_PACKET_PB12, 0x00); // PB12
    I2C_ByteWrite (X70_PACKET_PB13, 0x00); // PB13
    I2C_ByteWrite (X71_PACKET_PB14, 0x00); // PB14
    I2C_ByteWrite (X72_PACKET_PB15, 0x00); // PB15
    I2C_ByteWrite (X73_PACKET_PB16, 0x00); // PB16
    I2C_ByteWrite (X74_PACKET_PB17, 0x00); // PB17
    I2C_ByteWrite (X75_PACKET_PB18, 0x00); // PB18
    I2C_ByteWrite (X76_PACKET_PB19, 0x00); // PB19
    I2C_ByteWrite (X77_PACKET_PB20, 0x00); // PB20
    I2C_ByteWrite (X78_PACKET_PB21, 0x00); // PB21
    I2C_ByteWrite (X79_PACKET_PB22, 0x00); // PB22
    I2C_ByteWrite (X7A_PACKET_PB23, 0x00); // PB23
    I2C_ByteWrite (X7B_PACKET_PB24, 0x00); // PB24
    I2C_ByteWrite (X7C_PACKET_PB25, 0x00); // PB25
    I2C_ByteWrite (X7D_PACKET_PB26, 0x00); // PB26
    I2C_ByteWrite (X7E_PACKET_PB27, 0x00); // PB27

    I2C_ByteWrite (X5F_PACKET_INDEX, VENDOR_INFO_PACKET); // Index.5 Vender Specidic InfoFrame
    I2C_ByteWrite (X60_PACKET_HB0, 0x81); // HB0
    I2C_ByteWrite (X61_PACKET_HB1, 0x01); // HB1 InfoFrame Ver
    I2C_ByteWrite (X62_PACKET_HB2, 0x00); // HB2
    I2C_ByteWrite (X63_PACKET_PB0, 0x00); // PB0
    I2C_ByteWrite (X64_PACKET_PB1, 0x00); // PB1
    I2C_ByteWrite (X65_PACKET_PB2, 0x00); // PB2
    I2C_ByteWrite (X66_PACKET_PB3, 0x00); // PB3
    I2C_ByteWrite (X67_PACKET_PB4, 0x00); // PB4
    I2C_ByteWrite (X68_PACKET_PB5, 0x00); // PB5
    I2C_ByteWrite (X69_PACKET_PB6, 0x00); // PB6
    I2C_ByteWrite (X6A_PACKET_PB7, 0x00); // PB7
    I2C_ByteWrite (X6B_PACKET_PB8, 0x00); // PB8
    I2C_ByteWrite (X6C_PACKET_PB9, 0x00); // PB9
    I2C_ByteWrite (X6D_PACKET_PB10, 0x00); // PB10
    I2C_ByteWrite (X6E_PACKET_PB11, 0x00); // PB11
    I2C_ByteWrite (X6F_PACKET_PB12, 0x00); // PB12
    I2C_ByteWrite (X70_PACKET_PB13, 0x00); // PB13
    I2C_ByteWrite (X71_PACKET_PB14, 0x00); // PB14
    I2C_ByteWrite (X72_PACKET_PB15, 0x00); // PB15
    I2C_ByteWrite (X73_PACKET_PB16, 0x00); // PB16
    I2C_ByteWrite (X74_PACKET_PB17, 0x00); // PB17
    I2C_ByteWrite (X75_PACKET_PB18, 0x00); // PB18
    I2C_ByteWrite (X76_PACKET_PB19, 0x00); // PB19
    I2C_ByteWrite (X77_PACKET_PB20, 0x00); // PB20
    I2C_ByteWrite (X78_PACKET_PB21, 0x00); // PB21
    I2C_ByteWrite (X79_PACKET_PB22, 0x00); // PB22
    I2C_ByteWrite (X7A_PACKET_PB23, 0x00); // PB23
    I2C_ByteWrite (X7B_PACKET_PB24, 0x00); // PB24
    I2C_ByteWrite (X7C_PACKET_PB25, 0x00); // PB25
    I2C_ByteWrite (X7D_PACKET_PB26, 0x00); // PB26
    I2C_ByteWrite (X7E_PACKET_PB27, 0x00); // PB27

    // Setting AVI InfoFrame, for RGB
    I2C_ByteWrite (X5F_PACKET_INDEX, AVI_INFO_PACKET); // Index.6 AVI InfoFrame
    I2C_ByteWrite (X60_PACKET_HB0, 0x82); // HB0
    I2C_ByteWrite (X61_PACKET_HB1, 0x02); // HB1
    I2C_ByteWrite (X62_PACKET_HB2, 0x0D); // HB2
    I2C_ByteWrite (X63_PACKET_PB0, 0x00); // PB0
    I2C_ByteWrite (X64_PACKET_PB1, 0x00); // PB1
    I2C_ByteWrite (X65_PACKET_PB2, 0x28); // PB2    /* 16:9 */
    I2C_ByteWrite (X66_PACKET_PB3, 0x00); // PB3
    I2C_ByteWrite (X67_PACKET_PB4, 0x02); // PB4
    I2C_ByteWrite (X68_PACKET_PB5, 0x00); // PB5
    I2C_ByteWrite (X69_PACKET_PB6, 0x00); // PB6
    I2C_ByteWrite (X6A_PACKET_PB7, 0x00); // PB7
    I2C_ByteWrite (X6B_PACKET_PB8, 0x00); // PB8
    I2C_ByteWrite (X6C_PACKET_PB9, 0x00); // PB9
    I2C_ByteWrite (X6D_PACKET_PB10, 0x00); // PB10
    I2C_ByteWrite (X6E_PACKET_PB11, 0x00); // PB11
    I2C_ByteWrite (X6F_PACKET_PB12, 0x00); // PB12
    I2C_ByteWrite (X70_PACKET_PB13, 0x00); // PB13
    I2C_ByteWrite (X71_PACKET_PB14, 0x00); // PB14
    I2C_ByteWrite (X72_PACKET_PB15, 0x00); // PB15
    I2C_ByteWrite (X73_PACKET_PB16, 0x00); // PB16
    I2C_ByteWrite (X74_PACKET_PB17, 0x00); // PB17
    I2C_ByteWrite (X75_PACKET_PB18, 0x00); // PB18
    I2C_ByteWrite (X76_PACKET_PB19, 0x00); // PB19
    I2C_ByteWrite (X77_PACKET_PB20, 0x00); // PB20
    I2C_ByteWrite (X78_PACKET_PB21, 0x00); // PB21
    I2C_ByteWrite (X79_PACKET_PB22, 0x00); // PB22
    I2C_ByteWrite (X7A_PACKET_PB23, 0x00); // PB23
    I2C_ByteWrite (X7B_PACKET_PB24, 0x00); // PB24
    I2C_ByteWrite (X7C_PACKET_PB25, 0x00); // PB25
    I2C_ByteWrite (X7D_PACKET_PB26, 0x00); // PB26
    I2C_ByteWrite (X7E_PACKET_PB27, 0x00); // PB27

    // Setting Source Product Description InfoFrame (Ref. CEA-861-D, table 14, p.73)
    I2C_ByteWrite (X5F_PACKET_INDEX, PRODUCT_INFO_PACKET); // Index.7 Source Product Descriptor
    I2C_ByteWrite (X60_PACKET_HB0, 0x83); // PH0:  InfoFrame Type    0x83
    I2C_ByteWrite (X61_PACKET_HB1, 0x01); // PH1:  InfoFrame Version 0x01
    I2C_ByteWrite (X62_PACKET_HB2, 25);   // PH2:  Length of Source Product Description 25
    I2C_ByteWrite (X63_PACKET_PB0, 'S');  // PB1:  Vendor Name 1
    I2C_ByteWrite (X64_PACKET_PB1, 'L');  // PB2:  Vendor Name 2
    I2C_ByteWrite (X65_PACKET_PB2, 'I');  // PB3:  Vendor Name 3
    I2C_ByteWrite (X66_PACKET_PB3, ' ');  // PB4:  Vendor Name 4
    I2C_ByteWrite (X67_PACKET_PB4, 'U');  // PB5:  Vendor Name 5
    I2C_ByteWrite (X68_PACKET_PB5, 'S');  // PB6:  Vendor Name 6
    I2C_ByteWrite (X69_PACKET_PB6, 'A');  // PB7:  Vendor Name 7
    I2C_ByteWrite (X6A_PACKET_PB7, 0);    // PB8:  Vendor Name 8
    I2C_ByteWrite (X6B_PACKET_PB8, 'S');  // PB9:  Product Description 1
    I2C_ByteWrite (X6C_PACKET_PB9, 'L');  // PB10: Product Description 2
    I2C_ByteWrite (X6D_PACKET_PB10, 'I');  // PB11: Product Description 3
    I2C_ByteWrite (X6E_PACKET_PB11, '1');  // PB12: Product Description 4
    I2C_ByteWrite (X6F_PACKET_PB12, '0');  // PB13: Product Description 5
    I2C_ByteWrite (X70_PACKET_PB13, '1');  // PB14: Product Description 6
    I2C_ByteWrite (X71_PACKET_PB14, '3');  // PB15: Product Description 7
    I2C_ByteWrite (X72_PACKET_PB15, '1');  // PB16: Product Description 8
    I2C_ByteWrite (X73_PACKET_PB16, ' ');  // PB17: Product Description 9
    I2C_ByteWrite (X74_PACKET_PB17, 'H');  // PB18: Product Description 10
    I2C_ByteWrite (X75_PACKET_PB18, 'D');  // PB19: Product Description 11
    I2C_ByteWrite (X76_PACKET_PB19, 'M');  // PB20: Product Description 12
    I2C_ByteWrite (X77_PACKET_PB20, 'I');  // PB21: Product Description 13
    I2C_ByteWrite (X78_PACKET_PB21, ' ');  // PB22: Product Description 14
    I2C_ByteWrite (X79_PACKET_PB22, 'T');  // PB23: Product Description 15
    I2C_ByteWrite (X7A_PACKET_PB23, 'x');  // PB24: Product Description 16
    I2C_ByteWrite (X7B_PACKET_PB24, 0x02); // PB25: Source Device Information (2 for DVD Player)
    I2C_ByteWrite (X7C_PACKET_PB25, 0x00); // PB25
    I2C_ByteWrite (X7D_PACKET_PB26, 0x00); // PB26
    I2C_ByteWrite (X7E_PACKET_PB27, 0x00); // PB27

    I2C_ByteWrite (X5F_PACKET_INDEX, AUDIO_INFO_PACKET); // Index.8 Audio
    I2C_ByteWrite (X60_PACKET_HB0, 0x84); // HB0
    I2C_ByteWrite (X61_PACKET_HB1, 0x01); // HB1
    I2C_ByteWrite (X62_PACKET_HB2, 0x0A); // HB2
    I2C_ByteWrite (X63_PACKET_PB0, 0x00); // PB0
    I2C_ByteWrite (X64_PACKET_PB1, 0x00); // PB1
    I2C_ByteWrite (X65_PACKET_PB2, 0x00); // PB2
    I2C_ByteWrite (X66_PACKET_PB3, 0x00); // PB3
    I2C_ByteWrite (X67_PACKET_PB4, 0x00); // PB4
    I2C_ByteWrite (X68_PACKET_PB5, 0x00); // PB5
    I2C_ByteWrite (X69_PACKET_PB6, 0x00); // PB6
    I2C_ByteWrite (X6A_PACKET_PB7, 0x00); // PB7
    I2C_ByteWrite (X6B_PACKET_PB8, 0x00); // PB8
    I2C_ByteWrite (X6C_PACKET_PB9, 0x00); // PB9
    I2C_ByteWrite (X6D_PACKET_PB10, 0x00); // PB10
    I2C_ByteWrite (X6E_PACKET_PB11, 0x00); // PB11
    I2C_ByteWrite (X6F_PACKET_PB12, 0x00); // PB12
    I2C_ByteWrite (X70_PACKET_PB13, 0x00); // PB13
    I2C_ByteWrite (X71_PACKET_PB14, 0x00); // PB14
    I2C_ByteWrite (X72_PACKET_PB15, 0x00); // PB15
    I2C_ByteWrite (X73_PACKET_PB16, 0x00); // PB16
    I2C_ByteWrite (X74_PACKET_PB17, 0x00); // PB17
    I2C_ByteWrite (X75_PACKET_PB18, 0x00); // PB18
    I2C_ByteWrite (X76_PACKET_PB19, 0x00); // PB19
    I2C_ByteWrite (X77_PACKET_PB20, 0x00); // PB20
    I2C_ByteWrite (X78_PACKET_PB21, 0x00); // PB21
    I2C_ByteWrite (X79_PACKET_PB22, 0x00); // PB22
    I2C_ByteWrite (X7A_PACKET_PB23, 0x00); // PB23
    I2C_ByteWrite (X7B_PACKET_PB24, 0x00); // PB24
    I2C_ByteWrite (X7C_PACKET_PB25, 0x00); // PB25
    I2C_ByteWrite (X7D_PACKET_PB26, 0x00); // PB26
    I2C_ByteWrite (X7E_PACKET_PB27, 0x00); // PB27

    I2C_ByteWrite (X5F_PACKET_INDEX, MPEG_SRC_INFO_PACKET); // Index.9 MPEG
    I2C_ByteWrite (X60_PACKET_HB0, 0x85); // HB0
    I2C_ByteWrite (X61_PACKET_HB1, 0x01); // HB1
    I2C_ByteWrite (X62_PACKET_HB2, 0x0A); // HB2
    I2C_ByteWrite (X63_PACKET_PB0, 0x80); // PB0
    I2C_ByteWrite (X64_PACKET_PB1, 0x96); // PB1
    I2C_ByteWrite (X65_PACKET_PB2, 0x98); // PB2
    I2C_ByteWrite (X66_PACKET_PB3, 0x00); // PB3
    I2C_ByteWrite (X67_PACKET_PB4, 0x00); // PB4
    I2C_ByteWrite (X68_PACKET_PB5, 0x00); // PB5
    I2C_ByteWrite (X69_PACKET_PB6, 0x00); // PB6
    I2C_ByteWrite (X6A_PACKET_PB7, 0x00); // PB7
    I2C_ByteWrite (X6B_PACKET_PB8, 0x00); // PB8
    I2C_ByteWrite (X6C_PACKET_PB9, 0x00); // PB9
    I2C_ByteWrite (X6D_PACKET_PB10, 0x00); // PB10
    I2C_ByteWrite (X6E_PACKET_PB11, 0x00); // PB11
    I2C_ByteWrite (X6F_PACKET_PB12, 0x00); // PB12
    I2C_ByteWrite (X70_PACKET_PB13, 0x00); // PB13
    I2C_ByteWrite (X71_PACKET_PB14, 0x00); // PB14
    I2C_ByteWrite (X72_PACKET_PB15, 0x00); // PB15
    I2C_ByteWrite (X73_PACKET_PB16, 0x00); // PB16
    I2C_ByteWrite (X74_PACKET_PB17, 0x00); // PB17
    I2C_ByteWrite (X75_PACKET_PB18, 0x00); // PB18
    I2C_ByteWrite (X76_PACKET_PB19, 0x00); // PB19
    I2C_ByteWrite (X77_PACKET_PB20, 0x00); // PB20
    I2C_ByteWrite (X78_PACKET_PB21, 0x00); // PB21
    I2C_ByteWrite (X79_PACKET_PB22, 0x00); // PB22
    I2C_ByteWrite (X7A_PACKET_PB23, 0x00); // PB23
    I2C_ByteWrite (X7B_PACKET_PB24, 0x00); // PB24
    I2C_ByteWrite (X7C_PACKET_PB25, 0x00); // PB25
    I2C_ByteWrite (X7D_PACKET_PB26, 0x00); // PB26
    I2C_ByteWrite (X7E_PACKET_PB27, 0x00); // PB27

    HDCP_ON = HDCP_ENABLED;

#ifdef SLI10121
    // NON-HDCP SETTING - skip phy setting override
    if (!HDCP_ON)
        I2C_ByteWrite (XCF_HDCP_MEM_CTRL2, 0xC0);   // FIXME: Undocumented in V1.00d spec
#endif    // #ifdef SLI10121


    //I2C_ByteWrite (X17_DC_REG, 0x22);

    I2C_ByteWrite (0x46, 0x00); //JUN recommend

    HDMI_System_PD (PowerMode_A);
}


//-----------------------------------------------------------------------------
// HDMI_ISR_top
//-----------------------------------------------------------------------------
//
// Top half ISR for external interrupt from HDMI chip. Flag and disable INT0.
// External interrupt (INT0) will be re-enabled in the bottom half ISR.
//

void HDMI_ISR_top (void)
{
    // disable external interrupt (INT0)
    EX0 = 0;    // (same as IE &= 0xFE;)

    // raise HDMI interrupt flag
    HDMI_interrupt = 1;
}

void WDmonTM_Disable(void)
{
   ET2     = 0;                           // Desable Timer2 Intterupt
   TR2     = 0;                           // Clear and Stop Timer2
   TMR2CN  = 0x00;

#ifdef KENNY_CHANGE
    stop_timer();
#endif
}


void WDmonTM_Enable(void)
{
   TMR2CN  = 0x00;

   ET2     = 1;                           // Enable Timer2 Intterupt
   TR2     = 1;                           // Start Timer2
   WD_TIMER_CNT=0;

#ifdef KENNY_CHANGE
    start_timer();
#endif

}

//-----------------------------------------------------------------------------
// HDMI_ISR_bottom: MAIN STATE MACHINE
//-----------------------------------------------------------------------------
//
// Bottom half ISR for external interrupt (INT0) from HDMI chip.
// Modify & Add code here to change state machine.
//
void HDMI_ISR_bottom (void)
{
    static BYTE HDMI_STATE = HDMI_STATE_IDLE;
    BYTE HDMI_STATE_CHK = 0;      // Flow Control FLG for HDMI_STATE_IDLE
    static BYTE EDID_current = 0;
    static BYTE EDID_size = 0;
    static BYTE EDID_retry = EDID_RETRY_COUNTER;
    static BYTE EDID_process_done = 0;
    #define EDID_WORD   ((EDID_current % 2) ? 0x80 : 0x00)
    #define EDID_SEG    (EDID_current / 2)
    #define EDID_EXT    (EDID_Data[126])
    static BYTE VSYNC_counter = 0;
    // Reg value save
    BYTE INT_94h = 0;
    BYTE INT_95h = 0;
    BYTE checksum = 0;
    BYTE temp = 0;
    BYTE GM_INT_94h = 0;
    int  i,j;
    while (external_reg_update)
        msleep(1);

    in_bottom_isr = 1;

    if (PowerMode == PowerMode_E)
        rdy_cnt ++;

    if (PowerMode == PowerMode_A)
    {
        // PS mode a -> b ###
        // I2C Register Full Access
        PowerMode = PowerMode_B;
        HDMI_System_PD (PowerMode);
    }

    // 2009.06.16 Interrupt Handling.
    //            The following is the interrupt handling that there was in a case sentence
    if (HDMI_interrupt)
    {
        // Remove HDMI interrupt flag
        HDMI_interrupt = 0;

        // Save interrupt status from the last interrupt
        INT_94h = I2C_ByteRead (X94_INT1_ST);
        INT_95h = I2C_ByteRead (X95_INT2_ST);

        printk("SLI HDMI_interrupt, MASK_92h = 0x%x, INT_94h = 0x%x \n",
                    I2C_ByteRead(X92_INT_MASK1), INT_94h);

        // When plug-in or plug-out detect, go to status of HDMI_STATE_IDLE.
        if ((INT_94h & HOT_PLUG) && (PowerMode <= PowerMode_B))
        {
            HDMI_clear_to_IDLE();

            HDCP_ON = HDCP_ENABLED;
            if (HDCP_ON)
                HDMI_HDCPclear_to_IDLE();

            HDMI_STATE = HDMI_STATE_IDLE; /* kick off from IDLE state */

            HDMI_STATE_CHK=2;
        }

        /* workaround, sometimes we got a lot of vsync interrupt. I don't know why */
        if (PowerMode == PowerMode_E) {
            BYTE tmp;

            tmp = I2C_ByteRead (X92_INT_MASK1);
            tmp &= ~(0x1 << 5); //disable vsync
            I2C_ByteWrite(X92_INT_MASK1, tmp);
        }

        // clear all interrupts
        I2C_ByteWrite (X94_INT1_ST, INT_94h);
        I2C_ByteWrite (X95_INT2_ST, INT_95h);

        // Re-enable HDMI interrupt
        // Last interrupt saved, so the next interrupt will not override this process
        EX0 = 1;//IE |= 0x01;

    } else {
        /* prevent from loss interrupt */
        GM_INT_94h = I2C_ByteRead (X94_INT1_ST);
        if (GM_INT_94h & 0xF)
            I2C_ByteWrite (X94_INT1_ST, GM_INT_94h & 0xF);
    }

    // -- 2009.06.16

    // LED shows current state
//    P2 = ((HDMI_STATE & 0x0F) | (P2 & 0xF0));

    // Take log of state change history
    if (history[hist_i] != HDMI_STATE)
    {
        HDMI_STATE_CHK=1;

        // re-start WD Timer for HDMI Status monitor
        WDmonTM_Enable();

        hist_i++;
        if (hist_i == 15) hist_i = 0;
        history[hist_i] = HDMI_STATE;

#ifdef MonM
        USB_MCU_MON=1;
        if (USB_MCU_MON == 1){
            USB_Packet[2]= HDMI_STATE;
            Block_Write(USB_Packet, 8);
        }
#endif
    }

    // 2009.06.16
    // WD Timer check  for HDMI Status monitor
    // when HDMI_STATE is not  "HDMI_STATE_TX_RUNNING" or "HDMI_STATE_IDLE",
    // go to "HDMI_STATE_IDLE"
    // Time2 is used for a control timer.
    //         Overflow Count(WD_TIMER_CNT)=1 ...about  265mS
    //         Overflow Count(WD_TIMER_CNT)=2 ...about  530mS
    //         Overflow Count(WD_TIMER_CNT)=4 ...about 1090mS
    if( (WD_TIMER_CNT > 8 && !HDCP_ON) || (WD_TIMER_CNT > 14 && HDCP_ON))   // When 2s or 3.5s Fix Status
    {
        // stop WD Timer      for HDMI Status monitor
        WDmonTM_Disable();

        // Check state of HDMI
        if( ( HDMI_STATE != HDMI_STATE_TX_RUNNING && HDMI_STATE != HDMI_STATE_IDLE ) && HDMI_STATE_CHK == 0 )
        {
            // debug Led5
            Led5 = ~Led5;
            HDMI_clear_to_IDLE();

            HDCP_ON = HDCP_ENABLED;
            if (HDCP_ON)
                HDMI_HDCPclear_to_IDLE();

            HDMI_STATE = HDMI_STATE_IDLE;
            HDMI_STATE_CHK=2;
        }
        // re-start WD Timer   for HDMI Status monitor
        WDmonTM_Enable();

    }

    if (PowerMode != PowerMode_E) {
        if (HDMI_STATE > HDMI_STATE_HOTPLUG)
            HDMI_DEBUG_PRINT;
        //HDMI_PRINT_MODE(PowerMode);
    }

    switch (HDMI_STATE)
    {
        case HDMI_STATE_IDLE:
            if (HDMI_STATE_CHK)
            {
                HDMI_STATE_CHK=0;
                // Any interrupt, move to hotplug state
                HDMI_STATE = HDMI_STATE_HOTPLUG;
            }
            // -- 2009.06.16

            else if (PowerMode != PowerMode_A)
            {
                HDMI_PRINT_MODE(PowerMode);
                printk("%s, goback to PowerMode_A \n", __func__);
                // PS mode b -> a
                PowerMode = PowerMode_A;
                I2C_ByteWrite (X00_SYSTEM_CONTROL, PowerMode);
            }

            // Show black screen
            if (HDCP_BLACK)
                BLACK_SCREEN_ENABLED;
            else
                BLACK_SCREEN_DISABLED;
            break;

        case HDMI_STATE_HOTPLUG:
            // Reset EDID current block
            EDID_current = 0;

            if (HDMI_hotplug_check())
            {
                // Initialize global var settings
                HDMI_setting.vid           = 0;//VID_06_720x480i;
                HDMI_setting.audio_freq    = AUD_48K;
                HDMI_setting.audio_ch      = AUD_2CH;
                HDMI_setting.deep_color    = DEEP_COLOR_8BIT;
                HDMI_setting.down_sampling = DS_none;
                HDMI_setting.audio_source  = AUD_SPDIF; // 0 = I2S, 1 = SPDIF
#ifndef KENNY_CHANGE
                HDMI_setting.output_format = FORMAT_YCC422;
#endif
                HDMI_setting.dvi_mode      = 0;
                EDID_retry = EDID_RETRY_COUNTER;
                EDID_current = 0;
                EDID_process_done = 0;
                RE_SETTING = 0;

                // Move to next state
                HDMI_STATE = HDMI_STATE_EDID_START;

                // PS mode a -> b
                PowerMode = PowerMode_B;
                I2C_ByteWrite (X00_SYSTEM_CONTROL, PowerMode);
            }
            else
            {
                // Hotplug - unplugged
                HDMI_STATE = HDMI_STATE_IDLE;
            }
            break;

        case HDMI_STATE_EDID_START:
            // Enable EDID interrupt (disable in next state)
            I2C_ByteWrite (X92_INT_MASK1, 0xC6); //enable EDID_RDY, EDID_ERR interrupt
            if (I2C_ByteRead(X92_INT_MASK1) != 0xC6)
                panic("%s, why 0x%x \n", __func__, I2C_ByteRead(X92_INT_MASK1));

            // Set EDID word address (set to 00h for the first 128 bytes)
            I2C_ByteWrite (XC5_EDID_WD_ADDR, EDID_WORD);

            // Set EDID segment pointer 0
            // (Regsiter write to XC4_SEG_PTR will start EDID reading)
            I2C_ByteWrite (XC4_SEG_PTR, EDID_SEG);

            HDMI_STATE = HDMI_STATE_EDID_READY;
            break;

        case HDMI_STATE_EDID_READY:
            printk("%s, waiting EDID_RDY interrupt(0x92:0x%x, 0x94:0x%x, 0x%x) ...... \n", __func__,
                                                I2C_ByteRead(X92_INT_MASK1), INT_94h, GM_INT_94h);
            INT_94h |= (GM_INT_94h & 0xF);
            // EDID ERR interrupt, or EDID not ready
            if ((INT_94h & EDID_ERR))
            {
                EDID_retry--;

                // Retry EDID read...
                HDMI_STATE = HDMI_STATE_EDID_START;
                printk("%s, HDMI_STATE_EDID_READY: INT_94h EDID_ERR\n", __func__);

            }
            else if (INT_94h & EDID_RDY)
            {
                // Disable EDID interrupt
                I2C_ByteWrite (X92_INT_MASK1, 0xC0); //disable EDID_RDY, EDID_ERR interrupt

                HDMI_STATE = HDMI_STATE_EDID_READ;
                printk("%s, ok, get EDID_RDY INT_94h INTERRUPT ------- \n", __func__);
            }
            else/*no edid rdy and error case*/
            {
                EDID_retry--;
                // Retry EDID read...
                HDMI_STATE = HDMI_STATE_EDID_START;
                printk("%s, HDMI_STATE_EDID_READY: EDID no response\n", __func__);
            }

            // No more EDID retry. Check the line status
            if (EDID_retry == 0)
            {
                // Disable EDID interrupt
            #if 1/*bypass edid read m, set to default*/
                I2C_ByteWrite (X92_INT_MASK1, 0xC0);

                HDMI_STATE = HDMI_STATE_TX_SETTING;

            #else
                I2C_ByteWrite (X92_INT_MASK1, 0xC0);

                HDMI_STATE = HDMI_STATE_IDLE;

                printk("%s, go back to HDMI_STATE_IDLE due to EDID_retry fail!!!! \n", __func__);
            #endif

            }
            break;

        case HDMI_STATE_EDID_READ:
            // FIXME: Read EDID for 1 block (128bytes)
            I2C_ReadArray (EDID_Data + (EDID_current * 0x80), X80_EDID,128);
            //I2C_ReadArray (EDID_Data, X80_EDID, 128);

            // Calculate EDID data checksum
            checksum = HDMI_EDID_checksum(EDID_Data + (EDID_current * 0x80), 128);
            //checksum = HDMI_EDID_checksum(EDID_Data, 128);

            if (checksum != 0)
            {
                // Retry EDID read...
                EDID_retry--;
                HDMI_STATE = HDMI_STATE_EDID_START;
            }
            else
            {
                if(EDID_current == 0 && edid_check_block0_header() < 0){
                    printk("%s, EDID block0 error header\n", __func__);
                    EDID_retry--;
                    HDMI_STATE = HDMI_STATE_EDID_START;
                }
                else
                    HDMI_STATE = HDMI_STATE_EDID_PROCESS;
				/*DUMP EDID content*/

                if(g_dump_edid){
    				printk("%s,DUMP EDID content \n", __func__);

    				for(j = 0 ; j <= EDID_current ; j++)
    				{
    					for(i = 0 ; i < 128; i++ )
    					{
    						if(i % 16 == 0)
    							printk("\r\n");

    						printk("%02x ",EDID_Data[((j*128)+i)]);
    					}
    				}

    				printk("EDIDDATA[126]=%d\n",EDID_Data[126]);
                }
            }

            if (EDID_retry == 0)
            {
                // Disable EDID interrupt

                I2C_ByteWrite (X92_INT_MASK1, 0xC0);

                HDMI_STATE = HDMI_STATE_TX_SETTING;


            }
            break;

        case HDMI_STATE_EDID_PROCESS:
            // Proesss EDID Data
            // EDID block 0 setting
            if (EDID_current == 0)
            {
#ifdef KENNY_CHANGE /*Original is #ifndef , for ext block open it */

                //Read Extension flag of EDID
                EDID_size = EDID_EXT;

                if (EDID_size == 0) {
                    // connected to DVI device, always RGB
                    HDMI_setting.dvi_mode = 1;
                    HDMI_setting.output_format = FORMAT_RGB;
                }
                else
                {
                    // connected to HDMI device, or DVI device with multiple EDID blocks
                    HDMI_setting.dvi_mode = 1;
                    // Note: HDMI mode will be set (dvi_mode = 0),
                    // when vendor block description found in EDID
                }
#endif
            }
			//printk("%s, ok, CURRENT_EDID =%d ,EDIDEXT=%d \n", __func__ ,EDID_current,EDID_size);

            // EDID block 1 setting
            if ((EDID_current > 0) &&
                (EDID_Data [128*EDID_current + 0] == 0x02) &&
                (EDID_Data [128*EDID_current + 1] == 0x03) &&
                (EDID_process_done == 0))
            {
                short temp_loc = 0, vid_loc = 0, aud_loc = 0, aud_edid = 0, ch_loc = 0, ch_edid = 0, vendor_loc = 0, vendor_edid = 0;

                temp_loc = 128*EDID_current+4;
                //switch (EDID_Data[temp_loc] >> 5) {
                //  case 0x02:  vid_loc    = temp_loc;  break; // 0x02 = Video Short Block Description
                //  case 0x01:  aud_loc    = temp_loc;  break; // 0x01 = Audio Short Block Description
                //  case 0x04:  ch_loc     = temp_loc;  break; // 0x04 = Channel Allocation Data Block
                //  case 0x03:  vendor_loc = temp_loc;  break; // 0x03 = Vendor Allocation Data Block
                //  default: break;
                //}
                while (1)
                {
                    // Decode the Short Block Tag Code
                    if ( (EDID_Data[temp_loc] >> 5) == 0x02)
                    {
                        if (vid_loc != 0) break;
                        vid_loc    = temp_loc;  // Tag 2 = Video Short Block Description
                    }
                    else if ( (EDID_Data[temp_loc] >> 5) == 0x01)
                    {
                        if (aud_loc != 0) break;
                        aud_loc    = temp_loc;  // Tag 1 = Audio Short Block Description
                    }
                    else if ( (EDID_Data[temp_loc] >> 5) == 0x04)
                    {
                        if (ch_loc != 0) break;
                        ch_loc     = temp_loc;  // Tag 4 = Channel Allocation Data Block
                    }
                    else if ( (EDID_Data[temp_loc] >> 5) == 0x03)
                    {
                        if (vendor_loc != 0) break;
                        vendor_loc = temp_loc;  // Tag 3 = Vendor Allocation Data Block
                        if ((EDID_Data[vendor_loc + 1] == 0x03) &&
                            (EDID_Data[vendor_loc + 2] == 0x0C) &&
                            (EDID_Data[vendor_loc + 3] == 0x00))
                            HDMI_setting.dvi_mode = 0;  // set to HDMI mode
                    }

                    if (((vid_loc != 0) && (aud_loc != 0) && (ch_loc != 0) && (vendor_loc != 0)) ||
                        (temp_loc > 128*(EDID_current+1)) ||
                        (EDID_Data[temp_loc] == 0))
                        break;

                    temp_loc = (EDID_Data[temp_loc] & 0x1F) + temp_loc + 1;
                }
                // Original Short Block Tag decoding
                //aud_loc = 128*EDID_current + (EDID_Data[128*EDID_current+4] & 0x1F) + 5;
                //ch_loc  = (EDID_Data[aud_loc] & 0x1F) + aud_loc + 1;
                //vendor_loc = (EDID_Data[ch_loc] & 0x1F) + ch_loc + 1;

                if (vid_loc != 0)
                {
                    HDMI_setting.vid = EDID_Data[vid_loc + 1] & 0x7F; // preferred video format
                    //HDMI_setting.vid = EDID_Data[128*EDID_current + 5] & 0x7F; // preferred video format
                }

                if (aud_loc != 0)
                {
                    aud_edid = EDID_Data[aud_loc + 2];
                    if (aud_edid & 0x40) HDMI_setting.audio_freq = AUD_192K;
                    else if (aud_edid & 0x10) HDMI_setting.audio_freq = AUD_96K;
                    else HDMI_setting.audio_freq = AUD_48K;
                }

                if (ch_loc != 0)
                {
                    ch_edid = EDID_Data[ch_loc + 1];
                    if (ch_edid & 0x40) HDMI_setting.audio_ch = AUD_8CH;
                    else HDMI_setting.audio_ch = AUD_2CH;
                }

                if (vendor_loc != 0)
                {
                    vendor_edid = EDID_Data[vendor_loc];
                    vendor_edid = vendor_edid & 0x1F;
                    if (vendor_edid > 5)
                    {
                        // ACP, ISRC packet Set
                        CtrlPkt_Enable |= 0x06;
                    }
                    else CtrlPkt_Enable &= 0xFE;
                }

#ifdef KENNY_CHANGE /*open it to support 7-24 HDMI test item*/
                // color
                // Output Fomrat RGB or YCC422 or YCC444    2009.05.23    FORMAT_YCC444 > FORMAT_YCC422 > FORMAT_RGB
                /*According to sli SPEC. Figure 42 , YCC444 > YCC422 > rgb*/
                if(Sli10121_Get_Output_Mode() == FROCE_OUT_MODE_AUTO){
                    if (EDID_Data[128*EDID_current+3] & 0x20){
                        HDMI_setting.output_format = FORMAT_YCC444;
                    }
                    else if (EDID_Data[128*EDID_current+3] & 0x10){
                        HDMI_setting.output_format = FORMAT_YCC422;

                    }
                    else{
                        HDMI_setting.output_format = FORMAT_RGB;

                    }
                }
                else{
                    if(Sli10121_Get_Output_Mode() == FROCE_OUT_MODE_RGB)
                        HDMI_setting.output_format = FORMAT_RGB;
                    else if(Sli10121_Get_Output_Mode() == FROCE_OUT_MODE_YUV422)
                        HDMI_setting.output_format = FORMAT_YCC422;
                    else if(Sli10121_Get_Output_Mode() == FROCE_OUT_MODE_YUV444)
                        HDMI_setting.output_format = FORMAT_YCC444;
                    else{
                        printk("Force output mode to RGB\n");
                        HDMI_setting.output_format = FORMAT_RGB;
                    }

                }
#endif

                EDID_process_done = 1;
            }


            // Read More EDID (first block plus up to 7 extension blocks)
            if ((EDID_current < EDID_size) && (EDID_current < 7))
            {
                // Move to next EDID block
                EDID_current++;

                // EDID read...
                HDMI_STATE = HDMI_STATE_EDID_START;
            }
            // Last EDID block read
            else
            {
            	if(edid_parse_established_timer(((EDID_current > 0)?1:0) , EDID_current) != EDID_SUCCESS)
					printk("%s Invalide EDID Format\n",__func__);

                // init EDID_current for the next hotplug
                EDID_current = 0;

                HDMI_setting.vid = 0; // ignore what the EDID says

                HDMI_STATE = HDMI_STATE_TX_SETTING;
            }

            break;

        case HDMI_STATE_TX_SETTING:
#ifdef SLI10121
            // Disable Video/Audio
            if( HDCP_ON )
            {
                temp = I2C_ByteRead (X45_VIDEO2);
                I2C_ByteWrite (X45_VIDEO2, temp | 0x03);  // [1]NoAudio [0]NoVideo Mute
            }
#endif

            // CHIP: PS mode b -> d ### ADD TO SPEC ###
            PowerMode = PowerMode_D;
            HDMI_System_PD (PowerMode);

            // RE-SETTING by USB command
            if (RE_SETTING) {
                printk("%s, HDMI_STATE_TX_SETTING: RE_SETTING\n", __func__);
                RE_SETTING = 0;
            }
            else
            {
                if (COMP_MODE) // FPGA INPUT MODE
                {
                    // TEMPORARY SETTING
                    //HDMI_setting.vid = 0x02;//0x04; // VID 04 720p // VID_02_720x480p
                    //HDMI_setting.audio_freq    = AUD_48K;
                    //HDMI_setting.audio_ch      = AUD_2CH;
                    //HDMI_setting.deep_color    = DEEP_COLOR_8BIT;
                    //HDMI_setting.down_sampling = DS_none;
                    //HDMI_setting.audio_source  = AUD_SPDIF;     // SPDIF
                    //HDMI_setting.output_format = FORMAT_YCC422;
                    //HDMI_setting.dvi_mode      = 0;

                    // Compliance Mode: DIP switches override some EDID settings
                    // Get Input Video/Audio Information from FPGA(Video/Audio Generator)
                    Monitor_CompSwitch();
                }
                else
                {
                    // TEMPORARY SETTING
                    HDMI_setting.vid = VID_16_1920x1080p;
                    HDMI_setting.audio_freq    = AUD_48K;
                    HDMI_setting.audio_ch      = AUD_2CH;
                    HDMI_setting.deep_color    = DEEP_COLOR_8BIT;
                    HDMI_setting.down_sampling = DS_none;
                    HDMI_setting.audio_source  = AUD_SPDIF;     // SPDIF
                    //HDMI_setting.output_format = FORMAT_YCC422;
                    //HDMI_setting.dvi_mode      = 0;

                    // Wait until user set VID. When VID is set through USB,
                    // TI setting has been applied at the same time.

                    if (HDMI_setting.vid == 0)
                        break;
                }

#ifdef SLI11131
                // IP Conversio
                if(IP_ON)
                    hdmi_tx_ip_conversion_enable (&HDMI_setting);
                else
                    hdmi_tx_ip_conversion_disable (&HDMI_setting);
#endif //SLI11131

                // Apply settings in HDMI_setting struct
                HDMI_apply_setting ();

                // Enable HPG/MSENS/VSYNC interrupt
                I2C_ByteWrite (X92_INT_MASK1, 0xE0);

                // Move to TX Start state
                HDMI_STATE = HDMI_STATE_TX_START;
            }
            break;

        case HDMI_STATE_TX_START:
            if (COMP_MODE)
            {
                // CHIP: PS mode d -> e // NOT NEEDED FOR FPGA INPUT (TI MODE)
                PowerMode = PowerMode_E;
                HDMI_System_PD (PowerMode);

                printk("%s, change to PowerMode_E, the connection is going to ready. \n", __func__);
#ifndef T45TX
                // Change in Version 0.12 from 0.11_step
                if (PowerMode == PowerMode_E && HDCP_ON)
                {
                    // Note: Don't combine Phy loading and HDCP loading.
                    //       Do load Phy first, then wait at least 1 cycle before loading HDCP
                    I2C_ByteWrite(XB0_HDCP_STATUS, I2C_ByteRead(XB0_HDCP_STATUS) | 0x40);     // HDCP Loading is 0x40??
                    //I2C_ByteWrite (XCF_HDCP_MEM_CTRL2, I2C_ByteRead(XCF_HDCP_MEM_CTRL2)| 0x40);   // FIXME: Undocumented in V1.00d spec
                }
#endif

                // 2009.06.07 means of audio error with HDCP
                // Audio is mute after reset of audio is set.
                // Therefore, set it in the following procedures.
                //   Audio:  Save value of now => Audio Reset => Audio Active => Set value again
#ifdef SLI10121
                temp = I2C_ByteRead (X45_VIDEO2);
                I2C_ByteWrite (X45_VIDEO2, temp | 0x06 );   // Reset
                mdelay(1);
                I2C_ByteWrite (X45_VIDEO2, temp & 0xC0 );   // Reset Release and Audio Mute)
                I2C_ByteWrite (X45_VIDEO2, temp );          //
#endif /* SLI10121 */
            }
            else // TI MODE
            {
                if (PowerMode != PowerMode_E)
                    break;
            }
            HDMI_STATE = HDMI_STATE_TX_RUNNING;

            break;

        case HDMI_STATE_TX_RUNNING:
            // Monitoring PowerMode change by USB command
            if (PowerMode != PowerMode_E)
            {
                HDMI_setting.vid = 0;
                HDMI_STATE = HDMI_STATE_TX_SETTING;
                break;
            }

            // RE-SETTING by USB command
            if (RE_SETTING)
            {
                printk("%s, HDMI_STATE_TX_RUNNING: RE_SETTING  \n", __func__);
                HDMI_STATE = HDMI_STATE_TX_SETTING;
                break;
            }

            if (INT_94h & VSYNC)
            {
#ifdef SLI10121
                // Send Control Packet
                HDMI_control_packet_auto_Send (CtrlPkt_Auto, CtrlPkt_Enable);
#else
                HDMI_control_packet_manual_Send (&CtrlPkt_Manual, CtrlPkt_Enable);
#endif
                // Just to check receiving VSYNC
                if (VSYNC_counter == 30)
                {
                    Led7 = ~Led7;
                    VSYNC_counter = 0;

                    // LED6 blink indicates HDCP authenticated
                    temp = I2C_ByteRead (XB8_HDCP_STATUS);
                    if ((temp & 0x80) == 0x80)
                    {
                        Led6 = ~Led7;

                        if (HDCP_BLACK && COMP_MODE) // when HDCP is authenticated
                            BLACK_SCREEN_DISABLED;               // take off black screen
                    }
                }
                else
                    VSYNC_counter ++;
            }

            // HDCP interrupt
            if (INT_95h & HDCP_ERR)
            {
                // Retry HDCP start...
                HDMI_clear_to_IDLE();             // 2009.06.16
                HDMI_HDCPclear_to_IDLE();         // 2009.06.16
                HDMI_STATE = HDMI_STATE_IDLE;

                break;
            }

            if (HDCP_ON)
            {
                if( (I2C_ByteRead (XB8_HDCP_STATUS) & AUTH)== 0 )
                {
                    HDMI_HDCPclear_to_IDLE();       // 2009.07.30
                    HDMI_STATE = HDMI_STATE_HDCP_START;
                }
            }
            break;

        case HDMI_STATE_HDCP_START:
            HDMI_HDCP_enable();

            // Enable interrupt, KSV setting, enable HDCP bit
            HDMI_STATE = HDMI_STATE_HDCP_READY;
            break;

        case HDMI_STATE_HDCP_READY:
            if (PowerMode != PowerMode_E)
            {
                HDMI_clear_to_IDLE();            // 2009.06.16
                HDMI_HDCPclear_to_IDLE();        // 2009.06.16
                HDMI_STATE = HDMI_STATE_IDLE;
                break;
            }

            // RE-SETTING by USB command
            if(RE_SETTING)
            {
                HDMI_STATE = HDMI_STATE_TX_SETTING;
                break;
            }

            // HDCP interrupt
            if (INT_95h & HDCP_ERR)
            {
                HDMI_clear_to_IDLE();            // 2009.06.16
                HDMI_HDCPclear_to_IDLE();        // 2009.06.16
                HDMI_STATE = HDMI_STATE_IDLE;   // FIXME: which state should be entered
            }
            // BKSV list is ready to read
            else if (INT_95h & BKSV_RDY)
            {
                HDMI_STATE = HDMI_STATE_HDCP_READ;
            }

            if (INT_94h & VSYNC)
            {
#ifdef SLI10121
                // Send Control Packet
                HDMI_control_packet_auto_Send (CtrlPkt_Auto,CtrlPkt_Enable);
#else
                // Send Control Packet
                HDMI_control_packet_manual_Send (&CtrlPkt_Manual,CtrlPkt_Enable);
#endif
                // Just to check receiving VSYNC
                if (VSYNC_counter == 30)
                {
                    Led7 = ~Led7;
                    VSYNC_counter = 0;

                    // LED6 blink indicates HDCP authenticated
                    temp = I2C_ByteRead (XB8_HDCP_STATUS);
                    if ((temp & 0x80) == 0x80)
                    {
                        Led6 = ~Led7;
                    }
                }
                else
                    VSYNC_counter ++;
            }


            break;

        case HDMI_STATE_HDCP_READ:
            // Read BKSV for 5 bytes
            BKSV_Data[0] = I2C_ByteRead (XBF_KSV7_0);
            BKSV_Data[1] = I2C_ByteRead (XC0_KSV15_8);
            BKSV_Data[2] = I2C_ByteRead (XC1_KSV23_16);
            BKSV_Data[3] = I2C_ByteRead (XC2_KSV31_24);
            BKSV_Data[4] = I2C_ByteRead (XC3_KSV39_32);

            //====================================================
            //
            // CHECK BKSV WITH REVOCATION LIST HERE
            //
            //====================================================
            //temp = I2C_ByteRead (XAF_HDCP_CTRL);
            //if (temp & BAD_BKSV) // for now, always pass
            if (0) // for now, always pass
            {
                // Set BKSV flag - fail
                temp = I2C_ByteRead (XAF_HDCP_CTRL);
                I2C_ByteWrite (XAF_HDCP_CTRL, temp | 0x20); // BKSV_Fail is bit 5

                // Move to Other State - Continue TX
            }
            else
            {
                // Set BKSV flag - pass
                temp = I2C_ByteRead (XAF_HDCP_CTRL);
                I2C_ByteWrite (XAF_HDCP_CTRL, temp | 0x40); // BKSV_Pass is bit 6
#ifdef T45TX
                I2C_ByteWrite (XD0_HDCP_CTRL2, 0x20); // SKIP KSV READ - FIXME!! Enable PJ Check
                //I2C_ByteWrite (XD0_HDCP_CTRL2, I2C_ByteRead (XD0_HDCP_CTRL2) | 0x68); // SKIP KSV READ - FIXME!! Enable PJ Check
#endif

                HDMI_STATE = HDMI_STATE_HDCP_AUTH;
            }
            break;

        case HDMI_STATE_HDCP_AUTH:

            // Wait for HDCP interrupt BKSV FIFO ready (repeater mode interrupt)
            if (INT_95h & BKSV_RDY)
            {
                // Block Read 0x80 for BKSV List
                //====================================================
                //
                // CHECK BKSV LIST WITH REVOCATION LIST HERE
                //
                //====================================================

                // AF bit 6 to pass
                //temp = I2C_ByteRead (XAF_HDCP_CTRL);
                //if (temp & BAD_BKSV) // for now, always pass
                if (0) // for now, always pass
                {
                    // Set BKSV flag - fail
                    temp = I2C_ByteRead (XAF_HDCP_CTRL);
                    I2C_ByteWrite (XAF_HDCP_CTRL, temp | BKSV_FAIL); // BKSV_Fail is bit 5

                    // Move to Other State - Continue TX
                }
                else
                {
                    // Set BKSV flag - pass
                    temp = I2C_ByteRead (XAF_HDCP_CTRL);
                    I2C_ByteWrite (XAF_HDCP_CTRL, temp | BKSV_PASS); // BKSV_Pass is bit 6
                    HDMI_STATE = HDMI_STATE_HDCP_AUTH;
                }
            }
            // HDCP interrupt
            if (INT_95h & HDCP_DONE)
            {
                //HDCP_ON = 1; // HDCP_ON = 0 here if no retry
                // Disable HDCP interrupt
                I2C_ByteWrite (X93_INT_MASK2, 0x00);
#ifndef SLI10121
                I2C_ByteWrite (X45_VIDEO2, temp & 0xFC);  // Video/Audio Active ([1]NoAudio [0]NoVideo No Mute)
#endif
                HDMI_STATE = HDMI_STATE_TX_RUNNING;
            }
            if (INT_95h & HDCP_ERR)
            {
                // Retry HDCP
                HDMI_HDCPclear_to_IDLE();        // 2009.06.16
                HDMI_STATE = HDMI_STATE_HDCP_START;
            }
            // Check hardware state. Just in case 5-sec interrupt is missed.
            temp = I2C_ByteRead (0x0F); // special register to check
            if (temp == 0)
            {
                // Retry HDCP
                HDMI_HDCPclear_to_IDLE();        // 2009.06.16
                HDMI_STATE = HDMI_STATE_HDCP_START;
            }

            break;

        case HDMI_STATE_ERROR:
            HDMI_STATE = HDMI_STATE_IDLE;
            break;


        case STATE_DEBUG:

            break;

        default:
            break;
    }

    in_bottom_isr = 0;
}

void HDMI_clear_to_IDLE (void)
{
    // Intterupt of Mask ... gNot maskedh is only "hpg_msk" and "msens_msk".
    I2C_ByteWrite (X92_INT_MASK1, 0xC0);

#ifdef SLI10121
    I2C_ByteWrite (X45_VIDEO2, I2C_ByteRead( X45_VIDEO2 ) & 0xFB );   // Reset to Release
    // HDCP_ON Disable Video/Audio
    if( HDCP_ON )
        I2C_ByteWrite (X45_VIDEO2, I2C_ByteRead( X45_VIDEO2 ) | 0x03 );   // No Audio/Video
    else
        I2C_ByteWrite (X45_VIDEO2, I2C_ByteRead( X45_VIDEO2 ) & 0xFC );   // No Audio/Video
#endif
}
void HDMI_HDCPclear_to_IDLE (void)
{

#ifdef SLI10121
    // HDCP Error Clear
    I2C_ByteWrite (XC8_HDCP_ERR, 0x00);
    I2C_ByteWrite (X45_VIDEO2, I2C_ByteRead( X45_VIDEO2 ) | 0x03 );   // No Audio/Video
#endif
    //  HDCP   ...  gmaskedhof Intterupt
    I2C_ByteWrite (X93_INT_MASK2, 0x00);
    //HDCP Stop & Reset // hdcp_mode
    I2C_ByteWrite (XAF_HDCP_CTRL,  0x1B);
    // defult + HDCP Stop
    I2C_ByteWrite (XAF_HDCP_CTRL,  0x92);

#ifdef T45LP
    I2C_ByteWrite (XD0_HDCP_CTRL2, 0x08); // defult
#endif
}

BYTE HDMI_hotplug_check2 (void)
{
    BYTE STAT_DFh;

    STAT_DFh = I2C_ByteRead (XDF_HPG_STATUS);
	/*LO active so HOP_PLUG bit is lo to active*/
    return ((STAT_DFh == HPG_MSENS || (STAT_DFh & HOT_PLUG) == HOT_PLUG));
}

BYTE HDMI_hotplug_check (void)
{
    BYTE STAT_DFh;
    static int count = 0;

    // Wait time before check hot plug & MSENS pin status
    DelayMs (15);

    STAT_DFh = I2C_ByteRead (XDF_HPG_STATUS);

#if 1
    /* Harry, it should be check HPG_MSENS
     */
    if ((STAT_DFh & HPG_MSENS) == HPG_MSENS) {
        printk("%s, -------- dectect HPG_MSENS --------  \n", __func__);
        count = 0;
        return 1;
    }
#endif

    if ((STAT_DFh & HOT_PLUG) == HOT_PLUG)        // detect HOT_PLUG only
    {
        if ((count % 200) == 0)
            printk("%s, ---- dectect HOT_PLUG only!!! Monitor is VGA preferred? ---- \n", __func__);

#ifdef SLI10121
        // DDC I2C master controller reset ... ddc_ctrl_reset[bit4]
        I2C_ByteWrite (X3B_AVSET2, I2C_ByteRead (X3B_AVSET2) | 0x10);
        mdelay(1);
        I2C_ByteWrite (X3B_AVSET2, I2C_ByteRead (X3B_AVSET2) & 0xEF);
#endif
        count ++;
        return 0;
    }

    /* fail */
    return 0;
}


//-----------------------------------------------------------------------------
// HDMI_control_packet_auto_Send
//-----------------------------------------------------------------------------
//
// Return Value :
//
// Parameters   : BYTE CtrlPktAuto
//                BYTE CtrlPkt
//
//
void HDMI_control_packet_auto_Send (BYTE CtrlPktAuto, BYTE CtrlPkt)
{
    // Enable Send Packets on each VSYNC
    if (CtrlPktAuto)
        I2C_ByteWrite(X41_SEND_CPKT_AUTO, CtrlPkt);
    else
        I2C_ByteWrite(X41_SEND_CPKT_AUTO, 0x00);
}

#ifndef SLI10121
//-----------------------------------------------------------------------------
// hdmi_tx_control_packet_set
//-----------------------------------------------------------------------------
//
// Return Value :
//
// Parameters   : BYTE *CtrlPktSend
//                BYTE CtrlPkt
//
//
void HDMI_control_packet_manual_Send (BYTE *CtrlPktSend, BYTE CtrlPkt)
{
    // Send Control Packet
    if (*CtrlPktSend)
        I2C_ByteWrite (X40_CTRL_PKT_EN, I2C_ByteRead (X40_CTRL_PKT_EN) | CtrlPkt);
    *CtrlPktSend = 0;
    // 10/31/07: Moved outside of "if (CtrlPkt_Enable)".
}
#endif

//-----------------------------------------------------------------------------
// HDMI_System_PD ()
//-----------------------------------------------------------------------------
//
// Parameters   : unsigned char mode - SLI10130 power mode. 4 modes available.
//                  MODE_A (sleep), MODE_B (register access), MODE_D (clock), MODE_E (active).
//
//
void HDMI_System_PD (unsigned char mode)
{
    if ((PowerMode == PowerMode_B) && (mode == PowerMode_A)) {
        I2C_ByteWrite (X00_SYSTEM_CONTROL, 0x1C);
    } else {
        I2C_ByteWrite (X00_SYSTEM_CONTROL, mode | 0xC);
        DelayMs (1);    // wait 1ms
        I2C_ByteWrite (X00_SYSTEM_CONTROL, mode | 0x8);
        DelayMs (1);    // wait 1ms
        I2C_ByteWrite (X00_SYSTEM_CONTROL, mode);
        DelayMs (1);    // wait 1ms
    }

    PowerMode = mode;
    rdy_cnt = 0;
}

//-----------------------------------------------------------------------------
// HDMI_HDCP_enable / disable ()
//-----------------------------------------------------------------------------
//
// Enable / disable HDCP and change HDCP interrupt mask. HDMI_setting should be
// configured before this call.
//
//
void HDMI_HDCP_enable (void)
{
    BYTE temp, vid, t_100ms;
    short t_5sec;
    vid = HDMI_setting.vid;
    t_100ms = 0;
    t_5sec = 0;

    // Set to wait at least 100ms before R0 read after aksv write
    // interlace - 60Hz => 13, 50Hz => 11, 24Hz => 5
    // progressive - 60Hz => 7, 50Hz => 6, 24Hz => 3
    switch (HDMI_setting.vid)
    {
        // 60Hz progressive, 30Hz interlace =>
        case 1:     case 2:     case 3:     case 4:     case 8:
        case 9:     case 12:    case 13:    case 14:    case 15:
        case 16:    case 35:    case 36:
            t_100ms = 7;
            t_5sec = 301;
            break;

        // 60Hz interlace, 120Hz progressive
        case 5:     case 6:     case 7:     case 10:    case 11:
        case 47:    case 48:    case 49:
            t_100ms = 13;
            t_5sec = 601;
            break;

        // 50Hz progressive
        case 17:    case 18:    case 19:    case 23:    case 24:
        case 27:    case 28:    case 29:    case 30:    case 31:
        case 37:    case 38:
            t_100ms = 6;
            t_5sec = 251;
            break;

        // 50Hz interlace, 100Hz progressive
        case 20:    case 21:    case 22:    case 25:    case 26:
        case 39:    case 41:    case 42:    case 43:
            t_100ms = 11;
            t_5sec = 501;
            break;

        // 100Hz interlace, 200Hz progressive
        case 40:    case 44:    case 45:    case 52:    case 53:
            t_100ms = 21;
            t_5sec = 1001;
            break;

        // 24Hz/25Hz progressive
        case 32:
        case 33:
            t_100ms = 3;
            t_5sec = 126;
            break;

        // 30Hz progressive
        case 34:
            t_100ms = 4;
            t_5sec = 151;
            break;

        // 120Hz interlace, 240Hz progressive
        case 46:    case 50:    case 51:    case 56:    case 57:
            t_100ms = 25;
            t_5sec = 1201;
            break;

        // 200Hz interlace
        case 54:    case 55:
            t_100ms = 41;
            t_5sec = 2001;
            break;

        // 240Hz interlace
        case 58:    case 59:
            t_100ms = 49;
            t_5sec = 2451;
            break;

        default:
            t_100ms = 50;
            t_5sec = 2500;
            break;
    }


    // Set 100ms (lower 5 bits at 0xCA) and 5 sec timer (upper 3 bits at 0xCA and 8 bits in 0xC9)
    /*
    // === CHIP 2 ===
    if (t_100ms > 31)
        t_100ms = 31; // max 5 bits
    if (t_5sec > 2047)
        t_5sec = 2047; // max 11 bits
    temp = ((t_5sec >> 3) & 0xE0) | t_100ms;
    I2C_ByteWrite (0xC9, temp);
    // 5 sec timer
    temp = t_5sec & 0xFF;
    I2C_ByteWrite (0xCA, temp);
    */
#ifdef KENNY_CHANGE
    // TEMP FIX : DELAY 20% for 5sec timer
    t_5sec = (t_5sec * 120) / 100;
#else
    // TEMP FIX : DELAY 20% for 5sec timer
    t_5sec = t_5sec * 1.2;
#endif

    // === CHIP 1 (SN) ===
    if (t_100ms > 15)
        t_100ms = 15; // max 4 bits
    if (t_5sec > 1023)
        t_5sec = 1023; // max 10 bits
    temp = ((t_5sec >> 2) & 0xC0) | t_100ms;
    I2C_ByteWrite (XC9_TIMER, temp);
    // 5 sec timer
    temp = t_5sec & 0xFF;
    I2C_ByteWrite (XCA_TIMER, temp);


    // Enable HDCP interrupt
    I2C_ByteWrite (X93_INT_MASK2, 0xF8);
    temp = I2C_ByteRead (XAF_HDCP_CTRL);
    I2C_ByteWrite (XAF_HDCP_CTRL, temp | 0x80); // HDCP ON (0xAF bit7)
}


void HDMI_HDCP_disable (void)
{
    BYTE temp;

    temp = I2C_ByteRead (XAF_HDCP_CTRL);
    I2C_ByteWrite (XAF_HDCP_CTRL, temp | 0x08); // HDCP OFF (0xAF bit3)
}


//-----------------------------------------------------------------------------
// HDMI_EDID_checksum ()
//-----------------------------------------------------------------------------
//
// Return Value : checksum
//
// Parameters   : BYTE* array - EDID data
//              : BYTE size - size of array
//
BYTE HDMI_EDID_checksum (BYTE* array, BYTE size)
{
    BYTE i, sum = 0;
    for (i = 0; i < size; i++)
        sum += array[i];

    return sum;
}

#ifdef SLI11131

//-----------------------------------------------------------------------------
// IP_Conversion_Enable
//-----------------------------------------------------------------------------
//
// Return Value :
//
// Parameters   : HDMI_Setting_t *s
//
//
//
void hdmi_tx_ip_conversion_enable (HDMI_Setting_t *s)
{

    switch (s->vid & 0x7F)
    {
        case VID_06_720x480i:   // 480i  59.94/60Hz
            s->vid =VID_02_720x480p;
            IP_Conversion_setting_480(s->vid);
            break;
        case VID_07_720x480i:   // 480i  59.94/60Hz
            s->vid =VID_03_720x480p;
            IP_Conversion_setting_480(s->vid);
            break;
        case VID_21_720x576i:   // 576i  50Hz
            s->vid =VID_17_720x576p;
            IP_Conversion_setting_576(s->vid);
            break;
        case VID_22_720x576i:   // 576i  50Hz
            s->vid =VID_18_720x576p;
            IP_Conversion_setting_576(s->vid);
            break;
        case VID_05_1920x1080i: // 1080i 59.94/60Hz
            s->vid =VID_16_1920x1080p;
            IP_Conversion_setting_1080(s->vid);
            break;
        default:
            I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
                    & ~IP_CONV_EN & ~IP_CONV_PIX_REP );     // Disable IP convsersion
            break;
    }
}
#endif      // SLI11131

#ifdef SLI11131
//-----------------------------------------------------------------------------
// IP_Conversion_Disable
//-----------------------------------------------------------------------------
//
// Return Value :
//
// Parameters   : HDMI_Setting_t *s
//
//
//
void hdmi_tx_ip_conversion_disable (HDMI_Setting_t *s)
{

    if(I2C_ByteRead(XFF_IP_COM_CONTROL) & IP_CONV_PIX_REP)
        switch (s->vid & 0x7F)
        {
            case VID_02_720x480p:   // 480i  59.94/60Hz
                s->vid =VID_06_720x480i;
                I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead(XFF_IP_COM_CONTROL)
                        & ~IP_CONV_EN & ~IP_CONV_PIX_REP );     // Disable IP convsersion
                I2C_ByteWrite (X30_EXT_VPARAMS, 0x00);
                break;
            case VID_03_720x480p:   // 480i  59.94/60Hz
                s->vid =VID_07_720x480i;
                I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead(XFF_IP_COM_CONTROL)
                        & ~IP_CONV_EN & ~IP_CONV_PIX_REP );     // Disable IP convsersion
                I2C_ByteWrite (X30_EXT_VPARAMS, 0x00);
                break;
            case VID_17_720x576p:   // 576i  50Hz
                s->vid =VID_21_720x576i;
                I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead(XFF_IP_COM_CONTROL)
                        & ~IP_CONV_EN & ~IP_CONV_PIX_REP );     // Disable IP convsersion
                I2C_ByteWrite (X30_EXT_VPARAMS, 0x00);
                break;
            case VID_18_720x576p:   // 576i  50Hz
                s->vid =VID_22_720x576i;
                I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead(XFF_IP_COM_CONTROL)
                        & ~IP_CONV_EN & ~IP_CONV_PIX_REP );     // Disable IP convsersion
                I2C_ByteWrite (X30_EXT_VPARAMS, 0x00);
                break;
            case VID_16_1920x1080p: // 1080i 59.94/60Hz
                s->vid =VID_05_1920x1080i;
                I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead(XFF_IP_COM_CONTROL)
                         & ~IP_CONV_PIX_REP );                  // Disable IP convsersion
                I2C_ByteWrite (X30_EXT_VPARAMS, 0x00);
                break;
            default:
                I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead(XFF_IP_COM_CONTROL)
                        & ~IP_CONV_EN & ~IP_CONV_PIX_REP );     // Disable IP convsersion
                I2C_ByteWrite (X30_EXT_VPARAMS, 0x00);
                break;
        }
}
#endif      // SLI11131

#ifdef SLI11131
//-----------------------------------------------------------------------------
// IP_Conversion_setting_480
//-----------------------------------------------------------------------------
//
// Return Value :
//
// Parameters   : BYTE vid --
//
// Set IP convsersion on 480i->480p
//
//
static void  IP_Conversion_setting_480 (BYTE vid)
{
    I2C_ByteWrite (X30_EXT_VPARAMS, 0x33);
    I2C_ByteWrite (X31_EXT_HTOTAL, 0x5A);
    I2C_ByteWrite (X32_EXT_HTOTAL, 0x03);
    I2C_ByteWrite (X33_EXT_HBLANK, 0x8A);
    I2C_ByteWrite (X34_EXT_HBLANK, 0x00);
    I2C_ByteWrite (X35_EXT_HDLY, 0x77);
    I2C_ByteWrite (X36_EXT_HDLY, 0x00);
    I2C_ByteWrite (X37_EXT_HS_DUR, 0x3E);
    I2C_ByteWrite (X38_EXT_HS_DUR, 0x00);
    I2C_ByteWrite (X39_EXT_VTOTAL, 0x0D);
    I2C_ByteWrite (X3A_EXT_VTOTAL, 0x02);
    I2C_ByteWrite (X3D_EXT_VBLANK, 0x16);
    I2C_ByteWrite (X3E_EXT_VDLY, 0x15);
    I2C_ByteWrite (X3F_EXT_VS_DUR, 0x03);

    I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
         | IP_REG_OFFSET );                             // Set offset bit to access IP registers

    I2C_ByteWrite (FUNCTION_MODE_SETTINGS, INTER_OFF);  // Disable external frame memory
    I2C_ByteWrite (VIDEO_PARAMETER_SETTINGS_AFTER, 0x60);   // Set video timings after IP conversion
    I2C_ByteWrite (HORIZONTAL_TOTAL_AFTER_LSB, 0x5A);
    I2C_ByteWrite (HORIZONTAL_TOTAL_AFTER_MSB, 0x03);
    I2C_ByteWrite (HORIZONTAL_BLANK_AFTER_LSB, 0x8A);
    I2C_ByteWrite (HORIZONTAL_BLANK_AFTER_MSB, 0x00);
    I2C_ByteWrite (HORIZONTAL_DELAY_AFTER_LSB, 0x7A);
    I2C_ByteWrite (HORIZONTAL_DELAY_AFTER_MSB, 0x00);
    I2C_ByteWrite (HORIZONTAL_DURATION_AFTER_LSB, 0x3E);
    I2C_ByteWrite (HORIZONTAL_DURATION_AFTER_MSB, 0x00);
    I2C_ByteWrite (VERTICAL_TOTAL_AFTER_LSB, 0x0D);
    I2C_ByteWrite (VERTICAL_TOTAL_AFTER_MSB, 0x02);
    I2C_ByteWrite (OUTPUT_VIDEO_FORMAT_VID_AFTER, vid);
    I2C_ByteWrite (VERTICAL_BLANK_AFTER, 0x2D);
    I2C_ByteWrite (VERTICAL_DELAY_AFTER, 0x2A);
    I2C_ByteWrite (VERTICAL_DURATION_AFTER, 0x06);

    I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
         | IP_CONV_EN | IP_CONV_PIX_REP );              // Set IP convsersion on 480i->480p

    I2C_ByteWrite (HORIZONTAL_PIXEL_START_POSITION_TO_LINE_MEMORY_LSB, 0x66);   // henpos = 102d
    I2C_ByteWrite (HORIZONTAL_PIXEL_START_POSITION_TO_LINE_MEMORY_MSB, 0x00);
    I2C_ByteWrite (HORIZONTAL_PIXEL_COUNT_TO_LINE_MEMORY_LSB, 0xD2);            // henwdt = 722d = 2D2h
    I2C_ByteWrite (HORIZONTAL_PIXEL_COUNT_TO_LINE_MEMORY_MSB, 0x02);
    I2C_ByteWrite (VERTICAL_PIXEL_START_POSITION_TO_LINE_MEMORY_LSB, 0x10);     // venpos = 16d
    I2C_ByteWrite (VERTICAL_PIXEL_START_POSITION_TO_LINE_MEMORY_MSB, 0x00);
    I2C_ByteWrite (VERTICAL_PIXEL_COUNT_TO_LINE_MEMORY_LSB, 0xF0);              // venwdt = 240d
    I2C_ByteWrite (VERTICAL_PIXEL_COUNT_TO_LINE_MEMORY_MSB, 0x00);
    I2C_ByteWrite (OUTPUT_HSYNC_POSITION_LSB, 0x7E);                            // hdpos = 126d
    I2C_ByteWrite (OUTPUT_HSYNC_POSITION_MSB, 0x00);
    I2C_ByteWrite (OUTPUT_HSYNC_WIDTH, 0x3E);                                   // hdwdt = 62d
    I2C_ByteWrite (OUTPUT_VSYNC_POSITION_LSB, 0x03);                            // vdpos = 3d
    I2C_ByteWrite (OUTPUT_VSYNC_POSITION_MSB, 0x00);
    I2C_ByteWrite (OUTPUT_VSYNC_WIDTH, 0x06);                                   // vdwdt = 6d
    I2C_ByteWrite (DE_HORIZONTAL_START_POSITION_LSB, 0x7A);
    I2C_ByteWrite (DE_HORIZONTAL_START_POSITION_MSB, 0x00);
    I2C_ByteWrite (DE_HORIZONTAL_END_POSITION_LSB, 0x4A);
    I2C_ByteWrite (DE_HORIZONTAL_END_POSITION_MSB, 0x03);
    I2C_ByteWrite (DE_VERTICAL_START_POSITION_LSB, 0x24);
    I2C_ByteWrite (DE_VERTICAL_START_POSITION_MSB, 0x00);
    I2C_ByteWrite (DE_VERTICAL_END_POSITION_LSB, 0x04);
    I2C_ByteWrite (DE_VERTICAL_END_POSITION_MSB, 0x02);

    I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
         & ~IP_REG_OFFSET);             // Reset offset bit to access normal registers
}
#endif      // SLI11131

#ifdef SLI11131
//-----------------------------------------------------------------------------
// IP_Conversion_setting_576
//-----------------------------------------------------------------------------
//
// Return Value :
//
// Parameters   : BYTE vid --
//
//Set IP convsersion on 576i->576p
//
//
static void IP_Conversion_setting_576 (BYTE vid)
{
    I2C_ByteWrite (X30_EXT_VPARAMS, 0x03);
    I2C_ByteWrite (X31_EXT_HTOTAL, 0x60);
    I2C_ByteWrite (X32_EXT_HTOTAL, 0x03);
    I2C_ByteWrite (X33_EXT_HBLANK, 0x90);
    I2C_ByteWrite (X34_EXT_HBLANK, 0x00);
    I2C_ByteWrite (X35_EXT_HDLY, 0x84);
    I2C_ByteWrite (X36_EXT_HDLY, 0x00);
    I2C_ByteWrite (X37_EXT_HS_DUR, 0x3F);
    I2C_ByteWrite (X38_EXT_HS_DUR, 0x00);
    I2C_ByteWrite (X39_EXT_VTOTAL, 0x71);
    I2C_ByteWrite (X3A_EXT_VTOTAL, 0x02);
    I2C_ByteWrite (X3D_EXT_VBLANK, 0x18);
    I2C_ByteWrite (X3E_EXT_VDLY, 0x16);
    I2C_ByteWrite (X3F_EXT_VS_DUR, 0x02);

    I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
         | IP_REG_OFFSET );                             // Set offset bit to access IP registers

    I2C_ByteWrite (FUNCTION_MODE_SETTINGS, INTER_OFF);  // Disable external frame memory
    I2C_ByteWrite (VIDEO_PARAMETER_SETTINGS_AFTER, 0x01);   // Set video timings after IP conversion
    I2C_ByteWrite (HORIZONTAL_TOTAL_AFTER_LSB, 0x60);
    I2C_ByteWrite (HORIZONTAL_TOTAL_AFTER_MSB, 0x03);
    I2C_ByteWrite (HORIZONTAL_BLANK_AFTER_LSB, 0x90);
    I2C_ByteWrite (HORIZONTAL_BLANK_AFTER_MSB, 0x00);
    I2C_ByteWrite (HORIZONTAL_DELAY_AFTER_LSB, 0x84);
    I2C_ByteWrite (HORIZONTAL_DELAY_AFTER_MSB, 0x00);
    I2C_ByteWrite (HORIZONTAL_DURATION_AFTER_LSB, 0x40);
    I2C_ByteWrite (HORIZONTAL_DURATION_AFTER_MSB, 0x00);
    I2C_ByteWrite (VERTICAL_TOTAL_AFTER_LSB, 0x71);
    I2C_ByteWrite (VERTICAL_TOTAL_AFTER_MSB, 0x02);
    I2C_ByteWrite (OUTPUT_VIDEO_FORMAT_VID_AFTER, vid);
    I2C_ByteWrite (VERTICAL_BLANK_AFTER, 0x31);
    I2C_ByteWrite (VERTICAL_DELAY_AFTER, 0x2C);
    I2C_ByteWrite (VERTICAL_DURATION_AFTER, 0x05);

    I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
         | IP_CONV_EN | IP_CONV_PIX_REP );              // Set IP convsersion on 576i->576p

    I2C_ByteWrite (HORIZONTAL_PIXEL_START_POSITION_TO_LINE_MEMORY_LSB, 0x72);   // henpos = 114d
    I2C_ByteWrite (HORIZONTAL_PIXEL_START_POSITION_TO_LINE_MEMORY_MSB, 0x00);
    I2C_ByteWrite (HORIZONTAL_PIXEL_COUNT_TO_LINE_MEMORY_LSB, 0xD4);            // henwdt = 724d = 2D4h
    I2C_ByteWrite (HORIZONTAL_PIXEL_COUNT_TO_LINE_MEMORY_MSB, 0x02);
    I2C_ByteWrite (VERTICAL_PIXEL_START_POSITION_TO_LINE_MEMORY_LSB, 0x14);     // venpos = 20d
    I2C_ByteWrite (VERTICAL_PIXEL_START_POSITION_TO_LINE_MEMORY_MSB, 0x00);
    I2C_ByteWrite (VERTICAL_PIXEL_COUNT_TO_LINE_MEMORY_LSB, 0x20);              // venwdt = 288d
    I2C_ByteWrite (VERTICAL_PIXEL_COUNT_TO_LINE_MEMORY_MSB, 0x01);
    I2C_ByteWrite (OUTPUT_HSYNC_POSITION_LSB, 0x89);                            // hdpos = 137d
    I2C_ByteWrite (OUTPUT_HSYNC_POSITION_MSB, 0x00);
    I2C_ByteWrite (OUTPUT_HSYNC_WIDTH, 0x41);                                   // hdwdt = 65d
    I2C_ByteWrite (OUTPUT_VSYNC_POSITION_LSB, 0x03);                            // vdpos = 3d
    I2C_ByteWrite (OUTPUT_VSYNC_POSITION_MSB, 0x00);
    I2C_ByteWrite (OUTPUT_VSYNC_WIDTH, 0x05);                                   // vdwdt = 5d
    I2C_ByteWrite (DE_HORIZONTAL_START_POSITION_LSB, 0x84);
    I2C_ByteWrite (DE_HORIZONTAL_START_POSITION_MSB, 0x00);
    I2C_ByteWrite (DE_HORIZONTAL_END_POSITION_LSB, 0x54);
    I2C_ByteWrite (DE_HORIZONTAL_END_POSITION_MSB, 0x03);
    I2C_ByteWrite (DE_VERTICAL_START_POSITION_LSB, 0x2C);
    I2C_ByteWrite (DE_VERTICAL_START_POSITION_MSB, 0x00);
    I2C_ByteWrite (DE_VERTICAL_END_POSITION_LSB, 0x6C);
    I2C_ByteWrite (DE_VERTICAL_END_POSITION_MSB, 0x02);

    I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
         & ~IP_REG_OFFSET);             // Reset offset bit to access normal registers
}
#endif      // SLI11131

#ifdef SLI11131
//-----------------------------------------------------------------------------
// IP_Conversion_setting_1080
//-----------------------------------------------------------------------------
//
// Return Value :
//
// Parameters   : BYTE vid --
//
//Set IP convsersion on 1080i->1080p
//
//
static void IP_Conversion_setting_1080 (BYTE vid)
{
    I2C_ByteWrite (X30_EXT_VPARAMS, 0x0F);
    I2C_ByteWrite (X31_EXT_HTOTAL, 0x98);
    I2C_ByteWrite (X32_EXT_HTOTAL, 0x08);
    I2C_ByteWrite (X33_EXT_HBLANK, 0x18);
    I2C_ByteWrite (X34_EXT_HBLANK, 0x01);
    I2C_ByteWrite (X35_EXT_HDLY, 0xC0);
    I2C_ByteWrite (X36_EXT_HDLY, 0x00);
    I2C_ByteWrite (X37_EXT_HS_DUR, 0x2C);
    I2C_ByteWrite (X38_EXT_HS_DUR, 0x00);
    I2C_ByteWrite (X39_EXT_VTOTAL, 0x65);
    I2C_ByteWrite (X3A_EXT_VTOTAL, 0x04);
    I2C_ByteWrite (X3D_EXT_VBLANK, 0x16);
    I2C_ByteWrite (X3E_EXT_VDLY, 0x14);
    I2C_ByteWrite (X3F_EXT_VS_DUR, 0x05);

    I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
         | IP_REG_OFFSET );                             // Set offset bit to access IP registers

    I2C_ByteWrite (FUNCTION_MODE_SETTINGS, INTER_OFF);  // Disable external frame memory
    I2C_ByteWrite (VIDEO_PARAMETER_SETTINGS_AFTER, 0x0C);   // Set video timings after IP conversion
    I2C_ByteWrite (HORIZONTAL_TOTAL_AFTER_LSB, 0x98);
    I2C_ByteWrite (HORIZONTAL_TOTAL_AFTER_MSB, 0x08);
    I2C_ByteWrite (HORIZONTAL_BLANK_AFTER_LSB, 0x18);
    I2C_ByteWrite (HORIZONTAL_BLANK_AFTER_MSB, 0x01);
    I2C_ByteWrite (HORIZONTAL_DELAY_AFTER_LSB, 0xC0);
    I2C_ByteWrite (HORIZONTAL_DELAY_AFTER_MSB, 0x00);
    I2C_ByteWrite (HORIZONTAL_DURATION_AFTER_LSB, 0x2C);
    I2C_ByteWrite (HORIZONTAL_DURATION_AFTER_MSB, 0x00);
    I2C_ByteWrite (VERTICAL_TOTAL_AFTER_LSB, 0x65);
    I2C_ByteWrite (VERTICAL_TOTAL_AFTER_MSB, 0x04);
    I2C_ByteWrite (OUTPUT_VIDEO_FORMAT_VID_AFTER, vid);
    I2C_ByteWrite (VERTICAL_BLANK_AFTER, 0x2D);
    I2C_ByteWrite (VERTICAL_DELAY_AFTER, 0x29);
    I2C_ByteWrite (VERTICAL_DURATION_AFTER, 0x05);

    I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
         & ~IP_CONV_EN | IP_CONV_PIX_REP );             // Set IP convsersion on 1080i->1080p

    I2C_ByteWrite (HORIZONTAL_PIXEL_START_POSITION_TO_LINE_MEMORY_LSB, 0xAE);   //henpos = 174d = AEh
    I2C_ByteWrite (HORIZONTAL_PIXEL_START_POSITION_TO_LINE_MEMORY_MSB, 0x00);
    I2C_ByteWrite (HORIZONTAL_PIXEL_COUNT_TO_LINE_MEMORY_LSB, 0x84);            //  henwdt = 1924d = 784h
    I2C_ByteWrite (HORIZONTAL_PIXEL_COUNT_TO_LINE_MEMORY_MSB, 0x07);
    I2C_ByteWrite (VERTICAL_PIXEL_START_POSITION_TO_LINE_MEMORY_LSB, 0x12);     // venpos = 18d = 12h
    I2C_ByteWrite (VERTICAL_PIXEL_START_POSITION_TO_LINE_MEMORY_MSB, 0x00);
    I2C_ByteWrite (VERTICAL_PIXEL_COUNT_TO_LINE_MEMORY_LSB, 0x1C);              // venwdt = 540d = 21Ch
    I2C_ByteWrite (VERTICAL_PIXEL_COUNT_TO_LINE_MEMORY_MSB, 0x02);
    I2C_ByteWrite (OUTPUT_HSYNC_POSITION_LSB, 0xC5);                            // hdpos = 197d = C5h
    I2C_ByteWrite (OUTPUT_HSYNC_POSITION_MSB, 0x00);
    I2C_ByteWrite (OUTPUT_HSYNC_WIDTH, 0x2C);                                   // hdwdt = 44d = 2Ch
    I2C_ByteWrite (OUTPUT_VSYNC_POSITION_LSB, 0x02);                            // vdpos = 2d = 2h
    I2C_ByteWrite (OUTPUT_VSYNC_POSITION_MSB, 0x00);
    I2C_ByteWrite (OUTPUT_VSYNC_WIDTH, 0x05);                                   // vdwdt = 5d
    I2C_ByteWrite (DE_HORIZONTAL_START_POSITION_LSB, 0xC0);
    I2C_ByteWrite (DE_HORIZONTAL_START_POSITION_MSB, 0x00);
    I2C_ByteWrite (DE_HORIZONTAL_END_POSITION_LSB, 0x40);
    I2C_ByteWrite (DE_HORIZONTAL_END_POSITION_MSB, 0x08);
    I2C_ByteWrite (DE_VERTICAL_START_POSITION_LSB, 0x29);
    I2C_ByteWrite (DE_VERTICAL_START_POSITION_MSB, 0x00);
    I2C_ByteWrite (DE_VERTICAL_END_POSITION_LSB, 0x61);
    I2C_ByteWrite (DE_VERTICAL_END_POSITION_MSB, 0x04);

    I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
         & ~IP_REG_OFFSET);             // Reset offset bit to access normal registers
}
#endif      // SLI11131

void HDMI_apply_setting ()
{
    CtrlPkt_Auto = 1;
    CtrlPkt_Manual = 0;
#ifndef SLI10121
    CtrlPkt_Enable = (CtrlPkt_Enable & 0x06) | 0x61;
#endif


//printk("vid=0x%x freq=0x%x audio_ch=0x%x deep_color=0x%x down_sampling=0x%x audio_sour=0x%x\n",
//    HDMI_setting.vid,HDMI_setting.audio_freq,HDMI_setting.audio_ch,
//    HDMI_setting.deep_color,HDMI_setting.down_sampling,HDMI_setting.audio_source);

    HDMI_Audio_set_channel (HDMI_setting.audio_ch);
    HDMI_Video_set_format (HDMI_setting.vid);
    HDMI_Audio_set_freq (HDMI_setting.audio_freq);

    //HDMI_Audio_set_channel (HDMI_setting.audio_ch);
    HDMI_Video_set_color (HDMI_setting.deep_color);
    HDMI_Audio_set_ds (HDMI_setting.down_sampling);
    HDMI_Audio_SPDIF (HDMI_setting.audio_source);
    HDMI_Video_set_output ();

    HDMI_control_packet_auto_Send (CtrlPkt_Auto, CtrlPkt_Enable);

    HDMI_PHY_setup ();

    HDCP_ON = HDCP_ENABLED;
}

#ifdef KENNY_CHANGE
int HDMI_get_ext_idx(int ext_vid)
{
    int i;
    int ret_idx=-1;

    for (i = 0; i < ARRAY_SIZE(g_ext_frame_timing); i ++) {
        if (g_ext_frame_timing[i].vid_idx == ext_vid) {
            ret_idx=i;
            break;
        }
    }

    if (ret_idx == -1)
        panic("[ERROR]search ext idx fail(vid:%d)\n", ext_vid);

    return ret_idx;
}

/* check if this vid is a pre-programmed vid
 * 1 for yes, 0 for no
 */
int HDMI_is_preprog_vid(int vid)
{
    int ret = 0;

    switch(vid) {
      case VID_01_640x480p:
      case VID_02_720x480p:
      case VID_04_1280x720p:
      case VID_05_1920x1080i:
      case VID_06_720x480i:
      case VID_16_1920x1080p:
      case VID_17_720x576p:
      case VID_19_1280x720p:
      case VID_20_1920x1080i:
      case VID_21_720x576i:
      case VID_31_1920x1080p:
      case VID_32_1920x1080p:
        ret = 1;
        break;
      default:
        break;
    }

    return ret;
}
#endif /* KENNY_CHANGE */

//-----------------------------------------------------------------------------
// HDMI_Set_Color_Coefficient , Due to RGB to YCC load coefficient manually
//-----------------------------------------------------------------------------
//
// Parameters   : input format ,output format
//
//
void HDMI_Set_Color_Coefficient (unsigned char int_format,unsigned char out_format)
{
    int i;
    u32 ycc_mode = RGBTOYCC_SDTV_FULL_RANGE;
    unsigned char temp;
    unsigned char* p_val;

    if((int_format == FORMAT_RGB) &&
       (out_format == FORMAT_YCC422 || out_format == FORMAT_YCC444)) {

        if(out_format == FORMAT_YCC422){
            temp = (I2C_ByteRead (X15_AVSET1) & 0xF0);
            temp |= 0x00; // set YCC422(Embeded SYNC)04
            I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio, bit3-1 video (YCC422)
            I2C_ByteWrite (X16_VIDEO1,          0xb4);  // Video: setting
        }
        else if(out_format == FORMAT_YCC444){
            temp = (I2C_ByteRead (X15_AVSET1) & 0xF0);
            temp |= 0x00; // set YCC422(Embeded SYNC)04
            I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio, bit3-1 video (YCC422)
            I2C_ByteWrite (X16_VIDEO1,          0x74);  // Video: setting
        }
        else
            printk("%s %d no support\n",__func__,__LINE__);
        
        /*change to FULL range for white and black level issue*/
        ycc_mode = sli10121_Get_YCC_Mode();
        if(ycc_mode >= RGBTOYCC_MAX_ENRTRY)
            ycc_mode = RGBTOYCC_SDTV_FULL_RANGE;
        
        p_val = &g_rgbtoycc_parameter[ycc_mode*RGBTOTCC_COEFFICENCE_NUM];

        for( i = 0 ;i < RGBTOTCC_COEFFICENCE_NUM ; i+=2) {
            temp = p_val[i];
            I2C_ByteWrite(X18_CSC_C0_HI + i , temp&0x1F);
            temp = p_val[i+1];
            I2C_ByteWrite(X18_CSC_C0_HI + (i+1) , temp);
        }

        /*for( i = 0 ;i < RGBTOTCC_COEFFICENCE_NUM ; i+=2)
        {
            temp = I2C_ByteRead(X18_CSC_C0_HI + i );
            printk("reg=%x ,vlaue=%x\n",X18_CSC_C0_HI + i,temp);
            temp = I2C_ByteRead(X18_CSC_C0_HI + (i+1) );
            printk("reg=%x ,vlaue=%x\n",X18_CSC_C0_HI + (i+1),temp);
        }*/
    }
    else if((int_format == FORMAT_YCC422) &&
            (out_format == FORMAT_YCC422 || out_format == FORMAT_YCC444)){

        if(out_format == FORMAT_YCC422){
            temp = (I2C_ByteRead (X15_AVSET1) & 0xF0);
            temp |= 0x04; // set YCC422(Embeded SYNC)04
            I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio, bit3-1 video (YCC422)
            I2C_ByteWrite (X16_VIDEO1,          0xb5);  // Video: setting
        }
        else if(out_format == FORMAT_YCC444){
            temp = (I2C_ByteRead (X15_AVSET1) & 0xF0);
            temp |= 0x00; // set YCC444
            I2C_ByteWrite (X15_AVSET1,          0x00);  // Audio & Video: bit7-4 audio, bit3-1 video (YCC422)
            I2C_ByteWrite (X16_VIDEO1,          0x75);  // Video: setting
        }
        else
            printk("%s %d no support\n",__func__,__LINE__);

    }
    else if(int_format == FORMAT_YCC422  &&
            (out_format == FORMAT_RGB)){
#if 0/*So far , input doesn't support YCC*/

        temp = (I2C_ByteRead (X15_AVSET1) & 0xF0);
        temp |= 0x04; // set YCC422(Embeded SYNC)04
        I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio, bit3-1 video (YCC422)
        I2C_ByteWrite (X16_VIDEO1,          0x35);  // Video: setting
#endif
    }
    else if((int_format == FORMAT_RGB) &&
            (out_format == FORMAT_RGB)) {
        temp = (I2C_ByteRead (X15_AVSET1) & 0xF0);
        I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio, bit3-1 video (RGB)
        I2C_ByteWrite (X16_VIDEO1,          0x34);  // Video: setting
    }
    else{
        printk("%s %d not support %d:%d\n",__func__,__LINE__,int_format,out_format);
        /*always give RGB case*/
        temp = (I2C_ByteRead (X15_AVSET1) & 0xF0);
        I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio, bit3-1 video (RGB)
        I2C_ByteWrite (X16_VIDEO1,          0x34);  // Video: setting
    }
    printk("HDMI_Set_Color_Coefficient is %d-%d-%d\n",int_format,out_format,ycc_mode);
}

int  HDMI_Get_Color_TransMode (unsigned char int_format,unsigned char out_format)
{
    int ret ;

    if((int_format == FORMAT_RGB) &&
       (out_format == FORMAT_YCC422 || out_format == FORMAT_YCC444))
        ret = COLOR_TRANSFORM_MANUAL;
    else
        ret = COLOR_TRANSFORM_AUTO;
    return ret;
}

//-----------------------------------------------------------------------------
// HDMI_Video_set_format
//-----------------------------------------------------------------------------
//
// Parameters   : unsigned char format - Video ID
//
//
void HDMI_Video_set_format (unsigned char format)
{
    BYTE temp;
#ifdef KENNY_CHANGE
    int idx,hor_total,hor_blank,hor_delay,hor_duration,hor_front_porch;
    int ver_total,ver_blank,ver_delay,ver_duration,ver_front_porch;
    int pol_vsy,pol_hsy;
#endif
    int color_trans_mode = COLOR_TRANSFORM_AUTO;

#ifdef KENNY_CHANGE
#if 1
    HDMI_Set_Color_Coefficient(sli10121_get_lcd_output_mode(), HDMI_setting.output_format);
    color_trans_mode = HDMI_Get_Color_TransMode(sli10121_get_lcd_output_mode(), HDMI_setting.output_format);
#else
    if (HDMI_setting.output_format == FORMAT_YCC422){
        temp = (I2C_ByteRead (X15_AVSET1) & 0xF0);
        temp |= 0x00; // set YCC422(Embeded SYNC)04
        I2C_ByteWrite (X15_AVSET1,          0x00);  // Audio & Video: bit7-4 audio, bit3-1 video (YCC422)
        I2C_ByteWrite (X16_VIDEO1,          0xb4);  // Video: setting

        HDMI_Set_Color_Coefficient(HDMI_setting.output_format);
    } else {
        temp = (I2C_ByteRead (X15_AVSET1) & 0xF0);
        I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio, bit3-1 video (RGB)
        I2C_ByteWrite (X16_VIDEO1,          0x34);  // Video: setting
    }
#endif
#else
    temp = (I2C_ByteRead (X15_AVSET1) & 0xF0);
    temp |= 0x02; // set YCC422

    I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio, bit3-1 video (YCC422)
    I2C_ByteWrite (X16_VIDEO1,          0xb1);  // Video: setting
#endif

    if(color_trans_mode == COLOR_TRANSFORM_AUTO)
        I2C_ByteWrite (X3B_AVSET2,0x00);  // Video: CSC diabled
    else{
        temp = I2C_ByteRead (X3B_AVSET2);
        temp |= 0x01;
        I2C_ByteWrite (X3B_AVSET2, temp);  // Video: CSC enable
    }

    I2C_ByteWrite (X5F_PACKET_INDEX, AVI_INFO_PACKET);
    I2C_ByteWrite (X60_PACKET_HB0, 0x82);
    I2C_ByteWrite (X61_PACKET_HB1, 0x02);
    I2C_ByteWrite (X62_PACKET_HB2, 13);

    /* Harry */
    if (HDMI_setting.output_format == FORMAT_YCC422){
        I2C_ByteWrite (X64_PACKET_PB1, 0x20);       // YCC422, No Active Format, No Bar Info, NO DATA
    }
	/*refer to ECA-81 to support AVI format*/
    else if(HDMI_setting.output_format == FORMAT_YCC444)
        I2C_ByteWrite (X64_PACKET_PB1, 0x40);
    else
        I2C_ByteWrite (X64_PACKET_PB1, 0x00);

    /* See EIA-CEA-861.pdf p67 */
    switch (format) {
      case VID_16_1920x1080p:
      case VID_04_1280x720p:
        I2C_ByteWrite (X65_PACKET_PB2, 0x28);       // No colorimetry, No pic aspect, 16:9
        break;
      case VID_02_720x480p:
      case VID_17_720x576p:
      case VID_60_1024x768_RGB_EXT:
      case VID_61_1280x1024_RGB_EXT:
        I2C_ByteWrite (X65_PACKET_PB2, 0x18);       // No colorimetry, No pic aspect, 4:3
        break;
      default:
        I2C_ByteWrite (X65_PACKET_PB2, 0x08);       // No colorimetry, No pic aspect, same as org pic
        printk("%s, No specify the aspect, use No Data in AVI Packet Info. \n", __func__);
        break;
    }

    I2C_ByteWrite (X66_PACKET_PB3, 0x00);       // No IT, xvYCC601, default RGBQ, No scaling

    /* Harry:
     * if the source box is sending one of the video formats defined in this document, then it shall
     * set this field to the proper code. If a video format not listed in CEA-861-D is sent, then
     * the Video Identification Code shall be set to 0.
     */
    I2C_ByteWrite (X67_PACKET_PB4, (format & 0x7F));    // VID ID,

    if ((format == VID_06_720x480i) ||
        (format == VID_07_720x480i) ||
        (format == VID_21_720x576i) ||
        (format == VID_22_720x576i))
        I2C_ByteWrite (X68_PACKET_PB5, 0x01);   // Pixel repetition, 2 times
    else
        I2C_ByteWrite (X68_PACKET_PB5, 0x00);


    I2C_ByteWrite (X40_CTRL_PKT_EN,     0x00);  // FIXME: Auto Control Packet disable all other packets?


    if(color_trans_mode == COLOR_TRANSFORM_AUTO){
        if (COMP_MODE)
            I2C_ByteWrite (XD3_CSC_CONFIG1,     0x81);  // FIXME: 0x81->0x91? Video: color, signal no swap
        else
            I2C_ByteWrite (XD3_CSC_CONFIG1,     0x80);  // Video: color, TI video signal swap
    }
    else{
        temp = I2C_ByteRead (XD3_CSC_CONFIG1);
        temp &= ~0x80;
        I2C_ByteWrite (XD3_CSC_CONFIG1,temp);/*enable CSC convertion*/
    }

    HDMI_setting.dvi_mode = 0;

    switch(format){
        case VID_63_1280x1024P_EXT:
        case VID_64_1024x768P_EXT:
            HDMI_setting.dvi_mode = 0;
            idx = HDMI_get_ext_idx(format);
            pol_vsy = g_ext_frame_timing[idx].pol_vsy;
            pol_hsy = g_ext_frame_timing[idx].pos_hsy;
            hor_total = g_ext_frame_timing[idx].hor_sync_time + g_ext_frame_timing[idx].hor_back_porch +
                g_ext_frame_timing[idx].hor_active_video + g_ext_frame_timing[idx].hor_front_porch;
            hor_blank = g_ext_frame_timing[idx].hor_sync_time + g_ext_frame_timing[idx].hor_back_porch +
                g_ext_frame_timing[idx].hor_front_porch;
            hor_delay = g_ext_frame_timing[idx].hor_sync_time + g_ext_frame_timing[idx].hor_back_porch;
            hor_duration = g_ext_frame_timing[idx].hor_sync_time;
            hor_front_porch = g_ext_frame_timing[idx].hor_front_porch;
            ver_total = g_ext_frame_timing[idx].ver_sync_time + g_ext_frame_timing[idx].ver_back_porch +
                g_ext_frame_timing[idx].ver_active_video + g_ext_frame_timing[idx].ver_front_porch;
            ver_blank = g_ext_frame_timing[idx].ver_sync_time + g_ext_frame_timing[idx].ver_back_porch +
                g_ext_frame_timing[idx].ver_front_porch;
            ver_delay = g_ext_frame_timing[idx].ver_sync_time + g_ext_frame_timing[idx].ver_back_porch;
            ver_duration = g_ext_frame_timing[idx].ver_sync_time;
            ver_front_porch = g_ext_frame_timing[idx].ver_front_porch;

//            printk("[0]pol_vsy=%d pol_hsy=%d\n",pol_vsy,pol_hsy);
//            printk("[0]h_total=%d h_blank=%d h_delay=%d h_dura=%d h_frnot=%d\n",
//                hor_total,hor_blank,hor_delay,hor_duration,hor_front_porch);
//            printk("[0]v_total=%d v_blank=%d v_delay=%d v_dura=%d v_front=%d\n",
//                ver_total,ver_blank,ver_delay,ver_duration,ver_front_porch);

            I2C_ByteWrite(X30_EXT_VPARAMS, (0x01 | pol_vsy << 3 | pol_hsy << 2));
            I2C_ByteWrite(X31_EXT_HTOTAL, hor_total & 0xFF);
            I2C_ByteWrite(X32_EXT_HTOTAL, (hor_total >> 8) & 0xff);
            I2C_ByteWrite(X33_EXT_HBLANK, hor_blank & 0xFF);
            I2C_ByteWrite(X34_EXT_HBLANK, (hor_blank >> 8) & 0xFF);
            I2C_ByteWrite(X35_EXT_HDLY, hor_delay & 0xFF);
            I2C_ByteWrite(X36_EXT_HDLY, (hor_delay >> 8) & 0xFF);
            I2C_ByteWrite(X37_EXT_HS_DUR, hor_duration & 0xFF);
            I2C_ByteWrite(X38_EXT_HS_DUR, (hor_duration >> 8) & 0xFF);
            I2C_ByteWrite(X39_EXT_VTOTAL, ver_total & 0xFF);
            I2C_ByteWrite(X3A_EXT_VTOTAL, (ver_total >> 8) & 0xFF);
            I2C_ByteWrite(X3D_EXT_VBLANK, ver_blank & 0xFF);
            I2C_ByteWrite(X3E_EXT_VDLY, ver_delay & 0xFF);
            I2C_ByteWrite(X3F_EXT_VS_DUR, ver_duration & 0xFF);

            I2C_ByteWrite(X52_HSYNC_PLACE_656, hor_front_porch & 0xFF);
            I2C_ByteWrite(X53_HSYNC_PLACE_656, (hor_front_porch >> 8) & 0xFF);
            I2C_ByteWrite(X54_VSYNC_PLACE_656, ver_front_porch & 0xFF);
            I2C_ByteWrite(X55_VSYNC_PLACE_656, (ver_front_porch >> 8) & 0xFF);
            /* VID */
            I2C_ByteWrite (X5F_PACKET_INDEX, AVI_INFO_PACKET);
            I2C_ByteWrite (X67_PACKET_PB4, 0);    //0 means external vid in the CEA-861D
            break;

        case VID_60_1024x768_RGB_EXT: /* RGB */
        case VID_61_1280x1024_RGB_EXT:
            HDMI_setting.dvi_mode = 0;
            idx = HDMI_get_ext_idx(format);
            pol_vsy = g_ext_frame_timing[idx].pol_vsy;
            pol_hsy = g_ext_frame_timing[idx].pos_hsy;
            hor_total = g_ext_frame_timing[idx].hor_sync_time + g_ext_frame_timing[idx].hor_back_porch +
                g_ext_frame_timing[idx].hor_active_video + g_ext_frame_timing[idx].hor_front_porch;
            hor_blank = g_ext_frame_timing[idx].hor_sync_time + g_ext_frame_timing[idx].hor_back_porch +
                g_ext_frame_timing[idx].hor_front_porch;
            hor_delay = g_ext_frame_timing[idx].hor_sync_time + g_ext_frame_timing[idx].hor_back_porch;
            hor_duration = g_ext_frame_timing[idx].hor_sync_time;
            hor_front_porch = g_ext_frame_timing[idx].hor_front_porch;
            ver_total = g_ext_frame_timing[idx].ver_sync_time + g_ext_frame_timing[idx].ver_back_porch +
                g_ext_frame_timing[idx].ver_active_video + g_ext_frame_timing[idx].ver_front_porch;
            ver_blank = g_ext_frame_timing[idx].ver_sync_time + g_ext_frame_timing[idx].ver_back_porch +
                g_ext_frame_timing[idx].ver_front_porch;
            ver_delay = g_ext_frame_timing[idx].ver_sync_time + g_ext_frame_timing[idx].ver_back_porch;
            ver_duration = g_ext_frame_timing[idx].ver_sync_time;
            ver_front_porch = g_ext_frame_timing[idx].ver_front_porch;

//            printk("[2]pol_vsy=%d pol_hsy=%d\n",pol_vsy,pol_hsy);
//            printk("[2]h_total=%d h_blank=%d h_delay=%d h_dura=%d h_frnot=%d\n",
//                hor_total,hor_blank,hor_delay,hor_duration,hor_front_porch);
//            printk("[2]v_total=%d v_blank=%d v_delay=%d v_dura=%d v_front=%d\n",
//                ver_total,ver_blank,ver_delay,ver_duration,ver_front_porch);

            I2C_ByteWrite(X30_EXT_VPARAMS, (0x01 | pol_vsy << 3 | pol_hsy << 2));
            I2C_ByteWrite(X31_EXT_HTOTAL, hor_total & 0xFF);
            I2C_ByteWrite(X32_EXT_HTOTAL, (hor_total >> 8) & 0xFF);
            I2C_ByteWrite(X33_EXT_HBLANK, hor_blank & 0xFF);
            I2C_ByteWrite(X34_EXT_HBLANK, (hor_blank >> 8) & 0xFF);
            I2C_ByteWrite(X35_EXT_HDLY, hor_delay & 0xFF);
            I2C_ByteWrite(X36_EXT_HDLY, (hor_delay >> 8) & 0xFF);
            I2C_ByteWrite(X37_EXT_HS_DUR, hor_duration & 0xFF);
            I2C_ByteWrite(X38_EXT_HS_DUR, (hor_duration >> 8) & 0xFF);
            I2C_ByteWrite(X39_EXT_VTOTAL, ver_total & 0xFF);
            I2C_ByteWrite(X3A_EXT_VTOTAL, (ver_total >> 8) & 0xFF);
            I2C_ByteWrite(X3D_EXT_VBLANK, ver_blank & 0xFF);
            I2C_ByteWrite(X3E_EXT_VDLY, ver_delay & 0xFF);
            I2C_ByteWrite(X3F_EXT_VS_DUR, ver_duration & 0xFF);
            I2C_ByteWrite(X52_HSYNC_PLACE_656, hor_front_porch & 0xFF);
            I2C_ByteWrite(X53_HSYNC_PLACE_656, (hor_front_porch >> 8) & 0xFF);
            I2C_ByteWrite(X54_VSYNC_PLACE_656, ver_front_porch & 0xFF);
            I2C_ByteWrite(X55_VSYNC_PLACE_656, (ver_front_porch >> 8) & 0xFF);
            /* VID */
            I2C_ByteWrite (X5F_PACKET_INDEX, AVI_INFO_PACKET);
            I2C_ByteWrite (X67_PACKET_PB4, 0);    //0 means external vid in the CEA-861D
            break;

        default:
            if (HDMI_is_preprog_vid(format & 0x7F)) {
                /* pre-programmed vid */
                I2C_ByteWrite (X5F_PACKET_INDEX, AVI_INFO_PACKET);
                I2C_ByteWrite (X67_PACKET_PB4, (format & 0x7F));    // VID ID
                temp = I2C_ByteRead(X30_EXT_VPARAMS);
                I2C_ByteWrite(X30_EXT_VPARAMS, temp & 0xFE); //disable external video setting
            } else {
                panic("This vid %d is not supported! \n", format);
            }
            break;
    } /* switch */
}

//-----------------------------------------------------------------------------
// HDMI_Audio_set_freq
//-----------------------------------------------------------------------------
//
// Parameters   : unsigned char freq - audio frequency
//
// N value is set for 8-bit and 12-bit color (they use same value)
// For 10-bit color, need to assign new N value - FIXME
void HDMI_Audio_set_freq (unsigned char freq)
{
    BYTE temp;

    switch (freq)
    {
        case AUD_32K:
            I2C_ByteWrite (X01_N19_16,          0x00);  // Audio:
            I2C_ByteWrite (X02_N15_8,           0x10);  // Audio: 32K
            I2C_ByteWrite (X03_N7_0,            0x00);  // Audio:
            I2C_ByteWrite (X04_SPDIF_FS,        0x20);  // Auido: SPDIF not used
            I2C_ByteWrite (X0A_AUDIO_SOURCE,    0x00);  // Audio: I2S for internal
            I2C_ByteWrite (X0B_AUDIO_SET2,      0x40);  // Audio: setting2
            I2C_ByteWrite (X0D_DSD_MODE,        0x00);  // Audio: DSD audio disbled
            I2C_ByteWrite (X11_ASTATUS1,        0x00);  // Audio: Original frequency not indicated(defult)
            I2C_ByteWrite (X12_ASTATUS2,        0x00);  // Audio: misc setting
            I2C_ByteWrite (X13_CAT_CODE,        0x00);  // Audio: downsampling?
#ifdef SLI10121
            I2C_ByteWrite (X14_A_SOURCE,        0x02);  // Audio: downsampling?
#else
            I2C_ByteWrite (X14_A_SOURCE,        0x00);  // Audio: downsampling?
#endif
            temp = (I2C_ByteRead (X15_AVSET1) & 0x0F);
            temp |= 0x30; // set audio freq 32K
            I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio frequncy (48K), bit3-1 video
            break;
        case AUD_44K:
            I2C_ByteWrite (X01_N19_16,          0x00);  // Audio:
            I2C_ByteWrite (X02_N15_8,           0x18);  // Audio: 44.1K
            I2C_ByteWrite (X03_N7_0,            0x80);  // Audio:
            I2C_ByteWrite (X04_SPDIF_FS,        0x20);  // Auido: SPDIF not used
            I2C_ByteWrite (X0A_AUDIO_SOURCE,    0x00);  // Audio: I2S for internal
            I2C_ByteWrite (X0B_AUDIO_SET2,      0x40);  // Audio: setting2
            I2C_ByteWrite (X0D_DSD_MODE,        0x00);  // Audio: DSD audio disbled
            I2C_ByteWrite (X11_ASTATUS1,        0x00);  // Audio: Original frequency not indicated(defult)
            I2C_ByteWrite (X12_ASTATUS2,        0x00);  // Audio: misc setting
            I2C_ByteWrite (X13_CAT_CODE,        0x00);  // Audio: downsampling?
#ifdef SLI10121
            I2C_ByteWrite (X14_A_SOURCE,        0x02);  // Audio: downsampling?
#else
            I2C_ByteWrite (X14_A_SOURCE,        0x00);  // Audio: downsampling?
#endif
            temp = (I2C_ByteRead (X15_AVSET1) & 0x0F);
            temp |= 0x00; // set audio freq 44.1K
            I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio frequncy (48K), bit3-1 video
            break;
        case AUD_48K:
            I2C_ByteWrite (X01_N19_16,          0x00);  // Audio:
            I2C_ByteWrite (X02_N15_8,           0x18);  // Audio: 48K
            I2C_ByteWrite (X03_N7_0,            0x00);  // Audio:
            I2C_ByteWrite (X04_SPDIF_FS,        0x20);  // Auido: SPDIF not used
            I2C_ByteWrite (X0A_AUDIO_SOURCE,    0x00);  // Audio: I2S for internal
            I2C_ByteWrite (X0B_AUDIO_SET2,      0x40);  // Audio: setting2
            I2C_ByteWrite (X0D_DSD_MODE,        0x00);  // Audio: DSD audio disbled
            I2C_ByteWrite (X11_ASTATUS1,        0x00);  // Audio: Original frequency not indicated(defult)
            I2C_ByteWrite (X12_ASTATUS2,        0x00);  // Audio: misc setting
            I2C_ByteWrite (X13_CAT_CODE,        0x00);  // Audio: downsampling?
#ifdef SLI10121
            I2C_ByteWrite (X14_A_SOURCE,        0x02);  // Audio: downsampling?
#else
            I2C_ByteWrite (X14_A_SOURCE,        0x00);  // Audio: downsampling?
#endif
            temp = (I2C_ByteRead (X15_AVSET1) & 0x0F);
            temp |= 0x20; // set audio freq 48K
            I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio frequncy (48K), bit3-1 video
            break;

        case AUD_96K:
            I2C_ByteWrite (X01_N19_16,          0x00);  // Audio:
            I2C_ByteWrite (X02_N15_8,           0x30);  // Audio: 96K
            I2C_ByteWrite (X03_N7_0,            0x00);  // Audio:
            I2C_ByteWrite (X04_SPDIF_FS,        0x20);  // Auido: SPDIF not used
            I2C_ByteWrite (X0A_AUDIO_SOURCE,    0x00);  // Audio: I2S for internal
#ifdef SLI10121
            I2C_ByteWrite (X0B_AUDIO_SET2,      0x00);  // Audio: setting2
#else
            I2C_ByteWrite (X0B_AUDIO_SET2,      0x40);  // Audio: setting2
#endif
            I2C_ByteWrite (X0D_DSD_MODE,        0x00);  // Audio: DSD audio disbled
            I2C_ByteWrite (X11_ASTATUS1,        0x00);  // Audio: Original frequency not indicated(defult)
            I2C_ByteWrite (X12_ASTATUS2,        0x00);  // Audio: misc setting
            I2C_ByteWrite (X13_CAT_CODE,        0x00);  // Audio: downsampling?
            I2C_ByteWrite (X14_A_SOURCE,        0x00);  // Audio: downsampling?
            temp = (I2C_ByteRead (X15_AVSET1) & 0x0F);
            temp |= 0xA0; // set audio freq 96K
            I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio frequncy (96K), bit3-1 video
            break;

        case AUD_192K:
            I2C_ByteWrite (X01_N19_16,          0x00);  // Audio:
            I2C_ByteWrite (X02_N15_8,           0x60);  // Audio: 192K
            I2C_ByteWrite (X03_N7_0,            0x00);  // Audio:
            I2C_ByteWrite (X04_SPDIF_FS,        0x20);  // Auido: SPDIF not used
            I2C_ByteWrite (X0A_AUDIO_SOURCE,    0x00);  // Audio: I2S for internal
#ifdef SLI10121
            I2C_ByteWrite (X0B_AUDIO_SET2,      0x00);  // Audio: setting2
#else
            I2C_ByteWrite (X0B_AUDIO_SET2,      0x40);  // Audio: setting2
#endif
            I2C_ByteWrite (X0D_DSD_MODE,        0x00);  // Audio: DSD audio disbled
            I2C_ByteWrite (X11_ASTATUS1,        0x00);  // Audio: Original frequency not indicated(defult)
            I2C_ByteWrite (X12_ASTATUS2,        0x00);  // Audio: misc setting
            I2C_ByteWrite (X13_CAT_CODE,        0x00);  // Audio: downsampling?
            I2C_ByteWrite (X14_A_SOURCE,        0x00);  // Audio: downsampling?
            temp = (I2C_ByteRead (X15_AVSET1) & 0x0F);
            temp |= 0xE0; // set audio freq 192K
            I2C_ByteWrite (X15_AVSET1,          temp);  // Audio & Video: bit7-4 audio frequncy (192K), bit3-1 video
            break;

        default:
            break;
    }
}

//-----------------------------------------------------------------------------
// HDMI_Audio_set_channel
//-----------------------------------------------------------------------------
//
// Parameters   : unsigned char channel - 2ch or 8ch
//
void HDMI_Audio_set_channel (unsigned char channel) {

    I2C_ByteWrite (X5F_PACKET_INDEX, AUDIO_INFO_PACKET);
    I2C_ByteWrite (X60_PACKET_HB0, 0x84);
    I2C_ByteWrite (X61_PACKET_HB1, 0x01);
    I2C_ByteWrite (X62_PACKET_HB2, 10);

    switch (channel)
    {
        case AUD_2CH:
            I2C_ByteWrite (X0C_I2S_MODE,   0x04);         // Audio: I2S 2ch (0x3C for 8ch) + I2S
            I2C_ByteWrite (X64_PACKET_PB1, 0x01);       // Audio: channel - 2ch
            I2C_ByteWrite (X66_PACKET_PB3, 0x00);
            I2C_ByteWrite (X67_PACKET_PB4, 0x00);       // Speaker Allocation - 0x00 for 2ch
            I2C_ByteWrite (X68_PACKET_PB5, 0x00);       // DM_INH = 0, LShiftValue = 0
            break;

        case AUD_8CH:
            I2C_ByteWrite (X0C_I2S_MODE,   0x3C);  // Audio: I2S 8ch (0x04 for 2ch)
            I2C_ByteWrite (X64_PACKET_PB1, 0x07);       // Audio: channel - 8ch
            I2C_ByteWrite (X66_PACKET_PB3, 0x00);
            I2C_ByteWrite (X67_PACKET_PB4, 0x13);       // Speaker Allocation - 8ch
            I2C_ByteWrite (X68_PACKET_PB5, 0x00);       // DM_INH = 0, LShiftValue = 0

            I2C_ByteWrite (X14_A_SOURCE,        0x0B);  // Audio: Speaker mapping relate 10/31/07
            // 0x14 [0] 1 - max 24 bit, 0 - max 20 bit
            // 0x14 [3:1] 101 - 24 bit, 001 - 20 bit

            break;

        default:
            break;
    }
}

//-----------------------------------------------------------------------------
// HDMI_Audio_SPDIF ()
//-----------------------------------------------------------------------------
//
// Parameters   : unsigned char spdif - enable (1) or disable (0) SPDIF audio
//
void HDMI_Audio_SPDIF (unsigned char spdif)
{
    BYTE temp0A;
    temp0A = I2C_ByteRead (X0A_AUDIO_SOURCE) & 0xF7;
    switch (spdif)
    {
        case AUD_SPDIF:
            //0x0A <= bit 3 is high
            I2C_ByteWrite (X0A_AUDIO_SOURCE, temp0A | 0x08);
            break;
        case AUD_I2S:
            I2C_ByteWrite (X0A_AUDIO_SOURCE, temp0A);
            break;
        default:
            break;
    }
}

//-----------------------------------------------------------------------------
// HDMI_Audio_set_ds
//-----------------------------------------------------------------------------
//
// Parameters   : unsigned char ds - set downsampling option
//
void HDMI_Audio_set_ds (unsigned char ds)
{
    BYTE temp0A;
    temp0A = I2C_ByteRead (X0A_AUDIO_SOURCE) & 0x9F;

    switch (ds)
    {
        case DS_none:
            I2C_ByteWrite (X0A_AUDIO_SOURCE, temp0A );   // Audio: downsampling none (bit 6:5 = 00)
            break;
        case DS_2:
            I2C_ByteWrite (X0A_AUDIO_SOURCE, temp0A | 0x20);    // Audio: downsampling none (bit 6:5 = 01)
            break;
        case DS_4:
            I2C_ByteWrite (X0A_AUDIO_SOURCE, temp0A | 0x40);    // Audio: downsampling none (bit 6:5 = 10)
            break;
        default:
            break;
    }
}

// Set DVI/HDMI mode, Output Video Format, info frame output format
void HDMI_Video_set_output (void) {
    BYTE temp, output;


    output = HDMI_setting.output_format;      // 2009.05.23

    if (HDMI_setting.dvi_mode == 1)
    {
        // setting to DVI mode
        temp = I2C_ByteRead (XAF_HDCP_CTRL);
        I2C_ByteWrite (XAF_HDCP_CTRL, temp & 0xFD);

        // DVI mode is always FORMAT_RGB
        temp = I2C_ByteRead (X16_VIDEO1) & 0x3F;
        I2C_ByteWrite (X16_VIDEO1, temp);

        panic("%s, wrong output format! \n", __func__);
    }
    else
    {
        // Temoprary: Change back from manual set up RGB for 480p ###(2/2)
        //I2C_ByteWrite (0xD3, 0x81);

        // setting to HDMI mode
        temp = I2C_ByteRead (XAF_HDCP_CTRL);
        I2C_ByteWrite (XAF_HDCP_CTRL, temp | 0x02); /* the content should be 0x12 */

        switch (output) {
            case FORMAT_RGB:
                temp = I2C_ByteRead (X16_VIDEO1) & 0x3F;
                I2C_ByteWrite (X16_VIDEO1, temp | (0x3 << 4));

#ifdef SLI10121 // SLI11131 T65
                //PB1[6:5] = 00 when HDMI RGB
                temp = I2C_ByteRead (X64_PACKET_PB1) & 0x1F;
                I2C_ByteWrite (X64_PACKET_PB1, temp);
#endif
                break;

            case FORMAT_YCC422:
                temp = I2C_ByteRead (X16_VIDEO1) & 0x3F;
                I2C_ByteWrite (X16_VIDEO1, temp | 0x80);

#ifdef SLI10121
                //PB1[6:5] = 01 when HDMI YCC422
                temp = I2C_ByteRead (X64_PACKET_PB1) & 0x1F;
                I2C_ByteWrite (X64_PACKET_PB1, temp | 0x20);
#endif
                break;

            case FORMAT_YCC444:
                temp = I2C_ByteRead (X16_VIDEO1) & 0x3F;
                I2C_ByteWrite (X16_VIDEO1, temp | 0x40);
#ifdef SLI10121
                //PB1[6:5] = 1X when HDMI YCC444
                temp = I2C_ByteRead (X64_PACKET_PB1) & 0x1F;
                I2C_ByteWrite (X64_PACKET_PB1, temp | 0x40);
#endif
                break;

            default:
                // Default to YCC422 setting
                temp = I2C_ByteRead (X16_VIDEO1) & 0x3F;
                I2C_ByteWrite (X16_VIDEO1, temp | 0x80);
#ifdef SLI10121
                //PB1[6:5] = 01 when HDMI YCC422
                temp = I2C_ByteRead (X64_PACKET_PB1) & 0x1F;
                I2C_ByteWrite (X64_PACKET_PB1, temp | 0x20);
#endif
                break;
        }
    }
}

void HDMI_Video_set_color (unsigned char color) {
    BYTE temp17, temp16;

    temp16 = I2C_ByteRead (X16_VIDEO1);       //  Input data width
    temp17 = I2C_ByteRead (X17_DC_REG);       //  Output data width

    switch (color)
    {
        case DEEP_COLOR_8BIT:
            temp17 = (temp17 & 0x3F) | 0x00; // 8 bit
            temp16 = (temp16 & 0xCF) | 0x30; // 8 bit
            break;
        case DEEP_COLOR_10BIT:
            temp17 = (temp17 & 0x3F) | 0x40; // 10 bit
            temp16 = (temp16 & 0xCF) | 0x10; // 10 bit
            break;
        case DEEP_COLOR_12BIT:
            temp17 = (temp17 & 0x3F) | 0x80; // 12 bit
            temp16 = (temp16 & 0xCF) | 0x00; // 12 bit
            break;
        default:
            temp17 = (temp17 & 0x3F) | 0x00; // 8 bit
            temp16 = (temp16 & 0xCF) | 0x30; // 8 bit
            break;
    }

    I2C_ByteWrite (X17_DC_REG, temp17);
    I2C_ByteWrite (X16_VIDEO1, temp16);
}



//-----------------------------------------------------------------------------
// HDMI_PHY_setup ()
//-----------------------------------------------------------------------------
//
// Choose the best PHY setting for the current video setting (global var).
//
void HDMI_PHY_setup (void)
{
    //@----------------------Necessary to PHY setup in PowerMode B----------------------
    BYTE mode_temp;

    mode_temp = I2C_ByteRead (X00_SYSTEM_CONTROL) & 0xF0;
    I2C_ByteWrite (X00_SYSTEM_CONTROL,  0x20);          //Set to powermode B
    DelayMs(1);                                         //Wait at least 500 us
    //------------------------------------------2009.04.22 Bongryong@SLI ----------@
    if (HDMI_setting.deep_color == DEEP_COLOR_8BIT)
    {
        switch (HDMI_setting.vid & 0x7F)
        {
            case VID_02_720x480p:
            case VID_17_720x576p:
                HDMI_PHY_setting_27();
                break;

            case VID_04_1280x720p:  // 720p  60Hz
            case VID_19_1280x720p:  // 720p  50Hz
            case VID_05_1920x1080i: // 1080i 60Hz
            case VID_20_1920x1080i: // 1080i 50Hz
            case VID_32_1920x1080p: // 1080p 24Hz
            case VID_33_1920x1080p: // 1080p 25Hz
            case VID_34_1920x1080p: // 1080p 30Hz
            case VID_39_1920x1080i: // 1080i 50Hz
            case VID_60_1024x768_RGB_EXT:
            case VID_64_1024x768P_EXT:
                HDMI_PHY_setting_74();
                break;

            case VID_16_1920x1080p: // 1080p 60Hz
            case VID_31_1920x1080p: // 1080p 50Hz
            case VID_40_1920x1080i: // 1080i 100Hz
            case VID_61_1280x1024_RGB_EXT:
            case VID_63_1280x1024P_EXT:
                HDMI_PHY_setting_148();
                break;

            default:
                panic("%s, wrong vid = %d \n", __func__, HDMI_setting.vid);
                break;
        }
    } else {
        panic("%s, wrong value:%d! \n", __func__, HDMI_setting.deep_color);
    }
}

//-----------------------------------------------------------------------------
// HDMI_PHY_setting_XX ()
//-----------------------------------------------------------------------------
//
// Actual PHY values for each TMDS clock setting
// Note:
// This function only can be called in PowerModeB
//

void phy_reset(void)
{
    if (PowerMode != PowerMode_B)
        panic("%s, %d not in PowerMode_B \n", __func__, PowerMode);

    I2C_ByteWrite(X00_SYSTEM_CONTROL, 0x2C);
    udelay(100);
    I2C_ByteWrite(X00_SYSTEM_CONTROL, 0x20);
    mdelay(1);
}

// after Power Save mode is set to mode_e
// PHY setting for TMDS clock 27 (480i, 480p)
void HDMI_PHY_setting_27 ()
{
    BYTE CurrentPowerMode = PowerMode;

    /* switch to PowerModeB, refer to p171 */
    HDMI_System_PD(PowerMode_B);

#ifdef CONFIG_PLATFORM_GM8210
    I2C_ByteWrite (X57_PHY_CTRL, 0x00);
    phy_reset();
    I2C_ByteWrite (X58_PHY_CTRL, 0x04);
    phy_reset();
    I2C_ByteWrite (X59_PHY_CTRL, 0x0F);
    phy_reset();
    I2C_ByteWrite (X5A_PHY_CTRL, 0x3E);
    phy_reset();
    I2C_ByteWrite (X5B_PHY_CTRL, 0x1F);
    phy_reset();
    I2C_ByteWrite (X5C_PHY_CTRL, 0x0F);
    phy_reset();
    I2C_ByteWrite (X5D_PHY_CTRL, 0x41);
    phy_reset();
    I2C_ByteWrite (X5E_PHY_CTRL, 0x00);
    phy_reset();
    I2C_ByteWrite (X56_PHY_CTRL, 0x11);
    phy_reset();

#elif defined(CONFIG_PLATFORM_GM8287)

    I2C_ByteWrite (X57_PHY_CTRL, 0x00);
    phy_reset();
    I2C_ByteWrite (X58_PHY_CTRL, 0x04);
    phy_reset();
    I2C_ByteWrite (X59_PHY_CTRL, 0x0F);
    phy_reset();
    I2C_ByteWrite (X5A_PHY_CTRL, 0x3E);
    phy_reset(); 
    I2C_ByteWrite (X5B_PHY_CTRL, 0x1F);
    phy_reset();
    I2C_ByteWrite (X5C_PHY_CTRL, 0x0F);
    phy_reset();
    I2C_ByteWrite (X5D_PHY_CTRL, 0x41);
    I2C_ByteWrite (X5E_PHY_CTRL, 0x00);
    phy_reset();
    I2C_ByteWrite (X56_PHY_CTRL, 0x11);
    phy_reset();
#else
    #error "undefined platform!"
#endif

    I2C_ByteWrite(X00_SYSTEM_CONTROL, 0x4C);
    udelay(100);
    I2C_ByteWrite(X00_SYSTEM_CONTROL, 0x48);
    udelay(100);

    HDMI_System_PD(CurrentPowerMode); //It should be PowerMode_D

    printk("%s \n", __func__);
}

// PHY setting for TMDS clock 54 & 74.25 (720p, 1080i)
void HDMI_PHY_setting_74 ()
{
    BYTE CurrentPowerMode = PowerMode;

    /* switch to PowerModeB, refer to p171 */
    HDMI_System_PD(PowerMode_B);

    I2C_ByteWrite (X57_PHY_CTRL, 0x00);
    phy_reset();
    I2C_ByteWrite (X58_PHY_CTRL, 0x04);
    phy_reset();
    I2C_ByteWrite (X59_PHY_CTRL, 0x0F);
    phy_reset();
    I2C_ByteWrite (X5A_PHY_CTRL, 0x2E);
    phy_reset();    
    I2C_ByteWrite (X5B_PHY_CTRL, 0x0F); //org 0x9A
    phy_reset();
    I2C_ByteWrite (X5C_PHY_CTRL, 0x0E);
    phy_reset();    
    I2C_ByteWrite (X5D_PHY_CTRL, 0x41);
    phy_reset();
    I2C_ByteWrite (X5E_PHY_CTRL, 0x00);
    phy_reset();
    I2C_ByteWrite (X56_PHY_CTRL, 0x1A); //org 0x1F
    phy_reset();
    I2C_ByteWrite(X00_SYSTEM_CONTROL, 0x4C);
    udelay(100);
    I2C_ByteWrite(X00_SYSTEM_CONTROL, 0x48);
    udelay(100);

    HDMI_System_PD(CurrentPowerMode); //It should be PowerMode_D

    printk("%s \n", __func__);
}

// PHY setting for TMDS clock 148.5 (1080p)
void HDMI_PHY_setting_148 ()
{
    BYTE CurrentPowerMode = PowerMode;

    /* switch to PowerModeB, refer to p171 */
    HDMI_System_PD(PowerMode_B);

#ifdef CONFIG_PLATFORM_GM8210
    I2C_ByteWrite (X57_PHY_CTRL, 0x00);
    phy_reset();
    I2C_ByteWrite (X58_PHY_CTRL, 0x04);
    phy_reset();
    I2C_ByteWrite (X59_PHY_CTRL, 0x0F);
    phy_reset();
    I2C_ByteWrite (X5A_PHY_CTRL, 0x2E);
    phy_reset();
    I2C_ByteWrite (X5B_PHY_CTRL, 0x0F); //org is 0x9A
    phy_reset();
    I2C_ByteWrite (X5C_PHY_CTRL, 0x0E);
    phy_reset();
    I2C_ByteWrite (X5D_PHY_CTRL, 0x41);
    phy_reset();
    I2C_ByteWrite (X5E_PHY_CTRL, 0x00);
    phy_reset();
    I2C_ByteWrite (X56_PHY_CTRL, 0x1A); //org is 0x1F
    phy_reset();

#elif defined(CONFIG_PLATFORM_GM8287)

    I2C_ByteWrite (X57_PHY_CTRL, 0x00);
    phy_reset();
    I2C_ByteWrite (X58_PHY_CTRL, 0x04);
    phy_reset();
    I2C_ByteWrite (X59_PHY_CTRL, 0x0F);
    phy_reset();
    I2C_ByteWrite (X5A_PHY_CTRL, 0x2E);
    phy_reset();
#if 1//follow sli suggestion
    I2C_ByteWrite (X5B_PHY_CTRL, 0x1A);
#else
    I2C_ByteWrite (X5B_PHY_CTRL, 0x0F); //org is 0x9A
#endif
    phy_reset();
    I2C_ByteWrite (X5C_PHY_CTRL, 0x0E);
    phy_reset();
    I2C_ByteWrite (X5D_PHY_CTRL, 0x42);//follow sli suggestion
    phy_reset();
    I2C_ByteWrite (X5E_PHY_CTRL, 0x04);//follow sli suggestion
    phy_reset();
    I2C_ByteWrite (X56_PHY_CTRL, 0x1A); //org is 0x1F
    phy_reset();
#else
    #error "undefined platform"
#endif

    I2C_ByteWrite(X00_SYSTEM_CONTROL, 0x4C);
    udelay(100);
    I2C_ByteWrite(X00_SYSTEM_CONTROL, 0x48);
    udelay(100);

    HDMI_System_PD(CurrentPowerMode); //It should be PowerMode_D
    printk("%s \n", __func__);
}

/*
 * Update the register from external
 */
void hdmi_update_phyreg(unsigned char *phy_array)
{
    BYTE CurrentPowerMode = PowerMode;

    if (PowerMode != PowerMode_E) {
        printk("Error, the system is not actve in  PowerMode_E! \n");
        return;
    }

    while (in_bottom_isr)
        msleep(1);

    external_reg_update = 1;

    /* switch to PowerModeB, refer to p171 */
    HDMI_System_PD(PowerMode_B);

    I2C_ByteWrite (X57_PHY_CTRL, phy_array[1]);
    phy_reset();
    I2C_ByteWrite (X58_PHY_CTRL, phy_array[2]);
    phy_reset();
    I2C_ByteWrite (X59_PHY_CTRL, phy_array[3]);
    phy_reset();
    I2C_ByteWrite (X5A_PHY_CTRL, phy_array[4]);
    phy_reset();
    I2C_ByteWrite (X5B_PHY_CTRL, phy_array[5]);
    phy_reset();
    I2C_ByteWrite (X5C_PHY_CTRL, phy_array[6]);
    phy_reset();
    I2C_ByteWrite (X5D_PHY_CTRL, phy_array[7]);
    phy_reset();
    I2C_ByteWrite (X5E_PHY_CTRL, phy_array[8]);
    phy_reset();
    I2C_ByteWrite (X56_PHY_CTRL, phy_array[0]);
    phy_reset();

    I2C_ByteWrite(X00_SYSTEM_CONTROL, 0x4C);
    udelay(100);
    I2C_ByteWrite(X00_SYSTEM_CONTROL, 0x48);
    udelay(100);

    HDMI_System_PD(CurrentPowerMode); //It should be PowerMode_D

    external_reg_update = 0;

    printk("%s, powerMode = 0x%x \n", __func__, CurrentPowerMode);
}

/*
 * Update the register from external
 */
void hdmi_update_reg(unsigned char off, unsigned char data)
{
    BYTE CurrentPowerMode = PowerMode;

    if (PowerMode != PowerMode_E) {
        printk("Error, the system is not actve in  PowerMode_E! \n");
        return;
    }

    while (in_bottom_isr)
        msleep(1);

    external_reg_update = 1;

    /* switch to PowerModeB, refer to p171 */
    HDMI_System_PD(PowerMode_B);

    I2C_ByteWrite (off, data);

    I2C_ByteWrite(X00_SYSTEM_CONTROL, 0x4C);
    udelay(100);
    I2C_ByteWrite(X00_SYSTEM_CONTROL, 0x48);
    udelay(100);

    HDMI_System_PD(CurrentPowerMode);

    external_reg_update = 0;
}

#define EDID_UPPER_NIBBLE( x ) \
        (((128|64|32|16) & (x)) >> 4)

#define EDID_LOWER_NIBBLE( x ) \
        ((1|2|4|8) & (x))

#define EDID_COMBINE_HI_8LO( hi, lo ) \
        ( (((unsigned)hi) << 8) | (unsigned)lo )

#define EDID_COMBINE_HI_4LO( hi, lo ) \
        ( (((unsigned)hi) << 4) | (unsigned)lo )

#define EDID_PIXEL_CLOCK_LO     (unsigned)block[ 0 ]
#define EDID_PIXEL_CLOCK_HI     (unsigned)block[ 1 ]
#define EDID_PIXEL_CLOCK	    (EDID_COMBINE_HI_8LO(EDID_PIXEL_CLOCK_HI, \
                                 EDID_PIXEL_CLOCK_LO)*10000)

#define EDID_H_ACTIVE_LO        (unsigned)block[ 2 ]
#define EDID_H_BLANKING_LO      (unsigned)block[ 3 ]
#define EDID_H_ACTIVE_HI        EDID_UPPER_NIBBLE( (unsigned)block[ 4 ] )
#define EDID_H_ACTIVE           EDID_COMBINE_HI_8LO( EDID_H_ACTIVE_HI, EDID_H_ACTIVE_LO )
#define EDID_H_BLANKING_HI      EDID_LOWER_NIBBLE( (unsigned)block[ 4 ] )
#define EDID_H_BLANKING         EDID_COMBINE_HI_8LO( EDID_H_BLANKING_HI, EDID_H_BLANKING_LO )

#define EDID_V_ACTIVE_LO        (unsigned)block[ 5 ]
#define EDID_V_BLANKING_LO      (unsigned)block[ 6 ]
#define EDID_V_ACTIVE_HI        EDID_UPPER_NIBBLE( (unsigned)block[ 7 ] )
#define EDID_V_ACTIVE           EDID_COMBINE_HI_8LO( EDID_V_ACTIVE_HI, EDID_V_ACTIVE_LO )
#define EDID_V_BLANKING_HI      EDID_LOWER_NIBBLE( (unsigned)block[ 7 ] )
#define EDID_V_BLANKING         EDID_COMBINE_HI_8LO( EDID_V_BLANKING_HI, EDID_V_BLANKING_LO )

int edid_get_detailed_timing(unsigned char *block
	                         ,EDID_Support_setting_t *std_tm)
{
	if ((block[0] == 0x00 && block[1] == 0x00)) /*it mean monitor description*/
	{
		//printk("monitor description\n");
		return EDID_FAIL;
	}

	std_tm->xres = EDID_H_ACTIVE;
	std_tm->yres = EDID_V_ACTIVE;

	std_tm->refresh = EDID_PIXEL_CLOCK/((EDID_H_ACTIVE + EDID_H_BLANKING) *
				     (EDID_V_ACTIVE + EDID_V_BLANKING));

	//printk(" %s H_HI=%2x H_LO=%2x V_HI=%2x V_LO=%2x\n",__func__ ,EDID_H_ACTIVE_HI,EDID_H_ACTIVE_LO,EDID_V_ACTIVE_HI,EDID_V_ACTIVE_LO);
	//printk(" %s xres=%d  yres=%d refresh=%d \n",__func__ ,std_tm->xres,std_tm->yres,std_tm->refresh);
    /*if refresh is 59 that is 60 HZ*/
	if(std_tm->refresh == 59)
        std_tm->refresh = 60;
	return EDID_SUCCESS;
}

int edid_get_std_timing(unsigned char *block,EDID_Support_setting_t *std_tm)
{
	int xres, yres = 0, refresh, ratio;
	int ver, rev;

	ver = EDID_Data[EDID_STRUCT_VERSION];
	rev = EDID_Data[EDID_STRUCT_REVISION];

	xres = (block[0] + 31) * 8;
	if (xres <= 256)
	{
		//printk("xres <= 256\n");
		return EDID_FAIL;
	}

	//printk("block[0]=%2x block[1]=%2x\n",block[0],block[1]);
	ratio = (block[1] & 0xc0) >> 6;
	switch (ratio) {
	case 0:
		/* in EDID 1.3 the meaning of 0 changed to 16:10 (prior 1:1) */
		if (ver < 1 || (ver == 1 && rev < 3))
			yres = xres;
		else
			yres = (xres * 10)/16;
		break;
	case 1:
		yres = (xres * 3)/4;
		break;
	case 2:
		yres = (xres * 4)/5;
		break;
	case 3:
		yres = (xres * 9)/16;
		break;
	}
	refresh = (block[1] & 0x3f) + 60;

	std_tm->xres = xres;
	std_tm->yres = yres;
	std_tm->refresh   = refresh;

	//printk("  %s  %dx%d@%dHz\n",__func__ ,std_tm->xres, std_tm->yres, std_tm->refresh);
	return EDID_SUCCESS;
}
int edid_get_ext_cea_timing(unsigned char *star_block,unsigned int len)
{
    unsigned char       cea_mode_index;
    unsigned char*      modes = 0;
    int                 i;

    for (modes = star_block; modes < star_block + len; modes++) {
        cea_mode_index = (*modes & 127) - 1; /* CEA modes are numbered 1..127 */
        if (cea_mode_index <=0 || cea_mode_index >= EDID_SUPPORT_CEA81_MODE_NUM)
            continue;

        //printk(" %s xres=%d  yres=%d refresh=%d \n",__func__ ,edid_cea81_mode_array[cea_mode_index].xres,edid_cea81_mode_array[cea_mode_index].yres,edid_cea81_mode_array[cea_mode_index].refresh);

        for(i = 0 ; i < EDID_SUPPORT_TIMING_NUM ; i++) {
		    if(edid_cea81_mode_array[cea_mode_index].xres == edid_support_res_array[i].xres &&
			   edid_cea81_mode_array[cea_mode_index].yres == edid_support_res_array[i].yres &&
			   edid_cea81_mode_array[cea_mode_index].refresh == 60 && edid_cea81_mode_array[cea_mode_index].interlace_flag == 0){

                edid_support_res_array[i].active = 1;

            }
        }
    }

	return EDID_SUCCESS;
}


int edid_check_block0_header(void)
{
    int i;
    /*check edid header*/
    for(i = 0 ; i <= EDID_HEADER_END ; i++) {
        if (EDID_Data[EDID_HEADER + i] != edid_v1x_header[i])
            return -1;
    }
    return 0;
}
int edid_parse_established_timer(int do_ext,unsigned int block_num)
{
	int i = 0 , j = 0 ,n = 0,hdmi_id = 0,is_hdmi=0,b_n;
	unsigned char edid_23c,edid_24c;
	EDID_Support_setting_t edid_std_tm;
	unsigned char *pars_block,*db,dbl;

    if(block_num > 7)
        return EDID_FAIL;

	/*check edid header*/
	for(i = 0 ; i <= EDID_HEADER_END ; i++) {
		if (EDID_Data[EDID_HEADER + i] != edid_v1x_header[i])
			goto edid_err;
	}

	/*parse edid 23h~25h only parse 60hz*/
	/*edid 23h*/

	edid_23c = EDID_Data[ESTABLISHED_TIMING_1];
	edid_24c = EDID_Data[ESTABLISHED_TIMING_2];

	for(i = 0 ; i < EDID_SUPPORT_TIMING_NUM ; i++) {

		if(edid_support_res_array[i].xres == 640 &&
	   	   edid_support_res_array[i].yres == 480 && (edid_23c&0x20))
	   		edid_support_res_array[i].active = 1;

		if(edid_support_res_array[i].xres == 800 &&
	       edid_support_res_array[i].yres == 600 && (edid_23c&0x01))
	    	edid_support_res_array[i].active = 1;

	    if(edid_support_res_array[i].xres == 1024 &&
	       edid_support_res_array[i].yres ==  768 &&(edid_24c&0x08))
	        edid_support_res_array[i].active = 1;

	}

	/*parse std timming*/
	pars_block = EDID_Data + ESTABLISHED_STANDARD_TIMING;
	for(j = 0 ; j < ESTABLISHED_STANDARD_TIMING_NUM ; j++ , pars_block+=ESTABLISHED_STANDARD_TIMING_SIZE) {
		if((edid_get_std_timing(pars_block,&edid_std_tm)) != EDID_SUCCESS)
			continue;
		for(i = 0 ; i < EDID_SUPPORT_TIMING_NUM ; i++) {
		    if(edid_std_tm.xres == edid_support_res_array[i].xres &&
		   	   edid_std_tm.yres == edid_support_res_array[i].yres &&
		       edid_std_tm.refresh == 60)
		        edid_support_res_array[i].active = 1;
		}
	}

	/*parse detail timming*/
	pars_block = EDID_Data + DETAILED_TIMING_DESCRIPTIONS_START;
	for (j = 0; j < DETAILED_TIMING_DESCRIPTIONS_NUM; j++, pars_block+= DETAILED_TIMING_DESCRIPTION_SIZE) {
		if((edid_get_detailed_timing(pars_block,&edid_std_tm)) != EDID_SUCCESS)
			continue;

		for(i = 0 ; i < EDID_SUPPORT_TIMING_NUM ; i++) {
			if(edid_std_tm.xres == edid_support_res_array[i].xres &&
		   	   edid_std_tm.yres == edid_support_res_array[i].yres &&
		       edid_std_tm.refresh == 60)
			   edid_support_res_array[i].active = 1;
		}
	}

	/*parse EDID extention for short vedio description*/
	if(!do_ext)
		goto edid_finish;


    for(b_n = 1 ; b_n <= block_num ; b_n ++ ){

        pars_block = EDID_Data + (EDID_EXT_BEGINE*b_n);

        if (!(pars_block[0] == 0x02 && pars_block[1] >= 3)){
            continue;//goto edid_finish;
        }

        for (db = pars_block + 4; db < (pars_block + pars_block[2]); db += dbl + 1) {
        	/*bit 4..0: Total number of bytes in this block*/
        	dbl = db[0] & 0x1f;
        	/*Parse HDMI identified*/
            if (((db[0] & 0xe0) >> 5) ==  EDID_EXT_VENDOR_BLOCK) {
             	hdmi_id = db[1] | (db[2] << 8) |db[3] << 16;
        		/* Find HDMI identifier */
        		if (hdmi_id == EDID_EXT_HDMI_IDENTIFIER)
        			is_hdmi = 1;
        	}
        	/*bit 7..5: Block Type Tag (1 is audio, 2 is video, 3 is vendor specific, 4 is
        	  speaker allocation, all other values Reserved)*/
        	if (((db[0] & 0xe0) >> 5) == EDID_EXT_VIDEO_BLOCK) {
                edid_get_ext_cea_timing(db+1,dbl);
        	}
        }

        /*parse ext CEA detail timing*/
        pars_block = EDID_Data + (EDID_EXT_BEGINE*b_n);
        dbl = pars_block[0x02];

        /*move to cea detail data*/
        pars_block += dbl;

        n = (127 - dbl) / 18;
        for (j = 0; j < n; j++, pars_block+= DETAILED_TIMING_DESCRIPTION_SIZE) {
        	if((edid_get_detailed_timing(pars_block,&edid_std_tm)) != EDID_SUCCESS)
        		continue;

        	for(i = 0 ; i < EDID_SUPPORT_TIMING_NUM ; i++) {
        		if(edid_std_tm.xres == edid_support_res_array[i].xres &&
        		   edid_std_tm.yres == edid_support_res_array[i].yres &&
        		   edid_std_tm.refresh == 60)
        			edid_support_res_array[i].active = 1;
        	}
        }


        if(is_hdmi){
            //printk("support HDMI device\n");
            for(i = 0 ; edid_support_res_array[i].vid != -1 ; i++) {
                if(edid_support_res_array[i].active == 1)
        	        edid_support_res_array[i].is_hdmi = 1;
        	}
        }

    }
edid_finish:

	/*for(i = 0 ; i < EDID_SUPPORT_TIMING_NUM ; i++){
		if(edid_support_res_array[i].active == 1)
			printk("vid=%d is active , is_hdmi=%d\n",edid_support_res_array[i].vid,edid_support_res_array[i].is_hdmi);
	}*/

	return EDID_SUCCESS ;

edid_err:
	return EDID_FAIL;
}

