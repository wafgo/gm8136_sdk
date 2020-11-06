#ifndef _TP2802_DEV_H_
#define _TP2802_DEV_H_

struct tp2802_dev_t {
    int                     index;                          ///< device index
    char                    name[16];                       ///< device name
    struct semaphore        lock;                           ///< device locker
    struct miscdevice       miscdev;                        ///< device node for ioctl access

    int                     pre_plugin[TP2802_DEV_CH_MAX];  ///< device channel previous cable plugin status
    int                     cur_plugin[TP2802_DEV_CH_MAX];  ///< device channel current  cable plugin status

    int                     (*vfmt_notify)(int id, int ch, struct tp2802_video_fmt_t *vfmt);
    int                     (*vlos_notify)(int id, int ch, int vlos);
};

struct tp2802_params_t {
    int tp2802_dev_num;
    ushort* tp2802_iaddr;
    int* tp2802_vout_format;
    int tp2802_sample_rate;
    struct tp2802_dev_t* tp2802_dev;
    int init;
    u32* TP2802_TW2964_VCH_MAP;
};
typedef struct tp2802_params_t* tp2802_params_p;

int  tp2802_proc_init(int);
void tp2802_proc_remove(int);
int  tp2802_miscdev_init(int);
int  tp2802_device_init(int);
int  tp2802_device_register(void);
void tp2802_device_deregister(void);
int  tp2802_sound_switch(int, int, bool);
void tp2802_set_params(tp2802_params_p);

#endif  /* _TP2802_DEV_H_ */

