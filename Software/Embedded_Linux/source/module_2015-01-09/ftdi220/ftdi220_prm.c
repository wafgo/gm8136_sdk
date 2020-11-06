#include "ftdi220.h"
#include "ftdi220_prm.h"
#include <asm/io.h>

extern struct proc_dir_entry *p_root_proc;
//extern int temporalEn, spatialEn;
extern unsigned int temporal_nr, spatial_nr;
extern unsigned int sec_temporal_nr, sec_spatial_nr;
extern int allStaticEn;
extern int yuyv;

/*
 * Default value
 */
unsigned int default_param[][2] =
{
{ 2, 2 /* 12 */},
{ 3, 2 /* 12 */},
{ 4, 8 /* 16 */},
{ 5, 0},    //rapid_recover
{ 6, 1},    //temporalEn
{ 7, 2},
{ 8, 2},
{ 9, 2},
{ 10, 0},    //spatialEn
{ 11, 1},
{ 12, 6},
{ 13, 1},
{ 14, 9},
{ 15, 1},
{ 16, 63},
{ 17, 1},
{ 18, 1},
{ 19, 1},
{ 20, 1},
{ 21, 40},
{ 22, 40},
{ 23, 8},
{ 24, 1},
{ 25, 4},
};

unsigned int prm_open_set_count[] =
{
	PRM_BUS_CTRL_COUNT,
	RRM_TDN_VAR_COUNT,
	PRM_SDN_COUNT,
	PRM_TDI_MOTION_COUNT,
	PRM_SDI_ELA_COUNT,
	PRM_CMD_COUNT,
	//PRM_ROI_COUNT
	PRM_CMD_COUNT
};


prm_open_t bus_ctrl[PRM_BUS_CTRL_COUNT] =
{
	/* BUS TRAFFIC CTRL */
	{"dma_wait_cycle", BUS_CTRL, 0, 0x00, 16,  0}, /* PRM_BUS_CTRL_DMA_WAIT_CYCLE, */
	{"blk_wait_cycle", BUS_CTRL, 0, 0x00, 16, 16}, /* PRM_BUS_CTRL_BLK_WAIT_CYCLE, */
};

prm_open_t tdn[RRM_TDN_VAR_COUNT] =
{
	/* TEMPORAL DENOISED PARAMETER */
	{"tdn_var_cr", TDP, 0, 0x06,  4, 12},	/* PRM_TDN_CR_VAR, */
	{"tdn_var_cb", TDP, 0, 0x06,  4,  8},	/* PRM_TDN_CB_VAR, */
	{"tdn_var_y",  TDP, 0, 0x08,  5,  0},   /* PRM_TDN_Y_VAR,  */
	{"rapid_recover", TDP, 0, 0x0, 1, 30},  /* PRM_TDN_RAPID_RECOVER */
	{"temporalEn", TDP, 0, 0x1, 1, 31},     /* PRM_TDN_ENABLE */
};

prm_open_t sdn[PRM_SDN_COUNT] =
{
	/* SPATIAL DENOISED PARAMETER */
	{"sdn_line_det", 			SDP, 0, 0x00,  2, 24},  /*	PRM_SDN_LINE_DET_LV, */
	{"sdn_max_filter_level", 	SDP, 0, 0x00,  3,  4},  /*	PRM_SDN_MAX_FILTER_LEVEL,*/ /* 0~4 */
	{"sdn_min_filter_level", 	SDP, 0, 0x00,  3,  0},	/*	PRM_SDN_MIN_FILTER_LEVEL,*/ /* 0~4 */
	{"spatialEn",               SDP, 0, 0x01,  1, 31},  /*  PRM_SDN_ENABLE */
};

