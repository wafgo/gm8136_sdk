/* -----------------------------------------------------------------------------
 *  PLATFORM 8181T
 * -----------------------------------------------------------------------------
 */
/*
 * Local function declaration
 */ 
static void platform_pmu_switch_pinmux_8181t(void *ptr, int on);
static void platform_pmu_switch_pinmux_8181(void *ptr, int on);
static int platfrom_8181t_init(void);
static void platfrom_8181t_exit(void); 
static int platform_tve100_config_8181t(int lcd_idx, int mode, int input);

unsigned int  CT656_BASE = 0;

/*
 * Body
 */ 
unsigned int platform_get_pll11(void)
{
    /* As Terry's comment, FCLK is always 540Mhz */
    return 540 * 1000000;
}

/* pixclk: Standard VESA clock (kHz)
 */
unsigned int platform_pmu_calculate_clk_8181t(int lcd200_idx, u_long pixclk,
                                              unsigned int b_Use_Ext_Clk)
{
    u32 tmp, tmp_mask, LCDen;
    unsigned int pll3, div, sclar_div;
    unsigned int diff, tolerance, v1, v2, v3;
    unsigned int pll_clock;
    int org_val = 0, use_pll1 = 0;

    if (b_Use_Ext_Clk) {
    }

  re_calculate_clk:
    pll_clock = use_pll1 ? platform_get_pll11() : platform_get_pll3();

    pll3 = pll_clock / 1000;    //chage to Khz
    div = ((pll3 + pixclk / 2) / pixclk) - 1;
    if (!div)
        div = 1;

    /* calculate scalar clock */
    sclar_div = pll_clock;
    do_div(sclar_div, MAX_SCALAR_CLK);
    if (sclar_div > div)
        sclar_div = div;

    /* 
     * check if the clock divisor is changed 
     */
    if (lcd200_idx == LCD_ID) {
        u32 tmp, tmp2;

        tmp = ftpmu010_read_reg(0x70);
        tmp2 = (tmp >> 10) & 0x1F;      //scaler clock
        if (tmp2 == sclar_div) {
            tmp2 = (tmp >> 5) & 0x1F;
            if (tmp2 == div)
                goto exit;      /* do nothing */
        }
    } else if (lcd200_idx == LCD2_ID) {
        u32 tmp, tmp2;
        
        tmp = ftpmu010_read_reg(0x74);
        tmp2 = (tmp >> 26) & 0x1F;      //scaler clock
        if (tmp2 == sclar_div) {
            tmp2 = (tmp >> 21) & 0x1F;
            if (tmp2 == div)    /* just check TVE2_PVALUE */
                goto exit;      /* do nothing */
        }
    }

    v1 = pll_clock / (div + 1);
    v1 /= 1000000;              //to Mhz
    v2 = pll_clock / (div + 1);
    v2 = ((v2 % 1000000) * 100) / 1000000;
    v3 = pll_clock / (sclar_div + 1);
    v3 /= 1000000;              //to Mhz    

    diff = DIFF(pixclk * 1000, pll_clock / (div + 1));
    tolerance = diff / pixclk;  /* comment: (diff * 1000) / (pixclk * 1000); */

#if 1                           /* special case for 27M */
    if (tolerance > 5) {
        if (use_pll1 == 0) {
            use_pll1 = 1;
            goto re_calculate_clk;
        }
    }
#endif

    if (tolerance > 5) {        /* if the tolerance is larger than 0.5%, dump error message! */
        int i = 0;

        v1 = pll_clock / (div + 1);
        v1 /= 1000000;          //to Mhz
        v2 = pll_clock / (div + 1);
        v2 = ((v2 % 1000000) * 100) / 1000000;

        while (i < 50) {
            printk("The standard clock is %d.%dMHZ, but gets %d.%dMHZ. Please correct %s!\n",
                   (u32) (pixclk / 1000), (u32) ((100 * (pixclk % 1000)) / 1000), v1, v2,
                   use_pll1 ? "FCLK" : "PLL3");
            i++;
        }
    }

    if (use_pll1)
        printk("LCD%d uses FCLK clock as the source. \n", lcd200_idx);
        
    printk("LCD%d: Standard: %d.%dMHZ \n", lcd200_idx, (u32) (pixclk / 1000),
           (u32) ((100 * (pixclk % 1000)) / 1000));
    printk("LCD%d: PVALUE = %d, pixel clock = %d.%dMHZ, scalar clock = %dMHZ \n", lcd200_idx, div,
           v1, v2, v3);

    /* change the frequency, the steps will be:
     * 1) release the abh bus lock
     * 2) disable LCD controller
     * 3) turn off the gating clock
     */
    if (lcd200_idx == LCD_ID) {
        LCDen = ioread32(lcd_va_base[0]);
        if (LCDen & 0x1) {
            u32 temp;

            /* release bus */
            org_val = platform_lock_ahbbus(lcd200_idx, 0);
            /* disable LCD controller */
            temp = LCDen & (~0x1);
            iowrite32(temp, lcd_va_base[0]);
            /* disable gating clock */
            platform_cb->pmu_clock_gate(lcd200_idx, 0, (use_pll1 == 1));
        }
        
        tmp_mask = (0x1F << 10) | (0x1F << 5);  // maskout 14-10, 9-5
        tmp = (sclar_div << 10) | (div << 5);
        if (ftpmu010_write_reg(lcd_fd, 0x70, tmp, tmp_mask))
            panic("%s \n", __func__);
        
        /* rollback the orginal state */
        if (LCDen & 0x1) {
            /* enable gating clock */
            platform_cb->pmu_clock_gate(lcd200_idx, 1, (use_pll1 == 1));

            /* enable LCD controller */
            iowrite32(LCDen, lcd_va_base[0]);

            /* lock bus ? */
            platform_lock_ahbbus(lcd200_idx, org_val);
        } else {
            /* for special case FCLK as the clock source */
            u32 tmp, tmp_mask;
            
            tmp_mask = 0x1 << 19;
            tmp = use_pll1 ? 0x1 << 19 : 0x0 << 19; /* tve_cnt_sel, choose 1:FCLK or 0:PLL3 */
            if (ftpmu010_write_reg(lcd_fd, 0x28, tmp, tmp_mask))
                panic("%s, error! \n", __func__);
        }
    } else if (lcd200_idx == LCD2_ID) {
        LCDen = ioread32(lcd_va_base[1]);
        if (LCDen & 0x1) {
            u32 temp;

            /* release bus */
            org_val = platform_lock_ahbbus(lcd200_idx, 0);
            /* disable LCD controller */
            temp = LCDen & (~0x1);
            iowrite32(temp, lcd_va_base[1]);
            /* disable gating clock */
            platform_cb->pmu_clock_gate(lcd200_idx, 0, (use_pll1 == 1));
        }
        
        tmp_mask = (0x1F << 26) | (0x1F << 21); // maskout 30-26, 25-21
        tmp = ((sclar_div << 26) | (div << 21));
        if (ftpmu010_write_reg(lcd_fd, 0x74, tmp, tmp_mask))
            panic("%s, error2 \n", __func__);
        
        /* rollback the orginal state */
        if (LCDen & 0x1) {
            /* enable gating clock */
            platform_cb->pmu_clock_gate(lcd200_idx, 1, (use_pll1 == 1));

            /* enable LCD controller */
            iowrite32(LCDen, lcd_va_base[1]);

            /* lock bus ? */
            platform_lock_ahbbus(lcd200_idx, org_val);
        } else {
            /* for special case FCLK as the clock source */
            u32 tmp, tmp_mask;
            
            tmp_mask = 0x1 << 20;
            tmp = use_pll1 ? (0x1 << 20) : (0x0 << 20); /* tve_cnt_sel, choose 1:FCLK, 0:PLL3 */
            
            if (ftpmu010_write_reg(lcd_fd, 0x28, tmp, tmp_mask))
                panic("%s, error3 \n", __func__);
        }
    }

  exit:
    return pll_clock / (div + 1);
}

