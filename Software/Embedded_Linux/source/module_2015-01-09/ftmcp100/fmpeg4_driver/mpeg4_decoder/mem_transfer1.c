#define MEM_TRANSFER1_GLOBALS

#ifdef LINUX
#include "../common/portab.h"
#include "mem_transfer1.h"
#include "../common/define.h"
#else
#include "portab.h"
#include "mem_transfer1.h"
#include "define.h"
#endif

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
void 
transfer_nxm_copy_ben(uint8_t * dst,
				   const uint8_t * src,
				   const int n,
				   const int m)
{
	uint32_t i, j;

	for (j = 0; j < m; j++) {
		for (i = 0; i < n; i++) {
			*(dst ++) = *(src ++);
		}
		dst += (8 - n);
		src += (8 - n);
	}
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
void 
transfer8x8_copy_ben(DECODER * const dec,
				int curoffset,
				uint8_t * cur,
				uint8_t * ref,
				int refx,
				int refy)
{
	int refoffset;

	refoffset = ((refx >> 3) + ((int)dec->mb_width * 2) * (refy >> 3)) *SIZE_U;
	transfer_nxm_copy_ben(cur + curoffset,
					ref + refoffset
					+ (refy & 7) * 8 + (refx & 7),
					8 - (refx & 7),
					8 - (refy & 7));
	if ((refx & 7) != 0)
		transfer_nxm_copy_ben(cur + curoffset + (8 - (refx & 7)),
						ref + refoffset + 64 + (refy & 7) * 8,
						refx & 7,
						refy & 7);
	if ((refy & 7) != 0) {
		curoffset += (8 - (refx & 7)) * 8;
		refoffset += (int)dec->mb_width * 2 * SIZE_U;
		transfer_nxm_copy_ben(cur + curoffset,
						ref + refoffset + (refx & 7),
						8 - (refx & 7),
						8 - (refy & 7));
		if ((refx & 7) != 0)
			transfer_nxm_copy_ben(cur + curoffset + (8 - (refx & 7)),
							ref + refoffset + 64,
							8 - (refx & 7),
							8 - (refy & 7));
	}
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
