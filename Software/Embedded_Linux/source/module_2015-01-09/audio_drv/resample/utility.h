#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdio.h>

typedef struct{
	FILE * 	handle;
	int		iPos;
	short * psBuf;
}Data_File ;

extern Data_File tTmp, tData0, tData1, tOut0, tOut1;

#define PROFILING
#ifdef PROFILING
 #ifdef WIN32
  #include <time.h>
  extern clock_t tic,toc;
  #define time0(func) func
  #define time1(func) tic = clock();\
              func;\
              toc = clock();\
              fprintf(flog, "%f ms<-%s\n",(double)((tic-toc)*1000)/CLOCKS_PER_SEC,#func)
 #else
  #include <sys/time.h> // for timer
  extern struct timeval tic,toc;
  #define time0(func) func
  #define time1(func) gettimeofday(&tic,NULL);\
      func;\
      gettimeofday(&toc,NULL);\
      printf("%10llu <- %s\n",(long long)(toc.tv_sec-tic.tv_sec)*1000000+(toc.tv_usec-tic.tv_usec),#func)
      //printf("%10llu <- %.10s\n",(long long)(toc.tv_sec-tic.tv_sec)*1000000+(toc.tv_usec-tic.tv_usec),#func)
 #endif
#else
 #define time0(func) func
 #define time1(func) func
#endif

extern void Dump_Data_Float(void * pSrc, int size, int cnt);
extern void Dump_Data_Int(void * pSrc, int size, int cnt);
extern void Log_Data_Float(void * pSrc, int size, int cnt);
extern void Log_Data_Int(void * pSrc, int size, int cnt);
extern int file_write_txt(Data_File * ptFile, const char * pFileName, int size);
extern int file_read_bin(Data_File * ptFile, const char * pFileName, int size);
extern int file_write_bin(Data_File * ptFile, const char * pFileName, int size);
extern void filing_reset(void);
extern void filing_create();
extern void filing_destroy(void);
extern void compare_buf(char * pBuf1, char * pBuf2, int Len1);
extern void compare_file(const char * sSrc, const char * sDst);

#endif