void platform_pmu_switch_pinmux_8181t(void *ptr, int on)
{	
	struct lcd200_dev_info  *info = ptr;
	struct ffb_dev_info *fdinfo = (struct ffb_dev_info*)info;
	int     lcd_idx = info->lcd200_idx;
	u32     tmp, tmp_mask;
	
	if (on) 
	{	    
	    /* 0x28 config tveout_en to output mode */	    
	    tmp_mask = (0x1 << 14);
	    tmp = (0x1 << 14);
	    if (ftpmu010_write_reg(lcd_fd, 0x28, tmp, tmp_mask))
	        panic("%s, erro3\n", __func__);
	                    
        /* driving capbility */        
	    tmp_mask = (0xF << 16);   //tv_dcsr, bit16-19
	    tmp = (0xE << 16);    //16mA
	    if (ftpmu010_write_reg(lcd_fd, 0x44, tmp, tmp_mask))
	        panic("%s, error4 \n", __func__);
	    
        /* switch PMU to default output */
        platform_pmu_switch_pinmux_2(info->lcd200_idx, output_target[lcd_idx]);
        
	    if (lcd_idx == LCD_ID)
	    {
	        if (fdinfo->video_output_type >= VOT_CCIR656)
	        {
	            /* 
	             * TV 
	             */
	            if ((info->CCIR656.Parameter & CCIR656_SYS_MASK) == CCIR656_SYS_HDTV) 
	            {
	                /* BT1120 */
	            }
	            else
	            {
	                /* CVBS */
	            }
	        }
	        else
	        {
	            /* RGB888 */
	        }
	    }
	    else if (lcd_idx == LCD2_ID)
	    {
	        if (fdinfo->video_output_type >= VOT_CCIR656)
	        {
	            /* TV */
	            if ((info->CCIR656.Parameter & CCIR656_SYS_MASK) == CCIR656_SYS_HDTV) 
	            {
	                /* BT1120 */
	            }
	            else
	            {
	                /* CVBS */
	            }
	        }
	        else
	        {
	            /* RGB888 */
	        }
	    }
	    else
	    {
	        printk("Wrong LCD ID \n");
	    }
	} /* ON */
}

