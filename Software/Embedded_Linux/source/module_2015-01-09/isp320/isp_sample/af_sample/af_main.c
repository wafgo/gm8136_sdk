#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <sys/time.h>
#include "ioctl_isp320.h"

//============================================================================
// global
//============================================================================
static int isp_fd;
#define MIN(x,y) ((x)<(y) ? (x) : (y))
#define MAX(x,y) ((x)>(y) ? (x) : (y))
static int cal_f_value(af_sta_t *af_sta)
{
        /* 3x3 block array
        |-----------|
        | 0 | 1 | 2 |
        |---|---|---|
        | 3 | 4 | 5 |
        |---|---|---|
        | 6 | 7 | 8 |
        |-----------| */  
    return af_sta->af_result_w[4];
}

static int get_choose_int(void)
{
    int val;
    int error;
    char buf[256];

    error = scanf("%d", &val);
    if (error != 1) {
        printf("Invalid option. Try again.\n");
        clearerr(stdin);
        fgets(buf, sizeof(buf), stdin);
        val = -1;
    }

    return val;
}
static int af_set_enable(void)
{
    int af_en;
    int ret;

    ret = ioctl(isp_fd, AF_IOC_GET_ENABLE, &af_en);
    if (ret < 0)
        return ret;
    printf("AF: %s\n", (af_en) ? "ENABLE" : "DISABLE");

    do {
        printf("AF (0:Disable, 1:Enable) >> ");
        af_en = get_choose_int();
    } while (af_en < 0);

    return ioctl(isp_fd, AF_IOC_SET_ENABLE, &af_en);
}


