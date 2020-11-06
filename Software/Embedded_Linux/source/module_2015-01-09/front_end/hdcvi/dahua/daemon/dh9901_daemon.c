/**
 * @file dh9901_daemon.c
 * Daemon for DH9901 Init and Control
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "config.h"
#include "dh9901_lib.h"

#define I2C_SLAVE_FORCE                 0x0706

#define DEFAULT_DH9901_I2C_BUS_ID       0     ///< device i2c bus id
#define DEFAULT_DH9901_DEV_NUM          1     ///< device number on board
#define DEFAULT_DH9901_VCAP_MAP_IDX     0     ///< device CVI and VCAP mapping index
#define DEFAULT_DH9901_POLLING_TIME     500   ///< device polling tim(ms)

struct dh9901_status_t {
    int           vlos;
    int           vfmt;          ///< video format index
    unsigned int  width;         ///< video source width
    unsigned int  height;        ///< video source height
    unsigned int  prog;          ///< 0:interlace 1:progressive
    unsigned int  frame_rate;    ///< currect frame rate
};

struct dh9901_cfg_t {
    int                    dev_num;                                             ///< device number on board
    int                    dev_iaddr[MAX_DH9901_NUM_PER_CPU];                   ///< device i2c address
    char                   i2c_node[32];                                        ///< device i2c device node name
    int                    vport_xcap_map;                                      ///< device vport and xcap mapping table index
    int                    polling_time;                                        ///< device video signal polling time(ms)
    struct dh9901_status_t status[MAX_DH9901_NUM_PER_CPU][DH_CHNS_PER_DH9901];  ///< vin status
};

static char  *dh9901_cfg_path = NULL;
static struct dh9901_cfg_t g_dh9901_cfg;
static int dh9901_vcap_generic_id[MAX_DH9901_NUM_PER_CPU][DH_CHNS_PER_DH9901];

static void dh9901_print_help_msg(void)
{
    fprintf(stdout, "\nUsage: dh9901_daemon [-fh]"
                    "\n       -f <config_file>: system config file path"
                    "\n       -h print this help message"
                    "\n\n");
}

static unsigned char dh9901_read_byte(unsigned char iaddr, unsigned char reg_addr)
{
    int fd;
    unsigned char reg_data = 0;

    if((fd = open(g_dh9901_cfg.i2c_node, O_RDWR)) < 0) {
        fprintf(stderr, "Fail to open %s", g_dh9901_cfg.i2c_node);
        return 0;
    }

    if(ioctl(fd, I2C_SLAVE_FORCE, iaddr>>1) < 0) {
        fprintf(stderr, "Fail to set i2c_addr\n");
        goto exit;
    }

    if(write(fd, &reg_addr, 1) != 1) {
        fprintf(stderr, "Fail to send data\n");
        goto exit;
    }

    if(read(fd, &reg_data, 1) != 1) {
        fprintf(stderr, "Fail to receive data\n");
        goto exit;
    }

exit:
    close(fd);

    return reg_data;
}

static int dh9901_wtite_byte(unsigned char iaddr, unsigned char reg_addr, unsigned char reg_data)
{
    int ret = 0;
    int fd;
    unsigned char write_data[2];

    if((fd = open(g_dh9901_cfg.i2c_node, O_RDWR)) < 0) {
        fprintf(stderr, "Fail to open %s", g_dh9901_cfg.i2c_node);
        return -1;
    }

    if(ioctl(fd, I2C_SLAVE_FORCE, iaddr>>1) < 0) {
        fprintf(stderr, "Fail to set i2c_addr\n");
        ret = -1;
        goto exit;
    }

    write_data[0] = reg_addr;
    write_data[1] = reg_data;

    if(write(fd, write_data, 2) != 2) {
        fprintf(stderr, "Fail to send data\n");
        ret = -1;
        goto exit;
    }

exit:
    close(fd);

    return ret;
}

/******************************************************************************
 Generic EVB Channel and CVI Mapping Table

 VCH#0 ------> CVI#0.0 ---------> VOUT#0.0 -------> X_CAP#0
 VCH#1 ------> CVI#0.1 ---------> VOUT#0.1 -------> X_CAP#1
 VCH#2 ------> CVI#0.2 ---------> VOUT#0.2 -------> X_CAP#2
 VCH#3 ------> CVI#0.3 ---------> VOUT#0.3 -------> X_CAP#3

 ------------------------------------------------------------------------------
 GM8210/GM8287 EVB Channel and CVI Mapping Table (Socket Board)

 VCH#0 ------> CVI#0.0 ---------> VOUT#0.0 -------> X_CAP#1
 VCH#1 ------> CVI#0.1 ---------> VOUT#0.1 -------> X_CAP#0
 VCH#2 ------> CVI#0.2 ---------> VOUT#0.2 -------> X_CAP#3
 VCH#3 ------> CVI#0.3 ---------> VOUT#0.3 -------> X_CAP#2

*******************************************************************************/
static int dh9901_create_vcap_generic_id(int map_idx)
{
    int i, j;

    switch(map_idx) {
        case 1: ///< GM8210/GM8287 Socket Board
            for(i=0; i<g_dh9901_cfg.dev_num; i++) {
                for(j=0; j<DH_CHNS_PER_DH9901; j++) {
                    dh9901_vcap_generic_id[i][j] = i*DH_CHNS_PER_DH9901 + (j+1)%2 + (j/2)*2;
                }
            }
            break;

        case 0:
        default: ///< Generic Board
            for(i=0; i<g_dh9901_cfg.dev_num; i++) {
                for(j=0; j<DH_CHNS_PER_DH9901; j++) {
                    dh9901_vcap_generic_id[i][j] = i*DH_CHNS_PER_DH9901 + j;
                }
            }
            break;
    }

    return 0;
}