static void platform_pmu_switch_pinmux_2(int lcd_idx, OUT_TARGET_T target)
{
    u32     tmp, tmp_mask, found_vga = 0;
    
    /* this configuration must be done once */
    if (!lcd_pin_init[lcd_idx]) 
    {    
        lcd_pin_init[lcd_idx] = 1;  //marked as init already 
    }
    else 
    {
        if (output_target[lcd_idx] == target) 
            return;
    }
                
    if (target == OUT_TARGET_CVBS)
    {
        tmp = ioread32(CT656_BASE);
        tmp &= ~(0xF << 8); 
        if (lcd_idx == LCD_ID)
            tmp |= (0x0 << 8);  /* first LCD */
        else                
            tmp |= (0x6 << 8);  /* second LCD */
            
        iowrite32(tmp, CT656_BASE);
    }
    else if (target == OUT_TARGET_VGA)
    {
        tmp = 0;
        
        /* switch VGA DAC source */
        if (lcd_idx == LCD_ID)
        {
            tmp_mask = (0x1 << 27);
            tmp |= (0x0 << 27); /* VGA DAC choose LCD */
        }
        else
        {            
            tmp_mask = (0x1 << 27) | (0x3 << 16);
            tmp |= (0x1 << 27); /* VGA DAC choose LCD2 */
            tmp |= (0x3 << 16); /* HS/VS */
        }
        if (ftpmu010_write_reg(lcd_fd, 0x50, tmp, tmp_mask))
            panic("%s, error5 \n", __func__);
        
        /* turn on VGA chip */        
        tmp_mask = (0x1 << 13) | (0x1 << 14) | (0x1 << 15);
        tmp = 0;
        tmp |= (0x1 << 13);     /* vga_vs_oe/vga_hs_oe, output mode enable */
        tmp |= (0x0 << 14);     /* Power-down control of the VGA DAC, 0: Normal operation. */
	    tmp |= (0x0 << 15);     /* Standby mode control, 0: Normal operation. */
	    if (ftpmu010_write_reg(lcd_fd, 0x6C, tmp, tmp_mask))
            panic("%s, error6 \n", __func__);
    }
    else if (target == OUT_TARGET_LCD24)
    {
        int isPCI = 0;
                
        tmp = ftpmu010_read_reg(0x4);
        if (tmp & (0x1 << 4))
            isPCI = 1;          /* use PCI Boot */
        
        if (isPCI) {
            //clear X_TV_DATA, CAP3_LCD24_MUX, CAP3_LCD24_MUX2
            tmp_mask = (0x7 << 5) | (0x1 << 3) | (0x1 << 25);            
            tmp = (0x1 << 3);  /* choose LCD24 */
        } else {
            //clear X_TV_DATA, CAP4_LCD24_MUX, CAP4_LCD24_MUX2
            tmp_mask = (0x7 << 5) | (0x1 << 4) | (0x1 << 26);
            tmp = (0x1 << 4);  /* choose LCD24 */
        }
        
        /* start to OR operation */
        if (isPCI)
            tmp |= (0x1 << 3);  /* choose LCD24 */
        else
            tmp |= (0x1 << 4);  /* choose LCD24 */

        if (lcd_idx == LCD_ID) {
            tmp |= (0x1 << 5);  /* LCD RGB output[15:0] */

            if (isPCI)
                tmp |= (0x0 << 25);     /* choose LCD RGB[23:16] */
            else
                tmp |= (0x0 << 26);     /* choose LCD RGB[23:16] */
        } else {
            tmp |= (0x3 << 5);  /* LCD2 RGB output[15:0] */

            if (isPCI)
                tmp |= (0x1 << 25);     /* choose LCD2 RGB[23:16] */
            else
                tmp |= (0x1 << 26);     /* choose LCD2 RGB[23:16] */
        }
        if (ftpmu010_write_reg(lcd_fd, 0x50, tmp, tmp_mask))
            panic("%s, error8 \n", __func__);

        /* turn on VGA chip for dual output (VGA+HDMI) */
        tmp_mask = (0x1 << 13) | (0x1 << 14) | (0x1 << 15);
        tmp = 0;
        tmp |= (0x1 << 13);     /* vga_vs_oe/vga_hs_oe, output mode enable */
        tmp |= (0x0 << 14);    /* Power-down control of the VGA DAC, 0: Normal operation. */
        tmp |= (0x0 << 15);    /* Standby mode control, 0: Normal operation. */
        
        if (ftpmu010_write_reg(lcd_fd, 0x6c, tmp, tmp_mask))
            panic("%s, error8 \n", __func__);
    }
    else if (target == OUT_TARGET_BT1120)
    {
        tmp_mask = (0x7 << 5);  //clear X_TV_DATA
        tmp = 0;
        if (lcd_idx == LCD_ID)
            tmp |= (0x0 << 5);  /* choose LCD 656 output[15:0] */
        else                
            tmp |= (0x2 << 5);  /* choose LCD2 656 output[15:0] */
            
        if (ftpmu010_write_reg(lcd_fd, 0x50, tmp, tmp_mask))
            panic("%s, error7 \n", __func__);
    }
    else if (target == OUT_TARGET_DISABLE)
    {
        /* do nothing */
    }
    else
    {
        printk("Wrong output target %d \n", output_target[lcd_idx]);
        return;
    }
    
    output_target[lcd_idx] = target;
    
    /* if no VGA signal, we need to turn VGA DAC off */
    for (lcd_idx = LCD_ID; lcd_idx < LCD_IP_NUM; lcd_idx ++)
    {
        if (output_target[LCD_ID] == OUT_TARGET_LCD24)
        {
            /* needs HS/VS */    
            found_vga = 1;
            break;
        }
        
        if (output_target[lcd_idx] == OUT_TARGET_VGA)
        {
            found_vga = 1;
            break;
        }
    }
        
    if (!found_vga)
    {
        /* disable VGA chip first */
        tmp_mask = (0x1 << 13) | (0x1 << 14) | (0x1 << 15);
        tmp = 0;
        tmp |= (0x0 << 13);   /* vga_vs_he_oe output enable, 0:disabled */
        tmp |= (0x1 << 14);    /* Power-down control of the VGA DAC, 1: Power down. */
        tmp |= (0x1 << 15);    /* Standby mode control, 1: Standby mode. */
        ftpmu010_write_reg(lcd_fd, 0x6C, tmp, tmp_mask);
    }
}

