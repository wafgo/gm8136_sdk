#ifndef _GLOBAL_D_H_
#define _GLOBAL_D_H_

#ifdef LINUX
#include "../common/portab.h"
#else
#include "portab.h"
#endif

/* --- macroblock modes --- */
#define MODE_INTER		0
#define MODE_INTER_Q		1
#define MODE_INTER4V		2
#define MODE_INTRA		3
#define MODE_INTRA_Q		4
#define MODE_STUFFING		7
#define MODE_NOT_CODED	16

/* --- bframe specific --- */

#define MODE_DIRECT			0
#define MODE_INTERPOLATE		1
#define MODE_BACKWARD		2
#define MODE_FORWARD			3
#define MODE_DIRECT_NONE_MV	4


typedef struct
{
	uint32_t bufa;
	uint32_t bufb;
	uint32_t buf;
	int32_t pos;
	int32_t start_pos;
	uint32_t *tail;
	uint32_t *start;
	uint32_t length;
	uint32_t * start_phy;
	uint32_t * tail_phy;
}
Bitstream;

// sizeof (MACROBLOCK) must be 24 byte
typedef struct
{
	int16_t mb_jump;				// mb jump offset from the prevous correct one
	int16_t mode;
	union VECTOR1 mvs[4];
	union VECTOR1 MVuv;
}
MACROBLOCK;
#define MB_SIZE	(sizeof (MACROBLOCK))


typedef struct
{
	int32_t x;			// current mb x
	uint32_t y;			// current mb y
	int32_t mbpos;		// current mb position
	uint32_t toggle;		// indicate which table will be used;

	int quant;			// absolute quant
	int cbp;
}
MACROBLOCK_b;

// useful macros

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))
#define ABS(X)    (((X)>0)?(X):-(X))
#define SIGN(X)   (((X)>0)?1:-1)
#ifdef TWO_P_INT
//#pragma import(__use_no_semihosting_swi)
#endif

#endif /* _GLOBAL_D_H_ */

