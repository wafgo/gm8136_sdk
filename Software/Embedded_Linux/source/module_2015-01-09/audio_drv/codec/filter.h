#ifndef FILTER_DEF
#define FILTER_DEF

typedef struct
{
        short a[3];
        short b[3];
        int x_dly[32][2];
        int y_dly[32][2];
        short quan_gain;
}FILTER;

int init_bandpass_filter(FILTER **bandpass, short MultiChan);
void bandpass_filter(FILTER *filter, short input[][2048], short output[][2048], unsigned short sample_number, short MultiChan );
void filter_destory(FILTER *filter);
#endif
