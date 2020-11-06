#ifndef _MBFUNCTIONS_H
#define _MBFUNCTIONS_H

#include "encoder.h"
#include "bitstream.h"

/*****************************************************************************
 * Prototypes
 ****************************************************************************/



/* MBTransQuant.c */

void MBTransQuantIntra(const Encoder * pEnc,
					   FRAMEINFO * frame,
					   MACROBLOCK_E * pMB,
					   const uint32_t x_pos,    /* <-- The x position of the MB to be searched */
					   const uint32_t y_pos,
		 			   MACROBLOCK_pass * ptMBpass);
	



uint8_t MBTransQuantInter(const Encoder * pEnc,
						uint32_t buf_sel);
#endif
