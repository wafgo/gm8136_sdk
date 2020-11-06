/*
   Reads the AAC bitstream as defined in 14496-3 (MPEG-4 Audio)
*/
#include "common.h"
#include "structs.h"
//#include <stdlib.h>
//#include <string.h>
#include <linux/types.h>
#include <linux/string.h>
#include "decoder.h"
#include "syntax.h"
#include "specrec.h"
#include "huffman.h"
#include "bits.h"
#include "pulse.h"
#include "drc.h"

/* static function declarations */
static void decode_sce_lfe(GMAACDecHandle hDecoder, GMAACDecInfo *hInfo, bitfile *ld, char id_syn_ele);
static void decode_cpe(GMAACDecHandle hDecoder, GMAACDecInfo *hInfo, bitfile *ld, char id_syn_ele);
static char single_lfe_channel_element(GMAACDecHandle hDecoder, bitfile *ld, char channel, char *tag);
#if 0
static char channel_pair_element(GMAACDecHandle hDecoder, bitfile *ld, char channel, char *tag);
#else
//Shawn 2004.10.12
static int channel_pair_element(GMAACDecHandle hDecoder, bitfile *ld, char channel, char *tag);
#endif
#ifdef COUPLING_DEC
static char coupling_channel_element(GMAACDecHandle hDecoder, bitfile *ld);
#endif
static unsigned short data_stream_element(GMAACDecHandle hDecoder, bitfile *ld);
static char program_config_element(program_config *pce, bitfile *ld);
static char fill_element(GMAACDecHandle hDecoder, bitfile *ld, drc_info *drc);
static char individual_channel_stream(GMAACDecHandle hDecoder, element *ele, bitfile *ld, ic_stream *ics, char scal_flag, short *spec_data);
static char ics_info(GMAACDecHandle hDecoder, ic_stream *ics, bitfile *ld, char common_window);
#if 0
static char section_data(GMAACDecHandle hDecoder, ic_stream *ics, bitfile *ld);
static char scale_factor_data(GMAACDecHandle hDecoder, ic_stream *ics, bitfile *ld);
#else
//Shawn 2004.10.6 remove profile
static char section_data(ic_stream *ics, bitfile *ld);
static char scale_factor_data(ic_stream *ics, bitfile *ld);
#endif
static char spectral_data(GMAACDecHandle hDecoder, ic_stream *ics, bitfile *ld, short *spectral_data);
static unsigned short extension_payload(bitfile *ld, drc_info *drc, unsigned short count);
static char pulse_data(ic_stream *ics, pulse_info *pul, bitfile *ld);
static void tns_data(ic_stream *ics, tns_info *tns, bitfile *ld);
#ifdef LTP_DEC
static char ltp_data(GMAACDecHandle hDecoder, ic_stream *ics, ltp_info *ltp, bitfile *ld);
#endif
static char adts_fixed_header(adts_header *adts, bitfile *ld);
static void adts_variable_header(adts_header *adts, bitfile *ld);
//static void adts_error_check(adts_header *adts, bitfile *ld);
static char dynamic_range_info(bitfile *ld, drc_info *drc);
static char excluded_channels(bitfile *ld, drc_info *drc);

extern real_t time_out_buf[2*1024];

/* Table 4.4.1 */
signed char GASpecificConfig(bitfile *ld, mp4AudioSpecificConfig *mp4ASC, program_config *pce_out)
{
    program_config pce;

    /* 1024 or 960 */
    mp4ASC->frameLengthFlag = faad_get1bit(ld);

    if (mp4ASC->frameLengthFlag == 1)
        return -3;

    mp4ASC->dependsOnCoreCoder = faad_get1bit(ld);
    if (mp4ASC->dependsOnCoreCoder == 1)
    {
        mp4ASC->coreCoderDelay = (unsigned short)faad_getbits(ld, 14);
    }

    mp4ASC->extensionFlag = faad_get1bit(ld);
    if (mp4ASC->channelsConfiguration == 0)
    {
        if (program_config_element(&pce, ld))
            return -3;
        //mp4ASC->channelsConfiguration = pce.channels;

        if (pce_out != NULL)
            memcpy(pce_out, &pce, sizeof(program_config));

        /*
        if (pce.num_valid_cc_elements)
            return -3;
        */
    }

    return 0;
}

