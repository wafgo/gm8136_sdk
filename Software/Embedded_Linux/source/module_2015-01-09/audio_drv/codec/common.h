#ifndef __COMMON_H__
#define __COMMON_H__




#define INLINE __inline


#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

/* COMPILE TIME DEFINITIONS */

/* use double precision */
/* #define USE_DOUBLE_PRECISION */
/* use fixed point reals */
#define FIXED_POINT

/* Allow decoding of LTP profile AAC */
#define LTP_DEC

// Define LC_ONLY_DECODER if you want a pure AAC LC decoder (independant of SBR_DEC)
#define LC_ONLY_DECODER
#ifdef LC_ONLY_DECODER
  #undef LTP_DEC
#endif


/* END COMPILE TIME DEFINITIONS */

#if 0


typedef unsigned __int64 unsigned long long;
typedef unsigned int unsigned int;
typedef unsigned short unsigned short;
typedef unsigned char char;
typedef __int64 int64_t;
typedef int int;
typedef short short;
typedef signed char signed char;
typedef float float;

/*typedef unsigned __int64 unsigned long long;
typedef unsigned __int32 unsigned int;
typedef unsigned __int16 unsigned short;
typedef unsigned __int8 char;
typedef __int64 int64_t;
typedef __int32 int;
typedef __int16 short;
typedef __int8  signed char;
typedef float float;*/

#else

//#include <stdio.h>

/* we need these... */
/*typedef unsigned long long unsigned long long;
typedef unsigned long unsigned int;
typedef unsigned short unsigned short;
typedef unsigned char char;
typedef long long int64_t;
typedef long int;
typedef short short;
typedef char signed char;
typedef float float;*/

#endif



  #include "fixed.h"


#ifndef HAS_LRINTF
/* standard cast */
#define lrintf(f) ((int)(f))
#endif

typedef real_t complex_t[2];
#define RE(A) A[0]
#define IM(A) A[1]


/* common functions */
unsigned char cpu_has_sse(void);
unsigned int random_int(void);
unsigned char get_sr_index(const unsigned int samplerate);
unsigned char max_pred_sfb(const unsigned char sr_index);
unsigned char max_tns_sfb(const unsigned char sr_index, const unsigned char object_type,
                    const unsigned char is_short);
#if 0
unsigned int get_sample_rate(const char sr_index);
#else
//Shawn 2004.10.7
unsigned int get_sample_rate(int sr_index);
#endif
//signed char can_decode_ot(const char object_type);

void *faad_malloc(int size);
void faad_free(void *b);

//#define PROFILE

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2 /* PI/2 */
#define M_PI_2 1.57079632679489661923
#endif


#endif