/* this function will be called in initialization state or module removed */
void platform_pmu_clock_gate_8181t(int lcd_idx, int on_off, int use_ext_clk)
{
    u32     tmp, tmp_mask;
    
    if (on_off)
    {
        /* ON 
         */
             
        /* turn on CT656 clock */ 
        tmp_mask = (0x1 << 13) | (0x1 << 7);
        tmp = 0;
        tmp |= (0x0 << 13);    // ct656 clock 
        tmp |= (0x0 << 7);     // lcd global clock(hclk), TVE(hclk), LC_SCALER_CLK        
        if (ftpmu010_write_reg(lcd_fd, 0x38, tmp, tmp_mask))
            panic("%s, error9 \n", __func__);
                
        if (lcd_idx == LCD_ID)
        {
            /* choose clock source */   
            tmp_mask = (0x1 << 19) | (0x1 << 8) | (0x1 << 6);
            tmp = 0;
            tmp |= (0x1 << 6);  //FCLK
            if (use_ext_clk)
                tmp |= (0x1 << 19);     /* tve_cnt_sel, choose FCLK */
            else
                tmp |= (0x0 << 19);   /* tve_cnt_sel, choose PLL3 */
            tmp |= (0x0 << 8);    /* tveclk_sel, 0 from PLL3, 1:X_OSCH1 */
            if (ftpmu010_write_reg(lcd_fd, 0x28, tmp, tmp_mask))
                panic("%s, error10 \n", __func__);
            
            /* lcd clock gate */
            tmp_mask = (0x1 << 10);
            tmp = (0x0 << 10);    // LC_CLK(pixel clock)
            if (ftpmu010_write_reg(lcd_fd, 0x38, tmp, tmp_mask))
                panic("%s, error11 \n", __func__);
        }
        else if (lcd_idx == LCD2_ID)
        {                        
            /* choose clock source */   
            tmp_mask = (0x1 << 20) | (0x1 << 18) | (0x1 << 6);
            tmp = 0;
            tmp |= (0x1 << 6);  //FCLK
            if (use_ext_clk)
                tmp |= (0x1 << 20);    /* tve_cnt_sel, choose FCLK */
            else
                tmp |= (0x0 << 20);    /* tve_cnt_sel, choose PLL3 */
            tmp |= (0x1 << 18);    /* tveclk_sel, 0 from PLL3, 1:X_OSCH1 */
            if (ftpmu010_write_reg(lcd_fd, 0x28, tmp, tmp_mask))
                panic("%s, error11 \n", __func__);
                                   
            /* lcd clock gate */
            tmp_mask = (0x1 << 29) | (0x1 << 28);
            tmp = 0;
            tmp |= (0x0 << 29);     // lcd2 gate 
            tmp |= (0x0 << 28);     // LC_CLK(pixel clock)
            if (ftpmu010_write_reg(lcd_fd, 0x3C, tmp, tmp_mask))
                panic("%s, error12 \n", __func__);
        }
    }
    else
    {
        /* OFF
         */
        if (lcd_idx == LCD_ID)
        {
            u32     tmp2;
                        
            tmp_mask = (0x1 << 10);
            tmp = 0;
            tmp |= (0x1 << 10);     /* turn off LCD pixel clock */
            
        	/* read LCD2 gate clock. If it is disabled, we need turn off TVE hclk */
        	tmp2 = ftpmu010_read_reg(0x3C);
        	if (tmp2 & (0x1 << 28))
        	    tmp |= (0x1 << 7);
        	    
        	ftpmu010_write_reg(lcd_fd, 0x38, tmp, tmp_mask);
    	}
    	else if (lcd_idx == LCD2_ID)
    	{
        	tmp_mask = (0x1 << 29) | (0x1 << 28) | (0x1 << 29);
        	tmp = 0;
        	tmp |= (0x1 << 29);     /* turn off hclk */
        	tmp |= (0x1 << 28);     /* turn off LCD pixel clock */
        	tmp |= (0x1 << 29);     /* turn off LC_SCALER_CLK */
        	ftpmu010_write_reg(lcd_fd, 0x3C, tmp, tmp_mask);
        	
        	/* read LCD gate clock. If it is disabled, we need turn off TVE hclk */        	
        	tmp = ftpmu010_read_reg(0x38);
        	tmp_mask = (0x1 << 7);
        	if (tmp & (0x1 << 10))
        	    tmp = (0x1 << 7);
            ftpmu010_write_reg(lcd_fd, 0x38, tmp, tmp_mask);
    	}
    }
}

