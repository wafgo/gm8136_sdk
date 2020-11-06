/**
 * @file vcap_api.c
 *  vcap300 driver export API
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.23 $
 * $Date: 2014/12/16 09:26:16 $
 *
 * ChangeLog:
 *  $Log: vcap_api.c,v $
 *  Revision 1.23  2014/12/16 09:26:16  jerson_l
 *  1. add md gaussian value check for detect gaussian vaule update error problem when md region switched.
 *  2. add md get all channel tamper status api.
 *
 *  Revision 1.22  2014/09/05 02:48:53  jerson_l
 *  1. support osd force frame mode for GM8136
 *  2. support osd edge smooth edge mode for GM8136
 *  3. support osd auto color change scheme for GM8136
 *
 *  Revision 1.21  2014/02/10 09:52:53  jerson_l
 *  1. support tamper detection new feature for sensitive_black and sensitive_homogeneous threshold
 *
 *  Revision 1.20  2014/02/06 02:11:47  jerson_l
 *  1. support get channel input status api
 *
 *  Revision 1.19  2014/01/24 04:12:50  jerson_l
 *  1. support tamper detection parameter setup
 *
 *  Revision 1.18  2013/10/09 02:54:55  jerson_l
 *  1. add hardware mask border support parameter
 *
 *  Revision 1.17  2013/08/07 01:48:29  jerson_l
 *  1. fix set osd color api deadlock problem again.
 *
 *  Revision 1.16  2013/08/07 01:37:51  jerson_l
 *  1. fix osd set color api deadlock problem
 *
 *  Revision 1.15  2013/08/05 08:21:25  jerson_l
 *  1. fix osd window background color incorrect problem
 *
 *  Revision 1.14  2013/06/24 08:32:54  jerson_l
 *  1. remove unused code
 *  2. add osd window type check in vcap_api_osd_set_color
 *
 *  Revision 1.13  2013/06/18 02:10:06  jerson_l
 *  1. add api for get all channel md information
 *
 *  Revision 1.12  2013/06/14 10:29:09  jerson_l
 *  1. fix osd set character api return fail problem.
 *
 *  Revision 1.11  2013/06/04 11:33:15  jerson_l
 *  1. support osd window as osd_mask window function
 *
 *  Revision 1.10  2013/05/08 08:16:51  jerson_l
 *  1. remove osd row space alignment
 *
 *  Revision 1.9  2013/04/18 09:45:53  jerson_l
 *  1. fix row space and color space setting incorrect issue
 *
 *  Revision 1.8  2013/04/08 12:15:04  jerson_l
 *  1. add v_flip/h_flip set and get export api
 *
 *  Revision 1.7  2013/03/26 02:34:49  jerson_l
 *  1. add dev_per_ch_path_max information to hardware ability
 *
 *  Revision 1.6  2013/03/12 06:37:18  jerson_l
 *  1. add dev_palette_max to indicate hardware palette max number.
 *
 *  Revision 1.5  2013/01/23 03:10:13  jerson_l
 *  1. add interlace to md region structure for specify md operation mode
 *
 *  Revision 1.4  2013/01/02 07:28:01  jerson_l
 *  1. add osd character remove api
 *  2. add md region getting api
 *  3. add hardware ability getting api
 *
 *  Revision 1.3  2012/12/17 03:33:26  jerson_l
 *  1. modify osd window row_sp and col_sp definition
 *
 *  Revision 1.2  2012/12/13 07:33:04  jerson_l
 *  1. use fd to get path information for setup osd and mark window parameter
 *
 *  Revision 1.1  2012/12/11 10:29:50  jerson_l
 *  1. add export api interface support for OSD, Mask, Mark, MD, Shapness, FCS, Denoise
 *
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "bits.h"
#include "vcap_dev.h"
#include "vcap_dbg.h"
#include "vcap_vg.h"
#include "vcap_fcs.h"
#include "vcap_dn.h"
#include "vcap_sharp.h"
#include "vcap_md.h"
#include "vcap_palette.h"
#include "vcap_mask.h"
#include "vcap_mark.h"
#include "vcap_osd.h"
#include "vcap_api.h"
#include "vcap_input.h"

u32 vcap_api_get_version(void)
{
    return VCAP_API_VERSION;
}
EXPORT_SYMBOL(vcap_api_get_version);

int vcap_api_get_hw_ability(hw_ability_t *ability)
{
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!ability || !pdev_info)
        return -1;

    ability->ch_osd_win_max           = VCAP_OSD_WIN_MAX;
    ability->ch_mask_win_max          = VCAP_MASK_WIN_MAX;
    ability->ch_mark_win_max          = VCAP_MARK_WIN_MAX;
    ability->dev_palette_max          = VCAP_PALETTE_MAX;
    ability->dev_osd_mask_palette_max = VCAP_OSD_MASK_PALETTE_MAX;
    ability->dev_osd_char_max         = VCAP_OSD_CHAR_MAX;
    ability->dev_osd_disp_max         = VCAP_OSD_DISP_MAX;
    ability->dev_per_ch_path_max      = VCAP_SCALER_MAX;
#ifdef PLAT_OSD_COLOR_SCHEME
    switch((pdev_info->m_param)->accs_func) {
        case VCAP_ACCS_FUNC_HALF_MARK_MEM:
            ability->dev_mark_ram_size = VCAP_MARK_MEM_SIZE>>1;
            break;
        case VCAP_ACCS_FUNC_FULL_MARK_MEM:
            ability->dev_mark_ram_size = 0;
            break;
        default:
            ability->dev_mark_ram_size = VCAP_MARK_MEM_SIZE;
            break;
    }
#else
    ability->dev_mark_ram_size = VCAP_MARK_MEM_SIZE;
#endif

#ifdef PLAT_MASK_WIN_HOLLOW
    ability->dev_mask_border          = 1;
#else
    ability->dev_mask_border          = 0;
#endif

    return 0;
}
EXPORT_SYMBOL(vcap_api_get_hw_ability);

int vcap_api_fcs_get_param(u32 fd, VCAP_API_FCS_PARAM_T param_id, u32 *data)
{
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get fcs param#%d failed!(fd:%08x invalid)\n", pdev_info->index, param_id, fd);
        return -1;
    }

    if(param_id >= VCAP_API_FCS_PARAM_MAX) {
        vcap_err("[%d] ch#%d get fcs param#%d failed!(param_id => 0~%d)\n", pdev_info->index, ch, param_id, VCAP_API_FCS_PARAM_MAX-1);
        return -1;
    }

    if(!data) {
        vcap_err("[%d] ch#%d get fcs param#%d failed!(data buffer invalid)\n", pdev_info->index, ch, param_id);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);
    switch(param_id) {
        case VCAP_API_FCS_PARAM_LV0_THRED:
            data[0] = vcap_fcs_get_param(pdev_info, ch, VCAP_FCS_PARAM_LV0_THRED);
            break;
        case VCAP_API_FCS_PARAM_LV1_THRED:
            data[0] = vcap_fcs_get_param(pdev_info, ch, VCAP_FCS_PARAM_LV1_THRED);
            break;
        case VCAP_API_FCS_PARAM_LV2_THRED:
            data[0] = vcap_fcs_get_param(pdev_info, ch, VCAP_FCS_PARAM_LV2_THRED);
            break;
        case VCAP_API_FCS_PARAM_LV3_THRED:
            data[0] = vcap_fcs_get_param(pdev_info, ch, VCAP_FCS_PARAM_LV3_THRED);
            break;
        case VCAP_API_FCS_PARAM_LV4_THRED:
            data[0] = vcap_fcs_get_param(pdev_info, ch, VCAP_FCS_PARAM_LV4_THRED);
            break;
        case VCAP_API_FCS_PARAM_GREY_THRED:
            data[0] = vcap_fcs_get_param(pdev_info, ch, VCAP_FCS_PARAM_GREY_THRED);
            break;
        default:
            break;
    }
    up(&pdev_info->ch[ch].sema_lock);

    return 0;
}
EXPORT_SYMBOL(vcap_api_fcs_get_param);

int vcap_api_fcs_set_param(u32 fd, VCAP_API_FCS_PARAM_T param_id, u32 *data)
{
    int ret = 0;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set fcs param#%d failed!(fd:%08x invalid)\n", pdev_info->index, param_id, fd);
        return -1;
    }

    if(param_id >= VCAP_API_FCS_PARAM_MAX) {
        vcap_err("[%d] ch#%d set fcs param#%d failed!(param_id => 0~%d)\n", pdev_info->index, ch, param_id, VCAP_API_FCS_PARAM_MAX-1);
        return -1;
    }

    if(!data) {
        vcap_err("[%d] ch#%d set fcs param#%d failed!(data buffer invalid)\n", pdev_info->index, ch, param_id);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);
    switch(param_id) {
        case VCAP_API_FCS_PARAM_LV0_THRED:
            ret = vcap_fcs_set_param(pdev_info, ch, VCAP_FCS_PARAM_LV0_THRED, data[0]);
            break;
        case VCAP_API_FCS_PARAM_LV1_THRED:
            ret = vcap_fcs_set_param(pdev_info, ch, VCAP_FCS_PARAM_LV1_THRED, data[0]);
            break;
        case VCAP_API_FCS_PARAM_LV2_THRED:
            ret = vcap_fcs_set_param(pdev_info, ch, VCAP_FCS_PARAM_LV2_THRED, data[0]);
            break;
        case VCAP_API_FCS_PARAM_LV3_THRED:
            ret = vcap_fcs_set_param(pdev_info, ch, VCAP_FCS_PARAM_LV3_THRED, data[0]);
            break;
        case VCAP_API_FCS_PARAM_LV4_THRED:
            ret = vcap_fcs_set_param(pdev_info, ch, VCAP_FCS_PARAM_LV4_THRED, data[0]);
            break;
        case VCAP_API_FCS_PARAM_GREY_THRED:
            ret = vcap_fcs_set_param(pdev_info, ch, VCAP_FCS_PARAM_GREY_THRED, data[0]);
            break;
        default:
            break;
    }
    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_fcs_set_param);

int vcap_api_fcs_onoff(u32 fd, int on)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set fcs on/off failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    if(on)
        ret = vcap_fcs_enable(pdev_info, ch);
    else
        ret = vcap_fcs_disable(pdev_info, ch);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_fcs_onoff);

int vcap_api_sharp_get_param(u32 fd, VCAP_API_SHARP_PARAM_T param_id, u32 *data)
{
    int ch   = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    int path = VCAP_VG_FD_MINOR_PATH(ENTITY_FD_MINOR(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX || path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] get sharpness param#%d failed!(fd:%08x invalid)\n", pdev_info->index, param_id, fd);
        return -1;
    }

    if(param_id >= VCAP_API_FCS_PARAM_MAX) {
        vcap_err("[%d] ch#%d get sharpness param#%d failed!(param_id => 0~%d)\n", pdev_info->index, ch, param_id, VCAP_API_SHARP_PARAM_MAX-1);
        return -1;
    }

    if(!data) {
        vcap_err("[%d] ch#%d get sharpness param#%d failed!(data buffer invalid)\n", pdev_info->index, ch, param_id);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);
    switch(param_id) {
        case VCAP_API_SHARP_PARAM_ADAPTIVE_ENB:
            data[0] = vcap_sharp_get_param(pdev_info, ch, path, VCAP_SHARP_PARAM_ADAPTIVE_ENB);
            break;
        case VCAP_API_SHARP_PARAM_RADIUS:
            data[0] = vcap_sharp_get_param(pdev_info, ch, path, VCAP_SHARP_PARAM_RADIUS);
            break;
        case VCAP_API_SHARP_PARAM_AMOUNT:
            data[0] = vcap_sharp_get_param(pdev_info, ch, path, VCAP_SHARP_PARAM_AMOUNT);
            break;
        case VCAP_API_SHARP_PARAM_THRED:
            data[0] = vcap_sharp_get_param(pdev_info, ch, path, VCAP_SHARP_PARAM_THRED);
            break;
        case VCAP_API_SHARP_PARAM_ADAPTIVE_START:
            data[0] = vcap_sharp_get_param(pdev_info, ch, path, VCAP_SHARP_PARAM_ADAPTIVE_START);
            break;
        case VCAP_API_SHARP_PARAM_ADAPTIVE_STEP:
            data[0] = vcap_sharp_get_param(pdev_info, ch, path, VCAP_SHARP_PARAM_ADAPTIVE_STEP);
            break;
        default:
            break;
    }
    up(&pdev_info->ch[ch].sema_lock);

    return 0;
}
EXPORT_SYMBOL(vcap_api_sharp_get_param);

int vcap_api_sharp_set_param(u32 fd, VCAP_API_SHARP_PARAM_T param_id, u32 *data)
{
    int ret  = 0;
    int ch   = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    int path = VCAP_VG_FD_MINOR_PATH(ENTITY_FD_MINOR(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX || path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] set sharpness param#%d failed!(fd:%08x invalid)\n", pdev_info->index, param_id, fd);
        return -1;
    }

    if(param_id >= VCAP_API_FCS_PARAM_MAX) {
        vcap_err("[%d] ch#%d set sharpness param#%d failed!(param_id => 0~%d)\n", pdev_info->index, ch, param_id, VCAP_API_SHARP_PARAM_MAX-1);
        return -1;
    }

    if(!data) {
        vcap_err("[%d] ch#%d set fcs param#%d failed!(data buffer invalid)\n", pdev_info->index, ch, param_id);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);
    switch(param_id) {
        case VCAP_API_SHARP_PARAM_ADAPTIVE_ENB:
            ret = vcap_sharp_set_param(pdev_info, ch, path, VCAP_SHARP_PARAM_ADAPTIVE_ENB, data[0]);
            break;
        case VCAP_API_SHARP_PARAM_RADIUS:
            ret = vcap_sharp_set_param(pdev_info, ch, path, VCAP_SHARP_PARAM_RADIUS, data[0]);
            break;
        case VCAP_API_SHARP_PARAM_AMOUNT:
            ret = vcap_sharp_set_param(pdev_info, ch, path, VCAP_SHARP_PARAM_AMOUNT, data[0]);
            break;
        case VCAP_API_SHARP_PARAM_THRED:
            ret = vcap_sharp_set_param(pdev_info, ch, path, VCAP_SHARP_PARAM_THRED, data[0]);
            break;
        case VCAP_API_SHARP_PARAM_ADAPTIVE_START:
            ret = vcap_sharp_set_param(pdev_info, ch, path, VCAP_SHARP_PARAM_ADAPTIVE_START, data[0]);
            break;
        case VCAP_API_SHARP_PARAM_ADAPTIVE_STEP:
            ret = vcap_sharp_set_param(pdev_info, ch, path, VCAP_SHARP_PARAM_ADAPTIVE_STEP, data[0]);
            break;
        default:
            break;
    }
    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_sharp_set_param);

int vcap_api_dn_get_param(u32 fd, VCAP_API_DN_PARAM_T param_id, u32 *data)
{
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get denoise param#%d failed!(fd:%08x invalid)\n", pdev_info->index, param_id, fd);
        return -1;
    }

    if(param_id >= VCAP_API_DN_PARAM_MAX) {
        vcap_err("[%d] ch#%d get denoise param#%d failed!(param_id => 0~%d)\n", pdev_info->index, ch, param_id, VCAP_API_DN_PARAM_MAX-1);
        return -1;
    }

    if(!data) {
        vcap_err("[%d] ch#%d get denoise param#%d failed!(data buffer invalid)\n", pdev_info->index, ch, param_id);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);
    switch(param_id) {
        case VCAP_API_DN_PARAM_GEOMATRIC:
            data[0] = vcap_dn_get_param(pdev_info, ch, VCAP_DN_PARAM_GEOMATRIC);
            break;
        case VCAP_API_DN_PARAM_SIMILARITY:
            data[0] = vcap_dn_get_param(pdev_info, ch, VCAP_DN_PARAM_SIMILARITY);
            break;
        case VCAP_API_DN_PARAM_ADAPTIVE_ENB:
            data[0] = vcap_dn_get_param(pdev_info, ch, VCAP_DN_PARAM_ADAPTIVE_ENB);
            break;
        case VCAP_API_DN_PARAM_ADAPTIVE_STEP:
            data[0] = vcap_dn_get_param(pdev_info, ch, VCAP_DN_PARAM_ADAPTIVE_STEP);
            break;
        default:
            break;
    }
    up(&pdev_info->ch[ch].sema_lock);

    return 0;
}
EXPORT_SYMBOL(vcap_api_dn_get_param);

int vcap_api_dn_set_param(u32 fd, VCAP_API_DN_PARAM_T param_id, u32 *data)
{
    int ret = 0;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set denoise param#%d failed!(fd:%08x invalid)\n", pdev_info->index, param_id, fd);
        return -1;
    }

    if(param_id >= VCAP_API_DN_PARAM_MAX) {
        vcap_err("[%d] ch#%d set denoise param#%d failed!(param_id => 0~%d)\n", pdev_info->index, ch, param_id, VCAP_API_DN_PARAM_MAX-1);
        return -1;
    }

    if(!data) {
        vcap_err("[%d] ch#%d set denoise param#%d failed!(data buffer invalid)\n", pdev_info->index, ch, param_id);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);
    switch(param_id) {
        case VCAP_API_DN_PARAM_GEOMATRIC:
            ret = vcap_dn_set_param(pdev_info, ch, VCAP_DN_PARAM_GEOMATRIC, data[0]);
            break;
        case VCAP_API_DN_PARAM_SIMILARITY:
            ret = vcap_dn_set_param(pdev_info, ch, VCAP_DN_PARAM_SIMILARITY, data[0]);
            break;
        case VCAP_API_DN_PARAM_ADAPTIVE_ENB:
            ret = vcap_dn_set_param(pdev_info, ch, VCAP_DN_PARAM_ADAPTIVE_ENB, data[0]);
            break;
        case VCAP_API_DN_PARAM_ADAPTIVE_STEP:
            ret = vcap_dn_set_param(pdev_info, ch, VCAP_DN_PARAM_ADAPTIVE_STEP, data[0]);
            break;
        default:
            break;
    }
    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_dn_set_param);

int vcap_api_dn_onoff(u32 fd, int on)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set denoise on/off failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    if(on)
        ret = vcap_dn_enable(pdev_info, ch);
    else
        ret = vcap_dn_disable(pdev_info, ch);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_dn_onoff);

int vcap_api_md_get_event(u32 fd, u32 *even_buf, u32 buf_size)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!even_buf || !pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] ch#%d get md event failed!(fd:%08x invalid)\n", pdev_info->index, ch, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_md_get_event(pdev_info, ch, even_buf, buf_size);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_md_get_event);

int vcap_api_md_get_param(u32 fd, VCAP_API_MD_PARAM_T param_id, u32 *data)
{
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get md param#%d failed!(fd:%08x invalid)\n", pdev_info->index, param_id, fd);
        return -1;
    }

    if(param_id >= VCAP_API_MD_PARAM_MAX) {
        vcap_err("[%d] ch#%d get md param#%d failed!(param_id => 0~%d)\n", pdev_info->index, ch, param_id, VCAP_API_MD_PARAM_MAX-1);
        return -1;
    }

    if(!data) {
        vcap_err("[%d] ch#%d get md param#%d failed!(data buffer invalid)\n", pdev_info->index, ch, param_id);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);
    switch(param_id) {
        case VCAP_API_MD_PARAM_ALPHA:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_ALPHA);
            break;
        case VCAP_API_MD_PARAM_TBG:
            data[0]= vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_TBG);
            break;
        case VCAP_API_MD_PARAM_INIT_VAL:
            data[0]= vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_INIT_VAL);
            break;
        case VCAP_API_MD_PARAM_TB:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_TB);
            break;
        case VCAP_API_MD_PARAM_SIGMA:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_SIGMA);
            break;
        case VCAP_API_MD_PARAM_PRUNE:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_PRUNE);
            break;
        case VCAP_API_MD_PARAM_TAU:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_TAU);
            break;
        case VCAP_API_MD_PARAM_ALPHA_ACCURACY:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_ALPHA_ACCURACY);
            break;
        case VCAP_API_MD_PARAM_TG:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_TG);
            break;
        case VCAP_API_MD_PARAM_DXDY:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_DXDY);
            break;
        case VCAP_API_MD_PARAM_ONE_MIN_ALPHA:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_ONE_MIN_ALPHA);
            break;
        case VCAP_API_MD_PARAM_SHADOW:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_SHADOW);
            break;
        case VCAP_API_MD_PARAM_TAMPER_SENSITIVE_B:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_TAMPER_SENSITIVE_B);
            break;
        case VCAP_API_MD_PARAM_TAMPER_SENSITIVE_H:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_TAMPER_SENSITIVE_H);
            break;
        case VCAP_API_MD_PARAM_TAMPER_HISTOGRAM:
            data[0] = vcap_md_get_param(pdev_info, ch, VCAP_MD_PARAM_TAMPER_HISTOGRAM);
            break;
        default:
            break;
    }
    up(&pdev_info->ch[ch].sema_lock);

    return 0;
}
EXPORT_SYMBOL(vcap_api_md_get_param);

int vcap_api_md_set_param(u32 fd, VCAP_API_MD_PARAM_T param_id, u32 *data)
{
    int ret = 0;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set md param#%d failed!(fd:%08x invalid)\n", pdev_info->index, param_id, fd);
        return -1;
    }

    if(param_id >= VCAP_API_MD_PARAM_MAX) {
        vcap_err("[%d] ch#%d set md param#%d failed!(param_id => 0~%d)\n", pdev_info->index, ch, param_id, VCAP_API_MD_PARAM_MAX-1);
        return -1;
    }

    if(!data) {
        vcap_err("[%d] ch#%d set md param#%d failed!(data buffer invalid)\n", pdev_info->index, ch, param_id);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);
    switch(param_id) {
        case VCAP_API_MD_PARAM_ALPHA:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_ALPHA, data[0]);
            break;
        case VCAP_API_MD_PARAM_TBG:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_TBG, data[0]);
            break;
        case VCAP_API_MD_PARAM_INIT_VAL:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_INIT_VAL, data[0]);
            break;
        case VCAP_API_MD_PARAM_TB:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_TB, data[0]);
            break;
        case VCAP_API_MD_PARAM_SIGMA:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_SIGMA, data[0]);
            break;
        case VCAP_API_MD_PARAM_PRUNE:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_PRUNE, data[0]);
            break;
        case VCAP_API_MD_PARAM_TAU:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_TAU, data[0]);
            break;
        case VCAP_API_MD_PARAM_ALPHA_ACCURACY:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_ALPHA_ACCURACY, data[0]);
            break;
        case VCAP_API_MD_PARAM_TG:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_TG, data[0]);
            break;
        case VCAP_API_MD_PARAM_DXDY:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_DXDY, data[0]);
            break;
        case VCAP_API_MD_PARAM_ONE_MIN_ALPHA:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_ONE_MIN_ALPHA, data[0]);
            break;
        case VCAP_API_MD_PARAM_SHADOW:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_SHADOW, data[0]);
            break;
        case VCAP_API_MD_PARAM_TAMPER_SENSITIVE_B:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_TAMPER_SENSITIVE_B, data[0]);
            break;
        case VCAP_API_MD_PARAM_TAMPER_SENSITIVE_H:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_TAMPER_SENSITIVE_H, data[0]);
            break;
        case VCAP_API_MD_PARAM_TAMPER_HISTOGRAM:
            ret = vcap_md_set_param(pdev_info, ch, VCAP_MD_PARAM_TAMPER_HISTOGRAM, data[0]);
            break;
        default:
            break;
    }
    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_md_set_param);

int vcap_api_md_get_region(u32 fd, struct vcap_api_md_region_t *region)
{
    int ret = 0;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);
    struct vcap_md_region_t md_region;

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get md region failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    if(!region) {
        vcap_err("[%d] get md region failed!(invalid parameter)\n", pdev_info->index);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_md_get_region(pdev_info, ch, &md_region);

    up(&pdev_info->ch[ch].sema_lock);

    if(ret == 0) {
        region->enable    = md_region.enable;
        region->x_start   = md_region.x_start;
        region->y_start   = md_region.y_start;
        region->x_num     = md_region.x_num;
        region->y_num     = md_region.y_num;
        region->x_size    = md_region.x_size;
        region->y_size    = md_region.y_size;
        region->interlace = md_region.interlace;
    }

    return ret;
}
EXPORT_SYMBOL(vcap_api_md_get_region);

int vcap_api_md_get_all_info(u32 fd, struct vcap_api_md_all_info_t *info)
{
    int ret = 0;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(!info || (sizeof(struct vcap_api_md_all_info_t) != sizeof(struct vcap_md_all_info_t))) {
        vcap_err("[%d] get md all info failed!(invalid parameter)\n", pdev_info->index);
        return -1;
    }

    down(&pdev_info->sema_lock);

    ret = vcap_md_get_all_info(pdev_info, (struct vcap_md_all_info_t *)info);

    up(&pdev_info->sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_md_get_all_info);

int vcap_api_md_get_all_tamper(u32 fd, struct vcap_api_md_all_tamper_t *tamper)
{
    int ret = 0;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(!tamper || (sizeof(struct vcap_api_md_all_tamper_t) != sizeof(struct vcap_md_all_tamper_t))) {
        vcap_err("[%d] get md all tamper failed!(invalid parameter)\n", pdev_info->index);
        return -1;
    }

    down(&pdev_info->sema_lock);

    ret = vcap_md_get_all_tamper(pdev_info, (struct vcap_md_all_tamper_t *)tamper);

    up(&pdev_info->sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_md_get_all_tamper);

int vcap_api_mark_set_win(u32 fd, int win_idx, mark_win_param_t *win)
{
    int ret;
    struct vcap_mark_win_t mark_win;
    int ch   = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    int path = VCAP_VG_FD_MINOR_PATH(ENTITY_FD_MINOR(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !win)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX || path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] set mark window failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    mark_win.path    = path;
    mark_win.align   = win->align;
    mark_win.mark_id = win->mark_id;
    mark_win.x_start = win->x_start;
    mark_win.y_start = win->y_start;
    mark_win.zoom    = win->zoom;
    mark_win.alpha   = win->alpha;

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_mark_set_win(pdev_info, ch, win_idx, &mark_win);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_mark_set_win);

int vcap_api_mark_get_win(u32 fd, int win_idx, mark_win_param_t *win)
{
    int ret;
    struct vcap_mark_win_t mark_win;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !win)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get mark window failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_mark_get_win(pdev_info, ch, win_idx, &mark_win);

    up(&pdev_info->ch[ch].sema_lock);

    if(ret < 0)
        return ret;

    win->mark_id = mark_win.mark_id;
    win->x_start = mark_win.x_start;
    win->y_start = mark_win.y_start;
    win->zoom    = mark_win.zoom;
    win->alpha   = mark_win.alpha;

    return ret;
}
EXPORT_SYMBOL(vcap_api_mark_get_win);

int vcap_api_mark_load_image(u32 fd, mark_img_t *img)
{
    int ret;
    struct vcap_mark_img_t mark_img;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    mark_img.id         = img->id;
    mark_img.start_addr = img->start_addr;
    mark_img.x_dim      = img->x_dim;
    mark_img.y_dim      = img->y_dim;
    mark_img.img_buf    = (void *)img->img_buf;

    down(&pdev_info->sema_lock);

    ret = vcap_mark_load_image(pdev_info, &mark_img);

    up(&pdev_info->sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_mark_load_image);

int vcap_api_mark_win_enable(u32 fd, int win_idx)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] enable mark window failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_mark_win_enable(pdev_info, ch, win_idx);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_mark_win_enable);

int vcap_api_mark_win_disable(u32 fd, int win_idx)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] disable mark window failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_mark_win_disable(pdev_info, ch, win_idx);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_mark_win_disable);

int vcap_api_osd_set_char(u32 fd, osd_char_t *osd_char)
{
    int ret;
    struct vcap_osd_char_t o_char;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !osd_char)
        return -1;

    o_char.font = osd_char->font;
    memcpy(o_char.fbitmap, osd_char->fbitmap, sizeof(o_char.fbitmap));

    down(&pdev_info->sema_lock);

    if(PLAT_OSD_PIXEL_BITS == 4) {
        struct vcap_osd_color_t color;

        ret = vcap_osd_get_font_color(pdev_info, &color);
        if(ret < 0)
            goto end;

        ret = vcap_osd_set_char_4bits(pdev_info, &o_char, color.bg_color, color.fg_color);
    }
    else
        ret = vcap_osd_set_char(pdev_info, &o_char);

end:
    up(&pdev_info->sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_char);

int vcap_api_osd_remove_char(u32 fd, int font)
{
    int ret;

    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    down(&pdev_info->sema_lock);

    ret = vcap_osd_remove_char(pdev_info, font);

    up(&pdev_info->sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_remove_char);

int vcap_api_osd_set_disp_string(u32 fd, u32 offset, u16 *font_data, u16 font_num)
{
    int ret;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !font_data)
        return -1;

    down(&pdev_info->sema_lock);

    ret = vcap_osd_set_disp_string(pdev_info, offset, font_data, font_num);

    up(&pdev_info->sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_disp_string);

int vcap_api_osd_set_win(u32 fd, int win_idx, osd_win_param_t *win)
{
    int ret;
    struct vcap_osd_win_t osd_win;
    int ch   = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    int path = VCAP_VG_FD_MINOR_PATH(ENTITY_FD_MINOR(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !win)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX || path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] set osd window#%d failed!(fd:%08x invalid)\n", pdev_info->index, win_idx, fd);
        return -1;
    }

    osd_win.path     = path;
    osd_win.align    = win->align;
    osd_win.x_start  = win->x_start;
    osd_win.y_start  = win->y_start;
    osd_win.width    = win->width;
    osd_win.height   = win->height;
    osd_win.col_sp   = win->v_sp;
    osd_win.row_sp   = win->h_sp;
    osd_win.h_wd_num = win->h_wd_num;
    osd_win.v_wd_num = win->v_wd_num;
    osd_win.wd_addr  = win->wd_addr;

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_set_win(pdev_info, ch, win_idx, &osd_win);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_win);

int vcap_api_osd_get_win(u32 fd, int win_idx, osd_win_param_t *win)
{
    int ret;
    struct vcap_osd_win_t osd_win;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !win)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd window#%d failed!(fd:%08x invalid)\n", pdev_info->index, win_idx, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_get_win(pdev_info, ch, win_idx, &osd_win);

    up(&pdev_info->ch[ch].sema_lock);

    if(ret < 0)
        return ret;

    win->x_start  = osd_win.x_start;
    win->y_start  = osd_win.y_start;
    win->width    = osd_win.width;
    win->height   = osd_win.height;
    win->h_sp     = osd_win.row_sp;
    win->v_sp     = osd_win.col_sp;
    win->h_wd_num = osd_win.h_wd_num;
    win->v_wd_num = osd_win.v_wd_num;
    win->wd_addr  = osd_win.wd_addr;

    return 0;
}
EXPORT_SYMBOL(vcap_api_osd_get_win);

int vcap_api_osd_win_enable(u32 fd, int win_idx)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] enable osd window#%d failed!(fd:%08x invalid)\n", pdev_info->index, win_idx, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_win_enable(pdev_info, ch, win_idx);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_win_enable);

int vcap_api_osd_win_disable(u32 fd, int win_idx)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] disable osd window#%d failed!(fd:%08x invalid)\n", pdev_info->index, win_idx, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_win_disable(pdev_info, ch, win_idx);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_win_disable);

int vcap_api_osd_set_priority(u32 fd, osd_priority_t priority)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd/mark priority failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_set_priority(pdev_info, ch, priority);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_priority);

int vcap_api_osd_get_priority(u32 fd, osd_priority_t *priority)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !priority)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get osd/mark priority failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_get_priority(pdev_info, ch, (VCAP_OSD_PRIORITY_T *)priority);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_get_priority);

int vcap_api_osd_set_font_smooth_param(u32 fd, osd_smooth_t *param)
{
    int ret;
    struct vcap_osd_font_smooth_param_t smooth;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !param)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd font smooth parameter failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    smooth.enable = param->onoff;
    smooth.level  = param->level;

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_set_font_smooth_param(pdev_info, ch, &smooth);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_font_smooth_param);

int vcap_api_osd_get_font_smooth_param(u32 fd, osd_smooth_t *param)
{
    int ret;
    struct vcap_osd_font_smooth_param_t smooth;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !param)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get osd font smooth parameter failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_get_font_smooth_param(pdev_info, ch, &smooth);

    up(&pdev_info->ch[ch].sema_lock);

    if(ret < 0)
        return ret;

    param->onoff = smooth.enable;
    param->level = smooth.level;

    return 0;
}
EXPORT_SYMBOL(vcap_api_osd_get_font_smooth_param);

int vcap_api_osd_set_font_marquee_param(u32 fd, osd_marquee_param_t *param)
{
    int ret;
    struct vcap_osd_marquee_param_t marquee;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !param)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd font marquee parameter failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    marquee.length = param->length;
    marquee.speed  = param->speed;

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_set_font_marquee_param(pdev_info, ch, &marquee);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_font_marquee_param);

int vcap_api_osd_get_font_marquee_param(u32 fd, osd_marquee_param_t *param)
{
    int ret;
    struct vcap_osd_marquee_param_t marquee;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !param)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get osd font marquee parameter failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_get_font_marquee_param(pdev_info, ch, &marquee);

    up(&pdev_info->ch[ch].sema_lock);

    if(ret < 0)
        return -1;

    param->length = marquee.length;
    param->speed  = marquee.speed;

    return 0;
}
EXPORT_SYMBOL(vcap_api_osd_get_font_marquee_param);

int vcap_api_osd_set_font_marquee_mode(u32 fd, int win_idx, osd_marquee_mode_t mode)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd font marquee mode failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_set_font_marquee_mode(pdev_info, ch, win_idx, mode);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_font_marquee_mode);

int vcap_api_osd_get_font_marquee_mode(u32 fd, int win_idx, osd_marquee_mode_t *mode)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !mode)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get osd font marquee mode failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_get_font_marquee_mode(pdev_info, ch, win_idx, (VCAP_OSD_MARQUEE_MODE_T *)mode);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_get_font_marquee_mode);

int vcap_api_osd_set_img_border_color(u32 fd, int color)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd image border failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_set_img_border_color(pdev_info, ch, color);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_img_border_color);

int vcap_api_osd_get_img_border_color(u32 fd, int *color)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !color)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get osd image border failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_get_img_border_color(pdev_info, ch, color);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_get_img_border_color);

int vcap_api_osd_set_img_border_width(u32 fd, int width)
{
    int ret;
    int ch   = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    int path = VCAP_VG_FD_MINOR_PATH(ENTITY_FD_MINOR(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX || path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] set osd image border width failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_set_img_border_width(pdev_info, ch, path, width);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_img_border_width);

int vcap_api_osd_get_img_border_width(u32 fd, int *width)
{
    int ret;
    int ch   = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    int path = VCAP_VG_FD_MINOR_PATH(ENTITY_FD_MINOR(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !width)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX || path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] get osd image border width failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_get_img_border_width(pdev_info, ch, path, width);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_get_img_border_width);

int vcap_api_osd_img_border_enable(u32 fd)
{
    int ret;
    int ch   = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    int path = VCAP_VG_FD_MINOR_PATH(ENTITY_FD_MINOR(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX || path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] enable osd image border failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_img_border_enable(pdev_info, ch, path);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_img_border_enable);

int vcap_api_osd_img_border_disable(u32 fd)
{
    int ret;
    int ch   = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    int path = VCAP_VG_FD_MINOR_PATH(ENTITY_FD_MINOR(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX || path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] enable osd image border failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_img_border_disable(pdev_info, ch, path);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_img_border_disable);

int vcap_api_osd_set_font_zoom(u32 fd, int win_idx, osd_font_zoom_t zoom)
{
    int ret;
    int ch   = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd font zoom failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_set_font_zoom(pdev_info, ch, win_idx, zoom);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_font_zoom);

int vcap_api_osd_get_font_zoom(u32 fd, int win_idx, osd_font_zoom_t *zoom)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get osd font zoom failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_get_font_zoom(pdev_info, ch, win_idx, (VCAP_OSD_FONT_ZOOM_T *)zoom);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_get_font_zoom);

int vcap_api_osd_set_alpha(u32 fd, int win_idx, osd_alpha_t *alpha)
{
    int ret;
    struct vcap_osd_alpha_t osd_alpha;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !alpha)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd alpha failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    osd_alpha.font       = alpha->fg_alpha;
    osd_alpha.background = alpha->bg_alpha;

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_set_alpha(pdev_info, ch, win_idx, &osd_alpha);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_alpha);

int vcap_api_osd_get_alpha(u32 fd, int win_idx, osd_alpha_t *alpha)
{
    int ret;
    struct vcap_osd_alpha_t osd_alpha;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !alpha)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get osd alpha failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_get_alpha(pdev_info, ch, win_idx, &osd_alpha);

    up(&pdev_info->ch[ch].sema_lock);

    if(ret < 0)
        return ret;

    alpha->fg_alpha = osd_alpha.font;
    alpha->bg_alpha = osd_alpha.background;

    return 0;
}
EXPORT_SYMBOL(vcap_api_osd_get_alpha);

int vcap_api_osd_set_border_param(u32 fd, int win_idx, osd_border_param_t *param)
{
    int ret;
    struct vcap_osd_border_param_t border;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !param)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd border parameter failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    border.width = param->width;
    border.color = param->color;
    border.type  = param->type;

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_set_border_param(pdev_info, ch, win_idx, &border);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_border_param);

int vcap_api_osd_get_border_param(u32 fd, int win_idx, osd_border_param_t *param)
{
    int ret;
    struct vcap_osd_border_param_t border;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !param)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get osd border parameter failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_get_border_param(pdev_info, ch, win_idx, &border);

    up(&pdev_info->ch[ch].sema_lock);

    if(ret < 0)
        return ret;

    param->width = border.width;
    param->color = border.color;
    param->type  = border.type;

    return 0;
}
EXPORT_SYMBOL(vcap_api_osd_get_border_param);

int vcap_api_osd_border_enable(u32 fd, int win_idx)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] enable osd border failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_border_enable(pdev_info, ch, win_idx);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_border_enable);

int vcap_api_osd_border_disable(u32 fd, int win_idx)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] disable osd border failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_border_disable(pdev_info, ch, win_idx);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_border_disable);

int vcap_api_osd_set_color(u32 fd, int win_idx, osd_color_t *color)
{
    int ret;
    struct vcap_osd_color_t osd_color;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !color)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd window#%d color failed!(fd:%08x invalid)\n", pdev_info->index, win_idx, fd);
        return -1;
    }

    osd_color.fg_color = color->fg_color;
    osd_color.bg_color = color->bg_color;

    if(PLAT_OSD_PIXEL_BITS == 4) { ///< GM8210
        down(&pdev_info->sema_lock);
        ret = vcap_osd_set_font_color(pdev_info, &osd_color);
        up(&pdev_info->sema_lock);
    }
    else {
        down(&pdev_info->ch[ch].sema_lock);
        ret = vcap_osd_set_win_color(pdev_info, ch, win_idx, &osd_color);
        up(&pdev_info->ch[ch].sema_lock);
    }

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_set_color);

int vcap_api_osd_get_color(u32 fd, int win_idx, osd_color_t *color)
{
    int ret;
    struct vcap_osd_color_t osd_color;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !color)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get osd window#%d color failed!(fd:%08x invalid)\n", pdev_info->index, win_idx, fd);
        return -1;
    }

    if(PLAT_OSD_PIXEL_BITS == 4) {  ///< GM8210
        down(&pdev_info->sema_lock);
        ret = vcap_osd_get_font_color(pdev_info, &osd_color);
        up(&pdev_info->sema_lock);
    }
    else {
        down(&pdev_info->ch[ch].sema_lock);
        ret = vcap_osd_get_win_color(pdev_info, ch, win_idx, &osd_color);
        up(&pdev_info->ch[ch].sema_lock);
    }

    if(ret == 0) {
        color->fg_color = osd_color.fg_color;
        color->fg_color = osd_color.bg_color;
    }

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_get_color);

int vcap_api_palette_set(u32 fd, int idx, u32 crcby)
{
    int ret;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    down(&pdev_info->sema_lock);

    ret = vcap_palette_set(pdev_info, idx, crcby);

    up(&pdev_info->sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_palette_set);

int vcap_api_palette_get(u32 fd, int idx, u32 *crcby)
{
    int ret;
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !crcby)
        return -1;

    down(&pdev_info->sema_lock);

    ret = vcap_palette_get(pdev_info, idx, crcby);

    up(&pdev_info->sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_palette_get);

int vcap_api_mask_set_win(u32 fd, int win_idx, mask_win_param_t *win)
{
    int ret;
    struct vcap_mask_win_t mask_win;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !win)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set mask window failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    mask_win.x_start = win->x_start;
    mask_win.y_start = win->y_start;
    mask_win.width   = win->width;
    mask_win.height  = win->height;
    mask_win.color   = win->color;

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_mask_set_win(pdev_info, ch, win_idx, &mask_win);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_mask_set_win);

int vcap_api_mask_get_win(u32 fd, int win_idx, mask_win_param_t *win)
{
    int ret;
    struct vcap_mask_win_t mask_win;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !win)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get mask window failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_mask_get_win(pdev_info, ch, win_idx, &mask_win);

    up(&pdev_info->ch[ch].sema_lock);

    if(ret < 0)
        return ret;

    win->x_start = mask_win.x_start;
    win->y_start = mask_win.y_start;
    win->width   = mask_win.width;
    win->height  = mask_win.height;
    win->color   = mask_win.color;

    return ret;
}
EXPORT_SYMBOL(vcap_api_mask_get_win);

int vcap_api_mask_set_alpha(u32 fd, int win_idx, alpha_t alpha)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set mask window alpha failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_mask_set_alpha(pdev_info, ch, win_idx, alpha);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_mask_set_alpha);

int vcap_api_mask_get_alpha(u32 fd, int win_idx, alpha_t *alpha)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !alpha)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get mask window alpha failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_mask_get_alpha(pdev_info, ch, win_idx, (VCAP_MASK_ALPHA_T *)alpha);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_mask_get_alpha);

int vcap_api_mask_set_border(u32 fd, int win_idx, mask_border_t *border)
{
    int ret;
    struct vcap_mask_border_t mask_border;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !border)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set mask window border failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    mask_border.width = border->width;
    mask_border.type  = border->type;

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_mask_set_border(pdev_info, ch, win_idx, &mask_border);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_mask_set_border);

int vcap_api_mask_get_border(u32 fd, int win_idx, mask_border_t *border)
{
    int ret;
    struct vcap_mask_border_t mask_border;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !border)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get mask window border failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_mask_get_border(pdev_info, ch, win_idx, &mask_border);

    up(&pdev_info->ch[ch].sema_lock);

    if(ret < 0)
        return ret;

    border->width = mask_border.width;
    border->type  = mask_border.type;

    return 0;
}
EXPORT_SYMBOL(vcap_api_mask_get_border);

int vcap_api_mask_enable(u32 fd, int win_idx)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] enable mask window failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_mask_enable(pdev_info, ch, win_idx);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_mask_enable);

int vcap_api_mask_disable(u32 fd, int win_idx)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] disable mask window failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_mask_disable(pdev_info, ch, win_idx);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_mask_disable);

int vcap_api_flip_set_param(u32 fd, VCAP_API_FLIP_T flip)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set flip parameter failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    switch(flip) {
        case VCAP_API_FLIP_V:
            ret = vcap_dev_set_ch_flip(pdev_info, ch, 1, 0);
            break;
        case VCAP_API_FLIP_H:
            ret = vcap_dev_set_ch_flip(pdev_info, ch, 0, 1);
            break;
        case VCAP_API_FLIP_V_H:
            ret = vcap_dev_set_ch_flip(pdev_info, ch, 1, 1);
            break;
        default:    /* Disable */
            ret = vcap_dev_set_ch_flip(pdev_info, ch, 0, 0);
            break;
    }

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_flip_set_param);

