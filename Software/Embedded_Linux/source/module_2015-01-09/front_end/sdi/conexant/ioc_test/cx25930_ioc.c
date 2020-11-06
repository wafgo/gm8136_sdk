/*
 * CX25930 IOCTL Test Program
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

#include "conexant/cx25930.h"

static void cx25930_ioc_help_msg(void)
{
    fprintf(stdout, "\nUsage: cx25930_ioc <dev_node> [-nv]"
                    "\n       <dev_node>    : cx25930 device node, ex: /dev/cx25930.0"
                    "\n       -n <sdi_ch>   : get SDI Channel NOVID"
                    "\n       -v <sdi_ch>   : get SDI Channel Video Format"
                    "\n\n");
}

int main(int argc, char **argv)
{
    int opt, retval, tmp;
    int err = 0;
    int fd = 0;
    const char *dev_name = NULL;

    if(argc < 3) {
        cx25930_ioc_help_msg();
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

    while((opt = getopt(argc, argv, "n:v:")) != -1 ) {
        switch(opt) {
            case 'n':   /* Get SDI Channel NOVID */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    struct cx25930_ioc_data_t ioc_data;

                    ioc_data.sdi_ch = strtol(argv[optind-1], NULL, 0);
                    retval = ioctl(fd, CX25930_GET_NOVID, &ioc_data);
                    if (retval < 0) {
                        fprintf(stderr, "CX25930_GET_NOVID ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "<<SDI#%d>>\n", ioc_data.sdi_ch);
                    fprintf(stdout, " NOVID: %s\n", (ioc_data.data != 0) ? "Video Loss" : "Video On");
                }
                break;

            case 'v':   /* Get SDI Channel Video Format */
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    struct cx25930_ioc_vfmt_t ioc_vfmt;
                    char *bit_rate_str[] = {"270Mbps", "1.485Gbps", "2.97Gbps"};

                    ioc_vfmt.sdi_ch = strtol(argv[optind-1], NULL, 0);
                    retval = ioctl(fd, CX25930_GET_VIDEO_FMT, &ioc_vfmt);
                    if (retval < 0) {
                        fprintf(stderr, "CX25930_GET_VIDEO_FMT ioctl...error\n");
                        err = -EINVAL;
                        goto exit;
                    }
                    fprintf(stdout, "<<SDI#%d>>\n", ioc_vfmt.sdi_ch);
                    fprintf(stdout, " Active_Width : %d\n",     ioc_vfmt.active_width);
                    fprintf(stdout, " Active_Height: %d\n",     ioc_vfmt.active_height);
                    fprintf(stdout, " Total_Width  : %d\n",     ioc_vfmt.total_width);
                    fprintf(stdout, " Total_Height : %d\n",     ioc_vfmt.total_height);
                    fprintf(stdout, " Pixel_Rate   : %d(Hz)\n", ioc_vfmt.pixel_rate);
                    fprintf(stdout, " Bit_Width    : %d\n",     ioc_vfmt.bit_width);
                    fprintf(stdout, " Prog/Inter   : %s\n",     ((ioc_vfmt.prog == 1) ? "Progressive" : "Interlace"));
                    fprintf(stdout, " Frame_Rate   : %d\n",     ioc_vfmt.frame_rate);
                    fprintf(stdout, " Bit_Rate     : %s\n",     ((ioc_vfmt.bit_rate <= 2) ? bit_rate_str[ioc_vfmt.bit_rate] : "unknown"));
                }
                break;

            default:
                cx25930_ioc_help_msg();
                goto exit;
                break;
        }
    }

exit:
    if(fd)
        close(fd);

    return err;
}
