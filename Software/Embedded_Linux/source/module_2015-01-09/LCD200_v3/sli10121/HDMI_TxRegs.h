#ifndef  _HDMI_TXREGS_H_
#define  _HDMI_TXREGS_H_

// SLI-204-06012 Rev.2.01a
//-----------------------------------------------------------------------------
// SLISHDMI13T Regsiter Defines         Addr        RW  init    Description
//-----------------------------------------------------------------------------
    #define     X00_SYSTEM_CONTROL      0       //  RW  10h     Power save and interrupt output control
    #define     X01_N19_16              1       //  RW  00h     20-bit N used for cycle time stamp
    #define     X02_N15_8               2       //  RW  00h
    #define     X03_N7_0                3       //  RW  00h
    #define     X04_SPDIF_FS            4       //  RO  00h     SPDIF sampling frequency/CTS[19:16] internal
    #define     X05_CTS_INT             5       //  RO  00h     CTS[15:8] internal
    #define     X06_CTS_INT             6       //  RO  00h     CTS[7:0] internal
    #define     X07_CTS_EXT             7       //  RW  00h     CTS[19:16] external
    #define     X08_CTS_EXT             8       //  RW  00h     CTS[15:8] external
    #define     X09_CTS_EXT             9       //  RW  00h     CTS[7:0] external
    #define     X0A_AUDIO_SOURCE        10      //  RW  00h     Audio setting.1
    #define     X0B_AUDIO_SET2          11      //  RW  00h     Audio setting.2
    #define     X0C_I2S_MODE            12      //  RW  00h     I2S audio setting
    #define     X0D_DSD_MODE            13      //  RW  00h     DSD audio setting
    #define     X0E_DEBUG_MONITOR1      14      //  RO  00h     Reserved
    #define     X0F_DEBUG_MONITOR2      15      //  RO  00h     Reserved
    #define     X10_I2S_PINMODE         16      //  RW  00h     I2S input pin swap
    #define     X11_ASTATUS1            17      //  RW  00h     Audio status bits setting1
    #define     X12_ASTATUS2            18      //  RW  00h     Audio status bits setting2
    #define     X13_CAT_CODE            19      //  RW  00h     Category code
    #define     X14_A_SOURCE            20      //  RW  00h     Source number/ Audio word length
    #define     X15_AVSET1              21      //  RW  00h     Audio/Video setting.1
    #define     X16_VIDEO1              22      //  RW  34h     Video setting.1
    #define     X17_DC_REG              23      //  RW  20h     Deep color setting
    #define     X18_CSC_C0_HI           24      //  RW  04h     Color Space Conversion Parameters
    #define     X19_CSC_C0_LO           25      //  RW  00h
    #define     X1A_CSC_C1_HI           26      //  RW  05h     Color Space Conversion Parameters
    #define     X1B_CSC_C1_LO           27      //  RW  09h
    #define     X1C_CSC_C2_HI           28      //  RW  00h
    #define     X1D_CSC_C2_LO           29      //  RW  00h
    #define     X1E_CSC_C3_HI           30      //  RW  02h
    #define     X1F_CSC_C3_LO           31      //  RW  A1h
    #define     X20_CSC_C4_HI           32      //  RW  04h
    #define     X21_CSC_C4_LO           33      //  RW  00h
    #define     X22_CSC_C5_HI           34      //  RW  12h
    #define     X23_CSC_C5_LO           35      //  RW  91h
    #define     X24_CSC_C6_HI           36      //  RW  11h
    #define     X25_CSC_C6_LO           37      //  RW  59h
    #define     X26_CSC_C7_HI           38      //  RW  00h
    #define     X27_CSC_C7_LO           39      //  RW  7Dh
    #define     X28_CSC_C8_HI           40      //  RW  04h
    #define     X29_CSC_C8_LO           41      //  RW  00h
    #define     X2A_CSC_C9_HI           42      //  RW  00h
    #define     X2B_CSC_C9_LO           43      //  RW  00h
    #define     X2C_CSC_C10_HI          44      //  RW  06h
    #define     X2D_CSC_C10_LO          45      //  RW  EFh
    #define     X2E_CSC_C11_HI          46      //  RW  02h
    #define     X2F_CSC_C11_LO          47      //  RW  DDh
    #define     X30_EXT_VPARAMS         48      //  RW  00h     External video parameter settings
    #define     X31_EXT_HTOTAL          49      //  RW  00h     External horizontal total
    #define     X32_EXT_HTOTAL          50      //
    #define     X33_EXT_HBLANK          51      //  RW  00h     External horizontal blank
    #define     X34_EXT_HBLANK          52      //
    #define     X35_EXT_HDLY            53      //  RW  00h     External horizontal delay
    #define     X36_EXT_HDLY            54      //
    #define     X37_EXT_HS_DUR          55      //  RW  00h     External horizontal duration
    #define     X38_EXT_HS_DUR          56      //
    #define     X39_EXT_VTOTAL          57      //  RW  00h     External vertical total
    #define     X3A_EXT_VTOTAL          58      //
    #define     X3B_AVSET2                              59              //  RW                                          00h     Audio/Video setting.2
        #define     CSC ENABLE                                  0x01    //          R/W                                     1b  Color Space Conversion enable
        #define     SEL_FULL_RANGE                              0x02    //          R/W                                     1b  Select Full/Limited range for Send black video mode
        #define     EN_M0_LOAD                                  0x04    //          R/W                                     1b  Load M0 into Akey area
        #define     EXT_DE_CNT                                  0x20    //          R/W                                     1b  External DE control
        #define     CD_ZERO                                     0x40    //          R/W                                     1b  CD all zero override
        #define     CTS_DEBUG                                   0x80    //          R/W                                     1b  Debug bit for CTS timing
    #define     X3C_EX_VID              60      //  RW  00h     External input Video ID(VID)
    #define     X3D_EXT_VBLANK          61      //  RW  00h     External virtical blank
    #define     X3E_EXT_VDLY            62      //  RW  00h     External virtical delay
    #define     X3F_EXT_VS_DUR          63      //  RW  00h     External virtical duration
    #define     X40_CTRL_PKT_EN         64      //  RW  00h     Control packet enable
    #define     X41_SEND_CPKT_AUTO      65      //  RW  00h     HB0 for generic control packet
    #define     X42_AUTO_CHECKSUM       66      //  RW  00h     Auto checksum option