int vcap_api_flip_get_param(u32 fd, VCAP_API_FLIP_T *flip)
{
    int ret;
    int v_flip, h_flip;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get flip parameter failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    if(!flip) {
        vcap_err("[%d] ch#%d get flip parameter failed!\n", pdev_info->index, ch);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_dev_get_ch_flip(pdev_info, ch, &v_flip, &h_flip);

    up(&pdev_info->ch[ch].sema_lock);

    if(ret == 0) {
        if(v_flip) {
            if(h_flip)
                *flip = VCAP_API_FLIP_V_H;
            else
                *flip = VCAP_API_FLIP_V;
        }
        else {
            if(h_flip)
                *flip = VCAP_API_FLIP_H;
            else
                *flip = VCAP_API_FLIP_NONE;
        }
    }

    return ret;
}
EXPORT_SYMBOL(vcap_api_flip_get_param);

int vcap_api_osd_mask_win_enable(u32 fd, int win_idx)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] enable osd_mask window#%d failed!(fd:%08x invalid)\n", pdev_info->index, win_idx, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_win_enable(pdev_info, ch, win_idx);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_mask_win_enable);

int vcap_api_osd_mask_win_disable(u32 fd, int win_idx)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] disable osd_mask window#%d failed!(fd:%08x invalid)\n", pdev_info->index, win_idx, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_win_disable(pdev_info, ch, win_idx);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_mask_win_disable);

