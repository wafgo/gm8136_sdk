#ifndef __STRUCTS_H__
#define __STRUCTS_H__

#include "cfft.h"

#define MAX_CHAN        64
#define MAX_SYNTAX_ELEMENTS 48
#define MAX_WINDOW_GROUPS    8
#define MAX_SFB             51
#define MAX_LTP_SFB         40
#define MAX_LTP_SFB_S        8

/* used to save the prediction state */
typedef struct {
    short r[2];
    short COR[2];
    short VAR[2];
} pred_state;

typedef struct {
    unsigned short N;
    cfft_info *cfft;
    complex_t *sincos;
#ifdef PROFILE
    int64_t cycles;
    int64_t fft_cycles;
#endif
} mdct_info;

typedef struct
{
    const real_t *long_window[2];
    const real_t *short_window[2];

    mdct_info *mdct256;
    mdct_info *mdct2048;
#ifdef PROFILE
    int64_t cycles;
#endif
} fb_info;

typedef struct
{
    unsigned char present;

    unsigned char num_bands;
    unsigned char pce_instance_tag;
    unsigned char excluded_chns_present;
    unsigned char band_top[17];
    unsigned char prog_ref_level;
    unsigned char dyn_rng_sgn[17];
    unsigned char dyn_rng_ctl[17];
    unsigned char exclude_mask[MAX_CHAN];
    unsigned char additional_excluded_chns[MAX_CHAN];

    real_t ctrl1;
    real_t ctrl2;
} drc_info;

typedef struct
{
    unsigned char element_instance_tag;
    unsigned char object_type;
    unsigned char sf_index;
    unsigned char num_front_channel_elements;
    unsigned char num_side_channel_elements;
    unsigned char num_back_channel_elements;
    unsigned char num_lfe_channel_elements;
    unsigned char num_assoc_data_elements;
    unsigned char num_valid_cc_elements;
    unsigned char mono_mixdown_present;
    unsigned char mono_mixdown_element_number;
    unsigned char stereo_mixdown_present;
    unsigned char stereo_mixdown_element_number;
    unsigned char matrix_mixdown_idx_present;
    unsigned char pseudo_surround_enable;
    unsigned char matrix_mixdown_idx;
    unsigned char front_element_is_cpe[16];
    unsigned char front_element_tag_select[16];
    unsigned char side_element_is_cpe[16];
    unsigned char side_element_tag_select[16];
    unsigned char back_element_is_cpe[16];
    unsigned char back_element_tag_select[16];
    unsigned char lfe_element_tag_select[16];
    unsigned char assoc_data_element_tag_select[16];
    unsigned char cc_element_is_ind_sw[16];
    unsigned char valid_cc_element_tag_select[16];

    unsigned char channels;

    unsigned char comment_field_bytes;
    unsigned char comment_field_data[257];

    /* extra added values */
    unsigned char num_front_channels;
    unsigned char num_side_channels;
    unsigned char num_back_channels;
    unsigned char num_lfe_channels;
    unsigned char sce_channel[16];
    unsigned char cpe_channel[16];
} program_config;

typedef struct
{
    unsigned short syncword;
    unsigned char id;
    unsigned char layer;
    unsigned char protection_absent;
    unsigned char profile;
    unsigned char sf_index;
    unsigned char private_bit;
    unsigned char channel_configuration;
    unsigned char original;
    unsigned char home;
    unsigned char emphasis;
    unsigned char copyright_identification_bit;
    unsigned char copyright_identification_start;
    unsigned short aac_frame_length;
    unsigned short adts_buffer_fullness;
    unsigned char no_raw_data_blocks_in_frame;
    unsigned short crc_check;

    /* control param */
    unsigned char old_format;
} adts_header;

typedef struct
{
    unsigned char copyright_id_present;
    signed char copyright_id[10];
    unsigned char original_copy;
    unsigned char home;
    unsigned char bitstream_type;
    unsigned int bitrate;
    unsigned char num_program_config_elements;
    unsigned int adif_buffer_fullness;

    /* maximum of 16 PCEs */
    program_config pce[16];
} adif_header;

