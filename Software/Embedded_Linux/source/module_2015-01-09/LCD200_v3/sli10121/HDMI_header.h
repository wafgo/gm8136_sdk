#ifndef  _HDMI_HEADER_H_
#define  _HDMI_HEADER_H_


//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

#define SLI10121            //SLI10121/121A
//#define SLI11131            //SLI11131
//#define T65TX               //T65 (Internal AU_PLL)
//#define T45TX                 //Process T45 (Rosemary IP Ver.2.11)


#ifdef KENNY_CHANGE

typedef struct sli10121_ext_frame_timingTag {
    int vid_idx;
    int pol_vsy;
    int pos_hsy;
    int hor_sync_time;
    int hor_back_porch;
    int hor_active_video;
    int hor_front_porch;
    int ver_sync_time;
    int ver_back_porch;
    int ver_active_video;
    int ver_front_porch;
} sli10121_ext_frame_timing;

#endif /* KENNY_CHANGE */

// Video setting constants
#define VID_01_640x480p     1   /* pre-programmed VID, Progressive, 25Mhz, 60Hz */
#define VID_02_720x480p     2   /* pre-programmed VID, Progressive, 27Mhz, 60Hz */
#define VID_03_720x480p     3
#define VID_04_1280x720p    4   /* pre-programmed VID, Progressive, 74.25Mhz, 60Hz */
#define VID_05_1920x1080i   5   /* pre-programmed VID, Interlace, 74.25Mhz, 60Hz */
#define VID_06_720x480i     6   /* pre-programmed VID, Interlace, 27Mhz, 60Hz */
#define VID_07_720x480i     7
#define VID_08_720x240p     8
#define VID_09_720x240p     9
#define VID_10_2880x480i    10
#define VID_11_2880x480i    11
#define VID_12_2880x240p    12
#define VID_13_2880x240p    13
#define VID_14_1440x480p    14
#define VID_15_1440x480p    15
#define VID_16_1920x1080p   16  /* pre-programmed VID, Progressive, 148.5Mhz, 60Hz */
#define VID_17_720x576p     17  /* pre-programmed VID, Progressive, 27Mhz, 50Hz */
#define VID_18_720x576p     18
#define VID_19_1280x720p    19  /* pre-programmed VID, Progressive, 74.25Mhz, 50Hz */
#define VID_20_1920x1080i   20  /* pre-programmed VID, Interlace, 74.25Mhz, 50Hz */
#define VID_21_720x576i     21  /* pre-programmed VID, Interlace, 27Mhz, 50Hz */
#define VID_22_720x576i     22
#define VID_23_720x288p     23
#define VID_24_720x288p     24
#define VID_25_2880x576i    25
#define VID_26_2880x576i    26
#define VID_27_2880x288p    27
#define VID_28_2880x288p    28
#define VID_29_1440x576p    29
#define VID_30_1440x576p    30
#define VID_31_1920x1080p   31  /* pre-programmed VID, Progressive, 148.5Mhz, 50Hz */
#define VID_32_1920x1080p   32  /* pre-programmed VID, Progressive, 74.25Mhz, 24Hz */
#define VID_33_1920x1080p   33
#define VID_34_1920x1080p   34
#define VID_35_2880x480p    35
#define VID_36_2880x480p    36
#define VID_37_2880x576p    37
#define VID_38_2880x567p    38
#define VID_39_1920x1080i   39
#define VID_40_1920x1080i   40
#define VID_41_1280x720p    41
#define VID_42_720x576p     42
#define VID_43_720x576p     43
#define VID_44_720x576i     44
#define VID_45_720x576i     45
#define VID_46_1920x1080i   46
#define VID_47_1280x720p    47
#define VID_48_720x480p     48
#define VID_49_720x480p     49
#define VID_50_720x480i     50
#define VID_51_720x480i     51
#define VID_52_720x576p     52
#define VID_53_720x576p     53
#define VID_54_720x576i     54
#define VID_55_720x576i     55
#define VID_56_720x480p     56
#define VID_57_720x480p     57
#define VID_58_720x480i     58
#define VID_59_720x480i     59

#ifdef KENNY_CHANGE
#define VID_60_1024x768_RGB_EXT     60  /* 60 hz */
#define VID_61_1280x1024_RGB_EXT    61  /* 60 hz */
#define VID_63_1280x1024P_EXT       63  /* 60 hz */
#define VID_64_1024x768P_EXT        64  /* 60 hz */
#endif /* KENNY_CHANGE */

// Audio setting constants
#define AUD_32K         0x50    //32K
#define AUD_44K         0x60    //44.1K
#define AUD_48K         0x10
#define AUD_96K         0x20
#define AUD_192K        0x30
#define AUD_2CH         0x40    // 2ch audio
#define AUD_8CH         0x80    // 8ch audio
//#define DS_none           0x00    // No Downsampling
#define DS_none         0x01    // No Downsampling
#define DS_2            0x04    // Downsampling 96k to 48k
#define DS_4            0x08    // DOwnsampling 192k to 48k
#define AUD_SPDIF       0x01
#define AUD_I2S         0x02

