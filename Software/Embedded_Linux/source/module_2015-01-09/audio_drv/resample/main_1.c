#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resample.h"
#include "utility.h"

//====== Extern ======
#define PROFILING
#ifdef PROFILING
 #ifdef WIN32
  #include <time.h>
  clock_t tic,toc;
 #else
  #include <sys/time.h> // for timer
  struct timeval tic,toc;
 #endif
#endif

int MAX_BUF_SIZEW = 4096;

//==============================================================
//  Main()
//==============================================================
int main (int argc, char **argv)
{
    int Ret, inCnt, outCnt0;
    const char * pVersion=NULL;
    int FrameCnt=0;
    double TSum0;

#ifdef PROFILING
  #ifdef WIN32
    FILE * flog;
    fopen_s(&flog,"profiling.txt", "w");
  #endif
#endif

    inCnt   = 160;    // data in size, mono only
    outCnt0 =  80;    // data out size when down-sampling

    if ((inCnt > MAX_BUF_SIZEW) || (outCnt0 >= MAX_BUF_SIZEW))
        return 1;

    filing_create(MAX_BUF_SIZEW);

    GM_Resampling_ver(&pVersion);
    printf("\n ---=== %16s ===---\n", pVersion);

    // Init Resampling library
    if ((Ret = GM_Resampling_Init(inCnt, outCnt0))!=0)
        return Ret;

    TSum0 = 0;
    FrameCnt = 1;
    while (1)
    {
        // Read data from file
        //if((Ret=file_read_bin(&tData0, "D:\\Share\\Resampling\\src\\1_In0.pcm", inCnt))==1) break;
        if((Ret=file_read_bin(&tData0, "Stream_16k.pcm", inCnt))==1) break;
        //if((Ret=file_read_bin(&tData0, "3_In0.pcm", inCnt))==1) break;

      gettimeofday(&tic,NULL);
        // Resampling main process
        GM_Resampling((void *)(tData0.psBuf), (void *)(tOut0.psBuf));
      gettimeofday(&toc,NULL);
      TSum0 += ((double)(toc.tv_sec-tic.tv_sec)*1000000+(toc.tv_usec-tic.tv_usec));

        // Write data to file
        //if((Ret=file_write_bin(&tOut, "D:\\Share\\Resampling\\src\\1_Out.pcm", outCnt))==1) break;
        if((Ret=file_write_bin(&tOut0, "Stream_8k.pcm", outCnt0))==1) break;

        FrameCnt++;
    }

    printf("\n %d->%d:Avg %.2f samples/sec\n", inCnt, outCnt0, (outCnt0*1000000) / (TSum0/FrameCnt));

    // Destroy Resampling library
    GM_Resampling_Destroy();
    
    filing_destroy();

#ifdef PROFILING
 #ifdef WIN32
  fclose(flog);
 #endif
#endif

    return Ret;
}


