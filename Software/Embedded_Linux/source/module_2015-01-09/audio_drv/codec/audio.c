#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#if 0  //chri
#include <getopt.h>
#include	<linux/soundcard.h>
#include <sys/resource.h>
#include <linux/config.h>
#endif
#include <math.h>
#include "faad.h"
#include "audio.h"




audio_file *open_audio_file(char *infile, int samplerate, int channels,
                            int outputFormat, int fileType, long channelMask)
{
	//int freq = 44100;
	//int stereo = 1;
	
    audio_file *aufile = malloc(sizeof(audio_file));

    aufile->outputFormat = outputFormat;

    aufile->samplerate = samplerate;
    aufile->channels = channels;
    aufile->total_samples = 0;
    aufile->fileType = fileType;
    aufile->channelMask = channelMask;

    switch (outputFormat)
    {
    case FAAD_FMT_16BIT:
        aufile->bits_per_sample = 16;
        break;
    case FAAD_FMT_24BIT:
        aufile->bits_per_sample = 24;
        break;
    default:
        if (aufile) free(aufile);
        return NULL;
    }

    if(infile[0] == '-')
    {
        aufile->sndfile = stdout;
    } else {
        aufile->sndfile = (FILE *)open(infile, (O_WRONLY | O_CREAT | O_TRUNC), 0660);
    }

    if(infile[0] == '/' && infile[1] == 'd')
    {
      /*   //chri   
    	if (ioctl(aufile->sndfile, SNDCTL_DSP_SPEED, &freq) < 0)
	{
		printf("sampling frequency fail\n\n");	
		return (0);
	}
	if (ioctl(aufile->sndfile, SNDCTL_DSP_STEREO, &stereo) < 0)
	{
		printf("stereo fail\n\n");
		return (0);
	}
      */	 

    }
    else { //chri : already decided
 
	aufile->fileType = OUTPUT_WAV; 
   }



    if (aufile->sndfile == NULL)
    {
        if (aufile) free(aufile);
        return NULL;
    }

    if (aufile->fileType == OUTPUT_WAV)
    {
        if (aufile->channelMask)
            write_wav_extensible_header(aufile, aufile->channelMask);
        else
            write_wav_header(aufile);
    }

    return aufile;
}

#if 0
int write_audio_file(audio_file *aufile, void *sample_buffer, int samples, int offset)
{
    char *buf = (char *)sample_buffer;

    switch (aufile->outputFormat)
    {
    case FAAD_FMT_16BIT:
        return write_audio_16bit(aufile, buf + offset*2, samples);
    case FAAD_FMT_24BIT:
        //return write_audio_24bit(aufile, buf + offset*4, samples);
    default:
        return 0;
    }

    return 0;
}
#else
//Shawn 2004.8.27
int write_audio_file(audio_file *aufile, void *sample_buffer, int samples, int offset)
{
    char *buf = (char *)sample_buffer;

    switch (aufile->outputFormat)
    {
    case FAAD_FMT_16BIT:
		aufile->total_samples += samples;
		return (write(aufile->sndfile, (buf + offset*2), samples*2));
    case FAAD_FMT_24BIT:
        //return write_audio_24bit(aufile, buf + offset*4, samples);
        aufile->total_samples += samples;
		return (write(aufile->sndfile, (buf + offset*4), samples*4));
    default:
        return 0;
    }

    return 0;
}


#endif


void close_audio_file(audio_file *aufile)
{
    if (aufile->fileType == OUTPUT_WAV)
    {
        lseek(aufile->sndfile, 0, SEEK_SET);

        if (aufile->channelMask)
            write_wav_extensible_header(aufile, aufile->channelMask);
        else
            write_wav_header(aufile);
    }

    if (aufile->sndfile)
    close(aufile->sndfile);

    if (aufile) free(aufile);
}

