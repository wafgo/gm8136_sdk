//-----------------------------------------------------------------------------
// USB_MAIN.c
//-----------------------------------------------------------------------------
// Copyright 2007 Silicon Library USA, Inc.
//
// AUTH: TT
// DATE: August 1, 2007
//
//
// Target: C8051F34x
// Tool chain: KEIL C51 6.03 / KEIL EVAL C51
//
// REVISIONS:
// 10/24/07 - TT: Created step by step version
//
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <c8051f340.h>
#include <stddef.h>
#include "USB_API.h"
#include "I2C_header.h"
#include "HDMI_header.h"
#include "HDMI_TxRegs.h"


#define FW_VER      28


//TEMPORARY EXTERN
extern BYTE RE_SETTING;
extern BYTE HDCP_ENABLED;
extern BYTE HDCP_BLACK;
#ifdef SLI11131
extern BYTE IP_ON;
#endif  // SLI11131
extern BYTE xdata EDID_Data[128 * 8];
extern HDMI_Setting_t HDMI_setting;
extern BYTE CtrlPkt_Enable ;
extern BYTE CtrlPkt_Auot;
extern BYTE CtrlPkt_Manual;
extern BYTE PowerMode;

//-----------------------------------------------------------------------------
// 16-bit SFR Definitions for 'F32x
//-----------------------------------------------------------------------------

sfr16 TMR2RL   = 0xca;                   // Timer2 reload value
sfr16 TMR2     = 0xcc;                   // Timer2 counter
sfr16 ADC0     = 0xbe;

//-----------------------------------------------------------------------------
// Global CONSTANTS / VARIABLES
//-----------------------------------------------------------------------------


sbit Led1 = P2^0;                         // LED='1' means ON
sbit Led2 = P2^1;                         //
sbit Led3 = P2^2;                         //
sbit Led4 = P2^3;                         //
sbit Led5 = P2^4;                         //
sbit Led6 = P2^5;                         //
sbit Led7 = P2^6;                         //

#define Sw1 0x04                          // These are the port0 bits for Sw1
#define Sw2 0x08                          // and Sw2 on the development board
#define Sw3 0x01                          // These are the port1 bits for Sw3
#define Sw4 0x02                          // and Sw4 on the development board
BYTE Switch1State = 0;                    // Indicate status of switch
BYTE Switch2State = 0;                    // starting at 0 == off
BYTE Switch3State = 0;                    //
BYTE Switch4State = 0;                    //

BYTE Toggle1 = 0;                         // Variable to make sure each button
BYTE Toggle2 = 0;                         // press and release toggles switch
BYTE Toggle3 = 0;                         //
BYTE Toggle4 = 0;                         //

BYTE COMP_MODE = 0;
BYTE USB_ONLY_MODE = 0;

BYTE USB_CMD_Received = 0;
BYTE USB_Packet[16];                      // Last packet received from host
// Out_Packet (Host -> this), In_Packet (this -> Host) index
#define PKT_Code                USB_Packet[0]       // In & Out
#define PKT_DevAddr             USB_Packet[1]       // In & Out
#define PKT_VidFormat           USB_Packet[1]       // Mode
#define PTK_HDCP_Enabled        USB_Packet[1]       // HDCP
#define PKT_CtrlPkt_Enable      USB_Packet[1]       // CONTROL PACKET
#define PKT_RegAddr             USB_Packet[2]       // In & Out
#define PKT_AudFreq             USB_Packet[2]       // Mode
#define PKT_CtrlPkt_SND_AUOT    USB_Packet[2]       // CONTROL PACKET

#ifdef SLI11131
#define PKT_GAMMA               USB_Packet[2]       // Gamma Correctio
#define PKT_IP_COV_CONT         USB_Packet[2]       // I/P conversion
#endif //SLI11131

#define PKT_RegValue            USB_Packet[3]       // In & Out
#define PKT_AudCh               USB_Packet[3]       // Mode
//#define PKT_LED               USB_Packet[4]       // In & Out
#define PKT_AudSrc              USB_Packet[4]       // Mode
//#define PKT_Status            USB_Packet[5]       // In
#define PKT_HDCP                USB_Packet[5]       // Mode
//#define PKT_INT0              USB_Packet[6]       // In
#define PKT_DeepColor           USB_Packet[6]       // Mode
//#define PKT_INT1              USB_Packet[7]       // In
#define PKT_DownSampling        USB_Packet[7]       // Mode



