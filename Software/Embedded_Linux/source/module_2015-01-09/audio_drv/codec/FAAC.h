/******************************************************************************/
/*                                                                            */
/*      FILE : FAAC.h                                                         */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
typedef struct GMAACDecInfo
{
    unsigned long bytesconsumed;
    unsigned long samples;
    unsigned char channels;
    unsigned char error;
    unsigned long samplerate;

    /* SBR: 0: off, 1: on; upsample, 2: on; downsampled, 3: off; upsampled */
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
    unsigned char channel_position[64];
	float song_length;
} GMAACDecInfo;

/******************************************************************************/
/*                                                                            */
/*      End of FILE : FAAC.h                                                  */
/*                                                                            */
/******************************************************************************/


