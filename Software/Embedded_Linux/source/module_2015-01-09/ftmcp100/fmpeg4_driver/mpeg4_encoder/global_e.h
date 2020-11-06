#ifndef _GLOBAL_E_H_
#define _GLOBAL_E_H_

#ifdef LINUX
#include "../common/portab.h"
#include "../common/dma.h"
#include "Mp4Venc.h"
#else
#include "portab.h"
#include "dma.h"
#include "mp4venc.h"
#endif

/* --- macroblock modes --- */

#define MODE_INTER		0
#define MODE_INTER_Q	1
#define MODE_INTER4V	2
#define	MODE_INTRA		3
//#define MODE_INTRA_Q	4
#define MODE_STUFFING	7
#define MODE_NOT_CODED	16

/* --- bframe specific --- */

#define MODE_DIRECT			0
#define MODE_INTERPOLATE	1
#define MODE_BACKWARD		2
#define MODE_FORWARD		3
#define MODE_DIRECT_NONE_MV	4

/*
typedef struct
{
	uint32_t *tail;
	uint32_t *start;
	uint32_t length;
}
Bitstream;
*/

#if 1

// sizeof (MACROBLOCK) must be 24 byte ====> XXXXXX need to be modified in RISC version (ivan)
typedef struct
{
	int16_t quant;	
	int16_t mode;
	union VECTOR1 mvs[4];
	int32_t sad16;        // SAD value for inter-VECTOR
} MACROBLOCK_E;

#define MB_E_SIZE  (sizeof(MACROBLOCK_E))

#else

typedef struct
{
	// decoder/encoder 
	int mode;
	int quant;					// absolute quant

	// encoder specific

	int mv16x_0;
	int mv16y_0;
	int mv16x_1;
	int mv16y_1;
	int mv16x_2;
	int mv16y_2;
	int mv16x_3;
	int mv16y_3;
	
	int32_t dev;
	int32_t sad16;        // SAD value for inter-VECTOR
}
MACROBLOCK_E;
#endif
//---------------------------------------------------------------------------
typedef struct {
	// motion estimation related variables
	int32_t MVZ;
	uint32_t MB_mode; // use Inter if MB_mode == 1    
	uint32_t ME_COMMAND;
	uint32_t even_odd_1;  //  even_odd_1 was used to control the selection bewteen  (CUR_Y0,CUR_U0,CUR_V0) buffer and (CUR_Y1,CUR_U1,CUR_V1) buffer	
	uint32_t even_odd_I;  //  even_odd_I was used to control the selection bewteen LOCAL_PREDICTOR0 's double buffers
	uint32_t triple_buffer_selector;

	int32_t Raddr;
//	int32_t Raddr23;
	uint32_t acdc_status; // acdc_status is number to fill in the field of mcctl register
}
MACROBLOCK_pass;


typedef struct
{
	int32_t x;			// current mb x
	uint32_t y;		// current mb y
	int32_t mbpos;		// current mb position
	uint32_t toggle;
	uint32_t mvz;		// whether current motion vector is zero or not
	//#ifdef GM8180_OPTIMIZATION
	  uint32_t mecmd_pipe;	// to indicate the me command execution status
	  union VECTOR1 pmv[5];
	//#endif
}
MACROBLOCK_E_b;

// useful macros

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))
#define ABS(X)    (((X)>0)?(X):-(X))
#define SIGN(X)   (((X)>0)?1:-1)


#endif /* _GLOBAL_E_H_ */