//  #define     RESERVED                67      //  RW  00h
//  #define     RESERVED                68      //  RW  00h
    #define     X45_VIDEO2                              69              //  RW                                          00h     Video setting.2
        #define     NOVIDEO                                     0x01    //          R/W                                     1b  Send black video
        #define     NOAUDIO                                     0x02    //          R/W                                     1b  Send no audio
        #define     AUDIORST                                    0x04    //          R/W                                     1b  Audio capture logic reset
        #define     SET_AV_MUTE                                 0x40    //          R/W                                     1b  Send gSet AV muteh
        #define     CLEAR_AV_MUTE                               0x80    //          R/W                                     1b  Clear AV muteh
//  #define     RESERVED                70      //  RW  00h     Reserved
    #define     X47_VIDEO4              71      //  RW  00h     Video setting.4
    #define     X48_ACT_LN_STRT_LSB     72      //  RW  00h     Active Line Start LSB
    #define     X49_ACT_LN_STRT_MSB     73      //  RW  00h     Active Line Start MSB
    #define     X4A_ACT_LN_END_LSB      74      //  RW  00h     Active Line End LSB
    #define     X4B_ACT_LN_END_MSB      75      //  RW  00h     Active Line End MSB
    #define     X4C_ACT_PIX_STRT_LSB    76      //  RW  00h     Active Pixel Start LSB
    #define     X4D_ACT_PIX_STRT_MSB    77      //  RW  00h     Active Pixel Start MSB
    #define     X4E_ACT_PIX_END_LSB     78      //  RW  00h     Active Pixel End LSB
    #define     X4F_ACT_PIX_END_MSB     79      //  RW  00h     Active Pixel End MSB
    #define     X50_EXT_AUDIO_SET       80      //  RW  00h     Extra audio setting
//SLI10121
    #define     X51_SPEAKER_MAP         81      //  RW  00h     speaker Mapping CA[7:0]