int vcap_api_osd_mask_set_win(u32 fd, int win_idx, osd_mask_win_param_t *win)
{
    int ret;
    struct vcap_osd_mask_win_t osd_mask_win;
    int ch   = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    int path = VCAP_VG_FD_MINOR_PATH(ENTITY_FD_MINOR(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !win)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX || path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] set osd_mask window#%d failed!(fd:%08x invalid)\n", pdev_info->index, win_idx, fd);
        return -1;
    }

    osd_mask_win.path     = path;
    osd_mask_win.align    = win->align;
    osd_mask_win.x_start  = win->x_start;
    osd_mask_win.y_start  = win->y_start;
    osd_mask_win.width    = win->width;
    osd_mask_win.height   = win->height;
    osd_mask_win.color    = win->color;

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_mask_set_win(pdev_info, ch, win_idx, &osd_mask_win);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_mask_set_win);

int vcap_api_osd_mask_get_win(u32 fd, int win_idx, osd_mask_win_param_t *win)
{
    int ret;
    struct vcap_osd_mask_win_t osd_mask_win;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !win)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd_mask window#%d failed!(fd:%08x invalid)\n", pdev_info->index, win_idx, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_mask_get_win(pdev_info, ch, win_idx, &osd_mask_win);

    up(&pdev_info->ch[ch].sema_lock);

    if(ret < 0)
        return ret;

    win->align    = osd_mask_win.align;
    win->x_start  = osd_mask_win.x_start;
    win->y_start  = osd_mask_win.y_start;
    win->width    = osd_mask_win.width;
    win->height   = osd_mask_win.height;
    win->color    = osd_mask_win.color;

    return 0;
}
EXPORT_SYMBOL(vcap_api_osd_mask_get_win);

