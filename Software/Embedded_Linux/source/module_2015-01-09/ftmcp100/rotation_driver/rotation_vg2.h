#ifndef _ROTATION_VG2_H_
#define _ROTATION_VG2_H_

#include "video_entity.h"


#define RT_MAX_RECORD 10
struct rt_property_record_t {
    struct video_property_t property[RT_MAX_RECORD];
    struct video_entity_t   *entity;
    int    job_id;
};

struct rt_data_t {
    //int engine;
    int chn;
    unsigned int in_addr_pa;
    unsigned int in_size;
    unsigned int out_addr_pa;
    unsigned int out_size;
    unsigned int src_fmt;
    unsigned int src_dim;
    unsigned int clockwise;
    unsigned int width;
    unsigned int height;
    unsigned int output_width;
    unsigned int output_height;
    int stop_job;
    void *data;
};

#if 0 //allocation from videograph
#include "ms/ms_core.h"
#define driver_alloc(x) ms_small_fast_alloc(x)
#define driver_free(x) ms_small_fast_free(x)
#else
//#define driver_alloc(x) kmalloc(x,GFP_ATOMIC)
//#define driver_free(x) kfree(x)
#endif

#endif

