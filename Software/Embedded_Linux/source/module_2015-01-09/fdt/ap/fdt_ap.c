#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include "../fdt_ioctl.h"


void help()
{
    printf("Usage: [-c CAP_VCH] [-W WIDTH] [-H HEIGHT] [-w FIRST_WIDTH] [-h FIRST_HEIGHT] [-r RATIO] [-s SENSITIVITY] [-t TIMEOUT] [-f MAX_FACE_NUMBER] [-v 0|1] [-d DLY_FRAME_NUMBER] [-m MIN_FRAME_NUMBER] [-M MAX_FRAME_NUMBER] [-u FRAME_NUMBER]\n");
    printf("        -c    Capture channel selection. Default: 0.\n");
    printf("        -W    Source image width. Default: 1920.\n");
    printf("        -H    Source image height. Default: 1080.\n");
    printf("        -w    First scaled image width. Must be multiple of 4. Range: 64 ~ 320. Default: 320.\n");
    printf("        -h    First scaled image height. Must be multiple of 2. Range: 36 ~ 320. Default: 180.\n");
    printf("        -r    Scaling ratio. Range: 80 ~ 91. Default: 80.\n");
    printf("        -s    Detection sensitivity. The higher value the more sensitivity. Range: 0 ~ 9. Default: 7.\n");
    printf("        -t    Scaling timeout value in millisecond. Default: 1000.\n");
    printf("        -f    Maximum number of detected faces. Range: 1 ~ 1024. Default: 64.\n");
    printf("        -v    Anti-variation function switch. Set 0 to disable, 1 to enable. Default: 0.\n");
    printf("        -d    A face rectangle is removed if it's not detected for continuous DLY_FRAME_NUMBER frames. Range: 0 ~ 127. Default: 3.\n");
    printf("        -m    Minimum number of frames monitored by anti-variation. Range: 1 ~ 16. Default: 3.\n");
    printf("        -M    Maximum number of frames monitored by anti-variation. Range: 1 ~ 16. Default: 9.\n");
    printf("        -u    Runs for specified frame count. Default: infinity.\n");
}

int isValidParam(fdt_param_t *pParam)
{
    if ((pParam->first_width < 64) || (pParam->first_width > 320))
        return 0;
    if ((pParam->first_height < 36) || (pParam->first_height > 320))
        return 0;
    if ((pParam->ratio < 80) || (pParam->ratio > 91))
        return 0;
    if ((pParam->sensitivity < 0) || (pParam->sensitivity > 9))
        return 0;
    if ((pParam->max_face_num < 1) || (pParam->max_face_num > 1024))
        return 0;
    if ((pParam->av_param.delayed_vanish_frm_num < 0) || (pParam->av_param.delayed_vanish_frm_num > 127))
        return 0;
    if ((pParam->av_param.min_frm_num < 1) || (pParam->av_param.min_frm_num > 16))
        return 0;
    if ((pParam->av_param.max_frm_num < pParam->av_param.min_frm_num) || (pParam->av_param.max_frm_num > 16))
        return 0;

    return 1;
}