/*** [BEGIN] USB Descriptor Information [BEGIN] ***/
code const UINT USB_VID = 0x10C4;
code const UINT USB_PID = 0xEA61;
code const BYTE USB_MfrStr[] = {0x1A,0x03,'S',0,'i',0,'l',0,'i',0,'c',0,'o',0,'n',0,' ',0,'L',0,'a',0,'b',0,'s',0};                       // Manufacturer String
code const BYTE USB_ProductStr[] = {0x10,0x03,'U',0,'S',0,'B',0,' ',0,'A',0,'P',0,'I',0}; // Product Desc. String
code const BYTE USB_SerialStr[] = {0x0A,0x03,'1',0,'2',0,'3',0,'4',0};
code const BYTE USB_MaxPower = 15;            // Max current = 30 mA (15 * 2)
code const BYTE USB_PwAttributes = 0x80;      // Bus-powered, remote wakeup not supported
code const UINT USB_bcdDevice = 0x0100;       // Device release number 1.00
/*** [ END ] USB Descriptor Information [ END ] ***/

// Function Prototypes
void Timer_Init(void);                       // Start timer 2 for use by ADC and to check switches
void Port_Init(void);
void Suspend_Device(void);
void Initialize(void);
void Timer2_ISR(void);

unsigned char Process_I2C_request(void);
//unsigned char Monitor_CompSwitch(void);

#ifdef MonM
extern BYTE USB_MCU_MON  = 1;       // USB_Monitor ... 1, No Monitor ... 0;
#endif

BYTE WD_TIMER_CNT = 0;


//-----------------------------------------------------------------------------
// Main Routine
//-----------------------------------------------------------------------------
void main (void)
{
   PCA0MD &= ~0x40;                       // Disable Watchdog timer

   USB_Clock_Start();                     // Init USB clock *before* calling USB_Init
   USB_Init(USB_VID,USB_PID,USB_MfrStr,USB_ProductStr,USB_SerialStr,USB_MaxPower,USB_PwAttributes,USB_bcdDevice);

   Initialize();
   USB_Int_Enable();




   COMP_MODE = P1 & 0x10;       // SW1 Compliance mode: FPGA video/audio input needed
   USB_ONLY_MODE = P1 & 0x20;   // SW2 USB mode: no state machine, no hotplug detection
   HDCP_ENABLED = P1 & 0x01;    // SW3 HDCP mode
   HDCP_BLACK = P1 & 0x02;      // SW4 Black screen while HDCP not authenticated


   if (!USB_ONLY_MODE)
   {
       //if (I2C_register_test())
       //{
       //       Led6 = 0;
       //       Led7 = 0;//error turn off 2 LEDs
       //}
        HDMI_init();
   }

   while (1)
   {

        Process_I2C_request();

        // Allow dynamic change of SW2 USB mode switch.
        USB_ONLY_MODE = P1 & 0x20;

        if (!USB_ONLY_MODE)
        {
            HDMI_ISR_bottom();
        }
   }
}

//-----------------------------------------------------------------------------
// Initialization Subroutines
//-----------------------------------------------------------------------------

