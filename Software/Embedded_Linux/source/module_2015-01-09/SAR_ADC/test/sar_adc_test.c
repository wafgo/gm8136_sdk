#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "../../include/SAR_ADC/sar_adc_api.h"

#define KEY_DEV_NAME    	"/dev/sar_adc_drv"

int main(int argc, char *argv[])
{
	int key_fd = -1,n;
    struct timeval tv;
    fd_set rd;
    unsigned int duration = 100/*ms*/;
    unsigned int xgain = 0;
    unsigned int enable_dir_r = 0;
    sar_adc_pub_data  data;

    for (n = 1; n < argc; n++) {
        if (strncmp (argv[n], "--", 2) == 0) {
            if (strcmp (argv[n] + 2, "duration") == 0 &&
                ++n < argc &&
                sscanf (argv[n], "%d", &duration) == 1) 
                    continue; 
            else if (strcmp (argv[n] + 2, "xgain") == 0 &&
                     ++n < argc &&
                     sscanf (argv[n], "%d", &xgain) == 1) 
                    continue;
            else if (strcmp (argv[n] + 2, "direct_r") == 0){ 
                    enable_dir_r = 1;
                    continue;
            }
        }
        
    }
        
	key_fd = open(KEY_DEV_NAME, O_RDONLY|O_NONBLOCK);
	printf("key_fd = %d\n", key_fd);
	if (key_fd < 0) {
		printf("Can't open %s.\n", KEY_DEV_NAME);
		return -1;
	}
	printf("Start SAR ADC Keypad Test\n");
    
    if (ioctl(key_fd,SAR_ADC_KEY_SET_REPEAT_DURATION,&duration) < 0){
        printf("SAR_ADC_KEY_SET_REPEAT_DURATION fail\n"); 
        goto err;
    }

    if(enable_dir_r){
        if (ioctl(key_fd,SAR_ADC_KEY_SET_XGAIN_NUM,&xgain) < 0){
            printf("SAR_ADC_KEY_SET_XGAIN_NUM fail\n"); 
            goto err;
        }
    }
            
	while (1) {
        tv.tv_sec = 0;
        tv.tv_usec = 500000; /* timeout 500 ms */
        FD_ZERO(&rd);
        FD_SET(key_fd, &rd);

        if(enable_dir_r){
            if (ioctl(key_fd,SAR_ADC_KEY_ADC_DIRECT_READ,&n) < 0){
                printf("SAR_ADC_KEY_ADC_DIRECT_READ fail\n"); 
                goto err;
            }
            printf("AP get xgain =%d val = %x \n", xgain,n);                
        }
        
        select(key_fd+1, &rd, NULL, NULL, &tv);

        if(read(key_fd, &data, sizeof(sar_adc_pub_data))>0){
		    printf("AP get KEY ADC status =%x val = %x \n", data.status,data.adc_val);	
		}
	}
err:
	close(key_fd);
	return 0;
}