prm_open_t tdi_motion[PRM_TDI_MOTION_COUNT] =
{
	/* TEMPORAL DEINTERLACE MOTION PARAMETER */
	{"tdi_line_blk_en", 		MDP, 0, 0x01,  1, 30}, /*	PRM_TDI_LINE_BLK_EN, */
	{"tdi_line_blk_th", 		MDP, 0, 0x06,  7, 16}, /*	PRM_TDI_LINE_BLK_TH, */
	{"tdi_ext_motion_blk_en", 	MDP, 0, 0x01,  1, 29},	/*	PRM_TDI_EXTEND_MOTION_BLK_EN, */
	{"tdi_ext_motion_blk_th", 	MDP, 0, 0x06,  7,  8},	/*	PRM_TDI_EXTEND_MOTION_BLK_TH, */
	{"tdi_motion_blk_en", 		MDP, 0, 0x01,  1, 28},	/*	PRM_TDI_MOTION_BLK_EN, */
	{"tdi_motion_blk_th", 		MDP, 0, 0x06,  7,  0},	/*	PRM_TDI_MOTION_BLK_TH, */
	/* TEMPORAL DEINTERLACE MOTION PARAMETER 2 */
	{"tdi_line_det_en", 				MDP2, 0, 0x00,  1, 31}, /*	PRM_TDI_LINE_DET_EN, */
	{"tdi_motion_prm_static_bnd_det", 	MDP2, 0, 0x01,  1, 30}, /*	PRM_TDI_MOTION_PRM_STATIC_BND_DET,*/
	{"tdi_motion_prm_strong_md_en",   	MDP2, 0, 0x01,  1, 29}, /*	PRM_TDI_MOTION_PRM_STRONG_MD_EN,*/
	{"tdi_motion_prm_auto_th_en", 		MDP2, 0, 0x01,  1, 28}, /*	PRM_TDI_MOTION_PRM_AUTO_TH_EN,*/
	{"tdi_motion_prm_strong_edge", 		MDP2, 0, 0x28,  8, 16}, 	/*	PRM_TDI_MOTION_PRM_STRONG_EDGE,*/
	{"tdi_motion_prm_strong_md_th", 	MDP2, 0, 0x28,  8,  8}, 	/*	PRM_TDI_MOTION_PRM_STRONG_MD_TH,*/
	{"tdi_motion_prm_md_th", 			MDP2, 0, 0x08,  8,  0}, 	/*	PRM_TDI_MOTION_PRM_MD_TH,*/
	/* TEMPORAL DEINTERLACE MOTION PARAMETER 3 */
	{"tdi_motion_prm_static_blk_en", 	MDP3, 0, 0x00,  1, 28}, 	/*	PRM_TDI_MOTION_PRM_STATIC_BLK_EN,*/
	{"tdi_motion_prm_static_th",     	MDP3, 0, 0x0A,  7,  0}, 	/*	PRM_TDI_MOTION_PRM_STATIC_TH,*/
};

prm_open_t sdi_ela[PRM_SDI_ELA_COUNT] =
{
	/* SPATIAL DEINTERLACE PARAMETER */
	{"sdi_ela_adp_en", 		ELAP, 0, 0x01,  1, 20},	/*	PRM_SDI_ELA_ADP_EN, */
	{"sdi_ela_weight_th", 	ELAP, 0, 0x05,  3, 16},	/*	PRM_SDI_ELA_WEIGHT_TH, */
	{"sdi_ela_h_th", 		ELAP, 0, 0x3C,  8,  8},	/*	PPM_SDI_ELA_H_TH, */
	{"sdi_ela_i_th", 		ELAP, 0, 0x14,  8,  0},	/*	PRM_SDI_ELA_I_TH, */
};

prm_open_t roi[PRM_ROI_COUNT] =
{
	/* ROI */
	{"roi_x", SXY, 0, 0x00, 13, 16},	/*	PRM_ROI_X,  */
	{"roi_y", SXY, 0, 0x00, 13,  0},	/*	PRM_ROI_Y,  */
	{"roi_w", OFHW, 0, 0x00, 13, 16},	/*	PRM_ROI_W,  */
	{"roi_h", OFHW, 0, 0x00, 13,  0},	/*	PRM_ROI_H,  */
};

prm_open_t command[] = {
    {"AllStatic", COMD, 0xFF, 0x00, 1, 23},     /* AllStatic */
    {"YUYV", COMD, 0xFF, 0x00, 1, 24},          /* YUYV */
};