typedef struct
{
    unsigned char last_band;
    unsigned char data_present;
    unsigned short lag;
    unsigned char lag_update;
    unsigned char coef;
    unsigned char long_used[MAX_SFB];
    unsigned char short_used[8];
    unsigned char short_lag_present[8];
    unsigned char short_lag[8];
} ltp_info;//60 bytes

typedef struct
{
    unsigned char limit;
    unsigned char predictor_reset;
    unsigned char predictor_reset_group_number;
    unsigned char prediction_used[MAX_SFB];
} pred_info;

typedef struct
{
    unsigned char number_pulse;
    unsigned char pulse_start_sfb;
    unsigned char pulse_offset[4];
    unsigned char pulse_amp[4];
} pulse_info;//10 bytes

typedef struct
{
    unsigned char n_filt[8];
    unsigned char coef_res[8];
    unsigned char length[8][4];
    unsigned char order[8][4];
    unsigned char direction[8][4];
    unsigned char coef_compress[8][4];
    unsigned char coef[8][4][32];
} tns_info;//1168 bytes

#if 0
typedef struct
{
    char max_sfb;

    char num_swb;
    char num_window_groups;
    char num_windows;
    char window_sequence;
    char window_group_length[8];
    char window_shape;
    char scale_factor_grouping;
    unsigned short sect_sfb_offset[8][15*8];
    unsigned short swb_offset[52];

    char sect_cb[8][15*8];
    unsigned short sect_start[8][15*8];
    unsigned short sect_end[8][15*8];
    char sfb_cb[8][8*15];
    char num_sec[8]; /* number of sections in a group */

    char global_gain;
    short scale_factors[8][51]; /* [0..255] */

    char ms_mask_present;
    char ms_used[MAX_WINDOW_GROUPS][MAX_SFB];

    char noise_used;

    char pulse_data_present;
    char tns_data_present;
    char gain_control_data_present;
    char predictor_data_present;

    pulse_info pul;
    tns_info tns;
    pred_info pred;
    ltp_info ltp;
    ltp_info ltp2;
} ic_stream; /* individual channel stream */
#else
//Shawn 2004.10.11
extern volatile char sect_start_end_cb_offset_buf[2*(3*8*15*8 + 2*8*15*8)+1920+768];

typedef struct
{
    unsigned char max_sfb;

    unsigned char num_swb;
    unsigned char num_window_groups;
    unsigned char num_windows;
    unsigned char window_sequence;
    unsigned char window_group_length[8];
    unsigned char window_shape;
    unsigned char scale_factor_grouping;   
    unsigned short swb_offset[52];

	//unsigned short sect_sfb_offset[8*15*8];
    //char sect_cb[8*15*8];
    //char sect_start[8*15*8];//use 8-bits instead of 16-bits notice must modify also in spectral_data()
    //char sect_end[8*15*8];//use 8-bits instead of 16-bits notice must modify also in spectral_data()
	//char sfb_cb[8*8*15];
	unsigned char *sect_start_end_cb_offset;//[3*8*15*8 + 2*8*15*8];//use 8-bits instead of 16-bits notice must modify also in spectral_data()
    
	
	short scale_factors[8*51]; /* [0..255] */

    unsigned char num_sec[8]; /* number of sections in a group */

    unsigned char global_gain;
    

    unsigned char ms_mask_present;
    unsigned char ms_used[MAX_WINDOW_GROUPS*MAX_SFB];

    unsigned char noise_used;

    unsigned char pulse_data_present;
    unsigned char tns_data_present;
    unsigned char gain_control_data_present;
    unsigned char predictor_data_present;

    pulse_info pul;
    tns_info tns;
    pred_info pred;
    ltp_info ltp;
    ltp_info ltp2;
} ic_stream; /* individual channel stream */
#endif

