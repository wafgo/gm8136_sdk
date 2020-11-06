#ifndef __API_IF_H__
#define __API_IF_H__

enum {
    HPF0_5x5_USELPF_BIT = 0,
    HPF1_3x3_USELPF_BIT = 1,
    HPF2_1x5_USELPF_BIT = 2,
    HPF3_1x5_USELPF_BIT = 3,
    HPF4_5x1_USELPF_BIT = 4,
    HPF5_5x1_USELPF_BIT = 5,

    HPF0_5x5_ENABLE_BIT = 8,
    HPF1_3x3_ENABLE_BIT = 9,
    HPF2_1x5_ENABLE_BIT = 10,
    HPF3_1x5_ENABLE_BIT = 11,
    HPF4_5x1_ENABLE_BIT = 12,
    HPF5_5x1_ENABLE_BIT = 13,

    EDG_WT_ENABLE_BIT   = 16,
    NL_GAIN_ENABLE_BIT  = 17,

    HPF0_5x5_USELPF = 1 << HPF0_5x5_USELPF_BIT,
    HPF1_3x3_USELPF = 1 << HPF1_3x3_USELPF_BIT,
    HPF2_1x5_USELPF = 1 << HPF2_1x5_USELPF_BIT,
    HPF3_1x5_USELPF = 1 << HPF3_1x5_USELPF_BIT,
    HPF4_5x1_USELPF = 1 << HPF4_5x1_USELPF_BIT,
    HPF5_5x1_USELPF = 1 << HPF5_5x1_USELPF_BIT,

    HPF0_5x5_ENABLE = 1 << HPF0_5x5_ENABLE_BIT,
    HPF1_3x3_ENABLE = 1 << HPF1_3x3_ENABLE_BIT,
    HPF2_1x5_ENABLE = 1 << HPF2_1x5_ENABLE_BIT,
    HPF3_1x5_ENABLE = 1 << HPF3_1x5_ENABLE_BIT,
    HPF4_5x1_ENABLE = 1 << HPF4_5x1_ENABLE_BIT,
    HPF5_5x1_ENABLE = 1 << HPF5_5x1_ENABLE_BIT,

    EDG_WT_ENABLE   = 1 << EDG_WT_ENABLE_BIT,
    NL_GAIN_ENABLE  = 1 << NL_GAIN_ENABLE_BIT,
};

enum {
    THDNR_ERR_NONE = 0,
    THDNR_ERR_NULL_PTR = -1
};

/*
    MRNR export parameters
*/
#ifndef __DECLARE_MRNR_PARAM
#define __DECLARE_MRNR_PARAM
typedef struct mrnr_param {
    /*  edge direction threshold */
    unsigned short Y_L_edg_th[4][8];
    unsigned short Cb_L_edg_th[4];
    unsigned short Cr_L_edg_th[4];

    /*  edge smooth threshold */
    unsigned short Y_L_smo_th[4][8];
    unsigned short Cb_L_smo_th[4];
    unsigned short Cr_L_smo_th[4];

    /*  nr strength threshold */
    unsigned char Y_L_nr_str[4];
    unsigned char C_L_nr_str[4];
} mrnr_param_t;
#endif

#ifndef __DECLARE_TMNR_PARAM
#define __DECLARE_TMNR_PARAM
typedef struct tmnr_param {
	int Y_var;
	int Cb_var;
	int Cr_var;
	int K;
	int suppress_strength;
	int NF;
	int var_offset;
	int motion_var;
	int trade_thres;
	int auto_recover;
	int rapid_recover;
	int Motion_top_lft_th;
	int Motion_top_nrm_th;
	int Motion_top_rgt_th;
	int Motion_nrm_lft_th;
	int Motion_nrm_nrm_th;
	int Motion_nrm_rgt_th;
	int Motion_bot_lft_th;
	int Motion_bot_nrm_th;
	int Motion_bot_rgt_th;
} tmnr_param_t;
#endif

