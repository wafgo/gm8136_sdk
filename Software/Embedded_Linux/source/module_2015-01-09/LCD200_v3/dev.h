#ifndef __LCD200_DEV_H__
#define  __LCD200_DEV_H__

#define BLEND_0   0
#define BLEND_6   1
#define BLEND_12  2
#define BLEND_18  3
#define BLEND_25  4
#define BLEND_31  5
#define BLEND_37  6
#define BLEND_43  7
#define BLEND_50  8
#define BLEND_56  9
#define BLEND_62  10
#define BLEND_68  11
#define BLEND_75  12
#define BLEND_81  13
#define BLEND_87  14
#define BLEND_93  15
#define BLEND_100 16

enum { PLL_486, PLL_594 };

/**
 * This structure describes the machine which we are running on.
 */

#define LCDHTIMING(HBP, HFP, HW, PL)  ((((HBP)-1)&0xFF)<<24|(((HFP)-1)&0xFF)<<16| \
				       (((HW)-1)&0xFF)<<8|((((PL)>>4)-1)&0xFF))
#define LCDVTIMING0(VFP, VW, LF)      (((VFP)&0xFF)<<24|(((VW)-1)&0x3F)<<16|(((LF)-1)&0xFFF))
#define LCDVTIMING1(VBP)              (((VBP)-1)&0xFF)
//#define LCDVTIMING1(VBP)              ((VBP)&0xFF)
#define LCDCFG(PLA_PWR, PLA_DE, PLA_CLK, PLA_HS, PLA_VS) (((PLA_PWR)&0x01)<<4| \
							  ((PLA_DE)&0x01)<<3| \
							  ((PLA_CLK)&0x01)<<2| \
							  ((PLA_HS)&0x01)<<1| \
							  ((PLA_VS)&0x01))
#define LCDPARM(PLA_PWR, PLA_DE, PLA_CLK, PLA_HS, PLA_VS)
#define LCDHVTIMING_EX(HBP, HFP, HW, VFP, VW, VBP)  \
            ((((((HBP)-1)>>8)&0x3)<<8) | (((((HFP)-1)>>8)&0x3)<<4) | (((((HW)-1)>>8)&0x3)<<0) | \
            ((((VFP)>>8)&0x3)<<16) | (((((VW)-1)>>8)&0x3)<<12) | (((((VBP)-1)>>8)&0x3)<<20))

struct LCD_TIMING {
    u32 H_Timing;               //0x100
    u32 V_Timing0;              //0x104
    u32 V_Timing1;              //0x108
    u32 Configure;              //0x10c
    u32 Parameter;              //0x004
    u32 SPParameter;            //0x200
    u32 HV_TimingEx;            //0x138 //LCD210 v1.9 new
};

#define CCIR656CYC(H,V)             (((V)&0xFFF)<<12|((H)&0xFFF))
#define CCIR656FIELDPLA(F1_0,F0_1)  (((F0_1)&0xFFF)<<12|((F1_0)&0xFFF))
#define CCIR656VBLANK01(VB0,VB1)    (((VB1)&0x3FF)<<12|((VB0)&0x3FF))
#define CCIR656VBLANK23(VB2,VB3)    (((VB3)&0x3FF)<<12|((VB2)&0x3FF))
#define CCIR656VACTIVE(VA0,VA1)     (((VA1)&0xFFF)<<12|((VA0)&0xFFF))
#define CCIR656HBLANK01(HB0,HB1)    (((HB1)&0x3FF)<<12|((HB0)&0x3FF))
#define CCIR656HBLANK2(HB2)         (((HB2)&0x3FF))
#define CCIR656HACTIVE(HA)          (((HA)&0xFFF))
#define CCIR656VBLANK45(VB4,VB5)    (((VB5)&0x3FF)<<12|((VB4)&0x3FF))
#define CCIR656VBI01(VBI_0, VBI_1)  (((VBI_1)&0x3FF)<<12|((VBI_0)&0x3FF))

#define CCIR656_OUTFMT_MASK      1
#define CCIR656_OUTFMT_INTERLACE 0
#define CCIR656_OUTFMT_PROGRESS  1

struct CCIR656_TIMING {
    u32 Cycle_Num;              /* in new lcd200, it includes H_cyc_num and V_Line_num */
    u32 Field_Polarity;
    u32 V_Blanking01;
    u32 V_Blanking23;
    u32 V_Active;
    u32 H_Blanking01;
    u32 H_Blanking2;
    u32 H_Active;
    u32 V_Blanking45;
    u32 VBI_01;
    u32 Parameter;
};