typedef struct
{
    //char ele_id;

    unsigned char channel;
    short paired_channel;

    unsigned char element_instance_tag;
    unsigned char common_window;

    ic_stream ics1;
    ic_stream ics2;
} element; /* syntax element (SCE, CPE, LFE) */

typedef struct mp4AudioSpecificConfig
{
    /* Audio Specific Info */
    char objectTypeIndex;
    char samplingFrequencyIndex;
    unsigned int samplingFrequency;
    char channelsConfiguration;

    /* GA Specific Info */
    char frameLengthFlag;
    char dependsOnCoreCoder;
    unsigned short coreCoderDelay;
    char extensionFlag;
    char aacSectionDataResilienceFlag;
    char aacScalefactorDataResilienceFlag;
    char aacSpectralDataResilienceFlag;
    char epConfig;

    signed char sbr_present_flag;
    signed char forceUpSampling;
} mp4AudioSpecificConfig;

typedef struct faacDecConfiguration
{
    char defObjectType;
    unsigned int defSampleRate;
    char outputFormat;
    char downMatrix;
    char useOldADTSFormat;
    char dontUpSampleImplicitSBR;
} faacDecConfiguration, *faacDecConfigurationPtr;

typedef struct GMAACDecInfo
{
    unsigned int bytesconsumed;
    unsigned int samples;
    unsigned char channels;
    unsigned char error;
    unsigned int samplerate;

    /* SBR: 0: off, 1: on; normal, 2: on; downsampled */
    unsigned char sbr;

    /* MPEG-4 ObjectType */
    unsigned char object_type;

    /* AAC header type; MP4 will be signalled as RAW also */
    unsigned char header_type;

    /* multichannel configuration */
    unsigned char num_front_channels;
    unsigned char num_side_channels;
    unsigned char num_back_channels;
    unsigned char num_lfe_channels;
    unsigned char channel_position[MAX_CHAN];
} GMAACDecInfo;

typedef struct
{
    unsigned char adts_header_present;
    unsigned char adif_header_present;
    unsigned char sf_index;
    unsigned char object_type;
    unsigned char channelConfiguration;
    unsigned short frameLength;
    unsigned char postSeekResetFlag;

    unsigned int frame;

    unsigned char downMatrix;
    unsigned char first_syn_ele;
    unsigned char has_lfe;
    /* number of channels in current frame */
    unsigned char fr_channels;
    /* number of elements in current frame */
    unsigned char fr_ch_ele;

    /* element_output_channels:
       determines the number of channels the element will output
    */
    unsigned char element_output_channels[MAX_SYNTAX_ELEMENTS];
    /* element_alloced:
       determines whether the data needed for the element is allocated or not
    */
    unsigned char element_alloced[MAX_SYNTAX_ELEMENTS];
    /* alloced_channels:
       determines the number of channels where output data is allocated for
    */
    unsigned char alloced_channels;

    /* output data buffer */
    void *sample_buffer;

    unsigned char window_shape_prev[MAX_CHAN];
#ifdef LTP_DEC
    unsigned short ltp_lag[MAX_CHAN];
#endif
    fb_info *fb;
#ifdef DRC_DEC
    drc_info *drc;
#endif

    real_t *time_out[MAX_CHAN];
    real_t *fb_intermed[MAX_CHAN];


#ifdef LTP_DEC
    short *lt_pred_stat[MAX_CHAN];
#endif

    /* Program Config Element */
    unsigned char pce_set;
    program_config pce;
    unsigned char element_id[MAX_CHAN];
    unsigned char internal_channel[MAX_CHAN];

    /* Configuration data */
    faacDecConfiguration config;

#ifdef PROFILE
    int64_t cycles;
    int64_t spectral_cycles;
    int64_t output_cycles;
    int64_t scalefac_cycles;
    int64_t requant_cycles;
#endif
} faacDecStruct, *GMAACDecHandle;


#endif
