/*
 * NVP1104 IOCTL Test Program
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

#include "nextchip/nvp1104.h"

static void nvp1104_ioc_help_msg(void)
{
    fprintf(stdout, "\nUsage: nvp1104_ioc <dev_node> [-bchmns]"
                    "\n       <dev_node>    : nvp1104 device node, ex: /dev/nvp1104.0"
                    "\n       -m            : get Mode"
                    "\n       -n            : get channel NOVID"
                    "\n       -c <ch> [data]: get/set channel contrast"
                    "\n       -b <ch> [data]: get/set channel brightness"
                    "\n       -s <ch> [data]: get/set channel saturation"
                    "\n       -h <ch> [data]: get/set channel hue"
                    "\n\n");
}

static char *vmode_str[] = {"NTSC_720H_1CH", "NTSC_720H_2CH", "NTSC_720H_4CH", "NTSC_CIF_4CH",
                            "PAL_720H_1CH" , "PAL_720H_2CH" , "PAL_720H_4CH" , "PAL_CIF_4CH" };

int main(int argc, char **argv)
{
    int opt, retval, tmp;
    int err = 0;
    int fd = 0;
    const char *dev_name = NULL;
    struct nvp1104_ioc_data ioc_data;

    if(argc < 3) {
        nvp1104_ioc_help_msg();
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

    while((opt = getopt(argc, argv, "mn:b:c:s:h:")) != -1 ) {
        switch(opt) {
            case 'n':   /* Get Channel NOVID */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                    retval = ioctl(fd, NVP1104_GET_NOVID, &ioc_data);
                    if (retval < 0) {
                        fprintf(stderr, "NVP1104_GET_NOVID ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "<<CH#%d>>\n", ioc_data.ch);
                    fprintf(stdout, " NOVID: %s\n", (ioc_data.data != 0) ? "Video Loss" : "Video On");
                }
                break;

            case 'm':   /* Get Mode */
                retval = ioctl(fd, NVP1104_GET_MODE, &tmp);
                if (retval < 0) {
                    fprintf(stderr, "NVP1104_GET_MODE ioctl...error\n");
                    err = -EINVAL;
                    goto exit;
                }
                fprintf(stdout, "Mode: %s\n\n",(tmp >= 0 && tmp < NVP1104_VMODE_MAX) ? vmode_str[tmp] : "Unknown");
                break;

            case 'c':   /* Get/Set Contrast */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    if(optind == argc) {
                        ioc_data.ch = strtol(argv[optind-1], NULL, 0);
                        retval = ioctl(fd, NVP1104_GET_CONTRAST, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP1104_GET_CONTRAST ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Contrast: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nContrast[0x00 ~ 0xff] ==> 0x00=x0, 0x40=x0.5, 0x80=x1, 0xff=x2\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, NVP1104_SET_CONTRAST, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP1104_SET_CONTRAST ioctl...error\n");
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
                        retval = ioctl(fd, NVP1104_GET_BRIGHTNESS, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP1104_GET_BRIGHTNESS ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Brightness: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nBrightness[-128 ~ +127] ==> 0x01=+1, 0x7f=+127, 0x80=-128, 0xff=-1\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, NVP1104_SET_BRIGHTNESS, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP1104_SET_BRIGHTNESS ioctl...error\n");
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
                        retval = ioctl(fd, NVP1104_GET_SATURATION, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP1104_GET_SATURATION ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Saturation: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nSaturation[0x00 ~ 0xff] ==> 0x00=x0, 0x80=x1, 0xc0=x1.5, 0xff=x2\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, NVP1104_SET_SATURATION, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP1104_SET_SATURATION ioctl...error\n");
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
                        retval = ioctl(fd, NVP1104_GET_HUE, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP1104_GET_HUE ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                        fprintf(stdout, "CH%d_Hue: 0x%02x\n", ioc_data.ch, ioc_data.data);
                        fprintf(stdout, "\nHue[0x00 ~ 0xff] ==> 0x00=0, 0x40=90, 0x80=180, 0xff=360\n\n");
                    }
                    else {
                        ioc_data.ch   = strtol(argv[optind-1], NULL, 0);
                        ioc_data.data = strtol(argv[optind+0], NULL, 0);
                        retval = ioctl(fd, NVP1104_SET_HUE, &ioc_data);
                        if (retval < 0) {
                            fprintf(stderr, "NVP1104_SET_HUE ioctl...error\n");
                            err = -EINVAL;
                            goto exit;
                        }
                    }
                }
                break;

            default:
                nvp1104_ioc_help_msg();
                goto exit;
                break;
        }
    }

exit:
    if(fd)
        close(fd);

    return err;
}
