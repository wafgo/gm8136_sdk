#ifndef _IMAGE_DD_H_
#define _IMAGE_DD_H_

#ifdef LINUX
#include "../common/portab.h"
#else
#include "portab.h"
#endif
//#define EDGE_SIZE 16

typedef struct
{
	uint8_t *y;
	uint8_t *u;
	uint8_t *v;
	uint8_t *y_phy;
	uint8_t *u_phy;
	uint8_t *v_phy;
}
IMAGE;

#endif							/* _IMAGE_DD_H_ */
