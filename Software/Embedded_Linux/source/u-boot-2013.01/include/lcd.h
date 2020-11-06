#ifndef _LCD_H_
#define _LCD_H_

typedef enum {
    VIN_D1_NTSC,
    VIN_D1_PAL,
    VIN_1280x720,
    VIN_1920x1080,
    VIN_1024x768
} lcd_vin_t;

/* combination: MSB 16bits: codec, LSB 16bit: output format */
typedef enum {
    OUTPUT_CCIR656_NTSC = 0x1,
    OUTPUT_CCIR656_PAL,
    OUTPUT_BT1120_720P,
    OUTPUT_BT1120_1080P,
    /* The following is the VESA */
    OUTPUT_VESA = 0x1000,
    OUTPUT_VGA_1280x720 = OUTPUT_VESA,
    OUTPUT_VGA_1920x1080,
    OUTPUT_VGA_1024x768,
} lcd_output_t;

typedef enum {
    INPUT_FMT_YUV422 = 0,
    INPUT_FMT_RGB888,
    INPUT_FMT_RGB565
} input_fmt_t;

typedef enum {
    LCD_ID = 0,
    LCD0_ID = LCD_ID,
    LCD1_ID,
    LCD2_ID
} lcd_idx_t;

#endif /* _LCD_H_ */

