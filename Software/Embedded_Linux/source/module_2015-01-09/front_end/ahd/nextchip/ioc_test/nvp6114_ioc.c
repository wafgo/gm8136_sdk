/*
 * NVP6114 IOCTL Test Program
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

#include "nextchip/nvp6114.h"

static void nvp6114_ioc_help_msg(void)
{
    fprintf(stdout, "\nUsage: nvp6114_ioc <dev_node> [-bchmnpsv]"
                    "\n       <dev_node>    : nvp6114 device node, ex: /dev/nvp6114.0"
                    "\n       -m [data]     : get/set Mode"
                    "\n       -n <ch>       : get channel NOVID"
                    "\n       -v <ch>       : get channel input video format"
                    "\n       -c <ch> [data]: get/set channel contrast"
                    "\n       -b <ch> [data]: get/set channel brightness"
                    "\n       -s <ch> [data]: get/set channel saturation"
                    "\n       -h <ch> [data]: get/set channel hue"
                    "\n       -p <ch> [data]: get/set channel sharpness"
                    "\n       -e [data]     : get/set volume"
                    "\n       -o [data]     : get/set output channel"
                    "\n\n");
}

static char *vmode_str[] = {"NTSC_720H_1CH", "NTSC_720H_2CH", "NTSC_720H_4CH",
                            "NTSC_960H_1CH", "NTSC_960H_2CH", "NTSC_960H_4CH",
                            "NTSC_720P_1CH", "NTSC_720P_2CH",
                            "PAL_720H_1CH" , "PAL_720H_2CH" , "PAL_720H_4CH",
                            "PAL_960H_1CH" , "PAL_960H_2CH" , "PAL_960H_4CH",
                            "PAL_720P_1CH",  "PAL_720P_2CH"};

static char *output_ch_str[] = {"CH 1", "CH 2", "CH 3", "CH 4", "CH 5", "CH 6",
                                "CH 7", "CH 8", "CH 9", "CH 10", "CH 11", "CH 12",
                                "CH 13", "CH 14", "CH 15", "CH 16", "FIRST PLAYBACK AUDIO",
                                "SECOND PLAYBACK AUDIO", "THIRD PLAYBACK AUDIO",
                                "FOURTH PLAYBACK AUDIO", "MIC input 1", "MIC input 2",
                                "MIC input 3", "MIC input 4", "Mixed audio", "no audio output"};


static char *vfmt_str[] = {"Unknown", "SD", "SD_NTSC", "SD_PAL", "AHD_30P", "AHD_25P"};

int main(int argc, char **argv)
{
    int opt, retval, tmp;
    int err = 0;
    int fd = 0;
    const char *dev_name = NULL;
    struct nvp6114_ioc_data ioc_data;

    if(argc < 3) {
        nvp6114_ioc_help_msg();
        return -EPERM;
    }

    /* get device name */
    dev_name = argv[1];

    /* open device node */
    fd = open(dev_name, O_RDONLY);
    if (fd ==  -1) {
        fprintf(stderr, "open %s failed(%s)\n", dev_name, strerror(errno));
        err = errno;
        goto exit;
    }

    while((opt = getopt(argc, argv, "mn:b:c:s:h:p:v:")) != -1 ) {
        switch(opt) {
            case 'n':   /* Get Channel NOVID */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                    retval = ioctl(fd, NVP6114_GET_NOVID, &ioc_data);
                    if (retval < 0) {
                        fprintf(stderr, "NVP6114_GET_NOVID ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "<<CH#%d>>\n", ioc_data.ch);
                    fprintf(stdout, " NOVID: %s\n", (ioc_data.data != 0) ? "Video Loss" : "Video On");
                }
                break;

            case 'm':   /* Get/Set Mode */
                if (optind == argc) {
                    retval = ioctl(fd, NVP6114_GET_MODE, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "NVP6114_GET_MODE ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "Mode: %s\n\n",(tmp >= 0 && tmp < NVP6114_VMODE_MAX) ? vmode_str[tmp] : "Unknown");
                }
                else {
                    tmp = strtol(argv[optind+0], NULL, 0);
                    retval = ioctl(fd, NVP6114_SET_MODE, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "NVP6114_SET_MODE ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                }
                break;

            case 'c':   /* Get/Set Contrast */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    if(optind == argc) {
                        ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                        retval = ioctl(fd, NVP6114_GET_CONTRAST, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP6114_GET_CONTRAST ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Contrast: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nContrast[0x00 ~ 0xff] ==> 0x00=x0, 0x40=x0.5, 0x80=x1, 0xff=x2\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, NVP6114_SET_CONTRAST, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP6114_SET_CONTRAST ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                    }
                }
                break;

            case 'b':   /* Get/Set Brightness */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    if(optind == argc) {
                        ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                        retval = ioctl(fd, NVP6114_GET_BRIGHTNESS, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP6114_GET_BRIGHTNESS ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Brightness: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nBrightness[-128 ~ +127] ==> 0x01=+1, 0x7f=+127, 0x80=-128, 0xff=-1\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        printf("ch %d data %d\n", ioc_data.ch, ioc_data.data);
                        retval = ioctl(fd, NVP6114_SET_BRIGHTNESS, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP6114_SET_BRIGHTNESS ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                    }
                }
                break;

            case 's':   /* Get/Set Saturation */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    if(optind == argc) {
                        ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                        retval = ioctl(fd, NVP6114_GET_SATURATION, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP6114_GET_SATURATION ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Saturation: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nSaturation[0x00 ~ 0xff] ==> 0x00=x0, 0x80=x1, 0xc0=x1.5, 0xff=x2\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, NVP6114_SET_SATURATION, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP6114_SET_SATURATION ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                    }
                }
                break;

            case 'h':   /* Get/Set Hue */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    if(optind == argc) {
                        ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                        retval = ioctl(fd, NVP6114_GET_HUE, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP6114_GET_HUE ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Hue: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nHue[0x00 ~ 0xff] ==> 0x00=0, 0x40=90, 0x80=180, 0xff=360\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, NVP6114_SET_HUE, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP6114_SET_HUE ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                    }
                }
                break;

            case 'p':   /* Get/Set Sharpness */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    if(optind == argc) {
                        ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                        retval = ioctl(fd, NVP6114_GET_SHARPNESS, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP6114_GET_SHARPNESS ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Sharpness: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nH_Sharpness[0x0 ~ 0xf] - Bit[7:4] ==> 0x00:x0, 0x4:x0.5, 0x8:x1, 0xf:x2");
                        fprintf(stdout, "\nV_Sharpness[0x0 ~ 0xf] - Bit[3:0] ==> 0x00:x1, 0x4:x2,   0x8:x3, 0xf:x4\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, NVP6114_SET_SHARPNESS, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP6114_SET_SHARPNESS ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                    }
                }
                break;

            case 'v':   /* Get Channel Input Video Format */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                    retval = ioctl(fd, NVP6114_GET_INPUT_VFMT, &ioc_data);
                    if (retval < 0) {
                        fprintf(stderr, "NVP6114_GET_INPUT_VFMT ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "<<CH#%d>>\n", ioc_data.ch);
                    fprintf(stdout, " VFMT: %s\n", (ioc_data.data >= 0 && ioc_data.data <= NVP6114_INPUT_VFMT_MAX) ? vfmt_str[ioc_data.data] : "Unknown");
                }
                break;

            case 'e':   /* Get/Set Volume */
                if (optind == argc) {
                    retval = ioctl(fd, NVP6114_GET_VOL, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "NVP6114_GET_VOL ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "VOL: %d\n\n", tmp);
                }
                else {
                    tmp = strtol(argv[optind+0], NULL, 0);
                    retval = ioctl(fd, NVP6114_SET_VOL, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "NVP6114_SET_VOL ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                }
                break;

            case 'o':   /* Get/Set output channel */
                if (optind == argc) {
                    retval = ioctl(fd, NVP6114_GET_OUT_CH, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "NVP6114_GET_OUT_CH ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "Output channel: %s\n\n", output_ch_str[tmp]);
                }
                else {
                    tmp = strtol(argv[optind+0], NULL, 0);
                    retval = ioctl(fd, NVP6114_SET_OUT_CH, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "NVP6114_SET_OUT_CH ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                }
                break;

            default:
                nvp6114_ioc_help_msg();
                goto exit;
                break;
        }
    }

exit:
    if(fd)
        close(fd);

    return err;
}