union lcd200_config {
    uint32_t value;
    struct {
        uint32_t LCD_En:1;
        uint32_t LCD_On:1;
        uint32_t YUV420_En:1;
        uint32_t YUV_En:1;
        uint32_t OSD_En:1;
        uint32_t Scalar_En:1;
        uint32_t Dither_En:1;
        uint32_t POP_En:1;
        uint32_t Blend_En:2;
        uint32_t PIP_En:2;
        uint32_t Cursor_En:1;
        uint32_t CCIR_En:1;
        uint32_t PatGen:1;
        uint32_t VBI_En:1;
        uint32_t AddrSyn_En:1;  //16
        uint32_t AXI_En:1;      //17, Double_En
        uint32_t reserved:1;    //18
        uint32_t Deflicker_En;  //19
        uint32_t Reserve:12;
    } reg;
};

struct lcd200_dev_info;

struct lcd200_dev_ops {
    int (*set_par) (struct ffb_dev_info * info);
    int (*ioctl) (struct fb_info * info, unsigned int cmd, unsigned long arg);
    int (*reset_buf_manage) (struct lcd200_dev_info * info);
     uint32_t(*reg_read) (struct ffb_dev_info * dev_info, int offset);
    void (*reg_write) (struct ffb_dev_info * dev_info, int offset, uint32_t value, uint32_t mask);
    int (*suspend) (struct lcd200_dev_info * info, pm_message_t state, u32 level);
    int (*resume) (struct lcd200_dev_info * info, u32 level);
    int (*switch_input_mode) (struct ffb_info * fbi);
    int (*reset_dev) (struct ffb_dev_info * info);
    int (*config_input_misc) (struct lcd200_dev_info * info);
};

struct lcd200_dev_cursor {
    __u32 dx;
    __u32 dy;
    __u16 enable;
};

struct dev_vbi_info {
    int vbi_line_num;           /* which line to store VBI */
    int vbi_line_height;        /* number of lines to store VBI */
};

#if 0
struct scalar_info {
    __u16 fir_sel;              // 000: bypass 1st stage scalar, 001: 1/4, 010: 1/16 ...
    __u16 hor_no_in;            // actual horizontal resoultion, note: don't decrease 1
    __u16 ver_no_in;            // actual vertical resoultion, note: don't decrease 1
    __u16 hor_no_out;           // actual horizontal resoultion = hor_no_out
    __u16 ver_no_out;           // actual vertical resoultion = ver_no_out
    __u16 hor_inter_mode;
    __u16 ver_inter_mode;
    __u16 sec_bypass_mode;      // 2nd stage bypass mode, 1 for bypass
    __u16 g_enable;             // global enable/disable
};
#endif

#define FB_NUM  4

struct lcd200_dev_info {
    struct ffb_dev_info dev_info;
    struct ffb_info *fb[FB_NUM];
    struct LCD_TIMING LCD;
    struct CCIR656_TIMING CCIR656;
    struct semaphore cfg_sema;  /* used to protect the config path */
    struct lcd200_dev_ops *ops;
    u8 GammaTable[3][256];
    struct lcd200_dev_cursor cursor;
    u32 cursor_32x32;
    u32 lcd200_idx;             /* lcd200 index, starts from LCD_ID */
    struct dev_vbi_info vbi_info;       /* VBI blanking line */
    struct scalar_info scalar;  /* LCD scalar */
    int support_ge;             /* Graphic Encode/Decode */
    u32 color_mgt_p0;           /* saturation, brightness */
    u32 color_mgt_p1;           /* hue          */
    u32 color_mgt_p2;           /* sharpness    */
    u32 color_mgt_p3;           /* contrast     */
    u32 color_key1;             /* color key1, need to use YUV444, RGB888...  24bit */
    u32 color_key2;             /* color key2, need to use YUV444, RGB888...  24bit, LCD210 uses only */
};

