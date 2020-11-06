/**
 * @file sen_mn34220.c
 * Panasonic mn34220 sensor driver
 *
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/io.h>

#define PFX "sen_mn34220"
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1920
#define DEFAULT_IMAGE_HEIGHT    1080
#define DEFAULT_XCLK            27000000

static ushort sensor_w = DEFAULT_IMAGE_WIDTH;
module_param(sensor_w, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_w, "sensor image width");

static ushort sensor_h = DEFAULT_IMAGE_HEIGHT;
module_param(sensor_h, ushort, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_h, "sensor image height");

static uint sensor_xclk = DEFAULT_XCLK;
module_param(sensor_xclk, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sensor_xclk, "sensor external clock frequency");

static uint flip = 0;
module_param(flip, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(flip, "sensor flip function");

static uint ch_num = 4;
module_param(ch_num, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ch_num, "channel number");

static uint fps = 60;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "channel number");

//=============================================================================
// global
//=============================================================================
#define SENSOR_NAME         "mn34220"
#define SENSOR_MAX_WIDTH    1956
#define SENSOR_MAX_HEIGHT   1092

#define H_CYCLE 2200
#define V_CYCLE 1125

int vert_cycle = V_CYCLE;

static sensor_dev_t *g_psensor = NULL;

typedef struct sensor_info {
    bool is_init;
    u32 img_w;
    u32 img_h;
    int flip;
    u32 t_row;          // unit is 1/10 us
} sensor_info_t;

typedef struct sensor_reg {
    u16 addr;
    u16 val;
} sensor_reg_t;

static sensor_reg_t sensor_init_regs[] = {
#if 0   
    {0x300E, 0x0001},                       
    {0x300F, 0x0000},                       
    {0x0304, 0x0000},                       
    {0x0305, 0x0001},//27MHz            
    {0x0306, 0x0000},                       
    {0x0307, 0x0021},//27MHz            
    {0x3000, 0x0000},                       
    {0x3001, 0x0003},                       
    {0x0112, 0x000C},                       
    {0x0113, 0x000C},                       
    {0x3005, 0x0064},                       
    {0x3007, 0x0014},                       
    {0x3008, 0x0091},// 1ch4port        
    {0x300B, 0x0000},                       
    {0x3018, 0x0043},                       
    {0x3019, 0x0010},                       
    {0x301A, 0x00B9},                       
    {0x301B, 0x0000},                       
    {0x301C, 0x002B},                       
    {0x301D, 0x001B},                       
    {0x301E, 0x000D},                       
    {0x301F, 0x001F},                       
    {0x3000, 0x0000},                       
    {0x3001, 0x0053},                       
    {0x300E, 0x0000},                       
    {0x300F, 0x0000},                       
    {0x0202, 0x0004},                       
    {0x0203, 0x0063},                       
    {0x3036, 0x0000},                       
    {0x3039, 0x002E},                       
    {0x3058, 0x000F},                       
    {0x306E, 0x000C},                       
    {0x306F, 0x0000},                       
    {0x3074, 0x0001},                       
    {0x3098, 0x0000},                       
    {0x3099, 0x0000},                       
    {0x309A, 0x0001},                       
    {0x3104, 0x0004},                       
    {0x3106, 0x0000},                       
    {0x3107, 0x00C0},                       
    {0x3141, 0x0040},                       
    {0x3143, 0x0002},                       
    {0x3144, 0x0002},                       
    {0x3145, 0x0002},                       
    {0x3146, 0x0000},                       
    {0x3147, 0x0002},                       
    {0x314A, 0x0001},                       
    {0x314B, 0x0002},                       
    {0x314C, 0x0002},                       
    {0x314D, 0x0002},                       
    {0x314E, 0x0001},                       
    {0x314F, 0x0002},                       
    {0x3150, 0x0002},                       
    {0x3152, 0x0004},                       
    {0x3153, 0x00E3},                       
    {0x316F, 0x00C6},                       
    {0x3175, 0x0080},                       
    {0x318E, 0x0020},                       
    {0x318F, 0x0070},                       
    {0x3196, 0x0008},                       
    {0x3243, 0x00D7},                       
    {0x3247, 0x0038},                       
    {0x3248, 0x0003},                       
    {0x3249, 0x00E2},                       
    {0x324A, 0x0030},                       
    {0x324B, 0x0018},                       
    {0x324C, 0x0002},                       
    {0x3253, 0x00DE},                       
    {0x3259, 0x0068},                       
    {0x3272, 0x0046},                       
    {0x3280, 0x0030},                       
    {0x3288, 0x0001},                       
    {0x330E, 0x0005},                       
    {0x3310, 0x0002},                       
    {0x3315, 0x001F},                       
    {0x332C, 0x0002},                       
    {0x3339, 0x0002},                       
    {0x3000, 0x0000},                       
    {0x3001, 0x00D3},                       
    {0x0100, 0x0001},                       
    {0x0101, 0x0000}, 
#endif
#if 1    
    {0x300E, 0x0001},                 
    {0x300F, 0x0000},                 
    {0x0304, 0x0000},                 
    {0x0305, 0x0001},                 
    {0x0306, 0x0000},                 
    {0x0307, 0x0021},                 
    {0x3000, 0x0000},                 
    {0x3001, 0x0003},                 
    {0x0112, 0x000C},                 
    {0x0113, 0x000C},                 
    {0x3005, 0x0064},                 
    {0x3008, 0x0091}, //#for 1ch4port  
    {0x300B, 0x0000},                 
    {0x3018, 0x0043},                 
    {0x3019, 0x0010},                 
    {0x301A, 0x00B9},                 
    {0x301B, 0x0000},                 
    {0x301C, 0x002B},                 
    {0x301D, 0x001B},                 
    {0x301E, 0x000D},                 
    {0x301F, 0x001F},                 
    {0x3000, 0x0000},                 
    {0x3001, 0x0053},                 
    {0x300E, 0x0000},                 
    {0x300F, 0x0000},                 
    {0x0202, 0x0004},                 
    {0x0203, 0x0063},                 
    {0x3036, 0x0000},                 
    {0x3039, 0x002E},                 
    {0x3058, 0x000F},                 
    {0x306E, 0x000C},                 
    {0x306F, 0x0000},                 
    {0x3074, 0x0001},                 
    {0x3098, 0x0000},                 
    {0x3099, 0x0000},                 
    {0x309A, 0x0001},                 
    {0x3104, 0x0004},                 
    {0x3106, 0x0000},                 
    {0x3107, 0x00C0},                 
    {0x3141, 0x0040},                 
    {0x3143, 0x0002},                 
    {0x3144, 0x0002},                 
    {0x3145, 0x0002},                 
    {0x3146, 0x0000},                 
    {0x3147, 0x0002},                 
    {0x314A, 0x0001},                 
    {0x314B, 0x0002},                 
    {0x314C, 0x0002},                 
    {0x314D, 0x0002},                 
    {0x314E, 0x0001},                 
    {0x314F, 0x0002},                 
    {0x3150, 0x0002},                 
    {0x3152, 0x0004},                 
    {0x3153, 0x00E3},                 
    {0x316F, 0x00C6},                 
    {0x3175, 0x0080},                 
    {0x318E, 0x0020},                 
    {0x318F, 0x0070},                 
    {0x3196, 0x0008},                 
    {0x3243, 0x00D7},                 
    {0x3246, 0x0001},                 
    {0x3247, 0x0079},                 
    {0x324A, 0x0030},                 
    {0x324B, 0x0018},                 
    {0x324C, 0x0002},                 
    {0x3253, 0x00DE},                 
    {0x3258, 0x0001},                 
    {0x3259, 0x0049},                 
    {0x3272, 0x0046},                 
    {0x3280, 0x0030},                 
    {0x3288, 0x0001},                 
    {0x330E, 0x0005},                 
    {0x3310, 0x0002},                 
    {0x3315, 0x001F},                 
    {0x332C, 0x0002},                 
    {0x3339, 0x0002},                 
    {0x3000, 0x0000},                 
    {0x3001, 0x00D3},                 
    {0x0100, 0x0001},                 
    {0x0101, 0x0000}, 
#endif                                                  
};  
    
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))
    
typedef struct gain_set {
    u16 analog;
    u16 digit;
    u32 gain_x;
} gain_set_t;
      
static const gain_set_t gain_table[] = {
    {0x0100, 0x0100, 64   }, {0x0104, 0x0100, 67   }, {0x0108, 0x0100, 70   }, 
    {0x010C, 0x0100, 73   }, {0x0110, 0x0100, 76   }, {0x0114, 0x0100, 79   }, 
    {0x0118, 0x0100, 83   }, {0x011C, 0x0100, 87   }, {0x0120, 0x0100, 90   }, 
    {0x0124, 0x0100, 94   }, {0x0128, 0x0100, 99   }, {0x012C, 0x0100, 103  }, 
    {0x0130, 0x0100, 107  }, {0x0134, 0x0100, 112  }, {0x0138, 0x0100, 117  }, 
    {0x013C, 0x0100, 122  }, {0x0140, 0x0100, 128  }, {0x0144, 0x0100, 133  }, 
    {0x0148, 0x0100, 139  }, {0x014C, 0x0100, 145  }, {0x0150, 0x0100, 152  }, 
    {0x0154, 0x0100, 159  }, {0x0158, 0x0100, 165  }, {0x015C, 0x0100, 173  }, 
    {0x0160, 0x0100, 180  }, {0x0164, 0x0100, 188  }, {0x0168, 0x0100, 197  }, 
    {0x016C, 0x0100, 205  }, {0x0170, 0x0100, 214  }, {0x0174, 0x0100, 224  }, 
    {0x0178, 0x0100, 234  }, {0x017C, 0x0100, 244  }, {0x0180, 0x0100, 255  }, 
    {0x0184, 0x0100, 266  }, {0x0188, 0x0100, 278  }, {0x018C, 0x0100, 290  }, 
    {0x0190, 0x0100, 303  }, {0x0194, 0x0100, 316  }, {0x0198, 0x0100, 330  }, 
    {0x019C, 0x0100, 345  }, {0x01A0, 0x0100, 360  }, {0x01A4, 0x0100, 376  }, 
    {0x01A8, 0x0100, 392  }, {0x01AC, 0x0100, 410  }, {0x01B0, 0x0100, 428  }, 
    {0x01B4, 0x0100, 447  }, {0x01B8, 0x0100, 466  }, {0x01BC, 0x0100, 487  }, 
    {0x01C0, 0x0100, 508  }, {0x01C4, 0x0100, 531  }, {0x01C8, 0x0100, 554  }, 
    {0x01CC, 0x0100, 579  }, {0x01D0, 0x0100, 604  }, {0x01D4, 0x0100, 631  }, 
    {0x01D8, 0x0100, 659  }, {0x01DC, 0x0100, 688  }, {0x01E0, 0x0100, 718  }, 
    {0x01E4, 0x0100, 750  }, {0x01E8, 0x0100, 783  }, {0x01EC, 0x0100, 818  }, 
    {0x01F0, 0x0100, 853  }, {0x01F4, 0x0100, 892  }, {0x01F8, 0x0100, 930  }, 
    {0x01FC, 0x0100, 972  }, {0x0200, 0x0100, 1014 }, {0x0204, 0x0100, 1060 }, 
    {0x0208, 0x0100, 1106 }, {0x020C, 0x0100, 1155 }, {0x0210, 0x0100, 1206 }, 
    {0x0214, 0x0100, 1259 }, {0x0218, 0x0100, 1314 }, {0x021C, 0x0100, 1373 }, 
    {0x0220, 0x0100, 1433 }, {0x0224, 0x0100, 1497 }, {0x0228, 0x0100, 1562 }, 
    {0x022C, 0x0100, 1632 }, {0x0230, 0x0100, 1703 }, {0x0234, 0x0100, 1779 }, 
    {0x0238, 0x0100, 1856 }, {0x023C, 0x0100, 1939 }, {0x0240, 0x0100, 2024 }, 
    {0x0240, 0x0104, 2114 }, {0x0240, 0x0108, 2206 }, {0x0240, 0x010C, 2305 }, 
    {0x0240, 0x0110, 2405 }, {0x0240, 0x0114, 2513 }, {0x0240, 0x0118, 2622 }, 
    {0x0240, 0x011C, 2740 }, {0x0240, 0x0120, 2859 }, {0x0240, 0x0124, 2987 }, 
    {0x0240, 0x0128, 3117 }, {0x0240, 0x012C, 3256 }, {0x0240, 0x0130, 3398 }, 
    {0x0240, 0x0134, 3550 }, {0x0240, 0x0138, 3704 }, {0x0240, 0x013C, 3870 }, 
    {0x0240, 0x0140, 4038 }, {0x0240, 0x0144, 4219 }, {0x0240, 0x0148, 4402 }, 
    {0x0240, 0x014C, 4599 }, {0x0240, 0x0150, 4799 }, {0x0240, 0x0154, 5014 }, 
    {0x0240, 0x0158, 5232 }, {0x0240, 0x015C, 5466 }, {0x0240, 0x0160, 5704 }, 
    {0x0240, 0x0164, 5959 }, {0x0240, 0x0168, 6218 }, {0x0240, 0x016C, 6497 }, 
    {0x0240, 0x0170, 6779 }, {0x0240, 0x0174, 7082 }, {0x0240, 0x0178, 7391 }, 
    {0x0240, 0x017C, 7721 }, {0x0240, 0x0180, 8057 }, {0x0240, 0x0184, 8417 }, 
    {0x0240, 0x0188, 8784 }, {0x0240, 0x018C, 9177 }, {0x0240, 0x0190, 9576 }, 
    {0x0240, 0x0194, 10004}, {0x0240, 0x0198, 10440}, {0x0240, 0x019C, 10906}, 
    {0x0240, 0x01A0, 11381}, {0x0240, 0x01A4, 11890}, {0x0240, 0x01A8, 12407}, 
    {0x0240, 0x01AC, 12962}, {0x0240, 0x01B0, 13526}, {0x0240, 0x01B4, 14131}, 
    {0x0240, 0x01B8, 14746}, {0x0240, 0x01BC, 15406}, {0x0240, 0x01C0, 16076}, 
    {0x0240, 0x01C4, 16795}, {0x0240, 0x01C8, 17526}, {0x0240, 0x01CC, 18310}, 
    {0x0240, 0x01D0, 19106}, {0x0240, 0x01D4, 19961}, {0x0240, 0x01D8, 20830}, 
    {0x0240, 0x01DC, 21761}, {0x0240, 0x01E0, 22708}, {0x0240, 0x01E4, 23724}, 
    {0x0240, 0x01E8, 24756}, {0x0240, 0x01EC, 25863}, {0x0240, 0x01F0, 26989}, 
    {0x0240, 0x01F4, 28196}, {0x0240, 0x01F8, 29423}, {0x0240, 0x01FC, 30738}, 
    {0x0240, 0x0200, 32076}, {0x0240, 0x0204, 33510}, {0x0240, 0x0208, 34969}, 
    {0x0240, 0x020C, 36533}, {0x0240, 0x0210, 38122}, {0x0240, 0x0214, 39827}, 
    {0x0240, 0x0218, 41560}, {0x0240, 0x021C, 43419}, {0x0240, 0x0220, 45309}, 
    {0x0240, 0x0224, 47335}, {0x0240, 0x0228, 49395}, {0x0240, 0x022C, 51604}, 
    {0x0240, 0x0230, 53849}, {0x0240, 0x0234, 56257}, {0x0240, 0x0238, 58706}, 
    {0x0240, 0x023C, 61331}, {0x0240, 0x0240, 64000}
};

#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

#define MIN(x,y) ((x)<(y) ? (x) : (y))
#define MAX(x,y) ((x)>(y) ? (x) : (y))

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            (0x6C >> 1)
#include "i2c_common.c"


//============================================================================
// internal functions
//=============================================================================
static int read_reg(u32 addr, u32 *pval)
{
    struct i2c_msg  msgs[2];
    unsigned char   tmp[2], tmp2[2];

    tmp[0]        = (addr >> 8) & 0xFF;
    tmp[1]        = addr & 0xFF;
    msgs[0].addr  = I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len   = 2;
    msgs[0].buf   = tmp;

    tmp2[0]       = 0;
    msgs[1].addr  = I2C_ADDR;
    msgs[1].flags = 1;
    msgs[1].len   = 1;
    msgs[1].buf   = tmp2;

    if (sensor_i2c_transfer(msgs, 2))
        return -1;

    *pval = tmp2[0];
    return 0;
}

static int write_reg(u32 addr, u32 val)
{
    struct i2c_msg  msgs;
    unsigned char   buf[3];

    buf[0]     = (addr >> 8) & 0xFF;
    buf[1]     = addr & 0xFF;
    buf[2]     = val & 0xFF;
    msgs.addr  = I2C_ADDR;
    msgs.flags = 0;
    msgs.len   = 3;
    msgs.buf   = buf;

    return sensor_i2c_transfer(&msgs, 1);
}

static u32 get_pclk(u32 xclk)
{
    u32 pclk;
    
    //if(g_psensor->fps > 30)
        pclk = 74250000*2;
    //else
        //pclk = 74250000;

    isp_info("LVDS Pixel clock %d\n", pclk);

    return pclk;
}

static void adjust_blanking(int fps)
{ 
    vert_cycle = g_psensor->pclk / (H_CYCLE * fps);
    //isp_info("fps=%d, vert_cycle=%d\n", fps, vert_cycle);
    write_reg(0x0340, vert_cycle >> 8);
    write_reg(0x0341, vert_cycle);   
}

static void calculate_t_row(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    pinfo->t_row = H_CYCLE * 10000 / (g_psensor->pclk / 1000);

    //isp_info("t_row=%d pclk=%d\n",pinfo->t_row,g_psensor->pclk);
}

static int set_size(u32 width, u32 height)
{
    int ret = 0;
    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("Exceed maximum sensor resolution!\n");
        return -1;
    }

    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
      
    g_psensor->out_win.width = 1956;   
    g_psensor->out_win.height = 1108;   
        
    adjust_blanking(g_psensor->fps);
    calculate_t_row();
    
    g_psensor->img_win.x = (4+((1952 - width) >> 1)) &~BIT0;    
    g_psensor->img_win.y = (16+((1092 - height) >> 1)) &~BIT0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
   
    return ret;
}

static u32 get_exp(void)
{
    u32 lines, lef_h, lef_l, r_lef, coarse_h, coarse_l, coarse;

    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
   
    if (!g_psensor->curr_exp) {

        read_reg(0x3122, &lef_h);//MSB
        read_reg(0x3123, &lef_l);//LSB
        r_lef = (lef_h << 8) | lef_l;
        
        read_reg(0x0202, &coarse_h);//MSB
        read_reg(0x0203, &coarse_l);//LSB
        coarse = (coarse_h << 8) | coarse_l;
                
        // exposure = r_lef - 1(V) + r_coarse_integration_time(H)
        lines = (r_lef * vert_cycle) - coarse;
        g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;
    }
    
    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    u32 lines, tg_lcf;   

    lines = ((exp * 1000)+pinfo->t_row/2) / pinfo->t_row;
    lines = MAX(1, lines);

    //isp_info("Before:exp=%u,t_row=%d,  lines=%d \n",exp,pinfo->t_row,  lines);       
      
    if (lines >= vert_cycle) {
        tg_lcf = lines / vert_cycle;
        lines -= (tg_lcf * vert_cycle);
    } else {
        tg_lcf = 0;
    }
  
    write_reg(0x3122, tg_lcf >> 8);
    write_reg(0x3123, tg_lcf & 0xFF);
    write_reg(0x0202, lines >> 8);
    write_reg(0x0203, lines & 0xFF);
    lines += (tg_lcf * vert_cycle);
    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;
    
    //isp_info("tg_lcf =%d, lines=%d ,curr_exp=%d \n",tg_lcf, lines , g_psensor->curr_exp);
  
    return ret;
}

static u32 get_gain(void)
{
    if (g_psensor->curr_gain == 0) 
       g_psensor->curr_gain = gain_table[0].gain_x;  

    return g_psensor->curr_gain;  
}

static int set_gain(u32 gain)
{

    int ret = 0;
    u32 ang_x, dig_x, ang_h, ang_l, dig_h, dig_l;
    int i;

    // search most suitable gain into gain table
    ang_x = gain_table[0].analog;
    dig_x = gain_table[0].digit;

    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].gain_x > gain)
            break;
    }
    if (i > 0) {
        ang_x = gain_table[i-1].analog;
        dig_x = gain_table[i-1].digit;
    }
    
    ang_h = ang_x >> 8;
    ang_l = ang_x & 0xFF;
    
    dig_h = dig_x >> 8;
    dig_l = dig_x & 0xFF;

    write_reg(0x0204, ang_h);//MSB_ang
    write_reg(0x0205, ang_l);//LSB_ang
    
    write_reg(0x3108, dig_h);//MSB_dig
    write_reg(0x3109, dig_l);//LSB_dig
    
    g_psensor->curr_gain = gain_table[i-1].gain_x;
    
    //isp_info("set_gain %d, %d, 0x%x, 0x%x, %d\n", gain, i-1, ang_x, dig_x, g_psensor->curr_gain);

    return ret;  
}

static int get_flip(void)
{
    u32 reg;

    read_reg(0x0101, &reg);

    return (reg & BIT1) ? 1 : 0;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg;    
    
    read_reg(0x0101, &reg);//Vertical flipread out mode (V-Flip) 1¡GV-Flip 0: no flip[1]
    if (enable)
        reg |= BIT1;
    else
        reg &=~BIT1;
    write_reg(0x0101, reg);
    pinfo->flip = enable;

    return 0;
}
static int set_fps(int fps)
{
    	adjust_blanking(fps);
    	g_psensor->fps = fps;
		
    	//isp_info("g_psensor->fps=%d\n", g_psensor->fps);

	return 0;
}

static int get_property(enum SENSOR_CMD cmd, unsigned long arg)
{
    int ret = 0;

    switch (cmd) {       
        case ID_FLIP:
            *(int*)arg = get_flip();
            break;
        
        case ID_FPS:
            *(int*)arg = g_psensor->fps;
            break;

        default:
            ret = -EFAULT;
            break;
    }

    return ret;
}

static int set_property(enum SENSOR_CMD cmd, unsigned long arg)
{
    int ret = 0;

    switch (cmd) {        
        case ID_FLIP:
            set_flip((int)arg);
            break;

        case ID_FPS:
            set_fps((int)arg);
            break;
        default:
            ret = -EFAULT;
            break;
    }

    return ret;
}

static int init(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    int ret = 0;
    int i;
   
    if (pinfo->is_init)
        return 0;   
   
    for (i=0; i<NUM_OF_INIT_REGS; i++) {
        if (sensor_init_regs[i].addr == 0xFFFF) {            
            mdelay(sensor_init_regs[i].val);
        } else {     
            if (write_reg(sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
                isp_err("%s init fail!!", SENSOR_NAME);
                return -EINVAL;                
            }                   
        }
    }
        
    g_psensor->pclk = get_pclk(g_psensor->xclk);

    // set image size
    ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height);
    if (ret == 0)
        pinfo->is_init = 1;    

    // set flip    
    set_flip(flip);

    // get current shutter_width and gain setting
    g_psensor->curr_exp = get_exp();
    g_psensor->curr_gain = get_gain();        

    pinfo->is_init = true;
    
    return ret;
}

//=============================================================================
// external functions
//=============================================================================
static void mn34220_deconstruct(void)
{
    if ((g_psensor) && (g_psensor->private)) {
        kfree(g_psensor->private);
    }

    sensor_remove_i2c_driver();
    // turn off EXT_CLK
    isp_api_extclk_onoff(0);
    // release CAP_RST
    isp_api_cap_rst_exit();
}

static int mn34220_construct(u32 xclk, u16 width, u16 height)
{
    sensor_info_t *pinfo = NULL;
    int ret = 0;

    // initial CAP_RST
    ret = isp_api_cap_rst_init();
    if (ret < 0)
        return ret;

    // clear CAP_RST
    isp_api_cap_rst_set_value(0);

    // set EXT_CLK frequency and turn on it.
    ret = isp_api_extclk_set_freq(xclk);
    if (ret < 0)
        return ret;
    ret = isp_api_extclk_onoff(1);
    if (ret < 0)
        return ret;
    mdelay(50);

    // set CAP_RST
    isp_api_cap_rst_set_value(1);
    mdelay(50);

    pinfo = kzalloc(sizeof(sensor_info_t), GFP_KERNEL);
    if (!pinfo) {
        isp_err("failed to allocate private data!\n");
        return -ENOMEM;
    }

    g_psensor->private = pinfo;
    snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, SENSOR_NAME);
    g_psensor->capability = SUPPORT_FLIP;
    g_psensor->xclk = xclk;
    g_psensor->fmt = RAW12;
    g_psensor->bayer_type = BAYER_GR;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
    g_psensor->img_win.x = 4;
    g_psensor->img_win.y = 16;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec
    g_psensor->min_gain = gain_table[0].gain_x;
    g_psensor->max_gain = 4038;//64    
    g_psensor->exp_latency = 1;
    g_psensor->gain_latency = 0;
    g_psensor->fps = fps;
    g_psensor->num_of_lane = ch_num;
    g_psensor->interface = IF_LVDS_PANASONIC;//0x9bc00014
    g_psensor->fn_init = init;
    g_psensor->fn_read_reg = read_reg;
    g_psensor->fn_write_reg = write_reg;
    g_psensor->fn_set_size = set_size;
    g_psensor->fn_get_exp = get_exp;
    g_psensor->fn_set_exp = set_exp;
    g_psensor->fn_get_gain = get_gain;
    g_psensor->fn_set_gain = set_gain;
    g_psensor->fn_get_property = get_property;
    g_psensor->fn_set_property = set_property;

    if ((ret = sensor_init_i2c_driver()) < 0)
        goto construct_err;
	if (g_psensor->interface == IF_PARALLEL){
    	if ((ret = init()) < 0)
        	goto construct_err;
	}

    return 0;

construct_err:
    mn34220_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init mn34220_init(void)
{
    int ret = 0;

    // check ISP driver version
    if (isp_get_input_inf_ver() != ISP_INPUT_INF_VER) {
        isp_err("input module version(%x) is not equal to fisp320.ko(%x)!\n",
             ISP_INPUT_INF_VER, isp_get_input_inf_ver());
        ret = -EINVAL;
        goto init_err1;
    }

    g_psensor = kzalloc(sizeof(sensor_dev_t), GFP_KERNEL);
    if (!g_psensor) {
        ret = -ENODEV;
        goto init_err1;
    }

    if ((ret = mn34220_construct(sensor_xclk, sensor_w, sensor_h)) < 0){
        isp_err("mn34220_construct Error \n");
        goto init_err2;
        }
    // register sensor device to ISP driver    
    
    if ((ret = isp_register_sensor(g_psensor)) < 0){
        isp_err("isp_register_sensor Error \n");
        goto init_err2;
     } 
   
    return ret;

init_err2:
    kfree(g_psensor);

init_err1:
    return ret;
}

static void __exit mn34220_exit(void)
{
    isp_unregister_sensor(g_psensor);
    mn34220_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(mn34220_init);
module_exit(mn34220_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor mn34220");
MODULE_LICENSE("GPL");
