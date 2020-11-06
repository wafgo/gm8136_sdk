#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ioctl_pwm.h"

#define PWM_DEV_NAME "/dev/ftpwmtmr010"
static int is_set = 0;
//static int auto_flag = 1;

void auto_test(int freq, int *_fd, int *ret)
{
#define REPEAT_CNT 127
#define INTERRUPT_CNT 1
	pwm_info_t pwm;
	int fd = *_fd;
	//int i,j,k,l,m,n,o,p;
	int i;

	memset(&pwm, 0, sizeof(pwm));
	pwm.freq = freq;
	pwm.repeat_cnt = REPEAT_CNT;
	pwm.intr_cnt = INTERRUPT_CNT;
	pwm.clksrc = 1;
	pwm.mode = PWM_INTERVAL;
	for (i=0;i<8;i++) {
		if (is_set == 1)
			break;
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_REQUEST, &pwm.id);
		*ret = ioctl(fd, PWM_IOCTL_SET_CLKSRC, &pwm);
		*ret = ioctl(fd, PWM_IOCTL_SET_FREQ, &pwm);
		*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
		*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
	}
	is_set = 1;
	//*ret = ioctl(fd, PWM_IOCTL_ALL_START);
	//printf("Start Auto Test!\n");
#if 0
	//printf("Random Select\n");
	printf("\n\n");
	srand(time(NULL));
    int a;
	a=(rand() % 8)+1;
	pwm.intr_cnt = INTERRUPT_CNT;
	printf("a=%d\n",a);
	if (a%2==0) {
		for (i=0;i<a;i++) {
			pwm.id = i;
			usleep(10*1000);
			*ret = ioctl(fd, PWM_IOCTL_REQUEST, &pwm.id);
			*ret = ioctl(fd, PWM_IOCTL_SET_CLKSRC, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_SET_FREQ, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
			*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
		}
		*ret = ioctl(fd, PWM_IOCTL_ALL_START);
		usleep(300*1000);
		for (i=0;i<a;i++) {
			pwm.id = i;
			*ret = ioctl(fd, PWM_IOCTL_GET_INFO, &pwm);
			if (pwm.intr_cnt > 0) {
				printf("Not finished!!!!PWM%d's interrupt is %d\n", pwm.id, pwm.intr_cnt);
				auto_flag = 0;
			}
			pwm.id = i;
			*ret = ioctl(fd, PWM_IOCTL_RELEASE, &pwm.id);
		}
	} else {
		for (i=a;i>0;i--) {
			pwm.id = i;
			usleep(10*1000);
			*ret = ioctl(fd, PWM_IOCTL_REQUEST, &pwm.id);
			*ret = ioctl(fd, PWM_IOCTL_SET_CLKSRC, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_SET_FREQ, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
			*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
		}
		*ret = ioctl(fd, PWM_IOCTL_ALL_START);
		usleep(300*1000);
		for (i=a;i>0;i--) {
			pwm.id = i;
			*ret = ioctl(fd, PWM_IOCTL_GET_INFO, &pwm);
			if (pwm.intr_cnt > 0) {
				printf("Not finished!!!!PWM%d's interrupt is %d\n", pwm.id, pwm.intr_cnt);
				auto_flag = 0;
			}
			pwm.id = i;
			*ret = ioctl(fd, PWM_IOCTL_RELEASE, &pwm.id);
		}
	}
	*ret = ioctl(fd, PWM_IOCTL_ALL_STOP);
	usleep(300*1000);
#endif
#if 0
	printf("1. interval mode\n");
	usleep(100*1000);
	pwm.mode = PWM_INTERVAL;
	pwm.id = 0;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
	pwm.id = 1;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
	pwm.id = 2;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
	pwm.id = 3;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
	*ret = ioctl(fd, PWM_IOCTL_ALL_START);
	sleep(3);
	*ret = ioctl(fd, PWM_IOCTL_ALL_STOP);
	printf("2. oneshot mode(1 times interrupts)\n");
	usleep(100*1000);
	pwm.intr_cnt = 0;
	pwm.mode = PWM_ONESHOT;
	pwm.id = 0;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 1;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 2;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 3;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	*ret = ioctl(fd, PWM_IOCTL_ALL_UPDATE);
	*ret = ioctl(fd, PWM_IOCTL_ALL_START);
	sleep(1);
	*ret = ioctl(fd, PWM_IOCTL_ALL_STOP);
#endif
#if 0
	printf("3. repeat mode: count = %d\n", REPEAT_CNT);
	usleep(100*1000);
	pwm.mode = PWM_REPEAT;
	pwm.repeat_cnt = REPEAT_CNT;
	pwm.intr_cnt = 1000;
	printf("First Run: 1 pwm work\n");
	for (i=0;i<8;i++) {
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
		*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
		*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
		*ret = ioctl(fd, PWM_IOCTL_START, &pwm.id);
		usleep(300*1000);
		*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
	}
	*ret = ioctl(fd, PWM_IOCTL_ALL_STOP);
	printf("Second Run: 2 pwm work\n");
	for (i=0;i<8;i++) {
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
		*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
		*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
		for (j=0;j<8;j++) {
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
			*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
			*ret = ioctl(fd, PWM_IOCTL_ALL_START);
			usleep(300*1000);
			*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
		}
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
	}
	printf("Third Run: 3 pwm work\n");
	for (i=0;i<8;i++) {
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
		*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
		*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
		for (j=0;j<8;j++) {
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
			*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
			for (k=0;k<8;k++) {
				pwm.id = k;
				*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
				*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
				*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
				*ret = ioctl(fd, PWM_IOCTL_ALL_START);
				usleep(300*1000);
				*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
			}
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
		}
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
	}
	printf("Fourth Run: 4 pwm work\n");
	for (i=0;i<8;i++) {
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
		*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
		*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
		for (j=0;j<8;j++) {
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
			*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
			for (k=0;k<8;k++) {
				pwm.id = k;
				*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
				*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
				*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
				for (l=0;l<8;l++) {
					pwm.id = l;
					*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
					*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
					*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
					*ret = ioctl(fd, PWM_IOCTL_ALL_START);
					usleep(300*1000);
					*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
				}
				pwm.id = k;
				*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
			}
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
		}
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
	}
	printf("Fifth Run: 5 pwm work\n");
	for (i=0;i<8;i++) {
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
		*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
		*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
		for (j=0;j<8;j++) {
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
			*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
			for (k=0;k<8;k++) {
				pwm.id = k;
				*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
				*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
				*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
				for (l=0;l<8;l++) {
					pwm.id = l;
					*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
					*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
					*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
					for (m=0;m<8;m++) {
						pwm.id = m;
						*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
						*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
						*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
						*ret = ioctl(fd, PWM_IOCTL_ALL_START);
						usleep(300*1000);
						*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
					}
					pwm.id = l;
					*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
				}
				pwm.id = k;
				*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
			}
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
		}
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
	}
	printf("Sixth Run: 6 pwm work\n");
	for (i=0;i<8;i++) {
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
		*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
		*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
		for (j=0;j<8;j++) {
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
			*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
			for (k=0;k<8;k++) {
				pwm.id = k;
				*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
				*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
				*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
				for (l=0;l<8;l++) {
					pwm.id = l;
					*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
					*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
					*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
					for (m=0;m<8;m++) {
						pwm.id = m;
						*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
						*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
						*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
						for (n=0;n<8;n++) {
							pwm.id = n;
							*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
							*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
							*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
							*ret = ioctl(fd, PWM_IOCTL_ALL_START);
							usleep(300*1000);
							*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
						}
						pwm.id = m;
						*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
					}
					pwm.id = l;
					*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
				}
				pwm.id = k;
				*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
			}
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
		}
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
	}
	printf("Seventh Run: 7 pwm work\n");
	for (i=0;i<8;i++) {
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
		*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
		*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
		for (j=0;j<8;j++) {
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
			*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
			for (k=0;k<8;k++) {
				pwm.id = k;
				*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
				*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
				*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
				for (l=0;l<8;l++) {
					pwm.id = l;
					*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
					*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
					*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
					for (m=0;m<8;m++) {
						pwm.id = m;
						*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
						*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
						*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
						for (n=0;n<8;n++) {
							pwm.id = n;
							*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
							*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
							*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
							for (o=0;o<8;o++) {
								pwm.id = o;
								*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
								*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
								*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
								*ret = ioctl(fd, PWM_IOCTL_ALL_START);
								usleep(300*1000);
								*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
							}
							pwm.id = n;
							*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
						}
						pwm.id = m;
						*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
					}
					pwm.id = l;
					*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
				}
				pwm.id = k;
				*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
			}
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
		}
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
	}
	printf("Eighth Run: 8 pwm work\n");
	for (i=0;i<8;i++) {
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
		*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
		*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
		for (j=0;j<8;j++) {
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
			*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
			*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
			for (k=0;k<8;k++) {
				pwm.id = k;
				*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
				*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
				*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
				for (l=0;l<8;l++) {
					pwm.id = l;
					*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
					*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
					*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
					for (m=0;m<8;m++) {
						pwm.id = m;
						*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
						*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
						*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
						for (n=0;n<8;n++) {
							pwm.id = n;
							*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
							*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
							*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
							for (o=0;o<8;o++) {
								pwm.id = o;
								*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
								*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
								*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
								for (p=0;p<8;p++) {
									pwm.id = p;
									*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
									*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
									*ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
									*ret = ioctl(fd, PWM_IOCTL_ALL_START);
									usleep(300*1000);
									*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
								}
								pwm.id = o;
								*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
							}
							pwm.id = n;
							*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
						}
						pwm.id = m;
						*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
					}
					pwm.id = l;
					*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
				}
				pwm.id = k;
				*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
			}
			pwm.id = j;
			*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
		}
		pwm.id = i;
		*ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
	}
	*ret = ioctl(fd, PWM_IOCTL_ALL_STOP);
	usleep(500*1000);
#endif
#if 1
/*
	printf("4. pattern mode(len = 32)\n");
	usleep(100*1000);
	pwm.mode = PWM_PATTERN;
	pwm.repeat_cnt = REPEAT_CNT;
	pwm.pattern[0] = 0x96a596a5;
	pwm.pattern_len = (1<<5)-1;
	pwm.id = 0;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 1;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 2;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 3;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	*ret = ioctl(fd, PWM_IOCTL_ALL_UPDATE);
	*ret = ioctl(fd, PWM_IOCTL_ALL_START);
	usleep(100*1000);
	*ret = ioctl(fd, PWM_IOCTL_ALL_STOP);
	printf("5. pattern mode(len = 64)\n");
	usleep(100*1000);
	pwm.mode = PWM_PATTERN;
	pwm.repeat_cnt = REPEAT_CNT;
	pwm.pattern[0] = 0x96a596a5;
	pwm.pattern[1] = 0x9966aa55;
	pwm.pattern_len = (1<<6)-1;
	pwm.id = 0;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 1;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 2;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 3;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	*ret = ioctl(fd, PWM_IOCTL_ALL_UPDATE);
	*ret = ioctl(fd, PWM_IOCTL_ALL_START);
	usleep(100*1000);
	*ret = ioctl(fd, PWM_IOCTL_ALL_STOP);
	printf("6. pattern mode(len = 96)\n");
	usleep(100*1000);
	pwm.mode = PWM_PATTERN;
	pwm.repeat_cnt = REPEAT_CNT;
	pwm.pattern[0] = 0x96a596a5;
	pwm.pattern[1] = 0x9966aa55;
	pwm.pattern[2] = 0x66aa5599;
	pwm.pattern_len = (1<<7)-1;
	pwm.id = 0;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 1;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 2;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 3;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	*ret = ioctl(fd, PWM_IOCTL_ALL_UPDATE);
	*ret = ioctl(fd, PWM_IOCTL_ALL_START);
	usleep(100*1000);
	*ret = ioctl(fd, PWM_IOCTL_ALL_STOP);
*/
	printf("7. pattern mode(len = 128)\n");
	usleep(100*1000);
	pwm.mode = PWM_PATTERN;
	pwm.repeat_cnt = REPEAT_CNT;
	pwm.pattern[0] = 0xffffffff;
	pwm.pattern[1] = 0x00000000;
	pwm.pattern[2] = 0xffffffff;
	pwm.pattern[3] = 0x00000000;
	pwm.pattern_len = (1<<8)-1;
	pwm.id = 0;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 1;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 2;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	pwm.id = 3;*ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);*ret = ioctl(fd, PWM_IOCTL_ENABLE_INTERRUPT, &pwm.id);
	*ret = ioctl(fd, PWM_IOCTL_ALL_UPDATE);
	*ret = ioctl(fd, PWM_IOCTL_ALL_START);
	usleep(100*1000);
	*ret = ioctl(fd, PWM_IOCTL_ALL_STOP);
#endif
}