/* Table 4.4.2 */
/* An MPEG-4 Audio decoder is only required to follow the Program
   Configuration Element in GASpecificConfig(). The decoder shall ignore
   any Program Configuration Elements that may occur in raw data blocks.
   PCEs transmitted in raw data blocks cannot be used to convey decoder
   configuration information.
*/
#if 0
static char program_config_element(program_config *pce, bitfile *ld)
{
    char i;

    memset(pce, 0, sizeof(program_config));

    pce->channels = 0;

    pce->element_instance_tag = (char)faad_getbits(ld, 4);

    pce->object_type = (char)faad_getbits(ld, 2);
    pce->sf_index = (char)faad_getbits(ld, 4);
    pce->num_front_channel_elements = (char)faad_getbits(ld, 4);
    pce->num_side_channel_elements = (char)faad_getbits(ld, 4);
    pce->num_back_channel_elements = (char)faad_getbits(ld, 4);
    pce->num_lfe_channel_elements = (char)faad_getbits(ld, 2);
    pce->num_assoc_data_elements = (char)faad_getbits(ld, 3);
    pce->num_valid_cc_elements = (char)faad_getbits(ld, 4);

    pce->mono_mixdown_present = faad_get1bit(ld);
    if (pce->mono_mixdown_present == 1)
    {
        pce->mono_mixdown_element_number = (char)faad_getbits(ld, 4);
    }

    pce->stereo_mixdown_present = faad_get1bit(ld);
    if (pce->stereo_mixdown_present == 1)
    {
        pce->stereo_mixdown_element_number = (char)faad_getbits(ld, 4);
    }

    pce->matrix_mixdown_idx_present = faad_get1bit(ld);
    if (pce->matrix_mixdown_idx_present == 1)
    {
        pce->matrix_mixdown_idx = (char)faad_getbits(ld, 2);
        pce->pseudo_surround_enable = faad_get1bit(ld);
    }

    for (i = 0; i < pce->num_front_channel_elements; i++)
    {
        pce->front_element_is_cpe[i] = faad_get1bit(ld);
        pce->front_element_tag_select[i] = (char)faad_getbits(ld, 4);

        if (pce->front_element_is_cpe[i] & 1)
        {
            pce->cpe_channel[pce->front_element_tag_select[i]] = pce->channels;
            pce->num_front_channels += 2;
            pce->channels += 2;
        } else {
            pce->sce_channel[pce->front_element_tag_select[i]] = pce->channels;
            pce->num_front_channels++;
            pce->channels++;
        }
    }

    for (i = 0; i < pce->num_side_channel_elements; i++)
    {
        pce->side_element_is_cpe[i] = faad_get1bit(ld);
        pce->side_element_tag_select[i] = (char)faad_getbits(ld, 4);

        if (pce->side_element_is_cpe[i] & 1)
        {
            pce->cpe_channel[pce->side_element_tag_select[i]] = pce->channels;
            pce->num_side_channels += 2;
            pce->channels += 2;
        } else {
            pce->sce_channel[pce->side_element_tag_select[i]] = pce->channels;
            pce->num_side_channels++;
            pce->channels++;
        }
    }

    for (i = 0; i < pce->num_back_channel_elements; i++)
    {
        pce->back_element_is_cpe[i] = faad_get1bit(ld);
        pce->back_element_tag_select[i] = (char)faad_getbits(ld, 4);

        if (pce->back_element_is_cpe[i] & 1)
        {
            pce->cpe_channel[pce->back_element_tag_select[i]] = pce->channels;
            pce->channels += 2;
            pce->num_back_channels += 2;
        } else {
            pce->sce_channel[pce->back_element_tag_select[i]] = pce->channels;
            pce->num_back_channels++;
            pce->channels++;
        }
    }

    for (i = 0; i < pce->num_lfe_channel_elements; i++)
    {
        pce->lfe_element_tag_select[i] = (char)faad_getbits(ld, 4);

        pce->sce_channel[pce->lfe_element_tag_select[i]] = pce->channels;
        pce->num_lfe_channels++;
        pce->channels++;
    }

    for (i = 0; i < pce->num_assoc_data_elements; i++)
        pce->assoc_data_element_tag_select[i] = (char)faad_getbits(ld, 4);

    for (i = 0; i < pce->num_valid_cc_elements; i++)
    {
        pce->cc_element_is_ind_sw[i] = faad_get1bit(ld);
        pce->valid_cc_element_tag_select[i] = (char)faad_getbits(ld, 4);
    }

    faad_byte_align(ld);

    pce->comment_field_bytes = (char)faad_getbits(ld, 8);

    for (i = 0; i < pce->comment_field_bytes; i++)
    {
        pce->comment_field_data[i] = (char)faad_getbits(ld, 8);
    }
    pce->comment_field_data[i] = 0;

    if (pce->channels > MAX_CHAN)
        return 22;

    return 0;
}
#else
static char program_config_element(program_config *pce, bitfile *ld)
{
    int i;

    memset(pce, 0, sizeof(program_config));

    pce->channels = 0;

    pce->element_instance_tag = (char)faad_getbits(ld, 4);

    pce->object_type = (char)faad_getbits(ld, 2);
    pce->sf_index = (char)faad_getbits(ld, 4);
    pce->num_front_channel_elements = (char)faad_getbits(ld, 4);
    pce->num_side_channel_elements = (char)faad_getbits(ld, 4);
    pce->num_back_channel_elements = (char)faad_getbits(ld, 4);
    pce->num_lfe_channel_elements = (char)faad_getbits(ld, 2);
    pce->num_assoc_data_elements = (char)faad_getbits(ld, 3);
    pce->num_valid_cc_elements = (char)faad_getbits(ld, 4);

    pce->mono_mixdown_present = faad_get1bit(ld);
    if (pce->mono_mixdown_present == 1)
    {
        pce->mono_mixdown_element_number = (char)faad_getbits(ld, 4);
    }

    pce->stereo_mixdown_present = faad_get1bit(ld);
    if (pce->stereo_mixdown_present == 1)
    {
        pce->stereo_mixdown_element_number = (char)faad_getbits(ld, 4);
    }

    pce->matrix_mixdown_idx_present = faad_get1bit(ld);
    if (pce->matrix_mixdown_idx_present == 1)
    {
        pce->matrix_mixdown_idx = (char)faad_getbits(ld, 2);
        pce->pseudo_surround_enable = faad_get1bit(ld);
    }

    for (i = 0; i < pce->num_front_channel_elements; i++)
    {
        pce->front_element_is_cpe[i] = faad_get1bit(ld);
        pce->front_element_tag_select[i] = (char)faad_getbits(ld, 4);

        if (pce->front_element_is_cpe[i] & 1)
        {
            pce->cpe_channel[pce->front_element_tag_select[i]] = pce->channels;
            pce->num_front_channels += 2;
            pce->channels += 2;
        } else {
            pce->sce_channel[pce->front_element_tag_select[i]] = pce->channels;
            pce->num_front_channels++;
            pce->channels++;
        }
    }

    for (i = 0; i < pce->num_side_channel_elements; i++)
    {
        pce->side_element_is_cpe[i] = faad_get1bit(ld);
        pce->side_element_tag_select[i] = (char)faad_getbits(ld, 4);

        if (pce->side_element_is_cpe[i] & 1)
        {
            pce->cpe_channel[pce->side_element_tag_select[i]] = pce->channels;
            pce->num_side_channels += 2;
            pce->channels += 2;
        } else {
            pce->sce_channel[pce->side_element_tag_select[i]] = pce->channels;
            pce->num_side_channels++;
            pce->channels++;
        }
    }

    for (i = 0; i < pce->num_back_channel_elements; i++)
    {
        pce->back_element_is_cpe[i] = faad_get1bit(ld);
        pce->back_element_tag_select[i] = (char)faad_getbits(ld, 4);

        if (pce->back_element_is_cpe[i] & 1)
        {
            pce->cpe_channel[pce->back_element_tag_select[i]] = pce->channels;
            pce->channels += 2;
            pce->num_back_channels += 2;
        } else {
            pce->sce_channel[pce->back_element_tag_select[i]] = pce->channels;
            pce->num_back_channels++;
            pce->channels++;
        }
    }

    for (i = 0; i < pce->num_lfe_channel_elements; i++)
    {
        pce->lfe_element_tag_select[i] = (char)faad_getbits(ld, 4);

        pce->sce_channel[pce->lfe_element_tag_select[i]] = pce->channels;
        pce->num_lfe_channels++;
        pce->channels++;
    }

    for (i = 0; i < pce->num_assoc_data_elements; i++)
        pce->assoc_data_element_tag_select[i] = (char)faad_getbits(ld, 4);

    for (i = 0; i < pce->num_valid_cc_elements; i++)
    {
        pce->cc_element_is_ind_sw[i] = faad_get1bit(ld);
        pce->valid_cc_element_tag_select[i] = (char)faad_getbits(ld, 4);
    }

    faad_byte_align(ld);

    pce->comment_field_bytes = (char)faad_getbits(ld, 8);

    for (i = 0; i < pce->comment_field_bytes; i++)
    {
        pce->comment_field_data[i] = (char)faad_getbits(ld, 8);
    }
    pce->comment_field_data[i] = 0;

    if (pce->channels > MAX_CHAN)
        return 22;

    return 0;
}
#endif

static void decode_sce_lfe(GMAACDecHandle hDecoder, GMAACDecInfo *hInfo, bitfile *ld,  char id_syn_ele)
{
    unsigned char channels = hDecoder->fr_channels;
    unsigned char tag = 0;

    if (channels+1 > MAX_CHAN)
    {
        hInfo->error = 12;
        return;
    }
    if (hDecoder->fr_ch_ele+1 > MAX_SYNTAX_ELEMENTS)
    {
        hInfo->error = 13;
        return;
    }

    /* for SCE hDecoder->element_output_channels[] is not set here because this
       can become 2 when some form of Parametric Stereo coding is used
    */

    /* save the syntax element id */
    hDecoder->element_id[hDecoder->fr_ch_ele] = id_syn_ele;

    /* decode the element */
    hInfo->error = single_lfe_channel_element(hDecoder, ld, channels, &tag);

    /* map output channels position to internal data channels */
    if (hDecoder->element_output_channels[hDecoder->fr_ch_ele] == 2)
    {
        /* this might be faulty when pce_set is true */
        hDecoder->internal_channel[channels] = channels;
        hDecoder->internal_channel[channels+1] = channels+1;
    } else {
        if (hDecoder->pce_set)
            hDecoder->internal_channel[hDecoder->pce.sce_channel[tag]] = channels;
        else
            hDecoder->internal_channel[channels] = channels;
    }

    hDecoder->fr_channels += hDecoder->element_output_channels[hDecoder->fr_ch_ele];
    hDecoder->fr_ch_ele++;
}

