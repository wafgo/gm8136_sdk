#ifndef _FMPEG4_DEC_VG2_H_
#define _FMPEG4_DEC_VG2_H_

#include "video_entity.h"


/*
#define MAX_NAME    50
#define MAX_README  100
struct property_map_t {
    int id;
    char name[MAX_NAME];
    char readme[MAX_README];
};
*/
struct mp4d_property_record_t {
#define MP4D_MAX_RECORD MAX_PROPERTYS
    struct video_property_t property[MP4D_MAX_RECORD];
    struct video_entity_t   *entity;
    int    job_id;
};

#endif