//IP V2.11
    #define     X51_PHY_CTRL            81      //  RW  00h     Revervd[7:4],PHY_OPTION[3:0]

    #define     X52_HSYNC_PLACE_656     82      //  RW  00h     HSYNC placement at ITU656
    #define     X53_HSYNC_PLACE_656     83      //
    #define     X54_VSYNC_PLACE_656     84      //  RW  00h     VSYNC placement at ITU656
    #define     X55_VSYNC_PLACE_656     85      //
    #define     X56_PHY_CTRL            86      //  RW  0Fh     SLIPHDMIT parameter settings
    #define     X57_PHY_CTRL            87      //  RW  00h
    #define     X58_PHY_CTRL            88      //  RW  00h
    #define     X59_PHY_CTRL            89      //  RW  20h
    #define     X5A_PHY_CTRL            90      //  RW  00h
    #define     X5B_PHY_CTRL            91      //  RW  20h
    #define     X5C_PHY_CTRL            92      //  RW  00h
    #define     X5D_PHY_CTRL            93      //  RW  00h
    #define     X5E_PHY_CTRL            94      //  RW  00h
    #define     X5F_PACKET_INDEX                        95              //  R/W                                         00h     Packet Buffer Index
        #define     GENERIC_PACKET                              0x00    //          R/W                                     00h Generic packet
        #define     ACP_PACKET                                  0x01    //          R/W                                     00h ACP packet
        #define     ISRC1_PACKET                                0x02    //          R/W                                     00h ISRC1 packet
        #define     ISRC2_PACKET                                0x03    //          R/W                                     00h ISRC2 packet
        #define     GAMUT_PACKET                                0x04    //          R/W                                     00h Gamut metadata packet
        #define     VENDOR_INFO_PACKET                          0x05    //          R/W                                     00h Vendor specific InfoFrame
        #define     AVI_INFO_PACKET                             0x06    //          R/W                                     00h AVI InfoFrame
        #define     PRODUCT_INFO_PACKET                         0x07    //          R/W                                     00h Source product descriptor InfoFrame
        #define     AUDIO_INFO_PACKET                           0x08    //          R/W                                     00h Audio InfoFrame packet
        #define     MPEG_SRC_INFO_PACKET                        0x09    //          R/W                                     00h MPEG source InfoFrame
    #define     X60_PACKET_HB0          96      //  RW  00h     HB0
    #define     X61_PACKET_HB1          97      //  RW  00h     HB1
    #define     X62_PACKET_HB2          98      //  RW  00h     HB2
    #define     X63_PACKET_PB0          99      //  RW  00h     PB0
    #define     X64_PACKET_PB1          100     //  RW  00h
    #define     X65_PACKET_PB2          101     //  RW  00h
    #define     X66_PACKET_PB3          102     //  RW  00h
    #define     X67_PACKET_PB4          103     //  RW  00h
    #define     X68_PACKET_PB5          104     //  RW  00h
    #define     X69_PACKET_PB6          105     //  RW  00h
    #define     X6A_PACKET_PB7          106     //  RW  00h
    #define     X6B_PACKET_PB8          107     //  RW  00h
    #define     X6C_PACKET_PB9          108     //  RW  00h
    #define     X6D_PACKET_PB10         109     //  RW  00h
    #define     X6E_PACKET_PB11         110     //  RW  00h
    #define     X6F_PACKET_PB12         111     //  RW  00h
    #define     X70_PACKET_PB13         112     //  RW  00h
    #define     X71_PACKET_PB14         113     //  RW  00h
    #define     X72_PACKET_PB15         114     //  RW  00h
    #define     X73_PACKET_PB16         115     //  RW  00h
    #define     X74_PACKET_PB17         116     //  RW  00h
    #define     X75_PACKET_PB18         117     //  RW  00h
    #define     X76_PACKET_PB19         118     //  RW  00h
    #define     X77_PACKET_PB20         119     //  RW  00h
    #define     X78_PACKET_PB21         120     //  RW  00h
    #define     X79_PACKET_PB22         121     //  RW  00h
    #define     X7A_PACKET_PB23         122     //  RW  00h
    #define     X7B_PACKET_PB24         123     //  RW  00h
    #define     X7C_PACKET_PB25         124     //  RW  00h
    #define     X7D_PACKET_PB26         125     //  RW  00h
    #define     X7E_PACKET_PB27         126     //  RW  00h     PB27
//  #define     RESERVED                127     //  Reserved
    #define     X80_EDID                128     //  RO  -       Access window for EDID buffer
    #define     X81_ISRC2_PB0           129     //  RW  00h     ISRC2 PB0-15
    #define     X82_ISRC2_PB1           130     //  RW  00h
    #define     X83_ISRC2_PB2           131     //  RW  00h
    #define     X84_ISRC2_PB3           132     //  RW  00h
    #define     X85_ISRC2_PB4           133     //  RW  00h
    #define     X86_ISRC2_PB5           134     //  RW  00h
    #define     X87_ISRC2_PB6           135     //  RW  00h
    #define     X88_ISRC2_PB7           136     //  RW  00h
    #define     X89_ISRC2_PB8           137     //  RW  00h
    #define     X8A_ISRC2_PB9           138     //  RW  00h     ISRC2 PB0-15
    #define     X8B_ISRC2_PB10          139     //  RW  00h
    #define     X8C_ISRC2_PB11          140     //  RW  00h
    #define     X8D_ISRC2_PB12          141     //  RW  00h
    #define     X8E_ISRC2_PB13          142     //  RW  00h
    #define     X8F_ISRC2_PB14          143     //  RW  00h
    #define     X90_ISRC2_PB15          144     //  RW  00h
    #define     X91_ISRC1_HB1           145     //  RW  00h     ISRC2 HB1
    #define     X92_INT_MASK1                   146             //  RW                                          C0h     Mask for Interrupt Group1
        #define     EDID_ERR_MSK                        0x02    //          R/W                                     1b  EDID error detect interrupt mask
        #define     EDID_RDY_MSK                        0x04    //          R/W                                     1b  EDID ready interrupt mask
        #define     AFIFO_FULL_MSK                      0x10    //          R/W                                     0b  Audio FIFO full detect interrupt mask
        #define     VSYNC_MSK                           0x20    //          R/W                                     0b  VSYNC detect interrupt mask
        #define     MSENS_MSK                           0x40    //          R/W                                     0b  MSENS detect interrupt mask
        #define     HPG_MSK                             0x80    //          R/W                                     0b  Hot plug detect interrupt mask
    #define     X93_INT_MASK2                   147             //  RW                                          78h     Mask for Interrupt Group2
        #define     RDY_AUTH_MSK                        0x08    //          R/W                                     1b  Authentication ready interrupt mask
        #define     AUTH_DONE_MSK                       0x10    //          R/W                                     1b  HDCP authentication done interrupt mask
        #define     BKSV_RCRDY_MSK                      0x20    //          R/W                                     1b  BKSV ready from receiver interrupt mask
        #define     BKSV_RPRDY_MSK                      0x40    //          R/W                                     1b  BKSVs list ready from repeater interrupt mask
        #define     HDCP_ERR_MSK                        0x80    //          R/W                                     0b  HDCP error detect interrupt mask
    #define     X94_INT1_ST                     148             //  RW                                          00h     Interrupt status Group1
        #define     EDID_ERR_INT                        0x02    //          R/W                                     0b  EDID error detect interrupt
        #define     EDID_RDY_INT                        0x04    //          R/W                                     0b  EDID ready interrupt
        #define     AFIFO_FULL_INT                      0x10    //          R/W                                     0b  Audio FIFO full detect interrupt
        #define     VSYNC_INT                           0x20    //          R/W                                     0b  VSYNC detect interrupt
        #define     MSENS_INT                           0x40    //          R/W                                     0b  MSENS detect interrupt
        #define     HPG_INT                             0x80    //          R/W                                     0b  Hot plug detect interrupt
    #define     X95_INT2_ST                     149             //  RW                                          00h     Interrupt status Group2
        #define     RDY_AUTH_INT                        0x08    //          R/W                                     0b  Authentication ready interrupt
        #define     AUTH_DONE_INT                       0x10    //          R/W                                     0b  HDCP authentication done interrupt
        #define     BKSV_RCRDY_INT                      0x20    //          R/W                                     0b  BKSV ready from receiver interrupt
        #define     BKSV_RPRDY_INT                      0x40    //          R/W                                     0b  BKSVs list ready from repeater interrupt
        #define     HDCP_ERR_INT                        0x80    //          R/W                                     0b  HDCP error detect interrupt
