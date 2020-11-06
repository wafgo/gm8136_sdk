#ifndef _IMAGE_D_H_
#define _IMAGE_D_H_

#ifdef LINUX
#include "../common/portab.h"
#include "decoder.h"
#include "image_dd.h"
#else
#include "portab.h"
#include "decoder.h"
#include "image_dd.h"
#endif

int32_t image_create(IMAGE * image,
					uint32_t mbwidth,
					uint32_t mbheight,
					DECODER_ext * ptdec);
void image_destroy(IMAGE * image,
					uint32_t mbwidth,
					DECODER_ext * ptdec);


#endif							/* _IMAGE_D_H_ */
