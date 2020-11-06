#ifndef __CFFT_H__
#define __CFFT_H__

#if 0
typedef struct
{
    unsigned short n;
    unsigned short ifac[15];
    complex_t *work;
    complex_t *tab;
} cfft_info;
#else
//Shawn 2004.12.7
typedef struct
{
    unsigned short n;
    complex_t *work;
    //complex_t *tab;
} cfft_info;
#endif

void cfftf(cfft_info *cfft, complex_t *c);
void cfftb(cfft_info *cfft, complex_t *c);
cfft_info *cffti(unsigned short n);
void cfftu(cfft_info *cfft);

#endif