static void decode_cpe(GMAACDecHandle hDecoder, GMAACDecInfo *hInfo, bitfile *ld, char id_syn_ele)
{
    unsigned char channels = hDecoder->fr_channels;
    unsigned char tag = 0;

#if 0 //No display error
    if (channels+2 > MAX_CHAN)
    {
        hInfo->error = 12;
        return;
    }
    if (hDecoder->fr_ch_ele+1 > MAX_SYNTAX_ELEMENTS)
    {
        hInfo->error = 13;
        return;
    }
#endif

    /* for CPE the number of output channels is always 2 */
    if (hDecoder->element_output_channels[hDecoder->fr_ch_ele] == 0)
    {
        /* element_output_channels not set yet */
        hDecoder->element_output_channels[hDecoder->fr_ch_ele] = 2;
    }
#if 0 //No display error
	else if (hDecoder->element_output_channels[hDecoder->fr_ch_ele] != 2) {
        /* element inconsistency */
        hInfo->error = 21;
        return;
    }
#endif

    /* save the syntax element id */
    hDecoder->element_id[hDecoder->fr_ch_ele] = id_syn_ele;

    /* decode the element */
    hInfo->error = channel_pair_element(hDecoder, ld, channels, &tag);

    /* map output channel position to internal data channels */
    if (hDecoder->pce_set)
    {
        hDecoder->internal_channel[hDecoder->pce.cpe_channel[tag]] = channels;
        hDecoder->internal_channel[hDecoder->pce.cpe_channel[tag]+1] = channels+1;
    } else {
        hDecoder->internal_channel[channels] = channels;
        hDecoder->internal_channel[channels+1] = channels+1;
    }

    hDecoder->fr_channels += 2;
    hDecoder->fr_ch_ele++;
}

void raw_data_block(GMAACDecHandle hDecoder, GMAACDecInfo *hInfo, bitfile *ld, program_config *pce, drc_info *drc)
{
    char id_syn_ele;

    hDecoder->fr_channels = 0;
    hDecoder->fr_ch_ele = 0;
    hDecoder->first_syn_ele = 25;
    hDecoder->has_lfe = 0;

        /* Table 4.4.3: raw_data_block() */
        while ((id_syn_ele = (char)faad_getbits(ld, LEN_SE_ID)) != ID_END)
        {
            switch (id_syn_ele) {
            case ID_SCE:
                if (hDecoder->first_syn_ele == 25) hDecoder->first_syn_ele = id_syn_ele;
                decode_sce_lfe(hDecoder, hInfo, ld, id_syn_ele);
                if (hInfo->error > 0)
                    return;
                break;
            case ID_CPE:
                if (hDecoder->first_syn_ele == 25) hDecoder->first_syn_ele = id_syn_ele;
                decode_cpe(hDecoder, hInfo, ld, id_syn_ele);
                if (hInfo->error > 0)
                    return;
                break;
            case ID_LFE:
                hDecoder->has_lfe++;
                decode_sce_lfe(hDecoder, hInfo, ld, id_syn_ele);
                if (hInfo->error > 0)
                    return;
                break;
            case ID_CCE: /* not implemented yet, but skip the bits */
#ifdef COUPLING_DEC
                hInfo->error = coupling_channel_element(hDecoder, ld);
#else
                hInfo->error = 6;
#endif
                if (hInfo->error > 0)
                    return;
                break;
            case ID_DSE:
                data_stream_element(hDecoder, ld);
                break;
            case ID_PCE:
                /* 14496-4: 5.6.4.1.2.1.3: */
                /* program_configuration_element()'s in access units shall be ignored */
                program_config_element(pce, ld);
                //if ((hInfo->error = program_config_element(pce, ld)) > 0)
                //    return;
                //hDecoder->pce_set = 1;
                break;
            case ID_FIL:
                /* one sbr_info describes a channel_element not a channel! */
                /* if we encounter SBR data here: error */
                /* SBR data will be read directly in the SCE/LFE/CPE element */
                if ((hInfo->error = fill_element(hDecoder, ld, drc)) > 0)
                    return;
                break;
            }
        }

    /* new in corrigendum 14496-3:2002 */
    {
        faad_byte_align(ld);
    }

    return;
}

element cpe;
volatile char sect_start_end_cb_offset_buf[2*(3*8*15*8 + 2*8*15*8)+2688];

/* Table 4.4.4 and */
/* Table 4.4.9 */
#if 0
static char single_lfe_channel_element(GMAACDecHandle hDecoder, bitfile *ld, char channel, char *tag)
{
    char retval = 0;
    element sce = {0};
    ic_stream *ics = &(sce.ics1);
    short spec_data[1024] = {0};

    sce.element_instance_tag = (char)faad_getbits(ld, LEN_TAG);

    *tag = sce.element_instance_tag;
    sce.channel = channel;
    sce.paired_channel = -1;

    retval = individual_channel_stream(hDecoder, &sce, ld, ics, 0, spec_data);
    if (retval > 0)
        return retval;

    /* noiseless coding is done, spectral reconstruction is done now */
    retval = reconstruct_single_channel(hDecoder, ics, &sce, spec_data);
    if (retval > 0)
        return retval;

    return 0;
}
#else
//Shawn 2004.11.22

    element sce = {0};
    ic_stream *ics1 = &(cpe.ics1);
    ic_stream *ics2 = &(cpe.ics2);
static char single_lfe_channel_element(GMAACDecHandle hDecoder, bitfile *ld, char channel, char *tag)
{
    char retval = 0;
#if 0
    element sce = {0};
    ic_stream *ics1 = &(cpe.ics1);
    ic_stream *ics2 = &(cpe.ics2);
#endif
  #if 0  //for 1024 bytes limited
    short spec_data[1024] = {0};
  #else
    short *spec_data = (short *)&time_out_buf[1024];
    memset(spec_data, 0, 2048);
  #endif  

	ics1->sect_start_end_cb_offset = (char *)&sect_start_end_cb_offset_buf[0];
	ics2->sect_start_end_cb_offset = (char *)&sect_start_end_cb_offset_buf[3*8*15*8 + 2*8*15*8 + 8*15*8];

    sce.element_instance_tag = (char)faad_getbits(ld, LEN_TAG);

    *tag = sce.element_instance_tag;
    sce.channel = channel;
    sce.paired_channel = -1;

    retval = individual_channel_stream(hDecoder, &sce, ld, ics1, 0, spec_data);
    if (retval > 0)
        return retval;

    /* noiseless coding is done, spectral reconstruction is done now */
    retval = reconstruct_single_channel(hDecoder, ics1, &sce, spec_data);
    if (retval > 0)
        return retval;

    return 0;
}
#endif

/* Table 4.4.5 */
#if 0
static char channel_pair_element(GMAACDecHandle hDecoder, bitfile *ld, char channels, char *tag)
{
    short spec_data1[1024] = {0};
    short spec_data2[1024] = {0};
    element cpe = {0};
    ic_stream *ics1 = &(cpe.ics1);
    ic_stream *ics2 = &(cpe.ics2);
    char result;

    cpe.channel        = channels;
    cpe.paired_channel = channels+1;

    cpe.element_instance_tag = (char)faad_getbits(ld, LEN_TAG);
    *tag = cpe.element_instance_tag;

    if ((cpe.common_window = faad_get1bit(ld)) & 1)
    {
        /* both channels have common ics information */
        if ((result = ics_info(hDecoder, ics1, ld, cpe.common_window)) > 0)
            return result;

        ics1->ms_mask_present = (char)faad_getbits(ld, 2);
        if (ics1->ms_mask_present == 1)
        {
            char g, sfb;
            for (g = 0; g < ics1->num_window_groups; g++)
            {
                for (sfb = 0; sfb < ics1->max_sfb; sfb++)
                {
                    ics1->ms_used[g][sfb] = faad_get1bit(ld);
                }
            }
        }

        memcpy(ics2, ics1, sizeof(ic_stream));
    } else {
        ics1->ms_mask_present = 0;
    }

    if ((result = individual_channel_stream(hDecoder, &cpe, ld, ics1,
        0, spec_data1)) > 0)
    {
        return result;
    }

    if ((result = individual_channel_stream(hDecoder, &cpe, ld, ics2,
        0, spec_data2)) > 0)
    {
        return result;
    }

    /* noiseless coding is done, spectral reconstruction is done now */
    if ((result = reconstruct_channel_pair(hDecoder, ics1, ics2, &cpe,
        spec_data1, spec_data2)) > 0)
    {
        return result;
    }

    return 0;
}
#else
//Shawn 2004.10.12


