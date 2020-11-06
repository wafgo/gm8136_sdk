#ifdef LINUX
#include <linux/kernel.h>
#include "../common/portab.h"
#include "ftmcp100.h"
#include "../common/image.h"
#include "../common/define.h"
#include "Mp4Venc.h"
#include "encoder.h"
#else
#include "portab.h"
#include "ftmcp100.h"
#include "image.h"
#include "define.h"
#include "mp4venc.h"
#include "encoder.h"
#endif

#define SAFETY	64
#define EDGE_SIZE 16
#define EDGE_SIZE2  (EDGE_SIZE/2)

/*
size creat:

	  EDGE_SIZE		mb_width * PIXEL_Y
		    |	   /							      \
		/	\/								\
		-------------------------------------
		|	|								|\
		|	|								| EDGE_SIZE
		|	|								|/
		-------------------------------------
			|								|\
			|								| \
			|								|
			|								|
			|								|
			|								|
			|								|  mb_height * PIXEL_Y
			|								|
			|								|
			|								|
			|								| /
			|								|/
			-------------------------------------
			|								|	|\
			|								|	| EDGE_SIZE
			|								|	|/
			-------------------------------------
											\	/
											EDGE_SIZE

*/

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
int32_t 
image_create_edge(IMAGE * image,
			 uint32_t mbwidth,
			 uint32_t mbheight,
			 Encoder_x * pEnc_x)
{
	DMA_MALLOC_PTR_enc pfnDmaMalloc = pEnc_x->pfnDmaMalloc;
#if 0
	DMA_FREE_PTR_enc pfnDmaFree = pEnc_x->pfnDmaFree;
	image->y = (uint8_t *)pfnDmaMalloc(
				mbwidth * (2 + mbheight)  * PIXEL_Y * PIXEL_Y+ 2 * EDGE_SIZE * EDGE_SIZE,
				pEnc_x->u32CacheAlign, pEnc_x->u32CacheAlign, (void **)&(image->y_phy));
	if (image->y == NULL) {
#ifdef LINUX
    printk("Fail to allocate image->y 0x%x!\n",mbwidth*(2+mbheight)*PIXEL_Y*PIXEL_Y+2*EDGE_SIZE*EDGE_SIZE);
#endif
		return -1;
	}
	image->u =  (uint8_t *)pfnDmaMalloc(
				mbwidth * (2 + mbheight)  * PIXEL_U  * PIXEL_U +2 * EDGE_SIZE2 * EDGE_SIZE2,
				pEnc_x->u32CacheAlign, pEnc_x->u32CacheAlign, (void **)&(image->u_phy));
	if (image->u == NULL) {
#ifdef LINUX
        printk("Fail to allocate image->u 0x%x!\n",mbwidth*(2+mbheight)*PIXEL_U*PIXEL_U+2*EDGE_SIZE2*EDGE_SIZE2);
#endif
		pfnDmaFree(image->y, image->y_phy);
		image->y=image->y_phy=0;
		return -1;
	}

	image->v = (uint8_t *)pfnDmaMalloc(
				mbwidth * (2 + mbheight)  * PIXEL_V  * PIXEL_V + 2 * EDGE_SIZE2 * EDGE_SIZE2,
				pEnc_x->u32CacheAlign, pEnc_x->u32CacheAlign, (void **)&(image->v_phy));
	if (image->v == NULL) {
#ifdef LINUX
        printk("Fail to allocate image->v 0x%x!\n",mbwidth*(2+mbheight)*PIXEL_V*PIXEL_V+2*EDGE_SIZE2*EDGE_SIZE2);
#endif
		pfnDmaFree(image->u, image->u_phy);
		image->u=image->u_phy=0;
		pfnDmaFree(image->y, image->y_phy);
		image->y=image->y_phy=0;
		return -1;
	}

	image->y += mbwidth * PIXEL_Y * EDGE_SIZE + EDGE_SIZE * EDGE_SIZE;
	image->u += mbwidth * PIXEL_U * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2;
	image->v += mbwidth * PIXEL_V * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2;
	image->y_phy += mbwidth * PIXEL_Y * EDGE_SIZE + EDGE_SIZE * EDGE_SIZE;
	image->u_phy += mbwidth * PIXEL_U * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2;
	image->v_phy += mbwidth * PIXEL_V * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2;
#else
    int y_size, u_size, v_size;
    y_size = mbwidth * (2 + mbheight)  * PIXEL_Y * PIXEL_Y+ 2 * EDGE_SIZE * EDGE_SIZE;
    u_size = mbwidth * (2 + mbheight)  * PIXEL_U  * PIXEL_U +2 * EDGE_SIZE2 * EDGE_SIZE2;
    v_size = mbwidth * (2 + mbheight)  * PIXEL_V  * PIXEL_V + 2 * EDGE_SIZE2 * EDGE_SIZE2;

    image->y = (uint8_t *)pfnDmaMalloc(
        y_size + u_size + v_size, pEnc_x->u32CacheAlign, pEnc_x->u32CacheAlign, (void **)&(image->y_phy));
    if (NULL == image->y) {
        printk("Fail to allocate image->rec, size %d!\n", y_size + u_size + v_size);
		return -1;
	}
    image->u = image->y + y_size;
    image->v = image->u + u_size;
    image->u_phy = image->y_phy + y_size;
    image->v_phy = image->u_phy + u_size;

    image->y += mbwidth * PIXEL_Y * EDGE_SIZE + EDGE_SIZE * EDGE_SIZE;
	image->u += mbwidth * PIXEL_U * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2;
	image->v += mbwidth * PIXEL_V * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2;
	image->y_phy += mbwidth * PIXEL_Y * EDGE_SIZE + EDGE_SIZE * EDGE_SIZE;
	image->u_phy += mbwidth * PIXEL_U * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2;
	image->v_phy += mbwidth * PIXEL_V * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2;
#endif
	return 0;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
void 
image_adjust_edge(IMAGE * image,
			      uint32_t mbwidth,
			      uint32_t mbheight,unsigned char *addr)
{
    // the addr are user-provided physical address
    image->y_phy = addr;
    image->u_phy = image->y_phy+((mbwidth*(2+mbheight)+2)*256);
    image->v_phy = image->u_phy+((mbwidth*(2+mbheight)+2)*64);

	//image->y_virt += mbwidth * PIXEL_Y * EDGE_SIZE + EDGE_SIZE * EDGE_SIZE;
	//image->u_virt += mbwidth * PIXEL_U * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2;
	//image->v_virt += mbwidth * PIXEL_V * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2;
	image->y_phy += mbwidth * PIXEL_Y * EDGE_SIZE + EDGE_SIZE * EDGE_SIZE;
	image->u_phy += mbwidth * PIXEL_U * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2;
	image->v_phy += mbwidth * PIXEL_V * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
void 
image_destroy_edge(IMAGE * image,
			  uint32_t mbwidth,
			  Encoder_x * pEnc_x)
{
	DMA_FREE_PTR_enc pfnDmaFree = pEnc_x->pfnDmaFree;
#if 0
   	if (image->y) {
		pfnDmaFree(image->y - (mbwidth * PIXEL_Y * EDGE_SIZE + EDGE_SIZE * EDGE_SIZE),
				image->y_phy - (mbwidth * PIXEL_Y * EDGE_SIZE + EDGE_SIZE * EDGE_SIZE));
	}
	if (image->u) {
		pfnDmaFree(image->u - (mbwidth * PIXEL_U * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2),
				image->u_phy - (mbwidth * PIXEL_U * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2));
	}
	if (image->v) {
		pfnDmaFree(image->v - (mbwidth * PIXEL_V * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2),
				image->v_phy - (mbwidth * PIXEL_V * EDGE_SIZE2 + EDGE_SIZE2 * EDGE_SIZE2));
	}
#else
    if (image->y) {
		pfnDmaFree(image->y - (mbwidth * PIXEL_Y * EDGE_SIZE + EDGE_SIZE * EDGE_SIZE),
				image->y_phy - (mbwidth * PIXEL_Y * EDGE_SIZE + EDGE_SIZE * EDGE_SIZE));
    }
#endif
	image->y=image->y_phy=0;
	image->u=image->u_phy=0;
	image->v=image->v_phy=0;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
