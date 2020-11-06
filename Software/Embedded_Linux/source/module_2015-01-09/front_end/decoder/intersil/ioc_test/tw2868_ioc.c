/*
 * TW2868 IOCTL Test Program
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

#include "intersil/tw2868.h"

static void tw2868_ioc_help_msg(void)
{
    fprintf(stdout, "\nUsage: tw2868_ioc <dev_node> [-bchmnosv]"
                    "\n       <dev_node>    : tw2868 device node, ex: /dev/tw2868.0"
                    "\n       -m            : get Mode"
                    "\n       -n            : get channel NOVID"
                    "\n       -c <ch> [data]: get/set channel contrast"
                    "\n       -b <ch> [data]: get/set channel brightness"
                    "\n       -s <ch> [data]: get/set channel saturation_u"
                    "\n       -t <ch> [data]: get/set channel saturation_v"
                    "\n       -h <ch> [data]: get/set channel hue"
                    "\n       -v [data]     : get/set volume"
                    "\n       -o [data]     : get/set output channel"
                    "\n\n");
}

static char *vmode_str[] = {"NTSC_720H_1CH", "NTSC_720H_2CH", "NTSC_720H_4CH",
                            "NTSC_960H_1CH", "NTSC_960H_2CH", "NTSC_960H_4CH",
                            "PAL_720H_1CH" , "PAL_720H_2CH" , "PAL_720H_4CH",
                            "PAL_960H_1CH" , "PAL_960H_2CH" , "PAL_960H_4CH"};

static char *output_ch_str[] = {"CH 1", "CH 2", "CH 3", "CH 4", "CH 5", "CH 6",
                                "CH 7", "CH 8", "CH 9", "CH 10", "CH 11", "CH 12",
                                "CH 13", "CH 14", "CH 15", "CH 16", "PLAYBACK first stage",
                                "Reserved", "PLAYBACK last stage", "Reserved",
                                "Mixed audio", "AIN51", "AIN52", "AIN53", "AIN54"};

int main(int argc, char **argv)
{
    int opt, retval, tmp;
    int err = 0;
    int fd = 0;
    const char *dev_name = NULL;
    struct tw2868_ioc_data ioc_data;

    if(argc < 3) {
        tw2868_ioc_help_msg();
        return -EPERM;
    }

    /* get device name */
    dev_name = argv[1];

    /* open rtc device node */
    fd = open(dev_name, O_RDONLY);
    if (fd ==  -1) {
        fprintf(stderr, "open %s failed(%s)\n", dev_name, strerror(errno));
        err = errno;
        goto exit;
    }

    while((opt = getopt(argc, argv, "mvon:b:c:s:t:h:")) != -1 ) {
        switch(opt) {
            case 'n':   /* Get Channel NOVID */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                    retval = ioctl(fd, TW2868_GET_NOVID, &ioc_data);
                    if (retval < 0) {
                        fprintf(stderr, "TW2868_GET_NOVID ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "<<CH#%d>>\n", ioc_data.ch);
                    fprintf(stdout, " NOVID: %s\n", (ioc_data.data != 0) ? "Video Loss" : "Video On");
                }
                break;

            case 'm':   /* Get Mode */
                retval = ioctl(fd, TW2868_GET_MODE, &tmp);
                if (retval < 0) {
                    fprintf(stderr, "TW2868_GET_MODE ioctl...error\n");
                    err = -EINVAL;
                    goto exit;
                }
                fprintf(stdout, "Mode: %s\n\n",(tmp >= 0 && tmp < TW2868_VMODE_MAX) ? vmode_str[tmp] : "Unknown");
                break;

            case 'c':   /* Get/Set Contrast */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    if(optind == argc) {
                        ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                        retval = ioctl(fd, TW2868_GET_CONTRAST, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "TW2868_GET_CONTRAST ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Contrast: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nContrast[0%% ~ 255%%] ==> 0x00=0%%, 0xff=255%%\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, TW2868_SET_CONTRAST, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "TW2868_SET_CONTRAST ioctl...error\n");
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
                        retval = ioctl(fd, TW2868_GET_BRIGHTNESS, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "TW2868_GET_BRIGHTNESS ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Brightness: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nBrightness[-128 ~ +127] ==> 0x01=+1, 0x7f=+127, 0x80=-128, 0xff=-1\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, TW2868_SET_BRIGHTNESS, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "TW2868_SET_BRIGHTNESS ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                    }
                }
                break;

            case 's':   /* Get/Set Saturation_U */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    if(optind == argc) {
                        ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                        retval = ioctl(fd, TW2868_GET_SATURATION_U, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "TW2868_GET_SATURATION_U ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Saturation_U: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nSaturation_U[0%% ~ 200%%] ==> 0x80=100%%\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, TW2868_SET_SATURATION_U, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "TW2868_SET_SATURATION_U ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                    }
                }
                break;

            case 't':   /* Get/Set Saturation_V */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    if(optind == argc) {
                        ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                        retval = ioctl(fd, TW2868_GET_SATURATION_V, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "TW2868_GET_SATURATION_V ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Saturation_V: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nSaturation_V[0%% ~ 200%%] ==> 0x80=100%%\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, TW2868_SET_SATURATION_V, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "TW2868_SET_SATURATION_V ioctl...error\n");
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
                        retval = ioctl(fd, TW2868_GET_HUE, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "TW2868_GET_HUE ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Hue: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nHue[0x00 ~ 0xff] ==> 0x00=0, 0x7f=90, 0x80=-90\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, TW2868_SET_HUE, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "TW2868_SET_HUE ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                    }
                }
                break;

            case 'v':   /* Get/Set Volume */
                if (optind == argc) {
                    retval = ioctl(fd, TW2868_GET_VOL, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "TW2868_GET_VOL ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "VOL: %d\n\n", tmp);
                }
                else {
                    tmp = strtol(argv[optind+0], NULL, 0);
                    retval = ioctl(fd, TW2868_SET_VOL, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "TW2868_SET_VOL ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                }
                break;

            case 'o':   /* Get/Set output channel */
                if (optind == argc) {
                    retval = ioctl(fd, TW2868_GET_OUT_CH, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "TW2868_GET_OUT_CH ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "Output channel: %s\n\n", output_ch_str[tmp]);
                }
                else {
                    tmp = strtol(argv[optind+0], NULL, 0);
                    retval = ioctl(fd, TW2868_SET_OUT_CH, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "TW2868_SET_OUT_CH ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                }
                break;

            default:
                tw2868_ioc_help_msg();
                goto exit;
                break;
        }
    }

exit:
    if(fd)
        close(fd);

    return err;
}