static int channel_pair_element(GMAACDecHandle hDecoder, bitfile *ld, char channels, char *tag)
{
    short *spec_data1;
    short *spec_data2;
    ic_stream *ics1 = &(cpe.ics1);
    ic_stream *ics2 = &(cpe.ics2);
    int result;
	short *spec_data = (short *)&time_out_buf[1024];
	char *ms_used_p;

	ics1->sect_start_end_cb_offset = (char *)&sect_start_end_cb_offset_buf[0];
	ics2->sect_start_end_cb_offset = (char *)&sect_start_end_cb_offset_buf[3*8*15*8 + 2*8*15*8 + 8*15*8];

	spec_data1 = &spec_data[0];
	spec_data2 = &spec_data[1024];

	memset(spec_data1, 0, 4096);

    cpe.channel        = channels;
    cpe.paired_channel = channels+1;

    cpe.element_instance_tag = (char)faad_getbits(ld, LEN_TAG);
    *tag = cpe.element_instance_tag;

    if ((cpe.common_window = faad_get1bit(ld)) & 1)
    {
        /* both channels have common ics information */
        if ((result = ics_info(hDecoder, ics1, ld, cpe.common_window)) > 0)
            return result;

        ics1->ms_mask_present = (char)faad_getbits(ld, 2);
        if (ics1->ms_mask_present == 1)
        {
            int g, sfb;
            for (g = 0; g < ics1->num_window_groups; g++)
            {
				ms_used_p = (char *)&ics1->ms_used[g*MAX_SFB];

                for (sfb = ics1->max_sfb; sfb != 0; sfb --)
                {
                    *ms_used_p++ = faad_get1bit(ld);
                }
            }
        }

        memcpy(ics2, ics1, sizeof(ic_stream));
    }
	else
	{
        ics1->ms_mask_present = 0;
    }

    if ((result = individual_channel_stream(hDecoder, &cpe, ld, ics1,
        0, spec_data1)) > 0)
    {
        return result;
    }

    if ((result = individual_channel_stream(hDecoder, &cpe, ld, ics2,
        0, spec_data2)) > 0)
    {
        return result;
    }

    /* noiseless coding is done, spectral reconstruction is done now */
    if ((result = reconstruct_channel_pair(hDecoder, ics1, ics2, &cpe,
        spec_data1, spec_data2)) > 0)
    {
        return result;
    }

    return 0;
}
#endif

/* Table 4.4.6 */
static char ics_info(GMAACDecHandle hDecoder, ic_stream *ics, bitfile *ld, char common_window)
{
    char retval = 0;

    /* ics->ics_reserved_bit = */ faad_get1bit(ld);
    ics->window_sequence = (char)faad_getbits(ld, 2);
    ics->window_shape = faad_get1bit(ld);

    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
        ics->max_sfb = (char)faad_getbits(ld, 4);
        ics->scale_factor_grouping = (char)faad_getbits(ld, 7);
    } else {
        ics->max_sfb = (char)faad_getbits(ld, 6);
    }

    /* get the grouping information */
    if ((retval = window_grouping_info(hDecoder, ics)) > 0)
        return retval;

    /* should be an error */
    /* check the range of max_sfb */
    if (ics->max_sfb > ics->num_swb)
        return 16;

    if (ics->window_sequence != EIGHT_SHORT_SEQUENCE)
    {
        if ((ics->predictor_data_present = faad_get1bit(ld)) & 1)
        {
            if (hDecoder->object_type == MAIN) /* MPEG2 style AAC predictor */
            {
                unsigned char sfb;

                ics->pred.limit = min(ics->max_sfb, max_pred_sfb(hDecoder->sf_index));

                if ((ics->pred.predictor_reset = faad_get1bit(ld)) & 1)
                {
                    ics->pred.predictor_reset_group_number = (char)faad_getbits(ld, 5);
                }

                for (sfb = 0; sfb < ics->pred.limit; sfb++)
                {
                    ics->pred.prediction_used[sfb] = faad_get1bit(ld);
                }
            }
#ifdef LTP_DEC
            else { /* Long Term Prediction */
                if (hDecoder->object_type < ER_OBJECT_START)
                {
                    if ((ics->ltp.data_present = faad_get1bit(ld)) & 1)
                    {
                        if ((retval = ltp_data(hDecoder, ics, &(ics->ltp), ld)) > 0)
                        {
                            return retval;
                        }
                    }
                    if (common_window)
                    {
                        if ((ics->ltp2.data_present = faad_get1bit(ld)) & 1)
                        {
                            if ((retval = ltp_data(hDecoder, ics, &(ics->ltp2), ld)) > 0)
                            {
                                return retval;
                            }
                        }
                    }
                }
            }
#endif
        }
    }

    return retval;
}

/* Table 4.4.7 */
static char pulse_data(ic_stream *ics, pulse_info *pul, bitfile *ld)
{
    unsigned char i;

    pul->number_pulse = (unsigned char)faad_getbits(ld, 2);
    pul->pulse_start_sfb = (unsigned char)faad_getbits(ld, 6);

    /* check the range of pulse_start_sfb */
    if (pul->pulse_start_sfb > ics->num_swb)
        return 16;

    for (i = 0; i < pul->number_pulse+1; i++)
    {
        pul->pulse_offset[i] = (unsigned char)faad_getbits(ld, 5);
        pul->pulse_amp[i] = (unsigned char)faad_getbits(ld, 4);
    }

    return 0;
}

#if 0
#ifdef COUPLING_DEC
/* Table 4.4.8: Currently just for skipping the bits... */
static char coupling_channel_element(GMAACDecHandle hDecoder, bitfile *ld)
{
    char c, result = 0;
    char ind_sw_cce_flag = 0;
    char num_gain_element_lists = 0;
    char num_coupled_elements = 0;

    element el_empty = {0};
    ic_stream ics_empty = {0};
    short sh_data[1024];

    c = faad_getbits(ld, LEN_TAG);

    ind_sw_cce_flag = faad_get1bit(ld);
    num_coupled_elements = faad_getbits(ld, 3);

    for (c = 0; c < num_coupled_elements + 1; c++)
    {
        char cc_target_is_cpe, cc_target_tag_select;

        num_gain_element_lists++;

        cc_target_is_cpe = faad_get1bit(ld);
        cc_target_tag_select = faad_getbits(ld, 4);

        if (cc_target_is_cpe)
        {
            char cc_l = faad_get1bit(ld);
            char cc_r = faad_get1bit(ld);

            if (cc_l && cc_r)
                num_gain_element_lists++;
        }
    }

    faad_get1bit(ld);
    faad_get1bit(ld);
    faad_getbits(ld, 2);

    if ((result = individual_channel_stream(hDecoder, &el_empty, ld, &ics_empty,
        0, sh_data)) > 0)
    {
        return result;
    }

    for (c = 1; c < num_gain_element_lists; c++)
    {
        char cge;

        if (ind_sw_cce_flag)
        {
            cge = 1;
        } else {
            cge = faad_get1bit(ld);
        }

        if (cge)
        {
            huffman_scale_factor(ld);
        } else {
            char g, sfb;

            for (g = 0; g < ics_empty.num_window_groups; g++)
            {
                for (sfb = 0; sfb < ics_empty.max_sfb; sfb++)
                {
                    if (ics_empty.sfb_cb[g][sfb] != ZERO_HCB)
                        huffman_scale_factor(ld);
                }
            }
        }
    }

    return 0;
}
#endif
#endif

/* Table 4.4.10 */
static unsigned short data_stream_element(GMAACDecHandle hDecoder, bitfile *ld)
{
    char byte_aligned;
    unsigned short i, count;

    /* element_instance_tag = */ faad_getbits(ld, LEN_TAG);
    byte_aligned = faad_get1bit(ld);
    count = (unsigned short)faad_getbits(ld, 8);
    if (count == 255)
    {
        count += (unsigned short)faad_getbits(ld, 8);
    }
    if (byte_aligned)
        faad_byte_align(ld);

    for (i = 0; i < count; i++)
    {
        faad_getbits(ld, LEN_BYTE);
    }

    return count;
}

