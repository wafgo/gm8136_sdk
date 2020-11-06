#ifndef _DH9901_DEV_H_
#define _DH9901_DEV_H_

struct dh9901_dev_t {
    int                     index;                          ///< device index
    char                    name[16];                       ///< device name
    struct semaphore        lock;                           ///< device locker
    struct miscdevice       miscdev;                        ///< device node for ioctl access

    int                     pre_plugin[DH9901_DEV_CH_MAX];  ///< device channel previous cable plugin status
    int                     cur_plugin[DH9901_DEV_CH_MAX];  ///< device channel current  cable plugin status

    struct dh9901_ptz_t     ptz[DH9901_DEV_CH_MAX];         ///< device channel PTZ config

    int                     (*vfmt_notify)(int id, int ch, struct dh9901_video_fmt_t *vfmt);
    int                     (*vlos_notify)(int id, int ch, int vlos);
};

struct dh9901_params_t {
    int dh9901_dev_num;
    ushort* dh9901_iaddr;
    int* dh9901_vout_format;
    int dh9901_sample_rate;
    struct dh9901_dev_t* dh9901_dev;
    int init;
    struct dh9901_video_fmt_t* dh9901_tw2964_video_fmt_info;
    u32* DH9901_TW2964_VCH_MAP;
};
typedef struct dh9901_params_t* dh9901_params_p;

int  dh9901_proc_init(int);
void dh9901_proc_remove(int);
int  dh9901_miscdev_init(int);
int  dh9901_device_register(void);
void dh9901_device_deregister(void);
int  dh9901_sound_switch(int, int, bool);
void dh9901_set_params(dh9901_params_p);

#endif  /* _DH9901_DEV_H_ */