prm_open_t* g_p_prm_open_set[PRM_SET_COUNT] =
{
	bus_ctrl,
	tdn,
	sdn,
	tdi_motion,
	sdi_ela,
	command,
	//roi,
};

char g_prm_set_name[PRM_SET_COUNT][256] =
{
	"bus traffic ctrl",
	"temporal de-noise",
	"spatial de-noise",
	"temporal motion paramter",
	"spatial de-interlace ela",
	"command reg",
	//"roi",
};

void set_prm(unsigned int prm_index, unsigned int prm_value, unsigned int proc, unsigned int engine)
{
	unsigned int prm_count = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int prm_open_count = 0;
	unsigned int reg_value = 0;
	unsigned int value = 0;
	unsigned int valid_value = 0;
	unsigned int offset = 0;
	u32 __iomem base;
	prm_open_t* p_prm_open_set = NULL;

	for (i = 0; i < PRM_SET_COUNT; i++)
	{
		p_prm_open_set = g_p_prm_open_set[i];
		prm_open_count = prm_open_set_count[i];

		for (j = 0; j < prm_open_count ; j ++)
		{
			if (prm_count == prm_index)
			{
				p_prm_open_set->is_set = 1;
				p_prm_open_set->value = prm_value;
				offset = p_prm_open_set->offset;

                if (offset == TDP) {
                    u32 width = p_prm_open_set->bit_valid;
                    u32 shift = p_prm_open_set->shift_bit;
                    u32 mask, width_mask;

                    width_mask = (0x1 << width) - 1;
                    mask = ~(width_mask << shift);

                    temporal_nr &= mask;
                    temporal_nr |= ((prm_value & width_mask) << shift);
                }

                if (offset == SDP) {
                    u32 width = p_prm_open_set->bit_valid;
                    u32 shift = p_prm_open_set->shift_bit;
                    u32 mask, width_mask;

                    width_mask = (0x1 << width) - 1;
                    mask = ~(width_mask << shift);
                    spatial_nr &= mask;
                    spatial_nr |= ((prm_value & width_mask) << shift);
                }

                if (offset == COMD) {
                    if (p_prm_open_set->shift_bit == 23)
                        allStaticEn = prm_value;
                    if (p_prm_open_set->shift_bit == 24)
                        yuyv = prm_value;
                }

				if (1)
				{
					base = priv.engine[engine].ftdi220_reg;
					value = ioread32(base + offset);
					valid_value = ((1 << p_prm_open_set->bit_valid) - 1);
					value &= ~(valid_value << p_prm_open_set->shift_bit);
					//printk("value 0x%08X\n", value);
					value |= ((p_prm_open_set->value & valid_value) << p_prm_open_set->shift_bit);
					//printk("value 0x%08X\n", value);
					if ((offset != COMD) && (offset != SDP) && (offset != TDP))
					    iowrite32(value, base + offset);

					if (proc == 1)
					{
					    switch (offset) {
					      case TDP:
					        reg_value = value  = temporal_nr;
					        break;
					      case SDP:
					        reg_value = value  = spatial_nr;
					        break;
					      case COMD:
					        value  = ioread32(base + offset);
					        value &= ~(0x3 << 23);
					        value |= ((yuyv << 24) | (allStaticEn << 23));
					        reg_value = value;
					        break;
					      default:
					        reg_value = value  = ioread32(base + offset);
					        break;
					    }
						value  = (value >> p_prm_open_set->shift_bit);
						value &= valid_value;
						printk("write engine %d %02Xh %s as %d (0x%08X)\n", engine, offset, p_prm_open_set->prm_name, value, reg_value);
					}
				}
				goto exit;
			}
			prm_count++;
			p_prm_open_set++;

		}
	}
	exit:
    return;
}