/* Table 4.4.11 */
static char fill_element(GMAACDecHandle hDecoder, bitfile *ld, drc_info *drc)
{
    unsigned short count;

    count = (unsigned short)faad_getbits(ld, 4);
    if (count == 15)
    {
        count += (unsigned short)faad_getbits(ld, 8) - 1;
    }

    if (count > 0)
    {
            while (count > 0)
            {
                count -= extension_payload(ld, drc, count);
            }
    }

    return 0;
}

/* Table 4.4.24 */
#if 0
static char individual_channel_stream(GMAACDecHandle hDecoder, element *ele, bitfile *ld, ic_stream *ics, char scal_flag, short *spec_data)
{
    char result;

    ics->global_gain = (char)faad_getbits(ld, 8);

    if (!ele->common_window && !scal_flag)
    {
        if ((result = ics_info(hDecoder, ics, ld, ele->common_window)) > 0)
            return result;
    }

#if 0
    if ((result = section_data(hDecoder, ics, ld)) > 0)
        return result;

    if ((result = scale_factor_data(hDecoder, ics, ld)) > 0)
        return result;
#else
//Shawn 2004.10.6 remove profile
	if ((result = section_data(ics, ld)) > 0)
        return result;

	if ((result = scale_factor_data(ics, ld)) > 0)
        return result;
#endif

    if (!scal_flag)
    {
        /**
         **  NOTE: It could be that pulse data is available in scalable AAC too,
         **        as said in Amendment 1, this could be only the case for ER AAC,
         **        though. (have to check this out later)
         **/
        /* get pulse data */
        if ((ics->pulse_data_present = faad_get1bit(ld)) & 1)
        {
            if ((result = pulse_data(ics, &(ics->pul), ld)) > 0)
                return result;
        }

        /* get tns data */
        if ((ics->tns_data_present = faad_get1bit(ld)) & 1)
        {
                tns_data(ics, &(ics->tns), ld);
        }

        /* get gain control data */
        if ((ics->gain_control_data_present = faad_get1bit(ld)) & 1)
        {
            return 1;
        }
    }

        /* decode the spectral data */
        if ((result = spectral_data(hDecoder, ics, ld, spec_data)) > 0)
        {
            return result;
        }

    /* pulse coding reconstruction */
    if (ics->pulse_data_present)
    {
        if (ics->window_sequence != EIGHT_SHORT_SEQUENCE)
        {
            if ((result = pulse_decode(ics, spec_data, hDecoder->frameLength)) > 0)
                return result;
        } else {
            return 2; /* pulse coding not allowed for short blocks */
        }
    }

    return 0;
}
#else
//Shawn 2004.10.12
static char individual_channel_stream(GMAACDecHandle hDecoder, element *ele, bitfile *ld, ic_stream *ics, char scal_flag, short *spec_data)
{
    char result;

    ics->global_gain = (char)faad_getbits(ld, 8);

    if (!ele->common_window && !scal_flag)
    {
        if ((result = ics_info(hDecoder, ics, ld, ele->common_window)) > 0)
            return result;
    }

	if ((result = section_data(ics, ld)) > 0)
        return result;

	if ((result = scale_factor_data(ics, ld)) > 0)
        return result;

    if (!scal_flag)
    {
        /**
         **  NOTE: It could be that pulse data is available in scalable AAC too,
         **        as said in Amendment 1, this could be only the case for ER AAC,
         **        though. (have to check this out later)
         **/
        /* get pulse data */
        if ((ics->pulse_data_present = faad_get1bit(ld)) & 1)
        {
            if ((result = pulse_data(ics, &(ics->pul), ld)) > 0)
                return result;
        }

        /* get tns data */
        if ((ics->tns_data_present = faad_get1bit(ld)) & 1)
        {
                tns_data(ics, &(ics->tns), ld);
        }

        /* get gain control data */
        if ((ics->gain_control_data_present = faad_get1bit(ld)) & 1)
        {
            return 1;
        }
    }

        /* decode the spectral data */
        if ((result = spectral_data(hDecoder, ics, ld, spec_data)) > 0)
        {
            return result;
        }

    /* pulse coding reconstruction */
    if (ics->pulse_data_present)
    {
        if (ics->window_sequence != EIGHT_SHORT_SEQUENCE)
        {
            if ((result = pulse_decode(ics, spec_data, hDecoder->frameLength)) > 0)
                return result;
        }
		else
		{
            return 2; /* pulse coding not allowed for short blocks */
        }
    }

    return 0;
}
#endif

/* Table 4.4.25 */
#if 0
static char section_data(GMAACDecHandle hDecoder, ic_stream *ics, bitfile *ld)
{
    char g;
    char sect_esc_val, sect_bits;

    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
        sect_bits = 3;
    else
        sect_bits = 5;
    sect_esc_val = (1<<sect_bits) - 1;

#if 0
    printf("\ntotal sfb %d\n", ics->max_sfb);
    printf("   sect    top     cb\n");
#endif

    for (g = 0; g < ics->num_window_groups; g++)
    {
        char k = 0;
        char i = 0;

        while (k < ics->max_sfb)
        {
            char sfb;
            char sect_len_incr;
            unsigned short sect_len = 0;
            char sect_cb_bits = 4;

            /* if "faad_getbits" detects error and returns "0", "k" is never
               incremented and we cannot leave the while loop */
            if ((ld->error != 0) || (ld->no_more_reading))
                return 14;


            ics->sect_cb[g][i] = (char)faad_getbits(ld, sect_cb_bits);

            if (ics->sect_cb[g][i] == NOISE_HCB)
                ics->noise_used = 1;

                sect_len_incr = (char)faad_getbits(ld, sect_bits);
            while ((sect_len_incr == sect_esc_val) /* &&
                (k+sect_len < ics->max_sfb)*/)
            {
                sect_len += sect_len_incr;
                sect_len_incr = (char)faad_getbits(ld, sect_bits);
            }

            sect_len += sect_len_incr;

            ics->sect_start[g][i] = k;
            ics->sect_end[g][i] = k + sect_len;

            if (k + sect_len >= 8*15)
                return 15;
            if (i >= 8*15)
                return 15;

            for (sfb = k; sfb < k + sect_len; sfb++)
                ics->sfb_cb[g][sfb] = ics->sect_cb[g][i];

#if 0
            printf(" %6d %6d %6d\n",
                i,
                ics->sect_end[g][i],
                ics->sect_cb[g][i]);
#endif

            k += sect_len;
            i++;
        }
        ics->num_sec[g] = i;
    }

#if 0
    printf("\n");
#endif

    return 0;
}
#else
//Shawn 2004.10.6
static char section_data(ic_stream *ics, bitfile *ld)
{
    int g;
    int sect_esc_val, sect_bits;
	char *sfb_cb_p, *sfb_cb_pp;
	int reg_window_sequence;
	char *sect_start_end_cb_p;

	reg_window_sequence = ics->window_sequence;

    if (reg_window_sequence == EIGHT_SHORT_SEQUENCE)
	{
        sect_bits = 3;
		sect_esc_val = 7;//(1<<3) - 1;
	}
    else
	{
        sect_bits = 5;
		sect_esc_val = 31;//(1<<5) - 1;
	}

	sfb_cb_pp = &ics->sect_start_end_cb_offset[3*8*15*8 + 2*8*15*8];

    for (g = 0; g < ics->num_window_groups; g++)
    {
        int k = 0;
        int i = 0;

		sfb_cb_p = &sfb_cb_pp[g*8*15];//&ics->sfb_cb[g*8*15];
		sect_start_end_cb_p = &ics->sect_start_end_cb_offset[g*15*8];

        while (k < ics->max_sfb)
        {
            int sfb;
            int sect_len_incr;
            int sect_len = 0;
            int sect_cb_bits = 4;
			int reg_sect_cb;

            /* if "faad_getbits" detects error and returns "0", "k" is never
               incremented and we cannot leave the while loop */
            if ((ld->error != 0) || (ld->no_more_reading))
                return 14;

			reg_sect_cb = faad_getbits(ld, sect_cb_bits);

            sect_start_end_cb_p[2*8*15*8] = (char)reg_sect_cb;

            if (reg_sect_cb == NOISE_HCB)
                ics->noise_used = 1;

                sect_len_incr = (int)faad_getbits(ld, sect_bits);
            while ((sect_len_incr == sect_esc_val) /* &&
                (k+sect_len < ics->max_sfb)*/)
            {
                sect_len += sect_len_incr;
                sect_len_incr = (int)faad_getbits(ld, sect_bits);
            }

            sect_len += sect_len_incr;


            sect_start_end_cb_p[1*8*15*8] = k + sect_len;
			*sect_start_end_cb_p++ = k;

            if (k + sect_len >= 8*15 || i >= 8*15)
                return 15;
            /*if (i >= 8*15)
                return 15;*/

            for (sfb = sect_len; sfb != 0; sfb --)
				*sfb_cb_p++ = reg_sect_cb;

            k += sect_len;
            i++;
        }
        ics->num_sec[g] = i;
    }

    return 0;
}
#endif

