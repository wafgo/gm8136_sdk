#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#define MAXWAVESIZE     4294967040LU

#define OUTPUT_WAV 1
#define OUTPUT_RAW 2

typedef struct
{
    int outputFormat;
    FILE *sndfile;
    unsigned int fileType;
    unsigned long samplerate;
    unsigned int bits_per_sample;
    unsigned int channels;
    unsigned long total_samples;
    long channelMask;
} audio_file;

audio_file *open_audio_file(char *infile, int samplerate, int channels, int outputFormat, int fileType, long channelMask);
int write_audio_file(audio_file *aufile, void *sample_buffer, int samples, int offset);
void close_audio_file(audio_file *aufile);
static int write_wav_header(audio_file *aufile);
static int write_wav_extensible_header(audio_file *aufile, long channelMask);
static int write_audio_16bit(audio_file *aufile, void *sample_buffer, unsigned int samples);
//static int write_audio_24bit(audio_file *aufile, void *sample_buffer, unsigned int samples);

#endif
