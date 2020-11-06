#ifndef _TW2964_DEV_H_
#define _TW2964_DEV_H_

struct tw2964_dev_t {
    int                 index;      ///< device index
    char                name[16];   ///< device name
    struct semaphore    lock;       ///< device locker
    struct miscdevice   miscdev;    ///< device node for ioctl access

    int                 (*vlos_notify)(int id, int vin, int vlos);
};

struct tw2964_params_t {
    int tw2964_dev_num;
    ushort* tw2964_iaddr;
    int* tw2964_vmode;
    int tw2964_sample_rate;
    int tw2964_sample_size;
    struct tw2964_dev_t* tw2964_dev;
    int init;
    u32* DH9901_TW2964_VCH_MAP;
};
typedef struct tw2964_params_t* tw2964_params_p;

int  tw2964_proc_init(int);
void tw2964_proc_remove(int);
int  tw2964_miscdev_init(int);
int  tw2964_device_init(int);
int  tw2964_sound_switch(int, int, bool);
void tw2964_set_params(tw2964_params_p);

#endif  /* _TW2964_DEV_H_ */
