/*+++ *******************************************************************\ 
* 
*  Copyright and Disclaimer: 
*  
*     --------------------------------------------------------------- 
*     This software is provided "AS IS" without warranty of any kind, 
*     either expressed or implied, including but not limited to the 
*     implied warranties of noninfringement, merchantability and/or 
*     fitness for a particular purpose.
*     --------------------------------------------------------------- 
*   
*     Copyright (c) 2013 Conexant Systems, Inc. 
*     All rights reserved. 
*
\******************************************************************* ---*/ 

#ifndef _CX2684X_COLIBRI_H
#define _CX2684X_COLIBRI_H

#include "Comm.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* Colibri operating frequency clock */
#define COLIBRI_FCLK_720MHZ_CRYSTAL_48MHZ          0x0F
#define COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ          0x1E
#define COLIBRI_FCLK_624MHZ_CRYSTAL_48MHz          0x0D
#define COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz          0x1A

extern int CX2684x_initColibri(const CX_COMMUNICATION *p_comm, const unsigned char freq_clk);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CX2684X_COLIBRI_H */