static int dh9901_create_system_config(struct dh9901_cfg_t *pcfg)
{
    int ret = 0;
    int i2c_bus_id;

    if(!pcfg)
        return -1;

    /* clear buffer */
    memset(pcfg, 0, sizeof(struct dh9901_cfg_t));

    if(dh9901_cfg_path) {
        ret = getconfigint(NULL, "DEV_NUM", &pcfg->dev_num, dh9901_cfg_path);
        if(ret != 0) {
            fprintf(stderr, "%s: DEV_NUM not exist!\n", dh9901_cfg_path);
            goto exit;
        }

        ret = getconfigint(NULL, "DEV_I2C_BUS", &i2c_bus_id, dh9901_cfg_path);
        if(ret != 0) {
            fprintf(stderr, "%s: DEV_I2C_BUS not exist!\n", dh9901_cfg_path);
            goto exit;
        }
        sprintf(pcfg->i2c_node, "/dev/i2c-%d", i2c_bus_id);

        ret = getconfigint(NULL, "DEV_IADDR0", &pcfg->dev_iaddr[0], dh9901_cfg_path);
        if(ret != 0) {
            fprintf(stderr, "%s: DEV_IADDR0 not exist!\n", dh9901_cfg_path);
            goto exit;
        }

        ret = getconfigint(NULL, "DEV_IADDR1", &pcfg->dev_iaddr[1], dh9901_cfg_path);
        if(ret != 0) {
            fprintf(stderr, "%s: DEV_IADDR1 not exist!\n", dh9901_cfg_path);
            goto exit;
        }

        ret = getconfigint(NULL, "DEV_IADDR2", &pcfg->dev_iaddr[2], dh9901_cfg_path);
        if(ret != 0) {
            fprintf(stderr, "%s: DEV_IADDR2 not exist!\n", dh9901_cfg_path);
            goto exit;
        }

        ret = getconfigint(NULL, "DEV_IADDR3", &pcfg->dev_iaddr[3], dh9901_cfg_path);
        if(ret != 0) {
            fprintf(stderr, "%s: DEV_IADDR3 not exist!\n", dh9901_cfg_path);
            goto exit;
        }

        ret = getconfigint(NULL, "DEV_VPORT_XCAP_MAPPING", &pcfg->vport_xcap_map, dh9901_cfg_path);
        if(ret != 0) {
            fprintf(stderr, "%s: DEV_VPORT_XCAP_MAPPING not exist!\n", dh9901_cfg_path);
            goto exit;
        }

        ret = getconfigint(NULL, "DEV_POLLING_TIME", &pcfg->polling_time, dh9901_cfg_path);
        if(ret != 0) {
            fprintf(stderr, "%s: DEV_POLLING_TIME not exist!\n", dh9901_cfg_path);
            goto exit;
        }
    }
    else {
        /* Device Number */
        pcfg->dev_num = DEFAULT_DH9901_DEV_NUM;

        /* I2C */
        sprintf(pcfg->i2c_node, "/dev/i2c-%d", DEFAULT_DH9901_I2C_BUS_ID);
        pcfg->dev_iaddr[0] = 0x60;
        pcfg->dev_iaddr[1] = 0x62;
        pcfg->dev_iaddr[2] = 0x64;
        pcfg->dev_iaddr[3] = 0x66;

        pcfg->vport_xcap_map = DEFAULT_DH9901_VCAP_MAP_IDX;
        pcfg->polling_time   = DEFAULT_DH9901_POLLING_TIME;
    }

exit:
    return ret;
}

