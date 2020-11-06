#ifndef __RESAMPLING_H__
#define __RESAMPLING_H__

// -------------------------------------------------
// Description : Get pointer of version string
// Return :
// -------------------------------------------------
void GM_Resampling_ver(const char ** pVersion);

// -------------------------------------------------
// Description : initialize GM_Resampling library, mainly
//          for memory allocation.
// Return : 0 (Success)
//          101 (outCount or inCount equals to zero)
//          102 (fail to calloc memory space)
// -------------------------------------------------
int GM_Resampling_Init(int inCount, int outCount, int OneFrameMode);

// -------------------------------------------------
// Description : main program
//
// Return : 0 (Success)
//          101 (outCount or inCount equals to zero)
// -------------------------------------------------
int GM_Resampling(
	void *  pInBuf,   /*Input Buffer pointer*/
	void *  pOutBuf   /*Output Buffer pointer*/
);

// -------------------------------------------------
// Description : Ends GM_Resampling library, mainly for
//              releasing allocated memory space.
// Return :
// -------------------------------------------------
void GM_Resampling_Destroy(void);

#endif