//-------------------------
// Port_Init
//-------------------------
// Port Initialization
// - Configure the Crossbar and GPIO ports.
//
// P0.0   digital   open-drain    SMBus SDA
// P0.1   digital   open-drain    SMBus SCL
// P0.6   digital   open-drain    External Interrupt (skip crossbar)
//
// P2.2   digital   push-pull     LED
// P2.3   digital   push-pull     LED
// P2.5   analog                  Potentiometer
//
// Note: If the SMBus is moved, the SCL and SDA sbit declarations must also
// be adjusted.
//
void Port_Init (void)
{

// SILICON LAB BOARD SETTING
/*
   P2MDIN   = 0xDF;                        // Port 2 pin 5 set as analog input
   P0MDOUT  = 0x00;                        // Port 0 pins 0-3 set open-drain output (SMBus)
   P0SKIP   = 0x40;                        // Port 0 pin 6 skipped by crossbar (External INterrupt)
   P1MDOUT |= 0x0F;                        // Port 1 pins 0-3 set high impedence
   P2MDOUT |= 0x0C;                        // Port 2 pins 0,1 set high impedence
   P2SKIP   = 0x20;                        // Port 1 pin 7 skipped by crossbar
   XBR0     = 0x04;                        // Enable SMBus pins
   XBR1     = 0x40;                        // Enable Crossbar

   //P0 = 0xFF;

   IT01CF   = 0x06;                        // INT0 to P0.6, active low
   IT0      = 0;                           // INT0 is level sensitive
   EX0 = 1;//IE       |= 0x01;                        // Enable INT0 external interrupt
   //IP |= 0x01;
*/
    P0MDOUT   = 0x0F;
    P1MDOUT   = 0xE7;
    P2MDOUT   = 0xFF;
    P3MDOUT   = 0x80;
    P0SKIP    = 0x0F;
    P1SKIP    = 0xE7;
    P2SKIP    = 0xFF;
    P3SKIP    = 0xFF;//FF;
    XBR0      = 0x05;
    XBR1      = 0x58;

   IT01CF   = 0x00;                        // INT0 to P0.0, active low
   IT0      = 0;                           // INT0 is level sensitive
   EX0 = 1;//IE       |= 0x01;                        // Enable INT0 external interrupt

}

//-------------------------
// Timer_Init
//-------------------------
// Timer initialization
// - 1 mhz timer 2 reload, used to check if switch pressed on overflow and
// used for ADC continuous conversion
//
void Timer_Init (void)
{
   TMR2CN  = 0x00;                        // Stop Timer2; Clear TF2;

   CKCON  &= ~0xF0;                       // Timer2 clocked based on T2XCLK;
   TMR2RL  = -(24000000 / 12);            // Initialize reload value     // 1s overflow
   TMR2    = 0xffff;                      // Set to reload immediately

   ET2     = 1;                           // Enable Timer2 interrupts
   TR2     = 1;                           // Start Timer2
}


//-------------------------
// Initialize
//-------------------------
// Called when a DEV_CONFIGURED interrupt is received.
// - Enables all peripherals needed for the application
//
void Initialize (void)
{
   Port_Init();                           // Initialize crossbar and GPIO
   Timer_Init();                          // Initialize timer2
   Timer1_Init();                         // Initialize timer1
   Timer3_Init();                         // Initialize timer3
   SMBus_Init();

}


//-------------------------
// Timer2_ISR
//-------------------------
// Called when timer 2 overflows, check to see if switch is pressed,
// then watch for release.
//
void Timer2_ISR (void) interrupt 5
{

   WD_TIMER_CNT=WD_TIMER_CNT+1;
   // debug Led5
   //Led5 = ~Led5;

   // 2009.06.16 Because Time2 are unused, use is changed to HDMI Status of WD-timer.
   //if (!(P0 & Sw1))                      // Check for switch #1 pressed
   //{
   //   if (Toggle1 == 0)                   // Toggle is used to debounce switch
   //   {                                   // so that one press and release will
   //      Switch1State = ~Switch1State;    // toggle the state of the switch sent
   //      Toggle1 = 1;                     // to the host
   //   }
   //}
   //else Toggle1 = 0;                      // Reset toggle variable
   //
   //if (!(P0 & Sw2))                       // This is the same as above, but for Switch2
   //{
   //   if (Toggle2 == 0)
   //   {
   //      Switch2State = ~Switch2State;
   //      Toggle2 = 1;
   //   }
   //}
   //else Toggle2 = 0;
   //
   //if (!(P1 & Sw3))                       // This is the same as above, but for Switch3
   //{
   //   if (Toggle3 == 0)
   //   {
   //      Switch3State = ~Switch3State;
   //      Toggle3 = 1;
   //   }
   //}
   //else Toggle3 = 0;
   //
   //if (!(P1 & Sw4))                       // This is the same as above, but for Switch4
   //{
   //   if (Toggle4 == 0)
   //   {
   //      Switch4State = ~Switch4State;
   //      Toggle4 = 1;
   //   }
   //}
   //else Toggle4 = 0;
   //

   TF2H = 0;                              // Clear Timer2 interrupt flag
}

// Example ISR for USB_API
void  USB_API_TEST_ISR (void) interrupt 17
{
   BYTE INTVAL = Get_Interrupt_Source();

   if (INTVAL & RX_COMPLETE)
   {
      USB_Int_Disable();
      Block_Read(USB_Packet, 8);
      USB_CMD_Received = 1;
   }

   if (INTVAL & DEV_SUSPEND)
   {
        //Suspend_Device();
   }

   if (INTVAL & DEV_CONFIGURED)
   {
      // Initialize();
   }

}