//  #define     VN1                     150     //  RW  00h     Generic control packet (PB1-PB23)
//  #define     VN2                     151     //  RW  00h
//  #define     VN3                     152     //  RW  00h
//  #define     VN4                     153     //  RW  00h
//  #define     VN5                     154     //  RW  00h
//  #define     VN6                     155     //  RW  00h
//  #define     VN7                     156     //  RW  00h
//  #define     VN8                     157     //  RW  00h
//  #define     PD1                     158     //  RW  00h
//  #define     PD2                     159     //  RW  00h
//  #define     PD3                     160     //  RW  00h
//  #define     PD4                     161     //  RW  00h
//  #define     PD5                     162     //  RW  00h
//  #define     PD6                     163     //  RW  00h
//  #define     PD7                     164     //  RW  00h
//  #define     PD8                     165     //  RW  00h
//  #define     PD9                     166     //  RW  00h
//  #define     PD10                    167     //  RW  00h
//  #define     PD11                    168     //  RW  00h
//  #define     PD12                    169     //  RW  00h
//  #define     PD13                    170     //  RW  00h
//  #define     PD14                    171     //  RW  00h
//  #define     PD15                    172     //  RW  00h
//  #define     PD16                    173     //  RW  00h     Generic control packet (PB24-PB25)
//  #define     SRC_DEV_INFO            174     //  RW  00h
    #define     XAF_HDCP_CTRL                   175             //  R/W                                         12h     HDCP Control Register
        #define     HDCP_RESET                          0x01    //          R/W                                     0b  Reset HDCP
        #define     HDMI_MODE_CTRL                      0x02    //          R/W                                     0b  HDMI/DVI mode
        #define     ADV_CIPHER                          0x04    //          R/W                                     0b  Advanced cipher mode
        #define     STOP_AUTH                           0x08    //          R/W                                     0b  Stop HDCP authentication
        #define     FRAME_ENC                           0x10    //          R/W                                     0b  Frame encrypt
        #define     BKSV_FAIL                           0x20    //          R/W                                     0b  BKSV check result (FAIL)
        #define     BKSV_PASS                           0x40    //          R/W                                     0b  BKSV check result (PASS)
        #define     HDCP_REQ                            0x80    //          R/W                                     0b  HDCP authentication start
//  #define     AN                      176     //  RO  00h     An register
//                                      177     //
//                                      178     //
//                                      179     //
//                                      180     //
//                                      181     //
//                                      182     //
//                                      183     //
    #define     XB0_HDCP_STATUS                 176             //  RO                                          00h     HDCP Status Register
    #define     XB8_HDCP_STATUS                 184             //  RO                                          00h     HDCP Status Register
        #define     ADV_CIPHERI_STATUS                  0x08    //          R                                       0b  Advanced cipher status
        #define     HDCP_IDLE                           0x10    //          R                                       0b  HDCP state machine status
        #define     HDMI_STATUS                         0x20    //          R                                       0b  HDMI/DVI status
        #define     ENC                                 0x40    //          R                                       0b  Encryption status
        #define     AUTH                                0x80    //          R                                       0b  HDCP authentication status