/* 
 * mode: VOT_NTSC, VOT_PAL
 * input: 2 : 656in
 *        6 : colorbar output for test
 */
int platform_tve100_config_8181t(int lcd_idx, int mode, int input)
{
    u32     tmp, base = TVE100_BASE;
    static int  current_mode = VOT_NTSC;
    
    if (lcd_idx)    {}
    
    /* only support PAL & NTSC */
    if (mode != -1)
    {
        if ((mode != VOT_NTSC) && (mode != VOT_PAL))
            return -EINVAL;
            
        current_mode = mode;
    }
    
    if ((input != 2) && (input != 6)) /* color bar */
        return -EINVAL;
    
    /* check hclk */
    tmp = ftpmu010_read_reg(0x38);
    if (tmp & (0x1 << 7))
        return -EINVAL;
        
    /* check if LCD clock is on */
    if (lcd_idx == LCD_ID)
    {
        /* CT656 = LCD */
        tmp = ioread32(CT656_BASE);
        tmp &= (0xF << 8);
        if (tmp != 0)   /* TVE output = LCD */
            return -EINVAL;
        
        /* check LCD */
        tmp = ioread32(lcd_va_base[0]);
        if (!(tmp & 0x1))
            return -EINVAL;
    }
    else 
    {
        /* CT656 = LCD2 */
        tmp = ioread32(CT656_BASE);
        tmp &= (0xF << 8);
        if (tmp != (0x6 << 8))  /* TVE output = LCD2 */
            return EINVAL;
        
        /* check LCD */
        tmp = ioread32(lcd_va_base[1]);
        if (!(tmp & 0x1))
            return -EINVAL;
    }
    
    /* 0x0 */
    tmp = (current_mode == VOT_NTSC) ? 0x0 : 0x2;
    iowrite32(tmp, base + 0x0);
    
    /* 0x4 */
    iowrite32(input, base + 0x4);
    
    /* 0x8, 0xc */
    iowrite32(0, base + 0x8);
    
    /* bit 0: power down, bit 1: standby */
    iowrite32(0, base + 0xc);
    
    return 0;
}

