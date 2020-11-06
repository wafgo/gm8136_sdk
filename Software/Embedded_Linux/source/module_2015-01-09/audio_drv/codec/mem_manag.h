/*
***********************************************************************************
** FARADAY(MTD)/GRAIN CONFIDENTIAL
** Copyright FARADAY(MTD)/GRAIN Corporation All Rights Reserved.
**
** The source code contained or described herein and all documents related to
** the source code (Material) are owned by FARADAY(MTD)/GRAIN Corporation Title to the 
** Material remains with FARADAY(MTD)/GRAIN Corporation. The Material contains trade 
** secrets and proprietary and confidential information of FARADAY(MTD)/GRAIN. The 
** Material is protected by worldwide copyright and trade secret laws and treaty 
** provisions. No part of the Material may be used, copied, reproduced, modified, 
** published, uploaded, posted, transmitted, distributed, or disclosed in any way 
** without FARADAY(MTD)/GRAIN prior express written permission.
** 
** No license under any patent, copyright, trade secret or other intellectual 
** property right is granted to or conferred upon you by disclosure or delivery
** of the Materials, either expressly, by implication, inducement, estoppel or 
** otherwise. Any license under such intellectual property rights must be express
** and approved by FARADAY(MTD)/GRAIN in writing.
**
** 
***********************************************************************/



/*******************************************************************************
	File:		mem_manag.h

	Content:	Memory alloc alignments functions

*******************************************************************************/

#ifndef _mem_manag_h
#define _mem_manag_h

//#include "voMem.h"
//#include "voIndex.h"
#include "typedefs.h"
#include "typedef.h"

extern void *Alignm_mem(unsigned int size, unsigned char alignment);
extern void Aligmem_free(void *mem_ptr);

#endif	/* __VO_MEM_ALIGN_H__ */