//  #define     SHA_DISP0               185     //  RO  00h     SHA1 value
//  #define     SHA_DISP1               186     //  RO  00h
//  #define     SHA_DISP2               187     //  RO  00h
//  #define     SHA_DISP3               188     //  RO  00h
//  #define     SHA_DISP4               189     //  RO  00h
    #define     XBE_BCAPS               190     //  RO  00h     BCAPS value
    #define     XBF_KSV7_0              191     //  RO  00h     KSV[7:0] - AKSV/BKSV monitor registers
    #define     XC0_KSV15_8             192     //  RO  00h     KSV[15:8] - AKSV/BKSV monitor registers
    #define     XC1_KSV23_16            193     //  RO  00h     KSV[23:16] - AKSV/BKSV monitor registers
    #define     XC2_KSV31_24            194     //  RO  00h     KSV[31:24] - AKSV/BKSV monitor registers
    #define     XC3_KSV39_32            195     //  RO  00h     KSV[39:32] - AKSV/BKSV monitor registers
    #define     XC4_SEG_PTR             196     //  RW  00h     EDID segment pointer
    #define     XC5_EDID_WD_ADDR        197     //  RW  00h     EDID word address
    #define     XC6_GEN_PB26            198     //  RW  00h     Generic control packet (PB26)
//  #define     NUM_DEV                 199     //  RO  00h     HDCP BKSV Size
    #define     XC8_HDCP_ERR            200     //  RO  00h     HDCP error
        #define     BAD_BKSV                            0x01    //          R/W                                     0b  BKSV does not contain 20 0's and 20 1's
    #define     XC9_TIMER               201     //  RW  46h     Timer value for 100ms
    #define     XCA_TIMER               202     //  RW  2ch     Timer value for 5sec
    #define     XCB_READ_RI_CNT         203     //  RW  12h     Ri read count
    #define     XCC_AN_SEED             204     //  RW  00h     An Seed
    #define     XCD_MAX_REC_NUM         205     //  RW  16d     maximum number of receivers allowed
//  #define     //RESERVED              206     //      RO      00h
    #define     XCF_HDCP_MEM_CTRL2              207             //  RW                                          0Oh     [1:0] ID_FAX_KEY, ID_HDCP_KEY
        #define     LD_HDCP_KEY                         0x01    //          R/W                                     0b  Trigger for loading HDCP key from external memory
        #define     LD_FAX_KEY                          0x02    //          R/W                                     0b  Trigger for loading fax HDCP key
    #define     XD0_HDCP_CTRL2                  208             //  R/W                                         08h     HDCP Control 2
        #define     DELAY_RI_CHK                        0x08    //          R/W                                     0b  Set this bit to compare Ri at 129th frame instead of 128th frame for slower receivers.
        #define     DIS_0LEN_HASH                       0x10    //          R/W                                     0b  Some early repeaters may not load Hash value if number of devices is 0. Set this bit to skip Hash comparison when number of devices in Bstatus equals 0.
        #define     EN_PJ_CALC                          0x20    //          R/W                                     0b  Even though this bit is set, Pj calculation is enabled only if adv_cipher mode is selected.
        #define     DIS_127_CHK                         0x80    //          R/W                                     0b  This bit must be set to disable 127th Ri check if Ri check frequency is altered by ri_frame_cnt (B2h).
//  #define     //RESERVED              209     //      RO      00h
    #define     XD2_HDCP_KEY_CTRL               210             //  R/W                                         00h     HDCP KEY memory control
        #define     KEY_READY                           0x01    //          R                                       0b  This bit shows that HDCP key load has finished.
        #define     KEY_VALID                           0x02    //          R                                       0b  This bit shows whether HDCP key loaded from HDCP memory is valid.
        #define     KSV_VALID                           0x04    //          R                                       0b  This bit shows whether the loaded KSV is valid (has 20 1fs and 0fs).
        #define     KSV_SEL                             0x08    //          R                                       0b  This bit shows which KSV was actually loaded into memory from HDCP memory.
        #define     LOAD_AKSV                           0x10    //          R/W                                     0b  Select which KSV to be loaded into AKSV/BKSV register (BFh~C3h). Write 1 to load AKSV.
        #define     USE_KSV2                            0x20    //          R/W                                     0b  If this bit is set, load the 2nd KSV written in the HDCP memory. If both usv_ksv1 and use_ksv2 are 0, hardware loads whichever that has 20 1fs and 0fs.
        #define     USE_KSV1                            0x40    //          R/W                                     0b  If this bit is set, load the 1st KSV written in the HDCP memory.
    #define     XD3_CSC_CONFIG1         211     //  RW  81h     CSC/Video Config.1
    #define     XD4_VIDEO3              212     //  RW  00h     Video setting 3
