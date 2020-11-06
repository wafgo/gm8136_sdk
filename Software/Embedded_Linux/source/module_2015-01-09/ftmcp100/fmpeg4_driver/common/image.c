#include "portab.h"
#include "image.h"

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
void
image_swap(IMAGE * image1,
		   IMAGE * image2)
{
	uint8_t *tmp;

	tmp = image1->y;
	image1->y = image2->y;
	image2->y = tmp;

	tmp = image1->u;
	image1->u = image2->u;
	image2->u = tmp;

	tmp = image1->v;
	image1->v = image2->v;
	image2->v = tmp;

	tmp = image1->y_phy;
	image1->y_phy = image2->y_phy;
	image2->y_phy = tmp;

	tmp = image1->u_phy;
	image1->u_phy = image2->u_phy;
	image2->u_phy = tmp;

	tmp = image1->v_phy;
	image1->v_phy = image2->v_phy;
	image2->v_phy = tmp;
}

#endif

