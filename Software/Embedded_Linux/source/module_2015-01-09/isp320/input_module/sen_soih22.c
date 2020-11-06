/**
 * @file sen_soih22.c
 * OmniVision soih22 sensor driver
 *
 * Copyright (C) 2010 GM Corp. (http://www.grain-media.com)
 *
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
#include "isp_dbg.h"
#include "isp_input_inf.h"
#include "isp_api.h"

//=============================================================================
// module parameters
//=============================================================================
#define DEFAULT_IMAGE_WIDTH     1280
#define DEFAULT_IMAGE_HEIGHT    720
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

static uint mirror = 0;
module_param(mirror, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mirror, "sensor mirror function");

static uint flip = 0;
module_param(flip, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(flip, "sensor flip function");

static uint fps = 30;
module_param(fps, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fps, "sensor frame rate");

//=============================================================================
// global
//============================================================================
#define SENSOR_NAME         "soih22"
#define SENSOR_MAX_WIDTH    1280
#define SENSOR_MAX_HEIGHT   800

static sensor_dev_t *g_psensor = NULL;

typedef struct sensor_info {
    bool is_init;
    int mirror;
    int flip;
    u32 total_line;    
    u32 reg04;
    u32 t_row;          // unit is 1/10 us
    int htp;
} sensor_info_t;

typedef struct sensor_reg {
    u8 addr;
    u8 val;
} sensor_reg_t;

static sensor_reg_t sensor_init_regs[] = {   
    {0x12, 0x40},
    {0x1D, 0xFF},
    {0x1E, 0x9F},
    {0x37, 0x40},
    {0x38, 0x98},
    {0x20, 0xDC},
    {0x21, 0x05},
    {0x22, 0xC0},
    {0x23, 0x03},
    {0x24, 0x00},
    {0x25, 0xD0},
    {0x26, 0x25},
    {0x27, 0xC1},
    {0x28, 0x0D},
    {0x29, 0x00},
    {0x2C, 0x00},
    {0x2D, 0x0A},
    {0x2E, 0xC2},
    {0x2F, 0x20},
    {0x14, 0x80},
    {0x16, 0xA0},
    {0x17, 0x40},
    {0x18, 0xD5},
    {0x19, 0x10},
    {0x37, 0x40},
    {0x38, 0x98},
    {0x13, 0xC7},
    {0x12, 0x00},
    {0x99, 0x00},
    {0x9a, 0x00},
    {0x57, 0x00},
    {0x58, 0xC8},
    {0x59, 0xa0},
    {0x4c, 0x13},
    {0x4b, 0x36},
    {0x3d, 0x3c},
    {0x3e, 0x03},
    {0xbd, 0xa0},
    {0xbe, 0xc8},
    {0x04, 0xC8},
};
#define NUM_OF_INIT_REGS (sizeof(sensor_init_regs) / sizeof(sensor_reg_t))

typedef struct gain_set {
    u8  ad_reg;
    u32 gain_x;
} gain_set_t;

static const gain_set_t gain_table[] = {
    {0  ,   64 },{1  ,   68},{2  ,   72},{3  ,    76},{4  ,   80},
    {5  ,   84 },{6  ,   88},{7  ,   92},{8  ,    96},{9  ,  100},
    {10 ,   104},{11 ,  108},{12 ,  112},{13 ,   116},{14 ,  120},
    {15 ,   124},{16 ,  128},{17 ,  136},{18 ,   144},{19 ,  152},
    {20 ,   160},{21 ,  168},{22 ,  176},{23 ,   184},{24 ,  192},
    {25 ,   200},{26 ,  208},{27 ,  216},{28 ,   224},{29 ,  232},
    {30 ,   240},{31 ,  248},{48 ,  256},{49 ,   272},{50 ,  288},
    {51 ,   304},{52 ,  320},{53 ,  336},{54 ,   352},{55 ,  368},
    {56 ,   384},{57 ,  400},{58 ,  416},{59 ,   432},{60 ,  448},
    {61 ,   464},{62 ,  480},{63 ,  496},{112,   512},{113,  544},
    {114,   576},{115,  608},{116,  640},{117,   672},{118,  704},
    {119,   736},{120,  768},{121,  800},{122,   832},{123,  864},
    {124,   896},{125,  928},{126,  960},{127,   992},{240, 1024},
    {241,  1088},{242, 1152},{243, 1216},{244,  1280},{245, 1344},
    {246,  1408},{247, 1472},{248, 1536},{249,  1600},{250, 1664},
    {251,  1728},{252, 1792},{253, 1856},{254,  1920},{255, 1984} 
};  
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

//=============================================================================
// I2C
//=============================================================================
#define I2C_NAME            SENSOR_NAME
#define I2C_ADDR            (0x6C >> 1)
#include "i2c_common.c"

//=============================================================================
// internal functions
//=============================================================================
static int read_reg(u32 addr, u32 *pval)
{
    struct i2c_msg  msgs[2];
    unsigned char   tmp[1], tmp2[2];

    tmp[0]        = addr & 0xFF;
    msgs[0].addr  = I2C_ADDR;
    msgs[0].flags = 0; // write
    msgs[0].len   = 1;
    msgs[0].buf   = tmp;

    tmp2[0]       = 0; // clear
    msgs[1].addr  = I2C_ADDR;
    msgs[1].flags = 1; // read
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

    buf[0]     = addr & 0xFF;
    buf[1]     = val & 0xFF;
    msgs.addr  = I2C_ADDR;
    msgs.flags = 0;
    msgs.len   = 2;
    msgs.buf   = buf;

    return sensor_i2c_transfer(&msgs, 1);
}

static void adjust_vblk(int fps)
{    
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 reg_l, reg_h, tmp_pclk;                                                                              
    
    if((fps <= 30) && (fps >= 20)){
        write_reg(0x11, 0x80);// 43200000        
        // pclk = vts * total_line * fps
        pinfo->htp = g_psensor->pclk / fps / pinfo->total_line;        
        reg_h = (pinfo->htp >> 8) & 0xFF;
        reg_l = pinfo->htp & 0xFF;
        write_reg(0x21, reg_h);
        write_reg(0x20, reg_l);                
        pinfo->t_row = (pinfo->htp * 10000) / (g_psensor->pclk / 1000);    
        g_psensor->min_exp = (pinfo->t_row + 50) / 100;
        //printk("t_row = %d, min_exposure =%d, pclk=%d, htp=%d\n", pinfo->t_row, g_psensor->min_exp, g_psensor->pclk, pinfo->htp); 
    }
    if((fps < 20) && (fps > 5)){
        write_reg(0x11, 0x01);// 43200000/2
        tmp_pclk = g_psensor->pclk / 2;
        // pclk = vts * total_line * fps
        pinfo->htp = tmp_pclk / fps / pinfo->total_line;        
        reg_h = (pinfo->htp >> 8) & 0xFF;
        reg_l = pinfo->htp & 0xFF;
        write_reg(0x21, reg_h);
        write_reg(0x20, reg_l);                
        pinfo->t_row = (pinfo->htp * 10000) / (tmp_pclk / 1000);    
        g_psensor->min_exp = (pinfo->t_row + 50) / 100;
        //printk("t_row = %d, min_exposure =%d, tmp_pclk=%d, htp=%d\n", pinfo->t_row, g_psensor->min_exp, tmp_pclk, pinfo->htp); 
    }                               
    
}

static u32 get_pclk(u32 xclk)
{
    u32 pixclk;

    pixclk = 43200000;//xclk=27MHz    
          
    printk("pixel clock %d\n", pixclk);
    return pixclk;
}


static int set_size(u32 width, u32 height)
{
    int ret = 0;    

    if ((width > SENSOR_MAX_WIDTH) || (height > SENSOR_MAX_HEIGHT)) {
        isp_err("%dx%d exceeds maximum resolution %dx%d!\n",
            width, height, SENSOR_MAX_WIDTH, SENSOR_MAX_HEIGHT);
        return -1;
    }

    // update sensor information
    g_psensor->out_win.x = 0;
    g_psensor->out_win.y = 0;
    g_psensor->out_win.width = SENSOR_MAX_WIDTH;
    g_psensor->out_win.height = SENSOR_MAX_HEIGHT;
    g_psensor->img_win.x = 0;//((SENSOR_MAX_WIDTH - width) >> 1) &~BIT0;
    g_psensor->img_win.y = 0;//((SENSOR_MAX_HEIGHT - height) >> 1) &~BIT0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;
        
    //printk("x=%d,y=%d\n",g_psensor->img_win.x , g_psensor->img_win.y);        

    return ret;
}

static u32 get_exp(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 lines, sw_u, sw_l;

    if (!g_psensor->curr_exp) {
        read_reg(0x02, &sw_u);//high byte
        read_reg(0x01, &sw_l);//low  byte        
        lines = (sw_u << 8) | sw_l;
        g_psensor->curr_exp = (lines * pinfo->t_row + 50) / 100;
    }

    return g_psensor->curr_exp;
}

static int set_exp(u32 exp)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;
    u32 lines, tmp, max_exp_line;

    lines = ((exp * 1000)+ pinfo->t_row /2) / pinfo->t_row;
    
    isp_dbg("exp=%d,pinfo->t_row=%d\n", exp, pinfo->t_row);

    if (lines == 0)
        lines = 1;  
           
    max_exp_line = pinfo->total_line - 5;//Hardware limitation
    
    if (lines > max_exp_line) {
        tmp = max_exp_line;      
    } else {
        tmp = lines;
    }    

    if (lines > max_exp_line) {
        write_reg(0x23, (lines >> 8) & 0xFF);
        write_reg(0x22, (lines & 0xFF));                
        isp_dbg("lines = %d\n",lines); 
    }   
    
    write_reg(0x01, (tmp & 0xFF));
    write_reg(0x02, ((tmp >> 8) & 0xFF)); 
    
    g_psensor->curr_exp = ((lines * pinfo->t_row) + 500) / 1000;
     
    isp_dbg("g_psensor->curr_exp=%d,max_exp_line=%d,lines=%d,tmp=%d\n", g_psensor->curr_exp, max_exp_line, lines,tmp);

    return 0;
}

static u32 get_gain(void)
{
    u32 gain, reg, mul0, mul1=0;

     if (g_psensor->curr_gain == 0) {       
        read_reg(0x0, &reg);//analog
        read_reg(0x0d, &mul0);//digital
        mul0 = mul0 & 0x03;//[1:0]
        gain = (1+ (reg >> 6)) * (1+ (reg >> 5)) * (1+ (reg >> 4)) * (1 + (reg & 0x0F)/16);
        //printk("gain0=%d\n", gain);
        gain = gain * 1024;
        //printk("gain1=%d\n", gain);
        if(mul0 == 0)
            mul1 = 1;//1x
        else if(mul0 == 1)    
            mul1 = 2;//2x
            
        gain = gain * mul1;     
        g_psensor->curr_gain = gain;
        
        //printk("reg=%d,mul0=%d,mul1=%d\n", reg, mul0, mul1);
        //printk("get_curr_gain=%d\n",g_psensor->curr_gain);
    }

    return g_psensor->curr_gain;
}

static int set_gain(u32 gain)
{
    int ret = 0, i;
    u32 ad_gain;

    // search most suitable gain into gain table
    ad_gain = gain_table[0].ad_reg;

    for (i=0; i<NUM_OF_GAINSET; i++) {
        if (gain_table[i].gain_x > gain)
            break;
    }
    if (i > 0)
        ad_gain = gain_table[i-1].ad_reg;
    
    write_reg(0x00, ad_gain);
    g_psensor->curr_gain = gain_table[i-1].gain_x;
        
    //printk("g_psensor->curr_gain=%d\n",g_psensor->curr_gain);

    return ret;
}

static void update_bayer_type(void)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (pinfo->mirror) {
        if (pinfo->flip)
            g_psensor->bayer_type = BAYER_RG;
        else
            g_psensor->bayer_type = BAYER_GB;
    } else {
        if (pinfo->flip)
            g_psensor->bayer_type = BAYER_GR;
        else            
            g_psensor->bayer_type = BAYER_BG;
    }
}

static int get_mirror(void)
{
    u32 reg;
    read_reg(0x12, &reg);
    return (reg & BIT5) ? 1 : 0;
}

static int set_mirror(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;    

    if (enable) {
        pinfo->reg04 |= BIT5;
    } else {
        pinfo->reg04 &=~BIT5;
    }
    write_reg(0x12, pinfo->reg04);
    pinfo->mirror = enable;  
    update_bayer_type();  

    return 0;
}

static int get_flip(void)
{
    u32 reg;
    read_reg(0x12, &reg);
    return (reg & BIT4) ? 1 : 0;
}

static int set_flip(int enable)
{
    sensor_info_t *pinfo = (sensor_info_t *)g_psensor->private;

    if (enable) {
        pinfo->reg04 |= BIT4;

    } else {
        pinfo->reg04 &=~BIT4;
    }
    write_reg(0x12, pinfo->reg04);
    pinfo->flip = enable;
    update_bayer_type();

    return 0;
}

static int get_property(enum SENSOR_CMD cmd, unsigned long arg)
{
    int ret = 0;

    switch (cmd) {
    case ID_MIRROR:
        *(int*)arg = get_mirror();
        break;

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
    case ID_MIRROR:
        set_mirror((int)arg);
        break;

    case ID_FLIP:
        set_flip((int)arg);
        break;
    
    case ID_FPS:
        adjust_vblk((int)arg);
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
    u32 reg_h, reg_l;
    int i;    

    if (pinfo->is_init)
        return 0;
    
    //printk("pinfo->is_init = %d\n",pinfo->is_init);
    
    for (i=0; i<NUM_OF_INIT_REGS; i++) {
        if (write_reg(sensor_init_regs[i].addr, sensor_init_regs[i].val) < 0) {
            isp_err("failed to initial %s!\n", SENSOR_NAME);
            return -EINVAL;
        }
        mdelay(1);
        //printk("[%d]%x=%x\n",i, sensor_init_regs[i].addr, sensor_init_regs[i].val);
    }
      
    read_reg(0x23, &reg_h);
    read_reg(0x22, &reg_l);
    pinfo->total_line = ((reg_h << 8) | reg_l);         
    g_psensor->pclk = get_pclk(g_psensor->xclk); 
        
    read_reg(0x21, &reg_h);
    read_reg(0x20, &reg_l);
    pinfo->htp = ((reg_h << 8) | reg_l); 
    pinfo->t_row = (pinfo->htp * 10000) / (g_psensor->pclk / 1000);
    g_psensor->min_exp = (pinfo->t_row + 50) / 100;       
    
    //printk("pinfo->total_line=%d,pinfo->htp=%d,pinfo->t_row=%d,g_psensor->min_exp=%d\n",pinfo->total_line,pinfo->htp,pinfo->t_row,g_psensor->min_exp);    

    // set image size
    if ((ret = set_size(g_psensor->img_win.width, g_psensor->img_win.height)) < 0)
        return ret;

    // set mirror and flip
    set_mirror(mirror);    
    set_flip(flip);
    //printk("mirror=%d,flip=%d\n",mirror ,flip);

    // initial exposure and gain
    set_exp(g_psensor->min_exp);
    set_gain(g_psensor->min_gain);

    // get current shutter_width and gain setting
    g_psensor->curr_exp = get_exp();
    g_psensor->curr_gain = get_gain();   
    
    pinfo->is_init = true;
    
    return ret;
}

//=============================================================================
// external functions
//=============================================================================
static void soih22_deconstruct(void)
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

static int soih22_construct(u32 xclk, u16 width, u16 height)
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

    pinfo = kzalloc(sizeof(sensor_info_t), GFP_KERNEL);
    if (!pinfo) {
        isp_err("failed to allocate private data!\n");
        return -ENOMEM;
    }
    g_psensor->private = pinfo;
    snprintf(g_psensor->name, DEV_MAX_NAME_SIZE, SENSOR_NAME);
    g_psensor->capability = SUPPORT_MIRROR | SUPPORT_FLIP;
    g_psensor->xclk = xclk;
    g_psensor->interface = IF_PARALLEL;
    g_psensor->num_of_lane = 0;
    g_psensor->fmt = RAW10;
    g_psensor->bayer_type = BAYER_BG;
    g_psensor->hs_pol = ACTIVE_HIGH;
    g_psensor->vs_pol = ACTIVE_HIGH;
    
    g_psensor->img_win.x = 0;
    g_psensor->img_win.y = 0;
    g_psensor->img_win.width = width;
    g_psensor->img_win.height = height;

    g_psensor->min_exp = 1;
    g_psensor->max_exp = 5000; // 0.5 sec
    g_psensor->min_gain = gain_table[0].gain_x;
    g_psensor->max_gain = gain_table[NUM_OF_GAINSET - 1].gain_x;
    
    g_psensor->exp_latency = 1;
    g_psensor->gain_latency = 0;
    g_psensor->fps = fps;
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
        
#if 0
    if ((ret = init()) < 0)
        goto construct_err;
#endif

    return 0;

construct_err:
    soih22_deconstruct();
    return ret;
}

//=============================================================================
// module initialization / finalization
//=============================================================================
static int __init soih22_init(void)
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

    if ((ret = soih22_construct(sensor_xclk, sensor_w, sensor_h)) < 0)
        goto init_err2;

    // register sensor device to ISP driver
    if ((ret = isp_register_sensor(g_psensor)) < 0)
        goto init_err2;

    return ret;

init_err2:
    kfree(g_psensor);

init_err1:
    return ret;
}

static void __exit soih22_exit(void)
{
    isp_unregister_sensor(g_psensor);
    soih22_deconstruct();
    if (g_psensor)
        kfree(g_psensor);
}

module_init(soih22_init);
module_exit(soih22_exit);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("Sensor Soih22");
MODULE_LICENSE("GPL");