/*
 *  decode_scale_factors()
 *   decodes the scalefactors from the bitstream
 */
/*
 * All scalefactors (and also the stereo positions and pns energies) are
 * transmitted using Huffman coded DPCM relative to the previous active
 * scalefactor (respectively previous stereo position or previous pns energy,
 * see subclause 4.6.2 and 4.6.3). The first active scalefactor is
 * differentially coded relative to the global gain.
 */
#if 0
static char decode_scale_factors(ic_stream *ics, bitfile *ld)
{
    char g, sfb;
    short t;
    signed char noise_pcm_flag = 1;

    short scale_factor = ics->global_gain;
    short is_position = 0;
    short noise_energy = ics->global_gain - 90;

    for (g = 0; g < ics->num_window_groups; g++)
    {
        for (sfb = 0; sfb < ics->max_sfb; sfb++)
        {
            switch (ics->sfb_cb[g][sfb])
            {
            case ZERO_HCB: /* zero book */
                ics->scale_factors[g][sfb] = 0;
                break;
            case INTENSITY_HCB: /* intensity books */
            case INTENSITY_HCB2:

                /* decode intensity position */
                t = huffman_scale_factor(ld);
                is_position += (t - 60);
                ics->scale_factors[g][sfb] = is_position;

                break;
            case NOISE_HCB: /* noise books */

                /* decode noise energy */
                if (noise_pcm_flag)
                {
                    noise_pcm_flag = 0;
                    t = (short)faad_getbits(ld, 9) - 256;
                } else {
                    t = huffman_scale_factor(ld);
                    t -= 60;
                }
                noise_energy += t;
                ics->scale_factors[g][sfb] = noise_energy;

                break;
            default: /* spectral books */

                /* ics->scale_factors[g][sfb] must be between 0 and 255 */

                ics->scale_factors[g][sfb] = 0;

                /* decode scale factor */
                t = huffman_scale_factor(ld);
                scale_factor += (t - 60);
                if (scale_factor < 0 || scale_factor > 255)
                    return 4;
                ics->scale_factors[g][sfb] = scale_factor;

                break;
            }
        }
    }

    return 0;
}
#else
//Shawn 2004.10.6
static int decode_scale_factors(ic_stream *ics, bitfile *ld)
{
    int g, sfb;
    int t;
    int noise_pcm_flag = 1;
    int scale_factor;
    int is_position = 0;
    int noise_energy;
	int reg_global_gain = ics->global_gain;
	short *scale_factors_p;
	char *sfb_cb_p, *sfb_cb_pointer;
	short *scale_factors_pointer;

	scale_factors_pointer = &ics->scale_factors[0];//(short *)&ics->sect_start_end_cb_offset[3*8*15*8 + 2*8*15*8];//
	sfb_cb_pointer = &ics->sect_start_end_cb_offset[3*8*15*8 + 2*8*15*8];

	scale_factor = reg_global_gain;
	noise_energy = reg_global_gain - 90;

    for (g = 0; g < ics->num_window_groups; g ++)
    {
		scale_factors_p = &scale_factors_pointer[g*51];
		sfb_cb_p = &sfb_cb_pointer[g*15*8];//&ics->sfb_cb[g*15*8];

        for (sfb = ics->max_sfb; sfb != 0; sfb --)
        {
            switch (*sfb_cb_p++)
            {
            case ZERO_HCB: /* zero book */
                *scale_factors_p++ = 0;
                break;
            case INTENSITY_HCB: /* intensity books */
            case INTENSITY_HCB2:
                /* decode intensity position */
                t = huffman_scale_factor(ld);
                is_position += (t - 60);
                *scale_factors_p++ = is_position;
                break;
            case NOISE_HCB: /* noise books */
                /* decode noise energy */
                if (noise_pcm_flag)
                {
                    noise_pcm_flag = 0;
                    t = (int)faad_getbits(ld, 9) - 256;
                }
				else
				{
                    t = huffman_scale_factor(ld);
                    t -= 60;
                }
                noise_energy += t;
                *scale_factors_p++ = noise_energy;
                break;
            default: /* spectral books */
                /* ics->scale_factors[g][sfb] must be between 0 and 255 */

                /* decode scale factor */
                t = huffman_scale_factor(ld);
                scale_factor += (t - 60);
				//if (scale_factor >= 0 || scale_factor <= 255)
                if ((unsigned)(scale_factor) <= 255)
				{
					*scale_factors_p++ = scale_factor;
					break;
				}
				else
				{
					*scale_factors_p++ = 0;
                    return 4;
				}

            }
        }
    }

    return 0;
}
#endif

/* Table 4.4.26 */
#if 0
static char scale_factor_data(GMAACDecHandle hDecoder, ic_stream *ics, bitfile *ld)
{
    char ret = 0;
#ifdef PROFILE
    int64_t count = faad_get_ts();
#endif
        ret = decode_scale_factors(ics, ld);

#ifdef PROFILE
    count = faad_get_ts() - count;
    hDecoder->scalefac_cycles += count;
#endif

    return ret;
}
#else
//Shawn 2004.10.6 remove profile
static char scale_factor_data(ic_stream *ics, bitfile *ld)
{
    int ret;

    ret = decode_scale_factors(ics, ld);

    return ret;
}
#endif