int main(int argc, char **argv)
{
    int i;
    int ret = 0;
    fdt_param_t fdtParam;
    int fd;
    int frameCount = 0;
    int maxFrameCount = -1;
    const int fpsFrameCount = 10;
    unsigned short faceIdx;
    unsigned short *pVal;
    struct timeval t1;
    struct timeval t2;
    unsigned long td;
    float fps;

    // default parameters
    fdtParam.cap_vch = 0;
    fdtParam.first_width = 320;
    fdtParam.first_height = 180;
    fdtParam.ratio = 80;
    fdtParam.source_width = 1920;
    fdtParam.source_height = 1080;
    fdtParam.sensitivity = 7;
    fdtParam.scaling_timeout_ms = 1000;
    fdtParam.max_face_num = 64;
    fdtParam.av_param.en = 0;
    fdtParam.av_param.delayed_vanish_frm_num = 3;
    fdtParam.av_param.min_frm_num = 3;
    fdtParam.av_param.max_frm_num = 9;

    // parse arguments
    i = 1;
    while (i < argc) {
        if ((strcmp(argv[i], "-c") == 0) && (++i < argc))
            fdtParam.cap_vch = atoi(argv[i]);
        else if ((strcmp(argv[i], "-W") == 0) && (++i < argc))
            fdtParam.source_width = atoi(argv[i]);
        else if ((strcmp(argv[i], "-H") == 0) && (++i < argc))
            fdtParam.source_height = atoi(argv[i]);
        else if ((strcmp(argv[i], "-w") == 0) && (++i < argc))
            fdtParam.first_width = atoi(argv[i]);
        else if ((strcmp(argv[i], "-h") == 0) && (++i < argc))
            fdtParam.first_height = atoi(argv[i]);
        else if ((strcmp(argv[i], "-r") == 0) && (++i < argc))
            fdtParam.ratio = atoi(argv[i]);
        else if ((strcmp(argv[i], "-s") == 0) && (++i < argc))
            fdtParam.sensitivity = atoi(argv[i]);
        else if ((strcmp(argv[i], "-t") == 0) && (++i < argc))
            fdtParam.scaling_timeout_ms = atoi(argv[i]);
        else if ((strcmp(argv[i], "-f") == 0) && (++i < argc))
            fdtParam.max_face_num = atoi(argv[i]);
        else if ((strcmp(argv[i], "-v") == 0) && (++i < argc))
            fdtParam.av_param.en = atoi(argv[i]);
        else if ((strcmp(argv[i], "-d") == 0) && (++i < argc))
            fdtParam.av_param.delayed_vanish_frm_num = atoi(argv[i]);
        else if ((strcmp(argv[i], "-m") == 0) && (++i < argc))
            fdtParam.av_param.min_frm_num = atoi(argv[i]);
        else if ((strcmp(argv[i], "-M") == 0) && (++i < argc))
            fdtParam.av_param.max_frm_num = atoi(argv[i]);
        else if ((strcmp(argv[i], "-u") == 0) && (++i < argc))
            maxFrameCount = atoi(argv[i]);
        else {
            help();
            return -1;
        }
        ++i;
    }
    if (!isValidParam(&fdtParam)) {
        help();
        return -1;
    }

    if ((fd = open(FDT_DEV, O_RDWR)) < 0) {
        printf("Fail to open %s\n", FDT_DEV);
        return -1;
    }

    if ((ret = ioctl(fd, FDT_INIT, &fdtParam)) < 0) {
        printf("Fail to initialize face detection! Err code: %d\n", ret);
        goto exit;
    }

    fdtParam.faces = (unsigned short*)malloc(fdtParam.max_face_num * 8);

    gettimeofday(&t1, NULL);
    while ((frameCount < maxFrameCount) || (maxFrameCount < 0)) {
        if ((ret = ioctl(fd, FDT_DETECT, &fdtParam)) < 0) {
            printf("Error occurs in face detection! Err code: %d\n", ret);
            break;
        }
        if (ret >= 0) {
            pVal = fdtParam.faces;
            for (faceIdx = 0; faceIdx < fdtParam.face_num; ++faceIdx) {
                printf("%d: %d %d %d %d\n", faceIdx, pVal[0], pVal[1], pVal[2], pVal[3]);
                pVal += 4;
            }
            printf("\n");
        }
        ++frameCount;
        if (frameCount % fpsFrameCount == 0) {
            // calculate FPS
            gettimeofday(&t2, NULL);
            td = 1000000 * (t2.tv_sec - t1.tv_sec) + t2.tv_usec - t1.tv_usec;
            fps = (float)fpsFrameCount * 1000000 / td;
            printf("FPS = %.1f\n", fps);
            gettimeofday(&t1, NULL);
        }
    }

    if ((ret = ioctl(fd, FDT_END, &fdtParam)) < 0) {
        printf("Fail to end face detection! Err code: %d\n", ret);
    }

    free(fdtParam.faces);

exit:
    close(fd);

    return ret;
}