/* 
 * proc functions
 */
static struct proc_dir_entry *cvbs_proc = NULL; /* analog proc */
static struct proc_dir_entry *vga_proc = NULL;  /* analog proc */
static struct proc_dir_entry *bt1120_proc = NULL;   /* digital proc */
static struct proc_dir_entry *lcd24_proc = NULL;    /* digital proc */

/* 
 * CVBS
 */
static int proc_read_cvbs(char *page, char **start, off_t off, int count,
			  int *eof, void *data)
{
	int len = 0;
	int i, value = 0;
	
	for (i = 0; i < LCD_IP_NUM; i ++)
	{
	    if (output_target[i] == OUT_TARGET_CVBS)
	    {
	        value = i + 1;
	        break;
	    }
	}
	
	len += sprintf(page, "0:None, 1:First LCD, 2:Second LCD \n");
	len += sprintf(page+len, "current value = %d\n", value);
	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;
	return len;
}

static int proc_write_cvbs(struct file *file, const char *buffer,
			   unsigned long count, void *data)
{
	int len = count;
	unsigned char value[20];
	uint          tmp;

	if (copy_from_user(value, buffer, len))
		return 0;
	value[len] = '\0';
	sscanf(value, "%u\n", &tmp);
	
    switch (tmp)
    {
      case 1: /* LCD */
        /* we don't allow the same target */
        if (output_target[LCD2_ID] == OUT_TARGET_CVBS)
            platform_pmu_switch_pinmux_2(LCD2_ID, OUT_TARGET_DISABLE);
            
        platform_pmu_switch_pinmux_2(LCD_ID, OUT_TARGET_CVBS);
        platform_tve100_config_8181t(LCD_ID, -1, 2);
        break;
      case 2: /* LCD2 */
        /* we don't allow the same target */
        if (output_target[LCD_ID] == OUT_TARGET_CVBS)
            platform_pmu_switch_pinmux_2(LCD_ID, OUT_TARGET_DISABLE);
                
        platform_pmu_switch_pinmux_2(LCD2_ID, OUT_TARGET_CVBS);
        platform_tve100_config_8181t(LCD2_ID, -1, 2);
        break;
      default:
        printk("Invalid value! \n");
    }
    
	return count;
}

