/*
 * ES8328 IOCTL Test Program
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

#include "everest/es8328.h"

static void es8328_ioc_help_msg(void)
{
    fprintf(stdout, "\nUsage: es8328_ioc <dev_node> [-bchmnosv]"
                    "\n       <dev_node>    : es8328 device node, ex: /dev/es8328.0"
                    "\n       -m            : get Mode"
                    "\n       -n            : get channel NOVID"
                    "\n       -c <ch> [data]: get/set channel contrast"
                    "\n       -b <ch> [data]: get/set channel brightness"
                    "\n       -s <ch> [data]: get/set channel saturation"
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
                                "CH 13", "CH 14", "CH 15", "CH 16", "FIRST PLAYBACK AUDIO",
                                "SECOND PLAYBACK AUDIO", "THIRD PLAYBACK AUDIO",
                                "FOURTH PLAYBACK AUDIO", "MIC input 1", "MIC input 2",
                                "MIC input 3", "MIC input 4", "Mixed audio", "no audio output"};

int main(int argc, char **argv)
{
    int opt, retval, tmp;
    int err = 0;
    int fd = 0;
    const char *dev_name = NULL;
    struct es8328_ioc_data ioc_data;

    if(argc < 3) {
        es8328_ioc_help_msg();
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

    while((opt = getopt(argc, argv, "mvon:b:c:s:h:")) != -1 ) {
        switch(opt) {
            case 'v':   /* Get/Set Volume */
                if (optind == argc) {
                    retval = ioctl(fd, ES8328_GET_VOL, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "ES8328_GET_VOL ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "VOL: %d\n\n", tmp);
                }
                else {
                    tmp = strtol(argv[optind+0], NULL, 0);
                    retval = ioctl(fd, ES8328_SET_VOL, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "ES8328_SET_VOL ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                }
                break;

            case 'o':   /* Get/Set output channel */
                if (optind == argc) {
                    retval = ioctl(fd, ES8328_GET_OUT_CH, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "ES8328_GET_OUT_CH ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "Output channel: %s\n\n", output_ch_str[tmp]);
                }
                else {
                    tmp = strtol(argv[optind+0], NULL, 0);
                    retval = ioctl(fd, ES8328_SET_OUT_CH, &tmp);
                    if (retval < 0) {
                        fprintf(stderr, "ES8328_SET_OUT_CH ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                }
                break;

            default:
                es8328_ioc_help_msg();
                goto exit;
                break;
        }
    }

exit:
    if(fd)
        close(fd);

    return err;
}