typedef enum FTLCD200_REGISTER_tag {
    FEATURE = 0x000,
    PANEL_PARAM = 0x004,
    INT_MASK = 0x008,
    INT_CLEAR = 0x00C,
    INT_STATUS = 0x010,
    POP_SCALE = 0x014,
    FB0_BASE = 0x018,
    FB1_BASE = 0x024,
    FB2_BASE = 0x030,
    FB3_BASE = 0x03C,
    PATTERN_GEN = 0x048,
    FIFO_TH_CTRL = 0x04C,       ///<FIFO Threshold Control
    GPIO_CTRL = 0x050,
    LCD_HTIMING = 0x100,
    LCD_VTIMING0 = 0x104,
    LCD_VTIMING1 = 0x108,
    LCD_POLARITY = 0x10C,
    LCD_HVTIMING_EX = 0x138,
    SPANEL_PARAM = 0x200,
    CCIR656_PARM = 0x204,
    CCIR_RES = 0x208,
    CCIR_FIELD_PLY = 0x20C,     ///<CCIR656 Field Polarity
    CCIR_VBLK01_PLY = 0x210,    ///<CCIR656 Vertical Blanking Polarity
    CCIR_VBLK23_PLY = 0x214,    ///<CCIR656 Vertical Blanking Polarity
    CCIR_VACT_PLY = 0x218,      ///<CCIR656 Vertical Active Polarity
    CCIR_HBLK01_PLY = 0x21C,    ///<CCIR656 Horizontal Blanking Polarity
    CCIR_HBLK2_PLY = 0x220,     ///<CCIR656 Horizontal Blanking Polarity
    CCIR_HACT_PLY = 0x224,      ///<CCIR656 Horizontal Active Polarity
    CCIR_VBLK_PARAM = 0x228,    ///<TV Veritcal Blank Parameters
    CCIR_VBI_PARAM = 0x22C,     ///<TV VBI Parameters
    PIP_BLEND = 0x300,
    PIP_PICT1_POS = 0x304,      ///<PIP Sub-Picture1 Position
    PIP_PICT1_DIM = 0x308,      ///<PIP Sub-Picture1 Dimination
    PIP_PICT2_POS = 0x30C,      ///<PIP Sub-Picture2 Position
    PIP_PICT2_DIM = 0x310,      ///<PIP Sub-Picture2 Dimination
    PIP_IMG_PRI = 0x314,        ///<PIP Image Priority
    PIOP_IMG_FMT1 = 0x318,      ///<PIP or POP Image Format1
    PIOP_IMG_FMT2 = 0x31C,      ///<PIP or POP Image Format2
    PIP_COLOR_KEY = 0x320,      ///<PIP Color Key
#ifdef LCD210
    PIP_COLOR_KEY2 = 0x324,     ///<PIP Color Key2
#endif
    LCD_COLOR_MP0 = 0x400,      ///<LCD Color Management Parament0
    LCD_COLOR_MP1 = 0x404,      ///<LCD Color Management Parament1
    LCD_COLOR_MP2 = 0x408,      ///<LCD Color Management Parament2
    LCD_COLOR_MP3 = 0x40C,      ///<LCD Color Management Parament3
    LCD_GAMMA_RLTB = 0x600,     ///<LCD Gamma Red Lookup Table Base
    LCD_GAMMA_GLTB = 0x700,     ///<LCD Gamma Green Lookup Table Base
    LCD_GAMMA_BLTB = 0x800,     ///<LCD Gamma Blue Lookup Table Base
    LCD_PALETTE_RAM = 0xA00,    ///<LCD Palette RAM Write Accessing Port (0xA00-0xBFC)
    LCD_SCALAR_HIN = 0x1100,    ///<Horizontal resolution register of scalar input
    LCD_SCALAR_VIN = 0x1104,    ///<Vertical resolution register of scalar input
    LCD_SCALAR_HOUT = 0x1108,   ///<Horizontal resolution register of scalar output
    LCD_SCALAR_VOUT = 0x110C,   ///<Vertical resolution register of scalar output
    LCD_SCALAR_MISC = 0x1110,   ///<Miscellaneous register of scalar
    LCD_SCALAR_HHTH = 0x1114,   ///<Horizontal high threshold register of scalar
    LCD_SCALAR_HLTH = 0x1118,   ///<Horizontal low threshold register of scalar
    LCD_SCALAR_VHTH = 0x111C,   ///<Vertical high threshold register of scalar
    LCD_SCALAR_VLTH = 0x1120,   ///<Vertical low threshold register of scalar
    LCD_SCALAR_PARM = 0x112C,   ///<Scalar resolution parameter
    LCD_CURSOR_POS = 0x1200,    ///<Cursor Position Control Register
    LCD_CURSOR_COL1 = 0x1204,   ///<Cursor Color 1 Control Register
    LCD_CURSOR_COL2 = 0x1208,   ///<Cursor Color 2 Control Register
    LCD_CURSOR_COL3 = 0x120C,   ///<Cursor Color 3 Control Register
    LCD_CURSOR_DATA = 0x1300,   ///<Cursor data Register(Offset 0x1300~0x133C), The cursor size is 12x16 or 32x32
    LCD_CURSOR_DATA_LTOP = 0x1300,      ///<Cursor data Register(Offset 0x1300~0x133C), The cursor size is 12x16 or 32x32
    LCD_CURSOR_DATA_LBOM = 0x1380,      ///<Cursor data Register (left bottom)
    LCD_CURSOR_DATA_RTOP = 0x1340,      ///<Cursor data Register (right top)
    LCD_CURSOR_DATA_RBOM = 0x13C0,      ///<Cursor data Register (right bottom)
    VIRTUAL_SCREEN0 = 0x1500,
    VIRTUAL_SCREEN1 = 0x1504,
    VIRTUAL_SCREEN2 = 0x1508,
    VIRTUAL_SCREEN3 = 0x150C,
    LCD_CURSOR_DATA_64x64 = 0x1600,
} FTLCD200_REGISTER;

extern int dev_construct(struct lcd200_dev_info *info, union lcd200_config config);
extern void dev_deconstruct(struct lcd200_dev_info *info);
extern int lcd200_init(void);
extern int lcd200_cleanup(void);
extern int tve100_config(int mode, int input);
extern void LCD200_Ctrl(struct ffb_dev_info *dev_info, int on);

#endif /*  __LCD200_DEV_H__ */