static int dh9901_init(void)
{
    int i, j, ret;
    DH_DH9901_INIT_ATTR_S dev_attr;
    DH_VIDEO_COLOR_S video_color;

    if(g_dh9901_cfg.dev_num >= MAX_DH9901_NUM_PER_CPU)
        return -1;

    memset(&dev_attr, 0, sizeof(DH_DH9901_INIT_ATTR_S));

    /* device number on board */
    dev_attr.ucAdCount = g_dh9901_cfg.dev_num;

    for(i=0; i<dev_attr.ucAdCount; i++) {
        /* device i2c address */
        dev_attr.stDh9901Attr[i].ucChipAddr = g_dh9901_cfg.dev_iaddr[i];

        /* video attr */
        dev_attr.stDh9901Attr[i].stVideoAttr.enSetVideoMode   = DH_SET_MODE_USER;
        dev_attr.stDh9901Attr[i].stVideoAttr.ucChnSequence[0] = 0;  ///< VIN#0 -> VPORT#0
        dev_attr.stDh9901Attr[i].stVideoAttr.ucChnSequence[1] = 1;  ///< VIN#1 -> VPORT#1
        dev_attr.stDh9901Attr[i].stVideoAttr.ucChnSequence[2] = 2;  ///< VIN#2 -> VPORT#2
        dev_attr.stDh9901Attr[i].stVideoAttr.ucChnSequence[3] = 3;  ///< VIN#3 -> VPORT#3
        dev_attr.stDh9901Attr[i].stVideoAttr.ucVideoOutMux    = 0;  ///< 0:BT656, 1:BT1120

        /* audio attr */
        dev_attr.stDh9901Attr[i].stAuioAttr.bAudioEnable = 1;
        dev_attr.stDh9901Attr[i].stAuioAttr.stAudioConnectMode.ucCascade      = 0;
        dev_attr.stDh9901Attr[i].stAuioAttr.stAudioConnectMode.ucCascadeNum   = 0;
        dev_attr.stDh9901Attr[i].stAuioAttr.stAudioConnectMode.ucCascadeStage = 0;
        dev_attr.stDh9901Attr[i].stAuioAttr.enSetAudioMode    = DH_SET_MODE_USER;
        dev_attr.stDh9901Attr[i].stAuioAttr.enAudioSampleRate = DH_AUDIO_SAMPLERATE_8k;
        dev_attr.stDh9901Attr[i].stAuioAttr.enAudioEncClkMode = DH_AUDIO_ENCCLK_MODE_MASTER;
        dev_attr.stDh9901Attr[i].stAuioAttr.enAudioSyncMode   = DH_AUDIO_SYNC_MODE_I2S;
        dev_attr.stDh9901Attr[i].stAuioAttr.enAudioEncClkSel  = DH_AUDIO_ENCCLK_MODE_27M;

        /* rs485 attr */
        dev_attr.stDh9901Attr[i].stPtz485Attr.enSetVideoMode = DH_SET_MODE_USER;
        dev_attr.stDh9901Attr[i].stPtz485Attr.ucProtocolLen  = 7;
    }

    /* auto detect thread attr */
    dev_attr.stDetectAttr.enDetectMode   = DH_SET_MODE_USER;
    dev_attr.stDetectAttr.ucDetectPeriod = 200; ///< ms

    /* register i2c read/write function */
    dev_attr.Dh9901_WriteByte = dh9901_wtite_byte;
    dev_attr.Dh9901_ReadByte  = dh9901_read_byte;

    ret = DH9901_API_Init(&dev_attr);
    if(ret != 0)
        goto exit;

    /* setup default color setting */
    video_color.ucBrightness   = 50;
    video_color.ucContrast     = 50;
    video_color.ucSaturation   = 50;
    video_color.ucHue          = 50;
    video_color.ucGain         = 0;
    video_color.ucWhiteBalance = 0;
    video_color.ucSharpness    = 1;
    for(i=0; i<dev_attr.ucAdCount; i++) {
        for(j=0; j<DH_CHNS_PER_DH9901; j++) {
            ret = DH9901_API_SetColor(i, j, &video_color, DH_SET_MODE_USER);
            if(ret != 0)
                goto exit;
        }
    }

exit:
    return ret;
}

