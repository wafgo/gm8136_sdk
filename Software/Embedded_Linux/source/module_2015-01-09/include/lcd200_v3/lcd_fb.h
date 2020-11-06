/** file lcd_fb.h
 *
 *  description: the file provides the LCD related function calls or IOCTLs
 *
 *  author Harry Hsu
 *  version: 1.0
 *  ddate: 2010/9/24
 *
 */
#ifndef __LCD_FB_H__
#define __LCD_FB_H__

#ifndef __KERNEL__
#include <stdint.h>
#endif

/*
 * STRUCTURE
 */
#define MAX_FRAME_NO 10
#define LCD200_V3

/**
 * @brief This structure defines the structure of LCD scaler.
 */
struct scalar_info {
    int   hor_no_in;      ///< horizontal resoultion
    int   ver_no_in;      ///< vertical resoultion
    int   hor_no_out;     ///< horizontal resoultion
    int   ver_no_out;     ///< vertical resoultion
    int   hor_inter_mode; ///< please specify 0
    int   ver_inter_mode; ///< please specify 0
    int   g_enable;       ///< global enable/disable, 1 for enable, otherwise 0
};

/**
 * @brief: the old architecture of LCD use
 */
struct flcd_data {
	unsigned int buf_len;   ///< the buffer size of a frame buffer
	unsigned int uv_offset; ///< useless, it should be zero.
	unsigned int frame_no;  ///< how many frame buffers
	unsigned int mp4_map_dma[MAX_FRAME_NO]; ///< the virtual address via mmap()
};

/*
 * @brief: the palette ram
 */
struct palette {
    int  entry_idx; /* 1 ~ 255, 0 is used internal */
    int  color;     /* R[15:11], G[10:5], B[4:0] */
};

/*
 * @brief: the GUI clone
 * used to set the destination clone screen size
 */
struct gui_clone_adjust {
    int x;      /* x coordinate of the destination */
    int y;      /* y coordinate of the destination */
    int width;  /* The width of the destination */
    int height; /* The height of the destination */
};

/*
 * @brief: Support various input resolution. module parameter is defined here.
 */
typedef enum {
    VIN_D1 = 0,
    VIN_NTSC = VIN_D1,
    VIN_PAL = 1,
    VIN_640x480 = 2,
    VIN_1024x768 = 3,
    VIN_1440x1080 = 4,
    VIN_1280x800 = 5,
    VIN_1280x960 = 6,
    VIN_1280x1024 = 7,
    VIN_1360x768 = 8,
    VIN_720P = 9,
    VIN_1600x1200 = 10,
    VIN_640x360 = 11,
    VIN_1680x1050 = 12,
    VIN_800x600 = 14,
    VIN_1920x1080 = 15,
    VIN_1440x960 = 16,
    VIN_1440x1152 = 17,
    VIN_1440x900 = 18,
    VIN_540x360 = 19,
    VIN_352x240 = 20,
    VIN_352x288 = 21,
    VIN_320x240 = 22,
    VIN_1920x1152 = 23,
    VIN_1280x1080 = 24,
    VIN_NONE
} vin_res_t;

/*
 * @brief: Support various output_type, module parameter is defined here.
 */