//  #define     RI                      213     //  RO  00h     Ri
//                                      214     //  RO  00h     Pj
//  #define     PJ                      215     //  Dir.    Reset       Descriptions
//  #define     SHA_RD                  216     //  RW  00h     SHA index for read
    #define     XD9_GEN_PB27            217     //  RW  00h     Generic InfoFrame PB27
//  #define     MPEG_B0                 218     //  RW  00h     MPEG source InfoFrame
//  #define     MPEG_B1                 219     //  RW  00h
//  #define     MPEG_B2                 220     //  RW  00h
//  #define     MPEG_B3                 221     //  RW  00h
//  #define     MPEG_FR_MF              222     //  RW  00h
    #define     XDF_HPG_STATUS                  223             //  R                                           00h     Hot plug/MSENS status
        #define     BIST_FAIL                           0x01    //          R                                       0b  Dual port RAM BIST result fail
        #define     BIST_PASS                           0x02    //          R                                       0b  Dual port RAM BIST result passed
        #define     MSENS_PRT                           0x40    //          R                                       0b  MSENS input port status
        #define     HPG_PRT                             0x80    //          R                                       0b  Hot plug input port status
    #define     XE0_GAMUT_HB1           224     //  RW  00h     gamut metadata HB1
    #define     XE1_GAMUT_HB2           225     //  RW  00h     gamut metadata HB2
//  #define     GAMUT_PB0               226     //  RW  00h     gamut metadata PB0
//  #define     GAMUT_PB1               227     //  RW  00h     gamut metadata PB1
//  #define     GAMUT_PB2               228     //  RW  00h     gamut metadata PB2
//  #define     GAMUT_PB3               229     //  RW  00h     gamut metadata PB3
//  #define     GAMUT_PB4               230     //  RW  00h     gamut metadata PB4
//  #define     GAMUT_PB5               231     //  RW  00h     gamut metadata PB5
    #define     XE8_AN0                 232     //  RW  00h     An[7:0]
//  #define     GAMUT_PB7               233     //  RW  00h     gamut metadata PB7
//  #define     GAMUT_PB8               234     //  RW  00h     gamut metadata PB8
//  #define     GAMUT_PB9               235     //  RW  00h     gamut metadata PB9
//  #define     GAMUT_PB10              236     //  RW  00h     gamut metadata PB10
//  #define     GAMUT_PB11              237     //  RW  00h     gamut metadata PB11
//  #define     GAMUT_PB12              238     //  RW  00h     gamut metadata PB12
//  #define     GAMUT_PB13              239     //  RW  00h     gamut metadata PB13
//  #define     GAMUT_PB14              240     //  RW  00h     gamut metadata PB14
//  #define     GAMUT_PB15              241     //  RW  00h     gamut metadata PB15
//  #define     GAMUT_PB16              242     //  RW  00h     gamut metadata PB16
//  #define     GAMUT_PB17              243     //  RW  00h     gamut metadata PB17
//  #define     GAMUT_PB18              244     //  RW  00h     gamut metadata PB18
//  #define     GAMUT_PB19              245     //  RW  00h     gamut metadata PB19
//  #define     GAMUT_PB20              246     //  RW  00h     gamut metadata PB20
//  #define     GAMUT_PB21              247     //  RW  00h     gamut metadata PB21
//  #define     GAMUT_PB22              248     //  RW  00h     gamut metadata PB22
//  #define     GAMUT_PB23              249     //  RW  00h     gamut metadata PB23
//  #define     GAMUT_PB24              250     //  RW  00h     gamut metadata PB24
//  #define     GAMUT_PB25              251     //  RW  00h     gamut metadata PB25
//  #define     GAMUT_PB26              252     //  RW  00h     gamut metadata PB26
//  #define     GAMUT_PB27              253     //  RW  00h     gamut metadata PB27
//  #define     TEST_MODE               254     //  RW  00h     test mode register
    #define     XFF_IP_COM_CONTROL              255             //  R/W                                         40h     I/P conversion control
        #define     IP_CONV_PIX_REP                     0x01    //          R/W                                     0b  I/P conversion control
        #define     IP_CONV_EN                          0x02    //          R/W                                     0b  I/P conversion mode control
        #define     PRE_COLOR_CONV_ON                   0x10    //          R/W                                     0b  Pre-color space converter (RGB->YCbCr) control
        #define     PRE_DOWN_CONV_ON                    0x20    //          R/W                                     0b  Pre-pixel encoding converter (down sampler) control
        #define     STGAM_OFF                           0x40    //          R/W                                     1b  Gamma correction control
        #define     IP_REG_OFFSET                       0x80    //          R/W                                     0b  I/P conversion control register access control


