#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/interrupt.h>

#include <linux/ioport.h>
#include <linux/platform_device.h>

#include "ft3dnr.h"
#include <api_if.h>

#ifdef VIDEOGRAPH_INC
#include <video_entity.h>   //include video entity manager
#include <log.h>    //include log system "printm","damnit"...
#define MODULE_NAME         "DN"    //<<<<<<<<<< Modify your module name (two bytes) >>>>>>>>>>>>>>
#endif

void api_set_tmnr_en(bool temporal_en)
{
    down(&priv.sema_lock);
    //temproal_en  : enable temporal noise reduction
    priv.isp_param.ctrl.temporal_en = temporal_en;
    priv.isp_param.tmnr_id++;
    up(&priv.sema_lock);
}

bool api_get_tmnr_en(void)
{
    bool val;

    down(&priv.sema_lock);
    val = priv.isp_param.ctrl.temporal_en;
    up(&priv.sema_lock);

    return val;
}

void api_set_tmnr_edg_en(bool tnr_edg_en)
{
    down(&priv.sema_lock);
    //tnr_edg_en   : enable temporal edg detection
    priv.isp_param.ctrl.tnr_edg_en = tnr_edg_en;
    priv.isp_param.tmnr_id++;
    up(&priv.sema_lock);

}

bool api_get_tmnr_edg_en(void)
{
    bool val;

    down(&priv.sema_lock);
    val = priv.isp_param.ctrl.tnr_edg_en;
    up(&priv.sema_lock);

    return val;
}

void api_set_tmnr_learn_en(bool tnr_learn_en)
{
    down(&priv.sema_lock);
    //tnr_learn_en : enable temporal strength learning
    priv.isp_param.ctrl.tnr_learn_en = tnr_learn_en;
    priv.isp_param.tmnr_id++;
    up(&priv.sema_lock);
}

bool api_get_tmnr_learn_en(void)
{
    bool val;

    down(&priv.sema_lock);
    val = priv.isp_param.ctrl.tnr_learn_en;
    up(&priv.sema_lock);

    return val;
}

void api_set_tmnr_param(int Y_var, int Cb_var, int Cr_var)
{
    down(&priv.sema_lock);
    //set Y_var, Cb_var, Cr_var
    priv.isp_param.tmnr.Y_var = Y_var;
    priv.isp_param.tmnr.Cb_var = Cb_var;
    priv.isp_param.tmnr.Cr_var = Cr_var;
    priv.isp_param.tmnr_id++;
    up(&priv.sema_lock);
}

void api_get_tmnr_param(int *pY_var, int *pCb_var, int *pCr_var)
{
    down(&priv.sema_lock);
    *pY_var = priv.isp_param.tmnr.Y_var;
    *pCb_var = priv.isp_param.tmnr.Cb_var;
    *pCr_var = priv.isp_param.tmnr.Cr_var;
    up(&priv.sema_lock);
}

void api_set_tmnr_rlut(unsigned char *r_tab)
{
    int i = 0;

    down(&priv.sema_lock);

    for (i = 0; i < 5; i++) {
        priv.isp_param.rvlut.rlut[i] = (((r_tab[(i << 2) + 3] & 0xff) << 24) | ((r_tab[(i << 2) + 2] & 0xff) << 16) |
                                        ((r_tab[(i << 2) + 1] & 0xff) << 8) | (r_tab[i << 2] & 0xff));
    }
    priv.isp_param.rvlut.rlut[5] = r_tab[20] & 0xff;

    ft3dnr_set_tmnr_rlut();

    up(&priv.sema_lock);
}

