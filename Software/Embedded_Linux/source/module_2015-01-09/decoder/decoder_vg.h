#ifndef _DECODER_VG_H_
#define _DECODER_VG_H_

#include "video_entity.h"

/*
* 1.0.1: init. version
* 1.0.2: add module parameter to output error bitstream
*/
#define DECODER_VER             0x10000200
#define DECODER_VER_MAJOR       ((DECODER_VER>>28)&0x0F)  // huge change
#define DECODER_VER_MINOR       ((DECODER_VER>>20)&0xFF)  // interface change 
#define DECODER_VER_MINOR2      ((DECODER_VER>>8)&0x0FFF) // functional modified or buf fixed
#define DECODER_VER_BRANCH      (DECODER_VER&0xFF)        // branch for customer request

#define MAX_NAME    50
#define MAX_README  100

#define ENABLE_POC_SEQ_PROP // define this to use POC/SEQ property

#define MAX_DECODE_ENGINE_NUM	1
#define MAX_DECODE_CHN_NUM      64

#define LOG_DEC_ERROR       0
#define LOG_DEC_WARNING     1
#define LOG_DEC_INFO        2

//#define PARSING_RESOLUTION

typedef enum {

    TYPE_JPEG = 0,
    TYPE_MPEG4 = 1,
    TYPE_H264 = 2,
    DEC_TYPE_NUM
} DECODER_TYPE;

#define DECODER_MAX_MAP_ID  18
struct dec_property_map_t {
    int id;
    char name[MAX_NAME];
    char readme[MAX_README];
};


enum property_id {
    ID_YUV_WIDTH_THRESHOLD=(MAX_WELL_KNOWN_ID_NUM+1), //start from 100
    ID_SUB_YUV_RATIO,  
#ifdef ENABLE_POC_SEQ_PROP
    ID_POC,                       // ID of POC value
    ID_SEQ,                       // ID of sequence value
#endif // ENABLE_POC_SEQ_PROP

    ID_COUNT,
};

struct decoder_entity_t {
    DECODER_TYPE decoder_type;
#ifdef PARSING_RESOLUTION
    int (*put_job)(void *parm, int width, int height);
#else
    int (*put_job)(void *parm);
#endif
    int (*stop_job)(void *parm, int engine, int minor);
    int (*get_property)(void *parm, int engine, int minor, char *string);
};

int decoder_register(struct decoder_entity_t *entity, DECODER_TYPE dec_type);
int decoder_deregister(DECODER_TYPE dec_type);


#endif