// SLISHDMI13T I/P conversion registers Defintions
//--------------------------------------------------------------------------------------------------------------------------------------------------------------
// SLISHDMI13R  Regsiter Defines                Addr    bit         RW                                          init    Description
//--------------------------------------------------------------------------------------------------------------------------------------------------------------
    #define     FILM_DETECTION_SETTING_1        0x07            //  R/W                                         FFh
    #define     FILM_DETECTION_SETTING_2        0x08            //  R/W                                         08h
    #define     FILM_DETECTION_SETTING_3        0x09            //  R/W                                         4Ch
    #define     FILM_DETECTION_SETTING_4        0x0A            //  R/W                                         C2h
    #define     FILM_DETECTION_SETTING_5        0x0B            //  R/W                                         48h
    #define     FILM_DETECTION_SETTING_6        0x0C            //  R/W                                         02h
    #define     FILM_DETECTION_SETTING_7        0x0D            //  R/W                                         61h
    #define     FILM_DETECTION_SETTING_8        0x0E            //  R/W                                         2Dh
    #define     FILM_DETECTION_SETTING_9        0x0F            //  R/W                                         00h
    #define     HORIZONTAL_PIXEL_START_POSITION_TO_LINE_MEMORY_LSB      0x10        //  R/W                     AEh
    #define     HORIZONTAL_PIXEL_START_POSITION_TO_LINE_MEMORY_MSB      0x11        //  R/W                     00h
    #define     HORIZONTAL_PIXEL_COUNT_TO_LINE_MEMORY_LSB               0x12        //  R/W                     84h
    #define     HORIZONTAL_PIXEL_COUNT_TO_LINE_MEMORY_MSB               0x13        //  R/W                     07h
    #define     VERTICAL_PIXEL_START_POSITION_TO_LINE_MEMORY_LSB        0x14        //  R/W                     12h
    #define     VERTICAL_PIXEL_START_POSITION_TO_LINE_MEMORY_MSB        0x15        //  R/W                     00h
    #define     VERTICAL_PIXEL_COUNT_TO_LINE_MEMORY_LSB                 0x16        //  R/W                     1Ch
    #define     VERTICAL_PIXEL_COUNT_TO_LINE_MEMORY_MSB                 0x17        //  R/W                     02h
    #define     OUTPUT_HSYNC_POSITION_LSB       0x18            //  R/W                                         C5h
    #define     OUTPUT_HSYNC_POSITION_MSB       0x19            //  R/W                                         00h
    #define     OUTPUT_HSYNC_WIDTH              0x1A            //  R/W                                         2Ch
    #define     OUTPUT_VSYNC_POSITION_LSB       0x1B            //  R/W                                         02h
    #define     OUTPUT_VSYNC_POSITION_MSB       0x1C            //  R/W                                         00h
    #define     OUTPUT_VSYNC_WIDTH              0x1D            //  R/W                                         05h
    #define     Y_LEVEL_ON_OUT_OF_VALID_SCREEN  0x1E            //  R/W                                         10h
    #define     C_LEVEL_ON_OUT_OF_VALID_SCREEN  0x1F            //  R/W                                         80h
    #define     INTERNAL_VERTICAL_RESET_POSITION_ADJUSTMENT_LSB 0x20    //  RW                                  00h
    #define     INTERNAL_VERTICAL_RESET_POSITION_ADJUSTMENT_MSB 0x21    //  RW                                  00h
    #define     FUNCTION_MODE_SETTINGS          0x22            //  R/W                                         00h
        #define     INTER_OFF                           0x01    //          R/W                                     0b   Delay select for I/P conversion
        #define     VLPF_CTR                            0x02    //          R/W                                     0b   Vertical filter select
        #define     INTERADPT                           0x04    //          R/W                                     0b   Interpolation select between frames
//  #define     Motion detection setting.2 (#24h)
//  #define     Motion detection setting.3 (#25h)
//  #define     Motion detection setting.4 (#26h)
//  #define     Diagonal edge detect setting.1 (#27h)
//  #define     Diagonal edge detect setting.2 (#28h)
//  #define     Diagonal edge detect setting.3 (#29h)
//  #define     Diagonal edge detect setting.4 (#2Ah)
    #define     VIDEO_PARAMETER_SETTINGS_AFTER  0x30            //  R/W                                         00h
    #define     HORIZONTAL_TOTAL_AFTER_LSB      0x31            //  R/W                                         00h
    #define     HORIZONTAL_TOTAL_AFTER_MSB      0x32            //  R/W                                         00h
    #define     HORIZONTAL_BLANK_AFTER_LSB      0x33            //  R/W                                         00h
    #define     HORIZONTAL_BLANK_AFTER_MSB      0x34            //  R/W                                         00h
    #define     HORIZONTAL_DELAY_AFTER_LSB      0x35            //  R/W                                         00h
    #define     HORIZONTAL_DELAY_AFTER_MSB      0x36            //  R/W                                         00h
    #define     HORIZONTAL_DURATION_AFTER_LSB   0x37            //  R/W                                         00h
    #define     HORIZONTAL_DURATION_AFTER_MSB   0x38            //  R/W                                         00h
    #define     VERTICAL_TOTAL_AFTER_LSB        0x39            //  R/W                                         00h
    #define     VERTICAL_TOTAL_AFTER_MSB        0x3A            //  R/W                                         00h
    #define     OUTPUT_VIDEO_FORMAT_VID_AFTER   0x3C            //  R/W                                         00h
    #define     VERTICAL_BLANK_AFTER            0x3D            //  R/W                                         00h
    #define     VERTICAL_DELAY_AFTER            0x3E            //  R/W                                         00h
    #define     VERTICAL_DURATION_AFTER         0x3F            //  R/W                                         00h
