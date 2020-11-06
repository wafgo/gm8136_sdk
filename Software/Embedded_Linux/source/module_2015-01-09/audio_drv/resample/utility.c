#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "utility.h"


//====== Structure ======
Data_File tTmp, tData0, tData1, tOut0, tOut1;

FILE * hFile;

// ==========================================
//  Dump_Data_Float()
//
//      parameter:
//          pSrc : pointer to Float or Double buffer
//          size : 4(float) or 8(double)
//          cnt : number of floats(or doubles)
// ==========================================
void Dump_Data_Float(void * pSrc, int size, int cnt)
{
    int i;
    
    switch (size)
    {
        case 4:
        {
            float * ptr1=(float *)pSrc;            
            printf("\n");
            for (i=0;i<cnt;i++)
            {
                printf("%f ",*ptr1);
                ptr1++;
            }
            break;
        }
        case 8:
        {
            double * ptr2=(double *)pSrc;            
            printf("\n");
            for (i=0;i<cnt;i++)
            {
                printf("%f ",*ptr2);
                ptr2++;
            }
            break;
        }
        default:
            printf("\n Size not defined in Dump_Data_Float()");
            break;
    }
    fflush(stdout);
    
    return;
}

// ==========================================
//  Dump_Data_Int()
//
//      parameter:
//          pSrc : pointer to Float or Double buffer
//          size : 1(char),2(short),4(int)
//          cnt : number of chars(or short, or integer)
// ==========================================
void Dump_Data_Int(void * pSrc, int size, int cnt)
{
    int i;

    switch (size)
    {
        case 1:
        {
            char * ptr1=(char *)pSrc;            
            printf("\n");
            for (i=0;i<cnt;i++)
            {
                printf("%4x ",*ptr1);
                ptr1++;
            }
            break;
        }
        case 2:
        {
            short * ptr2=(short *)pSrc;            
            printf("\n");
            for (i=0;i<cnt;i++)
            {
                printf("%6d ",*ptr2);
                ptr2++;
            }
            break;
        }
        case 4:
        {
            int * ptr4=(int *)pSrc;            
            printf("\n");
            for (i=0;i<cnt;i++)
            {
                printf("%11d ",*ptr4);
                ptr4++;
            }
            break;
        }
        default:
            printf("\n Size not defined Dump_Data_Int()");
            break;
    }

    fflush(stdout);    
    
    return;
}

// ==========================================
//  Log_Data_Float() : save data to 'log.txt' at current folder
//
//      parameter:
//          pSrc : pointer to Float or Double buffer
//          size : 4(float),8(double)
//          cnt : number of floats(or doubles)
// ==========================================
void Log_Data_Float(void * pSrc, int size, int cnt)
{
    int i;
#ifdef WIN32
    errno_t err;
#endif

    if (hFile == NULL)
    {
#ifdef WIN32
        if((err = fopen_s(&hFile,"log.txt", "a"))!=0)
#else
    	if((hFile = fopen("log.txt", "a"))==NULL)
#endif
	    {
		    printf("\n Cannot open File 'log.txt'!");
	    }
	}

    if (hFile != NULL)
    {
        switch (size)
        {
            case 4:
            {
                float * ptr1=(float *)pSrc;            
                for (i=0;i<cnt;i++)
                {
                    fprintf(hFile,"%f ",*ptr1);
                    ptr1++;
                }
                fprintf(hFile,"\n");
                break;
            }
            case 8:
            {
                double * ptr2=(double *)pSrc;            
                for (i=0;i<cnt;i++)
                {
                    fprintf(hFile,"%f ",*ptr2);
                    ptr2++;
                }
                fprintf(hFile,"\n");
                break;
            }
            default:
                printf("\n Size not defined in Dump_Data_Float()");
                break;
        }
    }

    if (hFile != NULL)
    {
        fclose(hFile);
        hFile = NULL;
    }
    
    return;
}