/* 
 * VGA
 */
static int proc_read_vga(char *page, char **start, off_t off, int count,
			  int *eof, void *data)
{
	int len = 0;
	int i, value = 0;
	
	for (i = 0; i < LCD_IP_NUM; i ++)
	{
	    if (output_target[i] == OUT_TARGET_VGA)
	    {
	        value = i + 1;
	        break;
	    }
	}
	
	len += sprintf(page, "0:None, 1:First LCD, 2:Second LCD \n");
	len += sprintf(page+len, "current value = %d\n", value);
	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;
	return len;
}

static int proc_write_vga(struct file *file, const char *buffer,
			   unsigned long count, void *data)
{
	int len = count;
	unsigned char value[20];
	uint          tmp;

	if (copy_from_user(value, buffer, len))
		return 0;
	value[len] = '\0';
	sscanf(value, "%u\n", &tmp);
	
    switch (tmp)
    {
      case 1: /* LCD */
        /* we don't allow the same target */
        if (output_target[LCD2_ID] == OUT_TARGET_VGA)
            platform_pmu_switch_pinmux_2(LCD2_ID, OUT_TARGET_DISABLE);
            
        platform_pmu_switch_pinmux_2(LCD_ID, OUT_TARGET_VGA);
        break;
        
      case 2: /* LCD2 */
        /* we don't allow the same target */
        if (output_target[LCD_ID] == OUT_TARGET_VGA)
            platform_pmu_switch_pinmux_2(LCD_ID, OUT_TARGET_DISABLE);
        
        platform_pmu_switch_pinmux_2(LCD2_ID, OUT_TARGET_VGA);
        break;
      default:
        printk("Invalid value! \n");
    }
    
	return count;
}

/* 
 * BT1120
 */
static int proc_read_bt1120(char *page, char **start, off_t off, int count,
			  int *eof, void *data)
{
	int len = 0;
	int i, value = 0;
	
	for (i = 0; i < LCD_IP_NUM; i ++)
	{
	    if (output_target[i] == OUT_TARGET_BT1120)
	    {
	        value = i + 1;
	        break;
	    }
	}
	
	len += sprintf(page, "0:None, 1:First LCD, 2:Second LCD \n");
	len += sprintf(page+len, "current value = %d\n", value);
	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;
	return len;
}

static int proc_write_bt1120(struct file *file, const char *buffer,
			   unsigned long count, void *data)
{
	int len = count;
	unsigned char value[20];
	uint          tmp;

	if (copy_from_user(value, buffer, len))
		return 0;
	value[len] = '\0';
	sscanf(value, "%u\n", &tmp);
	
    switch (tmp)
    {
      case 1: /* LCD */
        /* we don't allow the same target */
        if (output_target[LCD2_ID] == OUT_TARGET_BT1120)
            platform_pmu_switch_pinmux_2(LCD2_ID, OUT_TARGET_DISABLE);
            
        platform_pmu_switch_pinmux_2(LCD_ID, OUT_TARGET_BT1120);
        break;
        
      case 2: /* LCD2 */
        /* we don't allow the same target */
        if (output_target[LCD_ID] == OUT_TARGET_BT1120)
            platform_pmu_switch_pinmux_2(LCD_ID, OUT_TARGET_DISABLE);
            
        platform_pmu_switch_pinmux_2(LCD2_ID, OUT_TARGET_BT1120);
        break;
      default:
        printk("Invalid value! \n");
    }
    
	return count;
}

/* 
 * LCD24
 */
static int proc_read_lcd24(char *page, char **start, off_t off, int count,
			  int *eof, void *data)
{
	int len = 0;
	int i, value = 0;
	
	for (i = 0; i < LCD_IP_NUM; i ++)
	{
	    if (output_target[i] == OUT_TARGET_LCD24)
	    {
	        value = i + 1;
	        break;
	    }
	}
	
	len += sprintf(page, "0:None, 1:First LCD, 2:Second LCD \n");
	len += sprintf(page+len, "current value = %d\n", value);
	*eof = 1;		//end of file
	*start = page + off;
	len = len - off;
	return len;
}