int main(int argc, char *argv[])
{
    pid_t pid, sid;
    int i, j, opt, ret, m_idx;
    char cmd_buf[128];
    DH_VIDEO_STATUS_S dh_status;
    struct dh9901_status_t vin_status;

    while((opt = getopt(argc, argv, "f:h")) != -1 ) {
        switch(opt){
            case 'f':   /* Get system configuration from config file */
                if(optarg==NULL) {
                    exit(1);
                }
                else {
                    dh9901_cfg_path = optarg;
                }
                break;
            default:
                dh9901_print_help_msg();
                exit(0);
        }
    }

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0)
        exit(1);

    /* Exit the parent process. */
    if (pid > 0)
        exit(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0)
        exit(1);

    /* Change the current working directory to root */
    if ((chdir("/")) < 0)
        exit(1);

    /* Daemon-specific initialization goes here */

    /* create dh9901 system config */
    ret = dh9901_create_system_config(&g_dh9901_cfg);
    if(ret != 0)
        exit(1);

    /* create vcap generic id mapping table */
    ret = dh9901_create_vcap_generic_id(g_dh9901_cfg.vport_xcap_map);
    if(ret != 0)
        exit(1);

    /* init dh9901 device */
    ret = dh9901_init();
    if(ret != 0)
        exit(1);

    /* check video format and video signal status */
    while (1) {
        for(i=0; i<g_dh9901_cfg.dev_num; i++) {
            for(j=0; j<DH_CHNS_PER_DH9901; j++) {
                ret = DH9901_API_GetVideoStatus(i, j, &dh_status);
                if(ret == 0) {
                    m_idx = dh9901_vcap_generic_id[i][j];

                    vin_status.vlos = dh_status.ucVideoLost;
                    vin_status.vfmt = dh_status.ucVideoFormat;
                    switch(vin_status.vfmt) {
                        case DH_HD_720P_30HZ:
                            vin_status.width      = 1280;
                            vin_status.height     = 720;
                            vin_status.prog       = 1;
                            vin_status.frame_rate = 30;
                            break;
                        case DH_HD_720P_50HZ:
                            vin_status.width      = 1280;
                            vin_status.height     = 720;
                            vin_status.prog       = 1;
                            vin_status.frame_rate = 50;
                            break;
                        case DH_HD_720P_60HZ:
                            vin_status.width      = 1280;
                            vin_status.height     = 720;
                            vin_status.prog       = 1;
                            vin_status.frame_rate = 60;
                            break;
                        case DH_HD_1080P_25HZ:
                            vin_status.width      = 1920;
                            vin_status.height     = 1080;
                            vin_status.prog       = 1;
                            vin_status.frame_rate = 25;
                            break;
                        case DH_HD_1080P_30HZ:
                            vin_status.width      = 1920;
                            vin_status.height     = 1080;
                            vin_status.prog       = 1;
                            vin_status.frame_rate = 30;
                            break;
                        case DH_HD_720P_25HZ:
                        default:
                            vin_status.width      = 1280;
                            vin_status.height     = 720;
                            vin_status.prog       = 1;
                            vin_status.frame_rate = 25;
                            break;
                    }

                    /* width x hieght */
                    if((vin_status.width != g_dh9901_cfg.status[i][j].width) || (vin_status.height != g_dh9901_cfg.status[i][j].height)) {
                        sprintf(cmd_buf, "echo %d %d > /proc/vcap300/input_module/generic.%d/norm", vin_status.width, vin_status.height, m_idx);
                        system(cmd_buf);

                        g_dh9901_cfg.status[i][j].width  = vin_status.width;
                        g_dh9901_cfg.status[i][j].height = vin_status.height;
                    }

                    /* frame rate & timeout & speed */
                    if((vin_status.frame_rate != 0) && (vin_status.frame_rate != g_dh9901_cfg.status[i][j].frame_rate)) {
                        sprintf(cmd_buf, "echo %d > /proc/vcap300/input_module/generic.%d/frame_rate", vin_status.frame_rate, m_idx);
                        system(cmd_buf);

                        g_dh9901_cfg.status[i][j].frame_rate = vin_status.frame_rate;

                        sprintf(cmd_buf, "echo %d > /proc/vcap300/input_module/generic.%d/timeout", (1000/vin_status.frame_rate)*2, m_idx);
                        system(cmd_buf);

                        if((vin_status.frame_rate == 50) || (vin_status.frame_rate == 60)) {
                            sprintf(cmd_buf, "echo 2 > /proc/vcap300/input_module/generic.%d/speed", m_idx);
                            system(cmd_buf);
                        }
                        else {
                            sprintf(cmd_buf, "echo 1 > /proc/vcap300/input_module/generic.%d/speed", m_idx);
                            system(cmd_buf);
                        }
                    }

                    /* video loss & video format index */
                    g_dh9901_cfg.status[i][j].vlos = vin_status.vlos;
                    g_dh9901_cfg.status[i][j].vfmt = vin_status.vfmt;
                }
            }
        }

        usleep(1000*g_dh9901_cfg.polling_time);
    }

    exit(0);
}