/* Table 4.4.27 */
#if 0
static void tns_data(ic_stream *ics, tns_info *tns, bitfile *ld)
{
    char w, filt, i, start_coef_bits, coef_bits;
    char n_filt_bits = 2;
    char length_bits = 6;
    char order_bits = 5;

    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
        n_filt_bits = 1;
        length_bits = 4;
        order_bits = 3;
    }

    for (w = 0; w < ics->num_windows; w++)
    {
        tns->n_filt[w] = (char)faad_getbits(ld, n_filt_bits);

        if (tns->n_filt[w])
        {
            if ((tns->coef_res[w] = faad_get1bit(ld)) & 1)
            {
                start_coef_bits = 4;
            } else {
                start_coef_bits = 3;
            }
        }

        for (filt = 0; filt < tns->n_filt[w]; filt++)
        {
            tns->length[w][filt] = (char)faad_getbits(ld, length_bits);
            tns->order[w][filt]  = (char)faad_getbits(ld, order_bits);
            if (tns->order[w][filt])
            {
                tns->direction[w][filt] = faad_get1bit(ld);
                tns->coef_compress[w][filt] = faad_get1bit(ld);

                coef_bits = start_coef_bits - tns->coef_compress[w][filt];
                for (i = 0; i < tns->order[w][filt]; i++)
                {
                    tns->coef[w][filt][i] = (char)faad_getbits(ld, coef_bits);
                }
            }
        }
    }
}
#else
//Shawn 2004.10.7
static void tns_data(ic_stream *ics, tns_info *tns, bitfile *ld)
{
    int w, filt, i, start_coef_bits, coef_bits;
    int n_filt_bits;
    int length_bits;
    int order_bits;
	char *coef_p;

    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
        n_filt_bits = 1;
        length_bits = 4;
        order_bits = 3;
    }
	else
	{
		n_filt_bits = 2;
        length_bits = 6;
        order_bits = 5;
	}

    for (w = 0; w < ics->num_windows; w++)
    {
        tns->n_filt[w] = (char)faad_getbits(ld, n_filt_bits);

        if (tns->n_filt[w])
        {
            if ((tns->coef_res[w] = faad_get1bit(ld)) & 1)
            {
                start_coef_bits = 4;
            } else {
                start_coef_bits = 3;
            }
        }

        for (filt = 0; filt < tns->n_filt[w]; filt++)
        {
            tns->length[w][filt] = (char)faad_getbits(ld, length_bits);
            tns->order[w][filt]  = (char)faad_getbits(ld, order_bits);

            if (tns->order[w][filt])
            {
                tns->direction[w][filt] = faad_get1bit(ld);
                tns->coef_compress[w][filt] = faad_get1bit(ld);

                coef_bits = start_coef_bits - tns->coef_compress[w][filt];

				coef_p = tns->coef[w][filt];


                for (i = tns->order[w][filt]; i != 0; i --)
                {
                    *coef_p++ = (char)faad_getbits(ld, coef_bits);
                }
            }
        }
    }
}
#endif

#ifdef LTP_DEC
/* Table 4.4.28 */
static char ltp_data(GMAACDecHandle hDecoder, ic_stream *ics, ltp_info *ltp, bitfile *ld)
{
    char sfb, w;

    ltp->lag = 0;

	ltp->lag = (unsigned short)faad_getbits(ld, 11);


    /* Check length of lag */
    if (ltp->lag > (hDecoder->frameLength << 1))
        return 18;

    ltp->coef = (char)faad_getbits(ld, 3);

    if (ics->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
        for (w = 0; w < ics->num_windows; w++)
        {
            if ((ltp->short_used[w] = faad_get1bit(ld)) & 1)
            {
                ltp->short_lag_present[w] = faad_get1bit(ld);
                if (ltp->short_lag_present[w])
                {
                    ltp->short_lag[w] = (char)faad_getbits(ld, 4);
                }
            }
        }
    } else {
        ltp->last_band = (ics->max_sfb < MAX_LTP_SFB ? ics->max_sfb : MAX_LTP_SFB);

        for (sfb = 0; sfb < ltp->last_band; sfb++)
        {
            ltp->long_used[sfb] = faad_get1bit(ld);
        }
    }

    return 0;
}
#endif

/* Table 4.4.29 */
#if 0
static char spectral_data(GMAACDecHandle hDecoder, ic_stream *ics, bitfile *ld, short *spectral_data)
{
    signed char i;
    char g;
    unsigned short inc, k, p = 0;
    char groups = 0;
    char sect_cb;
    char result;
    unsigned short nshort = hDecoder->frameLength/8;

#ifdef PROFILE
    int64_t count = faad_get_ts();
#endif

    for(g = 0; g < ics->num_window_groups; g++)
    {
        p = groups*nshort;

        for (i = 0; i < ics->num_sec[g]; i++)
        {
            sect_cb = ics->sect_cb[g][i];

            inc = (sect_cb >= FIRST_PAIR_HCB) ? 2 : 4;

            switch (sect_cb)
            {
            case ZERO_HCB:
            case NOISE_HCB:
            case INTENSITY_HCB:
            case INTENSITY_HCB2:
                p += (ics->sect_sfb_offset[g][ics->sect_end[g][i]] -
                    ics->sect_sfb_offset[g][ics->sect_start[g][i]]);
                break;
            default:
                for (k = ics->sect_sfb_offset[g][ics->sect_start[g][i]];
                     k < ics->sect_sfb_offset[g][ics->sect_end[g][i]]; k += inc)
                {
                    if ((result = huffman_spectral_data(sect_cb, ld, &spectral_data[p])) > 0)
                        return result;
                    p += inc;
                }
                break;
            }
        }
        groups += ics->window_group_length[g];
    }

#ifdef PROFILE
    count = faad_get_ts() - count;
    hDecoder->spectral_cycles += count;
#endif

    return 0;
}
#else
//Shawn 2004.10.6
static char spectral_data(GMAACDecHandle hDecoder, ic_stream *ics, bitfile *ld, short *spectral_data)
{
    int i;
    int g;
    int inc, k = 0;
    int groups = 0;
    int sect_cb;
    int result;
    int nshort = hDecoder->frameLength >> 3;

	unsigned char *sect_start_end_cb;
	short *spectral_data_p;
	unsigned short *sect_sfb_offset_p;

	sect_sfb_offset_p = (unsigned short *)&ics->sect_start_end_cb_offset[3*8*15*8+0];

    for(g = 0; g < ics->num_window_groups; g++)
    {
        spectral_data_p = (short *)&spectral_data[groups * nshort];

		sect_start_end_cb = &ics->sect_start_end_cb_offset[g*8*15];

        for (i = ics->num_sec[g]; i != 0 ; i --)
        {
			int reg_sect_end;
			int reg_sect_start;
			int reg0, reg1;

            sect_cb = sect_start_end_cb[2*8*15*8];//cb


            inc = (sect_cb >= FIRST_PAIR_HCB) ? 2 : 4;

			reg_sect_end = sect_start_end_cb[8*15*8];//end
			reg_sect_start = *sect_start_end_cb++;//start

			reg0 = sect_sfb_offset_p[reg_sect_start];
			reg1 = sect_sfb_offset_p[reg_sect_end];

            switch (sect_cb)
            {
            case ZERO_HCB:
            case NOISE_HCB:
            case INTENSITY_HCB:
            case INTENSITY_HCB2:
                spectral_data_p += (reg1 - reg0);
                break;
            default:
                for (k = reg1 - reg0; k > 0; k -= inc)
                {
                    if ((result = huffman_spectral_data(sect_cb, ld, spectral_data_p)) > 0)
                        return result;
                    spectral_data_p += inc;
                }
                break;
            }
        }

        groups += ics->window_group_length[g];
		sect_sfb_offset_p += (8*15);
    }

    return 0;
}
#endif

/* Table 4.4.30 */
static unsigned short extension_payload(bitfile *ld, drc_info *drc, unsigned short count)
{
    unsigned short i, n, dataElementLength;
    char dataElementLengthPart;
    char align = 4, data_element_version, loopCounter;

    char extension_type = (char)faad_getbits(ld, 4);

    switch (extension_type)
    {
    case EXT_DYNAMIC_RANGE:
        drc->present = 1;
        n = dynamic_range_info(ld, drc);
        return n;
    case EXT_FILL_DATA:
        /* fill_nibble = */ faad_getbits(ld, 4); /* must be ?000?*/
        for (i = 0; i < count-1; i++)
        {
            /* fill_byte[i] = */ faad_getbits(ld, 8); /* must be ?0100101?*/
        }
        return count;
    case EXT_DATA_ELEMENT:
        data_element_version = (char)faad_getbits(ld, 4);
        switch (data_element_version)
        {
        case ANC_DATA:
            loopCounter = 0;
            dataElementLength = 0;
            do {
                dataElementLengthPart = (char)faad_getbits(ld, 8);
                dataElementLength += dataElementLengthPart;
                loopCounter++;
            } while (dataElementLengthPart == 255);

            for (i = 0; i < dataElementLength; i++)
            {
                /* data_element_byte[i] = */ faad_getbits(ld, 8);
                return (dataElementLength+loopCounter+1);
            }
        default:
            align = 0;
        }
    case EXT_FIL:
    default:
        faad_getbits(ld, align);
        for (i = 0; i < count-1; i++)
        {
            /* other_bits[i] = */ faad_getbits(ld, 8);
        }
        return count;
    }
}