//-----------------------------------------------------------------------------
// Process_I2C_request()
//-----------------------------------------------------------------------------
//
// Return Value :
//
// Parameters   :
//
// This function read and/or write registers via I2C when there is a read or
// write request sent via USB. Prepares data to send back to the host.
//
unsigned char Process_I2C_request (void)
{
    if (USB_CMD_Received) {

        switch (PKT_Code)
        {
            //
            // REGISTER WRITE
            case 'w':
            case 'W':
                if (PKT_RegAddr == 0x00)
                    PowerMode = PKT_RegValue;
                I2C_ByteWrite (PKT_RegAddr, PKT_RegValue);
                Block_Write(USB_Packet, 8);
                break;

            // REGISTER READ
            case 'r':
            case 'R':
                PKT_RegValue = I2C_ByteRead (PKT_RegAddr);
                Block_Write(USB_Packet, 8);
                break;

#ifdef SLI11131
            // I/P conversion REGISTER WRITE
            case 'x':
            case 'X':
                I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
                    | IP_REG_OFFSET );              // Set offset bit to access IP registers
                I2C_ByteWrite (PKT_RegAddr, PKT_RegValue);
                Block_Write(USB_Packet, 8);
                I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
                     & ~IP_REG_OFFSET);             // Reset offset bit to access normal registers
                break;

            // I/P conversion REGISTER READ
            case 'y':
            case 'Y':
                I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
                    | IP_REG_OFFSET );              // Set offset bit to access IP registers
                PKT_RegValue = I2C_ByteRead (PKT_RegAddr);
                Block_Write(USB_Packet, 8);
                I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
                     & ~IP_REG_OFFSET);             // Reset offset bit to access normal registers
                break;
#endif      // SLI11131

            // SET MANUAL MODE
            case 'm':
            case 'M':
                switch (PowerMode)
                {
                    case PowerMode_D:
                        if (PKT_VidFormat && !COMP_MODE)
                        {
                            HDMI_setting.vid = PKT_VidFormat;
                            TI_apply_setting (PKT_VidFormat);
                            RE_SETTING = 0;
                        }
                        Block_Write(USB_Packet, 8); // doesn't have any status for now...
                        break;
                    case PowerMode_E:
                        if(RE_SETTING)
                            break;
                        if (PKT_VidFormat)
                        {
                            HDMI_setting.vid = PKT_VidFormat;
//                          HDMI_Video_set_format (PKT_VidFormat);
                            RE_SETTING = 1;
                        }
                        if (PKT_AudFreq)
                        {
                            HDMI_setting.audio_freq    = PKT_AudFreq;
//                          HDMI_Audio_set_freq (PKT_AudFreq);
                            RE_SETTING = 1;
                        }
                        if (PKT_AudCh)
                        {
                            HDMI_setting.audio_ch      = PKT_AudCh;
//                          HDMI_Audio_set_channel (PKT_AudCh);
                            RE_SETTING = 1;
                        }
                        if (PKT_AudSrc)
                        {
                            HDMI_setting.audio_source = PKT_AudSrc;
//                          if (PKT_AudSrc == AUD_SPDIF)
//                              HDMI_setting.audio_source = 1;
//                          else
//                              HDMI_setting.audio_source = 0;
//                          HDMI_Audio_SPDIF (HDMI_setting.audio_source);
                            RE_SETTING = 1;
                        }
                        if (PKT_DeepColor)
                        {
                            HDMI_setting.deep_color = PKT_DeepColor;
                            RE_SETTING = 1;
                        }
                        if (PKT_DownSampling)
                        {
                            HDMI_setting.down_sampling = PKT_DownSampling;
                            if( HDMI_setting.audio_freq == AUD_96K )
                            {
                                 if( HDMI_setting.down_sampling == DS_2 )
                                      HDMI_setting.audio_freq = AUD_48K;
                                 else
                                      HDMI_setting.down_sampling = DS_none;
                            }
                            else if( HDMI_setting.audio_freq == AUD_192K )
                            {
                                 if( HDMI_setting.down_sampling == DS_2 )
                                      HDMI_setting.audio_freq = AUD_96K;
                                 else if( HDMI_setting.down_sampling == DS_4 )
                                      HDMI_setting.audio_freq = AUD_48K;
                            }
                            else
                                 HDMI_setting.down_sampling = DS_none;
                            RE_SETTING = 1;
                        }
                        if (PKT_HDCP == 0x80)
                            HDMI_HDCP_disable();
                        else if (PKT_HDCP == 0x81)
                            HDMI_HDCP_enable();

                        if(RE_SETTING || PKT_HDCP)
                            Block_Write(USB_Packet, 8); // doesn't have any status for now...
                        break;
                    default:
                        break;
                }
                break;

            // SET AUTO MODE
            case 'a':
            case 'A':
                // FIXME: Change to video / audio setting in EDID
                Block_Write(USB_Packet, 8); // doesn't have any status for now...
                break;

            // GET STATUS
            case 's':
            case 'S':