#ifndef __DECLARE_SP_PARAM
#define __DECLARE_SP_PARAM
typedef struct sp_curve {
    u8  index[4];   // 8-bits
    u8  value[4];   // UFIX1.7
    s16 slope[5];   // SFIX9.7
} sp_curve_t;

typedef struct sp_param {
    bool hpf_enable[6];
    bool hpf_use_lpf[6];
    bool nl_gain_enable;
    bool edg_wt_enable;
    s16 gau_5x5_coeff[25];
    u8  gau_shift;
    s16 hpf0_5x5_coeff[25];
    s16 hpf1_3x3_coeff[9];
    s16 hpf2_1x5_coeff[5];
    s16 hpf3_1x5_coeff[5];
    s16 hpf4_5x1_coeff[5];
    s16 hpf5_5x1_coeff[5];
    u16 hpf_gain[6];
    u8  hpf_shift[6];
    u8  hpf_coring_th[6];
    u8  hpf_bright_clip[6];
    u8  hpf_dark_clip[6];
    u8  hpf_peak_gain[6];
    u8  rec_bright_clip;
    u8  rec_dark_clip;
    u8  rec_peak_gain;
    sp_curve_t nl_gain_curve;
    sp_curve_t edg_wt_curve;
} sp_param_t;
#endif

/*
    MRNR export APIs
*/
void api_set_mrnr_en(bool enable);
int api_set_mrnr_param(mrnr_param_t *pMRNRth);
bool api_get_mnnr_en(void);
int api_get_mrnr_param(mrnr_param_t *pMRNRth);

/*
    TMNR export APIs
*/
void api_set_tmnr_en(bool temporal_en);
void api_set_tmnr_learn_en(bool tnr_learn_en);
void api_set_tmnr_param(int Y_var, int Cb_var, int Cr_var, int motion_var,
                             int trade_thres, int suppress_strength, int K);
void api_set_tmnr_NF(int NF);
void api_set_tmnr_var_offset(int var_offset);
void api_set_tmnr_auto_recover_en(bool auto_recover_en);
void api_set_tmnr_rapid_recover_en(bool rapid_recover_en);
void api_set_tmnr_Motion_th(int Motion_top_lft_th,int Motion_top_nrm_th,
                                  int Motion_top_rgt_th,int Motion_nrm_lft_th,
                                  int Motion_nrm_nrm_th,int Motion_nrm_rgt_th,
                                  int Motion_bot_lft_th,int Motion_bot_nrm_th,
                                  int Motion_bot_rgt_th);
bool api_get_tmnr_en(void);
int api_get_tmnr_param(tmnr_param_t *ptmnr_param);

/*
    Sharpness export APIs
*/
void api_set_sp_en(bool sp_en);
int api_set_sp_param(sp_param_t *psp_param);
void api_set_sp_hpf_en(int idx, bool to_set);
void api_set_sp_hpf_use_lpf(int idx, bool to_set);
void api_set_sp_nl_gain_en(bool to_set);
void api_set_sp_edg_wt_en(bool to_set);
void api_set_sp_gau_coeff(s16 *coeff, u8 shift);
void api_set_sp_hpf_coeff(int idx, s16 *coeff, u8 shift);
void api_set_sp_hpf_gain(int idx, u16 gain);
void api_set_sp_hpf_coring_th(int idx, u8 coring_th);
void api_set_sp_hpf_bright_clip(int idx, u8 bright_clip);
void api_set_sp_hpf_dark_clip(int idx, u8 dark_clip);
void api_set_sp_hpf_peak_gain(int idx, u8 peak_gain);
void api_set_sp_rec_bright_clip(u8 bright_clip);
void api_set_sp_rec_dark_clip(u8 dark_clip);
void api_set_sp_rec_peak_gain(u8 peak_gain);
int api_set_sp_nl_gain_curve(sp_curve_t *pcurve);
int api_set_sp_edg_wt_curve(sp_curve_t *pcurve);
bool api_get_sp_en(void);
int api_get_sp_param(sp_param_t *psp_param);

/*
    Misc export APIs
*/
int api_get_isp_set(u8 *curr_set, int len);
#endif
