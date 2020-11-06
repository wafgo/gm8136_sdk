/**
 * @file lens_td9533t.h
 *  header file of lens_td9533t.c.
 * Copyright (C) 2010 GM Corp. (http://www.grain-media.com)
 *
 */
#ifndef __LENS_TD9533T_H
#define __LENS_TD9533T_H

#include "isp_input_inf.h"

#define TD9533T_LENS

#define lens_zoom_ini           0
#define lens_zoom_opening       1
#define lens_zoom_open_finish       2

#define lens_focus_ini          0
#define lens_focus_opening      1
#define lens_focus_open_finish      2

#define SPI_RENESAS_RESET_PIN     GPIO_ID(0, 13)
#define SPI_RENESAS_DAT_PIN     GPIO_ID(0, 12)
#define SPI_RENESAS_CLK_PIN     GPIO_ID(0, 11)
#define SPI_RENESAS_CS0_PIN     GPIO_ID(0, 10)

#define MOTOR_REG_FOCUS_ID      0x00    //channel 1,2   //TVT
#define MOTOR_REG_ZOOM_ID       0x10    //channel 3,4   //TVT

#define MOT_FPI_ANODE_PIN       12
#define MOT_ZPI_ANODE_PIN       13

#define MOT_ZPI_PIN         10
#define MOT_ZEXT_PIN            GPIO_ID(1, 31)
#define MOT_ZMOB_PIN            GPIO_ID(1, 30)


#define MOT_FPI_PIN         11
#define MOT_FEXT_PIN            GPIO_ID(1, 29)
#define MOT_FMOB_PIN            GPIO_ID(0, 9)

#define MOTOR_ZOOM_OFF                       0
#define MOTOR_ZOOM_ON                        1

#define MOTOR_FOCUS_OFF                      1
#define MOTOR_FOCUS_ON                       0


#define MOTOR_ZOOM_MOVE_REF_SPEED         129
#define MOTOR_ZOOM_MOVE_SPEED             60

#define MOTOR_ZOOM_MOVE_PR0_FACTOR        20
#define MOTOR_ZOOM_MOVE_PR1_FACTOR        40
#define MOTOR_ZOOM_MOVE_PR2_FACTOR        60
#define MOTOR_ZOOM_MOVE_PR3_FACTOR        80
#define MOTOR_ZOOM_MOVE_PR4_FACTOR        100


#define MOTOR_REG_PRE_EXCIT                  0x0A
#define MOTOR_REG_POST_EXCIT                 0x0B
#define MOTOR_EXCIT(val)                     (((val*100/82) & 0x3F) << 4)
#define MOTOR_EXCIT_FOREVER                  63


#define MOTOR_ZOOM_MOVE_NOP_STAGE   1

#define MOTOR_POS_EN                         1
#define MOTOR_POS_DIS                        0


#define MOTOR_ZOOM_OPEN_INIT_POSITON         0

#define MOTOR_ZOOM_MOVE_IN_DIRECTION         1
#define MOTOR_ZOOM_MOVE_OUT_DIRECTION        0

#define MOTOR_FOCUS_OPEN_INIT_POSITON        0

#define MOTOR_FOCUS_INFI_DIRECTION           1
#define MOTOR_FOCUS_NEAR_DIRECTION           0


#define MOTOR_REG_NOP_L                      0x01
#define MOTOR_NOP_L(val)                     (val & 0x003FF)

#define MOTOR_REG_NOP_H                      0x02
#define MOTOR_NOP_H(val)                     ((val & 0xFFC00) >> 10)




#define MOTOR_REG_FOCUS_PI                   1
#define MOTOR_REG_ZOOM_PI                    2


#define MOTOR_MOVE_PRR                    MOTOR_PRR_16
#define MOTOR_MOVE_PR                     16

#define MOTOR_REG_NOP_STAGE                  0x05

#define MOTOR_REG_PR0                        0x03
#define MOTOR_REG_PR1                        0x06
#define MOTOR_REG_PR2                        0x07
#define MOTOR_REG_PR3                        0x08
#define MOTOR_REG_PR4                        0x09


