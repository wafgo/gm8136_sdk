#ifndef __API_IF_H__
#define __API_IF_H__

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

/*
    MRNR export APIs
*/
void api_set_mrnr_en(bool enable);
bool api_get_mrnr_en(void);

void api_set_mrnr_param(mrnr_param_t *pMRNRth);
void api_get_mrnr_param(mrnr_param_t *pMRNRth);

/*
    TMNR export APIs
*/
void api_set_tmnr_en(bool temporal_en);
bool api_get_tmnr_en(void);

void api_set_tmnr_edg_en(bool tnr_edg_en);
bool api_get_tmnr_edg_en(void);

void api_set_tmnr_learn_en(bool tnr_learn_en);
bool api_get_tmnr_learn_en(void);

void api_set_tmnr_param(int Y_var, int Cb_var, int Cr_var);
void api_get_tmnr_param(int *pY_var, int *pCb_var, int *pCr_var);

void api_set_tmnr_rlut(unsigned char *r_tab);
void api_get_tmnr_rlut(unsigned char *r_tab);

void api_set_tmnr_vlut(unsigned char *v_tab);
void api_get_tmnr_vlut(unsigned char *v_tab);

void api_set_tmnr_learn_rate(u8 rate);
u8   api_get_tmnr_learn_rate(void);

void api_set_tmnr_his_factor(u8 his);
u8   api_get_tmnr_his_factor(void);

void api_set_tmnr_edge_th(u16 thresold);
u16  api_get_tmnr_edge_th(void);

void api_set_tmnr_dc_offset(u8 offset);
u8   api_get_tmnr_dc_offset(void);

/* get version */
int api_get_version(void);

int api_get_isp_set(u8 *curr_set, int len);
#endif
