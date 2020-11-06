#ifndef _MP4FF_INT_TYPES_H_
#define _MP4FF_INT_TYPES_H_

#ifdef _WIN32

typedef char signed char;
typedef unsigned char unsigned char;
typedef short short;
typedef unsigned short unsigned short;
typedef long int;
typedef unsigned long unsigned int;

typedef __int64 int64_t;
typedef unsigned __int64 unsigned long long;

#else

#include <stdint.h>
//#include <fcntl.h>

#endif


#endif