void api_get_tmnr_rlut(unsigned char *r_tab)
{
    int i = 0;

    if (r_tab == NULL) {
        panic("%s(%s): input pointer is NULL\n", DRIVER_NAME, __func__);
        return;
    }

    down(&priv.sema_lock);

    for (i = 0; i < 5; i++) {
        r_tab[i << 2] = (unsigned char) (priv.isp_param.rvlut.rlut[i] & 0xFF);
        r_tab[(i << 2) + 1] = (unsigned char) ((priv.isp_param.rvlut.rlut[i] >> 8) & 0xFF);
        r_tab[(i << 2) + 2] = (unsigned char) ((priv.isp_param.rvlut.rlut[i] >> 16) & 0xFF);
        r_tab[(i << 2) + 3] = (unsigned char) ((priv.isp_param.rvlut.rlut[i] >> 24) & 0xFF);
    }
    r_tab[20] = (unsigned char) (priv.isp_param.rvlut.rlut[5] & 0xFF);

    up(&priv.sema_lock);
}

void api_set_tmnr_vlut(unsigned char *v_tab)
{
    int i = 0;

    down(&priv.sema_lock);

    for (i = 0; i < 5; i++) {
        priv.isp_param.rvlut.vlut[i] = (((v_tab[(i << 2) + 3] & 0xff) << 24) | ((v_tab[(i << 2) + 2] & 0xff) << 16) |
                                        ((v_tab[(i << 2) + 1] & 0xff) << 8) | (v_tab[i << 2] & 0xff));
    }
    priv.isp_param.rvlut.vlut[5] = v_tab[20] & 0xff;

    ft3dnr_set_tmnr_vlut();

    up(&priv.sema_lock);
}

void api_get_tmnr_vlut(unsigned char *v_tab)
{
    int i = 0;

    if (v_tab == NULL) {
        panic("%s(%s): input pointer is NULL\n", DRIVER_NAME, __func__);
        return;
    }

    down(&priv.sema_lock);

    for (i = 0; i < 5; i++) {
        v_tab[i << 2] = (unsigned char) (priv.isp_param.rvlut.vlut[i] & 0xFF);
        v_tab[(i << 2) + 1] = (unsigned char) ((priv.isp_param.rvlut.vlut[i] >> 8) & 0xFF);
        v_tab[(i << 2) + 2] = (unsigned char) ((priv.isp_param.rvlut.vlut[i] >> 16) & 0xFF);
        v_tab[(i << 2) + 3] = (unsigned char) ((priv.isp_param.rvlut.vlut[i] >> 24) & 0xFF);
    }
    v_tab[20] = (unsigned char) (priv.isp_param.rvlut.vlut[5] & 0xFF);

    up(&priv.sema_lock);
}

void api_set_tmnr_learn_rate(u8 rate)
{
    down(&priv.sema_lock);

    ft3dnr_set_tmnr_learn_rate(rate);

    up(&priv.sema_lock);
}

u8 api_get_tmnr_learn_rate(void)
{
    u8 val = 0;

    down(&priv.sema_lock);
    val = (u8) (*(volatile unsigned int *)(priv.engine.ft3dnr_reg + 0x98) & 0x7F);
    up(&priv.sema_lock);

    return val;
}

void api_set_tmnr_his_factor(u8 his)
{
    down(&priv.sema_lock);

    ft3dnr_set_tmnr_his_factor(his);

    up(&priv.sema_lock);
}

u8 api_get_tmnr_his_factor(void)
{
    u8 val = 0;

    down(&priv.sema_lock);
    val = (u8) ((*(volatile unsigned int *)(priv.engine.ft3dnr_reg + 0x98) >> 8) & 0xF);
    up(&priv.sema_lock);

    return val;
}

void api_set_tmnr_edge_th(u16 threshold)
{
    down(&priv.sema_lock);

    ft3dnr_set_tmnr_edge_th(threshold);

    up(&priv.sema_lock);
}

u16 api_get_tmnr_edge_th(void)
{
    u16 val = 0;

    down(&priv.sema_lock);
    val = (u16) ((*(volatile unsigned int *)(priv.engine.ft3dnr_reg + 0x98) >> 16) & 0xFFFF);
    up(&priv.sema_lock);

    return val;
}