/* Table 4.4.31 */
static char dynamic_range_info(bitfile *ld, drc_info *drc)
{
    unsigned char i, n = 1;
    unsigned char band_incr;

    drc->num_bands = 1;

    if (faad_get1bit(ld) & 1)
    {
        drc->pce_instance_tag = (unsigned char)faad_getbits(ld, 4);
        /* drc->drc_tag_reserved_bits = */ faad_getbits(ld, 4);
        n++;
    }

    drc->excluded_chns_present = faad_get1bit(ld);
    if (drc->excluded_chns_present == 1)
    {
        n += excluded_channels(ld, drc);
    }

    if (faad_get1bit(ld) & 1)
    {
        band_incr = (unsigned char)faad_getbits(ld, 4);
        /* drc->drc_bands_reserved_bits = */ faad_getbits(ld, 4);
        n++;
        drc->num_bands += band_incr;

        for (i = 0; i < drc->num_bands; i++);
        {
            drc->band_top[i] = (unsigned char)faad_getbits(ld, 8);
            n++;
        }
    }

    if (faad_get1bit(ld) & 1)
    {
        drc->prog_ref_level = (unsigned char)faad_getbits(ld, 7);
        /* drc->prog_ref_level_reserved_bits = */ faad_get1bit(ld);
        n++;
    }

    for (i = 0; i < drc->num_bands; i++)
    {
        drc->dyn_rng_sgn[i] = faad_get1bit(ld);
        drc->dyn_rng_ctl[i] = (unsigned char)faad_getbits(ld, 7);
        n++;
    }

    return n;
}

/* Table 4.4.32 */
static char excluded_channels(bitfile *ld, drc_info *drc)
{
    unsigned char i, n = 0;
    unsigned char num_excl_chan = 7;

    for (i = 0; i < 7; i++)
    {
        drc->exclude_mask[i] = faad_get1bit(ld);
    }
    n++;

    while ((drc->additional_excluded_chns[n-1] = faad_get1bit(ld)) == 1)
    {
        for (i = num_excl_chan; i < num_excl_chan+7; i++)
        {
            drc->exclude_mask[i] = faad_get1bit(ld);
        }
        n++;
        num_excl_chan += 7;
    }

    return n;
}

/* Annex A: Audio Interchange Formats */

/* Table 1.A.2 */
void get_adif_header(adif_header *adif, bitfile *ld)
{
    unsigned char i;

    /* adif_id[0] = */ faad_getbits(ld, 8);
    /* adif_id[1] = */ faad_getbits(ld, 8);
    /* adif_id[2] = */ faad_getbits(ld, 8);
    /* adif_id[3] = */ faad_getbits(ld, 8);
    adif->copyright_id_present = faad_get1bit(ld);
    if(adif->copyright_id_present)
    {
        for (i = 0; i < 72/8; i++)
        {
            adif->copyright_id[i] = (signed char)faad_getbits(ld, 8);
        }
        adif->copyright_id[i] = 0;
    }
    adif->original_copy  = faad_get1bit(ld);
    adif->home = faad_get1bit(ld);
    adif->bitstream_type = faad_get1bit(ld);
    adif->bitrate = faad_getbits(ld, 23);
    adif->num_program_config_elements = (unsigned char)faad_getbits(ld, 4);

    for (i = 0; i < adif->num_program_config_elements + 1; i++)
    {
        if(adif->bitstream_type == 0)
        {
            adif->adif_buffer_fullness = faad_getbits(ld, 20);
        } else {
            adif->adif_buffer_fullness = 0;
        }

        program_config_element(&adif->pce[i], ld);
    }
}

/* Table 1.A.5 */
char adts_frame(adts_header *adts, bitfile *ld)
{
    /* faad_byte_align(ld); */
    if (adts_fixed_header(adts, ld))
        return 5;

    adts_variable_header(adts, ld);

	//adts_error_check(adts, ld);
	if (adts->protection_absent == 0)
    {
        adts->crc_check = (unsigned short)faad_getbits(ld, 16);
    }

    return 0;
}

/* Table 1.A.6 */
#if 0
static char adts_fixed_header(adts_header *adts, bitfile *ld)
{
    unsigned short i;
    char sync_err = 1;

    /* try to recover from sync errors */
    for (i = 0; i < 768; i++)
    {
        adts->syncword = (unsigned short)faad_showbits(ld, 12);
        if (adts->syncword != 0xFFF)
        {
            faad_getbits(ld, 8);
        } else {
            sync_err = 0;
            faad_getbits(ld, 12);
            break;
        }
    }
    if (sync_err)
        return 5;

    adts->id = faad_get1bit(ld);
    adts->layer = (char)faad_getbits(ld, 2);
    adts->protection_absent = faad_get1bit(ld);
    adts->profile = (char)faad_getbits(ld, 2);
    adts->sf_index = (char)faad_getbits(ld, 4);
    adts->private_bit = faad_get1bit(ld);
    adts->channel_configuration = (char)faad_getbits(ld, 3);
    adts->original = faad_get1bit(ld);
    adts->home = faad_get1bit(ld);

    if (adts->old_format == 1)
    {
        /* Removed in corrigendum 14496-3:2002 */
        if (adts->id == 0)
        {
            adts->emphasis = (char)faad_getbits(ld, 2);
        }
    }

    return 0;
}
#else
//Shawn 2004.12.9
static char adts_fixed_header(adts_header *adts, bitfile *ld)
{
    int i;
    int sync_err = 1;

    /* try to recover from sync errors */
    for (i = 768; i != 0; i --)
    {
        adts->syncword = (unsigned short)faad_showbits(ld, 12);

        if (adts->syncword != 0xFFF)
        {
            faad_getbits(ld, 8);
        }
	else
	{
            sync_err = 0;
            faad_getbits(ld, 12);
            break;
        }
    }

    if (sync_err)
        return 5;

    adts->id = faad_get1bit(ld);
    adts->layer = (char)faad_getbits(ld, 2);
    adts->protection_absent = faad_get1bit(ld);
    adts->profile = (char)faad_getbits(ld, 2);
    adts->sf_index = (char)faad_getbits(ld, 4);
    adts->private_bit = faad_get1bit(ld);
    adts->channel_configuration = (char)faad_getbits(ld, 3);
    adts->original = faad_get1bit(ld);
    adts->home = faad_get1bit(ld);

    if (adts->old_format == 1)
    {
        /* Removed in corrigendum 14496-3:2002 */
        if (adts->id == 0)
        {
            adts->emphasis = (char)faad_getbits(ld, 2);
        }
    }

    return 0;
}
#endif

/* Table 1.A.7 */
static void adts_variable_header(adts_header *adts, bitfile *ld)
{
    adts->copyright_identification_bit = faad_get1bit(ld);
    adts->copyright_identification_start = faad_get1bit(ld);
    adts->aac_frame_length = (unsigned short)faad_getbits(ld, 13);
    adts->adts_buffer_fullness = (unsigned short)faad_getbits(ld, 11);
    adts->no_raw_data_blocks_in_frame = (char)faad_getbits(ld, 2);
}

// not used in Faraday MPEG-2/4 AAC Decoder
#if 0
/* Table 1.A.8 */
#if 0
// insert into function
static void adts_error_check(adts_header *adts, bitfile *ld)
{
    if (adts->protection_absent == 0)
    {
        adts->crc_check = (unsigned short)faad_getbits(ld, 16);
    }
}
#endif
#endif
