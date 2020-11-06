#ifndef _COMMON_H_
#define _COMMON_H_

#define ALIGN16_DOWN(x)     ((x >> 4) << 4)
#define ALIGN16_UP(x)       (((x + 15) >> 4) << 4)
#define ALIGN8_DOWN(x)      ((x >> 3) << 3)
#define ALIGN8_UP(x)        (((x + 7) >> 3) << 3)
#define ALIGN4_DOWN(x)      ((x >> 2) << 2)
#define ALIGN4_UP(x)        (((x + 3) >> 2) << 2)
#define ALIGN2_DOWN(x)      ((x >> 1) << 1)
#define ALIGN2_UP(x)        (((x + 1) >> 1) << 1)

/*
Bitmap:
|---type(8)----+---vch(8)----+----engine(9)-----+---minor(7)---|
*/
#define EM_TYPE_OFFSET      24
#define EM_TYPE_BITMAP      0xFF000000
#define EM_TYPE_MAX_VALUE   (EM_TYPE_BITMAP >> EM_TYPE_OFFSET)

#define EM_VCH_OFFSET       16
#define EM_VCH_BITMAP       0x00FF0000
#define EM_VCH_MAX_VALUE    (EM_VCH_BITMAP >> EM_VCH_OFFSET)

#define EM_ENGINE_OFFSET    8
#define EM_ENGINE_BITMAP    0x0000FF00
#define EM_ENGINE_MAX_VALUE (EM_ENGINE_BITMAP >> EM_ENGINE_OFFSET)

#define EM_MINOR_OFFSET     0
#define EM_MINOR_BITMAP     0x000000FF
#define EM_MINOR_MAX_VALUE  (EM_MINOR_BITMAP >> EM_MINOR_OFFSET)

#define ENTITY_FD(type, engine, minor) \
    (((type) << EM_TYPE_OFFSET) | ((engine) << EM_ENGINE_OFFSET) | (minor))

#define ENTITY_FD_TYPE(fd)      ((fd & EM_TYPE_BITMAP) >> EM_TYPE_OFFSET)
#define ENTITY_FD_ENGINE(fd)    ((fd & EM_ENGINE_BITMAP) >> EM_ENGINE_OFFSET)
#define ENTITY_FD_MINOR(fd)     ((fd & EM_MINOR_BITMAP) >> EM_MINOR_OFFSET)
#define ENTITY_FD_VCHNUM(fd)    ((fd & EM_VCH_BITMAP) >> EM_VCH_OFFSET)
#define FD_WITHOUT_VCH(fd)      ENTITY_FD(ENTITY_FD_TYPE(fd),ENTITY_FD_ENGINE(fd),ENTITY_FD_MINOR(fd))

#define MAX_WELL_KNOWN_ID_NUM   100
#define MAX_PROPERTYS           368                 //32CH
#define MAX_PROCESS_PROPERTYS   10                  //max of preprocess/postprocess numbers
#define EM_MAX_BUF_COUNT        2

struct video_property_t {
    unsigned int    ch:8;
    unsigned int    id:24;
    unsigned int    value;
};


enum mm_type_t {
    TYPE_VAR_MEM = 0x2001,
    TYPE_FIX_MEM,
};

enum buf_type_t {
    TYPE_YUV420_PACKED = 0x101,
    TYPE_YUV420_FRAME = 0x102,
    TYPE_YUV422_FIELDS = 0x103,         //YUV generated only from TOP field
    TYPE_YUV422_FRAME = 0x104,          //YUV generated from odd and even interlaced
    TYPE_YUV422_RATIO_FRAME = 0x105,    //YUV for TYPE_YUV422_FRAME + (TYPE_YUV422_FRAME / ratio)
    TYPE_YUV422_2FRAMES = 0x106,        //YUV for TYPE_YUV422_FRAME x2
    TYPE_YUV400_FRAME = 0x107,          // frame: monochrome
    TYPE_YUV422_2FRAMES_2FRAMES = 0x108,  //YUV for TYPE_YUV422_FRAME x2 + TYPE_YUV422_FRAME x 2
    TYPE_YUV422_FRAME_2FRAMES = 0x109,    //YUV for TYPE_YUV422_FRAME + TYPE_YUV422_FRAME x 2
    TYPE_YUV422_FRAME_FRAME = 0x10A,    //YUV for TYPE_YUV422_FRAME + TYPE_YUV422_FRAME

    TYPE_YUV422_H264_2D_FRAME = 0x201,
    TYPE_YUV422_MPEG4_2D_FRAME,

    TYPE_DECODE_BITSTREAM = 0x300, //H264, MPEG, JPEG bitstream
    TYPE_BITSTREAM_H264 = 0x301,
    TYPE_BITSTREAM_MPEG4 = 0x302,
    TYPE_BITSTREAM_JPEG = 0x303,

    TYPE_AUDIO_BITSTREAM = 0x401,

    TYPE_RGB565 = 0x501,
    TYPE_YUV422_CASCADE_SCALING_FRAME = 0x502,
    TYPE_RGB555 = 0x503,
};

enum mm_vrc_mode_t {
    EM_VRC_CBR = 1,
    EM_VRC_VBR,
    EM_VRC_ECBR,
    EM_VRC_EVBR
};

enum slice_type_t {
    EM_SLICE_TYPE_P = 0,
    EM_SLICE_TYPE_I = 2,
    EM_SLICE_TYPE_B = 1,
    EM_SLICE_TYPE_AUDIO = 5,
    EM_SLICE_TYPE_RAW = 6,
};