typedef enum {
    OUTPUT_TYPE_DAC_D1_0 = 0,
    OUTPUT_TYPE_NTSC_0 = OUTPUT_TYPE_DAC_D1_0,
    OUTPUT_TYPE_PAL_1 = 1,
    OUTPUT_TYPE_CS4954_2 = 2,
    OUTPUT_TYPE_PVI2003A_3 = 3,
    OUTPUT_TYPE_FS453_4 = 4,
    OUTPUT_TYPE_MDIN200_HD_5 = 5,
    OUTPUT_TYPE_MDIN200_D1_6 = 6,
    OUTPUT_TYPE_VGA_1024x768_8 = 8,
    OUTPUT_TYPE_VGA_1280x800_9 = 9,
    OUTPUT_TYPE_VGA_1280x960_10 = 10,
    OUTPUT_TYPE_VGA_1920x1080_11 = 11,
    OUTPUT_TYPE_VGA_1440x900_12 = 12,
    OUTPUT_TYPE_VGA_1280x1024_13 = 13,
    OUTPUT_TYPE_VGA_1360x768_14 = 14,
    OUTPUT_TYPE_VGA_800x600_15 = 15,
    OUTPUT_TYPE_VGA_1600x1200_16 = 16,
    OUTPUT_TYPE_VGA_1680x1050_17 = 17,
    OUTPUT_TYPE_VGA_1280x720_18 = 18,
    OUTPUT_TYPE_CAS_1024x768x30_19 = 19,
    OUTPUT_TYPE_CAT6611_480P_20 = 20,
    OUTPUT_TYPE_CAT6611_576P_21 = 21,
    OUTPUT_TYPE_CAT6611_720P_22 = 22,
    OUTPUT_TYPE_CAT6611_1080P_23 = 23,
    OUTPUT_TYPE_CAT6611_1024x768_24 = 24,
    OUTPUT_TYPE_CAT6611_1024x768P_25 = 25,
    OUTPUT_TYPE_CAT6611_1280x1024P_26 = 26,
    OUTPUT_TYPE_CAT6611_1680x1050P_27 = 27,
    OUTPUT_TYPE_CAT6611_1280x1024_28 = 28,
    OUTPUT_TYPE_CAT6611_1920x1080_29 = 29,
    OUTPUT_TYPE_CAT6611_1440x900P_30 = 30,
    OUTPUT_TYPE_CAS_1280x1024x25_31   = 31,
    OUTPUT_TYPE_CAS_1280x1024x30_32   = 32,
    OUTPUT_TYPE_CAS_1920x1080x30_33   = 33, //8210 cascade
    OUTPUT_TYPE_CAS_1920x1080x25_34   = 34, //8210 cascade
    OUTPUT_TYPE_MDIN240_HD_35 = 35,
    OUTPUT_TYPE_MDIN240_SD_36 = 36,
    OUTPUT_TYPE_MDIN240_1024x768I_38 = 38,
    OUTPUT_TYPE_CAS_1440x960I_40 = 40,
    OUTPUT_TYPE_CAS_1440x1152I_41 = 41,
    OUTPUT_TYPE_SLI10121_480P_42     = 42,
    OUTPUT_TYPE_SLI10121_576P_43     = 43,
    OUTPUT_TYPE_SLI10121_720P_44     = 44,
    OUTPUT_TYPE_SLI10121_1080P_45    = 45,
    OUTPUT_TYPE_SLI10121_1024x768_46 = 46,
    OUTPUT_TYPE_SLI10121_1024x768P_47= 47,
    OUTPUT_TYPE_SLI10121_1280x1024P_48 = 48,
    OUTPUT_TYPE_SLI10121_1680x1050P_49 = 49,
    OUTPUT_TYPE_SLI10121_1280x1024_50  = 50,
    OUTPUT_TYPE_SLI10121_1920x1080_51  = 51,
    OUTPUT_TYPE_SLI10121_1440x900P_52  = 52,
    OUTPUT_TYPE_SLI10121_1280x720_53   = 53,
    OUTPUT_TYPE_MDIN340_720x480_58     = 58,
    OUTPUT_TYPE_MDIN340_720x576_59     = 59,
    OUTPUT_TYPE_MDIN340_480P_60        = 60,
    OUTPUT_TYPE_MDIN340_576P_61        = 61,
    OUTPUT_TYPE_MDIN340_1920x1080P_62  = 62,
    OUTPUT_TYPE_MDIN340_1920x1080_63   = 63,
    OUTPUT_TYPE_LAST
} output_type_t;

/*
 * MACROs
 */
#define FFB_BUFFMT_NONINTERLACE 0
#define FFB_BUFFMT_INTERLACE    1

/*
 * IOCTL
 */