//  #define     GAMMA_CORRECTION_1              0x40            //  R/W                                         00h
//  #define     GAMMA_CORRECTION_97             0xA0            //  R/W                                         00h
//  #define     Pre-Color space conversion parameters (#B0h)
//  #define     Pre-Color space conversion parameters (#B1h)
//  #define     Pre-Color space conversion parameters (#B2h)
//  #define     Pre-Color space conversion parameters (#B3h)
//  #define     Pre-Color space conversion parameters (#B4h)
//  #define     Pre-Color space conversion parameters (#B5h)
//  #define     Pre-Color space conversion parameters (#B6h)
//  #define     Pre-Color space conversion parameters (#B7h)
//  #define     Pre-Color space conversion parameters (#B8h)
//  #define     Pre-Color space conversion parameters (#B9h)
//  #define     Pre-Color space conversion parameters (#BAh)
//  #define     Pre-Color space conversion parameters (#BBh)
//  #define     Pre-Color space conversion parameters (#BCh)
//  #define     Pre-Color space conversion parameters (#BDh)
//  #define     Pre-Color space conversion parameters (#BEh)
//  #define     Pre-Color space conversion parameters (#BFh)
//  #define     Pre-Color space conversion parameters (#C0h)
//  #define     Pre-Color space conversion parameters (#C1h)
//  #define     Pre-Color space conversion parameters (#C2h)
//  #define     Pre-Color space conversion parameters (#C3h)
//  #define     Pre-Color space conversion parameters (#C4h)
//  #define     Pre-Color space conversion parameters (#C5h)
//  #define     Pre-Color space conversion parameters (#C6h)
//  #define     Pre-Color space conversion parameters (#C7h)
    #define     DE_HORIZONTAL_START_POSITION_LSB    0xD0        //  R/W                                         C0h
    #define     DE_HORIZONTAL_START_POSITION_MSB    0xD1        //  R/W                                         00h
    #define     DE_HORIZONTAL_END_POSITION_LSB  0xD2            //  R/W                                         60h
    #define     DE_HORIZONTAL_END_POSITION_MSB  0xD3            //  R/W                                         08h
    #define     DE_VERTICAL_START_POSITION_LSB  0xD4            //  R/W                                         29h
    #define     DE_VERTICAL_START_POSITION_MSB  0xD5            //  R/W                                         00h
    #define     DE_VERTICAL_END_POSITION_LSB    0xD6            //  R/W                                         61h
    #define     DE_VERTICAL_END_POSITION_MSB    0xD7            //  R/W                                         04h
//  #define     RECEIVED_N_DATA_FOR_DDR_TEST    0xD9            //  R/W                                         00h
//  #define     RECEIVED_N_DATA_FOR_DDR_TEST    0xDC            //  R/W                                         00h
//  #define     DDR controller PLL configuratiuon.1 (#E0h)
//  #define     DDR controller PLL configuratiuon.2 (#E1h)
//  #define     DDR controller PLL configuratiuon.3 (#E2h)
//  #define     DDR controller DQS0 delay control (#E3h)
//  #define     DDR controller DQS1 delay control (#E4h)
//  #define     DDR controller DQS2 delay control (#E5h)
//  #define     DDR controller DQS3 delay control (#E6h)
//  #define     DDR controller DQ0-7 delay control (#E7h)
//  #define     DDR controller DQ8-15 delay control (#E8h)
//  #define     DDR controller DQ16-23 delay control (#E9h)
//  #define     DDR controller DQ24-31 delay control (#EAh)
//  #define     DDR controller DQN0-7 delay control (#EBh)
//  #define     DDR controller DQN8-15 delay control (#ECh)
//  #define     DDR controller DQN16-23 delay control (#EDh)
//  #define     DDR controller DQN24-31 delay control (#EEh)
//  #define     DDR_TEST_ MODE                  0xF0            //  R/W                                         00h
//  #define     SEND_DATA_FOR_DDR_TEST          0xF1            //  R/W                                         00h
//  #define     SEND_DATA_FOR_DDR_TEST          0xF4            //  R/W                                         00h
//  #define     COMPARISON_DATA_FOR_DDR_TEST    0xF5            //  R/W                                         00h
//  #define     COMPARISON_DATA_FOR_DDR_TEST    0xF8            //  R/W                                         00h
//  #define     RECEIVED_P_DATA_FOR_DDR_TEST    0xF9            //  R/W                                         00h
//  #define     RECEIVED_P_DATA_FOR_DDR_TES     0xFC            //  R/W                                         00h
//  #define     DDR_CONTROLLER_INTERNAL_ERROR_STATUS    0xFD    //  R/W                                         00h

#endif  /* _HDMI_TXREGS_H_ */