int vcap_api_osd_mask_set_alpha(u32 fd, int win_idx, alpha_t alpha)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd_mask window alpha failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_mask_set_alpha(pdev_info, ch, win_idx, alpha);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_mask_set_alpha);

int vcap_api_osd_mask_get_alpha(u32 fd, int win_idx, alpha_t *alpha)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info || !alpha)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get osd_mask window alpha failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_mask_get_alpha(pdev_info, ch, win_idx, (VCAP_OSD_ALPHA_T *)alpha);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
}
EXPORT_SYMBOL(vcap_api_osd_mask_get_alpha);

int vcap_api_input_get_status(u32 fd, struct vcap_api_input_status_t *sts)
{
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t     *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);
    struct vcap_input_status_t input_sts;
    struct vcap_input_dev_t    *pinput;

    if(!pdev_info || !sts)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get input status failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    pinput = vcap_input_get_info(CH_TO_VI(ch));
    if(!pinput) {
        vcap_err("[%d] get input status failed!(no input device attach to fd:%08x)\n", pdev_info->index, fd);
        return -1;
    }

    ret = vcap_input_device_get_status(pinput, (ch%VCAP_INPUT_DEV_CH_MAX), &input_sts);
    if(ret < 0)
        goto exit;

    sts->vlos   = input_sts.vlos;
    sts->width  = input_sts.width;
    sts->height = input_sts.height;
    sts->fps    = input_sts.fps;