int main(int argc, char *argv[])
{
	pwm_info_t pwm;
	int fd = -1;
	int ret = 0, num = 0;
	memset(&pwm, 0, sizeof(pwm));

	fd = open(PWM_DEV_NAME, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		perror("Can't open /dev/ftpwmtmr010\n");
		return -1;
	}

	while (num < 11) {

		printf("======================================\n");
		printf("1. Request PWM\n");
		printf("2. Start PWM\n");
		printf("3. Stop PWM\n");
		printf("4. Get PWM info\n");
		printf("5. Set PWM Clock Source\n");
		printf("6. Set PWM Frequency\n");
		printf("7. Set PWM Step\n");
		printf("8. Set PWM Duty\n");
		printf("9. Set PWM  mode\n");
		printf("10. Auto Test\n");
		printf("11. Exit\n");
		printf("======================================\n");
		printf("Enter Your Choice:");
		scanf("%d", &ret);

		switch (ret) {
			case 1:
				printf("Enter PWM Num:");
				scanf("%d", &ret);
				pwm.id = ret;
				ret = ioctl(fd, PWM_IOCTL_REQUEST, &pwm.id);
				break;
			case 2:
				printf("Enter PWM Num:");
				scanf("%d", &ret);
				pwm.id = ret;
				ret = ioctl(fd, PWM_IOCTL_UPDATE, &pwm.id);
				ret = ioctl(fd, PWM_IOCTL_START, &pwm.id);
				break;
			case 3:
				printf("Enter PWM Num:");
				scanf("%d", &ret);
				pwm.id = ret;
				ret = ioctl(fd, PWM_IOCTL_STOP, &pwm.id);
				break;
			case 4:
				printf("Enter PWM Num:");
				scanf("%d", &ret);
				pwm.id = ret;
				ret = ioctl(fd, PWM_IOCTL_GET_INFO, &pwm);
				printf("*************************\n");
				printf("PWM %d information:\n", pwm.id);
				printf("Clock Source: %d\n", pwm.clksrc);
				printf("Frequency: %d\n", pwm.freq);
				printf("Step: %d\n", pwm.duty_steps);
				printf("Duty: %d\n", pwm.duty_ratio);
				printf("*************************\n");
				break;
			case 5:
				printf("Enter PWM Num:");
				scanf("%d", &ret);
				pwm.id = ret;
				printf("Enter PWM %d ClkSrc:", pwm.id);
				scanf("%d", &ret);
				pwm.clksrc = ret;
				ret = ioctl(fd, PWM_IOCTL_SET_CLKSRC, &pwm);
				break;
			case 6:
				printf("Enter PWM Num:");
				scanf("%d", &ret);
				pwm.id = ret;
				printf("Enter PWM %d Freq:", pwm.id);
				scanf("%d", &ret);
				pwm.freq = ret;
				ret = ioctl(fd, PWM_IOCTL_SET_FREQ, &pwm);
				break;
			case 7:
				printf("Enter PWM Num:");
				scanf("%d", &ret);
				pwm.id = ret;
				printf("Enter PWM %d Step:", pwm.id);
				scanf("%d", &ret);
				pwm.duty_steps = ret;
				ret = ioctl(fd, PWM_IOCTL_SET_DUTY_STEPS, &pwm);
				break;
			case 8:
				printf("Enter PWM Num:");
				scanf("%d", &ret);
				pwm.id = ret;
				printf("Enter PWM %d Duty:", pwm.id);
				scanf("%d", &ret);
				pwm.duty_ratio = ret;
				ret = ioctl(fd, PWM_IOCTL_SET_DUTY_RATIO, &pwm);
				break;
			case 9:
				printf("Enter PWM Num:");
				scanf("%d", &ret);
				pwm.id = ret;
				printf("Enter PWM count:");
				scanf("%d", &ret);
				pwm.intr_cnt = ret;
				printf("Enter PWM mode:");
				scanf("%d", &ret);
				pwm.mode = ret;
				ret = ioctl(fd, PWM_IOCTL_SET_MODE, &pwm);
				break;
			case 10:
//				auto_test(100, &fd, &ret);
				auto_test(1000*1000, &fd, &ret);
//				while (auto_flag) auto_test(1000*1000, &fd, &ret);
				break;
			case 11:
				ret = ioctl(fd, PWM_IOCTL_ALL_STOP);
				printf("Exit PWM TEST\n");
				num = 11;
				break;
			default:
				printf("No Command!\n");
				ret = 0;
				break;
		}
	}

	if (ret < 0) {
		perror("IOCTL err!\n");
		return -1;
	}
	close(fd);
	return 0;
}