// Power Mode - System Control
#define PowerMode_A             0x10
#define PowerMode_B             0x20
#define PowerMode_D             0x40
#define PowerMode_E             0x80

// Output Format
#define FORMAT_RGB              0
#define FORMAT_YCC422           1
#define FORMAT_YCC444           2

// Deep Color Bit,
// Should follow GCP_CD[3:0] definition
#define DEEP_COLOR_8BIT         4
#define DEEP_COLOR_10BIT        5
#define DEEP_COLOR_12BIT        6

// Register Values for Register 94h: Interrupt Status 1
#define INT1_RSVD               0x09
#define HOT_PLUG                0x80
#define HPG_MSENS               0xC0
#define EDID_ERR                0x02
#define EDID_RDY                0x04
#define VSYNC                   0x20

// Register Values for Register 95h: Interrupt Status 2
#define INT2_RSVD               0x07
#define HDCP_ERR                0x80
#define BKSV_RDY                0x60
#define HDCP_AUTH               0x08
#define HDCP_DONE               0x10

// STATE
#define HDMI_STATE_IDLE         0x01
#define HDMI_STATE_HOTPLUG      0x02
#define HDMI_STATE_EDID_START   0x03
#define HDMI_STATE_EDID_READY   0x04
#define HDMI_STATE_EDID_READ    0x05
#define HDMI_STATE_EDID_PROCESS 0x06
#define HDMI_STATE_TX_SETTING   0x07
#define HDMI_STATE_TX_START     0x08
#define HDMI_STATE_TX_RUNNING   0x09
#define HDMI_STATE_HDCP_START   0x0A
#define HDMI_STATE_HDCP_READY   0x0B
#define HDMI_STATE_HDCP_READ    0x0C
#define HDMI_STATE_HDCP_AUTH    0x0D
//#define HDMI_STATE_PHY_RESET    0x0E
#define HDMI_STATE_ERROR        0x00
#define STATE_DEBUG             0xFF


//-----------------------------------------------------------------------------
// Type Definition
//-----------------------------------------------------------------------------
typedef struct {
	int	vid;
	int	xres;
	int yres;
	int active;
	int refresh; /*only for parse*/
	unsigned int interlace_flag;
	char *descr;
    int is_hdmi;
} EDID_Support_setting_t;

typedef struct {
    BYTE vid; // video_format = VID_02_720x480p;
    BYTE audio_freq; // audio_freq = AUD_48K;
    BYTE audio_ch; // audio_ch = AUD_2CH;
    BYTE deep_color; // video_color = 0;
    BYTE down_sampling;// = 0;
    BYTE audio_source; //audio_spdif = 0;
    BYTE output_format; //ycc_rgb FORMAT_YCC422;
    BYTE dvi_mode;
} HDMI_Setting_t;

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------

// Function Prototypes for HDMI
void HDMI_init (void);
void HDMI_ISR_top (void);
void HDMI_apply_setting (void);
void HDMI_PHY_setup (void);
void HDMI_Video_set_format (unsigned char);
void HDMI_Audio_set_freq (unsigned char);
void HDMI_Audio_set_channel (unsigned char);
void HDMI_Audio_set_ds (unsigned char);
void HDMI_Video_set_color (unsigned char color);
void HDMI_Audio_SPDIF (unsigned char spdif);
void HDMI_Video_set_output ( void );
void HDMI_control_packet_auto_Send (BYTE, BYTE);
void HDMI_control_packet_manual_Send (BYTE *, BYTE);

void HDMI_System_PD (unsigned char mode);
void HDMI_HDCP_enable (void);
void HDMI_HDCP_disable (void);
void HDMI_EDID_read (void);
void HDMI_ISR_bottom (void);
BYTE HDMI_EDID_checksum (BYTE* array, BYTE size);
BYTE HDMI_hotplug_check (void);

void hdmi_tx_ip_conversion_enable (HDMI_Setting_t *);
void hdmi_tx_ip_conversion_disable (HDMI_Setting_t *);
#ifdef SLI11131
static void IP_Conversion_setting_480 (BYTE);
static void IP_Conversion_setting_576 (BYTE);
static void IP_Conversion_setting_1080 (BYTE);
#endif
void HDMI_clear_to_IDLE ( void );
void HDMI_HDCPclear_to_IDLE ( void );

void TI_apply_setting (unsigned char format);
void TI_board_setup_720p (void);
void TI_board_setup_480p (void);
void TI_board_setup_1080i (void);
void TI_board_setup_1080p (void);

void HDMI_PHY_setting_27 (void);
void HDMI_PHY_setting_33 (void);
void HDMI_PHY_setting_40 (void);
void HDMI_PHY_setting_74 (void);
void HDMI_PHY_setting_98 (void);
void HDMI_PHY_setting_111 (void);
void HDMI_PHY_setting_148 (void);
void HDMI_PHY_setting_185 (void);
void HDMI_PHY_setting_222 (void);

unsigned char Monitor_CompSwitch(void);
u32  Sli10121_Get_Output_Mode(void);
u32  sli10121_Get_YCC_Mode(void);
#endif  /* _HDMI_HEADER_H_ */