static int write_wav_header(audio_file *aufile)
{
    unsigned char header[44];
    unsigned char* p = header;
    unsigned int bytes = (aufile->bits_per_sample + 7) / 8;
    float data_size = (float)bytes * aufile->total_samples;
    unsigned long word32;

    *p++ = 'R'; *p++ = 'I'; *p++ = 'F'; *p++ = 'F';

    word32 = (data_size + (44 - 8) < (float)MAXWAVESIZE) ?
        (unsigned long)data_size + (44 - 8)  :  (unsigned long)MAXWAVESIZE;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    *p++ = 'W'; *p++ = 'A'; *p++ = 'V'; *p++ = 'E';

    *p++ = 'f'; *p++ = 'm'; *p++ = 't'; *p++ = ' ';

    *p++ = 0x10; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;

    if (aufile->outputFormat == FAAD_FMT_FLOAT)
    {
        *p++ = 0x03; *p++ = 0x00;
    } else {
        *p++ = 0x01; *p++ = 0x00;
    }

    *p++ = (unsigned char)(aufile->channels >> 0);
    *p++ = (unsigned char)(aufile->channels >> 8);

    word32 = (unsigned long)(aufile->samplerate + 0.5);
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    word32 = aufile->samplerate * bytes * aufile->channels;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    word32 = bytes * aufile->channels;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);

    *p++ = (unsigned char)(aufile->bits_per_sample >> 0);
    *p++ = (unsigned char)(aufile->bits_per_sample >> 8);

    *p++ = 'd'; *p++ = 'a'; *p++ = 't'; *p++ = 'a';

    word32 = data_size < MAXWAVESIZE ?
        (unsigned long)data_size : (unsigned long)MAXWAVESIZE;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    return write(aufile->sndfile, header, sizeof(header));
}

static int write_wav_extensible_header(audio_file *aufile, long channelMask)
{
    unsigned char header[68];
    unsigned char* p = header;
    unsigned int bytes = (aufile->bits_per_sample + 7) / 8;
    float data_size = (float)bytes * aufile->total_samples;
    unsigned long word32;

    *p++ = 'R'; *p++ = 'I'; *p++ = 'F'; *p++ = 'F';

    word32 = (data_size + (68 - 8) < (float)MAXWAVESIZE) ?
        (unsigned long)data_size + (68 - 8)  :  (unsigned long)MAXWAVESIZE;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    *p++ = 'W'; *p++ = 'A'; *p++ = 'V'; *p++ = 'E';

    *p++ = 'f'; *p++ = 'm'; *p++ = 't'; *p++ = ' ';

    *p++ = /*0x10*/0x28; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;

    /* WAVE_FORMAT_EXTENSIBLE */
    *p++ = 0xFE; *p++ = 0xFF;

    *p++ = (unsigned char)(aufile->channels >> 0);
    *p++ = (unsigned char)(aufile->channels >> 8);

    word32 = (unsigned long)(aufile->samplerate + 0.5);
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    word32 = aufile->samplerate * bytes * aufile->channels;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    word32 = bytes * aufile->channels;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);

    *p++ = (unsigned char)(aufile->bits_per_sample >> 0);
    *p++ = (unsigned char)(aufile->bits_per_sample >> 8);

    /* cbSize */
    *p++ = (unsigned char)(22);
    *p++ = (unsigned char)(0);

    /* WAVEFORMATEXTENSIBLE */

    /* wValidBitsPerSample */
    *p++ = (unsigned char)(aufile->bits_per_sample >> 0);
    *p++ = (unsigned char)(aufile->bits_per_sample >> 8);

    /* dwChannelMask */
    word32 = channelMask;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    /* SubFormat */
    if (aufile->outputFormat == FAAD_FMT_FLOAT)
    {
        /* KSDATAFORMAT_SUBTYPE_IEEE_FLOAT: 00000003-0000-0010-8000-00aa00389b71 */
        *p++ = 0x03;
        *p++ = 0x00;
        *p++ = 0x00;
        *p++ = 0x00;
        *p++ = 0x00; *p++ = 0x00; *p++ = 0x10; *p++ = 0x00; *p++ = 0x80; *p++ = 0x00;
        *p++ = 0x00; *p++ = 0xaa; *p++ = 0x00; *p++ = 0x38; *p++ = 0x9b; *p++ = 0x71;
    } else {
        /* KSDATAFORMAT_SUBTYPE_PCM: 00000001-0000-0010-8000-00aa00389b71 */
        *p++ = 0x01;
        *p++ = 0x00;
        *p++ = 0x00;
        *p++ = 0x00;
        *p++ = 0x00; *p++ = 0x00; *p++ = 0x10; *p++ = 0x00; *p++ = 0x80; *p++ = 0x00;
        *p++ = 0x00; *p++ = 0xaa; *p++ = 0x00; *p++ = 0x38; *p++ = 0x9b; *p++ = 0x71;
    }

    /* end WAVEFORMATEXTENSIBLE */

    *p++ = 'd'; *p++ = 'a'; *p++ = 't'; *p++ = 'a';

    word32 = data_size < MAXWAVESIZE ?
        (unsigned long)data_size : (unsigned long)MAXWAVESIZE;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    return write(aufile->sndfile, header, sizeof(header));
}

