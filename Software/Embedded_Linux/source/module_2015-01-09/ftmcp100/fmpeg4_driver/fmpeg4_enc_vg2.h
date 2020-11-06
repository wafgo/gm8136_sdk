#ifndef _FMPEG4_ENC_VG2_H_
#define _FMPEG4_ENC_VG2_H_

#include "video_entity.h"

//#define MP4E_MODULE_NAME    "ME"
//#define MP4E_MAX_CHANNEL    70


#define MP4E_MAX_RECORD 100
struct mp4e_property_record_t {
    struct video_property_t property[MP4E_MAX_RECORD];
    struct video_entity_t   *entity;
    int    job_id;
};

#endif