#define FFB_IOC_MAGIC  'f'
#define FFB_GETBUF            _IOR(FFB_IOC_MAGIC, 2, int)
#define FFB_PUTBUF            _IOW(FFB_IOC_MAGIC, 3, int)
#define FFB_SETBUF_FMT        _IOW(FFB_IOC_MAGIC, 4, int)
#define FFB_GETBUFSIZE        _IOR(FFB_IOC_MAGIC, 5, int)
/* hardware cursor position scale */
#define FFB_IOCURSOR_POS_SCAL   _IOW(FFB_IOC_MAGIC, 6, int)

#define FFB_PIP_MODE            _IOW(FFB_IOC_MAGIC, 7, unsigned int)
#define FFB_PIP_INPUT_RES       _IOW(FFB_IOC_MAGIC, 8, unsigned int)
#define FFB_PIP_OUTPUT_TYPE     _IOW(FFB_IOC_MAGIC, 9, unsigned int)
/* upper 16bits: output_type, lower 16bits: input_res */
#define FFB_GET_INPUTRES_OUTPUTTYPE     _IOR(FFB_IOC_MAGIC, 10, unsigned int)

#define FFB_IOSBUFFMT         _IOW(FFB_IOC_MAGIC, 14, unsigned int)
#define FFB_IOGBUFFMT         _IOR(FFB_IOC_MAGIC, 15, unsigned int)
#define FFB_IOCURSOR          _IOWR(FFB_IOC_MAGIC, 16, struct fb_cursor)
#define FFB_IOSRBSWAP         _IOWR(FFB_IOC_MAGIC, 17, int)

/* for CT656 */
#define FFB_BYPASS            _IOW(FFB_IOC_MAGIC, 18, int)
#define FFB_BYPASS_SRC        _IOW(FFB_IOC_MAGIC, 19, int)
#define FFB_BYPASS_INVERT_INPUT_CLK _IOW(FFB_IOC_MAGIC, 20, int)

/* VBI */
#define CCIR_VBI_LINE_NUM     _IOW(FFB_IOC_MAGIC, 21, unsigned int) /* start blanking line number for vbi */
#define CCIR_VBI_LINE_HEIGHT  _IOW(FFB_IOC_MAGIC, 22, unsigned int) /* number of lines */
#define CCIR_VBI_INS_ENABLE   _IOW(FFB_IOC_MAGIC, 23, unsigned int)

#define FFB_LCD_SCALAR        _IOWR(FFB_IOC_MAGIC, 24, int)
#define COLOR_KEY1            _IOWR(FFB_IOC_MAGIC, 25, int)
#define COLOR_KEY1_EN         _IOWR(FFB_IOC_MAGIC, 26, int)
#define COLOR_KEY2            _IOWR(FFB_IOC_MAGIC, 27, int)
#define COLOR_KEY2_EN         _IOWR(FFB_IOC_MAGIC, 28, int)

/* Frame rate control */
#define FFB_FRAME_CTL         _IOWR(FFB_IOC_MAGIC, 29, int)

/* VBI offset from each frame buffer start */
#define FFB_VBI_OFFSET        _IOR(FFB_IOC_MAGIC, 30, int)

/* Old structure use.
 */
#define FLCD_GET_DATA_SEP   0x46db
#define FLCD_SET_FB_NUM     0x46dd
#define FLCD_SET_SPECIAL_FB 0x46df

#define FFB_IOC_MAGIC_CMP  'c'

#define FFB_GET_DATA_SEP      _IOW(FFB_IOC_MAGIC_CMP, 10, struct flcd_data)
#define FFB_SET_FB_NUM        _IOW(FFB_IOC_MAGIC_CMP, 11, unsigned int)
#define FFB_SET_SPECIAL_FB    _IOW(FFB_IOC_MAGIC_CMP, 12, unsigned int)
#define FFB_SET_PALETTE       _IOW(FFB_IOC_MAGIC_CMP, 13, struct palette)

struct edid_table {
    /* when bit is set, it means the resoultion is support */
    unsigned int edid[(OUTPUT_TYPE_LAST + 7) / 8];
};

#define FFB_GET_EDID        _IOR(FFB_IOC_MAGIC_CMP, 14, struct edid_table)
#define FFB_SET_EDID        _IOW(FFB_IOC_MAGIC_CMP, 15, unsigned int)