#ifndef MonM
                USB_Packet[0] = FW_VER;             // FW version
                Block_Write(USB_Packet, 8); // doesn't have any status for now...
#else
                if (USB_Packet[1] == 0 ){
                    USB_Packet[0] = FW_VER;             // FW version
                    Block_Write(USB_Packet, 8); // doesn't have any status for now...
                }
                else{
                    USB_MCU_MON=USB_Packet[1];
                }
#endif
                break;
            // HDCP
            case 'h':
            case 'H':
                switch (PowerMode)
                {
                    case PowerMode_E:
                        if(RE_SETTING)
                            break;
                        else
                        {
                            HDCP_ENABLED = PTK_HDCP_Enabled;
                            RE_SETTING = 1;
                            Block_Write(USB_Packet, 8); // doesn't have any status for now...
                        }
                        break;
                    default:
                        break;
                }
            // GET EDID
            case 'e':
            case 'E':           // Set EDID word address (set to 00h for the first 128 bytes)
                // For now always return the entire EDID data buffer
                Block_Write(EDID_Data, 128 * 8);
                break;

            // CONTROL PACKET
            case 'c':
            case 'C':
                // Control Packet bit enabled after every VSYNC
                if(PKT_CtrlPkt_Enable)
                {
                    CtrlPkt_Enable = (CtrlPkt_Enable & 0x01) | (PKT_CtrlPkt_Enable & ~0x01);
                    CtrlPkt_Auot = 0;
                    CtrlPkt_Manual = 1;
                }
                else
                {
                    CtrlPkt_Enable = (CtrlPkt_Enable & 0x01) | 0x6E;
                    CtrlPkt_Auot = 1;
                    CtrlPkt_Manual = 0;
                }
                switch (PowerMode)
                {
                    case PowerMode_E:
                        HDMI_control_packet_auto_Send (CtrlPkt_Auot,CtrlPkt_Enable);
                        break;
                    default:
                        break;
                }
                Block_Write(USB_Packet, 8); // doesn't have any status for now...
                break;

#ifdef SLI11131
            // IP Conversion
            case 'i':
            case 'I':
                switch (PowerMode)
                {
                    case PowerMode_E:
                        if(RE_SETTING)
                            break;
                        else
                        {
                            IP_ON = PKT_IP_COV_CONT;
                            RE_SETTING = 1;
                            Block_Write(USB_Packet, 8); // doesn't have any status for now...
                        }
                        break;
                    default:
                        break;
                }
                break;
            //  Gamma correction control
            case 'g':
            case 'G':
                if(PKT_GAMMA)
                    I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
                        & ~STGAM_OFF);
                else
                    I2C_ByteWrite (XFF_IP_COM_CONTROL, I2C_ByteRead (XFF_IP_COM_CONTROL)
                        | STGAM_OFF);
                Block_Write(USB_Packet, 8); // doesn't have any status for now...
                break;
