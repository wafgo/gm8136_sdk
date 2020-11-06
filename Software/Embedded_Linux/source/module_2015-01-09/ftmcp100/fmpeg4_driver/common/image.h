#ifndef _IMAGE_H_
#define _IMAGE_H_

#ifdef LINUX
#include "../mpeg4_decoder/image_dd.h"
#else
#include "image_dd.h"
#endif

void image_swap(IMAGE * image1,
				IMAGE * image2);
#endif							/* _IMAGE_H_ */