exit:
    return ret;
}
EXPORT_SYMBOL(vcap_api_input_get_status);

int vcap_api_osd_frame_mode_enable(u32 fd)
{
#ifdef PLAT_OSD_FRAME_MODE
    int ret;
    int ch   = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    int path = VCAP_VG_FD_MINOR_PATH(ENTITY_FD_MINOR(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX || path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] enable osd frame mode failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_frame_mode_enable(pdev_info, ch, path);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
#else
    vcap_err("enable osd frame mode failed!(hardware not support)\n");
    return -1;
#endif
}
EXPORT_SYMBOL(vcap_api_osd_frame_mode_enable);

int vcap_api_osd_frame_mode_disable(u32 fd)
{
#ifdef PLAT_OSD_FRAME_MODE
    int ret;
    int ch   = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    int path = VCAP_VG_FD_MINOR_PATH(ENTITY_FD_MINOR(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX || path >= VCAP_SCALER_MAX) {
        vcap_err("[%d] enable osd frame mode failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_frame_mode_disable(pdev_info, ch, path);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
#else
    vcap_err("disable osd frame mode failed!(hardware not support)\n");
    return -1;
#endif
}
EXPORT_SYMBOL(vcap_api_osd_frame_mode_disable);

int vcap_api_osd_win_accs_enable(u32 fd, int win_idx)
{
#ifdef PLAT_OSD_COLOR_SCHEME
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] enable osd window#%d auto color change scheme failed!(fd:%08x invalid)\n", pdev_info->index, win_idx, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_accs_win_enable(pdev_info, ch, win_idx);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
#else
    vcap_err("enable osd auto color change scheme failed!(hardware not support)\n");
    return -1;
#endif
}
EXPORT_SYMBOL(vcap_api_osd_win_accs_enable);

int vcap_api_osd_win_accs_disable(u32 fd, int win_idx)
{
#ifdef PLAT_OSD_COLOR_SCHEME
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] disable osd window#%d auto color change scheme failed!(fd:%08x invalid)\n", pdev_info->index, win_idx, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_accs_win_disable(pdev_info, ch, win_idx);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
#else
    vcap_err("disable osd auto color change scheme failed!(hardware not support)\n");
    return -1;
#endif
}
EXPORT_SYMBOL(vcap_api_osd_win_accs_disable);

int vcap_api_osd_set_accs_data_thres(u32 fd, int thres)
{
#ifdef PLAT_OSD_COLOR_SCHEME
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd auto color change scheme data threshold failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_set_accs_data_thres(pdev_info, ch, thres);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
#else
    vcap_err("set osd auto color change scheme data threshold failed!(hardware not support)\n");
    return -1;
#endif
}
EXPORT_SYMBOL(vcap_api_osd_set_accs_data_thres);

int vcap_api_osd_get_accs_data_thres(u32 fd, int *thres)
{
#ifdef PLAT_OSD_COLOR_SCHEME
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!thres || !pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get osd auto color change scheme data threshold failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_get_accs_data_thres(pdev_info, ch, thres);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
#else
    vcap_err("get osd auto color change scheme data threshold failed!(hardware not support)\n");
    return -1;
#endif
}
EXPORT_SYMBOL(vcap_api_osd_get_accs_data_thres);

int vcap_api_osd_set_font_edge_mode(u32 fd, VCAP_API_OSD_FONT_EDGE_T edge_mode)
{
#ifdef PLAT_OSD_EDGE_SMOOTH
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] set osd font edge smooth mode failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_set_font_edge_mode(pdev_info, ch, (VCAP_OSD_FONT_EDGE_MODE_T)edge_mode);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
#else
    vcap_err("set osd font edge smooth mode failed!(hardware not support)\n");
    return -1;
#endif
}
EXPORT_SYMBOL(vcap_api_osd_set_font_edge_mode);

int vcap_api_osd_get_font_edge_mode(u32 fd, VCAP_API_OSD_FONT_EDGE_T *edge_mode)
{
#ifdef PLAT_OSD_EDGE_SMOOTH
    int ret;
    int ch = VCAP_VG_FD_ENGINE_CH(ENTITY_FD_ENGINE(fd));
    struct vcap_dev_info_t *pdev_info = (struct vcap_dev_info_t *)vcap_dev_get_dev_info(0);

    if(!pdev_info)
        return -1;

    if(ch >= VCAP_CHANNEL_MAX) {
        vcap_err("[%d] get osd font edge smooth mode failed!(fd:%08x invalid)\n", pdev_info->index, fd);
        return -1;
    }

    down(&pdev_info->ch[ch].sema_lock);

    ret = vcap_osd_get_font_edge_mode(pdev_info, ch, (VCAP_OSD_FONT_EDGE_MODE_T *)edge_mode);

    up(&pdev_info->ch[ch].sema_lock);

    return ret;
#else
    vcap_err("get osd font edge smooth mode failed!(hardware not support)\n");
    return -1;
#endif
}
EXPORT_SYMBOL(vcap_api_osd_get_font_edge_mode);