void set_prm_set(void __iomem * base)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int prm_open_count = 0;
	unsigned int value = 0;
	unsigned int offset = 0;
	prm_open_t* p_prm_open_set = NULL;

	for (i = 0; i < PRM_SET_COUNT; i++)
	{
		p_prm_open_set = g_p_prm_open_set[i];
		prm_open_count = prm_open_set_count[i];
		offset = p_prm_open_set->offset;
		value = ioread32(base + offset);

		for (j = 0; j < prm_open_count; j ++)
		{
			if (p_prm_open_set->is_set)
			{
				value &= ~(((1 << p_prm_open_set->bit_valid) - 1) << p_prm_open_set->shift_bit);
				value |= ((p_prm_open_set->value & p_prm_open_set->bit_valid) << p_prm_open_set->shift_bit);
			}
			p_prm_open_set++;
		}
	}
}


int proc_read_prm(char *page, char **start, off_t off, int count,int *eof, void *data)
{
	u32 __iomem base;
	unsigned int prm_count = 0;
	unsigned int value[FTDI220_MAX_NUM];
	unsigned int len = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int prm_open_count = 0;
	unsigned int offset = 0;
	unsigned int engine = 0;
	char temp[] = "parameter name";
	prm_open_t* p_prm_open_set = NULL;

	len += sprintf(page + len, "\n");
	len += sprintf(page + len, "label, %-30s ", temp);
	len += sprintf(page + len, "value"); /* only show one engine */

	len += sprintf(page + len, "\n");
	for (i = 0; i < PRM_SET_COUNT; i++)
	{
		p_prm_open_set = g_p_prm_open_set[i];
		prm_open_count = prm_open_set_count[i];
		len += sprintf(page + len, "%s\n", g_prm_set_name[i]);
		for (j = 0; j < prm_open_count ; j++)
		{
    		len += sprintf(page + len, "%4d   %-30s ", prm_count, p_prm_open_set->prm_name);
			{
				offset = p_prm_open_set->offset;

				for (engine = 0; engine < priv.eng_num; engine++)
				{
					value[engine] = 0;
					base = priv.engine[engine].ftdi220_reg;
                    /* the following cases should not read from registers. Instead, they are from variables */
                    if (offset == TDP) {
                        value[engine] = temporal_nr;
                        value[engine] = (temporal_nr >> p_prm_open_set->shift_bit) & ((0x1 << p_prm_open_set->bit_valid) - 1);
                    } else if (offset == SDP) {
                        value[engine] = spatial_nr;
    					value[engine] = (spatial_nr >> p_prm_open_set->shift_bit) & ((0x1 << p_prm_open_set->bit_valid) - 1);
                    } else if ((offset == COMD) && (p_prm_open_set->shift_bit == 23)) {
					    value[engine] = allStaticEn;
                    } else if ((offset == COMD) && (p_prm_open_set->shift_bit == 24)) {
					    value[engine] = yuyv;
					} else {
    					value[engine] = ioread32(base + offset);
    					value[engine] &= (((1 << p_prm_open_set->bit_valid) - 1) << p_prm_open_set->shift_bit);
    					value[engine] = value[engine] >> p_prm_open_set->shift_bit;
                    }

					if (engine == 0) /* only show one engine */
					{
    					len += sprintf(page + len, "%6d   ", value[engine]);
					}
					else if (value[engine] != value[0])
					{
						panic("set register of engine %d wrong\n", engine);
					}
				}
    			len += sprintf(page + len, "\n");
			}
			prm_count++;
			p_prm_open_set++;
		}
	}

    *eof = 1;
    return len;
}

int proc_write_prm(struct file *file, const char *buffer, unsigned long count, void *data)
{
	unsigned int prm_index;
	unsigned int prm_value;
	unsigned int engine = 0;

	sscanf(buffer, "%d %d", &prm_index, &prm_value);
	printk("\n");

	for (engine = 0; engine < priv.eng_num; engine ++)
	{
		set_prm(prm_index, prm_value, 1, engine);
	}
	return count;
}

void set_default_param(unsigned int engine)
{
	int i = 0;
	int count = ARRAY_SIZE(default_param);

	for (i = 0; i < count; i++) {
		set_prm(default_param[i][0], default_param[i][1], 0, engine);
	}
}

