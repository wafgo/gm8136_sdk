/*
 * Real Time Clock Driver Test/Example Program
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

static void rtc_help_msg(void)
{
    fprintf(stdout, "\nUsage: rtctest <rtcdev> [-rs]"
                    "\n       <rtcdev>  : rtc device node, ex: /dev/rtc"
                    "\n       -r        : read RTC date/time"
                    "\n       -s <year> <mon> <day> <hour> <min> <sec> : set RTC date/time"
                    "\n\n");
}

int main(int argc, char **argv)
{
    int opt, retval;
    int err = 0;
    int fd = 0;
    struct rtc_time rtc_tm;
    const char *rtc_dev = NULL;

    if(argc < 3) {
        rtc_help_msg();
        return -EPERM;
    }

    /* get rtc device name */
    rtc_dev = argv[1];

    /* open rtc device node */
    fd = open(rtc_dev, O_RDONLY);
    if (fd ==  -1) {
        fprintf(stderr, "open %s failed(%s)\n", rtc_dev, strerror(errno));
        err = errno;
        goto exit;
    }

    while((opt = getopt(argc, argv, "rs:")) != -1 ) {
        switch(opt) {
            case 'r':   /* Read the RTC time/date */
                retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
                if (retval == -1) {
                    fprintf(stderr, "RTC_RD_TIME ioctl\n");
                    err = -EINVAL;
                    goto exit;
                }
                fprintf(stdout, "\nCurrent RTC date/time is %d-%d-%d %02d:%02d:%02d\n",
                        rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
                        rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
                break;
            case 's':
                if(optarg==NULL) {
                    err = -EINVAL;
                    goto exit;
                }
                else {
                    if((optind+4) < argc) {
                        memset(&rtc_tm, 0, sizeof(struct rtc_time));
                        rtc_tm.tm_year = strtol(argv[optind-1]  , NULL, 0) - 1900;
                        rtc_tm.tm_mon  = strtol(argv[optind+0], NULL, 0) - 1;
                        rtc_tm.tm_mday = strtol(argv[optind+1], NULL, 0);
                        rtc_tm.tm_hour = strtol(argv[optind+2], NULL, 0);
                        rtc_tm.tm_min  = strtol(argv[optind+3], NULL, 0);
                        rtc_tm.tm_sec  = strtol(argv[optind+4], NULL, 0);
                        retval = ioctl(fd, RTC_SET_TIME, &rtc_tm);
                        if (retval == -1) {
                            fprintf(stderr, "RTC_SET_TIME ioctl\n");
                            err = -EINVAL;
                            goto exit;
                        }
                    }
                    else {
                        fprintf(stderr, "error!!..please specify <year> <mon> <day> <hour> <min> <sec>\n");
                        err = -EINVAL;
                        goto exit;
                    }
                }
                break;
            default:
                rtc_help_msg();
                goto exit;
                break;
        }
    }

exit:
    if(fd)
        close(fd);

    return err;
}