#define MOTOR_REG_CC                         0x00
#define MOTOR_DIR(val)                       ((val & 0x01) << 9)
#define MOTOR_DIR_FORWARD                    (0 << 9)
#define MOTOR_DIR_REVERSE                    (1 << 9)
#define MOTOR_MODE_MICRO_256               (0 << 6)
#define MOTOR_MODE_12_PHASE_100              (1 << 6)
#define MOTOR_MODE_12_PHASE_70               (2 << 6)
#define MOTOR_MODE_22_PHASE                  (3 << 6)
#define MOTOR_MODE_MICRO_512               (4 << 6)
#define MOTOR_MODE_MICRO_1024              (5 << 6)
#define MOTOR_NO_ACC_NO_DEC                  (0 << 4)
#define MOTOR_NO_ACC_DEC                     (1 << 4)
#define MOTOR_ACC_NO_DEC                     (2 << 4)
#define MOTOR_ACC_DEC                        (3 << 4)
#define MOTOR_POS_INIT(val)                  ((val & 0x07) << 1)
#define MOTOR_POS_EN                         1
#define MOTOR_POS_DIS                        0


#define MOTOR_REG_PRR_SP                    0x04
#define MOTOR_PRR_2048                       (0 << 6)
#define MOTOR_PRR_1024                       (1 << 6)
#define MOTOR_PRR_512                        (2 << 6)
#define MOTOR_PRR_256                        (3 << 6)
#define MOTOR_PRR_128                        (4 << 6)
#define MOTOR_PRR_64                         (5 << 6)
#define MOTOR_PRR_32                         (6 << 6)
#define MOTOR_PRR_16                         (7 << 6)
#define MOTOR_PRR_8                          (8 << 6)
#define MOTOR_PRR_4                          (9 << 6)
#define MOTOR_PRR_2                          (10 << 6)
#define MOTOR_PRR_2                          (10 << 6)
#define MOTOR_SP_12_PHASE                (1 << 0)
#define MOTOR_SP_22_PHASE                (2 << 0)

#define MOTOR_REG_PREP_EXCITE_TIME                    0x0A
#define MOTOR_REG_POST_EXCITE_TIME                    0x0B
#define MOTOR_EXC_TIME_983              (3<<6)


#define MOTOR_REG_EN                         0x0F
#define MOTOR_OPER_EN                        (1 << 9)
#define MOTOR_OPER_DIS                       (0 << 9)
#define MOTOR_HOLD                           (1 << 8)
#define MOTOR_RELEASE                        (0 << 8)
#define MOTOR_EXCIT_ON                       (1 << 7)
#define MOTOR_EXCIT_OFF                      (0 << 7)
#define MOTOR_EN_DIR(val)                       ((val & 0x01) << 6)
#define MOTOR_C_OPER_EN                      (1 << 5)
#define MOTOR_C_OPER_DIS                     (0 << 5)
#define MOTOR_VD_STOP_EN                     (1 << 0)


#define MOTOR_REG_VC                       0x0E   //TVT
#define MOTOR_VC_X1(val)                   (((0x3F - val/8)) << 4) //TVT
#define MOTOR_VC_X2(val)                   ((((0x3F - val/16)) << 4) | (1 << 3))    //TVT

#define MOTOR_REG_PI                         0x32
#define MOTOR_PI_MASK(num)                   (1 << (10 - num))
#define MOTOR_PI_EN(num)                     (1 << (10 - num))
#define MOTOR_PI_DIS(num)                    (0 << (10 - num))



#define MOTOR_REG_PS                         0x34   //TVT
#define MOTOR_PS_EN                          (1 << 9)
#define MOTOR_PS_DIS                         (0 << 9)
#define MOTOR_PS12_POW_ON                    (1 << 8)
#define MOTOR_PS12_POW_OFF                   (0 << 8)
#define MOTOR_PS34_POW_ON                    (3 << 6)
#define MOTOR_PS34_POW_OFF                   (0 << 6)

typedef struct _LENS_Z_F_TABLE
{
    int zoom_pos;
    int focus_min;
    int focus_max;

}LENS_Z_F_TAB, *P_LENS_Z_F_TAB;

extern int lens_dev_construct(lens_dev_t *pdev);
extern void lens_dev_deconstruct(lens_dev_t *pdev);

#endif /* __LENS_TD9533T_H */