#if 0
static int write_audio_16bit(audio_file *aufile, void *sample_buffer,
                             unsigned int samples)
{
    int ret;
    unsigned int i;
    short *sample_buffer16 = (short*)sample_buffer;
    char *data = malloc(samples*aufile->bits_per_sample*sizeof(char)/8);

    aufile->total_samples += samples;

    if (aufile->channels == 6 && aufile->channelMask)
    {
        for (i = 0; i < samples; i += aufile->channels)
        {
            short r1, r2, r3, r4, r5, r6;
            r1 = sample_buffer16[i];
            r2 = sample_buffer16[i+1];
            r3 = sample_buffer16[i+2];
            r4 = sample_buffer16[i+3];
            r5 = sample_buffer16[i+4];
            r6 = sample_buffer16[i+5];
            sample_buffer16[i] = r2;
            sample_buffer16[i+1] = r3;
            sample_buffer16[i+2] = r1;
            sample_buffer16[i+3] = r6;
            sample_buffer16[i+4] = r4;
            sample_buffer16[i+5] = r5;
        }
    }

    for (i = 0; i < samples; i++)
    {
        data[i*2] = (char)(sample_buffer16[i] & 0xFF);
        data[i*2+1] = (char)((sample_buffer16[i] >> 8) & 0xFF);
    }

    ret = write(aufile->sndfile, data, samples);

    if (data) free(data);

    return ret;
}
#else
//Shawn 2004.8.27
static int write_audio_16bit(audio_file *aufile, void *sample_buffer,
                             unsigned int samples)
{
    int ret;

    aufile->total_samples += samples;

    ret = write(aufile->sndfile, sample_buffer, samples);

    return ret;
}
#endif
/*
static int write_audio_24bit(audio_file *aufile, void *sample_buffer,
                             unsigned int samples)
{
    int ret;
    unsigned int i;
    long *sample_buffer24 = (long*)sample_buffer;
    char *data = malloc(samples*aufile->bits_per_sample*sizeof(char)/8);

    aufile->total_samples += samples;

    if (aufile->channels == 6 && aufile->channelMask)
    {
        for (i = 0; i < samples; i += aufile->channels)
        {
            long r1, r2, r3, r4, r5, r6;
            r1 = sample_buffer24[i];
            r2 = sample_buffer24[i+1];
            r3 = sample_buffer24[i+2];
            r4 = sample_buffer24[i+3];
            r5 = sample_buffer24[i+4];
            r6 = sample_buffer24[i+5];
            sample_buffer24[i] = r2;
            sample_buffer24[i+1] = r3;
            sample_buffer24[i+2] = r1;
            sample_buffer24[i+3] = r6;
            sample_buffer24[i+4] = r4;
            sample_buffer24[i+5] = r5;
        }
    }

    for (i = 0; i < samples; i++)
    {
        data[i*3] = (char)(sample_buffer24[i] & 0xFF);
        data[i*3+1] = (char)((sample_buffer24[i] >> 8) & 0xFF);
        data[i*3+2] = (char)((sample_buffer24[i] >> 16) & 0xFF);
    }

    ret = write(aufile->sndfile, data, samples);

    if (data) free(data);

    return ret;
}
*/