void api_set_tmnr_dc_offset(u8 offset)
{
    down(&priv.sema_lock);
    priv.isp_param.tmnr.dc_offset = offset;
    priv.isp_param.tmnr_id++;
    up(&priv.sema_lock);
}

u8 api_get_tmnr_dc_offset(void)
{
    u8 offset;

    down(&priv.sema_lock);
    offset = priv.isp_param.tmnr.dc_offset;
    up(&priv.sema_lock);

    return offset;
}

int api_get_version(void)
{
    return VERSION;
}

/*
    MRNR export APIs
*/
void api_set_mrnr_en(bool enable)
{
    down(&priv.sema_lock);
    // set spatial_en enable/disable
    priv.isp_param.ctrl.spatial_en = enable;
    priv.isp_param.mrnr_id++;
    up(&priv.sema_lock);
}

bool api_get_mrnr_en(void)
{
    bool val;

    down(&priv.sema_lock);
    val = priv.isp_param.ctrl.spatial_en;
    up(&priv.sema_lock);

    return val;
}

void api_set_mrnr_param(mrnr_param_t *pMRNRth)
{
    down(&priv.sema_lock);
    // set MRNR threshold parameters
    memcpy(&priv.isp_param.mrnr, pMRNRth, sizeof(mrnr_param_t));
    priv.isp_param.mrnr_id++;
    up(&priv.sema_lock);
}

void api_get_mrnr_param(mrnr_param_t *pMRNRth)
{
    if (pMRNRth == NULL) {
        panic("%s(%s): input pointer is NULL\n", DRIVER_NAME, __func__);
        return;
    }

    down(&priv.sema_lock);
    memset(pMRNRth, 0, sizeof(mrnr_param_t));
    memcpy(pMRNRth, &priv.isp_param.mrnr, sizeof(mrnr_param_t));
    up(&priv.sema_lock);
}

int api_get_isp_set(u8 *curr_set, int len)
{
    int i;

    if (curr_set == NULL)
        return 0;

    down(&priv.sema_lock);
    for (i = 0; (i < len) && (i < sizeof(priv.curr_reg)); i++)
        *(curr_set + i) = priv.curr_reg[i];
    up(&priv.sema_lock);

    return i;
}

EXPORT_SYMBOL(api_set_tmnr_en);
EXPORT_SYMBOL(api_get_tmnr_en);
EXPORT_SYMBOL(api_set_tmnr_edg_en);
EXPORT_SYMBOL(api_get_tmnr_edg_en);
EXPORT_SYMBOL(api_set_tmnr_learn_en);
EXPORT_SYMBOL(api_get_tmnr_learn_en);
EXPORT_SYMBOL(api_set_tmnr_param);
EXPORT_SYMBOL(api_get_tmnr_param);
EXPORT_SYMBOL(api_set_tmnr_rlut);
EXPORT_SYMBOL(api_get_tmnr_rlut);
EXPORT_SYMBOL(api_set_tmnr_vlut);
EXPORT_SYMBOL(api_get_tmnr_vlut);
EXPORT_SYMBOL(api_set_tmnr_learn_rate);
EXPORT_SYMBOL(api_get_tmnr_learn_rate);
EXPORT_SYMBOL(api_set_tmnr_his_factor);
EXPORT_SYMBOL(api_get_tmnr_his_factor);
EXPORT_SYMBOL(api_set_tmnr_edge_th);
EXPORT_SYMBOL(api_get_tmnr_edge_th);
EXPORT_SYMBOL(api_set_tmnr_dc_offset);
EXPORT_SYMBOL(api_get_tmnr_dc_offset);

EXPORT_SYMBOL(api_set_mrnr_en);
EXPORT_SYMBOL(api_get_mrnr_en);
EXPORT_SYMBOL(api_set_mrnr_param);
EXPORT_SYMBOL(api_get_mrnr_param);

EXPORT_SYMBOL(api_get_version);
EXPORT_SYMBOL(api_get_isp_set);
