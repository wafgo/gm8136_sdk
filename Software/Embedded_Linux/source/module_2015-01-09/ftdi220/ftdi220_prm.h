#ifndef __FTDI220_PRM_H__
#define __FTDI220_PRM_H__

#include <linux/proc_fs.h>

enum {
	/* BUS TRAFFIC CTRL 0x74 */
	PRM_BUS_CTRL_DMA_WAIT_CYCLE,
	PRM_BUS_CTRL_BLK_WAIT_CYCLE,
	PRM_BUS_CTRL_COUNT
};

enum{
	/* TEMPORAL DENOISED PARAMETER 0x2C*/
	PRM_TDN_CR_VAR,
	PRM_TDN_CB_VAR,
	PRM_TDN_Y_VAR,
	PRM_TDN_RAPID_RECOVER,
	PRM_TDN_ENABLE,
	RRM_TDN_VAR_COUNT
};

enum{
	/* SPATIAL DENOISED PARAMETER 0x30*/
	PRM_SDN_LINE_DET_LV,
	PRM_SDN_MAX_FILTER_LEVEL, /* 0~4 */
	PRM_SDN_MIN_FILTER_LEVEL, /* 0~4 */
	PRM_SDN_ENABLE,
	PRM_SDN_COUNT
};

enum{
	/* TEMPORAL DEINTERLACE MOTION PARAMETER 1 0x34*/
	PRM_TDI_LINE_BLK_EN,
	PRM_TDI_LINE_BLK_TH,
	PRM_TDI_EXTEND_MOTION_BLK_EN,
	PRM_TDI_EXTEND_MOTION_BLK_TH,
	PRM_TDI_MOTION_BLK_EN,
	PRM_TDI_MOTION_BLK_TH,
	/* TEMPORAL DEINTERLACE MOTION PARAMETER 2 0x64*/
	PRM_TDI_LINE_DET_EN,
	PRM_TDI_MOTION_PRM_STATIC_BND_DET,
	PRM_TDI_MOTION_PRM_STRONG_MD_EN,
	PRM_TDI_MOTION_PRM_AUTO_TH_EN,
	PRM_TDI_MOTION_PRM_STRONG_EDGE,
	PRM_TDI_MOTION_PRM_STRONG_MD_TH,
	PRM_TDI_MOTION_PRM_MD_TH,
	/* TEMPORAL DEINTERLACE MOTION PARAMETER 3 0x68*/
	PRM_TDI_MOTION_PRM_STATIC_BLK_EN,
	PRM_TDI_MOTION_PRM_STATIC_TH,
	PRM_TDI_MOTION_COUNT
};

enum{
	/* SPATIAL DEINTERLACE PARAMETER 0x38*/
	PRM_SDI_ELA_ADP_EN,
	PRM_SDI_ELA_WEIGHT_TH,
	PRM_SDI_ELA_H_TH,
	PRM_SDI_ELA_I_TH,
	PRM_SDI_ELA_COUNT,
};
enum{
	/* ROI point 0x70*/
	PRM_ROI_X,
	PRM_ROI_Y,
	/* ROI point 0x6C*/
	PRM_ROI_W,
	PRM_ROI_H,
	PRM_ROI_COUNT
};

enum {
	PRM_SET_BUS_CTRL,
	PRM_SET_TDN_VAR,
	PRM_SET_SDN,
	PRM_SET_TDI_MO,
	PRM_SET_SDI_ELA,
	PRM_SET_CMD,
	//PRM_SET_ROI,
	PRM_SET_COUNT,
};

enum {
    PRM_CMD_ALLSTATIC,
    PRM_CMD_YUYV,
    PRM_CMD_COUNT
};

typedef struct ftdi220_prm_open{
	char prm_name[256];
	unsigned int offset;
	unsigned int is_set;
	unsigned int value;
	unsigned int bit_valid;
	unsigned int shift_bit;
}prm_open_t;

enum{
#if 0
	PRM_PROC_ROOT,
	PRM_PROC_TOC,
#endif
	PRM_PROC_SET,
	PRM_PROC_COUNT
};

typedef struct ftdi220_param_proc{
	char proc_name[256];
	struct proc_dir_entry *parent;
	struct proc_dir_entry *proc_entry;
	int (*p_proc_write)(struct file *file, const char *buffer, unsigned long count, void *data);
	int (*p_proc_read)(char *page, char **start, off_t off, int count,int *eof, void *data);
}prm_proc_t;

int proc_read_prm(char *page, char **start, off_t off, int count,int *eof, void *data);
int proc_write_prm(struct file *file, const char *buffer, unsigned long count, void *data);
int proc_read_prm2(char *page, char **start, off_t off, int count,int *eof, void *data);
int proc_write_prm2(struct file *file, const char *buffer, unsigned long count, void *data);

extern void set_default_param(unsigned int engine);

#endif
