#ifndef _JOB_H_
#define _JOB_H_

/* video job status */
#define JOB_STATUS_FAIL     -1
#define JOB_STATUS_ONGOING  0
#define JOB_STATUS_FINISH   1

/* job */
struct video_entity_job_t {
    unsigned int            id;        //provide by EM User
    int                     fd;        //provide by EM User
    void                    *entity;   //point to Video Entity
    unsigned int            ver;       //provide by EM User,version of property
    int                     priority;  //providei by EM user, job priority,  0(normal) 1(high) 
    int                     front_dependency;    //depend with front job

    struct video_bufinfo_t  in_buf;    //provide by EM User
    struct video_bufinfo_t  out_buf;   //provide by EM User
    struct video_property_t in_prop[MAX_PROPERTYS];     //Provide by EM, use by Video Entity
    struct video_property_t out_prop[MAX_PROPERTYS];    //Fill by Video Entity
    int                     status;                     //Fill by Video Entity

    int                     (*callback)(void *);    //provide by EM User, call by video entity
    int                     (*src_callback)(void *);//provide by EM User, call by EM

    struct video_entity_job_t *next_job;    //provide by EM User

    void                    *em_user_priv;  //Use by EM user to record
    void                    *entity_priv;   //Use by Video Entity to record

    unsigned long long      root_time;      //indicate root entity callback time
    struct list_head        callback_list;  //internal used by EM
    int                     from_fd;        //provide by EM User
#define FLAGS_PUTJOB    (1 << 0)    //indicate this job is put to entity already
#define FLAGS_ROOTJOB   (1 << 1)    //indicate first job (multi-job head)
#define FLAGS_USER_DONE (1 << 2)    //indicate user putjob done
    unsigned int            internal_flags; //internal flags to indicate state machine
    unsigned int            callback_time;  //internal time to time from driver to GS
    unsigned int            flag;      //bit 0: repeat
    int                     group_id; // 0 for not group job
    int                     reserved2[1];
};

#endif
