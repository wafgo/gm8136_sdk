#ifndef _IMAGE_E_H_
#define _IMAGE_E_H_

#ifdef LINUX
#include "../common/portab.h"
#include "ftmcp100.h"
#include "encoder.h"
#include "../common/image.h"
#else
#include "portab.h"
#include "ftmcp100.h"
#include "encoder.h"
#include "image.h"
#endif
int32_t image_create_edge(IMAGE * image,
				uint32_t mbwidth,
				uint32_t mbheight,
				Encoder_x * pEnc_x);
void image_destroy_edge(IMAGE * image,
				uint32_t mbwidth,
				Encoder_x * pEnc_x);

void image_adjust_edge(IMAGE * image,
				uint32_t mbwidth,
				uint32_t mbheight,
				unsigned char *addr);


#endif							/* _IMAGE_E_H_ */