// ==========================================
//  Log_Data_Int() : save data to 'log.txt' at current folder
//
//      parameter:
//          pSrc : pointer to Float or Double buffer
//          size : 1(char),2(short),4(int)
//          cnt : number of chars(or short, or integer)
// ==========================================
void Log_Data_Int(void * pSrc, int size, int cnt)
{
    int i;
#ifdef WIN32
    errno_t err;
#endif

    if (hFile == NULL)
    {
#ifdef WIN32
        if((err = fopen_s(&hFile,"log.txt", "a"))!=0)
#else
    	if((hFile = fopen("log.txt", "a"))==NULL)
#endif
	    {
		    printf("\n Cannot open File 'log.txt'!");
	    }
	}

    if (hFile != NULL)
    {
        switch (size)
        {
            case 1:
            {
                char * ptr1=(char *)pSrc;            
                fprintf(hFile, "\n");
                for (i=0;i<cnt;i++)
                {
                    fprintf(hFile,"%4x ",*ptr1);
                    ptr1++;
                }
                break;
            }
            case 2:
            {
                short * ptr2=(short *)pSrc;            

                for (i=0;i<cnt;i++)
                {
                    fprintf(hFile, "%6d ",*ptr2);
                    ptr2++;
                }
                fprintf(hFile,"\n");
                break;
            }
            case 4:
            {
                int * ptr4=(int *)pSrc;            

                for (i=0;i<cnt;i++)
                {
                    fprintf(hFile,"%11d ",*ptr4);
                    ptr4++;
                }
                fprintf(hFile,"\n");
                break;
            }
            default:
                printf("\n Size not defined Dump_Data_Int()");
                break;
        }
    }

    if (hFile != NULL)
    {
        fclose(hFile);
        hFile = NULL;
    }
    
    return;
}

//==============================================================
//  file_write_txt()
//==============================================================
int file_write_txt(Data_File * ptFile, const char * pFileName, int size)
{
	int count, Ret = 0;
#ifdef WIN32
    errno_t err;
#endif

	if (ptFile->handle == NULL)
	{
#ifdef WIN32
        if((err = fopen_s(&(ptFile->handle),pFileName, "wb"))!=0)
#else
		if((ptFile->handle = fopen(pFileName, "wb"))==NULL)
#endif
		{
			printf("\n File %s was not opened !", pFileName);
			Ret = 1;
		}else
		{
			ptFile->iPos = 0;
		}
	}

	if (ptFile->handle != NULL)
	{
		count = (int)fwrite((void *)(ptFile->psBuf), sizeof(short), size, ptFile->handle);
    	ptFile->iPos += count;
	}
	return Ret;
}

//==============================================================
//  file_read_bin()
//==============================================================
int file_read_bin(Data_File * ptFile, const char * pFileName, int size)
{
	int count, Ret=0;
#ifdef WIN32
    errno_t err;
#endif

	if (ptFile->handle == NULL)
	{
#ifdef WIN32
        if((err = fopen_s(&(ptFile->handle),pFileName, "rb"))!=0)
#else
		if((ptFile->handle = fopen(pFileName, "rb"))==NULL)
#endif
		{
			printf("\n File %s was not opened !", pFileName);
			Ret = 1;
		}
		else
		{
			ptFile->iPos = 0;
		}
	}

	if (ptFile->handle != NULL)
	{
//printf("\n %d->%ld", (int)(ptFile->handle), ftell(ptFile->handle));
//printf("\n");
		count = (int)fread((void *)(ptFile->psBuf), sizeof(short), (size_t)size, ptFile->handle);
//printf("\n %d=>%ld", (int)(ptFile->handle), ftell(ptFile->handle));
//printf("\n");
        if (count != size)
        {
            //fseek(ptFile->handle, 0, SEEK_SET);   // rewind
            fclose(ptFile->handle);
			ptFile->handle = 0;
            Ret = 1;
        }
		else
	    {
	    	ptFile->iPos += count;
		}
	}
	return Ret;
}


//==============================================================
//  file_write_bin()
//==============================================================
int file_write_bin(Data_File * ptFile, const char * pFileName, int size)
{
	int count, Ret = 0;
#ifdef WIN32
    errno_t err;
#endif

	if (ptFile->handle == NULL)
	{
#ifdef WIN32
        if((err = fopen_s(&(ptFile->handle),pFileName, "wb"))!=0)
#else
		if((ptFile->handle = fopen(pFileName, "wb"))==NULL)
#endif
		{
			printf("\n File %s was not opened !", pFileName);
			Ret = 1;
		}else
		{
			ptFile->iPos = 0;
		}
	}

	if (ptFile->handle != NULL)
	{
		count = (int)fwrite((void *)(ptFile->psBuf), sizeof(short), size, ptFile->handle);
    	ptFile->iPos += count;
	}
	return Ret;
}