static int proc_write_lcd24(struct file *file, const char *buffer,
			   unsigned long count, void *data)
{
	int len = count;
	unsigned char value[20];	
	uint          tmp;

	if (copy_from_user(value, buffer, len))
		return 0;
	value[len] = '\0';
	sscanf(value, "%u\n", &tmp);
	
    switch (tmp)
    {
      case 1: /* LCD */
        /* we don't allow the same target */
        if (output_target[LCD2_ID] == OUT_TARGET_LCD24)
            platform_pmu_switch_pinmux_2(LCD2_ID, OUT_TARGET_DISABLE);
            
        platform_pmu_switch_pinmux_2(LCD_ID, OUT_TARGET_LCD24);
        break;
        
      case 2: /* LCD2 */
        /* we don't allow the same target */
        if (output_target[LCD_ID] == OUT_TARGET_LCD24)
            platform_pmu_switch_pinmux_2(LCD_ID, OUT_TARGET_DISABLE);
            
        platform_pmu_switch_pinmux_2(LCD2_ID, OUT_TARGET_LCD24);
        break;
      default:
        printk("Invalid value! \n");
    }
    
	return count;
}

/* Init function
 */
int platfrom_8181t_init(void)
{
    int     ret = 0;
    
    CT656_BASE = (unsigned int)ioremap_nocache(CT656_FTCT656_0_PA_BASE, CT656_FTCT656_0_PA_SIZE);
    if (!CT656_BASE)
    {
        printk("LCD: No virtual memory! \n");
        return -1;
    }
    
    /* CVBS
     */
    cvbs_proc = ffb_create_proc_entry("CVBS", S_IRUGO | S_IXUGO, flcd_common_proc);
    if (cvbs_proc == NULL)
    {
        panic("Fail to create CVBS proc!\n");
		ret = -EINVAL;
		goto exit;
	}
	cvbs_proc->read_proc = (read_proc_t *) proc_read_cvbs;
	cvbs_proc->write_proc = (write_proc_t *) proc_write_cvbs;
	cvbs_proc->owner = THIS_MODULE;
	
	/* VGA 
	 */	 
    vga_proc = ffb_create_proc_entry("VGA", S_IRUGO | S_IXUGO, flcd_common_proc);
    if (vga_proc == NULL)
    {
        panic("Fail to create VGA proc!\n");
		ret = -EINVAL;
		goto exit;
	}
	vga_proc->read_proc = (read_proc_t *) proc_read_vga;
	vga_proc->write_proc = (write_proc_t *) proc_write_vga;
	vga_proc->owner = THIS_MODULE;
    
    /* BT1120
     */
    bt1120_proc = ffb_create_proc_entry("BT1120", S_IRUGO | S_IXUGO, flcd_common_proc);
    if (bt1120_proc == NULL)
    {
        panic("Fail to create BT1120 proc!\n");
		ret = -EINVAL;
		goto exit;
	}
	bt1120_proc->read_proc = (read_proc_t *) proc_read_bt1120;
	bt1120_proc->write_proc = (write_proc_t *) proc_write_bt1120;
	bt1120_proc->owner = THIS_MODULE;
    
    /* RGB24
     */
    lcd24_proc = ffb_create_proc_entry("LCD24", S_IRUGO | S_IXUGO, flcd_common_proc);
    if (lcd24_proc == NULL)
    {
        panic("Fail to create LCD24 proc!\n");
		ret = -EINVAL;
		goto exit;
	}
    lcd24_proc->read_proc = (read_proc_t *) proc_read_lcd24;
	lcd24_proc->write_proc = (write_proc_t *) proc_write_lcd24;
	lcd24_proc->owner = THIS_MODULE;
	
exit:
    return ret;
}

/* Cleanup function
 */
void platfrom_8181t_exit(void)
{
    __iounmap((void*)CT656_BASE);
    
    if (cvbs_proc)
        ffb_remove_proc_entry(flcd_common_proc, cvbs_proc);
    if (vga_proc)
        ffb_remove_proc_entry(flcd_common_proc, vga_proc);
    if (bt1120_proc)
        ffb_remove_proc_entry(flcd_common_proc, bt1120_proc);
    if (lcd24_proc)
        ffb_remove_proc_entry(flcd_common_proc, lcd24_proc);
    
    cvbs_proc = vga_proc = bt1120_proc = lcd24_proc = NULL;
}
