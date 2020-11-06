#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

////////////////////////////////////////////Standard Include///////////////////////////////////////////////
////////////////////////////////////////////Project Include////////////////////////////////////////////////
///////////////////////////////////////////////////Macro///////////////////////////////////////////////////
#ifdef _MAIN_C_
#define EXTERN
#else
#define EXTERN extern
#endif


#define large
#define xdata
#define sbit    unsigned char
#define P2      0xFF

//////////////////////////////////////////Global Type Definitions//////////////////////////////////////////

typedef short SHORT, *PSHORT ;
//typedef unsigned short ushort, *pushort ;
typedef unsigned short *pushort ;
typedef unsigned short USHORT, *PUSHORT ;
typedef unsigned short word, *pword ;
typedef unsigned short WORD, *PWORD ;

typedef long LONG, *PLONG ;
//typedef unsigned long ulong, *pulong ;
typedef unsigned long *pulong ;
typedef unsigned long ULONG, *PULONG ;
typedef unsigned long dword, *pdword ;
typedef unsigned long DWORD, *PDWORD ;


typedef enum _SYS_STATUS {
    ER_SUCCESS = 0,
    ER_FAIL,
    ER_RESERVED
} SYS_STATUS ;


///////////////////////////////////////Global Function Definitions////////////////////////////////////////
///////////////////////////////////////Global Variable Definitions/////////////////////////////////////////
EXTERN unsigned int EX0;
EXTERN unsigned int ET2;
EXTERN unsigned int TR2;
EXTERN unsigned int TMR2CN;
EXTERN unsigned int P3;
EXTERN unsigned int P1;
EXTERN unsigned int CKCON;
EXTERN unsigned int TMR2RL;
EXTERN unsigned int TMR2;
//EXTERN unsigned char COMP_MODE;
EXTERN unsigned char Monitor_CompSwitch (void);
              // Set to reload immediately



#undef EXTERN
#endif // _TYPEDEF_H_