static int af_set_mode(void)
{
    int af_mode;
    int ret;

    ret = ioctl(isp_fd, AF_IOC_GET_SHOT_MODE, &af_mode);
    if (ret < 0)
        return ret;
    printf("AF mode: %s\n", (af_mode) ? "Continous" : "One Shot");

    do {
        printf("AF mode (0:One Shot, 1:Continous) >> ");
        af_mode = get_choose_int();
    } while (af_mode < 0);

    return ioctl(isp_fd, AF_IOC_SET_SHOT_MODE, &af_mode);
}
static int af_set_method(void)
{
    int af_method;
    int ret;

    ret = ioctl(isp_fd, AF_IOC_GET_AF_METHOD, &af_method);
    if (ret < 0){
        printf("AF_IOC_GET_AF_METHOD fail\n");
        return ret;
    }
    printf("AF method: %d\n", af_method);

    do {
        printf("AF method (0:Step, 1:Poly, 2: test motor, 3: global search >> ");
        af_method = get_choose_int();
    } while (af_method < 0);

    return ioctl(isp_fd, AF_IOC_SET_AF_METHOD, &af_method);
}
static int af_re_trigger(void)
{
    int ret=0;
    int busy=1;
    int timeout=0;
    ret = ioctl(isp_fd, AF_IOC_SET_RE_TRIGGER, NULL);
    if (ret < 0)
        return ret;
    printf("AF Re-Trigger\n");
    printf("Wait for converge ...\n");
    do {
        printf(".");
        ioctl(isp_fd, AF_IOC_CHECK_BUSY, &busy);
        sleep(1);
    } while (busy == 1 && ++timeout < 20);
    if(timeout >= 20)
        printf("\nAF timeout !\n");
    else
        printf("\nAF converged\n");
    return 0;
}
static int af_zoom(void)
{
    int tar_pos; 
    do {
        printf("new zoom x >> ");
        tar_pos = get_choose_int();
    } while (0);

    ioctl(isp_fd, LENS_IOC_SET_ZOOM_MOVE, &tar_pos);
    sleep(1);
		af_re_trigger();    

    return 0;
}
int main(int argc, char *argv[])
{
    int ret = 0;
    focus_range_t focus_range;
    int min_pos, max_pos;
    int curr_pos;
    int peak_fv, peak_pos;
    unsigned int FV;
    af_sta_t af_sta;

    int option;

    // open MCU device
    //TODO: revise device name
    isp_fd = open("/dev/isp320", O_RDWR);
    if (isp_fd < 0) {
        printf("Open ISP fail\n");
        return -1;
    }
    // get focus position range
    ret = ioctl(isp_fd, LENS_IOC_GET_FOCUS_RANGE, &focus_range);
    if (ret < 0){
        printf("AF_GET_FOCUS_RANGE fail!");
        goto end;
    }
    min_pos = focus_range.min;
    max_pos = focus_range.max;
    printf("AF range: %d, %d\n",min_pos, max_pos);
     
#if 0     
    // move focus to min. position
    ret = ioctl(isp_fd, LENS_IOC_SET_FOCUS_INIT, NULL);
    if (ret < 0){
        printf("AF_SET_INIT_POS fail!");
        goto end;
    }

    step_size = 4;
    printf("AF start!\n");
    curr_pos = min_pos;
    peak_fv = 0;
    peak_pos = curr_pos;
    while (1) {
        if((ret = ioctl(isp_fd, ISP_IOC_GET_AF_STA, &af_sta)) < 0){
            printf("ISP_IOC_GET_AF_STA fail!");
            break;
        }
        FV = cal_f_value(&af_sta);
        if(FV >= peak_fv){
            peak_fv = FV;
            peak_pos = curr_pos;
        }    
        if((ret = ioctl(isp_fd, LENS_IOC_GET_FOCUS_POS, &curr_pos)) < 0){
            printf("LENS_IOC_GET_FOCUS_POS fail!");
            break;
        }  
        printf("(pos, FV)= (%d,%d)\n", curr_pos, FV);      
        next_pos = MAX(min_pos, MIN(max_pos, curr_pos + step_size));
        ret = ioctl(isp_fd, LENS_IOC_SET_FOCUS_POS, &next_pos);

        if(ret < 0){
            printf("LENS_IOC_SET_FOCUS_POS fail!");
            break;
        }
        if(next_pos == max_pos){
            ioctl(isp_fd, LENS_IOC_SET_FOCUS_POS, &peak_pos);
            printf("<AF Done> Set Focus pos to %d\n", peak_pos);
            break;    
        }
    }
#else

    while (1) {
#if 1
            printf("----------------------------------------\n");
            printf(" 1. Focus pos\n");
            printf(" 2. Zoom move\n"); 
            printf(" 3. Focus home\n");              
            printf(" 4. Get FV\n");                
            printf(" 5. AF enable\n");                
            printf(" 6. AF re-triger\n");                
            printf(" 7. AF set mode\n");    
            printf(" 8. AF set method\n");            
            printf(" 9. Menu\n");                
            printf(" 0. Quit\n");
            printf("----------------------------------------\n");
#endif
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
#if 1            
        case 1:
            // get focus position range
            ret = ioctl(isp_fd, LENS_IOC_GET_FOCUS_RANGE, &focus_range);
            if (ret < 0){
                printf("AF_GET_FOCUS_RANGE fail!");
                goto end;
            }
            min_pos = focus_range.min;
            max_pos = focus_range.max;
            printf("AF range: %d, %d\n",min_pos, max_pos);

            if((ret = ioctl(isp_fd, LENS_IOC_GET_FOCUS_POS, &curr_pos)) < 0){
                printf("LENS_IOC_GET_FOCUS_POS fail!");
                break;
            }  
            printf("current focus pos = %d\n", curr_pos);

            do {
                printf("new focus pos >> ");
                curr_pos = get_choose_int();
            } while (0);

            ioctl(isp_fd, LENS_IOC_SET_FOCUS_POS, &curr_pos);
            
            break;
#endif            
        case 2:
            af_zoom();
            break;
        case 3:
            ioctl(isp_fd, LENS_IOC_SET_FOCUS_INIT, NULL);
            break;

        case 4:            
            if((ret = ioctl(isp_fd, ISP_IOC_GET_AF_STA, &af_sta)) < 0){
                printf("ISP_IOC_GET_AF_STA fail!");
                break;
            }
            FV = cal_f_value(&af_sta);
            if(FV >= peak_fv){
                peak_fv = FV;
                peak_pos = curr_pos;
            }    
            if((ret = ioctl(isp_fd, LENS_IOC_GET_FOCUS_POS, &curr_pos)) < 0){
                printf("LENS_IOC_GET_FOCUS_POS fail!");
                break;
            }  
            printf("(pos, FV)= (%d,%d)\n", curr_pos, FV);                  
            break;  
        case 5:
            af_set_enable();
            break;
        case 6:
            af_re_trigger();
            break;
        case 7:
            af_set_mode();
            break;  
        case 8:
            af_set_method();
            break;             
        case 9:
            printf("----------------------------------------\n");
#if 1        
            printf(" 1. Focus pos\n");
            printf(" 2. Zoom pos\n");
#endif        
            printf(" 3. Zoom move\n"); 
            printf(" 4. Focus home\n"); 
            printf(" 5. Zoom home\n");                
            printf(" 6. Get FV\n");                
            printf(" 7. Menu\n");                
            printf(" 0. Quit\n");
            printf("----------------------------------------\n");
            break;
        case 0:
            return 0;
        }
    }
#endif    

end:
    close(isp_fd);
    return 0;
}
