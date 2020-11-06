#ifdef LINUX
#include <linux/kernel.h>
#include "../common/portab.h"
#include "image_d.h"
#include "../common/define.h"
#else
#include "portab.h"
#include "image_d.h"
#include "define.h"
#endif

#define SAFETY	64
#define EDGE_SIZE2  (EDGE_SIZE/2)

int32_t
image_create(IMAGE * image,
			uint32_t mbwidth,
			uint32_t mbheight,
			DECODER_ext * ptdec)
{
#if 1
	DMA_MALLOC_PTR_dec pfnDmaMalloc = ptdec->pfnDmaMalloc;
	uint8_t u8align = ptdec->u32CacheAlign;

    int y_size = mbwidth * mbheight * SIZE_Y;
    int u_size = mbwidth * mbheight * SIZE_U;
    int v_size = mbwidth * mbheight * SIZE_V;

    image->y = (uint8_t *)pfnDmaMalloc(
				y_size + u_size + v_size,
				u8align, u8align, (void **)&image->y_phy);
    if (NULL == image->y) {
        printk("Fail to allocate reference buffer 0x%x!\n", y_size + u_size + v_size);
        image->y=image->y_phy=0;
        image->u=image->u_phy=0;
        image->v=image->v_phy=0;
		return -1;
    }
    image->u = image->y + y_size;
    image->v = image->u + u_size;
    image->u_phy = image->y_phy + y_size;
    image->v_phy = image->u_phy + u_size;
#else
	DMA_MALLOC_PTR_dec pfnDmaMalloc = ptdec->pfnDmaMalloc;
	DMA_FREE_PTR_dec pfnDmaFree = ptdec->pfnDmaFree;
	uint8_t u8align = ptdec->u32CacheAlign;

	image->y = (uint8_t *)pfnDmaMalloc(
				mbwidth * mbheight * SIZE_Y,
				u8align/*8*/, u8align, (void **)&image->y_phy);
	if (image->y == NULL) {
#ifdef LINUX
        printk("Fail to allocate image->y 0x%x!\n",mbwidth * mbheight * SIZE_Y);
#endif
		return -1;
	}
	image->u = (uint8_t *)pfnDmaMalloc(
				mbwidth * mbheight * SIZE_U,
				u8align/*8*/, u8align, (void **)&image->u_phy);
	if (image->u == NULL) {
#ifdef LINUX
        printk("Fail to allocate image->u 0x%x!\n",mbwidth * mbheight * SIZE_U);
#endif
		pfnDmaFree(image->y, image->y_phy);
		image->y=image->y_phy=0;
		return -1;
	}

	image->v = (uint8_t *)pfnDmaMalloc(
				mbwidth * mbheight * SIZE_V,
				u8align/*8*/, u8align, (void **)&image->v_phy);
	if (image->v == NULL) {
#ifdef LINUX
        printk("Fail to allocate image->v 0x%x!\n",mbwidth * mbheight * SIZE_V);
#endif
		pfnDmaFree(image->y, image->y_phy);
		image->y=image->y_phy=0;
		pfnDmaFree(image->u, image->u_phy);
		image->u=image->u_phy=0;
		return -1;
	}
#endif
	return 0;
}

void
image_destroy(IMAGE * image,
			uint32_t mbwidth,
	 		DECODER_ext * ptdec)
{
	DMA_FREE_PTR_dec pfnDmaFree = ptdec->pfnDmaFree;
#if 1
    if (image->y)
        pfnDmaFree(image->y, image->y_phy);
#else
	if (image->y)
		pfnDmaFree(image->y, image->y_phy);
	if (image->u)
		pfnDmaFree(image->u, image->u_phy);
	if (image->v)
		pfnDmaFree(image->v, image->v_phy);
#endif
    image->y=image->y_phy=0;
    image->u=image->u_phy=0;
    image->v=image->v_phy=0;
}