/* the bitmap definition for audio capability */
#define EM_AU_CAPABILITY_SAMPLE_RATE_8K          (1 << 0)    /*  8000 Hz */
#define EM_AU_CAPABILITY_SAMPLE_RATE_11K         (1 << 1)    /* 11025 Hz */
#define EM_AU_CAPABILITY_SAMPLE_RATE_16K         (1 << 2)    /* 16000 Hz */
#define EM_AU_CAPABILITY_SAMPLE_RATE_22K         (1 << 3)    /* 22050 Hz */
#define EM_AU_CAPABILITY_SAMPLE_RATE_32K         (1 << 4)    /* 32000 Hz */
#define EM_AU_CAPABILITY_SAMPLE_RATE_44K         (1 << 5)    /* 44100 Hz */
#define EM_AU_CAPABILITY_SAMPLE_RATE_48K         (1 << 6)    /* 48000 Hz */

#define EM_AU_CAPABILITY_SAMPLE_SIZE_8BIT        (1 << 0)
#define EM_AU_CAPABILITY_SAMPLE_SIZE_16BIT       (1 << 1)

#define EM_AU_CAPABILITY_CHANNELS_1CH            (1 << 0)    /* mono   */
#define EM_AU_CAPABILITY_CHANNELS_2CH            (1 << 1)    /* stereo */

enum audio_channel_type_t {
    EM_AUDIO_MONO = 1,
    EM_AUDIO_STEREO = 2,
};

enum audio_encode_type_t {
    EM_AUDIO_NONE = 0,
    EM_AUDIO_PCM = 1,
    EM_AUDIO_AAC = 2,
    EM_AUDIO_ADPCM = 3,
    EM_AUDIO_G711_ALAW = 4,
    EM_AUDIO_G711_ULAW = 5
};

enum notify_type {
    VG_NO_NEW_JOB = 0,
    VG_HW_DELAYED,
    VG_FRAME_DROP,
    VG_SIGNAL_LOSS,
    VG_SIGNAL_PRESENT,
    VG_FRAMERATE_CHANGE,
    VG_HW_CONFIG_CHANGE,
    VG_TAMPER_ALARM,
    VG_TAMPER_ALARM_RELEASE,
    VG_AUDIO_BUFFER_UNDERRUN,
    VG_AUDIO_BUFFER_OVERRUN,
    VG_NOTIFY_TYPE_COUNT
};

enum checksum_type {
    EM_CHECKSUM_NONE = 0x0,
    EM_CHECKSUM_ALL_CRC = 0x101,
    EM_CHECKSUM_ALL_SUM = 0x0102,
    EM_CHECKSUM_ALL_4_BYTE = 0x103,
    EM_CHECKSUM_ONLY_I_CRC = 0x201,
    EM_CHECKSUM_ONLY_I_SUM = 0x0202,
    EM_CHECKSUM_ONLY_I_4_BYTE = 0x203
};

struct video_buf_t {
    unsigned int addr_va;
    unsigned int addr_pa;
    unsigned int addr_prop; //used to record buffer property
    int need_realloc_val;
    int size;
    int width;
    int height;
    int behavior_bitmap;
    int vari_win_size;      //For VG internal use only
};

struct video_bufinfo_t {
    int                 mtype;  //memory type
    int                 btype;  //buffer type
    int                 count;  //buffer count provide by EM(get_binfo)-->GS-->EM(putjob)    
    struct video_buf_t buf[EM_MAX_BUF_COUNT];
    int fd;             //for reallocate fd
};


//buf_idx=> 0:buf[0], 1:buf[1]
//btype=>TYPE_YUV422_FRAME/TYPE_YUV420_FRAME
//mtype=>TYPE_FIX_MEM
//width=> type integer
//height=> type integer
struct video_buf_info_items_t {
    int buf_idx;
    int btype;
    int mtype;
    int width;
    int height;
};

enum entity_type_t {
    /*template*/
    TYPE_TEMPLATE = 0x0,
    /*capture*/
    TYPE_CAPTURE = 0x10,
    /*decode*/
    TYPE_DECODER = 0x20, //only have one decode entity for H264/MPEG4/JPEG
    /*encode*/
    TYPE_ENCODER = 0x30,
    TYPE_H264_ENC = 0x31,
    TYPE_MPEG4_ENC = 0x32,
    TYPE_JPEG_ENC = 0x33,
    TYPE_ROTATION = 0x34,
    /*scaler*/
    TYPE_SCALER = 0x40,
    /*3di*/
    TYPE_3DI = 0x50,
    TYPE_3DNR = 0x51,
    /*display*/
    TYPE_DISPLAY0 = 0x60,
    TYPE_DISPLAY1 = 0x61,
    TYPE_DISPLAY2 = 0x62,    
    /*datain/dataout*/
    TYPE_DATAIN = 0x70,
    TYPE_DATAOUT = 0x71,
    /* audio */
    TYPE_AUDIO_DEC = 0x80,
    TYPE_AUDIO_ENC = 0x81,
    TYPE_AU_RENDER = 0x80,  // the alias of TYPE_AUDIO_DEC
    TYPE_AU_GRAB = 0x81,    // the alias of TYPE_AUDIO_ENC

};


/* need sync with em/common.h  & gm_lib/src/pif.h  & ioctl/vplib.h  */
#define EM_PROPERTY_NULL_BYTE   0xFE
#define EM_PROPERTY_NULL_VALUE ((EM_PROPERTY_NULL_BYTE << 24) | (EM_PROPERTY_NULL_BYTE << 16) | \
                            (EM_PROPERTY_NULL_BYTE << 8) | (EM_PROPERTY_NULL_BYTE))
#endif