/* a bitmap to indicates which LCDs have the same GUI content */
#define FFB_GUI_CLONE  	    _IOW(FFB_IOC_MAGIC_CMP, 16, unsigned int)
/* for dual communication only */
#define FFB_FB0_INPUT       _IOW(FFB_IOC_MAGIC_CMP, 17, unsigned int)
#define FFB_FB1_INPUT       _IOW(FFB_IOC_MAGIC_CMP, 18, unsigned int)
#define FFB_FB2_INPUT       _IOW(FFB_IOC_MAGIC_CMP, 19, unsigned int)

#define FFB_GUI_CLONE_ADJUST _IOW(FFB_IOC_MAGIC_CMP, 20, struct gui_clone_adjust)

/* used to copy an image from src_pic_pa to LCDx gui
 */
struct gui_clone2 {
    unsigned int src_pic_pa;     /* source physical address */
    unsigned int src_width;
    unsigned int src_height;
    unsigned int dst_lcd_bitmap; /* BITMAP of LCDC. bit0 for LCD0, bit1 for LCD1, ..... */
};
#define FFB_GUI_CLONE2      _IOW(FFB_IOC_MAGIC_CMP, 21, struct gui_clone2)

/*
 * OSD Fonts
 */
struct fosd_string {
	unsigned int Str_row;
	//Add By Ken.Hsieh on 2007.11.29 for specific column and length color
	unsigned int column;
	unsigned int len;
	unsigned int display_mode;
	unsigned int fg_color;
	unsigned int bg_color;
	unsigned char Str_OSD[64];
};

struct fosd_data {
	unsigned int HPos;
	unsigned int VPos;
	unsigned int HDim;
	unsigned int VDim;
	unsigned int transparent_level;
	unsigned int HScal;
	unsigned int VScal;
	struct fosd_string Str_Data[31];
};

struct fiosd_char {
	unsigned char font_index;
	unsigned char fbitmap[32];
};

#define OSD_MAX_CHAR_MAX    256

struct fiosd_charmap {
	int map[OSD_MAX_CHAR_MAX];
};

#define FFBOSD_IOC_MAGIC  's'

#define FOSD_SETPOS    		    _IOW(FFBOSD_IOC_MAGIC, 1, struct fosd_data)
#define FOSD_SETDIM    		    _IOW(FFBOSD_IOC_MAGIC, 2, struct fosd_data)
#define FOSD_SETSCAL    	    _IOW(FFBOSD_IOC_MAGIC, 3, struct fosd_data)
#define FLCD_SET_TRANSPARENT    _IOW(FFBOSD_IOC_MAGIC, 4, struct fosd_data)
#define FLCD_SET_STRING    	    _IOW(FFBOSD_IOC_MAGIC, 5, struct fosd_data)
#define FOSD_ON    		        _IOW(FFBOSD_IOC_MAGIC, 6, struct fosd_data)
#define FOSD_OFF    		    _IOW(FFBOSD_IOC_MAGIC, 7, struct fosd_data)
//Add By Ken.Hsieh
#define FOSD_QUERY_CHARMAP    	_IOWR(FFBOSD_IOC_MAGIC, 8, struct fiosd_charmap)
#define FOSD_RMCHAR    		    _IOWR(FFBOSD_IOC_MAGIC, 9, int)
#define FOSD_SET_CHAR    	    _IOWR(FFBOSD_IOC_MAGIC, 10, struct fiosd_charmap)

#ifdef __KERNEL__

/*
 * @This function configure the output_type
 *
 * @function int pip_config_edid(output_type_t l_output_type);
 * @param l_output_type indicates the output_type
 * @return 0 on success, <0 on error
 */
int pip_config_edid(output_type_t l_output_type);

/*
 * @This function get the edit table supported by LCD
 *
 * @function int pip_get_edid(unsigned int *edid_table);
 * @param edid indicates the edid table. When the corresponding bit in output_type_t
 *                  in the edid array, it means available.
 * @return 0 on success, <0 on error
 */
int pip_get_edid(struct edid_table *edid);


#endif /* __KERNEL__ */

#endif /* __LCD_FB_H__ */