#endif   // SLI11131

            // DEBUG COMMAND
            case 'd':
            case 'D':
            /*
                I2C_ByteWrite (0xC4, 0); // seg pointer
                {
                    int x = 0;
                    while (x++ < 500);
                }
                I2C_ReadArray (debug_Data, 0x80, 8); // EDID buffer */
                USB_Packet[0] = EDID_Data[0];
                USB_Packet[1] = EDID_Data[1];
                USB_Packet[2] = EDID_Data[2];
                USB_Packet[3] = EDID_Data[3];
                USB_Packet[4] = EDID_Data[4];
                USB_Packet[5] = EDID_Data[5];
                USB_Packet[6] = EDID_Data[6];
                USB_Packet[7] = EDID_Data[7];
                Block_Write(USB_Packet, 8);
                break;

            default:
                break;
        }

        // Clear USB Command Packet flag
        // re-enable USB interrupt to process next packet
        USB_CMD_Received = 0;
        USB_Int_Enable();
    }

    return 0;
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
    static BYTE P3_saved = 0xFF;
    static BYTE P4_saved = 0xFF;

    unsigned char vid, audio, ch, color, ds, spdif;


    // SWITCH CHANGED - NEW SETTING

    P3_saved = P3;// & 0x7F;
    P4_saved = P4;// & 0x0F;

    // read video format
    switch (P3_saved & 0x0F)
    {
        case 0:
            vid = VID_06_720x480i;
            break;
        //case 1:   break;
        case 2:
            vid = VID_02_720x480p;
            break;
        case 3:
            vid = VID_04_1280x720p;
            break;
        case 4:
            vid = VID_05_1920x1080i;
            break;
        //case 5:   break;
        case 6:
            vid = VID_16_1920x1080p;
            break;
        case 7:
            vid = VID_32_1920x1080p;
            break;
        case 8:
            vid = VID_21_720x576i;
            break;
        //case 9:break;
        case 10:
            vid = VID_17_720x576p;
            break;
        case 11:
            vid = VID_19_1280x720p;
            break;
        case 12:
            vid = VID_20_1920x1080i;
            break;
        //case 13:break;
        case 14:
            vid = VID_31_1920x1080p;
            break;
        //case 15:break;
        default:
            vid = VID_02_720x480p;
            break;
    }

    // read audio frequency
    switch (P3_saved & 0x30)
    {
        case 0x00:
            audio = AUD_48K;
            break;
        case 0x10:
            audio = AUD_96K;
            break;
        case 0x20:
            audio = AUD_192K;
            break;
        //case 3:break;
        default:
            audio = AUD_48K;
            break;
    }

    // read audio channel
    if (P3_saved & 0x40)
        ch = AUD_8CH;
    else
        ch = AUD_2CH;

    // read deep color setting
//  color = P4_saved & 0x03;

    switch (P4_saved & 0x03)
    {
        case 0x00:
            color = DEEP_COLOR_8BIT;
            break;
        case 0x01:
            color = DEEP_COLOR_10BIT;
            break;
        case 0x02:
            color = DEEP_COLOR_12BIT;
            break;
        default:
            break;
    }
    // read downsampling setting
    //ds = P4_saved & 0x0C;
    switch (P4_saved & 0x0C)
    {
        case 0x00:
            ds = DS_none;
            break;
        case 0x04:
            ds = DS_none;
            if (audio == AUD_96K)
			{
                 ds = DS_2;
                 audio=AUD_48K;
            }
            else if (audio == AUD_192K)
			{
                 ds = DS_2;
                 audio=AUD_96K;
            }
			break;
        case 0x08:
 	        ds = DS_none;
            if (audio == AUD_192K)
			{
                 ds = DS_4;
                 audio=AUD_48K;
            }
            break;
        default:
            ds = DS_none;
            break;
    }
    // override audio setting when downsampling enable
    //if (ds != 0x00 && audio == AUD_48K ) ds = DS_none;
    //if (ds == 0x00) ds = DS_none;
    //else if (ds == 0x04) ds = DS_2;
    //else if (ds == 0x08) ds = DS_4;
    //else ds = DS_none;

    // SPDIF setting (P4 bit 5)
    switch (P4_saved & 0x20)
    {
        case 0x00:
            spdif = AUD_I2S;
            break;
        case 0x20:
            spdif = AUD_SPDIF;
            break;
        default:
            spdif = AUD_SPDIF;
            break;
    }
    //spdif = P4_saved & 0x20;

    // Set to Global
    HDMI_setting.vid           = vid;
    HDMI_setting.audio_freq    = audio;
    HDMI_setting.audio_ch      = ch;
    HDMI_setting.deep_color    = color;
    HDMI_setting.down_sampling = ds;
    HDMI_setting.audio_source  = spdif;

    return 0;
}


//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