// ==============================================================
//  filing_reset()
// ==============================================================
void filing_reset(void)
{
	if (tData0.handle)
    {
    	fclose(tData0.handle);
		tData0.handle = NULL;
    }
	if (tData1.handle)
    {
    	fclose(tData1.handle);
		tData1.handle = NULL;
    }
	if (tOut0.handle)
    {
    	fclose(tOut0.handle);
		tOut0.handle = NULL;
    }
    if (tOut1.handle)
    {
    	fclose(tOut1.handle);
		tOut1.handle = NULL;
    }
    if (tTmp.handle)
    {
    	fclose(tTmp.handle);
		tTmp.handle = NULL;
    }
}

// ==============================================================
//  filing_create()
// ==============================================================
void filing_create(int size_word)
{
    filing_reset();

    tData0.psBuf = (short *)calloc(size_word, sizeof(short));
    tData1.psBuf = (short *)calloc(size_word, sizeof(short));
    tOut0.psBuf   = (short *)calloc(size_word, sizeof(short));
    tOut1.psBuf   = (short *)calloc(size_word, sizeof(short));
    tTmp.psBuf   = (short *)calloc(size_word, sizeof(short));

    return;
}

// ==============================================================
//  filing_destroy()
// ==============================================================
void filing_destroy(void)
{
    free(tData0.psBuf);
    tData0.psBuf  = NULL;
    
    free(tData1.psBuf);
    tData1.psBuf  = NULL;

    free(tOut0.psBuf);
    tOut0.psBuf  = NULL;

    free(tOut1.psBuf);
    tOut1.psBuf  = NULL;

    free(tTmp.psBuf);
    tTmp.psBuf  = NULL;

    filing_reset();

    return ;
}

//==============================================================
//  compare_buf()
//==============================================================
void compare_buf(char * pBuf1, char * pBuf2, int Len1)
{
	char *ptr1, *ptr2;

	ptr1 = pBuf1;
	ptr2 = pBuf2;

	while(Len1 >0)
	{
		if (*ptr1++ != *ptr2++)
		{	printf("\n Byte[%d] is not the same!", -Len1);
			break;
		}
		Len1--;
	}
	if (Len1==0)
		printf("->Verified");

	printf("\n");

	return;
}

//==============================================================
//  compare_file()
//==============================================================
void compare_file(const char * sSrc, const char * sDst)
{
	FILE * pFile1=NULL, * pFile2=NULL;
	int Len1, Len2;
	char * pBuf1, *pBuf2, *ptr1, *ptr2;
#ifdef WIN32
    errno_t err;
#endif

#ifdef WIN32
    if((err = fopen_s(&pFile1,sSrc, "rb"))!=0)
#else
	if((pFile1 = fopen(sSrc, "rb"))==NULL)
#endif
		printf("\n File %s cannot be opened !", sSrc);

#ifdef WIN32
    if((err = fopen_s(&pFile2,sDst, "rb"))!=0)
#else
	if((pFile2 = fopen(sDst, "rb"))==NULL)
#endif
		printf("\n File %s cannot be opened !", sDst);

	if (fseek(pFile1, 0, SEEK_END)!=0)		printf("\n File %s seek error !", sSrc);
	if (fseek(pFile2, 0, SEEK_END)!=0)		printf("\n File %s seek error !", sSrc);

	Len1 = (int)ftell(pFile1);
	Len2 = (int)ftell(pFile2);

	if (Len1!=Len2)		printf("\n File Length is not the same");

	if (fseek(pFile1, 0, SEEK_SET)!=0)		printf("\n File %s seek error !", sSrc);
	if (fseek(pFile2, 0, SEEK_SET)!=0)		printf("\n File %s seek error !", sSrc);

	pBuf1 = (char *)malloc(Len1);
	pBuf2 = (char *)malloc(Len2);
	Len1 = (int)fread((void *)pBuf1, 1, Len1, pFile1);
	Len2 = (int)fread((void *)pBuf2, 1, Len2, pFile2);

	ptr1 = pBuf1;
	ptr2 = pBuf2;
	while(Len1 >=0)
	{
		if (*ptr1++ != *ptr2++)
		{	printf("\n Byte[%d] is not the same!", Len2-Len1);
			break;
		}
		Len1--;
	}
	if (Len1<0)
		printf("\n Success : File content is the same");
	free(pBuf1);
	free(pBuf2);
	fclose(pFile1);
	fclose(pFile2);

	printf("\n");
	return;
}