typedef struct sec_pass_dn {
    int     index;
    char    *name;
    int     start_bit;
    int     end_bit;
} sec_pass_dn_t;

/* Temporal denoised */
sec_pass_dn_t sec_pass_tdn[] = {
    {0, "tdn_var_y", 0, 4},
	{1, "tdn_var_cb", 8, 11},
	{2, "tdn_var_cr", 12, 15},
	{3, "tdn_luma_rapid_recover", 16, 19},
	{4, "tdn_chroma_rapid_recover", 20, 23},
	{5, "tdn_auto_rapid_recover", 29, 29},
	{6, "tdn_rapid_recover", 30, 30},
	{7, "tdn_temporal_en", 31, 31},
};

/* Spatial denoised */
sec_pass_dn_t sec_pass_spt[] = {
    {10, "sdn_min_filter_level", 0, 2},
    {11, "sdn_max_filter_level", 4, 6},
    {12, "sdn_start_threshold", 8, 13},
    {13, "sdn_line_det", 24, 25},
    {14, "sdn_patial_en", 31, 31},
};

/*
 * 2nd pass param
 */
int proc_read_prm2(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    unsigned int len = 0, i, value, width;

    len += sprintf(page + len, "\n");
    len += sprintf(page + len, "Temporal denoised parameter: \n");
    len += sprintf(page + len, "  index  parameter name                  value \n");

    for (i = 0; i < ARRAY_SIZE(sec_pass_tdn); i ++) {
        width = sec_pass_tdn[i].end_bit - sec_pass_tdn[i].start_bit + 1;
        value = (sec_temporal_nr >> sec_pass_tdn[i].start_bit) & ((0x1 << width) - 1);
        len += sprintf(page + len, "  %-5d  %-30s  %d\n", sec_pass_tdn[i].index, sec_pass_tdn[i].name, value);
    }

    len += sprintf(page + len, "\n");
    len += sprintf(page + len, "Spatial denoised parameter: \n");
    len += sprintf(page + len, "  index  parameter name                  value \n");
    for (i = 0; i < ARRAY_SIZE(sec_pass_spt); i ++) {
        width = sec_pass_spt[i].end_bit - sec_pass_spt[i].start_bit + 1;
        value = (sec_spatial_nr >> sec_pass_spt[i].start_bit) & ((0x1 << width) - 1);
        len += sprintf(page + len, "  %-5d  %-30s  %d\n", sec_pass_spt[i].index, sec_pass_spt[i].name, value);
    }

    *eof = 1;
    return len;
}

int proc_write_prm2(struct file *file, const char *buffer, unsigned long count, void *data)
{
    unsigned int prm_index;
	unsigned int prm_value;
	unsigned int i = 0, value, width, mask;

	sscanf(buffer, "%d %d", &prm_index, &prm_value);
	printk("\n");

    /* sec_temporal_nr */
	for (i = 0; i < ARRAY_SIZE(sec_pass_tdn); i ++) {
	    if (sec_pass_tdn[i].index != prm_index)
	        continue;
        width = sec_pass_tdn[i].end_bit - sec_pass_tdn[i].start_bit + 1;
        value = prm_value & ((0x1 << width) - 1);
        mask = ((0x1 << width) - 1) << sec_pass_tdn[i].start_bit;
        sec_temporal_nr &= ~mask;
        sec_temporal_nr |= (value << sec_pass_tdn[i].start_bit);
        break;
	}

    /* sec_spatial_nr */
	for (i = 0; i < ARRAY_SIZE(sec_pass_spt); i ++) {
	    if (sec_pass_spt[i].index != prm_index)
	        continue;
        width = sec_pass_spt[i].end_bit - sec_pass_spt[i].start_bit + 1;
        value = prm_value & ((0x1 << width) - 1);
        mask = ((0x1 << width) - 1) << sec_pass_spt[i].start_bit;
        sec_spatial_nr &= ~mask;
        sec_spatial_nr |= (value << sec_pass_spt[i].start_bit);
        break;
	}

	return count;
}