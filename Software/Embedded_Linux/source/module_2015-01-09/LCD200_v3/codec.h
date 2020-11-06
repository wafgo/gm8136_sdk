/*
 * This header file should be included by codec driver. It is used to register a codec driver to the 
 * database. When the user chooses the codec, LCD module will hook the exactly driver from the database 
 * and execute it.
 */
#ifndef __CODEC_H__
#define __CODEC_H__

#include "ffb.h"
#include "proc.h"
#include "lcd_def.h"

#define NAME_LEN    30

typedef struct {
    OUTPUT_TYPE_T codec_id;     /* this id is unique and can't conflict with others. */
    char name[NAME_LEN + 1];    /* the name of driver */
    int (*probe) (void *private);       /* will be called immediately while registering. only called while registering */
    void (*setting) (struct ffb_dev_info * info);       /* may be called several times */
    int (*enc_proc_init) (void);        /* proc init function */
    void (*enc_proc_remove) (void);     /* proc remove function */
    void (*remove) (void *private);     /* remove function */
    void *private;              /* private data */
} codec_driver_t;

/* 
 * Init codec database
 */
void codec_middleware_init(void);
void codec_middle_exit(void);
void codec_dump_database(void);

/* 
 * register the codec driver. The probe function will be called immediately while registering
 * return value: 0 for success, -1 for fail
 */
int codec_driver_register(codec_driver_t * driver);

/* 
 * de-register the codec driver. 
 */

int codec_driver_deregister(codec_driver_t * driver);

/* Get the system output_type
 */
OUTPUT_TYPE_T codec_get_output_type(int lcd_idx);

/*
 * We treat each function in a codec is a codec_node. For example, if the codec supports 480i, 480p, 720p, then
 * it will have 3 codec nodes.
 */
typedef struct {
    OUTPUT_TYPE_T output_type;
    void *tunnel_data;          //the data from PIP and pass to codec
} codec_device_t;

typedef struct {
    codec_driver_t driver;      //update from codec driver

    /* update from pip driver
     */
    codec_device_t device;
    struct list_head list;
} codec_setup_t;

/* return 0 for success, -1 for fail
 */
int codec_pip_new_device(unsigned char lcd_idx, codec_setup_t * setup_dev);
int codec_pip_remove_device(unsigned char lcd_idx, codec_setup_t * setup_dev);

#endif /* __CODEC_H__ */
