/* **************************************************************************
 * 
 * This program provides the function to record the audio from TW2835 to a file. 
 * And the audio is played through cat6611. 
 * When AUDIO_RECORD is defined, it will record the audio. 
 * When AUDIO_RECORD is no defined, it will play the audio through cat6611
 *
 * **************************************************************************
 */


#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <linux/soundcard.h>

#define PLAY_DEVICE "/dev/dsp2"
#define IOCTL(a,b,c) ioctl(a,b,&c)
char *play_file = "music_48KHz.pcm";

FILE *din;

#define NUM_OF_AUDIOIN_CHANNEL 2    //tw2835
#define SOURCE_MONO

int paudio, abuf_size;
u_char *audiobuf;

int main (int argc, char **argv)
{
    char *PAUDIO=NULL;
    int ret;
    int samplesize = 16;
    int dsp_speed = 48000;
    int dsp_stereo = 1;
        
#ifdef SOURCE_MONO
    dsp_stereo = 0;
#endif

    /* Initialize and set thread detached attribute */    
    audiobuf = NULL;
    paudio = -1;
       
    /* clear screen and print the message */
    fprintf(stdout, "\033[2J");
    fprintf(stdout, "\033[1;1H");
        
    PAUDIO = PLAY_DEVICE;
             
    paudio = open(PAUDIO, O_WRONLY, 0);
    if (paudio == -1) {
        perror(PAUDIO);
        exit (-1);
    }
    
    IOCTL(paudio, SNDCTL_DSP_SAMPLESIZE, samplesize);
    IOCTL(paudio, SNDCTL_DSP_SPEED, dsp_speed);            
    IOCTL(paudio, SNDCTL_DSP_STEREO, dsp_stereo);
    IOCTL(paudio, SNDCTL_DSP_GETBLKSIZE, abuf_size);
    
    if (audiobuf == NULL)
        audiobuf = (u_char *)malloc(abuf_size);    
    
    din = fopen(play_file, "r");
    if (din == NULL) {
        printf("Can't open file %s \n", play_file);
        exit (-1);
    }
        
    while (1) {
        ret = fread((unsigned char*)audiobuf, abuf_size, 1, din);
        if (!ret)
            break;
            
        write(paudio, (char *)audiobuf, abuf_size);
    }
    
    fclose(din);
    free(audiobuf);
    
    printf("file close. \n");
    
	return 0;
}
